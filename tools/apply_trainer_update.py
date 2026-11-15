"""Apply a verified complete-resource trainer update to an existing installation."""
from __future__ import annotations

from contextlib import contextmanager
import argparse
import json
import os
import shutil
from pathlib import Path
import tempfile
import zipfile

from installer_engine import InstallationError, digest, extract_patch, owned

KIND = 'local-server-downloaded-full-v1'
IMMUTABLE_LOCAL = ('.local/local-server/module/', '.local/offline-gi-instance/',
                   '.local/native-module/', '.local/local-network/module/',
                   '.local/native-baseline/portable-core/')


def read(path):
    return json.loads(path.read_text(encoding='utf-8'))


def validate_package(package):
    version = read(package / 'patch-version.json')
    manifest = read(package / 'profiles/downloaded-files.json')
    build = read(package / '.local/local-server/module/build.json')
    if (version.get('kind') != KIND or not version.get('release')
            or version.get('manifest_id') != '448DAE34A9D16C88'
            or manifest.get('manifest_id') != version['manifest_id']
            or build.get('module_sha256') != version.get('module_sha256')):
        raise InstallationError('更新包版本或资源清单不一致')
    inventory = read(package / 'package-sha256.json')
    required = {'patch-version.json', 'profiles/downloaded-files.json',
                '.local/local-server/module/build.json', '.local/local-server/module/local_server_bridge.dll'}
    if not required <= inventory.keys():
        raise InstallationError('更新包文件清单不完整')
    if len(inventory) != len({name.casefold() for name in inventory}):
        raise InstallationError('更新包包含大小写重复路径')
    for name, expected in inventory.items():
        source = owned(package, name)
        lower = name.casefold()
        if (lower.startswith(('client/', '.setup/', '.git/'))
                or (lower.startswith('.local/') and not lower.startswith(IMMUTABLE_LOCAL))):
            raise InstallationError('更新包包含用户数据或游戏资源路径')
        if digest(source) != expected:
            raise InstallationError('更新包内容校验失败 · ' + name)
    module = package / '.local/local-server/module'
    if digest(module / 'local_server_bridge.dll') != version['module_sha256']:
        raise InstallationError('更新包模块校验失败')
    for name, expected in build['source_sha256'].items():
        source = owned(module / 'source-snapshot', name)
        relative = source.relative_to(package).as_posix()
        if relative not in inventory or digest(source) != expected:
            raise InstallationError('更新包模块源码快照校验失败')
    return version, inventory


@contextmanager
def install_lock(target):
    path = owned(target, '.setup/install.lock')
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('a+b') as handle:
        if path.stat().st_size == 0:
            handle.write(b'0')
            handle.flush()
        handle.seek(0)
        try:
            import msvcrt
            msvcrt.locking(handle.fileno(), msvcrt.LK_NBLCK, 1)
        except ImportError:
            import fcntl
            fcntl.flock(handle.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        except OSError as error:
            raise InstallationError('已有安装或更新任务正在运行') from error
        yield


def apply(package, target):
    with install_lock(Path(target).resolve()):
        return _apply(package, target)


def _apply(package, target):
    package, target = Path(package).resolve(), Path(target).resolve()
    version, inventory = validate_package(package)
    if target == package or target in package.parents or package in target.parents:
        raise InstallationError('请将更新包解压到原训练器目录外的新文件夹')
    previous = read(target / 'patch-version.json')
    if previous.get('kind') != KIND or previous.get('manifest_id') != version['manifest_id']:
        raise InstallationError('请选择资源版本匹配的完整资源版训练器目录')
    # A persistent journal allows rollback after an interrupted process as well.
    transaction = owned(target, '.setup/update-transaction')
    journal = transaction / 'journal.json'
    def recover():
        if not journal.is_file():
            return
        state = read(journal)
        for name in reversed(state['files']):
            destination = owned(target, name)
            backup = owned(transaction / 'backup', name)
            if name in state['existed']:
                destination.parent.mkdir(parents=True, exist_ok=True)
                temporary = destination.with_name(destination.name + '.rollback')
                shutil.copyfile(backup, temporary)
                os.replace(temporary, destination)
            else:
                destination.unlink(missing_ok=True)
        journal.unlink()
    recover()
    names = [*inventory, 'package-sha256.json']
    existed = []
    for name in names:
        destination = owned(target, name)
        if destination.exists():
            backup = owned(transaction / 'backup', name)
            backup.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(destination, backup)
            existed.append(name)
    from installer_engine import atomic_json
    atomic_json(journal, {'files': names, 'existed': existed})
    try:
        with tempfile.TemporaryDirectory(prefix='trainer-update-') as temporary:
            archive = Path(temporary) / 'patch.zip'
            with zipfile.ZipFile(archive, 'w', zipfile.ZIP_STORED) as bundle:
                for name in names:
                    bundle.write(owned(package, name), name)
            extract_patch(archive, digest(archive), target)
        for name, expected in inventory.items():
            if digest(owned(target, name)) != expected:
                raise InstallationError('更新后文件校验失败 · ' + name)
        journal.unlink()
    except BaseException:
        recover()
        raise
    return {'release': version['release'], 'module_sha256': version['module_sha256'],
            'files_verified': len(inventory), 'target': str(target)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package-root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--target', type=Path)
    args = parser.parse_args()
    target = args.target or Path(input('请输入原完整资源版训练器文件夹路径（可粘贴）\n').strip().strip('"'))
    print(json.dumps(apply(args.package_root, target), ensure_ascii=False))
    print('更新完成，请从原训练器文件夹启动。')


if __name__ == '__main__':
    main()
