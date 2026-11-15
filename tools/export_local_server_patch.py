"""Export the current paired local server without bundling installed game data."""
from __future__ import annotations

import argparse
from contextlib import closing
import ast
from datetime import datetime, timezone
import hashlib
import importlib.metadata
import importlib.util
import json
import os
from pathlib import Path
import shutil
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
IMAGE = 'ShooterGame/Binaries/Win64/VALORANT-Win64-Shipping.exe'
SEEDS = ('trainer_patch', 'start_local_server_console', 'training_server', 'run_local_server',
         'run_native_baseline', 'training_host', 'training_hud', 'analyze_all_skins', 'native_oodle_directory')


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def copy(source, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)


def json_file(path, document):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(document, ensure_ascii=False, separators=(',', ':')) + '\n', encoding='utf-8')


def tool_closure():
    pending, selected = list(SEEDS), {}
    directories = (ROOT / 'tools', ROOT / 'work/extract', ROOT / 'work/duel1-respawn')
    while pending:
        name = pending.pop()
        if name in selected:
            continue
        path = next((folder / (name + '.py') for folder in directories if (folder / (name + '.py')).is_file()), None)
        if path is None:
            continue
        selected[name] = path
        for node in ast.walk(ast.parse(path.read_text(encoding='utf-8-sig'))):
            if isinstance(node, ast.Import):
                pending.extend(item.name.split('.')[0] for item in node.names)
            elif isinstance(node, ast.ImportFrom) and node.module:
                pending.append(node.module.split('.')[0])
    return sorted(selected.values())


def installation_manifest():
    project = json.loads((ROOT / 'project.json').read_text(encoding='utf-8'))
    source = Path(project['source_game_executable']).resolve(strict=True).parents[3]
    if digest(source / IMAGE) != project['expected_snapshot_sha256']:
        raise RuntimeError('Installed international client differs from the current build')
    loose = []
    for directory in ('ShooterGame/Binaries', 'Engine/Binaries', 'Engine/Content'):
        for path in sorted((ROOT / 'client' / directory).rglob('*')):
            if not path.is_file() or path.suffix.lower() in ('.log', '.dmp', '.pdb', '.pyc'):
                continue
            if any(part.lower() in ('saved', 'logs', 'crashes', '__pycache__') for part in path.parts):
                continue
            if path.name.lower().startswith('crashreportclient'):
                continue
            relative = path.relative_to(ROOT / 'client').as_posix()
            installed = source / relative
            record = {'file': relative, 'bytes': path.stat().st_size, 'sha256': digest(path)}
            if not installed.is_file() or installed.stat().st_size != record['bytes'] or digest(installed) != record['sha256']:
                raise RuntimeError('Tested runtime differs from source installation: ' + relative)
            loose.append(record)
    if not any(row['file'] == IMAGE for row in loose):
        raise RuntimeError('Client runtime manifest is incomplete')
    resources = []
    for path in sorted((source / 'ShooterGame/Content/Paks').iterdir()):
        if path.is_file() and path.suffix.lower() in ('.ucas', '.utoc', '.pak', '.sig'):
            row = {'file': path.name, 'bytes': path.stat().st_size}
            if path.suffix.lower() != '.ucas':
                row['sha256'] = digest(path)
            resources.append(row)
    return {'schema_version': 1, 'image': {'file': IMAGE, 'bytes': project['expected_snapshot_bytes'],
                                         'sha256': project['expected_snapshot_sha256']},
            'loose_files': loose, 'resources': resources,
            'resource_checks': 'Original UTOC/PAK/SIG whole-file SHA256 and UCAS sizes; original runtime block checks retained.'}


def command(path, arguments='', *, pause=False):
    value = ('@echo off\r\nsetlocal\r\nchcp 65001 >nul\r\ncd /d "%~dp0"\r\n'
             'set "TEMP=%~dp0.local\\runtime\\temp"\r\nset "TMP=%TEMP%"\r\n'
             'set "LOCALAPPDATA=%~dp0.local\\runtime\\LocalAppData"\r\n'
             'set "APPDATA=%~dp0.local\\runtime\\Roaming"\r\n'
             'set "UE-LocalDataCachePath=%~dp0.local\\runtime\\DerivedDataCache"\r\n'
             'for %%D in ("%TEMP%" "%LOCALAPPDATA%" "%APPDATA%" "%UE-LocalDataCachePath%") do if not exist "%%~D" mkdir "%%~D"\r\n'
             '"%~dp0runtime\\python.exe" -I -B -X utf8 "%~dp0tools\\trainer_patch.py" ' + arguments + ' %*\r\n'
             'set "trainerExitCode=%errorlevel%"\r\n' + ('pause\r\n' if pause else 'if not "%trainerExitCode%"=="0" pause\r\n') +
             'exit /b %trainerExitCode%\r\n')
    path.write_bytes(value.encode('ascii'))


def select_module(module_dir=None):
    import local_server_module
    directory = Path(module_dir or ROOT / '.local/local-server/module').resolve()
    record = local_server_module._verify_directory(directory)
    if module_dir is not None:
        current = local_server_module.capture_sources(ROOT)
        hashes = {name: hashlib.sha256(data).hexdigest() for name, data in current.items()}
        if hashes != record['source_sha256']:
            raise RuntimeError('Candidate native source closure differs from current project sources')
    return directory, record


def downloaded_metadata(destination, info, plan):
    import sqlite3
    if digest(plan) != info['plan_sha256']:
        raise RuntimeError('Resource plan differs from its descriptor')
    with closing(sqlite3.connect(plan.resolve().as_uri() + '?mode=ro&immutable=1', uri=True)) as database:
        database.row_factory = sqlite3.Row
        rows = [dict(row) for row in database.execute('SELECT path,size,sha256 FROM files ORDER BY id')]
    if len(rows) != info['file_count'] or sum(row['size'] for row in rows) != info['game_bytes']:
        raise RuntimeError('Resource plan totals differ from its descriptor')
    json_file(destination / 'profiles/downloaded-files.json', {
        'schema_version': 1, 'manifest_id': info['manifest_id'],
        'manifest_sha256': info['manifest_sha256'], 'languages': info['languages'], 'files': rows})
    minimal_path = ROOT / '.local/installer-build/resource-plan-minimal.json'
    if minimal_path.is_file():
        minimal = json.loads(minimal_path.read_text(encoding='utf-8'))
        json_file(destination / 'profiles/downloaded-files-minimal.json', {'schema_version': 1, 'manifest_id': '448DAE34A9D16C88', 'files': minimal['files']})
    for name in ('trainer_downloaded.py', 'resource_bundle.py'):
        copy(ROOT / 'tools' / name, destination / 'tools' / name)
    return dict(kind='local-server-downloaded-full-v1', distribution='complete-game-cdn-installer',
                manifest_id=info['manifest_id'], game_bytes=info['game_bytes'],
                game_files=info['file_count'], installation_cdn_tested=False)


def export(destination, *, module_dir=None, downloaded=False, release=None):
    from offline_gi_instance import verified_build as startup_verified
    from export_playtest import runtime
    module_path, module = select_module(module_dir)
    startup = startup_verified()
    if destination.exists():
        raise RuntimeError('Choose a new output directory; an existing export is never overwritten')
    manifest = installation_manifest()
    destination.mkdir(parents=True)
    for path in tool_closure():
        copy(path, destination / path.relative_to(ROOT))
    for path in (ROOT / 'tools').glob('*.json'):
        if path.name in ('humanlike_bot_spec.json', 'humanlike_rank_presets.json', 'parameter-explanations.json'):
            copy(path, destination / 'tools' / path.name)
    for name in ('app.js', 'human-bots.js', 'human-bots.css', 'index.html', 'style.css'):
        copy(ROOT / 'webui' / name, destination / 'webui' / name)
    for name in ('playable-heroes.json', 'humanlike-hold-positions.json'):
        copy(ROOT / 'profiles' / name, destination / 'profiles' / name)
    for name in ('all-weapons.json', 'all-skins.json', 'skin-effect-controls.json', 'training-maps.json', 'native-config-validation.json'):
        json_file(destination / 'reports' / name, json.loads((ROOT / 'reports' / name).read_text(encoding='utf-8')))
    maps = json.loads((ROOT / 'reports/training-maps.json').read_text(encoding='utf-8'))
    for row in maps['maps']:
        name = row.get('minimap', {}).get('relative_path')
        if name:
            path = (ROOT / name).resolve(strict=True)
            if ROOT / 'reports/training-map-images' != path.parent or path.suffix != '.png':
                raise RuntimeError('Unexpected map image path')
            copy(path, destination / name)
    copy(ROOT / '.local/native-baseline/portable-core/core-config.json',
         destination / '.local/native-baseline/portable-core/core-config.json')
    for path in (module_path / 'source-snapshot').rglob('*'):
        if path.is_file():
            copy(path, destination / '.local/local-server/module' / path.relative_to(module_path))
    for name in ('local_server_bridge.dll', 'build.json'):
        copy(module_path / name, destination / '.local/local-server/module' / name)
    for name in startup['sha256']:
        copy(ROOT / name, destination / name)
    copy(ROOT / '.local/offline-gi-instance/build.json', destination / '.local/offline-gi-instance/build.json')
    # The pair supervisor hashes these prior binaries to verify it never changes
    # them. They are not the candidate's gameplay module.
    for name in ('.local/native-module/trainer_bridge.dll', '.local/native-module/build.json',
                 '.local/local-network/module/network_probe.dll', '.local/local-network/module/build.json',
                 'work/client-analysis/BaseEngine.ini'):
        copy(ROOT / name, destination / name)
    for directory, names in (
            ('native_oodle', ('block_decode_worker.exe',)),
            ('native_oodle_mermaid', ('block_decode_worker.exe',)),
            ('native_oodle_leviathan', ('block_decode_worker.exe',))):
        for name in names:
            copy(ROOT / 'work/extract' / directory / name, destination / 'work/extract' / directory / name)
        copy(ROOT / 'work/extract' / directory / 'vendor/LICENSE',
             destination / 'work/extract' / directory / 'LICENSE')
    runtime(destination / 'runtime')
    pillow = importlib.util.find_spec('PIL')
    if not pillow or not pillow.origin:
        raise RuntimeError('Pillow is required for bundled original skin previews')
    shutil.copytree(Path(pillow.origin).parent, destination / 'runtime/Lib/site-packages/PIL',
                    ignore=shutil.ignore_patterns('__pycache__', '*.pyc'))
    distribution = importlib.metadata.distribution('Pillow')
    for item in distribution.files or ():
        if '.dist-info' in str(item) and ('license' in str(item).lower() or item.name == 'METADATA'):
            copy(distribution.locate_file(item), destination / 'runtime/Lib/site-packages' / str(item))
    settings = json.loads((ROOT / '.local/controller/local-server-candidate/humanlike-bots.json').read_text(encoding='utf-8'))
    from human_bot_integration import validate_config
    validate_config(settings)
    json_file(destination / 'profiles/patch-default-settings.json', settings)
    json_file(destination / 'profiles/patch-installation-manifest.json', manifest)
    json_file(destination / 'profiles/patch-project.json', {
        'name': 'ValorantTrainer-LocalServer-Patch', 'account_login': 'forbidden',
        'distribution_mode': 'portable-v1', 'resource_mode': 'installed-patch-readonly-v1',
        'game_executable': 'client/' + IMAGE, 'analysis_snapshot': 'client/' + IMAGE,
        'expected_snapshot_sha256': manifest['image']['sha256'], 'expected_snapshot_bytes': manifest['image']['bytes'],
        'active_development_map': 1})
    version = {'schema_version': 1, 'kind': 'local-server-installed-patch-v1',
               'created_utc': datetime.now(timezone.utc).isoformat(), 'module_sha256': module['module_sha256'],
               'client_image_sha256': manifest['image']['sha256'], 'client_image_bytes': manifest['image']['bytes'],
               'settings_revision': settings['revision'], 'game_resources_bundled': False,
               'system_python_required': False, 'compiler_required': False, 'account_login': False,
               'setup_client_copy_bytes': sum(row['bytes'] for row in manifest['loose_files']),
               'reused_resource_bytes': sum(row['bytes'] for row in manifest['resources']),
               'separate_device_tested': False, 'relocated_package_tested': False,
               'new_build_game_tested': False}
    if release:
        version['release'] = release
    if downloaded:
        info = json.loads((ROOT / '.local/installer-build/resource-plan.json').read_text(encoding='utf-8'))
        version.update(downloaded_metadata(destination, info, ROOT / '.local/installer-build/resource-plan.sqlite'))
        for name in ('apply_trainer_update.py', 'installer_engine.py', 'github_update.py'):
            copy(ROOT / 'tools' / name, destination / 'tools' / name)
        copy(ROOT / 'profiles/github-update.json', destination / 'profiles/github-update.json')
        (destination / '检查更新.cmd').write_bytes(
            ('@echo off\r\nchcp 65001 >nul\r\n"%~dp0runtime\\pythonw.exe" -I -B -X utf8 '
             '"%~dp0tools\\github_update.py" --gui --target "%~dp0."\r\n').encode('utf-8'))
        (destination / '更新现有训练器.cmd').write_bytes(
            ('@echo off\r\nchcp 65001 >nul\r\n"%~dp0runtime\\python.exe" -I -B -X utf8 '
             '"%~dp0tools\\apply_trainer_update.py" --package-root "%~dp0." %*\r\npause\r\n').encode('utf-8'))
    json_file(destination / 'patch-version.json', version)
    command(destination / '启动训练器.cmd')
    command(destination / '检查环境.cmd', '--check', pause=True)
    command(destination / '导出反馈日志.cmd', '--logs', pause=True)
    (destination / '使用说明.txt').write_text(
        '无畏契约离线训练器：本地服务器补丁测试版\n\n'
        '1. 把整个压缩包解压到独立文件夹，建议使用空间充足的非系统盘。不要覆盖国际服安装目录。\n'
        '2. 双击“启动训练器.cmd”。常见安装位置会自动识别；否则选择国际服 VALORANT 的 live 文件夹。\n'
        '3. 打开的 WebUI 地址为 http://127.0.0.1:8792 。点击开始进入训练器。\n'
        '   可选择架枪/个人乱斗、地图、武器与皮肤，并编辑每个英雄人机的参数和段位预设。\n'
        '   人机列表中的观战按钮跟随指定人机；观战后左右键切换目标。\n'
        '4. 结束时先在 WebUI 点击停止，再关闭启动窗口。\n\n'
        '运行条件\n'
        'Windows 10/11 64 位，已安装与本包完全匹配的国际服资源。运行环境已包含，无需安装 Python 或编译器。\n'
        '首次配置复制约 600 MB 必要运行文件到补丁目录，原游戏资源通过目录联接复用；不复制几十 GB 资源。\n'
        '建议预留至少 3 GB 给运行文件、缓存和测试日志。设置及日志都保存在补丁目录 .local。\n'
        '全程本机运行，不需要登录账号。游戏安装文件保持原样。\n'
        '本补丁不能用于任意更新版本：客户端、原资源索引/签名及文件大小会核对，更新国际服后可能需要新补丁。\n'
        f"匹配客户端：{manifest['image']['bytes']} 字节\nSHA-256：{manifest['image']['sha256']}\n\n"
        '测试范围\n'
        '这是当前开发版的导出，人机寻路和技能仍有未完善之处；导出不代表全部英雄技能与地图已验收。\n'
        '另一台真实设备尚待你测试。包内 patch-version.json 记录本次构建。\n\n'
        '排查\n'
        '启动失败时运行“检查环境.cmd”。报版本不匹配时不要跳过校验或覆盖原游戏。\n'
        '游戏目录搬迁、换盘或需要重新选择安装位置时，请将原 ZIP 解压到新的补丁文件夹。\n'
        '反馈问题时运行“导出反馈日志.cmd”，发送新生成的反馈日志 ZIP；不会收集原安装目录的账号文件。\n',
        encoding='utf-8-sig')
    if downloaded:
        (destination / '使用说明.txt').write_text(
            'ValorantTrainer 完整资源版更新包\n\n'
            '1. 结束训练器和游戏进程，将本包解压到一个新文件夹。\n'
            '2. 双击“更新现有训练器.cmd”，输入原完整资源版训练器所在文件夹。\n'
            '3. 更新成功后，回到原文件夹双击“启动训练器.cmd”。\n\n'
            '更新会保留原目录的设置、下载收据、反馈和游戏资源。可重复应用。\n'
            '资源版本为 448DAE34A9D16C88，资源由已有安装复用。\n'
            '本包完成静态与更新流程验证，三份反馈对应的真实游戏场景待实机复测。\n',
            encoding='utf-8-sig')
    # Mutable settings, install paths, cache, sessions, and original game bytes
    # have no place in this archive inventory.
    hashes = {p.relative_to(destination).as_posix(): digest(p) for p in sorted(destination.rglob('*')) if p.is_file()}
    json_file(destination / 'package-sha256.json', hashes)
    return version


def archive(destination, target, *, root_paths=False):
    if target.exists():
        raise RuntimeError('Archive destination already exists')
    inventory = json.loads((destination / 'package-sha256.json').read_text(encoding='utf-8'))
    paths = [*inventory, 'package-sha256.json']
    with zipfile.ZipFile(target, 'x', zipfile.ZIP_DEFLATED, compresslevel=6) as output:
        for name in paths:
            source = destination / name
            if name in inventory and digest(source) != inventory[name]:
                raise RuntimeError('Export changed before ZIP creation: ' + name)
            output.write(source, name if root_paths else destination.name + '/' + name)
    return {'archive': str(target), 'bytes': target.stat().st_size, 'sha256': digest(target), 'files': len(paths)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--archive', type=Path)
    parser.add_argument('--module-dir', type=Path)
    parser.add_argument('--downloaded', action='store_true')
    parser.add_argument('--release')
    args = parser.parse_args()
    destination = args.output.resolve()
    export(destination, module_dir=args.module_dir, downloaded=args.downloaded, release=args.release)
    result = archive(destination, args.archive.resolve(), root_paths=args.downloaded) if args.archive else {'directory': str(destination)}
    print(json.dumps(result, ensure_ascii=False))


if __name__ == '__main__':
    main()
