"""Versioned shared-memory channel for the owned VALORANT training module.

No process-memory scanning, remote calls or input injection: the native module
publishes a small explicit protocol, and this client uses only that mapping.
Bytearray-backed construction supports deterministic protocol tests off Windows.
"""
from __future__ import annotations

import ctypes
from dataclasses import dataclass
import math
import os
import struct
import threading
import time

MAGIC = 0x31525456
VERSION = 1
SIZE = 4096
INPUT_FIRE, INPUT_RELOAD, INPUT_ADS, INPUT_FOCUS, INPUT_TAB, INPUT_ESCAPE = 1, 2, 4, 8, 16, 32
INPUT_SMOKE, INPUT_UPDRAFT, INPUT_DASH, INPUT_ULTIMATE = 64, 128, 256, 512
SKILL_BLOCKED, SKILL_INSIDE_SMOKE, SKILL_BURST = 1, 2, 4
RENDER_BOT, RENDER_RELOADING, RENDER_HIT = 1, 2, 4


class ChannelError(RuntimeError):
    pass


class ChannelNotReady(ChannelError):
    pass


class ChannelTimeout(ChannelError):
    pass


@dataclass(frozen=True)
class NativeFrame:
    sequence: int
    time: float
    position: tuple[float, float, float]
    rotation: tuple[float, float, float]
    input_flags: int
    equipment_mode: int = 0
    skill_flags: int = 0
    smoke_count: int = 0
    smoke_cooldown: float = 0.
    updraft_cooldown: float = 0.
    dash_cooldown: float = 0.
    smoke_casts: int = 0
    updraft_casts: int = 0
    dash_casts: int = 0
    ultimate_casts: int = 0
    native_ui_flags: int = 0
    target_bot_id: int = 0
    target_head: tuple[float, float, float] = (0., 0., 0.)


@dataclass(frozen=True)
class TraceResult:
    hit: bool
    distance_cm: float
    impact: tuple[float, float, float]


@dataclass(frozen=True)
class TargetTraceResult:
    hit: bool
    distance_cm: float
    impact: tuple[float, float, float]
    region: int
    bot_id: int


@dataclass(frozen=True)
class RenderState:
    flags: int = 0
    bot_position: tuple[float, float, float] = (0., 0., 0.)
    bot_yaw: float = 0.
    recoil_pitch: float = 0.
    recoil_yaw: float = 0.
    kick: float = 0.
    shots: int = 0
    ammo: int = 25
    kills: int = 0
    hits: int = 0
    bot_id: int = 0
    health: float = 0.


def _finite_vector(value):
    result = tuple(float(x) for x in value)
    if len(result) != 3 or not all(math.isfinite(x) and abs(x) < 1e9 for x in result):
        raise ValueError("Invalid finite channel vector")
    return result


class ProcessWatch:
    """Wait-only lifetime binding to the explicitly supplied game PID."""
    def __init__(self, pid: int):
        if os.name != "nt":
            raise RuntimeError("The training host requires Windows")
        if pid <= 0:
            raise ValueError("PID must be positive")
        from ctypes import wintypes
        self.api = ctypes.WinDLL("kernel32", use_last_error=True)
        self.api.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
        self.api.OpenProcess.restype = wintypes.HANDLE
        self.api.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
        self.api.WaitForSingleObject.restype = wintypes.DWORD
        self.api.CloseHandle.argtypes = [wintypes.HANDLE]
        self.handle = self.api.OpenProcess(0x00100000, False, pid)  # SYNCHRONIZE only.
        if not self.handle:
            raise ChannelError("Cannot bind the supplied game process lifetime")

    def alive(self):
        result = self.api.WaitForSingleObject(self.handle, 0)
        if result == 0x102:
            return True
        if result == 0:
            return False
        raise ChannelError("Game-process lifetime check failed")

    def close(self):
        if self.handle:
            self.api.CloseHandle(self.handle)
            self.handle = None


class TrainingChannel:
    def __init__(self, buffer, pid: int, *, clock=time.monotonic, sleep=time.sleep):
        if len(buffer) < SIZE or pid <= 0:
            raise ValueError("Invalid training mapping or PID")
        self.buffer, self.pid = buffer, pid
        self.clock, self.sleep = clock, sleep
        self._request_lock = threading.Lock()
        self._pending = None
        self._mapping_api = self._mapping_handle = self._mapping_address = None
        self.validate_header()

    @classmethod
    def open(cls, pid, *, alive=lambda: True, timeout=20.):
        if os.name != "nt":
            raise RuntimeError("Named training mappings require Windows")
        from ctypes import wintypes
        api = ctypes.WinDLL("kernel32", use_last_error=True)
        api.OpenFileMappingW.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.LPCWSTR]
        api.OpenFileMappingW.restype = wintypes.HANDLE
        api.MapViewOfFile.argtypes = [wintypes.HANDLE, wintypes.DWORD, wintypes.DWORD, wintypes.DWORD, ctypes.c_size_t]
        api.MapViewOfFile.restype = ctypes.c_void_p
        api.UnmapViewOfFile.argtypes = [ctypes.c_void_p]
        api.CloseHandle.argtypes = [wintypes.HANDLE]
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline and alive():
            handle = api.OpenFileMappingW(0x6, False, f"Local\\ValorantTrainer-{pid}")
            if not handle:
                time.sleep(.05)
                continue
            address = api.MapViewOfFile(handle, 0x6, 0, 0, SIZE)
            if not address:
                api.CloseHandle(handle)
                raise ChannelError("Cannot map the native training channel")
            buffer = memoryview((ctypes.c_ubyte * SIZE).from_address(address)).cast("B")
            try:
                instance = cls(buffer, pid)
            except ChannelNotReady:
                buffer.release()
                api.UnmapViewOfFile(address)
                api.CloseHandle(handle)
                time.sleep(.05)
                continue
            except Exception:
                buffer.release()
                api.UnmapViewOfFile(address)
                api.CloseHandle(handle)
                raise
            instance._mapping_api, instance._mapping_handle, instance._mapping_address = api, handle, address
            return instance
        raise ChannelTimeout("Native training channel was not ready before the deadline")

    def validate_header(self):
        magic, version, pid, ready = struct.unpack_from("<4I", self.buffer, 0)
        if not magic or not ready:
            raise ChannelNotReady("Native resources are not ready")
        if magic != MAGIC or version != VERSION or pid != self.pid or ready != 1:
            raise ChannelError("Training channel identity/version mismatch")

    def read_frame(self, attempts=8) -> NativeFrame:
        self.validate_header()
        for _ in range(attempts):
            before = struct.unpack_from("<I", self.buffer, 64)[0]
            if not before or before & 1:
                self.sleep(.0005)
                continue
            raw = bytes(self.buffer[72:216])
            after = struct.unpack_from("<I", self.buffer, 64)[0]
            if before != after or after & 1:
                continue
            values = struct.unpack_from("<7d4I3f5I", raw)
            target_id = struct.unpack_from("<I", raw, 116)[0]  # Mapping +188.
            target_head = _finite_vector(struct.unpack_from("<3d", raw, 120))
            if not math.isfinite(values[0]) or values[0] < 0:
                raise ChannelError("Invalid native frame time")
            if values[8] not in (0, 1) or values[9] & ~7 or values[10] > 8:
                raise ChannelError("Invalid native equipment/skill flags")
            if not all(math.isfinite(v) and 0 <= v <= 3600 for v in values[11:14]):
                raise ChannelError("Invalid native skill cooldown")
            return NativeFrame(after, values[0], _finite_vector(values[1:4]),
                               _finite_vector(values[4:7]), *values[7:],
                               target_bot_id=target_id, target_head=target_head)
        raise ChannelNotReady("No stable native frame is available")

    def write_settings(self, settings, paused=False):
        from training_settings import normalize
        value = normalize(settings)
        before = struct.unpack_from('<I', self.buffer, 640)[0]
        sequence = ((before + 2) & 0xfffffffe) or 2
        struct.pack_into('<I', self.buffer, 640, sequence-1)
        struct.pack_into('<dfII', self.buffer, 648, value['sensitivity'], value['fov'],
                         int(value['enemy_outline']), int(paused))
        struct.pack_into('<I', self.buffer, 640, sequence)

    def trace(self, origin, direction, max_distance_cm=100000., *, timeout=1., alive=lambda: True) -> TraceResult:
        return self._trace(1, origin, direction, max_distance_cm, timeout=timeout, alive=alive)

    def trace_target(self, origin, direction, max_distance_cm=100000., *, timeout=1., alive=lambda: True) -> TargetTraceResult:
        """Query the visible enemy's original PhysicsAsset, with native target identity."""
        return self._trace(2, origin, direction, max_distance_cm, timeout=timeout, alive=alive)

    def _trace(self, opcode, origin, direction, max_distance_cm, *, timeout, alive):
        origin, direction = _finite_vector(origin), _finite_vector(direction)
        length = math.sqrt(sum(v*v for v in direction))
        if length < 1e-9 or not math.isfinite(max_distance_cm) or not 0 < max_distance_cm <= 1e6:
            raise ValueError("Invalid trace direction or range")
        if not 0 < timeout <= 1.:
            raise ValueError("A trace deadline must be positive and at most one second")
        direction = tuple(v/length for v in direction)
        with self._request_lock:
            self.validate_header()
            response = struct.unpack_from("<I", self.buffer, 384)[0]
            request = struct.unpack_from("<I", self.buffer, 256)[0]
            if request != response:
                raise ChannelError("An earlier native request is still in flight")
            sequence = (request + 1) & 0xffffffff or 1
            struct.pack_into("<I7d", self.buffer, 260, opcode, *origin, *direction, max_distance_cm)
            # Payload is fully written before publishing its sequence.
            struct.pack_into("<I", self.buffer, 256, sequence)
            self._pending = sequence
            deadline = self.clock() + timeout
            while self.clock() < deadline:
                if not alive():
                    raise ChannelError("Game process exited during a trace")
                if struct.unpack_from("<I", self.buffer, 384)[0] == sequence:
                    raw = bytes(self.buffer[388:432 if opcode == 2 else 424])
                    if struct.unpack_from("<I", self.buffer, 384)[0] != sequence:
                        continue
                    status, distance, x, y, z = struct.unpack_from("<id3d", raw)
                    region, bot_id = struct.unpack_from("<iI", raw, 36) if opcode == 2 else (0, 0)
                    self._pending = None
                    if status < 0:
                        raise ChannelError(f"Native {'target' if opcode == 2 else 'scene'} trace failed with status {status}")
                    if status == 0:
                        if distance != math.inf:
                            raise ChannelError("A native trace miss must report infinite distance")
                        return (TargetTraceResult(False, math.inf, (0., 0., 0.), 0, 0) if opcode == 2
                                else TraceResult(False, math.inf, (0., 0., 0.)))
                    if status != 1 or not math.isfinite(distance) or not 0 <= distance <= max_distance_cm + .1:
                        raise ChannelError("Invalid native trace response")
                    impact = _finite_vector((x, y, z))
                    if opcode == 2:
                        if region not in (0, 1, 2) or bot_id == 0:
                            raise ChannelError("Invalid native target region or identity")
                        return TargetTraceResult(True, distance, impact, region, bot_id)
                    return TraceResult(True, distance, impact)
                self.sleep(.001)
            raise ChannelTimeout(f"Native {'target' if opcode == 2 else 'scene'} trace exceeded its one-second deadline")

    def write_render(self, state: RenderState):
        self.validate_header()
        position = _finite_vector(state.bot_position)
        floats = (state.bot_yaw, state.recoil_pitch, state.recoil_yaw, state.kick, state.health)
        counters = (state.flags, state.shots, state.ammo, state.kills, state.hits, state.bot_id)
        if not all(math.isfinite(v) for v in floats) or state.health < 0:
            raise ValueError("Invalid render state")
        if not all(isinstance(v, int) and 0 <= v <= 0xffffffff for v in counters):
            raise ValueError("Render counters must be uint32 values")
        payload = struct.pack("<I7d5If", state.flags, *position, state.bot_yaw,
                              state.recoil_pitch, state.recoil_yaw, state.kick,
                              state.shots, state.ammo, state.kills, state.hits, state.bot_id, state.health)
        previous = struct.unpack_from("<I", self.buffer, 512)[0]
        writing = ((previous + 1) | 1) & 0xffffffff
        struct.pack_into("<I", self.buffer, 512, writing)
        self.buffer[516:600] = payload
        struct.pack_into("<I", self.buffer, 512, (writing + 1) & 0xffffffff)

    def close(self):
        if self._mapping_api is not None:
            self.buffer.release()
            self._mapping_api.UnmapViewOfFile(self._mapping_address)
            self._mapping_api.CloseHandle(self._mapping_handle)
            self._mapping_api = self._mapping_handle = self._mapping_address = None
