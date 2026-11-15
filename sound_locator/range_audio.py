"""Optional empirical footstep range evidence, conditional on a fixed audio chain.

No inverse-square distance assumption. A calibrated regression predicts log(m),
with conservative residual/local-spread uncertainty and a large uniform likelihood
floor. Unknown class/domain/context returns uninformative evidence. NumPy inference.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
from core import RATE, FREQ, BINS, stereo, spectra, read_wav
from event_audio import detect_events
from learned_audio import associate_events

LOG_GRID=np.linspace(np.log(.5),np.log(50.),128)
BAND_EDGES=np.geomspace(250.,12001.,13)
FREQUENCIES=FREQ[BINS]


def estimate_background(samples):
    z=spectra(stereo(samples))
    return np.percentile((np.abs(z)**2).sum(axis=1),90,axis=0)


def event_features(samples,relative_deg,noise_floor=None):
    x=stereo(samples)
    if len(x)<1024 or not np.isfinite(relative_deg):raise ValueError("Invalid event or bearing")
    z=spectra(x)
    power=(np.abs(z)**2).sum(axis=1)
    excess=power if noise_floor is None else np.maximum(power-noise_floor,0)
    spectrum=excess.mean(axis=0)
    bands=np.array([spectrum[(FREQUENCIES>=a)&(FREQUENCIES<b)].sum() for a,b in zip(BAND_EDGES[:-1],BAND_EDGES[1:])])
    bandshape=np.log((bands+1e-12)/(bands.sum()+1e-12)+1e-7)
    bandshape-=bandshape.mean()
    mono_power=np.mean(x*x,axis=1)
    if noise_floor is not None:
        # Parseval conversion for the selected one-sided Hann-window frequency bins.
        normalization=1024*np.sum(np.hanning(1024)**2)
        mono_power=np.maximum(mono_power-float(np.sum(noise_floor))/normalization,0)
    chunks=np.array_split(mono_power,8)
    env=np.array([c.mean() for c in chunks]);env/=env.sum()+1e-12
    if noise_floor is None:
        rms=float(np.sqrt(mono_power.mean()))
        peak_rms=float(np.sqrt(max(c.mean() for c in np.array_split(mono_power,max(1,len(x)//480)))))
    else:
        band_power=excess.sum(axis=1)/(1024*np.sum(np.hanning(1024)**2))
        rms=float(np.sqrt(band_power.mean()));peak_rms=float(np.sqrt(band_power.max()))
    level_db=20*np.log10(rms+1e-12)
    peak_db=20*np.log10(peak_rms+1e-12)
    # Class inputs exclude absolute level and direction, reducing distance/heading shortcuts.
    class_vector=np.r_[bandshape/5.,env*4.,(peak_db-level_db)/12.].astype(np.float32)
    angle=np.deg2rad(relative_deg)
    range_vector=np.r_[level_db/12.,peak_db/12.,class_vector,
                       np.sin(angle),np.cos(angle),np.sin(2*angle),np.cos(2*angle)].astype(np.float32)
    quality={"level_dbfs":float(level_db),"peak_dbfs":float(peak_db),
             "spectral_excess_fraction":float(excess.sum()/(power.sum()+1e-12))}
    return range_vector,class_vector,quality


class RangeAudioModel:
    def __init__(self,arrays,metadata=None):
        self.a={k:np.asarray(v,dtype=float) for k,v in arrays.items()}
        self.metadata=metadata or {}
        required=("range_mean","range_scale","range_coef","range_intercept","reference_x","reference_log_m",
                  "class_mean","class_scale","class_coef","class_intercept","noise_floor","residual_sigma",
                  "domain_limit","class_threshold","auto_enabled","log_min","log_max")
        if any(k not in self.a or not np.isfinite(self.a[k]).all() for k in required):raise ValueError("Incomplete or invalid range model")
        shapes={"range_mean":(27,),"range_scale":(27,),"range_coef":(27,),"class_mean":(21,),
                "class_scale":(21,),"class_coef":(21,),"noise_floor":(int(BINS.sum()),)}
        if any(self.a[k].shape!=shape for k,shape in shapes.items()):raise ValueError("Invalid range feature dimensions")
        if np.any(self.a["range_scale"]<=0) or np.any(self.a["class_scale"]<=0):raise ValueError("Invalid feature normalization")
        if self.a["reference_x"].ndim!=2 or self.a["reference_x"].shape[1]!=len(self.a["range_mean"]):raise ValueError("Invalid range references")
        if len(self.a["reference_log_m"])!=len(self.a["reference_x"]):raise ValueError("Invalid distance references")

    def uninformative(self,status,**extra):
        return {"status":status,"informative":False,"log_distance_grid":LOG_GRID.tolist(),
                "likelihood":np.ones_like(LOG_GRID).tolist(),"median_m":None,"interval_m":None,
                "score_is_probability":False,**extra}

    def predict(self,samples,relative_deg,*,event_class="auto",context_verified=False,background_samples=None):
        if not context_verified:return self.uninformative("unverified_audio_context")
        if event_class not in ("auto","footstep"):return self.uninformative("unsupported_event_class")
        if event_class=="auto" and not bool(self.a["auto_enabled"]):return self.uninformative("automatic_classification_not_validated")
        noise=estimate_background(background_samples) if background_samples is not None else self.a["noise_floor"]
        x,c,q=event_features(samples,relative_deg,noise)
        q["local_background_supplied"]=background_samples is not None
        if q["level_dbfs"]< -52 or q["spectral_excess_fraction"]<.08:return self.uninformative("insufficient_event_energy",quality=q)
        cz=np.clip((c-self.a["class_mean"])/self.a["class_scale"],-8,8)
        logit=float(cz@self.a["class_coef"]+self.a["class_intercept"])
        class_score=float(1/(1+np.exp(-np.clip(logit,-50,50))))
        if event_class=="auto" and class_score<float(self.a["class_threshold"]):
            return self.uninformative("unknown_audio_class",class_score=class_score,quality=q)
        rz=(x-self.a["range_mean"])/self.a["range_scale"]
        distances=np.sqrt(np.mean((self.a["reference_x"]-rz)**2,axis=1))
        nearest=float(distances.min())
        if nearest>float(self.a["domain_limit"]):
            return self.uninformative("outside_calibration_domain",domain_distance=nearest,class_score=class_score,quality=q)
        mu=float(rz@self.a["range_coef"]+self.a["range_intercept"])
        if not float(self.a["log_min"])-.3<=mu<=float(self.a["log_max"])+.3:
            return self.uninformative("outside_calibrated_range",domain_distance=nearest,class_score=class_score)
        weight=np.exp(-.5*(distances/max(float(self.a["domain_limit"])*.4,.25))**2)
        weight/=weight.sum()+1e-12
        local_mean=float(weight@self.a["reference_log_m"])
        local_spread=float(np.sqrt(weight@((self.a["reference_log_m"]-local_mean)**2)))
        # Conservative log-distance width. This is not a statistically calibrated confidence interval.
        sigma=max(.45,float(self.a["residual_sigma"]),local_spread*.65)
        likelihood=.70+.30*np.exp(-.5*((LOG_GRID-mu)/sigma)**2)
        return {"status":"weak_empirical_range","informative":True,"log_distance_grid":LOG_GRID.tolist(),
                "likelihood":likelihood.tolist(),"median_m":float(np.exp(mu)),
                "interval_m":[float(np.exp(mu-1.645*sigma)),float(np.exp(mu+1.645*sigma))],
                "log_sigma":sigma,"interval_is_calibrated":False,"score_is_probability":False,
                "domain_distance":nearest,"class_score":class_score,"quality":q,
                "limits":["valid only for calibrated footstep family and unchanged output/gain/settings",
                          "absolute volume changes can invalidate distance without being detectable",
                          "class score is not a calibrated probability; unknown types may resemble footsteps"]}

    def save(self,path):
        with Path(path).open("wb") as f:np.savez_compressed(f,**self.a,schema=1,metadata=json.dumps(self.metadata,ensure_ascii=False))

    @classmethod
    def load(cls,path):
        with np.load(path,allow_pickle=False) as d:
            if int(d["schema"])!=1:raise ValueError("Unsupported range model schema")
            return cls({k:d[k] for k in d.files if k not in ("schema","metadata")},json.loads(str(d["metadata"])))


def recording_examples(directory,noise_floor,split=None,label=1):
    directory=Path(directory);report=json.loads((directory/"recordings.json").read_text(encoding="utf-8"))
    examples=[];counts={"triggers":0,"detections":0,"associated":0}
    for clip in report["clips"]:
        if split is not None and clip["split"]!=split:continue
        audio=read_wav(directory/clip["wav"]);events=detect_events(audio)
        background=audio[:round(.3*RATE)]
        local_noise=estimate_background(background) if len(background)>=1024 else noise_floor
        matched=associate_events(events,clip["receipts"])
        counts["triggers"]+=len(clip["receipts"]);counts["detections"]+=len(events);counts["associated"]+=len(matched)
        for i,event in enumerate(events):
            if i not in matched:continue
            receipt=clip["receipts"][matched[i]]
            angle=float(receipt.get("angle_deg",clip["angle_deg"]))
            window=audio[event["start_sample"]:event["end_sample"]]
            x,c,q=event_features(window,angle,local_noise)
            distance=float(receipt["distance_cm"])/100.
            examples.append({"clip":clip["wav"],"split":clip["split"],"features":x,"class_features":c,
                             "distance_m":distance,"log_m":np.log(distance),"angle_deg":angle,"quality":q,
                             "label":label,"window":window,"background":background,"event":event})
    return examples,counts


def fit_range(recordings,negative_train,negative_validation,output):
    from sklearn.linear_model import Ridge,LogisticRegression
    output=Path(output);output.mkdir(parents=True,exist_ok=True)
    root=Path(recordings)
    z=spectra(read_wav(root/"background.wav"));noise=np.percentile((np.abs(z)**2).sum(axis=1),90,axis=0)
    train,counts=recording_examples(root,noise,"calibration")
    if len(train)<24:raise ValueError("Too few detected, annotated calibration footstep events")
    names=sorted({r["clip"] for r in train})
    if len(names)<8:raise ValueError("Need at least eight independent calibration clips")
    xx=np.stack([r["features"] for r in train]);yy=np.array([r["log_m"] for r in train])
    mean=xx.mean(axis=0);scale=np.maximum(xx.std(axis=0),.1);xx=(xx-mean)/scale
    # Leave entire clips out for regression selection/residual estimation; no holdout use.
    groups=np.array([names.index(r["clip"])%4 for r in train])
    candidates=[]
    for alpha in (1.,10.,100.):
        prediction=np.empty(len(yy))
        for g in range(4):
            m=groups!=g
            raw=np.stack([r["features"] for r in train]);fm=raw[m].mean(axis=0);fs=np.maximum(raw[m].std(axis=0),.1)
            reg=Ridge(alpha=alpha).fit((raw[m]-fm)/fs,yy[m]);prediction[~m]=reg.predict((raw[~m]-fm)/fs)
        candidates.append((float(np.mean(np.abs(prediction-yy))),alpha,prediction))
    error,alpha,oof=min(candidates,key=lambda item:item[0])
    residual=float(np.quantile(np.abs(oof-yy),.90)/1.645)
    reg=Ridge(alpha=alpha).fit(xx,yy)
    # Domain calibration uses nearest different-clip references, never a point's own duplicate.
    domain=[]
    clips=np.array([r["clip"] for r in train])
    for i in range(len(train)):
        d=np.sqrt(np.mean((xx[clips!=clips[i]]-xx[i])**2,axis=1));domain.append(float(d.min()))
    domain_limit=max(1.5,float(np.quantile(domain,.99))*1.5)
    neg,_=recording_examples(negative_train,noise,None,0)
    # Add source-free pre-event/background examples from the positive scene.
    background_examples=[]
    rjson=json.loads((root/"recordings.json").read_text(encoding="utf-8"))
    for clip in rjson["clips"]:
        if clip["split"]!="calibration":continue
        wave=read_wav(root/clip["wav"])
        if min(r["host_offset_seconds"] for r in clip["receipts"])>=.24:
            _,cf,_=event_features(wave[:7680],0,estimate_background(wave[:round(.3*RATE)]));background_examples.append(cf)
    cx=np.stack([r["class_features"] for r in train]+[r["class_features"] for r in neg]+background_examples)
    cy=np.r_[np.ones(len(train)),np.zeros(len(neg)+len(background_examples))]
    cm=cx.mean(axis=0);cs=np.maximum(cx.std(axis=0),.08)
    classifier=LogisticRegression(C=.2,class_weight="balanced",max_iter=1000).fit((cx-cm)/cs,cy)
    nval,_=recording_examples(negative_validation,noise,None,0)
    nscore=classifier.predict_proba((np.stack([r["class_features"] for r in nval])-cm)/cs)[:,1]
    # The negative validation set determines a conservative rejection threshold.
    threshold=max(.98,float(nscore.max())+.01)
    auto_enabled=threshold<.999
    threshold=min(threshold,.999)
    arrays={"range_mean":mean,"range_scale":scale,"range_coef":reg.coef_,"range_intercept":reg.intercept_,
            "reference_x":xx,"reference_log_m":yy,"noise_floor":noise,"residual_sigma":residual,
            "domain_limit":domain_limit,"class_mean":cm,"class_scale":cs,"class_coef":classifier.coef_[0],
            "class_intercept":classifier.intercept_[0],"class_threshold":threshold,"auto_enabled":int(auto_enabled),
            "log_min":float(yy.min()),"log_max":float(yy.max())}
    metadata={"kind":"optional empirical fixed-audio-chain footstep range", "recordings":str(root),
              "calibration_clips":names,"regression":"Ridge selected by whole-clip out-of-fold log-range MAE",
              "chosen_alpha":alpha,"oof_mean_abs_log_error":error,"noise_model":"per-clip first300ms 90th percentile spectrum; initial background fallback",
              "negative_train":str(negative_train),"negative_validation":str(negative_validation),
              "automatic_classification_scope":"footstep vs limited reload/background negatives only; no general audio taxonomy",
              "context_requires_same_output_volume_and_game_audio_settings":True,
              "no_trajectory_truth_used":True,"interval_is_calibrated":False}
    model=RangeAudioModel(arrays,metadata);model.save(output/"range-model.npz")
    summary={"calibration_events":len(train),"calibration_clips":len(names),"regression_alpha":alpha,
             "out_of_fold_abs_log_error":error,"residual_sigma":residual,"domain_limit":domain_limit,
             "negative_train_events":len(neg),"negative_validation_events":len(nval),
             "negative_validation_max_class_score":float(nscore.max()),"class_threshold":threshold,
             "auto_enabled":auto_enabled,"model_sha256":hashlib.sha256((output/"range-model.npz").read_bytes()).hexdigest()}
    (output/"training.json").write_text(json.dumps({"summary":summary,"metadata":metadata},ensure_ascii=False,indent=2),encoding="utf-8")
    return summary


def evaluate_range(recordings,model_path,output,split="holdout",direction_model_path=None):
    from learned_audio import LearnedAudioModel
    model=RangeAudioModel.load(model_path)
    direction_model=LearnedAudioModel.load(direction_model_path) if direction_model_path else None
    examples,counts=recording_examples(recordings,model.a["noise_floor"],split)
    rows=[]
    for row in examples:
        direction=direction_model.predict(row["window"]) if direction_model else None
        angle=direction.get("top_relative_deg") if direction is not None else row["angle_deg"]
        if angle is None:
            pred=explicit=model.uninformative("direction_unavailable")
        else:
            pred=model.predict(row["window"],angle,event_class="auto",context_verified=True,background_samples=row["background"])
            explicit=model.predict(row["window"],angle,event_class="footstep",context_verified=True,background_samples=row["background"])
        rows.append({k:row[k] for k in ("clip","distance_m","angle_deg")}|{"inference_bearing_deg":angle,"auto":pred,"explicit_footstep":explicit})
    summary={"counts":counts,"detected_annotated_events":len(rows),
             "inference_bearing":"independent frozen audio direction model" if direction_model else "oracle bearing; diagnostic upper bound only"}
    for key in ("auto","explicit_footstep"):
        valid=[r for r in rows if r[key]["informative"]]
        errors=[abs(r[key]["median_m"]-r["distance_m"]) for r in valid]
        rel=[abs(np.log(r[key]["median_m"]/r["distance_m"])) for r in valid]
        summary[key]={"returned":len(valid),"median_abs_error_m":float(np.median(errors)) if errors else None,
                      "p90_abs_error_m":float(np.percentile(errors,90)) if errors else None,
                      "median_multiplicative_error":float(np.exp(np.median(rel))) if rel else None,
                      "heuristic_interval_coverage":sum(r[key]["interval_m"][0]<=r["distance_m"]<=r[key]["interval_m"][1] for r in valid)/max(len(valid),1)}
    Path(output).write_text(json.dumps({"summary":summary,"rows":rows},ensure_ascii=False,indent=2),encoding="utf-8")
    return summary


def main():
    p=argparse.ArgumentParser(description=__doc__);sub=p.add_subparsers(dest="command",required=True)
    t=sub.add_parser("train");t.add_argument("recordings");t.add_argument("negative_train");t.add_argument("negative_validation");t.add_argument("output")
    e=sub.add_parser("evaluate");e.add_argument("recordings");e.add_argument("model");e.add_argument("output");e.add_argument("--split",default="holdout");e.add_argument("--direction-model")
    a=p.parse_args()
    result=fit_range(a.recordings,a.negative_train,a.negative_validation,a.output) if a.command=="train" else evaluate_range(a.recordings,a.model,a.output,a.split,a.direction_model)
    print(json.dumps(result,ensure_ascii=False,indent=2))


if __name__=="__main__":main()
