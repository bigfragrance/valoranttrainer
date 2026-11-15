"""Package the pinned complete-resource installer without contacting the CDN."""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import sqlite3
import subprocess
import sys
import zipfile

from installer_engine import atomic_json, digest, owned

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT/'.local/installer-build'
PAYLOAD = BUILD/'payload'
DONOR = ROOT/'exports/ValorantTrainer-Patch-20261109-r2'
DONOR_ZIP = DONOR.with_suffix('.zip')


def stage_patch(info, *, donor=DONOR, donor_sha256=None, release=None):
    donor = Path(donor).resolve()
    donor_archive = donor.with_suffix('.zip')
    expected = donor_sha256 or ('a5bf690bec4aadf5c384dbbcc62725986c1a457bb9ca197ddc423a0cc1673c7f' if donor == DONOR.resolve() else None)
    if not expected or digest(donor_archive) != expected:
        raise RuntimeError('The accepted patch archive has changed')
    stage = BUILD/'downloaded-patch'
    stage.mkdir(exist_ok=True)
    inventory = json.loads((donor/'package-sha256.json').read_text(encoding='utf-8'))
    for name, expected in inventory.items():
        source = owned(donor, name)
        if digest(source) != expected:
            raise RuntimeError('Accepted patch file changed: '+name)
        target = owned(stage, name)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
    for name in ('trainer_patch.py', 'trainer_downloaded.py', 'resource_bundle.py'):
        shutil.copyfile(ROOT/'tools'/name, stage/'tools'/name)
    db = sqlite3.connect((BUILD/'resource-plan.sqlite').as_uri()+'?mode=ro&immutable=1', uri=True)
    db.row_factory = sqlite3.Row
    rows = [dict(row) for row in db.execute('SELECT path,size,sha256 FROM files ORDER BY id')]
    db.close()
    atomic_json(stage/'profiles/downloaded-files.json', {'schema_version': 1,
        'manifest_id': info['manifest_id'], 'manifest_sha256': info['manifest_sha256'],
        'languages': info['languages'], 'files': rows})
    version = json.loads((stage/'patch-version.json').read_text(encoding='utf-8'))
    version.update(kind='local-server-downloaded-full-v1', distribution='complete-game-cdn-installer',
                   manifest_id=info['manifest_id'], game_bytes=info['game_bytes'],
                   game_files=info['file_count'], installation_cdn_tested=False)
    if release:
        version['release'] = release
    atomic_json(stage/'patch-version.json', version)
    (stage/'使用说明.txt').write_text(
        'ValorantTrainer 完整资源版\n\n'
        '双击“启动训练器.cmd”打开本机控制台，点击开始进入游戏。\n'
        '完整地图、英雄和枪皮资源，英语音频、繁体中文文本资源。\n'
        '设置、缓存和日志保存在本目录 .local。人机参数和皮肤可在 WebUI 调整。\n'
        '使用结束时，先在 WebUI 点击停止，再关闭启动窗口。\n\n'
        '文件缺失或校验失败时，重新打开安装器并选择本目录，使用“校验 / 修复”。\n'
        '资源版本固定为 448DAE34A9D16C88。\n', encoding='utf-8-sig')
    for name in ('github_update.py', 'apply_trainer_update.py', 'installer_engine.py'):
        shutil.copyfile(ROOT/'tools'/name, stage/'tools'/name)
    channel = ROOT/'profiles/github-update.json'
    shutil.copyfile(channel, stage/'profiles/github-update.json')
    (stage/'检查更新.cmd').write_text('@echo off\n"%~dp0runtime\\pythonw.exe" -I -B -X utf8 "%~dp0tools\\github_update.py" --gui --target "%~dp0."\n', encoding='utf-8-sig')
    packaged_names = set(inventory) | {'tools/github_update.py', 'tools/apply_trainer_update.py', 'tools/installer_engine.py', 'profiles/github-update.json', '检查更新.cmd'} | {'tools/trainer_downloaded.py', 'profiles/downloaded-files.json'}
    hashes = {name: digest(owned(stage, name)) for name in sorted(packaged_names)}
    atomic_json(stage/'package-sha256.json', hashes)
    archive = PAYLOAD/'patch.zip'
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as bundle:
        for name in (*hashes, 'package-sha256.json'):
            bundle.write(stage/name, name)
    with zipfile.ZipFile(archive) as bundle:
        assert bundle.testzip() is None
        size = sum(item.file_size for item in bundle.infolist())
    return digest(archive), size, version['module_sha256']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'exports/ValorantTrainer-Setup-20261113.exe')
    parser.add_argument('--donor', type=Path, default=DONOR)
    parser.add_argument('--donor-sha256')
    parser.add_argument('--release')
    args = parser.parse_args()
    destination = args.output.resolve()
    if ROOT not in destination.parents or destination.suffix.lower() != '.exe':
        raise ValueError('Installer output must be an EXE inside the project')
    destination.parent.mkdir(parents=True, exist_ok=True)
    BUILD.mkdir(exist_ok=True)
    PAYLOAD.mkdir(exist_ok=True)
    info = json.loads((BUILD/'resource-plan.json').read_text(encoding='utf-8'))
    assert digest(BUILD/'resource-plan.sqlite') == info['plan_sha256']
    patch_hash, patch_size, module_hash = stage_patch(info, donor=args.donor, donor_sha256=args.donor_sha256, release=args.release)
    shutil.copyfile(BUILD/'resource-plan.sqlite', PAYLOAD/'resource-plan.sqlite')
    shutil.copyfile(ROOT/'profiles/github-update.json', PAYLOAD/'github-update.json')
    shutil.copyfile('E:/Program Files/Git/mingw64/bin/libzstd.dll', PAYLOAD/'libzstd.dll')
    licenses = PAYLOAD/'licenses'
    licenses.mkdir(exist_ok=True)
    shutil.copyfile('E:/Program Files/Git/mingw64/share/licenses/zstd/LICENSE', licenses/'Zstandard-LICENSE.txt')
    shutil.copyfile(Path(sys.base_prefix)/'LICENSE.txt', licenses/'Python-LICENSE.txt')
    for name in ('tcl8.6', 'tk8.6'):
        path = Path(sys.base_prefix)/'tcl'/name/'license.terms'
        if path.is_file():
            shutil.copyfile(path, licenses/(name+'-LICENSE.txt'))
    descriptor = {**info, 'patch_sha256': patch_hash, 'patch_expanded_bytes': patch_size,
                  'module_sha256': module_hash, 'cdn_installation_tested': False,
                  'plan_profile': 'full-en-audio-zh-tw-text'}
    atomic_json(PAYLOAD/'installer.json', descriptor)
    environment = dict(os.environ)
    environment['PYINSTALLER_CONFIG_DIR'] = str(BUILD/'pyinstaller-cache')
    environment['TEMP'] = str(ROOT/'.local/runtime/temp')
    environment['TMP'] = environment['TEMP']
    environment['PYTHONPYCACHEPREFIX'] = str(ROOT/'.local/runtime/pycache')
    arguments = [sys.executable, '-B', '-m', 'PyInstaller', '--noconfirm', '--onefile', '--windowed',
                 '--name', 'TrainerSetupRuntime', '--distpath', str(BUILD/'pyi-dist'),
                 '--workpath', str(BUILD/'pyi-work'), '--specpath', str(BUILD),
                 '--paths', str(ROOT/'tools'), '--add-data', str(PAYLOAD)+';payload',
                 '--add-binary', str(PAYLOAD/'libzstd.dll')+';payload',
                 str(ROOT/'tools/installer_gui.py')]
    with (BUILD/'pyinstaller.log').open('w', encoding='utf-8') as log:
        process = subprocess.run(arguments, cwd=ROOT, env=environment, stdout=log, stderr=log,
                                 creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
    if process.returncode:
        raise RuntimeError('Installer runtime build failed; see '+str(BUILD/'pyinstaller.log'))
    runtime = BUILD/'pyi-dist/TrainerSetupRuntime.exe'
    manifest = BUILD/'setup.manifest'
    manifest.write_text('''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
  <assemblyIdentity version="1.0.0.0" processorArchitecture="amd64" name="ValorantTrainer.Setup" type="win32"/>
  <description>ValorantTrainer Setup</description>
  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3"><security><requestedPrivileges>
    <requestedExecutionLevel level="asInvoker" uiAccess="false"/>
  </requestedPrivileges></security></trustInfo>
</assembly>
''', encoding='utf-8')
    resource = BUILD/'setup.rc'
    resource.write_text('#include <windows.h>\n101 RCDATA "'+runtime.as_posix()+'"\n'
                        '1 RT_MANIFEST "'+manifest.as_posix()+'"\n', encoding='utf-8')
    compiler = Path('F:/VSBuildTools/2022/VC/Tools/MSVC/14.44.35207')
    sdk = Path('F:/zzz-rev/tools/sdk-nuget/headers/c')
    environment['INCLUDE'] = ';'.join(str(path) for path in (compiler/'include', sdk/'Include/10.0.26100.0/ucrt',
                                       sdk/'Include/10.0.26100.0/shared', sdk/'Include/10.0.26100.0/um'))
    environment['LIB'] = ';'.join(str(path) for path in (compiler/'lib/x64', sdk/'Lib/10.0.26100.0/ucrt/x64',
                                                      sdk/'Lib/10.0.26100.0/um/x64'))
    rc = sdk/'bin/10.0.26100.0/x64/rc.exe'
    if not rc.is_file():
        matches = list(Path('F:/').glob('zzz-rev/tools/sdk-nuget/**/rc.exe'))
        rc = next(path for path in matches if 'x64' in path.parts)
    resource_output = BUILD/'setup.res'
    result = subprocess.run([str(rc), '/nologo', '/fo', str(resource_output), str(resource)],
                            env=environment, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout+result.stderr)
    args = [str(compiler/'bin/Hostx64/x64/cl.exe'), '/nologo', '/std:c++20', '/W4', '/WX', '/EHsc', '/utf-8', '/O2', '/MT',
            '/DUNICODE', '/D_UNICODE', '/Fo:'+str(BUILD/'setup.obj'), str(ROOT/'packaging/setup_bootstrap.cpp'),
            str(resource_output), '/link', '/SUBSYSTEM:WINDOWS', '/MANIFEST:NO', '/OUT:'+str(destination),
            'user32.lib', 'shell32.lib', 'ole32.lib']
    result = subprocess.run(args, env=environment, capture_output=True, text=True)
    (BUILD/'bootstrap-build.log').write_text(result.stdout+result.stderr, encoding='utf-8')
    if result.returncode:
        raise RuntimeError(result.stdout+result.stderr)
    record = {'file': str(destination), 'bytes': destination.stat().st_size, 'sha256': digest(destination),
              'manifest_id': info['manifest_id'], 'game_files': info['file_count'],
              'game_bytes': info['game_bytes'], 'download_bytes': info['download_bytes'],
              'module_sha256': module_hash, 'cdn_installation_tested': False}
    atomic_json(BUILD/'delivery-build.json', record)
    destination.with_name(destination.name+'.sha256.txt').write_text(record['sha256']+'  '+destination.name+'\n', encoding='utf-8')
    print(json.dumps(record, ensure_ascii=False), flush=True)


if __name__ == '__main__':
    main()
