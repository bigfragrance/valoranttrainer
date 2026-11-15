"""Preflight and run one bounded, pre-engine game configuration capture.

Default: inspect prerequisites without launching the game. --capture launches
the selected capture probe, which samples GetString's returned configuration
and stops its own game process before engine-class loading. Class-override
experiments are never selected by this wrapper.
"""
from __future__ import annotations

import argparse
import ctypes
from ctypes import wintypes
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time

from client_layout import client_executable, is_portable


ROOT = Path(__file__).resolve().parents[1]
PROBE_DIR = ROOT / "work/client-analysis/debug-probe-cpp"
METHODS = {"snapshot": ("engine_snapshot_probe", "build-snapshot.cmd"),
           "hardware": ("engine_config_probe", "build.cmd"),
           "debugger": ("engine_config_probe", "build.cmd")}
SUPPORTED_GAME = Path("F:/Riot Games/VALORANT/live/ShooterGame/Binaries/Win64/VALORANT-Win64-Shipping.exe")


def sha256(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def save(path: Path, value: dict) -> None:
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding="utf-8")


def game_processes() -> list[dict]:
    """Read process identities and image paths, without command lines or tokens."""
    class ProcessEntry(ctypes.Structure):
        _fields_ = [("dwSize", wintypes.DWORD), ("cntUsage", wintypes.DWORD),
                    ("th32ProcessID", wintypes.DWORD), ("th32DefaultHeapID", ctypes.c_size_t),
                    ("th32ModuleID", wintypes.DWORD), ("cntThreads", wintypes.DWORD),
                    ("th32ParentProcessID", wintypes.DWORD), ("pcPriClassBase", wintypes.LONG),
                    ("dwFlags", wintypes.DWORD), ("szExeFile", wintypes.WCHAR * 260)]

    api = ctypes.WinDLL("kernel32", use_last_error=True)
    api.CreateToolhelp32Snapshot.argtypes = [wintypes.DWORD, wintypes.DWORD]
    api.CreateToolhelp32Snapshot.restype = wintypes.HANDLE
    api.Process32FirstW.argtypes = [wintypes.HANDLE, ctypes.POINTER(ProcessEntry)]
    api.Process32NextW.argtypes = [wintypes.HANDLE, ctypes.POINTER(ProcessEntry)]
    api.CloseHandle.argtypes = [wintypes.HANDLE]
    api.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    api.OpenProcess.restype = wintypes.HANDLE
    api.QueryFullProcessImageNameW.argtypes = [wintypes.HANDLE, wintypes.DWORD,
                                             wintypes.LPWSTR, ctypes.POINTER(wintypes.DWORD)]
    snapshot = api.CreateToolhelp32Snapshot(2, 0)
    if snapshot == wintypes.HANDLE(-1).value:
        raise RuntimeError("Process-name enumeration failed")
    try:
        entry = ProcessEntry()
        entry.dwSize = ctypes.sizeof(entry)
        found = bool(api.Process32FirstW(snapshot, ctypes.byref(entry)))
        if not found:
            raise RuntimeError("Process-name enumeration failed")
        matches = []
        while found:
            name = entry.szExeFile.casefold()
            if name.startswith("riotclient") or "valorant" in name:
                item = {"pid": entry.th32ProcessID, "parent_pid": entry.th32ParentProcessID,
                        "name": entry.szExeFile, "executable_path": None}
                handle = api.OpenProcess(0x1000, False, entry.th32ProcessID)
                if handle:
                    try:
                        path = ctypes.create_unicode_buffer(32768)
                        count = wintypes.DWORD(len(path))
                        if api.QueryFullProcessImageNameW(handle, 0, path, ctypes.byref(count)):
                            item["executable_path"] = path.value
                    finally:
                        api.CloseHandle(handle)
                matches.append(item)
            found = bool(api.Process32NextW(snapshot, ctypes.byref(entry)))
        if ctypes.get_last_error() != 18:  # ERROR_NO_MORE_FILES
            raise RuntimeError("Process-name enumeration did not complete")
        return matches
    finally:
        api.CloseHandle(snapshot)


def partition_game_processes(processes: list[dict], executable: Path) -> tuple[list[dict], list[dict]]:
    """Keep the verified installation isolated from a separate regional client.

    Unknown image paths still block startup. A positively identified foreign
    installation is reported, never opened for mutation or included in cleanup.
    """
    executable = executable.resolve()
    installation = next((p for p in executable.parents if p.name.casefold() == "riot games"), executable.parent)
    blocking, foreign = [], []
    for item in processes:
        path = item.get("executable_path")
        image = Path(path).resolve() if path else None
        (blocking if image is None or image == executable or installation in image.parents else foreign).append(item)
    return blocking, foreign


def matching_saved_credential() -> bool:
    """Inspect TargetName only; never read Username, CredentialBlob or attributes."""
    class Credential(ctypes.Structure):
        _fields_ = [("Flags", wintypes.DWORD), ("Type", wintypes.DWORD),
                    ("TargetName", wintypes.LPWSTR), ("Comment", wintypes.LPWSTR),
                    ("LastWritten", wintypes.FILETIME), ("CredentialBlobSize", wintypes.DWORD),
                    ("CredentialBlob", ctypes.c_void_p), ("Persist", wintypes.DWORD),
                    ("AttributeCount", wintypes.DWORD), ("Attributes", ctypes.c_void_p),
                    ("TargetAlias", wintypes.LPWSTR), ("UserName", wintypes.LPWSTR)]

    api = ctypes.WinDLL("advapi32", use_last_error=True)
    array = ctypes.POINTER(ctypes.POINTER(Credential))()
    count = wintypes.DWORD()
    api.CredEnumerateW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD,
                                  ctypes.POINTER(wintypes.DWORD),
                                  ctypes.POINTER(ctypes.POINTER(ctypes.POINTER(Credential)))]
    api.CredEnumerateW.restype = wintypes.BOOL
    api.CredFree.argtypes = [ctypes.c_void_p]
    if not api.CredEnumerateW(None, 0, ctypes.byref(count), ctypes.byref(array)):
        if ctypes.get_last_error() == 1168:  # ERROR_NOT_FOUND: no stored credentials
            return False
        raise RuntimeError("Saved-credential presence check failed")
    try:
        return any(re.search(r"riot|valorant", array[i].contents.TargetName or "", re.I)
                   is not None for i in range(count.value))
    finally:
        api.CredFree(array)


def saved_login_flags() -> dict:
    import yaml  # Already used by the imported no-credential launch tooling.

    path = Path(os.environ["LOCALAPPDATA"]) / "Riot Games/Riot Client/Data/RiotGamesPrivateSettings.yaml"
    result = {"settings_file_present": path.is_file(),
              "saved_login_present": False, "saved_authorization_present": False}
    if not path.is_file():
        return result
    try:
        document = yaml.safe_load(path.read_text(encoding="utf-8-sig")) or {}
    except Exception:
        raise RuntimeError("Cannot safely parse active local settings") from None
    if not isinstance(document, dict):
        raise RuntimeError("Unexpected active local settings format")

    def present(*keys: str) -> bool:
        node = document
        for key in keys:
            if node is None:
                return False
            if not isinstance(node, dict):
                raise RuntimeError("Unexpected active local settings structure")
            node = node.get(key)
        return bool(node)

    result["saved_login_present"] = present("riot-login", "persist")
    result["saved_authorization_present"] = present("psl", "authorization", "riot-client")
    return result


def preflight(config: dict) -> dict:
    if os.name != "nt":
        raise RuntimeError("This capture wrapper requires Windows")
    if config.get("account_login") != "forbidden":
        raise RuntimeError("Project must preserve the no-account-login boundary")
    portable = is_portable(config)
    executable = client_executable(config, ROOT)
    if not portable and executable != SUPPORTED_GAME.resolve(strict=True):
        raise RuntimeError("Native capture currently supports only its verified installation path")
    actual_hash = sha256(executable)
    actual_size = executable.stat().st_size
    # A complete offline package has its own core cache and settings directory.
    # It never needs to inspect another client's identity or saved credentials.
    flags = ({"settings_file_present": None, "saved_login_present": None,
              "saved_authorization_present": None} if portable else saved_login_flags())
    credential_present = None if portable else matching_saved_credential()
    blocking, foreign = partition_game_processes(game_processes(), executable)
    result = {"time_utc": datetime.now(timezone.utc).isoformat(),
              "game_sha256": actual_hash, "game_bytes": actual_size,
              "expected_image_matches": actual_hash == config["expected_snapshot_sha256"]
                                        and actual_size == config["expected_snapshot_bytes"],
              "process_scope": "bundled_client" if portable else "verified_installation",
              "existing_game_or_launcher_processes": bool(blocking),
              "blocking_processes": blocking,
              "separate_installation_processes": foreign,
              **flags, "saved_identity_checked": not portable,
              "matching_saved_credential_present": credential_present,
              "credentials_supplied": False, "login_action_requested": False}
    result["ready"] = (result["expected_image_matches"] and not any(result[key] for key in
                       ("existing_game_or_launcher_processes", "saved_login_present",
                        "saved_authorization_present", "matching_saved_credential_present")))
    return result


def build(method: str) -> None:
    command = [os.environ.get("COMSPEC", "cmd.exe"), "/d", "/c", str(PROBE_DIR / METHODS[method][1])]
    completed = subprocess.run(command, cwd=ROOT, capture_output=True, text=True,
                               creationflags=subprocess.CREATE_NO_WINDOW, timeout=60)
    if completed.returncode:
        raise RuntimeError("Native capture build failed; inspect the build command directly")


def run_capture(run_dir: Path, method: str) -> dict:
    stem = METHODS[method][0]
    probe = PROBE_DIR / (stem + ".exe")
    source = PROBE_DIR / (stem + ".cpp")
    header = PROBE_DIR / "workspace_paths.h"
    headers = [header] + ([PROBE_DIR / "debug_exception_policy.h"] if method in ("debugger", "hardware") else [])
    if not probe.is_file() or probe.stat().st_mtime < max(p.stat().st_mtime for p in [source, *headers]):
        raise RuntimeError("Native capture is missing or stale; use --build")
    source_before = sha256(source)
    headers_before = {p.name: sha256(p) for p in headers}
    probe_before = sha256(probe)
    runtime_log = PROBE_DIR / ("snapshot-runtime.log" if method == "snapshot" else "runtime.log")
    if runtime_log.is_file():
        shutil.copy2(runtime_log, run_dir / "previous-runtime.log")
    result_path = run_dir / "capture.json"
    command = [str(probe), "--output", str(result_path)] + (["--hardware"] if method == "hardware" else [])
    process = subprocess.Popen(command, cwd=ROOT,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               creationflags=subprocess.CREATE_NO_WINDOW)
    started = time.monotonic()
    next_notice = started + 25
    timed_out = False
    while True:
        try:
            process.communicate(timeout=.25)
            break
        except subprocess.TimeoutExpired:
            now = time.monotonic()
            if now >= next_notice:
                print(json.dumps({"capture_running": True, "elapsed_seconds": int(now - started)}), flush=True)
                next_notice = now + 25
            if now - started > (45 if method == "snapshot" else 115):
                # Terminate only this wrapper-owned capture process. The native
                # probe owns kill-on-close/debugger-exit cleanup of its child.
                process.kill()
                process.communicate(timeout=10)
                timed_out = True
                break
    capture = json.loads(result_path.read_text(encoding="utf-8")) if result_path.is_file() else None
    if runtime_log.is_file():
        shutil.copy2(runtime_log, run_dir / "runtime.log")
    source_after = sha256(source)
    headers_after = {p.name: sha256(p) for p in headers}
    return {"probe_returncode": process.returncode, "wrapper_timed_out": timed_out,
            "probe_sha256": probe_before, "source_sha256": source_before,
            "source_sha256_after": source_after, "header_sha256": headers_before[header.name],
            "headers_sha256": headers_before,
            "source_changed_during_capture": source_before != source_after or headers_before != headers_after,
            "probe_changed_during_capture": probe_before != sha256(probe), "capture": capture}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", action="store_true", help="Compile only the configuration-capture probe")
    parser.add_argument("--capture", action="store_true", help="Run one bounded pre-engine capture after successful preflight")
    parser.add_argument("--method", choices=sorted(METHODS), default="snapshot", help="Pre-engine capture method (default: snapshot)")
    args = parser.parse_args()
    run_dir = ROOT / ".local/native-probe" / datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    run_dir.mkdir(parents=True)
    report = {"scope": "Owned game process; stop before engine class loading; no account actions",
              "capture_requested": args.capture, "build_requested": args.build, "method": args.method}
    try:
        config = json.loads((ROOT / "project.json").read_text(encoding="utf-8"))
        report["preflight"] = preflight(config)
        save(run_dir / "preflight.json", report["preflight"])
        if args.build:
            build(args.method)
            report["build_passed"] = True
        if args.capture:
            # Recheck after the build, in case a launcher or game started meanwhile.
            report["preflight"] = preflight(config)
            save(run_dir / "preflight.json", report["preflight"])
            if not report["preflight"]["ready"]:
                raise RuntimeError("Capture blocked by preflight; no game process launched")
            report.update(run_capture(run_dir, args.method))
            report["postflight"] = preflight(config)
            report["image_unchanged"] = report["preflight"]["game_sha256"] == report["postflight"]["game_sha256"]
            capture = report.get("capture") or {}
            report["captured_pid_present_after"] = any(p["pid"] == capture.get("pid") for p in game_processes())
            report["capture_passed"] = (report["probe_returncode"] == 0 and not report["wrapper_timed_out"]
                                        and not report["source_changed_during_capture"]
                                        and not report["probe_changed_during_capture"]
                                        and capture.get("status") == "captured"
                                        and capture.get("owned_process_exited") is True
                                        and (args.method != "snapshot" or capture.get("stopped_before_engine_load") is True)
                                        and not report["captured_pid_present_after"]
                                        and report["image_unchanged"] and report["postflight"]["ready"])
        report["status"] = "ok" if not args.capture or report.get("capture_passed") else "capture_failed"
    except Exception as error:
        # Known validation messages carry no settings values or credential names.
        report["status"] = "error"
        report["error"] = str(error) if isinstance(error, RuntimeError) else type(error).__name__
    save(run_dir / "report.json", report)
    print(json.dumps({"status": report["status"], "report": str(run_dir / "report.json"),
                      "preflight": report.get("preflight"), "capture": report.get("capture"),
                      "capture_passed": report.get("capture_passed"), "error": report.get("error")},
                     ensure_ascii=False), flush=True)
    return 0 if report["status"] == "ok" else 1


if __name__ == "__main__":
    raise SystemExit(main())
