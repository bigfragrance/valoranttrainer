"""Sparse restoration preserves offsets, rejects corruption and remains portable."""
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from resource_bundle import MANIFEST, PAKS, allocated_bytes, ensure_resources


class ResourceBundleTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        (self.root / PAKS).mkdir(parents=True)
        (self.root / 'resources/blocks').mkdir(parents=True)
        (self.root / PAKS / 'fixture.utoc').write_bytes(b'original index fixture')
        self.payload = self.root / 'resources/blocks/fixture.ucas.bin'
        self.payload.write_bytes(b'ABCDxyz')
        self.item = {'file': 'fixture.ucas', 'toc': 'fixture.utoc', 'logical_bytes': 8192,
                     'toc_sha256': hashlib.sha256(b'original index fixture').hexdigest(),
                     'payload_bytes': 7, 'ranges': [[32, 4, hashlib.sha256(b'ABCD').hexdigest()],
                                                  [4096, 3, hashlib.sha256(b'xyz').hexdigest()]]}
        self.config = {'distribution_mode': 'portable-v1'}
        self.write_manifest()

    def write_manifest(self):
        data = json.dumps({'schema_version': 1, 'files': [self.item]}).encode()
        (self.root / MANIFEST).write_bytes(data)
        self.config['resources_manifest_sha256'] = hashlib.sha256(data).hexdigest()

    def test_ranges_keep_offsets_and_holes_without_changing_index(self):
        receipt = ensure_resources(self.root, self.config)
        self.assertTrue(receipt['complete'])
        data = (self.root / PAKS / 'fixture.ucas').read_bytes()
        self.assertEqual(len(data), 8192)
        self.assertEqual(data[32:36], b'ABCD')
        self.assertEqual(data[4096:4099], b'xyz')
        self.assertEqual(data[:32], bytes(32))
        self.assertEqual((self.root / PAKS / 'fixture.utoc').read_bytes(), b'original index fixture')
        self.payload.unlink()
        self.assertTrue(ensure_resources(self.root, self.config)['complete'])

    def test_bad_payload_fails_without_publishing_partial_resource(self):
        self.payload.write_bytes(b'BAD!xyz')
        with self.assertRaisesRegex(RuntimeError, '校验失败'):
            ensure_resources(self.root, self.config)
        self.assertFalse((self.root / PAKS / 'fixture.ucas').exists())
        self.assertFalse(list((self.root / PAKS).glob('*.assembling')))

    def test_changed_resource_is_repaired_using_bound_ranges(self):
        ensure_resources(self.root, self.config)
        target = self.root / PAKS / 'fixture.ucas'
        with target.open('r+b') as stream:
            stream.seek(32)
            stream.write(b'xxxx')
        ensure_resources(self.root, self.config, verify=True)
        self.assertEqual(target.read_bytes()[32:36], b'ABCD')

    def test_modified_index_or_path_escape_prevents_restoration(self):
        (self.root / PAKS / 'fixture.utoc').write_bytes(b'modified index')
        with self.assertRaises(RuntimeError):
            ensure_resources(self.root, self.config)
        self.item['file'] = '../foreign.ucas'
        self.write_manifest()
        with self.assertRaises(RuntimeError):
            ensure_resources(self.root, self.config)

    def test_large_logical_holes_do_not_allocate_the_full_file(self):
        self.item['logical_bytes'] = 64 * 1024 * 1024
        self.write_manifest()
        ensure_resources(self.root, self.config)
        target = self.root / PAKS / 'fixture.ucas'
        self.assertEqual(target.stat().st_size, self.item['logical_bytes'])
        self.assertLess(allocated_bytes(target), 1024 * 1024)

    def test_completed_restoration_can_reclaim_packed_intermediates(self):
        self.config['discard_resource_payload_after_prepare'] = True
        receipt = ensure_resources(self.root, self.config)
        self.assertTrue(receipt['packed_payloads_removed'])
        self.assertFalse(self.payload.exists())
        self.assertTrue(ensure_resources(self.root, self.config)['complete'])


if __name__ == '__main__':
    unittest.main()
