"""Build and load the isolated ShooterGame local host/client candidate.

The accepted trainer and the DefaultPawn network proof keep their own binaries,
manifests and source snapshots. This module never promotes either of them.
"""
from __future__ import annotations

from datetime import datetime, timezone
from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import subprocess
import shutil
import tempfile
import threading

from local_environment import project_environment
import local_network_probe as network

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / '.local/local-server/module'
MODULE_NAME = 'local_server_bridge.dll'
FLAVOR = 'playable'
PIN = network.PIN
SOURCE_NAME = 'native/local_server_bridge.cpp'
SCRIPT_NAME = 'native/build_local_server.cmd'
PROFILE_NAME = 'native/local_server_profiles.h'
SNAPSHOT_NAME = '.local/local-server/module/source-snapshot'
PROFILE_RVAS = (0x3edab80, 0x4c2b940, 0x4742820, 0x5118750,
                0x5118500, 0x4c54840, 0x3df7850, 0x491b420,
                0x4ce4910, 0x4c2bcd0, 0x51173c0, 0x21a82e0,
                0x18cbef0, 0x18a2f10, 0x3edad30, 0x4313720,
                0x4717bf0, 0x42db600, 0x42ddc40, 0x47113e0, 0x1cf35b0,
                0x43fada0, 0x3ef6cb0, 0x431cd80, 0x42ed380,
                0x4733950, 0x4737550, 0x1c63870, 0x7859fe0, 0x785a040,
                0x716ee90, 0x716e5a0, 0x5118870)
LOCAL_INCLUDE = re.compile(r'^\s*#\s*include\s*"([^"\r\n]+)"', re.MULTILINE)
SHA256 = re.compile(r'[0-9a-f]{64}')
_PUBLICATION_LOCK = threading.RLock()
_PRODUCT_NAMES = (MODULE_NAME, 'source-snapshot', 'build.log', 'local_server_bridge.map',
                  'local_server_bridge.lib', 'local_server_bridge.exp', 'build.json')


def local_path(path):
    path = Path(path).resolve()
    if ROOT.resolve() not in path.parents or (ROOT / '.local').resolve() not in path.parents:
        raise ValueError('Local server outputs must stay under project .local')
    return path


def _build_directory():
    path = local_path(BUILD)
    if path != (ROOT / '.local/local-server/module').resolve() or any(
            char in str(path) for char in '&|<>^%!"\r\n'):
        raise ValueError('Local server builds require their isolated module directory')
    return path


def _source_name(name):
    if (not isinstance(name, str) or '\\' in name or ':' in name
            or not name.startswith('native/') or '..' in PurePosixPath(name).parts
            or not re.fullmatch(r'native/(?:[A-Za-z0-9_.-]+/)*[A-Za-z0-9_.-]+\.(?:h|hpp|inl|cpp|cmd)', name)):
        raise RuntimeError('Unexpected local server build input')
    return name


def capture_sources(base):
    """Read exactly the recursive quoted-include closure and its build script."""
    base = Path(base).resolve()
    native = (base / 'native').resolve()
    if base not in native.parents:
        raise RuntimeError('Local server source directory resolves outside its snapshot')
    pending = [SOURCE_NAME, SCRIPT_NAME]
    captured = {}
    while pending:
        name = _source_name(pending.pop())
        if name in captured:
            continue
        path = (base / name).resolve(strict=True)
        if native not in path.parents:
            raise RuntimeError('Local include resolves outside the captured native source')
        data = path.read_bytes()
        captured[name] = data
        if path.suffix == '.cmd':
            continue
        for included in LOCAL_INCLUDE.findall(data.decode('utf-8-sig')):
            include = Path(included)
            if include.is_absolute() or ':' in included or '\\' in included:
                raise RuntimeError('Local server includes must be relative native paths')
            target = (path.parent / include).resolve(strict=True)
            if native not in target.parents:
                raise RuntimeError('Local include resolves outside the captured native source')
            pending.append(target.relative_to(base).as_posix())
    if PROFILE_NAME not in captured:
        raise RuntimeError('Local server source closure is missing its generated profiles')
    return dict(sorted(captured.items()))


def profile_header(profiles):
    header = '#pragma once\nstruct CodeProfile { unsigned rva, bytes; unsigned long long fnv; };\n'
    header += 'static constexpr CodeProfile NativeProfiles[]{\n'
    header += ''.join('    {0x%x,%d,0x%xULL},\n' % (row['rva'], row['bytes'], row['fnv64']) for row in profiles)
    return (header + '};\n').encode('utf-8')


def _validate_profiles(profiles):
    if not isinstance(profiles, list) or len(profiles) != len(PROFILE_RVAS):
        raise RuntimeError('Local server build has an incomplete native profile inventory')
    for rva, row in zip(PROFILE_RVAS, profiles):
        if (not isinstance(row, dict) or row.get('rva') != rva
                or type(row.get('bytes')) is not int or not 0 < row['bytes'] < 65536
                or type(row.get('fnv64')) is not int or not 0 <= row['fnv64'] < 2**64
                or not isinstance(row.get('sha256'), str) or not SHA256.fullmatch(row['sha256'])):
            raise RuntimeError('Local server build has an invalid original-code profile')


@contextmanager
def _publication_lock(destination):
    """Readers/loaders and publishers share a short, process-owned file lock."""
    lock_path = local_path(destination.parent / 'module-publication.lock')
    lock_path.parent.mkdir(parents=True, exist_ok=True)
    with _PUBLICATION_LOCK, lock_path.open('a+b') as handle:
        if not handle.tell():
            handle.write(b'\0')
            handle.flush()
        handle.seek(0)
        if os.name == 'nt':
            import msvcrt
            msvcrt.locking(handle.fileno(), msvcrt.LK_LOCK, 1)
        else:
            import fcntl
            fcntl.flock(handle.fileno(), fcntl.LOCK_EX)
        try:
            yield
        finally:
            handle.seek(0)
            if os.name == 'nt':
                msvcrt.locking(handle.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                fcntl.flock(handle.fileno(), fcntl.LOCK_UN)


def _stage_path(stage, destination):
    """Validate the exact small temporary tree before moving or deleting it."""
    stage = Path(stage)
    resolved = local_path(stage)
    if (resolved != stage.absolute() or resolved.parent != destination.parent
            or not resolved.name.startswith('.module-stage-')):
        raise RuntimeError('Local server staging tree is outside its owned workspace directory')
    return resolved


class _RollbackError(RuntimeError):
    """Retain the named recovery tree if the filesystem refuses restoration."""


def _cleanup_stage(stage, destination):
    """Delete only a resolved, bounded tree of our small source/build outputs."""
    stage = _stage_path(stage, destination)
    total, count = 0, 0
    for current, directories, files in os.walk(stage, followlinks=False):
        count += len(directories) + len(files)
        if count > 4096:
            raise RuntimeError('Retain oversized staging tree: ' + str(stage))
        for name in directories + files:
            path = Path(current) / name
            if path.resolve() != path.absolute():
                raise RuntimeError('Retain staging tree containing a redirected path: ' + str(stage))
        for name in files:
            path = Path(current) / name
            if path.suffix not in ('.h', '.hpp', '.inl', '.cpp', '.cmd', '.dll', '.map', '.lib', '.exp', '.obj', '.log', '.json'):
                raise RuntimeError('Retain staging tree containing an unknown output: ' + str(path))
            total += path.stat().st_size
            if total > 128 * 1024 * 1024:
                raise RuntimeError('Retain oversized staging tree: ' + str(stage))
    shutil.rmtree(stage)


def _publish(candidate, destination, stage):
    """Commit a validated bundle, restoring every previous item on failure.

    The fixed public paths remain compatible with existing readers. Cooperating
    verified_build/load callers cannot observe the manifest-last transaction.
    """
    stage = _stage_path(stage, destination)
    previous = stage / 'previous'
    previous.mkdir()
    moved_old, installed = [], []
    with _publication_lock(destination):
        try:
            for name in _PRODUCT_NAMES:
                target, source = destination / name, candidate / name
                if target.exists() or target.is_symlink():
                    # A junction/symlink must never move an outside source tree.
                    if local_path(target) != target.absolute():
                        raise RuntimeError('Local server product path must not redirect outside its bundle')
                    target.replace(previous / name)
                    moved_old.append(name)
                if source.exists():
                    source.replace(target)
                    installed.append(name)
            return _verify_directory(destination)
        except BaseException as error:
            rollback_errors = []
            for name in reversed(installed):
                try:
                    (destination / name).replace(candidate / name)
                except BaseException as restore_error:
                    rollback_errors.append('move ' + str(destination / name) + ' back to '
                                           + str(candidate / name) + ': ' + str(restore_error))
            for name in reversed(moved_old):
                try:
                    (previous / name).replace(destination / name)
                except BaseException as restore_error:
                    rollback_errors.append('restore ' + str(previous / name) + ' to '
                                           + str(destination / name) + ': ' + str(restore_error))
            if rollback_errors:
                raise _RollbackError('Local server publication rollback incomplete; preserve recovery tree '
                                     + str(stage) + ': ' + '; '.join(rollback_errors)) from error
            raise


def build():
    """Compile captured inputs in isolation; publish only a verified bundle."""
    destination = _build_directory()
    destination.mkdir(parents=True, exist_ok=True)
    profiles = network.code_profiles(PROFILE_RVAS)
    _validate_profiles(profiles)
    before = capture_sources(ROOT)
    captured = {**before, PROFILE_NAME: profile_header(profiles)}
    stage = _stage_path(Path(tempfile.mkdtemp(prefix='.module-stage-', dir=destination.parent)), destination)
    candidate, preserve = stage / 'candidate', False
    try:
        snapshot = candidate / 'source-snapshot'
        for name, data in captured.items():
            target = snapshot / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        result = subprocess.run(
            [os.environ.get('COMSPEC', 'cmd.exe'), '/d', '/c', str(snapshot / SCRIPT_NAME), str(candidate)],
            cwd=snapshot, env=project_environment(ROOT), capture_output=True, text=True,
            creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0), timeout=120)
        (candidate / 'build.log').write_text(result.stdout + result.stderr, encoding='utf-8')
        if result.returncode:
            raise RuntimeError((result.stdout + result.stderr)[-6500:])
        if capture_sources(ROOT) != before:
            raise RuntimeError('Local server source changed during compilation; rebuild it')
        record = {
            'schema_version': 1, 'network_flavor': FLAVOR, 'source_image_sha256': PIN,
            'source_snapshot': SNAPSHOT_NAME,
            'source_sha256': {name: hashlib.sha256(data).hexdigest() for name, data in captured.items()},
            'module_sha256': hashlib.sha256((candidate / MODULE_NAME).read_bytes()).hexdigest(),
            'native_profiles': profiles, 'built_utc': datetime.now(timezone.utc).isoformat(),
            'compiled_from_captured_sources': True,
        }
        (candidate / 'build.json').write_text(json.dumps(record, indent=2), encoding='utf-8')
        _verify_directory(candidate)
        # The compiler may only read the captured closure, including its script.
        # Preserve the working-source consistency gate independently as well.
        if capture_sources(ROOT) != before:
            raise RuntimeError('Local server source changed during compilation; rebuild it')
        return _publish(candidate, destination, stage)
    except _RollbackError:
        preserve = True
        raise
    finally:
        attempt_log = candidate / 'build.log'
        if attempt_log.is_file():
            try:
                (destination.parent / 'last-build-attempt.log').write_bytes(attempt_log.read_bytes())
            except OSError:
                pass
        if not preserve:
            try:
                _cleanup_stage(stage, destination)
            except (OSError, RuntimeError) as error:
                # Never turn a successful publication into a failed build, or
                # mask its original failure merely because cleanup is deferred.
                try:
                    (destination.parent / 'last-build-cleanup.json').write_text(json.dumps(
                        {'retained_stage': str(stage), 'action': 'inspect retained small build outputs',
                         'error': str(error)}), encoding='utf-8')
                except OSError:
                    pass


def verified_build():
    """Verify the build snapshot, independent of subsequent source edits."""
    destination = _build_directory()
    with _publication_lock(destination):
        return _verify_directory(destination)


def _verify_directory(destination):
    """The same complete validation for staged and published fixed-layout bundles."""
    destination = local_path(destination)
    record = json.loads((destination / 'build.json').read_text(encoding='utf-8'))
    if (record.get('schema_version') != 1 or record.get('network_flavor') != FLAVOR
            or record.get('source_image_sha256') != PIN or record.get('source_snapshot') != SNAPSHOT_NAME):
        raise RuntimeError('Local server build identity or source snapshot mismatch')
    inventory = record.get('source_sha256')
    if not isinstance(inventory, dict) or not {SOURCE_NAME, SCRIPT_NAME, PROFILE_NAME} <= set(inventory):
        raise RuntimeError('Local server captured source inventory is incomplete')
    for name, digest in inventory.items():
        _source_name(name)
        if not isinstance(digest, str) or not SHA256.fullmatch(digest):
            raise RuntimeError('Local server captured source digest is invalid')
    snapshot = local_path(destination / 'source-snapshot')
    try:
        captured = capture_sources(snapshot)
    except (OSError, UnicodeError) as error:
        raise RuntimeError('Local server captured sources cannot be read') from error
    if (set(captured) != set(inventory) or any(
            hashlib.sha256(data).hexdigest() != inventory[name] for name, data in captured.items())):
        raise RuntimeError('Local server captured source closure or digest mismatch')
    profiles = record.get('native_profiles')
    _validate_profiles(profiles)
    if captured[PROFILE_NAME] != profile_header(profiles):
        raise RuntimeError('Local server captured profile header differs from its build record')
    digest = record.get('module_sha256')
    if (not isinstance(digest, str) or not SHA256.fullmatch(digest)
            or hashlib.sha256((destination / MODULE_NAME).read_bytes()).hexdigest() != digest):
        raise RuntimeError('Local server library changed after its build')
    return record


def owned_identity(process, executable):
    return network.owned_identity(process, executable, build_record=verified_build(), network_flavor=FLAVOR)


def validate_peer(path, proof_id):
    return network.validate_peer(path, proof_id, build_record=verified_build(), network_flavor=FLAVOR)


def pair_preflight(config, peer=None, proof_id=None, *, allow_exited=False):
    return network.pair_preflight(config, peer, proof_id, build_record=verified_build(),
                                  network_flavor=FLAVOR, allow_exited=allow_exited)


def load(process, context, *, role, port, output, transport='ip'):
    destination = _build_directory()
    with _publication_lock(destination):
        return network.load(process, context, role=role, port=port, output=local_path(output), transport=transport,
                            module=destination / MODULE_NAME, build_record=_verify_directory(destination))


if __name__ == '__main__':
    result = build()
    print(json.dumps({'module': str(BUILD / MODULE_NAME), 'sha256': result['module_sha256'],
                      'profile_count': len(result['native_profiles']), 'flavor': FLAVOR}))
