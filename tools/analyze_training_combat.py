"""Reproduce the V2 combat bridge's offline ABI/provenance audit.

Reads the project's pinned analysis copy and source headers only. It does not
open a process, execute the target, modify installed files or contact a server.
The output deliberately separates static evidence from unperformed runtime QA.
"""
from __future__ import annotations

import argparse
import bisect
import hashlib
import json
from pathlib import Path
import re
import struct

import capstone
import pefile

EXPECTED = "147763350504d241ee20b77607d04bc85cebfedb1d8bc2f32473ca93bf8fe92f"
ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "reports/training-combat-runtime.json")
    args = parser.parse_args()
    pinned = ROOT / "work/client-analysis/Shipping-analysis-copy.exe"
    blob = pinned.read_bytes()
    digest = hashlib.sha256(blob).hexdigest()
    if digest != EXPECTED:
        raise ValueError("Pinned EXE identity mismatch; no runtime or substitute target is used")
    pe = pefile.PE(data=blob, fast_load=True)
    base = pe.OPTIONAL_HEADER.ImageBase
    image_bytes = pe.OPTIONAL_HEADER.SizeOfImage

    def raw(rva: int, size: int) -> bytes:
        for section in pe.sections:
            relative = rva - section.VirtualAddress
            if 0 <= relative and relative + size <= section.SizeOfRawData:
                offset = section.PointerToRawData + relative
                return blob[offset:offset + size]
        raise ValueError(f"Requested RVA {rva:#x}+{size:#x} is not raw-backed")

    def pointer(rva: int) -> int:
        return struct.unpack("<Q", raw(rva, 8))[0]

    def executable(rva: int) -> bool:
        return any(s.VirtualAddress <= rva < s.VirtualAddress + s.Misc_VirtualSize
                   and s.Characteristics & 0x20000000 for s in pe.sections)

    directory = pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
    functions = [x for x in struct.iter_unpack("<III", raw(directory.VirtualAddress, directory.Size))
                 if 0 <= x[0] < x[1] <= image_bytes and 0 < x[2] < image_bytes]
    starts = [x[0] for x in functions]
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)

    def function(rva: int, label: str) -> dict:
        index = bisect.bisect_right(starts, rva) - 1
        entry = functions[index] if index >= 0 else None
        lo, hi = (entry[0], entry[1]) if entry and entry[0] <= rva < entry[1] else (rva, rva + 32)
        instructions = list(md.disasm(raw(lo, hi - lo), base + lo))
        calls = []
        interesting = []
        for insn in instructions:
            text = f"{insn.mnemonic} {insn.op_str}"
            if insn.mnemonic in ("call", "jmp"):
                item = {"site_rva": hex(insn.address - base), "instruction": text}
                if re.fullmatch(r"0x[0-9a-f]+", insn.op_str):
                    item["target_rva"] = hex(int(insn.op_str, 16) - base)
                calls.append(item)
            if any(f"+ {offset}" in insn.op_str for offset in
                   ("0x230]", "0x828]", "0x850]", "0x888]", "0x898]", "0x4a8]", "0x4c8]", "0x238]")):
                interesting.append({"rva": hex(insn.address - base), "instruction": text})
        return {"label": label, "requested_rva": hex(rva), "pdata_range": [hex(lo), hex(hi)],
                "bytes": hi - lo, "first16": raw(rva, 16).hex(),
                "sha256_raw_range": hashlib.sha256(raw(lo, hi - lo)).hexdigest(),
                "calls": calls, "relevant_field_instructions": interesting}

    def table(rva: int, label: str, selected: dict[int, str]) -> dict:
        count = 0
        while count < 1024:
            value = pointer(rva + count * 8)
            if not base <= value < base + image_bytes or not executable(value - base):
                break
            count += 1
        if count == 1024:
            raise ValueError(f"No full original vtable delimiter for {label}")
        return {"label": label, "rva": hex(rva), "contiguous_executable_functions": count,
                "rtti_word": hex(pointer(rva - 8)), "terminating_word": hex(pointer(rva + count * 8)),
                "sha256_with_rtti": hashlib.sha256(raw(rva - 8, (count + 1) * 8)).hexdigest(),
                "selected_slots": [{"offset": hex(offset), "label": name,
                                    "target_rva": hex(pointer(rva + offset) - base)}
                                   for offset, name in selected.items()]}

    header_path = ROOT / "native/training_combat_runtime.h"
    source = header_path.read_text(encoding="utf8")
    native_sites = [
        (0x787F820, "Original ShooterCharacter.GetPawnViewLocation, forwarded without changing output"),
        (0x7884080, "Original firing location/direction helper; view query return site78840ca"),
        (0x7122760, "Original firing-origin helper; stack return site71227dd"),
        (0x711F0AE, "Native FireShot provenance callsite; actual containing pdata entry is recorded"),
        (0x70A5420, "Per-controller native aim sampler; accepts actual supplied PC, not LocalPlayer global"),
        (0x70AA150, "Original FAresInputStateBuffer constructor"),
        (0x18F5CE0, "Original40-byte event TArray growth"),
        (0x70A9A90, "Original input-buffer event cache maintenance"),
        (0x70A7110, "Original local submission/append to AresInputStateComponent"),
        (0x70AA200, "Original input-buffer cleanup"),
        (0x491E5F0, "Engine PC.ClientRestart reflected thunk -> PC virtualab0"),
        (0x492E5C0, "Original Engine PC.ClientRestart implementation -> Pawn virtual828"),
        (0x786D960, "ShooterCharacter.PawnClientRestart native leaf; constructs/sets up missing input only"),
        (0x787C520, "Original ShooterCharacter CreatePlayerInputComponent, real class6492f30"),
        (0x787C8D0, "Original ShooterCharacter SetupPlayerInputComponent"),
        (0x781CBD0, "Original Ares PC InitInputSystem; creates original AresPlayerInput"),
        (0x412E730, "Original component registration in PawnClientRestart"),
        (0x44E3030, "Original dynamic input-delegate binding in PawnClientRestart"),
        (0x42C8060, "Original rich-curve damage evaluator used by damage_tuning"),
        (0x78C6DE0, "Original regional-damage helper used by damage_tuning"),
        (0x6F56010, "Original point-damage response factory"),
        (0x6F9CE10, "Original point-damage response initialization"),
        (0x6F9FE30, "Original client health/death notification"),
        (0x1D4CFA0, "Original weak-object serial/pending-kill resolver"),
    ]
    evidence = [function(rva, label) for rva, label in native_sites]
    proof = [
        table(0xB4DC550, "ShooterCharacter native vtable", {
            0x540: "Actor Tick observer layer must survive",
            0x828: "PawnClientRestart", 0x850: "GetPawnViewLocation",
            0x888: "CreatePlayerInputComponent", 0x898: "SetupPlayerInputComponent"}),
        table(0xB29A8A0, "FiringEffectComponent native vtable", {0x298: "ProcessEvent"}),
        table(0xB50B540, "ShooterPlayerController native vtable", {
            0xAB0: "ClientRestart implementation", 0xED8: "SetupInputComponent", 0xFE8: "InitInputSystem"}),
    ]
    predicates = {
        "bounded32_sources": "MaxPlayers=32" in source,
        "independent_source_contexts": "std::array<Source,MaxPlayers> sources" in source,
        "full_previous_vptr_and_rtti_copy": "std::memcpy(result.copy.data(),previous-1,(count+1)*8)" in source,
        "compare_exchange_restore_previous_layer": "table.previous,table.Address()" in source,
        "source_spawn_epoch_binding_token": all(x in source for x in ("struct ShotToken", "spawnEpoch", "nativeOrigin", "binding")),
        "candidate_target_epoch_snapshot": "pending.victimEpochs[i]=player->spawnEpoch" in source,
        "live_weapon_switch_retirement_is_distinct": "RetireReason::WeaponChange" in source,
        "unbound_live_target_is_deferred": "if(!target.bound){++diagnostics.bindingDeferred;return Outcome::Deferred;}" in source,
        "body_physics_deferred": "if(!target.physicsReady)" in source and "Outcome::Deferred" in source,
        "original_native_body_trace": "original_damage::Trace(" in source,
        "original_scene_obstruction_trace": "KismetSystemLibrary.LineTraceSingle" in source,
        "per_source_damage_tuning_cache": "original_damage_tuning::diagnostics=s.tuningCached?s.tuning" in source,
        "all_rules_transaction_calls": all(f"deathmatch_rules::{x}(" in source for x in
                                          ("CanDamage", "BeginNativeDamage", "EndNativeDamage", "CancelNativeDamage")),
        "no_second_score_commit": "RecordKill(" not in source and "original_training::" not in source,
        "no_direct_aim_field_write": not re.search(r"Write[^;\n]*0x238", source),
        "no_image_write_api": "VirtualProtect(" not in source and "WriteProcessMemory(" not in source,
        "no_legacy_singleton_init_poll": "original_shots::Initialize(" not in source and "original_shots::Poll(" not in source,
        "native_pawn_input_lifecycle_when_missing": "reinterpret_cast<void(*)(void*)>(Read<void*>(pawnTable,0x828))(s.pawn)" in source,
        "native_per_source_view_sampler": "(s.input,&direction,s.controller)" in source,
    }
    report = {
        "schema_version": 1,
        "status": "implemented_static_audited_pending_root_compile_and_owned_runtime",
        "scope": "Only the new combat header, this offline analyzer and this report were authored. No build/game/process/UI/account/network/installed-client action.",
        "pinned_image": {"path": str(pinned), "bytes": len(blob), "sha256": digest, "preferred_base": hex(base)},
        "source": {"path": str(header_path), "sha256": hashlib.sha256(header_path.read_bytes()).hexdigest(),
                   "lines": len(source.splitlines())},
        "static_audit": {"all_passed": all(predicates.values()), "checks": predicates,
                         "qualification": "Source predicates are an audit index, not a C++ compiler or runtime proof."},
        "native_vtable_proof": proof,
        "native_functions": evidence,
        "public_api": {
            "Initialize(world)": "After original player/services initialization. Refuses legacy single-shot observer initialization.",
            "Bind(index,pawn,weapon,spawnEpoch)": "After original possession/equipment and Root ActorTick observer installation. Creates only missing original PC/Pawn input lifecycle, verifies real components/weak identities, binds damage body, caches this gun tuning, installs per-instance origin observer.",
            "BeforeRetire(index,RetireReason::WeaponChange)": "Default for same live Pawn gun switches/scroll. Releases native fire/ADS/reload, restores every prior instance vptr by CAS, leaves its damage target intact, never marks live Pawn as a corpse.",
            "BeforeRetire(index,RetireReason::PawnRetire)": "Use before retiring a dead Pawn. NativeDeath is called automatically after confirmed original death; duplicate before-respawn calls are idempotent.",
            "WeaponInput(index,fire,reload)": "Bot hook: called after actual PC.SetControlRotation. Samples that PC with native70a5420, sends native tag5 through original buffer transport, sends original ForceInputState0/2 edges, drains original+178. Original128Hz simulation owns firing/cadence/ammo/recoil/animation.",
            "WeaponInput(index,fire,reload,ads)": "Optional local physical trigger route including original ADS input1. UI/dead/ended callers send false releases.",
            "SubmitViewInput(index)": "Optional explicit per-source view transport; does not synthesize direction or write inputState238.",
            "Ready(index)": "Actual bound/alive19-body physics and original input-history readiness.",
            "Poll(monotonicSeconds)": "Poll original effect arrays, pair native origins/directions, enqueue full source/epoch/weapon-binding tokens, trace real body physics and scene, switch source-specific original damage tuning, perform one Begin/ApplyPoint/End transaction.",
            "Visible(source,target)": "AI scene visibility from original Pawn view location to original target root. This independent perception query never supplies a damage Shot.",
            "Pop(ObservedRay&)": "Separate observational history queue including source/spawnEpoch/binding/native-origin/pellet token, actual native Shot origin/direction and original aim/control snapshot. Popping does not consume the combat damage queue.",
            "WriteDiagnostics(path)": "Exports exact original origin/direction/transport counts, per-source aim readback, epoch/token and damage/death counters."
        },
        "integration_order": [
            "Root initializes original players/services/game-state and original inventory slots.",
            "Initialize the new combat bridge; V2 skips original_shots::Initialize/Poll and original_training::Initialize/Poll.",
            "Create/possess real Pawn, prepare its native content/armor/camera policies, equip original gun, install Root ActorTick observer; then Bind with actual spawnEpoch.",
            "Initialize/bind deathmatch_rules around the real roster. Its afterSpawn may call Bind while its phase is Rebuilding; processing waits for committed alive/ready state.",
            "Bot native PC rotation -> combat WeaponInput -> original input-state128Hz processing -> original FireShot -> combat Poll -> deathmatch_rules Poll/respawn. Root chooses exact frame placement and keeps only one rules Poll.",
            "Gun quiesce calls BeforeRetire default WeaponChange before any owner/slot replacement. Respawn calls PawnRetire before object retirement; Bind fresh original Pawn after Root reattaches its Tick observer.",
        ],
        "mechanisms": {
            "origin": "Native Pawn.GetPawnViewLocation output on exact caller78840ca and real backtrace71227dd/711f0ae; source firing typed-owner and native ShotIndex4a8; no camera origin.",
            "direction": "Original FiringEffect native48-byte result vector offset0. Initial cosmetic StartLocation+24 is ignored regardless of apparent finite values.",
            "pellets": "Every original payload direction retains its original pellet index. Deduplication uses globally unique native origin sequence plus pellet, embedded in source/spawnEpoch/binding token; ApplyPoint receives a separate globally monotonic nonzero damage token.",
            "occlusion": "Original19-body component trace for all alive epoch-matching candidates, nearest body first, then original scene blocking trace. Unready source/target bodies explicitly defer; no proxy capsule damage.",
            "tuning": "Save singleton Diagnostics+thread, select exact source cache or initialize its actual firing550 projectile and original SCS curves, native Calculate, save source cache, restore prior singleton. A changed projectile class reinitializes only that source.",
            "rules": "CanDamage -> BeginNativeDamage -> original_damage.ApplyPoint -> EndNativeDamage. Rules own all scoring/streak/respawn/match-end updates, including local player deaths.",
            "lifecycle": "Original PC.InitInputSystem if missing; only missing Pawn input uses actual native PawnClientRestart786d960, whose original virtuals construct/setup/register/delegate-bind the real component. No UPlayer transfer, role, NetMode, matrix or health writes.",
        },
        "runtime_validation_required": [
            "Root compiles the combined new headers with project warnings policy; this agent did not invoke a compiler.",
            "Two or more native firing sources simultaneously: independent actual origins/directions, original ammo consumption/reload/recoil and source attribution.",
            "NPC yaw/pitch changes produce native InputState238 vectors and native FireShot directions matching the original sampler, without a local-PC dependency.",
            "Player can take bot damage/die through original native health/death response; all players/body physics/armor become ready before trigger presses.",
            "Nearest body and original map cover stop other-player hits. Deferred physics/weapon-change bindings never let a shot pass through the omitted participant.",
            "Repeated burst ShotIndex resets, automatic fire and shotgun pellets do not duplicate damage tokens.",
            "Same Pawn weapon switch, scroll to owned weapons, native death and respawn restore the exact preceding Root ActorTick table and never write through expired weak identities.",
            "Rules enforce FreeForAll40 kills/540s and own the only kill/streak commit; long-session stability remains independently unverified.",
        ],
        "limitations": [
            "20 ballistic gun classes have a generic native shot path; per-gun runtime coverage is pending. The known original melee class is accepted as a non-ballistic input/damage-target context; native melee hit/damage observation is not implemented or claimed.",
            "This adapter supports direct scene-unobstructed bullet hits. It does not claim native wall penetration, ricochet, explosive or ability damage reconstruction.",
            "An input/history/body that is still unready defers actual processing. Shots older than the configured maximum age expire explicitly; geometry is never fabricated.",
            "Native tag5 standalone view transport reuses the previously validated local path; complete native context/type7 batching remains a separate investigation. No guessed clock-domain timestamp is introduced.",
            "A foreign observer layer above this module must retire first. CAS conflicts are reported and backing tables are not repurposed while still installed.",
        ],
    }
    if not report["static_audit"]["all_passed"]:
        report["status"] = "static_audit_requires_review"
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf8")
    print(json.dumps({"output": str(args.output), "status": report["status"],
                      "source_sha256": report["source"]["sha256"], "static_checks": predicates}, ensure_ascii=False))
    return 0 if report["static_audit"]["all_passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
