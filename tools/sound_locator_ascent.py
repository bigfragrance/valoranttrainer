"""Controlled moving footstep emitters on Ascent; WAV and listener poses are separate from truth."""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys
import time
from datetime import datetime,timezone
import numpy as np
from scipy.io import wavfile
from sound_locator_fixture import Capture,request,read_json
from training_runtime_client import OwnedSession,ROOT
from training_settings_v2 import Settings,validate
from local_environment import project_environment

COVER=(4720.668457,-4450.544434,402.298492)
OPEN=(4448.901367,-4893.875977,402.298492)
LISTENER_CANDIDATES=[(4482.,-4268.,410.),(4815.333333,-4734.666667,410.),
    (4681.858626,-4159.,410.),(4418.,-4403.666667,410.),(5015.333333,-4534.666667,410.),COVER]

def write_json(path,value):path.write_text(json.dumps(value,ensure_ascii=False,indent=2),encoding='utf-8')

def point_at(t,phase=0.):
    # 2.6m/s along the preexisting 5.2m route; every segment is checked again by the native fixture.
    length=math.dist(COVER,OPEN);period=2*length/260
    u=((t+phase)%period)/(period/2);u=u if u<=1 else 2-u
    return [COVER[i]+u*(OPEN[i]-COVER[i]) for i in range(3)]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--device',type=int,default=34)
    p.add_argument('--duration',type=float,default=24)
    p.add_argument('--wait-export',action='store_true')
    p.add_argument('--scenario',choices=['all','moving_source','turning_listener','two_sources','translating_listener','noncollinear_listener_motion'],default='all')
    p.add_argument('--listener-route',type=Path)
    args=p.parse_args()
    listener_route=None
    if args.listener_route:
        route=read_json(args.listener_route)
        listener_route=route['points_world_cm'] if isinstance(route,dict) else route
        if len(listener_route)<2 or any(len(v)!=3 or not all(math.isfinite(x) for x in v) for v in listener_route):p.error('Invalid listener route')
    if args.scenario=='noncollinear_listener_motion' and listener_route is None:p.error('This scenario requires --listener-route')
    def listener_point_at(t):
        segments=[math.dist(a,b) for a,b in zip(listener_route,listener_route[1:])]
        length=sum(segments);distance=(t*100)%(2*length)
        if distance>length:distance=2*length-distance
        for a,b,span in zip(listener_route,listener_route[1:],segments):
            if distance<=span:
                return [a[i]+(b[i]-a[i])*distance/max(span,1e-9) for i in range(3)]
            distance-=span
        return listener_route[-1]
    output=args.output.resolve()
    if ROOT/'.local' not in output.parents or not 8<=args.duration<=60:p.error('Invalid output or duration')
    output.mkdir(parents=True,exist_ok=False)
    tag=datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
    cfg=Settings(ROOT/'.local/controller/config.json').read()
    cfg.update(mode='deathmatch',map_id='ascent',invincible=True,paused=True,total_players=2)
    cfg['practice']['return_fire']=False;cfg=validate(cfg)
    config=output/'config.json';write_json(config,cfg)
    session_dir=ROOT/'.local/training-sessions'/('sound-ascent-'+tag)
    active=session_dir/'result.active.json'
    report={'map':'ascent','scope':'Controlled original footstep handler at moving world points, not NPC walking or official-server validation',
        'active_record':str(active),'datasets':[],'passed':False}
    (output/'module-build.json').write_bytes((ROOT/'.local/native-module/build.json').read_bytes())
    runner=session=capture=None
    with (output/'runner.log').open('w',encoding='utf-8') as log:
        try:
            command=[sys.executable,'-X','utf8',str(ROOT/'tools/run_duel1_training.py'),'--prebuilt','--seconds','900',
                '--controller-settings',str(config),'--session-dir',str(session_dir),'--test-input']
            runner=subprocess.Popen(command,cwd=ROOT,stdout=log,stderr=log,creationflags=subprocess.CREATE_NO_WINDOW,env=project_environment(ROOT))
            deadline=time.monotonic()+80
            while time.monotonic()<deadline and runner.poll() is None:
                if active.exists():
                    try:
                        candidate=OwnedSession(active)
                        if candidate.channel.state().get('ready'):session=candidate;break
                        candidate.close()
                    except Exception:pass
                time.sleep(.1)
            if session is None:raise RuntimeError('Ascent owned stage9 not ready')
            probe=session.original();report['pid']=session.active['pid']
            # Give the map-export worker a V2-command-exclusive phase before any recording.
            print(json.dumps({'checkpoint':'ascent_session_ready','active':str(active),'pid':report['pid'],
                'export_marker':str(output/'export-complete.json')}),flush=True)
            if args.wait_export:
                end=time.monotonic()+600
                while not (output/'export-complete.json').exists():
                    if not session.alive() or time.monotonic()>end:raise RuntimeError('Map export handoff timed out')
                    probe.command(flags=8);time.sleep(.05)
                report['map_export']=read_json(output/'export-complete.json')
            report['listener_placement_attempts']=[]
            for position in LISTENER_CANDIDATES:
                try:
                    receipt=request(session,5,world_position=position)
                    report['listener_placement']=receipt
                    report['listener_placement_attempts'].append({'position':position,'accepted':True})
                    break
                except RuntimeError as error:
                    if 'listener_pose_ground_or_clearance_failed' not in str(error):raise
                    report['listener_placement_attempts'].append({'position':position,'accepted':False,'error':str(error)})
            else:raise RuntimeError('No candidate listener station passed original ground and capsule checks')
            report['enable']=request(session,2)
            probe.aim(pitch=0,yaw=-85);probe.command(flags=8);time.sleep(.6)
            capture=Capture(args.device);report['capture_device']=capture.info
            scenarios=['moving_source','turning_listener','two_sources','translating_listener'] if args.scenario=='all' else [args.scenario]
            for scenario in scenarios:
                folder=output/scenario;folder.mkdir()
                # Fresh orientation and a distinct background file; background is never labelled silence.
                probe.aim(pitch=0,yaw=-85);time.sleep(.5);capture.reset()
                background_begin=time.monotonic()
                while time.monotonic()-background_begin<2:
                    probe.command(flags=8);probe.aim(pitch=0,yaw=-85);time.sleep(.05)
                background,bstatus=capture.finish();wavfile.write(folder/'background.wav',capture.rate,background)
                capture.reset();start=time.monotonic();next_pose=0.;next_step=[.5,.67];first=[True,True]
                poses=[];truth=[];max_gap=0.;last_pose=None;next_translation=0.;translations=[]
                with (folder/'listener_pose.jsonl').open('w',encoding='utf-8') as pose_file,(folder/'source_truth.jsonl').open('w',encoding='utf-8') as truth_file:
                    while time.monotonic()-start<args.duration:
                        elapsed=time.monotonic()-start
                        if not session.alive():raise RuntimeError('Owned Ascent client exited')
                        yaw=-85+100*math.sin(2*math.pi*elapsed/12) if scenario=='turning_listener' else -85.
                        if scenario in ('translating_listener','noncollinear_listener_motion') and elapsed>=next_translation:
                            moving=point_at(elapsed*.5)
                            target=listener_point_at(elapsed) if scenario=='noncollinear_listener_motion' else [COVER[i]+.45*(moving[i]-COVER[i]) for i in range(3)]
                            receipt=request(session,5,yaw=yaw,world_position=target)
                            translations.append({'host_elapsed_s':elapsed,'audio_frame':capture.frame_count(),'requested_feet':target,'receipt':receipt})
                            next_translation=elapsed+.3
                        if elapsed>=next_pose:
                            probe.command(flags=8);probe.aim(pitch=0,yaw=yaw)
                            frame=probe.read();sample=capture.frame_count()
                            pose={'t_audio_s':sample/capture.rate,'audio_frame':sample,'host_elapsed_s':elapsed,
                                  'world_x':frame.camera[0],'world_y':frame.camera[1],'world_z':frame.camera[2],
                                  'yaw':frame.rotation[1],'pitch':frame.rotation[0],'actor_position':list(frame.location),
                                  'requested_yaw':yaw}
                            if last_pose is not None:max_gap=max(max_gap,elapsed-last_pose)
                            last_pose=elapsed;poses.append(pose);pose_file.write(json.dumps(pose)+'\n')
                            next_pose=elapsed+.04
                        for source_id in range(2 if scenario=='two_sources' else 1):
                            if elapsed>=next_step[source_id]:
                                point=list(OPEN) if scenario in ('turning_listener','translating_listener','noncollinear_listener_motion') else point_at(elapsed,source_id*1.0)
                                before=capture.frame_count();sent=time.monotonic()-start
                                receipt=request(session,1,4,world_position=point,source_id=source_id,reset_path=first[source_id])
                                after=capture.frame_count();first[source_id]=False
                                item={'t_audio_s':before/capture.rate,'audio_frame':before,'audio_frame_after_ack':after,
                                      'host_elapsed_s':sent,'source_id':source_id,'position':receipt['source'],
                                      'ground_feet':receipt['ground_feet'],'event':'ground_step_hardsole_wushu',
                                      'handler_accepted':receipt['handler_accepted'],'receipt':receipt}
                                truth.append(item);truth_file.write(json.dumps(item)+'\n')
                                next_step[source_id]+= .45 if source_id==0 else .48
                        time.sleep(.002)
                audio,status=capture.finish();wavfile.write(folder/'audio.wav',capture.rate,audio)
                metadata={'scenario':scenario,'audio':'audio.wav','listener_pose':'listener_pose.jsonl','truth_for_evaluation_only':'source_truth.jsonl',
                    'sample_rate':capture.rate,'samples':len(audio),'pose_samples':len(poses),'pose_target_hz':25,'pose_max_gap_seconds':max_gap,
                    'source_events':len(truth),'capture_status_flags':status,'background_status_flags':bstatus,
                    'scope':'Controlled moving world-point footsteps via original handler; actors themselves are not walking',
                    'ground_and_segment_validation':True,'audio_sha256':hashlib.sha256((folder/'audio.wav').read_bytes()).hexdigest(),
                    'listener_initial':poses[0] if poses else None,'listener_final':poses[-1] if poses else None,
                    'passed':bool(truth) and len(audio)>capture.rate and not status}
                if translations:
                    metadata['listener_motion']='Discrete original TeleportTo at ground/capsule-validated locations; not natural walking; do not infer velocity from jumps'
                    write_json(folder/'listener_teleport_commands.json',translations)
                    xy=np.array([[v['world_x'],v['world_y']] for v in poses])
                    metadata['actual_listener_baseline_cm']=float(np.max(np.linalg.norm(xy[:,None,:]-xy[None,:,:],axis=2)))
                    source=np.array(truth[0]['position'][:2])
                    bearings=np.unwrap(np.arctan2(source[1]-xy[:,1],source[0]-xy[:,0]))
                    metadata['fixed_source_world_cm']=truth[0]['position']
                    metadata['actual_source_bearing_span_deg']=float(np.ptp(bearings)*180/math.pi)
                    event_xy=np.array([v['receipt']['listener'][:2] for v in truth])
                    event_bearing=np.unwrap(np.arctan2(source[1]-event_xy[:,1],source[0]-event_xy[:,0]))
                    metadata['event_time_listener_baseline_cm']=float(np.max(np.linalg.norm(event_xy[:,None,:]-event_xy[None,:,:],axis=2)))
                    metadata['event_time_source_bearing_span_deg']=float(np.ptp(event_bearing)*180/math.pi)
                    if listener_route is not None:metadata['listener_route_world_cm']=listener_route
                write_json(folder/'metadata.json',metadata);report['datasets'].append({'directory':str(folder),**metadata})
                print(json.dumps({'checkpoint':'ascent_trajectory_recorded','directory':str(folder),'events':len(truth),'poses':len(poses)}),flush=True)
            report['passed']=all(d['passed'] for d in report['datasets'])
        except Exception as error:
            report['error']=str(error);print(json.dumps({'error':str(error),'output':str(output)}),flush=True)
        finally:
            if capture:capture.close()
            if session:
                if session.alive():
                    try:report['restore']=request(session,3)
                    except Exception as error:report['restore_error']=str(error)
                    try:session.original().release();session.stop()
                    except Exception:pass
                session.close()
            if runner:
                try:runner.wait(timeout=20)
                except subprocess.TimeoutExpired:report['runner_pending']=True
    write_json(output/'report.json',report)
    print(json.dumps({'passed':report['passed'],'output':str(output)}),flush=True)
    return 0 if report['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
