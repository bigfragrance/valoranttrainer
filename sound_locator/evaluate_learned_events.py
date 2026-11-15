"""Independent event-level comparison. Audio detection never consumes source truth."""
import argparse
import json
from pathlib import Path
import numpy as np
from core import Profile, RATE, read_wav, angular_distance
from direction_field import DirectionField
from learned_audio import LearnedAudioModel, associate_events
from event_audio import detect_events


def evaluate(directory,model_path,profile_path,split="holdout"):
    directory=Path(directory)
    report=json.loads((directory/"recordings.json").read_text(encoding="utf-8"))
    neural=LearnedAudioModel.load(model_path)
    field=DirectionField(Profile.load(profile_path))
    rows=[];trigger_count=0;associated_count=0
    for clip in report["clips"]:
        if split!="all" and clip["split"]!=split:continue
        x=read_wav(directory/clip["wav"])
        events=detect_events(x)
        association=associate_events(events,clip["receipts"])
        trigger_count+=len(clip["receipts"]);associated_count+=len(association)
        for index,event in enumerate(events):
            window=x[event["start_sample"]:event["end_sample"]]
            learned=neural.predict(window)
            classic=field.analyze(window)
            nangle=learned.get("top_relative_deg")
            fangle=float(classic["angles"][np.argmax(classic["scores"])]) if classic["observations"] else None
            truth=clip["angle_deg"]
            row={"clip":clip["wav"],"event":event,"associated":index in association,"truth_deg":truth,
                 "neural":{"prediction":nangle,"status":learned["status"],"entropy":learned.get("normalized_entropy"),
                           "error_deg":float(angular_distance(nangle,truth)) if nangle is not None else None},
                 "direction_field":{"prediction":fangle,"status":classic["status"],"components":len(classic["observations"]),
                           "error_deg":float(angular_distance(fangle,truth)) if fangle is not None else None}}
            rows.append(row)
    summary={"scope":report["scope"],"split":split,"trigger_count":trigger_count,"detected_bursts":len(rows),
             "associated_bursts":associated_count,"missed_triggers":trigger_count-associated_count,
             "association_window_seconds":[-.02,.25],"detector_uses_truth":False,
             "note":"Unassociated acoustic bursts are not proven additional people or false physical sounds"}
    for method in ("neural","direction_field"):
        errors=[r[method]["error_deg"] for r in rows if r["associated"] and r[method]["error_deg"] is not None]
        summary[method]={"associated_returned":len(errors),"median_error_deg":float(np.median(errors)) if errors else None,
                         "p90_error_deg":float(np.percentile(errors,90)) if errors else None,
                         "mean_error_deg":float(np.mean(errors)) if errors else None,
                         "within30_of_all_triggers":sum(e<=30 for e in errors)/max(trigger_count,1),
                         "unassociated_bursts_returning_direction":sum(not r["associated"] and r[method]["prediction"] is not None for r in rows)}
    return {"summary":summary,"rows":rows}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("recordings");p.add_argument("model");p.add_argument("profile");p.add_argument("output")
    p.add_argument("--split",default="holdout")
    a=p.parse_args();result=evaluate(a.recordings,a.model,a.profile,a.split)
    Path(a.output).write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding="utf-8")
    print(json.dumps(result["summary"],ensure_ascii=False,indent=2))


if __name__=="__main__":main()
