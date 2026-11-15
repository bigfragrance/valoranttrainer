import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import installer_self_update as update
from installer_engine import InstallationError, digest, atomic_json

ROOT = Path(__file__).resolve().parents[1]


class SelfUpdateTests(unittest.TestCase):
    def release(self):
        name = 'ValorantTrainer-Setup-20261115.3.exe'
        base = 'https://github.com/a/b/releases/download/v2026.11.15.3/'
        manifest = {'schema_version': 1, 'version': '2026.11.15.3', 'asset': name,
                    'bytes': 3, 'sha256': hashlib.sha256(b'new').hexdigest()}
        assets = [{'name': update.MANIFEST, 'browser_download_url': base+update.MANIFEST},
                  {'name': name, 'browser_download_url': base+name, 'size': 3}]
        return {'tag_name': 'v2026.11.15.3', 'assets': assets}, manifest

    def test_version_and_release_validation(self):
        release, manifest = self.release()
        with patch.object(update.github_update, 'fetch', side_effect=[release, manifest]):
            self.assertTrue(update.check('a/b', '2026.11.15.2')['available'])
        with patch.object(update.github_update, 'fetch', side_effect=[release, manifest]):
            self.assertFalse(update.check('a/b', '2026.11.15.3')['available'])
        release['assets'][1]['browser_download_url'] = 'https://github.com/other/repo/file.exe'
        with patch.object(update.github_update, 'fetch', side_effect=[release, manifest]):
            with self.assertRaises(InstallationError):
                update.check('a/b', '2026.11.15.2')

    def test_corrupt_download_preserves_original(self):
        with tempfile.TemporaryDirectory() as temp:
            executable = Path(temp)/'setup.exe'
            executable.write_bytes(b'old')
            _, manifest = self.release()
            release = {'available': True, 'manifest': manifest, 'url': 'https://github.com/a/b/file'}
            def download(url, *, destination, **kwargs):
                destination.write_bytes(b'bad')
                return 3
            with patch.object(update.github_update, 'fetch', side_effect=download):
                with self.assertRaises(InstallationError):
                    update.prepare(release, executable, temp, ROOT/'packaging/installer_replace.ps1')
            self.assertEqual(executable.read_bytes(), b'old')

    @unittest.skipUnless(os.name == 'nt', 'Windows replacement worker')
    def test_real_worker_replaces_and_rolls_back(self):
        compiler = Path(os.environ['SystemRoot'])/'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
        self.assertTrue(compiler.is_file())
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root/'Fixture.cs'
            source.write_text('''using System; using System.IO; using System.Reflection; using System.Security.Cryptography;
class Fixture { static int Main(string[] args) {
if (File.Exists(Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "fail"))) return 1;
var sha = BitConverter.ToString(SHA256.Create().ComputeHash(File.ReadAllBytes(Assembly.GetExecutingAssembly().Location))).Replace("-", "").ToLowerInvariant();
File.WriteAllText(args[3], "{\\"sha256\\":\\""+sha+"\\"}"); return 0;
} }''', encoding='utf-8')
            compiled = root/'fixture.exe'
            subprocess.run([str(compiler), '/nologo', '/out:'+str(compiled), str(source)], check=True, capture_output=True)
            for failure in (False, True):
                folder = root/str(failure)
                folder.mkdir()
                target = folder/'目标 安装器.exe'
                target.write_bytes(b'old installer')
                candidate = folder/'candidate.exe'
                shutil.copyfile(compiled, candidate)
                if failure:
                    (folder/'fail').touch()
                config = folder/'handoff.json'
                result = folder/'result.json'
                settings = {'parent_pid': 2147483647, 'executable': str(target), 'candidate': str(candidate),
                            'backup': str(folder/'previous.exe'), 'ack': str(folder/'ready.json'),
                            'target': str(folder/'安装 目录'), 'result': str(result),
                            'sha256': digest(candidate), 'old_sha256': digest(target)}
                atomic_json(config, settings)
                process = subprocess.run(['powershell.exe', '-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass',
                    '-File', str(ROOT/'packaging/installer_replace.ps1'), '-Config', str(config), '-NoDialog'],
                    capture_output=True, timeout=30)
                receipt = json.loads(result.read_text(encoding='utf-8-sig'))
                self.assertEqual(receipt['status'], 'failed' if failure else 'complete', (receipt, process.stderr))
                self.assertEqual(digest(target), settings['old_sha256'] if failure else settings['sha256'])
                if not failure:
                    self.assertEqual((folder/'previous.exe').read_bytes(), b'old installer')


if __name__ == '__main__':
    unittest.main()
