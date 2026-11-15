"""Observe actual owned-pair runtime evidence; never inject gameplay or UI input."""
from __future__ import annotations

import argparse
import datetime as dt
import json
import math
from pathlib import Path
import time

ROOT = Path(__file__).resolve().parents[1]


def read(path: Path) -> dict:
    try:
        value = json.loads(path.read_text(encoding='utf-8'))
        return value if isinstance(value, dict) else {}
    except (OSError, ValueError):
        return {}


def vector(value):
    if (isinstance(value, list) and len(value) == 3
            and all(isinstance(x, (int, float)) and math.isfinite(x) for x in value)):
        return value
    return None


def distance(a, b):
    return math.sqrt(sum((x-y)**2 for x, y in zip(a, b)))


def observe(directory: Path, seconds: float) -> dict:
    directory = directory.resolve()
    parent = (ROOT / '.local/local-server').resolve()
    if directory.parent != parent or not directory.name.startswith('pair-20'):
        raise ValueError('Use one actual project-owned .local/local-server/pair-20... directory')
    identity = read(directory / 'session.active.json') or read(directory / 'progress.json')
    if identity.get('network_flavor') != 'playable' or not identity.get('module_sha256'):
        raise ValueError('Owned playable-pair identity and module digest are required')
    result = dict(schema_version=1, kind='actual-native-gameplay-observation',
                  directory=str(directory), module_sha256=identity['module_sha256'],
                  at_utc=dt.datetime.now(dt.timezone.utc).isoformat(),
                  playable_verified=False, read_only=True, requested_seconds=seconds,
                  samples=0, errors=[], maxima={}, evidence={}, checks={})
    maxes, evidence, checks = (result[k] for k in ('maxima', 'evidence', 'checks'))
    first_client_position = None
    last_host = last_client = {}
    start = time.monotonic()
    while time.monotonic()-start < seconds:
        host = read(directory / 'host/bridge-state.json')
        client = read(directory / 'client/bridge-state.json')
        if not host or not client:
            time.sleep(.1)
            continue
        last_host, last_client = host, client
        result['samples'] += 1
        elapsed = round(time.monotonic()-start, 3)
        for role, state in (('host', host), ('client', client)):
            if state.get('error'):
                error = dict(role=role, code=state['error'], stage=state.get('stage'))
                if error not in result['errors']:
                    result['errors'].append(error)
        authority = host.get('authority_input', {})
        combat = read(directory / 'host/combat-state.json')
        npc = host.get('npc_authority', {})
        prediction = client.get('session_runtime', {}).get('client_equipment', {}).get('native_prediction', {})
        counters = {
            'human_physics': host.get('input_physics', 0),
            'human_acks': host.get('input_acks', 0),
            'crouch_commands': authority.get('crouch_input_commands', 0),
            'crouch_transitions': authority.get('crouch_transitions', 0),
            'npc_physics': npc.get('physics_completed', 0),
            'autonomous_items': prediction.get('autonomous_items', 0),
            'predicted_states': prediction.get('predicted_states', 0),
            'human_native_rays': sum(p.get('native_rays', 0) for p in combat.get('players', []) if p.get('index') == 0),
            'npc_native_rays': sum(p.get('native_rays', 0) for p in combat.get('players', []) if p.get('index', 0) > 0),
            'native_damage_events': combat.get('damage_events', 0),
            'confirmed_kills': combat.get('kills', 0),
        }
        for name, value in counters.items():
            if isinstance(value, (int, float)):
                maxes[name] = max(value, maxes.get(name, 0))
        if prediction.get('all_native_prediction_ready') and 'native_prediction' not in evidence:
            evidence['native_prediction'] = dict(elapsed=elapsed, **prediction)
        intent = vector(authority.get('last_intent'))
        velocity = vector(authority.get('velocity_after'))
        if velocity:
            maxes['authority_upward_velocity_cm_s'] = max(maxes.get('authority_upward_velocity_cm_s', 0), velocity[2])
        before, after = (vector(authority.get(k)) for k in ('position_before', 'position_after'))
        if intent and before and after and distance(intent, [0, 0, 0]) > .01 and distance(before, after) > .01:
            evidence.setdefault('native_intent_physical_step', dict(elapsed=elapsed, intent=intent, before=before, after=after))
        pose = client.get('client_native_pose', {})
        position = vector(pose.get('position')) if pose.get('observed') else None
        if position:
            if first_client_position is None:
                first_client_position = position
            maxes['client_displacement_cm'] = max(maxes.get('client_displacement_cm', 0), distance(first_client_position, position))
            if after:
                result['last_host_client_distance_cm'] = distance(after, position)
        checks.update(
            no_observed_module_error=not result['errors'],
            original_client_hud=bool(client.get('client_hud_ready')),
            original_client_camera=bool(client.get('client_camera_ready')),
            actual_owned_item_prediction='native_prediction' in evidence,
            npc_original_physics=maxes.get('npc_physics', 0) > 128,
            original_human_input_ack=maxes.get('human_physics', 0) > 128 and maxes.get('human_acks', 0) > 100,
            human_intent_physical_step='native_intent_physical_step' in evidence,
            client_position_changed=maxes.get('client_displacement_cm', 0) > 10,
            native_crouch_transition=maxes.get('crouch_transitions', 0) > 0,
            human_original_firing_ray=maxes.get('human_native_rays', 0) > 0,
        )
        if result['errors'] or host.get('cleaned') or client.get('cleaned'):
            break
        time.sleep(.1)
    result['elapsed_seconds'] = round(time.monotonic()-start, 3)
    result['last_stages'] = dict(host=last_host.get('stage'), client=last_client.get('stage'))
    result['all_observed_checks_passed'] = bool(checks) and all(checks.values())
    result['limitations'] = ['No input generated by this observer.',
                            'Spawn displacement alone is not human movement proof.',
                            'Firing, damage, skill use, menus, audio, full-match and map acceptance remain separate.']
    (directory / 'root-gameplay-observation.json').write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session-dir', type=Path, required=True)
    parser.add_argument('--seconds', type=float, default=60)
    args = parser.parse_args()
    if not 1 <= args.seconds <= 300:
        parser.error('Choose 1..300 seconds')
    result = observe(args.session_dir, args.seconds)
    print(json.dumps(result, ensure_ascii=False))
    return 1 if result['errors'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
