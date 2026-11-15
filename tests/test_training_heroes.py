"""Hero configuration and session sidecar fixtures. Never launch a game."""
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import shutil
import struct
import sys
import tempfile
import threading
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch
from urllib.error import HTTPError
from urllib.request import Request, build_opener, ProxyHandler

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import local_server_controller as control
import run_local_server as runner
import training_heroes as heroes
from training_control import initial_payload, pack_configuration
from training_settings_v2 import DEFAULTS, Settings, merge
from training_server import Application, Runtime, make_server

ROOT = Path(__file__).resolve().parents[1]
PHOENIX = 'eb93336a449b9c1b0a54a891f7921d69'


def project_profile(root):
    destination = root / 'profiles/playable-heroes.json'
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(ROOT / 'profiles/playable-heroes.json', destination)


def map_report(root):
    control.atomic_json(root / 'reports/training-maps.json', {'maps': [
        {'id': 'duel1', 'kind': 'duel', 'world_path': '/Game/Maps/Duel/Duel_1/Skirmish_A'}]})


def supported_build(root):
    directory = root / '.local/local-server/module'
    captured = directory / 'source-snapshot' / heroes.SELECTION_SOURCE
    captured.parent.mkdir(parents=True, exist_ok=True)
    captured.write_bytes(b'fixture source snapshot, never executable')
    manifest = {'module_sha256': 'c' * 64,
                'source_sha256': {name: 'e' * 64 for name in control.TRAINING_SOURCES}}
    manifest['source_sha256'][heroes.SELECTION_SOURCE] = hashlib.sha256(captured.read_bytes()).hexdigest()
    control.atomic_json(directory / 'build.json', manifest)
    return manifest


class HeroSettingsTests(unittest.TestCase):
    def test_legacy_settings_read_keeps_bytes_then_explicit_save_persists_jett(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'config.json'
            old = deepcopy(DEFAULTS)
            old.pop('local_hero_id')
            old.update(revision=73, seed=9001, infinite_ammo=False)
            path.write_text(json.dumps(old), encoding='utf-8')
            before = path.read_bytes()
            settings = Settings(path)
            self.assertEqual(settings.read()['local_hero_id'], heroes.DEFAULT_HERO_ID)
            self.assertEqual(settings.read()['revision'], 73)
            self.assertEqual(path.read_bytes(), before)
            settings.update({'paused': True}, 73)
            saved = Settings(path).read()
            self.assertEqual(saved['local_hero_id'], heroes.DEFAULT_HERO_ID)
            self.assertEqual(saved['seed'], 9001)
            self.assertFalse(saved['infinite_ammo'])

    def test_direct_runner_reads_legacy_settings_without_rewriting_them(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            path = root / '.local/fixture/settings.json'
            path.parent.mkdir(parents=True)
            old = deepcopy(DEFAULTS)
            old.pop('local_hero_id')
            path.write_text(json.dumps(old), encoding='utf-8')
            before = path.read_bytes()
            with patch.object(runner, 'ROOT', root):
                result = runner.controller_configuration(path)
            self.assertEqual(result['local_hero_id'], heroes.DEFAULT_HERO_ID)
            self.assertEqual(path.read_bytes(), before)

    def test_saved_hero_is_real_profile_identity_and_survives_restart(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'config.json'
            settings = Settings(path)
            value = settings.update({'local_hero_id': PHOENIX}, 0)
            self.assertEqual(value['revision'], 1)
            self.assertEqual(Settings(path).read()['local_hero_id'], PHOENIX)
            before = path.read_bytes()
            for invalid in ('0' * 32, 'Phoenix', True, None, 1, PHOENIX.upper()):
                with self.subTest(invalid=invalid), self.assertRaises(ValueError):
                    settings.update({'local_hero_id': invalid}, 1)
            self.assertEqual(path.read_bytes(), before)

    def test_selection_does_not_repurpose_a_v2_reserved_float_or_change_initial(self):
        before = deepcopy(DEFAULTS)
        after = {**deepcopy(before), 'local_hero_id': PHOENIX}
        self.assertEqual(pack_configuration(before), pack_configuration(after))
        self.assertEqual(len(pack_configuration(after)), 72)
        self.assertEqual(struct.unpack_from('<f', pack_configuration(after), 60)[0], 0)
        self.assertEqual(initial_payload(before, {}), initial_payload(after, {}))
        self.assertEqual(len(initial_payload(after, {})), 7844)

    def test_original_backend_rejects_unsupported_hero_before_persistence(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            project_profile(root)
            app = Application(root, backend='original', runtime=False)
            with self.assertRaisesRegex(ValueError, '固定使用捷风'):
                app.update({'local_hero_id': PHOENIX}, 0)
            self.assertEqual(app.settings.read(), DEFAULTS)
            self.assertFalse(app.settings.path.exists())
            live = Runtime.__new__(Runtime)
            live.backend_adapter = None
            with self.assertRaisesRegex(ValueError, '固定使用捷风'):
                live.check_config({**deepcopy(DEFAULTS), 'local_hero_id': PHOENIX})

    def test_changing_local_hero_restarts_pair_without_sending_live_configure(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            project_profile(root)
            map_report(root)
            app = Application(root, backend=control.BACKEND, runtime=False)
            runtime = SimpleNamespace(sync_native=Mock(), check_config=Mock(),
                                      state=Mock(return_value={'status': 'running'}),
                                      start_async=Mock(return_value={'status': 'loading'}), apply=Mock())
            app.runtime = runtime
            result = app.update({'local_hero_id': PHOENIX}, 0)
            runtime.start_async.assert_called_once_with(restart=True)
            runtime.apply.assert_not_called()
            self.assertEqual(result['config']['local_hero_id'], PHOENIX)
            self.assertEqual(result['runtime']['status'], 'loading')


class HeroSidecarTests(unittest.TestCase):
    def test_exact_72_byte_original_identity_and_profile_digest(self):
        config = merge(DEFAULTS, {'local_hero_id': PHOENIX})
        payload = heroes.selection_payload(config)
        fields = heroes.SELECTION.unpack(payload)
        profile = heroes.hero_profile()
        self.assertEqual(len(payload), 72)
        self.assertEqual(fields[:5], (b'VTHERO01', 1, 72, 1, 1))
        self.assertEqual(fields[5:9], (0xeb93336a, 0x449b9c1b, 0x0a54a891, 0xf7921d69))
        self.assertEqual(fields[9], bytes.fromhex(profile['sha256']))

    def test_profile_mismatches_are_rejected_before_creating_sidecar(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            project_profile(root)
            profile_path = root / 'profiles/playable-heroes.json'
            original = json.loads(profile_path.read_text(encoding='utf-8'))
            for key, value in (('index', 7), ('id', '0' * 32), ('guid_words', [0, 0, 0, 0]),
                               ('playable', False), ('excluded_from_content_catalog', True)):
                changed = deepcopy(original)
                changed['heroes'][1][key] = value
                profile_path.write_text(json.dumps(changed), encoding='utf-8')
                with self.subTest(key=key), self.assertRaises(ValueError):
                    heroes.selection_payload({**deepcopy(DEFAULTS), 'local_hero_id': PHOENIX}, root=root)

    def test_initial_and_sidecar_are_bound_and_neither_can_be_overwritten(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            project_profile(root)
            directory = root / 'pair-fixture'
            directory.mkdir()
            config = merge(DEFAULTS, {'local_hero_id': PHOENIX})
            result = control.write_initial(directory, config, {}, root=root)
            path = directory / heroes.SELECTION_NAME
            before = path.read_bytes()
            self.assertEqual(result['bytes'], 7844)
            self.assertEqual(result['hero_selection']['hero_index'], 1)
            self.assertEqual(result['hero_selection']['hero_id'], PHOENIX)
            self.assertEqual(result['hero_selection']['sha256'], hashlib.sha256(before).hexdigest())
            (directory / 'session-initial.bin').unlink()
            with self.assertRaises(FileExistsError):
                control.write_initial(directory, deepcopy(DEFAULTS), {}, root=root)
            self.assertEqual(path.read_bytes(), before)
            self.assertFalse((directory / 'session-initial.bin').exists())

    def test_true_runner_initial_preparation_includes_sidecar_receipt(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            project_profile(root)
            directory = root / 'pair-fixture'
            directory.mkdir()
            package = '/Game/Maps/Duel/Duel_1/Skirmish_A'
            def bootstrap(project_root, map_id, mode, target):
                Path(target).write_bytes(b'fixture archive, never loaded')
                return {'package': package, 'archive': str(target)}
            with patch.object(runner, 'ROOT', root), \
                    patch('local_server_maps.selected_map', return_value={'spawn_anchors': []}), \
                    patch('native_maps.map_package', return_value=package), \
                    patch('run_map_probe.prepare', side_effect=bootstrap):
                receipt = runner.prepare_training_session(directory,
                    {**deepcopy(DEFAULTS), 'local_hero_id': PHOENIX})
            selected = receipt['initial_configuration']['hero_selection']
            self.assertEqual(selected['path'], str(directory / heroes.SELECTION_NAME))
            self.assertEqual(selected['hero_id'], PHOENIX)
            self.assertTrue(Path(selected['path']).is_file())

    def test_non_jett_cannot_launch_an_old_build_from_backend_or_direct_runner(self):
        config = {**deepcopy(DEFAULTS), 'local_hero_id': PHOENIX}
        old = {'module_sha256': 'b' * 64,
               'source_sha256': {name: 'e' * 64 for name in control.TRAINING_SOURCES}}
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            backend = control.LocalServerBackend(root)
            with patch('local_server_module.verified_build', return_value=old):
                with self.assertRaisesRegex(RuntimeError, '仅支持捷风'):
                    backend.prepare_launch(config, 'fixture')
            self.assertFalse((backend.controller_root / 'sessions').exists())
        with patch.object(runner, 'controller_configuration', return_value=config), \
                patch.object(runner, 'verified_build', return_value=old), \
                patch.object(runner, 'artifact_hashes', return_value={}), \
                patch('subprocess.Popen') as popen:
            with self.assertRaisesRegex(RuntimeError, '仅支持捷风'):
                runner.run_interactive(SimpleNamespace(controller_settings=None))
            popen.assert_not_called()


class HeroCapabilityTests(unittest.TestCase):
    def test_build_capability_uses_captured_source_not_current_header_presence(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            project_profile(root)
            current = root / heroes.SELECTION_SOURCE
            current.parent.mkdir(parents=True, exist_ok=True)
            current.write_text('current source only', encoding='utf-8')
            self.assertFalse(heroes.local_hero_capability(root, backend=control.BACKEND)['available'])
            supported_build(root)
            status = heroes.local_hero_capability(root, backend=control.BACKEND)
            self.assertTrue(status['available'])
            self.assertTrue(status['requires_restart'])
            self.assertFalse(status['runtime_verified'])
            self.assertEqual(status['count'], 30)
            captured = root / '.local/local-server/module/source-snapshot' / heroes.SELECTION_SOURCE
            captured.write_bytes(b'changed captured source')
            self.assertFalse(heroes.local_hero_capability(root, backend=control.BACKEND)['available'])

    def test_original_backend_never_claims_all_hero_selection_from_candidate_build(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            project_profile(root)
            supported_build(root)
            app = Application(root, runtime=False)
            status = app.capabilities()['local_hero_selection']
            self.assertFalse(status['available'])
            self.assertTrue(status['catalog_available'])
            rows = app.catalog.heroes()
            self.assertEqual(len(rows), 30)
            self.assertEqual(rows[1]['id'], PHOENIX)
            self.assertEqual(rows[1]['index'], 1)
            self.assertTrue(all(row['skills_runtime_status'] == 'not_verified' for row in rows))


class HeroHttpTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        project_profile(self.root)
        map_report(self.root)
        supported_build(self.root)
        self.app = Application(self.root, backend=control.BACKEND, runtime=False)
        self.server = make_server(0, app=self.app)
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        self.opener = build_opener(ProxyHandler({}))
        self.base = f'http://127.0.0.1:{self.server.server_port}'

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join()
        self.temporary.cleanup()

    def request(self, path, body=None):
        request = Request(self.base + path,
                          data=json.dumps(body).encode('utf-8') if body is not None else None,
                          method='PATCH' if body is not None else 'GET',
                          headers={'Content-Type': 'application/json'})
        with self.opener.open(request, timeout=3) as response:
            return json.load(response)

    def test_api_catalog_exposes_build_capability_and_real_selected_identity(self):
        before = self.request('/api/catalog')
        self.assertEqual(len(before['heroes']), 30)
        self.assertTrue(before['capabilities']['local_hero_selection']['available'])
        self.assertFalse(before['capabilities']['local_hero_selection']['runtime_verified'])
        result = self.request('/api/config', {'patch': {'local_hero_id': PHOENIX}, 'revision': 0})
        self.assertEqual(result['config']['local_hero_id'], PHOENIX)
        self.assertEqual(result['runtime']['status'], 'saved')
        after = self.request('/api/catalog')
        self.assertEqual(after['limits']['local_hero_id'], PHOENIX)
        self.assertEqual(after['limits']['local_hero'], 'Phoenix')

    def test_original_api_rejects_candidate_selection_without_changing_store(self):
        self.app.backend = 'original'
        self.assertFalse(self.request('/api/catalog')['capabilities']['local_hero_selection']['available'])
        before = self.app.settings.read()
        with self.assertRaises(HTTPError) as error:
            self.request('/api/config', {'patch': {'local_hero_id': PHOENIX}, 'revision': 0})
        self.assertEqual(error.exception.code, 400)
        self.assertEqual(self.app.settings.read(), before)


if __name__ == '__main__':
    unittest.main()
