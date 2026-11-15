"""Bounded Duel 1/Jett acceptance through the owned stage-4 test channel.

Without --run this prints the checks and does not build or start a game.
--run uses the same owned runner as verify_native_training.py. It does not use
desktop input automation. Stage 3 ignores these module-only test input fields.
All acceptance checks inspect published state or measured native displacement;
successful command publication alone is never counted as a passing skill.
"""
from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass
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
from training_channel import (ChannelNotReady, NativeFrame, ProcessWatch,
                              TrainingChannel, SKILL_BLOCKED)
from verify_native_training import read_json, target_head_for_status

CHECKS = ("native_ready", "knife_equip", "knife_miss_spends_one", "knife_kill_refills",
          "knife_burst_exhausts", "rifle_ammo_restored", "smoke_appears", "smoke_expires",
          "updraft_displacement", "dash_displacement", "final_knife_equip")


def read_render(channel) -> dict:
    """Read the host's actual published command frame under its sequence lock."""
    for _ in range(8):
        before = struct.unpack_from("<I", channel.buffer, 512)[0]
        if not before or before & 1:
            continue
        raw = bytes(channel.buffer[516:600])
        if before != struct.unpack_from("<I", channel.buffer, 512)[0]:
            continue
        v = struct.unpack("<I7d5If", raw)
        if not all(math.isfinite(x) for x in (*v[1:8], v[13])):
            raise RuntimeError("Nonfinite host render state")
        return {"sequence": before, "flags": v[0], "bot_position": v[1:4],
                "bot_yaw": v[4], "recoil_pitch": v[5], "recoil_yaw": v[6],
                "shots": v[8], "ammo": v[9], "kills": v[10], "hits": v[11],
                "bot_id": v[12], "health": v[13]}
    raise ChannelNotReady("Host has not published a stable render frame")


@dataclass(frozen=True)
class Observation:
    frame: NativeFrame
    render: dict
    status: dict

    def evidence(self):
        return {"native_frame": asdict(self.frame), "render": self.render,
                "host_updated_at": self.status.get("updated_at"),
                "host_equipment_mode": self.status.get("equipment_mode"),
                "host_ammo": self.status.get("ammo"), "host_shots": self.status.get("shots"),
                "host_kills": self.status.get("kills"), "skills": self.status.get("skills")}


class AbilityVerifier:
    def __init__(self, channel, process, runner, run_dir, summary, *, deadline):
        self.channel, self.process, self.runner = channel, process, runner
        self.run_dir, self.summary, self.deadline = run_dir, summary, deadline
        self.aim_sequence = struct.unpack_from("<I", channel.buffer, 448)[0]
        self.last = None

    def command(self, keys=0):
        if keys & ~1023:
            raise ValueError("Only the explicitly supported stage-4 input bits are accepted")
        struct.pack_into("<I", self.channel.buffer, 16, 0x80000000 | 8 | keys)

    def release_override(self):
        struct.pack_into("<I", self.channel.buffer, 16, 0)

    def observe(self):
        if self.runner.poll() is not None or not self.process.alive():
            raise RuntimeError("Owned game or runner exited before acceptance completed")
        if (self.run_dir / "stop-request.json").is_file():
            raise RuntimeError("Owned runner received a stop request")
        status = read_json(self.run_dir / "training/status.json")
        if status.get("phase") in ("error", "stopping", "stopped"):
            raise RuntimeError("Training host stopped: " + str(status.get("error", status.get("stop_reason", status["phase"]))))
        frame = self.channel.read_frame()
        render = read_render(self.channel)
        self.last = Observation(frame, render, status)
        return self.last

    def wait(self, name, predicate, timeout=5., *, drive=None, record=True):
        until = min(self.deadline, time.monotonic()+timeout)
        while time.monotonic() < until:
            try:
                observed = self.observe()
            except ChannelNotReady:
                time.sleep(.01)
                continue
            if predicate(observed):
                if record:
                    entry = {"name": name, "passed": True, **observed.evidence()}
                    self.summary["checks"].append(entry)
                    print(json.dumps({"check": name, "passed": True,
                                      "ammo": observed.render["ammo"], "shots": observed.render["shots"],
                                      "kills": observed.render["kills"], "position": observed.frame.position}), flush=True)
                return observed
            if drive:
                drive(observed)
            time.sleep(.008)
        raise RuntimeError(f"Timed out waiting for observed {name}")

    def pulse(self, key):
        self.command(0)
        self.wait("input_released", lambda o: not o.frame.input_flags & key, 2, record=False)
        self.command(key)
        pressed = self.wait("input_received", lambda o: bool(o.frame.input_flags & key), 2, record=False)
        self.command(0)
        self.wait("input_release_received", lambda o: o.frame.sequence != pressed.frame.sequence and not o.frame.input_flags & key,
                  2, record=False)

    def aim_now(self, pitch, yaw):
        self.aim_sequence = (self.aim_sequence+1) & 0xffffffff or 1
        struct.pack_into("<3d", self.channel.buffer, 456, pitch, yaw, 0.)
        struct.pack_into("<I", self.channel.buffer, 448, self.aim_sequence)

    def aim(self, pitch, yaw):
        self.aim_now(pitch, yaw)
        sequence = self.aim_sequence
        def reached(o):
            angular = lambda a, b: abs((a-b+180) % 360-180)
            return (struct.unpack_from("<I", self.channel.buffer, 480)[0] == sequence
                    and angular(o.frame.rotation[0], pitch) < 1
                    and angular(o.frame.rotation[1], yaw) < 1)
        return self.wait("camera_aim_observed", reached, 3, record=False)

    def ready_to_fire(self):
        return self.wait("equip_lock_finished", lambda o: not o.frame.skill_flags & SKILL_BLOCKED, 4, record=False)

    def settle(self):
        previous = None
        stable_since = None
        def stable(o):
            nonlocal previous, stable_since
            if previous is not None and o.frame.time > previous.time:
                speed = math.dist(o.frame.position, previous.position)/(o.frame.time-previous.time)
                stable_since = (stable_since or o.frame.time) if speed < 5 else None
            previous = o.frame
            return stable_since is not None and o.frame.time-stable_since >= .25
        self.command(0)
        return self.wait("native_position_settled", stable, 6, record=False)

    def choose_dash_direction(self, baseline):
        position = baseline.frame.position
        ground = self.channel.trace(position, (0, 0, -1), 500, alive=self.process.alive)
        if not ground.hit:
            raise RuntimeError("Dash baseline has no observed ground within 500cm")
        candidates = []
        for index in range(8):
            yaw = (baseline.frame.rotation[1]+index*45+180) % 360-180
            direction = (math.cos(math.radians(yaw)), math.sin(math.radians(yaw)), 0)
            distances = []
            for dz in (0, -85):
                origin = (position[0], position[1], position[2]+dz)
                hit = self.channel.trace(origin, direction, 800, alive=self.process.alive)
                distances.append(hit.distance_cm if hit.hit else 800.)
            destination = (position[0]+direction[0]*650, position[1]+direction[1]*650, position[2]+80)
            floor = self.channel.trace(destination, (0, 0, -1), 500, alive=self.process.alive)
            valid = min(distances) >= 750 and floor.hit and abs(floor.impact[2]-ground.impact[2]) < 35
            candidates.append({"yaw": yaw, "clearance_cm": distances, "floor_hit": floor.hit,
                               "floor_z": floor.impact[2] if floor.hit else None, "valid": valid})
            if valid:
                self.summary["dash_route_observations"] = candidates
                return yaw, direction
        self.summary["dash_route_observations"] = candidates
        raise RuntimeError("No dash direction passed original-scene clearance and destination-ground checks")

    def run(self):
        self.command(0)
        initial = self.wait("native_ready", lambda o: o.status.get("phase") == "running"
            and o.frame.equipment_mode == 0 and o.status.get("equipment_mode") == 0 and o.render["ammo"] > 0, 35)
        rifle_ammo = initial.render["ammo"]
        initial_ultimate = initial.frame.ultimate_casts
        self.pulse(512)
        self.wait("knife_equip", lambda o: o.frame.equipment_mode == 1
            and o.frame.ultimate_casts > initial_ultimate and o.status.get("equipment_mode") == 1
            and o.render["ammo"] == 5 and o.status.get("ammo") == 5)
        self.ready_to_fire()
        before_miss = self.aim(80, initial.frame.rotation[1])
        self.command(1)
        try:
            self.wait("knife_miss_spends_one", lambda o: o.render["shots"] == before_miss.render["shots"]+1
                and o.render["ammo"] == 4 and o.render["kills"] == before_miss.render["kills"])
        finally:
            self.command(0)
        kills_before = self.last.render["kills"]
        shots_before = self.last.render["shots"]
        def aim_at_head(o):
            point = target_head_for_status(o.frame, o.status)
            if (not o.render["flags"] & 1 or o.render["health"] <= 0 or point is None
                    or o.frame.target_bot_id != o.render["bot_id"]):
                self.command(0)
                return
            delta = tuple(point[i]-o.frame.position[i] for i in range(3))
            self.aim_now(math.degrees(math.atan2(delta[2], math.hypot(delta[0], delta[1]))),
                         math.degrees(math.atan2(delta[1], delta[0])))
            self.command(1)
        try:
            self.wait("knife_kill_refills", lambda o: o.render["kills"] > kills_before
                and o.render["shots"] > shots_before and o.render["ammo"] == 5
                and o.frame.equipment_mode == 1, 15, drive=aim_at_head)
        finally:
            self.command(0)
        self.aim(80, initial.frame.rotation[1])
        # Wait longer than one knife cadence; use native timestamp progression.
        native_time = self.last.frame.time
        before_burst = self.wait("knife_burst_cadence_ready", lambda o: o.frame.time-native_time >= .4, 2, record=False)
        self.pulse(4)
        self.wait("knife_burst_exhausts", lambda o: o.render["ammo"] == 0
            and o.render["shots"] == before_burst.render["shots"]+5
            and o.render["kills"] == before_burst.render["kills"])
        self.pulse(512)
        self.wait("rifle_ammo_restored", lambda o: o.frame.equipment_mode == 0
            and o.status.get("equipment_mode") == 0 and o.render["ammo"] == rifle_ammo)

        self.aim(0, initial.frame.rotation[1])
        smoke_before = self.last.frame.smoke_casts
        self.pulse(64)
        appeared = self.wait("smoke_appears", lambda o: o.frame.smoke_casts > smoke_before and o.frame.smoke_count > 0, 4)
        expired = self.wait("smoke_expires", lambda o: o.frame.smoke_count == 0
            and o.frame.smoke_casts == appeared.frame.smoke_casts, 8)
        duration = expired.frame.time-appeared.frame.time
        if not 2.5 <= duration <= 6.5:
            raise RuntimeError(f"Observed smoke duration outside local lifetime bounds: {duration:.3f}s")
        self.summary["smoke_observed_duration_seconds"] = duration

        baseline = self.settle()
        self.pulse(128)
        up = self.wait("updraft_displacement", lambda o: o.frame.updraft_casts > baseline.frame.updraft_casts
            and o.frame.position[2]-baseline.frame.position[2] > 70, 3)
        self.summary["updraft_observation"] = {"before": baseline.frame.position, "after": up.frame.position,
            "rise_cm": up.frame.position[2]-baseline.frame.position[2]}
        settled = self.settle()
        yaw, direction = self.choose_dash_direction(settled)
        baseline = self.aim(0, yaw)
        self.pulse(256)
        def moved(o):
            dx, dy = o.frame.position[0]-baseline.frame.position[0], o.frame.position[1]-baseline.frame.position[1]
            return (o.frame.dash_casts > baseline.frame.dash_casts and math.hypot(dx, dy) > 100
                    and dx*direction[0]+dy*direction[1] > 80)
        dash = self.wait("dash_displacement", moved, 3)
        self.summary["dash_observation"] = {"before": baseline.frame.position, "after": dash.frame.position,
            "horizontal_cm": math.dist(baseline.frame.position[:2], dash.frame.position[:2])}
        self.ready_to_fire()
        self.pulse(512)
        self.wait("final_knife_equip", lambda o: o.frame.equipment_mode == 1
            and o.frame.ultimate_casts > initial_ultimate+1 and o.render["ammo"] == 5
            and o.status.get("equipment_mode") == 1)
        self.command(0)
        self.ready_to_fire()
        self.summary["host_status"] = self.last.status
        self.summary["final_observation"] = self.last.evidence()
        self.summary["ability_checks_passed"] = [x["name"] for x in self.summary["checks"]] == list(CHECKS)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", action="store_true", help="Build and start the bounded owned stage-4 verification runner")
    parser.add_argument("--hold-on-pass", type=float, default=0., metavar="SECONDS",
                        help="Leave the final knife view open after checks, with test input released (0-60s)")
    args = parser.parse_args(argv)
    if not math.isfinite(args.hold_on_pass) or not 0 <= args.hold_on_pass <= 60:
        parser.error("--hold-on-pass must be between 0 and 60 seconds")
    if not args.run:
        print(json.dumps({"will_launch": False, "checks": CHECKS,
            "run_command": "python tools/verify_native_abilities.py --run --hold-on-pass 20"}, ensure_ascii=False))
        return 0

    build()
    folder = ROOT / ".local/ability-verification" / datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    archive = folder / "config.bin"
    prepare(ROOT, 1, "navigation", archive)
    result_file = folder / "run.json"
    summary = {"kind": "automated_native_jett_ability_check", "passed": False,
               "ability_checks_passed": False, "manual_input_verification": False,
               "desktop_automation_used": False, "production_input_override_enabled": False,
               "checks": [], "hold_on_pass_seconds": args.hold_on_pass}
    runner = process = channel = verifier = None
    run_dir = None
    started = time.monotonic()
    log = (folder / "runner.log").open("w", encoding="utf-8")
    try:
        runner = subprocess.Popen([sys.executable, str(ROOT / "tools/run_native_baseline.py"), "--run",
            "--seconds", "180", "--bootstrap", str(archive), "--map", "1", "--mode", "navigation",
            "--observe-world", "--training-module", "--module-stage", "4", "--no-training-hud",
            "--result-file", str(result_file)], cwd=ROOT, stdout=log, stderr=log,
            creationflags=subprocess.CREATE_NO_WINDOW)
        while runner.poll() is None and time.monotonic()-started < 25:
            active = read_json(result_file.with_suffix(".active.json"))
            if active.get("pid") and active.get("run_dir"):
                break
            time.sleep(.05)
        else:
            raise RuntimeError("Owned game did not publish its verification lifecycle")
        run_dir = Path(active["run_dir"])
        summary["run_dir"], summary["owned_pid"] = str(run_dir), active["pid"]
        process = ProcessWatch(active["pid"])
        channel = TrainingChannel.open(active["pid"], alive=process.alive)
        verifier = AbilityVerifier(channel, process, runner, run_dir, summary, deadline=started+110)
        verifier.run()
        summary["passed"] = summary["ability_checks_passed"]
        verifier.release_override()
        if args.hold_on_pass:
            print(json.dumps({"ability_checks_passed": True, "observation_window_seconds": args.hold_on_pass,
                              "test_input_released": True}), flush=True)
            until = time.monotonic()+args.hold_on_pass
            while time.monotonic() < until:
                if (run_dir / "stop-request.json").exists():
                    summary["observation_window_ended_by_stop_request"] = True
                    break
                if runner.poll() is not None or not process.alive():
                    raise RuntimeError("Owned game exited unexpectedly during the observation window")
                status = read_json(run_dir / "training/status.json")
                if status.get("phase") == "error":
                    raise RuntimeError("Training host failed during observation: " + str(status.get("error")))
                time.sleep(.1)
    except Exception as error:
        summary["passed"] = False
        summary["error"] = str(error)
        if verifier and verifier.last:
            summary["last_observation"] = verifier.last.evidence()
    finally:
        if channel:
            struct.pack_into("<I", channel.buffer, 16, 0)
            channel.close()
        if process:
            process.close()
        if run_dir and not (run_dir / "stop-request.json").exists():
            save(run_dir / "stop-request.json", {"reason": "ability_integration_check_complete"})
        if runner:
            # Keep the owned runner's cleanup rather than killing an arbitrary PID.
            while runner.poll() is None and time.monotonic()-started < 200:
                try:
                    runner.wait(timeout=1)
                except subprocess.TimeoutExpired:
                    pass
        log.close()
        final = read_json(result_file)
        summary["game_image_unchanged"] = final.get("image_unchanged")
        summary["owned_game_exited"] = final.get("owned_process_exited")
        summary["remaining_game_or_launcher_processes"] = final.get("remaining_game_or_launcher_processes")
        summary["runner_status"] = final.get("status")
        if final.get("status") != "recorded" or not final.get("image_unchanged") or not final.get("owned_process_exited"):
            summary["passed"] = False
        if run_dir:
            summary["events"] = read_json(run_dir / "training/events.json").get("events", [])
        save(folder / "verification.json", summary)
    print(json.dumps({"passed": summary["passed"], "error": summary.get("error"),
                      "report": str(folder / "verification.json")}, ensure_ascii=False), flush=True)
    return 0 if summary["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
