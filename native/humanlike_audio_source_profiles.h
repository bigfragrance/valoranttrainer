#pragma once
#include <cstdint>
// Generated from fixed PE SHA256 147763350504d241ee20b77607d04bc85cebfedb1d8bc2f32473ca93bf8fe92f
namespace humanlike_audio_source_profiles {
struct Profile {uint32_t rva,bytes;uint64_t fnv;};
inline constexpr Profile Profiles[]{
    {0x685cac0,28,0xaff09a3b7d0c5d07ULL}, // FootstepsComponent.execPlayFootstepSound
    {0x685be40,28,0x6d6b6b53eaeac3efULL}, // FootstepsComponent.execPlayJumpLandingSound
    {0x685be60,28,0xe5577bb929d4ed20ULL}, // FootstepsComponent.execPlayJumpTakeoffSound
    {0x6f33150,1650,0x528ab2c6bb922ffULL}, // FootstepsComponent.PlayCharacterGroundSound
    {0x1ce1dc0,20,0x69d3dd0ac834874dULL}, // VM.execFinalFunction
    {0x1cdb350,713,0xc227107fae939301ULL}, // UObject.CallFunction_native_dispatch
    {0x1b9c540,164,0x886e7968d8ef044ULL}, // UFunction.Invoke
    {0x685b1f0,174,0xb61e0fd177b0c407ULL}, // FootstepsComponent.class_getter
    {0x787b6c0,99,0x2fa4f38ac8e06b6aULL}, // GroundOrigin.typed_character_movement_query
    {0x4cd7160,48,0x17b67eea01bd69f2ULL}, // GroundOrigin.original_world_transport_predicate
    {0x2a64ee0,132,0x83dfe973e78a6846ULL}, // original_GetWorld_virtual_0x2a64ee0
    {0x3ed83b0,1609,0x15d2d4ecc2f49cf7ULL}, // original_GetWorld_virtual_0x3ed83b0
    {0xc8d1f10,8,0xa81700322783a8b1ULL}, // original_ground_request_Z_offset_double20
};
} // namespace humanlike_audio_source_profiles
