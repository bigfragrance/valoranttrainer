#pragma once
#include "local_server_abilities.h"
#include <array>
#include <cstdio>
#include <cstring>

// Authority-only observation of real original Jett hits, before the original
// synchronous hitscan can destroy its actor. No firing, tracing, point mutation,
// life write or original-install change. Evidence and integration contract:
// work/local-server-playable/abilities-runtime-review/report.md
namespace local_server_dagger_points {
using Weak=original_inventory_slots::WeakIdentity;
using Constructor=void(*)(const void*);
using EventFn=void(*)(void*,void*,void*);
using EntranceFn=void(*)(void*,const void*,const void*);
using PointSink=local_server_abilities::NativePointSink;
inline constexpr size_t MaxClasses=16,MaxActors=256,ActorEntries=246,DamageEntries=156;
inline constexpr size_t EventIndex=0x298/8,EntranceIndex=0x4d8/8;
inline constexpr uintptr_t ActorTableRva=0xb42fb38,DamageTableRva=0xb1faab0;
inline constexpr uintptr_t ConstructorRva=0x6b2ce90,ActorEventRva=0x3eda0d0,EntranceRva=0x78c6d60;
struct ClassEntry {
    void* type=nullptr;Weak identity{};Constructor parent=nullptr;bool installed=false;
};
struct ActorEntry {
    void* actor=nullptr;Weak identity{};uint32_t epoch=0;uint64_t registration=0;
    bool installed=false;
};
struct ComponentEntry {
    void *component=nullptr,*actor=nullptr;Weak identity{},actorIdentity{};
    uint32_t epoch=0;uint64_t registration=0;bool installed=false;
};
struct State {
    bool initialized=false,bound=false,disabled=false,profilesVerified=false,tablesReady=false,closed=false,restartRequired=false;
    void *world=nullptr,*pawn=nullptr,*ability=nullptr,*playerState=nullptr;
    Weak worldIdentity{},pawnIdentity{},abilityIdentity{};
    uint32_t epoch=0;uint64_t registration=0;unsigned error=0;uintptr_t exceptionRva=0;
    uint64_t installs=0,bindings=0,classInstalls=0,classRestores=0,constructors=0,constructorsForwarded=0,templates=0;
    uint64_t actorInstalls=0,actorEventsForwarded=0,lifecycleEvents=0,componentInstalls=0,entrancesForwarded=0;
    uint64_t sourceDeferrals=0,sourceRejections=0,staleCallbacks=0,missingFireBoundary=0,ambiguousFireBoundary=0;
    uint64_t captures=0,appendedPoints=0,deliveredPoints=0,noNewPoints=0,invalidArrays=0,restores=0,conflicts=0;
    uint32_t callbacksActive=0,constructorsActive=0;
    const char* stage="native_dagger_points_not_installed";
};
inline State state;
inline std::array<ClassEntry,MaxClasses> classes{};
inline std::array<ActorEntry,MaxActors> actors{};
inline std::array<ComponentEntry,MaxActors> components{};
// Backing remains valid for late native callbacks after a source retires.
inline std::array<void*,ActorEntries+1> actorTable{};
inline std::array<void*,DamageEntries+1> damageTable{};
inline void *beginFunction=nullptr,*constructionFunction=nullptr;
inline PointSink expectedSink=nullptr;
inline SRWLOCK snapshotLock=SRWLOCK_INIT;
inline State snapshot;
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Live(void* p,const Weak& id){return p&&original_inventory_slots::ResolveIdentity(id)==p;}
inline bool Identity(void* p,Weak& id){return original_inventory_slots::CaptureIdentity(p,id)&&Live(p,id);}
inline bool Thread(){return engine::image&&original_players::Thread();}
inline void Cache(){AcquireSRWLockExclusive(&snapshotLock);snapshot=state;ReleaseSRWLockExclusive(&snapshotLock);}
inline State Snapshot(){AcquireSRWLockShared(&snapshotLock);const auto s=snapshot;ReleaseSRWLockShared(&snapshotLock);return s;}
inline const State& GetState(){return state;} // GT only; Write uses Snapshot.
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;return false;}
inline bool Wait(const char* stage){state.stage=stage;return true;}
inline LONG Capture(EXCEPTION_POINTERS* e){
    state.error=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0006400U;
    const auto at=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;state.restartRequired=true;
    state.stage="native_dagger_points_observer_exception_requires_new_pair";Cache();return EXCEPTION_EXECUTE_HANDLER;
}
inline bool ImageRegion(const void* p,size_t bytes,bool executable){
    MEMORY_BASIC_INFORMATION m{};if(!p||!VirtualQuery(p,&m,sizeof(m)))return false;
    const auto start=reinterpret_cast<uintptr_t>(p),base=reinterpret_cast<uintptr_t>(m.BaseAddress);
    const auto protection=m.Protect&0xffU;
    return m.State==MEM_COMMIT&&m.Type==MEM_IMAGE&&m.AllocationBase==reinterpret_cast<void*>(engine::image)
        &&!(m.Protect&(PAGE_GUARD|PAGE_NOACCESS))&&start>=base&&bytes<=m.RegionSize&&start-base<=m.RegionSize-bytes
        &&(!executable||protection==PAGE_EXECUTE||protection==PAGE_EXECUTE_READ||protection==PAGE_EXECUTE_READWRITE||protection==PAGE_EXECUTE_WRITECOPY);
}
inline bool PrivateSlot(void* p){
    MEMORY_BASIC_INFORMATION m{};if(!p||!VirtualQuery(p,&m,sizeof(m)))return false;
    const auto start=reinterpret_cast<uintptr_t>(p),base=reinterpret_cast<uintptr_t>(m.BaseAddress);
    return m.State==MEM_COMMIT&&m.Type==MEM_PRIVATE&&(m.Protect&0xffU)==PAGE_READWRITE
        &&!(m.Protect&(PAGE_GUARD|PAGE_NOACCESS))&&start>=base&&m.RegionSize>=8&&start-base<=m.RegionSize-8;
}
inline bool Profile(uintptr_t rva,size_t bytes,uint64_t wanted){
    auto data=reinterpret_cast<const uint8_t*>(engine::image+rva);
    if(!ImageRegion(data,bytes,true))return false;uint64_t hash=1469598103934665603ULL;
    for(size_t n=0;n<bytes;++n)hash=(hash^data[n])*1099511628211ULL;return hash==wanted;
}
inline bool VerifyTable(uintptr_t rva,size_t count,uintptr_t event){
    auto table=reinterpret_cast<void**>(engine::image+rva);
    if(!ImageRegion(table-1,(count+2)*8,false)||table[EventIndex]!=engine::function<void*>(event))return false;
    for(size_t n=0;n<count;++n)if(!ImageRegion(table[n],1,true))return false;
    return !ImageRegion(table[count],1,true); // pinned, finite native table boundary
}
inline bool Verify(){
    if(!Profile(ConstructorRva,20,0xab888b7c04c93796ULL)
       ||!Profile(0x78cb550,1250,0xf468c06f4b096e01ULL)
       ||!Profile(EntranceRva,127,0x328324294ce4d800ULL)
       ||!VerifyTable(ActorTableRva,ActorEntries,ActorEventRva)
       ||!VerifyTable(DamageTableRva,DamageEntries,0x1cdc850))return false;
    return Read<void*>(engine::function<void*>(ActorTableRva),0x1b8)==engine::function<void*>(0x3ed83b0)
        &&Read<void*>(engine::function<void*>(DamageTableRva),0x4d8)==engine::function<void*>(EntranceRva);
}
inline local_server_abilities::Player* Source(){
    if(!state.bound||!Thread()||!Live(state.world,state.worldIdentity)||!Live(state.pawn,state.pawnIdentity)
       ||!Live(state.ability,state.abilityIdentity))return nullptr;
    auto p=local_server_abilities::Find(state.pawn);
    const auto& a=local_server_abilities::GetState();
    if(!p||!local_server_abilities::Source(*p)||!p->authority||!a.host||!a.practice||a.world!=state.world
       ||p->sourceEpoch!=state.epoch||p->registration!=state.registration||!p->sourceEpoch
       ||p->playerState!=state.playerState||p->skills[3].actor!=state.ability
       ||Read<uint8_t>(p->pawn,0x224)!=3||Read<uint8_t>(state.ability,0x224)!=3
       ||Read<void*>(state.ability,0x210)!=p->pawn||Read<void*>(state.ability,0x4a8)!=p->pawn)return nullptr;
    auto connection=Read<void*>(p->controller,0x510),driver=Read<void*>(state.world,0x40);
    return connection&&driver&&Read<void*>(connection,0x38)==p->controller&&Read<void*>(connection,0x60)==driver?p:nullptr;
}
inline bool InWorld(void* actor){
    // Exact native Actor::GetWorld virtual already verified in the pinned
    // native Projectile table. No world/Role/locality field is changed.
    return actor&&engine::function<void*(*)(void*)>(0x3ed83b0)(actor)==state.world;
}
inline ActorEntry* Actor(void* actor){
    for(auto& a:actors)if(a.actor==actor&&Live(actor,a.identity))return &a;return nullptr;
}
inline ComponentEntry* Component(void* component){
    for(auto& c:components)if(c.component==component&&Live(component,c.identity))return &c;return nullptr;
}
inline void ObserveActorEvent(void*,void*,void*);
inline void ObserveEntrance(void*,const void*,const void*);
inline void ObserveConstructor(const void*);
inline bool PrepareTables(){
    if(state.tablesReady)return true;
    auto nativeActor=reinterpret_cast<void**>(engine::image+ActorTableRva),nativeDamage=reinterpret_cast<void**>(engine::image+DamageTableRva);
    std::memcpy(actorTable.data(),nativeActor-1,sizeof(actorTable));std::memcpy(damageTable.data(),nativeDamage-1,sizeof(damageTable));
    actorTable[EventIndex+1]=reinterpret_cast<void*>(&ObserveActorEvent);
    damageTable[EntranceIndex+1]=reinterpret_cast<void*>(&ObserveEntrance);
    state.tablesReady=true;return true;
}
inline bool AttachActor(void* actor,ClassEntry& type){
    if(!Source()||!Live(type.type,type.identity)||Read<void*>(actor,0x10)!=type.type)return true;
    if(Read<uint32_t>(actor,8)&0x30U){++state.templates;return true;}
    if(Read<uint8_t>(actor,0x224)!=3||!InWorld(actor)){++state.sourceDeferrals;return true;}
    if(Actor(actor))return true;
    Weak id{};if(!Identity(actor,id)){++state.sourceDeferrals;return true;}
    auto native=reinterpret_cast<void**>(engine::image+ActorTableRva);
    if(Read<void*>(actor)!=native){++state.conflicts;return Fail(6401,"native_dagger_points_foreign_actor_table");}
    ActorEntry* slot=nullptr;
    for(auto& a:actors){if(a.actor&&!Live(a.actor,a.identity))a={};if(!a.actor&&!slot)slot=&a;}
    if(!slot)return Fail(6402,"native_dagger_points_actor_cache_full");
    *slot={};slot->actor=actor;slot->identity=id;slot->epoch=state.epoch;slot->registration=state.registration;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(actor),actorTable.data()+1,native)!=native)
        return Fail(6403,"native_dagger_points_actor_table_install_conflict");
    slot->installed=true;++state.actorInstalls;return true;
}
inline void AttachActorSafe(void* actor,ClassEntry& type){__try{AttachActor(actor,type);}__except(Capture(GetExceptionInformation())){}}
__declspec(noinline) inline void ObserveConstructor(const void* initializer){
    auto actor=Read<void*>(initializer),type=Read<void*>(actor,0x10);ClassEntry* entry=nullptr;
    for(auto& c:classes)if(c.type==type){entry=&c;break;}
    // All admitted constructors are the same original thunk. A callback that
    // was loaded before restoration must still forward after source retirement.
    auto parent=entry&&entry->parent?entry->parent:engine::function<Constructor>(ConstructorRva);
    const bool owned=Thread();if(owned){++state.constructors;++state.constructorsActive;}
    __try{parent(initializer);}__finally{if(owned){--state.constructorsActive;++state.constructorsForwarded;}}
    if(owned&&entry&&!state.closed&&!state.restartRequired){AttachActorSafe(Read<void*>(initializer),*entry);Cache();}
}
inline bool AttachComponent(ActorEntry& a){
    auto p=Source();if(!p||a.epoch!=state.epoch||a.registration!=state.registration){++state.staleCallbacks;return true;}
    if(!Live(a.actor,a.identity)||Read<uint8_t>(a.actor,0x224)!=3||!InWorld(a.actor)
       ||!local_server_abilities::ProjectileSource(*p,a.actor)){++state.sourceDeferrals;return true;}
    auto component=original_weapon::Component(a.actor,0x6708820);Weak id{};
    if(!component||Read<void*>(component,0xc8)!=a.actor||!Identity(component,id)){++state.sourceDeferrals;return true;}
    if(auto current=Component(component))return current->actor==a.actor;
    auto native=reinterpret_cast<void**>(engine::image+DamageTableRva);
    if(Read<void*>(component)!=native){++state.conflicts;return Fail(6404,"native_dagger_points_foreign_damage_table");}
    ComponentEntry* slot=nullptr;
    for(auto& c:components){if(c.component&&!Live(c.component,c.identity))c={};if(!c.component&&!slot)slot=&c;}
    if(!slot)return Fail(6405,"native_dagger_points_component_cache_full");
    *slot={};slot->component=component;slot->identity=id;slot->actor=a.actor;slot->actorIdentity=a.identity;
    slot->epoch=state.epoch;slot->registration=state.registration;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(component),damageTable.data()+1,native)!=native)
        return Fail(6406,"native_dagger_points_damage_table_install_conflict");
    slot->installed=true;++state.componentInstalls;return true;
}
inline void AttachComponentSafe(ActorEntry& actor){__try{AttachComponent(actor);}__except(Capture(GetExceptionInformation())){}}
inline bool SameEvent(void* actual,void* expected){
    return actual&&expected&&Read<uint16_t>(actual,0xbe)==0
        &&std::memcmp(static_cast<uint8_t*>(actual)+0x18,static_cast<uint8_t*>(expected)+0x18,12)==0;
}
__declspec(noinline) inline void ObserveActorEvent(void* object,void* function,void* parameters){
    auto parent=engine::function<EventFn>(ActorEventRva);ActorEntry* a=nullptr;bool lifecycle=false;
    const bool owned=Thread();if(owned){
        a=Actor(object);lifecycle=a&&Read<void*>(object)==actorTable.data()+1
            &&(SameEvent(function,beginFunction)||SameEvent(function,constructionFunction));
        ++state.actorEventsForwarded;if(lifecycle)++state.lifecycleEvents;++state.callbacksActive;
    }
    __try{parent(object,function,parameters);}__finally{if(owned)--state.callbacksActive;}
    // Both original construction and original BeginPlay finish BEFORE the
    // factory's first movement call. Never execute either event ourselves.
    if(lifecycle&&!state.closed&&!state.restartRequired){AttachComponentSafe(*a);Cache();}
}
inline bool PointArray(void* component,int32_t& count){
    auto data=Read<void*>(component,0x370);count=Read<int32_t>(component,0x378);const auto capacity=Read<int32_t>(component,0x37c);
    return count>=0&&count<=8192&&capacity>=count&&(!count||original_players::Readable(data,static_cast<size_t>(count)*0x130));
}
struct Before {ComponentEntry* component=nullptr;int32_t count=0;uint64_t sequence=0;void* fireState=nullptr;};
inline void BeforeEntrance(void* object,const void* entrance,Before& before){
    if(!Thread()||state.closed||state.restartRequired||state.error||!original_players::Readable(entrance,0xf0))return;
    auto p=Source();auto c=Component(object);
    if(!p||!c||Read<void*>(object)!=damageTable.data()+1||c->epoch!=state.epoch||c->registration!=state.registration||!Live(c->actor,c->actorIdentity)
       ||Read<void*>(object,0xc8)!=c->actor||Read<uint8_t>(c->actor,0x224)!=3||!InWorld(c->actor)
       ||!local_server_abilities::ProjectileSource(*p,c->actor)){++state.sourceRejections;return;}
    if(!expectedSink||local_server_abilities::pointSink!=expectedSink){++state.sourceDeferrals;return;}
    if(!PointArray(object,before.count)){++state.invalidArrays;return;}
    size_t found=0;
    for(const auto& fire:p->fireStates)if(fire.activeDepth&&fire.activeSequence&&Live(fire.object,fire.identity)){
        ++found;before.sequence=fire.activeSequence;before.fireState=fire.object;
    }
    if(!found){++state.missingFireBoundary;return;}
    if(found!=1){++state.ambiguousFireBoundary;return;}
    before.component=c;
}
inline void BeforeEntranceSafe(void* object,const void* entrance,Before& before){__try{BeforeEntrance(object,entrance,before);}__except(Capture(GetExceptionInformation())){}}
inline void AfterEntrance(const Before& before){
    if(!Thread()||!before.component||state.closed||state.restartRequired||state.error)return;
    auto p=Source();const auto& c=*before.component;int32_t count=0;
    if(!p||c.epoch!=state.epoch||c.registration!=state.registration||!Live(c.actor,c.actorIdentity)||!Live(c.component,c.identity)){
        ++state.staleCallbacks;return;
    }
    if(!PointArray(c.component,count)){++state.invalidArrays;return;}
    if(count<=before.count){++state.noNewPoints;return;}
    bool active=false;
    for(const auto& fire:p->fireStates)if(fire.object==before.fireState&&fire.activeDepth&&fire.activeSequence==before.sequence&&Live(fire.object,fire.identity))active=true;
    if(!active||local_server_abilities::pointSink!=expectedSink){++state.missingFireBoundary;return;}
    const auto delivered=local_server_abilities::GetState().factoryPointDeliveries;
    // Existing network sink validates exact pre-fire life, original Core,
    // source/epoch/registration and original point identity before one commit.
    // CapturePoints copies only the original new native records; it applies
    // no damage and never admits a dead projectile or replays a trace.
    if(!local_server_abilities::CapturePoints(*p,c.actor,true,before.sequence,before.fireState)){
        const auto& a=local_server_abilities::GetState();Fail(a.error?a.error:6407,a.stage);return;
    }
    ++state.captures;state.appendedPoints+=static_cast<uint64_t>(count-before.count);
    state.deliveredPoints+=local_server_abilities::GetState().factoryPointDeliveries-delivered;
}
inline void AfterEntranceSafe(const Before& before){__try{AfterEntrance(before);}__except(Capture(GetExceptionInformation())){}}
__declspec(noinline) inline void ObserveEntrance(void* object,const void* lastHit,const void* entrance){
    auto parent=engine::function<EntranceFn>(EntranceRva);Before before;const bool owned=Thread();
    if(owned){BeforeEntranceSafe(object,entrance,before);++state.entrancesForwarded;++state.callbacksActive;}
    // This is the actual original v4D8; keep both native 240B arguments intact.
    __try{parent(object,lastHit,entrance);}__finally{if(owned)--state.callbacksActive;}
    if(owned){AfterEntranceSafe(before);Cache();}
}
inline bool InstallClass(void* type){
    const auto dagger=engine::Find(original_jett::ProjectilePaths[1]);Weak id{};
    if(!type||!dagger||!original_players::Derived(type,dagger)||Read<int32_t>(type,0x60)<0x5e0||!Identity(type,id))
        return Fail(6408,"native_dagger_points_authored_projectile_class_shape_unknown");
    ClassEntry* slot=nullptr;
    for(auto& c:classes){
        if(c.type==type&&original_inventory_slots::SameIdentity(c.identity,id)){slot=&c;break;}
        if(c.type&&!Live(c.type,c.identity)&&!c.installed)c={};if(!c.type&&!slot)slot=&c;
    }
    if(!slot)return Fail(6409,"native_dagger_points_class_cache_full");
    if(slot->installed)return Read<void*>(type,0xb8)==reinterpret_cast<void*>(&ObserveConstructor)
        ||Fail(6410,"native_dagger_points_constructor_slot_changed");
    auto address=static_cast<uint8_t*>(type)+0xb8;auto parent=engine::function<Constructor>(ConstructorRva);
    if(!PrivateSlot(address)||Read<void*>(type,0xb8)!=reinterpret_cast<void*>(parent))
        return Fail(6411,"native_dagger_points_original_constructor_slot_unknown");
    *slot={};slot->type=type;slot->identity=id;slot->parent=parent;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(address),reinterpret_cast<void*>(&ObserveConstructor),reinterpret_cast<void*>(parent))!=reinterpret_cast<void*>(parent))
        return Fail(6412,"native_dagger_points_constructor_install_conflict");
    slot->installed=true;++state.classInstalls;return true;
}
inline bool Restore(){
    if(state.callbacksActive||state.constructorsActive)return Fail(6413,"native_dagger_points_restore_during_original_callback");
    bool okay=true;
    for(auto& c:classes)if(c.installed){
        if(!Live(c.type,c.identity)){c.installed=false;continue;}
        auto address=static_cast<uint8_t*>(c.type)+0xb8;auto actual=Read<void*>(c.type,0xb8);
        if(actual==reinterpret_cast<void*>(c.parent)){c.installed=false;continue;}
        if(actual!=reinterpret_cast<void*>(&ObserveConstructor)||!PrivateSlot(address)
           ||InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(address),reinterpret_cast<void*>(c.parent),actual)!=actual){++state.conflicts;okay=false;continue;}
        c.installed=false;++state.classRestores;
    }
    for(auto& c:components)if(c.installed){
        if(!Live(c.component,c.identity)){c.installed=false;continue;}
        auto native=reinterpret_cast<void**>(engine::image+DamageTableRva),actual=Read<void**>(c.component);
        if(actual==native){c.installed=false;continue;}
        if(actual!=damageTable.data()+1||InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(c.component),native,actual)!=actual){++state.conflicts;okay=false;continue;}
        c.installed=false;++state.restores;
    }
    for(auto& a:actors)if(a.installed){
        if(!Live(a.actor,a.identity)){a.installed=false;continue;}
        auto native=reinterpret_cast<void**>(engine::image+ActorTableRva),actual=Read<void**>(a.actor);
        if(actual==native){a.installed=false;continue;}
        if(actual!=actorTable.data()+1||InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(a.actor),native,actual)!=actual){++state.conflicts;okay=false;continue;}
        a.installed=false;++state.restores;
    }
    if(!okay)return Fail(6414,"native_dagger_points_restore_conflict");
    state.bound=false;state.pawn=state.ability=state.playerState=nullptr;state.epoch=0;state.registration=0;return true;
}
inline bool InstallImpl(void* world,PointSink sink){
    if(!Thread()||state.closed||state.restartRequired||state.error)return false;
    const auto& a=local_server_abilities::GetState();
    if(!a.initialized||!a.host||!a.practice){
        if(state.bound&&!Restore())return false;
        state.disabled=true;return Wait("native_dagger_points_disabled_outside_practice_authority");
    }
    state.disabled=false;++state.installs;
    if(!world||a.world!=world)return Fail(6415,"native_dagger_points_current_authority_world_mismatch");
    if(!state.initialized){
        if(!Verify()||!Identity(world,state.worldIdentity))return Fail(6416,"native_dagger_points_pinned_native_profile_unknown");
        beginFunction=engine::Find(L"/Script/Engine.Actor.ReceiveBeginPlay");constructionFunction=engine::Find(L"/Script/Engine.Actor.UserConstructionScript");
        auto hit=engine::Find(L"/Script/ShooterGame.BaseProjectileEffectComponent.OnEntranceHit");
        if(!beginFunction||!constructionFunction||Read<uint16_t>(beginFunction,0xbe)!=0||Read<uint16_t>(constructionFunction,0xbe)!=0
           ||!hit||Read<uint16_t>(hit,0xbe)!=0x1e0||Read<void*>(hit,0x28)!=engine::Class(0x63d9160))
            return Fail(6417,"native_dagger_points_original_lifecycle_or_hit_abi_unknown");
        state.world=world;state.profilesVerified=true;state.initialized=true;PrepareTables();
    }
    if(world!=state.world||!Live(world,state.worldIdentity))return Fail(6418,"native_dagger_points_world_changed_requires_close");
    if(state.bound&&!Source()){if(!Restore())return false;}
    if(state.bound){
        if(!sink||sink!=expectedSink||sink!=local_server_abilities::pointSink)return Fail(6423,"native_dagger_points_existing_network_sink_changed");
        for(const auto& c:classes)if(c.installed&&(!Live(c.type,c.identity)||Read<void*>(c.type,0xb8)!=reinterpret_cast<void*>(&ObserveConstructor)))
            return Fail(6424,"native_dagger_points_installed_constructor_readback_changed");
        return Wait("native_dagger_points_early_observers_installed_execution_unverified");
    }
    if(!sink||sink!=local_server_abilities::pointSink)return Wait("native_dagger_points_waiting_existing_network_native_point_sink");
    local_server_abilities::Player* current=nullptr;
    for(const auto& candidate:local_server_abilities::GetPlayers())if(candidate.used&&candidate.authority&&candidate.sourceEpoch
        &&candidate.ready&&local_server_abilities::Source(candidate)){
        if(current)return Fail(6419,"native_dagger_points_multiple_authority_humans_unproven");
        current=local_server_abilities::Find(candidate.pawn);
    }
    if(!current)return Wait("native_dagger_points_waiting_actual_registered_ready_human");
    state.pawn=current->pawn;state.ability=current->skills[3].actor;state.playerState=current->playerState;
    state.pawnIdentity=current->pawnId;state.abilityIdentity=current->skills[3].identity;
    state.epoch=current->sourceEpoch;state.registration=current->registration;state.bound=true;expectedSink=sink;
    if(!Source()){state.bound=false;return Fail(6420,"native_dagger_points_registered_human_connection_source_unknown");}
    constexpr const wchar_t* arrays[]{L"ProjectileArray",L"BurstProjectileArray"};
    for(auto name:arrays){
        const auto offset=original_jett_priming::FieldOffset(Read<void*>(state.ability,0x10),name,16);
        if(offset<0)return Fail(6421,"native_dagger_points_original_authored_class_array_unknown");
        auto data=Read<void**>(state.ability,static_cast<size_t>(offset));const auto count=Read<int32_t>(state.ability,static_cast<size_t>(offset)+8);
        const auto capacity=Read<int32_t>(state.ability,static_cast<size_t>(offset)+12);
        if(count<1||count>static_cast<int32_t>(MaxClasses)||capacity<count||!original_players::Readable(data,static_cast<size_t>(count)*8))
            return Fail(6422,"native_dagger_points_original_authored_class_array_invalid");
        for(int32_t n=0;n<count;++n)if(!InstallClass(data[n])){Restore();return false;}
    }
    ++state.bindings;return Wait("native_dagger_points_early_observers_installed_execution_unverified");
}
// Invoke on the host GT after real abilities+combat registration and before
// accepting the next original ability input. Repeated calls follow real epochs.
inline bool Install(void* world,PointSink existingNetworkSink){
    __try{const bool okay=InstallImpl(world,existingNetworkSink);Cache();return okay;}__except(Capture(GetExceptionInformation())){return false;}
}
// Close before the corresponding ability/match teardown, or before world travel.
// Does not free callback backing, destroy actors, clear points or touch health.
inline bool Close(){
    __try{if(!Thread())return false;const bool okay=Restore();state.closed=true;state.stage=okay?"native_dagger_points_observers_restored":"native_dagger_points_restore_unverified";Cache();return okay;}__except(Capture(GetExceptionInformation())){return false;}
}
inline void Write(FILE* f){
    if(!f)return;const auto s=Snapshot();const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(f,"{\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"bound\":%s,\"disabled\":%s,\"profiles_verified\":%s,\"closed\":%s,\"restart_required\":%s,\"source_epoch\":%u,\"source_registration\":%llu",s.stage,s.error,b(s.initialized),b(s.bound),b(s.disabled),b(s.profilesVerified),b(s.closed),b(s.restartRequired),s.epoch,static_cast<unsigned long long>(s.registration));
    std::fprintf(f,",\"installs\":%llu,\"bindings\":%llu,\"class_installs\":%llu,\"class_restores\":%llu,\"constructors\":%llu,\"constructors_forwarded\":%llu,\"templates_preserved\":%llu,\"actor_observers\":%llu,\"actor_events_forwarded\":%llu,\"lifecycle_events\":%llu,\"component_observers\":%llu,\"entrances_forwarded\":%llu",static_cast<unsigned long long>(s.installs),static_cast<unsigned long long>(s.bindings),static_cast<unsigned long long>(s.classInstalls),static_cast<unsigned long long>(s.classRestores),static_cast<unsigned long long>(s.constructors),static_cast<unsigned long long>(s.constructorsForwarded),static_cast<unsigned long long>(s.templates),static_cast<unsigned long long>(s.actorInstalls),static_cast<unsigned long long>(s.actorEventsForwarded),static_cast<unsigned long long>(s.lifecycleEvents),static_cast<unsigned long long>(s.componentInstalls),static_cast<unsigned long long>(s.entrancesForwarded));
    std::fprintf(f,",\"source_deferrals\":%llu,\"source_rejections\":%llu,\"stale_callbacks\":%llu,\"missing_fire_boundary\":%llu,\"ambiguous_fire_boundary\":%llu,\"captures\":%llu,\"appended_points\":%llu,\"delivered_points\":%llu,\"no_new_points\":%llu,\"invalid_arrays\":%llu,\"restores\":%llu,\"conflicts\":%llu,\"exception_rva\":%llu",static_cast<unsigned long long>(s.sourceDeferrals),static_cast<unsigned long long>(s.sourceRejections),static_cast<unsigned long long>(s.staleCallbacks),static_cast<unsigned long long>(s.missingFireBoundary),static_cast<unsigned long long>(s.ambiguousFireBoundary),static_cast<unsigned long long>(s.captures),static_cast<unsigned long long>(s.appendedPoints),static_cast<unsigned long long>(s.deliveredPoints),static_cast<unsigned long long>(s.noNewPoints),static_cast<unsigned long long>(s.invalidArrays),static_cast<unsigned long long>(s.restores),static_cast<unsigned long long>(s.conflicts),static_cast<unsigned long long>(s.exceptionRva));
    std::fputs(",\"canonical_native_points_only\":true,\"health_applied_by_observer\":false,\"firing_replayed\":false,\"actor_lifetime_extended\":false,\"network_execution_verified\":false}",f);
}
}
