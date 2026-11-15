"""Pure Python encoder for the frozen, independently versioned command-17 wire.

Does not open a process, a port, or an MMF. A Commit ACK is transport-only.
"""
from __future__ import annotations

from dataclasses import dataclass
import os
from pathlib import Path
import secrets
import struct

import humanlike_settings as settings

MAGIC = 0x3130544f42485456
VERSION = 1
HEADER = struct.Struct('<Q16I32s3Q')
HEADER_BYTES = 128
COMMAND_CAPACITY = 16384
PARAM_COUNT = settings.PARAM_COUNT
RECORD_BYTES = 16 + PARAM_COUNT * 4
MAX_PROFILES = 32
INITIAL, STAGE, COMMIT, ABORT = 1, 2, 3, 4
ENABLED = 1
FNV_OFFSET, FNV_PRIME = 0xcbf29ce484222325, 0x100000001b3
_VALUES = struct.Struct('<' + str(PARAM_COUNT) + 'f')
_LEGACY_INITIAL = struct.Struct('<Q2I')
assert HEADER.size == HEADER_BYTES
assert HEADER_BYTES + RECORD_BYTES <= COMMAND_CAPACITY


def fnv(data, value=FNV_OFFSET):
    for byte in data:
        value = ((value ^ byte) * FNV_PRIME) & 0xffffffffffffffff
    return value


@dataclass(frozen=True)
class Bundle:
    revision: int
    seed: int
    flags: int
    identities: tuple[int, ...]
    transaction_id: int
    records: tuple[bytes, ...]
    fingerprints: tuple[int, ...]
    checksum: int

    def _frame(self, operation, part_index, part_count, first_profile, records):
        payload = b''.join(records)
        header = HEADER.pack(MAGIC, VERSION, HEADER_BYTES, HEADER_BYTES + len(payload), operation,
                             self.revision, self.seed, self.flags, len(self.identities), PARAM_COUNT,
                             RECORD_BYTES, part_index, part_count, first_profile, len(records), len(payload), 0,
                             settings.SCHEMA_DIGEST, self.checksum, self.transaction_id, 0)
        return header + payload

    def initial(self):
        return self._frame(INITIAL, 0, 1, 0, self.records)

    def stages(self):
        per_part = (COMMAND_CAPACITY - HEADER_BYTES) // RECORD_BYTES
        count = (len(self.records) + per_part - 1) // per_part
        return tuple(self._frame(STAGE, part, count, part * per_part,
                                 self.records[part * per_part:(part + 1) * per_part])
                     for part in range(count))

    def commit(self):
        return self._frame(COMMIT, len(self.stages()), len(self.stages()), len(self.records), ())

    def abort(self):
        return self._frame(ABORT, 0, len(self.stages()), 0, ())

    def summary(self):
        return {'schema_digest': settings.SCHEMA_DIGEST.hex(), 'revision': self.revision,
                'seed': self.seed, 'enabled': bool(self.flags & ENABLED),
                'profile_count': len(self.identities), 'param_count': PARAM_COUNT,
                'record_bytes': RECORD_BYTES, 'transaction_id': f'{self.transaction_id:016x}',
                'content_checksum': f'{self.checksum:016x}',
                'fingerprints': [f'{v:016x}' for v in self.fingerprints],
                'stable_bot_ids': list(self.identities), 'stage_count': len(self.stages()),
                'initial_bytes': len(self.initial()), 'max_command_bytes': max(map(len, self.stages()))}


def make_bundle(config, *, seed, revision, identities, transaction_id=None):
    settings._uint(revision, 'revision', 0x7fffffff)
    settings._uint(seed, 'seed', minimum=1)
    transaction_id = (secrets.randbits(64) or 1) if transaction_id is None else transaction_id
    settings._uint(transaction_id, 'transactionId', 0xffffffffffffffff, 1)
    config = settings.validate(config)
    profiles = settings.resolve_roster(config, seed, identities)
    records, fingerprints = [], []
    for roster_index, (identity, profile) in enumerate(zip(identities, profiles)):
        identity_bytes = struct.pack('<2I', roster_index, identity)
        values = _VALUES.pack(*profile)
        fingerprint = fnv(identity_bytes + values)
        records.append(identity_bytes + struct.pack('<Q', fingerprint) + values)
        fingerprints.append(fingerprint)
    flags = ENABLED if config['enabled'] else 0
    prefix = struct.pack('<6I', revision, seed, flags, len(profiles), PARAM_COUNT, RECORD_BYTES)
    prefix += settings.SCHEMA_DIGEST + struct.pack('<Q', transaction_id)
    checksum = fnv(b''.join(records), fnv(prefix))
    return Bundle(revision, seed, flags, tuple(identities), transaction_id,
                  tuple(records), tuple(fingerprints), checksum)


def write_initial_sidefile(validated_session_initial, bundle, *, root=None):
    """Only the fixed sibling of the existing owned/validated 7844-byte Initial.

    Callers still run their existing session ownership/path validation first.
    This extra shape check neither weakens nor replaces that validation.
    """
    if root is None:
        root = next((p for p in Path(__file__).resolve().parents
                     if (p / 'project.json').is_file() and (p / 'native').is_dir()), None)
        if root is None:
            raise settings.ConfigError('无法确认本项目根目录')
    project_root = Path(root).resolve(strict=True)
    initial = Path(validated_session_initial).resolve(strict=True)
    if initial.name != 'session-initial.bin' or not initial.is_relative_to(project_root):
        raise settings.ConfigError('Sidefile只可写入本项目已验证session-initial.bin的目录')
    original = initial.read_bytes()
    if len(original) != 7844 or _LEGACY_INITIAL.unpack_from(original) != (0x32304C5443525456, 2, 7844):
        raise settings.ConfigError('原Initial7844形状不匹配')
    target = initial.with_name('session-human-bots.bin')
    if target.exists():
        raise FileExistsError('已有human-bots Initial侧文件不能被新配置静默替换')
    temporary = target.with_name(target.name + f'.{bundle.transaction_id:016x}.tmp')
    with temporary.open('xb') as stream:
        stream.write(bundle.initial())
        stream.flush()
        os.fsync(stream.fileno())
    # Windows rename refuses a pre-existing target, including a race after the
    # exists() check. The caller only writes a newly owned pair's Initial once.
    try:
        temporary.rename(target)
    except Exception:
        temporary.unlink(missing_ok=True)
        raise
    return target
