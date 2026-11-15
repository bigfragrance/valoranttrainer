"""Build a pinned downloader plan from existing local original game files.

This command performs no network requests. It checks the known patch hashes,
all source IoStore block hashes and a SHA-256 for every Riot download chunk.
"""
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
from pathlib import Path
import sqlite3
import sys

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT/'.local/installer-build'
DONOR = ROOT/'exports/ValorantTrainer-Patch-20261109-r2'
SOURCE = Path('F:/Riot Games/VALORANT/live')
MANIFEST = ROOT/'work/client-analysis/continue-patch/official-target.manifest'


def build():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    destination = OUTPUT/'resource-plan.sqlite'
    if destination.exists():
        raise RuntimeError('Plan already exists; preserve it or choose a new build directory')
    sys.path.insert(0, str(ROOT/'work/riot-client-downloads-20261113'))
    sys.path.insert(0, str(ROOT/'work/extract'))
    from parse_manifest import parse
    from iostore import IoStore
    required = json.loads((DONOR/'profiles/patch-installation-manifest.json').read_text(encoding='utf-8'))
    expected = {row['file']: row for row in required['loose_files']}
    expected.update({'ShooterGame/Content/Paks/'+row['file']: row for row in required['resources']})
    manifest = parse(MANIFEST, complete_chunk_paths=set(expected))
    assert manifest['manifest_id'] == '448DAE34A9D16C88'
    assert manifest['manifest_sha256'] == '68b8ec14ec4f8c6944be2072ea9935cae4929ee021dc52939716476bbfa576d8'
    files = {row['path']: row for row in manifest['files']}
    assert all(path in files and files[path]['bytes'] == row['bytes'] for path, row in expected.items())
    db = sqlite3.connect(destination)
    db.executescript('''
        CREATE TABLE meta(key TEXT PRIMARY KEY,value TEXT NOT NULL);
        CREATE TABLE files(id INTEGER PRIMARY KEY,path TEXT COLLATE NOCASE UNIQUE NOT NULL,
                           size INTEGER NOT NULL,sha256 TEXT NOT NULL);
        CREATE TABLE chunks(id TEXT PRIMARY KEY,bundle TEXT NOT NULL,offset INTEGER NOT NULL,
                            csize INTEGER NOT NULL,usize INTEGER NOT NULL,sha256 TEXT NOT NULL);
        CREATE TABLE file_chunks(file_id INTEGER NOT NULL,ordinal INTEGER NOT NULL,
                                 chunk_id TEXT NOT NULL,offset INTEGER NOT NULL,
                                 PRIMARY KEY(file_id,ordinal));
        CREATE INDEX chunk_bundle ON chunks(bundle,offset);
        CREATE INDEX consumers ON file_chunks(chunk_id);
    ''')
    seen = {}
    total_bytes = 0
    for identifier, (name, original) in enumerate(sorted(expected.items()), 1):
        source = SOURCE/name
        if not source.is_file() or source.stat().st_size != original['bytes']:
            raise RuntimeError('Source runtime changed: '+name)
        digest = hashlib.sha256()
        chunk_rows, positions = [], []
        with source.open('rb', buffering=8*1024*1024) as stream:
            for chunk in files[name]['chunk_plan']:
                data = stream.read(chunk['expanded_size'])
                if len(data) != chunk['expanded_size']:
                    raise RuntimeError('Source file shortened while reading: '+name)
                digest.update(data)
                checksum = hashlib.sha256(data).hexdigest()
                record = (chunk['id'], chunk['bundle_id'], chunk['compressed_offset'],
                          chunk['compressed_size'], chunk['expanded_size'], checksum)
                previous = seen.get(chunk['id'])
                if previous is not None and previous != record:
                    raise RuntimeError('Repeated original chunk has inconsistent bytes: '+chunk['id'])
                if previous is None:
                    seen[chunk['id']] = record
                    chunk_rows.append(record)
                positions.append((identifier, chunk['index'], chunk['id'], chunk['file_offset']))
            if stream.read(1):
                raise RuntimeError('Source file grew while reading: '+name)
        checksum = digest.hexdigest()
        if original.get('sha256') and original['sha256'] != checksum:
            raise RuntimeError('Source does not match the delivered patch: '+name)
        if source.stat().st_size != original['bytes']:
            raise RuntimeError('Source file changed: '+name)
        db.execute('INSERT INTO files VALUES(?,?,?,?)', (identifier, name, original['bytes'], checksum))
        db.executemany('INSERT INTO chunks VALUES(?,?,?,?,?,?)', chunk_rows)
        db.executemany('INSERT INTO file_chunks VALUES(?,?,?,?)', positions)
        total_bytes += original['bytes']
        db.commit()
        if original['bytes'] > 100000000 or identifier%40 == 0:
            print(json.dumps({'hashed_file': name, 'completed_files': identifier,
                              'bytes_checked': total_bytes}), flush=True)

    def verify_container(row):
        path = SOURCE/'ShooterGame/Content/Paks'/row['file']
        io = IoStore(path.with_suffix('.utoc'))
        if not io.block_hashes or io.partition_count != 1:
            raise RuntimeError('Unexpected original signed container layout')
        checked = 0
        with path.open('rb', buffering=8*1024*1024) as stream:
            for index, (offset, compressed, _, _) in enumerate(io.blocks):
                stream.seek(offset)
                length = (compressed+15)//16*16
                data = stream.read(length)
                if len(data) != length or hashlib.sha1(data).digest() != io.block_hashes[index]:
                    raise RuntimeError(f'Original block mismatch {path.name} #{index}')
                checked += 1
        return {'container': path.name, 'signed_original_blocks': checked}

    container_checks = []
    with ThreadPoolExecutor(max_workers=3) as pool:
        pending = [pool.submit(verify_container, row) for row in required['resources'] if row['file'].endswith('.ucas')]
        for task in as_completed(pending):
            check = task.result()
            container_checks.append(check)
            print(json.dumps(check), flush=True)
    compressed = db.execute('SELECT sum(csize) FROM chunks').fetchone()[0]
    info = {'schema_version': 1, 'manifest_id': manifest['manifest_id'],
            'manifest_sha256': manifest['manifest_sha256'], 'profile': 'full-en-audio-zh-tw-text',
            'file_count': len(expected), 'chunk_count': len(seen), 'game_bytes': total_bytes,
            'download_bytes': compressed, 'source_image_sha256': required['image']['sha256'],
            'original_signed_blocks_verified': sum(row['signed_original_blocks'] for row in container_checks),
            'all_maps_heroes_skins': True, 'languages': {'audio': 'en_US', 'text': 'zh_TW'}}
    db.executemany('INSERT INTO meta VALUES(?,?)', ((key, json.dumps(value)) for key, value in info.items()))
    db.commit()
    db.execute('VACUUM')
    db.close()
    with destination.open('rb') as stream:
        info['plan_sha256'] = hashlib.file_digest(stream, 'sha256').hexdigest()
    info['plan_bytes'] = destination.stat().st_size
    (OUTPUT/'resource-plan.json').write_text(json.dumps(info, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(info, ensure_ascii=False), flush=True)


if __name__ == '__main__':
    build()
