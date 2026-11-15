from __future__ import annotations
import argparse
import json
import time
from pathlib import Path
import numpy as np
from core import Locator, Pose, Profile, RATE, read_wav, angular_distance


def load_manifest(path):
    path = Path(path)
    data = json.loads(path.read_text(encoding="utf-8-sig"))
    return data, [(float(row["angle_deg"]), read_wav(path.parent / row["wav"])) for row in data["clips"]]


def main():
    parser = argparse.ArgumentParser(description="Stereo + HRTF 声音方向实验工具")
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("devices")
    p = sub.add_parser("calibrate")
    p.add_argument("manifest")
    p.add_argument("output")
    p = sub.add_parser("analyze")
    p.add_argument("wav")
    p.add_argument("--profile")
    p.add_argument("--out", required=True)
    p.add_argument("--x", type=float, default=.5)
    p.add_argument("--y", type=float, default=.5)
    p.add_argument("--heading", type=float, default=0)
    p = sub.add_parser("evaluate")
    p.add_argument("manifest", help="使用未参加校准的、方向已知的单源录音")
    p.add_argument("profile")
    p.add_argument("--out", required=True)
    args = parser.parse_args()
    if args.command == "devices":
        from capture import devices
        print(json.dumps(devices(), ensure_ascii=False, indent=2))
    elif args.command == "calibrate":
        data, clips = load_manifest(args.manifest)
        bg=read_wav(Path(args.manifest).parent/data["background_wav"]) if data.get("background_wav") else None
        p = Profile.train(clips, data.get("metadata", {}), background=bg)
        p.save(args.output)
        print(json.dumps({"angles": p.angles.tolist(), "full_circle": p.full_circle}))
    elif args.command == "analyze":
        x = read_wav(args.wav)
        locator = Locator(Profile.load(args.profile) if args.profile else None)
        pose = Pose(args.x, args.y, args.heading)
        with Path(args.out).open("w", encoding="utf-8") as f:
            for start in range(0, len(x)-7680+1, 3840):
                row = locator.analyze(x[start:start+7680], pose=pose)
                row["audio_time_s"] = (start+3840)/RATE
                f.write(json.dumps(row, ensure_ascii=False)+"\n")
    elif args.command == "evaluate":
        _, clips = load_manifest(args.manifest)
        locator, errors, rows, total = Locator(Profile.load(args.profile)), [], [], 0
        started = time.perf_counter()
        for truth, x in clips:
            for start in range(0, len(x)-7680+1, 7680):
                result = locator.analyze(x[start:start+7680])
                if result["status"] == "quiet":
                    continue
                total += 1
                candidates = result["candidates"]
                error = float(angular_distance(candidates[0]["relative_deg"], truth)) if candidates else None
                if error is not None:
                    errors.append(error)
                rows.append({"truth": truth, "error_deg": error, "result": result})
        report = {"kind": "held_out_evaluation_requires_independent_input", "active_windows": total,
                  "returned_windows": len(errors), "coverage": len(errors)/max(total, 1),
                  "median_top1_error_deg": float(np.median(errors)) if errors else None,
                  "p90_top1_error_deg": float(np.percentile(errors, 90)) if errors else None,
                  "within_30deg_fraction_of_active": sum(e <= 30 for e in errors)/max(total, 1),
                  "seconds": time.perf_counter()-started, "rows": rows}
        Path(args.out).write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
        print(json.dumps({k:v for k,v in report.items() if k != "rows"}, ensure_ascii=False))


if __name__ == "__main__":
    main()
