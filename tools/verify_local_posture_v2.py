"""Observe the real local capsule and camera while NPCs change stance."""
from __future__ import annotations

import copy
import time

from training_control import pack_configuration
from training_runtime_client import OwnedSession
from verify_combat_v2 import snapshot


def verify(active, config):
    report = {'passed': False, 'phases': {}, 'phase_diagnostics': {}, 'camera_events': [], 'modifier_windows': []}
    with OwnedSession(active) as session:
        probe = session.original()
        try:
            current = copy.deepcopy(config)
            current.update(paused=True, invincible=True)
            current['practice']['return_fire'] = True
            current['revision'] += 300
            session.command('configure', pack_configuration(current))
            next_native = 0
            saved_events = set()
            for name, seconds, crouch, paused in [('npc_paused', 3, False, True), ('no_local_crouch', 12, False, False), ('explicit_local_crouch', 1, True, False), ('released_local_crouch', 2, False, False)]:
                current.update(paused=paused)
                current['revision'] += 1
                session.command('configure', pack_configuration(current))
                rows = []
                if name == 'no_local_crouch':
                    report['before_automatic_landing'] = snapshot('training-landing-shake-probe')
                end = time.monotonic() + seconds
                previous = None
                while time.monotonic() < end:
                    if not session.alive():
                        raise RuntimeError('Owned client exited during posture observation')
                    probe.command(flags=8, crouch=crouch)
                    frame = probe.read()
                    if frame.time != previous:
                        rows.append({'time': frame.time, 'held': bool(frame.flags & 8),
                            'capsule': frame.capsule_half_height, 'camera': frame.camera,
                            'location': frame.location, 'camera_relative_z': frame.camera[2]-frame.location[2],
                            'movement_type': frame.movement_type, 'input_flags': frame.input_flags})
                        previous = frame.time
                    if name == 'no_local_crouch' and time.monotonic() >= next_native:
                        next_native = time.monotonic() + .2
                        diagnostics = snapshot('training-local-posture-probe')
                        modifiers = diagnostics.get('pcm_modifiers')
                        if modifiers:
                            report['modifier_windows'].append(modifiers)
                        native_rows = diagnostics['samples']
                        low = {p['sequence'] for p in native_rows if p['observation']['raw_held_1538'] == 0
                            and p['observation']['capsule_half_height'] >= 97.9
                            and min(p['observation']['follow_position'][2], p['observation']['pcm_position'][2])-p['eyes'][2] < -8}
                        near = {s+d for s in low for d in (-2, -1, 0, 1, 2)}
                        for p in native_rows:
                            if p['sequence'] in near and p['sequence'] not in saved_events and len(report['camera_events']) < 512:
                                report['camera_events'].append(p)
                                saved_events.add(p['sequence'])
                    time.sleep(.02)
                report['phases'][name] = rows
                report['phase_diagnostics'][name] = snapshot('training-local-posture-probe')
                if name == 'no_local_crouch':
                    report['after_automatic_landing'] = snapshot('training-landing-shake-probe')
            report['own_jump'] = []
            before_jump = probe.read()
            before_landings = snapshot('training-landing-shake-probe')['landing_calls']
            began = time.monotonic()
            while time.monotonic()-began < 2:
                probe.command(flags=8, jump=time.monotonic()-began < .15)
                frame = probe.read()
                report['own_jump'].append({'time': frame.time, 'z': frame.location[2], 'camera_z': frame.camera[2]})
                time.sleep(.03)
            probe.command(flags=8)
            report['after_own_jump'] = snapshot('training-landing-shake-probe')
            automatic = report['phases']['no_local_crouch'][25:]
            pressed = report['phases']['explicit_local_crouch'][15:]
            released = report['phases']['released_local_crouch'][25:]
            report['bot_diagnostics'] = snapshot('training-bot-controller')
            report['native_camera'] = snapshot('original-camera')
            report['checks'] = {
                'no_automatic_local_crouch': bool(automatic) and not any(p['held'] for p in automatic),
                'standing_capsule_stable': bool(automatic) and max(p['capsule'] for p in automatic)-min(p['capsule'] for p in automatic) < .05,
                'standing_camera_stable': bool(automatic) and max(p['camera_relative_z'] for p in automatic)-min(p['camera_relative_z'] for p in automatic) < 8,
                'explicit_native_crouch_works': bool(pressed) and any(p['held'] for p in pressed),
                'native_crouch_releases': bool(released) and not any(p['held'] for p in released),
                'standing_capsule_restored': bool(released) and abs(released[-1]['capsule']-automatic[0]['capsule']) < .05}
            report['checks']['no_foreign_landing_shakes'] = report['before_automatic_landing']['landing_calls'] == report['after_automatic_landing']['landing_calls']
            report['checks']['own_native_jump_and_landing_preserved'] = max(p['z'] for p in report['own_jump'])-before_jump.location[2] > 50 and report['after_own_jump']['landing_calls'] > before_landings
            report['passed'] = all(report['checks'].values())
            if not report['passed']:
                raise RuntimeError('Local posture changed without matching local crouch input')
        except Exception as error:
            report['error'] = str(error)
        finally:
            probe.release()
    return report
