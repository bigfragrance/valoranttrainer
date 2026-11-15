"""Rebase the validated core INI cache without shipping developer-specific paths."""
from __future__ import annotations

from pathlib import Path
import re


ROOT_TOKEN = "__TRAINER_ROOT__"
_DRIVE = re.compile(r"(?<![A-Za-z0-9_])[A-Za-z]:[\\/]")


def _strings(value, transform):
    if isinstance(value, str):
        return transform(value)
    if isinstance(value, list):
        return [_strings(item, transform) for item in value]
    if isinstance(value, dict):
        return {key: _strings(item, transform) for key, item in value.items()}
    return value


def export_core(snapshot: dict, source_root: Path) -> dict:
    """Keep all core values, isolating source hierarchy and optional cache paths."""
    if snapshot["other_files"]:
        raise ValueError("Portable configuration may contain only the core INI categories")
    given_root = Path(source_root)
    source_root = given_root.resolve()
    replacements = (
        (str(source_root), ROOT_TOKEN), (source_root.as_posix(), ROOT_TOKEN),
        (str(given_root), ROOT_TOKEN), (given_root.as_posix(), ROOT_TOKEN),
        ("C:/common-ddc/valorant/5.3", ROOT_TOKEN + "/.local/cache/common-ddc"),
        ("T:\\caches\\UnrealDDC\\VALORANT\\ddc.ddp", ROOT_TOKEN + "/.local/cache/ddc.ddp"),
        ("T:\\caches\\UnrealDDC", ROOT_TOKEN + "/.local/cache/ddc"),
        ("C:\\Program Files\\RenderDoc", ROOT_TOKEN + "/.local/optional/RenderDoc"),
    )

    def convert(text):
        for previous, replacement in replacements:
            text = text.replace(previous, replacement)
        text = re.sub(r"[A-Za-z]:/Users/[^/]+/(?:AppData/Local|Documents)/Unreal Engine/Engine/Config/(User[A-Za-z]+\.ini)",
                      ROOT_TOKEN + r"/.local/config/\1", text)
        if text.startswith(ROOT_TOKEN):
            text = text.replace("\\", "/")
        if _DRIVE.search(text):
            raise ValueError("An unhandled absolute path remains in the portable core configuration")
        return text

    return _strings(snapshot, convert)


def resolve_core(snapshot: dict, root: Path) -> dict:
    """Expand tokens on each launch, so copying the package preserves its paths."""
    root = Path(root).resolve()
    return _strings(snapshot, lambda text: text.replace(ROOT_TOKEN, root.as_posix()))
