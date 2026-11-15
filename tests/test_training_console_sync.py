"""Control-plane regressions: no game process or existing server required."""
from copy import deepcopy
from pathlib import Path
import struct
import sys
import tempfile
import threading
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from training_control import COMMANDS, CONFIG, pack_configuration, pack_routes
from training_settings_v2 import DEFAULTS, Settings, merge
from training_server import Application, Runtime, describe_game, runtime_text


class FakeChannel:
    def __init__(self, game):
        self.game = game
        self.sent = []

    def state(self):
        return deepcopy(self.game)

    def send(self, kind, payload):
        self.sent.append((kind, payload))
        return {'status': 'applied', 'code': 0, 'sequence': len(self.sent), 'message': 'configuration_applied'}


def runtime(settings, catalog=None):
    value = Runtime.__new__(Runtime)  # Never create the process monitor in these tests.
    value.settings, value.catalog = settings, catalog
    value.lock, value.configuration_lock = threading.RLock(), threading.RLock()
    value.status, value.startup_applying, value.submitted_revision = 'running', False, 0
    value.error = value.active = value.session = value.last_command = None
    value._reset_effect_sync()
    value.channel = FakeChannel({'map_id': 'duel1', 'ready': True, 'applied_revision': 0, 'infinite_ammo': False})
    return value


class ConsoleSyncTests(unittest.TestCase):
    def test_route_inherits_global_without_changing_global_enum(self):
        route = dict(id=1, name='箱后', start=[0, 0, 0], expose=[1, 2, 3], retreat=[0, 0, 0],
                     style='global', enabled=True, weight=1)
        config = merge(DEFAULTS, {'routes': {'duel1': [route]}})
        self.assertEqual(struct.unpack_from('<I', pack_routes(config['routes']['duel1'], 1), 12)[0], 6)
        self.assertEqual(CONFIG.unpack(pack_configuration(config))[-2], 5)
        with self.assertRaises(ValueError):
            merge(config, {'practice': {'style': 'global'}})
        self.assertEqual([COMMANDS[k] for k in ('configure', 'routes', 'weapon', 'skin', 'effect',
                                              'restart_match', 'validate_routes', 'toggle_buy_menu', 'buy_weapon')], list(range(1, 10)))
        self.assertEqual(COMMANDS['save_skin_selection'], 10)

    def test_f6_persists_once_without_command_echo(self):
        with tempfile.TemporaryDirectory() as directory:
            settings = Settings(Path(directory) / 'config.json')
            live = runtime(settings)
            live.sync_native()
            self.assertFalse(Settings(settings.path).read()['infinite_ammo'])
            self.assertEqual(settings.read()['revision'], 1)
            live.sync_native()
            self.assertEqual(settings.read()['revision'], 1)
            self.assertEqual(live.channel.sent, [])

    def test_stale_snapshot_or_other_map_cannot_override_requested_ammo(self):
        with tempfile.TemporaryDirectory() as directory:
            settings = Settings(Path(directory) / 'config.json')
            live = runtime(settings)
            live.submitted_revision = 2
            live.sync_native()
            self.assertTrue(settings.read()['infinite_ammo'])
            live.channel.game['applied_revision'] = 2
            live.channel.game['map_id'] = 'duel2'
            live.sync_native()
            self.assertTrue(settings.read()['infinite_ammo'])
            live.channel.game['map_id'] = 'duel1'
            live.startup_applying = True
            live.sync_native()
            self.assertTrue(settings.read()['infinite_ammo'])
            live.startup_applying = False
            live.sync_native()
            self.assertFalse(settings.read()['infinite_ammo'])

    def test_unrelated_save_preserves_native_f6_change_and_explicit_toggle_wins(self):
        with tempfile.TemporaryDirectory() as directory:
            app = Application(Path(directory), runtime=False)
            app.runtime = runtime(app.settings, app.catalog)
            app.runtime.configuration_lock = app.mutation
            result = app.update({'paused': True}, 0)
            self.assertFalse(result['config']['infinite_ammo'])
            flags = struct.unpack_from('<I', app.runtime.channel.sent[0][1], 12)[0]
            self.assertFalse(flags & 4)
            self.assertTrue(flags & 8)
            app.runtime.channel.game['applied_revision'] = result['config']['revision']
            requested = app.update({'infinite_ammo': True}, result['config']['revision'])
            app.runtime.sync_native()  # The old snapshot still has the previous revision.
            self.assertTrue(app.settings.read()['infinite_ammo'])
            self.assertTrue(requested['config']['infinite_ammo'])

    def test_startup_all_skins_are_saved_before_current_weapon_switch(self):
        class Catalog:
            @staticmethod
            def skin_payload(weapon, skin, level, chroma):
                return bytes.fromhex(weapon + skin + level + chroma)
        with tempfile.TemporaryDirectory() as directory:
            settings = Settings(Path(directory) / 'config.json')
            weapons = ['1' * 32, '2' * 32]
            config = settings.update({'weapon_id': weapons[0], 'skins': {
                key: dict(skin_id='3' * 32, level_id='4' * 32, chroma_id='5' * 32) for key in weapons}}, 0)
            live = runtime(settings, Catalog())
            live.apply(config, selections=True)
            commands = live.channel.sent
            self.assertEqual([name for name, _ in commands],
                             ['configure', 'routes', 'save_skin_selection', 'save_skin_selection', 'weapon', 'skin'])
            self.assertEqual([payload[:16].hex() for name, payload in commands if name == 'save_skin_selection'], weapons)
            self.assertTrue(all(len(payload) == 64 for name, payload in commands if name in ('save_skin_selection', 'skin')))

    def test_startup_waits_for_ready_and_preserves_early_f6(self):
        with tempfile.TemporaryDirectory() as directory:
            settings = Settings(Path(directory) / 'config.json')
            live = runtime(settings)
            live.startup_applying = True
            live.launch_config = settings.read()
            live.channel.game['ready'] = False
            class Wake:
                count = 0
                def wait(self, _):
                    self.count += 1
                    if self.count == 2:
                        self.assert_empty()
                        live.channel.game['ready'] = True
                    return False
                def assert_empty(self):
                    if live.channel.sent:
                        raise AssertionError('Commands reached the game before ready')
            live.closed = Wake()
            live._apply_saved()
            self.assertEqual(live.closed.count, 2)
            self.assertIsNone(live.error)
            self.assertFalse(live.startup_applying)
            self.assertFalse(settings.read()['infinite_ammo'])
            flags = struct.unpack_from('<I', live.channel.sent[0][1], 12)[0]
            self.assertFalse(flags & 4)

    def test_ui_requests_during_loading_are_saved_without_publishing_commands(self):
        with tempfile.TemporaryDirectory() as directory:
            settings = Settings(Path(directory) / 'config.json')
            live = runtime(settings)
            live.startup_applying = True
            result = live.apply(settings.read())
            self.assertEqual(result['status'], 'saved')
            self.assertEqual(live.command('weapon', bytes(16))['status'], 'saved')
            self.assertEqual(live.channel.sent, [])

    def test_readable_labels_preserve_native_diagnostics(self):
        game = describe_game({'stage': 'training_session_waiting_original_equip', 'error': None,
                              'players': [{'phase': 'no_saved_routes_for_this_map'}],
                              'routes': [{'message': 'following_original_navigation_path'}],
                              'effects': [{'description': 'original_champion_level4_required', 'numeric': True,
                                           'value': 12, 'minimum': 0, 'maximum': 25}]})
        self.assertEqual(game['stage_text'], '等待武器装备完成')
        self.assertEqual(game['players'][0]['phase_text'], '当前地图还没有保存路线')
        self.assertEqual(game['effects'][0]['maximum'], 25)
        code = 'training_session_original_hud_lifecycle_failed'
        self.assertIn('处理失败', runtime_text(code))
        failure = describe_game({'stage': code, 'error': code})
        self.assertEqual(failure['stage'], code)
        self.assertEqual(failure['error'], code)
        self.assertNotIn(code, failure['error_text'])


if __name__ == '__main__':
    unittest.main()
