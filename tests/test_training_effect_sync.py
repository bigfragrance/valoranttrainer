"""Staged native snapshots exercise deferred cosmetics without launching a game."""
from copy import deepcopy
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

from test_training_console_sync import FakeChannel, runtime
from training_settings_v2 import Settings


WEAPON, OTHER_WEAPON = '1' * 32, '2' * 32
SKIN = dict(skin_id='3' * 32, level_id='4' * 32, chroma_id='5' * 32)
KEY = 'skin_kill_count'


class Catalog:
    @staticmethod
    def skin_payload(weapon, skin, level, chroma):
        return bytes.fromhex(weapon + skin + level + chroma)


class AsyncChannel(FakeChannel):
    def __init__(self, game):
        super().__init__(game)
        self.effect_results = []

    def send(self, kind, payload):
        result = super().send(kind, payload)
        if kind == 'effect':
            next_result = self.effect_results.pop(0) if self.effect_results else 1
            if isinstance(next_result, Exception):
                raise next_result
            result.update(code=next_result, status={1: 'pending', 3: 'unavailable', 4: 'failed'}[next_result],
                          message={1: 'original_effect_override_submitted',
                                   3: 'original_owned_champion_effect_not_ready',
                                   4: 'original_effect_dispatch_failed'}[next_result])
        return result


class EffectSyncTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.settings = Settings(Path(self.directory.name) / 'config.json')
        self.config = self.settings.update({'skins': {WEAPON: SKIN},
                                           'effects': {KEY: {'mode': 1, 'value': 12}}}, 0)
        self.live = runtime(self.settings, Catalog())
        self.live.channel = AsyncChannel({'ready': True, 'map_id': 'duel1', 'applied_revision': self.config['revision'],
                                         'weapon': {'ready': True, 'id': WEAPON, **SKIN},
                                         'effects': [self.control()]})

    def control(self, **changes):
        return dict(key=KEY, numeric=True, minimum=0, maximum=25, requested=0, value=0,
                    applied=0, verified=False, supported=True, available=True, **changes)

    def commands(self, kind):
        return [payload for name, payload in self.live.channel.sent if name == kind]

    def test_no_effect_command_during_config_or_startup_and_default_held_skin_is_refreshed(self):
        self.live.channel.game['weapon']['skin_id'] = '6' * 32
        self.live.apply(self.config, selections=True)
        self.assertEqual([name for name, _ in self.live.channel.sent],
                         ['configure', 'routes', 'save_skin_selection', 'skin'])
        self.assertEqual(len(self.commands('skin')[0]), 64)
        self.live.apply(self.config)
        self.assertEqual(self.commands('effect'), [])
        self.assertEqual(self.commands('weapon'), [])

    def test_waits_for_skin_and_available_control_without_faking_applied_state(self):
        game = self.live.channel.game
        game['ready'] = False
        self.live.sync_effects()
        game['ready'] = True
        game['weapon']['skin_id'] = '6' * 32
        self.live.sync_effects()
        game['weapon'].update(SKIN)
        game['effects'][0]['supported'] = False
        self.live.sync_effects()
        game['effects'][0]['supported'] = True
        game['effects'][0]['available'] = False
        self.live.sync_effects()
        self.assertEqual(self.commands('effect'), [])
        game['effects'][0]['available'] = True
        before = deepcopy(game['effects'][0])
        self.live.sync_effects()
        self.assertEqual(len(self.commands('effect')), 1)
        self.assertEqual(game['effects'][0], before)
        self.assertEqual(self.live.last_command['status'], 'pending')
        game['effects'][0].update(requested=1, value=12)
        self.live.sync_effects()
        self.assertEqual(len(self.commands('effect')), 1)
        self.assertFalse(self.live.state()['game']['effects'][0]['verified'])
        self.assertEqual(self.live.state()['game']['effects'][0]['applied'], 0)

    def test_candidate_stores_supported_preference_before_effect_actor_exists(self):
        self.live.backend = 'local-server-candidate'
        control = self.live.channel.game['effects'][0]
        control['available'] = False
        before = deepcopy(control)
        self.live.sync_effects()
        self.assertEqual(len(self.commands('effect')), 1)
        self.assertEqual(self.live.last_command['status'], 'pending')
        self.assertEqual(control, before)
        control.update(requested=1, value=12)
        self.live.sync_effects()
        self.assertEqual(len(self.commands('effect')), 1)
        self.assertFalse(control['verified'])
        self.assertEqual(control['applied'], 0)

    def test_unavailable_is_deferred_then_retried_without_startup_error(self):
        self.live.channel.effect_results = [3, 1]
        with patch('training_server.time.monotonic', return_value=10):
            self.live.sync_effects()
        self.assertEqual(self.live.last_command['status'], 'unavailable')
        self.assertIsNone(self.live.effect_sync_error)
        with patch('training_server.time.monotonic', return_value=10.5):
            self.live.sync_effects()
        self.assertEqual(len(self.commands('effect')), 1)
        with patch('training_server.time.monotonic', return_value=11.1):
            self.live.sync_effects()
        self.assertEqual(len(self.commands('effect')), 2)
        self.assertIsNone(self.live.error)

    def test_failed_and_transport_errors_are_visible_but_do_not_break_plain_settings(self):
        for result in (4, OSError('test channel write failed')):
            with self.subTest(result=result):
                self.live._reset_effect_sync()
                self.live.channel.effect_results = [result]
                self.live.sync_effects()
                self.assertIn('皮肤效果同步失败', self.live.state()['error_text'])
                self.assertIsNone(self.live.error)
                before = len(self.commands('effect'))
                self.live.apply(self.config)
                self.assertEqual(len(self.commands('effect')), before)
                self.live.channel.game['effects'][0].update(requested=1, value=12)
                self.live.sync_effects()
                self.assertIsNone(self.live.effect_sync_error)
                self.live.channel.game['effects'][0].update(requested=0, value=0)

    def test_missing_key_on_other_weapon_is_ignored_and_switching_back_rechecks(self):
        with patch('training_server.time.monotonic', return_value=10):
            self.live.sync_effects()
            self.live.channel.game['weapon'] = {'ready': True, 'id': OTHER_WEAPON}
            self.live.channel.game['effects'] = []
            self.live.sync_effects()
            self.live.channel.game['weapon'] = {'ready': True, 'id': WEAPON, **SKIN}
            self.live.channel.game['effects'] = [self.control()]
            self.live.sync_effects()
        self.assertEqual(len(self.commands('effect')), 2)
        self.assertIsNone(self.live.effect_sync_error)

    def test_numeric_difference_and_changed_desired_value_are_not_lost_to_retry_delay(self):
        self.live.channel.game['effects'][0].update(requested=1, value=11)
        with patch('training_server.time.monotonic', return_value=10):
            self.live.sync_effects()
            self.settings.update({'effects': {KEY: {'mode': 1, 'value': 13}}}, self.config['revision'])
            self.live.sync_effects()
        self.assertEqual([struct.unpack_from('<If', payload, 80) for payload in self.commands('effect')],
                         [(1, 12), (1, 13)])

    def test_default_skin_can_wait_for_actual_default_weapon_identity(self):
        self.live.channel.game['weapon'] = {}
        self.live.apply(self.config, selections=True)
        self.assertTrue(self.live.pending_startup_skin)
        self.assertEqual(self.commands('skin'), [])
        self.live.channel.game['weapon'] = {'ready': True, 'id': WEAPON, 'skin_id': '6' * 32}
        self.live.sync_effects()
        self.assertEqual(len(self.commands('skin')), 1)
        self.assertFalse(self.live.pending_startup_skin)
        self.assertEqual(self.commands('effect'), [])
        self.live.sync_effects()
        self.assertEqual(len(self.commands('skin')), 1)
        self.live.channel.game['weapon'].update(SKIN)
        self.live.sync_effects()
        self.assertEqual(len(self.commands('effect')), 1)
        self.assertEqual(self.commands('weapon'), [])

    def test_stale_revision_and_other_map_never_receive_effects(self):
        self.live.submitted_revision = self.config['revision'] + 1
        self.live.sync_effects()
        self.live.channel.game['applied_revision'] = self.live.submitted_revision
        self.live.channel.game['map_id'] = 'duel2'
        self.live.sync_effects()
        self.live.channel.game['map_id'] = 'duel1'
        self.live.startup_applying = True
        self.live.sync_effects()
        self.assertEqual(self.commands('effect'), [])

    def test_busy_startup_readback_defers_default_skin_instead_of_failing_startup(self):
        self.live.channel.game['weapon']['skin_id'] = '6' * 32
        with patch.object(self.live.channel, 'state', side_effect=RuntimeError('Game state update is busy')):
            self.live.apply(self.config, selections=True)
        self.assertTrue(self.live.pending_startup_skin)
        self.assertEqual(self.commands('skin'), [])
        self.live.sync_effects()
        self.assertEqual(len(self.commands('skin')), 1)
        self.assertIsNone(self.live.effect_sync_error)


if __name__ == '__main__':
    unittest.main()
