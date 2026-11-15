"""Bounded read-only configuration observation for the wrapper-owned game process."""
from __future__ import annotations

import ctypes
from ctypes import wintypes
import hashlib
import struct


def read_fstring(reader, address, max_chars=32768):
    before = reader(address, 16)
    pointer, count, capacity = struct.unpack("<Qii", before)
    if not 0 <= count <= max_chars or not count <= capacity <= 65536 or (count and not pointer):
        raise ValueError("Invalid bounded FString header")
    payload = reader(pointer, count * 2) if count else b""
    if reader(address, 16) != before:
        raise ValueError("FString changed during observation")
    text = payload.decode("utf-16-le", errors="strict")
    if text and not text.endswith("\0"):
        raise ValueError("FString is not NUL terminated")
    text = text[:-1] if text else text
    if "\0" in text:
        raise ValueError("FString contains embedded NUL")
    return text


class ConfigObserver:
    """Only reads an existing Popen-owned handle; it never opens another process."""
    def __init__(self, process, image_name):
        self.process = process
        self.image_name = image_name.casefold()
        self.api = ctypes.WinDLL("kernel32", use_last_error=True)
        self.api.ReadProcessMemory.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p,
                                             ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
        self.api.ReadProcessMemory.restype = wintypes.BOOL
        self.api.CreateToolhelp32Snapshot.argtypes = [wintypes.DWORD, wintypes.DWORD]
        self.api.CreateToolhelp32Snapshot.restype = wintypes.HANDLE
        self.api.CloseHandle.argtypes = [wintypes.HANDLE]
        self.base = None
        self.image_bytes = None
        self._name_key = None

    def find_main_image(self):
        class ModuleEntry(ctypes.Structure):
            _fields_ = [("size", wintypes.DWORD), ("module_id", wintypes.DWORD),
                        ("pid", wintypes.DWORD), ("global_usage", wintypes.DWORD),
                        ("process_usage", wintypes.DWORD), ("base", ctypes.c_void_p),
                        ("image_bytes", wintypes.DWORD), ("module", wintypes.HMODULE),
                        ("name", wintypes.WCHAR * 256), ("path", wintypes.WCHAR * 260)]

        self.api.Module32FirstW.argtypes = [wintypes.HANDLE, ctypes.POINTER(ModuleEntry)]
        self.api.Module32NextW.argtypes = [wintypes.HANDLE, ctypes.POINTER(ModuleEntry)]
        snapshot = self.api.CreateToolhelp32Snapshot(8, self.process.pid)
        if snapshot == wintypes.HANDLE(-1).value:
            raise OSError(ctypes.get_last_error(), "Main image enumeration unavailable")
        try:
            entry = ModuleEntry()
            entry.size = ctypes.sizeof(entry)
            found = self.api.Module32FirstW(snapshot, ctypes.byref(entry))
            while found:
                if entry.name.casefold() == self.image_name:
                    self.base, self.image_bytes = entry.base, entry.image_bytes
                    return
                found = self.api.Module32NextW(snapshot, ctypes.byref(entry))
            raise ValueError("Main image not present in module snapshot")
        finally:
            self.api.CloseHandle(snapshot)

    def read(self, address, size):
        if not 0 <= size <= 65536:
            raise ValueError("Read exceeds configuration observation budget")
        output = ctypes.create_string_buffer(size)
        actual = ctypes.c_size_t()
        if not self.api.ReadProcessMemory(int(self.process._handle), address, output, size, ctypes.byref(actual)) or actual.value != size:
            raise OSError(ctypes.get_last_error(), "Bounded configuration read unavailable")
        return output.raw

    def engine_ini(self):
        if self.base is None:
            self.find_main_image()
        rva = 0xd76ca38
        if rva + 16 > self.image_bytes:
            raise ValueError("Configuration RVA outside the observed main image")
        return read_fstring(self.read, self.base + rva)

    def engine_record(self):
        category_key = self.engine_ini()
        cache = struct.unpack("<Q", self.read(self.base + 0xd97ad48, 8))[0]
        if cache < 0x10000:
            raise ValueError("Configuration cache is not initialized")
        # The fixed Engine category is index zero. Its filename is at C+0x20;
        # FConfigFile begins at C+0x30 (confirmed against ctor and finder code).
        filename = read_fstring(self.read, cache + 0x20)
        config_file = cache + 0x30
        metadata = self.read(config_file + 0x50, 3)
        section_slots = struct.unpack("<i", self.read(config_file + 8, 4))[0]
        free_slots = struct.unpack("<i", self.read(config_file + 0x34, 4))[0]
        source = struct.unpack("<Q", self.read(config_file + 0xd8, 8))[0]
        engine_config_dir = read_fstring(self.read, config_file + 0xb8)
        project_config_dir = read_fstring(self.read, config_file + 0xc8)
        if not 0 <= free_slots <= section_slots <= 65536:
            raise ValueError("Configuration section bounds are not stable")
        return {"category_key": category_key, "destination_filename": filename,
                "metadata_bytes_untyped": dict(zip(("0x50", "0x51", "0x52"), metadata)),
                "section_slots": section_slots, "free_section_slots": free_slots,
                "source_configuration_present": bool(source),
                "engine_config_dir": engine_config_dir, "project_config_dir": project_config_dir,
                "source_ini_paths": self.source_paths(config_file + 0x60),
                "preloaded_config_archive": self.saved_config_archive_info(),
                "engine_section_present": self.has_engine_section(config_file, "/Script/Engine.Engine"),
                "source_probe_section_present": self.has_engine_section(config_file, "TrainerConfigSourceProbe")}

    def name_entry(self, display_id):
        block_index, entry_index = display_id >> 16, display_id & 0xffff
        if block_index >= 8192:
            raise ValueError("NamePool block exceeds the observed layout bound")
        block = struct.unpack("<Q", self.read(self.base + 0xd8c0800 + 0x10 + block_index * 8, 8))[0]
        if block < 0x10000:
            raise ValueError("NamePool block is not initialized")
        entry = block + entry_index * 4
        comparison, header = struct.unpack("<IH", self.read(entry, 6))
        length, wide = header >> 1, bool(header & 1)
        if not 0 < length <= 1024:
            raise ValueError("Invalid bounded name-entry length")
        return comparison, length, wide, self.read(entry + 6, length * (2 if wide else 1))

    def configure_name_decoder(self, cache):
        # Derive only the metadata transformation from a statically established
        # name, then verify two independent category names. Never emit its bytes.
        name = self.read(cache + 0x30 + 0x54, 12)
        comparison, number, display = struct.unpack("<III", name)
        entry_comparison, length, wide, payload = self.name_entry(display)
        expected = b"Engine"
        if number or wide or length != len(expected) or comparison != entry_comparison:
            raise ValueError("Engine name is not the established decoder anchor")
        key = bytes(payload[i] ^ expected[i] ^ length for i in range(4))
        if bytes(byte ^ (length & 255) ^ key[i & 3] for i, byte in enumerate(payload)) != expected:
            raise ValueError("Name metadata transformation failed anchor verification")
        self._name_key = key
        try:
            for index, expected_name in enumerate(("Engine", "Game", "Input")):
                if self.fname(self.read(cache + 0x10 + index * 0x160, 12)) != expected_name:
                    raise ValueError("Name metadata transformation failed independent verification")
        except Exception:
            self._name_key = None
            raise

    def fname(self, value):
        if self._name_key is None:
            raise ValueError("Name metadata decoder is not verified")
        comparison, number, display = struct.unpack("<III", value)
        entry_comparison, length, wide, encrypted = self.name_entry(display)
        if comparison != entry_comparison or number > 2147483647:
            raise ValueError("Name entry identity is inconsistent")
        payload = bytearray(encrypted)
        for index in range(length):
            payload[index] ^= (length & 255) ^ self._name_key[index & 3]
        text = payload.decode("utf-16-le" if wide else "ascii", errors="strict")
        return text + ("_" + str(number - 1) if number else "")

    def category_names(self):
        if self.base is None:
            self.find_main_image()
        cache = struct.unpack("<Q", self.read(self.base + 0xd97ad48, 8))[0]
        self.configure_name_decoder(cache)
        return [self.fname(self.read(cache + 0x10 + index * 0x160, 12)) for index in range(9)]

    def saved_config_archive_info(self):
        pointer, length = struct.unpack("<QQ", self.read(self.base + 0xd76c1d8, 16))
        return {"present": bool(pointer), "bytes": length,
                "bounded": bool(pointer) and 0 < length <= 64 * 1024 * 1024}

    def export_saved_config_archive(self, path):
        location = self.base + 0xd76c1d8
        before = self.read(location, 16)
        pointer, length = struct.unpack("<QQ", before)
        if not pointer or not 0 < length <= 64 * 1024 * 1024:
            raise ValueError("No bounded preloaded configuration archive")
        payload = bytearray()
        for offset in range(0, length, 65536):
            payload.extend(self.read(pointer + offset, min(65536, length - offset)))
        if self.read(location, 16) != before:
            raise ValueError("Preloaded configuration archive changed during read")
        path.write_bytes(payload)
        return {"bytes": length, "sha256": hashlib.sha256(payload).hexdigest(), "path": str(path)}

    def source_paths(self, table):
        data, count, maximum = struct.unpack("<Qii", self.read(table, 16))
        if not 0 <= count <= maximum <= 256 or (count and not data):
            raise ValueError("Invalid configuration source-path array")
        secondary = struct.unpack("<Q", self.read(table + 0x20, 8))[0]
        bits = struct.unpack("<i", self.read(table + 0x28, 4))[0]
        if not count <= bits <= 256:
            raise ValueError("Invalid source-path allocation bits")
        words = self.read(secondary or table + 0x10, ((bits + 31) // 32) * 4)
        result = []
        for index in range(count):
            if words[index // 8] & (1 << (index % 8)):
                filename = read_fstring(self.read, data + index * 0x20 + 8)
                if filename.lower().endswith(".ini"):
                    result.append(filename)
        return result

    def has_engine_section(self, config_file, wanted):
        data, count, maximum = struct.unpack("<Qii", self.read(config_file, 16))
        if not 0 <= count <= maximum <= 4096 or (count and not data):
            raise ValueError("Invalid bounded Engine section array")
        secondary = struct.unpack("<Q", self.read(config_file + 0x20, 8))[0]
        bits = struct.unpack("<i", self.read(config_file + 0x28, 4))[0]
        if not count <= bits <= 4096:
            raise ValueError("Invalid Engine section allocation bits")
        words = self.read(secondary or config_file + 0x10, ((bits + 31) // 32) * 4)
        for index in range(count):
            if words[index // 8] & (1 << (index % 8)):
                name = read_fstring(self.read, data + index * 0xb8, max_chars=1024)
                if name == wanted:
                    return True
        return False

