"""Fixed-version class-property configuration experiment in an owned suspended child.

Only the Config bit of named generated property descriptors is changed. No
instructions, game installation files, services or other processes are changed.
The child lifetime owns every in-memory change; this is not a general patch API.
"""
from __future__ import annotations

import ctypes
from ctypes import wintypes
import hashlib
from pathlib import Path
import struct

IMAGE_SHA256 = "147763350504d241ee20b77607d04bc85cebfedb1d8bc2f32473ca93bf8fe92f"
PROPERTIES = {
    "DefaultPawnClass": (0xab32ac0, 0x14000000000005, 0x4a0),
    "PlayerControllerClass": (0xab403c0, 0x14000002000015, 0x488),
    "GameStateClass": (0xab37570, 0x14000002000015, 0x480),
    "PlayerStateClass": (0xab40830, 0x14000002000015, 0x490),
}


def descriptor_plan(image: Path, profile: str):
    import pefile
    selected = {"pawn": ("DefaultPawnClass",), "player": tuple(PROPERTIES)}
    if profile not in selected:
        raise ValueError("Unknown fixed class-configuration experiment")
    data = image.read_bytes()
    if hashlib.sha256(data).hexdigest() != IMAGE_SHA256:
        raise ValueError("Class metadata experiment image mismatch")
    pe = pefile.PE(data=data, fast_load=True)
    plans = []
    for name in selected[profile]:
        descriptor, flags, offset = PROPERTIES[name]
        section = pe.get_section_by_rva(descriptor)
        if section is None or section.Characteristics & 0x20000000:
            raise ValueError("Class metadata is not in a non-executable section")
        raw = pe.get_data(descriptor, 64)
        if struct.unpack_from("<Q", raw, 0x10)[0] != flags or struct.unpack_from("<I", raw, 0x32)[0] != offset:
            raise ValueError("Class metadata descriptor does not match its pinned layout")
        before = struct.pack("<Q", flags)
        after = struct.pack("<Q", flags | 0x4000)
        plans.append({"property": name, "descriptor_rva": descriptor, "flags_rva": descriptor + 0x10,
                      "offset": offset, "original_hex": before.hex(), "configured_hex": after.hex()})
    return plans


def apply_to_suspended_child(process, plans):
    """Caller must own a newly created, never-resumed CREATE_SUSPENDED process."""
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
            raise RuntimeError("Class descriptor read failed")
        return data.raw
    base = struct.unpack("<Q", read(info.peb + 0x10, 8))[0]
    if read(base, 2) != b"MZ":
        raise RuntimeError("Owned image header mismatch")
    # Validate every original before the first write. ASLR pointer fields are
    # excluded; the flags and packed property offsets are not relocations.
    for item in plans:
        expected = PROPERTIES.get(item["property"])
        if expected is None or (item["descriptor_rva"], item["flags_rva"], item["offset"],
                item["original_hex"], item["configured_hex"]) != (
                expected[0], expected[0] + 0x10, expected[2], struct.pack("<Q", expected[1]).hex(),
                struct.pack("<Q", expected[1] | 0x4000).hex()):
            raise RuntimeError("Descriptor plan is outside the fixed allowlist")
        if read(base + item["flags_rva"], 8) != bytes.fromhex(item["original_hex"]):
            raise RuntimeError("In-memory class flags differ from the pinned descriptor")
        if struct.unpack("<I", read(base + item["descriptor_rva"] + 0x32, 4))[0] != item["offset"]:
            raise RuntimeError("In-memory class property offset differs")
    applied = []
    for item in plans:
        address = base + item["flags_rva"]
        old, ignored = wintypes.DWORD(), wintypes.DWORD()
        region = MemoryInfo()
        if not kernel.VirtualQueryEx(handle, address, ctypes.byref(region), ctypes.sizeof(region)):
            raise RuntimeError("Class descriptor page query failed")
        if (region.state != 0x1000 or region.kind != 0x1000000
                or region.protect & (0xf0 | 0x100) or region.protect & 0xff not in (2, 4, 8)
                or region.allocation_base != base or not region.base <= address < address + 8 <= region.base + region.size):
            raise RuntimeError("Descriptor is outside ordinary non-executable image data")
        if not kernel.VirtualProtectEx(handle, address, 8, 4, ctypes.byref(old)):
            raise RuntimeError("Cannot temporarily write the class descriptor data")
        try:
            if old.value & (0xf0 | 0x100):
                raise RuntimeError("Class metadata page unexpectedly executable or guarded")
            payload = bytes.fromhex(item["configured_hex"])
            count = ctypes.c_size_t()
            buffer = ctypes.create_string_buffer(payload)
            if not kernel.WriteProcessMemory(handle, address, buffer, 8, ctypes.byref(count)) or count.value != 8:
                raise RuntimeError("Class descriptor write failed")
            if read(address, 8) != payload:
                raise RuntimeError("Class descriptor write did not verify")
            applied.append(item)
        finally:
            if not kernel.VirtualProtectEx(handle, address, 8, old.value, ctypes.byref(ignored)):
                raise RuntimeError("Class descriptor page protection restoration failed")
    return {"profile_kind": "generated_property_config_flags", "applied": applied,
            "instruction_bytes_modified": False, "file_modified": False,
            "page_protections_restored": True}
