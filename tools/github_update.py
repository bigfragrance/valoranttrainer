"""Check and install updates published as GitHub Release assets."""
from __future__ import annotations

import argparse
from contextlib import contextmanager
import json
from pathlib import Path
import re
import tempfile
from urllib.parse import urlsplit
from urllib.request import Request, urlopen
import zipfile

from apply_trainer_update import apply, read, install_lock
from installer_engine import InstallationError, digest, owned

MANIFEST = 'trainer-update.json'


def version_key(value):
    # Existing packages use date versions, optionally with a numbered revision.
    raw = re.findall(r'\d+', str(value))
    if raw and len(raw[0]) == 8:
        raw = [raw[0][:4], raw[0][4:6], raw[0][6:8], *raw[1:]]
    numbers = tuple(int(number) for number in raw)
    if not numbers:
        raise InstallationError('版本号缺少数字')
    return numbers + (0,) * max(0, 5 - len(numbers))


def repository_name(value):
    if (not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_.-]*/[A-Za-z0-9][A-Za-z0-9_.-]*', value or '')
            or any(part in ('.', '..') for part in (value or '').split('/'))):
        raise InstallationError('请先配置 GitHub 仓库，格式为 owner/repository')
    return value


def fetch(url, *, limit, destination=None):
    parsed = urlsplit(url)
    if parsed.scheme != 'https' or parsed.hostname not in ('api.github.com', 'github.com'):
        raise InstallationError('更新地址必须来自 GitHub HTTPS')
    request = Request(url, headers={'User-Agent': 'ValorantTrainer-Updater',
                                   'Accept': 'application/vnd.github+json' if parsed.hostname == 'api.github.com' else 'application/octet-stream'})
    with urlopen(request, timeout=60) as response:
        if destination is None:
            data = response.read(limit + 1)
            if len(data) > limit:
                raise InstallationError('更新元数据超过大小限制')
            return json.loads(data)
        count = 0
        with Path(destination).open('wb') as output:
            while block := response.read(1024 * 1024):
                count += len(block)
                if count > limit:
                    raise InstallationError('更新包超过声明大小')
                output.write(block)
        return count


def check(target, repository=None):
    target = Path(target).resolve()
    config = target / 'profiles/github-update.json'
    repository = repository_name(repository or (read(config).get('repository') if config.is_file() else None))
    current = read(target / 'patch-version.json')
    release = fetch('https://api.github.com/repos/' + repository + '/releases/latest', limit=2_000_000)
    if release.get('draft') or release.get('prerelease'):
        raise InstallationError('更新渠道没有正式版本')
    assets = {asset['name']: asset for asset in release.get('assets', [])}
    if MANIFEST not in assets:
        raise InstallationError('GitHub Release 缺少更新清单')
    manifest = fetch(assets[MANIFEST]['browser_download_url'], limit=65536)
    if (manifest.get('schema_version') != 1 or manifest.get('manifest_id') != current.get('manifest_id')
            or not re.fullmatch(r'[0-9a-f]{64}', manifest.get('sha256', ''))
            or not isinstance(manifest.get('bytes'), int) or not 0 < manifest['bytes'] <= 2_000_000_000
            or manifest.get('asset') not in assets):
        raise InstallationError('更新清单不兼容或不完整')
    if version_key(manifest['version']) != version_key(release['tag_name']):
        raise InstallationError('发布版本与更新清单不一致')
    asset = assets[manifest['asset']]
    if asset.get('size') != manifest['bytes']:
        raise InstallationError('更新包大小与清单不一致')
    return {'available': version_key(manifest['version']) > version_key(current['release']),
            'current': current['release'], 'version': manifest['version'], 'repository': repository,
            'manifest': manifest, 'url': asset['browser_download_url']}




def install(target, release):
    if not release['available']:
        return {'release': release['current'], 'updated': False}
    manifest = release['manifest']
    with tempfile.TemporaryDirectory(prefix='trainer-github-') as temporary:
        temporary = Path(temporary)
        archive = temporary / 'update.zip'
        count = fetch(release['url'], limit=manifest['bytes'], destination=archive)
        if count != manifest['bytes'] or digest(archive) != manifest['sha256']:
            raise InstallationError('下载的更新包 SHA256 或大小校验失败')
        package = temporary / 'package'
        package.mkdir()
        with zipfile.ZipFile(archive) as bundle:
            entries = bundle.infolist()
            if (len(entries) > 100000 or sum(item.file_size for item in entries) > 4_000_000_000
                    or len(entries) != len({item.filename.casefold() for item in entries})):
                raise InstallationError('更新包归档超出限制或存在重复路径')
            for item in entries:
                if item.is_dir():
                    continue
                if (item.external_attr >> 16) & 0o170000 == 0o120000:
                    raise InstallationError('更新包包含符号链接')
                destination = owned(package, item.filename)
                destination.parent.mkdir(parents=True, exist_ok=True)
                with bundle.open(item) as incoming, destination.open('wb') as output:
                    import shutil
                    shutil.copyfileobj(incoming, output)
        if read(package / 'patch-version.json')['release'] != manifest['version']:
            raise InstallationError('更新包内版本与发布清单不一致')
        return apply(package, target)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--target', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--repository')
    parser.add_argument('--install', action='store_true')
    parser.add_argument('--gui', action='store_true')
    args = parser.parse_args()
    if args.gui:
        import tkinter as tk
        from tkinter import messagebox
        root = tk.Tk()
        root.withdraw()
        try:
            release = check(args.target, args.repository)
            if release['available']:
                if messagebox.askyesno('训练器更新', '发现版本 ' + release['version'] + '\n请关闭游戏和训练控制台后继续更新。'):
                    install(args.target, release)
                    messagebox.showinfo('训练器更新', '更新完成，请重新启动训练器。')
            else:
                messagebox.showinfo('训练器更新', '当前已是最新版本 ' + release['current'])
        except Exception as error:
            messagebox.showerror('训练器更新', str(error))
        finally:
            root.destroy()
    else:
        release = check(args.target, args.repository)
        print(json.dumps(install(args.target, release) if args.install else release, ensure_ascii=False))


if __name__ == '__main__':
    main()
