"""Small learned binaural angular score model. NumPy inference, optional CPU Torch training.

This model was designed for independent-recording experiments, not as evidence of
unlimited source separation. It returns a complete circular score field. The scores
are NOT calibrated correctness probabilities, and single-source training does not
establish multi-source validity. Heading is an input coordinate transform, never a label.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import time
import numpy as np
from core import RATE, BINS, spectra, stereo, read_wav, angular_distance
from event_audio import detect_events

ANGLES = np.arange(0., 360., 5.)
SCHEMA = 1


def circular_target(degrees, width_deg=10.):
    """Smooth targets continuous across the 359/0 degree boundary."""
    distance = angular_distance(ANGLES[None, :], np.asarray(degrees).reshape(-1, 1))
    scores = np.exp(-.5*(distance/width_deg)**2)
    return scores/scores.sum(axis=1, keepdims=True)


def angular_features(samples, noise_floor=None):
    x = stereo(samples)
    if len(x) < 1024:
        raise ValueError("At least 1024 stereo samples are required")
    z = spectra(x)
    left, right = z[:, 0], z[:, 1]
    l, r = np.abs(left), np.abs(right)
    energy = l*l+r*r
    excess = energy if noise_floor is None else np.maximum(energy-2*noise_floor, 0)
    weight = np.sqrt(excess)
    weight[energy < max(float(energy.max())*.001, 1e-9)] = 0
    cross = right*np.conj(left)/(l*r+1e-15)
    total = weight.sum(axis=0)+1e-12
    phase = (weight*cross).sum(axis=0)/total
    ild = (weight*np.clip(20*np.log10((r+1e-10)/(l+1e-10)), -36, 36)).sum(axis=0)/total
    # Only interaural features are network inputs: no absolute audio spectrum.
    vector = np.concatenate([ild/12, phase.real, phase.imag, np.abs(phase)]).astype(np.float32)
    quality = {
        "level_dbfs": float(20*np.log10(np.sqrt(np.mean(x*x))+1e-12)),
        "background_excess_fraction": float(excess.sum()/(energy.sum()+1e-12)),
        "coherent_energy_fraction": float((weight*(np.abs(phase)>.55)).sum()/(weight.sum()+1e-12)),
        "identical_channels": bool(np.max(np.abs(x[:, 0]-x[:, 1])) < 1e-7),
    }
    return vector, quality


def quality_status(quality, has_background):
    if quality["level_dbfs"] < -52:return "quiet"
    if quality["identical_channels"]:return "center_or_mono"
    if has_background and quality["background_excess_fraction"] < .08:return "background"
    if quality["coherent_energy_fraction"] < .2:return "incoherent"
    return "angular_scores"


class LearnedAudioModel:
    """Inference API with no Torch runtime dependency and no candidate-count cap."""
    def __init__(self, arrays, metadata=None):
        self.arrays={k:np.asarray(v,dtype=np.float32) for k,v in arrays.items()}
        self.metadata=metadata or {}
        feature_count=int(BINS.sum())*4
        shapes={"mean":(feature_count,),"scale":(feature_count,),"w1":(64,feature_count),
                "b1":(64,),"w2":(64,64),"b2":(64,),"w3":(len(ANGLES),64),"b3":(len(ANGLES),)}
        for k,shape in shapes.items():
            if k not in self.arrays or self.arrays[k].shape!=shape or not np.isfinite(self.arrays[k]).all():
                raise ValueError("Invalid learned model array: "+k)
        if np.any(self.arrays["scale"]<=0):raise ValueError("Invalid normalization scale")
        noise=self.arrays.get("noise_floor",np.array([]))
        if noise.size and (noise.shape!=(int(BINS.sum()),) or np.any(noise<0) or not np.isfinite(noise).all()):
            raise ValueError("Invalid background model")
        self.noise_floor=noise if noise.size else None

    def scores_from_features(self, features):
        x=np.asarray(features,dtype=np.float32)
        if x.shape[-1]!=self.arrays["mean"].size or not np.isfinite(x).all():
            raise ValueError("Invalid audio feature vector")
        a=self.arrays
        x=np.clip((x-a["mean"])/a["scale"],-8,8)
        x=np.maximum(x@a["w1"].T+a["b1"],0)
        x=np.maximum(x@a["w2"].T+a["b2"],0)
        logits=x@a["w3"].T+a["b3"]
        logits=logits-logits.max(axis=-1,keepdims=True)
        score=np.exp(logits)
        return score/score.sum(axis=-1,keepdims=True)

    def predict(self, samples, heading_deg=0.):
        if not np.isfinite(heading_deg):raise ValueError("Heading must be finite")
        vector,quality=angular_features(samples,self.noise_floor)
        status=quality_status(quality,self.noise_floor is not None)
        scores=self.scores_from_features(vector) if status=="angular_scores" else np.zeros(len(ANGLES))
        entropy=float(-np.sum(scores*np.log(scores+1e-12))/np.log(len(scores))) if scores.sum() else None
        if entropy is not None and entropy>.92:
            status="uninformative_model";scores=np.zeros(len(ANGLES))
        result={"relative_angles_deg":ANGLES.tolist(),"world_angles_deg":((ANGLES+heading_deg)%360).tolist(),
                "direction_scores":scores.tolist(),"status":status,"score_is_probability":False,
                "quality":quality,"source_count":None,
                "normalized_entropy":entropy,
                "limits":["single-source training does not establish mixture validity",
                          "score field is not a calibrated posterior",
                          "heading is assumed known and time-aligned by caller"]}
        if status=="angular_scores":
            result["top_relative_deg"]=float(ANGLES[int(np.argmax(scores))])
        return result

    def save(self,path):
        path=Path(path);path.parent.mkdir(parents=True,exist_ok=True)
        with path.open("wb") as f:
            np.savez_compressed(f,**self.arrays,schema=SCHEMA,
                                metadata=json.dumps(self.metadata,ensure_ascii=False))

    @classmethod
    def load(cls,path):
        with np.load(path,allow_pickle=False) as d:
            if int(d["schema"])!=SCHEMA:raise ValueError("Unsupported learned model schema")
            return cls({k:d[k] for k in d.files if k not in ("schema","metadata")},json.loads(str(d["metadata"])))


def fixed_windows(row,x):
    for receipt in row["receipts"]:
        for delay in (0.,.08,.16):
            t=receipt["host_offset_seconds"]+delay
            start=round(t*RATE)
            if start+7680<=len(x):yield t,x[start:start+7680]


def associate_events(events,receipts):
    """Evaluation/annotation only. Detection and chosen audio windows never use truth."""
    available=set(range(len(receipts)));matched={}
    for i,event in enumerate(events):
        onset=event["onset_sample"]/RATE
        candidates=[j for j in available if -.02<=onset-receipts[j]["host_offset_seconds"]<=.25]
        if candidates:
            j=min(candidates,key=lambda j:abs(onset-receipts[j]["host_offset_seconds"]-.05))
            available.remove(j);matched[i]=j
    return matched


def _fit(x,y,mean,scale,epochs,seed=913,validation=None):
    # CPU only. NumPy arrays exported for inference; no pickled model loading.
    import torch
    torch.set_num_threads(2)
    torch.manual_seed(seed)
    net=torch.nn.Sequential(torch.nn.Linear(x.shape[1],64),torch.nn.ReLU(),torch.nn.Dropout(.15),
                            torch.nn.Linear(64,64),torch.nn.ReLU(),torch.nn.Dropout(.15),
                            torch.nn.Linear(64,len(ANGLES)))
    inputs=torch.from_numpy(np.clip((x-mean)/scale,-8,8).astype(np.float32))
    # NaN targets are deliberately uninformative pre-event background controls.
    target_array=np.full((len(y),len(ANGLES)),1/len(ANGLES),dtype=np.float32)
    finite=np.isfinite(y)
    target_array[finite]=circular_target(y[finite]).astype(np.float32)
    target=torch.from_numpy(target_array)
    optimizer=torch.optim.AdamW(net.parameters(),lr=.001,weight_decay=.01)
    history=[];best=(float("inf"),epochs)
    for epoch in range(1,epochs+1):
        net.train();optimizer.zero_grad()
        logits=net(inputs+torch.randn_like(inputs)*.025)
        loss=-(target*torch.log_softmax(logits,dim=1)).sum(dim=1).mean()
        loss.backward();optimizer.step()
        if validation is not None and (epoch%10==0 or epoch==epochs):
            net.eval()
            vx,vy=validation
            with torch.no_grad():
                scores=torch.softmax(net(torch.from_numpy(np.clip((vx-mean)/scale,-8,8).astype(np.float32))),dim=1).numpy()
            err=angular_distance(ANGLES[np.argmax(scores,axis=1)],vy)
            score=float(np.mean(err))
            history.append({"epoch":epoch,"mean_angle_error_deg":score,"median_angle_error_deg":float(np.median(err))})
            if score<best[0]:best=(score,epoch)
    net.eval()
    arrays={"mean":mean,"scale":scale}
    for i,layer in enumerate((net[0],net[3],net[6]),1):
        arrays[f"w{i}"]=layer.weight.detach().numpy()
        arrays[f"b{i}"]=layer.bias.detach().numpy()
    return arrays,best[1],history


def train_recordings(directory,output,max_epochs=300,window_mode="onset"):
    directory=Path(directory);output=Path(output);output.mkdir(parents=True,exist_ok=True)
    report=json.loads((directory/"recordings.json").read_text(encoding="utf-8"))
    calibration_hashes=set();holdout_hashes=set()
    for row in report["clips"]:
        digest=hashlib.sha256((directory/row["wav"]).read_bytes()).hexdigest()
        if row["split"]=="calibration":calibration_hashes.add(digest)
        elif row["split"]=="holdout":holdout_hashes.add(digest)
    if calibration_hashes & holdout_hashes:
        raise ValueError("Identical WAV content crosses calibration and holdout splits")
    z=spectra(read_wav(directory/"background.wav"))
    noise=np.percentile((np.abs(z)**2).sum(axis=1),90,axis=0).astype(np.float32)
    rows=[];negative_rows=[];event_counts={}
    for row in report["clips"]:
        x=read_wav(directory/row["wav"])
        onset=min((r["host_offset_seconds"] for r in row["receipts"]),default=0.)
        if onset>=.24 and len(x)>=7680:
            vector,quality=angular_features(x[:7680],noise)
            negative_rows.append({"clip":row["wav"],"split":row["split"],"features":vector,
                                  "quality":quality,"label":"pre-event background, not silence"})
        detected=detect_events(x)
        matched=associate_events(detected,row["receipts"])
        event_counts[row["wav"]]={"split":row["split"],"triggers":len(row["receipts"]),
                                   "detected":len(detected),"associated":len(matched)}
        if window_mode=="onset":
            windows=[(e["center_sample"]/RATE,x[e["start_sample"]:e["end_sample"]],i in matched)
                     for i,e in enumerate(detected)]
        else:windows=[(t,w,True) for t,w in fixed_windows(row,x)]
        for t,window,associated in windows:
            features,quality=angular_features(window,noise)
            rows.append({"clip":row["wav"],"split":row["split"],"truth":row["angle_deg"]%360,
                         "time":t,"features":features,"status":quality_status(quality,True),"associated":associated})
    calibration_names=sorted({r["clip"] for r in rows if r["split"]=="calibration"})
    if len(calibration_names)<12:raise ValueError("Need at least 12 independent calibration recordings")
    angles=np.sort(np.unique([r["truth"] for r in rows if r["split"]=="calibration"]))
    if len(angles)<8 or np.max(np.diff(np.r_[angles,angles[0]+360]))>60:
        raise ValueError("Calibration recordings must cover the full angular circle")
    # Entire clips stay together. Every fourth sorted recording is validation-only.
    validation_names=set(calibration_names[::4]);training_names=set(calibration_names)-validation_names
    def select(names):
        items=[r for r in rows if r["clip"] in names and r["status"]=="angular_scores" and r["associated"]]
        return np.stack([r["features"] for r in items]),np.array([r["truth"] for r in items])
    tx,ty=select(training_names);vx,vy=select(validation_names)
    mean=tx.mean(axis=0);scale=np.maximum(tx.std(axis=0),.06)
    def add_negatives(x,y,names):
        negatives=[r["features"] for r in negative_rows if r["clip"] in names]
        if negatives:
            return np.concatenate([x,np.stack(negatives)]),np.r_[y,np.full(len(negatives),np.nan)]
        return x,y
    tx_aug,ty_aug=add_negatives(tx,ty,training_names)
    _,epochs,history=_fit(tx_aug,ty_aug,mean,scale,max_epochs,validation=(vx,vy))
    # After selecting epoch count without test recordings, refit on full calibration.
    cx,cy=select(set(calibration_names));mean=cx.mean(axis=0);scale=np.maximum(cx.std(axis=0),.06)
    cx_aug,cy_aug=add_negatives(cx,cy,set(calibration_names))
    arrays,_,_=_fit(cx_aug,cy_aug,mean,scale,epochs)
    arrays["noise_floor"]=noise
    metadata={"architecture":"4 relative features per FFT bin -> 64 ReLU -> 64 ReLU -> 72 circular scores",
              "training_scope":report["scope"],"training_recordings":calibration_names,
              "selection_train_recordings":sorted(training_names),"selection_validation_recordings":sorted(validation_names),
              "selected_epochs":epochs,"training_seed":913,"heading_role":"external exact transform",
              "window_mode":window_mode,"detector":"past 10ms RMS + rolling background, fixed160ms from onset-20ms",
              "negative_controls":"first 160ms before first event in calibration clips only, uniform angular target",
              "single_source_training":True,"known_limit":"not an unlimited multi-source separator",
              "dataset":"already-used development recordings; not new Ascent acceptance"}
    model=LearnedAudioModel(arrays,metadata);model.save(output/"learned-model.npz")
    test=[]
    for row in rows:
        if row["split"]!="holdout":continue
        scores=model.scores_from_features(row["features"]) if row["status"]=="angular_scores" else None
        if scores is not None and -np.sum(scores*np.log(scores+1e-12))/np.log(len(scores))>.92:
            scores=None
        prediction=float(ANGLES[int(np.argmax(scores))]) if scores is not None else None
        test.append({k:row[k] for k in ("clip","truth","time","status","associated")}|{"prediction":prediction,
                    "error_deg":float(angular_distance(prediction,row["truth"])) if prediction is not None else None})
    errors=[r["error_deg"] for r in test if r["error_deg"] is not None and r["associated"]]
    holdout_triggers=sum(c["triggers"] for c in event_counts.values() if c["split"]=="holdout")
    negative_test=[]
    for row in negative_rows:
        if row["split"]!="holdout":continue
        status=quality_status(row["quality"],True)
        score=model.scores_from_features(row["features"])
        entropy=float(-np.sum(score*np.log(score+1e-12))/np.log(len(score)))
        negative_test.append({"clip":row["clip"],"quality_status":status,"normalized_entropy":entropy,
                              "returned":bool(status=="angular_scores" and entropy<=.92)})
    summary={"calibration_clips":len(calibration_names),"selection_train_clips":len(training_names),
             "selection_validation_clips":len(validation_names),"test_clips":len({r["clip"] for r in test}),
             "test_windows":len(test),"returned_windows":len(errors),"selected_epochs":epochs,
             "median_error_deg":float(np.median(errors)) if errors else None,
             "p90_error_deg":float(np.percentile(errors,90)) if errors else None,
             "mean_error_deg":float(np.mean(errors)) if errors else None,
             "within30_fraction_all":sum(e<=30 for e in errors)/max(len(test),1),
             "within30_fraction_triggers":sum(e<=30 for e in errors)/max(holdout_triggers,1),
             "unassociated_bursts_returning_direction":sum(not r["associated"] and r["prediction"] is not None for r in test),
             "window_mode":window_mode,
             "holdout_triggers":holdout_triggers,
             "holdout_detected_bursts":sum(c["detected"] for c in event_counts.values() if c["split"]=="holdout"),
             "holdout_associated_bursts":sum(c["associated"] for c in event_counts.values() if c["split"]=="holdout"),
             "holdout_pre_event_background_windows":len(negative_test),
             "holdout_pre_event_background_returned":sum(r["returned"] for r in negative_test),
             "model_sha256":hashlib.sha256((output/"learned-model.npz").read_bytes()).hexdigest(),
             "note":"Complete-clip split, overlapping windows not independent; development experiment only"}
    (output/"training-evaluation.json").write_text(json.dumps({"summary":summary,"metadata":metadata,
                            "selection_history":history,"test":test,"background_test":negative_test,"event_counts":event_counts},ensure_ascii=False,indent=2),encoding="utf-8")
    return summary


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    sub=parser.add_subparsers(dest="command",required=True)
    p=sub.add_parser("train");p.add_argument("recordings");p.add_argument("output");p.add_argument("--epochs",type=int,default=300);p.add_argument("--window-mode",choices=["onset","receipt"],default="onset")
    p=sub.add_parser("predict");p.add_argument("model");p.add_argument("wav");p.add_argument("output");p.add_argument("--heading",type=float,default=0)
    args=parser.parse_args()
    if args.command=="train":print(json.dumps(train_recordings(args.recordings,args.output,args.epochs,args.window_mode),ensure_ascii=False,indent=2))
    else:
        model=LearnedAudioModel.load(args.model);x=read_wav(args.wav)
        with Path(args.output).open("w",encoding="utf-8") as f:
            for start in range(0,len(x)-7680+1,3840):
                row=model.predict(x[start:start+7680],args.heading);row["time_s"]=(start+3840)/RATE
                f.write(json.dumps(row,ensure_ascii=False)+"\n")


if __name__=="__main__":main()
