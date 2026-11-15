"""Exercise C/Q/E/X through the owned original Pawn input scheduler."""
from __future__ import annotations

import json
import math
import time

from training_runtime_client import OwnedSession, ROOT
from verify_combat_v2 import aim_at, snapshot


def verify(active=None, selected='cqex'):
    report = {'passed': False, 'phases': {}}
    with OwnedSession(active) as session:
        probe = session.original()
        report['pid'] = session.active['pid']

        def phase(name, seconds, flags=8, forward=0, aim=False, pitch=0):
            rows = []
            report['phases'][name] = rows
            deadline = time.monotonic() + seconds
            while time.monotonic() < deadline:
                state = session.channel.state()
                if state.get('error') or not session.alive():
                    raise RuntimeError(str(state.get('error') or 'Owned ability client exited'))
                if aim:
                    targets = [p for p in state['players'] if p['bot'] and p['alive']]
                    if targets:
                        feet = targets[0]['feet']
                        aim_at(probe, (feet[0], feet[1], feet[2] + 120))
                else:
                    probe.aim(pitch=pitch, yaw=0)
                probe.command(flags=flags, forward=forward)
                frame = probe.read()
                rows.append({'location': frame.location, 'camera': frame.camera,
                             'velocity': frame.velocity, 'ammo': frame.ammo,
                             'state': state, 'jett': snapshot('original-jett'),
                             'world': snapshot('original-jett-world'),
                             'sync': snapshot('original-jett-sync')})
                dagger_path = ROOT / '.local/native-module/original-dagger-combat.json'
                if dagger_path.is_file():
                    rows[-1]['dagger'] = snapshot('original-dagger-combat')
                time.sleep(min(.04, max(0, deadline-time.monotonic())))
            report['phases'][name] = rows
            print(json.dumps({'checkpoint': 'native_ability_phase', 'phase': name}), flush=True)
            return rows

        def approach():
            deadline = time.monotonic() + 8
            first = last = None
            while time.monotonic() < deadline:
                state = session.channel.state()
                targets = [p for p in state['players'] if p['bot'] and p['alive']]
                if not targets:
                    probe.command(flags=8)
                    time.sleep(.06)
                    continue
                target = targets[0]
                frame = aim_at(probe, (*target['feet'][:2], target['feet'][2] + 120))
                last = math.dist(frame.location[:2], target['feet'][:2])
                if first is None:
                    first = last
                if last <= 150:
                    probe.command(flags=8)
                    report.setdefault('approaches', []).append({'before_cm': first, 'after_cm': last, 'epoch': target['epoch']})
                    return
                probe.command(flags=8, forward=1)
                time.sleep(.04)
            raise RuntimeError('Original player movement did not reach the real dagger target')

        try:
            session.wait(lambda s: s.get('ready'))
            phase('settle', 1.2)
            if 'c' in selected:
                phase('c_baseline', 1.1)
                phase('c_throw', .25, flags=72, pitch=-8)
                phase('c_settle', 3.0, pitch=-8)
            if 'q' in selected:
                phase('q_baseline', 1.1)
                phase('q_press', .15, flags=136)
                phase('q_flight', 3.4)
            if 'e' in selected:
                phase('e_baseline', 1.1)
                phase('e_run', .65, forward=1)
                phase('e_return', .65, forward=-1)
                phase('e_prime', .15, flags=264)
                phase('e_prime_settle', 1.7)
                phase('e_dash', .15, flags=264, forward=1)
                phase('e_continue', .45, forward=1)
                phase('e_settle', 1.7)
            if 'x' in selected:
                phase('x_baseline', 1.1)
                phase('x_equip', .15, flags=520)
                phase('x_equip_settle', 1.8)
                phase('x_primary_one', .08, flags=9, pitch=55)
                phase('x_primary_one_settle', 1.1, pitch=55)
                phase('x_primary_two', .08, flags=9, pitch=55)
                phase('x_primary_two_settle', 1.1, pitch=55)
                approach()
                phase('x_primary_hit', .08, flags=9, aim=True)
                phase('x_primary_hit_settle', 1.1, aim=True)
                phase('x_primary_kill', .08, flags=9, aim=True)
                phase('x_primary_kill_settle', 2.0, aim=True)
                approach()
                phase('x_secondary', .08, flags=12, aim=True)
                phase('x_secondary_settle', 1.8, aim=True)
            phase('finish', 1.1)
            groups = report['phases']
            checks = {}
            for key, index in zip('cqex', range(4)):
                if key not in selected:
                    continue
                before = groups[key + '_baseline'][-1]['jett']['skills'][index]
                rows = [r for name, values in groups.items() if name.startswith(key + '_') for r in values]
                skills = [r['jett']['skills'][index] for r in rows]
                checks[key + '_activation'] = max(s['input_edges'] for s in skills) >= before['input_edges'] + 2
                checks[key + '_states'] = max(s['state_transitions'] for s in skills) > before['state_transitions']
                checks[key + '_resource'] = 0 <= min(s['charges'] for s in skills) < before['charges']
                if key == 'q':
                    base = groups['q_baseline'][-1]
                    rise = max(r['location'][2] for r in rows) - base['location'][2]
                    camera = max(r['camera'][2] for r in rows) - base['camera'][2]
                    report['updraft'] = {'rise_cm': rise, 'camera_rise_cm': camera}
                    checks['q_motion_camera'] = rise > 100 and camera >= rise * .8
                if key == 'e':
                    normal = max(math.hypot(*r['velocity'][:2]) for r in groups['e_run'])
                    speed = max(math.hypot(*r['velocity'][:2]) for r in groups['e_dash'] + groups['e_continue'])
                    report['tailwind'] = {'run_cm_s': normal, 'dash_cm_s': speed}
                    checks['e_dash_speed'] = normal > 100 and speed > normal * 1.5
                if key in 'cx':
                    kind = 'smoke' if key == 'c' else 'dagger'
                    initial = groups[key + '_baseline'][-1]['jett']['projectiles'][kind]['local_initializations']
                    if key == 'x':
                        # Original dagger movement can hit and clean up inside
                        # the factory call, before any world-actor scan.
                        before_native = groups['x_baseline'][-1].get('dagger', {})
                        initial_native = before_native.get('constructorCompleted', 0)
                        checks['x_projectile'] = (max(r.get('dagger', {}).get('constructorCompleted', 0) for r in rows) > initial_native
                                                  and max(r.get('dagger', {}).get('nativeEntranceForwards', 0) for r in rows) > before_native.get('nativeEntranceForwards', 0))
                    else:
                        checks[key + '_projectile'] = max(r['jett']['projectiles'][kind]['local_initializations'] for r in rows) > initial
            report['checks'] = checks
            if 'x' in selected:
                xrows = [r for name, rows in groups.items() if name.startswith('x_') for r in rows]
                native = [r.get('dagger', {}) for r in xrows]
                baseline_native = groups['x_baseline'][-1].get('dagger', {})
                checks['x_native_damage'] = max((d.get('damageEvents', 0) for d in native), default=0) > baseline_native.get('damageEvents', 0)
                checks['x_native_kill_score'] = (max((d.get('kills', 0) for d in native), default=0) > baseline_native.get('kills', 0)
                                                and groups['finish'][-1]['state']['player']['kills'] > groups['x_baseline'][-1]['state']['player']['kills'])
                checks['x_primary_kill_refill'] = (max((d.get('primaryKills', 0) for d in native), default=0) > baseline_native.get('primaryKills', 0)
                                                    and any(r['jett']['skills'][3]['charges'] == 5 for r in groups['x_primary_kill_settle']))
                checks['x_burst_consumed_remaining'] = any(r['jett']['skills'][3]['charges'] == 0 for r in groups['x_secondary_settle'])
            report['selected'] = selected
            report['passed'] = all(checks.values())
        except Exception as error:
            report['error'] = str(error)
            report['failure_state'] = session.channel.state()
        finally:
            probe.release()
    return report
