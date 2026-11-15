"""Independent actual source-guard and pre-launch ordering checks; no game."""
import ast
import copy
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))

import human_bot_integration as integration
import local_server_controller as control
import run_local_server as runner

ROOT=Path(__file__).resolve().parents[1]
def manifest(names):return {'module_sha256':'a'*64,'source_sha256':{name:'e'*64 for name in names}}
def v3(enabled=False):return {'schema_version':3,'human_bots':{'enabled':enabled}}

class SourceGuardTests(unittest.TestCase):
    def test_legacy_v2_keeps_existing_training_source_contract(self):
        control.require_training_build(manifest(control.TRAINING_SOURCES),{'schema_version':2})

    def test_v3_disabled_rejects_old_training_only_module(self):
        with self.assertRaises(RuntimeError):control.require_training_build(manifest(control.TRAINING_SOURCES),v3())

    def test_v3_disabled_requires_each_real_store_and_wire_marker(self):
        complete=control.TRAINING_SOURCES|control.HUMAN_CONFIG_SOURCES
        for missing in sorted(control.HUMAN_CONFIG_SOURCES):
            with self.subTest(missing=missing),self.assertRaises(RuntimeError):control.require_training_build(manifest(complete-{missing}),v3())
        control.require_training_build(manifest(complete),v3())

    def test_v3_enabled_rejects_store_only_module_without_brain(self):
        with self.assertRaises(RuntimeError):control.require_training_build(manifest(control.TRAINING_SOURCES|control.HUMAN_CONFIG_SOURCES),v3(True))

    def test_v3_enabled_requires_each_native_runtime_owner_marker(self):
        complete=control.TRAINING_SOURCES|control.HUMAN_CONFIG_SOURCES|control.HUMAN_RUNTIME_SOURCES
        for missing in sorted(control.HUMAN_RUNTIME_SOURCES):
            with self.subTest(missing=missing),self.assertRaises(RuntimeError):control.require_training_build(manifest(complete-{missing}),v3(True))
        control.require_training_build(manifest(complete),v3(True))

    def test_all_tiers_still_require_each_existing_training_owner(self):
        complete=control.TRAINING_SOURCES|control.HUMAN_CONFIG_SOURCES|control.HUMAN_RUNTIME_SOURCES
        for config in ({'schema_version':2},v3(),v3(True)):
            for missing in sorted(control.TRAINING_SOURCES):
                with self.subTest(config=config,missing=missing),self.assertRaises(RuntimeError):control.require_training_build(manifest(complete-{missing}),config)

    def test_guard_never_mutates_manifest_config_or_shared_source_sets(self):
        built=manifest(control.TRAINING_SOURCES|control.HUMAN_CONFIG_SOURCES|control.HUMAN_RUNTIME_SOURCES);config=v3(True)
        expected=copy.deepcopy((built,config));sets=(control.TRAINING_SOURCES,control.HUMAN_CONFIG_SOURCES,control.HUMAN_RUNTIME_SOURCES)
        control.require_training_build(built,config)
        self.assertEqual((built,config),expected)
        self.assertEqual((control.TRAINING_SOURCES,control.HUMAN_CONFIG_SOURCES,control.HUMAN_RUNTIME_SOURCES),sets)

class PrelaunchOrderingTests(unittest.TestCase):
    def configuration(self,root,enabled):
        value=integration.HumanSettings(root/'settings.json').read();value['human_bots']['enabled']=enabled;return value

    def test_webui_prepare_rejects_v3_before_writing_launch_snapshot(self):
        for enabled in (False,True):
            with self.subTest(enabled=enabled),tempfile.TemporaryDirectory() as folder:
                root=Path(folder);config=self.configuration(root,enabled);backend=control.LocalServerBackend(root)
                with patch.object(control,'check_candidate_config',return_value=config), \
                     patch('local_server_module.verified_build',return_value=manifest(control.TRAINING_SOURCES)), \
                     patch.object(control,'require_selection_build') as selection:
                    with self.assertRaises(RuntimeError):backend.prepare_launch(config,'guard-self')
                selection.assert_not_called()
                self.assertFalse((root/'.local').exists())

    def test_interactive_runner_rejects_v3_before_pair_directory_or_popen(self):
        for enabled in (False,True):
            with self.subTest(enabled=enabled),tempfile.TemporaryDirectory() as folder:
                root=Path(folder);config=self.configuration(root,enabled)
                args=SimpleNamespace(controller_settings=None,scenario=None,seconds=10,native_local_cipher=False)
                with patch.object(runner,'controller_configuration',return_value=config), \
                     patch.object(runner,'verified_build',return_value=manifest(control.TRAINING_SOURCES)), \
                     patch.object(runner,'artifact_hashes',return_value={}), \
                     patch('training_heroes.require_selection_build') as selection, \
                     patch.object(runner.subprocess,'Popen',side_effect=AssertionError('no child may be launched')) as popen:
                    with self.assertRaises(RuntimeError):runner.run_interactive(args)
                popen.assert_not_called();selection.assert_not_called()
                self.assertFalse((root/'.local').exists())

    def test_noninteractive_configured_runner_rejects_v3_before_artifact_or_popen(self):
        for enabled in (False,True):
            with self.subTest(enabled=enabled),tempfile.TemporaryDirectory() as folder:
                root=Path(folder);config=self.configuration(root,enabled)
                with patch.object(runner,'controller_configuration',return_value=config), \
                     patch.object(runner,'verified_build',return_value=manifest(control.TRAINING_SOURCES)), \
                     patch.object(runner,'artifact_hashes',side_effect=AssertionError('no target artifact may be read after failed guard')) as artifacts, \
                     patch.object(runner.subprocess,'Popen',side_effect=AssertionError('no child may be launched')) as popen:
                    with self.assertRaises(RuntimeError):runner.main(['--controller-settings',str(root/'settings.json')])
                artifacts.assert_not_called();popen.assert_not_called()
                self.assertFalse((root/'.local').exists())

    def test_every_current_launch_guard_call_receives_the_actual_configuration(self):
        expected={'tools/local_server_controller.py':1,'tools/run_local_server.py':2}
        for name,count in expected.items():
            tree=ast.parse((ROOT/name).read_text(encoding='utf-8'));calls=[n for n in ast.walk(tree) if isinstance(n,ast.Call)
                and isinstance(n.func,ast.Name) and n.func.id=='require_training_build']
            self.assertEqual(len(calls),count)
            self.assertTrue(all(len(c.args)==2 and isinstance(c.args[1],ast.Name) and c.args[1].id in ('config','selected_config') for c in calls))

class ControllerReaderTests(unittest.TestCase):
    def read_snapshot(self,root,value):
        path=root/'.local'/'local-server'/'pair-reader-self'/'controller-config.json'
        path.parent.mkdir(parents=True)
        encoded=(json.dumps(value,ensure_ascii=False,indent=2)+'\n').encode('utf-8')
        path.write_bytes(encoded);stamp=path.stat().st_mtime_ns
        with patch.object(runner,'ROOT',root),patch.object(control,'selected_map',return_value={'validated_own_fixture':True}) as selected:
            actual=runner.controller_configuration(path)
        selected.assert_called_once()
        self.assertEqual(path.read_bytes(),encoded)
        self.assertEqual(path.stat().st_mtime_ns,stamp)
        self.assertEqual(actual,integration.validate_config(value))
        return actual

    def test_explicit_v2_reader_retains_legacy_schema_and_never_rewrites(self):
        with tempfile.TemporaryDirectory() as folder:
            actual=self.read_snapshot(Path(folder),copy.deepcopy(integration.legacy.DEFAULTS))
        self.assertEqual(actual['schema_version'],2)
        self.assertNotIn('human_bots',actual)

    def test_explicit_v3_reader_preserves_full_disabled_and_enabled_snapshots_without_rewrite(self):
        for enabled in (False,True):
            with self.subTest(enabled=enabled),tempfile.TemporaryDirectory() as folder:
                root=Path(folder);value=integration.HumanSettings(root/'unsaved-preferences.json').read()
                value['human_bots']['enabled']=enabled
                actual=self.read_snapshot(root,value)
                self.assertEqual(actual['schema_version'],3)
                self.assertIs(actual['human_bots']['enabled'],enabled)
                self.assertFalse((root/'unsaved-preferences.json').exists())

    def test_explicit_path_outside_project_local_is_rejected_before_any_settings_reader(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);path=root/'outside-config.json';encoded=b'{invalid json must never be parsed outside .local}'
            path.write_bytes(encoded);stamp=path.stat().st_mtime_ns
            with patch.object(runner,'ROOT',root),patch.object(control,'check_candidate_config',side_effect=AssertionError('no validation for outside path')) as check:
                with self.assertRaisesRegex(ValueError,'project .local'):runner.controller_configuration(path)
            check.assert_not_called();self.assertEqual(path.read_bytes(),encoded);self.assertEqual(path.stat().st_mtime_ns,stamp)
            self.assertFalse((root/'.local').exists())

    def test_explicit_missing_snapshot_is_rejected_without_creating_defaults(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);path=root/'.local'/'missing-config.json'
            with patch.object(runner,'ROOT',root):
                with self.assertRaises(FileNotFoundError):runner.controller_configuration(path)
            self.assertFalse(path.exists());self.assertFalse((root/'.local').exists())

    def test_invalid_inproject_json_and_v3_values_fail_without_rewrite(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);path=root/'.local'/'controller-invalid.json';path.parent.mkdir()
            value=integration.HumanSettings(root/'unsaved-preferences.json').read();value['human_bots']['enabled']='invalid'
            for encoded,exception in ((b'{invalid',json.JSONDecodeError),(json.dumps(value).encode('utf-8'),ValueError)):
                with self.subTest(exception=exception.__name__):
                    path.write_bytes(encoded);stamp=path.stat().st_mtime_ns
                    with patch.object(runner,'ROOT',root):
                        with self.assertRaises(exception):runner.controller_configuration(path)
                    self.assertEqual(path.read_bytes(),encoded);self.assertEqual(path.stat().st_mtime_ns,stamp)
