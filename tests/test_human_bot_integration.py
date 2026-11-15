"""Product preference/API adapters, using own files and no game processes."""
import copy
import json
from shutil import copyfile
import struct
from pathlib import Path
import sys
import tempfile
import threading
import unittest
from urllib.error import HTTPError
from urllib.request import Request, build_opener, ProxyHandler

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import human_bot_integration as integration
import humanlike_settings as human
import human_bot_roster as roster
import human_bot_pack as wire
import local_server_controller as local
from human_bot_delivery import legacy_transport_view
from training_heroes import hero_profile, selected_hero
from training_control import initial_payload
from training_server import Application, make_server
from training_settings_v2 import DEFAULTS, MAX_UNIQUE_PLAYERS, RevisionConflict


class HumanIntegrationTests(unittest.TestCase):
    def test_legacy_read_and_preview_do_not_rewrite_preferences(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'settings.json'
            original = json.dumps(DEFAULTS).encode()
            path.write_bytes(original)
            store = integration.HumanSettings(path)
            config = store.read()
            self.assertEqual(config['schema_version'], 3)
            self.assertFalse(config['human_bots']['enabled'])
            self.assertEqual(integration.preview(config, 1)['source'], 'configuration_preview')
            store.propose({'paused': True}, 0)
            self.assertEqual(path.read_bytes(), original)
            self.assertEqual(store.read()['revision'], 0)

    def test_population_compatibility_applies_only_to_old_persisted_files(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'settings.json'
            path.write_text(json.dumps({**DEFAULTS, 'total_players': 32}), encoding='utf-8')
            store = integration.HumanSettings(path)
            self.assertEqual(store.read()['total_players'], MAX_UNIQUE_PLAYERS)
            with self.assertRaises(ValueError):
                store.propose({'total_players': 32}, 0)
            invalid = store.read()
            invalid['total_players'] = 32
            path.write_text(json.dumps(invalid), encoding='utf-8')
            with self.assertRaises(ValueError):
                integration.HumanSettings(path)

    def test_disabled_legacy_controls_do_not_change_new_capabilities(self):
        with tempfile.TemporaryDirectory() as directory:
            store = integration.HumanSettings(Path(directory) / 'settings.json')
            before = store.read()
            proposal = store.propose({'bots': {'reaction_ms': 321}}, 0)
            self.assertEqual(proposal['human_bots'], before['human_bots'])
            self.assertEqual(proposal['bots']['reaction_ms'], 321)
            for patch in ({'bots': None}, {'bots': {'unknown': 1}},
                          {'mode': 'deathmatch', 'bots': {'reaction_ms': 321}, 'human_bots': {'enabled': True}}):
                with self.subTest(patch=patch), self.assertRaises(ValueError):
                    store.propose(patch, 0)
            with self.assertRaises(RevisionConflict):
                store.propose({'paused': True}, True)

    def test_practice_keeps_legacy_controls_when_ffa_preference_is_enabled(self):
        with tempfile.TemporaryDirectory() as directory:
            store = integration.HumanSettings(Path(directory) / 'settings.json')
            before = store.propose({'mode': 'practice', 'human_bots': {'enabled': True}}, 0)
            store.update({'mode': 'practice', 'human_bots': {'enabled': True}}, 0)
            proposal = store.propose({'bots': {'reaction_ms': 321}}, 1)
            self.assertEqual(proposal['human_bots'], before['human_bots'])
            self.assertEqual(proposal['bots']['reaction_ms'], 321)
            with self.assertRaises(ValueError):
                store.propose({'mode': 'deathmatch', 'bots': {'reaction_ms': 321}}, 1)

    def test_initial_and_live_bundle_identity_are_deterministic(self):
        with tempfile.TemporaryDirectory() as directory:
            store = integration.HumanSettings(Path(directory) / 'settings.json')
            config = store.read()
            first = integration.configuration_bundle(config)
            self.assertEqual(first, integration.configuration_bundle(copy.deepcopy(config)))
            changed = store.propose({'human_bots': {'global_overrides': {'aim.primary_speed_deg_s': 321}}}, 0)
            self.assertNotEqual(first.checksum, integration.configuration_bundle(changed).checksum)
            self.assertIsNone(integration.configuration_bundle(copy.deepcopy(DEFAULTS)))

    def test_sidefile_uses_original_initial_writer_and_cannot_replace_it(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            # Owned root with the same pinned catalog/map inputs as production.
            product_root = Path(integration.__file__).resolve().parents[1]
            for relative in ('profiles/playable-heroes.json', 'reports/training-maps.json'):
                target = root / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                copyfile(product_root / relative, target)
            store = integration.HumanSettings(root / 'settings.json')
            config = store.read()
            run = root / 'pair-own-test'
            def original_writer(folder, legacy, map_info, *, root):
                self.assertEqual(legacy['schema_version'], 2)
                folder.mkdir(parents=True, exist_ok=True)
                return local._write_legacy_initial(folder, legacy, map_info, root=root)
            result = integration.write_initial(run, config, {}, original_write_initial=original_writer, root=root)
            self.assertEqual((run / 'session-initial.bin').stat().st_size, 7844)
            self.assertEqual((run / 'session-human-bots.bin').read_bytes(), integration.configuration_bundle(config).initial())
            self.assertEqual(result['human_bots']['status'], 'initial_sidefile_written_consumer_pending')
            initial = (run / 'session-initial.bin').read_bytes()
            self.assertEqual(initial, initial_payload(legacy_transport_view(config), {}))
            self.assertEqual(struct.unpack_from('<Q2I', initial), (0x32304c5443525456, 2, 7844))
            raw = (run / roster.NAME).read_bytes()
            header = roster.HEADER.unpack_from(raw)
            entries = config['human_bots']['bot_roster']
            self.assertEqual(header[:5], (roster.MAGIC, 1, 128, 32, len(raw)))
            self.assertEqual(header[5:9], (config['revision'], config['seed'], config['total_players'], len(entries)))
            self.assertEqual(header[15].hex(), hero_profile(root)['sha256'])
            self.assertEqual(header[16:20], (wire.fnv(initial), wire.fnv(raw[128:]), 0, 0))
            self.assertEqual(len(raw), 128 + 32 * len(entries))
            hero_selection = (run / 'session-hero-selection.bin').read_bytes()
            self.assertEqual(len(hero_selection), 72)
            self.assertEqual(struct.pack('<4I', *header[11:15]), hero_selection[24:40])
            for slot, entry in enumerate(entries, 1):
                hero, _ = selected_hero(entry['hero_id'], root=root)
                self.assertEqual(roster.ROW.unpack_from(raw, 128 + (slot - 1) * 32),
                                 (slot, hero['index'], int(entry['id'], 16), *hero['guid_words']))
            originals = {path.name: path.read_bytes() for path in run.iterdir() if path.is_file()}
            with self.assertRaises(FileExistsError):
                integration.write_initial(run, config, {}, original_write_initial=original_writer, root=root)
            self.assertEqual({path.name: path.read_bytes() for path in run.iterdir() if path.is_file()}, originals)


class HumanApiTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.app = Application(Path(self.temporary.name), runtime=False, backend='local-server-candidate')
        self.server = make_server(0, app=self.app)
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        self.base = f'http://127.0.0.1:{self.server.server_port}'
        self.opener = build_opener(ProxyHandler({}))

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join()
        self.temporary.cleanup()

    def request(self, path, body=None):
        request = Request(self.base + path, data=json.dumps(body).encode() if body is not None else None,
                          headers={'Content-Type': 'application/json'})
        with self.opener.open(request, timeout=5) as response:
            return json.load(response)

    def test_real_spec_and_preview_are_not_native_execution_claims(self):
        spec = self.request('/api/human-bots/spec')
        self.assertEqual(spec['spec']['schema_digest_sha256'], human.SCHEMA_DIGEST.hex())
        self.assertEqual(spec['spec']['param_count'], human.PARAM_COUNT)
        original = self.app.settings.path.read_bytes()
        preview = self.request('/api/human-bots/preview?index=1')
        self.assertEqual(len(preview['values']), human.PARAM_COUNT)
        self.assertFalse(preview['consumer_effective'])
        self.assertFalse(preview['behavior_execution_verified'])
        draft = self.app.settings.read()['human_bots']
        draft['global_overrides']['aim.primary_speed_deg_s'] = 333
        draft['distribution'].pop('aim.primary_speed_deg_s', None)
        entry = draft['bot_roster'][0]
        # A saved entry snapshot deliberately outranks a new global override.
        inherited = self.request('/api/human-bots/preview', {'entry_id': entry['id'], 'human_bots': draft,
                                                           'keys': ['aim.primary_speed_deg_s']})
        self.assertEqual(inherited['values'], {'aim.primary_speed_deg_s': entry['overrides']['aim.primary_speed_deg_s']})
        self.assertEqual(inherited['sources']['aim.primary_speed_deg_s'], '名单个人覆盖')
        entry['overrides']['aim.primary_speed_deg_s'] = 333
        response = self.request('/api/human-bots/preview', {'entry_id': entry['id'], 'human_bots': draft,
                                                          'keys': ['aim.primary_speed_deg_s']})
        self.assertEqual(response['values'], {'aim.primary_speed_deg_s': 333})
        self.assertEqual((response['entry_id'], response['wire_slot']), (entry['id'], 1))
        self.assertEqual(self.app.settings.path.read_bytes(), original)

    def test_preview_rejects_human_zero_unknown_keys_and_invalid_drafts(self):
        for path in ('/api/human-bots/preview?index=0', '/api/human-bots/preview?index=01',
                     '/api/human-bots/preview?index=1&index=2'):
            with self.subTest(path=path), self.assertRaises(HTTPError) as caught:
                self.request(path)
            self.assertEqual(caught.exception.code, 400)
        for body in ({'roster_index': True}, {'roster_index': 1, 'seed': 0},
                     {'roster_index': 1, 'keys': ['unknown']}, {'roster_index': 1, 'unknown': 0}):
            with self.subTest(body=body), self.assertRaises(HTTPError):
                self.request('/api/human-bots/preview', body)
        self.assertEqual(self.app.settings.read()['revision'], 0)


if __name__ == '__main__':
    unittest.main()
