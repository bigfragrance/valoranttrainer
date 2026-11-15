"""Capture a small dump of the exact launcher-owned training process."""
import argparse
import ctypes as C
from ctypes import wintypes as W
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import struct

from capture_owned_thread import ROOT, MODULE, _validated_active, _kernel, _creation, _image, _modules, _inside, _hash, _read_memory


def capture(active_file):
    _, active, identity, run, executable = _validated_active(active_file)
    kernel = _kernel()
    process = kernel.OpenProcess(0x1000 | 0x400 | 0x10 | 0x100000, False, active['pid'])
    if not process:
        raise C.WinError(C.get_last_error())
    output = run / ('owned-small-' + datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ') + '.dmp')
    file_handle = None
    try:
        if (_creation(kernel, process) != identity['creation_filetime'] or _image(kernel, process) != executable
                or kernel.WaitForSingleObject(process, 0) != 0x102):
            raise RuntimeError('Owned process lifetime no longer matches the launch identity')
        modules = _modules(kernel, active['pid'])
        main = next((m for m in modules if Path(m['path']).resolve() == executable), None)
        project = next((m for m in modules if Path(m['path']).resolve() == MODULE.resolve()), None)
        if not main or not project:
            raise RuntimeError('Exact client and project module are not both loaded')
        context = struct.unpack('<8Q', (ROOT/f'.local/native-module/context-{active["pid"]}.bin').read_bytes())
        if context[:4] != (0x3152544c41564f4c, 1, active['pid'], main['base']) or context[7] != active['module_stage']:
            raise RuntimeError('Owned native context differs from the active launch')
        _inside(output, ROOT/'.local')
        extra_pages = []
        extra_data = []
        stub = next((m for m in modules if m['name'].casefold() == 'stub.dll'), None)
        if stub:
            # The preceding verified dump stopped at these exact missing
            # unwind/code ranges. Subsequent offline walks established the
            # 16-byte cross-page unwind tail and exact modal thunk/text inputs.
            # Keep reads bounded; never change protection or copy the module.
            ranges = ((0x4a4000, 4096), (0x4a5000, 16), (0x972000, 4096), (0x9a1000, 4096),
                      (0x4a1dbb, 32), (0x38c248, 64), (0x38c288, 256),
                      (0x38699c, 64), (0x2c7dc0, 523),
                      (0x53b01c, 64), (0xa06d88, 2917), (0x2c4140, 16),
                      (0x4a6c94, 64), (0x80a74b, 58847), (0x9482ab, 256),
                      (0x3f1400, 48), (0x3ed908, 4))
            for rva, size in ranges:
                row = {'module': 'stub.dll', 'module_base': stub['base'], 'rva': rva, 'bytes': size}
                if rva + size > stub['bytes']:
                    row['error'] = 'Range lies outside the observed module'
                else:
                    try:
                        payload = _read_memory(kernel, process, stub['base'] + rva, size)
                        target = output.with_suffix(f'.stub-{rva:x}.bin')
                        target.write_bytes(payload)
                        row.update(path=str(target), sha256=_hash(target))
                    except (OSError, RuntimeError) as error:
                        row['error'] = str(error)
                extra_pages.append(row)
            # The preceding exact function's original loads established this
            # collection layout: +8 begin, +16 end, stride0xb0. Read only that
            # bounded collection in this same owned process, not an old PID's
            # cached heap address. An unchanged header is recorded explicitly.
            record = {'kind': 'original_report_collection', 'module': 'stub.dll',
                      'header_rva': 0x3f1400, 'stride': 0xb0, 'maximum_entries': 64}
            try:
                header = _read_memory(kernel, process, stub['base'] + 0x3f1400, 48)
                begin, end = struct.unpack_from('<QQ', header, 8)
                if not (0x10000 <= begin <= end < 0x0000800000000000 and
                        (end-begin) % 0xb0 == 0 and 0 < end-begin <= 64*0xb0):
                    raise RuntimeError('Original collection header exceeds the established read bound')
                payload = _read_memory(kernel, process, begin, end-begin)
                target = output.with_suffix(f'.data-{begin:x}.bin')
                target.write_bytes(payload)
                record.update(address=begin, bytes=end-begin, entries=(end-begin)//0xb0,
                              path=str(target), sha256=_hash(target),
                              header_unchanged=header == _read_memory(kernel, process, stub['base'] + 0x3f1400, 48))
            except (OSError, RuntimeError) as error:
                record['error'] = str(error)
            extra_data.append(record)
        kernel.CreateFileW.argtypes = [W.LPCWSTR, W.DWORD, W.DWORD, C.c_void_p, W.DWORD, W.DWORD, W.HANDLE]
        kernel.CreateFileW.restype = W.HANDLE
        file_handle = kernel.CreateFileW(str(output), 0x40000000, 1, None, 1, 0x80, None)
        if file_handle == W.HANDLE(-1).value:
            file_handle = None
            raise C.WinError(C.get_last_error())
        debug = C.WinDLL(str(Path(os.environ['SystemRoot'])/'System32/dbghelp.dll'), use_last_error=True)
        debug.MiniDumpWriteDump.argtypes = [W.HANDLE, W.DWORD, W.HANDLE, W.DWORD, C.c_void_p, C.c_void_p, C.c_void_p]
        debug.MiniDumpWriteDump.restype = W.BOOL
        # The original report frame holds its actual error wstring by pointer.
        # Include memory indirectly referenced by captured thread stacks, so
        # that field survives a closed error window. This still excludes full
        # process memory, handle data, account files and unrelated processes.
        flags = 0x1000 | 0x800 | 0x20 | 0x20000 | 0x40
        if not debug.MiniDumpWriteDump(process, active['pid'], file_handle, flags, None, None, None):
            raise C.WinError(C.get_last_error())
        kernel.CloseHandle(file_handle)
        file_handle = None
        with output.open('rb') as stream:
            header = stream.read(32)
            count, directory = struct.unpack_from('<II', header, 8)
            stream.seek(directory)
            entries = stream.read(count*12)
        if header[:4] != b'MDMP' or not count or not any(entries):
            raise RuntimeError('Dump directory was not committed')
        result = {'utc': datetime.now(timezone.utc).isoformat(), 'pid': active['pid'], 'owned_creation_filetime': identity['creation_filetime'],
                  'dump': str(output), 'bytes': output.stat().st_size, 'sha256': _hash(output),
                  'streams': count, 'directory_committed': True, 'full_process_memory': False,
                  'indirect_stack_references_included': True,
                  'debugger_attached_by_capture': False, 'exception_context_supplied': False,
                  'extra_unwind_pages': extra_pages, 'extra_data_ranges': extra_data,
                  'page_protection_modified': False}
        output.with_suffix('.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
        return result
    finally:
        if file_handle:
            kernel.CloseHandle(file_handle)
        kernel.CloseHandle(process)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--active', type=Path)
    args = parser.parse_args()
    active = args.active
    if active is None:
        latest = json.loads((ROOT/'.local/latest-training-session.json').read_text(encoding='utf-8'))
        session = _inside(latest['session'], ROOT/'.local/training-sessions')
        active = session/'result.active.json'
    print(json.dumps(capture(active)))


if __name__ == '__main__':
    main()
