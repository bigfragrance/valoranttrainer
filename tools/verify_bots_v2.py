"""Observe original bot movement and independently validated practice routes."""
from __future__ import annotations

import copy
import json
import math
import time

from training_control import STYLES, pack_configuration
from training_runtime_client import OwnedSession
from verify_combat_v2 import snapshot


def verify(active, config):
    report = {'passed': False, 'styles': []}
    with OwnedSession(active) as session:
        try:
            state = session.wait(lambda s: s.get('ready'))
            initial = {p['index']: p for p in state['players']}
            current = copy.deepcopy(config)
            current['paused'] = False
            styles = STYLES if current['mode'] == 'practice' else ('deathmatch',)
            for style in styles:
                if style != 'deathmatch':
                    current['practice']['style'] = style
                    current['practice']['peeks_per_minute'] = 60
                current['revision'] += 1
                session.command('configure', pack_configuration(current))
                start = session.channel.state()
                deadline = time.monotonic() + (5 if style != 'deathmatch' else 8)
                maximum = 0
                while time.monotonic() < deadline:
                    state = session.channel.state()
                    if state.get('error'):
                        raise RuntimeError(state['error'])
                    for player in state['players']:
                        before = next((p for p in start['players'] if p['index'] == player['index']), None)
                        if player['bot'] and before and before['epoch'] == player['epoch']:
                            maximum = max(maximum, math.dist(player['feet'], before['feet']))
                    time.sleep(.1)
                diagnostics = snapshot('training-bot-controller')
                row = {'style': style, 'maximum_displacement_cm': maximum, 'diagnostics': diagnostics,
                       'state': state, 'passed': maximum > 35}
                report['styles'].append(row)
                if not row['passed']:
                    raise RuntimeError('Original bot input produced no actual movement: ' + style)
                print(json.dumps({'checkpoint': 'bot_movement', 'style': style, 'cm': round(maximum, 2)}), flush=True)
            if current['mode'] == 'practice':
                session.command('validate_routes')
                state = session.wait(lambda s: bool(s.get('routes')) and all(r['status'] != 'pending' for r in s['routes']), timeout=8)
                report['routes'] = state['routes']
                if not all(r['status'] == 'valid' for r in state['routes']):
                    raise RuntimeError('A default practice route did not pass actual geometry validation')
            profiles = [p['profile'] for p in initial.values() if p['bot']]
            report['varied_profiles'] = len(profiles) < 2 or len({json.dumps(p, sort_keys=True) for p in profiles}) > 1
            report['passed'] = report['varied_profiles'] and all(r['passed'] for r in report['styles'])
        except Exception as error:
            report['error'] = str(error)
            report['native_state'] = session.channel.state()
            report['native_bot'] = snapshot('training-bot-controller')
    return report
