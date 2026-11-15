from pathlib import Path
import json
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import local_server_mode_startup as subject


class ModeStartupTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='trainer-startup-test-')
        self.addCleanup(self.temp.cleanup)
        self.report = Path(self.temp.name) / 'mode-startup-state.json'
        options = subject._Options()
        options.report = str(self.report)
        options.primary_thread, options.image = 42, 0x140000000
        self.prepared = {'options': options, 'loaded': (0x100000, 0x10000, Path('bridge.dll')),
                         'configure': 0x100, 'factory': 0x200, 'receipt': {}}
        self.owned = SimpleNamespace(pid=123, call=Mock(return_value=0))
        self.gi = (0x200000, 0x10000, Path('startup.dll'))

    def test_native_rejection_without_report_preserves_actual_code(self):
        for code in (8713, 8714):
            with self.subTest(code=code), patch.object(subject, '_export') as export:
                self.owned.call.reset_mock()
                self.owned.call.return_value = code
                with self.assertRaisesRegex(RuntimeError, f'OfflineShooterModeConfigure returned {code}'):
                    subject.activate_under_hold(self.owned, self.gi, self.prepared)
                self.assertFalse(self.report.exists())
                self.assertEqual(self.owned.call.call_count, 1)
                export.assert_not_called()

    def test_success_without_report_is_explicit_and_does_not_bind(self):
        with patch.object(subject, '_export') as export:
            with self.assertRaisesRegex(RuntimeError, 'startup report is unavailable or invalid'):
                subject.activate_under_hold(self.owned, self.gi, self.prepared)
            export.assert_not_called()

    def test_malformed_report_is_explicit(self):
        self.report.write_text('{broken', encoding='utf-8')
        with self.assertRaisesRegex(RuntimeError, 'startup report is unavailable or invalid'):
            subject.activate_under_hold(self.owned, self.gi, self.prepared)

    def test_report_must_be_object(self):
        self.report.write_text('[]', encoding='utf-8')
        with self.assertRaisesRegex(RuntimeError, 'invalid startup report object'):
            subject.activate_under_hold(self.owned, self.gi, self.prepared)

    def test_failed_report_preserves_stage_without_binding(self):
        self.report.write_text(json.dumps({'configured': False, 'failed': True,
                                           'error': 8700, 'stage': 'fixture_profile_mismatch'}), encoding='utf-8')
        with patch.object(subject, '_export') as export:
            with self.assertRaisesRegex(RuntimeError, 'error=8700, stage=fixture_profile_mismatch'):
                subject.activate_under_hold(self.owned, self.gi, self.prepared)
            export.assert_not_called()

    def test_successful_report_binds_original_factory(self):
        self.report.write_text(json.dumps({'configured': True, 'failed': False, 'error': 0}), encoding='utf-8')
        with patch.object(subject, '_export', return_value=0x300):
            subject.activate_under_hold(self.owned, self.gi, self.prepared)
        self.assertEqual(self.owned.call.call_count, 2)
        self.assertEqual(self.owned.call.call_args.kwargs['operation'], 'OfflineGiInstanceBindHostModeFactory')
        self.assertTrue(self.prepared['receipt']['configured_under_original_primary_hold'])
        self.assertTrue(self.prepared['receipt']['original_gi_factory_bound_before_cdo'])


if __name__ == '__main__':
    unittest.main()
