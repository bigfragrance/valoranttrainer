"""Opt-in, local-only ephemeral credentials for a genuine native Ares pair.

No account/session DTO is created. Payloads are private bootstrap inputs, never
included in receipts, logs, or returned metadata. Native installation and real
encrypted connectivity need independent verification.
"""
from __future__ import annotations

import base64
import hashlib
from pathlib import Path
import re
import secrets
import struct

PIN = '147763350504d241ee20b77607d04bc85cebfedb1d8bc2f32473ca93bf8fe92f'
MAGIC = 0x3159454B53455241
MARKER_NAME = 'native-local-key.bin'
LAYOUT = struct.Struct('<Q4I16sQ44sI32s')
assert LAYOUT.size == 128


class LocalPairKey:
    def __init__(self, directory, proof_id, port):
        if not re.fullmatch('[0-9a-f]{32}', proof_id or '') or not 1024 <= port <= 65535:
            raise ValueError('Native local credential requires a bound pair identity and loopback port')
        self.directory = Path(directory).resolve(strict=True)
        self._proof = bytes.fromhex(proof_id)
        self._port = port
        self._nonce = secrets.randbits(64) or 1
        self._encoded = base64.b64encode(secrets.token_bytes(32))
        self._created = {}

    def __repr__(self):
        return '<LocalPairKey redacted>'

    def write_role(self, role):
        if role not in ('host', 'client') or role in self._created or self._encoded is None:
            raise ValueError('Native local credential role must be written exactly once')
        role_dir = (self.directory / role).resolve(strict=True)
        if role_dir.parent != self.directory:
            raise ValueError('Native local credential must remain in its exact pair role directory')
        payload = LAYOUT.pack(MAGIC, 1, LAYOUT.size, 1 if role == 'host' else 2, self._port,
                              self._proof, self._nonce, self._encoded, 1, bytes.fromhex(PIN))
        path = role_dir / MARKER_NAME
        with path.open('xb') as stream:
            stream.write(payload)
        self._created[role] = (path, hashlib.sha256(payload).digest())
        if len(self._created) == 2:
            self._encoded = None
        return {'role': role, 'marker_bytes': len(payload), 'ephemeral': True,
                'source': 'fresh_local_host_pair_credential', 'native_installation_verified': False}

    def close(self):
        self._encoded = None
        for path, digest in self._created.values():
            if path.exists():
                if path.resolve().parent.parent != self.directory or hashlib.sha256(path.read_bytes()).digest() != digest:
                    raise RuntimeError('Native local credential file identity changed; cleanup refused')
                path.unlink()
        self._created.clear()
