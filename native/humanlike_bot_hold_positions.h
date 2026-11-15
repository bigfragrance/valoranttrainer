#pragma once
#include "humanlike_bot_math.h"
#include <cmath>

// Pure authored-position policy. Unknown static risk never means safe; map
// proposal admission and original path/ground/cover proofs remain separate.
namespace humanlike_bot_hold_positions {
namespace dto=humanlike_bot;namespace math=dto::math;
struct Settings {
    float conventional=0,unconventional=0,coverDistance=0,arrival=0,maximumDistance=0,minimumConfidence=0;
    float safety=0,flankConcern=0,angleConcern=0;uint32_t maximumExposedAngles=0;
};
inline bool SamePoint(dto::Vec3 a,dto::Vec3 b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
inline bool SameLocation(const dto::HoldLocation&a,const dto::HoldLocation&b){
    return a.valid&&b.valid&&a.id==b.id&&a.revision==b.revision&&a.staticMapKnowledge==b.staticMapKnowledge
        &&a.conventional==b.conventional&&a.range==b.range&&SamePoint(a.position,b.position)&&SamePoint(a.cover,b.cover)
        &&SamePoint(a.retreat,b.retreat)&&SamePoint(a.focus,b.focus)&&a.confidence==b.confidence&&a.risk==b.risk
        &&a.flankRisk==b.flankRisk&&a.exposedAngles==b.exposedAngles&&a.riskKnown==b.riskKnown
        &&a.flankRiskKnown==b.flankRiskKnown&&a.exposedAnglesKnown==b.exposedAnglesKnown;
}
inline bool Eligible(const dto::HoldLocation&r,const dto::OwnSnapshot&own,const Settings&s){
    return r.valid&&r.staticMapKnowledge&&r.id&&r.revision&&math::Finite(r.position)&&math::Finite(r.cover)
        &&math::Finite(r.retreat)&&math::Finite(r.focus)&&math::Finite(r.confidence)&&r.confidence>0&&r.confidence<=1
        &&r.confidence>=s.minimumConfidence&&math::Finite(r.risk)&&r.risk>=0&&r.risk<=1
        &&math::Finite(r.flankRisk)&&r.flankRisk>=0&&r.flankRisk<=1
        &&r.exposedAnglesKnown&&r.exposedAngles>0&&r.exposedAngles<=s.maximumExposedAngles
        &&math::Distance(r.position,r.cover)>0&&math::Distance(r.position,r.focus)>0
        &&math::Distance(own.feet,r.position)<=s.maximumDistance;
}
inline float Weight(const dto::HoldLocation&r,const dto::OwnSnapshot&own,const Settings&s){
    if(!Eligible(r,own,s)||s.arrival<=0)return 0;
    const auto preference=r.conventional?s.conventional:s.unconventional;if(preference<=0)return 0;
    const auto risk=r.riskKnown?r.risk:1.f,flank=r.flankRiskKnown?r.flankRisk:1.f;
    const auto angles=static_cast<float>(r.exposedAngles)/(std::max)(1.f,static_cast<float>(s.maximumExposedAngles));
    const auto safety=(std::max)(0.f,1-risk*s.safety),side=(std::max)(0.f,1-flank*s.flankConcern);
    const auto exposure=(std::max)(0.f,1-angles*s.angleConcern);
    // Closeness to the independently editable preferred cover offset. The
    // existing arrival tolerance supplies its spatial resolution in cm.
    const auto offset=math::Distance(r.position,r.cover);
    const auto distancePreference=1.f/(1.f+std::abs(offset-s.coverDistance)/s.arrival);
    return preference*safety*side*exposure*distancePreference;
}
inline bool Arrived(const dto::OwnSnapshot&own,dto::Vec3 goal,float arrival){
    const auto delta=math::Sub(own.feet,goal);
    return own.groundedKnown&&own.grounded&&math::Finite(delta)&&math::Horizontal(delta)<=arrival&&std::abs(delta.z)<=arrival;
}
} // namespace humanlike_bot_hold_positions
