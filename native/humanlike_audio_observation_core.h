#pragma once
#include "humanlike_bot_native_events.h"
#include <cmath>
#include <cstdint>

// Backend-only request provenance, never passed to the pure brain. No raw
// FFrame reads and no velocity-based sound generation exist in this helper.
namespace humanlike_audio_observation {
namespace events=humanlike_bot_native_events;
enum class Request:uint8_t {Footstep,Landing,Takeoff};
enum class PositionSource:uint8_t {Unknown,OriginalGroundMovement,OriginalGroundRoot};
struct Weak {int32_t index=-1,serial=0;};
struct Point {double x=0,y=0,z=0;};
struct Sample {
    bool valid=false,sourceProfiles=false,metadata=false,ownedWorld=false,originalAsset=false;
    Request request=Request::Takeoff;PositionSource positionSource=PositionSource::Unknown;
    events::Subject subject{};Weak component{},world{},asset{};
    uintptr_t componentObject=0,worldObject=0,assetObject=0;
    events::Vec position{};Point originalPosition{};double atMatchSeconds=0,nativeWorldSeconds=0;
    bool nativeWorldTimeKnown=false;
};
struct Receipt {
    uint64_t occurrence=0;Sample before{},after{};
    bool originalForwardCompleted=false,sameLife=false,requestEligible=false;
    bool originalAudioPostObserved=false;uint32_t playingId=0;
    events::Event event{};
};
inline bool Same(const Weak&a,const Weak&b){return a.index==b.index&&a.serial==b.serial&&a.index>=0&&a.serial>0;}
inline bool SubjectSame(const events::Subject&a,const events::Subject&b){
    return events::SubjectValid(a)&&events::SubjectValid(b)&&a.index==b.index&&a.epoch==b.epoch&&a.pawn==b.pawn
        &&a.weakIndex==b.weakIndex&&a.weakSerial==b.weakSerial;
}
inline bool Good(const Sample&s){
    return s.valid&&s.sourceProfiles&&s.metadata&&s.ownedWorld&&s.originalAsset
        &&events::SubjectValid(s.subject)&&s.componentObject&&s.worldObject&&s.assetObject
        &&Same(s.component,s.component)&&Same(s.world,s.world)&&Same(s.asset,s.asset)
        &&s.positionSource!=PositionSource::Unknown&&events::Finite(s.position)
        &&std::isfinite(s.originalPosition.x)&&std::isfinite(s.originalPosition.y)&&std::isfinite(s.originalPosition.z)
        &&s.position.x==static_cast<float>(s.originalPosition.x)&&s.position.y==static_cast<float>(s.originalPosition.y)
        &&s.position.z==static_cast<float>(s.originalPosition.z)
        &&std::isfinite(s.atMatchSeconds)&&s.atMatchSeconds>=0
        &&(!s.nativeWorldTimeKnown||std::isfinite(s.nativeWorldSeconds));
}
inline bool Stable(const Sample&a,const Sample&b){
    return Good(a)&&Good(b)&&SubjectSame(a.subject,b.subject)&&a.request==b.request
        &&a.componentObject==b.componentObject&&Same(a.component,b.component)
        &&a.worldObject==b.worldObject&&Same(a.world,b.world)
        &&a.assetObject==b.assetObject&&Same(a.asset,b.asset)&&a.positionSource==b.positionSource
        &&a.originalPosition.x==b.originalPosition.x&&a.originalPosition.y==b.originalPosition.y&&a.originalPosition.z==b.originalPosition.z
        &&b.atMatchSeconds>=a.atMatchSeconds;
}
inline Receipt Evaluate(uint64_t occurrence,const Sample&before,const Sample&after,bool completed,
                        bool actualPostObserved=false,uint32_t playingId=0){
    Receipt r{};r.occurrence=occurrence;r.before=before;r.after=after;r.originalForwardCompleted=completed;
    r.sameLife=Stable(before,after);r.requestEligible=occurrence&&completed&&r.sameLife&&before.request!=Request::Takeoff;
    // A true handler Result or a component pointer is NOT a PlayingID.
    r.originalAudioPostObserved=actualPostObserved&&playingId!=0;r.playingId=r.originalAudioPostObserved?playingId:0;
    if(r.requestEligible){
        auto&e=r.event;e.kind=before.request==Request::Footstep?events::Kind::FootstepAudio:events::Kind::LandingAudio;
        e.subject=before.subject;e.atMatchSeconds=before.atMatchSeconds;e.position=before.position;
        e.token=occurrence;e.originalForwardCompleted=true;e.audioRequestObserved=true;
        e.audioProof=r.originalAudioPostObserved?events::AudioProof::ActualAudioPost:events::AudioProof::AuthoredAudioRequest;
        e.originalAudioPostObserved=r.originalAudioPostObserved;e.playingId=r.playingId;
    }return r;
}
} // namespace humanlike_audio_observation
