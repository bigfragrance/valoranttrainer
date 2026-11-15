"""V2 recording pipeline. Its inputs deliberately exclude source-truth files."""
from __future__ import annotations
from dataclasses import asdict
import argparse
import json
from pathlib import Path
import time
import numpy as np
from core import RATE,HOP,NFFT,Profile,read_wav,spectra
from direction_field import DirectionField,DirectionObservation
from tracking import MultiSourceTracker,ListenerPose
from map_geometry import MapGeometry
from event_audio import detect_events


class PoseTimeline:
    def __init__(self,rows):
        rows=sorted(rows,key=lambda r:float(r["t_audio_s"]))
        if len(rows)<2:raise ValueError("需要至少两个有时间戳的已知视角样本。")
        self.times=np.array([r["t_audio_s"] for r in rows],float)
        self.position=np.array([[r["world_x"],r["world_y"],r.get("world_z",0)] for r in rows],float)
        self.yaw=np.rad2deg(np.unwrap(np.deg2rad([r["yaw"] for r in rows])))
        if not all(np.isfinite(v).all() for v in (self.times,self.position,self.yaw)):
            raise ValueError("姿态流包含无效数值。")
    @classmethod
    def load(cls,path):
        return cls([json.loads(line) for line in Path(path).read_text(encoding="utf-8-sig").splitlines() if line.strip()])
    def at(self,t):
        if t<self.times[0]-.15 or t>self.times[-1]+.15:raise ValueError("音频时刻缺少同步姿态。")
        xyz=[float(np.interp(t,self.times,self.position[:,i])) for i in range(3)]
        return ListenerPose(xyz[0],xyz[1],float(np.interp(t,self.times,self.yaw)),float(t),xyz[2])
    def frame_yaws(self,start_sample,length):
        times=(start_sample+np.arange(0,length-NFFT+1,HOP)+NFFT/2)/RATE
        return np.interp(times,self.times,self.yaw)


def spectral_signature(x):
    power=np.sum(np.abs(spectra(x))**2,axis=(0,1))
    v=np.array([power[idx].sum() for idx in np.array_split(np.arange(len(power)),16)])
    return v/(np.linalg.norm(v)+1e-12)


class SceneAnalyzer:
    def __init__(self,geometry,profile,neural=None,particles=1600,range_model=None,range_context_verified=False,background_samples=None):
        self.geometry=geometry;self.field=DirectionField(profile);self.neural=neural
        self.range_model=range_model;self.range_context_verified=range_context_verified
        self.background_samples=background_samples
        self.tracker=MultiSourceTracker(geometry,particles_per_track=particles)

    def event(self,x,start_sample,poses):
        yaws=poses.frame_yaws(start_sample,len(x))
        center=(start_sample+NFFT/2+(len(yaws)//2)*HOP)/RATE
        pose=poses.at(center)
        result=self.field.analyze(x,frame_yaws_deg=yaws)
        observations=result["observations"]
        model_result=None
        if self.neural is not None and np.ptp(yaws)<=8:
            model_result=self.neural.predict(x,heading_deg=pose.yaw_deg)
            if model_result["status"]=="angular_scores":
                a=np.asarray(model_result["relative_angles_deg"])
                scores=np.asarray(model_result["direction_scores"])
                peak=float(a[scores.argmax()])
                explained=result.get("single_direction_explained_fraction",np.zeros_like(self.field.angles))
                agreement=float(np.interp(peak,np.r_[self.field.angles,360],np.r_[explained,explained[0]]))
                if len(observations)<=1 or agreement>=.72:
                    observations=[DirectionObservation(a,scores,spectral_signature(x),1.,float(a[scores.argmax()]),[])]
                else:
                    # A single-source network must never erase other TF components.
                    for obs in observations:
                        learned=np.interp(obs.angles,np.r_[a,360],np.r_[scores,scores[0]])
                        obs.scores*=.65+.35*learned/(learned.max()+1e-12)
                        obs.scores/=obs.scores.sum()
        range_result=None
        if self.range_model is not None and len(observations)==1 and np.ptp(yaws)<=8:
            obs=observations[0]
            peak=float(obs.angles[obs.scores.argmax()])
            range_result=self.range_model.predict(x,relative_deg=peak,event_class="auto",context_verified=self.range_context_verified,
                                                   background_samples=self.background_samples)
            # A rejected or unvalidated module returns no informative range likelihood.
            if range_result.get("likelihood") and np.ptp(range_result["likelihood"])>1e-8:
                obs.range_hint=range_result
        state=self.tracker.update(observations,pose)
        state["audio_range"]=range_result
        state["listener_pose"]=asdict(pose)
        state["audio_status"]=result["status"]
        state["known_rotation_span_deg"]=float(np.ptp(yaws))
        state["observations"]=[{"peak_relative_deg":o.peak_deg,"strength":o.strength,
                                "angles":o.angles.tolist(),"scores":o.scores.tolist(),
                                "alternatives_deg":o.alternatives_deg} for o in observations]
        state["neural_status"]=model_result["status"] if model_result else "not_used_during_fast_turn_or_unavailable"
        return state

    def analyze(self,audio,poses):
        events=detect_events(audio)
        for event in events:
            start,end=event["start_sample"],event["end_sample"]
            row=self.event(audio[start:end],start,poses)
            row["audio_event"]=event
            yield row


def main():
    p=argparse.ArgumentParser(description="音频+已知相机姿态+地图的时序分析，不读取声源真值")
    p.add_argument("session",type=Path)
    p.add_argument("--profile",type=Path,required=True)
    p.add_argument("--neural",type=Path)
    p.add_argument("--map",type=Path,default=Path(__file__).parent/"maps/ascent.json")
    p.add_argument("--out",type=Path,required=True)
    p.add_argument("--particles",type=int,default=1600)
    p.add_argument("--range-model",type=Path)
    p.add_argument("--same-audio-chain",action="store_true",help="明确同一输出设备/音量/HRTF链，才启用弱距离模型")
    args=p.parse_args()
    neural=None
    if args.neural:
        from learned_audio import LearnedAudioModel
        neural=LearnedAudioModel.load(args.neural)
    audio=read_wav(args.session/"audio.wav")
    poses=PoseTimeline.load(args.session/"listener_pose.jsonl")
    range_model=None
    if args.range_model:
        from range_audio import RangeAudioModel
        range_model=RangeAudioModel.load(args.range_model)
    background_path=args.session/"background.wav"
    background=read_wav(background_path) if background_path.is_file() else None
    analyzer=SceneAnalyzer(MapGeometry.load(args.map),Profile.load(args.profile),neural,args.particles,range_model,args.same_audio_chain,background)
    started=time.perf_counter();rows=list(analyzer.analyze(audio,poses))
    result={"estimator_inputs":["audio.wav","listener_pose.jsonl",str(background_path) if background is not None else None,
                               str(args.map),str(args.profile),str(args.neural),str(args.range_model)],
            "source_truth_read":False,"audio_duration_s":len(audio)/RATE,"processing_seconds":time.perf_counter()-started,
            "event_count":len(rows),"fixed_source_count_limit":None,"rows":rows,
            "limits":["Source count and identity are hypotheses","Region radius is not statistically calibrated",
                      "Rotation resolves bearing ambiguity but does not directly measure range",
                      "Position estimates depend on sampled map coverage and motion priors"]}
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding="utf-8")
    print(json.dumps({k:v for k,v in result.items() if k!="rows"},ensure_ascii=False))


if __name__=="__main__":main()
