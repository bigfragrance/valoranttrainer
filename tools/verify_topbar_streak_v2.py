"""Verify real FFA card text expires while the match score is retained."""
from __future__ import annotations

import copy
import time

from training_control import pack_configuration
from training_runtime_client import OwnedSession
from verify_combat_v2 import snapshot


def verify(active, config):
    report = {'passed': False}
    with OwnedSession(active) as session:
        probe = session.original()
        try:
            state = session.wait(lambda s: s.get('ready'))
            if state['mode'] != 'deathmatch':
                raise RuntimeError('Topbar streak verification requires a free-for-all')
            current = copy.deepcopy(config)
            current.update(paused=True, invincible=True)
            current['revision'] = max(current['revision'], state.get('applied_revision', 0)) + 1
            report['before_pause'] = snapshot('original-deathmatch-topbar')
            session.command('configure', pack_configuration(current))
            deadline = time.monotonic() + 1.2
            while time.monotonic() < deadline:
                probe.command(flags=8)
                time.sleep(.05)
            baseline = session.channel.state()
            report['baseline_totals'] = {p['index']: p['kills'] for p in baseline['players']}
            rules = snapshot('deathmatch-rules')
            if rules.get('streak_policy') == 'original_timed_component':
                report['original_timed_component'] = snapshot('original-timed-streak')
                active = [e for e in report['original_timed_component']['entries'] if e['count'] > 0]
                timeout = max((e['timer_remaining_seconds'] for e in active), default=0)
                report['streak_policy'] = 'original_timed_component'
                report['wait_from_original_remaining_seconds'] = timeout
            else:
                timeout = rules['streak_timeout_seconds']
                report['streak_timeout_seconds'] = timeout
            deadline = time.monotonic() + timeout + 1.3
            while time.monotonic() < deadline:
                probe.command(flags=8)
                time.sleep(.05)
            state = session.channel.state()
            topbar = snapshot('original-deathmatch-topbar')
            report['expired_topbar'] = topbar
            report['expired_totals'] = {p['index']: p['kills'] for p in state['players']}
            report['checks'] = {
                'source_is_timed_streak': topbar.get('score_semantics') == 'timed_kill_streak',
                'all_native_streaks_expired': all(p['streak'] == 0 for p in state['players']),
                'all_original_card_scores_zero': all(p['streak'] == p['original_points'] == 0 for p in topbar['players']),
                'all_actual_card_texts_zero': all(c['displayed_number'] == 0 and c['text_verified'] for c in topbar['cards_left_to_right']),
                'original_order_and_layout_verified': topbar['ready'] and topbar['descending_order_verified'] and topbar['native_card_order_verified'],
                'all_players_retained': topbar['card_count'] == len(state['players']),
                'total_match_score_preserved': report['baseline_totals'] == report['expired_totals'],
                'nonzero_match_score_retained': any(report['expired_totals'].values()),
                'match_not_ended_by_display_expiry': not state['ended'],
                'original_component_policy': rules.get('streak_policy') == 'original_timed_component',
            }
            report['passed'] = all(report['checks'].values())
            if not report['passed']:
                raise RuntimeError('Original topbar streak expiry or score preservation failed')
        except Exception as error:
            report['error'] = str(error)
        finally:
            probe.release()
    return report
