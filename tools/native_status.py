"""Summarize one recorded Unreal startup attempt without running the client.

Only selected structured observations are emitted. Raw log messages, command-line
arguments, URL queries, process IDs, UObject instance IDs and tokens are omitted.
A successful analysis (CLI exit 0) does not mean the recorded client started.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import sys
from typing import Any


_IDENTIFIER = r"[A-Za-z_][A-Za-z0-9_]{0,119}"
_FAILURE = re.compile(
    rf"Failed to initialize:\s*({_IDENTIFIER})\s*"
    rf"\(error:\s*({_IDENTIFIER})\)"
)
_STEP = re.compile(rf"\b(Initializing|Initialized):\s*({_IDENTIFIER})\b")
_STATUS = re.compile(r"Status is now:\s*(Uninitialized|Initializing|Initialized|InitFailure)\b")
_ENGINE = re.compile(r"/Engine/Transient\.((?:[A-Za-z_][A-Za-z0-9_]*)?GameEngine)_\d+\b")
_GAME_INSTANCE = re.compile(
    r"[/:.]((?:[A-Za-z_][A-Za-z0-9_]*)?GameInstance(?:_C)?)_\d+\b"
)
_SERIAL_SIZE = re.compile(
    r"Serial size mismatch:\s*Expected read size\s+(\d+),\s*Actual read size\s+(\d+)"
)
_PACKAGE = re.compile(r"^/(?:Game|Engine)/[A-Za-z0-9_./-]+$")


def canonical_package(value: Any) -> str | None:
    """Return a cooked package name, excluding query values and object suffixes."""
    if not isinstance(value, str):
        return None
    candidate = value.strip().strip("\"'").split("?", 1)[0].split("#", 1)[0]
    candidate = candidate.rstrip("\"',;)]")
    if not _PACKAGE.fullmatch(candidate) or ".." in candidate.split("/"):
        return None
    # Unreal can log either a package or its contained world object.
    head, tail = candidate.rsplit("/", 1)
    return head + "/" + tail.split(".", 1)[0]


def _requested_map(launch: dict[str, Any]) -> str | None:
    arguments = launch.get("arguments", [])
    if not isinstance(arguments, list):
        return None
    for argument in arguments:
        package = canonical_package(argument)
        if package is not None:
            return package
    return None


def _recorded_bool(launch: dict[str, Any], name: str) -> bool | None:
    value = launch.get(name)
    return value if isinstance(value, bool) else None


def _add_class(records: list[dict[str, Any]], name: str, line: int) -> None:
    for record in records:
        if record["class"] == name:
            if line not in record["evidence_lines"]:
                record["evidence_lines"].append(line)
            return
    records.append({"class": name, "evidence_lines": [line]})


def _world_flags(message: str) -> dict[str, bool]:
    result = {}
    for label, key in (
        ("Local World", "local_world"),
        ("Map Ready", "map_ready"),
        ("Map Complete", "map_complete"),
    ):
        match = re.search(rf"\[{label}:\s*(TRUE|FALSE)\b", message)
        if match:
            result[key] = match.group(1) == "TRUE"
    return result


def analyze_runtime(
    text: str,
    launch: dict[str, Any] | None = None,
    target_map: str | None = None,
) -> dict[str, Any]:
    """Analyze a single log file; missing evidence remains unknown, never success.

    Launch metadata describes the recorded request only. Object-path class names
    and MapLoadModel/LogWorld events supply runtime observations. Browse/LoadMap
    messages are kept separately because requesting travel does not complete it.
    """
    if launch is not None and not isinstance(launch, dict):
        raise ValueError("Launch metadata must be a JSON object.")
    launch_supplied = launch is not None
    launch = launch or {}
    requested = canonical_package(target_map) if target_map is not None else _requested_map(launch)
    if target_map is not None and requested is None:
        raise ValueError("Target map must be a /Game/ or /Engine/ package path.")

    worlds: list[dict[str, Any]] = []
    travel_requests: list[dict[str, Any]] = []
    engines: list[dict[str, Any]] = []
    game_instances: list[dict[str, Any]] = []
    platform_events: list[dict[str, Any]] = []
    failures: list[dict[str, Any]] = []
    serial_errors: list[dict[str, Any]] = []
    rnet_lines: list[int] = []
    native_exceptions: list[dict[str, Any]] = []
    main_image_stack: list[dict[str, Any]] = []
    final_status = None
    final_status_line = None

    for line_number, message in enumerate(text.splitlines(), 1):
        for engine in _ENGINE.finditer(message):
            _add_class(engines, engine.group(1), line_number)
        for instance in _GAME_INSTANCE.finditer(message):
            _add_class(game_instances, instance.group(1), line_number)

        if "LogMapLoadModel:" in message:
            match = re.search(r"\[URL:\s*([^\]\s]+)\]", message)
            package = canonical_package(match.group(1)) if match else None
            if package:
                worlds.append({"line": line_number, "package": package,
                               "kind": "map_load_model", **_world_flags(message)})
        elif "LogWorld:" in message:
            match = re.search(r"Bringing World\s+(\S+)\s+up for play\b", message)
            package = canonical_package(match.group(1)) if match else None
            if package:
                worlds.append({"line": line_number, "package": package,
                               "kind": "world_starting_play", "map_ready": True})

        if "LogLoad:" in message or "LogNet:" in message:
            match = re.search(r"\b(Browse|LoadMap):\s*(\S+)", message)
            package = canonical_package(match.group(2)) if match else None
            if package:
                travel_requests.append({"line": line_number, "package": package,
                                        "kind": match.group(1).lower()})

        if "LogPlatformInitializerV2:" in message:
            match = _FAILURE.search(message)
            if match:
                failure = {"line": line_number, "system": match.group(1),
                           "reason": match.group(2)}
                failures.append(failure)
                platform_events.append({**failure, "state": "failed"})
            else:
                match = _STEP.search(message)
                if match:
                    platform_events.append({"line": line_number,
                                            "system": match.group(2),
                                            "state": match.group(1).lower()})
            match = _STATUS.search(message)
            if match:
                final_status, final_status_line = match.group(1), line_number
                platform_events.append({"line": line_number, "state": final_status})
            elif "Beginning platform initialization" in message:
                platform_events.append({"line": line_number, "state": "begin"})

        match = _SERIAL_SIZE.search(message)
        if match:
            prefix = message[:match.start()]
            asset_match = re.search(r"ObjectSerializationError:\s*(/[^\s]+)", prefix)
            asset = canonical_package(asset_match.group(1)) if asset_match else None
            signature = (asset, int(match.group(1)), int(match.group(2)))
            existing = next((item for item in serial_errors
                             if (item["asset"], item["expected_bytes"], item["actual_bytes"])
                             == signature), None)
            if existing is None:
                serial_errors.append({"asset": asset, "expected_bytes": signature[1],
                                      "actual_bytes": signature[2],
                                      "evidence_lines": [line_number]})
            else:
                existing["evidence_lines"].append(line_number)
        if "LogRSOManager:" in message and "RNET_NOT_CONNECTED" in message:
            rnet_lines.append(line_number)
        exception = re.search(r"Unhandled Exception:\s*(EXCEPTION_[A-Z0-9_]+)", message)
        if exception:
            native_exceptions.append({"line": line_number, "exception": exception.group(1)})
        if "[Callstack]" in message and "VALORANT-Win64-Shipping.exe!" in message:
            location = re.search(r"\(module \+ (0x[0-9A-Fa-f]+)\)", message)
            if location:
                main_image_stack.append({"line": line_number, "rva": hex(int(location.group(1), 16))})

    target_events = [event for event in worlds if event["package"] == requested] if requested else []
    target_ready = any(event.get("map_ready") is True for event in target_events)
    active_blockers: list[dict[str, Any]] = []
    # A later overall Initialized state clears a previous initializer failure;
    # the failure itself remains visible in the historical event list.
    platform_failed = bool(failures) and not (
        final_status == "Initialized" and final_status_line > failures[-1]["line"]
    )
    if platform_failed or final_status == "InitFailure":
        evidence = [failure["line"] for failure in failures]
        if final_status == "InitFailure":
            evidence.append(final_status_line)
        active_blockers.append({"code": "platform_initialization_failure",
                                "evidence_lines": sorted(set(evidence))})
    if serial_errors:
        active_blockers.append({"code": "object_serialization_mismatch",
                                "evidence_lines": sorted({line for item in serial_errors
                                                          for line in item["evidence_lines"]})})
    if native_exceptions:
        active_blockers.append({"code": "native_exception", "evidence_lines": [item["line"] for item in native_exceptions]})
    if requested and not target_events:
        active_blockers.append({"code": "target_world_not_observed",
                                "evidence_lines": [event["line"] for event in worlds]})
    elif requested and not target_ready:
        active_blockers.append({"code": "target_map_ready_not_observed",
                                "evidence_lines": [event["line"] for event in target_events]})

    if active_blockers:
        outcome = "blocked_or_unproven"
    elif target_ready:
        outcome = "target_map_ready_observed"
    elif worlds:
        outcome = "world_state_observed"
    else:
        outcome = "no_world_evidence"
    recorded_exit = launch.get("exit_code")
    if not isinstance(recorded_exit, int) or isinstance(recorded_exit, bool):
        recorded_exit = None
    return {
        "schema_version": 1,
        "scope": "One recorded startup log; no live process, network or account verification.",
        "requested_map": requested,
        "launch_record": {
            "supplied": launch_supplied,
            "recorded_exit_code": recorded_exit,
            "recorded_credentials_supplied": _recorded_bool(launch, "credentials_supplied"),
            "recorded_login_action": _recorded_bool(launch, "login_action"),
        },
        "runtime": {
            "world_state_events": worlds,
            "map_travel_requests": travel_requests,
            "engine_classes_from_object_paths": engines,
            "game_instance_classes_from_object_paths": game_instances,
            "platform_events": platform_events,
            "platform_final_status": final_status,
            "platform_failures": failures,
            "serialization_errors": serial_errors,
            "native_exceptions": native_exceptions,
            "main_image_crash_stack": main_image_stack,
            "rnet_not_connected_lines": rnet_lines,
        },
        "assessment": {
            "outcome": outcome,
            "target_world_observed": bool(target_events),
            "target_map_ready_observed": target_ready,
            "training_ready_proven": False,
            "account_login_success_proven": False,
            "active_blockers": active_blockers,
            "limits": [
                "Exit code zero and launch arguments do not prove world entry.",
                "Browse/LoadMap requests do not prove completed map loading.",
                "Map Ready or world-starting-play events do not prove a working player, weapon or bot drill.",
                "An initialized authentication manager does not establish an account login.",
                "This parser does not authenticate log provenance or determine whether account activity occurred.",
                "Class evidence is limited to class names present in logged UObject paths.",
            ],
        },
    }


def analyze_files(
    log_path: str | Path,
    launch_path: str | Path | None = None,
    target_map: str | None = None,
) -> dict[str, Any]:
    raw = Path(log_path).read_bytes()
    encoding = "utf-16" if raw.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig"
    log = raw.decode(encoding, errors="replace")
    launch = None
    if launch_path is not None:
        launch = json.loads(Path(launch_path).read_text(encoding="utf-8-sig"))
    return analyze_runtime(log, launch, target_map)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--log", required=True, type=Path, help="Recorded Unreal runtime log")
    parser.add_argument("--launch", type=Path, help="Optional recorded launch JSON")
    parser.add_argument("--target-map", help="Override requested map used for assessment")
    parser.add_argument("--output", type=Path, help="Write the structured report to this path")
    args = parser.parse_args(argv)
    try:
        report = analyze_files(args.log, args.launch, args.target_map)
        encoded = json.dumps(report, ensure_ascii=False, indent=2) + "\n"
        if args.output is not None:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(encoded, encoding="utf-8")
        sys.stdout.write(encoded)
        return 0
    except (OSError, ValueError) as error:
        # File contents and JSON decoder snippets can contain sensitive values.
        # Only the exception type is needed to identify a failed local analysis.
        sys.stderr.write(f"Could not analyze the local inputs ({type(error).__name__}).\n")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
