#pragma once
#include "humanlike_audio_observation_core.h"
#include <cmath>
#include <cstdint>

// Proof of the pinned original Tick's commit AFTER its actual ground request.
// A moving pawn or nonzero velocity alone cannot satisfy any of these gates.
namespace humanlike_audio_tick_core {
namespace observation=humanlike_audio_observation;
struct Commit {
    float before=-3.402823466e+38f,after=-3.402823466e+38f;
    bool originalTickCompleted=false,originalBodyProfile=false,noNestedTick=false;
    bool beforeStampKnown=false,afterStampKnown=false;
};
struct Receipt {
    observation::Receipt request{};Commit stamp{};
    bool changed=false,savedOriginalWorldTime=false,provenNativeRequest=false;
};
inline Receipt Evaluate(uint64_t occurrence,const observation::Sample&before,
                        const observation::Sample&after,const Commit&commit){
    Receipt r{};r.stamp=commit;
    r.changed=commit.beforeStampKnown&&commit.afterStampKnown&&std::isfinite(commit.before)&&std::isfinite(commit.after)&&commit.before!=commit.after;
    // Original6f33a66 saves World+740 before the call. Only after return does
    // a86 convert THAT saved double to float, and a92 write component+DC.
    r.savedOriginalWorldTime=before.nativeWorldTimeKnown&&after.nativeWorldTimeKnown
        &&std::isfinite(before.nativeWorldSeconds)&&std::isfinite(after.nativeWorldSeconds)
        &&before.nativeWorldSeconds==after.nativeWorldSeconds
        &&static_cast<float>(before.nativeWorldSeconds)==commit.after;
    r.provenNativeRequest=commit.originalTickCompleted&&commit.originalBodyProfile&&commit.noNestedTick
        &&r.changed&&r.savedOriginalWorldTime&&before.request==observation::Request::Footstep;
    r.request=observation::Evaluate(occurrence,before,after,r.provenNativeRequest);
    if(r.request.requestEligible){
        // The commit is observed at Tick return. Use that measured match time
        // conservatively; the exact original world stamp remains opaque and
        // is never converted to a guessed wall-clock event instant.
        r.request.event.atMatchSeconds=after.atMatchSeconds;
        // Independent source namespace. The ring's own monotonically growing
        // event serial remains authoritative for sensor cursors.
        r.request.event.token=occurrence|0x4000000000000000ULL;
        r.request.event.origin=0x6f33a6e; // actual pinned original CALL site, not a playing ID
    }
    return r;
}
} // namespace humanlike_audio_tick_core
