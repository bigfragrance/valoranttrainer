"""Exercise original movement and AK state machines in a bounded owned client."""
from datetime import datetime, timezone
import argparse
import json
import math
import statistics
from pathlib import Path
import subprocess
import sys
import time
from build_native_module import build
from original_probe_channel import OriginalProbe
from run_map_probe import prepare
from run_native_probe import ROOT, save
from training_channel import ChannelNotReady, ProcessWatch, TrainingChannel


def read_json(path):
    try:
        return json.loads(path.read_text(encoding='utf-8'))
    except (OSError, ValueError):
        return {}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--skip-build', action='store_true')
    parser.add_argument('--only-fire', action='store_true',help='Isolate original firing transport without movement phases')
    parser.add_argument('--cycles', type=int, default=1, help='Repeat actual fire/reload cycles to exercise native effect lifetimes')
    args = parser.parse_args()
    if not 1 <= args.cycles <= 10:
        parser.error('cycles must be between 1 and 10')
    if not args.skip_build:
        build()
    folder = ROOT / '.local/original-player-verification' / datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
    folder.mkdir(parents=True)
    prepared = prepare(ROOT, 1, 'navigation', folder/'config.bin')
    result = folder/'run.json'
    summary = {'kind': 'original_player_runtime_check', 'passed': False,
               'python_weapon_simulation': False, 'os_input_injection': False}
    channel = watch = None
    samples = []
    run_dir = None
    with (folder/'runner.log').open('w', encoding='utf-8') as log:
        runner = subprocess.Popen([sys.executable, str(ROOT/'tools/run_native_baseline.py'), '--run',
            '--seconds', '100', '--bootstrap', prepared['archive'], '--map', '1', '--mode', 'navigation',
            '--observe-world', '--training-module', '--module-stage', '7', '--original-game-instance', '--original-visual-classes',
            '--no-training-hud', '--result-file', str(result)], cwd=ROOT, stdout=log, stderr=log,
            creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            deadline = time.monotonic()+35
            while time.monotonic()<deadline and runner.poll() is None:
                active=read_json(result.with_suffix('.active.json'))
                if active.get('pid'):
                    break
                time.sleep(.05)
            else:
                raise RuntimeError('Original-client lifecycle was not published')
            run_dir=Path(active['run_dir'])
            watch=ProcessWatch(active['pid'])
            channel=TrainingChannel.open(active['pid'],alive=watch.alive,timeout=35)
            probe=OriginalProbe(channel)
            deadline=time.monotonic()+12
            while not probe.read().flags&1:
                if time.monotonic()>deadline:
                    raise RuntimeError('Original AK did not become equipped')
                time.sleep(.025)

            def phase(name, seconds, **command):
                until=time.monotonic()+seconds
                collected=[]
                last_time=-1.
                last_changed=time.monotonic()
                while time.monotonic()<until and watch.alive():
                    probe.command(**command)
                    frame=probe.read()
                    if frame.time!=last_time:
                        last_time=frame.time;last_changed=time.monotonic()
                    elif time.monotonic()-last_changed>1.5:
                        from capture_owned_thread import capture_owned_thread
                        summary['hang_capture'] = capture_owned_thread(result.with_suffix('.active.json'),
                            folder/'hang-thread.json', reason=name)
                        raise RuntimeError('Original game stopped publishing fresh samples during '+name)
                    item={'phase':name,**frame.as_dict()}
                    samples.append(item);collected.append(item)
                    if frame.observer_error:
                        raise RuntimeError('Original shot observer rejected payload')
                    time.sleep(.015)
                if not collected:
                    raise RuntimeError('Original-client phase ended without samples: '+name)
                print(json.dumps({'phase':name,'ammo':collected[-1]['ammo'],
                    'physics_calls':collected[-1]['physics_calls'],
                    'native_rays':collected[-1]['native_rays']},ensure_ascii=False),flush=True)
                return collected

            idle=phase('settle',.6)
            if not args.only_fire:
                run=phase('rifle_run',.85,forward=1.)
                phase('return_run',.85,forward=-1.)
                walk=phase('rifle_walk',.85,forward=1.,walk=True)
                phase('return_walk',.85,forward=-1.,walk=True)
                crouch=phase('crouch',.8,crouch=True)
                crouch_move=phase('rifle_crouch_move',.6,forward=1.,crouch=True)
                phase('return_crouch',.6,forward=-1.,crouch=True)
                stand=phase('stand',.7)
                jump=phase('jump_press',.25,jump=True)
                jump+=phase('jump_flight',1.45)
            firing=[];recovery=[];reload=[]
            for cycle in range(args.cycles):
                prefix=f'cycle_{cycle+1}_' if args.cycles>1 else ''
                firing+=phase(prefix+'original_ak_fire',1.15,flags=9)
                recovery+=phase(prefix+'original_recoil_recovery',.8)
                phase(prefix+'original_reload_press',.15,flags=10)
                reload+=phase(prefix+'original_reload_complete',4.)
            probe.release()
            def max_speed(data):
                return max(math.hypot(x['velocity'][0],x['velocity'][1]) for x in data)
            def steady_speed(data):
                return statistics.median(math.hypot(*x['velocity'][:2]) for x in data[len(data)//2:])
            def reaches_original_speed(data):
                return any(x['maximum_speed']>0 and .85*x['maximum_speed']<=math.hypot(*x['velocity'][:2])<=1.08*x['maximum_speed'] for x in data)
            ammo_used=idle[-1]['ammo']-min(x['ammo'] for x in firing+recovery)
            peak_recoil=max(abs(x['pitch_recoil'])+abs(x['yaw_recoil']) for x in firing+recovery)
            assertions={
                'original_physics_advanced':reload[-1]['physics_calls']>idle[0]['physics_calls'],
                'original_ammo_consumed':ammo_used>=5,
                'original_recoil_nonzero':peak_recoil>.1,
                'native_shot_directions_observed':recovery[-1]['native_rays']>=ammo_used>0,
                'native_world_shot_origins_valid':bool(recovery[-1]['shots']) and all(
                    math.dist(shot['start'],min(firing+recovery,key=lambda f:abs(f['time']-shot['time']))['camera'])<200
                    for shot in recovery[-1]['shots']),
                'original_camera_recoil_observed':max(abs(x['rotation'][0]-firing[0]['rotation'][0]) for x in firing+recovery)>.1,
                'original_primary_input_consumed':any(x['original_input_observed']&1 for x in firing+recovery),
                'original_gun_anim_instance_present':any(x['original_gun_anim_instance'] for x in firing),
                'original_reload_restores_magazine':ammo_used>0 and reload[-1]['ammo']==reload[-1]['max_ammo']>0,
            }
            measured=dict(ammo_used=ammo_used,peak_recoil=peak_recoil,fire_reload_cycles=args.cycles)
            if not args.only_fire:
                jump_height=max(x['location'][2] for x in jump)-stand[-1]['location'][2]
                expected_height=idle[-1]['original_jump_height']
                assertions.update({
                'original_run_speed_reached':reaches_original_speed(run),
                'original_walk_speed_reached':reaches_original_speed(walk),
                'original_crouch_speed_reached':reaches_original_speed(crouch_move),
                'original_walk_state_observed':any(x['movement_type']==0 and x['flags']&4 for x in walk),
                'original_crouch_state_observed':any(x['movement_type']==3 and x['flags']&8 for x in crouch_move),
                'walk_slower_than_run':steady_speed(walk)<steady_speed(run)*.9,
                'crouch_changes_original_capsule':min(x['capsule_half_height'] for x in crouch)<idle[-1]['original_capsule_half_height']*.9,
                'stand_restores_original_capsule':abs(stand[-1]['capsule_half_height']-idle[-1]['original_capsule_half_height'])<.5,
                'jump_reaches_original_height':expected_height>0 and .85*expected_height<=jump_height<=1.15*expected_height,
                'camera_follows_original_jump':max(x['camera'][2] for x in jump)-stand[-1]['camera'][2]>.8*expected_height,
                'camera_follows_original_movement':abs(run[-1]['camera'][0]-run[0]['camera'][0])>200,
                })
                measured.update(run_speed=max_speed(run),walk_speed=steady_speed(walk),crouch_speed=steady_speed(crouch_move),
                                jump_height=jump_height,original_jump_height=expected_height)
            summary.update(assertions=assertions,passed=all(assertions.values()),
                movement_tested=not args.only_fire,measured=measured,
                original_last_frame=reload[-1])
        except Exception as error:
            summary['error']=str(error)
        finally:
            if channel is not None:
                try:
                    OriginalProbe(channel).release()
                except Exception:
                    pass
                channel.close()
            if run_dir is not None:
                save(run_dir/'stop-request.json',{'reason':'original_player_verification_complete'})
            try:
                runner.wait(timeout=10)
            except subprocess.TimeoutExpired:
                runner.terminate();runner.wait(timeout=10)
            if watch is not None:
                watch.close()
    summary['run']=read_json(result)
    save(folder/'samples.json',{'samples':samples})
    save(folder/'verification.json',summary)
    print(json.dumps({'passed':summary['passed'],'assertions':summary.get('assertions'),
                      'error':summary.get('error'),'artifact':str(folder/'verification.json')},ensure_ascii=False))
    return 0 if summary['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
