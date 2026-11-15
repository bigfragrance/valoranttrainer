#pragma once
#include "local_server_combat.h"
#include "original_jett_priming.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <string>

// The existing authority combat leaf calls the real reliable heal multicast
// once. This boundary supplies pickup-specific life/epoch and readback guards;
// it does not call the standalone local-only recovery implementation.
namespace local_server_recovery {
using Weak=original_inventory_slots::WeakIdentity;
inline constexpr size_t MaxPlayers=local_server_match::MaxPlayers;
enum class Outcome:uint8_t {None,Applied,AlreadyFull,Deferred,Rejected,Failed};
enum class Admission:uint8_t {Rejected,Deferred,Ready};
struct Gate {
    bool context=false,active=false,currentLife=false,aliveReady=false,busy=false,processing=false;
    int32_t pendingDamage=0;
};
inline Admission Decide(const Gate& g){
    if(!g.context||!g.active||!g.currentLife||!g.aliveReady||g.pendingDamage<0)return Admission::Rejected;
    return g.busy||g.processing||g.pendingDamage?Admission::Deferred:Admission::Ready;
}
// Pure data used by the actual native preflight and by the bounded own-data
// fixture. A zero-capacity default shield is not an effective armor section.
struct Section {
    uintptr_t object=0;uint8_t type=255;bool valid=false,emptyShield=false,healingAllowed=false;
    float life=0,maximum=0,scale=0;
};
struct Life {bool alive=false;float health=0,maximumHealth=0,shield=0,maximumShield=0,rawHealth=0,rawShield=0;};
struct Plan {std::array<size_t,2> rows{};size_t count=0;float amount=0;unsigned health=0,shield=0;};
inline bool Near(float a,float b){return std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<.01f;}
inline bool BuildPlan(const std::array<Section,32>& sections,int count,const Life& life,Plan& out){
    out={};
    if(count<=0||count>static_cast<int>(sections.size())||!life.alive||!std::isfinite(life.health)||life.health<=0
       ||!std::isfinite(life.maximumHealth)||life.maximumHealth<=0||!std::isfinite(life.shield)||life.shield<0
       ||!std::isfinite(life.maximumShield)||life.maximumShield<0
       ||!Near(life.health,life.rawHealth)||!Near(life.shield,life.rawShield))return false;
    for(int i=0;i<count;++i){const auto& s=sections[static_cast<size_t>(i)];
        if(!s.object||!s.valid||!std::isfinite(s.life)||!std::isfinite(s.maximum)||!std::isfinite(s.scale)
           ||s.life<0||s.maximum<0)return false;
        if(s.emptyShield){if(s.type!=1||s.life!=0||s.maximum!=0)return false;continue;}
        if(s.type>1){if(s.life>0)return false;continue;}
        if(s.maximum<=0||s.life>s.maximum+.01f||s.scale!=1||!s.healingAllowed)return false;
        if(s.type==0){
            if(++out.health!=1||!Near(s.life,life.health)||!Near(s.maximum,life.maximumHealth))return false;
        }else if(++out.shield!=1||!Near(s.life,life.shield)||!Near(s.maximum,life.maximumShield))return false;
        const float missing=s.maximum-s.life;if(missing<=.0001f)continue;
        if(out.count>=out.rows.size())return false;
        out.rows[out.count++]=static_cast<size_t>(i);out.amount+=missing;
    }
    return out.health==1&&(out.shield==1||(life.shield==0&&life.maximumShield==0))
        &&std::isfinite(out.amount)&&(out.count?out.amount>0:out.amount==0);
}
struct SectionReceipt {void* object=nullptr;Weak identity{};uint8_t type=255;float before=0,after=0,maximum=0;bool allowed=false,readback=false;};
struct Receipt {
    bool attempted=false,human=false,nativeAliveBefore=false,nativeAliveAfter=false,scoreUnchanged=false,countersUnchanged=false;
    bool tupleUnchanged=false,maximumReadback=false,nativeLeafCalled=false;
    Outcome outcome=Outcome::None;size_t index=MaxPlayers;uint32_t epoch=0;uint64_t sequence=0,sampledAt=0;
    void *pawn=nullptr,*playerState=nullptr,*damageable=nullptr;Weak pawnId{},stateId{},damageableId{},statsId{},infoId{};
    float healthBefore=0,healthAfter=0,maximumHealth=0,shieldBefore=0,shieldAfter=0,maximumShield=0,amount=0;
    int32_t killsBefore=0,killsAfter=0,deathsBefore=0,deathsAfter=0,roundKillsBefore=0,roundDeathsBefore=0;
    uint32_t matchKills=0,matchDeaths=0,streak=0;int32_t nativeEventIndex=0;size_t changes=0;
    std::array<SectionReceipt,2> sections{};
};
struct MethodMetadata {
    bool checked=false,verified=false,lifeChangeVerified=false,dispatchVerified=false;
    void *function=nullptr,*owner=nullptr,*implementation=nullptr;uint32_t flags=0;uint16_t bytes=0;
    uintptr_t execRva=0;int32_t innerBytes=0,structPropertiesBytes=0,callspace=0;
};
struct Diagnostics {
    Outcome outcome=Outcome::None;unsigned error=0;const char* stage="server_pickup_recovery_not_requested";
    bool busy=false,restartRequired=false;uintptr_t exceptionRva=0;
    uint64_t requests=0,applied=0,alreadyFull=0,deferred=0,rejected=0,failed=0,reentrant=0,nativeLeafCalls=0;
    uint64_t humanApplied=0,humanAlreadyFull=0;size_t lastIndex=MaxPlayers;MethodMetadata method{};
};
inline Diagnostics state;
inline std::array<Receipt,MaxPlayers> receipts{};
struct Snapshot {Diagnostics state;std::array<Receipt,MaxPlayers> receipts;};
inline Snapshot cached{};
inline SRWLOCK snapshotLock=SRWLOCK_INIT;
inline const Diagnostics& GetState(){return state;}
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void* object,const Weak& id){return object&&original_inventory_slots::ResolveIdentity(id)==object;}
inline bool Capture(void* object,Weak& id){return object&&original_inventory_slots::CaptureIdentity(object,id)&&Live(object,id);}
inline bool Thread(){return !state.restartRequired&&local_server_combat::Thread()&&local_server_match::Thread();}
inline void Cache(){AcquireSRWLockExclusive(&snapshotLock);cached={state,receipts};ReleaseSRWLockExclusive(&snapshotLock);}
inline bool Field(void* owner,const wchar_t* name,int offset,int bytes,original_jett_priming::PropertyInfo* found=nullptr){
    original_jett_priming::PropertyInfo local{};auto& value=found?*found:local;
    return original_jett_priming::FieldOffset(owner,name,static_cast<size_t>(bytes),&value)==offset
        &&value.elementSize==bytes&&value.arrayDim==1;
}
inline bool Metadata(void* damageable){
    auto& m=state.method;m={};m.checked=true;
    m.function=engine::Find(L"/Script/ShooterGame.DamageableComponent.MulticastNotifyHeal");
    if(!m.function)return false;
    m.owner=Read<void*>(m.function,0x28);m.flags=Read<uint32_t>(m.function,0xb8);m.bytes=Read<uint16_t>(m.function,0xbe);
    const auto exec=Read<uintptr_t>(m.function,0xe0);m.execRva=exec>=engine::image?exec-engine::image:0;
    const auto table=Read<void*>(damageable);m.implementation=Read<void*>(table,0x4d8);
    m.dispatchVerified=Read<void*>(table,0x298)==reinterpret_cast<void*>(engine::image+0x1cdc850)
        &&Read<void*>(table,0x2a0)==reinterpret_cast<void*>(engine::image+0x412c380)
        &&Read<void*>(table,0x2a8)==reinterpret_cast<void*>(engine::image+0x412c9e0);
    if(m.owner!=engine::Class(original_damage::DamageableClass)||m.bytes!=48||m.flags!=0x44cc1||m.execRva!=0x6704dc0
       ||!m.dispatchVerified||m.implementation!=reinterpret_cast<void*>(engine::image+0x6fa0940))return false;
    original_jett_priming::PropertyInfo array{};
    if(!Field(m.function,L"HealTaken",0,4)||!Field(m.function,L"LifeChangeBySection",8,16,&array)
       ||!Field(m.function,L"EventInstigator",24,8)||!Field(m.function,L"EventInstigatorPawn",32,8)
       ||!Field(m.function,L"HealCauser",40,8))return false;
    auto inner=Read<void*>(array.field,0x80),type=Read<void*>(inner,0x78);
    m.innerBytes=Read<int32_t>(inner,0x34);m.structPropertiesBytes=Read<int32_t>(type,0x60);
    auto originalType=engine::Find(L"/Script/ShooterGame.LifeChangeEvent");
    original_jett_priming::PropertyInfo alive{};
    m.lifeChangeVerified=type&&type==originalType&&m.innerBytes==static_cast<int32_t>(sizeof(original_damage::LifeChange))
        &&Read<int32_t>(inner,0x30)==1&&Field(type,L"ChangedComponent",0,8)&&Field(type,L"LifeResult",8,4)
        &&Field(type,L"DeltaLife",12,4)&&Field(type,L"bAliveAfterChange",16,1,&alive)
        &&Read<uint8_t>(alive.field,0x78)==1&&Read<uint8_t>(alive.field,0x79)==0
        &&Read<uint8_t>(alive.field,0x7a)==1&&Read<uint8_t>(alive.field,0x7b)==0xff;
    m.verified=m.lifeChangeVerified;
    if(m.verified)m.callspace=reinterpret_cast<int(*)(void*,void*,void*)>(Read<void*>(table,0x2a0))(damageable,m.function,nullptr);
    return m.verified;
}
inline Outcome Finish(Outcome result,const char* why,unsigned failureCode=0,bool terminal=false){
    state.outcome=result;state.stage=why;state.error=failureCode;if(terminal)state.restartRequired=true;
    if(state.lastIndex<receipts.size())receipts[state.lastIndex].outcome=result;
    if(result==Outcome::Applied)++state.applied;else if(result==Outcome::AlreadyFull)++state.alreadyFull;
    else if(result==Outcome::Deferred)++state.deferred;else if(result==Outcome::Rejected)++state.rejected;
    else if(result==Outcome::Failed)++state.failed;
    return result;
}
inline Outcome RestoreImpl(size_t index,void* expectedPawn,void* expectedState,uint32_t expectedEpoch){
    ++state.requests;state.lastIndex=index;state.error=0;
    if(index>=receipts.size())return Finish(Outcome::Rejected,"server_pickup_recovery_index_not_registered");
    auto& e=receipts[index];e={};e.attempted=true;e.index=index;e.sequence=state.requests;e.sampledAt=GetTickCount64();
    const auto p=local_server_match::Player(index);
    const bool current=p&&p->pawn==expectedPawn&&p->ps==expectedState&&p->epoch==expectedEpoch&&expectedEpoch
        &&local_server_combat::CurrentRegistration(index);
    const auto damageable=current?Read<void*>(p->pawn,0xc70):nullptr;
    const auto& combat=local_server_combat::GetState();const auto& match=local_server_match::GetState();
    const Gate gate{Thread()&&local_server_match::FreeForAll()&&!combat.replicationFault
        &&combat.profilesVerified&&(combat.netMode==1||combat.netMode==2),local_server_match::Active(),current,
        current&&p->ready&&p->equipmentReady&&p->alive&&p->phase==local_server_match::Phase::Alive,
        false,combat.processing||local_server_match::pending.active,damageable?Read<int32_t>(damageable,0x248):0};
    const auto admission=Decide(gate);
    if(admission==Admission::Rejected)return Finish(Outcome::Rejected,"server_pickup_recovery_requires_current_alive_authority_epoch");
    if(admission==Admission::Deferred)return Finish(Outcome::Deferred,"server_pickup_recovery_waiting_original_damage_dispatch");
    const auto& registration=local_server_combat::registrations[index];
    e.human=p->human;e.epoch=p->epoch;e.pawn=p->pawn;e.playerState=p->ps;e.damageable=damageable;
    e.pawnId=registration.pawnId;e.stateId=registration.psId;
    const auto worldEntry=Read<void*>(Read<void*>(e.pawn),0x1b8);
    if(!Live(e.pawn,e.pawnId)||!Live(e.playerState,e.stateId)||!Capture(damageable,e.damageableId)
       ||!original_players::IsA(damageable,original_damage::DamageableClass)||Read<void*>(damageable,0xc8)!=e.pawn
       ||!worldEntry||reinterpret_cast<void*(*)(void*)>(worldEntry)(e.pawn)!=match.world
       ||engine::function<int(*)(void*)>(0x3efc820)(e.pawn)!=combat.netMode
       ||original_damage::world!=match.world||combat.world!=match.world||original_players::world!=match.world
       ||Read<void*>(match.world,0x178)!=match.gameState)
        return Finish(Outcome::Failed,"server_pickup_recovery_native_world_or_component_identity_invalid",7200);
    if(!original_damage::Sample(index))return Finish(Outcome::Failed,"server_pickup_recovery_original_life_query_failed",7201);
    const auto& target=original_damage::targets[index];
    if(!target.bound||target.pawn!=e.pawn||target.state!=e.playerState||target.damageable!=damageable
       ||!target.alive||target.health<=0)return Finish(Outcome::Rejected,"server_pickup_recovery_never_resurrects_dead_pawn");
    e.nativeAliveBefore=target.alive;e.healthBefore=target.health;e.maximumHealth=target.maxHealth;
    e.shieldBefore=target.shield;e.maximumShield=target.maxShield;
    if(!Metadata(damageable))return Finish(Outcome::Failed,"server_pickup_recovery_original_reliable_heal_abi_not_verified",7202);
    if((state.method.callspace&3)!=3)return Finish(Outcome::Deferred,"server_pickup_recovery_waiting_original_local_and_remote_callspace");
    original_damage::SectionAudit audit{};
    if(!original_damage::DescribeSections(e.pawn,damageable,audit))return Finish(Outcome::Rejected,"server_pickup_recovery_original_section_schema_not_supported");
    std::array<Section,32> values{};
    for(int32_t n=0;n<audit.count;++n){const auto& row=audit.entries[n];
        values[static_cast<size_t>(n)]={reinterpret_cast<uintptr_t>(row.object),row.type,
            row.originalType&&row.ownerMatches&&row.nativeValues&&row.validNumbers,row.emptyShield,
            Read<uint8_t>(row.object,0x149)!=0,row.nativeLife,row.nativeMaximum,row.damageScale};
    }
    Plan plan{};const Life life{target.alive,target.health,target.maxHealth,target.shield,target.maxShield,target.rawHealth,target.rawShield};
    if(!BuildPlan(values,audit.count,life,plan))return Finish(Outcome::Rejected,"server_pickup_recovery_living_health_armor_schema_not_supported");
    e.changes=plan.count;e.amount=plan.amount;
    auto score=original_scoreboard::Find(e.playerState);
    if(!score||!original_scoreboard::ReadNativeScore(*score)||!Capture(score->stats,e.statsId)||!Capture(score->info,e.infoId))
        return Finish(Outcome::Failed,"server_pickup_recovery_original_score_guard_unavailable",7203);
    e.killsBefore=score->kills;e.deathsBefore=score->deaths;e.roundKillsBefore=score->round_kills;e.roundDeathsBefore=score->round_deaths;
    e.matchKills=p->kills;e.matchDeaths=p->deaths;e.streak=p->streak;e.nativeEventIndex=Read<int32_t>(damageable,0x1e0);
    const auto originalDamageEvents=original_damage::diagnostics.damageEvents,originalDeathEvents=original_damage::diagnostics.deathEvents;
    const auto hostDamageEvents=combat.damageEvents,hostKills=combat.kills;
    for(size_t n=0;n<plan.count;++n){const auto& row=audit.entries[plan.rows[n]];auto& section=e.sections[n];
        section.object=row.object;section.type=row.type;section.before=row.nativeLife;section.maximum=row.nativeMaximum;
        if(!Capture(row.object,section.identity)||!local_server_combat::ActualCanSetLife(row.object,section.allowed))
            return Finish(Outcome::Failed,"server_pickup_recovery_original_section_permission_query_failed",7204);
        if(!section.allowed)return Finish(Outcome::Rejected,"server_pickup_recovery_original_section_disallows_restore");
    }
    if(plan.count){
        e.nativeLeafCalled=true;++state.nativeLeafCalls;
        if(!local_server_combat::RestoreMaximum(index))
            return Finish(Outcome::Failed,"server_pickup_recovery_native_heal_leaf_failed_no_retry",7205,true);
    }
    const auto currentPlayer=local_server_match::Player(index);
    e.tupleUnchanged=currentPlayer&&local_server_combat::CurrentRegistration(index)&&currentPlayer->pawn==e.pawn
        &&currentPlayer->ps==e.playerState&&currentPlayer->epoch==e.epoch&&Live(e.damageable,e.damageableId)
        &&Read<void*>(e.pawn,0xc70)==e.damageable;
    if(!e.tupleUnchanged||!Live(score->stats,e.statsId)||!Live(score->info,e.infoId)
       ||!original_damage::Sample(index)||!original_scoreboard::ReadNativeScore(*score))
        return Finish(Outcome::Failed,"server_pickup_recovery_native_life_identity_or_score_readback_failed",7206,plan.count!=0);
    const auto& after=original_damage::targets[index];e.nativeAliveAfter=after.alive;e.healthAfter=after.health;e.shieldAfter=after.shield;
    e.killsAfter=score->kills;e.deathsAfter=score->deaths;
    e.scoreUnchanged=score->kills==e.killsBefore&&score->deaths==e.deathsBefore&&score->round_kills==e.roundKillsBefore
        &&score->round_deaths==e.roundDeathsBefore&&currentPlayer->kills==e.matchKills
        &&currentPlayer->deaths==e.matchDeaths&&currentPlayer->streak==e.streak;
    e.countersUnchanged=original_damage::diagnostics.damageEvents==originalDamageEvents
        &&original_damage::diagnostics.deathEvents==originalDeathEvents&&combat.damageEvents==hostDamageEvents&&combat.kills==hostKills
        &&Read<int32_t>(damageable,0x1e0)==e.nativeEventIndex&&!local_server_match::pending.active;
    e.maximumReadback=e.nativeAliveAfter&&after.health>0&&Near(after.health,e.maximumHealth)&&Near(after.rawHealth,e.maximumHealth)
        &&Near(after.shield,e.maximumShield)&&Near(after.rawShield,e.maximumShield)
        &&Near(after.maxHealth,e.maximumHealth)&&Near(after.maxShield,e.maximumShield);
    for(size_t n=0;n<plan.count;++n){auto& section=e.sections[n];
        if(!Live(section.object,section.identity)||Read<void*>(section.object,0x110)!=damageable
           ||!original_damage::Event(section.object,L"/Script/ShooterGame.DamageSectionComponent.GetLife",4,&section.after))
            return Finish(Outcome::Failed,"server_pickup_recovery_healed_section_identity_changed",7207,true);
        section.readback=Near(section.after,section.maximum);e.maximumReadback=e.maximumReadback&&section.readback;
    }
    if(!e.maximumReadback||!e.scoreUnchanged||!e.countersUnchanged||Read<int32_t>(damageable,0x248)!=0)
        return Finish(Outcome::Failed,"server_pickup_recovery_maximum_or_score_transaction_guard_failed",7208,plan.count!=0);
    if(e.human){if(plan.count)++state.humanApplied;else ++state.humanAlreadyFull;}
    return Finish(plan.count?Outcome::Applied:Outcome::AlreadyFull,plan.count
        ?"server_pickup_recovery_original_multicast_host_maximum_verified":"server_pickup_recovery_original_health_armor_already_full");
}
inline LONG CaptureException(EXCEPTION_POINTERS* exception){
    state.exceptionRva=0;unsigned code=0xe0007209U;
    if(exception&&exception->ExceptionRecord){code=exception->ExceptionRecord->ExceptionCode;
        const auto at=reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress);state.exceptionRva=at>=engine::image?at-engine::image:at;}
    Finish(Outcome::Failed,"server_pickup_recovery_native_exception_requires_new_session",code,true);return EXCEPTION_EXECUTE_HANDLER;
}
inline Outcome ProtectedRestore(size_t index,void* pawn,void* ps,uint32_t epoch){
    __try{return RestoreImpl(index,pawn,ps,epoch);}__except(local_server_recovery::CaptureException(GetExceptionInformation())){return Outcome::Failed;}
}
inline Outcome RestoreForPawn(size_t index,void* pawn,void* ps,uint32_t epoch){
    if(!Thread())return Outcome::Rejected;
    // Do not overwrite an outer receipt during an original OnHealed callback.
    if(state.busy){++state.deferred;++state.reentrant;return Outcome::Deferred;}
    const bool previous=state.busy;Outcome result=Outcome::Failed;state.busy=true;
    __try{result=ProtectedRestore(index,pawn,ps,epoch);}
    __finally{state.busy=previous;Cache();}
    return result;
}
inline std::string Json(){
    AcquireSRWLockShared(&snapshotLock);const auto value=cached;ReleaseSRWLockShared(&snapshotLock);
    const auto& s=value.state;local_server_match::Json out;const auto b=[](bool v){return v?"true":"false";};
    const auto u=[](uint64_t n){return static_cast<unsigned long long>(n);};
    const auto number=[&](float v){if(std::isfinite(v))out.Add("%.9g",static_cast<double>(v));else out.Add("null");};
    out.Add("{\"stage\":\"%s\",\"outcome\":%u,\"error\":%u,\"restart_required\":%s,\"requests\":%llu,\"applied\":%llu,\"already_full\":%llu,\"deferred\":%llu,\"rejected\":%llu,\"failed\":%llu,\"reentrant\":%llu,\"native_leaf_calls\":%llu,\"human_applied\":%llu,\"human_already_full\":%llu,\"exception_rva\":%llu,\"reliable_rpc_metadata\":{\"checked\":%s,\"verified\":%s,\"function\":\"%p\",\"flags\":%u,\"parameter_bytes\":%u,\"exec_rva\":%llu,\"life_change_stride\":%d,\"life_change_properties_bytes\":%d},\"players\":[",
        s.stage,unsigned(s.outcome),s.error,b(s.restartRequired),u(s.requests),u(s.applied),u(s.alreadyFull),u(s.deferred),u(s.rejected),u(s.failed),u(s.reentrant),u(s.nativeLeafCalls),u(s.humanApplied),u(s.humanAlreadyFull),u(s.exceptionRva),
        b(s.method.checked),b(s.method.verified),s.method.function,s.method.flags,unsigned(s.method.bytes),u(s.method.execRva),s.method.innerBytes,s.method.structPropertiesBytes);
    bool comma=false;for(const auto& e:value.receipts)if(e.attempted){
        out.Add("%s{\"index\":%zu,\"epoch\":%u,\"human\":%s,\"sequence\":%llu,\"sampled_at_ms\":%llu,\"outcome\":%u,\"pawn\":\"%p\",\"player_state\":\"%p\",\"damageable\":\"%p\",\"changes\":%zu,\"native_leaf_called\":%s,\"native_alive_before\":%s,\"native_alive_after\":%s,\"tuple_unchanged\":%s,\"maximum_readback\":%s,\"score_unchanged\":%s,\"damage_counters_unchanged\":%s,\"health_before\":",
            comma?",":"",e.index,e.epoch,b(e.human),u(e.sequence),u(e.sampledAt),unsigned(e.outcome),e.pawn,e.playerState,e.damageable,e.changes,b(e.nativeLeafCalled),b(e.nativeAliveBefore),b(e.nativeAliveAfter),b(e.tupleUnchanged),b(e.maximumReadback),b(e.scoreUnchanged),b(e.countersUnchanged));comma=true;
        number(e.healthBefore);out.Add(",\"health_after\":");number(e.healthAfter);out.Add(",\"maximum_health\":");number(e.maximumHealth);
        out.Add(",\"shield_before\":");number(e.shieldBefore);out.Add(",\"shield_after\":");number(e.shieldAfter);out.Add(",\"maximum_shield\":");number(e.maximumShield);
        out.Add(",\"healed_amount\":");number(e.amount);out.Add(",\"kills_before\":%d,\"kills_after\":%d,\"deaths_before\":%d,\"deaths_after\":%d}",e.killsBefore,e.killsAfter,e.deathsBefore,e.deathsAfter);
    }
    out.Add("],\"original_heal_callspace\":%d,\"native_dispatch_virtuals_verified\":%s,\"one_existing_native_multicast_leaf\":true,\"standalone_heal_leaf_calls\":0,\"direct_health_alive_writes\":0,\"negative_damage_healing\":false,\"replica_delivery_verified\":false,\"runtime_acceptance_verified\":false}",s.method.callspace,b(s.method.dispatchVerified));
    return out.value;
}
} // namespace local_server_recovery
