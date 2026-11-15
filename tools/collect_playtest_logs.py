"""Collect the latest exported trainer session, without installation/account files."""
from datetime import datetime
import json
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    output = ROOT / 'logs'
    output.mkdir(exist_ok=True)
    archive = output / ('playtest-feedback-' + datetime.now().strftime('%Y%m%d-%H%M%S') + '.zip')
    candidates = [ROOT / 'playtest-version.json']
    sessions = ROOT / '.local/training-sessions'
    if sessions.exists():
        latest = max((p for p in sessions.iterdir() if p.is_dir()),
                     key=lambda p: p.stat().st_mtime, default=None)
        if latest:
            candidates.extend(latest.rglob('*'))
            result = latest / 'result.json'
            if result.is_file():
                document = json.loads(result.read_text(encoding='utf-8'))
                run = Path(document.get('run_dir') or Path(document.get('report_path', '')).parent).resolve()
                boundary = (ROOT / '.local/native-baseline').resolve()
                if boundary in run.parents and run.is_dir():
                    candidates.extend(run.rglob('*'))
    native = ROOT / '.local/native-module'
    candidates.extend(native.glob('*.json'))
    written = set()
    with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED) as bundle:
        for item in candidates:
            if item.is_file() and item.suffix.lower() in ('.json', '.jsonl', '.log', '.txt'):
                relative = item.relative_to(ROOT).as_posix()
                if relative not in written:
                    bundle.write(item, relative)
                    written.add(relative)
    print('日志包：' + str(archive))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
