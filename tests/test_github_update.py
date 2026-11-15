import hashlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import github_update as update
import apply_trainer_update as offline
from test_feedback_update_package import FeedbackUpdatePackageTests


class GitHubUpdateTests(unittest.TestCase):
    def test_download_reports_incremental_bytes(self):
        events = []
        data = b'x' * 200000
        with tempfile.TemporaryDirectory() as temp:
            target = Path(temp) / 'update.zip'
            with patch.object(update, 'urlopen', return_value=io.BytesIO(data)):
                self.assertEqual(update.fetch('https://github.com/a/b/file', limit=len(data),
                                             destination=target, progress=events.append), len(data))
            self.assertEqual(target.read_bytes(), data)
        self.assertEqual(events[0]['completed'], 0)
        self.assertEqual(events[-1]['completed'], len(data))
        self.assertGreater(len(events), 3)
        self.assertEqual(events[-1]['total'], len(data))
        self.assertIn('总大小未知', update.progress_text({'stage': 'download', 'completed': 1, 'total': None}))

    def test_install_progress_covers_download_and_apply(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            package, target = self.fixture(root)
            archive = root / 'release.zip'
            with zipfile.ZipFile(archive, 'w') as bundle:
                for path in package.rglob('*'):
                    if path.is_file():
                        bundle.write(path, path.relative_to(package).as_posix())
            data = archive.read_bytes()
            release = {'available': True, 'url': 'https://github.com/a/b/file', 'manifest': {
                'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(),
                'version': offline.read(package / 'patch-version.json')['release']}}
            events = []
            with patch.object(update, 'urlopen', return_value=io.BytesIO(data)):
                update.install(target, release, events.append)
            stages = {event['stage'] for event in events}
            self.assertTrue({'connect', 'download', 'checksum', 'extract', 'validate', 'backup', 'install', 'verify', 'complete'} <= stages)
            self.assertEqual(events[-1]['stage'], 'complete')
            events.clear()
            release['manifest']['sha256'] = '0' * 64
            with patch.object(update, 'urlopen', return_value=io.BytesIO(data)):
                with self.assertRaises(offline.InstallationError):
                    update.install(target, release, events.append)
            self.assertNotIn('complete', {event['stage'] for event in events})

    def fixture(self, root):
        helper = FeedbackUpdatePackageTests()
        package, target = root / 'package', root / 'target'
        helper.fixture(package)
        helper.write(target, 'patch-version.json', {'kind': offline.KIND, 'manifest_id': '448DAE34A9D16C88', 'release': '2026.11.13'})
        helper.write(target, '.local/my-settings.json', 'keep')
        return package, target

    def test_release_version_ordering(self):
        self.assertGreater(update.version_key('2026.11.15'), update.version_key('20261114-feedback-r2'))
        self.assertGreater(update.version_key('2026.11.15-r2'), update.version_key('2026.11.15'))

    def test_repository_validation(self):
        for name in ('', '../repo', 'https://github.com/a/b', 'a/b/c'):
            with self.assertRaises(offline.InstallationError):
                update.repository_name(name)

    def test_check_compatible_release(self):
        with tempfile.TemporaryDirectory() as temp:
            _, target = self.fixture(Path(temp))
            manifest = {'schema_version': 1, 'version': '2026.11.15', 'manifest_id': '448DAE34A9D16C88', 'sha256': 'a'*64, 'bytes': 100, 'asset': 'update.zip'}
            release = {'tag_name': 'v2026.11.15', 'assets': [{'name': update.MANIFEST, 'browser_download_url': 'https://github.com/a/b/manifest'}, {'name': 'update.zip', 'size': 100, 'browser_download_url': 'https://github.com/a/b/package'}]}
            with patch.object(update, 'fetch', side_effect=[release, manifest]):
                result = update.check(target, 'a/b')
            self.assertTrue(result['available'])
            manifest['manifest_id'] = 'wrong'
            with patch.object(update, 'fetch', side_effect=[release, manifest]):
                with self.assertRaises(offline.InstallationError): update.check(target, 'a/b')

    def test_update_download_apply_preserves_settings(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            package, target = self.fixture(root)
            archive = root / 'release.zip'
            with zipfile.ZipFile(archive, 'w') as bundle:
                for path in package.rglob('*'):
                    if path.is_file(): bundle.write(path, path.relative_to(package).as_posix())
            data = archive.read_bytes()
            manifest = {'version': offline.read(package / 'patch-version.json')['release'], 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
            release = {'available': True, 'url': 'https://github.com/a/b/download', 'manifest': manifest}
            def download(url, *, limit, destination):
                destination.write_bytes(data)
                return len(data)
            with patch.object(update, 'fetch', side_effect=download): update.install(target, release)
            self.assertEqual((target / '.local/my-settings.json').read_text(), 'keep')
            self.assertEqual(offline.read(target / 'patch-version.json')['release'], manifest['version'])
            manifest['sha256'] = '0'*64
            with patch.object(update, 'fetch', side_effect=download):
                with self.assertRaises(offline.InstallationError): update.install(target, release)

    def test_reject_user_paths_with_windows_case_variants(self):
        with tempfile.TemporaryDirectory() as temp:
            package, _ = self.fixture(Path(temp))
            helper = FeedbackUpdatePackageTests()
            helper.write(package, '.LOCAL/controller/settings.json', 'unsafe')
            helper.inventory(package)
            with self.assertRaises(offline.InstallationError):
                offline.validate_package(package)

    def test_reject_archive_parent_traversal(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            _, target = self.fixture(root)
            archive = root / 'bad.zip'
            with zipfile.ZipFile(archive, 'w') as bundle:
                bundle.writestr('../outside.txt', 'unsafe')
            data = archive.read_bytes()
            release = {'available': True, 'url': 'https://github.com/a/b/asset', 'manifest': {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}}
            def download(url, *, limit, destination):
                destination.write_bytes(data)
                return len(data)
            with patch.object(update, 'fetch', side_effect=download):
                with self.assertRaises(offline.InstallationError):
                    update.install(target, release)
            self.assertFalse((root / 'outside.txt').exists())

    def test_update_rollback_after_partial_failure(self):
        with tempfile.TemporaryDirectory() as temp:
            package, target = self.fixture(Path(temp))
            before = (target / 'patch-version.json').read_bytes()
            def fail(archive, expected, destination):
                (target / 'patch-version.json').write_text('broken')
                raise OSError('injected disk failure')
            events = []
            with patch.object(offline, 'extract_patch', side_effect=fail):
                with self.assertRaises(OSError): offline.apply(package, target, events.append)
            self.assertEqual((target / 'patch-version.json').read_bytes(), before)
            self.assertFalse((target / '.setup/update-transaction/journal.json').exists())
            self.assertEqual(events[-1]['stage'], 'rollback')

    def test_recover_interrupted_update_before_retry(self):
        with tempfile.TemporaryDirectory() as temp:
            package, target = self.fixture(Path(temp))
            backup = target / '.setup/update-transaction/backup/old.txt'
            backup.parent.mkdir(parents=True)
            backup.write_text('old value')
            (target / 'old.txt').write_text('partial value')
            (backup.parents[1] / 'journal.json').write_text(json.dumps({'files': ['old.txt'], 'existed': ['old.txt']}))
            offline.apply(package, target)
            self.assertEqual((target / 'old.txt').read_text(), 'old value')

if __name__ == '__main__': unittest.main()
