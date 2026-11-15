"""Native-route proof, real-input gating and publication tests with explicit fixtures."""
from dataclasses import replace
import json
import math
from pathlib import Path
import random
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from local_training_rules import BotLane, LocalTrainingSession, WeaponConfig
from training_channel import NativeFrame, TargetTraceResult, TraceResult
from training_host import FloorEvidence, LanePlanner, TrainingHost, aim_direction, atomic_json


class FlatTestFloor:
    def height(self, x, y):
        return 200. if -2208 <= x <= 2208 and -1230 <= y <= 1230 else None


def flat_scene_trace(origin, direction, maximum):
    if direction[2] < -.9:
        distance = origin[2]-200.
        return TraceResult(True, distance, (origin[0], origin[1], 200.))
    return TraceResult(False, math.inf, (0, 0, 0))


class FakeChannel:
    pid = 1234
    def __init__(self):
        self.renders = []
        self.traces = []
        self.target_traces = []
    def trace(self, origin, direction, distance, **kwargs):
        self.traces.append((origin, direction, distance))
        return TraceResult(False, math.inf, (0, 0, 0))
    def trace_target(self, origin, direction, distance=100000., **kwargs):
        self.target_traces.append((origin, direction, distance))
        return TargetTraceResult(False, math.inf, (0, 0, 0), 0, 0)
    def write_render(self, state):
        self.renders.append(state)


def frame(sequence, at, flags=8):
    return NativeFrame(sequence, at, (0., 0., 165.), (0., 0., 0.), flags)


class HostTests(unittest.TestCase):
    def test_original_floor_transform_is_inverted_exactly(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp)/"scene.json"
            p.write_text(json.dumps({"units": "m", "meshes": [{"name": "Actor/Shell_0_WarehouseFloorA/material0",
                "vertices": [[0, 0, 2], [10, 0, 2], [0, 10, 2]], "indices": [0, 1, 2],
                "source_mesh": "fixture", "source_export": 0}]}), encoding="utf-8")
            floor = FloorEvidence(p)
            self.assertEqual(floor.height(100, -100), 200.)
            self.assertIsNone(floor.height(100, 100))

    def test_route_validation_requires_native_floor_and_body_path(self):
        camera = NativeFrame(2, 10, (-2050., 0., 300.), (0., 0., 0.), 8)
        planner = LanePlanner(FlatTestFloor(), flat_scene_trace, max_lanes=2)
        lanes = planner.plan(camera)
        self.assertEqual(len(lanes), 2)
        self.assertTrue(all(l.validated for l in lanes))
        self.assertTrue(all(l.start_cm[2] == 201. for l in lanes))
        no_ground = LanePlanner(FlatTestFloor(), lambda *_: TraceResult(False, math.inf, (0, 0, 0)))
        self.assertEqual(no_ground.plan(camera), [])
        def blocked(origin, direction, maximum):
            if abs(direction[2]) < .9:
                return TraceResult(True, min(10., maximum), (origin[0]+10, origin[1], origin[2]))
            return flat_scene_trace(origin, direction, maximum)
        self.assertEqual(LanePlanner(FlatTestFloor(), blocked).plan(camera), [])

    def test_camera_direction_uses_pitch_yaw_degrees(self):
        self.assertAlmostEqual(aim_direction((0, 90, 0))[1], 1)
        self.assertAlmostEqual(aim_direction((90, 0, 0))[2], 1)

    def test_focus_tab_pause_and_shot_tracing(self):
        with tempfile.TemporaryDirectory() as tmp:
            channel = FakeChannel()
            host = TrainingHost(channel, Path(tmp))
            config = replace(WeaponConfig(), base_spread_deg=0, moving_spread_deg=0,
                             recoil_pitch_per_shot_deg=0, recoil_yaw_per_shot_deg=0, bloom_per_shot_deg=0)
            host.session = LocalTrainingSession([BotLane("fixture", (1000, 0, 0), (1000, 200, 0), True)],
                clock=lambda: host.sim_time, rng=random.Random(1), weapon_config=config)
            host._last_frame = frame(2, 10)
            host.phase = "running"
            host.step(frame(4, 10.02, 9))
            self.assertEqual(host.session.weapon.shots_fired, 1)
            self.assertEqual(len(channel.traces), 1)
            host.step(frame(6, 10.04, 25))  # focus + fire + Tab
            self.assertTrue(host.paused)
            paused_time = host.sim_time
            host.step(frame(8, 10.06, 25))
            self.assertTrue(host.paused)  # Held Tab does not toggle repeatedly.
            host.step(frame(10, 10.08, 9))
            host.step(frame(12, 10.10, 25))
            self.assertFalse(host.paused)
            self.assertGreater(host.sim_time, paused_time)
            before = host.sim_time
            host.step(frame(14, 10.12, 1))  # Fire without focus.
            self.assertEqual(host.sim_time, before)
            self.assertFalse(host.session.weapon.trigger_held)
            self.assertEqual(channel.renders[-1].flags & 1, 0)

    def test_production_host_consumes_native_hitbox_result_without_fixture_geometry(self):
        with tempfile.TemporaryDirectory() as tmp:
            channel = FakeChannel()
            ordering = []
            def scene(origin, direction, distance, **kwargs):
                ordering.append(("world", origin, direction))
                return TraceResult(False, math.inf, (0, 0, 0))
            def target(origin, direction, **kwargs):
                ordering.append(("target", origin, direction))
                return TargetTraceResult(True, 985, (985, 0, 190), 1, 1)
            channel.trace, channel.trace_target = scene, target
            host = TrainingHost(channel, Path(tmp))
            host.session = LocalTrainingSession([BotLane("fixture", (1000, 0, 0), (1000, 0, 0), True)],
                clock=lambda: host.sim_time)
            host.session.drill.hit_spheres = lambda: self.fail("Production queried fabricated geometry")
            host._last_frame = replace(frame(2, 10), position=(0, 0, 190))
            host.phase = "running"
            host.step(replace(frame(4, 10.02, 9), position=(0, 0, 190)))
            self.assertEqual((host.session.hits, host.session.drill.kills, host.target_trace_count), (1, 1, 1))
            self.assertEqual([x[0] for x in ordering], ["world", "target"])
            self.assertEqual(ordering[0][1:], ordering[1][1:])
            self.assertEqual(host.session.snapshot()["hit_geometry"], "native_physics_asset")

    def test_native_clock_reversal_and_duplicate_frames(self):
        with tempfile.TemporaryDirectory() as tmp:
            host = TrainingHost(FakeChannel(), Path(tmp))
            host.session = LocalTrainingSession([BotLane("fixture", (1000, 0, 0), (1000, 100, 0), True)],
                clock=lambda: host.sim_time)
            host._last_frame = frame(10, 20)
            host.step(frame(10, 20, 9))
            self.assertEqual(host.session.weapon.shots_fired, 0)
            with self.assertRaises(RuntimeError):
                host.step(frame(12, 19, 9))

    def test_escape_requires_game_focus_and_stops_before_shooting(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)/"training"
            channel = FakeChannel()
            host = TrainingHost(channel, directory)
            host.session = LocalTrainingSession([BotLane("fixture", (1000, 0, 0), (1000, 100, 0), True)],
                clock=lambda: host.sim_time)
            host._last_frame = frame(2, 10)
            host.phase = "running"
            host.step(frame(4, 10.02, 32))  # Escape outside the game is ignored.
            self.assertFalse(host.stop_requested)
            self.assertFalse((Path(tmp)/"stop-request.json").exists())
            host.step(frame(6, 10.04, 41))  # Focus + Escape + fire.
            self.assertTrue(host.stop_requested)
            self.assertEqual(host.phase, "stopping")
            self.assertEqual(host.session.weapon.shots_fired, 0)
            request = json.loads((Path(tmp)/"stop-request.json").read_text(encoding="utf-8"))
            self.assertEqual((request["reason"], request["pid"]), ("user_escape", 1234))
            self.assertFalse((Path(tmp)/"stop-request.json.pending").exists())
            host.step(frame(8, 11, 9))
            self.assertEqual(host.session.weapon.shots_fired, 0)
            host.run(stop_after=.1)
            status = json.loads((directory/"status.json").read_text(encoding="utf-8"))
            self.assertEqual((status["phase"], status["stop_reason"]), ("stopping", "user_escape"))

    def test_inactive_frames_do_not_backfill_a_held_trigger(self):
        for inactive_flags in (1, 25):  # Unfocused fire, or focused fire + Tab pause.
            with self.subTest(flags=inactive_flags), tempfile.TemporaryDirectory() as tmp:
                channel = FakeChannel()
                host = TrainingHost(channel, Path(tmp))
                host.session = LocalTrainingSession([BotLane("fixture", (1000, 0, 0), (1000, 100, 0), True)],
                    clock=lambda: host.sim_time, weapon_config=WeaponConfig(shots_per_second=10))
                host._last_frame = frame(2, 10)
                host.phase = "running"
                host.step(frame(4, 10.01, 9))
                shots, traces, simulated = host.session.weapon.shots_fired, len(channel.traces), host.sim_time
                # The native gap crosses many fire deadlines, but the training
                # timeline must not advance while inactive or after Tab pauses.
                host.step(frame(6, 20, inactive_flags))
                host.step(frame(8, 40, inactive_flags))
                host.step(frame(10, 60, inactive_flags))
                self.assertEqual(host.sim_time, simulated)
                self.assertEqual(host.session.weapon.shots_fired, shots)
                self.assertEqual(len(channel.traces), traces)
                self.assertFalse(host.session.weapon.trigger_held)

    def test_atomic_status_never_leaves_pending_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/"status.json"
            atomic_json(path, {"ammo": 25})
            atomic_json(path, {"ammo": 24})
            self.assertEqual(json.loads(path.read_text(encoding="utf-8")), {"ammo": 24})
            self.assertFalse(path.with_name(path.name+".pending").exists())

    def test_native_knife_mode_blocked_gate_burst_and_skills_status(self):
        with tempfile.TemporaryDirectory() as tmp:
            channel = FakeChannel()
            host = TrainingHost(channel, Path(tmp))
            host.session = LocalTrainingSession([BotLane("fixture", (1000, 1000, 0), (1000, 1100, 0), True)],
                clock=lambda: host.sim_time)
            host._last_frame = frame(2, 10)
            host.phase = "running"
            host.step(replace(frame(4, 10.01, 9), equipment_mode=1, skill_flags=1,
                smoke_count=2, smoke_cooldown=.5, updraft_casts=2, ultimate_casts=1))
            self.assertEqual((host.session.equipment_mode, len(channel.traces)), (1, 0))
            host.step(replace(frame(6, 10.02, 9), equipment_mode=1))
            self.assertEqual((channel.renders[-1].ammo, len(channel.traces)), (4, 1))
            for n in range(7, 11):
                host.step(replace(frame(n*2, 10.02+(n-6)*.1, 8), equipment_mode=1))
            host.step(replace(frame(24, 10.53, 12), equipment_mode=1, skill_flags=6,
                smoke_count=2, smoke_cooldown=.5, updraft_casts=2, ultimate_casts=1))
            self.assertEqual((channel.renders[-1].ammo, len(channel.traces)), (0, 5))
            status = host.status()
            self.assertEqual(status["equipment_mode"], 1)
            self.assertEqual(status["skills"]["smoke"]["active"], 2)
            self.assertEqual(status["skills"]["updraft"]["casts"], 2)
            self.assertTrue(status["skills"]["inside_smoke"])
            self.assertEqual(status["skills"]["ultimate"]["knives"], 0)
            host.step(frame(26, 10.54, 8))
            self.assertEqual(channel.renders[-1].ammo, 25)
            self.assertEqual(channel.renders[-1].shots, 5)


if __name__ == "__main__":
    unittest.main()
