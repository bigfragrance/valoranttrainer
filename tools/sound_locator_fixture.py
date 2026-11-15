"""Explicit owned stage9 native audio recording. Never logs in or targets another process."""
from __future__ import annotations
import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess
import sys
import threading
import time

import numpy as np
from scipy.io import wavfile
import pyaudiowpatch as pa

from local_environment import project_environment
from training_control import COMMANDS
from training_runtime_client import OwnedSession, ROOT
from training_settings_v2 import Settings, validate

COMMANDS['sound_locator_fixture'] = 19
RECEIPT = ROOT/'.local/native-module/sound-locator-fixture.json'

def read_json(path):
    for _ in range(20):
        try:
            return json.loads(path.read_text(encoding='utf-8-sig'))
        except (OSError, ValueError):
            time.sleep(.02)
    raise RuntimeError('Cannot read '+str(path))

def request(session, operation=0, event=0, angle=0., distance=1000., height=0., yaw=0., world_position=None,source_id=0,reset_path=False):
    previous=read_json(RECEIPT) if RECEIPT.exists() else {}
    before=previous.get('sequence',0) if previous.get('pid')==session.active['pid'] else 0
    payload=struct.pack('<II4f',operation,event,angle,distance,height,yaw)
    if world_position is not None:payload+=struct.pack('<3dII',*world_position,source_id,int(reset_path))
    result=session.command('sound_locator_fixture',payload,timeout=8)
    receipt=read_json(RECEIPT)
    if receipt['pid']!=session.active['pid'] or (receipt['sequence']<=before and before):
        raise RuntimeError('Stale or foreign fixture receipt')
    receipt['acknowledgement']=result
    return receipt

class Capture:
    def __init__(self, device):
        self.api=pa.PyAudio()
        self.info=self.api.get_device_info_by_index(device)
        if not self.info.get('isLoopbackDevice') or self.info['maxInputChannels']!=2:
            raise RuntimeError('Selected device must be a stereo WASAPI loopback')
        self.rate=int(self.info['defaultSampleRate'])
        self.lock=threading.Lock();self.blocks=[];self.statuses=[];self.frames=0
        self.stream=self.api.open(format=pa.paFloat32,channels=2,rate=self.rate,input=True,
            input_device_index=device,frames_per_buffer=1024,stream_callback=self.callback)
    def callback(self,data,frames,times,status):
        with self.lock:
            if data:self.blocks.append(data);self.frames+=len(data)//8
            if status:self.statuses.append(status)
        return None,pa.paContinue
    def reset(self):
        with self.lock:self.blocks=[];self.statuses=[];self.frames=0
    def frame_count(self):
        with self.lock:return self.frames
    def finish(self):
        with self.lock:payload=b''.join(self.blocks);statuses=list(self.statuses)
        return np.frombuffer(payload,dtype='<f4').reshape(-1,2).copy(),statuses
    def close(self):
        self.stream.stop_stream();self.stream.close();self.api.terminate()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--device',type=int,default=34)
    parser.add_argument('--event',type=int,choices=range(5),default=0)
    parser.add_argument('--angles',default='-180,-150,-120,-90,-60,-30,0,30,60,90,120,150')
    parser.add_argument('--sets',type=int,default=2)
    parser.add_argument('--distance',type=float,default=1000)
    parser.add_argument('--seconds-per-angle',type=float,default=3)
    parser.add_argument('--active',type=Path,help='Existing explicit owned stage9 record; no unrelated process attachment')
    parser.add_argument('--output',type=Path)
    parser.add_argument('--final-evaluation',action='store_true',help='Label clips as frozen-algorithm final evaluation, never calibration')
    parser.add_argument('--map',default='ascent')
    parser.add_argument('--listener-point',help='Known walkable world x,y,z; original teleport validates ground and capsule')
    parser.add_argument('--world-source',help='Fixed world x,y,z; full footsteps route validates ground and capsule')
    parser.add_argument('--rotate-listener',action='store_true',help='Keep source fixed and rotate listener to sample relative directions')
    args=parser.parse_args()
    angles=[float(x) for x in args.angles.split(',')]
    listener_point=[float(x) for x in args.listener_point.split(',')] if args.listener_point else None
    world_source=[float(x) for x in args.world_source.split(',')] if args.world_source else None
    if any(p is not None and (len(p)!=3 or not all(math.isfinite(v) for v in p)) for p in (listener_point,world_source)):
        parser.error('World positions require three finite coordinates')
    if args.rotate_listener and world_source is None:parser.error('--rotate-listener requires --world-source')
    if any(not math.isfinite(x) or abs(x)>180 for x in angles) or not 1<=args.sets<=4 or not 1.5<=args.seconds_per_angle<=6:
        parser.error('Invalid angles, sets or clip duration')
    tag=datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
    output=(args.output or ROOT/'.local/sound-locator-recordings'/tag).resolve()
    if ROOT/'.local' not in output.parents:parser.error('Output must stay in project .local')
    output.mkdir(parents=True,exist_ok=False)
    report={'scope':'Actual international-client assets in owned offline stage9 trainer; not live official-server validation',
            'output':str(output),'clips':[],'passed':False,'online_game_validation':False,'hrtf_dsp_verified':False}
    report['intended_use']='frozen_algorithm_final_evaluation' if args.final_evaluation else 'calibration_and_development_holdout'
    build_manifest=ROOT/'.local/native-module/build.json'
    if build_manifest.exists():
        (output/'module-build.json').write_bytes(build_manifest.read_bytes())
    runner=None;session=None;capture=None;log=None
    keepalive_stop=threading.Event();heartbeat=None
    try:
        active=args.active
        if active is None:
            config=Settings(ROOT/'.local/controller/config.json').read()
            config.update(mode='practice' if args.map=='duel1' else 'deathmatch',map_id=args.map,invincible=True,paused=True,total_players=2)
            config['practice']['return_fire']=False
            config=validate(config)
            config_path=output/'config.json';config_path.write_text(json.dumps(config,ensure_ascii=False,indent=2),encoding='utf-8')
            session_dir=ROOT/'.local/training-sessions'/('sound-locator-'+tag)
            active=session_dir/'result.active.json'
            log=(output/'runner.log').open('w',encoding='utf-8')
            duration=min(900,int(len(angles)*args.sets*(args.seconds_per_angle+.8)+130))
            command=[sys.executable,'-X','utf8',str(ROOT/'tools/run_duel1_training.py'),'--prebuilt','--seconds',str(duration),
                '--controller-settings',str(config_path),'--session-dir',str(session_dir),'--test-input']
            runner=subprocess.Popen(command,cwd=ROOT,stdout=log,stderr=log,creationflags=subprocess.CREATE_NO_WINDOW,env=project_environment(ROOT))
            deadline=time.monotonic()+80
            while time.monotonic()<deadline and runner.poll() is None:
                if active.exists():
                    try:
                        candidate=OwnedSession(active)
                        state=candidate.channel.state()
                        if state.get('ready'):session=candidate;break
                        candidate.close()
                    except Exception:pass
                time.sleep(.1)
            if session is None:raise RuntimeError('Owned stage9 did not reach ready; inspect runner.log')
        else:
            session=OwnedSession(active);session.wait(lambda s:s.get('ready'),timeout=30)
        report['active_record']=str(active);report['pid']=session.active['pid']
        probe=session.original()
        heading=[0.]
        def keepalive():
            while not keepalive_stop.is_set() and session.alive():
                try:probe.command(flags=8,forward=0,right=0);probe.aim(pitch=0,yaw=heading[0])
                except Exception:break
                keepalive_stop.wait(.1)
        heartbeat=threading.Thread(target=keepalive,daemon=True);heartbeat.start()
        time.sleep(2)
        if listener_point is not None:
            report['listener_placement']=request(session,5,world_position=listener_point)
            time.sleep(.6)
        report['before']=request(session)
        report['enable']=request(session,2)
        time.sleep(2)
        report['initial_frame']=probe.read().as_dict()
        print(json.dumps({'checkpoint':'native_audio_fixture_ready','pid':report['pid'],'hrtf':report['enable']['hrtf_setting'],'listener':report['enable']['listener'],'output':str(output)}),flush=True)
        capture=Capture(args.device);report['capture_device']=capture.info
        capture.reset();time.sleep(1.5)
        baseline,status=capture.finish();wavfile.write(output/'background.wav',capture.rate,baseline)
        report['background_rms']=np.sqrt(np.mean(baseline.astype(float)**2,axis=0)).tolist() if len(baseline) else None
        for split in range(args.sets):
            for angle in angles:
                if not session.alive():raise RuntimeError('Owned game exited during recording')
                if args.rotate_listener:
                    listener=probe.read().camera
                    bearing=math.degrees(math.atan2(world_source[1]-listener[1],world_source[0]-listener[0]))
                    heading[0]=bearing-angle
                    time.sleep(.4)
                time.sleep(.4);capture.reset();begin=time.monotonic();receipts=[]
                frame_before=probe.read().as_dict()
                while time.monotonic()-begin<args.seconds_per_angle:
                    elapsed=time.monotonic()-begin
                    if elapsed>.3 and len(receipts)<max(1,int((args.seconds_per_angle-.5)/.55)) and elapsed>=.3+len(receipts)*.55:
                        command_start=time.monotonic()-begin;frame_start=capture.frame_count()
                        r=request(session,1,args.event,angle,args.distance,world_position=world_source)
                        r['host_offset_seconds']=time.monotonic()-begin
                        r['command_start_offset_seconds']=command_start
                        r['capture_frames_before_post']=frame_start
                        r['capture_frames_after_ack']=capture.frame_count()
                        receipts.append(r)
                    time.sleep(.01)
                audio,status=capture.finish();frame_after=probe.read().as_dict()
                if len(audio)<capture.rate:raise RuntimeError('Loopback captured insufficient samples')
                name=f'event{args.event}_set{split}_angle{angle:+07.2f}.wav'
                wavfile.write(output/name,capture.rate,audio)
                rms=np.sqrt(np.mean(audio.astype(float)**2,axis=0))
                row={'wav':name,'split':'final_unseen' if args.final_evaluation else 'calibration' if split==0 else 'holdout','set':split,'angle_deg':angle,
                     'event_id':args.event,'samples':len(audio),'sample_rate':capture.rate,'rms_lr':rms.tolist(),
                     'peak':float(np.max(np.abs(audio))),'capture_status_flags':status,'receipts':receipts,
                     'frame_before':frame_before,'frame_after':frame_after,
                     'player_displacement_cm':math.dist(frame_before['location'],frame_after['location']),
                     'sha256':hashlib.sha256((output/name).read_bytes()).hexdigest()}
                report['clips'].append(row)
                (output/'recordings.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
                print(json.dumps({'checkpoint':'clip_recorded','file':name,'rms_lr':rms.tolist(),'playing_ids':[r['playing_id'] for r in receipts]}),flush=True)
            time.sleep(.7);capture.reset();time.sleep(1.5)
            background,background_status=capture.finish()
            background_name=f'background_after_set{split}.wav'
            wavfile.write(output/background_name,capture.rate,background)
            report.setdefault('background_checks',[]).append({'wav':background_name,'label':'background, not silence',
                'rms_lr':np.sqrt(np.mean(background.astype(float)**2,axis=0)).tolist(),
                'capture_status_flags':background_status})
        report['passed']=bool(report['clips']) and all(c['peak']>1e-5 and c['player_displacement_cm']<5 and not c['capture_status_flags'] for c in report['clips'])
        report['pass_definition']='Nonzero recording + stationary player + no capture status flags; not localization accuracy or source attribution'
    except Exception as error:
        report['error']=str(error);print(json.dumps({'error':str(error),'output':str(output)}),flush=True)
    finally:
        if capture:capture.close()
        keepalive_stop.set()
        if heartbeat:heartbeat.join(timeout=2)
        if session:
            if session.alive():
                try:report['restore']=request(session,3)
                except Exception as e:report['restore_error']=str(e)
            try:session.original().release()
            except Exception:pass
            if runner and session.alive():session.stop()
            session.close()
        if runner:
            try:runner.wait(timeout=20)
            except subprocess.TimeoutExpired:report['runner_pending']=True
        if log:log.close()
        (output/'recordings.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({'output':str(output),'passed':report['passed'],'clips':len(report['clips'])}),flush=True)
    return 0 if report['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
