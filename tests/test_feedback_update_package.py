import hashlib
from contextlib import closing
import json
from pathlib import Path
import sys
import sqlite3
import tempfile
import unittest
from unittest.mock import patch
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import apply_trainer_update as update
import export_local_server_patch as exporter
import local_server_module
import build_trainer_installer as builder


class FeedbackUpdatePackageTests(unittest.TestCase):
    def write(self, root, name, value):
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(value) if isinstance(value, dict) else value, encoding='utf-8')

    def fixture(self, root):
        self.write(root, '.local/local-server/module/local_server_bridge.dll', 'new-module')
        module_hash = exporter.digest(root / '.local/local-server/module/local_server_bridge.dll')
        self.write(root, '.local/local-server/module/source-snapshot/native/example.h', 'source')
        self.write(root, '.local/local-server/module/build.json', {
            'module_sha256': module_hash,
            'source_sha256': {'native/example.h': hashlib.sha256(b'source').hexdigest()}})
        self.write(root, 'patch-version.json', {'kind': update.KIND, 'manifest_id': '448DAE34A9D16C88',
                                               'release': '20261114-feedback-r1', 'module_sha256': module_hash})
        self.write(root, 'profiles/downloaded-files.json', {'manifest_id': '448DAE34A9D16C88'})
        self.write(root, 'tools/trainer_downloaded.py', '# current')
        self.inventory(root)

    def inventory(self, root):
        self.write(root, 'package-sha256.json', {
            p.relative_to(root).as_posix(): exporter.digest(p)
            for p in root.rglob('*') if p.is_file() and p.name != 'package-sha256.json'})

    def test_explicit_candidate_selected_and_current_closure_required(self):
        root = Path('candidate')
        record = {'module_sha256': 'candidate', 'source_sha256': {'native/header.h': hashlib.sha256(b'new').hexdigest()}}
        with patch.object(local_server_module, '_verify_directory', return_value=record) as verify:
            with patch.object(local_server_module, 'capture_sources', return_value={'native/header.h': b'new'}):
                directory, result = exporter.select_module(root)
                self.assertEqual(directory, root.resolve())
                self.assertEqual(result['module_sha256'], 'candidate')
                verify.assert_called_once_with(root.resolve())
            with patch.object(local_server_module, 'capture_sources', return_value={'native/header.h': b'old'}):
                with self.assertRaisesRegex(RuntimeError, 'differs'):
                    exporter.select_module(root)

    def test_downloaded_metadata_uses_verified_plan(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            plan = root / 'plan.sqlite'
            with closing(sqlite3.connect(plan)) as database:
                database.execute('CREATE TABLE files (id INTEGER, path TEXT, size INTEGER, sha256 TEXT)')
                database.execute("INSERT INTO files VALUES (1, 'game.exe', 123, 'hash')")
                database.commit()
            info = {'plan_sha256': exporter.digest(plan), 'file_count': 1, 'game_bytes': 123,
                    'manifest_id': '448DAE34A9D16C88', 'manifest_sha256': 'manifest', 'languages': {'text': 'zh_TW'}}
            metadata = exporter.downloaded_metadata(root / 'package', info, plan)
            self.assertEqual(metadata['kind'], update.KIND)
            self.assertEqual(metadata['manifest_id'], '448DAE34A9D16C88')
            self.assertEqual(update.read(root / 'package/profiles/downloaded-files.json')['files'][0]['size'], 123)
            info['plan_sha256'] = 'wrong'
            with self.assertRaisesRegex(RuntimeError, 'differs'):
                exporter.downloaded_metadata(root / 'bad', info, plan)

    def test_installer_explicit_donor_requires_hash(self):
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(RuntimeError, 'changed'):
                builder.stage_patch({}, donor=Path(temp) / 'candidate')

    def test_archive_root_and_hashes(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / 'package'
            self.fixture(root)
            archive = Path(temp) / 'update.zip'
            exporter.archive(root, archive, root_paths=True)
            with zipfile.ZipFile(archive) as bundle:
                self.assertIn('tools/trainer_downloaded.py', bundle.namelist())
                self.assertNotIn('package/tools/trainer_downloaded.py', bundle.namelist())
                self.assertIsNone(bundle.testzip())

    def test_apply_retry_preserves_settings_receipts_resources_and_custom_files(self):
        with tempfile.TemporaryDirectory() as temp:
            package = Path(temp) / '更新 包'
            target = Path(temp) / '完整资源 训练器'
            self.fixture(package)
            self.write(target, 'patch-version.json', {'kind': update.KIND, 'manifest_id': '448DAE34A9D16C88'})
            preserved = {'.local/controller/local-server-candidate/humanlike-bots.json': 'my settings',
                         '.local/download-installation.json': 'download receipt',
                         '.local/feedback/a.txt': 'diagnostics', 'client/game.ucas': 'resource',
                         'my custom.txt': 'custom'}
            for name, value in preserved.items():
                self.write(target, name, value)
            before = {name: exporter.digest(target / name) for name in preserved}
            for _ in range(2):
                receipt = update.apply(package, target)
                self.assertEqual(receipt['release'], '20261114-feedback-r1')
                self.assertEqual(receipt['module_sha256'], exporter.digest(target / '.local/local-server/module/local_server_bridge.dll'))
                self.assertEqual(before, {name: exporter.digest(target / name) for name in preserved})

    def test_rejects_kind_manifest_module_and_content_mismatch(self):
        for filename, field in [('patch-version.json', 'kind'), ('profiles/downloaded-files.json', 'manifest_id'),
                                 ('.local/local-server/module/build.json', 'module_sha256')]:
            with self.subTest(filename=filename), tempfile.TemporaryDirectory() as temp:
                root = Path(temp)
                self.fixture(root)
                value = update.read(root / filename)
                value[field] = 'wrong'
                self.write(root, filename, value)
                self.inventory(root)
                with self.assertRaises(update.InstallationError):
                    update.validate_package(root)
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.fixture(root)
            self.write(root, 'tools/trainer_downloaded.py', 'tampered')
            with self.assertRaises(update.InstallationError):
                update.validate_package(root)

    def test_user_data_in_inventory_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            self.fixture(root)
            self.write(root, '.local/download-installation.json', 'must preserve')
            self.inventory(root)
            with self.assertRaises(update.InstallationError):
                update.validate_package(root)


if __name__ == '__main__':
    unittest.main()
