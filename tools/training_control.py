"""Version 2 local commands/state; never opens or creates another client's channel."""
from __future__ import annotations

import ctypes
import json
import math
import os
import struct
import threading
import time

MAGIC = 0x32304C5443525456
VERSION, BYTES = 2, 131072
COMMAND_OFFSET, COMMAND_CAPACITY = 256, 16384
STATE_OFFSET, STATE_CAPACITY = 32768, BYTES - 32768
COMMANDS = {"configure": 1, "routes": 2, "weapon": 3, "skin": 4,
            "effect": 5, "restart_match": 6, "validate_routes": 7, "toggle_buy_menu": 8, "buy_weapon": 9,
            "save_skin_selection": 10, "validate_skin_catalog": 11, "resolve_map_point": 12,
            "toggle_options": 13, "close_active_ui": 14, "test_player_near_bot": 15,
            "test_audio_registration": 16, "human_bots": 17, "spectate_bot": 18}
RESULTS = {0: "applied", 1: "pending", 2: "invalid", 3: "unavailable", 4: "failed"}
STYLES = ("jump", "wide", "short", "crouch", "peek", "random")
ROUTE_STYLES = (*STYLES, "global", "wide_crouch", "short_crouch")
CONFIG = struct.Struct("<4I12f2I")
ROUTE = struct.Struct("<3If9f")


def guid_words(value) -> tuple[int, int, int, int]:
    """Catalog IDs are original FGuid words, not RFC UUID byte order."""
    if isinstance(value, (list, tuple)) and len(value) == 4:
        if any(isinstance(x, bool) or not isinstance(x, int) or not 0 <= x <= 0xffffffff for x in value):
            raise ValueError("Invalid GUID words")
        return tuple(value)
    if not isinstance(value, str):
        raise ValueError("Invalid GUID")
    compact = value.replace("-", "").lower()
    if len(compact) != 32 or any(c not in "0123456789abcdef" for c in compact):
        raise ValueError("Invalid GUID")
    return tuple(int(compact[i:i+8], 16) for i in range(0, 32, 8))


def pack_configuration(config: dict) -> bytes:
    practice, bots = config["practice"], config["bots"]
    flags = ((1 if practice["return_fire"] else 0) | (2 if config["invincible"] else 0)
             | (4 if config["infinite_ammo"] else 0) | (8 if config["paused"] else 0))
    return CONFIG.pack(config["revision"], int(config["mode"] == "deathmatch"), config["total_players"], flags,
                       practice["peeks_per_minute"], config["respawn_seconds"], config["streak_seconds"],
                       bots["reaction_ms"], bots["reaction_spread_ms"], bots["aim_speed"], bots["aim_spread"],
                       bots["accuracy"], bots["accuracy_spread"], bots["movement"], bots["movement_spread"],
                       0.0, STYLES.index(practice["style"]), config["seed"])


def pack_routes(routes: list[dict], revision: int, practice=None) -> bytes:
    if len(routes) > 128:
        raise ValueError("At most 128 routes")
    result = bytearray(struct.pack("<2I", len(routes), revision))
    for row in routes:
        points = [float(v) for key in ("start", "expose", "retreat") for v in row[key]]
        if len(points) != 9 or not all(math.isfinite(v) and abs(v) <= 1e7 for v in points):
            raise ValueError("Invalid route point")
        result += ROUTE.pack(row["id"], ROUTE_STYLES.index(row["style"]), int(row["enabled"]), row["weight"], *points)
    if practice is not None:
        if len(routes) > 64:
            raise ValueError('Practice plans support at most 64 routes')
        # The upper half of the fixed RouteSet was reserved by the 64-route
        # controller limit. Store a tagged plan there without changing Initial.
        result += bytes(8 + 128 * ROUTE.size - len(result))
        entries = []
        waves = practice.get('waves', [])
        if isinstance(waves, dict):
            raise ValueError('Select the current map waves before packing')
        if practice.get('mode') == 'waves':
            for wave_index, wave in enumerate(waves):
                for slot, enemy in enumerate(wave):
                    entries.append((wave_index, slot, enemy['route_id'], enemy))
        else:
            entries = [(0, 0, r['id'], r) for r in routes]
        if len(entries) > 64:
            raise ValueError('At most 64 practice entries')
        offset = 8 + 64 * ROUTE.size
        struct.pack_into('<4IfI', result, offset, 0x504c414e, 1,
                         int(practice.get('mode') == 'waves'), len(entries), practice.get('minimum_interval', 1), practice.get('restart_serial', 0))
        for n, (wave, slot, route_id, enemy) in enumerate(entries):
            struct.pack_into('<5I2f', result, offset + 24 + n * 28,
                             wave, slot, route_id, ROUTE_STYLES.index(enemy['style']),
                             int(enemy.get('fire', practice.get('return_fire', False))),
                             enemy.get('delay_min', .1), enemy.get('delay_max', .4))
    return bytes(result)


def pack_practice_routes(config):
    practice = dict(config['practice'])
    practice['waves'] = practice.get('waves', {}).get(config['map_id'], [])
    return pack_routes(config['routes'].get(config['map_id'], []), config['revision'], practice)


def initial_payload(config: dict, map_info: dict) -> bytes:
    routes = pack_practice_routes(config)
    routes += bytes(8 + 128 * ROUTE.size - len(routes))
    anchors = map_info.get('spawn_anchors') or []
    points, yaws = [], []
    for anchor in anchors:
        if isinstance(anchor, dict) and anchor.get('disabled_by_original'):
            continue
        point = anchor.get('raw_actor_location_cm', anchor.get('position', anchor.get('world_position'))) if isinstance(anchor, dict) else anchor
        if isinstance(point, dict):
            point = [point.get(k) for k in ('x', 'y', 'z')]
        if (isinstance(point, (list, tuple)) and len(point) == 3
                and all(isinstance(v, (int, float)) and math.isfinite(v) and abs(v) < 1e7 for v in point)):
            points.append(point)
            yaws.append(float(anchor.get('yaw_deg', 0)) if isinstance(anchor, dict) else 0.0)
        if len(points) == 64:
            break
    point_payload = b''.join(struct.pack('<3f', *p) for p in points)
    point_payload += bytes(64 * 12 - len(point_payload))
    yaw_payload = b''.join(struct.pack('<f', yaw) for yaw in yaws) + bytes((64 - len(yaws)) * 4)
    body = (pack_configuration(config) + routes + config['map_id'].encode('ascii').ljust(64, b'\0')
            + struct.pack('<I', len(points)) + point_payload + yaw_payload)
    return struct.pack('<Q2I', MAGIC, VERSION, 16 + len(body)) + body


class Channel:
    def __init__(self, pid: int, *, memory=None):
        self.pid = int(pid)
        self._lock = threading.RLock()
        self._closed = False
        self._pending = None
        self._handle = self._view = None
        self._memory = memory
        if memory is None:
            if os.name != "nt":
                raise OSError("The game control channel requires Windows")
            from ctypes import wintypes
            api = ctypes.WinDLL("kernel32", use_last_error=True)
            api.OpenFileMappingW.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.LPCWSTR]
            api.OpenFileMappingW.restype = wintypes.HANDLE
            api.MapViewOfFile.argtypes = [wintypes.HANDLE, wintypes.DWORD, wintypes.DWORD, wintypes.DWORD, ctypes.c_size_t]
            api.MapViewOfFile.restype = ctypes.c_void_p
            api.UnmapViewOfFile.argtypes = [ctypes.c_void_p]
            api.CloseHandle.argtypes = [wintypes.HANDLE]
            self._api = api
            self._handle = api.OpenFileMappingW(6, False, f"Local\\ValorantTrainerControlV2_{self.pid}")
            if not self._handle:
                raise FileNotFoundError("Game control is not ready")
            self._view = api.MapViewOfFile(self._handle, 6, 0, 0, BYTES)
            if not self._view:
                self.close()
                raise OSError("Cannot map the owned game control channel")
        self._validate()
        self._sequence = struct.unpack("<I", self._read(32, 4))[0]

    def _read(self, offset, count):
        if self._closed:
            raise RuntimeError('Game control is closed')
        if self._memory is not None:
            return bytes(self._memory[offset:offset+count])
        return ctypes.string_at(self._view + offset, count)

    def _write(self, offset, data):
        if self._closed:
            raise RuntimeError('Game control is closed')
        if self._memory is not None:
            self._memory[offset:offset+len(data)] = data
        else:
            ctypes.memmove(self._view + offset, data, len(data))

    def _validate(self):
        magic, version, size, pid = struct.unpack("<Q3I", self._read(0, 20))
        if (magic, version, size, pid) != (MAGIC, VERSION, BYTES, self.pid):
            self.close()
            raise RuntimeError("Game control identity/version mismatch")

    def state(self):
        with self._lock:
            return self._state()

    def _state(self):
        self._validate()
        for _ in range(8):
            before, length = struct.unpack("<2I", self._read(56, 8))
            if before & 1:
                continue
            if length >= STATE_CAPACITY:
                raise RuntimeError("Invalid game state length")
            payload = self._read(STATE_OFFSET, length)
            after = struct.unpack("<I", self._read(56, 4))[0]
            if before == after and not after & 1:
                result = json.loads(payload) if payload else {}
                result["heartbeat_ms"] = struct.unpack("<Q", self._read(64, 8))[0]
                result["applied_revision"] = struct.unpack("<I", self._read(76, 4))[0]
                return result
        raise RuntimeError("Game state update is busy")

    def send(self, command: str, payload=b"", timeout=3.0):
        if command not in COMMANDS or len(payload) > COMMAND_CAPACITY:
            raise ValueError("Invalid game command")
        with self._lock:
            self._validate()
            # A timeout does not authorize overwriting an unconsumed command.
            if self._pending is not None:
                ack = struct.unpack("<I", self._read(48, 4))[0]
                if ack != self._pending:
                    raise RuntimeError("The previous game command is still pending")
                self._pending = None
            self._sequence = (self._sequence + 1) & 0xffffffff or 1
            self._write(COMMAND_OFFSET, bytes(payload))
            self._write(36, struct.pack("<2I", COMMANDS[command], len(payload)))
            self._write(32, struct.pack("<I", self._sequence))
            self._pending = self._sequence
            deadline = time.monotonic() + timeout
            while time.monotonic() < deadline:
                ack, code = struct.unpack("<2I", self._read(48, 8))
                if ack == self._sequence:
                    self._pending = None
                    message = self._read(80, 176).split(b"\0", 1)[0].decode("utf-8", "replace")
                    return {"sequence": ack, "status": RESULTS.get(code, "failed"), "code": code,
                            "message": message, "acknowledged": True}
                time.sleep(.01)
            return {"sequence": self._sequence, "status": "pending", "code": 1,
                    "message": "waiting_for_game_thread", "acknowledged": False}

    def close(self):
        with self._lock:
            self._closed = True
            if self._view:
                self._api.UnmapViewOfFile(self._view)
            if self._handle:
                self._api.CloseHandle(self._handle)
            self._view = self._handle = None
