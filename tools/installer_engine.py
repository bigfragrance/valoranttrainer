"""Pinned, resumable chunk downloads directly into an owned trainer folder."""
from __future__ import annotations

import base64
from collections import OrderedDict, defaultdict
from concurrent.futures import ThreadPoolExecutor
import ctypes
from datetime import datetime, timezone
import hashlib
import http.client
import json
import os
from pathlib import Path, PurePosixPath
import queue
import re
import shutil
import sqlite3
import ssl
import threading
import time
from urllib.parse import unquote, urlsplit
from urllib.request import getproxies, proxy_bypass
import uuid
import zipfile


class InstallationError(RuntimeError):
    pass


class Stopped(InstallationError):
    pass


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def atomic_json(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name+'.'+uuid.uuid4().hex+'.tmp')
    try:
        temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def relative_name(value):
    if not isinstance(value, str) or not value or '\\' in value or ':' in value:
        raise InstallationError('安装清单中的文件路径不合法')
    path = PurePosixPath(value)
    if path.is_absolute() or any(part in ('', '.', '..') for part in value.split('/')):
        raise InstallationError('安装清单中的文件路径不合法')
    reserved = {'con', 'prn', 'aux', 'nul', *(f'com{n}' for n in range(1, 10)), *(f'lpt{n}' for n in range(1, 10))}
    for part in path.parts:
        if part.endswith(('.', ' ')) or any(ord(c)<32 or c in '<>"|?*' for c in part):
            raise InstallationError('安装清单中的 Windows 文件名不合法')
        if part.split('.', 1)[0].casefold() in reserved:
            raise InstallationError('安装清单使用了保留文件名')
    return path


def owned(root, relative):
    root = Path(root).resolve()
    path = root.joinpath(*relative_name(relative).parts)
    if path.resolve() != path or root not in path.parents:
        raise InstallationError('安装路径包含指向其他位置的联接或链接，请选择新的安装目录')
    return path


def allocated_bytes(path):
    if not path.is_file():
        return 0
    if os.name != 'nt':
        stat = path.stat()
        return getattr(stat, 'st_blocks', (stat.st_size+511)//512)*512
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.GetCompressedFileSizeW.argtypes = [ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_ulong)]
    kernel.GetCompressedFileSizeW.restype = ctypes.c_ulong
    high = ctypes.c_ulong()
    ctypes.set_last_error(0)
    low = kernel.GetCompressedFileSizeW(str(path), ctypes.byref(high))
    if low == 0xffffffff and ctypes.get_last_error():
        return 0
    return (high.value<<32)|low


class Control:
    def __init__(self):
        self.running = threading.Event()
        self.running.set()
        self.stopped = threading.Event()

    def pause(self):
        self.running.clear()

    def resume(self):
        self.running.set()

    def stop(self):
        self.stopped.set()
        self.running.set()

    def checkpoint(self):
        while not self.running.wait(.2):
            if self.stopped.is_set():
                raise Stopped('已保存进度')
        if self.stopped.is_set():
            raise Stopped('已保存进度')

    def delay(self, seconds):
        end = time.monotonic()+seconds
        while time.monotonic() < end:
            self.checkpoint()
            if self.stopped.wait(min(.2, max(0, end-time.monotonic()))):
                raise Stopped('已保存进度')


class Plan:
    def __init__(self, path, expected_sha256):
        self.path = Path(path)
        if digest(self.path) != expected_sha256:
            raise InstallationError('安装器资源计划校验失败，请重新获取安装器')
        self.db = sqlite3.connect(self.path.resolve().as_uri()+'?mode=ro&immutable=1', uri=True)
        self.db.row_factory = sqlite3.Row
        self.meta = {row['key']: json.loads(row['value']) for row in self.db.execute('SELECT * FROM meta')}
        self.files = [dict(row) for row in self.db.execute('SELECT * FROM files ORDER BY id')]
        if self.meta.get('schema_version') != 1 or self.meta.get('file_count') != len(self.files):
            raise InstallationError('安装资源计划版本不受支持')
        seen = set()
        for item in self.files:
            relative_name(item['path'])
            name = item['path'].casefold()
            if name in seen or item['size'] < 0 or not re.fullmatch('[0-9a-f]{64}', item['sha256']):
                raise InstallationError('安装资源计划不一致')
            seen.add(name)
        if sum(item['size'] for item in self.files) != self.meta['game_bytes']:
            raise InstallationError('安装资源大小统计不一致')

    def close(self):
        self.db.close()

    def chunks(self, file_id):
        return [dict(row) for row in self.db.execute('''
            SELECT fc.ordinal,fc.offset AS file_offset,c.* FROM file_chunks fc
            JOIN chunks c ON c.id=fc.chunk_id WHERE fc.file_id=? ORDER BY fc.ordinal
        ''', (file_id,))]


class Zstd:
    def __init__(self, path):
        self.library = ctypes.CDLL(str(Path(path).resolve()))
        self.library.ZSTD_decompress.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_size_t]
        self.library.ZSTD_decompress.restype = ctypes.c_size_t
        self.library.ZSTD_isError.argtypes = [ctypes.c_size_t]
        self.library.ZSTD_isError.restype = ctypes.c_uint

    def decode(self, data, size):
        if not 0 < size <= 64*1024*1024:
            raise InstallationError('下载块的解压大小不合法')
        incoming = ctypes.create_string_buffer(data)
        output = ctypes.create_string_buffer(size)
        result = self.library.ZSTD_decompress(output, size, incoming, len(data))
        if self.library.ZSTD_isError(result) or result != size:
            raise InstallationError('下载块解压校验失败，进度已保留，可重新继续')
        return output.raw[:size]


CDN_ROUTES = {'primary': None, 'cloudflare': 'fallback2.riotcdn.net',
              'cloudfront': 'fallback3.riotcdn.net', 'akamai': 'fallback1.riotcdn.net'}


class CdnHTTPSConnection(http.client.HTTPSConnection):
    """Route the socket or CONNECT tunnel while authenticating the original CDN."""
    def __init__(self, host, port, *, origin, context):
        super().__init__(host, port, timeout=20, context=context)
        self.origin = origin

    def connect(self):
        http.client.HTTPConnection.connect(self)
        self.sock = self._context.wrap_socket(self.sock, server_hostname=self.origin)


class RangeTransport:
    def __init__(self, control, *, base_url='https://valorant.dyn.riotcdn.net/channels/public/bundles/',
                 allow_loopback=False, transferred=None, network_mode='direct', cdn_route='primary'):
        self.control = control
        self.base = urlsplit(base_url)
        allowed = self.base.scheme == 'https' and self.base.hostname == 'valorant.dyn.riotcdn.net'
        local = allow_loopback and self.base.scheme == 'http' and self.base.hostname in ('127.0.0.1', '::1')
        if not (allowed or local) or self.base.username is not None or self.base.query or self.base.fragment:
            raise InstallationError('资源下载服务器地址不合法')
        self.local = threading.local()
        self.connections = set()
        self.connection_lock = threading.Lock()
        self.transferred = transferred or (lambda count: None)
        self.context = ssl.create_default_context() if self.base.scheme == 'https' else None
        if cdn_route not in ('auto', *CDN_ROUTES):
            raise InstallationError('CDN 线路选项不受支持')
        self.cdn_route = 'primary' if local else cdn_route
        self.routes = ['primary'] if local else list(CDN_ROUTES)
        self.route_stats = {key: {'assignments': 0, 'bytes': 0, 'read_seconds': 0.0, 'errors': 0} for key in self.routes}
        self.route_retry_at = dict.fromkeys(self.routes, 0.0)
        self.proxy = None
        if network_mode not in ('direct', 'system'):
            raise InstallationError('网络连接选项不受支持')
        if network_mode == 'system' and not local:
            proxies = getproxies()
            address = proxies.get('https') or proxies.get('all')
            if address and not proxy_bypass(self.base.hostname):
                self.proxy = urlsplit(address if '://' in address else 'http://'+address)
                if self.proxy.scheme != 'http' or not self.proxy.hostname:
                    raise InstallationError('系统代理需要 HTTP CONNECT 支持，请选择直连或配置 HTTP 代理')
        self.metrics = {'requests': 0, 'retries': 0, 'errors': 0, 'received_bytes': 0,
                        'proxy_used': bool(self.proxy), 'endpoint': self.base.hostname,
                        'header_wait_seconds': 0.0, 'http_statuses': {}}

    def disconnect(self):
        connection = getattr(self.local, 'connection', None)
        if connection:
            connection.close()
            with self.connection_lock:
                self.connections.discard(connection)
        self.local.connection = None

    def close(self):
        with self.connection_lock:
            for connection in self.connections:
                connection.close()
            self.connections.clear()

    def choose_route(self, excluded=()):
        if self.cdn_route != 'auto':
            return self.cdn_route
        with self.connection_lock:
            available = [key for key in self.routes if key not in excluded
                         and self.route_retry_at[key] <= time.monotonic()]
            available = available or [key for key in self.routes if key not in excluded] or self.routes
            unsampled = [key for key in available if self.route_stats[key]['assignments'] == 0]
            measured = [key for key in available if self.route_stats[key]['bytes'] >= 128*1024]
            if unsampled:
                chosen = unsampled[0]
            elif measured:
                chosen = max(measured, key=self.route_speed)
            else:
                chosen = min(available, key=lambda key: self.route_stats[key]['assignments'])
            self.route_stats[chosen]['assignments'] += 1
            return chosen

    def route_speed(self, key):
        stats = self.route_stats[key]
        return stats['bytes']/max(stats['read_seconds'], .001)

    def faster_route_available(self, route):
        if self.cdn_route != 'auto' or self.local.request_read_seconds < 1:
            return False
        speed = self.local.request_received/self.local.request_read_seconds
        with self.connection_lock:
            return any(key != route and self.route_retry_at[key] <= time.monotonic()
                       and stats['bytes'] >= 128*1024 and self.route_speed(key) > speed*2
                       for key, stats in self.route_stats.items())

    def connection(self, route='primary'):
        if getattr(self.local, 'route', None) != route:
            self.disconnect()
        if getattr(self.local, 'connection', None) is None:
            if self.base.scheme == 'https':
                endpoint = CDN_ROUTES[route] or self.base.hostname
                host = self.proxy.hostname if self.proxy else endpoint
                port = (self.proxy.port or 80) if self.proxy else self.base.port
                self.local.connection = CdnHTTPSConnection(host, port, origin=self.base.hostname, context=self.context)
                if self.proxy:
                    headers = {}
                    if self.proxy.username is not None:
                        credentials = unquote(self.proxy.username)+':'+unquote(self.proxy.password or '')
                        headers['Proxy-Authorization'] = 'Basic '+base64.b64encode(credentials.encode()).decode('ascii')
                    self.local.connection.set_tunnel(endpoint, self.base.port or 443, headers=headers)
            else:
                self.local.connection = http.client.HTTPConnection(self.base.hostname, self.base.port, timeout=20)
            with self.connection_lock:
                self.connections.add(self.local.connection)
            self.local.route = route
        return self.local.connection

    def snapshot(self):
        with self.connection_lock:
            return {**self.metrics, 'route_mode': self.cdn_route,
                    'http_statuses': dict(self.metrics['http_statuses']),
                    'routes': {key: dict(stats) for key, stats in self.route_stats.items()}}

    def read_exact(self, response, size):
        body = bytearray()
        while len(body) < size:
            self.control.checkpoint()
            started = time.monotonic()
            data = response.read1(min(64*1024, size-len(body)))
            elapsed = time.monotonic()-started
            if not data:
                raise ConnectionError('下载连接提前结束')
            body.extend(data)
            with self.connection_lock:
                self.metrics['received_bytes'] += len(data)
                self.route_stats[self.local.route]['bytes'] += len(data)
                self.route_stats[self.local.route]['read_seconds'] += elapsed
            self.local.request_read_seconds += elapsed
            self.local.request_received += len(data)
            self.transferred(len(data))
        return bytes(body)

    def fetch(self, job, consume=None):
        self.control.checkpoint()
        if not re.fullmatch('[0-9A-F]{16}', job['bundle']):
            raise InstallationError('资源包 ID 不合法')
        begin = job['start']
        path = self.base.path.rstrip('/')+'/'+job['bundle']+'.bundle'
        excluded = set()
        switched = False
        for attempt in range(4):
            self.control.checkpoint()
            response = None
            retry_route = True
            try:
                route = self.choose_route(excluded)
                connection = self.connection(route)
                self.local.request_read_seconds = 0.0
                self.local.request_received = 0
                started = time.monotonic()
                with self.connection_lock:
                    self.metrics['requests'] += 1
                    self.metrics['retries'] += int(attempt > 0)
                connection.request('GET', path, headers={'Range': f"bytes={begin}-{job['end']-1}",
                    'User-Agent': 'ValorantTrainerInstaller/1.0', 'Accept-Encoding': 'identity', 'Host': self.base.netloc})
                response = connection.getresponse()
                with self.connection_lock:
                    self.metrics['header_wait_seconds'] += time.monotonic()-started
                    code = str(response.status)
                    self.metrics['http_statuses'][code] = self.metrics['http_statuses'].get(code, 0)+1
                if response.status in (429, 500, 502, 503, 504):
                    retry = response.getheader('Retry-After', '')
                    delay = int(retry) if retry.isdigit() else 2**attempt
                    with self.connection_lock:
                        self.route_retry_at[route] = time.monotonic()+max(delay, 10)
                    self.disconnect()
                    excluded.add(route)
                    if delay > 60:
                        retry_route = False
                        raise InstallationError(f'下载服务器繁忙，请在 {delay} 秒后继续，已完成的进度会保留')
                    if attempt == 3:
                        raise InstallationError('下载服务器暂时不可用，进度已保存')
                    self.control.delay(max(delay, 1))
                    continue
                if response.status != 206:
                    raise InstallationError(f'资源下载返回 HTTP {response.status}，请稍后使用安装器继续')
                match = re.fullmatch(r'bytes (\d+)-(\d+)/(\d+)', response.getheader('Content-Range', ''))
                if not match or tuple(map(int, match.group(1, 2))) != (begin, job['end']-1):
                    raise InstallationError('服务器的分段响应不符合请求，进度已保存')
                length = response.getheader('Content-Length', '')
                chunked = getattr(response, 'chunked', False) is True
                valid_length = (not length and chunked) or (length.isdigit() and int(length) == job['end']-begin)
                if int(match.group(3)) < job['end'] or not valid_length:
                    raise InstallationError('下载数据长度不符合资源计划')
                if consume:
                    change_route = False
                    for chunk in job['chunks']:
                        if chunk['offset'] < begin:
                            continue
                        data = self.read_exact(response, chunk['csize'])
                        consume(chunk, data)
                        begin = chunk['offset']+chunk['csize']
                        if (not switched and attempt < 3 and begin < job['end']
                                and self.faster_route_available(route)):
                            change_route = True
                            break
                    if change_route:
                        switched = True
                        excluded.add(route)
                        self.disconnect()
                        continue
                    body = None
                else:
                    body = self.read_exact(response, job['end']-begin)
                if chunked:
                    self.disconnect()
                response.close()
                return body
            except Stopped:
                self.disconnect()
                raise
            except InstallationError:
                with self.connection_lock:
                    self.metrics['errors'] += 1
                    self.route_stats[route]['errors'] += 1
                    self.route_retry_at[route] = time.monotonic()+10
                self.disconnect()
                if self.cdn_route == 'auto' and retry_route and attempt < 3:
                    excluded.add(route)
                    continue
                raise
            except (OSError, http.client.HTTPException) as error:
                with self.connection_lock:
                    self.metrics['errors'] += 1
                    self.route_stats[route]['errors'] += 1
                    self.route_retry_at[route] = time.monotonic()+10
                self.disconnect()
                excluded.add(route)
                if attempt == 3:
                    raise InstallationError('网络连接中断，进度已保存，可重新继续') from error
                self.control.delay(0.2 if self.cdn_route == 'auto' else 2**attempt)
            finally:
                if response:
                    response.close()
        raise InstallationError('下载重试次数已用尽')


class FileHandles:
    def __init__(self, limit=24):
        self.handles = OrderedDict()
        self.limit = limit

    def get(self, path):
        if path not in self.handles:
            self.handles[path] = path.open('r+b', buffering=0)
        self.handles.move_to_end(path)
        if len(self.handles) > self.limit:
            _, handle = self.handles.popitem(last=False)
            os.fsync(handle.fileno())
            handle.close()
        return self.handles[path]

    def sync(self, paths):
        for path in paths:
            if path in self.handles:
                os.fsync(self.handles[path].fileno())

    def close(self):
        for handle in self.handles.values():
            os.fsync(handle.fileno())
            handle.close()
        self.handles.clear()


class DownloadCheckpoint:
    """Publish resume markers only after the corresponding bytes are durable."""
    def __init__(self, state, handles):
        self.state = state
        self.handles = handles
        self.updates = []
        self.dirty = set()
        self.bytes = 0
        self.last_flush = time.monotonic()

    def record(self, file_id, ordinal, path, size):
        self.updates.append((file_id, ordinal))
        self.dirty.add(path)
        self.bytes += size

    def flush(self, *, force=False):
        if not self.updates:
            return
        if not force and self.bytes < 64*1024*1024 and time.monotonic()-self.last_flush < 1:
            return
        self.handles.sync(self.dirty)
        self.state.executemany('INSERT OR REPLACE INTO done VALUES(?,?)', self.updates)
        self.state.commit()
        self.updates.clear()
        self.dirty.clear()
        self.bytes = 0
        self.last_flush = time.monotonic()


def grouped_jobs(chunks):
    jobs = []
    current = None
    for item in sorted(chunks, key=lambda row: (row['bundle'], row['offset'])):
        if (current and item['bundle'] == current['bundle'] and item['offset'] == current['end']
                and item['offset']+item['csize']-current['start'] <= 16*1024*1024):
            current['end'] += item['csize']
            current['chunks'].append(item)
        else:
            current = {'bundle': item['bundle'], 'start': item['offset'],
                       'end': item['offset']+item['csize'], 'chunks': [item]}
            jobs.append(current)
    return jobs


class Installer:
    def __init__(self, plan_path, descriptor, target, codec, *, control=None, notify=None,
                 transport_factory=None, prepare=None, workers=32, network_mode='system', cdn_route='auto'):
        self.plan_path = Path(plan_path)
        self.descriptor = descriptor
        self.root = Path(target).absolute()
        self.codec = codec
        self.control = control or Control()
        self.notify = notify or (lambda event: None)
        self.transport_factory = transport_factory
        self.prepare = prepare
        self.workers = max(1, min(workers, 64))
        self.network_mode = network_mode
        self.cdn_route = cdn_route
        self.downloaded = 0
        self.done = 0
        self.total = descriptor.get('written_bytes', descriptor['game_bytes'])
        self.stats_lock = threading.Lock()
        self.last_emit = 0

    def event(self, phase, message, *, force=True):
        with self.stats_lock:
            if not force and time.monotonic()-self.last_emit < .2:
                return
            self.last_emit = time.monotonic()
            event = {'phase': phase, 'message': message, 'completed': self.done,
                     'total': self.total, 'downloaded': self.downloaded, 'timestamp': time.monotonic()}
        self.notify(event)

    def transferred(self, count):
        with self.stats_lock:
            self.downloaded += count
        self.event('download', '正在下载并还原游戏资源', force=False)

    def validate_target(self):
        if self.root.resolve() != self.root:
            raise InstallationError('请选择本地磁盘上的独立安装目录')
        if os.name == 'nt' and not re.fullmatch('[A-Za-z]:', self.root.drive):
            raise InstallationError('请选择带盘符的本地安装目录')
        if self.root.exists() and not self.root.is_dir():
            raise InstallationError('安装位置不是文件夹')
        for parent in (self.root, *self.root.parents):
            if (parent/'ShooterGame/Binaries/Win64/VALORANT-Win64-Shipping.exe').is_file():
                raise InstallationError('请在现有国际服安装目录以外选择独立的训练器目录')
        if self.root.exists():
            entries = {path.name for path in self.root.iterdir()}
            known = self.root/'.local/download-installation.json'
            marker = self.root/'.setup/installer-owner.json'
            package = self.root/'patch-version.json'
            if entries- {'.setup'} and not (known.is_file() or marker.is_file() or package.is_file()):
                raise InstallationError('目标文件夹包含其他文件，请选择新的训练器目录')
            if package.is_file():
                version = json.loads(package.read_text(encoding='utf-8'))
                if version.get('client_image_sha256') != self.descriptor['source_image_sha256']:
                    raise InstallationError('此目录属于其他客户端版本，请选择新的安装目录')
        self.root.mkdir(parents=True, exist_ok=True)
        setup = owned(self.root, '.setup/installer-owner.json')
        if setup.exists():
            marker = json.loads(setup.read_text(encoding='utf-8'))
            if marker.get('manifest_id') != self.descriptor['manifest_id']:
                raise InstallationError('此目录有其他版本的安装进度，请选择新的目录')
        atomic_json(setup, {'manifest_id': self.descriptor['manifest_id'], 'kind': 'trainer-downloader-v1'})

    def lock(self):
        lock_path = owned(self.root, '.setup/install.lock')
        handle = lock_path.open('a+b')
        if lock_path.stat().st_size == 0:
            handle.write(b'\0')
            handle.flush()
        handle.seek(0)
        try:
            if os.name == 'nt':
                import msvcrt
                msvcrt.locking(handle.fileno(), msvcrt.LK_NBLCK, 1)
            else:
                import fcntl
                fcntl.flock(handle.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        except OSError as error:
            handle.close()
            raise InstallationError('此目录已有安装任务正在运行') from error
        return handle

    def write_chunk(self, chunk, raw, consumers, active_files, handles, checkpoint):
        for file_id, ordinal, offset in consumers[chunk['id']]:
            part = active_files[file_id][2]
            handle = handles.get(part)
            handle.seek(offset)
            if handle.write(raw) != len(raw):
                raise InstallationError('写入安装目录失败')
            checkpoint.record(file_id, ordinal, part, len(raw))
            self.done += len(raw)

    def network_diagnostics(self, transport, started):
        try:
            atomic_json(owned(self.root, '.setup/download-diagnostics.json'), {
                **transport.snapshot(), 'workers': self.workers,
                'elapsed_seconds': round(time.monotonic()-started, 3),
                'written_bytes': self.done, 'downloaded_bytes': self.downloaded})
        except OSError:
            pass

    def download(self, chunks, consumers, active_files, state, handles):
        if not chunks:
            return
        # The queue holds verified blocks. Entire HTTP responses never wait in RAM.
        largest = max((chunk['usize'] for chunk in chunks.values()), default=1)
        received = queue.Queue(maxsize=max(1, 16*1024*1024//largest))
        transport = (self.transport_factory(self.control, self.transferred) if self.transport_factory
                     else RangeTransport(self.control, transferred=self.transferred,
                                         network_mode=self.network_mode, cdn_route=self.cdn_route))
        jobs = iter(grouped_jobs(chunks.values()))
        checkpoint = DownloadCheckpoint(state, handles)
        pool = ThreadPoolExecutor(max_workers=self.workers)
        pending = set()
        save_received = False
        started = time.monotonic()
        last_report = started

        def consume(chunk, data):
            self.control.checkpoint()
            raw = self.codec.decode(data, chunk['usize'])
            if hashlib.sha256(raw).hexdigest() != chunk['sha256']:
                raise InstallationError('下载块内容校验失败，进度已保存，可重新继续')
            while True:
                try:
                    received.put_nowait((chunk, raw))
                    return
                except queue.Full:
                    self.control.delay(.01)

        try:
            for _ in range(self.workers):
                job = next(jobs, None)
                if job is not None:
                    pending.add(pool.submit(transport.fetch, job, consume))
            while pending or not received.empty():
                self.control.checkpoint()
                try:
                    chunk, raw = received.get(timeout=.05)
                except queue.Empty:
                    pass
                else:
                    self.write_chunk(chunk, raw, consumers, active_files, handles, checkpoint)
                for task in tuple(pending):
                    if task.done():
                        pending.remove(task)
                        task.result()
                        job = next(jobs, None)
                        if job is not None:
                            pending.add(pool.submit(transport.fetch, job, consume))
                checkpoint.flush()
                self.event('download', '正在下载并还原游戏资源', force=False)
                if time.monotonic()-last_report >= 5:
                    self.network_diagnostics(transport, started)
                    last_report = time.monotonic()
        except BaseException as error:
            self.control.stop()
            save_received = isinstance(error, Stopped)
            raise
        finally:
            pool.shutdown(wait=True, cancel_futures=True)
            transport.close()
            if save_received:
                while not received.empty():
                    chunk, raw = received.get_nowait()
                    self.write_chunk(chunk, raw, consumers, active_files, handles, checkpoint)
            checkpoint.flush(force=True)
            self.network_diagnostics(transport, started)

    def file_valid(self, path, item, plan):
        if not path.is_file() or path.stat().st_size != item['size']:
            return False
        if self.descriptor.get('resource_profile') != 'minimal':
            return digest(path) == item['sha256']
        with path.open('rb') as stream:
            for chunk in plan.chunks(item['id']):
                stream.seek(chunk['file_offset'])
                if hashlib.sha256(stream.read(chunk['usize'])).hexdigest() != chunk['sha256']:
                    return False
        return True

    def run(self):
        self.validate_target()
        lock = self.lock()
        plan = None
        state = None
        handles = FileHandles()
        try:
            plan = Plan(self.plan_path, self.descriptor['plan_sha256'])
            if plan.meta['manifest_id'] != self.descriptor['manifest_id']:
                raise InstallationError('安装器版本与下载计划不一致')
            state_path = owned(self.root, '.setup/download-state.sqlite')
            state = sqlite3.connect(state_path)
            state.execute('PRAGMA journal_mode=WAL')
            state.execute('PRAGMA synchronous=FULL')
            state.executescript('CREATE TABLE IF NOT EXISTS meta(key TEXT PRIMARY KEY,value TEXT);'
                               'CREATE TABLE IF NOT EXISTS done(file_id INTEGER,ordinal INTEGER,PRIMARY KEY(file_id,ordinal));')
            previous = state.execute("SELECT value FROM meta WHERE key='plan'").fetchone()
            if previous and previous[0] != self.descriptor['plan_sha256']:
                state.execute('DELETE FROM done')
            state.execute("INSERT OR REPLACE INTO meta VALUES('plan',?)", (self.descriptor['plan_sha256'],))
            state.commit()
            completed = defaultdict(set)
            for file_id, ordinal in state.execute('SELECT * FROM done'):
                completed[file_id].add(ordinal)
            active_files = {}
            chunks = {}
            consumers = defaultdict(list)
            finished = set()
            needed_space = 0
            for item in plan.files:
                self.control.checkpoint()
                path = owned(self.root, 'client/'+item['path'])
                part = path.with_name(path.name+'.downloading')
                if part.resolve() != part:
                    raise InstallationError('下载临时文件路径指向了其他位置')
                self.event('verify', '检查已有文件 · '+Path(item['path']).name)
                if self.file_valid(path, item, plan):
                    finished.add(item['id'])
                    self.done += sum(c['usize'] for c in plan.chunks(item['id']))
                    continue
                if path.exists() and not part.exists():
                    os.replace(path, part)
                file_chunks = plan.chunks(item['id'])
                valid = set()
                if part.is_file():
                    with part.open('rb') as stream:
                        for chunk in file_chunks:
                            self.control.checkpoint()
                            stream.seek(chunk['file_offset'])
                            raw = stream.read(chunk['usize'])
                            if len(raw) == chunk['usize'] and hashlib.sha256(raw).hexdigest() == chunk['sha256']:
                                valid.add(chunk['ordinal'])
                                self.done += chunk['usize']
                state.executemany('DELETE FROM done WHERE file_id=? AND ordinal=?',
                                  ((item['id'], n) for n in completed[item['id']]-valid))
                state.executemany('INSERT OR IGNORE INTO done VALUES(?,?)',
                                  ((item['id'], n) for n in valid))
                allocated = allocated_bytes(part)
                needed_space += max(0, sum(c['usize'] for c in file_chunks)-allocated)
                active_files[item['id']] = (item, path, part)
                for chunk in file_chunks:
                    if chunk['ordinal'] in valid:
                        continue
                    chunks[chunk['id']] = chunk
                    consumers[chunk['id']].append((item['id'], chunk['ordinal'], chunk['file_offset']))
            state.commit()
            reserve = self.descriptor.get('patch_expanded_bytes', 0)+256*1024*1024
            if shutil.disk_usage(self.root).free < needed_space+reserve:
                raise InstallationError(f'安装目录空间不足，当前还需约 {(needed_space+reserve)/1000000000:.1f} GB 可用空间')
            for item, path, part in active_files.values():
                path.parent.mkdir(parents=True, exist_ok=True)
                with part.open('r+b' if part.exists() else 'w+b') as stream:
                    if self.descriptor.get('resource_profile') == 'minimal' and os.name == 'nt':
                        import msvcrt
                        result = ctypes.windll.kernel32.DeviceIoControl(ctypes.c_void_p(msvcrt.get_osfhandle(stream.fileno())), 0x900C4, None, 0, None, 0, ctypes.byref(ctypes.c_ulong()), None)
                        if not result:
                            raise InstallationError('精简资源需要支持稀疏文件的 NTFS 磁盘')
                    stream.truncate(item['size'])
            self.download(chunks, consumers, active_files, state, handles)
            handles.close()
            receipt_files = {}
            for item in plan.files:
                self.control.checkpoint()
                path = owned(self.root, 'client/'+item['path'])
                if item['id'] not in finished:
                    part = active_files[item['id']][2]
                    self.event('verify', '校验文件 · '+Path(item['path']).name)
                    if not self.file_valid(part, item, plan):
                        with part.open('rb') as stream:
                            for chunk in plan.chunks(item['id']):
                                stream.seek(chunk['file_offset'])
                                raw = stream.read(chunk['usize'])
                                if hashlib.sha256(raw).hexdigest() != chunk['sha256']:
                                    state.execute('DELETE FROM done WHERE file_id=? AND ordinal=?',
                                                  (item['id'], chunk['ordinal']))
                        state.commit()
                        raise InstallationError('文件完整性校验失败，请使用校验修复后继续 · '+Path(item['path']).name)
                    os.replace(part, path)
                stat = path.stat()
                receipt_files[item['path']] = {'bytes': stat.st_size, 'mtime_ns': stat.st_mtime_ns, 'sha256': item['sha256']}
            self.event('prepare', '正在准备补丁运行环境')
            if self.prepare:
                self.prepare(self.root, receipt_files)
            result = {'complete': True, 'manifest_id': plan.meta['manifest_id'],
                      'plan_sha256': self.descriptor['plan_sha256'], 'resource_profile': self.descriptor.get('resource_profile', 'full'), 'files': receipt_files,
                      'installed_utc': datetime.now(timezone.utc).isoformat(), 'downloaded_this_run': self.downloaded}
            atomic_json(owned(self.root, '.local/download-installation.json'), result)
            self.event('complete', '安装完成')
            return result
        finally:
            handles.close()
            if state:
                state.close()
            if plan:
                plan.close()
            lock.close()


def extract_patch(archive, expected_sha256, root):
    if digest(archive) != expected_sha256:
        raise InstallationError('安装器内的补丁校验失败')
    root = Path(root).resolve()
    stage = owned(root, '.setup/patch-stage')
    stage.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(archive) as bundle:
        names = bundle.namelist()
        if not names or len(names) != len({name.casefold() for name in names}):
            raise InstallationError('补丁归档文件清单不合法')
        for info in bundle.infolist():
            if info.is_dir():
                continue
            if (info.external_attr>>16)&0o170000 == 0o120000:
                raise InstallationError('补丁归档包含符号链接')
            target = owned(stage, info.filename)
            target.parent.mkdir(parents=True, exist_ok=True)
            with bundle.open(info) as incoming, target.open('wb') as outgoing:
                shutil.copyfileobj(incoming, outgoing)
    inventory = json.loads((stage/'package-sha256.json').read_text(encoding='utf-8'))
    for name, expected in inventory.items():
        source = owned(stage, name)
        if digest(source) != expected:
            raise InstallationError('补丁归档内容校验失败 · '+name)
    for name in sorted(inventory):
        source = owned(stage, name)
        target = owned(root, name)
        target.parent.mkdir(parents=True, exist_ok=True)
        temporary = target.with_name(target.name+'.'+uuid.uuid4().hex+'.installing')
        try:
            shutil.copyfile(source, temporary)
            os.replace(temporary, target)
        finally:
            temporary.unlink(missing_ok=True)
    shutil.copyfile(stage/'package-sha256.json', root/'package-sha256.json')
