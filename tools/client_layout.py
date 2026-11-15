"""Resolve the pinned client in a project or a movable, self-contained package."""
from __future__ import annotations

from pathlib import Path


PORTABLE_MODE = "portable-v1"
CLIENT_IMAGE = "client/ShooterGame/Binaries/Win64/VALORANT-Win64-Shipping.exe"


def is_portable(config: dict) -> bool:
    mode = config.get("distribution_mode")
    if mode not in (None, PORTABLE_MODE):
        raise RuntimeError("Unsupported client distribution layout")
    return mode == PORTABLE_MODE


def client_executable(config: dict, root: Path, *, strict: bool = True) -> Path:
    """A portable package may launch only its own client, irrespective of cwd."""
    root = Path(root).resolve()
    configured = Path(config["game_executable"])
    if not is_portable(config):
        return (root / configured).resolve(strict=strict)
    if configured.is_absolute() or configured.as_posix().casefold() != CLIENT_IMAGE.casefold():
        raise RuntimeError("Portable client must use its fixed package-relative image path")
    client_root = root / "client"
    image = (root / CLIENT_IMAGE).resolve(strict=strict)
    # Do not turn a complete package back into a link to an installed client.
    if client_root.resolve() != client_root or client_root not in image.parents:
        raise RuntimeError("Portable client image resolves outside its package")
    if config.get("analysis_snapshot") != CLIENT_IMAGE:
        raise RuntimeError("Portable static analysis must use the same bundled image")
    return image
