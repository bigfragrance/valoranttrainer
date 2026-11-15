"""Bounded in-client integration check using an explicit module-only test input path.

This is automated local verification, not evidence of manual player input.
Normal play (module stage 3) does not accept this test input path.
"""
from __future__ import annotations

from datetime import datetime, timezone
import json
import math
from pathlib import Path
import struct
import subprocess
import sys
import time

from build_native_module import build
from run_map_probe import prepare
from run_native_probe import ROOT, save
from training_channel import TrainingChannel, ProcessWatch, ChannelNotReady


def read_json(path):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return {}


def target_head_for_status(frame, status):
    bot = status.get("bot")
    if not bot or not frame.target_bot_id or frame.target_bot_id != bot.get("bot_id"):
        return None
    return frame.target_head


def main():
    build()
    folder = ROOT / ".local/training-verification" / datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    archive = folder / "config.bin"
    prepare(ROOT, 1, "navigation", archive)
    result_file = folder / "run.json"
    log = (folder / "runner.log").open("w", encoding="utf-8")
    runner = subprocess.Popen([sys.executable, str(ROOT / "tools/run_native_baseline.py"), "--run",
        "--seconds", "120", "--bootstrap", str(archive), "--map", "1", "--mode", "navigation",
        "--observe-world", "--training-module", "--module-stage", "4", "--no-training-hud",
        "--result-file", str(result_file)], cwd=ROOT, stdout=log, stderr=log,
        creationflags=subprocess.CREATE_NO_WINDOW)
    process = channel = None
    run_dir = None
    summary = {"kind": "automated_native_local_training_check", "passed": False,
               "manual_input_verification": False, "production_input_override_enabled": False}
    try:
        started = time.monotonic()
        while runner.poll() is None and time.monotonic()-started < 25:
            active = read_json(result_file.with_suffix(".active.json"))
            if active.get("pid"):
                break
            time.sleep(.05)
        else:
            raise RuntimeError("Owned game did not publish its verification lifecycle")
        run_dir = Path(active["run_dir"])
        process = ProcessWatch(active["pid"])
        channel = TrainingChannel.open(active["pid"], alive=process.alive)
        buffer = channel.buffer
        aim_sequence = 0
        seen_reload = False
        reload_requested = False
        reload_until = 0.
        next_print = 0.
        while runner.poll() is None and process.alive() and time.monotonic()-started < 100:
            status = read_json(run_dir / "training/status.json")
            if status.get("phase") == "error":
                raise RuntimeError("Training host rejected verification: " + status.get("error", "unknown"))
            try:
                frame = channel.read_frame()
            except ChannelNotReady:
                time.sleep(.01)
                continue
            bot = status.get("bot")
            target_head = target_head_for_status(frame, status)
            flags = 0x80000000 | 8
            now = time.monotonic()
            if status.get("reloading"):
                seen_reload = True
            if status.get("kills", 0) >= 2 and not reload_requested:
                reload_requested = True
                reload_until = now + .15
            if now < reload_until:
                flags |= 2
            elif bot and target_head is not None and not status.get("reloading"):
                delta = tuple(target_head[i]-frame.position[i] for i in range(3))
                pitch = math.degrees(math.atan2(delta[2], math.hypot(delta[0], delta[1]))) - status.get("recoil_pitch_deg", 0.)
                yaw = math.degrees(math.atan2(delta[1], delta[0])) - status.get("recoil_yaw_deg", 0.)
                aim_sequence += 1
                struct.pack_into("<3d", buffer, 456, pitch, yaw, 0.)
                struct.pack_into("<I", buffer, 448, aim_sequence)
                flags |= 1
            struct.pack_into("<I", buffer, 16, flags)
            if now >= next_print:
                print(json.dumps({"phase": status.get("phase"), "shots": status.get("shots"),
                    "hits": status.get("hits"), "kills": status.get("kills"), "ammo": status.get("ammo"),
                    "reload_seen": seen_reload}), flush=True)
                next_print = now + 2
            if status.get("kills", 0) >= 4 and seen_reload and not status.get("reloading"):
                events = read_json(run_dir / "training/events.json").get("events", [])
                required = {"shot", "bot_hit", "bot_killed", "bot_spawned", "reload_started", "reload_completed"}
                observed = {item["kind"] for item in events}
                if required <= observed:
                    summary.update({"passed": True, "host_status": status, "event_kinds": sorted(observed),
                        "native_module_state": read_json(ROOT / ".local/native-module/bridge-state.json"),
                        "run_dir": str(run_dir), "validated_lanes": read_json(run_dir / "training/lanes.json").get("validated_lanes")})
                    break
            time.sleep(.012)
        if not summary["passed"]:
            summary["failure"] = "Required native firing, hit, kill, respawn and reload events were not all observed"
            summary["host_status"] = read_json(run_dir / "training/status.json")
    except Exception as error:
        summary["error"] = str(error)
    finally:
        if channel:
            struct.pack_into("<I", channel.buffer, 16, 0)
            channel.close()
        if process:
            process.close()
        if run_dir:
            save(run_dir / "stop-request.json", {"reason": "integration_check_complete"})
        try:
            runner.wait(timeout=15)
        except subprocess.TimeoutExpired:
            # Preserve the bounded runner's owned-game cleanup; no arbitrary PID kill.
            runner.wait(timeout=130)
        log.close()
        final = read_json(result_file)
        summary["game_image_unchanged"] = final.get("image_unchanged")
        summary["owned_game_exited"] = final.get("owned_process_exited")
        summary["remaining_game_or_launcher_processes"] = final.get("remaining_game_or_launcher_processes")
        if not final.get("image_unchanged") or not final.get("owned_process_exited"):
            summary["passed"] = False
        save(folder / "verification.json", summary)
    print(json.dumps(summary, ensure_ascii=False), flush=True)
    print(folder / "verification.json")
    return 0 if summary["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
