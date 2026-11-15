"""Static original kill-champion rules and cooked function/property evidence.

Reads previously exported current original package; never loads the client.
All output remains below this F: workspace. Unknown tokens fail closed.
"""
from pathlib import Path
import hashlib
import json
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "tools"))
from analyze_skin_effect_controls import describe, save
sys.path.insert(0, str(ROOT / "work/duel1-original-jett-native"))
import decode_x_blueprint
decoder = decode_x_blueprint.base
sys.path.insert(0, str(ROOT / "work/deathmatch-feedback"))
from blueprint_fields import Fields as BaseFields


class Fields(BaseFields):
    def prop(self):
        if self.z.names[struct.unpack_from("<I", self.data, self.at)[0] & 0x3fffffff] not in ("EnumProperty", "MulticastInlineDelegateProperty"):
            return super().prop()
        start = self.at
        kind, name = self.name(), self.name()
        object_flags, array_dim, element_size = self.read("<III")
        flags, rep_index = self.read("<QH")
        rep_notify, rep_condition = self.name(), self.read("<B")
        result = {"start": start, "type": kind, "name": name, "object_flags": object_flags,
                  "array_dim": array_dim, "element_size": element_size, "flags": hex(flags),
                  "is_parameter": bool(flags & 0x80), "rep_index": rep_index,
                  "rep_notify": rep_notify, "rep_condition": rep_condition}
        if kind == "EnumProperty":
            result.update(enum_index=self.read("<i"), underlying=self.prop())
        else:
            result["type_index"] = self.read("<i")
        result["end"] = self.at
        return result


class ChampionScript(decoder.Script):
    def expr(self):
        opcode = self.data[self.at]
        if opcode not in (0x63, 0x69):
            return super().expr()
        at, vm = self.at, self.vm
        self.read("<B")
        result = {"vm": vm, "at": at, "op": hex(opcode)}
        if opcode == 0x63:  # EX_CallMulticastDelegate: signature, receiver, args.
            result.update(kind="CallMulticastDelegate", signature=self.object(), delegate=self.expr(), params=self.params())
        else:  # EX_SwitchValue: cases, end, selector, (value,next,result)*,default.
            count = self.read("<H")
            if count > 128:
                raise ValueError("unbounded switch")
            result.update(kind="SwitchValue", target=self.read("<I"), selector=self.expr())
            result["cases"] = [{"match": self.expr(), "next": self.read("<I"), "value": self.expr()} for _ in range(count)]
            result["default"] = self.expr()
        result["end"], result["vm_end"] = self.at, self.vm
        return result


def declaration(package, export):
    data = package.content(export)
    parser = Fields(package, data)
    parser.at = 10 if export["name"] == "Comp_GameState_KillChampion_C" else 6
    result = {"name": export["name"], "export_start": export["start"], "export_bytes": len(data)}
    try:
        result["super_index"] = parser.read("<i")
        children = parser.read("<I")
        if children > 4096:
            raise ValueError("unbounded child count")
        result["children"] = [parser.read("<i") for _ in range(children)]
        count = parser.read("<I")
        result["declared_field_count"] = count
        result["fields"] = [parser.prop() for _ in range(count)]
        result["after_fields"] = parser.at
        result["remaining_hex"] = data[parser.at:].hex()
    except Exception as error:
        result["error"] = str(error)
        result["stopped_at"] = parser.at
    return result


def decode_function(package, export):
    result = decoder.function(package, export)
    if result.get("status") != "missing_script_boundary":
        return result
    # Net UFunctions additionally serialize uint16 RepOffset immediately after
    # FunctionFlags. Only admit a two-byte-longer footer with actual FUNC_Net.
    data = package.content(export)
    end = len(data) - 14
    if end < 26 or not struct.unpack_from("<I", data, end)[0] & 0x40:
        return result
    declared = declaration(package, export)
    at = declared.get("after_fields")
    if at is None:
        return result
    memory, stored = struct.unpack_from("<II", data, at)
    start = at + 8
    if start + stored != end or data[end - 1] != 0x53:
        return result
    parser = ChampionScript(package, data[start:end])
    nodes = []
    while parser.at < len(parser.data):
        nodes.append(parser.expr())
    return {"name": export["name"], "script_package_offset": export["start"] + start,
            "memory_bytes": memory, "stored_bytes": stored, "decoded_memory": parser.vm,
            "decoded_stored": parser.at, "complete": parser.vm == memory and parser.at == stored,
            "nodes": nodes, "function_flags": hex(struct.unpack_from("<I", data, end)[0]),
            "net_rep_offset": struct.unpack_from("<H", data, end + 4)[0], "error": None}


def native_evidence(out):
    sys.path.insert(0, str(ROOT / "work"))
    import platform_core as pe
    sys.path.insert(0, str(ROOT / "work/client-analysis/continue-world"))
    import inspect_world as native
    namespace = {}
    source = (ROOT / "work/duel1-original-integration-player/properties.py").read_text(encoding="utf-8")
    exec(source.split("catalog=")[0], namespace)
    raw, q, prop = namespace["raw"], namespace["q"], namespace["prop"]
    native.ROOT = out / "native"
    native.ROOT.mkdir(parents=True, exist_ok=True)
    methods = [native.function(address) for address in (0x6c4ad30, 0x795b180)]
    layouts = []
    for name in ("GetShooterPlayerStatesSortedByScore", "GetTeamComponent", "GetTeamRole", "AuthGetShooterGameMode"):
        for _, string, _ in native.findstr(name):
            for reference, _ in native.ptrrefs(string):
                address = reference - 0x10
                try:
                    data = raw(address, 64)
                    if struct.unpack_from("<I", data, 0x34)[0] != 0x45:
                        continue
                    count, size = struct.unpack_from("<HH", data, 0x30)
                    if not 0 < count < 20 or size > 512:
                        continue
                    table = q(address + 0x28)
                    layouts.append({"name": name, "descriptor": hex(address), "parameter_bytes": size,
                                    "fields": [prop(q(table + index * 8)) for index in range(count)]})
                except Exception:
                    pass
    save(native.ROOT / "call-layouts.json", layouts)
    return {"static_shipping_sha256": hashlib.sha256(pe.b).hexdigest(), "methods": methods,
            "layouts": str((native.ROOT / "call-layouts.json").relative_to(ROOT))}


def main():
    decoder.Script = ChampionScript
    raw = ROOT / "work/skin-effect-controls/raw/GameModes/Components/Comp_GameState_KillChampion.uasset"
    out = ROOT / "work/training-champion-state"
    package = decoder.Zen(raw)
    functions = [decode_function(package, export) for export in package.exports
                 if decoder.assets.resolve(package, int(export["class_index"], 16)) == "/Script/CoreUObject.Function"]
    declarations = [declaration(package, export) for export in package.exports
                    if export["name"] == "Comp_GameState_KillChampion_C" or decoder.assets.resolve(package, int(export["class_index"], 16)) == "/Script/CoreUObject.Function"]
    result = {"source": str(raw.relative_to(ROOT)), "source_sha256": hashlib.sha256(raw.read_bytes()).hexdigest(),
              "source_manifest": json.loads(raw.with_suffix(".uasset.source.json").read_text(encoding="utf-8")),
              "functions": functions, "declarations": declarations, "exports": package.exports}
    result["native_evidence"] = native_evidence(out)
    save(out / "component.json", result)
    lines = []
    for function in functions:
        lines.append("\nFUNCTION " + function["name"] + " complete=" + str(function.get("complete")) + " error=" + str(function.get("error")))
        for node in function.get("nodes", []):
            if node["kind"] == "CallMulticastDelegate":
                text = "Broadcast " + describe(node["delegate"]) + "(" + ", ".join(describe(x) for x in node["params"]) + ")"
            elif node["kind"] == "SwitchValue":
                text = json.dumps(node, ensure_ascii=False)
            else:
                text = describe(node)
            lines.append(str(node["vm"]) + ": " + text)
    (out / "component.txt").write_text("\n".join(lines), encoding="utf-8")
    save(ROOT / "reports/training-champion-state.json", {
        "schema_version": 1, "scope": "Static shipped original code evidence and unexecuted local component assembly; no launch/build.",
        "component_class": "/Game/GameModes/Components/Comp_GameState_KillChampion.Comp_GameState_KillChampion_C",
        "component_super": "/Script/Engine.ActorComponent", "source_sha256": result["source_sha256"],
        "source_manifest": result["source_manifest"], "native_evidence": result["native_evidence"],
        "script_evidence": "work/training-champion-state/component.json",
        "complete_functions": [{key: value for key, value in function.items() if key in ("name", "stored_bytes", "memory_bytes", "complete", "function_flags", "net_rep_offset")} for function in functions],
        "fields": next(x["fields"] for x in declarations if x["name"] == "Comp_GameState_KillChampion_C"),
        "practice_gap": "Bare native ShooterGameState has no Blueprint Comp_GameState_KillChampion property. The helper finds/creates the actual component through original Actor.GetComponentByClass/AddComponentByClass/FinishAddComponent; it does not add a fake property to the original class/CDO.",
        "original_rule": {"entry": "Assign Kill Champion(Killer)", "entry_parameter_bytes": 8,
                          "roster_source": "Original ShooterBlueprintLibrary.GetShooterPlayerStatesSortedByScore(Self, valid Killer else current Champion, Alliance3, OmitBots=true, bOmitCoaches=true)",
                          "score_source": "Every eligible original ShooterPlayerState.GetOwnerExclusivePlayerInfo().GetKills()",
                          "team_source": "Original BaseTeamComponent.GetTeamComponent(info).GetTeamRole(); original role1 attacker / role2 defender branches",
                          "tie_rule": "Unmodified GetHighestPlayer735VM: original Map_Values/Keys and MaxOfIntArray; replaces existing champion only through the authored strict-greater branch. Otherwise it returns the existing champion and UpdateChampion=false.",
                          "updates": "Original MulticastUpdateChampion36VM -> graph302 writes Champion and broadcasts OnChampionUpdated; original Team54VM -> graph354 writes AttackingChampion/DefendingChampion and broadcasts OnTeamChampionUpdated.",
                          "rep_notify": "These original fields declare RepNotify=None; original multicast events/delegates perform notification, not an invented OnRep."},
        "local_authority_additions": ["Create the missing actual original ActorComponent under the real local GameState using the original registered component lifecycle.",
                                      "Call the full original Assign entry after each local scoreboard update because the missing authority GameMode event transport does not deliver it automatically.",
                                      "Read actual original component leader fields for the already audited original skin presentation adapter."],
        "required_root_calls": ["Initialize(world) and Poll(world,true) once after original player-info/stat creation.",
                                "Poll(world,true) synchronously after EACH successful RecordKill, even when no Champion skin is selected; this preserves original earlier-leader tie history.",
                                "Poll(world) during normal ticks (internally limited to4Hz), and WriteDiagnostics to its own artifact path."],
        "runtime_assertions": ["Original UFunction ownership, parameter sizes, exact original loaded Script.Num and endpoint opcodes match this source before creation/calls.",
                               "Real component Outer and Owner are the actual GameState; original GetComponentByClass returns the same weak identity and bRegistered is set.",
                               "Native sorted roster identities, real kills and original team roles are logged, including original bot/coach eligibility.",
                               "Missing AShooterGameMode automatic subscription is diagnosed separately; no global role/NetMode or score/champion field is changed by the helper."],
        "helper_writes_score_or_champion_fields": False, "original_rules_may_update_real_champion_fields": True,
        "helper_writes_materials": False, "compiled": False, "runtime_verified": False, "pixels_verified": False})
    print(json.dumps({"source_sha256": result["source_sha256"], "functions": [{key: value for key, value in function.items()
                     if key in ("name", "complete", "memory_bytes", "decoded_memory", "error", "status")} for function in functions]}))


if __name__ == "__main__":
    main()
