#pragma once
#include "humanlike_bot_types.h"
#include "humanlike_bot_native_events.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

// Portable production helper. Invoked once by the existing match TickBots pass;
// no timer/thread, clock, physics, actor writes or native observer lives here.
// Pose is privileged backend input and MUST NOT be passed to the pure brain.
namespace humanlike_bot_sensors {
using namespace humanlike_bot;
inline constexpr uint32_t MaxPlayers=32,PointCount=4;
inline constexpr float Pi=3.14159265358979323846f;
struct Identity {uint32_t index=0,epoch=0;uintptr_t pawn=0;int32_t weakIndex=-1,weakSerial=0;};
struct Pose {
    Identity identity{};bool valid=false,alive=false,viewValid=false;
    Vec3 eye{},feet{};Rotation view{};float radiusCm=0,halfHeightCm=0;
    bool crouched=false,airborne=false,postureKnown=false,nativeFovKnown=false;
    float nativeHorizontalFovDegrees=0;double sampledAtSeconds=0;
    // Backend identity only. Never crosses into a Brain contact or target point.
    uint64_t weaponBinding=0;bool weaponBindingKnown=false;
    // Combat policy is separate from the unchanged original alive sample.
    bool targetEligible=true;
};
// Values are resolved from shared humanlike_bot_config::Profile. These are not
// defaults or an alternate configuration schema. Safety array capacities above
// are compile-time bounds; every behavioral threshold below comes from Profile.
struct Settings {
    float horizontalFov=0,verticalFov=0,peripheralEdgeDegrees=0,maxDistanceCm=0;
    float centralGain=0,peripheralGain=0,distancePenalty=0,minimumVisibleSampleFraction=0;
    double visualSampleSeconds=0,recognitionSeconds=0,exposureSeconds=0,maxSampleGapSeconds=0;
    float missProbability=0,peripheralMissProbability=0;uint32_t pointSamples=0,minVisiblePoints=0,targetsPerSample=0;
    float chestHeightFraction=0,shoulderHeightFraction=0,shoulderRadiusScale=0;
    double memoryTtlSeconds=0,memoryHalfLifeSeconds=0;
    bool hearingGunshotEnabled=false,hearingFootstepEnabled=false,hearingLandingEnabled=false;
    float hearingRangeCm=0,hearingFootstepRangeCm=0,hearingLandingRangeCm=0;
    float hearingOcclusionScale=0,hearingBearingError=0,hearingDistanceError=0,hearingDistanceBandCm=0;
    float hearingWrongDirectionProbability=0,hurtWrongDirectionProbability=0;
    double hearingDelaySeconds=0,hearingTtlSeconds=0;float hearingMissProbability=0;
    float hurtBearingError=0;uint32_t eventBudget=0;double ownShotMaxAgeSeconds=0;
};
struct Budget {uint32_t rays=0,observers=0,hearingReserve=0;};
enum class Ray:uint8_t {Unknown,Blocked,Clear};
class Provider;struct Hooks;
using FeedbackPass=void(*)(Provider&,double now,Budget&,const Hooks&,void*);
struct Hooks {Ray (*trace)(uint32_t observer,const Vec3& from,const Vec3& to,void* context)=nullptr;void* context=nullptr;
    FeedbackPass feedbackPass=nullptr;void* feedbackContext=nullptr;
    bool (*visibleShotSourceLive)(uint32_t observer,const Identity&own,const Identity&target,uint64_t weaponBinding,void*context)=nullptr;};
struct VisibleShotState {
    uint64_t cursor=0,binding=0,origin=0;double primedAt=-1,cueAt=-1;
    Vec3 position{};bool primed=false;
};
struct Track {
    Identity identity{};uint64_t opaque=0;double firstExposed=-1,lastExposure=-1,recognized=-1,evidenceSeconds=0;
    VisualContact memory{};Visibility latest=Visibility::Unknown;VisibleShotState visibleShot{};
};
struct PendingSound {bool used=false;humanlike_bot_native_events::Event event{};double due=0;};
struct Bot {
    OwnLife own{};Identity ownIdentity{};uint64_t rng=0,frameSerial=0,eventCursor=0;uint32_t targetCursor=0,contactCursor=0;
    double nextSense=0;bool bound=false;
    std::array<Track,MaxPlayers> tracks{};std::array<PendingSound,MaxSounds> pendingSounds{};
    SensorFrame frame{};
};
struct Diagnostics {
    uint64_t passes=0,rays=0,unknownRays=0,budgetDeferred=0,recognitionPending=0;
    uint64_t detections=0,missed=0,retired=0,sounds=0,hearingExpired=0,eventGaps=0;
    uint64_t visibleShotPrimes=0,visibleShotNewEvents=0,visibleShotDerivedSamples=0,visibleShotRejected=0,visibleShotBudgetDrops=0;
};
inline bool Finite(const Vec3& v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
// Feedback consumers deduplicate serials across all provenance streams. Reserve
// two low protocol bits so independent backend queues cannot collide.
inline uint64_t FeedbackSerial(uint64_t sourceSerial,uint8_t producer){
    return sourceSerial&&producer>0&&producer<4&&sourceSerial<=(UINT64_MAX>>2)?(sourceSerial<<2)|producer:0;
}
inline Vec3 Add(const Vec3&a,const Vec3&b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec3 Sub(const Vec3&a,const Vec3&b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec3 Mul(const Vec3&a,float b){return {a.x*b,a.y*b,a.z*b};}
inline float Length(const Vec3&v){return std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);}
inline float Wrap(float degrees){while(degrees>180)degrees-=360;while(degrees<-180)degrees+=360;return degrees;}
inline uint64_t Mix(uint64_t x){x^=x>>30;x*=0xbf58476d1ce4e5b9ULL;x^=x>>27;x*=0x94d049bb133111ebULL;return x^(x>>31);}
inline float Random(Bot& b){b.rng=Mix(b.rng+0x9e3779b97f4a7c15ULL);return static_cast<float>((b.rng>>40)*(1.0/16777216.0));}
inline bool Same(const Identity&a,const Identity&b){return a.index==b.index&&a.epoch==b.epoch&&a.pawn==b.pawn&&a.weakIndex==b.weakIndex&&a.weakSerial==b.weakSerial;}
inline bool Same(const OwnLife&a,const OwnLife&b){return a.stableBotId==b.stableBotId&&a.spawnEpoch==b.spawnEpoch&&a.token==b.token;}
inline bool Matches(const Identity&a,const humanlike_bot_native_events::Subject&b){return a.index==b.index&&a.epoch==b.epoch&&a.pawn==b.pawn&&a.weakIndex==b.weakIndex&&a.weakSerial==b.weakSerial;}
inline bool Probability(float p){return std::isfinite(p)&&p>=0&&p<=1;}
inline bool Valid(const Settings&s){
    return s.horizontalFov>0&&s.horizontalFov<=360&&s.verticalFov>0&&s.verticalFov<=180
        &&s.peripheralEdgeDegrees>=0&&s.peripheralEdgeDegrees<=s.horizontalFov/2&&s.maxDistanceCm>0
        &&s.centralGain>=0&&std::isfinite(s.centralGain)&&s.peripheralGain>=0&&std::isfinite(s.peripheralGain)
        &&s.distancePenalty>=0&&std::isfinite(s.distancePenalty)&&Probability(s.minimumVisibleSampleFraction)
        &&s.visualSampleSeconds>0&&s.recognitionSeconds>=0&&s.exposureSeconds>=0&&s.maxSampleGapSeconds>=s.visualSampleSeconds
        &&s.pointSamples>=1&&s.pointSamples<=PointCount&&s.minVisiblePoints>=1&&s.minVisiblePoints<=s.pointSamples
        &&s.targetsPerSample>=1&&s.targetsPerSample<=MaxPlayers&&Probability(s.missProbability)&&Probability(s.peripheralMissProbability)
        &&s.chestHeightFraction>0&&s.chestHeightFraction<=1&&s.shoulderHeightFraction>0&&s.shoulderHeightFraction<=1
        &&s.shoulderRadiusScale>=0&&s.shoulderRadiusScale<=2&&s.memoryTtlSeconds>=0&&s.memoryHalfLifeSeconds>0
        &&s.hearingRangeCm>=0&&s.hearingFootstepRangeCm>=0&&s.hearingLandingRangeCm>=0
        &&Probability(s.hearingOcclusionScale)&&s.hearingBearingError>=0&&s.hearingBearingError<=180
        &&s.hearingDistanceError>=0&&s.hearingDistanceError<=1&&s.hearingDistanceBandCm>0
        &&s.hearingDelaySeconds>=0&&s.hearingTtlSeconds>=s.hearingDelaySeconds&&Probability(s.hearingMissProbability)
        &&Probability(s.hearingWrongDirectionProbability)&&Probability(s.hurtWrongDirectionProbability)
        &&s.hurtBearingError>=0&&s.hurtBearingError<=180&&s.eventBudget>0&&s.ownShotMaxAgeSeconds>0;
}
inline bool InFov(const Pose&self,const Vec3& point,const Settings&s,bool& peripheral){
    peripheral=false;if(!self.valid||!self.viewValid||!Finite(point))return false;
    const auto delta=Sub(point,self.eye);const float distance=Length(delta);
    if(!std::isfinite(distance)||distance<=0||distance>s.maxDistanceCm)return false;
    const float yaw=std::atan2(delta.y,delta.x)*180/Pi;
    const float pitch=std::atan2(delta.z,std::sqrt(delta.x*delta.x+delta.y*delta.y))*180/Pi;
    float horizontal=s.horizontalFov;
    if(self.nativeFovKnown&&std::isfinite(self.nativeHorizontalFovDegrees)&&self.nativeHorizontalFovDegrees>0)
        horizontal=(std::min)(horizontal,self.nativeHorizontalFovDegrees);
    const float angle=std::abs(Wrap(yaw-self.view.yaw));
    peripheral=angle>s.peripheralEdgeDegrees;
    return angle<=horizontal/2&&std::abs(Wrap(pitch-self.view.pitch))<=s.verticalFov/2;
}
inline std::array<VisiblePoint,PointCount> SamplePoints(const Pose&target,const Settings&s){
    std::array<VisiblePoint,PointCount> result{};
    result[0]={target.eye,PointKind::Eye,1};
    const float height=(std::max)(0.f,target.eye.z-target.feet.z);
    Vec3 chest=target.feet;chest.z+=height*s.chestHeightFraction;
    Vec3 shoulder=target.feet;shoulder.z+=height*s.shoulderHeightFraction;
    const float yaw=target.view.yaw*Pi/180;const Vec3 right{-std::sin(yaw),std::cos(yaw),0};
    const auto width=Mul(right,target.radiusCm*s.shoulderRadiusScale);
    result[1]={chest,PointKind::CapsuleChestApprox,1};
    result[2]={Add(shoulder,width),PointKind::CapsuleShoulderApprox,1};
    result[3]={Sub(shoulder,width),PointKind::CapsuleShoulderApprox,1};return result;
}
class Provider {
    std::array<Bot,MaxPlayers> bots_{};std::array<uint64_t,MaxPlayers> nextTracks_{};
    uint32_t nextObserver_=1;uint64_t seed_=0,activationCursor_=0;
    Diagnostics diagnostics_{};
    void Retire(Bot&b,Track&t,double now){
        if(t.opaque){if(b.frame.retiredTrackCount<b.frame.retiredTracks.size())
            b.frame.retiredTracks[b.frame.retiredTrackCount++]={t.opaque,now,true};else b.frame.eventsOverflow=true;
            ++diagnostics_.retired;}t={};
    }
    Ray Trace(const Hooks&hooks,uint32_t index,const Vec3&from,const Vec3&to,Budget& budget){
        if(!budget.rays||!hooks.trace)return Ray::Unknown;
        --budget.rays;++diagnostics_.rays;const auto result=hooks.trace(index,from,to,hooks.context);
        if(result==Ray::Unknown)++diagnostics_.unknownRays;return result;
    }
    bool VisibleShot(uint32_t observer,const Pose&self,const Pose&target,const Settings&s,Track&t,
                     double now,const Hooks&hooks,Budget&budget){
        // Derived perception, not pixels or a proved muzzle flash: an actual
        // native FireShot/geometry receipt is joined to an already acquired,
        // continuously visible identity, then the current own FOV and original
        // world LOS to that receipt's real firing start must be clear now.
        namespace events=humanlike_bot_native_events;auto&v=t.visibleShot;const auto&own=bots_[observer].frame.self;
        const auto*latest=events::LatestShot(target.identity.index);const auto event=latest?*latest:events::Event{};
        const bool valid=events::diagnostics.installed&&hooks.visibleShotSourceLive&&own.valid&&own.alive&&own.eyeValid&&own.viewValid
            &&self.valid&&self.alive&&self.viewValid&&target.valid&&target.alive&&target.weaponBindingKnown&&target.weaponBinding
            &&std::isfinite(own.sampledAtSeconds)&&own.sampledAtSeconds<=now&&now-own.sampledAtSeconds<=s.maxSampleGapSeconds
            &&std::isfinite(target.sampledAtSeconds)&&target.sampledAtSeconds<=now&&now-target.sampledAtSeconds<=s.maxSampleGapSeconds
            &&!(own.nativeBlindnessKnown&&own.nativeBlinded);
        if(!valid){v={};return false;}
        const bool continuous=t.opaque&&t.memory.permitted&&t.latest==Visibility::Visible
            &&now>=t.memory.observedAtSeconds&&now-t.memory.observedAtSeconds<=s.maxSampleGapSeconds;
        if(!continuous||!v.primed||v.binding!=target.weaponBinding||now<v.primedAt||event.serial<v.cursor){
            v={};v.primed=true;v.primedAt=now;v.binding=target.weaponBinding;v.cursor=event.serial;
            ++diagnostics_.visibleShotPrimes;return false; // first/re-seen/binding change never replays old fire
        }
        bool newEvent=false;
        if(event.serial>v.cursor){
            v.cursor=event.serial;v.cueAt=-1;v.origin=0;newEvent=true;
            if(event.kind!=events::Kind::Shot||!events::SubjectValid(event.subject)||!Matches(target.identity,event.subject)
               ||event.binding!=v.binding||!event.binding||!event.origin||!event.actualGeometryPaired||!Finite(event.position)
               ||!std::isfinite(event.atMatchSeconds)||event.atMatchSeconds<=v.primedAt||event.atMatchSeconds>now
               ||now-event.atMatchSeconds>s.maxSampleGapSeconds||now-event.atMatchSeconds>s.ownShotMaxAgeSeconds){
                ++diagnostics_.visibleShotRejected;return false;}
            v.origin=event.origin;v.cueAt=event.atMatchSeconds;v.position=event.position;
        }
        if(!v.origin||v.cueAt<0)return false;
        if(now<v.cueAt||now-v.cueAt>s.maxSampleGapSeconds||now-v.cueAt>s.ownShotMaxAgeSeconds){v.cueAt=-1;return false;}
        bool peripheral=false;
        if(!InFov(self,v.position,s,peripheral)){v.cueAt=-1;return false;}
        const auto live=[&](){return hooks.visibleShotSourceLive(observer,self.identity,target.identity,v.binding,hooks.context);};
        if(!live()){v={};return false;}
        if(!budget.rays){v.cueAt=-1;bots_[observer].frame.perceptionBudgetDeferred=true;++diagnostics_.visibleShotBudgetDrops;return false;}
        const auto ray=Trace(hooks,observer,self.eye,v.position,budget);
        if(!live()){v={};return false;} // original query may have retired a life/source while forwarding
        if(ray!=Ray::Clear){v.cueAt=-1;return false;}
        if(newEvent)++diagnostics_.visibleShotNewEvents;++diagnostics_.visibleShotDerivedSamples;return true;
    }
    void Sense(uint32_t observer,const Pose&self,const std::array<Pose,MaxPlayers>&poses,const Settings&s,
               double now,const Hooks&hooks,Budget&budget){
        auto& b=bots_[observer];uint32_t tested=0,scanned=0;
        while(scanned<MaxPlayers&&tested<s.targetsPerSample){
            const uint32_t n=b.targetCursor++%MaxPlayers;++scanned;if(n==observer)continue;
            const auto&target=poses[n];auto&t=b.tracks[n];
            if(!target.targetEligible||!target.valid||!target.alive)continue;
            if(!Same(t.identity,target.identity)){Retire(b,t,now);t.identity=target.identity;}
            const auto points=SamplePoints(target,s);bool anyInFov=false;
            for(uint32_t j=0;j<s.pointSamples;++j){bool peripheral=false;anyInFov=anyInFov||InFov(self,points[j].point,s,peripheral);}
            if(!anyInFov){t.latest=Visibility::Occluded;t.firstExposed=t.lastExposure=-1;continue;}
            if(budget.rays<s.pointSamples){t.latest=Visibility::Deferred;b.frame.perceptionBudgetDeferred=true;
                --b.targetCursor;++diagnostics_.budgetDeferred;break;}
            ++tested;uint8_t visible=0;bool unknown=false,central=false;
            VisualContact observation{};
            // Head/eye-only means this complete configured sample set: one
            // clear eye/head and three distinct, in-FOV, actually blocked body
            // samples. It is not a rendered head/body-area classification.
            bool sampledHeadClear=false,sampledBodiesBlocked=s.pointSamples==PointCount;
            for(uint32_t j=0;j<s.pointSamples;++j){bool peripheral=false;
                bool duplicate=false;for(uint32_t before=0;before<j;++before){const auto&a=points[j].point;const auto&z=points[before].point;
                    duplicate=duplicate||(a.x==z.x&&a.y==z.y&&a.z==z.z);}if(duplicate){sampledBodiesBlocked=false;continue;}
                ++observation.testedPointCount;
                if(!InFov(self,points[j].point,s,peripheral)){if(j)sampledBodiesBlocked=false;continue;}
                const auto ray=Trace(hooks,observer,self.eye,points[j].point,budget);
                const bool head=points[j].kind==PointKind::Eye||points[j].kind==PointKind::NativeHead;
                const bool body=points[j].kind==PointKind::NativeBody||points[j].kind==PointKind::CapsuleChestApprox||points[j].kind==PointKind::CapsuleShoulderApprox;
                if(j==0)sampledHeadClear=head&&ray==Ray::Clear;
                else if(!body||ray!=Ray::Blocked)sampledBodiesBlocked=false;
                if(ray==Ray::Unknown){unknown=true;continue;}
                if(ray==Ray::Clear){observation.points[visible++]=points[j];central=central||!peripheral;}
            }
            if(unknown){t.latest=Visibility::Unknown;t.firstExposed=t.lastExposure=-1;t.evidenceSeconds=0;continue;}
            const float sampleFraction=observation.testedPointCount?static_cast<float>(visible)/observation.testedPointCount:0;
            if(visible<s.minVisiblePoints||sampleFraction<s.minimumVisibleSampleFraction){t.latest=Visibility::Occluded;t.firstExposed=t.lastExposure=-1;t.evidenceSeconds=0;continue;}
            double elapsed=0;
            if(t.lastExposure<0||now-t.lastExposure>s.maxSampleGapSeconds){t.firstExposed=now;t.evidenceSeconds=0;t.recognized=-1;}
            else elapsed=now-t.lastExposure;
            const float distance=Length(Sub(observation.points[0].point,self.eye));
            const float gain=(central?s.centralGain:s.peripheralGain)/(1+s.distancePenalty*distance/s.maxDistanceCm);
            t.lastExposure=now;
            if(gain<=0){t.latest=Visibility::Unknown;continue;}
            t.evidenceSeconds+=elapsed*gain;
            if(t.recognized<0&&t.evidenceSeconds<(std::max)(s.recognitionSeconds,s.exposureSeconds)){
                ++diagnostics_.recognitionPending;t.latest=Visibility::Unknown;continue;}
            const float miss=central?s.missProbability:s.peripheralMissProbability;
            if(Random(b)<miss){++diagnostics_.missed;t.latest=Visibility::Occluded;continue;}
            if(!t.opaque){if(nextTracks_[observer]==UINT64_MAX){b.frame.eventsOverflow=true;continue;}
                t.opaque=++nextTracks_[observer];++diagnostics_.detections;}
            if(t.recognized<0)t.recognized=now;
            observation.track=t.opaque;observation.permitted=true;observation.visibility=Visibility::Visible;
            observation.visibilitySamplesKnown=true;observation.pointCount=visible;observation.observedAtSeconds=now;
            observation.firstExposedAtSeconds=t.firstExposed;observation.recognizedAtSeconds=t.recognized;observation.recognizedBySensor=true;
            observation.confidence=1-miss;
            observation.exposure=visible==s.pointSamples&&visible>1?Exposure::SampledBroad:
                sampledHeadClear&&sampledBodiesBlocked&&observation.testedPointCount==PointCount?Exposure::SampledHeadOnly:Exposure::SampledPartial;
            // Only differences between actually acquired point samples establish
            // perceived motion. Privileged target native velocity never crosses.
            const double dt=now-t.memory.observedAtSeconds;
            if(t.memory.permitted&&dt>0&&dt<=s.maxSampleGapSeconds&&t.memory.pointCount&&observation.pointCount
               &&t.memory.points[0].kind==observation.points[0].kind){
                observation.observedVelocity=Mul(Sub(observation.points[0].point,t.memory.points[0].point),static_cast<float>(1/dt));
                observation.motionKnown=Finite(observation.observedVelocity);
            }
            if(target.postureKnown&&visible==s.pointSamples&&visible>1){observation.postureKnown=true;observation.observedCrouched=target.crouched;observation.observedAirborne=target.airborne;}
            observation.firingVisuallyObserved=VisibleShot(observer,self,target,s,t,now,hooks,budget);
            t.memory=observation;t.latest=Visibility::Visible;
        }
        b.nextSense=now+s.visualSampleSeconds;
    }
    void CollectContacts(Bot&b,const Settings&s,double now){
        const auto first=b.contactCursor;
        for(uint32_t step=0;step<MaxPlayers;++step){const auto n=(first+step)%MaxPlayers;auto&t=b.tracks[n];if(!t.opaque||!t.memory.permitted)continue;
            const double age=now-t.memory.observedAtSeconds;
            const bool current=t.latest==Visibility::Visible&&age<=s.maxSampleGapSeconds;
            if(age<0||(!current&&age>s.memoryTtlSeconds)){Retire(b,t,now);continue;}
            if(b.frame.contactCount==b.frame.contacts.size()){b.frame.perceptionBudgetDeferred=true;b.contactCursor=n;break;}
            auto contact=t.memory;contact.visibility=t.latest;
            if(age>s.maxSampleGapSeconds&&contact.visibility==Visibility::Visible)contact.visibility=Visibility::Deferred;
            if(contact.visibility!=Visibility::Visible){contact.firingVisuallyObserved=false;contact.confidence*=static_cast<float>(std::exp2(-age/s.memoryHalfLifeSeconds));}
            b.frame.contacts[b.frame.contactCount++]=contact;
        }
    }
    void QueueSound(Bot&b,const humanlike_bot_native_events::Event&event,const Settings&s){
        for(auto&pending:b.pendingSounds)if(!pending.used){pending={true,event,event.atMatchSeconds+s.hearingDelaySeconds};return;}
        b.frame.eventsOverflow=true;
    }
    void Events(uint32_t index,const Pose&self,const std::array<Pose,MaxPlayers>&poses,const Settings&s,double now,const Hooks&hooks,Budget&budget){
        namespace events=humanlike_bot_native_events;auto&b=bots_[index];
        b.frame.eventFeedValid=events::diagnostics.installed;
        const auto kind=b.frame.self.weapon.kind;
        b.frame.self.weapon.shotFeedAvailable=b.frame.self.weapon.valid&&events::diagnostics.installed
            &&kind!=WeaponKind::Melee&&kind!=WeaponKind::Ability&&kind!=WeaponKind::Unknown;
        b.frame.hearingAvailable=events::diagnostics.installed&&(s.hearingGunshotEnabled
            ||(s.hearingFootstepEnabled&&events::diagnostics.footstepProducerInstalled)
            ||(s.hearingLandingEnabled&&events::diagnostics.landingAudioProducerInstalled));
        bool overflow=false;events::Event event{};uint32_t count=0;
        while(count<s.eventBudget&&events::Next(b.eventCursor,event,overflow)){
            ++count;
            const bool shot=event.kind==events::Kind::Shot;
            const bool audioProof=event.originalForwardCompleted&&event.audioRequestObserved
                &&(event.audioProof==events::AudioProof::AuthoredAudioRequest
                   ||(event.audioProof==events::AudioProof::ActualAudioPost&&event.originalAudioPostObserved&&event.playingId));
            const bool footstep=event.kind==events::Kind::FootstepAudio&&audioProof&&events::diagnostics.footstepProducerInstalled;
            const bool landing=event.kind==events::Kind::LandingAudio&&audioProof&&events::diagnostics.landingAudioProducerInstalled;
            if(shot||footstep||landing){
                if(event.subject.index>=MaxPlayers||(event.subject.index!=index&&!poses[event.subject.index].targetEligible))continue;
                if(event.subject.index==index){if(!Matches(self.identity,event.subject))continue;
                    if(!shot)continue;
                    if(now<event.atMatchSeconds||now-event.atMatchSeconds>s.ownShotMaxAgeSeconds){b.frame.shotsOverflow=true;continue;}
                    if(b.frame.shotCount==b.frame.shots.size()){b.frame.shotsOverflow=true;continue;}
                    b.frame.shots[b.frame.shotCount++]={b.own,event.serial,event.binding,event.origin,event.atMatchSeconds,true,event.actualGeometryPaired};
                }else if(now>=event.atMatchSeconds&&now-event.atMatchSeconds<=s.hearingTtlSeconds
                    &&((shot&&s.hearingGunshotEnabled&&event.actualGeometryPaired)||(footstep&&s.hearingFootstepEnabled)||(landing&&s.hearingLandingEnabled)))QueueSound(b,event,s);
                continue;
            }
            if(event.kind!=events::Kind::Hurt&&event.kind!=events::Kind::OwnDeath&&event.kind!=events::Kind::OwnKill)continue;
            if(!Matches(self.identity,event.subject))continue;
            if(event.kind==events::Kind::Hurt){
                if(b.frame.hurtCount==b.frame.hurt.size()){b.frame.eventsOverflow=true;continue;}
                HurtCue cue{};cue.valid=true;cue.serial=event.serial;cue.life=b.own;cue.acquiredAtSeconds=event.atMatchSeconds;
                cue.ownHealthLoss=event.healthLoss;cue.ownShieldLoss=event.shieldLoss;cue.confidence=1;
                if(event.directionKnown&&Finite(event.direction)){cue.directionKnown=true;
                    cue.bearingDegrees=Wrap(std::atan2(-event.direction.y,-event.direction.x)*180/Pi+(Random(b)*2-1)*s.hurtBearingError);
                    cue.bearingUncertaintyDegrees=s.hurtBearingError;
                    if(Random(b)<s.hurtWrongDirectionProbability){cue.bearingDegrees=Random(b)*360-180;cue.bearingUncertaintyDegrees=180;}}
                b.frame.hurt[b.frame.hurtCount++]=cue;
            }else{
                if(b.frame.feedbackCount==b.frame.feedback.size()){b.frame.eventsOverflow=true;continue;}
                Feedback feedback{};feedback.valid=feedback.nativeFeedbackObserved=true;feedback.life=b.own;
                feedback.serial=FeedbackSerial(event.serial,1);feedback.acquiredAtSeconds=event.atMatchSeconds;
                if(!feedback.serial){b.frame.eventsOverflow=true;continue;}
                feedback.kind=event.kind==events::Kind::OwnKill?Feedback::Kind::Kill:Feedback::Kind::OwnDeath;
                b.frame.feedback[b.frame.feedbackCount++]=feedback;
            }
        }
        if(overflow){b.frame.eventsOverflow=b.frame.shotsOverflow=true;++diagnostics_.eventGaps;}
        for(auto&pending:b.pendingSounds){if(!pending.used)continue;const auto&e=pending.event;
            if(e.subject.index>=MaxPlayers||!poses[e.subject.index].targetEligible){pending={};continue;}
            const bool isFootstep=e.kind==events::Kind::FootstepAudio,isLanding=e.kind==events::Kind::LandingAudio;
            // A live category disable retires its queued perception receipt.
            // Re-enabling later must not replay the pre-disable request.
            if((isFootstep&&!s.hearingFootstepEnabled)||(isLanding&&!s.hearingLandingEnabled)
               ||(!isFootstep&&!isLanding&&!s.hearingGunshotEnabled)){pending={};continue;}
            const double age=now-e.atMatchSeconds;
            if(age<0||age>s.hearingTtlSeconds){pending={};++diagnostics_.hearingExpired;continue;}
            if(now<pending.due)continue;
            const float range=isFootstep?s.hearingFootstepRangeCm:(isLanding?s.hearingLandingRangeCm:s.hearingRangeCm);
            const auto delta=Sub(e.position,self.eye);const float distance=Length(delta);
            if(!std::isfinite(distance)||distance>range||distance<=0){pending={};continue;}
            if(!budget.rays){b.frame.perceptionBudgetDeferred=true;continue;}
            const auto ray=Trace(hooks,index,self.eye,e.position,budget);
            if(ray==Ray::Unknown){b.frame.perceptionBudgetDeferred=true;continue;}
            if((ray==Ray::Blocked&&distance>range*s.hearingOcclusionScale)||Random(b)<s.hearingMissProbability){pending={};continue;}
            if(b.frame.soundCount==b.frame.sounds.size()){b.frame.eventsOverflow=true;pending={};continue;}
            SoundCue sound{};sound.valid=sound.directionKnown=sound.distanceBandKnown=true;
            sound.serial=e.serial;sound.kind=isFootstep?SoundKind::Footstep:(isLanding?SoundKind::Landing:SoundKind::Gunshot);sound.acquiredAtSeconds=now;
            sound.evidence=e.kind==events::Kind::Shot?SoundEvidence::NativeShotGeometryLogicalPropagation:
                (e.audioProof==events::AudioProof::ActualAudioPost?SoundEvidence::ActualAudioPostLogicalPropagation:SoundEvidence::AuthoredAudioRequestLogicalPropagation);
            sound.firstEvidenceAtSeconds=e.atMatchSeconds;sound.evidenceTimeKnown=true;
            sound.bearingDegrees=Wrap(std::atan2(delta.y,delta.x)*180/Pi+(Random(b)*2-1)*s.hearingBearingError);
            sound.bearingUncertaintyDegrees=s.hearingBearingError;
            if(Random(b)<s.hearingWrongDirectionProbability){sound.bearingDegrees=Random(b)*360-180;sound.bearingUncertaintyDegrees=180;}
            // The center must not reveal the exact source distance. Quantize
            // after perceptual error, including when the configured error is 0.
            const float error=distance*s.hearingDistanceError;
            const float estimate=(std::max)(0.f,distance+(Random(b)*2-1)*error);
            sound.minimumDistanceCm=(std::max)(0.f,std::floor((estimate-error)/s.hearingDistanceBandCm)*s.hearingDistanceBandCm);
            sound.maximumDistanceCm=(std::floor((estimate+error)/s.hearingDistanceBandCm)+1)*s.hearingDistanceBandCm;
            sound.confidence=1-s.hearingMissProbability;b.frame.sounds[b.frame.soundCount++]=sound;
            pending={};++diagnostics_.sounds;
        }
    }
public:
    void Reset(uint64_t seed){for(auto&b:bots_)b={};nextTracks_.fill(0);nextObserver_=1;seed_=seed;diagnostics_={};activationCursor_=humanlike_bot_native_events::writeSerial;}
    void Reseed(uint64_t seed){
        if(seed_==seed)return;seed_=seed;
        for(auto&b:bots_)if(b.bound)b.rng=Mix(seed_^Mix(b.own.stableBotId)^Mix(b.own.spawnEpoch)^Mix(b.frameSerial));
        // Acquired tracks, exposure evidence, event cursors and life stay intact.
    }
    const Diagnostics& Stats()const{return diagnostics_;}
    const SensorFrame* Frame(size_t index)const{return index<MaxPlayers&&bots_[index].bound?&bots_[index].frame:nullptr;}
    bool SetSpace(size_t index,const SpaceHints&space){if(index>=MaxPlayers||!bots_[index].bound)return false;bots_[index].frame.space=space;return true;}
    // Backend-only join; never expose Subject/Identity to the pure brain.
    bool AcquiredVisibleTrack(uint32_t observer,const humanlike_bot_native_events::Subject&victim,
        double evidenceAt,double now,double maximumAge,TrackId&track)const{
        track=0;if(observer>=MaxPlayers||!std::isfinite(evidenceAt)||!std::isfinite(now)||!std::isfinite(maximumAge)
            ||maximumAge<0||evidenceAt>now||now-evidenceAt>maximumAge)return false;
        const auto&b=bots_[observer];if(!b.bound||!b.frame.self.valid||!b.frame.self.alive)return false;
        for(const auto&t:b.tracks)if(t.opaque&&t.memory.permitted&&t.latest==Visibility::Visible
            &&t.recognized<=evidenceAt&&Matches(t.identity,victim)&&now-t.memory.observedAtSeconds<=maximumAge){track=t.opaque;return true;}
        return false;
    }
    bool OwnSubjectMatches(uint32_t observer,const humanlike_bot_native_events::Subject&subject)const{
        return observer<MaxPlayers&&bots_[observer].bound&&bots_[observer].frame.self.valid
            &&Matches(bots_[observer].ownIdentity,subject);
    }
    Ray TraceBudgeted(const Hooks&hooks,uint32_t observer,const Vec3&from,const Vec3&to,Budget&budget){return Trace(hooks,observer,from,to,budget);}
    bool AppendFeedback(uint32_t observer,const Feedback&feedback){
        if(observer>=MaxPlayers||!bots_[observer].bound||!feedback.valid||!feedback.serial||!Same(bots_[observer].own,feedback.life))return false;
        auto&frame=bots_[observer].frame;if(frame.feedbackCount==frame.feedback.size()){frame.eventsOverflow=true;return false;}
        if(feedback.kind==Feedback::Kind::Hit&&!feedback.nativeFeedbackObserved
           &&(feedback.provenance!=Feedback::Provenance::ProjectVisibleNativeContact||!feedback.nativeContactObserved
              ||!feedback.visibleImpactProven||!feedback.alreadyKnownTrack||!feedback.binding||!feedback.nativeOrigin))return false;
        frame.feedback[frame.feedbackCount++]=feedback;return true;
    }
    bool FlagShotGap(size_t index){if(index>=MaxPlayers||!bots_[index].bound)return false;
        bots_[index].frame.shotsOverflow=true;bots_[index].frame.self.weapon.shotFeedAvailable=false;return true;}
    bool FlagEventGap(size_t index){if(index>=MaxPlayers||!bots_[index].bound)return false;bots_[index].frame.eventsOverflow=true;return true;}
    bool FlagBudgetDeferred(size_t index){if(index>=MaxPlayers||!bots_[index].bound)return false;bots_[index].frame.perceptionBudgetDeferred=true;return true;}
    void Pass(double now,const std::array<Pose,MaxPlayers>&poses,const std::array<OwnSnapshot,MaxPlayers>&own,
              const std::array<Settings,MaxPlayers>&settings,Budget budget,const Hooks&hooks){
        if(!std::isfinite(now)||now<0)return;++diagnostics_.passes;
        // Cheap identity retirement is the only all-pairs bookkeeping. It does
        // not sample hidden target positions or publish new-life information.
        for(uint32_t i=1;i<MaxPlayers;++i){auto&b=bots_[i];if(!own[i].life.stableBotId){
                // Roster removal/profile loss is not permission to keep a
                // previous live frame or continue an old-life command.
                if(b.bound)b={};b.eventCursor=humanlike_bot_native_events::writeSerial;continue;}
            if(!b.bound||!Same(b.own,own[i].life)){const uint64_t cursor=b.eventCursor;b={};b.own=own[i].life;b.bound=true;
                b.rng=Mix(seed_^Mix(b.own.stableBotId)^Mix(b.own.spawnEpoch));b.eventCursor=(std::max)(cursor,activationCursor_);
                b.nextSense=now+settings[i].visualSampleSeconds*static_cast<double>(i)/MaxPlayers;}
            b.frame={};b.frame.serial=++b.frameSerial;b.frame.self=own[i];b.frame.perceivedAtSeconds=now;
            b.ownIdentity=poses[i].identity;
            const bool nativeBlinded=b.frame.self.nativeBlindnessKnown&&b.frame.self.nativeBlinded;
            b.frame.perceptionValid=Valid(settings[i])&&poses[i].valid&&poses[i].viewValid&&hooks.trace;
            for(size_t n=0;n<MaxPlayers;++n){auto&t=b.tracks[n];
                // Policy revocation is immediate, independent of visual sample/budget.
                if(!poses[n].targetEligible){Retire(b,t,now);continue;}
                if(t.identity.pawn&&poses[n].identity.pawn&&!Same(t.identity,poses[n].identity)){Retire(b,t,now);continue;}
                // Demote cached current visibility immediately, even before the next visual sample.
                // The acquired point, original observation time, opaque track and retirement stay intact.
                if(nativeBlinded){t.latest=Visibility::Occluded;t.firstExposed=t.lastExposure=-1;t.evidenceSeconds=0;}
                const double age=now-t.memory.observedAtSeconds;
                // Revocation is independent of the next point-sampling slot:
                // stale/lost source, own life/view, binding and FOV cannot keep
                // a previously seen fire cue alive through a deferred sample.
                if(t.visibleShot.primed){auto&v=t.visibleShot;const auto&target=poses[n];bool peripheral=false;
                    const bool continuous=b.frame.self.valid&&b.frame.self.alive&&b.frame.self.eyeValid&&b.frame.self.viewValid
                        &&humanlike_bot_native_events::diagnostics.installed&&!nativeBlinded&&poses[i].valid&&poses[i].alive&&poses[i].viewValid
                        &&target.valid&&target.alive&&target.weaponBindingKnown&&target.weaponBinding==v.binding
                        &&Same(target.identity,t.identity)&&t.latest==Visibility::Visible&&age>=0&&age<=settings[i].maxSampleGapSeconds
                        &&t.memory.pointCount&&InFov(poses[i],t.memory.points[0].point,settings[i],peripheral)
                        &&std::isfinite(target.sampledAtSeconds)&&target.sampledAtSeconds<=now&&now-target.sampledAtSeconds<=settings[i].maxSampleGapSeconds
                        &&std::isfinite(b.frame.self.sampledAtSeconds)&&b.frame.self.sampledAtSeconds<=now&&now-b.frame.self.sampledAtSeconds<=settings[i].maxSampleGapSeconds
                        &&(v.cueAt<0||(hooks.visibleShotSourceLive&&hooks.visibleShotSourceLive(i,poses[i].identity,target.identity,v.binding,hooks.context)));
                    if(!continuous){v={};t.memory.firingVisuallyObserved=false;}
                    else if(v.cueAt<0||now<v.cueAt||now-v.cueAt>settings[i].maxSampleGapSeconds
                            ||now-v.cueAt>settings[i].ownShotMaxAgeSeconds||!InFov(poses[i],v.position,settings[i],peripheral)){
                        v.cueAt=-1;t.memory.firingVisuallyObserved=false;}
                }
                if(t.opaque&&Valid(settings[i])&&(t.latest!=Visibility::Visible||age>settings[i].maxSampleGapSeconds)
                   &&age>settings[i].memoryTtlSeconds)Retire(b,t,now);}
        }
        if(hooks.feedbackPass){
            uint32_t visionReserve=0;for(uint32_t i=1;i<MaxPlayers;++i)if(bots_[i].bound&&Valid(settings[i]))visionReserve=(std::max)(visionReserve,settings[i].pointSamples);
            const uint32_t reserved=(std::min)(budget.rays,budget.hearingReserve+visionReserve);
            Budget feedback{budget.rays-reserved,0,0};const auto before=feedback.rays;
            hooks.feedbackPass(*this,now,feedback,hooks,hooks.feedbackContext);
            // A provider cannot increase the caller's budget through this hook.
            budget.rays-=before-(std::min)(before,feedback.rays);
        }
        Budget hearing{(std::min)(budget.rays,budget.hearingReserve),0,0};budget.rays-=hearing.rays;
        const uint32_t first=nextObserver_;uint32_t serviced=0;
        for(uint32_t step=0;step<MaxPlayers-1;++step){const uint32_t i=1+(first-1+step)%(MaxPlayers-1);
            auto&b=bots_[i];if(!b.bound||!b.frame.self.valid||!b.frame.self.alive||!Valid(settings[i]))continue;
            // Unknown blindness retains the existing partial point-LOS model; it is not a false/clear receipt.
            if(b.frame.self.nativeBlindnessKnown&&b.frame.self.nativeBlinded)continue;
            if(now>=b.nextSense){
                if(serviced>=budget.observers||budget.rays<settings[i].pointSamples){b.frame.perceptionBudgetDeferred=true;++diagnostics_.budgetDeferred;}
                else{Sense(i,poses[i],poses,settings[i],now,hooks,budget);++serviced;nextObserver_=1+i%(MaxPlayers-1);}
            }
        }
        // Independent event cursors and contact memories are retained per bot.
        // Hearing shares the global ray budget; it cannot bypass obstruction.
        // Rotate hearing too; a noisy early roster slot cannot permanently
        // monopolize the reserved native world-trace budget.
        for(uint32_t step=0;step<MaxPlayers-1;++step){const uint32_t i=1+(first-1+step)%(MaxPlayers-1);
            auto&b=bots_[i];if(!b.bound||!Valid(settings[i]))continue;
            if(b.frame.self.valid)Events(i,poses[i],poses,settings[i],now,hooks,hearing);
            CollectContacts(b,settings[i],now);
        }
    }
};
} // namespace humanlike_bot_sensors
