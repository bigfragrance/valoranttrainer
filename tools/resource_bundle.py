"""Restore selected original IoStore ranges into package-owned sparse files."""
from __future__ import annotations

import ctypes
import hashlib
import json
import os
from pathlib import Path
import re
import sys
import uuid

from client_layout import is_portable


MANIFEST = "resources/iostore-manifest.json"
PAKS = "client/ShooterGame/Content/Paks"
_HEX = re.compile(r"[0-9a-f]{64}\Z")
_NAME = re.compile(r"[A-Za-z0-9_-]+\.ucas\Z")


def _owned(root: Path, relative: str) -> Path:
    target = root / relative
    if target.resolve() != target or root not in target.parents:
        raise RuntimeError("Resource path resolves outside the package")
    if target.exists() and (not target.is_file() or target.stat().st_nlink != 1):
        raise RuntimeError("Resource output must be an ordinary package-owned file")
    return target


def _hash(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def _sparse_size(stream, size: int) -> None:
    if os.name != "nt":
        stream.truncate(size)
        return  # Unix truncate also creates holes; used by isolated fixture tests.
    import msvcrt
    from ctypes import wintypes as W
    api = ctypes.WinDLL("kernel32", use_last_error=True)
    api.DeviceIoControl.argtypes = [W.HANDLE, W.DWORD, ctypes.c_void_p, W.DWORD,
                                   ctypes.c_void_p, W.DWORD, ctypes.POINTER(W.DWORD), ctypes.c_void_p]
    api.DeviceIoControl.restype = W.BOOL
    returned = W.DWORD()
    if not api.DeviceIoControl(msvcrt.get_osfhandle(stream.fileno()), 0x900C4,
                               None, 0, None, 0, ctypes.byref(returned), None):
        raise RuntimeError("资源准备需要支持稀疏文件的磁盘，请将整个包解压到 NTFS 磁盘后重试。")
    # Windows CRT _chsize_s (used by file.truncate) writes zeroes while growing
    # and allocates the holes again. Set the sparse file's end directly instead.
    api.SetFilePointerEx.argtypes = [W.HANDLE, ctypes.c_longlong, ctypes.c_void_p, W.DWORD]
    api.SetFilePointerEx.restype = W.BOOL
    api.SetEndOfFile.argtypes = [W.HANDLE]
    api.SetEndOfFile.restype = W.BOOL
    handle = msvcrt.get_osfhandle(stream.fileno())
    if not api.SetFilePointerEx(handle, size, None, 0) or not api.SetEndOfFile(handle):
        raise ctypes.WinError(ctypes.get_last_error())
    stream.seek(0)


def allocated_bytes(path: Path) -> int:
    if os.name != 'nt':
        return path.stat().st_blocks * 512
    from ctypes import wintypes as W
    api = ctypes.WinDLL('kernel32', use_last_error=True)
    api.GetCompressedFileSizeW.argtypes = [W.LPCWSTR, ctypes.POINTER(W.DWORD)]
    api.GetCompressedFileSizeW.restype = W.DWORD
    high = W.DWORD()
    ctypes.set_last_error(0)
    low = api.GetCompressedFileSizeW(str(path), ctypes.byref(high))
    if low == 0xffffffff and ctypes.get_last_error():
        raise ctypes.WinError(ctypes.get_last_error())
    return (high.value << 32) | low


def _write_json(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + "." + uuid.uuid4().hex + ".tmp")
    try:
        with temporary.open("x", encoding="utf-8") as stream:
            json.dump(value, stream, ensure_ascii=False, indent=2)
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def _validate(root: Path, config: dict) -> tuple[dict, str]:
    manifest_path = _owned(root, MANIFEST)
    payload = manifest_path.read_bytes()
    digest = hashlib.sha256(payload).hexdigest()
    if digest != config.get("resources_manifest_sha256"):
        raise RuntimeError("资源清单校验失败，请重新完整解压测试包。")
    manifest = json.loads(payload)
    if manifest.get("schema_version") != 1 or not 1 <= len(manifest.get("files", [])) <= 64:
        raise RuntimeError("Unsupported resource manifest")
    seen = set()
    for item in manifest["files"]:
        name = item["file"]
        if not isinstance(name, str) or not _NAME.fullmatch(name) or name in seen:
            raise RuntimeError("Invalid resource file name")
        seen.add(name)
        if item["toc"] != name[:-5] + ".utoc" or not _HEX.fullmatch(item["toc_sha256"]):
            raise RuntimeError("Invalid original resource index")
        _owned(root, PAKS + "/" + name)
        toc = _owned(root, PAKS + "/" + item["toc"])
        if _hash(toc) != item["toc_sha256"]:
            raise RuntimeError("原资源索引校验失败，请重新完整解压测试包。")
        if not isinstance(item["logical_bytes"], int) or not 0 < item["logical_bytes"] < 80 * 1024**3:
            raise RuntimeError("Invalid original resource size")
        if not 1 <= len(item["ranges"]) <= 300000:
            raise RuntimeError("Invalid resource range count")
        end = total = 0
        for offset, size, sha in item["ranges"]:
            if not isinstance(offset, int) or not isinstance(size, int) or offset < end or size <= 0:
                raise RuntimeError("Invalid or overlapping resource ranges")
            end = offset + size
            total += size
            if end > item["logical_bytes"] or not _HEX.fullmatch(sha):
                raise RuntimeError("Resource range exceeds original file")
        if total != item["payload_bytes"]:
            raise RuntimeError("Resource payload size mismatch")
    return manifest, digest


def _verify_ranges(path: Path, item: dict) -> bool:
    if not path.is_file() or path.stat().st_size != item["logical_bytes"]:
        return False
    with path.open("rb") as stream:
        for offset, size, expected in item["ranges"]:
            stream.seek(offset)
            digest = hashlib.sha256()
            remaining = size
            while remaining:
                data = stream.read(min(1024 * 1024, remaining))
                if not data:
                    return False
                digest.update(data)
                remaining -= len(data)
            if digest.hexdigest() != expected:
                return False
    return True


def ensure_resources(root: Path, config: dict, *, verify=False) -> dict | None:
    """No source installation, network, re-encoding or index/signature changes."""
    if not is_portable(config):
        return None
    root = Path(root).resolve()
    if config.get('resource_mode') == 'downloaded-original-files-v1':
        from trainer_downloaded import prepare
        return prepare(root)
    if config.get('resource_mode') == 'installed-readonly-v1':
        return _development_readonly_resources(root, config, verify=verify)
    if config.get('resource_mode') is not None:
        raise RuntimeError('Unknown resource mode')
    manifest, digest = _validate(root, config)
    receipt_path = root / ".local/resource-installation.json"
    try:
        receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    except (FileNotFoundError, ValueError):
        receipt = {}
    previous = receipt.get("files", {}) if receipt.get("manifest_sha256") == digest else {}
    records = {}
    done = 0
    total = sum(item["payload_bytes"] for item in manifest["files"])
    progress = root / ".local/resource-progress.json"
    for item in manifest["files"]:
        target = _owned(root, PAKS + "/" + item["file"])
        stat = target.stat() if target.is_file() else None
        prior = previous.get(item["file"], {})
        unchanged = bool(stat and stat.st_size == item["logical_bytes"] and
                         stat.st_mtime_ns == prior.get("mtime_ns") and prior.get("range_sha256_verified"))
        valid = (unchanged and not verify) or _verify_ranges(target, item)
        if not valid:
            source = _owned(root, "resources/blocks/" + item["file"] + ".bin")
            if not source.is_file() or source.stat().st_size != item["payload_bytes"]:
                raise RuntimeError("所需的资源数据缺失，请重新完整解压测试包。")
            temporary = target.with_name(target.name + "." + uuid.uuid4().hex + ".assembling")
            try:
                target.parent.mkdir(parents=True, exist_ok=True)
                with source.open("rb", buffering=16 * 1024 * 1024) as incoming, temporary.open("x+b") as outgoing:
                    _sparse_size(outgoing, item["logical_bytes"])
                    for offset, size, expected in item["ranges"]:
                        outgoing.seek(offset)
                        sha = hashlib.sha256()
                        remaining = size
                        while remaining:
                            data = incoming.read(min(1024 * 1024, remaining))
                            if not data:
                                raise RuntimeError("Resource payload is truncated")
                            sha.update(data)
                            outgoing.write(data)
                            remaining -= len(data)
                        if sha.hexdigest() != expected:
                            raise RuntimeError("资源数据校验失败，请重新完整解压测试包。")
                    if incoming.read(1):
                        raise RuntimeError("Unexpected trailing resource payload")
                    outgoing.flush()
                    os.fsync(outgoing.fileno())
                os.replace(temporary, target)
            finally:
                temporary.unlink(missing_ok=True)
        stat = target.stat()
        records[item["file"]] = {"logical_bytes": stat.st_size, "allocated_bytes": allocated_bytes(target),
                                 "mtime_ns": stat.st_mtime_ns,
                                 "range_sha256_verified": True}
        done += item["payload_bytes"]
        _write_json(progress, {"complete": False, "prepared_bytes": done, "total_bytes": total,
                               "current_file": item["file"]})
        if sys.stdout:
            print(f"准备资源 {done * 100 // total}%: {item['file']}", flush=True)
    result = {"schema_version": 1, "manifest_sha256": digest, "files": records,
              "original_indices_preserved": True, "selected_payload_bytes": total, "complete": True}
    _write_json(receipt_path, result)
    if config.get('discard_resource_payload_after_prepare') is True:
        for item in manifest['files']:
            packed = _owned(root, 'resources/blocks/' + item['file'] + '.bin')
            if packed.is_file():
                packed.unlink()
        result['packed_payloads_removed'] = True
        _write_json(receipt_path, result)
    _write_json(progress, {"complete": True, "prepared_bytes": total, "total_bytes": total})
    return result


def _development_readonly_resources(root: Path, config: dict, *, verify=False) -> dict:
    """Strictly separate from hydration: this path never creates or writes Paks."""
    source = Path('F:/Riot Games/VALORANT/live/ShooterGame/Content/Paks')
    if Path(config.get('source_resource_directory', '')).resolve() != source.resolve(strict=True):
        raise RuntimeError('Development assets must use the pinned international installation')
    target = root / 'client/ShooterGame/Content/Paks'
    if target.resolve(strict=True) != source or not target.lstat().st_file_attributes & 0x400:
        raise RuntimeError('Development asset mount does not resolve to the pinned read-only source')
    manifest_path = (root / config['source_resource_manifest']).resolve(strict=True)
    if root / '.local' not in manifest_path.parents:
        raise RuntimeError('Development resource proof is outside the workspace')
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    records = {}
    receipt_path = root / '.local/development-resources.json'
    try:
        prior = json.loads(receipt_path.read_text(encoding='utf-8')).get('files', {})
    except (FileNotFoundError, ValueError):
        prior = {}
    for item in manifest['files']:
        name = item['file']
        if Path(name).name != name:
            raise RuntimeError('Invalid source resource filename')
        path = source / name
        stat = path.stat()
        if stat.st_size != item['logical_bytes']:
            raise RuntimeError('The installed resource version changed')
        # UCAS source bytes are complete. Its original UTOC/signature and all
        # other whole files are verified against the tested release identities.
        checked = not name.endswith('.ucas')
        cached = prior.get(name, {})
        if checked and (verify or cached.get('mtime_ns') != stat.st_mtime_ns or not cached.get('verified')):
            if not _verify_ranges(path, item):
                raise RuntimeError('The installed resource index/signature version changed')
        records[name] = {'bytes': stat.st_size, 'mtime_ns': stat.st_mtime_ns, 'verified': checked}
    result = {'complete': True, 'mode': 'installed-readonly-v1', 'source': str(source),
              'source_files_modified': False, 'hydration_performed': False, 'files': records}
    _write_json(receipt_path, result)
    return result
