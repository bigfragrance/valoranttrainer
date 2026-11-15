"""Prepare/inspect an exact child-bound empty-equipment marker; never launch a process."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path, PureWindowsPath
import re
import struct


MAGIC = 0x314250454D505459
VERSION = 1
SIZE = 2096
MAX_POLLS = 600
EXPECTED_EPOCH = 1
FORMAT = struct.Struct('<QIIIIII16s1024s1024s')
assert FORMAT.size == SIZE
ROLE_IDS = {'host': 1, 'client': 2}
ENV_MARKER = 'TRAINER_EMPTY_BOOTSTRAP_MARKER'
ENV_NONCE = 'TRAINER_EMPTY_BOOTSTRAP_NONCE'
MARKER_NAME = 'empty-equipment-bootstrap.bin'
FIELD = 'empty_equipment_bootstrap_probe'
PAIR_ROOT = PureWindowsPath(r'F:\valoranttrainer\.local\local-server')
ALLOWED_IMAGES = frozenset((
    r'F:\valoranttrainer\client\ShooterGame\Binaries\Win64\VALORANT-Win64-Shipping.exe',
))


def normalize_nonce(value):
    if (not isinstance(value, str) or not re.fullmatch(r'[0-9a-fA-F]{32}', value)
            or int(value, 16) == 0):
        raise ValueError('Empty bootstrap nonce must be exactly 32 hex characters and must not be all zero')
    return value.lower()


def _uint32(value, name):
    if type(value) is not int or not 1 <= value <= 0xffffffff:
        raise ValueError(name + ' must be a positive uint32')
    return value


def _f_path(value):
    text = str(value)
    if not text or '\x00' in text or any(char in text for char in '\r\n'):
        raise ValueError('Invalid Windows path')
    path = PureWindowsPath(text)
    if path.drive != 'F:' or not path.is_absolute() or text.startswith(('\\\\', '\\\\?\\')):
        raise ValueError('Probe paths must be absolute paths on F:')
    if any(part in ('.', '..') or part.endswith((' ', '.')) or ':' in part
           for part in path.parts[1:]):
        raise ValueError('Probe paths must not contain traversal, ADS or ambiguous components')
    # PureWindowsPath drops '.', so inspect raw components as well.
    if any(part in ('.', '..') for part in text.replace('/', '\\').split('\\')):
        raise ValueError('Probe paths must not contain traversal')
    return path


def image_path(value):
    path = _f_path(value)
    if str(path) not in ALLOWED_IMAGES:
        raise ValueError('Explicit empty bootstrap probe requires the exact copied client image')
    return str(path)


def pair_path(value):
    path = _f_path(value)
    if (path.parent != PAIR_ROOT or not re.fullmatch(r'pair[A-Za-z0-9_.-]+', path.name)
            or '..' in path.name[4:]):
        raise ValueError('Pair must be directly under the fixed F: local-server pair root')
    return str(path)


def role_output(value, role):
    if role not in ROLE_IDS:
        raise ValueError('Expected host or client role')
    path = _f_path(value)
    if path.name != role or pair_path(path.parent) != str(path.parent):
        raise ValueError('Output must be the exact pair role directory')
    return str(path)


def marker_path(value):
    path = _f_path(value)
    if path.name != MARKER_NAME or path.parent.name not in ROLE_IDS:
        raise ValueError('Unexpected marker name or role directory')
    role_output(path.parent, path.parent.name)
    return str(path)


def request(role, image, pair, nonce):
    if role not in ROLE_IDS:
        raise ValueError('Expected host or client role')
    pair = pair_path(pair)
    return {'role': role, 'image': image_path(image), 'pair': pair,
            'nonce': normalize_nonce(nonce),
            'marker': str(PureWindowsPath(pair) / role / MARKER_NAME)}


def _wide(value):
    data = value.encode('utf-16-le', errors='strict')
    if b'\x00\x00' == data[:2] or '\x00' in value or len(data) > 1022:
        raise ValueError('UTF16 path must fit 511 code units plus terminator')
    return data + b'\x00' * (1024 - len(data))


def _unwide(data):
    units = struct.unpack('<512H', data)
    try:
        end = units.index(0)
    except ValueError as error:
        raise ValueError('UTF16 path has no terminator') from error
    if end == 0 or any(units[end:]):
        raise ValueError('UTF16 path is empty or has nonzero padding')
    return data[:end * 2].decode('utf-16-le', errors='strict')


def encode_marker(*, role, pid, nonce, image, pair):
    expected = request(role, image, pair, nonce)
    return FORMAT.pack(MAGIC, VERSION, SIZE, ROLE_IDS[role], _uint32(pid, 'pid'),
                       MAX_POLLS, EXPECTED_EPOCH, bytes.fromhex(expected['nonce']),
                       _wide(expected['image']), _wide(expected['pair']))


def decode_marker(data):
    if not isinstance(data, bytes) or len(data) != SIZE:
        raise ValueError('Marker must contain exactly 2096 bytes')
    magic, version, size, role_id, pid, polls, epoch, nonce, image, pair = FORMAT.unpack(data)
    if (magic, version, size, polls, epoch) != (MAGIC, VERSION, SIZE, MAX_POLLS, EXPECTED_EPOCH):
        raise ValueError('Marker protocol header differs from the frozen POD')
    roles = {number: name for name, number in ROLE_IDS.items()}
    if role_id not in roles:
        raise ValueError('Unknown marker role')
    role = roles[role_id]
    expected = request(role, _unwide(image), _unwide(pair), nonce.hex())
    _uint32(pid, 'pid')
    return {**expected, 'pid': pid, 'magic': f'0x{magic:016x}', 'version': version,
            'bytes': size, 'role_id': role_id, 'max_polls': polls, 'expected_epoch': epoch}


def match_marker(data, *, role, pid, nonce, image, pair):
    result = decode_marker(data)
    expected = request(role, image, pair, nonce)
    expected['pid'] = _uint32(pid, 'pid')
    for key, value in expected.items():
        if result[key] != value:
            raise ValueError('Marker identity mismatch: ' + key)
    return result


def child_environment(base, probe_request=None):
    """A copy for the actual game Popen only; strip inherited probe variables."""
    names = {ENV_MARKER.casefold(), ENV_NONCE.casefold()}
    result = {key: value for key, value in base.items() if key.casefold() not in names}
    if probe_request is not None:
        expected = request(probe_request['role'], probe_request['image'],
                           probe_request['pair'], probe_request['nonce'])
        if probe_request != expected:
            raise ValueError('Probe request changed before child environment preparation')
        result[ENV_MARKER] = expected['marker']
        result[ENV_NONCE] = expected['nonce']
    return result


def validate_pair_request(nonce, *, interactive, original_host_mode, mode='training', scenario=None, config=None):
    if nonce is None:
        return None
    nonce = normalize_nonce(nonce)
    if not interactive or not original_host_mode or mode != 'training' or scenario is not None:
        raise ValueError('Empty bootstrap probe requires interactive original-host training without a native scenario')
    if config is not None and config.get('mode') != 'deathmatch':
        raise ValueError('Empty bootstrap probe requires the existing ordinary deathmatch configuration')
    return nonce


def validate_native_options(args):
    nonce = getattr(args, 'empty_bootstrap_probe_nonce', None)
    if nonce is None:
        return None
    nonce = normalize_nonce(nonce)
    role = args.network_probe
    if (role not in ROLE_IDS or args.network_flavor != 'playable' or args.mode != 'training'
            or not args.original_game_instance or not args.interactive_local_server
            or args.native_test_scenario is not None or args.controller_settings is not None
            or role == 'host' and not args.original_host_mode):
        raise ValueError('Empty bootstrap probe requires an explicit paired interactive original-host FFA runner without scenarios')
    return nonce


def require_probe_build(manifest):
    sources = _object(_object(manifest).get('source_sha256'))
    names = {PureWindowsPath(name).name for name in sources}
    if not {'local_server_empty_bootstrap_probe.h', 'empty_bootstrap_probe_core.h'} <= names:
        raise ValueError('Verified candidate build must include both empty-bootstrap probe headers')


def _existing_file(path):
    canonical = _f_path(path)
    target = Path(str(canonical))
    if PureWindowsPath(str(target.resolve(strict=True))) != canonical:
        raise ValueError('Probe evidence path resolves outside its exact F: scope')
    return target


def _unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('Duplicate cached JSON key: ' + key)
        result[key] = value
    return result


def _read_json(path):
    data = _existing_file(path).read_bytes()
    if len(data) > 1024 * 1024:
        raise ValueError('Cached probe JSON is too large')
    value = json.loads(data.decode('utf-8-sig'), object_pairs_hook=_unique_object)
    if not isinstance(value, dict):
        raise ValueError('Probe JSON must be an object')
    return value


def native_request(args, executable, *, read_json=None):
    nonce = validate_native_options(args)
    if nonce is None:
        return None
    output = role_output(args.network_output, args.network_probe)
    pair = str(PureWindowsPath(output).parent)
    expected = request(args.network_probe, executable, pair, nonce)
    read_json = _read_json if read_json is None else read_json
    session = read_json(str(PureWindowsPath(pair) / 'session.active.json'))
    bound = session.get(FIELD)
    if (type(session.get('schema_version')) is not int or session['schema_version'] != 1
            or session.get('kind') != 'native-shooter-candidate-interactive'
            or session.get('network_flavor') != 'playable' or session.get('original_host_mode') is not True
            or session.get('training_mode') != 'deathmatch' or session.get('engine_mode') != 'training'
            or pair_path(session.get('directory', '')) != pair
            or session.get('proof_id') != args.network_proof_id
            or not isinstance(bound, dict) or bound.get('requested') is not True
            or bound.get('nonce') != nonce or bound.get('scenario') is not None):
        raise ValueError('Pair metadata does not bind this ordinary FFA probe request')
    # Existing network preflight binds the client to this pair's exact owned host.
    if args.network_probe == 'client':
        if str(_f_path(args.network_peer)) != str(PureWindowsPath(pair) / 'host-result.active.json'):
            raise ValueError('Client peer must belong to this exact marker pair')
    return expected


def _write_exclusive(path, data):
    # Fail closed on reparse/redirection; create no pair directories or processes.
    target = Path(path)
    if not target.parent.is_dir() or str(target.parent.resolve()) != str(target.parent):
        raise ValueError('Role output must exist without path redirection')
    with target.open('xb') as stream:
        stream.write(data)


def arm_for_child(process, probe_request, *, writer=None):
    """Use only the actual returned game Popen.pid, never the Python runner PID."""
    expected = request(probe_request['role'], probe_request['image'],
                       probe_request['pair'], probe_request['nonce'])
    if probe_request != expected:
        raise ValueError('Probe request changed before marker publication')
    pid = _uint32(process.pid, 'actual game child pid')
    data = encode_marker(pid=pid, **{key: expected[key] for key in ('role', 'nonce', 'image', 'pair')})
    (_write_exclusive if writer is None else writer)(expected['marker'], data)
    return {**decode_marker(data), 'requested': True, 'marker_sha256': hashlib.sha256(data).hexdigest(),
            'pid_source': 'actual_game_popen.pid', 'native_probe_observed': False,
            'physical_input_verified': False}


def inspect_marker(path):
    path = marker_path(path)
    result = decode_marker(_existing_file(path).read_bytes())
    if result['marker'] != path:
        raise ValueError('Marker file path differs from its embedded pair/role')
    return result


def _integer(value, minimum=0, maximum=0xffffffffffffffff):
    return type(value) is int and minimum <= value <= maximum


def _pointer(value, nonzero=True):
    if not isinstance(value, str) or not re.fullmatch(r'(?:0x)?[0-9a-fA-F]{1,16}', value):
        return False
    return (int(value, 16) != 0) if nonzero else (int(value, 16) == 0)


def _weak(value):
    return (isinstance(value, list) and len(value) == 2
            and _integer(value[0], 0, 0x7fffffff) and _integer(value[1], 1, 0x7fffffff))


def _object(value):
    return value if isinstance(value, dict) else {}


def _finite(value):
    if type(value) not in (float, int):
        return False
    try:
        return math.isfinite(value)
    except OverflowError:
        return False


def check_cached_bridge(bridge, expected):
    """Recompute one role witness from cached POD; never resolve a live object."""
    expected = _object(expected)
    try:
        identity = request(expected['role'], expected['image'], expected['pair'], expected['nonce'])
        expected_bound = (all(expected.get(key) == value for key, value in identity.items())
                          and _uint32(expected['pid'], 'expected game pid') == expected['pid'])
    except (KeyError, ValueError, TypeError):
        expected_bound = False
    data = _object(_object(bridge).get(FIELD))
    owners = _object(data.get('owners'))
    inventory = _object(data.get('empty_inventory'))
    host = _object(data.get('host_input'))
    client = _object(data.get('client_source'))
    first = _object(host.get('first_batch'))
    role = expected.get('role')
    host_role = role == 'host'
    checks = {}
    def put(name, value):
        checks[name] = bool(value)
    put('expected_marker_identity', expected_bound)
    put('schema_version', type(data.get('schema_version')) is int and data['schema_version'] == 1)
    put('role_pid_nonce', type(data.get('role')) is int and data['role'] == ROLE_IDS.get(role)
        and type(data.get('pid')) is int and data['pid'] == expected.get('pid')
        and data.get('nonce') == expected.get('nonce'))
    for key in ('initialized', 'requested', 'armed'):
        put(key, data.get(key) is True)
    put('first_completed_bounded_receipt', data.get('observation_complete') is True
        and data.get('receipt_scope') == 'first_completed_observation_within_poll_bound_not_live_status')
    put('admission', data.get('admission') == 'explicit_owned_marker_armed')
    for key in ('expired', 'invalidated', 'scope_rejected', 'pair_acceptance'):
        put('native_' + key + '_false', data.get(key) is False)
    put('bounds', _integer(data.get('polls'), 1, MAX_POLLS)
        and type(data.get('max_polls')) is int and data['max_polls'] == MAX_POLLS
        and type(data.get('expected_epoch')) is int and data['expected_epoch'] == EXPECTED_EPOCH)
    put('host_only_pending', data.get('host_bound') is host_role
        and data.get('formal_publications_pending') is host_role)
    put('owner_pointers', all(_pointer(owners.get(key)) for key in
        ('world', 'pc', 'ps', 'pawn', 'connection', 'movement', 'prediction_allocation', 'clock'))
        and _pointer(inventory.get('inventory')))
    put('prediction_allocation_unchanged', _pointer(owners.get('first_prediction_allocation'))
        and _pointer(owners.get('prediction_allocation'))
        and int(owners['first_prediction_allocation'], 16) == int(owners['prediction_allocation'], 16))
    weak_names = ('pc_weak', 'ps_weak', 'pawn_weak', 'movement_weak', 'inventory_weak')
    put('owner_weak_identities', all(_weak(owners.get(key)) for key in weak_names))
    for key in ('observed', 'typed', 'owners', 'weak_verified', 'empty'):
        put('inventory_' + key, inventory.get(key) is True)
    put('host_slot_cache', inventory.get('host_original_slot_cache') is host_role)
    put('all_17_slots', type(inventory.get('slots_verified')) is int and inventory['slots_verified'] == 17
        and type(inventory.get('slots_empty')) is int and inventory['slots_empty'] == 17
        and type(inventory.get('slot_valid_mask')) is int and inventory['slot_valid_mask'] == (1 << 17) - 1
        and type(inventory.get('slot_empty_mask')) is int and inventory['slot_empty_mask'] == (1 << 17) - 1
        and _integer(inventory.get('multi_slots'), 0, 17))
    put('all_5_equipped_pointers_zero', all(_pointer(inventory.get(key), nonzero=False) for key in
        ('desired_268', 'pointer_278', 'current_280', 'pointer_2a0', 'corrected_320')))
    if host_role:
        put('host_scope', type(data.get('epoch')) is int and data['epoch'] == EXPECTED_EPOCH
            and data.get('scope_guard') == 'exact_owned_ffa_human_life1_baseline_registered'
            and _integer(data.get('scope_attempts'), 1))
        put('host_installed_owned_current', all(host.get(key) is True for key in
            ('installed', 'owned', 'current_prediction')))
        put('host_epoch_error', type(host.get('epoch')) is int and host['epoch'] == EXPECTED_EPOCH
            and type(host.get('error')) is int and host['error'] == 0)
        put('host_real_packet_counters', _integer(host.get('received_packets'), 1)
            and _integer(host.get('accepted_packets'), 1)
            and host['received_packets'] >= host['accepted_packets']
            and _integer(host.get('decoded_moves'), 1) and _integer(host.get('unique_moves'), 1))
        put('host_real_first_batch', first.get('captured') is True and _integer(first.get('packet'), 1)
            and _integer(first.get('count'), 1, 0xffffffff)
            and _integer(first.get('first'), 0, 0xffffffff) and _integer(first.get('last'), 0, 0xffffffff)
            and type(first.get('first_step')) is int and 0 <= first['first_step'] <= 1
            and _finite(first.get('time_shift')))
        put('host_original_apply_return', _integer(host.get('original_apply_attempts'), 1)
            and _integer(host.get('original_apply_returns'), 1)
            and host['original_apply_returns'] <= host['original_apply_attempts'])
        put('host_real_physics_clock', _integer(host.get('physics_completed'), 1)
            and _integer(host.get('clock_index'), 1, 0xffffffff)
            and type(host.get('clock_step')) is int and host['clock_step'] == 1
            and type(host.get('last_consumed_index')) is int
            and host['clock_index'] == host['last_consumed_index'])
        put('declared_host_witness_consistent', data.get('host_witness') is True and data.get('client_witness') is False)
    else:
        for key in ('ready', 'current_prediction', 'local_owner', 'clock_observed', 'roundtrip'):
            put('client_' + key, client.get(key) is True)
        put('client_exact_source_mask', type(client.get('passed_mask')) is int and client['passed_mask'] == 1023
            and type(client.get('required_mask')) is int and client['required_mask'] == 1023
            and type(client.get('weak_failure_index')) is int and client['weak_failure_index'] == -1
            and _pointer(client.get('pc_table')))
        identities = client.get('identities')
        put('client_exact_owner_weak_lifecycle', isinstance(identities, list) and len(identities) == 12
            and all(_weak(item) for item in identities)
            and all(owners.get(key) == identities[index] for key, index in
                (('pc_weak', 1), ('ps_weak', 2), ('pawn_weak', 3), ('movement_weak', 10), ('inventory_weak', 11))))
        put('client_clock_types', _integer(client.get('raw_index'), 0, 0xffffffff)
            and _integer(client.get('timestep'), -0x80000000, 0x7fffffff))
        # Client adapter does not capture an epoch; expected_epoch plus exact
        # local owner Weak identities is the supported lifecycle scope.
        put('declared_client_witness_consistent', data.get('client_witness') is True and data.get('host_witness') is False)
    witnessed = bool(checks) and all(checks.values())
    return {'passed': False, 'pair_acceptance': False,
            'status': 'single_role_witness_peer_required' if witnessed else 'cached_witness_rejected',
            'field': FIELD, 'role': role, 'pid': expected.get('pid'), 'nonce': expected.get('nonce'),
            'witness': witnessed, 'checks': checks,
            'failed_checks': [key for key, value in checks.items() if not value],
            'receipt_scope': 'first_completed_observation_within_poll_bound_not_live_status',
            'live_objects_read': False, 'physical_input_verified': False, 'playable_verified': False}


def check_pair(host_bridge, client_bridge, host_marker, client_marker):
    host = check_cached_bridge(host_bridge, host_marker)
    client = check_cached_bridge(client_bridge, client_marker)
    bound = (host_marker.get('role') == 'host' and client_marker.get('role') == 'client'
             and host_marker.get('pair') == client_marker.get('pair')
             and host_marker.get('nonce') == client_marker.get('nonce')
             and host_marker.get('pid') != client_marker.get('pid'))
    passed = bool(bound and host['witness'] and client['witness'])
    return {'passed': passed, 'pair_acceptance': passed,
            'status': 'paired_cached_witnesses_verified' if passed else 'paired_cached_witnesses_rejected',
            'pair_marker_identities_bound': bool(bound), 'host': host, 'client': client,
            'receipt_scope': 'first_completed_observation_within_poll_bound_not_live_status',
            'live_objects_read': False, 'physical_input_verified': False, 'playable_verified': False}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='operation', required=True)
    arm = sub.add_parser('arm', help='Only write a marker; never start or inspect a process')
    inspect = sub.add_parser('inspect', help='Read and validate a bounded marker')
    check = sub.add_parser('check', help='Read marker/cached bridge only; no process access')
    inspect.add_argument('--marker', required=True)
    check.add_argument('--marker', required=True)
    check.add_argument('--bridge', required=True)
    check.add_argument('--peer-marker')
    check.add_argument('--peer-bridge')
    check.add_argument('--peer-pid', type=int)
    check.add_argument('--peer-image')
    for command in (arm, check):
        command.add_argument('--role', required=True, choices=tuple(ROLE_IDS))
        command.add_argument('--pid', required=True, type=int)
        command.add_argument('--nonce', required=True, type=normalize_nonce)
        command.add_argument('--image', required=True)
        command.add_argument('--pair', required=True)
    args = parser.parse_args(argv)
    try:
        if args.operation == 'inspect':
            result = inspect_marker(args.marker)
        else:
            expected = request(args.role, args.image, args.pair, args.nonce)
            if args.operation == 'arm':
                class ExplicitChildIdentity:
                    pid = args.pid
                result = arm_for_child(ExplicitChildIdentity(), expected)
                result['pid_source'] = 'explicit_marker_tool_argument'
                result['owned_child_verified'] = False
            else:
                actual_path = marker_path(args.marker)
                if actual_path != expected['marker']:
                    raise ValueError('Requested marker is outside the exact role output')
                checked_marker = match_marker(_existing_file(actual_path).read_bytes(), pid=args.pid,
                    **{key: expected[key] for key in ('role', 'nonce', 'image', 'pair')})
                bridge = _f_path(args.bridge)
                if bridge != PureWindowsPath(expected['pair']) / args.role / 'bridge-state.json':
                    raise ValueError('Cached bridge must be this exact role output bridge-state.json')
                own_bridge = _read_json(str(bridge))
                result = check_cached_bridge(own_bridge, checked_marker)
                peer_options = (args.peer_marker, args.peer_bridge, args.peer_pid)
                if any(value is not None for value in peer_options) or args.peer_image:
                    if not all(value is not None for value in peer_options):
                        raise ValueError('Paired check requires peer marker, bridge and actual game pid')
                    peer_role = 'client' if args.role == 'host' else 'host'
                    peer = request(peer_role, args.peer_image or args.image, args.pair, args.nonce)
                    if marker_path(args.peer_marker) != peer['marker']:
                        raise ValueError('Peer marker is outside this exact pair/role')
                    peer_marker = match_marker(_existing_file(args.peer_marker).read_bytes(), pid=args.peer_pid,
                        **{key: peer[key] for key in ('role', 'nonce', 'image', 'pair')})
                    peer_bridge = _f_path(args.peer_bridge)
                    if peer_bridge != PureWindowsPath(args.pair) / peer_role / 'bridge-state.json':
                        raise ValueError('Peer cached bridge must belong to this exact pair/role')
                    peer_data = _read_json(str(peer_bridge))
                    result = (check_pair(own_bridge, peer_data, checked_marker, peer_marker) if args.role == 'host'
                              else check_pair(peer_data, own_bridge, peer_marker, checked_marker))
        print(json.dumps(result, ensure_ascii=False, indent=2))
        return 0 if args.operation != 'check' or result['passed'] else 2
    except (OSError, ValueError, UnicodeError) as error:
        print(json.dumps({'passed': False, 'error': str(error)}, ensure_ascii=False))
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
