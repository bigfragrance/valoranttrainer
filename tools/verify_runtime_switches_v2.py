"""Verify HTTP mode/map/population changes recreate exact owned scenes."""
from __future__ import annotations

import json
import time

from verify_live_webui_v2 import ROOT, request, wait


def main():
    original = request('/api/state')['config']
    report = {'passed': False, 'cases': []}

    def update(patch):
        revision = request('/api/state')['config']['revision']
        return request('/api/config', {'patch': patch, 'revision': revision}, method='PATCH')

    try:
        identity = request('/api/identity')
        if identity.get('root') != str(ROOT) or identity.get('application') != 'valoranttrainer':
            raise RuntimeError('Local controller identity did not match this project')
        request('/api/stop', {})
        previous = None
        for mode, map_id, count in [('practice', 'duel1', 4), ('deathmatch', 'ascent', 4),
                                    ('deathmatch', 'duality', 3), ('practice', 'duel1', 4)]:
            started = time.monotonic()
            update({'mode': mode, 'map_id': map_id, 'total_players': count,
                    'paused': True, 'invincible': True})
            request('/api/start', {})
            expected = 2 if mode == 'practice' else count
            state = wait(lambda s: s['runtime'].get('game', {}).get('ready')
                         and s['runtime']['game']['mode'] == mode
                         and s['runtime']['game']['map_id'] == map_id
                         and len(s['runtime']['game']['players']) == expected, seconds=45)
            game, pid = state['runtime']['game'], state['runtime']['pid']
            weapon = original.get('weapon_id')
            saved = original['skins'].get(weapon)
            if saved:
                state = wait(lambda s: s['runtime'].get('game', {}).get('weapon', {}).get('skin_id') == saved['skin_id'])
                game = state['runtime']['game']
            row = {'mode': mode, 'map_id': map_id, 'players': len(game['players']), 'pid': pid,
                   'different_process': previous is None or pid != previous,
                   'seconds': time.monotonic() - started, 'weapon': game['weapon'],
                   'saved_skin_restored': not saved or game['weapon']['skin_id'] == saved['skin_id']}
            row['passed'] = row['different_process'] and row['saved_skin_restored']
            report['cases'].append(row)
            if not row['passed']:
                raise RuntimeError('Map change did not create a fresh original scene with the saved loadout')
            print(json.dumps({'checkpoint': 'http_scene_ready', 'mode': mode, 'map_id': map_id,
                              'players': row['players'], 'pid': pid}), flush=True)
            previous = pid
        report['passed'] = len(report['cases']) == 4 and all(c['passed'] for c in report['cases'])
    except Exception as error:
        report['error'] = str(error)
    finally:
        request('/api/stop', {})
        update({k: v for k, v in original.items() if k not in ('schema_version', 'revision')})
        report['restored_config'] = request('/api/state')['config']
    target = ROOT / '.local/webui-verification/runtime-switches.json'
    target.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'passed': report['passed'], 'report': str(target), 'error': report.get('error')}), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
