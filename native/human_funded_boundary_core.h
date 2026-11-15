#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>

// Diagnostic arithmetic only. There is no offset, publication or clock API.
namespace human_funded_boundary_core {
inline constexpr uint32_t MaxExactIndex=0x00fffffeU;
enum class Phase:uint8_t {FirstAuditedFundedPoll,BeforeFirstPrivateSeed,AfterFirstAcceptedPacket,BeforeFirstNativeAdvance,Count};
inline const char* Name(Phase p){switch(p){
    case Phase::FirstAuditedFundedPoll:return "first_audited_funded_poll";
    case Phase::BeforeFirstPrivateSeed:return "before_first_original_private_seed";
    case Phase::AfterFirstAcceptedPacket:return "after_first_accepted_packet";
    case Phase::BeforeFirstNativeAdvance:return "before_first_native_advance_after_supply";
    case Phase::Count:return "invalid_phase";
    }return "invalid_phase";}
inline uint32_t Bits(float value){uint32_t bits=0;std::memcpy(&bits,&value,4);return bits;}
struct ArithmeticInput {
    bool sameThread=false,ownedTuple=false,sourceVerified=false,samePoint=false,actualFunding=false;
    uint32_t cBefore=0,cAfter=0,rawBefore=0,rawAfter=0,step=0;
    int32_t actualOffset=0;float aBefore=0,aAfter=0;
};
struct Arithmetic {
    bool valid=false;uint32_t wholeFundedSteps=0,originCandidate=0;float remainder=0;
    const char* reason="not_observed";
};
inline Arithmetic Diagnose(const ArithmeticInput& in){
    Arithmetic out{};
    if(!in.sameThread){out.reason="not_same_owned_game_thread";return out;}
    if(!in.ownedTuple){out.reason="current_owned_life_tuple_not_verified";return out;}
    if(!in.sourceVerified){out.reason="original_mode_source_not_verified";return out;}
    if(!in.samePoint||in.cBefore!=in.cAfter||Bits(in.aBefore)!=Bits(in.aAfter)||in.rawBefore!=in.rawAfter){out.reason="synchronous_point_changed_during_readback";return out;}
    if(in.rawBefore||in.step!=1||in.actualOffset){out.reason="actual_human_clock_not_fresh_neutral_single_step";return out;}
    if(!in.actualFunding){out.reason="no_actual_native_funding_receipt";return out;}
    if(!std::isfinite(in.aBefore)||in.aBefore<0||in.aBefore>0.25f){out.reason="actual_native_accumulator_outside_startup_window";return out;}
    // A is the real funded native accumulator, not elapsed wall time and not
    // executable credit in the presence of the original TimeShift monitor.
    out.wholeFundedSteps=static_cast<uint32_t>(in.aBefore*128.0f);
    if(in.cBefore>MaxExactIndex||in.cBefore<out.wholeFundedSteps){out.reason="original_source_index_cannot_cover_funded_window";return out;}
    out.originCandidate=in.cBefore-out.wholeFundedSteps;
    out.remainder=in.aBefore-float(out.wholeFundedSteps)*(1.0f/128.0f);
    out.valid=true;out.reason="diagnostic_only_C_minus_funded_whole_steps_no_policy_applied";return out;
}
} // namespace human_funded_boundary_core
