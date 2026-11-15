"""Launch original Duel 1 training; Esc opens settings and F10 ends the owned child."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import subprocess
import sys
from local_environment import project_environment
from native_maps import training_maps

from build_native_module import build
from run_map_probe import prepare
from run_native_probe import ROOT, preflight, save
from resource_bundle import ensure_resources


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seconds", type=int, default=86400)
    parser.add_argument("--no-hud", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--prebuilt", action="store_true", help="Use the verified exported module without a compiler")
    parser.add_argument("--check-only", action="store_true", help="Check the package and client without starting a game")
    parser.add_argument("--controller-settings", type=Path, help="Start a map/mode selected by the local WebUI")
    parser.add_argument("--session-dir", type=Path, help=argparse.SUPPRESS)
    parser.add_argument("--test-input", action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    if not 1 <= args.seconds <= 86400:
        parser.error("Duration must be from 1 second to 24 hours; F10 ends the session earlier")
    session = (args.session_dir.resolve() if args.session_dir else
               ROOT / ".local/training-sessions" / datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ"))
    if ROOT / '.local/training-sessions' not in session.parents:
        raise ValueError('Session directory must belong to this trainer')
    session.mkdir(parents=True)
    config = json.loads((ROOT / "project.json").read_text(encoding="utf-8"))
    ensure_resources(ROOT, config)
    check = preflight(config)
    save(session / "preflight.json", check)
    if not check["ready"]:
        raise RuntimeError("启动检查未通过：请检查本项目使用的客户端副本是否已运行，以及版本和本地配置。")
    if args.prebuilt:
        from native_module import verified_build
        verified_build()
    else:
        build()
    selected_map, selected_mode = 1, 'navigation'
    if args.controller_settings:
        from training_settings_v2 import validate
        args.controller_settings = args.controller_settings.resolve(strict=True)
        if ROOT / '.local' not in args.controller_settings.parents:
            raise ValueError('Controller settings must belong to this workspace')
        selected_config = validate(json.loads(args.controller_settings.read_text(encoding='utf-8')))
        selected_map, selected_mode = selected_config['map_id'], 'training'
        if selected_map not in training_maps():
            raise ValueError('所选地图在当前资源中不存在')
    prepared = prepare(ROOT, selected_map, selected_mode, session / "training-config.bin")
    save(session / "config.json", prepared)
    if args.check_only:
        print(json.dumps({"ready": True, "game_started": False, "session": str(session)}, ensure_ascii=False))
        return 0
    save(ROOT / ".local/latest-training-session.json", {"session": str(session)})
    command = [sys.executable, str(ROOT / "tools/run_native_baseline.py"), "--run",
               "--seconds", str(args.seconds), "--bootstrap", prepared["archive"], "--map", str(selected_map),
               "--mode", selected_mode, "--observe-world", "--training-module", "--module-stage", "9" if args.test_input else "8",
               "--original-game-instance", "--original-visual-classes", "--no-training-hud",
               "--result-file", str(session / "result.json")]
    if not args.test_input:
        command.append('--interactive-training')
    if args.controller_settings:
        command += ['--controller-settings', str(args.controller_settings)]
    with (session / "session.log").open("w", encoding="utf-8") as log:
        result = subprocess.run(command, cwd=ROOT, stdout=log, stderr=log,
                                creationflags=subprocess.CREATE_NO_WINDOW, env=project_environment(ROOT))
    return result.returncode


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        message = str(error)
        if sys.stderr:
            print(message, file=sys.stderr)
        else:
            import ctypes
            ctypes.windll.user32.MessageBoxW(None, message, "斗牛 1 练枪启动失败", 0x10)
        raise SystemExit(1)
