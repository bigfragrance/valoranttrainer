"""No process launches: verify the no-login and version preflight boundaries."""
from __future__ import annotations

import hashlib
import importlib.util
import json
import os
from pathlib import Path
import tempfile
import sys
import unittest
from unittest.mock import Mock, patch

MODULE_PATH = Path(__file__).resolve().parents[1] / "tools/run_native_probe.py"
sys.path.insert(0, str(MODULE_PATH.parent))
SPEC = importlib.util.spec_from_file_location("native_probe_under_test", MODULE_PATH)
probe = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(probe)


@unittest.skipUnless(os.name == "nt", "Windows preflight")
class NativeProbeBoundaryTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.image = self.root / "fixture.exe"
        self.image.write_bytes(b"offline-test-image; never executable")
        self.config = {"account_login": "forbidden", "game_executable": str(self.image),
                       "expected_snapshot_sha256": hashlib.sha256(self.image.read_bytes()).hexdigest(),
                       "expected_snapshot_bytes": self.image.stat().st_size}
        self.flags = {"settings_file_present": True, "saved_login_present": False,
                      "saved_authorization_present": False}
        for name, value in (("SUPPORTED_GAME", self.image), ("game_processes", []),
                            ("matching_saved_credential", False), ("saved_login_flags", self.flags)):
            handle = patch.object(probe, name, value) if name == "SUPPORTED_GAME" else patch.object(probe, name, return_value=value)
            handle.start()
            self.addCleanup(handle.stop)

    def test_empty_saved_identity_allows_preflight_only(self):
        self.assertTrue(probe.preflight(self.config)["ready"])

    def test_existing_game_or_launcher_blocks_launch(self):
        with patch.object(probe, "game_processes", return_value=[{"name": "RiotClientServices.exe", "pid": 1}]):
            self.assertFalse(probe.preflight(self.config)["ready"])

    def test_separate_regional_installation_does_not_block_or_get_selected(self):
        separate = {"name": "VALORANT-Win64-Shipping.exe", "pid": 2,
                    "executable_path": str(self.root.parent / 'separate-client' / 'VALORANT-Win64-Shipping.exe')}
        with patch.object(probe, "game_processes", return_value=[separate]):
            result = probe.preflight(self.config)
        self.assertTrue(result["ready"])
        self.assertEqual(result["blocking_processes"], [])
        self.assertEqual(result["separate_installation_processes"], [separate])

    def test_target_image_still_blocks_when_foreign_client_also_runs(self):
        processes = [{"pid": 2, "executable_path": str(self.root.parent / 'foreign.exe')},
                     {"pid": 3, "executable_path": str(self.image)}]
        with patch.object(probe, "game_processes", return_value=processes):
            result = probe.preflight(self.config)
        self.assertFalse(result["ready"])
        self.assertEqual([p['pid'] for p in result['blocking_processes']], [3])

    def test_launcher_in_verified_installation_still_blocks(self):
        processes = [{"pid": 3, "executable_path": str(self.root / 'RiotClientServices.exe')}]
        with patch.object(probe, "game_processes", return_value=processes):
            self.assertFalse(probe.preflight(self.config)["ready"])

    def test_any_saved_identity_blocks_launch(self):
        for flag in ("saved_login_present", "saved_authorization_present"):
            with self.subTest(flag=flag), patch.object(probe, "saved_login_flags", return_value={**self.flags, flag: True}):
                self.assertFalse(probe.preflight(self.config)["ready"])
        with patch.object(probe, "matching_saved_credential", return_value=True):
            self.assertFalse(probe.preflight(self.config)["ready"])

    def test_changed_build_blocks_launch(self):
        self.image.write_bytes(b"different-image")
        self.assertFalse(probe.preflight(self.config)["ready"])

    def test_project_cannot_enable_account_login(self):
        with self.assertRaises(RuntimeError):
            probe.preflight({**self.config, "account_login": "allowed"})

    def test_settings_values_never_appear_in_report(self):
        settings = self.root / "Riot Games/Riot Client/Data/RiotGamesPrivateSettings.yaml"
        settings.parent.mkdir(parents=True)
        sentinel = "SYNTHETIC_SECRET_DO_NOT_PRINT"
        settings.write_text(f"riot-login:\n  persist: {sentinel}\npsl:\n  authorization:\n    riot-client: {sentinel}\n", encoding="utf-8")
        # Undo only this test's function mock to exercise the real parser.
        fresh_spec = importlib.util.spec_from_file_location("native_probe_settings_test", MODULE_PATH)
        fresh = importlib.util.module_from_spec(fresh_spec)
        fresh_spec.loader.exec_module(fresh)
        with patch.dict(os.environ, {"LOCALAPPDATA": str(self.root)}):
            result = fresh.saved_login_flags()
        self.assertTrue(result["saved_login_present"])
        self.assertTrue(result["saved_authorization_present"])
        self.assertNotIn(sentinel, json.dumps(result))

    def capture_fixture(self, edit_source):
        directory = self.root / "probe"
        directory.mkdir()
        source = directory / "engine_snapshot_probe.cpp"
        source.write_text("version A", encoding="utf-8")
        (directory / "workspace_paths.h").write_text("test header", encoding="utf-8")
        (directory / "engine_snapshot_probe.exe").write_bytes(b"fixture; not executable")
        run_dir = self.root / "run"
        run_dir.mkdir()
        (run_dir / "capture.json").write_text('{"status":"captured"}', encoding="utf-8")
        child = Mock(returncode=0)

        def finish(**kwargs):
            if edit_source:
                source.write_text("version B", encoding="utf-8")
            return b"", b""

        child.communicate.side_effect = finish
        with patch.object(probe, "PROBE_DIR", directory), patch.object(probe.subprocess, "Popen", return_value=child):
            return probe.run_capture(run_dir, "snapshot")

    def test_source_change_cannot_be_attributed_to_running_build(self):
        result = self.capture_fixture(edit_source=True)
        self.assertTrue(result["source_changed_during_capture"])
        self.assertNotEqual(result["source_sha256"], result["source_sha256_after"])
        self.assertFalse(result["probe_changed_during_capture"])

    def test_stable_capture_records_source_and_header_identity(self):
        result = self.capture_fixture(edit_source=False)
        self.assertFalse(result["source_changed_during_capture"])
        self.assertEqual(result["source_sha256"], result["source_sha256_after"])
        self.assertEqual(result["header_sha256"], result["headers_sha256"]["workspace_paths.h"])


if __name__ == "__main__":
    unittest.main()
