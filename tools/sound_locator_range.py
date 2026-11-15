"""Bounded Ascent footstep distance dataset; original handler, legal map points, separate holdout."""
from __future__ import annotations
import argparse
from datetime import datetime,timezone
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys
import time
import numpy as np
from scipy.io import wavfile
from local_environment import project_environment
from training_runtime_client import OwnedSession,ROOT
from training_settings_v2 import Settings,validate
from sound_locator_fixture import Capture,request,read_json

LISTENER=(4482.,-4268.,410.)

def save(path,value):path.write_text(json.dumps(value,ensure_ascii=False,indent=2),encoding='utf-8')

def candidate_points(graph,target_m,target_angle):
    nodes=np.asarray(graph['nodes_world_cm'],float)
    center=np.asarray(LISTENER)
    distances=np.linalg.norm(nodes[:,:2]-center[:2],axis=1)
    angles=np.degrees(np.arctan2(nodes[:,1]-center[1],nodes[:,0]-center[0]))
    # Use the listener's connected navigation component and the same floor band.
    start=int(np.argmin(np.linalg.norm(nodes-center,axis=1)));component={start};pending=[start]
    while pending:
        at=pending.pop()
        for other in graph['adjacency'][at]:
            if other not in component:component.add(other);pending.append(other)
    radius=target_m*100;tolerance=max(45.,radius*.12)
    mask=(abs(distances-radius)<=tolerance)&(abs(nodes[:,2]-center[2])<90)
    ids=np.array([i for i in np.flatnonzero(mask) if int(i) in component],dtype=int)
    angular=np.abs((angles[ids]-target_angle+180)%360-180)
    ids=ids[angular<=35]
    angular=np.abs((angles[ids]-target_angle+180)%360-180)
    scores=abs(distances[ids]-radius)/tolerance+angular/25
    return [{'node_id':int(i),'position':nodes[i].tolist(),'nav_distance_cm':float(distances[i]),'nav_angle_deg':float(angles[i])}
            for i in ids[np.argsort(scores)[:6]]]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--device',type=int,default=34)
    args=parser.parse_args()
    output=args.output.resolve()
    if ROOT/'.local' not in output.parents:parser.error('Output must remain in .local')
    output.mkdir(parents=True,exist_ok=False)
    tag=datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
    graph=read_json(ROOT/'sound_locator/maps/ascent.json')
    plan=[('calibration',m,a,2) for m in [2,4,6,10,15,20] for a in [-135,-30,90]]
    plan += [('holdout',m,a,1) for m in [3,5,8,12] for a in [-75,150]]
    report={'schema_version':1,'kind':'independent_optional_footstep_range_dataset','map_id':'ascent',
        'scope':'Original offline Ascent handler at validated static world points, not NPC walking or online official server',
        'output':str(output),'clips':[],'skipped':[],'point_checks':[],'passed':False,
        'settings':{'event_id':4,'surface_type':1,'perspective':'ThirdPersonEnemy','volume_multiplier':1,
                    'system_volume':'unknown; not changed by recorder','game_effects_volume':'unknown; not changed by recorder',
                    'hrtf_dsp_verified':False,'occlusion_route':'original AudioHandlerActor ground-sound path enables occlusion',
                    'numeric_occlusion':'unknown; no numeric attenuation inferred'},
        'holdout_policy':'Intermediate distances and azimuths excluded from calibration; no model is run by the recorder'}
    save(output/'plan.json',{'listener_candidate':LISTENER,'plan':plan})
    (output/'module-build.json').write_bytes((ROOT/'.local/native-module/build.json').read_bytes())
    cfg=Settings(ROOT/'.local/controller/config.json').read()
    cfg.update(mode='deathmatch',map_id='ascent',invincible=True,paused=True,total_players=2)
    cfg['practice']['return_fire']=False;cfg=validate(cfg);save(output/'config.json',cfg)
    session_dir=ROOT/'.local/training-sessions'/('sound-range-'+tag);active=session_dir/'result.active.json'
    report['active_record']=str(active)
    session=capture=runner=None
    with (output/'runner.log').open('w',encoding='utf-8') as log:
        try:
            command=[sys.executable,'-X','utf8',str(ROOT/'tools/run_duel1_training.py'),'--prebuilt','--seconds','600',
                '--controller-settings',str(output/'config.json'),'--session-dir',str(session_dir),'--test-input']
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
            if session is None:raise RuntimeError('Ascent stage9 did not reach ready')
            report['pid']=session.active['pid'];probe=session.original()
            report['listener_placement']=request(session,5,world_position=LISTENER)
            report['hrtf_before']=request(session);report['hrtf_enable']=request(session,2)
            def keepalive():probe.command(flags=8);probe.aim(pitch=0,yaw=0)
            def settle(seconds):
                until=time.monotonic()+seconds
                while time.monotonic()<until:keepalive();time.sleep(.04)
            settle(.8);report['initial_frame']=probe.read().as_dict()
            capture=Capture(args.device);report['capture_device']=capture.info
            capture.reset();settle(2);background,status=capture.finish()
            wavfile.write(output/'background.wav',capture.rate,background)
            report['background_status_flags']=status
            print(json.dumps({'checkpoint':'range_session_ready','output':str(output),'pid':report['pid']}),flush=True)
            for split,target_m,target_angle,repeats in plan:
                accepted=None;checks=[]
                for candidate in candidate_points(graph,target_m,target_angle):
                    try:
                        # A bounded point probe plays one original step outside the dataset clip.
                        receipt=request(session,1,4,world_position=candidate['position'],reset_path=True)
                        valid_floor=abs(receipt['ground_feet'][2]-candidate['position'][2])<100
                        checks.append({**candidate,'accepted':valid_floor,'receipt':receipt})
                        if valid_floor:accepted=candidate;break
                    except RuntimeError as error:
                        if any(x in str(error) for x in ['source_ground_or_clearance_failed','source_path_segment_blocked']):
                            checks.append({**candidate,'accepted':False,'error':str(error)})
                        else:raise
                report['point_checks'].append({'split':split,'target_m':target_m,'target_angle':target_angle,'checks':checks})
                if accepted is None:
                    report['skipped'].append({'split':split,'target_m':target_m,'target_angle':target_angle,'reason':'No candidate passed original ground/capsule and floor checks'})
                    save(output/'recordings.json',report);continue
                settle(.7)
                for repeat in range(repeats):
                    settle(.4);capture.reset();start=time.monotonic();receipts=[];poses=[];next_pose=0
                    while time.monotonic()-start<2.05:
                        elapsed=time.monotonic()-start;keepalive()
                        if elapsed>=next_pose:
                            frame=probe.read();poses.append({'t_audio_s':capture.frame_count()/capture.rate,
                                'position_cm':list(frame.camera),'yaw_deg':frame.rotation[1],'pitch_deg':frame.rotation[0],
                                'actor_position':list(frame.location)})
                            next_pose=elapsed+.04
                        if len(receipts)<3 and elapsed>=.4+len(receipts)*.55:
                            before=capture.frame_count();offset=time.monotonic()-start
                            receipt=request(session,1,4,world_position=accepted['position'],reset_path=not receipts)
                            receipt.update(command_start_offset_seconds=offset,host_offset_seconds=time.monotonic()-start,
                                capture_frames_before_post=before,capture_frames_after_ack=capture.frame_count())
                            receipts.append(receipt)
                        time.sleep(.002)
                    audio,status=capture.finish();frame=probe.read().as_dict()
                    ordinal=len(report['clips']);name=f'{split}_{ordinal:03d}_target{target_m:02d}m_angle{target_angle:+04d}_rep{repeat}.wav'
                    wavfile.write(output/name,capture.rate,audio)
                    actual_distance=float(np.median([x['distance_cm'] for x in receipts]));actual_angle=float(np.median([x['angle_deg'] for x in receipts]))
                    row={'wav':name,'split':split,'event_id':4,'target_distance_m':target_m,'target_angle_deg':target_angle,
                        'distance_cm':actual_distance,'angle_deg':actual_angle,'repeat':repeat,'node_id':accepted['node_id'],
                        'receipts':receipts,'listener_pose':poses,'frame_after':frame,'sample_rate':capture.rate,'samples':len(audio),
                        'pre_event_background_seconds':.3,'capture_status_flags':status,'peak':float(np.max(abs(audio))),
                        'rms_lr':np.sqrt(np.mean(audio.astype(float)**2,axis=0)).tolist(),
                        'sha256':hashlib.sha256((output/name).read_bytes()).hexdigest()}
                    report['clips'].append(row);save(output/'recordings.json',report)
                    print(json.dumps({'checkpoint':'range_clip','split':split,'file':name,'distance_cm':actual_distance,'angle_deg':actual_angle,'events':len(receipts)}),flush=True)
            report['passed']=bool(report['clips']) and all(len(c['receipts'])==3 and not c['capture_status_flags'] and c['samples']>capture.rate for c in report['clips'])
            report['pass_definition']='Three original handler receipts and valid stereo capture per clip; not range accuracy or audibility of every distant step'
        except Exception as error:
            report['error']=str(error);print(json.dumps({'error':str(error)}),flush=True)
        finally:
            if capture:capture.close()
            if session:
                if session.alive():
                    try:report['hrtf_restore']=request(session,3)
                    except Exception as error:report['restore_error']=str(error)
                    try:session.original().release();session.stop()
                    except Exception:pass
                session.close()
            if runner:
                try:runner.wait(timeout=20)
                except subprocess.TimeoutExpired:report['runner_pending']=True
    save(output/'recordings.json',report)
    print(json.dumps({'output':str(output),'passed':report['passed'],'clips':len(report['clips']),'skipped':len(report['skipped'])}),flush=True)
    return 0 if report['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
