"""Recover bounded standard metadata from an unfinished owned PKM minidump.

Never repairs or writes the dump, never scans its full memory payload. Only a
1 MiB prefix is read, plus at most 16 KiB per selected, structurally mapped
thread stack. Reports contain module identities/control registers, no raw memory.
The zero-directory recovery order is explicitly validated rather than guessed.
"""
from __future__ import annotations

import argparse
import datetime as dt
import json
import struct
from pathlib import Path


PREFIX_LIMIT = 1024 * 1024


def utc_epoch(seconds: float) -> str:
    return dt.datetime.fromtimestamp(seconds, dt.timezone.utc).isoformat()


def utc_filetime(value: int) -> str | None:
    return utc_epoch((value - 116444736000000000) / 10_000_000) if value else None


def recover(dump: Path, owned_snapshot: Path) -> dict:
    owned = json.loads(owned_snapshot.read_text(encoding="utf-8"))
    before = dump.stat()
    with dump.open("rb") as stream:
        raw = stream.read(PREFIX_LIMIT)
    report = {"schema_version": 1, "read_only": True,
              "dump": str(dump.resolve()), "file_bytes": before.st_size,
              "prefix_bytes_read": len(raw), "memory_payload_scanned": False,
              "directory_repaired": False, "errors": []}

    def require(condition: bool, message: str) -> None:
        if not condition:
            raise ValueError(message)

    def u32(offset: int) -> int:
        return struct.unpack_from("<I", raw, offset)[0]

    def u64(offset: int) -> int:
        return struct.unpack_from("<Q", raw, offset)[0]

    def module_name(offset: int) -> str:
        require(0 < offset < len(raw) - 4, "module string RVA outside bounded prefix")
        size = u32(offset)
        require(0 < size <= 2048 and not size % 2 and offset + 4 + size <= len(raw),
                "invalid module name size")
        # Deliberately omit directory/account paths from diagnostic output.
        return raw[offset + 4:offset + 4 + size].decode("utf-16-le").replace("\\", "/").rsplit("/", 1)[-1]

    try:
        signature, version, count, directory, checksum, stamp, flags = struct.unpack_from("<IIIIIIQ", raw)
        require(signature == 0x504D444D and 0 < count <= 64 and directory == 32,
                "unsupported minidump header")
        require(not any(raw[directory:directory + count * 12]),
                "directory is populated; use a normal minidump parser")
        report["header"] = {"streams_declared": count, "directory_rva": directory,
                            "directory_all_zero": True, "flags": hex(flags),
                            "timestamp_utc": utc_epoch(stamp)}
        cursor = directory + count * 12
        require(u32(cursor) & 0xFFFF == 9 and u32(cursor + 8) == 10,
                "expected AMD64 Windows system info at payload start")
        report["system_info"] = {"rva": cursor, "bytes": 56,
                                 "os_build": u32(cursor + 16)}
        cursor += 56
        misc_size, misc_flags, pid, created = struct.unpack_from("<IIII", raw, cursor)
        require(misc_size in (24, 44, 232, 832, 1364), "unsupported misc-info size")
        require(misc_flags & 1 and pid == owned["pid"], "dump PID does not match owned archive")
        owned_created = (owned["creation_filetime"] - 116444736000000000) / 10_000_000
        require(abs(created - owned_created) < 1, "dump process creation differs from owned archive")
        report["owned_identity"] = {"pid": pid, "process_created_utc": utc_epoch(created),
                                    "pid_and_creation_match": True}
        report["misc_info"] = {"rva": cursor, "bytes": misc_size}
        cursor += misc_size
        exception_rva = cursor
        tid, align, code, exc_flags, nested, address, parameters, pad = struct.unpack_from("<IIIIQQII", raw, cursor)
        context_size, context_rva = struct.unpack_from("<II", raw, cursor + 160)
        require(tid and not tid % 4 and not align and parameters <= 15 and not pad and
                context_size == 0x4D0 and context_rva + context_size <= len(raw),
                "invalid recovered exception stream")
        report["exception"] = {"stream_rva": cursor, "stream_bytes": 168,
                               "thread_id": tid, "code": hex(code), "flags": hex(exc_flags),
                               "address": address, "parameter_count": parameters,
                               "context_size": context_size, "context_rva": context_rva}
        cursor += 168
        thread_list_rva = cursor
        thread_count = u32(cursor)
        require(0 < thread_count <= 2048, "invalid thread count")
        cursor += 4
        threads = []
        for index in range(thread_count):
            offset = cursor + 48 * index
            values = struct.unpack_from("<IIIIQQIIII", raw, offset)
            thread, suspend, _, _, teb, stack_va, stack_size, stack_rva, cs, cr = values
            require(thread and not thread % 4 and cs == 0x4D0 and cr + cs <= len(raw),
                    "invalid recovered thread/context descriptor")
            cf = u32(cr + 0x30)
            require(cf & 0x100000, "context is not AMD64")
            threads.append({"thread_id": thread, "entry_rva": offset,
                            "recorded_suspend_count": suspend,
                            "stack_start": stack_va, "stack_bytes": stack_size,
                            "stack_rva": stack_rva, "context_rva": cr,
                            "context_flags": hex(cf), "rsp": u64(cr + 0x98),
                            "rip": u64(cr + 0xF8)})
        cursor += 48 * thread_count
        report["thread_list"] = {"rva": thread_list_rva, "count": thread_count,
                                 "contexts_validated": len(threads),
                                 "nonzero_stack_rva_count": sum(bool(t["stack_rva"]) for t in threads)}
        h, entry, n = struct.unpack_from("<III", raw, cursor)
        require((h, entry, n) == (12, 64, thread_count), "unexpected thread-info header")
        info = {}
        for index in range(n):
            values = struct.unpack_from("<IIIIQQQQQQ", raw, cursor + h + entry * index)
            th, fl, error, status, ct, et, kernel, user, start, affinity = values
            info[th] = {"dump_flags": hex(fl), "writing_thread": bool(fl & 2),
                        "dump_error": hex(error), "exit_status": hex(status),
                        "created_utc": utc_filetime(ct), "start_address": start}
        report["thread_info"] = {"rva": cursor, "entry_bytes": entry, "count": n}
        cursor += h + entry * n
        module_rva = cursor
        module_count = u32(cursor)
        require(0 < module_count <= 1024, "invalid recovered module count")
        cursor += 4
        modules = []
        for index in range(module_count):
            offset = cursor + 108 * index
            base, size, check, timestamp, name = struct.unpack_from("<QIIII", raw, offset)
            require(0x10000 <= base < 0x800000000000 and 0 < size < 0x40000000,
                    "invalid recovered module range")
            modules.append({"name": module_name(name), "base": base, "bytes": size,
                            "timestamp": timestamp, "entry_rva": offset})
        report["module_list"] = {"rva": module_rva, "count": module_count,
                                 "modules": modules}

        def owner(virtual_address: int) -> dict:
            for mod in modules:
                if mod["base"] <= virtual_address < mod["base"] + mod["bytes"]:
                    return {"module": mod["name"], "rva": hex(virtual_address - mod["base"])}
            return {"module": None}

        expected_trainer = owned["project_module"]
        trainer = next((m for m in modules if m["name"].lower() == "trainer_bridge.dll"), None)
        require(trainer is not None and trainer["base"] == expected_trainer["base"] and
                trainer["bytes"] == expected_trainer["bytes"], "trainer module differs from owned archive")
        report["owned_identity"]["trainer_base_and_size_match"] = True
        report["exception"]["address_owner"] = owner(address)
        report["exception"]["context_rip_owner"] = owner(u64(context_rva + 0xF8))
        report["exception"]["context_rsp"] = u64(context_rva + 0x98)
        selected = []
        for thread in threads:
            if thread["thread_id"] not in (tid, owned["game_thread"]):
                continue
            row = {**thread, "rip_owner": owner(thread["rip"]),
                   "thread_info": info.get(thread["thread_id"])}
            if row["thread_info"]:
                row["thread_info"]["start_owner"] = owner(row["thread_info"].pop("start_address"))
            if thread["thread_id"] == owned["game_thread"]:
                row["rip_equals_later_stale_snapshot"] = thread["rip"] == owned["registers"]["rip"]
            selected.append(row)
        report["selected_threads"] = selected
        report["exception"]["is_dump_writing_thread"] = info.get(tid, {}).get("writing_thread", False)
        report["exception"]["threadlist_context_equals_exception_context"] = any(
            t["thread_id"] == tid and raw[t["context_rva"]:t["context_rva"] + 0x4D0] ==
            raw[context_rva:context_rva + 0x4D0] for t in threads)
        # Find a Memory64 descriptor stream only within the metadata prefix.
        memory_maps = []
        for offset in range(200, len(raw) - 16, 4):
            ranges, base_rva = struct.unpack_from("<QQ", raw, offset)
            end = offset + 16 + 16 * ranges
            if not (64 <= ranges <= 32768 and end <= len(raw) and
                    end <= base_rva < before.st_size):
                continue
            previous = 0
            descriptors = []
            for index in range(ranges):
                va, size = struct.unpack_from("<QQ", raw, offset + 16 + 16 * index)
                if not (0x10000 <= va < 0x800000000000 and not va % 4096 and
                        0 < size < 0x10000000000 and not size % 4096 and va >= previous):
                    break
                descriptors.append((va, size))
                previous = va + size
            if len(descriptors) == ranges:
                memory_maps.append((offset, base_rva, descriptors))
        report["memory64_recovery"] = {"search_bytes": len(raw), "valid_candidates": len(memory_maps),
                                      "full_memory_payload_scanned": False}
        report["stack_recovery"] = {"status": "not_recoverable_from_available_metadata",
                                    "qualification": "No raw stack scan or guessed call chain is emitted."}
        if len(memory_maps) == 1:
            offset, base_rva, descriptors = memory_maps[0]
            report["memory64_recovery"].update({"stream_rva": offset, "base_rva": base_rva,
                                               "ranges": len(descriptors)})
            stack_candidates = []
            for thread in selected:
                file_offset = base_rva
                for va, size in descriptors:
                    if va <= thread["rsp"] < va + size:
                        begin = file_offset + thread["rsp"] - va
                        amount = min(16384, va + size - thread["rsp"], before.st_size - begin)
                        if amount > 0:
                            with dump.open("rb") as stream:
                                stream.seek(begin)
                                stack = stream.read(amount)
                            candidates = []
                            for at in range(0, len(stack) - 7, 8):
                                target = struct.unpack_from("<Q", stack, at)[0]
                                mapped = owner(target)
                                if mapped["module"]:
                                    candidates.append({"stack_offset": at, **mapped})
                            stack_candidates.append({"thread_id": thread["thread_id"],
                                                     "bytes_read": len(stack),
                                                     "module_pointer_candidates": candidates[:64]})
                        break
                    file_offset += size
            report["stack_recovery"] = {"status": "bounded_mapped_candidates",
                                        "threads": stack_candidates,
                                        "qualification": "Module pointer candidates are not an unwound call chain."}
        report["assessment"] = (
            "The breakpoint record belongs to the MiniDumpWriteDump writing thread. "
            "This supports a dump-generation context; it does not establish the original Critical Error cause. "
            "The GameThread RIP matching a later prior-suspend snapshot supports suspension during dump collection.")
        report["primary_format_reference"] = (
            "https://learn.microsoft.com/en-us/windows/win32/api/minidumpapiset/ns-minidumpapiset-minidump_thread_info")
    except (ValueError, KeyError, struct.error, UnicodeError) as exc:
        report["errors"].append(f"{type(exc).__name__}: {exc}")
    after = dump.stat()
    report["source_unchanged_during_read"] = (before.st_size, before.st_mtime_ns) == (after.st_size, after.st_mtime_ns)
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dump", type=Path)
    parser.add_argument("--owned-snapshot", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = recover(args.dump, args.owned_snapshot)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({"output": str(args.output.resolve()), "errors": result["errors"],
                      "exception": result.get("exception"),
                      "thread_count": result.get("thread_list", {}).get("count"),
                      "module_count": result.get("module_list", {}).get("count"),
                      "memory64": result.get("memory64_recovery")}, ensure_ascii=False))
