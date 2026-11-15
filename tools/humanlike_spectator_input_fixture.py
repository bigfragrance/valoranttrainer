"""Explicit scenario15 only. Marker + client proof + bounded host correlation.

No launch, input, camera, configuration, socket or runtime dependency is created
by importing this module. Samples alone cannot identify an individual RPC packet.
"""
from __future__ import annotations
import struct

SCENARIO = 'spectator-input-fixture'
MAXIMUM_NATIVE_MS = 45000


def require_build(build):
    required = {'native/local_server_native_spectator_input.inl', 'native/original_input_bindings.h',
                'native/local_server_native_test.h', 'native/local_server_native_shop_dispatch.inl'}
    if not required <= set(build.get('source_sha256', {})):
        raise RuntimeError('Candidate module lacks explicit Scenario15 original-key fixture source closure')


def marker(nonce, bind_index=0):
    if type(nonce) is not int or not 0 < nonce < 2**64:
        raise ValueError('Spectator input marker requires a positive uint64 nonce')
    if type(bind_index) is not int or not 0 <= bind_index <= 7:
        raise ValueError('Native action bind index must be an integer from 0 to 7')
    return struct.pack('<Q4IQ8I', 0x31545345544E534C, 1, 64, 1, 15, nonce,
                       500, 0, 10000, MAXIMUM_NATIVE_MS, bind_index, 0, 0, 0)


def policy(config, seconds):
    if (config.get('mode') != 'deathmatch' or config.get('paused') is not False
            or type(config.get('total_players')) is not int or not 3 <= config['total_players'] <= 32):
        raise ValueError('spectator-input-fixture requires explicit unpaused original deathmatch with at least two bots; settings are not changed')
    return min(seconds, 120)


def observation(client, nonce):
    fixture = client.get('native_integration_test', {}) if isinstance(client, dict) else {}
    exact = bool(isinstance(fixture, dict) and type(fixture.get('scenario')) is int and fixture['scenario'] == 15
                 and type(fixture.get('nonce')) is int and fixture['nonce'] == nonce
                 and fixture.get('configured') is True and fixture.get('cli_matched') is True and fixture.get('armed') is True)
    proof = fixture.get('spectator_input', {}) if exact else {}
    receipts = proof.get('receipts', []) if isinstance(proof, dict) else []
    receipt_checks = []
    def weak(value):
        return bool(isinstance(value, list) and len(value) == 2 and all(type(item) is int for item in value)
                    and value[0] >= 0 and value[1] > 0)
    for index in range(2):
        r = receipts[index] if isinstance(receipts, list) and len(receipts) == 2 else {}
        integer_fields = ('direction', 'bind_index', 'down_calls', 'up_calls', 'request_before', 'request_after', 'ack_before', 'ack_after',
                          'pressed_at_ms', 'completed_at_ms', 'camera_sample_at_ms', 'from_id', 'to_id')
        valid = bool(isinstance(r, dict) and all(type(r.get(key)) is int for key in integer_fields))
        if valid:
            valid = bool(r['down_calls'] == r['up_calls'] == 1 and r.get('direction') == index
                         and r.get('action') == ('SecondaryTriggerDown' if index else 'PrimaryTriggerDown')
                         and all(r.get(key) is True for key in ('down_dispatched', 'up_dispatched', 'release_observed',
                                                               'request_observed', 'ack_observed', 'camera_observed'))
                         and r['request_before'] >= 0 and r['request_after'] == r['request_before'] + 1
                         and r['ack_before'] == r['request_before'] and r['ack_after'] > r['ack_before']
                         and r['ack_after'] >= r['request_after']
                         and 0 < r['pressed_at_ms'] < r['camera_sample_at_ms'] <= r['completed_at_ms']
                         and r['completed_at_ms'] - r['pressed_at_ms'] < 10000
                         and r['from_id'] > 0 and r['to_id'] > 0 and r['from_id'] != r['to_id']
                         and 0 <= r['bind_index'] <= 7 and r.get('modifiers') == [0, 0, 0, 0, 0]
                         and type(r.get('tap_hold_type')) is int and r['tap_hold_type'] == 0
                         and weak(r.get('from_pawn_weak')) and weak(r.get('to_pawn_weak')))
        receipt_checks.append(valid)
    relation = bool(all(receipt_checks) and receipts[0]['from_id'] == receipts[1]['to_id']
                    and receipts[0]['to_id'] == receipts[1]['from_id']
                    and receipts[1]['pressed_at_ms'] >= receipts[0]['completed_at_ms'] + 500
                    and receipts[0].get('from_pawn_weak') == receipts[1].get('to_pawn_weak'))
    relation = bool(relation and receipts[0].get('to_pawn_weak') == receipts[1].get('from_pawn_weak'))
    completed = bool(exact and fixture.get('finished') is True and fixture.get('failed') is False
                     and type(fixture.get('error')) is int and fixture['error'] == 0
                     and fixture.get('actions_completed') is True and isinstance(proof, dict)
                     and proof.get('bound') is True and proof.get('metadata_verified') is True
                     and proof.get('release_unavailable') is False and proof.get('skipped_or_not_observed') is False
                     and proof.get('key_source_rva') == '0x708d250' and proof.get('shared_release_rva') == '0x18a31e0'
                     and proof.get('direct_request_calls') == 0 and proof.get('camera_updates') == 0
                     and proof.get('target_writes') == 0 and relation)
    return {'scenario': SCENARIO, 'scope': 'client_original_key_ack_cached_camera_only',
            'exact_marker_observed': exact, 'finished': bool(exact and fixture.get('finished') is True),
            'input_actions_completed': completed, 'client_key_ack_camera_verified': completed,
            'host_rpc_counter_windows_verified': False, 'per_packet_rpc_causality_verified': False,
            'physical_os_input_verified': False, 'playable_verified': False,
            'client': fixture if exact else {}}


def correlate(samples, nonce, final_client=None):
    """Require one native player-input RPC/selection/direction window per click.

    Baseline must be sampled before that click, with this exact marker. Missing
    rows/counters, multiple requests or unrelated activity leave this gate false.
    This is temporal counter association, not a packet identity/nonce receipt.
    """
    rows = [row for row in samples if isinstance(row, dict)]
    client = final_client if isinstance(final_client, dict) else (rows[-1].get('client', {}) if rows else {})
    result = observation(client, nonce)
    windows = []
    fields = ('original_server_calls', 'server_selections', 'player_input_requests')
    for step in range(2):
        before = after = None
        for row in rows:
            sample = observation(row.get('client', {}), nonce)
            if not sample['exact_marker_observed']:
                continue
            fixture = sample['client'].get('spectator_input', {})
            receipts = fixture.get('receipts', []) if isinstance(fixture, dict) else []
            if not isinstance(receipts, list) or len(receipts) != 2 or not isinstance(receipts[step], dict):
                continue
            receipt = receipts[step]
            # Native bridge samples nest the authoritative policy state here;
            # the client's spectator view is a different, read-only record.
            host_sample = row.get('host', {})
            runtime = host_sample.get('session_runtime', {}) if isinstance(host_sample, dict) else {}
            host = runtime.get('spectator_authority', {}) if isinstance(runtime, dict) else {}
            if not isinstance(host, dict):
                continue
            if type(receipt.get('pressed_at_ms')) is int and receipt['pressed_at_ms'] == 0:
                before = host
            elif receipt.get('camera_observed') is True:
                after = host
                break
        valid = bool(before is not None and after is not None
                     and all(type(value.get(field)) is int and value[field] >= 0 for value in (before, after) for field in fields)
                     and all(value.get('server_policy_installed') is True and value.get('server_policy_profiles') is True
                             for value in (before, after)))
        deltas = {field: after[field] - before[field] for field in fields} if valid else None
        valid = bool(valid and all(delta == 1 for delta in deltas.values()) and after.get('last_direction') == step)
        windows.append({'direction': step, 'sampled_baseline_before_key': before is not None,
                        'sampled_after_camera': after is not None, 'deltas': deltas, 'verified': valid})
    result['host_rpc_counter_windows'] = windows
    result['host_rpc_counter_windows_verified'] = bool(result['client_key_ack_camera_verified']
                                                     and all(window['verified'] for window in windows))
    return result
