#pragma once
#include "original_ability_tuning.h"
#include "original_players.h"
#include "original_weapon.h"
#include <array>

// Local authority assembly for the actual Wushu ability equippables. The
// installed Blueprint owns its state machines, animations, meshes and FXC.
// This adapter never plays a substitute pose or generates gun firing results.
// Static provenance: work/duel1-original-jett-native and its report. A successful
// grant is not evidence that a skill has executed; counters below separate them.
namespace original_jett {
inline constexpr size_t SkillCount=4;
inline constexpr uint32_t InputBits[SkillCount]{64,128,256,512};
inline constexpr const wchar_t* SkillPaths[SkillCount]{
    L"/Game/Characters/Wushu/S0/Ability_4/Ability_Wushu_4_Smoke.Ability_Wushu_4_Smoke_C",
    L"/Game/Characters/Wushu/S0/Ability_Q/Ability_Wushu_Q_CycloneBoost.Ability_Wushu_Q_CycloneBoost_C",
    L"/Game/Characters/Wushu/S0/Ability_E/Ability_Wushu_E_Dash.Ability_Wushu_E_Dash_C",
    L"/Game/Characters/Wushu/S0/Ability_X/Ability_Wushu_X_Dagger_Production.Ability_Wushu_X_Dagger_Production_C"
};
inline constexpr const char* SkillNames[SkillCount]{"Cloudburst","Updraft","Tailwind","BladeStorm"};
inline constexpr uintptr_t ResourceClass=0x637ed10,EquipmentChargeClass=0x675ea00;
inline constexpr uintptr_t SignatureResourceClass=0x6cde0f0;
inline constexpr const wchar_t* DaggerResourcePath=L"/Game/Characters/Wushu/S0/Ability_X/Comp_Wushu_X_DaggerResource.Comp_Wushu_X_DaggerResource_C";
inline constexpr const wchar_t* DaggerReplenish=L"/Game/Characters/Wushu/S0/Ability_X/Comp_Wushu_X_DaggerResource.Comp_Wushu_X_DaggerResource_C.ReplenishAllDaggers";
inline constexpr const wchar_t* DaggerRemaining=L"/Game/Characters/Wushu/S0/Ability_X/Comp_Wushu_X_DaggerResource.Comp_Wushu_X_DaggerResource_C.GetDaggersRemaining";
inline constexpr const wchar_t* ProjectilePaths[2]{
    L"/Game/Characters/Wushu/S0/Ability_4/Projectile_Wushu_4_Smoke.Projectile_Wushu_4_Smoke_C",
    L"/Game/Characters/Wushu/S0/Ability_X/Projectile_Wushu_X_DaggerParent.Projectile_Wushu_X_DaggerParent_C"
};
struct Skill {
    void *type=nullptr,*defaults=nullptr,*actor=nullptr,*slot=nullptr,*resource=nullptr,*stateMachine=nullptr,*currentState=nullptr;
    void *previousState=nullptr;
    bool classLoaded=false,defaultSlotValid=false,attempted=false,created=false,alreadyPresent=false;
    bool slotCreated=false,slotPublished=false,ownerBound=false,resourcePresent=false,stateMachinePresent=false;
    bool inStartingClasses=false,grantReady=false,resourceQueryValid=false,activationQueryValid=false;
    bool currentEquippable=false,desiredEquippable=false,hasBegunPlay=false,chargeRefillRequested=false;
    uint8_t slotType=254,activationState=0,tickState=0,tickContents=0;
    int32_t charges=-1,maximumCharges=-1,resourcePredicted=-1,resourceAuthority=-1;
    uint32_t inputEdges=0,stateTransitions=0,resourceChanges=0,refillRequests=0;
    int32_t previousCharges=-1;
    bool forceBridgeAttempted=false,forceHandleActive=false,originalForceAlreadyActive=false;
    uint32_t forceBridgeAttempts=0;
    uint64_t forceHandle=0;
    float originalForceDuration=0;
    void *forceBridgeState=nullptr,*forceModuleClass=nullptr;
    wchar_t currentStateName[128]{};
    bool stateNameQueryValid=false,originalForceState=false,forceQueryValid=false,daggersActive=false,daggersActiveQueryValid=false;
    uint32_t forceStateEntries=0,forceActiveObservations=0,forceBridgeSuccessful=0;
};
struct ProjectileObservation {
    void *type=nullptr,*lastActor=nullptr,*lastPlayerState=nullptr;
    bool classLoaded=false,queryValid=false,creationQueryValid=false,lastCleanedUp=false;
    int32_t worldCount=0,localCount=0;
    uint64_t scans=0,localInitializations=0,trackingOverflow=0;
    float lastInitializedAt=0;
    uint32_t alignmentPadding[3]{};
    engine::Transform lastCreation{};
};
struct ProjectileIdentity {
    int32_t index=-1,serial=0;
    uint32_t initializedBits=0;
    bool used=false;
};
struct Diagnostics {
    bool initialized=false,loadoutGranted=false,restartRequired=false,startingClassesReadable=false;
    bool originalWushu=false,originalInventory=false,ultimateReady=false,ultimateQueryValid=false;
    bool localAuthority=false;
    int32_t originalStartingClassCount=0,originalStartingEquippableCount=0,originalAbilityUIOrderCount=0;
    int32_t ultimatePoints=-1,ultimatePointsRequired=-1,activeForceModuleCount=0;
    uint32_t error=0,refillCalls=0,activationSubmissions=0,samples=0;
    uintptr_t exceptionRva=0;
    const char* stage="original_jett_not_initialized";
    const char* exceptionOperation="none";
    const wchar_t* failedEvent=nullptr;
    uint16_t expectedEventSize=0,actualEventSize=0;
    void *world=nullptr,*pawn=nullptr,*inventory=nullptr,*playerState=nullptr,*forceManager=nullptr;
};
inline Diagnostics state;
inline std::array<Skill,SkillCount> skills{};
// Separate read-only records: never consume/overwrite the rifle observer's
// namespace-global MachineDiagnostics or equipment/input ownership.
inline std::array<original_weapon::MachineDiagnostics,SkillCount> responseGraphs{};
struct InputResponseObservation {
    uint8_t input=255,action=255,activation=255;
    bool canOutOfGameplay=false,canSuppressed=false,canInSpawnZones=false;
    float delay=0;
    int32_t requiredTags=0,ignoredTags=0;
    void* target=nullptr;
    char targetName[128]{};
};
struct InputGraphObservation {
    bool valid=false;
    int32_t count=0;
    unsigned captured=0;
    std::array<InputResponseObservation,16> responses{};
};
inline std::array<InputGraphObservation,SkillCount> inputResponseGraphs{};
inline std::array<ProjectileObservation,2> projectileObservations{};
inline std::array<std::array<ProjectileIdentity,256>,2> projectileIdentities{};
inline DWORD thread=0;
inline uint32_t previousInputs=0;
inline const Diagnostics& State(){return state;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool IsA(void* object,uintptr_t getter){return original_players::IsA(object,getter);}
inline bool Fail(uint32_t error,const char* stage){state.error=error;state.stage=stage;return false;}
inline bool Thread(){return !state.restartRequired&&thread==GetCurrentThreadId()&&original_players::Thread();}
inline bool Event(void* object,const wchar_t* name,uint16_t size,void* parameters){
    auto fn=engine::Find(name);
    const uint16_t actual=Read<uint16_t>(fn,0xbe);
    if(!object||!fn||actual!=size){state.failedEvent=name;state.expectedEventSize=size;state.actualEventSize=actual;return false;}
    auto vtable=Read<void**>(object,0);
    auto process=Read<void*>(vtable,0x298);
    if(!process)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,fn,parameters);return true;
}
inline LONG CaptureException(EXCEPTION_POINTERS* exception){
    state.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0001200U;
    const auto pc=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=pc>=engine::image?pc-engine::image:pc;state.exceptionOperation=state.stage;
    state.restartRequired=true;state.stage="original_jett_exception_requires_new_session";
    return EXCEPTION_EXECUTE_HANDLER;
}
inline void* LoadEquippableClass(const wchar_t* path){
    return engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t,void*)>(0x1d0c6e0)
        (engine::Class(0x645c660),nullptr,path,nullptr,0,nullptr);
}
inline bool ReadStateName(void* object,wchar_t (&output)[128]){
    struct Query{void* object;wchar_t* data=nullptr;int32_t count=0,capacity=0;} p{object};
    static_assert(sizeof(Query)==24);
    output[0]=0;
    if(!Event(engine::DefaultObject(engine::Class(0x45aa110)),L"/Script/Engine.KismetSystemLibrary.GetObjectName",24,&p))return false;
    const bool valid=p.count>=0&&p.count<=128&&p.capacity>=p.count
        &&(!p.count||(original_players::Readable(p.data,static_cast<size_t>(p.count)*sizeof(wchar_t))&&p.data[p.count-1]==0));
    if(valid&&p.count)std::memcpy(output,p.data,static_cast<size_t>(p.count)*sizeof(wchar_t));
    if(p.data)engine::function<void(*)(void*)>(0x1915720)(p.data);
    return valid;
}
inline bool ForceHandleActive(uint64_t handle,bool& active);
inline bool ObserveOriginalState(Skill& skill){
    if(!skill.actor)return true;
    skill.stateMachine=Read<void*>(skill.actor,0xe00);skill.stateMachinePresent=IsA(skill.stateMachine,0x67722c0);
    void* current=nullptr;
    if(!skill.stateMachinePresent||!Event(skill.stateMachine,L"/Script/ShooterGame.StateMachineComponent.GetCurrentState",8,&current))
        return Fail(1210,"original_jett_state_machine_query_failed");
    skill.currentState=current;
    const bool changed=skill.previousState!=current;
    if(changed){
        if(skill.previousState)++skill.stateTransitions;
        skill.stateNameQueryValid=current&&ReadStateName(current,skill.currentStateName);
    }
    skill.previousState=current;
    if(skill.forceBridgeState&&skill.forceBridgeState!=current){
        skill.forceBridgeState=nullptr;skill.forceBridgeAttempted=false;skill.forceHandleActive=false;
        skill.originalForceAlreadyActive=false;
    }
    skill.originalForceState=IsA(current,0x63d9f60);
    if(skill.originalForceState){
        if(changed)++skill.forceStateEntries;
        skill.forceHandle=Read<uint64_t>(current,0x4a0);
        skill.forceModuleClass=Read<void*>(current,0x498);
        bool active=false;
        skill.forceQueryValid=state.forceManager&&ForceHandleActive(skill.forceHandle,active);
        skill.forceHandleActive=skill.forceQueryValid&&active;
        if(skill.forceHandleActive)++skill.forceActiveObservations;
    }
    return true;
}
inline bool ObserveProjectiles(size_t kind){
    auto& observation=projectileObservations[kind];
    observation.queryValid=false;observation.worldCount=observation.localCount=0;
    if(!observation.classLoaded)return false;
    struct Query{void* world;void* type;void** actors=nullptr;int32_t count=0,capacity=0;} p{state.world,observation.type};
    static_assert(sizeof(Query)==32);
    if(!Event(engine::DefaultObject(engine::Class(0x43a9670)),L"/Script/Engine.GameplayStatics.GetAllActorsOfClass",32,&p))return false;
    const bool valid=p.count>=0&&p.count<=65536&&p.capacity>=p.count
        &&(!p.count||original_players::Readable(p.actors,static_cast<size_t>(p.count)*sizeof(void*)));
    if(valid){
        observation.worldCount=p.count;observation.queryValid=true;++observation.scans;
        auto& identities=projectileIdentities[kind];
        for(auto& record:identities)if(record.used){
            struct Weak{int32_t index,serial;} weak{record.index,record.serial};
            if(!engine::function<void*(*)(const Weak*)>(0x1d4cfa0)(&weak))record={};
        }
        for(int32_t i=0;i<p.count;++i){
            auto actor=p.actors[i];void* firingState=nullptr;
            if(!IsA(actor,0x644d990)||!Event(actor,L"/Script/ShooterGame.Projectile.GetFiringPlayerState",8,&firingState)
               ||firingState!=state.playerState)continue;
            ++observation.localCount;
            const auto initialized=Read<float>(actor,0x518);
            if(!std::isfinite(initialized))continue;
            const auto index=Read<int32_t>(actor,0xc);if(index<0)continue;
            const auto serial=engine::function<int32_t(*)(void*,int32_t)>(0x1cf35b0)
                (reinterpret_cast<void*>(engine::image+0xd7b87c0),index);
            if(serial<=0)continue;
            uint32_t initializedBits=0;std::memcpy(&initializedBits,&initialized,sizeof(initializedBits));
            ProjectileIdentity *record=nullptr,*empty=nullptr;
            for(auto& candidate:identities){
                if(candidate.used&&candidate.index==index&&candidate.serial==serial){record=&candidate;break;}
                if(!candidate.used&&!empty)empty=&candidate;
            }
            if(record&&record->initializedBits==initializedBits)continue;
            if(!record)record=empty;
            if(!record){++observation.trackingOverflow;continue;}
            *record={index,serial,initializedBits,true};++observation.localInitializations;
            observation.lastActor=actor;observation.lastPlayerState=firingState;observation.lastInitializedAt=initialized;
            observation.lastCleanedUp=Read<uint8_t>(actor,0x5d1)!=0;
            observation.creationQueryValid=Event(actor,L"/Script/ShooterGame.Projectile.GetCreationTransform",96,&observation.lastCreation)
                &&original_players::Finite(observation.lastCreation.translation);
        }
    }
    if(p.actors)engine::function<void(*)(void*)>(0x1915720)(p.actors);
    return valid;
}
inline bool StartingClassContains(void* type){
    auto classes=Read<void**>(state.pawn,0xbd8);
    const auto count=Read<int32_t>(state.pawn,0xbe0),capacity=Read<int32_t>(state.pawn,0xbe4);
    state.originalStartingClassCount=count;
    state.startingClassesReadable=count>=0&&count<=64&&capacity>=count&&capacity<=1024
        &&(!count||original_players::Readable(classes,static_cast<size_t>(count)*sizeof(void*)));
    if(!state.startingClassesReadable)return false;
    for(int32_t i=0;i<count;++i)if(Read<void*>(classes,static_cast<size_t>(i)*8)==type)return true;
    return false;
}
inline bool InitializeImpl(void* world,void* pawn){
    if(state.initialized)return state.world==world&&state.pawn==pawn;
    if(!engine::image||!world||!pawn||!original_players::Thread())return Fail(1201,"original_jett_context_missing");
    thread=GetCurrentThreadId();state.world=world;state.pawn=pawn;
    state.localAuthority=Read<uint8_t>(pawn,0x224)==3;
    auto jett=engine::Find(L"/Game/Characters/Wushu/Wushu_PC.Wushu_PC_C");
    state.originalWushu=jett&&original_players::Derived(Read<void*>(pawn,0x10),jett);
    if(!state.originalWushu||!state.localAuthority||!Read<uint8_t>(pawn,0xa90))
        return Fail(1202,"original_jett_requires_initialized_real_wushu_authority");
    state.inventory=Read<void*>(pawn,0xc08);state.playerState=Read<void*>(pawn,0x488);
    state.forceManager=Read<void*>(pawn,0x1100);
    state.originalInventory=IsA(state.inventory,0x6499330)&&Read<void*>(state.inventory,0xc8)==pawn
        &&Read<void*>(state.inventory,0x2f0)==pawn;
    if(!state.originalInventory||!IsA(state.playerState,original_players::ShooterStateClass))
        return Fail(1203,"original_jett_inventory_or_player_state_missing");
    state.originalStartingEquippableCount=Read<int32_t>(pawn,0xbc8);
    state.originalAbilityUIOrderCount=Read<int32_t>(pawn,0xb78);
    state.stage="original_jett_loading_original_classes";
    for(size_t i=0;i<SkillCount;++i){
        auto& skill=skills[i];skill.type=LoadEquippableClass(SkillPaths[i]);
        skill.classLoaded=skill.type&&original_players::Derived(skill.type,engine::Class(0x645c660));
        if(!skill.classLoaded)return Fail(1204,"original_jett_ability_class_missing");
        skill.defaults=engine::DefaultObject(skill.type);skill.slotType=Read<uint8_t>(skill.defaults,0x4a0);
        // The exact original EAresItemSlot table atc28c650 has these distinct
        // ability slots. Read each CDO, rather than guessing which key uses one.
        skill.defaultSlotValid=skill.defaults&&(skill.slotType==3||skill.slotType==4||skill.slotType==5||skill.slotType==9);
        skill.inStartingClasses=StartingClassContains(skill.type);
        if(!skill.defaultSlotValid)return Fail(1205,"original_jett_cdo_slot_unexpected");
        for(size_t other=0;other<i;++other)if(skills[other].slotType==skill.slotType)
            return Fail(1206,"original_jett_duplicate_original_slots");
    }
    // Classes only, for passive native-projectile provenance. This does not
    // create a projectile or alter an original projectile state/component.
    for(size_t i=0;i<projectileObservations.size();++i){
        auto& observation=projectileObservations[i];
        observation.type=engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t,void*)>(0x1d0c6e0)
            (engine::Class(0x3683900),nullptr,ProjectilePaths[i],nullptr,0,nullptr);
        observation.classLoaded=observation.type&&original_players::Derived(observation.type,engine::Class(0x644d990));
    }
    state.initialized=true;state.error=0;state.stage="original_jett_original_loadout_diagnosed";return true;
}
inline bool Initialize(void* world,void* pawn){
    __try{return InitializeImpl(world,pawn);}__except(CaptureException(GetExceptionInformation())){return false;}
}
inline bool EnsureSlot(Skill& skill){
    skill.slot=Read<void*>(state.inventory,0x1b8+skill.slotType*8);
    if(!skill.slot){
        struct Params{void* type;void* outer;void* result=nullptr;} p{engine::Class(0x68f63c0),state.inventory};
        if(!Event(engine::DefaultObject(engine::Class(0x43a9670)),L"/Script/Engine.GameplayStatics.SpawnObject",24,&p)||!p.result)
            return Fail(1207,"original_jett_missing_item_slot_constructor_failed");
        skill.slot=p.result;skill.slotCreated=true;
        if(!IsA(skill.slot,0x68f63c0)||Read<void*>(skill.slot,0x28)!=state.inventory
           ||!original_players::Write(skill.slot,0x30,skill.slotType)
           ||!original_players::Write(skill.slot,0x40,state.inventory)
           ||!original_players::Write(state.inventory,0x1b8+skill.slotType*8,skill.slot))
            return Fail(1208,"original_jett_item_slot_publish_failed");
    }
    if(!IsA(skill.slot,0x68f63c0)||IsA(skill.slot,0x69cf3d0)
       ||Read<uint8_t>(skill.slot,0x30)!=skill.slotType||Read<void*>(skill.slot,0x40)!=state.inventory)
        return Fail(1209,"original_jett_existing_slot_schema_mismatch");
    return true;
}
inline bool SampleImpl(){
    if(!state.initialized||!Thread())return false;
    for(size_t i=0;i<SkillCount;++i){
        auto& skill=skills[i];if(!skill.actor)continue;
        skill.ownerBound=Read<void*>(skill.actor,0x210)==state.pawn&&Read<void*>(skill.actor,0x4a8)==state.pawn;
        skill.slotPublished=Read<void*>(skill.slot,0x38)==skill.actor;
        skill.tickState=Read<uint8_t>(skill.actor,0x4f0);skill.tickContents=Read<uint8_t>(skill.actor,0x4f1);
        if(!ObserveOriginalState(skill))return false;
        auto& graph=responseGraphs[i];
        graph.component=reinterpret_cast<uintptr_t>(skill.stateMachine);
        graph.inactive_state=reinterpret_cast<uintptr_t>(Read<void*>(skill.stateMachine,0x350));
        original_weapon::CaptureInactiveResponseGraph(skill.stateMachine,graph);
        // That shared helper also compares three bindings to the rifle's
        // globals. Replace those diagnostic comparisons with this real skill's
        // native actor/owner/inventory; no engine or rifle state is written.
        auto inactive=reinterpret_cast<void*>(graph.inactive_state);
        if(graph.inactive_is_response_state){
            graph.inactive_native_pawn_owner_valid=engine::function<void*(*)(void*)>(0x7115010)(inactive)==state.pawn;
            graph.inactive_inventory_equipped_binding=original_weapon::OriginalDelegateBindsObject(state.inventory,0x120,inactive);
            graph.inactive_inventory_unequipped_binding=original_weapon::OriginalDelegateBindsObject(state.inventory,0x130,inactive);
            graph.inactive_owner_set_binding=original_weapon::OriginalDelegateBindsObject(skill.actor,0x480,inactive);
        }
        auto& inputs=inputResponseGraphs[i];inputs.captured=0;
        inputs.count=Read<int32_t>(inactive,0x318);
        const auto capacity=Read<int32_t>(inactive,0x31c);auto entries=Read<uint8_t*>(inactive,0x310);
        inputs.valid=graph.inactive_is_response_state&&inputs.count>=0&&inputs.count<=128&&capacity>=inputs.count
            &&(!inputs.count||original_players::Readable(entries,static_cast<size_t>(inputs.count)*96));
        if(inputs.valid)for(int32_t n=0;n<inputs.count&&n<16;++n){
            auto& response=inputs.responses[inputs.captured++];auto entry=entries+static_cast<size_t>(n)*96;
            response.input=Read<uint8_t>(entry,0);response.action=Read<uint8_t>(entry,1);response.activation=Read<uint8_t>(entry,2);
            response.delay=Read<float>(entry,0x10);response.canOutOfGameplay=Read<uint8_t>(entry,0x14)!=0;
            response.canSuppressed=Read<uint8_t>(entry,0x15)!=0;response.canInSpawnZones=Read<uint8_t>(entry,0x16)!=0;
            response.requiredTags=Read<int32_t>(entry,0x20);response.ignoredTags=Read<int32_t>(entry,0x40);
            auto target=Read<void*>(entry,0x58);
            if(target!=response.target)original_weapon::OriginalObjectName(target,response.targetName);
            response.target=target;
        }
        if(!skill.resource)skill.resource=original_weapon::Component(skill.actor,ResourceClass);
        skill.resourcePresent=IsA(skill.resource,ResourceClass);
        skill.resourceQueryValid=skill.resourcePresent;
        skill.resourcePredicted=Read<int32_t>(skill.resource,0x10c);skill.resourceAuthority=Read<int32_t>(skill.resource,0x110);
        skill.maximumCharges=IsA(skill.resource,EquipmentChargeClass)?Read<int32_t>(skill.resource,0x188):-1;
        if(i==3){
            skill.charges=-1;
            skill.resourceQueryValid=Event(skill.resource,DaggerRemaining,4,&skill.charges);
            uint8_t active=0;
            skill.daggersActiveQueryValid=Event(skill.resource,L"/Game/Characters/Wushu/S0/Ability_X/Comp_Wushu_X_DaggerResource.Comp_Wushu_X_DaggerResource_C.GetAreDaggersActive",1,&active);
            skill.daggersActive=skill.daggersActiveQueryValid&&active!=0;
        }else if(IsA(skill.resource,EquipmentChargeClass)){
            skill.charges=-1;
            skill.resourceQueryValid=Event(skill.resource,L"/Script/ShooterGame.EquipmentChargeComponent.GetCurrentCharges",4,&skill.charges);
        }else skill.charges=skill.resourcePredicted;
        if(skill.previousCharges>=0&&skill.charges!=skill.previousCharges)++skill.resourceChanges;
        skill.previousCharges=skill.charges;
        struct Activation{uint8_t slot,result=0;} activation{skill.slotType};
        skill.activationQueryValid=Event(state.pawn,L"/Script/ShooterGame.ShooterCharacter.GetCurrentActivationInputState",2,&activation);
        skill.activationState=activation.result;
        skill.currentEquippable=Read<void*>(state.inventory,0x280)==skill.actor;
        skill.desiredEquippable=Read<void*>(state.inventory,0x268)==skill.actor;
        uint8_t begun=0;skill.hasBegunPlay=Event(skill.actor,L"/Script/Engine.Actor.HasActorBegunPlay",1,&begun)&&begun!=0;
        skill.grantReady=skill.ownerBound&&skill.slotPublished&&skill.resourcePresent&&skill.stateMachinePresent&&skill.hasBegunPlay;
    }
    state.ultimateQueryValid=Event(state.pawn,L"/Script/ShooterGame.ShooterCharacter.GetNumPointsNeededForUltimate",4,&state.ultimatePointsRequired)
        &&Event(state.playerState,L"/Script/ShooterGame.ShooterPlayerState.GetNumUltimatePoints",4,&state.ultimatePoints);
    uint8_t ready=0;
    state.ultimateReady=Event(state.playerState,L"/Script/ShooterGame.ShooterPlayerState.IsUltimateReady",1,&ready)&&ready!=0;
    state.activeForceModuleCount=0;
    if(state.forceManager)Event(state.forceManager,L"/Script/ShooterGame.ForceModuleManagerComponent.GetActiveForceModuleCount",4,&state.activeForceModuleCount);
    // Polling can observe pooled or cleaned-up native projectiles too. Each
    // initialization is keyed by original UObject index+weak serial+timestamp;
    // a recycled address alone is never treated as a shot identity.
    for(size_t i=0;i<projectileObservations.size();++i)ObserveProjectiles(i);
    ++state.samples;state.error=0;state.stage="original_jett_original_skill_state_sampled";return true;
}
inline bool Sample(){__try{return SampleImpl();}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool GrantMissingLoadoutImpl(){
    if(!state.initialized||!Thread())return false;
    if(state.loadoutGranted)return SampleImpl();
    for(auto& skill:skills){
        if(skill.attempted)return Fail(1211,"original_jett_partial_grant_requires_new_session");
        skill.attempted=true;state.stage="original_jett_ensure_original_ability_slot";
        if(!EnsureSlot(skill))return false;
        auto contents=Read<void*>(skill.slot,0x38);
        if(contents){
            if(!original_players::Derived(Read<void*>(contents,0x10),skill.type))
                return Fail(1212,"original_jett_ability_slot_occupied_by_other_item");
            skill.actor=contents;skill.alreadyPresent=true;continue;
        }
        engine::Vec location{};
        if(!Event(state.pawn,L"/Script/Engine.Actor.K2_GetActorLocation",24,&location))return Fail(1213,"original_jett_pawn_location_query_failed");
        engine::Transform transform;transform.translation=location;
        state.stage="original_jett_begin_original_ability_actor";
        skill.actor=engine::function<void*(*)(void*,void*,const engine::Transform*,uint8_t,void*,uint8_t)>(0x43fada0)
            (state.world,skill.type,&transform,1,state.pawn,0);
        if(!skill.actor)return Fail(1214,"original_jett_original_ability_spawn_failed");
        skill.created=true;state.stage="original_jett_finish_original_ability_scs";
        engine::function<void(*)(void*,const engine::Transform*,bool,void*,uint8_t)>(0x3ef6cb0)
            (skill.actor,&transform,false,nullptr,0);
        if(!original_ability_tuning::EnsureActor(skill.actor))
            return Fail(original_ability_tuning::State().error,original_ability_tuning::State().stage);
        if(!original_effects::EnsureActor(skill.actor,"local_ability"))
            return Fail(original_effects::State().error,original_effects::State().stage);
        state.stage="original_jett_bind_original_ability_owner";
        if(!Event(skill.actor,L"/Script/Engine.Actor.SetOwner",8,&state.pawn)||Read<void*>(skill.actor,0x210)!=state.pawn)
            return Fail(1215,"original_jett_original_ability_actor_owner_failed");
        if(Read<void*>(skill.actor,0x4a8)!=state.pawn){
            auto table=Read<void**>(skill.actor,0);
            if(Read<void*>(table,0x7d8)!=reinterpret_cast<void*>(engine::image+0x70ec480))
                return Fail(1216,"original_jett_equippable_owner_callback_changed");
            reinterpret_cast<void(*)(void*,void*,void*)>(table[0x7d8/8])(skill.actor,nullptr,state.pawn);
        }
        if(Read<void*>(skill.actor,0x4a8)!=state.pawn)return Fail(1217,"original_jett_original_ability_typed_owner_failed");
        state.stage="original_jett_publish_local_authority_ability_grant";
        // The same missing authority data as the original AK grant: original
        // ItemSlot.AuthAddItem is a false stub. Do not replace ability states.
        uint8_t inInventory=1;
        if(!original_players::Write(skill.slot,0x38,skill.actor)
           ||!original_players::Write(skill.actor,0x4b0,inInventory)
           ||!Event(skill.slot,L"/Script/ShooterGame.ItemSlot.OnRep_Contents",0,nullptr))
            return Fail(1218,"original_jett_original_slot_contents_notification_failed");
    }
    if(!SampleImpl())return false;
    for(const auto& skill:skills)if(!skill.grantReady)return Fail(1219,"original_jett_original_ability_grant_not_observed");
    state.loadoutGranted=true;state.error=0;state.stage="original_jett_loadout_granted_execution_unverified";return true;
}
inline bool GrantMissingLoadout(){__try{return GrantMissingLoadoutImpl();}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool RefillImpl(){
    if(!state.loadoutGranted||!Thread()||!SampleImpl())return false;
    state.stage="original_jett_refill_training_resources";
    for(size_t i=0;i<SkillCount;++i){
        auto& skill=skills[i];
        if(!skill.resourcePresent)return Fail(1220,"original_jett_resource_missing_for_refill");
        if(i==3){
            // The original dagger Blueprint owns MaxDaggers, its multicast
            // update and HUD notifications. Do not write a guessed BP offset.
            if(!Event(skill.resource,DaggerReplenish,0,nullptr))return Fail(1221,"original_jett_original_dagger_refill_missing");
        }else{
            if(skill.maximumCharges<=0||skill.maximumCharges>100)return Fail(1222,"original_jett_original_maximum_charges_invalid");
            const auto amount=(std::max)(0,skill.maximumCharges-skill.charges);
            if(amount){
                struct Replenish{int32_t units,result=0;} p{amount};
                if(!Event(skill.resource,L"/Script/ShooterGame.ResourceComponent.Replenish",8,&p))
                    return Fail(1223,"original_jett_original_resource_replenish_missing");
            }
        }
        skill.chargeRefillRequested=true;++skill.refillRequests;
    }
    if(!state.ultimateQueryValid||state.ultimatePointsRequired<=0||state.ultimatePointsRequired>100)
        return Fail(1224,"original_jett_original_ultimate_requirement_unavailable");
    // AuthGiveUltimate/AuthSetUltimatePoints are stripped. Supply this one
    // authorized local training authority record, then let the original OnRep
    // update original ability/UI observers. No account data or global CDO edit.
    if(state.ultimatePoints<state.ultimatePointsRequired){
        if(!original_players::Write(state.playerState,0xbdc,state.ultimatePointsRequired)
           ||!Event(state.playerState,L"/Script/ShooterGame.ShooterPlayerState.OnRep_NumUltimatePoints",0,nullptr))
            return Fail(1225,"original_jett_original_ultimate_notification_failed");
    }
    ++state.refillCalls;return SampleImpl();
}
inline bool Refill(){__try{return RefillImpl();}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool ForceHandleActive(uint64_t handle,bool& active){
    struct Query{uint64_t handle;uint8_t result=0,pad[7]{};} p{handle};
    // Return bool ends at+8; runtime UFunction.ParmsSize=9, aligned buffer16.
    if(!Event(state.forceManager,L"/Script/ShooterGame.ForceModuleManagerComponent.HasActiveForceModule",9,&p))return false;
    active=p.result!=0;return true;
}
inline bool BridgeMissingActiveForceModulesImpl(){
    if(!state.loadoutGranted||!Thread())return false;
    // This optional bridge can be polled per native tick without performing
    // the slower world-projectile or resource diagnostic snapshots each time.
    for(auto& skill:skills)if(!ObserveOriginalState(skill))return false;
    if(!IsA(state.forceManager,0x685f510))return Fail(1227,"original_jett_original_force_manager_missing");
    // Opt-in local authority bridge, only after the ORIGINAL ability state has
    // entered its real ApplyForceModuleState. It does not activate a skill or
    // choose a motion formula. The full original deterministic factory still
    // owns resistance/movement gates, allocation, initialization and simulation.
    for(size_t i=1;i<=2;++i){
        auto& skill=skills[i];auto current=skill.currentState;
        if(!IsA(current,0x63d9f60))continue;
        if(skill.forceBridgeState==current&&skill.forceBridgeAttempted)continue;
        skill.forceHandle=Read<uint64_t>(current,0x4a0);
        if(skill.forceHandle){
            if(!ForceHandleActive(skill.forceHandle,skill.forceHandleActive))
                return Fail(1233,"original_jett_original_force_handle_query_failed");
            if(skill.forceHandleActive){
                skill.forceBridgeState=current;skill.forceBridgeAttempted=true;skill.originalForceAlreadyActive=true;continue;
            }
        }
        skill.forceBridgeState=current;skill.forceBridgeAttempted=true;++skill.forceBridgeAttempts;
        state.stage="original_jett_bridge_missing_authority_force_application";
        skill.forceModuleClass=Read<void*>(current,0x498);
        if(!original_players::Derived(skill.forceModuleClass,engine::Class(0x6858850)))
            return Fail(1228,"original_jett_active_force_state_module_class_missing");
        skill.originalForceDuration=0;
        if(!Event(current,L"/Script/ShooterGame.TimedStateComponent.GetTimerLength",4,&skill.originalForceDuration)
           ||!std::isfinite(skill.originalForceDuration)||skill.originalForceDuration<=0||skill.originalForceDuration>10)
            return Fail(1229,"original_jett_original_force_state_duration_unavailable");
        const auto manager=Read<void*>(current,0x4a8);
        if(manager&&manager!=state.forceManager)return Fail(1230,"original_jett_force_state_target_manager_mismatch");
        auto source=Read<void*>(current,0x4b0);
        if(!source)source=skill.actor;
        // AuthSetSourceActor and authority state entry are absent in this client.
        // These two real instance links are the missing grant bookkeeping; no
        // force velocity, curve, integrator, tick flags or actor Role is changed.
        if(!original_players::Write(current,0x4a8,state.forceManager)
           ||!original_players::Write(current,0x4b0,source))return Fail(1231,"original_jett_force_state_local_links_failed");
        struct Apply{void* source;void* type;float duration;uint32_t padding=0;void* context;uint64_t handle=0;}
            p{source,skill.forceModuleClass,skill.originalForceDuration,0,skill.actor};
        static_assert(sizeof(Apply)==40&&offsetof(Apply,handle)==32);
        if(!Event(state.forceManager,L"/Script/ShooterGame.ForceModuleManagerComponent.ApplyDeterministicForceModule",40,&p))
            return Fail(1232,"original_jett_original_deterministic_force_factory_missing");
        skill.forceHandle=p.handle;
        if(!ForceHandleActive(p.handle,skill.forceHandleActive))return Fail(1233,"original_jett_original_force_handle_query_failed");
        if(skill.forceHandleActive&&!original_players::Write(current,0x4a0,p.handle))
            return Fail(1234,"original_jett_original_force_handle_association_failed");
        if(skill.forceHandleActive){++skill.forceBridgeSuccessful;++skill.forceActiveObservations;}
    }
    state.error=0;state.stage="original_jett_active_force_bridge_sampled";return true;
}
inline bool BridgeMissingActiveForceModules(){
    __try{return BridgeMissingActiveForceModulesImpl();}__except(CaptureException(GetExceptionInformation())){return false;}
}
inline bool UpdateInputImpl(uint32_t flags){
    if(!state.loadoutGranted||!Thread())return false;
    // Passive queries every native input update retain short original state
    // transitions/force activity between the slower JSON publication samples.
    // No state entry, advance, projectile fire or force application occurs here.
    for(auto& skill:skills)if(!ObserveOriginalState(skill))return false;
    const uint32_t mask=(flags&8)?flags&(64|128|256|512):0;
    for(size_t i=0;i<SkillCount;++i){
        const auto bit=InputBits[i];if((mask&bit)==(previousInputs&bit))continue;
        // Original enum c277230: StandardInput0. Pressed1/Released0 are the
        // original EAresInputState values. The original native scheduler stamps
        // this event and routes it to the original ability state machine.
        struct Activation{uint8_t slot,inputState,activationType=0,index=0;} p{skills[i].slotType,static_cast<uint8_t>((mask&bit)?1:0)};
        state.stage="original_jett_submit_original_activation_input";
        if(!Event(state.pawn,L"/Script/ShooterGame.ShooterCharacter.ForceActivationInputState",4,&p))
            return Fail(1226,"original_jett_original_activation_input_missing");
        ++skills[i].inputEdges;++state.activationSubmissions;
    }
    previousInputs=mask;state.error=0;state.stage="original_jett_activation_inputs_submitted";return true;
}
inline bool UpdateInput(uint32_t flags){__try{return UpdateInputImpl(flags);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline void ReleaseInputs(){if(state.loadoutGranted)UpdateInput(0);}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;std::wstring temporary(path);temporary+=L".tmp";
    FILE* f=nullptr;if(_wfopen_s(&f,temporary.c_str(),L"wb")||!f)return false;
    const auto b=[](bool v){return v?"true":"false";};
    std::fprintf(f,"{\"schema_version\":2,\"pid\":%lu,\"stage\":",GetCurrentProcessId());original_players::Text(f,state.stage);
    std::fprintf(f,",\"error\":%u,\"initialized\":%s,\"loadout_granted\":%s,\"restart_required\":%s,\"original_wushu\":%s,\"original_inventory\":%s,\"local_authority\":%s,\"exception_rva\":%llu,\"exception_operation\":",state.error,b(state.initialized),b(state.loadoutGranted),b(state.restartRequired),b(state.originalWushu),b(state.originalInventory),b(state.localAuthority),static_cast<unsigned long long>(state.exceptionRva));original_players::Text(f,state.exceptionOperation);
    std::fprintf(f,",\"original_starting_class_count\":%d,\"starting_classes_readable\":%s,\"original_starting_equippable_count\":%d,\"original_ability_ui_order_count\":%d,\"ultimate_points\":%d,\"ultimate_required\":%d,\"ultimate_ready\":%s,\"ultimate_query_valid\":%s,\"active_force_modules\":%d,\"refill_calls\":%u,\"activation_submissions\":%u,\"samples\":%u,\"failed_event\":",state.originalStartingClassCount,b(state.startingClassesReadable),state.originalStartingEquippableCount,state.originalAbilityUIOrderCount,state.ultimatePoints,state.ultimatePointsRequired,b(state.ultimateReady),b(state.ultimateQueryValid),state.activeForceModuleCount,state.refillCalls,state.activationSubmissions,state.samples);
    const auto failed=original_players::Utf8(state.failedEvent);original_players::Text(f,failed.c_str());
    std::fprintf(f,",\"expected_event_size\":%u,\"actual_event_size\":%u,\"world\":\"%p\",\"pawn\":\"%p\",\"inventory\":\"%p\",\"player_state\":\"%p\",\"skills\":[",state.expectedEventSize,state.actualEventSize,state.world,state.pawn,state.inventory,state.playerState);
    for(size_t i=0;i<SkillCount;++i){
        const auto& s=skills[i];if(i)std::fputc(',',f);
        std::fprintf(f,"{\"name\":\"%s\",\"class_loaded\":%s,\"default_slot_valid\":%s,\"slot_type\":%u,\"in_starting_classes\":%s,\"created\":%s,\"already_present\":%s,\"slot_created\":%s,\"slot_published\":%s,\"owner_bound\":%s,\"resource_present\":%s,\"state_machine_present\":%s,\"grant_ready\":%s,\"resource_query_valid\":%s,\"activation_query_valid\":%s,\"activation_state\":%u,\"charges\":%d,\"maximum_charges\":%d,\"resource_predicted\":%d,\"resource_authority\":%d,\"current_equippable\":%s,\"desired_equippable\":%s,\"has_begun_play\":%s,\"tick_state\":%u,\"tick_contents\":%u,\"input_edges\":%u,\"state_transitions\":%u,\"resource_changes\":%u,\"refill_requests\":%u,\"actor\":\"%p\",\"slot\":\"%p\",\"resource\":\"%p\",\"state_machine\":\"%p\",\"current_state\":\"%p\"",SkillNames[i],b(s.classLoaded),b(s.defaultSlotValid),static_cast<unsigned>(s.slotType),b(s.inStartingClasses),b(s.created),b(s.alreadyPresent),b(s.slotCreated),b(s.slotPublished),b(s.ownerBound),b(s.resourcePresent),b(s.stateMachinePresent),b(s.grantReady),b(s.resourceQueryValid),b(s.activationQueryValid),static_cast<unsigned>(s.activationState),s.charges,s.maximumCharges,s.resourcePredicted,s.resourceAuthority,b(s.currentEquippable),b(s.desiredEquippable),b(s.hasBegunPlay),static_cast<unsigned>(s.tickState),static_cast<unsigned>(s.tickContents),s.inputEdges,s.stateTransitions,s.resourceChanges,s.refillRequests,s.actor,s.slot,s.resource,s.stateMachine,s.currentState);
        std::fprintf(f,",\"current_state_name\":");const auto label=original_players::Utf8(s.currentStateName);original_players::Text(f,label.c_str());
        std::fprintf(f,",\"state_name_query_valid\":%s,\"original_force_state\":%s,\"force_query_valid\":%s,\"force_state_entries\":%u,\"force_active_observations\":%u,\"force_bridge_successful\":%u,\"daggers_active_query_valid\":%s,\"daggers_active\":%s",b(s.stateNameQueryValid),b(s.originalForceState),b(s.forceQueryValid),s.forceStateEntries,s.forceActiveObservations,s.forceBridgeSuccessful,b(s.daggersActiveQueryValid),b(s.daggersActive));
        std::fprintf(f,",\"force_bridge_attempts\":%u,\"force_handle_active\":%s,\"original_force_already_active\":%s,\"force_handle\":%llu,\"original_force_duration\":%.9g,\"force_module_class\":\"%p\"",s.forceBridgeAttempts,b(s.forceHandleActive),b(s.originalForceAlreadyActive),static_cast<unsigned long long>(s.forceHandle),s.originalForceDuration,s.forceModuleClass);
        const auto& g=responseGraphs[i];
        std::fprintf(f,",\"inactive_graph\":{\"component\":%llu,\"inactive_state\":%llu,\"starting_state\":%llu,\"starting_state_name\":",static_cast<unsigned long long>(g.component),static_cast<unsigned long long>(g.inactive_state),static_cast<unsigned long long>(g.starting_state));original_players::Text(f,g.starting_state_name);
        std::fprintf(f,",\"response_state\":%s,\"machine_owner_valid\":%s,\"pawn_owner_valid\":%s,\"equipped_binding\":%s,\"unequipped_binding\":%s,\"owner_set_binding\":%s,\"event_map_valid\":%s,\"event_count\":%d,\"inventory_array_valid\":%s,\"inventory_count\":%d,\"events\":[",b(g.inactive_is_response_state),b(g.inactive_machine_owner_valid),b(g.inactive_native_pawn_owner_valid),b(g.inactive_inventory_equipped_binding),b(g.inactive_inventory_unequipped_binding),b(g.inactive_owner_set_binding),b(g.inactive_response_map_valid),g.inactive_response_count,b(g.inactive_inventory_responses_valid),g.inactive_inventory_response_count);
        for(unsigned n=0;n<g.captured_response_count;++n){
            const auto& r=g.responses[n];if(n)std::fputc(',',f);
            std::fprintf(f,"{\"event_class\":%llu,\"event_name\":",static_cast<unsigned long long>(r.event_class));original_players::Text(f,r.event_name);
            std::fprintf(f,",\"next_state\":%llu,\"next_state_name\":",static_cast<unsigned long long>(r.next_state));original_players::Text(f,r.next_state_name);std::fputc('}',f);
        }
        std::fputs("],\"inventory\":[",f);
        for(unsigned n=0;n<g.captured_inventory_response_count;++n){
            const auto& r=g.inventory_responses[n];if(n)std::fputc(',',f);
            std::fprintf(f,"{\"equip_type\":%u,\"filtered_class_count\":%d,\"next_state\":%llu,\"next_state_name\":",static_cast<unsigned>(r.equip_type),r.filtered_class_count,static_cast<unsigned long long>(r.next_state));original_players::Text(f,r.next_state_name);std::fputc('}',f);
        }
        const auto& inputGraph=inputResponseGraphs[i];
        std::fprintf(f,"],\"input_array_valid\":%s,\"input_count\":%d,\"inputs\":[",b(inputGraph.valid),inputGraph.count);
        for(unsigned n=0;n<inputGraph.captured;++n){
            const auto& r=inputGraph.responses[n];if(n)std::fputc(',',f);
            std::fprintf(f,"{\"input\":%u,\"action\":%u,\"activation_type\":%u,\"response_delay\":%.9g,\"can_out_of_gameplay\":%s,\"can_suppressed\":%s,\"can_in_spawn_zones\":%s,\"required_tags\":%d,\"ignored_tags\":%d,\"next_state\":\"%p\",\"next_state_name\":",static_cast<unsigned>(r.input),static_cast<unsigned>(r.action),static_cast<unsigned>(r.activation),std::isfinite(r.delay)?r.delay:0.0f,b(r.canOutOfGameplay),b(r.canSuppressed),b(r.canInSpawnZones),r.requiredTags,r.ignoredTags,r.target);original_players::Text(f,r.targetName);std::fputc('}',f);
        }
        std::fputs("]}}",f);
    }
    std::fputs("],\"projectiles\":{",f);
    for(size_t i=0;i<projectileObservations.size();++i){
        const auto& p=projectileObservations[i];if(i)std::fputc(',',f);
        std::fprintf(f,"\"%s\":{\"class_loaded\":%s,\"class\":\"%p\",\"query_valid\":%s,\"world_count\":%d,\"local_count\":%d,\"scans\":%llu,\"local_initializations\":%llu,\"tracking_overflow\":%llu,\"last_actor\":\"%p\",\"last_firing_player_state\":\"%p\",\"last_initialized_at\":%.9g,\"last_cleaned_up\":%s,\"creation_query_valid\":%s,\"last_creation_location\":[%.9g,%.9g,%.9g],\"last_creation_quaternion\":[%.9g,%.9g,%.9g,%.9g]}",
            i?"dagger":"smoke",b(p.classLoaded),p.type,b(p.queryValid),p.worldCount,p.localCount,
            static_cast<unsigned long long>(p.scans),static_cast<unsigned long long>(p.localInitializations),
            static_cast<unsigned long long>(p.trackingOverflow),p.lastActor,p.lastPlayerState,p.lastInitializedAt,b(p.lastCleanedUp),b(p.creationQueryValid),
            p.lastCreation.translation.x,p.lastCreation.translation.y,p.lastCreation.translation.z,
            p.lastCreation.rotation[0],p.lastCreation.rotation[1],p.lastCreation.rotation[2],p.lastCreation.rotation[3]);
    }
    std::fputs("},\"skill_execution_verified\":false,\"original_animation_pixels_verified\":false,\"uses_substitute_single_node_presentation\":false}\n",f);
    const bool ok=std::ferror(f)==0;const bool closed=std::fclose(f)==0;
    return ok&&closed&&MoveFileExW(temporary.c_str(),path,MOVEFILE_REPLACE_EXISTING);
}
}
