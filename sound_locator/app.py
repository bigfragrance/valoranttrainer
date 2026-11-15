"""Standalone Windows desktop UI for calibrated binaural direction hypotheses."""
from __future__ import annotations
from collections import deque
from datetime import datetime
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
from scipy.io import wavfile
from PIL import Image, ImageTk
from core import Locator, Pose, Profile, RATE, read_pose, read_wav, angular_distance
from capture import Capture, devices

BASE = Path(sys.executable).parent if getattr(sys, "frozen", False) else Path(__file__).resolve().parent
DATA = BASE / "data"
STATUS = {"quiet":"等待声音", "uncalibrated":"未校准 · 仅左右线索", "incomplete_profile":"校准方向未覆盖完整一圈",
          "candidates":"方向候选 · 可能为歧义或混合声源", "unmatched":"与校准不匹配 · 暂不定位",
          "center_or_mono":"双声道相同 · 中央声音或单声道，无法定位", "too_short":"音频太短",
          "incoherent":"双耳特征不稳定 · 暂不定位", "background":"未明显超过校准背景 · 暂不定位"}


class App:
    def __init__(self, root):
        self.root = root
        root.title("声向 · VALORANT HRTF 方向分析")
        root.geometry("1160x800")
        root.minsize(1050, 770)
        root.configure(bg="#f2f4f7")
        self.events = queue.Queue(maxsize=12)
        self.capture = None
        self.replay_stop = threading.Event()
        self.profile = None
        self.locator = Locator()
        self.latest = None
        self.audio_buffer = np.empty((0, 2))
        self.recording = None
        self.calibrations = []
        self.session = DATA / "calibrations" / datetime.now().strftime("%Y%m%d-%H%M%S")
        self.lock = threading.Lock()
        self.pose_history = deque(maxlen=100)
        self.pose = Pose()
        self.pose_problem = ""
        self.background = None
        self.bg_tk = None
        self.current_device = ""
        self.generation = 0
        self.frame_counter = 0
        self.calibration_token = 0
        self.log = None
        self.build()
        self.refresh_devices()
        root.protocol("WM_DELETE_WINDOW", self.close)
        root.after(80, self.tick)

    def build(self):
        style = ttk.Style()
        style.theme_use("clam")
        style.configure("TFrame", background="#f2f4f7")
        style.configure("TLabel", background="#f2f4f7", foreground="#1d2838", font=("Microsoft YaHei UI", 10))
        style.configure("TButton", font=("Microsoft YaHei UI", 10), padding=6)
        style.configure("TLabelframe", background="#f2f4f7")
        style.configure("TLabelframe.Label", background="#f2f4f7", font=("Microsoft YaHei UI", 10, "bold"))
        top = ttk.Frame(self.root, padding=(22, 16, 22, 10)); top.pack(fill="x")
        ttk.Label(top, text="声向", font=("Microsoft YaHei UI", 24, "bold")).pack(side="left")
        ttk.Label(top, text="  Stereo + HRTF  /  实验版 · 需要实录校准", foreground="#65758b").pack(side="left", padx=15)
        self.state = tk.StringVar(value="准备就绪 · 尚未采集")
        ttk.Label(top, textvariable=self.state, foreground="#316d80").pack(side="right")
        body = ttk.Frame(self.root, padding=(22, 0, 22, 10)); body.pack(fill="both", expand=True)
        left = ttk.Frame(body); left.pack(side="left", fill="both", expand=True)
        right = ttk.Frame(body, width=350); right.pack(side="right", fill="y", padx=(18,0)); right.pack_propagate(False)
        self.canvas = tk.Canvas(left, bg="#101e30", highlightthickness=0)
        self.canvas.pack(fill="both", expand=True)
        self.canvas.bind("<Configure>", lambda e:self.draw())
        self.canvas.bind("<Button-1>", self.map_click)
        self.info = tk.StringVar(value="开始采集后显示左右差异。完成校准后显示角度。")
        ttk.Label(left, textvariable=self.info, wraplength=700).pack(anchor="w", pady=(10,4))
        ttk.Label(left, text="点击地图设置位置 · Shift + 点击设置朝向\n方向线长度固定，不能表示距离；候选条数不能表示人数。", foreground="#65758b").pack(anchor="w")

        audio = ttk.LabelFrame(right, text="01  音频", padding=10); audio.pack(fill="x", pady=(0,10))
        self.device_var = tk.StringVar()
        self.device_combo = ttk.Combobox(audio, textvariable=self.device_var, state="readonly"); self.device_combo.pack(fill="x")
        row = ttk.Frame(audio); row.pack(fill="x", pady=(6,0))
        ttk.Button(row, text="开始采集", command=self.start).pack(side="left")
        ttk.Button(row, text="停止", command=self.stop).pack(side="left", padx=5)
        ttk.Button(row, text="刷新", command=self.refresh_devices).pack(side="left")
        ttk.Button(audio, text="分析 WAV 录音…", command=self.replay).pack(fill="x", pady=(6,0))
        ttk.Label(audio, text="采集所选输出的全部声音，包括语音和其他应用。", wraplength=310, foreground="#65758b", font=("Microsoft YaHei UI",9)).pack(anchor="w", pady=(6,0))

        pose = ttk.LabelFrame(right, text="02  你在地图上的位置 / 朝向", padding=10); pose.pack(fill="x", pady=(0,10))
        self.x = tk.StringVar(value="0.5"); self.y = tk.StringVar(value="0.5"); self.heading = tk.StringVar(value="0")
        row = ttk.Frame(pose); row.pack(fill="x")
        for label,var in (("X",self.x),("Y",self.y),("朝向°",self.heading)):
            ttk.Label(row,text=label).pack(side="left")
            ttk.Entry(row,textvariable=var,width=6).pack(side="left",padx=(3,8))
        ttk.Label(pose, text="地图上方 = 0°，右方 = 90°；X/Y 为 0～1", font=("Microsoft YaHei UI",9), foreground="#65758b").pack(anchor="w",pady=5)
        self.pose_path = tk.StringVar()
        row = ttk.Frame(pose); row.pack(fill="x")
        ttk.Button(row,text="接入位置 JSON…",command=self.choose_pose).pack(side="left")
        ttk.Button(row,text="手动输入",command=lambda:self.pose_path.set("")).pack(side="left",padx=5)
        self.pose_label=tk.StringVar(value="手动位置 · 移动后需更新")
        ttk.Label(pose,textvariable=self.pose_label,wraplength=310,foreground="#65758b",font=("Microsoft YaHei UI",9)).pack(anchor="w",pady=4)
        ttk.Button(pose,text="加载固定朝向的小地图图片…",command=self.load_map).pack(fill="x")

        cal = ttk.LabelFrame(right, text="03  游戏实录校准", padding=10); cal.pack(fill="x", pady=(0,10))
        ttk.Label(cal,text="单个已知方向声源；0°前 / 90°右 / 180°后",wraplength=310,font=("Microsoft YaHei UI",9)).pack(anchor="w")
        row=ttk.Frame(cal);row.pack(fill="x",pady=6)
        self.angle=tk.StringVar(value="0")
        ttk.Combobox(row,textvariable=self.angle,values=[str(x) for x in range(0,360,30)],width=5,state="readonly").pack(side="left")
        self.record_button=ttk.Button(row,text="3 秒后录 3 秒",command=self.record);self.record_button.pack(side="left",padx=6)
        ttk.Button(row,text="导入 WAV",command=self.import_calibration).pack(side="left")
        self.cal_label=tk.StringVar(value="未加载校准 · 建议采集 12 个方向")
        ttk.Label(cal,textvariable=self.cal_label,wraplength=310,foreground="#65758b",font=("Microsoft YaHei UI",9)).pack(anchor="w")
        row=ttk.Frame(cal);row.pack(fill="x",pady=(6,0))
        ttk.Button(row,text="加载配置…",command=self.load_profile).pack(side="left")
        ttk.Button(row,text="新建校准",command=self.reset_calibration).pack(side="left",padx=6)

        ttk.Button(right,text="导出 / 停止导出结果 JSONL…",command=self.toggle_log).pack(side="bottom",fill="x",pady=(8,0))
        self.listing=tk.Text(right,height=2,bg="#e5ebf1",fg="#263d54",font=("Microsoft YaHei UI",10),relief="flat",padx=10,pady=8,state="disabled")
        self.listing.pack(fill="both",expand=True)
        ttk.Label(self.root,text="声音特征只能支持方向假设。自身音效、HUD、单声道、混响和重叠声音可能无法定位。",foreground="#65758b",padding=(22,0,22,12)).pack(anchor="w")

    def post(self, kind, data):
        try:self.events.put_nowait((kind,data))
        except queue.Full:
            if kind != "result":
                try:self.events.get_nowait()
                except queue.Empty:pass
                self.events.put_nowait((kind,data))

    def refresh_devices(self):
        try:
            self.devices=devices()
            self.device_combo["values"]=[f'{d["index"]} · {d["name"]}' for d in self.devices]
            if self.devices:self.device_combo.current(next((i for i,d in enumerate(self.devices) if d["default"]),0))
        except Exception as e:
            self.devices=[];self.state.set(f"音频设备不可用：{e}")

    def start(self):
        try:
            self.stop()
            index=self.device_combo.current()
            if index < 0:raise ValueError("请先选择音频输出设备。")
            d=self.devices[index]
            self.current_device=d["name"]
            expected=(self.profile.metadata.get("device") if self.profile else None)
            if expected and expected != self.current_device:
                raise ValueError("输出设备与当前校准不一致。请新建校准或加载匹配配置。")
            generation=self.generation
            self.capture=Capture(d,lambda x,t:self.receive_audio(x,t,generation),lambda e:self.post("error",e))
            self.capture.start();self.state.set("实时采集 · 系统播放音频")
        except Exception as e:self.error(e)

    def stop(self):
        self.generation+=1
        self.calibration_token+=1
        if self.capture:self.capture.stop();self.capture=None
        self.replay_stop.set()
        with self.lock:
            self.audio_buffer=np.empty((0,2));self.recording=None
        self.record_button.configure(state="normal")
        self.latest=None;self.state.set("已停止")
        self.draw()

    def receive_audio(self, x, delivered, generation):
        if generation != self.generation:return
        with self.lock:
            self.audio_buffer=np.concatenate((self.audio_buffer,x))[-7680:]
            if self.recording is not None:
                self.recording["chunks"].append(x.copy())
                self.recording["count"]+=len(x)
                if self.recording["count"]>=3*RATE:
                    r=self.recording;self.recording=None
                    self.post("calibration",(generation,r["token"],r["angle"],np.concatenate(r["chunks"])[:3*RATE]))
            buf=self.audio_buffer.copy()
        self.frame_counter+=1
        if len(buf)<7680 or self.frame_counter%2:return
        center_time=delivered-.08
        history=list(self.pose_history)
        pose=None
        moving=False
        if history and not self.pose_problem:
            stamp,pose=min(history,key=lambda row:abs(row[0]-center_time))
            if abs(stamp-center_time)>.3:pose=None
            recent=[p for t,p in history if delivered-.16<=t<=delivered]
            moving=bool(recent and max(float(angular_distance(p.heading_deg,recent[0].heading_deg)) for p in recent)>15)
            if moving:pose=None
        result=self.locator.analyze(buf,pose=pose)
        result["audio_timestamp_approx"]=center_time
        result["pose_warning"]="朝向变化过快，暂缓地图投影" if moving else self.pose_problem
        self.post("result",(generation,result))

    def replay(self):
        path=filedialog.askopenfilename(filetypes=[("Stereo WAV","*.wav")])
        if not path:return
        try:x=read_wav(path)
        except Exception as e:self.error(e);return
        self.stop();self.replay_stop=threading.Event();event=self.replay_stop;generation=self.generation
        self.state.set("WAV 回放分析 · 使用当前位置和朝向")
        def run():
            for i in range(0,len(x),1920):
                if event.is_set():return
                self.receive_audio(x[i:i+1920],time.time(),generation)
                event.wait(.04)
            self.post("replay_done",generation)
        threading.Thread(target=run,daemon=True).start()

    def update_pose(self):
        try:
            if self.pose_path.get():
                pose=read_pose(self.pose_path.get())
                self.pose_label.set("JSON 接入中 · "+Path(self.pose_path.get()).name)
                stamp=pose.timestamp
            else:
                pose=Pose(float(self.x.get()),float(self.y.get()),float(self.heading.get()))
                stamp=time.time()
                self.pose_label.set("手动位置 · 移动后需更新")
            self.pose=pose;self.pose_problem=""
            self.pose_history.append((stamp,pose))
        except Exception as e:
            self.pose_problem=str(e);self.pose_label.set(str(e))

    def choose_pose(self):
        path=filedialog.askopenfilename(filetypes=[("Pose JSON","*.json")])
        if path:self.pose_path.set(path)

    def load_map(self):
        path=filedialog.askopenfilename(filetypes=[("Map image","*.png *.jpg *.jpeg *.webp")])
        if path:
            try:
                with Image.open(path) as im:self.background=im.convert("RGB").copy()
                self.draw()
            except Exception as e:self.error(e)

    def map_rect(self):
        w,h=self.canvas.winfo_width(),self.canvas.winfo_height()
        s=max(20,min(w,h)-90)
        return (w-s)/2,(h-s)/2,s

    def map_click(self,event):
        if self.pose_path.get():return
        ox,oy,s=self.map_rect()
        x,y=(event.x-ox)/s,(event.y-oy)/s
        if not (0<=x<=1 and 0<=y<=1):return
        if event.state&1:
            self.heading.set(f"{math.degrees(math.atan2(x-self.pose.x,-(y-self.pose.y)))%360:.1f}")
        else:self.x.set(f"{x:.3f}");self.y.set(f"{y:.3f}")
        self.update_pose();self.draw()

    def record(self):
        if not self.capture:self.start()
        if not self.capture:return
        self.record_button.configure(state="disabled")
        generation=self.generation;angle=float(self.angle.get())
        self.calibration_token+=1;token=self.calibration_token
        def countdown(n):
            if generation!=self.generation or token!=self.calibration_token:return
            if n:
                self.cal_label.set(f"{n} 秒后开始 · 固定相对方向 {angle:g}°")
                self.root.after(1000,lambda:countdown(n-1))
            else:
                with self.lock:self.recording={"angle":angle,"chunks":[],"count":0,"token":token}
                self.cal_label.set(f"正在录制 {angle:g}° · 持续 3 秒")
                self.root.after(8000,lambda:self.record_timeout(generation,token))
        countdown(3)

    def record_timeout(self,generation,token):
        if generation!=self.generation or token!=self.calibration_token:return
        with self.lock:
            timed_out=self.recording is not None
            if timed_out:self.recording=None
        if timed_out:
            self.record_button.configure(state="normal")
            self.cal_label.set("未获得足够音频，请检查输出设备后重试。")

    def import_calibration(self):
        path=filedialog.askopenfilename(filetypes=[("Stereo WAV","*.wav")])
        if path:
            try:self.add_calibration(float(self.angle.get()),read_wav(path))
            except Exception as e:self.error(e)

    def add_calibration(self,angle,x):
        self.record_button.configure(state="normal")
        proposed=self.calibrations+[(angle,x)]
        metadata={"mode":"Stereo + in-game HRTF", "device":self.current_device,
                  "created":datetime.now().isoformat(),"validation":"not_game_validated"}
        profile=Profile.train(proposed,metadata)
        self.session.mkdir(parents=True,exist_ok=True)
        name=f"{len(proposed):03d}_{angle:g}deg.wav"
        wavfile.write(self.session/name,RATE,x.astype(np.float32))
        self.calibrations=proposed
        profile.save(self.session/"profile.npz")
        self.profile=profile;self.locator=Locator(profile)
        manifest={"metadata":metadata,"clips":[{"angle_deg":a,"wav":f"{i+1:03d}_{a:g}deg.wav"} for i,(a,_) in enumerate(proposed)]}
        (self.session/"manifest.json").write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding="utf-8")
        self.cal_label.set(f"已采集 {len(profile.angles)} 个方向 · "+("完整一圈；仍需独立实录验证" if profile.full_circle else "尚未覆盖完整一圈"))
        remaining=[a for a in range(0,360,30) if a not in profile.angles]
        if remaining:self.angle.set(str(remaining[0]))

    def reset_calibration(self):
        self.calibration_token+=1
        with self.lock:self.recording=None
        self.record_button.configure(state="normal")
        self.calibrations=[];self.profile=None;self.locator=Locator()
        self.session=DATA/"calibrations"/datetime.now().strftime("%Y%m%d-%H%M%S-%f")
        self.angle.set("0");self.cal_label.set("新建校准 · 已保存的历史录音保留")

    def load_profile(self):
        path=filedialog.askopenfilename(filetypes=[("Calibration","*.npz")])
        if path:
            try:
                profile=Profile.load(path)
                if self.capture and profile.metadata.get("device") and profile.metadata["device"]!=self.current_device:
                    raise ValueError("此配置对应的输出设备与当前设备不一致。")
                self.profile=profile;self.locator=Locator(profile)
                self.cal_label.set(f"已加载 {len(profile.angles)} 个方向 · "+("完整一圈" if profile.full_circle else "覆盖不足"))
            except Exception as e:self.error(e)

    def toggle_log(self):
        if self.log:self.log.close();self.log=None;self.state.set("已保存结果日志");return
        path=filedialog.asksaveasfilename(defaultextension=".jsonl",filetypes=[("Results","*.jsonl")])
        if path:
            try:self.log=Path(path).open("w",encoding="utf-8");self.state.set("正在导出分析结果")
            except Exception as e:self.error(e)

    def tick(self):
        self.update_pose()
        while True:
            try:kind,data=self.events.get_nowait()
            except queue.Empty:break
            try:
                if kind=="result":
                    generation,result=data
                    if generation!=self.generation:continue
                    self.latest=result
                    if self.log:self.log.write(json.dumps(result,ensure_ascii=False)+"\n");self.log.flush()
                elif kind=="calibration":
                    generation,token,angle,x=data
                    if generation==self.generation and token==self.calibration_token:self.add_calibration(angle,x)
                elif kind=="error":self.stop();self.error(data)
                elif kind=="replay_done" and data==self.generation:self.state.set("WAV 分析结束")
            except Exception as e:self.error(e)
        self.draw()
        self.root.after(80,self.tick)

    def draw(self):
        c=self.canvas;c.delete("all")
        ox,oy,s=self.map_rect()
        if self.background is not None:
            size=int(s)
            # Square padding preserves bearing angles and normalized coordinates.
            im=self.background.copy();im.thumbnail((size,size))
            bg=Image.new("RGB",(size,size),"#182a3d");bg.paste(im,((size-im.width)//2,(size-im.height)//2))
            self.bg_tk=ImageTk.PhotoImage(bg);c.create_image(ox,oy,image=self.bg_tk,anchor="nw")
        else:
            for i in range(11):
                u=s*i/10
                c.create_line(ox+u,oy,ox+u,oy+s,fill="#203349")
                c.create_line(ox,oy+u,ox+s,oy+u,fill="#203349")
        c.create_rectangle(ox,oy,ox+s,oy+s,outline="#496078")
        for text,x,y in (("地图上方  0°",ox+s/2,oy-22),("90°",ox+s+23,oy+s/2),("180°",ox+s/2,oy+s+22),("270°",ox-25,oy+s/2)):
            c.create_text(x,y,text=text,fill="#9eb1c7",font=("Microsoft YaHei UI",10))
        pose=self.pose
        px,py=ox+pose.x*s,oy+pose.y*s
        end=pose.ray(0,.09)["display_end"]
        c.create_line(px,py,ox+end[0]*s,oy+end[1]*s,fill="#c5d8ef",width=2,arrow="last")
        c.create_oval(px-6,py-6,px+6,py+6,fill="#e4efff",outline="")
        colors=["#63dfc0","#f2bf6e","#a9a3f2"]
        text="等待音频分析。"
        if self.latest:
            r=self.latest
            self.info.set(f'{STATUS.get(r["status"],r["status"])}\n音量 {r["level_dbfs"]:.1f} dBFS · 右减左 {r["ild_db"]:+.1f} dB · {r["side_hint"]}')
            lines=[]
            for i,can in enumerate(r["candidates"]):
                ray=can["ray"]
                absolute=f'地图 {ray["bearing_deg"]:.0f}°' if ray and not self.pose_problem else "位置不可用"
                lines.append(f'{i+1}. 相对 {can["relative_deg"]:.0f}° / {absolute}\n    特征支持分数 {can["support"]:.2f}')
                if can.get("ambiguous_with_deg"):
                    lines.append("    难以区分："+", ".join(f"{a:.0f}°" for a in can["ambiguous_with_deg"]))
                if ray and not self.pose_problem:
                    sx,sy=ray["origin"];ex,ey=ray["display_end"]
                    ex,ey=np.clip([ex,ey],0,1)
                    # Clip ray parametrically to the square, keeping its bearing.
                    dx,dy=ray["unit_vector"];length=.35
                    for pos,direction in ((sx,dx),(sy,dy)):
                        if direction>1e-9:length=min(length,(1-pos)/direction)
                        elif direction< -1e-9:length=min(length,-pos/direction)
                    ex,ey=sx+dx*length,sy+dy*length
                    c.create_line(ox+sx*s,oy+sy*s,ox+ex*s,oy+ey*s,fill=colors[i%3],width=3,arrow="last")
                    c.create_text(ox+ex*s,oy+ey*s-12,text=f'{can["relative_deg"]:.0f}°',fill=colors[i%3],font=("Microsoft YaHei UI",11,"bold"))
                    for alias in can.get("ambiguous_with_deg",[]):
                        origin=r["pose"]
                        alt=Pose(origin["x"],origin["y"],origin["heading_deg"]).ray(alias,.25)
                        ax,ay=alt["display_end"]
                        if 0<=ax<=1 and 0<=ay<=1:
                            c.create_line(ox+sx*s,oy+sy*s,ox+ax*s,oy+ay*s,fill=colors[i%3],width=2,dash=(5,5))
            text="\n".join(lines) or "暂不输出精确角度。\n"+STATUS.get(r["status"],r["status"])
            if r.get("pose_warning"):text+="\n"+r["pose_warning"]
        self.listing.configure(state="normal");self.listing.delete("1.0","end");self.listing.insert("end",text);self.listing.configure(state="disabled")

    def error(self,e):
        self.state.set(str(e));messagebox.showerror("声向",str(e))

    def close(self):
        self.stop()
        if self.log:self.log.close()
        self.root.destroy()


def main():
    root=tk.Tk();App(root);root.mainloop()


if __name__=="__main__":main()
