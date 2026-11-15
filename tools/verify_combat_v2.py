"""Exercise actual original aim/fire input and observe native damage/respawn."""
from __future__ import annotations

import json
import math
import time
from copy import deepcopy

from training_runtime_client import OwnedSession, ROOT
from training_control import pack_configuration


def snapshot(name):
    path = ROOT / '.local/native-module' / (name + '.json')
    for attempt in range(12):
        try:
            return json.loads(path.read_text(encoding='utf-8'))
        except (PermissionError, FileNotFoundError, json.JSONDecodeError):
            if attempt == 11:
                raise
            time.sleep(.01)


def aim_at(probe, point):
    frame = probe.read()
    delta = [point[i] - frame.camera[i] for i in range(3)]
    pitch = math.degrees(math.atan2(delta[2], math.hypot(delta[0], delta[1])))
    yaw = math.degrees(math.atan2(delta[1], delta[0]))
    probe.aim(pitch=max(-88, min(88, pitch)), yaw=yaw)
    return frame


def prepare_target(session, config):
    """Let the real bot leave cover, then pause it at a saved expose point."""
    if not config or session.channel.state().get('mode') != 'practice':
        return
    routes = [r for r in config['routes'].get(config['map_id'], []) if r.get('enabled')]
    if not routes:
        return
    current = deepcopy(config)
    current.update(paused=False, invincible=True)
    current['practice']['return_fire'] = False
    current['revision'] += 1
    session.command('configure', pack_configuration(current))
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        state = session.channel.state()
        if any(p['bot'] and p['alive'] and (math.dist(p['feet'][:2], state['player']['feet'][:2]) < 250
                   or any(math.dist(p['feet'][:2], r['expose'][:2]) < 45 for r in routes))
               for p in state['players']):
            current['paused'] = True
            current['revision'] += 1
            session.command('configure', pack_configuration(current))
            return
        time.sleep(.03)
    raise RuntimeError('Original bot did not reach a saved expose point')


def verify(active=None, config=None):
    report = {'passed': False, 'kills': []}
    with OwnedSession(active) as session:
        state = session.wait(lambda s: s.get('ready'))
        report['pid'] = session.active['pid']
        report['initial_state'] = state
        probe = session.original()
        try:
            before = state['player']['kills']
            for ordinal in range(3):
                prepare_target(session, config)
                target_kills = before + ordinal + 1
                deadline = time.monotonic() + 12
                burst_epoch = time.monotonic()
                recoil = []
                while time.monotonic() < deadline:
                    state = session.channel.state()
                    if state.get('error'):
                        raise RuntimeError(state['error'])
                    if state.get('player', {}).get('kills', 0) >= target_kills:
                        break
                    enemies = [p for p in state.get('players', []) if p['bot'] and p['alive']]
                    if enemies:
                        feet = enemies[0]['feet']
                        frame = aim_at(probe, (feet[0], feet[1], feet[2] + 120))
                        recoil.append((frame.pitch_recoil, frame.yaw_recoil))
                        # Use real short bursts and the original recoil recovery.
                        # Holding a full spray while aiming only the base camera
                        # can climb above the target and is not a damage failure.
                        firing = (time.monotonic() - burst_epoch) % .8 < .3
                        probe.command(flags=9 if firing else 8)
                    else:
                        probe.command(flags=8)
                    time.sleep(.05)
                probe.command(flags=8)
                if state.get('player', {}).get('kills', 0) < target_kills:
                    report['failure_state'] = state
                    report['combat'] = snapshot('training-combat-runtime')
                    report['rules'] = snapshot('deathmatch-rules')
                    report['probe'] = probe.read().as_dict()
                    raise RuntimeError('Original fire input did not complete an actual native kill')
                report['kills'].append({'ordinal': ordinal + 1, 'state': state, 'recoil_samples': recoil})
                print(json.dumps({'checkpoint': 'native_kill', 'kills': target_kills}), flush=True)
                session.wait(lambda s: any(p['bot'] and p['alive'] and p['epoch'] > ordinal + 1 for p in s['players']), timeout=6)
            # Real elapsed idle time; total kills must persist while feedback resets.
            timeout = snapshot('deathmatch-rules')['streak_timeout_seconds']
            deadline = time.monotonic() + timeout + .6
            while time.monotonic() < deadline:
                probe.command(flags=8)
                time.sleep(.1)
            state = session.channel.state()
            report['after_timeout'] = state
            report['rules'] = snapshot('deathmatch-rules')
            report['combat'] = snapshot('training-combat-runtime')
            report['passed'] = state['player']['kills'] == before + 3 and state['player']['streak'] == 0
            if not report['passed']:
                raise RuntimeError('Kill streak did not reset while preserving match kills')
        except Exception as error:
            report['error'] = str(error)
        finally:
            probe.release()
    return report


if __name__ == '__main__':
    result = verify()
    output = ROOT / '.local/feature-verification/combat.json'
    output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'passed': result['passed'], 'error': result.get('error')}))
    raise SystemExit(0 if result['passed'] else 1)
