"""Versioned GitHub installer downloads and an external Windows replacement worker."""
from __future__ import annotations

import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

import github_update
from installer_engine import InstallationError, atomic_json, digest

MANIFEST = 'installer-update.json'


def check(repository, current):
    repository = github_update.repository_name(repository)
    release = github_update.fetch('https://api.github.com/repos/' + repository + '/releases/latest', limit=2_000_000)
    if release.get('draft') or release.get('prerelease'):
        raise InstallationError('安装器更新渠道没有正式版本')
    assets = {item['name']: item for item in release.get('assets', [])}
    if MANIFEST not in assets:
        raise InstallationError('此 Release 尚未提供安装器更新清单')
    manifest = github_update.fetch(assets[MANIFEST]['browser_download_url'], limit=65536)
    name = manifest.get('asset', '')
    if (manifest.get('schema_version') != 1 or not re.fullmatch(r'ValorantTrainer-Setup-[\w.-]+\.exe', name)
            or name not in assets or not isinstance(manifest.get('bytes'), int)
            or not 0 < manifest['bytes'] <= 1_000_000_000
            or not re.fullmatch(r'[0-9a-f]{64}', manifest.get('sha256', ''))):
        raise InstallationError('安装器更新清单不完整')
    if (github_update.version_key(manifest['version']) != github_update.version_key(release['tag_name'])
            or assets[name].get('size') != manifest['bytes']):
        raise InstallationError('安装器版本或大小与 Release 不一致')
    expected = 'https://github.com/' + repository + '/releases/download/' + release['tag_name'] + '/' + name
    if assets[name]['browser_download_url'] != expected:
        raise InstallationError('安装器下载地址与发布仓库不一致')
    return {'available': github_update.version_key(manifest['version']) > github_update.version_key(current),
            'current': current, 'version': manifest['version'], 'manifest': manifest, 'url': expected}


def prepare(release, executable, target, worker_script, progress=None):
    executable = Path(executable).resolve(strict=True)
    if not release['available'] or executable.suffix.lower() != '.exe':
        raise InstallationError('当前安装器无需替换或并非打包版本')
    stage = Path(tempfile.mkdtemp(prefix='.trainer-self-update-', dir=executable.parent))
    candidate = stage / 'installer.exe'
    try:
        manifest = release['manifest']
        count = github_update.fetch(release['url'], limit=manifest['bytes'], destination=candidate, progress=progress)
        github_update.report(progress, 'checksum', '正在校验新版安装器')
        if count != manifest['bytes'] or digest(candidate) != manifest['sha256']:
            raise InstallationError('新版安装器大小或 SHA256 校验失败')
        script = stage / 'replace.ps1'
        shutil.copyfile(worker_script, script)
        config = stage / 'handoff.json'
        atomic_json(config, {'executable': str(executable), 'candidate': str(candidate),
                            'backup': str(stage / 'previous.exe'), 'ack': str(stage / 'ready.json'),
                            'result': str(stage / 'result.json'), 'target': str(Path(target).resolve()),
                            'old_sha256': digest(executable), 'sha256': manifest['sha256'],
                            'parent_pid': int(os.environ.get('TRAINER_SETUP_PID', os.getpid()))})
        return config
    except Exception:
        candidate.unlink(missing_ok=True)
        raise


def launch(config):
    config = Path(config).resolve(strict=True)
    powershell = Path(os.environ['SystemRoot']) / 'System32/WindowsPowerShell/v1.0/powershell.exe'
    return subprocess.Popen([str(powershell), '-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass',
                             '-File', str(config.parent / 'replace.ps1'), '-Config', str(config)],
                            creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))


def build_manifest(executable, version, output):
    executable = Path(executable)
    record = {'schema_version': 1, 'version': version, 'asset': executable.name,
              'bytes': executable.stat().st_size, 'sha256': digest(executable)}
    atomic_json(output, record)
    return record
