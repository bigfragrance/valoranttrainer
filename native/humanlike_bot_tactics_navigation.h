#pragma once
#include "humanlike_bot_math.h"
#include <array>
#include <cmath>

// Acquired-data hypotheses only. A sound sector is frozen at the listener's
// receipt position. Neither a hypothesis nor a submitted request proves a
// source identity, a gunfight, a navigable destination or original movement.
namespace humanlike_bot_tactics_navigation {
namespace dto=humanlike_bot;
namespace math=dto::math;
struct Settings {
    double ttl=0,halfLife=0;
    float confidenceDecayPerS=0,uncertaintyGrowthCmS=0,minimumConfidence=0;
};
struct Estimate {
    bool valid=false,heard=false,gunshot=false;uint64_t hypothesis=0,serial=0;
    dto::TrackId visualTrack=0;
    dto::Vec3 point{};float radiusCm=0,confidence=0;
    double eventAt=0,availableAt=0,sampledAt=0,expiresAt=0;
};
inline float Confidence(const Estimate&e,double now,const Settings&s){
    const double age=(std::max)(0.0,now-e.sampledAt);
    const auto retention=s.halfLife>0?std::exp2(-age/s.halfLife):(age==0?1.0:0.0);
    return math::Clamp(static_cast<float>(e.confidence*retention-age*s.confidenceDecayPerS),0,1);
}
inline float Radius(const Estimate&e,double now,const Settings&s){
    return e.radiusCm+static_cast<float>((std::max)(0.0,now-e.sampledAt))*s.uncertaintyGrowthCmS;
}
inline bool Current(const Estimate&e,double now,const Settings&s){
    if(!(e.valid&&math::Finite(now)&&math::Finite(e.point)&&math::Finite(e.radiusCm)&&e.radiusCm>=0
        &&math::Finite(e.eventAt)&&math::Finite(e.availableAt)&&math::Finite(e.sampledAt)&&math::Finite(e.expiresAt)
        &&e.eventAt<=e.availableAt&&e.availableAt<=now&&e.sampledAt<=now&&now<e.expiresAt))return false;
    const auto confidence=Confidence(e,now,s);return confidence>0&&confidence>=s.minimumConfidence;
}
inline Estimate FromSound(const dto::SoundCue&cue,const dto::OwnSnapshot&own,const Settings&s){
    Estimate e{};
    if(!own.valid||!own.alive||own.paused||own.retired||!math::Finite(own.feet)||!math::Finite(own.nowSeconds)
       ||!cue.valid||cue.ownSound||!cue.serial||!cue.directionKnown||!cue.distanceBandKnown
       ||!math::Finite(cue.bearingDegrees)||!math::Finite(cue.bearingUncertaintyDegrees)
       ||cue.bearingUncertaintyDegrees<0||cue.bearingUncertaintyDegrees>180
       ||!math::Finite(cue.minimumDistanceCm)||!math::Finite(cue.maximumDistanceCm)
       ||cue.minimumDistanceCm<0||cue.maximumDistanceCm<cue.minimumDistanceCm
       ||!math::Finite(cue.confidence)||cue.confidence<=0||cue.confidence>1
       ||!math::Finite(cue.acquiredAtSeconds)||cue.acquiredAtSeconds>own.nowSeconds
       ||(cue.evidenceTimeKnown&&(!math::Finite(cue.firstEvidenceAtSeconds)||cue.firstEvidenceAtSeconds>cue.acquiredAtSeconds)))return e;
    const auto distance=(cue.minimumDistanceCm+cue.maximumDistanceCm)*.5f;
    const auto angle=cue.bearingDegrees*math::Pi/180;
    e.point=math::Add(own.feet,{static_cast<float>(std::cos(angle)*distance),static_cast<float>(std::sin(angle)*distance),0});
    // Enclose the entire annular sector in an XY disc around its midpoint.
    // This uses reported uncertainty, not an unexposed separation threshold.
    const auto uncertainty=cue.bearingUncertaintyDegrees*math::Pi/180;
    const auto edge=[&](float radius){return std::hypot(radius*std::sin(uncertainty),radius*std::cos(uncertainty)-distance);};
    e.radiusCm=static_cast<float>((std::max)(edge(cue.minimumDistanceCm),edge(cue.maximumDistanceCm)));
    e.valid=e.heard=true;e.gunshot=cue.kind==dto::SoundKind::Gunshot;e.serial=cue.serial;
    e.confidence=cue.confidence;e.sampledAt=e.availableAt=cue.acquiredAtSeconds;
    e.eventAt=cue.evidenceTimeKnown?cue.firstEvidenceAtSeconds:cue.acquiredAtSeconds;
    e.expiresAt=cue.acquiredAtSeconds+s.ttl;
    if(!Current(e,own.nowSeconds,s))return{};return e;
}
inline bool Separated(const Estimate&a,const Estimate&b,double now,const Settings&s){
    if(!Current(a,now,s)||!Current(b,now,s)||!a.gunshot||!b.gunshot)return false;
    if(a.visualTrack&&b.visualTrack&&a.visualTrack==b.visualTrack)return false;
    return math::Horizontal(math::Sub(a.point,b.point))>Radius(a,now,s)+Radius(b,now,s);
}
template<size_t N>inline void Insert(std::array<Estimate,N>&rows,Estimate e,double now,const Settings&s,uint64_t&nextHypothesis){
    if(!Current(e,now,s))return;
    size_t slot=N;
    for(size_t n=0;n<N;++n){auto&old=rows[n];
        if(!Current(old,now,s)){old={};if(slot==N)slot=n;continue;}
        // A new serial is an event, never a new speaker. Overlapping sound
        // footprints remain one hypothesis, including repeated automatic fire.
        const bool sameVisual=e.visualTrack&&old.visualTrack==e.visualTrack;
        const bool overlap=e.heard&&old.heard&&math::Horizontal(math::Sub(e.point,old.point))<=Radius(e,now,s)+Radius(old,now,s);
        if(sameVisual||overlap){e.hypothesis=old.hypothesis;
            if(e.sampledAt>=old.sampledAt)old=e;return;}
    }
    if(slot==N){slot=0;for(size_t n=1;n<N;++n)if(rows[n].sampledAt<rows[slot].sampledAt)slot=n;}
    e.hypothesis=++nextHypothesis;rows[slot]=e;
}
inline bool ValidRequest(const dto::EstimatedNavigation&r,const dto::OwnSnapshot&own,double now){
    return r.requested&&r.key&&r.purpose!=dto::NavigationPurpose::None
        &&((r.purpose==dto::NavigationPurpose::MapHold&&r.knowledge==dto::NavigationKnowledge::StaticMap
            &&!r.usesHearing&&r.mapAnnotationId&&r.mapRevision&&r.uncertaintyCm==0)
           ||((r.purpose==dto::NavigationPurpose::SoundApproach||r.purpose==dto::NavigationPurpose::ThirdPartyJoin)
            &&r.knowledge==dto::NavigationKnowledge::AcquiredEstimate&&!r.mapAnnotationId&&!r.mapRevision))
        &&own.valid&&own.alive&&!own.paused&&!own.retired&&own.life.stableBotId&&own.life.spawnEpoch&&own.life.token&&math::Same(r.life,own.life)
        &&math::Finite(now)&&own.nowSeconds==now&&math::Finite(r.estimatedGoal)
        &&math::Finite(r.evidenceAtSeconds)&&math::Finite(r.readyAtSeconds)&&math::Finite(r.expiresAtSeconds)
        &&r.evidenceAtSeconds<=r.readyAtSeconds&&r.readyAtSeconds<=now
        &&(now<r.expiresAtSeconds||(r.expiresAtSeconds==0&&r.purpose==dto::NavigationPurpose::MapHold
            &&r.knowledge==dto::NavigationKnowledge::StaticMap))
        &&math::Finite(r.confidence)&&r.confidence>0&&r.confidence<=1&&math::Finite(r.uncertaintyCm)&&r.uncertaintyCm>=0;
}
inline bool SameRequest(const dto::EstimatedNavigation&a,const dto::EstimatedNavigation&b){
    return a.requested&&b.requested&&a.key==b.key&&math::Same(a.life,b.life)&&a.purpose==b.purpose
        &&a.estimatedGoal.x==b.estimatedGoal.x&&a.estimatedGoal.y==b.estimatedGoal.y&&a.estimatedGoal.z==b.estimatedGoal.z
        &&a.evidenceAtSeconds==b.evidenceAtSeconds&&a.readyAtSeconds==b.readyAtSeconds&&a.expiresAtSeconds==b.expiresAtSeconds
        &&a.confidence==b.confidence&&a.uncertaintyCm==b.uncertaintyCm&&a.usesHearing==b.usesHearing
        &&a.knowledge==b.knowledge&&a.mapAnnotationId==b.mapAnnotationId&&a.mapRevision==b.mapRevision;
}
inline dto::Vec3 GroundAnchorFocus(dto::Vec3 ground,const dto::OwnSnapshot&own){
    // Only original grounded map anchors call this approximation. An authored
    // head/focus point has its own height and must never be raised again.
    ground.z+=own.eye.z-own.feet.z;return ground;
}
// Only a newly proved auditory lease can extend the expiry of an otherwise
// identical frozen plan. Static and third-party requests retain their contract.
inline bool SoundLeaseExtension(const dto::EstimatedNavigation&previous,const dto::EstimatedNavigation&request){
    if(previous.purpose!=dto::NavigationPurpose::SoundApproach||!previous.usesHearing
       ||!math::Finite(previous.expiresAtSeconds)||previous.expiresAtSeconds<=0
       ||!math::Finite(request.expiresAtSeconds)||request.expiresAtSeconds<=previous.expiresAtSeconds)return false;
    auto renewed=previous;renewed.expiresAtSeconds=request.expiresAtSeconds;
    return SameRequest(renewed,request);
}
} // namespace humanlike_bot_tactics_navigation
