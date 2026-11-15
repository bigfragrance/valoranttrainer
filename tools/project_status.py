"""Read local project state; this command never starts the game or launcher."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

from native_status import analyze_files

ROOT = Path(__file__).resolve().parents[1]


def main() -> None:
    config = json.loads((ROOT / "project.json").read_text(encoding="utf-8"))
    snapshot = ROOT / config["analysis_snapshot"]
    actual_hash = None
    if snapshot.is_file():
        with snapshot.open("rb") as stream:
            actual_hash = hashlib.file_digest(stream, "sha256").hexdigest()
    migration = ROOT / ".local/migration-manifest.json"
    imported = json.loads(migration.read_text(encoding="utf-8")) if migration.exists() else {}
    log = ROOT / "work/client-analysis/base-engine-textconfig-runtime.log"
    launch = ROOT / "work/client-analysis/base-engine-textconfig-launch.json"
    recorded = analyze_files(log, launch)
    reports = sorted((ROOT / ".local/native-probe").glob("*/report.json"))
    last_capture = None
    for report_path in reversed(reports):
        latest = json.loads(report_path.read_text(encoding="utf-8"))
        if not latest.get("capture_requested"):
            continue
        # Link the local record; do not replay raw process/settings details.
        capture = latest.get("capture") or {}
        last_capture = {"report": str(report_path), "status": latest.get("status"),
                        "engine_class": capture.get("engine_class"),
                        "engine_ini": capture.get("engine_ini"),
                        "capture_passed": latest.get("capture_passed"),
                        "error": capture.get("error") or capture.get("failure_mechanism") or latest.get("error")}
        break
    latest_startup = None
    for report_path in reversed(sorted((ROOT / ".local/native-baseline").glob("*/report.json"))):
        runtime_log = report_path.parent / "runtime.log"
        launch_record = report_path.parent / "launch.json"
        if not runtime_log.is_file() or not launch_record.is_file():
            continue
        current = analyze_files(runtime_log, launch_record)
        latest_startup = {"report": str(report_path), "assessment": current["assessment"],
                          "native_exceptions": current["runtime"]["native_exceptions"],
                          "main_image_crash_stack": current["runtime"]["main_image_crash_stack"]}
        break
    status = {
        "project": str(ROOT),
        "target": config["development_target"],
        "active_development_map": config.get("active_development_map"),
        "account_login": config["account_login"],
        "migration_initial_verification": {
            "files": imported.get("files"), "bytes": imported.get("bytes"),
            "mismatches": len(imported.get("mismatches", [])) if imported else None,
        },
        "snapshot_sha256": actual_hash,
        "snapshot_matches": actual_hash == config["expected_snapshot_sha256"],
        "recorded_native_startup": {
            "log": str(log), "actual_worlds": recorded["runtime"]["world_state_events"],
            "engine_classes": recorded["runtime"]["engine_classes_from_object_paths"],
            "failures": recorded["runtime"]["platform_failures"],
            "assessment": recorded["assessment"],
        },
        "latest_configuration_capture": last_capture,
        "latest_native_startup": latest_startup,
    }
    training_report = ROOT / "reports/training-v2-verification.json"
    if training_report.is_file():
        training = json.loads(training_report.read_text(encoding="utf-8"))
        status["current_training_v2"] = {
            "report": str(training_report),
            "entry": str(ROOT / "启动训练控制台.cmd"),
            "url": "http://127.0.0.1:8790",
            "verified": training.get("verified"),
            "limits": training.get("limits"),
        }
    navigation_report = ROOT / "reports/duel1-navigation-validation.json"
    if navigation_report.is_file():
        navigation = json.loads(navigation_report.read_text(encoding="utf-8"))
        status["duel1_navigation_validation"] = {
            "report": str(navigation_report),
            "target_matches_snapshot": navigation.get("target_sha256") == actual_hash,
            "object_chain": navigation.get("observed_object_chain"),
            "movement_observation": navigation.get("movement_observation"),
            "original_character_movement_proven": False,
            "training_ready_proven": False,
        }
    visual_report = ROOT / "reports/native-scene-validation.json"
    if visual_report.is_file():
        visual = json.loads(visual_report.read_text(encoding="utf-8"))
        status["recorded_scene_visual_validation"] = {
            "report": str(visual_report),
            "target_matches_snapshot": visual.get("target_sha256") == actual_hash,
            "maps_visually_observed": [item["map_number"] for item in visual.get("runs", [])
                                       if item.get("visual_scene_observed")],
            "evidence_kind": "recorded manual viewport inspection; separate from log readiness",
            "training_ready_proven": False,
        }
    print(json.dumps(status, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()

