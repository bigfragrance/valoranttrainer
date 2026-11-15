"""Prepare a movable trainer whose complete original files were downloaded."""
from __future__ import annotations

from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
KIND = 'local-server-downloaded-full-v1'
MODE = 'downloaded-original-files-v1'
SETTINGS = '.local/controller/local-server-candidate/humanlike-bots.json'


def file_hash(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def manifest(root):
    from trainer_patch import read_json
    profile = root/'.local/download-resource-profile.json'
    minimal = profile.is_file() and read_json(profile).get('resource_profile') == 'minimal'
    data = read_json(root/('profiles/downloaded-files-minimal.json' if minimal else 'profiles/downloaded-files.json'))
    if data.get('schema_version') != 1 or data.get('manifest_id') != '448DAE34A9D16C88':
        raise RuntimeError('下载资源的版本清单不受支持')
    return data


def valid_contents(path, row):
    if 'ranges' not in row:
        return file_hash(path) == row['sha256']
    with path.open('rb') as stream:
        for offset, size, expected in row['ranges']:
            stream.seek(offset)
            if hashlib.sha256(stream.read(size)).hexdigest() != expected:
                return False
    return True


def finalize(root, records):
    from trainer_patch import owned, read_json, write_json
    root = Path(root).resolve()
    data = manifest(root)
    if set(records) != {row['path'] for row in data['files']}:
        raise RuntimeError('下载文件清单尚未完成')
    for row in data['files']:
        path = owned(root, 'client/'+row['path'])
        record = records[row['path']]
        stat = path.stat()
        if (record.get('sha256') != row['sha256'] or record.get('bytes') != row['size']
                or stat.st_size != row['size'] or stat.st_mtime_ns != record.get('mtime_ns')):
            raise RuntimeError('下载文件校验记录不一致 · '+path.name)
    config = read_json(root/'profiles/patch-project.json')
    config.update(resource_mode=MODE, source_resource_directory=str(root/'client/ShooterGame/Content/Paks'),
                  source_game_executable=str(root/'client/ShooterGame/Binaries/Win64/VALORANT-Win64-Shipping.exe'),
                  downloaded_manifest_id=data['manifest_id'])
    write_json(root/'project.json', config)
    settings = owned(root, SETTINGS)
    if not settings.exists():
        settings.parent.mkdir(parents=True, exist_ok=True)
        defaults = read_json(root/'profiles/patch-default-settings.json')
        if (root/'.local/download-resource-profile.json').is_file() and read_json(root/'.local/download-resource-profile.json').get('resource_profile') == 'minimal':
            defaults['map_id'] = 'jam'
        write_json(settings, defaults)
    write_json(root/'.local/download-environment.json', {'prepared': True, 'manifest_id': data['manifest_id'],
        'prepared_utc': datetime.now(timezone.utc).isoformat()})


def state(root=ROOT, *, full=False):
    from trainer_patch import owned, read_json, write_json
    root = Path(root).resolve()
    data = manifest(root)
    receipt_path = root/'.local/download-installation.json'
    receipt = read_json(receipt_path) if receipt_path.is_file() else {}
    if receipt.get('complete') is not True or receipt.get('manifest_id') != data['manifest_id']:
        raise RuntimeError('游戏资源安装尚未完成，请打开安装器继续')
    records = receipt.get('files', {})
    refreshed = False
    for row in data['files']:
        path = owned(root, 'client/'+row['path'])
        record = records.get(row['path'], {})
        if not path.is_file():
            raise RuntimeError('游戏文件缺失，请打开安装器进行校验修复 · '+path.name)
        stat = path.stat()
        if stat.st_size != row['size'] or record.get('sha256') != row['sha256']:
            raise RuntimeError('游戏文件版本不一致，请打开安装器进行校验修复 · '+path.name)
        if full or stat.st_mtime_ns != record.get('mtime_ns'):
            if not valid_contents(path, row):
                raise RuntimeError('游戏文件校验失败，请打开安装器进行校验修复 · '+path.name)
            if stat.st_mtime_ns != record.get('mtime_ns'):
                record['mtime_ns'] = stat.st_mtime_ns
                refreshed = True
    if refreshed:
        write_json(receipt_path, receipt)
    return receipt


def prepare(root=ROOT):
    root = Path(root).resolve()
    receipt = state(root)
    finalize(root, receipt['files'])
    return receipt


def check(root=ROOT):
    root = Path(root).resolve()
    data = state(root, full=True)
    finalize(root, data['files'])
    from local_server_module import verified_build
    from offline_gi_instance import verified_build as startup_build
    from start_local_server_console import configuration_plan
    from local_server_controller import check_candidate_config
    from trainer_patch import read_json
    build = verified_build()
    version = read_json(root/'patch-version.json')
    if build['module_sha256'] != version['module_sha256']:
        raise RuntimeError('本地服务器模块与安装版本不一致')
    startup_build()
    check_candidate_config(configuration_plan(root)['configuration'], root=root)
    return {'passed': True, 'manifest_id': data['manifest_id'], 'game_files': len(data['files']),
            'module_sha256': build['module_sha256'], 'self_contained': True, 'game_started': False}


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--finalize-install', action='store_true', required=True)
    parser.parse_args()
    from trainer_patch import read_json
    finalize(ROOT, read_json(ROOT/'.setup/verified-files.json')['files'])
    print(json.dumps({'prepared': True, 'game_started': False}))
