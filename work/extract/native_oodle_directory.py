"""Bounded Windows subprocess adapter for the research-only Oodle worker.

Only the locally built, frozen executable is run. A Job Object is assigned
before any compressed data is sent. Original resources, paths and DLLs are
never accessed by this module; callers provide bytes explicitly. This is not
part of the independent gameplay client or its distributed runtime.
"""
from __future__ import annotations

import ctypes
from ctypes import wintypes
import hashlib
import os
from pathlib import Path
import struct
import subprocess

MAX_SOURCE = 64 * 1024
MAX_BLOCK_SOURCE = 512 * 1024
MAX_EXPANDED = 32 * 1024 * 1024
MAX_WORKER_IMAGE = 8 * 1024 * 1024
WORKER_TIMEOUT_SECONDS = 3
WORKER_MEMORY_LIMIT = 256 * 1024 * 1024
# Filled only after the root has inspected the local source/build manifest.
WORKER_SHA256 = "53459eb46f3281a7250f015fa484e6cc384bd17c9dc734b3bea3cdcb3bc62b1e"
KRAKEN_BLOCK_WORKER_SHA256 = "ff503a95a9831ff0c1d5841e0f345c8d22783325ce03956dab98260ff67d3b89"
MERMAID_WORKER_SHA256 = "6a71d9ee3d86a3b92e78277df480887d758e0f1282e431892170c1a197a5953a"
MERMAID_BLOCK_WORKER_SHA256 = "e2581fce57d51bcb95356d84fe6f8db9c15ce672ef8d6e53dd55d9743ac01190"
LEVIATHAN_BLOCK_WORKER_SHA256 = "da5d569d16456e8b194bd62f78ce1beffbe99a301edcea918bb5e38e0e0e09ea"


class NativeDirectoryError(ValueError):
    """Value-free worker, format or operating-system failure."""


class _BasicLimits(ctypes.Structure):
    _fields_ = [
        ("PerProcessUserTimeLimit", ctypes.c_int64),
        ("PerJobUserTimeLimit", ctypes.c_int64),
        ("LimitFlags", wintypes.DWORD),
        ("MinimumWorkingSetSize", ctypes.c_size_t),
        ("MaximumWorkingSetSize", ctypes.c_size_t),
        ("ActiveProcessLimit", wintypes.DWORD),
        ("Affinity", ctypes.c_size_t),
        ("PriorityClass", wintypes.DWORD),
        ("SchedulingClass", wintypes.DWORD),
    ]


class _IoCounters(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint64) for name in (
        "ReadOperationCount", "WriteOperationCount", "OtherOperationCount",
        "ReadTransferCount", "WriteTransferCount", "OtherTransferCount")]


class _ExtendedLimits(ctypes.Structure):
    _fields_ = [
        ("BasicLimitInformation", _BasicLimits),
        ("IoInfo", _IoCounters),
        ("ProcessMemoryLimit", ctypes.c_size_t),
        ("JobMemoryLimit", ctypes.c_size_t),
        ("PeakProcessMemoryUsed", ctypes.c_size_t),
        ("PeakJobMemoryUsed", ctypes.c_size_t),
    ]


class _WorkerJob:
    def __init__(self):
        self.kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        self.kernel.CreateJobObjectW.argtypes = [ctypes.c_void_p, wintypes.LPCWSTR]
        self.kernel.CreateJobObjectW.restype = wintypes.HANDLE
        self.kernel.SetInformationJobObject.argtypes = [
            wintypes.HANDLE, ctypes.c_int, ctypes.c_void_p, wintypes.DWORD]
        self.kernel.SetInformationJobObject.restype = wintypes.BOOL
        self.kernel.AssignProcessToJobObject.argtypes = [wintypes.HANDLE, wintypes.HANDLE]
        self.kernel.AssignProcessToJobObject.restype = wintypes.BOOL
        self.kernel.TerminateJobObject.argtypes = [wintypes.HANDLE, wintypes.UINT]
        self.kernel.TerminateJobObject.restype = wintypes.BOOL
        self.kernel.CloseHandle.argtypes = [wintypes.HANDLE]
        self.kernel.CloseHandle.restype = wintypes.BOOL
        self.handle = self.kernel.CreateJobObjectW(None, None)
        if not self.handle:
            raise NativeDirectoryError("Worker job creation failed")
        limits = _ExtendedLimits()
        # KILL_ON_JOB_CLOSE | ACTIVE_PROCESS | PROCESS_MEMORY | JOB_MEMORY.
        limits.BasicLimitInformation.LimitFlags = 0x2000 | 0x8 | 0x100 | 0x200
        limits.BasicLimitInformation.ActiveProcessLimit = 1
        limits.ProcessMemoryLimit = WORKER_MEMORY_LIMIT
        limits.JobMemoryLimit = WORKER_MEMORY_LIMIT
        if not self.kernel.SetInformationJobObject(
                self.handle, 9, ctypes.byref(limits), ctypes.sizeof(limits)):
            self.close()
            raise NativeDirectoryError("Worker job limits failed")

    def assign(self, process):
        if not self.kernel.AssignProcessToJobObject(self.handle, int(process._handle)):
            raise NativeDirectoryError("Worker job assignment failed")

    def terminate(self):
        if self.handle:
            self.kernel.TerminateJobObject(self.handle, 1)

    def close(self):
        if self.handle:
            self.kernel.CloseHandle(self.handle)
            self.handle = None


def _verified_worker_path(codec: int = 6, *, data_block: bool = False) -> Path:
    if os.name != "nt" or struct.calcsize("P") != 8:
        raise NativeDirectoryError("Unsupported worker platform")
    if type(data_block) is not bool:
        raise NativeDirectoryError("Invalid worker selection")
    filename = "directory_decode_worker.exe"
    if data_block:
        if codec == 6:
            directory, expected_sha = "native_oodle", KRAKEN_BLOCK_WORKER_SHA256
        elif codec == 10:
            directory, expected_sha = "native_oodle_mermaid", MERMAID_BLOCK_WORKER_SHA256
        elif codec == 12:
            directory, expected_sha = "native_oodle_leviathan", LEVIATHAN_BLOCK_WORKER_SHA256
        else:
            raise NativeDirectoryError("Unsupported data block codec")
        filename = "block_decode_worker.exe"
    elif codec == 6:
        directory, expected_sha = "native_oodle", WORKER_SHA256
    elif codec == 10:
        directory, expected_sha = "native_oodle_mermaid", MERMAID_WORKER_SHA256
    else:
        raise NativeDirectoryError("Unsupported directory codec")
    if not isinstance(expected_sha, str) or len(expected_sha) != 64:
        raise NativeDirectoryError("Worker build is not frozen")
    root = Path(__file__).resolve().parent
    expected = root / directory / filename
    try:
        path = expected.resolve(strict=True)
        if path != expected or not 1 <= path.stat().st_size <= MAX_WORKER_IMAGE:
            raise NativeDirectoryError("Worker image location or size mismatch")
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        if digest != expected_sha:
            raise NativeDirectoryError("Worker image hash mismatch")
    except OSError:
        raise NativeDirectoryError("Worker image unavailable") from None
    return path


def _cleanup_worker(process, job):
    # Close the kill-on-close job first, even when a wait or pipe close fails.
    # A process not assigned to the job still needs the explicit kill below.
    close_error = False
    try:
        job.close()
    except Exception:
        close_error = True
    finally:
        if process is not None:
            try:
                if process.poll() is None:
                    process.kill()
                process.wait(timeout=WORKER_TIMEOUT_SECONDS)
            except Exception:
                # Cleanup cannot replace a value-free decode failure with an
                # exception containing subprocess input/output or OS paths.
                pass
            for pipe in (process.stdin, process.stdout, process.stderr):
                if pipe is not None:
                    try:
                        pipe.close()
                    except Exception:
                        pass
    if close_error:
        raise NativeDirectoryError("Worker cleanup failed") from None


def _decompress_payload(compressed: bytes, expanded_size: int, *, data_block: bool) -> bytes:
    """Run one exact ODI1 frame under a memory/child/time budget.

    A successful ODO1 response must have the exact declared size and no tail.
    stderr, native exception messages and compressed data never enter errors.
    Codec/checksum/block validation additionally belongs to the frozen worker.
    """
    max_source = MAX_BLOCK_SOURCE if data_block else MAX_SOURCE
    if type(compressed) is not bytes or not 1 <= len(compressed) <= max_source:
        raise NativeDirectoryError("Invalid compressed directory size")
    if type(expanded_size) is not int or not 1 <= expanded_size <= MAX_EXPANDED:
        raise NativeDirectoryError("Invalid expanded directory size")
    if len(compressed) < 2:
        raise NativeDirectoryError("Incomplete directory codec header")
    path = _verified_worker_path(compressed[1] & 127, data_block=data_block)
    frame = b"ODI1" + struct.pack("<II", len(compressed), expanded_size) + compressed
    try:
        job = _WorkerJob()
    except OSError:
        raise NativeDirectoryError("Worker job creation failed") from None
    process = None
    try:
        system_root = os.environ.get("SystemRoot")
        if not system_root:
            raise NativeDirectoryError("Worker system environment unavailable")
        process = subprocess.Popen(
            [str(path)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, cwd=str(path.parent),
            env={"SystemRoot": system_root}, shell=False, close_fds=True,
            creationflags=subprocess.CREATE_NO_WINDOW)
        job.assign(process)
        try:
            output, diagnostics = process.communicate(frame, timeout=WORKER_TIMEOUT_SECONDS)
        except subprocess.TimeoutExpired:
            job.terminate()
            process.kill()
            process.communicate(timeout=WORKER_TIMEOUT_SECONDS)
            raise NativeDirectoryError("Worker deadline exceeded") from None
        if process.returncode != 0 or diagnostics:
            raise NativeDirectoryError("Worker decode failed")
        if (len(output) != 8 + expanded_size or output[:4] != b"ODO1" or
                struct.unpack_from("<I", output, 4)[0] != expanded_size):
            raise NativeDirectoryError("Worker output framing mismatch")
        return output[8:]
    except (OSError, subprocess.TimeoutExpired):
        raise NativeDirectoryError("Worker operating-system failure") from None
    finally:
        _cleanup_worker(process, job)


def decompress_directory(compressed: bytes, expanded_size: int) -> bytes:
    """Decode one bounded directory using its pinned codec 6 or 10 worker."""
    return _decompress_payload(compressed, expanded_size, data_block=False)


def decompress_block(compressed: bytes, expanded_size: int) -> bytes:
    """Decode one caller-selected data block using its pinned codec 6/10/12 worker.

    The 128KiB source cap covers the measured 86,472-byte first block payload.
    This function never reads resources or chooses block/file extents.
    """
    return _decompress_payload(compressed, expanded_size, data_block=True)
