"""Pinned original GameInstance with the engine-only offline lifecycle.

Two validated data pointers select the original engine-only lifecycle for the
original game-instance class. No instruction bytes or installation files change.
The child lifetime owns every in-memory change; this is not a general patch API.
"""
from __future__ import annotations

import ctypes
from ctypes import wintypes
import hashlib
from pathlib import Path
import struct

IMAGE_SHA256 = "147763350504d241ee20b77607d04bc85cebfedb1d8bc2f32473ca93bf8fe92f"
VTABLE = 0xb0869c0
LIFECYCLE = {'Init': (0x308, 0x7047190, 0x43c4260),
             'Shutdown': (0x310, 0x7047530, 0x43c48c0)}


def lifecycle_plan(image: Path):
    import pefile
    data=image.read_bytes()
    if hashlib.sha256(data).hexdigest()!=IMAGE_SHA256:
        raise ValueError('Original lifecycle image mismatch')
    pe=pefile.PE(data=data,fast_load=True)
    preferred=pe.OPTIONAL_HEADER.ImageBase
    result=[]
    for name,(slot,original,replacement) in LIFECYCLE.items():
        rva=VTABLE+slot
        section=pe.get_section_by_rva(rva)
        if section is None or section.Characteristics & 0x20000000:
            raise ValueError('Original lifecycle table is not non-executable image data')
        if struct.unpack('<Q',pe.get_data(rva,8))[0]!=preferred+original:
            raise ValueError('Original lifecycle table entry mismatch')
        for target in (original,replacement):
            target_section=pe.get_section_by_rva(target)
            if target_section is None or not target_section.Characteristics & 0x20000000:
                raise ValueError('Lifecycle target is not pinned original executable code')
        result.append({'name':name,'pointer_rva':rva,'original_target_rva':original,
                       'replacement_target_rva':replacement,'preferred_base':preferred})
    return result


def apply_to_owned_startup(process, plans):
    """Caller owns this new process and its pre-GameInstance log checkpoint."""
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    nt = ctypes.WinDLL("ntdll")
    handle = int(process._handle)
    kernel.ReadProcessMemory.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p,
                                        ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
    kernel.WriteProcessMemory.argtypes = kernel.ReadProcessMemory.argtypes
    kernel.VirtualProtectEx.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_size_t,
                                       wintypes.DWORD, ctypes.POINTER(wintypes.DWORD)]
    class MemoryInfo(ctypes.Structure):
        _fields_ = [("base", ctypes.c_void_p), ("allocation_base", ctypes.c_void_p),
                    ("allocation_protect", wintypes.DWORD), ("partition", wintypes.WORD),
                    ("size", ctypes.c_size_t), ("state", wintypes.DWORD),
                    ("protect", wintypes.DWORD), ("kind", wintypes.DWORD)]
    kernel.VirtualQueryEx.argtypes = [wintypes.HANDLE, ctypes.c_void_p,
                                     ctypes.POINTER(MemoryInfo), ctypes.c_size_t]
    kernel.VirtualQueryEx.restype = ctypes.c_size_t
    class BasicInfo(ctypes.Structure):
        _fields_ = [("reserved", ctypes.c_void_p), ("peb", ctypes.c_void_p),
                    ("reserved2", ctypes.c_void_p * 2), ("pid", ctypes.c_size_t),
                    ("reserved3", ctypes.c_void_p)]
    nt.NtQueryInformationProcess.argtypes = [wintypes.HANDLE, wintypes.ULONG, ctypes.c_void_p,
                                           wintypes.ULONG, ctypes.POINTER(wintypes.ULONG)]
    nt.NtQueryInformationProcess.restype = wintypes.LONG
    info, returned = BasicInfo(), wintypes.ULONG()
    if nt.NtQueryInformationProcess(handle, 0, ctypes.byref(info), ctypes.sizeof(info), ctypes.byref(returned)) < 0 or info.pid != process.pid:
        raise RuntimeError("Could not establish the owned suspended child's image")
    def read(address, size):
        data = ctypes.create_string_buffer(size)
        count = ctypes.c_size_t()
        if not kernel.ReadProcessMemory(handle, address, data, size, ctypes.byref(count)) or count.value != size:
            raise RuntimeError("Lifecycle pointer read failed")
        return data.raw
    base = struct.unpack("<Q", read(info.peb + 0x10, 8))[0]
    if read(base, 2) != b"MZ":
        raise RuntimeError("Owned image header mismatch")
    # A never-resumed PE can still contain preferred-base addresses. Preserve
    # that representation so loader relocation cannot apply ASLR twice.
    prepared=[]
    for item in plans:
        expected=LIFECYCLE.get(item['name'])
        if expected is None or (item['pointer_rva'],item['original_target_rva'],item['replacement_target_rva']) != (VTABLE+expected[0],expected[1],expected[2]):
            raise RuntimeError('Lifecycle plan is outside the two-entry allowlist')
        observed=struct.unpack('<Q',read(base+item['pointer_rva'],8))[0]
        if observed==base+expected[1]: representation=base
        elif observed==item['preferred_base']+expected[1]: representation=item['preferred_base']
        else: raise RuntimeError('Owned original GameInstance lifecycle pointer mismatch')
        prepared.append({**item,'flags_rva':item['pointer_rva'],
                         'original_hex':struct.pack('<Q',observed).hex(),
                         'configured_hex':struct.pack('<Q',representation+expected[2]).hex(),
                         'relocation_representation':'loaded' if representation==base else 'preferred'})
    plans=prepared
    applied=[]
    for item in plans:
        address = base + item["flags_rva"]
        old, ignored = wintypes.DWORD(), wintypes.DWORD()
        region = MemoryInfo()
        if not kernel.VirtualQueryEx(handle, address, ctypes.byref(region), ctypes.sizeof(region)):
            raise RuntimeError("Lifecycle pointer page query failed")
        # This fixed client maps its read-only image-data section as MEM_MAPPED
        # after startup; allocation base, pinned pointer values and static PE
        # section validation still bind these exact two slots to the owned image.
        if (region.state != 0x1000 or region.kind not in (0x1000000, 0x40000)
                or region.protect & (0xf0 | 0x100) or region.protect & 0xff not in (2, 4, 8)
                or region.allocation_base != base or not region.base <= address < address + 8 <= region.base + region.size):
            raise RuntimeError(f"Lifecycle table page mismatch: state={region.state:#x} kind={region.kind:#x} protection={region.protect:#x} allocation={region.allocation_base:#x} image={base:#x} region={region.base:#x}+{region.size:#x}")
        if not kernel.VirtualProtectEx(handle, address, 8, 8, ctypes.byref(old)):
            raise RuntimeError(f"Cannot obtain private copy-on-write lifecycle data: Win32 {ctypes.get_last_error()}")
        try:
            if old.value & (0xf0 | 0x100):
                raise RuntimeError("Lifecycle table page unexpectedly executable or guarded")
            payload = bytes.fromhex(item["configured_hex"])
            count = ctypes.c_size_t()
            buffer = ctypes.create_string_buffer(payload)
            if not kernel.WriteProcessMemory(handle, address, buffer, 8, ctypes.byref(count)) or count.value != 8:
                raise RuntimeError("Lifecycle pointer write failed")
            if read(address, 8) != payload:
                raise RuntimeError("Lifecycle pointer write did not verify")
            applied.append(item)
        finally:
            if not kernel.VirtualProtectEx(handle, address, 8, old.value, ctypes.byref(ignored)):
                raise RuntimeError("Lifecycle pointer page protection restoration failed")
    return {"profile_kind": "original_game_instance_engine_only_lifecycle", "applied": applied,
            "instruction_bytes_modified": False, "file_modified": False,
            "page_protections_restored": True}
