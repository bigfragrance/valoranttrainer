#pragma once
#include "original_content.h"
#include "original_weapon.h"
#include <cstddef>
#include <cwchar>

// Original fixed-client weapon catalog and local missing inventory authority.
// Actor/SCS, owner callback, equipped-event graph, TryEquip, input, firing,
// reload, recoil and animation are the original implementations. No shot model.
// reports/all-weapons.json; reports/duel1-original-attachment-factory-audit.json.
namespace all_weapons {
using Guid=original_content::Guid;
enum class Gate : uint8_t { Pending, Ready, Failed };
enum class Phase : uint8_t { Idle, Quiesce, Spawn, Prepare, Grant, Equip, WaitCurrent, Bind, Failed };
struct Definition {
    Guid id{};
    const char *guid=nullptr,*label=nullptr,*category=nullptr,*icon=nullptr,*source_path=nullptr;
    const wchar_t *dataAssetClassPath=nullptr,*weaponClassPath=nullptr,*defaultSkinClassPath=nullptr;
    uint8_t categoryValue=0;
    void *runtimeClass=nullptr,*dataAsset=nullptr,*defaults=nullptr;
    int equipmentSlot=-1;
    bool identityVerified=false;
    const char* validationStage="not_loaded";
};
// BEGIN GENERATED ORIGINAL CATALOG
inline Definition definitions[] = {
    { {{0x63e6c2b6u, 0x4a8e869cu, 0x3d4ce383u, 0x55226584u}}, "63e6c2b6-4a8e-869c-3d4c-e38355226584", "Odin", "heavy_machine_gun", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_HMG", "/Game/Equippables/Guns/HvyMachineGuns/HMG/HMGPrimaryAsset", L"/Game/Equippables/Guns/HvyMachineGuns/HMG/HMGPrimaryAsset.HMGPrimaryAsset_C", L"/Game/Equippables/Guns/HvyMachineGuns/HMG/HeavyMachineGun.HeavyMachineGun_C", L"/Game/Equippables/Guns/HvyMachineGuns/HMG/Standard/HeavyMachineGun_Standard_PrimaryAsset.HeavyMachineGun_Standard_PrimaryAsset_C", 1 },
    { {{0x55d8a0f4u, 0x4274ca67u, 0xfe2c06abu, 0x45efdf58u}}, "55d8a0f4-4274-ca67-fe2c-06ab45efdf58", "Ares", "heavy_machine_gun", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_LMG", "/Game/Equippables/Guns/HvyMachineGuns/LMG/LMGPrimaryAsset", L"/Game/Equippables/Guns/HvyMachineGuns/LMG/LMGPrimaryAsset.LMGPrimaryAsset_C", L"/Game/Equippables/Guns/HvyMachineGuns/LMG/LightMachineGun.LightMachineGun_C", L"/Game/Equippables/Guns/HvyMachineGuns/LMG/Standard/LightMachineGun_Standard_PrimaryAsset.LightMachineGun_Standard_PrimaryAsset_C", 1 },
    { {{0x9c82e19du, 0x45750200u, 0x1a813eacu, 0xf00cf872u}}, "9c82e19d-4575-0200-1a81-3eacf00cf872", "Vandal", "rifle", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_Rifles_Volcano", "/Game/Equippables/Guns/Rifles/AK/AKPrimaryAsset", L"/Game/Equippables/Guns/Rifles/AK/AKPrimaryAsset.AKPrimaryAsset_C", L"/Game/Equippables/Guns/Rifles/AK/AssaultRifle_AK.AssaultRifle_AK_C", L"/Game/Equippables/Guns/Rifles/AK/Standard/AssaultRifle_AK_Standard_PrimaryAsset.AssaultRifle_AK_Standard_PrimaryAsset_C", 2 },
    { {{0x8db0a1bfu, 0x4a50832au, 0x4566faaau, 0xa6d250cau}}, "8db0a1bf-4a50-832a-4566-faaaa6d250ca", "Warden", "rifle", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_Rifle_BattleRifle", "/Game/Equippables/Guns/Rifles/BattleRifle/BattleRifle_PrimaryAsset", L"/Game/Equippables/Guns/Rifles/BattleRifle/BattleRifle_PrimaryAsset.BattleRifle_PrimaryAsset_C", L"/Game/Equippables/Guns/Rifles/BattleRifle/BattleRifle.BattleRifle_C", L"/Game/Equippables/Guns/Rifles/BattleRifle/Standard/BattleRifle_Standard_PrimaryAsset.BattleRifle_Standard_PrimaryAsset_C", 2 },
    { {{0xae3de142u, 0x4d852547u, 0xdd264e90u, 0xbed35cf7u}}, "ae3de142-4d85-2547-dd26-4e90bed35cf7", "Bulldog", "rifle", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_Rifles_Burst", "/Game/Equippables/Guns/Rifles/Burst/BurstPrimaryAsset", L"/Game/Equippables/Guns/Rifles/Burst/BurstPrimaryAsset.BurstPrimaryAsset_C", L"/Game/Equippables/Guns/Rifles/Burst/AssaultRifle_Burst.AssaultRifle_Burst_C", L"/Game/Equippables/Guns/Rifles/Burst/Standard/AssaultRifle_Burst_Standard_PrimaryAsset.AssaultRifle_Burst_Standard_PrimaryAsset_C", 2 },
    { {{0xee8e8d15u, 0x496b07acu, 0xe5f68faeu, 0x5d4c7b1au}}, "ee8e8d15-496b-07ac-e5f6-8fae5d4c7b1a", "Phantom", "rifle", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_Rifles_Ghost", "/Game/Equippables/Guns/Rifles/Carbine/CarbinePrimaryAsset", L"/Game/Equippables/Guns/Rifles/Carbine/CarbinePrimaryAsset.CarbinePrimaryAsset_C", L"/Game/Equippables/Guns/Rifles/Carbine/AssaultRifle_ACR.AssaultRifle_ACR_C", L"/Game/Equippables/Guns/Rifles/Carbine/Standard/AssaultRifle_ACR_Standard_PrimaryAsset.AssaultRifle_ACR_Standard_PrimaryAsset_C", 2 },
    { {{0xec845bf4u, 0x4f79dddau, 0xa3da0db3u, 0x774b2794u}}, "ec845bf4-4f79-ddda-a3da-0db3774b2794", "Judge", "shotgun", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_Shotguns_Persuader", "/Game/Equippables/Guns/Shotguns/AutoShotgun/AutoShotgunPrimaryAsset", L"/Game/Equippables/Guns/Shotguns/AutoShotgun/AutoShotgunPrimaryAsset.AutoShotgunPrimaryAsset_C", L"/Game/Equippables/Guns/Shotguns/AutoShotgun/AutomaticShotgun.AutomaticShotgun_C", L"/Game/Equippables/Guns/Shotguns/AutoShotgun/Standard/AutomaticShotgun_Standard_PrimaryAsset.AutomaticShotgun_Standard_PrimaryAsset_C", 3 },
    { {{0x910be174u, 0x449bc412u, 0xab22d087u, 0x3436b21bu}}, "910be174-449b-c412-ab22-d0873436b21b", "Bucky", "shotgun", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_Shotguns_Pump", "/Game/Equippables/Guns/Shotguns/PumpShotgun/PumpShotgunPrimaryAsset", L"/Game/Equippables/Guns/Shotguns/PumpShotgun/PumpShotgunPrimaryAsset.PumpShotgunPrimaryAsset_C", L"/Game/Equippables/Guns/Shotguns/PumpShotgun/PumpShotgun.PumpShotgun_C", L"/Game/Equippables/Guns/Shotguns/PumpShotgun/Standard/PumpShotgun_Standard_PrimaryAsset.PumpShotgun_Standard_PrimaryAsset_C", 3 },
    { {{0x44d4e95cu, 0x41570037u, 0x81b21784u, 0x1bf2e8e3u}}, "44d4e95c-4157-0037-81b2-17841bf2e8e3", "Frenzy", "sidearm", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_Pistol_AutoPistol", "/Game/Equippables/Guns/Sidearms/AutoPistol/AutoPistolPrimaryAsset", L"/Game/Equippables/Guns/Sidearms/AutoPistol/AutoPistolPrimaryAsset.AutoPistolPrimaryAsset_C", L"/Game/Equippables/Guns/Sidearms/AutoPistol/AutomaticPistol.AutomaticPistol_C", L"/Game/Equippables/Guns/Sidearms/AutoPistol/Standard/AutomaticPistol_Standard_PrimaryAsset.AutomaticPistol_Standard_PrimaryAsset_C", 4 },
    { {{0x29a0cfabu, 0x485bf5d5u, 0x779ab59fu, 0x85e204a8u}}, "29a0cfab-485b-f5d5-779a-b59f85e204a8", "Classic", "sidearm", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_Pistol_Classic", "/Game/Equippables/Guns/Sidearms/BasePistol/BasePistolPrimaryAsset", L"/Game/Equippables/Guns/Sidearms/BasePistol/BasePistolPrimaryAsset.BasePistolPrimaryAsset_C", L"/Game/Equippables/Guns/Sidearms/BasePistol/BasePistol.BasePistol_C", L"/Game/Equippables/Guns/Sidearms/BasePistol/Standard/BasePistol_Standard_PrimaryAsset.BasePistol_Standard_PrimaryAsset_C", 4 },
    { {{0x410b2e0bu, 0x4ceb1321u, 0x17272085u, 0x8f7f3477u}}, "410b2e0b-4ceb-1321-1727-20858f7f3477", "Bandit", "sidearm", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_Pistol_Compact", "/Game/Equippables/Guns/Sidearms/Compact/Compact_PrimaryAsset", L"/Game/Equippables/Guns/Sidearms/Compact/Compact_PrimaryAsset.Compact_PrimaryAsset_C", L"/Game/Equippables/Guns/Sidearms/Compact/CompactPistol.CompactPistol_C", L"/Game/Equippables/Guns/Sidearms/Compact/Compact_Standard_PrimaryAsset.Compact_Standard_PrimaryAsset_C", 4 },
    { {{0x1baa85b4u, 0x4c701284u, 0x64bb6481u, 0xdfc3bb4eu}}, "1baa85b4-4c70-1284-64bb-6481dfc3bb4e", "Ghost", "sidearm", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_Pistol_Luger", "/Game/Equippables/Guns/Sidearms/Luger/LugerPrimaryAsset", L"/Game/Equippables/Guns/Sidearms/Luger/LugerPrimaryAsset.LugerPrimaryAsset_C", L"/Game/Equippables/Guns/Sidearms/Luger/LugerPistol.LugerPistol_C", L"/Game/Equippables/Guns/Sidearms/Luger/Standard/LugerPistol_Standard_PrimaryAsset.LugerPistol_Standard_PrimaryAsset_C", 4 },
    { {{0xe336c6b8u, 0x418d9340u, 0xd77f7a9eu, 0x4cfe0702u}}, "e336c6b8-418d-9340-d77f-7a9e4cfe0702", "Sheriff", "sidearm", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_Pistol_Sheriff", "/Game/Equippables/Guns/Sidearms/Revolver/RevolverPrimaryAsset", L"/Game/Equippables/Guns/Sidearms/Revolver/RevolverPrimaryAsset.RevolverPrimaryAsset_C", L"/Game/Equippables/Guns/Sidearms/Revolver/RevolverPistol.RevolverPistol_C", L"/Game/Equippables/Guns/Sidearms/Revolver/Standard/RevolverPistol_Standard_PrimaryAsset.RevolverPistol_Standard_PrimaryAsset_C", 4 },
    { {{0x42da8cccu, 0x40d5affcu, 0xbeec15aau, 0x47b42edau}}, "42da8ccc-40d5-affc-beec-15aa47b42eda", "Shorty", "sidearm", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_Pistol_Slim", "/Game/Equippables/Guns/Sidearms/Slim/SlimPrimaryAsset", L"/Game/Equippables/Guns/Sidearms/Slim/SlimPrimaryAsset.SlimPrimaryAsset_C", L"/Game/Equippables/Guns/Sidearms/Slim/SawedOffShotgun.SawedOffShotgun_C", L"/Game/Equippables/Guns/Sidearms/Slim/Standard/SawedOffShotgun_Standard_PrimaryAsset.SawedOffShotgun_Standard_PrimaryAsset_C", 4 },
    { {{0xa03b24d3u, 0x4319996du, 0x0f8c94bbu, 0xfba1dfc7u}}, "a03b24d3-4319-996d-0f8c-94bbfba1dfc7", "Operator", "sniper_rifle", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_Sniper_Operater", "/Game/Equippables/Guns/SniperRifles/Boltsniper/BoltSniperPrimaryAsset", L"/Game/Equippables/Guns/SniperRifles/Boltsniper/BoltSniperPrimaryAsset.BoltSniperPrimaryAsset_C", L"/Game/Equippables/Guns/SniperRifles/Boltsniper/BoltSniper.BoltSniper_C", L"/Game/Equippables/Guns/SniperRifles/Boltsniper/Standard/BoltSniper_Standard_PrimaryAsset.BoltSniper_Standard_PrimaryAsset_C", 6 },
    { {{0x4ade7faau, 0x4cf18376u, 0x95ef3988u, 0x4480959bu}}, "4ade7faa-4cf1-8376-95ef-39884480959b", "Guardian", "rifle", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_Rifles_DMR", "/Game/Equippables/Guns/SniperRifles/DMR/DMRPrimaryAsset", L"/Game/Equippables/Guns/SniperRifles/DMR/DMRPrimaryAsset.DMRPrimaryAsset_C", L"/Game/Equippables/Guns/SniperRifles/DMR/DMR.DMR_C", L"/Game/Equippables/Guns/SniperRifles/DMR/Standard/DMR_Standard_PrimaryAsset.DMR_Standard_PrimaryAsset_C", 2 },
    { {{0x5f0aaf7au, 0x42893998u, 0xd5ffeb9au, 0x5cf7ef5cu}}, "5f0aaf7a-4289-3998-d5ff-eb9a5cf7ef5c", "Outlaw", "sniper_rifle", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_Sniper_DoubleSniper", "/Game/Equippables/Guns/SniperRifles/Doublesniper/DS_Gun_PrimaryAsset", L"/Game/Equippables/Guns/SniperRifles/Doublesniper/DS_Gun_PrimaryAsset.DS_Gun_PrimaryAsset_C", L"/Game/Equippables/Guns/SniperRifles/Doublesniper/DS_Gun.DS_Gun_C", L"/Game/Equippables/Guns/SniperRifles/Doublesniper/Standard/DS_Gun_Standard_PrimaryAsset.DS_Gun_Standard_PrimaryAsset_C", 6 },
    { {{0xc4883e50u, 0x4494202cu, 0x3ec36b8au, 0x9284f00bu}}, "c4883e50-4494-202c-3ec3-6b8a9284f00b", "Marshal", "sniper_rifle", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_Sniper_Bolt", "/Game/Equippables/Guns/SniperRifles/Leversniper/LeverSniperPrimaryAsset", L"/Game/Equippables/Guns/SniperRifles/Leversniper/LeverSniperPrimaryAsset.LeverSniperPrimaryAsset_C", L"/Game/Equippables/Guns/SniperRifles/Leversniper/LeverSniperRifle.LeverSniperRifle_C", L"/Game/Equippables/Guns/SniperRifles/Leversniper/Standard/LeverSniperRifle_Standard_PrimaryAsset.LeverSniperRifle_Standard_PrimaryAsset_C", 6 },
    { {{0x462080d1u, 0x40352937u, 0x7c0927aau, 0x2a5c27a7u}}, "462080d1-4035-2937-7c09-27aa2a5c27a7", "Spectre", "submachine_gun", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_SMGs_Ninja", "/Game/Equippables/Guns/SubMachineGuns/MP5/MP5PrimaryAsset", L"/Game/Equippables/Guns/SubMachineGuns/MP5/MP5PrimaryAsset.MP5PrimaryAsset_C", L"/Game/Equippables/Guns/SubMachineGuns/MP5/SubMachineGun_MP5.SubMachineGun_MP5_C", L"/Game/Equippables/Guns/SubMachineGuns/MP5/Standard/SubMachineGun_MP5_Standard_PrimaryAsset.SubMachineGun_MP5_Standard_PrimaryAsset_C", 5 },
    { {{0xf7e1b454u, 0x4ad41063u, 0xec0a159eu, 0x56b58941u}}, "f7e1b454-4ad4-1063-ec0a-159e56b58941", "Stinger", "submachine_gun", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_SMGs_Vector", "/Game/Equippables/Guns/SubMachineGuns/Vector/VectorPrimaryAsset", L"/Game/Equippables/Guns/SubMachineGuns/Vector/VectorPrimaryAsset.VectorPrimaryAsset_C", L"/Game/Equippables/Guns/SubMachineGuns/Vector/Vector.Vector_C", L"/Game/Equippables/Guns/SubMachineGuns/Vector/Standard/Vector_Standard_PrimaryAsset.Vector_Standard_PrimaryAsset_C", 5 },
    { {{0x2f59173cu, 0x4bedb6c3u, 0x2191dea9u, 0xb58be9c7u}}, "2f59173c-4bed-b6c3-2191-dea9b58be9c7", "Melee", "melee", "/Game/UI/Screens/Shared/Icons/Weapons/icons/TX_Hud_Knife_Standard", "/Game/Equippables/Melee/Melee_PrimaryAsset", L"/Game/Equippables/Melee/Melee_PrimaryAsset.Melee_PrimaryAsset_C", L"/Game/Equippables/Melee/Ability_Melee_Base.Ability_Melee_Base_C", L"/Game/Equippables/Melee/Standard/Melee_Base_Standard_PrimaryAsset.Melee_Base_Standard_PrimaryAsset_C", 7 },
};
// END GENERATED ORIGINAL CATALOG
inline constexpr size_t Count=sizeof(definitions)/sizeof(definitions[0]);
struct Hooks {
    bool (*quiesce)(void* oldWeapon)=nullptr;
    bool (*beforeFinishSpawn)(void* newWeapon,const Definition&)=nullptr;
    Gate (*prepareWeapon)(void* newWeapon,const Definition&)=nullptr;
    bool (*weaponChanged)(void* oldWeapon,void* newWeapon,const Definition&)=nullptr;
    bool (*beforeRetire)(void* replacedWeapon)=nullptr;
};
struct SlotSnapshot {
    Guid id{};bool valid=false;
    void* actor=nullptr;original_effects::WeakIdentity identity{};
};
struct LoadoutSnapshot {
    SlotSnapshot slots[3]{};
    Guid held{};bool heldValid=false;
    uint64_t revision=0;
};
struct RestorePlan {LoadoutSnapshot loadout{};unsigned cursor=0;bool active=false;};
struct RetiredWeapon {void* actor=nullptr;original_effects::WeakIdentity identity{};};
struct Diagnostics {
    Phase phase=Phase::Idle;
    unsigned error=0,requestSequence=0,completedSequence=0,generation=0;
    size_t verifiedDefinitions=0;
    bool initialized=false,completeCatalog=false,busy=false,ready=false,firingReady=false,skinReady=false;
    bool forcedRebuild=false,slotPublished=false,equipAccepted=false;
    bool existingOnly=false,observedSwitch=false,grantOnly=false,restoring=false,bindingsReady=false;
    void *world=nullptr,*pawn=nullptr,*inventory=nullptr,*currentWeapon=nullptr;
    void *oldCurrent=nullptr,*candidate=nullptr,*replaced=nullptr,*targetSlot=nullptr,*boundWeapon=nullptr;
    const Definition *currentDefinition=nullptr,*requestedDefinition=nullptr;
    Guid currentGuid{},requestedGuid{};
    uint64_t requestStarted=0;
    const char* stage="all_weapons_not_initialized";
};
inline Diagnostics diagnostics;
inline Hooks hooks;
inline original_effects::WeakIdentity candidateIdentity{},replacedIdentity{};
inline bool changeNotified=false;
inline LoadoutSnapshot savedLoadout;
inline RestorePlan restorePlan;
inline RetiredWeapon retired[16]{};
inline original_effects::WeakIdentity pawnIdentity{};
inline bool internalRestoreRequest=false;
inline const Diagnostics& State(){return diagnostics;}
inline bool Same(const Guid& a,const Guid& b){return std::memcmp(&a,&b,sizeof(Guid))==0;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Thread(){return original_players::gameThread==GetCurrentThreadId()&&engine::image;}
inline bool Fail(unsigned code,const char* stage){diagnostics.error=code;diagnostics.phase=Phase::Failed;
    diagnostics.ready=false;diagnostics.firingReady=false;diagnostics.stage=stage;return false;}
inline bool Event(void* object,const wchar_t* path,uint16_t bytes,void* args){
    auto fn=engine::Find(path);return object&&fn&&all_weapons::Read<uint16_t>(fn,0xbe)==bytes&&engine::Event(object,path,args);
}
inline void SetHooks(const Hooks& value){hooks=value;}
inline void SetWeaponChangedCallback(bool (*callback)(void*,void*,const Definition&)){hooks.weaponChanged=callback;}
inline const Definition* Catalog(size_t& count){count=Count;return definitions;}
inline const Definition* Find(const Guid& id){for(auto& d:definitions)if(Same(d.id,id))return &d;return nullptr;}
inline const Definition* FindByClass(void* type){for(auto& d:definitions)if(d.runtimeClass==type&&d.identityVerified)return &d;return nullptr;}
inline void* CurrentWeapon(){return all_weapons::Read<void*>(diagnostics.inventory,0x280);}
inline const Definition* CurrentDefinition(){return FindByClass(all_weapons::Read<void*>(CurrentWeapon(),0x10));}
inline const LoadoutSnapshot& SavedLoadout(){return savedLoadout;}
inline bool DispatchRestore();
inline bool SaveLoadout();
inline bool RestoreLoadout(const LoadoutSnapshot& snapshot);
inline void* LoadClass(uintptr_t base,const wchar_t* path){
    return engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t,void*)>(0x1d0c6e0)
        (engine::Class(base),nullptr,path,nullptr,0,nullptr);
}
inline void* ResolveLoadedSoft(void* value){
    auto result=engine::function<void*(*)(void*)>(0x1d4cfa0)(value);
    return result?result:engine::function<void*(*)(void*)>(0x1ce5c40)(static_cast<uint8_t*>(value)+8);
}
inline bool ResolveDefinition(Definition& d){
    if(d.identityVerified)return true;
    d.validationStage="loading_original_data_and_weapon_classes";
    auto dataClass=LoadClass(0x6638d00,d.dataAssetClassPath);
    d.runtimeClass=LoadClass(0x645c660,d.weaponClassPath);
    if(!dataClass||!d.runtimeClass)return false;
    d.dataAsset=engine::DefaultObject(dataClass);d.defaults=engine::DefaultObject(d.runtimeClass);
    d.validationStage="checking_original_data_asset_type_and_uuid";
    if(!original_players::IsA(d.dataAsset,0x6638d00)||!original_players::IsA(d.defaults,0x645c660)
        ||!Same(all_weapons::Read<Guid>(d.dataAsset,0x38),d.id))return false;
    // Native71ce540 returns EquippableData (not a UObject). The typed original
    // struct getter6671c90 declares its DataAsset at+a0.
    auto indexed=engine::function<void*(*)(void*,const Guid*)>(0x71ce540)(original_content::State().index,&d.id);
    d.validationStage="checking_original_content_index_and_equippable_soft_class";
    if(all_weapons::Read<void*>(indexed,0xa0)!=d.dataAsset
        ||ResolveLoadedSoft(static_cast<uint8_t*>(d.dataAsset)+0x50)!=d.runtimeClass)return false;
    uint8_t slot=255;
    d.validationStage="checking_original_cdo_item_slot";
    if(!Event(d.defaults,L"/Script/ShooterGame.AresItem.GetItemSlot",1,&slot)||slot>2)return false;
    // Original vslot798 ->70f3150 reads AresItem+4a0. Never infer slots from
    // English labels or directory names (Guardian is in a Sniper folder).
    if(slot!=all_weapons::Read<uint8_t>(d.defaults,0x4a0))return false;
    d.equipmentSlot=slot;d.identityVerified=true;d.validationStage="native_catalog_class_guid_slot_verified";return true;
}
inline void SampleCurrent(){
    diagnostics.currentWeapon=CurrentWeapon();diagnostics.currentDefinition=CurrentDefinition();
    diagnostics.currentGuid=diagnostics.currentDefinition?diagnostics.currentDefinition->id:Guid{};
    diagnostics.firingReady=diagnostics.ready&&diagnostics.currentDefinition
        &&(diagnostics.currentDefinition->categoryValue==7||original_weapon::Component(diagnostics.currentWeapon,0x684d4c0));
}
inline bool Initialize(void* world,void* pawn){
    if(!Thread())return Fail(2301,"all_weapons_wrong_game_thread");
    if(original_weapon::thread&&original_weapon::thread!=GetCurrentThreadId())return Fail(2334,"all_weapons_existing_weapon_thread_mismatch");
    original_weapon::thread=GetCurrentThreadId();
    if(diagnostics.initialized)return diagnostics.world==world&&diagnostics.pawn==pawn;
    if(!original_content::State().initialized||!original_content::State().index)
        return Fail(2302,"all_weapons_original_content_index_not_ready");
    auto inventory=all_weapons::Read<void*>(pawn,0xc08);
    if(!world||!original_players::IsA(pawn,0x6c5c480)||!all_weapons::Read<uint8_t>(pawn,0xa90)
        ||!original_players::IsA(inventory,0x6499330)||all_weapons::Read<void*>(inventory,0x2f0)!=pawn)
        return Fail(2303,"all_weapons_original_player_inventory_not_ready");
    diagnostics.world=world;diagnostics.pawn=pawn;diagnostics.inventory=inventory;
    if(!original_effects::CaptureIdentity(pawn,pawnIdentity))return Fail(2325,"all_weapons_original_pawn_identity_unavailable");
    diagnostics.verifiedDefinitions=0;
    for(auto& d:definitions)if(ResolveDefinition(d))++diagnostics.verifiedDefinitions;
    diagnostics.completeCatalog=diagnostics.verifiedDefinitions==Count;
    diagnostics.initialized=true;diagnostics.phase=Phase::Idle;diagnostics.error=0;
    SampleCurrent();diagnostics.stage="all_weapons_original_catalog_resolved";return true;
}
inline LoadoutSnapshot DefaultLoadout(){
    LoadoutSnapshot result;
    constexpr const wchar_t* paths[]={
        L"/Game/Equippables/Guns/Rifles/AK/AssaultRifle_AK.AssaultRifle_AK_C",
        L"/Game/Equippables/Guns/Sidearms/BasePistol/BasePistol.BasePistol_C",
        L"/Game/Equippables/Melee/Ability_Melee_Base.Ability_Melee_Base_C"};
    for(size_t slot=0;slot<3;++slot)for(const auto& d:definitions)
        if(std::wcscmp(d.weaponClassPath,paths[slot])==0){result.slots[slot].id=d.id;result.slots[slot].valid=true;break;}
    result.held=result.slots[0].id;result.heldValid=result.slots[0].valid;return result;
}
inline bool SaveLoadout(){
    if(!Thread()||!diagnostics.initialized||diagnostics.restoring||diagnostics.busy
        ||original_effects::ResolveIdentity(pawnIdentity)!=diagnostics.pawn)return false;
    uint8_t alive=0;
    if(!Event(diagnostics.pawn,L"/Script/ShooterGame.ShooterCharacter.IsAlive",1,&alive)||!alive)return false;
    auto next=savedLoadout;bool changed=false;
    for(size_t slotIndex=0;slotIndex<3;++slotIndex){
        SlotSnapshot slot;
        auto originalSlot=all_weapons::Read<void*>(diagnostics.inventory,0x1b8+slotIndex*8);
        auto actor=all_weapons::Read<void*>(originalSlot,0x38);
        auto def=FindByClass(all_weapons::Read<void*>(actor,0x10));
        if(def&&def->equipmentSlot==static_cast<int>(slotIndex)&&original_players::IsA(originalSlot,0x68f63c0)
            &&all_weapons::Read<void*>(originalSlot,0x40)==diagnostics.inventory
            &&all_weapons::Read<void*>(actor,0x4a8)==diagnostics.pawn&&original_effects::CaptureIdentity(actor,slot.identity)){
            slot.valid=true;slot.id=def->id;slot.actor=actor;
        }
        changed=changed||next.slots[slotIndex].valid!=slot.valid||!Same(next.slots[slotIndex].id,slot.id)
            ||next.slots[slotIndex].actor!=slot.actor;
        next.slots[slotIndex]=slot;
    }
    const auto current=CurrentDefinition();
    if(current){changed=changed||!next.heldValid||!Same(next.held,current->id);next.held=current->id;next.heldValid=true;}
    if(changed)next.revision=savedLoadout.revision+1;
    savedLoadout=next;return true;
}
inline void* FindExisting(const Definition& definition){
    if(definition.equipmentSlot<0||definition.equipmentSlot>2)return nullptr;
    const auto slot=all_weapons::Read<void*>(diagnostics.inventory,0x1b8+static_cast<size_t>(definition.equipmentSlot)*8);
    const auto object=all_weapons::Read<void*>(slot,0x38);
    return original_players::IsA(slot,0x68f63c0)&&all_weapons::Read<void*>(slot,0x40)==diagnostics.inventory
        &&all_weapons::Read<void*>(object,0x10)==definition.runtimeClass
        &&all_weapons::Read<void*>(object,0x4a8)==diagnostics.pawn?object:nullptr;
}
inline bool Equip(const Guid& id,bool force=false){
    if(!Thread()||!diagnostics.initialized)return Fail(2304,"all_weapons_equip_requires_game_thread_and_initialize");
    if(diagnostics.busy||diagnostics.phase==Phase::Failed||(restorePlan.active&&!internalRestoreRequest))return false;
    const auto definition=Find(id);
    if(!definition||!definition->identityVerified)return false;
    if(!force&&CurrentDefinition()==definition&&diagnostics.boundWeapon==CurrentWeapon()&&diagnostics.ready){diagnostics.stage="all_weapons_already_current";return true;}
    if(!hooks.quiesce||!hooks.beforeFinishSpawn||!hooks.prepareWeapon||!hooks.weaponChanged||!hooks.beforeRetire)
        return Fail(2305,"all_weapons_requires_original_loadout_and_observer_lifecycle_hooks");
    if(!restorePlan.active)SaveLoadout();
    diagnostics.requestedDefinition=definition;diagnostics.requestedGuid=id;diagnostics.forcedRebuild=force;
    diagnostics.oldCurrent=diagnostics.boundWeapon?diagnostics.boundWeapon:CurrentWeapon();
    diagnostics.candidate=force?nullptr:FindExisting(*definition);diagnostics.existingOnly=diagnostics.candidate!=nullptr;
    diagnostics.observedSwitch=false;diagnostics.grantOnly=false;diagnostics.replaced=nullptr;diagnostics.targetSlot=nullptr;
    diagnostics.ready=false;diagnostics.skinReady=false;diagnostics.firingReady=false;diagnostics.busy=true;
    diagnostics.slotPublished=false;diagnostics.equipAccepted=false;diagnostics.requestStarted=GetTickCount64();
    diagnostics.phase=Phase::Quiesce;diagnostics.stage="all_weapons_quiescing_original_input";
    candidateIdentity={};replacedIdentity={};changeNotified=false;++diagnostics.requestSequence;
    if(diagnostics.candidate&&!original_effects::CaptureIdentity(diagnostics.candidate,candidateIdentity))return Fail(2326,"all_weapons_existing_actor_identity_invalid");
    return true;
}
inline bool EquipExisting(const Guid& id){
    const auto d=Find(id);return d&&FindExisting(*d)&&Equip(id,false);
}
inline bool EnsureSlot(){
    const auto type=diagnostics.requestedDefinition->equipmentSlot;
    const size_t offset=0x1b8+static_cast<size_t>(type)*sizeof(void*);
    auto slot=all_weapons::Read<void*>(diagnostics.inventory,offset);
    if(!slot){
        struct P{void* type;void* outer;void* result=nullptr;} p{engine::Class(0x68f63c0),diagnostics.inventory};
        if(!Event(engine::DefaultObject(engine::Class(0x43a9670)),L"/Script/Engine.GameplayStatics.SpawnObject",24,&p)
            ||!original_players::IsA(p.result,0x68f63c0))return Fail(2306,"all_weapons_original_slot_creation_failed");
        slot=p.result;
        if(!original_players::Write<uint8_t>(slot,0x30,static_cast<uint8_t>(type))
            ||!original_players::Write(slot,0x40,diagnostics.inventory)||!original_players::Write(diagnostics.inventory,offset,slot))
            return Fail(2307,"all_weapons_local_slot_authority_failed");
    }
    if(!original_players::IsA(slot,0x68f63c0)||original_players::IsA(slot,0x69cf3d0)
        ||all_weapons::Read<void*>(slot,0x40)!=diagnostics.inventory||all_weapons::Read<uint8_t>(slot,0x30)!=type)
        return Fail(2308,"all_weapons_original_slot_identity_mismatch");
    diagnostics.targetSlot=slot;diagnostics.replaced=all_weapons::Read<void*>(slot,0x38);
    if(diagnostics.replaced&&!original_effects::CaptureIdentity(diagnostics.replaced,replacedIdentity))
        return Fail(2309,"all_weapons_replaced_item_identity_invalid");
    return true;
}
inline bool SpawnCandidate(){
    const auto& definition=*diagnostics.requestedDefinition;
    engine::Transform transform;
    if(!Event(diagnostics.pawn,L"/Script/Engine.Actor.K2_GetActorLocation",24,&transform.translation))
        return Fail(2310,"all_weapons_original_pawn_transform_unavailable");
    diagnostics.candidate=engine::function<void*(*)(void*,void*,const engine::Transform*,uint8_t,void*,uint8_t)>(0x43fada0)
        (diagnostics.world,definition.runtimeClass,&transform,1,diagnostics.pawn,0);
    if(!diagnostics.candidate)return Fail(2311,"all_weapons_original_deferred_spawn_failed");
    if(!original_effects::CaptureIdentity(diagnostics.candidate,candidateIdentity)
        ||!original_players::Write(diagnostics.candidate,0xfa8,definition.dataAsset)
        ||!hooks.beforeFinishSpawn(diagnostics.candidate,definition))
        return Fail(2312,"all_weapons_original_prefinish_loadout_not_prepared");
    engine::function<void(*)(void*,const engine::Transform*,bool,void*,uint8_t)>(0x3ef6cb0)
        (diagnostics.candidate,&transform,false,nullptr,0);
    if(!original_effects::EnsureActor(diagnostics.candidate,"local_catalog_weapon"))
        return Fail(2313,"all_weapons_original_effect_manager_not_ready");
    if(!Event(diagnostics.candidate,L"/Script/Engine.Actor.SetOwner",8,&diagnostics.pawn)
        ||all_weapons::Read<void*>(diagnostics.candidate,0x210)!=diagnostics.pawn)
        return Fail(2314,"all_weapons_original_owner_assignment_failed");
    if(all_weapons::Read<void*>(diagnostics.candidate,0x4a8)!=diagnostics.pawn){
        const auto table=all_weapons::Read<void**>(diagnostics.candidate,0);
        const auto callback=all_weapons::Read<void*>(table,0x7d8);
        if(callback!=reinterpret_cast<void*>(engine::image+0x70ec480))
            return Fail(2315,"all_weapons_original_owner_callback_profile_mismatch");
        reinterpret_cast<void(*)(void*,void*,void*)>(callback)(diagnostics.candidate,nullptr,diagnostics.pawn);
    }
    if(all_weapons::Read<void*>(diagnostics.candidate,0x4a8)!=diagnostics.pawn
        ||!original_weapon::RegisterOriginalOfflineEquipEventFor(diagnostics.pawn,diagnostics.candidate))
        return Fail(2316,"all_weapons_original_owner_or_equipped_event_not_ready");
    return true;
}
inline bool RetireReplaced(){
    auto old=diagnostics.replaced;
    if(!old||old==diagnostics.candidate)return true;
    if(original_effects::ResolveIdentity(replacedIdentity)!=old)return true;
    if(old==CurrentWeapon()||old==all_weapons::Read<void*>(diagnostics.inventory,0x268))return false;
    for(size_t i=0;i<17;++i){auto slot=all_weapons::Read<void*>(diagnostics.inventory,0x1b8+i*8);
        if(original_players::IsA(slot,0x68f63c0)&&all_weapons::Read<void*>(slot,0x38)==old)return false;}
    if(!hooks.beforeRetire(old))return false;
    // Whole original actor destruction invokes attachment EndPlay710aa10,
    // which destroys AddedMeshes and preserves original Blueprint cleanup.
    if(!Event(old,L"/Script/Engine.Actor.K2_DestroyActor",0,nullptr))
        return Fail(2317,"all_weapons_original_replaced_actor_retire_failed");
    diagnostics.replaced=nullptr;return true;
}
inline bool QueueRetirement(void* actor,const original_effects::WeakIdentity& identity){
    if(!actor)return true;
    for(auto& item:retired)if(item.actor==actor)return true;
    for(auto& item:retired)if(!item.actor){item.actor=actor;item.identity=identity;return true;}
    return Fail(2327,"all_weapons_retirement_queue_full");
}
inline bool SweepRetired(){
    for(auto& item:retired)if(item.actor){
        auto actor=original_effects::ResolveIdentity(item.identity);
        if(actor!=item.actor){item={};continue;}
        if(actor==CurrentWeapon()||actor==all_weapons::Read<void*>(diagnostics.inventory,0x268))continue;
        bool held=false;
        for(size_t i=0;i<17;++i){auto slot=all_weapons::Read<void*>(diagnostics.inventory,0x1b8+i*8);
            if(original_players::IsA(slot,0x68f63c0)&&all_weapons::Read<void*>(slot,0x38)==actor){held=true;break;}}
        if(held)continue;
        if(!hooks.beforeRetire||!hooks.beforeRetire(actor))return false;
        if(!Event(actor,L"/Script/Engine.Actor.K2_DestroyActor",0,nullptr))return Fail(2328,"all_weapons_queued_native_retirement_failed");
        item={};
    }
    return true;
}
inline bool RestoreLoadout(const LoadoutSnapshot& snapshot){
    if(!Thread()||!diagnostics.initialized||diagnostics.busy||restorePlan.active||diagnostics.phase==Phase::Failed)return false;
    auto plan=snapshot;const auto defaults=DefaultLoadout();
    for(size_t i=0;i<3;++i){
        if(!plan.slots[i].valid)plan.slots[i]=defaults.slots[i];
        const auto d=Find(plan.slots[i].id);
        if(!d||!d->identityVerified||d->equipmentSlot!=static_cast<int>(i))return Fail(2329,"all_weapons_restore_definition_or_slot_unavailable");
        plan.slots[i].actor=nullptr;plan.slots[i].identity={};
    }
    bool heldFound=false;
    for(const auto& slot:plan.slots)if(plan.heldValid&&Same(slot.id,plan.held))heldFound=true;
    if(!heldFound){plan.held=plan.slots[0].id;plan.heldValid=true;}
    restorePlan.loadout=plan;restorePlan.cursor=0;restorePlan.active=true;
    diagnostics.restoring=true;diagnostics.ready=false;diagnostics.stage="all_weapons_restoring_three_original_slots";
    return DispatchRestore();
}
inline bool DispatchRestore(){
    if(!restorePlan.active||diagnostics.busy)return true;
    internalRestoreRequest=true;bool accepted=false;
    if(restorePlan.cursor<3){
        const auto id=restorePlan.loadout.slots[restorePlan.cursor++].id;
        accepted=Equip(id,true);
        if(accepted)diagnostics.grantOnly=true;
    }else if(restorePlan.cursor==3){
        ++restorePlan.cursor;accepted=EquipExisting(restorePlan.loadout.held);
    }else{
        internalRestoreRequest=false;restorePlan.active=false;diagnostics.restoring=false;
        diagnostics.ready=diagnostics.boundWeapon==CurrentWeapon()&&CurrentDefinition()!=nullptr;
        diagnostics.stage="all_weapons_three_slots_and_held_weapon_restored";SaveLoadout();return SweepRetired();
    }
    internalRestoreRequest=false;
    if(!accepted)return Fail(2330,"all_weapons_original_loadout_restore_request_rejected");
    return true;
}
inline bool ObserveExistingSwitch(){
    const auto current=CurrentWeapon();const auto definition=CurrentDefinition();
    if(!definition){diagnostics.ready=false;return true;} // Original ability may temporarily own the hands.
    if(current==diagnostics.boundWeapon&&diagnostics.bindingsReady){diagnostics.ready=true;return true;}
    if(!hooks.quiesce||!hooks.weaponChanged)return false;
    diagnostics.oldCurrent=diagnostics.boundWeapon;diagnostics.candidate=current;
    diagnostics.requestedDefinition=definition;diagnostics.requestedGuid=definition->id;
    diagnostics.existingOnly=true;diagnostics.observedSwitch=true;diagnostics.grantOnly=false;diagnostics.forcedRebuild=false;
    diagnostics.replaced=nullptr;diagnostics.targetSlot=nullptr;diagnostics.busy=true;diagnostics.ready=false;
    diagnostics.skinReady=false;diagnostics.firingReady=false;diagnostics.slotPublished=false;diagnostics.equipAccepted=false;
    diagnostics.requestStarted=GetTickCount64();diagnostics.phase=Phase::Quiesce;changeNotified=false;
    if(!original_effects::CaptureIdentity(current,candidateIdentity))return Fail(2331,"all_weapons_observed_current_identity_invalid");
    ++diagnostics.requestSequence;diagnostics.stage="all_weapons_observed_original_key_or_scroll_switch";return true;
}
inline bool Tick(){
    if(!Thread()||!diagnostics.initialized)return false;
    SampleCurrent();
    if(!diagnostics.busy){
        if(diagnostics.phase==Phase::Failed)return false;
        if(restorePlan.active)return DispatchRestore();
        SaveLoadout();
        if(!SweepRetired())return false;
        return ObserveExistingSwitch();
    }
    if(diagnostics.phase==Phase::Failed)return false;
    if(GetTickCount64()-diagnostics.requestStarted>20000)return Fail(2318,"all_weapons_original_switch_timeout");
    const auto definition=diagnostics.requestedDefinition;
    switch(diagnostics.phase){
    case Phase::Quiesce:
        if(!hooks.quiesce(diagnostics.oldCurrent))return true;
        diagnostics.bindingsReady=false;
        if(diagnostics.existingOnly){
            if(original_effects::ResolveIdentity(candidateIdentity)!=diagnostics.candidate
                ||FindExisting(*definition)!=diagnostics.candidate)return Fail(2332,"all_weapons_existing_slot_actor_changed");
            if(diagnostics.observedSwitch){
                // The original input/state machine already performed this
                // switch. Observe/rebind it without another equip request.
                if(CurrentWeapon()!=diagnostics.candidate){diagnostics.busy=false;diagnostics.phase=Phase::Idle;return true;}
                diagnostics.phase=Phase::Bind;return true;
            }
            if(!original_weapon::RegisterOriginalOfflineEquipEventFor(diagnostics.pawn,diagnostics.candidate))
                return Fail(2333,"all_weapons_existing_original_equipped_event_unavailable");
            diagnostics.phase=Phase::Equip;return true;
        }
        if(!EnsureSlot())return false;
        diagnostics.phase=Phase::Spawn;diagnostics.stage="all_weapons_spawning_original_definition";return true;
    case Phase::Spawn:
        if(!SpawnCandidate())return false;
        diagnostics.phase=Phase::Prepare;diagnostics.stage="all_weapons_waiting_original_loadout_and_scope";return true;
    case Phase::Prepare:{
        if(original_effects::ResolveIdentity(candidateIdentity)!=diagnostics.candidate)return Fail(2319,"all_weapons_candidate_expired");
        const auto result=hooks.prepareWeapon(diagnostics.candidate,*definition);
        if(result==Gate::Failed)return Fail(2320,"all_weapons_original_loadout_preparation_failed");
        if(result==Gate::Pending)return true;
        diagnostics.skinReady=true;diagnostics.phase=Phase::Grant;return true;}
    case Phase::Grant:
        if(all_weapons::Read<void*>(diagnostics.targetSlot,0x38)!=diagnostics.replaced)
            return Fail(2321,"all_weapons_inventory_slot_changed_during_prepare");
        if(!original_players::Write(diagnostics.targetSlot,0x38,diagnostics.candidate)
            ||!original_players::Write<uint8_t>(diagnostics.candidate,0x4b0,1)
            ||!Event(diagnostics.targetSlot,L"/Script/ShooterGame.ItemSlot.OnRep_Contents",0,nullptr))
            return Fail(2322,"all_weapons_original_slot_publication_failed");
        if(diagnostics.replaced)original_players::Write<uint8_t>(diagnostics.replaced,0x4b0,0);
        diagnostics.slotPublished=true;
        if(diagnostics.grantOnly){
            if(!QueueRetirement(diagnostics.replaced,replacedIdentity))return false;
            diagnostics.replaced=nullptr;diagnostics.completedSequence=diagnostics.requestSequence;++diagnostics.generation;
            diagnostics.busy=false;diagnostics.phase=Phase::Idle;diagnostics.stage="all_weapons_original_restore_slot_granted";
            return DispatchRestore();
        }
        diagnostics.phase=Phase::Equip;return true;
    case Phase::Equip:{
        struct P{uint8_t source=9,skipRestrictions=0,result=0;} p;
        if(!Event(diagnostics.candidate,L"/Script/ShooterGame.AresEquippable.TryEquip",3,&p)||!p.result)
            return Fail(2323,"all_weapons_original_try_equip_rejected");
        diagnostics.equipAccepted=true;diagnostics.phase=Phase::WaitCurrent;
        diagnostics.stage="all_weapons_waiting_original_equip_state";return true;}
    case Phase::WaitCurrent:
        if(CurrentWeapon()!=diagnostics.candidate||all_weapons::Read<void*>(diagnostics.inventory,0x268)!=diagnostics.candidate)return true;
        diagnostics.phase=Phase::Bind;diagnostics.stage="all_weapons_binding_current_original_weapon";return true;
    case Phase::Bind:
        if(diagnostics.observedSwitch&&CurrentWeapon()!=diagnostics.candidate){
            diagnostics.busy=false;diagnostics.phase=Phase::Idle;return true;
        }
        if(!changeNotified){
            if(!hooks.weaponChanged(diagnostics.oldCurrent,diagnostics.candidate,*definition))
                return Fail(2324,"all_weapons_original_observers_not_rebound");
            changeNotified=true;
        }
        if(!RetireReplaced())return diagnostics.phase!=Phase::Failed;
        diagnostics.completedSequence=diagnostics.requestSequence;++diagnostics.generation;
        diagnostics.boundWeapon=diagnostics.candidate;diagnostics.bindingsReady=true;diagnostics.busy=false;diagnostics.ready=true;diagnostics.skinReady=true;diagnostics.phase=Phase::Idle;
        diagnostics.stage="all_weapons_original_switch_completed";SampleCurrent();
        if(restorePlan.active)return DispatchRestore();
        SaveLoadout();return SweepRetired();
    default:return false;
    }
}
inline bool OnPawnReplaced(void* world,void* oldPawn,void* newPawn){
    if(!Thread()||diagnostics.pawn!=oldPawn)return false;
    const auto snapshot=savedLoadout;
    // Called by the root's death/respawn coordinator after its old-slot cleanup.
    // Never traverse the old Pawn or Inventory here; they may be pending kill.
    if(hooks.quiesce&&!hooks.quiesce(diagnostics.currentWeapon))return false;
    auto pending=original_effects::ResolveIdentity(candidateIdentity);
    if(pending&&pending!=diagnostics.currentWeapon){
        if(!hooks.beforeRetire||!hooks.beforeRetire(pending))return false;
        if(!Event(pending,L"/Script/Engine.Actor.K2_DestroyActor",0,nullptr))return false;
    }
    // These weak identities were captured while IsAlive was true. Do not walk
    // the corpse's inventory to reconstruct or clean up a loadout.
    for(const auto& slot:snapshot.slots)
        if(slot.actor&&!QueueRetirement(slot.actor,slot.identity))return false;
    const auto generation=diagnostics.generation,requested=diagnostics.requestSequence,completed=diagnostics.completedSequence;
    diagnostics=Diagnostics{};diagnostics.generation=generation;diagnostics.requestSequence=requested;diagnostics.completedSequence=completed;
    candidateIdentity={};replacedIdentity={};changeNotified=false;restorePlan={};internalRestoreRequest=false;
    if(!Initialize(world,newPawn))return false;
    return RestoreLoadout(snapshot);
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    std::fprintf(f,"{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u,\"phase\":%u,\"initialized\":%d,\"busy\":%d,\"ready\":%d,\"firing_ready\":%d,\"skin_ready\":%d,\"verified_definitions\":%zu,\"complete_catalog\":%d,\"request_sequence\":%u,\"completed_sequence\":%u,\"generation\":%u,\"current_weapon\":\"%p\",\"current_guid\":\"%s\",\"candidate\":\"%p\",\"requested_guid\":\"%s\",\"forced_rebuild\":%d,\"slot_published\":%d,\"equip_accepted\":%d,\"definitions\":[",
        diagnostics.stage,diagnostics.error,static_cast<unsigned>(diagnostics.phase),diagnostics.initialized,diagnostics.busy,
        diagnostics.ready,diagnostics.firingReady,diagnostics.skinReady,diagnostics.verifiedDefinitions,diagnostics.completeCatalog,
        diagnostics.requestSequence,diagnostics.completedSequence,diagnostics.generation,diagnostics.currentWeapon,
        diagnostics.currentDefinition?diagnostics.currentDefinition->guid:"",diagnostics.candidate,
        diagnostics.requestedDefinition?diagnostics.requestedDefinition->guid:"",diagnostics.forcedRebuild,diagnostics.slotPublished,diagnostics.equipAccepted);
    for(size_t i=0;i<Count;++i){const auto& d=definitions[i];
        std::fprintf(f,"%s{\"guid\":\"%s\",\"label\":\"%s\",\"category\":\"%s\",\"equipment_slot\":%d,\"identity_verified\":%d,\"validation_stage\":\"%s\",\"class\":\"%p\",\"data_asset\":\"%p\"}",
            i?",":"",d.guid,d.label,d.category,d.equipmentSlot,d.identityVerified,d.validationStage,d.runtimeClass,d.dataAsset);
    }
    std::fprintf(f,"],\"bound_weapon\":\"%p\",\"restoring\":%d,\"restore_cursor\":%u,\"existing_only\":%d,\"observed_native_switch\":%d,\"grant_only\":%d,\"saved_loadout_revision\":%llu,\"saved_slots\":[",
        diagnostics.boundWeapon,diagnostics.restoring,restorePlan.cursor,diagnostics.existingOnly,diagnostics.observedSwitch,diagnostics.grantOnly,
        static_cast<unsigned long long>(savedLoadout.revision));
    for(size_t i=0;i<3;++i){const auto d=savedLoadout.slots[i].valid?Find(savedLoadout.slots[i].id):nullptr;
        std::fprintf(f,"%s{\"slot\":%zu,\"guid\":\"%s\",\"valid\":%d}",i?",":"",i,d?d->guid:"",savedLoadout.slots[i].valid);}
    const auto held=savedLoadout.heldValid?Find(savedLoadout.held):nullptr;
    std::fprintf(f,"],\"saved_held_guid\":\"%s\"}\n",held?held->guid:"");
    const bool ok=std::ferror(f)==0;std::fclose(f);return ok;
}
} // namespace all_weapons
