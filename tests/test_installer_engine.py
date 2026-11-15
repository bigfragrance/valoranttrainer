"""Local HTTP fixtures only. External network connections are forbidden."""
import ctypes
import hashlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import io
import json
import os
from pathlib import Path
import socket
import sqlite3
import sys
import tempfile
import threading
import unittest
from unittest.mock import patch
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
import installer_engine as subject
import trainer_downloaded
import trainer_patch


class DownloadFixture:
    def __init__(self, base):
        self.base = base
        self.codec = subject.Zstd('E:/Program Files/Git/mingw64/bin/libzstd.dll')
        library = self.codec.library
        library.ZSTD_compressBound.argtypes = [ctypes.c_size_t]
        library.ZSTD_compressBound.restype = ctypes.c_size_t
        library.ZSTD_compress.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int]
        library.ZSTD_compress.restype = ctypes.c_size_t
        self.bundle = '1234567890ABCDEF'
        self.raw = [hashlib.shake_256(str(index).encode()).digest(20000) for index in range(6)]
        encoded = []
        self.chunks = []
        offset = 0
        for number, raw in enumerate(self.raw):
            capacity = library.ZSTD_compressBound(len(raw))
            output, incoming = ctypes.create_string_buffer(capacity), ctypes.create_string_buffer(raw)
            size = library.ZSTD_compress(output, capacity, incoming, len(raw), 3)
            assert not library.ZSTD_isError(size)
            frame = output.raw[:size]
            self.chunks.append((f'{number:016X}', self.bundle, offset, len(frame), len(raw), hashlib.sha256(raw).hexdigest()))
            encoded.append(frame)
            offset += len(frame)
        self.body = b''.join(encoded)
        self.files = [('ShooterGame/Binaries/Win64/VALORANT-Win64-Shipping.exe', [0, 1, 2]),
                      ('ShooterGame/Content/Paks/fixture.ucas', [2, 3, 4, 5]),
                      ('ShooterGame/Content/Paks/empty.sig', [])]
        self.contents = {name: b''.join(self.raw[index] for index in indexes) for name, indexes in self.files}
        self.plan = base/'plan.sqlite'
        db = sqlite3.connect(self.plan)
        db.executescript('''
            CREATE TABLE meta(key TEXT PRIMARY KEY,value TEXT);
            CREATE TABLE files(id INTEGER PRIMARY KEY,path TEXT,size INTEGER,sha256 TEXT);
            CREATE TABLE chunks(id TEXT PRIMARY KEY,bundle TEXT,offset INTEGER,csize INTEGER,usize INTEGER,sha256 TEXT);
            CREATE TABLE file_chunks(file_id INTEGER,ordinal INTEGER,chunk_id TEXT,offset INTEGER);
        ''')
        db.executemany('INSERT INTO chunks VALUES(?,?,?,?,?,?)', self.chunks)
        for identifier, (name, indexes) in enumerate(self.files, 1):
            value = self.contents[name]
            db.execute('INSERT INTO files VALUES(?,?,?,?)', (identifier, name, len(value), hashlib.sha256(value).hexdigest()))
            position = 0
            for ordinal, index in enumerate(indexes):
                db.execute('INSERT INTO file_chunks VALUES(?,?,?,?)', (identifier, ordinal, f'{index:016X}', position))
                position += len(self.raw[index])
        self.meta = {'schema_version': 1, 'manifest_id': '448DAE34A9D16C88', 'file_count': len(self.files),
                     'game_bytes': sum(len(value) for value in self.contents.values()), 'download_bytes': len(self.body)}
        db.executemany('INSERT INTO meta VALUES(?,?)', ((key, json.dumps(value)) for key, value in self.meta.items()))
        db.commit()
        db.close()
        self.descriptor = {**self.meta, 'plan_sha256': subject.digest(self.plan),
                           'source_image_sha256': hashlib.sha256(self.contents[self.files[0][0]]).hexdigest()}
        self.requests = []
        self.mode = 'normal'
        self.first_block_sent = threading.Event()
        self.release_body = threading.Event()
        fixture = self

        class Handler(BaseHTTPRequestHandler):
            protocol_version = 'HTTP/1.1'

            def handle(self):
                try:
                    super().handle()
                except ConnectionError:
                    pass

            def do_GET(self):
                fixture.requests.append((self.path, self.headers.get('Range')))
                if fixture.mode == 'missing':
                    self.send_response(404)
                    self.send_header('Content-Length', '0')
                    self.end_headers()
                    return
                start, end = map(int, self.headers['Range'][6:].split('-'))
                body = fixture.body[start:end+1]
                if fixture.mode == 'corrupt':
                    body = bytearray(body)
                    body[len(body)//2] ^= 1
                    body = bytes(body)
                self.send_response(200 if fixture.mode == 'ignore_range' else 206)
                first = start+1 if fixture.mode == 'wrong_range' else start
                self.send_header('Content-Range', f'bytes {first}-{end}/{len(fixture.body)}')
                if fixture.mode == 'chunked':
                    self.send_header('Transfer-Encoding', 'chunked')
                else:
                    self.send_header('Content-Length', str(len(body)))
                self.end_headers()
                try:
                    if fixture.mode == 'chunked':
                        self.wfile.write(f'{len(body):X}\r\n'.encode()+body+b'\r\n0\r\n\r\n')
                        return
                    first_size = fixture.chunks[0][3]
                    if fixture.mode == 'truncated_once' and len(fixture.requests) == 1:
                        self.wfile.write(body[:first_size+10])
                        self.wfile.flush()
                        self.close_connection = True
                        return
                    if fixture.mode == 'held_tail':
                        self.wfile.write(body[:first_size])
                        self.wfile.flush()
                        fixture.first_block_sent.set()
                        fixture.release_body.wait(timeout=5)
                        body = body[first_size:]
                    self.wfile.write(body)
                except (ConnectionError, OSError):
                    pass

            def log_message(self, *_args):
                pass
        self.server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()

    def close(self):
        self.release_body.set()
        self.server.shutdown()
        self.server.server_close()
        self.thread.join(timeout=5)

    def transport(self, control, transferred):
        return subject.RangeTransport(control,
            base_url=f'http://127.0.0.1:{self.server.server_port}/bundles/', allow_loopback=True, transferred=transferred)

    def installer(self, target, **kwargs):
        return subject.Installer(self.plan, self.descriptor, target, self.codec,
                                 transport_factory=self.transport, workers=2, **kwargs)


class InstallerTests(unittest.TestCase):
    def setUp(self):
        directory = ROOT/'work/installer-tests'
        directory.mkdir(exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=directory)
        self.base = Path(self.temp.name).resolve()
        self.assertIn(ROOT, self.base.parents)
        self.fixture = DownloadFixture(self.base)
        self.target = self.base/'中文 安装位置'
        original = socket.create_connection
        def local_only(address, *args, **kwargs):
            if address[0] not in ('127.0.0.1', '::1', 'localhost'):
                raise AssertionError('External downloads forbidden in installer tests')
            return original(address, *args, **kwargs)
        self.guard = patch.object(socket, 'create_connection', local_only)
        self.guard.start()

    def tearDown(self):
        self.guard.stop()
        self.fixture.close()
        self.assertIn(ROOT, self.base.resolve().parents)
        self.temp.cleanup()

    def test_full_install_decompresses_shared_chunks_and_prepares_once(self):
        prepared = []
        result = self.fixture.installer(self.target, prepare=lambda root, files: prepared.append((root, files))).run()
        self.assertTrue(result['complete'])
        self.assertEqual(len(prepared), 1)
        for name, value in self.fixture.contents.items():
            self.assertEqual((self.target/'client'/name).read_bytes(), value)
        self.assertLessEqual(len(self.fixture.requests), 2)
        self.assertTrue((self.target/'.local/download-installation.json').is_file())

    def test_completed_install_repair_uses_no_network(self):
        self.fixture.installer(self.target).run()
        self.fixture.requests.clear()
        self.fixture.mode = 'missing'
        result = self.fixture.installer(self.target).run()
        self.assertEqual(result['downloaded_this_run'], 0)
        self.assertFalse(self.fixture.requests)

        from unittest.mock import Mock
        unused_transport = Mock(side_effect=AssertionError('A complete installation needs no network setup'))
        subject.Installer(self.fixture.plan, self.fixture.descriptor, self.target,
                          self.fixture.codec, transport_factory=unused_transport).run()
        unused_transport.assert_not_called()

    def test_streaming_saves_first_block_before_response_tail_arrives(self):
        self.fixture.mode = 'held_tail'
        control = subject.Control()
        written = threading.Event()
        errors = []
        def notify(event):
            if event['phase'] == 'download' and event['completed'] > 0:
                written.set()
        def run():
            try:
                self.fixture.installer(self.target, control=control, notify=notify).run()
            except subject.Stopped as error:
                errors.append(error)
        worker = threading.Thread(target=run)
        worker.start()
        try:
            self.assertTrue(self.fixture.first_block_sent.wait(timeout=2))
            self.assertTrue(written.wait(timeout=2), 'First block must be written before the rest is released')
        finally:
            control.stop()
            self.fixture.release_body.set()
            worker.join(timeout=10)
        self.assertFalse(worker.is_alive())
        self.assertEqual(len(errors), 1)
        result = self.fixture.installer(self.target).run()
        self.assertTrue(result['complete'])

    def test_streaming_retry_starts_after_last_complete_verified_block(self):
        self.fixture.mode = 'truncated_once'
        result = self.fixture.installer(self.target).run()
        expected = self.fixture.chunks[0][3]
        self.assertEqual(self.fixture.requests[1][1], f'bytes={expected}-{len(self.fixture.body)-1}')
        self.assertTrue(result['complete'])

    def test_valid_chunked_ranges_are_verified_without_content_length(self):
        self.fixture.mode = 'chunked'
        result = self.fixture.installer(self.target).run()
        self.assertTrue(result['complete'])
        for name, data in self.fixture.contents.items():
            self.assertEqual((self.target/'client'/name).read_bytes(), data)

    def test_tls_keeps_original_origin_after_proxy_tunnel(self):
        from unittest.mock import Mock
        context = Mock()
        connection = subject.CdnHTTPSConnection('127.0.0.1', 8080,
            origin='valorant.dyn.riotcdn.net', context=context)
        connection.set_tunnel('fallback2.riotcdn.net', 443)
        original_socket = Mock()
        connection.sock = original_socket
        with patch.object(subject.http.client.HTTPConnection, 'connect'):
            connection.connect()
        context.wrap_socket.assert_called_once_with(original_socket, server_hostname='valorant.dyn.riotcdn.net')

    def test_system_proxy_credentials_are_limited_to_connect_headers(self):
        with patch.object(subject, 'getproxies', return_value={'https': 'http://sample:private@127.0.0.1:8080'}), \
                patch.object(subject, 'proxy_bypass', return_value=False):
            transport = subject.RangeTransport(subject.Control(), network_mode='system', cdn_route='cloudflare')
        try:
            connection = transport.connection('cloudflare')
            self.assertEqual(connection.host, '127.0.0.1')
            self.assertEqual(connection._tunnel_host, 'fallback2.riotcdn.net')
            self.assertEqual(connection.origin, 'valorant.dyn.riotcdn.net')
            self.assertIn('Proxy-Authorization', connection._tunnel_headers)
            self.assertNotIn('private', json.dumps(transport.snapshot()))
        finally:
            transport.close()

    def test_automatic_route_recovers_from_unavailable_primary(self):
        from unittest.mock import Mock
        transport = subject.RangeTransport(subject.Control(), cdn_route='auto')
        job = subject.grouped_jobs([dict(zip(('id','bundle','offset','csize','usize','sha256'), row))
                                    for row in self.fixture.chunks])[0]
        selected, delivered = [], []
        def connection(route):
            selected.append(route)
            transport.local.route = route
            response = Mock()
            response.status = 404 if route == 'primary' else 206
            headers = {'Content-Range': f'bytes 0-{len(self.fixture.body)-1}/{len(self.fixture.body)}',
                       'Content-Length': str(len(self.fixture.body))}
            response.getheader.side_effect = lambda name, default='': headers.get(name, default)
            response.read1.side_effect = io.BytesIO(self.fixture.body).read
            result = Mock()
            result.getresponse.return_value = response
            return result
        with patch.object(transport, 'connection', side_effect=connection):
            transport.fetch(job, lambda chunk, data: delivered.append(chunk['id']))
        self.assertEqual(selected, ['primary', 'cloudflare'])
        self.assertEqual(delivered, [row[0] for row in self.fixture.chunks])

    def test_automatic_route_prefers_measured_faster_provider(self):
        transport = subject.RangeTransport(subject.Control(), cdn_route='auto')
        for stats in transport.route_stats.values():
            stats['assignments'] = 1
        transport.route_stats['primary'].update(bytes=256*1024, read_seconds=4)
        transport.route_stats['cloudflare'].update(bytes=256*1024, read_seconds=.1)
        transport.local.request_read_seconds = 2
        transport.local.request_received = 32*1024
        self.assertTrue(transport.faster_route_available('primary'))
        self.assertEqual(transport.choose_route(), 'cloudflare')

    def test_range_groups_preserve_gaps_and_bundle_boundaries(self):
        chunks = [dict(id=str(n), bundle='A', offset=n*4*1024*1024, csize=4*1024*1024)
                  for n in range(5)]
        chunks.extend([dict(id='gap', bundle='A', offset=22*1024*1024, csize=1024),
                       dict(id='other', bundle='B', offset=0, csize=1024)])
        jobs = subject.grouped_jobs(chunks)
        self.assertEqual([len(job['chunks']) for job in jobs], [4, 1, 1, 1])
        self.assertEqual(jobs[0]['end']-jobs[0]['start'], 16*1024*1024)

    def test_batched_resume_markers_wait_for_successful_disk_sync(self):
        from unittest.mock import Mock
        state, handles = Mock(), Mock()
        submitted = []
        state.executemany.side_effect = lambda query, rows: submitted.append(list(rows))
        checkpoint = subject.DownloadCheckpoint(state, handles)
        checkpoint.record(1, 0, self.target/'part', 100)
        checkpoint.flush()
        state.commit.assert_not_called()
        handles.sync.side_effect = OSError('disk sync failed')
        with self.assertRaises(OSError):
            checkpoint.flush(force=True)
        state.executemany.assert_not_called()
        handles.sync.side_effect = None
        checkpoint.flush(force=True)
        self.assertEqual(submitted, [[(1, 0)]])
        state.executemany.assert_called_once()
        state.commit.assert_called_once()
        self.assertFalse(checkpoint.updates)

    def test_corrupt_final_file_repairs_only_its_bad_chunk(self):
        self.fixture.installer(self.target).run()
        path = self.target/'client'/self.fixture.files[1][0]
        with path.open('r+b') as stream:
            stream.seek(21000)
            stream.write(b'broken')
        self.fixture.requests.clear()
        result = self.fixture.installer(self.target).run()
        self.assertLess(result['downloaded_this_run'], len(self.fixture.body))
        self.assertEqual(path.read_bytes(), self.fixture.contents[self.fixture.files[1][0]])

    def test_pause_and_stop_resume_saved_completed_job(self):
        control = subject.Control()
        def stop_after_download(event):
            if event['phase'] == 'verify' and event['completed']:
                control.stop()
        with self.assertRaises(subject.Stopped):
            self.fixture.installer(self.target, control=control, notify=stop_after_download).run()
        result = self.fixture.installer(self.target).run()
        self.assertTrue(result['complete'])

    def test_stop_inside_request_saves_verified_chunks_before_resuming(self):
        control = subject.Control()
        original = self.fixture.codec.decode
        decoded = 0
        def stop_after_two_chunks(data, size):
            nonlocal decoded
            result = original(data, size)
            decoded += 1
            if decoded == 2:
                control.stop()
            return result
        self.fixture.codec.decode = stop_after_two_chunks
        with self.assertRaises(subject.Stopped):
            self.fixture.installer(self.target, control=control).run()
        state = sqlite3.connect(self.target/'.setup/download-state.sqlite')
        try:
            self.assertEqual(state.execute('SELECT count(*) FROM done').fetchone()[0], 2)
        finally:
            state.close()
        self.fixture.codec.decode = original
        self.fixture.requests.clear()
        result = self.fixture.installer(self.target).run()
        self.assertLess(result['downloaded_this_run'], len(self.fixture.body))
        self.assertTrue(result['complete'])

    def test_paused_install_waits_before_sending_requests(self):
        control = subject.Control()
        control.pause()
        result = []
        worker = threading.Thread(target=lambda: result.append(self.fixture.installer(self.target, control=control).run()))
        worker.start()
        self.assertFalse(threading.Event().wait(.05))
        self.assertFalse(self.fixture.requests)
        control.resume()
        worker.join(timeout=10)
        self.assertFalse(worker.is_alive())
        self.assertTrue(result[0]['complete'])

    def test_corrupt_resume_bytes_are_checked_again(self):
        self.fixture.installer(self.target).run()
        name = self.fixture.files[0][0]
        path = self.target/'client'/name
        part = path.with_name(path.name+'.downloading')
        os.replace(path, part)
        with part.open('r+b') as stream:
            stream.seek(100)
            stream.write(b'wrong')
        self.fixture.requests.clear()
        self.fixture.installer(self.target).run()
        self.assertEqual(path.read_bytes(), self.fixture.contents[name])
        self.assertTrue(self.fixture.requests)

    def test_missing_bundle_preserves_progress_and_has_no_complete_receipt(self):
        self.fixture.mode = 'missing'
        with self.assertRaisesRegex(subject.InstallationError, '404'):
            self.fixture.installer(self.target).run()
        self.assertFalse((self.target/'.local/download-installation.json').exists())
        self.assertTrue((self.target/'.setup/download-state.sqlite').is_file())

    def test_ignored_and_mismatched_ranges_are_rejected(self):
        for mode in ('ignore_range', 'wrong_range'):
            with self.subTest(mode=mode):
                self.fixture.mode = mode
                with self.assertRaises(subject.InstallationError):
                    self.fixture.installer(self.base/mode).run()

    def test_payload_corruption_is_rejected_before_completion(self):
        self.fixture.mode = 'corrupt'
        with self.assertRaises(subject.InstallationError):
            self.fixture.installer(self.target).run()
        self.assertFalse((self.target/'.local/download-installation.json').exists())

    def test_tampered_plan_is_rejected_without_http(self):
        self.fixture.descriptor['plan_sha256'] = '0'*64
        with self.assertRaisesRegex(subject.InstallationError, '计划校验'):
            self.fixture.installer(self.target).run()
        self.assertFalse(self.fixture.requests)

    def test_foreign_folder_and_existing_game_are_preserved(self):
        self.target.mkdir()
        important = self.target/'my.txt'
        important.write_text('keep', encoding='utf-8')
        with self.assertRaises(subject.InstallationError):
            self.fixture.installer(self.target).run()
        self.assertEqual(important.read_text(encoding='utf-8'), 'keep')
        game = self.base/'Game'
        image = game/'ShooterGame/Binaries/Win64/VALORANT-Win64-Shipping.exe'
        image.parent.mkdir(parents=True)
        image.write_bytes(b'original')
        with self.assertRaises(subject.InstallationError):
            self.fixture.installer(game/'NewTrainer').run()
        self.assertEqual(image.read_bytes(), b'original')

    def test_traversal_and_windows_device_names_rejected(self):
        for path in ('../escape', '/absolute', 'C:/outside', 'x\\file', 'CON.txt', 'dir/NUL', 'dir./file', 'file:stream'):
            with self.subTest(path=path), self.assertRaises(subject.InstallationError):
                subject.relative_name(path)

    def test_same_install_directory_is_locked(self):
        first = self.fixture.installer(self.target)
        first.validate_target()
        lock = first.lock()
        try:
            with self.assertRaisesRegex(subject.InstallationError, '正在运行'):
                self.fixture.installer(self.target).run()
        finally:
            lock.close()

    def test_prepare_failure_can_resume_without_redownload(self):
        def fail(*_args):
            raise subject.InstallationError('prepare failure')
        with self.assertRaisesRegex(subject.InstallationError, 'prepare failure'):
            self.fixture.installer(self.target, prepare=fail).run()
        self.fixture.requests.clear()
        result = self.fixture.installer(self.target).run()
        self.assertEqual(result['downloaded_this_run'], 0)

    def test_patch_archive_and_inventory_are_verified(self):
        bundle = self.base/'patch.zip'
        content = b'patched tool'
        inventory = {'tools/test.py': hashlib.sha256(content).hexdigest()}
        with zipfile.ZipFile(bundle, 'w') as archive:
            archive.writestr('tools/test.py', content)
            archive.writestr('package-sha256.json', json.dumps(inventory))
        self.target.mkdir()
        subject.extract_patch(bundle, subject.digest(bundle), self.target)
        self.assertEqual((self.target/'tools/test.py').read_bytes(), content)
        with self.assertRaises(subject.InstallationError):
            subject.extract_patch(bundle, '0'*64, self.target)

    def test_environment_preserves_existing_settings_and_rebases_after_move(self):
        result = self.fixture.installer(self.target).run()
        profiles = self.target/'profiles'
        profiles.mkdir()
        rows = [{'path': name, 'size': len(value), 'sha256': hashlib.sha256(value).hexdigest()}
                for name, value in self.fixture.contents.items()]
        subject.atomic_json(profiles/'downloaded-files.json', {'schema_version': 1, 'manifest_id': self.fixture.meta['manifest_id'], 'files': rows})
        subject.atomic_json(profiles/'patch-project.json', {'distribution_mode': 'portable-v1'})
        subject.atomic_json(profiles/'patch-default-settings.json', {'revision': 1})
        trainer_downloaded.finalize(self.target, result['files'])
        settings = self.target/trainer_downloaded.SETTINGS
        subject.atomic_json(settings, {'revision': 7})
        moved = self.base/'换位置'
        self.target.rename(moved)
        trainer_downloaded.prepare(moved)
        self.assertEqual(json.loads((moved/trainer_downloaded.SETTINGS).read_text())['revision'], 7)
        project = json.loads((moved/'project.json').read_text())
        self.assertEqual(Path(project['source_resource_directory']), moved/'client/ShooterGame/Content/Paks')


if __name__ == '__main__':
    unittest.main()
