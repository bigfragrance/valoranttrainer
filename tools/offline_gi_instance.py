"""Separate, hash-bound GameInstance constructor adapter for a new owned child.

This module never writes an original image page. Its only remote writes are the
private DLL pathname and its private install-argument block. The DLL edits the
already constructed UClass's private heap constructor callback and each future
real GameInstance's private vptr. The verified trainer_bridge.dll is unchanged.

Call lifecycle_plan(image), then apply_to_owned_startup(process, plan) from the
existing early owned-child log checkpoint. No arbitrary PID command is exposed.
"""
from __future__ import annotations

import ctypes as C
from ctypes import wintypes as W
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / ".local/offline-gi-instance"
MODULE = BUILD / "offline_gi_instance.dll"
SOURCES = ("native/offline_gi_instance.cpp", "native/build_offline_gi_instance.cmd",
           "tools/offline_gi_instance.py", "tools/local_server_mode_startup.py")
IMAGE_SHA256 = "147763350504d241ee20b77607d04bc85cebfedb1d8bc2f32473ca93bf8fe92f"
CLASS_CACHE, CLASS_CTOR, CLASS_CDO = 0xda6a910, 0xb8, 0x120
ORIGINAL_CTOR, VTABLE, TABLE_BYTES = 0x648a040, 0xb0869c0, 0x460
MAGIC = 0x314947454e4f4c41


def build():
    """Explicit root-owned build of only the separate startup DLL."""
    subprocess.run(["cmd.exe", "/d", "/c", str(ROOT / SOURCES[1])], cwd=ROOT, check=True)
    manifest = {"schema_version": 1, "module": MODULE.relative_to(ROOT).as_posix(),
                "image_sha256": IMAGE_SHA256,
                "sha256": {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest()
                           for p in (*SOURCES, MODULE.relative_to(ROOT).as_posix())}}
    (BUILD / "build.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return manifest


def verified_build():
    manifest = json.loads((BUILD / "build.json").read_text(encoding="utf-8"))
    expected = {*SOURCES, MODULE.relative_to(ROOT).as_posix()}
    if manifest.get("image_sha256") != IMAGE_SHA256 or set(manifest.get("sha256", {})) != expected:
        raise RuntimeError("Separate GameInstance DLL build inventory mismatch")
    for name, digest in manifest["sha256"].items():
        if hashlib.sha256((ROOT / name).read_bytes()).hexdigest() != digest:
            raise RuntimeError(f"Separate GameInstance DLL input changed: {name}")
    return manifest


def lifecycle_plan(image: Path):
    import pefile
    data = Path(image).read_bytes()
    if hashlib.sha256(data).hexdigest() != IMAGE_SHA256:
        raise ValueError("Original GameInstance image mismatch")
    pe = pefile.PE(data=data, fast_load=True)
    original = struct.unpack("<140Q", pe.get_data(VTABLE, TABLE_BYTES))
    for pointer in original:
        section = pe.get_section_by_rva(pointer - pe.OPTIONAL_HEADER.ImageBase)
        if section is None or not section.Characteristics & 0x20000000:
            raise ValueError("Original complete GameInstance vtable extent mismatch")
    if (original[0x308 // 8], original[0x310 // 8]) != tuple(
            pe.OPTIONAL_HEADER.ImageBase + r for r in (0x7047190, 0x7047530)):
        raise ValueError("Original GameInstance lifecycle entries mismatch")
    return {"profile_kind": "original_game_instance_private_constructor_lifecycle",
            "image_sha256": IMAGE_SHA256, "class_cache_rva": CLASS_CACHE,
            "class_constructor_offset": CLASS_CTOR, "original_constructor_rva": ORIGINAL_CTOR,
            "vtable_rva": VTABLE, "vtable_bytes": TABLE_BYTES,
            "init_target_rva": 0x43c4260, "shutdown_target_rva": 0x43c48c0,
            "image_pages_written": False, "requires_pre_cdo_primary_thread_hold": True}


class _Module(C.Structure):
    _fields_ = [("size", W.DWORD), ("id", W.DWORD), ("pid", W.DWORD),
                ("global_count", W.DWORD), ("process_count", W.DWORD),
                ("base", C.c_void_p), ("bytes", W.DWORD), ("module", W.HMODULE),
                ("name", W.WCHAR * 256), ("path", W.WCHAR * 260)]


class _Thread(C.Structure):
    _fields_ = [("size", W.DWORD), ("usage", W.DWORD), ("id", W.DWORD),
                ("owner", W.DWORD), ("base_priority", W.LONG),
                ("delta_priority", W.LONG), ("flags", W.DWORD)]


class _Config(C.Structure):
    _fields_ = [("magic", C.c_uint64), ("version", C.c_uint32), ("bytes", C.c_uint32),
                ("pid", C.c_uint32), ("primary_thread", C.c_uint32), ("image", C.c_uint64),
                ("reserved", C.c_uint32), ("flags", C.c_uint32), ("report", W.WCHAR * 1024)]


class _DiagnosticMemoryStatus(C.Structure):
    _fields_ = [("length", W.DWORD), ("load", W.DWORD)] + [(name, C.c_uint64) for name in
        ("total_phys", "avail_phys", "total_pagefile", "avail_pagefile", "total_virtual", "avail_virtual", "avail_extended")]


class _DiagnosticProcessMemory(C.Structure):
    _fields_ = [("bytes", W.DWORD), ("page_faults", W.DWORD)] + [(name, C.c_size_t) for name in
        ("peak_working_set", "working_set", "peak_paged_pool", "paged_pool", "peak_nonpaged_pool", "nonpaged_pool", "pagefile", "peak_pagefile", "private")]


class _Owned:
    def __init__(self, process, diagnostics=None):
        self.process, self.pid, self.handle = process, process.pid, int(process._handle)
        self._diagnostics = diagnostics if isinstance(diagnostics, dict) else None
        self._diagnostic_primary = {"thread_id": None, "held_by_this_wrapper": False, "actual_suspend_count_known": False}
        self._diagnostic_primary_handle = None
        self._diagnostic_optional = set()
        if self._diagnostics is not None:
            self._diagnostics.update(schema_version=1, pid=self.pid, phase="owned_bind", begin_monotonic_ns=time.perf_counter_ns(),
                                     calls=[], events=[], failure_phase=None, observes_only=True)
        self.api = C.WinDLL("kernel32", use_last_error=True)
        signatures = {
            "GetProcessId": ([W.HANDLE], W.DWORD),
            "GetProcessIdOfThread": ([W.HANDLE], W.DWORD),
            "ReadProcessMemory": ([W.HANDLE, C.c_void_p, C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)], W.BOOL),
            "WriteProcessMemory": ([W.HANDLE, C.c_void_p, C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)], W.BOOL),
            "CreateToolhelp32Snapshot": ([W.DWORD, W.DWORD], W.HANDLE),
            "Module32FirstW": ([W.HANDLE, C.POINTER(_Module)], W.BOOL),
            "Module32NextW": ([W.HANDLE, C.POINTER(_Module)], W.BOOL),
            "Thread32First": ([W.HANDLE, C.POINTER(_Thread)], W.BOOL),
            "Thread32Next": ([W.HANDLE, C.POINTER(_Thread)], W.BOOL),
            "OpenThread": ([W.DWORD, W.BOOL, W.DWORD], W.HANDLE),
            "GetThreadTimes": ([W.HANDLE, C.POINTER(W.FILETIME), C.POINTER(W.FILETIME), C.POINTER(W.FILETIME), C.POINTER(W.FILETIME)], W.BOOL),
            "SuspendThread": ([W.HANDLE], W.DWORD), "ResumeThread": ([W.HANDLE], W.DWORD),
            "CloseHandle": ([W.HANDLE], W.BOOL),
            "GetModuleHandleW": ([W.LPCWSTR], W.HMODULE),
            "GetProcAddress": ([W.HMODULE, C.c_char_p], C.c_void_p),
            "GetModuleHandleExW": ([W.DWORD, C.c_void_p, C.POINTER(W.HMODULE)], W.BOOL),
            "GetModuleFileNameW": ([W.HMODULE, W.LPWSTR, W.DWORD], W.DWORD),
            "VirtualAllocEx": ([W.HANDLE, C.c_void_p, C.c_size_t, W.DWORD, W.DWORD], C.c_void_p),
            "VirtualFreeEx": ([W.HANDLE, C.c_void_p, C.c_size_t, W.DWORD], W.BOOL),
            "CreateRemoteThread": ([W.HANDLE, C.c_void_p, C.c_size_t, C.c_void_p, C.c_void_p, W.DWORD, C.c_void_p], W.HANDLE),
            "WaitForSingleObject": ([W.HANDLE, W.DWORD], W.DWORD),
            "GetExitCodeThread": ([W.HANDLE, C.POINTER(W.DWORD)], W.BOOL),
            "TerminateProcess": ([W.HANDLE, W.UINT], W.BOOL),
        }
        for name, (args, result) in signatures.items():
            fn = getattr(self.api, name); fn.argtypes, fn.restype = args, result
        if self._diagnostics is not None:
            optional = {
                "GetExitCodeProcess": ([W.HANDLE, C.POINTER(W.DWORD)], W.BOOL),
                "GetProcessTimes": ([W.HANDLE, C.POINTER(W.FILETIME), C.POINTER(W.FILETIME), C.POINTER(W.FILETIME), C.POINTER(W.FILETIME)], W.BOOL),
                "GetProcessHandleCount": ([W.HANDLE, C.POINTER(W.DWORD)], W.BOOL),
                "GlobalMemoryStatusEx": ([C.POINTER(_DiagnosticMemoryStatus)], W.BOOL),
                "K32GetProcessMemoryInfo": ([W.HANDLE, C.POINTER(_DiagnosticProcessMemory), W.DWORD], W.BOOL),
            }
            for name, (args, result) in optional.items():
                try:
                    fn = getattr(self.api, name); fn.argtypes, fn.restype = args, result
                    self._diagnostic_optional.add(name)
                except (AttributeError, OSError):
                    pass
        if self.api.GetProcessId(self.handle) != self.pid or process.poll() is not None:
            raise RuntimeError("GameInstance adapter requires this wrapper's live child handle")

        if self._diagnostics is not None:
            self._diagnostics["resources_at_owned_bind"] = self._diagnostic_resources()

    def note(self, phase, **fields):
        # Pure controller-side bounded POD metadata. No writes/queries/waits.
        try:
            d = self._diagnostics
            if d is None:
                return
            d["phase"] = phase
            event = {"phase": phase, "at_ms": (time.perf_counter_ns()-d["begin_monotonic_ns"])/1e6, **fields}
            if len(d["events"]) < 64:
                d["events"].append(event)
            else:
                d["event_overflow"] = d.get("event_overflow", 0)+1
        except Exception:
            pass

    def _diagnostic_update(self, record, **fields):
        try:
            record.update(fields)
        except Exception:
            pass

    def _diagnostic_thread_times(self, thread):
        try:
            values = [W.FILETIME() for _ in range(4)]
            if not self.api.GetThreadTimes(thread, *(C.byref(v) for v in values)):
                return {"query_error": C.get_last_error()}
            quad = lambda v: (v.dwHighDateTime << 32) | v.dwLowDateTime
            # Exit FILETIME is undefined before exit; never report it here.
            return {"creation_filetime": quad(values[0]), "kernel_100ns": quad(values[2]), "user_100ns": quad(values[3])}
        except Exception as error:
            return {"query_exception": type(error).__name__}

    def _diagnostic_resources(self):
        # Used once before the first call, and AFTER a confirmed terminated
        # child only. Never delay fail-closed termination after a wait failure.
        result = {"at_monotonic_ns": time.perf_counter_ns(), "primary": dict(self._diagnostic_primary)}
        def query(name, *args):
            if name not in self._diagnostic_optional:
                result[name] = {"available": False}; return False
            try:
                ok = bool(getattr(self.api, name)(*args))
                if not ok: result[name] = {"query_error": C.get_last_error()}
                return ok
            except Exception as error:
                result[name] = {"query_exception": type(error).__name__}; return False
        try:
            exit_code, handles = W.DWORD(), W.DWORD()
            if query("GetExitCodeProcess", self.handle, C.byref(exit_code)): result["process_exit_code"] = exit_code.value
            if query("GetProcessHandleCount", self.handle, C.byref(handles)): result["process_handle_count"] = handles.value
            values = [W.FILETIME() for _ in range(4)]
            if query("GetProcessTimes", self.handle, *(C.byref(v) for v in values)):
                quad = lambda v: (v.dwHighDateTime << 32) | v.dwLowDateTime
                result["process_times"] = {"creation_filetime": quad(values[0]), "kernel_100ns": quad(values[2]), "user_100ns": quad(values[3])}
            memory = _DiagnosticProcessMemory(); memory.bytes = C.sizeof(memory)
            if query("K32GetProcessMemoryInfo", self.handle, C.byref(memory), memory.bytes):
                result["process_memory"] = {name: int(getattr(memory, name)) for name in ("page_faults", "working_set", "private", "pagefile")}
            status = _DiagnosticMemoryStatus(); status.length = C.sizeof(status)
            if query("GlobalMemoryStatusEx", C.byref(status)):
                result["system_memory"] = {name: int(getattr(status, name)) for name in ("load", "total_phys", "avail_phys", "total_pagefile", "avail_pagefile")}
            if self._diagnostic_primary_handle:
                result["primary_thread_times"] = self._diagnostic_thread_times(self._diagnostic_primary_handle)
        except Exception as error:
            result["snapshot_exception"] = type(error).__name__
        return result

    def read(self, address, size):
        data, count = C.create_string_buffer(size), C.c_size_t()
        if not self.api.ReadProcessMemory(self.handle, address, data, size, C.byref(count)) or count.value != size:
            raise RuntimeError("Owned GameInstance startup read failed")
        return data.raw

    def u64(self, address):
        return struct.unpack("<Q", self.read(address, 8))[0]

    def modules(self):
        # The loader can change its module list during the early owned startup.
        # Win32 explicitly permits retrying ERROR_BAD_LENGTH. Keep it bounded
        # and recheck the actual Popen/handle owner before every new snapshot.
        for attempt in range(16):
            if self.process.poll() is not None or self.api.GetProcessId(self.handle) != self.pid:
                raise RuntimeError('Owned startup child exited before module enumeration')
            snapshot = self.api.CreateToolhelp32Snapshot(8, self.pid)
            if snapshot != W.HANDLE(-1).value:
                break
            error = C.get_last_error()
            if error != 24 or attempt == 15:
                raise RuntimeError(f"Owned startup module enumeration failed (Win32 {error}, owned PID {self.pid})")
            self.note('module_snapshot_retry', error=error, attempt=attempt+1)
            time.sleep(.002)
        found = {}
        try:
            item = _Module(); item.size = C.sizeof(item)
            ok = self.api.Module32FirstW(snapshot, C.byref(item))
            while ok:
                found[item.name.casefold()] = (item.base, item.bytes, Path(item.path))
                ok = self.api.Module32NextW(snapshot, C.byref(item))
        finally:
            self.api.CloseHandle(snapshot)
        return found

    def primary_thread(self):
        snapshot = self.api.CreateToolhelp32Snapshot(4, 0)
        if snapshot == W.HANDLE(-1).value:
            raise RuntimeError("Owned primary thread enumeration failed")
        candidates = []
        try:
            item = _Thread(); item.size = C.sizeof(item)
            ok = self.api.Thread32First(snapshot, C.byref(item))
            while ok:
                if item.owner == self.pid:
                    thread = self.api.OpenThread(0x42, False, item.id)
                    if thread:
                        creation, exit_time, kernel, user = (W.FILETIME() for _ in range(4))
                        if self.api.GetThreadTimes(thread, C.byref(creation), C.byref(exit_time), C.byref(kernel), C.byref(user)):
                            candidates.append(((creation.dwHighDateTime << 32) | creation.dwLowDateTime, item.id))
                        self.api.CloseHandle(thread)
                ok = self.api.Thread32Next(snapshot, C.byref(item))
        finally:
            self.api.CloseHandle(snapshot)
        if not candidates:
            raise RuntimeError("Owned primary thread could not be established")
        return min(candidates)[1]

    def call(self, address, payload, timeout_ms=5000, *, operation="unlabelled", details=None):
        begun = time.perf_counter_ns()
        record = {"operation": operation, "entry_address": hex(address), "payload_bytes": len(payload), "timeout_ms": timeout_ms,
                  "details": dict(details or {}), "primary": dict(self._diagnostic_primary)}
        if self._diagnostics is not None:
            try:
                if len(self._diagnostics["calls"]) < 16: self._diagnostics["calls"].append(record)
            except Exception:
                pass
        self.note("call_begin", operation=operation)
        remote = self.api.VirtualAllocEx(self.handle, None, len(payload), 0x3000, 4)
        if not remote:
            self._diagnostic_update(record, failure_phase="argument_allocation", win32_error=C.get_last_error())
            raise RuntimeError("Owned startup private argument allocation failed")
        thread, completed = None, False
        phase = "argument_write"
        try:
            count, data = C.c_size_t(), C.create_string_buffer(payload)
            if not self.api.WriteProcessMemory(self.handle, remote, data, len(payload), C.byref(count)) or count.value != len(payload):
                raise RuntimeError("Owned startup private argument write failed")
            phase = "remote_thread_create"
            thread_id = W.DWORD()
            thread = self.api.CreateRemoteThread(self.handle, None, 0, address, remote, 0, C.byref(thread_id))
            if not thread:
                raise RuntimeError("Owned startup adapter thread creation failed")
            self._diagnostic_update(record, remote_thread_id=thread_id.value, remote_thread_owner_pid=self.pid)
            if self._diagnostics is not None:
                self._diagnostic_update(record, thread_times_before_wait=self._diagnostic_thread_times(thread))
            self.note("call_wait", operation=operation, remote_thread_id=thread_id.value, timeout_ms=timeout_ms)
            phase = "thread_wait"
            wait_begun = time.perf_counter_ns()
            wait_result = self.api.WaitForSingleObject(thread, timeout_ms)
            wait_error = C.get_last_error() if wait_result == 0xffffffff else None
            completed = wait_result == 0
            self._diagnostic_update(record, wait_result=wait_result, wait_error=wait_error,
                wait_elapsed_ms=(time.perf_counter_ns()-wait_begun)/1e6,
                wait_status={0:"signaled",258:"timeout",0xffffffff:"wait_failed"}.get(wait_result,"unexpected_wait_result"))
            if not completed:
                # No new API query, sleep or wait here. Original finally closes
                # the handle; outer failure path immediately terminates child.
                raise RuntimeError("Owned startup adapter call timed out; child must remain stopped")
            phase = "thread_exit_read"
            result = W.DWORD()
            if not self.api.GetExitCodeThread(thread, C.byref(result)):
                raise RuntimeError("Owned startup adapter return could not be read")
            self._diagnostic_update(record, thread_exit_dword=result.value)
            self.note("call_returned", operation=operation, thread_exit_dword=result.value)
            return result.value
        except Exception as error:
            self._diagnostic_update(record, failure_phase=phase, error_type=type(error).__name__, error=str(error))
            if self._diagnostics is not None:
                self._diagnostics["failure_phase"] = operation+":"+phase
            self.note("call_failed", operation=operation, failure_phase=phase)
            raise
        finally:
            if thread:
                self.api.CloseHandle(thread)
            if not thread or completed:
                self.api.VirtualFreeEx(self.handle, remote, 0, 0x8000)
            self._diagnostic_update(record, elapsed_ms=(time.perf_counter_ns()-begun)/1e6,
                argument_buffer_retained_until_owned_termination=bool(thread and not completed))

    def load(self, module=MODULE):
        module = Path(module).resolve(strict=True)
        if not module.is_relative_to((ROOT / '.local').resolve()):
            raise ValueError("Owned startup modules must stay in project .local")
        address = self.api.GetProcAddress(self.api.GetModuleHandleW("kernel32.dll"), b"LoadLibraryW")
        owner, filename = W.HMODULE(), C.create_unicode_buffer(32768)
        if not address or not self.api.GetModuleHandleExW(6, address, C.byref(owner)):
            raise RuntimeError("Owned startup standard loader could not be identified")
        if not self.api.GetModuleFileNameW(owner, filename, len(filename)):
            raise RuntimeError("Owned startup loader filename unavailable")
        name = Path(filename.value).name.casefold()
        if name not in ("kernel32.dll", "kernelbase.dll"):
            raise RuntimeError("Owned startup loader is outside standard Windows modules")
        remote_owner = self.modules().get(name)
        offset = address - owner.value
        if not remote_owner or not 0 <= offset < remote_owner[1]:
            raise RuntimeError("Owned startup standard loader module mismatch")
        self.call(remote_owner[0] + offset, (str(module) + "\0").encode("utf-16le"), operation="LoadLibraryW",
                  details={"requested_module": str(module), "entry_owner": name, "entry_owner_base": hex(remote_owner[0]),
                           "entry_owner_bytes": remote_owner[1], "entry_rva": offset})
        loaded = self.modules().get(module.name.casefold())
        if not loaded or loaded[2].resolve() != module:
            raise RuntimeError("Separate GameInstance startup DLL did not load")
        return loaded


def apply_to_owned_startup(process, plan, *, timeout=15.0, primary_thread_id=None, report_directory=None,
                           host_mode_startup=None, diagnostics=None):
    """Arm a future original constructor, never race-patch an existing GI.

    Polling only selects an already registered class. Installation is synchronous
    while the verified primary thread is suspended, and the DLL REJECTS an
    existing CDO. Thus a missed early registration window fails the owned launch;
    it is never reported as a successful installation. Any error terminates this
    owned child without resuming its held primary thread.
    """
    import pefile
    if not isinstance(plan, dict) or plan.get("image_sha256") != IMAGE_SHA256 or plan.get("profile_kind") != "original_game_instance_private_constructor_lifecycle":
        raise ValueError("Unexpected separate GameInstance startup plan")
    manifest = verified_build()
    owned = _Owned(process, diagnostics=diagnostics)
    directory = Path(report_directory or BUILD).resolve(); directory.mkdir(parents=True, exist_ok=True)
    if not directory.is_relative_to(ROOT.resolve()):
        raise ValueError("GameInstance startup diagnostics must remain in the workspace")
    path = directory / f"offline-gi-instance-{process.pid}.json"
    if len(str(path)) >= 1024:
        raise ValueError("GameInstance diagnostics path too long")
    thread, held, success = None, False, False
    try:
        # Loader calls complete BEFORE the primary thread hold. DllMain performs
        # no game action, registration call, polling loop or callback replacement.
        owned.note("gi_loader_begin", native_report_path=str(path))
        loaded = owned.load()
        owned.note("gi_loader_verified")
        primary = primary_thread_id or owned.primary_thread()
        owned._diagnostic_primary["thread_id"] = primary
        owned.note("primary_identified", primary_thread_id=primary)
        main_module = next((m for m in owned.modules().values() if m[2].suffix.casefold() == ".exe"), None)
        if not main_module:
            raise RuntimeError("Owned original image module unavailable")
        base = main_module[0]
        native_mode = None
        if host_mode_startup is not None:
            # Both DLL loader calls finish before the primary-thread hold.
            # The mode marker and network POD are staged before its DllMain.
            from local_server_mode_startup import prepare_for_hold
            native_mode = prepare_for_hold(owned, host_mode_startup, primary, base)
        owned.note("class_registration_wait")
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError("Owned child exited before original class registration")
            candidate = owned.u64(base + CLASS_CACHE)
            if candidate and owned.u64(candidate + CLASS_CTOR) == base + ORIGINAL_CTOR:
                break
            time.sleep(.001)
        else:
            raise RuntimeError("Original class was not registered before the startup deadline")
        thread = owned.api.OpenThread(0x42, False, primary)
        if not thread or owned.api.GetProcessIdOfThread(thread) != process.pid:
            raise RuntimeError("Startup primary thread does not belong to this owned child")
        owned.note("primary_hold_request", primary_thread_id=primary)
        previous_suspend_count = owned.api.SuspendThread(thread)
        if previous_suspend_count == 0xffffffff:
            raise RuntimeError("Owned primary thread could not be held for constructor installation")
        held = True
        owned._diagnostic_primary.update(held_by_this_wrapper=True, previous_suspend_count=previous_suspend_count)
        owned._diagnostic_primary_handle = thread
        owned.note("primary_hold_succeeded", previous_suspend_count=previous_suspend_count)
        # Re-read under the actual hold; the initial class observation is not an
        # installation proof. An already existing CDO is a fail-closed late hold.
        candidate = owned.u64(base + CLASS_CACHE)
        if not candidate or owned.u64(candidate + CLASS_CTOR) != base + ORIGINAL_CTOR:
            raise RuntimeError("Original class changed before the primary thread hold")
        if owned.u64(candidate + CLASS_CDO):
            raise RuntimeError("Original class CDO already exists; an earlier startup checkpoint is required")
        pe = pefile.PE(str(MODULE), fast_load=False)
        export = next((e.address for e in pe.DIRECTORY_ENTRY_EXPORT.symbols if e.name == b"OfflineGiInstanceInstall"), None)
        if export is None:
            raise RuntimeError("Separate GameInstance installer export missing")
        config = _Config(); config.magic, config.version, config.bytes = MAGIC, 1, C.sizeof(_Config)
        config.pid, config.primary_thread, config.image, config.flags = process.pid, primary, base, 1
        config.report = str(path)
        if C.sizeof(config) != 2088:
            raise RuntimeError("Separate GameInstance installer ABI mismatch")
        code = owned.call(loaded[0] + export, bytes(config), operation="OfflineGiInstanceInstall",
                          details={"module": str(loaded[2]), "module_base": hex(loaded[0]), "entry_rva": export})
        state = json.loads(path.read_text(encoding="utf-8")) if path.exists() else {}
        if code or state.get("pid") != process.pid or state.get("installed") != 1 or state.get("error"):
            raise RuntimeError(f"Original class constructor install failed ({code}): {state.get('stage', 'no marker')}")
        if not state.get("primary_suspension_verified") or not state.get("installed_before_cdo") or not state.get("source_valid_and_shared"):
            raise RuntimeError("Original class constructor early/shared-page proof incomplete")
        if native_mode is not None:
            from local_server_mode_startup import activate_under_hold
            activate_under_hold(owned, loaded, native_mode)
            state = json.loads(path.read_text(encoding="utf-8"))
            if state.get('error') or state.get('host_mode_factory_bound') is not True:
                raise RuntimeError('Original GI host-mode factory binding readback missing')
        success = True
        owned.note("startup_lifecycle_verified")
        return {"profile_kind": plan["profile_kind"], "module": str(MODULE),
                "module_sha256": manifest["sha256"][MODULE.relative_to(ROOT).as_posix()],
                "report_path": str(path), "state": state, "instruction_bytes_modified": False,
                "image_pages_written": False, "file_modified": False,
                "page_protections_restored": True, "private_instance_lifecycle": True,
                "host_mode_startup": native_mode['receipt'] if native_mode is not None else None}
    finally:
        if not success:
            # Never resume a launch whose offline lifecycle was not proven.
            owned.note("owned_failure_termination_requested", primary_held_by_wrapper=held, will_resume=False)
            terminated = owned.api.TerminateProcess(owned.handle, 0x47514946)
            stopped = owned.api.WaitForSingleObject(owned.handle, 5000)
            owned.note("owned_failure_termination_wait", terminate_return=bool(terminated), raw_wait=stopped, will_resume=False)
            if stopped == 0 and owned._diagnostics is not None:
                owned._diagnostics["resources_after_confirmed_termination"] = owned._diagnostic_resources()
        if held and success:
            resumed = owned.api.ResumeThread(thread)
            resume_succeeded = resumed != 0xffffffff
            if resume_succeeded:
                owned._diagnostic_primary["held_by_this_wrapper"] = False
            owned.note("primary_resume_returned_after_verified_success", raw_return=resumed,
                       resume_succeeded=resume_succeeded,
                       held_by_this_wrapper=owned._diagnostic_primary["held_by_this_wrapper"])
        if thread:
            owned.api.CloseHandle(thread)


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", action="store_true", help="Build only the independent startup DLL")
    args = parser.parse_args()
    print(json.dumps(build() if args.build else verified_build(), indent=2))
