"""Read the nine core configuration categories; no account-settings maps are read."""
from __future__ import annotations

import struct
import time

from native_observation import read_fstring

CORE_CATEGORIES = ("Engine", "Game", "Input", "DeviceProfiles", "Scalability",
                   "RuntimeOptions", "InstallBundle", "Hardware", "GameplayTags")
GLOBAL_STRING_RVAS = (0xd76c7a8, 0xd76d088, 0xd76c778, 0xd76cc30, 0xd76cd90,
                      0xd76c580, 0xd76cb18, 0xd76c5b0, 0xd76c7b8, 0xd76c828,
                      0xd76c570, 0xd76cc00, 0xd76d048, 0xd76ca38)


class CoreSnapshot:
    def __init__(self, observer, seconds=4):
        self.observer = observer
        self.deadline = time.monotonic() + seconds
        self.bytes_read = 0

    def read(self, address, size):
        if time.monotonic() > self.deadline:
            raise ValueError("Core configuration observation deadline")
        self.bytes_read += size
        if self.bytes_read > 64 * 1024 * 1024 or not 0 <= size <= 2 * 1024 * 1024:
            raise ValueError("Core configuration observation budget")
        return b"".join(self.observer.read(address + offset, min(65536, size - offset))
                        for offset in range(0, size, 65536))

    def string(self, address):
        return read_fstring(self.read, address, max_chars=1024 * 1024)

    def name(self, address):
        return self.observer.fname(self.read(address, 12))

    def entries(self, table, stride, max_slots=65536):
        header = self.read(table, 16)
        data, count, maximum = struct.unpack("<Qii", header)
        if not 0 <= count <= maximum <= max_slots or (count and not data):
            raise ValueError("Invalid core configuration table")
        secondary = struct.unpack("<Q", self.read(table + 0x20, 8))[0]
        bits = struct.unpack("<i", self.read(table + 0x28, 4))[0]
        free = struct.unpack("<i", self.read(table + 0x34, 4))[0]
        if not count <= bits <= max_slots or not 0 <= free <= count:
            raise ValueError("Invalid core configuration table allocation")
        allocated = self.read(secondary or table + 0x10, ((bits + 31) // 32) * 4)
        slots = [data + index * stride for index in range(count)
                 if allocated[index // 8] & (1 << (index % 8))]
        if len(slots) != count - free or self.read(table, 16) != header:
            raise ValueError("Core configuration table changed during observation")
        return slots

    def name_string_map(self, table):
        return [[self.name(entry), self.string(entry + 0x10)]
                for entry in self.entries(table, 0x28)]

    def section(self, address):
        return {"pairs": [[self.name(entry), self.string(entry + 0x10)]
                           for entry in self.entries(address, 0x40)],
                "array_of_struct_keys": self.name_string_map(address + 0x50)}

    def file(self, address, parents=()):
        if address in parents or len(parents) > 4:
            raise ValueError("Recursive core configuration is cyclic or too deep")
        before = self.read(address, 16)
        sections = [[self.string(entry), self.section(entry + 0x10)]
                    for entry in self.entries(address, 0xb8, 8192)]
        source = struct.unpack("<Q", self.read(address + 0xd8, 8))[0]
        result = {
            "sections": sections,
            "flags": list(self.read(address + 0x50, 3)),
            "name": self.name(address + 0x54),
            "source_hierarchy": [[struct.unpack("<i", self.read(entry, 4))[0], self.string(entry + 8)]
                                  for entry in self.entries(address + 0x60, 0x20, 1024)],
            "integer_b0": struct.unpack("<I", self.read(address + 0xb0, 4))[0],
            "engine_config_dir": self.string(address + 0xb8),
            "source_config": self.file(source, (*parents, address)) if source else None,
            "project_config_dir": self.string(address + 0xc8),
            "source_filename": self.string(address + 0xe0),
            "per_object_config_array_of_struct_keys": [
                [self.string(entry), self.name_string_map(entry + 0x10)]
                for entry in self.entries(address + 0xf0, 0x68, 8192)],
        }
        if self.read(address, 16) != before:
            raise ValueError("Core configuration sections changed during observation")
        return result

    def capture(self):
        if self.observer.base is None:
            self.observer.find_main_image()
        cache = struct.unpack("<Q", self.read(self.observer.base + 0xd97ad48, 8))[0]
        self.observer.configure_name_decoder(cache)
        result = {"schema_version": 1, "scope": "Nine core game configuration categories; no account-settings maps",
                  "categories": [], "other_files": [],
                  "other_cached_file_count_not_read": struct.unpack("<i", self.read(cache + 0xc78, 4))[0]
                                                      - struct.unpack("<i", self.read(cache + 0xca4, 4))[0]}
        for index, expected in enumerate(CORE_CATEGORIES):
            entry = cache + 0x10 + index * 0x160
            category = self.name(entry)
            if category != expected:
                raise ValueError("Core configuration category order mismatch")
            result["categories"].append({"category": category, "filename": self.string(entry + 0x10),
                                         "file": self.file(entry + 0x20)})
        result["cache_flags"] = list(self.read(cache + 8, 3))
        result["global_ini_strings"] = [self.string(self.observer.base + rva) for rva in GLOBAL_STRING_RVAS]
        result["observed_bytes_read"] = self.bytes_read
        return result
