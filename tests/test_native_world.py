"""Read-only observation bounds, world identity, player transitions and profile scope."""
from pathlib import Path
import struct
import sys
from types import SimpleNamespace
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from ini_bootstrap import apply_profile
from native_maps import MAPS, map_url
from native_world_observation import (WorldObserver, assess_world_samples,
                                      read_vector, read_world_pointer, WORLD_STATE_RVA)
from test_ini_bootstrap import snapshot


def synthetic_world_state():
    # For key=0, fixed native branch 0 XORs the selected slot with uint32(-1).
    state = bytearray(65)
    struct.pack_into("<Q", state, 0, 0x20000 ^ 0xffffffff)
    return bytes(state)


def fake_observer():
    memory = {}
    names = {}
    def put(address, data):
        for index, byte in enumerate(data):
            memory[address + index] = byte
    def pointer(address, value):
        put(address, struct.pack("<Q", value))
    def name(obj, text):
        value = len(names) + 1
        names[value] = text
        put(obj + 0x18, struct.pack("<III", value, 0, value))
    def obj(address, class_address, class_name):
        pointer(address + 0x10, class_address)
        name(class_address, class_name)
        pointer(class_address + 0x48, 0)
    put(0x10000000 + WORLD_STATE_RVA, synthetic_world_state())
    pointer(0x20000, 0x30000)
    obj(0x30000, 0x40000, "World")
    name(0x30000, "Skirmish_A")
    pointer(0x30028, 0x50000)
    name(0x50000, MAPS[1])
    pointer(0x30170, 0)
    pointer(0x30178, 0)
    pointer(0x301d8, 0x60000)
    obj(0x60000, 0x70000, "GameInstance")
    put(0x60040, struct.pack("<Qii", 0x80000, 1, 1))
    pointer(0x80000, 0x90000)
    obj(0x90000, 0xa0000, "LocalPlayer")
    pointer(0x90038, 0xb0000)
    obj(0xb0000, 0xc0000, "PlayerController")
    put(0xb04e8, struct.pack("<ddd", 0, 0, 0))
    pointer(0xb04b0, 0xd0000)
    obj(0xd0000, 0xe0000, "DefaultPawn")
    pointer(0xd0290, 0xf0000)
    obj(0xf0000, 0x100000, "SphereComponent")
    put(0xf0170, struct.pack("<ddd", -2050, 0, 300))
    observer = SimpleNamespace(base=0x10000000, _name_key=b"verified",
        read=lambda address, size: bytes(memory[address+i] for i in range(size)),
        fname=lambda raw: names[struct.unpack("<III", raw)[0]])
    return observer, put, pointer


class WorldObservationTests(unittest.TestCase):
    def test_real_object_chain_reports_world_and_pawn_without_exposing_addresses(self):
        observer, _, _ = fake_observer()
        value = WorldObserver(observer, MAPS[1]).sample()
        self.assertEqual(value["world_package"], MAPS[1])
        self.assertEqual(value["pawn_class"], "DefaultPawn")
        self.assertEqual(value["relative_location_cm"], [-2050, 0, 300])
        self.assertNotIn("address", str(value))

    def test_wrong_world_is_rejected_before_player_reads(self):
        observer, _, _ = fake_observer()
        with self.assertRaisesRegex(ValueError, "Target map"):
            WorldObserver(observer, MAPS[2]).sample()

    def test_controller_without_pawn_remains_unproven(self):
        observer, _, pointer = fake_observer()
        pointer(0xb04b0, 0)
        sample = WorldObserver(observer, MAPS[1]).sample()
        result = assess_world_samples([{"world": sample}], MAPS[1])
        self.assertTrue(result["target_world_observed"])
        self.assertFalse(result["local_pawn_observed"])
        self.assertFalse(result["training_ready_proven"])

    def test_large_player_count_is_rejected(self):
        observer, put, _ = fake_observer()
        put(0x60040, struct.pack("<Qii", 0x80000, 10000, 10000))
        with self.assertRaisesRegex(ValueError, "local-player"):
            WorldObserver(observer, MAPS[1]).sample()

    def test_racing_world_snapshot_is_rejected(self):
        reads = iter([synthetic_world_state(), struct.pack("<Q", 0x30000), bytes(65)])
        with self.assertRaisesRegex(ValueError, "changed"):
            read_world_pointer(lambda address, size: next(reads), 0x10000)

    def test_world_lock_is_observed_without_writing_or_waiting(self):
        value = bytearray(synthetic_world_state())
        value[-1] = 1
        with self.assertRaisesRegex(ValueError, "being updated"):
            read_world_pointer(lambda address, size: bytes(value), 0x10000)

    def test_nonfinite_or_out_of_bounds_location_is_rejected(self):
        for number in (float("nan"), float("inf"), 1e10):
            with self.assertRaises(ValueError):
                read_vector(lambda address, size: struct.pack("<ddd", number, 0, 0), 0)

    def test_navigation_is_confined_to_duel1_and_is_not_original_gameplay(self):
        for number in (2, 3, 4, 5):
            with self.assertRaisesRegex(ValueError, "Duel 1"):
                map_url(number, "navigation")
            with self.assertRaisesRegex(ValueError, "Duel 1"):
                apply_profile(snapshot(), "duel1-navigation", number)
        config = apply_profile(snapshot(), "duel1-navigation")
        pairs = dict(next(s["pairs"] for name, s in config["categories"][0]["file"]["sections"]
                          if name == "/Script/EngineSettings.GameMapsSettings"))
        self.assertEqual(pairs["LocalMapOptions"], "?game=/Script/Engine.GameModeBase")
        input_pairs = dict(next(s["pairs"] for name, s in config["categories"][2]["file"]["sections"]
                                if name == "/Script/Engine.InputSettings"))
        self.assertEqual(input_pairs["DefaultPlayerInputClass"], "/Script/Engine.PlayerInput")
        self.assertEqual(input_pairs["DefaultInputComponentClass"], "/Script/Engine.InputComponent")
        self.assertFalse(assess_world_samples([], MAPS[1])["target_world_observed"])


if __name__ == "__main__":
    unittest.main()
