"""Validate original skin identities and representative live skin/ADS/FX switches."""
from __future__ import annotations

import json
import struct
import time

from training_runtime_client import OwnedSession, ROOT
from verify_combat_v2 import snapshot


def compact(value):
    return value.replace('-', '').lower()


def effect_payload(key, mode, value=0):
    return struct.pack('<80sIf', key.encode(), mode, value)


def verify(active=None):
    report = {'passed': False, 'cases': []}
    with OwnedSession(active) as session:
        report['pid'] = session.active['pid']
        session.wait(lambda s: s.get('ready'))
        try:
            session.command('validate_skin_catalog', timeout=15)
            time.sleep(1.1)
            report['catalog'] = snapshot('skin-catalog-validation')
            if not report['catalog']['passed']:
                raise RuntimeError('Original skin catalog identity audit failed')
            cases = json.loads((ROOT / 'reports/all-skins-test-cases.json').read_text(encoding='utf-8'))['cases']
            probe = session.original()
            for case in cases:
                row = {'name': case['name'], 'passed': False}
                report['cases'].append(row)
                words = case['weapon_words'] + case['skin_words'] + case['level_words'] + case['chroma_words']
                session.command('save_skin_selection', struct.pack('<16I', *words))
                session.command('weapon', struct.pack('<4I', *case['weapon_words']))
                session.wait(lambda s: s.get('ready') and s.get('weapon', {}).get('id') == compact(case['weapon_guid']))
                session.command('skin', struct.pack('<16I', *words))
                state = session.wait(lambda s: s.get('ready') and s.get('weapon', {}).get('skin_id') == compact(case['skin_guid'])
                                     and s.get('weapon', {}).get('level_id') == compact(case['level_guid'])
                                     and s.get('weapon', {}).get('chroma_id') == compact(case['chroma_guid']), timeout=25)
                row['actual_weapon'] = state['weapon']
                is_melee = state['weapon'].get('name') == 'Melee'
                if not is_melee:
                    for _ in range(16):
                        probe.command(flags=8)
                        time.sleep(.06)
                    row['hip_fov'] = probe.read().field_of_view
                    for _ in range(14):
                        probe.command(flags=12)
                        time.sleep(.06)
                    row['ads_fov'] = probe.read().field_of_view
                    if row['ads_fov'] >= row['hip_fov'] - .01:
                        for _ in range(4):
                            probe.command(flags=8)
                            time.sleep(.06)
                        for _ in range(14):
                            probe.command(flags=12)
                            time.sleep(.06)
                        row['ads_fov'] = probe.read().field_of_view
                    probe.command(flags=8)
                    if row['ads_fov'] >= row['hip_fov'] - .01:
                        raise RuntimeError('Original ADS did not reduce the actual field of view: ' + case['name'])
                if case['name'].startswith('Champions') and not is_melee:
                    row['effects'] = []
                    for mode in (1, 2, 0):
                        session.command('effect', effect_payload('champion_glow', mode))
                        state = session.wait(lambda s: any(e['key'] == 'champion_glow' and e['requested'] == mode and e['verified'] for e in s.get('effects', [])), timeout=5)
                        row['effects'].append(next(e for e in state['effects'] if e['key'] == 'champion_glow'))
                    count = next((e for e in state.get('effects', []) if e['key'] == 'skin_kill_count' and e['available']), None)
                    if count:
                        session.command('effect', effect_payload('skin_kill_count', 1, 25))
                        state = session.wait(lambda s: any(e['key'] == 'skin_kill_count' and e['verified'] and e.get('value') == 25 for e in s.get('effects', [])), timeout=5)
                        row['kill_progression'] = next(e for e in state['effects'] if e['key'] == 'skin_kill_count')
                row['passed'] = True
                print(json.dumps({'checkpoint': 'skin_ads_effect', 'name': row['name'], 'passed': True}), flush=True)
            report['passed'] = len(report['cases']) == len(cases) and all(c['passed'] for c in report['cases'])
        except Exception as error:
            report['error'] = str(error)
            report['native_state'] = session.channel.state()
            report['native_skins'] = snapshot('all-skins')
            report['native_effects'] = snapshot('skin-effect-controls')
        finally:
            if session.original_channel:
                session.original_probe.release()
    return report
