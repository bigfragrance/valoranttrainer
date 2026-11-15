"""Request shutdown from this directory's owned trainer runner."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    record = json.loads((ROOT / '.local/latest-training-session.json').read_text(encoding='utf-8'))
    session = Path(record['session']).resolve()
    if (ROOT / '.local/training-sessions').resolve() not in session.parents:
        raise RuntimeError('Session is outside this trainer directory')
    active = json.loads((session / 'result.active.json').read_text(encoding='utf-8'))
    run = Path(active['run_dir']).resolve()
    if (ROOT / '.local/native-baseline').resolve() not in run.parents:
        raise RuntimeError('Runner is outside this trainer directory')
    (run / 'stop-request.json').write_text(json.dumps({'reason': 'user_requested_trainer_shutdown'}), encoding='utf-8')
    print('已请求退出此目录启动的训练器。')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
