"""Observe original Jett C/Q/E/X through the owned stage9 test protocol.

This command never builds the module. It sends expiring original input/aim
commands and reads native observations; it supplies no movement, projectile,
animation, spread or damage simulation. A missing observation fails its check.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import struct
import subprocess
import sys
import time

from original_probe_channel import OriginalProbe
from run_map_probe import prepare
from run_native_probe import ROOT, save
from training_channel import ChannelNotReady, ProcessWatch, TrainingChannel


SKILLS = ("Cloudburst", "Updraft", "Tailwind", "BladeStorm")
BITS = dict(zip("cqex", (64, 128, 256, 512)))
DIAGNOSTICS = ROOT / ".local/native-module"


def read_json(path):
    try:
        return json.loads(Path(path).read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return {}


def pointer(value):
    try:
        return int(str(value), 16)
    except (ValueError, TypeError):
        return 0


def skill(document, index):
    entries = document.get("skills", [])
    return next((entry for entry in entries if entry.get("name") == SKILLS[index]), {})


def compact_jett(document):
    fields = ("samples", "error", "stage", "pawn", "initialized", "loadout_granted",
              "restart_required", "ultimate_ready", "ultimate_points", "ultimate_required",
              "active_force_modules", "activation_submissions", "refill_calls", "projectiles")
    return {name: document[name] for name in fields if name in document} | {
        "skills": document.get("skills", [])}


def compact_effects(document, jett):
    # Original state FXCs can belong to the pawn/controller manager. Preserve
    # those managers as well; a skill-actor-only filter loses their evidence.
    fields = ("label", "actor", "component", "owner", "registered", "identity_verified", "generation",
              "process_events", "effect_containers", "unstopped_containers", "max_effect_containers",
              "last_container", "last_container_class", "last_target")
    return {"samples": document.get("samples", 0), "error": document.get("error", 0),
            "managers": [{name: entry[name] for name in fields if name in entry}
                         for entry in document.get("managers", [])]}


def manager(row, index):
    actor = pointer(skill(row.get("jett", {}), index).get("actor"))
    return next((entry for entry in row.get("effects", {}).get("managers", [])
                 if pointer(entry.get("actor")) == actor), {})


def owned_character_effect_grew(before, row):
    """Updraft's native FXC can be owned by the pawn, not its ability actor."""
    pawn = pointer(row.get('jett', {}).get('pawn'))
    if not pawn or pawn != pointer(before.get('jett', {}).get('pawn')):
        return False
    def entry(sample):
        return next((m for m in sample.get('effects', {}).get('managers', [])
                     if m.get('label') == 'local_character' and pointer(m.get('actor')) == pawn), {})
    initial, current = entry(before), entry(row)
    return (bool(initial) and bool(current) and current.get('registered') and current.get('identity_verified')
            and current.get('component') == initial.get('component')
            and current.get('generation') == initial.get('generation')
            and current.get('effect_containers', 0) > 0
            and current.get('last_container') != initial.get('last_container')
            and pointer(current.get('last_target')) == pawn
            and pointer(current.get('last_container_class')) != 0
            and current.get('last_container_class') != initial.get('last_container_class'))


def maximum(rows, getter, default=0):
    return max((getter(row) for row in rows), default=default)


def field_number(document, name, default=0):
    result = document.get(name, default)
    return result if isinstance(result, (int, float)) and math.isfinite(result) else default


def projectile(row, name):
    if name == "smoke_zone":
        return row.get("skill_world", {}).get("smoke_zone", {})
    return row.get("jett", {}).get("projectiles", {}).get(name, {})


class Observations:
    """One watchdog spans every phase, including short press/release phases."""

    def __init__(self, probe, watch, pid, active_file, folder):
        self.probe, self.watch, self.pid = probe, watch, pid
        self.active_file, self.folder = active_file, folder
        self.last_native_time = None
        self.native_changed_at = time.monotonic()
        self.last_diagnostic_at = time.monotonic()
        self.last_jett_sample = -1
        self.pawn = 0
        self.jett = self.effects = self.training = {}
        self.gameplay = self.tuning = {}
        self.sync = self.skill_events = {}
        self.skill_world = {}
        self.samples = []
        self.phase_summaries = []
        self.hang_capture = None

    def stale(self, phase, reason):
        try:
            from capture_owned_thread import capture_owned_thread
            self.hang_capture = capture_owned_thread(
                self.active_file, self.folder / "hang-thread.json", reason=reason + ":" + phase)
        except Exception as error:
            self.hang_capture = {"error": str(error), "reason": reason, "phase": phase}
        raise RuntimeError(reason + " during " + phase)

    def read(self, phase):
        if not self.watch.alive():
            raise RuntimeError("The explicitly owned game process exited during " + phase)
        frame = self.probe.read()
        now = time.monotonic()
        if frame.time != self.last_native_time:
            if self.last_native_time is not None and frame.time < self.last_native_time:
                raise RuntimeError("Original simulation time moved backwards")
            self.last_native_time, self.native_changed_at = frame.time, now
        elif now - self.native_changed_at > 1.5:
            self.stale(phase, "Original simulation stopped advancing")
        if frame.observer_error:
            raise RuntimeError("Original rifle observer rejected a payload")

        bridge = read_json(DIAGNOSTICS / "bridge-state.json")
        if bridge and bridge.get("pid") != self.pid:
            raise RuntimeError("Native diagnostics belong to a different process")
        if bridge.get("error"):
            raise RuntimeError("Native bridge error: " + str(bridge.get("stage")))
        if bridge.get("pid") == self.pid:
            current = read_json(DIAGNOSTICS / "original-jett.json")
            counter = current.get("samples", -1)
            if current.get("pid") == self.pid and isinstance(counter, int) and counter > self.last_jett_sample:
                if current.get("error") or current.get("restart_required"):
                    raise RuntimeError("Original Jett error: " + str(current.get("stage")))
                actual_pawn = pointer(current.get("pawn"))
                if self.pawn and actual_pawn != self.pawn:
                    raise RuntimeError("Original Jett observation changed its local pawn")
                if actual_pawn:
                    self.pawn = actual_pawn
                self.jett = compact_jett(current)
                self.last_jett_sample, self.last_diagnostic_at = counter, now
            self.effects = compact_effects(
                read_json(DIAGNOSTICS / "original-effects.json"), self.jett)
            self.training = read_json(DIAGNOSTICS / "original-training.json")
            gameplay = read_json(DIAGNOSTICS / "original-gameplay.json")
            tuning = read_json(DIAGNOSTICS / "original-ability-tuning.json")
            if gameplay.get("pid") == self.pid:
                self.gameplay = gameplay
            if tuning.get("pid") == self.pid:
                self.tuning = tuning
            sync = read_json(DIAGNOSTICS / "original-jett-sync.json")
            skill_events = read_json(DIAGNOSTICS / "original-jett-events.json")
            if sync.get("pid") == self.pid:
                self.sync = sync
            if skill_events.get("pid") == self.pid:
                self.skill_events = skill_events
            world = read_json(DIAGNOSTICS / "original-jett-world.json")
            if world.get("pid") == self.pid:
                self.skill_world = {name: world[name] for name in
                    ("samples", "error", "smoke_zone", "dagger", "dagger_native_events", "dagger_invalid_geometry",
                     "smoke_loop_actors", "smoke_loop_actors_maximum", "smoke_loop_class") if name in world}
        if self.pawn and now - self.last_diagnostic_at > 2.5:
            self.stale(phase, "Original skill diagnostics stopped advancing")
        return {"phase": phase, **frame.as_dict(), "jett": self.jett,
                "effects": self.effects, "training": self.training,
                "gameplay": self.gameplay, "tuning": self.tuning, "skill_world": self.skill_world,
                "original_e_sync": self.sync, "original_skill_events": self.skill_events}

    def phase(self, name, seconds, *, pitch=0., yaw=0., aim_at_target=False, **command):
        started = self.read(name)
        phase_end = started["time"] + seconds
        deadline = time.monotonic() + max(4., seconds * 6.)
        rows = []
        flags = command.pop("flags", 8)
        while True:
            target = None
            target_pitch, target_yaw = pitch, yaw
            if aim_at_target:
                current = self.probe.read()
                live = {pointer(item.get("pawn")) for item in self.training.get("targets", [])
                        if item.get("alive") and item.get("original_physics_ready")}
                candidates = [item for item in read_json(DIAGNOSTICS / "original-enemies.json").get("enemies", [])
                              if item.get("ready") and pointer(item.get("pawn")) in live
                              and len(item.get("position", ())) == 3]
                target = min(candidates, key=lambda item: math.dist(item["position"], current.camera), default=None)
                if target:
                    x, y, z = target["position"]
                    # Aim inside the actual original capsule, using its measured
                    # center/height. The game's native projectile and original
                    # body collision decide whether this test aim actually hits.
                    z += float(target.get("capsule_half_height", 0)) * .2
                    dx, dy, dz = x-current.camera[0], y-current.camera[1], z-current.camera[2]
                    target_pitch = math.degrees(math.atan2(dz, math.hypot(dx, dy)))
                    target_yaw = math.degrees(math.atan2(dy, dx))
            self.probe.aim(pitch=target_pitch, yaw=target_yaw)
            self.probe.command(flags=flags, **command)
            row = self.read(name)
            if aim_at_target:
                row["aim_target_roster"] = target.get("roster_index") if target else None
                row["aim_target_pawn"] = target.get("pawn") if target else None
            rows.append(row)
            self.samples.append(row)
            if row["time"] >= phase_end:
                break
            if time.monotonic() > deadline:
                raise RuntimeError("Original phase exceeded its bounded deadline: " + name)
            time.sleep(.015)
        item = {"phase": name, "native_start": started["time"],
                "native_end": rows[-1]["time"], "samples": len(rows),
                "charges": [skill(rows[-1]["jett"], i).get("charges") for i in range(4)],
                "maximum_horizontal_speed": maximum(rows, lambda r: math.hypot(*r["velocity"][:2])),
                "maximum_z": maximum(rows, lambda r: r["location"][2]),
                "native_skill_samples": rows[-1]["jett"].get("samples", 0)}
        self.phase_summaries.append(item)
        print(json.dumps(item, ensure_ascii=False), flush=True)
        return rows


def evaluate(groups, selected):
    base = groups["settle"][-1]
    assertions = {"original_physics_advanced": groups["finish"][-1]["physics_calls"] > base["physics_calls"],
                  "original_four_skill_grants": all(skill(base["jett"], i).get("grant_ready") for i in range(4)),
                  "original_skill_diagnostics_advanced": groups["finish"][-1]["jett"].get("samples", 0)
                  > base["jett"].get("samples", 0),
                  "original_gameplay_active": any(row.get("gameplay", {}).get("ready") for row in groups["finish"])}
    measured = {}
    all_rows = [row for rows in groups.values() for row in rows]
    for key, index in zip("cqex", range(4)):
        if key not in selected:
            continue
        rows = [row for name, values in groups.items() if name.startswith(key + "_") for row in values]
        before = groups[key + "_baseline"][-1]
        initial = skill(before["jett"], index)
        minimum_charges = min((field_number(skill(r["jett"], index), "charges", -1) for r in rows), default=-1)
        transitions = maximum(rows, lambda r: field_number(skill(r["jett"], index), "state_transitions"))
        input_edges = maximum(rows, lambda r: field_number(skill(r["jett"], index), "input_edges"))
        assertions[key + "_original_activation_edges"] = input_edges >= field_number(initial, "input_edges") + 2
        assertions[key + "_original_state_progress"] = transitions > field_number(initial, "state_transitions")
        assertions[key + "_original_resource_consumed"] = 0 <= minimum_charges < field_number(initial, "charges", -1)
        initial_containers = field_number(manager(before, index), "max_effect_containers")
        initial_active = field_number(manager(before, index), "unstopped_containers")
        if key != "c":
            assertions[key + "_original_ability_effect_container"] = any(
                field_number(manager(row, index), "max_effect_containers") > initial_containers
                or field_number(manager(row, index), "unstopped_containers") > initial_active
                or (key == 'q' and owned_character_effect_grew(before, row)) for row in rows)
        measured[key] = {"initial_charges": initial.get("charges"), "minimum_charges": minimum_charges,
                         "state_transitions": transitions, "input_edges": input_edges}
        if key in "cx":
            category = "smoke" if key == "c" else "dagger"
            initialized = maximum(rows, lambda r: field_number(projectile(r, category), "local_initializations"))
            previous = field_number(projectile(before, category), "local_initializations")
            assertions[key + "_original_local_projectile_initialized"] = initialized > previous
            measured[key]["observed_native_projectile_initializations"] = initialized - previous
            if key == "c":
                assertions["c_original_smoke_zone_observed"] = maximum(rows, lambda r: field_number(
                    projectile(r, "smoke_zone"), "local_initializations")) > field_number(
                    projectile(before, "smoke_zone"), "local_initializations")
                measured[key]["original_smoke_loop_effect_observed"] = any(
                    field_number(r.get("skill_world", {}), "smoke_loop_actors") > 0 for r in rows)
                assertions["c_original_smoke_loop_effect"] = measured[key]["original_smoke_loop_effect_observed"]
        if key in "qe":
            assertions[key + "_original_force_module_observed"] = any(
                field_number(r["jett"], "active_force_modules") > 0
                or skill(r["jett"], index).get("force_handle_active")
                or field_number(skill(r["jett"], index), "force_active_observations")
                > field_number(initial, "force_active_observations") for r in rows)
        if key == "q":
            rise = maximum(rows, lambda r: r["location"][2]) - before["location"][2]
            expected_jump = before.get("original_jump_height", 0)
            assertions["q_original_upward_motion"] = rise > max(100., expected_jump * 1.1)
            camera_rise = maximum(rows, lambda r: r["camera"][2]) - before["camera"][2]
            assertions["q_original_camera_follows_updraft"] = rise > 0 and camera_rise >= rise*.8
            measured[key].update(rise_cm=rise, camera_rise_cm=camera_rise, original_jump_height_cm=expected_jump)
        elif key == "e":
            speed = maximum(rows, lambda r: math.hypot(*r["velocity"][:2]))
            normal = maximum(groups["e_calibrate_run"], lambda r: math.hypot(*r["velocity"][:2]))
            distance = maximum(rows, lambda r: math.dist(r["location"][:2], before["location"][:2]))
            assertions["e_original_dash_exceeds_rifle_run"] = normal > 100. and speed > normal * 1.5
            assertions["e_original_dash_displacement"] = distance > 100.
            measured[key].update(maximum_speed_cm_s=speed, original_rifle_run_cm_s=normal,
                                 maximum_displacement_cm=distance)
        elif key == "x":
            equip = groups["x_equip"] + groups["x_equip_settle"]
            assertions["x_original_blade_storm_equipped"] = any(skill(r["jett"], index).get("current_equippable") for r in equip)
            assertions["x_original_five_daggers_ready"] = any(skill(r["jett"], index).get("charges") == 5 for r in equip)
            one = groups["x_primary_one"] + groups["x_primary_one_settle"]
            two = groups["x_primary_two"] + groups["x_primary_two_settle"]
            hit = groups["x_primary_hit"] + groups["x_primary_hit_settle"]
            burst = groups["x_secondary_burst"] + groups["x_secondary_burst_settle"]
            first_min = min((field_number(skill(r["jett"], index), "charges", -1) for r in one), default=-1)
            second_min = min((field_number(skill(r["jett"], index), "charges", -1) for r in two), default=-1)
            assertions["x_original_primary_consumes_one_each"] = first_min == 4 and second_min == 3
            assertions["x_original_aimed_primary_consumes_one"] = any(
                skill(r["jett"], index).get("charges") == 2 for r in hit)
            assertions["x_aimed_at_actual_original_enemy"] = any(r.get("aim_target_pawn") for r in hit)
            assertions["x_original_burst_consumes_remaining"] = any(skill(r["jett"], index).get("charges") == 0 for r in burst)
            assertions["x_does_not_consume_rifle_ammo"] = all(r["ammo"] == before["ammo"] for r in rows)
            assertions["x_original_native_point_damage_event_observed"] = maximum(rows, lambda r: field_number(
                r.get("skill_world", {}), "dagger_native_events")) > field_number(
                before.get("skill_world", {}), "dagger_native_events")
            # The rifle observer is deliberately source-bound. A knife damage
            # adapter must publish its own original projectile/tuning evidence.
            assertions["x_original_damage_verified"] = any(
                field_number(r.get("training", {}), "original_dagger_damage_applied") >
                field_number(before.get("training", {}), "original_dagger_damage_applied") for r in rows)
    return assertions, measured, all_rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--skills", default="cqex", help="Subset of cqex, in that fixed order")
    parser.add_argument("--attach-active", type=Path, help="Use the explicitly supplied owned baseline .active.json; do not stop it")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--ready-timeout", type=float, default=50.)
    parser.add_argument("--seconds", type=int, default=140, help="Owned launch lifetime, at most180 seconds")
    parser.add_argument("--yaw", type=float, default=0.)
    args = parser.parse_args()
    selected = "".join(key for key in "cqex" if key in args.skills.lower())
    if not selected or any(key not in "cqex" for key in args.skills.lower()):
        parser.error("--skills must contain only c, q, e and x")
    if not 60 <= args.seconds <= 180 or not 1 <= args.ready_timeout <= 90 or not math.isfinite(args.yaw):
        parser.error("Invalid bounded lifetime, readiness timeout or yaw")
    folder = args.output or ROOT / ".local/original-jett-verification" / datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    folder.mkdir(parents=True, exist_ok=False)
    result = folder / "run.json"
    active_file = args.attach_active or result.with_suffix(".active.json")
    summary = {"kind": "original_jett_runtime_check", "passed": False,
               "selected_skills": selected, "module_stage": 9, "build_performed": False,
               "python_skill_simulation": False, "os_input_injection": False,
               "original_animation_pixels_verified": False,
               "owned_launch": args.attach_active is None}
    runner = watch = channel = observations = None
    run_dir = None
    log = None
    groups = {}
    try:
        if args.attach_active is None:
            prepared = prepare(ROOT, 1, "navigation", folder / "config.bin")
            log = (folder / "runner.log").open("w", encoding="utf-8")
            runner = subprocess.Popen([sys.executable, str(ROOT / "tools/run_native_baseline.py"), "--run",
                "--seconds", str(args.seconds), "--bootstrap", prepared["archive"], "--map", "1", "--mode", "navigation",
                "--observe-world", "--training-module", "--module-stage", "9", "--original-game-instance",
                "--original-visual-classes", "--no-training-hud", "--result-file", str(result)],
                cwd=ROOT, stdout=log, stderr=log, creationflags=subprocess.CREATE_NO_WINDOW)
        deadline = time.monotonic() + args.ready_timeout
        while time.monotonic() < deadline:
            active = read_json(active_file)
            if active.get("pid"):
                break
            if runner and runner.poll() is not None:
                raise RuntimeError("Owned baseline runner exited before publishing its lifecycle")
            time.sleep(.05)
        else:
            raise RuntimeError("Owned stage9 lifecycle was not published")
        pid = active["pid"]
        if not isinstance(pid, int) or pid <= 0 or not active.get("run_dir"):
            raise RuntimeError("Invalid explicitly supplied owned baseline lifecycle")
        run_dir = Path(active["run_dir"])
        summary.update(pid=pid, run_dir=str(run_dir), active_file=str(active_file))
        watch = ProcessWatch(pid)
        channel = TrainingChannel.open(pid, alive=watch.alive, timeout=args.ready_timeout)
        if struct.unpack_from("<I", channel.buffer, 56)[0] != 9:
            raise RuntimeError("This verifier requires explicit module stage9")
        probe = OriginalProbe(channel)
        observations = Observations(probe, watch, pid, active_file, folder)
        deadline = time.monotonic() + args.ready_timeout
        while time.monotonic() < deadline:
            try:
                row = observations.read("ready")
                if row["flags"] & 1 and row["gameplay"].get("ready") and row["jett"].get("loadout_granted") and all(
                        skill(row["jett"], i).get("grant_ready") for i in range(4)):
                    break
            except ChannelNotReady:
                pass
            time.sleep(.025)
        else:
            raise RuntimeError("Original AK, gameplay-active gate and four original ability actors were not ready")

        def phase(name, seconds, **command):
            rows = observations.phase(name, seconds, yaw=args.yaw, **command)
            groups[name] = rows
            return rows

        phase("settle", .7)
        for key in selected:
            phase(key + "_baseline", .9)
            if key == "c":
                phase("c_throw_hold", .25, flags=8 | BITS[key], pitch=-8.)
                phase("c_throw_release", 2.4, pitch=-8.)
            elif key == "q":
                phase("q_updraft_press", .15, flags=8 | BITS[key])
                phase("q_updraft_flight", 3.2)
            elif key == "e":
                phase("e_calibrate_run", .65, forward=1.)
                phase("e_calibrate_return", .65, forward=-1.)
                phase("e_prime", .12, flags=8 | BITS[key])
                # This is a test wait, not a replacement ability timer. Give
                # the original priming state time to finish before the next
                # distinct activation edge; its own authored timer still runs.
                phase("e_prime_settle", 1.5)
                phase("e_dash", .15, flags=8 | BITS[key], forward=1.)
                phase("e_dash_continue", .4, forward=1.)
                phase("e_dash_settle", 1.1)
            elif key == "x":
                # Stage9 bit16 requests the original training coordinator's
                # explicit stationary-target test; it sends real zero AI input.
                phase("x_equip", .15, flags=24 | BITS[key])
                phase("x_equip_settle", 1.6, flags=24)
                phase("x_primary_one", .12, flags=25)
                phase("x_primary_one_settle", .8, flags=24)
                phase("x_primary_two", .12, flags=25)
                phase("x_primary_two_settle", .8, flags=24)
                phase("x_primary_hit", .12, flags=25, aim_at_target=True)
                phase("x_primary_hit_settle", .8, flags=24, aim_at_target=True)
                phase("x_secondary_burst", .12, flags=28, aim_at_target=True)
                phase("x_secondary_burst_settle", 1.5, flags=24, aim_at_target=True)
        phase("finish", 1.2)
        probe.release()
        assertions, measured, _ = evaluate(groups, selected)
        execution = {}
        for key in selected:
            core = [value for name, value in assertions.items() if name.startswith(key + "_")
                    and "effect_container" not in name and name not in
                    ("c_original_smoke_zone_observed", "x_original_damage_verified")]
            execution[key] = {"original_input_state_resource_and_motion_or_projectile_passed":
                              bool(core) and all(value is True for value in core),
                              "skill_actor_effect_container_observed":
                              assertions.get(key + "_original_ability_effect_container", False)}
            if key == "c":
                execution[key]["original_smoke_zone_observed"] = assertions["c_original_smoke_zone_observed"]
                execution[key]["original_smoke_loop_effect_observed"] = assertions["c_original_smoke_loop_effect"]
            if key == "x":
                execution[key]["original_dagger_damage_verified"] = assertions["x_original_damage_verified"]
        summary.update(assertions=assertions, measured=measured,
                       execution=execution,
                       passed=bool(assertions) and all(value is True for value in assertions.values()),
                       skill_execution_passed=all(value is True for key, value in assertions.items()
                                                  if key != "x_original_damage_verified"),
                       original_dagger_damage_verified=assertions.get("x_original_damage_verified", False),
                       phases=observations.phase_summaries,
                       observation_boundaries=[
                           "Effect and native state counters do not certify rendered hand/knife/smoke pixels.",
                           "A skill-actor effect check can miss original FXCs owned by the pawn/controller; their managers are retained in samples.",
                           "Smoke projectile evidence alone does not certify the original smoke-zone actor.",
                           "Original rifle ray evidence is never counted as original knife evidence.",
                           "Missing native projectile or knife damage observations fail their checks."])
    except Exception as error:
        summary["error"] = str(error)
    finally:
        if observations:
            summary["hang_capture"] = observations.hang_capture
            save(folder / "samples.json", {"samples": observations.samples})
        if channel:
            try:
                OriginalProbe(channel).release()
            except Exception:
                pass
            channel.close()
        if runner:
            if run_dir:
                save(run_dir / "stop-request.json", {"reason": "original_jett_verification_complete"})
            try:
                runner.wait(timeout=10)
            except subprocess.TimeoutExpired:
                runner.terminate()
                runner.wait(timeout=10)
        if watch:
            watch.close()
        if log:
            log.close()
    summary["run"] = read_json(result) if runner else {"attached": True}
    save(folder / "verification.json", summary)
    print(json.dumps({"passed": summary["passed"], "assertions": summary.get("assertions"),
                      "error": summary.get("error"), "artifact": str(folder / "verification.json")}, ensure_ascii=False))
    return 0 if summary["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
