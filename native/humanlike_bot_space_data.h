#pragma once
#include "humanlike_bot_types.h"
#include <cstddef>
#include <cstdint>

namespace humanlike_bot_space_data {
inline constexpr size_t Capacity=256;
enum class Kind:uint8_t {Cover,Edge,Search,Hold};
struct Annotation {
    uint32_t id=0;char mapId[64]{};Kind kind=Kind::Cover;
    humanlike_bot::Vec3 start{},open{},retreat{},focus{};
    bool focusKnown=false,enabled=false,conventional=true;
    float authoredConfidence=0,authoredRisk=1;
    humanlike_bot::HoldRange range=humanlike_bot::HoldRange::Unknown;
    bool riskKnown=false,flankRiskKnown=false,exposedAnglesKnown=false;
    float authoredFlankRisk=1;uint32_t authoredExposedAngles=0;
};
// Coordinates/author labels are map content, not bot ability defaults. Every
// loaded record remains unverified until native world/path queries admit it.
} // namespace humanlike_bot_space_data
