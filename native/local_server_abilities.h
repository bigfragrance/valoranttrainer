#pragma once
#include "original_jett_native.h"
#include "original_jett_priming.h"
#include "original_jett_world.h"
#include "original_inventory_slots.h"
#include "local_server_item_ownership.h"
#include "local_server_resource_authority.h"
#include <array>

// The network host creates the real Wushu loadout. The client only adopts
// replicated actor/slot references. Native input scheduling, state transitions,
// deterministic force modules, projectiles, animation and reconciliation stay
// in the original client. File-only evidence: work/local-server-playable/abilities.
namespace local_server_abilities {
using Weak=original_inventory_slots::WeakIdentity;
using EventFn=void(*)(void*,void*,void*);
inline constexpr size_t SkillCount=original_jett::SkillCount,MaxPlayers=8,MaxComponents=128;
inline constexpr size_t EventSlot=0x298/8,MaxFunctions=1024,MaxTables=24;
inline constexpr size_t MaxDiagnosticMachines=16;
enum Prerequisite:uint32_t {
    ExpectedClass=1U<<0,NativeInventoryCurrent=1U<<1,NativeSingleSlot=1U<<2,NativeSlotBackref=1U<<3,
    NativeSlotContents=1U<<4,OriginalActorClass=1U<<5,ActorIdentity=1U<<6,ActorOwner=1U<<7,
    TypedActorOwner=1U<<8,ActorBeginPlay=1U<<9,PrimaryMachine=1U<<10,PrimaryMachineOwner=1U<<11,
    OriginalResource=1U<<12,ResourceOwner=1U<<13,OriginalEventRegistration=1U<<14,ActualNetworkRole=1U<<15
};
inline constexpr uint32_t RequiredPrerequisites=(1U<<16)-1;
inline constexpr const char* PrerequisiteNames[]{"expected_original_class","registered_inventory_matches_native_pawn","native_single_item_slot","slot_inventory_backref","resolved_slot_contents","original_skill_actor_class","captured_live_actor_identity","native_actor_owner","typed_equippable_owner","original_actor_begin_play","original_primary_machine","primary_machine_owner","original_resource_component","resource_component_owner","original_event_registration","actual_network_role"};
struct NamedObservation {void* object=nullptr;char name[128]{};};
struct MachineObservation {
    NamedObservation machine{},starting{},authority{},predicted{},selected{};
    void* owner=nullptr;uint8_t ownerRole=255;bool typed=false,ownerMatches=false,enabled=false;
    bool startingOwnerMatches=false,startingMachineMatches=false,selectedNonempty=false;
    float subframe=0;
    // Cache-only diagnostics. A Q force lives in BoostStateMachine rather
    // than the primary machine, so the legacy skill counter is not complete.
    bool forceState=false,forceQueryAttempted=false,forceQueryValid=false,forceActive=false;
    void *forceModuleClass=nullptr,*forceManager=nullptr,*forceSource=nullptr;
    uint64_t forceHandle=0;
};
struct SkillObservation {
    uint32_t prerequisiteMask=0,missingMask=RequiredPrerequisites;
    const char* waiting="not_sampled";
    void *cachedInventory=nullptr,*nativeInventory=nullptr,*cachedSlot=nullptr,*nativeSlot=nullptr;
    void *cachedContents=nullptr,*nativeContents=nullptr,*slotOuter=nullptr,*slotInventory=nullptr;
    void *observedActor=nullptr,*actorOwner=nullptr,*typedOwner=nullptr,*resource=nullptr,*resourceOwner=nullptr;
    NamedObservation expectedType{},actualType{};
    uint8_t actualRole=255,nativeSlotType=255;uint32_t actorFlags=0;
    bool slotTypeValid=false,slotOuterMatches=false,slotInventoryMatches=false,contentsMatch=false;
    bool cachedActorMatchesNative=false,actorIdentityObserved=false,replicates=false;
    bool beginQueryAttempted=false,beginQueryValid=false,beginObserved=false,resourceLookupAttempted=false;
    void *connection=nullptr,*channel=nullptr;bool channelSnapshotValid=false,channelFound=false,channelClosing=false,channelOpenAck=false;
    uint8_t channelFlags=0,channelOpenFlags=0;
    bool machineCacheValid=false;int32_t machineCacheCount=0;size_t machinesCaptured=0;
    std::array<MachineObservation,MaxDiagnosticMachines> machines{};
    void* tuning=nullptr;int32_t tuningMapCount=-1,tuningArrayCount=-1;
    local_server_resource_authority::ComponentRoute resourceRoute{},tuningRoute{};
};
inline constexpr const wchar_t* XPrefix=L"/Game/Characters/Wushu/S0/Ability_X/Ability_Wushu_X_Dagger_Production.Ability_Wushu_X_Dagger_Production_C.";
inline constexpr const wchar_t* FirePrefix=L"/Game/Characters/States/FireProjectileMulticast_StateComponent.FireProjectileMulticast_StateComponent_C.";
struct Skill {
    void *type=nullptr,*actor=nullptr,*slot=nullptr,*resource=nullptr,*machine=nullptr,*current=nullptr;
    Weak identity{};
    uint8_t slotType=255,role=255;
    bool spawnAttempted=false,spawned=false,adopted=false,ready=false,eventsReady=false;
    bool begunPlay=false,ownerReady=false,slotReady=false,resourceReady=false;
    bool resourceQuery=false,forceHandleActive=false;
    bool beginQueryAttempted=false,beginQueryValid=false,resourceLookupAttempted=false;
    int32_t charges=-1,maxCharges=-1,authResource=-1,predictedResource=-1;
    uint64_t transitions=0,resourceChanges=0,initializations=0,forceObservations=0;
    uint64_t originalForceHandle=0;
    wchar_t currentName[128]{};
    std::array<void*,MaxComponents> components{};
    size_t componentCount=0;
    SkillObservation observed{};
};
struct FireState {
    void* object=nullptr;Weak identity{};bool enterBound=false,exitBound=false;
    uint64_t entries=0,classSupplies=0,activeSequence=0,factoryResults=0,retainedPoints=0,nonLiveReturns=0,frameMisses=0;
    uint32_t activeDepth=0;
};
struct Commit {Weak state{},machine{};uint32_t timestamp=0;void* function=nullptr;};
struct FieldObservation {
    int32_t offset=-1;uint8_t fieldSize=0,byteOffset=0,byteMask=0,fieldMask=0;
    bool valid=false,value=false;int32_t integer=-1;
};
struct DaggerObservation {
    void* resource=nullptr;bool sourceValid=false,fieldsCaptured=false,multicastObserved=false;
    FieldObservation remaining{},maximum{},shouldConsume{},shouldReset{};
    uint32_t multicastFlags=0;uint16_t multicastBytes=0;int32_t multicastScriptBytes=-1;
    NamedObservation primaryClass{},burstClass{};
};
struct GlideObservation {
    NamedObservation actor{};Weak identity{};void* slot=nullptr;
    uint8_t role=255;bool slotSchema=false,arrayValid=false,sourceValid=false;
    int32_t count=-1,matches=0;bool activeFieldCaptured=false;
    FieldObservation active{};
    SkillObservation graph{};
};
struct Player {
    void *controller=nullptr,*playerState=nullptr,*pawn=nullptr,*inventory=nullptr,*forceManager=nullptr;
    Weak controllerId{},stateId{},pawnId{};
    bool used=false,authority=false,grantComplete=false,ready=false,refilled=false,daggerBound=false;
    bool startingStatesReady=false,primingReady=false,ultimateQuery=false,ultimateReady=false;
    uint32_t sourceEpoch=0;
    uint64_t registration=0;
    const char* waiting="not_sampled";uint64_t diagnosticAt=0,diagnosticSamples=0;
    bool readyAtDiagnostic=false;
    uint32_t sourceMask=0;
    void *observedNativeInventory=nullptr,*observedNativeForceManager=nullptr;
    uint8_t role=255;
    int32_t points=-1,pointsRequired=-1,activeForces=-1;
    uint64_t polls=0,refills=0,ownerCallbacks=0,slotNotifications=0,eventRegistrations=0;
    uint64_t primaryCommits=0,burstCommits=0,duplicateCommits=0,consumes=0,alreadyConsumed=0;
    uint64_t ultimateConsumes=0,ultimateActiveUpdates=0;
    std::array<Skill,SkillCount> skills{};
    std::array<FireState,2> fireStates{};
    std::array<Commit,64> commits{};
    size_t commitCursor=0;
    original_jett_priming::Diagnostics priming{};
    DaggerObservation daggerObserved{};GlideObservation glideObserved{};
};
struct State {
    bool initialized=false,host=false,practice=false,restartRequired=false,profilesVerified=false,worldReplicationSchemaVerified=false;
    bool nativePredictionPreserved=true,nativeForceEntryPreserved=true;
    // These become proven only through the independent two-process harness.
    bool networkExecutionVerified=false,damageApplicationVerified=false;
    void* world=nullptr;
    DWORD gameThread=0;
    unsigned error=0,players=0,readyPlayers=0;
    uint64_t polls=0,eventForwards=0,eventPrepareFailures=0,tablesInstalled=0;
    uint64_t retiredPlayers=0,restoredObservers=0,worldSamples=0,worldSampleAt=0;
    uint64_t nativeDaggerPoints=0,invalidDaggerPoints=0,smokeZones=0,smokeLoops=0,worldEffects=0,daggerInitializations=0;
    uint64_t playerRegistrations=0,fireSequences=0,factoryPointDeliveries=0,worldReplicationCandidates=0,worldReplicationFiltered=0,worldSourceDeferrals=0;
    uint64_t replicaGraphRetirements=0,replicaCaptureFailures=0,replicaResolveDeferrals=0;
    void* lastReplicaIdentityActor=nullptr;
    Weak lastReplicaIdentity{};
    uint32_t lastReplicaIdentitySkill=0;
    bool lastReplicaCaptureSucceeded=false;
    bool diagnosticForceChecked=false,diagnosticForceProfileVerified=false;
    uintptr_t exceptionRva=0;
    const char* stage="network_abilities_not_initialized";
    const wchar_t* failedEvent=nullptr;
    wchar_t failedEventName[512]{};
    uint16_t expectedBytes=0,actualBytes=0;
};
struct Table {void** parent=nullptr;void** copy=nullptr;size_t count=0;EventFn event=nullptr;};
inline State state;
inline std::array<Player,MaxPlayers> players{};
inline std::array<Table,MaxTables> tables{};
inline std::array<void*,5> primaryCallbacks{};
inline void *burstCallback=nullptr,*fireEnter=nullptr,*fireExit=nullptr,*fireProjectiles=nullptr,*fireGraph=nullptr;
inline void *consumeUltimate=nullptr,*setUltActive=nullptr,*setUltimateActive=nullptr;
struct NativePoint {
    uint64_t sequence=0;void *pawn=nullptr,*playerState=nullptr,*ability=nullptr,*projectile=nullptr,*component=nullptr,*victim=nullptr;
    Weak projectileId{},componentId{},victimId{};
    uint32_t initializedBits=0;int32_t nativeIndex=-1;
    bool geometryValid=false;
    std::array<uint8_t,0x130> bytes{};
    // Appended fields preserve every pre-existing source field and its offset.
    uint64_t fireSequence=0;void* fireState=nullptr;bool fromFactory=false;
};
struct WorldWitness {
    void* actor=nullptr;Weak identity{};void* ownerPawn=nullptr;uint32_t initializedBits=0;int32_t nextPoint=0;bool dagger=false;
    Weak ownerPawnId{};uint64_t ownerRegistration=0;uint32_t sourceEpoch=0;bool effect=false,projectile=false;
};
struct WorldActorInfo {
    void *sourcePawn=nullptr,*sourcePlayerState=nullptr,*sourceAbility=nullptr;
    Weak sourcePawnId{};uint64_t sourceRegistration=0;uint32_t sourceEpoch=0;
    bool dagger=false,effect=false,projectile=false,originalReplicates=false,predictionMergeProven=false;
};
inline std::array<WorldWitness,512> worldWitnesses{};
inline std::array<NativePoint,64> pointWitnesses{};
using NativePointSink=void(*)(const NativePoint&);
inline NativePointSink pointSink=nullptr;
inline void SetNativePointSink(NativePointSink sink){pointSink=sink;}
using FireBoundarySink=void(*)(void* sourcePawn,void* fireState,uint64_t fireSequence,bool before);
inline FireBoundarySink fireBoundarySink=nullptr;
inline void SetFireBoundarySink(FireBoundarySink sink){fireBoundarySink=sink;}
inline const NativePoint* NativePointAt(uint64_t sequence){
    if(!sequence||sequence>state.nativeDaggerPoints||state.nativeDaggerPoints-sequence>=pointWitnesses.size())return nullptr;
    const auto& point=pointWitnesses[(sequence-1)%pointWitnesses.size()];return point.sequence==sequence?&point:nullptr;
}
inline const State& GetState(){return state;}
inline const std::array<Player,MaxPlayers>& GetPlayers(){return players;}
template<class T> inline T Read(const void* p,size_t offset){return original_players::Value<T>(p,offset);}
inline bool Fail(unsigned code,const char* why){state.error=code;state.stage=why;return false;}
inline bool Thread(){return state.initialized&&!state.restartRequired&&state.gameThread==GetCurrentThreadId()&&original_players::Thread();}
inline bool Live(void* p,const Weak& id){return p&&original_inventory_slots::ResolveIdentity(id)==p;}
inline bool Identity(void* p,Weak& id){return original_inventory_slots::CaptureIdentity(p,id)&&Live(p,id);}
inline bool IsA(void* p,uintptr_t getter){return original_players::IsA(p,getter);}
inline bool Event(void* object,const wchar_t* path,uint16_t bytes,void* p){
    auto fn=engine::Find(path);const auto actual=Read<uint16_t>(fn,0xbe);
    if(!object||!fn||actual!=bytes){
        state.failedEventName[0]=0;if(path)wcsncpy_s(state.failedEventName,path,_TRUNCATE);
        state.failedEvent=state.failedEventName;state.expectedBytes=bytes;state.actualBytes=actual;return false;
    }
    auto table=Read<void**>(object,0);auto event=Read<EventFn>(table,0x298);
    if(!event)return false;event(object,fn,p);return true;
}
inline LONG Capture(EXCEPTION_POINTERS* e){
    state.error=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0004400U;
    auto at=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;state.restartRequired=true;
    state.stage="network_abilities_exception_requires_new_session";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Source(const Player& p){
    return p.used&&Live(p.pawn,p.pawnId)&&Live(p.playerState,p.stateId)&&Live(p.controller,p.controllerId)
        &&Read<void*>(p.pawn,0x488)==p.playerState&&Read<void*>(p.controller,0x470)==p.playerState
        &&Read<void*>(p.controller,0x4b0)==p.pawn&&Read<uint8_t>(p.pawn,0x224)==p.role
        &&p.authority==state.host;
}
inline Player* Find(void* pawn){for(auto& p:players)if(p.used&&p.pawn==pawn&&Live(p.pawn,p.pawnId))return &p;return nullptr;}
inline void ObserveName(NamedObservation& observation,void* object){
    if(observation.object!=object){original_weapon::OriginalObjectName(object,observation.name);observation.object=object;}
}
// Optional diagnostics never fail a gameplay prerequisite or dispatch an
// unknown function. Unknown ABI/profile remains an unobserved query.
inline bool DiagnosticForceQueryImpl(void* manager,uint64_t handle,bool& active){
    if(!Thread()||!IsA(manager,0x685f510)||!handle)return false;
    if(!state.diagnosticForceChecked){
        state.diagnosticForceChecked=true;const auto code=reinterpret_cast<const uint8_t*>(engine::image+0x68622e0);
        if(original_players::Readable(code,330)){
            uint64_t digest=1469598103934665603ULL;
            for(size_t n=0;n<330;++n)digest=(digest^code[n])*1099511628211ULL;
            state.diagnosticForceProfileVerified=digest==0x09f9130632353ab4ULL;
        }
    }
    if(!state.diagnosticForceProfileVerified)return false;
    auto fn=engine::Find(L"/Script/ShooterGame.ForceModuleManagerComponent.HasActiveForceModule");
    if(!fn||Read<uint16_t>(fn,0xbe)!=9||Read<void*>(fn,0xf0)!=reinterpret_cast<void*>(engine::image+0x68622e0)
       ||Read<void*>(fn,0x28)!=engine::Class(0x685f510)||(Read<uint32_t>(fn,0xb8)&0x400U)==0)return false;
    auto dispatch=Read<EventFn>(Read<void**>(manager,0),0x298);if(!dispatch)return false;
    struct Query {uint64_t handle;uint8_t result=0,pad[7]{};} query{handle};
    dispatch(manager,fn,&query);active=query.result!=0;return true;
}
inline bool DiagnosticForceQuery(void* manager,uint64_t handle,bool& active){
    __try{return DiagnosticForceQueryImpl(manager,handle,active);}__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline void CaptureField(FieldObservation& out,void* object,const wchar_t* name,bool boolean){
    out={};original_jett_priming::PropertyInfo info{};
    out.offset=original_jett_priming::FieldOffset(Read<void*>(object,0x10),name,boolean?1:4,&info);
    if(out.offset<0||info.arrayDim!=1||info.elementSize!=(boolean?1:4))return;
    if(boolean){
        out.fieldSize=Read<uint8_t>(info.field,0x78);out.byteOffset=Read<uint8_t>(info.field,0x79);
        out.byteMask=Read<uint8_t>(info.field,0x7a);out.fieldMask=Read<uint8_t>(info.field,0x7b);
        out.valid=out.fieldSize==1&&out.byteOffset==0&&out.byteMask&&out.fieldMask;
    }else out.valid=true;
}
inline void SampleField(FieldObservation& field,void* object,bool boolean){
    if(!field.valid)return;
    if(boolean)field.value=(Read<uint8_t>(object,static_cast<size_t>(field.offset)+field.byteOffset)&field.fieldMask)!=0;
    else field.integer=Read<int32_t>(object,static_cast<size_t>(field.offset));
}
inline void* DiagnosticObjectField(void* object,const wchar_t* name){
    const auto offset=original_jett_priming::FieldOffset(Read<void*>(object,0x10),name,8);
    return offset<0?nullptr:Read<void*>(object,static_cast<size_t>(offset));
}
inline void ObserveMachine(SkillObservation& d,void* object,void* actor,void* forceManager=nullptr){
    if(!object||d.machinesCaptured==d.machines.size())return;
    for(size_t n=0;n<d.machinesCaptured;++n)if(d.machines[n].machine.object==object)return;
    auto& m=d.machines[d.machinesCaptured++];ObserveName(m.machine,object);
    m.typed=IsA(object,0x67722c0);m.owner=Read<void*>(object,0xc8);m.ownerMatches=m.owner==actor;
    m.ownerRole=Read<uint8_t>(m.owner,0x224);m.enabled=m.typed&&Read<uint8_t>(object,0x348)!=0;
    auto starting=Read<void*>(object,0x1e8);
    ObserveName(m.starting,starting);ObserveName(m.authority,Read<void*>(object,0x1c8));ObserveName(m.predicted,Read<void*>(object,0x1b0));
    ObserveName(m.selected,m.ownerRole==3?m.authority.object:m.predicted.object);
    m.startingOwnerMatches=starting&&Read<void*>(starting,0xc8)==actor;
    m.startingMachineMatches=starting&&Read<void*>(starting,0xe0)==object;
    m.selectedNonempty=m.selected.object!=nullptr;m.subframe=Read<float>(object,0x1e0);
    m.forceState=IsA(m.selected.object,0x63d9f60);m.forceHandle=0;
    m.forceModuleClass=m.forceManager=m.forceSource=nullptr;
    m.forceQueryAttempted=m.forceQueryValid=m.forceActive=false;
    if(m.forceState&&m.ownerMatches&&Read<void*>(m.selected.object,0xc8)==actor&&Read<void*>(m.selected.object,0xe0)==object){
        m.forceModuleClass=Read<void*>(m.selected.object,0x498);m.forceHandle=Read<uint64_t>(m.selected.object,0x4a0);
        m.forceManager=Read<void*>(m.selected.object,0x4a8);m.forceSource=Read<void*>(m.selected.object,0x4b0);
        if(forceManager&&m.forceManager==forceManager&&Read<void*>(forceManager,0xc8)==Read<void*>(actor,0x210)&&m.forceHandle){
            m.forceQueryAttempted=true;m.forceQueryValid=DiagnosticForceQuery(forceManager,m.forceHandle,m.forceActive);
        }
    }
}
inline void CaptureDaggerDiagnostics(Player& p){
    auto& d=p.daggerObserved;auto resource=p.skills[3].resource;
    if(resource!=d.resource){d={};d.resource=resource;}
    d.sourceValid=IsA(resource,original_jett::ResourceClass)&&Read<void*>(resource,0xc8)==p.skills[3].actor;
    if(!d.sourceValid)return;
    if(!d.fieldsCaptured){
        CaptureField(d.remaining,resource,L"DaggersRemaining",false);CaptureField(d.maximum,resource,L"MaxDaggers",false);
        CaptureField(d.shouldConsume,resource,L"ShouldConsumeUlt",true);CaptureField(d.shouldReset,resource,L"ShouldDaggersReset",true);
        auto fn=engine::Find((std::wstring(original_jett::DaggerResourcePath)+L".Multicast Update Daggers").c_str());
        d.multicastObserved=fn&&Read<void*>(fn,0x28)==Read<void*>(resource,0x10);
        if(d.multicastObserved){d.multicastFlags=Read<uint32_t>(fn,0xb8);d.multicastBytes=Read<uint16_t>(fn,0xbe);d.multicastScriptBytes=Read<int32_t>(fn,0x70);}
        d.fieldsCaptured=true;
    }
    SampleField(d.remaining,resource,false);SampleField(d.maximum,resource,false);
    SampleField(d.shouldConsume,resource,true);SampleField(d.shouldReset,resource,true);
    ObserveName(d.primaryClass,DiagnosticObjectField(p.fireStates[0].object,L"ProjectileClass"));
    ObserveName(d.burstClass,DiagnosticObjectField(p.fireStates[1].object,L"ProjectileClass"));
}
inline void CaptureGlideDiagnostics(Player& p){
    auto& g=p.glideObserved;g.slot=Read<void*>(p.inventory,0x1b8+6*8);g.sourceValid=false;g.matches=0;g.count=-1;
    g.slotSchema=IsA(g.slot,0x69cf3d0)&&Read<void*>(g.slot,0x40)==p.inventory
        &&original_jett_priming::FieldOffset(Read<void*>(g.slot,0x10),L"MultiContents",16)==0x50;
    if(!g.slotSchema){g.arrayValid=false;return;}
    auto items=Read<void**>(g.slot,0x50);g.count=Read<int32_t>(g.slot,0x58);const auto capacity=Read<int32_t>(g.slot,0x5c);
    g.arrayValid=g.count>=0&&g.count<=16&&capacity>=g.count&&(!g.count||original_players::Readable(items,static_cast<size_t>(g.count)*8));
    if(!g.arrayValid)return;
    auto type=engine::Find(L"/Game/Characters/Wushu/S0/Glide/Ability_Wushu_Passive_Glide.Ability_Wushu_Passive_Glide_C");
    void* actor=nullptr;
    if(type)for(int32_t n=0;n<g.count;++n)if(IsA(items[n],0x645c660)&&original_players::Derived(Read<void*>(items[n],0x10),type)){actor=items[n];++g.matches;}
    if(g.matches!=1||!actor)return;
    if(g.actor.object!=actor){g.graph={};g.active={};g.activeFieldCaptured=false;g.identity={};ObserveName(g.actor,actor);Identity(actor,g.identity);}
    g.role=Read<uint8_t>(actor,0x224);g.sourceValid=Live(actor,g.identity)&&Read<void*>(actor,0x210)==p.pawn&&Read<void*>(actor,0x4a8)==p.pawn;
    if(!g.sourceValid)return;
    if(!g.activeFieldCaptured){CaptureField(g.active,actor,L"IsActive",true);g.activeFieldCaptured=true;}
    SampleField(g.active,actor,true);auto& d=g.graph;d.machinesCaptured=0;
    ObserveMachine(d,Read<void*>(actor,0xe00),actor,p.forceManager);
    auto cache=Read<void**>(actor,0xe30);d.machineCacheCount=Read<int32_t>(actor,0xe38);const auto machineCapacity=Read<int32_t>(actor,0xe3c);
    d.machineCacheValid=d.machineCacheCount>=0&&d.machineCacheCount<=128&&machineCapacity>=d.machineCacheCount
        &&(!d.machineCacheCount||original_players::Readable(cache,static_cast<size_t>(d.machineCacheCount)*8));
    if(d.machineCacheValid)for(int32_t n=0;n<d.machineCacheCount;++n)ObserveMachine(d,cache[n],actor,p.forceManager);
    for(size_t n=d.machinesCaptured;n<d.machines.size();++n)d.machines[n]={};
}
inline void CaptureDiagnostics(Player& p){
    if(!Thread())return;const auto now=GetTickCount64();
    if(p.diagnosticSamples&&now-p.diagnosticAt<100)return;
    p.diagnosticAt=now;++p.diagnosticSamples;p.sourceMask=0;p.readyAtDiagnostic=p.ready;
    if(Live(p.pawn,p.pawnId))p.sourceMask|=1U;
    if(Live(p.playerState,p.stateId))p.sourceMask|=2U;
    if(Live(p.controller,p.controllerId))p.sourceMask|=4U;
    if(Read<void*>(p.pawn,0x488)==p.playerState)p.sourceMask|=8U;
    if(Read<void*>(p.controller,0x470)==p.playerState)p.sourceMask|=16U;
    if(Read<void*>(p.controller,0x4b0)==p.pawn)p.sourceMask|=32U;
    if(Read<uint8_t>(p.pawn,0x224)==p.role)p.sourceMask|=64U;
    if(p.authority==state.host)p.sourceMask|=128U;
    p.observedNativeInventory=Read<void*>(p.pawn,0xc08);p.observedNativeForceManager=Read<void*>(p.pawn,0x1100);
    for(auto& s:p.skills){auto& d=s.observed;d.prerequisiteMask=0;
        d.cachedInventory=p.inventory;d.nativeInventory=p.observedNativeInventory;
        d.cachedSlot=s.slot;d.nativeSlot=s.slotType<17?Read<void*>(d.nativeInventory,0x1b8+static_cast<size_t>(s.slotType)*8):nullptr;
        d.cachedContents=Read<void*>(d.cachedSlot,0x38);d.nativeContents=Read<void*>(d.nativeSlot,0x38);
        d.slotOuter=Read<void*>(d.nativeSlot,0x28);d.slotInventory=Read<void*>(d.nativeSlot,0x40);d.nativeSlotType=Read<uint8_t>(d.nativeSlot,0x30);
        d.slotTypeValid=IsA(d.nativeSlot,0x68f63c0)&&!IsA(d.nativeSlot,0x69cf3d0)&&d.nativeSlotType==s.slotType;
        d.slotOuterMatches=d.slotOuter==d.nativeInventory;d.slotInventoryMatches=d.slotInventory==d.nativeInventory;
        d.observedActor=d.nativeContents?d.nativeContents:s.actor;
        d.contentsMatch=s.actor&&d.nativeContents==s.actor&&d.cachedContents==s.actor;
        d.cachedActorMatchesNative=s.actor&&s.actor==d.nativeContents;
        d.actorIdentityObserved=d.observedActor==s.actor&&s.actor&&Live(s.actor,s.identity);
        d.actorOwner=Read<void*>(d.observedActor,0x210);d.typedOwner=Read<void*>(d.observedActor,0x4a8);
        d.actualRole=Read<uint8_t>(d.observedActor,0x224);d.actorFlags=Read<uint32_t>(d.observedActor,8);
        d.replicates=(Read<uint8_t>(d.observedActor,0x11a)&0x10U)!=0;
        ObserveName(d.expectedType,s.type);ObserveName(d.actualType,Read<void*>(d.observedActor,0x10));
        d.beginQueryAttempted=s.beginQueryAttempted;d.beginQueryValid=s.beginQueryValid;d.beginObserved=s.begunPlay;
        d.resource=s.resource;d.resourceOwner=Read<void*>(s.resource,0xc8);d.resourceLookupAttempted=s.resourceLookupAttempted;
        d.tuning=original_weapon::Component(d.observedActor,original_ability_tuning::ComponentClass);
        d.tuningMapCount=d.tuning?original_ability_tuning::MapCount(static_cast<uint8_t*>(d.tuning)+0xe8):-1;
        d.tuningArrayCount=d.tuning?Read<int32_t>(d.tuning,0x140):-1;
        local_server_resource_authority::ObserveRoute(d.observedActor,d.resource,d.resourceRoute);
        local_server_resource_authority::ObserveRoute(d.observedActor,d.tuning,d.tuningRoute);
        auto primary=Read<void*>(d.observedActor,0xe00);d.machinesCaptured=0;ObserveMachine(d,primary,d.observedActor,p.forceManager);
        auto cache=Read<void**>(d.observedActor,0xe30);d.machineCacheCount=Read<int32_t>(d.observedActor,0xe38);
        const auto capacity=Read<int32_t>(d.observedActor,0xe3c);
        d.machineCacheValid=d.observedActor&&d.machineCacheCount>=0&&d.machineCacheCount<=128&&capacity>=d.machineCacheCount
            &&(!d.machineCacheCount||original_players::Readable(cache,static_cast<size_t>(d.machineCacheCount)*8));
        if(d.machineCacheValid)for(int32_t n=0;n<d.machineCacheCount;++n)ObserveMachine(d,cache[n],d.observedActor,p.forceManager);
        for(size_t n=d.machinesCaptured;n<d.machines.size();++n)if(d.machines[n].machine.object)d.machines[n]={};
        const auto mark=[&](bool valid,uint32_t bit){if(valid)d.prerequisiteMask|=bit;};
        mark(s.type!=nullptr,ExpectedClass);mark(p.inventory&&p.inventory==d.nativeInventory,NativeInventoryCurrent);
        mark(d.slotTypeValid,NativeSingleSlot);mark(d.slotInventoryMatches&&d.nativeInventory,NativeSlotBackref);
        mark(d.contentsMatch,NativeSlotContents);mark(s.type&&d.actualType.object&&original_players::Derived(d.actualType.object,s.type),OriginalActorClass);
        mark(d.actorIdentityObserved,ActorIdentity);mark(d.actorOwner==p.pawn&&p.pawn,ActorOwner);mark(d.typedOwner==p.pawn&&p.pawn,TypedActorOwner);
        mark(d.beginQueryValid&&d.beginObserved,ActorBeginPlay);mark(IsA(primary,0x67722c0),PrimaryMachine);
        mark(primary&&Read<void*>(primary,0xc8)==d.observedActor,PrimaryMachineOwner);
        mark(IsA(s.resource,original_jett::ResourceClass),OriginalResource);mark(d.resource&&d.resourceOwner==d.observedActor,ResourceOwner);
        mark(s.eventsReady,OriginalEventRegistration);mark(d.observedActor&&(p.authority?d.actualRole==3:d.actualRole==1||d.actualRole==2),ActualNetworkRole);
        d.missingMask=RequiredPrerequisites&~d.prerequisiteMask;d.waiting="prerequisites_observed";
        for(unsigned bit=0;bit<16;++bit)if(d.missingMask&(1U<<bit)){d.waiting=PrerequisiteNames[bit];break;}
        if((d.missingMask&OriginalResource)&&!d.resourceLookupAttempted)d.waiting=(d.missingMask&~uint32_t(OriginalResource|ResourceOwner|OriginalEventRegistration))?d.waiting:"resource_lookup_not_reached";
        d.connection=nullptr;d.channel=nullptr;d.channelSnapshotValid=d.channelFound=d.channelClosing=d.channelOpenAck=false;d.channelFlags=d.channelOpenFlags=0;
    }
    CaptureDaggerDiagnostics(p);CaptureGlideDiagnostics(p);
    auto driver=Read<void*>(state.world,0x40);auto connection=state.host?Read<void*>(p.controller,0x510):Read<void*>(driver,0xc0);
    struct Channels{void** data;int32_t count,capacity;};const auto channels=Read<Channels>(connection,0x78);
    const bool channelsValid=connection&&Read<void*>(connection,0x60)==driver&&Read<void*>(connection,0x38)==p.controller
        &&channels.count>=0&&channels.count<=512&&channels.capacity>=channels.count
        &&(!channels.count||original_players::Readable(channels.data,static_cast<size_t>(channels.count)*8));
    if(channelsValid){
        for(auto& s:p.skills){s.observed.connection=connection;s.observed.channelSnapshotValid=true;}
        for(int32_t n=0;n<channels.count;++n){auto channel=channels.data[n];
            if(!channel||Read<void*>(channel,0x30)!=connection||!IsA(channel,0x3dd3d40))continue;
            auto actor=Read<void*>(channel,0x78);
            for(auto& s:p.skills){auto& d=s.observed;if(!d.observedActor||actor!=d.observedActor)continue;
                d.channel=channel;d.channelFound=true;d.channelFlags=Read<uint8_t>(channel,0x38);d.channelOpenFlags=Read<uint8_t>(channel,0xa0);
                d.channelClosing=(d.channelFlags&2U)!=0;d.channelOpenAck=(d.channelOpenFlags&1U)!=0;
            }
        }
    }
    p.waiting=p.sourceMask!=255U?"actual_player_source_links":p.inventory!=p.observedNativeInventory?"registered_inventory_differs_from_native_replica":"original_skill_prerequisites";
    bool all=true;for(const auto& s:p.skills)all&=s.observed.missingMask==0;
    if(all)p.waiting=!p.startingStatesReady?"original_machine_starting_states":!p.primingReady?"original_E_priming_source_or_graph":!p.daggerBound?"original_X_lifecycle_bindings":p.ready?"original_graph_ready_execution_unverified":"ready_not_observed";
}
struct DiagnosticScope {Player& player;~DiagnosticScope(){CaptureDiagnostics(player);}};
inline void* ObjectField(void* object,const wchar_t* name){
    const auto offset=original_jett_priming::FieldOffset(Read<void*>(object,0x10),name,8);
    return offset<0?nullptr:Read<void*>(object,static_cast<size_t>(offset));
}
inline bool Script(void* fn,void* owner,int32_t bytes,uint16_t parameters){
    auto code=Read<uint8_t*>(fn,0x68);
    return fn&&owner&&Read<void*>(fn,0x28)==owner&&Read<uint16_t>(fn,0xbe)==parameters
        &&Read<int32_t>(fn,0x70)==bytes&&original_players::Readable(code,static_cast<size_t>(bytes))&&code[bytes-1]==0x53;
}
inline bool Components(Skill& s){
    struct Query {void* type;void** data=nullptr;int32_t count=0,capacity=0;} q{engine::Class(0x36837a0)};
    if(!Event(s.actor,L"/Script/Engine.Actor.K2_GetComponentsByClass",24,&q))return Fail(4401,"network_abilities_component_query_failed");
    struct Allocation {void* p;~Allocation(){if(p)engine::function<void(*)(void*)>(0x1915720)(p);}} allocation{q.data};
    if(q.count<0||q.count>static_cast<int32_t>(MaxComponents)||q.capacity<q.count
        ||(q.count&&!original_players::Readable(q.data,static_cast<size_t>(q.count)*8)))
        return Fail(4402,"network_abilities_component_query_layout_invalid");
    s.componentCount=0;
    for(int32_t i=0;i<q.count;++i){
        if(!IsA(q.data[i],0x36837a0)||Read<void*>(q.data[i],0xc8)!=s.actor)
            return Fail(4403,"network_abilities_component_owner_mismatch");
        s.components[s.componentCount++]=q.data[i];
    }
    return true;
}
inline bool RegisterEvent(Player& p,Skill& s,uintptr_t typeGetter){
    auto component=Read<void*>(s.actor,0xe18),registry=Read<void*>(component,0x198),type=engine::Class(typeGetter);
    if(!IsA(component,0x6768f30)||Read<void*>(component,0xc8)!=s.actor||!Read<uint8_t>(component,0x189)
        ||!registry||!type||engine::function<void*(*)(void*)>(0x6ff6000)(s.actor)!=registry)
        return Fail(4404,"network_abilities_native_event_registry_missing");
    // Original ServerOnly1 is stripped. Mode0 builds the original per-item set
    // and original central pool on both network roles; it emits no event.
    engine::function<void(*)(void*,void*,uint8_t)>(0x6ff5f40)(component,type,0);
    if(!engine::function<bool(*)(void*,void*)>(0x6ff68d0)(registry,type))
        return Fail(4405,"network_abilities_native_event_registration_unobserved");
    ++p.eventRegistrations;return true;
}
inline bool Owner(Player& p,Skill& s){
    s.ownerReady=false;
    s.role=Read<uint8_t>(s.actor,0x224);
    if((p.authority&&s.role!=3)||(!p.authority&&s.role!=2&&s.role!=1))return Fail(4406,"network_abilities_unexpected_actual_actor_role");
    if(Read<void*>(s.actor,0x210)!=p.pawn)return !p.authority;
    if(Read<void*>(s.actor,0x4a8)!=p.pawn){
        auto table=Read<void**>(s.actor,0);auto callback=Read<void*>(table,0x7d8);
        if(callback!=engine::function<void*>(0x70ec480))return Fail(4407,"network_abilities_original_owner_callback_changed");
        // AresItem's typed owner is not a replicated property. The original
        // AActor owner has already arrived; complete its original callback.
        reinterpret_cast<void(*)(void*,void*,void*)>(callback)(s.actor,Read<void*>(s.actor,0x4a8),p.pawn);
        ++p.ownerCallbacks;
    }
    s.ownerReady=Read<void*>(s.actor,0x4a8)==p.pawn;
    return s.ownerReady||Fail(4408,"network_abilities_typed_owner_callback_incomplete");
}
inline bool StartingStates(Player& p){
    bool ready=true;
    for(auto& s:p.skills){
        // An owned client item still arriving as native simulated Role1 cannot
        // predict. Original item70ED690 deliberately disables that role. Wait
        // for normal owned RemoteRole publication and native RoleChanged.
        if(!p.authority&&Read<uint8_t>(s.actor,0x224)!=2)ready=false;
        auto cached=Read<void**>(s.actor,0xe30);const auto cachedCount=Read<int32_t>(s.actor,0xe38);
        const bool cacheValid=cachedCount>0&&cachedCount<=128&&original_players::Readable(cached,size_t(cachedCount)*8);
        if(!p.authority&&!cacheValid)ready=false;
        if(!p.authority&&cacheValid)for(int32_t k=0;k<cachedCount;++k){
            auto machine=cached[k];
            if(!IsA(machine,0x67722c0)||Read<void*>(machine,0xc8)!=s.actor
               ||!Read<uint8_t>(machine,0x348)||!Read<void*>(machine,0x1b0))ready=false;
        }
        for(size_t n=0;n<s.componentCount;++n){auto machine=s.components[n];
            if(!IsA(machine,0x67722c0))continue;
            bool inOriginalCache=false;
            if(cacheValid)for(int32_t k=0;k<cachedCount;++k)if(cached[k]==machine){inOriginalCache=true;break;}
            if(!Read<uint8_t>(machine,0x348)){if(!p.authority&&inOriginalCache)ready=false;continue;}
            auto starting=Read<void*>(machine,0x1e8);
            if(!starting||Read<void*>(starting,0xc8)!=s.actor||Read<void*>(starting,0xe0)!=machine)
                return Fail(4409,"network_abilities_authored_starting_state_owner_invalid");
            void* current=nullptr;
            if(!Event(machine,L"/Script/ShooterGame.StateMachineComponent.GetCurrentState",8,&current))
                return Fail(4410,"network_abilities_original_current_state_query_failed");
            if(current)continue;
            // Only the completely missing authored initialization is filled.
            // An authority correction or live predicted state is never replaced.
            if(Read<void*>(machine,0x1b0)||Read<void*>(machine,0x1c8)){ready=false;continue;}
            if(!Event(machine,L"/Script/ShooterGame.StateMachineComponent.InitializeStartingState",8,&s.actor))
                return Fail(4411,"network_abilities_original_start_initializer_missing");
            ++s.initializations;
            if(!Event(machine,L"/Script/ShooterGame.StateMachineComponent.GetCurrentState",8,&current)||!current)ready=false;
        }
    }
    p.startingStatesReady=ready;return true;
}
inline bool PrimeSource(Player& p,original_jett_priming::Diagnostics& d){
    if(!IsA(d.tuning,original_ability_tuning::ComponentClass)||Read<void*>(d.tuning,0xc8)!=d.ability
        ||!Event(d.tuning,L"/Script/ShooterGame.AbilityTuningComponent.GetAbilityTuningTable",8,&d.sourceTable)
        ||!IsA(d.sourceTable,original_ability_tuning::DataTableClass))return p.authority?Fail(4412,"network_abilities_authority_tuning_source_missing"):true;
    if(Read<void*>(d.sourceTable,0x30)!=engine::Find(L"/Script/ShooterGame.AbilityTuningDataTableRow"))return Fail(4412,"network_abilities_prime_source_row_type_mismatch");
    original_ability_tuning::NativeName tag{};
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&tag,original_jett_priming::DurationTag,1);
    d.runtimeValue=original_ability_tuning::FindValue(static_cast<uint8_t*>(d.tuning)+0xe8,&tag);
    // Wait for the host's real tuning map on clients. No client-side import.
    if(!original_players::Readable(d.runtimeValue,24))return p.authority?Fail(4412,"network_abilities_authority_tuning_map_missing"):true;
    if(Read<uint8_t>(d.runtimeValue,0)!=2)return Fail(4412,"network_abilities_prime_duration_type_mismatch");
    struct Names {void* table;original_ability_tuning::NativeName* data=nullptr;int32_t count=0,capacity=0;} names{d.sourceTable};
    const bool queried=Event(engine::DefaultObject(engine::Class(0x42f1030)),L"/Script/Engine.DataTableFunctionLibrary.GetDataTableRowNames",24,&names);
    struct Allocation {void* p;~Allocation(){if(p)engine::function<void(*)(void*)>(0x1915720)(p);}} allocation{names.data};
    if(!queried||names.count<=0||names.count>512||names.capacity<names.count
        ||!original_players::Readable(names.data,static_cast<size_t>(names.count)*12))return Fail(4413,"network_abilities_prime_source_rows_unavailable");
    int matches=0;
    for(int32_t i=0;i<names.count;++i){
        auto row=engine::function<uint8_t*(*)(void*,const original_ability_tuning::NativeName*)>(0x42ee820)(d.sourceTable,names.data+i);
        if(!original_players::Readable(row,48))return Fail(4414,"network_abilities_prime_source_row_unreadable");
        if(std::memcmp(row+8,&tag,8)==0){++matches;d.sourceRow=row;std::memcpy(&d.sourceBytes,row+24,24);}
    }
    d.source=local_server_abilities::Read<float>(&d.sourceBytes,8);
    d.sourceValid=matches==1&&std::isfinite(d.source)&&d.source>0&&local_server_abilities::Read<uint8_t>(&d.sourceBytes,0)==2
        &&std::memcmp(d.runtimeValue,&d.sourceBytes,24)==0;
    return d.sourceValid||Fail(4415,"network_abilities_prime_source_or_runtime_override_conflict");
}
inline bool PrimeInitial(Player& p,original_jett_priming::Diagnostics& d){
    struct Activation {uint8_t slot,value=255;} activation{p.skills[2].slotType};
    if(!Event(p.pawn,L"/Script/ShooterGame.ShooterCharacter.GetCurrentActivationInputState",2,&activation))return false;
    int32_t forces=-1;
    void *primary=nullptr,*secondary=nullptr;
    return activation.value==0&&p.skills[2].transitions==0
        &&Event(p.forceManager,L"/Script/ShooterGame.ForceModuleManagerComponent.GetActiveForceModuleCount",4,&forces)&&forces==0
        &&Event(d.primaryMachine,L"/Script/ShooterGame.StateMachineComponent.GetCurrentState",8,&primary)
        &&Event(d.primingMachine,L"/Script/ShooterGame.StateMachineComponent.GetCurrentState",8,&secondary)
        &&primary==d.inactive&&secondary==d.primeIdle
        &&Read<void*>(d.primaryMachine,0x1e8)==primary&&Read<void*>(d.primingMachine,0x1e8)==secondary;
}
inline bool Prime(Player& p){
    if(p.primingReady)return true;
    auto& d=p.priming;auto& s=p.skills[2];
    d.pawn=p.pawn;d.ability=s.actor;d.type=s.type;d.tuning=ObjectField(s.actor,L"AbilityTuning");
    d.primed=ObjectField(s.actor,L"Prime_Primed");d.primeIdle=ObjectField(s.actor,L"Prime_Idle");
    d.primingMachine=ObjectField(s.actor,L"StateMachine_Priming");d.primaryMachine=s.machine;d.inactive=Read<void*>(s.machine,0x350);
    if(!IsA(d.primed,0x63d9390)||!IsA(d.primeIdle,0x6d6e830)||!IsA(d.primingMachine,0x67722c0)
        ||Read<void*>(d.primed,0xc8)!=s.actor||Read<void*>(d.primed,0xe0)!=d.primingMachine
        ||Read<void*>(d.primeIdle,0xc8)!=s.actor||Read<void*>(d.primeIdle,0xe0)!=d.primingMachine)
        return Fail(4416,"network_abilities_original_prime_graph_relationships_invalid");
    if(!PrimeSource(p,d))return false;
    if(!d.sourceValid){state.stage="network_abilities_waiting_replicated_tuning";return true;}
    if(!Event(d.primed,L"/Script/ShooterGame.TimedStateComponent.GetTimerLength",4,&d.before)||!std::isfinite(d.before))
        return Fail(4417,"network_abilities_original_prime_timer_query_failed");
    if(d.before==d.source){d.ready=d.unchanged=p.primingReady=true;d.after=d.before;return true;}
    if(d.before!=0||Read<uint64_t>(d.primed,0x48c)!=0)return Fail(4418,"network_abilities_nonempty_prime_duration_preserved");
    if(!PrimeInitial(p,d))return Fail(4419,"network_abilities_prime_catchup_requires_unactivated_original_states");
    // Reuse the old exact Blueprint/persistent-frame profile validator. Its
    // diagnostic scratch is adapter-only and restored on exit; engine ownership,
    // roles and LocalPlayer references are never changed by this scope.
    struct Scratch {original_jett_priming::Diagnostics saved;~Scratch(){original_jett_priming::state=saved;}} scratch{original_jett_priming::state};
    original_jett_priming::state=d;
    const bool profile=original_jett_priming::Profile();d=original_jett_priming::state;
    if(!profile)return Fail(d.error?d.error:4420,"network_abilities_original_prime_graph_profile_mismatch");
    int32_t entry=original_jett_priming::OriginalEntry;d.attempted=true;++d.originalCalls;
    if(!Event(s.actor,original_jett_priming::GraphPath,4,&entry))return Fail(4421,"network_abilities_original_prime_query_dispatch_failed");
    ++d.completedCalls;
    d.originalQueryValue=Read<float>(d.frame,static_cast<size_t>(d.outValueOffset));
    d.originalQuerySucceeded=static_cast<uint8_t>(Read<uint8_t>(d.frame,static_cast<size_t>(d.successOffset))&d.successProperty.boolFieldMask);
    d.postconditions=Event(d.primed,L"/Script/ShooterGame.TimedStateComponent.GetTimerLength",4,&d.after)
        &&d.after==d.source&&d.originalQueryValue==d.source&&d.originalQuerySucceeded
        &&Read<void*>(s.actor,static_cast<size_t>(d.frameOffset))==d.frame
        &&std::memcmp(d.runtimeValue,&d.sourceBytes,24)==0
        &&std::memcmp(static_cast<uint8_t*>(d.sourceRow)+24,&d.sourceBytes,24)==0&&PrimeInitial(p,d);
    if(!d.postconditions){state.restartRequired=true;return Fail(4422,"network_abilities_original_prime_readback_failed");}
    d.ready=p.primingReady=true;return true;
}

inline void ObserveEvent(void*,void*,void*);
inline bool NativeTable(void** table){
    MEMORY_BASIC_INFORMATION m{};
    return table&&VirtualQuery(table,&m,sizeof(m))&&m.State==MEM_COMMIT&&m.Type==MEM_IMAGE
        &&m.AllocationBase==reinterpret_cast<void*>(engine::image)&&!(m.Protect&(PAGE_GUARD|PAGE_NOACCESS));
}
inline Table* FindTable(void* object){
    auto actual=Read<void**>(object,0);
    for(auto& t:tables)if(t.parent&&(t.copy==actual||t.parent==actual))return &t;
    return nullptr;
}
inline bool Observe(void* object){
    auto actual=Read<void**>(object,0);
    for(auto& t:tables)if(t.copy==actual)return true;
    if(!NativeTable(actual))return Fail(4423,"network_abilities_foreign_event_table_requires_composition");
    auto event=Read<void*>(actual,0x298);
    const bool actor=IsA(object,0x3683900),component=IsA(object,0x36837a0);
    if((actor&&event!=engine::function<void*>(0x3eda0d0))||(component&&event!=engine::function<void*>(0x1cdc850))||(!actor&&!component))
        return Fail(4424,"network_abilities_native_event_parent_mismatch");
    size_t count=0;
    for(;count<MaxFunctions;++count){
        auto entry=Read<void*>(actual,count*8);MEMORY_BASIC_INFORMATION m{};
        if(!entry||!VirtualQuery(entry,&m,sizeof(m))||m.State!=MEM_COMMIT||m.Type!=MEM_IMAGE
            ||m.AllocationBase!=reinterpret_cast<void*>(engine::image)||(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)))break;
        const auto flags=m.Protect&0xffU;
        if(flags!=PAGE_EXECUTE&&flags!=PAGE_EXECUTE_READ&&flags!=PAGE_EXECUTE_READWRITE&&flags!=PAGE_EXECUTE_WRITECOPY)break;
    }
    if(count<=EventSlot||count==MaxFunctions||!original_players::Readable(actual-1,(count+1)*8))
        return Fail(4425,"network_abilities_original_event_table_extent_invalid");
    Table* table=nullptr;
    for(auto& t:tables)if(t.parent==actual){table=&t;break;}
    if(!table){
        for(auto& t:tables)if(!t.parent){table=&t;break;}
        if(!table)return Fail(4426,"network_abilities_event_table_capacity");
        auto allocation=static_cast<void**>(VirtualAlloc(nullptr,(count+1)*8,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
        if(!allocation)return Fail(4427,"network_abilities_event_table_allocation_failed");
        std::memcpy(allocation,actual-1,(count+1)*8);allocation[EventSlot+1]=reinterpret_cast<void*>(&ObserveEvent);
        DWORD previous=0;
        if(!VirtualProtect(allocation,(count+1)*8,PAGE_READONLY,&previous))return Fail(4428,"network_abilities_event_table_protection_failed");
        table->count=count;table->event=reinterpret_cast<EventFn>(event);table->copy=allocation+1;MemoryBarrier();table->parent=actual;
    }
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(object),table->copy,actual)!=actual)
        return Fail(4429,"network_abilities_event_table_install_conflict");
    ++state.tablesInstalled;return true;
}
inline bool Delegate(FireState& f,const wchar_t* fieldName,void* function,bool& bound){
    original_jett_priming::PropertyInfo field{};
    const auto offset=original_jett_priming::FieldOffset(Read<void*>(f.object,0x10),fieldName,16,&field);
    if(offset<0||(field.elementSize!=16&&field.elementSize!=24)||!function)
        return Fail(4430,"network_abilities_original_fire_delegate_layout_invalid");
    auto address=static_cast<uint8_t*>(f.object)+offset;
    auto initialEntries=Read<uint8_t*>(address,0);const auto initialCount=Read<int32_t>(address,8),initialCapacity=Read<int32_t>(address,12);
    if(initialCount<0||initialCount>256||initialCapacity<initialCount
        ||(initialCount&&!original_players::Readable(initialEntries,static_cast<size_t>(initialCount)*20)))
        return Fail(4430,"network_abilities_original_fire_delegate_array_invalid");
    const auto contains=[&](){
        auto entries=Read<uint8_t*>(address,0);auto count=Read<int32_t>(address,8),capacity=Read<int32_t>(address,12);
        if(count<0||count>256||capacity<count||(count&&!original_players::Readable(entries,static_cast<size_t>(count)*20)))return false;
        for(int32_t n=0;n<count;++n){auto entry=entries+n*20;
            if(original_inventory_slots::ResolveIdentity(Read<Weak>(entry,0))==f.object
                &&std::memcmp(entry+8,static_cast<uint8_t*>(function)+0x18,12)==0)return true;}
        return false;
    };
    if(contains()){bound=true;return true;}
    struct NativeDelegate {Weak object;uint8_t name[12]{};} native{f.identity};static_assert(sizeof(NativeDelegate)==20);
    std::memcpy(native.name,static_cast<uint8_t*>(function)+0x18,12);
    engine::function<void(*)(void*,const void*)>(0x1cf4e70)(address,&native);
    bound=contains();return bound||Fail(4431,"network_abilities_original_fire_delegate_readback_failed");
}
inline bool DaggerBindings(Player& p){
    if(p.daggerBound)return true;
    auto ability=p.skills[3].actor,type=p.skills[3].type;
    constexpr const wchar_t* names[]{
        L"BndEvt__Ability_Wushu_X_Dagger_Production_Projectile0CommitState_K2Node_ComponentBoundEvent_9_OnStateEnterSignature__DelegateSignature",
        L"BndEvt__Ability_Wushu_X_Dagger_Production_Projectile1CommitState_K2Node_ComponentBoundEvent_10_OnStateEnterSignature__DelegateSignature",
        L"BndEvt__Ability_Wushu_X_Dagger_Production_Projectile2CommitState_K2Node_ComponentBoundEvent_13_OnStateEnterSignature__DelegateSignature",
        L"BndEvt__Ability_Wushu_X_Dagger_Production_Projectile3CommitState_K2Node_ComponentBoundEvent_14_OnStateEnterSignature__DelegateSignature",
        L"BndEvt__Ability_Wushu_X_Dagger_Production_Projectile4CommitState_K2Node_ComponentBoundEvent_15_OnStateEnterSignature__DelegateSignature"};
    for(size_t i=0;i<5;++i){primaryCallbacks[i]=engine::Find((std::wstring(XPrefix)+names[i]).c_str());
        if(!Script(primaryCallbacks[i],type,72,20))return Fail(4432,"network_abilities_original_primary_commit_profile_mismatch");}
    burstCallback=engine::Find((std::wstring(XPrefix)+L"BndEvt__ProjectileCommit2State_K2Node_ComponentBoundEvent_3_OnStateEnterSignature__DelegateSignature").c_str());
    fireEnter=engine::Find((std::wstring(FirePrefix)+L"StateEnterEvent").c_str());
    fireExit=engine::Find((std::wstring(FirePrefix)+L"OnStateExit_Event_0").c_str());
    fireProjectiles=engine::Find((std::wstring(FirePrefix)+L"FireProjectiles").c_str());
    fireGraph=engine::Find((std::wstring(FirePrefix)+L"ExecuteUbergraph_FireProjectileMulticast_StateComponent").c_str());
    if(!Script(burstCallback,type,72,20)||!Script(fireEnter,Read<void*>(fireEnter,0x28),72,20)
        ||!Script(fireExit,Read<void*>(fireExit,0x28),54,16)
        ||!Script(fireProjectiles,Read<void*>(fireEnter,0x28),1004,64)
        ||!Script(fireGraph,Read<void*>(fireEnter,0x28),1134,4))return Fail(4433,"network_abilities_original_dagger_fire_profile_mismatch");
    constexpr const wchar_t* fields[]{L"ProjectileFireState",L"BurstProjectileFireState"};
    for(size_t i=0;i<2;++i){auto& f=p.fireStates[i];f.object=ObjectField(ability,fields[i]);
        if(!f.object||Read<void*>(f.object,0xc8)!=ability||!Identity(f.object,f.identity))return Fail(4434,"network_abilities_original_dagger_fire_state_missing");
        if(!Delegate(f,L"OnStateEnter",fireEnter,f.enterBound)||!Delegate(f,L"OnStateExit",fireExit,f.exitBound)||!Observe(f.object))return false;
    }
    if(!Observe(ability))return false;
    if(p.authority&&!Observe(p.playerState))return false;
    p.daggerBound=true;return true;
}
struct Before {
    Player* player=nullptr;bool primary=false,burst=false,consumeUlt=false,ultActive=false;
    uint8_t active=0;int32_t remaining=-1,points=-1;
    FireState* fire=nullptr;uint64_t fireSequence=0;
};
inline bool CaptureFactoryResult(Player& p,FireState& f,uint64_t sequence);
inline bool Remaining(Player& p,int32_t& remaining){
    return Event(p.skills[3].resource,original_jett::DaggerRemaining,4,&remaining)&&remaining>=0&&remaining<=64;
}
inline bool Fresh(Player& p,void* entered,void* function){
    auto machine=Read<void*>(entered,0xe0);Weak enteredId{},machineId{};
    const auto timestamp=Read<float>(machine,0x1e0);uint32_t bits=0;std::memcpy(&bits,&timestamp,4);
    if(!IsA(machine,0x67722c0)||!std::isfinite(timestamp)||!Identity(entered,enteredId)||!Identity(machine,machineId))return false;
    for(const auto& c:p.commits)if(c.function==function&&c.timestamp==bits
        &&original_inventory_slots::SameIdentity(c.state,enteredId)&&original_inventory_slots::SameIdentity(c.machine,machineId)){
        ++p.duplicateCommits;return false;}
    p.commits[p.commitCursor++%p.commits.size()]={enteredId,machineId,bits,function};return true;
}
inline void PrepareEvent(void* object,void* function,void* parameters,Before& b){
    if(!Thread()||state.error||!state.practice)return;
    for(auto& p:players){if(!p.used||!p.daggerBound||!Source(p))continue;
        if(object==p.playerState&&p.authority){
            if(function==consumeUltimate&&Read<uint16_t>(function,0xbe)==0){b.player=&p;b.consumeUlt=true;b.points=Read<int32_t>(p.playerState,0xbdc);}
            if((function==setUltActive||function==setUltimateActive)&&Read<uint16_t>(function,0xbe)==1&&parameters){b.player=&p;b.ultActive=true;b.active=Read<uint8_t>(parameters,0)?1:0;}
        }
        if(object==p.skills[3].actor&&parameters&&Read<uint16_t>(function,0xbe)==20){
            bool primary=false;for(auto fn:primaryCallbacks)primary|=fn==function;const bool burst=function==burstCallback;
            auto entered=Read<void*>(parameters,0);
            if((primary||burst)&&Read<void*>(entered,0xc8)==object&&Fresh(p,entered,function)){
                b.player=&p;b.primary=primary;b.burst=burst;Remaining(p,b.remaining);
                if(primary)++p.primaryCommits;else ++p.burstCommits;
            }
        }
        for(size_t kind=0;kind<2;++kind){auto& f=p.fireStates[kind];
            if(object!=f.object||!parameters||!Live(f.object,f.identity))continue;
            const bool entry=function==fireEnter&&Read<void*>(parameters,0)==object;
            const bool factory=function==fireProjectiles&&Read<uint16_t>(function,0xbe)==64;
            if(!entry&&!factory)continue;
            b.player=&p;b.fire=&f;
            if(!f.activeDepth){
                if(state.fireSequences==UINT64_MAX){Fail(4483,"network_abilities_native_fire_sequence_exhausted");return;}
                f.activeSequence=++state.fireSequences;
                // Consumer captures native life before the original factory,
                // associated with this real source and callback occurrence.
                if(fireBoundarySink&&p.authority)fireBoundarySink(p.pawn,f.object,f.activeSequence,true);
            }
            ++f.activeDepth;b.fireSequence=f.activeSequence;
            if(!entry)continue;
            ++f.entries;auto before=ObjectField(f.object,L"ProjectileClass");if(before)continue;
            const auto path=std::wstring(XPrefix)+(kind?L"UpdateDaggerClassForIndexBurst":L"UpdateDaggerClassForIndex");
            auto fn=engine::Find(path.c_str());
            // Execute its actual index/class selection only at the real fire
            // entry. No class array, dagger index or launch vector is fabricated.
            if(!Script(fn,p.skills[3].type,177,8)||!Event(p.skills[3].actor,path.c_str(),8,&f.object)
                ||!ObjectField(f.object,L"ProjectileClass")){Fail(4435,"network_abilities_original_dagger_class_selection_failed");++state.eventPrepareFailures;}
            else ++f.classSupplies;
        }
    }
}
inline void AfterEvent(Before& b){
    if(b.fire&&b.player){
        auto& f=*b.fire;
        if(Thread()&&!state.error&&Source(*b.player)&&Live(f.object,f.identity))CaptureFactoryResult(*b.player,f,b.fireSequence);
        if(f.activeDepth)--f.activeDepth;
        if(!f.activeDepth){
            if(fireBoundarySink&&b.player->authority)fireBoundarySink(b.player->pawn,f.object,b.fireSequence,false);
            f.activeSequence=0;
        }
    }
    if(!b.player||!Thread()||state.error||!Source(*b.player)||!b.player->authority)return;
    auto& p=*b.player;
    if(b.consumeUlt&&b.points>0&&Read<int32_t>(p.playerState,0xbdc)==b.points){
        // Original AuthConsumeUltimatePoints is the no-op exec3b866f0.
        // Supply its host record only when the authored resource requested it.
        int32_t zero=0;
        if(!original_players::Write(p.playerState,0xbdc,zero)||!Event(p.playerState,L"/Script/ShooterGame.ShooterPlayerState.OnRep_NumUltimatePoints",0,nullptr))Fail(4436,"network_abilities_ultimate_consumption_notification_failed");
        else ++p.ultimateConsumes;
    }
    if(b.ultActive&&Read<uint8_t>(p.playerState,0xbd8)!=b.active){
        if(!original_players::Write(p.playerState,0xbd8,b.active)||!Event(p.playerState,L"/Script/ShooterGame.ShooterPlayerState.OnRep_UltimateActive",0,nullptr))Fail(4437,"network_abilities_ultimate_active_notification_failed");
        else ++p.ultimateActiveUpdates;
    }
    if((!b.primary&&!b.burst)||b.remaining<=0)return;
    int32_t after=-1;if(!Remaining(p,after)){Fail(4438,"network_abilities_original_dagger_resource_query_failed");return;}
    if(after!=b.remaining){++p.alreadyConsumed;return;}
    // The original commit's consume leaf is gated on DedicatedServer. This
    // local listen host has real authority; use the original resource leaf
    // after that exact commit instead of changing the world's server query.
    const auto path=std::wstring(original_jett::DaggerResourcePath)+(b.burst?L".ConsumeAllDaggers":L".ConsumeDagger");
    auto fn=engine::Find(path.c_str());
    if(!Script(fn,Read<void*>(p.skills[3].resource,0x10),b.burst?55:196,0)||!Event(p.skills[3].resource,path.c_str(),0,nullptr))Fail(4439,"network_abilities_original_dagger_consume_leaf_missing");
    else ++p.consumes;
}
inline void PrepareSafe(void* object,void* function,void* parameters,Before& b){__try{PrepareEvent(object,function,parameters,b);}__except(Capture(GetExceptionInformation())){}}
inline void AfterSafe(Before& b){__try{AfterEvent(b);}__except(Capture(GetExceptionInformation())){}}
__declspec(noinline) inline void ObserveEvent(void* object,void* function,void* parameters){
    auto table=FindTable(object);
    if(!table||!table->event){Fail(4440,"network_abilities_original_event_forwarder_missing");return;}
    Before before{};PrepareSafe(object,function,parameters,before);
    ++state.eventForwards;table->event(object,function,parameters);AfterSafe(before);
}
inline bool Grant(Player& p){
    if(!p.authority)return true;
    if(!original_inventory_slots::EnsureForPawn(p.pawn))return Fail(original_inventory_slots::State().error,original_inventory_slots::State().stage);
    for(size_t i=0;i<SkillCount;++i){auto& s=p.skills[i];
        if(!s.type){s.type=original_jett::LoadEquippableClass(original_jett::SkillPaths[i]);
            if(!s.type||!original_players::Derived(s.type,engine::Class(0x645c660)))return Fail(4441,"network_abilities_original_class_load_failed");
            s.slotType=Read<uint8_t>(engine::DefaultObject(s.type),0x4a0);
            if(s.slotType!=3&&s.slotType!=4&&s.slotType!=5&&s.slotType!=9)return Fail(4442,"network_abilities_original_cdo_slot_invalid");
            for(size_t k=0;k<i;++k)if(p.skills[k].slotType==s.slotType)return Fail(4443,"network_abilities_duplicate_original_ability_slots");
        }
        s.slot=Read<void*>(p.inventory,0x1b8+static_cast<size_t>(s.slotType)*8);
        if(!IsA(s.slot,0x68f63c0)||IsA(s.slot,0x69cf3d0)||Read<void*>(s.slot,0x40)!=p.inventory)
            return Fail(4444,"network_abilities_original_single_slot_missing");
        auto contents=Read<void*>(s.slot,0x38);
        if(contents){
            if(!original_players::Derived(Read<void*>(contents,0x10),s.type))return Fail(4445,"network_abilities_original_slot_occupied_by_different_item");
            s.actor=contents;s.adopted=!s.spawned;
        }else{
            if(s.spawnAttempted)return Fail(4446,"network_abilities_partial_factory_requires_new_session");
            s.spawnAttempted=true;engine::Vec location{};
            if(!Event(p.pawn,L"/Script/Engine.Actor.K2_GetActorLocation",24,&location)||!original_players::Finite(location))return Fail(4447,"network_abilities_authority_pawn_location_unavailable");
            engine::Transform transform;transform.translation=location;
            s.actor=engine::function<void*(*)(void*,void*,const engine::Transform*,uint8_t,void*,uint8_t)>(0x43fada0)(p.pawn,s.type,&transform,1,p.pawn,0);
            if(!s.actor)return Fail(4448,"network_abilities_authority_original_factory_failed");
            s.spawned=true;
            engine::function<void(*)(void*,const engine::Transform*,bool,void*,uint8_t)>(0x3ef6cb0)(s.actor,&transform,false,nullptr,0);
            if(!original_ability_tuning::EnsureActor(s.actor)||!original_effects::EnsureActor(s.actor,"network_authority_ability"))return Fail(4449,"network_abilities_original_tuning_or_effects_incomplete");
            if(!Event(s.actor,L"/Script/Engine.Actor.SetOwner",8,&p.pawn)||Read<void*>(s.actor,0x210)!=p.pawn||!Owner(p,s))return Fail(4450,"network_abilities_original_authority_owner_failed");
            uint8_t one=1;
            if(!Event(s.actor,L"/Script/Engine.Actor.SetReplicates",1,&one))return Fail(4452,"network_abilities_native_replication_enable_failed");
            if(!local_server_item_ownership::Prepare(state.world,p.controller,p.playerState,p.pawn,p.inventory,s.actor,s.slot,
                local_server_item_ownership::Source::PendingAuthorityGrant))
                return Fail(local_server_item_ownership::GetState().error,local_server_item_ownership::GetState().stage);
            const auto ownership=local_server_item_ownership::GetState().status;
            if(ownership!=local_server_item_ownership::Status::Published&&ownership!=local_server_item_ownership::Status::AlreadyPublished)
                return Fail(4491,"network_abilities_owned_remote_policy_not_published_before_slot");
            if(!original_players::Write(s.slot,0x38,s.actor)||!original_players::Write(s.actor,0x4b0,one)
                ||!Event(s.slot,L"/Script/ShooterGame.ItemSlot.OnRep_Contents",0,nullptr))return Fail(4451,"network_abilities_authority_slot_publish_failed");
            ++p.slotNotifications;
        }
        if(!Identity(s.actor,s.identity))return Fail(4453,"network_abilities_actor_identity_unavailable");
        if(!s.spawned){
            if(!Owner(p,s)||!s.ownerReady)return Fail(4493,"network_abilities_adopted_authority_owner_missing");
            if(!local_server_item_ownership::Prepare(state.world,p.controller,p.playerState,p.pawn,p.inventory,s.actor,s.slot,
                local_server_item_ownership::Source::PublishedSlot))
                return Fail(local_server_item_ownership::GetState().error,local_server_item_ownership::GetState().stage);
            const auto ownership=local_server_item_ownership::GetState().status;
            if(ownership!=local_server_item_ownership::Status::Published&&ownership!=local_server_item_ownership::Status::AlreadyPublished)
                return Fail(4492,"network_abilities_adopted_owned_remote_policy_not_published");
        }
    }
    p.grantComplete=true;return true;
}
inline bool RestoreObserver(void* object,const Weak& identity);
inline bool RetireClientReplicaGraph(Player& p){
    // Native item destruction and replicated slot clearing arrive separately.
    // Retire cached observers using the old weak identities before sampling a
    // replacement. The authority owns all actor/slot destruction and creation.
    for(auto& f:p.fireStates)if(f.object&&!RestoreObserver(f.object,f.identity))return false;
    if(p.skills[3].actor&&!RestoreObserver(p.skills[3].actor,p.skills[3].identity))return false;
    for(auto& s:p.skills){
        const auto type=s.type;
        const auto slotType=s.slotType;
        s={};s.type=type;s.slotType=slotType;
    }
    p.fireStates={};p.commits={};p.commitCursor=0;
    p.priming={};p.daggerObserved={};p.glideObserved={};
    p.grantComplete=p.ready=p.readyAtDiagnostic=p.startingStatesReady=p.primingReady=p.daggerBound=false;
    p.ultimateQuery=p.ultimateReady=false;
    ++state.replicaGraphRetirements;
    p.waiting="live_replicated_skill_actors";return true;
}
inline bool AdoptClient(Player& p){
    if(p.authority)return true;
    for(const auto& s:p.skills)if(s.actor&&!Live(s.actor,s.identity)){
        if(!RetireClientReplicaGraph(p))return false;
        break;
    }
    bool allPresent=true;
    for(size_t i=0;i<SkillCount;++i){auto& s=p.skills[i];
        if(!s.type){s.type=original_jett::LoadEquippableClass(original_jett::SkillPaths[i]);
            if(!s.type)return Fail(4454,"network_abilities_client_original_class_unavailable");
            s.slotType=Read<uint8_t>(engine::DefaultObject(s.type),0x4a0);
            if(s.slotType>=17)return Fail(4455,"network_abilities_client_original_slot_invalid");
        }
        s.slot=Read<void*>(p.inventory,0x1b8+static_cast<size_t>(s.slotType)*8);
        auto actor=Read<void*>(s.slot,0x38);
        if(!actor){allPresent=false;continue;}
        if(!original_players::Derived(Read<void*>(actor,0x10),s.type))return Fail(4456,"network_abilities_replicated_slot_class_mismatch");
        if(s.actor&&s.actor!=actor)return Fail(4457,"network_abilities_replica_replacement_requires_retirement");
        Weak identity{};
        const bool captured=original_inventory_slots::CaptureIdentity(actor,identity);
        if(!captured||!Live(actor,identity)){
            state.lastReplicaIdentityActor=actor;state.lastReplicaIdentity=identity;
            state.lastReplicaIdentitySkill=static_cast<uint32_t>(i);state.lastReplicaCaptureSucceeded=captured;
            if(!captured){
                ++state.replicaCaptureFailures;
                return Fail(4458,"network_abilities_replicated_actor_identity_unavailable");
            }
            // A pending-kill slot reference is non-actionable until native
            // replication clears it. Never publish that pointer to Sample.
            ++state.replicaResolveDeferrals;
            allPresent=false;s.ready=false;continue;
        }
        s.actor=actor;s.identity=identity;s.adopted=true;
    }
    p.grantComplete=allPresent;
    if(!allPresent){p.ready=false;state.stage="network_abilities_waiting_replicated_skill_actors";}
    return true;
}
inline bool Sample(Player& p){
    p.ready=false;
    if(!Source(p))return Fail(4459,"network_abilities_player_source_changed");
    if(!p.authority&&!p.grantComplete)return true;
    DiagnosticScope diagnostic{p};
    bool ready=p.grantComplete;
    for(size_t i=0;i<SkillCount;++i){auto& s=p.skills[i];s.ready=false;s.beginQueryAttempted=false;s.beginQueryValid=false;
        if(!s.actor){ready=false;continue;}
        if(!Live(s.actor,s.identity))return Fail(4460,"network_abilities_skill_actor_retired_without_notification");
        uint8_t begun=0;s.beginQueryAttempted=true;s.beginQueryValid=Event(s.actor,L"/Script/Engine.Actor.HasActorBegunPlay",1,&begun);
        s.begunPlay=s.beginQueryValid&&begun;
        if(!Owner(p,s))return false;
        if(!s.ownerReady){ready=false;continue;}
        s.slotReady=Read<void*>(s.slot,0x38)==s.actor&&Read<void*>(s.slot,0x40)==p.inventory;
        s.machine=Read<void*>(s.actor,0xe00);
        if(!s.begunPlay||!s.slotReady||!IsA(s.machine,0x67722c0)){ready=false;continue;}
        if(!s.componentCount&&!Components(s))return false;
        if(!s.eventsReady){if(!RegisterEvent(p,s,0x6767a30)||(i==0&&!RegisterEvent(p,s,0x6767b90)))return false;s.eventsReady=true;}
        if(!s.resource){s.resourceLookupAttempted=true;s.resource=original_weapon::Component(s.actor,original_jett::ResourceClass);}
        s.resourceReady=IsA(s.resource,original_jett::ResourceClass)&&Read<void*>(s.resource,0xc8)==s.actor;
        if(!s.resourceReady){ready=false;continue;}
        if(p.authority){
            if(!p.sourceEpoch){ready=false;continue;}
            if(!local_server_resource_authority::Prepare(state.world,p.controller,p.playerState,p.pawn,p.inventory,s.actor,p.sourceEpoch)
               ||!local_server_resource_authority::PollItem(s.actor))
                return Fail(local_server_resource_authority::GetState().error,local_server_resource_authority::GetState().stage);
            const auto* authorityResource=local_server_resource_authority::FindCached(s.actor);
            if(!authorityResource||!authorityResource->ready||!authorityResource->resourceMetadata
               ||!authorityResource->resourceRoute.ready||!authorityResource->tuningReady){
                s.resourceReady=false;ready=false;continue;
            }
        }
        auto previous=s.current;void* current=nullptr;
        if(!Event(s.machine,L"/Script/ShooterGame.StateMachineComponent.GetCurrentState",8,&current))return Fail(4461,"network_abilities_native_state_query_failed");
        if(previous&&current!=previous)++s.transitions;
        if(current!=previous){original_jett::ReadStateName(current,s.currentName);s.current=current;}
        const auto old=s.charges;
        s.authResource=Read<int32_t>(s.resource,0x110);s.predictedResource=Read<int32_t>(s.resource,0x10c);
        s.maxCharges=IsA(s.resource,original_jett::EquipmentChargeClass)?Read<int32_t>(s.resource,0x188):-1;
        if(i==3)s.resourceQuery=Remaining(p,s.charges);
        else s.resourceQuery=Event(s.resource,L"/Script/ShooterGame.EquipmentChargeComponent.GetCurrentCharges",4,&s.charges);
        if(old>=0&&s.charges!=old)++s.resourceChanges;
        s.forceHandleActive=false;
        if(IsA(current,0x63d9f60)){
            s.originalForceHandle=Read<uint64_t>(current,0x4a0);
            struct Query {uint64_t handle;uint8_t result=0;} query{s.originalForceHandle};
            s.forceHandleActive=Event(p.forceManager,L"/Script/ShooterGame.ForceModuleManagerComponent.HasActiveForceModule",9,&query)&&query.result;
            if(s.forceHandleActive)++s.forceObservations;
        }
        s.ready=s.begunPlay&&s.slotReady&&s.ownerReady&&s.resourceReady&&s.eventsReady;ready&=s.ready;
    }
    if(!ready)return true;
    if(!StartingStates(p)||!Prime(p)||!DaggerBindings(p))return false;
    p.ultimateQuery=Event(p.pawn,L"/Script/ShooterGame.ShooterCharacter.GetNumPointsNeededForUltimate",4,&p.pointsRequired)
        &&Event(p.playerState,L"/Script/ShooterGame.ShooterPlayerState.GetNumUltimatePoints",4,&p.points);
    uint8_t ultimate=0;p.ultimateReady=Event(p.playerState,L"/Script/ShooterGame.ShooterPlayerState.IsUltimateReady",1,&ultimate)&&ultimate;
    Event(p.forceManager,L"/Script/ShooterGame.ForceModuleManagerComponent.GetActiveForceModuleCount",4,&p.activeForces);
    p.ready=p.startingStatesReady&&p.primingReady&&p.daggerBound;
    ++p.polls;return true;
}
inline bool RefillImpl(void* pawn){
    if(state.error)return false;
    auto p=Find(pawn);if(!Thread()||!p||!p->authority||!state.practice||!p->ready)return Fail(4462,"network_abilities_refill_requires_ready_practice_authority");
    for(size_t i=0;i<SkillCount;++i){auto& s=p->skills[i];
        if(i==3){if(!Event(s.resource,original_jett::DaggerReplenish,0,nullptr))return Fail(4463,"network_abilities_original_dagger_refill_unavailable");}
        else{
            if(!s.resourceQuery||s.maxCharges<=0||s.maxCharges>100)return Fail(4464,"network_abilities_original_charge_limits_unavailable");
            const int32_t amount=(std::max)(0,s.maxCharges-s.charges);
            if(amount){struct Replenish {int32_t amount,result=0;} replenish{amount};
                if(!Event(s.resource,L"/Script/ShooterGame.ResourceComponent.Replenish",8,&replenish))return Fail(4465,"network_abilities_original_replenish_unavailable");}
        }
        if(!local_server_resource_authority::PollItem(s.actor))
            return Fail(local_server_resource_authority::GetState().error,local_server_resource_authority::GetState().stage);
    }
    if(!p->ultimateQuery||p->pointsRequired<=0||p->pointsRequired>100)return Fail(4466,"network_abilities_original_ultimate_requirement_invalid");
    if(p->points<p->pointsRequired){
        if(!original_players::Write(p->playerState,0xbdc,p->pointsRequired)
            ||!Event(p->playerState,L"/Script/ShooterGame.ShooterPlayerState.OnRep_NumUltimatePoints",0,nullptr))return Fail(4467,"network_abilities_authority_ultimate_grant_failed");
    }
    p->refilled=true;++p->refills;return Sample(*p);
}
inline bool VerifyProfiles(){
    struct Profile {uintptr_t rva;size_t bytes;uint64_t digest;};
    constexpr Profile profiles[]{
        {0x6ff5f40,101,0x60919118dc125053ULL},
        {0x70ec480,636,0x94bb9888e3374b1cULL},
        {0x70cef60,1907,0x8faf2527eb67d2b6ULL},
        {0x716a740,37,0xa5a0fed0ddf10f68ULL},
        {0x3b866f0,20,0xff34ed25828c3ebfULL},
        {0x6d25830,68,0xc139875b4b1b94aaULL}
    };
    for(const auto& p:profiles){
        auto bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes))return false;
        uint64_t digest=1469598103934665603ULL;
        for(size_t n=0;n<p.bytes;++n)digest=(digest^bytes[n])*1099511628211ULL;
        if(digest!=p.digest)return false;
    }
    return true;
}
inline bool ProjectileSource(const Player& p,void* actor,void* firingState=nullptr){
    if(!Source(p)||!actor)return false;
    if(!firingState&&!Event(actor,L"/Script/ShooterGame.Projectile.GetFiringPlayerState",8,&firingState))return false;
    if(firingState!=p.playerState)return false;
    auto owner=Read<void*>(actor,0x210);
    for(unsigned depth=0;owner&&depth<8;++depth){
        if(owner==p.pawn)return true;
        for(const auto& skill:p.skills)if(owner==skill.actor&&Live(skill.actor,skill.identity))return true;
        auto next=Read<void*>(owner,0x210);if(next==owner)break;owner=next;
    }
    // A PS survives respawn. Independently require the actual current Pawn
    // instigator and original PlayerInfo when native pool ownership differs.
    void *instigator=nullptr,*projectileInfo=nullptr,*pawnInfo=nullptr;
    return Event(actor,L"/Script/Engine.Actor.GetInstigator",8,&instigator)&&instigator==p.pawn
        &&Event(actor,L"/Script/ShooterGame.Projectile.GetInstigatorPlayerInfo",8,&projectileInfo)
        &&Event(p.pawn,L"/Script/ShooterGame.ShooterCharacter.GetOwnerExclusivePlayerInfo",8,&pawnInfo)
        &&pawnInfo&&projectileInfo==pawnInfo;
}
inline Player* WorldOwner(void* actor,void* firingState=nullptr){
    if(firingState){for(auto& p:players)if(p.used&&ProjectileSource(p,actor,firingState))return &p;return nullptr;}
    for(auto& p:players)if(p.used&&Source(p)){
        auto cursor=actor;
        for(unsigned depth=0;cursor&&depth<8;++depth){
            if(cursor==p.pawn)return &p;
            for(const auto& skill:p.skills)if(cursor==skill.actor&&Live(skill.actor,skill.identity))return &p;
            auto next=Read<void*>(cursor,0x210);if(next==cursor)break;cursor=next;
        }
    }
    return nullptr;
}
inline WorldWitness* TrackWorld(void* actor,Player& p,bool dagger,bool& fresh){
    Weak identity{};fresh=false;if(!Identity(actor,identity))return nullptr;
    WorldWitness* empty=nullptr;
    for(auto& w:worldWitnesses){
        if(w.actor&&!Live(w.actor,w.identity))w={};
        if(w.actor==actor&&original_inventory_slots::SameIdentity(w.identity,identity)){
            if(w.ownerRegistration!=p.registration||!original_inventory_slots::SameIdentity(w.ownerPawnId,p.pawnId)){
                // A real native pooled projectile may be reused by another
                // source. Admit only a new native initialization timestamp;
                // a transient owner change alone cannot reattribute old points.
                const auto initialized=Read<float>(actor,0x518);uint32_t bits=0;std::memcpy(&bits,&initialized,4);
                if((dagger||w.projectile)&&IsA(actor,0x644d990)&&std::isfinite(initialized)&&bits!=w.initializedBits){
                    w={};w.actor=actor;w.identity=identity;w.ownerPawn=p.pawn;w.dagger=dagger;
                    w.ownerPawnId=p.pawnId;w.ownerRegistration=p.registration;w.sourceEpoch=p.sourceEpoch;fresh=true;return &w;
                }
                ++state.worldSourceDeferrals;return nullptr;
            }
            if(!w.sourceEpoch&&p.sourceEpoch)w.sourceEpoch=p.sourceEpoch;
            return &w;
        }
        if(!w.actor&&!empty)empty=&w;
    }
    if(!empty){Fail(4478,"network_abilities_world_witness_capacity");return nullptr;}
    empty->actor=actor;empty->identity=identity;empty->ownerPawn=p.pawn;empty->dagger=dagger;
    empty->ownerPawnId=p.pawnId;empty->ownerRegistration=p.registration;empty->sourceEpoch=p.sourceEpoch;fresh=true;return empty;
}
inline bool CapturePoints(Player& p,void* actor,bool fromFactory=false,uint64_t fireSequence=0,void* fireState=nullptr){
    if(!ProjectileSource(p,actor))return true;
    bool fresh=false;auto witness=TrackWorld(actor,p,true,fresh);if(!witness)return state.error==0;
    witness->projectile=true;
    const auto initialized=Read<float>(actor,0x518);if(!std::isfinite(initialized))return true;
    uint32_t bits=0;std::memcpy(&bits,&initialized,4);
    if(fresh||witness->initializedBits!=bits){witness->initializedBits=bits;witness->nextPoint=0;++state.daggerInitializations;}
    auto component=original_weapon::Component(actor,0x6708820);Weak componentId{};
    if(!component||Read<void*>(component,0xc8)!=actor||!Identity(component,componentId))return true;
    auto entries=Read<uint8_t*>(component,0x370);const auto count=Read<int32_t>(component,0x378),capacity=Read<int32_t>(component,0x37c);
    if(count<0||count>8192||capacity<count||(count&&!original_players::Readable(entries,static_cast<size_t>(count)*0x130)))
        return Fail(4481,"network_abilities_original_dagger_point_array_invalid");
    if(count<witness->nextPoint)witness->nextPoint=0;
    for(int32_t index=witness->nextPoint;index<count;++index){
        const auto native=entries+static_cast<size_t>(index)*0x130;
        auto& point=pointWitnesses[state.nativeDaggerPoints%pointWitnesses.size()];point={};point.sequence=++state.nativeDaggerPoints;
        point.pawn=p.pawn;point.playerState=p.playerState;point.ability=p.skills[3].actor;point.projectile=actor;point.component=component;
        point.projectileId=witness->identity;point.componentId=componentId;point.initializedBits=bits;point.nativeIndex=index;
        point.fireSequence=fireSequence;point.fireState=fireState;point.fromFactory=fromFactory;
        std::memcpy(point.bytes.data(),native,0x130);
        point.victim=engine::function<void*(*)(const void*)>(0x437b290)(native+0x30+0xb8);
        if(point.victim)Identity(point.victim,point.victimId);
        const auto direction=Read<engine::Vec>(native,0x18),start=Read<engine::Vec>(actor,0x570);
        const auto norm=direction.x*direction.x+direction.y*direction.y+direction.z*direction.z;
        const auto damage=Read<float>(native,0x10),falloff=Read<float>(native,0x124);
        point.geometryValid=original_players::Finite(direction)&&original_players::Finite(start)&&norm>.99&&norm<1.01
            &&std::isfinite(damage)&&damage>=0&&std::isfinite(falloff)&&falloff>=0;
        if(!point.geometryValid)++state.invalidDaggerPoints;
        if(fromFactory)++state.factoryPointDeliveries;
        // This is the original native event, not a substitute trace or damage
        // application. The sink validates its source/target life separately.
        if(pointSink&&state.host&&point.geometryValid)pointSink(point);
    }
    witness->nextPoint=count;return true;
}
inline bool CaptureFactoryResult(Player& p,FireState& f,uint64_t sequence){
    if(!fireGraph||!Live(f.object,f.identity))return true;
    auto graphOwner=Read<void*>(fireGraph,0x28);auto classTable=Read<void**>(graphOwner,0);
    auto getter=Read<void*>(classTable,0x3c0),frameField=Read<void*>(graphOwner,0x290);
    const auto frameOffset=Read<int32_t>(frameField,0x44);
    const auto fieldOffset=original_jett_priming::FieldOffset(fireGraph,L"CallFunc_SetNewContext_ReturnValue",8);
    if(getter!=engine::function<void*>(0x40c0c80)||Read<void*>(graphOwner,0x298)!=fireGraph
        ||!frameField||frameOffset<0x30||frameOffset+8>Read<int32_t>(Read<void*>(f.object,0x10),0x60)||fieldOffset<0){++f.frameMisses;return true;}
    auto frame=reinterpret_cast<void*(*)(void*,void*,void*)>(getter)(graphOwner,f.object,fireGraph);
    if(!frame||frame!=Read<void*>(f.object,static_cast<size_t>(frameOffset))
        ||!original_players::Readable(frame,static_cast<size_t>(Read<int32_t>(fireGraph,0x60)))){++f.frameMisses;return true;}
    auto context=Read<void*>(frame,static_cast<size_t>(fieldOffset));Weak contextId{};
    if(!IsA(context,0x6d271c0)||!Identity(context,contextId))return true;
    auto actors=Read<void**>(context,0x38);const auto count=Read<int32_t>(context,0x40),capacity=Read<int32_t>(context,0x44);
    if(count<0||count>1024||capacity<count||(count&&!original_players::Readable(actors,static_cast<size_t>(count)*8)))
        return Fail(4485,"network_abilities_original_factory_actor_list_invalid");
    void* first=nullptr;
    if(!Event(context,L"/Script/ShooterGame.ActorListTransitionContext.GetFirstActor",8,&first)||first!=(count?actors[0]:nullptr))
        return Fail(4486,"network_abilities_original_factory_actor_list_readback_mismatch");
    auto daggerType=engine::Find(original_jett::ProjectilePaths[1]);
    for(int32_t n=0;n<count;++n){auto actor=actors[n];
        if(!actor||!daggerType||!original_players::Derived(Read<void*>(actor,0x10),daggerType))continue;
        ++f.factoryResults;
        auto component=original_weapon::Component(actor,0x6708820);Weak actorId{},componentId{};
        const auto nativeCount=Read<int32_t>(component,0x378);
        if(nativeCount>0&&nativeCount<=8192)f.retainedPoints+=static_cast<uint64_t>(nativeCount);
        // The strong original ActorList may retain a hitscan actor after its
        // immediate native cleanup. Diagnose such a return; never resurrect it.
        if(!Identity(actor,actorId)||!component||!Identity(component,componentId)){++f.nonLiveReturns;continue;}
        void* firingState=nullptr;
        if(!Event(actor,L"/Script/ShooterGame.Projectile.GetFiringPlayerState",8,&firingState)||firingState!=p.playerState)continue;
        if(p.authority&&Read<uint8_t>(actor,0x224)!=3)continue;
        if(!CapturePoints(p,actor,true,sequence,f.object))return false;
    }
    return true;
}
inline Player* EffectOwner(void* actor){
    if(Read<uint8_t>(actor,0x5fc))return nullptr;
    auto manager=Read<void*>(actor,0x618);
    if(IsA(manager,original_effects::ManagerClass)){
        auto owner=Read<void*>(manager,0xc8);if(auto p=WorldOwner(owner))return p;
    }
    // Smoke's OriginalTarget is its real source-owned SmokeZone. A generic
    // target pawn alone does not establish who originated a combat effect.
    auto target=Read<void*>(actor,0x620),smokeType=engine::Find(original_jett_world::SmokeZonePath);
    return target&&smokeType&&original_players::Derived(Read<void*>(target,0x10),smokeType)?WorldOwner(target):nullptr;
}
inline bool WorldGroup(unsigned kind){
    const auto path=kind==0?original_jett_world::SmokeZonePath:kind==1?original_jett::ProjectilePaths[1]:kind==4?original_jett::ProjectilePaths[0]:original_jett_world::SmokeLoopPath;
    auto type=kind==3?engine::Class(0x648ada0):engine::Find(path);if(!type)return true;
    struct Query {void* world;void* type;void** actors=nullptr;int32_t count=0,capacity=0;} q{state.world,type};
    if(!Event(engine::DefaultObject(engine::Class(0x43a9670)),L"/Script/Engine.GameplayStatics.GetAllActorsOfClass",32,&q))return Fail(4479,"network_abilities_original_world_query_failed");
    struct Allocation {void* p;~Allocation(){if(p)engine::function<void(*)(void*)>(0x1915720)(p);}} allocation{q.actors};
    if(q.count<0||q.count>65536||q.capacity<q.count||(q.count&&!original_players::Readable(q.actors,static_cast<size_t>(q.count)*8)))return Fail(4480,"network_abilities_original_world_actor_array_invalid");
    for(int32_t n=0;n<q.count;++n){auto actor=q.actors[n];void* firingState=nullptr;Player* p=nullptr;
        if(kind==1||kind==4){
            if(!Event(actor,L"/Script/ShooterGame.Projectile.GetFiringPlayerState",8,&firingState))continue;
            p=WorldOwner(actor,firingState);
        }else if(kind==2){auto target=Read<void*>(actor,0x620);
            if(!target||Read<uint8_t>(actor,0x5fc)||!original_players::Derived(Read<void*>(target,0x10),engine::Find(original_jett_world::SmokeZonePath)))continue;
            p=WorldOwner(target);
        }else if(kind==3)p=EffectOwner(actor);
        else p=WorldOwner(actor);
        if(!p)continue;
        if(kind==0)++state.smokeZones;
        if(kind==2||kind==3){
            if(kind==2)++state.smokeLoops;else ++state.worldEffects;
            bool fresh=false;auto witness=TrackWorld(actor,*p,false,fresh);
            if(!witness){if(state.error)return false;continue;}witness->effect=true;continue;
        }
        if(kind==1){if(!CapturePoints(*p,actor))return false;continue;}
        bool fresh=false;auto witness=TrackWorld(actor,*p,false,fresh);if(!witness){if(state.error)return false;continue;}
        if(kind==4)witness->projectile=true;
    }
    return true;
}
inline bool PollWorld(){
    const auto now=GetTickCount64();if(state.worldSamples&&now-state.worldSampleAt<100)return true;
    state.smokeZones=state.smokeLoops=state.worldEffects=0;
    if(!WorldGroup(0)||!WorldGroup(1)||!WorldGroup(2)||!WorldGroup(3)||!WorldGroup(4))return false;
    ++state.worldSamples;state.worldSampleAt=now;return true;
}
inline bool RestoreObserver(void* object,const Weak& identity){
    if(!Live(object,identity))return true;
    auto actual=Read<void**>(object,0);
    for(const auto& t:tables)if(t.copy==actual){
        if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(object),t.parent,t.copy)!=t.copy)return Fail(4482,"network_abilities_observer_retirement_conflict");
        ++state.restoredObservers;return true;
    }
    return true;
}
inline bool RetirePlayerImpl(void* pawn){
    if(!Thread())return false;Player* p=nullptr;
    for(auto& candidate:players)if(candidate.used&&candidate.pawn==pawn){p=&candidate;break;}
    if(!p)return true;
    for(auto& f:p->fireStates)if(f.object&&!RestoreObserver(f.object,f.identity))return false;
    if(p->skills[3].actor&&!RestoreObserver(p->skills[3].actor,p->skills[3].identity))return false;
    if(p->authority&&!RestoreObserver(p->playerState,p->stateId))return false;
    for(auto& w:worldWitnesses)if(w.ownerPawn==pawn)w={};
    if(p->authority)local_server_resource_authority::RetirePlayer(pawn);
    // Shared read-only table backing stays alive for late original callbacks.
    // Actor destruction/slot teardown belong to the host's native respawn path.
    *p={};if(state.players)--state.players;++state.retiredPlayers;return true;
}
inline bool InitImpl(void* world,bool host,bool practice){
    if(state.error||state.restartRequired)return false;
    if(state.initialized)return (state.world==world&&state.host==host&&state.practice==practice)||Fail(4468,"network_abilities_context_change_requires_new_session");
    if(!world||!engine::image||!original_players::Thread())return Fail(4469,"network_abilities_original_game_thread_context_missing");
    state.initialized=true;state.host=host;state.practice=practice;state.world=world;state.gameThread=GetCurrentThreadId();
    consumeUltimate=engine::Find(L"/Script/ShooterGame.ShooterPlayerState.AuthConsumeUltimatePoints");
    setUltActive=engine::Find(L"/Script/ShooterGame.ShooterPlayerState.AuthSetUltActive");
    setUltimateActive=engine::Find(L"/Script/ShooterGame.ShooterPlayerState.AuthSetUltimateActive");
    if(!consumeUltimate||!setUltActive||!setUltimateActive)return Fail(4470,"network_abilities_original_ultimate_callbacks_unavailable");
    // Exact finite byte profiles supplement the launcher's pinned image check;
    // a different native body is rejected instead of adapting by guesswork.
    state.profilesVerified=VerifyProfiles();
    if(!state.profilesVerified)return Fail(4471,"network_abilities_pinned_native_entry_profile_mismatch");
    original_jett_priming::PropertyInfo replicates{};
    const auto replicatesOffset=original_jett_priming::FieldOffset(engine::Class(0x3683900),L"bReplicates",1,&replicates);
    state.worldReplicationSchemaVerified=replicatesOffset>=0
        &&static_cast<size_t>(replicatesOffset)+replicates.boolByteOffset==0x11a
        &&replicates.boolFieldSize>0&&replicates.boolByteMask==0x10&&replicates.boolFieldMask==0x10;
    state.stage=practice?"network_abilities_waiting_actual_player":"network_abilities_ffa_disabled";return true;
}
inline bool BindPlayerEpochImpl(void* pawn,uint32_t epoch){
    if(!Thread()||state.error)return false;auto p=Find(pawn);
    if(!p||!epoch)return Fail(4487,"network_abilities_explicit_source_epoch_missing");
    if(p->sourceEpoch&&p->sourceEpoch!=epoch)return Fail(4488,"network_abilities_source_epoch_changed_without_retirement");
    p->sourceEpoch=epoch;return true;
}
inline bool RegisterPlayerImpl(void* controller,void* playerState,void* pawn,uint32_t sourceEpoch){
    if(!Thread()||state.error)return false;
    if(!state.practice){state.stage="network_abilities_ffa_disabled";return true;}
    if(auto p=Find(pawn)){
        if(p->controller!=controller||p->playerState!=playerState)return Fail(4472,"network_abilities_registered_player_links_changed");
        return !sourceEpoch||BindPlayerEpochImpl(pawn,sourceEpoch);
    }
    if(!IsA(pawn,original_players::ShooterCharacterClass)||!IsA(playerState,original_players::ShooterStateClass)
        ||Read<void*>(pawn,0x488)!=playerState||Read<void*>(controller,0x4b0)!=pawn||Read<void*>(controller,0x470)!=playerState)
        return Fail(4473,"network_abilities_actual_player_links_invalid");
    auto jett=engine::Find(L"/Game/Characters/Wushu/Wushu_PC.Wushu_PC_C");
    if(!jett||!original_players::Derived(Read<void*>(pawn,0x10),jett))return Fail(4474,"network_abilities_requires_actual_wushu_character");
    const auto role=Read<uint8_t>(pawn,0x224);
    if(role!=(state.host?3:2))return Fail(4475,"network_abilities_actual_network_authority_mismatch");
    Player* p=nullptr;for(auto& slot:players)if(!slot.used){p=&slot;break;}
    if(!p)return Fail(4476,"network_abilities_player_capacity");
    p->controller=controller;p->playerState=playerState;p->pawn=pawn;p->role=role;p->authority=state.host;
    p->inventory=Read<void*>(pawn,0xc08);p->forceManager=Read<void*>(pawn,0x1100);
    if(!IsA(p->inventory,0x6499330)||Read<void*>(p->inventory,0xc8)!=pawn||Read<void*>(p->inventory,0x2f0)!=pawn
        ||!IsA(p->forceManager,0x685f510)||!Identity(pawn,p->pawnId)||!Identity(playerState,p->stateId)||!Identity(controller,p->controllerId))
        return Fail(4477,"network_abilities_actual_inventory_or_identity_missing");
    if(state.playerRegistrations==UINT64_MAX)return Fail(4489,"network_abilities_source_registration_exhausted");
    p->sourceEpoch=sourceEpoch;p->registration=++state.playerRegistrations;p->used=true;++state.players;
    if(!Grant(*p)||!AdoptClient(*p)||!Sample(*p))return false;
    if(p->authority&&p->ready&&!p->refilled&&!RefillImpl(pawn))return false;
    state.stage=p->ready?"network_abilities_original_graph_ready_execution_unverified":"network_abilities_waiting_original_replica_graph";return true;
}
inline bool PollImpl(){
    if(!Thread()||state.error)return false;
    if(!state.practice){state.stage="network_abilities_ffa_disabled";return true;}
    state.readyPlayers=0;
    for(auto& p:players)if(p.used){
        if(!AdoptClient(p)||!Sample(p))return false;
        if(p.authority&&p.ready&&!p.refilled&&!RefillImpl(p.pawn))return false;
        if(p.ready)++state.readyPlayers;
    }
    if(!PollWorld())return false;
    ++state.polls;
    state.stage=state.players&&state.players==state.readyPlayers?"network_abilities_original_graph_ready_execution_unverified":"network_abilities_waiting_original_replica_graph";
    return state.error==0;
}
inline bool Init(void* world,bool host,bool practice){__try{return InitImpl(world,host,practice);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool RegisterPlayer(void* pc,void* ps,void* pawn,uint32_t sourceEpoch=0){__try{return RegisterPlayerImpl(pc,ps,pawn,sourceEpoch);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool BindPlayerEpoch(void* pawn,uint32_t sourceEpoch){__try{return BindPlayerEpochImpl(pawn,sourceEpoch);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool Poll(){__try{return PollImpl();}__except(Capture(GetExceptionInformation())){return false;}}
inline bool Refill(void* pawn){__try{return RefillImpl(pawn);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool RetirePlayer(void* pawn){__try{return RetirePlayerImpl(pawn);}__except(Capture(GetExceptionInformation())){return false;}}
// Visit(rootActor, referencedObject) lets the bridge use its existing native
// ActorChannel/ReplicateSubobject path and original PackageMap NetGUIDs. This is
// an actual object graph, never a second state serialization or pointer codec.
template<class Visit> inline bool VisitReplicationGraph(Visit&& visit){
    if(!Thread()||!state.host||!state.practice)return true;
    for(const auto& p:players)if(p.used&&Source(p))for(const auto& s:p.skills)if(s.actor&&Live(s.actor,s.identity)){
        visit(s.actor,s.actor);visit(p.pawn,s.slot);
        for(size_t n=0;n<s.componentCount;++n)visit(s.actor,s.components[n]);
    }
    return true;
}
// Deliberately separate from the equipment graph: original predicted actors
// may use native RPC/context keys instead of ordinary property replication.
// visitor(actor,info) receives only original bReplicates actors. Projectiles need
// explicit opt-in while the native prediction-key merge remains unproven.
template<class Visit> inline bool VisitWorldActors(Visit&& visit,bool includeProjectiles=false){
    if(!Thread()||!state.host||!state.practice)return true;
    if(!state.worldReplicationSchemaVerified)return Fail(4490,"network_abilities_original_world_replication_schema_mismatch");
    for(const auto& w:worldWitnesses){
        if(!w.actor||!Live(w.actor,w.identity)||Read<uint8_t>(w.actor,0x224)!=3)continue;
        auto p=Find(w.ownerPawn);
        if(!p||!Source(*p)||!p->authority||!p->sourceEpoch||w.sourceEpoch!=p->sourceEpoch
            ||w.ownerRegistration!=p->registration||!original_inventory_slots::SameIdentity(w.ownerPawnId,p->pawnId))continue;
        bool sourceMatches=false;
        if(w.projectile){void* firingState=nullptr;
            sourceMatches=Event(w.actor,L"/Script/ShooterGame.Projectile.GetFiringPlayerState",8,&firingState)&&ProjectileSource(*p,w.actor,firingState);
        }else if(w.effect)sourceMatches=EffectOwner(w.actor)==p;
        else sourceMatches=WorldOwner(w.actor)==p;
        if(!sourceMatches)continue;
        const bool replicates=(Read<uint8_t>(w.actor,0x11a)&0x10U)!=0;
        if(!replicates||(w.projectile&&!includeProjectiles)){++state.worldReplicationFiltered;continue;}
        uint8_t destroying=1;
        if(!Event(w.actor,L"/Script/Engine.Actor.IsActorBeingDestroyed",1,&destroying)||destroying)continue;
        WorldActorInfo info{};info.sourcePawn=p->pawn;info.sourcePlayerState=p->playerState;
        if(w.projectile)info.sourceAbility=p->skills[w.dagger?3:0].actor;
        else if(!w.effect)info.sourceAbility=p->skills[0].actor;
        else{auto managerOwner=Read<void*>(Read<void*>(w.actor,0x618),0xc8);
            if(IsA(managerOwner,0x645c660)&&Read<void*>(managerOwner,0x4a8)==p->pawn)info.sourceAbility=managerOwner;
        }
        info.sourcePawnId=p->pawnId;
        info.sourceRegistration=p->registration;info.sourceEpoch=p->sourceEpoch;
        info.dagger=w.dagger;info.effect=w.effect;info.projectile=w.projectile;info.originalReplicates=replicates;
        ++state.worldReplicationCandidates;visit(w.actor,info);
    }
    return true;
}
inline void WriteNamedObservation(FILE* f,const char* key,const NamedObservation& named){
    std::fprintf(f,",\"%s\":{\"object\":\"%p\",\"name\":",key,named.object);original_players::Text(f,named.name);std::fputc('}',f);
}
inline void WriteMachineObservation(FILE* f,const MachineObservation& m){
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(f,"{\"typed\":%s,\"owner\":\"%p\",\"owner_role\":%u,\"owner_matches\":%s,\"enabled\":%s,\"starting_owner_matches\":%s,\"starting_machine_matches\":%s,\"role_selected_nonempty\":%s,\"subframe\":",b(m.typed),m.owner,unsigned(m.ownerRole),b(m.ownerMatches),b(m.enabled),b(m.startingOwnerMatches),b(m.startingMachineMatches),b(m.selectedNonempty));
    if(std::isfinite(m.subframe))std::fprintf(f,"%.9g",double(m.subframe));else std::fputs("null",f);
    WriteNamedObservation(f,"machine",m.machine);WriteNamedObservation(f,"starting",m.starting);
    WriteNamedObservation(f,"authority",m.authority);WriteNamedObservation(f,"predicted",m.predicted);WriteNamedObservation(f,"role_selected",m.selected);
    std::fprintf(f,",\"force_state\":%s,\"force_handle\":%llu,\"force_module_class\":\"%p\",\"force_manager\":\"%p\",\"force_source\":\"%p\",\"force_query_attempted\":%s,\"force_query_valid\":%s,\"force_active\":",b(m.forceState),static_cast<unsigned long long>(m.forceHandle),m.forceModuleClass,m.forceManager,m.forceSource,b(m.forceQueryAttempted),b(m.forceQueryValid));
    std::fputs(m.forceQueryValid?b(m.forceActive):"null",f);std::fputc('}',f);
}
inline void WriteFieldObservation(FILE* f,const FieldObservation& field,bool boolean){
    std::fprintf(f,"{\"observed\":%s,\"offset\":%d,\"value\":",field.valid?"true":"false",field.offset);
    if(!field.valid)std::fputs("null",f);else if(boolean)std::fputs(field.value?"true":"false",f);else std::fprintf(f,"%d",field.integer);
    if(boolean)std::fprintf(f,",\"field_size\":%u,\"byte_offset\":%u,\"byte_mask\":%u,\"field_mask\":%u",unsigned(field.fieldSize),unsigned(field.byteOffset),unsigned(field.byteMask),unsigned(field.fieldMask));
    std::fputc('}',f);
}
inline void WriteDaggerObservation(FILE* f,const DaggerObservation& d){
    auto remaining=d.remaining,maximum=d.maximum,consume=d.shouldConsume,reset=d.shouldReset;
    if(!d.sourceValid)remaining.valid=maximum.valid=consume.valid=reset.valid=false;
    std::fprintf(f,"{\"resource\":\"%p\",\"source_valid\":%s,\"remaining\":",d.resource,d.sourceValid?"true":"false");WriteFieldObservation(f,remaining,false);
    std::fputs(",\"maximum\":",f);WriteFieldObservation(f,maximum,false);
    std::fputs(",\"should_consume_ult\":",f);WriteFieldObservation(f,consume,true);
    std::fputs(",\"should_daggers_reset\":",f);WriteFieldObservation(f,reset,true);
    std::fprintf(f,",\"multicast_observed\":%s,\"multicast_flags\":%u,\"multicast_parameter_bytes\":%u,\"multicast_script_bytes\":%d,\"multicast_execution_observed\":false",d.multicastObserved?"true":"false",d.multicastFlags,unsigned(d.multicastBytes),d.multicastScriptBytes);
    WriteNamedObservation(f,"primary_projectile_class",d.primaryClass);WriteNamedObservation(f,"burst_projectile_class",d.burstClass);std::fputc('}',f);
}
inline void WriteGlideObservation(FILE* f,const GlideObservation& g){
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(f,"{\"slot\":\"%p\",\"slot_schema_observed\":%s,\"multi_array_valid\":%s,\"multi_count\":%d,\"matching_original_actors\":%d,\"source_valid\":%s,\"role\":%u",g.slot,b(g.slotSchema),b(g.arrayValid),g.count,g.matches,b(g.sourceValid),unsigned(g.role));
    auto active=g.active;if(!g.sourceValid)active.valid=false;
    WriteNamedObservation(f,"actor",g.actor);std::fputs(",\"visual_active\":",f);WriteFieldObservation(f,active,true);
    std::fprintf(f,",\"machine_cache_valid\":%s,\"machine_cache_count\":%d,\"machines\":[",b(g.graph.machineCacheValid),g.graph.machineCacheCount);
    if(g.sourceValid)for(size_t n=0;n<g.graph.machinesCaptured;++n){if(n)std::fputc(',',f);WriteMachineObservation(f,g.graph.machines[n]);}
    std::fputs("],\"execution_verified\":false}",f);
}
inline void WriteSkillObservation(FILE* f,const SkillObservation& d){
    const auto b=[](bool value){return value?"true":"false";};
    std::fputs("{\"waiting_reason\":",f);original_players::Text(f,d.waiting);
    std::fprintf(f,",\"prerequisite_mask\":%u,\"required_mask\":%u,\"missing_mask\":%u,\"missing\":[",d.prerequisiteMask,RequiredPrerequisites,d.missingMask);
    bool comma=false;for(unsigned n=0;n<16;++n)if(d.missingMask&(1U<<n)){if(comma)std::fputc(',',f);comma=true;original_players::Text(f,PrerequisiteNames[n]);}
    std::fprintf(f,"],\"cached_inventory\":\"%p\",\"native_inventory\":\"%p\",\"cached_slot\":\"%p\",\"native_slot\":\"%p\",\"cached_contents\":\"%p\",\"native_contents\":\"%p\",\"observed_actor\":\"%p\"",d.cachedInventory,d.nativeInventory,d.cachedSlot,d.nativeSlot,d.cachedContents,d.nativeContents,d.observedActor);
    std::fprintf(f,",\"slot_outer\":\"%p\",\"slot_inventory\":\"%p\",\"native_slot_type\":%u,\"slot_type_valid\":%s,\"slot_outer_matches\":%s,\"slot_inventory_matches\":%s,\"contents_match\":%s",d.slotOuter,d.slotInventory,unsigned(d.nativeSlotType),b(d.slotTypeValid),b(d.slotOuterMatches),b(d.slotInventoryMatches),b(d.contentsMatch));
    std::fprintf(f,",\"native_owner\":\"%p\",\"typed_owner\":\"%p\",\"resource\":\"%p\",\"resource_owner\":\"%p\",\"actual_role\":%u,\"object_flags\":%u,\"original_replicates\":%s",d.actorOwner,d.typedOwner,d.resource,d.resourceOwner,unsigned(d.actualRole),d.actorFlags,b(d.replicates));
    std::fprintf(f,",\"cached_actor_matches_native\":%s,\"actor_identity_observed\":%s,\"begin_query_attempted\":%s,\"begin_query_valid\":%s,\"begin_play_observed\":%s,\"resource_lookup_attempted\":%s",b(d.cachedActorMatchesNative),b(d.actorIdentityObserved),b(d.beginQueryAttempted),b(d.beginQueryValid),b(d.beginObserved),b(d.resourceLookupAttempted));
    WriteNamedObservation(f,"expected_class",d.expectedType);WriteNamedObservation(f,"actual_class",d.actualType);
    std::fprintf(f,",\"native_connection\":\"%p\",\"native_actor_channel\":\"%p\",\"channel_snapshot_valid\":%s,\"channel_found\":%s,\"channel_closing\":%s,\"channel_open_ack\":%s,\"channel_flags\":%u,\"channel_open_flags\":%u,\"netguid_value_observed\":false",d.connection,d.channel,b(d.channelSnapshotValid),b(d.channelFound),b(d.channelClosing),b(d.channelOpenAck),unsigned(d.channelFlags),unsigned(d.channelOpenFlags));
    std::fprintf(f,",\"machine_cache_valid\":%s,\"machine_cache_count\":%d,\"machines_captured\":%zu,\"machines\":[",b(d.machineCacheValid),d.machineCacheCount,d.machinesCaptured);
    for(size_t n=0;n<d.machinesCaptured;++n){if(n)std::fputc(',',f);WriteMachineObservation(f,d.machines[n]);}
    std::fprintf(f,"],\"tuning\":\"%p\",\"tuning_map_count\":%d,\"tuning_native_net_array_count\":%d,\"resource_component_route\":",d.tuning,d.tuningMapCount,d.tuningArrayCount);
    local_server_resource_authority::WriteRoute(f,d.resourceRoute);
    std::fputs(",\"tuning_component_route\":",f);local_server_resource_authority::WriteRoute(f,d.tuningRoute);std::fputc('}',f);
}
inline void WriteState(FILE* f){
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(f,"{\"stage\":\"%s\",\"error\":%u,\"host\":%s,\"practice\":%s,\"restart_required\":%s,\"profiles_verified\":%s,\"players\":%u,\"ready_players\":%u,\"polls\":%llu,\"event_forwards\":%llu,\"tables_installed\":%llu,\"smoke_zones\":%llu,\"smoke_loops\":%llu,\"dagger_initializations\":%llu,\"native_dagger_points\":%llu,\"network_execution_verified\":false,\"damage_application_verified\":false,\"native_prediction_preserved\":true,\"native_force_entry_preserved\":true,\"player_states\":[",state.stage,state.error,b(state.host),b(state.practice),b(state.restartRequired),b(state.profilesVerified),state.players,state.readyPlayers,static_cast<unsigned long long>(state.polls),static_cast<unsigned long long>(state.eventForwards),static_cast<unsigned long long>(state.tablesInstalled),static_cast<unsigned long long>(state.smokeZones),static_cast<unsigned long long>(state.smokeLoops),static_cast<unsigned long long>(state.daggerInitializations),static_cast<unsigned long long>(state.nativeDaggerPoints));
    bool comma=false;for(const auto& p:players)if(p.used){
        if(comma)std::fputc(',',f);comma=true;
        std::fprintf(f,"{\"pawn\":\"%p\",\"authority\":%s,\"ready\":%s,\"priming_ready\":%s,\"prime_before\":%.9g,\"prime_source\":%.9g,\"prime_after\":%.9g,\"dagger_bound\":%s,\"points\":%d,\"active_forces\":%d,\"primary_commits\":%llu,\"burst_commits\":%llu,\"consumes\":%llu,\"skills\":[",p.pawn,b(p.authority),b(p.ready),b(p.primingReady),double(p.priming.before),double(p.priming.source),double(p.priming.after),b(p.daggerBound),p.points,p.activeForces,static_cast<unsigned long long>(p.primaryCommits),static_cast<unsigned long long>(p.burstCommits),static_cast<unsigned long long>(p.consumes));
        for(size_t i=0;i<SkillCount;++i){if(i)std::fputc(',',f);const auto& s=p.skills[i];
            std::fprintf(f,"{\"name\":\"%s\",\"actor\":\"%p\",\"slot\":%u,\"role\":%u,\"ready\":%s,\"spawned\":%s,\"charges\":%d,\"auth_resource\":%d,\"predicted_resource\":%d,\"current_state\":\"%p\",\"transitions\":%llu,\"force_observations\":%llu,\"components\":%zu,\"replica_prerequisites\":",original_jett::SkillNames[i],s.actor,unsigned(s.slotType),unsigned(s.role),b(s.ready),b(s.spawned),s.charges,s.authResource,s.predictedResource,s.current,static_cast<unsigned long long>(s.transitions),static_cast<unsigned long long>(s.forceObservations),s.componentCount);
            WriteSkillObservation(f,s.observed);std::fputs(",\"authority_publication\":",f);
            local_server_resource_authority::WriteItem(f,s.actor);std::fputc('}',f);
        }
        std::fprintf(f,"],\"source_epoch\":%u,\"registration\":%llu,\"grant_complete\":%s,\"starting_states_ready\":%s,\"refilled\":%s,\"source_mask\":%u,\"source_required_mask\":255,\"cached_inventory\":\"%p\",\"native_pawn_inventory\":\"%p\",\"cached_force_manager\":\"%p\",\"native_force_manager\":\"%p\",\"diagnostic_samples\":%llu,\"diagnostic_at_ms\":%llu,\"ready_at_diagnostic\":%s,\"waiting_reason\":",p.sourceEpoch,static_cast<unsigned long long>(p.registration),b(p.grantComplete),b(p.startingStatesReady),b(p.refilled),p.sourceMask,p.inventory,p.observedNativeInventory,p.forceManager,p.observedNativeForceManager,static_cast<unsigned long long>(p.diagnosticSamples),static_cast<unsigned long long>(p.diagnosticAt),b(p.readyAtDiagnostic));
        original_players::Text(f,p.waiting);std::fputs(",\"fire_states\":[",f);
        for(size_t n=0;n<p.fireStates.size();++n){if(n)std::fputc(',',f);const auto& fire=p.fireStates[n];
            std::fprintf(f,"{\"entries\":%llu,\"factory_results\":%llu,\"retained_points\":%llu,\"nonlive_returns\":%llu,\"frame_misses\":%llu,\"class_supplies\":%llu}",static_cast<unsigned long long>(fire.entries),static_cast<unsigned long long>(fire.factoryResults),static_cast<unsigned long long>(fire.retainedPoints),static_cast<unsigned long long>(fire.nonLiveReturns),static_cast<unsigned long long>(fire.frameMisses),static_cast<unsigned long long>(fire.classSupplies));
        }
        std::fputs("],\"dagger_native_state\":",f);WriteDaggerObservation(f,p.daggerObserved);
        std::fputs(",\"passive_glide\":",f);WriteGlideObservation(f,p.glideObserved);std::fputc('}',f);
    }
    std::fprintf(f,"],\"fire_sequences\":%llu,\"factory_point_deliveries\":%llu,\"world_effects\":%llu,\"world_replication_schema_verified\":%s,\"world_replication_candidates\":%llu,\"world_replication_filtered\":%llu,\"world_source_deferrals\":%llu,\"resource_authority\":",static_cast<unsigned long long>(state.fireSequences),static_cast<unsigned long long>(state.factoryPointDeliveries),static_cast<unsigned long long>(state.worldEffects),b(state.worldReplicationSchemaVerified),static_cast<unsigned long long>(state.worldReplicationCandidates),static_cast<unsigned long long>(state.worldReplicationFiltered),static_cast<unsigned long long>(state.worldSourceDeferrals));
    local_server_resource_authority::WriteSummary(f);
    std::fprintf(f,",\"replica_identity\":{\"graph_retirements\":%llu,\"capture_failures\":%llu,\"resolve_deferrals\":%llu,\"failure_observed\":%s,\"last_actor\":\"%p\",\"last_skill\":%u,\"last_weak\":[%d,%d],\"last_capture_succeeded\":%s,\"last_resolve_attempted\":%s,\"last_resolve_succeeded\":false}",
        static_cast<unsigned long long>(state.replicaGraphRetirements),static_cast<unsigned long long>(state.replicaCaptureFailures),
        static_cast<unsigned long long>(state.replicaResolveDeferrals),b(state.lastReplicaIdentityActor!=nullptr),state.lastReplicaIdentityActor,state.lastReplicaIdentitySkill,
        state.lastReplicaIdentity.index,state.lastReplicaIdentity.serial,b(state.lastReplicaCaptureSucceeded),b(state.lastReplicaCaptureSucceeded));
    std::fprintf(f,",\"diagnostic_force_profile_checked\":%s,\"diagnostic_force_profile_verified\":%s,\"diagnostic_machine_interval_ms\":100}",b(state.diagnosticForceChecked),b(state.diagnosticForceProfileVerified));
}
}
