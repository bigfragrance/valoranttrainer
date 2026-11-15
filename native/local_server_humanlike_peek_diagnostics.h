#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include "humanlike_bot_brain.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>

// Own-plan debug cache for the FILE bridge only. No timer, game read, target
// access, transport/ACK mutation, or addition to human Summary/ControlState.
namespace local_server_humanlike_peek_diagnostics {
namespace dto=humanlike_bot;
inline constexpr size_t TraceTailCapacity=2,MaxBotBytes=5120,MaxRosterProfiles=32;
struct Source {const dto::Brain* brain=nullptr;const dto::OwnSnapshot* own=nullptr;};
inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline std::string cached="{\"schema_version\":1,\"cached\":false,\"bots\":[]}";
inline void Bool(std::string&out,bool value){out+=value?"true":"false";}
inline void Number(std::string&out,double value,int decimals=3){
    if(!std::isfinite(value)){out+="null";return;}
    char buffer[384]{};const int size=snprintf(buffer,sizeof(buffer),"%.*f",decimals,value);
    if(size<0||static_cast<size_t>(size)>=sizeof(buffer)){out+="null";return;}
    out.append(buffer,static_cast<size_t>(size));
}
inline void Integer(std::string&out,uint64_t value){
    char buffer[32]{};const int size=snprintf(buffer,sizeof(buffer),"%llu",static_cast<unsigned long long>(value));
    if(size>0&&static_cast<size_t>(size)<sizeof(buffer))out.append(buffer,static_cast<size_t>(size));else out+="null";
}
inline void Vec(std::string&out,const dto::Vec3&value,bool known){
    if(!known||!dto::math::Finite(value)){out+="null";return;}
    out+='[';Number(out,value.x);out+=',';Number(out,value.y);out+=',';Number(out,value.z);out+=']';
}
inline void Plan(std::string&out,const dto::PeekPlanSnapshot&p){
    out+="{\"known\":";Bool(out,p.known);out+=" ,\"active\":";Bool(out,p.active);
    out+=" ,\"peek\":";Integer(out,static_cast<unsigned>(p.peek));out+=" ,\"last_active_motion\":";Integer(out,static_cast<unsigned>(p.lastActiveMotion));
    out+=" ,\"stage_mask_at_note\":";Integer(out,p.stageMaskAtNote);out+=" ,\"profile_revision\":";Integer(out,p.revision);
    out+=" ,\"first_intent_serial\":";Integer(out,p.firstIntentSerial);out+=" ,\"last_intent_serial\":";Integer(out,p.lastIntentSerial);
    out+=" ,\"first_trace_serial\":";Integer(out,p.firstTraceSerial);out+=" ,\"last_trace_serial\":";Integer(out,p.lastTraceSerial);
    out+=" ,\"started_at\":";if(p.known)Number(out,p.startedAtSeconds,6);else out+="null";
    out+=" ,\"stage_at\":";if(p.known)Number(out,p.stageAtSeconds,6);else out+="null";
    out+=" ,\"observed_at\":";if(p.known)Number(out,p.observedAtSeconds,6);else out+="null";
    out+=" ,\"ended_at\":";if(p.known&&p.endedAtSeconds>=0)Number(out,p.endedAtSeconds,6);else out+="null";
    out+=" ,\"start_feet\":";Vec(out,p.start,p.known);out+=" ,\"goal_feet\":";Vec(out,p.goal,p.known);out+=" ,\"retreat_feet\":";Vec(out,p.retreat,p.known);
    out+=" ,\"last_active_flags\":{\"jump_requested\":";Bool(out,p.jumpRequested);out+=" ,\"air_seen\":";Bool(out,p.airSeen);
    out+=" ,\"landed\":";Bool(out,p.landed);out+=" ,\"crouch_seen\":";Bool(out,p.crouchSeen);
    out+=" ,\"information_seen\":";Bool(out,p.informationSeen);out+=" ,\"return_chosen\":";Bool(out,p.returnChosen);
    out+=" ,\"poppin_extended\":";Bool(out,p.poppinExtended);out+='}';
    out+=" ,\"terminal_own_sample\":{\"known\":";Bool(out,p.terminalOwnKnown);
    out+=" ,\"sampled_at\":";if(p.terminalOwnKnown)Number(out,p.terminalOwnSampledAtSeconds,6);else out+="null";
    out+=" ,\"feet\":";Vec(out,p.terminalOwnFeet,p.terminalOwnKnown);
    out+=" ,\"ground_known\":";Bool(out,p.terminalGroundKnown);out+=" ,\"grounded\":";Bool(out,p.terminalGrounded);out+="}}";
}
inline void Combat(std::string& out,const dto::Brain& brain,const dto::OwnSnapshot* own){
    const auto value=own?brain.ReadCombat(*own):dto::CombatReadback{};
    out+=" ,\"combat_readback\":{\"current\":";Bool(out,value.current);
    out+=",\"now\":";Number(out,value.now,6);out+=",\"own_sample_at\":";Number(out,value.ownSampleAt,6);
    out+=",\"target_track\":";Integer(out,value.target);
    out+=",\"target_known\":";Bool(out,value.targetKnown);out+=",\"target_visible\":";Bool(out,value.targetVisible);
    out+=",\"target_retired\":";Bool(out,value.targetRetired);out+=",\"target_point\":";Vec(out,value.point,value.targetKnown);
    out+=",\"target_point_kind\":";if(value.targetKnown)Integer(out,static_cast<unsigned>(value.pointKind));else out+="null";
    out+=",\"head_visible_when_selected\":";if(value.targetKnown)Bool(out,value.headAvailableAtSelection);else out+="null";
    out+=",\"target_observed_at\":";Number(out,value.targetObservedAt,6);
    out+=",\"observed_eye_angle_error_deg\":";if(value.targetKnown)Number(out,value.observedErrorDegrees,6);else out+="null";
    out+=",\"existing_fire_gate_angle_error_deg\":";if(value.targetKnown&&value.shootingViewKnown)Number(out,value.gateErrorDegrees,6);else out+="null";
    out+=",\"shooting_view_required\":";Bool(out,value.shootingViewRequired);
    out+=",\"shooting_view_known\":";Bool(out,value.shootingViewKnown);
    out+=",\"shooting_view_sample_at\":";Number(out,value.shootingViewAt,6);
    out+=",\"fire_alignment_tolerance_deg\":";Number(out,value.toleranceDegrees,6);
    out+=",\"notice_at\":";Number(out,value.noticeAt,6);out+=",\"action_at\":";Number(out,value.actionAt,6);
    out+=",\"next_fire_at\":";Number(out,value.nextFireAt,6);out+=",\"requested_fire\":";Bool(out,value.requestedFire);
    out+=",\"heard_sound_episode\":";Integer(out,brain.HeardSoundEpisode());
    out+=",\"navigation_sound_episode\":";Integer(out,brain.SoundNavigationEpisode());
    out+=",\"failed_sound_navigation_episode\":";Integer(out,brain.FailedSoundNavigationEpisode());
    out+=",\"history_ready\":";Bool(out,value.historyReady);out+=",\"fire_window\":";Bool(out,value.window);
    out+=",\"holding\":";Bool(out,value.holding);out+=",\"need_release\":";Bool(out,value.needRelease);
    out+=",\"per_intent_execution_proven\":false,\"source\":\"own_snapshot_and_acquired_track\"}";
}
inline bool AppendBot(std::string&out,size_t index,const Source&source){
    const auto begin=out.size();out+="{\"index\":";Integer(out,index);out+=" ,\"available\":";Bool(out,source.brain!=nullptr);
    if(!source.brain){out+='}';return false;}
    const auto&s=source.brain->Snapshot();const auto&p=s.peekPlan;const auto&ended=s.lastEndedPeekPlan;const auto*own=source.own;
    out+=" ,\"epoch\":";Integer(out,s.life.spawnEpoch);out+=" ,\"life_token\":";Integer(out,s.life.token);
    out+=" ,\"requested_intent_serial\":";Integer(out,s.requestedIntents);
    out+=" ,\"requested_flags\":{\"jump\":";Bool(out,s.lastIntentJump);out+=" ,\"crouch\":";Bool(out,s.lastIntentCrouch);
    out+=" ,\"walk\":";Bool(out,s.lastIntentWalk);out+=" ,\"release\":";Bool(out,s.lastIntentRelease);out+='}';
    out+=" ,\"plan\":";Plan(out,p);out+=" ,\"last_ended_plan\":";
    if(ended.known&&(!p.known||ended.firstIntentSerial!=p.firstIntentSerial))Plan(out,ended);else out+="null";
    const bool ownKnown=own&&own->valid&&dto::math::Finite(own->feet)&&dto::math::Finite(own->nowSeconds)
        &&dto::math::Finite(own->sampledAtSeconds)&&own->sampledAtSeconds>=0&&own->sampledAtSeconds<=own->nowSeconds
        &&own->nowSeconds-own->sampledAtSeconds<=humanlike_bot_config::Get(source.brain->Profile(),humanlike_bot_config::Param::OwnSnapshotMaxAgeMs)*.001;
    out+=" ,\"current_own_sample\":{\"known\":";Bool(out,ownKnown);
    out+=" ,\"life_matches_plan_owner\":";Bool(out,own&&dto::math::Same(own->life,s.life));
    out+=" ,\"epoch\":";Integer(out,own?own->life.spawnEpoch:0);out+=" ,\"life_token\":";Integer(out,own?own->life.token:0);
    out+=" ,\"now\":";if(own)Number(out,own->nowSeconds,6);else out+="null";
    out+=" ,\"sampled_at\":";if(ownKnown)Number(out,own->sampledAtSeconds,6);else out+="null";
    out+=" ,\"feet\":";if(own)Vec(out,own->feet,ownKnown);else out+="null";
    out+=" ,\"ground_known\":";Bool(out,ownKnown&&own->groundedKnown);out+=" ,\"grounded\":";Bool(out,ownKnown&&own->groundedKnown&&own->grounded);out+='}';
    Combat(out,*source.brain,own);
    std::array<dto::Trace,TraceTailCapacity> tail{};uint64_t missed=0;size_t count=0;
    // Favor the bounded last-ended plan for acceptance when a later current
    // plan has already begun. The explicit serial scope prevents cross-plan
    // trace attribution; this is still a tail, never a complete trace archive.
    const auto&scope=ended.known?ended:p;
    if(scope.known&&scope.lastTraceSerial){const auto after=scope.lastTraceSerial>TraceTailCapacity?scope.lastTraceSerial-TraceTailCapacity:0;
        count=source.brain->CopyTrace(after,tail.data(),tail.size(),missed);}
    out+=" ,\"trace_plan_first_intent_serial\":";Integer(out,scope.firstIntentSerial);
    out+=" ,\"trace_overwritten_total\":";Integer(out,s.traceOverwritten);out+=" ,\"tail_missed\":";Integer(out,missed);out+=" ,\"plan_trace_tail\":[";
    bool comma=false;
    for(size_t n=0;n<count;++n){const auto&t=tail[n];
        if(t.serial<scope.firstTraceSerial||t.serial>scope.lastTraceSerial||t.requestedIntentSerial<scope.firstIntentSerial||t.requestedIntentSerial>scope.lastIntentSerial)continue;
        if(comma)out+=',';comma=true;out+="{\"serial\":";Integer(out,t.serial);
        out+=" ,\"at\":";if(t.timeKnown)Number(out,t.atSeconds,6);else out+="null";
        out+=" ,\"request_serial\":";Integer(out,t.requestedIntentSerial);out+=" ,\"consumed_serial\":";Integer(out,t.consumedIntentSerial);
        out+=" ,\"peek\":";Integer(out,static_cast<unsigned>(t.peek));out+=" ,\"motion\":";Integer(out,static_cast<unsigned>(t.motion));
        out+=" ,\"reason\":";Integer(out,static_cast<unsigned>(t.reason));out+=" ,\"input_requested\":";Bool(out,t.inputRequested);
        out+=" ,\"native_execution_observed\":";Bool(out,t.nativeExecutionObserved);out+='}';
    }
    out+="]}";
    if(out.size()-begin<=MaxBotBytes)return false;
    out.resize(begin);out+="{\"index\":";Integer(out,index);out+=" ,\"truncated\":true,\"plan\":{\"known\":false}}";return true;
}
template<class Reader>inline void Cache(double now,size_t profileCount,Reader reader){
    std::string next="{\"schema_version\":1,\"cached\":true,\"cache_interval_seconds\":0.5,\"sampled_at\":";Number(next,now,6);
    next+=" ,\"trace_tail_capacity\":2,\"max_bot_bytes\":";Integer(next,MaxBotBytes);
    next+=",\"coordinates\":\"world_feet_plan_and_own_sample\",\"native_position_reference\":\"npc_authority.entries.position_after_is_root_center\",\"new_native_ack_produced\":false,\"roster_clamped\":";
    Bool(next,profileCount>MaxRosterProfiles);next+=" ,\"bots\":[";bool comma=false;size_t truncated=0;
    const auto count=(std::min)(profileCount,MaxRosterProfiles);
    for(size_t index=1;index<count;++index){if(comma)next+=',';comma=true;truncated+=AppendBot(next,index,reader(index))?1U:0U;}
    next+="] ,\"truncated_bots\":";Integer(next,truncated);next+='}';
    AcquireSRWLockExclusive(&cacheLock);cached.swap(next);ReleaseSRWLockExclusive(&cacheLock);
}
inline std::string Json(){
    struct Lock{Lock(){AcquireSRWLockShared(&cacheLock);}~Lock(){ReleaseSRWLockShared(&cacheLock);}} lock;
    return cached;
}
} // namespace local_server_humanlike_peek_diagnostics
