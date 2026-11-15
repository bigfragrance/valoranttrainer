#pragma once
#include "local_server_humanlike_sensors.h"
#include "humanlike_bot_space_data.h"
#include "humanlike_bot_space_authored.h"
#include "humanlike_bot_local_cover.h"
#include "humanlike_bot_navigation_progress.h"
#include "humanlike_bot_tactics_navigation.h"
#include "humanlike_bot_hold_positions.h"
#include "humanlike_bot_prefire.h"
#include "humanlike_bot_route_focus.h"
#include "humanlike_capsule_profile_comparison.h"
#include <array>
#include <cmath>
#include <cstring>

// Native static-map proposals and proofs, serviced inside the same sensor pass.
// A proposal is not movement execution, a ground-supported landing point is not
// an original jump-link reachability proof, and static risk is not enemy data.
namespace local_server_humanlike_space {
namespace sensors=local_server_humanlike_sensors;
namespace core=humanlike_bot_sensors;
namespace dto=humanlike_bot;
namespace cfg=humanlike_bot_config;
namespace data=humanlike_bot_space_data;
namespace local=humanlike_bot_local_cover;
namespace progress=humanlike_bot_navigation_progress;
namespace patrol=humanlike_bot_route_focus;
inline constexpr size_t MaxPlayers=original_players::MaxPlayers,MaxPath=64;
enum class GeometryResult:uint8_t {Unknown,Deferred,Blocked,Clear};
struct PathAdmission {
    bool pending=false,projectedArrived=false,fullConnection=false,groundRequired=false;
    bool sweepDone=false,projectedGroundKnown=false;
    dto::OwnLife life{};uint64_t generation=0;uint32_t goal=0;
    size_t node=1,groundCursor=0,groundSamples=0;
    double radius=0,half=0,step=0,at=-1;
    engine::Vec origin{},end{},destination{},previousGround{},sweepFrom{},sweepTo{},rawProjection{},projectedGround{};
    GeometryResult result=GeometryResult::Unknown;
    const char* stage="path_admission_not_observed";
    humanlike_capsule_profile_comparison::Comparison collision{};
};

struct Cache {
    dto::OwnLife life{};uint32_t goalId=0,goalSequence=0,annotationId=0;bool haveGoal=false,pathValid=false;
    std::array<engine::Vec,MaxPath> path{};size_t pathCount=0,pathAt=0;engine::Vec goal{};
    uint64_t pathGeneration=0,recoveryReplans=0,recoveryGoalChanges=0;
    uint32_t failedGoalId=0;uint8_t pathQueryFailures=0;bool recoveryRetried=false;progress::State progress{};
    double nextPlan=0;uint8_t routeSegments=0;bool coverLOS=false,coverBlocked=false,openClear=false;
    dto::SpaceHints proofs{};dto::Vec3 proofFeet{};float radius=0,half=0;local::Work localCover{};
    dto::EstimatedNavigation navigationEstimate{};bool estimatedGoalReached=false,estimatedGoalRejected=false;
    dto::EstimatedAim aimEstimate{};bool aimKnown=false,aimClear=false;dto::Vec3 aimEye{};double aimAt=-1;
    patrol::State routeFocus{};
    PathAdmission admission{};
    bool blockedBoundaryKnown=false;
    std::array<bool,training_bot_controller::MaxAnchors> recoveryAnchorsTried{};
    engine::Vec blockedBoundaryFeet{},blockedBoundaryNormal{};
    double blockedBoundaryRadius=0,blockedBoundaryStep=0;
    bool slopeProbePending=false;
    uint64_t slopeProbeGeneration=0;size_t slopeProbeNode=0;
    double slopeProbeAt=-1,slopeProbeRadius=0,slopeProbeHalf=0,slopeProbeStep=0;
    engine::Vec slopeProbeFrom{},slopeProbeTo{},slopeProbeRaisedTo{};
    GeometryResult slopeProbeResult=GeometryResult::Unknown,slopeProbeRaisedResult=GeometryResult::Unknown;
    humanlike_capsule_profile_comparison::Comparison slopeProbeCollision{},slopeProbeRaisedCollision{};
    bool slopeProbeSupportKnown=false,slopeProbeSupportPresent=false;
    bool slopeProbeStartSupportKnown=false,slopeProbeStartSupportPresent=false;
    engine::Vec slopeProbeSupport{},slopeProbeStartSupport{};
    double movementCheckedAt=-1;size_t movementCheckedNode=0;
    bool slopeTransitionPending=false;uint64_t slopeTransitionGeneration=0,slopeTransitionPhysics=0,slopeTransitionNative=0;
    size_t slopeTransitionNode=0;double slopeTransitionOpportunityAt=-1;
    GeometryResult cornerResult=GeometryResult::Unknown;double cornerRadius=0,cornerHalf=0;
    uint64_t cornerQueries=0,cornerAdvances=0,cornerDeferrals=0,cornerGeneration=0;
    size_t cornerNode=0;double cornerAt=-1;bool cornerProven=false;
    engine::Vec cornerFrom{},cornerCurrent{},cornerSuccessor{};
    humanlike_capsule_profile_comparison::Comparison cornerProfile{};
    uint64_t cornerProfileSamples=0,cornerProfileBudgetDeferrals=0,cornerProfileGeneration=0;
    size_t cornerProfileNode=0;
    double cornerProfileAt=-1,cornerProfileNextAt=-1;
    bool cornerProfileOldSegment=false;
    engine::Vec cornerProfileFrom{},cornerProfileTo{};
};
struct State {
    bool initialized=false;uint64_t seed=0,passes=0,paths=0,pathFailures=0,localPaths=0;
    uint64_t coverValidations=0,coverFailures=0,budgetDeferred=0,clearanceProbes=0,sightProbes=0;
    uint64_t localCoverCandidates=0,localCoverReady=0,localCoverRejected=0,localCoverDeferred=0,localCoverInvalidated=0;
    uint32_t nextJob=1,eligible=1,groundBudget=0,queryBudget=0;double passAt=-1;
    std::array<data::Annotation,data::Capacity> annotations{};size_t count=0;uint32_t revision=0;
};
inline State state{};inline std::array<Cache,MaxPlayers> caches{};
template<class T>inline T Read(const void*object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline float SpaceValue(const cfg::Profile&p,cfg::Param id){return cfg::Get(p,id);}
// Match the decision's horizontal arrival without discarding height.
// Vertical separation retains the same original arrival bound: another floor
// outside that bound is never consumed merely because its XY is close.
inline bool ReachedPathPoint(const engine::Vec&feet,const engine::Vec&point,double arrival){
    if(!original_players::Finite(feet)||!original_players::Finite(point)||!std::isfinite(arrival)||arrival<0)return false;
    const auto delta=training_bot_controller::Sub(point,feet);
    return training_bot_controller::Length2D(delta)<=arrival&&std::abs(delta.z)<=arrival;
}
inline bool SameLife(const dto::OwnLife&a,const dto::OwnLife&b){return a.stableBotId==b.stableBotId&&a.spawnEpoch==b.spawnEpoch&&a.token==b.token;}
inline bool Context(){return state.initialized&&sensors::Context()&&training_world_geometry::world==sensors::GetState().world;}
inline uint32_t NextNPC(uint32_t after){
    for(uint32_t step=1;step<MaxPlayers;++step){const uint32_t index=1+(after-1+step)%static_cast<uint32_t>(MaxPlayers-1);
        const auto p=local_server_match::Player(index);if(p&&p->present&&!p->human&&p->alive)return index;}
    return after;
}
inline bool Spend(uint32_t ground,uint32_t query){
    if(ground>state.groundBudget||query>state.queryBudget){++state.budgetDeferred;return false;}
    state.groundBudget-=ground;state.queryBudget-=query;return true;
}
inline uint32_t GroundCost(const engine::Vec&from,const engine::Vec&to,double radius){
    const auto dx=to.x-from.x,dy=to.y-from.y;
    // Same sample upper bound as the existing original capsule/support helper.
    return 1+static_cast<uint32_t>((std::min)(128.,(std::max)(1.,std::ceil(std::sqrt(dx*dx+dy*dy)/(std::max)(10.,radius)))));
}
inline bool LocalSource(size_t index,const dto::OwnLife&life);
inline GeometryResult MovementSweep(size_t index,const engine::Vec&from,const engine::Vec&to,double radius,double half,
                                    humanlike_capsule_profile_comparison::Comparison* observation=nullptr,bool spend=true){
    if(!index||index>=MaxPlayers||!LocalSource(index,caches[index].life))return GeometryResult::Unknown;
    if(spend&&!Spend(0,1))return GeometryResult::Deferred;
    const auto life=caches[index].life;const auto*player=local_server_match::Player(index);
    if(!player)return GeometryResult::Unknown;
    const auto result=humanlike_capsule_profile_comparison::Sweep(index,player->pawn,from,to,radius,half);
    if(!LocalSource(index,life)||!result.sweepKnown)return GeometryResult::Unknown;
    if(observation)*observation=result;
    return result.sweepClear?GeometryResult::Clear:GeometryResult::Blocked;
}
inline bool Segment(size_t index,const engine::Vec&from,const engine::Vec&to,double radius,double half,GeometryResult* observation=nullptr){
    if(observation)*observation=GeometryResult::Deferred;
    if(!Spend(GroundCost(from,to,radius),1))return false;
    auto result=MovementSweep(index,from,to,radius,half,nullptr,false);
    if(result==GeometryResult::Clear){
        const auto life=caches[index].life;bool known=false;
        const bool supported=training_world_geometry::ValidateGroundSegment(index,from,to,radius,&known);
        result=!known||!LocalSource(index,life)?GeometryResult::Unknown:supported?GeometryResult::Clear:GeometryResult::Blocked;
    }
    if(observation)*observation=result;
    return result==GeometryResult::Clear;
}

inline bool SetAnnotations(const data::Annotation*rows,size_t count,uint32_t revision){
    if(!original_players::Thread()||!revision||count>state.annotations.size()||(count&&!rows))return false;
    for(size_t n=0;n<count;++n){const auto&r=rows[n];
        if(!r.id||!std::memchr(r.mapId,0,sizeof(r.mapId))||!training_maps::Find(r.mapId)
           ||!core::Finite(r.start)||!core::Finite(r.open)||!core::Finite(r.retreat)||!core::Finite(r.focus)
           ||!std::isfinite(r.authoredConfidence)||r.authoredConfidence<0||r.authoredConfidence>1
           ||!std::isfinite(r.authoredRisk)||r.authoredRisk<0||r.authoredRisk>1
           ||static_cast<unsigned>(r.kind)>static_cast<unsigned>(data::Kind::Hold)
           ||static_cast<unsigned>(r.range)>static_cast<unsigned>(dto::HoldRange::Far)
           ||!std::isfinite(r.authoredFlankRisk)||r.authoredFlankRisk<0||r.authoredFlankRisk>1
           ||r.authoredExposedAngles>dto::MaxAngles
           ||((r.kind==data::Kind::Cover||r.kind==data::Kind::Hold)&&!r.focusKnown)
           ||(r.kind==data::Kind::Hold&&r.enabled&&(!r.exposedAnglesKnown||!r.authoredExposedAngles
              ||core::Length(core::Sub(r.open,r.start))<=0||core::Length(core::Sub(r.focus,r.open))<=0)))return false;
        for(size_t j=0;j<n;++j)if(!std::strcmp(rows[j].mapId,r.mapId)
           &&(rows[j].id==r.id||(r.kind==data::Kind::Hold&&rows[j].kind==data::Kind::Hold
              &&humanlike_bot_hold_positions::SamePoint(rows[j].open,r.open))))return false;
    }
    for(size_t n=0;n<count;++n)state.annotations[n]=rows[n];state.count=count;state.revision=revision;
    for(auto&cache:caches){cache.annotationId=0;cache.routeSegments=0;cache.coverLOS=false;cache.proofs={};cache.localCover={};
        if(cache.navigationEstimate.purpose==dto::NavigationPurpose::MapHold){cache.navigationEstimate={};
            cache.haveGoal=cache.pathValid=cache.estimatedGoalReached=cache.estimatedGoalRejected=false;cache.pathCount=cache.pathAt=0;progress::Pause(cache.progress);}}
    return true;
}
inline bool Initialize(uint64_t seed){
    if(!sensors::Context())return false;state={};state.initialized=true;state.seed=seed;
    for(auto&cache:caches)cache={};return SetAnnotations(data::Authored.data(),data::Authored.size(),data::AuthoredRevision);
}
inline bool LocalSource(size_t index,const dto::OwnLife&life);
inline void ClearAim(Cache&cache){cache.aimEstimate={};cache.aimKnown=cache.aimClear=false;cache.aimEye={};cache.aimAt=-1;}
inline void ClearAimRequests(){for(auto&cache:caches)ClearAim(cache);}
inline void RetireAim(size_t index,uint32_t epoch){if(index&&index<MaxPlayers&&caches[index].life.spawnEpoch==epoch)ClearAim(caches[index]);}
inline bool AimSourceKnown(const dto::EstimatedAim&request){
    if(request.source==dto::EstimatedAimSource::Heard)return true;
    if(request.mapRevision!=state.revision)return false;const auto map=local_server_match::Configuration().mapId;
    for(size_t n=0;n<state.count;++n){const auto&r=state.annotations[n];
        const auto expected=request.source==dto::EstimatedAimSource::AuthoredAngle?(r.id|0x80000000U):r.id;
        if(r.enabled&&r.focusKnown&&expected==request.mapAnnotationId&&!std::strcmp(r.mapId,map)
           &&request.confidence<=r.authoredConfidence)return true;
    }return false;
}
// This queues an own-angle proof only after the existing input Submit succeeds.
// It cannot submit input, create a Track, identify a sound source or grant fire.
inline void AimSubmitted(size_t index,double now,const dto::OwnSnapshot*own,const dto::EstimatedAim*request){
    if(!index||index>=MaxPlayers)return;auto&cache=caches[index];namespace pre=humanlike_bot_prefire;
    if(!Context()||!own||own->nowSeconds!=now||!SameLife(cache.life,own->life)||!LocalSource(index,own->life)
       ||!request||!pre::Valid(*request,*own,now)||!AimSourceKnown(*request)){ClearAim(cache);return;}
    if(pre::Same(cache.aimEstimate,*request))return;
    if(cache.aimEstimate.requested&&cache.aimEstimate.key==request->key){ClearAim(cache);return;}
    ClearAim(cache);cache.aimEstimate=*request;
}
inline void AimProof(size_t index,double now,Cache&cache,const dto::OwnSnapshot&own){
    if(!humanlike_bot_prefire::Valid(cache.aimEstimate,own,now)||!AimSourceKnown(cache.aimEstimate)||!LocalSource(index,own.life)){ClearAim(cache);return;}
    if(!Spend(0,1))return;
    const auto life=own.life;const auto ray=sensors::WorldTrace(static_cast<uint32_t>(index),own.eye,cache.aimEstimate.point,nullptr);
    ++state.sightProbes;
    if(!LocalSource(index,life)){ClearAim(cache);return;}
    cache.aimKnown=ray!=core::Ray::Unknown;cache.aimClear=ray==core::Ray::Clear;
    cache.aimEye=own.eye;cache.aimAt=now;
}
struct PathAttempt {bool queryExecuted=false,budgetDeferred=false,geometryBlocked=false,geometryUnknown=false;};
inline bool NativeFlatEdge(const Cache& cache,size_t node){
    if(!node||node>=cache.pathCount)return false;
    const size_t previous=node-1;
    // Numerical tolerance for the native points of one level. Actual stair
    // rises retain the original navigation path and movement semantics.
    const auto prior=node==1&&cache.admission.projectedGroundKnown
        &&cache.admission.generation==cache.pathGeneration?cache.admission.rawProjection:cache.path[previous];
    return std::abs(cache.path[node].z-prior.z)<=.01;
}
inline engine::Vec MovementPrefix(engine::Vec feet,engine::Vec destination,double distance){
    const double dx=destination.x-feet.x,dy=destination.y-feet.y,length=std::sqrt(dx*dx+dy*dy);
    if(length>0){const double fraction=(std::min)(1.,distance/length);feet.x+=dx*fraction;feet.y+=dy*fraction;}
    return feet;
}
inline bool SideObstacle(const humanlike_capsule_profile_comparison::Comparison& collision){
    return collision.sweepKnown&&!collision.sweepClear&&collision.hitKnown&&collision.hitBlocking
        &&original_players::Finite(collision.hitNormal)&&std::abs(collision.hitNormal.z)<=.01
        &&training_bot_controller::Length2D(collision.hitNormal)>.99;
}
inline bool NativeWallSlide(const humanlike_capsule_profile_comparison::Comparison& collision,
                            const engine::Vec& from,const engine::Vec& to,const dto::OwnSnapshot& own,double arrival){
    if(!own.groundedKnown||!own.grounded||!own.movementValid||!SideObstacle(collision))return false;
    const auto delta=training_bot_controller::Sub(to,from);
    const double into=delta.x*collision.hitNormal.x+delta.y*collision.hitNormal.y;
    const double tangent=std::hypot(delta.x-into*collision.hitNormal.x,delta.y-into*collision.hitNormal.y);
    // A navmesh corner may graze the capsule's side by less than one native
    // movement step. Original movement performs the collision and wall slide.
    return std::abs(delta.z)<=.01&&into<0&&-into<=own.nativeMaxSpeedCmS*local_server_npc_input::Step
        &&tangent>arrival;
}
inline void RememberBlockedBoundary(Cache& cache){
    const auto& a=cache.admission;
    if(!SideObstacle(a.collision)||a.radius<=0||a.step<=0||!original_players::Finite(a.sweepFrom))return;
    const auto normal=a.collision.hitNormal;
    const bool sameBoundary=cache.blockedBoundaryKnown
        &&training_bot_controller::Distance(cache.blockedBoundaryFeet,a.sweepFrom)<=2*a.radius
        &&cache.blockedBoundaryNormal.x*normal.x+cache.blockedBoundaryNormal.y*normal.y>.99;
    if(!sameBoundary)cache.recoveryAnchorsTried.fill(false);
    if(cache.goalId&&cache.goalId<=cache.recoveryAnchorsTried.size())cache.recoveryAnchorsTried[cache.goalId-1]=true;
    if(sameBoundary)return;
    cache.blockedBoundaryKnown=true;cache.blockedBoundaryFeet=a.sweepFrom;
    cache.blockedBoundaryNormal=a.collision.hitNormal;
    cache.blockedBoundaryRadius=a.radius;cache.blockedBoundaryStep=a.step;
}
inline size_t RecoveryAnchor(Cache& cache,const dto::OwnSnapshot& own,const cfg::Profile& profile,size_t count){
    if(!cache.blockedBoundaryKnown)return count;
    const double arrival=SpaceValue(profile,cfg::Param::MoveArrivalCm);
    const double localDistance=2*cache.blockedBoundaryRadius+SpaceValue(profile,cfg::Param::SearchStepCm)+arrival;
    if(training_bot_controller::Distance(cache.blockedBoundaryFeet,sensors::Native(own.feet))>localDistance){
        cache.blockedBoundaryKnown=false;return count;
    }
    size_t best=count;double nearest=INFINITY;
    for(size_t n=0;n<count;++n){
        const auto& point=training_bot_controller::anchors[n];
        if(cache.recoveryAnchorsTried[n]||n+1==cache.failedGoalId||ReachedPathPoint(sensors::Native(own.feet),point,arrival)
           ||std::abs(point.z-own.feet.z)>cache.blockedBoundaryStep)continue;
        const auto delta=training_bot_controller::Sub(point,cache.blockedBoundaryFeet);
        if(delta.x*cache.blockedBoundaryNormal.x+delta.y*cache.blockedBoundaryNormal.y<0)continue;
        const double distance=training_bot_controller::Distance(sensors::Native(own.feet),point);
        if(distance<nearest){best=n;nearest=distance;}
    }
    if(best<count)cache.recoveryAnchorsTried[best]=true;
    else cache.blockedBoundaryKnown=false;
    return best;
}
inline void RememberStalledApproach(Cache& cache,const engine::Vec& feet,const engine::Vec& next,double radius,double step){
    const auto delta=training_bot_controller::Sub(feet,next);
    const double distance=training_bot_controller::Length2D(delta);
    if(!original_players::Finite(delta)||!std::isfinite(distance)||distance<=0||radius<=0||step<=0)return;
    const engine::Vec away{delta.x/distance,delta.y/distance,0};
    const bool same=cache.blockedBoundaryKnown&&training_bot_controller::Distance(cache.blockedBoundaryFeet,feet)<=2*radius
        &&cache.blockedBoundaryNormal.x*away.x+cache.blockedBoundaryNormal.y*away.y>.99;
    if(!same){cache.recoveryAnchorsTried.fill(false);cache.blockedBoundaryFeet=feet;cache.blockedBoundaryNormal=away;}
    cache.blockedBoundaryKnown=true;cache.blockedBoundaryRadius=radius;cache.blockedBoundaryStep=step;
    if(cache.goalId&&cache.goalId<=cache.recoveryAnchorsTried.size())cache.recoveryAnchorsTried[cache.goalId-1]=true;
}
inline bool RevisitsFailedApproach(const Cache& cache,const dto::OwnSnapshot& own,const cfg::Profile& profile){
    if(!cache.blockedBoundaryKnown||cache.pathAt>=cache.pathCount)return false;
    const double arrival=SpaceValue(profile,cfg::Param::MoveArrivalCm);
    const double localDistance=2*cache.blockedBoundaryRadius+SpaceValue(profile,cfg::Param::SearchStepCm)+arrival;
    if(training_bot_controller::Distance(cache.blockedBoundaryFeet,sensors::Native(own.feet))>localDistance)return false;
    const auto offset=training_bot_controller::Sub(cache.path[cache.pathAt],cache.blockedBoundaryFeet);
    return offset.x*cache.blockedBoundaryNormal.x+offset.y*cache.blockedBoundaryNormal.y < -arrival;
}
inline void RejectBlockedPath(Cache& cache){
    RememberBlockedBoundary(cache);cache.slopeProbePending=false;
    cache.failedGoalId=cache.goalId;cache.haveGoal=cache.pathValid=false;
    cache.pathCount=cache.pathAt=0;cache.admission.pending=false;cache.pathQueryFailures=0;cache.nextPlan=0;
    if(cache.navigationEstimate.requested)cache.estimatedGoalRejected=true;
    ++cache.recoveryGoalChanges;progress::Pause(cache.progress);
}
inline void PrepareAdmissionCandidate(Cache& cache,size_t node,const cfg::Profile& profile){
    auto&a=cache.admission;a.node=node;a.sweepDone=false;a.projectedGroundKnown=false;a.groundCursor=0;a.previousGround=a.origin;a.collision={};
    a.fullConnection=node==0||std::abs(cache.path[0].z-a.origin.z)>a.step||std::abs(cache.path[node].z-a.origin.z)>a.step;
    a.end=a.fullConnection?cache.path[node]:MovementPrefix(a.origin,cache.path[node],SpaceValue(profile,cfg::Param::SearchStepCm));
    a.groundRequired=!a.projectedArrived||a.fullConnection;
    a.groundSamples=GroundCost(a.origin,a.end,a.radius)-1;
    a.stage=node?"native_next_point_pending_actual_capsule":"native_projected_start_pending_actual_capsule";
}
inline GeometryResult AdvanceAdmission(size_t index,Cache& cache,const dto::OwnSnapshot& own,const cfg::Profile& profile){
    auto&a=cache.admission;
    if(!a.pending||!LocalSource(index,own.life)||!SameLife(a.life,own.life)
       ||a.generation!=cache.pathGeneration||a.goal!=cache.goalId||cache.pathCount<2||cache.pathCount>MaxPath)
        return GeometryResult::Unknown;
    for(unsigned candidate=0;candidate<2;++candidate){
        if(a.node==0&&!a.projectedGroundKnown){
            if(!Spend(1,0)){a.result=GeometryResult::Deferred;return a.result;}
            bool known=false;engine::Vec floor{};
            const bool supported=training_world_geometry::Ground(a.rawProjection,floor,a.step+10,a.step+10,&known);
            if(!LocalSource(index,a.life)||!known){a.result=GeometryResult::Unknown;return a.result;}
            if(!supported||std::abs(floor.z-a.rawProjection.z)>a.step){
                a.pending=false;a.result=GeometryResult::Blocked;a.stage="native_projected_start_support_unreachable";return a.result;
            }
            // Native navmesh surface heights can sit above their real support.
            // Only node0 is adjusted, after the original ground query proves it.
            a.projectedGround=floor;a.projectedGroundKnown=true;a.end=floor;
            a.groundSamples=GroundCost(a.origin,a.end,a.radius)-1;
            const auto supportOffset=training_bot_controller::Sub(floor,sensors::Native(own.feet));
            if(own.groundedKnown&&own.grounded&&!NativeFlatEdge(cache,1)
               &&training_bot_controller::Length2D(supportOffset)<=(std::max)(static_cast<double>(SpaceValue(profile,cfg::Param::MoveArrivalCm)),a.radius)
               &&std::abs(supportOffset.z)<=a.step){
                // Contact with a slope lifts the capsule base above the floor
                // under its center. The proved nearby support connects the
                // original slope route without a downward movement request.
                a.projectedArrived=true;a.pending=false;a.result=GeometryResult::Clear;
                a.stage="original_native_slope_ground_connection";cache.pathValid=true;cache.pathAt=1;
                return a.result;
            }
        }
        if(!a.sweepDone){
            a.collision={};
            a.sweepFrom=a.origin;a.sweepTo=a.end;
            a.result=MovementSweep(index,a.origin,a.end,a.radius,a.half,&a.collision);a.at=own.nowSeconds;
            if(a.result==GeometryResult::Deferred||a.result==GeometryResult::Unknown)return a.result;
            if(a.projectedArrived&&!a.fullConnection&&NativeWallSlide(a.collision,a.origin,a.end,own,SpaceValue(profile,cfg::Param::MoveArrivalCm)))
                a.result=GeometryResult::Clear;
            if(a.result==GeometryResult::Clear)a.sweepDone=true;
        }
        if(a.sweepDone&&a.groundRequired){
            while(a.groundCursor<=a.groundSamples){
                if(!Spend(1,0)){a.result=GeometryResult::Deferred;return a.result;}
                const double fraction=static_cast<double>(a.groundCursor)/a.groundSamples;
                const engine::Vec expected{a.origin.x+(a.end.x-a.origin.x)*fraction,
                    a.origin.y+(a.end.y-a.origin.y)*fraction,a.origin.z+(a.end.z-a.origin.z)*fraction};
                engine::Vec floor{};bool known=false;
                const bool supported=training_world_geometry::Ground(expected,floor,a.step+10,a.step+10,&known);
                if(!LocalSource(index,a.life)||!known){a.result=GeometryResult::Unknown;return a.result;}
                if(!supported||std::abs(floor.z-expected.z)>a.step||std::abs(floor.z-a.previousGround.z)>a.step){
                    a.result=GeometryResult::Blocked;a.stage="native_connection_support_rejected";break;
                }
                a.previousGround=floor;++a.groundCursor;
            }
        }
        if(a.sweepDone&&(!a.groundRequired||a.groundCursor>a.groundSamples)){
            a.result=GeometryResult::Clear;
            const auto current=sensors::Native(own.feet);
            if(training_bot_controller::Distance(a.origin,current)>.01){
                a.sweepFrom=current;a.sweepTo=a.fullConnection?(a.node==0&&a.projectedGroundKnown?a.projectedGround:cache.path[a.node]):MovementPrefix(current,cache.path[a.node],SpaceValue(profile,cfg::Param::SearchStepCm));
                a.collision={};a.result=MovementSweep(index,a.sweepFrom,a.sweepTo,a.radius,a.half,&a.collision);
                if(a.result==GeometryResult::Deferred||a.result==GeometryResult::Unknown)return a.result;
                if(a.result==GeometryResult::Clear&&a.groundRequired){
                    if(!Spend(1,0)){a.result=GeometryResult::Deferred;return a.result;}
                    engine::Vec floor{};bool known=false;
                    const bool supported=training_world_geometry::Ground(a.sweepTo,floor,a.step+10,a.step+10,&known);
                    if(!LocalSource(index,a.life)||!known){a.result=GeometryResult::Unknown;return a.result;}
                    if(!supported||std::abs(floor.z-a.sweepTo.z)>a.step||std::abs(floor.z-a.previousGround.z)>a.step)
                        a.result=GeometryResult::Blocked;
                }
            }
            if(a.result==GeometryResult::Clear){
                // Direct entry to node1 has already failed before this node0
                // fallback. A horizontally reached projection supplies no
                // remaining movement, even when its raw navmesh Z floats
                // above the original support sample.
                const auto arrival=SpaceValue(profile,cfg::Param::MoveArrivalCm);
                if(a.node==0&&training_bot_controller::Length2D(training_bot_controller::Sub(cache.path[0],current))<=arrival){
                    a.pending=false;a.result=GeometryResult::Blocked;
                    a.stage=ReachedPathPoint(current,a.previousGround,arrival)
                        ?"native_projected_start_support_already_reached":"native_projected_start_vertical_connection_unreachable";
                    return a.result;
                }
                a.pending=false;a.stage=a.node?"native_next_point_admitted":"native_projected_start_admitted";
                if(a.node==0&&a.projectedGroundKnown)cache.path[0]=a.projectedGround;
                cache.pathValid=true;cache.pathAt=a.node;cache.movementCheckedAt=own.nowSeconds;cache.movementCheckedNode=a.node;
                return a.result;
            }
            a.sweepDone=false;a.origin=current;
        }
        // A projected start can sit behind a blocker while the next original
        // point leads away. Prefer the actual direct entry above. If that is
        // blocked, retain node0 only after its own physical connection passes.
        if(a.node==1&&!a.projectedArrived){RememberBlockedBoundary(cache);PrepareAdmissionCandidate(cache,0,profile);continue;}
        a.pending=false;a.result=GeometryResult::Blocked;a.stage="native_path_entry_component_blocked";return a.result;
    }
    a.result=GeometryResult::Deferred;return a.result;
}
inline bool AdmissionResult(Cache& cache,GeometryResult result,PathAttempt* attempt){
    if(attempt){attempt->queryExecuted=true;attempt->budgetDeferred=result==GeometryResult::Deferred;
        attempt->geometryUnknown=result==GeometryResult::Unknown;attempt->geometryBlocked=result==GeometryResult::Blocked;}
    if(result==GeometryResult::Blocked)++state.pathFailures;
    return result==GeometryResult::Clear&&cache.pathValid;
}
inline bool Path(size_t index,Cache&cache,const dto::OwnSnapshot&own,const cfg::Profile&profile,PathAttempt*attempt=nullptr){
    if(attempt)*attempt={};
    if(!index||index>=MaxPlayers){if(attempt)attempt->geometryUnknown=true;return false;}
    const auto*p=local_server_match::Player(index);const auto&o=training_bot_controller::actors[index].observed;
    const auto start=sensors::Native(own.feet);const double arrival=SpaceValue(profile,cfg::Param::MoveArrivalCm);
    auto&a=cache.admission;
    if(a.pending&&SameLife(a.life,own.life)&&a.generation==cache.pathGeneration&&a.goal==cache.goalId
       &&cache.pathCount>=2&&cache.pathCount<=MaxPath&&a.node<cache.pathCount
       &&a.destination.x==cache.goal.x&&a.destination.y==cache.goal.y&&a.destination.z==cache.goal.z
       &&a.radius==o.capsuleRadius&&a.half==o.capsuleHalfHeight
       &&training_bot_controller::Distance(a.origin,start)<=arrival){
        return AdmissionResult(cache,AdvanceAdmission(index,cache,own,profile),attempt);
    }
    a.pending=false;
    if(!p||!LocalSource(index,own.life)){if(attempt)attempt->geometryUnknown=true;return false;}
    if(!Spend(0,1)){if(attempt)attempt->budgetDeferred=true;return false;}
    ++state.paths;++cache.pathGeneration;cache.pathValid=false;cache.pathCount=cache.pathAt=0;
    struct Query {void* context;engine::Vec start,end;void*pawn;void*filter=nullptr;void*result=nullptr;} query{sensors::GetState().world,start,cache.goal,p->pawn};
    static_assert(sizeof(Query)==80&&offsetof(Query,result)==72);
    const bool queryExecuted=training_bot_controller::Event(engine::DefaultObject(engine::Class(0x4f7fff0)),L"/Script/NavigationSystem.NavigationSystemV1.FindPathToLocationSynchronously",80,&query);
    if(attempt)attempt->queryExecuted=queryExecuted;
    if(queryExecuted&&original_players::IsA(query.result,0x4f80dd0)){
        uint8_t valid=0,partial=1;const auto points=Read<engine::Vec*>(query.result,0x40);
        const auto count=Read<int32_t>(query.result,0x48),capacity=Read<int32_t>(query.result,0x4c);
        if(training_bot_controller::Event(query.result,L"/Script/NavigationSystem.NavigationPath.IsValid",1,&valid)
           &&training_bot_controller::Event(query.result,L"/Script/NavigationSystem.NavigationPath.IsPartial",1,&partial)&&valid&&!partial
           &&count>=2&&count<=static_cast<int32_t>(MaxPath)&&capacity>=count&&original_players::Readable(points,static_cast<size_t>(count)*sizeof(engine::Vec))
           &&training_bot_controller::Distance(points[count-1],cache.goal)<=arrival){
            bool finite=true;for(int32_t n=0;n<count;++n){cache.path[static_cast<size_t>(n)]=points[n];finite=finite&&original_players::Finite(points[n]);}
            if(finite&&LocalSource(index,own.life)){
                cache.pathCount=static_cast<size_t>(count);cache.pathAt=1;
                a={};a.life=own.life;a.generation=cache.pathGeneration;a.goal=cache.goalId;a.origin=start;a.destination=cache.goal;a.radius=o.capsuleRadius;a.half=o.capsuleHalfHeight;
                if(!training_world_geometry::NativeStepHeight(index,a.step)){if(attempt)attempt->geometryUnknown=true;return false;}
                a.rawProjection=cache.path[0];a.projectedArrived=ReachedPathPoint(start,cache.path[0],arrival);
                // A grounded capsule can contact a slope above the navmesh
                // surface directly below its center. Use the native step range
                // for this initial mesh association only. Subsequent node and
                // destination arrival still require the existing full test.
                const auto projectionOffset=training_bot_controller::Sub(cache.path[0],start);
                if(own.groundedKnown&&own.grounded&&training_bot_controller::Length2D(projectionOffset)<=(std::max)(arrival,a.radius)
                   &&std::abs(projectionOffset.z)<=a.step)a.projectedArrived=true;
                if(RevisitsFailedApproach(cache,own,profile)){
                    a.result=GeometryResult::Blocked;a.stage="native_route_repeats_failed_local_approach";
                    return AdmissionResult(cache,a.result,attempt);
                }
                if(a.projectedArrived&&!NativeFlatEdge(cache,1)){
                    cache.pathValid=true;a.result=GeometryResult::Clear;a.at=own.nowSeconds;
                    a.stage="original_native_slope_path_retained";return true;
                }
                a.pending=true;PrepareAdmissionCandidate(cache,1,profile);
                return AdmissionResult(cache,AdvanceAdmission(index,cache,own,profile),attempt);
            }
        }
    }
    const uint32_t cost=GroundCost(start,cache.goal,o.capsuleRadius);
    const bool fallbackAffordable=cost<=state.groundBudget&&state.queryBudget>0;
    GeometryResult fallbackResult=GeometryResult::Deferred;
    if(fallbackAffordable&&Segment(index,start,cache.goal,o.capsuleRadius,o.capsuleHalfHeight,&fallbackResult)){
        cache.pathValid=true;cache.path[0]=cache.goal;cache.pathCount=1;cache.pathAt=0;++state.localPaths;return true;
    }
    if(attempt&&fallbackAffordable&&fallbackResult==GeometryResult::Unknown)attempt->geometryUnknown=true;
    if(attempt&&!fallbackAffordable){
        const auto*global=sensors::GetState().readProfile?sensors::GetState().readProfile(0):nullptr;
        const double ground=global?SpaceValue(*global,cfg::Param::SpaceGroundQueryBudget):-1;
        const double queries=global?SpaceValue(*global,cfg::Param::SpaceCoverQueryBudget):-1;
        attempt->budgetDeferred=!std::isfinite(ground)||!std::isfinite(queries)||ground<0||queries<0||(cost<=ground&&queries>=2);
    }
    ++state.pathFailures;return false;
}
// Look ahead along the requested movement, then test only the first side
// contact at native step height. A one-physics-step lookahead can end before
// a nearby blocking surface; a long raised sweep can instead hit a later
// stair riser. Neither query moves the character or replaces native StepUp.
inline void CheckRisingMovement(size_t index,double now,Cache&cache,const dto::OwnSnapshot&own,const cfg::Profile&profile){
    const auto& observed=training_bot_controller::actors[index].observed;
    double step=0;
    if(!training_world_geometry::NativeStepHeight(index,step)||!own.movementValid
       ||!std::isfinite(own.nativeMaxSpeedCmS)||own.nativeMaxSpeedCmS<=0)return;
    const double radius=observed.capsuleRadius,half=observed.capsuleHalfHeight;
    const auto from=sensors::Native(own.feet);
    const double nativeStep=static_cast<double>(own.nativeMaxSpeedCmS)*local_server_npc_input::Step;
    const double distance=SpaceValue(profile,cfg::Param::SearchStepCm);
    if(!std::isfinite(distance)||distance<=0)return;
    const auto to=MovementPrefix(from,cache.path[cache.pathAt],distance);
    const bool continuing=cache.slopeProbePending&&cache.slopeProbeGeneration==cache.pathGeneration
        &&cache.slopeProbeNode==cache.pathAt&&cache.slopeProbeRadius==radius&&cache.slopeProbeHalf==half
        &&cache.slopeProbeStep==step&&training_bot_controller::Distance(cache.slopeProbeFrom,from)<=.01
        &&now>=cache.slopeProbeAt&&now-cache.slopeProbeAt<=SpaceValue(profile,cfg::Param::OwnSnapshotMaxAgeMs)*.001;
    if(!continuing){
        cache.slopeProbePending=false;
        cache.slopeProbeGeneration=cache.pathGeneration;cache.slopeProbeNode=cache.pathAt;
        cache.slopeProbeFrom=from;cache.slopeProbeTo=to;cache.slopeProbeRadius=radius;cache.slopeProbeHalf=half;
        cache.slopeProbeStep=step;cache.slopeProbeAt=now;cache.slopeProbeRaisedTo={};
        cache.slopeProbeCollision={};cache.slopeProbeRaisedCollision={};cache.slopeProbeRaisedResult=GeometryResult::Unknown;
        cache.slopeProbeSupportKnown=cache.slopeProbeSupportPresent=false;cache.slopeProbeSupport={};
        cache.slopeProbeStartSupportKnown=cache.slopeProbeStartSupportPresent=false;cache.slopeProbeStartSupport={};
        humanlike_capsule_profile_comparison::Comparison collision{};
        const auto result=MovementSweep(index,from,to,radius,half,&collision);
        cache.slopeProbeResult=result;cache.slopeProbeCollision=collision;
        if(result!=GeometryResult::Blocked||!SideObstacle(collision))return;
        if(NativeWallSlide(collision,from,to,own,SpaceValue(profile,cfg::Param::MoveArrivalCm)))return;
        const double length=training_bot_controller::Distance(from,to);
        if(!std::isfinite(collision.hitDistance)||collision.hitDistance<0||collision.hitDistance>length)return;
        cache.slopeProbeRaisedTo=MovementPrefix(from,to,(std::min)(length,collision.hitDistance+nativeStep));
        cache.slopeProbePending=true;
    }
    if(cache.slopeProbeRaisedResult!=GeometryResult::Blocked){
        auto raisedFrom=cache.slopeProbeFrom,raisedTo=cache.slopeProbeRaisedTo;raisedFrom.z+=step;raisedTo.z+=step;
        humanlike_capsule_profile_comparison::Comparison raised{};
        const auto result=MovementSweep(index,raisedFrom,raisedTo,radius,half,&raised);
        cache.slopeProbeRaisedResult=result;cache.slopeProbeRaisedCollision=raised;
        if(result==GeometryResult::Deferred)return;
    }
    const auto&raised=cache.slopeProbeRaisedCollision;
    if(cache.slopeProbeRaisedResult!=GeometryResult::Blocked||!SideObstacle(raised)
       ||raised.hitNormal.x*cache.slopeProbeCollision.hitNormal.x+raised.hitNormal.y*cache.slopeProbeCollision.hitNormal.y<=.99){
        cache.slopeProbePending=false;return;
    }
    {
        // A native descent may go below a farther wall. Compare original
        // ground at the current feet and first contact; even a small drop
        // can clear an overhead obstacle, so leave that descent to movement.
        if(!cache.slopeProbeStartSupportKnown){
            if(!Spend(1,0))return;
            bool known=false;engine::Vec floor{};
            const bool supported=training_world_geometry::Ground(from,floor,step+10,step+10,&known);
            cache.slopeProbeStartSupportKnown=known&&LocalSource(index,own.life);
            cache.slopeProbeStartSupportPresent=cache.slopeProbeStartSupportKnown&&supported;
            cache.slopeProbeStartSupport=floor;
            if(!cache.slopeProbeStartSupportPresent||std::abs(floor.z-from.z)>step){cache.slopeProbePending=false;return;}
        }
        if(!Spend(1,0))return;
        const auto contact=MovementPrefix(from,to,cache.slopeProbeCollision.hitDistance);
        bool known=false;engine::Vec floor{};
        const bool supported=training_world_geometry::Ground(contact,floor,step+10,step+10,&known);
        cache.slopeProbeSupportKnown=known&&LocalSource(index,own.life);
        cache.slopeProbeSupportPresent=cache.slopeProbeSupportKnown&&supported;
        cache.slopeProbeSupport=floor;
        if(!cache.slopeProbeSupportPresent||std::abs(floor.z-from.z)>step
           ||floor.z<cache.slopeProbeStartSupport.z-.01){cache.slopeProbePending=false;return;}
    }
    cache.slopeProbePending=false;
    auto& admission=cache.admission;admission.origin=admission.sweepFrom=from;
    admission.end=admission.sweepTo=to;admission.radius=radius;admission.half=half;admission.step=step;
    admission.at=now;admission.node=cache.pathAt;admission.collision=cache.slopeProbeCollision;
    admission.result=GeometryResult::Blocked;admission.stage="native_next_step_side_wall_exceeds_step_height";
    RejectBlockedPath(cache);
}
inline void CheckCurrentMovement(size_t index,double now,Cache&cache,const dto::OwnSnapshot&own,const cfg::Profile&profile){
    if(!cache.pathValid||!cache.haveGoal||cache.pathAt>=cache.pathCount
       ||!own.groundedKnown||!own.grounded||!LocalSource(index,own.life)
       ||(cache.movementCheckedAt==now&&cache.movementCheckedNode==cache.pathAt))return;
    if(RevisitsFailedApproach(cache,own,profile)){
        cache.admission.collision={};cache.admission.result=GeometryResult::Blocked;
        cache.admission.stage="native_route_repeats_failed_local_approach";RejectBlockedPath(cache);return;
    }
    if(!NativeFlatEdge(cache,cache.pathAt)){
        CheckRisingMovement(index,now,cache,own,profile);return;
    }
    cache.slopeProbePending=false;
    if(cache.slopeTransitionPending&&cache.slopeTransitionGeneration==cache.pathGeneration&&cache.slopeTransitionNode==cache.pathAt){
        const auto*npc=local_server_npc_input::Player(index);
        if(npc&&npc->epoch==own.life.spawnEpoch
           &&(npc->physicsCompleted>cache.slopeTransitionPhysics||npc->nativeAdvances>cache.slopeTransitionNative)
           &&cache.slopeTransitionOpportunityAt<0)cache.slopeTransitionOpportunityAt=now;
        if(cache.slopeTransitionOpportunityAt<0||own.sampledAtSeconds<cache.slopeTransitionOpportunityAt)return;
        cache.slopeTransitionPending=false;
    }
    double step=0;if(!training_world_geometry::NativeStepHeight(index,step))return;
    const auto feet=sensors::Native(own.feet);
    if(std::abs(cache.path[cache.pathAt].z-feet.z)>step){
        cache.pathValid=false;cache.admission.pending=false;cache.nextPlan=now;
        cache.admission.stage="actual_floor_changed_original_path_required";return;
    }
    auto&a=cache.admission;a.origin=feet;a.end=MovementPrefix(feet,cache.path[cache.pathAt],SpaceValue(profile,cfg::Param::SearchStepCm));
    a.sweepFrom=a.origin;a.sweepTo=a.end;
    a.node=cache.pathAt;a.at=now;a.collision={};
    const auto&o=training_bot_controller::actors[index].observed;
    a.radius=o.capsuleRadius;a.half=o.capsuleHalfHeight;a.step=step;
    a.result=MovementSweep(index,a.origin,a.end,o.capsuleRadius,o.capsuleHalfHeight,&a.collision);
    a.stage="current_native_path_step_checked";
    if(a.result==GeometryResult::Blocked){
        if(NativeWallSlide(a.collision,a.origin,a.end,own,SpaceValue(profile,cfg::Param::MoveArrivalCm)))
            a.stage="native_tangential_wall_slide_retained";
        else{a.stage="current_native_path_step_component_blocked";RejectBlockedPath(cache);}
    }
    if(a.result==GeometryResult::Clear){cache.movementCheckedAt=now;cache.movementCheckedNode=cache.pathAt;}
}

inline void ClearEstimatedGoal(Cache&cache,double now){
    if(!cache.navigationEstimate.requested)return;
    cache.navigationEstimate={};cache.estimatedGoalReached=cache.estimatedGoalRejected=false;
    cache.proofs.holdProofReady=false;
    cache.haveGoal=cache.pathValid=false;cache.pathCount=cache.pathAt=0;
    cache.recoveryRetried=false;cache.pathQueryFailures=0;cache.failedGoalId=0;cache.nextPlan=std::isfinite(now)?now:0;
    progress::Pause(cache.progress);
}
inline const data::Annotation* HoldAnnotation(const dto::EstimatedNavigation&request){
    if(request.purpose!=dto::NavigationPurpose::MapHold||request.knowledge!=dto::NavigationKnowledge::StaticMap
       ||request.usesHearing||request.mapRevision!=state.revision)return nullptr;
    const auto map=local_server_match::Configuration().mapId;
    for(size_t n=0;n<state.count;++n){const auto&r=state.annotations[n];
        if(r.enabled&&r.kind==data::Kind::Hold&&r.focusKnown&&r.exposedAnglesKnown&&r.id==request.mapAnnotationId
           &&!std::strcmp(r.mapId,map)&&request.confidence==r.authoredConfidence
           &&humanlike_bot_hold_positions::SamePoint(r.open,request.estimatedGoal))return &r;
    }return nullptr;
}
inline void AcceptEstimatedGoal(Cache&cache,const dto::EstimatedNavigation*request,const dto::OwnSnapshot&own,double now){
    namespace tactic=humanlike_bot_tactics_navigation;
    if(!request||!tactic::ValidRequest(*request,own,now)){ClearEstimatedGoal(cache,now);return;}
    if(request->purpose==dto::NavigationPurpose::MapHold&&!HoldAnnotation(*request)){ClearEstimatedGoal(cache,now);return;}
    if(tactic::SameRequest(cache.navigationEstimate,*request))return;
    if(tactic::SoundLeaseExtension(cache.navigationEstimate,*request)){
        cache.navigationEstimate.expiresAtSeconds=request->expiresAtSeconds;return;
    }
    // A key binds an immutable frozen estimate. A mutation under the same key
    // is invalid, rather than silently moving the destination each own tick.
    if(cache.navigationEstimate.requested&&cache.navigationEstimate.key==request->key){ClearEstimatedGoal(cache,now);return;}
    ClearEstimatedGoal(cache,now);cache.navigationEstimate=*request;
    cache.haveGoal=true;cache.pathValid=false;cache.pathCount=cache.pathAt=0;
    cache.goal=sensors::Native(request->estimatedGoal);
    // Internal nonzero route-ID domain, distinct from static anchor IDs.
    cache.goalId=0x40000000U|static_cast<uint32_t>(request->key&0x3fffffffULL);
    cache.recoveryRetried=false;cache.pathQueryFailures=0;cache.failedGoalId=0;cache.nextPlan=now;
    progress::Pause(cache.progress);
}
// Called only after the existing successful native input submission. It never
// submits an input or invokes movement/path physics, and never marks a node reached.
inline void NavigationSubmitted(size_t index,double now,const dto::OwnSnapshot*own,bool demand,const dto::EstimatedNavigation*request=nullptr){
    if(index==0||index>=MaxPlayers)return;auto&cache=caches[index];
    if(!Context()||!own||own->nowSeconds!=now||!SameLife(cache.life,own->life)||!LocalSource(index,own->life)){
        ClearEstimatedGoal(cache,now);progress::Pause(cache.progress);return;
    }
    const auto*profile=sensors::GetState().readProfile?sensors::GetState().readProfile(index):nullptr;
    const auto*p=local_server_match::Player(index);const auto*npc=local_server_npc_input::Player(index);
    if(!profile||!p||!npc||!npc->present||!npc->alive||!npc->ready||npc->error
       ||npc->owner==local_server_npc_input::Owner::Unknown||npc->pawn!=p->pawn||npc->pc!=p->pc||npc->ps!=p->ps
       ||npc->epoch!=p->epoch||npc->pawnId.index!=p->pawnIdentity.index||npc->pawnId.serial!=p->pawnIdentity.serial){
        ClearEstimatedGoal(cache,now);progress::Pause(cache.progress);return;
    }
    AcceptEstimatedGoal(cache,request,*own,now);
    // A settled body that drifted past a MapHold may request a current original
    // recovery path to the same immutable key/goal. Coasting alone is no demand.
    if(demand&&request&&request->purpose==dto::NavigationPurpose::MapHold&&cache.estimatedGoalReached
       &&humanlike_bot_tactics_navigation::SameRequest(cache.navigationEstimate,*request)
       &&own->valid&&own->alive&&!own->paused&&!own->retired&&std::isfinite(own->sampledAtSeconds)&&own->sampledAtSeconds<=now
       &&now-own->sampledAtSeconds<=SpaceValue(*profile,cfg::Param::OwnSnapshotMaxAgeMs)*.001
       &&own->movementValid&&core::Finite(own->velocity)&&own->velocity.x==0&&own->velocity.y==0&&own->velocity.z==0
       &&own->groundedKnown&&own->grounded
       &&!humanlike_bot_hold_positions::Arrived(*own,request->estimatedGoal,SpaceValue(*profile,cfg::Param::MoveArrivalCm))){
        cache.estimatedGoalReached=false;cache.haveGoal=true;cache.pathValid=false;cache.pathCount=cache.pathAt=0;
        cache.goal=sensors::Native(request->estimatedGoal);cache.nextPlan=now;progress::Pause(cache.progress);
    }
    progress::Sample sample{};sample.own=own;sample.key={own->life,cache.pathGeneration,cache.goalId,cache.pathAt};
    sample.pathValid=cache.haveGoal&&cache.pathValid&&cache.pathAt<cache.pathCount&&cache.pathCount<=MaxPath;
    if(sample.pathValid)sample.nextNode=sensors::Vec(cache.path[cache.pathAt]);
    sample.navigationSubmitted=demand;sample.nativeOpportunityKnown=true;
    sample.physicsCompleted=npc->physicsCompleted;sample.nativeAdvances=npc->nativeAdvances;
    const auto result=progress::Observe(cache.progress,sample,{SpaceValue(*profile,cfg::Param::MoveArrivalCm),
        SpaceValue(*profile,cfg::Param::MoveStallMs)*.001,SpaceValue(*profile,cfg::Param::OwnSnapshotMaxAgeMs)*.001});
    // A current-goal episode permits one original repath. Normal own progress
    // only resets the helper window: a physical backtrack can return to the
    // same blocked edge, so departure must not erase this retry stage.
    if(!result.stalled)return;
    // Completed native input and fresh own positions established a failed
    // approach even if its static collision query was clear. Retain that
    // local direction across goal changes so recovery can leave the area.
    double recoveryStep=0;
    if(sample.pathValid&&training_world_geometry::NativeStepHeight(index,recoveryStep))
        RememberStalledApproach(cache,sensors::Native(own->feet),cache.path[cache.pathAt],
            training_bot_controller::actors[index].observed.capsuleRadius,recoveryStep);
    // First ask the same original pathfinder again from current real feet.
    // A repeat without own progress abandons this static goal, not a path edge.
    // New pathProven remains false until the original query/strict fallback passes.
    cache.pathValid=false;cache.pathCount=cache.pathAt=0;cache.nextPlan=now;
    if(cache.recoveryRetried){cache.failedGoalId=cache.goalId;cache.haveGoal=false;++cache.recoveryGoalChanges;
        if(cache.navigationEstimate.requested)cache.estimatedGoalRejected=true;}
    else{cache.recoveryRetried=true;++cache.recoveryReplans;}
}
inline const data::Annotation* Closest(const dto::OwnSnapshot&own,const cfg::Profile&profile){
    const auto map=local_server_match::Configuration().mapId;const data::Annotation*best=nullptr;
    float distance=SpaceValue(profile,cfg::Param::CoverMaximumAnnotationDistanceCm);
    for(size_t n=0;n<state.count;++n){const auto&r=state.annotations[n];
        if(!r.enabled||r.kind==data::Kind::Hold||std::strcmp(r.mapId,map)||r.authoredConfidence<SpaceValue(profile,cfg::Param::CoverMinimumConfidence))continue;
        const float d=core::Length(core::Sub(r.start,own.feet));if(d<distance){distance=d;best=&r;}}
    return best;
}
inline bool Clearance(size_t index,const dto::Vec3&feet,const dto::Vec3&direction,float length,float radius,float half,float&distance){
    const auto result=MovementSweep(index,sensors::Native(feet),sensors::Native(core::Add(feet,core::Mul(direction,length))),radius,half);
    if(result==GeometryResult::Unknown||result==GeometryResult::Deferred)return false;
    ++state.clearanceProbes;distance=result==GeometryResult::Clear?length:0;return true;
}
// Only the current NPC's real tuple is read. No opponent position participates
// in proposal construction or geometry proofs, including between native calls.
inline bool LocalSource(size_t index,const dto::OwnLife&life){
    if(!Context()||!index||index>=MaxPlayers||life.stableBotId!=index)return false;
    const auto p=local_server_match::Player(index);if(!p||!p->present||p->human||!p->alive
       ||p->epoch!=life.spawnEpoch||sensors::OwnToken(p->pawnIdentity)!=life.token
       ||!sensors::Live(p->pawn,p->pawnIdentity)||!sensors::Live(p->pc,p->pcIdentity)||!sensors::Live(p->ps,p->psIdentity)
       ||Read<void*>(p->pawn,0x4a0)!=p->pc||Read<void*>(p->pc,0x4b0)!=p->pawn||Read<void*>(p->pawn,0x488)!=p->ps)return false;
    const auto&a=training_bot_controller::actors[index];const auto&o=a.observed;
    return a.bound&&a.pawn==p->pawn&&a.epoch==p->epoch&&o.ready&&o.alive&&o.pawn==p->pawn&&o.controller==p->pc&&o.spawnEpoch==p->epoch;
}
struct LocalQueries {
    size_t index=0;uint32_t maximumGround=0,maximumQueries=0;
    float expectedRadius=0,expectedHalf=0;bool crouched=false;
    bool Live(const dto::OwnLife&life)const{
        if(!LocalSource(index,life))return false;const auto&o=training_bot_controller::actors[index].observed;
        return o.capsuleRadius==expectedRadius&&o.capsuleHalfHeight==expectedHalf&&o.crouched==crouched;
    }
    local::Afford SegmentAfford(dto::Vec3 a,dto::Vec3 b,float radius)const{
        const auto ground=GroundCost(sensors::Native(a),sensors::Native(b),radius);
        if(ground>maximumGround||!maximumQueries)return local::Afford::Impossible;
        if(ground>state.groundBudget||!state.queryBudget){++state.budgetDeferred;return local::Afford::Deferred;}
        return local::Afford::Ready;
    }
    bool Segment(dto::Vec3 a,dto::Vec3 b,float radius,float half)const{
        const bool okay=local_server_humanlike_space::Segment(index,sensors::Native(a),sensors::Native(b),radius,half);
        if(okay)++state.coverValidations;return okay;
    }
    local::Afford TraceAfford(uint32_t count)const{
        if(count>maximumQueries)return local::Afford::Impossible;
        if(count>state.queryBudget){++state.budgetDeferred;return local::Afford::Deferred;}
        return local::Afford::Ready;
    }
    local::Ray Trace(dto::Vec3 a,dto::Vec3 b)const{
        if(!Spend(0,1))return local::Ray::Unknown;++state.sightProbes;
        const auto ray=sensors::WorldTrace(static_cast<uint32_t>(index),a,b,nullptr);
        return ray==core::Ray::Clear?local::Ray::Clear:ray==core::Ray::Blocked?local::Ray::Blocked:local::Ray::Unknown;
    }
};
inline void ClearCover(Cache&cache){
    cache.proofs.holdProofReady=false;
    cache.proofs.coverKnown=cache.proofs.peekPathProven=cache.proofs.retreatPathProven=false;
    cache.proofs.landingSupportKnown=cache.proofs.landingProven=cache.proofs.peekSightKnown=false;
    cache.proofs.groundProven=cache.proofs.peekSightClear=false;
    cache.proofs.coverConfidence=cache.proofs.exposureRisk=0;cache.proofs.exposedAngleCount=0;
}
inline void ApplyLocalProof(dto::SpaceHints&out,const dto::SpaceHints&p){
    out.coverKnown=p.coverKnown;out.groundProven=p.groundProven;
    out.peekPathProven=p.peekPathProven;out.retreatPathProven=p.retreatPathProven;
    out.coverStart=p.coverStart;out.peekOpen=p.peekOpen;out.retreatPoint=p.retreatPoint;
    out.jumpLanding=p.jumpLanding;out.landingSupportKnown=p.landingSupportKnown;out.landingProven=false;
    out.peekSightKnown=p.peekSightKnown;out.peekSightClear=p.peekSightClear;
    out.coverConfidence=p.coverConfidence;out.exposureRisk=p.exposureRisk;out.exposedAngleCount=p.exposedAngleCount;
}
inline void LocalProof(size_t index,Cache&cache,const dto::OwnSnapshot&own,const cfg::Profile&profile,float radius,float half){
    ClearCover(cache);const auto*global=sensors::GetState().readProfile(0);if(!global)return;
    // Native navigation/grounded map anchors are static map knowledge. Prefer
    // the next path turn; an opponent position is never offered as a focus.
    bool navKnown=false;dto::Vec3 navFocus{};
    if(cache.pathValid&&cache.pathAt<cache.pathCount){
        const auto selected=(std::min)(cache.pathAt+1,cache.pathCount-1);navFocus=sensors::Vec(cache.path[selected]);
        navFocus.z+=own.eye.z-own.feet.z;navKnown=core::Finite(navFocus);
    }else if(cache.haveGoal&&original_players::Finite(cache.goal)){
        navFocus=sensors::Vec(cache.goal);navFocus.z+=own.eye.z-own.feet.z;navKnown=core::Finite(navFocus);
    }
    LocalQueries queries{index,static_cast<uint32_t>(SpaceValue(*global,cfg::Param::SpaceGroundQueryBudget)),
        static_cast<uint32_t>(SpaceValue(*global,cfg::Param::SpaceCoverQueryBudget)),radius,half,own.crouched};
    const auto before=cache.localCover.proposals,invalid=cache.localCover.invalidations,deferred=cache.localCover.deferrals;
    const auto result=local::Step(cache.localCover,own,local::FromProfile(profile,radius),radius,half,navKnown,navFocus,queries);
    state.localCoverCandidates+=cache.localCover.proposals-before;state.localCoverInvalidated+=cache.localCover.invalidations-invalid;
    state.localCoverDeferred+=cache.localCover.deferrals-deferred;
    if(result==local::Result::Deferred)return;
    if(result==local::Result::Rejected){++state.localCoverRejected;++state.coverFailures;return;}
    if(result!=local::Result::Ready&&result!=local::Result::Retained)return;
    if(result==local::Result::Ready)++state.localCoverReady;
    const auto p=local::Publish(cache.localCover,own);ApplyLocalProof(cache.proofs,p);
    for(size_t n=0;n<p.angleCount&&cache.proofs.angleCount<cache.proofs.angles.size();++n)
        cache.proofs.angles[cache.proofs.angleCount++]=p.angles[n];
}
inline void Proof(size_t index,double now,Cache&cache,const dto::OwnSnapshot&own,const cfg::Profile&profile){
    const auto&o=training_bot_controller::actors[index].observed;
    const float radius=static_cast<float>(o.capsuleRadius),half=static_cast<float>(o.capsuleHalfHeight);
    if(radius<=0||half<radius)return;
    const float yaw=own.view.yaw*core::Pi/180;const dto::Vec3 forward{std::cos(yaw),std::sin(yaw),0},right{-std::sin(yaw),std::cos(yaw),0};
    const float length=SpaceValue(profile,cfg::Param::SearchStepCm);
    if(state.queryBudget>=3){float left=0,rightDistance=0,front=0;
        if(Clearance(index,own.feet,core::Mul(right,-1),length,radius,half,left)
           &&Clearance(index,own.feet,right,length,radius,half,rightDistance)
           &&Clearance(index,own.feet,forward,length,radius,half,front)){
            cache.proofs.clearanceKnown=true;cache.proofs.leftClearanceCm=left;cache.proofs.rightClearanceCm=rightDistance;cache.proofs.forwardClearanceCm=front;
            cache.proofFeet=own.feet;
        }else cache.proofs.clearanceKnown=false;
    }
    const bool mapHold=cache.navigationEstimate.requested&&cache.navigationEstimate.purpose==dto::NavigationPurpose::MapHold;
    const auto row=mapHold?HoldAnnotation(cache.navigationEstimate):Closest(own,profile);
    if(mapHold&&(!row||cache.estimatedGoalRejected)){cache.proofs.holdProofReady=false;ClearCover(cache);return;}
    if(!row){cache.annotationId=0;cache.routeSegments=0;cache.coverLOS=false;cache.proofs.holdProofReady=false;
        LocalProof(index,cache,own,profile,radius,half);return;}
    cache.localCover={}; // Explicit authored rows retain precedence.
    if(cache.annotationId!=row->id||cache.radius!=radius||cache.half!=half){cache.annotationId=row->id;cache.routeSegments=0;cache.coverLOS=false;cache.radius=radius;cache.half=half;
        ClearCover(cache);cache.proofs.holdProofReady=false;}
    const engine::Vec starts[]{sensors::Native(row->start),sensors::Native(row->open),sensors::Native(row->retreat)};
    const engine::Vec ends[]{sensors::Native(row->open),sensors::Native(row->retreat),sensors::Native(row->start)};
    for(uint32_t part=0;part<3;++part)if(!(cache.routeSegments&(1U<<part))){
        const auto cost=GroundCost(starts[part],ends[part],radius);
        if(cost>state.groundBudget||!state.queryBudget){++state.budgetDeferred;break;}
        if(Segment(index,starts[part],ends[part],radius,half)){cache.routeSegments|=static_cast<uint8_t>(1U<<part);++state.coverValidations;}
        else{++state.coverFailures;
            if(mapHold){cache.estimatedGoalRejected=true;cache.haveGoal=cache.pathValid=false;cache.proofs.holdProofReady=false;ClearCover(cache);}break;}
    }
    if(cache.routeSegments!=7)return;
    cache.proofs.coverStart=row->start;cache.proofs.peekOpen=row->open;cache.proofs.retreatPoint=row->retreat;
    cache.proofs.peekPathProven=cache.proofs.retreatPathProven=cache.proofs.groundProven=true;
    cache.proofs.jumpLanding=row->open;cache.proofs.landingSupportKnown=true;cache.proofs.landingProven=false;
    cache.proofs.coverConfidence=row->authoredConfidence;cache.proofs.exposureRisk=row->authoredRisk;
    if(row->focusKnown&&state.queryBudget>=3){
        const float eyeHeight=own.eye.z-own.feet.z;auto cover=row->start,open=row->open;cover.z+=eyeHeight;open.z+=eyeHeight;
        if(Spend(0,3)){
            const auto coverRay=sensors::WorldTrace(static_cast<uint32_t>(index),cover,row->focus,nullptr);
            const auto openRay=sensors::WorldTrace(static_cast<uint32_t>(index),open,row->focus,nullptr);
            const auto actual=sensors::WorldTrace(static_cast<uint32_t>(index),own.eye,row->focus,nullptr);
            state.sightProbes+=3;cache.coverLOS=coverRay!=core::Ray::Unknown&&openRay!=core::Ray::Unknown;
            cache.coverBlocked=coverRay==core::Ray::Blocked;cache.openClear=openRay==core::Ray::Clear;
            cache.proofs.peekSightKnown=actual!=core::Ray::Unknown;cache.proofs.peekSightClear=actual==core::Ray::Clear;
        }
        cache.proofs.exposedAngleCount=mapHold?row->authoredExposedAngles:(cache.openClear?1U:0U);
        cache.proofs.coverKnown=(row->kind==data::Kind::Cover||row->kind==data::Kind::Hold)&&cache.coverLOS&&cache.coverBlocked&&cache.openClear
            &&cache.proofs.exposedAngleCount<=static_cast<uint32_t>(SpaceValue(profile,cfg::Param::CoverMaximumExposedAngles));
        if(mapHold){
            cache.proofs.holdProofReady=cache.proofs.coverKnown&&cache.proofs.retreatPathProven&&cache.proofs.groundProven;
            // All three calls ran under budget; an unknown/rejected native LOS
            // cannot become a later optimistic hold at an unproved location.
            if(!cache.proofs.holdProofReady){cache.estimatedGoalRejected=true;cache.haveGoal=cache.pathValid=false;ClearCover(cache);}
        }
        if(cache.proofs.angleCount<cache.proofs.angles.size()){
            auto&angle=cache.proofs.angles[cache.proofs.angleCount++];angle={};angle.id=row->id|0x80000000U;
            angle.valid=angle.staticMapKnowledge=true;angle.conventional=row->conventional;angle.focusPoint=row->focus;
            angle.risk=row->authoredRisk;angle.heightConfidence=row->authoredConfidence;
            angle.coverFocusProven=cache.proofs.coverKnown;
            angle.authoredStatic=true;angle.mapRevision=state.revision;
        }
    }
    (void)now;
}
inline void AdvanceProvenCorner(size_t index,double now,Cache&cache,const dto::OwnSnapshot&own,const cfg::Profile&profile){
    if(!index||index>=MaxPlayers||!LocalSource(index,own.life)||!SameLife(cache.life,own.life)
       ||!cache.haveGoal||!cache.pathValid||!cache.pathGeneration||!cache.goalId
       ||cache.pathCount>MaxPath||cache.pathAt+1>=cache.pathCount
       ||!cache.progress.previousDemand||!SameLife(cache.progress.key.life,own.life)
       ||cache.progress.key.pathGeneration!=cache.pathGeneration||cache.progress.key.goalId!=cache.goalId
       ||cache.progress.key.node!=cache.pathAt)return;
    const auto*player=local_server_match::Player(index);const auto&actor=training_bot_controller::actors[index];
    const auto&observed=actor.observed;
    if(!player||!actor.bound||actor.pawn!=player->pawn||actor.epoch!=player->epoch
       ||!observed.ready||observed.pawn!=player->pawn||observed.spawnEpoch!=own.life.spawnEpoch||observed.controller!=player->pc)return;
    const double radius=observed.capsuleRadius,half=observed.capsuleHalfHeight;
    const double arrival=SpaceValue(profile,cfg::Param::MoveArrivalCm);
    if(!progress::CanTestCornerShortcut(own,sensors::Vec(cache.path[cache.pathAt]),arrival,
       SpaceValue(profile,cfg::Param::OwnSnapshotMaxAgeMs)*.001,radius,half))return;
    if(cache.cornerResult==GeometryResult::Blocked&&cache.cornerGeneration==cache.pathGeneration
       &&cache.cornerNode==cache.pathAt&&cache.cornerRadius==radius&&cache.cornerHalf==half
       &&training_bot_controller::Distance(cache.cornerFrom,sensors::Native(own.feet))<=arrival)return;
    const auto from=sensors::Native(own.feet),successor=cache.path[cache.pathAt+1];
    if(!original_players::Finite(successor))return;
    if(GroundCost(from,successor,radius)>state.groundBudget||!state.queryBudget){++cache.cornerDeferrals;return;}
    cache.cornerAt=now;cache.cornerGeneration=cache.pathGeneration;cache.cornerNode=cache.pathAt;
    cache.cornerFrom=from;cache.cornerCurrent=cache.path[cache.pathAt];cache.cornerSuccessor=successor;
    const auto life=own.life;const auto generation=cache.pathGeneration;const auto node=cache.pathAt;const auto goal=cache.goalId;
    ++cache.cornerQueries;cache.cornerProven=false;
    GeometryResult cornerResult=GeometryResult::Unknown;
    const bool proven=Segment(index,from,successor,radius,half,&cornerResult);
    if(!LocalSource(index,life)||!SameLife(cache.life,life)||!cache.haveGoal||!cache.pathValid
       ||generation!=cache.pathGeneration||goal!=cache.goalId||node!=cache.pathAt||cache.pathAt+1>=cache.pathCount)return;
    cache.cornerProven=proven;cache.cornerResult=cornerResult;cache.cornerRadius=radius;cache.cornerHalf=half;
    if(now>=cache.cornerProfileNextAt){
        cache.cornerProfileNextAt=now+1;
        if(Spend(0,1)){
            const auto comparison=humanlike_capsule_profile_comparison::Observe(index,player->pawn,from,successor,radius,half);
            if(!LocalSource(index,life)||!SameLife(cache.life,life)||!cache.haveGoal||!cache.pathValid
               ||generation!=cache.pathGeneration||goal!=cache.goalId||node!=cache.pathAt||cache.pathAt+1>=cache.pathCount)return;
            cache.cornerProfile=comparison;
            ++cache.cornerProfileSamples;
            cache.cornerProfileAt=now;
            cache.cornerProfileGeneration=generation;
            cache.cornerProfileNode=node;
            cache.cornerProfileOldSegment=proven;
            cache.cornerProfileFrom=from;
            cache.cornerProfileTo=successor;
        }else ++cache.cornerProfileBudgetDeferrals;
    }
    if(!proven)return;
    // Original capsule and supporting-ground proof permits movement toward
    // this successor. The final destination still uses normal own arrival.
    ++cache.pathAt;++cache.cornerAdvances;cache.movementCheckedAt=now;cache.movementCheckedNode=cache.pathAt;
}
inline bool ReadSpace(size_t index,double now,const dto::OwnSnapshot&own,dto::SpaceHints&out){
    out={};if(!Context()||index==0||index>=MaxPlayers||!own.valid||!own.alive||!sensors::GetState().readProfile
       ||!std::isfinite(now)||now<0||own.nowSeconds!=now||!LocalSource(index,own.life)){
        if(index>0&&index<MaxPlayers){ClearEstimatedGoal(caches[index],now);ClearAim(caches[index]);}return false;
    }
    const auto*global=sensors::GetState().readProfile(0),*profile=sensors::GetState().readProfile(index);if(!global||!profile)return false;
    auto&cache=caches[index];if(!SameLife(cache.life,own.life)){cache={};cache.life=own.life;}
    if(cache.navigationEstimate.requested&&!humanlike_bot_tactics_navigation::ValidRequest(cache.navigationEstimate,own,now))ClearEstimatedGoal(cache,now);
    if(now!=state.passAt){state.passAt=now;++state.passes;state.groundBudget=static_cast<uint32_t>(SpaceValue(*global,cfg::Param::SpaceGroundQueryBudget));
        state.queryBudget=static_cast<uint32_t>(SpaceValue(*global,cfg::Param::SpaceCoverQueryBudget));state.eligible=state.nextJob;}
    // Grounded original map anchors are static knowledge, never opponent data.
    const auto count=training_bot_controller::anchorCount;
    const float arrival=SpaceValue(*profile,cfg::Param::MoveArrivalCm);
    // Spawn/map anchors propose destinations only. They are not visible
    // threat angles and must never direct the crosshair through a wall.
    if(cache.pathValid){while(cache.pathAt<cache.pathCount&&ReachedPathPoint(sensors::Native(own.feet),cache.path[cache.pathAt],arrival)){
            if(cache.pathAt&&!NativeFlatEdge(cache,cache.pathAt)){
                const auto*npc=local_server_npc_input::Player(index);
                cache.slopeTransitionPending=true;cache.slopeTransitionGeneration=cache.pathGeneration;cache.slopeTransitionNode=cache.pathAt+1;
                cache.slopeTransitionPhysics=npc?npc->physicsCompleted:0;cache.slopeTransitionNative=npc?npc->nativeAdvances:0;
                cache.slopeTransitionOpportunityAt=-1;
            }
            ++cache.pathAt;
        }
        if(cache.pathAt==cache.pathCount){cache.pathValid=false;cache.haveGoal=false;cache.recoveryRetried=false;cache.pathQueryFailures=0;cache.failedGoalId=0;
            if(cache.navigationEstimate.requested)cache.estimatedGoalReached=true;}}
    if(index==state.eligible){state.nextJob=NextNPC(static_cast<uint32_t>(index));
        if(!cache.navigationEstimate.requested&&!cache.haveGoal&&count&&!(count==1&&cache.failedGoalId==1)){auto choice=core::Mix(state.seed^core::Mix(own.life.stableBotId)^core::Mix(++cache.goalSequence));
            const auto lifeSample=core::Mix(state.seed^core::Mix(own.life.stableBotId)^core::Mix(own.life.spawnEpoch));
            const double quantile=(lifeSample>>40)*(1.0/16777216.0);
            if(quantile<SpaceValue(*profile,cfg::Param::RespawnRouteVariation))choice=core::Mix(choice^core::Mix(own.life.spawnEpoch));
            size_t selected=RecoveryAnchor(cache,own,*profile,count);
            for(size_t offset=0;selected==count&&offset<count;++offset){const auto candidate=(static_cast<size_t>(choice%count)+offset)%count;
                if(candidate+1==cache.failedGoalId||ReachedPathPoint(sensors::Native(own.feet),training_bot_controller::anchors[candidate],arrival))continue;
                selected=candidate;break;
            }
            if(selected<count){const auto nextGoalId=static_cast<uint32_t>(selected+1);
                if(nextGoalId!=cache.goalId){cache.recoveryRetried=false;cache.pathQueryFailures=0;}
                cache.failedGoalId=0;cache.goal=training_bot_controller::anchors[selected];cache.goalId=nextGoalId;cache.haveGoal=true;}}
        if(cache.haveGoal&&!cache.pathValid&&now>=cache.nextPlan){
            cache.nextPlan=now+SpaceValue(*profile,cfg::Param::MoveStallMs)/1000.;PathAttempt attempt{};
            const bool accepted=Path(index,cache,own,*profile,&attempt);
            if(accepted)cache.pathQueryFailures=0;
            else if(attempt.geometryBlocked){RejectBlockedPath(cache);}
            else if(attempt.budgetDeferred||attempt.geometryUnknown){cache.nextPlan=now;}
            else if(attempt.queryExecuted){
                cache.pathQueryFailures=cache.pathQueryFailures?2:1;
                // An acquired position is a frozen uncertain guess. Repeating
                // its failed route after the full stall delay can let frequent
                // sound episodes suspend roaming indefinitely. Return this
                // failed estimate to the brain immediately; explicit unknown
                // and budget-deferred work was retained by the branch above.
                const bool acquired=cache.navigationEstimate.requested
                    &&cache.navigationEstimate.knowledge==dto::NavigationKnowledge::AcquiredEstimate;
                if(acquired||cache.recoveryRetried||cache.pathQueryFailures==2){cache.failedGoalId=cache.goalId;cache.haveGoal=false;
                    if(cache.navigationEstimate.requested)cache.estimatedGoalRejected=true;
                    cache.nextPlan=now;++cache.recoveryGoalChanges;}
            }
        }
        AdvanceProvenCorner(index,now,cache,own,*profile);
        CheckCurrentMovement(index,now,cache,own,*profile);
        cache.proofs.angleCount=0;Proof(index,now,cache,own,*profile);
        if(cache.aimEstimate.requested)AimProof(index,now,cache,own);
    }
    out=cache.proofs;
    out.estimatedNavigation=cache.navigationEstimate;out.estimatedGoalReached=cache.estimatedGoalReached;out.estimatedGoalRejected=cache.estimatedGoalRejected;
    if(cache.aimEstimate.requested&&(!humanlike_bot_prefire::Valid(cache.aimEstimate,own,now)||!AimSourceKnown(cache.aimEstimate)))ClearAim(cache);
    out.estimatedAim=cache.aimEstimate;out.estimatedAimKnown=cache.aimKnown;out.estimatedAimClear=cache.aimClear;
    out.estimatedAimEye=cache.aimEye;out.estimatedAimAtSeconds=cache.aimAt;
    // Prefer the selected immutable row, then the nearest bounded static list.
    // Unknown labels/risk remain unknown; random anchors never become holds.
    std::array<float,dto::MaxHoldLocations> distances{};distances.fill(INFINITY);
    const auto map=local_server_match::Configuration().mapId;
    const auto*selected=HoldAnnotation(cache.navigationEstimate);
    for(size_t n=0;n<state.count;++n){const auto&r=state.annotations[n];
        if(!r.enabled||r.kind!=data::Kind::Hold||std::strcmp(r.mapId,map))continue;++out.authoredHoldCount;
        dto::HoldLocation location{};location.id=r.id;location.revision=state.revision;location.valid=location.staticMapKnowledge=r.focusKnown;
        location.conventional=r.conventional;location.position=r.open;location.cover=r.start;location.retreat=r.retreat;location.focus=r.focus;
        location.range=r.range;location.confidence=r.authoredConfidence;location.risk=r.authoredRisk;location.flankRisk=r.authoredFlankRisk;
        location.exposedAngles=r.authoredExposedAngles;location.riskKnown=r.riskKnown;location.flankRiskKnown=r.flankRiskKnown;location.exposedAnglesKnown=r.exposedAnglesKnown;
        const float distance=&r==selected?-1.f:core::Length(core::Sub(r.open,own.feet));
        size_t at=0;while(at<out.holdLocationCount&&distances[at]<=distance)++at;
        if(at>=dto::MaxHoldLocations)continue;
        for(size_t j=(std::min)(static_cast<size_t>(out.holdLocationCount),dto::MaxHoldLocations-1);j>at;--j){out.holdLocations[j]=out.holdLocations[j-1];distances[j]=distances[j-1];}
        out.holdLocations[at]=location;distances[at]=distance;if(out.holdLocationCount<dto::MaxHoldLocations)++out.holdLocationCount;
    }
    if(cache.navigationEstimate.purpose==dto::NavigationPurpose::MapHold)
        out.holdProofReady=selected&&cache.annotationId==selected->id&&cache.proofs.holdProofReady&&!cache.estimatedGoalRejected;
    out.navigationKnown=count>0||cache.navigationEstimate.requested;out.pathProven=cache.pathValid&&cache.pathAt<cache.pathCount;
    if(out.pathProven){out.nextPathPoint=sensors::Vec(cache.path[cache.pathAt]);out.routeGoal=sensors::Vec(cache.goal);out.routeId=cache.goalId;out.groundProven=true;}
    if(cache.navigationEstimate.purpose==dto::NavigationPurpose::MapHold&&!out.holdProofReady)out.pathProven=false;
    const double maximumAge=SpaceValue(*profile,cfg::Param::OwnSnapshotMaxAgeMs)*.001;
    patrol::Step(cache.routeFocus,own,cache.pathGeneration,cache.pathAt,cache.pathCount,out.pathProven,arrival,maximumAge,
        [&](size_t node){return sensors::Vec(cache.path[node]);},[&](dto::Vec3 eye,dto::Vec3 point){
            // Keep the existing fair spatial job and reserve its original
            // path/cover work. Early roster slots cannot consume a later
            // slot's cover budget merely to extend their own lookahead.
            if(index!=state.eligible)return patrol::Ray::Deferred;
            if(!Spend(0,1))return patrol::Ray::Deferred;
            ++state.sightProbes;const auto ray=sensors::WorldTrace(static_cast<uint32_t>(index),eye,point,nullptr);
            return ray==core::Ray::Clear?patrol::Ray::Clear:ray==core::Ray::Blocked?patrol::Ray::Blocked:patrol::Ray::Unknown;
        });
    if(out.pathProven&&patrol::Current(cache.routeFocus,own,arrival,maximumAge)){
        const auto keep=(std::min)(static_cast<size_t>(out.angleCount),out.angles.size()-1);
        for(size_t n=keep;n>0;--n)out.angles[n]=out.angles[n-1];
        auto&angle=out.angles[0];angle={};angle.id=0x40000000U|(cache.goalId<<8)|static_cast<uint32_t>(cache.routeFocus.node);
        angle.valid=angle.staticMapKnowledge=angle.conventional=angle.routeDirected=true;angle.focusPoint=cache.routeFocus.focus;
        out.angleCount=static_cast<uint8_t>(keep+1);
    }
    // Local clearance/reveal proof follows actual own position; old static
    // annotation coordinates never substitute for current physical exposure.
    if(core::Length(core::Sub(own.feet,cache.proofFeet))>arrival){out.clearanceKnown=false;out.peekSightKnown=false;}
    if(cache.annotationId==0&&cache.localCover.proposal.valid){
        const auto&o=training_bot_controller::actors[index].observed;
        const auto currentLocal=local::Publish(cache.localCover,own,local::FromProfile(*profile,static_cast<float>(o.capsuleRadius)),
            static_cast<float>(o.capsuleRadius),static_cast<float>(o.capsuleHalfHeight));
        ApplyLocalProof(out,currentLocal);out.groundProven=out.pathProven||currentLocal.groundProven;
    }
    if(selected&&cache.navigationEstimate.purpose==dto::NavigationPurpose::MapHold){
        // A remote authored triangle proves safety at that destination, not a
        // direct own-feet approach or retreat while en route. Keep its separate
        // hold admission proof for original-path movement; ordinary duel/reload
        // consumers may use the triangle only after actual local arrival.
        const bool connected=humanlike_bot_hold_positions::Arrived(own,selected->open,arrival)
            ||humanlike_bot_hold_positions::Arrived(own,selected->start,arrival)
            ||humanlike_bot_hold_positions::Arrived(own,selected->retreat,arrival);
        if(!connected){out.coverKnown=out.retreatPathProven=out.peekPathProven=out.landingSupportKnown=false;out.peekSightKnown=out.peekSightClear=false;}
    }
    const auto current=local_server_match::Player(index);
    return current&&current->epoch==own.life.spawnEpoch&&sensors::OwnToken(current->pawnIdentity)==own.life.token;
}
inline bool Reseed(uint64_t seed){if(!Context())return false;state.seed=seed;return true;}
inline const State& GetState(){return state;}
} // namespace local_server_humanlike_space
