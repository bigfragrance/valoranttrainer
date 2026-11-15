"""Verify a copied workspace without printing or interpreting file contents."""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path


def digest(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def verify(source: Path, destination: Path) -> dict:
    source, destination = source.resolve(strict=True), destination.resolve(strict=True)
    if source == destination or source in destination.parents or destination in source.parents:
        raise ValueError("Source and destination must be separate directories")
    files = sorted(p for p in source.rglob("*") if p.is_file())

    def check(path: Path) -> dict:
        relative = path.relative_to(source)
        target = destination / relative
        result = {"path": relative.as_posix(), "bytes": path.stat().st_size,
                  "source_sha256": digest(path)}
        result["destination_sha256"] = digest(target) if target.is_file() else None
        result["match"] = (result["source_sha256"] == result["destination_sha256"])
        return result

    with ThreadPoolExecutor(max_workers=4) as pool:
        records = list(pool.map(check, files))
    return {"verified_at": datetime.now(timezone.utc).isoformat(),
            "source": str(source), "destination": str(destination),
            "files": len(records), "bytes": sum(r["bytes"] for r in records),
            "mismatches": [r["path"] for r in records if not r["match"]],
            "records": records}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = verify(args.source, args.destination)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({key: value for key, value in result.items() if key != "records"},
                     ensure_ascii=False))
    return int(bool(result["mismatches"]))


if __name__ == "__main__":
    raise SystemExit(main())
