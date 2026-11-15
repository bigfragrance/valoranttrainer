#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

// Pinned original EX_FinalFunction -> UFunction.Invoke -> native audio query.
// Evidence: work/original-npc-audio-routing/frame-recovery. No target code edits.
namespace original_audio_spectated_consumers {
struct Profile {const char* name;const wchar_t* path;const wchar_t* owner;int32_t scriptBytes;uint32_t queryResume;bool staticContext;};
inline constexpr std::array<Profile,3> Profiles{{
    {"Aud_GetHitConfirmLocality",L"/Game/Audio/Core/Aud_FunctionLibrary.Aud_FunctionLibrary_C.Aud_GetHitConfirmLocality",L"/Game/Audio/Core/Aud_FunctionLibrary.Aud_FunctionLibrary_C",312,51,true},
    {"TriggerWorldImpactEffectSound",L"/Game/Audio/Core/AudioHandlerActor.AudioHandlerActor_C.TriggerWorldImpactEffectSound",L"/Game/Audio/Core/AudioHandlerActor.AudioHandlerActor_C",812,127,false},
    {"GetPlayOnStartEvent",L"/Game/Audio/Core/Comp_FXC_AudioBasic_PerspectiveEvents_ContextTeam.Comp_FXC_AudioBasic_PerspectiveEvents_ContextTeam_C.GetPlayOnStartEvent",L"/Game/Audio/Core/Comp_FXC_AudioBasic_PerspectiveEvents_ContextTeam.Comp_FXC_AudioBasic_PerspectiveEvents_ContextTeam_C",731,130,false},
}};
struct NativeProfile {uintptr_t rva;size_t bytes;uint64_t fnv;};
inline constexpr std::array<NativeProfile,8> NativeProfiles{{
    {0x1b9c540,164,0x886e7968d8ef044ULL},
    {0x6c2ced0,318,0xa1390600925cca43ULL},
    {0x786b550,190,0x21341cfa0e3ffed6ULL},
    {0x48548e0,50,0x5598334b3612dde7ULL},
    {0x1cdb350,713,0xc227107fae939301ULL},
    {0x1ce1dc0,20,0x69d3dd0ac834874dULL},
    {0x8e7b4b,14,0x14976b5b1d958887ULL},
    {0x1cdcad7,44,0xaa66e6804fba0829ULL},
}};
}

