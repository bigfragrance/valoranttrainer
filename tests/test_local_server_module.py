"""Source/ownership boundaries only; no native builds or game processes."""
from contextlib import redirect_stdout
import hashlib
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
from types import SimpleNamespace
import threading
import unittest
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import local_network_probe as network
import local_server_module as server
import run_native_baseline as baseline


class ServerModuleFixture(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        self.destination = self.root / '.local/local-server/module'
        self.destination.mkdir(parents=True)
        for module, key, value in ((server, 'ROOT', self.root), (server, 'BUILD', self.destination),
                                    (network, 'ROOT', self.root)):
            handle = patch.object(module, key, value)
            handle.start()
            self.addCleanup(handle.stop)
        self.profiles = [{'rva': rva, 'bytes': 1, 'fnv64': index + 1,
                          'sha256': hashlib.sha256(bytes([index])).hexdigest()}
                         for index, rva in enumerate(server.PROFILE_RVAS)]
        self.files = {
            server.SOURCE_NAME: b'#include "engine_api.h"\n#include "local_server_profiles.h"\n',
            server.SCRIPT_NAME: b'fixture build script, never executed\n',
            server.PROFILE_NAME: server.profile_header(self.profiles),
            'native/engine_api.h': b'#include "nested/leaf.h"\n',
            'native/nested/leaf.h': b'#pragma once\n#include "../engine_api.h"\n',
        }
        for name, data in self.files.items():
            target = self.root / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        for name in ('.local/native-module/trainer_bridge.dll', '.local/local-network/module/network_probe.dll'):
            target = self.root / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(b'preserved fixture')
        self.module = self.destination / server.MODULE_NAME
        self.module.write_bytes(b'fixture DLL; not executable')
        self.record = {
            'schema_version': 1, 'network_flavor': server.FLAVOR, 'source_image_sha256': server.PIN,
            'source_snapshot': server.SNAPSHOT_NAME,
            'source_sha256': {name: hashlib.sha256(data).hexdigest() for name, data in self.files.items()},
            'module_sha256': hashlib.sha256(self.module.read_bytes()).hexdigest(),
            'native_profiles': self.profiles,
        }
        for name, data in self.files.items():
            target = self.root / server.SNAPSHOT_NAME / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        self.save()

    def save(self):
        (self.destination / 'build.json').write_text(json.dumps(self.record), encoding='utf-8')


class ServerBuildSnapshotTests(ServerModuleFixture):
    def compile_candidate(self, command, **options):
        """Mock only the compiler: all staging, guards and publication run for real."""
        output = Path(command[-1])
        snapshot = Path(options['cwd'])
        self.assertNotEqual(output, self.destination)
        self.assertEqual(snapshot, output / 'source-snapshot')
        self.assertEqual(Path(command[-2]), snapshot / server.SCRIPT_NAME)
        self.assertEqual(server.capture_sources(snapshot), self.files)
        self.assertEqual(self.module.read_bytes(), b'fixture DLL; not executable')
        (output / server.MODULE_NAME).write_bytes(b'next fixture DLL; not executable')
        (output / 'local_server_bridge.map').write_bytes(b'next fixture map')
        return SimpleNamespace(returncode=0, stdout='fixture compiler output', stderr='')

    def build_with(self, compiler):
        with patch.object(network, 'code_profiles', return_value=self.profiles), \
                patch.object(server, 'project_environment', return_value={}), \
                patch.object(server.subprocess, 'run', side_effect=compiler):
            return server.build()

    def assert_previous_verified(self, manifest):
        self.assertEqual((self.destination / 'build.json').read_bytes(), manifest)
        self.assertEqual(self.module.read_bytes(), b'fixture DLL; not executable')
        self.assertEqual(server.verified_build()['module_sha256'], self.record['module_sha256'])
        self.assertFalse(list(self.destination.parent.glob('.module-stage-*')))

    def test_captures_transitive_includes_and_cycle_once(self):
        self.assertEqual(server.capture_sources(self.root), dict(sorted(self.files.items())))

    def test_verified_snapshot_survives_later_source_edits(self):
        (self.root / 'native/nested/leaf.h').write_text('next source revision', encoding='utf-8')
        (self.root / 'native/unrelated.h').write_text('unused header', encoding='utf-8')
        self.assertEqual(server.verified_build()['module_sha256'], self.record['module_sha256'])

    def test_changed_captured_transitive_header_is_rejected(self):
        (self.root / server.SNAPSHOT_NAME / 'native/nested/leaf.h').write_bytes(b'changed')
        with self.assertRaisesRegex(RuntimeError, 'closure or digest'):
            server.verified_build()

    def test_omitted_transitive_source_is_rejected(self):
        self.record['source_sha256'].pop('native/nested/leaf.h')
        self.save()
        with self.assertRaisesRegex(RuntimeError, 'closure or digest'):
            server.verified_build()

    def test_uncompiled_source_cannot_be_added_to_manifest(self):
        target = self.root / server.SNAPSHOT_NAME / 'native/unrelated.h'
        target.write_bytes(b'unused')
        self.record['source_sha256']['native/unrelated.h'] = hashlib.sha256(b'unused').hexdigest()
        self.save()
        with self.assertRaisesRegex(RuntimeError, 'closure or digest'):
            server.verified_build()

    def test_other_flavor_manifest_and_redirected_snapshot_are_rejected(self):
        for key, value in (('network_flavor', 'proof'), ('source_image_sha256', '0' * 64),
                           ('source_snapshot', '.local/local-network/module/source-snapshot')):
            with self.subTest(key=key):
                original = self.record[key]
                self.record[key] = value
                self.save()
                with self.assertRaisesRegex(RuntimeError, 'identity or source snapshot'):
                    server.verified_build()
                self.record[key] = original

    def test_escaping_manifest_and_include_are_rejected(self):
        self.record['source_sha256']['native/../../outside.h'] = '0' * 64
        self.save()
        with self.assertRaisesRegex(RuntimeError, 'Unexpected'):
            server.verified_build()
        (self.root / 'outside.h').write_bytes(b'outside native')
        (self.root / server.SOURCE_NAME).write_text('#include "../outside.h"\n', encoding='utf-8')
        with self.assertRaisesRegex(RuntimeError, 'outside the captured'):
            server.capture_sources(self.root)

    def test_changed_library_or_profile_record_is_rejected(self):
        self.module.write_bytes(b'changed candidate')
        with self.assertRaisesRegex(RuntimeError, 'library changed'):
            server.verified_build()
        self.module.write_bytes(b'fixture DLL; not executable')
        self.record['native_profiles'][0]['fnv64'] += 1
        self.save()
        with self.assertRaisesRegex(RuntimeError, 'profile header differs'):
            server.verified_build()

    def test_build_captures_only_its_own_closure_and_preserves_old_libraries(self):
        with patch.object(network, 'code_profiles', return_value=self.profiles), \
                patch.object(server, 'project_environment', return_value={}), \
                patch.object(server.subprocess, 'run', side_effect=self.compile_candidate) as compile_mock:
            record = server.build()
        self.assertEqual(set(record['source_sha256']), set(self.files))
        self.assertNotEqual(compile_mock.call_args.args[0][-1], str(self.destination))
        self.assertNotEqual(compile_mock.call_args.kwargs['cwd'], self.root)
        self.assertTrue(record['compiled_from_captured_sources'])
        self.assertEqual(self.module.read_bytes(), b'next fixture DLL; not executable')
        self.assertEqual(server.capture_sources(self.root / server.SNAPSHOT_NAME), self.files)
        for name in ('.local/native-module/trainer_bridge.dll', '.local/local-network/module/network_probe.dll'):
            self.assertEqual((self.root / name).read_bytes(), b'preserved fixture')

    def test_source_change_during_mocked_compile_cannot_publish_a_new_manifest(self):
        before = (self.destination / 'build.json').read_bytes()
        def changed(*args, **kwargs):
            result = self.compile_candidate(*args, **kwargs)
            (self.root / 'native/nested/leaf.h').write_bytes(b'changed during compilation')
            return result
        with patch.object(network, 'code_profiles', return_value=self.profiles), \
                patch.object(server, 'project_environment', return_value={}), \
                patch.object(server.subprocess, 'run', side_effect=changed):
            with self.assertRaisesRegex(RuntimeError, 'source changed during compilation'):
                server.build()
        self.assert_previous_verified(before)

    def test_compiler_failure_after_writing_its_dll_cannot_damage_previous_build(self):
        before = (self.destination / 'build.json').read_bytes()
        def failed(*args, **kwargs):
            self.compile_candidate(*args, **kwargs)
            return SimpleNamespace(returncode=1, stdout='', stderr='fixture compiler failed')
        with self.assertRaisesRegex(RuntimeError, 'fixture compiler failed'):
            self.build_with(failed)
        self.assert_previous_verified(before)

    def test_compiler_timeout_preserves_previous_verified_build(self):
        before = (self.destination / 'build.json').read_bytes()
        def timed_out(*args, **kwargs):
            self.compile_candidate(*args, **kwargs)
            raise server.subprocess.TimeoutExpired('fixture compiler', 120)
        with self.assertRaises(server.subprocess.TimeoutExpired):
            self.build_with(timed_out)
        self.assert_previous_verified(before)

    def test_changed_staged_closure_fails_before_publication(self):
        before = (self.destination / 'build.json').read_bytes()
        def corrupt(*args, **kwargs):
            result = self.compile_candidate(*args, **kwargs)
            (Path(kwargs['cwd']) / 'native/nested/leaf.h').write_bytes(b'not the captured input')
            return result
        with self.assertRaisesRegex(RuntimeError, 'closure or digest'):
            self.build_with(corrupt)
        self.assert_previous_verified(before)

    def test_manifest_publication_failure_rolls_back_dll_snapshot_and_map(self):
        before = (self.destination / 'build.json').read_bytes()
        old_map = self.destination / 'local_server_bridge.map'
        old_map.write_bytes(b'old fixture map')
        replace = Path.replace
        def reject_manifest(path, target):
            if Path(target) == self.destination / 'build.json' and path.parent.name == 'candidate':
                raise OSError('fixture manifest publication failed')
            return replace(path, target)
        with patch.object(Path, 'replace', reject_manifest):
            with self.assertRaisesRegex(OSError, 'manifest publication failed'):
                self.build_with(self.compile_candidate)
        self.assert_previous_verified(before)
        self.assertEqual(old_map.read_bytes(), b'old fixture map')

    def test_final_verification_failure_restores_previous_complete_bundle(self):
        before = (self.destination / 'build.json').read_bytes()
        verify = server._verify_directory
        def reject_published(directory):
            if Path(directory) == self.destination:
                raise RuntimeError('fixture final verification failure')
            return verify(directory)
        with patch.object(server, '_verify_directory', side_effect=reject_published):
            with self.assertRaisesRegex(RuntimeError, 'final verification failure'):
                self.build_with(self.compile_candidate)
        self.assert_previous_verified(before)

    def test_source_change_after_staged_validation_still_cannot_publish(self):
        before = (self.destination / 'build.json').read_bytes()
        verify = server._verify_directory
        def changed_after_verify(directory):
            result = verify(directory)
            if Path(directory) != self.destination:
                (self.root / 'native/nested/leaf.h').write_bytes(b'changed after compiler returned')
            return result
        with patch.object(server, '_verify_directory', side_effect=changed_after_verify):
            with self.assertRaisesRegex(RuntimeError, 'source changed during compilation'):
                self.build_with(self.compile_candidate)
        self.assert_previous_verified(before)

    def test_profile_generation_only_changes_the_real_captured_compile_inputs(self):
        working_profile = (self.root / server.PROFILE_NAME).read_bytes()
        self.profiles[0]['fnv64'] += 1
        def compile_profiles(command, **options):
            output = Path(command[-1])
            self.assertEqual((Path(options['cwd']) / server.PROFILE_NAME).read_bytes(), server.profile_header(self.profiles))
            self.assertEqual((self.root / server.PROFILE_NAME).read_bytes(), working_profile)
            (output / server.MODULE_NAME).write_bytes(b'next fixture DLL; not executable')
            return SimpleNamespace(returncode=0, stdout='', stderr='')
        result = self.build_with(compile_profiles)
        self.assertEqual(result['native_profiles'], self.profiles)
        self.assertEqual((self.root / server.PROFILE_NAME).read_bytes(), working_profile)
        self.assertEqual(server.verified_build(), result)

    def test_verified_reader_waits_for_the_complete_publication(self):
        replace = Path.replace
        reader_started, reader_finished = threading.Event(), threading.Event()
        observed, failures, readers = [], [], []
        def read_bundle():
            reader_started.set()
            try:
                observed.append(server.verified_build())
            except Exception as error:
                failures.append(error)
            finally:
                reader_finished.set()
        def start_reader_mid_publication(path, target):
            result = replace(path, target)
            if path.parent.name == 'candidate' and Path(target) == self.module:
                reader = threading.Thread(target=read_bundle)
                readers.append(reader)
                reader.start()
                self.assertTrue(reader_started.wait(1))
                self.assertFalse(reader_finished.wait(.03))
            return result
        with patch.object(Path, 'replace', start_reader_mid_publication):
            record = self.build_with(self.compile_candidate)
        for reader in readers:
            reader.join(2)
        self.assertTrue(reader_finished.is_set())
        self.assertEqual(failures, [])
        self.assertEqual(observed, [record])

    def test_failed_rollback_keeps_old_dll_and_named_recovery_action(self):
        replace = Path.replace
        def reject_publish_and_restore(path, target):
            if path.parent.name == 'candidate' and Path(target) == self.destination / 'build.json':
                raise OSError('fixture commit failure')
            if path.parent.name == 'previous' and path.name == server.MODULE_NAME:
                raise OSError('fixture restoration temporarily blocked')
            return replace(path, target)
        with patch.object(Path, 'replace', reject_publish_and_restore):
            with self.assertRaisesRegex(server._RollbackError, 'restore .*previous.*local_server_bridge.dll'):
                self.build_with(self.compile_candidate)
        stages = list(self.destination.parent.glob('.module-stage-*'))
        self.assertEqual(len(stages), 1)
        self.assertEqual((stages[0] / 'previous' / server.MODULE_NAME).read_bytes(), b'fixture DLL; not executable')
        self.assertTrue((stages[0] / 'candidate' / server.MODULE_NAME).is_file())

    def test_cleanup_never_recursively_deletes_an_unknown_or_large_tree(self):
        stage = self.destination.parent / '.module-stage-fixture'
        stage.mkdir()
        (stage / 'unexpected-resource.pak').write_bytes(b'preserve unknown content')
        with self.assertRaisesRegex(RuntimeError, 'unknown output'):
            server._cleanup_stage(stage, self.destination)
        self.assertTrue((stage / 'unexpected-resource.pak').is_file())
        with self.assertRaisesRegex(RuntimeError, 'owned workspace'):
            server._cleanup_stage(self.destination, self.destination)


class ServerOwnershipTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        self.destination = self.root / '.local/local-server/module'
        self.destination.mkdir(parents=True)
        self.image = self.root / 'fixture.exe'
        (self.root / 'project.json').write_text('{}', encoding='utf-8')
        self.path = self.root / '.local/host.active.json'
        self.active = {'pid': 41, 'network_probe': {'role': 'host', 'proof_id': 'session', 'flavor': 'playable'},
                       'network_probe_identity': {'pid': 41, 'creation_filetime': 123, 'image_sha256': network.PIN,
                           'executable': str(self.image), 'module_sha256': 'new-module',
                           'network_flavor': 'playable', 'published_from_original_popen_handle': True}}
        self.kernel = Mock()
        self.kernel.OpenProcess.return_value = 11
        self.kernel.GetProcessId.return_value = 41
        self.kernel.WaitForSingleObject.return_value = 0x102
        for module, key, value in ((server, 'ROOT', self.root), (server, 'BUILD', self.destination),
                                    (network, 'ROOT', self.root),
                                    (server, 'verified_build', Mock(return_value={'module_sha256': 'new-module'})),
                                    (server, '_verify_directory', Mock(return_value={'module_sha256': 'new-module'})),
                                    (network, 'client_executable', Mock(return_value=self.image)),
                                    (network, 'verified_build', Mock(side_effect=AssertionError('Wrong proof build selected')))):
            handle = patch.object(module, key, value)
            handle.start()
            self.addCleanup(handle.stop)
        for key, value in (('_kernel', self.kernel), ('_creation', 123), ('_image', self.image)):
            handle = patch('capture_owned_thread.' + key, return_value=value)
            handle.start()
            self.addCleanup(handle.stop)

    def save(self):
        self.path.write_text(json.dumps(self.active), encoding='utf-8')

    def test_playable_peer_uses_its_verified_module_and_original_process_identity(self):
        self.save()
        self.assertEqual(server.validate_peer(self.path, 'session'), self.active)
        self.kernel.CloseHandle.assert_called_once_with(11)

    def test_new_owned_publication_does_not_consult_proof_build(self):
        child = SimpleNamespace(pid=41, _handle=10)
        identity = server.owned_identity(child, self.image)
        self.assertEqual(identity['module_sha256'], 'new-module')
        self.assertEqual(identity['network_flavor'], 'playable')
        self.assertTrue(identity['published_from_original_popen_handle'])

    def test_playable_refuses_legacy_proof_even_with_the_same_hash(self):
        self.active['network_probe'].pop('flavor')
        self.active['network_probe_identity'].pop('network_flavor')
        self.save()
        with self.assertRaisesRegex(RuntimeError, 'matching runner-owned'):
            server.validate_peer(self.path, 'session')
        self.kernel.OpenProcess.assert_not_called()

    def test_proof_refuses_playable_even_with_the_same_hash(self):
        self.save()
        with self.assertRaisesRegex(RuntimeError, 'matching runner-owned'):
            network.validate_peer(self.path, 'session', module_sha256='new-module')
        self.kernel.OpenProcess.assert_not_called()

    def test_default_proof_accepts_legacy_matching_record(self):
        self.active['network_probe'].pop('flavor')
        self.active['network_probe_identity'].pop('network_flavor')
        self.save()
        with patch.object(network, 'verified_build', return_value={'module_sha256': 'new-module'}):
            self.assertEqual(network.validate_peer(self.path, 'session'), self.active)

    def test_mismatched_explicit_hash_and_build_record_fail(self):
        with self.assertRaisesRegex(RuntimeError, 'differs from the verified'):
            network._module_identity('other-module', {'module_sha256': 'new-module'})

    def test_paired_preflight_preserves_account_gates_with_new_build(self):
        self.save()
        initial = {'ready': False, 'expected_image_matches': True,
                   'existing_game_or_launcher_processes': True, 'blocking_processes': [{'pid': 41}],
                   'saved_login_present': True, 'saved_authorization_present': False,
                   'matching_saved_credential_present': False}
        with patch.object(network, 'preflight', return_value=initial):
            result = server.pair_preflight({}, self.path, 'session')
        self.assertFalse(result['ready'])
        self.assertEqual(result['blocking_processes'], [])
        self.assertEqual(result['paired_owned_host']['creation_filetime'], 123)

    def test_postflight_exited_peer_policy_is_forwarded_explicitly(self):
        with patch.object(network, 'pair_preflight', return_value={'ready': True}) as check:
            self.assertEqual(server.pair_preflight({}, self.path, 'session', allow_exited=True),
                             {'ready': True})
        self.assertTrue(check.call_args.kwargs['allow_exited'])
        self.assertEqual(check.call_args.kwargs['build_record']['module_sha256'],
                         'new-module')

    def test_new_loader_places_exact_network_abi_next_to_new_dll(self):
        output = self.root / '.local/local-server/test-host'
        process = SimpleNamespace(pid=41)
        context = {'image': 0x10000, 'world': 0x20000, 'controller': 0x30000, 'pawn': 0x40000}
        with patch.object(network, 'load_verified_owned_library', return_value={'loaded': True}) as loader:
            self.assertEqual(server.load(process, context, role='host', port=17777, output=output), {'loaded': True})
        payload = (self.destination / 'network-41.bin').read_bytes()
        self.assertEqual(len(payload), 2072)
        self.assertEqual(struct.unpack_from('<Q4I', payload), (network.CONTEXT_MAGIC, 1, 1, 17777, 1))
        self.assertEqual(payload[24:].decode('utf-16-le').rstrip('\0'), str(output) + '\\')
        self.assertEqual(loader.call_args.args,
                         (process, context, self.destination / server.MODULE_NAME, 'new-module', output / 'bridge-state.json', 1))
        self.assertFalse((self.root / '.local/local-network/module/network-41.bin').exists())


class BaselineFlavorTests(unittest.TestCase):
    def test_selector_has_two_independent_modules(self):
        self.assertIs(baseline.network_module_for_flavor('proof'), network)
        self.assertIs(baseline.network_module_for_flavor('playable'), server)
        with self.assertRaises(ValueError):
            baseline.network_module_for_flavor('other')

    def test_preflight_only_uses_selected_flavor_without_launching(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary).resolve()
            (root / 'project.json').write_text('{}', encoding='utf-8')
            selected = Mock()
            selected.local_path.side_effect = lambda path: Path(path).resolve()
            selected.pair_preflight.return_value = {'ready': True}
            argv = ['run_native_baseline.py', '--network-probe', 'host', '--network-flavor', 'playable',
                    '--network-proof-id', 'a' * 32, '--network-output', str(root / '.local/host'),
                    '--result-file', str(root / '.local/result.json'), '--original-game-instance',
                    '--mode', 'navigation', '--bootstrap', str(root / 'fixture.bin')]
            with patch.object(baseline, 'ROOT', root), patch.object(sys, 'argv', argv), \
                    patch.object(baseline, 'network_module_for_flavor', return_value=selected) as selector, \
                    patch.object(baseline.ctypes, 'WinDLL', return_value=Mock()), \
                    patch.object(baseline.subprocess, 'Popen', side_effect=AssertionError('No process permitted')), \
                    redirect_stdout(io.StringIO()):
                self.assertEqual(baseline.main(), 0)
            selector.assert_called_once_with('playable')
            selected.verified_build.assert_called_once_with()
            selected.pair_preflight.assert_called_once_with({}, None, 'a' * 32, allow_exited=False)
            selected.owned_identity.assert_not_called()
            selected.load.assert_not_called()


if __name__ == '__main__':
    unittest.main()
