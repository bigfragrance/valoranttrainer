"""Exact owned-session client shared by offline feature verification tools."""
from __future__ import annotations

import json
from pathlib import Path
import time

from capture_owned_thread import ROOT, _validated_active, _kernel, _creation, _image
from training_control import Channel


class OwnedSession:
    def __init__(self, active_path=None):
        if active_path is None:
            latest = json.loads((ROOT / '.local/latest-training-session.json').read_text(encoding='utf-8'))
            active_path = Path(latest['session']) / 'result.active.json'
        self.active_path, self.active, self.identity, self.run_dir, image = _validated_active(active_path)
        self.kernel = _kernel()
        self.process = self.kernel.OpenProcess(0x1000 | 0x100000, False, self.active['pid'])
        self.channel = self.original_channel = None
        if not self.process:
            raise RuntimeError('The owned test client is no longer running')
        try:
            if not self.alive() or _creation(self.kernel, self.process) != self.identity['creation_filetime'] or _image(self.kernel, self.process) != image:
                raise RuntimeError('The active test process identity no longer matches')
            self.channel = Channel(self.active['pid'])
        except Exception:
            self.close()
            raise

    def alive(self):
        return bool(self.process and self.kernel.WaitForSingleObject(self.process, 0) == 0x102)

    def wait(self, predicate, timeout=20):
        deadline = time.monotonic() + timeout
        last = {}
        while self.alive() and time.monotonic() < deadline:
            last = self.channel.state()
            if last.get('error'):
                raise RuntimeError(last['error'])
            if predicate(last):
                return last
            time.sleep(.04)
        raise RuntimeError('Native checkpoint failed: ' + str({k: last.get(k) for k in ('stage', 'error', 'weapon')}))

    def command(self, kind, payload=b'', timeout=3):
        if not self.alive():
            raise RuntimeError('The owned test client exited')
        result = self.channel.send(kind, payload, timeout)
        if result['code'] >= 2:
            raise RuntimeError(result['message'])
        return result

    def original(self):
        if self.active['module_stage'] != 9:
            raise RuntimeError('Expiring test input requires an explicit stage9 session')
        if self.original_channel is None:
            from training_channel import TrainingChannel
            from original_probe_channel import OriginalProbe
            self.original_channel = TrainingChannel.open(self.active['pid'], alive=self.alive)
            self.original_probe = OriginalProbe(self.original_channel)
        return self.original_probe

    def stop(self):
        (self.run_dir / 'stop-request.json').write_text(json.dumps({'reason': 'owned_feature_verification_complete'}), encoding='utf-8')

    def close(self):
        if self.original_channel:
            try:
                self.original_probe.release()
            except Exception:
                pass
            self.original_channel.close()
        if self.channel:
            self.channel.close()
        if self.process:
            self.kernel.CloseHandle(self.process)
        self.process = self.channel = self.original_channel = None

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()
