#pragma once
#include <cstdint>
#include <cstddef>
namespace local_server_phoenix_c_fire_gate_source {
struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
inline constexpr uintptr_t ImageBytes=0xe69c000;
inline constexpr Profile Profiles[]{
    {0x7139c50,246,0xd256afa16847671eULL},
    {0x713a6e0,104,0x8c38887b59608f13ULL},
    {0x713a750,810,0x7ca7cd9093b06517ULL},
    {0x2a64ee0,28,0x150c6babc8428900ULL},
    {0x2a64efc,98,0x82f6847ffee8b7ecULL},
    {0x2a64f5e,6,0xad7a3baae1c89672ULL},
    {0xc8d2ccc,4,0x4b72c77f9c5d0918ULL},
};
}
