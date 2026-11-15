"""Verify the running project's HTTP -> native control path without OS input."""
from __future__ import annotations

import json
from pathlib import Path
import time
from urllib.request import Request, build_opener, ProxyHandler

from training_settings_v2 import world_to_uv

ROOT = Path(__file__).resolve().parents[1]
URL = 'http://127.0.0.1:8790'
HTTP = build_opener(ProxyHandler({}))


def request(path, body=None, method=None):
    req = Request(URL+path, data=json.dumps(body).encode() if body is not None else None,
                  method=method, headers={'Content-Type': 'application/json'})
    with HTTP.open(req, timeout=12) as response:
        return json.load(response)


def wait(check, seconds=25):
    deadline = time.monotonic()+seconds
    while time.monotonic() < deadline:
        state = request('/api/state')
        runtime = state['runtime']
        if runtime.get('error_text'):
            raise RuntimeError(runtime['error_text'])
        if check(state):
            return state
        time.sleep(.12)
    raise RuntimeError('Live WebUI native checkpoint did not complete')


def main():
    original = request('/api/state')['config']
    report = {'passed': False, 'checks': {}}
    try:
        state = wait(lambda s: s['runtime'].get('game', {}).get('ready'))
        report['pid'] = state['runtime']['pid']
        weapon = state['runtime']['game']['weapon']
        report['checks']['saved_default_skin'] = weapon['skin_id'] == original['skins'][original['weapon_id']]['skin_id']
        for mode in (1, 2, 0):
            revision = request('/api/state')['config']['revision']
            request('/api/effect', {'key': 'champion_glow', 'mode': mode, 'revision': revision})
            state = wait(lambda s: any(e['key'] == 'champion_glow' and e['requested'] == mode and e['verified'] for e in s['runtime']['game'].get('effects', [])))
            report['checks']['champion_glow_' + str(mode)] = True
        maps = request('/api/catalog')['maps']
        info = next(m for m in maps if m['id'] == state['config']['map_id'])
        uv = world_to_uv(info['world_to_image'], [-900, -300, 0])
        resolved = request('/api/map-point', {'map_id': info['id'], 'uv': uv, 'z': 0})
        report['map_point'] = resolved
        report['checks']['map_point_native_ground'] = resolved.get('validation') == 'original_ground_and_capsule_verified' and resolved['position'][2] > 100
        # Changing to a normal skin must not fail because a champion option was saved.
        phantom = 'ee8e8d15496b07ace5f68fae5d4c7b1a'
        request('/api/weapon', {'weapon_id': phantom, 'revision': state['config']['revision']})
        state = wait(lambda s: s['runtime']['game'].get('ready') and s['runtime']['game']['weapon']['id'] == phantom)
        report['checks']['normal_weapon_with_saved_champion_setting'] = not state['runtime'].get('effect_sync_error')
        request('/api/weapon', {'weapon_id': original['weapon_id'], 'revision': state['config']['revision']})
        state = wait(lambda s: s['runtime']['game'].get('ready') and s['runtime']['game']['weapon']['id'] == original['weapon_id'] and s['runtime']['game']['weapon']['skin_id'] == original['skins'][original['weapon_id']]['skin_id'])
        report['checks']['saved_skin_restored_after_switch'] = True
        report['passed'] = all(report['checks'].values())
    except Exception as error:
        report['error'] = str(error)
    finally:
        current = request('/api/state')['config']
        patch = {k:v for k,v in original.items() if k not in ('schema_version', 'revision')}
        request('/api/config', {'revision': current['revision'], 'patch': patch}, method='PATCH')
    path = ROOT / '.local/webui-verification/live-native.json'
    path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
