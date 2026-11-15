"""Evidence and cleanup tests with fixtures; no game or server is launched."""
import copy
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import run_local_server as runner


def hero_sample():
    return {
        'host': {'host_shooter_world_ready': True, 'local_services_ready': True,
                 'authority_hero_spawned': True, 'authority_hero_possessed': True,
                 'authority_hero': '30000', 'authority_controller': '40000', 'authority_player_state': '50000',
                 'host_connections': [{'state': 3, 'login_state': 3, 'owner_links_verified': True,
                                       'controller_role': 3, 'pawn_role': 3, 'controller': '40000',
                                       'player_state': '50000', 'pawn': '30000'}]},
        'client': {'local_services_ready': True, 'local_hero_received': True,
                   'connection_state': 3, 'own_local_links_verified': True,
                   'local_controller_is_connection_owner': True, 'local_controller_role': 2,
                   'current_world': '20000', 'initial_world': '10000', 'pending_netgame': '0',
                   'world_authority_game_mode': '0', 'local_player_state': '60000',
                   'local_pawn': '70000', 'local_pawn_role': 2},
    }


class HeroLifecycleEvidenceTests(unittest.TestCase):
    def test_full_hero_lifecycle_is_a_phase_gate_without_playable_or_input_claim(self):
        result = runner.analyze_gameplay_samples([hero_sample()])
        self.assertTrue(result['shooter_hero_round_trip_proven'])
        self.assertTrue(result['native_control_join_proven'])
        self.assertFalse(result['playable_verified'])
        self.assertFalse(result['authority_movement_proven'])
        self.assertFalse(result['server_ai_movement_replicated_proven'])

    def test_default_pawn_connection_cannot_pass_the_shooter_phase(self):
        sample = hero_sample()
        sample['host']['host_shooter_world_ready'] = False
        sample['client']['local_hero_received'] = False
        result = runner.analyze_gameplay_samples([sample])
        self.assertTrue(result['native_control_join_proven'])
        self.assertFalse(result['shooter_hero_round_trip_proven'])

    def test_local_bootstrap_world_and_missing_network_possession_fail(self):
        for key, value in (('current_world', '10000'), ('local_controller_is_connection_owner', False),
                           ('local_pawn_role', 3), ('local_player_state', '0')):
            with self.subTest(key=key):
                sample = hero_sample()
                sample['client'][key] = value
                self.assertFalse(runner.analyze_gameplay_samples([sample])['shooter_hero_round_trip_proven'])

    def test_unrelated_server_pawn_or_player_state_cannot_prove_authority_hero(self):
        for key in ('controller', 'player_state', 'pawn'):
            with self.subTest(key=key):
                sample = hero_sample()
                sample['host']['host_connections'][0][key] = '90000'
                result = runner.analyze_gameplay_samples([sample])
                self.assertFalse(result['authority_hero_possessed'])
                self.assertFalse(result['shooter_hero_round_trip_proven'])

    def test_unrelated_time_slices_cannot_combine_into_a_round_trip(self):
        first, second = hero_sample(), hero_sample()
        first['client']['local_hero_received'] = False
        second['host']['authority_hero_possessed'] = False
        result = runner.analyze_gameplay_samples([first, second])
        self.assertTrue(result['authority_hero_possessed'])
        self.assertTrue(result['local_hero_received'])
        self.assertFalse(result['shooter_hero_round_trip_proven'])

    def test_native_gameplay_error_prevents_round_trip_acceptance(self):
        for side in ('host', 'client'):
            for key in ('error', 'playable_error'):
                with self.subTest(side=side, key=key):
                    sample = hero_sample()
                    sample[side][key] = 3102
                    self.assertFalse(runner.analyze_gameplay_samples([sample])['shooter_hero_round_trip_proven'])


class ServerRunnerTests(unittest.TestCase):
    def test_live_progress_is_bounded_without_discarding_verification_history(self):
        samples = [{'elapsed': n, 'host': {'stage': str(n)}, 'client': {}} for n in range(120)]
        report = {'samples': samples, 'total_samples': 200, 'native_gates': {'real_phase_gate': True}}
        current = runner.progress_snapshot(report)
        self.assertEqual(len(current['samples']), 1)
        self.assertEqual(current['samples'][0]['elapsed'], 119)
        self.assertEqual(current['total_samples'], 200)
        self.assertEqual(current['history_samples_retained_for_verification'], 120)
        self.assertEqual(current['native_gates'], report['native_gates'])
        self.assertEqual(len(report['samples']), 120)

    def test_explicit_native_fixture_never_reaches_authority_process(self):
        options = {'directory': Path('session').resolve(), 'proof_id': 'a' * 32,
                   'port': 17777, 'transport': 'ip', 'seconds': 35,
                   'bootstrap': Path('fixture.bin'), 'interactive': True, 'scenario': 'shoot'}
        host = runner.runner_arguments('host', **options)
        client = runner.runner_arguments('client', **options)
        self.assertNotIn('--native-test-scenario', host)
        self.assertEqual(client[client.index('--native-test-scenario') + 1], 'shoot')
        options.pop('scenario')
        self.assertNotIn('--native-test-scenario', runner.runner_arguments('client', **options))

    def test_host_and_client_choose_playable_and_exact_peer(self):
        directory = Path('session').resolve()
        options = {'directory': directory, 'proof_id': 'a' * 32, 'port': 17777,
                   'transport': 'ip', 'seconds': 35, 'bootstrap': Path('fixture.bin')}
        host = runner.runner_arguments('host', **options)
        client = runner.runner_arguments('client', **options)
        for command in (host, client):
            self.assertEqual(command[command.index('--network-flavor') + 1], 'playable')
            self.assertEqual(command[command.index('--network-transport') + 1], 'ip')
            self.assertEqual(command[command.index('--network-proof-id') + 1], options['proof_id'])
            self.assertIn('--original-game-instance', command)
        self.assertNotIn('--network-peer', host)
        self.assertEqual(client[client.index('--network-peer') + 1], str(directory / 'host-result.active.json'))

    def active(self, root):
        return {'pid': 41, 'run_dir': str(root / '.local/native-baseline/owned'),
                'network_probe': {'role': 'host', 'proof_id': 'session', 'flavor': 'playable'},
                'network_probe_identity': {'pid': 41, 'network_flavor': 'playable', 'module_sha256': 'module',
                                           'published_from_original_popen_handle': True}}

    def test_stop_file_is_bound_to_this_flavor_session_and_owned_directory(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary).resolve()
            active = self.active(root)
            options = {'proof_id': 'session', 'role': 'host', 'module_sha256': 'module'}
            with patch.object(runner, 'ROOT', root):
                self.assertEqual(runner.stop_request_path(active, **options),
                                 root / '.local/native-baseline/owned/stop-request.json')
                for item in ('proof_id', 'role', 'flavor'):
                    with self.subTest(item=item):
                        changed = copy.deepcopy(active)
                        changed['network_probe'][item] = 'other'
                        with self.assertRaisesRegex(RuntimeError, 'ownership record'):
                            runner.stop_request_path(changed, **options)
                changed = copy.deepcopy(active)
                changed['run_dir'] = str(root / '.local/elsewhere')
                with self.assertRaisesRegex(RuntimeError, 'ownership directory'):
                    runner.stop_request_path(changed, **options)

    def test_cleanup_timeout_terminates_only_its_passed_runner_handle(self):
        process = Mock()
        process.wait.side_effect = [subprocess.TimeoutExpired('fixture', 15),
                                    subprocess.TimeoutExpired('fixture', 5), None]
        process.poll.return_value = 1
        report = {}
        runner.wait_runner(process, 'client', report)
        process.terminate.assert_called_once_with()
        process.kill.assert_called_once_with()
        self.assertTrue(report['client_runner_forced_stop'])
        self.assertEqual(report['client_runner_exit_code'], 1)

    def test_requested_shutdown_needs_no_forced_termination(self):
        process = Mock()
        process.poll.return_value = 0
        report = {}
        runner.wait_runner(process, 'host', report)
        process.terminate.assert_not_called()
        process.kill.assert_not_called()
        self.assertEqual(report, {'host_runner_exit_code': 0})


class ServerCompletionTests(unittest.TestCase):
    def complete(self):
        result = {'owned_process_exited': True, 'stop_reason': 'requested_shutdown',
                  'image_unchanged': True, 'postflight': {'ready': True}}
        return {'observation_completed': True, 'host_cleanup': {'cleaned': True},
                'protected_builds_unchanged': True, 'candidate_build_unchanged': True,
                'host_runner_exit_code': 0, 'client_runner_exit_code': 0,
                'host_critical_errors': 0, 'client_critical_errors': 0,
                'host_result': copy.deepcopy(result), 'client_result': copy.deepcopy(result)}

    def test_full_bounded_cleanup_passes(self):
        self.assertTrue(runner.assess_completion(self.complete())['passed'])

    def test_short_observation_or_changed_preserved_or_candidate_build_fails(self):
        for key in ('observation_completed', 'protected_builds_unchanged', 'candidate_build_unchanged'):
            with self.subTest(key=key):
                report = self.complete()
                report[key] = False
                self.assertFalse(runner.assess_completion(report)['passed'])

    def test_crash_missing_log_critical_error_or_forced_stop_fails(self):
        reports = []
        report = self.complete()
        report['client_result']['stop_reason'] = 'target_exited'
        reports.append(report)
        for count in (None, 1):
            report = self.complete()
            report['client_critical_errors'] = count
            reports.append(report)
        report = self.complete()
        report['host_runner_forced_stop'] = True
        reports.append(report)
        for report in reports:
            self.assertFalse(runner.assess_completion(report)['passed'])

    def test_cleanup_errors_and_driver_still_alive_fail(self):
        report = self.complete()
        report['cleanup_errors'] = ['fixture failure']
        self.assertFalse(runner.assess_completion(report)['passed'])
        report = self.complete()
        report['host_cleanup']['cleaned'] = False
        self.assertFalse(runner.assess_completion(report)['passed'])


if __name__ == '__main__':
    unittest.main()
