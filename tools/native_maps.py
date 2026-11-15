"""Pinned map package identities from the imported IoStore source sidecars."""
from __future__ import annotations
import json
from pathlib import Path

MAPS = {
    1: "/Game/Maps/Duel/Duel_1/Skirmish_A",
    2: "/Game/Maps/Duel/Duel_2/Skirmish_B",
    3: "/Game/Maps/Duel/Duel_3/Skirmish_C",
    4: "/Game/Maps/Duel/Duel_Platform/Skirmish_D",
    5: "/Game/Maps/Duel/Duel_Heady/Skirmish_E",
}


def training_maps(root=None) -> dict:
    root = Path(root or Path(__file__).resolve().parents[1])
    result = {"duel1": {"id": "duel1", "label": "斗牛一", "world_path": MAPS[1],
                        "kind": "duel", "validation": {"runtime": "verified"}}}
    try:
        report = json.loads((root / "reports/training-maps.json").read_text(encoding="utf-8"))
    except FileNotFoundError:
        return result
    for row in report.get("maps", []):
        key, package = row.get("id"), row.get("world_path")
        if (not isinstance(key, str) or not key.isalnum() or len(key) > 48
                or not isinstance(package, str) or not package.startswith("/Game/Maps/")
                or any(c in package for c in "?\"\r\n\\")):
            raise ValueError("Invalid training map catalog entry")
        result[key] = row
    return result


def parse_map(value):
    return int(value) if str(value).isdigit() else str(value).lower()


def map_package(number, *, training=False):
    if not isinstance(number, bool) and number in MAPS:
        return MAPS[number]
    if training and number in training_maps():
        return training_maps()[number]["world_path"]
    raise ValueError("Choose an available map from the original resource catalog")


def map_url(number, mode: str = "original") -> str:
    if mode not in ("original", "scene", "navigation", "training"):
        raise ValueError("Choose original, scene, navigation or training mode")
    package = map_package(number, training=mode == "training")
    if mode == "navigation" and number != 1:
        raise ValueError("Navigation development is restricted to Duel 1")
    # A diagnostic control using a shipped Engine class, not a gameplay fix.
    options = {"original": "", "scene": "?game=/Script/Engine.GameMode",
               "navigation": "?game=/Script/Engine.GameModeBase",
               "training": "?game=/Script/Engine.GameModeBase"}
    return package + options[mode]
