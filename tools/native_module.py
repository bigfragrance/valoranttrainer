"""Load only this project's hash-bound training module into the wrapper-owned child."""
from __future__ import annotations

import ctypes
from ctypes import wintypes
import hashlib
import json
import struct
import re
from pathlib import Path
import time

ROOT = Path(__file__).resolve().parents[1]

def build_inputs():
    return tuple(sorted(p.relative_to(ROOT).as_posix() for p in (ROOT / 'native').iterdir()
                        if p.is_file() and p.suffix in ('.h', '.cpp', '.cmd'))) + ('.local/native-module/trainer_bridge.dll',)


def verified_build():
    """Validate the shipped module against its captured source inventory."""
    manifest = json.loads((ROOT / ".local/native-module/build.json").read_text(encoding="utf-8"))
    if manifest.get('source_snapshot') != '.local/native-module/source-snapshot':
        raise RuntimeError('Native module requires a captured build-source snapshot; rebuild it')
    source_root = ROOT / manifest['source_snapshot']
    if not {'native/trainer_bridge.cpp', 'native/engine_api.h', 'native/build.cmd', '.local/native-module/trainer_bridge.dll'} <= set(manifest['sha256']):
        raise RuntimeError('Native module build source inventory is incomplete')
    for relative, expected in manifest["sha256"].items():
        if relative == '.local/native-module/trainer_bridge.dll':
            actual = ROOT / relative
        elif re.fullmatch(r'native/[A-Za-z0-9_]+(?:\.generated)?\.(?:h|cpp|cmd)', relative):
            actual = source_root / relative
        else:
            raise RuntimeError('Unexpected native build input')
        if hashlib.sha256(actual.read_bytes()).hexdigest() != expected:
            raise RuntimeError("Native training module changed after its build")
    return manifest


def load_owned_module(process, context, stage=1):
    manifest = verified_build()
    module = ROOT / ".local/native-module/trainer_bridge.dll"
    return load_verified_owned_library(process, context, module,
        manifest["sha256"][".local/native-module/trainer_bridge.dll"],
        ROOT / '.local/native-module/bridge-state.json', stage)


def load_verified_owned_library(process, context, module, expected_sha256, marker, stage=1):
    """Shared loader for a caller-verified project library and its owned child."""
    module, marker = Path(module).resolve(strict=True), Path(marker).resolve()
    local = (ROOT / '.local').resolve()
    if local not in module.parents or local not in marker.parents:
        raise RuntimeError('Owned libraries and markers must stay inside project .local')
    if hashlib.sha256(module.read_bytes()).hexdigest() != expected_sha256:
        raise RuntimeError('Owned library differs from its verified build')
    if not all(isinstance(context.get(key), int) and context[key] > 0x10000 for key in ("image", "world", "controller", "pawn")):
        raise RuntimeError("Training module requires the verified local world and Pawn context")
    (module.parent / f"context-{process.pid}.bin").write_bytes(struct.pack("<8Q",
        0x3152544c41564f4c, 1, process.pid, context["image"], context["world"],
        context["controller"], context["pawn"], stage))
    api = ctypes.WinDLL("kernel32", use_last_error=True)
    handle = int(process._handle)
    api.GetModuleHandleW.argtypes = [wintypes.LPCWSTR]
    api.GetModuleHandleW.restype = wintypes.HMODULE
    api.GetProcAddress.argtypes = [wintypes.HMODULE, ctypes.c_char_p]
    api.GetProcAddress.restype = ctypes.c_void_p
    api.GetModuleHandleExW.argtypes = [wintypes.DWORD, ctypes.c_void_p, ctypes.POINTER(wintypes.HMODULE)]
    api.GetModuleFileNameW.argtypes = [wintypes.HMODULE, wintypes.LPWSTR, wintypes.DWORD]
    address = api.GetProcAddress(api.GetModuleHandleW("kernel32.dll"), b"LoadLibraryW")
    owner = wintypes.HMODULE()
    if not address or not api.GetModuleHandleExW(6, address, ctypes.byref(owner)):
        raise RuntimeError("Could not resolve the standard module loader")
    filename = ctypes.create_unicode_buffer(32768)
    if not api.GetModuleFileNameW(owner, filename, len(filename)):
        raise RuntimeError("Could not identify the standard loader module")
    owner_name = Path(filename.value).name.casefold()
    if owner_name not in ("kernel32.dll", "kernelbase.dll"):
        raise RuntimeError("Standard loader resolved outside a system loader module")

    class ModuleEntry(ctypes.Structure):
        _fields_ = [("size", wintypes.DWORD), ("id", wintypes.DWORD), ("pid", wintypes.DWORD),
                    ("global_count", wintypes.DWORD), ("process_count", wintypes.DWORD),
                    ("base", ctypes.c_void_p), ("bytes", wintypes.DWORD), ("module", wintypes.HMODULE),
                    ("name", wintypes.WCHAR * 256), ("path", wintypes.WCHAR * 260)]
    api.CreateToolhelp32Snapshot.argtypes = [wintypes.DWORD, wintypes.DWORD]
    api.CreateToolhelp32Snapshot.restype = wintypes.HANDLE
    api.Module32FirstW.argtypes = [wintypes.HANDLE, ctypes.POINTER(ModuleEntry)]
    api.Module32NextW.argtypes = [wintypes.HANDLE, ctypes.POINTER(ModuleEntry)]
    api.CloseHandle.argtypes = [wintypes.HANDLE]
    def modules():
        snapshot = api.CreateToolhelp32Snapshot(8, process.pid)
        if snapshot == wintypes.HANDLE(-1).value:
            raise RuntimeError("Owned child module enumeration failed")
        result = {}
        try:
            entry = ModuleEntry()
            entry.size = ctypes.sizeof(entry)
            found = api.Module32FirstW(snapshot, ctypes.byref(entry))
            while found:
                result[entry.name.casefold()] = (entry.base, entry.bytes, entry.path)
                found = api.Module32NextW(snapshot, ctypes.byref(entry))
            return result
        finally:
            api.CloseHandle(snapshot)
    remote_owner = modules().get(owner_name)
    rva = address - owner.value
    if remote_owner is None or not 0 <= rva < remote_owner[1]:
        raise RuntimeError("Standard loader is not present in the owned child")
    api.VirtualAllocEx.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_size_t, wintypes.DWORD, wintypes.DWORD]
    api.VirtualAllocEx.restype = ctypes.c_void_p
    api.VirtualFreeEx.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_size_t, wintypes.DWORD]
    api.WriteProcessMemory.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
    api.CreateRemoteThread.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p,
                                      ctypes.c_void_p, wintypes.DWORD, ctypes.c_void_p]
    api.CreateRemoteThread.restype = wintypes.HANDLE
    api.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
    api.WaitForSingleObject.restype = wintypes.DWORD
    payload = (str(module) + "\0").encode("utf-16-le")
    remote = api.VirtualAllocEx(handle, None, len(payload), 0x3000, 4)
    if not remote:
        raise RuntimeError("Module path allocation failed")
    thread = None
    completed = False
    try:
        written = ctypes.c_size_t()
        buffer = ctypes.create_string_buffer(payload)
        if not api.WriteProcessMemory(handle, remote, buffer, len(payload), ctypes.byref(written)) or written.value != len(payload):
            raise RuntimeError("Module path write failed")
        thread = api.CreateRemoteThread(handle, None, 0, remote_owner[0] + rva, remote, 0, None)
        if not thread:
            raise RuntimeError("Owned child module-load thread was not created")
        completed = api.WaitForSingleObject(thread, 5000) == 0
        if not completed:
            raise RuntimeError("Owned child module loading did not finish in time")
        state = None
        for _ in range(20):
            if marker.is_file():
                try:
                    candidate = json.loads(marker.read_text(encoding="utf-8"))
                    if candidate.get("pid") == process.pid:
                        state = candidate
                        break
                except (OSError, ValueError):
                    pass
            time.sleep(.05)
        loaded = modules().get(module.name.casefold())
        if loaded is None or Path(loaded[2]).resolve() != module.resolve() or not state:
            raise RuntimeError("The training module did not establish its owned-process marker")
        return {"module": str(module), "sha256": expected_sha256,
                "state": state, "instruction_bytes_modified": False}
    finally:
        if thread:
            api.CloseHandle(thread)
        # A timed-out loader may still refer to the path. Its child is terminated
        # by the wrapper; do not free a live thread's argument prematurely.
        if not thread or completed:
            api.VirtualFreeEx(handle, remote, 0, 0x8000)
