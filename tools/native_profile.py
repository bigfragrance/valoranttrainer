"""Temporarily add a verified configuration-source overlay and restore its prior bytes."""
from contextlib import contextmanager
import hashlib
import os
from pathlib import Path
import tempfile


def atomic_write(path, payload):
    with tempfile.NamedTemporaryFile(dir=path.parent, prefix="trainer-profile-", suffix=".tmp", delete=False) as stream:
        temporary = Path(stream.name)
        stream.write(payload)
    try:
        os.replace(temporary, path)
    finally:
        if temporary.exists():
            temporary.unlink()


@contextmanager
def temporary_profile(target, overlay, run_dir):
    target = Path(target).resolve()
    if target.name != "UserEngine.ini":
        raise ValueError("Only the established UserEngine.ini source is supported")
    target.parent.mkdir(parents=True, exist_ok=True)
    original = target.read_bytes() if target.exists() else None
    if original is not None:
        (run_dir / "UserEngine.original.bin").write_bytes(original)
        encoding = "utf-16" if original.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig"
        prior_text = original.decode(encoding)
    else:
        prior_text = ""
    payload = (prior_text + "\n" + overlay + "\n").encode("utf-8")
    expected = hashlib.sha256(payload).hexdigest()
    state = {"target": str(target), "prior_file_present": original is not None,
             "prior_sha256": hashlib.sha256(original).hexdigest() if original is not None else None,
             "overlay_sha256": expected, "restored": False}
    atomic_write(target, payload)
    try:
        yield state
    finally:
        if not target.is_file() or hashlib.sha256(target.read_bytes()).hexdigest() != expected:
            state["restoration_conflict"] = True
            raise RuntimeError("Temporary profile changed externally; prior bytes preserved in run archive")
        if original is None:
            target.unlink()
        else:
            atomic_write(target, original)
        state["restored"] = True

