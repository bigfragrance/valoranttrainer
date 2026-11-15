#pragma once
#include "local_server_ability_spawn.h"
#include "local_server_ability_arena.h"
#include "local_server_ability_subequippable.h"
#include <array>
#include <cstdint>
#include <cstdio>

// Candidate HOST coordination only. The framework proves an actual registered
// human/epoch; the two source-bound factory adapters keep original construction,
// ownership and lifetime. Existing original actors reach the parent's real
// ActorChannels through its registry visitor, never through a client factory.
// This is partial authority support (Veto E / Hush C factories, authored native
// child-firearm assembly and Iso preflight),
// not a claim that all hero gameplay, Arena execution or networking is verified.
namespace local_server_hero_authority {
namespace framework=local_server_hero_abilities;
namespace spawn=local_server_ability_spawn;
namespace arena=local_server_ability_arena;
namespace subequipment=local_server_ability_subequippable;
using View=framework::SourceView;
using Weak=original_inventory_slots::WeakIdentity;
inline constexpr size_t MaxSourceProofs=512,MaxRetiringEpochs=framework::MaxPlayers;

struct LifecycleHooks {
    // Optional future producer composition, after the source admission fence.
    // No unverified damage/subequippable/authority-init module is included here.
    bool (*retirePlayer)(void* pawn,uint32_t epoch)=nullptr;
    bool (*close)()=nullptr;
};
struct SourceProof {View view{};bool used=false;};
struct RetiringEpoch {
    void* pawn=nullptr;Weak identity{};uint32_t epoch=0;uint64_t registration=0;bool used=false;
};
struct ArenaActor {
    void *actor=nullptr,*world=nullptr,*owner=nullptr,*type=nullptr;
    Weak identity{},worldId{},ownerId{},typeId{};
    uint32_t sourceEpoch=0;uint64_t sourceRegistration=0;
    bool used=false,originalReplicates=false;
};
struct State {
    bool initialized=false,closed=false,closing=false,restartRequired=false,hostContextVerified=false;
    bool policyKnown=false,skillsEnabled=false,deathmatch=false,sourceGate=false,sourceVerified=false;
    bool spawnInstalled=false,arenaPreflightActive=false;
    bool subequipmentOwned=false,subequipmentInitialized=false,subequipmentActive=false;
    void *world=nullptr,*policyPawn=nullptr;Weak worldId{},policyPawnId{};DWORD gameThread=0;
    unsigned error=0,heroIndex=UINT32_MAX;uint32_t sourceEpoch=0;uint64_t sourceRegistration=0;
    uint32_t cachedProofs=0,retiringEpochs=0,arenaActors=0;
    uint64_t polls=0,sourceQueries=0,sourceAccepted=0,sourceRejected=0,sourceValidations=0;
    uint64_t weakProofDeferrals=0,proofPruned=0,retirements=0,closes=0;
    uint64_t registryVisits=0,spawnActorVisits=0,arenaActorVisits=0,arenaActorRegistrations=0;
    uint64_t nativeNonreplicated=0,arenaActorsPruned=0,disabledPolls=0;
    uint64_t subequipmentRegistrations=0,subequipmentPolls=0,subequipmentMaintenancePolls=0,subequipmentGraphVisits=0,subequipmentRetirements=0;
    uintptr_t exceptionRva=0;
    const char* stage="hero_authority_not_initialized";
};
inline State state{},cached{};
inline LifecycleHooks lifecycle{};
inline std::array<SourceProof,MaxSourceProofs> proofs{};
inline std::array<RetiringEpoch,MaxRetiringEpochs> retiring{};
inline std::array<ArenaActor,arena::MaximumArenas> arenaActors{};
inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline const State& GetState(){return state;} // Original game thread only.
inline void Cache(){AcquireSRWLockExclusive(&cacheLock);cached=state;ReleaseSRWLockExclusive(&cacheLock);}
inline State Snapshot(){AcquireSRWLockShared(&cacheLock);auto value=cached;ReleaseSRWLockShared(&cacheLock);return value;}
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Capture(void* object,Weak& identity){return original_inventory_slots::CaptureIdentity(object,identity)&&Live(object,identity);}
inline bool Thread(){return engine::image&&original_players::Thread()&&state.gameThread==GetCurrentThreadId();}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.sourceGate=false;state.stage=stage;Cache();return false;}
inline bool Wait(const char* stage){state.stage=stage;Cache();return true;}
inline bool SetLifecycleHooks(const LifecycleHooks& value){
    if(state.initialized||state.world||state.closed||state.closing)return false;lifecycle=value;return true;
}
inline bool Fields(const View& a,const View& b){
    return a.contextObject==b.contextObject&&a.world==b.world&&a.controller==b.controller&&a.playerState==b.playerState
        &&a.pawn==b.pawn&&a.inventory==b.inventory&&a.ability==b.ability&&a.heroIndex==b.heroIndex
        &&a.sourceEpoch==b.sourceEpoch&&a.registration==b.registration&&a.pendingGrant==b.pendingGrant;
}
inline bool Fields(const spawn::Source& a,const View& b){
    return a.contextObject==b.contextObject&&a.world==b.world&&a.controller==b.controller&&a.playerState==b.playerState
        &&a.pawn==b.pawn&&a.inventory==b.inventory&&a.ability==b.ability&&a.heroIndex==b.heroIndex
        &&a.sourceEpoch==b.sourceEpoch&&a.registration==b.registration&&a.pendingGrant==b.pendingGrant;
}
inline bool WeakFields(const View& a,const View& b){
    const void* objects[]{a.contextObject,a.world,a.controller,a.playerState,a.pawn,a.inventory,a.ability};
    for(size_t n=0;n<a.identities.size();++n){
        if(!objects[n]){
            // Only a real Pawn-context view has no ability. Preserve the empty
            // Weak rather than inventing an ability identity for that view.
            if(n!=6||a.ability||b.ability||a.identities[n].index!=-1||a.identities[n].serial
               ||b.identities[n].index!=-1||b.identities[n].serial)return false;
            continue;
        }
        if(!original_inventory_slots::SameIdentity(a.identities[n],b.identities[n])
           ||!Live(const_cast<void*>(objects[n]),a.identities[n]))return false;
    }
    return true;
}
inline bool Retiring(const View& view){
    for(const auto& r:retiring)if(r.used&&r.pawn==view.pawn&&r.epoch==view.sourceEpoch
        &&r.registration==view.registration&&original_inventory_slots::SameIdentity(r.identity,view.identities[4]))return true;
    return false;
}
inline bool Policy(const View& view){
    return state.initialized&&!state.closed&&!state.closing&&!state.restartRequired&&!state.error&&Thread()
        &&state.policyKnown&&state.skillsEnabled&&!state.deathmatch&&state.sourceGate
        &&view.world==state.world&&Live(state.world,state.worldId)&&view.pawn==state.policyPawn
        &&original_inventory_slots::SameIdentity(view.identities[4],state.policyPawnId)
        &&Live(state.policyPawn,state.policyPawnId)&&view.sourceEpoch&&view.registration&&!Retiring(view);
}
inline bool RegisteredSource(void* context,View& out){
    out={};
    if(framework::ResolveSource(context,out)&&framework::ValidateSource(out))return true;
    out={};
    return state.subequipmentOwned&&subequipment::ResolveOwnedSubequippable(context,out)
        &&subequipment::ValidateOwnedSource(out);
}
inline bool RegisteredCurrent(const View& before,View& after){
    return RegisteredSource(before.contextObject,after)
        &&Fields(before,after)&&WeakFields(before,after);
}
// This is an admission FILTER, never a SourceView producer. The subequipment
// module still proves its real child/component/class/slot/native membership.
// Do not call its resolver here: ProofAlive -> guard -> resolver would recurse.
inline bool AllowSubequipmentSourceImpl(const View& source){
    if(!Policy(source)||source.heroIndex==0||source.heroIndex>=original_heroes::PlayableHeroCount
       ||!source.contextObject||!source.ability)return false;
    View human{};
    if(!framework::ResolveSource(source.pawn,human)||!framework::ValidateSource(human)
       ||human.contextObject!=source.pawn||human.ability||human.pendingGrant||human.world!=source.world
       ||human.controller!=source.controller||human.playerState!=source.playerState||human.pawn!=source.pawn
       ||human.inventory!=source.inventory||human.heroIndex!=source.heroIndex||human.sourceEpoch!=source.sourceEpoch
       ||human.registration!=source.registration)return false;
    for(size_t n=1;n<=5;++n)if(!original_inventory_slots::SameIdentity(source.identities[n],human.identities[n]))return false;
    if(!Live(source.contextObject,source.identities[0])||!Live(source.ability,source.identities[6])
       ||Read<uint8_t>(source.ability,0x224)!=3||Read<void*>(source.ability,0x210)!=source.pawn
       ||Read<void*>(source.ability,0x4a8)!=source.pawn||spawn::ActorWorld(source.ability)!=source.world)return false;
    void* connection=nullptr;
    return local_server_item_ownership::ActualHuman(source.world,source.controller,source.playerState,source.pawn,source.inventory,connection);
}
// Public composition proof for later frozen producers. Policy and exact current
// pending/publication stage are checked each time; an already-created deployable
// is NOT required to keep a living source merely to remain in replication.
inline bool ResolveHostSourceImpl(void* caller,View& out){
    out={};if(!state.initialized||!Thread()||state.closed||state.closing)return false;
    ++state.sourceQueries;
    View view{},after{};
    if(!caller||!RegisteredSource(caller,view)||!Policy(view)||!RegisteredCurrent(view,after)){
        ++state.sourceRejected;return false;}
    out=after;++state.sourceAccepted;return true;
}
inline bool ValidateHostSourceImpl(const View& before){
    if(!state.initialized||!Thread()||state.closed||state.closing)return false;
    ++state.sourceValidations;View after{};
    return Policy(before)&&RegisteredCurrent(before,after)&&Policy(after);
}
inline LONG Exception(EXCEPTION_POINTERS* details){
    const auto address=details&&details->ExceptionRecord?reinterpret_cast<uintptr_t>(details->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=address>=engine::image?address-engine::image:address;state.restartRequired=true;
    Fail(6990,"hero_authority_bridge_exception_requires_new_owned_pair");return EXCEPTION_EXECUTE_HANDLER;
}
inline bool ResolveHostSource(void* caller,View& out){
    __try{return ResolveHostSourceImpl(caller,out);}
    __except(Exception(GetExceptionInformation())){out={};return false;}
}
inline bool ValidateHostSource(const View& before){
    __try{return ValidateHostSourceImpl(before);}
    __except(Exception(GetExceptionInformation())){return false;}
}
inline bool AllowSubequipmentSource(const View& source){
    __try{return AllowSubequipmentSourceImpl(source);}
    __except(Exception(GetExceptionInformation())){return false;}
}
inline void Convert(const View& view,spawn::Source& out){
    out.contextObject=view.contextObject;out.world=view.world;out.controller=view.controller;
    out.playerState=view.playerState;out.pawn=view.pawn;out.inventory=view.inventory;out.ability=view.ability;
    out.heroIndex=view.heroIndex;out.sourceEpoch=view.sourceEpoch;out.registration=view.registration;out.pendingGrant=view.pendingGrant;
}
inline bool Remember(const View& view){
    SourceProof* available=nullptr;
    for(auto& p:proofs){
        if(!p.used){if(!available)available=&p;continue;}
        if(!Fields(p.view,view))continue;
        if(WeakFields(p.view,view))return true;
        // Do not replace an in-flight context's immutable Weak proof with a
        // reused pointer. The next idle GT poll can retire the expired proof.
        ++state.weakProofDeferrals;return false;
    }
    if(!available)return Fail(6950,"hero_authority_original_source_proof_capacity_exceeded");
    available->used=true;available->view=view;++state.cachedProofs;return true;
}
inline bool ResolveSpawnImpl(void* caller,spawn::Source& out){
    out={};View view{};
    if(!ResolveHostSource(caller,view)||!view.ability)return false;
    spawn::Source next{};Convert(view,next);
    // No admit override is installed: only the two exact original source and
    // class cases in the frozen adapter may reach its original factory.
    if(!spawn::KnownSource(next)||!Remember(view))return false;
    out=next;return true;
}
inline bool ValidateSpawnImpl(const spawn::Source& source){
    if(!source.ability)return false;
    for(const auto& proof:proofs)if(proof.used&&Fields(source,proof.view))return ValidateHostSource(proof.view);
    return false;
}
inline bool ResolveSpawn(void* caller,spawn::Source& out){
    __try{return ResolveSpawnImpl(caller,out);}
    __except(Exception(GetExceptionInformation())){out={};return false;}
}
inline bool ValidateSpawn(const spawn::Source& source){
    __try{return ValidateSpawnImpl(source);}
    __except(Exception(GetExceptionInformation())){return false;}
}
inline bool FactoryOwned(){
    const auto& factory=spawn::GetState();
    if(!factory.installed||factory.closed||factory.world!=state.world||spawn::hooks.resolve!=ResolveSpawn
       ||spawn::hooks.validate!=ValidateSpawn||spawn::hooks.admit||spawn::hooks.prepare)return false;
    for(const auto& function:spawn::functions)if(!function.installed||!spawn::Live(function.object,function.identity)
       ||Read<spawn::Exec>(function.object,0xe0)!=function.replacement)return false;
    return true;
}
inline bool SubequipmentBindingsOwned(){
    const auto& s=subequipment::GetState();
    if(!s.initialized||!s.host||!s.installed||s.closed||s.world!=state.world||subequipment::sourceAllowed!=AllowSubequipmentSource)return false;
    const subequipment::FunctionBinding* bindings[]{&subequipment::grantBinding,&subequipment::deleteBinding};
    for(auto binding:bindings)if(!subequipment::Live(binding->function,binding->identity)
        ||Read<subequipment::ExecFn>(binding->function,0xe0)!=binding->replacement)return false;
    return true;
}
inline bool Subsystems(){
    const auto& factory=spawn::GetState();if(factory.error)return Fail(factory.error,factory.stage);
    if(state.initialized&&!state.closing&&!FactoryOwned())
        return Fail(6971,"hero_authority_original_factory_bindings_changed_after_install");
    const auto& iso=arena::GetState();if(iso.error)return Fail(iso.error,iso.stage);
    if(state.subequipmentOwned){const auto& child=subequipment::GetState();
        if(child.error)return Fail(child.error,child.stage);
        if(!state.closing&&!SubequipmentBindingsOwned())return Fail(6972,"hero_authority_subequipment_original_bindings_changed_after_install");}
    const auto& loadout=framework::GetState();
    if(loadout.initialized&&loadout.host&&loadout.world==state.world&&loadout.error)return Fail(loadout.error,loadout.stage);
    return true;
}
inline bool InitHostImpl(void* world){
    if(state.closed||state.closing)return Fail(6951,"hero_authority_requires_new_pair_after_close");
    if(state.initialized)return Thread()&&world==state.world&&Live(world,state.worldId)&&Subsystems();
    if(!engine::image||!original_players::Thread()||!spawn::HostWorld(world))
        return Fail(6952,"hero_authority_requires_actual_original_host_world");
    state.gameThread=GetCurrentThreadId();state.world=world;
    if(!Capture(world,state.worldId))return Fail(6953,"hero_authority_original_host_world_identity_unavailable");
    spawn::Hooks callbacks{};callbacks.resolve=ResolveSpawn;callbacks.validate=ValidateSpawn;
    // prepare/admit deliberately remain null. No guessed tuning/FXC/class or
    // replacement ownership contract is introduced by this coordination layer.
    if(!spawn::Install(world,callbacks)){
        const auto& s=spawn::GetState();return Fail(s.error?s.error:6954,s.error?s.stage:"hero_authority_factory_hooks_owned_by_other_layer");}
    state.initialized=state.hostContextVerified=state.spawnInstalled=true;
    state.stage="hero_authority_partial_adapters_installed_waiting_session_policy";Cache();return true;
}
inline void PruneProofs(){
    if(spawn::GetState().active)return;
    for(auto& p:proofs)if(p.used){View after{};
        if(!RegisteredCurrent(p.view,after)||Retiring(p.view)){p={};++state.proofPruned;if(state.cachedProofs)--state.cachedProofs;}}
    for(auto& r:retiring)if(r.used){
        bool registered=false;
        for(const auto& p:framework::GetPlayers())if(p.used&&!p.retired&&p.authority&&p.registration==r.registration
            &&p.epoch==r.epoch&&p.context.pawn==r.pawn&&original_inventory_slots::SameIdentity(p.context.pawnId,r.identity))registered=true;
        if(!registered){r={};if(state.retiringEpochs)--state.retiringEpochs;}}
}
inline bool MaintainSubequipment(){
    if(!state.subequipmentOwned||!state.subequipmentInitialized||subequipment::GetState().closed)return true;
    if(!SubequipmentBindingsOwned())return Fail(6972,"hero_authority_subequipment_original_bindings_changed_after_install");
    // New-grant guard is already closed by the parent's current policy/fence.
    // Keep real queued delete/Unequip/reference-history retirement progressing;
    // Raw ownership proof is independent of permission to create a new child.
    if(!subequipment::Poll()){const auto& child=subequipment::GetState();return Fail(child.error?child.error:6975,child.error?child.stage:"hero_authority_owned_subequipment_maintenance_failed");}
    ++state.subequipmentMaintenancePolls;return true;
}
inline bool PollSubequipment(const View& human){
    state.subequipmentActive=false;
    if(human.heroIndex==0||!Policy(human))return MaintainSubequipment();
    bool required=false;
    if(!framework::VisitRegisteredComponents([&](const View& registered,void* component){
        if(registered.pawn==human.pawn&&registered.sourceEpoch==human.sourceEpoch
           &&registered.registration==human.registration&&registered.heroIndex==human.heroIndex
           &&Read<void*>(component,0x10)==engine::Class(subequipment::ComponentClass)
           &&ValidateHostSource(registered))required=true;
        return true;
    })){const auto& parent=framework::GetState();return Fail(parent.error?parent.error:6974,parent.stage);}
    // Heroes without an authored native child-firearm component need no grant
    // adapter. Previously owned children still complete their native cleanup.
    if(!required)return MaintainSubequipment();
    if(!state.subequipmentInitialized){
        const bool initialized=subequipment::Init(state.world,true,AllowSubequipmentSource);
        const auto& child=subequipment::GetState();
        // Remember partial ownership as well, so an Init failure cannot leave
        // an installed grant exec outside the parent's later Close cleanup.
        state.subequipmentOwned=child.world==state.world&&child.host&&subequipment::sourceAllowed==AllowSubequipmentSource;
        if(!initialized)return Fail(child.error?child.error:6973,child.error?child.stage:"hero_authority_subequipment_init_owned_by_other_layer");
        state.subequipmentInitialized=true;
    }
    if(!SubequipmentBindingsOwned())return Fail(6972,"hero_authority_subequipment_original_bindings_changed_after_install");
    bool registrationOkay=true;
    const bool discovered=framework::VisitRegisteredComponents([&](const View& registered,void* component){
        if(registered.pawn!=human.pawn||registered.sourceEpoch!=human.sourceEpoch||registered.registration!=human.registration
           ||registered.heroIndex!=human.heroIndex||!ValidateHostSource(registered))return true;
        if(Read<void*>(component,0x10)!=engine::Class(subequipment::ComponentClass))return true;
        View actual{};
        if(!ResolveHostSource(component,actual)||!AllowSubequipmentSource(actual))return true;
        const bool existed=subequipment::Find(component)!=nullptr;
        if(!subequipment::Register(actual.world,actual.controller,actual.playerState,actual.pawn,actual.sourceEpoch,actual.ability,component)){
            registrationOkay=false;return false;}
        if(!existed&&subequipment::Find(component))++state.subequipmentRegistrations;
        return true;
    });
    if(!discovered||!registrationOkay){
        const auto& child=subequipment::GetState();const auto& parent=framework::GetState();
        return Fail(child.error?child.error:(parent.error?parent.error:6974),child.error?child.stage:(parent.error?parent.stage:"hero_authority_registered_subequipment_discovery_failed"));}
    // The real native initial request compensates for a grant originally
    // missed before component discovery. Poll preserves true channel/history
    // waits; no empty grant or fabricated RPC/readiness is supplied here.
    if(!subequipment::Poll()){const auto& child=subequipment::GetState();return Fail(child.error?child.error:6975,child.error?child.stage:"hero_authority_original_subequipment_poll_failed");}
    ++state.subequipmentPolls;state.subequipmentActive=true;return true;
}
inline bool CaptureArenaActors(){
    const auto& s=arena::GetState();
    if(!s.initialized||!s.sourceVerified||!s.originalDedicated||!s.nativeInitializationReady)return true;
    const auto& source=arena::source;
    if(s.world!=state.world||s.pawn!=source.pawn||s.epoch!=source.sourceEpoch||!ValidateHostSource(source))
        return Fail(6955,"hero_authority_original_arena_registration_source_changed");
    for(const auto& observed:s.arenas){if(!observed.actor)continue;
        if(!observed.live||!observed.originalClass||!observed.ownerMatches||!observed.worldMatches||!observed.begun
           ||!Live(observed.actor,observed.identity)||Read<uint8_t>(observed.actor,0x224)!=3
           ||spawn::ActorWorld(observed.actor)!=state.world||Read<void*>(observed.actor,0x210)!=s.gameState)
            return Fail(6956,"hero_authority_original_arena_registration_receipt_invalid");
        ArenaActor* available=nullptr;bool found=false;
        for(auto& e:arenaActors){
            if(e.used&&!Live(e.actor,e.identity)){e={};++state.arenaActorsPruned;if(state.arenaActors)--state.arenaActors;}
            if(!e.used){if(!available)available=&e;continue;}
            if(e.actor==observed.actor){
                if(!original_inventory_slots::SameIdentity(e.identity,observed.identity))return Fail(6957,"hero_authority_original_arena_identity_changed");
                found=true;break;}}
        if(found)continue;
        if(!available)return Fail(6958,"hero_authority_original_arena_registry_capacity_exceeded");
        ArenaActor entry{};entry.actor=observed.actor;entry.identity=observed.identity;entry.world=state.world;entry.worldId=state.worldId;
        entry.owner=s.gameState;entry.type=Read<void*>(observed.actor,0x10);entry.sourceEpoch=source.sourceEpoch;entry.sourceRegistration=source.registration;
        if(!Capture(entry.owner,entry.ownerId)||!Capture(entry.type,entry.typeId)||!arena::Live(arena::arenaType,arena::arenaTypeId)
           ||!original_players::Derived(entry.type,arena::arenaType))return Fail(6959,"hero_authority_original_arena_owner_or_class_identity_missing");
        entry.originalReplicates=spawn::Replicates(entry.actor);entry.used=true;*available=entry;
        ++state.arenaActors;++state.arenaActorRegistrations;
    }
    return true;
}
inline bool PollHostImpl(void* actualHumanPawn,bool skillsEnabled,bool deathmatch){
    if(!state.initialized||!Thread()||state.closed||state.closing||state.restartRequired)
        return Fail(6960,"hero_authority_poll_requires_initialized_original_host_thread");
    if(!Subsystems())return false;
    if(!Live(state.world,state.worldId)||!spawn::HostWorld(state.world))
        return Fail(6969,"hero_authority_original_host_world_or_driver_retired");
    ++state.polls;state.policyKnown=true;state.skillsEnabled=skillsEnabled;state.deathmatch=deathmatch;
    state.sourceGate=false;state.sourceVerified=false;state.heroIndex=UINT32_MAX;state.sourceEpoch=0;state.sourceRegistration=0;
    state.arenaPreflightActive=false;
    state.subequipmentActive=false;
    state.policyPawn=nullptr;state.policyPawnId={};PruneProofs();
    if(!skillsEnabled||deathmatch){
        ++state.disabledPolls;
        if(!MaintainSubequipment())return false;
        if(!arena::Poll(actualHumanPawn,false,deathmatch))return Subsystems();
        state.arenaPreflightActive=false;return Wait("hero_authority_new_factories_disabled_by_original_session_policy");}
    auto pc=Read<void*>(actualHumanPawn,0x4a0),ps=Read<void*>(actualHumanPawn,0x488),inventory=Read<void*>(actualHumanPawn,0xc08);
    void* connection=nullptr;Weak identity{};
    if(!actualHumanPawn||!Capture(actualHumanPawn,identity)
       ||!local_server_item_ownership::ActualHuman(state.world,pc,ps,actualHumanPawn,inventory,connection)){
        if(!MaintainSubequipment())return false;
        // Null has no registered source. The public Arena path marks its cached
        // observation inactive before any native predicate/type/manager query.
        if(!arena::Poll(nullptr,true,false))return Subsystems();
        return Wait("hero_authority_waiting_actual_admitted_human_pawn");
    }
    state.policyPawn=actualHumanPawn;state.policyPawnId=identity;state.sourceGate=true;
    View view{};
    if(!ResolveHostSource(actualHumanPawn,view)){
        if(state.error)return false;
        if(!MaintainSubequipment())return false;
        if(!arena::Poll(nullptr,true,false))return Subsystems();
        return Wait("hero_authority_waiting_registered_original_human_skill_source");}
    if(view.contextObject!=actualHumanPawn||view.ability||view.pendingGrant)
        return Fail(6961,"hero_authority_pawn_context_must_be_original_registered_source");
    state.sourceVerified=true;state.heroIndex=view.heroIndex;state.sourceEpoch=view.sourceEpoch;state.sourceRegistration=view.registration;
    if(!PollSubequipment(view))return false;
    if(!arena::Poll(actualHumanPawn,skillsEnabled,deathmatch)||!Subsystems()||!CaptureArenaActors())return false;
    state.arenaPreflightActive=arena::GetState().active;
    state.stage="hero_authority_partial_factory_sources_active_other_authority_unverified";Cache();return true;
}
inline bool RetirePlayerImpl(void* pawn,uint32_t epoch){
    if(!state.initialized||state.closed)return true;
    if(!Thread()||!pawn||!epoch)return Fail(6962,"hero_authority_retirement_requires_original_thread_and_exact_epoch");
    RetiringEpoch token{};
    for(const auto& p:framework::GetPlayers())if(p.used&&!p.retired&&p.authority&&p.context.pawn==pawn&&p.epoch==epoch&&p.registration){
        token={pawn,p.context.pawnId,epoch,p.registration,true};break;}
    if(token.used){
        RetiringEpoch* available=nullptr;bool found=false;
        for(auto& r:retiring){
            if(!r.used){if(!available)available=&r;continue;}
            if(r.pawn==token.pawn&&r.epoch==token.epoch&&r.registration==token.registration
               &&original_inventory_slots::SameIdentity(r.identity,token.identity)){found=true;break;}}
        if(!found){if(!available)return Fail(6963,"hero_authority_retirement_fence_capacity_exceeded");
            *available=token;++state.retiringEpochs;}
    }
    if(state.policyPawn==pawn&&((token.used&&original_inventory_slots::SameIdentity(state.policyPawnId,token.identity))
       ||state.sourceEpoch==epoch)){
        state.sourceGate=false;state.sourceVerified=false;state.sourceEpoch=0;state.sourceRegistration=0;}
    // The admission fence precedes optional producer retirement and framework
    // resource-cache retirement. Committed native deployables are not destroyed.
    if(state.subequipmentOwned){
        if(!subequipment::RetirePlayer(pawn,epoch)){const auto& child=subequipment::GetState();
            return Fail(child.error?child.error:6976,child.error?child.stage:"hero_authority_owned_subequipment_retirement_pending_or_failed");}
        ++state.subequipmentRetirements;state.subequipmentActive=false;
    }
    if(lifecycle.retirePlayer&&!lifecycle.retirePlayer(pawn,epoch))return Fail(6964,"hero_authority_composed_producer_retirement_failed");
    const auto& iso=arena::GetState();
    if(iso.initialized&&iso.pawn==pawn&&iso.epoch==epoch&&!arena::Close())return Fail(arena::GetState().error,arena::GetState().stage);
    ++state.retirements;state.arenaPreflightActive=arena::GetState().active;
    state.stage="hero_authority_epoch_fenced_existing_native_actors_keep_original_lifetime";Cache();return true;
}
inline bool CloseImpl(){
    if(state.closed)return true;
    bool published=false;for(const auto& function:spawn::functions)if(function.installed)published=true;
    if(!state.initialized&&!published&&!state.subequipmentOwned)return true;
    if(!Thread())return Fail(6965,"hero_authority_close_requires_original_game_thread");
    state.closing=true;state.sourceGate=false;state.sourceVerified=false;
    const bool factoryOwnership=spawn::hooks.resolve==ResolveSpawn&&spawn::hooks.validate==ValidateSpawn&&spawn::GetState().world==state.world;
    const bool childOwnership=!state.subequipmentOwned
        ||(subequipment::GetState().world==state.world&&subequipment::sourceAllowed==AllowSubequipmentSource);
    bool childrenClosed=true;
    if(state.subequipmentOwned)childrenClosed=childOwnership&&subequipment::Close();
    // Restore each independent owned layer even if a sibling has a conflict.
    // No failed layer is marked closed or replaced with a success receipt.
    const bool factoryClosed=factoryOwnership&&spawn::Close();if(factoryClosed)state.spawnInstalled=false;
    const bool extensionClosed=!lifecycle.close||lifecycle.close();
    const bool arenaClosed=arena::Close();
    if(!childOwnership)return Fail(6977,"hero_authority_close_cannot_restore_another_subequipment_layer");
    if(!factoryOwnership)return Fail(6970,"hero_authority_close_cannot_restore_another_factory_layer");
    if(!childrenClosed){const auto& child=subequipment::GetState();return Fail(child.error?child.error:6978,child.error?child.stage:"hero_authority_subequipment_close_failed");}
    if(!factoryClosed)return Fail(spawn::GetState().error,spawn::GetState().stage);
    if(!extensionClosed)return Fail(6966,"hero_authority_composed_producer_close_failed");
    if(!arenaClosed)return Fail(arena::GetState().error,arena::GetState().stage);
    proofs={};retiring={};arenaActors={};state.cachedProofs=state.retiringEpochs=state.arenaActors=0;
    state.policyPawn=nullptr;state.policyPawnId={};state.arenaPreflightActive=false;
    state.subequipmentActive=false;state.subequipmentInitialized=false;
    state.initialized=false;state.closed=true;state.closing=false;++state.closes;
    state.stage="hero_authority_closed_original_execs_restored_native_world_owns_actors";Cache();return true;
}
inline bool InitHost(void* world){
    __try{const auto result=InitHostImpl(world);Cache();return result;}
    __except(Exception(GetExceptionInformation())){return false;}
}
inline bool PollHost(void* actualHumanPawn,bool skillsEnabled,bool deathmatch){
    __try{const auto result=PollHostImpl(actualHumanPawn,skillsEnabled,deathmatch);Cache();return result;}
    __except(Exception(GetExceptionInformation())){return false;}
}
inline bool RetirePlayer(void* pawn,uint32_t epoch){
    __try{const auto result=RetirePlayerImpl(pawn,epoch);Cache();return result;}
    __except(Exception(GetExceptionInformation())){return false;}
}
inline bool Close(){
    __try{const auto result=CloseImpl();Cache();return result;}
    __except(Exception(GetExceptionInformation())){return false;}
}
template<class Visit> inline bool VisitReplicationGraph(Visit&& visit){
    if(state.closed)return true;
    if(!state.initialized)return !state.error;
    if(!Thread()||!Live(state.world,state.worldId)||!Subsystems())return false;
    ++state.registryVisits;
    // This existing registry already owns exact actor Weak identities, source-
    // proven factory receipts, same-world Role3 and the real reflected native
    // bReplicates check. Source death is not a second actor-destruction policy.
    if(!spawn::VisitReplicationActors([&](void* actor){++state.spawnActorVisits;visit(actor,actor);}))
        return Fail(spawn::GetState().error?spawn::GetState().error:6967,spawn::GetState().error?spawn::GetState().stage:"hero_authority_existing_spawn_actor_registry_failed");
    if(state.subequipmentOwned){
        if(!subequipment::VisitReplicationGraph([&](void* root,void* object){
            // The subequipment registry supplies exact original actor/slot/
            // component membership. Preserve those roots and native subobject
            // relationships; only an actual native replicating Actor is sent.
            if(!root||Read<uint8_t>(root,0x224)!=3||spawn::ActorWorld(root)!=state.world||!spawn::Replicates(root))return;
            ++state.subequipmentGraphVisits;visit(root,object);
        })){const auto& child=subequipment::GetState();return Fail(child.error?child.error:6979,child.error?child.stage:"hero_authority_subequipment_replication_graph_failed");}
    }
    for(auto& entry:arenaActors){if(!entry.used)continue;
        if(!Live(entry.actor,entry.identity)){entry={};++state.arenaActorsPruned;if(state.arenaActors)--state.arenaActors;continue;}
        if(!Live(entry.world,entry.worldId)||entry.world!=state.world||!Live(entry.owner,entry.ownerId)||!Live(entry.type,entry.typeId)
           ||Read<void*>(entry.actor,0x10)!=entry.type||Read<void*>(entry.actor,0x210)!=entry.owner
           ||Read<uint8_t>(entry.actor,0x224)!=3||spawn::ActorWorld(entry.actor)!=state.world)
            return Fail(6968,"hero_authority_existing_original_arena_actor_contract_changed");
        if(!spawn::Replicates(entry.actor)){++state.nativeNonreplicated;continue;}
        ++state.arenaActorVisits;visit(entry.actor,entry.actor);
    }
    Cache();return true;
}
inline void Write(FILE* file){
    if(!file)return;const auto s=Snapshot();const auto yes=[](bool value){return value?"true":"false";};
    std::fprintf(file,"{\"stage\":");original_players::Text(file,s.stage);
    std::fprintf(file,",\"subequipment_owned\":%s,\"subequipment_initialized\":%s,\"subequipment_active\":%s,\"subequipment_registrations\":%llu,\"subequipment_polls\":%llu,\"subequipment_maintenance_polls\":%llu,\"subequipment_graph_visits\":%llu,\"subequipment_retirements\":%llu",
        yes(s.subequipmentOwned),yes(s.subequipmentInitialized),yes(s.subequipmentActive),static_cast<unsigned long long>(s.subequipmentRegistrations),static_cast<unsigned long long>(s.subequipmentPolls),static_cast<unsigned long long>(s.subequipmentMaintenancePolls),static_cast<unsigned long long>(s.subequipmentGraphVisits),static_cast<unsigned long long>(s.subequipmentRetirements));
    std::fprintf(file,",\"error\":%u,\"initialized\":%s,\"closed\":%s,\"closing\":%s,\"restart_required\":%s,\"host_context_verified\":%s,\"policy_known\":%s,\"skills_enabled\":%s,\"deathmatch\":%s,\"source_gate\":%s,\"source_verified\":%s,\"spawn_installed\":%s,\"arena_preflight_active\":%s,\"world\":\"%p\",\"policy_pawn\":\"%p\",\"hero_index\":%u,\"source_epoch\":%u,\"source_registration\":%llu,\"cached_proofs\":%u,\"retiring_epochs\":%u,\"arena_actors\":%u,\"polls\":%llu,\"source_queries\":%llu,\"source_accepted\":%llu,\"source_rejected\":%llu,\"source_validations\":%llu,\"weak_proof_deferrals\":%llu,\"proof_pruned\":%llu,\"retirements\":%llu,\"closes\":%llu,\"registry_visits\":%llu,\"spawn_actor_visits\":%llu,\"arena_actor_visits\":%llu,\"arena_actor_registrations\":%llu,\"native_nonreplicated\":%llu,\"arena_actors_pruned\":%llu,\"disabled_polls\":%llu,\"exception_rva\":\"0x%llx\",\"support\":{\"factory_source_count\":2,\"factory_sources\":[\"Veto_E\",\"Hush_C\"],\"iso_x\":\"original_preflight_only\",\"all_hero_authority_supported\":false},\"network_execution_verified\":false,\"all_hero_skills_verified\":false,\"playable_verified\":false,\"client_factory\":false,\"adapter_role_or_locality_writes\":0,\"adapter_replication_flag_writes\":0,\"retirement_destroys_deployables\":false,\"spawn\":",
        s.error,yes(s.initialized),yes(s.closed),yes(s.closing),yes(s.restartRequired),yes(s.hostContextVerified),yes(s.policyKnown),yes(s.skillsEnabled),yes(s.deathmatch),yes(s.sourceGate),yes(s.sourceVerified),yes(s.spawnInstalled),yes(s.arenaPreflightActive),
        s.world,s.policyPawn,s.heroIndex,s.sourceEpoch,static_cast<unsigned long long>(s.sourceRegistration),s.cachedProofs,s.retiringEpochs,s.arenaActors,
        static_cast<unsigned long long>(s.polls),static_cast<unsigned long long>(s.sourceQueries),static_cast<unsigned long long>(s.sourceAccepted),static_cast<unsigned long long>(s.sourceRejected),static_cast<unsigned long long>(s.sourceValidations),
        static_cast<unsigned long long>(s.weakProofDeferrals),static_cast<unsigned long long>(s.proofPruned),static_cast<unsigned long long>(s.retirements),static_cast<unsigned long long>(s.closes),static_cast<unsigned long long>(s.registryVisits),
        static_cast<unsigned long long>(s.spawnActorVisits),static_cast<unsigned long long>(s.arenaActorVisits),static_cast<unsigned long long>(s.arenaActorRegistrations),static_cast<unsigned long long>(s.nativeNonreplicated),static_cast<unsigned long long>(s.arenaActorsPruned),static_cast<unsigned long long>(s.disabledPolls),static_cast<unsigned long long>(s.exceptionRva));
    spawn::Write(file);std::fputs(",\"arena\":",file);arena::Write(file);std::fputs(",\"subequipment\":",file);subequipment::Write(file);std::fputc('}',file);
}
} // namespace local_server_hero_authority
