"""Exercise owned native shots, hero hitboxes, death, score and fresh hero lives."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import subprocess
import sys
import time

from build_native_module import build
from original_probe_channel import OriginalProbe
from run_map_probe import prepare
from run_native_probe import ROOT, save
from training_channel import ProcessWatch, TrainingChannel


def read_json(path):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return {}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--skip-build", action="store_true")
    parser.add_argument("--seconds", type=float, default=35)
    args = parser.parse_args()
    if not 10 <= args.seconds <= 90:
        parser.error("seconds must be between 10 and 90")
    if not args.skip_build:
        build()
    folder = ROOT / ".local/original-training-verification" / datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    folder.mkdir(parents=True)
    prepared = prepare(ROOT, 1, "navigation", folder / "config.bin")
    result = folder / "run.json"
    summary = {"kind": "original_training_runtime_check", "passed": False,
               "python_weapon_simulation": False, "os_input_injection": False}
    channel = watch = None
    run_dir = None
    samples = []
    with (folder / "runner.log").open("w", encoding="utf-8") as log:
        runner = subprocess.Popen([sys.executable, str(ROOT / "tools/run_native_baseline.py"), "--run",
            "--seconds", str(args.seconds + 40), "--bootstrap", prepared["archive"], "--map", "1",
            "--mode", "navigation", "--observe-world", "--training-module", "--module-stage", "9",
            "--original-game-instance", "--original-visual-classes", "--no-training-hud",
            "--result-file", str(result)], cwd=ROOT, stdout=log, stderr=log, creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            deadline = time.monotonic() + 35
            active_path = result.with_suffix(".active.json")
            while runner.poll() is None and time.monotonic() < deadline:
                active = read_json(active_path)
                if active.get("pid"):
                    break
                time.sleep(.05)
            else:
                raise RuntimeError("Original owned lifecycle was not published")
            run_dir = Path(active["run_dir"])
            watch = ProcessWatch(active["pid"])
            channel = TrainingChannel.open(active["pid"], alive=watch.alive, timeout=35)
            probe = OriginalProbe(channel)
            started = time.monotonic()
            changed = started
            previous_time = -1
            last_shot_count = 0
            aimed_at = set()
            shot_origins_verified = True
            local = ROOT / ".local/native-module"
            while watch.alive() and time.monotonic() - started < args.seconds:
                frame = probe.read()
                if frame.time != previous_time:
                    previous_time, changed = frame.time, time.monotonic()
                elif time.monotonic() - changed > 1.5:
                    from capture_owned_thread import capture_owned_thread
                    summary["hang_capture"] = capture_owned_thread(active_path, folder / "hang-thread.json", reason="native_training_stale")
                    raise RuntimeError("Original game stopped publishing fresh training samples")
                enemies = read_json(local / "original-enemies.json").get("enemies", [])
                candidates = [e for e in enemies if e.get("ready") and e.get("position")]
                damage = read_json(local / "original-damage.json")
                dead_indices = {e.get("index") for e in damage.get("targets", []) if not e.get("alive", True)}
                candidates = [e for e in candidates if e.get("roster_index") not in dead_indices]
                flags = 8 | 16  # Focus plus explicit stage9 stationary-target test.
                target = min(candidates, key=lambda e: abs(e["position"][1]-frame.camera[1]), default=None)
                if target is not None and frame.flags & 1:
                    x, y, z = target["position"]
                    dx, dy, dz = x-frame.camera[0], y-frame.camera[1], z+68-frame.camera[2]
                    probe.aim(pitch=math.degrees(math.atan2(dz, math.hypot(dx, dy))),
                              yaw=math.degrees(math.atan2(dy, dx)))
                    aimed_at.add(target.get("roster_index"))
                    if frame.ammo <= 2:
                        flags |= 2
                    elif (time.monotonic()-started) % .8 < .15:
                        flags |= 1
                probe.command(flags=flags)
                for shot in frame.shots:
                    if shot["sequence"] > last_shot_count:
                        shot_origins_verified &= bool(shot["flags"] & 8) and math.dist(shot["start"], frame.camera) < 200
                last_shot_count = frame.shot_count
                samples.append({"time": frame.time, "ammo": frame.ammo, "native_rays": frame.native_rays,
                                "physics_calls": frame.physics_calls, "target": target.get("roster_index") if target else None})
                if frame.observer_error:
                    raise RuntimeError("Original shot observer rejected a native payload")
                time.sleep(.03)
            if not watch.alive():
                raise RuntimeError("Owned original client exited during the training drill")
            probe.release()
            training = read_json(local / "original-training.json")
            damage = read_json(local / "original-damage.json")
            score = read_json(local / "original-scoreboard.json")
            respawn = read_json(local / "original-enemy-respawn.json")
            assertions = {
                "actual_original_shots_observed": bool(samples) and samples[-1]["native_rays"] >= 5,
                "actual_original_world_origins_used": last_shot_count > 0 and shot_origins_verified,
                "native_damage_responses_observed": damage.get("native_responses", 0) > 0,
                "native_health_readbacks_observed": damage.get("native_health_readbacks", 0) > 0,
                "actual_original_death_observed": damage.get("death_events", 0) > 0,
                "native_score_getters_observe_kills": score.get("kills_recorded", 0) > 0,
                "fresh_original_hero_life_observed": respawn.get("completed", 0) > 0,
                "training_coordinator_has_no_error": bool(training) and not training.get("error", 1),
            }
            summary.update(assertions=assertions, passed=all(assertions.values()),
                           aimed_at=sorted(aimed_at), training=training, damage=damage, score=score, respawn=respawn)
        except Exception as error:
            summary["error"] = str(error)
        finally:
            if channel is not None:
                try:
                    OriginalProbe(channel).release()
                except Exception:
                    pass
                channel.close()
            if run_dir is not None:
                save(run_dir / "stop-request.json", {"reason": "original_training_verification_complete"})
            try:
                runner.wait(timeout=10)
            except subprocess.TimeoutExpired:
                runner.terminate()
                runner.wait(timeout=10)
            if watch is not None:
                watch.close()
    summary["run"] = read_json(result)
    save(folder / "samples.json", {"samples": samples})
    save(folder / "verification.json", summary)
    print(json.dumps({"passed": summary["passed"], "assertions": summary.get("assertions"),
                      "error": summary.get("error"), "artifact": str(folder / "verification.json")}, ensure_ascii=False))
    return 0 if summary["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
