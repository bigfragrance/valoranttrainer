"""Read actual locality/audio selection and settled original body transforms."""
from __future__ import annotations

import copy
import time

from training_control import pack_configuration
from training_runtime_client import OwnedSession
from verify_combat_v2 import aim_at, snapshot


def angle(value):
    return abs((value + 180) % 360 - 180)


def verify(active, config):
    report = {'passed': False, 'samples': []}
    with OwnedSession(active) as session:
        probe = session.original()
        try:
            current = copy.deepcopy(config)
            current.update(paused=True, invincible=True)
            current['revision'] += 200
            session.command('configure', pack_configuration(current))
            session.wait(lambda s: s.get('ready') and s['player']['alive'])
            for _ in range(3):
                document = snapshot('training-player-roles')
                if document['pid'] != session.active['pid']:
                    raise RuntimeError('Role snapshot belongs to another client')
                report['samples'].append(document)
                time.sleep(.6)
            checks = {}
            for document in report['samples']:
                rows = document['players']
                checks['one_actual_local'] = document['actual_local_count'] == document['local_player_count'] == 1
                # In Standalone, the original authority simulation legitimately
                # treats NPCs as locally controlled. Human view ownership must
                # use the actual LocalPlayer links rather than this predicate.
                checks['original_npc_simulation_ownership'] = all(p['locality_query'] and p['effective_locally_controlled'] for p in rows if not p['local'] and p['alive'])
                checks['one_first_person_sound'] = sum(p['wwise_perspective'] == 'FirstPerson' for p in rows) == 1
                checks['npc_third_person_sound'] = all(p['wwise_perspective'].startswith('ThirdPerson') for p in rows if not p['local'])
                checks['one_original_listener'] = document['listener_query'] and document['listener_is_actual_local']
                checks['npc_sound_circles_empty'] = all(p['attenuation_query'] and p['one_shot_radius'] == 0 and p['constant_visualizations'] == 0 for p in rows if not p['local'])
                if not all(checks.values()):
                    raise RuntimeError('Actual view/audio ownership or original NPC simulation selection is incorrect')
            # Freeze only the normal AI input through its public configuration.
            # Give original scene/animation transform propagation time to settle.
            report['local_movement'] = []
            until = time.monotonic() + 2.2
            last_sample = report['samples'][-1]['samples']
            while time.monotonic() < until:
                probe.command(flags=8, forward=1)
                document = snapshot('training-player-roles')
                if document['samples'] != last_sample:
                    report['local_movement'].append(document)
                    last_sample = document['samples']
                time.sleep(.05)
            probe.command(flags=8)
            checks['local_sound_circle_preserved'] = any(p['local'] and p['attenuation_query'] and
                (p['one_shot_radius'] > 0 or p['maximum_constant_radius'] > 0) for d in report['local_movement'] for p in d['players'])
            checks['npc_sound_circles_empty_while_self_moves'] = bool(report['local_movement']) and all(p['attenuation_query'] and p['one_shot_radius'] == 0 and p['constant_visualizations'] == 0
                for d in report['local_movement'] for p in d['players'] if not p['local'])
            current['revision'] += 1
            session.command('configure', pack_configuration(current))
            state = session.channel.state()
            target = next((p for p in state['players'] if p['bot'] and p['alive']), None)
            if target:
                aim_at(probe, (*target['feet'][:2], target['feet'][2]+120))
            before = last_sample
            deadline = time.monotonic() + 5
            while time.monotonic() < deadline:
                probe.command(flags=8)
                document = snapshot('training-player-roles')
                if document['samples'] >= before + 2:
                    break
                time.sleep(.1)
            report['settled'] = document
            bodies = [p for p in document['players'] if not p['local'] and p['alive']]
            # Original hidden-mesh optimization can deliberately freeze cached
            # child transforms. Only visible, propagating bodies are a visual
            # transform acceptance surface; every living Actor still must aim.
            visible = [p for p in bodies if p['cosmetic_absolute_flags_1d0'] & 0x20
                       and not p['cosmetic_visibility_flags_251'] & 2
                       and not (p['core_scene_flags_250'] | p['cosmetic_scene_flags_250']) & 0x20
                       and not p['root_scene_flags_250'] & 0x80
                       and not (p['core_absolute_flags_1d0'] | p['cosmetic_absolute_flags_1d0']) & 8]
            report['body_errors'] = [{'index': p['index'], 'actor_control': p['actor_control_yaw_error'],
                                      'core_actor': angle(p['core_rotation'][1]-p['actor_rotation'][1]-p['core_relative_rotation'][1]),
                                      'cosmetic_actor': angle(p['cosmetic_rotation'][1]-p['core_rotation'][1]-p['cosmetic_relative_rotation'][1]),
                                      'visible_and_propagating': p in visible} for p in bodies]
            checks['native_body_control_rotation'] = bool(bodies) and all(p['actor_control_yaw_error'] < .05 for p in bodies)
            report['visible_original_bodies'] = len(visible)
            checks['settled_visible_original_mesh_rotation'] = bool(visible) and all(p['core_query'] and p['cosmetic_query'] and
                   angle(p['core_rotation'][1]-p['actor_rotation'][1]-p['core_relative_rotation'][1]) < .05 and
                   angle(p['cosmetic_rotation'][1]-p['core_rotation'][1]-p['cosmetic_relative_rotation'][1]) < .05 for p in visible)
            report['checks'] = checks
            report['passed'] = all(checks.values())
            if not report['passed']:
                raise RuntimeError('Original facing/mesh transforms did not settle to actual aim')
        except Exception as error:
            report['error'] = str(error)
        finally:
            probe.release()
    return report
