"""Walk the real player into native melee range and observe original contacts."""
from __future__ import annotations

import json
import math
import struct
import time

from training_control import guid_words
from training_runtime_client import OwnedSession, ROOT
from verify_combat_v2 import aim_at, snapshot


def verify(active):
    report = {'passed': False}
    with OwnedSession(active) as session:
        probe = session.original()
        definitions = json.loads((ROOT / 'reports/all-weapons.json').read_text(encoding='utf-8'))['weapons']
        knife = next(w for w in definitions if w['display_name'] == 'Melee')
        try:
            session.command('weapon', struct.pack('<4I', *knife['words']))
            state = session.wait(lambda s: s.get('ready') and s['weapon']['id'] == ''.join(f'{word:08x}' for word in knife['words']))
            before = state['player']['kills']
            for _ in range(20):
                probe.command(flags=8)
                time.sleep(.06)
            deadline = time.monotonic() + 5
            while time.monotonic() < deadline:
                state = session.channel.state()
                target = next(p for p in state['players'] if p['bot'] and p['alive'])
                distance = math.dist(state['player']['feet'][:2], target['feet'][:2])
                aim_at(probe, (target['feet'][0], target['feet'][1], target['feet'][2]+95))
                if distance < 135:
                    break
                probe.command(flags=8, forward=1)
                time.sleep(.04)
            probe.command(flags=8)
            report['distance_cm'] = distance
            deadline = time.monotonic() + 5
            while time.monotonic() < deadline:
                state = session.channel.state()
                if state['player']['kills'] > before:
                    break
                target = next((p for p in state['players'] if p['bot'] and p['alive']), None)
                if target:
                    aim_at(probe, (target['feet'][0], target['feet'][1], target['feet'][2]+95))
                    probe.command(flags=9)
                else:
                    probe.command(flags=8)
                time.sleep(.05)
            probe.release()
            time.sleep(1.1)
            report['state'] = session.channel.state()
            report['melee'] = snapshot('original-melee-combat')
            report['passed'] = report['state']['player']['kills'] > before and report['melee']['damage_events'] > 0
            if not report['passed']:
                raise RuntimeError('Original melee contacts did not produce an actual native kill')
        except Exception as error:
            report['error'] = str(error)
            report['melee'] = snapshot('original-melee-combat')
        finally:
            probe.release()
    return report
