"""Reproduce stopping a launch while its owned Popen call is still completing."""
from pathlib import Path
import sys
import tempfile
import threading
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from training_server import Runtime
from training_settings_v2 import Settings


class Catalog:
    def check_config(self, config):
        pass


class Process:
    def poll(self):
        return None


class RuntimeLifecycleTests(unittest.TestCase):
    def test_duplicate_start_and_stop_during_process_creation_leave_no_child(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            runtime = Runtime(Settings(root / 'config.json'), Catalog())
            entering, release, stopped = threading.Event(), threading.Event(), threading.Event()
            created, retired = [], []

            def popen(*args, **kwargs):
                entering.set()
                if not release.wait(2):
                    raise RuntimeError('Test launch did not release')
                child = Process()
                created.append(child)
                return child

            def cleanup():
                with runtime.lock:
                    if runtime.process is not None:
                        retired.append(runtime.process)
                        runtime.process = None
                    if runtime.log:
                        runtime.log.close()
                    runtime.log = None
                    runtime.status = 'stopped'
                return {'status': 'stopped'}

            try:
                with patch('training_server.ROOT', root), patch('subprocess.Popen', side_effect=popen), \
                        patch('native_module.verified_build', return_value={'sha256': {'native/training_control.h': 'test'}}), \
                        patch.object(runtime, '_stop_owned_process', side_effect=cleanup):
                    runtime.start_async()
                    self.assertTrue(entering.wait(2))
                    self.assertEqual(runtime.start_async()['status'], 'loading')
                    stopper = threading.Thread(target=lambda: (runtime.stop(), stopped.set()))
                    stopper.start()
                    self.assertTrue(runtime.launch_cancelled.wait(1))
                    self.assertFalse(stopped.is_set())
                    release.set()
                    stopper.join(3)
                    self.assertTrue(stopped.is_set())
                    self.assertEqual(len(created), 1)
                    self.assertEqual(retired, created)
                    self.assertIsNone(runtime.process)
                    self.assertEqual(runtime.status, 'stopped')
            finally:
                release.set()
                runtime.close()


if __name__ == '__main__':
    unittest.main()
