"""Native wire-format and fidelity boundaries, without launching the game."""
from copy import deepcopy
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from ini_bootstrap import Archive, apply_profile, decode, encode
from native_config_snapshot import CORE_CATEGORIES


def config_file(name="Engine"):
    return {"sections": [["Section", {"pairs": [["Array", "first"], ["Array", "second"], ["Text", "原值🙂"]],
                                       "array_of_struct_keys": [["Struct_0", "Member"]]}]],
            "flags": [1, 0, 1], "name": name, "source_hierarchy": [[12, "Config/UserEngine.ini"]],
            "integer_b0": 0xffffabcd, "engine_config_dir": "Engine/Config/", "source_config": None,
            "project_config_dir": "Game/Config/", "source_filename": "source.ini",
            "per_object_config_array_of_struct_keys": [["Outer", [["Inner", "Original"]]]]}


def snapshot():
    result = {"other_files": [], "categories": [{"category": name, "filename": name + ".ini", "file": config_file(name)}
                                                  for name in CORE_CATEGORIES],
              "cache_flags": [0, 1, 0], "global_ini_strings": ["Engine"] * 14}
    result["categories"][0]["file"]["source_config"] = config_file()
    return result


class NativeBootstrapCodecTests(unittest.TestCase):
    def test_ascii_wire_bytes_are_not_utf8_length_guesses(self):
        archive = Archive()
        archive.string("A")
        self.assertEqual(bytes(archive.data), struct.pack("<i", 2) + b"A\0")

    def test_wide_wire_counts_utf16_surrogate_units(self):
        archive = Archive()
        archive.string("A🙂")
        self.assertEqual(bytes(archive.data), struct.pack("<i", -4) + "A🙂\0".encode("utf-16-le"))

    def test_complete_recursive_content_and_duplicate_order_survive(self):
        original = snapshot()
        restored = decode(encode(original))
        self.assertEqual(restored, original)
        self.assertEqual(restored["categories"][0]["file"]["sections"][0][1]["pairs"][:2],
                         [["Array", "first"], ["Array", "second"]])

    def test_noncanonical_bool32_is_rejected(self):
        with self.assertRaises(ValueError):
            Archive(struct.pack("<I", 2)).boolean()

    def test_negative_count_and_trailing_bytes_are_rejected(self):
        with self.assertRaises(ValueError):
            decode(b"\xff\xff\xff\xff")
        with self.assertRaisesRegex(ValueError, "Trailing"):
            decode(encode(snapshot()) + b"unexpected")

    def test_truncated_archive_is_rejected(self):
        with self.assertRaises(ValueError):
            decode(encode(snapshot())[:-1])

    def test_profile_keeps_original_sources_and_other_categories(self):
        original = snapshot()
        prior = deepcopy(original)
        modified = apply_profile(original, "base-engine")
        self.assertEqual(original, prior)
        self.assertEqual(modified["categories"][1:], original["categories"][1:])
        self.assertEqual(modified["categories"][0]["file"]["source_config"],
                         original["categories"][0]["file"]["source_config"])
        self.assertEqual(modified["categories"][0]["file"]["sections"][0], original["categories"][0]["file"]["sections"][0])

    def test_fixed_categories_and_footer_cannot_be_dropped(self):
        for field in ("categories", "global_ini_strings"):
            value = snapshot()
            value[field].pop()
            with self.assertRaises(ValueError):
                encode(value)
