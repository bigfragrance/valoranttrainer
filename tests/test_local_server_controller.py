"""Candidate control/lifecycle fixtures; never launch or attach a game process."""
from copy import deepcopy
from io import StringIO
import json
from pathlib import Path
import struct
import subprocess
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
import human_bot_roster as roster
from human_bot_delivery import legacy_transport_view
from training_control import initial_payload
from training_settings_v2 import DEFAULTS, Settings
from training_server import Application, Runtime, make_server

W, S, L, C = ('1' * 32, '2' * 32, '3' * 32, '4' * 32)


def pair_record(root):
    return {'schema_version': 1, 'kind': control.KIND, 'network_flavor': 'playable',
            'directory': str(root / '.local/local-server/pair-fixture'), 'proof_id': 'a' * 32,
            'module_sha256': 'b' * 64, 'port': 17777, 'transport': 'ip',
            'runner_identity': {'schema_version': 1, 'pid': 71, 'creation_filetime': 123,
                                'executable': str(Path(sys.executable).resolve()),
                                'published_from_own_process': True}}


def child_record(root, role, pid):
    pair = pair_record(root)
    return {'pid': pid, 'run_dir': str(root / '.local/native-baseline' / role),
            'network_probe': {'role': role, 'proof_id': pair['proof_id'], 'flavor': 'playable',
                              'port': pair['port'], 'transport': 'ip',
                              'output': str(Path(pair['directory']) / role)},
            'network_probe_identity': {'schema_version': 1, 'pid': pid, 'creation_filetime': 200 + pid,
                                       'executable': str(root / 'client.exe'), 'image_sha256': 'c' * 64,
                                       'module_sha256': pair['module_sha256'], 'network_flavor': 'playable',
                                       'published_from_original_popen_handle': True}}


def project(root):
    (root / 'client.exe').write_bytes(b'fixture, never executable')
    control.atomic_json(root / 'project.json', {'game_executable': 'client.exe',
                                               'expected_snapshot_sha256': 'c' * 64})


def map_report(root):
    control.atomic_json(root / 'reports/training-maps.json', {'maps': [
        {'id': 'duel1', 'kind': 'duel', 'world_path': '/Game/Maps/Duel/Duel_1/Skirmish_A',
         'world_to_image': {'type': 'affine_2x3', 'matrix': [[1, 0, 0], [0, 1, 0]]}},
        {'id': 'ascent', 'kind': 'standard_5v5', 'world_path': '/Game/Maps/Ascent/Ascent',
         'validation': {'runtime': 'passed'}}]})


class Watch:
    def __init__(self, active):
        self.pid, self.living, self.closed = active['pid'], True, False

    def alive(self):
        return self.living and not self.closed

    def close(self):
        self.closed = True


class FakeChannel:
    def __init__(self, pid):
        self.pid, self.sent, self.closed = pid, [], False
        self.game = {'ready': True, 'map_id': 'duel1', 'applied_revision': 100,
                     'infinite_ammo': True, 'players': [],
                     'weapon': {'ready': True, 'id': W, 'skin_id': S, 'level_id': L, 'chroma_id': C},
                     'effects': [{'key': 'champion_glow', 'supported': True, 'available': True,
                                  'requested': 0, 'applied': 0, 'verified': False}]}

    def state(self):
        return deepcopy(self.game)

    def send(self, kind, payload=b'', timeout=3):
        self.sent.append((kind, payload))
        message = ('{"position":[1,2,3],"ground":true,"capsule":true}' if kind == 'resolve_map_point'
                   else 'configuration_applied')
        return {'sequence': len(self.sent), 'status': 'applied', 'code': 0,
                'message': message, 'acknowledged': True}

    def close(self):
        self.closed = True


def fake_pair():
    active = {'pid': 81, 'pair_children': {'host': {'pid': 81}, 'client': {'pid': 82}}}
    return control.PairChannel(active, channel_factory=FakeChannel, watch_factory=Watch)


class OwnershipTests(unittest.TestCase):
    def test_atomic_record_retries_transient_windows_replace_lock(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'record.json'
            original, attempts = Path.replace, []
            def replace(source, target):
                attempts.append((source, target))
                if len(attempts) == 1:
                    raise PermissionError('fixture transient sharing lock')
                return original(source, target)
            with patch.object(Path, 'replace', autospec=True, side_effect=replace):
                control.atomic_json(path, {'saved': True})
            self.assertTrue(control.read_json(path)['saved'])
            self.assertEqual(len(attempts), 2)
            self.assertFalse(path.with_suffix('.json.tmp').exists())

    def test_initial_archive_is_existing_wire_layout_and_cannot_overwrite(self):
        with tempfile.TemporaryDirectory() as folder:
            directory = Path(folder)
            result = control.write_initial(directory, deepcopy(DEFAULTS),
                                           {'spawn_anchors': [{'raw_actor_location_cm': [1, 2, 3]}]})
            self.assertEqual(result['bytes'], 7844)
            payload = Path(result['path']).read_bytes()
            self.assertEqual(struct.unpack_from('<I', payload, 12)[0], len(payload))
            self.assertEqual(struct.unpack_from('<I3f', payload, 6820 - 4), (1, 1., 2., 3.))
            with self.assertRaises(FileExistsError):
                control.write_initial(directory, deepcopy(DEFAULTS), {})

    def test_pair_directory_refuses_other_local_outputs(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            self.assertEqual(control.pair_directory(root / '.local/local-server/pair-a', root),
                             root / '.local/local-server/pair-a')
            for relative in ('.local/local-server/module', '.local/native-baseline/pair-a',
                             '.local/local-server/pair-a/host', '../pair-a'):
                with self.subTest(relative=relative), self.assertRaises(ValueError):
                    control.pair_directory(root / relative, root)

    def test_child_receipt_rejects_wrong_role_proof_image_module_and_output_before_handle_access(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            project(root)
            pair, child = pair_record(root), child_record(root, 'host', 81)
            self.assertEqual(control.validate_child_record(child, 'host', pair, root=root), child)
            for field, key, value in (('network_probe', 'role', 'client'), ('network_probe', 'proof_id', 'other'),
                                      ('network_probe', 'flavor', 'proof'), ('network_probe', 'output', str(root)),
                                      ('network_probe_identity', 'pid', 82),
                                      ('network_probe_identity', 'creation_filetime', 0),
                                      ('network_probe_identity', 'module_sha256', 'd' * 64),
                                      ('network_probe_identity', 'image_sha256', 'd' * 64),
                                      ('network_probe_identity', 'executable', str(root / 'official.exe')),
                                      ('network_probe_identity', 'published_from_original_popen_handle', False)):
                changed = deepcopy(child)
                changed[field][key] = value
                with self.subTest(key=key), patch('capture_owned_thread._kernel') as kernel:
                    with self.assertRaisesRegex(RuntimeError, 'ownership record'):
                        control.validate_child_record(changed, 'host', pair, root=root)
                    kernel.assert_not_called()
            child['run_dir'] = str(root / '.local/other')
            with self.assertRaisesRegex(RuntimeError, 'directory'):
                control.validate_child_record(child, 'host', pair, root=root)

    def test_supervisor_receipt_is_checked_against_original_popen_handle(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            pair, process = pair_record(root), SimpleNamespace(pid=71, _handle=919, poll=lambda: None)
            kernel = Mock()
            kernel.GetProcessId.return_value = 71
            kernel.WaitForSingleObject.return_value = 0x102
            with patch('capture_owned_thread._kernel', return_value=kernel), \
                    patch('capture_owned_thread._creation', return_value=123) as creation, \
                    patch('capture_owned_thread._image', return_value=Path(sys.executable).resolve()):
                control.validate_pair_record(pair, pair['directory'], process,
                                             module_sha256=pair['module_sha256'], root=root)
                creation.assert_called_with(kernel, 919)
                kernel.OpenProcess.assert_not_called()
                with patch('capture_owned_thread._creation', return_value=999):
                    with self.assertRaisesRegex(RuntimeError, 'identity changed'):
                        control.validate_pair_record(pair, pair['directory'], process,
                                                     module_sha256=pair['module_sha256'], root=root)

    def test_missing_or_same_child_receipts_cannot_attach_a_pair(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            project(root)
            pair, backend = pair_record(root), control.LocalServerBackend(root)
            directory = Path(pair['directory'])
            with patch.object(backend, 'pair_record', return_value=pair):
                self.assertIsNone(backend.active(directory, None))
                control.atomic_json(directory / 'host-result.active.json', child_record(root, 'host', 81))
                self.assertIsNone(backend.active(directory, None))
                control.atomic_json(directory / 'client-result.active.json', child_record(root, 'client', 81))
                with self.assertRaisesRegex(RuntimeError, 'distinct'):
                    backend.active(directory, None)
                control.atomic_json(directory / 'client-result.active.json', child_record(root, 'client', 82))
                self.assertEqual(backend.active(directory, None)['client_pid'], 82)


class PairChannelTests(unittest.TestCase):
    def test_authority_commands_and_client_only_ui_commands_keep_native_ack(self):
        channel = fake_pair()
        try:
            for kind in ('configure', 'routes', 'weapon', 'skin', 'effect', 'buy_weapon', 'save_skin_selection'):
                result = channel.send(kind, b'fixture')
                self.assertEqual((result['target_role'], result['target_pid']), ('host', 81))
                self.assertTrue(result['acknowledged'])
            self.assertIsNone(channel.client)
            for kind in control.UI_COMMANDS:
                result = channel.send(kind)
                self.assertEqual((result['target_role'], result['target_pid']), ('client', 82))
            self.assertEqual(len(channel.host.sent), 7)
            self.assertEqual(len(channel.client.sent), 3)
        finally:
            channel.close()

    def test_missing_client_ui_channel_returns_unavailable_without_native_ack(self):
        channel = fake_pair()
        channel._channel_factory = Mock(side_effect=FileNotFoundError)
        try:
            result = channel.send('toggle_buy_menu')
            self.assertEqual(result['code'], 3)
            self.assertFalse(result['acknowledged'])
            self.assertEqual(result['sequence'], 0)
            self.assertEqual(channel.host.sent, [])
        finally:
            channel.close()

    def test_exited_child_or_closed_channel_cannot_receive_more_commands(self):
        channel = fake_pair()
        host, watches = channel.host, list(channel.watches.values())
        channel.watches['client'].living = False
        with self.assertRaisesRegex(RuntimeError, 'exited'):
            channel.send('weapon', bytes(16))
        self.assertEqual(host.sent, [])
        channel.close()
        self.assertTrue(host.closed)
        self.assertTrue(all(watch.closed for watch in watches))
        with self.assertRaisesRegex(RuntimeError, 'closed'):
            channel.state()

    def test_channel_existence_cannot_manufacture_ready_or_playable(self):
        channel = fake_pair()
        try:
            channel.host.game = {}
            state = channel.state()
            self.assertFalse(state['ready'])
            self.assertFalse(state['playable_verified'])
            self.assertTrue(state['developmental'])
        finally:
            channel.close()


class Catalog:
    def __init__(self, root):
        self.root = root

    def check_config(self, config):
        pass

    def maps(self):
        return [{'id': 'duel1', 'kind': 'duel',
                 'world_to_image': {'type': 'affine_2x3', 'matrix': [[1, 0, 0], [0, 1, 0]]}}]

    def weapons(self):
        return [{'id': W}]

    def effects(self):
        return [{'key': 'champion_glow'}]

    def heroes(self):
        return []

    def skin_payload(self, weapon, skin, level, chroma):
        if (weapon, skin, level, chroma) != (W, S, L, C):
            raise ValueError('fixture skin mismatch')
        return bytes.fromhex(weapon + skin + level + chroma)


class BackendTests(unittest.TestCase):
    def test_supervisor_job_uses_only_the_original_runner_handle(self):
        kernel = Mock()
        kernel.GetProcessId.return_value = 71
        process = SimpleNamespace(pid=71, _handle=919, poll=lambda: None)
        with patch('capture_owned_thread._kernel', return_value=kernel), \
                patch('run_native_baseline.assign_owned_job', return_value=441) as assign:
            job = runner.RunnerJob(process)
            assign.assert_called_once_with(kernel, 919)
            kernel.OpenProcess.assert_not_called()
            job.close()
            job.close()
            kernel.CloseHandle.assert_called_once_with(441)

    def test_launch_is_explicit_interactive_candidate_and_requires_built_control(self):
        with tempfile.TemporaryDirectory() as folder:
            backend = control.LocalServerBackend(Path(folder))
            manifest = {'module_sha256': 'b' * 64, 'source_sha256': {}}
            with patch('local_server_module.verified_build', return_value=manifest):
                with self.assertRaisesRegex(RuntimeError, 'V2'):
                    backend.prepare_launch(deepcopy(DEFAULTS), 'fixture')
                manifest['source_sha256']['native/training_control.h'] = 'e' * 64
                with self.assertRaisesRegex(RuntimeError, 'Initial/V2'):
                    backend.prepare_launch(deepcopy(DEFAULTS), 'fixture')
                manifest['source_sha256'].update({name: 'e' * 64 for name in control.TRAINING_SOURCES})
                directory, command = backend.prepare_launch(deepcopy(DEFAULTS), 'fixture')
                self.assertEqual(directory.name, 'pair-fixture')
                self.assertIn('--interactive', command)
                self.assertEqual(command[command.index('--seconds') + 1], '86400')
                self.assertNotIn('--network-peer', command)
                snapshot = Path(command[command.index('--controller-settings') + 1])
                self.assertTrue(snapshot.is_file())
                self.assertEqual(snapshot.parent, backend.controller_root / 'sessions')

    def test_stop_request_has_pair_and_wrapper_identity_and_waits_for_publication(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            pair, backend = pair_record(root), control.LocalServerBackend(root)
            process = SimpleNamespace(pid=71)
            with patch.object(backend, 'pair_record', return_value=None):
                self.assertFalse(backend.request_stop(pair['directory'], process))
                self.assertFalse((Path(pair['directory']) / 'stop-request.json').exists())
            with patch.object(backend, 'pair_record', return_value=pair):
                self.assertTrue(backend.request_stop(pair['directory'], process))
            self.assertTrue(runner.interactive_stop_requested(pair['directory'], pair['proof_id'], 71))
            self.assertFalse(runner.interactive_stop_requested(pair['directory'], 'd' * 32, 71))
            self.assertFalse(runner.interactive_stop_requested(pair['directory'], pair['proof_id'], 72))

    def test_interactive_runner_flags_are_bounded_and_client_gets_original_viewport(self):
        args = dict(directory=Path('fixture'), proof_id='a' * 32, port=17777,
                    transport='ip', seconds=86400, bootstrap=Path('fixture.bin'))
        host = runner.runner_arguments('host', interactive=True, **args)
        client = runner.runner_arguments('client', interactive=True, **args)
        self.assertIn('--interactive-local-server', host)
        self.assertIn('--interactive-local-server', client)
        self.assertNotIn('--original-visual-classes', host)
        self.assertIn('--original-visual-classes', client)
        with patch.object(runner, 'run_interactive', return_value=0) as run:
            self.assertEqual(runner.main(['--interactive', '--seconds', '86400']), 0)
            self.assertEqual(run.call_args.args[0].seconds, 86400)
        with patch('sys.stderr', new=StringIO()), self.assertRaises(SystemExit):
            runner.main(['--interactive', '--seconds', '86401'])

    def test_stop_during_candidate_popen_retires_exact_created_supervisor(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            runtime = Runtime(Settings(root / 'config.json'), Catalog(root), backend=control.BACKEND)
            entering, release, stopped = threading.Event(), threading.Event(), threading.Event()
            created, retired = [], []
            def popen(*args, **kwargs):
                entering.set()
                if not release.wait(2):
                    raise RuntimeError('Fixture did not release')
                process = SimpleNamespace(poll=lambda: None)
                created.append(process)
                return process
            def cleanup():
                with runtime.lock:
                    if runtime.process:
                        retired.append(runtime.process)
                    runtime.process = None
                    if runtime.log:
                        runtime.log.close()
                    runtime.log = None
                    runtime.status = 'stopped'
                return {'status': 'stopped'}
            try:
                with patch('training_server.ROOT', root), patch('subprocess.Popen', side_effect=popen), \
                        patch.object(runtime, '_prepare_launch', return_value=(root / 'pair-fixture', ['fixture'])), \
                        patch.object(runtime, '_stop_owned_process', side_effect=cleanup):
                    runtime.start_async()
                    self.assertTrue(entering.wait(2))
                    self.assertEqual(runtime.start_async()['status'], 'loading')
                    stopper = threading.Thread(target=lambda: (runtime.stop(), stopped.set()))
                    stopper.start()
                    self.assertTrue(runtime.launch_cancelled.wait(1))
                    self.assertFalse(stopped.is_set())
                    release.set()
                    stopper.join(3)
                    self.assertTrue(stopped.is_set())
                    self.assertEqual(retired, created)
                    self.assertEqual(len(retired), 1)
            finally:
                release.set()
                runtime.close()


def fixed_roster_readback(config):
    """Decode the real sidecar into an explicitly artificial current-host map."""
    raw = roster.payload(config, initial_payload(legacy_transport_view(config), {}))
    header = roster.HEADER.unpack_from(raw)
    entries = []
    for n in range(header[8]):
        slot, hero, token, *guid = roster.ROW.unpack_from(raw, 128 + n * 32)
        entries.append({'slot': slot, 'hero_index': hero, 'entry_token': f'{token:016x}',
                        'hero_id': ''.join(f'{word:08x}' for word in guid), 'native_player_id': slot})
    return {'loaded': True, 'required': True, 'sidecar_present': True, 'fixed': True,
            'validated': True, 'native_catalog_verified': True, 'error': 0, 'io_error': 0,
            'revision': header[5], 'seed': header[6], 'count': header[8],
            'human_hero_index': header[9], 'human_hero_id': ''.join(f'{word:08x}' for word in header[11:15]),
            'catalog_profile_sha256': header[15].hex(), 'initial_fnv': f'{header[16]:016x}',
            'rows_fnv': f'{header[17]:016x}', 'entries': entries}


class HttpCandidateTests(unittest.TestCase):
    def setUp(self):
        self.human_apply = patch('human_bot_integration.apply_to_channel',
                                 return_value={'status': 'consumer_effective', 'consumer_effective': True})
        self.human_apply_mock = self.human_apply.start()
        self.addCleanup(self.human_apply.stop)
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        map_report(self.root)
        self.app = Application(self.root, runtime=False, backend=control.BACKEND)
        self.app.catalog = Catalog(self.root)
        self.app.runtime = Runtime(self.app.settings, self.app.catalog,
                                   configuration_lock=self.app.mutation, backend=control.BACKEND)
        self.channel = fake_pair()
        self.app.runtime.channel, self.app.runtime.status = self.channel, 'running'
        config = self.app.settings.read()
        self.channel.host.game['configured_bot_roster'] = fixed_roster_readback(config)
        self.assertTrue(roster.live_match(config, self.channel.state()))
        self.server = make_server(0, app=self.app)
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        self.opener = build_opener(ProxyHandler({}))
        self.base = f'http://127.0.0.1:{self.server.server_port}'

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join()
        self.app.runtime.close()
        self.channel.close()
        self.temporary.cleanup()

    def request(self, path, body=None, method='POST'):
        request = Request(self.base + path, data=json.dumps(body).encode() if body is not None else None,
                          headers={'Content-Type': 'application/json'}, method=method if body is not None else 'GET')
        with self.opener.open(request, timeout=3) as response:
            return json.load(response)

    def test_http_commands_keep_existing_wire_shapes_and_target_host_ack(self):
        result = self.request('/api/config', {'revision': 0, 'patch': {'paused': True}}, 'PATCH')
        self.assertTrue(result['runtime']['acknowledged'])
        self.assertEqual(result['runtime']['target_pid'], 81)
        self.human_apply_mock.assert_called_once()
        self.assertIs(self.human_apply_mock.call_args.args[0], self.channel)
        self.assertEqual(self.human_apply_mock.call_args.args[1]['schema_version'], 3)
        result = self.request('/api/weapon', {'revision': 1, 'weapon_id': W})
        self.assertEqual(result['runtime']['target_role'], 'host')
        result = self.request('/api/skin', {'revision': 2, 'weapon_id': W, 'skin_id': S, 'level_id': L, 'chroma_id': C})
        self.assertEqual(result['runtime']['target_role'], 'host')
        self.request('/api/effect', {'revision': 3, 'key': 'champion_glow', 'mode': 1})
        self.app.runtime.sync_effects()
        self.request('/api/validate-routes', {})
        point = self.request('/api/map-point', {'map_id': 'duel1', 'uv': [.5, .5], 'z': 0})
        self.assertEqual(point['validation'], 'original_ground_and_capsule_verified')
        commands = dict(self.channel.host.sent)
        self.assertEqual(len(commands['configure']), 72)
        self.assertEqual(len(commands['routes']), 8 + 2 * 52)
        self.assertEqual(len(commands['weapon']), 16)
        self.assertEqual(len(commands['skin']), 64)
        self.assertEqual(len(commands['effect']), 88)
        self.assertIsNone(self.channel.client)

    def test_missing_current_roster_saves_pending_without_native_commands(self):
        self.channel.host.game.pop('configured_bot_roster')
        result = self.request('/api/config', {'revision': 0, 'patch': {'paused': True}}, 'PATCH')
        self.assertEqual(result['config']['revision'], 1)
        self.assertEqual(result['runtime']['status'], 'saved')
        self.assertTrue(result['runtime']['restart_required'])
        self.assertNotIn('acknowledged', result['runtime'])
        self.assertEqual(self.channel.host.sent, [])
        self.human_apply_mock.assert_not_called()

    def test_candidate_is_labeled_and_unknown_config_is_not_saved(self):
        identity = self.request('/api/identity')
        self.assertEqual(identity['backend'], control.BACKEND)
        self.assertTrue(identity['developmental'])
        state = self.request('/api/state')['runtime']
        self.assertTrue(state['developmental'])
        self.assertFalse(state['playable_verified'])
        with self.assertRaises(HTTPError) as caught:
            self.request('/api/config', {'revision': 0, 'patch': {'map_id': 'unknownmap'}}, 'PATCH')
        self.assertEqual(caught.exception.code, 400)
        self.assertEqual(self.app.settings.read()['revision'], 0)
        self.assertEqual(self.channel.host.sent, [])

    def test_selected_ffa_and_5v5_map_save_together_and_restart_owned_pair(self):
        with patch.object(self.app.runtime, 'start_async', return_value={'status': 'loading'}) as start:
            result = self.request('/api/config', {'revision': 0, 'patch': {'mode': 'deathmatch', 'map_id': 'ascent'}}, 'PATCH')
        self.assertEqual(result['config']['mode'], 'deathmatch')
        self.assertEqual(result['config']['map_id'], 'ascent')
        start.assert_called_once_with(restart=True)
        maps = self.request('/api/catalog')['maps']
        ascent = next(row for row in maps if row['id'] == 'ascent')
        self.assertEqual(ascent['validation']['runtime'], 'not_tested')
        self.assertEqual(ascent['local_server_validation']['deathmatch']['status'], 'not_tested')


if __name__ == '__main__':
    unittest.main()
