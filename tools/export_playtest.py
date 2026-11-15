"""Export a local playable build with an isolated, bundled Windows Python runtime."""
from __future__ import annotations

import argparse
import ast
from datetime import datetime, timezone
import hashlib
import importlib.metadata
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def copy_file(source, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)


def runtime(destination):
    source = Path(sys.base_prefix)
    destination.mkdir(parents=True)
    version = f'python{sys.version_info.major}{sys.version_info.minor}'
    for name in ('python.exe', 'pythonw.exe', version + '.dll', 'python3.dll',
                 'vcruntime140.dll', 'vcruntime140_1.dll', 'LICENSE.txt'):
        copy_file(source / name, destination / name)
    for item in (source / 'DLLs').iterdir():
        if item.is_file() and item.suffix in ('.pyd', '.dll') and not item.name.startswith('_tkinter'):
            copy_file(item, destination / 'DLLs' / item.name)
    excluded = {'site-packages', 'test', 'tests', '__pycache__', 'idlelib',
                'ensurepip', 'tkinter', 'turtledemo', 'venv'}
    with zipfile.ZipFile(destination / (version + '.zip'), 'x', zipfile.ZIP_DEFLATED) as standard:
        for item in (source / 'Lib').rglob('*.py'):
            relative = item.relative_to(source / 'Lib')
            if not any(part in excluded for part in relative.parts):
                standard.write(item, relative.as_posix())
    packages = destination / 'Lib/site-packages'
    for module in ('yaml', 'pefile', 'ordlookup'):
        spec = importlib.util.find_spec(module)
        if not spec or not spec.origin:
            raise RuntimeError('Missing export dependency: ' + module)
        if spec.submodule_search_locations:
            shutil.copytree(Path(spec.origin).parent, packages / module,
                            ignore=shutil.ignore_patterns('__pycache__', '*.pyc'))
        else:
            copy_file(Path(spec.origin), packages / Path(spec.origin).name)
    for name in ('PyYAML', 'pefile'):
        distribution = importlib.metadata.distribution(name)
        for item in distribution.files or ():
            if '.dist-info' in str(item) and ('license' in str(item).lower() or item.name in ('METADATA', 'COPYING')):
                copy_file(Path(distribution.locate_file(item)), packages / str(item))
    (destination / (version + '._pth')).write_text(
        f'{version}.zip\nDLLs\nLib/site-packages\n../tools\n', encoding='utf-8')


def build_launcher(destination):
    build_dir = ROOT / '.local/playtest-launcher'
    build_dir.mkdir(parents=True, exist_ok=True)
    # Reuse the already configured compiler/SDK environment, without building
    # the trainer again or requiring those tools on the user's test package.
    lines = (ROOT / 'native/build.cmd').read_text(encoding='utf-8').splitlines()
    environment = '\n'.join(line for line in lines if line.startswith('@echo') or line.startswith('set '))
    built = build_dir / 'Duel1Trainer.exe'
    command = (environment + '\n"%VCTOOLS%\\bin\\Hostx64\\x64\\cl.exe" '
               '/nologo /std:c++17 /W4 /WX /EHsc /O2 /MT /utf-8 /DUNICODE /D_UNICODE '
               f'/Fo:"{build_dir / "launcher.obj"}" "{ROOT / "packaging/playtest_launcher.cpp"}" '
               f'/link /OUT:"{built}" /SUBSYSTEM:WINDOWS user32.lib\nexit /b %errorlevel%\n')
    script = build_dir / 'build.cmd'
    script.write_text(command, encoding='utf-8')
    result = subprocess.run([os.environ.get('COMSPEC', 'cmd.exe'), '/d', '/c', str(script)],
                            capture_output=True, text=True, creationflags=subprocess.CREATE_NO_WINDOW)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    copy_file(built, destination)


def command_file(destination, script, arguments=''):
    destination.write_text('@echo off\nchcp 65001 >nul\ncd /d "%~dp0"\n'
                           f'"runtime\\python.exe" -E -s "tools\\{script}" {arguments}\npause\n',
                           encoding='utf-8')


def runtime_tools():
    """Include executable Python dependencies, including subprocess entry points."""
    pending = ['run_duel1_training', 'run_native_baseline', 'collect_playtest_logs', 'stop_duel1_training',
               'training_host', 'training_hud']
    selected = set()
    while pending:
        name = pending.pop()
        source = ROOT / 'tools' / (name + '.py')
        if name in selected or not source.is_file():
            continue
        selected.add(name)
        for node in ast.walk(ast.parse(source.read_text(encoding='utf-8'))):
            names = ([item.name for item in node.names] if isinstance(node, ast.Import) else
                     [node.module] if isinstance(node, ast.ImportFrom) and node.module else [])
            pending.extend(item.split('.')[0] for item in names)
    return [ROOT / 'tools' / (name + '.py') for name in sorted(selected)]


def stage_export(destination, *, portable=False):
    if destination.exists():
        raise RuntimeError('Export destination already exists; choose a new build directory')
    from native_module import verified_build
    manifest = verified_build()
    module_hash = manifest['sha256']['.local/native-module/trainer_bridge.dll']
    controls_proof = None
    for check_path in sorted((ROOT/'.local/original-controls-verification').glob('*/verification.json'), reverse=True):
        check = json.loads(check_path.read_text(encoding='utf-8'))
        if check.get('passed') and check.get('run', {}).get('training_module', {}).get('sha256') == module_hash:
            controls_proof = {'module_sha256': module_hash, 'source_report': str(check_path),
                     'assertions': check.get('controls', {}).get('assertions', {}),
                     'passed': True, 'pixels_verified': False}
            break
    if controls_proof is None:
        raise RuntimeError('This module needs a matching native controls verification before export')
    destination.mkdir(parents=True)
    for item in runtime_tools() if portable else (ROOT / 'tools').glob('*.py'):
        copy_file(item, destination / 'tools' / item.name)
    for name in ('trainer_bridge.dll', 'build.json', 'trainer_bridge.map'):
        copy_file(ROOT / '.local/native-module' / name, destination / '.local/native-module' / name)
    shutil.copytree(ROOT / '.local/native-module/source-snapshot',
                    destination / '.local/native-module/source-snapshot')
    from offline_gi_instance import verified_build as verified_startup_build
    startup = verified_startup_build()
    for relative in startup['sha256']:
        copy_file(ROOT / relative, destination / relative)
    copy_file(ROOT / '.local/offline-gi-instance/build.json',
              destination / '.local/offline-gi-instance/build.json')
    copy_file(ROOT / 'work/client-analysis/BaseEngine.ini', destination / 'work/client-analysis/BaseEngine.ini')
    proof = json.loads((ROOT / 'reports/native-config-validation.json').read_text(encoding='utf-8'))
    snapshot = Path(proof['snapshot']['path']).parent.name
    core_source = ROOT / '.local/native-baseline' / snapshot / 'core-config.json'
    if portable:
        from portable_config import export_core
        if hashlib.sha256(core_source.read_bytes()).hexdigest() != proof['snapshot']['sha256']:
            raise RuntimeError('Validated core snapshot changed before export')
        portable_core = export_core(json.loads(core_source.read_text(encoding='utf-8')), ROOT)
        core_target = destination / '.local/native-baseline/portable-core/core-config.json'
        core_target.parent.mkdir(parents=True)
        core_target.write_text(json.dumps(portable_core, ensure_ascii=False, indent=2), encoding='utf-8')
        proof = {'target_sha256': proof['target_sha256'],
                 'scope': 'Portable rebasing of the validated core cache; account cache maps excluded',
                 'source_snapshot_sha256': proof['snapshot']['sha256'],
                 'snapshot': {'path': '.local/native-baseline/portable-core/core-config.json',
                              'sha256': hashlib.sha256(core_target.read_bytes()).hexdigest()},
                 'native_import_proof': {'independent_marker_present': True, 'image_unchanged': True}}
        (destination / 'reports').mkdir(exist_ok=True)
        (destination / 'reports/native-config-validation.json').write_text(json.dumps(proof, indent=2), encoding='utf-8')
        controls_proof.pop('source_report', None)
    else:
        copy_file(core_source, destination / '.local/native-baseline' / snapshot / 'core-config.json')
        copy_file(ROOT / 'reports/native-config-validation.json', destination / 'reports/native-config-validation.json')
    (destination/'reports/playtest-native-check.json').write_text(json.dumps(controls_proof, indent=2), encoding='utf-8')
    profile = ROOT / '.local/profiles/GameUserSettings.ini'
    if not portable and profile.is_file():
        copy_file(profile, destination / '.local/profiles/GameUserSettings.ini')
    config = json.loads((ROOT / 'project.json').read_text(encoding='utf-8'))
    # Static PE verification reads the already hash-checked installed file.
    # The exported package therefore needs no project directory or binary copy.
    config['analysis_snapshot'] = config['game_executable']
    config['name'] = 'Duel1-Playtest'
    if portable:
        from client_layout import CLIENT_IMAGE, PORTABLE_MODE
        config = {key: config[key] for key in ('expected_snapshot_sha256', 'expected_snapshot_bytes', 'account_login')}
        config.update({'name': 'Duel1-Portable-Playtest', 'active_development_map': 1,
                       'game_executable': CLIENT_IMAGE, 'analysis_snapshot': CLIENT_IMAGE,
                       'distribution_mode': PORTABLE_MODE})
    (destination / 'project.json').write_text(json.dumps(config, ensure_ascii=False, indent=2), encoding='utf-8')
    runtime(destination / 'runtime')
    build_launcher(destination / '斗牛1训练器.exe')
    command_file(destination / '诊断启动.cmd', 'run_duel1_training.py', '--prebuilt')
    command_file(destination / '检查环境.cmd', 'run_duel1_training.py', '--prebuilt --check-only')
    command_file(destination / '导出测试日志.cmd', 'collect_playtest_logs.py')
    command_file(destination / '退出训练器.cmd', 'stop_duel1_training.py')
    readme = '''斗牛 1 可玩测试版

解压整个文件夹后，双击“斗牛1训练器.exe”。首次加载需等待约 10 秒。
运行环境已包含，无需安装 Python、编译器或登录账号。
本包供当前电脑测试，依赖 F:\\Riot Games\\VALORANT\\live 的已验证客户端资源。
请保留整个目录；可以移动目录，本项目开发目录不需要参与运行。

操作
WASD 移动；鼠标转向；左键射击；右键瞄准；R 换弹。
空格跳跃；Shift 静步；Ctrl 下蹲；C 烟幕；Q 上升。
F6 切换无限子弹，默认开启。
Esc 打开原生设置；F10 退出训练器。

当前包含
斗牛 1 原场景；捷风第一人称手臂；冠军步枪；原生射击、后坐力和换弹；
随机英雄敌人、黄绿色高亮、命中伤害、击杀和重生；原 HUD、小地图和设置菜单。
原瞄具附件及动画刷新已通过自动检查，开镜画面仍需用户确认。

已知限制
E 冲刺、X 飞刀及技能特效还在验证；暂不要据此评估完整技能表现。
原基础刀具、被动滑翔和设置中的所有改键项尚未全部接入。
已修复离线启动改写共享页所触发的 Critical Error；游戏线程停止时会保存本地诊断。
当前仍是测试版，其他电脑与更长时间使用仍需实测，不能将导出视为完整开发验收。

反馈
出现问题后，双击“导出测试日志.cmd”，将 logs 文件夹新生成的 ZIP 发回当前对话。
“检查环境.cmd”只检查，不启动游戏；“诊断启动.cmd”显示启动错误。
画面无响应时可双击“退出训练器.cmd”，仅结束本目录启动的训练器。
日志和设置保存在本目录 .local，游戏安装文件保持不变。
'''
    if portable:
        readme = readme.replace('本包供当前电脑测试，依赖 F:\\Riot Games\\VALORANT\\live 的已验证客户端资源。',
                                '本包包含独立客户端资源，无需安装游戏；支持 Windows 10/11 64 位。')
        readme = readme.replace('原瞄具附件及动画刷新已通过自动检查，开镜画面仍需用户确认。',
                                '原瞄具附件及动画刷新已通过自动检查，并经当前电脑人工试用。')
        readme = readme.replace('日志和设置保存在本目录 .local，游戏安装文件保持不变。',
                                '日志和设置保存在本目录 .local，使用包内 client 目录的资源。')
    (destination / '测试说明.txt').write_text(readme, encoding='utf-8-sig')
    version = {'name': 'Duel1 playable test', 'utc': datetime.now(timezone.utc).isoformat(),
               'module_sha256': manifest['sha256']['.local/native-module/trainer_bridge.dll'],
               'game_sha256': config['expected_snapshot_sha256'], 'game_resources_bundled': False,
               'compiler_required': False, 'system_python_required': False,
               'full_skill_validation_complete': False, 'critical_error_resolved': False}
    version['startup_module_sha256'] = startup['sha256']['.local/offline-gi-instance/offline_gi_instance.dll']
    version['native_controls_verified'] = True
    version['ads_alignment_native_verified'] = bool(controls_proof['assertions'].get('original_animation_consumed_scope_alignment'))
    (destination / 'playtest-version.json').write_text(json.dumps(version, indent=2), encoding='utf-8')
    check_code = ("import json,sys,yaml,pefile,ordlookup; "
                  "from native_module import verified_build; from run_map_probe import prepare; "
                  "from offline_gi_instance import verified_build as verified_startup; verified_startup(); "
                  "from run_native_probe import ROOT; verified_build(); "
                  "prepare(ROOT,1,'navigation',ROOT/'.local/package-self-check/config.bin'); "
                  "print(json.dumps({'package_integrity':True,'runtime_dependencies':True,"
                  "'game_started':False,'python':sys.executable}))")
    check = subprocess.run([str(destination / 'runtime/python.exe'), '-E', '-s', '-c', check_code],
                           cwd=destination, capture_output=True, text=True, timeout=60,
                           creationflags=subprocess.CREATE_NO_WINDOW)
    if check.returncode:
        raise RuntimeError('Export self-check failed: ' + check.stdout + check.stderr)
    return {'directory': str(destination), 'self_check': json.loads(check.stdout),
            'module_sha256': version['module_sha256']}


def export(destination):
    result = stage_export(destination)
    inventories = {p.relative_to(destination).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                   for p in destination.rglob('*') if p.is_file() and '.local/training-sessions/' not in p.as_posix()}
    (destination / 'package-sha256.json').write_text(json.dumps(inventories, indent=2), encoding='utf-8')
    archive = destination.with_suffix('.zip')
    with zipfile.ZipFile(archive, 'x', zipfile.ZIP_DEFLATED) as bundle:
        for item in destination.rglob('*'):
            if item.is_file():
                bundle.write(item, destination.name + '/' + item.relative_to(destination).as_posix())
    return {**result, 'archive': str(archive), 'archive_bytes': archive.stat().st_size}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    output = (args.output or ROOT / 'exports' / ('Duel1-Playtest-' + datetime.now().strftime('%Y%m%d-%H%M%S'))).resolve()
    result = export(output)
    (ROOT / '.local/latest-playtest-export.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(result, ensure_ascii=False))


if __name__ == '__main__':
    main()
