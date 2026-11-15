"""Read-only snapshot reliability tests using synthetic bytes only."""
import importlib.util
from pathlib import Path
import struct
import unittest

spec = importlib.util.spec_from_file_location("native_observation_tested", Path(__file__).resolve().parents[1] / "tools/native_observation.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class FStringObservationTests(unittest.TestCase):
    def test_utf16_path_and_empty_string(self):
        for text in ("F:/项目/Config/UserEngine.ini", ""):
            raw = (text + "\0").encode("utf-16-le") if text else b""
            header = struct.pack("<Qii", 0x2000 if raw else 0, len(raw) // 2, len(raw) // 2)
            def read(address, size):
                return (header if address == 0x1000 else raw)[:size]
            self.assertEqual(module.read_fstring(read, 0x1000), text)

    def test_racing_header_is_rejected(self):
        old = struct.pack("<Qii", 0x2000, 2, 2)
        new = struct.pack("<Qii", 0x3000, 2, 2)
        headers = iter((old, new))
        def read(address, size):
            return next(headers) if address == 0x1000 else b"A\0\0\0"
        with self.assertRaisesRegex(ValueError, "changed"):
            module.read_fstring(read, 0x1000)

    def test_out_of_bounds_count_is_rejected_before_payload_read(self):
        calls = []
        def read(address, size):
            calls.append((address, size))
            return struct.pack("<Qii", 0x2000, 65537, 65537)
        with self.assertRaises(ValueError):
            module.read_fstring(read, 0x1000)
        self.assertEqual(calls, [(0x1000, 16)])

