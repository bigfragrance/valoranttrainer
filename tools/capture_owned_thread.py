"""Snapshot only a launcher-bound training child's GameThread, then unwind offline.

The runner must publish owned_launch_identity(process, executable) in .active.json
using its original Popen handle. Missing/stale identities fail before OpenThread.
No debugger attachment, remote call, memory write, symbol server or game launch.
Only GetThreadContext and one bounded stack read occur while suspended; an 80 ms
watchdog and finally both restore only our one suspension increment. An existing
suspension is preserved. StackWalk64 runs after our increment has been restored.
"""
from __future__ import annotations

import argparse
import ctypes as C
from ctypes import wintypes as W
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import struct
import threading
import time

from client_layout import client_executable

ROOT = Path(__file__).resolve().parents[1]
MODULE = ROOT / '.local/native-module/trainer_bridge.dll'
CONTEXT_BYTES = 1232
MAX_STACK_BYTES = 64 * 1024
SUSPEND_BUDGET_SECONDS = .100


def _json(path):
    return json.loads(Path(path).read_text(encoding='utf-8'))


def _hash(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def _inside(path, root):
    path, root = Path(path).resolve(), Path(root).resolve()
    if path != root and root not in path.parents:
        raise RuntimeError('Observation path is outside the project-owned directory')
    return path


def _kernel():
    if os.name != 'nt' or C.sizeof(C.c_void_p) != 8:
        raise RuntimeError('Owned thread observation requires 64-bit Windows Python')
    k = C.WinDLL('kernel32', use_last_error=True)
    signatures = {
        'OpenProcess': ([W.DWORD, W.BOOL, W.DWORD], W.HANDLE),
        'GetProcessId': ([W.HANDLE], W.DWORD),
        'GetProcessIdOfThread': ([W.HANDLE], W.DWORD),
        'GetProcessTimes': ([W.HANDLE, C.POINTER(W.FILETIME), C.POINTER(W.FILETIME), C.POINTER(W.FILETIME), C.POINTER(W.FILETIME)], W.BOOL),
        'QueryFullProcessImageNameW': ([W.HANDLE, W.DWORD, W.LPWSTR, C.POINTER(W.DWORD)], W.BOOL),
        'WaitForSingleObject': ([W.HANDLE, W.DWORD], W.DWORD),
        'CloseHandle': ([W.HANDLE], W.BOOL),
        'CreateToolhelp32Snapshot': ([W.DWORD, W.DWORD], W.HANDLE),
        'OpenThread': ([W.DWORD, W.BOOL, W.DWORD], W.HANDLE),
        'SuspendThread': ([W.HANDLE], W.DWORD),
        'ResumeThread': ([W.HANDLE], W.DWORD),
        'GetThreadContext': ([W.HANDLE, C.c_void_p], W.BOOL),
        'CheckRemoteDebuggerPresent': ([W.HANDLE, C.POINTER(W.BOOL)], W.BOOL),
        'ReadProcessMemory': ([W.HANDLE, C.c_void_p, C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)], W.BOOL),
    }
    for name, (args, result) in signatures.items():
        getattr(k, name).argtypes = args
        getattr(k, name).restype = result
    return k


def _creation(k, handle):
    times = [W.FILETIME() for _ in range(4)]
    if not k.GetProcessTimes(handle, *(C.byref(x) for x in times)):
        raise C.WinError(C.get_last_error())
    return times[0].dwLowDateTime | times[0].dwHighDateTime << 32


def _image(k, handle):
    text = C.create_unicode_buffer(32768)
    size = W.DWORD(len(text))
    if not k.QueryFullProcessImageNameW(handle, 0, text, C.byref(size)):
        raise C.WinError(C.get_last_error())
    return Path(text.value).resolve()


def owned_launch_identity(process, executable, module_path=MODULE):
    """Call in the runner with its live, newly created Popen object, before tests.

    Merge the returned dict into the existing active record. This deliberately
    has no PID-only binding mode: a stale observer may not invent a new identity.
    """
    k = _kernel()
    config = _json(ROOT / 'project.json')
    executable, module_path = Path(executable).resolve(), Path(module_path).resolve()
    if executable != client_executable(config, ROOT) or module_path != MODULE.resolve():
        raise RuntimeError('Only the configured original client and project module can be bound')
    handle = int(process._handle)
    if k.GetProcessId(handle) != process.pid or _image(k, handle) != executable:
        raise RuntimeError('Original Popen handle does not match the configured child image')
    if k.WaitForSingleObject(handle, 0) != 0x102:
        raise RuntimeError('Original Popen child already exited')
    manifest = _json(ROOT / '.local/native-module/build.json')
    expected_module = manifest['sha256']['.local/native-module/trainer_bridge.dll']
    if _hash(module_path) != expected_module:
        raise RuntimeError('Module differs from its captured build')
    return {'owned_capture_identity': {
        'schema_version': 1, 'pid': process.pid,
        'creation_filetime': _creation(k, handle), 'executable': str(executable),
        'image_sha256': config['expected_snapshot_sha256'],
        'module_path': str(module_path), 'module_sha256': expected_module,
        'published_from_original_popen_handle': True,
    }}


class ModuleEntry(C.Structure):
    _fields_ = [('size', W.DWORD), ('id', W.DWORD), ('pid', W.DWORD),
                ('global_count', W.DWORD), ('process_count', W.DWORD),
                ('base', C.c_void_p), ('bytes', W.DWORD), ('module', W.HMODULE),
                ('name', W.WCHAR * 256), ('path', W.WCHAR * 260)]


def _modules(k, pid):
    k.Module32FirstW.argtypes = [W.HANDLE, C.POINTER(ModuleEntry)]
    k.Module32NextW.argtypes = [W.HANDLE, C.POINTER(ModuleEntry)]
    snapshot = k.CreateToolhelp32Snapshot(0x18, pid)
    if snapshot == W.HANDLE(-1).value:
        raise C.WinError(C.get_last_error())
    result = []
    try:
        item = ModuleEntry()
        item.size = C.sizeof(item)
        found = k.Module32FirstW(snapshot, C.byref(item))
        while found:
            if item.pid != pid or not item.base or not item.bytes:
                raise RuntimeError('Module snapshot is not bound to the requested child')
            path = Path(item.path).resolve()
            if str(path).startswith('\\\\'):
                raise RuntimeError('Network module paths are not supported')
            result.append({'name': item.name, 'path': str(path), 'base': item.base, 'bytes': item.bytes})
            if len(result) > 512:
                raise RuntimeError('Unexpected child module count')
            found = k.Module32NextW(snapshot, C.byref(item))
        if not result or C.get_last_error() != 18:
            raise RuntimeError('Owned module snapshot did not complete')
        return result
    finally:
        k.CloseHandle(snapshot)


def _validated_active(active_path):
    path = _inside(active_path, ROOT / '.local')
    if not path.name.endswith('.active.json'):
        raise RuntimeError('An explicit launcher .active.json is required')
    active = _json(path)
    identity = active.get('owned_capture_identity', {})
    required = ('pid', 'creation_filetime', 'executable', 'image_sha256', 'module_path', 'module_sha256')
    if identity.get('schema_version') != 1 or not identity.get('published_from_original_popen_handle') or any(key not in identity for key in required):
        raise RuntimeError('Active record lacks exact original-handle ownership identity; refusing thread access')
    pid = active.get('pid')
    if not isinstance(pid, int) or pid <= 0 or pid != identity['pid'] or not isinstance(identity['creation_filetime'], int) or identity['creation_filetime'] <= 0:
        raise RuntimeError('Invalid active process identity')
    run_dir = _inside(active['run_dir'], ROOT / '.local/native-baseline')
    # The wrapper writes launch.json only after its child exits. The live
    # ownership receipt must therefore be complete independently of that file.
    launch_path = run_dir / 'launch.json'
    launch = _json(launch_path) if launch_path.is_file() else None
    config = _json(ROOT / 'project.json')
    executable = client_executable(config, ROOT)
    if Path(identity['executable']).resolve() != executable or Path(identity['module_path']).resolve() != MODULE.resolve():
        raise RuntimeError('Active identity is outside the configured original installation/module')
    if identity['image_sha256'] != config['expected_snapshot_sha256'] or (launch is not None and launch.get('pid') != pid):
        raise RuntimeError('Active identity differs from the pinned launch')
    from native_maps import training_maps
    legacy_mode = active.get('mode') == 'navigation' and active.get('map_number') == 1
    controller_mode = active.get('mode') == 'training' and active.get('map_number') in training_maps()
    if not (legacy_mode or controller_mode) or active.get('module_stage') not in (6, 7, 8, 9):
        raise RuntimeError('Only explicit original-client training experiments are observable')
    if launch is not None and Path(launch['report_path']).resolve() != run_dir / 'report.json':
        raise RuntimeError('Launch report is not in the active child directory')
    manifest = _json(ROOT / '.local/native-module/build.json')
    if manifest['sha256']['.local/native-module/trainer_bridge.dll'] != identity['module_sha256']:
        raise RuntimeError('Build changed since ownership was published')
    if _hash(MODULE) != identity['module_sha256'] or _hash(executable) != identity['image_sha256']:
        raise RuntimeError('Pinned image or project module hash mismatch')
    return path, active, identity, run_dir, executable


def _read_memory(k, handle, address, size):
    data = C.create_string_buffer(size)
    read = C.c_size_t()
    success = k.ReadProcessMemory(handle, address, data, size, C.byref(read))
    if not success and not read.value:
        return b''
    return data.raw[:read.value]


def _thread_stack_bounds(k, process, thread, pid, tid):
    class ThreadBasic(C.Structure):
        _fields_ = [('status', W.LONG), ('teb', C.c_void_p),
                    ('pid', C.c_void_p), ('tid', C.c_void_p),
                    ('affinity', C.c_size_t), ('priority', W.LONG), ('base_priority', W.LONG)]
    if C.sizeof(ThreadBasic) != 48:
        raise RuntimeError('ThreadBasicInformation x64 ABI mismatch')
    ntdll = C.WinDLL(str(Path(os.environ['SystemRoot'])/'System32/ntdll.dll'), use_last_error=True)
    query = ntdll.NtQueryInformationThread
    query.argtypes = [W.HANDLE, W.DWORD, C.c_void_p, W.DWORD, C.POINTER(W.DWORD)]
    query.restype = W.LONG
    info, length = ThreadBasic(), W.DWORD()
    if query(thread, 0, C.byref(info), C.sizeof(info), C.byref(length)) < 0 or info.pid != pid or info.tid != tid or not info.teb:
        raise RuntimeError('Could not verify this owned thread TEB identity')
    tib = _read_memory(k, process, info.teb, 24)
    if len(tib) != 24:
        raise RuntimeError('Owned thread stack bounds are unreadable')
    base, limit = struct.unpack_from('<QQ', tib, 8)
    if not 0x10000 <= limit < base < 0x0000800000000000:
        raise RuntimeError('Owned thread stack bounds are invalid')
    return {'teb': info.teb, 'base': base, 'limit_at_observation': limit}


class Address64(C.Structure):
    _fields_ = [('offset', C.c_uint64), ('segment', W.WORD), ('mode', C.c_int)]


class KdHelp64(C.Structure):
    _fields_ = [('thread', C.c_uint64), ('callback_stack', W.DWORD), ('callback_bstore', W.DWORD),
                ('next_callback', W.DWORD), ('frame_pointer', W.DWORD),
                ('call_user', C.c_uint64), ('user_callback', C.c_uint64), ('system_start', C.c_uint64),
                ('user_exception', C.c_uint64), ('stack_base', C.c_uint64), ('stack_limit', C.c_uint64),
                ('build_version', W.DWORD), ('retpoline_count', W.DWORD), ('retpoline_table', C.c_uint64),
                ('retpoline_offset', W.DWORD), ('retpoline_size', W.DWORD), ('reserved', C.c_uint64 * 2)]


class StackFrame64(C.Structure):
    _fields_ = [('pc', Address64), ('ret', Address64), ('frame', Address64), ('stack', Address64),
                ('bstore', Address64), ('function_table', C.c_void_p), ('params', C.c_uint64 * 4),
                ('far', W.BOOL), ('virtual', W.BOOL), ('reserved', C.c_uint64 * 3), ('help', KdHelp64)]


def _address_record(address, modules):
    result = {'address': address, 'address_hex': hex(address)}
    for module in modules:
        if module['base'] <= address < module['base'] + module['bytes']:
            result.update(module=module['name'], rva=address-module['base'], rva_hex=hex(address-module['base']))
            break
    return result


def _debugger_observation(k, process):
    """Query only the already ownership-validated retained process handle."""
    result = {'api': 'CheckRemoteDebuggerPresent', 'queried': False,
              'attached': None, 'phase': 'before_owned_suspend_increment',
              'utc': datetime.now(timezone.utc).isoformat()}
    attached = W.BOOL()
    try:
        C.set_last_error(0)
        result['queried'] = bool(k.CheckRemoteDebuggerPresent(process, C.byref(attached)))
        if result['queried']:
            result['attached'] = bool(attached.value)
        else:
            result['win32_error'] = C.get_last_error()
    except Exception as error:
        result['error'] = str(error)[:200]
    return result


def _observe_streaming_manager(k, process, main, modules, registers, *, restored_at, thread_running):
    """Bounded, non-atomic RPM only, after our suspend increment is restored.

    Offsets are pinned-copy evidence, not a scan or a remote virtual call. None
    means unknown/unreadable; an observed null pointer remains an explicit zero.
    This observer must never be moved into the GetThreadContext/stack window.
    """
    started = time.perf_counter()
    result = {
        'schema_version': 1, 'status': 'unknown',
        'utc': datetime.now(timezone.utc).isoformat(),
        'phase': 'after_owned_suspend_increment_restored_before_stackwalk',
        'ms_after_owned_increment_restored': round((started-restored_at)*1000, 3),
        'owned_suspend_increment_held': False, 'game_thread_running': thread_running,
        'non_atomic_read': True, 'singleton_slot_rva': '0xda12fc0',
        'expected_singleton_table_rva': '0xab18d20',
        'expected_singleton_virtual8_rva': '0x42a95e0',
        'max_children': 64, 'read_call_budget': 160, 'read_byte_budget': 8192,
        'read_calls': 0, 'requested_bytes': 0, 'read_bytes': 0,
        'failures': [], 'children': [],
        'context_registers': {name: _address_record(registers[name], modules)
                              for name in ('rax', 'rcx') if name in registers},
    }

    def read(address, size, label):
        if (not isinstance(address, int) or not 0x10000 <= address
                or address+size >= 0x0000800000000000 or not 0 < size <= 512):
            result['failures'].append({'field': label, 'status': 'invalid_address_or_size'})
            return None
        if (result['read_calls'] >= result['read_call_budget']
                or result['requested_bytes']+size > result['read_byte_budget']
                or time.perf_counter()-started > .150):
            result['budget_exhausted'] = True
            result['failures'].append({'field': label, 'status': 'bounded_read_budget_exhausted'})
            return None
        result['read_calls'] += 1
        result['requested_bytes'] += size
        try:
            data = _read_memory(k, process, address, size)
        except Exception as error:
            result['failures'].append({'field': label, 'status': 'read_exception',
                                       'error': str(error)[:160]})
            return None
        result['read_bytes'] += len(data)
        if len(data) != size:
            result['failures'].append({'field': label, 'status': 'unknown_unreadable_or_partial',
                                       'requested_bytes': size, 'read_bytes': len(data)})
            return None
        return data

    def table_record(table, label):
        record = {'table': _address_record(table, modules),
                  'virtual8': None, 'virtual10': None, 'status': 'unknown'}
        if not table:
            record['status'] = 'observed_null_table'
            return record
        data = read(table, 0x18, label+'.table')
        if data is not None:
            record['virtual8'] = _address_record(struct.unpack_from('<Q', data, 8)[0], modules)
            record['virtual10'] = _address_record(struct.unpack_from('<Q', data, 0x10)[0], modules)
            record['status'] = 'observed'
        return record

    try:
        slot = main['base']+0xda12fc0
        result['singleton_slot'] = _address_record(slot, modules)
        pointer = read(slot, 8, 'singleton_pointer')
        if pointer is None:
            return result
        singleton = struct.unpack('<Q', pointer)[0]
        result['singleton'] = _address_record(singleton, modules)
        if not singleton:
            result['status'] = 'observed_null_singleton'
            return result
        header = read(singleton, 0x28, 'singleton_header')
        if header is None:
            return result
        table = struct.unpack_from('<Q', header)[0]
        children = struct.unpack_from('<Q', header, 0x10)[0]
        count, capacity, passes = struct.unpack_from('<iiI', header, 0x18)
        result.update(singleton_dispatch=table_record(table, 'singleton'),
                      child_array=_address_record(children, modules), child_count=count,
                      child_capacity=capacity, passes_u32_at20=passes,
                      passes_i32_at20=struct.unpack_from('<i', header, 0x20)[0])
        dispatch = result['singleton_dispatch']
        result['singleton_table_matches_pinned_default'] = table == main['base']+0xab18d20
        result['singleton_virtual8_matches_pinned_default'] = (
            dispatch['virtual8']['address'] == main['base']+0x42a95e0
            if dispatch['virtual8'] is not None else None)
        result['context_rcx_equals_singleton'] = registers.get('rcx') == singleton if 'rcx' in registers else None
        result['context_rax_equals_singleton_virtual8'] = (
            registers['rax'] == dispatch['virtual8']['address']
            if 'rax' in registers and dispatch['virtual8'] is not None else None)
        valid = 0 <= count <= capacity <= 1048576 and (not count or children != 0)
        result['child_array_header_valid'] = valid
        result['children_truncated'] = count > result['max_children'] if valid else None
        sampled_children = None
        if valid:
            observed_count = min(count, result['max_children'])
            data = read(children, observed_count*8, 'child_pointer_array') if observed_count else b''
            sampled_children = data
            if data is not None:
                for index, (child,) in enumerate(struct.iter_unpack('<Q', data)):
                    entry = {'index': index, 'child': _address_record(child, modules),
                             'dispatch': None, 'status': 'unknown'}
                    result['children'].append(entry)
                    if not child:
                        entry['status'] = 'observed_null_child'
                        continue
                    pointer = read(child, 8, f'child[{index}].table_pointer')
                    if pointer is None:
                        continue
                    child_table = struct.unpack('<Q', pointer)[0]
                    entry['dispatch'] = table_record(child_table, f'child[{index}]')
                    entry['status'] = entry['dispatch']['status']
                    entry['context_rcx_equals_child'] = registers.get('rcx') == child if 'rcx' in registers else None
                    update = entry['dispatch']['virtual10']
                    entry['context_rax_equals_update10'] = (
                        registers['rax'] == update['address'] if 'rax' in registers and update is not None else None)
        else:
            result['failures'].append({'field': 'child_pointer_array', 'status': 'invalid_array_header'})
        again = read(slot, 8, 'singleton_pointer_after')
        result['singleton_pointer_unchanged'] = again == struct.pack('<Q', singleton) if again is not None else None
        after = read(singleton, 0x28, 'singleton_header_after')
        result['singleton_header_unchanged'] = after == header if after is not None else None
        if sampled_children:
            after_children = read(children, len(sampled_children), 'child_pointer_array_after')
            result['child_pointer_array_unchanged'] = after_children == sampled_children if after_children is not None else None
        else:
            result['child_pointer_array_unchanged'] = True if sampled_children == b'' else None
        result['observed_child_count'] = len(result['children'])
        stable = [result['singleton_pointer_unchanged'], result['singleton_header_unchanged'],
                  result['child_pointer_array_unchanged']]
        result['changed_during_observation'] = False if all(x is True for x in stable) else (True if False in stable else None)
        result['status'] = ('changed_during_observation' if result['changed_during_observation']
                            else ('partial' if result['failures'] or result['children_truncated'] else 'observed'))
        return result
    except Exception as error:
        result['error'] = str(error)[:240]
        return result
    finally:
        result['elapsed_ms'] = round((time.perf_counter()-started)*1000, 3)


def _unwind(k, process, thread, context, stack_start, stack_copy, modules, *, thread_running):
    """Our increment is restored; any preexisting suspension remains untouched."""
    if C.sizeof(Address64) != 16 or C.sizeof(KdHelp64) != 112 or C.sizeof(StackFrame64) != 264:
        raise RuntimeError('Local DbgHelp x64 ABI mismatch')
    dll = Path(os.environ['SystemRoot']) / 'System32/dbghelp.dll'
    dbg = C.WinDLL(str(dll), use_last_error=True)
    dbg.SymSetOptions.argtypes = [W.DWORD]
    dbg.SymSetOptions.restype = W.DWORD
    # Local explicit paths only. Never use _NT_SYMBOL_PATH, symsrv or PDB prompts.
    dbg.SymSetOptions(0x4 | 0x200 | 0x1000 | 0x20000 | 0x40000 | 0x80000 | 0x200000 | 0x02000000 | 0x40000000)
    dbg.SymInitializeW.argtypes = [W.HANDLE, W.LPCWSTR, W.BOOL]
    dbg.SymInitializeW.restype = W.BOOL
    dbg.SymCleanup.argtypes = [W.HANDLE]
    dbg.SymLoadModuleExW.argtypes = [W.HANDLE, W.HANDLE, W.LPCWSTR, W.LPCWSTR, C.c_uint64, W.DWORD, C.c_void_p, W.DWORD]
    dbg.SymLoadModuleExW.restype = C.c_uint64
    dbg.SymFunctionTableAccess64.argtypes = [W.HANDLE, C.c_uint64]
    dbg.SymFunctionTableAccess64.restype = C.c_void_p
    dbg.SymGetModuleBase64.argtypes = [W.HANDLE, C.c_uint64]
    dbg.SymGetModuleBase64.restype = C.c_uint64
    reader_type = C.WINFUNCTYPE(W.BOOL, W.HANDLE, C.c_uint64, C.c_void_p, W.DWORD, C.POINTER(W.DWORD))

    @reader_type
    def reader(_process, address, destination, count, read):
        try:
            if stack_start <= address and address+count <= stack_start+len(stack_copy):
                data = stack_copy[address-stack_start:address-stack_start+count]
            elif any(m['base'] <= address and address+count <= m['base']+m['bytes'] for m in modules):
                data = _read_memory(k, process, address, count)
            else:
                read[0] = 0
                return False
            C.memmove(destination, data, len(data))
            read[0] = len(data)
            return len(data) == count
        except Exception:
            read[0] = 0
            return False

    dbg.StackWalk64.argtypes = [W.DWORD, W.HANDLE, W.HANDLE, C.POINTER(StackFrame64), C.c_void_p,
                               reader_type, C.c_void_p, C.c_void_p, C.c_void_p]
    dbg.StackWalk64.restype = W.BOOL
    search = ROOT / '.local/native-module/source-snapshot'
    if not dbg.SymInitializeW(process, str(search), False):
        raise C.WinError(C.get_last_error())
    try:
        loaded = 0
        for module in modules:
            if Path(module['path']).is_file():
                loaded += bool(dbg.SymLoadModuleExW(process, None, module['path'], module['name'], module['base'], module['bytes'], None, 0))
        raw = C.string_at(context, CONTEXT_BYTES)
        rsp, rbp, rip = (struct.unpack_from('<Q', raw, offset)[0] for offset in (152, 160, 248))
        frame = StackFrame64()
        frame.pc.offset, frame.stack.offset, frame.frame.offset = rip, rsp, rbp
        frame.pc.mode = frame.stack.mode = frame.frame.mode = 3
        frames = [dict(index=0, stack_pointer=rsp, **_address_record(rip, modules))]
        seen = {(rip, rsp)}
        for index in range(1, 65):
            if not dbg.StackWalk64(0x8664, process, thread, C.byref(frame), context, reader,
                                   C.cast(dbg.SymFunctionTableAccess64, C.c_void_p),
                                   C.cast(dbg.SymGetModuleBase64, C.c_void_p), None):
                break
            pair = (frame.pc.offset, frame.stack.offset)
            if not pair[0]:
                break
            # Some DbgHelp builds report the seeded top frame on their first
            # successful call, then advance on the second call.
            if pair in seen:
                if index == 1 and pair == (rip, rsp):
                    continue
                break
            seen.add(pair)
            frames.append(dict(index=len(frames), stack_pointer=pair[1], **_address_record(pair[0], modules)))
        return {'engine': 'local_system_dbghelp_StackWalk64', 'thread_resumed_before_unwind': thread_running,
                'owned_suspend_increment_restored_before_unwind': True,
                'symbol_server_used': False, 'loaded_module_count': loaded, 'frames': frames}
    finally:
        dbg.SymCleanup(process)


def capture_owned_thread(active_path, output_path=None, *, reason='stale_native_frame'):
    """Verifier hook: call before release/stop-request in its stale branch.

    Returns a JSON-ready report and writes output_path (default run_dir/hang-thread.json).
    Ownership failures never call OpenThread/SuspendThread. Capture failures still
    serialize an error and always restore our suspension before stack walking.
    """
    started = time.perf_counter()
    report = {'schema_version': 4, 'kind': 'owned_game_thread_snapshot',
              'utc': datetime.now(timezone.utc).isoformat(), 'reason': str(reason)[:200],
              'success': False, 'ownership_verified': False, 'suspend_performed': False,
              'thread_resumed': False, 'owned_suspend_increment_restored': False,
              'externally_suspended': False, 'suspend_budget_ms': 100,
              'debugger_attached': None, 'debugger_attachment_performed_by_observer': False,
              'memory_written': False, 'symbol_server_used': False}
    process = thread = None
    k = None
    output = None
    try:
        active_file, active, identity, run_dir, executable = _validated_active(active_path)
        output = _inside(output_path or run_dir/'hang-thread.json', ROOT / '.local')
        k = _kernel()
        process = k.OpenProcess(0x1000 | 0x400 | 0x10 | 0x100000, False, active['pid'])
        if not process:
            raise C.WinError(C.get_last_error())
        if k.GetProcessId(process) != active['pid'] or k.WaitForSingleObject(process, 0) != 0x102:
            raise RuntimeError('Bound child is not alive')
        if _creation(k, process) != identity['creation_filetime'] or _image(k, process) != executable:
            raise RuntimeError('PID was reused or process image differs from original ownership')
        modules = _modules(k, active['pid'])
        main = next((m for m in modules if Path(m['path']).resolve() == executable), None)
        training = next((m for m in modules if Path(m['path']).resolve() == MODULE.resolve()), None)
        if not main or not training:
            raise RuntimeError('Exact main image/project module are not both loaded')
        context_path = ROOT / f".local/native-module/context-{active['pid']}.bin"
        context_record = struct.unpack('<8Q', context_path.read_bytes())
        if context_record[:4] != (0x3152544c41564f4c, 1, active['pid'], main['base']) or context_record[7] != active['module_stage']:
            raise RuntimeError('Loaded module context does not match active PID/image/stage')
        marker = _json(ROOT / '.local/native-module/bridge-state.json')
        tid = marker.get('tick_thread')
        if marker.get('pid') != active['pid'] or not marker.get('module_loaded') or not isinstance(tid, int) or tid <= 0:
            raise RuntimeError('Project module has not published this child GameThread')
        report.update(pid=active['pid'], creation_filetime=identity['creation_filetime'],
                      executable=str(executable), image_base=main['base'], project_module=training,
                      game_thread=tid, last_operation=marker.get('operation'), tick_count=marker.get('tick_count'),
                      active_file=str(active_file), run_dir=str(run_dir))
        thread = k.OpenThread(0x2 | 0x8 | 0x40, False, tid)
        if not thread or k.GetProcessIdOfThread(thread) != active['pid']:
            raise RuntimeError('Published GameThread does not belong to the bound child')
        stack_bounds = _thread_stack_bounds(k, process, thread, active['pid'], tid)
        report['stack_bounds'] = stack_bounds
        # Verify again through the retained process handle immediately before suspension.
        if _creation(k, process) != identity['creation_filetime'] or k.WaitForSingleObject(process, 0) != 0x102:
            raise RuntimeError('Owned lifetime ended before thread capture')
        report['ownership_verified'] = True
        report['debugger_observation'] = _debugger_observation(k, process)
        report['debugger_attached'] = report['debugger_observation']['attached']
        storage = C.create_string_buffer(CONTEXT_BYTES + 15)
        address = (C.addressof(storage) + 15) & ~15
        C.c_uint32.from_address(address + 48).value = 0x10000B
        lock = threading.Lock()
        suspended = {'held': False, 'resumed_at': None, 'resume_error': None, 'resume_previous_count': None, 'watchdog_fired': False}

        def resume(from_watchdog=False):
            with lock:
                if suspended['held']:
                    suspended['watchdog_fired'] |= from_watchdog
                    count = k.ResumeThread(thread)
                    if count == 0xffffffff:
                        suspended['resume_error'] = C.get_last_error()
                    else:
                        suspended['held'] = False
                        suspended['resumed_at'] = time.perf_counter()
                        suspended['resume_previous_count'] = count

        armed, cancelled = threading.Event(), threading.Event()

        def watchdog():
            armed.wait()
            if not cancelled.wait(.080):
                resume(from_watchdog=True)

        # Start the watchdog before suspending: Python thread creation is never
        # part of the target's paused interval.
        timer = threading.Thread(target=watchdog, daemon=True)
        timer.start()
        suspend_started = time.perf_counter()
        stack_copy = b''
        raw_context = b''
        try:
            with lock:
                previous = k.SuspendThread(thread)
                if previous == 0xffffffff:
                    raise C.WinError(C.get_last_error())
                suspended['held'] = True
                report['suspend_performed'] = True
                report['prior_suspend_count'] = previous
                report['externally_suspended'] = previous > 0
                armed.set()
            # A preexisting suspension is not ours to remove. Our extra count
            # still makes GetThreadContext + the bounded stack snapshot safe;
            # finally/timeout subtract exactly this one extra increment only.
            if not k.GetThreadContext(thread, address):
                raise C.WinError(C.get_last_error())
            raw_context = C.string_at(address, CONTEXT_BYTES)
            rsp = struct.unpack_from('<Q', raw_context, 152)[0]
            if not 0x10000 <= rsp < stack_bounds['base']:
                raise RuntimeError('Invalid captured user-mode stack pointer')
            stack_copy = _read_memory(k, process, rsp, min(MAX_STACK_BYTES, stack_bounds['base']-rsp))
            with lock:
                if not suspended['held']:
                    raise RuntimeError('80 ms watchdog restored the thread before a consistent snapshot finished')
        finally:
            resume()
            cancelled.set()
            armed.set()
            timer.join(timeout=.1)
            report['owned_suspend_increment_restored'] = not suspended['held']
            report['resume_previous_count'] = suspended['resume_previous_count']
            remaining = None if suspended['resume_previous_count'] is None else max(0, suspended['resume_previous_count']-1)
            report['remaining_suspend_count_after_our_resume'] = remaining
            report['preexisting_suspend_count_preserved'] = not suspended['held'] and remaining == report.get('prior_suspend_count')
            report['thread_resumed'] = not suspended['held'] and remaining == 0
            report['resume_error'] = suspended['resume_error']
            report['watchdog_fired'] = suspended['watchdog_fired']
            end = suspended['resumed_at'] or time.perf_counter()
            report['suspended_ms'] = round((end-suspend_started)*1000, 3)
            report['within_suspend_budget'] = report['owned_suspend_increment_restored'] and end-suspend_started < SUSPEND_BUDGET_SECONDS
        if not report['owned_suspend_increment_restored']:
            raise RuntimeError('Failed to restore owned suspension; runner must close its own child')
        if not report['within_suspend_budget']:
            raise RuntimeError('Owned thread restored, but measured suspension exceeded 100 ms')
        if len(stack_copy) < 8:
            raise RuntimeError('No bounded stack bytes were readable')
        report['captured_stack_bytes'] = len(stack_copy)
        registers = ['rax', 'rcx', 'rdx', 'rbx', 'rsp', 'rbp', 'rsi', 'rdi',
                     'r8', 'r9', 'r10', 'r11', 'r12', 'r13', 'r14', 'r15', 'rip']
        report['registers'] = {name: struct.unpack_from('<Q', raw_context, 120+8*n)[0] for n, name in enumerate(registers)}
        report['instruction'] = _address_record(report['registers']['rip'], modules)
        # Read the original monitor's own clock/records after returning our
        # suspend increment. Simulation time alone cannot show a stale outer
        # engine heartbeat, and this observation never refreshes that heartbeat.
        try:
            from original_heartbeat_snapshot import snapshot as heartbeat_snapshot
            heartbeat_started = time.perf_counter()
            def heartbeat_read(at, size):
                if size > 16384 or time.perf_counter()-heartbeat_started > .150:
                    raise RuntimeError('Original heartbeat snapshot read budget exceeded')
                return _read_memory(k, process, at, size)
            report['original_heartbeat'] = heartbeat_snapshot(heartbeat_read, main['base'], tid)
        except Exception as error:
            report['original_heartbeat'] = {'error': str(error), 'read_only': True}
        report['streaming_manager'] = _observe_streaming_manager(
            k, process, main, modules, report['registers'],
            restored_at=end, thread_running=report['thread_resumed'])
        # StackWalk may mutate its context copy; the saved register JSON remains exact.
        report['unwind'] = _unwind(k, process, thread, address, rsp, stack_copy, modules, thread_running=report['thread_resumed'])
        report['success'] = len(report['unwind']['frames']) > 1
    except Exception as error:
        report['error'] = str(error)
    finally:
        if k is not None:
            if thread:
                k.CloseHandle(thread)
            if process:
                k.CloseHandle(process)
        report['elapsed_ms'] = round((time.perf_counter()-started)*1000, 3)
        if output is not None:
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--active', type=Path, required=True)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--reason', default='stale_native_frame')
    args = parser.parse_args()
    report = capture_owned_thread(args.active, args.output, reason=args.reason)
    print(json.dumps(report, ensure_ascii=False))
    return 0 if report['success'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
