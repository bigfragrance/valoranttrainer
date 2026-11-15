#pragma once
#include "humanlike_bot_types.h"
#include "humanlike_bot_math.h"
#include "humanlike_bot_config.h"
#include "humanlike_bot_tactics_navigation.h"
#include "humanlike_bot_hold_positions.h"
#include "humanlike_bot_prefire.h"
#include "humanlike_bot_navigation_progress.h"
#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>

namespace humanlike_bot {
namespace cfg=humanlike_bot_config;
using Param=cfg::Param;
struct ProfileUpdate {bool accepted=false,changed=false;uint32_t revision=0,interruptedDomains=0;};
struct MemoryView {
    bool used=false,visible=false,retired=false;TrackId track=0;
    Vec3 point{},velocity{};float confidence=0,uncertaintyCm=0;
    double lastSeenSeconds=0,firstEvidenceSeconds=0,recognizedSeconds=0;
    Exposure exposure=Exposure::Unknown;
};
// One current/last same-life own peek plan, diagnostic storage only.
// Coordinates are static/own world-feet plans, never target coordinates.
struct PeekPlanSnapshot {
    uint64_t firstIntentSerial=0,lastIntentSerial=0,firstTraceSerial=0,lastTraceSerial=0;
    double startedAtSeconds=0,stageAtSeconds=0,observedAtSeconds=0,endedAtSeconds=-1,terminalOwnSampledAtSeconds=-1;
    Vec3 start{},goal{},retreat{},terminalOwnFeet{};
    uint32_t revision=0;uint16_t stageMaskAtNote=0;
    PeekKind peek=PeekKind::None;MotionStage lastActiveMotion=MotionStage::Idle;
    bool known=false,active=false,terminalOwnKnown=false,terminalGroundKnown=false,terminalGrounded=false;
    bool jumpRequested=false,airSeen=false,landed=false,crouchSeen=false,informationSeen=false,returnChosen=false,poppinExtended=false;
};
struct BrainSnapshot {
    EstimatedAim estimatedPrefire{};bool estimatedPrefireStarted=false;
    bool configured=false;OwnLife life{};uint32_t revision=0;
    Activity activity=Activity::Dead;AimStage aim=AimStage::Idle;
    PeekKind peek=PeekKind::None;MotionStage motion=MotionStage::Idle;
    Attention attention=Attention::Normal;FireMode fireMode=FireMode::None;
    TrackId target=0;Reason reason=Reason::None;
    uint64_t requestedIntents=0,actualShots=0,duplicateShotReceipts=0,traceOverwritten=0;
    uint32_t actualShotsInWindow=0,windowGoalShots=0;
    bool nativeIntentConsumptionKnown=false;
    PeekPlanSnapshot peekPlan{},lastEndedPeekPlan{};
    bool lastIntentJump=false,lastIntentCrouch=false,lastIntentWalk=false,lastIntentRelease=true;
    Feedback::Provenance feedbackProvenance=Feedback::Provenance::Unspecified;
    float feedbackConfidence=0,feedbackAngularUncertaintyDeg=0;
    double eventAtSeconds=0,noticedAtSeconds=0,decidedAtSeconds=0,actionAtSeconds=0;
    // Last sampled legitimate scenario event, not a native execution receipt.
    cfg::ReactionContext lastReactionContext=cfg::ReactionContext::Count;
    uint64_t lastReactionEvent=0;
    float lastReactionHesitationMs=0,lastReactionYawErrorDeg=0,lastReactionPitchErrorDeg=0;
};

struct CombatReadback {
    bool current=false,targetKnown=false,targetVisible=false,targetRetired=false,headAvailableAtSelection=false;
    bool shootingViewRequired=false,shootingViewKnown=false;double shootingViewAt=0;
    bool requestedFire=false,historyReady=false,window=false,holding=false,needRelease=false;
    TrackId target=0;Vec3 point{};PointKind pointKind=PointKind::Eye;Rotation observedView{},gateView{};
    double now=0,ownSampleAt=0,targetObservedAt=-1,noticeAt=0,actionAt=0,nextFireAt=0;
    float observedErrorDegrees=0,gateErrorDegrees=0,toleranceDegrees=0;
};

// One instance per stable NPC, called by existing TickBots only. No scheduler,
// physics, game API, callback registration, heap allocation or global target
// query is present. Instances/profile banks belong in static/heap storage.
class Brain {
public:
    bool Initialize(const cfg::Profile& profile,uint64_t matchSeed,uint32_t stableBotId,uint32_t revision=1);
    ProfileUpdate ApplyProfile(const cfg::Profile& profile,uint32_t revision);
    ProfileUpdate ApplyConfiguration(const cfg::Profile& profile,uint32_t revision,bool enabled);
    void SetEnabled(bool enabled){if(enabled_!=enabled){enabled_=enabled;Interrupt();releasePending_=true;}}
    bool Reseed(uint64_t matchSeed); // changed only; same seed is idempotent
    bool BeginLife(OwnLife life);
    Intent Tick(const SensorFrame& frame);
    const BrainSnapshot& Snapshot()const{return snapshot_;}
    CombatReadback ReadCombat(const OwnSnapshot& own)const;
    uint64_t HeardSoundEpisode()const{return prefireSoundEpisode_;}
    uint64_t SoundNavigationEpisode()const{return navigationEstimate_.purpose==NavigationPurpose::SoundApproach?navigationFirst_:0;}
    uint64_t FailedSoundNavigationEpisode()const{return failedSound_.episode;}
    const cfg::Profile& Profile()const{return profile_;}
    // Logical settling state only; actual own/physics feedback remains separate.
    bool MapHoldSettled()const{return mapHold_.active&&mapHold_.arrived;}
    bool MapHoldSettling()const{return mapHold_.active&&mapHold_.arrival.active&&!mapHold_.arrived;}
    double MapHoldSettledAt()const{return mapHold_.arrived?mapHold_.arrivedAt:-1;}
    MemoryView Memory(TrackId id)const;
    size_t CopyTrace(uint64_t afterSerial,Trace* destination,size_t capacity,uint64_t& missed)const;
private:
    enum class Domain:uint8_t {General,Attention,Memory,Aim,Fire,Motion,Decision,Count};
    enum class DuelStyle:uint8_t {None,Strafe,DotCrouch,Guangxi,Deadzone};
    struct SeenSample {Vec3 point{},velocity{};double at=-1;bool motionKnown=false;};
    struct ReactionVariation {float hesitationSeconds=0,yawError=0,pitchError=0;double correctionUntil=-1;};
    struct AdvanceGait {bool known=false,walk=false;double until=0;};
    struct ViewCommand {
        double sampleAt=-1,intervalStart=-1;Rotation reference{},command{};TrackId target=0;
        bool exactTime=false,spent=true,commandKnown=false,preaim=false;
    };
    static constexpr size_t SeenHistoryCapacity=64;
    struct Track {
        bool used=false,visible=false,retired=false,lifeRetired=false,motionKnown=false,postureKnown=false,crouched=false,airborne=false,firing=false,reselectPoint=false;
        bool deferredSight=false,headAvailableAtSelection=false;
        TrackId id=0;Vec3 point{},velocity{},previousVelocity{};PointKind pointKind=PointKind::Eye;Exposure exposure=Exposure::Unknown;
        float baseConfidence=0,confidence=0,uncertainty=0;
        double lastSeen=0,firstEvidence=0,recognized=0,lastPacket=-1,motionChangedAt=-1,postureChangedAt=-1,firingAt=-1,reappearedAt=-1;
        ReactionVariation directionReaction{},postureReaction{};
        std::array<SeenSample,SeenHistoryCapacity> history{};size_t historyWrite=0,historyCount=0;
        double informationHandledAt=-1; // consume acquired evidence, never a hidden refresh
    };
    struct Aim {
        AimStage stage=AimStage::Idle;TrackId target=0;unsigned corrections=0;
        float yawError=0,pitchError=0,angularSpeed=0;double stageAt=0,startAt=0,decisionAt=0,noticeAt=0,eventAt=0,jitterAt=0;
        Rotation requestedGoal{};bool endpointKnown=false,historyReady=true;
    };
    struct Fire {
        uint64_t estimateKey=0;
        bool window=false,holding=false,precise=false,reloadRequested=false,reloadObserved=false,needRelease=false,prefire=false,hesitated=false;
        FireMode mode=FireMode::None;uint64_t binding=0;uint32_t count=0,goal=0;
        double startedAt=0,nextAt=0,lastActualAt=-1,lastReleaseAt=-1,reloadAt=0,reloadBeforeAt=-1,reloadPositiveAt=-1,releaseRequestedAt=-1;
        int32_t reloadBefore=-1;uint64_t lastReceiptSerial=0;
    };
    struct ArrivalControl {
        bool active=false,pulsing=false,needPath=false,done=false,failed=false,correctionIssued=false;
        double releasedAt=0,pulseUntil=0,pulseSeconds=0,zeroSince=-1,lastProgressAt=0,lastSampleAt=-1;
        Vec3 lastFeet{};float lastSpeed=0,stoppedDistance=-1;
    };
    struct Motion {
        DuelStyle duel=DuelStyle::None;PeekKind peek=PeekKind::None;MotionStage stage=MotionStage::Idle;
        Vec3 start{},direction{},goal{},retreat{},lastFeet{};int sign=1,lastSign=1;
        double startedAt=0,stageAt=0,lastProgressAt=0,crouchUntil=0,nextCrouchAt=0,jumpAt=0,crouchAt=-1,airSampleAt=-1;
        float distance=0;uint32_t comboLeft=0,shotsAtPhase=0,goalShots=0;
        TrackId informationTrack=0;double informationObservedAt=-1;Vec3 informationProofOpen{},informationFocus{};
        uint32_t informationAngle=0;
        bool informationRepeek=false;
        ArrivalControl arrival{};
        bool jumpRequested=false,airSeen=false,landed=false,crouchRequested=false,crouchSeen=false,returnChosen=false,firstMissHandled=false,informationSeen=false,poppinExtended=false;
    };
    struct ShotKey {OwnLife life{};uint64_t binding=0,origin=0;};
    struct AngleMemory {uint32_t id=0;double checkedAt=-1;};
    cfg::Profile profile_{};
    std::array<std::bitset<cfg::ParamCount>,static_cast<size_t>(Domain::Count)> dependencies_{};
    Domain domain_=Domain::General;
    std::array<Track,MaxContacts> tracks_{};
    std::array<AngleMemory,MaxAngles> angles_{};
    std::array<AngleMemory,MaxAngles> informationAngles_{};
    std::array<ShotKey,128> seenShots_{};size_t nextShotKey_=0;
    std::array<uint64_t,MaxHurtEvents*4> seenHurt_{};size_t nextHurtKey_=0;
    std::array<uint64_t,MaxSounds*4> seenSounds_{};size_t nextSoundKey_=0;
    std::array<uint64_t,MaxFeedback*4> seenFeedback_{};size_t nextFeedbackKey_=0;
    std::array<Trace,TraceCapacity> traces_{};uint64_t traceWrite_=0;
    uint64_t seed_=0;uint32_t stableBotId_=0;math::Rng rng_{};
    std::array<uint64_t,cfg::ReactionContextCount> reactionEvents_{};
    ReactionVariation hurtReaction_{},soundReaction_{};AdvanceGait advanceGait_{};uint64_t advanceGaitEvent_=0;
    OwnLife life_{};bool configured_=false,enabled_=true,releasePending_=false,profileChanged_=false;
    double lastNow_=-1,lifeAt_=-1,lastPerception_=-1,lastTraceAt_=-1,lastAttentionAt_=-1;
    double attentionUntil_=0,lastSalientAt_=0,lastHurtAt_=-1,hurtNoticeAt_=0,hurtDirectionAt_=0,hurtActionAt_=0,lastSoundAt_=-1,soundNoticeAt_=0,lastKillAt_=-1;
    double soundDecisionAt_=0,holdUntil_=0,searchUntil_=0,angleUntil_=0,relocateAt_=-1,hitDecisionAt_=-1,nextDecisionAt_=0;
    uint32_t patrolRouteId_=0,preaimAngleId_=0;
    float hurtBearing_=0,hurtConfidence_=0,soundBearing_=0,soundDistance_=0,soundConfidence_=0;
    bool hurtDirectionKnown_=false,soundDirectionKnown_=false,recentHit_=false,recentMiss_=false,seenAnyTarget_=false,hurtAcknowledged_=false,soundAcknowledged_=false;
    float preaimMargin_=0;uint64_t lastIssuedSerial_=0,lastObservedConsumedSerial_=0;
    double nextCrouchAllowedAt_=0,nextMotionChoiceAt_=0,lastNeutralAt_=-1;int previousMoveSign_=1;
    bool hurtFlank_=false,soundFlank_=false;double soundEventAt_=-1;
    SoundKind lastSoundKind_=SoundKind::OtherCombat;
    TrackId previousAimTarget_=0;
    using TacticEstimate=humanlike_bot_tactics_navigation::Estimate;
    std::array<TacticEstimate,MaxSounds+MaxContacts> combatEstimates_{};
    TacticEstimate soundEstimate_{};EstimatedNavigation navigationEstimate_{};double soundApproachReadyAt_=0;
    uint64_t nextHypothesis_=0,nextNavigationEstimate_=0,soundHandledSerial_=0;
    struct CombatPair {uint64_t first=0,second=0;double readyAt=0;bool decided=false;};
    CombatPair combatPair_{};uint64_t navigationFirst_=0,navigationSecond_=0;
    struct MapHold {HoldLocation location{};bool active=false,arrived=false;double approachUntil=0,approachSampleAt=-1,arrivedAt=-1,leaveAt=-1;Vec3 approachFeet{};ArrivalControl arrival{};};
    MapHold mapHold_{};uint32_t previousHoldId_=0;double nextHoldChoiceAt_=0;
    struct FailedHold {uint32_t id=0,revision=0;Vec3 goal{},escapeGoal{};bool haveEscape=false;};
    FailedHold failedHold_{};
    struct FailedSound {uint64_t episode=0;Vec3 goal{},escapeGoal{};bool haveEscape=false;};
    FailedSound failedSound_{};uint64_t navigationSoundSerial_=0;
    struct Prefire {EstimatedAim aim{};bool sourceCurrent=false,started=false;};
    Prefire estimatedPrefire_{};uint64_t nextPrefire_=0,prefireSoundEpisode_=0,prefireAngleEpisode_=0;
    double prefireSoundAt_=-1;bool prefireSoundProvenance_=false;std::array<uint64_t,4> prefireHandled_{};
    Vec3 relocationGoal_{};uint32_t relocationRouteId_=0;bool relocationGoalKnown_=false;
    uint32_t searchAngle_=0;Attention attention_=Attention::Normal;Activity activity_=Activity::Dead;
    Activity eventActivity_=Activity::Dead;
    Aim aim_{};Fire fire_{};Motion motion_{};BrainSnapshot snapshot_{};
    ViewCommand viewCommand_{};
    bool lastRequestedFire_=false;
    bool ReadShootingView(const OwnSnapshot& own,Rotation& view)const;
    float P(Param p){dependencies_[static_cast<size_t>(domain_)].set(cfg::Index(p));return cfg::Get(profile_,p);}
    float Inclination(Param p){const auto maximum=cfg::Metadata[cfg::Index(p)].maximum;return maximum>0?P(p)/maximum:0;}
    float Milliseconds(Param p){return P(p)*0.001f;}
    bool Enabled(Param p){return P(p)==1;}
    float Tune(const OwnSnapshot& own,cfg::WeaponTuning tuning);
    cfg::Distance Band(const OwnSnapshot& own,float distanceCm);
    Param PeekParameter(PeekKind kind,unsigned field)const;
    ReactionVariation SampleReaction(cfg::ReactionContext context);
    bool AdvanceWalk(double now);
    bool ObserveView(const OwnSnapshot& own,Reason& reason);
    double ViewDelta(double notBefore)const;
    bool ReplayView(const OwnSnapshot& own,TrackId target,bool preaim,Intent& intent);
    void StepView(const OwnSnapshot& own,Rotation goal,float speed,double dt,float smoothing,TrackId target,bool preaim,Intent& intent);
    void Scope(Domain d,bool clear=false){domain_=d;if(clear)dependencies_[static_cast<size_t>(d)].reset();}
    void Interrupt(bool resetAim=true,bool resetMotion=true,bool resetFire=true);
    Intent Neutral(const OwnSnapshot& own,Reason reason);
    Track* Find(TrackId id);
    const Track* Find(TrackId id)const;
    bool FlankBearing(const OwnSnapshot& own,float bearing);
    double DirectionActionAt(double eventAt,double noticedAt,double baselineActionAt,bool flank,Param decide,Param start,double extraStart=0);
    void Receive(const SensorFrame& frame);
    humanlike_bot_tactics_navigation::Settings TacticSettings();
    void ResetTactics();
    void CancelEstimatedNavigation(){navigationEstimate_={};navigationFirst_=navigationSecond_=navigationSoundSerial_=0;mapHold_={};}
    humanlike_bot_hold_positions::Settings HoldSettings();
    bool StartMapHold(const SensorFrame& frame);
    bool ObserveNavigationDeparture(const SensorFrame& frame,Vec3 failedGoal,Vec3& escapeGoal,bool& haveEscape);
    void ObserveHoldDeparture(const SensorFrame& frame);
    void RecordFailedHold();
    void MapHoldTick(const SensorFrame& frame);
    static bool ZeroOwnMotion(const OwnSnapshot& own);
    void BeginArrival(ArrivalControl& state,const OwnSnapshot& own);
    void ArrivalTick(ArrivalControl& state,const OwnSnapshot& own,Vec3 goal,bool correctionPath);
    void TacticsTick(const SensorFrame& frame);
    bool StartEstimatedNavigation(const SensorFrame& frame,const TacticEstimate& first,const TacticEstimate* second,NavigationPurpose purpose,double readyAt);
    void AttentionTick(const SensorFrame& frame,double dt);
    void Remember(const SensorFrame& frame);
    void Decay(double now);
    Track* SelectTarget(const SensorFrame& frame);
    void Acquire(const SensorFrame& frame,Track& target,bool switching);
    void AimTick(const SensorFrame& frame,Track* target,double dt,Intent& intent);
    void Preaim(const SensorFrame& frame,double dt,Intent& intent);
    void Decide(const SensorFrame& frame,Track* target);
    bool InformationMemoryReady(const SensorFrame& frame,const Track& target,bool unhandled);
    const AngleHint* InformationAngleReady(const SensorFrame& frame);
    bool AtPeekCover(const SensorFrame& frame);
    bool InformationPurposeReady(const SensorFrame& frame,Track* target);
    bool PickPeek(const SensorFrame& frame,Track* target);
    void PickDuel(const SensorFrame& frame,Track& target);
    void MotionTick(const SensorFrame& frame,Track* target,Intent& intent);
    bool CanShoot(const SensorFrame& frame,Track& target,FireMode mode,Reason& reason);
    bool WeaponFireReady(const SensorFrame& frame,float distance,FireMode mode,Reason& reason);
    bool MotionAimFireReady(const SensorFrame& frame,Vec3 point,FireMode mode,Reason& reason);
    bool CanEstimatedShoot(const SensorFrame& frame,FireMode mode,Reason& reason);
    void StopEstimatedPrefire(double now,Reason reason=Reason::None);
    void ConsiderEstimatedPrefire(const SensorFrame& frame,EstimatedAim candidate,Rotation& goal);
    void FireTick(const SensorFrame& frame,Track* target,Intent& intent);
    void ReloadTick(const SensorFrame& frame,Intent& intent);
    bool ActionConditions(float anyMask,float allMask,const SensorFrame& frame,Track* target);
    float TargetScore(const SensorFrame& frame,const Track& target);
    float TargetDistance(const OwnSnapshot& own,const Track& target)const{return math::Distance(own.eye,target.point);}
    void MoveTo(const OwnSnapshot& own,Vec3 point,Intent& intent,bool walk=false);
    void Note(const SensorFrame& frame,const Intent& intent,Reason reason);
    void ReasonNow(Reason reason){if(snapshot_.reason==Reason::None)snapshot_.reason=reason;}
};
} // namespace humanlike_bot
#include "humanlike_bot_brain.inl"
