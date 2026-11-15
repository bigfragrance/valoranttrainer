"""Isolated marker, CLI and evidence tests; never launch a game or server."""
from copy import deepcopy
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import run_local_server as runner


def finite_ffa():
    return {'mode': 'deathmatch', 'infinite_ammo': False, 'invincible': False,
            'paused': False, 'total_players': 14}


class NativeFixtureMarkerTests(unittest.TestCase):
    def test_legacy_markers_remain_byte_exact(self):
        for index, name in enumerate(('shoot', 'shop', 'shoot-target'), 1):
            with self.subTest(name=name):
                self.assertEqual(runner.native_test_marker(name, 7), struct.pack(
                    '<Q4IQ8I', 0x31545345544E534C, 1, 64, 1, index, 7,
                    500, 500, 10000, 20000, 0, 0, 0, 0))

    def test_ffa_has_a_separate_scenario_and_bounded_native_wall_deadline(self):
        marker = runner.native_test_marker('ffa-battle', 2**64 - 1)
        self.assertEqual(len(marker), 64)
        fields = struct.unpack('<Q4IQ8I', marker)
        self.assertEqual(fields[:5], (0x31545345544E534C, 1, 64, 1, 4))
        self.assertEqual(fields[5:10], (2**64 - 1, 500, 500, 15000, 660000))
        self.assertEqual(fields[10:], (0, 0, 0, 0))

    def test_unknown_scenario_or_nonce_is_rejected(self):
        for scenario, nonce in [('battle', 1), ('', 1), ('ffa-battle', 0),
                                ('ffa-battle', -1), ('ffa-battle', 2**64), ('ffa-battle', True)]:
            with self.subTest(scenario=scenario, nonce=nonce):
                with self.assertRaises(ValueError):
                    runner.native_test_marker(scenario, nonce)

    def test_ffa_requires_existing_finite_unpaused_noninvincible_configuration(self):
        for key, value in [('mode', 'practice'), ('infinite_ammo', True),
                           ('infinite_ammo', None), ('invincible', True),
                           ('paused', True), ('total_players', 1), ('total_players', True)]:
            with self.subTest(key=key, value=value):
                config = finite_ffa()
                config[key] = value
                before = deepcopy(config)
                with self.assertRaises(ValueError):
                    runner.native_test_policy('ffa-battle', config, 86400)
                self.assertEqual(config, before)

    def test_only_ffa_is_clamped_and_no_settings_are_mutated(self):
        config = finite_ffa()
        before = deepcopy(config)
        self.assertEqual(runner.native_test_policy('ffa-battle', config, 86400), 720)
        self.assertEqual(runner.native_test_policy('ffa-battle', config, 35), 35)
        self.assertEqual(runner.native_test_policy('shoot-target', config, 86400), 86400)
        self.assertEqual(config, before)

    def test_battle_cli_is_only_sent_to_explicit_owned_client(self):
        options = dict(directory=ROOT / '.local/fixture-not-created', proof_id='a' * 32,
                       port=17777, transport='ip', seconds=720, bootstrap=ROOT / 'fixture.bin',
                       interactive=True, mode='training', scenario='ffa-battle')
        host, client = (runner.runner_arguments(role, **options) for role in ('host', 'client'))
        self.assertNotIn('--native-test-scenario', host)
        self.assertEqual(client[client.index('--native-test-scenario') + 1], 'ffa-battle')
        options['interactive'] = False
        with self.assertRaises(ValueError):
            runner.runner_arguments('client', **options)
        options.update(interactive=True, mode='navigation')
        with self.assertRaises(ValueError):
            runner.runner_arguments('client', **options)


def fixture_sample():
    return {'native_integration_test': {'scenario': 4, 'nonce': 91, 'configured': True,
            'cli_matched': True, 'armed': True, 'finished': True, 'failed': False,
            'actions_completed': True, 'error': 0,
            'ffa_battle': {'match_end_observed': True}}}


class NativeFixtureEvidenceTests(unittest.TestCase):
    def test_exact_finished_input_fixture_still_never_claims_match_acceptance(self):
        result = runner.native_fixture_observation(fixture_sample(), 'ffa-battle', 91)
        self.assertTrue(result['exact_marker_observed'])
        self.assertTrue(result['input_actions_completed'])
        self.assertTrue(result['match_end_observed'])
        for key in ('host_result_verified', 'full_match_acceptance_verified',
                    'physical_input_verified', 'playable_verified'):
            self.assertFalse(result[key])

    def test_stale_wrong_or_unarmed_marker_cannot_finish_this_fixture(self):
        for key, value in [('scenario', 3), ('scenario', 4.0), ('nonce', 90), ('nonce', 91.0),
                           ('configured', False), ('cli_matched', False), ('armed', False)]:
            with self.subTest(key=key, value=value):
                sample = fixture_sample()
                sample['native_integration_test'][key] = value
                result = runner.native_fixture_observation(sample, 'ffa-battle', 91)
                self.assertFalse(result['exact_marker_observed'])
                self.assertFalse(result['finished'])
                self.assertFalse(result['input_actions_completed'])

    def test_failure_or_no_real_input_cannot_count_as_completed_actions(self):
        for key, value in [('failed', True), ('actions_completed', False), ('error', 4913), ('error', False)]:
            with self.subTest(key=key):
                sample = fixture_sample()
                sample['native_integration_test'][key] = value
                result = runner.native_fixture_observation(sample, 'ffa-battle', 91)
                self.assertFalse(result['input_actions_completed'])
                self.assertFalse(result['full_match_acceptance_verified'])

    def test_missing_or_malformed_battle_never_supplies_a_match_end(self):
        for battle in (None, [], {}, {'match_end_observed': False}):
            with self.subTest(battle=battle):
                sample = fixture_sample()
                sample['native_integration_test']['ffa_battle'] = battle
                self.assertFalse(runner.native_fixture_observation(sample, 'ffa-battle', 91)['match_end_observed'])


class NativeMarkerExecutableTests(unittest.TestCase):
    """This tiny standalone parser executable never calls Tick or any engine API."""
    @classmethod
    def setUpClass(cls):
        cls.program = ROOT / 'work/local-server-playable/input-authority/native-test-ffa/marker-contract.exe'
        if not cls.program.is_file():
            raise unittest.SkipTest('Build the isolated marker-contract.cpp harness to test native parsing')
        cls.temp_root = ROOT / '.local/runtime/temp'
        cls.temp_root.mkdir(parents=True, exist_ok=True)

    def parse(self, marker):
        with tempfile.TemporaryDirectory(prefix='ffa-marker-', dir=self.temp_root) as temporary:
            if marker is not None:
                (Path(temporary) / 'native-test-scenario.bin').write_bytes(marker)
            process = subprocess.run([str(self.program), temporary], cwd=ROOT,
                                     capture_output=True, text=True, check=True, timeout=5)
            return json.loads(process.stdout)

    def test_native_valid_markers_are_configured_but_never_armed(self):
        for name in runner.NATIVE_TEST_SCENARIOS:
            with self.subTest(name=name):
                result = self.parse(runner.native_test_marker(name, 13))
                self.assertTrue(result['configured'])
                self.assertFalse(result['armed'])
                self.assertEqual(result['error'], 0)

    def test_missing_marker_remains_inert(self):
        result = self.parse(None)
        self.assertFalse(result['configured'])
        self.assertFalse(result['armed'])

    def test_actual_native_diagnostics_are_valid_json_and_do_not_invent_a_run(self):
        with tempfile.TemporaryDirectory(prefix='ffa-state-', dir=self.temp_root) as temporary:
            (Path(temporary) / 'native-test-scenario.bin').write_bytes(runner.native_test_marker('ffa-battle', 19))
            process = subprocess.run([str(self.program), 'state', temporary], cwd=ROOT,
                                     capture_output=True, text=True, check=True, timeout=5)
            state = json.loads(process.stdout)
            self.assertTrue(state['configured'])
            self.assertFalse(state['armed'])
            self.assertTrue(state['ffa_battle']['enabled'])
            self.assertFalse(state['ffa_battle']['match_end_observed'])
            self.assertFalse(state['ffa_battle']['playable_verified'])
            self.assertFalse(state['passed'])

    def test_native_rejects_bad_schema_or_unbounded_battle(self):
        base = list(struct.unpack('<Q4IQ8I', runner.native_test_marker('ffa-battle', 17)))
        for index, value in [(0, 0), (1, 2), (2, 63), (3, 0), (4, 0), (4, 5), (5, 0),
                             (8, 15001), (9, 659999), (9, 660001), (10, 1)]:
            with self.subTest(index=index, value=value):
                fields = base.copy()
                fields[index] = value
                result = self.parse(struct.pack('<Q4IQ8I', *fields))
                self.assertEqual(result['error'], 4904)
                self.assertFalse(result['configured'])
                self.assertFalse(result['armed'])
        result = self.parse(runner.native_test_marker('ffa-battle', 17) + b'x')
        self.assertEqual(result['error'], 4904)

    def test_native_token_matching_requires_whole_original_cli_token(self):
        token = '-LocalServerNativeTest=ffa-battle'
        for command, expected in [(token, True), ('map ' + token + ' -log', True),
                                  ('x' + token, False), (token + '-extra', False),
                                  ('-LocalServerNativeTest=shoot-target', False)]:
            with self.subTest(command=command):
                result = subprocess.run([str(self.program), 'token', command, token], cwd=ROOT,
                                        capture_output=True, text=True, check=True, timeout=5)
                self.assertEqual(json.loads(result.stdout)['matched'], expected)


if __name__ == '__main__':
    unittest.main()
