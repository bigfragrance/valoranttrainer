"""Restore selected original resource bytes without an installed game."""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import uuid
import zipfile

from resource_bundle import _sparse_size, allocated_bytes
from trainer_patch import owned, read_json, write_json, digest, SETTINGS

ROOT=Path(__file__).resolve().parents[1]
PAKS='client/ShooterGame/Content/Paks'
RECEIPT='.local/standalone-resources.json'


def manifest(root):
    version=read_json(root/'patch-version.json')
    path=owned(root,'profiles/standalone-dependencies.json')
    if digest(path)!=version.get('dependency_manifest_sha256'):
        raise RuntimeError('资源清单已改变，请重新解压启动包')
    value=read_json(path)
    if value.get('schema_version')!=1 or value.get('kind')!='original-signed-ranges-v1':
        raise RuntimeError('无效的独立资源格式')
    names=set()
    for row in value['containers']:
        if Path(row['file']).name!=row['file'] or not row['file'].endswith('.ucas') or row['file'] in names:
            raise RuntimeError('无效的资源文件名')
        names.add(row['file'])
        previous=total=0
        for offset,length,sha in row['ranges']:
            if offset<previous or length<=0 or offset+length>row['logical_bytes'] or len(sha)!=64:
                raise RuntimeError('无效的原资源区间')
            previous=offset+length
            total+=length
        if total!=row['payload_bytes'] or not row['ranges']:
            raise RuntimeError('原资源区间长度不一致')
    for row in value['passthrough']:
        if Path(row['file']).name!=row['file'] or row['file'] in names or row['bytes']<=0:
            raise RuntimeError('无效的原资源索引清单')
        names.add(row['file'])
    return value,version


def range_check(path,row):
    if not path.is_file() or path.stat().st_size!=row['logical_bytes']:
        return False
    with path.open('rb',buffering=16*1024*1024) as stream:
        for offset,length,expected in row['ranges']:
            stream.seek(offset)
            sha=hashlib.sha256()
            while length:
                data=stream.read(min(length,1024*1024))
                if not data:
                    return False
                sha.update(data)
                length-=len(data)
            if sha.hexdigest()!=expected:
                return False
    return True


def prior_record(root):
    try:
        return read_json(owned(root,RECEIPT))
    except (OSError,ValueError):
        return {}


def status(root=ROOT,*,verify=False):
    root=Path(root).resolve()
    value,version=manifest(root)
    prior=prior_record(root)
    cached=prior.get('files',{}) if prior.get('manifest_sha256')==version['dependency_manifest_sha256'] else {}
    invalid=[]
    records={}
    for row in value['containers']+value['passthrough']:
        path=owned(root,PAKS+'/'+row['file'])
        stat=path.stat() if path.is_file() else None
        size=row.get('logical_bytes',row.get('bytes'))
        good=bool(stat and stat.st_nlink==1 and stat.st_size==size)
        old=cached.get(row['file'],{})
        same=good and old.get('mtime_ns')==stat.st_mtime_ns and old.get('verified') is True
        if good and (verify or not same):
            good=range_check(path,row) if 'ranges' in row else digest(path)==row['sha256']
        if not good:
            invalid.append(row['file'])
        elif stat:
            records[row['file']]={'mtime_ns':stat.st_mtime_ns,'bytes':stat.st_size,'verified':True}
    return {'complete':not invalid,'invalid_files':invalid,'files':records,
            'manifest_sha256':version['dependency_manifest_sha256']}


def prepare(root=ROOT,bundle_path=None):
    root=Path(root).resolve()
    value,version=manifest(root)
    current=status(root)
    if not current['complete']:
        bundle=Path(bundle_path).resolve(strict=True) if bundle_path else root/version['dependency_file']
        if not bundle.is_file():
            raise RuntimeError('请将 '+version['dependency_file']+' 放到启动脚本同一文件夹，再次启动。无需安装国际服。')
        if bundle.stat().st_size!=version['dependency_bytes']:
            raise RuntimeError('资源依赖文件大小不正确，请重新传输该文件')
        # The first install validates transport identity before touching outputs.
        print('正在校验资源依赖文件，首次准备需要几分钟。',flush=True)
        if digest(bundle)!=version['dependency_sha256']:
            raise RuntimeError('资源依赖文件校验失败，请重新传输')
        wanted=set(current['invalid_files'])
        with zipfile.ZipFile(bundle) as incoming:
            data=incoming.read('dependency-manifest.json')
            if hashlib.sha256(data).hexdigest()!=version['dependency_manifest_sha256']:
                raise RuntimeError('资源文件与启动包不是同一版本')
            members={'dependency-manifest.json',*[row['member'] for row in value['containers']+value['passthrough']]}
            if len(incoming.namelist())!=len(members) or set(incoming.namelist())!=members:
                raise RuntimeError('资源文件成员清单不匹配')
            paks=owned(root,PAKS)
            paks.mkdir(parents=True,exist_ok=True)
            for row in value['passthrough']+value['containers']:
                if row['file'] not in wanted:
                    continue
                target=owned(root,PAKS+'/'+row['file'])
                temporary=target.with_name(target.name+'.'+uuid.uuid4().hex+'.preparing')
                try:
                    with incoming.open(row['member']) as stream,temporary.open('x+b',buffering=16*1024*1024) as outgoing:
                        if 'ranges' in row:
                            _sparse_size(outgoing,row['logical_bytes'])
                            ranges=row['ranges']
                        else:
                            ranges=[[0,row['bytes'],row['sha256']]]
                        total=0
                        for offset,length,expected in ranges:
                            outgoing.seek(offset)
                            sha=hashlib.sha256()
                            remaining=length
                            while remaining:
                                data=stream.read(min(remaining,1024*1024))
                                if not data:
                                    raise RuntimeError('资源数据已截断')
                                outgoing.write(data)
                                sha.update(data)
                                remaining-=len(data)
                                total+=len(data)
                            if sha.hexdigest()!=expected:
                                raise RuntimeError('原资源区间校验失败：'+row['file'])
                        if stream.read(1) or total!=row.get('payload_bytes',row.get('bytes')):
                            raise RuntimeError('资源数据长度不一致')
                        outgoing.flush()
                        os.fsync(outgoing.fileno())
                    os.replace(temporary,target)
                finally:
                    temporary.unlink(missing_ok=True)
                stat=target.stat()
                current['files'][row['file']]={'bytes':stat.st_size,'mtime_ns':stat.st_mtime_ns,'verified':True}
                write_json(owned(root,RECEIPT),{'complete':False,'manifest_sha256':version['dependency_manifest_sha256'],'files':current['files']})
                print('已准备：'+row['file'],flush=True)
        current=status(root)
        if not current['complete']:
            raise RuntimeError('资源准备后验证未通过')
    write_json(owned(root,RECEIPT),{**current,'complete':True,'original_checks_preserved':True,
                                   'original_installation_used':False,
                                   'allocated_bytes':sum(allocated_bytes(owned(root,PAKS+'/'+name)) for name in current['files'])})
    config=read_json(root/'profiles/patch-project.json')
    config.update(resource_mode='standalone-original-ranges-v1',source_resource_directory=str(root/PAKS),
                  resources_manifest_sha256=version['dependency_manifest_sha256'])
    write_json(root/'project.json',config)
    settings=owned(root,SETTINGS)
    if not settings.exists():
        import shutil
        settings.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(root/'profiles/patch-default-settings.json',settings)
    return current


def check(root=ROOT):
    root=Path(root).resolve()
    state=status(root)
    if not state['complete']:
        raise RuntimeError('独立资源尚未准备完整，请先运行启动入口。缺少：'+', '.join(state['invalid_files'][:4]))
    config=read_json(root/'project.json')
    if config.get('resource_mode')!='standalone-original-ranges-v1' or Path(config.get('source_resource_directory','')).resolve()!=root/PAKS:
        raise RuntimeError('独立资源配置不属于本包')
    for row in read_json(root/'profiles/patch-installation-manifest.json')['loose_files']:
        if digest(owned(root,'client/'+row['file']))!=row['sha256']:
            raise RuntimeError('包内运行文件已改变：'+row['file'])
    from local_server_module import verified_build
    from offline_gi_instance import verified_build as startup_build
    startup_build()
    build=verified_build()
    from start_local_server_console import configuration_plan
    from local_server_controller import check_candidate_config
    check_candidate_config(configuration_plan(root)['configuration'],root=root)
    return {'passed':True,'module_sha256':build['module_sha256'],'installed_game_required':False,
            'source_resource_directory':str(root/PAKS),'original_checks_preserved':True,
            'game_started':False,'account_login':False}
