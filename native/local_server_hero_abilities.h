#pragma once
#include "local_server_starting_equipment.h"
#include "local_server_resource_authority.h"
#include "original_heroes_catalog.h"
#include "original_effects.h"
#include <array>
#include <cstdio>

// Missing local HOST loadout assembly, selected by the real pawn CDO rather
// than Wushu class names or four assumed key slots. Native state machines,
// input, resources and prediction remain original. Receivers never construct
// an item or write a replicated slot. Assembly readiness is deliberately not
// an assertion that hero-specific server effects or execution are verified.
// File provenance: work/local-server-playable/hero-abilities/group*.json.
namespace local_server_hero_abilities {
namespace loadout=local_server_starting_equipment;
using Weak=original_inventory_slots::WeakIdentity;
using Array=loadout::Array<void*>;
inline constexpr size_t MaxPlayers=8,MaxItems=64;
enum class Status:uint8_t {NotRegistered,Waiting,AssemblyReady,Retired,Unsupported,Failed};
struct Item {
    loadout::Starter native{};
    void *defaults=nullptr,*resource=nullptr,*tuning=nullptr;
    Weak defaultsId{},resourceId{},tuningId{};
    bool equipmentManaged=false,assembled=false,graphReady=false,resourcesReady=false;
    bool resourceSupported=false,resourcePrepared=false,commonEquipRegistered=false;
    bool componentsSampled=false,defaultGraphSampled=false,defaultGraphComplete=false,defaultConstructionMachines=false;
    bool defaultPrimaryMachine=false,graphAuthoredInactive=false;
    unsigned machines=0,enabledMachines=0,selectedMachines=0,resources=0,tunings=0,defaultMachines=0,defaultEnabledMachines=0;
    uint64_t transitions=0,resourceRegistration=0;
    const char* waiting="authored_item_not_sampled";
};
struct Player {
    bool used=false,retired=false,authority=false,catalogBound=false,getterVerified=false,slotsEnsured=false,assemblyReady=false;
    unsigned hero=0,error=0;uint32_t epoch=0;Status status=Status::NotRegistered;
    void *heroClass=nullptr,*heroDefaults=nullptr;
    Weak heroClassId{},heroDefaultsId{};
    // The existing helpers accept this real source tuple. They do not depend
    // on the Wushu-only registration/array indexing in their original module.
    loadout::Player context{};
    size_t count=0,assembledItems=0,readyGraphs=0,readyResources=0;
    std::array<Item,MaxItems> items{};
    uint64_t polls=0,lastSample=0,registration=0;
    const char* stage="hero_loadout_not_registered";
};
struct State {
    bool initialized=false,host=false,profilesVerified=false,schemaVerified=false,restartRequired=false;
    unsigned error=0,players=0,readyPlayers=0;DWORD gameThread=0;
    void* world=nullptr;Weak worldId{};
    uint64_t registrations=0,retirements=0,staleRetirements=0,polls=0,nonHumanUnchanged=0;
    uintptr_t exceptionRva=0;
    const char* stage="hero_loadout_not_initialized";
};
struct ItemDiagnostic {
    void *type=nullptr,*actor=nullptr,*slot=nullptr,*resource=nullptr,*tuning=nullptr;
    uint8_t slotType=255,role=255,remoteRole=255;
    bool equipmentManaged=false,multi=false,spawned=false,adopted=false,published=false;
    bool assembled=false,graphReady=false,resourcesReady=false,resourceSupported=false;
    bool commonEquipRegistered=false,beginPlay=false,ownerReady=false;
    bool defaultGraphSampled=false,defaultGraphComplete=false,defaultConstructionMachines=false,graphAuthoredInactive=false;
    unsigned machines=0,enabledMachines=0,selectedMachines=0,resources=0,tunings=0,defaultMachines=0,defaultEnabledMachines=0;
    uint64_t transitions=0;const char* waiting="authored_item_not_sampled";
};
struct PlayerDiagnostic {
    bool used=false,retired=false,authority=false,catalogBound=false,assemblyReady=false;
    unsigned hero=0,error=0;uint32_t epoch=0;Status status=Status::NotRegistered;
    void* pawn=nullptr;size_t count=0,assembledItems=0,readyGraphs=0,readyResources=0;
    const char* stage="hero_loadout_not_registered";
    std::array<ItemDiagnostic,MaxItems> items{};
};
struct Diagnostics {State summary{};std::array<PlayerDiagnostic,MaxPlayers> players{};uint64_t at=0;};
// Pure source proof for separate missing-host producers (child actors, game
// objects, subequippables). The producer owns its own generated-actor registry
// and cleanup; this record is not a replication or gameplay-success receipt.
struct SourceView {
    void *contextObject=nullptr,*world=nullptr,*controller=nullptr,*playerState=nullptr,*pawn=nullptr,*inventory=nullptr,*ability=nullptr;
    // Same order as the fields above, for explicit weak source ownership.
    std::array<Weak,7> identities{};
    unsigned heroIndex=0;uint32_t sourceEpoch=0;uint64_t registration=0;
    bool pendingGrant=false;
};
// Optional typed resource producer, installed after Init and before the first
// registered Pawn. Generic resource/tuning preparation still runs first. A
// producer may handle a separate native resource (such as UltPoints) without
// routing it through scalar +10c/+110 publication. No factory or graph hook.
struct ResourceExtensionResult {
    bool handled=false,supported=false,ready=false;unsigned error=0;
    const char* stage="hero_typed_resource_extension_unhandled";
};
using ResourceExtension=ResourceExtensionResult(*)(Player&,Item&);
inline State state{};
inline std::array<Player,MaxPlayers> players{};
inline Diagnostics cached{};
inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline void *startingGetter=nullptr;
inline ResourceExtension resourceExtension=nullptr;
inline const State& GetState(){return state;}
inline const std::array<Player,MaxPlayers>& GetPlayers(){return players;}
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Live(void* p,const Weak& id){return p&&original_inventory_slots::ResolveIdentity(id)==p;}
inline bool Capture(void* p,Weak& id){return original_inventory_slots::CaptureIdentity(p,id)&&Live(p,id);}
inline bool Thread(){return state.initialized&&!state.restartRequired&&original_players::Thread()
    &&GetCurrentThreadId()==state.gameThread;}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;return false;}
inline bool Fail(Player& p,unsigned error,const char* stage){p.error=error;p.status=Status::Failed;
    p.assemblyReady=false;p.stage=stage;return Fail(error,stage);}
inline bool Wait(Player& p,const char* stage){p.status=Status::Waiting;p.assemblyReady=false;p.stage=stage;state.stage=stage;return true;}
inline bool BindResourceExtension(ResourceExtension extension){
    if(!Thread()||state.error||!extension)return Fail(6153,"hero_resource_extension_requires_initialized_original_game_thread");
    if(resourceExtension==extension)return true;
    if(resourceExtension)return Fail(6154,"hero_resource_extension_already_bound");
    for(const auto& p:players)if(p.used&&!p.retired)return Fail(6155,"hero_resource_extension_requires_binding_before_player_registration");
    resourceExtension=extension;return true;
}
inline bool NativeResult(Player& p,bool result){
    if(result)return true;
    const auto& native=loadout::GetState();
    return Fail(p,p.context.error?p.context.error:(native.error?native.error:6100),
        p.context.error?p.context.stage:(native.error?native.stage:"hero_loadout_shared_native_helper_failed"));
}
inline void Cache(){
    AcquireSRWLockExclusive(&cacheLock);cached.summary=state;cached.at=GetTickCount64();
    for(size_t n=0;n<players.size();++n){const auto& p=players[n];auto& d=cached.players[n];d={};
        d.used=p.used;d.retired=p.retired;d.authority=p.authority;d.catalogBound=p.catalogBound;d.assemblyReady=p.assemblyReady;
        d.hero=p.hero;d.error=p.error;d.epoch=p.epoch;d.status=p.status;d.pawn=p.context.pawn;d.count=p.count;
        d.assembledItems=p.assembledItems;d.readyGraphs=p.readyGraphs;d.readyResources=p.readyResources;d.stage=p.stage;
        for(size_t i=0;i<p.count;++i){const auto& s=p.items[i];const auto& native=s.native;auto& out=d.items[i];
            out.type=native.type;out.actor=native.actor;out.slot=native.slot;out.resource=s.resource;out.tuning=s.tuning;
            out.slotType=native.slotType;out.role=native.role;out.remoteRole=native.remoteRole;out.multi=native.multi;
            out.equipmentManaged=s.equipmentManaged;out.spawned=native.spawned;out.adopted=native.adopted;out.published=native.published;
            out.assembled=s.assembled;out.graphReady=s.graphReady;out.resourcesReady=s.resourcesReady;out.resourceSupported=s.resourceSupported;
            out.commonEquipRegistered=s.commonEquipRegistered;out.beginPlay=native.beginPlay;out.ownerReady=native.ownerReady;
            out.machines=s.machines;out.enabledMachines=s.enabledMachines;out.selectedMachines=s.selectedMachines;
            out.resources=s.resources;out.tunings=s.tunings;out.transitions=s.transitions;out.waiting=s.waiting;
            out.defaultGraphSampled=s.defaultGraphSampled;out.graphAuthoredInactive=s.graphAuthoredInactive;
            out.defaultGraphComplete=s.defaultGraphComplete;out.defaultConstructionMachines=s.defaultConstructionMachines;
            out.defaultMachines=s.defaultMachines;out.defaultEnabledMachines=s.defaultEnabledMachines;}
    }
    ReleaseSRWLockExclusive(&cacheLock);
}
inline Diagnostics Snapshot(){AcquireSRWLockShared(&cacheLock);auto result=cached;ReleaseSRWLockShared(&cacheLock);return result;}
inline bool Profiles(){
    if(state.profilesVerified)return true;
    // Reuse the audited factory, reflection, slot and owner source profiles;
    // reuse their native helper implementations as well, not a second codec.
    if(!loadout::VerifyProfiles()||!loadout::VerifySchema())
        return Fail(loadout::GetState().error,loadout::GetState().stage);
    auto code=reinterpret_cast<const uint8_t*>(engine::image+0x6c63b80);
    if(!original_players::Readable(code,141))return Fail(6101,"hero_loadout_native_starting_getter_unreadable");
    uint64_t digest=0xcbf29ce484222325ULL;for(size_t i=0;i<141;++i)digest=(digest^code[i])*0x100000001b3ULL;
    if(digest!=0xe6c9d6fd342ad39dULL)return Fail(6102,"hero_loadout_native_starting_getter_profile_mismatch");
    startingGetter=engine::Find(L"/Script/ShooterGame.ShooterCharacter.GetStartingEquippableClasses");
    if(!startingGetter||Read<uint16_t>(startingGetter,0xbe)!=16
       ||Read<uintptr_t>(startingGetter,0xe0)!=engine::image+0x6c63b80
       ||Read<void*>(startingGetter,0x28)!=engine::Class(original_players::ShooterCharacterClass))
        return Fail(6103,"hero_loadout_native_starting_getter_metadata_mismatch");
    // CDO component queries do not necessarily contain BP SCS templates.
    // These real reflected fields are used only to establish whether an
    // empty/inactive default-graph observation is complete enough to accept.
    if(!loadout::Field(engine::Class(0x3bbe3a0),L"SimpleConstructionScript",0x278,8,1,0)
       ||!loadout::Field(engine::Class(0x4a46b10),L"AllNodes",0x40,16,1,0)
       ||!loadout::Field(engine::Class(0x4a46bc0),L"ComponentClass",0x30,8,1,0)
       ||!loadout::Field(engine::Class(0x4a46bc0),L"ComponentTemplate",0x38,8,1,0))
        return Fail(6152,"hero_loadout_original_construction_graph_metadata_mismatch");
    state.profilesVerified=state.schemaVerified=true;return true;
}
inline bool Source(Player& p){
    auto& c=p.context;c.sourceReady=false;
    if(!Live(c.world,c.worldId)||!Live(c.controller,c.controllerId)||!Live(c.playerState,c.playerStateId)
       ||!Live(c.pawn,c.pawnId)||!Live(c.inventory,c.inventoryId))return Fail(p,6104,"hero_loadout_source_identity_retired_without_notification");
    if(Read<void*>(c.controller,0x470)!=c.playerState||Read<void*>(c.controller,0x4b0)!=c.pawn
       ||Read<void*>(c.pawn,0x488)!=c.playerState||Read<void*>(c.pawn,0x4a0)!=c.controller
       ||Read<void*>(c.pawn,0xc08)!=c.inventory||Read<void*>(c.inventory,0xc8)!=c.pawn
       ||Read<void*>(c.inventory,0x2f0)!=c.pawn)return Fail(p,6105,"hero_loadout_actual_possession_or_inventory_changed");
    if(p.authority){
        void* connection=nullptr;
        if(!local_server_item_ownership::ActualHuman(c.world,c.controller,c.playerState,c.pawn,c.inventory,connection))
            return Wait(p,"hero_loadout_waiting_actual_authority_human_connection");
    }else if(Read<uint8_t>(c.pawn,0x224)!=2||Read<uint8_t>(c.controller,0x224)!=2
        ||original_players::activePawn!=c.pawn||original_players::activeController!=c.controller
        ||!original_players::localPlayer||Read<void*>(c.controller,0x510)!=original_players::localPlayer
        ||Read<void*>(original_players::localPlayer,0x38)!=c.controller)
        return Wait(p,"hero_loadout_waiting_actual_owned_role2_client");
    c.sourceReady=true;return true;
}
inline bool SourceViewImpl(void* object,SourceView& out){
    out={};if(!Thread()||state.error||!state.host||!object||!Live(state.world,state.worldId))return false;
    Weak contextId{};if(!Capture(object,contextId))return false;
    for(const auto& p:players){const auto& c=p.context;
        if(!p.used||p.retired||!p.authority||!p.catalogBound||p.error||!p.registration||!p.epoch||c.epoch!=p.epoch
           ||!Live(c.world,c.worldId)||!Live(c.controller,c.controllerId)||!Live(c.playerState,c.playerStateId)
           ||!Live(c.pawn,c.pawnId)||!Live(c.inventory,c.inventoryId)
           ||!Live(p.heroClass,p.heroClassId)||!Live(p.heroDefaults,p.heroDefaultsId))continue;
        void* connection=nullptr;
        if(c.world!=state.world||!local_server_item_ownership::ActualHuman(c.world,c.controller,c.playerState,c.pawn,c.inventory,connection)
           ||Read<void*>(c.inventory,0x2f0)!=c.pawn
           ||!original_players::Derived(Read<void*>(c.pawn,0x10),p.heroClass))continue;
        void* ability=nullptr;Weak abilityId{};bool pending=false,matched=object==c.pawn;
        for(size_t i=0;!matched&&i<p.count;++i){const auto& s=p.items[i];const auto& n=s.native;
            if(s.equipmentManaged||!Live(n.actor,n.actorId)||!Live(n.type,n.typeId)
               ||n.slotType>=17||!Live(n.slot,n.slotId)
               ||Read<void*>(c.inventory,0x1b8+size_t(n.slotType)*8)!=n.slot
               ||!local_server_item_ownership::ValidSlot(c.inventory,n.slot,n.slotType)
               ||!original_players::Derived(Read<void*>(n.actor,0x10),n.type)
               ||Read<void*>(n.actor,0x210)!=c.pawn||Read<uint8_t>(n.actor,0x224)!=3
               ||local_server_item_ownership::ActorWorld(n.actor)!=c.world)continue;
            const bool direct=object==n.actor;
            const bool component=original_players::IsA(object,0x36837a0)&&Read<void*>(object,0xc8)==n.actor;
            if(!direct&&!component)continue;
            // Pending is a captured *real* original deferred factory result,
            // already owned by this Pawn; it is useful inside native BeginPlay.
            // Never admit a foreign actor merely because its Owner points here.
            pending=n.spawned&&!n.published;
            if(!pending&&(!n.published||!loadout::Contains(n)||Read<void*>(n.actor,0x4a8)!=c.pawn))continue;
            matched=true;ability=n.actor;abilityId=n.actorId;
        }
        if(!matched)continue;
        out.contextObject=object;out.world=c.world;out.controller=c.controller;out.playerState=c.playerState;
        out.pawn=c.pawn;out.inventory=c.inventory;out.ability=ability;out.heroIndex=p.hero;out.sourceEpoch=p.epoch;
        out.registration=p.registration;out.pendingGrant=pending;
        out.identities={contextId,c.worldId,c.controllerId,c.playerStateId,c.pawnId,c.inventoryId,abilityId};
        return true;
    }
    return false;
}
inline bool ResolveSource(void* contextObject,SourceView& out){return SourceViewImpl(contextObject,out);}
inline bool ValidateSource(const SourceView& before){
    SourceView after{};
    if(!ResolveSource(before.contextObject,after)||before.world!=after.world||before.controller!=after.controller
       ||before.playerState!=after.playerState||before.pawn!=after.pawn||before.inventory!=after.inventory
       ||before.ability!=after.ability||before.heroIndex!=after.heroIndex||before.sourceEpoch!=after.sourceEpoch
       ||before.registration!=after.registration)return false;
    for(size_t i=0;i<before.identities.size();++i){
        // A Pawn-context call has no ability identity; its invalid empty Weak
        // is preserved rather than manufactured to satisfy SameIdentity.
        if(i==6&&!before.ability)continue;
        if(!original_inventory_slots::SameIdentity(before.identities[i],after.identities[i]))return false;}
    return true;
}
inline bool Authored(Player& p){
    const bool previouslyBound=p.count>0;p.catalogBound=false;
    const auto actualType=Read<void*>(p.context.pawn,0x10);
    if(!Live(p.heroClass,p.heroClassId)||!Live(p.heroDefaults,p.heroDefaultsId)
       ||!original_players::Derived(actualType,p.heroClass))return Fail(p,6106,"hero_loadout_original_hero_class_identity_changed");
    const auto authored=Read<Array>(p.heroDefaults,0xbd8),actual=Read<Array>(p.context.pawn,0xbd8);
    if(!loadout::ValidArray(authored,static_cast<int32_t>(MaxItems))||!loadout::ValidArray(actual,static_cast<int32_t>(MaxItems))
       ||!authored.count||actual.count!=authored.count)return Fail(p,6107,"hero_loadout_actual_starting_array_differs_from_original_cdo");
    if(previouslyBound&&p.count!=static_cast<size_t>(authored.count))return Fail(p,6108,"hero_loadout_authored_count_changed_within_epoch");
    for(int32_t i=0;i<authored.count;++i){const auto type=authored.data[i];auto& s=p.items[static_cast<size_t>(i)];
        if(actual.data[i]!=type||!original_players::IsA(type,0x1af0eb0)
           ||!original_players::Derived(type,engine::Class(0x645c660)))
            return Fail(p,6109,"hero_loadout_authored_class_is_not_original_equippable");
        if(s.native.type&&(s.native.type!=type||!Live(type,s.native.typeId)))
            return Fail(p,6110,"hero_loadout_authored_type_changed_within_epoch");
        auto defaults=Read<void*>(type,0x120);
        if(!defaults||!original_players::IsA(defaults,0x645c660))return Wait(p,"hero_loadout_waiting_original_equippable_cdo");
        if(s.defaults&&(s.defaults!=defaults||!Live(defaults,s.defaultsId)))return Fail(p,6111,"hero_loadout_item_cdo_changed_within_epoch");
        if(!s.native.type){
            s.native.type=type;s.defaults=defaults;
            if(!Capture(type,s.native.typeId)||!Capture(defaults,s.defaultsId))return Fail(p,6112,"hero_loadout_cdo_weak_identity_unavailable");
            s.native.slotType=Read<uint8_t>(defaults,0x4a0);
            if(s.native.slotType>=17)return Fail(p,6113,"hero_loadout_original_item_slot_outside_native_enum");
            // Existing equipment owns the ordinary primary/secondary/melee.
            // A hero weapon in an ability or invisible slot is still a real
            // authored skill item and is not excluded merely for being a gun.
            s.equipmentManaged=s.native.slotType<=2;
        }
        if(previouslyBound&&s.native.slotType!=Read<uint8_t>(defaults,0x4a0))return Fail(p,6114,"hero_loadout_original_item_cdo_slot_changed");
    }
    p.count=static_cast<size_t>(authored.count);p.catalogBound=true;return true;
}
inline bool NativeStartingGetter(Player& p){
    // Cross-check the actual original API once at registration. Its returned
    // array is an engine allocation, independently owned from both real CDOs.
    Array copied{};auto dispatch=Read<void*>(Read<void*>(p.context.pawn),0x298);
    if(!dispatch)return Fail(p,6115,"hero_loadout_original_starting_getter_dispatch_missing");
    reinterpret_cast<void(*)(void*,void*,void*)>(dispatch)(p.context.pawn,startingGetter,&copied);
    struct Allocation{void* data;~Allocation(){if(data)engine::function<void(*)(void*)>(0x1915720)(data);}} allocation{copied.data};
    const auto actual=Read<Array>(p.context.pawn,0xbd8);
    if(!loadout::ValidArray(copied,static_cast<int32_t>(MaxItems))||copied.count!=actual.count)
        return Fail(p,6116,"hero_loadout_original_getter_and_native_property_disagree");
    for(int32_t i=0;i<copied.count;++i)if(copied.data[i]!=actual.data[i])
        return Fail(p,6116,"hero_loadout_original_getter_and_native_property_disagree");
    p.getterVerified=true;return true;
}
inline bool Slot(Player& p,Item& s){
    auto& n=s.native;auto slot=Read<void*>(p.context.inventory,0x1b8+size_t(n.slotType)*8);
    if(!slot)return n.slot?Fail(p,6118,"hero_loadout_original_slot_detached_within_epoch"):true;
    if(!local_server_item_ownership::ValidSlot(p.context.inventory,slot,n.slotType))
        return Fail(p,6117,"hero_loadout_original_slot_schema_or_backref_invalid");
    if(n.slot&&(n.slot!=slot||!Live(slot,n.slotId)))return Fail(p,6118,"hero_loadout_original_slot_changed_within_epoch");
    if(!n.slot){n.slot=slot;if(!Capture(slot,n.slotId))return Fail(p,6119,"hero_loadout_original_slot_identity_unavailable");}
    n.multi=original_players::IsA(slot,0x69cf3d0);return true;
}
inline bool Adopt(Player& p,Item& s){
    auto& n=s.native;void* found=nullptr;
    if(n.multi){const auto values=Read<Array>(n.slot,0x50);
        if(!loadout::ValidArray(values,256))return Fail(p,6120,"hero_loadout_original_multi_contents_invalid");
        for(int32_t i=0;i<values.count;++i){auto item=values.data[i];
            if(!item||!original_players::Derived(Read<void*>(item,0x10),n.type))continue;
            if(found&&found!=item)return Fail(p,6121,"hero_loadout_duplicate_original_class_in_multi_slot");found=item;}
    }else found=Read<void*>(n.slot,0x38);
    if(s.equipmentManaged){
        // Equipment is allowed to change an ordinary weapon while this pawn
        // epoch stays alive. Only cache its current, real slot receipt here.
        n.actor=found;n.adopted=found!=nullptr;n.published=found!=nullptr;
        if(found&&(!original_players::IsA(found,0x645c660)||!Capture(found,n.actorId)))
            return Fail(p,6124,"hero_loadout_equipment_managed_item_identity_invalid");
        return true;
    }
    if(found&&!original_players::Derived(Read<void*>(found,0x10),n.type)){
        // A live ordinary weapon selection can legitimately replace an old
        // starting weapon. This module never takes ownership of those slots.
        return Fail(p,6122,"hero_loadout_original_single_slot_has_different_item");
    }
    if(n.actor&&found&&n.actor!=found)return Fail(p,6123,"hero_loadout_actor_replacement_requires_epoch_retirement");
    if(found&&!n.actor){n.actor=found;n.adopted=true;n.published=true;
        if(!Capture(found,n.actorId))return Fail(p,6124,"hero_loadout_original_item_weak_identity_unavailable");}
    return true;
}
inline bool ClientOwner(Player& p,Item& s){
    auto& n=s.native;n.ownerReady=false;n.role=Read<uint8_t>(n.actor,0x224);n.remoteRole=Read<uint8_t>(n.actor,0x128);
    if(n.role!=1&&n.role!=2)return Fail(p,6125,"hero_loadout_receiving_client_item_has_invalid_role");
    if(Read<void*>(n.actor,0x210)!=p.context.pawn)return true;
    if(Read<void*>(n.actor,0x4a8)!=p.context.pawn){auto previous=Read<void*>(n.actor,0x4a8);
        auto callback=Read<void*>(Read<void*>(n.actor),0x7d8);
        if(previous||callback!=reinterpret_cast<void*>(engine::image+0x70ec480))return Fail(p,6126,"hero_loadout_native_receiving_owner_callback_not_profiled");
        // Original Actor.Owner already replicated. Complete its existing typed
        // AresItem callback, as the original Wushu and equipment adapters do.
        reinterpret_cast<void(*)(void*,void*,void*)>(callback)(n.actor,nullptr,p.context.pawn);++n.ownerCallbacks;
    }
    n.ownerReady=Read<void*>(n.actor,0x4a8)==p.context.pawn;return true;
}
inline bool CommonEquip(Player& p,Item& s){
    s.commonEquipRegistered=false;auto& n=s.native;auto component=Read<void*>(n.actor,0xe18);auto registry=Read<void*>(component,0x198);
    auto type=engine::Class(0x6767a30);
    if(!original_players::IsA(component,0x6768f30)||Read<void*>(component,0xc8)!=n.actor
       ||!Read<uint8_t>(component,0x189)||!registry||!type
       ||engine::function<void*(*)(void*)>(0x6ff6000)(n.actor)!=registry){s.waiting="original_equip_event_registry_not_ready";return true;}
    Weak componentId{},registryId{};
    if(!Capture(component,componentId)||!Capture(registry,registryId)){s.waiting="original_equip_event_registry_identity_not_ready";return true;}
    if(!engine::function<bool(*)(void*,void*)>(0x6ff68d0)(registry,type)){
        // Retained ServerOnly registration is stripped. Register only the
        // actual common equip type through its existing mode0 implementation.
        // Hero-specific event classes are not guessed from Jett's loadout.
        engine::function<void(*)(void*,void*,uint8_t)>(0x6ff5f40)(component,type,0);++n.eventRegistrations;
    }
    s.commonEquipRegistered=Live(component,componentId)&&Live(registry,registryId)
        &&Read<void*>(n.actor,0xe18)==component&&Read<void*>(component,0x198)==registry
        &&engine::function<bool(*)(void*,void*)>(0x6ff68d0)(registry,type);
    return s.commonEquipRegistered||Fail(p,6127,"hero_loadout_original_common_equip_registration_failed");
}
inline bool Graph(Player& p,Item& s){
    auto& n=s.native;s.graphReady=s.graphAuthoredInactive=false;s.machines=s.enabledMachines=s.selectedMachines=0;
    if(!s.defaultGraphSampled){
        struct Query {void* type;void** data=nullptr;int32_t count=0,capacity=0;} q{engine::Class(0x36837a0)};
        if(!Live(s.defaults,s.defaultsId)||!loadout::Event(s.defaults,L"/Script/Engine.Actor.K2_GetComponentsByClass",24,&q))
            return Fail(p,6148,"hero_loadout_original_default_component_graph_query_failed");
        struct Allocation{void* data;~Allocation(){if(data)engine::function<void(*)(void*)>(0x1915720)(data);}} allocation{q.data};
        if(q.count<0||q.count>static_cast<int32_t>(loadout::MaxComponents)||q.capacity<q.count
           ||(q.count&&!original_players::Readable(q.data,size_t(q.count)*8)))
            return Fail(p,6149,"hero_loadout_original_default_component_graph_array_invalid");
        for(int32_t i=0;i<q.count;++i){auto component=q.data[i];
            if(!original_players::IsA(component,0x36837a0)||Read<void*>(component,0xc8)!=s.defaults)
                return Fail(p,6150,"hero_loadout_original_default_component_graph_owner_invalid");
            if(original_players::IsA(component,0x67722c0)){++s.defaultMachines;if(Read<uint8_t>(component,0x348))++s.defaultEnabledMachines;}
        }
        // A Blueprint may expose its exact primary template through the CDO
        // even when a class-default component query omits SCS templates. Such
        // a CDO is not evidence for a zero-machine/inactive graph exemption.
        s.defaultPrimaryMachine=Read<void*>(s.defaults,0xe00)!=nullptr;s.defaultGraphSampled=true;
        s.defaultGraphComplete=true;void* type=n.type;unsigned depth=0;
        for(;type&&depth<64;++depth){
            if(!original_players::IsA(type,0x1af0eb0)){s.defaultGraphComplete=false;break;}
            if(original_players::IsA(type,0x3bbe3a0)){auto script=Read<void*>(type,0x278);
                if(script){
                    if(!original_players::IsA(script,0x4a46b10)){s.defaultGraphComplete=false;break;}
                    const auto nodes=Read<Array>(script,0x40);
                    if(!loadout::ValidArray(nodes,4096)){s.defaultGraphComplete=false;break;}
                    for(int32_t j=0;j<nodes.count;++j){auto node=nodes.data[j];
                        if(!original_players::IsA(node,0x4a46bc0)){s.defaultGraphComplete=false;break;}
                        auto componentClass=Read<void*>(node,0x30),component=Read<void*>(node,0x38);
                        if(!componentClass&&component)componentClass=Read<void*>(component,0x10);
                        if(!componentClass||!original_players::Derived(componentClass,engine::Class(0x36837a0))){s.defaultGraphComplete=false;break;}
                        if(original_players::Derived(componentClass,engine::Class(0x67722c0))){
                            // A construction-template state machine can have
                            // inherited overrides absent from the CDO-owned
                            // array. Defer an inactive-graph exemption until
                            // that class-specific construction is corroborated.
                            s.defaultConstructionMachines=true;s.defaultGraphComplete=false;
                        }
                    }
                }
            }
            auto parent=Read<void*>(type,0x48);if(parent==type){s.defaultGraphComplete=false;break;}type=parent;
        }
        if(type&&depth==64)s.defaultGraphComplete=false;
    }
    if(!NativeResult(p,loadout::Components(p.context,n)))return false;
    s.componentsSampled=true;auto previous=n.selectedState;n.selectedState=nullptr;
    for(size_t i=0;i<n.componentCount;++i){auto machine=n.components[i];
        if(!original_players::IsA(machine,0x67722c0))continue;++s.machines;
        if(Read<void*>(machine,0xc8)!=n.actor)return Fail(p,6128,"hero_loadout_original_machine_owner_invalid");
        if(!Read<uint8_t>(machine,0x348))continue;++s.enabledMachines;
        void* current=Read<void*>(machine,p.authority?0x1c8:0x1b0);
        if(!current&&p.authority&&!Read<void*>(machine,0x1b0)&&!Read<void*>(machine,0x1c8)){
            auto starting=Read<void*>(machine,0x1e8);
            if(!starting||Read<void*>(starting,0xc8)!=n.actor||Read<void*>(starting,0xe0)!=machine)
                return Fail(p,6129,"hero_loadout_original_authored_starting_state_owner_invalid");
            if(!loadout::Event(machine,L"/Script/ShooterGame.StateMachineComponent.InitializeStartingState",8,&n.actor))
                return Fail(p,6130,"hero_loadout_original_host_starting_initializer_failed");
            ++n.startingInitializations;current=Read<void*>(machine,0x1c8);
        }
        // No receiving-client state pointer or initialization write. Native
        // RoleChanged and prediction consume the actual replicated ownership.
        if(!current)continue;Weak id{};
        if(!Capture(current,id)||Read<void*>(current,0xc8)!=n.actor||Read<void*>(current,0xe0)!=machine)
            return Fail(p,6131,"hero_loadout_original_selected_state_owner_invalid");
        ++s.selectedMachines;if(machine==Read<void*>(n.actor,0xe00))n.selectedState=current;
    }
    if(previous&&n.selectedState!=previous)++s.transitions;
    if(!s.enabledMachines){
        const bool noMachines=s.defaultGraphComplete&&!s.machines&&!s.defaultMachines&&!s.defaultPrimaryMachine;
        const bool authoredDisabled=s.defaultGraphComplete&&s.machines>0&&s.machines==s.defaultMachines&&!s.defaultEnabledMachines;
        s.graphAuthoredInactive=noMachines||authoredDisabled;
        s.graphReady=s.graphAuthoredInactive;
        if(!s.graphReady)s.waiting="original_disabled_graph_requires_authored_component_profile";
    }else s.graphReady=s.enabledMachines==s.selectedMachines&&s.machines>=s.defaultMachines;
    return true;
}
inline bool Resources(Player& p,Item& s){
    s.resourcesReady=false;s.resourceSupported=false;s.resources=s.tunings=0;void *resource=nullptr,*tuning=nullptr;auto& n=s.native;
    for(size_t i=0;i<n.componentCount;++i){auto component=n.components[i];
        if(original_players::IsA(component,local_server_resource_authority::ResourceClass)){++s.resources;resource=component;}
        if(original_players::IsA(component,original_ability_tuning::ComponentClass)){++s.tunings;tuning=component;}}
    if(s.resource&&(s.resources!=1||s.resource!=resource||!Live(resource,s.resourceId)))return Fail(p,6132,"hero_loadout_native_resource_changed_within_item_epoch");
    if(s.tuning&&(s.tunings!=1||s.tuning!=tuning||!Live(tuning,s.tuningId)))return Fail(p,6133,"hero_loadout_native_tuning_changed_within_item_epoch");
    if(s.resources>1||s.tunings>1){s.waiting="hero_multiple_resource_or_tuning_authority_not_implemented";return true;}
    if(tuning&&!s.tuning){s.tuning=tuning;if(!Capture(tuning,s.tuningId))return Fail(p,6135,"hero_loadout_native_tuning_identity_unavailable");}
    if(!s.resources){
        // A real tuning component is an independent native NET source. Its
        // absence of a sibling Resource is not an initialized zero resource.
        s.resourcesReady=!s.tuning||p.hero==0;s.waiting=s.tuning&&p.hero!=0?"hero_tuning_only_native_publication_pending":"no_native_resource_observed_hero_semantics_unverified";
        if(p.hero!=0&&s.tuning&&resourceExtension){const auto result=resourceExtension(p,s);
            if(result.error)return Fail(p,result.error,result.stage);
            if(result.handled){s.resourceSupported=result.supported;s.resourcesReady=result.ready;s.waiting=result.stage;}}
        return true;
    }
    if(!s.resource){s.resource=resource;if(!Capture(resource,s.resourceId))return Fail(p,6134,"hero_loadout_native_resource_identity_unavailable");}
    s.resourceSupported=original_players::IsA(resource,local_server_resource_authority::BlueprintResourceClass)
        ||original_players::IsA(resource,local_server_resource_authority::EquipmentChargeClass)
        ||original_players::IsA(resource,local_server_resource_authority::AmmoClass);
    if(!s.resourceSupported)s.waiting="hero_native_resource_simulation_requires_separate_source_profile";
    else if(p.authority){
        if(!s.resourcePrepared){
            if(!local_server_resource_authority::Prepare(p.context.world,p.context.controller,p.context.playerState,p.context.pawn,
                p.context.inventory,n.actor,p.epoch))return Fail(p,local_server_resource_authority::GetState().error,local_server_resource_authority::GetState().stage);
            const auto record=local_server_resource_authority::FindCached(n.actor);
            if(!record||record->pawn!=p.context.pawn||record->sourceEpoch!=p.epoch)
                return Fail(p,6146,"hero_loadout_prepared_resource_receipt_not_observed");
            s.resourcePrepared=true;s.resourceRegistration=record->registration;
        }
        if(!local_server_resource_authority::PollItem(n.actor))return Fail(p,local_server_resource_authority::GetState().error,local_server_resource_authority::GetState().stage);
        s.resourcesReady=local_server_resource_authority::ItemReady(n.actor);
    }else{
        // Only observe the actual native registered replicated component. Do
        // not import a tuning map or copy an authority resource on the client.
        const auto amount=Read<int32_t>(resource,0x110),predicted=Read<int32_t>(resource,0x10c);
        s.resourcesReady=Read<void*>(resource,0xc8)==n.actor&&amount>=0&&predicted>=0;
    }
    if(resourceExtension){const auto result=resourceExtension(p,s);
        if(result.error)return Fail(p,result.error,result.stage);
        if(result.handled){s.resourceSupported=result.supported;s.resourcesReady=result.ready;s.waiting=result.stage;}}
    return true;
}
inline bool Ensure(Player& p){
    if(p.retired||p.error)return !p.error;p.assemblyReady=false;++p.polls;
    if(!Source(p)||!p.context.sourceReady)return !state.error;
    // Authority slot construction follows real possession/connection/source
    // validation. Merely passing a matching hero class is insufficient.
    if(p.authority&&!p.slotsEnsured){
        if(!original_inventory_slots::EnsureForPawn(p.context.pawn))
            return Fail(p,original_inventory_slots::State().error,original_inventory_slots::State().stage);
        p.slotsEnsured=true;
    }
    if(!Authored(p)||!p.catalogBound)return !state.error;
    if(!p.getterVerified&&!NativeStartingGetter(p))return false;
    p.assembledItems=p.readyGraphs=p.readyResources=0;bool ready=true;
    for(size_t i=0;i<p.count;++i){auto& s=p.items[i];auto& n=s.native;s.assembled=false;
        if(!Slot(p,s))return false;
        if(!n.slot){s.waiting="waiting_original_replicated_item_slot";ready=false;continue;}
        if(!Adopt(p,s))return false;
        if(s.equipmentManaged){
            // Ordinary weapon actors and their resources/skins retain the
            // existing Equipment module's own lifecycle. No second factory.
            s.assembled=n.actor&&Read<void*>(n.actor,0x210)==p.context.pawn&&Read<void*>(n.actor,0x4a8)==p.context.pawn
                &&local_server_item_ownership::Contains(n.slot,n.actor);
            s.graphReady=s.resourcesReady=s.assembled;
            if(s.assembled){++p.assembledItems;++p.readyGraphs;++p.readyResources;s.waiting="ordinary_weapon_equipment_module_receipt_observed";}
            else{s.waiting="waiting_ordinary_weapon_equipment_module_receipt";ready=false;}
            continue;
        }
        if(!n.actor){
            if(!p.authority){s.waiting="waiting_original_replicated_skill_actor";ready=false;continue;}
            if(!NativeResult(p,loadout::Spawn(p.context,n)))return false;
            if(!original_ability_tuning::EnsureActor(n.actor)||!original_effects::EnsureActor(n.actor,"host_authored_hero_skill"))
                return Fail(p,6136,"hero_loadout_original_tuning_or_effect_context_incomplete");
        }
        if(!Live(n.actor,n.actorId))return Fail(p,6137,"hero_loadout_original_item_retired_without_epoch_notification");
        if(p.authority){
            if(!NativeResult(p,loadout::Owner(p.context,n))||!NativeResult(p,loadout::Policy(p.context,n)))return false;
            if(!n.ownerReady||!n.ownershipReady){s.waiting="waiting_original_host_item_owner_policy";ready=false;continue;}
            if(!NativeResult(p,loadout::Publish(p.context,n)))return false;
        }else if(!ClientOwner(p,s))return false;
        n.role=Read<uint8_t>(n.actor,0x224);n.remoteRole=Read<uint8_t>(n.actor,0x128);uint8_t begun=0;
        if(!loadout::Event(n.actor,L"/Script/Engine.Actor.HasActorBegunPlay",1,&begun))return Fail(p,6138,"hero_loadout_original_begin_play_query_failed");
        n.beginPlay=begun!=0;s.assembled=n.published&&n.ownerReady&&n.beginPlay&&loadout::Contains(n)
            &&(p.authority?n.role==3&&n.remoteRole==2:n.role==2);
        if(!s.assembled){s.waiting="waiting_original_skill_actor_lifecycle_and_role";ready=false;continue;}
        ++p.assembledItems;
        if(!CommonEquip(p,s)||!Graph(p,s)||!Resources(p,s))return false;
        if(s.graphReady)++p.readyGraphs;if(s.resourcesReady)++p.readyResources;
        ready=ready&&s.commonEquipRegistered&&s.graphReady&&s.resourcesReady;
        if(s.commonEquipRegistered&&s.graphReady&&s.resourcesReady)s.waiting="native_loadout_assembled_hero_execution_unverified";
    }
    if(!ready)return Wait(p,"hero_loadout_waiting_original_graph_or_resource_receipts");
    p.assemblyReady=true;p.status=Status::AssemblyReady;p.stage="native_hero_loadout_assembly_ready_execution_unverified";state.stage=p.stage;return true;
}
inline bool InitImpl(void* world,bool host){
    if(!world||!engine::image||!original_players::Thread())return Fail(6139,"hero_loadout_init_requires_owned_game_thread_and_world");
    if(state.initialized)return (state.world==world&&state.host==host)||Fail(6140,"hero_loadout_world_or_network_role_changed_requires_new_pair");
    if(!Capture(world,state.worldId))return Fail(6151,"hero_loadout_original_world_weak_identity_unavailable");
    if(!Profiles())return false;
    state.world=world;state.host=host;state.gameThread=GetCurrentThreadId();state.initialized=true;
    state.stage="hero_loadout_native_catalog_interface_initialized";return true;
}
inline bool RegisterPlayerImpl(void* pc,void* ps,void* pawn,unsigned hero,uint32_t epoch){
    if(!Thread()||state.error||!pc||!ps||!pawn||!epoch||hero>=original_heroes::PlayableHeroCount)
        return Fail(6141,"hero_loadout_registration_requires_actual_source_hero_and_epoch");
    if(state.host&&!Read<void*>(pc,0x510)){++state.nonHumanUnchanged;return true;}
    for(auto& existing:players)if(existing.used&&!existing.retired&&existing.context.pawn==pawn){
        if(existing.epoch!=epoch||existing.hero!=hero||existing.context.controller!=pc||existing.context.playerState!=ps)
            return Fail(existing,6142,"hero_loadout_registered_source_changed_without_retirement");
        return Ensure(existing);
    }
    auto type=engine::Find(original_heroes::Catalog[hero].path);auto actualType=Read<void*>(pawn,0x10);
    if(!type||!original_players::Derived(actualType,type))return Fail(6143,"hero_loadout_actual_pawn_does_not_match_selected_original_hero");
    Player* target=nullptr;for(auto& p:players)if(!p.used||p.retired){target=&p;break;}
    if(!target)return Fail(6144,"hero_loadout_bounded_source_cache_full");
    *target={};auto& p=*target;p.used=true;p.authority=state.host;p.hero=hero;p.epoch=epoch;
    p.heroClass=type;p.heroDefaults=Read<void*>(actualType,0x120);auto& c=p.context;
    c.used=true;c.epoch=epoch;c.world=state.world;c.worldId=state.worldId;c.controller=pc;c.playerState=ps;c.pawn=pawn;c.inventory=Read<void*>(pawn,0xc08);
    if(!Capture(type,p.heroClassId)||!Capture(p.heroDefaults,p.heroDefaultsId)||!Capture(pc,c.controllerId)
       ||!Capture(ps,c.playerStateId)||!Capture(pawn,c.pawnId)||!Capture(c.inventory,c.inventoryId))
        return Fail(p,6145,"hero_loadout_actual_source_weak_identity_capture_failed");
    if(state.registrations==UINT64_MAX)return Fail(p,6147,"hero_loadout_source_registration_generation_exhausted");
    p.registration=++state.registrations;
    if(!Authored(p)||!p.catalogBound)return !state.error;
    return Ensure(p);
}
inline void RetireResources(Player& p){
    if(!p.authority)return;
    // Only this epoch's exact cached component route is retired. This also
    // handles a GC-reused Pawn address without clearing a newer epoch's cache.
    // No UObject/native resource/slot is mutated and weapon records remain
    // with Equipment's owner. An earlier native/Jett retirement is harmless.
    for(auto& record:local_server_resource_authority::records){
        if(!record.used||record.weaponAmmo||record.pawn!=p.context.pawn||record.sourceEpoch!=p.epoch
           ||!original_inventory_slots::SameIdentity(record.identities[3],p.context.pawnId))continue;
        for(size_t i=0;i<p.count;++i){const auto& item=p.items[i];
            if(!item.resourcePrepared||item.native.actor!=record.item||item.resourceRegistration!=record.registration
               ||!original_inventory_slots::SameIdentity(item.native.actorId,record.identities[5]))continue;
            record={};++local_server_resource_authority::state.retired;break;}
    }
}
inline bool PollImpl(){
    if(!Thread()||state.error)return false;state.players=state.readyPlayers=0;const auto now=GetTickCount64();
    for(auto& p:players)if(p.used&&!p.retired){
        if(!Live(p.context.pawn,p.context.pawnId)){RetireResources(p);p.retired=true;p.assemblyReady=false;p.status=Status::Retired;
            p.stage="hero_loadout_expired_epoch_diagnostic_retired";++state.staleRetirements;continue;}
        ++state.players;if(!p.lastSample||now-p.lastSample>=100){p.lastSample=now;if(!Ensure(p))return false;}
        if(p.assemblyReady)++state.readyPlayers;}
    ++state.polls;return true;
}
inline bool RetireImpl(void* pawn,uint32_t epoch){
    if(!Thread())return false;
    for(auto& p:players)if(p.used&&!p.retired&&p.context.pawn==pawn&&p.epoch==epoch){
        // Only cached receipts retire here. Match/Equipment own original actor
        // and resource teardown; this module never destroys a second copy.
        RetireResources(p);p.retired=true;p.assemblyReady=false;p.status=Status::Retired;p.stage="hero_loadout_epoch_receipts_retired";++state.retirements;}
    return true;
}
inline LONG Exception(EXCEPTION_POINTERS* e){
    state.error=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0006100U;
    auto at=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;state.restartRequired=true;
    state.stage="hero_loadout_native_exception_requires_new_owned_pair";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Init(void* world,bool host){__try{const bool result=InitImpl(world,host);Cache();return result;}__except(Exception(GetExceptionInformation())){Cache();return false;}}
inline bool RegisterPlayer(void* pc,void* ps,void* pawn,unsigned hero,uint32_t epoch){
    __try{const bool result=RegisterPlayerImpl(pc,ps,pawn,hero,epoch);Cache();return result;}__except(Exception(GetExceptionInformation())){Cache();return false;}}
inline bool Poll(){__try{const bool result=PollImpl();Cache();return result;}__except(Exception(GetExceptionInformation())){Cache();return false;}}
inline bool RetirePlayer(void* pawn,uint32_t epoch){__try{const bool result=RetireImpl(pawn,epoch);Cache();return result;}__except(Exception(GetExceptionInformation())){Cache();return false;}}
inline bool AssemblyReady(void* pawn,uint32_t epoch){
    if(!Thread()||state.error||state.restartRequired)return false;
    for(const auto& p:players)if(p.used&&!p.retired&&p.context.pawn==pawn&&p.epoch==epoch)
        return p.assemblyReady&&!p.error&&Live(pawn,p.context.pawnId)&&Live(p.heroClass,p.heroClassId)&&Live(p.heroDefaults,p.heroDefaultsId);
    return false;
}
template<class Visit> inline bool VisitReplicationGraph(Visit&& visit){
    if(!Thread()||state.error)return false;
    for(const auto& p:players)if(p.used&&!p.retired&&p.authority&&Live(p.context.pawn,p.context.pawnId))
        for(size_t i=0;i<p.count;++i){const auto& s=p.items[i];const auto& n=s.native;
            if(s.equipmentManaged||!n.ownershipReady||!Live(n.actor,n.actorId))continue;
            visit(n.actor,n.actor);if(Live(n.slot,n.slotId))visit(p.context.pawn,n.slot);
            for(size_t k=0;k<n.componentCount;++k)if(Live(n.components[k],n.componentIds[k]))visit(n.actor,n.components[k]);}
    return true;
}
// Registration hook for independent host producers. A visitor returns bool
// and receives the proof for the original skill plus one actual owned native
// component. Consumers still validate their own exact component class/ABI.
template<class Visit> inline bool VisitRegisteredComponents(Visit&& visit){
    if(!Thread()||state.error)return false;
    for(const auto& p:players)if(p.used&&!p.retired&&p.authority)
        for(size_t i=0;i<p.count;++i){const auto& n=p.items[i].native;SourceView source{};
            if(!n.actor||!ResolveSource(n.actor,source))continue;
            for(size_t k=0;k<n.componentCount;++k){auto component=n.components[k];
                if(!Live(component,n.componentIds[k])||Read<void*>(component,0xc8)!=n.actor)continue;
                if(!visit(source,component))return false;}
        }
    return true;
}
inline void Write(FILE* file){
    if(!file)return;const auto d=Snapshot();const auto& s=d.summary;auto b=[](bool v){return v?"true":"false";};
    std::fprintf(file,"{\"stage\":");original_players::Text(file,s.stage);
    std::fprintf(file,",\"error\":%u,\"initialized\":%s,\"host\":%s,\"profiles_verified\":%s,\"schema_verified\":%s,\"restart_required\":%s,\"players\":%u,\"assembly_ready_players\":%u,\"registrations\":%llu,\"retirements\":%llu,\"stale_retirements\":%llu,\"nonhuman_unchanged\":%llu,\"exception_rva\":%llu,\"network_execution_verified\":false,\"hero_specific_authority_verified\":false,\"playable_verified\":false,\"client_factory\":false,\"client_slot_writes\":0,\"cached_snapshot\":true,\"sampled_at_ms\":%llu,\"entries\":[",
        s.error,b(s.initialized),b(s.host),b(s.profilesVerified),b(s.schemaVerified),b(s.restartRequired),s.players,s.readyPlayers,
        static_cast<unsigned long long>(s.registrations),static_cast<unsigned long long>(s.retirements),static_cast<unsigned long long>(s.staleRetirements),
        static_cast<unsigned long long>(s.nonHumanUnchanged),static_cast<unsigned long long>(s.exceptionRva),static_cast<unsigned long long>(d.at));
    bool comma=false;for(const auto& p:d.players)if(p.used){if(comma)std::fputc(',',file);comma=true;
        std::fprintf(file,"{\"hero\":%u,\"hero_name\":",p.hero);const auto label=original_players::Utf8(original_heroes::Catalog[p.hero].name);original_players::Text(file,label.c_str());
        std::fprintf(file,",\"pawn\":\"%p\",\"epoch\":%u,\"authority\":%s,\"status\":%u,\"error\":%u,\"retired\":%s,\"catalog_bound\":%s,\"assembly_ready\":%s,\"authored_count\":%zu,\"assembled_items\":%zu,\"ready_graphs\":%zu,\"ready_resources\":%zu,\"stage\":",
            p.pawn,p.epoch,b(p.authority),unsigned(p.status),p.error,b(p.retired),b(p.catalogBound),b(p.assemblyReady),p.count,p.assembledItems,p.readyGraphs,p.readyResources);original_players::Text(file,p.stage);
        std::fputs(",\"items\":[",file);for(size_t i=0;i<p.count;++i){const auto& item=p.items[i];if(i)std::fputc(',',file);
            std::fprintf(file,"{\"authored_index\":%zu,\"class\":\"%p\",\"actor\":\"%p\",\"slot\":%u,\"multi\":%s,\"equipment_managed\":%s,\"role\":%u,\"remote_role\":%u,\"spawned\":%s,\"adopted\":%s,\"published\":%s,\"assembled\":%s,\"begin_play\":%s,\"owner_ready\":%s,\"graph_ready\":%s,\"default_graph_sampled\":%s,\"default_graph_complete\":%s,\"default_construction_machines\":%s,\"default_machines\":%u,\"default_enabled_machines\":%u,\"graph_authored_inactive\":%s,\"resource_ready\":%s,\"common_equip_registered\":%s,\"machines\":%u,\"enabled_machines\":%u,\"selected_machines\":%u,\"transitions\":%llu,\"resources\":%u,\"tunings\":%u,\"resource_supported\":%s,\"waiting\":",
                i,item.type,item.actor,unsigned(item.slotType),b(item.multi),b(item.equipmentManaged),unsigned(item.role),unsigned(item.remoteRole),b(item.spawned),b(item.adopted),b(item.published),b(item.assembled),b(item.beginPlay),b(item.ownerReady),b(item.graphReady),b(item.defaultGraphSampled),b(item.defaultGraphComplete),b(item.defaultConstructionMachines),item.defaultMachines,item.defaultEnabledMachines,b(item.graphAuthoredInactive),b(item.resourcesReady),b(item.commonEquipRegistered),item.machines,item.enabledMachines,item.selectedMachines,static_cast<unsigned long long>(item.transitions),item.resources,item.tunings,b(item.resourceSupported));original_players::Text(file,item.waiting);std::fputc('}',file);}
        std::fputs("]}",file);}
    std::fputs("]}",file);
}
} // namespace local_server_hero_abilities
