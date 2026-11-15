#pragma once
#include "local_server_hold_fixture_contract.h"
#include "local_server_humanlike_space.h"
#include "humanlike_bot_brain.h"
#include "human_bot_store.h"

// Explicit Scenario14 only. Queries run in the existing own decision pass;
// Brain::Tick and the existing SubmitBot remain the only input producer/path.
namespace local_server_hold_fixture {
namespace dto=humanlike_bot;namespace cfg=humanlike_bot_config;
namespace space=local_server_humanlike_space;namespace data=humanlike_bot_space_data;
namespace sensors=local_server_humanlike_sensors;
inline core::Point Point(dto::Vec3 p){return {p.x,p.y,p.z};}
inline core::Point Point(engine::Vec p){return {p.x,p.y,p.z};}
inline engine::Vec Native(core::Point p){return {p.x,p.y,p.z};}
inline dto::Vec3 Vector(core::Point p){return {static_cast<float>(p.x),static_cast<float>(p.y),static_cast<float>(p.z)};}
using Identity=std::array<uint64_t,19>;
enum class Readiness:uint8_t {Wait,Ready,Rejected};
enum Block:uint32_t {MatchStarting=1,OwnUnknown=2,ObservationUnknown=4,SpeedUnknown=8,CapsuleUnknown=16,EyeUnknown=32,GroundUnknown=64,KnownInvalid=128,Paused=256};
struct StartupProbe {
    bool matchActive=false,configuredPaused=false,ownPaused=false,ownValid=false,ownAlive=false,ownRetired=false;
    bool observationReady=false,speedReady=false,capsuleKnown=false,eyeKnown=false,eyeFinite=false,groundKnown=false,grounded=false;
    double radius=0,half=0,eyeHeight=0,at=-1;
};
inline Readiness StartupReadiness(const StartupProbe& p,bool started,uint32_t& mask){
    mask=0;
    if(p.capsuleKnown&&(!std::isfinite(p.radius)||!std::isfinite(p.half)||p.radius<=0||p.half<p.radius))mask|=KnownInvalid;
    if(p.eyeKnown&&(!p.eyeFinite||!std::isfinite(p.eyeHeight)||p.eyeHeight<=0))mask|=KnownInvalid;
    if(p.configuredPaused||p.ownRetired||!p.ownAlive)mask|=Paused;
    if(started&&(!p.matchActive||p.ownPaused))mask|=Paused;
    if(mask)return Readiness::Rejected;
    if(!p.matchActive||p.ownPaused)mask|=MatchStarting;
    if(!p.ownValid)mask|=OwnUnknown;
    if(!p.observationReady)mask|=ObservationUnknown;
    if(!p.speedReady)mask|=SpeedUnknown;
    if(!p.capsuleKnown)mask|=CapsuleUnknown;
    if(!p.eyeKnown)mask|=EyeUnknown;
    if(!p.groundKnown||!p.grounded)mask|=GroundUnknown;
    if(started&&mask)return Readiness::Rejected;
    return mask?Readiness::Wait:Readiness::Ready;
}
inline Identity Source(const local_server_npc_input::Entry& p){
    const auto weak=[](auto v){return (uint64_t(uint32_t(v.index))<<32)|uint32_t(v.serial);};
    return {reinterpret_cast<uint64_t>(p.pc),reinterpret_cast<uint64_t>(p.ps),reinterpret_cast<uint64_t>(p.pawn),
        reinterpret_cast<uint64_t>(p.movement),reinterpret_cast<uint64_t>(p.root),reinterpret_cast<uint64_t>(p.input),
        reinterpret_cast<uint64_t>(p.inventory),reinterpret_cast<uint64_t>(p.stream),reinterpret_cast<uint64_t>(p.prediction),
        reinterpret_cast<uint64_t>(p.clock),reinterpret_cast<uint64_t>(p.timeShift),weak(p.pcId),weak(p.psId),weak(p.pawnId),
        weak(p.movementId),weak(p.rootId),weak(p.inputId),weak(p.inventoryId),weak(p.streamId)};
}
struct Status {
    bool configured=false,armed=false,setup=false,failed=false,notObserved=true,installed=false,restored=false;
    bool brainReleased=false,dwelled=false,releaseSubmitted=false,releaseUnavailable=false,routeNativePath=false;
    uint32_t error=0,annotation=0,revision=0;uint64_t publication=0,produced=0,drained=0,dropped=0,ioErrors=0;
    uint64_t startedMs=0;double lastSampledAt=-1;
    uint64_t nonce=0,caseHash=0,releasePhysics=0,releaseSubmissions=0;double releaseAt=-1;
    core::Point releaseFeet{},releaseDirection{};Identity sourceIdentity{};
    core::Configuration configuration{};core::Preflight preflight{};core::Acceptance acceptance{};
    uint64_t routeGeneration=0;uint32_t routePathCount=0;
    std::array<core::Point,64> routePath{};
    StartupProbe startup{};uint64_t readinessWaits=0,firstFailureAtMs=0;
    uint32_t firstFailureCode=0,firstFailurePreflightStage=0,firstFailureGuardMask=0;
    uint32_t guardMask=0,readinessBlockMask=0,lastBlockMask=0,lastBlockPreflightStage=0,lastAttemptedPreflightStage=0;
    const char* firstFailureStage="not_observed",*lastBlockStage="not_observed";
    const char* stage="map_hold_not_observed_before_first_owned_sample";
};
struct Record {uint64_t sequence=0;core::Sample own{};engine::Vec rootBefore{},rootAfter{},consumedInput{};
    uint64_t capture=0,snapshotFrame=0;uint32_t reason=0,activity=0;bool pathProven=false,holdProof=false;
    double capsuleHalfHeight=0,capsuleRadius=0,eyeHeight=0;};
static_assert(std::is_trivially_copyable_v<Status>&&std::is_trivially_copyable_v<Record>);
inline Status live{},cached{};inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline SRWLOCK drainLock=SRWLOCK_INIT;
inline std::array<Record,512> records{};
inline Identity identity{};inline data::Annotation proposal{};
inline std::array<data::Annotation,data::Capacity> backup{};inline size_t backupCount=0;inline uint32_t backupRevision=0;
inline uint64_t completionWritten=0;
inline void Publish(){
    live.configured=context.configured;live.armed=context.armed;++live.publication;
    live.nonce=context.contract.nonce;live.caseHash=context.contract.caseHash;live.sourceIdentity=identity;
    AcquireSRWLockExclusive(&cacheLock);live.drained=cached.drained;live.ioErrors=cached.ioErrors;cached=live;ReleaseSRWLockExclusive(&cacheLock);
}
inline Status Copy(){AcquireSRWLockShared(&cacheLock);const auto result=cached;ReleaseSRWLockShared(&cacheLock);return result;}
inline bool Fence(core::Life life){
    const auto* p=local_server_match::Player(1);const auto* native=local_server_npc_input::Player(1);
    return original_players::Thread()&&context.armed&&!context.failed&&space::Context()&&p&&native&&p->present&&!p->human&&p->alive
        &&p->epoch==life.epoch&&native->epoch==p->epoch&&native->present&&native->alive&&native->ready&&!native->error
        &&native->owner!=local_server_npc_input::Owner::Unknown&&native->pc==p->pc&&native->ps==p->ps&&native->pawn==p->pawn
        &&native->pawnId.index==p->pawnIdentity.index&&native->pawnId.serial==p->pawnIdentity.serial
        &&Source(*native)==identity&&Hash(identity.data(),sizeof(identity))==life.source&&space::LocalSource(1,{life.id,life.epoch,life.token});
}
inline void Restore(){
    if(!live.installed||live.restored)return;
    // Never overwrite a later user/fixture generation during interruption.
    if(space::state.revision==live.revision&&space::SetAnnotations(backup.data(),backupCount,backupRevision))live.restored=true;
    else live.releaseUnavailable=true;
}
inline void NoteFailure(uint32_t code,const char* reason){
    if(!live.firstFailureCode){live.firstFailureCode=code;live.firstFailureStage=reason;live.firstFailureAtMs=GetTickCount64();
        live.firstFailurePreflightStage=live.preflight.stage==core::Stage::Rejected?live.lastAttemptedPreflightStage:unsigned(live.preflight.stage);
        live.firstFailureGuardMask=live.guardMask;}
    live.failed=live.notObserved=true;live.error=code;live.stage=reason;
}
inline void Blocked(const char* reason,uint32_t mask,uint32_t preflightStage){
    live.lastBlockStage=reason;live.lastBlockMask=mask;live.lastBlockPreflightStage=preflightStage;live.stage=reason;
}
inline void Abort(uint32_t code,const char* reason,dto::Brain& brain){
    NoteFailure(code,reason);brain.SetEnabled(false);Restore();
    if(!live.setup)live.releaseUnavailable=true;
}
inline bool OwnsInterruption(size_t index){return index==1&&context.configured&&context.host&&live.setup;}
inline void GroupDisabled(size_t index,dto::Brain& brain){
    if(!OwnsInterruption(index))return;
    if(!live.failed)Abort(7888,"map_hold_group_disabled_original_Brain_release_pending",brain);
    else brain.SetEnabled(false);Publish();
}
// Explicit fixture proposals from scene007 rows319/637. Historical
// local geometry is provenance only: all current-run proof flags stay unknown.
// This row is never added to data::Authored or the production map JSON.
inline bool TestOnlyProposal(uint32_t id,data::Annotation& row){
    if(id!=1303&&id!=1304)return false;row={};row.id=id;std::memcpy(row.mapId,"ascent",7);
    row.kind=data::Kind::Hold;row.conventional=false;row.authoredConfidence=.5f;
    row.start={-2710.22265625f,-3945.1005859375f,102.15003967285156f};
    row.open={-2417.2763671875f,-4374.73095703125f,102.15003967285156f};
    row.retreat=row.start;
    row.focus={-2587.116943359375f,-3861.16015625f,276.1500244140625f};
    if(id==1304){row.conventional=true;
        row.start={4720.66845703125f,-4450.54443359375f,402.2984924316406f};
        row.open={4448.9013671875f,-4893.8759765625f,402.2984924316406f};
        row.retreat=row.start;
        row.focus={4593.63720703125f,-4372.6728515625f,576.2984619140625f};}
    row.focusKnown=true; // Exact static focus proposal, not a current LOS proof.
    return true;
}
struct API {
    const dto::OwnSnapshot& own;const cfg::Profile& profile;
    bool Fence(core::Life life){return local_server_hold_fixture::Fence(life);}
    core::Query Path(core::Point goal,std::array<core::Point,64>& out,uint32_t& count){
        if(space::state.queryBudget<1)return core::Query::Deferred;
        space::Cache temporary{};temporary.goal=Native(goal);space::PathAttempt attempt{};
        if(!space::Path(1,temporary,own,profile,&attempt)||!attempt.queryExecuted||temporary.pathCount<2)return core::Query::Rejected;
        count=static_cast<uint32_t>(temporary.pathCount);for(size_t n=0;n<temporary.pathCount;++n)out[n]=Point(temporary.path[n]);return core::Query::Passed;
    }
    core::Query Ground(core::Point expected,core::Point& actual){
        if(!space::Spend(1,0))return core::Query::Deferred;engine::Vec feet{};
        if(!training_world_geometry::Ground(Native(expected),feet))return core::Query::Rejected;
        actual=Point(feet);return core::Query::Passed;
    }
    core::Query Segment(core::Point from,core::Point to,double radius,double half){
        const auto cost=space::GroundCost(Native(from),Native(to),radius);
        if(space::state.groundBudget<cost||space::state.queryBudget<1)return core::Query::Deferred;
        return space::Segment(1,Native(from),Native(to),radius,half)?core::Query::Passed:core::Query::Rejected;
    }
    core::Query Trace(core::Point from,core::Point to,core::Ray& result){
        if(!space::Spend(0,1))return core::Query::Deferred;
        const auto ray=sensors::WorldTrace(1,Vector(from),Vector(to),nullptr);++space::state.sightProbes;
        result=ray==humanlike_bot_sensors::Ray::Clear?core::Ray::Clear:ray==humanlike_bot_sensors::Ray::Blocked?core::Ray::Blocked:core::Ray::Unknown;
        return core::Query::Passed;
    }
};
inline bool Bind(const dto::SensorFrame& frame,const cfg::Profile& profile,dto::Brain& brain){
    const auto* p=local_server_match::Player(1);const auto* human=local_server_match::Player(0);const auto* native=local_server_npc_input::Player(1);
    const auto& own=frame.self;const auto& c=context.contract;
    if(!p||!human||!native||!own.valid||!own.alive||!own.groundedKnown||!own.grounded||!native->ready||native->error)return false;
    const auto* zero=human_bot_store::store.Find(0);const auto* one=human_bot_store::store.Find(1);
    if(!zero||!one||zero->profileFingerprint!=context.fingerprints[0]||one->profileFingerprint!=context.fingerprints[1]
       ||brain.Snapshot().revision!=c.revision){Abort(7870,"map_hold_exact_profile_revision_changed",brain);return false;}
    if(local_server_match::GetState().wantedPlayers!=2||!human->human||p->human||p->epoch!=1||human->epoch!=1
       ||own.life.stableBotId!=1||own.life.spawnEpoch!=p->epoch||!own.life.token){Abort(7871,"map_hold_wrong_original_two_person_life",brain);return false;}
    identity=Source(*native);core::Life life{own.life.stableBotId,own.life.spawnEpoch,own.life.token,Hash(identity.data(),sizeof(identity))};
    if(!Fence(life)){Abort(7875,"map_hold_initial_own_source_fence_failed",brain);return false;}
    // Bind the real source before scene checks, so a rejected spawn can still
    // record the following normal Brain/Submit release for that same source.
    live.setup=true;live.preflight.life=life;live.annotation=c.annotation;live.revision=c.annotationRevision+1;
    const core::Point requested{c.sut[0],c.sut[1],c.sut[2]};
    if(!core::Arrived(Point(p->spawnFeet),requested,c.arrival)||!core::Arrived(Point(p->feet),requested,c.arrival)
       ||!core::Arrived(Point(human->spawnFeet),{c.human[0],c.human[1],c.human[2]},c.arrival)){
        Abort(7872,"map_hold_supported_spawn_shift_not_authored_start",brain);return false;}
    bool found=false;for(const auto& row:data::Authored)if(row.id==c.annotation&&row.kind==data::Kind::Hold&&!std::strcmp(row.mapId,"ascent")){
        if(row.enabled||row.riskKnown||row.flankRiskKnown||row.exposedAnglesKnown){Abort(7873,"map_hold_requires_disabled_unknown_source_proposal",brain);return false;}
        proposal=row;found=true;break;}
    if(c.annotation==1303||c.annotation==1304)found=TestOnlyProposal(c.annotation,proposal);
    if(!found||space::state.revision!=c.annotationRevision){Abort(7873,"map_hold_authored_proposal_or_revision_missing",brain);return false;}
    live.annotation=c.annotation;live.revision=c.annotationRevision+1;auto& config=live.configuration;
    config.annotation=c.annotation;config.revision=live.revision;config.conventional=proposal.conventional;config.start=requested;
    config.cover=Point(proposal.start);config.open=Point(proposal.open);config.retreat=Point(proposal.retreat);config.focus=Point(proposal.focus);
    config.arrival=c.arrival;config.dwell=c.dwell;config.maxSampleGap=cfg::Get(profile,cfg::Param::OwnSnapshotMaxAgeMs)*.001;
    config.riskReach=cfg::Get(profile,cfg::Param::CoverMaximumAnnotationDistanceCm);
    if((proposal.conventional?cfg::Get(profile,cfg::Param::HoldConventionalWeight):cfg::Get(profile,cfg::Param::HoldUnconventionalWeight))!=100
       ||(proposal.conventional?cfg::Get(profile,cfg::Param::HoldUnconventionalWeight):cfg::Get(profile,cfg::Param::HoldConventionalWeight))!=0
       ||cfg::Get(profile,cfg::Param::HoldDurationMs)*.001!=c.dwell||cfg::Get(profile,cfg::Param::MoveArrivalCm)!=c.arrival
       ||core::Arrived(requested,config.open,c.arrival)){Abort(7874,"map_hold_explicit_class_weight_or_real_travel_contract_rejected",brain);return false;}
    if(!Fence(life)){Abort(7875,"map_hold_initial_own_source_fence_failed",brain);return false;}
    live.setup=true;live.preflight.life=life;live.stage="map_hold_original_preflight_pending_shared_budget";return true;
}
inline void PrepareBody(size_t index,const dto::SensorFrame& frame,dto::Brain& brain,const cfg::Profile& profile){
    if(index!=1||!context.configured||!context.host)return;
    if(!live.startedMs)live.startedMs=GetTickCount64();
    if(!Arm()){Abort(context.error,context.stage,brain);return;}
    if(!context.armed){brain.SetEnabled(false);return;}
    if(live.failed||live.dwelled||stopRequested.load(std::memory_order_relaxed)){
        brain.SetEnabled(false);if(stopRequested.load(std::memory_order_relaxed)&&!live.dwelled)Abort(7876,"map_hold_explicit_stop_original_release_requested",brain);
        Restore();return;
    }
    brain.SetEnabled(live.brainReleased);
    if(GetTickCount64()-live.startedMs>=static_cast<uint64_t>(context.contract.total*1000)){Abort(7877,"map_hold_bounded_preflight_approach_or_dwell_timeout",brain);return;}
    if(!live.setup&&!Bind(frame,profile,brain))return;
    const auto& own=frame.self;const auto life=live.preflight.life;
    const auto* current=human_bot_store::store.Find(1);
    live.guardMask=0;
    if(!current)live.guardMask|=1;
    if(current&&current->profileFingerprint!=context.fingerprints[1])live.guardMask|=2;
    if(brain.Snapshot().revision!=context.contract.revision)live.guardMask|=4;
    if(!Fence(life))live.guardMask|=8;
    if(!own.alive||own.retired)live.guardMask|=16;
    if(own.life.stableBotId!=life.id||own.life.spawnEpoch!=life.epoch||own.life.token!=life.token)live.guardMask|=32;
    if(live.guardMask){
        Blocked("map_hold_source_life_profile_or_owner_guard_rejected",live.guardMask,unsigned(live.preflight.stage));
        Abort(7878,"map_hold_source_life_profile_pause_or_owner_interruption",brain);return;}
    const auto& observed=training_bot_controller::actors[1].observed;
    auto& probe=live.startup;probe={};probe.at=own.nowSeconds;
    probe.matchActive=local_server_match::Active();probe.configuredPaused=(local_server_match::Configuration().config.flags&training_control::Paused)!=0;
    probe.ownPaused=own.paused;probe.ownValid=own.valid;probe.ownAlive=own.alive;probe.ownRetired=own.retired;
    probe.observationReady=observed.ready;probe.speedReady=observed.speedProbeReady&&own.movementValid;
    probe.eyeKnown=own.valid&&own.eyeValid;probe.eyeFinite=dto::math::Finite(own.eye)&&dto::math::Finite(own.feet);
    probe.eyeHeight=own.eye.z-own.feet.z;probe.groundKnown=own.groundedKnown;probe.grounded=own.grounded;
    void* capsule=nullptr;float radius=0,half=0;
    probe.capsuleKnown=original_players::Read(observed.pawn,0x500,capsule)&&capsule
        &&original_players::Read(capsule,0x6dc,radius)&&original_players::Read(capsule,0x6d8,half);
    probe.radius=radius;probe.half=half;
    const auto ready=StartupReadiness(probe,live.preflight.bound||live.brainReleased,live.readinessBlockMask);
    if(ready==Readiness::Wait){++live.readinessWaits;brain.SetEnabled(false);
        Blocked("map_hold_waiting_real_match_own_dimensions_and_speed",live.readinessBlockMask,unsigned(live.preflight.stage));return;}
    if(ready==Readiness::Rejected){Blocked("map_hold_known_invalid_or_interrupted_readiness",live.readinessBlockMask,unsigned(live.preflight.stage));
        Abort(7878,"map_hold_known_invalid_or_interrupted_readiness",brain);return;}
    if(observed.capsuleRadius!=probe.radius||observed.capsuleHalfHeight!=probe.half){
        Blocked("map_hold_waiting_current_observation_capsule_sample",CapsuleUnknown,unsigned(live.preflight.stage));
        if(live.preflight.bound||live.brainReleased){Abort(7878,"map_hold_current_observation_capsule_changed",brain);return;}
        ++live.readinessWaits;brain.SetEnabled(false);return;
    }
    if(live.brainReleased)return;
    live.lastAttemptedPreflightStage=unsigned(live.preflight.stage);
    API api{own,profile};const auto result=core::Step(live.preflight,live.configuration,life,own.nowSeconds,
        own.eye.z-own.feet.z,observed.capsuleRadius,observed.capsuleHalfHeight,api);
    if(result==core::Query::Rejected){Blocked("map_hold_original_preflight_query_rejected",0,live.lastAttemptedPreflightStage);Abort(7879,"map_hold_original_preflight_rejected_or_unknown",brain);return;}
    if(result!=core::Query::Passed){if(unsigned(live.preflight.stage)==live.lastAttemptedPreflightStage)
            Blocked("map_hold_waiting_existing_shared_query_budget",0,live.lastAttemptedPreflightStage);
        else live.stage="map_hold_original_preflight_progressed";return;}
    if(live.preflight.exposed>static_cast<uint32_t>(cfg::Get(profile,cfg::Param::CoverMaximumExposedAngles))
       ||proposal.authoredConfidence<cfg::Get(profile,cfg::Param::CoverMinimumConfidence)){
        Abort(7880,"map_hold_measured_exposure_or_author_confidence_rejected_by_profile",brain);return;}
    backup=space::state.annotations;backupCount=space::state.count;backupRevision=space::state.revision;
    auto clone=backup;size_t cloneCount=backupCount;bool installed=false;
    for(size_t n=0;n<backupCount;++n)if(clone[n].kind==data::Kind::Hold){clone[n].enabled=false;
        // Reuse only an exact static-geometry alias in this test-local clone.
        const bool exactTestAlias=(live.annotation==1303||live.annotation==1304)&&!std::strcmp(clone[n].mapId,proposal.mapId)
            &&humanlike_bot_hold_positions::SamePoint(clone[n].start,proposal.start)&&humanlike_bot_hold_positions::SamePoint(clone[n].open,proposal.open)
            &&humanlike_bot_hold_positions::SamePoint(clone[n].retreat,proposal.retreat)&&humanlike_bot_hold_positions::SamePoint(clone[n].focus,proposal.focus);
        if(clone[n].id==live.annotation||exactTestAlias){auto& row=clone[n];row=proposal;row.enabled=true;
            row.riskKnown=row.flankRiskKnown=row.exposedAnglesKnown=true;row.authoredRisk=static_cast<float>(live.preflight.risk);
            row.authoredFlankRisk=static_cast<float>(live.preflight.flankRisk);row.authoredExposedAngles=live.preflight.exposed;installed=true;}}
    if(!installed&&(live.annotation==1303||live.annotation==1304)&&cloneCount<clone.size()){
        auto& row=clone[cloneCount++];row=proposal;row.enabled=true;
        row.riskKnown=row.flankRiskKnown=row.exposedAnglesKnown=true;row.authoredRisk=static_cast<float>(live.preflight.risk);
        row.authoredFlankRisk=static_cast<float>(live.preflight.flankRisk);row.authoredExposedAngles=live.preflight.exposed;installed=true;
    }
    if(!installed||!Fence(life)||!space::SetAnnotations(clone.data(),cloneCount,live.revision)){
        Abort(7881,"map_hold_test_local_measured_clone_install_rejected",brain);return;}
    live.installed=live.brainReleased=true;live.notObserved=false;live.stage="map_hold_measured_clone_installed_original_Brain_path_pending";
    brain.SetEnabled(true); // Its normal release frame runs before a later real ReadSpace.
}
inline void Prepare(size_t index,const dto::SensorFrame& frame,dto::Brain& brain,const cfg::Profile& profile){
    if(index!=1||!context.configured||!context.host)return;
    bool returned=false;__try{PrepareBody(index,frame,brain,profile);returned=true;}
    __finally{if(!returned)NoteFailure(7882,"map_hold_native_preflight_exception_unobserved");Publish();}
}
inline void Submitted(size_t index,const local_server_match::Frame& submitted,const dto::SensorFrame* frame,
                      dto::Brain& brain,bool navigationRequested,const dto::EstimatedNavigation& request){
    if(index!=1||!context.configured||!context.host||!live.setup)return;
    const auto* native=local_server_npc_input::Player(1);
    if(!native||!Fence(live.preflight.life)){live.releaseUnavailable=true;Abort(7883,"map_hold_release_or_sample_old_life_unavailable",brain);Publish();return;}
    const bool released=!submitted.fire&&!submitted.reload&&!submitted.jump&&!submitted.crouch&&!submitted.walk
        &&training_bot_controller::Length2D(submitted.worldDirection)==0;
    if(live.failed||live.dwelled){if(released){
        if(!live.releaseSubmitted){live.releaseAt=frame?frame->self.nowSeconds:local_server_match::GetState().now;live.releasePhysics=native->physicsCompleted;live.releaseSubmissions=native->submissions;
            live.releaseFeet=Point(native->positionAfter);live.releaseFeet.z-=training_bot_controller::actors[1].observed.capsuleHalfHeight;
            live.releaseDirection=Point(submitted.worldDirection);}
        live.releaseSubmitted=true;live.stage=live.dwelled?"map_hold_physics_arrival_dwell_and_original_release_observed":"map_hold_interruption_original_release_submitted";}Publish();return;}
    if(!frame){Abort(7889,"map_hold_current_owned_sensor_frame_unavailable",brain);Publish();return;}
    const auto& own=frame->self;
    if(own.sampledAtSeconds<=live.lastSampledAt)return;
    live.lastSampledAt=own.sampledAtSeconds;
    Record r{};r.sequence=live.produced+1;auto& sample=r.own;
    sample.life=live.preflight.life;sample.now=own.nowSeconds;sample.sampledAt=own.sampledAtSeconds;sample.ownFeet=Point(own.feet);
    const auto& actual=training_bot_controller::actors[1].observed;
    r.capsuleHalfHeight=actual.capsuleHalfHeight;r.capsuleRadius=actual.capsuleRadius;r.eyeHeight=own.eye.z-own.feet.z;
    sample.physicalFeet=Point(native->positionAfter);sample.physicalFeet.z-=actual.capsuleHalfHeight;
    sample.velocity=Point(native->velocityAfter);sample.physics=native->physicsCompleted;sample.submissions=native->submissions;
    sample.physicalValid=native->rootPhysicsDispatched&&native->rootPhysicsClassValid&&!native->error;
    sample.groundKnown=own.groundedKnown;sample.grounded=own.grounded;sample.successfulSubmit=true;
    sample.mapRequest=request.requested&&request.purpose==dto::NavigationPurpose::MapHold;
    sample.staticMap=request.knowledge==dto::NavigationKnowledge::StaticMap&&!request.usesHearing&&request.uncertaintyCm==0;
    sample.annotation=request.mapAnnotationId;sample.revision=request.mapRevision;sample.key=request.key;
    sample.navigationRequested=navigationRequested;sample.holdProof=frame->space.holdProofReady;sample.goalReached=frame->space.estimatedGoalReached;sample.released=released;
    r.rootBefore=native->positionBefore;r.rootAfter=native->positionAfter;r.consumedInput=native->consumedInput;
    r.capture=native->captureGeneration;r.snapshotFrame=native->snapshotFrame;r.reason=static_cast<uint32_t>(brain.Snapshot().reason);
    r.activity=static_cast<uint32_t>(brain.Snapshot().activity);r.pathProven=frame->space.pathProven;r.holdProof=frame->space.holdProofReady;
    const auto& cache=space::caches[1];
    if(cache.navigationEstimate.requested&&cache.navigationEstimate.mapAnnotationId==live.annotation&&cache.navigationEstimate.mapRevision==live.revision
       &&cache.navigationEstimate.key==request.key&&cache.pathCount>=2&&cache.pathCount<=live.routePath.size()){
        live.routeNativePath=true;live.routePathCount=static_cast<uint32_t>(cache.pathCount);live.routeGeneration=cache.pathGeneration;
        for(size_t n=0;n<cache.pathCount;++n)live.routePath[n]=Point(cache.path[n]);}
    // During preflight only record native zero-input physics; no arrival claim.
    if(live.brainReleased){const auto result=core::Observe(live.acceptance,live.configuration,sample);
        if(result==core::Result::Rejected)Abort(7884,"map_hold_actual_physics_key_ground_or_dwell_continuity_rejected",brain);
        else if(result==core::Result::Dwelled||result==core::Result::Released){
            if(!live.routeNativePath)Abort(7885,"map_hold_Brain_original_native_path_not_observed",brain);
            else{live.dwelled=true;live.stage="map_hold_same_life_physical_dwell_observed_release_pending";brain.SetEnabled(false);Restore();}}
    }
    AcquireSRWLockExclusive(&cacheLock);
    if(live.produced-cached.drained>=records.size()){++live.dropped;NoteFailure(7886,"map_hold_lossless_sample_buffer_exhausted");}
    else{records[live.produced%records.size()]=r;++live.produced;}
    ReleaseSRWLockExclusive(&cacheLock);Publish();
}
inline void Retiring(size_t index,uint32_t epoch,dto::Brain& brain){
    if(index!=1||!context.configured||!context.host||!live.setup||epoch!=live.preflight.life.epoch)return;
    brain.SetEnabled(false);Restore();
    if(!live.dwelled)NoteFailure(7887,"map_hold_original_lifecycle_retirement_interrupted_acceptance");
    // The existing owner's retirement performs native input cleanup afterward.
    // This callback does not fabricate an unavailable per-intent release ACK.
    if(!live.releaseSubmitted)live.releaseUnavailable=true;Publish();
}
inline const char* Boolean(bool value){return value?"true":"false";}
inline void Number(FILE* f,double value){if(std::isfinite(value))std::fprintf(f,"%.17g",value);else std::fputs("null",f);}
inline void WritePoint(FILE* f,core::Point p){std::fputc('[',f);Number(f,p.x);std::fputc(',',f);Number(f,p.y);std::fputc(',',f);Number(f,p.z);std::fputc(']',f);}
inline void WriteRecord(FILE* f,const Record& r){const auto& s=r.own;
    std::fprintf(f,"{\"scenario\":14,\"sequence\":%llu,\"epoch\":%u,\"life_token\":%llu,\"source\":\"%016llx\",\"key\":%llu,\"annotation_id\":%u,\"annotation_revision\":%u,\"now\":",static_cast<unsigned long long>(r.sequence),s.life.epoch,static_cast<unsigned long long>(s.life.token),static_cast<unsigned long long>(s.life.source),static_cast<unsigned long long>(s.key),s.annotation,s.revision);Number(f,s.now);
    std::fputs(",\"own_sampled_at\":",f);Number(f,s.sampledAt);std::fputs(",\"actual_feet\":",f);WritePoint(f,s.physicalFeet);
    std::fputs(",\"own_feet\":",f);WritePoint(f,s.ownFeet);std::fputs(",\"root_before\":",f);WritePoint(f,Point(r.rootBefore));std::fputs(",\"root_after\":",f);WritePoint(f,Point(r.rootAfter));
    std::fputs(",\"native_velocity\":",f);WritePoint(f,s.velocity);std::fputs(",\"native_consumed_input_vector\":",f);WritePoint(f,Point(r.consumedInput));
    std::fputs(",\"original_capsule_half_height\":",f);Number(f,r.capsuleHalfHeight);std::fputs(",\"original_capsule_radius\":",f);Number(f,r.capsuleRadius);std::fputs(",\"original_eye_height\":",f);Number(f,r.eyeHeight);
    std::fprintf(f,",\"physics_completed\":%llu,\"submissions\":%llu,\"capture_generation\":%llu,\"snapshot_frame\":%llu,\"physical_valid\":%s,\"ground_known\":%s,\"grounded\":%s,\"successful_submit\":%s,\"navigation_requested\":%s,\"hold_proof\":%s,\"goal_reached\":%s,\"map_request\":%s,\"released\":%s,\"reason\":%u,\"activity\":%u,\"per_intent_consumption_known\":false}\n",static_cast<unsigned long long>(s.physics),static_cast<unsigned long long>(s.submissions),static_cast<unsigned long long>(r.capture),static_cast<unsigned long long>(r.snapshotFrame),Boolean(s.physicalValid),Boolean(s.groundKnown),Boolean(s.grounded),Boolean(s.successfulSubmit),Boolean(s.navigationRequested),Boolean(s.holdProof),Boolean(s.goalReached),Boolean(s.mapRequest),Boolean(s.released),r.reason,r.activity);
}
inline void WriteStatus(FILE* f){const auto s=Copy();
    std::fprintf(f,"{\"scenario\":14,\"configured\":%s,\"armed\":%s,\"setup\":%s,\"failed\":%s,\"not_observed\":%s,\"error\":%u,\"stage\":\"%s\",\"annotation_id\":%u,\"annotation_revision\":%u,\"publication\":%llu,\"produced\":%llu,\"drained\":%llu,\"dropped\":%llu,\"io_errors\":%llu,\"test_local_installed\":%s,\"annotations_restored\":%s,\"brain_released\":%s,\"actual_dwelled\":%s,\"release_submitted\":%s,\"release_unavailable\":%s,\"route_native_path\":%s,\"request_key\":%llu,\"dwell_seconds\":",Boolean(s.configured),Boolean(s.armed),Boolean(s.setup),Boolean(s.failed),Boolean(s.notObserved),s.error,s.stage,s.annotation,s.revision,static_cast<unsigned long long>(s.publication),static_cast<unsigned long long>(s.produced),static_cast<unsigned long long>(s.drained),static_cast<unsigned long long>(s.dropped),static_cast<unsigned long long>(s.ioErrors),Boolean(s.installed),Boolean(s.restored),Boolean(s.brainReleased),Boolean(s.dwelled),Boolean(s.releaseSubmitted),Boolean(s.releaseUnavailable),Boolean(s.routeNativePath),static_cast<unsigned long long>(s.acceptance.key));Number(f,s.acceptance.dwellSeconds);
    std::fprintf(f,",\"preflight_stage\":%u,\"preflight_ground_mask\":%u,\"preflight_capsule_segments\":%u,\"risk_exposed_directions\":%u,\"four_direction_static_risk\":",unsigned(s.preflight.stage),unsigned(s.preflight.ground),unsigned(s.preflight.segments),s.preflight.exposed);Number(f,s.preflight.risk);
    std::fputs(",\"four_direction_static_flank_risk\":",f);Number(f,s.preflight.flankRisk);
    std::fputs(",\"preflight_original_path\":[",f);for(uint32_t n=0;n<s.preflight.pathCount&&n<s.preflight.path.size();++n){if(n)std::fputc(',',f);WritePoint(f,s.preflight.path[n]);}
    std::fputs("],\"Brain_original_path\":[",f);for(uint32_t n=0;n<s.routePathCount&&n<s.routePath.size();++n){if(n)std::fputc(',',f);WritePoint(f,s.routePath[n]);}
    std::fputs("],\"risk_rays\":[",f);for(size_t n=0;n<s.preflight.rays.size();++n){std::fprintf(f,"%s{\"result\":%u,\"at\":",n?",":"",unsigned(s.preflight.rays[n]));Number(f,s.preflight.rayAt[n]);std::fputc('}',f);}
    std::fputs("],\"initial_actual_feet\":",f);WritePoint(f,s.acceptance.initialFeet);std::fputs(",\"arrival_actual_feet\":",f);WritePoint(f,s.acceptance.arrivalFeet);
    std::fputs(",\"last_actual_feet\":",f);WritePoint(f,s.acceptance.lastFeet);
    std::fprintf(f,",\"nonce\":\"%016llx\",\"case_hash\":\"%016llx\",\"release_physics_completed\":%llu,\"release_submissions\":%llu,\"release_at\":",static_cast<unsigned long long>(s.nonce),static_cast<unsigned long long>(s.caseHash),static_cast<unsigned long long>(s.releasePhysics),static_cast<unsigned long long>(s.releaseSubmissions));Number(f,s.releaseAt);
    std::fputs(",\"release_actual_feet\":",f);WritePoint(f,s.releaseFeet);std::fputs(",\"release_actual_direction\":",f);WritePoint(f,s.releaseDirection);
    std::fputs(",\"preflight_original_eye_height\":",f);Number(f,s.preflight.eyeHeight);std::fputs(",\"preflight_original_capsule_radius\":",f);Number(f,s.preflight.radius);std::fputs(",\"preflight_original_capsule_half_height\":",f);Number(f,s.preflight.half);
    std::fputs(",\"preflight_actual_ground\":[",f);for(size_t n=0;n<s.preflight.actualGround.size();++n){if(n)std::fputc(',',f);if(s.preflight.ground&(1U<<n))WritePoint(f,s.preflight.actualGround[n]);else std::fputs("null",f);}
    std::fputs("],\"four_direction_risk_endpoints\":[",f);for(size_t n=0;n<s.preflight.riskEnds.size();++n){if(n)std::fputc(',',f);if(s.preflight.bound)WritePoint(f,s.preflight.riskEnds[n]);else std::fputs("null",f);}
    std::fputs("],\"native_source_tuple\":[",f);for(size_t n=0;n<s.sourceIdentity.size();++n)std::fprintf(f,"%s\"%016llx\"",n?",":"",static_cast<unsigned long long>(s.sourceIdentity[n]));
    std::fputs("]",f);
    std::fprintf(f,",\"first_failure_code\":%u,\"first_failure_stage\":\"%s\",\"first_failure_at_ms\":%llu,\"first_failure_preflight_stage\":%u,\"first_failure_guard_mask\":%u,\"guard_mask\":%u,\"readiness_block_mask\":%u,\"last_block_mask\":%u,\"last_block_stage\":\"%s\",\"last_block_preflight_stage\":%u,\"last_attempted_preflight_stage\":%u,\"readiness_waits\":%llu,\"readiness_match_active\":%s,\"readiness_configured_paused\":%s,\"readiness_own_paused\":%s,\"readiness_own_valid\":%s,\"readiness_observation_ready\":%s,\"readiness_speed_ready\":%s,\"readiness_capsule_known\":%s,\"readiness_eye_known\":%s,\"readiness_ground_known\":%s,\"readiness_radius\":",s.firstFailureCode,s.firstFailureStage,static_cast<unsigned long long>(s.firstFailureAtMs),s.firstFailurePreflightStage,s.firstFailureGuardMask,s.guardMask,s.readinessBlockMask,s.lastBlockMask,s.lastBlockStage,s.lastBlockPreflightStage,s.lastAttemptedPreflightStage,static_cast<unsigned long long>(s.readinessWaits),Boolean(s.startup.matchActive),Boolean(s.startup.configuredPaused),Boolean(s.startup.ownPaused),Boolean(s.startup.ownValid),Boolean(s.startup.observationReady),Boolean(s.startup.speedReady),Boolean(s.startup.capsuleKnown),Boolean(s.startup.eyeKnown),Boolean(s.startup.groundKnown));Number(f,s.startup.radius);
    std::fputs(",\"readiness_half_height\":",f);Number(f,s.startup.half);std::fputs(",\"readiness_eye_height\":",f);Number(f,s.startup.eyeHeight);
    std::fprintf(f,",\"readiness_eye_finite\":%s,\"readiness_grounded\":%s",Boolean(s.startup.eyeFinite),Boolean(s.startup.grounded));
    std::fputs(",\"risk_scope\":\"four_authored_static_map_directions_only\",\"range_known\":false,\"player_heatmap_verified\":false,\"all_firing_angles_covered\":false,\"per_intent_consumption_known\":false}",f);
}
inline void DrainBody(){
    if(!context.configured||!context.host)return;const auto s=Copy();
    if(((s.failed&&(s.releaseSubmitted||s.releaseUnavailable))||(s.dwelled&&s.releaseSubmitted))&&s.publication!=completionWritten){
        Completion result;result.nonce=context.contract.nonce;result.verdict=s.failed?2:1;result.publication=s.publication;
        const auto path=context.parent+L"map-hold-fixture-complete.bin",temporary=path+L".tmp";FILE* f=nullptr;
        bool okay=_wfopen_s(&f,temporary.c_str(),L"wb")==0&&f;
        if(okay){okay=std::fwrite(&result,1,sizeof(result),f)==sizeof(result);okay=std::fclose(f)==0&&okay;}
        if(okay)okay=MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
        if(okay)completionWritten=s.publication;else{AcquireSRWLockExclusive(&cacheLock);++cached.ioErrors;ReleaseSRWLockExclusive(&cacheLock);}
    }
    std::array<Record,32> batch{};size_t count=0;
    AcquireSRWLockShared(&cacheLock);const auto first=cached.drained;
    while(count<batch.size()&&first+count<cached.produced){batch[count]=records[(first+count)%records.size()];++count;}
    ReleaseSRWLockShared(&cacheLock);if(!count)return;
    FILE* f=nullptr;bool okay=_wfopen_s(&f,(context.directory+L"map-hold-fixture-own-native.jsonl").c_str(),L"ab")==0&&f;
    if(okay){for(size_t n=0;n<count;++n)WriteRecord(f,batch[n]);okay=std::ferror(f)==0;okay=std::fclose(f)==0&&okay;}
    AcquireSRWLockExclusive(&cacheLock);if(okay)cached.drained=first+count;else ++cached.ioErrors;ReleaseSRWLockExclusive(&cacheLock);
}
inline void Drain(){
    // SaveState may be called by GT or Observe. Serialize the file writer;
    // neither waits on a busy writer nor holds this lock during engine calls.
    if(!TryAcquireSRWLockExclusive(&drainLock))return;
    __try{DrainBody();}__finally{ReleaseSRWLockExclusive(&drainLock);}
}
} // namespace local_server_hold_fixture
