"""Own-data and fake-owned-runner checks only; never launch a game."""
from copy import deepcopy
import gzip
import hashlib
import importlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'work/humanlike-bots/root-integration/evidence-tests'
OUT.mkdir(parents=True, exist_ok=True)
sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tests')]
import local_server_sample_evidence as evidence
import run_local_server as runner
import record_local_server_map_case as map_record
import check_local_server_remote_sync as sync
import test_local_server_runner as gates
import test_local_server_interactive as lifecycle

TEMP = OUT / 'test-temp'
TEMP.mkdir(exist_ok=True)


def report_fixture():
    return {'proof_id': 'a' * 32, 'module_sha256': 'b' * 64,
            'samples': [], 'total_samples': 0, 'native_gates': {'actual_gate': False}}


def sample_fixture(n=0):
    value = gates.hero_sample()
    value['elapsed'] = n * .5
    value['host']['full_diagnostic_only'] = {'unicode': '完整诊断', 'counter': n,
                                           'types': [None, False, 2**63 + n, 0.125, -0.0]}
    value['client']['full_diagnostic_only'] = {'sample': n}
    return value


class EvidenceTests(unittest.TestCase):
    def archive(self, count=2):
        temporary = tempfile.TemporaryDirectory(dir=TEMP)
        self.addCleanup(temporary.cleanup)
        directory = Path(temporary.name)
        report = report_fixture()
        history = evidence.SampleHistory(report)
        rows = [sample_fixture(n) for n in range(count)]
        for row in rows:
            history.append(report, row)
        history.write(directory, report)
        return directory, report, history, rows

    def test_full_diagnostics_roundtrip_unicode_and_numeric_types(self):
        directory, report, history, rows = self.archive()
        self.assertEqual(list(evidence.iter_full_samples(report, directory)), rows)
        self.assertNotIn('full_diagnostic_only', report['samples'][0]['host'])
        self.assertEqual(report['sample_evidence']['archive']['status'], 'complete')

    def test_retention_limit_is_the_same_240_rows_and_total_counter(self):
        directory, report, history, rows = self.archive(265)
        self.assertEqual(len(history.members), 240)
        self.assertEqual(len(report['samples']), 240)
        self.assertEqual(report['total_samples'], 265)
        self.assertEqual(report['sample_evidence']['retained_samples'], 240)
        self.assertEqual(list(evidence.iter_full_samples(report, directory)), rows[-240:])
        self.assertEqual(sum(len(x[0]) for x in history.members), history.compressed_bytes)
        self.assertEqual(sum(x[1] for x in history.members), history.plain_bytes)

    def test_full_gate_equivalence_for_all_missing_top_level_fields(self):
        full = sample_fixture()
        for side in ('host', 'client'):
            for key in tuple(full[side]):
                with self.subTest(side=side, key=key):
                    altered = deepcopy(full)
                    del altered[side][key]
                    self.assertEqual(runner.analyze_gameplay_samples([altered]),
                                     runner.analyze_gameplay_samples([evidence.gate_sample(altered)]))

    def test_actual_negative_hero_gates_are_not_relaxed(self):
        for side, field, value in (
                ('host', 'error', 7924), ('client', 'playable_error', 4920),
                ('host', 'authority_hero', '0'), ('host', 'local_services_ready', False),
                ('client', 'current_world', '10000'), ('client', 'local_pawn_role', 3),
                ('client', 'local_controller_is_connection_owner', False)):
            with self.subTest(side=side, field=field):
                row = sample_fixture()
                row[side][field] = value
                projected = evidence.gate_sample(row)
                self.assertEqual(runner.analyze_gameplay_samples([row]),
                                 runner.analyze_gameplay_samples([projected]))
                self.assertFalse(runner.analyze_gameplay_samples([projected])['shooter_hero_round_trip_proven'])

    def test_remote_timed_trajectory_gate_keeps_all_inputs(self):
        rows = []
        for n in range(7):
            row = sample_fixture(n)
            row['host'].update(remote_pawn='90000', remote_ai_is_local=True,
                               remote_pawn_is_local=True, server_remote_position=[n * 30., 2., 3.],
                               server_remote_sample_at_ms=n * 500 + 1000, remote_move_submissions=n * 3)
            row['client'].update(client_remote_count=1, client_remote_position=[n * 30. + 1, 2., 3.],
                                 client_remote_sample_at_ms=n * 500 + 1000)
            rows.append(row)
        expected = runner.analyze_gameplay_samples(rows)
        self.assertTrue(expected['server_ai_movement_replicated_proven'])
        self.assertEqual(expected, runner.analyze_gameplay_samples([evidence.gate_sample(r) for r in rows]))
        rows[-1]['client']['client_remote_position'] = [10000., 2., 3.]
        self.assertEqual(runner.analyze_gameplay_samples(rows),
                         runner.analyze_gameplay_samples([evidence.gate_sample(r) for r in rows]))

    def test_projection_is_detached_and_does_not_mutate_full_data(self):
        row = sample_fixture()
        old = deepcopy(row)
        result = evidence.gate_sample(row)
        result['host']['host_connections'][0]['pawn'] = '0'
        self.assertEqual(row, old)

    def test_unserializable_row_never_partly_advances_counters(self):
        report = report_fixture()
        history = evidence.SampleHistory(report)
        row = sample_fixture()
        row['host']['unserializable'] = object()
        with self.assertRaises(TypeError):
            history.append(report, row)
        self.assertEqual((len(history.members), report['total_samples'], report['samples']), (0, 0, []))

    def test_progress_is_explicitly_projection_and_preserves_actual_verdict(self):
        directory, report, history, rows = self.archive()
        for key in evidence.FULL_REPORT_FIELDS:
            report[key] = {'large_full_state': 'x' * 5000}
        old = deepcopy(report)
        current = runner.progress_snapshot(report)
        self.assertEqual(current['native_gates'], {'actual_gate': False})
        self.assertEqual(current['progress_samples_scope'], 'native_gate_projection_v1')
        self.assertEqual(len(current['samples']), 1)
        self.assertEqual(current['history_samples_retained_for_verification'], 2)
        self.assertTrue(all(k not in current for k in evidence.FULL_REPORT_FIELDS))
        self.assertEqual(report, old)

    def test_legacy_report_reader_and_progress_are_unchanged(self):
        report = {'samples': [sample_fixture(0), sample_fixture(1)], 'total_samples': 2}
        self.assertEqual(list(evidence.iter_full_samples(report, None)), report['samples'])
        self.assertNotIn('sample_evidence', runner.progress_snapshot(report))
        self.assertEqual(runner.progress_snapshot(report)['progress_samples_scope'], 'latest_pair_snapshot')

    def test_empty_session_can_retain_an_explicit_complete_empty_history(self):
        directory, report, history, rows = self.archive(0)
        self.assertEqual(list(evidence.iter_full_samples(report, directory)), [])

    def test_unknown_schema_pending_failed_and_path_escape_are_not_full_evidence(self):
        directory, report, history, rows = self.archive()
        changes = [('schema_version', 2), ('kind', 'unknown'), ('retention_limit_samples', 239),
                   ('inline_samples_scope', 'full'), ('retained_samples', 3), ('total_samples', -1)]
        for field, value in changes:
            with self.subTest(field=field):
                changed = deepcopy(report)
                changed['sample_evidence'][field] = value
                with self.assertRaises(ValueError):
                    list(evidence.iter_full_samples(changed, directory))
        for field, value in [('status', 'pending'), ('status', 'failed'), ('name', '../outside.gz'),
                             ('format', 'raw'), ('sha256', '0' * 64), ('bytes', 0),
                             ('decompressed_bytes', 1), ('decompressed_bytes', 99999999)]:
            with self.subTest(archive_field=field, value=value):
                changed = deepcopy(report)
                changed['sample_evidence']['archive'][field] = value
                with self.assertRaises(ValueError):
                    list(evidence.iter_full_samples(changed, directory))

    def test_archive_is_bound_to_same_proof_and_build(self):
        directory, report, history, rows = self.archive()
        for field in ('proof_id', 'module_sha256'):
            changed = deepcopy(report)
            changed[field] = 'wrong'
            with self.assertRaises(ValueError):
                list(evidence.iter_full_samples(changed, directory))

    def test_truncated_archive_cannot_be_used(self):
        directory, report, history, rows = self.archive()
        path = directory / evidence.ARCHIVE_NAME
        path.write_bytes(path.read_bytes()[:-4])
        with self.assertRaises(ValueError):
            list(evidence.iter_full_samples(report, directory))

    def test_valid_gzip_but_extra_row_is_rejected_even_with_matching_hash(self):
        directory, report, history, rows = self.archive()
        path = directory / evidence.ARCHIVE_NAME
        extra = evidence._encode(sample_fixture(5))
        data = path.read_bytes() + gzip.compress(extra, mtime=0)
        path.write_bytes(data)
        report['sample_evidence']['archive'].update(bytes=len(data), sha256=hashlib.sha256(data).hexdigest(),
            decompressed_bytes=report['sample_evidence']['archive']['decompressed_bytes'] + len(extra))
        with self.assertRaises(ValueError):
            list(evidence.iter_full_samples(report, directory))

    def test_existing_archive_is_never_replaced(self):
        directory, report, history, rows = self.archive()
        path = directory / evidence.ARCHIVE_NAME
        original = path.read_bytes()
        with self.assertRaises(FileExistsError):
            history.write(directory, report)
        self.assertEqual(path.read_bytes(), original)

    def test_map_consumer_requires_real_directory_for_projected_report(self):
        directory, report, history, rows = self.archive()
        with self.assertRaisesRegex(ValueError, 'actual pair directory'):
            map_record.checks_for_pair(report, {}, {}, package='x', map_id='duel1')


class LifecycleExtras(unittest.TestCase):
    def simulate(self, **kwargs):
        return lifecycle.InteractiveLifecycleTests().simulate(**kwargs)

    def test_archive_write_failure_fails_after_exact_owned_cleanup(self):
        with patch.object(evidence.SampleHistory, 'write', side_effect=OSError('fixture archive disk full')):
            code, report, events, processes = self.simulate(stop_at=2)
        self.assertEqual(code, 1)
        self.assertEqual(events[-2:], ['shutdown_client', 'shutdown_host'])
        self.assertFalse(report['completion']['passed'])
        self.assertEqual(report['sample_evidence']['archive']['status'], 'failed')
        self.assertIn('full interactive evidence', report['error'])

    def test_explicit_native_scenario_never_installs_new_retention_policy(self):
        original_namespace = lifecycle.SimpleNamespace
        def namespace(**kwargs):
            if 'controller_settings' in kwargs:
                kwargs['scenario'] = 'shoot'
            return original_namespace(**kwargs)
        with patch.object(lifecycle, 'SimpleNamespace', side_effect=namespace), \
                patch.object(runner, 'SampleHistory', side_effect=AssertionError('fixture policy changed')):
            code, report, events, processes = self.simulate(stop_at=2)
        self.assertEqual(code, 0)
        self.assertNotIn('sample_evidence', report)
        self.assertTrue(report['completion']['passed'])


if __name__ == '__main__':
    suite = unittest.TestSuite()
    loader = unittest.defaultTestLoader
    for module in (sys.modules[__name__], gates, lifecycle,
                   importlib.import_module('test_local_server_map_record')):
        suite.addTests(loader.loadTestsFromModule(module))
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    (OUT / 'own-tests.json').write_text(json.dumps({
        'scope': 'own-data archives and existing fake Popen lifecycle tests; no game or native target run',
        'tests_run': result.testsRun, 'failures': len(result.failures), 'errors': len(result.errors),
        'successful': result.wasSuccessful(),
        'failure_details': [str(test) + ': ' + trace for test, trace in (*result.failures, *result.errors)],
    }, indent=2) + '\n', encoding='utf-8')
    raise SystemExit(0 if result.wasSuccessful() else 1)
