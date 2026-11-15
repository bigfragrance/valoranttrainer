"""Let a real bot kill the local player and validate a fresh original pawn."""
from __future__ import annotations

import copy
import struct
import time

from training_control import pack_configuration, guid_words
from training_runtime_client import OwnedSession
from verify_combat_v2 import aim_at, snapshot, prepare_target


def verify(active, config):
    report = {'passed': False}
    with OwnedSession(active) as session:
        probe = session.original()
        try:
            state = session.wait(lambda s: s.get('ready'))
            vandal = '9c82e19d457502001a813eacf00cf872'
            session.command('weapon', struct.pack('<4I', *guid_words(vandal)))
            state = session.wait(lambda s: s.get('ready') and s.get('weapon', {}).get('id') == vandal)
            settle = time.monotonic() + 1.7
            while time.monotonic() < settle:
                probe.command(flags=8)
                time.sleep(.06)
            before_epoch = state['player']['epoch']
            before_deaths = state['player']['deaths']
            prepare_target(session, config)
            # One authentic kill establishes feedback and persistent score.
            deadline = time.monotonic() + 12
            before_kills = state['player']['kills']
            while time.monotonic() < deadline:
                state = session.channel.state()
                if state['player']['kills'] > before_kills:
                    break
                bot = next(p for p in state['players'] if p['bot'] and p['alive'])
                aim_at(probe, (bot['feet'][0], bot['feet'][1], bot['feet'][2]+120))
                probe.command(flags=9)
                time.sleep(.05)
            probe.command(flags=8)
            report['before_death'] = state
            if state['player']['kills'] <= before_kills:
                raise RuntimeError('Could not establish an actual kill before the death test')
            session.wait(lambda s: any(p['bot'] and p['alive'] and p['epoch'] > 1 for p in s['players']), timeout=6)
            current = copy.deepcopy(config)
            current.update(invincible=False, paused=False)
            current['practice'].update(return_fire=True, style='wide')
            current['bots'].update(reaction_ms=60, reaction_spread_ms=0, aim_speed=1080, aim_spread=0,
                                   accuracy=1, accuracy_spread=.01, movement=.05, movement_spread=.01)
            current['revision'] += 1
            session.command('configure', pack_configuration(current))
            state = session.wait(lambda s: s['player']['deaths'] > before_deaths, timeout=18)
            report['death'] = state
            current.update(invincible=True, paused=True)
            current['revision'] += 1
            session.command('configure', pack_configuration(current))
            state = session.wait(lambda s: s.get('ready') and s['player']['alive'] and s['player']['epoch'] > before_epoch, timeout=15)
            report['after_respawn'] = state
            report['rules'] = snapshot('deathmatch-rules')
            report['passed'] = state['player']['health'] == 100 and state['player']['streak'] == 0 and state['player']['kills'] == before_kills+1
            if not report['passed']:
                raise RuntimeError('Fresh original local pawn did not preserve score and reset feedback')
        except Exception as error:
            report['error'] = str(error)
            report['native_state'] = session.channel.state()
            report['combat'] = snapshot('training-combat-runtime')
            report['rules'] = snapshot('deathmatch-rules')
        finally:
            probe.release()
    return report
