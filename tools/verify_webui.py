"""Exercise the local UI in an isolated, headless Edge profile on the project drive."""
from __future__ import annotations

import argparse
import base64
import json
from pathlib import Path
import subprocess
import time
from urllib.request import urlopen

from local_environment import project_environment

ROOT = Path(__file__).resolve().parents[1]


class DevTools:
    def __init__(self, address):
        import websocket
        self.socket = websocket.create_connection(address, timeout=10, suppress_origin=True)
        self.sequence, self.errors = 0, []

    def call(self, method, params=None):
        self.sequence += 1
        sequence = self.sequence
        self.socket.send(json.dumps({'id': sequence, 'method': method, 'params': params or {}}))
        while True:
            event = json.loads(self.socket.recv())
            if event.get('method') == 'Runtime.exceptionThrown':
                self.errors.append(event['params'])
            if event.get('id') == sequence:
                if 'error' in event:
                    raise RuntimeError(event['error'])
                return event.get('result', {})

    def evaluate(self, expression):
        result = self.call('Runtime.evaluate', {'expression': expression, 'returnByValue': True, 'awaitPromise': True})
        if result.get('exceptionDetails'):
            raise RuntimeError(result['exceptionDetails'])
        return result.get('result', {}).get('value')

    def wait(self, expression, timeout=12):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            result = self.evaluate(expression)
            if result:
                return result
            time.sleep(.1)
        raise RuntimeError('UI checkpoint timed out: ' + expression)


def verify(url):
    from urllib.parse import urlsplit
    address = urlsplit(url)
    if address.hostname not in ('localhost', '127.0.0.1') or address.scheme != 'http':
        raise ValueError('Only the local development UI may be tested')
    edge = Path('C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe')
    target = ROOT / '.local/webui-verification'
    target.mkdir(parents=True, exist_ok=True)
    profile = target / ('profile-' + str(time.time_ns()))
    log = (target / 'browser.log').open('wb')
    process = subprocess.Popen([str(edge), '--headless=new', '--no-first-run', '--no-default-browser-check',
                                '--disable-gpu', '--disable-background-networking', '--disable-component-update',
                                '--disable-sync', '--remote-debugging-port=0', '--user-data-dir=' + str(profile),
                                '--hide-scrollbars', 'about:blank'], stdout=log, stderr=log,
                               creationflags=subprocess.CREATE_NO_WINDOW, env=project_environment(ROOT))
    client = None
    try:
        deadline = time.monotonic() + 15
        active_port = profile / 'DevToolsActivePort'
        while not active_port.is_file() and time.monotonic() < deadline:
            time.sleep(.1)
        if not active_port.is_file():
            raise RuntimeError('Headless Edge did not publish its local debugging endpoint')
        port = int(active_port.read_text().splitlines()[0])
        with urlopen(f'http://127.0.0.1:{port}/json/list') as response:
            page = next(x for x in json.load(response) if x['type'] == 'page')
        client = DevTools(page['webSocketDebuggerUrl'])
        client.call('Runtime.enable')
        client.call('Page.enable')
        client.call('Emulation.setDeviceMetricsOverride', {'width': 1440, 'height': 1100, 'deviceScaleFactor': 1, 'mobile': False})
        client.call('Page.navigate', {'url': url})
        client.wait("document.querySelector('#connection')?.classList.contains('online') && document.querySelector('#practice-map').options.length > 1")
        initial = client.evaluate("fetch('/api/state').then(r=>r.json()).then(x=>x.config)")
        checks = {'desktop_overflow': client.evaluate('document.body.scrollWidth <= innerWidth')}
        checks['map_ready'] = client.wait("document.querySelector('#map-image').complete && document.querySelector('#map-image').naturalWidth > 0")
        for view in ('practice', 'loadout', 'deathmatch'):
            client.evaluate(f"document.querySelector('[data-view={view}]').click()")
            if view == 'loadout':
                client.wait("document.querySelectorAll('.skin-card').length > 0")
                checks['weapon_buttons'] = client.evaluate("document.querySelectorAll('.weapon-row').length")
                checks['skin_buttons'] = client.evaluate("document.querySelectorAll('.skin-card').length")
            time.sleep(.3)
            screenshot = client.call('Page.captureScreenshot', {'format': 'png', 'captureBeyondViewport': False})
            (target / (view + '-desktop.png')).write_bytes(base64.b64decode(screenshot['data']))
        client.call('Emulation.setDeviceMetricsOverride', {'width': 390, 'height': 844, 'deviceScaleFactor': 1, 'mobile': True})
        client.evaluate("document.querySelector('[data-view=practice]').click()")
        time.sleep(.3)
        checks['mobile_overflow'] = client.evaluate('document.body.scrollWidth <= innerWidth')
        screenshot = client.call('Page.captureScreenshot', {'format': 'png', 'captureBeyondViewport': True})
        (target / 'practice-mobile.png').write_bytes(base64.b64decode(screenshot['data']))
        # Navigation changes mode intentionally. Restore the pre-test config's
        # mode/map without replacing routes or user selections.
        restore = json.dumps({'mode': initial['mode'], 'map_id': initial['map_id']})
        client.evaluate(f"fetch('/api/state').then(r=>r.json()).then(s=>fetch('/api/config',{{method:'PATCH',headers:{{'Content-Type':'application/json'}},body:JSON.stringify({{revision:s.config.revision,patch:{restore}}})}})).then(r=>r.json())")
        checks['javascript_exceptions'] = client.errors
        result = {'checks': checks, 'passed': bool(checks['desktop_overflow'] and checks['mobile_overflow'] and not client.errors
                  and checks.get('weapon_buttons', 0) >= 20 and checks.get('skin_buttons', 0) > 0),
                  'artifacts': str(target)}
        (target / 'verification.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
        return result
    finally:
        if client:
            try:
                client.call('Browser.close')
            except Exception:
                pass
            client.socket.close()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.terminate()
            process.wait(timeout=5)
        log.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--url', default='http://127.0.0.1:8790')
    result = verify(parser.parse_args().url)
    print(json.dumps(result, ensure_ascii=False))
    raise SystemExit(0 if result['passed'] else 1)
