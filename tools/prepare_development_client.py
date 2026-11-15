"""Reuse the project's tested client copy with installed assets mounted read-only.

This development layout is intentionally not an export. Its executable, settings,
logs and modules belong to the workspace; only its Paks directory resolves to the
verified international installation. No source file is written or re-protected.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

from run_native_probe import ROOT, SUPPORTED_GAME, game_processes

SOURCE_PAKS = SUPPORTED_GAME.parents[2] / "Content/Paks"


def prepare(package: Path):
    root = ROOT.resolve()
    package = package.resolve(strict=True)
    donor = package / "client"
    target = root / "client"
    if root / "exports" not in package.parents or donor.resolve(strict=True) != donor:
        raise RuntimeError("The donor must be a real project-owned QA extraction")
    image_relative = Path("ShooterGame/Binaries/Win64/VALORANT-Win64-Shipping.exe")
    config = json.loads((root / "project.json").read_text(encoding="utf-8"))
    manifest = package / "resources/iostore-manifest.json"
    if not manifest.is_file():
        raise RuntimeError("The tested QA extraction has no resource identity manifest")
    core = ".local/native-baseline/portable-core/core-config.json"
    for required in (package / core, package / "reports/native-config-validation.json"):
        if not required.is_file():
            raise RuntimeError("The QA extraction has no verified core configuration")
    if target.exists() or target.is_symlink():
        raise RuntimeError("A development client already exists; it was left unchanged")
    for item in game_processes():
        image = item.get("executable_path")
        if image and donor in Path(image).resolve().parents:
            raise RuntimeError("The donor's owned test session must finish before moving its files")
    source_image = donor / image_relative
    with source_image.open("rb") as stream:
        digest = hashlib.file_digest(stream, "sha256").hexdigest()
    if digest != config["expected_snapshot_sha256"]:
        raise RuntimeError("The QA client image does not match this project")
    if SOURCE_PAKS.resolve(strict=True) != SOURCE_PAKS or not SOURCE_PAKS.is_dir():
        raise RuntimeError("The fixed source asset directory is unavailable")
    backup = root / ".local/development-resource-backup"
    backup.mkdir(parents=True, exist_ok=True)
    paks_backup = backup / "Paks"
    # Both recursive moves are confined to the resolved workspace. Same-volume
    # renames preserve sparse ranges and do not duplicate the large resources.
    for path in (donor, target, donor / "ShooterGame/Content/Paks", paks_backup):
        if root not in path.resolve().parents:
            raise RuntimeError("A development move would leave the workspace")
    if paks_backup.exists():
        raise RuntimeError("A previous asset backup exists; inspect it before continuing")
    (backup / "project-before.json").write_bytes((root / "project.json").read_bytes())
    os.rename(donor, target)
    paks = target / "ShooterGame/Content/Paks"
    os.rename(paks, paks_backup)
    completed = subprocess.run([os.environ.get("COMSPEC", "cmd.exe"), "/d", "/c", "mklink", "/J",
                                str(paks), str(SOURCE_PAKS)], capture_output=True)
    if completed.returncode or paks.resolve() != SOURCE_PAKS:
        raise RuntimeError("Read-only source asset mount was not created; the prior sparse assets remain in the backup")
    (root / core).parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(package / core, root / core)
    proof_path = root / "reports/native-config-validation.json"
    shutil.copy2(proof_path, backup / "native-config-validation-before.json")
    shutil.copy2(package / "reports/native-config-validation.json", proof_path)
    shutil.copy2(manifest, backup / "source-resource-manifest.json")
    config.update({"distribution_mode": "portable-v1",
                   "game_executable": "client/" + image_relative.as_posix(),
                   "analysis_snapshot": "client/" + image_relative.as_posix(),
                   "resource_mode": "installed-readonly-v1",
                   "source_game_executable": str(SUPPORTED_GAME),
                   "source_resource_directory": str(SOURCE_PAKS),
                   "source_resource_manifest": ".local/development-resource-backup/source-resource-manifest.json"})
    (root / "project.json").write_text(json.dumps(config, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    receipt = {"kind": "owned-development-client", "image": str(target / image_relative),
               "asset_mount": str(paks), "asset_source": str(SOURCE_PAKS),
               "asset_source_access": "read-only", "source_files_modified": False,
               "previous_sparse_assets": str(paks_backup), "qa_donor": str(package)}
    (backup / "layout.json").write_text(json.dumps(receipt, indent=2), encoding="utf-8")
    return receipt


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--from-package", type=Path, required=True)
    print(json.dumps(prepare(parser.parse_args().from_package), ensure_ascii=False))
