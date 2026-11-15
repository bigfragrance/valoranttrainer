"""Configure and run the movable, exact-version local-server patch package."""
from __future__ import annotations

import argparse
import ctypes
from ctypes import wintypes as W
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import struct
import sys
import uuid
import zipfile

ROOT = Path(__file__).resolve().parents[1]
IMAGE = 'ShooterGame/Binaries/Win64/VALORANT-Win64-Shipping.exe'
SETTINGS = '.local/controller/local-server-candidate/humanlike-bots.json'
RESOURCE_MODE = 'installed-patch-readonly-v1'


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def read_json(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def write_json(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + '.' + uuid.uuid4().hex + '.tmp')
    try:
        temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def owned(root, relative):
    if not isinstance(relative, str):
        raise ValueError('补丁文件路径不合法')
    name = PurePosixPath(relative)
    if ('\\' in relative or ':' in relative
            or name.is_absolute() or '..' in name.parts or not name.parts):
        raise ValueError('补丁文件路径不合法')
    root = Path(root).resolve()
    target = root.joinpath(*name.parts)
    if target.resolve() != target or root not in target.parents:
        raise ValueError('补丁文件路径指向包外，已停止：' + relative)
    return target


def verify_package(root=ROOT):
    root = Path(root).resolve()
    version = read_json(root / 'patch-version.json')
    inventory = read_json(root / 'package-sha256.json')
    if version.get('kind') not in ('local-server-installed-patch-v1','local-server-standalone-lite-v1',
                                   'local-server-downloaded-full-v1') or not isinstance(inventory, dict):
        raise RuntimeError('这不是有效的本地服务器补丁包')
    for name, expected in inventory.items():
        if digest(owned(root, name)) != expected:
            raise RuntimeError('补丁文件缺失或已改变，请重新解压：' + name)
    return version


def installation_root(path):
    path = Path(path).expanduser().resolve(strict=True)
    if path.is_file() and path.name.casefold() == 'valorant-win64-shipping.exe':
        path = path.parents[3]
    for candidate in (path, path / 'live', path / 'VALORANT/live'):
        if (candidate / IMAGE).is_file():
            return candidate.resolve(strict=True)
    raise RuntimeError('请选择包含 ShooterGame 文件夹的国际服 live 目录')


def validate_installation(source, manifest, *, root=ROOT):
    source, root = installation_root(source), Path(root).resolve()
    if source == root or source in root.parents or root in source.parents:
        raise RuntimeError('请把补丁解压到游戏安装目录以外的独立文件夹')
    image = source / IMAGE
    expected = manifest['image']
    if image.stat().st_size != expected['bytes'] or digest(image) != expected['sha256']:
        raise RuntimeError('国际服客户端版本不匹配。此补丁仅支持包内说明指定的同一版本；请勿覆盖游戏文件或跳过校验。')
    for item in manifest['loose_files']:
        path = source / item['file']
        if not path.is_file() or path.stat().st_size != item['bytes'] or digest(path) != item['sha256']:
            raise RuntimeError('国际服运行文件缺失或版本不同：' + item['file'])
    paks = (source / 'ShooterGame/Content/Paks').resolve(strict=True)
    if not paks.is_dir():
        raise RuntimeError('国际服资源目录不存在')
    for item in manifest['resources']:
        name = item['file']
        if Path(name).name != name:
            raise ValueError('无效的资源清单')
        path = paks / name
        if not path.is_file() or path.stat().st_size != item['bytes']:
            raise RuntimeError('国际服资源缺失或版本不同：' + name)
        if item.get('sha256') and digest(path) != item['sha256']:
            raise RuntimeError('国际服资源索引或签名版本不同：' + name)
    return source, paks


def create_junction(target, source):
    """Create one new package-owned NTFS junction without a command shell."""
    target, source = Path(target), Path(source).resolve(strict=True)
    if os.name != 'nt':
        raise RuntimeError('补丁需要 Windows 10/11 64 位和支持目录联接的磁盘')
    if target.exists() or target.is_symlink():
        if target.is_dir() and target.resolve() == source and target.lstat().st_file_attributes & 0x400:
            return
        raise RuntimeError('资源目录已存在且不是所选安装的联接。请使用一个新解压的补丁文件夹。')
    target.parent.mkdir(parents=True, exist_ok=True)
    target.mkdir()
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.CreateFileW.argtypes = [W.LPCWSTR, W.DWORD, W.DWORD, ctypes.c_void_p, W.DWORD, W.DWORD, W.HANDLE]
    kernel.CreateFileW.restype = W.HANDLE
    kernel.DeviceIoControl.argtypes = [W.HANDLE, W.DWORD, ctypes.c_void_p, W.DWORD,
                                      ctypes.c_void_p, W.DWORD, ctypes.POINTER(W.DWORD), ctypes.c_void_p]
    kernel.DeviceIoControl.restype = W.BOOL
    kernel.CloseHandle.argtypes = [W.HANDLE]
    handle = kernel.CreateFileW(str(target), 0x40000000, 3, None, 3, 0x02200000, None)
    try:
        if handle == W.HANDLE(-1).value:
            raise ctypes.WinError(ctypes.get_last_error())
        substitute = ('\\??\\' + str(source)).encode('utf-16-le')
        printed = str(source).encode('utf-16-le')
        paths = substitute + b'\0\0' + printed + b'\0\0'
        data = struct.pack('<I6H', 0xA0000003, 8 + len(paths), 0, 0, len(substitute),
                           len(substitute) + 2, len(printed)) + paths
        buffer = ctypes.create_string_buffer(data)
        returned = W.DWORD()
        if not kernel.DeviceIoControl(handle, 0x900A4, buffer, len(data), None, 0, ctypes.byref(returned), None):
            raise ctypes.WinError(ctypes.get_last_error())
    except BaseException:
        if handle != W.HANDLE(-1).value:
            kernel.CloseHandle(handle)
            handle = W.HANDLE(-1).value
        target.rmdir()  # Only the exact newly created empty directory.
        raise
    finally:
        if handle != W.HANDLE(-1).value:
            kernel.CloseHandle(handle)
    if target.resolve(strict=True) != source:
        raise RuntimeError('资源联接验证失败')


def bind_installation(source, *, root=ROOT):
    root = Path(root).resolve()
    manifest = read_json(root / 'profiles/patch-installation-manifest.json')
    source, paks = validate_installation(source, manifest, root=root)
    client = owned(root, 'client')
    client.mkdir(exist_ok=True)
    # An old junction is never removed or redirected automatically.
    target_paks = client / 'ShooterGame/Content/Paks'
    if target_paks.exists() and target_paks.resolve() != paks:
        raise RuntimeError('补丁已绑定另一处游戏资源。请重新解压到新文件夹后选择新的安装目录。')
    copied = 0
    for item in manifest['loose_files']:
        target = owned(root, 'client/' + item['file'])
        if target.is_file() and target.stat().st_size == item['bytes'] and digest(target) == item['sha256']:
            continue
        target.parent.mkdir(parents=True, exist_ok=True)
        temporary = target.with_name(target.name + '.' + uuid.uuid4().hex + '.copying')
        try:
            shutil.copy2(source / item['file'], temporary)
            if digest(temporary) != item['sha256']:
                raise RuntimeError('复制时源文件发生变化，请关闭国际服更新后重试')
            os.replace(temporary, target)
        finally:
            temporary.unlink(missing_ok=True)
        copied += item['bytes']
    create_junction(target_paks, paks)
    config = read_json(root / 'profiles/patch-project.json')
    config.update(source_game_executable=str(source / IMAGE), source_resource_directory=str(paks),
                  resource_mode=RESOURCE_MODE)
    write_json(root / 'project.json', config)
    target = owned(root, SETTINGS)
    if not target.exists():
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(root / 'profiles/patch-default-settings.json', target)
    receipt = {'schema_version': 1, 'source_root': str(source), 'resource_source': str(paks),
               'copied_client_bytes': sum(row['bytes'] for row in manifest['loose_files']),
               'copied_this_time_bytes': copied, 'resources_copied_bytes': 0,
               'source_files_modified': False, 'configured_utc': datetime.now(timezone.utc).isoformat()}
    write_json(root / '.local/patch-installation.json', receipt)
    return receipt


def auto_installations():
    if os.name != 'nt':
        return []
    mask = ctypes.windll.kernel32.GetLogicalDrives()
    return [Path(chr(65 + n) + ':/Riot Games/VALORANT/live') for n in range(26)
            if mask & (1 << n) and Path(chr(65 + n) + ':/Riot Games/VALORANT/live', IMAGE).is_file()]


def choose_directory():
    class BrowseInfo(ctypes.Structure):
        _fields_ = [('owner', W.HWND), ('root', ctypes.c_void_p), ('display', W.LPWSTR),
                    ('title', W.LPCWSTR), ('flags', W.UINT), ('callback', ctypes.c_void_p),
                    ('parameter', W.LPARAM), ('image', ctypes.c_int)]
    shell = ctypes.WinDLL('shell32')
    ole = ctypes.OleDLL('ole32')
    shell.SHBrowseForFolderW.argtypes = [ctypes.POINTER(BrowseInfo)]
    shell.SHBrowseForFolderW.restype = ctypes.c_void_p
    shell.SHGetPathFromIDListW.argtypes = [ctypes.c_void_p, W.LPWSTR]
    shell.SHGetPathFromIDListW.restype = W.BOOL
    ole.CoTaskMemFree.argtypes = [ctypes.c_void_p]
    initialized = ole.CoInitializeEx(None, 2) in (0, 1)
    display, path = ctypes.create_unicode_buffer(32768), ctypes.create_unicode_buffer(32768)
    info = BrowseInfo(None, None, ctypes.cast(display, W.LPWSTR),
                      '选择同版本国际服 VALORANT 的 live 文件夹（包含 ShooterGame）', 0x51, None, 0, 0)
    result = None
    try:
        result = shell.SHBrowseForFolderW(ctypes.byref(info))
        return Path(path.value) if result and shell.SHGetPathFromIDListW(result, path) else None
    finally:
        if result:
            ole.CoTaskMemFree(result)
        if initialized:
            ole.CoUninitialize()


def configured_source(root=ROOT):
    path = Path(root) / '.local/patch-installation.json'
    return read_json(path).get('source_root') if path.is_file() else None


def check(root=ROOT):
    root = Path(root).resolve()
    version = verify_package(root)
    if version.get('kind')=='local-server-downloaded-full-v1':
        from trainer_downloaded import check as check_downloaded
        return check_downloaded(root)
    if version.get('kind')=='local-server-standalone-lite-v1':
        from trainer_dependencies import check as check_standalone
        return check_standalone(root)
    source = configured_source(root)
    if not source:
        raise RuntimeError('尚未配置游戏目录，请先双击“启动训练器.cmd”')
    manifest = read_json(root / 'profiles/patch-installation-manifest.json')
    source, paks = validate_installation(source, manifest, root=root)
    config = read_json(root / 'project.json')
    if config.get('resource_mode') != RESOURCE_MODE or Path(config.get('source_resource_directory', '')).resolve() != paks:
        raise RuntimeError('资源配置与已选择的安装不一致，请重新解压补丁')
    target = root / 'client/ShooterGame/Content/Paks'
    if target.resolve(strict=True) != paks or not target.lstat().st_file_attributes & 0x400:
        raise RuntimeError('游戏资源联接无效，请重新解压并配置')
    for item in manifest['loose_files']:
        if digest(owned(root, 'client/' + item['file'])) != item['sha256']:
            raise RuntimeError('补丁运行文件已改变，请重新运行启动入口修复：' + item['file'])
    from local_server_module import verified_build
    from offline_gi_instance import verified_build as startup_build
    if verified_build()['module_sha256'] != version['module_sha256']:
        raise RuntimeError('本地服务器模块与补丁版本不一致')
    startup_build()
    from start_local_server_console import configuration_plan
    from local_server_controller import check_candidate_config
    check_candidate_config(configuration_plan(root)['configuration'], root=root)
    return {'passed': True, 'module_sha256': version['module_sha256'],
            'source_root': str(source), 'resources_shared': True,
            'game_started': False, 'account_login': False, 'system_python_required': False}


def collect_logs(root=ROOT):
    root = Path(root).resolve()
    destination = root / '反馈日志'
    destination.mkdir(exist_ok=True)
    archive = destination / ('trainer-feedback-' + datetime.now().strftime('%Y%m%d-%H%M%S') + '.zip')
    candidates = [root / 'patch-version.json', root / '.local/patch-installation.json']
    pairs = root / '.local/local-server'
    latest = max(pairs.glob('pair-*'), key=lambda p: p.stat().st_mtime, default=None)
    if latest:
        candidates.extend(latest.rglob('*'))
        for role in ('host', 'client'):
            for result in (latest / (role + '-result.json'), latest / (role + '-result.active.json')):
                if result.is_file():
                    record = read_json(result)
                    run = Path(record.get('run_dir') or Path(record.get('report_path', '')).parent).resolve()
                    boundary = (root / '.local/native-baseline').resolve()
                    if boundary in run.parents and run.is_dir():
                        candidates.extend(run.rglob('*'))
    written, total = set(), 0
    with zipfile.ZipFile(archive, 'x', zipfile.ZIP_DEFLATED) as output:
        for path in candidates:
            if not path.is_file() or path.suffix not in ('.json', '.jsonl', '.log', '.txt'):
                continue
            if path.resolve() != path or root not in path.parents or path.stat().st_size > 8 * 1024 * 1024:
                continue
            name = path.relative_to(root).as_posix()
            if name in written or total + path.stat().st_size > 40 * 1024 * 1024:
                continue
            output.write(path, name)
            written.add(name)
            total += path.stat().st_size
    return archive


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-dir', type=Path)
    parser.add_argument('--choose', action='store_true')
    parser.add_argument('--configure-only', action='store_true')
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--logs', action='store_true')
    parser.add_argument('--dependencies', type=Path)
    args = parser.parse_args(argv)
    try:
        if args.logs:
            print('反馈日志：' + str(collect_logs()))
            return 0
        if args.check:
            print(json.dumps(check(), ensure_ascii=False, indent=2))
            return 0
        version=verify_package()
        if version.get('kind')=='local-server-downloaded-full-v1':
            from trainer_downloaded import prepare as prepare_downloaded
            result=prepare_downloaded(ROOT)
            if args.configure_only:
                print(json.dumps({'complete': True, 'manifest_id': result['manifest_id'],
                                  'game_files': len(result['files'])}, ensure_ascii=False))
                return 0
            from local_environment import project_environment
            os.environ.update(project_environment(ROOT))
            from start_local_server_console import main as console_main
            return console_main(['--open'])
        if version.get('kind')=='local-server-standalone-lite-v1':
            from trainer_dependencies import prepare
            result=prepare(ROOT,args.dependencies)
            if args.configure_only:
                print(json.dumps({'complete':result['complete'],'resource_files':len(result['files']),
                                  'installed_game_required':False},ensure_ascii=False))
                return 0
            from local_environment import project_environment
            os.environ.update(project_environment(ROOT))
            from start_local_server_console import main as console_main
            return console_main(['--open'])
        source = args.game_dir or (None if args.choose else configured_source())
        if not source and not args.choose:
            matches = auto_installations()
            source = matches[0] if len(matches) == 1 else None
        if source is None:
            print('请选择已安装的同版本国际服资源目录。')
            source = choose_directory()
            if source is None:
                print('已取消，未启动游戏。')
                return 0
        print('正在检查国际服版本并准备独立运行文件；首次配置约占用 600 MB。')
        receipt = bind_installation(source)
        print('资源已就绪，复用原安装资源，设置和日志保存在补丁目录。')
        if args.configure_only:
            print(json.dumps(receipt, ensure_ascii=False, indent=2))
            return 0
        from local_environment import project_environment
        os.environ.update(project_environment(ROOT))
        from start_local_server_console import main as console_main
        return console_main(['--open'])
    except (OSError, RuntimeError, ValueError, KeyError) as error:
        print('启动失败：' + str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
