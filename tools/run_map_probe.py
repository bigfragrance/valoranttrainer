"""Prepare and optionally run repeatable native map diagnostics, without account login."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys

from ini_bootstrap import apply_profile, decode, encode
from native_maps import MAPS, map_package
from run_native_probe import ROOT, save
from client_layout import is_portable
from portable_config import resolve_core


def prepare(root: Path, number: int, mode: str, output: Path) -> dict:
    """Bind each archive to the validated source snapshot and fixed game version."""
    if mode not in ("original", "scene", "navigation", "training"):
        raise ValueError("Invalid map diagnostic selection")
    package = map_package(number, training=mode == 'training')
    if mode == "navigation" and number != 1:
        raise ValueError("Navigation development is restricted to Duel 1")
    config = json.loads((root / "project.json").read_text(encoding="utf-8"))
    proof = json.loads((root / "reports/native-config-validation.json").read_text(encoding="utf-8"))
    if proof["target_sha256"] != config["expected_snapshot_sha256"]:
        raise ValueError("Configuration evidence belongs to another client version")
    # Rebase the historical absolute path after a project move.
    source = root / ".local/native-baseline" / Path(proof["snapshot"]["path"]).parent.name / "core-config.json"
    payload = source.read_bytes()
    source_hash = hashlib.sha256(payload).hexdigest()
    if source_hash != proof["snapshot"]["sha256"]:
        raise ValueError("Validated core snapshot changed; recapture and validate it before use")
    original = json.loads(payload)
    if original["other_files"]:
        raise ValueError("Only core categories are allowed in a map diagnostic")
    if is_portable(config):
        original = resolve_core(original, root)
    profiles = {"scene": "scene", "original": "base-engine", "navigation": "duel1-navigation", "training": "training"}
    snapshot = apply_profile(original, profiles[mode], number)
    encoded = encode(snapshot)
    expected = {key: snapshot[key] for key in ("other_files", "categories", "cache_flags", "global_ini_strings")}
    if decode(encoded) != expected:
        raise ValueError("Native configuration round-trip mismatch")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(encoded)
    return {"map_number": number, "package": package, "mode": mode,
            "archive": str(output), "archive_sha256": hashlib.sha256(encoded).hexdigest(),
            "source_snapshot_sha256": source_hash, "target_sha256": proof["target_sha256"],
            "round_trip": True, "training_ready_proven": False}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    selection = parser.add_mutually_exclusive_group()
    selection.add_argument("--map", type=int, choices=MAPS, default=1, dest="map_number")
    selection.add_argument("--all", action="store_true")
    parser.add_argument("--mode", choices=("original", "scene", "navigation"), default="original")
    parser.add_argument("--seconds", type=float, default=25)
    parser.add_argument("--run", action="store_true", help="Launch each bounded test after the shared no-login checks")
    args = parser.parse_args()
    max_seconds = 180 if args.mode == "navigation" else 45
    if not 1 <= args.seconds <= max_seconds:
        parser.error(f"seconds must be between 1 and {max_seconds} for this mode")
    if args.mode == "navigation" and (args.all or args.map_number != 1):
        parser.error("Navigation development is restricted to Duel 1")
    session = ROOT / ".local/map-probe" / datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    records = []
    exit_code = 0
    for number in MAPS if args.all else [args.map_number]:
        record = prepare(ROOT, number, args.mode, session / f"map-{number}.bin")
        records.append(record)
        if args.run:
            result_file = session / f"map-{number}-run.json"
            command = [sys.executable, str(ROOT / "tools/run_native_baseline.py"), "--run",
                       "--seconds", str(args.seconds), "--bootstrap", record["archive"],
                       "--map", str(number), "--mode", args.mode, "--result-file", str(result_file)]
            if args.mode == "navigation":
                command.append("--observe-world")
            # The child owns bounded lifecycle/cleanup; wait for it before the next map.
            completed = subprocess.run(command, cwd=ROOT)
            run = json.loads(result_file.read_text(encoding="utf-8")) if result_file.is_file() else {}
            assessment = run.get("runtime_assessment", {})
            world_assessment = run.get("runtime_world_assessment", {})
            record.update({"run_report": run.get("report_path"), "run_status": run.get("status", "missing_report"),
                           "exit_code": run.get("exit_code"), "assessment": assessment,
                           "world_assessment": world_assessment,
                           "image_unchanged": run.get("image_unchanged"),
                           "owned_process_exited": run.get("owned_process_exited")})
            if completed.returncode or run.get("status") != "recorded":
                exit_code = 2
                break
            if args.mode == "navigation":
                navigation_observed = all(world_assessment.get(key) for key in
                    ("target_world_observed", "local_controller_observed", "local_pawn_observed"))
                # Object-chain observations establish navigation prerequisites only.
                blockers = [b for b in assessment.get("active_blockers", []) if b["code"] != "target_world_not_observed"]
                if not navigation_observed or blockers:
                    exit_code = 1
            elif not assessment.get("target_map_ready_observed") or assessment.get("active_blockers"):
                exit_code = 1
            if not run.get("image_unchanged") or run.get("remaining_game_or_launcher_processes"):
                exit_code = 2
                break
        print(json.dumps(record, ensure_ascii=False), flush=True)
    save(session / "summary.json", {"runs": records, "execution_requested": args.run,
                                   "training_ready_proven": False, "exit_code": exit_code})
    print(str(session / "summary.json"))
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
