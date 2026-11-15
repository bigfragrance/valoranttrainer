"""Ephemeral local credentials: exact ABI, isolation, no receipt disclosure."""
import base64
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import local_server_ares_key as keys
import run_local_server as runner


class NativeLocalCredentialTests(unittest.TestCase):
    def test_pair_inputs_share_only_their_own_fresh_credential_and_cleanup(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            for role in ('host', 'client'):
                (root / role).mkdir()
            public_fixture = bytes(range(32))
            with patch.object(keys.secrets, 'token_bytes', return_value=public_fixture):
                pair = keys.LocalPairKey(root, '1' * 32, 17777)
            metadata = [pair.write_role(role) for role in ('host', 'client')]
            host = keys.LAYOUT.unpack((root / 'host' / keys.MARKER_NAME).read_bytes())
            client = keys.LAYOUT.unpack((root / 'client' / keys.MARKER_NAME).read_bytes())
            self.assertEqual((host[0], host[1], host[2]), (keys.MAGIC, 1, 128))
            self.assertEqual((host[3], client[3]), (1, 2))
            self.assertEqual(host[5:8], client[5:8])
            self.assertEqual(base64.b64decode(host[7], validate=True), public_fixture)
            self.assertEqual(host[-1].hex(), keys.PIN)
            self.assertTrue(all('key' not in item and 'payload' not in item for item in metadata))
            self.assertEqual(repr(pair), '<LocalPairKey redacted>')
            with self.assertRaises(ValueError):
                pair.write_role('host')
            pair.close()
            self.assertFalse((root / 'host' / keys.MARKER_NAME).exists())
            self.assertFalse((root / 'client' / keys.MARKER_NAME).exists())

    def test_cleanup_preserves_a_file_whose_identity_was_changed(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / 'host').mkdir()
            pair = keys.LocalPairKey(root, '2' * 32, 17777)
            pair.write_role('host')
            path = root / 'host' / keys.MARKER_NAME
            path.write_bytes(b'changed test file')
            with self.assertRaises(RuntimeError):
                pair.close()
            self.assertTrue(path.exists())

    def test_local_cipher_is_explicit_and_cannot_reach_proof_or_ip_defaults(self):
        options = {'directory': Path('session').resolve(), 'proof_id': '3' * 32,
                   'port': 17777, 'transport': 'ip', 'seconds': 35, 'bootstrap': Path('fixture.bin')}
        self.assertNotIn('--native-local-cipher', runner.runner_arguments('host', **options))
        with self.assertRaises(ValueError):
            runner.runner_arguments('host', **options, native_local_cipher=True)
        options.update(transport='ares', interactive=True)
        for role in ('host', 'client'):
            self.assertIn('--native-local-cipher', runner.runner_arguments(role, **options, native_local_cipher=True))


if __name__ == '__main__':
    unittest.main()
