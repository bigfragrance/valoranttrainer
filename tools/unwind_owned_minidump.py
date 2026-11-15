"""Offline x64 StackWalk64 over a committed owned minidump.

All memory reads come from dump ranges or version-matched, raw-backed local PE
sections. No process is opened, no target code is executed, and no symbol server
or symbol engine is used. Missing protected/runtime-only unwind data stops the
walk; stack qwords are never scanned and presented as a call chain.
"""
from __future__ import annotations

import argparse
import bisect
import ctypes as C
from ctypes import wintypes as W
import hashlib
import json
import os
from pathlib import Path
import struct

import pefile
from capture_owned_thread import Address64, KdHelp64, StackFrame64


class OfflineDump:
    def __init__(self, path: Path):
        self.path = path
        self.raw = path.read_bytes()
        signature, version, count, directory, _, stamp, flags = struct.unpack_from("<IIIIIIQ", self.raw)
        if signature != 0x504D444D or not 0 < count <= 64:
            raise ValueError("invalid minidump header")
        self.streams = {kind: (size, rva) for kind, size, rva in
                        struct.iter_unpack("<III", self.raw[directory:directory + count * 12]) if kind}
        self.flags = flags
        self.modules = []
        _, rva = self.streams[4]
        for index in range(self.u32(rva)):
            offset = rva + 4 + index * 108
            base, size, checksum, timestamp, name_rva = struct.unpack_from("<QIIII", self.raw, offset)
            n = self.u32(name_rva)
            path_text = self.raw[name_rva + 4:name_rva + 4 + n].decode("utf-16-le")
            self.modules.append({"name": path_text.replace("\\", "/").rsplit("/", 1)[-1],
                                 "path": path_text, "base": base, "bytes": size,
                                 "timestamp": timestamp})
        self.memory = []
        _, rva = self.streams[5]
        for index in range(self.u32(rva)):
            address, size, offset = struct.unpack_from("<QII", self.raw, rva + 4 + index * 16)
            if offset + size <= len(self.raw):
                self.memory.append((address, address + size, offset))
        self.memory.sort()
        self.starts = [x[0] for x in self.memory]
        self.threads = {}
        _, rva = self.streams[3]
        for index in range(self.u32(rva)):
            offset = rva + 4 + index * 48
            tid, suspend, pc, priority, teb, stack, size, srva, cs, crva = struct.unpack_from("<IIIIQQIIII", self.raw, offset)
            self.threads[tid] = {"thread_id": tid, "recorded_suspend_count": suspend,
                                 "stack_start": stack, "stack_bytes": size,
                                 "context": self.raw[crva:crva + cs]}
        self.local = {}
        self.failures = []
        self.function_buffers = {}

    def u32(self, offset):
        return struct.unpack_from("<I", self.raw, offset)[0]

    def module(self, address):
        return next((m for m in self.modules if m["base"] <= address < m["base"] + m["bytes"]), None)

    def pe_for(self, module):
        base = module["base"]
        if base in self.local:
            return self.local[base]
        candidates = [Path(module["path"])]
        if module["name"].lower() == "valorant-win64-shipping.exe":
            candidates.insert(0, Path("work/client-analysis/Shipping-analysis-copy.exe"))
        if module["name"].lower() == "stub.dll":
            candidates.insert(0, Path("work/duel1-partial-pkm/stub-analysis-copy.dll"))
        for path in candidates:
            if not path.is_file():
                continue
            try:
                image = path.read_bytes()
                pe = pefile.PE(data=image, fast_load=True)
                if pe.FILE_HEADER.TimeDateStamp != module["timestamp"] or pe.OPTIONAL_HEADER.SizeOfImage != module["bytes"]:
                    continue
                result = {"pe": pe, "image": image, "name": path.name,
                          "functions": [], "starts": [], "exports": []}
                self.local[base] = result
                directory = pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
                data = self.pe_read(module, directory.VirtualAddress, directory.Size)
                if data is not None and len(data) % 12 == 0:
                    result["functions"] = [(a, b, u) for a, b, u in struct.iter_unpack("<III", data)
                                           if 0 <= a < b <= module["bytes"] and 0 < u < module["bytes"]]
                    result["starts"] = [x[0] for x in result["functions"]]
                pe.parse_data_directories(directories=[0])
                result["exports"] = sorted((x.address, x.name.decode("ascii", errors="replace")) for x in
                                           getattr(pe, "DIRECTORY_ENTRY_EXPORT", type("E", (), {"symbols": []})()).symbols
                                           if x.name and not x.forwarder)
                return result
            except (OSError, pefile.PEFormatError, struct.error):
                continue
        self.local[base] = None
        return None

    def pe_read(self, module, rva, size):
        cached = self.local.get(module["base"])
        if not cached:
            return None
        pe, image = cached["pe"], cached["image"]
        if 0 <= rva and rva + size <= pe.OPTIONAL_HEADER.SizeOfHeaders:
            return image[rva:rva + size]
        for section in pe.sections:
            relative = rva - section.VirtualAddress
            # Never use pefile's RVA fallback for virtual-only protected sections.
            if 0 <= relative and relative + size <= section.SizeOfRawData:
                begin = section.PointerToRawData + relative
                if begin + size <= len(image):
                    return image[begin:begin + size]
        return None

    def read(self, address, size):
        index = bisect.bisect_right(self.starts, address) - 1
        if index >= 0:
            begin, end, rva = self.memory[index]
            if begin <= address and address + size <= end:
                offset = rva + address - begin
                return self.raw[offset:offset + size]
        module = self.module(address)
        if module:
            self.pe_for(module)
            data = self.pe_read(module, address - module["base"], size)
            if data is not None:
                return data
        if len(self.failures) < 32:
            self.failures.append({"address": hex(address), "bytes": size,
                                  "module": module["name"] if module else None})
        return None

    def function(self, address):
        module = self.module(address)
        local = self.pe_for(module) if module else None
        if not local:
            return None
        rva = address - module["base"]
        index = bisect.bisect_right(local["starts"], rva) - 1
        if index >= 0:
            entry = local["functions"][index]
            if entry[0] <= rva < entry[1]:
                return entry
        return None

    def describe(self, address):
        module = self.module(address)
        result = {"address": hex(address)}
        if not module:
            return result
        rva = address - module["base"]
        result.update(module=module["name"], rva=hex(rva))
        local = self.pe_for(module)
        function = self.function(address)
        if function:
            result["runtime_function"] = {"begin": hex(function[0]), "end": hex(function[1]),
                                          "unwind_rva": hex(function[2])}
        if local:
            nearest = [(a, n) for a, n in local["exports"] if a <= rva]
            if nearest:
                a, n = nearest[-1]
                # Export names are accepted only within the same runtime function,
                # or an exact tiny leaf/syscall range. Other frames retain RVAs.
                if (function and function[0] <= a < function[1]) or rva - a <= 0x20:
                    result["export"] = n
                    result["export_offset"] = hex(rva - a)
        return result


def walk(dump, thread_id, max_frames=64):
    thread = dump.threads[thread_id]
    context_raw = thread["context"]
    storage = C.create_string_buffer(len(context_raw) + 16)
    context_address = (C.addressof(storage) + 15) & ~15
    C.memmove(context_address, context_raw, len(context_raw))
    rsp, rbp, rip = (struct.unpack_from("<Q", context_raw, x)[0] for x in (152, 160, 248))
    dbg = C.WinDLL(str(Path(os.environ["SystemRoot"]) / "System32/dbghelp.dll"), use_last_error=True)
    reader_type = C.WINFUNCTYPE(W.BOOL, W.HANDLE, C.c_uint64, C.c_void_p, W.DWORD, C.POINTER(W.DWORD))
    function_type = C.WINFUNCTYPE(C.c_void_p, W.HANDLE, C.c_uint64)
    module_type = C.WINFUNCTYPE(C.c_uint64, W.HANDLE, C.c_uint64)

    @reader_type
    def reader(_process, address, destination, count, actual):
        try:
            data = dump.read(address, count)
            if data is None:
                actual[0] = 0
                return False
            C.memmove(destination, data, count)
            actual[0] = count
            return True
        except Exception:
            actual[0] = 0
            return False

    @function_type
    def function(_process, address):
        entry = dump.function(address)
        if entry is None:
            return 0
        key = (dump.module(address)["base"], entry[0])
        if key not in dump.function_buffers:
            dump.function_buffers[key] = C.create_string_buffer(struct.pack("<III", *entry))
        return C.addressof(dump.function_buffers[key])

    @module_type
    def module(_process, address):
        found = dump.module(address)
        return found["base"] if found else 0

    dbg.StackWalk64.argtypes = [W.DWORD, W.HANDLE, W.HANDLE, C.POINTER(StackFrame64), C.c_void_p,
                               reader_type, function_type, module_type, C.c_void_p]
    dbg.StackWalk64.restype = W.BOOL
    frame = StackFrame64()
    frame.pc.offset, frame.stack.offset, frame.frame.offset = rip, rsp, rbp
    frame.pc.mode = frame.stack.mode = frame.frame.mode = 3
    frames = [{"index": 0, "rsp": hex(rsp), **dump.describe(rip)}]
    seen = {(rip, rsp)}
    reason = "frame_limit"
    for index in range(max_frames):
        current = frame.pc.offset
        local_module = dump.module(current)
        if local_module is None:
            reason = "current_pc_outside_captured_module_ranges"
            break
        local = dump.pe_for(local_module)
        if local is None:
            reason = "matching_local_pe_unavailable"
            break
        # Absence of .pdata in a virtual-only local module is not a leaf frame.
        if not local["functions"]:
            description = dump.describe(current)
            syscall_leaf = (description.get("export", "").startswith(("Nt", "Zw")) and
                            dump.pe_read(local_module, current - local_module["base"], 1) is not None)
            if not syscall_leaf:
                reason = "module_runtime_unwind_metadata_unavailable"
                break
        if not dbg.StackWalk64(0x8664, W.HANDLE(0xD00F), W.HANDLE(thread_id), C.byref(frame),
                               context_address, reader, function, module, None):
            reason = "StackWalk64_stopped_or_required_dump_memory_missing"
            break
        pair = (frame.pc.offset, frame.stack.offset)
        if pair in seen:
            if index == 0 and pair == (rip, rsp):
                continue
            reason = "repeated_frame"
            break
        if not pair[0]:
            reason = "zero_return_address"
            break
        if pair[1] < rsp or pair[1] > thread["stack_start"] + thread["stack_bytes"]:
            reason = "unwind_rsp_outside_saved_stack"
            break
        seen.add(pair)
        frames.append({"index": len(frames), "rsp": hex(pair[1]),
                       "transition": "local_system_StackWalk64", **dump.describe(pair[0])})
    return {"thread_id": thread_id, "engine": "offline_local_system_StackWalk64",
            "symbol_engine_used": False, "symbol_server_used": False,
            "target_process_opened": False, "raw_address_scan_used": False,
            "recorded_suspend_count": thread["recorded_suspend_count"],
            "frames": frames, "stop_reason": reason}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dump", type=Path)
    parser.add_argument("--threads", type=int, nargs="+", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    image = OfflineDump(args.dump)
    result = {"schema_version": 1, "dump": str(args.dump.resolve()),
              "sha256": hashlib.sha256(image.raw).hexdigest(),
              "bytes": len(image.raw), "flags": hex(image.flags),
              "exception_stream_present": 6 in image.streams,
              "thread_count": len(image.threads), "module_count": len(image.modules),
              "walks": [walk(image, tid) for tid in args.threads if tid in image.threads],
              "missing_memory_reads": image.failures}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({"output": str(args.output.resolve()),
                      "walks": [{"thread_id": x["thread_id"], "frames": len(x["frames"]),
                                 "stop_reason": x["stop_reason"]} for x in result["walks"]]}))
