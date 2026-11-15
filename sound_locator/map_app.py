"""Ascent sound-map desktop UI. No game process access and no truth-file reads.

Recording replay reads only audio.wav and listener_pose.jsonl. Live capture is
system endpoint loopback; camera pose is supplied separately by the caller.
"""
from __future__ import annotations
from collections import deque
from dataclasses import asdict
import colorsys
import json
import math
from pathlib import Path
import queue
import sys
import threading
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

import numpy as np
from PIL import Image, ImageTk
from core import RATE, Profile, read_wav
from capture import Capture, devices
from event_audio import detect_events
from learned_audio import LearnedAudioModel
from map_geometry import MapGeometry
from scene import SceneAnalyzer, PoseTimeline
from gui_paths import GraphPathCache

SOURCE = Path(__file__).resolve().parent
ASSETS = Path(getattr(sys, "_MEIPASS", SOURCE))
APP_HOME = Path(sys.executable).parent if getattr(sys, "frozen", False) else SOURCE


def default_paths():
    packaged = ASSETS / "models"
    if packaged.is_dir():
        return ASSETS / "maps/ascent.json", packaged / "ascent-profile.npz", packaged / "ascent-neural.npz"
    return (SOURCE / "maps/ascent.json", SOURCE / "validation/ascent-footstep-baseline/profile.npz",
            SOURCE / "validation/learned-ascent-onset/learned-model.npz")


def default_range_path():
    """The optional model may be supplied after the application was installed."""
    external=APP_HOME/"models/ascent-range.npz"
    if external.is_file():return external
    if (ASSETS/"models").is_dir():return ASSETS/"models/ascent-range.npz"
    return SOURCE/"validation/range-ascent/range-model.npz"


def range_hint_text(value):
    """Render only accepted weak evidence, never a rejected class/context result."""
    if not value or not value.get("informative",False):return ""
    try:
        median=float(value["median_m"]);low,high=map(float,value["interval_m"])
    except (KeyError,TypeError,ValueError):return ""
    if not np.isfinite([median,low,high]).all() or not 0<low<=median<=high:return ""
    return f"脚步距离弱提示：约 {median:.1f} m，候选区间 {low:.1f}–{high:.1f} m（非精确距离）"


def normalized_pose(data, geometry, now=None):
    """Validate an externally supplied live pose, never obtain a source position."""
    stamp = float(data["timestamp"])
    age = (time.time() if now is None else now) - stamp
    if not math.isfinite(stamp) or age > 1 or age < -.25:
        raise ValueError("姿态数据过期或时间戳不一致")
    if "world_x" in data and "world_y" in data:
        x, y = float(data["world_x"]), float(data["world_y"])
        if "yaw" not in data and "yaw_deg" not in data:
            raise ValueError("姿态必须包含已知 yaw 或 yaw_deg")
        yaw = float(data.get("yaw", data.get("yaw_deg")))
    else:
        u, v = float(data["x"]), float(data["y"])
        if not 0 <= u <= 1 or not 0 <= v <= 1:
            raise ValueError("地图坐标 X/Y 必须在 0～1 之间")
        x, y = geometry.map_to_world([u, v])
        yaw = float(data["heading_deg"])
    z = float(data.get("world_z", 0))
    if not np.isfinite([x, y, z, yaw]).all():
        raise ValueError("姿态包含无效数值")
    return {"timestamp": stamp, "world_x": float(x), "world_y": float(y), "world_z": z, "yaw": yaw}


def track_table_rows(state):
    """Return every track hypothesis, without a presentation source-count cap."""
    rows = []
    for t in state.get("tracks", []):
        age = float(t.get("seconds_since_evidence", 0))
        status = "候选" if t.get("status") == "tentative" else "持续跟踪"
        if age > 1: status = "暂未听到"
        u, v = t["map_uv"]
        location = f"{u:.3f}, {v:.3f}（估计）" if t.get("position_concentrated",False) else "位置未收敛"
        rows.append((str(t["track_id"]), status, location, f"{t['radius90_cm']/100:.1f} m"))
    return rows


def load_session_inputs(path):
    """Only the two estimator inputs are opened, never an evaluation truth file."""
    path=Path(path)
    audio,poses=read_wav(path/"audio.wav"),PoseTimeline.load(path/"listener_pose.jsonl")
    # Device frame counters may repeat across two host pose polls. Preserve the
    # recording, but do not treat conflicting poses at one audio time as exact.
    unique,first,counts=np.unique(poses.times,return_index=True,return_counts=True)
    ambiguous=[]
    if np.any(counts>1):
        for t,i,n in zip(unique,first,counts):
            if n>1 and (np.ptp(poses.position[i:i+n],axis=0).max()>1e-6 or np.ptp(poses.yaw[i:i+n])>1e-6):
                ambiguous.append((float(t)-.05,float(t)+.05))
        keep=first+counts-1
        poses=PoseTimeline([dict(t_audio_s=float(poses.times[i]),world_x=float(poses.position[i,0]),
                                 world_y=float(poses.position[i,1]),world_z=float(poses.position[i,2]),
                                 yaw=float(poses.yaw[i])) for i in keep])
    poses.ambiguous_intervals=ambiguous
    return audio,poses


def load_range_background(path,enabled):
    """Optional separate recording. Never replace it with a detected event window."""
    background=Path(path)/"background.wav"
    if not enabled or not background.is_file():return None
    samples=read_wav(background)
    if len(samples)<1024:raise ValueError("背景录音过短，无法用于距离实验")
    return samples


def pose_window_covered(poses,start,end):
    """Require nearby pose samples throughout a window, not across a clock gap."""
    times=poses.times
    if start<times[0]-.05 or end>times[-1]+.05:return False
    if any(start<=right and end>=left for left,right in getattr(poses,"ambiguous_intervals",[])):return False
    lo=max(0,int(np.searchsorted(times,start,side="right"))-1)
    hi=min(len(times)-1,int(np.searchsorted(times,end,side="left")))
    if hi==lo:
        if hi<len(times)-1:hi+=1
        elif lo>0:lo-=1
    return hi>lo and bool(np.all(np.diff(times[lo:hi+1])>0) and np.max(np.diff(times[lo:hi+1]))<=.15)


class LiveEventEngine:
    """Small audio ring buffer; every mature onset is consumed at most once.

    The first packet fixes an approximate audio-to-wall-clock epoch. WASAPI
    delivery timestamps are not hardware-synchronized pose timestamps.
    """
    def __init__(self, analyzer):
        self.analyzer = analyzer
        self.audio = np.empty((0, 2))
        self.total = 0
        self.epoch = None
        self.last_onset = -RATE
        self.last_idle = -1.
        self.skipped_unsynchronized_events = 0

    def consume(self, samples, delivered, pose_rows):
        x = np.asarray(samples)
        if self.epoch is None: self.epoch = delivered - len(x)/RATE
        self.total += len(x)
        self.audio = np.concatenate((self.audio, x))[-2*RATE:]
        offset = self.total-len(self.audio)
        if len(pose_rows) < 2 or delivered-pose_rows[-1]["timestamp"] > 1:
            return [], "等待同步位置和视角"
        poses = PoseTimeline([dict(t_audio_s=p["timestamp"]-self.epoch,
                                  world_x=p["world_x"], world_y=p["world_y"],
                                  world_z=p.get("world_z", 0), yaw=p["yaw"]) for p in pose_rows])
        states = []
        for event in detect_events(self.audio):
            onset = offset+event["onset_sample"]
            if onset <= self.last_onset+int(.18*RATE): continue
            self.last_onset = onset
            start, end = event["start_sample"], event["end_sample"]
            center=(offset+event["center_sample"])/RATE
            last=self.analyzer.tracker.last_time
            # A pose outage can leave old events in the ring when poses resume.
            # Do not extrapolate these events or send them backward to the tracker.
            start_time,end_time=(offset+start)/RATE,(offset+end)/RATE
            if not pose_window_covered(poses,start_time,end_time) or (last is not None and center<=last):
                self.skipped_unsynchronized_events+=1
                continue
            row = self.analyzer.event(self.audio[start:end], offset+start, poses)
            row["audio_event"] = {**event, "start_sample":offset+start, "end_sample":offset+end,
                                  "center_sample":offset+event["center_sample"], "onset_sample":onset}
            states.append(row)
        # Stay behind released event centers, so later events cannot travel backward
        # in tracker time. This also expires stale hypotheses during quiet intervals.
        idle = self.total/RATE-.25
        last = self.analyzer.tracker.last_time
        if (not states and idle > 0 and idle-self.last_idle >= .25 and (last is None or idle > last)
                and pose_window_covered(poses,idle,idle)):
            pose = poses.at(idle)
            row = self.analyzer.tracker.update([], pose)
            row.update(listener_pose=asdict(pose), audio_status="waiting_for_transient",
                       observations=[], neural_status="no_new_event")
            states.append(row); self.last_idle = idle
        note="实时采集 · 视角时间同步为近似值"
        if self.skipped_unsynchronized_events:
            note+=f" · {self.skipped_unsynchronized_events} 个旧事件缺同步姿态"
        return states,note


class SoundMapApp:
    def __init__(self, root):
        self.root = root
        root.title("声迹 · Ascent 声源地图")
        root.geometry("1250x880"); root.minsize(1080, 790)
        root.configure(bg="#f4f5f6")
        self.map_path, self.profile_path, self.neural_path = default_paths()
        self.range_path=default_range_path()
        self.geometry = MapGeometry.load(self.map_path)
        with Image.open(self.geometry.image_path) as im: self.map_image = im.convert("RGB").copy()
        self.map_photo = None; self.map_photo_size = 0
        self.graph_paths=None
        self.latest = {}; self.generation = 0; self.capture = None
        self.worker_stop = threading.Event(); self.worker = None
        self.events = queue.Queue(maxsize=10); self.audio_queue = None
        self.pose_lock = threading.Lock(); self.pose_rows = deque(maxlen=1600)
        self.running = False; self.last_table = None
        self.mode = "idle"
        self.build(); self.refresh_devices(); self.update_model_label()
        root.protocol("WM_DELETE_WINDOW", self.close)
        root.after(50, self.tick)

    def build(self):
        style = ttk.Style(); style.theme_use("clam")
        style.configure("TFrame", background="#f4f5f6")
        style.configure("TLabel", background="#f4f5f6", foreground="#253241", font=("Microsoft YaHei UI",10))
        style.configure("TLabelframe", background="#f4f5f6")
        style.configure("TLabelframe.Label", background="#f4f5f6", font=("Microsoft YaHei UI",10,"bold"))
        style.configure("TButton", font=("Microsoft YaHei UI",10), padding=6)
        style.configure("Treeview", rowheight=28, font=("Microsoft YaHei UI",9))
        head = ttk.Frame(self.root,padding=(22,14,22,12)); head.pack(fill="x")
        ttk.Label(head,text="声迹",font=("Microsoft YaHei UI",25,"bold")).pack(side="left")
        ttk.Label(head,text="Ascent  /  已知视角 · 声源区域与轨迹",foreground="#6a7786").pack(side="left",padx=18)
        self.status = tk.StringVar(value="准备就绪")
        ttk.Label(head,textvariable=self.status,foreground="#266f76").pack(side="right")
        body = ttk.Frame(self.root,padding=(22,0,22,10)); body.pack(fill="both",expand=True)
        left = ttk.Frame(body); left.pack(side="left",fill="both",expand=True)
        right = ttk.Frame(body,width=350); right.pack(side="right",fill="y",padx=(18,0)); right.pack_propagate(False)
        self.canvas = tk.Canvas(left,bg="#172331",highlightthickness=0); self.canvas.pack(fill="both",expand=True)
        self.canvas.bind("<Configure>",lambda e:self.draw())
        self.canvas.bind("<Button-1>",self.map_click)
        self.show_paths=tk.BooleanVar(value=False)
        ttk.Checkbutton(left,text="显示候选路径（可能多解）",variable=self.show_paths,command=self.draw).pack(anchor="w",pady=(6,0))
        self.detail = tk.StringVar(value="选择会话进行回放，或接入准确的位置与视角后开始采集。")
        ttk.Label(left,textvariable=self.detail,wraplength=800).pack(anchor="w",pady=(9,5))
        ttk.Label(left,text="彩色点表示候选区域；位置未收敛时不显示代表点。\n虚线为当前方位或沿导航图连接的候选路径，不是实测位置或已确认轨迹。",foreground="#697686",font=("Microsoft YaHei UI",9)).pack(anchor="w")

        audio = ttk.LabelFrame(right,text="声音来源",padding=10); audio.pack(fill="x",pady=(0,10))
        self.device = ttk.Combobox(audio,state="readonly"); self.device.pack(fill="x")
        row=ttk.Frame(audio); row.pack(fill="x",pady=(6,0))
        ttk.Button(row,text="实时采集",command=self.start_live).pack(side="left")
        ttk.Button(row,text="停止",command=self.stop).pack(side="left",padx=6)
        ttk.Button(row,text="刷新",command=self.refresh_devices).pack(side="left")
        ttk.Button(audio,text="回放录音会话…",command=self.open_session).pack(fill="x",pady=(7,0))
        row=ttk.Frame(audio); row.pack(fill="x",pady=(5,0))
        ttk.Label(row,text="回放速度").pack(side="left")
        self.speed=tk.StringVar(value="4")
        ttk.Combobox(row,textvariable=self.speed,values=["1","2","4","8","尽快"],state="readonly",width=6).pack(side="left",padx=8)
        ttk.Label(audio,text="仅分析声音，不播放录音。实时模式读取整个输出设备。",wraplength=315,foreground="#697686",font=("Microsoft YaHei UI",9)).pack(anchor="w",pady=(5,0))

        pose = ttk.LabelFrame(right,text="已知位置与视角",padding=10); pose.pack(fill="x",pady=(0,10))
        self.x=tk.StringVar(value="0.50"); self.y=tk.StringVar(value="0.50"); self.yaw=tk.StringVar(value="0")
        row=ttk.Frame(pose); row.pack(fill="x")
        for text,var in (("X",self.x),("Y",self.y),("朝向°",self.yaw)):
            ttk.Label(row,text=text).pack(side="left")
            ttk.Entry(row,textvariable=var,width=6).pack(side="left",padx=(3,6))
        self.pose_file=tk.StringVar(); self.pose_note=tk.StringVar(value="手动模式：点击地图设位置，Shift + 点击设朝向")
        ttk.Label(pose,textvariable=self.pose_note,wraplength=315,foreground="#697686",font=("Microsoft YaHei UI",9)).pack(anchor="w",pady=7)
        row=ttk.Frame(pose);row.pack(fill="x")
        ttk.Button(row,text="接入姿态 JSON…",command=self.choose_pose).pack(side="left")
        ttk.Button(row,text="手动",command=lambda:self.pose_file.set("")).pack(side="left",padx=6)
        ttk.Label(pose,text="X/Y：地图 0～1；0°向上、90°向右。\n移动或转头时，需连续提供同步姿态。",foreground="#697686",font=("Microsoft YaHei UI",9)).pack(anchor="w",pady=(6,0))

        model=ttk.LabelFrame(right,text="实测音频模型",padding=10);model.pack(fill="x",pady=(0,10))
        self.model_note=tk.StringVar()
        ttk.Label(model,textvariable=self.model_note,wraplength=315,font=("Microsoft YaHei UI",9)).pack(anchor="w")
        self.use_neural=tk.BooleanVar(value=True)
        ttk.Checkbutton(model,text="使用学习模型辅助方向估计",variable=self.use_neural,command=self.model_toggled).pack(anchor="w",pady=5)
        self.use_range=tk.BooleanVar(value=False)
        ttk.Checkbutton(model,text="同一音频设置：启用脚步距离实验",variable=self.use_range,command=self.model_toggled).pack(anchor="w",pady=(0,5))
        row=ttk.Frame(model);row.pack(fill="x")
        ttk.Button(row,text="方向配置…",command=lambda:self.choose_model(False)).pack(side="left")
        ttk.Button(row,text="学习模型…",command=lambda:self.choose_model(True)).pack(side="left",padx=6)
        ttk.Button(row,text="距离模型…",command=self.choose_range_model).pack(side="left")

        ttk.Button(right,text="导出当前估计…",command=self.export_current).pack(side="bottom",fill="x",pady=(8,0))
        self.count=tk.StringVar(value="当前轨迹假设  0")
        ttk.Label(right,textvariable=self.count,font=("Microsoft YaHei UI",12,"bold")).pack(anchor="w",pady=(0,7))
        table=ttk.Frame(right);table.pack(fill="both",expand=True)
        self.tracks=ttk.Treeview(table,columns=("id","state","uv","spread"),show="headings",height=4)
        for key,label,width in (("id","编号",42),("state","状态",73),("uv","地图位置",113),("spread","候选范围",75)):
            self.tracks.heading(key,text=label);self.tracks.column(key,width=width,minwidth=width,stretch=key=="uv")
        scroll=ttk.Scrollbar(table,orient="vertical",command=self.tracks.yview);self.tracks.configure(yscrollcommand=scroll.set)
        scroll.pack(side="right",fill="y");self.tracks.pack(fill="both",expand=True)
        ttk.Label(self.root,text="实验版 · 声源数量不设固定上限；重叠声音仍可能无法分离。地图采样和轨迹范围均不是精确声学位置或统计置信区间。",foreground="#697686",font=("Microsoft YaHei UI",9),padding=(22,0,22,12)).pack(anchor="w")

    def emit(self, generation, kind, value):
        if generation!=self.generation:return
        try:self.events.put_nowait((generation,kind,value))
        except queue.Full:
            try:self.events.get_nowait()
            except queue.Empty:pass
            try:self.events.put_nowait((generation,kind,value))
            except queue.Full:pass

    def refresh_devices(self):
        try:
            self.devices=devices();self.device["values"]=[f'{d["index"]} · {d["name"]}' for d in self.devices]
            if self.devices:self.device.current(next((i for i,d in enumerate(self.devices) if d["default"]),0))
        except Exception as e:self.devices=[];self.status.set(f"音频设备不可用：{e}")

    def update_model_label(self):
        good=self.profile_path.is_file() and (not self.use_neural.get() or self.neural_path.is_file())
        if not self.range_path.is_file():self.range_path=default_range_path()
        title="Ascent 脚步实验模型" if good else "方向模型未就绪，请选择配置"
        if self.use_range.get() and not self.range_path.is_file():title+=" · 距离模型未就绪"
        self.model_note.set(title+"\n设备、音量、HRTF须与校准一致；距离仅弱提示。")

    def model_toggled(self):
        self.stop();self.update_model_label()

    def make_analyzer(self, profile_path, neural_path, use_neural, range_path=None, use_range=False,background_samples=None):
        profile=Profile.load(profile_path)
        neural=LearnedAudioModel.load(neural_path) if use_neural else None
        range_model=None
        if use_range:
            if range_path is None or not Path(range_path).is_file():
                raise ValueError("距离模型未就绪，请加载实测模型或取消脚步距离实验")
            from range_audio import RangeAudioModel
            range_model=RangeAudioModel.load(range_path)
        # This checkbox attests only to the audio chain. SceneAnalyzer still asks
        # the model to classify each event automatically and reject unknown/OOD audio.
        return SceneAnalyzer(self.geometry,profile,neural,range_model=range_model,range_context_verified=bool(use_range),
                             background_samples=background_samples if use_range else None)

    def start_live(self):
        self.stop()
        index=self.device.current()
        if index<0:self.error("请选择音频输出设备");return
        d=self.devices[index]
        if d["channels"]!=2:self.error("请选择两声道输出设备");return
        generation=self.generation;stop=threading.Event();self.worker_stop=stop
        audio_queue=queue.Queue(maxsize=50);self.audio_queue=audio_queue
        profile_path,neural_path,use=self.profile_path,self.neural_path,self.use_neural.get()
        range_path,use_range=self.range_path,self.use_range.get()
        def receive(x,t):
            if stop.is_set():return
            try:audio_queue.put_nowait((x,t))
            except queue.Full:
                stop.set();self.emit(generation,"error","分析队列积压超过 2 秒，已停止。请关闭其他负载后重试。")
        def work():
            try:
                analyzer=self.make_analyzer(profile_path,neural_path,use,range_path,use_range)
                expected=analyzer.field.profile.metadata.get("device")
                if expected and expected!=d["name"]:raise ValueError("输出设备与方向配置不一致，请选择匹配的实测配置")
                engine=LiveEventEngine(analyzer)
                while not stop.is_set():
                    try:x,delivered=audio_queue.get(timeout=.15)
                    except queue.Empty:continue
                    with self.pose_lock:poses=list(self.pose_rows)
                    rows,note=engine.consume(x,delivered,poses)
                    for row in rows:
                        row["range_background"]="calibration_fallback" if use_range else "disabled"
                        self.emit(generation,"state",row)
                    if use_range:note+=" · 距离实验未提供当前背景"
                    self.emit(generation,"status",note)
            except Exception as e:stop.set();self.emit(generation,"error",str(e))
        self.running=True;self.mode="live";self.worker=threading.Thread(target=work,daemon=True);self.worker.start()
        self.capture=Capture(d,receive,lambda e:self.emit(generation,"error",e))
        try:self.capture.start();self.status.set("正在采集 · 等待声音与姿态")
        except Exception as e:self.stop();self.error(e)

    def open_session(self):
        folder=filedialog.askdirectory(title="选择包含 audio.wav 和 listener_pose.jsonl 的会话目录")
        if not folder:return
        path=Path(folder)
        if not (path/"audio.wav").is_file() or not (path/"listener_pose.jsonl").is_file():
            self.error("会话需要 audio.wav 和 listener_pose.jsonl");return
        self.stop();generation=self.generation;stop=threading.Event();self.worker_stop=stop
        profile_path,neural_path,use=self.profile_path,self.neural_path,self.use_neural.get()
        range_path,use_range=self.range_path,self.use_range.get()
        speed=None if self.speed.get()=="尽快" else float(self.speed.get())
        def work():
            try:
                audio,poses=load_session_inputs(path)
                background=load_range_background(path,use_range)
                analyzer=self.make_analyzer(profile_path,neural_path,use,range_path,use_range,background_samples=background)
                events=detect_events(audio);previous=0.;accepted=0;skipped=0
                for i,event in enumerate(events):
                    if stop.is_set():return
                    target=event["center_sample"]/RATE
                    if speed and stop.wait(min(1.,max(0,target-previous)/speed)):return
                    start,end=event["start_sample"],event["end_sample"]
                    if not pose_window_covered(poses,start/RATE,end/RATE):
                        skipped+=1;previous=target
                        self.emit(generation,"status",f"姿态缺口 · 第 {i+1} 个事件暂不定位")
                        continue
                    row=analyzer.event(audio[start:end],start,poses);row["audio_event"]=event
                    row["range_background"]=("session_background" if background is not None else "calibration_fallback") if use_range else "disabled"
                    accepted+=1
                    self.emit(generation,"state",row)
                    self.emit(generation,"status",f"回放分析 {i+1}/{len(events)} · {target:.1f} s")
                    previous=target
                self.emit(generation,"done",f"分析完成 · {accepted} 个事件"+(f" · {skipped} 个缺同步姿态" if skipped else "")+f" · {len(audio)/RATE:.1f} s")
            except Exception as e:self.emit(generation,"error",str(e))
        self.running=True;self.mode="replay";self.status.set("正在读取录音与姿态…")
        self.worker=threading.Thread(target=work,daemon=True);self.worker.start()

    def stop(self):
        self.generation+=1;self.worker_stop.set()
        if self.capture:self.capture.stop();self.capture=None
        self.running=False;self.mode="idle";self.latest={};self.last_table=None
        self.status.set("已停止")
        self.detail.set("已停止。位置未收敛时，只能保留候选区域与方位。")
        if hasattr(self,"canvas"):self.draw()

    def choose_pose(self):
        path=filedialog.askopenfilename(filetypes=[("姿态 JSON","*.json")])
        if path:self.pose_file.set(path)

    def choose_model(self, neural):
        path=filedialog.askopenfilename(filetypes=[("实测模型","*.npz")])
        if not path:return
        try:
            (LearnedAudioModel.load if neural else Profile.load)(path)
            self.stop()
            if neural:self.neural_path=Path(path)
            else:self.profile_path=Path(path)
            self.update_model_label()
        except Exception as e:self.error(e)

    def choose_range_model(self):
        path=filedialog.askopenfilename(title="选择可选的脚步距离实验模型",filetypes=[("距离实验模型","*.npz")])
        if not path:return
        # Selecting a path does not enable the experiment or load the model.
        if self.use_range.get():self.stop()
        self.range_path=Path(path);self.update_model_label()

    def update_pose(self):
        try:
            if self.pose_file.get():
                data=json.loads(Path(self.pose_file.get()).read_text(encoding="utf-8-sig"))
                self.pose_note.set("连续姿态接入中 · "+Path(self.pose_file.get()).name)
            else:
                data={"x":float(self.x.get()),"y":float(self.y.get()),"heading_deg":float(self.yaw.get()),"timestamp":time.time()}
                self.pose_note.set("手动模式：点击地图设位置，Shift + 点击设朝向")
            p=normalized_pose(data,self.geometry)
            with self.pose_lock:
                if not self.pose_rows or p["timestamp"]>self.pose_rows[-1]["timestamp"]:self.pose_rows.append(p)
            self.manual_pose=p
        except Exception as e:self.pose_note.set(str(e))

    def rect(self):
        w,h=self.canvas.winfo_width(),self.canvas.winfo_height();s=max(20,min(w,h)-32)
        return (w-s)/2,(h-s)/2,s

    def map_click(self,event):
        if self.mode=="replay" or self.pose_file.get():return
        ox,oy,s=self.rect();u,v=(event.x-ox)/s,(event.y-oy)/s
        if not 0<=u<=1 or not 0<=v<=1:return
        if event.state&1:
            self.yaw.set(f"{math.degrees(math.atan2(u-float(self.x.get()),-(v-float(self.y.get()))))%360:.1f}")
        else:self.x.set(f"{u:.4f}");self.y.set(f"{v:.4f}")
        self.update_pose();self.draw()

    @staticmethod
    def color(identifier):
        rgb=colorsys.hsv_to_rgb((int(identifier)*.61803398875)%1,.72,.92)
        return "#"+"".join(f"{int(v*255):02x}" for v in rgb)

    def draw(self):
        if not hasattr(self,"canvas"):return
        c=self.canvas;c.delete("all");ox,oy,s=self.rect()
        size=max(1,int(s))
        if self.map_photo is None or size!=self.map_photo_size:
            self.map_photo=ImageTk.PhotoImage(self.map_image.resize((size,size),Image.Resampling.LANCZOS));self.map_photo_size=size
        c.create_image(ox,oy,image=self.map_photo,anchor="nw")
        pose=self.latest.get("listener_pose")
        if pose:
            xy=[pose["x_cm"],pose["y_cm"]];yaw=pose["yaw_deg"]
        elif hasattr(self,"manual_pose"):
            xy=[self.manual_pose["world_x"],self.manual_pose["world_y"]];yaw=self.manual_pose["yaw"]
        else:xy=self.geometry.map_to_world([.5,.5]);yaw=0
        listener_uv=self.geometry.world_to_map(xy)
        # These rays belong to current audio direction components, not to a claimed
        # target range or a source/track association that the estimator did not make.
        for observation in self.latest.get("observations",[]):
            world_yaw=yaw+observation["peak_relative_deg"]
            angle=math.radians(float(self.geometry.world_yaw_to_map_heading(world_yaw)))
            direction=np.array([math.sin(angle),-math.cos(angle)])
            length=.4
            for pos,delta in zip(listener_uv,direction):
                if delta>1e-9:length=min(length,(1-pos)/delta)
                elif delta< -1e-9:length=min(length,-pos/delta)
            if length>0:
                end=listener_uv+direction*length
                c.create_line(ox+listener_uv[0]*s,oy+listener_uv[1]*s,ox+end[0]*s,oy+end[1]*s,
                              fill="#d6e4ed",width=2,dash=(6,5))
        candidate_legend=0
        for track in self.latest.get("tracks",[]):
            color=self.color(track["track_id"])
            cloud=track.get("particles_world_xy_cm",[])
            if cloud:
                uv=self.geometry.world_to_map(cloud)
                for u,v in uv:
                    if 0<=u<=1 and 0<=v<=1:c.create_oval(ox+u*s-2,oy+v*s-2,ox+u*s+2,oy+v*s+2,fill=color,outline="")
            concentrated=bool(track.get("position_concentrated",False))
            history=track.get("history",[]) if self.show_paths.get() else []
            if len(history)>1 and self.graph_paths is None:self.graph_paths=GraphPathCache(self.geometry)
            for a,b in zip(history,history[1:]):
                path=self.graph_paths.between(a,b)
                if path is not None:
                    uv=self.geometry.world_to_map(path)
                    if np.all((uv>=0)&(uv<=1)):c.create_line(*(np.array([ox,oy])+uv*s).ravel().tolist(),fill=color,width=2,dash=(4,4))
            if concentrated:
                u,v=track["map_uv"]
                if 0<=u<=1 and 0<=v<=1:
                    c.create_text(ox+u*s+8,oy+v*s-12,text=f'#{track["track_id"]} 候选（估计）',fill=color,font=("Microsoft YaHei UI",10,"bold"),anchor="w")
            else:
                kind="候选路径" if self.show_paths.get() else "候选"
                c.create_text(ox+9,oy+15+19*candidate_legend,text=f'#{track["track_id"]} {kind} · 位置未收敛',
                              fill=color,font=("Microsoft YaHei UI",10,"bold"),anchor="w")
                candidate_legend+=1
        u,v=self.geometry.world_to_map(xy);px,py=ox+u*s,oy+v*s
        heading=math.radians(float(self.geometry.world_yaw_to_map_heading(yaw)))
        c.create_oval(px-6,py-6,px+6,py+6,fill="#ffffff",outline="#182a38",width=2)
        c.create_line(px,py,px+math.sin(heading)*30,py-math.cos(heading)*30,fill="#ffffff",width=3,arrow="last")
        rows=track_table_rows(self.latest)
        if rows!=self.last_table:
            self.tracks.delete(*self.tracks.get_children())
            for row in rows:self.tracks.insert("","end",values=row)
            self.last_table=rows
        self.count.set(f"当前轨迹假设  {len(rows)}")
        if self.latest:
            t=self.latest.get("time_s",0);n=len(self.latest.get("observations",[]))
            detail=f"音频 {t:.2f} s · 本次方向成分 {n} · 轨迹假设 {len(rows)}\n范围较大时，位置仍有多个解释；只转头不能直接测出距离。"
            hint=range_hint_text(self.latest.get("audio_range"))
            if hint:detail+="\n"+hint
            if self.latest.get("range_background")=="calibration_fallback":
                detail+="\n距离实验未提供当前背景，使用校准背景。"
            self.detail.set(detail)

    def tick(self):
        self.update_pose()
        redraw=False
        while True:
            try:generation,kind,value=self.events.get_nowait()
            except queue.Empty:break
            if generation!=self.generation:continue
            if kind=="state":self.latest=value;redraw=True
            elif kind=="status":self.status.set(value)
            elif kind=="done":self.running=False;self.mode="idle";self.status.set(value)
            elif kind=="error":self.stop();self.error(value)
        if redraw:self.draw()
        self.root.after(50,self.tick)

    def export_current(self):
        if not self.latest:self.error("尚无可导出的轨迹估计");return
        path=filedialog.asksaveasfilename(defaultextension=".json",filetypes=[("当前估计 JSON","*.json")])
        if not path:return
        data={"kind":"sound_map_hypotheses","source_truth_read":False,"map":str(self.map_path),
              "profile":str(self.profile_path),"neural":str(self.neural_path) if self.use_neural.get() else None,
              "range_model":str(self.range_path) if self.use_range.get() else None,
              "range_context_verified":bool(self.use_range.get()),"range_event_class":"auto",
              "state":self.latest,"note":"位置、轨迹和数量均为假设；候选范围不是统计置信区间"}
        try:Path(path).write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding="utf-8");self.status.set("已导出当前估计")
        except Exception as e:self.error(e)

    def error(self,error):
        self.status.set(str(error));messagebox.showerror("声迹",str(error))

    def close(self):
        self.stop();self.root.destroy()


def main():
    root=tk.Tk();SoundMapApp(root);root.mainloop()


if __name__=="__main__":main()
