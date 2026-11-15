#pragma once
#include "deathmatch_rules.h"
#include "original_armor.h"
#include <array>
#include <cmath>

// AuthRestore is empty in this fixed client. Supply the missing local positive
// LifeChange records; the original MulticastNotifyHeal implementation performs
// section updates, dirty/HUD notification and OnHealed. No point-damage event,
// kill token, score write, character resurrection, image or CDO mutation.
// ABI/native evidence: work/original-recovery.
namespace original_recovery {
inline constexpr size_t MaxPlayers=original_players::MaxPlayers;
inline constexpr uintptr_t NativeHealImplementation=0x6fa0940;
enum class Outcome:uint8_t {None,Applied,AlreadyFull,Deferred,Rejected,Failed};
struct HealParameters {
    float amount=0;uint32_t padding=0;
    original_damage::Array<original_damage::LifeChange> changes;
    void *instigator=nullptr,*instigatorPawn=nullptr,*causer=nullptr;
};
static_assert(sizeof(HealParameters)==48&&offsetof(HealParameters,changes)==8&&offsetof(HealParameters,causer)==40);
struct Entry {
    void *pawn=nullptr,*damageable=nullptr,*playerState=nullptr;
    original_inventory_slots::WeakIdentity pawnIdentity{};
    uint32_t spawnEpoch=0,attempts=0,completed=0;
    Outcome outcome=Outcome::None;
    bool nativeReadback=false,scoreUnchanged=false,damageCountersUnchanged=false;
    float healthBefore=0,healthAfter=0,maxHealth=0,nativeMaxHealth=0,shieldBefore=0,shieldAfter=0,maxShield=0,amount=0;
    uint32_t changedSections=0;
    int32_t killsBefore=0,killsAfter=0,deathsBefore=0,deathsAfter=0;
    original_damage::SectionAudit before,after;
};
struct Diagnostics {
    bool busy=false,restartRequired=false;
    unsigned error=0,requests=0,applied=0,alreadyFull=0,deferred=0,rejected=0,nativeHealCalls=0;
    Outcome outcome=Outcome::None;
    size_t lastIndex=MaxPlayers;
    uintptr_t exceptionRva=0;
    const char* stage="original_recovery_not_requested";
};
inline Diagnostics diagnostics;
inline std::array<Entry,MaxPlayers> entries{};
inline bool inProgress=false;
inline const Diagnostics& State(){return diagnostics;}
inline const Entry* Recovery(size_t index){return index<entries.size()?&entries[index]:nullptr;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Thread(){return !diagnostics.restartRequired&&original_players::Thread()&&deathmatch_rules::Thread();}
inline bool Finish(Outcome outcome,const char* stage,unsigned error=0){
    diagnostics.outcome=outcome;diagnostics.stage=stage;diagnostics.error=error;diagnostics.busy=outcome==Outcome::Deferred;
    if(diagnostics.lastIndex<entries.size())entries[diagnostics.lastIndex].outcome=outcome;
    if(outcome==Outcome::Deferred)++diagnostics.deferred;
    else if(outcome==Outcome::Rejected)++diagnostics.rejected;
    return outcome==Outcome::Applied||outcome==Outcome::AlreadyFull;
}
inline bool Fail(unsigned error,const char* stage){return Finish(Outcome::Failed,stage,error);}
inline bool ActualCanSetLife(void* section,bool& allowed){
    auto base=engine::Find(L"/Script/ShooterGame.DamageSectionComponent.CanSetLife");
    if(!base)return false;
    original_damage::Name name;std::memcpy(name.bytes,static_cast<uint8_t*>(base)+0x18,sizeof(name));
    // Same original object/class function lookup used by native item callbacks;
    // retain BasicArmorAttachedDamageSection's real Blueprint override.
    auto actual=engine::function<void*(*)(void*,const original_damage::Name*)>(0x1cdc710)(section,&name);
    uint8_t result=0;
    if(!original_damage::EventWithFunction(section,L"/Script/ShooterGame.DamageSectionComponent.CanSetLife",actual,1,&result))return false;
    allowed=result!=0;return true;
}
inline bool ClampMaximum(float maximum,float& value){
    struct Clamp {double value=0,minimum=0,maximum=0,result=0;} q;
    static_assert(sizeof(Clamp)==32);
    q.value=q.maximum=maximum;
    if(!original_damage::Event(engine::DefaultObject(engine::Class(0x4526580)),L"/Script/Engine.KismetMathLibrary.FClamp",32,&q)
       ||!std::isfinite(q.result)||q.result<0||q.result>maximum)return false;
    value=static_cast<float>(q.result);return std::isfinite(value);
}
inline bool RestoreImpl(size_t index){
    ++diagnostics.requests;diagnostics.lastIndex=index;diagnostics.busy=false;diagnostics.error=0;
    if(!Thread()||index>=entries.size())return Fail(2850,"original_recovery_original_game_thread_or_index_missing");
    if(deathmatch_rules::pending.active)return Finish(Outcome::Deferred,"original_recovery_waiting_for_native_damage_transaction");
    const auto* participant=deathmatch_rules::Player(index);
    if(!deathmatch_rules::Active()||!participant||!participant->registered||!participant->ready
       ||!participant->alive||participant->phase!=deathmatch_rules::Phase::Alive)
        return Finish(Outcome::Rejected,"original_recovery_requires_living_active_participant");
    auto& e=entries[index];const auto attempts=e.attempts+1,completed=e.completed;e={};e.attempts=attempts;e.completed=completed;
    e.pawn=participant->pawn;e.playerState=participant->playerState;e.spawnEpoch=participant->spawnEpoch;e.damageable=Read<void*>(e.pawn,0xc70);
    const auto& roster=original_players::players[index];
    if(roster.character!=e.pawn||roster.state!=e.playerState||!original_players::Registered(e.playerState)
       ||!original_inventory_slots::CaptureIdentity(e.pawn,e.pawnIdentity)
       ||!original_players::IsA(e.damageable,original_damage::DamageableClass)||Read<void*>(e.damageable,0xc8)!=e.pawn
       ||engine::function<int(*)(void*)>(0x3efc820)(e.pawn)!=0)
        return Fail(2851,"original_recovery_original_identity_or_standalone_context_invalid");
    if(Read<int32_t>(e.damageable,0x248)!=0)return Finish(Outcome::Deferred,"original_recovery_waiting_for_queued_native_damage");
    bool alive=false;
    if(!deathmatch_rules::ReadLife(e.pawn,alive,e.healthBefore,e.maxHealth)||!alive||e.healthBefore<=0)
        return Finish(Outcome::Rejected,"original_recovery_does_not_resurrect_native_dead_character");
    if(!original_damage::DescribeSections(e.pawn,e.damageable,e.before))return Fail(2852,"original_recovery_original_section_queries_failed");
    auto score=original_scoreboard::Find(e.playerState);
    if(!score||!original_scoreboard::ReadNativeScore(*score))return Fail(2853,"original_recovery_score_guard_unavailable");
    e.killsBefore=score->kills;e.deathsBefore=score->deaths;
    const auto matchKills=participant->matchKills,matchDeaths=participant->matchDeaths,streak=participant->streak;
    const auto damageEvents=original_damage::diagnostics.damageEvents,deathEvents=original_damage::diagnostics.deathEvents;
    std::array<original_damage::LifeChange,2> changes{};int32_t changeCount=0;
    unsigned healthCount=0,shieldCount=0;
    for(int32_t i=0;i<e.before.count;++i){const auto& row=e.before.entries[i];
        if(row.type==1&&row.emptyShield)continue;
        if(row.type>1){if(row.nativeLife>0)return Fail(2854,"original_recovery_active_nonstandard_section_requires_original_evidence");continue;}
        if(row.type==0){
            if(++healthCount>1)return Fail(2855,"original_recovery_multiple_health_sections_not_supported");
            e.nativeMaxHealth=row.nativeMaximum;
            if(std::abs(row.nativeLife-e.healthBefore)>.01f)return Fail(2869,"original_recovery_temporary_or_nonstandard_health_requires_original_evidence");
        }
        else if(++shieldCount>1)return Fail(2856,"original_recovery_multiple_effective_shield_sections_not_supported");
        if(row.nativeMaximum<=0||row.nativeLife>row.nativeMaximum+.01f||row.damageScale!=1
           ||!Read<uint8_t>(row.object,0x149))return Fail(2857,"original_recovery_nonstandard_section_or_healing_policy");
        if(row.type==1){e.shieldBefore=row.nativeLife;e.maxShield=row.nativeMaximum;e.before.selectedShield=row.object;}
        if(row.nativeMaximum-row.nativeLife<=.0001f)continue;
        bool canSet=false;
        if(!ActualCanSetLife(row.object,canSet))return Fail(2858,"original_recovery_original_can_set_life_query_failed");
        if(!canSet)return Finish(Outcome::Rejected,"original_recovery_original_section_disallows_life_change");
        float value=0;
        if(!ClampMaximum(row.nativeMaximum,value))return Fail(2859,"original_recovery_original_clamp_failed");
        auto& change=changes[changeCount++];change.section=row.object;change.life=value;
        change.delta=value-row.nativeLife;change.alive=value>0?1:0;e.amount+=change.delta;
    }
    if(healthCount!=1||(deathmatch_rules::config.mode==deathmatch_rules::Mode::FreeForAll&&shieldCount!=1))
        return Fail(2860,"original_recovery_required_original_health_or_ffa_armor_missing");
    e.changedSections=static_cast<uint32_t>(changeCount);
    if(!changeCount){
        e.after=e.before;e.healthAfter=e.healthBefore;e.shieldAfter=e.shieldBefore;
        e.killsAfter=e.killsBefore;e.deathsAfter=e.deathsBefore;e.nativeReadback=e.scoreUnchanged=e.damageCountersUnchanged=true;
        ++diagnostics.alreadyFull;return Finish(Outcome::AlreadyFull,"original_recovery_original_sections_already_full");
    }
    if(!std::isfinite(e.amount)||e.amount<=0)return Fail(2861,"original_recovery_positive_heal_amount_invalid");
    auto implementation=Read<void*>(Read<void**>(e.damageable,0),0x4d8);
    if(implementation!=reinterpret_cast<void*>(engine::image+NativeHealImplementation))return Fail(2862,"original_recovery_original_heal_implementation_profile_changed");
    HealParameters heal;heal.amount=e.amount;heal.changes={changes.data(),changeCount,static_cast<int32_t>(changes.size())};
    // Exact complete original local multicast implementation. Null contexts
    // match the original HealthPack's two AuthRestore(null,null) calls. The
    // native routine never changes the Damageable's global alive/death state.
    reinterpret_cast<void(*)(void*,float,const original_damage::Array<original_damage::LifeChange>*,void*,void*,void*)>(implementation)
        (e.damageable,heal.amount,&heal.changes,heal.instigator,heal.instigatorPawn,heal.causer);
    ++diagnostics.nativeHealCalls;
    if(original_inventory_slots::ResolveIdentity(e.pawnIdentity)!=e.pawn
       ||deathmatch_rules::pending.active||Read<int32_t>(e.damageable,0x248)!=0
       ||!deathmatch_rules::ReadLife(e.pawn,alive,e.healthAfter,e.maxHealth)||!alive||e.healthAfter<=0
       ||!original_damage::DescribeSections(e.pawn,e.damageable,e.after))return Fail(2863,"original_recovery_native_identity_or_life_readback_failed");
    e.nativeReadback=std::abs(e.healthAfter-e.nativeMaxHealth)<.01f;
    for(int32_t i=0;i<changeCount;++i){const auto& change=changes[i];bool found=false;
        for(int32_t j=0;j<e.after.count;++j){const auto& row=e.after.entries[j];if(row.object!=change.section)continue;
            found=true;if(std::abs(row.nativeLife-change.life)>.01f||!row.alive)e.nativeReadback=false;
        }
        if(!found)e.nativeReadback=false;
    }
    if(!original_damage::Event(e.pawn,L"/Script/ShooterGame.ShooterCharacter.GetShield",4,&e.shieldAfter))return Fail(2864,"original_recovery_original_shield_readback_failed");
    e.nativeReadback=e.nativeReadback&&std::isfinite(e.shieldAfter)&&std::abs(e.shieldAfter-e.maxShield)<.01f;
    if(!original_scoreboard::ReadNativeScore(*score))return Fail(2865,"original_recovery_score_guard_readback_failed");
    e.killsAfter=score->kills;e.deathsAfter=score->deaths;
    participant=deathmatch_rules::Player(index);
    e.scoreUnchanged=e.killsAfter==e.killsBefore&&e.deathsAfter==e.deathsBefore
        &&participant&&participant->pawn==e.pawn&&participant->spawnEpoch==e.spawnEpoch
        &&participant->matchKills==matchKills&&participant->matchDeaths==matchDeaths&&participant->streak==streak;
    e.damageCountersUnchanged=damageEvents==original_damage::diagnostics.damageEvents&&deathEvents==original_damage::diagnostics.deathEvents;
    if(!e.nativeReadback||!e.scoreUnchanged||!e.damageCountersUnchanged)return Fail(2866,"original_recovery_native_maximum_or_ledger_guard_failed");
    if(original_damage::targets[index].bound&&original_damage::targets[index].pawn==e.pawn&&!original_damage::Sample(index))
        return Fail(2867,"original_recovery_damage_cache_native_readback_failed");
    const auto armor=original_armor::Armor(index);
    if(armor&&armor->bound&&armor->pawn==e.pawn&&!original_armor::Sample(index))return Fail(2868,"original_recovery_armor_native_readback_failed");
    ++e.completed;++diagnostics.applied;return Finish(Outcome::Applied,"original_recovery_native_heal_maximum_verified");
}
inline LONG Capture(EXCEPTION_POINTERS* p){
    diagnostics.error=p&&p->ExceptionRecord?p->ExceptionRecord->ExceptionCode:0xe0002850U;
    const auto address=p&&p->ExceptionRecord?reinterpret_cast<uintptr_t>(p->ExceptionRecord->ExceptionAddress):0;
    diagnostics.exceptionRva=address>=engine::image?address-engine::image:address;
    diagnostics.restartRequired=true;diagnostics.busy=false;diagnostics.outcome=Outcome::Failed;
    diagnostics.stage="original_recovery_native_exception_requires_new_session";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool RestoreMaximum(size_t index){
    if(inProgress){diagnostics.busy=true;++diagnostics.deferred;return false;}
    inProgress=true;bool result=false;
    __try{result=RestoreImpl(index);}__except(Capture(GetExceptionInformation())){result=false;}
    inProgress=false;return result;
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    auto b=[](bool v){return v?"true":"false";};
    std::fprintf(f,"{\"schema_version\":1,\"stage\":");original_players::Text(f,diagnostics.stage);
    std::fprintf(f,",\"error\":%u,\"outcome\":%u,\"busy\":%s,\"requests\":%u,\"applied\":%u,\"already_full\":%u,\"deferred\":%u,\"rejected\":%u,\"native_heal_calls\":%u,\"players\":[",diagnostics.error,static_cast<unsigned>(diagnostics.outcome),b(diagnostics.busy),diagnostics.requests,diagnostics.applied,diagnostics.alreadyFull,diagnostics.deferred,diagnostics.rejected,diagnostics.nativeHealCalls);
    bool first=true;for(size_t i=0;i<entries.size();++i){const auto& e=entries[i];if(!e.attempts)continue;
        std::fprintf(f,"%s{\"index\":%zu,\"attempts\":%u,\"completed\":%u,\"spawn_epoch\":%u,\"outcome\":%u,\"health_before\":%.9g,\"health_after\":%.9g,\"max_health\":%.9g,\"native_maximum_health\":%.9g,\"shield_before\":%.9g,\"shield_after\":%.9g,\"max_shield\":%.9g,\"heal_amount\":%.9g,\"changed_sections\":%u,\"native_readback\":%s,\"score_unchanged\":%s,\"damage_counters_unchanged\":%s,\"kills_before\":%d,\"kills_after\":%d,\"deaths_before\":%d,\"deaths_after\":%d,\"before\":",first?"":",",i,e.attempts,e.completed,e.spawnEpoch,static_cast<unsigned>(e.outcome),e.healthBefore,e.healthAfter,e.maxHealth,e.nativeMaxHealth,e.shieldBefore,e.shieldAfter,e.maxShield,e.amount,e.changedSections,b(e.nativeReadback),b(e.scoreUnchanged),b(e.damageCountersUnchanged),e.killsBefore,e.killsAfter,e.deathsBefore,e.deathsAfter);
        original_damage::WriteSectionAudit(f,e.before);std::fputs(",\"after\":",f);original_damage::WriteSectionAudit(f,e.after);std::fputc('}',f);first=false;
    }
    std::fputs("],\"local_authority_heal_records\":true,\"direct_native_life_writes\":false,\"point_damage_events_created\":false,\"kill_events_created\":false,\"global_alive_written\":false,\"image_pages_written\":false,\"runtime_verified\":false}\n",f);
    const bool ok=std::ferror(f)==0;std::fclose(f);return ok;
}
}
