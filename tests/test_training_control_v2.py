from copy import deepcopy
import json
from pathlib import Path
import struct
import sys
import tempfile
import threading
import time
import unittest
from urllib.error import HTTPError
from urllib.request import Request, urlopen

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from training_control import BYTES, STATE_CAPACITY, MAGIC, VERSION, Channel, CONFIG, ROUTE, initial_payload, pack_configuration, guid_words
from training_settings_v2 import DEFAULTS, Settings, RevisionConflict, merge, uv_to_world, world_to_uv, MAX_UNIQUE_PLAYERS
from training_server import Application, make_server, Catalog


class WireTests(unittest.TestCase):
    def memory(self):
        memory = bytearray(BYTES)
        struct.pack_into('<Q3I', memory, 0, MAGIC, VERSION, BYTES, 42)
        return memory

    def test_wire_size_and_original_guid_word_order(self):
        self.assertEqual(CONFIG.size, 72)
        self.assertEqual(ROUTE.size, 52)
        self.assertEqual(len(pack_configuration(DEFAULTS)), 72)
        self.assertEqual(guid_words('9c82e19d-4575-0200-1a81-3eacf00cf872'), (0x9c82e19d, 0x45750200, 0x1a813eac, 0xf00cf872))
        data = initial_payload(DEFAULTS, {'spawn_anchors': [{'raw_actor_location_cm': [1, 2, 3]}]})
        self.assertEqual(len(data), 7844)
        self.assertEqual(struct.unpack_from('<I', data, 12)[0], len(data))
        self.assertEqual(struct.unpack_from('<I3f', data, 16 + 72 + 8 + 128 * 52 + 64), (1, 1, 2, 3))

    def test_wrong_pid_or_version_cannot_attach(self):
        memory = self.memory()
        with self.assertRaisesRegex(RuntimeError, 'identity/version'):
            Channel(43, memory=memory)
        struct.pack_into('<I', memory, 8, VERSION + 1)
        with self.assertRaises(RuntimeError):
            Channel(42, memory=memory)

    def test_state_sequence_lock_and_json_bounds(self):
        memory = self.memory()
        channel = Channel(42, memory=memory)
        payload = b'{"players":[{"index":0}]}'
        memory[32768:32768+len(payload)] = payload
        struct.pack_into('<2I', memory, 56, 2, len(payload))
        self.assertEqual(channel.state()['players'][0]['index'], 0)
        struct.pack_into('<I', memory, 56, 3)
        with self.assertRaisesRegex(RuntimeError, 'busy'):
            channel.state()
        struct.pack_into('<2I', memory, 56, 4, STATE_CAPACITY)
        with self.assertRaisesRegex(RuntimeError, 'length'):
            channel.state()

    def test_timeout_never_overwrites_unconsumed_command(self):
        memory = self.memory()
        channel = Channel(42, memory=memory)
        result = channel.send('weapon', b'x' * 16, timeout=.01)
        self.assertEqual(result['status'], 'pending')
        self.assertFalse(result['acknowledged'])
        command = bytes(memory[32:48])
        with self.assertRaisesRegex(RuntimeError, 'still pending'):
            channel.send('weapon', b'y' * 16, timeout=.01)
        self.assertEqual(bytes(memory[32:48]), command)

    def test_acknowledged_command_preserves_error(self):
        memory = self.memory()
        channel = Channel(42, memory=memory)
        def game():
            while not struct.unpack_from('<I', memory, 32)[0]:
                time.sleep(.001)
            seq = struct.unpack_from('<I', memory, 32)[0]
            memory[80:94] = b'skin_mismatch\0'
            struct.pack_into('<2I', memory, 48, seq, 2)
        thread = threading.Thread(target=game)
        thread.start()
        result = channel.send('skin', bytes(64))
        thread.join()
        self.assertEqual(result['status'], 'invalid')
        self.assertEqual(result['message'], 'skin_mismatch')
        self.assertTrue(result['acknowledged'])

    def test_native_pending_ack_is_distinct_from_an_unconsumed_command(self):
        memory = self.memory()
        channel = Channel(42, memory=memory)
        def game():
            while not struct.unpack_from('<I', memory, 32)[0]:
                time.sleep(.001)
            sequence = struct.unpack_from('<I', memory, 32)[0]
            struct.pack_into('<2I', memory, 48, sequence, 1)
        thread = threading.Thread(target=game)
        thread.start()
        result = channel.send('skin', bytes(64))
        thread.join()
        self.assertEqual(result['status'], 'pending')
        self.assertTrue(result['acknowledged'])

    def test_close_waits_for_pending_command_and_rejects_later_reads(self):
        memory = self.memory()
        channel = Channel(42, memory=memory)
        result, closed = [], threading.Event()
        sender = threading.Thread(target=lambda: result.append(channel.send('weapon', bytes(16), timeout=1)))
        sender.start()
        deadline = time.monotonic() + 1
        while not struct.unpack_from('<I', memory, 32)[0] and time.monotonic() < deadline:
            time.sleep(.001)
        self.assertNotEqual(struct.unpack_from('<I', memory, 32)[0], 0)
        closer = threading.Thread(target=lambda: (channel.close(), closed.set()))
        closer.start()
        self.assertFalse(closed.wait(.03))
        sequence = struct.unpack_from('<I', memory, 32)[0]
        struct.pack_into('<2I', memory, 48, sequence, 1)
        sender.join(2)
        closer.join(2)
        self.assertTrue(closed.is_set())
        self.assertEqual(result[0]['sequence'], sequence)
        with self.assertRaisesRegex(RuntimeError, 'closed'):
            channel.state()
        with self.assertRaisesRegex(RuntimeError, 'closed'):
            channel.send('weapon', bytes(16))


class SettingsTests(unittest.TestCase):
    def test_population_limit_matches_the_playable_resource_catalog(self):
        heroes = Catalog(Path(__file__).resolve().parents[1]).heroes()
        self.assertEqual(len(heroes), MAX_UNIQUE_PLAYERS)
        self.assertEqual(len({hero['id'] for hero in heroes}), MAX_UNIQUE_PLAYERS)

    def test_population_cannot_exceed_unique_hero_pool(self):
        self.assertEqual(merge(DEFAULTS, {'total_players': MAX_UNIQUE_PLAYERS})['total_players'], MAX_UNIQUE_PLAYERS)
        with self.assertRaises(ValueError):
            merge(DEFAULTS, {'total_players': MAX_UNIQUE_PLAYERS + 1})

    def test_old_32_player_settings_keep_loadout_and_routes(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'config.json'
            value = deepcopy(DEFAULTS)
            value.update(total_players=32, revision=12, mode='deathmatch', map_id='ascent', infinite_ammo=False)
            path.write_text(json.dumps(value), encoding='utf-8')
            restored = Settings(path).read()
            self.assertEqual(restored, {**value, 'total_players': MAX_UNIQUE_PLAYERS})

    def test_revisions_atomic_persistence_and_no_lost_update(self):
        with tempfile.TemporaryDirectory() as directory:
            settings = Settings(Path(directory) / 'config.json')
            result = settings.update({'practice': {'return_fire': True}}, 0)
            self.assertEqual(result['revision'], 1)
            self.assertTrue(result['practice']['return_fire'])
            self.assertEqual(result['practice']['peeks_per_minute'], 20)
            with self.assertRaises(RevisionConflict):
                settings.update({'paused': True}, 0)
            self.assertEqual(Settings(settings.path).read(), result)
            self.assertFalse(settings.path.with_suffix('.json.tmp').exists())

    def test_invalid_values_cannot_reach_native(self):
        for patch in ({'total_players': True}, {'total_players': 33}, {'invincible': 1},
                      {'bots': {'accuracy': float('nan')}}, {'unknown': 1},
                      {'effects': {'champion_glow': {'mode': True}}},
                      {'routes': {'duel1': [{'id': 1}]}}):
            with self.subTest(patch=patch), self.assertRaises(ValueError):
                merge(DEFAULTS, patch)

    def test_original_cross_axis_and_affine_maps_roundtrip(self):
        transforms = [dict(x_multiplier=.0001, x_scalar_to_add=.5, y_multiplier=-.0002, y_scalar_to_add=.4),
                      dict(type='affine_2x3', matrix=[[0, .0001, .5], [-.0002, 0, .4]])]
        for transform in transforms:
            uv = world_to_uv(transform, [123, -456, 70])
            point = uv_to_world(transform, uv, 70)
            for actual, expected in zip(point, [123, -456, 70]):
                self.assertAlmostEqual(actual, expected)


class HttpTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        (self.root / 'reports').mkdir()
        self.app = Application(self.root, runtime=False)
        self.server = make_server(0, app=self.app)
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        self.base = f'http://127.0.0.1:{self.server.server_port}'

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join()
        self.temporary.cleanup()

    def request(self, path, body=None, method='POST', headers=None):
        request = Request(self.base + path, data=json.dumps(body).encode() if body is not None else None,
                          method=method if body is not None else 'GET', headers={'Content-Type': 'application/json', **(headers or {})})
        with urlopen(request, timeout=3) as response:
            return response.status, json.load(response)

    def test_http_settings_and_revision_conflict(self):
        self.assertEqual(self.request('/api/state')[1]['config']['revision'], 0)
        self.assertEqual(self.request('/api/config', {'revision': 0, 'patch': {'paused': True}}, 'PATCH')[1]['config']['revision'], 1)
        with self.assertRaises(HTTPError) as caught:
            self.request('/api/config', {'revision': 0, 'patch': {'paused': False}}, 'PATCH')
        self.assertEqual(caught.exception.code, 409)

    def test_unknown_map_and_cross_origin_request_are_rejected(self):
        with self.assertRaises(HTTPError) as caught:
            self.request('/api/config', {'revision': 0, 'patch': {'map_id': 'madeup'}}, 'PATCH')
        self.assertEqual(caught.exception.code, 400)
        with self.assertRaises(HTTPError) as caught:
            self.request('/api/config', {'revision': 0, 'patch': {'paused': True}}, 'PATCH', {'Origin': 'https://example.com'})
        self.assertEqual(caught.exception.code, 400)
        self.assertEqual(self.app.settings.read()['revision'], 0)


if __name__ == '__main__':
    unittest.main()
