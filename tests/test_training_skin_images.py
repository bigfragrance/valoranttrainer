import hashlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from training_skin_images import SkinPreviews

WEAPON, SKIN, LEVEL, UPGRADE, DEFAULT, RED = (f'{n:032x}' for n in range(1, 7))
BASE_ICON = '/Game/UI/Original/Skin_Base'
UPGRADE_ICON = '/Game/UI/Original/Skin_Upgrade'
RED_ICON = '/Game/UI/Original/Skin_Red'


class Decoder:
    def __init__(self, missing=()):
        self.calls = []
        self.missing = set(missing)

    def decode(self, package, cache):
        self.calls.append(package)
        if package in self.missing:
            raise FileNotFoundError('missing fixture texture')
        color = hashlib.sha256(package.encode()).digest()
        image = Image.new('RGBA', (16, 5), (*color[:3], 180))
        stream = io.BytesIO()
        image.save(stream, format='PNG')
        data = stream.getvalue()
        return data, {'package': package, 'width': 16, 'height': 5,
                      'png_sha256': hashlib.sha256(data).hexdigest()}


class PreviewTests(unittest.TestCase):
    def setUp(self):
        self.base = ROOT / '.local/skin-preview-tests'
        self.base.mkdir(parents=True, exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(dir=self.base)
        self.root = Path(self.temporary.name).resolve()
        assert self.root.parent == self.base.resolve()
        self.row = {'guid': SKIN, 'weapon_guid': WEAPON, 'icon': BASE_ICON,
                    'default_level_guid': LEVEL, 'default_chroma_guid': DEFAULT,
                    'levels': [{'guid': LEVEL, 'icon': BASE_ICON}, {'guid': UPGRADE, 'icon': UPGRADE_ICON}],
                    'chromas': [{'guid': DEFAULT, 'is_default': True, 'icon': BASE_ICON},
                                {'guid': RED, 'icon': RED_ICON}]}
        (self.root / 'reports').mkdir()
        self.assets = []
        self.write_report()

    def tearDown(self):
        assert self.root.parent == self.base.resolve()
        self.temporary.cleanup()

    def write_report(self):
        (self.root / 'reports/all-skins.json').write_text(json.dumps({'inventory_sha256': 'fixture-v1', 'skins': [self.row], 'assets': self.assets}), encoding='utf-8')

    def test_exact_chroma_image_and_cache_have_the_selected_identity(self):
        decoder = Decoder()
        store = SkinPreviews(self.root, decoder=decoder)
        original = store.describe(WEAPON, SKIN, LEVEL, RED)
        again = store.describe(WEAPON, SKIN, LEVEL, RED)
        self.assertEqual(original, again)
        self.assertTrue(original['exact_chroma'])
        self.assertFalse(original['fallback'])
        self.assertEqual(original['chroma_id'], RED)
        self.assertEqual(decoder.calls, [RED_ICON])
        reply = store.handle_request(original['image_url'])
        self.assertEqual(reply.content_type, 'image/png')
        self.assertTrue(reply.payload.startswith(b'\x89PNG\r\n\x1a\n'))
        self.assertFalse(list(store.cache.glob('*.uasset')))

    def test_distinct_authored_level_picture_is_used_for_default_chroma(self):
        decoder = Decoder()
        result = SkinPreviews(self.root, decoder=decoder).describe(WEAPON, SKIN, UPGRADE, DEFAULT)
        self.assertTrue(result['available'])
        self.assertEqual(result['kind'], 'level')
        self.assertEqual(decoder.calls, [UPGRADE_ICON])

    def test_full_render_is_used_instead_of_an_inherited_no_image_placeholder(self):
        placeholder = '/Game/UI/Store/NoImage_Placeholder'
        self.row['icon'] = placeholder
        self.row['levels'][0]['icon'] = placeholder
        self.row['chromas'][0]['icon'] = placeholder
        self.assets = [{'guid': DEFAULT, 'ui': {'icon': placeholder, 'full_render': BASE_ICON}}]
        self.write_report()
        decoder = Decoder()
        result = SkinPreviews(self.root, decoder=decoder).describe(WEAPON, SKIN, LEVEL, DEFAULT)
        self.assertTrue(result['available'])
        self.assertEqual(result['image_field'], 'FullRender')
        self.assertEqual(decoder.calls, [BASE_ICON])

    def test_original_no_image_placeholder_does_not_count_as_a_skin_preview(self):
        self.row['icon'] = '/Game/UI/Store/NoImage_Placeholder'
        for item in self.row['levels']+self.row['chromas']:
            item['icon'] = self.row['icon']
        self.write_report()
        decoder = Decoder()
        result = SkinPreviews(self.root, decoder=decoder).describe(WEAPON, SKIN, LEVEL, DEFAULT)
        self.assertFalse(result['available'])
        self.assertEqual(decoder.calls, [])

    def test_missing_chroma_discloses_skin_picture_fallback(self):
        result = SkinPreviews(self.root, decoder=Decoder([RED_ICON])).describe(WEAPON, SKIN, LEVEL, RED)
        self.assertTrue(result['available'])
        self.assertFalse(result['exact_chroma'])
        self.assertTrue(result['fallback'])
        self.assertIn('此炫彩暂无单独预览', result['note'])

    def test_missing_all_images_is_an_explicit_unavailable_state(self):
        store = SkinPreviews(self.root, decoder=Decoder([BASE_ICON, RED_ICON, UPGRADE_ICON]))
        result = store.describe(WEAPON, SKIN, LEVEL, RED)
        self.assertFalse(result['available'])
        self.assertIsNone(result['image_url'])
        reply = store.handle_request(f'/api/skin-preview?weapon={WEAPON}&skin={SKIN}&level={LEVEL}&chroma={RED}&format=png')
        self.assertEqual(reply.status, 404)
        self.assertEqual(json.loads(reply.payload)['error'], result['note'])

    def test_foreign_guid_and_arbitrary_asset_path_never_reach_decoder(self):
        decoder = Decoder()
        store = SkinPreviews(self.root, decoder=decoder)
        for args in [('f'*32, SKIN, LEVEL, RED), (WEAPON, SKIN, 'f'*32, RED), (WEAPON, SKIN, LEVEL, 'f'*32), ('../../file', SKIN, LEVEL, RED)]:
            with self.assertRaises(ValueError):
                store.describe(*args)
        with self.assertRaises(ValueError):
            store.handle_request('/api/skin-preview?package=/Game/UI/Anything')
        with self.assertRaises(ValueError):
            store.handle_request('/api/skin-previews/../../config.json')
        self.assertEqual(decoder.calls, [])

    def test_authored_null_relationship_does_not_break_other_valid_previews(self):
        self.row['levels'].append({'guid': None, 'icon': None})
        self.row['chromas'].insert(0, {'guid': None, 'icon': None})
        self.write_report()
        result = SkinPreviews(self.root, decoder=Decoder()).describe(WEAPON, SKIN, LEVEL, RED)
        self.assertTrue(result['available'])
        self.assertEqual(result['chroma_id'], RED)

    def test_cache_eviction_is_limited_to_its_own_hash_files(self):
        decoder = Decoder()
        png, _ = decoder.decode(BASE_ICON, None)
        store = SkinPreviews(self.root, decoder=decoder, cache_bytes=len(png)+10)
        first = store.describe(WEAPON, SKIN, LEVEL, DEFAULT)
        unrelated = store.cache / 'keep-user-file.txt'
        unrelated.write_text('preserve')
        store.describe(WEAPON, SKIN, LEVEL, RED)
        self.assertFalse((store.cache / (first['key']+'.png')).exists())
        self.assertEqual(unrelated.read_text(), 'preserve')
        self.assertLessEqual(sum(p.stat().st_size for p in store.cache.glob('*.png')), len(png)+10)


if __name__ == '__main__':
    unittest.main()
