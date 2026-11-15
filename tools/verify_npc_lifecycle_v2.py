"""Observe real NPC ownership, shots and respawns in an owned free-for-all."""
from __future__ import annotations

import copy
import math
import time

from training_control import pack_configuration
from training_runtime_client import OwnedSession
from verify_combat_v2 import snapshot


def verify(active, config, *, ui_respawn=False):
    report = {'passed': False, 'samples': []}
    with OwnedSession(active) as session:
        probe = session.original()
        try:
            start = session.wait(lambda s: s.get('ready'))
            if start['mode'] != 'deathmatch':
                raise RuntimeError('NPC lifecycle check requires free-for-all')
            current = copy.deepcopy(config)
            current.update(paused=False, invincible=False)
            current['revision'] += 100
            session.command('configure', pack_configuration(current))
            # Put the real local actor at an original collision-checked spot
            # near a real bot, so a death/respawn does not depend on patrol luck.
            import struct
            for target in start['players']:
                if not target['bot'] or not target['alive']:
                    continue
                try:
                    session.command('test_player_near_bot', struct.pack('<I', target['index']))
                    break
                except RuntimeError:
                    continue
            if ui_respawn:
                session.command('toggle_options')
            began = time.monotonic()
            next_sample = began
            maximum_displacement = 0
            start_players = {p['index']: p for p in start['players']}
            assigned_classes = {p['index']: p['hero_class'] for p in start['players']}
            if len(set(assigned_classes.values())) != len(assigned_classes):
                raise RuntimeError('Free-for-all hero assignment contains duplicates')
            report['assigned_classes'] = assigned_classes
            while time.monotonic() - began < 100:
                if not session.alive():
                    raise RuntimeError('Owned native client exited during NPC lifecycle check')
                probe.command(flags=8)
                state = session.channel.state()
                if state.get('error'):
                    raise RuntimeError(state['error'])
                if time.monotonic() >= next_sample:
                    next_sample = time.monotonic() + 1
                    roles = snapshot('training-player-roles')
                    ownership = snapshot('original-npc-locality')
                    combat = snapshot('training-combat-runtime')
                    topbar = snapshot('original-deathmatch-topbar')
                    if roles['pid'] != session.active['pid']:
                        raise RuntimeError('Role snapshot belongs to another client')
                    rows = roles['players']
                    npcs = [p for p in rows if not p['local']]
                    if roles['actual_local_count'] != 1 or roles['local_player_count'] != 1:
                        raise RuntimeError('Native local-player membership changed')
                    if any(p['in_local_players'] or not p['perspective_query'] or not p['wwise_perspective'].startswith('ThirdPerson') for p in npcs):
                        raise RuntimeError('NPC acquired first-person view/audio ownership across a native death/respawn')
                    if not roles['listener_query'] or not roles['listener_is_actual_local']:
                        raise RuntimeError('Original audio listener changed ownership')
                    if ownership['error'] or not ownership['source_untouched'] or not ownership['actual_local_pair_valid']:
                        raise RuntimeError('NPC ownership adapter failed its original-link checks')
                    for player in state['players']:
                        if player['hero_class'] != assigned_classes[player['index']]:
                            raise RuntimeError('A native respawn changed its assigned hero')
                        before = start_players[player['index']]
                        if player['bot'] and player['epoch'] == before['epoch']:
                            maximum_displacement = max(maximum_displacement, math.dist(player['feet'], before['feet']))
                    respawns = sum(max(0, p['epoch']-start_players[p['index']]['epoch']) for p in state['players'])
                    local_deaths = state['player']['deaths'] - start['player']['deaths']
                    report['samples'].append({'elapsed': time.monotonic()-began,
                        'roles': roles, 'ownership': ownership, 'state': state,
                        'combat': combat, 'topbar': topbar, 'respawns': respawns, 'local_deaths': local_deaths,
                        'ui': snapshot('original-ui-input-mode') if ui_respawn else None})
                    if time.monotonic()-began >= 30 and respawns >= 10 and local_deaths >= 1 and state['player']['alive'] and state.get('ready'):
                        break
                time.sleep(.05)
            last = report['samples'][-1]
            report['maximum_same_pawn_displacement_cm'] = maximum_displacement
            topbar_samples = [s['topbar'] for s in report['samples']]
            report['topbar_death_resets'] = [
                {'index': e['index'], 'match_kills': e['match_kills'], 'streak': e['streak']}
                for s in report['samples'] for e in s['topbar']['players']
                if e.get('streak') == 0 and e['match_kills'] > 0
                and any(p['index'] == e['index'] and not p['alive'] for p in s['state']['players'])]
            report['checks'] = {'actual_npc_movement': maximum_displacement > 100,
                'native_respawns': last['respawns'] >= 10,
                'actual_local_death_and_fresh_pawn': last['local_deaths'] >= 1 and state['player']['epoch'] > start['player']['epoch'],
                'original_native_shots': last['combat'].get('nativeRays', 0) > 0,
                'original_ffa_topbar_scores_and_order': last['topbar']['ready'] and last['topbar']['original_points_verified'] and last['topbar']['descending_order_verified'] and last['topbar']['card_bindings_verified'],
                'original_topbar_has_all_players': last['topbar']['card_count'] == len(start_players),
                'no_team_topbar': last['topbar']['old_master_collapsed'],
                'topbar_uses_timed_streaks': all(t.get('score_semantics') == 'timed_kill_streak' for t in topbar_samples),
                'topbar_observed_nonzero_streak': any(e.get('streak', 0) > 0 for t in topbar_samples for e in t['players']),
                'topbar_displays_streak_separately_from_total': any(e['match_kills'] > e.get('streak', e['match_kills']) for t in topbar_samples for e in t['players']),
                # Card/FText inspection is deliberately throttled; players[]
                # contains newer live counts. Compare the text to the two native
                # team queries captured in that same card observation instead.
                'topbar_actual_text_matches_streak': all(
                    c['text_verified'] and c['score_verified'] and c['native_team_queries_verified']
                    and c['displayed_number'] == c['cached_score'] == c['player_team_points'] == c['actor_team_points']
                    for t in topbar_samples if t['ready'] for c in t['cards_left_to_right']),
                'topbar_resets_after_death': bool(report['topbar_death_resets'])}
            report['passed'] = all(report['checks'].values())
            if ui_respawn:
                report['ui_after_respawn'] = snapshot('original-ui-input-mode')
                mode = report['ui_after_respawn']
                open_now = mode['options_open'] or mode.get('shop_open') or mode.get('map_open')
                report['checks']['ui_mode_valid_after_local_respawn'] = (
                    mode['cursor_visible'] and mode['viewport_mode'] == 'ui_only'
                    and mode['ui_only_applied'] and mode['block_gameplay'] if open_now
                    else not mode['cursor_visible'] and mode['viewport_mode'] == 'game_only')
                session.command('close_active_ui')
                deadline = time.monotonic() + 3
                while time.monotonic() < deadline:
                    mode = snapshot('original-ui-input-mode')
                    if not mode['block_gameplay'] and not mode['cursor_visible']:
                        break
                    time.sleep(.08)
                report['ui_closed_after_respawn'] = mode
                report['checks']['ui_can_close_after_local_respawn'] = not mode['block_gameplay'] and not mode['cursor_visible']
                report['passed'] = all(report['checks'].values())
            if not report['passed']:
                raise RuntimeError('Native movement/shots/respawn evidence was incomplete')
        except Exception as error:
            report['error'] = str(error)
        finally:
            probe.release()
    return report
