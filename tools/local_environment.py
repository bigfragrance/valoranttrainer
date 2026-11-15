"""Project-owned caches for development, builds and owned child processes."""
from __future__ import annotations

import os
from pathlib import Path


def project_environment(root: Path, base=None) -> dict[str, str]:
    env = dict(os.environ if base is None else base)
    runtime = Path(root).resolve() / ".local/runtime"
    paths = {"TEMP": runtime / "temp", "TMP": runtime / "temp",
             "LOCALAPPDATA": runtime / "LocalAppData", "APPDATA": runtime / "Roaming",
             "UE-LocalDataCachePath": runtime / "DerivedDataCache",
             "PYTHONPYCACHEPREFIX": runtime / "pycache"}
    for key, path in paths.items():
        path.mkdir(parents=True, exist_ok=True)
        env[key] = str(path)
    env["PYTHONUTF8"] = "1"
    return env
