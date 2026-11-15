"""Run one isolated, bounded native loopback host/client connection proof."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time
import uuid

from local_environment import project_environment
from local_network_probe import ROOT, verified_build, analyze_connection_samples


def read(path):
    try:
        return json.loads(Path(path).read_text(encoding='utf-8'))
    except (OSError,ValueError):
        return {}


def assess_completion(report):
    """A brief successful join cannot certify an interrupted or crashed run."""
    checks={
        'observation_completed':report.get('observation_completed') is True,
        'no_probe_error':not report.get('error'),
        'native_driver_shutdown':report.get('host_cleanup',{}).get('cleaned') is True,
        'accepted_build_unchanged':report.get('accepted_playable_build_unchanged') is True,
    }
    for role in ('host','client'):
        result=report.get(role+'_result',{})
        checks[role+'_completed']=bool(
            report.get(role+'_runner_exit_code')==0
            and result.get('owned_process_exited') is True
            and result.get('stop_reason')=='requested_shutdown'
            and result.get('image_unchanged') is True
            and result.get('postflight',{}).get('ready') is True
            and report.get(role+'_critical_errors')==0)
    return {'passed':all(checks.values()),'checks':checks}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port',type=int,default=17777)
    parser.add_argument('--observe-seconds',type=int,default=35)
    parser.add_argument('--transport',choices=('ares','ip'),default='ares')
    args=parser.parse_args()
    if not 1024<=args.port<=65535 or not 5<=args.observe_seconds<=75:
        parser.error('Choose an unprivileged loopback port and 5..75 seconds of observation')
    build=verified_build()
    tag=datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
    directory=ROOT/'.local/local-network'/('pair-'+tag);directory.mkdir(parents=True)
    proof=uuid.uuid4().hex
    base=[sys.executable,'-X','utf8',str(ROOT/'tools/run_native_baseline.py'),'--run','--bootstrap',
          str(ROOT/'.local/map-probe/20261101T020214.576649Z/map-1.bin'),'--map','1','--mode','navigation',
          '--original-game-instance','--network-proof-id',proof,'--network-port',str(args.port),
          '--network-transport',args.transport]
    sources=('.local/native-module/trainer_bridge.dll','.local/native-module/build.json')
    original={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in sources}
    report={'schema_version':1,'directory':str(directory),'proof_id':proof,'module_sha256':build['module_sha256'],
            'transport':args.transport,
            'host_listen_verified':False,'client_travel_requested':False,'native_join_proven':False,
            'remote_character_replication_proven':False,'authority_movement_proven':False,
            'requested_observation_seconds':args.observe_seconds,'observation_completed':False,
            'passed':False,'samples':[]}
    processes={};logs={}
    def save():
        (directory/'progress.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    def launch(role,seconds):
        output=directory/role;output.mkdir()
        command=base+['--seconds',str(seconds),'--network-probe',role,'--network-output',str(output),
                      '--result-file',str(directory/(role+'-result.json'))]
        if role=='client':command+=['--network-peer',str(directory/'host-result.active.json')]
        log=(directory/(role+'-runner.log')).open('w',encoding='utf-8');logs[role]=log
        processes[role]=subprocess.Popen(command,cwd=ROOT,env=project_environment(ROOT),stdout=log,stderr=log,
                                         creationflags=subprocess.CREATE_NO_WINDOW)
    def alive(role):
        return role in processes and processes[role].poll() is None
    try:
        (ROOT/'.local/local-network/latest-proof.json').write_text(json.dumps({'directory':str(directory),'proof_id':proof,'kind':'native-host-client'}),encoding='utf-8')
        launch('host',min(175,args.observe_seconds+90))
        until=time.monotonic()+40
        while time.monotonic()<until and alive('host'):
            state=read(directory/'host/bridge-state.json')
            if state.get('error'):raise RuntimeError('Host: '+str(state.get('failure_stage') or state))
            if state.get('listen_returned') and state.get('loopback_candidates_verified') and state.get('bind_address')=='127.0.0.1':
                report['host_listen_verified']=True;report['host']=state
                print(json.dumps({'checkpoint':'native_host_listening','pid':state['pid'],'address':'127.0.0.1','port':args.port}),flush=True)
                break
            time.sleep(.1)
        if not report['host_listen_verified']:raise RuntimeError('Owned native host did not reach its verified loopback bind')
        launch('client',min(150,args.observe_seconds+45))
        start=time.monotonic();deadline=start+args.observe_seconds+35;ready_at=None
        while time.monotonic()<deadline and alive('host') and alive('client'):
            host=read(directory/'host/bridge-state.json');client=read(directory/'client/bridge-state.json')
            if host.get('error') or client.get('error'):
                report['last_host'],report['last_client']=host,client
                raise RuntimeError('Native probe reported an error')
            if client.get('action_completed'):
                ready_at=ready_at or time.monotonic();report['client_travel_requested']=True
            row={'elapsed':round(time.monotonic()-start,3),'host':host,'client':client}
            if not report['samples'] or row['elapsed']-report['samples'][-1]['elapsed']>=.1:
                report['samples'].append(row)
                if len(report['samples'])%10==0:save()
            if ready_at and time.monotonic()-ready_at>=args.observe_seconds:
                report['observation_completed']=alive('host') and alive('client')
                break
            time.sleep(.1)
        report['observed_seconds']=round(time.monotonic()-ready_at,3) if ready_at else 0
        report['last_host']=read(directory/'host/bridge-state.json')
        report['last_client']=read(directory/'client/bridge-state.json')
        report['connection_phase_observed']={
            'client_server_connection':any(int(s['client'].get('server_connection','0'),16)!=0 for s in report['samples']),
            'host_client_connection':any(s['host'].get('client_connections',0)>0 for s in report['samples']),
            'client_welcome':any(s['client'].get('pending_welcome_received') for s in report['samples']),
            'client_entered_join_path':any(s['client'].get('pending_join_path_entered') for s in report['samples']),
            'client_map_loaded':any(s['client'].get('pending_map_loaded') for s in report['samples'])}
        report['native_gates']=analyze_connection_samples(report['samples'])
        report['native_join_proven']=report['native_gates']['native_control_join_proven']
        report['remote_character_replication_proven']=report['native_gates']['remote_character_replication_proven']
        report['server_ai_movement_replicated_proven']=report['native_gates']['server_ai_movement_replicated_proven']
        # Native PC/Pawn possession/replication is a separate gate; the flags
        # above deliberately do not convert a handshake into a successful join.
        report['replication_gates_passed']=(report['native_join_proven'] and report['native_gates']['replicated_own_state_and_pawn_proven']
                                            and report['server_ai_movement_replicated_proven'])
    except Exception as error:
        report['error']=str(error)
    finally:
        # Each runner terminates only its original Popen child. Stop client first
        # so its peer identity remains valid through postflight.
        for role in ('client','host'):
            active=read(directory/(role+'-result.active.json'))
            if active:
                if role=='host' and alive('host'):
                    (directory/'host/stop-network').write_text('original driver shutdown',encoding='utf-8')
                    until=time.monotonic()+3
                    while time.monotonic()<until:
                        if read(directory/'host/bridge-state.json').get('cleaned'):break
                        time.sleep(.05)
                    report['host_cleanup']=read(directory/'host/bridge-state.json')
                owned=Path(active['run_dir']).resolve()
                if ROOT/'.local/native-baseline' not in owned.parents:
                    raise RuntimeError('Unexpected network proof child ownership directory')
                (owned/'stop-request.json').write_text(json.dumps({'reason':'native_network_proof_complete','proof_id':proof}),encoding='utf-8')
            if role in processes:
                try:processes[role].wait(timeout=15)
                except subprocess.TimeoutExpired:report[role+'_runner_cleanup_pending']=True
                report[role+'_runner_exit_code']=processes[role].poll()
                report[role+'_result']=read(directory/(role+'-result.json'))
                if active:
                    log_path=Path(active['run_dir'])/'runtime.log'
                    if log_path.is_file():
                        report[role+'_critical_errors']=log_path.read_text(encoding='utf-8',errors='replace').count('Critical error:')
            if role in logs:logs[role].close()
        report['accepted_playable_build_unchanged']=all(hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==digest for name,digest in original.items())
        report['completion']=assess_completion(report)
        report['passed']=bool(report.get('replication_gates_passed') and report['completion']['passed'])
        report['at_utc']=datetime.now(timezone.utc).isoformat();save()
        (directory/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({'passed':report['passed'],'host_listen_verified':report['host_listen_verified'],
                      'phase':report.get('connection_phase_observed'),'error':report.get('error'),
                      'report':str(directory/'verification.json')}),flush=True)
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
