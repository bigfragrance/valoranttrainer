#pragma once
#include "armor_poll_performance.h"
#include "human_bot_store.h"
#include "humanlike_bot_brain.h"
#include "local_server_humanlike_sensors.h"
#include "local_server_humanlike_space.h"
#include "local_server_humanlike_peek_diagnostics.h"
#include "local_server_humanlike_nav_diagnostics.h"
#include "humanlike_bot_visible_impacts.h"
#include "local_server_peek_fixture_observer.h"
#include "local_server_hold_fixture.h"
#include "local_server_humanlike_audio_requests.h"
#include "local_server_humanlike_audio_tick.h"
#include <array>
#include <cstdarg>
#include <cmath>
#include <cstdio>
#include <string>

// The existing authority TickBots owns both callbacks. This adapter owns no
// timer, physics step, LocalPlayer, enemy-position query or second input path.
namespace local_server_humanlike_bots {
namespace dto=humanlike_bot;
namespace cfg=humanlike_bot_config;
namespace sensors=local_server_humanlike_sensors;
namespace space=local_server_humanlike_space;
using Frame=training_bot_controller::Frame;
inline constexpr size_t MaxPlayers=local_server_match::MaxPlayers;
struct Player {
    bool configured=false,releaseLegacy=false,navigationRequested=false;
    bool releasePending=false;dto::EstimatedNavigation navigationEstimate{};
    dto::EstimatedAim aimEstimate{};
    void*releasePawn=nullptr;uint32_t releaseEpoch=0;
    uint32_t stableId=0,revision=0;
    uint64_t decisions=0,moveSamples=0,crouchSamples=0,jumpSamples=0;
    uint64_t nativeMoveSamples=0,nativeCrouchSamples=0,nativeJumpSamples=0;
    uint64_t frameSamples=0,coverSamples=0,groundSamples=0,clearanceSamples=0,activePeekSamples=0,groundCoverClearanceSamples=0;
    uint64_t jumpIntentSamples=0,reloadIntentSamples=0,reloadPositiveSamples=0,footstepCueSamples=0;
    uint64_t magazineIncreaseSamples=0,reserveDecreaseSamples=0,lastWeaponBinding=0;
    uint32_t lastWeaponEpoch=0;int32_t lastMagazine=-1,lastReserve=-1;
    double lastDecision=-1;
};
struct State {
    bool initialized=false,closed=false,configured=false,enabled=false,sensorsReady=false;
    uint32_t revision=0,seed=0,profileCount=0;
    uint64_t fingerprint=0,passes=0,configurations=0,decisions=0,humanRejections=0;
    unsigned error=0;
    bool ackPending=false;
    const char*stage="humanlike_bots_not_initialized";
    double nextCache=0;
};
inline State state{};
inline std::array<dto::Brain,MaxPlayers> brains{};
inline std::array<cfg::Profile,MaxPlayers> profiles{};
inline std::array<Player,MaxPlayers> players{};
inline humanlike_bot_visible_impacts::Bridge visibleImpacts;
inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline std::string behaviorJson="{\"connected\":false,\"execution_verified\":false,\"bots\":[]}";
inline const State& GetState(){return state;}
inline bool Context(){return state.initialized&&!state.closed&&original_players::Thread();}
inline const cfg::Profile* ReadProfile(size_t index){
    return Context()&&state.configured&&index<state.profileCount?&profiles[index]:nullptr;
}
inline bool Fail(unsigned code,const char*stage){state.error=code;state.stage=stage;return false;}
inline const char* Activity(dto::Activity value){
    switch(value){
    case dto::Activity::Dead:return "dead";case dto::Activity::Respawn:return "respawn";
    case dto::Activity::Roam:return "roam";case dto::Activity::Search:return "search";
    case dto::Activity::Hold:return "hold";case dto::Activity::Engage:return "engage";
    case dto::Activity::Information:return "information";case dto::Activity::Retreat:return "retreat";
    case dto::Activity::Relocate:return "relocate";case dto::Activity::Reload:return "reload";
    }return "unknown";
}
inline const char* Reason(dto::Reason value){
    using R=dto::Reason;
#define HUMAN_REASON(name) case R::name:return #name
    switch(value){
    HUMAN_REASON(None);HUMAN_REASON(InvalidTime);HUMAN_REASON(InvalidOwnLife);
    HUMAN_REASON(OwnStateUnavailable);HUMAN_REASON(NativeActionUnavailable);HUMAN_REASON(Dead);
    HUMAN_REASON(Retired);HUMAN_REASON(Paused);HUMAN_REASON(ProfileUnavailable);HUMAN_REASON(ProfileInvalid);
    HUMAN_REASON(PerceptionUnavailable);HUMAN_REASON(NoAcquiredTarget);HUMAN_REASON(VisualUnknown);
    HUMAN_REASON(VisualDeferred);HUMAN_REASON(RecognitionPending);HUMAN_REASON(OutsideFieldOfView);
    HUMAN_REASON(ExposureInsufficient);HUMAN_REASON(MemoryExpired);HUMAN_REASON(ReactionPending);
    HUMAN_REASON(AttentionLapse);HUMAN_REASON(TurnPending);HUMAN_REASON(AimOutsideTolerance);
    HUMAN_REASON(TargetLost);HUMAN_REASON(TargetRetired);HUMAN_REASON(TargetSwitchPending);
    HUMAN_REASON(WeaponUnknown);HUMAN_REASON(WeaponBindingChanged);HUMAN_REASON(AmmoUnknown);
    HUMAN_REASON(EmptyMagazine);HUMAN_REASON(ReloadUnknown);HUMAN_REASON(ReloadPending);
    HUMAN_REASON(WeaponRecoveryUnknown);HUMAN_REASON(WeaponNotRecovered);HUMAN_REASON(MovementUnknown);
    HUMAN_REASON(MovementTooFast);HUMAN_REASON(ActualShotFeedUnavailable);HUMAN_REASON(ActualShotPending);
    HUMAN_REASON(ShotOverflow);HUMAN_REASON(ShotWindowComplete);HUMAN_REASON(TriggerRelease);
    HUMAN_REASON(NavigationUnavailable);HUMAN_REASON(CoverUnavailable);HUMAN_REASON(ClearanceUnavailable);
    HUMAN_REASON(JumpLandingUnproven);HUMAN_REASON(JumpNotObserved);HUMAN_REASON(CrouchNotObserved);
    HUMAN_REASON(Collision);HUMAN_REASON(Stalled);HUMAN_REASON(MultipleThreats);
    HUMAN_REASON(HurtDirectionUnknown);HUMAN_REASON(HearingUnavailable);HUMAN_REASON(RetreatUnavailable);
    HUMAN_REASON(ActionInterrupted);HUMAN_REASON(PeekPurposeComplete);HUMAN_REASON(InputNotExecution);
    HUMAN_REASON(ParameterUnsupported);HUMAN_REASON(TraceOverflow);HUMAN_REASON(ProfileChanged);
    HUMAN_REASON(GroundingUnknown);HUMAN_REASON(PermissionRejected);HUMAN_REASON(PoppinEvidenceMissing);
    HUMAN_REASON(Disabled);HUMAN_REASON(NativeShotOvershoot);HUMAN_REASON(NativeBurstUnproven);
    HUMAN_REASON(NativeReleaseUnproven);HUMAN_REASON(ReloadNotObserved);
    HUMAN_REASON(ActualMissFeedbackUnavailable);HUMAN_REASON(ObservationHistoryUnavailable);
    }
#undef HUMAN_REASON
    return "UnknownReason";
}
inline void Append(std::string&out,const char*format,...){
    char buffer[4096]{};va_list args;va_start(args,format);
    const int size=vsnprintf(buffer,sizeof(buffer),format,args);va_end(args);
    if(size>=0&&static_cast<size_t>(size)<sizeof(buffer))out.append(buffer,static_cast<size_t>(size));
}
// A frame pointer and OwnSnapshot.valid do not prove these debug metrics
// finite. Preserve their flags; a null numeric token means a nonfinite value.
inline std::string DiagnosticNumber(double value,int decimals){
    if(!std::isfinite(value))return "null";
    char buffer[384]{};
    const int size=snprintf(buffer,sizeof(buffer),"%.*f",decimals,value);
    return size>=0&&static_cast<size_t>(size)<sizeof(buffer)
        ?std::string(buffer,static_cast<size_t>(size)):"null";
}
inline void Cache(double now,bool force=false){
    if(!force&&now<state.nextCache)return;state.nextCache=now+.5;
    std::string next;
    Append(next,"{\"connected\":%s,\"enabled\":%s,\"execution_verified\":false,\"stage\":\"%s\",\"error\":%u,\"passes\":%llu,\"decisions\":%llu,\"configuration_count\":%llu,\"human_rejections\":%llu,\"bots\":[",
        state.initialized&&!state.closed?"true":"false",state.enabled?"true":"false",state.stage,state.error,
        static_cast<unsigned long long>(state.passes),static_cast<unsigned long long>(state.decisions),
        static_cast<unsigned long long>(state.configurations),static_cast<unsigned long long>(state.humanRejections));
    bool comma=false;
    for(size_t n=1;n<state.profileCount;++n){const auto&p=players[n];const auto&s=brains[n].Snapshot();
        const auto native=local_server_match::Player(n);
        const auto phase=native&&!native->alive?dto::Activity::Dead:s.activity;
        Append(next,"%s{\"index\":%u,\"stable_bot_id\":%u,\"configured\":%s,\"profile_revision\":%u,\"phase\":\"%s\",\"reason_code\":%u,\"reason_text\":\"%s\",\"hold_perceive_ms\":%.4f,\"primary_speed_deg_s\":%.4f,\"epoch\":%u,\"target_track\":%llu,\"aim_stage\":%u,\"motion_stage\":%u,\"peek\":%u,\"attention\":%u,\"fire_mode\":%u,\"actual_shots\":%llu,\"shots_in_window\":%u,\"window_goal\":%u,\"intents\":%llu,\"consumption_known\":%s,\"event_at\":%.4f,\"noticed_at\":%.4f,\"decided_at\":%.4f,\"action_at\":%.4f,\"native_move_samples\":%llu,\"native_crouch_samples\":%llu,\"native_jump_samples\":%llu",
            comma?",":"",static_cast<unsigned>(n),p.stableId,p.configured?"true":"false",p.revision,
            Activity(phase),static_cast<unsigned>(s.reason),Reason(s.reason),
            cfg::Get(profiles[n],cfg::Param::ReactionHoldPerceiveMs),cfg::Get(profiles[n],cfg::Param::PrimarySpeedDegS),
            s.life.spawnEpoch,static_cast<unsigned long long>(s.target),static_cast<unsigned>(s.aim),static_cast<unsigned>(s.motion),
            static_cast<unsigned>(s.peek),static_cast<unsigned>(s.attention),static_cast<unsigned>(s.fireMode),
            static_cast<unsigned long long>(s.actualShots),s.actualShotsInWindow,s.windowGoalShots,
            static_cast<unsigned long long>(s.requestedIntents),s.nativeIntentConsumptionKnown?"true":"false",
            s.eventAtSeconds,s.noticedAtSeconds,s.decidedAtSeconds,s.actionAtSeconds,
            static_cast<unsigned long long>(p.nativeMoveSamples),static_cast<unsigned long long>(p.nativeCrouchSamples),
            static_cast<unsigned long long>(p.nativeJumpSamples));
        const auto*f=state.sensorsReady?sensors::Frame(n):nullptr;const auto yes=[](bool v){return v?"true":"false";};
        const auto&estimate=s.estimatedPrefire;
        if(!estimate.requested)Append(next,",\"estimated_prefire\":{\"requested\":false,\"estimate\":null,\"proof\":null,\"started\":false,\"native_execution_receipt\":false}");
        else{
            // Existing cached hypothesis/proof only; this never queries geometry
            // or certifies a native input, shot, hit or completed fire window.
            const auto number=[](double value){
                if(!std::isfinite(value))return std::string("null");char text[64]{};
                const int size=snprintf(text,sizeof(text),"%.17g",value);
                return size>=0&&static_cast<size_t>(size)<sizeof(text)?std::string(text,static_cast<size_t>(size)):"null";
            };
            const char*source=estimate.source==dto::EstimatedAimSource::Heard?"heard":estimate.source==dto::EstimatedAimSource::AuthoredAngle?"authored_angle":estimate.source==dto::EstimatedAimSource::AuthoredHold?"authored_hold":"unknown";
            Append(next,",\"estimated_prefire\":{\"requested\":true,\"estimate\":{\"key\":%llu,\"source\":\"%s\",\"source_episode\":%llu,\"own_life\":[%u,%u,%llu],\"point\":[%s,%s,%s],\"acquired_at\":%s,\"ready_at\":%s,\"expires_at\":%s,\"confidence\":%s,\"uncertainty_cm\":%s},\"started\":%s,\"proof\":",
                static_cast<unsigned long long>(estimate.key),source,static_cast<unsigned long long>(estimate.sourceEpisode),
                estimate.life.stableBotId,estimate.life.spawnEpoch,static_cast<unsigned long long>(estimate.life.token),
                number(estimate.point.x).c_str(),number(estimate.point.y).c_str(),number(estimate.point.z).c_str(),
                number(estimate.acquiredAtSeconds).c_str(),number(estimate.readyAtSeconds).c_str(),number(estimate.expiresAtSeconds).c_str(),
                number(estimate.confidence).c_str(),number(estimate.uncertaintyCm).c_str(),yes(s.estimatedPrefireStarted));
            if(!f)Append(next,"null");
            else{
                const auto&proof=f->space;
                const double maximumAge=(std::min)(static_cast<double>(cfg::Get(profiles[n],cfg::Param::OwnSnapshotMaxAgeMs))*.001,
                    static_cast<double>(cfg::Get(profiles[n],cfg::Param::FirePrefireMaxAgeMs))*.001);
                Append(next,"{\"key\":%llu,\"same_key\":%s,\"same_estimate\":%s,\"proof_known\":%s,\"clear\":%s,\"eye\":[%s,%s,%s],\"same_eye\":%s,\"sampled_at\":%s,\"own_now\":%s,\"maximum_age_seconds\":%s,\"gate_passed\":%s}",
                    static_cast<unsigned long long>(proof.estimatedAim.key),yes(proof.estimatedAim.key==estimate.key),
                    yes(humanlike_bot_prefire::Same(estimate,proof.estimatedAim)),yes(proof.estimatedAimKnown),yes(proof.estimatedAimClear),
                    number(proof.estimatedAimEye.x).c_str(),number(proof.estimatedAimEye.y).c_str(),number(proof.estimatedAimEye.z).c_str(),
                    yes(humanlike_bot_prefire::Point(proof.estimatedAimEye,f->self.eye)),number(proof.estimatedAimAtSeconds).c_str(),
                    number(f->self.nowSeconds).c_str(),number(maximumAge).c_str(),yes(humanlike_bot_prefire::Proof(estimate,*f,maximumAge)));
            }
            Append(next,",\"native_execution_receipt\":false}");
        }
        constexpr std::array<const char*,9> reactionNames{{"hold","active_peek","third_party","flank","hearing","hurt","reappear","direction_change","crouch_jump"}};
        static_assert(cfg::ReactionContextCount==reactionNames.size());
        const auto reaction=static_cast<size_t>(s.lastReactionContext);
        Append(next,",\"sampled_reaction\":{\"known\":%s,\"context\":\"%s\",\"event\":%llu,\"hesitation_ms\":%s,\"yaw_error_deg\":%s,\"pitch_error_deg\":%s,\"native_execution_receipt\":false},\"requested_stance\":{\"walk\":%s,\"crouch\":%s,\"jump\":%s,\"release_all\":%s}",
            yes(reaction<reactionNames.size()&&s.lastReactionEvent>0),reaction<reactionNames.size()?reactionNames[reaction]:"none",
            static_cast<unsigned long long>(s.lastReactionEvent),DiagnosticNumber(s.lastReactionHesitationMs,4).c_str(),
            DiagnosticNumber(s.lastReactionYawErrorDeg,5).c_str(),DiagnosticNumber(s.lastReactionPitchErrorDeg,5).c_str(),
            yes(s.lastIntentWalk),yes(s.lastIntentCrouch),yes(s.lastIntentJump),yes(s.lastIntentRelease));
        // Values in the actual native profile, separate from action receipts.
        Append(next,",\"configured_flank_hesitation_ms\":%s,\"configured_advance_walk_probability\":%s",
            DiagnosticNumber(cfg::Get(profiles[n],cfg::Param::ReactionFlankHesitateMs),4).c_str(),
            DiagnosticNumber(cfg::Get(profiles[n],cfg::Param::AdvanceWalkProbability),6).c_str());
        Append(next,",\"frame_debug\":{\"available\":%s,\"own_valid\":%s,\"ground_known\":%s,\"grounded\":%s,\"clearance_known\":%s,\"cover_known\":%s,\"peek_path\":%s,\"retreat_path\":%s,\"ground_path\":%s,\"landing_support\":%s,\"cover_confidence\":%s,\"exposure_risk\":%s,\"available_space_cm\":%s,\"contact_count\":%u,\"sound_count\":%u},\"sample_counts\":{\"frames\":%llu,\"ground\":%llu,\"cover\":%llu,\"clearance\":%llu,\"active_peek\":%llu,\"jump_intent\":%llu,\"reload_intent\":%llu,\"positive_reload\":%llu,\"footstep_cue\":%llu,\"magazine_increases\":%llu,\"reserve_decreases\":%llu}",
            yes(f!=nullptr),yes(f&&f->self.valid),yes(f&&f->self.groundedKnown),yes(f&&f->self.grounded),
            yes(f&&f->space.clearanceKnown),yes(f&&f->space.coverKnown),yes(f&&f->space.peekPathProven),
            yes(f&&f->space.retreatPathProven),yes(f&&f->space.groundProven),yes(f&&f->space.landingSupportKnown),
            DiagnosticNumber(f?f->space.coverConfidence:0.,4).c_str(),DiagnosticNumber(f?f->space.exposureRisk:0.,4).c_str(),
            DiagnosticNumber(f?dto::math::Horizontal(dto::math::Sub(f->space.peekOpen,f->space.coverStart)):0.,3).c_str(),f?unsigned(f->contactCount):0U,f?unsigned(f->soundCount):0U,
            static_cast<unsigned long long>(p.frameSamples),static_cast<unsigned long long>(p.groundSamples),
            static_cast<unsigned long long>(p.coverSamples),static_cast<unsigned long long>(p.clearanceSamples),
            static_cast<unsigned long long>(p.activePeekSamples),static_cast<unsigned long long>(p.jumpIntentSamples),
            static_cast<unsigned long long>(p.reloadIntentSamples),static_cast<unsigned long long>(p.reloadPositiveSamples),
            static_cast<unsigned long long>(p.footstepCueSamples),static_cast<unsigned long long>(p.magazineIncreaseSamples),
            static_cast<unsigned long long>(p.reserveDecreaseSamples));
        Append(next,",\"own_vision\":{\"native_blindness_known\":%s,\"native_blinded\":%s,\"rendered_visibility_complete\":false}",
            yes(f&&f->self.nativeBlindnessKnown),yes(f&&f->self.nativeBlindnessKnown&&f->self.nativeBlinded));
        // Configuration values are explicit settings, not execution receipts.
        Append(next,",\"jump_settings\":{\"enabled\":%s,\"weight\":%s,\"distance_cm\":%s,\"minimum_space_cm\":%s},\"ground_cover_clearance_samples\":%llu}",
            DiagnosticNumber(cfg::Get(profiles[n],cfg::Param::JumpEnabled),0).c_str(),DiagnosticNumber(cfg::Get(profiles[n],cfg::Param::JumpWeight),3).c_str(),
            DiagnosticNumber(cfg::Get(profiles[n],cfg::Param::JumpDistanceCm),3).c_str(),DiagnosticNumber(cfg::Get(profiles[n],cfg::Param::JumpMinimumSpaceCm),3).c_str(),
            static_cast<unsigned long long>(p.groundCoverClearanceSamples));comma=true;
    }
    const auto&sensor=sensors::GetState();const auto&navigation=space::state;const auto&feedback=visibleImpacts.Stats();
    Append(next,"],\"sensors\":{\"passes\":%llu,\"own_samples\":%llu,\"weapon_samples\":%llu,\"trace_queries\":%llu,\"trace_failures\":%llu,\"exception\":%u,\"exception_rva\":%llu},\"space\":{\"paths\":%llu,\"path_failures\":%llu,\"local_paths\":%llu,\"cover_validations\":%llu,\"cover_failures\":%llu,\"authored_annotations\":%u,\"budget_deferred\":%llu,\"local_cover_candidates\":%llu,\"local_cover_ready\":%llu,\"local_cover_rejected\":%llu,\"local_cover_deferred\":%llu,\"local_cover_invalidated\":%llu},\"visible_contact_model\":{\"accepted\":%llu,\"deferred\":%llu,\"rays\":%llu,\"native_hit_marker_observed\":false}}",
        static_cast<unsigned long long>(sensor.passes),static_cast<unsigned long long>(sensor.ownSamples),
        static_cast<unsigned long long>(sensor.weaponSamples),static_cast<unsigned long long>(sensor.traceQueries),
        static_cast<unsigned long long>(sensor.traceFailures),sensor.exception,static_cast<unsigned long long>(sensor.exceptionRva),
        static_cast<unsigned long long>(navigation.paths),static_cast<unsigned long long>(navigation.pathFailures),
        static_cast<unsigned long long>(navigation.localPaths),static_cast<unsigned long long>(navigation.coverValidations),
        static_cast<unsigned long long>(navigation.coverFailures),static_cast<unsigned>(navigation.count),
        static_cast<unsigned long long>(navigation.budgetDeferred),
        static_cast<unsigned long long>(navigation.localCoverCandidates),static_cast<unsigned long long>(navigation.localCoverReady),
        static_cast<unsigned long long>(navigation.localCoverRejected),static_cast<unsigned long long>(navigation.localCoverDeferred),
        static_cast<unsigned long long>(navigation.localCoverInvalidated),static_cast<unsigned long long>(feedback.accepted),
        static_cast<unsigned long long>(feedback.deferred),static_cast<unsigned long long>(feedback.rays));
    const auto&blind=local_server_humanlike_blind::state;
    next.pop_back();Append(next,",\"native_blind_query\":{\"samples\":%llu,\"known\":%llu,\"active\":%llu,\"unknown\":%llu,\"queries\":%llu,\"faults\":%llu,\"metadata_rejected\":%llu,\"life_rejected\":%llu,\"world_rejected\":%llu,\"source_rejected\":%llu,\"generation_rejected\":%llu,\"last_exception\":%u,\"last_exception_rva\":%llu,\"flash_opacity_known\":false,\"smoke_obstruction_complete\":false}}",
        static_cast<unsigned long long>(blind.samples),static_cast<unsigned long long>(blind.known),static_cast<unsigned long long>(blind.active),
        static_cast<unsigned long long>(blind.unknown),static_cast<unsigned long long>(blind.queries),static_cast<unsigned long long>(blind.faults),
        static_cast<unsigned long long>(blind.metadataRejects),static_cast<unsigned long long>(blind.lifeRejects),static_cast<unsigned long long>(blind.worldRejects),
        static_cast<unsigned long long>(blind.sourceRejects),static_cast<unsigned long long>(blind.generationRejects),blind.lastException,static_cast<unsigned long long>(blind.lastExceptionRva));
    const auto&visibleShot=sensors::provider.Stats();
    next.pop_back();Append(next,",\"visible_native_shot_perception\":{\"provenance\":\"derived_paired_native_shot_current_acquired_contact_fov_and_world_los\",\"muzzle_pixels_verified\":false,\"primes\":%llu,\"new_events\":%llu,\"derived_samples\":%llu,\"rejected\":%llu,\"budget_drops\":%llu}}",
        static_cast<unsigned long long>(visibleShot.visibleShotPrimes),static_cast<unsigned long long>(visibleShot.visibleShotNewEvents),
        static_cast<unsigned long long>(visibleShot.visibleShotDerivedSamples),static_cast<unsigned long long>(visibleShot.visibleShotRejected),
        static_cast<unsigned long long>(visibleShot.visibleShotBudgetDrops));
    AcquireSRWLockExclusive(&cacheLock);behaviorJson.swap(next);ReleaseSRWLockExclusive(&cacheLock);
    // Full bridge-file diagnostics only; never part of SummaryJson/MMF.
    local_server_humanlike_nav_diagnostics::Cache(now,state.profileCount,[](size_t index){
        const auto*frame=state.sensorsReady?sensors::Frame(index):nullptr;
        return local_server_humanlike_nav_diagnostics::Source{frame,&profiles[index]};
    });
    local_server_humanlike_peek_diagnostics::Cache(now,state.profileCount,[](size_t index){
        const auto*frame=state.sensorsReady?sensors::Frame(index):nullptr;
        return local_server_humanlike_peek_diagnostics::Source{&brains[index],frame?&frame->self:nullptr};
    });
}
inline const std::string& CapabilityJson(){
    // Reviewed production readers, not a claim that every native precondition
    // or action has executed. Unsupported fields remain visible but locked.
    static const std::string value=[](){
        std::string out="{\"status\":\"conditional\",\"runtime_execution_verified\":false,\"reason_zh\":\"已接入当前决策与原生数据链路；动作仍取决于本局有效读回、地图几何和感知条件。\",\"unsupported\":[";
        const std::array<size_t,12> unavailable{{cfg::Index(cfg::Param::HearingLandingEnabled),cfg::Index(cfg::Param::HearingLandingRangeCm),
            cfg::Index(cfg::Param::HearingOtherCombatEnabled),cfg::Index(cfg::Param::HearingOtherRangeCm),
            cfg::Index(cfg::Param::HearingSuppressedMultiplier),cfg::Index(cfg::Param::MeleeUseWeight),
            cfg::Index(cfg::Param::MeleeEngageDistanceCm),cfg::Index(cfg::Param::MeleeExtraWaitMs),
            cfg::Index(cfg::Param::FirstMissContinueWeight),cfg::Index(cfg::Param::FirstMissStrafeWeight),
            cfg::Index(cfg::Param::FirstMissCrouchWeight),cfg::Index(cfg::Param::FirstMissRetreatWeight)}};
        bool comma=false;for(const auto id:unavailable){Append(out,"%s%zu",comma?",":"",id);comma=true;}
        out+="],\"conditional\":[";comma=false;
        for(size_t id=0;id<cfg::ParamCount;++id){bool skip=false;for(const auto unavailableId:unavailable)if(id==unavailableId)skip=true;
            if(!skip){Append(out,"%s%zu",comma?",":"",id);comma=true;}}
        out+="],\"notes\":[\"视觉露出是采样点比例，并非渲染面积\",\"脚步来自已观测原Tick或原请求，听觉仍为逻辑传播模型；落地OnLanded直调分支尚无稳定回执，其开关和距离已锁定\",\"未获完整发射终止证据时不会以缺少命中推断未命中\",\"近战攻击与其他战斗声、消音倍率尚无可靠消费者\"]}";
        return out;
    }();return value;
}
inline std::string SummaryJson(){
    auto result=human_bot_store::SummaryJson();if(result.empty()||result.back()!='}')return result;
    result.pop_back();result+=",\"behavior\":";
    AcquireSRWLockShared(&cacheLock);result+=behaviorJson;ReleaseSRWLockShared(&cacheLock);
    result+=",\"capabilities\":";result+=CapabilityJson();
    result+='}';return result;
}
inline bool Initialize(){
    if(!original_players::Thread()||!local_server_match::GetState().initialized)return false;
    state={};for(auto&p:players)p={};state.initialized=true;
    state.stage="humanlike_bots_waiting_validated_profile_group";Cache(0,true);return true;
}
inline bool AcknowledgeIfReleased(){
    if(!state.ackPending)return true;
    for(size_t n=1;n<state.profileCount;++n)if(players[n].releasePending)return true;
    const auto*current=human_bot_store::Configuration();
    // A later complete transaction may supersede an unacknowledged disable.
    // Its normal observation boundary will adopt it; never ACK the wrong group.
    if(!current||current->revision!=state.revision||current->contentChecksum!=state.fingerprint)return true;
    if(!human_bot_store::AckConsumerRevision(state.revision))return Fail(7755,"humanlike_consumer_ack_did_not_match_active_group");
    state.ackPending=false;return true;
}
inline bool AdoptConfiguration(){
    const auto*h=human_bot_store::Configuration();if(!h)return true;
    if(state.configured&&h->contentChecksum==state.fingerprint&&h->revision==state.revision)return AcknowledgeIfReleased();
    const auto count=local_server_match::GetState().wantedPlayers;
    if(!count||h->profileCount!=count||count>MaxPlayers)return Fail(7750,"humanlike_profile_roster_requires_matching_native_session");
    for(uint32_t n=0;n<count;++n){const auto*r=human_bot_store::FindRecord(n);
        if(!r||r->rosterIndex!=n||r->stableBotId!=n||!cfg::Validate(r->profile))return Fail(7751,"humanlike_validated_profile_identity_missing");}
    const bool enabled=(h->flags&human_bot_wire::Enabled)!=0&&local_server_match::FreeForAll();
    const bool wasEnabled=state.enabled;
    for(uint32_t n=0;n<count;++n){const auto*r=human_bot_store::FindRecord(n);profiles[n]=r->profile;
        if(n==0)continue;auto&p=players[n];
        if(!p.configured){if(!brains[n].Initialize(r->profile,h->seed,r->stableBotId,h->revision))return Fail(7752,"humanlike_brain_initial_profile_rejected");}
        else if(!brains[n].ApplyConfiguration(r->profile,h->revision,enabled).accepted)return Fail(7753,"humanlike_brain_live_profile_rejected");
        if(p.configured&&state.seed!=h->seed)brains[n].Reseed(h->seed);
        brains[n].SetEnabled(enabled);p.releaseLegacy=p.releaseLegacy||(wasEnabled&&!enabled);
        if(wasEnabled&&!enabled){const auto*owned=local_server_npc_input::Player(n);
            if(owned&&owned->present&&owned->hasIntent){p.releasePending=true;p.releasePawn=owned->pawn;p.releaseEpoch=owned->epoch;}}
        p.configured=true;p.stableId=r->stableBotId;p.revision=h->revision;
    }
    state.configured=true;state.enabled=enabled;state.profileCount=count;
    const bool seedChanged=state.seed!=h->seed;
    state.revision=h->revision;state.seed=h->seed;state.fingerprint=h->contentChecksum;state.ackPending=true;
    ++state.configurations;
    if(state.enabled&&!state.sensorsReady){
        if(!sensors::Initialize(state.seed,ReadProfile,space::ReadSpace)||!space::Initialize(state.seed))return Fail(7754,"humanlike_native_sensor_initialization_failed");
        visibleImpacts.Reset(ReadProfile);sensors::SetFeedbackPass(humanlike_bot_visible_impacts::Bridge::Pass,&visibleImpacts);
        if(!local_server_humanlike_audio_requests::InitHost(local_server_match::GetState().world))
            return Fail(7760,"humanlike_native_audio_request_observers_failed");
        if(!local_server_humanlike_audio_tick::InitHost(local_server_match::GetState().world))
            return Fail(7774,"humanlike_native_audio_tick_observer_initialization_failed");
        state.sensorsReady=true;
    }
    if(seedChanged&&state.sensorsReady&&(!sensors::Reseed(state.seed)||!space::Reseed(state.seed)))
        return Fail(7763,"humanlike_seed_consumer_context_not_ready");
    if(!AcknowledgeIfReleased())return false;
    state.stage=state.enabled?"humanlike_profiles_adopted_native_behavior_pending":"humanlike_disabled_group_adopted_legacy_behavior";
    Cache(local_server_match::GetState().now,true);return true;
}
inline bool SensorPass(double now){
    if(!Context())return false;if(!AdoptConfiguration())return false;++state.passes;
    if(state.sensorsReady&&!local_server_humanlike_audio_requests::Poll())
        return Fail(7761,"humanlike_native_audio_request_observer_generation_changed");
    if(state.sensorsReady&&!local_server_humanlike_audio_tick::Poll())
        return Fail(7775,"humanlike_native_audio_tick_source_lifecycle_changed");
    if(state.enabled&&!sensors::Pass(now))return Fail(7756,"humanlike_native_perception_pass_failed");
    Cache(now);return true;
}
inline dto::Intent TickPureBrain(size_t index,const dto::SensorFrame&frame){
    // Per-bot calls sum pure computation only; sensing and input submission are outside.
    armor_poll_performance::Scope measurement(armor_poll_performance::Slot::PureBrainTick);
    return brains[index].Tick(frame);
}
inline bool Decide(size_t index,double now,Frame&frame,bool&handled){
    handled=false;if(!Context())return false;
    if(index==0||index>=state.profileCount){++state.humanRejections;return index!=0;}
    auto&p=players[index];p.navigationRequested=false;p.navigationEstimate={};p.aimEstimate={};
    if(!state.enabled)local_server_hold_fixture::GroupDisabled(index,brains[index]);
    if(p.releaseLegacy){p.releaseLegacy=false;const auto rotation=frame.rotation;frame={};frame.rotation=rotation;handled=true;return true;}
    if(!state.enabled&&!local_server_hold_fixture::OwnsInterruption(index))return true;handled=true;
    const auto*f=sensors::Frame(index);if(!f)return Fail(7757,"humanlike_current_owned_sensor_frame_missing");
    local_server_hold_fixture::Prepare(index,*f,brains[index],profiles[index]);
    const auto intent=TickPureBrain(index,*f);
    if(local_server_peek_fixture::context.configured)local_server_peek_fixture::Observe(index,*f,brains[index].Snapshot(),brains[index].Memory(brains[index].Snapshot().target),profiles[index]);
    p.navigationRequested=intent.navigationRequested;p.navigationEstimate=intent.estimatedNavigation;p.aimEstimate=intent.estimatedAim;
    // false means release, never 'leave old input held'. Rotation is optional.
    const auto priorRotation=frame.rotation;frame={};frame.rotation=priorRotation;
    if(intent.rotationRequested)frame.rotation={intent.rotation.pitch,intent.rotation.yaw,0};
    if(!intent.releaseAll){frame.worldDirection={intent.worldDirection.x,intent.worldDirection.y,intent.worldDirection.z};
        frame.walk=intent.walk;frame.crouch=intent.crouch;frame.jump=intent.jump;frame.fire=intent.fire;frame.reload=intent.reload;}
    ++p.decisions;++state.decisions;p.lastDecision=now;
    if(humanlike_bot::math::Horizontal(intent.worldDirection)>0)++p.moveSamples;
    if(intent.crouch)++p.crouchSamples;if(intent.jump)++p.jumpSamples;
    if(f->self.moveObserved)++p.nativeMoveSamples;if(f->self.crouchObserved)++p.nativeCrouchSamples;if(f->self.jumpObserved)++p.nativeJumpSamples;
    ++p.frameSamples;if(f->self.groundedKnown&&f->self.grounded)++p.groundSamples;
    if(f->space.coverKnown)++p.coverSamples;if(f->space.clearanceKnown)++p.clearanceSamples;
    if(f->self.groundedKnown&&f->self.grounded&&f->space.coverKnown&&f->space.clearanceKnown)++p.groundCoverClearanceSamples;
    if(brains[index].Snapshot().peek!=dto::PeekKind::None)++p.activePeekSamples;
    if(intent.jump)++p.jumpIntentSamples;if(intent.reload)++p.reloadIntentSamples;
    if(f->self.weapon.reloadKnown&&f->self.weapon.reloading)++p.reloadPositiveSamples;
    for(size_t n=0;n<f->soundCount&&n<f->sounds.size();++n)if(f->sounds[n].valid&&f->sounds[n].kind==dto::SoundKind::Footstep)++p.footstepCueSamples;
    const auto&w=f->self.weapon;
    if(w.valid&&w.binding&&p.lastWeaponBinding==w.binding&&p.lastWeaponEpoch==f->self.life.spawnEpoch){
        if(w.ammoKnown&&p.lastMagazine>=0&&w.magazine>p.lastMagazine)++p.magazineIncreaseSamples;
        if(w.reserveKnown&&p.lastReserve>=0&&w.reserve<p.lastReserve)++p.reserveDecreaseSamples;
    }
    p.lastWeaponBinding=w.valid?w.binding:0;p.lastWeaponEpoch=f->self.life.spawnEpoch;
    p.lastMagazine=w.ammoKnown?w.magazine:-1;p.lastReserve=w.reserveKnown?w.reserve:-1;
    training_bot_controller::actors[index].target=-1;
    training_bot_controller::actors[index].reason=Reason(brains[index].Snapshot().reason);
    state.stage="humanlike_single_authority_decision_path_active";return true;
}
inline bool BeforeRetire(size_t index,void*pawn,uint32_t epoch){
    if(!local_server_humanlike_audio_tick::RetirePlayer(index,pawn,epoch))return false;
    if(!Context()||index==0)return true;
    space::RetireAim(index,epoch);
    local_server_hold_fixture::Retiring(index,epoch,brains[index]);
    return humanlike_bot_weapon_feedback::RetireOwnLife(index,pawn,epoch);
}
// Retain an already reached static goal while its same-life authored-hold
// estimate fires. This is not navigation demand or permission to move/fire.
inline bool RetainArrivedHoldGoal(size_t index,const Player&p,const Frame&frame,const dto::SensorFrame*f,double now){
    if(!index||index>=state.profileCount||index>=MaxPlayers||!state.enabled||!p.configured||!f||!frame.fire
       ||p.lastDecision!=now||f->self.nowSeconds!=now||frame.reload||frame.jump||frame.crouch
       ||!original_players::Finite(frame.worldDirection)||frame.worldDirection.x!=0||frame.worldDirection.y!=0||frame.worldDirection.z!=0)return false;
    const auto&own=f->self;const auto&goal=p.navigationEstimate;const auto&aim=p.aimEstimate;
    const auto&profile=profiles[index];
    const double ownAge=cfg::Get(profile,cfg::Param::OwnSnapshotMaxAgeMs)*.001;
    if(!own.movementValid||!dto::math::Finite(own.velocity)||own.velocity.x!=0||own.velocity.y!=0||own.velocity.z!=0
       ||own.life.stableBotId!=index||p.stableId!=own.life.stableBotId||!std::isfinite(own.sampledAtSeconds)
       ||own.sampledAtSeconds>now||now-own.sampledAtSeconds>ownAge
       ||goal.purpose!=dto::NavigationPurpose::MapHold||!humanlike_bot_tactics_navigation::ValidRequest(goal,own,now)
       ||!humanlike_bot_tactics_navigation::SameRequest(goal,f->space.estimatedNavigation)
       ||!f->space.estimatedGoalReached||f->space.estimatedGoalRejected||!f->space.holdProofReady
       ||!humanlike_bot_hold_positions::Arrived(own,goal.estimatedGoal,cfg::Get(profile,cfg::Param::MoveArrivalCm)))return false;
    if(aim.source!=dto::EstimatedAimSource::AuthoredHold||aim.sourceEpisode!=goal.key||aim.mapAnnotationId!=goal.mapAnnotationId
       ||aim.mapRevision!=goal.mapRevision||now<aim.readyAtSeconds)return false;
    return humanlike_bot_prefire::Proof(aim,*f,(std::min)(ownAge,cfg::Get(profile,cfg::Param::FirePrefireMaxAgeMs)*.001));
}
inline bool Submitted(size_t index,const Frame&frame){
    if(!Context()||index==0||index>=state.profileCount)return true;
    auto&p=players[index];
    const auto*f=state.enabled?sensors::Frame(index):nullptr;
    const bool aimAllowed=state.enabled&&p.lastDecision==local_server_match::GetState().now&&!frame.reload&&!frame.jump;
    space::AimSubmitted(index,local_server_match::GetState().now,f?&f->self:nullptr,aimAllowed?&p.aimEstimate:nullptr);
    const bool navigationAllowed=state.enabled&&p.lastDecision==local_server_match::GetState().now
        &&!frame.fire&&!frame.reload&&!frame.jump&&!frame.crouch;
    const bool retainHoldGoal=RetainArrivedHoldGoal(index,p,frame,f,local_server_match::GetState().now);
    const bool goalAllowed=navigationAllowed||retainHoldGoal;
    space::NavigationSubmitted(index,local_server_match::GetState().now,f?&f->self:nullptr,
        navigationAllowed&&p.navigationRequested,goalAllowed?&p.navigationEstimate:nullptr);
    local_server_hold_fixture::Submitted(index,frame,f,brains[index],navigationAllowed&&p.navigationRequested,
        goalAllowed?p.navigationEstimate:dto::EstimatedNavigation{});
    if(!p.releasePending)return true;
    const auto*owned=local_server_npc_input::Player(index);
    if(!owned||owned->pawn!=p.releasePawn||owned->epoch!=p.releaseEpoch)return true;
    if(frame.fire||frame.reload||frame.jump||frame.crouch||frame.walk||training_bot_controller::Length2D(frame.worldDirection)>0)return true;
    p.releasePending=false;return AcknowledgeIfReleased();
}
inline bool Retired(size_t index,void*pawn,uint32_t epoch){
    if(!Context()||index==0||index>=state.profileCount)return true;
    auto&p=players[index];if(p.releasePending&&p.releasePawn==pawn&&p.releaseEpoch==epoch)p.releasePending=false;
    return AcknowledgeIfReleased();
}
inline bool Close(){
    if(state.closed)return true;
    if(!local_server_humanlike_audio_tick::Close())return false;
    if(!local_server_humanlike_audio_requests::Close())return false;
    space::ClearAimRequests();state.closed=true;state.enabled=false;state.stage="humanlike_bots_closed";
    humanlike_bot_native_events::EndSession();humanlike_bot_point_impacts::EndSession();Cache(local_server_match::GetState().now,true);
    return true;
}
} // namespace local_server_humanlike_bots
