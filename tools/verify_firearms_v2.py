"""Verify each original firearm emits its own native shot directions."""
from __future__ import annotations

import json
import struct
import time

from training_runtime_client import OwnedSession, ROOT


def verify(active):
    report = {'passed': False, 'weapons': []}
    with OwnedSession(active) as session:
        probe = session.original()
        definitions = json.loads((ROOT / 'reports/all-weapons.json').read_text(encoding='utf-8'))['weapons']
        try:
            for weapon in definitions:
                if weapon['display_name'] == 'Melee':
                    continue
                key = ''.join(f'{word:08x}' for word in weapon['words'])
                row = {'name': weapon['display_name'], 'passed': False}
                report['weapons'].append(row)
                session.command('weapon', struct.pack('<4I', *weapon['words']))
                session.wait(lambda s: s.get('ready') and s['weapon']['id'] == key)
                for _ in range(28):
                    probe.aim(pitch=60, yaw=0)
                    probe.command(flags=8)
                    time.sleep(.06)
                before = session.channel.state()['combat']['native_rays']
                deadline = time.monotonic() + 3
                pressed = True
                while time.monotonic() < deadline:
                    probe.aim(pitch=60, yaw=0)
                    probe.command(flags=9 if pressed else 8)
                    pressed = not pressed
                    time.sleep(.06)
                    after = session.channel.state()['combat']['native_rays']
                    if after > before:
                        break
                probe.command(flags=8)
                row.update(native_rays=after-before, passed=after>before)
                if not row['passed']:
                    row['probe'] = probe.read().as_dict()
                    row['state'] = session.channel.state()
                    raise RuntimeError('No original shot direction emitted: ' + row['name'])
                print(json.dumps({'checkpoint': 'original_firearm_shot', 'name': row['name'], 'rays': row['native_rays']}), flush=True)
            report['passed'] = len(report['weapons']) == 20 and all(w['passed'] for w in report['weapons'])
        except Exception as error:
            report['error'] = str(error)
        finally:
            probe.release()
    return report
