"""Compare cached native host positions with the real client's remote Pawns.

This observer reads existing diagnostic files only. PlayerId joins the native
PlayerStates; Pawn pointers join the client's timeline and render observations.
Z is excluded because the host exposes feet and the client exposes Actor center.
No movement, clock, input, health or match state is changed.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import time

from local_server_sample_evidence import iter_full_samples


def read(path):
    try:
        return json.loads(path.read_text(encoding='utf-8'))
    except (OSError, ValueError):
        return {}


def observe(pair):
    host = read(pair / 'host/bridge-state.json')
    client = read(pair / 'client/bridge-state.json')
    result = compare(host, client)
    now = time.time()
    for side in ('host', 'client'):
        path = pair / side / 'bridge-state.json'
        result[side + '_file_age_seconds'] = now - path.stat().st_mtime if path.exists() else None
    return result


def compare(host, client):
    match = host.get('session_runtime', {}).get('match', {})
    timeline = client.get('remote_timeline_probe', {})
    render = client.get('remote_render_probe', {})
    manager = timeline.get('shared_manager', {})
    rows = {r.get('pawn'): r for r in render.get('characters', [])}
    players = {p.get('player_id'): p for p in match.get('players', [])}
    npc = {p.get('index'): p for p in host.get('npc_authority', {}).get('entries', [])}
    host_at = host.get('npc_authority', {}).get('frame_snapshot_at_ms', 0)
    client_at = timeline.get('sampled_at_ms', 0)
    result = {
        'observed_at': datetime.now(timezone.utc).isoformat(),
        'host_error': host.get('error'), 'client_error': client.get('error'),
        'host_stage': host.get('stage'), 'client_stage': client.get('stage'),
        'match_active': match.get('active'), 'match_ended': match.get('ended'),
        'kills': match.get('kills'), 'human_respawns': match.get('human_respawns'),
        'host_cache_at_ms': host_at, 'client_cache_at_ms': client_at,
        'cache_skew_ms': abs(host_at - client_at) if host_at and client_at else None,
        'client_world_time': timeline.get('world_time740'),
        'manager_index': manager.get('current_server_index104'),
        'manager_rebases': manager.get('native_num_rebases_e0'),
        'host_common_clock_stage': host.get('authority_clock', {}).get('stage'),
        'host_common_clock_index': host.get('authority_clock', {}).get('raw_index'),
        'rows': [],
    }
    for row in timeline.get('characters', []):
        player = players.get(row.get('player_id'))
        visual = rows.get(row.get('actor', {}).get('object'))
        if not player or not visual:
            continue
        feet, center = player.get('feet'), visual.get('pawn_position')
        if not feet or not center:
            continue
        clock = row.get('clock', {})
        entry = npc.get(player.get('index'), {})
        result['rows'].append({
            'player_id': row.get('player_id'), 'host_epoch': player.get('epoch'),
            'host_alive': player.get('alive'),
            'client_pawn_weak': row.get('pawn_weak'),
            'host_feet': feet, 'client_actor_center': center,
            'xy_error_cm': round(math.hypot(feet[0] - center[0], feet[1] - center[1]), 3),
            'host_physical_index': entry.get('clock_index'),
            'host_public_index': entry.get('public_wire_index'),
            'host_public_target_index': entry.get('public_target_index'),
            'host_public_unserved_steps': entry.get('public_unserved_steps'),
            'client_physical_index': clock.get('raw_index'),
            'client_server_index': clock.get('server_domain_index'),
            'client_history_initialized': clock.get('initialized4d8'),
            'client_history_count': clock.get('history_count4d0'),
            'client_stream_processing': visual.get('stream_processing_e0'),
            'client_stream_registered': visual.get('movement_stream1080'),
            'client_incoming_count': row.get('incoming_queue', {}).get('queued_count14a0'),
        })
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('pair', type=Path)
    parser.add_argument('--seconds', type=float, default=0)
    parser.add_argument('--interval', type=float, default=1)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--recorded', action='store_true',
                        help='Read the existing verification.json sample history instead of watching')
    args = parser.parse_args()
    if args.seconds < 0 or args.seconds > 720 or not .1 <= args.interval <= 30:
        parser.error('Expected a bounded 0..720s observation and a .1..30s interval')
    samples = []
    if args.recorded:
        report = read(args.pair / 'verification.json')
        for recorded in iter_full_samples(report, args.pair):
            sample = compare(recorded.get('host', {}), recorded.get('client', {}))
            sample['recorded_elapsed_seconds'] = recorded.get('elapsed')
            sample.pop('observed_at', None)
            samples.append(sample)
    deadline = time.monotonic() + args.seconds
    while not args.recorded:
        sample = observe(args.pair)
        samples.append(sample)
        active = sample['match_active'] and not sample['match_ended']
        errors = [r['xy_error_cm'] for r in sample['rows'] if r['host_alive']]
        histories = sum(bool(r['client_history_initialized']) for r in sample['rows'])
        print(json.dumps({'active': bool(active), 'rows': len(sample['rows']),
                          'max_xy_cm': max(errors, default=None), 'histories': histories,
                          'rebases': sample['manager_rebases'],
                          'cache_skew_ms': sample['cache_skew_ms']}, ensure_ascii=False), flush=True)
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            break
        time.sleep(min(args.interval, remaining))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps({
        'schema_version': 1, 'pair': str(args.pair.resolve()),
        'scope': 'cached native PlayerId and client Pawn joins; XY only',
        'same_epoch_not_proven_by_player_id': True,
        'physical_input_or_hit_acceptance_claimed': False, 'samples': samples,
    }, ensure_ascii=False, indent=2), encoding='utf-8')


if __name__ == '__main__':
    main()
