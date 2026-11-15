"""Codec for the pinned client's native IniBootstrap archive (see format report)."""
from __future__ import annotations

import argparse
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import struct

from native_config_snapshot import CORE_CATEGORIES
from native_maps import MAPS, map_package

MAX_BYTES = 64 * 1024 * 1024
MAX_COUNT = 100000
MAX_STRING_UNITS = 1024 * 1024


class Archive:
    def __init__(self, data=None):
        self.loading = data is not None
        self.data = bytes(data) if self.loading else bytearray()
        self.offset = 0
        if len(self.data) > MAX_BYTES:
            raise ValueError("Archive byte limit")

    def transfer(self, size, value=None):
        if self.loading:
            if size < 0 or self.offset + size > len(self.data):
                raise ValueError("Truncated native config archive")
            result = self.data[self.offset:self.offset + size]
            self.offset += size
            return result
        if value is None or len(value) != size or len(self.data) + size > MAX_BYTES:
            raise ValueError("Invalid native config write")
        self.data.extend(value)

    def number(self, code, value=None):
        size = struct.calcsize(code)
        if self.loading:
            return struct.unpack(code, self.transfer(size))[0]
        self.transfer(size, struct.pack(code, value))
        return value

    def boolean(self, value=None):
        if not self.loading and value not in (0, 1, False, True):
            raise ValueError("Noncanonical Bool32")
        result = self.number("<I", None if self.loading else int(value))
        if result not in (0, 1):
            raise ValueError("Noncanonical Bool32")
        return result

    def string(self, value=None):
        if self.loading:
            length = self.number("<i")
            if abs(length) > MAX_STRING_UNITS:
                raise ValueError("Native string limit")
            if not length:
                return ""
            payload = self.transfer(length if length > 0 else -length * 2)
            text = payload.decode("ascii" if length > 0 else "utf-16-le", errors="strict")
            if not text.endswith("\0") or "\0" in text[:-1] or "\uffff" in text:
                raise ValueError("Invalid native string")
            return text[:-1]
        if not isinstance(value, str) or "\0" in value or "\uffff" in value:
            raise ValueError("Invalid native string input")
        if not value:
            self.number("<i", 0)
            return value
        wide = not value.isascii()
        payload = (value + "\0").encode("utf-16-le" if wide else "ascii")
        units = len(payload) // 2 if wide else len(payload)
        if units > MAX_STRING_UNITS:
            raise ValueError("Native string limit")
        self.number("<i", -units if wide else units)
        self.transfer(len(payload), payload)
        return value

    def sequence(self, item, values=None):
        count = self.number("<i", None if self.loading else len(values))
        if not 0 <= count <= MAX_COUNT:
            raise ValueError("Native container count limit")
        return [item(None if self.loading else values[index]) for index in range(count)]

    def name_strings(self, values=None):
        return self.sequence(lambda pair: [self.string(None if pair is None else pair[0]),
                                           self.string(None if pair is None else pair[1])], values)

    def section(self, value=None):
        return {"pairs": self.name_strings(None if self.loading else value["pairs"]),
                "array_of_struct_keys": self.name_strings(None if self.loading else value["array_of_struct_keys"])}

    def config_file(self, value=None, depth=0):
        if depth > 8:
            raise ValueError("Native config recursion limit")
        def section_item(pair):
            return [self.string(None if pair is None else pair[0]), self.section(None if pair is None else pair[1])]
        result = {"sections": self.sequence(section_item, None if self.loading else value["sections"])}
        result["flags"] = [self.boolean(None if self.loading else value["flags"][index]) for index in range(3)]
        result["name"] = self.string(None if self.loading else value["name"])
        result["source_hierarchy"] = self.sequence(
            lambda pair: [self.number("<i", None if pair is None else pair[0]), self.string(None if pair is None else pair[1])],
            None if self.loading else value["source_hierarchy"])
        result["integer_b0"] = self.number("<I", None if self.loading else value["integer_b0"] & 0xffffffff)
        result["engine_config_dir"] = self.string(None if self.loading else value["engine_config_dir"])
        has_source = self.boolean(None if self.loading else int(value["source_config"] is not None))
        result["source_config"] = self.config_file(None if self.loading else value["source_config"], depth + 1) if has_source else None
        result["project_config_dir"] = self.string(None if self.loading else value["project_config_dir"])
        result["source_filename"] = self.string(None if self.loading else value["source_filename"])
        result["per_object_config_array_of_struct_keys"] = self.sequence(
            lambda pair: [self.string(None if pair is None else pair[0]), self.name_strings(None if pair is None else pair[1])],
            None if self.loading else value["per_object_config_array_of_struct_keys"])
        return result

    def bootstrap(self, value=None):
        result = {"other_files": self.sequence(
            lambda item: {"filename": self.string(None if item is None else item["filename"]),
                          "file": self.config_file(None if item is None else item["file"])},
            None if self.loading else value["other_files"]), "categories": []}
        for index, category in enumerate(CORE_CATEGORIES):
            item = None if self.loading else value["categories"][index]
            if item is not None and item["category"] != category:
                raise ValueError("Native category order mismatch")
            result["categories"].append({"category": category,
                                          "filename": self.string(None if self.loading else item["filename"]),
                                          "file": self.config_file(None if self.loading else item["file"])})
        result["cache_flags"] = [self.boolean(None if self.loading else value["cache_flags"][0]),
                                  self.boolean(None if self.loading else value["cache_flags"][1]),
                                  self.number("<B", None if self.loading else value["cache_flags"][2])]
        result["global_ini_strings"] = [self.string(None if self.loading else value["global_ini_strings"][index]) for index in range(14)]
        if self.loading and self.offset != len(self.data):
            raise ValueError("Trailing bytes after native config archive")
        return result


def encode(snapshot):
    if len(snapshot["categories"]) != 9 or len(snapshot["global_ini_strings"]) != 14:
        raise ValueError("Incomplete native configuration snapshot")
    archive = Archive()
    archive.bootstrap(snapshot)
    return bytes(archive.data)


def decode(payload):
    return Archive(payload).bootstrap()


def set_value(config_file, section_name, key, value):
    for name, section in config_file["sections"]:
        if name == section_name:
            matches = [index for index, pair in enumerate(section["pairs"]) if pair[0] == key]
            if matches:
                section["pairs"][matches[-1]][1] = value
            else:
                section["pairs"].append([key, value])
            break
    else:
        config_file["sections"].append([section_name, {"pairs": [[key, value]], "array_of_struct_keys": []}])


def apply_profile(snapshot, profile, map_number=1):
    package = map_package(map_number, training=profile == 'training')
    if profile == "duel1-navigation" and map_number != 1:
        raise ValueError("Navigation development is restricted to Duel 1")
    result = deepcopy(snapshot)
    if profile in ("bootstrap-probe", "base-engine", "scene", "duel1-navigation", "training"):
        set_value(result["categories"][0]["file"], "TrainerConfigSourceProbe", "SourceMarker", "NativeBootstrapV1")
    if profile in ("base-engine", "scene", "duel1-navigation", "training"):
        engine = result["categories"][0]["file"]
        for section, key, value in (
            ("/Script/Engine.Engine", "GameEngine", "/Script/Engine.GameEngine"),
            ("/Script/Engine.Engine", "GameViewportClientClassName", "/Script/Engine.GameViewportClient"),
            ("/Script/Engine.Engine", "LocalPlayerClassName", "/Script/Engine.LocalPlayer"),
            ("/Script/EngineSettings.GameMapsSettings", "GameInstanceClass", "/Script/Engine.GameInstance"),
            ("/Script/EngineSettings.GameMapsSettings", "GameDefaultMap", package),
        ):
            set_value(engine, section, key, value)
        if profile == "scene":
            set_value(engine, "/Script/EngineSettings.GameMapsSettings", "LocalMapOptions",
                      "?game=/Script/Engine.GameMode")
        elif profile in ("duel1-navigation", "training"):
            set_value(engine, "/Script/EngineSettings.GameMapsSettings", "LocalMapOptions",
                      "?game=/Script/Engine.GameModeBase")
            # The original EnhancedInput configuration depends on title-specific
            # mapping contexts. Use the shipped base input classes for DefaultPawn.
            input_config = result["categories"][2]["file"]
            for key, value in (("DefaultPlayerInputClass", "/Script/Engine.PlayerInput"),
                               ("DefaultInputComponentClass", "/Script/Engine.InputComponent")):
                set_value(input_config, "/Script/Engine.InputSettings", key, value)
    elif profile not in ("original", "bootstrap-probe"):
        raise ValueError("Unknown native profile")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("snapshot", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--profile", choices=("original", "bootstrap-probe", "base-engine", "scene", "duel1-navigation"), default="original")
    parser.add_argument("--map", type=int, choices=MAPS, default=1, dest="map_number")
    args = parser.parse_args()
    snapshot = apply_profile(json.loads(args.snapshot.read_text(encoding="utf-8")), args.profile, args.map_number)
    payload = encode(snapshot)
    reconstructed = decode(payload)
    if encode(reconstructed) != payload:
        raise ValueError("Native archive round-trip mismatch")
    # Compare complete encoded wire content, preserving all duplicate pairs.
    expected = {key: snapshot[key] for key in ("other_files", "categories", "cache_flags", "global_ini_strings")}
    if reconstructed != expected:
        raise ValueError("Native archive did not preserve original core configuration")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(payload)
    print(json.dumps({"bytes": len(payload), "sha256": hashlib.sha256(payload).hexdigest(),
                      "profile": args.profile, "categories": len(snapshot["categories"]),
                      "other_cached_file_count_not_read": snapshot.get("other_cached_file_count_not_read"),
                      "round_trip": True}))


if __name__ == "__main__":
    main()
