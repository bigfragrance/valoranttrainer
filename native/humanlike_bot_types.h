#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

// Portable observation/decision contract, version 1. All time fields ending
// in Seconds are in caller-supplied match.now; opaque native IDs are not time.
// Only this bot's actual native identity/state and already acquired opponent
// observations may cross this boundary. No raw actor pointers/global roster.
namespace humanlike_bot {
inline constexpr uint32_t ContractVersion=1;
inline constexpr size_t MaxContacts=16,MaxVisiblePoints=8,MaxSounds=16;
inline constexpr size_t MaxHurtEvents=8,MaxShotReceipts=32,MaxFeedback=16;
inline constexpr size_t MaxRetiredTracks=16,MaxAngles=16,TraceCapacity=256;
inline constexpr size_t MaxHoldLocations=16;
using TrackId=uint64_t;
struct Vec3 {float x=0,y=0,z=0;};
struct Rotation {float pitch=0,yaw=0;};
struct OwnLife {uint32_t stableBotId=0,spawnEpoch=0;uint64_t token=0;};
enum class WeaponKind:uint8_t {Unknown,Pistol,SMG,Rifle,Sniper,Shotgun,Machinegun,Melee,Ability};
enum class Visibility:uint8_t {Unknown,Deferred,Occluded,Visible};
enum class PointKind:uint8_t {Eye,NativeHead,NativeBody,CapsuleChestApprox,CapsuleShoulderApprox,OtherApprox};
enum class Exposure:uint8_t {Unknown,SampledHeadOnly,SampledPartial,SampledBroad};
enum class SoundKind:uint8_t {Footstep,Gunshot,Landing,OtherCombat};
enum class SoundEvidence:uint8_t {Unknown,NativeShotGeometryLogicalPropagation,OriginalAudioRequestLogicalPropagation,AuthoredAudioRequestLogicalPropagation,ActualAudioPostLogicalPropagation};
enum class PeekKind:uint8_t {None,Wide,Short,SlowCreep,Jump,Crouch,Poppin,Shoulder,InfoRepeek};
enum class AimStage:uint8_t {Idle,React,Primary,Secondary,Micro,Track,Switch};
enum class Attention:uint8_t {Focused,Normal,Distracted,Recovering};
enum class FireMode:uint8_t {None,Single,Double,Burst,Spray,MovingClose};
enum class MotionStage:uint8_t {Idle,Approach,Preaim,Out,Brake,Engage,Reverse,Return,Recover};
enum class Activity:uint8_t {Dead,Respawn,Roam,Search,Hold,Engage,Information,Retreat,Relocate,Reload};
enum class Reason:uint16_t {
    None,InvalidTime,InvalidOwnLife,OwnStateUnavailable,NativeActionUnavailable,Dead,Retired,Paused,
    ProfileUnavailable,ProfileInvalid,PerceptionUnavailable,NoAcquiredTarget,VisualUnknown,VisualDeferred,
    RecognitionPending,OutsideFieldOfView,ExposureInsufficient,MemoryExpired,ReactionPending,AttentionLapse,
    TurnPending,AimOutsideTolerance,TargetLost,TargetRetired,TargetSwitchPending,WeaponUnknown,
    WeaponBindingChanged,AmmoUnknown,EmptyMagazine,ReloadUnknown,ReloadPending,WeaponRecoveryUnknown,
    WeaponNotRecovered,MovementUnknown,MovementTooFast,ActualShotFeedUnavailable,ActualShotPending,
    ShotOverflow,ShotWindowComplete,TriggerRelease,NavigationUnavailable,CoverUnavailable,
    ClearanceUnavailable,JumpLandingUnproven,JumpNotObserved,CrouchNotObserved,Collision,Stalled,
    MultipleThreats,HurtDirectionUnknown,HearingUnavailable,RetreatUnavailable,ActionInterrupted,
    PeekPurposeComplete,InputNotExecution,ParameterUnsupported,TraceOverflow,ProfileChanged,
    GroundingUnknown,PermissionRejected,PoppinEvidenceMissing,Disabled,NativeShotOvershoot,NativeBurstUnproven,
    NativeReleaseUnproven,ReloadNotObserved,ActualMissFeedbackUnavailable,ObservationHistoryUnavailable
};
struct WeaponReadback {
    WeaponKind kind=WeaponKind::Unknown;uint64_t binding=0;
    uint16_t catalogWeaponIndex=0xffff; // frozen original catalog index, never inferred from kind
    bool valid=false,ammoKnown=false,reserveKnown=false,canFireKnown=false,canFire=false;
    bool reloadKnown=false,reloading=false,recoveryKnown=false,recoilKnown=false,shotFeedAvailable=false;
    bool fireBehaviorKnown=false,automatic=false,nativeBurst=false,nativeBurstSizeKnown=false;
    bool triggerHeldKnown=false,triggerHeld=false;uint16_t nativeBurstSize=0;
    int32_t magazine=-1,magazineMaximum=-1,reserve=-1;
    float firingErrorDegrees=0,movementErrorDegrees=0,pitchRecoilDegrees=0,yawRecoilDegrees=0;
    double sampledAtSeconds=0;
};
struct OwnSnapshot {
    OwnLife life{};double nowSeconds=0,sampledAtSeconds=0;
    bool valid=false,alive=false,paused=false,retired=false;
    bool eyeValid=false,viewValid=false,movementValid=false,stanceValid=false,healthKnown=false,shieldKnown=false;
    Vec3 eye{},feet{},velocity{};Rotation view{};
    // Canonical original FullRecoil view used by the firing ray. Native
    // adapters require this separately from a visualization camera view.
    bool shootingViewRequired=false,shootingViewKnown=false;
    Rotation shootingView{};double shootingViewSampledAtSeconds=0;
    float nativeMaxSpeedCmS=0,health=0,maxHealth=0,ownShield=0,maxShield=0;
    bool groundedKnown=false,collisionKnown=false,stalledKnown=false,nativeViewFovKnown=false;
    bool grounded=false,crouched=false,walking=false,collision=false,stalled=false;
    float nativeHorizontalFovDegrees=0;
    // Original own active-blind boolean, not flash opacity or complete rendered visibility.
    bool nativeBlindnessKnown=false,nativeBlinded=false;
    uint64_t nativeFrame=0,lastConsumedIntentSerial=0; // opaque native/input readback only
    bool consumptionKnown=false,jumpObserved=false,crouchObserved=false,moveObserved=false;
    WeaponReadback weapon{};
    // Time this view was acquired by an existing own Eyes/Camera read. This
    // is not a hardware/render timestamp, engine tick or command-consumption ACK.
    bool viewSampleTimeKnown=false;double viewSampledAtSeconds=0;
};
struct VisiblePoint {Vec3 point{};PointKind kind=PointKind::Eye;float confidence=0;};
struct VisualContact {
    TrackId track=0;double observedAtSeconds=0;
    double firstExposedAtSeconds=0,recognizedAtSeconds=0;
    Visibility visibility=Visibility::Unknown;Exposure exposure=Exposure::Unknown;
    bool permitted=false,recognizedBySensor=false,visibilitySamplesKnown=false,motionKnown=false,postureKnown=false;
    std::array<VisiblePoint,MaxVisiblePoints> points{};uint8_t pointCount=0,testedPointCount=0;
    Vec3 observedVelocity{};bool observedCrouched=false,observedAirborne=false;
    bool firingVisuallyObserved=false;float confidence=0;
    // A point-sample ratio is not rendered model area or a screen-space percent.
};
struct SoundCue {
    uint64_t serial=0;SoundKind kind=SoundKind::OtherCombat;double acquiredAtSeconds=0;
    SoundEvidence evidence=SoundEvidence::Unknown; // logical propagation, never a Wwise audible receipt
    double firstEvidenceAtSeconds=0;bool evidenceTimeKnown=false;
    bool valid=false,directionKnown=false,distanceBandKnown=false,ownSound=false;
    float bearingDegrees=0,bearingUncertaintyDegrees=0; // world yaw [-180,180], never relative to view
    float minimumDistanceCm=0,maximumDistanceCm=0,confidence=0;
    // No source identity, exact world position, enemy ammunition or hidden state.
};
struct HurtCue {
    uint64_t serial=0;OwnLife life{};double acquiredAtSeconds=0;
    bool valid=false,directionKnown=false;float bearingDegrees=0,bearingUncertaintyDegrees=0; // world yaw
    float ownHealthLoss=0,ownShieldLoss=0,confidence=0;
};
struct ShotReceipt {
    OwnLife life{};uint64_t serial=0,binding=0,nativeOrigin=0;
    double acquiredAtSeconds=0;bool valid=false,actualOriginGeometryPaired=false;
    // One actual origin is one shot. Pellets/rays/ammo deltas are not shots.
};
struct Feedback {
    enum class Kind:uint8_t {Hit,Kill,OwnDeath,WeaponChanged,Miss};
    enum class Provenance:uint8_t {Unspecified,NativePerceptible,ProjectVisibleNativeContact};
    Kind kind=Kind::Hit;uint64_t serial=0;OwnLife life{};
    double acquiredAtSeconds=0;bool valid=false,nativeFeedbackObserved=false;
    TrackId alreadyKnownTrack=0; // zero if native feedback did not identify an acquired track
    uint64_t binding=0,nativeOrigin=0; // required for a proven terminal own-shot miss
    Provenance provenance=Provenance::Unspecified;
    bool nativeContactObserved=false,visibleImpactProven=false,wholeOriginComplete=false,perceivedMissProven=false;
    float confidence=1,angularUncertaintyDeg=0;
};
struct RetiredTrack {TrackId track=0;double learnedAtSeconds=0;bool valid=false;};
struct AngleHint {
    uint32_t id=0;bool valid=false,staticMapKnowledge=false;
    bool conventional=true;
    Vec3 focusPoint{};float risk=0,heightConfidence=0;
    bool authoredStatic=false;uint32_t mapRevision=0;
    bool coverFocusProven=false; // same completed closed/open cover proof; never enemy data
    bool routeDirected=false; // current own path with a clear, current head-height geometry ray
};
enum class EstimatedAimSource:uint8_t {None,Heard,AuthoredAngle,AuthoredHold};
// Internal prefire hypothesis, never a visual contact, Track or hit receipt.
struct EstimatedAim {
    bool requested=false;uint64_t key=0,sourceEpisode=0;OwnLife life{};
    EstimatedAimSource source=EstimatedAimSource::None;Vec3 point{};
    uint32_t mapAnnotationId=0,mapRevision=0;
    double acquiredAtSeconds=0,readyAtSeconds=0,expiresAtSeconds=0;
    float confidence=0,uncertaintyCm=0;
};
// Internal sole-navigation request DTO. Never serialized into the MMF wire.
// Acquired destinations are fallible local-floor estimates; MapHold is an
// explicitly authored static map point. Neither grants unproved movement.
enum class NavigationPurpose:uint8_t {None,SoundApproach,ThirdPartyJoin,MapHold};
enum class NavigationKnowledge:uint8_t {AcquiredEstimate,StaticMap};
enum class HoldRange:uint8_t {Unknown,Near,Mid,Far};
// An authored map proposal. No enemy location or completed geometry proof.
struct HoldLocation {
    uint32_t id=0,revision=0;bool valid=false,staticMapKnowledge=false,conventional=true;
    Vec3 position{},cover{},retreat{},focus{};HoldRange range=HoldRange::Unknown;
    float confidence=0,risk=1,flankRisk=1;uint32_t exposedAngles=0;
    bool riskKnown=false,flankRiskKnown=false,exposedAnglesKnown=false;
};
struct EstimatedNavigation {
    bool requested=false,usesHearing=false;uint64_t key=0;OwnLife life{};
    NavigationPurpose purpose=NavigationPurpose::None;Vec3 estimatedGoal{};
    double evidenceAtSeconds=0,readyAtSeconds=0,expiresAtSeconds=0;
    float confidence=0,uncertaintyCm=0;
    NavigationKnowledge knowledge=NavigationKnowledge::AcquiredEstimate;
    uint32_t mapAnnotationId=0,mapRevision=0;
};
struct SpaceHints {
    EstimatedAim estimatedAim{};bool estimatedAimKnown=false,estimatedAimClear=false;
    Vec3 estimatedAimEye{};double estimatedAimAtSeconds=-1;
    bool navigationKnown=false,pathProven=false,groundProven=false;
    Vec3 nextPathPoint{},routeGoal{};uint32_t routeId=0;
    EstimatedNavigation estimatedNavigation{};
    bool estimatedGoalReached=false,estimatedGoalRejected=false;
    std::array<HoldLocation,MaxHoldLocations> holdLocations{};uint8_t holdLocationCount=0;
    uint32_t authoredHoldCount=0;bool holdProofReady=false;
    bool coverKnown=false,retreatPathProven=false,peekPathProven=false,landingProven=false;
    bool landingSupportKnown=false,peekSightKnown=false,peekSightClear=false;
    Vec3 coverStart{},peekOpen{},retreatPoint{},jumpLanding{};
    bool clearanceKnown=false;float leftClearanceCm=0,rightClearanceCm=0,forwardClearanceCm=0;
    float coverConfidence=0,exposureRisk=0;uint32_t exposedAngleCount=0;
    std::array<AngleHint,MaxAngles> angles{};uint8_t angleCount=0;
    // Static/own geometry proofs only. No hidden/current opponent coordinates.
};
struct SensorFrame {
    uint32_t version=ContractVersion;uint64_t serial=0;
    OwnSnapshot self{};double perceivedAtSeconds=0;
    bool perceptionValid=false,hearingAvailable=false,eventFeedValid=false;
    bool shotsOverflow=false,eventsOverflow=false,perceptionBudgetDeferred=false;
    std::array<VisualContact,MaxContacts> contacts{};uint8_t contactCount=0;
    std::array<SoundCue,MaxSounds> sounds{};uint8_t soundCount=0;
    std::array<HurtCue,MaxHurtEvents> hurt{};uint8_t hurtCount=0;
    std::array<ShotReceipt,MaxShotReceipts> shots{};uint8_t shotCount=0;
    std::array<Feedback,MaxFeedback> feedback{};uint8_t feedbackCount=0;
    std::array<RetiredTrack,MaxRetiredTracks> retiredTracks{};uint8_t retiredTrackCount=0;
    SpaceHints space{};
};
struct Intent {
    EstimatedAim estimatedAim{};
    uint64_t serial=0;OwnLife life{};bool rotationRequested=false;
    Rotation rotation{};Vec3 worldDirection{};
    bool walk=false,crouch=false,jump=false,fire=false,reload=false,releaseAll=true;
    // Reached the existing navigation branch; neutral movement is still a request
    // when XY has arrived but the original path node is on a different level.
    bool navigationRequested=false;
    EstimatedNavigation estimatedNavigation{};
};
struct Trace {
    uint64_t serial=0;double atSeconds=0;TrackId target=0;
    uint64_t requestedIntentSerial=0,consumedIntentSerial=0;
    Feedback::Provenance feedbackProvenance=Feedback::Provenance::Unspecified;
    float feedbackConfidence=0,feedbackAngularUncertaintyDeg=0;
    Activity activity=Activity::Dead;AimStage aim=AimStage::Idle;
    PeekKind peek=PeekKind::None;MotionStage motion=MotionStage::Idle;
    FireMode fireMode=FireMode::None;Attention attention=Attention::Normal;
    Reason reason=Reason::None;uint32_t actualShotsInWindow=0;
    float aimErrorDegrees=0,speedCmS=0,confidence=0;
    bool inputRequested=false,nativeExecutionObserved=false,timeKnown=false;
};
} // namespace humanlike_bot
