"""Build a conservative CDN-chunk superset of the existing Duel1/Lotus dependency closure."""
import bisect
import json
from pathlib import Path
import shutil
import sqlite3
from installer_engine import atomic_json, digest

ROOT = Path(__file__).resolve().parents[1]

def build(full, dependencies, output):
    source = json.loads(Path(dependencies).read_text(encoding='utf-8'))
    selected = {row['file']: row for row in source['containers']}
    shutil.copyfile(full, output)
    db = sqlite3.connect(output)
    db.row_factory = sqlite3.Row
    rows = [dict(row) for row in db.execute('SELECT * FROM files ORDER BY id')]
    by_name = {Path(row['path']).name: row for row in rows}
    for row in source.get('passthrough', []):
        current = by_name.get(row['file'])
        if not current or current['size'] != row['bytes'] or current['sha256'] != row['sha256']:
            raise ValueError('Dependency index version mismatch: '+row['file'])
    manifest = []
    for row in rows:
        chunks = [dict(c) for c in db.execute('SELECT fc.ordinal,fc.offset,c.usize,c.sha256 FROM file_chunks fc JOIN chunks c ON c.id=fc.chunk_id WHERE file_id=? ORDER BY fc.ordinal', (row['id'],))]
        dependency = selected.get(Path(row['path']).name)
        if dependency:
            if dependency['logical_bytes'] != row['size']:
                raise ValueError('Dependency version mismatch: '+row['path'])
            ranges = dependency['ranges']
            ends = [offset+length for offset,length,_ in ranges]
            keep = []
            for chunk in chunks:
                i = bisect.bisect_right(ends, chunk['offset'])
                if i < len(ranges) and ranges[i][0] < chunk['offset']+chunk['usize']:
                    keep.append(chunk)
                else:
                    db.execute('DELETE FROM file_chunks WHERE file_id=? AND ordinal=?', (row['id'],chunk['ordinal']))
            row['ranges'] = [[c['offset'],c['usize'],c['sha256']] for c in keep]
        manifest.append(row)
    db.execute('DELETE FROM chunks WHERE id NOT IN (SELECT chunk_id FROM file_chunks)')
    totals = dict(db.execute('SELECT key,value FROM meta').fetchall())
    download = db.execute('SELECT SUM(csize) FROM chunks').fetchone()[0]
    written = db.execute('SELECT SUM(c.usize) FROM file_chunks fc JOIN chunks c ON c.id=fc.chunk_id').fetchone()[0]
    db.commit()
    db.execute('VACUUM')
    db.close()
    return {'plan_sha256':digest(output),'download_bytes':download,'written_bytes':written,
            'game_bytes':json.loads(totals['game_bytes']),'file_count':len(rows),'files':manifest}

if __name__ == '__main__':
    folder = ROOT/'.local/installer-build'
    result = build(folder/'resource-plan.sqlite', ROOT/'exports/ValorantTrainer-Standalone-Duel1-Lotus-20261108/profiles/standalone-dependencies.json', folder/'resource-plan-minimal.sqlite')
    atomic_json(folder/'resource-plan-minimal.json',result)
    print(json.dumps({k:v for k,v in result.items() if k!='files'}))
