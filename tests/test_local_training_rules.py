"""Meaningful timing, occlusion, deterministic hit and drill lifecycle checks."""
from dataclasses import asdict, replace
import math
from pathlib import Path
import random
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from local_training_rules import (BotConfig, BotDrill, BotLane, LocalTrainingSession,
                                  LocalWeapon, RayHit, Shot, TargetSphere, WeaponConfig,
                                  nearest_target_hit, ray_sphere)


class Clock:
    def __init__(self):
        self.now = 0.
    def __call__(self):
        return self.now


def exact_weapon(**kwargs):
    return replace(WeaponConfig(), base_spread_deg=0., moving_spread_deg=0.,
                   bloom_per_shot_deg=0., recoil_pitch_per_shot_deg=0.,
                   recoil_yaw_per_shot_deg=0., **kwargs)


def lanes():
    # Synthetic test geometry; these coordinates are not asserted to be walkable
    # in the actual game. Production lanes must come from the scene integration.
    return [BotLane("left", (1000., 0., 0.), (1000., 300., 0.), validated=True),
            BotLane("right", (1200., 0., 0.), (1200., -300., 0.), validated=True)]


class WeaponRulesTests(unittest.TestCase):
    def test_single_tap_does_not_backfill_idle_time(self):
        gun = LocalWeapon(exact_weapon(), clock=Clock(), rng=random.Random(1))
        self.assertEqual(len(gun.step(True, (0, 0, 0), (1, 0, 0), now=10).shots), 1)
        gun.step(False, (0, 0, 0), (1, 0, 0), now=10.01)
        self.assertEqual(gun.ammo, 24)
        self.assertEqual(len(gun.step(True, (0, 0, 0), (1, 0, 0), now=20).shots), 1)
        self.assertEqual(gun.ammo, 23)

    def test_held_fire_uses_exact_rate_and_is_poll_frequency_independent(self):
        coarse = LocalWeapon(exact_weapon(), clock=Clock(), rng=random.Random(7))
        fine = LocalWeapon(exact_weapon(), clock=Clock(), rng=random.Random(7))
        a = list(coarse.step(True, (0, 0, 0), (1, 0, 0), now=0).shots)
        a.extend(coarse.step(True, (0, 0, 0), (1, 0, 0), now=1).shots)
        b = []
        for i in range(101):
            b.extend(fine.step(True, (0, 0, 0), (1, 0, 0), now=i/100).shots)
        self.assertEqual(len(a), 10)
        self.assertEqual([asdict(s) for s in a], [asdict(s) for s in b])
        for left, right in zip(a, a[1:]):
            self.assertAlmostEqual(right.at-left.at, 1/9.75)

    def test_release_at_shot_deadline_prevents_that_shot(self):
        gun = LocalWeapon(exact_weapon(shots_per_second=10), clock=Clock())
        gun.step(True, (0, 0, 0), (1, 0, 0), now=0)
        frame = gun.step(False, (0, 0, 0), (1, 0, 0), now=.1)
        self.assertEqual(frame.shots, ())
        self.assertEqual(gun.ammo, 24)

    def test_semi_automatic_requires_release_and_press(self):
        gun = LocalWeapon(exact_weapon(automatic=False), clock=Clock())
        gun.step(True, (0, 0, 0), (1, 0, 0), now=0)
        self.assertFalse(gun.step(True, (0, 0, 0), (1, 0, 0), now=1).shots)
        gun.step(False, (0, 0, 0), (1, 0, 0), now=1.01)
        self.assertEqual(len(gun.step(True, (0, 0, 0), (1, 0, 0), now=1.02).shots), 1)

    def test_empty_magazine_never_has_negative_ammo_or_extra_bullets(self):
        gun = LocalWeapon(exact_weapon(), clock=Clock())
        first = gun.step(True, (0, 0, 0), (1, 0, 0), now=0)
        rest = gun.step(True, (0, 0, 0), (1, 0, 0), now=100)
        self.assertEqual(len(first.shots)+len(rest.shots), 25)
        self.assertEqual(gun.ammo, 0)
        self.assertEqual([e.kind for e in rest.events].count("dry_fire"), 1)
        self.assertFalse(gun.step(True, (0, 0, 0), (1, 0, 0), now=101).events)

    def test_reload_blocks_fire_and_held_reload_does_not_extend_timer(self):
        gun = LocalWeapon(exact_weapon(magazine_size=2), clock=Clock())
        gun.step(True, (0, 0, 0), (1, 0, 0), now=0)
        frame = gun.step(False, (0, 0, 0), (1, 0, 0), now=.01, reload_requested=True)
        self.assertEqual(frame.events[0].kind, "reload_started")
        self.assertAlmostEqual(gun.reload_ends_at, 2.51)
        self.assertFalse(gun.step(True, (0, 0, 0), (1, 0, 0), now=2.50, reload_requested=True).shots)
        frame = gun.step(True, (0, 0, 0), (1, 0, 0), now=2.51, reload_requested=True)
        self.assertEqual(len(frame.shots), 1)
        self.assertEqual(gun.ammo, 1)
        self.assertEqual([e.kind for e in frame.events], ["reload_completed", "shot"])

    def test_finite_reserve_and_no_reload_of_full_magazine(self):
        gun = LocalWeapon(exact_weapon(magazine_size=3, reserve_ammo=1), clock=Clock())
        self.assertFalse(gun.step(False, (0, 0, 0), (1, 0, 0), now=0, reload_requested=True).events)
        gun.step(True, (0, 0, 0), (1, 0, 0), now=.01)
        gun.step(False, (0, 0, 0), (1, 0, 0), now=.25)
        self.assertEqual(gun.ammo, 0)
        gun.step(False, (0, 0, 0), (1, 0, 0), now=.3, reload_requested=True)
        gun.step(False, (0, 0, 0), (1, 0, 0), now=2.8)
        self.assertEqual((gun.ammo, gun.reserve), (1, 0))

    def test_seeded_recoil_spread_and_recovery_are_deterministic(self):
        a = LocalWeapon(clock=Clock(), rng=random.Random(23))
        b = LocalWeapon(clock=Clock(), rng=random.Random(23))
        for at in (0, .2, .4):
            self.assertEqual(a.step(True, (0, 0, 0), (1, 0, 0), now=at),
                             b.step(True, (0, 0, 0), (1, 0, 0), now=at))
        self.assertGreater(a.recoil_pitch_deg, 0)
        pitch = a.recoil_pitch_deg
        a.step(False, (0, 0, 0), (1, 0, 0), now=.41)
        a.step(False, (0, 0, 0), (1, 0, 0), now=3)
        self.assertLess(a.recoil_pitch_deg, pitch*.001)
        self.assertLess(a.bloom_deg, .001)

    def test_movement_increases_spread_and_ads_scales_it(self):
        frames = []
        for speed, ads in ((0, False), (450, False), (450, True)):
            gun = LocalWeapon(clock=Clock(), rng=random.Random(3))
            frames.append(gun.step(True, (0, 0, 0), (1, 0, 0), now=0,
                                   movement_speed_cm_s=speed, ads=ads).shots[0])
        self.assertGreater(frames[1].spread_deg, frames[0].spread_deg)
        self.assertAlmostEqual(frames[2].spread_deg, frames[1].spread_deg*.75)
        self.assertAlmostEqual(sum(v*v for v in frames[2].direction), 1.)


class HitRulesTests(unittest.TestCase):
    def test_nearest_target_and_wall_occlusion_in_world_units(self):
        targets = [TargetSphere(2, "body", (200, 0, 0), 10),
                   TargetSphere(1, "head", (100, 0, 0), 10)]
        hit = nearest_target_hit((0, 0, 0), (5, 0, 0), targets, blocking_distance_cm=150)
        self.assertEqual(hit.target_id, 1)
        self.assertAlmostEqual(hit.distance_cm, 90)
        for wall in (80, 90, 90.05):
            self.assertIsNone(nearest_target_hit((0, 0, 0), (1, 0, 0), targets, blocking_distance_cm=wall))

    def test_inside_tangent_and_behind_spheres(self):
        self.assertEqual(ray_sphere((0, 0, 0), (1, 0, 0), (0, 0, 0), 10), 0)
        self.assertAlmostEqual(ray_sphere((0, 0, 0), (1, 0, 0), (10, 1, 0), 1), 10)
        self.assertIsNone(ray_sphere((0, 0, 0), (1, 0, 0), (-20, 0, 0), 2))
        with self.assertRaises(ValueError):
            ray_sphere((0, 0, 0), (0, 0, 0), (20, 0, 0), 2)
        with self.assertRaises(ValueError):
            nearest_target_hit((0, 0, 0), (1, 0, 0), [], blocking_distance_cm=float("nan"))


class BotAndSessionTests(unittest.TestCase):
    def test_unvalidated_or_other_map_lanes_are_rejected(self):
        with self.assertRaises(ValueError):
            BotDrill([BotLane("guess", (0, 0, 0), (100, 0, 0))], clock=Clock())
        with self.assertRaises(ValueError):
            BotLane("wrong", (0, 0, 0), (100, 0, 0), True, "/Game/Other")
        with self.assertRaises(ValueError):
            BotDrill([], clock=Clock())

    def test_peek_moves_at_configured_speed_then_escapes(self):
        drill = BotDrill(lanes()[:1], BotConfig(speed_cm_s=100), clock=Clock(), rng=random.Random(1))
        self.assertEqual(drill.advance(0)[0].kind, "bot_spawned")
        drill.advance(1)
        self.assertEqual(drill.bot.position_cm, (1000, 100, 0))
        events = drill.advance(3.4)
        self.assertEqual([e.kind for e in events], ["bot_escaped"])
        self.assertIsNone(drill.bot)
        self.assertEqual(drill.missed, 1)

    def test_head_kill_schedules_random_next_lane_without_reusing_dead_target(self):
        drill = BotDrill(lanes(), clock=Clock(), rng=random.Random(10))
        drill.advance(0)
        initial = drill.bot
        shot = Shot(1, 0, (0, 0, 165), (1, 0, 0), 24, 0, 0, 0)
        events = drill.resolve_shot(shot, None, use_fixture_geometry=True)
        self.assertEqual([e.kind for e in events], ["bot_hit", "bot_killed"])
        self.assertEqual(events[0].data["region"], "head")
        self.assertEqual(events[0].data["health_after"], 0)
        self.assertIsNone(drill.bot)
        self.assertEqual(drill.hit_spheres(), ())
        self.assertGreaterEqual(drill.next_spawn_at, .6)
        self.assertLessEqual(drill.next_spawn_at, 1.2)
        drill.advance(drill.next_spawn_at)
        self.assertNotEqual(drill.bot.lane_name, initial.lane_name)
        self.assertEqual(drill.bot.health, 150)
        self.assertEqual(drill.bot.bot_id, 2)

    def test_body_hit_damages_without_killing(self):
        drill = BotDrill(lanes()[:1], clock=Clock())
        drill.advance(0)
        shot = Shot(1, 0, (0, 0, 110), (1, 0, 0), 24, 0, 0, 0)
        events = drill.resolve_shot(shot, None, use_fixture_geometry=True)
        self.assertEqual([e.kind for e in events], ["bot_hit"])
        self.assertEqual(drill.bot.health, 110)

    def test_session_requires_scene_trace_and_does_not_damage_through_wall(self):
        session = LocalTrainingSession(lanes(), weapon_config=exact_weapon(), clock=Clock(), rng=random.Random(5), use_fixture_geometry=True)
        with self.assertRaises(ValueError):
            session.step(True, (0, 0, 165), (1, 0, 0), now=0, scene_trace=None)
        traces = []
        def scene_trace(origin, direction, at):
            traces.append((origin, direction, at))
            return 100.
        events = session.step(True, (0, 0, 165), (1, 0, 0), now=0, scene_trace=scene_trace)
        self.assertEqual(len(traces), 1)
        self.assertIn("shot_missed", [e.kind for e in events])
        self.assertEqual(session.snapshot()["hits"], 0)
        self.assertEqual(session.drill.bot.health, 150)

    def test_scene_trace_receives_each_spread_ray_and_timeline_is_sorted(self):
        session = LocalTrainingSession(lanes(), clock=Clock(), rng=random.Random(5), use_fixture_geometry=True)
        traced = []
        def trace(origin, direction, at):
            traced.append((direction, at))
            return None
        first = session.step(True, (0, 0, 165), (1, 0, 0), now=0, scene_trace=trace)
        second = session.step(True, (0, 0, 165), (1, 0, 0), now=.3, scene_trace=trace)
        shot_events = [e for e in first+second if e.kind == "shot"]
        self.assertEqual(len(traced), 3)
        self.assertEqual([r[0] for r in traced], [e.data["direction"] for e in shot_events])
        self.assertEqual([e.at for e in second], sorted(e.at for e in second))
        self.assertFalse(session.snapshot()["original_server_equivalence_proven"])

    def test_monotonic_time_and_seeded_session_replay(self):
        def run():
            session = LocalTrainingSession(lanes(), weapon_config=exact_weapon(), clock=Clock(), rng=random.Random(99), use_fixture_geometry=True)
            events = []
            for i in range(101):
                events.extend(session.step(i % 20 < 10, (0, 0, 165), (1, 0, 0), now=i*.02,
                                           scene_trace=lambda *_: None))
            return [asdict(e) for e in events], session.snapshot(), session
        a, sa, instance = run()
        b, sb, _ = run()
        self.assertEqual((a, sa), (b, sb))
        with self.assertRaises(ValueError):
            instance.step(False, (0, 0, 165), (1, 0, 0), now=1, scene_trace=lambda *_: None)


class NativeGeometryRulesTests(unittest.TestCase):
    def session(self, **kwargs):
        return LocalTrainingSession([BotLane("original-shape-fixture", (1000, 0, 0), (1000, 0, 0), True)],
            clock=Clock(), weapon_config=exact_weapon(), bot_config=BotConfig(hold_at_end_seconds=30), **kwargs)

    def test_missing_native_target_callback_cannot_silently_use_spheres(self):
        s = self.session()
        with self.assertRaisesRegex(ValueError, "native target trace"):
            s.step(True, (0, 0, 165), (1, 0, 0), now=0, scene_trace=lambda *_: math.inf)
        self.assertEqual(s.rifle.ammo, 25)

    def test_original_head_hit_above_old_sphere_uses_native_impact_and_region(self):
        s = self.session()
        s.drill.hit_spheres = lambda: self.fail("Production must never consult fixture hit spheres")
        calls = []
        def world(*args):
            calls.append(("world", args))
            return math.inf
        def target(*args):
            calls.append(("target", args))
            return RayHit(1, "head", 990, (990, 0, 190))
        events = s.step(True, (0, 0, 190), (1, 0, 0), now=0, scene_trace=world, target_trace=target)
        self.assertEqual([c[0] for c in calls], ["world", "target"])
        self.assertEqual(calls[0][1], calls[1][1])
        hit = next(e for e in events if e.kind == "bot_hit")
        self.assertEqual((hit.data["position"], hit.data["distance_cm"], hit.data["region"]), ((990, 0, 190), 990, "head"))
        self.assertEqual(s.drill.kills, 1)

    def test_native_miss_does_not_hit_the_legacy_head_sphere(self):
        s = self.session(target_trace=lambda *_: None)
        events = s.step(True, (0, 0, 165), (1, 0, 0), now=0, scene_trace=lambda *_: math.inf)
        self.assertIn("shot_missed", [e.kind for e in events])
        self.assertEqual((s.hits, s.drill.bot.health), (0, 150))

    def test_world_occlusion_ties_and_stale_target_identity_reject_damage(self):
        for wall, native_id in ((500, 1), (990, 1), (990.05, 1), (math.inf, 7)):
            with self.subTest(wall=wall, native_id=native_id):
                s = self.session(target_trace=lambda *_: RayHit(native_id, "head", 990, (990, 0, 190)))
                s.step(True, (0, 0, 190), (1, 0, 0), now=0, scene_trace=lambda *_: wall)
                self.assertEqual((s.hits, s.drill.bot.health), (0, 150))

    def test_each_native_region_applies_only_its_local_damage(self):
        for region, expected in (("head", 150), ("body", 40), ("legs", 34)):
            with self.subTest(region=region):
                s = self.session(target_trace=lambda *_: RayHit(1, region, 990, (990, 0, 190)))
                events = s.step(True, (0, 0, 190), (1, 0, 0), now=0, scene_trace=lambda *_: math.inf)
                self.assertEqual(next(e.data["damage"] for e in events if e.kind == "bot_hit"), expected)

    def test_old_native_id_cannot_damage_respawned_target(self):
        s = self.session(target_trace=lambda *_: RayHit(1, "head", 990, (990, 0, 190)))
        s.step(True, (0, 0, 190), (1, 0, 0), now=0, scene_trace=lambda *_: math.inf)
        s.step(False, (0, 0, 190), (1, 0, 0), now=.01, scene_trace=lambda *_: math.inf)
        at = s.drill.next_spawn_at
        s.step(True, (0, 0, 190), (1, 0, 0), now=at, scene_trace=lambda *_: math.inf)
        self.assertEqual((s.drill.bot.bot_id, s.drill.bot.health, s.drill.kills), (2, 150, 1))

    def test_rifle_and_knife_burst_query_every_actual_ray_in_order(self):
        calls = []
        s = self.session(target_trace=lambda *args: calls.append(("target", args)))
        world = lambda *args: calls.append(("world", args)) or math.inf
        s.step(True, (0, 0, 190), (1, 0, 0), now=0, scene_trace=world)
        s.step(False, (0, 0, 190), (1, 0, 0), now=.01, scene_trace=world, equipment_mode=1)
        s.step(False, (0, 0, 190), (1, 0, 0), now=.5, scene_trace=world, burst_requested=True)
        self.assertEqual(len(calls), 12)  # One rifle round and five knives, two queries each.
        for i in range(0, len(calls), 2):
            self.assertEqual((calls[i][0], calls[i+1][0]), ("world", "target"))
            self.assertEqual(calls[i][1], calls[i+1][1])


class JettKnifeRulesTests(unittest.TestCase):
    def session(self):
        return LocalTrainingSession([BotLane("stationary", (1000, 0, 0), (1000, 0, 0), True)],
            clock=Clock(), weapon_config=exact_weapon(),
            bot_config=BotConfig(hold_at_end_seconds=30), rng=random.Random(41), use_fixture_geometry=True)

    def step(self, session, at, fire=False, **kwargs):
        return session.step(fire, kwargs.pop("origin", (0, 0, 165)), kwargs.pop("direction", (0, 1, 0)),
            now=at, scene_trace=kwargs.pop("scene_trace", lambda *_: math.inf), **kwargs)

    def test_switch_preserves_rifle_ammo_and_monotonic_shot_ids(self):
        s = self.session()
        events = self.step(s, 0, True)
        self.assertEqual(s.rifle.ammo, 24)
        self.step(s, .01, equipment_mode=1)
        events += self.step(s, .31, True)
        self.assertEqual((s.weapon.ammo, s.rifle.ammo), (4, 24))
        self.step(s, .5, equipment_mode=0)
        self.assertIs(s.weapon, s.rifle)
        self.assertEqual(s.weapon.ammo, 24)
        events += self.step(s, .61, True)
        self.step(s, .62, equipment_mode=1)
        self.assertEqual((s.knives.ammo, s.rifle.ammo), (5, 23))
        self.assertEqual([e.data["shot_id"] for e in events if e.kind == "shot"], [1, 2, 3])
        self.assertEqual(s.snapshot()["shots"], 3)

    def test_five_knives_exhaust_without_reload_or_spread_while_moving(self):
        s = self.session()
        rays = []
        def trace(origin, direction, at):
            rays.append(direction)
            return math.inf
        events = self.step(s, 0, True, equipment_mode=1, movement_speed_cm_s=1000, scene_trace=trace)
        events += self.step(s, 2, True, movement_speed_cm_s=1000, ads=True, scene_trace=trace)
        events += self.step(s, 3, True, reload_requested=True, scene_trace=trace)
        self.assertEqual((s.knives.ammo, s.snapshot()["shots"], len(rays)), (0, 5, 5))
        self.assertTrue(all(ray == (0, 1, 0) for ray in rays))
        self.assertEqual(s.weapon.recoil_pitch_deg, 0)
        self.assertFalse(any(e.kind.startswith("reload_") for e in events))

    def test_exact_body_region_damage_and_head_kill_refill(self):
        for height, expected in ((165, 150), (110, 50), (45, 42.5)):
            with self.subTest(height=height):
                s = self.session()
                events = self.step(s, 0, True, equipment_mode=1, origin=(0, 0, height), direction=(1, 0, 0))
                hit = next(e for e in events if e.kind == "bot_hit")
                self.assertEqual(hit.data["damage"], expected)
                self.assertEqual(s.knives.ammo, 5 if height == 165 else 4)
                self.assertEqual(s.drill.kills, int(height == 165))

    def test_third_body_hit_kills_and_restores_five(self):
        s = self.session()
        for at in (0, .3, .6):
            events = self.step(s, at, True, equipment_mode=1, origin=(0, 0, 110), direction=(1, 0, 0))
        self.assertEqual((s.drill.kills, s.knives.ammo, s.snapshot()["shots"]), (1, 5, 3))
        self.assertTrue(any(e.kind == "knives_refilled" for e in events))

    def test_blocked_cancels_prior_held_interval_without_catchup(self):
        for mode in (0, 1):
            with self.subTest(mode=mode):
                s = self.session()
                self.step(s, 0, True, equipment_mode=mode)
                self.step(s, 1, True, firing_blocked=True, reload_requested=True, burst_requested=True)
                self.assertEqual(s.snapshot()["shots"], 1)
                self.assertIsNone(s.weapon.reload_ends_at)
                self.step(s, 1.01, True)
                self.assertEqual(s.snapshot()["shots"], 2)

    def test_right_click_spends_only_remaining_knives_and_traces_every_ray(self):
        s = self.session()
        rays = []
        trace = lambda *args: rays.append(args) or math.inf
        self.step(s, 0, True, equipment_mode=1, scene_trace=trace)
        self.step(s, .3, True, scene_trace=trace)
        self.step(s, .31, scene_trace=trace)
        events = self.step(s, .7, burst_requested=True, scene_trace=trace)
        shots = [e for e in events if e.kind == "shot"]
        self.assertEqual((len(shots), s.knives.ammo, len(rays)), (3, 0, 5))
        self.assertTrue(all(e.data["burst"] and e.at == .7 for e in shots))
        self.step(s, 1.1, burst_requested=True, scene_trace=trace)
        self.assertEqual(len(rays), 5)

    def test_knives_still_obey_original_scene_wall_trace(self):
        s = self.session()
        self.step(s, 0, True, equipment_mode=1, direction=(1, 0, 0), scene_trace=lambda *_: 500)
        self.assertEqual((s.hits, s.drill.kills, s.knives.ammo), (0, 0, 4))


if __name__ == "__main__":
    unittest.main()
