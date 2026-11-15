"""Bounded read-only world/player observations for the pinned local client.

The wrapper owns the process handle and verifies its image before launch. This
module performs no target calls or writes and does not attach to other processes.
Layout provenance is recorded in reports/duel1-world-layout.json.
"""
from __future__ import annotations

import math
import struct

MASK64 = (1 << 64) - 1
MASK32 = (1 << 32) - 1
WORLD_STATE_RVA = 0xd7f7300


def rotate(value, count, left=True):
    count &= 63
    if not left:
        count = (-count) & 63
    return ((value << count) | (value >> ((-count) & 63))) & MASK64


def swap_groups(value):
    for shift, mask in ((1, 0xaaaaaaaaaaaaaaaa), (2, 0xcccccccccccccccc),
                        (4, 0xf0f0f0f0f0f0f0f0), (8, 0xff00ff00ff00ff00)):
        value = (((value << shift) ^ (value >> shift)) & mask) ^ (value >> shift)
    return rotate(value, 32)


def world_slot_address(state):
    """Read-side arithmetic from fixed-image function 0x4c2a420, excluding writes."""
    if len(state) != 0x41 or state[0x40]:
        raise ValueError("World state unavailable or being updated")
    key = struct.unpack_from("<I", state, 0x38)[0]
    mixed = ((((key << 25) & MASK32) ^ ((key ^ (key >> 15)) >> 12) ^ key)
             * 0x2545f4914f6cdd1d) & MASK64
    index, high = mixed % 7, mixed >> 32
    value = struct.unpack_from("<Q", state, index * 8)[0]
    one = (high + index) & MASK32
    two = (high + index * 2) & MASK32
    if index == 0:
        value ^= (high - 1) & MASK32
    elif index == 1:
        value = rotate((value + two) & MASK64, one % 63 + 1, left=False)
    elif index == 2:
        value = (rotate(value, two % 63 + 1) - one) & MASK64
    elif index == 3:
        value = swap_groups(value ^ (~two & MASK64))
    elif index == 4:
        value = swap_groups(~value & MASK64)
    elif index == 5:
        value = ((value + two) & MASK64) ^ (~one & MASK64)
    else:
        value = ~swap_groups(value) & MASK64
    return value ^ key


def checked_pointer(value):
    if not 0x10000 <= value < 0x0000800000000000 or value & 7:
        raise ValueError("Unavailable aligned user-mode pointer")
    return value


def read_world_pointer(reader, state_address):
    before = reader(state_address, 0x41)
    slot = checked_pointer(world_slot_address(before))
    world = struct.unpack("<Q", reader(slot, 8))[0]
    if reader(state_address, 0x41) != before:
        raise ValueError("World state changed during observation")
    return checked_pointer(world)


def read_vector(reader, address):
    values = struct.unpack("<ddd", reader(address, 24))
    if not all(math.isfinite(value) and abs(value) < 1e9 for value in values):
        raise ValueError("Invalid bounded spatial values")
    return [round(value, 3) for value in values]


class WorldObserver:
    def __init__(self, observer, target_package):
        self.observer = observer
        self.target_package = target_package

    def ptr(self, address, optional=False):
        value = struct.unpack("<Q", self.observer.read(address, 8))[0]
        return None if optional and not value else checked_pointer(value)

    def name(self, obj):
        return self.observer.fname(self.observer.read(obj + 0x18, 12))

    def class_name(self, obj):
        return self.name(self.ptr(obj + 0x10))

    def inherits(self, obj, expected):
        current = self.ptr(obj + 0x10)
        seen = set()
        for _ in range(24):
            if current in seen:
                raise ValueError("Cyclic runtime class hierarchy")
            seen.add(current)
            if self.name(current) == expected:
                return True
            current = self.ptr(current + 0x48, optional=True)
            if current is None:
                return False
        raise ValueError("Runtime class hierarchy exceeds observation bound")

    def sample(self):
        o = self.observer
        if o.base is None:
            o.find_main_image()
        if o._name_key is None:
            cache = self.ptr(o.base + 0xd97ad48)
            o.configure_name_decoder(cache)
        world = read_world_pointer(o.read, o.base + WORLD_STATE_RVA)
        if self.class_name(world) != "World":
            raise ValueError("Observed object is not a World")
        package = self.name(self.ptr(world + 0x28))
        if package != self.target_package:
            raise ValueError("Target map is not the current world")
        result = {"world_package": package, "world_class": "World", "world_name": self.name(world)}
        for key, offset in (("game_mode_class", 0x170), ("game_state_class", 0x178)):
            obj = self.ptr(world + offset, optional=True)
            result[key] = self.class_name(obj) if obj else None
        instance = self.ptr(world + 0x1d8)
        result["game_instance_class"] = self.class_name(instance)
        header = o.read(instance + 0x40, 16)
        players, count, capacity = struct.unpack("<Qii", header)
        if not 0 <= count <= capacity <= 4:
            raise ValueError("Invalid bounded local-player list")
        result["local_player_count"] = count
        if count:
            player = self.ptr(checked_pointer(players))
            result["local_player_class"] = self.class_name(player)
            controller = self.ptr(player + 0x38, optional=True)
            result["controller_class"] = self.class_name(controller) if controller else None
            if controller:
                result["control_rotation"] = read_vector(o.read, controller + 0x4e8)
                pawn = self.ptr(controller + 0x4b0, optional=True)
                result["pawn_class"] = self.class_name(pawn) if pawn else None
                if pawn:
                    result["pawn_is_shooter_character"] = self.inherits(pawn, "ShooterCharacter")
                    if result["pawn_is_shooter_character"]:
                        result["shooter_character_initialized"] = bool(o.read(pawn + 0xa90, 1)[0])
                        inventory = self.ptr(pawn + 0xc08, optional=True)
                        result["inventory_class"] = self.class_name(inventory) if inventory else None
                        if inventory:
                            if not self.inherits(inventory, "AresInventory"):
                                raise ValueError("Original character inventory has an unexpected type")
                            current = self.ptr(inventory + 0x280, optional=True)
                            desired = self.ptr(inventory + 0x268, optional=True)
                            result["current_equippable_class"] = self.class_name(current) if current else None
                            result["desired_equippable_class"] = self.class_name(desired) if desired else None
                    component = self.ptr(pawn + 0x290, optional=True)
                    result["root_component_class"] = self.class_name(component) if component else None
                    if component:
                        result["relative_location_cm"] = read_vector(o.read, component + 0x170)
                    if self.ptr(controller + 0x4b0, optional=True) != pawn:
                        raise ValueError("Controlled Pawn changed during observation")
                if self.ptr(player + 0x38, optional=True) != controller:
                    raise ValueError("Local controller changed during observation")
            if o.read(instance + 0x40, 16) != header:
                raise ValueError("Local player list changed during observation")
        if read_world_pointer(o.read, o.base + WORLD_STATE_RVA) != world:
            raise ValueError("Current world changed during observation")
        self.context = {"image": o.base, "world": world,
                        "controller": controller if count else None,
                        "pawn": pawn if count and controller else None}
        return result


def assess_world_samples(samples, target_package):
    matching = [row["world"] for row in samples if row.get("world", {}).get("world_package") == target_package]
    last = matching[-1] if matching else {}
    return {"evidence_kind": "read_only_owned_process_object_chain",
            "target_world_observed": bool(matching),
            "local_controller_observed": bool(last.get("controller_class")),
            "local_pawn_observed": bool(last.get("pawn_class")),
            "pawn_classes": sorted({row["pawn_class"] for row in matching if row.get("pawn_class")}),
            "original_character_movement_proven": False, "training_ready_proven": False}
