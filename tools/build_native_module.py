"""Build the local native training module and bind its source identity."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess
from native_module import build_inputs
from local_environment import project_environment

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = ROOT / '.local/native-module'


def build(output_directory=None):
    destination = Path(output_directory).resolve() if output_directory is not None else DEFAULT_OUTPUT
    local_root = (ROOT / '.local').resolve()
    if local_root not in destination.parents or any(c in str(destination) for c in '&|<>^%!"\r\n'):
        raise ValueError('Native build output must be a safe directory under this project .local')
    destination.mkdir(parents=True, exist_ok=True)
    sources = tuple(p for p in build_inputs() if p.startswith('native/'))
    before = {p: (ROOT / p).read_bytes() for p in sources}
    result = subprocess.run([os.environ.get("COMSPEC", "cmd.exe"), "/d", "/c", str(ROOT / "native/build.cmd"), str(destination)],
                            cwd=ROOT, capture_output=True, text=True,
                            creationflags=subprocess.CREATE_NO_WINDOW, timeout=60, env=project_environment(ROOT))
    (destination / 'build.log').write_text(result.stdout + result.stderr, encoding='utf-8')
    if result.returncode:
        raise RuntimeError(result.stdout[:5000] + result.stderr[:1000])
    if set(sources) != {p for p in build_inputs() if p.startswith('native/')} or any(
            (ROOT / p).read_bytes() != payload for p, payload in before.items()):
        raise RuntimeError('Native source changed during compilation; rebuild the updated sources')
    snapshot = destination / 'source-snapshot'
    for p, payload in before.items():
        target = snapshot / p
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(payload)
    paths = (*sources, (destination / 'trainer_bridge.dll').relative_to(ROOT).as_posix())
    record = {"sha256": {p: hashlib.sha256(before[p] if p in before else (ROOT / p).read_bytes()).hexdigest() for p in paths}}
    record['source_snapshot'] = snapshot.relative_to(ROOT).as_posix()
    record['candidate'] = destination != DEFAULT_OUTPUT
    (destination / 'build.json').write_text(json.dumps(record, indent=2), encoding='utf-8')
    print(json.dumps(record))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, help='Compile into an isolated directory under .local')
    args = parser.parse_args()
    build(args.output_dir)
