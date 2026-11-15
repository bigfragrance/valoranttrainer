"""Selected-map bootstrap and candidate-only map proof; no game execution."""
from copy import deepcopy
import hashlib
from io import StringIO
import json
from pathlib import Path
import shutil
import struct
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import local_server_controller as controller
import local_server_maps as maps
import native_maps
import run_local_server as runner
import run_native_baseline as baseline
from ini_bootstrap import decode, encode
from training_settings_v2 import DEFAULTS
from test_ini_bootstrap import snapshot


def map_report(root):
    (root / 'profiles').mkdir(parents=True, exist_ok=True)
    shutil.copyfile(Path(__file__).resolve().parents[1] / 'profiles/playable-heroes.json',
                    root / 'profiles/playable-heroes.json')
    controller.atomic_json(root / 'reports/training-maps.json', {'maps': [
        {'id': 'ascent', 'kind': 'standard_5v5', 'world_path': '/Game/Maps/Ascent/Ascent',
         'validation': {'runtime': 'passed', 'navigation': 'native_path_and_movement_verified',
                        'spawn_collision': 'four_original_hero_capsules_verified', 'static_metadata': 'verified_original_bytes'}},
        {'id': 'debug', 'kind': 'diagnostic', 'world_path': '/Game/Maps/Init/Init', 'validation': {'runtime': 'passed'}}]})


class SelectedMapTests(unittest.TestCase):
    def test_all_catalog_5v5_maps_accept_practice_and_ffa_without_a_verified_claim(self):
        original = native_maps.training_maps()
        expected = {key for key, row in original.items() if row.get('kind') == 'standard_5v5'}
        self.assertTrue(expected)
        for key in expected:
            for mode in ('practice', 'deathmatch'):
                with self.subTest(key=key, mode=mode):
                    config = {**deepcopy(DEFAULTS), 'map_id': key, 'mode': mode}
                    self.assertEqual(controller.check_candidate_config(config)['map_id'], key)
                    self.assertEqual(maps.selected_map(key, mode)['world_path'], native_maps.map_package(key, training=True))
        with self.assertRaises(ValueError):
            controller.check_candidate_config({**deepcopy(DEFAULTS), 'mode': 'deathmatch'})
        with self.assertRaises(ValueError):
            controller.check_candidate_config({**deepcopy(DEFAULTS), 'map_id': 'unregistered'})

    def test_selected_bootstrap_initial_and_both_runner_commands_match_map_and_mode(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            map_report(root)
            source = root / 'source.bin'
            original = encode(snapshot())
            source.write_bytes(original)
            for mode, number in (('practice', 0), ('deathmatch', 1)):
                directory = root / '.local/local-server' / ('pair-' + mode)
                directory.mkdir(parents=True)
                config = {**deepcopy(DEFAULTS), 'map_id': 'ascent', 'mode': mode}
                with patch.object(runner, 'ROOT', root):
                    selected = runner.prepare_training_session(directory, config, source)
                self.assertEqual(selected['training_mode'], mode)
                self.assertEqual(selected['engine_mode'], 'training')
                document = decode(Path(selected['bootstrap']['archive']).read_bytes())
                settings = dict(next(section['pairs'] for name, section in document['categories'][0]['file']['sections']
                                     if name == '/Script/EngineSettings.GameMapsSettings'))
                self.assertEqual(settings['GameDefaultMap'], '/Game/Maps/Ascent/Ascent')
                self.assertEqual(len(document['categories']), len(snapshot()['categories']))
                for actual, prior in zip(document['categories'], snapshot()['categories']):
                    self.assertEqual(actual['category'], prior['category'])
                    self.assertEqual(actual['file']['sections'][0], prior['file']['sections'][0])
                payload = (directory / 'session-initial.bin').read_bytes()
                self.assertEqual(struct.unpack_from('<I', payload, 20)[0], number)
                self.assertEqual(payload[6752:6816].split(b'\0', 1)[0], b'ascent')
                for role in ('host', 'client'):
                    command = runner.runner_arguments(role, directory=directory, proof_id='a' * 32,
                                                      port=17777, transport='ip', seconds=35,
                                                      bootstrap=Path(selected['bootstrap']['archive']),
                                                      map_id='ascent', mode='training')
                    self.assertEqual(command[command.index('--map') + 1], 'ascent')
                    self.assertEqual(command[command.index('--mode') + 1], 'training')
                    self.assertNotIn('--controller-settings', command)
                self.assertEqual(source.read_bytes(), original)

    def test_configured_child_receipts_cannot_change_map_or_engine_mode(self):
        from test_local_server_controller import pair_record, child_record, project
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            project(root)
            pair = {**pair_record(root), 'map_id': 'ascent', 'training_mode': 'deathmatch', 'engine_mode': 'training'}
            child = {**child_record(root, 'host', 81), 'map_number': 'ascent', 'mode': 'training'}
            controller.validate_child_record(child, 'host', pair, root=root)
            for patch_value in ({'map_number': 'bonsai'}, {'mode': 'navigation'}):
                with self.assertRaisesRegex(RuntimeError, 'map or mode'):
                    controller.validate_child_record({**child, **patch_value}, 'host', pair, root=root)

    def test_default_selected_bootstrap_uses_the_verified_core_snapshot_for_that_map(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            map_report(root)
            source = root / '.local/native-baseline/capture/core-config.json'
            source.parent.mkdir(parents=True)
            source.write_text(json.dumps(snapshot()), encoding='utf-8')
            controller.atomic_json(root / 'project.json', {'expected_snapshot_sha256': 'fixture-image'})
            controller.atomic_json(root / 'reports/native-config-validation.json',
                                   {'target_sha256': 'fixture-image', 'snapshot': {
                                       'path': str(source), 'sha256': hashlib.sha256(source.read_bytes()).hexdigest()}})
            directory = root / '.local/local-server/pair-default'
            directory.mkdir(parents=True)
            config = {**deepcopy(DEFAULTS), 'map_id': 'ascent', 'mode': 'deathmatch'}
            with patch.object(runner, 'ROOT', root):
                selected = runner.prepare_training_session(directory, config)
            self.assertEqual(selected['bootstrap']['map_number'], 'ascent')
            self.assertEqual(selected['bootstrap']['package'], '/Game/Maps/Ascent/Ascent')
            self.assertTrue(selected['bootstrap']['round_trip'])
            self.assertEqual(selected['initial_configuration']['mode'], 'deathmatch')


class ProofIsolationTests(unittest.TestCase):
    def test_preserved_proof_stays_duel1_navigation_while_candidate_allows_selected_training(self):
        self.assertEqual(baseline.network_world_package('proof', 1, 'navigation'), native_maps.MAPS[1])
        self.assertEqual(baseline.network_world_package('playable', 1, 'navigation'), native_maps.MAPS[1])
        self.assertEqual(baseline.network_world_package('playable', 'ascent', 'training'), '/Game/Maps/Ascent/Ascent')
        for flavor, selection, mode in (('proof', 'ascent', 'training'), ('proof', 'duel1', 'training'),
                                        ('proof', 1, 'training'), ('proof', True, 'navigation'),
                                        ('playable', 'unknownmap', 'training'), ('playable', 'ascent', 'navigation')):
            with self.subTest(flavor=flavor, selection=selection, mode=mode), self.assertRaises(ValueError):
                baseline.network_world_package(flavor, selection, mode)

    def test_network_training_loads_only_selected_candidate_even_with_a_legacy_stage(self):
        candidate, old = Mock(), Mock()
        process, context = object(), object()
        args = SimpleNamespace(network_probe='client', network_port=17777, network_output=Path('fixture'),
                               network_transport='ip', module_stage=8)
        with patch.dict(sys.modules, {'native_module': SimpleNamespace(load_owned_module=old)}):
            baseline.load_selected_training_module(args, process, context, candidate)
        candidate.load.assert_called_once_with(process, context, role='client', port=17777,
                                               output=Path('fixture'), transport='ip')
        old.assert_not_called()
        args.network_probe = None
        with patch.dict(sys.modules, {'native_module': SimpleNamespace(load_owned_module=old)}):
            baseline.load_selected_training_module(args, process, context)
        old.assert_called_once_with(process, context, 8)

    def test_network_cli_rejects_legacy_controller_sidecar_before_build_or_process_actions(self):
        command = ['baseline', '--network-probe', 'host', '--network-flavor', 'playable', '--map', 'ascent',
                   '--mode', 'training', '--bootstrap', 'fixture.bin', '--original-game-instance',
                   '--network-output', 'fixture-output', '--result-file', 'fixture-result.json',
                   '--network-proof-id', 'a' * 32, '--controller-settings', 'old-controller.json']
        candidate = Mock()
        with patch.object(sys, 'argv', command), patch.object(baseline, 'network_module_for_flavor', return_value=candidate), \
                patch('subprocess.Popen') as process, patch('sys.stderr', new=StringIO()):
            with self.assertRaises(SystemExit):
                baseline.main()
        candidate.verified_build.assert_not_called()
        process.assert_not_called()


class CandidateMapEvidenceTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        map_report(self.root)
        module = self.root / '.local/local-server/module/local_server_bridge.dll'
        module.parent.mkdir(parents=True)
        module.write_bytes(b'candidate fixture, never loaded')
        self.module = module
        self.digest = hashlib.sha256(module.read_bytes()).hexdigest()
        controller.atomic_json(module.parent / 'build.json', {'module_sha256': self.digest})
        self.path = self.root / '.local/local-server/pair-fixture/map-case.json'
        self.evidence = {'schema_version': 1, 'kind': maps.CASE_KIND, 'backend': maps.BACKEND,
                         'network_flavor': 'playable', 'map_id': 'ascent', 'mode': 'practice',
                         'module_sha256': self.digest, 'passed': True, 'checks': {'selected_world_and_original_player': True}}
        self.index = {'schema_version': 1, 'backend': maps.BACKEND, 'source': maps.VALIDATION_SOURCE,
                      'cases': [{'map_id': 'ascent', 'mode': 'practice', 'module_sha256': self.digest,
                                 'evidence': self.path.relative_to(self.root).as_posix()}]}

    def tearDown(self):
        self.temporary.cleanup()

    def save(self):
        controller.atomic_json(self.path, self.evidence)
        controller.atomic_json(self.root / maps.VALIDATION_INDEX, self.index)

    def ascent(self):
        return next(row for row in maps.catalog_maps(self.root) if row['id'] == 'ascent')

    def test_old_trainer_passes_are_not_exposed_as_candidate_runtime_navigation_or_spawn_proof(self):
        row = self.ascent()
        self.assertEqual(row['validation']['runtime'], 'not_tested')
        self.assertNotIn('navigation', row['validation'])
        self.assertNotIn('spawn_collision', row['validation'])
        self.assertEqual(row['validation']['static_metadata'], 'verified_original_bytes')
        self.assertEqual(row['local_server_validation']['practice']['status'], 'not_tested')
        self.assertEqual(row['local_server_validation']['deathmatch']['status'], 'not_tested')
        self.assertNotIn('debug', {row['id'] for row in maps.catalog_maps(self.root)})

    def test_matching_candidate_case_is_scoped_to_one_mode_and_never_marks_playable(self):
        self.save()
        row = self.ascent()
        self.assertEqual(row['local_server_validation']['practice']['status'], 'passed')
        self.assertFalse(row['local_server_validation']['practice']['playable_verified'])
        self.assertEqual(row['local_server_validation']['deathmatch']['status'], 'not_tested')

    def test_old_report_directory_changed_module_or_failed_check_cannot_pass(self):
        self.save()
        self.index['cases'][0]['evidence'] = '.local/feature-verification/old/map-case.json'
        controller.atomic_json(self.root / self.index['cases'][0]['evidence'], self.evidence)
        self.save()
        self.assertEqual(self.ascent()['local_server_validation']['practice']['status'], 'not_tested')
        self.index['cases'][0]['evidence'] = self.path.relative_to(self.root).as_posix()
        self.evidence['checks']['selected_world_and_original_player'] = False
        self.save()
        self.assertEqual(self.ascent()['local_server_validation']['practice']['status'], 'failed')
        self.module.write_bytes(b'a different build')
        self.assertEqual(self.ascent()['local_server_validation']['practice']['status'], 'not_tested')


if __name__ == '__main__':
    unittest.main()
