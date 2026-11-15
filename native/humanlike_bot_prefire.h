#pragma once
#include "humanlike_bot_math.h"
namespace humanlike_bot_prefire {
namespace dto=humanlike_bot;namespace math=dto::math;
inline bool Point(dto::Vec3 a,dto::Vec3 b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
inline bool Valid(const dto::EstimatedAim&r,const dto::OwnSnapshot&own,double now){
    const bool heard=r.source==dto::EstimatedAimSource::Heard;
    const bool authored=r.source==dto::EstimatedAimSource::AuthoredAngle||r.source==dto::EstimatedAimSource::AuthoredHold;
    return r.requested&&r.key&&r.sourceEpisode&&(heard||authored)&&math::Same(r.life,own.life)
        &&own.valid&&own.alive&&!own.paused&&!own.retired&&own.life.stableBotId&&own.life.spawnEpoch&&own.life.token
        &&own.eyeValid&&math::Finite(own.eye)&&math::Finite(r.point)&&math::Distance(own.eye,r.point)>0
        &&math::Finite(now)&&now==own.nowSeconds&&math::Finite(r.acquiredAtSeconds)&&r.acquiredAtSeconds>=0
        &&math::Finite(r.readyAtSeconds)&&r.readyAtSeconds>=r.acquiredAtSeconds
        &&math::Finite(r.expiresAtSeconds)&&r.acquiredAtSeconds<=now&&now<r.expiresAtSeconds
        &&math::Finite(r.confidence)&&r.confidence>0&&r.confidence<=1&&math::Finite(r.uncertaintyCm)&&r.uncertaintyCm>=0
        &&(heard?(!r.mapAnnotationId&&!r.mapRevision):(r.mapAnnotationId&&r.mapRevision));
}
inline bool Same(const dto::EstimatedAim&a,const dto::EstimatedAim&b){
    return a.requested&&b.requested&&a.key==b.key&&math::Same(a.life,b.life)&&a.source==b.source&&a.sourceEpisode==b.sourceEpisode
        &&Point(a.point,b.point)&&a.mapAnnotationId==b.mapAnnotationId&&a.mapRevision==b.mapRevision
        &&a.acquiredAtSeconds==b.acquiredAtSeconds&&a.readyAtSeconds==b.readyAtSeconds&&a.expiresAtSeconds==b.expiresAtSeconds
        &&a.confidence==b.confidence&&a.uncertaintyCm==b.uncertaintyCm;
}
inline bool Proof(const dto::EstimatedAim&r,const dto::SensorFrame&f,double maximumAge){
    const auto&s=f.space;return Valid(r,f.self,f.self.nowSeconds)&&Same(r,s.estimatedAim)
        &&s.estimatedAimKnown&&s.estimatedAimClear&&Point(s.estimatedAimEye,f.self.eye)
        &&math::Finite(s.estimatedAimAtSeconds)&&s.estimatedAimAtSeconds>=r.acquiredAtSeconds
        &&s.estimatedAimAtSeconds<=f.self.nowSeconds&&f.self.nowSeconds-s.estimatedAimAtSeconds<=maximumAge;
}
} // namespace humanlike_bot_prefire
