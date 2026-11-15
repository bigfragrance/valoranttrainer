"""A shareable archive includes runtime inputs and omits developer/test state."""
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from export_shareable import package_files


class ShareableFileTests(unittest.TestCase):
    def test_qa_logs_settings_dumps_and_sparse_outputs_are_not_shipped(self):
        keep = {'project.json', '测试说明.txt', 'runtime/python.exe', 'tools/run_duel1_training.py',
                'resources/iostore-manifest.json', 'resources/blocks/pakchunk0-WindowsClient.ucas.bin',
                'client/ShooterGame/Content/Paks/pakchunk0-WindowsClient.utoc',
                '.local/native-module/build.json', '.local/native-module/source-snapshot/native/trainer_bridge.cpp',
                '.local/offline-gi-instance/build.json', '.local/offline-gi-instance/offline_gi_instance.dll',
                'native/offline_gi_instance.cpp', 'native/build_offline_gi_instance.cmd',
                '.local/native-baseline/portable-core/core-config.json', 'reports/portable-validation.json'}
        omit = {'client/ShooterGame/Content/Paks/pakchunk0-WindowsClient.ucas',
                'client/ShooterGame/Saved/Crashes/qa.dmp', 'client/Engine/Programs/CrashReporter/Saved/Log.txt',
                '.local/profiles/GameUserSettings.ini', '.local/native-baseline/test/runtime.log',
                '.local/package-self-check/config.bin', '.local/native-module/context-123.bin',
                'tools/verify_original_aim.py', 'tools/__pycache__/runtime.pyc', 'logs/feedback.zip',
                'reports/unreviewed-qa.json', 'resources/blocks/interrupted.tmp', 'unrelated-note.txt'}
        omit.add('native/unused-developer-helper.cpp')
        omit |= {'client/Engine/Binaries/Win64/CrashReportClient.exe',
                 'client/Engine/Programs/CrashReportClient/Content/Slate/Upload.png'}
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for relative in keep | omit:
                path = root / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b'fixture')
            self.assertEqual({p.relative_to(root).as_posix() for p in package_files(root)}, keep)


if __name__ == '__main__':
    unittest.main()
