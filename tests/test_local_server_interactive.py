"""Simulate the full pair supervisor with original Popen objects replaced by fixtures."""
from copy import deepcopy
from io import StringIO
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import local_server_controller as control
import run_local_server as runner
from training_settings_v2 import DEFAULTS
from ini_bootstrap import encode
from test_ini_bootstrap import snapshot


class Clock:
    def __init__(self, directory, *, stop_at=None, host_ready_at=0):
        self.directory, self.now, self.stop_at = directory, 0., stop_at
        self.host_ready_at = host_ready_at

    def monotonic(self):
        return self.now

    def sleep(self, seconds):
        self.now += seconds
        if self.now >= self.host_ready_at:
            path = self.directory / 'host/bridge-state.json'
            host = control.read_json(path)
            if host and not host.get('action_completed'):
                host['action_completed'] = True
                control.atomic_json(path, host)
        pair = (control.read_json(self.directory / 'session.active.json')
                or control.read_json(self.directory.parent / 'latest-session.json'))
        if self.stop_at is not None and self.now >= self.stop_at:
            control.atomic_json(self.directory / 'stop-request.json',
                                {'directory': str(self.directory), 'proof_id': pair['proof_id'],
                                 'runner_pid': os.getpid(), 'reason': 'fixture_webui_stop'})
        if (self.directory / 'host/stop-network').is_file():
            host = control.read_json(self.directory / 'host/bridge-state.json')
            host['cleaned'] = True
            control.atomic_json(self.directory / 'host/bridge-state.json', host)


class Process:
    def __init__(self, command, root, clock, events, *, exit_early=False, wrong_identity=False, user_exit=False):
        self.role = command[command.index('--network-probe') + 1]
        self.directory = Path(command[command.index('--network-output') + 1]).parent
        self.root, self.clock, self.events = root, clock, events
        self.pid, self.code = (101 if self.role == 'host' else 102), None
        self.exit_early, self.terminated = exit_early and self.role == 'client', False
        self.user_exit = user_exit and self.role == 'client'
        pair = (control.read_json(self.directory / 'session.active.json')
                or control.read_json(root / '.local/local-server/latest-session.json'))
        self.run = root / '.local/native-baseline' / self.role
        self.run.mkdir(parents=True)
        (self.run / 'runtime.log').write_text('fixture only', encoding='utf-8')
        active = {'pid': self.pid, 'run_dir': str(self.run),
                  'network_probe': {'role': self.role, 'proof_id': pair['proof_id'], 'flavor': 'playable'},
                  'network_probe_identity': {'pid': self.pid, 'network_flavor': 'playable',
                                             'module_sha256': 'wrong' if wrong_identity else 'b' * 64,
                                             'published_from_original_popen_handle': True}}
        control.atomic_json(self.directory / (self.role + '-result.active.json'), active)
        state = {'pid': self.pid, 'role': self.role, 'port': 17777, 'action_completed': True}
        if self.role == 'host':
            state['action_completed'] = clock.host_ready_at == 0
            state.update(listen_returned=True, loopback_candidates_verified=True, bind_address='127.0.0.1')
            if command[command.index('--mode') + 1] == 'training' and not (self.directory / 'session-initial.bin').is_file():
                raise AssertionError('Host was launched before its initial configuration existed')
            if command[command.index('--mode') + 1] == 'training' and not (self.directory / 'session-hero-selection.bin').is_file():
                raise AssertionError('Host was launched before its hero selection existed')
        elif self.clock.now < self.clock.host_ready_at:
            raise AssertionError('Client was launched before the host runtime/login factory completed')
        control.atomic_json(self.directory / self.role / 'bridge-state.json', state)
        events.append('launch_' + self.role)
        self.command = command
        self.launched_at = clock.now

    def poll(self):
        if self.exit_early and self.clock.now >= 1 and self.code is None:
            self.code = 1
            self.result('target_exited')
        if self.user_exit and self.clock.now >= 1 and self.code is None:
            self.code = 0
            self.events.append('user_f10_client')
            self.result('user_f10')
        return self.code

    def result(self, reason):
        control.atomic_json(self.directory / (self.role + '-result.json'),
                            {'owned_process_exited': True, 'stop_reason': reason,
                             'image_unchanged': True, 'postflight': {'ready': True}})

    def wait(self, timeout):
        if self.code is None:
            if not (self.run / 'stop-request.json').is_file():
                raise subprocess.TimeoutExpired('fixture', timeout)
            self.events.append('shutdown_' + self.role)
            self.code = 0
            self.result('requested_shutdown')
        return self.code

    def terminate(self):
        self.terminated = True
        self.code = 1
        self.events.append('terminate_' + self.role)

    def kill(self):
        self.terminate()


class InteractiveLifecycleTests(unittest.TestCase):
    def simulate(self, *, stop_at=None, exit_early=False, wrong_identity=False, user_exit=False,
                 progress_error=False, host_ready_at=0, bounded=False, selected_config=None):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / 'profiles').mkdir()
            shutil.copyfile(Path(__file__).resolve().parents[1] / 'profiles/playable-heroes.json',
                            root / 'profiles/playable-heroes.json')
            build = root / '.local/local-server/module'
            build.mkdir(parents=True)
            (build / 'build.json').write_text('fixture manifest', encoding='utf-8')
            bootstrap = root / 'bootstrap.bin'
            bootstrap.write_bytes(encode(snapshot()))
            settings = root / '.local/controller/config.json'
            control.atomic_json(settings, deepcopy(selected_config or DEFAULTS))
            directory = root / '.local/local-server/pair-fixture'
            args = SimpleNamespace(controller_settings=settings, bootstrap=bootstrap, session_dir=directory,
                                   transport='ip', port=17777, seconds=5, scenario=None)
            clock, events, processes = Clock(directory, stop_at=stop_at, host_ready_at=host_ready_at), [], []
            def popen(command, **kwargs):
                process = Process(command, root, clock, events, exit_early=exit_early,
                                  wrong_identity=wrong_identity, user_exit=user_exit)
                processes.append(process)
                return process
            def job(process):
                process.job_closed = False
                def close():
                    process.job_closed = True
                return SimpleNamespace(close=close)
            original_save = runner.save
            def save(path, value):
                if progress_error and Path(path).name == 'progress.json':
                    raise OSError('fixture report I/O failure')
                return original_save(path, value)
            with patch.object(runner, 'ROOT', root), patch.object(runner, 'BUILD', build), \
                    patch.object(runner, 'verified_build', return_value={
                        'module_sha256': 'b' * 64, 'source_sha256': {name: 'e' * 64 for name in control.TRAINING_SOURCES}}), \
                    patch.object(runner, 'artifact_hashes', return_value={'fixture': 'unchanged'}), \
                    patch.object(runner.subprocess, 'Popen', side_effect=popen), \
                    patch.object(runner, 'RunnerJob', side_effect=job), \
                    patch.object(runner, 'save', side_effect=save), \
                    patch.object(runner.time, 'monotonic', side_effect=clock.monotonic), \
                    patch.object(runner.time, 'sleep', side_effect=clock.sleep), \
                    patch('local_server_controller.current_runner_identity',
                          return_value={'pid': os.getpid(), 'published_from_own_process': True}), \
                    patch('native_maps.training_maps', return_value={
                        'duel1': {'id': 'duel1', 'kind': 'duel', 'world_path': '/Game/Maps/Duel/Duel_1/Skirmish_A'},
                        'ascent': {'id': 'ascent', 'kind': 'standard_5v5', 'world_path': '/Game/Maps/Ascent/Ascent'}}), \
                    patch('sys.stdout', new=StringIO()):
                command = ['--bootstrap', str(bootstrap), '--session-dir', str(directory), '--observe-seconds', '5']
                if selected_config:
                    command += ['--controller-settings', str(settings)]
                code = (runner.main(command)
                        if bounded else runner.run_interactive(args))
            report = control.read_json(directory / 'verification.json')
            self.assertTrue(all(process.poll() is not None for process in processes))
            if not bounded:
                self.assertTrue(all(process.job_closed for process in processes))
            return code, report, events, processes

    def test_webui_stop_cleans_client_before_host_without_manufacturing_playable(self):
        code, report, events, processes = self.simulate(stop_at=2)
        self.assertEqual(code, 0)
        self.assertEqual(events, ['launch_host', 'launch_client', 'shutdown_client', 'shutdown_host'])
        self.assertEqual(report['stop_reason'], 'requested_shutdown')
        self.assertTrue(report['completion']['passed'])
        self.assertFalse(report['playable_verified'])
        self.assertFalse(report['passed'])  # Empty actor fixtures do not pass the phase gate.
        self.assertFalse(any(process.terminated for process in processes))
        self.assertEqual(report['initial_configuration']['bytes'], 7844)
        for process in processes:
            self.assertEqual(process.command[process.command.index('--map') + 1], 'duel1')
            self.assertEqual(process.command[process.command.index('--mode') + 1], 'training')

    def test_deadline_still_uses_the_same_owned_cleanup_order(self):
        code, report, events, processes = self.simulate()
        self.assertEqual(code, 0)
        self.assertEqual(report['stop_reason'], 'bounded_deadline')
        self.assertEqual(events[-2:], ['shutdown_client', 'shutdown_host'])
        self.assertTrue(report['completion']['passed'])
        for process in processes:
            duration = int(process.command[process.command.index('--seconds') + 1])
            self.assertGreater(duration, 5, 'Children must remain alive for ordered supervisor cleanup')

    def test_crashed_client_still_stops_host_and_reports_failed_lifecycle(self):
        code, report, events, _ = self.simulate(exit_early=True)
        self.assertEqual(code, 1)
        self.assertIn('client exited', report['error'])
        self.assertEqual(events[-1], 'shutdown_host')
        self.assertFalse(report['completion']['passed'])

    def test_client_f10_retains_living_host_until_client_postflight_finishes(self):
        code, report, events, _ = self.simulate(user_exit=True)
        self.assertEqual(code, 0)
        self.assertEqual(report['stop_reason'], 'client_requested_shutdown')
        self.assertEqual(events[-2:], ['user_f10_client', 'shutdown_host'])
        self.assertTrue(report['completion']['passed'])

    def test_wrong_child_receipt_never_writes_a_stop_file_but_exact_runner_is_retired(self):
        code, report, events, processes = self.simulate(stop_at=2, wrong_identity=True)
        self.assertEqual(code, 1)
        self.assertTrue(report['cleanup_errors'])
        self.assertEqual(events[-2:], ['terminate_client', 'terminate_host'])
        self.assertTrue(all(process.terminated for process in processes))
        self.assertFalse(report['completion']['passed'])

    def test_progress_io_failure_does_not_bypass_child_cleanup(self):
        code, report, events, _ = self.simulate(stop_at=2, progress_error=True)
        self.assertEqual(code, 1)
        self.assertIn('progress', report['error'])
        self.assertEqual(events[-2:], ['shutdown_client', 'shutdown_host'])

    def test_interactive_client_waits_for_host_action_completed_after_bind(self):
        code, report, _, processes = self.simulate(stop_at=3, host_ready_at=1)
        self.assertEqual(code, 0)
        client = next(process for process in processes if process.role == 'client')
        self.assertGreaterEqual(client.launched_at, 1)
        self.assertTrue(report['host_listen_verified'])

    def test_bounded_client_also_waits_for_host_action_completed_after_bind(self):
        _, report, _, processes = self.simulate(host_ready_at=1, bounded=True)
        client = next(process for process in processes if process.role == 'client')
        self.assertGreaterEqual(client.launched_at, 1)
        self.assertTrue(report['host_listen_verified'])
        self.assertTrue(report['observation_completed'])

    def test_configured_bounded_pair_launches_selected_ffa_map_and_initial(self):
        config = {**deepcopy(DEFAULTS), 'map_id': 'ascent', 'mode': 'deathmatch'}
        _, report, _, processes = self.simulate(bounded=True, selected_config=config)
        self.assertEqual(report['map_id'], 'ascent')
        self.assertEqual(report['training_mode'], 'deathmatch')
        self.assertEqual(report['initial_configuration']['mode'], 'deathmatch')
        for process in processes:
            self.assertEqual(process.command[process.command.index('--map') + 1], 'ascent')
            self.assertEqual(process.command[process.command.index('--mode') + 1], 'training')


if __name__ == '__main__':
    unittest.main()
