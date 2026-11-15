#pragma once
#include "humanlike_bot_math.h"
#include <cmath>
#include <cstddef>
#include <cstdint>

// Own-data observation only. An accepted input request and advancing original
// physics are opportunities, not per-Intent consumption or edge reachability.
namespace humanlike_bot_navigation_progress {
namespace dto=humanlike_bot;
struct Settings {double arrival=0,stallSeconds=0,maximumAgeSeconds=0;};
struct Key {dto::OwnLife life{};uint64_t pathGeneration=0;uint32_t goalId=0;size_t node=0;};
inline bool Same(const Key&a,const Key&b){return dto::math::Same(a.life,b.life)&&a.pathGeneration==b.pathGeneration&&a.goalId==b.goalId&&a.node==b.node;}
inline bool Same(const Settings&a,const Settings&b){return a.arrival==b.arrival&&a.stallSeconds==b.stallSeconds&&a.maximumAgeSeconds==b.maximumAgeSeconds;}
struct Sample {
    const dto::OwnSnapshot*own=nullptr;Key key{};dto::Vec3 nextNode{};
    bool pathValid=false,navigationSubmitted=false,nativeOpportunityKnown=false;
    uint64_t physicsCompleted=0,nativeAdvances=0;
};
struct State {
    bool bound=false,previousDemand=false;Key key{};Settings settings{};dto::Vec3 progressFeet{},nextNode{};double bestDistance=0,progressDistance=0;
    double lastAt=-1,lastOwnSample=-1,lastOpportunityAt=-1,thresholdAt=-1,pendingSeconds=0,activeSeconds=0;
    uint64_t physicsCompleted=0,nativeAdvances=0,eligibleIntervals=0,stalls=0;
};
struct Result {bool progressed=false,stalled=false,eligible=false;};
inline void Pause(State&state){state.previousDemand=false;state.pendingSeconds=0;}
inline bool CanTestCornerShortcut(const dto::OwnSnapshot& own,dto::Vec3 corner,
                                  double arrival,double maximumAge,double radius,double halfHeight){
    if(!own.life.stableBotId||!own.life.spawnEpoch||!own.life.token||!own.valid||!own.alive||own.retired||own.paused
       ||!own.movementValid||!own.groundedKnown||!own.grounded||!dto::math::Finite(own.feet)||!dto::math::Finite(corner)
       ||!std::isfinite(own.nowSeconds)||!std::isfinite(own.sampledAtSeconds)||own.sampledAtSeconds<0
       ||own.sampledAtSeconds>own.nowSeconds||!std::isfinite(maximumAge)||maximumAge<=0
       ||own.nowSeconds-own.sampledAtSeconds>maximumAge||!std::isfinite(arrival)||arrival<=0
       ||!std::isfinite(radius)||!std::isfinite(halfHeight)||radius<=0||halfHeight<radius)return false;
    const auto offset=dto::math::Sub(corner,own.feet);
    const double distance=dto::math::Horizontal(offset);
    return distance>arrival&&distance<=2*radius+arrival&&std::abs(offset.z)<=arrival;
}
inline double MinimumProgressCm(const Settings& settings){
    // Accumulate sub-centimeter steps. The threshold stays below the distance
    // a 1cm/s approach travels during the configured window, up to 1cm.
    return (std::min)(1.0,settings.stallSeconds*.5);
}
inline Result Observe(State&state,const Sample&sample,const Settings&settings){
    const auto*own=sample.own;
    const bool valid=own&&own->life.stableBotId>0&&own->life.spawnEpoch&&own->life.token&&own->valid&&own->alive&&!own->retired&&!own->paused
        &&own->movementValid&&dto::math::Finite(own->feet)&&std::isfinite(own->nowSeconds)&&std::isfinite(own->sampledAtSeconds)
        &&own->sampledAtSeconds>=0&&own->sampledAtSeconds<=own->nowSeconds
        &&std::isfinite(settings.arrival)&&settings.arrival>0&&std::isfinite(settings.stallSeconds)&&settings.stallSeconds>0
        &&std::isfinite(settings.maximumAgeSeconds)&&settings.maximumAgeSeconds>0
        &&own->nowSeconds-own->sampledAtSeconds<=settings.maximumAgeSeconds
        &&sample.pathValid&&dto::math::Finite(sample.nextNode)&&sample.key.pathGeneration&&sample.key.goalId&&dto::math::Same(sample.key.life,own->life);
    if(!valid){Pause(state);return{};}
    const double dx=static_cast<double>(own->feet.x)-sample.nextNode.x;
    const double dy=static_cast<double>(own->feet.y)-sample.nextNode.y;
    const double dz=static_cast<double>(own->feet.z)-sample.nextNode.z;
    const double distance=std::sqrt(dx*dx+dy*dy+dz*dz);
    if(!std::isfinite(distance)){Pause(state);return{};}
    if(!state.bound||!Same(state.key,sample.key)||!Same(state.settings,settings)
       ||state.nextNode.x!=sample.nextNode.x||state.nextNode.y!=sample.nextNode.y||state.nextNode.z!=sample.nextNode.z
       ||own->nowSeconds<state.lastAt||own->sampledAtSeconds<state.lastOwnSample
       ||sample.physicsCompleted<state.physicsCompleted||sample.nativeAdvances<state.nativeAdvances){
        const auto stalls=state.stalls,intervals=state.eligibleIntervals;
        state={};state.bound=true;state.key=sample.key;state.settings=settings;state.progressFeet=own->feet;
        state.nextNode=sample.nextNode;state.bestDistance=state.progressDistance=distance;
        state.lastAt=state.lastOpportunityAt=own->nowSeconds;state.lastOwnSample=own->sampledAtSeconds;
        state.physicsCompleted=sample.physicsCompleted;state.nativeAdvances=sample.nativeAdvances;
        state.stalls=stalls;state.eligibleIntervals=intervals;
        state.previousDemand=sample.navigationSubmitted&&sample.nativeOpportunityKnown&&own->groundedKnown&&own->grounded;
        return{};
    }
    Result result{};
    const bool freshSample=own->sampledAtSeconds>state.lastOwnSample;
    // Keep every better distance, but reset the timer only after cumulative
    // progress reaches a small distance threshold. Millimeter collision drift
    // can otherwise postpone recovery forever while full input is consumed.
    if(freshSample&&distance<state.bestDistance){
        state.bestDistance=distance;
        if(state.progressDistance-distance>=MinimumProgressCm(settings)){
            state.progressDistance=distance;state.progressFeet=own->feet;
            state.pendingSeconds=state.activeSeconds=0;state.thresholdAt=-1;result.progressed=true;
        }
    }
    const bool opportunity=sample.nativeOpportunityKnown&&(sample.physicsCompleted>state.physicsCompleted||sample.nativeAdvances>state.nativeAdvances);
    const bool demand=sample.navigationSubmitted&&sample.nativeOpportunityKnown&&own->groundedKnown&&own->grounded;
    const auto delta=own->nowSeconds-state.lastAt;
    const bool continuous=demand&&state.previousDemand&&delta>0&&delta<=settings.maximumAgeSeconds;
    if(continuous&&!result.progressed)state.pendingSeconds+=delta;
    else if(!demand||!state.previousDemand||delta>settings.maximumAgeSeconds||result.progressed)state.pendingSeconds=0;
    // GT polls may be faster than the sole original 128Hz physics owner.
    // Settle continuous requested time on its next actual opportunity, rather
    // than counting only the last tiny GT delta or a long unsimulated gap.
    const auto opportunitySpan=own->nowSeconds-state.lastOpportunityAt;
    result.eligible=demand&&state.previousDemand&&opportunity&&state.pendingSeconds>0
        &&opportunitySpan>0&&opportunitySpan<=settings.maximumAgeSeconds;
    if(opportunity){
        if(result.eligible){state.activeSeconds+=state.pendingSeconds;++state.eligibleIntervals;}
        state.pendingSeconds=0;state.lastOpportunityAt=own->nowSeconds;
    }
    // A cached stationary pose cannot itself terminate the observation window.
    // A real own sample at/after the completed window confirms the lack of
    // progress. It need not arrive on the exact same GT poll as a physics step.
    if(result.eligible&&!result.progressed&&state.activeSeconds>=settings.stallSeconds){
        if(state.thresholdAt<0)state.thresholdAt=own->nowSeconds;
        if(own->sampledAtSeconds>=state.thresholdAt){
            result.stalled=true;++state.stalls;state.activeSeconds=0;state.thresholdAt=-1;state.progressFeet=own->feet;
            state.progressDistance=state.bestDistance;
        }
    }
    state.previousDemand=demand;state.lastAt=own->nowSeconds;state.lastOwnSample=own->sampledAtSeconds;
    state.physicsCompleted=sample.physicsCompleted;state.nativeAdvances=sample.nativeAdvances;
    return result;
}
} // namespace humanlike_bot_navigation_progress
