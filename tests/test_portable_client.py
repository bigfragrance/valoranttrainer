"""Check moved-package paths and account isolation without launching a client."""
from copy import deepcopy
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from client_layout import CLIENT_IMAGE, PORTABLE_MODE, client_executable
from portable_config import ROOT_TOKEN, export_core, resolve_core
import run_native_probe as probe


class PortableClientTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name) / "转发 测试"
        self.image = self.root / CLIENT_IMAGE
        self.image.parent.mkdir(parents=True)
        self.image.write_bytes(b"synthetic image; never executed")
        self.config = {"distribution_mode": PORTABLE_MODE, "account_login": "forbidden",
                       "game_executable": CLIENT_IMAGE, "analysis_snapshot": CLIENT_IMAGE,
                       "expected_snapshot_sha256": hashlib.sha256(self.image.read_bytes()).hexdigest(),
                       "expected_snapshot_bytes": self.image.stat().st_size}

    def test_relative_image_is_bound_to_package_not_working_directory(self):
        self.assertEqual(client_executable(self.config, self.root), self.image.resolve())
        elsewhere = Path(self.temporary.name) / "relocated package"
        image = elsewhere / CLIENT_IMAGE
        image.parent.mkdir(parents=True)
        image.write_bytes(self.image.read_bytes())
        self.assertEqual(client_executable(self.config, elsewhere), image.resolve())

    def test_portable_mode_rejects_external_image_and_analysis_paths(self):
        for field, value in (("game_executable", str(self.image)),
                             ("game_executable", "client/../foreign.exe"),
                             ("analysis_snapshot", "elsewhere.exe"),
                             ("distribution_mode", "unknown-layout")):
            with self.subTest(field=field, value=value), self.assertRaises(RuntimeError):
                client_executable({**self.config, field: value}, self.root)

    @unittest.skipUnless(os.name == "nt", "Windows preflight")
    def test_portable_preflight_does_not_read_installed_accounts(self):
        with patch.object(probe, "ROOT", self.root), patch.object(probe, "game_processes", return_value=[]), \
             patch.object(probe, "saved_login_flags", side_effect=AssertionError("must not inspect accounts")), \
             patch.object(probe, "matching_saved_credential", side_effect=AssertionError("must not inspect credentials")):
            result = probe.preflight(self.config)
            self.assertTrue(result["ready"])
            self.assertFalse(result["saved_identity_checked"])
            self.assertIsNone(result["saved_login_present"])
            self.assertEqual(result["process_scope"], "bundled_client")
            self.image.write_bytes(b"changed fixture")
            self.assertFalse(probe.preflight(self.config)["ready"])

    def test_core_rebase_removes_developer_paths_and_expands_at_launch(self):
        original = {"other_files": [], "categories": [{"filename": str(self.root / "work/client-analysis/BaseEngine.ini"),
                    "file": {"source_hierarchy": [[0, "C:/Users/Fixture/AppData/Local/Unreal Engine/Engine/Config/UserEngine.ini"]],
                             "values": ['(Type=ReadPak, Filename="T:\\caches\\UnrealDDC\\VALORANT\\ddc.ddp")',
                                        '(Type=FileSystem, Path="C:/common-ddc/valorant/5.3")']}}]}
        before = deepcopy(original)
        exported = export_core(original, self.root)
        self.assertEqual(original, before)
        serialized = json.dumps(exported)
        self.assertNotRegex(serialized, r"[A-Za-z]:[\\/]")
        self.assertNotIn("Users/Fixture", serialized)
        self.assertIn(ROOT_TOKEN, serialized)
        expanded = resolve_core(exported, self.root)
        self.assertEqual(expanded["categories"][0]["filename"], (self.root.resolve() / "work/client-analysis/BaseEngine.ini").as_posix())
        self.assertNotIn(ROOT_TOKEN, json.dumps(expanded))

    def test_unhandled_core_path_fails_export(self):
        with self.assertRaises(ValueError):
            export_core({"other_files": [], "extra": "Z:/unexpected-source/file"}, self.root)
        with self.assertRaises(ValueError):
            export_core({"other_files": [{"account-map": "must not copy"}]}, self.root)


if __name__ == "__main__":
    unittest.main()
