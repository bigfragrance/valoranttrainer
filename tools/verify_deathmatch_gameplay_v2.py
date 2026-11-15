"""Observe native finite ammo and actual spawn-protection removal/expiry."""
from __future__ import annotations

import time

from training_runtime_client import OwnedSession
from verify_combat_v2 import snapshot


def verify(active):
    report = {'passed': False}
    with OwnedSession(active) as session:
        probe = session.original()
        try:
            state = session.wait(lambda s: s.get('ready'))
            if state['mode'] != 'deathmatch' or state['infinite_ammo']:
                raise RuntimeError('This check requires deathmatch with finite ammo')
            report['before'] = state
            protected = [p['index'] for p in state['players'] if p['bot'] and p['spawn_protected']]
            # ready is the original inventory/binding checkpoint. The weapon's
            # authored draw animation still owns when the first click can fire.
            deadline = time.monotonic() + 1.7
            while time.monotonic() < deadline:
                probe.command(flags=8)
                time.sleep(.07)
            initial_ammo = probe.read().ammo
            report['initial_ammo'] = initial_ammo
            deadline = time.monotonic() + .65
            while time.monotonic() < deadline:
                probe.aim(pitch=65, yaw=0)
                probe.command(flags=9)
                time.sleep(.05)
            probe.command(flags=8)
            report['after_fire'] = probe.read().as_dict()
            report['ammo_consumed'] = 0 < report['after_fire']['ammo'] < initial_ammo
            state = session.wait(lambda s: not s['player']['spawn_protected'], timeout=2)
            report['fire_removed_protection'] = not state['player']['spawn_protected']
            if protected:
                state = session.wait(lambda s: all(not p['spawn_protected'] for p in s['players']
                                                   if p['index'] in protected), timeout=7)
                report['original_protection_expired'] = True
            else:
                report['original_protection_expired'] = False
            time.sleep(1.1)
            report['pickups'] = snapshot('deathmatch-pickups')
            report['after'] = state
            report['passed'] = all(report[k] for k in ('ammo_consumed', 'fire_removed_protection', 'original_protection_expired'))
            if not report['passed']:
                raise RuntimeError('Native finite-ammo/protection checkpoints did not pass')
        except Exception as error:
            report['error'] = str(error)
        finally:
            probe.release()
    return report
