"""Check audio-adapter byte profiles against a file; never executes the target."""
from __future__ import annotations

import argparse
import hashlib
import json
import mmap
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
PINNED_SHA256 = '147763350504d241ee20b77607d04bc85cebfedb1d8bc2f32473ca93bf8fe92f'


def verify(path):
    listener_source = (ROOT/'native/original_audio_listeners.h').read_text(encoding='utf-8')
    profiles = re.findall(r'Bytes\((0x[0-9a-f]+),(\d+),(0x[0-9a-f]+)ULL\)', listener_source)
    if len(profiles) != 6:
        raise ValueError('Expected the six complete original listener bodies')
    with Path(path).open('rb') as stream, mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as image:
        digest = hashlib.sha256(image).hexdigest()
        if digest != PINNED_SHA256:
            raise ValueError('Selected file is not the pinned international Shipping artifact')
        pe = struct.unpack_from('<I', image, 0x3c)[0]
        if image[pe:pe+4] != b'PE\0\0' or struct.unpack_from('<H', image, pe+4)[0] != 0x8664:
            raise ValueError('Expected the original x64 PE')
        count = struct.unpack_from('<H', image, pe+6)[0]
        optional_bytes = struct.unpack_from('<H', image, pe+20)[0]
        section_table = pe+24+optional_bytes
        sections = []
        for n in range(count):
            off = section_table+n*40
            virtual, rva, size, raw = struct.unpack_from('<4I', image, off+8)
            sections.append((rva, size, raw, virtual))

        def bytes_at(rva, size):
            for start, raw_size, raw, _ in sections:
                delta = rva-start
                if 0 <= delta and delta+size <= raw_size:
                    return image[raw+delta:raw+delta+size]
            raise ValueError('Code profile is outside a file-backed PE section')

        for rva, size, expected in profiles:
            value = 14695981039346656037
            for byte in bytes_at(int(rva, 16), int(size)):
                value = ((value ^ byte)*1099511628211) & 0xffffffffffffffff
            if value != int(expected, 16):
                raise ValueError('Original listener body profile mismatch at '+rva)

        spectated = (ROOT/'native/original_audio_spectated_consumers.h').read_text(encoding='utf-8')
        extra_profiles = re.findall(r'\{(0x[0-9a-f]+),(\d+),(0x[0-9a-f]+)ULL\}', spectated)
        if len(extra_profiles) != 8:
            raise ValueError('Expected eight original spectated-query frame profiles')
        for rva, size, expected in extra_profiles:
            value = 14695981039346656037
            for byte in bytes_at(int(rva, 16), int(size)):
                value = ((value ^ byte)*1099511628211) & 0xffffffffffffffff
            if value != int(expected, 16):
                raise ValueError('Original spectated frame profile mismatch at '+rva)

        locality = (ROOT/'native/original_npc_locality.h').read_text(encoding='utf-8')
        signatures = {'ListenerRegistrationBody': 0x596d2d1, 'PawnLocalBody': 0x48548e0,
                      'ViewHelperBody': 0x786b550, 'BlueprintLocalQueryThunk': 0x4852990,
                      'FrameConstructionBody': 0x1cdcad7}
        for name, rva in signatures.items():
            block = re.search(r'\b'+name+r'\[\]\{(.*?)\};', locality, re.S)
            if not block:
                raise ValueError('Missing native stack signature '+name)
            expected = bytes(int(x, 16) for x in re.findall(r'0x([0-9a-fA-F]+)', block.group(1)))
            if not expected or bytes_at(rva, len(expected)) != expected:
                raise ValueError('Native stack signature mismatch '+name)
    return dict(passed=True, source_image_sha256=digest, complete_native_bodies=6,
                native_stack_signatures=5, spectated_query_profiles=8,
                target_executed=False, live_process_access=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--image', type=Path, default=ROOT/'work/client-analysis/Shipping-analysis-copy.exe')
    args = parser.parse_args()
    print(json.dumps(verify(args.image)))


if __name__ == '__main__':
    main()
