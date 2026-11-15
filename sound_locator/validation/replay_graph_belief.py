"""Replay cached audio direction evidence, with no source-truth access."""
import argparse
import json
from pathlib import Path
import sys
import time
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import tracking
from graph_belief import GraphBeliefTrack
from direction_field import DirectionObservation
from map_geometry import MapGeometry
tracking.GraphParticleTrack=GraphBeliefTrack


def main():
    p=argparse.ArgumentParser();p.add_argument("prediction",type=Path);p.add_argument("--out",type=Path,required=True);a=p.parse_args()
    data=json.loads(a.prediction.read_text(encoding="utf-8"))
    geometry=MapGeometry.load();tracker=tracking.MultiSourceTracker(geometry)
    rows=[];start=time.perf_counter()
    for row in data["rows"]:
        pose=tracking.ListenerPose(**row["listener_pose"])
        observations=[DirectionObservation(np.asarray(o["angles"]),np.asarray(o["scores"]),np.ones(16)/4,
                o["strength"],o["peak_relative_deg"],o["alternatives_deg"]) for o in row["observations"]]
        state=tracker.update(observations,pose)
        for key in ("audio_event","listener_pose","audio_status","observations","neural_status"):
            if key in row:state[key]=row[key]
        rows.append(state)
    data["rows"]=rows;data["processing_seconds"]=time.perf_counter()-start
    data["filter"]="GraphBeliefTrack cached-observation replay"
    data["spectral_signature_replay_limit"]="Cached observations do not retain spectral signatures; constant normalized signature used in association"
    data["source_truth_read"]=False
    a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text(json.dumps(data,ensure_ascii=False,separators=(",",":")),encoding="utf-8")
    print(json.dumps({"output":str(a.out),"events":len(rows),"processing_seconds":data["processing_seconds"],"tracks_at_end":rows[-1]["track_count_hypotheses"],
        "last":[{k:t[k] for k in ("track_id","world_xy_cm","radius90_cm","position_concentrated","listener_baseline_cm","stationary_mode_probability")} for t in rows[-1]["tracks"]]}))


if __name__=="__main__":main()
