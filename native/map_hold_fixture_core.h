#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <type_traits>

// Pure Scenario14 acceptance policy. Called only by the existing NPC boundary.
// A passed query or accepted intent is never a physical arrival receipt.
namespace map_hold_fixture_core {
struct Point {double x=0,y=0,z=0;};
inline bool Finite(Point p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
inline Point Sub(Point a,Point b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline double Length(Point p){return std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z);}
inline bool Arrived(Point actual,Point goal,double tolerance){
    const auto d=Sub(actual,goal);return Finite(d)&&std::isfinite(tolerance)&&tolerance>0
        &&std::hypot(d.x,d.y)<=tolerance&&std::abs(d.z)<=tolerance;
}
struct Life {uint32_t id=0,epoch=0;uint64_t token=0,source=0;};
inline bool Same(Life a,Life b){return a.id&&a.epoch&&a.token&&a.source&&a.id==b.id&&a.epoch==b.epoch&&a.token==b.token&&a.source==b.source;}
struct Configuration {
    uint32_t annotation=0,revision=0;bool conventional=false;
    Point start{},cover{},open{},retreat{},focus{};
    double arrival=18,dwell=2.5,maxSampleGap=.25,riskReach=2000;
};
enum class Query:uint8_t {Deferred,Rejected,Passed};
enum class Ray:uint8_t {Unknown,Blocked,Clear};
enum class Stage:uint8_t {Path,GroundCover,GroundOpen,GroundRetreat,Outbound,Return,ClosedLoop,CoverLOS,OpenLOS,Risk0,Risk1,Risk2,Risk3,Ready,Rejected};
struct Preflight {
    Stage stage=Stage::Path;Life life{};
    bool bound=false;uint8_t segments=0,ground=0;
    std::array<Point,64> path{};uint32_t pathCount=0;
    std::array<Ray,6> rays{};std::array<Point,4> riskEnds{};
    std::array<double,6> rayAt{};std::array<Point,3> actualGround{};
    double eyeHeight=0,radius=0,half=0,startedAt=-1,completedAt=-1;
    uint32_t exposed=0;double risk=-1,flankRisk=-1;
    // Completion and budget deferral are separate facts. A pending stage is
    // never converted to a passed query by a frame/timer count.
};
inline bool RiskEnds(const Configuration& c,double eyeHeight,std::array<Point,4>& out){
    const auto d=Sub(c.focus,c.open);const auto horizontal=std::hypot(d.x,d.y);
    if(!Finite(d)||!std::isfinite(eyeHeight)||eyeHeight<=0||!std::isfinite(c.riskReach)||c.riskReach<=0||horizontal<=0)return false;
    const double reach=(std::min)(horizontal,c.riskReach),x=d.x/horizontal*reach,y=d.y/horizontal*reach;
    out[0]=c.focus;out[1]={c.open.x-y,c.open.y+x,c.focus.z};
    out[2]={c.open.x+y,c.open.y-x,c.focus.z};out[3]={c.open.x-x,c.open.y-y,c.focus.z};return true;
}
inline Query Reject(Preflight& p){p.stage=Stage::Rejected;return Query::Rejected;}
template<class API>inline Query Step(Preflight& p,const Configuration& c,Life life,double now,double eyeHeight,double radius,double half,API& api){
    if(p.stage==Stage::Rejected)return Query::Rejected;
    if(!Same(life,life)||!std::isfinite(now)||now<0||!std::isfinite(radius)||!std::isfinite(half)||radius<=0||half<radius
       ||!std::isfinite(eyeHeight)||eyeHeight<=0||!Finite(c.cover)||!Finite(c.open)||!Finite(c.retreat)||!Finite(c.focus)
       ||!c.annotation||!c.revision||c.arrival<=0)return Reject(p);
    if(!p.bound){p.bound=true;p.life=life;p.startedAt=now;p.eyeHeight=eyeHeight;p.radius=radius;p.half=half;
        if(!RiskEnds(c,eyeHeight,p.riskEnds))return Reject(p);}
    if(!Same(p.life,life)||now<p.startedAt||p.radius!=radius||p.half!=half||std::abs(p.eyeHeight-eyeHeight)>.5)return Reject(p);
    if(p.stage==Stage::Ready)return Query::Passed;
    const auto n=static_cast<unsigned>(p.stage);Query result=Query::Rejected;
    if(p.stage==Stage::Path){std::array<Point,64> actual{};uint32_t count=0;result=api.Path(c.open,actual,count);
        if(result==Query::Passed){if(count<2||count>actual.size()||!Arrived(actual[count-1],c.open,c.arrival))return Reject(p);
            for(uint32_t i=0;i<count;++i)if(!Finite(actual[i]))return Reject(p);p.path=actual;p.pathCount=count;}}
    else if(n>=1&&n<=3){const Point points[]{c.cover,c.open,c.retreat};Point actual{};
        result=api.Ground(points[n-1],actual);if(result==Query::Passed){if(!Arrived(actual,points[n-1],c.arrival))return Reject(p);p.actualGround[n-1]=actual;p.ground|=uint8_t(1U<<(n-1));}}
    else if(n>=4&&n<=6){const Point starts[]{c.cover,c.open,c.retreat},ends[]{c.open,c.retreat,c.cover};
        result=api.Segment(starts[n-4],ends[n-4],radius,half);if(result==Query::Passed)p.segments|=uint8_t(1U<<(n-4));}
    else{
        const unsigned ray=n-7;auto start=ray==0?c.cover:c.open;start.z+=p.eyeHeight;
        const auto end=ray<2?c.focus:p.riskEnds[ray-2];Ray measured=Ray::Unknown;
        result=api.Trace(start,end,measured);
        if(result==Query::Passed){p.rays[ray]=measured;p.rayAt[ray]=now;
            if(measured==Ray::Unknown||(ray==0&&measured!=Ray::Blocked)||(ray==1&&measured!=Ray::Clear))return Reject(p);}
    }
    if(result==Query::Deferred)return result;
    if(result!=Query::Passed||!api.Fence(life))return Reject(p);
    p.stage=static_cast<Stage>(n+1);
    if(p.stage!=Stage::Ready)return Query::Deferred;
    for(unsigned i=2;i<6;++i)if(p.rays[i]==Ray::Clear)++p.exposed;
    if(p.ground!=7||p.segments!=7||!p.exposed)return Reject(p);
    unsigned side=0;for(unsigned i=3;i<6;++i)if(p.rays[i]==Ray::Clear)++side;
    p.risk=p.exposed/4.;p.flankRisk=side/3.;p.completedAt=now;return Query::Passed;
}
struct Sample {
    Life life{};double now=-1,sampledAt=-1;Point ownFeet{},physicalFeet{},velocity{};
    uint64_t physics=0,submissions=0,key=0;uint32_t annotation=0,revision=0;
    bool physicalValid=false,groundKnown=false,grounded=false,mapRequest=false,staticMap=false;
    bool successfulSubmit=false,navigationRequested=false,holdProof=false,goalReached=false,released=false;
};
enum class Result:uint8_t {Pending,Rejected,Dwelled,Released};
struct Acceptance {
    Life life{};bool bound=false,startedOutside=false,moved=false,navigationSubmitted=false,arrived=false,dwelled=false,releaseSubmitted=false;
    uint64_t key=0,firstPhysics=0,lastPhysics=0,releasePhysics=0;
    double firstAt=-1,lastAt=-1,arrivedAt=-1,lastDwellAt=-1,dwellSeconds=0;
    Point initialFeet{},arrivalFeet{},lastFeet{};
};
inline Result Observe(Acceptance& a,const Configuration& c,const Sample& s){
    if(!Same(s.life,s.life)||!s.physicalValid||!s.groundKnown||!std::isfinite(s.now)||!std::isfinite(s.sampledAt)
       ||s.sampledAt>s.now||s.now-s.sampledAt>c.maxSampleGap||!Finite(s.ownFeet)||!Finite(s.physicalFeet)
       ||!Arrived(s.ownFeet,s.physicalFeet,c.arrival)||!s.successfulSubmit)return Result::Rejected;
    if(!a.bound){a.bound=true;a.life=s.life;a.initialFeet=s.physicalFeet;a.firstAt=s.now;a.firstPhysics=s.physics;
        a.startedOutside=!Arrived(s.physicalFeet,c.open,c.arrival);}
    if(!Same(a.life,s.life)||s.physics<a.lastPhysics||(a.lastAt>=0&&(s.now<a.lastAt||s.now-a.lastAt>c.maxSampleGap)))return Result::Rejected;
    if(s.mapRequest){
        if(!s.staticMap||s.annotation!=c.annotation||s.revision!=c.revision||!s.key||(a.key&&s.key!=a.key))return Result::Rejected;
        a.key=s.key;if(s.navigationRequested)a.navigationSubmitted=true;
    }
    const bool newPhysics=s.physics>a.lastPhysics;
    a.lastPhysics=s.physics;a.lastAt=s.now;a.lastFeet=s.physicalFeet;
    a.moved=a.moved||(s.physics>a.firstPhysics&&Length(Sub(s.physicalFeet,a.initialFeet))>c.arrival);
    if(a.dwelled){if(s.released){a.releaseSubmitted=true;a.releasePhysics=s.physics;return Result::Released;}return Result::Dwelled;}
    const bool at=s.grounded&&Finite(s.velocity)&&s.velocity.x==0&&s.velocity.y==0&&s.velocity.z==0
        &&s.holdProof&&s.goalReached&&a.key&&a.navigationSubmitted&&a.startedOutside&&a.moved
        &&Arrived(s.physicalFeet,c.open,c.arrival)&&Arrived(s.ownFeet,c.open,c.arrival);
    if(a.arrived&&!at)return Result::Rejected;
    if(!a.arrived&&at){a.arrived=true;a.arrivedAt=s.now;a.arrivalFeet=s.physicalFeet;a.lastDwellAt=s.now;}
    if(a.arrived&&newPhysics){
        if(s.navigationRequested)return Result::Rejected;
        a.lastDwellAt=s.now;a.dwellSeconds=s.now-a.arrivedAt;
        if(a.dwellSeconds>=c.dwell){a.dwelled=true;return Result::Dwelled;}
    }
    return Result::Pending;
}
static_assert(std::is_trivially_copyable_v<Preflight>&&std::is_trivially_copyable_v<Sample>&&std::is_trivially_copyable_v<Acceptance>);
} // namespace map_hold_fixture_core
