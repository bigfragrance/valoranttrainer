from copy import deepcopy
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from training_settings_v2 import DEFAULTS, validate
from training_control import pack_practice_routes, initial_payload


class HoldPracticeTests(unittest.TestCase):
    def enemy(self, **patch):
        return dict(route_id=1, style='wide_crouch', fire=True,
                    delay_min=.5, delay_max=1.5, **patch)

    def test_plan_keeps_initial_layout_and_uses_global_cooldown(self):
        config = validate(deepcopy(DEFAULTS))
        config['practice']['minimum_interval'] = 2.25
        wire = pack_practice_routes(config)
        self.assertEqual(len(wire), 8 + 128 * 52)
        self.assertEqual(struct.unpack_from('<4If', wire, 8 + 64 * 52),
                         (0x504c414e, 1, 0, 2, 2.25))
        self.assertEqual(len(initial_payload(config, {})), 7844)

    def test_wave_slots_are_unique_and_restart_from_zero_each_wave(self):
        config = deepcopy(DEFAULTS)
        config['practice'].update(mode='waves', waves={'duel1': [
            [self.enemy(), self.enemy()], [self.enemy()]]})
        wire = pack_practice_routes(validate(config))
        offset = 8 + 64 * 52 + 24
        entries = [struct.unpack_from('<5I2f', wire, offset + n * 28) for n in range(3)]
        self.assertEqual([entry[:2] for entry in entries], [(0, 0), (0, 1), (1, 0)])
        self.assertEqual(entries[0][2:], (1, 7, 1, .5, 1.5))

    def test_old_preferences_receive_practice_and_point_defaults(self):
        config = deepcopy(DEFAULTS)
        for key in ('mode', 'minimum_interval', 'waves'):
            config['practice'].pop(key)
        for route in config['routes']['duel1']:
            for key in ('fire', 'delay_min', 'delay_max'):
                route.pop(key)
        migrated = validate(config)
        self.assertEqual(migrated['practice']['mode'], 'continuous')
        self.assertEqual(migrated['routes']['duel1'][0]['delay_max'], .4)

    def test_invalid_enemy_range_reference_or_capacity_is_rejected(self):
        for enemy in [dict(self.enemy(), delay_min=2), dict(self.enemy(), route_id=999),
                      dict(self.enemy(), fire=1)]:
            config = deepcopy(DEFAULTS)
            config['practice']['waves'] = {'duel1': [[enemy]]}
            with self.assertRaises(ValueError):
                validate(config)
        config = deepcopy(DEFAULTS)
        config['total_players'] = 2
        config['practice']['waves'] = {'duel1': [[self.enemy(), self.enemy()]]}
        with self.assertRaises(ValueError):
            validate(config)


if __name__ == '__main__':
    unittest.main()
