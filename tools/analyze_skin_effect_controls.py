"""Read original cooked skin scripts/data without loading or launching the game.

Output distinguishes original asset/handler presence from a verified manual
adapter. Unknown script tokens or unresolved parameter layouts stay unknown.
All exports and reports are written below this project's F: workspace.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / "work/skin-effect-controls"
PINNED_SHA256 = "147763350504d241ee20b77607d04bc85cebfedb1d8bc2f32473ca93bf8fe92f"


def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def nodes(value):
    if isinstance(value, dict):
        if "kind" in value:
            yield value
        for child in value.values():
            yield from nodes(child)
    elif isinstance(value, list):
        for child in value:
            yield from nodes(child)


def package_path(path):
    if path.startswith("../../../ShooterGame/Content/"):
        return "/Game/" + path.split("/Content/", 1)[1].removesuffix(".uasset")
    return path.removesuffix(".uasset")


def describe(node):
    kind = node.get("kind", "unknown")
    if kind in ("Local", "Instance", "Default", "Out", "Property"):
        return kind + ":" + ".".join(node["field"]["names"])
    if kind in ("Call", "VirtualCall"):
        return str(node.get("function")) + "(" + ", ".join(describe(x) for x in node.get("params", [])) + ")"
    if kind == "Context":
        return describe(node["object"]) + "." + describe(node["value"])
    if kind in ("Object", "Name", "Constant", "String", "Vector", "Rotation"):
        return repr(node.get("value"))
    if kind == "JumpIfNot":
        return "if not " + describe(node["condition"]) + " goto " + str(node["target"])
    if "left" in node and "right" in node:
        return kind + "(" + describe(node["left"]) + ", " + describe(node["right"]) + ")"
    if "values" in node:
        return kind + "[" + ", ".join(describe(x) for x in node["values"]) + "]"
    if "value" in node:
        return kind + "(" + (describe(node["value"]) if isinstance(node["value"], dict) else repr(node["value"])) + ")"
    return kind + (" " + str(node["target"]) if "target" in node else "")


def family(path):
    parts = path.split("/")
    for i, part in enumerate(parts):
        if part.startswith("Champions") and "." not in part:
            return "/".join(parts[max(0, i - 1):i + 1])
    return None


def refresh_sources(assets):
    old = {item["container"]: item.get("sha256") for item in assets.inv}
    current, paths = [], {}
    source = Path("F:/Riot Games/VALORANT/live/ShooterGame/Content/Paks")
    for toc in sorted(source.glob("*.utoc")):
        reader = assets.IoStore(toc)
        digest = hashlib.sha256(reader.data).hexdigest()
        assets.readers[toc.name] = reader
        entries = [{"path": path, "index": index, "size": reader.ranges[index][1]}
                   for path, index in reader.files.items()]
        current.append({"container": toc.name, "sha256": digest, "old_inventory_sha256": old.get(toc.name),
                        "old_inventory_matches": old.get(toc.name) == digest, "files": entries})
        for entry in entries:
            paths[entry["path"]] = (toc.name, entry)
    destination = WORK / "current-inventory.json"
    destination.write_text(json.dumps(current, ensure_ascii=False, separators=(",", ":")), encoding="utf-8")
    assets.paths = paths
    return [{key: value for key, value in item.items() if key != "files"} for item in current]


def native_metadata():
    import struct
    sys.path.insert(0, str(ROOT / "work"))
    import platform_core as pe
    sys.path.insert(0, str(ROOT / "work/client-analysis/continue-world"))
    import inspect_world as native
    sys.path.insert(0, str(ROOT / "work/duel1-respawn"))
    from inspect_respawn import table
    namespace = {}
    source = (ROOT / "work/duel1-original-integration-player/properties.py").read_text(encoding="utf-8")
    exec(source.split("catalog=")[0], namespace)
    raw, q, prop = namespace["raw"], namespace["q"], namespace["prop"]
    destination = WORK / "native"
    destination.mkdir(parents=True, exist_ok=True)
    native.ROOT = destination
    result = {}
    for name in ("EquippableSkinDataAsset", "EquippableSkinLevelDataAsset", "EquippableSkinFeaturesDataAsset",
                 "AresEquippable", "AresWeaponAttachmentComponent", "ChromaSkinOverride", "EffectContainerGunSkinFunctionLibrary",
                 "EffectContainer", "KismetSystemLibrary"):
        methods = table(name)
        getter = int(methods["class"]["getter_rva"], 16)
        properties = []
        for address, _ in native.ptrrefs(getter):
            try:
                pointers = q(address + 0x28)
                prop(q(pointers))
                count = (struct.unpack("<I", raw(address + 0x38, 4))[0] >> 15) & 0x7ff
                fields = []
                for i in range(count):
                    try:
                        entry = prop(q(pointers + i * 8))
                        entry["table_index"] = i
                        fields.append(entry)
                    except Exception:
                        break
                if fields:
                    properties.append({"descriptor": hex(address), "properties": fields})
            except Exception:
                pass
        record = {"class": methods["class"], "methods": methods["methods"], "properties": properties}
        save(destination / (name + ".json"), record)
        result[name] = str((destination / (name + ".json")).relative_to(ROOT))
    for address in (0x6465df0, 0x6465ef0):
        native.function(address)
    layouts = []
    for name in ("OverrideEffect", "GetOverridableEffect", "SetObjectPropertyByName", "SetIntPropertyByName", "GetPathName"):
        for _, string_address, _ in native.findstr(name):
            for reference, _ in native.ptrrefs(string_address):
                address = reference - 0x10
                try:
                    descriptor = raw(address, 64)
                    if struct.unpack_from("<I", descriptor, 0x34)[0] != 0x45:
                        continue
                    count, size = struct.unpack_from("<HH", descriptor, 0x30)
                    if not 0 < count <= 16 or size > 1024:
                        continue
                    parameters = q(address + 0x28)
                    fields = [prop(q(parameters + i * 8)) for i in range(count)]
                    layouts.append({"name": name, "descriptor": hex(address), "parameter_bytes": size, "parameters": fields})
                except Exception:
                    pass
    save(destination / "call-layouts.json", layouts)
    result["call_layouts"] = str((destination / "call-layouts.json").relative_to(ROOT))
    return result


def adapter_metadata(records):
    specs = [
        ("FXC_AK_Champions_Champion_Effect", "AK/Champions", "Custom Champion Updated", [2801], 2381, 8),
        ("FXC_Carbine_Champions2_WhileEquipped_1P_Smoke_LV4", "Carbine/Champions2", "Custom Champion Updated", [2148], 7415, 8),
        ("FXC_AK_WhileEquipped_1P_Champions2023_Smoke_LV4", "AK/Champions2023", "Custom Champion Updated", [1085, 1481], 4986, 8),
        ("FXC_Carbine_Champions2024_WhileEquipped_1P_Smoke_LV4", "Carbine/Champions2024", "Custom Champion Updated", [717, 2563], 10010, 8),
        ("FXC_Carbine_Champions2026_WhileEquipped_1P_Smoke_Lv4_Attacker", "Carbine/Champions2026", "Team Champion Updated", [1783], 4521, 16),
        ("FXC_Carbine_Champions2026_WhileEquipped_1P_Smoke_Lv4_Defender", "Carbine/Champions2026", "Champion Updated", [1965], 7517, 8),
    ]
    profiles = []
    for leaf, group, event, initialize, update, size in specs:
        record = next((x for x in records.values() if x["package"].endswith("/" + leaf)), None)
        if not record:
            profiles.append({"family": group, "original_effect": leaf, "available_in_source": False})
            continue
        functions = json.loads((ROOT / record["script_evidence"]).read_text(encoding="utf-8"))["functions"]
        graph = next(x for x in functions if x["name"].startswith("ExecuteUbergraph_"))
        points = {node["vm"]: node for node in graph.get("nodes", [])}
        entries = initialize + [update] + ([7600] if group == "Carbine/Champions2" else [])
        profiles.append({"family": group, "original_effect": record["package"], "sha256": record["sha256"],
                         "original_handler": event, "expected_handler_parameter_bytes": size,
                         "initialize_client_entries": initialize, "update_client_entry": update,
                         "expected_script_memory_bytes": graph["memory_bytes"],
                         "source_nodes": [{"vm": entry, "opcode": points.get(entry, {}).get("op"),
                                           "kind": points.get(entry, {}).get("kind")} for entry in entries],
                         "complete_original_graph_decode": graph.get("complete", False),
                         "script_evidence": record["script_evidence"], "runtime_verified": False})
    parent = next((record for record in records.values() if record["package"].endswith("/FXC_Carbine_Champions2024_WhileEquipped_1P_Smoke_Lv2")), None)
    if parent:
        graph = next(item for item in json.loads((ROOT / parent["script_evidence"]).read_text(encoding="utf-8"))["functions"]
                     if item["name"].startswith("ExecuteUbergraph_"))
        points = {node["vm"]: node for node in graph["nodes"]}
        target = next(item for item in profiles if item["family"] == "Carbine/Champions2024")
        target["required_original_parent_initialization"] = {
            "package": parent["package"], "sha256": parent["sha256"], "script_evidence": parent["script_evidence"],
            "memory_bytes": graph["memory_bytes"], "stored_bytes": graph["stored_bytes"], "complete": graph["complete"],
            "entries": [{"vm": entry, "opcode": points[entry]["op"], "kind": points[entry]["kind"]} for entry in (96, 366)],
            "original_dependency": "Entry96 executes original GetOwner(Pawn) -> Inventory.GetCurrentEquippable -> GetComponentByClass(Weapon_Skin_Base) -> My Gun Skin. Entry366 copies the already populated original StartEffect target into My FXC Target.",
            "admission": "Actual original inherited UFunction ownership, actor class inheritance, Script.Num426, original start/end opcodes and exact entry opcodes must match. Original actor owner, Equippable, native current inventory and real skin component must match this binding. OriginalTarget is an optional transient weak cache observed null in the actual local lifecycle; any live cached target must agree. The original StartEffect frame's actual target must independently read back from leaf366 as the actual Pawn.",
            "readback": "My Gun Skin must be the real current skin attachment and My FXC Target must be the actual player Pawn before original Lv4 entries717/2563 run. Original materials and nonempty original VFXMesh List must then exist.",
            "practice_component_reference": "Bare practice ShooterGameState lacks the original Base_Game_State Blueprint field. Only the actual original FXC myCompGameStateKillChampion object reference is supplied through original SetObjectPropertyByName32, pointing to the real registered original training champion component; no score/champion/material/color/mesh field is synthesized.",
            "runtime_fix_verified": False}
    return [
        {"key": "champion_glow", "label": "冠军领先发光", "scope": "Only the actual current Level4 original Champion skin and its source-bound original FXC.",
         "modes": {"0": "Follow original Comp_GameState_KillChampion.Champion/AttackingChampion/DefendingChampion", "1": "Force original owner-leading presentation", "2": "Force original non-leading presentation"},
         "value": {"type": "unused", "meaning": "No invented color/intensity slider; the original curves and materials own these values."},
         "profiles": profiles,
         "standalone_adapter": "Original CheckLocalOnly explicitly requires !IsServer, so Standalone skips client presentation. The adapter checks exact original UClass/function ownership, parameter ABI, original loaded VM byte count and entry opcode before calling only audited original client leaves. It never changes NetMode or real ranking.",
         "actual_application_gate": "Original event and original client leaf must both complete on a live original FXC whose original skin/material objects match this binding. Requested mode and applied mode remain distinct; missing FXC/materials is unavailable, not success."},
        {"key": "skin_kill_count", "label": "皮肤击杀递进", "families": ["Carbine/Champions2"], "level": 4,
         "modes": {"0": "Original owning player's real GetKills", "1": "Manual cosmetic progression value", "2": "Original zero-kill cosmetic progression"},
         "value": {"type": "integer", "minimum": 0, "maximum": 25, "source": "Original Ubergraph VM7600 Clamp(MyKills,0,25), then original crystal/material/timeline code."},
         "source_state": "Only this actual FXC's original Comp_FXC_Game_Events.MyKills; original SetIntPropertyByName24B plus readback. No PlayerState/scoreboard field is changed.",
         "runtime_verified": False},
        {"key_pattern": "fxc_[0-9a-f]{16}", "label": "原皮效果：<actual original SkinFXC object name>",
         "stable_key": "fxc_ plus16 lower hexadecimal digits of FNV-1a64 over the exact UTF-8 original BaseFXC UClass GetPathName output.",
         "scope": "Dynamic current-weapon capabilities only, for actual EffectOverrides entries whose original getter agrees with the stored original skin class.",
         "modes": {"0": "Restore/follow the actual original skin mapping", "1": "Use the actual original skin FXC", "2": "Use the actual original BaseFXC instead of the extra skin FXC"},
         "follow_semantics": "Default follow only reads the live original map. It restores the last observed authored mapping solely when releasing a manual value still owned by this adapter; a native rule returning to BaseFXC is preserved. An indistinguishable native rewrite of the same currently forced class cannot be attributed without an event observer.",
         "value": {"type": "unused"},
         "native_evidence": {"map_offset": "0xcb0", "sparse_entry_bytes": 64, "soft_class_key_bytes": 48, "original_class_value_offset": "0x30", "original_setter_parameter_bytes": 56, "original_getter_parameter_bytes": 16,
                             "files": ["work/skin-effect-controls/native/AresEquippable.json", "work/skin-effect-controls/native/call-layouts.json", "work/skin-effect-controls/native/function-0x6465ef0.json", "work/skin-effect-controls/native/function-0x71279a0.json", "work/skin-effect-controls/native/function-0x70ee9a0.json"]},
         "timing": "The changed original map is read back immediately. Actual presentation occurs at the next original shot/equip/reload/inspect event that queries that BaseFXC; an already-running continuous FXC is not fabricated/restarted by this generic switch.",
         "runtime_verified": False}
    ]


def inspect_package(path, assets, decoder):
    # Refresh this selected package against its CURRENT original container;
    # previous extracted copies never silently stand in for new shipped data.
    file = "../../../ShooterGame/Content/" + path.removeprefix("/Game/") + ".uasset"
    container, _ = assets.paths[file]
    destination = WORK / "raw" / file.split("/Content/", 1)[1]
    assets.readers[container].export(file, destination)
    exported = assets.export(path)
    z = assets.Zen(exported["raw"])
    functions = [decoder.function(z, entry) for entry in z.exports
                 if assets.resolve(z, int(entry["class_index"], 16)) == "/Script/CoreUObject.Function"]
    for fn in functions:
        if fn.get("error") != "ambiguous complete script boundaries" or not fn["name"].startswith("ExecuteUbergraph_"):
            continue
        entries = [p["value"] for caller in functions for node in nodes(caller.get("nodes", []))
                   if node.get("kind") == "Call" and node.get("function") == fn["name"]
                   for p in node.get("params", []) if p.get("kind") == "Constant" and isinstance(p.get("value"), int)]
        candidates = [x for x in fn.get("boundary_attempts", []) if x.get("complete")
                      and all(0 <= entry < x["memory_bytes"] for entry in entries)]
        # A coincidental tiny tail script cannot implement the package's real
        # source-bound Ubergraph entry points. Refine by these actual callers.
        if entries and len(candidates) == 1 and candidates[0]["script_package_offset"] == fn["script_package_offset"]:
            fn["complete"], fn["error"] = True, None
            fn["boundary_refinement"] = {"rule": "all actual same-package explicit Ubergraph entry points must fit",
                                         "original_entry_points": sorted(set(entries))}
    script_file = WORK / "scripts" / (path.removeprefix("/Game/") + ".json")
    save(script_file, {"path": path, "functions": functions})
    script_file.with_suffix(".txt").write_text("\n".join(
        "\nFUNCTION " + fn["name"] + " complete=" + str(fn.get("complete")) + "\n" +
        "\n".join(str(node["vm"]) + ": " + describe(node) for node in fn.get("nodes", []))
        for fn in functions) + "\n", encoding="utf-8")
    classes = [entry for entry in exported["exports"]
               if entry["type"] == "/Script/Engine.BlueprintGeneratedClass"]
    calls = []
    for fn in functions:
        for node in nodes(fn.get("nodes", [])):
            if node["kind"] in ("Call", "VirtualCall"):
                calls.append({"function": fn["name"], "vm": node["vm"],
                              "callee": node.get("function"), "parameters": node.get("params", [])})
    return {"package": path, "raw": exported["raw"],
            "sha256": hashlib.sha256(z.data).hexdigest(), "family": family(path),
            "generated_classes": [{"name": entry["name"], "super": entry["super"]} for entry in classes],
            "imports": z.imported_packages, "script_evidence": str(script_file.relative_to(ROOT)),
            "functions": [{key: fn[key] for key in ("name", "complete", "error", "stored_bytes", "memory_bytes")
                           if key in fn} for fn in functions], "calls": calls,
            "original_functions_complete": bool(functions) and all(fn.get("complete") for fn in functions)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", type=Path, default=ROOT / "reports/skin-effect-controls.json")
    parser.add_argument("--package", action="append", default=[], help="Additional exact /Game package to inspect")
    parser.add_argument("--all-skin-scripts", action="store_true", help="Inspect all actual gun skin-level Blueprint packages")
    parser.add_argument("--native-only", action="store_true", help="Inspect native original skin/effect metadata only")
    args = parser.parse_args()
    os.chdir(ROOT)
    WORK.mkdir(parents=True, exist_ok=True)
    if args.native_only:
        print(json.dumps({"native_metadata": native_metadata()}, ensure_ascii=False))
        return
    sys.path.insert(0, str(ROOT / "work/duel1-jett-abilities"))
    import assets
    assets.ROOT = WORK
    containers = refresh_sources(assets)
    sys.path.insert(0, str(ROOT / "work/duel1-original-jett-native"))
    import decode_x_blueprint
    decoder = decode_x_blueprint.base
    class SkinScript(decoder.Script):
        def expr(self):
            op = self.data[self.at]
            if op not in (0x67, 0x29):
                return super().expr()
            at, vm = self.at, self.vm
            self.read("<B")
            result = {"vm": vm, "at": at, "op": hex(op)}
            if op == 0x67:  # EX_SoftObjectConst carries an expression/string.
                result.update(kind="SoftObjectConstant", value=self.expr())
            else:  # EX_TextConst: admit only the observed empty-text subtype.
                subtype = self.read("<B")
                if subtype == 0:
                    result.update(kind="EmptyTextConstant", value="")
                elif subtype == 1:
                    result.update(kind="LocalizedTextConstant", source=self.expr(), key=self.expr(), namespace=self.expr())
                else:
                    raise ValueError(f"unsupported TextConst subtype{subtype} at{at:#x}")
            result["end"], result["vm_end"] = self.at, self.vm
            return result
    decoder.Script = SkinScript

    files = sorted(path for path in assets.paths if path.endswith(".uasset"))
    skins = [path for path in files if "/Equippables/Guns/" in path and "/Levels/" in path and path.endswith("_Skin.uasset")]
    champion_inventory = [path for path in files if "/Equippables/" in path and "/Champions" in path]
    targets = [package_path(path) for path in skins if args.all_skin_scripts or "/Champions" in path]
    targets += [package_path(path) for path in args.package]
    path_casefold = {path.casefold(): path for path in files}
    pending, records, failures = list(dict.fromkeys(targets)), {}, []
    while pending:
        path = pending.pop(0)
        requested = "../../../ShooterGame/Content/" + path.removeprefix("/Game/") + ".uasset"
        if requested.casefold() in path_casefold:
            path = package_path(path_casefold[requested.casefold()])
        if path in records:
            continue
        try:
            record = inspect_package(path, assets, decoder)
            records[path] = record
            # Follow original Champion script/FXC/control components, not names
            # alone as support proof. Exact imported packages establish edges.
            for dependency in record["imports"]:
                filename = dependency.rsplit("/", 1)[-1]
                if "/Champions" in dependency and (filename.startswith(("FXC_", "Comp_", "BP_")) or filename.endswith("_Skin")):
                    if dependency not in records and dependency not in pending:
                        pending.append(dependency)
        except Exception as error:
            failures.append({"package": path, "error": str(error)})

    families = sorted({family(path) for path in champion_inventory if family(path)})
    report = {"schema_version": 1, "scope": "Original inventory/package/script inspection only; no game launch or build.",
              "pinned_image_sha256": PINNED_SHA256,
              "original_install_read_only": True, "account_or_network_operations": False,
              "inventory_source": "work/skin-effect-controls/current-inventory.json",
              "older_inventory_comparison": "work/extract/inventory.json", "original_containers": containers,
              "inventory_skin_level_blueprints": len(skins),
              "champion_original_package_count": len(champion_inventory), "champion_original_families": families,
              "packages_inspected": len(records), "failures": failures,
              "assets": list(records.values()), "manual_controls": adapter_metadata(records),
              "runtime_header": "native/skin_effect_controls.h", "compiled_or_launched_by_this_agent": False,
              "runtime_api": ["Initialize(void* world, void* pawn)", "OnWeaponChanged(const Binding&) or compatible CurrentBinding",
                              "PrepareForBindingChange() before retiring/replacing current attachments", "SetOverride(const char* key,uint32_t mode,float value=0)", "Poll()/Tick()", "State()", "WriteDiagnostics(path)"],
              "original_feature_categories": {"native_type": "EquippableSkinFeaturesDataAsset", "evidence": "work/skin-effect-controls/native/EquippableSkinFeaturesDataAsset.json",
                                              "fields": ["ModelCategory", "ReloadVFXCategory", "InspectVFXCategory", "EquipVFXCategory", "IdleVFXCategory", "FiringVFXCategory", "SFXCategory", "FiringSFXCategory", "FinisherCategory", "KillBanner"],
                                              "meaning": "These are actual original data categories; category presence alone is not an independent toggle or manual callback."},
              "unavailable_independent_controls": [
                  {"key": "finisher", "reason": "No audited independent force-finisher callback in this adapter. Actual original finisher assets remain present; a true per-FXC override row is exposed when the live original map supplies one."},
                  {"key": "kill_banner", "reason": "Original SkinKillBannerData+f28 exists, but no standalone per-banner toggle is claimed without an audited native notification/data lifecycle."},
                  {"key": "arbitrary_glow_color_or_strength", "reason": "No invented shader parameters. Original skin curves/materials own the glow output."},
                  {"key": "unknown_skin_specific_parameters", "reason": "No UI configurability is inferred from a theme name, an imported texture, or a handler name alone."}],
              "boundaries": ["An asset/handler being present does not prove it can be configured safely.",
                             "Only original exact handlers with validated parameter/source bindings will be enabled in the runtime adapter.",
                             "Raw material values, substitute effects and non-existent effects are never invented.",
                             "Runtime/pixel acceptance belongs to the parent; this analyzer never loads the game."]}
    save(args.report, report)
    print(json.dumps({"report": str(args.report), "skin_level_blueprints": len(skins),
                      "champion_families": families, "packages": len(records), "failures": len(failures)}, ensure_ascii=False))


if __name__ == "__main__":
    main()
