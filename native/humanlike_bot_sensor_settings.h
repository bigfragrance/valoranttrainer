#pragma once
#include "humanlike_bot_config.h"
#include "humanlike_bot_sensors_core.h"

namespace humanlike_bot_sensors {
inline Settings SettingsFromProfile(const humanlike_bot_config::Profile& p){
    using P=humanlike_bot_config::Param;
    const auto get=[&](P key){return humanlike_bot_config::Get(p,key);};
    const auto seconds=[&](P key){return static_cast<double>(get(key))/1000.;};
    Settings s{};
    s.horizontalFov=get(P::VisualFovHorizontal);s.verticalFov=get(P::VisualFovVertical);
    s.peripheralEdgeDegrees=get(P::VisualPeripheralEdgeDeg);s.maxDistanceCm=get(P::VisualMaxDistanceCm);
    s.centralGain=get(P::VisualCenterGain);s.peripheralGain=get(P::VisualEdgeGain);s.distancePenalty=get(P::VisualDistancePenalty);
    s.minimumVisibleSampleFraction=get(P::VisualMinimumExposure); // unique point ratio, never model/rendered area
    s.visualSampleSeconds=seconds(P::VisualSampleIntervalMs);s.recognitionSeconds=seconds(P::VisualRecognitionMs);
    s.exposureSeconds=seconds(P::VisualExposureMs);s.maxSampleGapSeconds=seconds(P::VisualMaxSampleGapMs);
    s.missProbability=get(P::VisualMissProbability);s.peripheralMissProbability=get(P::VisualPeripheralMissProbability);
    s.pointSamples=static_cast<uint32_t>(get(P::VisualPointCount));s.minVisiblePoints=static_cast<uint32_t>(get(P::VisualMinimumVisiblePoints));
    s.targetsPerSample=static_cast<uint32_t>(get(P::VisualTargetQueriesPerBot));
    s.chestHeightFraction=get(P::VisualChestHeightFraction);s.shoulderHeightFraction=get(P::VisualShoulderHeightFraction);
    s.shoulderRadiusScale=get(P::VisualShoulderRadiusScale);
    s.memoryTtlSeconds=seconds(P::MemoryDurationMs);s.memoryHalfLifeSeconds=seconds(P::MemoryConfidenceHalfLifeMs);
    s.hearingGunshotEnabled=humanlike_bot_config::Enabled(p,P::HearingGunshotEnabled);
    s.hearingFootstepEnabled=humanlike_bot_config::Enabled(p,P::HearingFootstepEnabled);
    s.hearingLandingEnabled=humanlike_bot_config::Enabled(p,P::HearingLandingEnabled);
    s.hearingFootstepRangeCm=get(P::HearingFootstepRangeCm);s.hearingLandingRangeCm=get(P::HearingLandingRangeCm);
    s.hearingRangeCm=get(P::HearingGunshotRangeCm);s.hearingOcclusionScale=get(P::HearingOcclusionFactor);
    s.hearingBearingError=get(P::HearingBearingErrorDeg);s.hearingDistanceError=get(P::HearingDistanceErrorRatio);
    s.hearingDistanceBandCm=get(P::HearingDistanceBandCm);s.hearingDelaySeconds=seconds(P::HearingDelayMs);
    s.hearingTtlSeconds=seconds(P::HearingStimulusTtlMs);s.hearingMissProbability=get(P::HearingMissProbability);
    s.hearingWrongDirectionProbability=get(P::HearingFalseDirectionProbability);
    s.hurtBearingError=get(P::HurtDirectionErrorDeg);s.hurtWrongDirectionProbability=get(P::HurtDirectionWrongProbability);
    s.eventBudget=static_cast<uint32_t>(get(P::SensorEventBudgetPerBot));s.ownShotMaxAgeSeconds=seconds(P::ShotReceiptMaxAgeMs);return s;
}
inline Budget BudgetFromProfile(const humanlike_bot_config::Profile& global){
    using P=humanlike_bot_config::Param;
    return {static_cast<uint32_t>(humanlike_bot_config::Get(global,P::VisualGlobalRayBudget)),
            static_cast<uint32_t>(MaxPlayers-1),
            static_cast<uint32_t>(humanlike_bot_config::Get(global,P::SensorHearingRayReserve))};
}
} // namespace humanlike_bot_sensors
