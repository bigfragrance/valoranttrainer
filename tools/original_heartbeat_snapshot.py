"""Bounded read-only original heartbeat snapshot, using an owned-process reader.

No process is opened, suspended, resumed or written by this module. The caller
must supply an already ownership-validated read_memory(address, size) callback.
Addresses/ABI refer only to Shipping-analysis-copy.exe SHA256 14776335...92f.
"""
from __future__ import annotations

import ctypes
import math
import struct
from typing import Callable


def _qpc() -> int:
    value = ctypes.c_longlong()
    if not ctypes.windll.kernel32.QueryPerformanceCounter(ctypes.byref(value)):
        raise OSError("QueryPerformanceCounter failed")
    return value.value


def snapshot(read_memory: Callable[[int, int], bytes], image_base: int,
             game_thread_id: int, *, max_slots: int = 128,
             qpc_now: int | None = None) -> dict:
    """Return actual native record ages; all reads are bounded, with no injection.

    On Windows the local QPC is comparable to the owned child's QPC. A supplied
    qpc_now must use that same counter, never Unix time/world simulation seconds.
    This live RPM observation is not atomic. Errors and changed map headers are
    reported; missing allocation bits never cause unused slots to be treated live.
    """
    errors: list[str] = []
    result = {"schema_version": 1, "read_only": True, "atomic": False,
              "game_thread_id": int(game_thread_id), "errors": errors,
              "records": [], "game_thread_records": []}
    max_slots = max(1, min(int(max_slots), 512))

    def read(address: int, size: int) -> bytes | None:
        try:
            raw = read_memory(int(address), int(size))
            if raw is None or len(raw) != size:
                raise ValueError("partial read")
            return bytes(raw)
        except Exception as exc:
            errors.append(f"read {address:#x}+{size:#x}: {type(exc).__name__}")
            return None

    def pointer(raw: bytes, offset: int = 0) -> int:
        return struct.unpack_from("<Q", raw, offset)[0]

    def readable_pointer(value: int) -> bool:
        return 0x10000 <= value < 0x800000000000 and value % 8 == 0

    aliases = {}
    for rva in (0xD958AC0, 0xDAD5C08):
        raw = read(image_base + rva, 8)
        aliases[hex(rva)] = pointer(raw) if raw else None
    result["singleton_aliases"] = aliases
    actual = [x for x in aliases.values() if x and readable_pointer(x)]
    if not actual:
        errors.append("no readable native heartbeat singleton")
        return result
    if len(set(actual)) != 1:
        errors.append("native heartbeat singleton aliases disagree")
        return result
    heartbeat = actual[0]
    result["heartbeat"] = heartbeat
    first = read(heartbeat, 0x250)
    if first is None:
        return result
    data = pointer(first, 0x40)
    count, capacity = struct.unpack_from("<ii", first, 0x48)
    num_bits, max_bits = struct.unpack_from("<ii", first, 0x68)
    bit_secondary = pointer(first, 0x60)
    result["map"] = {"data": data, "count": count, "capacity": capacity,
                     "num_bits": num_bits, "max_bits": max_bits,
                     "allocation_secondary": bit_secondary, "stride": 0x50}
    if not (0 <= count <= capacity <= 65536 and
            0 <= num_bits <= max_bits <= 65536 and num_bits <= count):
        errors.append("invalid original sparse-map header")
        return result
    scan_count = min(count, num_bits, max_slots)
    result["map"].update({"slots_scanned": scan_count,
                          "truncated": num_bits > scan_count})
    if scan_count and not readable_pointer(data):
        errors.append("invalid native heartbeat record data pointer")
        return result
    if bit_secondary:
        bits = read(bit_secondary, ((scan_count + 31) // 32) * 4)
    elif scan_count <= 128:
        bits = first[0x50:0x60]
    else:
        bits = None
        errors.append("inline allocation bits exceed original four-word capacity")
    records = read(data, scan_count * 0x50) if scan_count else b""
    second = read(heartbeat, 0x250)
    if bits is None or records is None or second is None:
        return result
    # Compare only map ownership/shape; clock and heartbeat writes may change
    # normally while the game is advancing, without invalidating its container.
    signature = lambda b: b[0x40:0x70]
    stable = signature(first) == signature(second)
    result["map"]["header_unchanged"] = stable
    if not stable:
        errors.append("native heartbeat map changed during bounded snapshot")
        return result
    factor_raw = read(image_base + 0xD747868, 8)
    factor = struct.unpack("<d", factor_raw)[0] if factor_raw else float("nan")
    accumulated, previous, cap_delta = struct.unpack_from("<QQQ", second, 0x218)
    try:
        current = int(qpc_now) if qpc_now is not None else _qpc()
    except Exception as exc:
        errors.append(f"QPC: {type(exc).__name__}")
        current = None
    clock = {"accumulated_cycles": accumulated, "previous_monitor_qpc": previous,
             "max_delta_cycles": cap_delta, "sample_qpc": current,
             "cycle_seconds": factor if math.isfinite(factor) else None}
    seconds = None
    if current is not None and current >= previous and 0 < factor < 1:
        ticks = accumulated + min(current - previous, cap_delta)
        frequency = int(1.0 / factor)
        if frequency > 0:
            whole, remainder = divmod(ticks, frequency)
            seconds = whole + remainder * factor
            clock["native_now_seconds_estimate"] = seconds
    if seconds is None:
        errors.append("native clock could not be estimated consistently")
    result["clock"] = clock
    result["config"] = {"hang_duration": struct.unpack_from("<d", second, 0x1C8)[0],
                        "scaled_hang_duration": struct.unpack_from("<d", second, 0x1D0)[0],
                        "stuck_duration": struct.unpack_from("<d", second, 0x1E8)[0],
                        "selected_stuck_thread_id": struct.unpack_from("<i", second, 0x208)[0],
                        "stuck_delegate": pointer(second, 0x230),
                        "unstuck_delegate": pointer(second, 0x240)}
    for index in range(scan_count):
        word = struct.unpack_from("<I", bits, (index // 32) * 4)[0]
        if not word & (1 << (index % 32)):
            continue
        offset = index * 0x50
        thread_id = struct.unpack_from("<I", records, offset)[0]
        last, reported = struct.unpack_from("<dd", records, offset + 8)
        ignored = struct.unpack_from("<i", records, offset + 0x18)[0]
        duration, stuck_stamp, stuck_duration = struct.unpack_from("<ddd", records, offset + 0x20)
        row = {"slot": index, "thread_id": thread_id,
               "last_heartbeat_seconds": last, "last_reported_seconds": reported,
               "ignore_count": ignored, "hang_duration_seconds": duration,
               "stuck_timestamp_seconds": stuck_stamp,
               "stuck_duration_seconds": stuck_duration}
        finite = all(math.isfinite(x) for x in (last, reported, duration, stuck_stamp, stuck_duration))
        if finite and seconds is not None:
            age = seconds - last
            row["heartbeat_age_seconds_estimate"] = age
            row["record_timeout_comparison"] = bool(ignored == 0 and duration > 0 and
                                                      age > duration and last >= reported)
        else:
            row["invalid_or_clock_unknown"] = True
        result["records"].append(row)
        if thread_id == game_thread_id:
            result["game_thread_records"].append(row)
    lock = read(image_base + 0xDAD9BF0, 0x28)
    if lock:
        result["original_stackwalk_critical_section"] = {
            "address": image_base + 0xDAD9BF0,
            "lock_count": struct.unpack_from("<i", lock, 8)[0],
            "recursion_count": struct.unpack_from("<i", lock, 12)[0],
            "owning_thread": pointer(lock, 0x10)}
    result["qualification"] = ("Record timeout comparison is not the complete native enabled/"
                               "debugger/command-line gate. Live RPM values are non-atomic; "
                               "no thread is resumed or heartbeat refreshed by this helper.")
    return result
