"""Exercise original menu open/leaf-close, input focus and original purchases."""
from __future__ import annotations

import copy
import struct
import time

from training_control import guid_words, pack_configuration
from training_runtime_client import OwnedSession
from verify_combat_v2 import snapshot


def verify(active, config):
    report = {'passed': False, 'cycles': []}
    with OwnedSession(active) as session:
        probe = session.original()
        try:
            state = session.wait(lambda s: s.get('ready'))
            initial_weapon_id = state['weapon']['id']
            report['mode'] = state['mode']
            current = copy.deepcopy(config)
            current.update(paused=state['mode'] != 'deathmatch', invincible=True)
            current['revision'] = max(current['revision'], state.get('applied_revision', 0)) + 1
            session.command('configure', pack_configuration(current))
            session.command('close_active_ui')

            def wait_ui(opened):
                deadline = time.monotonic() + 5
                last = None
                while time.monotonic() < deadline:
                    probe.command(flags=8)
                    last = snapshot('original-ui-input-mode')
                    if last.get('error'):
                        raise RuntimeError(last['stage'])
                    if opened:
                        if last.get('cursor_visible') and last.get('viewport_mode') == 'ui_only' and last.get('block_gameplay'):
                            return last
                    elif (not last.get('cursor_visible') and last.get('viewport_mode') == 'game_only'
                          and not last.get('block_gameplay') and not last.get('options_open') and not last.get('shop_open')):
                        return last
                    time.sleep(.08)
                raise RuntimeError('Original viewport did not settle: ' + str(last))

            wait_ui(False)
            def hold_menu(seconds=3):
                samples = []
                until = time.monotonic() + seconds
                while time.monotonic() < until:
                    probe.command(flags=8)
                    observed = snapshot('original-ui-input-mode')
                    samples.append({k: observed.get(k) for k in
                                    ('updates', 'mode_edges', 'cursor_visible', 'block_gameplay',
                                     'viewport_mode', 'options_open', 'shop_open', 'error')})
                    if (observed.get('error') or not observed.get('cursor_visible')
                            or not observed.get('block_gameplay') or observed.get('viewport_mode') != 'ui_only'):
                        raise RuntimeError('Original menu lost its continuous input state')
                    time.sleep(.15)
                return samples

            for kind, command in [('settings', 'toggle_options'), ('shop', 'toggle_buy_menu'),
                                  ('settings_again', 'toggle_options'), ('shop_again', 'toggle_buy_menu')]:
                session.command(command)
                opened = wait_ui(True)
                row = {'kind': kind, 'opened': opened}
                row['held_menu_samples'] = hold_menu()
                if kind == 'shop':
                    shop_deadline = time.monotonic() + 5
                    while True:
                        shop = snapshot('original-buy-menu')
                        if shop.get('open') and shop.get('native_open') and any(m.get('catalog_matched') for m in shop.get('models', [])):
                            break
                        if time.monotonic() >= shop_deadline:
                            raise RuntimeError('Original shop model snapshot did not reach its open generation')
                        time.sleep(.08)
                    report['shop_before'] = shop
                    pair = ['9c82e19d457502001a813eacf00cf872', 'ee8e8d15496b07ace5f68fae5d4c7b1a']
                    row['ownership_switches'] = []
                    for key in pair * 2:
                        deadline = time.monotonic() + 35
                        while True:
                            session.command('buy_weapon', struct.pack('<4I', *guid_words(key)))
                            try:
                                session.wait(lambda s: s.get('ready') and s.get('weapon', {}).get('id') == key, timeout=.6)
                                break
                            except RuntimeError:
                                if time.monotonic() >= deadline:
                                    raise
                                time.sleep(.15)
                        until = time.monotonic() + 8
                        while True:
                            after = snapshot('original-buy-menu')
                            models = {''.join(f'{w:08x}' for w in m['guid_words']): m for m in after.get('models', []) if m.get('catalog_matched')}
                            if (after.get('open') and all(k in models for k in pair)
                                    and all(models[k].get('owned') == (k == key) for k in pair)):
                                row['ownership_switches'].append({'weapon_id': key,
                                    'models': {k: models[k] for k in pair},
                                    'refresh_passes': after.get('model_refresh_passes'),
                                    'menu_remained_open': True})
                                break
                            if time.monotonic() >= until:
                                raise RuntimeError('Original shop ownership did not refresh without reopening')
                            probe.command(flags=8)
                            time.sleep(.1)
                    row['purchased_actual_weapon'] = session.channel.state()['weapon']
                    report['shop_after'] = snapshot('original-buy-menu')
                session.command('close_active_ui')
                row['closed'] = wait_ui(False)
                report['cycles'].append(row)
            before = probe.read()
            wanted = before.rotation[1] + 11.25
            for _ in range(8):
                probe.aim(pitch=0, yaw=wanted)
                probe.command(flags=8)
                time.sleep(.05)
            after = probe.read()
            report['view_before'], report['view_after'] = before.as_dict(), after.as_dict()
            error = abs((after.rotation[1]-wanted+180) % 360-180)
            report['checks'] = {
                'four_original_menu_roundtrips': len(report['cycles']) == 4,
                'original_ui_only_mode': all(r['opened']['ui_only_applied'] for r in report['cycles']),
                'cursor_visible_in_menus': all(r['opened']['cursor_visible'] for r in report['cycles']),
                'continuous_menu_state': all(len(r.get('held_menu_samples', [])) >= 10 for r in report['cycles']),
                'ownership_refresh_without_reopening': len(report['cycles'][1].get('ownership_switches', [])) == 4,
                'same_escape_leaf_close_path': all(r['closed'].get('close_completions', 0) > 0 for r in report['cycles']),
                'game_only_and_cursor_restored': all(r['closed']['viewport_mode'] == 'game_only' and not r['closed']['cursor_visible'] for r in report['cycles']),
                'original_purchase_equips_weapon': bool(report['cycles'][1].get('purchased_actual_weapon')),
                'view_rotation_restored': error < .1,
            }
            report['passed'] = all(report['checks'].values())
            if not report['passed']:
                raise RuntimeError('Original UI mode/purchase/close checks failed')
        except Exception as error:
            report['error'] = str(error)
            report['ui_at_failure'] = snapshot('original-ui-input-mode')
            report['shop_at_failure'] = snapshot('original-buy-menu')
        finally:
            try:
                session.command('close_active_ui')
            except Exception:
                pass
            probe.release()
            if 'initial_weapon_id' in locals() and session.alive():
                session.command('weapon', struct.pack('<4I', *guid_words(initial_weapon_id)))
                session.wait(lambda s: s.get('ready') and s.get('weapon', {}).get('id') == initial_weapon_id)
    report['physical_mouse_or_key_delivery_tested'] = False
    return report
