"""Original hero identities and a candidate-only, session-bound sidecar.

The original V2 command/Initial byte layout is deliberately unchanged. A hero
selection is consumed before the candidate creates a Pawn; changing it requires
a new pair and never mutates the currently controlled character.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import struct

from training_control import guid_words

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_HERO_ID = 'add6443a41bde414f6ade58d267f4e95'
SELECTION_NAME = 'session-hero-selection.bin'
SELECTION_MAGIC = b'VTHERO01'
SELECTION_VERSION = 1
SELECTION = struct.Struct('<8s4I4I32s')
SELECTION_SOURCE = 'native/local_server_hero_selection.h'


def hero_profile(root=ROOT):
    path = Path(root) / 'profiles/playable-heroes.json'
    raw = path.read_bytes()
    profile = json.loads(raw.decode('utf-8-sig'))
    if not isinstance(profile, dict):
        raise ValueError('可用英雄目录不完整')
    count, rows = profile.get('count'), profile.get('heroes')
    if (profile.get('schema_version') != 1 or type(count) is not int
            or not 2 <= count <= 32 or not isinstance(rows, list) or count != len(rows)):
        raise ValueError('可用英雄目录不完整')
    ids, codes = set(), set()
    for index, row in enumerate(rows):
        if not isinstance(row, dict):
            raise ValueError('可用英雄身份或顺序不匹配')
        hero_id, code = row.get('id'), row.get('code')
        if (type(row.get('index')) is not int or row['index'] != index
                or not isinstance(hero_id, str) or not re.fullmatch('[0-9a-f]{32}', hero_id)
                or hero_id in ids or list(guid_words(hero_id)) != row.get('guid_words')
                or not isinstance(code, str) or not code.isascii() or not code.isalnum() or code in codes
                or not isinstance(row.get('name'), str) or not row['name']
                or not isinstance(row.get('class'), str) or not row['class'].startswith('/Game/Characters/')
                or row.get('playable') is not True or row.get('excluded_from_content_catalog') is not False):
            raise ValueError('可用英雄身份或顺序不匹配')
        ids.add(hero_id)
        codes.add(code)
    if DEFAULT_HERO_ID not in ids:
        raise ValueError('英雄目录缺少默认捷风')
    return {'path': path, 'sha256': hashlib.sha256(raw).hexdigest(),
            'count': count, 'heroes': rows}


def selected_hero(hero_id, *, root=ROOT):
    if not isinstance(hero_id, str):
        raise ValueError('无效英雄')
    profile = hero_profile(root)
    row = next((row for row in profile['heroes'] if row['id'] == hero_id), None)
    if row is None:
        raise ValueError('所选英雄不在当前原资源目录中')
    return row, profile


def selection_payload(config, *, root=ROOT):
    row, profile = selected_hero(config.get('local_hero_id', DEFAULT_HERO_ID), root=root)
    revision = config.get('revision')
    if type(revision) is not int or not 0 <= revision <= 0x7fffffff:
        raise ValueError('无效英雄选择版本')
    return SELECTION.pack(SELECTION_MAGIC, SELECTION_VERSION, SELECTION.size,
                          revision, row['index'], *guid_words(row['id']),
                          bytes.fromhex(profile['sha256']))


def selection_build_supported(manifest):
    if not isinstance(manifest, dict) or not isinstance(manifest.get('source_sha256'), dict):
        return False
    digest = manifest['source_sha256'].get(SELECTION_SOURCE)
    return isinstance(digest, str) and re.fullmatch('[0-9a-f]{64}', digest) is not None


def require_selection_build(manifest, config):
    # Historical candidate builds still run their original Jett path. They must
    # never silently turn a newly selected hero back into Jett.
    if config.get('local_hero_id', DEFAULT_HERO_ID) != DEFAULT_HERO_ID and not selection_build_supported(manifest):
        raise RuntimeError('当前本地服务器构建仅支持捷风；请先构建包含全英雄选择的候选模块')


def local_hero_capability(root=ROOT, *, backend='original'):
    result = {'available': False, 'requires_restart': True, 'runtime_verified': False,
              'default_hero_id': DEFAULT_HERO_ID, 'catalog_available': False,
              'count': 0, 'backend': backend}
    try:
        profile = hero_profile(root)
        result.update(catalog_available=True, count=profile['count'], profile_sha256=profile['sha256'])
    except (FileNotFoundError, ValueError, KeyError, TypeError):
        result['reason'] = '当前英雄目录不可用'
        return result
    if backend != 'local-server-candidate':
        result['reason'] = '当前版本固定使用捷风；全英雄切换仅在本地服务器候选中开放'
        return result
    directory = Path(root) / '.local/local-server/module'
    try:
        manifest = json.loads((directory / 'build.json').read_text(encoding='utf-8-sig'))
        if selection_build_supported(manifest):
            expected = manifest['source_sha256'][SELECTION_SOURCE]
            captured = (directory / 'source-snapshot' / SELECTION_SOURCE).read_bytes()
            result['available'] = hashlib.sha256(captured).hexdigest() == expected
    except (FileNotFoundError, ValueError, KeyError, TypeError):
        pass
    result['reason'] = ('更换英雄会重新开始本局；各英雄技能仍在逐项验证' if result['available']
                        else '当前构建固定使用捷风；全英雄切换尚未开放')
    return result
