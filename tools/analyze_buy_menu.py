"""Pinned original buy-menu evidence. Static PE reads only; no game process.

Outputs stay under F:. The original installation is never changed.
"""
from __future__ import annotations
import argparse
import bisect
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "work"))
import platform_core as p
sys.path.insert(0, str(ROOT / "work/client-analysis/continue-world"))
import inspect_world as w

EXPECTED = "147763350504d241ee20b77607d04bc85cebfedb1d8bc2f32473ca93bf8fe92f"
assert hashlib.sha256(p.b).hexdigest() == EXPECTED
CATALOG = json.loads((ROOT / "work/client-analysis/continue-world/base-bootstrap/class-catalog.json").read_text())["classes"]
# This generated native class is present in the pinned PE but omitted by the
# older broad bootstrap catalog; resolver6a7c9a0/ClassParamsb3e09b0 proves it.
if not any(c["name"] == "PlayerPurchaseablesComponent" for c in CATALOG):
    CATALOG.append({"name": "PlayerPurchaseablesComponent", "getter_rva": "0x6a78fe0", "catalog_recovery": "original ClassParams0xb3e09b0 and native getter name"})
BY_GETTER = {int(c["getter_rva"], 16): c for c in CATALOG}
CLASSES = ["ShopScreenCoordinator", "ShopScreen", "ShopContentWidget", "ShopPurchasableModel", "ShopPlayerModel", "ShopGameInfoModel", "PurchasableWeaponData", "AresPurchasable", "AresPurchasableEquippable", "MoneyManagementComponent", "PlayerPurchaseablesComponent", "PurchasableFilterComponent", "ShopItemCategoryContainer", "OwnerExclusivePlayerInfo", "AresStore", "ShooterGameState"]
NAMES = ["ToggleShop", "SetShopVisibility", "OnShopOpenStateChanged", "GetCanOpenShop", "CanOpenShop", "ShowShop", "GetShopScreenCoordinator", "GetShopContentWidget", "GetDefaultShopScreen", "BuildPurchasablesModel", "TryPurchase", "Purchase", "ServerPurchase", "AuthTryPurchase", "AuthSetIgnoreShoppingRestrictionsEnabled", "SetShoppingEnabled", "GetPurchasable", "GetPurchasables", "SetPurchasables", "GetPurchasableEquippableClass", "CanAffordPurchasable", "CanHavePurchasable", "AuthGrantPurchasable", "AuthForceGivePurchasable", "SetIgnoreShoppingRestrictions", "SetShopScreen", "SetShopScreenCoordinator", "GetShopScreenClassForCurrentGameMode", "GetAllValidPurchasables", "OnPurchase", "GetMoney", "SetMoney", "AuthSetMoney", "AuthAddMoney", "GetPurchasableForWeaponItemClass"]
NAMES += ["OnRep_Money", "SetCoordinatedShooterUIModel", "OnOpenedShopScreen", "OnClosedShopScreen", "OnRep_GameInfo", "SetIgnoreShoppingRestrictions", "Conv_ClassToSoftClassReference"]
NAMES += ["HandleLocalPlayerStateChanged"]

def raw(rva: int, n: int) -> bytes:
    off = p.pe.get_offset_from_rva(rva)
    return p.b[off:off+n]

def q(rva: int) -> int:
    return struct.unpack("<Q", raw(rva, 8))[0] - p.BASE

def prop(rva: int) -> dict:
    b = raw(rva, 64)
    if struct.unpack_from("<I", b, 0x1c)[0] != 0x45:
        raise ValueError("not a generated property")
    kind = struct.unpack_from("<I", b, 0x18)[0] & 0x3f
    item = {"name": p.string(q(rva)), "descriptor_rva": hex(rva), "kind": kind,
            "offset": hex(struct.unpack_from("<I", b, 0x32)[0]), "flags": hex(struct.unpack_from("<Q", b, 0x10)[0]), "sha256": hashlib.sha256(b).hexdigest()}
    if kind == 12:
        setter = q(rva+0x38)
        ins = next(p.md.disasm(raw(setter, 16), p.BASE+setter))
        item.update(offset=hex(ins.operands[0].mem.disp), setter_rva=hex(setter), setter_instruction=ins.mnemonic+" "+ins.op_str)
    return item

def owner_class(resolver: int) -> dict | None:
    for ins in p.md.disasm(raw(resolver, 64), p.BASE+resolver):
        if ins.mnemonic == "lea" and ins.op_str.startswith("rdx, [rip"):
            params = ins.address+ins.size+ins.operands[1].mem.disp-p.BASE
            return BY_GETTER.get(q(params))
    return None

def class_properties(name: str) -> dict:
    cls = next(c for c in CATALOG if c["name"] == name)
    candidates = []
    for rva, _ in w.ptrrefs(int(cls["getter_rva"], 16)):
        try:
            table = q(rva+0x28)
            count = (struct.unpack("<I", raw(rva+0x38, 4))[0] >> 15) & 0x7ff
            if count > 1024:
                continue
            props = [prop(q(table+n*8)) for n in range(count)]
            if props:
                candidates.append({"class_params_rva": hex(rva), "properties": props})
        except Exception:
            continue
    return {"class": cls, "candidates": candidates}

def function_body(rva: int) -> dict:
    f = p.fn(rva)
    if not f:
        return {"entry_rva": hex(rva), "unwind_range": None, "instructions": p.disasm(rva, rva+32)}
    idx = bisect.bisect_right(p.starts, f[0])-1
    entry = p.entries[idx]
    def parent(e):
        b = raw(e[2], 4)
        if (b[0] >> 3) & 4:
            return struct.unpack("<III", raw(e[2]+4+((b[2]+1)&~1)*2, 12))
        return None
    while parent(entry):
        entry = parent(entry)
    idx = bisect.bisect_left(p.starts, entry[0])
    ranges = [entry[:2]]
    for e in p.entries[idx+1:idx+256]:
        a = parent(e)
        if not a:
            break
        while parent(a):
            a = parent(a)
        if a[0] != entry[0]:
            break
        ranges.append(e[:2])
    instructions = [ins for lo, hi in ranges for ins in p.disasm(lo, hi)]
    return {"entry_rva": hex(rva), "body_ranges": [[hex(a), hex(b)] for a,b in ranges], "instructions": instructions}

def method(name: str) -> dict:
    descriptors, execs = [], []
    for _, srva, _ in w.findstr(name):
        for rva, off in w.ptrrefs(srva):
            d = off-0x10
            b = p.b[d:d+64]
            if len(b) == 64 and struct.unpack_from("<I", b, 0x34)[0] == 0x45:
                outer = struct.unpack_from("<Q", b)[0]-p.BASE
                count, size = struct.unpack_from("<HH", b, 0x30)
                if 0 < outer < 0x9e00000 and count <= 32 and size <= 1024:
                    table = struct.unpack_from("<Q", b, 0x28)[0]-p.BASE
                    try:
                        params = [prop(q(table+n*8)) for n in range(count)]
                        owner = owner_class(outer)
                        descriptors.append({"descriptor_rva": hex(p.pe.get_rva_from_offset(d)), "owner": owner["name"] if owner else None, "outer_resolver_rva": hex(outer), "parameter_size": size, "parameters": params})
                    except Exception:
                        pass
            try:
                code = struct.unpack_from("<Q", p.b, off+8)[0]-p.BASE
                fn = p.fn(code) if 0 < code < 0x9e00000 else None
                if fn and fn[0] == code:
                    execs.append({"native_table_reference_rva": hex(rva), "body": function_body(code)})
            except Exception:
                pass
    return {"name": name, "descriptors": descriptors, "execs": execs}

def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--function", action="append", default=[], type=lambda x:int(x,0))
    ap.add_argument("--report", type=Path, default=ROOT/"reports/original-buy-menu.json")
    ap.add_argument("--refresh-metadata", action="store_true")
    ap.add_argument("--asset", action="append", default=[])
    args = ap.parse_args()
    report = args.report.resolve()
    if report.drive.upper() != "F:":
        raise ValueError("new analysis output must stay on F:")
    data = json.loads(report.read_text(encoding="utf8")) if report.exists() else {}
    data.update(schema_version=1, image_sha256=EXPECTED, scope="Static original in-game shop evidence. No launch/build, online Init, account interaction or installation mutation.")
    if args.refresh_metadata or "classes" not in data:
        data["classes"] = [class_properties(n) for n in CLASSES if any(c["name"] == n for c in CATALOG)]
        data["methods"] = [method(n) for n in NAMES]
    bodies = data.setdefault("native_bodies", {})
    for rva in args.function:
        bodies[hex(rva)] = function_body(rva)
    if args.asset:
        sys.path.insert(0, str(ROOT / "work/extract"))
        from iostore import IoStore
        from zen import names_at
        inventory = json.loads((ROOT / "work/extract/inventory.json").read_text(encoding="utf8"))
        paths = {f["path"]:(c["container"], f) for c in inventory for f in c["files"]}
        stores = {}
        assets = data.setdefault("original_assets", {})
        for package in args.asset:
            path = "../../../ShooterGame/Content/"+package.removeprefix("/Game/")+".uasset"
            container, item = paths[path]
            store = stores.setdefault(container, IoStore(Path("F:/Riot Games/VALORANT/live/ShooterGame/Content/Paks")/container))
            b = store.read(path)
            head = struct.unpack_from("<13I", b)
            names, _ = names_at(b, 52)
            try:
                imported, _ = names_at(b, head[12])
            except (AssertionError, struct.error):
                imported = []
            exports = []
            for at in range(head[8], head[9], 72):
                offset, size, ni, nn, outer, cls, sup, template, public, flags = struct.unpack_from("<QQIIQQQQQI", b, at)
                exports.append({"name":names[ni&0x3fffffff], "size":size, "class_index":hex(cls), "super_index":hex(sup)})
            assets[package] = {"source_container":container, "sha256":hashlib.sha256(b).hexdigest(), "size":len(b), "imported_packages":imported, "names":names, "exports":exports}
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(json.dumps(data, ensure_ascii=False, indent=2)+"\n", encoding="utf8")
    print(json.dumps({"report": str(report), "classes": len(data["classes"]), "methods": len(data["methods"]), "native_bodies": len(bodies)}))

if __name__ == "__main__":
    main()
