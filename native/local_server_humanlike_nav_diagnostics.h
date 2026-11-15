#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include "local_server_humanlike_space.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>

// FILE bridge only. Cache is called on the existing owner GT/cache cadence;
// Json only copies a string. No UObject, native query, action, MMF or timer.
namespace local_server_humanlike_nav_diagnostics {
namespace dto=humanlike_bot;namespace cfg=humanlike_bot_config;namespace nav=local_server_humanlike_space;
struct Source{const dto::SensorFrame*frame=nullptr;const cfg::Profile*profile=nullptr;};
inline SRWLOCK lock=SRWLOCK_INIT;
inline std::string cached="{\"cached\":false,\"bots\":[]}";
inline void Bool(std::string&out,bool value){out+=value?"true":"false";}
inline void Number(std::string&out,double value){if(!std::isfinite(value)){out+="null";return;}char b[384]{};const int n=snprintf(b,sizeof(b),"%.6f",value);if(n<0||static_cast<size_t>(n)>=sizeof(b))out+="null";else out.append(b,static_cast<size_t>(n));}
inline void Integer(std::string&out,uint64_t value){char b[32]{};const int n=snprintf(b,sizeof(b),"%llu",static_cast<unsigned long long>(value));if(n>0&&static_cast<size_t>(n)<sizeof(b))out.append(b,static_cast<size_t>(n));else out+="null";}
inline void Point(std::string&out,const engine::Vec&p,bool known){if(!known){out+="null";return;}out+='[';Number(out,p.x);out+=',';Number(out,p.y);out+=',';Number(out,p.z);out+=']';}
inline void Life(std::string&out,const dto::OwnLife&life){out+="{\"stable_bot_id\":";Integer(out,life.stableBotId);out+=",\"epoch\":";Integer(out,life.spawnEpoch);out+=",\"token\":";Integer(out,life.token);out+='}';}

// These are the existing cached own/local-cover PODs, not new geometry work.
inline void OptionalNumber(std::string&out,double value,bool known){if(known&&std::isfinite(value))Number(out,value);else out+="null";}
inline void OwnRotation(std::string&out,dto::Rotation value,bool known){
    if(!known||!dto::math::Finite(value)){out+="null";return;}
    out+="{\"pitch_deg\":";Number(out,value.pitch);out+=",\"yaw_deg\":";Number(out,value.yaw);out+='}';
}
inline void GeometryPoint(std::string&out,dto::Vec3 value,bool known){Point(out,{value.x,value.y,value.z},known&&dto::math::Finite(value));}
inline void AppendGeometry(std::string&out,const nav::Cache&c,const dto::SensorFrame*f,bool ownKnown,bool currentKnown){
    const auto*own=f?&f->self:nullptr;const auto&w=c.localCover;const auto&p=w.proposal;
    const bool localLife=own&&dto::math::Same(w.life,own->life);
    out+=",\"own_eye\":";GeometryPoint(out,own?own->eye:dto::Vec3{},ownKnown&&own->eyeValid);
    out+=",\"own_view\":";OwnRotation(out,own?own->view:dto::Rotation{},ownKnown&&own->viewValid);
    out+=",\"view_sample_time_known\":";Bool(out,ownKnown&&own->viewSampleTimeKnown);
    out+=",\"view_sampled_at\":";OptionalNumber(out,own?own->viewSampledAtSeconds:0,ownKnown&&own->viewSampleTimeKnown);
    out+=",\"own_stance_known\":";Bool(out,ownKnown&&own->stanceValid);
    out+=",\"own_crouched\":";if(ownKnown&&own->stanceValid)Bool(out,own->crouched);else out+="null";
    out+=",\"own_movement_known\":";Bool(out,ownKnown&&own->movementValid);
    out+=",\"own_walking\":";if(ownKnown&&own->movementValid)Bool(out,own->walking);else out+="null";
    out+=",\"own_native_max_speed_cm_s\":";OptionalNumber(out,own?own->nativeMaxSpeedCmS:0,ownKnown&&own->movementValid&&own->nativeMaxSpeedCmS>=0);
    out+=",\"own_native_fov_deg\":";OptionalNumber(out,own?own->nativeHorizontalFovDegrees:0,ownKnown&&own->nativeViewFovKnown&&own->nativeHorizontalFovDegrees>0);
    out+=",\"local_cover\":{\"schema_version\":1,\"life\":";Life(out,w.life);
    out+=",\"same_own_life\":";Bool(out,localLife);out+=",\"cache_annotation_id\":";Integer(out,c.annotationId);
    out+=",\"ready\":";Bool(out,w.ready);out+=",\"proposal_valid\":";Bool(out,p.valid);
    out+=",\"proposal_id\":";if(p.valid)Integer(out,p.id);else out+="null";
    out+=",\"source\":";
    if(!p.valid)out+="null";
    else if(p.source==humanlike_bot_local_cover::Source::Navigation)out+="\"navigation\"";
    else if(p.source==humanlike_bot_local_cover::Source::OwnView)out+="\"own_view\"";else out+="null";
    out+=",\"cursor\":";if(w.cursor<8)Integer(out,w.cursor);else out+="null";
    out+=",\"start_feet\":";GeometryPoint(out,p.start,p.valid);out+=",\"open_feet\":";GeometryPoint(out,p.open,p.valid);
    out+=",\"retreat_feet\":";GeometryPoint(out,p.retreat,p.valid);out+=",\"focus\":";GeometryPoint(out,p.focus,p.valid);
    out+=",\"generated_at\":";OptionalNumber(out,w.generatedAt,p.valid&&w.generatedAt>=0);
    out+=",\"proved_at\":";OptionalNumber(out,w.provedAt,w.ready&&w.provedAt>=0);
    out+=",\"sight_at\":";OptionalNumber(out,w.sightAt,w.sightRay!=humanlike_bot_local_cover::Ray::Unknown&&w.sightAt>=0);
    out+=",\"proved_feet\":";GeometryPoint(out,w.provedFeet,w.ready);out+=",\"sight_eye\":";GeometryPoint(out,w.sightEye,w.sightRay!=humanlike_bot_local_cover::Ray::Unknown);
    out+=",\"sight_ray\":\"";out+=w.sightRay==humanlike_bot_local_cover::Ray::Blocked?"blocked":w.sightRay==humanlike_bot_local_cover::Ray::Clear?"clear":"unknown";out+='\"';
    out+=",\"checked_closed\":";Bool(out,w.checkedClosed);out+=",\"checked_open\":";Bool(out,w.checkedOpen);
    out+=",\"segment_mask\":";Integer(out,w.segments);
    out+=",\"settings_known\":";Bool(out,w.settingsKnown);
    out+=",\"settings_valid\":";Bool(out,w.settingsKnown&&humanlike_bot_local_cover::Valid(w.settings));
    out+=",\"near_cm\":";OptionalNumber(out,w.settings.nearCm,w.settingsKnown&&w.settings.nearCm>=0);
    out+=",\"far_cm\":";OptionalNumber(out,w.settings.farCm,w.settingsKnown&&w.settings.farCm>=0);
    out+=",\"focus_cm\":";OptionalNumber(out,w.settings.focusCm,w.settingsKnown&&w.settings.focusCm>=0);
    out+=",\"capsule_radius_cm\":";OptionalNumber(out,w.radius,w.settingsKnown&&w.radius>0);
    out+=",\"capsule_half_height_cm\":";OptionalNumber(out,w.half,w.settingsKnown&&w.half>=w.radius&&w.half>0);
    out+=",\"current_predicates_known\":";Bool(out,currentKnown);
    out+=",\"current_proof\":";if(currentKnown)Bool(out,humanlike_bot_local_cover::CurrentProof(w,*own));else out+="null";
    out+=",\"current_approach\":";if(currentKnown)Bool(out,humanlike_bot_local_cover::CurrentApproach(w,*own));else out+="null";
    out+=",\"current_sight\":";if(currentKnown)Bool(out,humanlike_bot_local_cover::CurrentSight(w,*own));else out+="null";
    // Exact pure helper predicates above are not a replacement publication.
    // Preserve the actual SensorFrame output and its original proof flags.
    out+=",\"current_out\":";
    if(!f)out+="null";else {const auto&s=f->space;out+="{\"cover_known\":";Bool(out,s.coverKnown);
        out+=",\"peek_path\":";Bool(out,s.peekPathProven);out+=",\"retreat_path\":";Bool(out,s.retreatPathProven);
        out+=",\"ground_path\":";Bool(out,s.groundProven);out+=",\"clearance_known\":";Bool(out,s.clearanceKnown);
        out+=",\"peek_sight_known\":";Bool(out,s.peekSightKnown);out+=",\"peek_sight_clear\":";Bool(out,s.peekSightClear);
        out+=",\"landing_support\":";Bool(out,s.landingSupportKnown);
        out+=",\"cover_confidence\":";Number(out,s.coverConfidence);out+=",\"exposure_risk\":";Number(out,s.exposureRisk);
        out+=",\"cover_start_feet\":";GeometryPoint(out,s.coverStart,s.coverKnown);
        out+=",\"peek_open_feet\":";GeometryPoint(out,s.peekOpen,s.peekPathProven);
        out+=",\"retreat_feet\":";GeometryPoint(out,s.retreatPoint,s.retreatPathProven);out+='}';
    }out+='}';
}

inline void AppendBot(std::string&out,size_t index,const Source&source,double now){
    const auto&c=nav::caches[index];const auto*f=source.frame;const auto*own=f?&f->self:nullptr;
    const bool life=own&&nav::SameLife(c.life,own->life);
    const bool ownKnown=own&&own->valid&&own->alive&&std::isfinite(own->nowSeconds)&&std::isfinite(own->sampledAtSeconds)
        &&own->sampledAtSeconds>=0&&own->sampledAtSeconds<=own->nowSeconds&&dto::math::Finite(own->feet);
    const bool profile=source.profile!=nullptr;
    const bool samePass=ownKnown&&own->nowSeconds==nav::state.passAt&&now==nav::state.passAt;
    const bool ownFresh=ownKnown&&profile&&now>=own->sampledAtSeconds
        &&now-own->sampledAtSeconds<=cfg::Get(*source.profile,cfg::Param::OwnSnapshotMaxAgeMs)*.001;
    const double arrival=profile?cfg::Get(*source.profile,cfg::Param::MoveArrivalCm):0;
    const double search=profile?cfg::Get(*source.profile,cfg::Param::SearchStepCm):0;
    const bool node=life&&c.pathValid&&c.pathCount<=nav::MaxPath&&c.pathAt<c.pathCount;
    const engine::Vec feet=ownKnown?engine::Vec{own->feet.x,own->feet.y,own->feet.z}:engine::Vec{};
    const auto next=node?c.path[c.pathAt]:engine::Vec{};
    const auto delta=training_bot_controller::Sub(next,feet);
    const bool metrics=node&&ownFresh&&samePass&&profile&&std::isfinite(arrival)&&arrival>=0&&std::isfinite(search)&&search>=0
        &&original_players::Finite(next)&&original_players::Finite(feet)&&original_players::Finite(delta);
    const double horizontal=metrics?training_bot_controller::Length2D(delta):0;
    const double distance=metrics?training_bot_controller::Distance(feet,next):0;
    const double vertical=metrics?std::abs(delta.z):0;
    const double step=metrics?(std::min)(search,horizontal):0;
    out+="{\"index\":";Integer(out,index);out+=",\"available\":";Bool(out,f!=nullptr);out+=",\"own_known\":";Bool(out,ownKnown);
    out+=",\"own_life\":";Life(out,own?own->life:dto::OwnLife{});out+=",\"cache_life\":";Life(out,c.life);
    out+=",\"same_life\":";Bool(out,life);out+=",\"cache_at\":";Number(out,now);out+=",\"space_pass_at\":";Number(out,nav::state.passAt);
    out+=",\"own_now\":";if(own)Number(out,own->nowSeconds);else out+="null";
    out+=",\"own_sampled_at\":";if(own)Number(out,own->sampledAtSeconds);else out+="null";
    out+=",\"own_age_ms\":";if(ownKnown)Number(out,(own->nowSeconds-own->sampledAtSeconds)*1000.);else out+="null";
    out+=",\"same_space_pass\":";Bool(out,samePass);out+=",\"own_fresh_at_cache\":";Bool(out,ownFresh);
    const auto&actor=training_bot_controller::actors[index];const auto&observed=actor.observed;
    const bool nativeOwnKnown=ownKnown&&life&&actor.bound&&actor.epoch==own->life.spawnEpoch
        &&observed.ready&&observed.alive&&observed.spawnEpoch==own->life.spawnEpoch&&observed.pawn==actor.pawn
        &&original_players::Finite(observed.feet)&&original_players::Finite(observed.navAgent);
    out+=",\"native_observed_feet\":";Point(out,observed.feet,nativeOwnKnown);
    out+=",\"native_nav_agent_location\":";Point(out,observed.navAgent,nativeOwnKnown);
    out+=",\"own_feet\":";Point(out,feet,ownKnown);out+=",\"ground_known\":";Bool(out,ownKnown&&own->groundedKnown);
    out+=",\"grounded\":";Bool(out,ownKnown&&own->groundedKnown&&own->grounded);
    AppendGeometry(out,c,f,ownKnown,ownKnown&&ownFresh&&samePass&&life);
    const auto&estimate=c.navigationEstimate;
    out+=",\"estimated_navigation\":{\"requested\":";Bool(out,estimate.requested);
    out+=",\"key\":";Integer(out,estimate.key);out+=",\"life\":";Life(out,estimate.life);
    out+=",\"purpose\":\"";
    out+=estimate.purpose==dto::NavigationPurpose::SoundApproach?"sound_approach":
        estimate.purpose==dto::NavigationPurpose::ThirdPartyJoin?"third_party_join":
        estimate.purpose==dto::NavigationPurpose::MapHold?"map_hold":"none";
    out+="\",\"knowledge\":\"";out+=estimate.knowledge==dto::NavigationKnowledge::StaticMap?"static_map":"acquired_estimate";
    out+="\",\"estimate_only_not_source_position\":";Bool(out,estimate.knowledge==dto::NavigationKnowledge::AcquiredEstimate);
    out+=",\"map_annotation_id\":";Integer(out,estimate.mapAnnotationId);out+=",\"map_revision\":";Integer(out,estimate.mapRevision);
    out+=",\"uses_hearing\":";Bool(out,estimate.usesHearing);
    out+=",\"estimated_goal\":";Point(out,{estimate.estimatedGoal.x,estimate.estimatedGoal.y,estimate.estimatedGoal.z},estimate.requested);
    out+=",\"confidence\":";Number(out,estimate.confidence);out+=",\"uncertainty_cm\":";Number(out,estimate.uncertaintyCm);
    out+=",\"evidence_at\":";Number(out,estimate.evidenceAtSeconds);out+=",\"ready_at\":";Number(out,estimate.readyAtSeconds);
    out+=",\"expires_at\":";Number(out,estimate.expiresAtSeconds);out+=",\"reached\":";Bool(out,c.estimatedGoalReached);
    out+=",\"rejected\":";Bool(out,c.estimatedGoalRejected);out+="}";
    out+=",\"map_hold\":{\"annotation_revision\":";Integer(out,nav::state.revision);
    out+=",\"enabled_authored_count\":";Integer(out,f?f->space.authoredHoldCount:0);
    out+=",\"proof_ready\":";Bool(out,f&&f->space.holdProofReady);out+=",\"locations\":[";
    if(f)for(size_t n=0;n<f->space.holdLocationCount&&n<dto::MaxHoldLocations;++n){const auto&r=f->space.holdLocations[n];
        if(n)out+=',';out+="{\"id\":";Integer(out,r.id);out+=",\"revision\":";Integer(out,r.revision);
        out+=",\"conventional\":";Bool(out,r.conventional);out+=",\"range\":\"";
        out+=r.range==dto::HoldRange::Near?"near":r.range==dto::HoldRange::Mid?"mid":r.range==dto::HoldRange::Far?"far":"unknown";
        out+="\",\"position\":";Point(out,{r.position.x,r.position.y,r.position.z},r.valid);
        out+=",\"cover\":";Point(out,{r.cover.x,r.cover.y,r.cover.z},r.valid);
        out+=",\"retreat\":";Point(out,{r.retreat.x,r.retreat.y,r.retreat.z},r.valid);
        out+=",\"focus\":";Point(out,{r.focus.x,r.focus.y,r.focus.z},r.valid);
        out+=",\"risk_known\":";Bool(out,r.riskKnown);out+=",\"risk\":";if(r.riskKnown)Number(out,r.risk);else out+="null";
        out+=",\"flank_risk_known\":";Bool(out,r.flankRiskKnown);out+=",\"flank_risk\":";if(r.flankRiskKnown)Number(out,r.flankRisk);else out+="null";
        out+=",\"exposed_angles_known\":";Bool(out,r.exposedAnglesKnown);out+=",\"exposed_angles\":";if(r.exposedAnglesKnown)Integer(out,r.exposedAngles);else out+="null";out+='}';
    }out+="]}";
    out+=",\"have_goal\":";Bool(out,c.haveGoal);out+=",\"path_valid\":";Bool(out,c.pathValid);
    out+=",\"goal_id\":";Integer(out,c.goalId);out+=",\"goal_sequence\":";Integer(out,c.goalSequence);
    out+=",\"path_count\":";Integer(out,c.pathCount);out+=",\"path_at\":";Integer(out,c.pathAt);
    out+=",\"next_node_known\":";Bool(out,node);out+=",\"next_node\":";Point(out,next,node);
    out+=",\"previous_node\":";Point(out,c.path[(node&&c.pathAt>0)?c.pathAt-1:0],node&&c.pathAt>0);
    out+=",\"terminal_node\":";Point(out,c.path[(node&&c.pathCount>0)?c.pathCount-1:0],node);
    const bool successor=node&&c.pathAt+1<c.pathCount;
    out+=",\"successor_node\":";Point(out,c.path[successor?c.pathAt+1:0],successor);
    out+=",\"corner_shortcut\":{\"queries\":";Integer(out,c.cornerQueries);
    out+=",\"advances\":";Integer(out,c.cornerAdvances);out+=",\"budget_deferrals\":";Integer(out,c.cornerDeferrals);
    out+=",\"last_at\":";Number(out,c.cornerAt);out+=",\"last_path_generation\":";Integer(out,c.cornerGeneration);
    out+=",\"last_node\":";Integer(out,c.cornerNode);out+=",\"segment_proven\":";Bool(out,c.cornerProven);
    out+=",\"from\":";Point(out,c.cornerFrom,c.cornerAt>=0);out+=",\"corner\":";Point(out,c.cornerCurrent,c.cornerAt>=0);
    out+=",\"successor\":";Point(out,c.cornerSuccessor,c.cornerAt>=0);out+="}";
    const auto&comparison=c.cornerProfile;
    out+=",\"corner_profile_comparison\":{\"capsule_query_only\":true,\"samples\":";Integer(out,c.cornerProfileSamples);
    out+=",\"budget_deferrals\":";Integer(out,c.cornerProfileBudgetDeferrals);
    out+=",\"at\":";Number(out,c.cornerProfileAt);
    out+=",\"path_generation\":";Integer(out,c.cornerProfileGeneration);
    out+=",\"node\":";Integer(out,c.cornerProfileNode);
    out+=",\"stage\":\"";out+=comparison.stage;out+='\"';
    out+=",\"abi_known\":";Bool(out,comparison.abiKnown);
    out+=",\"source_known\":";Bool(out,comparison.sourceKnown);
    out+=",\"profile_known\":";Bool(out,comparison.profileKnown);
    out+=",\"profile_resolved\":";Bool(out,comparison.profileResolved);
    out+=",\"resolved_object_type\":";if(comparison.profileResolved)Integer(out,comparison.resolvedObjectType);else out+="null";
    out+=",\"object_type_known\":";Bool(out,comparison.objectTypeKnown);
    out+=",\"object_type\":";if(comparison.objectTypeKnown)Integer(out,comparison.objectType);else out+="null";
    out+=",\"profile_name_words\":";
    if(comparison.profileKnown){out+='[';Integer(out,comparison.profileName.index);out+=',';Integer(out,comparison.profileName.comparison);out+=',';Integer(out,comparison.profileName.number);out+=']';}else out+="null";
    out+=",\"sweep_attempted\":";Bool(out,comparison.sweepAttempted);
    out+=",\"sweep_known\":";Bool(out,comparison.sweepKnown);
    out+=",\"profile_simple_capsule_clear\":";if(comparison.sweepKnown&&!comparison.componentResponsesKnown)Bool(out,comparison.sweepClear);else out+="null";
    out+=",\"component_responses_known\":";Bool(out,comparison.componentResponsesKnown);
    out+=",\"component_response_read_count\":";Integer(out,comparison.responseReadCount);
    out+=",\"component_responses\":";
    if(comparison.componentResponsesKnown){out+='[';for(size_t channel=0;channel<comparison.responses.size();++channel){if(channel)out+=',';Integer(out,comparison.responses[channel]);}out+=']';}else out+="null";
    out+=",\"component_simple_capsule_clear\":";if(comparison.sweepKnown&&comparison.componentResponsesKnown)Bool(out,comparison.sweepClear);else out+="null";
    out+=",\"old_trace0_complex_segment_clear\":null,\"movement_segment_grounded_clear\":";if(c.cornerProfileSamples)Bool(out,c.cornerProfileOldSegment);else out+="null";
    out+=",\"hit_known\":";Bool(out,comparison.hitKnown);
    out+=",\"hit_blocking\":";if(comparison.hitKnown)Bool(out,comparison.hitBlocking);else out+="null";
    out+=",\"hit_time\":";OptionalNumber(out,comparison.hitTime,comparison.hitKnown);
    out+=",\"hit_distance_cm\":";OptionalNumber(out,comparison.hitDistance,comparison.hitKnown);
    out+=",\"hit_impact_normal\":";Point(out,comparison.hitNormal,comparison.hitKnown);
    out+=",\"hit_component\":";if(comparison.hitComponent)Integer(out,comparison.hitComponent);else out+="null";
    out+=",\"hit_component_class\":";if(comparison.hitComponentClass)Integer(out,comparison.hitComponentClass);else out+="null";
    out+=",\"hit_component_owner\":";if(comparison.hitActor)Integer(out,comparison.hitActor);else out+="null";
    out+=",\"hit_component_owner_class\":";if(comparison.hitActorClass)Integer(out,comparison.hitActorClass);else out+="null";
    out+=",\"component_owner_is_blocking_volume\":";if(comparison.hitOwnerBlockingVolumeKnown)Bool(out,comparison.hitOwnerBlockingVolume);else out+="null";
    out+=",\"from\":";Point(out,c.cornerProfileFrom,c.cornerProfileSamples>0);
    out+=",\"to\":";Point(out,c.cornerProfileTo,c.cornerProfileSamples>0);out+='}';
    const auto&admission=c.admission;
    out+=",\"path_admission\":{\"pending\":";Bool(out,admission.pending);
    out+=",\"stage\":\"";out+=admission.stage;out+='\"';
    out+=",\"result\":";Integer(out,static_cast<uint8_t>(admission.result));
    out+=",\"path_generation\":";Integer(out,admission.generation);
    out+=",\"goal_id\":";Integer(out,admission.goal);out+=",\"node\":";Integer(out,admission.node);
    out+=",\"projected_start_arrived\":";Bool(out,admission.projectedArrived);
    out+=",\"raw_projected_start\":";Point(out,admission.rawProjection,admission.generation>0);
    out+=",\"projected_ground_known\":";Bool(out,admission.projectedGroundKnown);
    out+=",\"projected_ground\":";Point(out,admission.projectedGround,admission.projectedGroundKnown);
    out+=",\"full_connection\":";Bool(out,admission.fullConnection);
    out+=",\"ground_required\":";Bool(out,admission.groundRequired);
    out+=",\"ground_cursor\":";Integer(out,admission.groundCursor);
    out+=",\"ground_samples\":";Integer(out,admission.groundSamples);
    out+=",\"at\":";Number(out,admission.at);
    out+=",\"from\":";Point(out,admission.origin,admission.at>=0);
    out+=",\"to\":";Point(out,admission.end,admission.at>=0);
    out+=",\"sweep_from\":";Point(out,admission.sweepFrom,admission.collision.sweepKnown);
    out+=",\"sweep_to\":";Point(out,admission.sweepTo,admission.collision.sweepKnown);
    out+=",\"sweep_known\":";Bool(out,admission.collision.sweepKnown);
    out+=",\"capsule_clear\":";if(admission.collision.sweepKnown)Bool(out,admission.collision.sweepClear);else out+="null";
    out+=",\"hit_distance_cm\":";OptionalNumber(out,admission.collision.hitDistance,admission.collision.hitKnown);
    out+=",\"hit_normal\":";Point(out,admission.collision.hitNormal,admission.collision.hitKnown);
    out+=",\"hit_component_owner\":";if(admission.collision.hitActor)Integer(out,admission.collision.hitActor);else out+="null";
    out+='}';

    out+=",\"slope_probe\":{\"pending\":";Bool(out,c.slopeProbePending);
    out+=",\"at\":";Number(out,c.slopeProbeAt);out+=",\"path_generation\":";Integer(out,c.slopeProbeGeneration);
    out+=",\"node\":";Integer(out,c.slopeProbeNode);out+=",\"result\":";Integer(out,static_cast<uint8_t>(c.slopeProbeResult));
    out+=",\"from\":";Point(out,c.slopeProbeFrom,c.slopeProbeAt>=0);out+=",\"to\":";Point(out,c.slopeProbeTo,c.slopeProbeAt>=0);
    out+=",\"hit_distance_cm\":";OptionalNumber(out,c.slopeProbeCollision.hitDistance,c.slopeProbeCollision.hitKnown);
    out+=",\"hit_normal\":";Point(out,c.slopeProbeCollision.hitNormal,c.slopeProbeCollision.hitKnown);
    out+=",\"raised_result\":";Integer(out,static_cast<uint8_t>(c.slopeProbeRaisedResult));
    out+=",\"raised_to_before_lift\":";Point(out,c.slopeProbeRaisedTo,c.slopeProbeCollision.hitKnown);
    out+=",\"native_step_height_cm\":";Number(out,c.slopeProbeStep);
    out+=",\"raised_hit_distance_cm\":";OptionalNumber(out,c.slopeProbeRaisedCollision.hitDistance,c.slopeProbeRaisedCollision.hitKnown);
    out+=",\"raised_hit_normal\":";Point(out,c.slopeProbeRaisedCollision.hitNormal,c.slopeProbeRaisedCollision.hitKnown);
    out+=",\"support_known\":";Bool(out,c.slopeProbeSupportKnown);out+=",\"support_present\":";Bool(out,c.slopeProbeSupportPresent);
    out+=",\"support\":";Point(out,c.slopeProbeSupport,c.slopeProbeSupportPresent);
    out+=",\"start_support_known\":";Bool(out,c.slopeProbeStartSupportKnown);
    out+=",\"start_support_present\":";Bool(out,c.slopeProbeStartSupportPresent);
    out+=",\"start_support\":";Point(out,c.slopeProbeStartSupport,c.slopeProbeStartSupportPresent);out+='}';
    out+=",\"goal\":";Point(out,c.goal,life&&c.haveGoal);
    out+=",\"published_path_proven\":";Bool(out,f&&f->space.pathProven);out+=",\"published_route_id\":";Integer(out,f?f->space.routeId:0);
    out+=",\"published_next_point\":";Point(out,f?engine::Vec{f->space.nextPathPoint.x,f->space.nextPathPoint.y,f->space.nextPathPoint.z}:engine::Vec{},f&&f->space.pathProven);
    const auto&focus=c.routeFocus;
    const bool focusCurrent=life&&ownFresh&&profile&&nav::patrol::Current(focus,*own,static_cast<float>(arrival),
        cfg::Get(*source.profile,cfg::Param::OwnSnapshotMaxAgeMs)*.001);
    out+=",\"patrol_focus\":{\"current_clear_proof\":";Bool(out,focusCurrent);
    out+=",\"point\":";GeometryPoint(out,focus.focus,focusCurrent);
    out+=",\"proof_eye\":";GeometryPoint(out,focus.eye,focus.at>=0);
    out+=",\"proof_at\":";Number(out,focus.at);
    out+=",\"standing_eye_height_cm\":";Number(out,focus.standingEyeHeight);
    out+=",\"path_generation\":";Integer(out,focus.generation);
    out+=",\"path_at\":";Integer(out,focus.pathAt);out+=",\"visible_node\":";Integer(out,focus.node);
    out+=",\"last_ray\":\"";
    out+=focus.lastRay==nav::patrol::Ray::Clear?"clear":focus.lastRay==nav::patrol::Ray::Blocked?"blocked":focus.lastRay==nav::patrol::Ray::Deferred?"budget_deferred":"unknown";
    out+="\",\"static_path_only\":true,\"grants_fire\":false}";
    out+=",\"metrics_known\":";Bool(out,metrics);
    for(const auto&value:std::array<std::pair<const char*,double>,6>{{{"distance_3d_cm",distance},{"distance_xy_cm",horizontal},{"absolute_dz_cm",vertical},{"arrival_cm",arrival},{"search_step_cm",search},{"brain_flat_step_cm",step}}}){
        out+=",\"";out+=value.first;out+="\":";if(metrics)Number(out,value.second);else out+="null";
    }
    out+=",\"predicates_are_gate_inputs_not_executed_actions\":true,\"euclidean_arrived\":";Bool(out,metrics&&distance<=arrival);
    out+=",\"horizontal_arrived\":";Bool(out,metrics&&horizontal<=arrival);out+=",\"vertical_within_arrival\":";Bool(out,metrics&&vertical<=arrival);
    out+=",\"small_height_arrival_disagreement\":";Bool(out,metrics&&horizontal<=arrival&&vertical<=arrival&&distance>arrival);
    out+=",\"large_vertical_disagreement\":";Bool(out,metrics&&horizontal<=arrival&&vertical>arrival);
    out+=",\"brain_flat_step_below_arrival\":";Bool(out,metrics&&step<=arrival);
    out+=",\"recovery\":{\"path_generation\":";Integer(out,c.pathGeneration);
    out+=",\"active_navigation_seconds\":";Number(out,c.progress.activeSeconds);
    out+=",\"previous_navigation_demand\":";Bool(out,c.progress.previousDemand);
    out+=",\"eligible_intervals\":";Integer(out,c.progress.eligibleIntervals);
    out+=",\"stalls\":";Integer(out,c.progress.stalls);
    out+=",\"replans\":";Integer(out,c.recoveryReplans);
    out+=",\"goal_changes\":";Integer(out,c.recoveryGoalChanges);
    out+=",\"current_goal_query_failures\":";Integer(out,c.pathQueryFailures);
    out+=",\"replan_pending_confirmation\":";Bool(out,c.recoveryRetried);
    out+=",\"avoided_static_goal_id\":";Integer(out,c.failedGoalId);
    out+=",\"per_intent_consumption_proven\":false}}";
}
template<class Reader>inline void Cache(double now,size_t profileCount,Reader reader){
    if(!original_players::Thread())return;
    std::string next="{\"scope\":\"cached_own_static_navigation_full_bridge_only\",\"cached\":true,\"not_in_mmf\":true,\"cache_at\":";Number(next,now);
    next+=",\"profile_count\":";Integer(next,profileCount);next+=",\"bots\":[";bool comma=false;
    for(size_t n=1;n<(std::min)(profileCount,nav::MaxPlayers);++n){if(comma)next+=',';comma=true;AppendBot(next,n,reader(n),now);}
    next+="]}";AcquireSRWLockExclusive(&lock);cached.swap(next);ReleaseSRWLockExclusive(&lock);
}
inline std::string Json(){AcquireSRWLockShared(&lock);const auto result=cached;ReleaseSRWLockShared(&lock);return result;}
}
