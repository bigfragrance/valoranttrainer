"""Controlled main-path cleanup and report tests; never starts a game."""
import contextlib
import io
import itertools
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import run_native_baseline as runner
import local_network_probe as probe


class MainCleanupTests(unittest.TestCase):
    def execute(self, *, launch_failure=False, postflight_error=False, cleanup_failures=False, initial_timeout=False, postflight_exited=False, close_handle_fails=False):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder).resolve()
            (root / 'project.json').write_text('{}', encoding='utf-8')
            result = root / '.local' / 'result.json'
            kernel = Mock()
            kernel.CreateFileW.return_value = 111
            kernel.CloseHandle.return_value = 0 if close_handle_fails else 1
            process = Mock(pid=41, _handle=123, returncode=None)
            process.poll.return_value = None
            process.terminate.side_effect = RuntimeError('original terminate failure')
            if initial_timeout:
                process.terminate.side_effect = None
                process.wait.side_effect = subprocess.TimeoutExpired('fixture', 5)
            if cleanup_failures:
                process.kill.side_effect = RuntimeError('kill failed')
                process.wait.side_effect = subprocess.TimeoutExpired('fixture', 5)
            channel = Mock(); host = Mock(returncode=None); hostlog = Mock(); profiles = Mock()
            if cleanup_failures:
                channel.close.side_effect = RuntimeError('channel failed')
                host.wait.side_effect = [subprocess.TimeoutExpired('host', 3), subprocess.TimeoutExpired('host', 3)]
                host.terminate.side_effect = RuntimeError('host terminate failed')
            original_cleanup = runner.cleanup_resources
            def cleanup(report, _channel, proc, _host, _log, kern, image, job, _profiles):
                original_cleanup(report, channel, proc, host, hostlog, kern, image, job, profiles)
            preflight = Mock(side_effect=[{'ready': True, 'game_sha256': 'image'},
                                         RuntimeError('Peer process identity changed') if postflight_error else
                                         {'ready': True, 'game_sha256': 'image'}])
            if postflight_exited:
                def exited_postflight():
                    result = {'ready': True, 'game_sha256': 'image', 'blocking_processes': []}
                    with patch.object(probe, 'preflight', return_value=result), patch.object(probe, 'validate_peer', return_value={'pid': 41, '_peer_exited': True}):
                        return probe.pair_preflight({}, 'fixture', 'proof', allow_exited=True)
                preflight.side_effect = [{'ready': True, 'game_sha256': 'image'}, exited_postflight()]
            popen = Mock(side_effect=RuntimeError('launch failed')) if launch_failure else Mock(return_value=process)
            args = ['runner', '--run', '--read-lock', '--job', '--seconds', '1', '--result-file', str(result)]
            with contextlib.ExitStack() as stack:
                for target, value in [('ROOT', root), ('preflight', preflight), ('client_executable', Mock(return_value=root/'client.exe')),
                                      ('project_environment', Mock(return_value={})), ('native_request', Mock(return_value=None)),
                                      ('child_environment', Mock(return_value={})), ('assign_owned_job', Mock(return_value=222)),
                                      ('game_processes', Mock(return_value=[])), ('cleanup_resources', cleanup)]:
                    stack.enter_context(patch.object(runner, target, value))
                stack.enter_context(patch.object(runner.ctypes, 'WinDLL', return_value=kernel))
                stack.enter_context(patch.object(runner.subprocess, 'Popen', popen))
                stack.enter_context(patch.object(runner.time, 'monotonic', side_effect=itertools.count()))
                stack.enter_context(patch.object(sys, 'argv', args))
                stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
                code = runner.main()
            data = json.loads(result.read_text(encoding='utf-8'))
            report = json.loads(Path(data['report_path']).read_text(encoding='utf-8'))
            self.assertEqual(data, report)
            self.assertEqual(code, 1)
            self.assertEqual(data['status'], 'error')
            channel.close.assert_called_once(); hostlog.close.assert_called_once(); profiles.close.assert_called_once()
            kernel.CloseHandle.assert_any_call(111)
            if not launch_failure:
                kernel.CloseHandle.assert_any_call(222)
            return data, process, host

    def test_launch_failure_saves_both_reports_and_closes_resources(self):
        data, _, _ = self.execute(launch_failure=True)
        self.assertEqual(data['error'], 'launch failed')

    def test_exited_peer_postflight_is_saved(self):
        data, _, _ = self.execute(postflight_exited=True)
        self.assertEqual(data['postflight']['paired_owned_host_exited']['pid'], 41)
        self.assertEqual(data['error'], 'original terminate failure')

    def test_failed_closehandle_is_recorded_and_other_cleanup_continues(self):
        data, _, _ = self.execute(close_handle_fails=True)
        actions = {e['action'] for e in data['cleanup_errors']}
        self.assertTrue({'image_handle.close', 'job_handle.close'} <= actions)
        self.assertEqual(data['error'], 'original terminate failure')

    def test_runner_wait_timeout_still_writes_reports_and_cleans_up(self):
        data, process, _ = self.execute(initial_timeout=True)
        self.assertEqual(data['error'], 'TimeoutExpired')
        self.assertGreaterEqual(process.wait.call_count, 2)

    def test_cleanup_failures_preserve_original_error_and_continue(self):
        data, process, host = self.execute(cleanup_failures=True, postflight_error=True)
        self.assertEqual(data['error'], 'original terminate failure')
        self.assertEqual(data['postflight_error']['message'], 'Peer process identity changed')
        actions = {e['action'] for e in data['cleanup_errors']}
        self.assertTrue({'native_channel.close', 'process.kill', 'process.wait', 'training_host.terminate', 'training_host.wait_after_terminate'} <= actions)
        process.wait.assert_called(); host.terminate.assert_called_once()


if __name__ == '__main__': unittest.main()
