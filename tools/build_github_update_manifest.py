"""Generate the metadata asset accompanying a complete-resource update ZIP."""
import argparse
import json
from pathlib import Path
import zipfile
from installer_engine import atomic_json, digest
from apply_trainer_update import KIND


def build(archive, output):
    archive = Path(archive)
    with zipfile.ZipFile(archive) as bundle:
        version = json.loads(bundle.read('patch-version.json'))
    if version.get('kind') != KIND:
        raise ValueError('A complete-resource update package is required')
    record = {'schema_version': 1, 'version': version['release'],
              'manifest_id': version['manifest_id'], 'asset': archive.name,
              'bytes': archive.stat().st_size, 'sha256': digest(archive)}
    atomic_json(output, record)
    return record


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive', type=Path)
    parser.add_argument('--output', type=Path, default=Path('exports/trainer-update.json'))
    args = parser.parse_args()
    print(json.dumps(build(args.archive, args.output)))
