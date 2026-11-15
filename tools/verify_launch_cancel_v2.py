"""Verify Stop cancels the owned startup and cannot leave a delayed client."""
from __future__ import annotations

import json
import time

from verify_live_webui_v2 import ROOT, request


def main():
    identity = request('/api/identity')
    if identity.get('root') != str(ROOT):
        raise RuntimeError('Controller belongs to a different project')
    report = {'passed': False}
    try:
        request('/api/stop', {})
        started = time.monotonic()
        report['start'] = request('/api/start', {})
        report['stop'] = request('/api/stop', {})
        report['stop_seconds'] = time.monotonic() - started
        time.sleep(2)
        report['after_wait'] = request('/api/state')['runtime']
        report['passed'] = report['after_wait']['status'] == 'stopped' and report['after_wait']['pid'] is None
    except Exception as error:
        report['error'] = str(error)
    finally:
        request('/api/stop', {})
    output = ROOT / '.local/webui-verification/launch-cancel.json'
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'passed': report['passed'], 'seconds': report.get('stop_seconds'), 'error': report.get('error')}, ensure_ascii=False))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
