#pragma once
#include "local_server_item_ownership.h"
#include "original_jett_priming.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

// Missing HOST starting-loadout assembly for the admitted Wushu player only.
// BD8 is the source of classes; only its actual missing Unarmed/Glide entries
// are constructed. The retail ItemSlot AuthAdd/Remove v2F8/v300 bodies both
// return false. Native reflection/container APIs below fill that absent host
// policy; this is not a recovered retail server grant implementation.
// No client factory, role/state/enable write, shop gate write or resource fill.
// Evidence: work/local-server-playable/equipment/starting-loadout-audit.
namespace local_server_starting_equipment {
using Weak=original_inventory_slots::WeakIdentity;
inline constexpr size_t MaxPlayers=8,StartingCount=7,StarterCount=2,MaxComponents=128;
inline constexpr size_t AuthoredIndices[StarterCount]{0,6};
inline constexpr uint8_t AuthoredSlots[StarterCount]{10,6};
inline constexpr const char* StarterNames[StarterCount]{"Unarmed","Glide"};
inline constexpr const wchar_t* StarterPaths[StarterCount]{
    L"/Game/Characters/_Core/Equippable_Unarmed.Equippable_Unarmed_C",
    L"/Game/Characters/Wushu/S0/Glide/Ability_Wushu_Passive_Glide.Ability_Wushu_Passive_Glide_C"};
inline constexpr const wchar_t* WushuPath=L"/Game/Characters/Wushu/Wushu_PC.Wushu_PC_C";

enum class Status:uint8_t {NotRegistered,Waiting,Ready,Retired,Failed};
struct ClassObservation {
    void *type=nullptr,*defaults=nullptr,*item=nullptr;
    Weak typeId{},defaultsId{},itemId{};
    uint8_t defaultSlot=255;
    bool classLive=false,defaultsLive=false,queried=false,itemLive=false,itemOwned=false;
};
struct Starter {
    void *type=nullptr,*slot=nullptr,*actor=nullptr,*machine=nullptr;
    Weak typeId{},slotId{},actorId{},machineId{};
    std::array<void*,MaxComponents> components{};
    std::array<Weak,MaxComponents> componentIds{};
    size_t componentCount=0;
    void* selectedState=nullptr;Weak selectedStateId{};
    void *instigatorBefore=nullptr,*instigator=nullptr;
    uint8_t slotType=255,role=255,remoteRole=255,inInventory=0;
    bool multi=false,spawnAttempted=false,spawned=false,adopted=false;
    bool ownerReady=false,ownershipReady=false,published=false,beginPlay=false;
    bool componentsCaptured=false,machineEnabled=false,machineReady=false,eventReady=false,ready=false;
    bool selectedStateLive=false,stateOwnerMatches=false,stateMachineMatches=false;
    bool instigatorQueried=false,instigatorReady=false;
    local_server_item_ownership::Status ownershipStatus=local_server_item_ownership::Status::NotObserved;
    int32_t multiBefore=-1,multiAfter=-1,nativeAddIndex=-1;
    uint64_t ownerCallbacks=0,slotNotifications=0,startingInitializations=0,eventRegistrations=0,instigatorSetters=0;
};
struct Player {
    bool used=false,retired=false,ready=false,sourceReady=false,slotsReady=false,classesReady=false;
    unsigned error=0;uint32_t epoch=0;Status status=Status::NotRegistered;
    const char* stage="host_starting_equipment_not_registered";
    void *world=nullptr,*controller=nullptr,*playerState=nullptr,*pawn=nullptr,*inventory=nullptr;
    Weak worldId{},controllerId{},playerStateId{},pawnId{},inventoryId{};
    std::array<ClassObservation,StartingCount> classes{};
    std::array<Starter,StarterCount> starters{};
    uint32_t presentMask=0,ownedMask=0,missingMask=0;
    unsigned nativeLookups=0;uint64_t nativeLookupTotal=0,polls=0,lastSampleMs=0;
};
struct State {
    bool initialized=false,profilesVerified=false,schemaVerified=false,restartRequired=false;
    unsigned error=0,players=0,readyPlayers=0;DWORD gameThread=0;
    void* world=nullptr;Weak worldId{};
    uint64_t polls=0,registrations=0,retirements=0,staleRetirements=0,nonHumanUnchanged=0;
    uint64_t nativeFactories=0,nativeArrayAdds=0,nativeObjectSetters=0;
    uintptr_t exceptionRva=0;
    const char* stage="host_starting_equipment_not_initialized";
    const char* operation="none";
    const wchar_t* failedEvent=nullptr;uint16_t expectedEventBytes=0,actualEventBytes=0;
};
struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
inline constexpr Profile Profiles[]{
    {0x43fada0,2189,0xbce8a0b9824dcd0aULL},{0x3ef6cb0,5560,0xbe0c00750310708dULL},
    {0x70f82b0,248,0x435698ca5f5cafc5ULL},
    {0x712a0d0,84,0x5bebb710b3f4f290ULL},{0x712a124,69,0x9d98430f149cb282ULL},
    {0x712a169,38,0x34c1789499574c3eULL},{0x7132d50,173,0x99bb1e6fa9fc2da6ULL},
    {0x4516300,497,0x36959e979faea5d0ULL},{0x68f7050,69,0x068af81ea47454eaULL},
    {0x45c9b70,762,0xf4cca461305f33f1ULL},
    {0x3ef1e00,32,0xee604397a5fa7d5bULL},
    {0x70ec480,636,0x2b872f1c0627e25aULL},{0x18a76a0,3,0x62941518298855c4ULL},
    {0x6ff5f40,101,0xeb154c7c237fd429ULL},{0x6ff6000,20,0x54a3b75986931278ULL},
    {0x6ff6014,4,0xdbdaadda7244606aULL},{0x6ff6018,784,0x83a108b8d3ee771cULL},
    {0x6ff6328,441,0x180238b5ad5f5d6bULL},{0x6ff64e1,30,0xc4f7623038ff7438ULL},
    {0x6ff64ff,322,0x1b8cc2abfd8f54a1ULL},{0x6ff6641,8,0x2d61151501c50057ULL},
    {0x6ff68d0,77,0x08971fb2c88a16baULL},{0x7116e60,380,0x5a885bf9275e75c1ULL}
};
inline State state;
inline std::array<Player,MaxPlayers> players{};
struct CachedDiagnostics {State summary{};std::array<Player,MaxPlayers> entries{};uint64_t sampledAtMs=0;};
inline SRWLOCK diagnosticsLock=SRWLOCK_INIT;
inline CachedDiagnostics cachedDiagnostics{};
inline uint64_t lastDiagnosticsAt=0;
inline void CacheDiagnostics(bool force=false){
    const auto now=GetTickCount64();if(!force&&lastDiagnosticsAt&&now-lastDiagnosticsAt<100)return;
    AcquireSRWLockExclusive(&diagnosticsLock);
    cachedDiagnostics.summary=state;cachedDiagnostics.entries=players;cachedDiagnostics.sampledAtMs=now;
    ReleaseSRWLockExclusive(&diagnosticsLock);lastDiagnosticsAt=now;
}
inline CachedDiagnostics DiagnosticsSnapshot(){
    AcquireSRWLockShared(&diagnosticsLock);const auto result=cachedDiagnostics;ReleaseSRWLockShared(&diagnosticsLock);return result;
}
inline void *multiContentsProperty=nullptr,*singleContentsProperty=nullptr,*instigatorProperty=nullptr;
inline void *objectSetter=nullptr,*systemLibrary=nullptr,*instigatorGetter=nullptr;
struct Name {uint32_t words[3]{};};
inline Name contentsName{},instigatorName{};
template<class T> struct Array {T* data=nullptr;int32_t count=0,capacity=0;};
static_assert(sizeof(Array<void*>)==16&&sizeof(Name)==12);
struct ObjectSet {void* object=nullptr;Name name{};uint32_t padding=0;void* value=nullptr;};
static_assert(sizeof(ObjectSet)==32&&offsetof(ObjectSet,name)==8&&offsetof(ObjectSet,value)==24);

inline const State& GetState(){return state;}
// Game-thread grant receipt. File diagnostics use the separate immutable
// snapshot above; native client receipt and execution remain separate.
inline const Player* GetPlayer(void* pawn,uint32_t epoch){
    for(const auto& p:players)if(p.used&&p.pawn==pawn&&p.epoch==epoch)return &p;return nullptr;
}
inline bool Ready(void* pawn,uint32_t epoch){auto p=GetPlayer(pawn,epoch);return !state.error&&!state.restartRequired&&p&&!p->retired&&p->ready&&!p->error;}
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Capture(void* object,Weak& identity){return original_inventory_slots::CaptureIdentity(object,identity)&&Live(object,identity);}
inline bool Thread(){return engine::image&&!state.restartRequired&&original_players::Thread()
    &&(!state.gameThread||state.gameThread==GetCurrentThreadId());}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;return false;}
inline bool Fail(Player& p,unsigned error,const char* stage){p.error=error;p.ready=false;p.status=Status::Failed;p.stage=stage;return Fail(error,stage);}
inline bool Wait(Player& p,const char* stage){p.ready=false;p.status=Status::Waiting;p.stage=stage;state.stage=stage;return true;}
inline bool Event(void* object,const wchar_t* path,uint16_t bytes,void* parameters){
    auto function=engine::Find(path);const auto actual=Read<uint16_t>(function,0xbe);
    if(!object||!function||actual!=bytes||(Read<uint32_t>(function,0xb8)&0x400U)==0){state.failedEvent=path;state.expectedEventBytes=bytes;state.actualEventBytes=actual;return false;}
    auto dispatch=Read<void*>(Read<void*>(object),0x298);if(!dispatch)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(dispatch)(object,function,parameters);return true;
}
inline bool ValidArray(const Array<void*>& value,int32_t bound){
    return value.count>=0&&value.count<=bound&&value.capacity>=value.count
        &&(!value.count||original_players::Readable(value.data,static_cast<size_t>(value.count)*8));
}
inline bool VerifyProfiles(){
    if(state.profilesVerified)return true;
    for(const auto& profile:Profiles){auto code=reinterpret_cast<const uint8_t*>(engine::image+profile.rva);
        if(!original_players::Readable(code,profile.bytes))return Fail(5800,"host_starting_equipment_profile_unreadable");
        uint64_t digest=0xcbf29ce484222325ULL;for(size_t i=0;i<profile.bytes;++i)digest=(digest^code[i])*0x100000001b3ULL;
        if(digest!=profile.fnv)return Fail(5801,"host_starting_equipment_original_profile_mismatch");}
    state.profilesVerified=true;return true;
}
inline bool Field(void* type,const wchar_t* name,int32_t offset,int32_t size,int32_t dimensions,uint64_t flags,void** result=nullptr){
    original_jett_priming::PropertyInfo info{};
    if(original_jett_priming::FieldOffset(type,name,static_cast<size_t>(size)*static_cast<size_t>(dimensions),&info)!=offset
       ||info.arrayDim!=dimensions||info.elementSize!=size||(Read<uint64_t>(info.field,0x38)&flags)!=flags)return false;
    if(result)*result=info.field;return true;
}
inline bool VerifySchema(){
    if(state.schemaVerified)return true;
    auto item=engine::Class(0x645c570);auto equippable=engine::Class(0x645c660);
    auto single=engine::Class(0x68f63c0);auto multi=engine::Class(0x69cf3d0);
    if(!Field(engine::Class(original_players::ShooterCharacterClass),L"StartingEquippableClasses",0xbd8,16,1,0)
       ||!Field(engine::Class(0x6499330),L"ItemSlots",0x1b8,8,17,0x100000020ULL)
       ||!Field(item,L"EquippableSlot",0x4a0,1,1,0)||!Field(item,L"InInventory",0x4b0,1,1,0)
       ||!Field(item,L"Owner",0x210,8,1,0x20)||!Field(item,L"Instigator",0x278,8,1,0x20,&instigatorProperty)
       ||!Field(item,L"MyPawn",0x4a8,8,1,0)
       ||!Field(single,L"Contents",0x38,8,1,0x100000020ULL,&singleContentsProperty)
       ||!Field(multi,L"MultiContents",0x50,16,1,0x100000020ULL,&multiContentsProperty)
       ||!Field(multi,L"Capacity",0x48,4,1,0)||!Field(single,L"Inventory",0x40,8,1,0)
       ||!Field(single,L"SlotType",0x30,1,1,0)
       ||!Field(equippable,L"EquippableStateMachine",0xe00,8,1,0))
        return Fail(5802,"host_starting_equipment_original_slot_schema_mismatch");
    auto inner=Read<void*>(multiContentsProperty,0x80);
    if(!inner||Read<int32_t>(inner,0x30)!=1||Read<int32_t>(inner,0x34)!=8||Read<void*>(inner,0x78)!=item
       ||Read<void*>(singleContentsProperty,0x78)!=item||Read<void*>(instigatorProperty,0x78)!=engine::Class(0x4853c50))
        return Fail(5803,"host_starting_equipment_original_item_property_type_mismatch");
    auto onRep=engine::Find(L"/Script/ShooterGame.ItemSlot.OnRep_Contents");
    objectSetter=engine::Find(L"/Script/Engine.KismetSystemLibrary.SetObjectPropertyByName");
    instigatorGetter=engine::Find(L"/Script/Engine.Actor.GetInstigator");
    systemLibrary=engine::DefaultObject(engine::Class(0x45aa110));
    if(!onRep||Read<uint16_t>(onRep,0xbe)!=0||Read<void*>(onRep,0xe0)!=engine::function<void*>(0x68f7050)
       ||!objectSetter||Read<uint16_t>(objectSetter,0xbe)!=sizeof(ObjectSet)
       ||Read<void*>(objectSetter,0xe0)!=engine::function<void*>(0x45c9b70)||!systemLibrary
       ||!instigatorGetter||Read<uint16_t>(instigatorGetter,0xbe)!=8||Read<uint32_t>(instigatorGetter,0xb8)!=0x54020403U
       ||Read<void*>(instigatorGetter,0xe0)!=engine::function<void*>(0x3ef1e00)
       ||Read<void*>(instigatorGetter,0x28)!=engine::Class(0x3683900))
        return Fail(5804,"host_starting_equipment_original_property_api_mismatch");
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&contentsName,L"Contents",1);
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&instigatorName,L"Instigator",1);
    state.schemaVerified=true;return true;
}
inline bool Source(Player& p){
    p.sourceReady=false;
    if(!Live(p.world,p.worldId)||!Live(p.controller,p.controllerId)||!Live(p.playerState,p.playerStateId)
       ||!Live(p.pawn,p.pawnId)||!Live(p.inventory,p.inventoryId))return Wait(p,"host_starting_equipment_waiting_live_source_epoch");
    void* connection=nullptr;
    if(!local_server_item_ownership::ActualHuman(p.world,p.controller,p.playerState,p.pawn,p.inventory,connection))
        return Wait(p,"host_starting_equipment_waiting_actual_human_connection_and_possession");
    if(Read<void*>(p.inventory,0x2f0)!=p.pawn)return Fail(p,5805,"host_starting_equipment_native_inventory_pawn_mismatch");
    p.sourceReady=true;return true;
}
inline bool Slots(Player& p){
    p.slotsReady=false;
    for(size_t i=0;i<17;++i){auto slot=Read<void*>(p.inventory,0x1b8+i*8);
        if(!local_server_item_ownership::ValidSlot(p.inventory,slot,i))return Wait(p,"host_starting_equipment_waiting_native_seventeen_slots");}
    p.slotsReady=true;return true;
}
inline bool Authored(Player& p){
    p.classesReady=false;const auto classes=Read<Array<void*>>(p.pawn,0xbd8);
    if(!ValidArray(classes,64))return Fail(p,5806,"host_starting_equipment_authored_class_array_invalid");
    if(classes.count!=static_cast<int32_t>(StartingCount))return Wait(p,"host_starting_equipment_waiting_authored_wushu_seven_classes");
    auto equippable=engine::Class(0x645c660);
    for(size_t i=0;i<StartingCount;++i){auto& row=p.classes[i];auto type=Read<void*>(classes.data,i*8);
        if(row.type&&(row.type!=type||!Live(type,row.typeId)))return Fail(p,5807,"host_starting_equipment_authored_class_changed_within_epoch");
        if(!row.type){row.type=type;if(!Capture(type,row.typeId))return Fail(p,5850,"host_starting_equipment_authored_class_weak_capture_failed");}
        row.classLive=Live(type,row.typeId)&&original_players::IsA(type,0x1af0eb0)
            &&original_players::Derived(type,equippable);
        if(!row.classLive)return Fail(p,5808,"host_starting_equipment_authored_class_identity_invalid");
        auto defaults=Read<void*>(type,0x120);
        if(row.defaults&&(row.defaults!=defaults||!Live(defaults,row.defaultsId)))return Fail(p,5851,"host_starting_equipment_authored_defaults_changed_within_epoch");
        if(!defaults)return Wait(p,"host_starting_equipment_waiting_original_loaded_class_defaults");
        if(!row.defaults){row.defaults=defaults;if(!Capture(defaults,row.defaultsId))return Fail(p,5809,"host_starting_equipment_authored_defaults_identity_invalid");}
        row.defaultsLive=Live(row.defaults,row.defaultsId)&&original_players::IsA(row.defaults,0x645c660);
        if(!row.defaultsLive)return Fail(p,5809,"host_starting_equipment_authored_defaults_identity_invalid");
        row.defaultSlot=Read<uint8_t>(row.defaults,0x4a0);
        if(row.defaultSlot>=17)return Fail(p,5810,"host_starting_equipment_authored_default_slot_outside_native_enum");
    }
    for(size_t i=0;i<StarterCount;++i){auto& s=p.starters[i];auto& row=p.classes[AuthoredIndices[i]];
        if(row.type!=engine::Find(StarterPaths[i])||row.defaultSlot!=AuthoredSlots[i])
            return Fail(p,5811,"host_starting_equipment_pinned_authored_starter_mismatch");
        s.type=row.type;s.typeId=row.typeId;s.slotType=row.defaultSlot;
        auto slot=Read<void*>(p.inventory,0x1b8+static_cast<size_t>(s.slotType)*8);
        if(s.slot&&(s.slot!=slot||!Live(slot,s.slotId)))return Fail(p,5852,"host_starting_equipment_actual_slot_changed_within_epoch");
        if(!s.slot){s.slot=slot;if(!Capture(s.slot,s.slotId))return Fail(p,5812,"host_starting_equipment_actual_starter_slot_weak_identity_missing");}
        s.multi=original_players::IsA(s.slot,0x69cf3d0);
        if(s.multi!=(i==1))return Fail(p,5813,"host_starting_equipment_authored_single_or_multi_slot_mismatch");
        auto table=Read<void*>(s.slot);
        if(Read<void*>(table,0x2f8)!=engine::function<void*>(0x18a76a0)
           ||Read<void*>(table,0x300)!=engine::function<void*>(0x18a76a0)
           ||Read<void*>(table,0x328)!=engine::function<void*>(i==1?0x7132d50:0x712a0d0))
            return Fail(p,5814,"host_starting_equipment_native_item_slot_dispatch_mismatch");
    }
    p.classesReady=true;return true;
}
inline bool Contains(const Starter& s){
    return Live(s.slot,s.slotId)&&local_server_item_ownership::Contains(s.slot,s.actor);
}
inline bool Owner(Player& p,Starter& s){
    s.ownerReady=false;s.instigatorReady=false;
    if(!Live(s.actor,s.actorId)||!original_players::Derived(Read<void*>(s.actor,0x10),s.type)
       ||Read<uint8_t>(s.actor,0x224)!=3||local_server_item_ownership::ActorWorld(s.actor)!=p.world)
        return Fail(p,5815,"host_starting_equipment_original_actor_identity_or_authority_invalid");
    void* nativeInstigator=nullptr;
    if(!Event(s.actor,L"/Script/Engine.Actor.GetInstigator",8,&nativeInstigator))
        return Fail(p,5855,"host_starting_equipment_original_instigator_query_failed");
    if(!s.instigatorQueried)s.instigatorBefore=nativeInstigator;s.instigatorQueried=true;s.instigator=nativeInstigator;
    if(nativeInstigator&&nativeInstigator!=p.pawn)return Fail(p,5820,"host_starting_equipment_native_factory_instigator_mismatch");
    if(s.spawned&&Read<void*>(s.actor,0x210)!=p.pawn){
        if(Read<void*>(s.actor,0x210))return Fail(p,5816,"host_starting_equipment_spawned_actor_has_foreign_owner");
        if(!Event(s.actor,L"/Script/Engine.Actor.SetOwner",8,&p.pawn))return Fail(p,5817,"host_starting_equipment_native_set_owner_failed");}
    if(Read<void*>(s.actor,0x210)!=p.pawn)return Fail(p,5818,"host_starting_equipment_existing_actor_owner_mismatch");
    if(Read<void*>(s.actor,0x4a8)!=p.pawn){auto old=Read<void*>(s.actor,0x4a8);auto callback=Read<void*>(Read<void*>(s.actor),0x7d8);
        if(old||callback!=engine::function<void*>(0x70ec480))return Fail(p,5819,"host_starting_equipment_original_typed_owner_callback_mismatch");
        reinterpret_cast<void(*)(void*,void*,void*)>(callback)(s.actor,nullptr,p.pawn);++s.ownerCallbacks;}
    if(Read<void*>(s.actor,0x4a8)!=p.pawn)return Wait(p,"host_starting_equipment_waiting_original_typed_owner");
    if(!Event(s.actor,L"/Script/Engine.Actor.GetInstigator",8,&nativeInstigator))
        return Fail(p,5855,"host_starting_equipment_original_instigator_query_failed");
    if(nativeInstigator&&nativeInstigator!=p.pawn)return Fail(p,5820,"host_starting_equipment_native_factory_instigator_mismatch");
    if(!nativeInstigator&&s.spawned){
        // This image exposes GetInstigator but no Actor.SetInstigator UFunction.
        // Supply the missing host grant through its actual Pawn-typed property
        // setter, not a guessed pointer write. Never replace a non-null value.
        if(!Source(p)||!p.sourceReady)return !state.error;
        ObjectSet set{s.actor,instigatorName,0,p.pawn};state.operation="host_starting_equipment_original_instigator_property_set";
        if(!Event(systemLibrary,L"/Script/Engine.KismetSystemLibrary.SetObjectPropertyByName",32,&set))
            return Fail(p,5856,"host_starting_equipment_original_instigator_setter_failed");
        ++s.instigatorSetters;++state.nativeObjectSetters;
        if(!Event(s.actor,L"/Script/Engine.Actor.GetInstigator",8,&nativeInstigator))
            return Fail(p,5855,"host_starting_equipment_original_instigator_query_failed");
        if(!Live(s.actor,s.actorId)||!Live(p.pawn,p.pawnId)||Read<void*>(s.actor,0x210)!=p.pawn||Read<void*>(s.actor,0x4a8)!=p.pawn
           ||local_server_item_ownership::ActorWorld(s.actor)!=p.world||nativeInstigator!=p.pawn)
            return Fail(p,5857,"host_starting_equipment_original_instigator_setter_readback_failed");
        state.operation="none";
    }
    s.instigator=nativeInstigator;s.instigatorReady=nativeInstigator==p.pawn;
    s.ownerReady=Read<void*>(s.actor,0x210)==p.pawn&&Read<void*>(s.actor,0x4a8)==p.pawn&&s.instigatorReady;
    return s.ownerReady||Wait(p,"host_starting_equipment_waiting_original_typed_owner");
}
inline bool Spawn(Player& p,Starter& s){
    if(s.actor)return true;
    if(s.spawnAttempted)return Fail(p,5821,"host_starting_equipment_partial_factory_requires_new_pair");
    engine::Transform transform;
    if(!Event(p.pawn,L"/Script/Engine.Actor.K2_GetActorLocation",24,&transform.translation)
       ||!original_players::Finite(transform.translation))return Fail(p,5822,"host_starting_equipment_native_pawn_transform_missing");
    s.spawnAttempted=true;state.operation="host_starting_equipment_original_deferred_factory";
    // Original RCX is WorldContextObject, independent of the fifth Owner
    // argument. A real Pawn context supplies itself as native Instigator;
    // passing UWorld would deliberately leave that factory parameter null.
    s.actor=engine::function<void*(*)(void*,void*,const engine::Transform*,uint8_t,void*,uint8_t)>(0x43fada0)
        (p.pawn,s.type,&transform,1,p.pawn,0);
    if(!s.actor||!Capture(s.actor,s.actorId))return Fail(p,5823,"host_starting_equipment_original_actor_factory_failed");
    s.spawned=true;++state.nativeFactories;
    engine::function<void(*)(void*,const engine::Transform*,bool,void*,uint8_t)>(0x3ef6cb0)(s.actor,&transform,false,nullptr,0);
    uint8_t one=1;
    if(!Owner(p,s)||!Event(s.actor,L"/Script/Engine.Actor.SetReplicates",1,&one))
        return Fail(p,state.error?state.error:5824,state.error?state.stage:"host_starting_equipment_original_set_replicates_failed");
    state.operation="none";return true;
}
inline bool Policy(Player& p,Starter& s){
    if(s.ownershipReady)return true;
    if(!local_server_item_ownership::Prepare(p.world,p.controller,p.playerState,p.pawn,p.inventory,s.actor,s.slot,
        s.published?local_server_item_ownership::Source::PublishedSlot:local_server_item_ownership::Source::PendingAuthorityGrant)){
        const auto& ownership=local_server_item_ownership::GetState();return Fail(p,ownership.error?ownership.error:5825,ownership.stage);}
    s.ownershipStatus=local_server_item_ownership::GetState().status;
    s.ownershipReady=s.ownershipStatus==local_server_item_ownership::Status::Published
        ||s.ownershipStatus==local_server_item_ownership::Status::AlreadyPublished;
    return s.ownershipReady?true:Wait(p,"host_starting_equipment_waiting_actual_owned_remote_simulation_policy");
}
inline bool Publish(Player& p,Starter& s){
    if(s.published)return Contains(s)||Fail(p,5826,"host_starting_equipment_published_membership_changed");
    if(!s.ownershipReady)return true;
    if(!Contains(s)){
        if(s.multi){const auto array=Read<Array<void*>>(s.slot,0x50);const auto capacity=Read<int32_t>(s.slot,0x48);
            if(!ValidArray(array,256)||capacity<=0||capacity>256||array.count>=capacity)
                return Fail(p,5827,"host_starting_equipment_native_multi_capacity_invalid_or_full");
            s.multiBefore=array.count;state.operation="host_starting_equipment_native_reflected_array_add";
            s.nativeAddIndex=engine::function<int32_t(*)(void*,void*,const void*)>(0x4516300)
                (static_cast<uint8_t*>(s.slot)+0x50,multiContentsProperty,&s.actor);
            ++state.nativeArrayAdds;s.multiAfter=Read<int32_t>(s.slot,0x58);
            if(s.nativeAddIndex!=s.multiBefore||s.multiAfter!=s.multiBefore+1||!Contains(s)){
                state.restartRequired=true;return Fail(p,5828,"host_starting_equipment_native_array_add_readback_failed_requires_new_pair");}
        }else{
            if(Read<void*>(s.slot,0x38))return Fail(p,5829,"host_starting_equipment_original_unarmed_slot_occupied");
            ObjectSet set{s.slot,contentsName,0,s.actor};state.operation="host_starting_equipment_original_reflection_object_set";
            auto dispatch=Read<void*>(Read<void*>(systemLibrary),0x298);
            if(!dispatch)return Fail(p,5830,"host_starting_equipment_original_object_setter_dispatch_missing");
            reinterpret_cast<void(*)(void*,void*,void*)>(dispatch)(systemLibrary,objectSetter,&set);++state.nativeObjectSetters;
            if(Read<void*>(s.slot,0x38)!=s.actor)return Fail(p,5831,"host_starting_equipment_original_object_setter_readback_failed");
        }
    }
    // Explicit missing host grant state. The original client only receives it
    // and the actual replicated native slot graph; it never writes this field.
    uint8_t inInventory=1;
    if(!original_players::Write(s.actor,0x4b0,inInventory)
       ||!Event(s.slot,L"/Script/ShooterGame.ItemSlot.OnRep_Contents",0,nullptr))
        return Fail(p,5832,"host_starting_equipment_original_slot_notification_failed");
    ++s.slotNotifications;s.published=true;state.operation="none";return true;
}
inline bool Components(Player& p,Starter& s){
    if(s.componentsCaptured){
        bool current=true;for(size_t i=0;i<s.componentCount;++i)
            current=current&&Live(s.components[i],s.componentIds[i])&&Read<void*>(s.components[i],0xc8)==s.actor;
        if(current)return true;
        // Refresh only from the actor's actual native component query. Never
        // reinterpret an expired cached address as its replacement object.
        s.componentsCaptured=false;s.componentCount=0;
    }
    struct Query {void* type;void** data=nullptr;int32_t count=0,capacity=0;} query{engine::Class(0x36837a0)};
    if(!Event(s.actor,L"/Script/Engine.Actor.K2_GetComponentsByClass",24,&query))return Fail(p,5833,"host_starting_equipment_original_component_query_failed");
    struct Allocation {void* data;~Allocation(){if(data)engine::function<void(*)(void*)>(0x1915720)(data);}} allocation{query.data};
    if(query.count<=0||query.count>static_cast<int32_t>(MaxComponents)||query.capacity<query.count
       ||!original_players::Readable(query.data,static_cast<size_t>(query.count)*8))
        return Fail(p,5834,"host_starting_equipment_original_component_array_invalid");
    s.componentCount=0;
    for(int32_t i=0;i<query.count;++i){auto component=query.data[i];const auto n=s.componentCount;
        if(!original_players::IsA(component,0x36837a0)||Read<void*>(component,0xc8)!=s.actor||!Capture(component,s.componentIds[n]))
            return Fail(p,5835,"host_starting_equipment_original_component_identity_or_owner_mismatch");
        s.components[n]=component;++s.componentCount;}
    s.componentsCaptured=true;return true;
}
inline bool NativeLifecycle(Player& p,Starter& s,size_t index){
    s.ready=false;s.role=Read<uint8_t>(s.actor,0x224);s.remoteRole=Read<uint8_t>(s.actor,0x128);s.inInventory=Read<uint8_t>(s.actor,0x4b0);
    s.selectedStateLive=s.stateOwnerMatches=s.stateMachineMatches=false;
    uint8_t begun=0;
    if(!Event(s.actor,L"/Script/Engine.Actor.HasActorBegunPlay",1,&begun))return Fail(p,5836,"host_starting_equipment_original_begin_play_query_failed");
    s.beginPlay=begun!=0;
    if(!s.beginPlay)return Wait(p,"host_starting_equipment_waiting_original_begin_play");
    if(!Components(p,s))return false;
    if(index==0&&!s.eventReady){auto component=Read<void*>(s.actor,0xe18);auto registry=Read<void*>(component,0x198);auto type=engine::Class(0x6767a30);
        if(!original_players::IsA(component,0x6768f30)||Read<void*>(component,0xc8)!=s.actor||!Read<uint8_t>(component,0x189)
           ||!registry||!type||engine::function<void*(*)(void*)>(0x6ff6000)(s.actor)!=registry)
            return Wait(p,"host_starting_equipment_waiting_original_unarmed_event_registry");
        if(!engine::function<bool(*)(void*,void*)>(0x6ff68d0)(registry,type)){
            // Same retained mode0 host registration used by the other actual
            // equippables. The retail mode1 ServerOnly branch is stripped.
            engine::function<void(*)(void*,void*,uint8_t)>(0x6ff5f40)(component,type,0);++s.eventRegistrations;}
        s.eventReady=engine::function<bool(*)(void*,void*)>(0x6ff68d0)(registry,type);
        if(!s.eventReady)return Fail(p,5837,"host_starting_equipment_original_unarmed_event_registration_failed");
    }
    if(index==1)s.eventReady=true; // Glide's BP binds owner/movement/landing, not a buy/charge input.
    auto machine=Read<void*>(s.actor,0xe00);
    if(s.machine&&(s.machine!=machine||!Live(machine,s.machineId)))return Fail(p,5853,"host_starting_equipment_primary_machine_changed_within_actor_epoch");
    if(!original_players::IsA(machine,0x67722c0)||Read<void*>(machine,0xc8)!=s.actor)
        return Wait(p,"host_starting_equipment_waiting_original_primary_machine");
    if(!s.machine){s.machine=machine;if(!Capture(machine,s.machineId))return Wait(p,"host_starting_equipment_waiting_original_primary_machine");}
    s.machineEnabled=Read<uint8_t>(s.machine,0x348)!=0;s.selectedState=Read<void*>(s.machine,0x1c8);
    if(s.machineEnabled&&!s.selectedState&&!Read<void*>(s.machine,0x1b0)){
        auto starting=Read<void*>(s.machine,0x1e8);
        if(!starting||Read<void*>(starting,0xc8)!=s.actor||Read<void*>(starting,0xe0)!=s.machine)
            return Fail(p,5838,"host_starting_equipment_authored_starting_state_owner_mismatch");
        if(!Event(s.machine,L"/Script/ShooterGame.StateMachineComponent.InitializeStartingState",8,&s.actor))
            return Fail(p,5839,"host_starting_equipment_original_starting_initializer_failed");
        ++s.startingInitializations;s.selectedState=Read<void*>(s.machine,0x1c8);
    }
    s.selectedStateLive=s.selectedState&&Capture(s.selectedState,s.selectedStateId);
    if(s.selectedStateLive){s.stateOwnerMatches=Read<void*>(s.selectedState,0xc8)==s.actor;s.stateMachineMatches=Read<void*>(s.selectedState,0xe0)==s.machine;}
    s.machineReady=s.machineEnabled&&s.selectedStateLive&&s.stateOwnerMatches&&s.stateMachineMatches;
    s.ready=s.published&&Contains(s)&&s.ownerReady&&s.ownershipReady&&s.role==3&&s.remoteRole==2&&s.inInventory==1
        &&s.beginPlay&&s.componentsCaptured&&s.machineReady&&s.eventReady;
    return s.ready?true:Wait(p,"host_starting_equipment_waiting_original_native_item_lifecycle");
}
inline bool ObserveClasses(Player& p){
    p.presentMask=p.ownedMask=p.missingMask=0;p.nativeLookups=0;
    for(size_t i=0;i<StartingCount;++i){auto& row=p.classes[i];row.item=nullptr;row.itemLive=row.itemOwned=row.queried=false;
        if(!Live(row.type,row.typeId)||!Live(row.defaults,row.defaultsId))return Fail(p,5840,"host_starting_equipment_authored_class_or_defaults_expired");
        row.queried=true;row.item=engine::function<void*(*)(void*,void*,uint8_t)>(0x70f82b0)(p.inventory,row.type,0xfd);
        ++p.nativeLookups;++p.nativeLookupTotal;
        if(!row.item){p.missingMask|=1U<<i;continue;}
        row.itemLive=Capture(row.item,row.itemId)&&original_players::IsA(row.item,0x645c660);
        row.itemOwned=row.itemLive&&Read<void*>(row.item,0x210)==p.pawn&&Read<void*>(row.item,0x4a8)==p.pawn;
        if(!row.itemLive)return Fail(p,5841,"host_starting_equipment_original_lookup_returned_invalid_equippable");
        p.presentMask|=1U<<i;if(row.itemOwned)p.ownedMask|=1U<<i;
    }
    return true;
}
inline bool Ensure(Player& p){
    if(p.retired||p.error)return !p.error;p.ready=false;++p.polls;
    if(!Source(p)||!p.sourceReady||!Slots(p)||!p.slotsReady||!Authored(p)||!p.classesReady)return !state.error;
    if(!ObserveClasses(p))return false;
    for(size_t i=0;i<StarterCount;++i){auto& s=p.starters[i];auto found=p.classes[AuthoredIndices[i]].item;
        if(found){
            if(s.actor&&s.actor!=found)return Fail(p,5842,"host_starting_equipment_original_starter_replaced_within_epoch");
            if(!s.actor){s.actor=found;s.actorId=p.classes[AuthoredIndices[i]].itemId;s.adopted=true;s.published=true;}
        }
        if(!Spawn(p,s)||!Owner(p,s)||!Policy(p,s))return false;
        if(!s.ownerReady||!s.ownershipReady)continue;
        if(!Publish(p,s)||!NativeLifecycle(p,s,i))return false;
    }
    if(!ObserveClasses(p))return false;
    bool ready=p.presentMask==0x7fU&&p.ownedMask==0x7fU;
    for(const auto& s:p.starters)ready=ready&&s.ready;
    if(!ready)return Wait(p,"host_starting_equipment_waiting_original_all_seven_class_receipts");
    p.ready=true;p.status=Status::Ready;p.stage="host_authored_starting_loadout_ready_execution_unverified";state.stage=p.stage;return true;
}
inline bool RegisterOrEnsureImpl(void* world,void* pc,void* ps,void* pawn,uint32_t epoch){
    if(!Thread()||!world||!pc||!ps||!pawn||!epoch)return Fail(5843,"host_starting_equipment_requires_game_thread_and_actual_epoch");
    if(state.error)return false;
    // Other authority/NPC loadouts retain their own policy and scheduling.
    if(!Read<void*>(pc,0x510)){++state.nonHumanUnchanged;return true;}
    if(Read<uint8_t>(pc,0x224)!=3||Read<uint8_t>(pawn,0x224)!=3||!original_players::IsA(ps,original_players::ShooterStateClass)
       ||!original_players::IsA(pawn,original_players::ShooterCharacterClass))
        return Fail(5844,"host_starting_equipment_requires_actual_host_player");
    auto wushu=engine::Find(WushuPath);
    if(!wushu||!original_players::Derived(Read<void*>(pawn,0x10),wushu))return Fail(5845,"host_starting_equipment_only_pinned_authored_wushu_supported");
    if(!state.initialized){
        if(!Capture(world,state.worldId))return Fail(5854,"host_starting_equipment_world_weak_identity_missing");
        if(!VerifyProfiles()||!VerifySchema())return false;
        state.world=world;state.gameThread=GetCurrentThreadId();state.initialized=true;
    }
    if(world!=state.world||!Live(world,state.worldId))return Fail(5846,"host_starting_equipment_world_changed_requires_new_pair");
    Player* player=nullptr;
    for(auto& p:players)if(p.used&&p.pawn==pawn&&p.epoch==epoch){player=&p;break;}
    if(player){
        if(player->retired)return true;
        if(player->controller!=pc||player->playerState!=ps)return Fail(*player,5847,"host_starting_equipment_registered_source_changed");
        return Ensure(*player);
    }
    for(auto& p:players)if(!p.used||p.retired){player=&p;break;}
    if(!player)return Fail(5848,"host_starting_equipment_player_cache_capacity");
    *player={};auto& p=*player;p.used=true;p.epoch=epoch;p.world=world;p.worldId=state.worldId;
    p.controller=pc;p.playerState=ps;p.pawn=pawn;p.inventory=Read<void*>(pawn,0xc08);p.status=Status::Waiting;
    if(!Capture(pc,p.controllerId)||!Capture(ps,p.playerStateId)||!Capture(pawn,p.pawnId)||!Capture(p.inventory,p.inventoryId))
        return Fail(p,5849,"host_starting_equipment_actual_source_weak_identity_missing");
    if(!original_inventory_slots::EnsureForPawn(pawn))return Fail(p,original_inventory_slots::State().error,original_inventory_slots::State().stage);
    ++state.registrations;return Ensure(p);
}
inline bool PollImpl(){
    if(!Thread()||state.error)return false;if(!state.initialized)return true;
    state.players=state.readyPlayers=0;const auto now=GetTickCount64();
    for(auto& p:players)if(p.used&&!p.retired){
        if(!Live(p.pawn,p.pawnId)){p.retired=true;p.ready=false;p.status=Status::Retired;
            p.stage="host_starting_equipment_expired_epoch_cache_retired";++state.staleRetirements;continue;}
        ++state.players;
        if(!p.lastSampleMs||now-p.lastSampleMs>=100){p.lastSampleMs=now;if(!Ensure(p))return false;}
        if(p.ready)++state.readyPlayers;
    }
    ++state.polls;return true;
}
inline bool RetirePlayerImpl(void* pawn,uint32_t epoch){
    if(!Thread())return false;
    for(auto& p:players)if(p.used&&p.pawn==pawn&&p.epoch==epoch&&!p.retired){
        // Cache retirement only. Match owns native actor destruction and the
        // inventory's current/desired/slot teardown; no competing removal.
        p.retired=true;p.ready=false;p.status=Status::Retired;p.stage="host_starting_equipment_epoch_cache_retired";++state.retirements;
    }
    return true;
}
inline LONG CaptureException(EXCEPTION_POINTERS* exception){
    state.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0005800U;
    const auto address=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=address>=engine::image?address-engine::image:address;state.restartRequired=true;
    state.stage="host_starting_equipment_exception_requires_new_owned_pair";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool RegisterOrEnsure(void* world,void* pc,void* ps,void* pawn,uint32_t epoch){
    __try{const bool result=RegisterOrEnsureImpl(world,pc,ps,pawn,epoch);CacheDiagnostics(!result);return result;}__except(CaptureException(GetExceptionInformation())){CacheDiagnostics(true);return false;}
}
inline bool Poll(){__try{const bool result=PollImpl();CacheDiagnostics(!result);return result;}__except(CaptureException(GetExceptionInformation())){CacheDiagnostics(true);return false;}}
inline bool RetirePlayer(void* pawn,uint32_t epoch){__try{const bool result=RetirePlayerImpl(pawn,epoch);CacheDiagnostics(true);return result;}__except(CaptureException(GetExceptionInformation())){CacheDiagnostics(true);return false;}}
// Existing Equipment::CollectGraph also discovers both actual slot members.
// Optional visitor supplies exact original subobject dependencies, never IDs,
// fake slots or a second serialization. Root already deduplicates this graph.
template<class Visit> inline bool VisitReplicationGraph(Visit&& visit){
    if(!Thread()||state.error)return false;
    for(const auto& p:players)if(p.used&&!p.retired&&Live(p.pawn,p.pawnId))
        for(const auto& s:p.starters)if(s.ownershipReady&&Live(s.actor,s.actorId)){
            visit(s.actor,s.actor);if(Live(s.slot,s.slotId))visit(p.pawn,s.slot);
            for(size_t i=0;i<s.componentCount;++i)if(Live(s.components[i],s.componentIds[i]))visit(s.actor,s.components[i]);
        }
    return true;
}
inline void Write(FILE* file){
    if(!file)return;const auto snapshot=DiagnosticsSnapshot();const auto& summary=snapshot.summary;
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(file,"{\"stage\":");original_players::Text(file,summary.stage);
    std::fprintf(file,",\"error\":%u,\"initialized\":%s,\"profiles_verified\":%s,\"schema_verified\":%s,\"restart_required\":%s,\"players\":%u,\"ready_players\":%u,\"native_factories\":%llu,\"native_array_adds\":%llu,\"native_object_setters\":%llu,\"registrations\":%llu,\"retirements\":%llu,\"stale_retirements\":%llu,\"nonhuman_unchanged\":%llu,\"exception_rva\":%llu,\"operation\":",
        summary.error,b(summary.initialized),b(summary.profilesVerified),b(summary.schemaVerified),b(summary.restartRequired),summary.players,summary.readyPlayers,
        static_cast<unsigned long long>(summary.nativeFactories),static_cast<unsigned long long>(summary.nativeArrayAdds),static_cast<unsigned long long>(summary.nativeObjectSetters),
        static_cast<unsigned long long>(summary.registrations),static_cast<unsigned long long>(summary.retirements),static_cast<unsigned long long>(summary.staleRetirements),
        static_cast<unsigned long long>(summary.nonHumanUnchanged),static_cast<unsigned long long>(summary.exceptionRva));original_players::Text(file,summary.operation);
    std::fprintf(file,",\"original_server_grant_body_recovered\":false,\"client_factory\":false,\"resource_fill\":false,\"execution_verified\":false,\"cached_snapshot\":true,\"sampled_at_ms\":%llu,\"entries\":[",static_cast<unsigned long long>(snapshot.sampledAtMs));
    bool first=true;for(const auto& p:snapshot.entries)if(p.used){if(!first)std::fputc(',',file);first=false;
        std::fprintf(file,"{\"pawn\":\"%p\",\"epoch\":%u,\"status\":%u,\"error\":%u,\"retired\":%s,\"ready\":%s,\"source_ready\":%s,\"slots_ready\":%s,\"classes_ready\":%s,\"present_mask\":%u,\"owned_mask\":%u,\"missing_mask\":%u,\"native_lookup_calls\":%u,\"native_lookup_total\":%llu,\"stage\":",
            p.pawn,p.epoch,static_cast<unsigned>(p.status),p.error,b(p.retired),b(p.ready),b(p.sourceReady),b(p.slotsReady),b(p.classesReady),p.presentMask,p.ownedMask,p.missingMask,p.nativeLookups,static_cast<unsigned long long>(p.nativeLookupTotal));original_players::Text(file,p.stage);
        std::fprintf(file,",\"authored_classes\":[");for(size_t i=0;i<StartingCount;++i){const auto& c=p.classes[i];if(i)std::fputc(',',file);
            std::fprintf(file,"{\"index\":%zu,\"class\":\"%p\",\"class_live\":%s,\"class_index\":%d,\"class_serial\":%d,\"defaults\":\"%p\",\"default_slot\":%u,\"queried\":%s,\"item\":\"%p\",\"item_live\":%s,\"item_owned\":%s}",
                i,c.type,b(c.classLive),c.typeId.index,c.typeId.serial,c.defaults,static_cast<unsigned>(c.defaultSlot),b(c.queried),c.item,b(c.itemLive),b(c.itemOwned));}
        std::fprintf(file,"],\"starters\":[");for(size_t i=0;i<StarterCount;++i){const auto& s=p.starters[i];if(i)std::fputc(',',file);
            std::fprintf(file,"{\"name\":\"%s\",\"actor\":\"%p\",\"actor_index\":%d,\"actor_serial\":%d,\"slot\":%u,\"multi\":%s,\"spawned\":%s,\"adopted\":%s,\"published\":%s,\"owner_ready\":%s,\"ownership_ready\":%s,\"ownership_status\":%u,\"begin_play\":%s,\"role\":%u,\"remote_role\":%u,\"in_inventory\":%u,\"machine_enabled\":%s,\"machine_ready\":%s,\"selected_state\":\"%p\",\"selected_state_live\":%s,\"state_owner_matches\":%s,\"state_machine_matches\":%s,\"components\":%zu,\"native_add_index\":%d,\"multi_before\":%d,\"multi_after\":%d,\"owner_callbacks\":%llu,\"slot_notifications\":%llu,\"starting_initializations\":%llu,\"ready\":%s",
                StarterNames[i],s.actor,s.actorId.index,s.actorId.serial,static_cast<unsigned>(s.slotType),b(s.multi),b(s.spawned),b(s.adopted),b(s.published),b(s.ownerReady),b(s.ownershipReady),static_cast<unsigned>(s.ownershipStatus),b(s.beginPlay),static_cast<unsigned>(s.role),static_cast<unsigned>(s.remoteRole),static_cast<unsigned>(s.inInventory),b(s.machineEnabled),b(s.machineReady),s.selectedState,b(s.selectedStateLive),b(s.stateOwnerMatches),b(s.stateMachineMatches),s.componentCount,s.nativeAddIndex,s.multiBefore,s.multiAfter,static_cast<unsigned long long>(s.ownerCallbacks),static_cast<unsigned long long>(s.slotNotifications),static_cast<unsigned long long>(s.startingInitializations),b(s.ready));
            std::fprintf(file,",\"instigator_queried\":%s,\"instigator_before\":\"%p\",\"instigator\":\"%p\",\"instigator_ready\":%s,\"instigator_setters\":%llu}",b(s.instigatorQueried),s.instigatorBefore,s.instigator,b(s.instigatorReady),static_cast<unsigned long long>(s.instigatorSetters));}
        std::fprintf(file,"]}");}
    std::fprintf(file,"]}");
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;Write(file);std::fputc('\n',file);return std::fclose(file)==0;
}
} // namespace local_server_starting_equipment
