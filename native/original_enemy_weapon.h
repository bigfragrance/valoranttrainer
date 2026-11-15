#pragma once
#include "original_players.h"
#include "original_weapon.h"

// Only the missing local authority grant for another real roster character.
// Every actor, owner notification, inventory callback and equip transition is
// the same original path used for the local AK. No weapon simulator or static
// socket prop is created, and the local weapon observer/context is untouched.
namespace original_enemy_weapon {
struct Entry {
    bool attempted=false,ownerBound=false,granted=false,equipRequested=false,current=false,ready=false;
    void *pawn=nullptr,*inventory=nullptr,*slot=nullptr,*weapon=nullptr,*mesh3p=nullptr,*stateMachine=nullptr;
    int ammo=-1;
};
inline Entry entries[original_players::MaxPlayers]{};
inline unsigned error=0;
inline const char* stage="original_enemy_weapons_uninitialized";
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Fail(unsigned code,const char* text){error=code;stage=text;return false;}
inline bool Sample(size_t index){
    if(!original_players::Thread()||index>=original_players::MaxPlayers||!entries[index].weapon)return false;
    auto& e=entries[index];e.current=Read<void*>(e.inventory,0x280)==e.weapon;
    e.ownerBound=Read<void*>(e.weapon,0x210)==e.pawn&&Read<void*>(e.weapon,0x4a8)==e.pawn;
    e.mesh3p=Read<void*>(e.weapon,0xdf0);e.stateMachine=Read<void*>(e.weapon,0xe00);
    auto ammo=original_weapon::Component(e.weapon,0x63c7e00);e.ammo=-1;
    if(ammo)engine::Event(ammo,L"/Script/ShooterGame.AmmoComponent.GetAmmo",&e.ammo);
    e.ready=e.current&&e.ownerBound&&e.mesh3p&&e.stateMachine&&ammo;
    return true;
}
inline bool SpawnAK(size_t index){
    if(!original_players::Thread()||index>=original_players::MaxPlayers
       ||static_cast<int>(index)==original_players::localIndex)return Fail(1250,"enemy_weapon_requires_other_original_player");
    auto& roster=original_players::players[index];auto& e=entries[index];
    if(!roster.ready||!roster.registered||!roster.characterInitialized)
        return Fail(1251,"enemy_weapon_original_roster_not_ready");
    if(e.attempted)return Fail(1252,"enemy_weapon_grant_already_attempted");
    e.attempted=true;e.pawn=roster.character;e.inventory=roster.inventory;
    if(!original_players::IsA(e.inventory,0x6499330)||Read<void*>(e.inventory,0xc8)!=e.pawn
       ||Read<void*>(e.inventory,0x2f0)!=e.pawn)return Fail(1253,"enemy_weapon_inventory_owner_invalid");
    e.slot=Read<void*>(e.inventory,0x1b8);
    if(!e.slot){
        struct NewSlot {void* type;void* outer;void* result=nullptr;} p{engine::Class(0x68f63c0),e.inventory};
        if(!engine::Event(engine::DefaultObject(engine::Class(0x43a9670)),L"/Script/Engine.GameplayStatics.SpawnObject",&p)
           ||!original_players::IsA(p.result,0x68f63c0)||Read<void*>(p.result,0x28)!=e.inventory)
            return Fail(1254,"enemy_weapon_original_slot_constructor_failed");
        e.slot=p.result;
        if(!original_players::Write<uint8_t>(e.slot,0x30,0)||!original_players::Write(e.slot,0x40,e.inventory)
           ||!original_players::Write(e.inventory,0x1b8,e.slot))return Fail(1255,"enemy_weapon_slot_authority_assembly_failed");
    }
    if(!original_players::IsA(e.slot,0x68f63c0)||Read<void*>(e.slot,0x40)!=e.inventory||Read<void*>(e.slot,0x38))
        return Fail(1256,"enemy_weapon_primary_slot_not_empty_original_item_slot");
    auto type=engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t,void*)>(0x1d0c6e0)
        (engine::Class(0x645c660),nullptr,original_weapon::kAK,nullptr,0,nullptr);
    if(!type)return Fail(1257,"enemy_weapon_original_ak_class_missing");
    engine::Transform transform;
    if(!engine::Event(e.pawn,L"/Script/Engine.Actor.K2_GetActorLocation",&transform.translation))
        return Fail(1258,"enemy_weapon_original_pawn_location_missing");
    stage="enemy_weapon_original_deferred_spawn";
    e.weapon=engine::function<void*(*)(void*,void*,const engine::Transform*,uint8_t,void*,uint8_t)>(0x43fada0)
        (original_players::world,type,&transform,1,nullptr,0);
    if(!e.weapon)return Fail(1259,"enemy_weapon_original_actor_spawn_failed");
    engine::function<void(*)(void*,const engine::Transform*,bool,void*,uint8_t)>(0x3ef6cb0)
        (e.weapon,&transform,false,nullptr,0);
    if(!original_effects::EnsureActor(e.weapon,"enemy_weapon"))
        return Fail(original_effects::State().error,original_effects::State().stage);
    if(!engine::Event(e.weapon,L"/Script/Engine.Actor.SetOwner",&e.pawn)||Read<void*>(e.weapon,0x210)!=e.pawn)
        return Fail(1260,"enemy_weapon_original_actor_owner_failed");
    if(Read<void*>(e.weapon,0x4a8)!=e.pawn){
        auto table=Read<void**>(e.weapon,0);
        auto owner=Read<void*>(table,0x7d8);
        if(owner!=reinterpret_cast<void*>(engine::image+0x70ec480))return Fail(1261,"enemy_weapon_original_owner_callback_changed");
        reinterpret_cast<void(*)(void*,void*,void*)>(owner)(e.weapon,nullptr,e.pawn);
    }
    if(Read<void*>(e.weapon,0x4a8)!=e.pawn)return Fail(1262,"enemy_weapon_original_owner_callback_not_observed");
    if(!original_weapon::RegisterOriginalOfflineEquipEventFor(e.pawn,e.weapon))
        return Fail(1265,"enemy_weapon_original_local_equip_event_registration_failed");
    if(!original_players::Write(e.slot,0x38,e.weapon)||!original_players::Write<uint8_t>(e.weapon,0x4b0,1)
       ||!engine::Event(e.slot,L"/Script/ShooterGame.ItemSlot.OnRep_Contents",nullptr))
        return Fail(1263,"enemy_weapon_original_grant_notification_failed");
    e.granted=true;
    struct Equip {uint8_t source=9,skipRestrictions=0,result=0;} equip;
    if(!engine::Event(e.weapon,L"/Script/ShooterGame.AresEquippable.TryEquip",&equip)||!equip.result)
        return Fail(1264,"enemy_weapon_original_equip_rejected");
    e.equipRequested=true;Sample(index);error=0;stage="enemy_weapons_granted_waiting_original_equips";return true;
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    std::fprintf(f,"{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u,\"weapons\":[",stage,error);
    bool first=true;
    for(size_t index=0;index<original_players::MaxPlayers;++index){const auto& e=entries[index];if(!e.attempted)continue;
        std::fprintf(f,"%s{\"roster_index\":%zu,\"owner_bound\":%s,\"granted\":%s,\"equip_requested\":%s,\"current\":%s,\"ready\":%s,\"ammo\":%d,\"weapon\":\"%p\",\"mesh3p\":\"%p\",\"state_machine\":\"%p\"}",
            first?"":",",index,e.ownerBound?"true":"false",e.granted?"true":"false",e.equipRequested?"true":"false",e.current?"true":"false",e.ready?"true":"false",e.ammo,e.weapon,e.mesh3p,e.stateMachine);first=false;
    }
    std::fputs("]}\n",f);const bool ok=std::ferror(f)==0;std::fclose(f);return ok;
}
}
