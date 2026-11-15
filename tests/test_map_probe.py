"""Map identity, snapshot provenance and runtime-evidence regression checks."""
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from ini_bootstrap import apply_profile, decode
from native_maps import MAPS, map_url
from native_status import analyze_runtime
from run_map_probe import prepare
from test_ini_bootstrap import snapshot


def values(document):
    return dict(next(section["pairs"] for name, section in document["categories"][0]["file"]["sections"]
                     if name == "/Script/EngineSettings.GameMapsSettings"))


class NativeMapTests(unittest.TestCase):
    def test_all_map_profiles_change_actual_default_and_keep_other_categories(self):
        original = snapshot()
        prior = deepcopy(original)
        for number, package in MAPS.items():
            modified = apply_profile(original, "base-engine", number)
            self.assertEqual(values(modified)["GameDefaultMap"], package)
            self.assertEqual(modified["categories"][1:], prior["categories"][1:])
        self.assertEqual(original, prior)
        self.assertIn("Duel_Platform", MAPS[4])
        self.assertIn("Duel_Heady", MAPS[5])

    def test_scene_control_uses_local_map_options_and_preserves_soft_object_path(self):
        modified = apply_profile(snapshot(), "scene", 4)
        self.assertEqual(values(modified)["GameDefaultMap"], MAPS[4])
        self.assertEqual(values(modified)["LocalMapOptions"], "?game=/Script/Engine.GameMode")
        self.assertNotIn("?", values(modified)["GameDefaultMap"])

    def test_url_accepts_only_registered_local_maps_and_modes(self):
        for number, mode in ((0, "original"), (6, "scene"), (True, "scene"), (1, "remote")):
            with self.assertRaises(ValueError):
                map_url(number, mode)
        self.assertEqual(map_url(5), MAPS[5])

    def test_process_survival_does_not_prove_map_entry(self):
        report = analyze_runtime("LogStreaming: Display: FlushAsyncLoading(161,162)",
                                 {"arguments": [map_url(1, "scene")], "exit_code": 1})
        self.assertFalse(report["assessment"]["target_map_ready_observed"])
        self.assertFalse(report["assessment"]["training_ready_proven"])

    def test_preparation_validates_source_version_and_does_not_overwrite_archive(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / ".local/native-baseline/capture/core-config.json"
            source.parent.mkdir(parents=True)
            source.write_text(json.dumps(snapshot()), encoding="utf-8")
            (root / "reports").mkdir()
            (root / "project.json").write_text(json.dumps({"expected_snapshot_sha256": "image"}), encoding="utf-8")
            proof = {"target_sha256": "image", "snapshot": {
                "path": str(source), "sha256": hashlib.sha256(source.read_bytes()).hexdigest()}}
            validation = root / "reports/native-config-validation.json"
            validation.write_text(json.dumps(proof), encoding="utf-8")
            output = root / "prepared/map.bin"
            record = prepare(root, 3, "scene", output)
            self.assertTrue(record["round_trip"])
            self.assertEqual(values(decode(output.read_bytes()))["GameDefaultMap"], MAPS[3])
            with self.assertRaises(FileExistsError):
                prepare(root, 3, "scene", output)
            source.write_bytes(source.read_bytes() + b" ")
            with self.assertRaisesRegex(ValueError, "snapshot changed"):
                prepare(root, 3, "scene", root / "changed.bin")
            proof["target_sha256"] = "different-image"
            validation.write_text(json.dumps(proof), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "another client version"):
                prepare(root, 3, "scene", root / "changed.bin")


if __name__ == "__main__":
    unittest.main()
