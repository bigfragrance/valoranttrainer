"""Audit archived native audio snapshots; never launches or accesses a process."""
from __future__ import annotations

import argparse
import json
from pathlib import Path


def pointer(value):
    return int(value or '0', 16)


def audit(directory, expected_pid=None):
    directory = Path(directory)
    listeners = json.loads((directory / 'original-audio-listeners.json').read_text(encoding='utf-8-sig'))
    locality = json.loads((directory / 'original-npc-locality.json').read_text(encoding='utf-8-sig'))
    roles = json.loads((directory / 'training-player-roles.json').read_text(encoding='utf-8-sig'))
    pid = expected_pid if expected_pid is not None else roles['pid']
    current = listeners.get('current', {})
    rows = current.get('listeners', [])
    own = [r for r in rows if r.get('kind') == 'actual_local_pc']
    local_players = [p for p in roles.get('players', []) if p.get('local') and p.get('uplayer_is_local')]
    primary = pointer(current.get('primary_spatial_listener'))
    checks = {
        'same_native_process': pid > 0 and listeners.get('pid') == locality.get('pid') == roles.get('pid') == pid,
        'native_audio_profile_verified': listeners.get('source_verified') is True and listeners.get('error') == 0,
        'actual_local_binding_verified': listeners.get('actual_local_pair_valid') is True,
        'audio_device_ready': listeners.get('device_ready') is True and not listeners.get('waiting'),
        'listener_snapshot_consistent': current.get('valid') is True and current.get('consistent') is True,
        'listener_ownership_unambiguous': listeners.get('ambiguous') is False,
        'one_actual_local_player': roles.get('local_player_count') == 1 and roles.get('actual_local_count') == 1 and len(local_players) == 1,
        'one_actual_local_default_listener': current.get('own') == len(own) == 1,
        'no_nonlocal_camera_listeners': current.get('nonlocal_pc') == 0 and not any(r.get('kind') == 'same_world_nonlocal_pc' for r in rows),
        'native_listener_count_readback': current.get('count') == len(rows),
        'listener_owned_by_actual_player': len(own) == 1 and len(local_players) == 1 and pointer(own[0]['controller']) == pointer(local_players[0]['controller']) == pointer(listeners.get('active_controller')),
        'primary_spatial_listener_preserved_or_local': primary == 0 or any(pointer(r.get('component')) == primary and r.get('kind') in ('actual_local_pc', 'non_pc_special_preserved') for r in rows),
        'original_remove_completed': listeners.get('native_calls') == listeners.get('native_completed'),
        'original_remaining_set_preserved': all(r.get('original_completed') and r.get('removed') and r.get('remaining_preserved') for r in listeners.get('removals', [])),
        'no_manual_audio_value_rewrite': all(listeners.get(k) == 0 for k in ('image_writes', 'manual_set_writes', 'volume_writes', 'locality_writes')),
        'five_original_audio_functions_bound': locality.get('audio_functions_bound') == 5 and locality.get('audio_bind_mismatches') == 0,
        'listener_registration_consumer_verified': locality.get('listener_registration_profile') is True,
        'audio_frame_guards_valid': locality.get('audio_frame_context_rejects') == locality.get('audio_frame_code_rejects') == 0,
        'spectated_audio_frame_guards_valid': locality.get('spectated_frame_profile') is True and all(locality.get(k) == 0 for k in ('spectated_frame_copy_rejects', 'spectated_context_rejects', 'spectated_code_rejects')),
        'simulation_locality_preserved': locality.get('local_cache_writes') == locality.get('image_writes') == 0,
    }
    return {
        'passed': all(checks.values()), 'pid': pid, 'checks': checks,
        'listener_count': current.get('count'),
        'special_listeners_preserved': current.get('special_preserved'),
        'audio_blueprint_calls_observed': locality.get('audio_frame_returns', 0),
        'audio_blueprint_coverage_observed': locality.get('audio_frame_returns', 0) > 0,
        'spectated_audio_calls_observed': locality.get('spectated_view_returns', 0),
        'spectated_audio_functions_bound': locality.get('spectated_audio_functions_bound', 0),
        'acoustic_acceptance': False,
        'scope': 'Original registry, ownership and routing diagnostics; actual listening is a separate check.',
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    parser.add_argument('--pid', type=int)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    result = audit(args.directory, args.pid)
    if args.output:
        args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(result, ensure_ascii=False))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
