"""Explicit, owned early ShooterMode startup; no existing process selector.

Only the caller's new Popen child and its existing pre-CDO primary hold are
used. The original GI factory remains the sole creator of Mode/GameState.
"""
from __future__ import annotations

import ctypes as C
from ctypes import wintypes as W
import hashlib
import json
import stat
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
MAGIC = 0x3154524154534D53


class _Options(C.Structure):
    _fields_ = [('magic', C.c_uint64), ('version', C.c_uint32), ('bytes', C.c_uint32),
                ('pid', C.c_uint32), ('primary_thread', C.c_uint32), ('image', C.c_uint64),
                ('host', C.c_uint32), ('bootstrap_engine_classes', C.c_uint32),
                ('allow_state_instance_override', C.c_uint32), ('timeout_ms', C.c_uint32),
                ('no_local_player_bootstrap', C.c_uint32), ('require_dedicated', C.c_uint32),
                ('reserved', C.c_uint32 * 2), ('package', W.WCHAR * 512),
                ('mode_class', W.WCHAR * 512), ('game_state_class', W.WCHAR * 512),
                ('report', W.WCHAR * 1024)]


class _Binding(C.Structure):
    _fields_ = [('magic', C.c_uint64), ('version', C.c_uint32), ('bytes', C.c_uint32),
                ('pid', C.c_uint32), ('primary_thread', C.c_uint32), ('image', C.c_uint64),
                ('owner', C.c_uint64), ('factory', C.c_uint64)]


def request(*, output, package, port, transport):
    """Build an opt-in request from the exact freshly created pair sidecar."""
    import local_server_module as module
    output = module.local_path(output)
    session = json.loads((output.parent / 'session.active.json').read_text(encoding='utf-8'))
    if (session.get('network_flavor') != 'playable' or session.get('training_mode') not in ('practice', 'deathmatch')
            or Path(session.get('directory', '')).resolve() != output.parent
            or session.get('port') != port or session.get('transport') != transport):
        raise ValueError('Original host mode requires the exact new local training pair')
    if not isinstance(package, str) or not package.startswith('/Game/Maps/') or '?' in package:
        raise ValueError('Original host mode requires the whitelisted map package')
    record = module.verified_build()
    required = {'native/local_server_mode_startup.h', 'native/local_server_mode_startup.generated.h'}
    if not required <= set(record.get('source_sha256', {})):
        raise RuntimeError('Compiled bridge does not contain original host-mode startup')
    binary = module.BUILD / module.MODULE_NAME
    exports = _exports(binary, (b'OfflineShooterModeConfigure', b'OfflineShooterModeCreateGameModeForURL'))
    return {'module': str(binary), 'module_sha256': record['module_sha256'],
            'exports': {'configure': exports[b'OfflineShooterModeConfigure'],
                        'factory': exports[b'OfflineShooterModeCreateGameModeForURL']},
            'output': str(output), 'package': package, 'port': port, 'transport': transport,
            'game_state_class': ('/Game/GameModes/Deathmatch/DeathmatchGameState.DeathmatchGameState_C'
                                 if session['training_mode'] == 'deathmatch' else '/Game/BaseGameState.BaseGameState_C')}


def _exports(module, names):
    import pefile
    pe = pefile.PE(str(module), fast_load=True)
    try:
        # Parse only the one needed directory, once for all requested names.
        # Request construction runs before Popen, outside the pre-CDO window.
        pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_EXPORT']])
        directory = getattr(pe, 'DIRECTORY_ENTRY_EXPORT', None)
        symbols = {e.name: e for e in directory.symbols} if directory else {}
        result = {}
        for name in names:
            symbol = symbols.get(name)
            if symbol is None:
                raise RuntimeError('Required owned startup export is absent: ' + name.decode('ascii'))
            value = symbol.address
            section = pe.get_section_by_rva(value)
            if (symbol.forwarder is not None or not 0 < value < pe.OPTIONAL_HEADER.SizeOfImage
                    or section is None or not section.Characteristics & 0x20000000):
                raise RuntimeError('Owned startup export is not original executable module code')
            result[name] = value
        return result
    finally:
        pe.close()


def _export(module, name):
    return _exports(module, (name,))[name]


def _archive_previous_pid_sidecars(owned, module, output, files):
    """Retain old launch inputs only before this owned child's first load."""
    existing = [path for path in files if path.exists() or path.is_symlink()]
    if not existing:
        return []
    if owned.api.GetProcessId(owned.handle) != owned.pid or owned.process.poll() is not None:
        raise RuntimeError('PID sidecar recovery requires the current live child handle')
    if module.name.casefold() in owned.modules():
        raise RuntimeError('PID sidecar recovery requires the bridge to be unloaded')
    creation_query = owned.api.GetProcessTimes
    creation_query.argtypes = [W.HANDLE] + [C.POINTER(W.FILETIME)] * 4
    creation_query.restype = W.BOOL
    times = [W.FILETIME() for _ in range(4)]
    if not creation_query(owned.handle, *(C.byref(value) for value in times)):
        raise C.WinError(C.get_last_error())
    creation = times[0].dwLowDateTime | times[0].dwHighDateTime << 32
    created_ns = (creation - 116444736000000000) * 100
    records = []
    for path in existing:
        before = path.lstat()
        birth_ns = getattr(before, 'st_birthtime_ns', before.st_ctime_ns)
        if (not stat.S_ISREG(before.st_mode) or created_ns <= 0
                or not 0 < birth_ns < created_ns or not 0 < before.st_mtime_ns < created_ns):
            raise RuntimeError('Original mode early startup refuses current or unverified PID sidecars')
        raw = path.read_bytes()
        if path.lstat() != before:
            raise RuntimeError('Previous PID sidecar changed during recovery observation')
        records.append({'name': path.name, 'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest(),
                        'birth_ns': birth_ns, 'mtime_ns': before.st_mtime_ns})
    # Validate both files before archiving either one. Windows rename refuses an
    # existing destination, preserving receipts if this preparation is repeated.
    archive = output / 'previous-pid-sidecars'
    archive.mkdir(exist_ok=False)
    receipt = {'pid': owned.pid, 'current_creation_filetime': creation, 'files': records}
    (archive / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    for path, record in zip(existing, records):
        current = path.lstat()
        if (current.st_mtime_ns != record['mtime_ns'] or current.st_size != record['bytes']
                or hashlib.sha256(path.read_bytes()).hexdigest() != record['sha256']):
            raise RuntimeError('Previous PID sidecar changed before archival')
        path.rename(archive / path.name)
    return records


def prepare_for_hold(owned, requested, primary, base):
    """Stage both exact PODs and load the verified bridge BEFORE suspension."""
    if C.sizeof(_Options) != 5184 or C.sizeof(_Binding) != 48:
        raise RuntimeError('Original mode startup POD ABI mismatch')
    module = Path(requested['module']).resolve(strict=True)
    allowed = (ROOT / '.local/local-server/module/local_server_bridge.dll').resolve()
    output = Path(requested['output']).resolve(strict=True)
    if (module != allowed or not output.is_relative_to((ROOT / '.local/local-server').resolve())
            or requested['transport'] not in ('ip', 'ares') or not 1024 <= requested['port'] <= 65535
            or hashlib.sha256(module.read_bytes()).hexdigest() != requested['module_sha256']):
        raise RuntimeError('Original mode startup module, output or transport identity changed')
    # These RVAs were resolved and executable-section checked before Popen.
    # Both existing module hashes and the loaded image-range check remain.
    exports = requested.get('exports', {})
    configure, factory = exports.get('configure'), exports.get('factory')
    if any(type(value) is not int or not 0 < value < 0x100000000 for value in (configure, factory)):
        raise RuntimeError('Original mode prelaunch export plan is absent or malformed')
    options = _Options()
    options.magic, options.version, options.bytes = MAGIC, 1, C.sizeof(options)
    options.pid, options.primary_thread, options.image = owned.pid, primary, base
    options.host, options.bootstrap_engine_classes, options.allow_state_instance_override = 1, 1, 1
    options.timeout_ms = 30000
    options.package = requested['package']
    options.mode_class = '/Script/ShooterGame.ShooterGameMode'
    options.game_state_class = requested['game_state_class']
    options.report = str(output / 'mode-startup-state.json')
    mode_file = module.parent / f'mode-startup-{owned.pid}.bin'
    network_file = module.parent / f'network-{owned.pid}.bin'
    previous_sidecars = _archive_previous_pid_sidecars(owned, module, output, (mode_file, network_file))
    directory = (str(output) + '\\').encode('utf-16le')
    if len(directory) >= 2048:
        raise ValueError('Original mode output path is too long')
    payload = struct.pack('<Q4I', 0x3150524F5754454E, 1, 1, requested['port'],
                          int(requested['transport'] == 'ip')) + directory.ljust(2048, b'\0')
    with mode_file.open('xb') as stream:
        stream.write(bytes(options))
    with network_file.open('xb') as stream:
        stream.write(payload)
    loaded = owned.load(module)
    if hashlib.sha256(module.read_bytes()).hexdigest() != requested['module_sha256']:
        raise RuntimeError('Original mode module changed during early loading')
    if not all(value < loaded[1] for value in (configure, factory)):
        raise RuntimeError('Original mode prelaunch exports exceed the actual loaded image')
    return {'options': options, 'loaded': loaded, 'configure': configure, 'factory': factory,
            'receipt': {'module': str(module), 'module_sha256': requested['module_sha256'],
                        'configured_under_original_primary_hold': False,
                        'original_gi_factory_bound_before_cdo': False,
                        'marker': str(output / 'bridge-state.json'), 'early_loaded': True,
                        'archived_previous_pid_sidecars': previous_sidecars}}


def activate_under_hold(owned, gi_loaded, prepared):
    """No Unreal calls occur in these two POD-only installer exports."""
    options, loaded = prepared['options'], prepared['loaded']
    code = owned.call(loaded[0] + prepared['configure'], bytes(options), operation="OfflineShooterModeConfigure",
                      details={"module": str(loaded[2]), "module_base": hex(loaded[0]), "entry_rva": prepared['configure']})
    if code:
        reason = {8713: 'startup configuration rejected',
                  8714: 'startup report is outside the owned session directory'}.get(code, 'native configure failed')
        raise RuntimeError(f'OfflineShooterModeConfigure returned {code}: {reason}')
    try:
        report = json.loads(Path(options.report).read_text(encoding='utf-8'))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise RuntimeError('OfflineShooterModeConfigure returned success but its startup report '
                           'is unavailable or invalid: ' + str(options.report)) from error
    if not isinstance(report, dict):
        raise RuntimeError('OfflineShooterModeConfigure returned an invalid startup report object')
    if report.get('configured') is not True or report.get('failed') or report.get('error'):
        raise RuntimeError('Original mode startup report rejected configuration: '
                           f"error={report.get('error')}, stage={report.get('stage')}")
    binding = _Binding()
    binding.magic, binding.version, binding.bytes = 0x31524F544341464D, 1, C.sizeof(binding)
    binding.pid, binding.primary_thread, binding.image = owned.pid, options.primary_thread, options.image
    binding.owner, binding.factory = loaded[0], loaded[0] + prepared['factory']
    offset = _export(gi_loaded[2], b'OfflineGiInstanceBindHostModeFactory')
    if owned.call(gi_loaded[0] + offset, bytes(binding), operation="OfflineGiInstanceBindHostModeFactory",
                  details={"module": str(gi_loaded[2]), "module_base": hex(gi_loaded[0]), "entry_rva": offset}):
        raise RuntimeError('Original GI host-mode factory did not bind before its first constructor')
    prepared['receipt']['configured_under_original_primary_hold'] = True
    prepared['receipt']['original_gi_factory_bound_before_cdo'] = True
