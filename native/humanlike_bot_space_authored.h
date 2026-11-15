#pragma once
#include "humanlike_bot_space_data.h"
#include <array>
// Generated map content, not player heat-map facts or runtime proof.
// Source/resolved SHA256: 8e8383a7303862b0b94d055072837bc3f27a0088916c20cbda1a1df1ebe562fc
namespace humanlike_bot_space_data {
inline constexpr uint32_t AuthoredRevision=2U;
inline constexpr std::array<Annotation,4> Authored=[]{
    std::array<Annotation,4> rows{};
    {auto&r=rows[0];
        r.id=1301U;const char map[]="ascent";for(size_t n=0;n<sizeof(map);++n)r.mapId[n]=map[n];
        r.kind=Kind::Hold;
        r.start={6020.12305f,-7653.41797f,502.299011f};
        r.open={6006.34717f,-7643.25391f,502.299011f};
        r.retreat={6020.12305f,-7653.41797f,502.299011f};
        r.focus={5828.23682f,-7884.66016f,662.299011f};
        r.focusKnown=true;r.enabled=false;r.conventional=true;
        r.range=humanlike_bot::HoldRange::Unknown;r.authoredConfidence=0.5f;
        r.riskKnown=false;r.authoredRisk=1.0f;
        r.flankRiskKnown=false;r.authoredFlankRisk=1.0f;
        r.exposedAnglesKnown=false;r.authoredExposedAngles=0U;}
    {auto&r=rows[1];
        r.id=1302U;const char map[]="ascent";for(size_t n=0;n<sizeof(map);++n)r.mapId[n]=map[n];
        r.kind=Kind::Hold;
        r.start={-25.6620007f,-7648.50781f,302.298004f};
        r.open={10.2729998f,-7318.229f,302.298004f};
        r.retreat={-25.6620007f,-7648.50781f,302.298004f};
        r.focus={-287.96701f,-7285.77979f,462.298004f};
        r.focusKnown=true;r.enabled=false;r.conventional=false;
        r.range=humanlike_bot::HoldRange::Unknown;r.authoredConfidence=0.5f;
        r.riskKnown=false;r.authoredRisk=1.0f;
        r.flankRiskKnown=false;r.authoredFlankRisk=1.0f;
        r.exposedAnglesKnown=false;r.authoredExposedAngles=0U;}
    {auto&r=rows[2];
        r.id=1501U;const char map[]="ascent";for(size_t n=0;n<sizeof(map);++n)r.mapId[n]=map[n];
        r.kind=Kind::Hold;
        r.start={4720.66846f,-4450.54443f,402.298492f};
        r.open={4448.90137f,-4893.87598f,402.298492f};
        r.retreat={4720.66846f,-4450.54443f,402.298492f};
        r.focus={4593.63721f,-4372.67285f,576.298462f};
        r.focusKnown=true;r.enabled=true;r.conventional=true;
        r.range=humanlike_bot::HoldRange::Unknown;r.authoredConfidence=0.800000012f;
        r.riskKnown=true;r.authoredRisk=0.25f;
        r.flankRiskKnown=true;r.authoredFlankRisk=0.0f;
        r.exposedAnglesKnown=true;r.authoredExposedAngles=1U;}
    {auto&r=rows[3];
        r.id=1502U;const char map[]="ascent";for(size_t n=0;n<sizeof(map);++n)r.mapId[n]=map[n];
        r.kind=Kind::Hold;
        r.start={-2710.22266f,-3945.10059f,102.15004f};
        r.open={-2417.27637f,-4374.73096f,102.15004f};
        r.retreat={-2710.22266f,-3945.10059f,102.15004f};
        r.focus={-2587.11694f,-3861.16016f,276.150024f};
        r.focusKnown=true;r.enabled=true;r.conventional=false;
        r.range=humanlike_bot::HoldRange::Unknown;r.authoredConfidence=0.800000012f;
        r.riskKnown=true;r.authoredRisk=0.75f;
        r.flankRiskKnown=true;r.authoredFlankRisk=0.666666687f;
        r.exposedAnglesKnown=true;r.authoredExposedAngles=3U;}
    return rows;
}();
} // namespace humanlike_bot_space_data
