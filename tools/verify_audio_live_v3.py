"""Verify native audio ownership in one explicitly owned stage9 test session."""
from __future__ import annotations

import copy
import json
import struct
import time

from training_control import pack_configuration
from training_runtime_client import OwnedSession, ROOT
from verify_audio_snapshot_v3 import audit
from verify_combat_v2 import snapshot


def verify(active, config):
    report = {'passed': False, 'acoustic_acceptance': False}
    with OwnedSession(active) as session:
        initial = session.wait(lambda s: s.get('ready'))
        current = copy.deepcopy(config)
        current.update(paused=False, invincible=True)
        current['revision'] = max(current['revision'], initial.get('applied_revision', 0)) + 1
        session.command('configure', pack_configuration(current))
        before = snapshot('original-npc-locality')
        started = time.monotonic()
        deadline = started + 45
        samples = []
        audio_observed = False
        try:
            while time.monotonic() < deadline:
                state = session.channel.state()
                if state.get('error'):
                    raise RuntimeError(state['error'])
                listeners = snapshot('original-audio-listeners')
                if listeners.get('error'):
                    raise RuntimeError(listeners['stage'])
                locality = snapshot('original-npc-locality')
                sample = {'elapsed': round(time.monotonic()-started, 3),
                          'registry': listeners.get('current', {}),
                          'native_removals': listeners.get('native_completed'),
                          'direct_audio_calls': locality.get('audio_frame_returns'),
                          'spectated_audio_calls': locality.get('spectated_view_returns'),
                          'sound_scope_calls': locality.get('sound_direct_returns', 0) + locality.get('sound_view_helper_returns', 0),
                          'kills': sum(p.get('kills', 0) for p in state.get('players', []))}
                if not samples or sample['elapsed']-samples[-1]['elapsed'] >= 1:
                    samples.append(sample)
                try:
                    result = audit(ROOT/'.local/native-module', session.active['pid'])
                except (FileNotFoundError, PermissionError, json.JSONDecodeError):
                    time.sleep(.05)
                    continue
                report['registry_audit'] = result
                audio_observed = sample['sound_scope_calls'] > (
                    before.get('sound_direct_returns', 0) + before.get('sound_view_helper_returns', 0))
                if time.monotonic()-started >= 15 and result['passed'] and audio_observed and sample['kills'] > 0:
                    report['registration_probes'] = [session.command('test_audio_registration', struct.pack('<I', i))
                                                      for i in range(1, min(4, len(state['players'])))]
                    report['registration_gate_exercised'] = any(r['code'] == 0 for r in report['registration_probes'])
                    report['passed'] = True
                    break
                time.sleep(.2)
            report['checks'] = {'registry_and_routing_audit': report.get('registry_audit', {}).get('passed', False),
                                'original_registration_gate_exercised': report.get('registration_gate_exercised', False),
                                'original_npc_audio_consumer_exercised': audio_observed if samples else False,
                                'native_combat_continued': bool(samples and samples[-1]['kills'] > 0)}
            report['passed'] = all(report['checks'].values())
            if not report['passed']:
                raise RuntimeError('Native audio ownership/routing did not reach its checkpoints')
        except Exception as error:
            report['error'] = str(error)
        finally:
            report['samples'] = samples
            report['listeners_final'] = snapshot('original-audio-listeners')
            report['locality_final'] = snapshot('original-npc-locality')
            if session.alive():
                current['paused'] = True
                current['revision'] = max(current['revision'], session.channel.state().get('applied_revision', 0)) + 1
                session.command('configure', pack_configuration(current))
    return report
