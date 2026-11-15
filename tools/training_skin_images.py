"""On-demand original 2D skin images; installed containers remain read-only.

Only GUID selections in the captured catalog are accepted. No mesh/material
rendering, external image service or default-weapon substitution is involved.
Decoded PNGs and their small provenance records are the only persistent cache.
"""
from __future__ import annotations

from dataclasses import dataclass
from collections import OrderedDict
import hashlib
import io
import json
from pathlib import Path
import re
import struct
import sys
import tempfile
import threading
import time
from urllib.parse import parse_qs, urlsplit

ROOT = Path(__file__).resolve().parents[1]
MAX_CACHE_BYTES = 48 * 1024 * 1024
MAX_TEXTURE_BYTES = 32 * 1024 * 1024
MAX_BULK_BYTES = 64 * 1024 * 1024
GUID = re.compile(r'[0-9a-f]{32}')
KEY = re.compile(r'[0-9a-f]{64}')
ALGORITHM = 'original-full-render-mip-v2-1024'


def _guid(value, *, optional=False):
    if optional and not value:
        return None
    if not isinstance(value, str):
        raise ValueError('无效的外观编号')
    value = value.replace('-', '').lower()
    if not GUID.fullmatch(value):
        raise ValueError('无效的外观编号')
    return value


def _package(value):
    if not isinstance(value, str) or not value.startswith('/Game/') or any(x in value for x in ('\\', '..', '?', '\r', '\n')):
        return None
    # Captured DisplayIcon references are package paths. Accept the equivalent
    # object path only when its object name is the package's own leaf.
    leaf = value.rsplit('/', 1)[-1]
    if '.' in leaf:
        left, right = leaf.split('.', 1)
        if left != right:
            return None
        value = value.rsplit('.', 1)[0]
    if 'placeholder' in value.rsplit('/', 1)[-1].casefold():
        return None
    return value


@dataclass(frozen=True)
class PreviewResponse:
    payload: bytes
    content_type: str
    status: int = 200
    cache_control: str = 'no-store'


class OriginalTextureArchive:
    """Reuse the project's verified Zen/mipmap/IoStore readers lazily."""
    def __init__(self, root, packages):
        self.root = Path(root).resolve()
        project = json.loads((self.root / 'project.json').read_text(encoding='utf-8'))
        self.paks = Path(project['source_resource_directory']).resolve()
        self.tocs = iter(sorted(self.paks.glob('*.utoc')))
        self.packages = {self.filename(p).casefold() for p in packages}
        self.sources = {}
        self.exhausted = False
        self.stores = []
        # No existing extraction directory is modified. All source streams
        # stay rb; the one short-lived Zen header file is on the project drive.
        sys.path.insert(0, str(self.root / 'work/extract'))
        from analyze_all_skins import CachedStore
        from zen import Zen
        from textures import mipmaps, dds_bytes
        self.Store, self.Zen, self.mipmaps, self.dds_bytes = CachedStore, Zen, mipmaps, dds_bytes

    @staticmethod
    def filename(package, suffix='.uasset'):
        return '../../../ShooterGame/Content/' + package.removeprefix('/Game/') + suffix

    def _source(self, package):
        name = self.filename(package).casefold()
        while name not in self.sources and not self.exhausted:
            try:
                path = next(self.tocs)
            except StopIteration:
                self.exhausted = True
                break
            store = self.Store(path)
            relevant = self.packages.intersection(store.case_paths)
            if relevant:
                self.stores.append(store)
                toc_hash = hashlib.sha256(store.data).hexdigest()
                for item in relevant:
                    if item in self.sources:
                        raise ValueError('原二维图片资源有重复容器记录')
                    self.sources[item] = store, toc_hash
        if name not in self.sources:
            raise FileNotFoundError('当前资源中没有这张原二维图片')
        return self.sources[name]

    def decode(self, package, cache):
        from PIL import Image
        store, toc_hash = self._source(package)
        name = self.filename(package)
        actual = store.case_paths[name.casefold()]
        size = store.ranges[store.files[actual]][1]
        if not 0 < size <= MAX_TEXTURE_BYTES:
            raise ValueError('原二维图片资源大小超出范围')
        raw = store.read(actual)
        # Zen's established parser takes a Path. Its header is deleted after
        # parsing; bulk data is never copied to disk.
        with tempfile.NamedTemporaryFile(dir=cache, prefix='decode-', suffix='.uasset', delete=False) as stream:
            header = Path(stream.name)
            stream.write(raw)
        try:
            z = self.Zen(header)
        finally:
            header.unlink(missing_ok=True)
        textures = [e for e in z.exports if e['class_index'] == '0x5b93bca796d1fa6f']
        if len(textures) != 1:
            raise ValueError('原二维图片不是受支持的单一 Texture2D')
        fmt, mips = self.mipmaps(z)
        mip = next((m for m in mips if max(m['width'], m['height']) <= 1024), mips[-1])
        if 'data' in mip:
            pixels = mip['data']
            bulk_mode = 'inline'
        else:
            bulk_name = self.filename(package, '.ubulk')
            bulk_path = store.case_paths.get(bulk_name.casefold())
            if bulk_path is None:
                raise FileNotFoundError('原二维图片缺少像素数据')
            bulk_size = store.ranges[store.files[bulk_path]][1]
            if not 0 < bulk_size <= MAX_BULK_BYTES:
                raise ValueError('原二维图片像素数据超出范围')
            bulk = store.read(bulk_path)
            pixels = bulk[mip['offset']:mip['offset'] + mip['size']]
            bulk_mode = 'external'
        if len(pixels) != mip['size']:
            raise ValueError('原二维图片像素长度不匹配')
        width, height = mip['width'], mip['height']
        if max(width, height) > 4096:
            raise ValueError('原二维图片尺寸超出预览范围')
        if fmt in ('PF_B8G8R8A8', 'PF_G8'):
            channels = 4 if fmt == 'PF_B8G8R8A8' else 1
            if len(pixels) != width * height * channels:
                raise ValueError('原二维图片像素尺寸不匹配')
            image = Image.frombytes('RGBA' if channels == 4 else 'L', (width, height), pixels,
                                    'raw', 'BGRA' if channels == 4 else 'L').convert('RGBA')
        else:
            with Image.open(io.BytesIO(self.dds_bytes(fmt, width, height, pixels))) as decoded:
                image = decoded.convert('RGBA')
        output = io.BytesIO()
        image.save(output, format='PNG', optimize=True)
        png = output.getvalue()
        provenance = {'package': package, 'container': store.path.name, 'toc_sha256': toc_hash,
                      'source_sha256': hashlib.sha256(raw).hexdigest(), 'pixel_sha256': hashlib.sha256(pixels).hexdigest(),
                      'png_sha256': hashlib.sha256(png).hexdigest(), 'pixel_format': fmt,
                      'mip': mip['mip'], 'bulk_mode': bulk_mode, 'width': width, 'height': height,
                      'pixel_policy': 'Original selected mip pixels and alpha; no recoloring, 3D rendering or upscaling.'}
        return png, provenance


class SkinPreviews:
    def __init__(self, root=ROOT, *, decoder=None, cache_bytes=MAX_CACHE_BYTES):
        self.root = Path(root).resolve()
        self.cache = (self.root / '.local/skin-previews').resolve()
        if self.root not in self.cache.parents:
            raise ValueError('预览缓存必须位于本项目内')
        self.lock = threading.RLock()
        self.maximum = cache_bytes
        self._decoder = decoder
        self._selections = OrderedDict()
        self._skins = {}
        report = json.loads((self.root / 'reports/all-skins.json').read_text(encoding='utf-8'))
        self.catalog_id = report.get('inventory_sha256') or hashlib.sha256(json.dumps(report, sort_keys=True).encode()).hexdigest()
        self._ui = {_guid(asset.get('guid', asset.get('id'))): asset['ui']
                    for asset in report.get('assets', []) if isinstance(asset.get('ui'), dict) and asset.get('guid', asset.get('id'))}
        self.packages = set()
        rows = report.get('skins', [])
        for row in rows.values() if isinstance(rows, dict) else rows:
            skin_id = _guid(row.get('guid', row.get('id')))
            weapon_id = _guid(row.get('weapon_guid', row.get('equippable_guid')))
            self._skins[(weapon_id, skin_id)] = row
            for item in (row, *row.get('levels', []), *row.get('chromas', [])):
                self.packages.update(p for p, _ in self._pictures(item))

    def _pictures(self, item):
        ui = self._ui.get(_guid(item.get('guid', item.get('id')), optional=True), {})
        result = []
        for path, field in ((ui.get('full_render_override'), 'FullRenderOverride'),
                            (ui.get('full_render'), 'FullRender'),
                            (item.get('icon'), 'DisplayIcon'), (ui.get('icon'), 'DisplayIcon')):
            package = _package(path)
            if package and package not in {p for p, _ in result}:
                result.append((package, field))
        return result

    @staticmethod
    def _choice(rows, requested, label):
        result = next((r for r in rows if _guid(r.get('guid', r.get('id')), optional=True) == requested), None)
        if result is None:
            raise ValueError(f'所选{label}不属于此皮肤')
        return result

    def _selection(self, weapon, skin, level, chroma):
        weapon, skin = _guid(weapon), _guid(skin)
        row = self._skins.get((weapon, skin))
        if row is None:
            raise ValueError('所选皮肤不属于此武器')
        levels, chromas = row.get('levels', []), row.get('chromas', [])
        if not levels or not chromas:
            raise ValueError('此皮肤的等级或炫彩目录不完整')
        level = _guid(level, optional=True) or _guid(row.get('default_level_guid') or levels[-1]['guid'])
        chroma = _guid(chroma, optional=True) or _guid(row.get('default_chroma_guid') or next((r['guid'] for r in chromas if r.get('is_default')), chromas[0]['guid']))
        lv, ch = self._choice(levels, level, '等级'), self._choice(chromas, chroma, '炫彩')
        return {'weapon_id': weapon, 'skin_id': skin, 'level_id': level, 'chroma_id': chroma}, row, lv, ch

    def _read_cache(self, key):
        png, meta = self.cache / (key + '.png'), self.cache / (key + '.json')
        try:
            record = json.loads(meta.read_text(encoding='utf-8'))
            data = png.read_bytes()
            if record.get('catalog_id') != self.catalog_id or record.get('png_sha256') != hashlib.sha256(data).hexdigest():
                return None
            png.touch()
            return data, record
        except (OSError, ValueError):
            return None

    def _image(self, package):
        key = hashlib.sha256((ALGORITHM + '\n' + self.catalog_id + '\n' + package).encode()).hexdigest()
        cached = self._read_cache(key)
        if cached:
            return key, *cached
        self.cache.mkdir(parents=True, exist_ok=True)
        if self._decoder is None:
            self._decoder = OriginalTextureArchive(self.root, self.packages)
        data, record = self._decoder.decode(package, self.cache)
        if len(data) > self.maximum:
            raise ValueError('这张原二维图片超出预览缓存范围')
        record.update(catalog_id=self.catalog_id, key=key)
        target = self.cache / (key + '.png')
        temporary = self.cache / (key + '.tmp')
        temporary.write_bytes(data)
        temporary.replace(target)
        target.with_suffix('.json').write_text(json.dumps(record, ensure_ascii=False, indent=2), encoding='utf-8')
        self._trim_cache(keep=key)
        return key, data, record

    def _trim_cache(self, *, keep):
        # Evict only this helper's own hash-named PNG/JSON files, never recurse
        # or touch resources, user settings or other development artifacts.
        files = [p for p in self.cache.glob('*.png') if KEY.fullmatch(p.stem) and p.resolve().parent == self.cache]
        total = sum(p.stat().st_size for p in files)
        for path in sorted(files, key=lambda p: p.stat().st_mtime_ns):
            if total <= self.maximum:
                break
            if path.stem == keep:
                continue
            size = path.stat().st_size
            path.unlink(missing_ok=True)
            path.with_suffix('.json').unlink(missing_ok=True)
            total -= size

    def describe(self, weapon, skin, level=None, chroma=None):
        with self.lock:
            selection, row, lv, ch = self._selection(weapon, skin, level, chroma)
            requested = tuple(selection.values())
            if requested in self._selections:
                stamp, old = self._selections[requested]
                self._selections.move_to_end(requested)
                if (not old.get('available') and time.monotonic()-stamp < 8) or (old.get('available') and self._read_cache(old['key'])):
                    return dict(old)
            is_default = bool(ch.get('is_default') or _guid(row.get('default_chroma_guid'), optional=True) == selection['chroma_id'])
            default = next((r for r in row['chromas'] if r.get('is_default')
                            or _guid(r.get('guid', r.get('id')), optional=True) == _guid(row.get('default_chroma_guid'), optional=True)), None)
            default_packages = {p for p, _ in self._pictures(default)} if default else set()
            # Chroma UIData owns the actual FullRender references. A number of
            # standard skins inherit a dummy DisplayIcon while supplying their
            # real picture here; that dummy must never count as a gun preview.
            chroma_pictures = self._pictures(ch)
            level_pictures = self._pictures(lv)
            if not is_default:
                chroma_pictures.sort(key=lambda picture: picture[0] in default_packages)
            order = [(p, 'chroma', field) for p, field in chroma_pictures]
            if (is_default and level_pictures and chroma_pictures
                    and not any(field != 'DisplayIcon' for _, field in chroma_pictures)
                    and level_pictures[0][0] not in default_packages):
                order = [(p, 'level', field) for p, field in level_pictures] + order
            else:
                order += [(p, 'level', field) for p, field in level_pictures]
            order += [(p, 'skin', field) for p, field in self._pictures(row)]
            attempted = set()
            errors = []
            result = {**selection, 'available': False, 'image_url': None, 'kind': None,
                      'exact_chroma': False, 'fallback': False, 'note': '此款暂无可用的二维预览，仍可正常应用外观。'}
            for package, kind, field in order:
                if package is None or package in attempted:
                    continue
                attempted.add(package)
                try:
                    key, _, record = self._image(package)
                except (OSError, ValueError, RuntimeError, IndexError, KeyError, AssertionError, struct.error) as error:
                    errors.append(type(error).__name__)
                    continue
                exact_chroma = kind == 'chroma' and (is_default or package not in default_packages)
                fallback = bool(errors) or not is_default and not exact_chroma
                result.update(available=True, key=key, image_url=f'/api/skin-previews/{key}.png',
                              width=record['width'], height=record['height'], kind=kind, image_field=field,
                              exact_chroma=exact_chroma, fallback=fallback,
                              note=('此炫彩暂无单独预览，显示原款图片。' if not is_default and not exact_chroma
                                    else '静态预览，动态特效请在游戏内查看。'))
                break
            self._selections[requested] = time.monotonic(), dict(result)
            while len(self._selections) > 256:
                self._selections.popitem(last=False)
            return result

    @staticmethod
    def _json(value, status=200):
        return PreviewResponse(json.dumps(value, ensure_ascii=False).encode('utf-8'), 'application/json; charset=utf-8', status)

    def handle_request(self, url):
        parsed = urlsplit(url)
        if parsed.path == '/api/skin-preview':
            query = parse_qs(parsed.query, keep_blank_values=True)
            if set(query) - {'weapon', 'skin', 'level', 'chroma', 'format'} or any(len(v) != 1 for v in query.values()):
                raise ValueError('无效的预览请求')
            args = {k: query.get(k, [None])[0] for k in ('weapon', 'skin', 'level', 'chroma')}
            format_ = query.get('format', ['json'])[0]
            if format_ not in ('json', 'png'):
                raise ValueError('无效的预览格式')
            with self.lock:
                description = self.describe(**args)
                if format_ == 'json':
                    return self._json(description)
                if not description['available']:
                    return self._json({'error': description['note']}, 404)
                data, _ = self._read_cache(description['key'])
                return PreviewResponse(data, 'image/png')
        if parsed.path.startswith('/api/skin-previews/'):
            leaf = parsed.path.removeprefix('/api/skin-previews/')
            if not leaf.endswith('.png') or not KEY.fullmatch(leaf[:-4]):
                raise ValueError('无效的预览图片编号')
            with self.lock:
                found = self._read_cache(leaf[:-4])
                if found is None:
                    return self._json({'error': '预览缓存已更新，请重新选择外观'}, 404)
                return PreviewResponse(found[0], 'image/png', cache_control='private, max-age=86400')
        return None


_instances = {}
_instances_lock = threading.Lock()


def previews_for(root=ROOT):
    root = Path(root).resolve()
    with _instances_lock:
        if root not in _instances:
            _instances[root] = SkinPreviews(root)
        return _instances[root]
