"""Separate persistent preferences and controller records for each backend."""
from __future__ import annotations

import json
import os
from pathlib import Path
import uuid

from training_settings_v2 import Settings

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL = 'original'
CANDIDATE = 'local-server-candidate'


def controller_directory(root=ROOT, backend=ORIGINAL):
    if backend not in (ORIGINAL, CANDIDATE):
        raise ValueError('Unknown local training backend')
    base = Path(root).resolve() / '.local/controller'
    return base if backend == ORIGINAL else base / CANDIDATE


def settings_path(root=ROOT, backend=ORIGINAL, settings_file=None):
    root = Path(root).resolve()
    original = (controller_directory(root, ORIGINAL) / 'config.json').resolve()
    if backend == ORIGINAL:
        if settings_file is not None:
            raise ValueError('--settings-file 仅用于候选后端，原训练器继续使用原设置文件')
        return original
    directory = controller_directory(root, backend).resolve()
    path = (Path(settings_file) if settings_file is not None else directory / 'config.json')
    if not path.is_absolute():
        path = root / path
    path = path.resolve()
    local, old_controller = (root / '.local').resolve(), controller_directory(root, ORIGINAL).resolve()
    if local not in path.parents or path.suffix.lower() != '.json':
        raise ValueError('候选设置文件必须是本项目 .local 内的 JSON 文件')
    if path == original or old_controller in path.parents and directory not in path.parents:
        raise ValueError('候选设置不能使用原训练器的设置或运行记录目录')
    if path.exists() and original.exists() and path.samefile(original):
        raise ValueError('候选设置不能指向原训练器的设置文件')
    return path


def _seed_once(path, value):
    """Publish a complete, new JSON file without replacing an existing store."""
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + '.' + uuid.uuid4().hex + '.seed')
    try:
        with temporary.open('x', encoding='utf-8', newline='\n') as stream:
            json.dump(value, stream, ensure_ascii=False, indent=2)
            stream.flush()
            os.fsync(stream.fileno())
        try:
            # A same-directory hard link publishes complete bytes atomically and
            # fails if another controller has already created its own store.
            os.link(temporary, path)
        except FileExistsError:
            pass
    finally:
        temporary.unlink(missing_ok=True)


def open_settings(root=ROOT, backend=ORIGINAL, settings_file=None):
    path = settings_path(root, backend, settings_file)
    if backend == CANDIDATE and not path.exists():
        original = settings_path(root, ORIGINAL)
        _seed_once(path, Settings(original).read())
    if backend == CANDIDATE:
        from human_bot_integration import HumanSettings
        return HumanSettings(path)
    return Settings(path)


def identity_matches_settings(identity, requested, *, root=ROOT, backend=ORIGINAL):
    published = identity.get('settings_file')
    if published is None:
        # Preserve reuse of older original consoles. An older candidate console
        # may still own the original store, so it cannot be reused implicitly.
        return backend == ORIGINAL and Path(requested).resolve() == settings_path(root, ORIGINAL)
    return isinstance(published, str) and Path(published).resolve() == Path(requested).resolve()
