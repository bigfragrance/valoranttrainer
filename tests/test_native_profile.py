"""Verify reversible local configuration overlays without launching any process."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("native_profile_tested", Path(__file__).resolve().parents[1] / "tools/native_profile.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class ProfileRestorationTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix="trainer-profile-test-")
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.target = self.root / "Config/UserEngine.ini"
        self.run = self.root / "run"
        self.run.mkdir()

    def test_new_profile_is_removed_on_exit(self):
        with module.temporary_profile(self.target, "[Probe]\nValue=1", self.run) as state:
            self.assertTrue(self.target.is_file())
        self.assertFalse(self.target.exists())
        self.assertTrue(state["restored"])

    def test_existing_unicode_file_is_restored_byte_for_byte(self):
        self.target.parent.mkdir()
        original = "[Existing]\nValue=原值\n".encode("utf-16")
        self.target.write_bytes(original)
        with module.temporary_profile(self.target, "[Probe]\nValue=1", self.run):
            self.assertIn("原值", self.target.read_text(encoding="utf-8"))
        self.assertEqual(self.target.read_bytes(), original)

    def test_failure_inside_session_still_restores(self):
        with self.assertRaisesRegex(RuntimeError, "synthetic"):
            with module.temporary_profile(self.target, "[Probe]", self.run):
                raise RuntimeError("synthetic child failure")
        self.assertFalse(self.target.exists())

    def test_external_edit_is_preserved(self):
        with self.assertRaisesRegex(RuntimeError, "changed externally"):
            with module.temporary_profile(self.target, "[Probe]", self.run) as state:
                self.target.write_bytes(b"external edit")
        self.assertEqual(self.target.read_bytes(), b"external edit")
        self.assertTrue(state["restoration_conflict"])

    def test_other_filename_is_rejected(self):
        with self.assertRaises(ValueError):
            with module.temporary_profile(self.root / "DefaultEngine.ini", "[Probe]", self.run):
                self.fail("Unexpected profile scope")
