#pragma once
#include "humanlike_bot_native_events.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

// Backend-only candidate evidence. This is a project perception model input,
// never original hit-marker/audio feedback or a complete-origin Miss receipt.
// Subject/impact may be used only for private acquired-track/visibility joins;
// neither this structure nor these world coordinates may reach the pure brain.
namespace humanlike_bot_point_impacts {
using Subject=humanlike_bot_native_events::Subject;
using Vec=humanlike_bot::Vec3;
inline constexpr size_t Capacity=2048,RecentCapacity=2048;
struct ImpactReceipt {
    Subject source{},victim{};
    uint64_t serial=0,damageToken=0,binding=0,nativeOrigin=0;
    uint32_t pellet=0,firingSlot=0;
    double acquiredAtSeconds=0;
    Vec impact{};
    bool actualGeometryPaired=false,nativePointResponseVerified=false,nativeCommitVerified=false;
    // Positive nonlethal point damage has already been checked by the owner.
    // No enemy life value, damage amount, unseen pose or raw response escapes.
};
struct Diagnostics {uint64_t accepted=0,duplicates=0,rejected=0,overwritten=0;bool installed=false;};
inline Diagnostics diagnostics{};
inline std::array<ImpactReceipt,Capacity> queue{};
inline std::array<uint64_t,RecentCapacity> recentDamage{};
inline uint64_t writeSerial=0,recentCursor=0;
inline bool Same(const Subject& a,const Subject& b){
    return a.index==b.index&&a.epoch==b.epoch&&a.pawn==b.pawn&&a.weakIndex==b.weakIndex&&a.weakSerial==b.weakSerial;
}
inline bool Valid(const Subject& value){return humanlike_bot_native_events::SubjectValid(value);}
inline bool Point(const Vec& point){
    return std::isfinite(point.x)&&std::isfinite(point.y)&&std::isfinite(point.z)
        &&std::abs(point.x)<1.e9f&&std::abs(point.y)<1.e9f&&std::abs(point.z)<1.e9f;
}
inline bool SameContact(const ImpactReceipt& a,const ImpactReceipt& b){
    return Same(a.source,b.source)&&Same(a.victim,b.victim)&&a.damageToken==b.damageToken
        &&a.binding==b.binding&&a.nativeOrigin==b.nativeOrigin&&a.pellet==b.pellet&&a.firingSlot==b.firingSlot
        &&a.impact.x==b.impact.x&&a.impact.y==b.impact.y&&a.impact.z==b.impact.z;
}
inline void BeginSession(){diagnostics={};diagnostics.installed=true;writeSerial=recentCursor=0;recentDamage={};}
inline void EndSession(){diagnostics.installed=false;}
inline bool Publish(ImpactReceipt event){
    if(!diagnostics.installed||!Valid(event.source)||!Valid(event.victim)||event.source.index==event.victim.index
       ||!event.damageToken||!event.binding||!event.nativeOrigin||event.pellet>=64||event.firingSlot>=16
       ||!std::isfinite(event.acquiredAtSeconds)||event.acquiredAtSeconds<0||!Point(event.impact)
       ||!event.actualGeometryPaired||!event.nativePointResponseVerified||!event.nativeCommitVerified){
        ++diagnostics.rejected;return false;
    }
    static_assert(RecentCapacity==Capacity);
    for(size_t n=0;n<recentDamage.size();++n)if(recentDamage[n]==event.damageToken){
        if(!SameContact(queue[n],event)){++diagnostics.rejected;return false;}
        ++diagnostics.duplicates;return true;
    }
    if(writeSerial==UINT64_MAX){++diagnostics.rejected;return false;}
    event.serial=++writeSerial;queue[(event.serial-1)%Capacity]=event;
    recentDamage[recentCursor++%RecentCapacity]=event.damageToken;
    ++diagnostics.accepted;if(event.serial>Capacity)++diagnostics.overwritten;
    return true;
}
// Each NPC owns its independent non-destructive cursor. Loss is explicit;
// neither a gap nor an empty queue is proof that an origin missed its target.
inline bool Next(uint64_t& cursor,ImpactReceipt& event,bool& overflow){
    if(cursor>writeSerial){cursor=writeSerial;overflow=true;}
    if(writeSerial-cursor>Capacity){cursor=writeSerial-Capacity;overflow=true;}
    if(cursor==writeSerial)return false;
    event=queue[cursor%Capacity];++cursor;
    if(event.serial!=cursor){overflow=true;return false;}
    return true;
}
} // namespace humanlike_bot_point_impacts
