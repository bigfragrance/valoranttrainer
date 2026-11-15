#pragma once
#include "humanlike_bot_types.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

// Backend only. Never pass Event/Subject or LatestShot to the pure brain.
// Producers run on the existing combat owner GT, after real native evidence.
// Seconds use combat::Now == match::Now (GetTickCount64), never native indices.
namespace humanlike_bot_native_events {
using Vec=humanlike_bot::Vec3;
inline constexpr size_t MaxPlayers=32,Capacity=2048,RecentDamageCapacity=128;
enum class Kind:uint8_t {Shot,Hurt,OwnDeath,OwnKill,FootstepAudio,LandingAudio,LandingPhysicsOnly};
enum class AudioProof:uint8_t {Unknown,AuthoredAudioRequest,ActualAudioPost};
struct Subject {
    uint32_t index=0,epoch=0;uintptr_t pawn=0;
    int32_t weakIndex=-1,weakSerial=0;
};
struct Event {
    Kind kind=Kind::Shot;Subject subject{};uint64_t serial=0;
    double atMatchSeconds=0;uint64_t token=0,binding=0,origin=0;
    uint32_t firingSlot=0;Vec position{},direction{};
    bool directionKnown=false,actualGeometryPaired=false;
    bool originalForwardCompleted=false,audioRequestObserved=false;
    AudioProof audioProof=AudioProof::Unknown;bool originalAudioPostObserved=false;uint32_t playingId=0;
    float healthLoss=0,shieldLoss=0;
};
struct DirectionEvidence {uint64_t token=0;uint32_t victim=0,epoch=0;Vec incoming{};bool valid=false;};
struct Diagnostics {uint64_t published=0,overwritten=0,rejected=0,duplicates=0;
    bool installed=false,footstepProducerInstalled=false,landingAudioProducerInstalled=false;};
inline Diagnostics diagnostics{};
inline std::array<Event,Capacity> queue{};
inline std::array<Event,MaxPlayers> latestShots{};
inline std::array<uint64_t,RecentDamageCapacity> recentDamage{};
inline uint64_t writeSerial=0,damageCursor=0;
inline DirectionEvidence directionEvidence{};
inline bool Finite(const Vec& v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
inline bool SubjectValid(const Subject& s){return s.index<MaxPlayers&&s.epoch&&s.pawn&&s.weakIndex>=0&&s.weakSerial>0;}
inline void BeginSession(){
    diagnostics={};diagnostics.installed=true;
    for(auto&event:queue)event={};for(auto&event:latestShots)event={};recentDamage.fill(0);
    writeSerial=damageCursor=0;directionEvidence={};
}
inline void EndSession(){diagnostics.installed=false;directionEvidence={};}
inline bool Publish(Event event){
    if(!diagnostics.installed||!SubjectValid(event.subject)||!std::isfinite(event.atMatchSeconds)
       ||event.atMatchSeconds<0||writeSerial==UINT64_MAX){++diagnostics.rejected;return false;}
    event.serial=++writeSerial;queue[(event.serial-1)%Capacity]=event;
    if(event.serial>Capacity)++diagnostics.overwritten;
    ++diagnostics.published;return true;
}
inline bool PublishShot(Event event){
    if(event.kind!=Kind::Shot||!event.origin||!event.binding||!event.actualGeometryPaired
       ||!Finite(event.position)||!Finite(event.direction)){++diagnostics.rejected;return false;}
    if(event.subject.index<latestShots.size()){const auto&last=latestShots[event.subject.index];
        if(last.serial&&last.subject.epoch==event.subject.epoch&&last.subject.pawn==event.subject.pawn
           &&last.subject.weakIndex==event.subject.weakIndex&&last.subject.weakSerial==event.subject.weakSerial
           &&last.binding==event.binding&&last.origin==event.origin){++diagnostics.duplicates;return true;}}
    if(!Publish(event))return false;
    latestShots[event.subject.index]=queue[(writeSerial-1)%Capacity];return true;
}
inline const Event* LatestShot(size_t index){
    return index<latestShots.size()&&latestShots[index].serial?&latestShots[index]:nullptr;
}
inline void SetAudioAvailability(bool footstep,bool landing){
    diagnostics.footstepProducerInstalled=footstep;diagnostics.landingAudioProducerInstalled=landing;
}
inline bool PublishAudio(Event event){
    if((event.kind!=Kind::FootstepAudio&&event.kind!=Kind::LandingAudio)||!event.originalForwardCompleted
       ||!event.audioRequestObserved||!Finite(event.position)||event.audioProof==AudioProof::Unknown
       ||(event.audioProof==AudioProof::ActualAudioPost&&(!event.originalAudioPostObserved||!event.playingId))){++diagnostics.rejected;return false;}
    if((event.kind==Kind::FootstepAudio&&!diagnostics.footstepProducerInstalled)
       ||(event.kind==Kind::LandingAudio&&!diagnostics.landingAudioProducerInstalled)){++diagnostics.rejected;return false;}
    return Publish(event);
}
inline void StageDirection(uint64_t token,uint32_t victim,uint32_t epoch,const Vec& incoming){
    directionEvidence={};
    const double n=double(incoming.x)*incoming.x+double(incoming.y)*incoming.y+double(incoming.z)*incoming.z;
    if(!token||victim>=MaxPlayers||!epoch||!Finite(incoming)||n<1.e-12)return;
    const float inverse=static_cast<float>(1/std::sqrt(n));
    directionEvidence={token,victim,epoch,{incoming.x*inverse,incoming.y*inverse,incoming.z*inverse},true};
}
inline DirectionEvidence TakeDirection(uint64_t token,uint32_t victim,uint32_t epoch){
    const auto result=directionEvidence;directionEvidence={};
    return result.valid&&result.token==token&&result.victim==victim&&result.epoch==epoch?result:DirectionEvidence{};
}
// Called only after policy::EndNativeDamage forwarded the original match commit
// once and that commit validated the original response/life/score readbacks.
inline bool PublishDamage(uint64_t token,double at,const Subject& victim,const Subject& source,
    float healthBefore,float healthAfter,float shieldBefore,float shieldAfter,bool killed,const DirectionEvidence& direction){
    if(!token||!SubjectValid(victim)||!SubjectValid(source)||!std::isfinite(healthBefore)||!std::isfinite(healthAfter)
       ||!std::isfinite(shieldBefore)||!std::isfinite(shieldAfter)||healthBefore<0||healthAfter<0||shieldBefore<0||shieldAfter<0){
        ++diagnostics.rejected;return false;}
    for(const auto seen:recentDamage)if(seen==token){++diagnostics.duplicates;return true;}
    recentDamage[damageCursor++%RecentDamageCapacity]=token;
    Event hurt{};hurt.kind=Kind::Hurt;hurt.subject=victim;hurt.token=token;hurt.atMatchSeconds=at;
    hurt.healthLoss=(std::max)(0.f,healthBefore-healthAfter);hurt.shieldLoss=(std::max)(0.f,shieldBefore-shieldAfter);
    if(direction.valid&&direction.token==token&&direction.victim==victim.index&&direction.epoch==victim.epoch){
        hurt.directionKnown=true;hurt.direction=direction.incoming;}
    bool okay=true;if(hurt.healthLoss>0||hurt.shieldLoss>0)okay=Publish(hurt);
    if(killed){auto death=hurt;death.kind=Kind::OwnDeath;okay=Publish(death)&&okay;
        Event kill{};kill.kind=Kind::OwnKill;kill.subject=source;kill.token=token;kill.atMatchSeconds=at;okay=Publish(kill)&&okay;}
    return okay;
}
// Independent non-destructive cursor. Reader state belongs to each NPC. A gap
// must be reported to brain; never silently infer that no own shots occurred.
inline bool Next(uint64_t& cursor,Event& event,bool& overflow){
    if(cursor>writeSerial){cursor=writeSerial;overflow=true;}
    if(writeSerial-cursor>Capacity){cursor=writeSerial-Capacity;overflow=true;}
    if(cursor==writeSerial)return false;
    event=queue[cursor%Capacity];++cursor;
    if(event.serial!=cursor){overflow=true;return false;}return true;
}
} // namespace humanlike_bot_native_events
