import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import trainer_patch as subject


class InstalledPatchTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(dir=Path(__file__).resolve().parents[1] / 'work')
        self.base = Path(self.temporary.name).resolve()
        self.root = self.base / '补丁 测试'
        self.source = self.base / '另一个盘 游戏/live'
        self.root.mkdir()
        self.image = self.source / subject.IMAGE
        self.image.parent.mkdir(parents=True)
        self.image.write_bytes(b'matching-test-image')
        self.paks = self.source / 'ShooterGame/Content/Paks'
        self.paks.mkdir(parents=True)
        (self.paks / 'data.utoc').write_bytes(b'toc')
        (self.paks / 'data.ucas').write_bytes(b'payload')
        self.manifest = {'image': {'bytes': self.image.stat().st_size, 'sha256': subject.digest(self.image)},
                         'loose_files': [{'file': subject.IMAGE, 'bytes': self.image.stat().st_size,
                                          'sha256': subject.digest(self.image)}],
                         'resources': [{'file': 'data.utoc', 'bytes': 3, 'sha256': subject.digest(self.paks / 'data.utoc')},
                                       {'file': 'data.ucas', 'bytes': 7}]}

    def tearDown(self):
        junction = self.root / 'client/ShooterGame/Content/Paks'
        if junction.exists() and junction.resolve() == self.paks:
            junction.rmdir()  # Remove the exact junction only, never its source.
        self.temporary.cleanup()

    def test_installation_accepts_parent_and_image_paths(self):
        for path in (self.source, self.source.parent, self.image):
            self.assertEqual(subject.installation_root(path), self.source)
        self.assertEqual(subject.validate_installation(self.source, self.manifest, root=self.root),
                         (self.source, self.paks))

    def test_rejects_same_size_changed_image(self):
        self.image.write_bytes(b'X' * self.image.stat().st_size)
        with self.assertRaisesRegex(RuntimeError, '版本不匹配'):
            subject.validate_installation(self.source, self.manifest, root=self.root)

    def test_rejects_changed_resource_index(self):
        (self.paks / 'data.utoc').write_bytes(b'bad')
        with self.assertRaisesRegex(RuntimeError, '索引或签名'):
            subject.validate_installation(self.source, self.manifest, root=self.root)

    def test_rejects_changed_resource_data_length(self):
        (self.paks / 'data.ucas').write_bytes(b'short')
        with self.assertRaisesRegex(RuntimeError, '资源缺失或版本不同'):
            subject.validate_installation(self.source, self.manifest, root=self.root)

    def test_refuses_export_inside_installation(self):
        with self.assertRaisesRegex(RuntimeError, '独立文件夹'):
            subject.validate_installation(self.source, self.manifest, root=self.source / 'patch')

    def test_owned_paths_cannot_escape(self):
        for path in ('../file', 'C:/file', '/file', 'folder\\file', 1):
            with self.assertRaises(ValueError):
                subject.owned(self.root, path)

    @unittest.skipUnless(os.name == 'nt', 'Windows directory junction')
    def test_real_junction_reuse_and_source_files_untouched(self):
        target = self.root / 'client/ShooterGame/Content/Paks'
        before = {p.name: (p.read_bytes(), p.stat().st_mtime_ns) for p in self.paks.iterdir()}
        subject.create_junction(target, self.paks)
        self.assertEqual(target.resolve(), self.paks)
        subject.create_junction(target, self.paks)
        self.assertEqual(before, {p.name: (p.read_bytes(), p.stat().st_mtime_ns) for p in self.paks.iterdir()})
        with self.assertRaises(ValueError):
            subject.owned(self.root, 'client/ShooterGame/Content/Paks/data.utoc')

    @unittest.skipUnless(os.name == 'nt', 'Windows directory junction')
    def test_binding_copies_image_reuses_assets_and_keeps_existing_settings(self):
        for name, value in (('patch-installation-manifest.json', self.manifest),
                            ('patch-project.json', {'account_login': 'forbidden'}),
                            ('patch-default-settings.json', {'revision': 1})):
            subject.write_json(self.root / 'profiles' / name, value)
        before = {str(p.relative_to(self.source)): (subject.digest(p), p.stat().st_mtime_ns)
                  for p in self.source.rglob('*') if p.is_file()}
        receipt = subject.bind_installation(self.source, root=self.root)
        self.assertEqual(receipt['resources_copied_bytes'], 0)
        self.assertFalse(receipt['source_files_modified'])
        copied = self.root / 'client' / subject.IMAGE
        self.assertEqual(copied.read_bytes(), self.image.read_bytes())
        self.assertNotEqual(copied.stat().st_ino, self.image.stat().st_ino)
        subject.write_json(self.root / subject.SETTINGS, {'revision': 7})
        receipt = subject.bind_installation(self.source, root=self.root)
        self.assertEqual(receipt['copied_this_time_bytes'], 0)
        self.assertEqual(subject.read_json(self.root / subject.SETTINGS)['revision'], 7)
        after = {str(p.relative_to(self.source)): (subject.digest(p), p.stat().st_mtime_ns)
                 for p in self.source.rglob('*') if p.is_file()}
        self.assertEqual(before, after)


if __name__ == '__main__':
    unittest.main()
