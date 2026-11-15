#pragma once
#include "humanlike_bot_types.h"
#include "humanlike_bot_math.h"
#include "humanlike_bot_config.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

// Portable local static-geometry proposal/proof helper. Queries are supplied by
// the existing owned-GT space pass; this helper creates no timer or input and
// has no actor/roster/target access. A proposal is never a proof.
namespace humanlike_bot_local_cover {
namespace dto=humanlike_bot;
namespace math=dto::math;
namespace cfg=humanlike_bot_config;
enum class Ray:uint8_t {Unknown,Blocked,Clear};
enum class Afford:uint8_t {Ready,Deferred,Impossible};
enum class Result:uint8_t {Invalid,Disabled,Rejected,Deferred,Ready,Retained};
enum class Source:uint8_t {OwnView,Navigation};
struct Settings {
    float nearCm=0,farCm=0,focusCm=0,maximumCm=0,arrivalCm=0,minimumConfidence=0;
    float horizontalFov=0,verticalFov=0;double rememberSeconds=0,sightSeconds=0;
    uint32_t maximumExposed=0;
};
inline float Value(const cfg::Profile&p,cfg::Param id){return cfg::Get(p,id);}
inline Settings FromProfile(const cfg::Profile&p,float radius){
    Settings s;
    for(size_t n=0;n<cfg::PeekCount;++n){const auto kind=static_cast<cfg::Peek>(n);
        // A zero-weight technique cannot be selected by Brain::PickPeek.
        // It must not reserve a span that can starve the actual choices.
        if(Value(p,cfg::PeekParam(kind,cfg::PeekField::Enabled))!=1
           ||Value(p,cfg::PeekParam(kind,cfg::PeekField::Weight))<=0)continue;
        float extent=Value(p,cfg::PeekParam(kind,cfg::PeekField::DistanceCm))
            +Value(p,cfg::PeekParam(kind,cfg::PeekField::DistanceErrorCm));
        if(kind==cfg::Peek::Poppin)extent+=Value(p,cfg::Param::PoppinExtraDistanceCm);
        extent=(std::max)(extent,Value(p,cfg::PeekParam(kind,cfg::PeekField::MinimumSpaceCm)));
        const bool shortSpan=kind==cfg::Peek::Short||kind==cfg::Peek::SlowCreep||kind==cfg::Peek::Crouch||kind==cfg::Peek::Shoulder;
        (shortSpan?s.nearCm:s.farCm)=(std::max)(shortSpan?s.nearCm:s.farCm,extent);
    }
    s.maximumCm=Value(p,cfg::Param::CoverMaximumAnnotationDistanceCm);
    s.arrivalCm=Value(p,cfg::Param::MoveArrivalCm);
    s.minimumConfidence=Value(p,cfg::Param::CoverMinimumConfidence);
    s.maximumExposed=static_cast<uint32_t>(Value(p,cfg::Param::CoverMaximumExposedAngles));
    s.rememberSeconds=Value(p,cfg::Param::SearchAngleMemoryMs)*.001;
    s.sightSeconds=Value(p,cfg::Param::VisualStaleMs)*.001;
    // This is a static possible angle at own head height, not an enemy point.
    // The reach uses existing search/cover preferences and actual capsule size.
    s.focusCm=(std::min)(Value(p,cfg::Param::VisualMaxDistanceCm),
        (std::min)(s.maximumCm,(std::max)(Value(p,cfg::Param::HoldCoverDistanceCm),
            Value(p,cfg::Param::SearchStepCm)+radius+radius)));
    s.horizontalFov=Value(p,cfg::Param::VisualFovHorizontal);
    s.verticalFov=Value(p,cfg::Param::VisualFovVertical);
    return s;
}
inline bool Same(const Settings&a,const Settings&b){
    return a.nearCm==b.nearCm&&a.farCm==b.farCm&&a.focusCm==b.focusCm&&a.maximumCm==b.maximumCm
        &&a.arrivalCm==b.arrivalCm&&a.minimumConfidence==b.minimumConfidence&&a.maximumExposed==b.maximumExposed
        &&a.rememberSeconds==b.rememberSeconds&&a.sightSeconds==b.sightSeconds
        &&a.horizontalFov==b.horizontalFov&&a.verticalFov==b.verticalFov;
}
inline bool Valid(const Settings&s){
    return math::Finite(s.nearCm)&&math::Finite(s.farCm)&&math::Finite(s.focusCm)&&math::Finite(s.maximumCm)
        &&math::Finite(s.arrivalCm)&&math::Finite(s.minimumConfidence)&&math::Finite(s.rememberSeconds)
        &&math::Finite(s.sightSeconds)
        &&math::Finite(s.horizontalFov)&&math::Finite(s.verticalFov)
        &&s.nearCm>=0&&s.farCm>=0&&s.focusCm>=0&&s.maximumCm>=0&&s.arrivalCm>0
        &&s.minimumConfidence>=0&&s.minimumConfidence<=1&&s.rememberSeconds>=0&&s.sightSeconds>=0&&s.maximumExposed>0
        &&s.horizontalFov>0&&s.horizontalFov<=360&&s.verticalFov>0&&s.verticalFov<=180;
}
inline uint32_t GroundCost(dto::Vec3 a,dto::Vec3 b,float radius){
    const double dx=static_cast<double>(b.x)-a.x,dy=static_cast<double>(b.y)-a.y;
    return 1+static_cast<uint32_t>((std::min)(128.0,(std::max)(1.0,
        std::ceil(std::sqrt(dx*dx+dy*dy)/(std::max)(10.0,static_cast<double>(radius))))));
}
struct Proposal {
    bool valid=false;uint32_t id=0;Source source=Source::OwnView;
    dto::Vec3 start{},open{},retreat{},focus{},forward{},right{};
};
struct Work {
    dto::OwnLife life{};Settings settings{};Proposal proposal{};
    uint32_t sequence=0;uint8_t cursor=0,segments=0;bool settingsKnown=false,checkedClosed=false,checkedOpen=false;
    bool crouched=false,ready=false;float radius=0,half=0,eyeHeight=0;
    double generatedAt=-1,provedAt=-1,sightAt=-1;Ray sightRay=Ray::Unknown;
    dto::Vec3 provedFeet{},provedEye{},sightEye{};dto::Rotation provedView{};
    dto::SpaceHints staticProof{},proof{};
    uint64_t proposals=0,rejections=0,deferrals=0,invalidations=0;
};
inline void ClearSight(Work&w){w.sightRay=Ray::Unknown;w.sightAt=-1;w.sightEye={};}
inline void ClearProof(Work&w){w.ready=false;w.staticProof=w.proof={};w.provedAt=-1;ClearSight(w);}
inline void Abandon(Work&w){ClearProof(w);w.proposal={};w.segments=0;w.checkedClosed=w.checkedOpen=false;}
inline void Reject(Work&w){Abandon(w);w.cursor=static_cast<uint8_t>((w.cursor+1)%8);++w.rejections;}
inline float Confidence(const Work&w,double now){
    if(!math::Finite(now)||now<w.generatedAt)return 0;
    if(w.settings.rememberSeconds<=0)return now==w.generatedAt?1.0f:0.0f;
    return math::Clamp(static_cast<float>(1-(now-w.generatedAt)/w.settings.rememberSeconds),0,1);
}
inline bool OwnValid(const dto::OwnSnapshot&o){
    return o.valid&&o.alive&&!o.paused&&!o.retired&&o.life.stableBotId>0&&o.life.spawnEpoch>0&&o.life.token
        &&o.eyeValid&&o.viewValid&&o.stanceValid
        &&math::Finite(o.feet)&&math::Finite(o.eye)&&math::Finite(o.view)&&math::Finite(o.nowSeconds)&&o.nowSeconds>=0;
}
inline bool Grounded(const dto::OwnSnapshot&o){return o.groundedKnown&&o.grounded;}
inline Proposal Make(const dto::OwnSnapshot&o,const Settings&s,float radius,uint8_t cursor,
                     bool navKnown,dto::Vec3 navFocus,uint32_t sequence){
    Proposal p;
    const bool longSpan=(cursor&4)!=0,wantNav=(cursor&2)==0;
    const float extent=longSpan?s.farCm:s.nearCm;
    if(extent<=s.arrivalCm||extent>s.maximumCm||s.focusCm<=radius)return p;
    p.start=p.retreat=o.feet;
    if(wantNav&&navKnown&&math::Finite(navFocus)&&math::Distance(o.eye,navFocus)<=s.maximumCm
       &&math::Horizontal(math::Sub(navFocus,o.eye))>radius){p.focus=navFocus;p.source=Source::Navigation;}
    else if(wantNav&&navKnown)return p;
    else {p.focus=math::Add(o.eye,math::Mul(math::Forward({0,o.view.yaw}),s.focusCm));p.source=Source::OwnView;}
    p.forward=math::FlatUnit(math::Sub(p.focus,o.eye));p.right={-p.forward.y,p.forward.x,0};
    const float sign=(cursor&1)?1.0f:-1.0f;p.open=math::Add(p.start,math::Mul(p.right,sign*extent));
    p.id=0x80000000U|((o.life.stableBotId&31U)<<20)|(sequence&0xfffffU);
    p.valid=math::Finite(p.open)&&math::Finite(p.focus)&&math::Horizontal(p.forward)>0;return p;
}
inline bool VisibleDirection(const dto::OwnSnapshot&o,const Settings&s,dto::Vec3 point){
    const auto look=math::Look(o.eye,point);
    float horizontal=s.horizontalFov;
    if(o.nativeViewFovKnown&&math::Finite(o.nativeHorizontalFovDegrees)&&o.nativeHorizontalFovDegrees>0)
        horizontal=(std::min)(horizontal,o.nativeHorizontalFovDegrees);
    return std::abs(math::Yaw(look.yaw-o.view.yaw))<=horizontal*.5f&&std::abs(look.pitch-o.view.pitch)<=s.verticalFov*.5f;
}
// These predicates deliberately separate static map knowledge, the last
// grounded approach sweep, and a current eye ray. Ground-supported route points
// remain static knowledge in flight; they never assert a jump arc or landing.
inline bool CurrentProof(const Work&w,const dto::OwnSnapshot&o){
    return w.ready&&w.proposal.valid&&OwnValid(o)&&math::Same(w.life,o.life)&&w.crouched==o.crouched
        &&o.nowSeconds>=w.generatedAt&&o.nowSeconds-w.generatedAt<=w.settings.rememberSeconds
        &&Confidence(w,o.nowSeconds)>=w.settings.minimumConfidence
        &&std::abs(w.eyeHeight-(o.eye.z-o.feet.z))<=w.settings.arrivalCm
        &&math::Distance(o.feet,w.proposal.start)<=w.settings.maximumCm;
}
inline bool CurrentApproach(const Work&w,const dto::OwnSnapshot&o){
    return CurrentProof(w,o)&&Grounded(o)&&o.nowSeconds>=w.provedAt
        &&math::Distance(o.feet,w.provedFeet)<=w.settings.arrivalCm;
}
inline bool CurrentSight(const Work&w,const dto::OwnSnapshot&o){
    return CurrentProof(w,o)&&w.sightRay!=Ray::Unknown&&o.nowSeconds>=w.sightAt
        &&o.nowSeconds-w.sightAt<=w.settings.sightSeconds
        &&o.eye.x==w.sightEye.x&&o.eye.y==w.sightEye.y&&o.eye.z==w.sightEye.z;
}
// Always republish against the CURRENT own sample. In particular, a same-eye
// turn recomputes FOV; a changed eye cannot inherit an earlier clear trace.
inline dto::SpaceHints Publish(const Work&w,const dto::OwnSnapshot&o){
    if(!CurrentProof(w,o))return {};
    auto out=w.staticProof;out.coverConfidence=Confidence(w,o.nowSeconds);
    // coverKnown authorizes a NEW ground maneuver and therefore needs the
    // grounded current-approach proof. An already-active airborne maneuver can
    // keep the static route flags below without receiving new eligibility.
    out.coverKnown=out.coverKnown&&CurrentApproach(w,o);
    out.peekSightKnown=CurrentSight(w,o);
    out.peekSightClear=out.peekSightKnown&&w.sightRay==Ray::Clear&&VisibleDirection(o,w.settings,w.proposal.focus);
    out.landingProven=false;return out;
}
// The native publication path supplies CURRENT resolved profile and actual
// capsule dimensions, even on passes when this NPC is not the geometry job.
inline dto::SpaceHints Publish(const Work&w,const dto::OwnSnapshot&o,const Settings&currentSettings,float radius,float half){
    if(!Valid(currentSettings)||!Same(w.settings,currentSettings)||w.radius!=radius||w.half!=half)return {};
    return Publish(w,o);
}
// Query owns the *shared* live budget. Required methods:
// Live(OwnLife), SegmentAfford(a,b,r), Segment(a,b,r,h), TraceAfford(count), Trace(a,b).
// Afford::Impossible denotes a segment/group larger than the configured whole
// pass capacity; it is never retried forever as temporary budget starvation.
template<class Query> inline Result Step(Work&w,const dto::OwnSnapshot&o,const Settings&s,float radius,float half,
                                       bool navKnown,dto::Vec3 navFocus,Query&q){
    if(!OwnValid(o)||!Valid(s)||!math::Finite(radius)||!math::Finite(half)||radius<=0||half<radius||!q.Live(o.life)){
        Abandon(w);++w.invalidations;return Result::Invalid;
    }
    const float height=o.eye.z-o.feet.z;
    const bool changed=!math::Same(w.life,o.life)||!w.settingsKnown||!Same(w.settings,s)
        ||w.radius!=radius||w.half!=half||w.crouched!=o.crouched||std::abs(w.eyeHeight-height)>s.arrivalCm;
    if(changed){const auto proposals=w.proposals,rejections=w.rejections,deferrals=w.deferrals,invalidations=w.invalidations+1;
        w={};w.proposals=proposals;w.rejections=rejections;w.deferrals=deferrals;w.invalidations=invalidations;
        w.life=o.life;w.settings=s;w.settingsKnown=true;w.radius=radius;w.half=half;w.eyeHeight=height;w.crouched=o.crouched;}
    if(s.maximumCm<=0||(s.nearCm<=0&&s.farCm<=0)){Abandon(w);return Result::Disabled;}
    if(w.proposal.valid){
        // Source/range/clock invalidation is not an ordinary completed-proposal
        // lifetime. Life/posture/capsule/profile edits already reset above.
        if(o.nowSeconds<w.generatedAt||math::Distance(o.feet,w.proposal.start)>s.maximumCm)Abandon(w);
        else if(o.nowSeconds-w.generatedAt>s.rememberSeconds||Confidence(w,o.nowSeconds)<s.minimumConfidence){
            const bool completed=w.ready;Abandon(w);
            // A successful short route must not monopolize every later TTL.
            // Continue the existing bounded eight alternatives only after its
            // actual proof expires; do not preempt a currently valid route.
            if(completed)w.cursor=static_cast<uint8_t>((w.cursor+1)%8);
        }
    }
    const auto checked=[&](){if(q.Live(o.life))return true;Abandon(w);++w.invalidations;return false;};
    const auto defer=[&](){++w.deferrals;return Result::Deferred;};
    if(w.ready){
        // Reuse only completed static predicates. No supporting-ground
        // query or proposal is generated from an airborne current foot.
        if(Grounded(o)&&!CurrentApproach(w,o)){
            const auto afford=q.SegmentAfford(o.feet,w.proposal.start,radius);
            if(afford==Afford::Deferred){++w.deferrals;w.proof=Publish(w,o);return Result::Retained;}
            if(afford==Afford::Impossible){Reject(w);return Result::Rejected;}
            if(!checked())return Result::Invalid;const bool okay=q.Segment(o.feet,w.proposal.start,radius,half);
            if(!checked())return Result::Invalid;if(!okay){Reject(w);return Result::Rejected;}
            w.provedAt=o.nowSeconds;w.provedFeet=o.feet;
        }
        const auto afford=q.TraceAfford(1);
        if(afford==Afford::Ready){
            if(!checked())return Result::Invalid;const auto ray=q.Trace(o.eye,w.proposal.focus);
            if(!checked())return Result::Invalid;w.sightRay=ray;w.sightAt=o.nowSeconds;w.sightEye=o.eye;
        }else if(afford==Afford::Deferred)++w.deferrals;else ClearSight(w);
        w.proof=Publish(w,o);return Result::Retained;
    }
    w.proof={};
    if(!Grounded(o))return defer();
    if(!w.proposal.valid){w.proposal=Make(o,s,radius,w.cursor,navKnown,navFocus,++w.sequence);w.generatedAt=o.nowSeconds;++w.proposals;
        if(!w.proposal.valid){Reject(w);return Result::Rejected;}}
    const auto&p=w.proposal;
    auto closed=p.start,open=p.open;closed.z+=w.eyeHeight;open.z+=w.eyeHeight;
    // Cheap actual occlusion/opening rejection precedes supporting-ground work.
    if(!w.checkedClosed||!w.checkedOpen){const auto afford=q.TraceAfford(2);
        if(afford==Afford::Deferred)return defer();if(afford==Afford::Impossible){Reject(w);return Result::Rejected;}
        if(!checked())return Result::Invalid;const auto a=q.Trace(closed,p.focus);if(!checked())return Result::Invalid;
        const auto b=q.Trace(open,p.focus);if(!checked())return Result::Invalid;
        if(a!=Ray::Blocked||b!=Ray::Clear){Reject(w);return Result::Rejected;}
        w.checkedClosed=w.checkedOpen=true;
    }
    const std::array<dto::Vec3,3> starts{p.start,p.open,p.retreat},ends{p.open,p.retreat,p.start};
    for(uint8_t n=0;n<starts.size();++n)if(!(w.segments&(1U<<n))){const auto afford=q.SegmentAfford(starts[n],ends[n],radius);
        if(afford==Afford::Deferred)return defer();if(afford==Afford::Impossible){Reject(w);return Result::Rejected;}
        if(!checked())return Result::Invalid;const bool okay=q.Segment(starts[n],ends[n],radius,half);
        if(!checked())return Result::Invalid;if(!okay){Reject(w);return Result::Rejected;}w.segments|=static_cast<uint8_t>(1U<<n);
    }
    const auto approach=q.SegmentAfford(o.feet,p.start,radius);
    if(approach==Afford::Deferred)return defer();if(approach==Afford::Impossible){Reject(w);return Result::Rejected;}
    if(!checked())return Result::Invalid;const bool walkable=q.Segment(o.feet,p.start,radius,half);
    if(!checked())return Result::Invalid;if(!walkable){Reject(w);return Result::Rejected;}
    const auto finalAfford=q.TraceAfford(5);
    if(finalAfford==Afford::Deferred)return defer();if(finalAfford==Afford::Impossible){Reject(w);return Result::Rejected;}
    std::array<Ray,5> rays{};
    const float focusRange=(std::min)(s.maximumCm,(std::max)(math::Horizontal(math::Sub(p.focus,closed)),
        math::Horizontal(math::Sub(p.open,p.start))+radius+radius));
    const std::array<dto::Vec3,5> from{closed,open,o.eye,open,open};
    const std::array<dto::Vec3,5> to{p.focus,p.focus,p.focus,
        math::Add(open,math::Mul(p.right,focusRange)),math::Add(open,math::Mul(p.right,-focusRange))};
    for(size_t n=0;n<rays.size();++n){if(!checked())return Result::Invalid;rays[n]=q.Trace(from[n],to[n]);
        if(!checked())return Result::Invalid;if(rays[n]==Ray::Unknown){Reject(w);return Result::Rejected;}}
    const uint32_t exposed=1U+static_cast<uint32_t>(rays[3]==Ray::Clear)+static_cast<uint32_t>(rays[4]==Ray::Clear);
    if(rays[0]!=Ray::Blocked||rays[1]!=Ray::Clear||exposed>s.maximumExposed){Reject(w);return Result::Rejected;}
    const float confidence=Confidence(w,o.nowSeconds);
    if(confidence<s.minimumConfidence){Reject(w);return Result::Rejected;}
    auto&out=w.staticProof;out.coverStart=p.start;out.peekOpen=p.open;out.retreatPoint=p.retreat;
    out.coverKnown=out.groundProven=out.peekPathProven=out.retreatPathProven=true;
    out.jumpLanding=p.open;out.landingSupportKnown=true;out.landingProven=false;
    out.peekSightKnown=out.peekSightClear=false;
    w.sightRay=rays[2];w.sightAt=o.nowSeconds;w.sightEye=o.eye;
    out.coverConfidence=confidence;out.exposedAngleCount=exposed;out.exposureRisk=static_cast<float>(exposed)/3.0f;
    out.angles[0]={p.id,true,true,false,p.focus,out.exposureRisk,0};out.angleCount=1;
    out.angles[0].coverFocusProven=true;
    // Confidence is an explicit age factor over fully completed geometry
    // predicates, governed by the existing angle-memory duration. Risk is
    // clear directions among these three sampled headings, never enemy odds or
    // proof that all other map angles are safe. Focus height is not enemy data.
    w.ready=true;w.provedAt=o.nowSeconds;w.provedFeet=o.feet;w.provedEye=o.eye;w.provedView=o.view;
    w.proof=Publish(w,o);return Result::Ready;
}
} // namespace humanlike_bot_local_cover
