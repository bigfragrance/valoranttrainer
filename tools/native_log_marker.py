"""Bounded incremental inspection of one owned client's append-only log."""
from pathlib import Path


class NativeLogMarker:
    def __init__(self, marker=b'Beginning platform initialization'):
        if not marker:
            raise ValueError('A nonempty log marker is required')
        self.marker = marker
        self.offset = 0
        self.carry = b''
        self.found = False

    def scan(self, path: Path, budget=1024*1024):
        if budget <= 0:
            raise ValueError('Log scan budget must be positive')
        size = path.stat().st_size
        if size < self.offset:
            self.offset, self.carry, self.found = 0, b'', False
        if self.found:
            return True
        with path.open('rb') as stream:
            stream.seek(self.offset)
            while budget > 0:
                chunk = stream.read(min(budget, 256*1024))
                if not chunk:
                    break
                self.offset += len(chunk)
                budget -= len(chunk)
                combined = self.carry+chunk
                self.found = self.marker in combined
                self.carry = combined[-(len(self.marker)-1):] if len(self.marker)>1 else b''
                if self.found:
                    return True
        return False
