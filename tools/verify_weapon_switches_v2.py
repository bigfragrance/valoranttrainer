"""Exercise real original equip/skin transactions in this project's owned session."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import struct
import time

from training_control import guid_words
from training_runtime_client import OwnedSession, ROOT


def verify(active=None, *, shop=False, equip=True):
    report = {'schema_version': 1, 'started_utc': datetime.now(timezone.utc).isoformat(), 'weapons': [], 'passed': False}
    output = ROOT / '.local/feature-verification/weapons.json'
    output.parent.mkdir(parents=True, exist_ok=True)
    def save():
        temporary = output.with_suffix('.json.tmp')
        temporary.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        temporary.replace(output)
    with OwnedSession(active) as session:
        report['pid'] = session.active['pid']
        report['run_dir'] = str(session.run_dir)
        session.wait(lambda state: state.get('ready'))
        if shop:
            session.command('toggle_buy_menu')
            time.sleep(1.2)
            path = ROOT / '.local/native-module/original-buy-menu.json'
            menu = json.loads(path.read_text(encoding='utf-8'))
            report['shop'] = {key: menu.get(key) for key in ('open', 'native_open', 'model_count', 'catalog_model_count', 'cursor_visible', 'error')}
            if not (menu.get('open') and menu.get('native_open') and menu.get('model_count', 0) >= 20):
                save()
                raise RuntimeError('Original B shop did not expose its real 20 weapon models')
            report['purchases'] = []
            for model in menu['models']:
                words = model['guid_words']
                key = ''.join(f'{w:08x}' for w in words)
                deadline = time.monotonic() + 32
                while True:
                    session.command('buy_weapon', struct.pack('<4I', *words))
                    try:
                        session.wait(lambda s: s.get('ready') and s.get('weapon', {}).get('id') == key, timeout=.4)
                        break
                    except RuntimeError:
                        if time.monotonic() >= deadline:
                            raise
                        # Keep the original command rate limiter; retry as a
                        # subsequent ordinary click rather than changing it.
                        time.sleep(.15)
                report['purchases'].append({'id': key, 'passed': True})
            time.sleep(1.1)
            report['shop_after_purchases'] = json.loads(path.read_text(encoding='utf-8'))
            session.command('toggle_buy_menu')
        if not equip:
            report['passed'] = True
            save()
            return report
        weapons = json.loads((ROOT / 'reports/all-weapons.json').read_text(encoding='utf-8'))['weapons']
        for weapon in weapons:
            started = time.monotonic()
            key = ''.join(f'{w:08x}' for w in weapon['words'])
            row = {'id': key, 'name': weapon['display_name'], 'passed': False}
            try:
                session.command('weapon', struct.pack('<4I', *weapon['words']))
                state = session.wait(lambda s: s.get('ready') and s.get('weapon', {}).get('id') == key)
                row.update({'passed': True, 'seconds': round(time.monotonic() - started, 3), 'actual_weapon': state['weapon']})
                print(json.dumps({'weapon': row['name'], 'ready': True, 'seconds': row['seconds']}), flush=True)
            except Exception as error:
                row['error'] = str(error)
                report['weapons'].append(row)
                save()
                raise
            report['weapons'].append(row)
            save()
        report['passed'] = all(row['passed'] for row in report['weapons']) and len(report['weapons']) == 21
        # Restore the training primary weapon after exercising every definition.
        session.command('weapon', struct.pack('<4I', *guid_words('9c82e19d457502001a813eacf00cf872')))
        session.wait(lambda s: s.get('ready') and s.get('weapon', {}).get('id') == '9c82e19d457502001a813eacf00cf872')
        save()
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--active', type=Path)
    parser.add_argument('--shop', action='store_true')
    result = verify(parser.parse_args().active, shop=parser.parse_args().shop)
    print(json.dumps({'passed': result['passed'], 'weapons': len(result['weapons'])}))
