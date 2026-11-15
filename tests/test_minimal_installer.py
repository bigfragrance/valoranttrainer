import json
from pathlib import Path
import sqlite3
import tempfile
import unittest
from test_installer_engine import DownloadFixture
import installer_engine as engine
from build_minimal_resource_plan import build
import trainer_downloaded

class MinimalInstallerTests(unittest.TestCase):
    def test_sparse_resume_repair_and_expand(self):
        with tempfile.TemporaryDirectory(dir=Path(__file__).resolve().parents[1]/'work') as folder:
            base = Path(folder)
            fixture = DownloadFixture(base)
            try:
                dependencies = base/'dependencies.json'
                raw = fixture.raw[3]
                import hashlib
                dependencies.write_text(json.dumps({'containers':[{'file':'fixture.ucas','logical_bytes':80000,'ranges':[[20010,20,hashlib.sha256(raw[10:30]).hexdigest()]]}]}))
                path = base/'minimal.sqlite'
                info = build(fixture.plan, dependencies, path)
                descriptor = {**fixture.descriptor, **{k:v for k,v in info.items() if k!='files'}, 'resource_profile':'minimal'}
                target = base/'installed'
                factory = fixture.transport
                installer = engine.Installer(path,descriptor,target,fixture.codec,transport_factory=factory)
                installer.run()
                sparse = target/'client/ShooterGame/Content/Paks/fixture.ucas'
                value = sparse.read_bytes()
                self.assertEqual(value[:20000], bytes(20000))
                self.assertEqual(value[20000:40000], fixture.raw[3])
                row = next(r for r in info['files'] if r['path'].endswith('fixture.ucas'))
                self.assertTrue(trainer_downloaded.valid_contents(sparse,row))
                previous = len(fixture.requests)
                engine.Installer(path,descriptor,target,fixture.codec,transport_factory=factory).run()
                self.assertEqual(len(fixture.requests),previous)
                with sparse.open('r+b') as stream:
                    stream.seek(20000)
                    stream.write(b'x')
                engine.Installer(path,descriptor,target,fixture.codec,transport_factory=factory).run()
                self.assertTrue(trainer_downloaded.valid_contents(sparse,row))
                before_expand = len(fixture.requests)
                engine.Installer(fixture.plan,fixture.descriptor,target,fixture.codec,transport_factory=factory).run()
                self.assertGreater(len(fixture.requests), before_expand)
                self.assertEqual(sparse.read_bytes(),fixture.contents[fixture.files[1][0]])
            finally:
                fixture.close()

if __name__ == '__main__':
    unittest.main()


