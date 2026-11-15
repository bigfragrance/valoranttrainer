"""Candidate map allowlist and evidence independent of the standalone trainer."""
from __future__ import annotations

from copy import deepcopy
import hashlib
import json
from pathlib import Path

import native_maps

ROOT = Path(__file__).resolve().parents[1]
BACKEND = 'local-server-candidate'
VALIDATION_SOURCE = 'local-server-map-validation-v1'
CASE_KIND = 'local-server-map-case'
VALIDATION_INDEX = '.local/local-server/map-validation/results.json'


def read_json(path):
    try:
        value = json.loads(Path(path).read_text(encoding='utf-8'))
        return value if isinstance(value, dict) else {}
    except (FileNotFoundError, json.JSONDecodeError):
        return {}


def whitelisted_maps(root=ROOT):
    return {key: row for key, row in native_maps.training_maps(root).items()
            if row.get('kind') == 'standard_5v5' or key == 'duel1' and row.get('kind') == 'duel'}


def selected_map(map_id, mode='practice', *, root=ROOT):
    if not isinstance(map_id, str) or mode not in ('practice', 'deathmatch'):
        raise ValueError('候选训练需要目录中的地图 ID 与已知模式')
    info = whitelisted_maps(root).get(map_id)
    if not info or mode == 'deathmatch' and info.get('kind') != 'standard_5v5':
        raise ValueError('所选地图或模式不在本地服务器候选允许的目录中')
    if native_maps.map_package(map_id, training=True) != info['world_path']:
        raise ValueError('候选地图目录与原资源路径不匹配')
    return info


def _current_module(root):
    module = root / '.local/local-server/module/local_server_bridge.dll'
    digest = read_json(module.parent / 'build.json').get('module_sha256')
    if not isinstance(digest, str) or len(digest) != 64:
        return None
    try:
        return digest if hashlib.sha256(module.read_bytes()).hexdigest() == digest else None
    except OSError:
        return None


def _case_status(root, index, module, map_id, mode):
    pending = {'status': 'not_tested', 'source': VALIDATION_SOURCE, 'playable_verified': False}
    if (not module or index.get('schema_version') != 1 or index.get('backend') != BACKEND
            or index.get('source') != VALIDATION_SOURCE):
        return pending
    entries = index.get('cases')
    if not isinstance(entries, list):
        return pending
    for entry in reversed(entries):
        if (not isinstance(entry, dict) or entry.get('map_id') != map_id or entry.get('mode') != mode
                or entry.get('module_sha256') != module or not isinstance(entry.get('evidence'), str)):
            continue
        path = (root / entry['evidence']).resolve()
        if (root / '.local/local-server').resolve() not in path.parents:
            continue  # The old trainer's feature-verification reports are not candidate proof.
        evidence = read_json(path)
        if (evidence.get('schema_version') != 1 or evidence.get('kind') != CASE_KIND
                or evidence.get('backend') != BACKEND or evidence.get('network_flavor') != 'playable'
                or evidence.get('module_sha256') != module or evidence.get('map_id') != map_id
                or evidence.get('mode') != mode or type(evidence.get('passed')) is not bool):
            continue
        checks = evidence.get('checks')
        passed = evidence['passed'] and isinstance(checks, dict) and bool(checks) and all(value is True for value in checks.values())
        return {'status': 'passed' if passed else 'failed', 'source': VALIDATION_SOURCE,
                'evidence': str(path), 'module_sha256': module, 'playable_verified': False,
                'scope': evidence.get('scope', 'Selected map/mode case only; full playable acceptance remains separate.')}
    return pending


def catalog_maps(root=ROOT):
    root = Path(root).resolve()
    maps = {key: deepcopy(row) for key, row in whitelisted_maps(root).items()}
    index = read_json(root / VALIDATION_INDEX)
    module = _current_module(root) if index else None
    for key, row in maps.items():
        modes = {mode: _case_status(root, index, module, key, mode) for mode in ('practice', 'deathmatch')}
        if row.get('kind') != 'standard_5v5':
            modes['deathmatch'] = {'status': 'unsupported', 'source': VALIDATION_SOURCE, 'playable_verified': False}
        row['local_server_validation'] = modes
        # Do not leave the old runtime/spawn/navigation verdict where an existing
        # map consumer might display it as a result of this different backend.
        row['validation'] = {name: value for name, value in row.get('validation', {}).items() if name.startswith('static_')}
        row['validation'].update(runtime='not_tested', runtime_source=VALIDATION_SOURCE)
    return list(maps.values())
