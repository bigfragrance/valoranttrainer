"""Read-only original-client map catalogue and source minimap extraction.

All source containers are opened read-only. This tool does not start the game,
alter native code or containers, or infer runtime validation from static data.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import sys
from collections import Counter

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "work/extract"))
from iostore import IoStore
from zen import Zen, names_at

SOURCE = Path("F:/Riot Games/VALORANT/live/ShooterGame/Content/Paks")
WORK = ROOT / "work/training-maps"
STANDARD_CODES = {"Ascent", "Bonsai", "Canyon", "Duality", "Foxtrot", "Infinity", "Jam", "Juliett", "Pitt", "Plummet", "Port", "Rook", "Triad"}
EDITOR_ZH_ALIASES = {"Ascent": "亚海悬城", "Bonsai": "霓虹町", "Canyon": "裂变峡谷", "Duality": "源工重镇", "Foxtrot": "微风岛屿", "Infinity": "幽邃地窟", "Jam": "莲华古城", "Juliett": "日落之城", "Pitt": "深海明珠", "Port": "森寒冬港", "Rook": "盐海矿镇", "Triad": "隐世修所"}
DEDICATED_BARRIER_LAYERS = {
    "/Game/Maps/Bonsai/Bonsai_SpawnBarrierVisuals",
    "/Game/Maps/Duality/Duality_SpawnBarrier",
    "/Game/Maps/Infinity/Infinity_Barriers",
    "/Game/Maps/Juliett/Juliett_Barriers",
    "/Game/Maps/Plummet/Plummet_Design_SpawnBarriers",
}


class Cursor:
    def __init__(self, data, position=0, names=None):
        self.data, self.position, self.names = data, position, names

    def read(self, size):
        if size < 0 or self.position + size > len(self.data):
            raise ValueError("serialized value exceeds export")
        result = self.data[self.position:self.position + size]
        self.position += size
        return result

    def value(self, fmt):
        values = struct.unpack(fmt, self.read(struct.calcsize(fmt)))
        return values[0] if len(values) == 1 else list(values)

    def string(self):
        count = self.value("<i")
        if abs(count) > 100000:
            raise ValueError("oversized source FString")
        value = self.read(count if count >= 0 else -2 * count)
        return value.decode("utf8" if count >= 0 else "utf-16le").rstrip("\0")

    def name(self):
        index, number = self.value("<II")
        if index & 0xc0000000 or index >= len(self.names):
            raise ValueError("invalid serialized local FName")
        return self.names[index] + ("_" + str(number - 1) if number else "")

    def softpath(self):
        package, asset, sub = self.name(), self.name(), self.string()
        return dict(package=package, asset=asset, subpath=sub,
                    object_path=package + "." + asset + (":" + sub if sub else ""))

    def text(self):
        flags = self.value("<I")
        history = self.value("<b")
        if history == 11:
            return dict(flags=flags, history="string_table", table=self.name(), key=self.string())
        if history == -1:
            return dict(flags=flags, history="none", text=self.string())
        if history == 0:
            return dict(flags=flags, history="base", namespace=self.string(), key=self.string(), text=self.string())
        raise ValueError("unsupported original FText history " + str(history))


def object_reference(source, z, reference):
    if reference == 0:
        return None
    if reference > 0:
        entry = z.exports[reference - 1]
        return dict(export=reference - 1, name=entry["name"], type=source.resolve(z, int(entry["class_index"], 16)))
    return source.resolve(z, z.imports[-reference - 1])


def decode_map_asset(source, record):
    z = Zen(record["raw"])
    entry = next(e for e in z.exports if e["name"].startswith("Default__"))
    data = z.content(entry)
    fields, start = unversioned(data)
    cursor = Cursor(data, start, z.names)
    shift = int("/Game/Maps/BaseMapPrimaryAsset" in z.imported_packages)
    result = dict(source=record["package"], cdo_export=entry["index"], serialized_fields=[])
    keys = ["world_path", "story_content_tracked", "ui_data", "loading_image", "mode_tags", "map_asset_id", "hide_if_not_owned", "base_content", "native_class"]
    for slot, zero in fields:
        offset = cursor.position
        if shift and slot == 0:
            result["original_map_enum_value"] = 0 if zero else cursor.value("<B")
            result["serialized_fields"].append(dict(slot=slot, name="BaseMapPrimaryAsset.MapID", offset=entry["start"] + offset))
            continue
        index = slot - shift
        key = keys[index]
        if zero:
            value = None
        elif index == 0:
            value = cursor.string()
        elif index in (1, 6, 7):
            value = bool(cursor.value("<B"))
        elif index in (2, 3):
            value = cursor.softpath()
        elif index == 4:
            count = cursor.value("<I")
            if count > 1000:
                raise ValueError("invalid map mode-tag count")
            value = [cursor.name() for _ in range(count)]
        elif index == 5:
            words = cursor.value("<4I")
            compact = "".join(f"{word:08x}" for word in words)
            value = "-".join((compact[:8], compact[8:12], compact[12:16], compact[16:20], compact[20:]))
            result["guid_words"] = words
        elif index == 8:
            value = object_reference(source, z, cursor.value("<i"))
        result[key] = value
        result["serialized_fields"].append(dict(slot=slot, name=key, offset=entry["start"] + offset))
    if data[cursor.position:] != bytes(4):
        raise ValueError("MapDataAsset native suffix did not validate")
    return result


def decode_ui_asset(source, package):
    record = source.describe(package)
    z = Zen(record["raw"])
    entry = next(e for e in z.exports if e["name"].startswith("Default__"))
    data = z.content(entry)
    fields, start = unversioned(data)
    cursor = Cursor(data, start, z.names)
    keys = ["x_offset", "x_multiplier", "y_offset", "y_multiplier", "list_view_icon", "list_view_icon_tall", "coordinates", "narrative_description", "tactical_description", "background_image", "secondary_background_image", "stylized_background_image", "premier_background_image", "display_name", "display_name_all_caps", "display_icon", "icon_transform", "world_context"]
    values, evidence = {}, []
    for slot, zero in fields:
        key = keys[slot]
        offset = cursor.position
        if zero:
            value = None
        elif slot <= 3:
            value = cursor.value("<f")
        elif slot in (6, 7, 8, 13, 14):
            value = cursor.text()
        elif slot in (4, 5, 9, 10, 11, 12, 15):
            value = object_reference(source, z, cursor.value("<i"))
        else:
            raise ValueError("unhandled serialized MapUIData property " + key)
        values[key] = value
        evidence.append(dict(slot=slot, name=key, offset=entry["start"] + offset))
    if data[cursor.position:] != bytes(4):
        raise ValueError("MapUIData native suffix did not validate")
    return dict(package=package, source_sha256=record["sha256"], values=values,
                serialized_fields=evidence, native_schema="work/training-map-minimap-static/classes.json")


def decode_string_table(source, package):
    path = source.export(package)
    z = Zen(path)
    entry = next(e for e in z.exports if source.resolve(z, int(e["class_index"], 16)) == "/Script/Engine.StringTable")
    data = z.content(entry)
    fields, start = unversioned(data)
    if fields or data[start:start + 4] != bytes(4):
        raise ValueError("unsupported string-table property prefix")
    cursor = Cursor(data, start + 4, z.names)
    namespace = cursor.string()
    count = cursor.value("<I")
    if count > 10000:
        raise ValueError("oversized map string table")
    entries = {}
    for _ in range(count):
        key = cursor.string()
        entries[key] = cursor.string()
    # Per-key metadata can follow; map name extraction does not reinterpret it.
    return dict(package=package, namespace=namespace, entries=entries,
                metadata_suffix_bytes=len(data) - cursor.position,
                source_sha256=hashlib.sha256(z.data).hexdigest())


def package_name(path):
    if path.startswith("../../../ShooterGame/Content/"):
        path = "/Game/" + path.split("Content/", 1)[1]
    elif path.startswith("../../../Engine/Content/"):
        path = "/Engine/" + path.split("Content/", 1)[1]
    elif "/Content/" in path:
        parent, path = path.split("/Content/", 1)
        path = "/" + parent.rsplit("/", 1)[-1] + "/" + path
    else:
        return None
    return re.sub(r"\.(m\.ubulk|uasset|umap|ubulk|uptnl)$", "", path)


def unversioned(data):
    fragments = []
    position = 0
    index = 0
    while True:
        fragment = struct.unpack_from("<H", data, position)[0]
        position += 2
        skip, has_zero, last, count = fragment & 127, bool(fragment & 128), bool(fragment & 256), fragment >> 9
        index += skip
        fragments.append((index, count, has_zero))
        index += count
        if last:
            break
        if len(fragments) > 1024:
            raise ValueError("invalid unversioned property fragments")
    zero_bits = sum(count for _, count, has_zero in fragments if has_zero)
    # The observed format stores a byte/word/dword aligned zero mask.
    mask_bytes = 0 if not zero_bits else 1 if zero_bits <= 8 else 2 if zero_bits <= 16 else 4 * ((zero_bits + 31) // 32)
    mask = int.from_bytes(data[position:position + mask_bytes], "little")
    position += mask_bytes
    result = []
    bit = 0
    for start, count, has_zero in fragments:
        for item in range(start, start + count):
            result.append((item, bool(mask & (1 << bit)) if has_zero else False))
            bit += int(has_zero)
    return result, position


class Source:
    def __init__(self, path=SOURCE):
        self.stores = {}
        self.index = {}
        self.containers = []
        self.toc_hashes = {}
        for toc in sorted(path.glob("*.utoc")):
            store = IoStore(toc)
            self.stores[toc.name] = store
            self.toc_hashes[toc.name] = hashlib.sha256(store.data).hexdigest()
            self.containers.append(dict(name=toc.name, sha256=self.toc_hashes[toc.name], indexed_files=len(store.files)))
            for filename, index in store.files.items():
                package = package_name(filename)
                if package:
                    extension = filename[len(filename.rsplit(".", 1)[0]):]
                    if filename.endswith(".m.ubulk"):
                        extension = ".m.ubulk"
                    self.index[(package.lower(), extension)] = (store, filename, index)
        raw = (ROOT / "work/client-analysis/continue-blueprint/global-scriptobjects.bin").read_bytes()
        names, cursor = names_at(raw, 0)
        count = struct.unpack_from("<I", raw, cursor)[0]
        cursor += 4
        self.script_objects = {}
        for item in range(count):
            ni, nn, index, outer, cdo = struct.unpack_from("<IIQQQ", raw, cursor + item * 32)
            name = names[ni & 0x3fffffff] + ("_" + str(nn - 1) if nn else "")
            self.script_objects[index] = (name, outer)

    def find(self, package, extension=None):
        if extension:
            return self.index[(package.lower(), extension)]
        return self.index.get((package.lower(), ".uasset")) or self.index[(package.lower(), ".umap")]

    def export(self, package, extension=None):
        store, filename, index = self.find(package, extension)
        extension = extension or (".umap" if filename.endswith(".umap") else ".uasset")
        destination = WORK / "metadata" / (package.strip("/") + extension)
        metadata_path = destination.with_name(destination.name + ".source.json")
        expected_id = store.ids[index]
        valid = False
        if destination.exists() and metadata_path.exists():
            record = json.loads(metadata_path.read_text(encoding="utf8"))
            valid = (record.get("chunk_id") == expected_id
                     and record.get("source_toc_sha256") == self.toc_hashes[store.path.name]
                     and record.get("sha256") == hashlib.sha256(destination.read_bytes()).hexdigest())
        if not valid:
            record = store.export(filename, destination)
            record["source_toc_sha256"] = self.toc_hashes[store.path.name]
            metadata_path.write_text(json.dumps(record, indent=2), encoding="utf8")
        return destination

    def read_bulk(self, package):
        path = self.export(package, ".ubulk")
        return path.read_bytes(), path

    def resolve(self, z, index):
        if index == 0xffffffffffffffff:
            return None
        if index >> 62 == 1:
            def name(value):
                if value == 0xffffffffffffffff:
                    return ""
                if value not in self.script_objects:
                    return hex(value)
                leaf, outer = self.script_objects[value]
                parent = name(outer)
                return parent + "." + leaf if parent else leaf
            return name(index)
        if index >> 62 == 2:
            return z.imported_packages[(index >> 32) & 0x3fffffff] + " #" + hex(z.hashes[index & 0xffffffff])
        if index < len(z.exports):
            return z.exports[index]["name"]
        return hex(index)

    def describe(self, package):
        path = self.export(package)
        z = Zen(path)
        exports = []
        for entry in z.exports:
            data = z.content(entry)
            try:
                properties, start = unversioned(data)
            except (ValueError, struct.error):
                properties, start = None, None
            exports.append(dict(entry, type=self.resolve(z, int(entry["class_index"], 16)),
                                super=self.resolve(z, int(entry["super_index"], 16)),
                                property_slots=properties, properties_start=start,
                                serialized_hex=data.hex() if len(data) <= 8192 else None))
        provenance = json.loads(path.with_name(path.name + ".source.json").read_text(encoding="utf8"))
        return dict(package=package, raw=str(path), sha256=hashlib.sha256(z.data).hexdigest(),
                    container=provenance["container"], chunk_id=provenance["chunk_id"],
                    source_blocks_verified=provenance["physical_block_sha1_verified_against_toc"],
                    imports=z.imported_packages, names=z.names, exports=exports)


def discover(source):
    candidates = []
    for (package, extension), (store, filename, index) in source.index.items():
        if extension != ".uasset":
            continue
        original = package_name(filename)
        if re.fullmatch(r"/Game/Maps/[^/]+/[^/]*PrimaryAsset", original):
            candidates.append(original)
    candidates = sorted(set(candidates))
    records = [source.describe(package) for package in candidates]
    dependencies = set()
    for record in records:
        dependencies.update(p for p in record["imports"] if p.startswith("/Game/Maps/") or "UIData" in p)
        dependencies.update(p for p in record["names"] if p.startswith("/Game/") and "UIData" in p)
    secondary = []
    for package in sorted(dependencies - set(candidates)):
        try:
            store, path, index = source.find(package)
            # Asset definitions only; do not copy entire map geometry here.
            if store.ranges[index][1] <= 2 * 1024 * 1024 and path.endswith(".uasset"):
                secondary.append(source.describe(package))
        except KeyError:
            pass
    result = dict(schema_version=1, generated_utc=datetime.now(timezone.utc).isoformat(),
                  source_read_only=str(SOURCE), source_containers=source.containers,
                  map_primary_assets=records, related_asset_definitions=secondary,
                  runtime_validation="pending; no game started by analyser")
    WORK.mkdir(parents=True, exist_ok=True)
    (WORK / "discovery.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf8")
    print(json.dumps(dict(map_primary_assets=len(records), related_asset_definitions=len(secondary),
                          candidates=candidates), ensure_ascii=False))
    return result


def decode_spawn_capsule(source, z, entry):
    data = z.content(entry)
    fields, start = unversioned(data)
    cursor = Cursor(data, start, z.names)
    values = dict(location=[0., 0., 0.], rotation=[0., 0., 0.], scale=[1., 1., 1.])
    offsets = {}
    for slot, zero in fields:
        offsets[str(slot)] = entry["start"] + cursor.position
        if slot in (0, 1):
            values["capsule_half_height_cm" if slot == 0 else "capsule_radius_cm"] = 0. if zero else cursor.value("<f")
        elif slot == 9:
            if not zero:
                cursor.read(4)  # original ShapeComponent.AreaClassOverride UClass reference
        elif slot == 10:
            if not zero:
                cursor.read(1)  # original bUseSystemDefaultObstacleAreaClass bool
        elif slot in (137, 138, 139):
            value = [0., 0., 0.] if zero else cursor.value("<3d")
            if not all(math.isfinite(x) and abs(x) < 1e8 for x in value):
                raise ValueError("invalid source spawn component transform")
            values[{137: "location", 138: "rotation", 139: "scale"}[slot]] = value
        else:
            raise ValueError("unhandled original spawn-capsule slot " + str(slot))
    if data[cursor.position:] != bytes(8):
        raise ValueError("spawn capsule native suffix did not validate")
    return values, offsets


def decode_spawn_actor(source, z, entry):
    """Confirm the actual actor root and original spawn flags before using it."""
    data = z.content(entry)
    fields, start = unversioned(data)
    cursor = Cursor(data, start, z.names)
    values = {}
    for slot, zero in fields:
        if slot in (0, 1, 8, 9, 10, 75):
            value = 0 if zero else cursor.value("<i")
            values[{0: "team_component_ref", 1: "role_component_ref", 8: "capsule_ref", 9: "good_sprite_ref", 10: "bad_sprite_ref", 75: "root_ref"}[slot]] = value
        elif slot == 2:
            values["spawn_set"] = 0 if zero else cursor.value("<i")
        elif slot in (3, 4):
            values["disabled" if slot == 3 else "initial_spawn"] = False if zero else bool(cursor.value("<B"))
        elif slot in (6, 63):
            values["spawn_restriction" if slot == 6 else "spawn_collision_handling"] = 0 if zero else cursor.value("<B")
        elif slot == 7:
            values["player_start_tag"] = None if zero else cursor.name()
        elif slot == 5:
            count = 0 if zero else cursor.value("<I")
            if count > 1000:
                raise ValueError("oversized authored spawn rotations")
            # Exact UHT inner descriptor is Float, not FRotator.
            values["additional_spawn_rotations_float_values"] = [cursor.value("<f") for _ in range(count)]
        elif not zero:
            raise ValueError("unhandled original spawn-actor slot " + str(slot))
    # UObject GUID marker followed by original Actor serialization/label.
    if data[cursor.position:cursor.position + 4] != bytes(4):
        raise ValueError("spawn actor UObject suffix did not validate")
    values["native_footer_offset"] = entry["start"] + cursor.position
    return values


def spawn_anchors(source, package):
    record = source.describe(package)
    z = Zen(record["raw"])
    anchors, errors = [], []
    for entry in z.exports:
        if source.resolve(z, int(entry["class_index"], 16)) != "/Script/Engine.CapsuleComponent":
            continue
        outer = int(entry["outer"], 16)
        if outer >= len(z.exports):
            continue
        actor = z.exports[outer]
        actor_type = source.resolve(z, int(actor["class_index"], 16))
        if actor_type != "/Script/ShooterGame.ShooterTeamStart":
            continue
        try:
            actor_values = decode_spawn_actor(source, z, actor)
            if actor_values.get("root_ref") != entry["index"] + 1 or actor_values.get("capsule_ref") != entry["index"] + 1:
                raise ValueError("actor RootComponent/CapsuleComponent do not reference this original capsule")
            values, offsets = decode_spawn_capsule(source, z, entry)
            pitch, yaw, roll = values["rotation"]
            anchors.append(dict(id=actor["name"], actor_class=actor_type,
                                actor_path=package + "." + package.rsplit("/", 1)[-1] + ":PersistentLevel." + actor["name"],
                                source_package=package, actor_export=actor["index"], capsule_export=entry["index"],
                                raw_actor_location_cm=values["location"], rotation_pitch_yaw_roll_deg=values["rotation"],
                                yaw_deg=yaw, scale=values["scale"],
                                coordinate_basis="original PlayerStart root/capsule center; not feet",
                                feet_cm=None, feet_status="not inferred; resolve actual spawned hero capsule and native spawn semantics",
                                capsule_half_height_cm=values.get("capsule_half_height_cm"),
                                capsule_radius_cm=values.get("capsule_radius_cm"),
                                capsule_default_status="only serialized overrides are reported; native PlayerStart defaults unresolved",
                                root_component_reference_verified=True, original_spawn_properties=actor_values,
                                disabled_by_original=actor_values.get("disabled", False),
                                serialized_offsets=offsets,
                                source_sha256=record["sha256"], runtime_validation="pending"))
        except (ValueError, struct.error, IndexError) as exc:
            errors.append(dict(actor=actor["name"], capsule_export=entry["index"], error=str(exc)))
    return dict(package=package, original_actor_class="/Script/ShooterGame.ShooterTeamStart",
                anchors=anchors, errors=errors, source_sha256=record["sha256"],
                source_actor_count=sum(e["type"] == "/Script/ShooterGame.ShooterTeamStart" for e in record["exports"]))


def package_store_graph(source):
    # Reuse the frozen, separately validated v3 reader without changing its file.
    sys.path.insert(0, str(ROOT / "tools"))
    from analyze_share_resources import parse_header
    graph, indexed = {}, {}
    for store in source.stores.values():
        for filename, index in store.files.items():
            if bytes.fromhex(store.ids[index])[-1] == 1:
                indexed[int.from_bytes(bytes.fromhex(store.ids[index])[:8], "little")] = package_name(filename)
        for index, chunk in enumerate(store.ids):
            if bytes.fromhex(chunk)[-1] == 6:
                store.files["__training_map_header__"] = index
                entries, detail = parse_header(store.read("__training_map_header__"))
                graph.update(entries)
    return graph, indexed


def closure_evidence(source, roots, graph, indexed):
    todo, visited = [], set()
    root_ids, absent_roots = {}, []
    for package in roots:
        try:
            store, filename, index = source.find(package)
        except KeyError:
            absent_roots.append(package)
            continue
        pid = int.from_bytes(bytes.fromhex(store.ids[index])[:8], "little")
        root_ids[package] = f"{pid:016x}"
        todo.append(pid)
    missing_store = set()
    while todo:
        pid = todo.pop()
        if pid in visited:
            continue
        visited.add(pid)
        if pid not in graph:
            missing_store.add(pid)
        todo.extend(graph.get(pid, ()))
    missing_index = visited - indexed.keys()
    missing_details = []
    if not hasattr(source, "missing_name_evidence"):
        source.missing_name_evidence = []
        prior_plan = ROOT / "reports/portable-resource-plan.json"
        if prior_plan.exists():
            prior = json.loads(prior_plan.read_text(encoding="utf8"))
            identities = prior.get("container_evidence", {})
            if set(identities) == set(source.stores) and all(
                    identities[name]["toc_sha256"] == source.toc_hashes[name]
                    for name in source.stores):
                source.missing_name_evidence = prior.get("missing_dependency_evidence", [])
    if missing_store:
        missing_details = [row for row in source.missing_name_evidence
                           if int(row["package_id"], 16) in missing_store]
    return dict(root_packages=root_ids, package_count=len(visited),
                absent_roots=absent_roots,
                missing_store_ids=[f"{p:016x}" for p in sorted(missing_store)],
                missing_store_name_evidence=missing_details,
                non_directory_indexed_ids=[f"{p:016x}" for p in sorted(missing_index)],
                note="Original PackageStore hard imports plus explicitly enumerated map sublevels. Soft references, streaming and actual asset construction still require runtime validation.")


def streaming_levels(source, world):
    z = Zen(world["raw"])
    results = []
    for entry in z.exports:
        kind = source.resolve(z, int(entry["class_index"], 16))
        if kind not in ("/Script/Engine.LevelStreamingAlwaysLoaded", "/Script/Engine.LevelStreamingDynamic"):
            continue
        data = z.content(entry)
        fields, start = unversioned(data)
        cursor = Cursor(data, start, z.names)
        shift = 2 if kind.endswith("LevelStreamingDynamic") else 0
        values = {}
        error = None
        try:
            for slot, zero in fields:
                if shift and slot < shift:
                    values["initially_loaded" if slot == 0 else "initially_visible"] = False if zero else bool(cursor.value("<B"))
                    continue
                index = slot - shift
                if index == 0:
                    values["world_asset"] = None if zero else cursor.softpath()
                elif index in (1, 6):
                    values["priority" if index == 1 else "lod_index"] = 0 if zero else cursor.value("<i")
                elif index == 2:
                    values["package_name_to_load"] = None if zero else cursor.name()
                elif index in (3, 16):
                    if zero:
                        count = 0
                    else:
                        count = cursor.value("<I")
                    if count > 10000:
                        raise ValueError("oversized streaming reference array")
                    values["lod_packages" if index == 3 else "editor_streaming_volumes"] = [cursor.name() if index == 3 else cursor.value("<i") for _ in range(count)]
                elif index == 4:
                    # Keep the authored transform bytes without inventing a
                    # native transform-serializer interpretation.
                    raise ValueError("non-default LevelTransform requires independent decoding")
                elif index in (5, 7, 8, 9, 10, 11, 12, 13, 14):
                    key = {5: "client_only_visible", 7: "should_be_visible", 8: "should_be_loaded", 9: "locked", 10: "is_static", 11: "block_on_load", 12: "block_on_unload", 13: "disable_distance_streaming", 14: "draw_on_status_map"}[index]
                    values[key] = False if zero else bool(cursor.value("<B"))
                elif index == 15:
                    values["level_color"] = None if zero else cursor.value("<4f")
                elif index == 17:
                    values["min_time_between_unloads"] = 0. if zero else cursor.value("<f")
                elif index in (22, 23):
                    values["loaded_level" if index == 22 else "pending_unload_level"] = None if zero else object_reference(source, z, cursor.value("<i"))
                elif not zero:
                    raise ValueError("unhandled streaming property " + str(index))
            if data[cursor.position:] != bytes(4):
                raise ValueError("streaming native suffix did not validate")
        except (ValueError, struct.error, IndexError) as exc:
            error = str(exc)
        world_asset = values.get("world_asset")
        package = world_asset["package"] if world_asset else None
        present = None
        if package:
            try:
                source.find(package)
                present = True
            except KeyError:
                present = False
        results.append(dict(export=entry["index"], name=entry["name"], native_class=kind,
                            authored_values=values, package=package, package_present=present,
                            parse_error=error,
                            native_schema="work/training-maps/native-levelstreaming-schema.json"))
    return results


def layer_policy(row, duel=False):
    package = row.get("package") or ""
    leaf = package.rsplit("/", 1)[-1].lower()
    if not duel and re.search(r"_modes?_ffa(?:_|$)", leaf):
        return "LoadFFA"
    if not duel and package in DEDICATED_BARRIER_LAYERS:
        return "HideBarrier"
    if not duel and (re.search(r"_(?:modes?_|bombgamemode_|siterushgamemode_|quickspikegamemode_)", leaf)
                     or "fortcollins" in leaf or "teamspawnpoints" in leaf):
        return "HideOtherMode"
    if row["native_class"].endswith("LevelStreamingAlwaysLoaded"):
        return "LoadCommon"
    if "navmesh" in leaf:
        return "LoadCommon"
    return "Preserve"


def merge_projections(source, catalogue):
    path = ROOT / "work/training-map-minimap-static/projection.json"
    if not path.exists():
        return catalogue
    document = json.loads(path.read_text(encoding="utf8"))
    projections = {row["id"]: row for row in document["maps"]}
    for row in catalogue["maps"]:
        calibration = projections[row["id"]]
        if calibration["world_path"] != row["world_path"] or calibration["source_main_sha256"] != row["evidence"]["main_world_sha256"]:
            raise ValueError("projection main package identity differs: " + row["id"])
        camera = source.export(calibration.get("camera_package", row["world_path"]))
        if hashlib.sha256(camera.read_bytes()).hexdigest() != calibration.get("camera_package_sha256", calibration["source_main_sha256"]):
            raise ValueError("projection camera package identity differs: " + row["id"])
        forward, inverse = calibration["world_to_uv"], calibration["uv_to_world_cm"]
        width, height = row["minimap"]["width"], row["minimap"]["height"]
        max_error = 0.
        points = [anchor["raw_actor_location_cm"][:2] for anchor in row["spawn_anchors"]]
        points += calibration["uncropped_texture_world_xy_corners_cm"]
        for x, y in points:
            u, v = [a * x + b * y + c for a, b, c in forward]
            rx, ry = [a * u + b * v + c for a, b, c in inverse]
            max_error = max(max_error, abs(rx - x), abs(ry - y))
        if max_error > 1e-6:
            raise ValueError("source map projection round-trip failed: " + row["id"])
        transform = dict(type="affine_2x3", input="world_xy_cm", output="normalized_uv",
                         convention="[u,v] = matrix * [world_x_cm,world_y_cm,1]; u right, v down",
                         matrix=forward, pixel_matrix=[[v * width for v in forward[0]], [v * height for v in forward[1]]],
                         image_width=width, image_height=height,
                         source="original AresMinimapManager full capture transform, native builder RVA 0x6f0b300",
                         evidence="work/training-map-minimap-static/projection.json",
                         runtime_marker_validation="pending")
        reverse = dict(type="affine_2x3", input="normalized_uv", output="world_xy_cm", matrix=inverse,
                       z="not supplied by 2D minimap; original navigation/ground/capsule validation required")
        row["world_to_image"] = transform
        row["image_to_world"] = reverse
        row["minimap"].update(world_to_image=transform, image_to_world=reverse,
                              transform_status="verified_original_camera_bytes_runtime_marker_pending")
        row["world_bounds"] = dict(basis="uncropped_minimap_capture_rectangle; not a walkability or full-level bound",
                                   xy_aabb_cm=calibration["uncropped_texture_world_xy_aabb_cm"],
                                   xy_corners_cm=calibration["uncropped_texture_world_xy_corners_cm"],
                                   ground_z_cm=None)
        row["floor_metadata"] = calibration.get("floors", {"effective_runtime_values": "unknown"})
        row["evidence"]["minimap_projection"] = calibration
        row["validation"]["editor_coordinates"] = "static_source_camera_verified_runtime_marker_pending"
        row["validation"]["projection_round_trip_max_error_cm"] = max_error
    catalogue["revision"] = 2
    catalogue["projection_verification"] = dict(maps=len(catalogue["maps"]),
                                               independent_quaternion_evidence="work/training-map-minimap-static/projection-verification.json",
                                               native_builder_rva=document["world_to_uv_native_builder_rva"])
    return catalogue


def emit_native_catalog(catalogue):
    path = ROOT / "native/training_maps.h"
    source = path.read_text(encoding="utf8")
    begin, end = "// BEGIN GENERATED ORIGINAL MAP CATALOG", "// END GENERATED ORIGINAL MAP CATALOG"
    code = [begin, "// Source: reports/training-maps.json; exact original LevelStreaming package names."]
    for row in catalogue["maps"]:
        code.append("inline constexpr LayerSpec Layers_" + row["id"] + "[] = {")
        for layer in row["streaming_levels"]:
            if layer.get("parse_error") or not layer.get("package") or not layer.get("package_present"):
                raise ValueError("cannot emit unresolved original streaming whitelist")
            policy = layer_policy(layer, duel=row["kind"] == "duel")
            package = layer["package"]
            always = "true" if layer["native_class"].endswith("LevelStreamingAlwaysLoaded") else "false"
            code.append(f'    {{L"{package}", Action::{policy}, {always}}},')
        code.append("};")
    code.append("inline constexpr MapSpec Catalog[] = {")
    for row in catalogue["maps"]:
        identifier = row["id"]
        ffa = "true" if row["supports_ffa"] else "false"
        code.append(f'    {{"{identifier}", L"{row["world_path"]}", Layers_{identifier}, std::size(Layers_{identifier}), {ffa}}},')
    code.extend(["};", end])
    first = source.index(begin)
    last = source.index(end, first) + len(end)
    path.write_text(source[:first] + "\n".join(code) + source[last:], encoding="utf8")


def catalogue(source, discovery, output, images=True, emit_native=False):
    graph, indexed = package_store_graph(source)
    records, excluded, errors, world_evidence = [], [], [], []
    primary_records = list(discovery["map_primary_assets"])
    primary_records.append(source.describe("/Game/Maps/Duel/Duel_1/Skirmish_1_PrimaryAsset"))
    for primary in primary_records:
        code = primary["package"].split("/")[3]
        duel = primary["package"] == "/Game/Maps/Duel/Duel_1/Skirmish_1_PrimaryAsset"
        if duel:
            code = "Duel1"
        try:
            asset = decode_map_asset(source, primary)
            if code not in STANDARD_CODES and not duel:
                excluded.append(dict(id=code.lower(), primary_asset=asset,
                                     reason="training range, onboarding or non-standard mode; not included in 5v5 selection"))
                continue
            world_path = asset["world_path"]
            ui = decode_ui_asset(source, asset["ui_data"]["package"])
            display = ui["values"]["display_name"]
            table_path = display["table"].split(".", 1)[0]
            table = decode_string_table(source, table_path)
            label = table["entries"][display["key"]]
            world = source.describe(world_path)
            world_evidence.append(world)
            prefix = world_path.rsplit("/", 1)[0] + "/"
            maps = sorted({package_name(filename) for (key, extension), (store, filename, index) in source.index.items()
                           if extension == ".umap" and package_name(filename).startswith(prefix)})
            ffa = [name for name in maps if re.search(r"_modes?_ffa$", name, re.IGNORECASE)]
            team = [name for name in maps if "teamspawnpoint" in name.lower()]
            nav = [name for name in maps if "navmesh" in name.lower()]
            ffa_spawns = [spawn_anchors(source, name) for name in ffa]
            team_spawns = [spawn_anchors(source, name) for name in team]
            map_spawns = spawn_anchors(source, world_path) if duel else None
            nav_evidence = []
            for name in nav:
                metadata = source.describe(name)
                types = Counter(e["type"] for e in metadata["exports"])
                chunks = [dict(export=e["index"], name=e["name"], serialized_bytes=e["size"])
                          for e in metadata["exports"] if e["type"] == "/Script/NavigationSystem.RecastNavMeshDataChunk"]
                nav_evidence.append(dict(package=name, source_sha256=metadata["sha256"],
                                         recast_data_chunks=chunks, native_export_counts=dict(types)))
            textures = [name for name in world["imports"] if "/Minimap/" in name and "revealed" in name.lower()]
            texture_role = "revealed"
            if not textures:
                textures = [name for name in world["imports"] if "/Minimap/" in name and "fogofwar" in name.lower()]
                texture_role = "original_fog_of_war_layer"
            texture = textures[0] if len(textures) == 1 else None
            minimap = dict(source_texture_package=texture, source_texture_role=texture_role,
                           relative_path=None, world_to_image=None, image_to_world=None,
                           transform_status="pending original minimap capture matrix; MapUIData scalars are separately preserved",
                           ui_data_coordinate_scaling={key: ui["values"].get(key) for key in ("x_offset", "x_multiplier", "y_offset", "y_multiplier")})
            if texture:
                import textures as texture_decoder
                texture_decoder.TEXTURES = ROOT / "reports/training-map-images"
                if images:
                    decoded = texture_decoder.decode_texture(texture, source=source, max_size=1024)
                    minimap.update(relative_path=Path(decoded["texture"]).relative_to(ROOT).as_posix(),
                                   width=decoded["width"], height=decoded["height"], texture_evidence=decoded)
                else:
                    texture_zen = Zen(source.export(texture))
                    fmt, mips = texture_decoder.mipmaps(texture_zen)
                    mip = next((item for item in mips if max(item["width"], item["height"]) <= 1024), mips[-1])
                    minimap.update(width=mip["width"], height=mip["height"], image_export="disabled by --no-images", pixel_format=fmt)
            roots = set(maps) | {primary["package"], ui["package"]}
            if texture:
                roots.add(texture)
            dependencies = closure_evidence(source, roots, graph, indexed)
            anchors = map_spawns["anchors"] if duel else [anchor for group in ffa_spawns for anchor in group["anchors"]]
            streams = streaming_levels(source, world)
            alias = "斗牛一" if duel else EDITOR_ZH_ALIASES.get(code)
            record = dict(id=code.lower(), code_name=code, kind="duel" if duel else "standard_5v5", supports_ffa=not duel,
                          label="斗牛一" if duel else label, label_en=label,
                          label_zh=alias, label_zh_source="project_editor_alias; not read from original localized resource" if alias else None,
                          label_source=dict(string_table=table_path, key=display["key"], sha256=table["source_sha256"], culture="original source strings"),
                          map_asset_id=asset["map_asset_id"], map_primary_asset_path=primary["package"],
                          map_data_class=primary["package"] + "." + primary["package"].rsplit("/", 1)[-1] + "_C",
                          world_path=world_path, original_mode_tags=asset.get("mode_tags", []),
                          original_map_enum_value=asset.get("original_map_enum_value"),
                          minimap=minimap, world_to_image=None, image_to_world=None, world_bounds=None, floors=[],
                          spawn_anchors=anchors, spawn_anchor_basis="original_actor_root_center_cm",
                          spawn_metadata=dict(ffa=ffa_spawns, team=team_spawns, map=map_spawns),
                          navigation=dict(status="original_recast_data_present_runtime_pending" if any(e["recast_data_chunks"] for e in nav_evidence) else "not_verified",
                                          packages=nav_evidence,
                                          main_world_recast_actors=[dict(name=e["name"], export=e["index"], serialized_bytes=e["size"])
                                                                    for e in world["exports"] if e["type"] == "/Script/NavigationSystem.RecastNavMesh"],
                                          runtime_project_point=None, runtime_path_query=None),
                          obstacles=dict(status="original_collision_layers_present_runtime_pending",
                                         packages=[name for name in maps if any(term in name.lower() for term in ("bvpawn", "bv_pawn", "bvprojectile", "bv_projectile", "killvolume", "gameplay", "greybox"))],
                                         note="Original native map collision and navigation must be used. The 2D image is not an occupancy grid and provides no wall or walkability inference."),
                          sublevel_packages=maps, streaming_levels=streams, dependencies=dependencies,
                          streaming_policy=[dict(package=s["package"], action=layer_policy(s, duel), original_class=s["native_class"]) for s in streams],
                          main_world_hard_closure=closure_evidence(source, [world_path], graph, indexed),
                          validation=dict(static_metadata="verified_original_bytes", static_hard_closure="indexed" if not dependencies["missing_store_ids"] and not dependencies["absent_roots"] else "unresolved_ids",
                                          runtime="pending", fully_loadable=None, editor_coordinates="pending", navigation="pending", spawn_collision="pending"),
                          evidence=dict(primary=asset, ui_data=ui, main_world_sha256=world["sha256"]))
            records.append(record)
            print(f"{code}: {label}, {len(maps)} sublevels, {len(anchors)} original spawn anchors, {sum(len(e['recast_data_chunks']) for e in nav_evidence)} nav chunks", flush=True)
        except Exception as exc:
            errors.append(dict(package=primary["package"], error=type(exc).__name__ + ": " + str(exc)))
            print(code + ": " + errors[-1]["error"], flush=True)
    result = dict(schema_version=1, revision=1, generated_utc=datetime.now(timezone.utc).isoformat(),
                  source_read_only=str(SOURCE), source_containers=source.containers,
                  coordinate_convention=dict(units="Unreal world centimeters", up_axis="Z", rotations="Unreal pitch/yaw/roll degrees", image_origin="top left", normalized_image="u right, v down", spawn_basis="original PlayerStart root/capsule center; feet must not be assumed"),
                  maps=records, excluded=excluded, errors=errors,
                  limits=["Static container presence is not proof of complete runtime loading. Parent validation must fill runtime fields per map.",
                          "No navigation, collision, floor height or spawn-feet position is invented from a minimap pixel.",
                          "Source StringTable labels are English. A translated editor alias must be marked separately from original localized strings."])
    result = merge_projections(source, result)
    if emit_native:
        emit_native_catalog(result)
    native_header = ROOT / "native/training_maps.h"
    result["native_layer_adapter"] = dict(header="native/training_maps.h", header_sha256=hashlib.sha256(native_header.read_bytes()).hexdigest() if native_header.exists() else None,
                                          initialize="training_maps::Initialize(void* realWorld, const char* catalogMapId)",
                                          poll="training_maps::Poll() on original game thread; internally throttled to 200ms",
                                          readiness="training_maps::Ready(); actual original IsLevelLoaded/IsLevelVisible/GetLoadedLevel, not request completion",
                                          diagnostics="training_maps::State() / WriteDiagnostics(const wchar_t* filePath)",
                                          game_thread_precondition="original_players::Initialize has bound the owned game thread",
                                          native_abi="work/training-map-minimap-static/streaming-api.json",
                                          native_function_validation="UFunction.ParmsSize +be, Native flag +b8 &400, NativeFunc +e0 must equal exact pinned exec RVA",
                                          signature_validation_preserved=True,
                                          source_whitelist_entries=sum(len(row["streaming_levels"]) for row in records),
                                          always_loaded_unload="original setter is a no-op; only original visibility request is used for dedicated barriers; readiness waits actual visibility=false",
                                          shared_gameplay_barriers="keep shared Gameplay level and its original map collision; parent may run its existing original barrier actor lifecycle after streaming readiness",
                                          runtime_validation="pending; analyser does not build or run")
    result["ground_projection"] = dict(native_class="/Script/NavigationSystem.NavigationSystemV1", class_getter_rva="0x4f7fff0",
                                       function="/Script/NavigationSystem.NavigationSystemV1.K2_ProjectPointToNavigation",
                                       parameter_size=97, storage_size=104,
                                       offsets=dict(world=0,point_cm=8,projected_cm=32,nav_data=56,filter_class=64,query_extent_cm=72,return_bool=96),
                                       basis="Seed with original PlayerStart root center and actual original hero nav/capsule geometry. The projected Nav location still needs original supporting-ground and capsule-clearance validation before it is accepted as feet.",
                                       z_from_minimap=False, capsule_height_is_not_ground_detection=True,
                                       evidence="reports/training-bot-controller.json", runtime_validation="pending")
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name(output.name + ".tmp")
    temporary.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf8")
    temporary.replace(output)
    (WORK / "world-evidence.json").write_text(json.dumps(world_evidence, ensure_ascii=False, indent=2), encoding="utf8")
    print(json.dumps(dict(maps=len(records), errors=errors, output=str(output)), ensure_ascii=False))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage", choices=["discover", "catalogue"], default="catalogue")
    parser.add_argument("--output", type=Path, default=ROOT / "reports/training-maps.json")
    parser.add_argument("--no-images", action="store_true")
    parser.add_argument("--emit-native-catalog", action="store_true")
    arguments = parser.parse_args()
    source = Source()
    discovery = discover(source)
    if arguments.stage == "catalogue":
        catalogue(source, discovery, arguments.output, images=not arguments.no_images, emit_native=arguments.emit_native_catalog)


if __name__ == "__main__":
    main()
