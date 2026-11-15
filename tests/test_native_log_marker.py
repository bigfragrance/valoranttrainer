from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from native_log_marker import NativeLogMarker


class NativeLogMarkerTests(unittest.TestCase):
    def test_marker_split_across_appends_is_detected(self):
        with tempfile.TemporaryDirectory() as folder:
            log=Path(folder)/'runtime.log'
            scanner=NativeLogMarker()
            log.write_bytes(b'prefix\nBeginning platform ini')
            self.assertFalse(scanner.scan(log))
            with log.open('ab') as stream:
                stream.write(b'tialization\n')
            self.assertTrue(scanner.scan(log))

    def test_log_growth_is_read_once_with_a_bounded_scan(self):
        with tempfile.TemporaryDirectory() as folder:
            log=Path(folder)/'runtime.log'
            scanner=NativeLogMarker()
            log.write_bytes(b'x'*(2*1024*1024)+scanner.marker)
            self.assertFalse(scanner.scan(log))
            self.assertEqual(scanner.offset,1024*1024)
            self.assertFalse(scanner.scan(log))
            self.assertTrue(scanner.scan(log))
            offset=scanner.offset
            self.assertTrue(scanner.scan(log))
            self.assertEqual(scanner.offset,offset)

    def test_truncated_owned_log_is_scanned_from_its_new_start(self):
        with tempfile.TemporaryDirectory() as folder:
            log=Path(folder)/'runtime.log'
            scanner=NativeLogMarker()
            log.write_bytes(b'x'*1000)
            self.assertFalse(scanner.scan(log))
            log.write_bytes(scanner.marker)
            self.assertTrue(scanner.scan(log))
