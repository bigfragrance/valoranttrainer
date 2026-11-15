#pragma once
#include "local_server_equipment.h"
#include "local_server_client.h"
#include "original_armor.h"
#include "armor_poll_performance.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <string>

// Separate-host default loadout wiring for the real, non-equippable heavy
// armor item. Its original SCS/Blueprint/native damage section owns the 50
// shield. No shield life, maximum, damage partition or healability flag is
// written. Client Owner/slot/component replication runs its original leaves.
namespace local_server_armor {
using Weak=original_inventory_slots::WeakIdentity;
inline constexpr size_t MaxPlayers=original_players::MaxPlayers;
inline constexpr uint8_t ArmorSlot=11;
inline constexpr float HeavyMaximum=50;
struct Entry {
    bool used=false,spawnAttempted=false,spawned=false,owned=false,replicated=false;
    bool published=false,authAttached=false,ready=false,retired=false,readback=false;
    size_t index=0;uint32_t epoch=0;unsigned error=0;
    void *pc=nullptr,*ps=nullptr,*pawn=nullptr,*inventory=nullptr,*slot=nullptr;
    void *item=nullptr,*section=nullptr,*damageable=nullptr;
    Weak pcId{},psId{},pawnId{},inventoryId{},slotId{},itemId{},sectionId{},damageableId{};
    float life=0,maximum=0,shield=0,maxShield=0,rawShield=0,damageScale=0;
    bool sectionAlive=false,canBeHealed=false,canBeDestroyed=false,sectionReplicated=false;
    uint32_t effectiveShields=0;uint64_t samples=0;
    const char* stage="server_armor_absent";
};
struct ClientObservation {
    bool observed=false,itemPresent=false,itemOwned=false,slotMatches=false,sectionPresent=false;
    bool sectionAttached=false,sectionReplicated=false,heavyClass=false,readback=false;
    bool localPair=false,worldMatches=false,tupleStable=false,weakStable=false,flagsValid=false;
    uint64_t samples=0,sampleFaults=0;uintptr_t exceptionRva=0;
    uint32_t epoch=0,effectiveShields=0;int32_t respawn=-1;uint8_t itemRole=255;
    Weak pawnId{},itemId{},sectionId{};float life=0,maximum=0,shield=0,maxShield=0,rawShield=0;
    const char* stage="client_armor_not_observed";
};
struct Diagnostics {
    bool initialized=false,restartRequired=false;unsigned error=0;uintptr_t exceptionRva=0;
    DWORD gameThread=0;void *world=nullptr,*heavyClass=nullptr;Weak worldId{},classId{};
    uint64_t registrations=0,grants=0,samples=0,retirements=0,expiredRecords=0;
    uint64_t actorReplicationEnables=0,componentReplicationEnables=0,authorityAttaches=0;
    uint64_t slotPublications=0,slotClears=0,destroyRequests=0,alreadyDestroying=0;
    const char* stage="server_armor_not_initialized";ClientObservation client;
};
inline Diagnostics state;
inline std::array<Entry,MaxPlayers> entries{};
inline SRWLOCK jsonLock=SRWLOCK_INIT;
inline std::string jsonSnapshot="{\"schema_version\":1,\"stage\":\"server_armor_not_initialized\",\"error\":0}";
inline const Diagnostics& GetState(){return state;}
inline const Entry* Armor(size_t index){return index<entries.size()?&entries[index]:nullptr;}
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Capture(void* object,Weak& identity){return original_inventory_slots::CaptureIdentity(object,identity)&&Live(object,identity);}
inline bool Thread(){return !state.restartRequired&&original_players::Thread()&&(!state.gameThread||state.gameThread==GetCurrentThreadId());}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;return false;}
inline bool Fail(Entry& e,unsigned error,const char* stage){e.error=error;e.ready=false;e.stage=stage;return Fail(error,stage);}
inline bool Wait(Entry& e,const char* stage){e.ready=false;e.stage=stage;state.stage=stage;return true;}
inline bool Event(void* object,const wchar_t* path,uint16_t bytes,void* parameters){return original_damage::Event(object,path,bytes,parameters);}
inline bool NativeMethod(const wchar_t* path,uint16_t bytes,uintptr_t exec){
    auto f=engine::Find(path);return f&&Read<uint16_t>(f,0xbe)==bytes&&(Read<uint32_t>(f,0xb8)&0x400)
        &&Read<uintptr_t>(f,0xe0)==engine::image+exec;
}
inline bool Context(){return Thread()&&state.initialized&&Live(state.world,state.worldId)&&Live(state.heavyClass,state.classId)
    &&local_server_equipment::State().world==state.world;}
inline bool Source(const Entry& e){
    armor_poll_performance::Scope measurement(armor_poll_performance::Slot::SourceGuard);
    const auto p=local_server_equipment::State(e.index);
    return p&&p->registered&&!p->retiring&&p->controller==e.pc&&p->playerState==e.ps&&p->pawn==e.pawn&&p->inventory==e.inventory&&p->sourceEpoch==e.epoch
        &&local_server_equipment::Ownership(*p)&&Live(e.pc,e.pcId)&&Live(e.ps,e.psId)&&Live(e.pawn,e.pawnId)
        &&Live(e.inventory,e.inventoryId)&&Live(e.slot,e.slotId)&&Live(e.damageable,e.damageableId)
        &&original_inventory_slots::SameIdentity(e.pawnId,p->pawnIdentity)
        &&Read<void*>(e.inventory,0x1b8+ArmorSlot*8)==e.slot&&Read<void*>(e.slot,0x40)==e.inventory;
}
inline bool Registered(void* damageable,void* section){
    const auto a=Read<original_damage::Array<void*>>(damageable,0x1e8);
    if(a.count<0||a.count>32||a.capacity<a.count||(a.count&&!original_players::Readable(a.data,static_cast<size_t>(a.count)*8)))return false;
    unsigned count=0;for(int32_t n=0;n<a.count;++n)count+=a.data[n]==section;
    return count==1&&Read<void*>(section,0x110)==damageable;
}
inline bool SampleImpl(Entry& e,bool initial=false){
    armor_poll_performance::Scope measurement(armor_poll_performance::Slot::FullSample);
    if(!Source(e)||!Live(e.item,e.itemId)||!Live(e.section,e.sectionId))return Fail(e,5504,"server_armor_sample_live_source_or_item_changed");
    e.readback=false;
    if(Read<void*>(e.slot,0x38)!=e.item||Read<void*>(e.item,0x210)!=e.pawn||Read<void*>(e.item,0x4a8)!=e.pawn
       ||Read<void*>(e.section,0xc8)!=e.item||!Registered(e.damageable,e.section))return Fail(e,5505,"server_armor_original_inventory_or_section_attachment_missing");
    original_damage::SectionAudit audit;
    {armor_poll_performance::Scope measurementSections(armor_poll_performance::Slot::DescribeSections);
    if(!original_damage::DescribeSections(e.pawn,e.damageable,audit))return Fail(e,5506,"server_armor_original_section_audit_failed");}
    unsigned owned=0;for(int32_t n=0;n<audit.count;++n){const auto& r=audit.entries[n];if(r.type==1&&!r.emptyShield&&r.object==e.section)++owned;}
    e.effectiveShields=audit.effectiveShields;e.life=Read<float>(e.section,0x124);e.maximum=Read<float>(e.section,0x128);
    e.damageScale=Read<float>(e.section,0x144);e.sectionAlive=Read<uint8_t>(e.section,0x120)!=0;
    e.canBeDestroyed=Read<uint8_t>(e.section,0x119)!=0;e.canBeHealed=Read<uint8_t>(e.section,0x149)==1;
    e.sectionReplicated=(Read<uint8_t>(e.section,0xa0)&0x20)!=0;
    if(owned!=1||e.effectiveShields!=1||Read<uint8_t>(e.section,0x118)!=1||e.canBeDestroyed||!e.canBeHealed||!e.sectionReplicated
       ||!std::isfinite(e.life)||!std::isfinite(e.maximum)||!std::isfinite(e.damageScale)
       ||e.life<0||e.life>HeavyMaximum+.01f||e.maximum!=HeavyMaximum||e.damageScale!=1
       ||!Event(e.pawn,L"/Script/ShooterGame.ShooterCharacter.GetShield",4,&e.shield)
       ||!Event(e.pawn,L"/Script/ShooterGame.ShooterCharacter.GetMaxShield",4,&e.maxShield)
       ||!Event(e.damageable,L"/Script/ShooterGame.DamageableComponent.GetRawShield",4,&e.rawShield)
       ||!std::isfinite(e.shield)||!std::isfinite(e.maxShield)||!std::isfinite(e.rawShield)
       ||e.shield<0||e.shield>HeavyMaximum+.01f||e.maxShield!=HeavyMaximum||std::abs(e.rawShield-e.life)>.01f
       ||(initial&&(e.life!=HeavyMaximum||e.rawShield!=HeavyMaximum||e.shield!=HeavyMaximum)))
        return Fail(e,5507,"server_armor_requires_actual_native_heavy_shield_and_original_flags");
    e.readback=true;e.ready=true;++e.samples;++state.samples;e.stage="server_armor_original_heavy_section_verified";return true;
}
inline bool InitializeImpl(void* world){
    if(state.initialized)return Context()&&state.world==world;
    if(!Thread()||!local_server_equipment::State().initialized||local_server_equipment::State().world!=world)return false;
    state.gameThread=GetCurrentThreadId();state.world=world;
    if(!Capture(world,state.worldId))return Fail(5500,"server_armor_world_weak_identity_missing");
    state.heavyClass=engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t,void*)>(0x1d0c6e0)
        (engine::Class(original_armor::ItemClass),nullptr,original_armor::HeavyClassPath,nullptr,0,nullptr);
    auto defaults=state.heavyClass?engine::DefaultObject(state.heavyClass):nullptr;uint8_t slot=255,equippable=1;
    if(!Capture(state.heavyClass,state.classId)||!original_players::IsA(defaults,original_armor::ItemClass)
       ||!Event(defaults,L"/Script/ShooterGame.AresItem.GetItemSlot",1,&slot)||slot!=ArmorSlot
       ||!Event(defaults,L"/Script/ShooterGame.AresItem.IsEquippable",1,&equippable)||equippable)
        return Fail(5501,"server_armor_original_heavy_non_equippable_slot_not_verified");
    if(!NativeMethod(L"/Script/Engine.Actor.SetReplicates",1,0x3ef2c10)
       ||!NativeMethod(L"/Script/Engine.ActorComponent.SetIsReplicated",1,0x41296b0)
       ||!NativeMethod(L"/Script/ShooterGame.AttachedDamageSectionComponent.AuthAttachDamageSection",8,0x6588b60))
        return Fail(5502,"server_armor_original_replication_or_attach_abi_changed");
    state.initialized=true;state.stage="server_armor_original_heavy_factory_ready";return true;
}
inline bool Advance(Entry& e){
    if(e.error)return false;
    const auto current=local_server_equipment::State(e.index);
    if(current&&current->registered&&current->retiring&&current->pawn==e.pawn&&current->sourceEpoch==e.epoch)
        return Wait(e,"server_armor_waiting_current_life_retirement_boundary");
    if(!Source(e))return Fail(e,5508,"server_armor_current_authority_registration_changed");
    if(e.ready)return SampleImpl(e);
    if(!original_effects::State().initialized)return Wait(e,"server_armor_waiting_original_effect_services");
    if(!e.spawned){
        const auto existing=Read<void*>(e.slot,0x38);
        if(existing)return Fail(e,5509,"server_armor_default_grant_requires_empty_armor_slot");
        if(e.spawnAttempted)return Fail(e,5510,"server_armor_partial_spawn_cannot_be_replayed");
        e.spawnAttempted=true;engine::Transform transform;
        if(!Event(e.pawn,L"/Script/Engine.Actor.K2_GetActorLocation",24,&transform.translation))return Fail(e,5511,"server_armor_original_pawn_transform_missing");
        e.item=engine::function<void*(*)(void*,void*,const engine::Transform*,uint8_t,void*,uint8_t)>(0x43fada0)
            (e.pawn,state.heavyClass,&transform,1,e.pawn,0);
        if(!original_players::IsA(e.item,original_armor::ItemClass)||Read<void*>(e.item,0x10)!=state.heavyClass
           ||Read<uint8_t>(e.item,0x224)!=3||!Capture(e.item,e.itemId))return Fail(e,5512,"server_armor_original_deferred_factory_failed");
        engine::function<void(*)(void*,const engine::Transform*,bool,void*,uint8_t)>(0x3ef6cb0)(e.item,&transform,false,nullptr,0);
        struct Query {void* type;void* result=nullptr;} q{engine::Class(original_armor::AttachedSectionClass)};
        if(!Event(e.item,L"/Script/Engine.Actor.GetComponentByClass",16,&q)||!original_players::IsA(q.result,original_armor::AttachedSectionClass)
           ||Read<void*>(q.result,0xc8)!=e.item||!Capture(q.result,e.sectionId))return Fail(e,5513,"server_armor_original_scs_attached_section_missing");
        e.section=q.result;e.spawned=true;
    }
    if(!Live(e.item,e.itemId)||!Live(e.section,e.sectionId))return Fail(e,5514,"server_armor_factory_result_weak_identity_expired");
    if(!original_effects::EnsureActor(e.item,"server_default_heavy_armor")){
        if(original_effects::State().error)return Fail(e,original_effects::State().error,original_effects::State().stage);
        return Wait(e,"server_armor_waiting_original_effect_context");
    }
    if(!e.owned){
        if(!Event(e.item,L"/Script/Engine.Actor.SetOwner",8,&e.pawn)||Read<void*>(e.item,0x210)!=e.pawn)return Fail(e,5515,"server_armor_original_actor_owner_not_observed");
        if(Read<void*>(e.item,0x4a8)!=e.pawn){
            auto callback=Read<void*>(Read<void**>(e.item),0x7d8);
            if(callback!=reinterpret_cast<void*>(engine::image+original_armor::OriginalOwnerCallback))return Fail(e,5516,"server_armor_typed_owner_callback_profile_changed");
            reinterpret_cast<void(*)(void*,void*,void*)>(callback)(e.item,nullptr,e.pawn);
        }
        if(Read<void*>(e.item,0x4a8)!=e.pawn)return Fail(e,5517,"server_armor_original_typed_owner_missing");
        e.owned=true;
    }
    if(!e.replicated){
        uint8_t yes=1;
        if(!(Read<uint8_t>(e.item,0x11a)&0x10)){
            if(!Event(e.item,L"/Script/Engine.Actor.SetReplicates",1,&yes))return Fail(e,5518,"server_armor_native_actor_replication_enable_failed");
            ++state.actorReplicationEnables;
        }
        if(!(Read<uint8_t>(e.section,0xa0)&0x20)){
            if(!Event(e.section,L"/Script/Engine.ActorComponent.SetIsReplicated",1,&yes))return Fail(e,5519,"server_armor_native_section_replication_enable_failed");
            ++state.componentReplicationEnables;
        }
        if(!(Read<uint8_t>(e.item,0x11a)&0x10)||!(Read<uint8_t>(e.section,0xa0)&0x20)||Read<uint8_t>(e.item,0x224)!=3)
            return Fail(e,5520,"server_armor_native_replication_readback_failed");
        e.replicated=true;
    }
    if(!e.published){
        if(Read<void*>(e.slot,0x38))return Fail(e,5521,"server_armor_actual_slot_changed_before_publication");
        // Only missing AuthAddItem data, matching the established host inventory
        // publisher. No client inventory/owner callback is invoked on the host.
        if(!original_players::Write(e.slot,0x38,e.item)||!original_players::Write<uint8_t>(e.item,0x4b0,1)
           ||!Event(e.slot,L"/Script/ShooterGame.ItemSlot.OnRep_Contents",0,nullptr))return Fail(e,5522,"server_armor_original_slot_publication_failed");
        e.published=true;++state.slotPublications;
    }
    if(!e.authAttached){
        if(!Event(e.item,L"/Game/Gear/BasicArmorItem.BasicArmorItem_C.ReceiveAuthSetOwningCharacter",8,&e.pawn))
            return Fail(e,5523,"server_armor_original_authority_attach_graph_failed");
        e.authAttached=true;++state.authorityAttaches;
    }
    if(!SampleImpl(e,true))return false;
    ++state.grants;state.error=0;state.stage="server_armor_original_heavy_granted";return true;
}
inline bool RegisterOrEnsureImpl(size_t index,void* pc,void* ps,void* pawn,uint32_t epoch){
    if(!Context()||index>=entries.size()||!epoch)return false;
    auto& e=entries[index];const auto p=local_server_equipment::State(index);
    if(!p||!p->registered||p->controller!=pc||p->playerState!=ps||p->pawn!=pawn||p->sourceEpoch!=epoch||!local_server_equipment::Ownership(*p))
        return Fail(5524,"server_armor_requires_current_native_equipment_authority_tuple");
    if(e.used){
        if(e.pawn==pawn&&e.epoch==epoch&&original_inventory_slots::SameIdentity(e.pawnId,p->pawnIdentity))return Advance(e);
        if(Live(e.item,e.itemId))return Fail(e,5525,"server_armor_old_live_item_requires_retirement_before_new_life");
        e={};++state.expiredRecords; // Private expired record only; no claimed engine release/death.
    }
    e={};e.used=true;e.index=index;e.epoch=epoch;e.pc=pc;e.ps=ps;e.pawn=pawn;e.inventory=p->inventory;
    e.pcId=p->controllerIdentity;e.psId=p->stateIdentity;e.pawnId=p->pawnIdentity;e.inventoryId=p->inventoryIdentity;
    e.slot=Read<void*>(e.inventory,0x1b8+ArmorSlot*8);e.damageable=Read<void*>(pawn,0xc70);
    if(!original_players::IsA(e.slot,original_inventory_slots::ItemSlotClass)||original_players::IsA(e.slot,original_inventory_slots::MultiItemSlotClass)
       ||Read<uint8_t>(e.slot,0x30)!=ArmorSlot||Read<void*>(e.slot,0x40)!=e.inventory
       ||!original_players::IsA(e.damageable,original_damage::DamageableClass)||Read<void*>(e.damageable,0xc8)!=pawn
       ||!Capture(e.slot,e.slotId)||!Capture(e.damageable,e.damageableId))return Fail(e,5526,"server_armor_real_slot_or_damageable_identity_missing");
    ++state.registrations;return Advance(e);
}
inline bool PollImpl(){
    armor_poll_performance::Scope measurement(armor_poll_performance::Slot::ArmorPoll);
    if(!Context())return false;
    for(auto& e:entries)if(e.used&&!e.retired&&!Advance(e))return false;
    return true;
}
inline bool Ready(size_t index,void* pawn,uint32_t epoch){
    armor_poll_performance::Scope measurement(armor_poll_performance::Slot::Ready);
    if(!Context()||index>=entries.size())return false;const auto& e=entries[index];
    return e.used&&!e.retired&&!e.error&&e.ready&&e.readback&&e.pawn==pawn&&e.epoch==epoch&&Source(e);
}
inline bool BeforeRetireImpl(size_t index,void* pawn,uint32_t epoch){
    if(!Context()||index>=entries.size())return false;auto& e=entries[index];if(!e.used)return true;
    if(e.pawn!=pawn||e.epoch!=epoch)return Fail(e,5527,"server_armor_retirement_life_mismatch");
    if(!e.item||!Live(e.item,e.itemId)){e={};++state.expiredRecords;return true;}
    uint8_t destroying=0;
    if(!Event(e.item,L"/Script/Engine.Actor.IsActorBeingDestroyed",1,&destroying))return Fail(e,5528,"server_armor_destroy_state_query_failed");
    if(destroying){e={};++state.alreadyDestroying;return true;}
    if(!Live(e.pawn,e.pawnId)||!Live(e.damageable,e.damageableId)||!Live(e.slot,e.slotId))return Fail(e,5529,"server_armor_retirement_requires_live_old_native_context");
    if(Read<void*>(e.item,0x210)&&Read<void*>(e.item,0x210)!=e.pawn)return Fail(e,5530,"server_armor_retirement_item_transferred_to_other_owner");
    if(Live(e.section,e.sectionId)&&(Registered(e.damageable,e.section)||Read<void*>(e.section,0x150)||Read<void*>(e.section,0x110))){
        if(!Event(e.item,L"/Game/Gear/BasicArmorItem.BasicArmorItem_C.ReceiveAuthClearOwningCharacter",8,&e.pawn))
            return Fail(e,5531,"server_armor_original_authority_clear_graph_failed");
    }
    if(Read<void*>(e.slot,0x38)==e.item){void* empty=nullptr;
        if(!original_players::Write(e.slot,0x38,empty)||!original_players::Write<uint8_t>(e.item,0x4b0,0)
           ||!Event(e.slot,L"/Script/ShooterGame.ItemSlot.OnRep_Contents",0,nullptr))return Fail(e,5532,"server_armor_original_slot_clear_failed");
        ++state.slotClears;
    }
    void* empty=nullptr;
    if(Read<void*>(e.item,0x210)&&!Event(e.item,L"/Script/Engine.Actor.SetOwner",8,&empty))return Fail(e,5533,"server_armor_original_actor_owner_clear_failed");
    if(Read<void*>(e.item,0x4a8)){
        if(Read<void*>(e.item,0x4a8)!=e.pawn)return Fail(e,5530,"server_armor_retirement_item_transferred_to_other_owner");
        auto callback=Read<void*>(Read<void**>(e.item),0x7d8);
        if(callback!=reinterpret_cast<void*>(engine::image+original_armor::OriginalOwnerCallback))return Fail(e,5516,"server_armor_typed_owner_callback_profile_changed");
        reinterpret_cast<void(*)(void*,void*,void*)>(callback)(e.item,e.pawn,nullptr);
    }
    if(Read<void*>(e.item,0x4a8)||!original_effects::RetireActor(e.item)||!Event(e.item,L"/Script/Engine.Actor.K2_DestroyActor",0,nullptr))
        return Fail(e,5534,"server_armor_original_item_retirement_failed");
    ++state.destroyRequests;++state.retirements;e={};return true;
}
inline bool ObserveClientImpl(void* world,void* pc,void* ps,void* pawn,uint32_t epoch){
    const auto& actual=local_server_client::GetState();
    if(!actual.gameThread||GetCurrentThreadId()!=actual.gameThread)return true;
    if(!state.gameThread)state.gameThread=actual.gameThread;
    auto& c=state.client;const auto samples=c.samples+1,faults=c.sampleFaults;c={};c.samples=samples;c.sampleFaults=faults;c.epoch=epoch;
    const auto pending=[&](const char* stage){c.readback=false;c.stage=stage;return true;};
    c.localPair=actual.initialized&&!actual.retired&&actual.nativeClient&&actual.localPairVerified
        &&actual.world==world&&actual.controller==pc&&actual.playerState==ps&&actual.pawn==pawn;
    if(!c.localPair||!world||!pc||!ps||!pawn)return pending("client_armor_waiting_verified_actual_local_pair");
    if(!Live(world,actual.worldId)||!Live(pc,actual.controllerId)||!Live(ps,actual.playerStateId)
       ||!Live(pawn,actual.pawnId)||!Live(actual.localPlayer,actual.localPlayerId))
        return pending("client_armor_invalid_local_pair_weak_identity");
    c.worldMatches=original_effects::GetWorld(pc)==world&&original_effects::GetWorld(ps)==world&&original_effects::GetWorld(pawn)==world;
    const auto driver=actual.driver,connection=actual.connection,localPlayer=actual.localPlayer;
    const auto tuple=[&](){return c.worldMatches&&Read<uint8_t>(pc,0x224)==2&&Read<uint8_t>(pawn,0x224)==2
        &&Read<void*>(pc,0x4b0)==pawn&&Read<void*>(pc,0x470)==ps&&Read<void*>(pawn,0x488)==ps&&Read<void*>(pawn,0x4a0)==pc
        &&Read<void*>(ps,0xbc8)==pawn&&Read<void*>(world,0x40)==driver&&Read<void*>(driver,0xc0)==connection
        &&Read<void*>(connection,0x38)==pc&&Read<void*>(connection,0xa0)==pc
        &&Read<void*>(pc,0x510)==localPlayer&&Read<void*>(localPlayer,0x38)==pc;};
    if(!tuple()||engine::function<int(*)(void*)>(0x3efc820)(pawn)!=3)return pending("client_armor_invalid_current_world_or_possession_tuple");
    c.observed=true;c.pawnId=actual.pawnId;
    auto inventory=Read<void*>(pawn,0xc08),slot=Read<void*>(inventory,0x1b8+ArmorSlot*8),item=Read<void*>(slot,0x38);
    auto damageable=Read<void*>(pawn,0xc70);
    if(!original_players::IsA(inventory,original_inventory_slots::InventoryClass)||Read<void*>(inventory,0xc8)!=pawn
       ||Read<void*>(inventory,0x2f0)!=pawn||!original_players::IsA(slot,original_inventory_slots::ItemSlotClass)
       ||original_players::IsA(slot,original_inventory_slots::MultiItemSlotClass)||Read<void*>(slot,0x40)!=inventory
       ||Read<uint8_t>(slot,0x30)!=ArmorSlot||!original_players::IsA(damageable,original_damage::DamageableClass)
       ||Read<void*>(damageable,0xc8)!=pawn)return pending("client_armor_waiting_original_inventory_and_damageable");
    if(!item)return pending("client_armor_waiting_original_inventory_replica");
    c.itemPresent=true;c.itemRole=Read<uint8_t>(item,0x224);
    const auto heavy=engine::Find(original_armor::HeavyClassPath);c.heavyClass=heavy&&Read<void*>(item,0x10)==heavy;
    if(!c.heavyClass||!original_players::IsA(item,original_armor::ItemClass)||!Capture(item,c.itemId))
        return pending("client_armor_invalid_received_item_type_or_weak");
    c.itemOwned=Read<void*>(item,0x210)==pawn&&Read<void*>(item,0x4a8)==pawn;
    c.slotMatches=Read<void*>(slot,0x38)==item&&Read<uint8_t>(item,0x4a0)==ArmorSlot;
    struct Query {void* type;void* result=nullptr;} q{engine::Class(original_armor::AttachedSectionClass)};
    if(!Event(item,L"/Script/Engine.Actor.GetComponentByClass",16,&q)||!original_players::IsA(q.result,original_armor::AttachedSectionClass))
        return pending("client_armor_waiting_original_scs_section_replica");
    c.sectionPresent=true;if(!Capture(q.result,c.sectionId))return pending("client_armor_invalid_section_weak_identity");
    std::array<void*,12> objects{world,pc,ps,pawn,inventory,slot,item,q.result,damageable,driver,connection,localPlayer};
    std::array<Weak,12> identities{};
    for(size_t n=0;n<objects.size();++n)if(!Capture(objects[n],identities[n]))return pending("client_armor_invalid_sample_object_weak_identity");
    c.sectionAttached=Registered(damageable,q.result);c.sectionReplicated=(Read<uint8_t>(q.result,0xa0)&0x20)!=0;
    c.flagsValid=Read<void*>(q.result,0xc8)==item&&Read<uint8_t>(q.result,0x118)==1
        &&Read<uint8_t>(q.result,0x119)==0&&Read<uint8_t>(q.result,0x149)==1&&Read<float>(q.result,0x144)==1;
    original_damage::SectionAudit audit;
    if(!original_damage::DescribeSections(pawn,damageable,audit))return pending("client_armor_invalid_native_section_audit");
    unsigned owned=0;for(int32_t n=0;n<audit.count;++n){const auto& r=audit.entries[n];if(r.type==1&&!r.emptyShield&&r.object==q.result)++owned;}
    c.effectiveShields=audit.effectiveShields;
    c.life=Read<float>(q.result,0x124);c.maximum=Read<float>(q.result,0x128);
    if(!Event(pawn,L"/Script/ShooterGame.ShooterCharacter.GetShield",4,&c.shield)
       ||!Event(pawn,L"/Script/ShooterGame.ShooterCharacter.GetMaxShield",4,&c.maxShield)
       ||!Event(pawn,L"/Script/ShooterGame.ShooterCharacter.GetRespawnNumber",4,&c.respawn)
       ||!Event(damageable,L"/Script/ShooterGame.DamageableComponent.GetRawShield",4,&c.rawShield))
        return pending("client_armor_invalid_native_getter_sample");
    c.weakStable=true;for(size_t n=0;n<objects.size();++n)c.weakStable=c.weakStable&&Live(objects[n],identities[n]);
    if(!c.weakStable)return pending("client_armor_sample_weak_changed_during_native_getters");
    const auto& after=local_server_client::GetState();
    c.worldMatches=original_effects::GetWorld(pc)==world&&original_effects::GetWorld(ps)==world&&original_effects::GetWorld(pawn)==world;
    c.tupleStable=after.initialized&&!after.retired&&after.nativeClient&&after.localPairVerified
        &&after.world==world&&after.controller==pc&&after.playerState==ps&&after.pawn==pawn&&tuple()
        &&after.driver==driver&&after.connection==connection&&after.localPlayer==localPlayer
        &&original_inventory_slots::SameIdentity(after.pawnId,c.pawnId)
        &&Read<void*>(pawn,0xc08)==inventory&&Read<void*>(slot,0x38)==item&&Read<void*>(slot,0x40)==inventory
        &&Read<void*>(item,0x210)==pawn&&Read<void*>(item,0x4a8)==pawn&&Read<void*>(q.result,0xc8)==item&&Registered(damageable,q.result);
    c.readback=c.tupleStable&&c.weakStable&&c.heavyClass&&c.itemOwned&&c.slotMatches&&c.sectionAttached&&c.sectionReplicated&&c.flagsValid
        &&owned==1&&c.effectiveShields==1&&c.maximum==HeavyMaximum&&c.maxShield==HeavyMaximum
        &&std::isfinite(c.life)&&std::isfinite(c.maximum)&&std::isfinite(c.shield)&&std::isfinite(c.maxShield)&&std::isfinite(c.rawShield)
        &&c.life>=0&&c.life<=HeavyMaximum&&c.shield>=0&&c.shield<=HeavyMaximum&&c.rawShield>=0&&c.rawShield<=HeavyMaximum
        &&std::abs(c.rawShield-c.life)<.01f;
    c.stage=c.readback?"client_armor_stable_original_heavy_replica_readback":"client_armor_original_replica_pending_native_readback";return true;
}
// Optional source of existing actors for Root's deduplicating visitor. The
// normal CollectGraph/AddEquipmentActor path already sees published slot 11.
// This creates no ActorChannel, subobject replicator, GUID or component clone.
template<class Visit> inline bool VisitActors(Visit&& visit){
    if(!Context())return false;
    for(const auto& e:entries)if(e.used&&!e.retired&&e.published&&e.owned&&e.replicated){
        const auto p=local_server_equipment::State(e.index);if(p&&p->retiring)continue;
        if(!Source(e)||!Live(e.item,e.itemId)||Read<void*>(e.item,0x210)!=e.pawn||Read<void*>(e.item,0x4a8)!=e.pawn)return false;
        visit(e.item);
    }
    return true;
}
inline std::string Number(float value){
    if(!std::isfinite(value))return "null";char text[64]{};std::snprintf(text,sizeof(text),"%.9g",static_cast<double>(value));return text;
}
inline void CacheJson(){
    if(state.gameThread&&GetCurrentThreadId()!=state.gameThread)return;
    armor_poll_performance::Scope measurementCall(armor_poll_performance::Slot::JsonCall);
    armor_poll_performance::Scope measurement(armor_poll_performance::Slot::JsonBuild);
    const auto b=[](bool v){return v?"true":"false";};const auto u=[](uint64_t v){return static_cast<unsigned long long>(v);};
    char text[2048]{};std::string value;
    std::snprintf(text,sizeof(text),"{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"restart_required\":%s,\"grants\":%llu,\"registrations\":%llu,\"retirements\":%llu,\"expired_private_records\":%llu,\"destroy_requests\":%llu,\"already_destroying\":%llu,\"section_replication_enables\":%llu,\"native_shield_life_writes\":0,\"client_owner_events_forced\":0,\"players\":[",
        state.stage,state.error,b(state.initialized),b(state.restartRequired),u(state.grants),u(state.registrations),u(state.retirements),u(state.expiredRecords),u(state.destroyRequests),u(state.alreadyDestroying),u(state.componentReplicationEnables));value=text;
    bool comma=false;for(const auto& e:entries)if(e.used){
        std::snprintf(text,sizeof(text),"%s{\"index\":%zu,\"epoch\":%u,\"stage\":\"%s\",\"error\":%u,\"ready\":%s,\"spawned\":%s,\"slot_published\":%s,\"auth_attached\":%s,\"readback\":%s,\"pawn_weak\":[%d,%d],\"item_weak\":[%d,%d],\"section_weak\":[%d,%d],\"life\":%s,\"maximum\":%s,\"shield\":%s,\"max_shield\":%s,\"raw_shield\":%s,\"damage_scale\":%s,\"can_be_healed\":%s,\"can_be_destroyed\":%s,\"section_replicated\":%s,\"effective_shields\":%u}",
            comma?",":"",e.index,e.epoch,e.stage,e.error,b(e.ready),b(e.spawned),b(e.published),b(e.authAttached),b(e.readback),e.pawnId.index,e.pawnId.serial,e.itemId.index,e.itemId.serial,e.sectionId.index,e.sectionId.serial,
            Number(e.life).c_str(),Number(e.maximum).c_str(),Number(e.shield).c_str(),Number(e.maxShield).c_str(),Number(e.rawShield).c_str(),Number(e.damageScale).c_str(),b(e.canBeHealed),b(e.canBeDestroyed),b(e.sectionReplicated),e.effectiveShields);value+=text;comma=true;
    }
    const auto& c=state.client;
    std::snprintf(text,sizeof(text),"],\"client\":{\"stage\":\"%s\",\"observed\":%s,\"epoch_counter\":%u,\"native_respawn\":%d,\"item_present\":%s,\"item_role\":%u,\"item_owned\":%s,\"slot_matches\":%s,\"section_attached\":%s,\"section_replicated\":%s,\"heavy_class\":%s,\"readback\":%s,\"local_pair\":%s,\"world_matches\":%s,\"tuple_stable\":%s,\"weak_stable\":%s,\"flags_valid\":%s,\"effective_shields\":%u,\"sample_faults\":%llu,\"exception_rva\":%llu,\"pawn_weak\":[%d,%d],\"item_weak\":[%d,%d],\"section_weak\":[%d,%d],\"life\":%s,\"maximum\":%s,\"shield\":%s,\"max_shield\":%s,\"raw_shield\":%s}}",
        c.stage,b(c.observed),c.epoch,c.respawn,b(c.itemPresent),static_cast<unsigned>(c.itemRole),b(c.itemOwned),b(c.slotMatches),b(c.sectionAttached),b(c.sectionReplicated),b(c.heavyClass),b(c.readback),b(c.localPair),b(c.worldMatches),b(c.tupleStable),b(c.weakStable),b(c.flagsValid),c.effectiveShields,u(c.sampleFaults),u(c.exceptionRva),
        c.pawnId.index,c.pawnId.serial,c.itemId.index,c.itemId.serial,c.sectionId.index,c.sectionId.serial,Number(c.life).c_str(),Number(c.maximum).c_str(),Number(c.shield).c_str(),Number(c.maxShield).c_str(),Number(c.rawShield).c_str());value+=text;
    struct Lock {Lock(){AcquireSRWLockExclusive(&jsonLock);}~Lock(){ReleaseSRWLockExclusive(&jsonLock);}} lock;jsonSnapshot=std::move(value);
}
inline std::string Json(){struct Lock{Lock(){AcquireSRWLockShared(&jsonLock);}~Lock(){ReleaseSRWLockShared(&jsonLock);}} lock;return jsonSnapshot;}
inline LONG CaptureException(EXCEPTION_POINTERS* info){
    auto r=info?info->ExceptionRecord:nullptr;const auto at=r?reinterpret_cast<uintptr_t>(r->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;state.restartRequired=true;
    Fail(r?r->ExceptionCode:0xe0005535U,"server_armor_native_exception_requires_new_session");return EXCEPTION_EXECUTE_HANDLER;
}
inline LONG CaptureObservationException(EXCEPTION_POINTERS* info){
    const auto r=info?info->ExceptionRecord:nullptr;const auto at=r?reinterpret_cast<uintptr_t>(r->ExceptionAddress):0;
    auto& c=state.client;c.observed=false;c.readback=false;c.tupleStable=false;c.weakStable=false;++c.sampleFaults;
    c.exceptionRva=at>=engine::image?at-engine::image:at;c.stage="client_armor_invalid_readonly_sample_exception";
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* world){__try{const bool okay=InitializeImpl(world);CacheJson();return okay;}__except(CaptureException(GetExceptionInformation())){CacheJson();return false;}}
inline bool RegisterOrEnsure(size_t index,void* pc,void* ps,void* pawn,uint32_t epoch){__try{const bool okay=RegisterOrEnsureImpl(index,pc,ps,pawn,epoch);CacheJson();return okay;}__except(CaptureException(GetExceptionInformation())){CacheJson();return false;}}
inline bool Poll(){__try{const bool okay=PollImpl();CacheJson();return okay;}__except(CaptureException(GetExceptionInformation())){CacheJson();return false;}}
inline bool BeforeRetire(size_t index,void* pawn,uint32_t epoch){__try{const bool okay=BeforeRetireImpl(index,pawn,epoch);CacheJson();return okay;}__except(CaptureException(GetExceptionInformation())){CacheJson();return false;}}
inline bool ObserveClient(void* world,void* pc,void* ps,void* pawn,uint32_t epoch){__try{ObserveClientImpl(world,pc,ps,pawn,epoch);CacheJson();return true;}__except(CaptureObservationException(GetExceptionInformation())){CacheJson();return true;}}
inline bool WriteDiagnostics(const wchar_t* path){
    FILE* file=nullptr;if(!path||_wfopen_s(&file,path,L"wb")||!file)return false;const auto value=Json();
    const bool okay=std::fwrite(value.data(),1,value.size(),file)==value.size()&&std::fputc('\n',file)!=EOF&&std::ferror(file)==0;std::fclose(file);return okay;
}
} // namespace local_server_armor
