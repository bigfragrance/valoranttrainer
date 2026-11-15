import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import trainer_dependencies as subject
from trainer_patch import write_json,digest


class StandaloneResourceTests(unittest.TestCase):
    def setUp(self):
        self.temporary=tempfile.TemporaryDirectory(dir=Path(__file__).resolve().parents[1]/'work')
        self.root=Path(self.temporary.name).resolve()
        self.bundle=self.root/'fixture.vtr'
        self.toc=b'original-test-index'
        sha=lambda value:hashlib.sha256(value).hexdigest()
        self.manifest={'schema_version':1,'kind':'original-signed-ranges-v1',
                       'containers':[{'file':'test.ucas','toc':'test.utoc','logical_bytes':1024,'payload_bytes':8,
                                      'member':'payload/test.bin','ranges':[[0,4,sha(b'head')],[800,4,sha(b'tail')]]}],
                       'passthrough':[{'file':'test.utoc','bytes':len(self.toc),'member':'paks/test.utoc','sha256':sha(self.toc)}]}
        payload=json.dumps(self.manifest,separators=(',',':')).encode()
        with zipfile.ZipFile(self.bundle,'x') as output:
            output.writestr('dependency-manifest.json',payload)
            output.writestr('payload/test.bin',b'headtail')
            output.writestr('paks/test.utoc',self.toc)
        profile=self.root/'profiles/standalone-dependencies.json'
        profile.parent.mkdir()
        profile.write_bytes(payload)
        self.version={'dependency_manifest_sha256':sha(payload),'dependency_file':self.bundle.name,
                      'dependency_sha256':digest(self.bundle),'dependency_bytes':self.bundle.stat().st_size}
        write_json(self.root/'patch-version.json',self.version)
        write_json(self.root/'profiles/patch-project.json',{'account_login':'forbidden'})
        write_json(self.root/'profiles/patch-default-settings.json',{'revision':3})

    def tearDown(self):
        self.temporary.cleanup()

    def test_original_offsets_and_holes_restore_without_installed_game(self):
        state=subject.prepare(self.root)
        self.assertTrue(state['complete'])
        path=self.root/subject.PAKS/'test.ucas'
        data=path.read_bytes()
        self.assertEqual(len(data),1024)
        self.assertEqual(data[:4],b'head')
        self.assertEqual(data[800:804],b'tail')
        self.assertEqual(data[4:800],bytes(796))
        self.assertEqual((self.root/subject.PAKS/'test.utoc').read_bytes(),self.toc)
        self.assertTrue(subject.status(self.root,verify=True)['complete'])
        config=json.loads((self.root/'project.json').read_text())
        self.assertEqual(Path(config['source_resource_directory']),self.root/subject.PAKS)
        self.assertNotIn('source_game_executable',config)

    def test_prepared_files_run_without_bundle_and_settings_are_preserved(self):
        subject.prepare(self.root)
        write_json(self.root/subject.SETTINGS,{'revision':7})
        self.bundle.rename(self.root/'retained-elsewhere.vtr')
        self.assertTrue(subject.prepare(self.root)['complete'])
        self.assertEqual(json.loads((self.root/subject.SETTINGS).read_text())['revision'],7)

    def test_changed_selected_range_is_detected_and_repaired(self):
        subject.prepare(self.root)
        path=self.root/subject.PAKS/'test.ucas'
        old=path.stat().st_mtime_ns
        with path.open('r+b') as stream:
            stream.write(b'bad!')
        os.utime(path,ns=(old+1000000,old+1000000))
        self.assertFalse(subject.status(self.root)['complete'])
        subject.prepare(self.root)
        self.assertEqual(path.read_bytes()[:4],b'head')

    def test_changed_transport_fails_before_any_resource_is_installed(self):
        with self.bundle.open('ab') as stream:stream.write(b'bad')
        self.version['dependency_bytes']=self.bundle.stat().st_size
        write_json(self.root/'patch-version.json',self.version)
        with self.assertRaisesRegex(RuntimeError,'校验失败'):
            subject.prepare(self.root)
        self.assertFalse((self.root/subject.PAKS/'test.ucas').exists())

    def test_manifest_change_and_path_escape_fail_closed(self):
        self.manifest['containers'][0]['file']='../outside.ucas'
        path=self.root/'profiles/standalone-dependencies.json'
        path.write_text(json.dumps(self.manifest),encoding='utf-8')
        with self.assertRaisesRegex(RuntimeError,'清单已改变'):
            subject.manifest(self.root)
        self.version['dependency_manifest_sha256']=digest(path)
        write_json(self.root/'patch-version.json',self.version)
        with self.assertRaisesRegex(RuntimeError,'文件名'):
            subject.manifest(self.root)

    def test_missing_bundle_reports_actionable_filename(self):
        self.bundle.rename(self.root/'not-in-package.vtr')
        with self.assertRaisesRegex(RuntimeError,'fixture.vtr'):
            subject.prepare(self.root)


if __name__=='__main__':
    unittest.main()
