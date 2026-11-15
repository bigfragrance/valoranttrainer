"""Preference namespace isolation; temporary files and HTTP fixtures only."""
from copy import deepcopy
from io import BytesIO, StringIO
import json
import os
from pathlib import Path
import sys
import tempfile
import threading
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch
from urllib.request import Request, build_opener, ProxyHandler

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import training_controller_store as store
import training_server as server
import local_server_controller as control
import run_local_server as pair_runner
from training_settings_v2 import DEFAULTS, Settings
from human_bot_delivery import legacy_transport_view


def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False), encoding='utf-8')


class StoreIsolationTests(unittest.TestCase):
    def test_first_clone_preserves_all_preferences_and_revision_then_each_store_changes_independently(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            original = store.open_settings(root)
            original.update({'mode': 'deathmatch', 'map_id': 'ascent', 'total_players': 15,
                             'weapon_id': '1' * 32, 'skins': {'1' * 32: {
                                 'skin_id': '2' * 32, 'level_id': '3' * 32, 'chroma_id': '4' * 32}},
                             'effects': {'champion_glow': {'mode': 1, 'value': 0}}, 'seed': 731}, 0)
            source = original.path.read_bytes()
            candidate = store.open_settings(root, store.CANDIDATE)
            self.assertEqual(candidate.path, root / '.local/controller/local-server-candidate/config.json')
            self.assertEqual(legacy_transport_view(candidate.read()), original.read())
            self.assertEqual(candidate.read()['schema_version'], 3)
            self.assertFalse(candidate.read()['human_bots']['enabled'])
            self.assertEqual(candidate.read()['revision'], 1)
            self.assertEqual(original.path.read_bytes(), source)
            candidate.update({'paused': True, 'infinite_ammo': False}, 1)
            self.assertEqual(original.path.read_bytes(), source)
            original.update({'seed': 999}, 1)
            candidate_again = store.open_settings(root, store.CANDIDATE)
            self.assertEqual(candidate_again.read()['seed'], 731)
            self.assertTrue(candidate_again.read()['paused'])
            self.assertFalse(candidate_again.read()['infinite_ammo'])
            self.assertEqual(store.open_settings(root).read()['seed'], 999)
            self.assertFalse(store.open_settings(root).read()['paused'])
            self.assertEqual(list(root.rglob('*.seed')), [])

    def test_missing_original_is_not_created_when_candidate_seeds_defaults(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            original = store.open_settings(root)
            self.assertEqual(original.read(), DEFAULTS)
            self.assertFalse(original.path.exists())
            candidate = store.open_settings(root, store.CANDIDATE)
            self.assertEqual(legacy_transport_view(candidate.read()), DEFAULTS)
            self.assertTrue(candidate.path.is_file())
            self.assertFalse(original.path.exists())

    def test_existing_explicit_fixture_wins_and_only_that_file_is_updated(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            original = store.open_settings(root)
            original.update({'total_players': 15, 'seed': 123}, 0)
            before = original.path.read_bytes()
            fixture = root / '.local/local-server/settings-fixtures/practice-minimal.json'
            value = {**deepcopy(DEFAULTS), 'total_players': 2, 'seed': 17}
            write(fixture, value)
            app = server.Application(root, runtime=False, backend=store.CANDIDATE, settings_file=fixture)
            self.assertEqual(app.settings.path, fixture)
            self.assertEqual(legacy_transport_view(app.settings.read()), value)
            self.assertEqual(json.loads(fixture.read_text(encoding='utf-8')), value)
            result = app.update({'paused': True}, 0)
            self.assertTrue(result['config']['paused'])
            self.assertTrue(store.open_settings(root, store.CANDIDATE, fixture).read()['paused'])
            self.assertEqual(original.path.read_bytes(), before)
            self.assertFalse((store.controller_directory(root, store.CANDIDATE) / 'config.json').exists())

    def test_explicit_missing_fixture_clones_once_without_copying_resources(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            original = store.open_settings(root)
            original.update({'seed': 713}, 0)
            fixture = Path('.local/local-server/settings-fixtures/new.json')
            result = store.open_settings(root, store.CANDIDATE, fixture)
            self.assertEqual(legacy_transport_view(result.read()), original.read())
            self.assertEqual({path.suffix for path in root.rglob('*') if path.is_file()}, {'.json'})
            self.assertEqual(result.path, root / fixture)

    def test_original_namespace_outside_local_non_json_and_original_alias_are_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            original = store.open_settings(root)
            original.update({'seed': 71}, 0)
            for candidate in (original.path, root / '.local/controller/sessions/old.json',
                              root / 'outside.json', root / '.local/local-server/settings.bin'):
                with self.subTest(candidate=candidate), self.assertRaises(ValueError):
                    store.open_settings(root, store.CANDIDATE, candidate)
            with self.assertRaises(ValueError):
                store.open_settings(root, store.ORIGINAL, root / '.local/other.json')
            alias = root / '.local/local-server/alias.json'
            alias.parent.mkdir(parents=True)
            os.link(original.path, alias)
            with self.assertRaises(ValueError):
                store.open_settings(root, store.CANDIDATE, alias)
            self.assertEqual(store.open_settings(root).read()['seed'], 71)

    def test_complete_seed_publication_never_replaces_an_existing_candidate_file(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'candidate.json'
            write(path, {'existing': True})
            store._seed_once(path, {'replacement': True})
            self.assertEqual(json.loads(path.read_text(encoding='utf-8')), {'existing': True})
            self.assertEqual(list(path.parent.glob('*.seed')), [])

    def test_direct_interactive_runner_defaults_to_the_candidate_store(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            original = store.open_settings(root)
            original.update({'total_players': 15}, 0)
            candidate = store.open_settings(root, store.CANDIDATE)
            candidate.update({'total_players': 2, 'paused': True}, 1)
            before = original.path.read_bytes()
            with patch.object(pair_runner, 'ROOT', root):
                value = pair_runner.controller_configuration()
            self.assertEqual(value['total_players'], 2)
            self.assertTrue(value['paused'])
            self.assertEqual(original.path.read_bytes(), before)

    def test_candidate_f6_readback_persists_only_its_own_store(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            original = store.open_settings(root)
            original.update({'seed': 71}, 0)
            before = original.path.read_bytes()
            app = server.Application(root, backend=store.CANDIDATE)
            try:
                app.runtime.channel = SimpleNamespace(state=lambda: {
                    'ready': True, 'map_id': 'duel1', 'infinite_ammo': False, 'applied_revision': 99})
                app.runtime.status = 'running'
                app.runtime.sync_native()
                self.assertFalse(store.open_settings(root, store.CANDIDATE, app.settings.path).read()['infinite_ammo'])
                self.assertEqual(original.path.read_bytes(), before)
                self.assertTrue(Settings(original.path).read()['infinite_ammo'])
            finally:
                app.runtime.channel = None
                app.runtime.close()

    def test_explicit_minimal_store_enters_the_candidate_runtime_launch_snapshot(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            fixture = root / '.local/local-server/settings-fixtures/practice-minimal.json'
            value = {**deepcopy(DEFAULTS), 'total_players': 2}
            write(fixture, value)
            app = server.Application(root, backend=store.CANDIDATE, settings_file=fixture)
            manifest = {'module_sha256': 'a' * 64, 'source_sha256': {name: 'e' * 64 for name in (control.TRAINING_SOURCES | control.HUMAN_CONFIG_SOURCES)}}
            try:
                with patch('local_server_module.verified_build', return_value=manifest):
                    _, command = app.runtime._prepare_launch(app.settings.read(), 'minimal-fixture')
                launch = Path(command[command.index('--controller-settings') + 1])
                self.assertEqual(launch.parent, store.controller_directory(root, store.CANDIDATE) / 'sessions')
                snapshot = json.loads(launch.read_text(encoding='utf-8'))
                self.assertEqual(legacy_transport_view(snapshot), value)
                self.assertEqual(snapshot['schema_version'], 3)
                self.assertFalse(snapshot['human_bots']['enabled'])
                self.assertEqual(app.runtime.backend_adapter.controller_root, store.controller_directory(root, store.CANDIDATE))
                self.assertFalse((root / '.local/controller/sessions').exists())
            finally:
                app.runtime.close()

    def test_console_reuse_requires_candidate_settings_identity_but_accepts_old_original_default(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            original = store.settings_path(root)
            candidate = store.settings_path(root, store.CANDIDATE)
            self.assertTrue(store.identity_matches_settings({}, original, root=root))
            self.assertFalse(store.identity_matches_settings({}, candidate, root=root, backend=store.CANDIDATE))
            self.assertFalse(store.identity_matches_settings({'settings_file': str(original)}, candidate,
                                                             root=root, backend=store.CANDIDATE))
            self.assertTrue(store.identity_matches_settings({'settings_file': str(candidate)}, candidate,
                                                            root=root, backend=store.CANDIDATE))


class HttpNamespaceTests(unittest.TestCase):
    def test_candidate_and_original_api_mutations_are_independent(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            original = server.Application(root, runtime=False)
            original.update({'seed': 91}, 0)
            candidate = server.Application(root, runtime=False, backend=store.CANDIDATE)
            servers, threads = [], []
            opener = build_opener(ProxyHandler({}))
            def request(instance, path, body=None, method='PATCH'):
                url = f'http://127.0.0.1:{instance.server_port}' + path
                request = Request(url, data=json.dumps(body).encode() if body is not None else None,
                                  headers={'Content-Type': 'application/json'}, method=method if body is not None else 'GET')
                with opener.open(request, timeout=3) as response:
                    return json.load(response)
            try:
                for app in (original, candidate):
                    instance = server.make_server(0, app=app)
                    thread = threading.Thread(target=instance.serve_forever, daemon=True)
                    thread.start()
                    servers.append(instance)
                    threads.append(thread)
                old, new = servers
                self.assertNotEqual(request(old, '/api/identity')['settings_file'], request(new, '/api/identity')['settings_file'])
                request(new, '/api/config', {'revision': 1, 'patch': {'paused': True}})
                self.assertFalse(request(old, '/api/state')['config']['paused'])
                self.assertEqual(request(old, '/api/state')['config']['revision'], 1)
                request(old, '/api/config', {'revision': 1, 'patch': {'seed': 92}})
                self.assertEqual(request(new, '/api/state')['config']['seed'], 91)
                self.assertTrue(request(new, '/api/state')['config']['paused'])
            finally:
                for instance in servers:
                    instance.shutdown()
                    instance.server_close()
                for thread in threads:
                    thread.join()


class CliNamespaceTests(unittest.TestCase):
    def test_explicit_fixture_is_forwarded_and_candidate_server_record_is_independent(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            fixture = root / '.local/local-server/settings-fixtures/practice-minimal.json'
            write(fixture, {**deepcopy(DEFAULTS), 'total_players': 2})
            app = server.Application(root, runtime=False, backend=store.CANDIDATE, settings_file=fixture)
            app.runtime = SimpleNamespace(close=Mock())
            instance = SimpleNamespace(app=app, server_port=8792, serve_forever=Mock(), server_close=Mock())
            opener = Mock()
            opener.open.side_effect = OSError('no existing fixture server')
            command = ['training_server', '--backend', store.CANDIDATE, '--port', '8792', '--config', str(fixture)]
            with patch.object(server, 'ROOT', root), patch.object(sys, 'argv', command), \
                    patch('urllib.request.build_opener', return_value=opener), \
                    patch.object(server, 'make_server', return_value=instance) as make, \
                    patch.object(server.signal, 'signal'), patch.dict(os.environ), patch('sys.stdout', new=StringIO()), \
                    patch('subprocess.Popen') as process:
                self.assertEqual(server.main(), 0)
            make.assert_called_once_with(8792, backend=store.CANDIDATE, settings_file=fixture)
            process.assert_not_called()
            record = store.controller_directory(root, store.CANDIDATE) / 'server-8792.json'
            self.assertEqual(json.loads(record.read_text(encoding='utf-8'))['settings_file'], str(fixture))
            self.assertFalse((root / '.local/controller/server.json').exists())

    def test_existing_old_candidate_is_not_started_with_the_original_or_wrong_settings_store(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            fixture = root / '.local/local-server/settings-fixtures/practice-minimal.json'
            identity = {'application': 'valoranttrainer', 'protocol': 2, 'root': str(root), 'backend': store.CANDIDATE}
            command = ['training_server', '--backend', store.CANDIDATE, '--port', '8792',
                       '--settings-file', str(fixture), '--start']
            for extra in ({}, {'settings_file': str(root / '.local/controller/config.json')}):
                response = BytesIO(json.dumps({**identity, **extra}).encode())
                opener = Mock()
                opener.open.return_value = response
                with patch.object(server, 'ROOT', root), patch.object(sys, 'argv', command), \
                        patch('urllib.request.build_opener', return_value=opener), patch.dict(os.environ), \
                        patch.object(server, 'make_server') as make, patch('subprocess.Popen') as process:
                    with self.assertRaisesRegex(RuntimeError, '设置文件'):
                        server.main()
                self.assertEqual(opener.open.call_count, 1)  # Identity GET only; no POST /api/start.
                make.assert_not_called()
                process.assert_not_called()


if __name__ == '__main__':
    unittest.main()
