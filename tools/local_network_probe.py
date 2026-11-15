"""Isolated native network proof build, loader and exact owned-peer checks."""
from __future__ import annotations

import ctypes
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import struct
import subprocess

from local_environment import project_environment
from client_layout import client_executable
from native_module import load_verified_owned_library
from run_native_probe import preflight

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'work/local-server-proof/native'
BUILD = ROOT / '.local/local-network/module'
PIN = '147763350504d241ee20b77607d04bc85cebfedb1d8bc2f32473ca93bf8fe92f'
CONTEXT_MAGIC = 0x3150524F5754454E


def local_path(path):
    path = Path(path).resolve()
    if (ROOT / '.local').resolve() not in path.parents:
        raise ValueError('Network proof outputs must stay under project .local')
    return path


def code_profiles(addresses):
    """Bind complete pdata ranges to the pinned file; no target execution."""
    import bisect
    import mmap
    path = ROOT / 'work/client-analysis/Shipping-analysis-copy.exe'
    with path.open('rb') as stream, mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as image:
        if hashlib.sha256(image).hexdigest() != PIN:
            raise ValueError('Native network proof requires the pinned original image')
        pe = struct.unpack_from('<I', image, 0x3c)[0]
        count = struct.unpack_from('<H', image, pe+6)[0]
        optional = pe+24
        table = optional+struct.unpack_from('<H', image, pe+20)[0]
        sections = [struct.unpack_from('<4I', image, table+n*40+8) for n in range(count)]

        def read(rva, size):
            for _, base, length, raw in sections:
                offset = rva-base
                if 0 <= offset and offset+size <= length:
                    return image[raw+offset:raw+offset+size]
            raise ValueError('Original code is not fully backed by file bytes')

        pdata, length = struct.unpack_from('<II', image, optional+112+3*8)
        entries = list(struct.iter_unpack('<III', read(pdata, length)))
        starts = [row[0] for row in entries]
        records = []
        for rva in addresses:
            index = bisect.bisect_left(starts, rva)
            if index == len(entries) or entries[index][0] != rva:
                raise ValueError('Expected original function entry: '+hex(rva))
            start, end, _ = entries[index]
            if not 0 < end-start < 65536:
                raise ValueError('Unbounded original function profile')
            raw = read(start, end-start)
            fnv = 14695981039346656037
            for byte in raw:
                fnv = ((fnv ^ byte)*1099511628211) & 0xffffffffffffffff
            records.append({'rva':start, 'bytes':len(raw), 'fnv64':fnv,
                            'sha256':hashlib.sha256(raw).hexdigest()})
    return records


def build():
    BUILD.mkdir(parents=True, exist_ok=True)
    profiles = code_profiles((0x3edab80, 0x4c2b940, 0x4742820, 0x5118750,
                              0x5118500, 0x4c54840, 0x3df7850, 0x491b420,
                              0x4ce4910,0x4c2bcd0,0x51173c0,0x21a82e0,
                              0x18cbef0,0x18a2f10,0x3edad30,0x4313720,
                              0x4717bf0,0x42db600,0x42ddc40,0x47113e0,0x1cf35b0,
                              0x43fada0,0x3ef6cb0,0x431cd80,0x42ed380))
    header = '#pragma once\nstruct CodeProfile { unsigned rva, bytes; unsigned long long fnv; };\n'
    header += 'static constexpr CodeProfile NativeProfiles[]{\n'
    header += ''.join('    {0x%x,%d,0x%xULL},\n' % (r['rva'],r['bytes'],r['fnv64']) for r in profiles)
    header += '};\n'
    (SOURCE/'network_profiles.h').write_text(header, encoding='utf-8')
    inputs = [SOURCE/'network_probe.cpp',SOURCE/'network_profiles.h',SOURCE/'build.cmd',ROOT/'native/engine_api.h']
    captured = {str(p.relative_to(ROOT)):p.read_bytes() for p in inputs}
    result = subprocess.run([os.environ.get('COMSPEC','cmd.exe'),'/d','/c',str(SOURCE/'build.cmd'),str(BUILD)],
                            cwd=ROOT,env=project_environment(ROOT),capture_output=True,text=True,
                            creationflags=subprocess.CREATE_NO_WINDOW,timeout=60)
    (BUILD/'build.log').write_text(result.stdout+result.stderr,encoding='utf-8')
    if result.returncode:
        raise RuntimeError((result.stdout+result.stderr)[-6500:])
    if any((ROOT/name).read_bytes()!=data for name,data in captured.items()):
        raise RuntimeError('Network proof source changed during compilation')
    snapshot = BUILD/'source-snapshot'
    for name,data in captured.items():
        target=snapshot/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    record={'schema_version':1,'source_image_sha256':PIN,
            'source_snapshot':str(snapshot.relative_to(ROOT)),
            'source_sha256':{name:hashlib.sha256(data).hexdigest() for name,data in captured.items()},
            'module_sha256':hashlib.sha256((BUILD/'network_probe.dll').read_bytes()).hexdigest(),
            'native_profiles':profiles,'built_utc':datetime.now(timezone.utc).isoformat()}
    (BUILD/'build.json').write_text(json.dumps(record,indent=2),encoding='utf-8')
    return record


def verified_build():
    record=json.loads((BUILD/'build.json').read_text(encoding='utf-8'))
    if record['source_image_sha256']!=PIN:
        raise RuntimeError('Network probe image identity mismatch')
    snapshot=local_path(ROOT/record['source_snapshot'])
    for name,digest in record['source_sha256'].items():
        target=(snapshot/name).resolve()
        if snapshot not in target.parents or hashlib.sha256(target.read_bytes()).hexdigest()!=digest:
            raise RuntimeError('Network probe captured source mismatch')
    if hashlib.sha256((BUILD/'network_probe.dll').read_bytes()).hexdigest()!=record['module_sha256']:
        raise RuntimeError('Network probe library changed after its build')
    return record


def _module_identity(module_sha256=None, build_record=None):
    """Bind shared ownership checks to the caller's already verified flavor."""
    if build_record is None and module_sha256 is None:
        build_record=verified_build()
    recorded=(build_record or {}).get('module_sha256')
    if module_sha256 is not None and recorded is not None and module_sha256!=recorded:
        raise RuntimeError('Network library identity differs from the verified build record')
    expected=module_sha256 if module_sha256 is not None else recorded
    if not isinstance(expected,str) or not expected:
        raise RuntimeError('Network ownership requires a verified library identity')
    return expected


def owned_identity(process, executable, *, module_sha256=None, build_record=None, network_flavor='proof'):
    from capture_owned_thread import _kernel,_creation,_image
    kernel=_kernel();handle=int(process._handle)
    expected=client_executable(json.loads((ROOT/'project.json').read_text(encoding='utf-8')),ROOT)
    if kernel.GetProcessId(handle)!=process.pid or _image(kernel,handle)!=expected or Path(executable).resolve()!=expected:
        raise RuntimeError('Network proof Popen handle/image mismatch')
    if kernel.WaitForSingleObject(handle,0)!=0x102:
        raise RuntimeError('Network proof child exited before ownership publication')
    return {'schema_version':1,'pid':process.pid,'creation_filetime':_creation(kernel,handle),
            'executable':str(expected),'image_sha256':PIN,
            'published_from_original_popen_handle':True,'module_sha256':_module_identity(module_sha256,build_record),
            'network_flavor':network_flavor}


def validate_peer(path, proof_id, *, module_sha256=None, build_record=None, network_flavor='proof', allow_exited=False):
    from capture_owned_thread import _kernel,_creation,_image
    path=local_path(path);active=json.loads(path.read_text(encoding='utf-8'))
    proof=active.get('network_probe') or {};identity=active.get('network_probe_identity') or {}
    if (proof.get('role')!='host' or proof.get('proof_id')!=proof_id
            or proof.get('flavor','proof')!=network_flavor
            or identity.get('network_flavor','proof')!=network_flavor
            or not identity.get('published_from_original_popen_handle')
            or identity.get('pid')!=active.get('pid') or identity.get('image_sha256')!=PIN
            or identity.get('module_sha256')!=_module_identity(module_sha256,build_record)):
        raise RuntimeError('Peer is not the matching runner-owned host proof')
    expected=client_executable(json.loads((ROOT/'project.json').read_text(encoding='utf-8')),ROOT)
    if Path(identity['executable']).resolve()!=expected:
        raise RuntimeError('Peer image is outside the configured owned client')
    kernel=_kernel();handle=kernel.OpenProcess(0x1000|0x100000,False,active['pid'])
    if not handle:
        import ctypes
        if allow_exited and ctypes.get_last_error()==87:
            return dict(active, _peer_exited=True)
        raise RuntimeError('Owned peer no longer exists or cannot be opened')
    try:
        if _creation(kernel,handle)!=identity['creation_filetime'] or _image(kernel,handle)!=expected:
            raise RuntimeError('Peer process identity changed')
        state=kernel.WaitForSingleObject(handle,0)
        if state==0 and allow_exited:
            return dict(active, _peer_exited=True)
        if state!=0x102:raise RuntimeError('Peer process identity changed')
    finally:kernel.CloseHandle(handle)
    return active


def pair_preflight(config, peer=None, proof_id=None, *, module_sha256=None, build_record=None, network_flavor='proof', allow_exited=False):
    result=preflight(config)
    if not peer:return result
    active=validate_peer(peer,proof_id,module_sha256=module_sha256,build_record=build_record,network_flavor=network_flavor,allow_exited=allow_exited)
    blocking=result['blocking_processes']
    if active.get('_peer_exited'):
        if any(p['pid']==active['pid'] for p in blocking):
            raise RuntimeError('Exited peer PID still present in preflight inventory')
        result['paired_owned_host_exited']={'pid':active['pid']}
        return result
    retained=[p for p in blocking if p['pid']!=active['pid']]
    removed=[p for p in blocking if p['pid']==active['pid']]
    if len(removed)!=1:raise RuntimeError('Owned peer did not match the preflight process inventory')
    result['blocking_processes']=retained
    result['existing_game_or_launcher_processes']=bool(retained)
    result['paired_owned_host']={'pid':active['pid'],'creation_filetime':active['network_probe_identity']['creation_filetime']}
    result['ready']=result['expected_image_matches'] and not any(result[k] for k in
        ('existing_game_or_launcher_processes','saved_login_present','saved_authorization_present','matching_saved_credential_present'))
    return result


def configure_ip_transport(snapshot):
    """Select the shipped generic IP backend in the isolated bootstrap only."""
    from ini_bootstrap import set_value
    changed=0
    def visit(file):
        nonlocal changed
        for name,section in file.get('sections',[]):
            if name in ('/Script/Engine.Engine','/Script/Engine.GameEngine'):
                for pair in section['pairs']:
                    if pair[0]=='NetDriverDefinitions' and 'DefName="GameNetDriver"' in pair[1]:
                        pair[1]='(DefName="GameNetDriver",DriverClassName="/Script/OnlineSubsystemUtils.IpNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")'
                        changed+=1
        set_value(file,'/Script/OnlineSubsystemUtils.IpNetDriver','NetConnectionClassName','/Script/OnlineSubsystemUtils.IpConnection')
        if isinstance(file.get('source_config'),dict):visit(file['source_config'])
    visit(snapshot['categories'][0]['file'])
    if not changed:raise ValueError('Original bootstrap has no GameNetDriver definition')
    return changed


def analyze_connection_samples(samples):
    def pointer(value):return int(value or '0',16)
    joined=False;possessed=False;remote_rows=[]
    for row in samples:
        host,client=row.get('host',{}),row.get('client',{})
        server_ready=any(c.get('state')==3 and c.get('login_state')==3
                         and c.get('owner_links_verified') and c.get('controller_role')==3
                         for c in host.get('host_connections',[]))
        client_ready=(client.get('connection_state')==3 and client.get('own_local_links_verified')
                      and client.get('local_controller_is_connection_owner') and client.get('local_controller_role')==2
                      and pointer(client.get('current_world'))!=pointer(client.get('initial_world'))
                      and not pointer(client.get('pending_netgame'))
                      and not pointer(client.get('world_authority_game_mode')))
        joined|=bool(server_ready and client_ready)
        possessed|=bool(server_ready and client_ready and pointer(client.get('local_player_state'))
                        and pointer(client.get('local_pawn')) and client.get('local_pawn_role')==2)
        if (server_ready and client_ready and pointer(host.get('remote_pawn'))
                and client.get('client_remote_count')==1 and host.get('remote_ai_is_local')
                and host.get('remote_pawn_is_local')):
            remote_rows.append(row)
    def points(side,key):
        return [r[side][key] for r in remote_rows if len(r[side].get(key,[]))==3
                and all(isinstance(x,(int,float)) and math.isfinite(x) for x in r[side][key])]
    def span(values):return max((math.dist(a,b) for a in values for b in values),default=0)
    server_span=span(points('host','server_remote_position'))
    client_span=span(points('client','client_remote_position'))
    remote=len(remote_rows)>=2
    movements=[r['host'].get('remote_move_submissions',0) for r in remote_rows]
    errors=[]
    for row in remote_rows:
        client=row['client'];at=client.get('client_remote_sample_at_ms',0)
        position=client.get('client_remote_position')
        candidates=[r['host'].get('server_remote_position') for r in remote_rows
                    if at>0 and abs(r['host'].get('server_remote_sample_at_ms',0)-at)<=500]
        if position and candidates:errors.append(min(math.dist(position,p) for p in candidates if p))
    matching=sum(e<=150 for e in errors)/len(errors) if errors else 0
    movement=bool(remote and server_span>50 and client_span>50 and max(movements)-min(movements)>10
                  and len(errors)>=3 and matching>=.9)
    return {'native_control_join_proven':joined,'replicated_own_state_and_pawn_proven':possessed,
            'remote_character_replication_proven':remote,'server_ai_movement_replicated_proven':movement,
            'server_remote_span_cm':round(server_span,4),'client_remote_span_cm':round(client_span,4),
            'trajectory_samples':len(errors),'trajectory_match_ratio':round(matching,4),
            'nearest_timed_server_error_max_cm':round(max(errors,default=0),4),
            'authority_movement_proven':False,
            'scope':'Native DefaultPawn/AI connection and replication proof; original ShooterCharacter input/ACK/correction remains a separate gate.'}


def load(process, context, *, role, port, output, transport='ares', module=None, build_record=None):
    record=verified_build() if build_record is None else build_record
    module=BUILD/'network_probe.dll' if module is None else local_path(module)
    output=local_path(output);output.mkdir(parents=True,exist_ok=True)
    if role not in ('host','client') or transport not in ('ares','ip') or not 1024<=port<=65535:
        raise ValueError('Native network loading requires a host/client role, supported transport and unprivileged port')
    directory=(str(output)+'\\').encode('utf-16-le')
    if len(directory)>=2048:raise ValueError('Network output path is too long')
    payload=struct.pack('<Q4I',CONTEXT_MAGIC,1,1 if role=='host' else 2,port,1 if transport=='ip' else 0)+directory.ljust(2048,b'\0')
    (module.parent/f'network-{process.pid}.bin').write_bytes(payload)
    return load_verified_owned_library(process,context,module,_module_identity(build_record=record),output/'bridge-state.json',1)


if __name__=='__main__':
    result=build()
    print(json.dumps({'module':str(BUILD/'network_probe.dll'),'sha256':result['module_sha256'],
                      'profile_count':len(result['native_profiles'])}))
