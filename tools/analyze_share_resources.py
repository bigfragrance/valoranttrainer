"""Read-only package-dependency and original-block portable-resource planner.

Reads current v5 UTOCs and v3 container headers. Never writes the installation,
changes signatures, builds a container, launches the game, or emits a resource
copy. Exact physical range manifests are analysis output only.
"""
from __future__ import annotations

import argparse
import bisect
from collections import Counter, defaultdict, deque, OrderedDict
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "work/extract"))
from iostore import IoStore
from zen import names_at


def package_name(path):
    if path.startswith("../../../ShooterGame/Content/"):
        path = "/Game/" + path.split("Content/", 1)[1]
    elif path.startswith("../../../Engine/Content/"):
        path = "/Engine/" + path.split("Content/", 1)[1]
    elif "/Content/" in path:
        prefix, rest = path.split("/Content/", 1)
        mount = prefix.rsplit("/", 1)[-1]
        path = "/" + mount + "/" + rest
    else:
        return None
    return re.sub(r"\.(m\.ubulk|uasset|umap|ubulk|uptnl)$", "", path)


def merged_ranges(ranges):
    result = []
    for offset, size in sorted(ranges):
        end = offset + size
        if result and offset <= result[-1][0] + result[-1][1]:
            result[-1][1] = max(end, result[-1][0] + result[-1][1]) - result[-1][0]
        else:
            result.append([offset, size])
    return result


def consecutive(values):
    result = []
    for value in sorted(values):
        if result and value == result[-1][1] + 1:
            result[-1][1] = value
        else:
            result.append([value, value])
    return result


def parse_header(data):
    signature, version, container_id, count = struct.unpack_from("<IIQI", data)
    if (signature, version) != (0x496F436E, 3):
        raise ValueError("unsupported container header signature/version")
    cursor = 20
    ids = struct.unpack_from("<" + "Q" * count, data, cursor)
    cursor += 8 * count
    size = struct.unpack_from("<I", data, cursor)[0]
    cursor += 4
    entries = data[cursor:cursor + size]
    if len(entries) != size or size < count * 16:
        raise ValueError("truncated original package store")
    result = {}
    shaders = 0
    for index, pid in enumerate(ids):
        offset = index * 16
        number, relative, shader_number, shader_relative = struct.unpack_from("<IIII", entries, offset)
        begin = offset + relative
        if number and not (count * 16 <= begin <= begin + number * 8 <= size):
            raise ValueError("invalid package-import relative view")
        dependencies = struct.unpack_from("<" + "Q" * number, entries, begin) if number else ()
        shader_begin = offset + 8 + shader_relative
        if shader_number and not (count * 16 <= shader_begin <= shader_begin + shader_number * 20 <= size):
            raise ValueError("invalid shader-map-hash relative view")
        shaders += shader_number
        result[pid] = dependencies
    cursor += size
    optional_ids = 0
    if cursor + 4 <= len(data):
        optional_ids = struct.unpack_from("<I", data, cursor)[0]
    return result, {"signature": hex(signature), "version": version,
                    "container_id": hex(container_id), "package_count": count,
                    "entry_stride": 16, "store_array_bytes": size,
                    "hard_import_edges": sum(map(len, result.values())),
                    "shader_map_hash_references": shaders,
                    "optional_segment_package_count": optional_ids,
                    "remaining_header_bytes": len(data) - cursor,
                    "remaining_header_all_zero": not any(data[cursor:]),
                    "header_sha256": hashlib.sha256(data).hexdigest()}


class PackageNameReader:
    """Read only each package's serialized FName batch, with a bounded LRU."""
    def __init__(self):
        self.cache = OrderedDict()
        self.files = {}
        self.stats = Counter()

    def block(self, io, index):
        key = (io.path.name, index)
        if key in self.cache:
            self.cache.move_to_end(key)
            self.stats["cache_hits"] += 1
            return self.cache[key]
        physical, compressed, expanded, method = io.blocks[index]
        partition, offset = divmod(physical, io.partition_size)
        path = io.path.with_suffix(".ucas") if not partition else io.path.with_name(io.path.stem + f"_s{partition}.ucas")
        if path not in self.files:
            self.files[path] = path.open("rb")
        stream = self.files[path]
        stream.seek(offset)
        size = (compressed + 15) // 16 * 16 if io.block_hashes else compressed
        stored = stream.read(size)
        if len(stored) != size or (io.block_hashes and hashlib.sha1(stored).digest() != io.block_hashes[index]):
            raise ValueError("source compressed metadata block mismatch")
        packed = stored[:compressed]
        if io.methods[method] == "None":
            data = packed
        elif io.methods[method] == "Oodle":
            data = io._oodle(packed, expanded)
        elif io.methods[method] == "Zlib":
            import zlib
            data = zlib.decompress(packed)
        else:
            raise ValueError("unsupported metadata compression")
        if len(data) != expanded:
            raise ValueError("source metadata length mismatch")
        self.stats["physical_blocks_read"] += 1
        self.stats["physical_bytes_read"] += size
        self.stats["method_" + io.methods[method]] += 1
        self.cache[key] = data
        while len(self.cache) > 128:
            self.cache.popitem(last=False)
        return data

    def prefix(self, io, index, count):
        offset, length = io.ranges[index]
        if count > length:
            raise ValueError("package metadata exceeds chunk")
        result = bytearray()
        cursor = offset
        while len(result) < count:
            block_index, at = divmod(cursor, io.block_size)
            block = self.block(io, block_index)
            number = min(count - len(result), len(block) - at)
            if number <= 0:
                raise ValueError("invalid package metadata block interval")
            result += block[at:at + number]
            cursor += number
        return bytes(result)

    def names(self, io, index):
        prefix = self.prefix(io, index, 60)
        versioned = struct.unpack_from("<I", prefix)[0]
        number, string_bytes = struct.unpack_from("<II", prefix, 52)
        needed = 52 + 16 + 10 * number + string_bytes
        if versioned or number > 100000 or needed > 4 * 1024 * 1024:
            raise ValueError("unsupported or oversized package name batch")
        data = self.prefix(io, index, needed)
        names, end = names_at(data, 52)
        if end != needed:
            raise ValueError("name batch span mismatch")
        return names

    def close(self):
        for stream in self.files.values():
            stream.close()
        self.files.clear()
        self.cache.clear()


def analyze(source_root, output, soft_scan_limit=5000, reuse_soft_plan=None, core_ini=None,
            observed_runtime_logs=()):
    prior = json.loads(reuse_soft_plan.read_text(encoding="utf-8")) if reuse_soft_plan else None
    paks = source_root / "ShooterGame/Content/Paks"
    package_paths = {}
    path_ids = defaultdict(set)
    locations = defaultdict(list)
    dependencies = defaultdict(set)
    ordered_dependencies = {}
    sources = {}
    group_ids = defaultdict(set)
    metadata = {}

    def groups(path):
        result = []
        if path.startswith("/Game/Maps/Duel/Duel_1/"):
            result.append("duel1_all_authored_sublevels")
        for hero in ("Wushu", "Phoenix", "Thorne"):
            prefix = f"/Game/Characters/{hero}/"
            if path.startswith(prefix + "S0/") or (path.startswith(prefix) and "/" not in path[len(prefix):]):
                result.append(hero.lower() + "_default_hero_and_s0")
        if path.startswith("/Game/Equippables/Guns/Rifles/AK/Champions/"):
            result.append("ak_champions_full_family")
        if path.startswith("/Game/Equippables/Guns/Attachments/RedDot/Champions/"):
            result.append("champions_reflex")
        if path.startswith(("/Game/UI/InGame/HUD/", "/Game/UI/Screens/InGame/HUD/")):
            result.append("original_hud")
        if path.startswith("/Game/UI/") and ("Settings" in path or "/OptionsMenu/" in path):
            result.append("original_settings_candidates")
        if path.startswith("/Game/Characters/Wushu/S0/Ability_4/") or path.startswith("/Game/Characters/Wushu/S0/Ability_Q/"):
            result.append("jett_cq")
        if path.startswith("/Game/Globals/"):
            result.append("native_globals_blueprints")
        if path.startswith("/Game/Settings/"):
            result.append("complete_settings_database")
        if path.startswith("/Game/UI/InGame/QuickComms/"):
            result.append("complete_quickcomms_definitions")
        name = path.rsplit("/", 1)[-1]
        if path.startswith("/Engine/"):
            result.append("all_engine_bootstrap")
        if "PrimaryAsset" in name or name.endswith("UIData"):
            result.append("catalog_named_metadata_guard")
        return result

    for toc in sorted(paks.glob("*.utoc")):
        io = IoStore(toc)
        name = toc.name
        kinds = Counter(bytes.fromhex(cid)[-1] for cid in io.ids)
        associated = [f for f in paks.iterdir() if f.is_file() and
                      (f.stem == toc.stem or (f.suffix == ".ucas" and f.stem.startswith(toc.stem + "_s")))]
        main_names = {}
        logical = 0
        for path, index in io.files.items():
            cid = bytes.fromhex(io.ids[index])
            if cid[-1] != 1:
                continue
            normalized = package_name(path)
            if normalized is None:
                continue
            pid = int.from_bytes(cid[:8], "little")
            main_names[pid] = normalized
            package_paths.setdefault(pid, normalized)
            path_ids[normalized.casefold()].add(pid)
            locations[pid].append((name, index))
            logical += io.ranges[index][1]
            for group in groups(normalized):
                group_ids[group].add(pid)
            if "/Plugins/" in path and "/Content/" in path:
                group_ids["all_engine_and_project_plugin_bootstrap"].add(pid)
        headers = [i for i, cid in enumerate(io.ids) if bytes.fromhex(cid)[-1] == 6]
        detail = None
        if headers:
            if len(headers) != 1:
                raise ValueError("multiple container headers")
            io.files["__analysis_container_header__"] = headers[0]
            meta = io.metadata("__analysis_container_header__")
            graph, detail = parse_header(io.read("__analysis_container_header__"))
            detail["toc_entry_index"] = headers[0]
            detail["source_chunk_id"] = io.ids[headers[0]]
            detail["compression_methods"] = sorted({b["method"] for b in meta["blocks"]})
            for pid, imported in graph.items():
                dependencies[pid].update(imported)
                ordered_dependencies.setdefault(pid, imported)
            if set(main_names) - set(graph):
                raise ValueError("directory package missing from original store")
        sources[name] = {"toc_sha256": hashlib.sha256(io.data).hexdigest(),
                         "toc_bytes": len(io.data), "flags": io.flags,
                         "signed_block_hashes_present": bool(io.block_hashes),
                         "chunk_types": dict(kinds), "indexed_packages": len(main_names),
                         "main_package_logical_bytes": logical,
                         "physical_files": [{"name": f.name, "bytes": f.stat().st_size} for f in sorted(associated)],
                         "whole_container_bytes": sum(f.stat().st_size for f in associated),
                         "package_store": detail}
        metadata[name] = io
        print(f"indexed {name}: {len(main_names)} packages", flush=True)

    companion_owners = {}
    companion_validation = {"indexed_bulk_chunks": 0, "same_stem_owner_found": 0,
                            "chunk_first8_equals_owner_package_id": 0,
                            "different_first8_companions": [], "owner_name_unmatched": []}
    for toc, io in metadata.items():
        for path, index in io.files.items():
            cid = bytes.fromhex(io.ids[index])
            if cid[-1] not in (2, 3, 4):
                continue
            companion_validation["indexed_bulk_chunks"] += 1
            normalized = package_name(path)
            owners = path_ids.get(normalized.casefold(), ()) if normalized else ()
            if owners:
                companion_validation["same_stem_owner_found"] += 1
                companion_owners[(toc, index)] = set(owners)
                if int.from_bytes(cid[:8], "little") in owners:
                    companion_validation["chunk_first8_equals_owner_package_id"] += 1
                else:
                    companion_validation["different_first8_companions"].append(
                        {"toc": toc, "entry_index": index, "chunk_id": io.ids[index],
                         "owner_package": normalized, "owner_package_ids": [f"{pid:016x}" for pid in owners]})
            else:
                companion_validation["owner_name_unmatched"].append(
                    {"toc": toc, "entry_index": index, "chunk_id": io.ids[index], "path": path})

    native_roots = set()
    native_evidence = []
    unresolved_native = []
    for header in sorted((ROOT / "native").glob("original*.h")):
        text = header.read_text(encoding="utf-8", errors="replace")
        for match in re.finditer(r'(?:L)?"(/(?:Game|Engine)/[^"\s\\]+)"', text):
            path = match.group(1).split(".", 1)[0]
            found = path_ids.get(path.casefold())
            if found:
                native_roots.update(found)
                native_evidence.append({"header": str(header.relative_to(ROOT)),
                                        "line": text.count("\n", 0, match.start()) + 1,
                                        "package": path})
            else:
                unresolved_native.append(path)
    group_ids["literal_native_package_references"] = native_roots
    # Concrete current ABI roots complement folder-level authored families.
    explicit = ["/Game/Equippables/Guns/Rifles/AK/AssaultRifle_AK",
                "/Game/Equippables/Guns/Rifles/AK/AKPrimaryAsset",
                "/Game/Equippables/Guns/Rifles/AK/Attachments/Reflex/RedDot/Attachment_AK_ReflexPrimaryAsset",
                "/Game/Equippables/Guns/_Core/ScopeComponent",
                "/Game/Equippables/Guns/Attachments/RedDot/Attachment_Base_Reflex_RedDot",
                "/Game/Equippables/Guns/_Core/Attachments/Sockets/AttachmentSocket_ReflexPrimaryAsset"]
    group_ids["explicit_ak_scope_roots"].update(pid for path in explicit for pid in path_ids.get(path.casefold(), ()))
    ini_evidence = {"archive": str(core_ini.resolve()) if core_ini else None,
                    "active_path_references": [], "inactive_or_scan_path_references": []}
    if core_ini:
        from ini_bootstrap import decode
        archive_bytes = core_ini.read_bytes()
        snapshot = decode(archive_bytes)
        ini_evidence["sha256"] = hashlib.sha256(archive_bytes).hexdigest()
        for category in snapshot["categories"]:
            sections = category["file"]["sections"]
            explicit_game_override = any(key == "LocalMapOptions" and "?game=" in value
                                         for _, section in sections for key, value in section["pairs"])
            for section_name, section in sections:
                if section_name == "/Script/Engine.AssetManagerSettings":
                    ini_evidence["asset_manager_settings"] = {
                        "pair_count": len(section["pairs"]),
                        "primary_asset_type_definitions": [
                            {"key": key,
                             "primary_type": re.findall(r'PrimaryAssetType="([^"]+)"', value),
                             "base_class": re.findall(r'AssetBaseClass="([^"]+)"', value),
                             "asset_or_directory_paths": re.findall(r'/(?:Game|Engine)/[A-Za-z0-9_./-]+', value)}
                            for key, value in section["pairs"] if key == "PrimaryAssetTypesToScan"]}
                for key, value in section["pairs"]:
                    for match in re.finditer(r"/(?:Game|Engine)/[A-Za-z0-9_./-]+", value):
                        path = match.group().split(".", 1)[0].rstrip("/")
                        ids = path_ids.get(path.casefold(), ())
                        inactive = ("Editor" in key or "ServerDefault" in key or "ServerGameMode" in key or
                                    "MapsToCook" in key or "AssetManagerSettings" in section_name or
                                    (key == "GlobalDefaultGameMode" and explicit_game_override))
                        evidence = {"category": category["category"], "section": section_name,
                                    "key": key, "package_or_directory": path,
                                    "matched_package_ids": [f"{pid:016x}" for pid in sorted(ids)]}
                        if inactive or not ids:
                            evidence["qualification"] = "server/editor/cook/catalog scan or unmatched directory; not equated with active runtime load"
                            ini_evidence["inactive_or_scan_path_references"].append(evidence)
                        else:
                            group_ids["effective_core_ini_active_paths"].update(ids)
                            ini_evidence["active_path_references"].append(evidence)

    runtime_log_evidence = []
    previous_profile = (prior or {}).get("profiles", {}).get("observed_catalog_closure") or (prior or {}).get("profiles", {}).get("complete_settings_engine_plugins_active_soft_catalog", {})
    prior_observed = (prior or {}).get("profiles", {}).get("observed_catalog_closure", {})
    group_ids["preserved_prior_observed_closure"].update(int(pid, 16) for pid in prior_observed.get("selected_package_ids", []))
    required_original_blocks = defaultdict(set)
    if prior:
        for evidence in prior.get("observed_runtime_log_evidence", []):
            for stem, blocks in evidence.get("signature_error_blocks", {}).items():
                required_original_blocks[stem + ".utoc"].update(blocks)
        for toc, blocks in prior.get("observed_required_original_block_ids", {}).items():
            required_original_blocks[toc].update(blocks)
    previous_manifests = previous_profile.get("offset_preserving_sparse_candidate", {}).get("manifest", [])
    previous_intervals = defaultdict(list)
    for manifest in previous_manifests:
        previous_intervals[manifest["toc"]].extend(manifest["block_index_intervals_inclusive"])
    for intervals in previous_intervals.values():
        intervals.sort()
    for path in observed_runtime_logs:
        log_bytes = path.read_bytes()
        text = log_bytes.decode("utf-8", errors="replace")
        failed = sorted(set(value.split(".", 1)[0] for value in re.findall(r"FailedRead:\s+(/[^\s(]+)", text)))
        prefixes = Counter("/".join(value.split("/")[:5]) for value in failed)
        errors = defaultdict(set)
        for container, block in re.findall(r"Signature error detected in container '([^']+)' at block index '(\d+)'", text):
            errors[container].add(int(block))
        failed_roots = set()
        unmatched = []
        for name in failed:
            ids = path_ids.get(name.casefold(), ())
            if ids:
                failed_roots.update(ids)
            else:
                unmatched.append(name)
        block_mappings = []
        block_roots = set()
        already_in_previous = 0
        for stem, blocks in errors.items():
            toc = stem + ".utoc"
            required_original_blocks[toc].update(blocks)
            if toc not in metadata:
                continue
            io = metadata[toc]
            ordered_blocks = sorted(blocks)
            mapped = {block: [] for block in ordered_blocks}
            for index, id_hex in enumerate(io.ids):
                offset, length = io.ranges[index]
                first = offset // io.block_size
                last = (offset + length + io.block_size - 1) // io.block_size
                at = bisect.bisect_left(ordered_blocks, first)
                cid = bytes.fromhex(id_hex)
                while at < len(ordered_blocks) and ordered_blocks[at] < last:
                    pid = int.from_bytes(cid[:8], "little")
                    mapped[ordered_blocks[at]].append({"entry_index": index, "chunk_id": id_hex,
                                                       "chunk_type": cid[-1], "package_id": f"{pid:016x}",
                                                       "package": package_paths.get(pid)})
                    if cid[-1] in (1, 2, 3, 4):
                        block_roots.add(pid)
                    at += 1
            old = previous_intervals.get(toc, [])
            starts = [pair[0] for pair in old]
            for block in ordered_blocks:
                position = bisect.bisect_right(starts, block) - 1
                included = position >= 0 and block <= old[position][1]
                already_in_previous += included
                physical, compressed, expanded, method = io.blocks[block]
                block_mappings.append({"toc": toc, "block_index": block,
                                       "physical_offset": physical, "compressed_bytes": compressed,
                                       "signed_stored_bytes": (compressed + 15) // 16 * 16 if io.block_hashes else compressed,
                                       "was_in_previous_settings_profile": included,
                                       "was_in_previous_selected_profile": included,
                                       "associated_original_chunks": mapped[block]})
        observed_roots = failed_roots | block_roots
        group_ids["actual_runtime_catalog_requests"].update(observed_roots)
        runtime_log_evidence.append({"log": str(path.resolve()), "sha256": hashlib.sha256(log_bytes).hexdigest(),
                                     "failed_read_line_count": text.count("FailedRead:"),
                                     "failed_packages": failed, "distinct_failed_prefixes": dict(prefixes),
                                     "failed_path_package_roots": len(failed_roots),
                                     "signature_mapped_package_roots": len(block_roots),
                                     "combined_explicit_package_roots": len(observed_roots),
                                     "unmatched_failed_paths": unmatched,
                                     "signature_error_blocks": {key: sorted(value) for key, value in errors.items()},
                                     "reported_blocks_already_in_previous_settings_profile": already_in_previous,
                                     "signature_block_package_evidence": block_mappings})

    def closure(seeds):
        visited = set(seeds)
        queue = deque(seeds)
        missing = set()
        parents = {}
        while queue:
            pid = queue.popleft()
            for child in dependencies.get(pid, ()):
                if child not in dependencies:
                    missing.add(child)
                if child not in visited:
                    visited.add(child)
                    parents[child] = pid
                    queue.append(child)
        return visited, missing, parents

    feature_groups = [name for name in group_ids if name not in
                      {"catalog_named_metadata_guard", "all_engine_bootstrap", "effective_core_ini_active_paths",
                       "all_engine_and_project_plugin_bootstrap", "complete_settings_database"}]
    feature_groups = [name for name in feature_groups if name not in
                      {"actual_runtime_catalog_requests", "preserved_prior_observed_closure",
                       "complete_quickcomms_definitions"}]
    feature_seeds = set().union(*(group_ids[g] for g in feature_groups))
    guarded_seeds = feature_seeds | group_ids["catalog_named_metadata_guard"]
    core_tocs = {"pakchunk0-WindowsClient.utoc", "pakchunk1-WindowsClient.utoc"}
    core_ids = {pid for pid, places in locations.items() if any(toc in core_tocs for toc, _ in places)}
    active, _, _ = closure(feature_seeds)
    initial_active = set(active)
    reused_soft = None
    if prior:
        same_tocs = set(prior["container_evidence"]) == set(sources) and all(
            prior["container_evidence"][name]["toc_sha256"] == value["toc_sha256"]
            for name, value in sources.items())
        if not same_tocs:
            raise ValueError("cached soft plan source UTOC identities differ")
        reused_soft = prior.get("soft_dependency_name_scan", {})
        cached_ids = {int(pid, 16) for pid in reused_soft.get("new_package_witnesses", {})}
        active.update(closure(cached_ids)[0])
    def scan_key(pid):
        if pid not in locations:
            return (2, "", 0)
        toc, index = locations[pid][0]
        return (0 if pid in feature_seeds else 1, toc, metadata[toc].ranges[index][0])
    queue = deque(sorted(active, key=scan_key))
    scanned = set()
    soft_witnesses = {}
    soft_errors = []
    soft_unmatched = set()
    reader = PackageNameReader()
    try:
        while queue and len(scanned) < soft_scan_limit:
            pid = queue.popleft()
            if pid in scanned or pid not in locations:
                continue
            scanned.add(pid)
            toc, index = locations[pid][0]
            try:
                names = reader.names(metadata[toc], index)
                for value in names:
                    if not value.startswith("/"):
                        continue
                    path = value.split(".", 1)[0]
                    targets = path_ids.get(path.casefold())
                    if not targets:
                        if value.startswith(("/Game/", "/Engine/")):
                            soft_unmatched.add(path)
                        continue
                    for target in targets - active:
                        extra, _, _ = closure({target})
                        new = extra - active
                        active.update(new)
                        for item in new:
                            soft_witnesses.setdefault(item, {"path_valued_fname": path,
                                                            "found_in": package_paths.get(pid),
                                                            "trigger_package": package_paths.get(target)})
                        queue.extend(sorted(new, key=scan_key))
            except Exception as exc:
                soft_errors.append({"package": package_paths.get(pid), "error": type(exc).__name__})
            if len(scanned) % 1000 == 0:
                print(f"active path-valued FName scan: {len(scanned)} scanned, {len(active)} selected", flush=True)
    finally:
        reader.close()
    soft_report = {"active_initial_hard_packages": len(initial_active),
                   "active_name_batches_scanned": len(scanned),
                   "active_packages_after_path_closure": len(active),
                   "additional_active_packages": len(active - initial_active),
                   "scan_limit": soft_scan_limit,
                   "active_seed_packages": len(feature_seeds),
                   "active_seed_name_batches_scanned": len(feature_seeds & scanned),
                   "active_seed_name_batches_remaining": len((feature_seeds - scanned) & set(locations)),
                   "unscanned_active_package_count": len((active - scanned) & set(locations)),
                   "complete_for_selected_name_batches": not queue,
                   "source_read_statistics": dict(reader.stats),
                   "name_batch_errors": soft_errors,
                   "unmatched_path_valued_fnames": sorted(soft_unmatched),
                   "new_package_witnesses": {f"{pid:016x}": value for pid, value in soft_witnesses.items()},
                   "qualification": "Conservative package-path FName candidates include hard/soft/editor/default references. This is not a claim that all such paths execute. FString/dynamic-generated paths are not fully decoded. Catalog-only CDO soft references are intentionally not expanded into every unused skin model."}
    if reused_soft:
        old_witnesses = reused_soft.get("new_package_witnesses", {})
        soft_report["new_package_witnesses"] = {**old_witnesses, **soft_report["new_package_witnesses"]}
        origin = reused_soft.get("reused_verified_name_batch_scan", {}) if not reused_soft.get("active_name_batches_scanned") else {}
        soft_report["reused_verified_name_batch_scan"] = {
            "source_plan_revision": prior.get("plan_revision"), "all_source_toc_sha256_equal": True,
            "prior_active_name_batches_scanned": origin.get("prior_active_name_batches_scanned", reused_soft.get("active_name_batches_scanned")),
            "prior_active_seed_name_batches_scanned": origin.get("prior_active_seed_name_batches_scanned", reused_soft.get("active_seed_name_batches_scanned")),
            "prior_name_batch_errors": origin.get("prior_name_batch_errors", len(reused_soft.get("name_batch_errors", []))),
            "cached_soft_derived_ids_retained": len(old_witnesses),
            "qualification": "Reuses immutable source metadata relationships; does not claim a new full soft-path scan."}
    profiles = {}
    profile_sets = {}
    for name, seeds in [("features_hard_closure", feature_seeds),
                        ("catalog_guarded_hard_closure", guarded_seeds),
                        ("features_soft_names_plus_catalog_guard", active | group_ids["catalog_named_metadata_guard"]),
                        ("engine_bootstrap_plus_active_soft_and_catalog",
                         active | group_ids["catalog_named_metadata_guard"] |
                         group_ids["all_engine_bootstrap"] | group_ids["effective_core_ini_active_paths"]),
                        ("engine_plugins_bootstrap_plus_active_soft_and_catalog",
                         active | group_ids["catalog_named_metadata_guard"] |
                         group_ids["all_engine_bootstrap"] | group_ids["effective_core_ini_active_paths"] |
                         group_ids["all_engine_and_project_plugin_bootstrap"]),
                        ("complete_settings_engine_plugins_active_soft_catalog",
                         active | group_ids["catalog_named_metadata_guard"] |
                         group_ids["all_engine_bootstrap"] | group_ids["effective_core_ini_active_paths"] |
                         group_ids["all_engine_and_project_plugin_bootstrap"] |
                         group_ids["complete_settings_database"]),
                        ("observed_catalog_closure",
                         active | group_ids["catalog_named_metadata_guard"] |
                         group_ids["all_engine_bootstrap"] | group_ids["effective_core_ini_active_paths"] |
                         group_ids["all_engine_and_project_plugin_bootstrap"] |
                         group_ids["complete_settings_database"] | group_ids["actual_runtime_catalog_requests"] |
                         group_ids["complete_quickcomms_definitions"] | group_ids["preserved_prior_observed_closure"]),
                        ("whole_core_plus_features", guarded_seeds | core_ids)]:
        selected, missing, parents = closure(seeds)
        profile_sets[name] = selected
        containers = sorted({toc for pid in selected for toc, _ in locations.get(pid, ())})
        reason = {}
        for toc in containers:
            example = next(pid for pid in selected if any(t == toc for t, _ in locations.get(pid, ())))
            chain = [example]
            while chain[-1] in parents and len(chain) < 32:
                chain.append(parents[chain[-1]])
            reason[toc] = [package_paths.get(pid, f"id:{pid:016x}") for pid in reversed(chain)]
        profiles[name] = {"seed_packages": len(seeds), "selected_packages": len(selected),
                           "hard_import_edges_followed": sum(len(dependencies.get(pid, ())) for pid in selected),
                           "unresolved_import_ids": [f"{pid:016x}" for pid in sorted(missing)],
                           "required_package_containers": containers, "container_witness_chains": reason,
                           "selected_package_ids": [f"{pid:016x}" for pid in sorted(selected)]}

    group_report = {}
    for name, seeds in sorted(group_ids.items()):
        selected, missing, _ = closure(seeds)
        counts = Counter(toc for pid in seeds for toc, _ in locations.get(pid, ()))
        group_report[name] = {"seed_packages": len(seeds), "direct_containers": dict(counts),
                              "hard_closure_packages": len(selected),
                              "hard_closure_containers": sorted({toc for pid in selected for toc, _ in locations.get(pid, ())}),
                              "unresolved_import_count": len(missing),
                              "sample_roots": [package_paths[p] for p in sorted(seeds)[:6] if p in package_paths]}

    # Resolve existing-source missing IDs by the original package's own Zen
    # imported-name table, not by guessing a hash or filename. Imported order is
    # accepted only if every known paired ID/name in that sample agrees.
    all_missing = {int(pid, 16) for profile in profiles.values() for pid in profile["unresolved_import_ids"]}
    missing_evidence = {pid: {"package_id": f"{pid:016x}", "referenced_by": [],
                               "resolved_name_evidence": []} for pid in all_missing}
    for parent, imported in dependencies.items():
        for child in imported & all_missing:
            missing_evidence[child]["referenced_by"].append(package_paths.get(parent, f"id:{parent:016x}"))
    sample_parents = []
    for missing in sorted(all_missing):
        candidates = [parent for parent, imported in dependencies.items()
                      if missing in imported and parent in locations]
        candidates.sort(key=lambda parent: metadata[locations[parent][0][0]].ranges[locations[parent][0][1]][1])
        sample_parents.extend(candidates[:2])
    sampled = set()
    dependency_name_samples = []
    for parent in dict.fromkeys(sample_parents):
        if len(sampled) >= 40:
            break
        toc, index = locations[parent][0]
        io = metadata[toc]
        if io.ranges[index][1] > 8 * 1024 * 1024:
            continue
        sampled.add(parent)
        label = "__analysis_import_names__"
        io.files[label] = index
        try:
            data = io.read(label)
            header = struct.unpack_from("<13I", data)
            imported_names, at = names_at(data, header[12])
            numbers = struct.unpack_from("<" + "I" * len(imported_names), data, at)
            imported_names = [name + ("_" + str(number - 1) if number else "")
                              for name, number in zip(imported_names, numbers)]
            ordered = ordered_dependencies[parent]
            same_count = len(imported_names) == len(ordered)
            checked = [(pid, name) for pid, name in zip(ordered, imported_names) if pid in package_paths]
            agreeing = sum(package_paths[pid].casefold() == name.casefold() for pid, name in checked)
            accepted = same_count and (bool(checked) or len(ordered) == 1) and agreeing == len(checked)
            sample = {"parent_package": package_paths.get(parent), "container": toc,
                      "source_entry_index": index, "imported_names_offset": header[12],
                      "package_store_import_count": len(ordered), "zen_name_count": len(imported_names),
                      "known_pairs_checked": len(checked), "known_pairs_agreeing": agreeing,
                      "ordered_mapping_accepted": accepted}
            dependency_name_samples.append(sample)
            if accepted:
                for import_index, (pid, name) in enumerate(zip(ordered, imported_names)):
                    if pid in missing_evidence:
                        missing_evidence[pid]["resolved_name_evidence"].append(
                            {"name": name, "parent": sample["parent_package"],
                             "container": toc, "import_index": import_index})
        except Exception as exc:
            dependency_name_samples.append({"parent_package": package_paths.get(parent),
                                            "container": toc, "error": type(exc).__name__})

    paks_files = [p for p in paks.iterdir() if p.is_file()]
    passthrough = [p for p in paks_files if p.suffix != ".ucas"]
    passthrough_bytes = sum(p.stat().st_size for p in passthrough)
    for name, selected in profile_sets.items():
        manifests = []
        selected_types = Counter()
        for toc_name, io in metadata.items():
            block_indices = set()
            selected_chunks = 0
            for index, id_hex in enumerate(io.ids):
                cid = bytes.fromhex(id_hex)
                kind, pid = cid[-1], int.from_bytes(cid[:8], "little")
                # Include every data/bulk/optional/memory-mapped index for a
                # selected package ID, even if not visible in directory paths.
                # Retain ALL script/header/shader chunks: shader-code IDs are
                # not package IDs and their archive indirection is not pruned.
                keep = ((kind in (1, 2, 3, 4) and pid in selected) or kind in (5, 6, 8, 9) or
                        bool(companion_owners.get((toc_name, index), set()) & selected))
                if not keep:
                    continue
                selected_chunks += 1
                selected_types[kind] += 1
                offset, length = io.ranges[index]
                block_indices.update(range(offset // io.block_size,
                                           (offset + length + io.block_size - 1) // io.block_size))
            if name == "observed_catalog_closure":
                block_indices.update(required_original_blocks.get(toc_name, ()))
            by_file = defaultdict(list)
            by_block = defaultdict(list)
            for bi in sorted(block_indices):
                physical, compressed, expanded, method = io.blocks[bi]
                size = (compressed + 15) // 16 * 16 if io.block_hashes else compressed
                partition, offset = divmod(physical, io.partition_size)
                if offset + size > io.partition_size:
                    raise ValueError("compressed block crosses partition")
                filename = io.path.stem + (f"_s{partition}" if partition else "") + ".ucas"
                by_file[filename].append((offset, size))
                by_block[filename].append(bi)
            for filename, ranges in sorted(by_file.items()):
                physical = merged_ranges(ranges)
                original_size = (paks / filename).stat().st_size
                if any(o + n > original_size for o, n in physical):
                    raise ValueError("selected physical range exceeds original UCAS")
                units64 = merged_ranges(((o // 65536 * 65536,
                                          ((o + n + 65535) // 65536 - o // 65536) * 65536)
                                         for o, n in physical))
                manifests.append({"file": filename, "original_logical_file_bytes": original_size,
                                  "toc": toc_name, "toc_sha256": sources[toc_name]["toc_sha256"],
                                  "original_stored_payload_bytes": sum(n for _, n in physical),
                                  "estimated_sparse_64k_allocation_bytes": sum(n for _, n in units64),
                                  "selected_original_blocks": len(by_block[filename]),
                                  "block_index_intervals_inclusive": consecutive(by_block[filename]),
                                  "physical_ranges_offset_length": physical})
        prof = profiles[name]
        whole_keep = set(prof["required_package_containers"]) | {"global.utoc", "en_US_Audio-WindowsClient.utoc", "zh_TW_Text-WindowsClient.utoc"}
        # Existing nontrivial PAKs contain loose config/registry/audio resources;
        # they are retained whole because no PAK dependency prune is proven.
        whole_keep.update(toc for toc, s in sources.items() if any(f["name"].endswith(".pak") and f["bytes"] > 4096 for f in s["physical_files"]))
        removable = sorted(set(sources) - whole_keep)
        whole_saved = sum(sources[toc]["whole_container_bytes"] for toc in removable)
        prof["whole_container_candidate"] = {"keep_containers": sorted(whole_keep),
                                               "exclude_containers_if_validated": removable,
                                               "potential_saved_bytes": whole_saved,
                                               "retained_paks_bytes": sum(f.stat().st_size for f in paks_files) - whole_saved,
                                               "status": "hard-dependency candidate; soft/dynamic/runtime validation required"}
        payload = sum(m["original_stored_payload_bytes"] for m in manifests)
        allocated = sum(m["estimated_sparse_64k_allocation_bytes"] for m in manifests)
        prof["offset_preserving_sparse_candidate"] = {"original_ucas_logical_bytes": sum(p.stat().st_size for p in paks_files if p.suffix == ".ucas"),
                                                      "original_compressed_ranges_payload_bytes": payload,
                                                      "estimated_sparse_64k_allocation_bytes": allocated,
                                                      "all_original_toc_pak_sig_passthrough_bytes": passthrough_bytes,
                                                      "estimated_resource_bundle_payload_before_zip": payload + passthrough_bytes,
                                                      "included_chunk_types": dict(selected_types),
                                                      "manifest": manifests}

    old = ROOT / "work/extract/inventory.json"
    old_names = []
    if old.exists():
        # Cheap bounded provenance check; current TOCs remain authoritative.
        old_names = re.findall(r'"container"\s*:\s*"([^\"]+)"', old.read_text(encoding="utf-8"))
    all_source_bytes = sum(p.stat().st_size for p in source_root.rglob("*") if p.is_file())
    paks_bytes = sum(p.stat().st_size for p in paks_files)
    warning_queries = ["/Engine/EngineMaterials/DefaultTextMaterialOpaque",
                       "/Engine/EngineMaterials/WeightMapPlaceholderTexture"]
    baseline_warning_evidence = [{"queried_package": query,
                                 "exact_package_present_in_current_source": bool(path_ids.get(query.casefold())),
                                 "related_current_packages": sorted({name for name in package_paths.values()
                                                                       if "DefaultTextMaterial" in name}),
                                 "qualification": "Absence is from all current source UTOC directories; no replacement package is fabricated. Loose PAK exports were not separately indexed."}
                                for query in warning_queries]
    report = {"schema_version": 1, "plan_revision": 7, "generated_utc": datetime.now(timezone.utc).isoformat(),
              "scope": "Read-only current source TOCs/container headers and current native path literals. No resource copy, container rewrite, game launch, signature changes, or verification bypass.",
              "source_root": str(source_root.resolve()), "source_total_bytes": all_source_bytes,
              "paks_total_bytes": paks_bytes, "non_paks_bytes": all_source_bytes - paks_bytes,
              "current_container_count": len(sources), "packages_in_original_store": len(dependencies),
              "hard_import_edges_total": sum(map(len, dependencies.values())),
              "old_inventory_containers_absent_from_current_source": sorted(set(old_names) - set(sources)),
              "container_evidence": sources, "root_group_evidence": group_report,
              "native_literal_evidence": native_evidence,
              "native_literal_unresolved_paths": sorted(set(unresolved_native)),
              "missing_dependency_evidence": list(missing_evidence.values()),
              "zen_import_name_validation": dependency_name_samples,
              "soft_dependency_name_scan": soft_report,
              "effective_core_ini_evidence": ini_evidence,
              "baseline_warning_evidence": baseline_warning_evidence,
              "observed_runtime_log_evidence": runtime_log_evidence,
              "observed_required_original_block_ids": {toc: sorted(blocks) for toc, blocks in required_original_blocks.items()},
              "bulk_companion_identity_validation": companion_validation,
              "profiles": profiles,
              "sparse_format_requirements": [
                  "Preserve every original UTOC byte, signature, physical block offset, chunk ID, logical chunk range, compression method and original UCAS logical file length.",
                  "Write complete original compressed blocks, including signed16-byte padding. Original per-block SHA1 comes from the unchanged UTOC; this planner does not re-encode or regenerate signatures.",
                  "Retain all package data/bulk/optional/memory-mapped chunk indices sharing a selected package ID, every container header, global script objects and all current shader libraries/code chunks.",
                  "Ship explicit offset/range payloads and a manifest, then materialize sparse UCAS on a filesystem supporting sparse files. Ordinary ZIP extraction of logical zero-filled UCAS can allocate the entire35GB again.",
                  "Missing selected/dynamic chunks will read as zeros and fail existing checks; do not weaken those checks. Expand the candidate using actual missing-read evidence and repeat full feature verification.",
                  "No source format evidence requires recompressing retained blocks. Whole UCAS logical size stays original; mount-time whole-file verification or full-file background readers remain runtime questions.",
                  "Directly bundle source ranges without staging a second full35GB copy.64KiB allocation is a conservative estimate, not measured NTFS allocation or ZIP size."],
              "coverage_limits": [
                  "PackageStore imports are hard dependencies only. Soft paths, streamed sublevels, runtime-generated names, materialization choices and current AssetRegistry/native catalog enumeration may require additional packages.",
                  "Whole Duel1 authored folder, selected hero S0 families, Champions/Reflex families, HUD/options candidates and native literals are conservative seeds, not proof of runtime liveness for every package.",
                  "PrimaryAsset/UIData naming identifies an overinclusive catalog guard seed. Exact AssetRegistry class-tag enumeration has not been decoded; it is not a complete proven catalog list.",
                  "Whole_core_plus_features retains all packages in original chunks0/1 to reduce bootstrap uncertainty, but no candidate is called a verified portable release.",
                  "Every original non-UCAS PAK/TOC/SIG is retained in sparse estimates. Loose Wwise/audio/config/AssetRegistry files in PAKs were not dependency-pruned.",
                  "All shader chunks are retained because per-package shader-map-to-code pruning is not established.",
                  "Container block hashes/signatures remain unchanged. Selected payload bytes and actual archive size were not copied or newly hashed by this planning-only operation."],
              "verification_boundary": [
                  "Relocated launch with no sourceF path/junction/network/account dependency and original signed TOCs intact.",
                  "Duel1 all streaming sublevels/materials/lighting/collision; Wushu/Phoenix/Thorne bodies/animations/outlines and repeated respawns.",
                  "Original AK Champions model/equip/fire/recoil/reload/inspect/ADS/Reflex/top-frag effects, originalHUD/minimap/settings/input and Jett C/Q plus other enabledskills.",
                  "Native catalog/skin preload and shader/warmup paths, sound/locale loading, render/resource streaming and long sessions through GC.",
                  "Observe every missing original package/chunk/block read. Expand range manifest until no covered scenario requests a hole; preserve original verification and do not substitute models or effects."]}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path("F:/Riot Games/VALORANT/live"))
    parser.add_argument("--output", type=Path, default=ROOT / "reports/portable-resource-plan.json")
    parser.add_argument("--soft-scan-limit", type=int, default=5000)
    parser.add_argument("--reuse-soft-plan", type=Path)
    parser.add_argument("--core-ini", type=Path)
    parser.add_argument("--observed-runtime-log", type=Path, action="append", default=[])
    args = parser.parse_args()
    report = analyze(args.source_root.resolve(), args.output.resolve(), args.soft_scan_limit,
                     args.reuse_soft_plan, args.core_ini, args.observed_runtime_log)
    for name, profile in report["profiles"].items():
        sparse = profile["offset_preserving_sparse_candidate"]
        print(json.dumps({"profile": name, "packages": profile["selected_packages"],
                          "whole_drop": profile["whole_container_candidate"]["exclude_containers_if_validated"],
                          "whole_saved": profile["whole_container_candidate"]["potential_saved_bytes"],
                          "ranges_bytes": sparse["original_compressed_ranges_payload_bytes"],
                          "resource_bundle_payload": sparse["estimated_resource_bundle_payload_before_zip"],
                          "unresolved_imports": len(profile["unresolved_import_ids"])}), flush=True)
