#pragma once
#include "local_server_peek_fixture_contract.h"
#include "humanlike_bot_brain.h"
#include "local_server_npc_input.h"
#include <type_traits>

// A bounded copy at the existing Decide boundary. No new timer, tick, native
// query, actor0 AI call, target point, fake contact or consumed-serial producer.
namespace local_server_peek_fixture {
namespace dto=humanlike_bot;namespace cfg=humanlike_bot_config;
struct Sample {
    uint64_t sequence=0,source=0,physics=0,captureGeneration=0,snapshotFrame=0,jumpCalls=0;
    uint64_t shotSerial=0,shotToken=0,shotBinding=0,shotOrigin=0,visibleNewEvents=0,visiblePrimes=0;
    dto::OwnSnapshot own{};dto::PeekPlanSnapshot plan{},ended{};
    engine::Vec before{},after{},velocity{};
    uint32_t reason=0;uint8_t stage=0,movementType=255;
    bool geometry=false,physicalValid=false,newPhysics=false,contactVisible=false,contactOccluded=false,derivedFire=false;
    bool memoryKnown=false,memoryVisible=false,targetCrouchHeld=false,pairedTargetShot=false,acquiredShotJoin=false;
    double memoryLastSeen=-1,shotAt=-1,nativeSubmittedAt=-1,nativeElapsed=-1,capsuleHalfHeight=0,capsuleRadius=0;float memoryConfidence=0;
};
static_assert(std::is_trivially_copyable<Sample>::value);
struct Status {
    uint64_t produced=0,drained=0,dropped=0,ioErrors=0,source=0,token=0,controlSequence=0;
    uint32_t epoch=0,error=0,controlAction=0;bool setup=false,failed=false,acquired=false,occluded=false;
    double acquiredAt=-1,occludedAt=-1,memoryAgeAtOcclusion=-1;
    const char*stage="peek_fixture_waiting_original_two_person_spawn";
};
inline SRWLOCK observerLock=SRWLOCK_INIT;
inline Status observed{};
inline std::array<Sample,512> samples{};
inline double lastOwnSample=-1,lastOwnNow=-1;
inline uint64_t previousPhysics=0,previousSignal=0;
inline float initialTargetHealth=-1,initialSutHealth=-1;
using SourceIdentity=std::array<uint64_t,19>;
inline SourceIdentity lockedIdentity{};
inline SourceIdentity Identity(const local_server_npc_input::Entry&p){
    const auto weak=[](auto value){return (static_cast<uint64_t>(static_cast<uint32_t>(value.index))<<32)|static_cast<uint32_t>(value.serial);};
    return {reinterpret_cast<uint64_t>(p.pc),reinterpret_cast<uint64_t>(p.ps),reinterpret_cast<uint64_t>(p.pawn),
        reinterpret_cast<uint64_t>(p.movement),reinterpret_cast<uint64_t>(p.root),reinterpret_cast<uint64_t>(p.input),
        reinterpret_cast<uint64_t>(p.inventory),reinterpret_cast<uint64_t>(p.stream),reinterpret_cast<uint64_t>(p.prediction),
        reinterpret_cast<uint64_t>(p.clock),reinterpret_cast<uint64_t>(p.timeShift),weak(p.pcId),weak(p.psId),weak(p.pawnId),
        weak(p.movementId),weak(p.rootId),weak(p.inputId),weak(p.inventoryId),weak(p.streamId)};
}
inline double Distance(const engine::Vec&p,const double*q){const double x=p.x-q[0],y=p.y-q[1],z=p.z-q[2];return std::sqrt(x*x+y*y+z*z);}
inline void PublishStatus(Status status){AcquireSRWLockExclusive(&observerLock);status.drained=observed.drained;status.ioErrors=observed.ioErrors;observed=status;ReleaseSRWLockExclusive(&observerLock);}
inline Status StatusCopy(){AcquireSRWLockShared(&observerLock);const auto copy=observed;ReleaseSRWLockShared(&observerLock);return copy;}
inline void Observe(size_t index,const dto::SensorFrame&frame,const dto::BrainSnapshot&brain,const dto::MemoryView&memory,const cfg::Profile&profile){
    if(index!=1||!context.configured||!context.host)return;
    auto status=StatusCopy();if(!Arm()){status.failed=true;status.error=context.error;status.stage=context.stage;PublishStatus(status);return;}
    if(!context.armed||status.failed)return;
    const auto*sut=local_server_match::Player(1);const auto*target=local_server_match::Player(0);const auto*physical=local_server_npc_input::Player(1);
    const auto&own=frame.self;const auto&c=context.contract;
    const auto fail=[&](uint32_t code,const char*stage){status.failed=true;status.error=code;status.stage=stage;status.controlAction=2;++status.controlSequence;PublishStatus(status);};
    const auto*humanProfile=human_bot_store::store.Find(0);const auto*sutProfile=human_bot_store::store.Find(1);
    if(!humanProfile||!sutProfile)return;
    if(humanProfile->profileFingerprint!=context.profileFingerprints[0]||sutProfile->profileFingerprint!=context.profileFingerprints[1]
       ||brain.revision!=c.revision){fail(7836,"peek_fixture_exact_consumer_profile_or_revision_changed");return;}
    if(!sut||!target||!physical||!own.valid){if(status.setup)fail(7830,"peek_fixture_original_own_source_lost");return;}
    const bool live=sut->present&&target->present&&sut->alive&&target->alive&&sut->epoch==1&&target->epoch==1
        &&physical->pawn==sut->pawn&&physical->pc==sut->pc&&physical->ps==sut->ps&&physical->epoch==sut->epoch&&own.life.spawnEpoch==sut->epoch
        &&own.life.stableBotId==1&&brain.life.stableBotId==1&&brain.life.spawnEpoch==own.life.spawnEpoch&&brain.life.token==own.life.token
        &&physical->pawnId.index==sut->pawnIdentity.index&&physical->pawnId.serial==sut->pawnIdentity.serial;
    if(!live){if(status.setup)fail(7831,"peek_fixture_life_replacement_or_death_no_cross_life_completion");return;}
    const auto identity=Identity(*physical);const auto source=Hash(identity.data(),sizeof(identity));
    if(!status.setup){
        if(local_server_match::GetState().wantedPlayers!=2||!target->human||sut->human
           ||Distance(sut->spawnFeet,c.sut)>c.arrivalCm||Distance(target->spawnFeet,c.target)>c.arrivalCm
           ||Distance(sut->feet,c.sut)>c.arrivalCm||Distance(target->feet,c.target)>c.arrivalCm
           ||std::abs(std::remainder(training_bot_controller::actors[1].observed.rotation.y-c.sutYaw,360.))>.25){
            fail(7832,"peek_fixture_supported_spawn_shift_or_wrong_initial_scene");return;
        }
        status.setup=true;status.source=source;status.epoch=own.life.spawnEpoch;status.token=own.life.token;
        lockedIdentity=identity;
        initialTargetHealth=target->health;initialSutHealth=sut->health;
        status.stage="peek_fixture_original_spawn_bound_acceptance_pending";
    }else if(identity!=lockedIdentity||source!=status.source||own.life.spawnEpoch!=status.epoch||own.life.token!=status.token){
        fail(7833,"peek_fixture_original_source_identity_changed");return;
    }
    if((initialTargetHealth>0&&target->health<initialTargetHealth)||(initialSutHealth>0&&sut->health<initialSutHealth)){
        fail(7834,"peek_fixture_unexpected_damage_no_health_write_or_inferred_success");return;
    }
    if(previousPhysics&&physical->physicsCompleted<previousPhysics){fail(7837,"peek_fixture_same_source_original_physics_counter_regressed");return;}
    const auto interval=cfg::Get(profile,cfg::Param::OwnSampleMs)*.001;
    if(!std::isfinite(own.nowSeconds)||!std::isfinite(own.sampledAtSeconds)||own.sampledAtSeconds<0||own.sampledAtSeconds>own.nowSeconds)return;
    if(own.nowSeconds-own.sampledAtSeconds>cfg::Get(profile,cfg::Param::OwnSnapshotMaxAgeMs)*.001){fail(7838,"peek_fixture_original_own_sample_stale_not_covered");return;}
    if(lastOwnSample>=0&&(own.nowSeconds<lastOwnNow||own.sampledAtSeconds<lastOwnSample)){fail(7839,"peek_fixture_same_life_own_clock_regressed");return;}
    if(lastOwnSample>=0&&(own.sampledAtSeconds<=lastOwnSample||own.nowSeconds<lastOwnNow+interval))return;
    lastOwnSample=own.sampledAtSeconds;lastOwnNow=own.nowSeconds;
    Sample sample;sample.sequence=status.produced+1;sample.source=source;sample.own=own;sample.plan=brain.peekPlan;sample.ended=brain.lastEndedPeekPlan;
    sample.reason=static_cast<uint32_t>(brain.reason);sample.stage=static_cast<uint8_t>(brain.motion);
    sample.physics=physical->physicsCompleted;sample.captureGeneration=physical->captureGeneration;sample.snapshotFrame=physical->snapshotFrame;
    sample.jumpCalls=physical->jumpCalls;sample.before=physical->positionBefore;sample.after=physical->positionAfter;sample.velocity=physical->velocityAfter;
    sample.nativeSubmittedAt=physical->submittedAt;sample.nativeElapsed=local_server_npc_input::GetState().elapsed;
    sample.physicalValid=physical->rootPhysicsDispatched&&physical->rootPhysicsClassValid&&physical->error==0;
    sample.newPhysics=physical->physicsCompleted>previousPhysics;previousPhysics=physical->physicsCompleted;
    const auto&original=training_bot_controller::actors[1].observed;
    sample.movementType=original.speedProbeReady?original.movementType:255;
    sample.capsuleHalfHeight=original.capsuleHalfHeight;sample.capsuleRadius=original.capsuleRadius;
    sample.geometry=own.groundedKnown&&own.grounded&&frame.space.coverKnown&&frame.space.clearanceKnown
        &&frame.space.peekPathProven&&frame.space.retreatPathProven&&frame.space.groundProven;
    sample.memoryKnown=memory.used&&!memory.retired;sample.memoryVisible=sample.memoryKnown&&memory.visible;
    sample.memoryLastSeen=sample.memoryKnown?memory.lastSeenSeconds:-1;sample.memoryConfidence=sample.memoryKnown?memory.confidence:0;
    for(size_t n=0;n<frame.contactCount&&n<frame.contacts.size();++n){const auto&contact=frame.contacts[n];
        if(contact.track!=memory.track)continue;
        sample.contactVisible=contact.permitted&&contact.recognizedBySensor&&contact.visibility==dto::Visibility::Visible;
        sample.contactOccluded=contact.visibilitySamplesKnown&&contact.visibility==dto::Visibility::Occluded;
        sample.derivedFire=sample.contactVisible&&contact.firingVisuallyObserved;
    }
    const auto&visibleStats=local_server_humanlike_sensors::provider.Stats();
    sample.visibleNewEvents=visibleStats.visibleShotNewEvents;sample.visiblePrimes=visibleStats.visibleShotPrimes;
    const auto*shot=humanlike_bot_native_events::LatestShot(0);
    if(shot&&shot->subject.index==0&&shot->subject.epoch==target->epoch&&shot->subject.pawn==reinterpret_cast<uintptr_t>(target->pawn)
       &&shot->subject.weakIndex==target->pawnIdentity.index&&shot->subject.weakSerial==target->pawnIdentity.serial){
        sample.shotSerial=shot->serial;sample.shotToken=shot->token;sample.shotBinding=shot->binding;sample.shotOrigin=shot->origin;sample.shotAt=shot->atMatchSeconds;
        sample.pairedTargetShot=shot->serial&&shot->token&&shot->binding&&shot->origin&&shot->actualGeometryPaired;
        dto::TrackId track=0;
        sample.acquiredShotJoin=sample.pairedTargetShot&&local_server_humanlike_sensors::provider.AcquiredVisibleTrack(1,shot->subject,
            shot->atMatchSeconds,own.nowSeconds,cfg::Get(profile,cfg::Param::VisualMaxSampleGapMs)*.001,track)&&track==memory.track;
    }
    const auto&human=training_bot_controller::actors[0].observed;
    sample.targetCrouchHeld=human.ready&&human.pawn==target->pawn&&human.spawnEpoch==target->epoch&&human.crouched;
    if(c.kind==8&&sample.geometry&&sample.memoryKnown&&sample.memoryVisible&&sample.contactVisible&&!status.acquired){
        status.acquired=true;status.acquiredAt=own.nowSeconds;status.controlAction=1;++status.controlSequence;
        status.stage="peek_fixture_real_acquired_contact_requesting_original_target_crouch";
    }
    if(c.kind==8&&status.acquired&&sample.memoryKnown&&!sample.memoryVisible&&sample.contactOccluded&&sample.targetCrouchHeld&&!status.occluded){
        status.occluded=true;status.occludedAt=own.nowSeconds;status.memoryAgeAtOcclusion=own.nowSeconds-sample.memoryLastSeen;
        status.stage=status.memoryAgeAtOcclusion>=0&&status.memoryAgeAtOcclusion<=cfg::Get(profile,cfg::Param::MemoryLostHoldMs)*.001
            ?"peek_fixture_real_occluded_contact_recent_memory_acceptance_pending":"peek_fixture_real_occlusion_after_original_memory_expired_not_covered";
    }
    AcquireSRWLockExclusive(&observerLock);
    if(status.produced-observed.drained>=samples.size()){++status.dropped;status.failed=true;status.error=7835;status.stage="peek_fixture_bounded_sample_overrun_unknown_result";}
    else{samples[status.produced%samples.size()]=sample;++status.produced;}
    status.drained=observed.drained;status.ioErrors=observed.ioErrors;observed=status;ReleaseSRWLockExclusive(&observerLock);
}
inline const char*Bool(bool value){return value?"true":"false";}
inline void Number(FILE*f,double value){if(std::isfinite(value))std::fprintf(f,"%.9g",value);else std::fputs("null",f);}
template<class V>inline void Vec(FILE*f,const V&v){std::fputc('[',f);Number(f,v.x);std::fputc(',',f);Number(f,v.y);std::fputc(',',f);Number(f,v.z);std::fputc(']',f);}
inline void Plan(FILE*f,const dto::PeekPlanSnapshot&p){
    std::fprintf(f,"{\"known\":%s,\"active\":%s,\"kind\":%u,\"first_intent\":%llu,\"last_intent\":%llu,\"stage_mask\":%u,\"started_at\":",Bool(p.known),Bool(p.active),unsigned(p.peek),static_cast<unsigned long long>(p.firstIntentSerial),static_cast<unsigned long long>(p.lastIntentSerial),unsigned(p.stageMaskAtNote));Number(f,p.startedAtSeconds);
    std::fputs(",\"stage_at\":",f);Number(f,p.stageAtSeconds);std::fputs(",\"ended_at\":",f);Number(f,p.endedAtSeconds);
    std::fputs(",\"start_feet\":",f);Vec(f,p.start);std::fputs(",\"goal_feet\":",f);Vec(f,p.goal);std::fputs(",\"retreat_feet\":",f);Vec(f,p.retreat);
    std::fprintf(f,",\"terminal_known\":%s,\"terminal_sampled_at\":",Bool(p.terminalOwnKnown));Number(f,p.terminalOwnSampledAtSeconds);std::fputs(",\"terminal_feet\":",f);Vec(f,p.terminalOwnFeet);
    std::fprintf(f,",\"terminal_ground_known\":%s,\"terminal_grounded\":%s,\"jump_requested\":%s,\"air_seen\":%s,\"landed\":%s,\"crouch_seen\":%s,\"information_seen\":%s,\"return_chosen\":%s,\"poppin_extended\":%s}",Bool(p.terminalGroundKnown),Bool(p.terminalGrounded),Bool(p.jumpRequested),Bool(p.airSeen),Bool(p.landed),Bool(p.crouchSeen),Bool(p.informationSeen),Bool(p.returnChosen),Bool(p.poppinExtended));
}
inline void WriteSample(FILE*f,const Sample&s){
    const auto&c=context.contract;std::fprintf(f,"{\"version\":1,\"scenario\":13,\"nonce\":\"%016llx\",\"case_hash\":\"%016llx\",\"sequence\":%llu,\"index\":1,\"epoch\":%u,\"life_token\":%llu,\"source\":\"%016llx\",\"now\":",static_cast<unsigned long long>(c.nonce),static_cast<unsigned long long>(c.caseHash),static_cast<unsigned long long>(s.sequence),s.own.life.spawnEpoch,static_cast<unsigned long long>(s.own.life.token),static_cast<unsigned long long>(s.source));Number(f,s.own.nowSeconds);
    std::fputs(",\"own_sampled_at\":",f);Number(f,s.own.sampledAtSeconds);std::fputs(",\"own_feet\":",f);Vec(f,s.own.feet);std::fputs(",\"own_velocity\":",f);Vec(f,s.own.velocity);
    std::fprintf(f,",\"own_view_known\":%s,\"own_view_pitch_deg\":",Bool(s.own.viewValid));Number(f,s.own.view.pitch);
    std::fputs(",\"own_view_yaw_deg\":",f);Number(f,s.own.view.yaw);
    std::fprintf(f,",\"view_sample_time_known\":%s,\"view_sampled_at\":",Bool(s.own.viewSampleTimeKnown));
    if(s.own.viewSampleTimeKnown)Number(f,s.own.viewSampledAtSeconds);else std::fputs("null",f);
    std::fprintf(f,",\"ground_known\":%s,\"grounded\":%s,\"stance_known\":%s,\"crouched\":%s,\"gait_known\":%s,\"walking\":%s,\"native_movement_type\":%u,\"geometry\":%s,\"stage\":%u,\"reason\":%u,\"native_max_speed\":",Bool(s.own.groundedKnown),Bool(s.own.grounded),Bool(s.own.stanceValid),Bool(s.own.crouched),Bool(s.own.movementValid),Bool(s.own.walking),unsigned(s.movementType),Bool(s.geometry),unsigned(s.stage),s.reason);Number(f,s.own.nativeMaxSpeedCmS);
    std::fprintf(f,",\"physics_completed\":%llu,\"capture_generation\":%llu,\"snapshot_frame\":%llu,\"jump_calls\":%llu,\"new_physics\":%s,\"physical_valid\":%s,\"root_before\":",static_cast<unsigned long long>(s.physics),static_cast<unsigned long long>(s.captureGeneration),static_cast<unsigned long long>(s.snapshotFrame),static_cast<unsigned long long>(s.jumpCalls),Bool(s.newPhysics),Bool(s.physicalValid));Vec(f,s.before);std::fputs(",\"root_after\":",f);Vec(f,s.after);std::fputs(",\"native_velocity\":",f);Vec(f,s.velocity);
    std::fputs(",\"native_submitted_at\":",f);Number(f,s.nativeSubmittedAt);std::fputs(",\"native_elapsed\":",f);Number(f,s.nativeElapsed);
    std::fputs(",\"original_capsule_half_height\":",f);Number(f,s.capsuleHalfHeight);std::fputs(",\"original_capsule_radius\":",f);Number(f,s.capsuleRadius);
    std::fprintf(f,",\"memory_known\":%s,\"memory_visible\":%s,\"contact_visible\":%s,\"contact_occluded\":%s,\"derived_native_fire\":%s,\"target_crouch_held\":%s,\"memory_last_seen\":",Bool(s.memoryKnown),Bool(s.memoryVisible),Bool(s.contactVisible),Bool(s.contactOccluded),Bool(s.derivedFire),Bool(s.targetCrouchHeld));Number(f,s.memoryLastSeen);std::fputs(",\"memory_confidence\":",f);Number(f,s.memoryConfidence);
    std::fprintf(f,",\"paired_target0_shot\":%s,\"acquired_visible_shot_join\":%s,\"shot_serial\":%llu,\"shot_token\":%llu,\"shot_binding\":%llu,\"shot_origin\":%llu,\"producer_new_events\":%llu,\"producer_primes\":%llu,\"shot_at\":",Bool(s.pairedTargetShot),Bool(s.acquiredShotJoin),static_cast<unsigned long long>(s.shotSerial),static_cast<unsigned long long>(s.shotToken),static_cast<unsigned long long>(s.shotBinding),static_cast<unsigned long long>(s.shotOrigin),static_cast<unsigned long long>(s.visibleNewEvents),static_cast<unsigned long long>(s.visiblePrimes));Number(f,s.shotAt);
    std::fputs(",\"plan\":",f);Plan(f,s.plan);std::fputs(",\"last_ended_plan\":",f);Plan(f,s.ended);
    std::fputs(",\"per_intent_consumption_known\":false,\"consumed_serial\":0}\n",f);
}
inline void WriteStatus(FILE*f){
    const auto s=StatusCopy();std::fprintf(f,"{\"configured\":%s,\"scenario\":13,\"setup_valid\":%s,\"failed\":%s,\"error\":%u,\"stage\":\"%s\",\"produced\":%llu,\"drained\":%llu,\"dropped\":%llu,\"io_errors\":%llu,\"real_acquired\":%s,\"real_occluded\":%s,\"acceptance_claimed\":false,\"per_intent_consumption_known\":false}",Bool(context.configured),Bool(s.setup),Bool(s.failed),s.error,s.stage,static_cast<unsigned long long>(s.produced),static_cast<unsigned long long>(s.drained),static_cast<unsigned long long>(s.dropped),static_cast<unsigned long long>(s.ioErrors),Bool(s.acquired),Bool(s.occluded));
}
inline void Drain(){
    if(!context.configured||!context.host)return;auto status=StatusCopy();
    if(status.controlSequence&&status.controlSequence!=previousSignal){
        Control value;value.nonce=context.contract.nonce;value.sequence=status.controlSequence;value.action=status.controlAction;
        const auto temporary=context.signalPath+L".tmp";FILE*f=nullptr;
        bool okay=_wfopen_s(&f,temporary.c_str(),L"wb")==0&&f;
        if(okay){okay=std::fwrite(&value,1,sizeof(value),f)==sizeof(value);okay=std::fclose(f)==0&&okay;}
        if(okay)okay=MoveFileExW(temporary.c_str(),context.signalPath.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
        if(okay)previousSignal=status.controlSequence;else{AcquireSRWLockExclusive(&observerLock);++observed.ioErrors;ReleaseSRWLockExclusive(&observerLock);}
    }
    if(status.drained>=status.produced)return;
    std::array<Sample,32>batch{};size_t count=0;
    AcquireSRWLockShared(&observerLock);
    const auto first=observed.drained;while(count<batch.size()&&first+count<observed.produced){batch[count]=samples[(first+count)%samples.size()];++count;}
    ReleaseSRWLockShared(&observerLock);
    FILE*f=nullptr;const auto path=context.directory+L"peek-fixture-own-native.jsonl";
    bool okay=_wfopen_s(&f,path.c_str(),L"ab")==0&&f;
    if(okay){for(size_t n=0;n<count;++n)WriteSample(f,batch[n]);okay=std::ferror(f)==0;okay=std::fclose(f)==0&&okay;}
    AcquireSRWLockExclusive(&observerLock);if(okay)observed.drained=first+count;else ++observed.ioErrors;ReleaseSRWLockExclusive(&observerLock);
}
} // namespace local_server_peek_fixture
