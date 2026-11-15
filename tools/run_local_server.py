"""Run a bounded loopback ShooterGame host/client candidate session.

The report separates Shooter world/hero lifecycle evidence from a playable
training verdict. Input authority, weapons, abilities and presentation require
their own evidence; this runner never marks playable_verified true.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess
import struct
import sys
import time
import uuid

from local_environment import project_environment
import humanlike_peek_fixture as peek_fixture
import humanlike_map_hold_fixture as hold_fixture
import humanlike_spectator_input_fixture as spectator_input_fixture
from empty_bootstrap_probe import normalize_nonce, validate_pair_request, require_probe_build
from local_network_probe import analyze_connection_samples
from local_server_sample_evidence import SampleHistory, compact_progress
from local_server_module import ROOT, BUILD, MODULE_NAME, FLAVOR, verified_build

PROTECTED_ARTIFACTS = (
    '.local/native-module/trainer_bridge.dll', '.local/native-module/build.json',
    '.local/local-network/module/network_probe.dll', '.local/local-network/module/build.json',
)
DEFAULT_BOOTSTRAP = ROOT / '.local/map-probe/20261101T020214.576649Z/map-1.bin'
JETT_SCENARIOS = ('jett-c', 'jett-q', 'jett-e', 'jett-x', 'jett-x-target')
NATIVE_TEST_SCENARIOS = ('shoot', 'shop', 'shoot-target', 'ffa-battle', 'phoenix-c-primary') + JETT_SCENARIOS + ('shop-correction-prelude', 'peek-fixture', 'map-hold-fixture', 'spectator-input-fixture')
FFA_BATTLE_NATIVE_MS = 660000  # Original 540s match plus 120s setup/respawn grace.
FFA_BATTLE_PAIR_SECONDS = 720  # Includes the owned host's bounded startup.
PHOENIX_C_NATIVE_MS = 45000
PHOENIX_C_PAIR_SECONDS = 120


def native_test_marker(scenario, nonce):
    """The fixed 64-byte opt-in marker; no controller configuration is changed."""
    if scenario not in NATIVE_TEST_SCENARIOS:
        raise ValueError('Unknown explicit native integration scenario')
    if type(nonce) is not int or not 0 < nonce < 2**64:
        raise ValueError('Native integration marker requires a nonzero uint64 nonce')
    if scenario == spectator_input_fixture.SCENARIO:
        return spectator_input_fixture.marker(nonce)
    if scenario == 'shop-correction-prelude':
        return struct.pack('<Q4IQ8I', 0x31545345544E534C, 1, 64, 1, 12, nonce,
                           500, 500, 30000, 45000, 0, 0, 0, 0)
    if scenario == 'peek-fixture':
        return peek_fixture.marker(nonce)
    if scenario == 'map-hold-fixture':
        return hold_fixture.marker(nonce)
    if scenario in JETT_SCENARIOS:
        return struct.pack('<Q4IQ8I', 0x31545345544E534C, 1, 64, 1, 6 + JETT_SCENARIOS.index(scenario), nonce,
                           500, 500, 30000, PHOENIX_C_NATIVE_MS, 0, 0, 0, 0)
    if scenario == 'phoenix-c-primary':
        return struct.pack('<Q4IQ8I', 0x31545345544E534C, 1, 64, 1, 5, nonce,
                           500, 500, 30000, PHOENIX_C_NATIVE_MS, 0, 0, 0, 0)
    return struct.pack('<Q4IQ8I', 0x31545345544E534C, 1, 64, 1,
                       NATIVE_TEST_SCENARIOS.index(scenario) + 1, nonce,
                       500, 500, 15000 if scenario == 'ffa-battle' else 10000,
                       FFA_BATTLE_NATIVE_MS if scenario == 'ffa-battle' else 20000,
                       0, 0, 0, 0)


def native_test_policy(scenario, config, seconds):
    """Validate the requested fixture against the existing, unmodified settings."""
    if scenario is not None and scenario not in NATIVE_TEST_SCENARIOS:
        raise ValueError('Unknown explicit native integration scenario')
    if scenario == spectator_input_fixture.SCENARIO:
        return spectator_input_fixture.policy(config, seconds)
    if scenario == 'shop-correction-prelude':
        if config.get('mode') not in ('deathmatch', 'practice') or config.get('paused') is not False:
            raise ValueError('shop-correction-prelude requires existing unpaused offline practice/deathmatch; settings are not changed')
        return min(seconds, 120)
    if scenario == 'map-hold-fixture':
        if config.get('mode') != 'deathmatch' or config.get('paused') is not False or config.get('total_players') != 2:
            raise ValueError('map-hold-fixture requires explicit two-person original deathmatch')
        return min(seconds, 120)
    if scenario == 'peek-fixture':
        if config.get('mode') != 'deathmatch' or config.get('total_players') != 2 or config.get('invincible') is not False or config.get('infinite_ammo') is not False or config.get('paused') is not False:
            raise ValueError('peek-fixture requires explicit finite two-person ordinary deathmatch')
        return min(seconds, 90)
    if scenario in JETT_SCENARIOS:
        from training_heroes import selected_hero
        hero, _ = selected_hero(config.get('local_hero_id'))
        if config.get('mode') != 'practice' or config.get('paused') is not False or hero['index'] != 0:
            raise ValueError('Jett fixtures require existing unpaused practice with Jett; no settings are changed')
        if scenario == 'jett-x-target' and (config.get('invincible') is not False
                or type(config.get('total_players')) is not int or config['total_players'] < 2):
            raise ValueError('jett-x-target requires an existing non-invincible practice with at least two players')
        return min(seconds, PHOENIX_C_PAIR_SECONDS)
    if scenario == 'phoenix-c-primary':
        from training_heroes import selected_hero
        hero, _ = selected_hero(config.get('local_hero_id'))
        if config.get('mode') != 'practice' or config.get('paused') is not False or hero['index'] != 1:
            raise ValueError('phoenix-c-primary requires existing unpaused practice with Phoenix; no settings are changed')
        return min(seconds, PHOENIX_C_PAIR_SECONDS)
    if scenario != 'ffa-battle':
        return seconds
    if (config.get('mode') != 'deathmatch' or config.get('infinite_ammo') is not False
            or config.get('invincible') is not False or config.get('paused') is not False
            or type(config.get('total_players')) is not int or config['total_players'] < 2):
        raise ValueError('ffa-battle requires an existing deathmatch configuration with finite ammo, '
                         'no invincibility, no pause and at least two players; settings are not changed')
    return min(seconds, FFA_BATTLE_PAIR_SECONDS)


def native_fixture_observation(client, scenario, nonce):
    """Client input evidence only; host match correctness remains unverified."""
    if scenario == 'peek-fixture':
        return peek_fixture.observation(client, nonce)
    if scenario == 'map-hold-fixture':
        return hold_fixture.observation(client, nonce)
    if scenario == spectator_input_fixture.SCENARIO:
        return spectator_input_fixture.observation(client, nonce)
    fixture = client.get('native_integration_test', {})
    if scenario == 'shop-correction-prelude':
        exact = bool(isinstance(fixture, dict) and type(fixture.get('scenario')) is int and fixture['scenario'] == 12
                     and type(fixture.get('nonce')) is int and fixture['nonce'] == nonce
                     and fixture.get('configured') is True and fixture.get('cli_matched') is True and fixture.get('armed') is True)
        receipt = fixture.get('shop_prelude', {}) if exact else {}
        completed = bool(exact and fixture.get('finished') is True and fixture.get('failed') is False
                         and type(fixture.get('error')) is int and fixture['error'] == 0 and fixture.get('actions_completed') is True
                         and isinstance(receipt, dict) and receipt.get('not_observed') is False
                         and receipt.get('snapshot_published') is True and receipt.get('snapshot_exit') in ('tick', 'stop')
                         and receipt.get('at_purchase_receipt_captured') is True and receipt.get('purchase_return_observed') is True
                         and receipt.get('methods_verified') is True and receipt.get('owner_bound') is True and receipt.get('model_ready') is True
                         and receipt.get('purchase_submitted') is True and receipt.get('original_purchase_returned_true') is True
                         and receipt.get('at_purchase_transition_proven') is True and receipt.get('release_unavailable') is False
                         and type(receipt.get('at_purchase_pending_count')) is int and 3 <= receipt['at_purchase_pending_count'] <= 8192
                         and type(receipt.get('at_purchase_old_client_index')) is int and receipt['at_purchase_old_client_index'] >= 0
                         and type(receipt.get('at_purchase_preamble_owned_slot')) is int and receipt['at_purchase_preamble_owned_slot'] in (0, 1, 2)
                         and receipt.get('parameter_mismatch_masks') == [0, 0, 0, 0]
                         and type(receipt.get('switch_native_two_edge_queue_proofs')) is int
                         and receipt['switch_native_two_edge_queue_proofs'] >= 3)
        return {'scope': 'client_original_shop_prelude_fixture_only', 'exact_marker_observed': exact,
                'finished': bool(exact and fixture.get('finished') is True), 'input_actions_completed': completed,
                'not_observed': bool(exact and receipt.get('not_observed') is True), 'scenario': scenario,
                'client': fixture if exact else {}, 'host_result_verified': False,
                'bug_reproduction_verified': False, 'physical_input_verified': False, 'playable_verified': False}
    if scenario in JETT_SCENARIOS:
        exact = bool(isinstance(fixture, dict) and type(fixture.get('scenario')) is int
                     and fixture['scenario'] == 6 + JETT_SCENARIOS.index(scenario)
                     and type(fixture.get('nonce')) is int and fixture['nonce'] == nonce
                     and fixture.get('configured') is True and fixture.get('cli_matched') is True
                     and fixture.get('armed') is True)
        skill = fixture.get('jett_skill', {}) if exact else {}
        receipt = fixture.get('jett_input_receipt', {}) if exact else {}
        target = fixture.get('jett_target', {}) if exact else {}
        presses = 2 if scenario == 'jett-e' else 1
        completed = bool(exact and fixture.get('finished') is True and fixture.get('failed') is False
                         and type(fixture.get('error')) is int and fixture['error'] == 0
                         and fixture.get('actions_completed') is True and isinstance(skill, dict)
                         and isinstance(receipt, dict) and receipt.get('fields_verified') is True and receipt.get('pending') is False
                         and skill.get('bound') is True and skill.get('input_complete') is True
                         and type(skill.get('epoch')) is int and skill['epoch'] > 0
                         and type(skill.get('registration')) is int and skill['registration'] > 0
                         and type(skill.get('activation_press_readbacks')) is int and skill['activation_press_readbacks'] == presses
                         and type(skill.get('activation_release_readbacks')) is int and skill['activation_release_readbacks'] == presses
                         and type(skill.get('activation_now')) is int and skill['activation_now'] == 0 and skill.get('activation_held') is False
                         and skill.get('private_intent_retired') is False and skill.get('release_unavailable') is False
                         and (scenario != 'jett-e' or skill.get('primed_observed') is True)
                         and (scenario not in ('jett-x', 'jett-x-target') or skill.get('x_equipped_observed') is True and skill.get('primary_press_observed') is True
                              and skill.get('primary_release_observed') is True)
                         and (scenario != 'jett-x-target' or isinstance(target, dict)
                              and target.get('enabled') is True and target.get('enemy_metadata_verified') is True
                              and target.get('field_reject_mask') == 0 and target.get('pre_primary_ready') is True
                              and target.get('enemy_at_press') is True
                              and type(target.get('rotation_readbacks_at_press')) is int and target['rotation_readbacks_at_press'] > 0))
        return {'scope': 'client_original_jett_input_fixture_only', 'exact_marker_observed': exact,
                'finished': bool(exact and fixture.get('finished') is True), 'input_actions_completed': completed,
                'scenario': scenario, 'client': fixture if exact else {}, 'host_result_verified': False,
                'skill_cast_verified': False, 'physical_input_verified': False, 'playable_verified': False}
    if scenario == 'phoenix-c-primary':
        exact = bool(isinstance(fixture, dict) and type(fixture.get('scenario')) is int and fixture['scenario'] == 5
                     and type(fixture.get('nonce')) is int and fixture['nonce'] == nonce
                     and fixture.get('configured') is True and fixture.get('cli_matched') is True and fixture.get('armed') is True)
        skill = fixture.get('phoenix_c_primary', {}) if exact else {}
        return {'scope': 'client_native_primary_input_fixture_only', 'exact_marker_observed': exact,
                'finished': bool(exact and fixture.get('finished') is True),
                'input_actions_completed': bool(exact and fixture.get('actions_completed') is True
                                                and type(fixture.get('error')) is int and fixture['error'] == 0
                                                and fixture.get('failed') is False
                                                and fixture.get('held_observed') is True and fixture.get('release_observed') is True
                                                and isinstance(skill, dict) and skill.get('selection_bound') is True),
                'scenario': scenario, 'client': fixture if exact else {},
                'host_result_verified': False, 'skill_cast_verified': False,
                'physical_input_verified': False, 'playable_verified': False}

    exact = bool(scenario == 'ffa-battle' and isinstance(fixture, dict)
                 and type(fixture.get('scenario')) is int and fixture['scenario'] == 4
                 and type(fixture.get('nonce')) is int and fixture['nonce'] == nonce and fixture.get('configured') is True
                 and fixture.get('cli_matched') is True and fixture.get('armed') is True)
    battle = fixture.get('ffa_battle', {}) if exact else {}
    return {'scope': 'client_native_input_fixture_only', 'exact_marker_observed': exact,
            'finished': bool(exact and fixture.get('finished') is True),
            'input_actions_completed': bool(exact and fixture.get('actions_completed') is True
                                            and type(fixture.get('error')) is int and fixture['error'] == 0
                                            and fixture.get('failed') is False),
            'match_end_observed': bool(exact and isinstance(battle, dict)
                                       and battle.get('match_end_observed') is True),
            'scenario': scenario, 'client': fixture if exact else {},
            'host_result_verified': False, 'full_match_acceptance_verified': False,
            'physical_input_verified': False, 'playable_verified': False}


def read(path):
    try:
        value = json.loads(Path(path).read_text(encoding='utf-8'))
        return value if isinstance(value, dict) else {}
    except (OSError, ValueError):
        return {}


def save(path, value):
    path = Path(path)
    temporary = path.with_name(path.name + '.tmp')
    temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding='utf-8')
    for attempt in range(5):
        try:
            temporary.replace(path)
            return
        except PermissionError:
            if attempt == 4:
                raise
            time.sleep(.01)


def artifact_hashes():
    return {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in PROTECTED_ARTIFACTS}


def progress_snapshot(report):
    """Publish current state without repeatedly encoding the complete history.

    Explicit fixtures retain their original full inline history. Ordinary
    interactive sessions publish a gate projection and retain every full
    sampled pair in their versioned, bounded lossless archive.
    """
    if report.get('sample_evidence') is not None:
        return compact_progress(report)
    samples = report.get('samples', [])
    return {**report, 'samples': samples[-1:], 'progress_samples_scope': 'latest_pair_snapshot',
            'history_samples_retained_for_verification': len(samples)}


def host_ready(state, port):
    """A bound driver precedes host runtime/login readiness during initialization."""
    return bool(state.get('role') == 'host' and state.get('port') == port
                and state.get('listen_returned') is True and state.get('loopback_candidates_verified') is True
                and state.get('action_completed') is True and state.get('bind_address') == '127.0.0.1')


def runner_arguments(role, *, directory, proof_id, port, transport, seconds, bootstrap, interactive=False,
                     map_id=1, mode='navigation', scenario=None, native_local_cipher=False,
                     original_host_mode=False, empty_bootstrap_probe_nonce=None, fixed_bot_roster_required=False):
    if role not in ('host', 'client'):
        raise ValueError('Expected a host or client runner')
    empty_bootstrap_probe_nonce = validate_pair_request(
        empty_bootstrap_probe_nonce, interactive=interactive, original_host_mode=original_host_mode,
        mode=mode, scenario=scenario)
    if scenario is not None and (scenario not in NATIVE_TEST_SCENARIOS or not interactive):
        raise ValueError('Native integration scenarios require an explicit interactive development pair')
    if scenario in JETT_SCENARIOS and mode != 'training':
        raise ValueError('Jett fixtures require explicit configured practice')
    if scenario == 'phoenix-c-primary' and mode != 'training':
        raise ValueError('phoenix-c-primary requires explicit configured practice')
    if scenario == spectator_input_fixture.SCENARIO and mode != 'training':
        raise ValueError('spectator-input-fixture requires an explicit configured training pair')
    if scenario == 'ffa-battle' and mode != 'training':
        raise ValueError('ffa-battle requires the existing training configuration and map')
    from native_maps import map_package
    map_package(map_id, training=mode == 'training')
    if mode not in ('navigation', 'training') or mode == 'navigation' and map_id != 1:
        raise ValueError('Candidate runners require Duel 1 navigation or a selected training map')
    command = [sys.executable, '-X', 'utf8', str(ROOT / 'tools/run_native_baseline.py'),
               '--run', '--bootstrap', str(bootstrap), '--map', str(map_id), '--mode', mode,
               '--original-game-instance', '--network-flavor', FLAVOR,
               '--network-proof-id', proof_id, '--network-port', str(port),
               '--network-transport', transport, '--seconds', str(seconds),
               '--network-probe', role, '--network-output', str(directory / role),
               '--result-file', str(directory / (role + '-result.json'))]
    if role == 'client':
        command += ['--network-peer', str(directory / 'host-result.active.json'), '--original-visual-classes']
        if scenario:
            command += ['--native-test-scenario', scenario]
    if interactive:
        command.append('--interactive-local-server')
    if native_local_cipher:
        if not interactive or transport != 'ares':
            raise ValueError('Native local cipher is restricted to an explicit interactive Ares pair')
        command.append('--native-local-cipher')
    if original_host_mode:
        if not interactive or mode != 'training':
            raise ValueError('Original host mode requires an explicit interactive training pair')
        if role == 'host':
            command.append('--original-host-mode')
    if fixed_bot_roster_required:
        if not interactive or not original_host_mode or mode != 'training':
            raise ValueError('Fixed bot roster requires an explicit original training host')
        if role == 'host': command.append('--fixed-bot-roster-required')
    if empty_bootstrap_probe_nonce is not None:
        command += ['--empty-bootstrap-probe-nonce', empty_bootstrap_probe_nonce]
    return command


def stop_request_path(active, *, proof_id, role, module_sha256):
    """Only this session's published child directory may receive a stop file."""
    metadata = active.get('network_probe') or {}
    identity = active.get('network_probe_identity') or {}
    if (metadata.get('role') != role or metadata.get('proof_id') != proof_id
            or metadata.get('flavor') != FLAVOR or identity.get('network_flavor') != FLAVOR
            or identity.get('module_sha256') != module_sha256
            or identity.get('pid') != active.get('pid')
            or identity.get('published_from_original_popen_handle') is not True):
        raise RuntimeError('Unexpected local server child ownership record')
    owned = Path(active.get('run_dir', '')).resolve()
    native_runs = (ROOT / '.local/native-baseline').resolve()
    target = (owned / 'stop-request.json').resolve()
    if native_runs not in owned.parents or owned not in target.parents:
        raise RuntimeError('Unexpected local server child ownership directory')
    return target


def _pointer(value):
    try:
        return int(value, 16) if isinstance(value, str) else int(value or 0)
    except (TypeError, ValueError):
        return 0


def analyze_gameplay_samples(samples):
    native = analyze_connection_samples(samples)
    checks = {'host_shooter_world_ready': False, 'authority_hero_spawned': False,
              'authority_hero_possessed': False, 'local_hero_received': False,
              'shooter_hero_round_trip_proven': False}
    for row in samples:
        host, client = row.get('host', {}), row.get('client', {})
        host_world = host.get('host_shooter_world_ready') is True and host.get('local_services_ready') is True
        spawned = host_world and host.get('authority_hero_spawned') is True and bool(_pointer(host.get('authority_hero')))
        owner = next((connection for connection in host.get('host_connections', [])
                      if connection.get('state') == 3 and connection.get('login_state') == 3
                      and connection.get('owner_links_verified') is True
                      and connection.get('controller_role') == 3 and connection.get('pawn_role') == 3
                      and _pointer(connection.get('controller')) == _pointer(host.get('authority_controller')) != 0
                      and _pointer(connection.get('player_state')) == _pointer(host.get('authority_player_state')) != 0
                      and _pointer(connection.get('pawn')) == _pointer(host.get('authority_hero')) != 0), None)
        possessed = spawned and host.get('authority_hero_possessed') is True and owner is not None
        native_row = analyze_connection_samples([row])
        received = (client.get('local_hero_received') is True and client.get('local_services_ready') is True
                    and native_row['replicated_own_state_and_pawn_proven'])
        checks['host_shooter_world_ready'] |= host_world
        checks['authority_hero_spawned'] |= spawned
        checks['authority_hero_possessed'] |= possessed
        checks['local_hero_received'] |= received
        checks['shooter_hero_round_trip_proven'] |= bool(
            possessed and received and not host.get('error') and not host.get('playable_error')
            and not client.get('error') and not client.get('playable_error'))
    return {**native, **checks, 'playable_verified': False,
            'scope': 'Shooter world and native authority hero lifecycle only; input/ACK/correction, weapons, abilities and presentation remain separate gates.'}


def assess_completion(report):
    checks = {
        'observation_completed': report.get('observation_completed') is True,
        'no_session_error': not report.get('error'),
        'no_cleanup_error': not report.get('cleanup_errors'),
        'native_driver_shutdown': report.get('host_cleanup', {}).get('cleaned') is True,
        'protected_builds_unchanged': report.get('protected_builds_unchanged') is True,
        'candidate_build_unchanged': report.get('candidate_build_unchanged') is True,
    }
    for role in ('host', 'client'):
        result = report.get(role + '_result', {})
        checks[role + '_completed'] = bool(
            report.get(role + '_runner_exit_code') == 0
            and not report.get(role + '_runner_forced_stop')
            and result.get('owned_process_exited') is True
            and result.get('stop_reason') == 'requested_shutdown'
            and result.get('image_unchanged') is True
            and result.get('postflight', {}).get('ready') is True
            and report.get(role + '_critical_errors') == 0)
    return {'passed': all(checks.values()), 'checks': checks}


def wait_runner(process, role, report):
    """A wrapper timeout still closes its own runner and kill-on-close job."""
    try:
        process.wait(timeout=15)
    except subprocess.TimeoutExpired:
        report[role + '_runner_forced_stop'] = True
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=5)
    report[role + '_runner_exit_code'] = process.poll()


def interactive_stop_requested(directory, proof_id, runner_pid):
    request = read(Path(directory) / 'stop-request.json')
    return bool(request and request.get('proof_id') == proof_id
                and request.get('runner_pid') == runner_pid
                and request.get('directory') == str(Path(directory).resolve()))


def host_start_failure(result):
    if result.get('status') != 'error':
        return None
    detail = result.get('error_detail') or {}
    return {'error': result.get('error'), 'detail': detail,
            'startup_step': result.get('startup_step')}


class RunnerJob:
    """Close this supervisor's exact baseline runner even if the supervisor exits."""
    def __init__(self, process):
        from capture_owned_thread import _kernel
        from run_native_baseline import assign_owned_job
        self.kernel, self.handle = _kernel(), None
        process_handle = int(process._handle)
        if process.poll() is not None or self.kernel.GetProcessId(process_handle) != process.pid:
            raise RuntimeError('Local server runner Popen identity changed before job assignment')
        self.handle = assign_owned_job(self.kernel, process_handle)

    def close(self):
        if self.handle:
            self.kernel.CloseHandle(self.handle)
        self.handle = None


def interactive_completion(report):
    """Lifecycle evidence only; an orderly early stop is not a readiness claim."""
    checks = {'no_session_error': not report.get('error'),
              'no_cleanup_error': not report.get('cleanup_errors'),
              'protected_builds_unchanged': report.get('protected_builds_unchanged') is True,
              'candidate_build_unchanged': report.get('candidate_build_unchanged') is True}
    for role in report.get('launched_roles', []):
        result = report.get(role + '_result', {})
        checks[role + '_completed'] = bool(
            report.get(role + '_runner_exit_code') == 0
            and not report.get(role + '_runner_forced_stop')
            and result.get('owned_process_exited') is True
            and result.get('stop_reason') in ('requested_shutdown', 'user_f10', 'bounded_deadline')
            and result.get('image_unchanged') is True
            and result.get('postflight', {}).get('ready') is True
            and report.get(role + '_critical_errors') == 0)
    if report.get('host_listen_verified'):
        checks['native_driver_shutdown'] = report.get('host_cleanup', {}).get('cleaned') is True
    return {'passed': all(checks.values()), 'checks': checks}


def controller_configuration(path=None):
    from local_server_controller import check_candidate_config
    if path is None:
        from training_controller_store import CANDIDATE, open_settings
        config = open_settings(ROOT, CANDIDATE).read()
    else:
        path = Path(path).resolve(strict=True)
        if (ROOT / '.local').resolve() not in path.parents:
            raise ValueError('Controller settings must belong to this project .local directory')
        # Explicit schema3 session snapshots include human_bots. Keep legacy
        # fixture files on their old reader; neither reader rewrites the file.
        saved = json.loads(path.read_text(encoding='utf-8'))
        if isinstance(saved, dict) and saved.get('schema_version') == 3:
            from human_bot_integration import HumanSettings as Settings
        else:
            from training_settings_v2 import Settings
        config = Settings(path).read()
    return check_candidate_config(config, root=ROOT)


def prepare_training_session(directory, config, bootstrap=None, peek_request=None):
    """Bind both native runners and the host Initial to the selected map/mode."""
    from local_server_controller import write_initial
    from local_server_maps import selected_map
    from native_maps import map_package
    directory = Path(directory)
    info = selected_map(config['map_id'], config['mode'], root=ROOT)
    if peek_request is not None:
        info = {**info, **peek_request['map_info']}
    package = map_package(config['map_id'], training=True)
    target = directory / 'bootstrap.bin'
    if bootstrap is None:
        from run_map_probe import prepare
        prepared = prepare(ROOT, config['map_id'], 'training', target)
    else:
        from ini_bootstrap import apply_profile, decode, encode
        source = Path(bootstrap).resolve(strict=True)
        if ROOT.resolve() not in source.parents:
            raise ValueError('Bootstrap archive must stay inside this project')
        payload = source.read_bytes()
        snapshot = apply_profile(decode(payload), 'training', config['map_id'])
        encoded = encode(snapshot)
        if decode(encoded) != snapshot:
            raise ValueError('Selected candidate bootstrap did not round trip')
        with target.open('xb') as stream:
            stream.write(encoded)
        prepared = {'map_number': config['map_id'], 'package': package, 'mode': 'training',
                    'archive': str(target), 'archive_sha256': hashlib.sha256(encoded).hexdigest(),
                    'source_archive': str(source), 'source_archive_sha256': hashlib.sha256(payload).hexdigest(),
                    'round_trip': True}
    if prepared['package'] != package:
        raise ValueError('Prepared candidate bootstrap differs from the selected map')
    initial = write_initial(directory, config, info, root=ROOT)
    return {'map_id': config['map_id'], 'training_mode': config['mode'], 'engine_mode': 'training',
            'map_package': package, 'bootstrap': prepared, 'initial_configuration': initial}


def run_interactive(args):
    """Keep both exact baseline runners alive until Stop, F10, error or 24h.

    Only the Popen objects created here can be forcibly retired. Their own
    kill-on-close jobs retain ownership of native children during cleanup.
    """
    from local_server_controller import (KIND, atomic_json, current_runner_identity, pair_directory,
                                         require_training_build)
    from training_heroes import require_selection_build

    config = controller_configuration(args.controller_settings)
    peek_request, config = peek_fixture.prepare_request(args, config)
    hold_request, config = hold_fixture.prepare_request(args, config)
    if peek_request is not None:
        args.scenario = 'peek-fixture'
    scenario = getattr(args, 'scenario', None)
    probe_nonce = validate_pair_request(
        getattr(args, 'empty_bootstrap_probe_nonce', None),
        interactive=bool(getattr(args, 'interactive', False)),
        original_host_mode=bool(getattr(args, 'original_host_mode', False)),
        scenario=scenario, config=config)
    bounded_seconds = native_test_policy(scenario, config, args.seconds)
    build, original = verified_build(), artifact_hashes()
    if scenario == spectator_input_fixture.SCENARIO:
        spectator_input_fixture.require_build(build)
    if peek_request is not None:
        peek_fixture.require_build(build)
    if hold_request is not None:
        hold_fixture.require_build(build)
    if probe_nonce is not None:
        require_probe_build(build)
    require_training_build(build, config)
    require_selection_build(build, config)
    if getattr(args, 'native_local_cipher', False) and not any(
            Path(name).name == 'local_server_ares_local_key.h' for name in build.get('source_sha256', {})):
        raise RuntimeError('This compiled candidate does not include the optional native local cipher adapter')
    candidate_manifest = hashlib.sha256((BUILD / 'build.json').read_bytes()).hexdigest()
    tag = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
    directory = pair_directory(args.session_dir or ROOT / '.local/local-server' / ('pair-' + tag), ROOT)
    directory.mkdir(parents=True)  # Refuse reuse of an existing pair or its stop files.
    proof_id = uuid.uuid4().hex
    selection = prepare_training_session(directory, config, args.bootstrap, peek_request or hold_request)
    if peek_request is not None:
        peek_fixture.write_contract(directory, peek_request, config, build)
    if hold_request is not None:
        hold_fixture.write_contract(directory, hold_request, config, build)
    bootstrap = Path(selection['bootstrap']['archive'])
    pair = {'schema_version': 1, 'kind': KIND, 'network_flavor': FLAVOR,
            'directory': str(directory), 'proof_id': proof_id, 'module_sha256': build['module_sha256'],
            'runner_identity': current_runner_identity(), 'transport': args.transport, 'port': args.port,
            'max_seconds': bounded_seconds, 'requested_seconds': args.seconds,
            'developmental': True, 'playable_verified': False,
            'original_host_mode': bool(getattr(args, 'original_host_mode', False)),
            **selection}
    if probe_nonce is not None:
        pair['empty_equipment_bootstrap_probe'] = {
            'requested': True, 'nonce': probe_nonce, 'scenario': None}
    atomic_json(directory / 'session.active.json', pair)
    atomic_json(ROOT / '.local/local-server/latest-session.json',
                {'directory': str(directory), 'proof_id': proof_id, 'kind': KIND})
    report = {**pair, 'status': 'loading', 'passed': False, 'launched_roles': [],
              'host_listen_verified': False, 'client_travel_requested': False,
              'samples': [], 'total_samples': 0, 'cleanup_errors': []}
    # Explicit native fixtures keep their original full inline evidence.
    history = SampleHistory(report) if scenario is None or scenario in ('shop-correction-prelude', 'map-hold-fixture') else None
    processes, logs, jobs = {}, {}, {}
    local_pair_key = None
    if getattr(args, 'native_local_cipher', False):
        from local_server_ares_key import LocalPairKey
        local_pair_key = LocalPairKey(directory, proof_id, args.port)
        report['native_local_cipher'] = {'requested': True, 'credentials': [], 'encrypted_join_verified': False}
    started = time.monotonic()
    deadline = started + bounded_seconds

    def progress():
        try:
            save(directory / 'progress.json', progress_snapshot(report))
        except OSError as error:
            # Report I/O must not prevent the owned runners' finally cleanup.
            report.setdefault('error', 'Cannot write interactive progress: ' + str(error))

    def alive(role):
        return role in processes and processes[role].poll() is None

    def requested():
        return interactive_stop_requested(directory, proof_id, os.getpid())

    def launch(role):
        if requested() or time.monotonic() >= deadline:
            return False
        (directory / role).mkdir()
        if local_pair_key:
            report['native_local_cipher']['credentials'].append(local_pair_key.write_role(role))
        if peek_request is not None:
            peek_fixture.copy_role_contract(directory, role)
        if hold_request is not None:
            hold_fixture.copy_role_contract(directory, role)
        if role == 'client' and scenario:
            nonce = (peek_request or hold_request)['nonce'] if peek_request is not None or hold_request is not None else int.from_bytes(os.urandom(8), 'little') or 1
            marker = native_test_marker(scenario, nonce)
            (directory / role / 'native-test-scenario.bin').write_bytes(marker)
            report['native_test_scenario'] = {'name': scenario, 'nonce': nonce, 'marker_bytes': len(marker),
                                            'marker_sha256': hashlib.sha256(marker).hexdigest(),
                                            'maximum_native_wall_ms': FFA_BATTLE_NATIVE_MS if scenario == 'ffa-battle' else 20000,
                                            'physical_input_verified': False}
        if role == 'client' and (scenario == 'phoenix-c-primary' or scenario in JETT_SCENARIOS):
            report['native_test_scenario']['maximum_native_wall_ms'] = PHOENIX_C_NATIVE_MS
        if role == 'client' and scenario in ('map-hold-fixture', spectator_input_fixture.SCENARIO):
            report['native_test_scenario']['maximum_native_wall_ms'] = 45000
        command = runner_arguments(role, directory=directory, proof_id=proof_id, port=args.port,
                                   transport=args.transport, seconds=min(86400, max(1, int(deadline - time.monotonic()) + 60)),
                                   bootstrap=bootstrap, interactive=True, map_id=config['map_id'], mode='training', scenario=scenario,
                                   native_local_cipher=bool(local_pair_key),
                                   original_host_mode=bool(getattr(args, 'original_host_mode', False)),
                                   empty_bootstrap_probe_nonce=probe_nonce,
                                   fixed_bot_roster_required=bool(getattr(args, 'fixed_bot_roster_required', False) or config.get('human_bots', {}).get('bot_roster') is not None))
        if peek_request is not None:
            command.extend(('--peek-fixture-nonce', f"{peek_request['nonce']:016x}"))
        if hold_request is not None:
            command.extend(('--map-hold-fixture-nonce', f"{hold_request['nonce']:016x}"))
        log = (directory / (role + '-runner.log')).open('w', encoding='utf-8')
        logs[role] = log
        try:
            processes[role] = subprocess.Popen(command, cwd=ROOT, env=project_environment(ROOT),
                                               stdout=log, stderr=log,
                                               creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        except Exception:
            log.close()
            raise
        report['launched_roles'].append(role)
        jobs[role] = RunnerJob(processes[role])
        report[role + '_runner_job_assigned'] = True
        progress()
        return True

    try:
        if launch('host'):
            listen_deadline = min(deadline, time.monotonic() + 50)
            while time.monotonic() < listen_deadline and alive('host') and not requested():
                state = read(directory / 'host/bridge-state.json')
                if state.get('error') or state.get('playable_error'):
                    report['last_host'] = state
                    raise RuntimeError('Host: ' + str(state.get('failure_stage') or state.get('playable_stage')))
                if host_ready(state, args.port):
                    report['host_listen_verified'] = True
                    progress()
                    break
                time.sleep(.1)
            if not report['host_listen_verified'] and not requested() and time.monotonic() < deadline:
                result = read(directory / 'host-result.json')
                failure = host_start_failure(result)
                if failure:
                    report['host_failure'] = failure
                    message = failure['detail'].get('message') or failure['error'] or 'unknown error'
                    raise RuntimeError('Host 启动失败: ' + str(message))
                raise RuntimeError('Owned local server host did not reach its verified loopback bind')
        if report['host_listen_verified'] and launch('client'):
            last_save = 0
            while time.monotonic() < deadline and not requested():
                for role in ('host', 'client'):
                    if not alive(role):
                        result = read(directory / (role + '-result.json'))
                        if role == 'client' and processes[role].poll() == 0 and result.get('stop_reason') in ('user_f10', 'requested_shutdown'):
                            report['stop_reason'] = 'client_requested_shutdown'
                            break
                        raise RuntimeError('Owned local server ' + role + ' exited before shutdown')
                else:
                    host = read(directory / 'host/bridge-state.json')
                    client = read(directory / 'client/bridge-state.json')
                    if scenario == 'phoenix-c-primary' or scenario in JETT_SCENARIOS or scenario in ('shop-correction-prelude', spectator_input_fixture.SCENARIO):
                        report['native_fixture_observation'] = native_fixture_observation(client, scenario, report['native_test_scenario']['nonce'])
                    if scenario == 'peek-fixture':
                        report['native_fixture_observation'] = peek_fixture.observation(client, report['native_test_scenario']['nonce'])
                        fixture_host = host.get('peek_fixture', {})
                        if fixture_host.get('failed') is True or fixture_host.get('io_errors', 0):
                            raise RuntimeError('Peek fixture original scene/source/evidence gate failed: ' + str(fixture_host.get('stage')))
                    if scenario == 'map-hold-fixture':
                        report['native_fixture_observation'] = hold_fixture.observation(client, report['native_test_scenario']['nonce'], host)
                    if scenario == 'ffa-battle':
                        report['native_fixture_observation'] = native_fixture_observation(
                            client, scenario, report['native_test_scenario']['nonce'])
                    if (host.get('error') or host.get('playable_error') or client.get('error')
                            or client.get('playable_error') or client.get('pending_travel_failed')):
                        report['last_host'], report['last_client'] = host, client
                        raise RuntimeError('Native local server candidate reported an error')
                    now = time.monotonic()
                    # Retain the real paired terminal read before Scenario15
                    # stops; it can finish between the ordinary .5s samples.
                    spectator_terminal = (scenario == spectator_input_fixture.SCENARIO
                                          and report['native_fixture_observation']['finished'])
                    if spectator_terminal or not report['samples'] or now - started - report['samples'][-1]['elapsed'] >= .5:
                        row = {'elapsed': round(now - started, 3), 'host': host, 'client': client}
                        if history is not None:
                            history.append(report, row)
                        else:
                            report['samples'].append(row)
                            del report['samples'][:-240]
                            report['total_samples'] += 1
                        report['client_travel_requested'] |= bool(client.get('action_completed'))
                    if (scenario == 'ffa-battle' or scenario in ('peek-fixture', 'map-hold-fixture', spectator_input_fixture.SCENARIO) or scenario in JETT_SCENARIOS or scenario == 'shop-correction-prelude') and report['native_fixture_observation']['finished']:
                        report['stop_reason'] = 'native_fixture_finished'
                        break
                    if now - last_save >= 1:
                        report['status'] = 'observing'
                        report['native_gates'] = analyze_gameplay_samples(report['samples'])
                        progress()
                        last_save = now
                    time.sleep(.1)
                    continue
                break
        report.setdefault('stop_reason', 'requested_shutdown' if requested() else 'bounded_deadline')
    except KeyboardInterrupt:
        report['stop_reason'] = 'supervisor_interrupted'
    except Exception as error:
        report['error'] = str(error)
        report['stop_reason'] = 'error'
    finally:
        report['status'] = 'stopping'
        progress()
        report['last_host'] = read(directory / 'host/bridge-state.json') or report.get('last_host', {})
        report['last_client'] = read(directory / 'client/bridge-state.json') or report.get('last_client', {})
        if (scenario == 'phoenix-c-primary' or scenario in JETT_SCENARIOS or scenario in ('shop-correction-prelude', spectator_input_fixture.SCENARIO)) and report.get('native_test_scenario'):
            report['native_fixture_observation'] = native_fixture_observation(report['last_client'], scenario, report['native_test_scenario']['nonce'])
        if scenario == 'peek-fixture' and report.get('native_test_scenario'):
            report['native_fixture_observation'] = peek_fixture.observation(report['last_client'], report['native_test_scenario']['nonce'])
        if scenario == 'map-hold-fixture' and report.get('native_test_scenario'):
            report['native_fixture_observation'] = hold_fixture.observation(report['last_client'], report['native_test_scenario']['nonce'], report['last_host'])
        if scenario == 'ffa-battle' and report.get('native_test_scenario'):
            report['native_fixture_observation'] = native_fixture_observation(
                report['last_client'], scenario, report['native_test_scenario']['nonce'])
        # The client's postflight requires the original, still living host.
        for role in ('client', 'host'):
            if role not in processes:
                continue
            active = read(directory / (role + '-result.active.json'))
            active_deadline = time.monotonic() + 3
            while not active and alive(role) and time.monotonic() < active_deadline:
                time.sleep(.05)
                active = read(directory / (role + '-result.active.json'))
            try:
                if active:
                    target = stop_request_path(active, proof_id=proof_id, role=role,
                                               module_sha256=build['module_sha256'])
                    if role == 'host' and alive('host'):
                        (directory / 'host/stop-network').write_text('original driver shutdown', encoding='utf-8')
                        cleanup_deadline = time.monotonic() + 3
                        while time.monotonic() < cleanup_deadline:
                            if read(directory / 'host/bridge-state.json').get('cleaned'):
                                break
                            time.sleep(.05)
                        report['host_cleanup'] = read(directory / 'host/bridge-state.json')
                    save(target, {'reason': 'local_server_interactive_complete', 'proof_id': proof_id})
                wait_runner(processes[role], role, report)
                report[role + '_result'] = read(directory / (role + '-result.json'))
                if active:
                    log_path = stop_request_path(active, proof_id=proof_id, role=role,
                                                 module_sha256=build['module_sha256']).parent / 'runtime.log'
                    if log_path.is_file():
                        report[role + '_critical_errors'] = log_path.read_text(encoding='utf-8', errors='replace').count('Critical error:')
            except Exception as error:
                report['cleanup_errors'].append(role + ': ' + str(error))
                # Even a malformed receipt cannot leave our exact runner alive.
                try:
                    wait_runner(processes[role], role, report)
                except Exception as cleanup_error:
                    report['cleanup_errors'].append(role + ' runner: ' + str(cleanup_error))
            finally:
                if role in jobs:
                    jobs[role].close()
                logs[role].close()
        if local_pair_key:
            try:
                local_pair_key.close()
                report['native_local_cipher']['credential_files_removed'] = True
            except (OSError, RuntimeError) as error:
                report['cleanup_errors'].append('Native local credential cleanup: ' + str(error))
        try:
            report['protected_builds_unchanged'] = artifact_hashes() == original
            report['candidate_build_unchanged'] = bool(
                verified_build()['module_sha256'] == build['module_sha256']
                and hashlib.sha256((BUILD / 'build.json').read_bytes()).hexdigest() == candidate_manifest)
        except (OSError, ValueError, RuntimeError) as error:
            report['cleanup_errors'].append('build verification: ' + str(error))
        if history is not None:
            try:
                history.write(directory, report)
            except (OSError, ValueError) as error:
                report['sample_evidence']['archive'].update(status='failed', error=str(error))
                report.setdefault('error', 'Cannot retain full interactive evidence: ' + str(error))
        report['native_gates'] = analyze_gameplay_samples(report['samples'])
        report['phase_gates_passed'] = bool(report['host_listen_verified']
                                           and report['native_gates']['shooter_hero_round_trip_proven'])
        report['completion'] = interactive_completion(report)
        report['passed'] = report['phase_gates_passed'] and report['completion']['passed']
        if scenario == 'ffa-battle':
            observed = report.get('native_fixture_observation', {})
            report['passed'] &= bool(observed.get('finished') and observed.get('input_actions_completed')
                                     and observed.get('match_end_observed'))
        if scenario in JETT_SCENARIOS:
            observed = report.get('native_fixture_observation', {})
            report['passed'] &= bool(observed.get('finished') and observed.get('input_actions_completed'))
        if scenario == 'map-hold-fixture':
            observed = report.get('native_fixture_observation', {})
            report['passed'] &= bool(observed.get('finished') and observed.get('map_hold_physics_verified'))
        report['status'] = 'stopped' if report['completion']['passed'] else 'error'
        if scenario == spectator_input_fixture.SCENARIO:
            report['native_fixture_observation'] = spectator_input_fixture.correlate(
                report['samples'], report['native_test_scenario']['nonce'], report.get('last_client'))
            report['passed'] &= report['native_fixture_observation']['host_rpc_counter_windows_verified']
        if scenario in ('map-hold-fixture', spectator_input_fixture.SCENARIO) and report['completion']['passed'] and not report['passed']:
            report['status'] = 'not_observed'
        report['elapsed_seconds'] = round(time.monotonic() - started, 3)
        report['at_utc'] = datetime.now(timezone.utc).isoformat()
        progress()
        save(directory / 'verification.json', report)
    print(json.dumps({'status': report['status'], 'passed': report['passed'], 'playable_verified': False,
                      'error': report.get('error'), 'report': str(directory / 'verification.json')}), flush=True)
    return 0 if (report['passed'] if scenario in ('ffa-battle', 'map-hold-fixture', spectator_input_fixture.SCENARIO) or scenario in JETT_SCENARIOS else report['completion']['passed']) else 1


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    peek_fixture.add_arguments(parser)
    hold_fixture.add_arguments(parser)
    parser.add_argument('--port', type=int, default=17777)
    parser.add_argument('--observe-seconds', type=int, default=35)
    parser.add_argument('--transport', choices=('ip', 'ares'), default='ip')
    parser.add_argument('--bootstrap', type=Path)
    parser.add_argument('--interactive', action='store_true', help='Opt-in development pair, up to 24h; Stop or F10 ends it earlier')
    parser.add_argument('--seconds', type=int, default=86400)
    parser.add_argument('--controller-settings', type=Path)
    parser.add_argument('--session-dir', type=Path)
    parser.add_argument('--scenario', choices=NATIVE_TEST_SCENARIOS, help='Explicit bounded native integration fixture; ffa-battle uses finite-ammo deathmatch and at most 720s; not a physical input test')
    parser.add_argument('--native-local-cipher', action='store_true', help='Opt-in native Ares local credential candidate; encrypted connection remains unverified')
    parser.add_argument('--original-host-mode', action='store_true', help='Opt-in original ShooterMode with original timing lifecycle; interactive training pair only')
    parser.add_argument('--fixed-bot-roster-required', action='store_true', help='Require the exact fixed bot roster sidecar for this new original-host pair')
    parser.add_argument('--empty-bootstrap-probe-nonce', type=normalize_nonce,
                        help='Explicit 32-hex empty-equipment probe; ordinary interactive original-host FFA only')
    args = parser.parse_args(argv)
    try:
        validate_pair_request(args.empty_bootstrap_probe_nonce, interactive=args.interactive,
                              original_host_mode=args.original_host_mode, scenario=args.scenario)
    except ValueError as error:
        parser.error(str(error))
    if not 1024 <= args.port <= 65535 or not 5 <= args.observe_seconds <= 75:
        parser.error('Choose an unprivileged loopback port and 5..75 seconds of observation')
    if not 1 <= args.seconds <= 86400:
        parser.error('Interactive duration must be between 1 second and 24 hours')
    if not args.interactive and args.seconds != 86400:
        parser.error('Long-running duration requires --interactive')
    if args.scenario and not args.interactive:
        parser.error('Native integration scenarios require the explicit interactive development pair')
    if args.original_host_mode and not args.interactive:
        parser.error('Original host mode requires the explicit interactive training pair')
    if args.fixed_bot_roster_required and not (args.interactive and args.original_host_mode):
        parser.error('Fixed bot roster requires the explicit interactive original-host pair')
    if args.native_local_cipher and (not args.interactive or args.transport != 'ares'):
        parser.error('Native local cipher requires an explicit interactive Ares pair')
    if args.interactive:
        return run_interactive(args)
    selected_config = controller_configuration(args.controller_settings) if args.controller_settings else None
    bootstrap = None
    if selected_config is None:
        bootstrap = (args.bootstrap or DEFAULT_BOOTSTRAP).resolve(strict=True)
        if ROOT.resolve() not in bootstrap.parents:
            parser.error('Bootstrap archive must stay inside this project')
    build = verified_build()
    if selected_config:
        from local_server_controller import require_training_build
        require_training_build(build, selected_config)
    original = artifact_hashes()
    candidate_manifest = hashlib.sha256((BUILD / 'build.json').read_bytes()).hexdigest()
    tag = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
    from local_server_controller import pair_directory
    directory = pair_directory(args.session_dir or ROOT / '.local/local-server' / ('pair-' + tag), ROOT)
    directory.mkdir(parents=True)
    selection = prepare_training_session(directory, selected_config, args.bootstrap) if selected_config else {}
    if selection:
        bootstrap = Path(selection['bootstrap']['archive'])
    proof_id = uuid.uuid4().hex
    report = {
        'schema_version': 1, 'kind': 'native-shooter-candidate', 'network_flavor': FLAVOR,
        'directory': str(directory), 'proof_id': proof_id, 'module_sha256': build['module_sha256'],
        'transport': args.transport, 'port': args.port,
        'host_listen_verified': False, 'client_travel_requested': False,
        'requested_observation_seconds': args.observe_seconds, 'observation_completed': False,
        'playable_verified': False, 'passed': False, 'samples': [], 'cleanup_errors': [], **selection,
    }
    processes, logs = {}, {}

    def progress():
        save(directory / 'progress.json', report)

    def alive(role):
        return role in processes and processes[role].poll() is None

    def launch(role, seconds):
        (directory / role).mkdir()
        command = runner_arguments(role, directory=directory, proof_id=proof_id, port=args.port,
                                   transport=args.transport, seconds=seconds, bootstrap=bootstrap,
                                   map_id=selected_config['map_id'] if selected_config else 1,
                                   mode='training' if selected_config else 'navigation')
        log = (directory / (role + '-runner.log')).open('w', encoding='utf-8')
        logs[role] = log
        processes[role] = subprocess.Popen(command, cwd=ROOT, env=project_environment(ROOT),
                                           stdout=log, stderr=log, creationflags=subprocess.CREATE_NO_WINDOW)

    try:
        save(ROOT / '.local/local-server/latest-session.json',
             {'directory': str(directory), 'proof_id': proof_id, 'kind': report['kind']})
        launch('host', min(175, args.observe_seconds + 100))
        deadline = time.monotonic() + 40
        while time.monotonic() < deadline and alive('host'):
            state = read(directory / 'host/bridge-state.json')
            if state.get('error') or state.get('playable_error'):
                report['last_host'] = state
                raise RuntimeError('Host: ' + str(state.get('failure_stage') or state.get('playable_stage')))
            if host_ready(state, args.port):
                report['host_listen_verified'] = True
                report['host'] = state
                print(json.dumps({'checkpoint': 'shooter_candidate_host_listening', 'pid': state['pid'],
                                  'address': '127.0.0.1', 'port': args.port}), flush=True)
                break
            time.sleep(.1)
        if not report['host_listen_verified']:
            raise RuntimeError('Owned Shooter candidate host did not reach its verified loopback bind')
        launch('client', min(150, args.observe_seconds + 50))
        start = time.monotonic()
        deadline, ready_at = start + args.observe_seconds + 35, None
        while time.monotonic() < deadline and alive('host') and alive('client'):
            host = read(directory / 'host/bridge-state.json')
            client = read(directory / 'client/bridge-state.json')
            if (host.get('error') or host.get('playable_error') or client.get('error')
                    or client.get('playable_error') or client.get('pending_travel_failed')):
                report['last_host'], report['last_client'] = host, client
                raise RuntimeError('Native Shooter candidate reported an error')
            if client.get('action_completed'):
                ready_at = ready_at or time.monotonic()
                report['client_travel_requested'] = True
            row = {'elapsed': round(time.monotonic() - start, 3), 'host': host, 'client': client}
            if not report['samples'] or row['elapsed'] - report['samples'][-1]['elapsed'] >= .1:
                report['samples'].append(row)
                if len(report['samples']) % 10 == 0:
                    progress()
            if ready_at and time.monotonic() - ready_at >= args.observe_seconds:
                report['observation_completed'] = alive('host') and alive('client')
                break
            time.sleep(.1)
        report['observed_seconds'] = round(time.monotonic() - ready_at, 3) if ready_at else 0
    except KeyboardInterrupt:
        report['error'] = 'Session interrupted'
    except Exception as error:
        report['error'] = str(error)
    finally:
        report['last_host'] = read(directory / 'host/bridge-state.json') or report.get('last_host', {})
        report['last_client'] = read(directory / 'client/bridge-state.json') or report.get('last_client', {})
        # The client's postflight still needs the exact, living host identity.
        for role in ('client', 'host'):
            active = read(directory / (role + '-result.active.json'))
            try:
                if active:
                    target = stop_request_path(active, proof_id=proof_id, role=role,
                                               module_sha256=build['module_sha256'])
                    if role == 'host' and alive('host'):
                        (directory / 'host/stop-network').write_text('original driver shutdown', encoding='utf-8')
                        deadline = time.monotonic() + 3
                        while time.monotonic() < deadline:
                            if read(directory / 'host/bridge-state.json').get('cleaned'):
                                break
                            time.sleep(.05)
                        report['host_cleanup'] = read(directory / 'host/bridge-state.json')
                    save(target, {'reason': 'local_server_candidate_complete', 'proof_id': proof_id})
            except Exception as error:
                report['cleanup_errors'].append(role + ': ' + str(error))
            if role in processes:
                try:
                    wait_runner(processes[role], role, report)
                except Exception as error:
                    report['cleanup_errors'].append(role + ' runner: ' + str(error))
                report[role + '_result'] = read(directory / (role + '-result.json'))
                if active:
                    try:
                        target = stop_request_path(active, proof_id=proof_id, role=role,
                                                   module_sha256=build['module_sha256'])
                        log = target.parent / 'runtime.log'
                        if log.is_file():
                            report[role + '_critical_errors'] = log.read_text(encoding='utf-8', errors='replace').count('Critical error:')
                    except (OSError, RuntimeError) as error:
                        report['cleanup_errors'].append(role + ' log: ' + str(error))
            if role in logs:
                logs[role].close()
        try:
            report['protected_builds_unchanged'] = artifact_hashes() == original
            report['candidate_build_unchanged'] = bool(
                verified_build()['module_sha256'] == build['module_sha256']
                and hashlib.sha256((BUILD / 'build.json').read_bytes()).hexdigest() == candidate_manifest)
        except (OSError, ValueError, RuntimeError) as error:
            report['cleanup_errors'].append('build verification: ' + str(error))
        report['native_gates'] = analyze_gameplay_samples(report['samples'])
        report['phase_gates_passed'] = bool(
            report['host_listen_verified'] and report['native_gates']['shooter_hero_round_trip_proven'])
        report['completion'] = assess_completion(report)
        report['passed'] = report['phase_gates_passed'] and report['completion']['passed']
        report['at_utc'] = datetime.now(timezone.utc).isoformat()
        progress()
        save(directory / 'verification.json', report)
    print(json.dumps({'passed': report['passed'], 'playable_verified': False,
                      'native_gates': report['native_gates'], 'error': report.get('error'),
                      'report': str(directory / 'verification.json')}), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
