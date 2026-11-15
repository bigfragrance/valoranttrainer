"""Local rules host for one explicitly supplied, module-equipped game process.

It consumes only the native shared-memory protocol, validates Duel 1 routes with
native scene traces, runs local rules and publishes render commands. It neither
launches nor patches the game and has no network or account operation.
"""
from __future__ import annotations

import argparse
from dataclasses import asdict
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import random
import subprocess
import sys
import time

from local_training_rules import BotLane, DUEL1_MAP, LocalTrainingSession
from training_channel import (ChannelError, ChannelNotReady, NativeFrame, ProcessWatch,
                              RenderState, TrainingChannel, INPUT_ADS, INPUT_FIRE,
                              INPUT_FOCUS, INPUT_RELOAD, INPUT_TAB, INPUT_ESCAPE,
                              RENDER_BOT, RENDER_HIT, RENDER_RELOADING,
                              SKILL_BLOCKED, SKILL_INSIDE_SMOKE, SKILL_BURST)

ROOT = Path(__file__).resolve().parents[1]
from training_settings import load as load_settings


class StopRequested(Exception):
    """The focused training window requested an orderly owner-managed exit."""


def atomic_json(path: Path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    pending = path.with_name(path.name + ".pending")
    pending.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding="utf-8")
    for attempt in range(5):
        try:
            pending.replace(path)
            return
        except PermissionError:
            if attempt == 4:
                raise
            time.sleep(.003)


def aim_direction(rotation):
    pitch, yaw, _ = map(math.radians, rotation)
    return (math.cos(pitch)*math.cos(yaw), math.cos(pitch)*math.sin(yaw), math.sin(pitch))


class FloorEvidence:
    """Candidate floor coverage from the previously extracted original mesh.

    The scene file converts native cm to (x/100,-y/100,z/100). Invert that exact
    transform; final walkability still requires live native ground/path traces.
    """
    def __init__(self, scene_path: Path):
        payload = scene_path.read_bytes()
        scene = json.loads(payload)
        if scene.get("units") != "m":
            raise ValueError("Expected the extracted Duel 1 scene in metres")
        self.path, self.sha256 = str(scene_path), hashlib.sha256(payload).hexdigest()
        self.triangles = []
        self.source_meshes = []
        for mesh in scene.get("meshes", []):
            if "/Shell_0_WarehouseFloorA/" not in mesh.get("name", ""):
                continue
            vertices = [(v[0]*100., -v[1]*100., v[2]*100.) for v in mesh["vertices"]]
            indices = mesh["indices"]
            if len(indices) % 3:
                raise ValueError("Malformed original floor triangle indices")
            self.source_meshes.append({"name": mesh["name"], "source_mesh": mesh.get("source_mesh"),
                                       "source_export": mesh.get("source_export")})
            for start in range(0, len(indices), 3):
                triangle = tuple(vertices[indices[start+i]] for i in range(3))
                if max(v[2] for v in triangle)-min(v[2] for v in triangle) < .5:
                    self.triangles.append(triangle)
        if not self.triangles:
            raise ValueError("Original Duel 1 WarehouseFloorA geometry is missing")

    def height(self, x, y):
        for a, b, c in self.triangles:
            denominator = (b[1]-c[1])*(a[0]-c[0]) + (c[0]-b[0])*(a[1]-c[1])
            if abs(denominator) < 1e-8:
                continue
            u = ((b[1]-c[1])*(x-c[0]) + (c[0]-b[0])*(y-c[1]))/denominator
            v = ((c[1]-a[1])*(x-c[0]) + (a[0]-c[0])*(y-c[1]))/denominator
            if u >= -1e-6 and v >= -1e-6 and u+v <= 1+1e-6:
                return u*a[2]+v*b[2]+(1-u-v)*c[2]
        return None


class LanePlanner:
    def __init__(self, floor, trace, *, max_lanes=4):
        self.floor, self.trace, self.max_lanes = floor, trace, max_lanes
        self.records = []

    def _ground(self, point):
        expected = self.floor.height(point[0], point[1])
        if expected is None:
            return None
        origin = (point[0], point[1], expected+300.)
        hit = self.trace(origin, (0., 0., -1.), 600.)
        if not hit.hit or abs(hit.impact[2]-expected) > 8.:
            return None
        foot = (hit.impact[0], hit.impact[1], hit.impact[2]+1.)
        # Native trace, not a floor-bounds guess, must also prove headroom.
        roof = self.trace((foot[0], foot[1], foot[2]+5.), (0., 0., 1.), 185.)
        if roof.hit:
            return None
        return foot

    def _clear_segment(self, start, end, height, lateral_offset=0.):
        delta = tuple(end[i]-start[i] for i in range(3))
        length = math.sqrt(sum(v*v for v in delta))
        if length < 1:
            return False
        direction = tuple(v/length for v in delta)
        side = (-direction[1]*lateral_offset, direction[0]*lateral_offset)
        origin = (start[0]+side[0], start[1]+side[1], start[2]+height)
        result = self.trace(origin, direction, length)
        return not result.hit or result.distance_cm >= length-.5

    def _visible(self, camera, foot):
        target = (foot[0], foot[1], foot[2]+150.)
        delta = tuple(target[i]-camera[i] for i in range(3))
        distance = math.sqrt(sum(v*v for v in delta))
        result = self.trace(camera, delta, distance)
        return not result.hit or result.distance_cm >= distance-1.

    def plan(self, frame: NativeFrame):
        yaw = math.radians(frame.rotation[1])
        forward, right = (math.cos(yaw), math.sin(yaw)), (-math.sin(yaw), math.cos(yaw))
        lanes = []
        # Only generate candidates in the original floor mesh and in front of
        # the observed camera. The short rows are fallback inside a spawn area.
        for distance in (600., 900., 1200., 1500., 1800., 400., 300.):
            for lateral in (-450., 0., 450.):
                name = f"duel1-{int(distance)}-{int(lateral)}"
                points = [(frame.position[0]+forward[0]*distance+right[0]*(lateral+offset),
                           frame.position[1]+forward[1]*distance+right[1]*(lateral+offset), 0.)
                          for offset in (-120., 120.)]
                start, end = self._ground(points[0]), self._ground(points[1])
                record = {"name": name, "candidate_xy_cm": [list(p[:2]) for p in points], "validated": False}
                self.records.append(record)
                if start is None or end is None or abs(start[2]-end[2]) > 8:
                    record["reason"] = "floor_or_headroom_not_confirmed"
                    continue
                # Ground every 60 cm prevents routes crossing a hole.
                grounded = True
                for step in (1, 2, 3):
                    sample = tuple(start[i]+(end[i]-start[i])*step/4 for i in range(3))
                    ground = self._ground(sample)
                    if ground is None or abs(ground[2]-start[2]) > 8:
                        grounded = False
                        break
                if not grounded:
                    record["reason"] = "intermediate_floor_not_confirmed"
                    continue
                clear = all(self._clear_segment(start, end, height, side)
                            for height in (35., 100., 165.) for side in (-25., 0., 25.))
                if not clear:
                    record["reason"] = "body_path_blocked"
                    continue
                start_visible, end_visible = self._visible(frame.position, start), self._visible(frame.position, end)
                if not start_visible and not end_visible:
                    record["reason"] = "both_endpoints_occluded"
                    continue
                if start_visible and not end_visible:
                    start, end = end, start
                    start_visible, end_visible = end_visible, start_visible
                record.update({"validated": True, "start_cm": start, "end_cm": end,
                               "ground_and_headroom_samples": 5, "body_path_rays": 9,
                               "start_visible": start_visible, "end_visible": end_visible,
                               "native_trace_validation": True})
                lanes.append(BotLane(name, start, end, validated=True))
                if len(lanes) >= self.max_lanes:
                    return lanes
        return lanes


class TrainingHost:
    def __init__(self, channel, run_dir: Path, *, alive=lambda: True, seed=20261101, ui_enabled=False):
        self.channel, self.run_dir, self.alive = channel, run_dir, alive
        self.seed = seed
        self.session = None
        self.sim_time = 0.
        self.paused = False
        self._tab_down = False
        self._last_frame = None
        self._last_event_write = 0.
        self._last_status_write = 0.
        self._last_shot_at = -math.inf
        self._hit_until = -math.inf
        self.events = []
        self.total_events = 0
        self.trace_count = 0
        self.target_trace_count = 0
        self.phase = "waiting_for_native"
        self.stop_requested = False
        self.ui_enabled = ui_enabled
        self.settings_open = False
        self.settings = load_settings()
        self._escape_down = False
        self._last_settings_poll = -math.inf
        self._ui_command_id = None

    def poll_settings(self):
        now = time.monotonic()
        if now-self._last_settings_poll < .15:
            return
        self._last_settings_poll = now
        self.settings = load_settings()
        try:
            command = json.loads((self.run_dir / 'ui-command.json').read_text(encoding='utf-8'))
        except (OSError, ValueError):
            command = {}
        if command.get('id') and command['id'] != self._ui_command_id:
            self._ui_command_id = command['id']
            if command.get('action') == 'resume':
                self.settings_open = False
            elif command.get('action') == 'quit':
                self.stop_requested = True
                self.phase = 'stopping'
                atomic_json(self.run_dir.parent / 'stop-request.json', {'reason': 'user_settings_quit'})
        if hasattr(self.channel, 'write_settings'):
            self.channel.write_settings(self.settings, self.settings_open or self.paused)

    def handle_escape(self, frame: NativeFrame):
        self.poll_settings()
        if self.stop_requested:
            return True
        escape_down = bool(frame.input_flags & INPUT_FOCUS and frame.input_flags & INPUT_ESCAPE)
        pressed = escape_down and not self._escape_down
        self._escape_down = escape_down
        if self.ui_enabled:
            if pressed:
                self.settings_open = not self.settings_open
                self.status()
                self._last_settings_poll = -math.inf
                self.poll_settings()
            return False
        if frame.input_flags & INPUT_FOCUS and frame.input_flags & INPUT_ESCAPE:
            self._last_frame = frame
            self.stop_requested = True
            self.phase = "stopping"
            atomic_json(self.run_dir.parent / "stop-request.json", {
                "reason": "user_escape", "pid": self.channel.pid, "requested_at": time.time()})
            self.status(stop_reason="user_escape")
            self.channel.write_render(RenderState(flags=0))
            return True
        return False

    def status(self, **extra):
        value = {"schema_version": 1, "pid": self.channel.pid, "phase": self.phase,
                 "updated_at": time.time(), "map_package": DUEL1_MAP,
                 "paused": self.paused, "focused": bool(self._last_frame and self._last_frame.input_flags & INPUT_FOCUS),
                 "trace_count": self.trace_count, "target_trace_count": self.target_trace_count, "local_rules": True,
                 "original_server_equivalence_proven": False}
        value.update({'settings_open': self.settings_open, 'settings': self.settings})
        if self.session:
            value.update(self.session.snapshot())
            value["hitmarker"] = self.sim_time < self._hit_until
            value["reloading"] = self.session.weapon.reload_ends_at is not None
        frame = self._last_frame
        if frame:
            value['native_ui_flags'] = frame.native_ui_flags
            value['player'] = {'hero': 'Jett', 'position_cm': frame.position, 'rotation': frame.rotation, 'health': 100}
            value["equipment_mode"] = frame.equipment_mode
            value["skills"] = {
                "smoke": {"key": "C", "cooldown": frame.smoke_cooldown,
                          "casts": frame.smoke_casts, "active": frame.smoke_count},
                "updraft": {"key": "Q", "cooldown": frame.updraft_cooldown, "casts": frame.updraft_casts},
                "dash": {"key": "E", "cooldown": frame.dash_cooldown, "casts": frame.dash_casts},
                "ultimate": {"key": "X", "active": frame.equipment_mode == 1,
                             "casts": frame.ultimate_casts, "knives": self.session.knives.ammo if self.session else 5},
                "blocked": bool(frame.skill_flags & SKILL_BLOCKED),
                "inside_smoke": bool(frame.skill_flags & SKILL_INSIDE_SMOKE)}
        value.update(extra)
        atomic_json(self.run_dir / "status.json", value)
        return value

    def trace(self, origin, direction, distance=100000.):
        self.trace_count += 1
        if self.phase == "validating_routes":
            try:
                if self.handle_escape(self.channel.read_frame()):
                    raise StopRequested()
            except ChannelNotReady:
                pass
        if self.phase == "validating_routes" and self.trace_count % 10 == 0:
            self.status()
        return self.channel.trace(origin, direction, distance, alive=self.alive)

    def trace_target(self, origin, direction, at):
        from local_training_rules import RayHit
        self.target_trace_count += 1
        result = self.channel.trace_target(origin, direction, alive=self.alive)
        if not result.hit:
            return None
        return RayHit(result.bot_id, ("body", "head", "legs")[result.region],
                      result.distance_cm, result.impact)

    def initialize(self, floor_path: Path):
        deadline = time.monotonic()+20.
        frame = None
        while self.alive() and time.monotonic() < deadline:
            try:
                frame = self.channel.read_frame()
                break
            except ChannelNotReady:
                time.sleep(.02)
        if frame is None:
            raise ChannelError("A native camera frame did not become available")
        self._last_frame = frame
        if self.handle_escape(frame):
            raise StopRequested()
        self.phase = "validating_routes"
        self.status()
        floor = FloorEvidence(floor_path)
        planner = LanePlanner(floor, self.trace)
        lanes = planner.plan(frame)
        atomic_json(self.run_dir / "lanes.json", {
            "map_package": DUEL1_MAP, "floor_source": floor.path, "floor_sha256": floor.sha256,
            "floor_meshes": floor.source_meshes, "camera_at_validation": asdict(frame),
            "candidates": planner.records, "validated_lanes": [asdict(lane) for lane in lanes],
            "limits": "Native ground, headroom, body-path and visibility traces; not an original navigation-mesh certification."})
        if not lanes:
            raise ChannelError("No candidate lane passed original-scene ground/path/visibility checks; inspect lanes.json")
        self.session = LocalTrainingSession(lanes, clock=lambda: self.sim_time, rng=random.Random(self.seed),
                                            target_trace=self.trace_target)
        self._last_frame = self.channel.read_frame()
        self.phase = "running"
        self.status(lane_count=len(lanes))

    def _record(self, events):
        self.events.extend(asdict(event) for event in events)
        self.total_events += len(events)
        if len(self.events) > 10000:
            del self.events[:-10000]

    def step(self, frame: NativeFrame):
        if self.handle_escape(frame):
            return
        if self.session is None:
            raise RuntimeError("Host routes are not initialized")
        if self._last_frame and frame.sequence == self._last_frame.sequence:
            return
        if self._last_frame and frame.time < self._last_frame.time:
            raise ChannelError("Native frame clock moved backwards")
        dt = 0. if self._last_frame is None else frame.time-self._last_frame.time
        focused = bool(frame.input_flags & INPUT_FOCUS)
        tab_down = bool(frame.input_flags & INPUT_TAB)
        if focused and tab_down and not self._tab_down:
            self.paused = not self.paused
        self._tab_down = tab_down
        active = focused and not self.paused and not self.settings_open
        # Never manufacture a long backlog of shots after a stalled host frame.
        # Inactive frames keep exactly the prior simulation timestamp and release
        # the trigger below. Therefore the weapon has no elapsed held interval to
        # backfill on focus loss / Tab pause, even if native wall time advances.
        if active:
            self.sim_time += min(dt, .10)
        direction = aim_direction(frame.rotation)
        movement = 0.
        if self._last_frame and 0 < dt <= .25:
            movement = min(1000., math.dist(frame.position, self._last_frame.position)/dt)
        # Preserve alt-fire even if the host missed the native one-frame edge.
        right_pressed = bool(frame.input_flags & INPUT_ADS) and not bool(
            self._last_frame and self._last_frame.input_flags & INPUT_ADS)
        self._last_frame = frame
        events = self.session.step(active and bool(frame.input_flags & INPUT_FIRE), frame.position, direction,
            now=self.sim_time, reload_requested=active and bool(frame.input_flags & INPUT_RELOAD),
            ads=bool(frame.input_flags & INPUT_ADS), movement_speed_cm_s=movement,
            equipment_mode=frame.equipment_mode, firing_blocked=not active or bool(frame.skill_flags & SKILL_BLOCKED),
            burst_requested=active and (bool(frame.skill_flags & SKILL_BURST) or right_pressed),
            scene_trace=lambda origin, ray, at: self.trace(origin, ray).distance_cm,
            target_trace=self.trace_target)
        self._record(events)
        for event in events:
            if event.kind == "shot":
                self._last_shot_at = self.sim_time
            elif event.kind == "bot_hit":
                self._hit_until = self.sim_time+.15
        self.publish_render(active)
        now = time.monotonic()
        if now-self._last_status_write >= .05:
            self.status()
            self._last_status_write = now
        if now-self._last_event_write >= .5:
            self.flush_events()
            self._last_event_write = now

    def publish_render(self, active=True):
        snapshot = self.session.snapshot()
        bot = self.session.drill.bot
        flags = 0
        if bot is not None and active:
            flags |= RENDER_BOT
        if snapshot["reload_ends_at"] is not None:
            flags |= RENDER_RELOADING
        if self.sim_time < self._hit_until:
            flags |= RENDER_HIT
        position = bot.position_cm if bot else (0., 0., 0.)
        player = self._last_frame.position
        yaw = math.degrees(math.atan2(player[1]-position[1], player[0]-position[0])) if bot else 0.
        kick = math.exp(-(self.sim_time-self._last_shot_at)/.08) if active else 0.
        self.channel.write_render(RenderState(flags, position, yaw,
            snapshot["recoil_pitch_deg"], snapshot["recoil_yaw_deg"], kick,
            snapshot["shots"], snapshot["ammo"], snapshot["kills"], snapshot["hits"],
            bot.bot_id if bot else 0, bot.health if bot else 0.))

    def flush_events(self):
        atomic_json(self.run_dir / "events.json", {"total_events": self.total_events,
                    "retained_events": len(self.events), "older_events_truncated": self.total_events > len(self.events),
                    "events": self.events})

    def run(self, stop_after=None):
        deadline = time.monotonic()+stop_after if stop_after else math.inf
        last_frame_seen = time.monotonic()
        while not self.stop_requested and self.alive() and time.monotonic() < deadline:
            try:
                frame = self.channel.read_frame()
                if not self._last_frame or frame.sequence != self._last_frame.sequence:
                    self.step(frame)
                    last_frame_seen = time.monotonic()
                    if self.stop_requested:
                        break
                elif time.monotonic()-last_frame_seen > 5:
                    raise ChannelError("Native game frames stopped arriving")
            except ChannelNotReady:
                if time.monotonic()-last_frame_seen > 5:
                    raise ChannelError("Native game frame publication is unavailable")
            time.sleep(.004)
        if self.stop_requested:
            self.phase = "stopping"
            self.status(stop_reason="user_escape")
        else:
            self.phase = "stopped"
            self.status(stop_reason="game_exited" if not self.alive() else "host_deadline")
        self.flush_events()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pid", type=int, required=True)
    parser.add_argument("--run-dir", type=Path, required=True)
    parser.add_argument("--scene", type=Path, default=ROOT / "work/trainer/scenes/duel-1.json")
    parser.add_argument("--seed", type=int, default=20261101)
    parser.add_argument("--no-hud", action="store_true")
    parser.add_argument("--stop-after", type=float)
    args = parser.parse_args()
    if args.pid <= 0 or (args.stop_after is not None and args.stop_after <= 0):
        parser.error("PID and optional duration must be positive")
    run_dir = args.run_dir.resolve()
    run_dir.mkdir(parents=True, exist_ok=True)
    process = channel = host = hud = hud_log = None
    try:
        process = ProcessWatch(args.pid)
        atomic_json(run_dir / "status.json", {"phase": "waiting_for_native", "pid": args.pid, "updated_at": time.time()})
        channel = TrainingChannel.open(args.pid, alive=process.alive)
        host = TrainingHost(channel, run_dir, alive=process.alive, seed=args.seed, ui_enabled=not args.no_hud)
        if not args.no_hud:
            hud_log = (run_dir / "hud.log").open("w", encoding="utf-8")
            hud = subprocess.Popen([sys.executable, str(ROOT / "tools/training_hud.py"),
                "--pid", str(args.pid), "--status", str(run_dir / "status.json")],
                stdout=hud_log, stderr=hud_log, creationflags=subprocess.CREATE_NO_WINDOW)
        host.initialize(args.scene.resolve(strict=True))
        host.run(args.stop_after)
        return 0
    except StopRequested:
        if host:
            host.flush_events()
        return 0
    except KeyboardInterrupt:
        if host:
            host.phase = "stopped"
            host.status(stop_reason="host_interrupted")
            host.flush_events()
        return 0
    except Exception as error:
        failure = {"phase": "error", "pid": args.pid, "updated_at": time.time(),
                   "error": str(error) if isinstance(error, (ChannelError, ValueError)) else type(error).__name__,
                   "original_server_equivalence_proven": False}
        atomic_json(run_dir / "status.json", failure)
        if host:
            host.flush_events()
        print(json.dumps(failure), flush=True)
        return 2
    finally:
        if channel:
            try:
                channel.write_render(RenderState(flags=0))
            except (ChannelError, ValueError):
                pass
            channel.close()
        if hud and hud.poll() is None:
            hud.terminate()
            try:
                hud.wait(timeout=3)
            except subprocess.TimeoutExpired:
                hud.kill()
                hud.wait(timeout=3)
        if hud_log:
            hud_log.close()
        if process:
            process.close()


if __name__ == "__main__":
    raise SystemExit(main())
