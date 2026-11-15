"""Exercise original 2D skin previews in an isolated server/headless profile."""
from __future__ import annotations

import base64
from copy import deepcopy
from datetime import datetime, timezone
import json
from pathlib import Path
import shutil
import subprocess
import threading
import time
from urllib.request import urlopen

from local_environment import project_environment
from training_server import Application, Catalog, make_server
from training_settings_v2 import DEFAULTS
from verify_webui import DevTools

ROOT = Path(__file__).resolve().parents[1]
VANDAL = '9c82e19d457502001a813eacf00cf872'
PRIME = 'b9ee2457481c67763f5b0ca8e8f90c89'
IMMORTALIZED = 'd980c0c8492bb8df2d91af99a7707170'


class StoppedRuntime:
    """The real HTTP/catalog/settings flow, with no launchable game backend."""
    def __init__(self):
        self.commands = []

    def state(self):
        return {'status': 'stopped', 'game': {}}

    def sync_native(self):
        pass

    def apply(self, config):
        return {'status': 'saved'}

    def command(self, kind, payload):
        self.commands.append((kind, payload.hex()))
        return {'status': 'saved'}

    def start_async(self, **_):
        raise RuntimeError('This isolated preview test cannot start a game')


def main():
    target = ROOT / '.local/skin-preview-verification'
    target.mkdir(parents=True, exist_ok=True)
    tag = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
    directory = target / tag
    directory.mkdir()
    app = Application(directory / 'isolated-app', runtime=False)
    app.root = ROOT
    app.catalog = Catalog(ROOT)
    app.runtime = StoppedRuntime()
    # All writes from the browser belong to this separate Settings instance.
    app.settings.value = deepcopy(DEFAULTS)
    app.settings.value.update(weapon_id=VANDAL, paused=True, invincible=True)
    server = make_server(0, app=app)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    url = f'http://127.0.0.1:{server.server_port}'
    profile = directory / 'headless-profile'
    edge = Path('C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe')
    log = (directory / 'browser.log').open('wb')
    process = subprocess.Popen([str(edge), '--headless=new', '--no-first-run', '--no-default-browser-check',
                                '--disable-gpu', '--disable-background-networking', '--disable-component-update',
                                '--disable-sync', '--remote-debugging-port=0', '--user-data-dir=' + str(profile),
                                '--disk-cache-size=1', '--media-cache-size=1', '--hide-scrollbars', 'about:blank'],
                               stdout=log, stderr=log, creationflags=subprocess.CREATE_NO_WINDOW,
                               env=project_environment(ROOT))
    report = {'passed': False, 'isolated_server': url, 'main_server_touched': False,
              'game_launched': False, 'physical_computer_use': False, 'checks': {}}
    client = None
    try:
        deadline = time.monotonic()+15
        port_file = profile / 'DevToolsActivePort'
        while not port_file.exists() and time.monotonic() < deadline:
            time.sleep(.1)
        if not port_file.exists():
            raise RuntimeError('Isolated headless browser did not start')
        port = int(port_file.read_text().splitlines()[0])
        with urlopen(f'http://127.0.0.1:{port}/json/list') as response:
            page = next(p for p in json.load(response) if p['type'] == 'page')
        client = DevTools(page['webSocketDebuggerUrl'])
        client.call('Runtime.enable')
        client.call('Page.enable')
        client.call('Emulation.setDeviceMetricsOverride', {'width': 1440, 'height': 1100, 'deviceScaleFactor': 1, 'mobile': False})
        client.call('Page.navigate', {'url': url})
        client.wait("document.querySelector('#connection')?.classList.contains('online')")
        client.evaluate("document.querySelector('[data-view=loadout]').click()")
        client.wait("document.querySelector('#skin-preview')?.dataset.state === 'ready'", timeout=45)
        checks = report['checks']
        checks['initial_original_image'] = client.evaluate("document.querySelector('#skin-preview-picture img').naturalWidth > 0")
        checks['lazy_original_thumbnails'] = client.wait("[...document.querySelectorAll('.skin-thumbnail img')].some(i => i.naturalWidth > 0)", timeout=30)
        client.evaluate(f"document.querySelector('[data-skin-id=\"{IMMORTALIZED}\"]').focus();document.querySelector('[data-skin-id=\"{IMMORTALIZED}\"]').click()")
        client.wait("document.querySelector('#skin-preview')?.dataset.state === 'ready'", timeout=30)
        checks['skin_selection_preserves_keyboard_focus'] = client.evaluate(f"document.activeElement.dataset.skinId === '{IMMORTALIZED}'")
        options = client.evaluate("[...document.querySelector('#skin-chroma').options].map(o=>({id:o.value,name:o.textContent}))")
        first = client.evaluate("document.querySelector('#skin-preview-picture img').src")
        client.evaluate(f"document.querySelector('#skin-chroma').value={json.dumps(options[1]['id'])};document.querySelector('#skin-chroma').dispatchEvent(new Event('change'))")
        checks['old_image_cleared_during_change'] = client.evaluate("!document.querySelector('#skin-preview-picture img') && document.querySelector('#skin-preview').dataset.state === 'loading'")
        client.wait("document.querySelector('#skin-preview')?.dataset.state === 'ready'", timeout=30)
        second = client.evaluate("document.querySelector('#skin-preview-picture img').src")
        checks['chroma_changes_real_image'] = first != second
        report['chroma_original_urls'] = [first, second]
        # Deliberately resolve an older request last to exercise the sequence
        # guard, without mutating production source or the user's settings.
        client.evaluate("window.realPreviewApi=api;api=async function(path,...args){const result=await realPreviewApi(path,...args);if(path.startsWith('/api/skin-preview?')&&path.includes('chroma=" + options[0]['id'] + "'))await new Promise(r=>setTimeout(r,400));return result}")
        client.evaluate(f"document.querySelector('#skin-chroma').value={json.dumps(options[0]['id'])};document.querySelector('#skin-chroma').dispatchEvent(new Event('change'));setTimeout(()=>{{document.querySelector('#skin-chroma').value={json.dumps(options[1]['id'])};document.querySelector('#skin-chroma').dispatchEvent(new Event('change'))}},30)")
        time.sleep(.8)
        client.wait("document.querySelector('#skin-preview')?.dataset.state === 'ready'")
        checks['late_old_preview_does_not_replace_current'] = client.evaluate(f"document.querySelector('#skin-preview-picture img').src === {json.dumps(second)}")
        client.evaluate('api=realPreviewApi')
        # Prime supplies four authored upgrade levels and four chromas.
        client.evaluate(f"document.querySelector('[data-skin-id=\"{PRIME}\"]').click()")
        client.wait("document.querySelector('#skin-preview')?.dataset.state === 'ready'", timeout=30)
        client.evaluate("document.querySelector('#skin-level').selectedIndex=0;document.querySelector('#skin-level').dispatchEvent(new Event('change'))")
        client.wait("document.querySelector('#skin-preview')?.dataset.state === 'ready'")
        checks['level_preview_refreshes'] = client.evaluate("document.querySelector('#skin-preview-variant').textContent.startsWith('等级 1')")
        client.evaluate("document.querySelector('#apply-skin').click()")
        client.wait("document.querySelector('#skin-application-status').textContent.includes('已保存')")
        checks['equip_flow_preserved'] = app.settings.read()['skins'][VANDAL]['skin_id'] == PRIME
        screenshot = client.call('Page.captureScreenshot', {'format': 'png', 'captureBeyondViewport': False})
        (directory / 'loadout-desktop.png').write_bytes(base64.b64decode(screenshot['data']))
        checks['desktop_no_horizontal_overflow'] = client.evaluate('document.body.scrollWidth <= innerWidth')
        # Simulated image unavailability must not present a previous skin or
        # prevent applying a valid catalog selection.
        client.evaluate("api=async function(path,...args){return path.startsWith('/api/skin-preview?')?{available:false,note:'测试：当前图片不可用'}:realPreviewApi(path,...args)};renderSkinPreview()")
        client.wait("document.querySelector('#skin-preview').dataset.state === 'unavailable'")
        checks['unavailable_clears_image_and_keeps_apply'] = client.evaluate("!document.querySelector('#skin-preview-picture img') && !document.querySelector('#apply-skin').disabled")
        client.evaluate('api=realPreviewApi;renderSkinPreview()')
        client.wait("document.querySelector('#skin-preview')?.dataset.state === 'ready'")
        # A weapon can change through native B while another page is visible.
        # Returning to the collection must fetch that weapon's skin catalog.
        ghost = '1baa85b44c70128464bb6481dfc3bb4e'
        client.evaluate("setView('practice');runtime.status='running';runtime.game.weapon={ready:true,id:'" + ghost + "'};syncWeaponSelection();setView('loadout')")
        client.wait("selectedSkinFor === '" + ghost + "' && selectedSkin?.weapon_id === '" + ghost + "' && document.querySelector('#skin-preview').dataset.state === 'ready'", timeout=30)
        checks['return_to_collection_uses_actual_weapon'] = True
        client.evaluate("runtime.status='stopped';selectedWeapon='" + VANDAL + "';loadSkins()")
        client.wait("selectedSkinFor === '" + VANDAL + "' && document.querySelector('#skin-preview').dataset.state === 'ready'")
        for width, name in [(900, 'compact'), (390, 'mobile')]:
            client.call('Emulation.setDeviceMetricsOverride', {'width': width, 'height': 900, 'deviceScaleFactor': 1, 'mobile': width < 500})
            client.evaluate("document.querySelector('.skin-panel').scrollIntoView({block:'start'})")
            time.sleep(.2)
            checks[name + '_no_horizontal_overflow'] = client.evaluate('document.body.scrollWidth <= innerWidth')
            screenshot = client.call('Page.captureScreenshot', {'format': 'png', 'captureBeyondViewport': False})
            (directory / ('loadout-' + name + '.png')).write_bytes(base64.b64decode(screenshot['data']))
        checks['no_javascript_exceptions'] = not client.errors
        report['javascript_exceptions'] = client.errors
        report['isolated_saved_selection'] = app.settings.read()['skins'][VANDAL]
        report['isolated_runtime_commands'] = app.runtime.commands
        report['passed'] = all(checks.values())
    except Exception as error:
        report['error'] = str(error)
        if client:
            try:
                report['failure_state'] = client.evaluate("({preview:document.querySelector('#skin-preview')?.dataset.state,caption:document.querySelector('#skin-preview-status')?.textContent,notice:document.querySelector('#notice')?.textContent,skins:document.querySelectorAll('.skin-card').length})")
                report['javascript_exceptions'] = client.errors
                screenshot = client.call('Page.captureScreenshot', {'format': 'png', 'captureBeyondViewport': False})
                (directory / 'failure.png').write_bytes(base64.b64decode(screenshot['data']))
            except Exception:
                pass
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
        server.shutdown()
        server.server_close()
        thread.join()
        resolved = profile.resolve()
        if resolved.parent == directory.resolve() and ROOT in resolved.parents and resolved.is_dir():
            for attempt in range(5):
                try:
                    shutil.rmtree(resolved)
                    report['isolated_headless_profile_removed'] = True
                    break
                except OSError:
                    if attempt == 4:
                        report['isolated_headless_profile_cleanup_pending'] = True
                    else:
                        time.sleep(.2)
    (directory / 'verification.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    (target / 'latest.json').write_text(json.dumps({'directory': str(directory), 'passed': report['passed']}), encoding='utf-8')
    print(json.dumps({'passed': report['passed'], 'error': report.get('error'), 'report': str(directory / 'verification.json')}, ensure_ascii=False))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
