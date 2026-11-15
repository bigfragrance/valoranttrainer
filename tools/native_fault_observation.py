"""Read the passive exception ring; no process access or UI operations."""
import struct

OFFSET = 3648
BYTES = 400
MAGIC = 0x31544c46


def sample(shared):
    for _ in range(3):
        before = struct.unpack_from('<I', shared, OFFSET + 4)[0]
        if before & 1:
            continue
        data = bytes(shared[OFFSET:OFFSET + BYTES])
        after = struct.unpack_from('<I', shared, OFFSET + 4)[0]
        magic, sequence, published, dropped = struct.unpack_from('<4I', data)
        if before != after or before != sequence or sequence & 1:
            continue
        if magic != MAGIC:
            return None
        records = []
        for index in range(3):
            raw = data[16 + 128 * index:16 + 128 * (index + 1)]
            count, ordinal = struct.unpack_from('<II', raw, 120)
            if not ordinal or ordinal > published or ordinal <= max(0, published - 3):
                continue
            uptime, tid, code, flags, context = struct.unpack_from('<Q4I', raw)
            addresses = struct.unpack_from('<12Q', raw, 24)
            names = ('exception_address', 'rip', 'rsp', 'rcx', 'rdx', 'r8', 'r9', 'rax', 'rbx', 'rbp', 'info0', 'info1')
            records.append({'sequence': ordinal, 'uptime_ms': uptime, 'thread_id': tid,
                            'exception_code': hex(code), 'exception_flags': flags, 'context_flags': context,
                            'parameter_count': count, **dict(zip(names, addresses))})
        return {'schema_version': 1, 'published': published, 'dropped_concurrent': dropped,
                'records': sorted(records, key=lambda row: row['sequence']),
                'qualification': 'First-chance observations do not prove an exception was unhandled; breakpoints may be dump capture contexts.'}
    return None
