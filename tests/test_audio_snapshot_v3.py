"""Offline handoff checks reject stale or incorrectly attributed audio evidence."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('audio_snapshot_checked', ROOT/'tools/verify_audio_snapshot_v3.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class AudioSnapshotChecks(unittest.TestCase):
    def setUp(self):
        parent = (ROOT/'.local/runtime/temp').resolve()
        parent.mkdir(parents=True, exist_ok=True)
        temporary = tempfile.TemporaryDirectory(prefix='audio-audit-', dir=parent)
        self.directory = Path(temporary.name).resolve()
        self.assertEqual(self.directory.parent, parent)
        self.addCleanup(temporary.cleanup)
        self.roles = dict(pid=123, local_player_count=1, actual_local_count=1,
                          players=[dict(local=True, uplayer_is_local=True, controller='0x111')])
        self.locality = dict(pid=123, audio_functions_bound=5, audio_bind_mismatches=0,
                             listener_registration_profile=True, audio_frame_context_rejects=0,
                             audio_frame_code_rejects=0, local_cache_writes=0, image_writes=0,
                             spectated_frame_profile=True, spectated_frame_copy_rejects=0,
                             spectated_context_rejects=0, spectated_code_rejects=0,
                             audio_frame_returns=0)
        self.listeners = dict(pid=123, source_verified=True, error=0,
                              actual_local_pair_valid=True, device_ready=True, waiting=False,
                              ambiguous=False, active_controller='00000111', native_calls=0,
                              native_completed=0, removals=[], image_writes=0, manual_set_writes=0,
                              volume_writes=0, locality_writes=0,
                              current=dict(valid=True, consistent=True, own=1, nonlocal_pc=0,
                                           count=2, special_preserved=1,
                                           listeners=[dict(kind='actual_local_pc', controller='0x111'),
                                                      dict(kind='non_pc_special_preserved', controller='0')]))

    def check(self):
        for name, value in [('training-player-roles', self.roles), ('original-npc-locality', self.locality),
                            ('original-audio-listeners', self.listeners)]:
            (self.directory/(name+'.json')).write_text(json.dumps(value), encoding='utf-8')
        return module.audit(self.directory, 123)

    def test_special_listener_and_unobserved_audio_are_reported_honestly(self):
        report = self.check()
        self.assertTrue(report['passed'])
        self.assertEqual(report['special_listeners_preserved'], 1)
        self.assertFalse(report['audio_blueprint_coverage_observed'])
        self.assertFalse(report['acoustic_acceptance'])

    def test_same_addresses_from_other_process_are_rejected(self):
        self.listeners['pid'] = 456
        report = self.check()
        self.assertFalse(report['passed'])
        self.assertFalse(report['checks']['same_native_process'])

    def test_top_level_own_count_cannot_hide_wrong_owner(self):
        self.listeners['current']['listeners'][0]['controller'] = '0x777'
        report = self.check()
        self.assertFalse(report['passed'])
        self.assertFalse(report['checks']['listener_owned_by_actual_player'])

    def test_extra_npc_listener_is_rejected(self):
        current = self.listeners['current']
        current['count'] = 3
        current['nonlocal_pc'] = 1
        current['listeners'].append(dict(kind='same_world_nonlocal_pc', controller='0x777'))
        self.assertFalse(self.check()['checks']['no_nonlocal_camera_listeners'])

    def test_native_remove_must_preserve_remaining_entries(self):
        self.listeners.update(native_calls=1, native_completed=1,
                              removals=[dict(original_completed=True, removed=True, remaining_preserved=False)])
        self.assertFalse(self.check()['checks']['original_remaining_set_preserved'])


if __name__ == '__main__':
    unittest.main()
