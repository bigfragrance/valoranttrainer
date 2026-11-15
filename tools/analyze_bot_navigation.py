"""Read-only analysis of original bot-navigation metadata and recorded route runs.

No game/process launch, map modification, account access, or original-file write.
All generated artifacts are required to remain on F:.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STYLE_NAMES = ["jump_wide", "wide", "small", "crouch", "peek", "random", "global"]


def read_json(path: Path | None):
    return json.loads(path.read_text(encoding="utf-8-sig")) if path else None


def vector(value):
    if isinstance(value, dict):
        value = [value.get(k) for k in ("x", "y", "z")]
    if not isinstance(value, (list, tuple)) or len(value) != 3:
        raise ValueError("route point requires exactly three world-centimetre feet coordinates")
    if any(isinstance(v, bool) or not isinstance(v, (int, float)) or not math.isfinite(v) for v in value):
        raise ValueError("route coordinates must be finite numbers")
    return [float(v) for v in value]


def route_analysis(document):
    rows = document.get("routes", []) if isinstance(document, dict) else document
    if not isinstance(rows, list) or len(rows) > 64:
        raise ValueError("routes must be a list with at most64 entries")
    result, ids = [], set()
    for row in rows:
        identity = row.get("id")
        if isinstance(identity, bool) or not isinstance(identity, int) or identity <= 0 or identity in ids:
            raise ValueError("route ids must be unique positive integers")
        ids.add(identity)
        map_key = row.get("map_key", row.get("mapKey"))
        if not isinstance(map_key, str) or not map_key or len(map_key.encode("utf8")) >= 96:
            raise ValueError("every route needs its exact map_key")
        points = [vector(row[name]) for name in ("start", "expose", "retreat")]
        legs = []
        for start, end in ((0, 1), (1, 2), (2, 0)):
            delta = [points[end][i] - points[start][i] for i in range(3)]
            legs.append({"from": ("start", "expose", "retreat")[start], "to": ("start", "expose", "retreat")[end],
                         "length_cm": math.dist(points[start], points[end]), "height_change_cm": delta[2]})
        if legs[0]["length_cm"] < 20 or any(leg["length_cm"] > 20000 for leg in legs):
            raise ValueError("route expose leg must be at least20cm and every leg at most20000cm")
        enabled = row.get("enabled", True)
        weight = row.get("weight", 1)
        style = row.get("style", "global")
        if not isinstance(enabled, bool) or isinstance(weight, bool) or not isinstance(weight, (int, float)) or not math.isfinite(weight) or not 0 <= weight <= 1000:
            raise ValueError("route enabled must be boolean and weight finite0..1000")
        if isinstance(style, int) and not isinstance(style, bool) and 0 <= style <= 6:
            style = STYLE_NAMES[style]
        if style not in STYLE_NAMES:
            raise ValueError("unknown route style")
        result.append({"id": identity, "map_key": map_key, "feet": points, "legs": legs,
                       "enabled": enabled, "weight": weight, "style": style,
                       "traversability": "not_inferred_from_editor_points",
                       "required": "each actual leg needs a complete original Nav path or original capsule-and-ground validation"})
    total_weight = sum(r["weight"] for r in result if r["enabled"])
    for row in result:
        row["selection_probability"] = row["weight"] / total_weight if row["enabled"] and total_weight else 0
    return result


def analyze(diagnostics=None, route_document=None, movement=None):
    header = ROOT / "native/training_bot_controller.h"
    evidence = ROOT / "work/training-bot-controller"
    result = {
        "schema_version": 1,
        "scope": "Independent local decision policy on original heroes/input/physics; static and recorded diagnostics only. No native tactical-AI reuse claim.",
        "status": "implemented_static_checks_complete_root_build_and_runtime_pending",
        "original_image_sha256": "147763350504d241ee20b77607d04bc85cebfedb1d8bc2f32473ca93bf8fe92f",
        "header": {"path": str(header), "sha256": hashlib.sha256(header.read_bytes()).hexdigest()},
        "api": {
            "initialize": "Initialize(world, Hooks); SetConfig/Configure(Config); SetRoutes(exactMapKey, Route*,count); SetAnchors(Vec*,count)",
            "config_mapping": {"reaction_ms": "reactionMs", "reaction_spread_ms": "reactionSpreadMs", "aim_speed": "aimSpeed (degrees/sec)", "aim_spread": "aimSpeedSpread", "accuracy": "accuracy (0..1)", "accuracy_spread": "accuracySpread", "movement": "movement (0..1)", "movement_spread": "movementSpread", "seed": "uint32 seed"},
            "legacy": "strength/strengthSpread retained for source compatibility but not used to collapse the four controls; explicit SetSkill remains an optional per-actor override",
            "bind": "Bind(rosterIndex, originalPawn, spawnEpoch,isBot); bind local0 with isBot=false for observations only",
            "tick": "Tick(monotonicSeconds) replaces old original_enemies::Update for bound bots; do not run both AI schedulers",
            "stopping": "Stop immediately submits released movement/jump/fire states to ready living bots",
            "hooks": ["observe from original alive/spawn-epoch registry", "visible from original world trace", "validateSegment original capsule clearance plus support ground", "weaponInput original per-actor fire/reload input"],
            "match_owner": "Root deathmatch_rules owns all-pairs FFA eligibility, player invincibility,40-kill/540-second finish, real damage, death and respawn",
        },
        "original_inputs": {
            "world_movement": "Pawn.AddMovementInput original virtual8a0=4856280 via existing SubmitWorldInput. Physics, acceleration, friction, collision, speed and AnimBP stay original.",
            "stance": "ForceMoveInputState payload2 (AlternateMovement0/Crouch1,Released0/Pressed1); ForceJumpInputState payload1, exec6c6a880->7882850, with original standalone input transport. Genuine SetJumpHeld6c648d0->78812f0 is used when no original component/force gate accepts the jump.",
            "npc_initialization": "PrepareNPCInput(index) automatically called by bot Bind: original ShooterPC.InitInputSystem781cbd0 (vtablefe8) constructs real AresPlayerInput and Engine492dd80 PC input component. Guards require no NPC UPlayer and unchanged actual local player/pawn/input/3P policy. No local hardware-binding application is called by adapter.",
            "standalone_transport": "Pending original AresInputComponent command buffer is submitted via original70a7110(pawn.ba0,input+178); native fixed-step AresInputState tick remains the interpreter. Existing weapon hook may already drain it; empty queue is not resubmitted.",
            "readback": "Original GetCrouchHeld/GetJumpHeld/GetMovementType, actual velocity, current capsule geometry and guarded original speed/jump getters. Crouch requested for>.5s without native consumption blocks the route rather than simulating a pose.",
            "feet": "Original Actor.GetActorLocation minus actual current original capsule half-height, with original Pawn.GetNavAgentLocation separately recorded. The nav-agent location is not asserted equal to feet without runtime comparison.",
            "eyes": "Original Actor.GetActorEyesViewPoint (48-byte payload)",
            "sizes": "No write to native capsule, mesh scale, bone pose, movement tuning or eye height",
            "npc_input_limit": "NPC PC.PlayerInput and source ownership are established/read back by original initialization; runtime original held flags, crouch interpolation and jump arc still need verification. Stored force flags or a successful initializer alone do not prove physical consumption.",
        },
        "navigation": {
            "availability": "unverified_runtime",
            "original_system": "NavigationSystemV1.GetNavigationSystem, payload16; presence alone is not a usable NavMesh proof",
            "path": "FindPathToLocationSynchronously80, PathfindingContext=actual original pawn; require original NavigationPath.IsValid and !IsPartial and finite full PathPoints",
            "path_points": "original NavigationPath.PathPoints+40, count+48, capacity+4c; FVector24; at most64 copied waypoints",
            "projection": "K2_ProjectPointToNavigation metadata found (storage104, return bool+96; runtime payload expected97); controller does not silently project/alter editor endpoints",
            "local_fallback": "Only when root validateSegment certifies current original capsule clearance AND supporting ground;<=1800cm by default,<=60cm endpoint height change; no unverified direct movement",
            "blocked": "No validated path, partial path, oversized path or native collision/progress stall stops that route; no per-frame teleport or invented step-up",
            "legacy_limit": "reports/duel1-navigation-validation.json validated old DefaultPawn manual navigation, not original NavMesh or tactical navigation",
        },
        "hold_angle": {
            "coordinates": "Map-specific world-centimetre feet: start/expose/retreat; map image conversion and file persistence belong to map/editor/control layer",
            "route_count_limit": 64, "route_selection": "enabled positive-weight saved routes sampled proportionally to weight; route style overrides global, Random stays random, Global explicitly inherits",
            "cycle": "physically reach start -> wait -> expose -> brief hold -> withdraw -> return -> next saved route",
            "styles": STYLE_NAMES,
            "config_updates": "identical config is a no-op; only changed personality fields refresh profiles; only mode/navigation-shape changes reset plans. Frequency/fire/invincibility/style edits preserve ongoing pulls. Route metadata edits preserve unaffected geometry/plans.",
            "style_details": "JumpWide submits native jump edge for.13s; Wide uses full exposure; Small targets45% of the selected start-to-expose leg and validates that actual target; Crouch uses original stance; Peek walks with brief.06s exposure; Random picks a method per cycle",
            "frequency": "0.5..180 requested cycles/minute, constrained by actual original travel/stance time; no speed scaling to fake a requested rate",
            "return_fire_default": False,
        },
        "deathmatch_policy": {
            "total_players_limit": 32, "requested_rules": {"kills": 40, "seconds": 540},
            "strengths": "Four independent configured controls, each sampled per actor with a centered triangular distribution bounded by卤spread; seed mixes configured seed, roster index and full64-bit spawn epoch. Behavior RNG is separate from profile draws.",
            "configuration": {"reaction_ms": "reactionMs/reactionSpreadMs;0..5000ms config, actual sampled reaction floor40ms", "aim_speed": "aimSpeed/aimSpeedSpread;1..1440degrees/s", "accuracy": "accuracy/accuracySpread;normalized0..1", "movement": "movement/movementSpread;normalized0..1; decisions only, never original movement-speed/tuning writes", "seed": "uint32 deterministic seed; Configure/SetConfig idempotent; zero spread intentionally gives identical configured values"},
            "reaction": "sampled reactionMs卤reactionSpreadMs, floor40ms, plus.02..12s encounter jitter before correction/fire",
            "turn": "sampled aimSpeed卤aimSpeedSpread (1..1440deg/s), bounded by elapsed time; no instantaneous target snap",
            "accuracy": "strength-dependent input aim error, moving penalty, time-varying bias, head/chest selection and bursts with pauses; native gun spread/recoil remain original",
            "movement": "validated anchor roaming, original-path or capsule-validated local strafes, held crouch decisions; no claimed original tactical cover or original bot-controller transplant",
            "visibility": "original trace hook required for target acquisition/fire; missing hook disables fire",
            "shots": "weaponInput is a request to root real weapon adapter; missing adapter disables fire and increments diagnostics; no generated hits/damage",
        },
        "runtime": {"provided": diagnostics is not None},
        "routes": route_analysis(route_document) if route_document is not None else [],
        "required_root_validation": [
            "Use real loaded map and verify original NavigationPath availability/full paths or actual capsule+ground validator before enabling a route",
            "Bind exact current pawn/spawnEpoch and release old input on death; bind fresh pawn only after original alive/weapon readiness",
            "Exercise every style and compare requested vs original held flags, actual displacement, speed, crouched capsule and jump arc; verify original mesh/material/foot animations visually",
            "Save/reload multiple map-specific routes through control layer, then observe complete random cycles and frequency limits",
            "Return-fire off must generate no bot weapon-input shot; on must advance original fire/ammo/recoil/shot callbacks and original trace/damage",
            "Deathmatch needs root all-pairs rule and real enemy weapon runtime for every actor, including local target;40-kill/9min outcome is owned and verified by deathmatch_rules",
        ],
        "limits": ["No build or game run by this agent", "No Nav availability inferred from metadata or pointer existence", "No original tactical AI reused", "Recorded native inputs are not pixel or gameplay acceptance"],
    }
    result["native_input_static_evidence"] = []
    for address in ("781cbd0", "492dd80", "78826c0", "7882850", "6c648d0", "78812f0", "7881110"):
        source = evidence / ("function-0x" + address + ".json")
        if source.exists():
            result["native_input_static_evidence"].append({"rva": "0x" + address, "disassembly": str(source), "sha256": hashlib.sha256(source.read_bytes()).hexdigest()})
    result["required_root_validation"].extend([
        "Identical SetConfig must leave config_updates/profile_refreshes/plan_resets unchanged; reaction/aim/accuracy/movement edits may refresh profiles but preserve current routes; fire/frequency/invincibility changes must not restart exposes",
        "Weighted routes must exclude disabled and zero-weight entries; local style overrides global, Global6 inherits, Random5 remains random; completed cycles randomize routes by enabled weight",
        "NPC PrepareNPCInput must read back original AresPlayerInput owned by that NPC PC, preserve actual local pair/input/3P selector, and then original crouch/slowwalk must actually consume force overrides",
        "Jump uses original ForceJump+standalone transport when accepted, otherwise genuine original SetJumpHeld cooldown/Jump path; verify actual arc, not just queued event/held count",
    ])
    abi_file = evidence / "abi-discovered.json"
    if abi_file.exists():
        result["static_abi"] = [x for x in read_json(abi_file) if x["name"] not in ("IsValid",)]
    if diagnostics is not None:
        result["runtime"]["diagnostics"] = diagnostics
        result["runtime"]["original_nav_succeeded"] = bool(diagnostics.get("original_path_succeeded"))
        result["runtime"]["validated_local_segment_succeeded"] = bool(diagnostics.get("validated_local_segment_succeeded"))
        result["runtime"]["not_all_routes_certified"] = True
    if movement is not None:
        result["runtime"]["recorded_original_movement"] = movement
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--diagnostics", type=Path)
    parser.add_argument("--routes", type=Path)
    parser.add_argument("--movement", type=Path)
    parser.add_argument("--out", type=Path, default=ROOT / "reports/training-bot-controller.json")
    args = parser.parse_args()
    paths = [p.resolve() for p in (args.diagnostics, args.routes, args.movement, args.out) if p]
    if any(p.drive.lower() != "f:" for p in paths):
        parser.error("this offline task reads and writes task artifacts only on F:")
    result = analyze(read_json(args.diagnostics), read_json(args.routes), read_json(args.movement))
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf8")
    print(json.dumps({"report": str(args.out), "status": result["status"], "routes": len(result["routes"])}, ensure_ascii=False))


if __name__ == "__main__":
    main()
