"""Deterministic, process-independent local training rules for Duel 1.

All distances use native Unreal centimetres, Z up. These are new local rules,
not recovered server gameplay. The 25-round magazine and 9.75 shots/s defaults
use the *inferred* scalar candidates in work/gameplay/report.json (AK asset SHA
81c706428a8d2e5bcd0b9475b38c5dbb1f37ed15a820498cf4b1d62f225601d1).
Reload time, damage, recoil, spread and bot timing are design values. Production
hit geometry comes from native queries against the original PhysicsAsset.

The caller must supply a finite collection of validated Duel 1 peek lanes and
must provide scene obstruction distances for every actual shot direction. This
module does not guess navigable coordinates or read/control the game process.
"""
from __future__ import annotations

from dataclasses import asdict, dataclass, replace
import math
import random
import time
from typing import Callable, Iterable

DUEL1_MAP = "/Game/Maps/Duel/Duel_1/Skirmish_A"
Vec3 = tuple[float, float, float]
EPS = 1e-9
PARAMETER_PROVENANCE = {
    "magazine_size": "25: inferred MagazineAmmo asset scalar, offsets 21644/21648",
    "shots_per_second": "9.75: inferred FiringState asset scalar, offset 22922",
    "other_parameters": "Local design choices; not original server-equivalent rules",
    "spawn_coordinates": "Caller-supplied and caller-validated; no built-in coordinate guesses",
}


def vector(value: Iterable[float]) -> Vec3:
    result = tuple(float(v) for v in value)
    if len(result) != 3 or not all(math.isfinite(v) for v in result):
        raise ValueError("Expected a finite three-dimensional vector")
    return result  # type: ignore[return-value]


def normalized(value: Iterable[float]) -> Vec3:
    result = vector(value)
    length = math.sqrt(sum(v * v for v in result))
    if length < EPS:
        raise ValueError("Ray direction must be nonzero")
    return tuple(v / length for v in result)  # type: ignore[return-value]


def _cross(a: Vec3, b: Vec3) -> Vec3:
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def ray_sphere(origin, direction, center, radius_cm: float) -> float | None:
    """Nearest forward distance in cm; a ray beginning inside returns zero."""
    origin, direction, center = vector(origin), normalized(direction), vector(center)
    if not math.isfinite(radius_cm) or radius_cm <= 0:
        raise ValueError("Sphere radius must be finite and positive")
    delta = tuple(origin[i] - center[i] for i in range(3))
    b = sum(delta[i] * direction[i] for i in range(3))
    c = sum(v*v for v in delta) - radius_cm*radius_cm
    if c <= 0:
        return 0.0
    discriminant = b*b - c
    if discriminant < 0:
        return None
    distance = -b - math.sqrt(discriminant)
    return distance if distance >= 0 else None


@dataclass(frozen=True)
class TargetSphere:
    target_id: int
    region: str
    center: Vec3
    radius_cm: float


@dataclass(frozen=True)
class RayHit:
    target_id: int
    region: str
    distance_cm: float
    position: Vec3


def nearest_target_hit(origin, direction, targets: Iterable[TargetSphere], *,
                       blocking_distance_cm: float | None, wall_epsilon_cm: float = .1) -> RayHit | None:
    """Resolve closest sphere only before the scene obstruction. Walls win ties.

    None/+inf means the scene trace explicitly reported no blocking hit. A
    non-normalized direction is accepted, but returned distances remain in cm.
    """
    origin, direction = vector(origin), normalized(direction)
    wall = math.inf if blocking_distance_cm is None else float(blocking_distance_cm)
    if math.isnan(wall) or wall < 0 or not math.isfinite(wall_epsilon_cm) or wall_epsilon_cm < 0:
        raise ValueError("Invalid scene obstruction distance or tolerance")
    hits = []
    order = {"head": 0, "body": 1, "legs": 2}
    for target in targets:
        distance = ray_sphere(origin, direction, target.center, target.radius_cm)
        if distance is not None and distance < wall - wall_epsilon_cm:
            hits.append((distance, order.get(target.region, 3), target.target_id, target))
    if not hits:
        return None
    distance, _, _, target = min(hits, key=lambda row: row[:3])
    point = tuple(origin[i] + direction[i]*distance for i in range(3))
    return RayHit(target.target_id, target.region, distance, point)


@dataclass(frozen=True)
class TrainingEvent:
    kind: str
    at: float
    data: dict


@dataclass(frozen=True)
class WeaponConfig:
    shots_per_second: float = 9.75
    magazine_size: int = 25
    reload_seconds: float = 2.5
    reserve_ammo: int | None = None  # Infinite reserve, finite magazine, for training.
    automatic: bool = True
    base_spread_deg: float = .10
    moving_spread_deg: float = 1.50
    reference_move_speed_cm_s: float = 450.0
    bloom_per_shot_deg: float = .045
    max_bloom_deg: float = 1.5
    recoil_pitch_per_shot_deg: float = .35
    recoil_yaw_per_shot_deg: float = .12
    max_recoil_pitch_deg: float = 8.0
    recovery_delay_seconds: float = .12
    recovery_rate: float = 7.0
    ads_spread_multiplier: float = .75

    def __post_init__(self):
        positive = (self.shots_per_second, self.reload_seconds, self.reference_move_speed_cm_s,
                    self.recovery_rate, self.ads_spread_multiplier)
        nonnegative = (self.base_spread_deg, self.moving_spread_deg, self.bloom_per_shot_deg,
                       self.max_bloom_deg, self.recoil_pitch_per_shot_deg, self.recoil_yaw_per_shot_deg,
                       self.max_recoil_pitch_deg, self.recovery_delay_seconds)
        if not all(math.isfinite(v) and v > 0 for v in positive):
            raise ValueError("Weapon time/rate/scaling parameters must be finite and positive")
        if not all(math.isfinite(v) and v >= 0 for v in nonnegative):
            raise ValueError("Recoil/spread parameters must be finite and nonnegative")
        if isinstance(self.magazine_size, bool) or not isinstance(self.magazine_size, int) or not 1 <= self.magazine_size <= 1000:
            raise ValueError("Magazine size must be an integer from 1 to 1000")
        if self.reserve_ammo is not None and (isinstance(self.reserve_ammo, bool) or
                not isinstance(self.reserve_ammo, int) or self.reserve_ammo < 0):
            raise ValueError("Reserve ammunition must be a nonnegative integer or None")


KNIFE_CONFIG = WeaponConfig(shots_per_second=3.3333333333333335, magazine_size=5,
    reserve_ammo=0, base_spread_deg=0, moving_spread_deg=0, bloom_per_shot_deg=0,
    max_bloom_deg=0, recoil_pitch_per_shot_deg=0, recoil_yaw_per_shot_deg=0,
    max_recoil_pitch_deg=0)
KNIFE_DAMAGE = {"head": 150., "body": 50., "legs": 42.5}


@dataclass(frozen=True)
class Shot:
    shot_id: int
    at: float
    origin: Vec3
    direction: Vec3
    ammo_after: int
    pitch_offset_deg: float
    yaw_offset_deg: float
    spread_deg: float


@dataclass(frozen=True)
class WeaponFrame:
    shots: tuple[Shot, ...]
    events: tuple[TrainingEvent, ...]


class LocalWeapon:
    """Sampled trigger with exact shot scheduling and injectable time/randomness.

    Inputs change at the supplied timestamp. Catch-up shots between calls use
    the supplied aim sample; callers with changing aim should step every frame.
    Trigger releases stop shots at that timestamp, but preserve earlier shots.
    """
    def __init__(self, config: WeaponConfig | None = None, *, clock=time.monotonic,
                 rng: random.Random | None = None):
        self.config = config or WeaponConfig()
        self.clock, self.rng = clock, rng if rng is not None else random.Random()
        self.ammo = self.config.magazine_size
        self.reserve = self.config.reserve_ammo
        self.reload_ends_at: float | None = None
        self.trigger_held = False
        self._reload_pressed = False
        self._time = float(clock())
        self._next_shot = self._time
        self._recoil_time = self._time
        self._last_shot = -math.inf
        self.recoil_pitch_deg = self.recoil_yaw_deg = self.bloom_deg = 0.0
        self.shots_fired = 0
        self._dry_reported = False

    def _recover(self, at):
        begin = max(self._recoil_time, self._last_shot + self.config.recovery_delay_seconds)
        if at > begin:
            decay = math.exp(-self.config.recovery_rate * (at - begin))
            self.recoil_pitch_deg *= decay
            self.recoil_yaw_deg *= decay
            self.bloom_deg *= decay
        self._recoil_time = at

    def _complete_reload(self, at, events):
        if self.reload_ends_at is None or self.reload_ends_at > at + EPS:
            return
        end = self.reload_ends_at
        count = self.config.magazine_size - self.ammo
        if self.reserve is not None:
            count = min(count, self.reserve)
            self.reserve -= count
        self.ammo += count
        self.reload_ends_at = None
        self._dry_reported = False
        events.append(TrainingEvent("reload_completed", end, {"ammo": self.ammo, "reserve": self.reserve}))

    def _shoot(self, at, origin, direction, movement_speed, ads):
        self._recover(at)
        c = self.config
        spread = c.base_spread_deg + self.bloom_deg + c.moving_spread_deg * min(1.0, movement_speed/c.reference_move_speed_cm_s)
        if ads:
            spread *= c.ads_spread_multiplier
        angle = self.rng.uniform(0, math.tau)
        radius = math.sqrt(self.rng.random()) * spread
        pitch = self.recoil_pitch_deg + math.sin(angle)*radius
        yaw = self.recoil_yaw_deg + math.cos(angle)*radius
        reference_up = (0., 0., 1.) if abs(direction[2]) < .999 else (0., 1., 0.)
        right = normalized(_cross(reference_up, direction))
        up = _cross(direction, right)
        shot_direction = normalized(tuple(direction[i] + right[i]*math.tan(math.radians(yaw))
                                          + up[i]*math.tan(math.radians(pitch)) for i in range(3)))
        self.shots_fired += 1
        self.ammo -= 1
        shot = Shot(self.shots_fired, at, origin, shot_direction, self.ammo, pitch, yaw, spread)
        self.recoil_pitch_deg = min(c.max_recoil_pitch_deg, self.recoil_pitch_deg+c.recoil_pitch_per_shot_deg)
        self.recoil_yaw_deg += self.rng.uniform(-c.recoil_yaw_per_shot_deg, c.recoil_yaw_per_shot_deg)
        self.bloom_deg = min(c.max_bloom_deg, self.bloom_deg+c.bloom_per_shot_deg)
        self._last_shot = at
        self._next_shot = at + 1/c.shots_per_second
        return shot

    def step(self, trigger_held: bool, origin, aim_direction, *, now: float | None = None,
             reload_requested=False, movement_speed_cm_s=0., ads=False, blocked=False) -> WeaponFrame:
        at = float(self.clock() if now is None else now)
        if not math.isfinite(at) or at < self._time - EPS:
            raise ValueError("Weapon time must be finite and monotonic")
        origin, direction = vector(origin), normalized(aim_direction)
        movement_speed = float(movement_speed_cm_s)
        if not math.isfinite(movement_speed) or movement_speed < 0:
            raise ValueError("Movement speed must be finite and nonnegative")
        shots, events = [], []
        if blocked:
            # Skills/focus gates cancel the preceding held interval too. Merely
            # passing trigger=False would still catch up shots before this time.
            self.trigger_held = False
            trigger_held = reload_requested = False
        # Process the previous held interval, excluding the new input timestamp.
        if self.trigger_held and self.config.automatic:
            while True:
                due = max(self._next_shot, self.reload_ends_at or self._next_shot)
                if due >= at - EPS:
                    break
                self._complete_reload(due, events)
                if not self.ammo:
                    break
                shot = self._shoot(due, origin, direction, movement_speed, ads)
                shots.append(shot)
        self._complete_reload(at, events)
        rising = bool(trigger_held) and not self.trigger_held
        if reload_requested and not self._reload_pressed and self.reload_ends_at is None:
            if self.ammo < self.config.magazine_size and self.reserve != 0:
                self.reload_ends_at = at + self.config.reload_seconds
                events.append(TrainingEvent("reload_started", at, {"ends_at": self.reload_ends_at}))
        self.trigger_held = bool(trigger_held)
        self._reload_pressed = bool(reload_requested)
        if not trigger_held:
            self._dry_reported = False
        allowed = bool(trigger_held) and (self.config.automatic or rising)
        if allowed and self.reload_ends_at is None and at + EPS >= self._next_shot:
            if self.ammo:
                # A new press after idle starts now; it never backfills the idle period.
                shot = self._shoot(at, origin, direction, movement_speed, ads)
                shots.append(shot)
            elif not self._dry_reported or rising:
                events.append(TrainingEvent("dry_fire", at, {}))
                self._dry_reported = True
        self._recover(at)
        self._time = at
        events.extend(TrainingEvent("shot", shot.at, asdict(shot)) for shot in shots)
        return WeaponFrame(tuple(shots), tuple(sorted(events, key=lambda e: e.at)))

    def suspend(self, at: float):
        """Stow without spending ammo or later replaying a held trigger."""
        if not math.isfinite(at) or at < self._time - EPS:
            raise ValueError("Weapon time must be finite and monotonic")
        self.trigger_held = self._reload_pressed = False
        self.reload_ends_at = None
        self._recover(at)
        self._time = at
        self._next_shot = max(at, self._next_shot)

    def burst(self, origin, aim_direction, *, now: float) -> WeaponFrame:
        """Local knife alt-fire: spend the current supply at one timestamp."""
        self.trigger_held = False
        base = self.step(False, origin, aim_direction, now=now)
        if now + EPS < self._next_shot or not self.ammo:
            return base
        origin, direction = vector(origin), normalized(aim_direction)
        shots = tuple(self._shoot(now, origin, direction, 0., False) for _ in range(self.ammo))
        return WeaponFrame(shots, base.events + tuple(TrainingEvent("shot", s.at, asdict(s)) for s in shots))


@dataclass(frozen=True)
class BotLane:
    name: str
    start_cm: Vec3
    end_cm: Vec3
    validated: bool = False
    map_package: str = DUEL1_MAP

    def __post_init__(self):
        object.__setattr__(self, "start_cm", vector(self.start_cm))
        object.__setattr__(self, "end_cm", vector(self.end_cm))
        if not self.name or self.map_package != DUEL1_MAP:
            raise ValueError("A named Duel 1 lane is required")

    @property
    def length_cm(self):
        return math.dist(self.start_cm, self.end_cm)


@dataclass(frozen=True)
class BotConfig:
    health: float = 150.
    speed_cm_s: float = 350.
    hold_at_end_seconds: float = .4
    spawn_delay_seconds: tuple[float, float] = (.6, 1.2)
    head_damage: float = 160.
    body_damage: float = 40.
    leg_damage: float = 34.
    avoid_immediate_lane_repeat: bool = True

    def __post_init__(self):
        positive = (self.health, self.speed_cm_s, self.hold_at_end_seconds,
                    self.head_damage, self.body_damage, self.leg_damage)
        if not all(math.isfinite(v) and v > 0 for v in positive):
            raise ValueError("Bot parameters must be finite and positive")
        low, high = self.spawn_delay_seconds
        if not all(math.isfinite(v) for v in (low, high)) or not .05 <= low <= high:
            raise ValueError("Respawn delay must be ordered and at least .05 seconds")


@dataclass
class BotState:
    bot_id: int
    lane_name: str
    position_cm: Vec3
    health: float
    spawned_at: float
    expires_at: float


class BotDrill:
    def __init__(self, lanes: Iterable[BotLane], config: BotConfig | None = None, *,
                 clock=time.monotonic, rng: random.Random | None = None, initial_delay_seconds=0.):
        self.lanes = tuple(lanes)
        if not self.lanes or len(self.lanes) > 64 or not all(l.validated for l in self.lanes):
            raise ValueError("Provide 1–64 explicitly validated Duel 1 lanes; no coordinates are guessed")
        if len({l.name for l in self.lanes}) != len(self.lanes):
            raise ValueError("Lane names must be unique")
        if not math.isfinite(initial_delay_seconds) or initial_delay_seconds < 0:
            raise ValueError("Invalid initial spawn delay")
        self.config = config or BotConfig()
        self.clock, self.rng = clock, rng if rng is not None else random.Random()
        self._time = float(clock())
        self.next_spawn_at = self._time + initial_delay_seconds
        self.bot: BotState | None = None
        self._lane: BotLane | None = None
        self._previous_lane = None
        self.rounds = self.kills = self.missed = 0

    def _schedule(self, at):
        self.next_spawn_at = at + self.rng.uniform(*self.config.spawn_delay_seconds)

    def advance(self, now: float | None = None) -> list[TrainingEvent]:
        at = float(self.clock() if now is None else now)
        if not math.isfinite(at) or at < self._time - EPS:
            raise ValueError("Bot time must be finite and monotonic")
        events = []
        for _ in range(10000):
            if self.bot is None:
                if self.next_spawn_at > at + EPS:
                    break
                options = list(self.lanes)
                if self.config.avoid_immediate_lane_repeat and len(options) > 1:
                    options = [l for l in options if l.name != self._previous_lane]
                self._lane = self.rng.choice(options)
                self._previous_lane = self._lane.name
                self.rounds += 1
                spawned = self.next_spawn_at
                expires = spawned + self._lane.length_cm/self.config.speed_cm_s + self.config.hold_at_end_seconds
                self.bot = BotState(self.rounds, self._lane.name, self._lane.start_cm, self.config.health, spawned, expires)
                events.append(TrainingEvent("bot_spawned", spawned, asdict(self.bot)))
            assert self._lane is not None
            elapsed = max(0., min(at, self.bot.expires_at) - self.bot.spawned_at)
            length = self._lane.length_cm
            progress = min(1., elapsed*self.config.speed_cm_s/length) if length else 1.
            self.bot.position_cm = tuple(a+(b-a)*progress for a, b in zip(self._lane.start_cm, self._lane.end_cm))
            if self.bot.expires_at > at + EPS:
                break
            expired = self.bot.expires_at
            events.append(TrainingEvent("bot_escaped", expired, asdict(self.bot)))
            self.missed += 1
            self.bot = None
            self._schedule(expired)
        else:
            raise ValueError("Bot update exceeds the bounded event budget; update more frequently")
        self._time = at
        return events

    def hit_spheres(self) -> tuple[TargetSphere, ...]:
        """Legacy geometry for explicitly selected offline fixtures only."""
        if self.bot is None or self.bot.health <= 0:
            return ()
        x, y, z = self.bot.position_cm
        return (TargetSphere(self.bot.bot_id, "head", (x, y, z+165), 16),
                TargetSphere(self.bot.bot_id, "body", (x, y, z+110), 38),
                TargetSphere(self.bot.bot_id, "legs", (x, y, z+45), 35))

    def resolve_shot(self, shot: Shot, blocking_distance_cm: float | None, *,
                     target_hit: RayHit | None = None, use_fixture_geometry=False,
                     damage_by_region=None) -> list[TrainingEvent]:
        events = self.advance(shot.at)
        wall = math.inf if blocking_distance_cm is None else float(blocking_distance_cm)
        if math.isnan(wall) or wall < 0:
            raise ValueError("Invalid scene obstruction distance")
        hit = target_hit
        if use_fixture_geometry:
            hit = nearest_target_hit(shot.origin, shot.direction, self.hit_spheres(),
                                     blocking_distance_cm=wall)
        elif hit is not None:
            if (hit.region not in ("head", "body", "legs") or not math.isfinite(hit.distance_cm)
                    or hit.distance_cm < 0):
                raise ValueError("Invalid native target hit")
            vector(hit.position)
            # Native geometry belongs to the rendered target, which may differ
            # from the target now scheduled by the local drill. Never transfer damage.
            if self.bot is None or hit.target_id != self.bot.bot_id or hit.distance_cm >= wall - .1:
                hit = None
        if hit is None or self.bot is None:
            events.append(TrainingEvent("shot_missed", shot.at, {"shot_id": shot.shot_id}))
            return events
        damage = (damage_by_region or {"head": self.config.head_damage, "body": self.config.body_damage,
                  "legs": self.config.leg_damage})[hit.region]
        if not math.isfinite(damage) or damage <= 0:
            raise ValueError("Damage must be finite and positive")
        applied = min(self.bot.health, damage)
        self.bot.health = max(0., self.bot.health-damage)
        payload = {**asdict(hit), "shot_id": shot.shot_id, "damage": applied, "health_after": self.bot.health}
        events.append(TrainingEvent("bot_hit", shot.at, payload))
        if self.bot.health == 0:
            self.kills += 1
            events.append(TrainingEvent("bot_killed", shot.at, {**payload, "lane_name": self.bot.lane_name}))
            self.bot = None
            self._schedule(shot.at)
        return events


class LocalTrainingSession:
    """Pure local health/weapon rules; production requires both native trace callbacks."""
    def __init__(self, lanes: Iterable[BotLane], *, weapon_config=None, bot_config=None,
                 clock=time.monotonic, rng: random.Random | None = None,
                 target_trace=None, use_fixture_geometry=False):
        self.clock = clock
        self.target_trace = target_trace
        self.use_fixture_geometry = bool(use_fixture_geometry)
        rng = rng if rng is not None else random.Random()
        self.rifle = LocalWeapon(weapon_config, clock=clock, rng=random.Random(rng.getrandbits(64)))
        self.weapon = self.rifle
        self.drill = BotDrill(lanes, bot_config, clock=clock, rng=random.Random(rng.getrandbits(64)))
        self.knives = LocalWeapon(KNIFE_CONFIG, clock=clock, rng=random.Random(rng.getrandbits(64)))
        self.equipment_mode = 0
        self.shots_fired = 0
        self._burst_pressed = False
        self.hits = self.headshots = 0

    def equip(self, mode: int, at: float) -> list[TrainingEvent]:
        if isinstance(mode, bool) or mode not in (0, 1):
            raise ValueError("Equipment mode must be 0 (rifle) or 1 (knives)")
        if mode == self.equipment_mode:
            return []
        self.weapon.suspend(at)
        self.weapon = self.knives if mode else self.rifle
        self.weapon.suspend(at)
        self.equipment_mode = mode
        self._burst_pressed = False
        if mode:
            self.knives.ammo = 5
            self.knives._dry_reported = False
        return [TrainingEvent("equipment_changed", at, {"equipment_mode": mode, "ammo": self.weapon.ammo})]

    def step(self, trigger_held, origin, aim_direction, *, scene_trace: Callable[[Vec3, Vec3, float], float | None],
             now=None, reload_requested=False, movement_speed_cm_s=0., ads=False,
             equipment_mode=None, firing_blocked=False, burst_requested=False,
             target_trace: Callable[[Vec3, Vec3, float], RayHit | None] | None = None) -> list[TrainingEvent]:
        if not callable(scene_trace):
            raise ValueError("A scene obstruction trace callback is required")
        query_target = self.target_trace if target_trace is None else target_trace
        if not callable(query_target) and not self.use_fixture_geometry:
            raise ValueError("A native target trace callback is required; fixture geometry must be explicitly selected")
        at = float(self.clock() if now is None else now)
        events = self.equip(equipment_mode, at) if equipment_mode is not None else []
        burst = self.equipment_mode == 1 and burst_requested and not self._burst_pressed and not firing_blocked
        self._burst_pressed = bool(burst_requested)
        if burst:
            frame = self.weapon.burst(origin, aim_direction, now=at)
        else:
            frame = self.weapon.step(trigger_held, origin, aim_direction, now=at,
                                     reload_requested=reload_requested and self.equipment_mode == 0,
                                     movement_speed_cm_s=movement_speed_cm_s, ads=ads and self.equipment_mode == 0,
                                     blocked=firing_blocked)
        events.extend(e for e in frame.events if e.kind != "shot")
        killed_with_knives = False
        for shot in frame.shots:
            self.shots_fired += 1
            shot = replace(shot, shot_id=self.shots_fired)
            events.append(TrainingEvent("shot", shot.at, {**asdict(shot), "equipment_mode": self.equipment_mode, "burst": bool(burst)}))
            wall_distance = scene_trace(shot.origin, shot.direction, shot.at)
            target_hit = query_target(shot.origin, shot.direction, shot.at) if callable(query_target) else None
            resolved = self.drill.resolve_shot(shot, wall_distance,
                target_hit=target_hit, use_fixture_geometry=self.use_fixture_geometry and not callable(query_target),
                damage_by_region=KNIFE_DAMAGE if self.equipment_mode else None)
            for event in resolved:
                if event.kind == "bot_hit":
                    self.hits += 1
                    self.headshots += int(event.data["region"] == "head")
                elif event.kind == "bot_killed" and self.equipment_mode == 1:
                    killed_with_knives = True
            events.extend(resolved)
        if killed_with_knives:
            self.knives.ammo = 5
            self.knives._dry_reported = False
            events.append(TrainingEvent("knives_refilled", at, {"ammo": 5}))
        events.extend(self.drill.advance(at))
        priority = {"reload_completed": 0, "bot_spawned": 1, "shot": 2, "bot_hit": 3, "bot_killed": 4}
        return sorted(events, key=lambda event: (event.at, priority.get(event.kind, 5)))

    def snapshot(self) -> dict:
        return {"map_package": DUEL1_MAP, "rules_kind": "local_reimplementation",
                "hit_geometry": "fixture_spheres" if self.use_fixture_geometry else "native_physics_asset",
                "equipment_mode": self.equipment_mode, "equipment": "knives" if self.equipment_mode else "vandal",
                "rifle_ammo": self.rifle.ammo, "knife_ammo": self.knives.ammo,
                "magazine_size": self.weapon.config.magazine_size,
                "ammo": self.weapon.ammo, "reserve": self.weapon.reserve,
                "reload_ends_at": self.weapon.reload_ends_at,
                "recoil_pitch_deg": self.weapon.recoil_pitch_deg,
                "recoil_yaw_deg": self.weapon.recoil_yaw_deg,
                "bot": asdict(self.drill.bot) if self.drill.bot else None,
                "rounds": self.drill.rounds, "kills": self.drill.kills, "escaped": self.drill.missed,
                "shots": self.shots_fired, "hits": self.hits, "headshots": self.headshots,
                "original_server_equivalence_proven": False}
