"""Separate evaluator: source truth is read here, never by SceneAnalyzer."""
from pathlib import Path
import argparse
import json
import math
import numpy as np
from core import angular_distance
from map_geometry import MapGeometry


def main():
    p=argparse.ArgumentParser()
    p.add_argument("prediction",type=Path)
    p.add_argument("truth",type=Path)
    p.add_argument("--out",type=Path,required=True)
    args=p.parse_args()
    geometry=MapGeometry.load()
    prediction=json.loads(args.prediction.read_text(encoding="utf-8"))
    truth=[json.loads(s) for s in args.truth.read_text(encoding="utf-8-sig").splitlines() if s.strip()]
    truth=[t for t in truth if t.get("handler_accepted",True)]
    rows=prediction["rows"]
    event_times=np.array([r["audio_event"].get("onset_sample",r["audio_event"]["start_sample"])/48000 for r in rows])
    bearing_errors=[];position_errors=[];radii=[];inside=[];region_distances=[];details=[];ids={};switches=0
    for t in truth:
        if not len(rows):break
        k=int(np.argmin(np.abs(event_times-t["t_audio_s"])))
        gap=float(abs(event_times[k]-t["t_audio_s"]))
        if gap>.25:
            details.append({"truth":t,"matched_audio":False});continue
        row=rows[k];source=np.asarray(t["position"][:2])
        listener=row["listener_pose"]
        delta=source-np.array([listener["x_cm"],listener["y_cm"]])
        bearing=(math.degrees(math.atan2(delta[1],delta[0]))-listener["yaw_deg"])%360
        observations=row["observations"]
        # Best-candidate angle is explicitly optimistic for multi-source ambiguity.
        best_bearing=min((float(angular_distance(o["angles"][np.argmax(o["scores"])],bearing)) for o in observations),default=None)
        tracks=[tr for tr in row["tracks"] if tr["status"]!="tentative"]
        closest=min(tracks,key=lambda tr:np.linalg.norm(np.asarray(tr["world_xy_cm"])-source)) if tracks else None
        detail={"truth_source_id":t["source_id"],"time_s":t["t_audio_s"],"matched_audio":True,
                "time_gap_s":gap,"best_candidate_bearing_error_deg":best_bearing,
                "truth_position_cm":source.tolist(),"track_hypotheses":len(tracks)}
        if best_bearing is not None:bearing_errors.append(best_bearing)
        if closest:
            error=float(np.linalg.norm(np.asarray(closest["world_xy_cm"])-source))
            position_errors.append(error);radii.append(closest["radius90_cm"])
            inside.append(error<=closest["radius90_cm"])
            region_nodes=[n for region in closest.get("candidate_regions",[]) for n in region.get("node_indices",[])]
            if region_nodes:
                region_distance=float(np.min(np.linalg.norm(geometry.points_world[region_nodes,:2]-source,axis=1)))
                region_distances.append(region_distance)
                detail["nearest_supported_region_distance_cm"]=region_distance
            sid=t["source_id"]
            if sid in ids and ids[sid]!=closest["track_id"]:switches+=1
            ids[sid]=closest["track_id"]
            detail.update(position_error_cm=error,nearest_track_id=closest["track_id"],
                          reported_radius90_cm=closest["radius90_cm"])
        details.append(detail)
    def summary(values):
        return {"median":float(np.median(values)),"p90":float(np.percentile(values,90)),"mean":float(np.mean(values))} if values else None
    result={"truth_events":len(truth),"detected_audio_events":len(rows),
            "truth_events_with_nearby_detection":sum(d["matched_audio"] for d in details),
            "events_with_confirmed_track_estimate":len(position_errors),
            "best_candidate_bearing_error_deg":summary(bearing_errors),
            "nearest_track_position_error_cm":summary(position_errors),
            "reported_region_radius90_cm":summary(radii),
            "inside_reported_region_fraction":float(np.mean(inside)) if inside else None,
            "candidate_region_nearest_distance_cm":summary(region_distances),
            "candidate_region_within_150cm_fraction":float(np.mean(np.asarray(region_distances)<=150)) if region_distances else None,
            "nearest_track_id_switches":switches,
            "limits":["Closest-candidate/closest-track metrics are optimistic, not identity proof",
                      "Overlapping triggers can match the same detected burst",
                      "Region coverage is empirical, not a calibrated confidence guarantee"],
            "details":details}
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding="utf-8")
    print(json.dumps({k:v for k,v in result.items() if k!="details"},ensure_ascii=False,indent=2))


if __name__=="__main__":main()
