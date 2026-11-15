#pragma once
#include "local_server_hero_authority.h"
#include "local_server_ability_damage.h"
#include <array>
#include <cstdio>

// Host-only compositor. Root owns lifecycle order and enables this explicitly;
// including this file alone does not publish observers or a native producer.
// No match rule, original image, Role, input clock or client life is rewritten.
namespace local_server_ability_damage_integration {
namespace authority=local_server_hero_authority;
namespace damage=local_server_ability_damage;
namespace combat=local_server_combat;
namespace spawn=local_server_ability_spawn;
using View=authority::View;
using Weak=original_inventory_slots::WeakIdentity;
using Contact=damage::Contact;
using Result=original_damage::Result;
inline constexpr size_t MaxPlayers=original_players::MaxPlayers;
struct Settings {
    bool enableImmediate=false;
    // Additional Root scope gate. It MUST be read-only and may reject any
    // activity owned outside this compositor (for example world retirement).
    bool (*immediateBoundary)()=nullptr;
};
struct PendingDeath {
    bool used=false;uint32_t epoch=0;uint64_t token=0;void* pawn=nullptr;Weak identity{};
};
struct State {
    bool initialized=false,closing=false,closed=false,committing=false,flushing=false,immediateEnabled=false;
    bool restartRequired=false,damageOwned=false;void* world=nullptr;Weak worldId{};DWORD gameThread=0;unsigned error=0;
    uint64_t sourceQueries=0,directSources=0,spawnSources=0,unresolvedSources=0;
    uint64_t pendingGrantRejected=0,nonHumanRejected=0,sourceRetired=0,staleVictims=0;
    uint64_t selfDamageUnsupported=0,policyRejected=0,typedReferenceUnsupported=0;
    uint64_t commits=0,applied=0,kills=0,deferredDeaths=0,deathCleanups=0,retirementWaits=0;
    uint64_t immediateDeferrals=0,immediateCommits=0,queueBoundaryDeferrals=0;
    uint64_t exceptionRva=0;const char* stage="ability_damage_compositor_not_initialized";
};
inline State state{},cached{};inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline Settings settings{};inline std::array<PendingDeath,MaxPlayers> deaths{};
inline const State& GetState(){return state;}
inline void Cache(){AcquireSRWLockExclusive(&cacheLock);cached=state;ReleaseSRWLockExclusive(&cacheLock);}
inline State Snapshot(){AcquireSRWLockShared(&cacheLock);auto result=cached;ReleaseSRWLockShared(&cacheLock);return result;}
template<class T> inline T Read(const void* p,size_t at=0){return original_players::Value<T>(p,at);}
inline bool Live(void* p,const Weak& id){return p&&original_inventory_slots::ResolveIdentity(id)==p;}
inline bool Thread(){return engine::image&&state.gameThread==GetCurrentThreadId()&&original_players::Thread();}
inline bool CleanupThread(){return engine::image&&state.gameThread==GetCurrentThreadId()
    &&original_players::gameThread==state.gameThread;}
inline bool Fail(unsigned error,const char* stage){state.error=error?error:6399;state.stage=stage;Cache();return false;}
inline LONG Exception(EXCEPTION_POINTERS* details){
    const auto at=details&&details->ExceptionRecord?reinterpret_cast<uintptr_t>(details->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;state.restartRequired=true;
    Fail(6390,"ability_damage_compositor_exception_requires_new_owned_pair");return EXCEPTION_EXECUTE_HANDLER;
}
inline bool PublishedSource(const View& view,size_t& index){
    index=MaxPlayers;
    if(view.heroIndex==0||view.heroIndex>=original_heroes::PlayableHeroCount)return false;
    if(view.pendingGrant){++state.pendingGrantRejected;return false;}
    if(!view.ability||view.world!=state.world||!Live(state.world,state.worldId)
       ||!authority::ValidateHostSource(view))return false;
    for(size_t n=0;n<MaxPlayers;++n){const auto p=local_server_match::Player(n);
        if(!p||!p->present||p->pawn!=view.pawn||p->pc!=view.controller||p->ps!=view.playerState
           ||p->epoch!=view.sourceEpoch||p->hero!=view.heroIndex)continue;
        if(!p->human){++state.nonHumanRejected;return false;}
        if(!combat::CurrentRegistration(n)||!combat::policy::Authority(n,p->pawn,p->pc))return false;
        const auto& r=combat::registrations[n];
        if(!original_inventory_slots::SameIdentity(view.identities[2],r.pcId)
           ||!original_inventory_slots::SameIdentity(view.identities[3],r.psId)
           ||!original_inventory_slots::SameIdentity(view.identities[4],r.pawnId))return false;
        index=n;return true;
    }
    return false;
}
inline bool ResolveSourceProof(void* causer,View& out,uint64_t& receipt){
    out={};receipt=0;
    if(!state.initialized||state.closing||state.closed||state.error||state.restartRequired||!Thread()||!causer)return false;
    ++state.sourceQueries;View direct{};size_t index=MaxPlayers;
    if(authority::ResolveHostSource(causer,direct)&&PublishedSource(direct,index)){
        out=direct;++state.directSources;return true;
    }
    // Resolve only the exact private native factory receipt, not Actor.Owner.
    // The child context is never passed off as a framework ability context.
    spawn::OwnedChildSource child{};
    if(spawn::ResolveOwnedChildSource(causer,child)&&child.receipt&&child.actor==causer
       &&child.source.contextObject==causer&&!child.source.pendingGrant
       &&Live(child.actor,child.actorIdentity)
       &&original_inventory_slots::SameIdentity(child.identities[0],child.actorIdentity)
       &&spawn::ValidateOwnedChildSource(child)){
        View current{};const auto& saved=child.source;
        if(authority::ResolveHostSource(saved.ability,current)&&current.contextObject==saved.ability
           &&current.world==saved.world&&current.controller==saved.controller&&current.playerState==saved.playerState
           &&current.pawn==saved.pawn&&current.inventory==saved.inventory&&current.ability==saved.ability
           &&current.heroIndex==saved.heroIndex&&current.sourceEpoch==saved.sourceEpoch
           &&current.registration==saved.registration&&current.pendingGrant==saved.pendingGrant){
            bool same=true;
            for(size_t n=1;n<current.identities.size();++n)
                same=same&&original_inventory_slots::SameIdentity(current.identities[n],child.identities[n]);
            if(same&&PublishedSource(current,index)&&spawn::ValidateOwnedChildSource(child)){
                out=current;receipt=child.receipt;++state.spawnSources;return true;
            }
        }
    }
    ++state.unresolvedSources;return false;
}
inline bool ResolveSource(void* causer,View& out){
    uint64_t receipt=0;
    __try{return ResolveSourceProof(causer,out,receipt);}__except(Exception(GetExceptionInformation())){out={};return false;}
}
inline bool CaptureSourceReceipt(void* causer,const View& expected,uint64_t& receipt){
    View current{};receipt=0;
    __try{return ResolveSourceProof(causer,current,receipt)&&damage::SameSource(expected,current);}
    __except(Exception(GetExceptionInformation())){receipt=0;return false;}
}
inline bool VictimReady(size_t index){
    if(!Thread()||index>=MaxPlayers||!combat::CurrentRegistration(index))return false;
    const auto p=local_server_match::Player(index);
    if(!p||!p->present||!p->ready||!p->alive)return false;
    if(p->human){const auto& input=local_server_input::GetState();
        return input.installed&&!input.epochRetired&&!input.error&&input.pawn==p->pawn;
    }
    // The lower source owns the native view table (or a genuine non-ballistic
    // context). Its retirement hook removes this upper layer on a gun switch.
    const auto& s=combat::firearms::sources[index];
    return s.bound&&s.pawn==p->pawn&&s.spawnEpoch==p->epoch&&combat::firearms::SourceIdentity(s);
}
inline bool CommonBoundary(){
    return state.initialized&&!state.closing&&!state.closed&&!state.error&&!state.restartRequired
        &&!state.committing&&!state.flushing&&Thread()
        &&combat::AbilityDamageCommitBoundary();
}
inline bool QueueBoundary(){
    const auto& input=local_server_input::GetState();
    const bool ready=CommonBoundary()&&!input.inPoll&&!input.inReceive;
    if(!ready)++state.queueBoundaryDeferrals;return ready;
}
inline bool RequestRetireIndex(size_t index,void* pawn,uint32_t epoch){
    if(index>=MaxPlayers||!CleanupThread())return false;
    const auto& binding=damage::bindings[index];
    if(binding.used&&(binding.pawn!=pawn||binding.epoch!=epoch))
        return Fail(6371,"ability_damage_retirement_tuple_conflict");
    if(!binding.used)return true;
    if(!damage::Retire(index))return Fail(damage::GetState().error,damage::GetState().stage);
    if(damage::bindings[index].used){++state.retirementWaits;
        state.stage="ability_damage_retirement_waiting_outermost_native_return";Cache();return false;
    }
    return true;
}
// True means the actual upper binding is gone, not merely a queued request.
// Root MUST gate lower input/coordinator retirement on this completion result.
inline bool FlushRetirements();
inline bool ReadyForPlayerRetirement(void* pawn,uint32_t epoch){
    if(!state.initialized)return true;if(!pawn||!epoch||!CleanupThread())return false;
    if(state.committing||state.flushing||damage::WrapperCount()||damage::depth){
        ++state.retirementWaits;state.stage="ability_damage_player_retirement_waiting_safe_native_boundary";Cache();return false;
    }
    const auto& input=local_server_input::GetState();
    if(input.pawn==pawn&&(input.inPoll||input.inReceive)){
        ++state.retirementWaits;state.stage="ability_damage_human_retirement_waiting_native_input_return";Cache();return false;
    }
    for(size_t n=0;n<MaxPlayers;++n){const auto& binding=damage::bindings[n];
        if(binding.used&&binding.pawn==pawn&&!RequestRetireIndex(n,pawn,epoch))return false;
    }
    // Complete this old life before Root can reuse its roster/source slot.
    // Otherwise a later pending death cleanup could target a fresh respawn.
    if(!FlushRetirements())return false;
    for(const auto& pending:deaths)if(pending.used&&pending.pawn==pawn){
        ++state.retirementWaits;return false;
    }
    return true;
}
inline bool RetirePlayer(void* pawn,uint32_t epoch){
    __try{return ReadyForPlayerRetirement(pawn,epoch);}__except(Exception(GetExceptionInformation())){return false;}
}
inline bool BeforeNPCPawnRetire(size_t index,void* pawn,uint32_t epoch){
    if(!state.initialized)return true;
    __try{return RequestRetireIndex(index,pawn,epoch);}__except(Exception(GetExceptionInformation())){return false;}
}
inline bool FlushRetirementsImpl(){
    if(!state.initialized)return true;
    if(!CleanupThread())return false;
    if(damage::WrapperCount()||damage::depth||state.committing||state.flushing)return true;
    if(damage::GetState().initialized&&damage::HasDeferredRetirements()&&!damage::FlushDeferredRetirements())
        return Fail(damage::GetState().error,damage::GetState().stage);
    state.flushing=true;bool okay=true;
    __try{
        for(size_t n=0;n<deaths.size();++n){auto& pending=deaths[n];if(!pending.used)continue;
            if(!RequestRetireIndex(n,pending.pawn,pending.epoch)){okay=state.error==0;break;}
            // Never apply old-epoch cleanup to a newly registered source.
            const auto& r=combat::registrations[n];const auto& s=combat::firearms::sources[n];
            const auto& m=combat::melee::sources[n];
            if((r.present&&(r.pawn!=pending.pawn||r.epoch!=pending.epoch))
               ||(s.bound&&(s.pawn!=pending.pawn||s.spawnEpoch!=pending.epoch))
               ||(m.bound&&(m.pawn!=pending.pawn||m.spawnEpoch!=pending.epoch))){
                okay=Fail(6372,"ability_damage_death_cleanup_epoch_changed_before_original_observers_retired");break;
            }
            if(!combat::BeforeNativeDeath(n)){okay=Fail(combat::GetState().error?combat::GetState().error:6373,
                "ability_damage_native_death_lower_observer_retirement_failed");break;}
            pending={};++state.deathCleanups;
        }
    }__finally{state.flushing=false;}
    Cache();return okay;
}
// Root calls this early, before PreprocessDeaths, NPC weapon changes and the
// ordinary combat Poll; ordinary damage Poll runs after those older queues.
inline bool FlushRetirements(){
    __try{return FlushRetirementsImpl();}__except(Exception(GetExceptionInformation())){return false;}
}
inline bool Context(const Contact& c,combat::NativeDamageContext& out){
    out={};size_t source=MaxPlayers;View current{};
    if(!local_server_ability_damage_integration::ResolveSource(c.causer,current)
       ||!damage::SameSource(c.source,current)||!PublishedSource(current,source)){
        ++state.sourceRetired;return false;
    }
    if(c.victim>=MaxPlayers||!damage::CurrentVictim(c)||!combat::melee::SameLife(c.before)){
        ++state.staleVictims;return false;
    }
    if(source==c.victim){++state.selfDamageUnsupported;
        state.stage="ability_damage_self_damage_suicide_policy_unsupported";return false;
    }
    if(c.instigator!=current.controller||!combat::policy::AbilitiesAllowed()||!combat::policy::CanDamage(source,c.victim)){
        ++state.policyRejected;return false;
    }
    out.source=source;out.victim=c.victim;out.sourceEpoch=current.sourceEpoch;out.victimEpoch=c.victimEpoch;
    out.sourcePawn=current.pawn;out.controller=current.controller;out.playerState=current.playerState;
    out.equippable=current.ability;out.causer=c.causer;out.damageType=c.damageType;out.victimPawn=c.victimPawn;
    out.sourceId=current.identities[4];out.controllerId=current.identities[2];out.playerStateId=current.identities[3];
    out.equippableId=current.identities[6];out.causerId=c.causerId;out.damageTypeId=c.damageTypeId;out.victimId=c.victimId;
    return true;
}
inline bool CommitImpl(const Contact& c,double now,Result& result){
    result={};
    if(!state.initialized||state.closing||state.closed||state.error||state.restartRequired||state.committing||!Thread())return false;
    combat::NativeDamageContext context{};
    if(!Context(c,context))return !state.error&&!state.restartRequired;
    if(!combat::PreflightAuthorityNativeEvent(context,c.geometry)){
        ++state.typedReferenceUnsupported;state.stage="ability_damage_real_typed_reference_or_schema_not_supported";Cache();
        return !original_damage::State().error&&!combat::GetState().error;
    }
    if(!damage::CurrentSource(c)||!damage::CurrentVictim(c)||!combat::melee::SameLife(c.before))return true;
    original_damage::DeathData death{};
    if(!combat::policy::BeginNativeDamage(c.token,context.source,c.victim,context.equippable,now,death)){
        if(!combat::policy::CanDamage(context.source,c.victim)){++state.policyRejected;return true;}
        return Fail(local_server_match::GetState().error?local_server_match::GetState().error:6374,
            "ability_damage_original_match_transaction_begin_failed");
    }
    state.committing=true;bool okay=false;
    __try{
        const bool applied=combat::ApplyAuthorityNativeEvent(context,c.geometry,c.accepted,c.token,result,&death);
        const bool committed=combat::policy::EndNativeDamage(applied,result);
        okay=applied&&committed;
        if(!okay)Fail(!applied?original_damage::State().error:local_server_match::GetState().error,
            "ability_damage_original_typed_notification_or_match_receipt_failed");
        else{
            ++state.commits;
            if(result.applied&&!result.duplicate){++state.applied;
                if(result.killed){++state.kills;
                    auto& pending=deaths[c.victim];
                    if(pending.used&&(pending.pawn!=c.victimPawn||pending.epoch!=c.victimEpoch||pending.token!=c.token))
                        okay=Fail(6375,"ability_damage_unretired_death_receipt_slot_conflict");
                    else{pending={true,c.victimEpoch,c.token,c.victimPawn,c.victimId};++state.deferredDeaths;
                        // In a synchronous wrapper this is an explicit deferred
                        // request. The forward target/backing stay live until
                        // its outermost original TakeDamage has fully returned.
                        if(!damage::Retire(c.victim))okay=Fail(damage::GetState().error,damage::GetState().stage);
                    }
                }
            }
        }
    }__finally{
        if(local_server_match::pending.active&&local_server_match::pending.token==c.token)combat::policy::CancelNativeDamage();
        state.committing=false;
    }
    // An accepted local result still closes the genuine match transaction when
    // transport subsequently fails. Never queue/replay a committed native hit.
    if(okay&&(combat::GetState().replicationFault||combat::GetState().restartRequired))
        okay=Fail(combat::GetState().error?combat::GetState().error:6376,"ability_damage_real_host_receipt_transport_failed_requires_new_pair");
    Cache();return okay;
}
inline bool Commit(const Contact& c,double now,Result& result){
    __try{return CommitImpl(c,now,result);}__except(Exception(GetExceptionInformation())){return false;}
}
inline damage::Opportunity TryImmediate(const Contact& c,double now,Result& result){
    const auto& input=local_server_input::GetState();const auto victim=local_server_match::Player(c.victim);
    // Root may allow the original input Poll stack. The sole admitted victim
    // is an actual NPC; only its deferred upper observer retirement is queued.
    // This never retires the human input/clock from inside original execution.
    if(!settings.enableImmediate||!settings.immediateBoundary||!victim||victim->human
       ||input.inReceive||!CommonBoundary()||!settings.immediateBoundary()){
        ++state.immediateDeferrals;return damage::Opportunity::Defer;
    }
    ++state.immediateCommits;
    return Commit(c,now,result)?damage::Opportunity::Handled:damage::Opportunity::Failure;
}
inline bool OwnsDamageHooks(){
    if(!state.damageOwned)return false;
    if(!damage::GetState().initialized){
        if(damage::GetState().closePending||damage::WrapperCount())return false;
        for(const auto& binding:damage::bindings)if(binding.used)return false;
        return !damage::hooks.resolveSource&&!damage::hooks.commit;
    }
    const auto& h=damage::hooks;
    return damage::GetState().world==state.world&&h.resolveSource==ResolveSource
        &&h.captureSourceReceipt==CaptureSourceReceipt&&h.allocateToken==combat::AllocateDamageToken&&h.commit==Commit
        &&h.installSharedTakeDamage==local_server_input::InstallAuthorityTakeDamageObserver
        &&h.clearSharedTakeDamage==local_server_input::ClearAuthorityTakeDamageObserver
        &&h.tryCommitBeforeCallerCleanup==(settings.enableImmediate?TryImmediate:nullptr)
        &&h.victimObserverReady==VictimReady&&h.commitBoundaryReady==QueueBoundary;
}
inline bool Initialize(void* world,const Settings& options=Settings{}){
    if(state.initialized)return world==state.world&&!state.error&&!state.closing&&OwnsDamageHooks()
        &&settings.enableImmediate==options.enableImmediate&&settings.immediateBoundary==options.immediateBoundary;
    if(state.closed||!original_players::Thread()||!combat::GetState().initialized||combat::GetState().world!=world
       ||!authority::GetState().initialized||authority::GetState().world!=world
       ||(options.enableImmediate&&!options.immediateBoundary))return false;
    if(damage::GetState().initialized||damage::GetState().error||damage::WrapperCount())
        return Fail(6381,"ability_damage_compositor_cannot_adopt_an_existing_observer_instance");
    state.world=world;state.gameThread=GetCurrentThreadId();settings=options;
    if(!original_inventory_slots::CaptureIdentity(world,state.worldId)||!Live(world,state.worldId))return false;
    if(!combat::SetPawnRetirementObserver(BeforeNPCPawnRetire))return Fail(6377,"ability_damage_npc_retirement_hook_already_owned");
    state.initialized=true;state.immediateEnabled=options.enableImmediate;
    damage::Hooks hooks{};hooks.resolveSource=ResolveSource;hooks.captureSourceReceipt=CaptureSourceReceipt;hooks.allocateToken=combat::AllocateDamageToken;hooks.commit=Commit;
    hooks.installSharedTakeDamage=local_server_input::InstallAuthorityTakeDamageObserver;
    hooks.clearSharedTakeDamage=local_server_input::ClearAuthorityTakeDamageObserver;
    hooks.tryCommitBeforeCallerCleanup=options.enableImmediate?TryImmediate:nullptr;
    hooks.victimObserverReady=VictimReady;hooks.commitBoundaryReady=QueueBoundary;
    state.damageOwned=true;
    if(!damage::Initialize(world,hooks))return Fail(damage::GetState().error?damage::GetState().error:6378,damage::GetState().stage);
    state.stage="ability_damage_compositor_explicitly_initialized_runtime_unverified";Cache();return true;
}
inline bool Poll(double now){
    if(!state.initialized||state.closing||state.closed||state.error||state.restartRequired||!Thread())return false;
    if(!OwnsDamageHooks()||combat::pawnRetirementObserver!=BeforeNPCPawnRetire)
        return Fail(6382,"ability_damage_compositor_observer_hook_ownership_changed");
    if(!FlushRetirements())return false;
    if(!damage::Poll(now))return Fail(damage::GetState().error?damage::GetState().error:6379,damage::GetState().stage);
    if(!FlushRetirements())return false;
    state.stage=damage::GetState().waitingTargets?"ability_damage_compositor_waiting_lower_owned_victim_tables":
        "ability_damage_compositor_real_native_base_radial_observation_ready";Cache();return true;
}
inline bool Close(){
    if(state.closed||!state.initialized)return true;if(!CleanupThread())return false;
    if(!OwnsDamageHooks())return Fail(6383,"ability_damage_compositor_cannot_close_another_observer_instance");
    state.closing=true;
    if(!FlushRetirements())return false;
    if(!damage::Close()){state.stage="ability_damage_compositor_close_waiting_outermost_native_return";Cache();return false;}
    if(!FlushRetirements())return false;
    for(const auto& pending:deaths)if(pending.used)return false;
    if(!combat::ClearPawnRetirementObserver(BeforeNPCPawnRetire))return Fail(6380,"ability_damage_npc_retirement_hook_restore_conflict");
    state.initialized=false;state.closed=true;state.closing=false;
    state.stage="ability_damage_compositor_original_slots_and_retirement_hook_restored";Cache();return true;
}
inline void Write(FILE* file){const auto s=Snapshot();auto b=[](bool v){return v?"true":"false";};
    std::fprintf(file,"{\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"closing\":%s,\"closed\":%s,\"restart_required\":%s,\"immediate_enabled\":%s,\"source_queries\":%llu,\"direct_sources\":%llu,\"spawn_sources\":%llu,\"unresolved_sources\":%llu,\"pending_grant_rejected\":%llu,\"non_human_rejected\":%llu,\"retired_sources\":%llu,\"stale_victims\":%llu,\"self_damage_suicide_unsupported\":%llu,\"policy_rejected\":%llu,\"typed_reference_unsupported\":%llu,\"commits\":%llu,\"applied\":%llu,\"kills\":%llu,\"deferred_deaths\":%llu,\"death_cleanups\":%llu,\"retirement_waits\":%llu,\"immediate_deferrals\":%llu,\"immediate_commits\":%llu,\"queue_boundary_deferrals\":%llu,\"exception_rva\":%llu,\"runtime_damage_verified\":false,\"postmortem_credit_supported\":false,\"self_damage_supported\":false}",
        s.stage,s.error,b(s.initialized),b(s.closing),b(s.closed),b(s.restartRequired),b(s.immediateEnabled),
        s.sourceQueries,s.directSources,s.spawnSources,s.unresolvedSources,s.pendingGrantRejected,s.nonHumanRejected,
        s.sourceRetired,s.staleVictims,s.selfDamageUnsupported,s.policyRejected,s.typedReferenceUnsupported,s.commits,
        s.applied,s.kills,s.deferredDeaths,s.deathCleanups,s.retirementWaits,s.immediateDeferrals,s.immediateCommits,
        s.queueBoundaryDeferrals,s.exceptionRva);
}
} // namespace local_server_ability_damage_integration
