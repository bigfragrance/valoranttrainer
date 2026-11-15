"""Map proof rejects selected-config claims without actual same-frame worlds."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from record_local_server_map_case import checks_for_pair
from test_local_server_runner import hero_sample

PACKAGE = '/Game/Maps/Ascent/Ascent'


def pair_fixture():
    samples = []
    for n in range(13):
        sample = hero_sample()
        sample['elapsed'] = n / 2
        for side in ('host', 'client'):
            current = sample[side].setdefault('current_world', '20000')
            sample[side]['observed_world_identity'] = {'world': current, 'package_name': PACKAGE}
            sample[side]['session_runtime'] = {'ready': True}
        samples.append(sample)
    report = {'map_package': PACKAGE, 'samples': samples, 'completion': {'passed': True},
              'error': None, 'host_critical_errors': 0, 'client_critical_errors': 0,
              'protected_builds_unchanged': True, 'candidate_build_unchanged': True}
    host = {'initialized': True, 'host_ready': True, 'native_active': True, 'map_id': 'ascent', 'error': 0}
    client = {'initialized': True, 'passive_client': True, 'native_active': True,
              'client_layers_ready': True, 'client_local_readbacks_match': True,
              'map_id': 'ascent', 'error': 0}
    return report, host, client


class MapRecordEvidenceTests(unittest.TestCase):
    def check(self, report, host, client):
        return checks_for_pair(report, host, client, package=PACKAGE, map_id='ascent')

    def test_same_pair_original_worlds_and_completed_lifecycle_pass_loading_scope(self):
        self.assertTrue(all(self.check(*pair_fixture()).values()))

    def test_configured_map_without_actual_world_identity_cannot_pass(self):
        report, host, client = pair_fixture()
        for sample in report['samples']:
            sample['client']['observed_world_identity']['package_name'] = '/Game/Maps/Init/Init'
        checks = self.check(report, host, client)
        self.assertFalse(checks['native_world_identity_seen_on_both_sides'])
        self.assertFalse(checks['actual_join_and_owned_shooter_hero'])

    def test_interrupted_loading_is_not_a_stable_interval(self):
        report, host, client = pair_fixture()
        report['samples'][6]['host']['session_runtime']['ready'] = False
        self.assertFalse(self.check(report, host, client)['stable_native_graph_for_at_least_five_seconds'])

    def test_environment_and_cleanup_evidence_are_both_required(self):
        for field in ('client_local_readbacks_match', 'client_layers_ready'):
            report, host, client = pair_fixture()
            client[field] = False
            self.assertFalse(all(self.check(report, host, client).values()))
        report, host, client = pair_fixture()
        report['completion']['passed'] = False
        self.assertFalse(self.check(report, host, client)['complete_owned_pair_shutdown'])


if __name__ == '__main__':
    unittest.main()
