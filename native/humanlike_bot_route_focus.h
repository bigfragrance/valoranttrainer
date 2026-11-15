#pragma once
#include "humanlike_bot_math.h"
#include <cstddef>
#include <cstdint>

// The only inputs are the own life/eye and an original static path. A focus
// ray never creates a visual contact or gives permission to fire.
namespace humanlike_bot_route_focus {
namespace dto=humanlike_bot;
enum class Ray:uint8_t {Unknown,Clear,Blocked,Deferred};
struct State {
    dto::OwnLife life{};uint64_t generation=0;size_t pathAt=0,node=0,cursor=0;
    bool known=false,haveVisible=false,retryVisible=false;
    dto::Vec3 focus{},eye{};double at=-1;float standingEyeHeight=0;Ray lastRay=Ray::Unknown;
};
inline bool Current(const State&s,const dto::OwnSnapshot&own,float arrival,double maximumAge){
    return s.known&&dto::math::Same(s.life,own.life)&&own.valid&&own.alive&&own.eyeValid
        &&dto::math::Finite(own.eye)&&dto::math::Finite(s.focus)&&std::isfinite(maximumAge)&&maximumAge>0
        &&std::isfinite(arrival)&&arrival>0&&std::isfinite(own.nowSeconds)&&s.at>=0&&own.nowSeconds>=s.at&&own.nowSeconds-s.at<=maximumAge
        &&std::isfinite(own.sampledAtSeconds)&&own.sampledAtSeconds>=0&&own.sampledAtSeconds<=own.nowSeconds
        &&own.nowSeconds-own.sampledAtSeconds<=maximumAge&&dto::math::Distance(own.eye,s.eye)<=arrival
        &&dto::math::Horizontal(dto::math::Sub(s.focus,own.eye))>arrival;
}
template<class PointAt,class Probe>inline void Step(State&s,const dto::OwnSnapshot&own,uint64_t generation,
    size_t pathAt,size_t count,bool valid,float arrival,double maximumAge,PointAt pointAt,Probe probe){
    const bool sameLife=dto::math::Same(s.life,own.life);
    if(!sameLife){s={};s.life=own.life;}
    const float height=own.eye.z-own.feet.z;
    if(own.valid&&own.eyeValid&&own.stanceValid&&!own.crouched&&std::isfinite(height)&&height>0)s.standingEyeHeight=height;
    if(!valid||!own.life.stableBotId||!own.life.token||!own.life.spawnEpoch||!own.valid||!own.alive||own.retired||own.paused
       ||!own.eyeValid||!dto::math::Finite(own.eye)||!dto::math::Finite(own.feet)||!std::isfinite(own.nowSeconds)
       ||!generation||!count||pathAt>=count||!std::isfinite(height)||height<=0||!std::isfinite(maximumAge)||maximumAge<=0
       ||!std::isfinite(own.sampledAtSeconds)||own.sampledAtSeconds<0||own.sampledAtSeconds>own.nowSeconds
       ||own.nowSeconds-own.sampledAtSeconds>maximumAge){s.known=false;return;}
    if(s.generation!=generation||s.pathAt!=pathAt){
        const auto standing=s.standingEyeHeight;s={};s.life=own.life;s.generation=generation;
        s.pathAt=s.node=s.cursor=pathAt;s.standingEyeHeight=standing;
    }
    if(!Current(s,own,arrival,maximumAge))s.known=false;
    const auto candidate=s.retryVisible&&s.haveVisible?s.node:s.cursor;
    if(candidate>=count){s.known=false;s.haveVisible=s.retryVisible=false;s.cursor=pathAt;return;}
    auto point=pointAt(candidate);
    if(!dto::math::Finite(point)){s.known=false;s.lastRay=Ray::Unknown;return;}
    point.z+=s.standingEyeHeight>0?s.standingEyeHeight:height;
    // An almost reached node gives no useful horizontal look direction.
    if(dto::math::Horizontal(dto::math::Sub(point,own.eye))<=arrival){
        s.known=false;
        if(candidate+1<count){s.cursor=candidate+1;s.retryVisible=false;}
        return;
    }
    s.lastRay=probe(own.eye,point);
    if(s.lastRay==Ray::Deferred)return;
    if(s.lastRay==Ray::Clear){
        s.focus=point;s.eye=own.eye;s.at=own.nowSeconds;s.known=s.haveVisible=true;s.node=candidate;
        s.cursor=candidate+1<count?candidate+1:candidate;s.retryVisible=false;
    }else{
        if(candidate==s.node||!Current(s,own,arrival,maximumAge))s.known=false;
        s.retryVisible=s.haveVisible;
        if(!s.haveVisible)s.cursor=pathAt;
    }
}
}
