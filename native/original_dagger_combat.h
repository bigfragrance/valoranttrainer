#pragma once
#include "original_jett_world.h"
#include "training_combat_runtime.h"
#include "original_melee_combat.h"
#include "original_effects.h"
#include <array>
#include <limits>

// Standalone missing authority for REAL original Wushu dagger events. Original
// state callbacks, class arrays, FireProjectiles factory, transforms, random
// error, regional damage and complete FAresPointDamageEvent run unchanged.
// No gun observation is synthesized and no trajectory/health is written here.
namespace original_dagger_combat {
using Weak=original_inventory_slots::WeakIdentity;
using NativeEvent=void(*)(void*,void*,void*);
using NativeConstructor=void(*)(const void*);
using NativeEntranceCallback=void(*)(void*,const void*,const void*);
inline constexpr size_t MaxProjectiles=256,MaxTables=16,MaxFunctions=1024,QueueCapacity=256,EventSlot=0x298/8,EntranceSlot=0x4d8/8;
inline constexpr uintptr_t OriginalEvent=0x1cdc850,OriginalActorEvent=0x3eda0d0,DamageComponentClass=0x6708820,NativeEntrance=0x78c6d60;
inline constexpr uintptr_t ProjectileConstructor=0x6b2ce90;
inline constexpr const wchar_t* EntrancePath=L"/Script/ShooterGame.BaseProjectileEffectComponent.OnEntranceHit";
inline constexpr const wchar_t* FirePrefix=L"/Game/Characters/States/FireProjectileMulticast_StateComponent.FireProjectileMulticast_StateComponent_C.";
inline constexpr const wchar_t* XPrefix=L"/Game/Characters/Wushu/S0/Ability_X/Ability_Wushu_X_Dagger_Production.Ability_Wushu_X_Dagger_Production_C.";
inline constexpr const wchar_t* PrimaryDamagePath=L"/Game/Characters/Wushu/S0/Ability_X/DmgType_Wushu_X_Dagger.DmgType_Wushu_X_Dagger_C";
inline constexpr const wchar_t* BurstDamagePath=L"/Game/Characters/Wushu/S0/Ability_X/DmgType_Wushu_X_Dagger_Burst.DmgType_Wushu_X_Dagger_Burst_C";
struct Table {
    void** previous=nullptr;size_t count=0;NativeEvent parentEvent=nullptr;NativeEntranceCallback entranceParent=nullptr;
    void** nativeTable=nullptr;uint32_t profile=0;
    std::array<void*,MaxFunctions+1> copy{};void** Address(){return copy.data()+1;}
};
struct TableProbe {
    void *object=nullptr,*type=nullptr,*event=nullptr,*boundary=nullptr;void **actual=nullptr,**nativeTable=nullptr;
    size_t count=0;uint32_t profile=0;uintptr_t eventRva=0,tableRva=0;
    bool identityVerified=false,nativeImage=false,extentVerified=false,parentVerified=false;
    const char* reason="not_requested";
};
struct Projectile {
    bool used=false,installed=false;void *actor=nullptr,*component=nullptr;Weak actorId{},componentId{};
    Table* table=nullptr;uint32_t initializedBits=0;int32_t nextEvent=0;uint64_t callbacks=0;
};
struct ConstructorType {
    void* type=nullptr;Weak identity{};NativeConstructor parent=nullptr;bool installed=false;
    uint64_t entered=0,completed=0,templates=0,instances=0;
};
struct ActorWitness {
    void *actor=nullptr,*type=nullptr;Weak identity{};Table* table=nullptr;bool installed=false,hitObserverReady=false;
    uint64_t binding=0,callbacks=0;uint32_t epoch=0;
};
struct FireState {
    void* object=nullptr;Weak identity{};Table* table=nullptr;bool installed=false,enterBound=false,exitBound=false;
    void *projectileClass=nullptr,*classBefore=nullptr;uint64_t entries=0,classSupplies=0,bindingSupplies=0;
    uint64_t samples=0;void *world=nullptr,*equippable=nullptr,*pawn=nullptr,*playerState=nullptr,*playerInfo=nullptr,*classDefault=nullptr;
    void *graph=nullptr,*frame=nullptr,*frameGetter=nullptr,*transitionContext=nullptr;
    bool ownerQueriesValid=false,fieldsValid=false,frameValid=false,useStartTransform=false,useErrorValues=false,originalIsServer=false;
    uint8_t pooledClass=0,spawnMode=0;uint64_t fireHandle=0;double projectileDelay=0,errorPower=0;
    engine::Vec originalLocation{},originalDirection{};
    int32_t returnedActorCount=-1,returnedActorCapacity=-1,nativeDamageCount=-1,nativeDamageCapacity=-1;
    void **returnedActors=nullptr;void *firstReturnedActor=nullptr,*firstReturnedClass=nullptr,*firstDamageComponent=nullptr;
    uint32_t firstObjectFlags=0;int32_t firstInternalIndex=-1;float firstInitializedAt=0;
    bool contextValid=false,firstGetterMatches=false,firstActorLive=false,firstComponentLive=false;
    uint8_t firstReusable=0,firstPool=0,firstCleaned=0,firstStopAction=0;float firstDestroyDelay=0;
    uint64_t factoryResults=0,nonLiveFactoryActors=0,retainedNativePoints=0,importedNativePoints=0;
};
struct Contact {
    bool valid=false,unchangedNativeLife=false;uint32_t sourceEpoch=0,victimEpoch=0,initialBits=0;
    uint8_t originalProjectileKind=0; //1 actual primary class array,2 actual burst class array.
    size_t victim=32;uint64_t binding=0,token=0,hash=0;int32_t nativeIndex=0;double at=0;
    void *projectile=nullptr,*component=nullptr,*pawn=nullptr,*ability=nullptr,*victimPawn=nullptr,*damageType=nullptr;
    Weak projectileId{},componentId{},pawnId{},abilityId{},victimId{},damageTypeId{};
    std::array<uint8_t,0x130> point{};
    original_shots::Shot geometry{};original_damage::Hit hit{};
    float damage=0,falloff=0;
};
struct Seen {Weak actor{},component{};uint32_t init=0;int32_t index=0;uint64_t hash=0;};
struct CommitSeen {Weak state{},machine{};uint32_t epoch=0,timeBits=0;void* function=nullptr;};
struct Diagnostics {
    bool initialized=false,bound=false,ready=false,restartRequired=false,processing=false;
    unsigned error=0;uintptr_t exceptionRva=0;const char* stage="original_dagger_not_initialized";
    void *world=nullptr,*pawn=nullptr,*ability=nullptr,*resource=nullptr,*playerState=nullptr,*daggerClass=nullptr;
    uint32_t epoch=0;uint64_t binding=0,polls=0,binds=0,retirements=0,restores=0,tableConflicts=0;
    uint64_t projectilesSeen=0,observersInstalled=0,entranceCallbacks=0,nativePoints=0,polledPoints=0;
    uint64_t queued=0,duplicates=0,nativeLifeChanged=0,baselineUnavailable=0,invalidEvents=0,worldHits=0,nonCoreHits=0;
    uint64_t stale=0,processed=0,damageEvents=0,kills=0,queueOverflow=0,pendingDeferred=0;
    uint64_t primaryCommits=0,burstCommits=0,resourceCalls=0,resourceAlreadyConsumed=0;
    uint64_t dedicatedQueries=0;void* dedicatedWorld=nullptr;bool dedicatedResult=true;
    uint64_t duplicateCommits=0;
    uint64_t primaryKills=0,burstKills=0,unknownKindKills=0,primaryRefills=0,primaryAlreadyRefilled=0;
    int32_t killRefillBefore=-1,killRefillAfter=-1;
    int32_t resourceBefore=-1,resourceAfter=-1;uint64_t lastToken=0;
    int32_t originalDaggerIndex=-1,originalPrimaryClassCount=-1,originalBurstClassCount=-1;
    void *lastProjectileOwner=nullptr,*lastNativeInstigator=nullptr,*lastInstigatorInfo=nullptr,*lastExpectedInfo=nullptr;
    uint64_t ownerChainMatches=0,instigatorMatches=0,sourceRejected=0;
    size_t lastVictim=32;float lastDamage=0,lastFalloff=0;double lastPoll=-1;
    wchar_t failedEvent[512]{};uint16_t expectedBytes=0,actualBytes=0;
    void* failedScriptFunction=nullptr;int32_t expectedScriptBytes=0,actualScriptBytes=0;
    TableProbe observerProbe{},failedObserverProbe{};
    void *lastForwardObject=nullptr,*lastForwardParent=nullptr;void** lastForwardTable=nullptr;
    uint64_t originalForwards=0,actorForwards=0,componentForwards=0,effectsParentForwards=0,missingForwarders=0;
    uint64_t constructorEntered=0,constructorCompleted=0,constructorTemplates=0,constructorRestores=0;
    uint64_t actorCandidates=0,actorObserversInstalled=0,constructionEvents=0,beginPlayEvents=0,earlyHitObservers=0,lifecycleSourceRejected=0;
    void *lastConstructorType=nullptr,*lastConstructorParent=nullptr,*lastCandidateActor=nullptr,*lastEarlyComponent=nullptr;
    uint32_t constructorTypeCount=0;
    uint64_t nativeEntranceForwards=0;
};
inline Diagnostics diagnostics;
inline std::array<Table,MaxTables> tables{};
inline std::array<Projectile,MaxProjectiles> projectiles{};
inline std::array<ConstructorType,16> constructorTypes{};
inline std::array<ActorWitness,MaxProjectiles> actorWitnesses{};
inline std::array<FireState,2> fireStates{};
inline std::array<Contact,QueueCapacity> queue{};
inline std::array<Seen,512> seen{};
inline std::array<CommitSeen,64> commits{};inline size_t commitCursor=0;
inline std::array<original_melee_combat::LifeSnapshot,32> baseline{};
inline Weak pawnIdentity{},abilityIdentity{},resourceIdentity{};
inline Table* actorTable=nullptr;inline bool actorInstalled=false;
inline void *entranceFunction=nullptr,*fireEnterFunction=nullptr,*fireExitFunction=nullptr;
inline std::array<void*,5> primaryFunctions{};inline void* burstFunction=nullptr;
inline void* fireGraphFunction=nullptr;
inline void *actorBeginPlayFunction=nullptr,*actorConstructionFunction=nullptr;
inline uint64_t readIndex=0,writeIndex=0,seenIndex=0,nextBinding=0,worldSequence=0;
inline unsigned callbacksActive=0;
inline unsigned constructorsActive=0;
inline const Diagnostics& State(){return diagnostics;}
template<class T> inline T Read(const void* p,size_t at){return original_players::Value<T>(p,at);}
inline bool Live(void* p,const Weak& id){return p&&original_inventory_slots::ResolveIdentity(id)==p;}
inline bool Identity(void* p,Weak& id){return original_inventory_slots::CaptureIdentity(p,id)&&Live(p,id);}
inline bool Fail(unsigned code,const char* why,bool terminal=false){diagnostics.error=code;diagnostics.stage=why;diagnostics.ready=false;diagnostics.restartRequired|=terminal;return false;}
inline bool Thread(){return diagnostics.initialized&&!diagnostics.restartRequired&&original_players::Thread();}
inline double Now(){return static_cast<double>(GetTickCount64())/1000.;}
inline bool Event(void* object,const wchar_t* name,uint16_t bytes,void* parameters){
    auto fn=engine::Find(name);const auto actual=Read<uint16_t>(fn,0xbe);
    if(!object||!fn||actual!=bytes){wcsncpy_s(diagnostics.failedEvent,name,_TRUNCATE);diagnostics.expectedBytes=bytes;diagnostics.actualBytes=actual;return false;}
    return engine::Event(object,name,parameters);
}
inline void* Field(void* object,const wchar_t* wanted){
    const auto target=original_players::Utf8(wanted);auto type=Read<void*>(object,0x10);
    for(unsigned depth=0;type&&depth<128;++depth){
        auto field=Read<void*>(type,0x58);for(unsigned fields=0;field&&fields<4096;++fields){char name[128]{};original_weapon::OriginalNameValue(static_cast<uint8_t*>(field)+0x20,name);
            if(target==name)return field;auto next=Read<void*>(field,0x18);if(next==field)return nullptr;field=next;}
        auto query=Read<void*>(Read<void**>(type,0),0x2f8);if(!query)return nullptr;auto next=reinterpret_cast<void*(*)(void*)>(query)(type);if(next==type)return nullptr;type=next;
    }return nullptr;
}
inline bool Object(void* object,const wchar_t* name,void*& value){
    value=nullptr;auto field=Field(object,name);const auto offset=Read<int32_t>(field,0x44),size=Read<int32_t>(Read<void*>(object,0x10),0x60);
    if(!field||Read<int32_t>(field,0x34)!=8||offset<0x30||size<offset+8||size>2*1024*1024)return false;
    value=Read<void*>(object,static_cast<size_t>(offset));return true;
}
inline void* StructField(void* type,const wchar_t* name,size_t bytes){
    original_ability_tuning::NativeName native{};
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&native,name,1);
    auto field=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,&native,3);
    const auto offset=Read<int32_t>(field,0x44),size=Read<int32_t>(type,0x60);
    return field&&(Read<uint64_t>(Read<void*>(field,8),0x18)&0x8001)==0x8001
        &&Read<uint64_t>(field,0x20)==original_dagger_combat::Read<uint64_t>(&native,0)&&Read<int32_t>(field,0x30)==1
        &&Read<int32_t>(field,0x34)==static_cast<int32_t>(bytes)&&offset>=0&&size>=0
        &&static_cast<size_t>(offset)+bytes<=static_cast<size_t>(size)?field:nullptr;
}
inline bool FieldBytes(void* data,void* type,const wchar_t* name,void* out,size_t bytes){
    auto field=StructField(type,name,bytes);if(!field)return false;
    const auto offset=static_cast<size_t>(Read<int32_t>(field,0x44));
    if(!original_players::Readable(static_cast<uint8_t*>(data)+offset,bytes))return false;
    std::memcpy(out,static_cast<uint8_t*>(data)+offset,bytes);return true;
}
inline bool FieldBool(void* data,void* type,const wchar_t* name,bool& out){
    auto field=StructField(type,name,1);if(!field||(Read<uint64_t>(Read<void*>(field,8),0x18)&0x28001)!=0x28001
       ||Read<uint8_t>(field,0x78)!=1||Read<uint8_t>(field,0x79)!=0)return false;
    const auto mask=Read<uint8_t>(field,0x7b);if(!mask)return false;
    out=(Read<uint8_t>(data,static_cast<size_t>(Read<int32_t>(field,0x44)))&mask)!=0;return true;
}
inline bool Source(){
    const auto p=deathmatch_rules::Player(0);
    return diagnostics.bound&&p&&p->registered&&p->pawn==diagnostics.pawn&&p->spawnEpoch==diagnostics.epoch
        &&Live(diagnostics.pawn,pawnIdentity)&&Live(diagnostics.ability,abilityIdentity)&&Live(diagnostics.resource,resourceIdentity)
        &&Read<void*>(diagnostics.ability,0x4a8)==diagnostics.pawn&&Read<void*>(diagnostics.resource,0xc8)==diagnostics.ability;
}
inline bool LocalProjectile(void* actor){
    if(!Source()||!actor||!diagnostics.daggerClass||!original_players::Derived(Read<void*>(actor,0x10),diagnostics.daggerClass))return false;
    void* ps=nullptr;if(!Event(actor,L"/Script/ShooterGame.Projectile.GetFiringPlayerState",8,&ps)||ps!=diagnostics.playerState)return false;
    auto owner=Read<void*>(actor,0x210);diagnostics.lastProjectileOwner=owner;
    for(unsigned depth=0;owner&&depth<8;++depth){if(owner==diagnostics.ability||owner==diagnostics.pawn){++diagnostics.ownerChainMatches;return true;}
        auto next=Read<void*>(owner,0x210);if(next==owner)break;owner=next;}
    // FireMultipleProjectiles receives the actual FiringEquippable, and its
    // native78ce15f path resolves that item's GetPawnOwner. Owner may differ
    // in pooled/native actors: accept only independently verified original
    // Actor.GetInstigator==this SAME Pawn plus the real original player-info
    // link. The same PS surviving respawn alone is never enough.
    diagnostics.lastNativeInstigator=diagnostics.lastInstigatorInfo=diagnostics.lastExpectedInfo=nullptr;
    if(Event(actor,L"/Script/Engine.Actor.GetInstigator",8,&diagnostics.lastNativeInstigator)
       &&diagnostics.lastNativeInstigator==diagnostics.pawn
       &&Event(actor,L"/Script/ShooterGame.Projectile.GetInstigatorPlayerInfo",8,&diagnostics.lastInstigatorInfo)
       &&Event(diagnostics.pawn,L"/Script/ShooterGame.ShooterCharacter.GetOwnerExclusivePlayerInfo",8,&diagnostics.lastExpectedInfo)
       &&diagnostics.lastExpectedInfo&&diagnostics.lastExpectedInfo==diagnostics.lastInstigatorInfo){++diagnostics.instigatorMatches;return true;}
    ++diagnostics.sourceRejected;return false;
}
inline LONG Capture(EXCEPTION_POINTERS* e){
    const auto at=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    diagnostics.exceptionRva=at>=engine::image?at-engine::image:at;
    deathmatch_rules::CancelNativeDamage();Fail(e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0003400U,"original_dagger_exception_requires_new_session",true);return EXCEPTION_EXECUTE_HANDLER;
}
inline void ObserveEvent(void*,void*,void*);
inline void ObserveEntrance(void*,const void*,const void*);
inline bool NativeTable(void** table){
    MEMORY_BASIC_INFORMATION memory{};
    return table&&VirtualQuery(table,&memory,sizeof(memory))&&memory.State==MEM_COMMIT&&memory.Type==MEM_IMAGE
        &&memory.AllocationBase==reinterpret_cast<void*>(engine::image)&&!(memory.Protect&(PAGE_GUARD|PAGE_NOACCESS));
}
inline uintptr_t NativeRva(const void* address){
    MEMORY_BASIC_INFORMATION memory{};
    return address&&VirtualQuery(address,&memory,sizeof(memory))&&memory.State==MEM_COMMIT&&memory.Type==MEM_IMAGE
        &&memory.AllocationBase==reinterpret_cast<void*>(engine::image)?reinterpret_cast<uintptr_t>(address)-engine::image:0;
}
inline bool RejectTable(const char* reason){
    diagnostics.observerProbe.reason=reason;diagnostics.failedObserverProbe=diagnostics.observerProbe;return false;
}
inline bool OriginalEffectsParent(void* object,void** actual,size_t count,TableProbe& proof){
    // This is the sole admitted non-native parent. The FX observer actually
    // owns manager COMPONENTS, not the ability Actor. Verify its live record,
    // process-lifetime shared allocation, and every cloned virtual; matching
    // its entry address alone never permits an arbitrary foreign table.
    for(const auto& entry:original_effects::managers){
        if(entry.component!=object||!entry.observerInstalled||entry.observedTable!=actual)continue;
        if(original_effects::ResolveIdentity(entry.componentIdentity)!=object
           ||original_effects::ResolveIdentity(entry.actorIdentity)!=entry.actor||!entry.actor
           ||Read<void*>(object,0xc8)!=entry.actor)return RejectTable("effects_parent_weak_identity_or_owner_mismatch");
        size_t profile=2;
        for(size_t i=0;i<2;++i)if(original_effects::observerTables[i].observed==actual
            &&original_effects::observerTables[i].original==entry.originalTable){profile=i;break;}
        if(profile==2)return RejectTable("effects_parent_shared_table_not_owned");
        const auto native=reinterpret_cast<void**>(engine::image+(profile?0xb332af0:0xb21fc90));
        const size_t expected=profile?170U:165U;
        if(entry.originalTable!=native||entry.tableEntries!=expected||count!=expected
           ||entry.originalEvent!=engine::function<original_effects::ProcessEvent>(OriginalEvent)
           ||!original_players::IsA(object,profile?original_effects::LocationalClass:original_effects::ManagerClass)
           ||!NativeTable(native)||!original_players::Readable(native-1,(expected+1)*8)
           ||native[EventSlot]!=engine::function<void*>(OriginalEvent))return RejectTable("effects_parent_native_profile_mismatch");
        if(actual[-1]!=native[-1])return RejectTable("effects_parent_rtti_mismatch");
        for(size_t i=0;i<expected;++i){void* wanted=native[i];
            if(i==EventSlot)wanted=reinterpret_cast<void*>(&original_effects::ObserveEvent);
            else if(i==0x3e0/8)wanted=reinterpret_cast<void*>(&original_effects::ObserveBeginPlay);
            else if(i==0x508/8)wanted=profile?reinterpret_cast<void*>(&original_effects::ObserveLocationalStartDeferred)
                :reinterpret_cast<void*>(&original_effects::ObserveBaseStartDeferred);
            if(actual[i]!=wanted)return RejectTable("effects_parent_full_clone_mismatch");}
        proof.nativeTable=native;proof.profile=3;proof.parentVerified=true;return true;
    }
    return RejectTable("effects_parent_live_manager_record_missing");
}
inline bool InspectParent(void* object,void** actual,size_t count){
    auto& proof=diagnostics.observerProbe;Weak identity{};proof.identityVerified=Identity(object,identity);
    if(!proof.identityVerified)return RejectTable("object_weak_identity_unavailable");
    proof.extentVerified=count>EventSlot&&count<MaxFunctions&&original_players::Readable(actual-1,(count+1)*8);
    if(!proof.extentVerified)return RejectTable(count==MaxFunctions?"table_extent_scan_limit":"table_extent_or_readability_mismatch");
    proof.event=actual[EventSlot];proof.nativeImage=NativeTable(actual);
    if(proof.nativeImage){proof.eventRva=reinterpret_cast<uintptr_t>(proof.event)>=engine::image
            ?reinterpret_cast<uintptr_t>(proof.event)-engine::image:0;
        proof.tableRva=reinterpret_cast<uintptr_t>(actual)-engine::image;proof.nativeTable=actual;
        // Actor3eda0d0 checks the original world/function conditions before
        // forwarding to UObject1cdc850. Preserve it for the actual X Actor.
        if(proof.event==engine::function<void*>(OriginalActorEvent)
           &&original_players::Derived(proof.type,engine::Find(L"/Script/Engine.Actor"))){proof.profile=2;proof.parentVerified=true;return true;}
        if(proof.event==engine::function<void*>(OriginalEvent)
           &&original_players::Derived(proof.type,engine::Find(L"/Script/Engine.ActorComponent"))){proof.profile=1;proof.parentVerified=true;return true;}
        return RejectTable("native_parent_event_or_object_type_profile_mismatch");
    }
    if(proof.event==reinterpret_cast<void*>(&original_effects::ObserveEvent))return OriginalEffectsParent(object,actual,count,proof);
    return RejectTable("foreign_parent_table_or_event_not_admitted");
}
inline Table* SharedTable(void* object){
    auto original=Read<void**>(object,0);diagnostics.observerProbe={};auto& proof=diagnostics.observerProbe;
    proof.object=object;proof.type=Read<void*>(object,0x10);proof.actual=original;proof.event=Read<void*>(original,EventSlot*8);
    proof.eventRva=NativeRva(proof.event);proof.tableRva=NativeRva(original);proof.nativeImage=NativeTable(original);
    // Re-entering a previously installed own table still requires the exact
    // immutable copy/parent relationship. New objects sharing its parent are
    // independently admitted below, including their actual weak identities.
    for(auto& t:tables)if(t.previous&&t.Address()==original){
        proof.count=t.count;proof.nativeTable=t.nativeTable;proof.profile=t.profile;
        Weak identity{};proof.identityVerified=Identity(object,identity);
        if(!proof.identityVerified||original[EventSlot]!=reinterpret_cast<void*>(&ObserveEvent)||!t.parentEvent
           ||t.previous[EventSlot]!=reinterpret_cast<void*>(t.parentEvent)) {RejectTable("own_table_parent_relationship_changed");return nullptr;}
        proof.parentVerified=true;proof.extentVerified=true;proof.reason="existing_owned_clone";return &t;}
    size_t count=0;MEMORY_BASIC_INFORMATION region{};uintptr_t begin=0,end=0;
    for(;count<MaxFunctions;++count){auto entry=Read<void*>(original,count*8);const auto at=reinterpret_cast<uintptr_t>(entry);if(!at)break;
        if(at<begin||at>=end){if(!VirtualQuery(entry,&region,sizeof(region))||region.State!=MEM_COMMIT||(region.Protect&(PAGE_GUARD|PAGE_NOACCESS)))break;
            const auto flags=region.Protect&0xffU;if(flags!=PAGE_EXECUTE&&flags!=PAGE_EXECUTE_READ&&flags!=PAGE_EXECUTE_READWRITE&&flags!=PAGE_EXECUTE_WRITECOPY)break;
            begin=reinterpret_cast<uintptr_t>(region.BaseAddress);end=begin+region.RegionSize;}}
    proof.count=count;proof.boundary=Read<void*>(original,count*8);if(!InspectParent(object,original,count))return nullptr;
    const bool observeEntrance=original_players::IsA(object,DamageComponentClass);
    if(observeEntrance&&(count<=EntranceSlot||original[EntranceSlot]!=engine::function<void*>(NativeEntrance))){RejectTable("native_damage_entrance_profile_mismatch");return nullptr;}
    for(auto& t:tables)if(t.previous==original){
        if(t.count!=count||reinterpret_cast<void*>(t.parentEvent)!=proof.event||t.profile!=proof.profile
           ||observeEntrance!=(t.entranceParent!=nullptr)){RejectTable("cached_parent_profile_changed");return nullptr;}
        proof.reason="verified_shared_parent";return &t;}
    Table* result=nullptr;for(auto& t:tables)if(!t.previous){result=&t;break;}
    if(!result){RejectTable("shared_table_capacity_exhausted");return nullptr;}
    std::memcpy(result->copy.data(),original-1,(count+1)*8);result->copy[1+EventSlot]=reinterpret_cast<void*>(ObserveEvent);
    if(observeEntrance){result->entranceParent=reinterpret_cast<NativeEntranceCallback>(original[EntranceSlot]);result->copy[1+EntranceSlot]=reinterpret_cast<void*>(&ObserveEntrance);}
    result->count=count;result->parentEvent=reinterpret_cast<NativeEvent>(proof.event);result->nativeTable=proof.nativeTable;result->profile=proof.profile;
    MemoryBarrier();result->previous=original;proof.reason="verified_parent_cloned";return result;
}
inline bool Install(void* object,Table*& table,bool& installed){
    if(installed)return table&&Read<void**>(object,0)==table->Address();table=SharedTable(object);if(!table)return false;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(object),table->Address(),table->previous)!=table->previous)return RejectTable("object_table_changed_during_install");
    installed=true;
    if(Read<void**>(object,0)!=table->Address()||Read<void*>(table->Address(),EventSlot*8)!=reinterpret_cast<void*>(&ObserveEvent)
       ||Read<void*>(table->previous,EventSlot*8)!=reinterpret_cast<void*>(table->parentEvent))return RejectTable("installed_clone_or_parent_readback_mismatch");
    if(table->entranceParent&&(Read<void*>(table->Address(),EntranceSlot*8)!=reinterpret_cast<void*>(&ObserveEntrance)
       ||Read<void*>(table->previous,EntranceSlot*8)!=reinterpret_cast<void*>(table->entranceParent)))return RejectTable("installed_native_entrance_or_parent_readback_mismatch");
    return true;
}
inline bool Restore(void* object,const Weak& identity,Table* table,bool& installed){
    if(!installed)return true;if(!Live(object,identity)){installed=false;return true;}
    auto current=Read<void**>(object,0);if(current==table->previous){installed=false;return true;}
    if(current!=table->Address()||InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(object),table->previous,table->Address())!=table->Address()){
        ++diagnostics.tableConflicts;return Fail(3401,"original_dagger_foreign_observer_restore_conflict",true);}
    installed=false;++diagnostics.restores;return true;
}
inline bool Array(Projectile& p,uint8_t*& data,int32_t& count){
    data=Read<uint8_t*>(p.component,0x370);count=Read<int32_t>(p.component,0x378);const auto capacity=Read<int32_t>(p.component,0x37c);
    return count>=0&&count<=8192&&capacity>=count&&(!count||original_players::Readable(data,static_cast<size_t>(count)*0x130));
}
inline Projectile* Track(void* actor){
    auto component=original_weapon::Component(actor,DamageComponentClass);Weak actorId{},componentId{};
    if(!component||Read<void*>(component,0xc8)!=actor||!Identity(actor,actorId)||!Identity(component,componentId))return nullptr;
    Projectile* empty=nullptr;
    for(auto& p:projectiles){if(p.used&&!Live(p.component,p.componentId))p={};
        if(p.used&&p.component==component&&p.componentId.index==componentId.index&&p.componentId.serial==componentId.serial)return &p;
        if(!p.used&&!empty)empty=&p;}
    if(!empty){Fail(3402,"original_dagger_projectile_cache_full",true);return nullptr;}
    auto table=Read<void**>(component,0);auto original=table;for(auto& t:tables)if(t.Address()==table)original=t.previous;
    if(Read<void*>(original,0x4d8)!=engine::function<void*>(NativeEntrance)){Fail(3403,"original_dagger_native_hit_entry_changed",true);return nullptr;}
    auto& p=*empty;p.used=true;p.actor=actor;p.component=component;p.actorId=actorId;p.componentId=componentId;
    if(!Install(component,p.table,p.installed)){p={};Fail(3404,"original_dagger_hit_observer_install_failed",true);return nullptr;}
    ++diagnostics.observersInstalled;return &p;
}
inline ActorWitness* Witness(void* actor){
    for(auto& witness:actorWitnesses)if(witness.actor==actor&&Live(actor,witness.identity))return &witness;
    return nullptr;
}
inline bool ObserveConstructedActor(void* actor,ConstructorType& type){
    if(!Source()||!Live(type.type,type.identity)||Read<void*>(actor,0x10)!=type.type)return true;
    if((Read<uint32_t>(actor,8)&0x30U)!=0){++type.templates;++diagnostics.constructorTemplates;return true;}
    ++type.instances;++diagnostics.actorCandidates;diagnostics.lastCandidateActor=actor;
    Weak identity{};if(!Identity(actor,identity))return Fail(3437,"original_dagger_new_projectile_weak_identity_unavailable",true);
    ActorWitness* available=nullptr;
    for(auto& witness:actorWitnesses){
        if(witness.actor&&!Live(witness.actor,witness.identity))witness={};
        if(witness.actor==actor&&original_inventory_slots::SameIdentity(witness.identity,identity))return true;
        if(!witness.actor&&!available)available=&witness;}
    if(!available)return Fail(3438,"original_dagger_actor_observer_capacity_exhausted",true);
    auto& witness=*available;witness.actor=actor;witness.type=type.type;witness.identity=identity;
    witness.binding=diagnostics.binding;witness.epoch=diagnostics.epoch;
    // The original full native Projectile constructor has already returned.
    // Observe this instance only; no CDO/vtable image or class flag is changed.
    if(!Install(actor,witness.table,witness.installed))return Fail(3439,"original_dagger_actor_lifecycle_observer_install_failed",true);
    ++diagnostics.actorObserversInstalled;return true;
}
inline void ObserveConstructedActorSafe(void* actor,ConstructorType& type){
    __try{ObserveConstructedActor(actor,type);}__except(Capture(GetExceptionInformation())){}
}
__declspec(noinline) inline void ObserveConstructor(const void* initializer){
    auto actor=Read<void*>(initializer,0);auto type=Read<void*>(actor,0x10);ConstructorType* entry=nullptr;
    for(auto& candidate:constructorTypes)if(candidate.type==type){entry=&candidate;break;}
    // Every admitted ClassConstructor is this exact original FObjectInitializer
    // thunk, including late callbacks whose observation record no longer lives.
    const auto parent=entry&&entry->parent?entry->parent:engine::function<NativeConstructor>(ProjectileConstructor);
    const bool ownedThread=GetCurrentThreadId()==original_players::gameThread;
    if(ownedThread){++constructorsActive;++diagnostics.constructorEntered;if(entry)++entry->entered;}
    __try{parent(initializer);}__finally{if(ownedThread)--constructorsActive;}
    if(ownedThread){++diagnostics.constructorCompleted;if(entry){++entry->completed;ObserveConstructedActorSafe(Read<void*>(initializer,0),*entry);}}
}
inline bool PrivateWritableSlot(void* slot){
    MEMORY_BASIC_INFORMATION memory{};if(!slot||!VirtualQuery(slot,&memory,sizeof(memory)))return false;
    const auto start=reinterpret_cast<uintptr_t>(slot),base=reinterpret_cast<uintptr_t>(memory.BaseAddress);
    return memory.State==MEM_COMMIT&&memory.Type==MEM_PRIVATE&&(memory.Protect&0xffU)==PAGE_READWRITE
        &&!(memory.Protect&(PAGE_GUARD|PAGE_NOACCESS))&&start>=base&&start+8<=base+memory.RegionSize;
}
inline bool ObserveConstructorType(void* type){
    diagnostics.lastConstructorType=type;diagnostics.lastConstructorParent=Read<void*>(type,0xb8);
    Weak identity{};
    if(!type||!original_players::Derived(type,diagnostics.daggerClass)||Read<int32_t>(type,0x60)<0x5e0||!Identity(type,identity))
        return Fail(3440,"original_dagger_constructor_type_profile_mismatch",true);
    ConstructorType* available=nullptr;
    for(auto& candidate:constructorTypes){
        if(candidate.type==type&&original_inventory_slots::SameIdentity(candidate.identity,identity)){available=&candidate;break;}
        if(!candidate.type&&!available)available=&candidate;}
    if(!available)return Fail(3441,"original_dagger_constructor_type_capacity_exhausted",true);
    if(available->installed)return Read<void*>(type,0xb8)==reinterpret_cast<void*>(&ObserveConstructor)
        ?true:Fail(3442,"original_dagger_constructor_observer_replaced",true);
    auto slot=static_cast<uint8_t*>(type)+0xb8;
    const auto parent=engine::function<NativeConstructor>(ProjectileConstructor);
    if(Read<void*>(type,0xb8)!=reinterpret_cast<void*>(parent)||!PrivateWritableSlot(slot))
        return Fail(3443,"original_dagger_original_constructor_or_private_slot_mismatch",true);
    available->type=type;available->identity=identity;available->parent=parent;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(slot),reinterpret_cast<void*>(&ObserveConstructor),reinterpret_cast<void*>(parent))!=reinterpret_cast<void*>(parent))
        return Fail(3444,"original_dagger_constructor_changed_during_install",true);
    available->installed=true;++diagnostics.constructorTypeCount;return true;
}
inline bool ObserveOriginalConstructorTypes(){
    // Only the actual authored classes in this exact X's original arrays are
    // admitted. Bind1b96db0 inherits the native Projectile6b2ce90 initializer
    // thunk at ClassConstructor+b8; any different actual pointer is rejected.
    constexpr const wchar_t* names[]{L"ProjectileArray",L"BurstProjectileArray"};
    for(auto name:names){auto field=Field(diagnostics.ability,name);
        if(!field||Read<int32_t>(field,0x34)!=16)return Fail(3445,"original_dagger_original_constructor_class_array_missing",true);
        const auto offset=Read<int32_t>(field,0x44),size=Read<int32_t>(Read<void*>(diagnostics.ability,0x10),0x60);
        if(offset<0x30||offset+16>size)return Fail(3445,"original_dagger_original_constructor_class_array_missing",true);
        auto array=static_cast<uint8_t*>(diagnostics.ability)+offset;auto data=Read<void**>(array,0);
        const auto count=Read<int32_t>(array,8),capacity=Read<int32_t>(array,12);
        if(count<=0||count>64||capacity<count||!original_players::Readable(data,static_cast<size_t>(count)*8))
            return Fail(3446,"original_dagger_original_constructor_class_array_invalid",true);
        for(int32_t n=0;n<count;++n)if(!ObserveConstructorType(data[n]))return false;}
    return true;
}
inline bool RestoreConstructorTypes(){
    for(auto& type:constructorTypes)if(type.installed){
        if(!Live(type.type,type.identity)){type.installed=false;continue;}
        auto slot=static_cast<uint8_t*>(type.type)+0xb8;auto current=Read<void*>(type.type,0xb8);
        if(current==reinterpret_cast<void*>(type.parent)){type.installed=false;continue;}
        if(current!=reinterpret_cast<void*>(&ObserveConstructor)||!PrivateWritableSlot(slot)
           ||InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(slot),reinterpret_cast<void*>(type.parent),reinterpret_cast<void*>(&ObserveConstructor))!=reinterpret_cast<void*>(&ObserveConstructor))
            return Fail(3447,"original_dagger_constructor_retirement_conflict",true);
        type.installed=false;++diagnostics.constructorRestores;}
    return true;
}
inline void AttachEarlyHitObserver(ActorWitness& witness){
    if(witness.hitObserverReady||!Source()||witness.binding!=diagnostics.binding||witness.epoch!=diagnostics.epoch
       ||!Live(witness.actor,witness.identity))return;
    auto table=Read<void**>(witness.actor,0);auto getWorld=Read<void*>(table,0x1b8);
    if(!getWorld||reinterpret_cast<void*(*)(void*)>(getWorld)(witness.actor)!=diagnostics.world||!LocalProjectile(witness.actor)){
        ++diagnostics.lifecycleSourceRejected;return;}
    auto projectile=Track(witness.actor);if(!projectile)return;
    witness.hitObserverReady=true;diagnostics.lastEarlyComponent=projectile->component;++diagnostics.earlyHitObservers;
}
inline uint64_t Hash(const uint8_t* bytes,size_t size){uint64_t h=14695981039346656037ULL;for(size_t i=0;i<size;++i){h^=bytes[i];h*=1099511628211ULL;}return h;}
inline bool SameSeen(const Seen& s,const Projectile& p,uint32_t bits,int32_t index,uint64_t hash){return s.actor.index==p.actorId.index&&s.actor.serial==p.actorId.serial
    &&s.component.index==p.componentId.index&&s.component.serial==p.componentId.serial&&s.init==bits&&s.index==index&&s.hash==hash;}
inline bool ArrayContainsClass(const wchar_t* name,void* type){
    auto field=Field(diagnostics.ability,name);if(!field||Read<int32_t>(field,0x34)!=16)return false;
    const auto offset=Read<int32_t>(field,0x44),size=Read<int32_t>(Read<void*>(diagnostics.ability,0x10),0x60);
    if(offset<0x30||size<offset+16||size>2*1024*1024)return false;
    auto array=static_cast<uint8_t*>(diagnostics.ability)+offset;auto data=Read<void**>(array,0);const auto count=Read<int32_t>(array,8),cap=Read<int32_t>(array,12);
    if(count<0||count>64||cap<count||(count&&!original_players::Readable(data,static_cast<size_t>(count)*8)))return false;
    for(int32_t i=0;i<count;++i)if(data[i]==type)return true;return false;
}
inline bool QueuePoint(Projectile& p,const uint8_t* native,int32_t index,bool immediate,const original_melee_combat::LifeSnapshot* before){
    if(!LocalProjectile(p.actor)||!original_players::Readable(native,0x130)){++diagnostics.invalidEvents;return true;}
    const auto initialized=Read<float>(p.actor,0x518);uint32_t bits=0;std::memcpy(&bits,&initialized,4);if(!std::isfinite(initialized))return true;
    const auto hash=Hash(native,0x130);if(!immediate)for(const auto& s:seen)if(SameSeen(s,p,bits,index,hash)){++diagnostics.duplicates;return true;}
    auto victim=engine::function<void*(*)(const void*)>(0x437b290)(native+0x30+0xb8);size_t victimIndex=32;
    for(size_t i=0;i<32;++i){const auto player=deathmatch_rules::Player(i);if(player&&player->registered&&player->pawn==victim){victimIndex=i;break;}}
    if(victimIndex==32){++diagnostics.worldHits;return true;}
    const auto& life=before?*before:baseline[victimIndex];const auto target=deathmatch_rules::Player(victimIndex);
    if(!life.valid||life.pawn!=victim||!target||target->spawnEpoch!=life.epoch){++diagnostics.baselineUnavailable;return true;}
    if(!original_melee_combat::SameLife(life)){++diagnostics.nativeLifeChanged;return true;}
    Weak componentId=Read<Weak>(native,0x30+0xd0);auto hitComponent=original_inventory_slots::ResolveIdentity(componentId);
    if(hitComponent!=original_damage::targets[victimIndex].core){++diagnostics.nonCoreHits;return true;}
    Contact c;c.valid=c.unchangedNativeLife=true;c.binding=diagnostics.binding;c.sourceEpoch=diagnostics.epoch;c.victimEpoch=life.epoch;c.victim=victimIndex;
    const auto actualClass=Read<void*>(p.actor,0x10);const bool primary=ArrayContainsClass(L"ProjectileArray",actualClass),burst=ArrayContainsClass(L"BurstProjectileArray",actualClass);
    c.originalProjectileKind=primary&&!burst?uint8_t(1):burst&&!primary?uint8_t(2):uint8_t(0);
    c.projectile=p.actor;c.component=p.component;c.projectileId=p.actorId;c.componentId=p.componentId;c.initialBits=bits;c.nativeIndex=index;c.hash=hash;
    c.pawn=diagnostics.pawn;c.ability=diagnostics.ability;c.pawnId=pawnIdentity;c.abilityId=abilityIdentity;c.victimPawn=victim;
    c.at=Now();c.damageType=Read<void*>(native,8);c.damage=Read<float>(native,0x10);c.falloff=Read<float>(native,0x124);
    // The actual burst package references DmgType_Wushu_X_Dagger_Burst. Also
    // inspect this ORIGINAL event field, so a burst fragment reusing a class
    // can never be credited as a primary-kill resource reset.
    auto burstType=engine::Find(BurstDamagePath),primaryType=engine::Find(PrimaryDamagePath);
    if(burstType&&original_players::Derived(c.damageType,burstType))c.originalProjectileKind=2;
    else if(c.originalProjectileKind==1&&(!primaryType||!original_players::Derived(c.damageType,primaryType)))c.originalProjectileKind=0;
    c.hit.hit=true;c.hit.blocking=(native[0x30+0xad]&1U)!=0;c.hit.region=native[0x120];c.hit.component=hitComponent;
    c.hit.distance=Read<float>(native,0x38);c.hit.location=Read<engine::Vec>(native,0x30+0x28);c.hit.normal=Read<engine::Vec>(native,0x30+0x58);
    std::memcpy(c.hit.bone.bytes,native+0x30+0xd8,12);std::memcpy(c.hit.nativeHit,native+0x30,0xf0);std::memcpy(c.point.data(),native,0x130);
    c.geometry.observed_at=c.at;c.geometry.start=Read<engine::Vec>(p.actor,0x570);c.geometry.direction=Read<engine::Vec>(native,0x18);
    const auto length=c.geometry.direction.x*c.geometry.direction.x+c.geometry.direction.y*c.geometry.direction.y+c.geometry.direction.z*c.geometry.direction.z;
    if(!Identity(victim,c.victimId)||!Identity(c.damageType,c.damageTypeId)||!original_players::Derived(c.damageType,engine::Class(original_damage::DamageTypeClass))
       ||!std::isfinite(c.damage)||c.damage<=0||c.damage>1000000||!std::isfinite(c.falloff)||c.falloff<0||c.hit.region>2
       ||!original_players::Finite(c.geometry.start)||!original_players::Finite(c.geometry.direction)||!original_players::Finite(c.hit.location)
       ||!original_players::Finite(c.hit.normal)||!std::isfinite(c.hit.distance)||c.hit.distance<0||length<.99||length>1.01){++diagnostics.invalidEvents;return true;}
    if(writeIndex-readIndex>=queue.size()){++diagnostics.queueOverflow;return Fail(3405,"original_dagger_damage_queue_full",true);}
    if(training_combat_runtime::nextDamageToken==(std::numeric_limits<uint64_t>::max)())return Fail(3406,"original_dagger_shared_token_exhausted",true);
    c.token=++training_combat_runtime::nextDamageToken;c.geometry.sequence=c.token;
    seen[seenIndex++%seen.size()]={p.actorId,p.componentId,bits,index,hash};queue[writeIndex++%queue.size()]=c;++diagnostics.queued;return true;
}
inline bool DelegateHas(void* state,const wchar_t* fieldName,void* function,void*& address){
    address=nullptr;auto field=Field(state,fieldName);const auto offset=Read<int32_t>(field,0x44),size=Read<int32_t>(Read<void*>(state,0x10),0x60);
    const auto bytes=Read<int32_t>(field,0x34);
    // Native ScriptState OnStateEnter1f8 / OnStateExit208 are16B delegates;
    // some cooked Blueprint inline declarations reserve24B. Both store the
    // same native invocation-list TArray at0 and native20B script entries.
    if(!field||(bytes!=16&&bytes!=24)||offset<0x30||size<offset+bytes||size>2*1024*1024)return false;
    address=static_cast<uint8_t*>(state)+offset;auto data=Read<uint8_t*>(address,0);const auto count=Read<int32_t>(address,8),cap=Read<int32_t>(address,12);
    if(count<0||count>256||cap<count||(count&&!original_players::Readable(data,static_cast<size_t>(count)*20))){address=nullptr;return false;}
    for(int32_t i=0;i<count;++i){auto item=data+static_cast<size_t>(i)*20;
        if(original_inventory_slots::ResolveIdentity(Read<Weak>(item,0))==state
           &&std::memcmp(item+8,static_cast<uint8_t*>(function)+0x18,12)==0)return true;}
    return false;
}
inline bool EnsureOriginalDelegate(FireState& s,const wchar_t* fieldName,void* function,bool& bound){
    void* address=nullptr;bound=DelegateHas(s.object,fieldName,function,address);if(bound)return true;
    if(!address||!function)return Fail(3407,"original_dagger_original_state_delegate_layout_unavailable");
    // Exact native FScriptDelegate20B and Add used by the shipped BeginPlay:
    // weak UObject0, original FName8, native1cf4e70 stride20 (no custom event).
    struct Delegate {Weak object;uint8_t name[12]{};} delegate{s.identity};static_assert(sizeof(Delegate)==20);
    std::memcpy(delegate.name,static_cast<uint8_t*>(function)+0x18,12);
    engine::function<void(*)(void*,const void*)>(0x1cf4e70)(address,&delegate);
    bound=DelegateHas(s.object,fieldName,function,address);if(bound)++s.bindingSupplies;
    return bound?true:Fail(3408,"original_dagger_original_state_delegate_not_read_back");
}
inline bool Script(void* function,void* owner,int32_t bytes,uint16_t parameters){
    auto data=Read<uint8_t*>(function,0x68);
    const bool verified=function&&owner&&Read<void*>(function,0x28)==owner&&Read<uint16_t>(function,0xbe)==parameters
        &&Read<int32_t>(function,0x70)==bytes&&original_players::Readable(data,static_cast<size_t>(bytes))&&data[bytes-1]==0x53;
    if(!verified){diagnostics.failedScriptFunction=function;diagnostics.expectedScriptBytes=bytes;diagnostics.actualScriptBytes=Read<int32_t>(function,0x70);
        diagnostics.expectedBytes=parameters;diagnostics.actualBytes=Read<uint16_t>(function,0xbe);}
    return verified;
}
inline bool OriginalClassForFire(FireState& s,size_t which){
    auto index=Field(diagnostics.ability,L"CurrentDaggerIndex");
    if(index&&Read<int32_t>(index,0x34)==4)diagnostics.originalDaggerIndex=Read<int32_t>(diagnostics.ability,static_cast<size_t>(Read<int32_t>(index,0x44)));
    constexpr const wchar_t* arrays[]{L"ProjectileArray",L"BurstProjectileArray"};
    for(size_t i=0;i<2;++i){auto field=Field(diagnostics.ability,arrays[i]);int32_t count=-1;
        if(field&&Read<int32_t>(field,0x34)==16)count=Read<int32_t>(diagnostics.ability,static_cast<size_t>(Read<int32_t>(field,0x44))+8);
        if(i)diagnostics.originalBurstClassCount=count;else diagnostics.originalPrimaryClassCount=count;}
    if(!Object(s.object,L"ProjectileClass",s.projectileClass))return Fail(3409,"original_dagger_fire_state_class_field_missing");
    s.classBefore=s.projectileClass;if(s.projectileClass)return original_players::Derived(s.projectileClass,diagnostics.daggerClass);
    // Only in the real fire-state OnStateEnter callback. The authored X method
    // selects from its own actual ProjectileArray/BurstProjectileArray using
    // its own dagger index. No class, index or projectile tuning is invented.
    const auto name=std::wstring(XPrefix)+(which?L"UpdateDaggerClassForIndexBurst":L"UpdateDaggerClassForIndex");
    auto function=engine::Find(name.c_str());auto owner=Read<void*>(diagnostics.ability,0x10);
    if(!Script(function,owner,177,8)||!Event(diagnostics.ability,name.c_str(),8,&s.object))return Fail(3410,"original_dagger_authored_projectile_class_selection_unavailable");
    ++s.classSupplies;
    if(!Object(s.object,L"ProjectileClass",s.projectileClass)||!s.projectileClass
       ||!original_players::Derived(s.projectileClass,diagnostics.daggerClass))return Fail(3411,"original_dagger_authored_projectile_array_or_index_not_ready");
    return true;
}
inline void SampleFire(FireState& s){
    if(!Source()||!Live(s.object,s.identity))return;++s.samples;
    const auto type=Read<void*>(s.object,0x10);const auto table=Read<void**>(s.object,0);
    const auto getter=Read<void*>(table,0x1b8);
    s.world=getter?reinterpret_cast<void*(*)(void*)>(getter)(s.object):nullptr;
    s.equippable=s.pawn=s.playerState=s.playerInfo=nullptr;
    s.ownerQueriesValid=Event(s.object,L"/Script/ShooterGame.EquippableStateComponent.GetOwningEquippable",8,&s.equippable)
        &&Event(s.object,L"/Script/ShooterGame.EquippableStateComponent.GetOwningShooterCharacter",8,&s.pawn)
        &&s.equippable==diagnostics.ability&&s.pawn==diagnostics.pawn
        &&Event(s.pawn,L"/Script/ShooterGame.ShooterCharacter.GetShooterPlayerState",8,&s.playerState)
        &&Event(s.pawn,L"/Script/ShooterGame.ShooterCharacter.GetOwnerExclusivePlayerInfo",8,&s.playerInfo);
    s.fieldsValid=FieldBytes(s.object,type,L"ProjectileDelay",&s.projectileDelay,8)
        &&FieldBytes(s.object,type,L"ErrorPower",&s.errorPower,8)
        &&FieldBytes(s.object,type,L"FireProjectileHandle",&s.fireHandle,8)
        &&FieldBool(s.object,type,L"UseStartTransform",s.useStartTransform)
        &&FieldBool(s.object,type,L"UseErrorValues",s.useErrorValues);
    Object(s.object,L"ProjectileClass",s.projectileClass);
    s.classDefault=s.projectileClass?engine::DefaultObject(s.projectileClass):nullptr;
    if(s.classDefault){s.pooledClass=Read<uint8_t>(s.classDefault,0x514);s.spawnMode=Read<uint8_t>(s.classDefault,0x524);}
    s.graph=fireGraphFunction;s.frame=nullptr;s.frameValid=false;
    const auto graphOwner=Read<void*>(s.graph,0x28);const auto classTable=Read<void**>(graphOwner,0);
    s.frameGetter=Read<void*>(classTable,0x3c0);
    // Read the actual base-class persistent frame using the same ordinary BP
    // getter proven for E. ManualError's derived override has no own graph.
    if(s.frameGetter!=engine::function<void*>(0x40c0c80)||Read<void*>(graphOwner,0x298)!=s.graph)return;
    const auto frameField=Read<void*>(graphOwner,0x290);const auto offset=Read<int32_t>(frameField,0x44);
    if(!frameField||offset<0x30||offset+8>Read<int32_t>(type,0x60))return;
    s.frame=reinterpret_cast<void*(*)(void*,void*,void*)>(s.frameGetter)(graphOwner,s.object,s.graph);
    if(!s.frame||s.frame!=Read<void*>(s.object,static_cast<size_t>(offset))
       ||!original_players::Readable(s.frame,static_cast<size_t>(Read<int32_t>(s.graph,0x60))))return;
    s.frameValid=FieldBytes(s.frame,s.graph,L"CallFunc_SetNewContext_ReturnValue",&s.transitionContext,8)
        &&FieldBytes(s.frame,s.graph,L"CallFunc_GetFiringLocationAndDirection_FiringLocation",&s.originalLocation,24)
        &&FieldBytes(s.frame,s.graph,L"CallFunc_GetFiringLocationAndDirection_FiringDirection",&s.originalDirection,24)
        &&FieldBool(s.frame,s.graph,L"CallFunc_IsServer_ReturnValue",s.originalIsServer);
}
inline bool CaptureFactoryResult(FireState& s,const std::array<original_melee_combat::LifeSnapshot,32>& lives){
    s.contextValid=s.firstGetterMatches=s.firstActorLive=s.firstComponentLive=false;
    s.returnedActors=nullptr;s.firstReturnedActor=s.firstReturnedClass=s.firstDamageComponent=nullptr;
    s.returnedActorCount=s.returnedActorCapacity=s.nativeDamageCount=s.nativeDamageCapacity=-1;
    if(!s.frameValid||!s.transitionContext||!original_players::IsA(s.transitionContext,0x6d271c0))return true;
    Weak contextIdentity{};if(!Identity(s.transitionContext,contextIdentity))return true;
    s.returnedActors=Read<void**>(s.transitionContext,0x38);
    s.returnedActorCount=Read<int32_t>(s.transitionContext,0x40);s.returnedActorCapacity=Read<int32_t>(s.transitionContext,0x44);
    s.contextValid=s.returnedActorCount>=0&&s.returnedActorCount<=1024&&s.returnedActorCapacity>=s.returnedActorCount
        &&(!s.returnedActorCount||original_players::Readable(s.returnedActors,static_cast<size_t>(s.returnedActorCount)*8));
    if(!s.contextValid)return true;
    void* first=nullptr;
    if(!Event(s.transitionContext,L"/Script/ShooterGame.ActorListTransitionContext.GetFirstActor",8,&first))
        return Fail(3435,"original_dagger_original_factory_context_getter_abi_failed");
    s.firstGetterMatches=first==(s.returnedActorCount?s.returnedActors[0]:nullptr);
    if(!s.firstGetterMatches)return Fail(3436,"original_dagger_original_factory_context_readback_mismatch");
    for(int32_t n=0;n<s.returnedActorCount;++n){auto actor=s.returnedActors[n];
        if(!actor||!original_players::Readable(actor,0x5d2)||!original_players::Derived(Read<void*>(actor,0x10),diagnostics.daggerClass))continue;
        ++s.factoryResults;Weak actorIdentity{},componentIdentity{};
        const bool actorLive=Identity(actor,actorIdentity);
        // The context's original strong Actor list retains the actual return
        // value even after immediate native hitscan cleanup excludes it from
        // GetAllActors. Inspect only; never respawn/reinitialize/retrace it.
        auto component=original_weapon::Component(actor,DamageComponentClass);
        const bool componentLive=Identity(component,componentIdentity);
        uint8_t* points=nullptr;int32_t count=-1,capacity=-1;
        if(component&&Read<void*>(component,0xc8)==actor&&original_players::Readable(component,0x380)){
            points=Read<uint8_t*>(component,0x370);count=Read<int32_t>(component,0x378);capacity=Read<int32_t>(component,0x37c);
            if(count<0||count>8192||capacity<count||(count&&!original_players::Readable(points,static_cast<size_t>(count)*0x130)))count=-1;}
        if(n==0){s.firstReturnedActor=actor;s.firstReturnedClass=Read<void*>(actor,0x10);s.firstDamageComponent=component;
            s.firstObjectFlags=Read<uint32_t>(actor,8);s.firstInternalIndex=Read<int32_t>(actor,0xc);
            s.firstActorLive=actorLive;s.firstComponentLive=componentLive;s.nativeDamageCount=count;s.nativeDamageCapacity=capacity;
            s.firstInitializedAt=Read<float>(actor,0x518);s.firstReusable=Read<uint8_t>(actor,0x513);s.firstPool=Read<uint8_t>(actor,0x514);
            s.firstStopAction=Read<uint8_t>(actor,0x525);s.firstDestroyDelay=Read<float>(actor,0x528);s.firstCleaned=Read<uint8_t>(actor,0x5d1);}
        if(count>0)s.retainedNativePoints+=static_cast<uint64_t>(count);
        // A copied complete native event may outlive its original projectile;
        // admission still requires the real weak identities at capture time.
        // Pending-kill-only returns are diagnosed, never blindly resurrected.
        if(!actorLive||!componentLive){++s.nonLiveFactoryActors;continue;}
        if(count<=0)continue;auto tracked=Track(actor);if(!tracked){if(diagnostics.restartRequired)return false;continue;}
        for(int32_t i=0;i<count;++i){auto point=points+static_cast<size_t>(i)*0x130;
            auto victim=engine::function<void*(*)(const void*)>(0x437b290)(point+0x30+0xb8);
            const original_melee_combat::LifeSnapshot* before=nullptr;
            for(const auto& life:lives)if(life.valid&&life.pawn==victim){before=&life;break;}
            if(!before){++diagnostics.baselineUnavailable;continue;}
            const auto queued=diagnostics.queued;
            if(!QueuePoint(*tracked,point,i,false,before))return false;
            s.importedNativePoints+=diagnostics.queued-queued;
        }
        tracked->nextEvent=count;
    }
    return true;
}
inline bool Remaining(int32_t& value){return Event(diagnostics.resource,original_jett::DaggerRemaining,4,&value)&&value>=0&&value<=64;}
inline void ConsumeAfterOriginal(bool burst,int32_t before){
    if(!Source()||before<=0)return;int32_t after=-1;if(!Remaining(after)){Fail(3412,"original_dagger_resource_getter_failed");return;}
    diagnostics.resourceBefore=before;diagnostics.resourceAfter=after;
    if(after!=before){++diagnostics.resourceAlreadyConsumed;return;}
    // Original reflected ParmsSize is9: WorldContextObject0/8B, bool return8.
    // The C++ storage may include tail padding; ProcessEvent copies only the
    // original9-byte parameter block. Supplying a standalone bool silently
    // skipped every missing-authority consume in the previous revision.
    struct DedicatedQuery{void* world;uint8_t result=1;} query{diagnostics.world};
    static_assert(offsetof(DedicatedQuery,result)==8);
    diagnostics.dedicatedWorld=query.world;++diagnostics.dedicatedQueries;
    if(!Event(engine::DefaultObject(engine::Class(0x45aa110)),L"/Script/Engine.KismetSystemLibrary.IsDedicatedServer",9,&query)){
        Fail(3432,"original_dagger_original_dedicated_query_abi_or_call_failed");return;}
    diagnostics.dedicatedResult=query.result!=0;
    if(query.result){Fail(3433,"original_dagger_local_authority_requires_standalone_world");return;}
    const auto path=std::wstring(original_jett::DaggerResourcePath)+(burst?L".ConsumeAllDaggers":L".ConsumeDagger");
    auto fn=engine::Find(path.c_str());auto type=Read<void*>(diagnostics.resource,0x10);
    if(!Script(fn,type,burst?55:196,0)||!Event(diagnostics.resource,path.c_str(),0,nullptr)){
        Fail(3413,"original_dagger_original_dedicated_resource_leaf_missing");return;}
    ++diagnostics.resourceCalls;if(!Remaining(diagnostics.resourceAfter)){Fail(3412,"original_dagger_resource_getter_failed");return;}
    if(diagnostics.resourceAfter>=before)Fail(3434,"original_dagger_original_consume_resource_readback_unchanged");
}
inline Projectile* FindProjectile(void* component){for(auto& p:projectiles)if(p.used&&p.component==component&&Live(component,p.componentId))return &p;return nullptr;}
inline FireState* FindFire(void* object,size_t& which){for(size_t i=0;i<fireStates.size();++i)if(fireStates[i].object==object&&Live(object,fireStates[i].identity)){which=i;return &fireStates[i];}return nullptr;}
inline bool PrimaryCallback(void* function){
    for(auto original:primaryFunctions)if(function==original)return true;return false;
}
inline bool BurstCallback(void* function){return function==burstFunction;}
struct Before {
    Projectile* projectile=nullptr;FireState* fire=nullptr;ActorWitness* lifecycle=nullptr;bool capture=false,primary=false,burst=false;
    int32_t count=0,remaining=-1;original_melee_combat::LifeSnapshot life;
    std::array<original_melee_combat::LifeSnapshot,32> fireLives{};
};
inline bool FreshCommit(void* state,void* function){
    auto machine=Read<void*>(state,0xe0);const auto time=Read<float>(machine,0x1e0);Weak stateId{},machineId{};
    if(!original_players::IsA(machine,0x67722c0)||!std::isfinite(time)||!Identity(state,stateId)||!Identity(machine,machineId)){
        Fail(3427,"original_dagger_original_commit_time_identity_unavailable");return false;}
    uint32_t bits=0;std::memcpy(&bits,&time,4);
    for(const auto& c:commits)if(c.function==function&&c.epoch==diagnostics.epoch&&c.timeBits==bits
        &&c.state.index==stateId.index&&c.state.serial==stateId.serial&&c.machine.index==machineId.index&&c.machine.serial==machineId.serial){++diagnostics.duplicateCommits;return false;}
    commits[commitCursor++%commits.size()]={stateId,machineId,diagnostics.epoch,bits,function};return true;
}
inline void Prepare(void* object,void* function,void* parameters,Before& before){
    if(!Thread()||!Source())return;
    if(auto witness=Witness(object);witness&&Read<uint16_t>(function,0xbe)==0){
        // Blueprint overrides retain the original native Actor event's FName.
        // Match the exact full native12B name, not a guessed package suffix.
        const bool begin=actorBeginPlayFunction&&std::memcmp(static_cast<uint8_t*>(function)+0x18,static_cast<uint8_t*>(actorBeginPlayFunction)+0x18,12)==0;
        const bool construction=actorConstructionFunction&&std::memcmp(static_cast<uint8_t*>(function)+0x18,static_cast<uint8_t*>(actorConstructionFunction)+0x18,12)==0;
        if(begin||construction){before.lifecycle=witness;++witness->callbacks;
            if(begin)++diagnostics.beginPlayEvents;else ++diagnostics.constructionEvents;}
    }
    if(object==diagnostics.ability&&Live(object,abilityIdentity)&&Read<void*>(function,0x28)==Read<void*>(object,0x10)
       &&Read<uint16_t>(function,0xbe)==20&&parameters){
        auto entered=Read<void*>(parameters,0);
        if(Read<void*>(entered,0xc8)==diagnostics.ability){before.primary=PrimaryCallback(function);before.burst=BurstCallback(function);
            if((before.primary||before.burst)&&!FreshCommit(entered,function))before.primary=before.burst=false;
            if(before.primary||before.burst){if(!Remaining(before.remaining))before.remaining=-1;
                if(before.primary)++diagnostics.primaryCommits;else ++diagnostics.burstCommits;}}
    }
    size_t which=0;if(auto fire=FindFire(object,which);fire&&function==fireEnterFunction&&parameters){
        if(Read<void*>(parameters,0)==object){++fire->entries;before.fire=fire;
            for(size_t i=0;i<before.fireLives.size();++i)original_melee_combat::ReadLife(i,before.fireLives[i]);
            OriginalClassForFire(*fire,which);}
    }
    auto p=FindProjectile(object);if(!p||function!=entranceFunction||!parameters||!Live(p->actor,p->actorId)||!LocalProjectile(p->actor))return;
    // The native virtual wrapper below also sees native C++ direct calls.
    // When a reflected invocation reaches that same virtual, capture there
    // only, so ProcessEvent cannot enqueue the same appended point twice.
    if(p->table&&p->table->entranceParent)return;
    ++p->callbacks;++diagnostics.entranceCallbacks;uint8_t* data=nullptr;
    if(!Array(*p,data,before.count)||!original_players::Readable(parameters,0x1e0))return;
    auto victim=engine::function<void*(*)(const void*)>(0x437b290)(static_cast<uint8_t*>(parameters)+0xf0+0xb8);
    for(size_t i=0;i<32;++i){const auto player=deathmatch_rules::Player(i);
        if(player&&player->registered&&player->pawn==victim){before.capture=original_melee_combat::ReadLife(i,before.life);break;}}
    before.projectile=p;
}
inline void PrepareSafe(void* object,void* function,void* parameters,Before& before){__try{Prepare(object,function,parameters,before);}__except(Capture(GetExceptionInformation())){}}
inline void After(Before& before){
    if(!Thread())return;
    if(before.lifecycle)AttachEarlyHitObserver(*before.lifecycle);
    if(before.fire){SampleFire(*before.fire);if(!CaptureFactoryResult(*before.fire,before.fireLives))return;}
    if(before.primary||before.burst)ConsumeAfterOriginal(before.burst,before.remaining);
    auto p=before.projectile;if(!p||!before.capture||!Live(p->component,p->componentId)||!Live(p->actor,p->actorId))return;
    uint8_t* data=nullptr;int32_t count=0;if(!Array(*p,data,count))return;
    if(count<before.count){p->nextEvent=0;return;}
    for(int32_t i=before.count;i<count;++i){++diagnostics.nativePoints;if(!QueuePoint(*p,data+static_cast<size_t>(i)*0x130,i,true,&before.life))return;}
    p->nextEvent=count;
}
inline void AfterSafe(Before& before){__try{After(before);}__except(Capture(GetExceptionInformation())){}}
inline Table* ForwardTable(void* object){
    const auto actual=Read<void**>(object,0);
    for(auto& t:tables)if(t.previous&&t.Address()==actual)return &t;
    // A caller may already have loaded our callback just before retirement
    // restored its admitted parent table. Shared backing is never released.
    for(auto& t:tables)if(t.previous&&t.previous==actual)return &t;
    return nullptr;
}
inline void PrepareEntrance(void* object,const void* entrance,Before& before){
    if(!Thread()||!Source()||!original_players::Readable(entrance,0xf0))return;
    auto p=FindProjectile(object);if(!p||!Live(p->actor,p->actorId)||!LocalProjectile(p->actor))return;
    ++p->callbacks;++diagnostics.entranceCallbacks;uint8_t* data=nullptr;
    if(!Array(*p,data,before.count))return;
    auto victim=engine::function<void*(*)(const void*)>(0x437b290)(static_cast<const uint8_t*>(entrance)+0xb8);
    for(size_t i=0;i<32;++i){const auto player=deathmatch_rules::Player(i);
        if(player&&player->registered&&player->pawn==victim){before.capture=original_melee_combat::ReadLife(i,before.life);break;}}
    before.projectile=p;
}
inline void PrepareEntranceSafe(void* object,const void* entrance,Before& before){
    __try{PrepareEntrance(object,entrance,before);}__except(Capture(GetExceptionInformation())){}
}
__declspec(noinline) inline void ObserveEntrance(void* object,const void* lastHit,const void* entrance){
    auto table=ForwardTable(object);auto parent=table?table->entranceParent:nullptr;
    if(!parent){if(GetCurrentThreadId()==original_players::gameThread)Fail(3449,"original_dagger_native_entrance_parent_missing",true);return;}
    if(GetCurrentThreadId()!=original_players::gameThread){parent(object,lastHit,entrance);return;}
    Before before;PrepareEntranceSafe(object,entrance,before);++callbacksActive;++diagnostics.nativeEntranceForwards;
    // Exact original DamageProjectileEffectComponent virtual4d8 ABI: this in
    // RCX, unmodified LastHit240 in RDX and EntranceHit240 in R8. The retained
    // native78c6d60 body uses R8 and creates the complete original304B point.
    __try{parent(object,lastHit,entrance);}__finally{--callbacksActive;}
    AfterSafe(before);
}
__declspec(noinline) inline void ObserveEvent(void* object,void* function,void* parameters){
    const auto table=ForwardTable(object);const auto parent=table?table->parentEvent:nullptr;
    if(!parent){if(GetCurrentThreadId()==original_players::gameThread){++diagnostics.missingForwarders;Fail(3431,"original_dagger_admitted_parent_forwarder_missing",true);}return;}
    // Every branch forwards the captured, admitted parent exactly once. In
    // particular Actor events retain3eda0d0's original world/function checks;
    // an owned FX parent retains all of its own lifecycle/event observation.
    if(GetCurrentThreadId()!=original_players::gameThread){parent(object,function,parameters);return;}
    diagnostics.lastForwardObject=object;diagnostics.lastForwardTable=table->previous;
    diagnostics.lastForwardParent=reinterpret_cast<void*>(parent);++diagnostics.originalForwards;
    if(table->profile==2)++diagnostics.actorForwards;else if(table->profile==3)++diagnostics.effectsParentForwards;else ++diagnostics.componentForwards;
    Before before;PrepareSafe(object,function,parameters,before);++callbacksActive;
    __try{parent(object,function,parameters);}__finally{--callbacksActive;}
    AfterSafe(before);
}
inline bool ObserveProjectiles(){
    if(!diagnostics.daggerClass)diagnostics.daggerClass=engine::Find(original_jett::ProjectilePaths[1]);if(!diagnostics.daggerClass)return true;
    struct Query{void* world;void* type;void** actors=nullptr;int32_t count=0,capacity=0;} q{diagnostics.world,diagnostics.daggerClass};
    if(!Event(engine::DefaultObject(engine::Class(0x43a9670)),L"/Script/Engine.GameplayStatics.GetAllActorsOfClass",32,&q))return false;
    const bool valid=q.count>=0&&q.count<=65536&&q.capacity>=q.count&&(!q.count||original_players::Readable(q.actors,static_cast<size_t>(q.count)*8));
    bool result=valid;
    if(valid)for(int32_t n=0;n<q.count;++n){auto p=Track(q.actors[n]);if(!p){if(diagnostics.restartRequired){result=false;break;}continue;}
        ++diagnostics.projectilesSeen;if(!LocalProjectile(p->actor))continue;
        const auto time=Read<float>(p->actor,0x518);uint32_t bits=0;std::memcpy(&bits,&time,4);
        if(!std::isfinite(time))continue;if(bits!=p->initializedBits){p->initializedBits=bits;p->nextEvent=0;}
        uint8_t* data=nullptr;int32_t count=0;if(!Array(*p,data,count))continue;if(count<p->nextEvent)p->nextEvent=0;
        for(int32_t i=p->nextEvent;i<count;++i){++diagnostics.polledPoints;if(!QueuePoint(*p,data+static_cast<size_t>(i)*0x130,i,false,nullptr)){result=false;break;}}
        p->nextEvent=count;if(!result)break;}
    if(q.actors)engine::function<void(*)(void*)>(0x1915720)(q.actors);return result;
}
inline bool DrainWorldRing(){
    const auto maximum=original_jett_world::State().daggerNativeEvents;
    if(maximum<worldSequence)worldSequence=0;
    if(maximum-worldSequence>original_jett_world::EventCapacity)worldSequence=maximum-original_jett_world::EventCapacity;
    while(worldSequence<maximum){const auto event=original_jett_world::EventAt(++worldSequence);if(!event||event->sourcePawn!=diagnostics.pawn||event->sourceEquippable!=diagnostics.ability)continue;
        Weak id{event->identity.index,event->identity.serial};if(!Live(event->projectile,id))continue;auto p=Track(event->projectile);if(!p)continue;
        uint8_t* data=nullptr;int32_t count=0;if(!Array(*p,data,count)||event->nativeEventIndex<0||event->nativeEventIndex>=count)continue;
        auto native=data+static_cast<size_t>(event->nativeEventIndex)*0x130;
        if(std::memcmp(native+0x30,event->nativeHit.data(),0xf0)!=0)continue;
        if(!QueuePoint(*p,native,event->nativeEventIndex,false,nullptr))return false;}
    return true;
}
inline bool RefillActualPrimaryKill(){
    if(!Remaining(diagnostics.killRefillBefore))return Fail(3428,"original_dagger_primary_kill_resource_readback_failed");
    auto field=Field(diagnostics.resource,L"MaxDaggers");if(!field||Read<int32_t>(field,0x34)!=4)return Fail(3429,"original_dagger_original_max_daggers_missing");
    const auto maximum=Read<int32_t>(diagnostics.resource,static_cast<size_t>(Read<int32_t>(field,0x44)));
    if(maximum<=0||maximum>64)return Fail(3429,"original_dagger_original_max_daggers_invalid");
    if(diagnostics.killRefillBefore==maximum){diagnostics.killRefillAfter=maximum;++diagnostics.primaryAlreadyRefilled;return true;}
    // Original OnAuthGenerateKillEffects merely EnableDaggerReset; its missing
    // authority notification is not supplied by RecordKill. Clear that pending
    // original reset through its own original method before replenishing, so a
    // delayed reset cannot grant a second unconsumed knife on the next commit.
    const auto disable=std::wstring(original_jett::DaggerResourcePath)+L".DisableDaggerReset";
    auto fn=engine::Find(disable.c_str());auto type=Read<void*>(diagnostics.resource,0x10);
    if(!Script(fn,type,14,0)||!Event(diagnostics.resource,disable.c_str(),0,nullptr)
       ||!Event(diagnostics.resource,original_jett::DaggerReplenish,0,nullptr)||!Remaining(diagnostics.killRefillAfter)
       ||diagnostics.killRefillAfter!=maximum)return Fail(3430,"original_dagger_primary_kill_original_replenish_not_observed");
    ++diagnostics.primaryRefills;return true;
}
inline bool Apply(Contact& c,double now,bool& deferred){
    deferred=false;const auto victim=deathmatch_rules::Player(c.victim);
    if(!Source()||c.binding!=diagnostics.binding||c.sourceEpoch!=diagnostics.epoch||!c.valid||!c.unchangedNativeLife
       ||!victim||!victim->registered||victim->spawnEpoch!=c.victimEpoch||victim->pawn!=c.victimPawn
       ||!Live(c.victimPawn,c.victimId)||!Live(c.damageType,c.damageTypeId)||!Live(c.pawn,c.pawnId)||!Live(c.ability,c.abilityId)
       ||now-c.at>1.0){++diagnostics.stale;return true;}
    // Root may pass the tick's start clock after a same-tick native callback.
    // Wait for the next monotonic poll rather than discarding that real hit.
    if(now<c.at){deferred=true;++diagnostics.pendingDeferred;return true;}
    if(!deathmatch_rules::CanDamage(0,c.victim))return true;
    if(!original_damage::Sample(c.victim))return false;
    if(original_damage::targets[c.victim].pendingDamageEvents){deferred=true;++diagnostics.pendingDeferred;return true;}
    original_damage::DeathData death;if(!deathmatch_rules::BeginNativeDamage(c.token,0,c.victim,c.ability,now,death))return false;
    original_damage::Result result;
    // Damage+10 is ALREADY the original final amount: native78c738e/7393/7397
    // multiplies the original distance/region factors before78c74e5 stores it.
    // Falloff124 is copied as original metadata, never multiplied a second time.
    const bool applied=original_damage::ApplyPoint(c.victim,c.pawn,c.ability,c.damageType,c.geometry,c.hit,c.damage,c.token,result,&death,c.falloff);
    const bool committed=deathmatch_rules::EndNativeDamage(applied,result);
    if(!applied||!committed)return Fail(!applied?original_damage::State().error:deathmatch_rules::State().error,
        !applied?original_damage::State().stage:deathmatch_rules::State().stage,true);
    if(result.applied){++diagnostics.damageEvents;diagnostics.lastToken=c.token;diagnostics.lastVictim=c.victim;diagnostics.lastDamage=c.damage;diagnostics.lastFalloff=c.falloff;}
    if(result.killed){++diagnostics.kills;
        if(c.originalProjectileKind==1){++diagnostics.primaryKills;if(!RefillActualPrimaryKill())return false;}
        else if(c.originalProjectileKind==2)++diagnostics.burstKills;else ++diagnostics.unknownKindKills;
        if(!original_melee_combat::BeforeRetire(c.victim)
       ||!training_combat_runtime::BeforeRetire(c.victim,training_combat_runtime::RetireReason::NativeDeath))return false;}
    return true;
}
inline bool InitializeImpl(void* world){
    if(diagnostics.initialized)return diagnostics.world==world&&Thread();
    if(!original_players::Thread()||!world||!deathmatch_rules::State().initialized||deathmatch_rules::world!=world
       ||!training_combat_runtime::State().initialized)return Fail(3414,"original_dagger_requires_original_v2_match");
    entranceFunction=engine::Find(EntrancePath);
    if(!entranceFunction||Read<uint16_t>(entranceFunction,0xbe)!=0x1e0||Read<void*>(entranceFunction,0x28)!=engine::Class(0x63d9160))
        return Fail(3415,"original_dagger_entrance_hit_abi_changed");
    const auto enter=std::wstring(FirePrefix)+L"StateEnterEvent",exit=std::wstring(FirePrefix)+L"OnStateExit_Event_0";
    fireEnterFunction=engine::Find(enter.c_str());fireExitFunction=engine::Find(exit.c_str());
    auto owner=Read<void*>(fireEnterFunction,0x28);
    const auto firing=engine::Find((std::wstring(FirePrefix)+L"FireProjectiles").c_str());
    const auto graph=engine::Find((std::wstring(FirePrefix)+L"ExecuteUbergraph_FireProjectileMulticast_StateComponent").c_str());fireGraphFunction=graph;
    if(!Script(fireEnterFunction,owner,72,20)||!Script(fireExitFunction,owner,54,16)
       ||!Script(firing,owner,1004,64)||!Script(graph,owner,1134,4))return Fail(3416,"original_dagger_original_firing_script_profile_changed");
    actorBeginPlayFunction=engine::Find(L"/Script/Engine.Actor.ReceiveBeginPlay");
    actorConstructionFunction=engine::Find(L"/Script/Engine.Actor.UserConstructionScript");
    if(!actorBeginPlayFunction||!actorConstructionFunction||Read<uint16_t>(actorBeginPlayFunction,0xbe)!=0||Read<uint16_t>(actorConstructionFunction,0xbe)!=0)
        return Fail(3448,"original_dagger_original_actor_lifecycle_event_profile_missing");
    diagnostics.world=world;diagnostics.daggerClass=engine::Find(original_jett::ProjectilePaths[1]);diagnostics.initialized=true;
    diagnostics.stage="original_dagger_original_interfaces_verified";return true;
}
inline bool BindImpl(void* pawn,void* ability,uint32_t epoch){
    if(!Thread()||!pawn||!ability||!epoch)return false;
    if(diagnostics.bound)return diagnostics.pawn==pawn&&diagnostics.ability==ability&&diagnostics.epoch==epoch&&Source();
    const auto player=deathmatch_rules::Player(0);
    if(!player||!player->local||player->pawn!=pawn||player->spawnEpoch!=epoch||ability!=original_jett::skills[3].actor
       ||original_jett::State().pawn!=pawn||Read<void*>(ability,0x4a8)!=pawn)return Fail(3417,"original_dagger_bind_requires_actual_local_x");
    diagnostics.pawn=pawn;diagnostics.ability=ability;diagnostics.resource=original_jett::skills[3].resource;diagnostics.playerState=player->playerState;diagnostics.epoch=epoch;
    if(!Identity(pawn,pawnIdentity)||!Identity(ability,abilityIdentity)||!Identity(diagnostics.resource,resourceIdentity))return Fail(3418,"original_dagger_source_weak_identity_missing");
    // These names exceed the old128-byte debug-name helper. Resolve their
    // exact original UFunction identities once instead of truncating names.
    constexpr const wchar_t* primary[]{
        L"BndEvt__Ability_Wushu_X_Dagger_Production_Projectile0CommitState_K2Node_ComponentBoundEvent_9_OnStateEnterSignature__DelegateSignature",
        L"BndEvt__Ability_Wushu_X_Dagger_Production_Projectile1CommitState_K2Node_ComponentBoundEvent_10_OnStateEnterSignature__DelegateSignature",
        L"BndEvt__Ability_Wushu_X_Dagger_Production_Projectile2CommitState_K2Node_ComponentBoundEvent_13_OnStateEnterSignature__DelegateSignature",
        L"BndEvt__Ability_Wushu_X_Dagger_Production_Projectile3CommitState_K2Node_ComponentBoundEvent_14_OnStateEnterSignature__DelegateSignature",
        L"BndEvt__Ability_Wushu_X_Dagger_Production_Projectile4CommitState_K2Node_ComponentBoundEvent_15_OnStateEnterSignature__DelegateSignature"};
    auto type=Read<void*>(ability,0x10);
    for(size_t i=0;i<primaryFunctions.size();++i){const auto path=std::wstring(XPrefix)+primary[i];primaryFunctions[i]=engine::Find(path.c_str());
        if(!Script(primaryFunctions[i],type,72,20))return Fail(3425,"original_dagger_primary_commit_callback_profile_changed");}
    burstFunction=engine::Find((std::wstring(XPrefix)+L"BndEvt__ProjectileCommit2State_K2Node_ComponentBoundEvent_3_OnStateEnterSignature__DelegateSignature").c_str());
    if(!Script(burstFunction,type,72,20))return Fail(3426,"original_dagger_burst_commit_callback_profile_changed");
    if(!Install(ability,actorTable,actorInstalled))return Fail(3419,"original_dagger_original_commit_observer_unavailable",true);
    diagnostics.bound=true;diagnostics.binding=++nextBinding;++diagnostics.binds;
    constexpr const wchar_t* names[]{L"ProjectileFireState",L"BurstProjectileFireState"};
    for(size_t i=0;i<fireStates.size();++i){auto& s=fireStates[i];
        if(!Object(ability,names[i],s.object)||!s.object||Read<void*>(s.object,0xc8)!=ability||!Identity(s.object,s.identity))return Fail(3420,"original_dagger_real_fire_state_missing");
        if(!EnsureOriginalDelegate(s,L"OnStateEnter",fireEnterFunction,s.enterBound)||!EnsureOriginalDelegate(s,L"OnStateExit",fireExitFunction,s.exitBound))return false;
        if(!Install(s.object,s.table,s.installed))return Fail(3421,"original_dagger_original_factory_observer_unavailable",true);
    }
    if(!ObserveOriginalConstructorTypes())return false;
    for(size_t i=0;i<baseline.size();++i)original_melee_combat::ReadLife(i,baseline[i]);
    diagnostics.ready=true;diagnostics.error=0;diagnostics.stage="original_dagger_original_commit_factory_and_hit_observers_bound";return ObserveProjectiles();
}
inline bool BeforeRetireImpl(void* pawn){
    if(!Thread())return false;if(!diagnostics.bound)return true;if(pawn!=diagnostics.pawn)return true;
    if(callbacksActive||constructorsActive)return Fail(3422,"original_dagger_retirement_inside_original_callback_rejected");
    if(!RestoreConstructorTypes())return false;
    if(!Restore(diagnostics.ability,abilityIdentity,actorTable,actorInstalled))return false;
    for(auto& s:fireStates)if(!Restore(s.object,s.identity,s.table,s.installed))return false;
    for(auto& witness:actorWitnesses)if(witness.actor&&!Restore(witness.actor,witness.identity,witness.table,witness.installed))return false;
    for(auto& p:projectiles)if(p.used&&!Restore(p.component,p.componentId,p.table,p.installed))return false;
    diagnostics.bound=diagnostics.ready=false;++diagnostics.retirements;fireStates={};projectiles={};actorWitnesses={};baseline={};seen={};commits={};commitCursor=0;readIndex=writeIndex;return true;
}
inline bool PollImpl(double now){
    if(!Thread()||!std::isfinite(now)||now<0||now+.05<diagnostics.lastPoll)return false;
    diagnostics.lastPoll=now;++diagnostics.polls;if(!diagnostics.bound)return true;if(!Source())return Fail(3423,"original_dagger_source_life_epoch_changed_before_retire",true);
    if(diagnostics.error)return false;if(!ObserveProjectiles()||!DrainWorldRing())return false;
    const auto end=writeIndex;size_t budget=32;while(readIndex<end&&budget--){bool deferred=false;if(!Apply(queue[readIndex%queue.size()],now,deferred))return false;if(deferred)break;++readIndex;++diagnostics.processed;}
    for(size_t i=0;i<baseline.size();++i)original_melee_combat::ReadLife(i,baseline[i]);
    diagnostics.ready=true;diagnostics.stage="original_dagger_waiting_or_consuming_real_native_contacts";return true;
}
inline bool Initialize(void* world){__try{return InitializeImpl(world);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool Bind(void* pawn,void* ability,uint32_t epoch){__try{return BindImpl(pawn,ability,epoch);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool BeforeRetire(void* pawn){__try{return BeforeRetireImpl(pawn);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool Poll(double now){if(diagnostics.processing)return Fail(3424,"original_dagger_reentrant_poll_rejected");diagnostics.processing=true;bool okay=false;
    __try{okay=PollImpl(now);}__except(Capture(GetExceptionInformation())){okay=false;}diagnostics.processing=false;return okay;}
inline void WriteTableProbe(FILE* f,const TableProbe& p){
    const auto b=[](bool v){return v?"true":"false";};
    std::fprintf(f,"{\"object\":\"%p\",\"class\":\"%p\",\"actual_table\":\"%p\",\"native_table\":\"%p\",\"parent_pe\":\"%p\",\"parent_pe_rva\":%llu,\"table_rva\":%llu,\"function_count\":%zu,\"boundary\":\"%p\",\"profile\":%u,\"weak_identity_verified\":%s,\"native_image\":%s,\"extent_verified\":%s,\"parent_verified\":%s,\"reason\":",
        p.object,p.type,p.actual,p.nativeTable,p.event,static_cast<unsigned long long>(p.eventRva),static_cast<unsigned long long>(p.tableRva),p.count,p.boundary,p.profile,b(p.identityVerified),b(p.nativeImage),b(p.extentVerified),b(p.parentVerified));
    original_players::Text(f,p.reason);std::fputc('}',f);
}
inline void WriteObserverTable(FILE* f,const Table& t,void* object=nullptr){
    const auto b=[](bool v){return v?"true":"false";};
    const auto actual=object?Read<void**>(object,0):nullptr;
    const auto parent=reinterpret_cast<void*>(t.parentEvent);
    const bool sameParent=t.previous&&Read<void*>(t.previous,EventSlot*8)==parent;
    const bool sameClone=t.copy[1+EventSlot]==reinterpret_cast<void*>(&ObserveEvent);
    const auto rva=(t.profile==1||t.profile==2)&&reinterpret_cast<uintptr_t>(parent)>=engine::image?reinterpret_cast<uintptr_t>(parent)-engine::image:0;
    std::fprintf(f,"{\"object\":\"%p\",\"actual_table\":\"%p\",\"parent_table\":\"%p\",\"cloned_table\":\"%p\",\"native_table\":\"%p\",\"function_count\":%zu,\"parent_pe\":\"%p\",\"parent_pe_rva\":%llu,\"profile\":%u,\"installed_readback\":%s,\"parent_pe_readback\":%s,\"clone_pe_readback\":%s,\"native_entrance_parent\":\"%p\",\"native_entrance_parent_rva\":%llu,\"entrance_clone_readback\":%s}",
        object,actual,t.previous,t.copy.data()+1,t.nativeTable,t.count,parent,static_cast<unsigned long long>(rva),t.profile,b(actual==t.copy.data()+1),b(sameParent),b(sameClone),reinterpret_cast<void*>(t.entranceParent),static_cast<unsigned long long>(NativeRva(reinterpret_cast<void*>(t.entranceParent))),b(t.entranceParent&&t.copy[1+EntranceSlot]==reinterpret_cast<void*>(&ObserveEntrance)));
}
inline void WriteFireInput(FILE* f,const FireState& s){
    const auto b=[](bool v){return v?"true":"false";};const auto finite=[](double v){return std::isfinite(v)?v:0.;};
    std::fprintf(f,"{\"samples_after_original_enter\":%llu,\"native_world\":\"%p\",\"world_matches\":%s,\"original_equippable\":\"%p\",\"equippable_owner\":\"%p\",\"original_pawn\":\"%p\",\"original_player_state\":\"%p\",\"original_player_info\":\"%p\",\"owner_queries_valid\":%s,\"original_fields_valid\":%s,\"projectile_delay\":%.9g,\"error_power\":%.9g,\"use_start_transform\":%s,\"use_error_values\":%s,\"fire_timer_handle\":%llu,\"projectile_class_default\":\"%p\",\"original_pool_flag_514\":%u,\"original_spawn_mode_524\":%u,\"original_graph\":\"%p\",\"original_graph_frame\":\"%p\",\"original_frame_getter\":\"%p\",\"original_frame_valid\":%s,\"original_transition_context\":\"%p\",\"original_is_server\":%s,\"original_firing_location\":[%.9g,%.9g,%.9g],\"original_firing_direction\":[%.9g,%.9g,%.9g],\"original_vectors_finite\":%s}",
        static_cast<unsigned long long>(s.samples),s.world,b(s.world==diagnostics.world),s.equippable,Read<void*>(s.equippable,0x210),s.pawn,s.playerState,s.playerInfo,b(s.ownerQueriesValid),b(s.fieldsValid),finite(s.projectileDelay),finite(s.errorPower),b(s.useStartTransform),b(s.useErrorValues),static_cast<unsigned long long>(s.fireHandle),s.classDefault,static_cast<unsigned>(s.pooledClass),static_cast<unsigned>(s.spawnMode),s.graph,s.frame,s.frameGetter,b(s.frameValid),s.transitionContext,b(s.originalIsServer),
        finite(s.originalLocation.x),finite(s.originalLocation.y),finite(s.originalLocation.z),finite(s.originalDirection.x),finite(s.originalDirection.y),finite(s.originalDirection.z),b(original_players::Finite(s.originalLocation)&&original_players::Finite(s.originalDirection)));
}
inline void WriteFactoryResult(FILE* f,const FireState& s){
    const auto b=[](bool v){return v?"true":"false";};const auto finite=[](double v){return std::isfinite(v)?v:0.;};
    std::fprintf(f,"{\"context_valid\":%s,\"array_data\":\"%p\",\"returned_actor_count\":%d,\"returned_actor_capacity\":%d,\"original_get_first_matches\":%s,\"first_actor\":\"%p\",\"first_class\":\"%p\",\"first_damage_component\":\"%p\",\"actor_weak_live\":%s,\"component_weak_live\":%s,\"actor_object_flags\":%u,\"actor_internal_index\":%d,\"original_initialized_at\":%.9g,\"original_reusable_513\":%u,\"original_pool_514\":%u,\"original_stop_action_525\":%u,\"original_destroy_delay_528\":%.9g,\"original_cleaned_5d1\":%u,\"native_point_count\":%d,\"native_point_capacity\":%d,\"factory_actor_observations\":%llu,\"nonlive_actor_observations\":%llu,\"retained_native_point_observations\":%llu,\"imported_native_points\":%llu}",
        b(s.contextValid),s.returnedActors,s.returnedActorCount,s.returnedActorCapacity,b(s.firstGetterMatches),s.firstReturnedActor,s.firstReturnedClass,s.firstDamageComponent,b(s.firstActorLive),b(s.firstComponentLive),s.firstObjectFlags,s.firstInternalIndex,finite(s.firstInitializedAt),static_cast<unsigned>(s.firstReusable),static_cast<unsigned>(s.firstPool),static_cast<unsigned>(s.firstStopAction),finite(s.firstDestroyDelay),static_cast<unsigned>(s.firstCleaned),s.nativeDamageCount,s.nativeDamageCapacity,
        static_cast<unsigned long long>(s.factoryResults),static_cast<unsigned long long>(s.nonLiveFactoryActors),static_cast<unsigned long long>(s.retainedNativePoints),static_cast<unsigned long long>(s.importedNativePoints));
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;std::wstring temporary(path);temporary+=L".tmp";FILE* f=nullptr;if(_wfopen_s(&f,temporary.c_str(),L"wb")||!f)return false;
    const auto b=[](bool v){return v?"true":"false";};const auto& d=diagnostics;
    std::fprintf(f,"{\"schema_version\":2,\"pid\":%lu,\"stage\":",GetCurrentProcessId());original_players::Text(f,d.stage);
    std::fprintf(f,",\"error\":%u,\"initialized\":%s,\"bound\":%s,\"ready\":%s,\"restart_required\":%s,\"pawn\":\"%p\",\"ability\":\"%p\",\"resource\":\"%p\",\"epoch\":%u,\"binding\":%llu,\"exception_rva\":%llu",d.error,b(d.initialized),b(d.bound),b(d.ready),b(d.restartRequired),d.pawn,d.ability,d.resource,d.epoch,static_cast<unsigned long long>(d.binding),static_cast<unsigned long long>(d.exceptionRva));
#define DAGGER_COUNT(name) std::fprintf(f,",\"" #name "\":%llu",static_cast<unsigned long long>(d.name))
    DAGGER_COUNT(polls);DAGGER_COUNT(binds);DAGGER_COUNT(retirements);DAGGER_COUNT(restores);DAGGER_COUNT(tableConflicts);DAGGER_COUNT(projectilesSeen);DAGGER_COUNT(observersInstalled);
    DAGGER_COUNT(entranceCallbacks);DAGGER_COUNT(nativePoints);DAGGER_COUNT(polledPoints);DAGGER_COUNT(queued);DAGGER_COUNT(duplicates);DAGGER_COUNT(nativeLifeChanged);DAGGER_COUNT(baselineUnavailable);
    DAGGER_COUNT(invalidEvents);DAGGER_COUNT(worldHits);DAGGER_COUNT(nonCoreHits);DAGGER_COUNT(stale);DAGGER_COUNT(processed);DAGGER_COUNT(damageEvents);DAGGER_COUNT(kills);DAGGER_COUNT(queueOverflow);DAGGER_COUNT(pendingDeferred);
    DAGGER_COUNT(primaryCommits);DAGGER_COUNT(burstCommits);DAGGER_COUNT(resourceCalls);DAGGER_COUNT(resourceAlreadyConsumed);
    DAGGER_COUNT(dedicatedQueries);
    DAGGER_COUNT(duplicateCommits);
    DAGGER_COUNT(primaryKills);DAGGER_COUNT(burstKills);DAGGER_COUNT(unknownKindKills);DAGGER_COUNT(primaryRefills);DAGGER_COUNT(primaryAlreadyRefilled);
    DAGGER_COUNT(ownerChainMatches);DAGGER_COUNT(instigatorMatches);DAGGER_COUNT(sourceRejected);
    DAGGER_COUNT(originalForwards);DAGGER_COUNT(actorForwards);DAGGER_COUNT(componentForwards);DAGGER_COUNT(effectsParentForwards);DAGGER_COUNT(missingForwarders);
    DAGGER_COUNT(constructorEntered);DAGGER_COUNT(constructorCompleted);DAGGER_COUNT(constructorTemplates);DAGGER_COUNT(constructorRestores);
    DAGGER_COUNT(actorCandidates);DAGGER_COUNT(actorObserversInstalled);DAGGER_COUNT(constructionEvents);DAGGER_COUNT(beginPlayEvents);DAGGER_COUNT(earlyHitObservers);DAGGER_COUNT(lifecycleSourceRejected);DAGGER_COUNT(nativeEntranceForwards);
#undef DAGGER_COUNT
    std::fprintf(f,",\"last_constructor_type\":\"%p\",\"last_constructor_parent\":\"%p\",\"last_constructor_parent_rva\":%llu,\"last_candidate_actor\":\"%p\",\"last_early_damage_component\":\"%p\",\"constructor_type_installs\":%u,\"constructor_types\":[",d.lastConstructorType,d.lastConstructorParent,static_cast<unsigned long long>(NativeRva(d.lastConstructorParent)),d.lastCandidateActor,d.lastEarlyComponent,d.constructorTypeCount);
    bool firstConstructor=true;for(const auto& type:constructorTypes)if(type.type){const bool live=Live(type.type,type.identity);
        std::fprintf(f,"%s{\"class\":\"%p\",\"weak_live\":%s,\"installed\":%s,\"actual_constructor\":\"%p\",\"parent_constructor\":\"%p\",\"parent_rva\":%llu,\"readback_matches\":%s,\"entered\":%llu,\"completed\":%llu,\"templates_preserved\":%llu,\"instances_observed\":%llu}",firstConstructor?"":",",type.type,b(live),b(type.installed),live?Read<void*>(type.type,0xb8):nullptr,reinterpret_cast<void*>(type.parent),static_cast<unsigned long long>(NativeRva(reinterpret_cast<void*>(type.parent))),b(live&&Read<void*>(type.type,0xb8)==(type.installed?reinterpret_cast<void*>(&ObserveConstructor):reinterpret_cast<void*>(type.parent))),static_cast<unsigned long long>(type.entered),static_cast<unsigned long long>(type.completed),static_cast<unsigned long long>(type.templates),static_cast<unsigned long long>(type.instances));firstConstructor=false;}
    std::fputs("],\"actor_lifecycle_observers\":[",f);bool firstActor=true;for(const auto& witness:actorWitnesses)if(witness.actor){
        std::fprintf(f,"%s{\"actor\":\"%p\",\"class\":\"%p\",\"weak_live\":%s,\"binding\":%llu,\"epoch\":%u,\"callbacks\":%llu,\"hit_observer_ready\":%s,\"table\":",firstActor?"":",",witness.actor,witness.type,b(Live(witness.actor,witness.identity)),static_cast<unsigned long long>(witness.binding),witness.epoch,static_cast<unsigned long long>(witness.callbacks),b(witness.hitObserverReady));
        if(witness.table)WriteObserverTable(f,*witness.table,witness.actor);else std::fputs("null",f);std::fputc('}',f);firstActor=false;}
    std::fputs("],\"early_observer_policy\":\"exact_authored_classes_original_ctor_once_original_actor_events_then_native_entrance_once\",\"image_pages_written\":false,\"projectile_lifetime_changed\":false",f);
    std::fprintf(f,",\"dedicated_query_world\":\"%p\",\"dedicated_query_result\":%s,\"dedicated_query_parameter_bytes\":9",d.dedicatedWorld,b(d.dedicatedResult));
    std::fputs(",\"observer_probe\":",f);WriteTableProbe(f,d.observerProbe);
    std::fputs(",\"failed_observer_probe\":",f);WriteTableProbe(f,d.failedObserverProbe);
    std::fprintf(f,",\"last_forward_object\":\"%p\",\"last_forward_parent\":\"%p\",\"last_forward_parent_table\":\"%p\",\"observer_profiles\":{\"1\":\"native_actor_component_1cdc850\",\"2\":\"native_actor_3eda0d0\",\"3\":\"verified_original_effects_manager_parent\"},\"actor_observer\":",d.lastForwardObject,d.lastForwardParent,d.lastForwardTable);
    if(actorTable)WriteObserverTable(f,*actorTable,d.ability);else std::fputs("null",f);
    std::fputs(",\"observer_tables\":[",f);bool firstTable=true;for(const auto& table:tables)if(table.previous){std::fputs(firstTable?"":",",f);WriteObserverTable(f,table);firstTable=false;}
    std::fputc(']',f);
    std::fprintf(f,",\"original_dagger_index\":%d,\"original_primary_class_count\":%d,\"original_burst_class_count\":%d,\"last_projectile_owner\":\"%p\",\"last_native_instigator\":\"%p\",\"last_instigator_info\":\"%p\",\"last_expected_info\":\"%p\"",d.originalDaggerIndex,d.originalPrimaryClassCount,d.originalBurstClassCount,d.lastProjectileOwner,d.lastNativeInstigator,d.lastInstigatorInfo,d.lastExpectedInfo);
    std::fprintf(f,",\"primary_kill_refill_before\":%d,\"primary_kill_refill_after\":%d",d.killRefillBefore,d.killRefillAfter);
    std::fprintf(f,",\"queue_pending\":%llu,\"resource_before\":%d,\"resource_after\":%d,\"last_token\":%llu,\"last_victim\":%zu,\"last_original_damage\":%.9g,\"last_original_falloff\":%.9g,\"fire_states\":[",static_cast<unsigned long long>(writeIndex-readIndex),d.resourceBefore,d.resourceAfter,static_cast<unsigned long long>(d.lastToken),d.lastVictim,d.lastDamage,d.lastFalloff);
    for(size_t i=0;i<fireStates.size();++i){const auto& s=fireStates[i];std::fprintf(f,"%s{\"state\":\"%p\",\"on_enter_bound\":%s,\"on_exit_bound\":%s,\"real_state_enter_callbacks\":%llu,\"original_class_selection_calls\":%llu,\"missing_original_bindings_supplied\":%llu,\"projectile_class_before\":\"%p\",\"projectile_class\":\"%p\",\"observer\":",i?",":"",s.object,b(s.enterBound),b(s.exitBound),static_cast<unsigned long long>(s.entries),static_cast<unsigned long long>(s.classSupplies),static_cast<unsigned long long>(s.bindingSupplies),s.classBefore,s.projectileClass);
        if(s.table)WriteObserverTable(f,*s.table,s.object);else std::fputs("null",f);
        std::fputs(",\"original_fire_input\":",f);WriteFireInput(f,s);
        std::fputs(",\"original_factory_result\":",f);WriteFactoryResult(f,s);std::fputc('}',f);}
    std::fputs("],\"failed_original_event\":",f);const auto failed=original_players::Utf8(d.failedEvent);original_players::Text(f,failed.c_str());
    std::fprintf(f,",\"failed_script_function\":\"%p\",\"expected_script_bytes\":%d,\"actual_script_bytes\":%d",d.failedScriptFunction,d.expectedScriptBytes,d.actualScriptBytes);
    std::fprintf(f,",\"expected_parameter_bytes\":%u,\"actual_parameter_bytes\":%u,\"complete_native_point_event_bytes\":304,\"native_entrance_parameter_bytes\":480,\"score_owner\":\"deathmatch_rules\",\"shared_rifle_ring\":false,\"replacement_rays_or_trajectories\":false,\"direct_health_or_resource_writes\":false,\"original_callback_forwarding\":\"exactly_once\",\"compiled_by_this_agent\":false,\"runtime_verified\":false}\n",static_cast<unsigned>(d.expectedBytes),static_cast<unsigned>(d.actualBytes));
    const bool okay=std::ferror(f)==0,closed=std::fclose(f)==0;return okay&&closed&&MoveFileExW(temporary.c_str(),path,MOVEFILE_REPLACE_EXISTING);
}
}
