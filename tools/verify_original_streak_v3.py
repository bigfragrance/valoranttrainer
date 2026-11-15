"""Exercise real FFA gun input and read the shipped streak timers and HUD."""
from __future__ import annotations

import copy
import math
import struct
import time

from training_control import pack_configuration
from training_runtime_client import OwnedSession
from verify_combat_v2 import aim_at, snapshot


def verify(active, config):
    report = {'passed': False, 'kills': [], 'hud_during_hold': []}
    with OwnedSession(active) as session:
        probe = session.original()
        try:
            state = session.wait(lambda s: s.get('ready'))
            if state['mode'] != 'deathmatch':
                raise RuntimeError('Original streak/HUD check requires free-for-all')
            current = copy.deepcopy(config)
            # Deliberately change the legacy practice-only interval. The original
            # FFA timers must remain governed by the shipped component and CDO.
            current.update(paused=False, invincible=True, infinite_ammo=True, streak_seconds=.5)
            current['revision'] = max(current['revision'], state.get('applied_revision', 0)) + 1
            session.command('configure', pack_configuration(current))
            initial_kills = last_kills = state['player']['kills']
            report['initial_match_kills'] = initial_kills
            target_id, target_until, last_health = None, 0., None
            blocked_until = {}
            deadline = time.monotonic() + 95
            while time.monotonic() < deadline:
                state = session.channel.state()
                if state.get('error'):
                    raise RuntimeError(state['error'])
                if state['ended']:
                    raise RuntimeError('The match ended before the local original HUD check')
                now = time.monotonic()
                if state['player']['kills'] > last_kills:
                    probe.command(flags=8)
                    last_kills = state['player']['kills']
                    observed = None
                    read_deadline = time.monotonic() + .8
                    while time.monotonic() < read_deadline:
                        native = snapshot('original-timed-streak')
                        observed = next((e for e in native['entries'] if e['index'] == 0), None)
                        if observed and observed['count'] == state['player']['streak'] and observed['timer_exists']:
                            break
                        time.sleep(.03)
                    hud = snapshot('original-streak-hud')
                    report['kills'].append({'match_kills': last_kills, 'state_streak': state['player']['streak'],
                                            'native': observed, 'hud': hud})
                    print(__import__('json').dumps({'checkpoint': 'own_original_streak',
                          'match_kills': last_kills, 'streak': state['player']['streak'],
                          'rate': observed.get('timer_rate_seconds') if observed else None,
                          'own_hud_visible': hud['own_visible']}), flush=True)
                    if observed and observed['count'] == 3:
                        break
                    target_id, target_until, last_health = None, 0., None
                enemies = [p for p in state['players'] if p['bot'] and p['alive']]
                target = next((p for p in enemies if p['index'] == target_id), None)
                if target:
                    health = target['health'] + target.get('shield', 0)
                    if last_health is not None and health < last_health:
                        target_until = now + .7
                    last_health = health
                if target is None or now >= target_until:
                    if target is not None:
                        blocked_until[target['index']] = now + 1.0
                    candidates = [p for p in enemies if blocked_until.get(p['index'], 0) <= now]
                    target = min(candidates, key=lambda p: math.dist(p['feet'], state['player']['feet']), default=None)
                    target_id = target['index'] if target else None
                    target_until, last_health = now + .4, None
                    if target:
                        probe.command(flags=8)
                        try:
                            session.command('test_player_near_bot', struct.pack('<I', target_id))
                            time.sleep(.10)
                        except RuntimeError:
                            pass  # Try the next actual target if its nearby geometry is blocked.
                if target:
                    feet = target['feet']
                    aim_at(probe, (feet[0], feet[1], feet[2] + 120))
                    probe.command(flags=9)
                else:
                    probe.command(flags=8)
                time.sleep(.035)
            else:
                raise RuntimeError('Real original firing did not reach a three-kill timed streak')
            probe.command(flags=8)
            current.update(paused=True)
            current['revision'] = max(current['revision'], state.get('applied_revision', 0)) + 1
            session.command('configure', pack_configuration(current))
            held_count = state['player']['streak']
            hold_until = time.monotonic() + 5.25
            while time.monotonic() < hold_until:
                probe.command(flags=8)
                report['hud_during_hold'].append(snapshot('original-streak-hud'))
                time.sleep(.18)
            held = session.channel.state()
            report['after_more_than_five_seconds'] = held['player']
            native = snapshot('original-timed-streak')
            report['native_after_hold'] = native
            remaining = max((e['timer_remaining_seconds'] for e in native['entries'] if e['count'] > 0), default=0)
            expire_until = time.monotonic() + remaining + 1.4
            while time.monotonic() < expire_until:
                probe.command(flags=8)
                time.sleep(.08)
            final = session.channel.state()
            hud = snapshot('original-streak-hud')
            report['expired_player'] = final['player']
            report['expired_hud'] = hud
            report['expired_native'] = snapshot('original-timed-streak')
            visible = [h for h in report['hud_during_hold'] if h['own_visible'] > 0]
            rows = [r for h in visible for r in h['rows'] if r['actual_own_source'] and r['visible']]
            percents = [r['charge_percent'] for r in rows]
            report['checks'] = {
                'real_local_kills': last_kills >= initial_kills + 3,
                'native_original_base_window': native['cdo_base_window'] == 4,
                'no_local_clock_or_window_table': not native['local_timeout_clock'] and not native['window_table_reimplemented'],
                'original_three_kill_timer_eight_seconds': any(k['native'] and k['native']['count'] == 3 and abs(k['native']['timer_rate_seconds']-8) < .03 for k in report['kills']),
                'legacy_interval_and_fixed_five_ignored': held['player']['streak'] == held_count == 3,
                'own_original_hud_visible': bool(visible),
                'original_widget_in_local_viewport': bool(rows) and all(r['original_widget_class'] and r['owner_is_actual_local'] and r['in_viewport'] for r in rows),
                'original_text_and_segments_present': any(r['text_queried'] and r['displayed_text'] and r['segments_queried'] and r['segment_children'] > 0 for r in rows),
                'original_countdown_decreases': bool(percents) and max(percents)-min(percents) > .3,
                'no_npc_hud': all(h['max_foreign_widgets'] == h['max_foreign_in_viewport'] == 0 for h in [hud, *report['hud_during_hold']]),
                'original_timer_resets_counter': final['player']['streak'] == 0,
                'original_hud_stops_after_window': hud['own_visible'] == 0,
                'match_kills_retained': final['player']['kills'] == held['player']['kills'],
            }
            report['passed'] = all(report['checks'].values())
            if not report['passed']:
                raise RuntimeError('Original timer/HUD checks failed')
        except Exception as error:
            report['error'] = str(error)
            report['failure_state'] = session.channel.state()
            report['native_at_failure'] = snapshot('original-timed-streak')
            report['hud_at_failure'] = snapshot('original-streak-hud')
        finally:
            probe.release()
    return report
