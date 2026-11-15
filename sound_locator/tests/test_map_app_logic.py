"""Headless UI boundary checks. These tests create no Tk windows and play no audio."""
from pathlib import Path
import json
import sys
from types import SimpleNamespace
import numpy as np
import pytest
from scipy.io import wavfile
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from core import RATE
from map_geometry import MapGeometry
from map_app import normalized_pose,track_table_rows,load_session_inputs,LiveEventEngine,default_paths,SoundMapApp,pose_window_covered,range_hint_text,default_range_path,load_range_background


def test_pose_mapping_and_stale_or_missing_heading_rejected():
    geometry=MapGeometry.load()
    pose=normalized_pose({"x":.4,"y":.6,"heading_deg":350,"timestamp":100},geometry,now=100.1)
    assert np.allclose(geometry.world_to_map([pose["world_x"],pose["world_y"]]),[.4,.6])
    assert pose["yaw"]==350
    with pytest.raises(ValueError):normalized_pose({**pose,"timestamp":90},geometry,now=100)
    with pytest.raises(ValueError):normalized_pose({"world_x":1,"world_y":2,"timestamp":100},geometry,now=100)


def test_display_retains_all_track_hypotheses():
    tracks=[dict(track_id=i,map_uv=[.4,.6],radius90_cm=900,status="track_hypothesis") for i in range(17)]
    rows=track_table_rows({"tracks":tracks})
    assert len(rows)==17 and rows[-1][0]=="16"
    assert rows[0][-1]=="9.0 m"
    assert all(row[2]=="位置未收敛" for row in rows)
    tracks[0]["position_concentrated"]=True
    assert "估计" in track_table_rows({"tracks":tracks})[0][2]


def test_replay_reads_audio_and_pose_without_truth(tmp_path):
    wavfile.write(tmp_path/"audio.wav",RATE,np.zeros((RATE,2),np.float32))
    (tmp_path/"listener_pose.jsonl").write_text("\n".join(json.dumps(dict(t_audio_s=t,world_x=1,world_y=2,world_z=3,yaw=359 if t==0 else 1)) for t in [0,1]),encoding="utf-8")
    (tmp_path/"source_truth.jsonl").write_text("THIS IS NOT VALID JSON AND MUST NEVER BE READ",encoding="utf-8")
    audio,poses=load_session_inputs(tmp_path)
    assert audio.shape==(RATE,2)
    assert poses.at(.5).yaw_deg==pytest.approx(360)


def test_live_events_are_not_duplicated_by_sliding_buffer():
    class Tracker:
        last_time=None
        def update(self,obs,pose):
            assert self.last_time is None or pose.time_s>=self.last_time
            self.last_time=pose.time_s
            return {"time_s":pose.time_s,"tracks":[]}
    class Analyzer:
        def __init__(self):self.tracker=Tracker();self.starts=[]
        def event(self,x,start,poses):
            self.starts.append(start)
            return self.tracker.update([],poses.at((start+len(x)//2)/RATE))
    rng=np.random.default_rng(3)
    x=rng.normal(0,.0005,(3*RATE,2))
    for t in [.4,1.2,2.0]:
        i=int(t*RATE);x[i:i+1440]+=rng.normal(0,.1,(1440,2))
    analyzer=Analyzer();engine=LiveEventEngine(analyzer)
    poses=[]
    for begin in range(0,len(x),1920):
        delivered=1000+(begin+1920)/RATE
        poses.append(dict(timestamp=delivered,world_x=0,world_y=0,world_z=0,yaw=0))
        engine.consume(x[begin:begin+1920],delivered,poses)
    assert len(analyzer.starts)==3
    assert len(set(analyzer.starts))==3


def test_default_assets_exist():
    assert all(path.is_file() for path in default_paths())


def test_pose_gaps_are_not_interpolated_as_known_motion():
    poses=SimpleNamespace(times=np.array([0,.05,.10,.15,1.,1.05,1.10,1.15]))
    assert pose_window_covered(poses,0,.15)
    assert not pose_window_covered(poses,.1,1.1)
    assert not pose_window_covered(poses,.6,.6)
    assert not pose_window_covered(poses,2,2)


def test_duplicate_audio_frame_pose_conflict_only_masks_its_local_interval(tmp_path):
    wavfile.write(tmp_path/"audio.wav",RATE,np.zeros((RATE,2),np.float32))
    rows=[dict(t_audio_s=t,world_x=1,world_y=2,world_z=3,yaw=10) for t in np.arange(0,1.01,.05)]
    duplicate={**rows[10],"world_x":20}
    rows.insert(11,duplicate)
    (tmp_path/"listener_pose.jsonl").write_text("\n".join(map(json.dumps,rows)),encoding="utf-8")
    _,poses=load_session_inputs(tmp_path)
    assert np.all(np.diff(poses.times)>0)
    assert not pose_window_covered(poses,.45,.6)
    assert pose_window_covered(poses,.1,.25)


def test_stopped_worker_cannot_publish_errors_into_new_generation():
    import queue
    app=SoundMapApp.__new__(SoundMapApp)
    app.generation=4;app.events=queue.Queue()
    SoundMapApp.emit(app,3,"error","old worker error")
    assert app.events.empty()
    # A message could already have been queued before stop; the UI drain also checks.
    app.events.put((3,"error","queued old worker error"))
    failures=[]
    app.update_pose=lambda:None
    app.root=SimpleNamespace(after=lambda *args:None)
    app.stop=lambda:failures.append("unexpected stop")
    app.error=lambda e:failures.append(e)
    SoundMapApp.tick(app)
    assert not failures


def test_optional_range_only_loads_when_enabled(tmp_path,monkeypatch):
    import map_app
    import range_audio
    calls=[]
    marker=object()
    monkeypatch.setattr(map_app.Profile,"load",lambda path:object())
    monkeypatch.setattr(range_audio.RangeAudioModel,"load",lambda path:(calls.append(path),marker)[1])
    monkeypatch.setattr(map_app,"SceneAnalyzer",lambda *args,**kwargs:kwargs)
    app=SoundMapApp.__new__(SoundMapApp);app.geometry=object()
    missing=tmp_path/"future-model.npz"
    result=app.make_analyzer("profile.npz","neural.npz",False,missing,False)
    assert not calls and result["range_model"] is None and not result["range_context_verified"]
    with pytest.raises(ValueError):app.make_analyzer("profile.npz","neural.npz",False,missing,True)
    assert not calls
    missing.write_bytes(b"test placeholder; loader is isolated by this test")
    result=app.make_analyzer("profile.npz","neural.npz",False,missing,True)
    assert calls==[missing] and result["range_model"] is marker and result["range_context_verified"]
    assert "event_class" not in result  # The UI never declares that a sound is a footstep.


def test_range_display_requires_accepted_informative_evidence():
    assert range_hint_text(None)==""
    assert range_hint_text({"informative":False,"median_m":10,"interval_m":[2,30]})==""
    text=range_hint_text({"informative":True,"median_m":10,"interval_m":[2,30]})
    assert "弱提示" in text and "10.0 m" in text and "2.0–30.0 m" in text and "非精确距离" in text
    assert range_hint_text({"informative":True,"median_m":float("nan"),"interval_m":[2,30]})==""
    assert default_range_path().name=="range-model.npz" or default_range_path().name=="ascent-range.npz"


def test_range_background_is_optional_and_separate_from_events(tmp_path):
    (tmp_path/"background.wav").write_text("invalid but must not be read while range is off")
    assert load_range_background(tmp_path,False) is None
    waveform=np.full((RATE,2),.005,np.float32)
    wavfile.write(tmp_path/"background.wav",RATE,waveform)
    assert np.allclose(load_range_background(tmp_path,True),waveform)
    assert load_range_background(tmp_path/"missing",True) is None
