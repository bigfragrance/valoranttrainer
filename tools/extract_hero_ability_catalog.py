"""Consolidate source-bound original hero ability evidence, without a game.

The five independent cooked-package audits retain full declarations, public
hashes and scripts in work/. This command produces a compact downstream
catalogue and independently rehashes every cited local asset. Static package
attribution, runtime CDO/slot observation and actual skill execution remain
separate gates; a complete catalogue never asserts playable completion.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / "work/local-server-playable/hero-abilities"
PINNED_IMAGE = "147763350504d241ee20b77607d04bc85cebfedb1d8bc2f32473ca93bf8fe92f"
SLOTS = ["Primary", "Secondary", "Melee", "GrenadeAbility", "Ability1", "Ability2",
         "Passive", "Level", "Invisible", "Ultimate", "Unarmed", "Armor", "Downed",
         "Backpack", "Totem", "PrimaryStorage", "SecondaryStorage"]


def owned_path(value: str | Path) -> Path:
    path = Path(value)
    if not path.is_absolute():
        path = ROOT / path
    path = path.resolve()
    if not path.is_relative_to(ROOT):
        raise ValueError("Catalogue evidence/output must stay inside the project: " + str(path))
    return path


def read_document(path: Path) -> tuple[Any, str]:
    raw = path.read_bytes()
    document = json.loads(raw.decode("utf-8-sig"))
    # Reports can still be refreshed by their owning worker. Do not consume a
    # document whose bytes changed while this consolidation was in progress.
    if path.read_bytes() != raw:
        raise ValueError("Evidence changed during extraction; rerun after its owner freezes: " + str(path))
    return document, hashlib.sha256(raw).hexdigest()


def asset_provenance(record: Any) -> dict[str, Any] | None:
    if not isinstance(record, dict):
        return None
    path = next((record[key] for key in ("source_path", "source_file", "path", "raw")
                 if isinstance(record.get(key), str)
                 and record[key].lower().endswith((".uasset", ".umap"))
                 and not record[key].startswith(("/Game/", "/Script/", "/Engine/"))), None)
    digest = next((record[key] for key in ("artifact_sha256", "asset_sha256", "source_sha256", "sha256")
                   if isinstance(record.get(key), str) and re.fullmatch("[0-9a-fA-F]{64}", record[key])), None)
    if path and digest:
        return {"path": path, "sha256": digest.lower()}
    for key in ("source", "asset", "pawn", "character_source"):
        if value := asset_provenance(record.get(key)):
            return value
    return None


def verify_asset(source: dict[str, Any] | None, cache: dict[tuple[str, str], dict[str, Any]]) -> dict[str, Any]:
    if source is None:
        return {"verified": False, "reason": "audit_does_not_supply_a_source_bound_local_asset"}
    path, expected = owned_path(source["path"]), source["sha256"]
    key = str(path), expected
    if key not in cache:
        if not path.is_file():
            raise ValueError("Cited asset is missing: " + str(path))
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        if digest != expected:
            raise ValueError("Cited asset hash mismatch: " + str(path))
        cache[key] = {"path": str(path.relative_to(ROOT)).replace("\\", "/"),
                      "sha256": digest, "bytes": path.stat().st_size, "verified": True}
    return dict(cache[key])


def hero_index(row: dict[str, Any]) -> int:
    return int(row["index"] if "index" in row else row["hero_index"])


def original_classes(row: dict[str, Any]) -> tuple[list[dict[str, Any]], dict[str, Any], str]:
    classes = row.get("starting_classes")
    if isinstance(classes, list):
        return classes, row.get("serialized_evidence", {}), "/starting_classes/"
    if isinstance(classes, dict) and isinstance(classes.get("classes"), list):
        return classes["classes"], {key: value for key, value in classes.items() if key != "classes"}, "/starting_classes/classes/"
    starters = row.get("starters")
    if isinstance(starters, list):
        array = row.get("selected_class_array_candidate", row.get("starter_array", {}))
        return starters, {key: value for key, value in array.items()
                          if key not in ("classes", "class_ancestries", "resolved")}, "/starters/"
    raise ValueError("Unsupported audit schema for hero " + str(hero_index(row)))


def class_details(row: dict[str, Any], item: dict[str, Any]) -> dict[str, Any]:
    details = dict(item.get("source", {})) if isinstance(item.get("source"), dict) else {}
    details.update(item)
    for extra in row.get("starter_details", []):
        if extra.get("class_object") == item.get("class_object"):
            details.update(extra)
            break
    return details


def slot_value(details: dict[str, Any]) -> int | None:
    value = details.get("slot", details.get("equippable_slot"))
    if isinstance(value, dict):
        value = value.get("value", value.get("slot"))
    if isinstance(value, int) and not isinstance(value, bool) and 0 <= value < len(SLOTS):
        return value
    return None


def extract(catalog_path: Path, evidence_dir: Path, *, allow_partial: bool) -> dict[str, Any]:
    catalog, catalog_digest = read_document(catalog_path)
    expected = {int(hero["index"]): hero for hero in catalog["heroes"]}
    if len(expected) != catalog["count"] or len(expected) != 30:
        raise ValueError("Pinned original playable hero catalogue is incomplete or duplicated")
    reports, rows, missing = [], {}, []
    for group in range(5):
        path = evidence_dir / f"group{group}.json"
        if not path.exists():
            missing.append(str(path.relative_to(ROOT)).replace("\\", "/"))
            continue
        document, digest = read_document(path)
        image = next((document[key] for key in ("native_image_sha256", "image_sha256", "original_binary_sha256")
                      if document.get(key)), None)
        if image is not None and image != PINNED_IMAGE:
            raise ValueError("Audit belongs to another native image: " + str(path))
        report_ref = str(path.relative_to(ROOT)).replace("\\", "/")
        reports.append({"path": report_ref, "sha256": digest, "group": group,
                        "scope": document.get("scope"), "image_identity_declared": image == PINNED_IMAGE})
        for ordinal, row in enumerate(document.get("heroes", [])):
            index = hero_index(row)
            if index not in expected or index in rows or index // 6 != group:
                raise ValueError("Unexpected, duplicated or wrong-group hero index: " + str(index))
            rows[index] = (row, report_ref + f"#/heroes/{ordinal}")
    if missing and not allow_partial:
        raise ValueError("Required hero audits are still pending: " + ", ".join(missing))
    cache: dict[tuple[str, str], dict[str, Any]] = {}
    heroes, unknowns = [], []
    for index in sorted(expected):
        source_hero = expected[index]
        base = {"index": index, "name": source_hero["name"], "code": source_hero["code"],
                "class": source_hero["class"], "id": source_hero["id"],
                "native_runtime_verified": False, "skill_execution_verified": False,
                "server_authority_verified": False, "playable_verified": False}
        if index not in rows:
            heroes.append({**base, "catalog_status": "pending_independent_audit", "starting_classes": []})
            unknowns.append({"hero": index, "reason": "independent_audit_pending"})
            continue
        row, reference = rows[index]
        name = row.get("name", row.get("hero_name"))
        actual_class = row.get("class_object", row.get("pawn_class", row.get("class", row.get("character_class"))))
        if actual_class is None and isinstance(row.get("pawn"), dict):
            actual_class = row["pawn"].get("class_object")
        if name != base["name"] or row.get("code") != base["code"] or actual_class != base["class"]:
            raise ValueError("Hero identity differs from the pinned playable catalogue: " + str(index))
        pawn_asset = verify_asset(asset_provenance(row), cache)
        classes, array, class_ref = original_classes(row)
        if not 0 < len(classes) <= 64:
            raise ValueError("Original starting class count is outside the native bound: " + str(index))
        entries, seen, complete = [], set(), pawn_asset["verified"]
        for order, item in enumerate(classes):
            details = class_details(row, item)
            path = details.get("class_object")
            if not isinstance(path, str) or not path.startswith("/Game/") or not path.endswith("_C") or path in seen:
                raise ValueError("Missing, duplicated or non-original starting class: " + repr(path))
            seen.add(path)
            asset = verify_asset(asset_provenance(details), cache)
            public_hash = details.get("class_public_hash", details.get("public_hash"))
            if not isinstance(public_hash, str) or not re.fullmatch(r"0x[0-9a-fA-F]{1,16}", public_hash):
                raise ValueError("Missing source public hash for " + path)
            slot = slot_value(details)
            complete = complete and asset["verified"]
            entries.append({"authored_index": order, "class": path,
                            "public_hash": "0x" + public_hash[2:].lower().zfill(16),
                            "asset": asset, "slot_static": slot,
                            "slot_name_static": SLOTS[slot] if slot is not None else None,
                            "slot_runtime_verified": False,
                            "native_parent": details.get("parent", details.get("class_super")),
                            "evidence_ref": reference + class_ref + str(order),
                            "slot_evidence": details.get("slot_evidence", details.get("slot_serialization", details.get("equippable_slot"))),
                            "runtime_cdo_crosscheck_required": True})
            if slot is None:
                unknowns.append({"hero": index, "class": path, "reason": "slot_requires_original_loaded_cdo_observation"})
            if not asset["verified"]:
                unknowns.append({"hero": index, "class": path, "reason": asset["reason"]})
        # Only the fully decoded property evidence can establish this static
        # attribution. Unique byte-array candidates remain explicitly candidates.
        property_evidence = row.get("serialized_evidence")
        property_attribution = isinstance(property_evidence, dict) and property_evidence.get("found") is True
        if not property_attribution:
            unknowns.append({"hero": index, "reason": "starting_array_property_attribution_requires_native_getter_crosscheck"})
        heroes.append({**base, "catalog_status": "source_assets_verified" if complete else "source_evidence_incomplete",
                       "asset": pawn_asset, "evidence_ref": reference,
                       "authored_class_count": len(entries), "static_property_attribution_verified": property_attribution,
                       "array_evidence": array, "starting_classes": entries,
                       "active_slots_static": sorted({item["slot_static"] for item in entries
                                                      if item["slot_static"] in (3, 4, 5, 9)}),
                       "unknown_slots": sum(item["slot_static"] is None for item in entries),
                       "authority_gaps": row.get("shared_authority_gap", row.get("authority_gaps", []))})
    complete = len(rows) == len(expected) and all(hero["catalog_status"] == "source_assets_verified" for hero in heroes)
    return {"schema_version": 1, "generated_utc": datetime.now(timezone.utc).isoformat(),
            "scope": "Static original cooked-package catalogue; no target execution, network connection, client mutation or original-install write",
            "native_image_sha256": PINNED_IMAGE,
            "catalog_source": {"path": str(catalog_path.relative_to(ROOT)).replace("\\", "/"), "sha256": catalog_digest},
            "audit_reports": reports, "pending_reports": missing,
            "hero_count": len(heroes), "audited_heroes": len(rows),
            "source_assets_verified": len(cache),
            "source_catalog_complete": complete,
            "starting_class_count": sum(len(hero["starting_classes"]) for hero in heroes),
            "heroes": heroes, "unknowns": unknowns,
            "runtime_validation": {"requires_original_loaded_cdo_and_getter_crosscheck": True,
                                   "requires_original_slot_roles_and_owner_receipts": True,
                                   "requires_hero_specific_authority_world_effects": True,
                                   "all_heroes_skill_execution_verified": False,
                                   "all_heroes_playable_verified": False},
            "limitations": ["Starting class order, package directory names and internal Q/E/4 labels are not current input bindings.",
                            "Inherited EquippableSlot must be read from the real loaded CDO; unknown static slots are never guessed.",
                            "Granting original skill actors does not recover stripped authority functions such as SpawnAbilityChildActor or SpawnGameObject.",
                            "The original host/client runtime, game input, animations, effects, damage and cleanup still require independent execution evidence."]}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=owned_path, default=ROOT / "profiles/playable-heroes.json")
    parser.add_argument("--evidence-dir", type=owned_path, default=EVIDENCE)
    parser.add_argument("--output", type=owned_path, default=ROOT / "reports/local-server-hero-abilities-catalog.json")
    parser.add_argument("--allow-partial", action="store_true", help="Record missing groups as pending; never mark a partial catalogue complete")
    args = parser.parse_args()
    result = extract(args.catalog, args.evidence_dir, allow_partial=args.allow_partial)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    temporary = args.output.with_name(args.output.name + ".tmp")
    temporary.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    temporary.replace(args.output)
    print(json.dumps({"output": str(args.output), "heroes": result["hero_count"], "audited": result["audited_heroes"],
                      "classes": result["starting_class_count"], "verified_assets": result["source_assets_verified"],
                      "source_catalog_complete": result["source_catalog_complete"],
                      "playable_verified": False}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
