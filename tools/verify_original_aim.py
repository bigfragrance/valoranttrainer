"""Compare actual native shots after deliberate, isolated controller turns."""
import argparse
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import subprocess
import sys
import time

from original_probe_channel import OriginalProbe
from run_map_probe import prepare
from run_native_probe import ROOT, save
from training_channel import ProcessWatch, TrainingChannel
from verify_original_jett import Observations, read_json


def direction(pitch, yaw):
    p, y = math.radians(pitch), math.radians(yaw)
    return math.cos(p)*math.cos(y), math.cos(p)*math.sin(y), math.sin(p)


def angle(a, b):
    length = math.sqrt(sum(x*x for x in a)*sum(x*x for x in b))
    if length < .001:
        return None
    return math.degrees(math.acos(max(-1., min(1., sum(x*y for x, y in zip(a, b))/length))))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--controls-only', action='store_true', help='Check native ADS and the infinite-ammo toggle')
    args = parser.parse_args()
    category = 'original-controls-verification' if args.controls_only else 'original-aim-verification'
    folder = ROOT/'.local'/category/datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
    folder.mkdir(parents=True)
    prepared = prepare(ROOT, 1, 'navigation', folder/'config.bin')
    result = folder/'run.json'
    summary = {'passed': False, 'kind': 'original_aim_turn_check', 'os_input_injection': False,
               'native_physical_input_disabled_for_test': True, 'projectile_directions_replaced': False}
    channel = watch = observations = None
    run_dir = None
    with (folder/'runner.log').open('w', encoding='utf-8') as log:
        runner = subprocess.Popen([sys.executable, str(ROOT/'tools/run_native_baseline.py'), '--run',
            '--seconds', '70', '--bootstrap', prepared['archive'], '--map', '1', '--mode', 'navigation',
            '--observe-world', '--training-module', '--module-stage', '9', '--original-game-instance',
            '--original-visual-classes', '--no-training-hud', '--result-file', str(result)],
            cwd=ROOT, stdout=log, stderr=log, creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            deadline = time.monotonic()+35
            active_file = result.with_suffix('.active.json')
            while time.monotonic() < deadline and runner.poll() is None:
                active = read_json(active_file)
                if active.get('pid'):
                    break
                time.sleep(.05)
            else:
                raise RuntimeError('Owned native lifecycle did not become available')
            run_dir = Path(active['run_dir'])
            watch = ProcessWatch(active['pid'])
            channel = TrainingChannel.open(active['pid'], alive=watch.alive, timeout=35)
            probe = OriginalProbe(channel)
            observations = Observations(probe, watch, active['pid'], active_file, folder)
            deadline = time.monotonic()+12
            while not probe.read().flags & 1:
                if time.monotonic() > deadline:
                    raise RuntimeError('Native rifle did not finish equipping')
                time.sleep(.025)
            checks = []
            for pitch, yaw in (() if args.controls_only else ((0, 0), (12, 45), (-15, -90), (30, 160))):
                name = f'pitch_{pitch}_yaw_{yaw}'
                settle = observations.phase(name+'_settle', .65, pitch=pitch, yaw=yaw, flags=24)
                before = settle[-1]['shot_count']
                fire = observations.phase(name+'_fire', .08, pitch=pitch, yaw=yaw, flags=25)
                recovery = observations.phase(name+'_recover', .9, pitch=pitch, yaw=yaw, flags=24)
                shots = [s for s in recovery[-1]['shots'] if s['sequence'] > before]
                first = min(shots, key=lambda s: s['sequence'], default=None)
                error = angle(first['direction'], direction(pitch, yaw)) if first else None
                actual_camera = settle[-1]['rotation']
                camera_error = angle(direction(*actual_camera[:2]), direction(pitch, yaw))
                checks.append({'pitch': pitch, 'yaw': yaw, 'native_shots': len(shots),
                               'first_native_shot': first, 'direction_error_degrees': error,
                               'camera_after_settle': actual_camera,
                               'camera_direction_error_degrees': camera_error,
                               'passed': bool(first and first['flags'] & 8 and error is not None and error < 2.
                                              and camera_error is not None and camera_error < 1.)})
            if args.controls_only:
                hip = observations.phase('hip', .7, flags=24)[-1]
                ads = observations.phase('native_ads_hold', 1.3, flags=28)
                observations.phase('native_ads_release', .3, flags=24)
                before = observations.phase('infinite_on_press', .08, flags=56)[-1]
                observations.phase('infinite_on_release', .15, flags=24)
                on = observations.phase('infinite_fire', 3.2, flags=25)
                on += observations.phase('infinite_fire_release', .25, flags=24)
                enabled = read_json(ROOT/'.local/native-module/original-infinite-ammo.json')
                observations.phase('infinite_off_press', .08, flags=56)
                observations.phase('infinite_off_release', .15, flags=24)
                off = observations.phase('finite_fire', .3, flags=25)
                off += observations.phase('finite_fire_release', .4, flags=24)
                disabled = read_json(ROOT/'.local/native-module/original-infinite-ammo.json')
                observations.phase('infinite_reenable_press', .08, flags=56)
                again = observations.phase('infinite_reenable_release', .7, flags=24)
                restored = read_json(ROOT/'.local/native-module/original-infinite-ammo.json')
                camera = read_json(ROOT/'.local/native-module/original-camera.json')
                scope = read_json(ROOT/'.local/native-module/original-scope.json')
                assertions = {
                    'bootstrap_fov_unlocked': camera.get('fov_unlocked_current') is True,
                    'original_ads_changes_fov': min(r['field_of_view'] for r in ads) < hip['field_of_view'] - .5,
                    'infinite_fires_more_than_one_magazine': on[-1]['shot_count'] - before['shot_count'] > 25,
                    'infinite_keeps_native_ammo_full': all(r['ammo'] == r['max_ammo'] for r in on),
                    'original_infinite_rule_enabled': enabled.get('native_ammo_rule') is True,
                    'original_infinite_rule_disabled': disabled.get('native_ammo_rule') is False,
                    'finite_firing_consumes_ammo': min(r['ammo'] for r in off) < off[0]['max_ammo'],
                    'reenable_replenishes_with_original_resource': again[-1]['ammo'] == again[-1]['max_ammo']
                        and restored.get('replenish_calls') == 1,
                    'original_scope_alignment_generated': bool(scope.get('scope_ready'))
                        and not scope.get('scope_transform_identity', True),
                    'original_animation_consumed_scope_alignment': bool(scope.get('animation_consumed'))
                        and not scope.get('gun_to_scope_identity', True),
                }
                summary['controls'] = {'assertions': assertions, 'hip_fov': hip['field_of_view'],
                    'minimum_ads_fov': min(r['field_of_view'] for r in ads),
                    'infinite_shots': on[-1]['shot_count'] - before['shot_count'],
                    'enabled': enabled, 'disabled': disabled, 'restored': restored, 'scope': scope}
            summary.update(checks=checks, passed=all(c['passed'] for c in checks)
                           and (not args.controls_only or all(summary['controls']['assertions'].values())))
            save(folder/'samples.json', {'samples': observations.samples})
        except Exception as error:
            summary['error'] = str(error)
            if observations is not None:
                summary['hang_capture'] = observations.hang_capture
                save(folder/'samples.json', {'samples': observations.samples})
        finally:
            if channel is not None:
                try:
                    OriginalProbe(channel).release()
                except Exception:
                    pass
                channel.close()
            if run_dir is not None:
                save(run_dir/'stop-request.json', {'reason': 'native_aim_verification_complete'})
            try:
                runner.wait(timeout=10)
            except subprocess.TimeoutExpired:
                runner.terminate(); runner.wait(timeout=10)
            if watch is not None:
                watch.close()
    summary['run'] = read_json(result)
    save(folder/'verification.json', summary)
    print(json.dumps({'passed': summary['passed'], 'error': summary.get('error'),
                      'checks': summary.get('checks'),
                      'controls': summary.get('controls', {}).get('assertions'),
                      'artifact': str(folder/'verification.json')}, ensure_ascii=False))
    return 0 if summary['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
