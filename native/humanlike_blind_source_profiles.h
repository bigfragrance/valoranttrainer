#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace humanlike_blind_source_profiles {
struct Span {uintptr_t rva;size_t bytes;uint64_t hash;};
inline constexpr std::array<Span,8> Spans{{
    {0x65b54e0,66,0x46c5dc55d2c1e602ULL},
    {0x65b3db0,174,0xa28c929518b8ae6ULL},
    {0x2a64ee0,28,0x150c6babc8428900ULL},
    {0x2a64efc,98,0x82f6847ffee8b7ecULL},
    {0x2a64f5e,6,0xad7a3baae1c89672ULL},
    {0x3ed83b0,60,0xb0370ccdf6164999ULL},
    {0x3ed83ec,1515,0xeebf5d5f356d29e0ULL},
    {0x3ed89d7,34,0x3250c3a3c23e774eULL},
}};
} // namespace humanlike_blind_source_profiles
