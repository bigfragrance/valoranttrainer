"""Evaluate owned-client recordings; calibration and holdout takes stay separate."""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import time
import numpy as np
from core import Profile, Locator, read_wav, RATE, angular_distance


def event_windows(row, x):
    # Fixed windows relative to command acknowledgement; not selected by prediction/error.
    # Host offset approximates playback onset; actual hardware latency is unknown.
    for receipt in row["receipts"]:
        offset=receipt["host_offset_seconds"]
        for delay in (0.,.08,.16):
            start=round((offset+delay)*RATE)
            if start+7680<=len(x):
                yield offset+delay, x[start:start+7680]


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("recordings",type=Path)
    parser.add_argument("--out",type=Path,required=True)
    parser.add_argument("--profile",type=Path,help="Frozen profile; only evaluate, never retrain")
    args=parser.parse_args()
    report=json.loads((args.recordings/"recordings.json").read_text(encoding="utf-8"))
    clips=[]
    for row in report["clips"]:
        if row["split"]=="calibration":
            clips.append((row["angle_deg"],read_wav(args.recordings/row["wav"])))
    metadata={"scope":report["scope"], "mode":"Stereo + in-game HRTF setting true",
              "event":report["clips"][0]["receipts"][0]["event_path"],
              "device":report["capture_device"]["name"], "validation":"owned_client_holdout_only",
              "distance_cm":report["clips"][0]["receipts"][0]["distance_cm"]}
    profile=Profile.load(args.profile) if args.profile else Profile.train(clips,metadata,background=read_wav(args.recordings/"background.wav"))
    args.out.mkdir(parents=True,exist_ok=True)
    profile.save(args.out/"profile.npz")
    locator=Locator(profile)
    rows=[];errors=[];all_errors=[];window_count=0;ambiguous=0
    started=time.perf_counter()
    for row in report["clips"]:
        if not args.profile and row["split"]!="holdout":continue
        x=read_wav(args.recordings/row["wav"])
        predictions=[]
        for t,window in event_windows(row,x):
            result=locator.analyze(window)
            window_count+=1
            cand=result["candidates"]
            error=float(angular_distance(cand[0]["relative_deg"],row["angle_deg"])) if cand else None
            if error is not None:
                errors.append(error)
                all_errors.append(min(float(angular_distance(c["relative_deg"],row["angle_deg"])) for c in cand))
                ambiguous+=bool(cand[0].get("ambiguous_with_deg"))
            predictions.append({"time":t,"top1_error_deg":error,"result":result})
        rows.append({"wav":row["wav"],"truth_deg":row["angle_deg"],"windows":predictions})
    b=read_wav(args.recordings/"background.wav")
    bg=[locator.analyze(b[i:i+7680]) for i in range(0,len(b)-7680+1,7680)]
    additional_background=[]
    for item in report.get("background_checks",[]):
        bx=read_wav(args.recordings/item["wav"])
        results=[locator.analyze(bx[i:i+7680]) for i in range(0,len(bx)-7680+1,7680)]
        additional_background.append({"wav":item["wav"],"window_count":len(results),
                                      "candidate_windows":sum(bool(r["candidates"]) for r in results),"results":results})
    summary={"scope":report["scope"],"calibration_clips":len(clips),"holdout_clips":len(rows),
             "evaluation_windows":"3 fixed 160ms windows at receipt +0/+80/+160 ms per event",
             "window_count":window_count,"returned_windows":len(errors),
             "coverage":len(errors)/max(window_count,1),
             "median_top1_error_deg":float(np.median(errors)) if errors else None,
             "p90_top1_error_deg":float(np.percentile(errors,90)) if errors else None,
             "top1_within_30deg_fraction_of_all":sum(e<=30 for e in errors)/max(window_count,1),
             "any_candidate_within_30deg_fraction_of_all":sum(e<=30 for e in all_errors)/max(window_count,1),
             "ambiguous_top1_windows":ambiguous,
             "background_windows":len(bg),"background_candidate_windows":sum(bool(r["candidates"]) for r in bg),
             "later_background_windows":sum(r["window_count"] for r in additional_background),
             "later_background_candidate_windows":sum(r["candidate_windows"] for r in additional_background),
             "mean_analysis_ms":(time.perf_counter()-started)*1000/max(window_count,1),
             "profile_metadata":profile.metadata,
             "limits":["One fixed offline map/listener, not live official multiplayer validation",
                       "HRTF setting read back, actual DSP kernel not independently verified",
                       "System endpoint contains background; background directions cannot be attributed to the triggered event",
                       "Discrete direction templates, no elevation, distance or full source separation",
                       "Nearby analysis windows overlap and are statistically dependent"]}
    (args.out/"evaluation.json").write_text(json.dumps({"summary":summary,"rows":rows,"background":bg,"later_background":additional_background},ensure_ascii=False,indent=2),encoding="utf-8")
    print(json.dumps(summary,ensure_ascii=False,indent=2))


if __name__=="__main__":main()
