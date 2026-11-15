#pragma once
#include "original_client_lifecycle.h"
#include "original_inventory_slots.h"
#include "original_audio_view_consumers.h"
#include "original_audio_spectated_consumers.h"
#include <atomic>
#include <intrin.h>

// Pinned shipped client: Pawn.v830 -> Controller.v800. The stock PC predicate
// deliberately returns true for every Standalone PC, including authority NPCs
// without a UPlayer. This instance adapter supplies the missing local roster
// distinction ONLY inside verified Wwise/ClientPlayAimpunch view call chains
// or verified pure audio/audio-minimap Blueprint query frames. Engine local-authority simulation
// semantics remain unchanged outside these selected view consumers.
// A separate original audio-minimap component instance adapter rejects only Add
// visualization events for NPC view ownership; all removal/destruction survives.
// No image page, local cache, possession, role, net mode, input or audio value
// is written. All remaining original virtuals, including destruction, survive.
namespace original_npc_locality {
inline constexpr uintptr_t NativeTableRva=0xb50b540,NativePredicateRva=0x4929b30;
inline constexpr size_t TableEntries=623,PredicateSlot=0x800/8,MaxPlayers=original_players::MaxPlayers;
using WeakIdentity=original_inventory_slots::WeakIdentity;
using Predicate=bool(*)(void*);
// Filled from the pinned PE, not inferred from a live object's adjacent memory.
inline constexpr uint32_t NativeSlots[TableEntries]{
    0x6cbaa40U,0x18a3a00U,0x1cfa620U,0x1af09d0U,0x18a76a0U,0x3ed82e0U,0x1d06c40U,0x18a3a00U,0x42d2880U,0x42d2890U,
    0x3ed7050U,0x1af0aa0U,0x1af0a90U,0x1af0ab0U,0x3ed8280U,0x18a3a00U,0x18a3a00U,0x3ed83a0U,0x18a76a0U,0x3ed83a0U,
    0x18a3a00U,0x36afd10U,0x18a3a00U,0x18a76a0U,0x18a3ec0U,0x492f2f0U,0x3ed9c00U,0x3ed9030U,0x36c7460U,0x1c53840U,
    0x3ef6ca0U,0x3ed9770U,0x3ed98d0U,0x18a3a00U,0x18a3a00U,0x3f00330U,0x18a3a00U,0x1af0b30U,0x18a3a00U,0x1c53720U,
    0x1c53700U,0x18a3ec0U,0x18a76a0U,0x3ed82f0U,0x18a76a0U,0x18a76a0U,0x18a76a0U,0x1c53be0U,0x18a3a00U,0x18a3a00U,
    0x18a3a00U,0x18a3a00U,0x18a3a00U,0x3eda8f0U,0x1af0b50U,0x3ed83b0U,0x18a76a0U,0x1c562c0U,0x1af0b70U,0x18a3ed0U,
    0x18a3ec0U,0x1c554e0U,0x18a3a00U,0x3ed8300U,0x1c56490U,0x1c56550U,0x1c56640U,0x1c56670U,0x78b13e0U,0x3f06af0U,
    0x3f07590U,0x3f07520U,0x1c5dc90U,0x18a3ec0U,0x3f07140U,0x3f050b0U,0x3f050f0U,0x18a3a00U,0x18a3a00U,0x1c54130U,
    0x18a3ed0U,0x18a3a00U,0x18a3a00U,0x3eda0d0U,0x3efc9b0U,0x3efd360U,0x18a76a0U,0x18a3ed0U,0x18a3a00U,0x3ed7960U,
    0x6cba740U,0x18a3a00U,0x18dd100U,0x18a3a00U,0x18a3a00U,0x3f01d20U,0x3ee1720U,0x18a3ec0U,0x3ede430U,0x18a3a00U,
    0x3ef3450U,0x3efa480U,0x3efa7a0U,0x3ede560U,0x18a76a0U,0x18a3a00U,0x3f06b50U,0x3f073b0U,0x3f07480U,0x3edac90U,
    0x3edb100U,0x18a3a00U,0x4941370U,0x3efacb0U,0x49413a0U,0x3ec5810U,0x3ec57c0U,0x3efbce0U,0x18a3a00U,0x3ed8fb0U,
    0x3ed8fd0U,0x3ed8ff0U,0x3ed9010U,0x78adc40U,0x78aea10U,0x3edd790U,0x3edd7f0U,0x3edd850U,0x3edd8b0U,0x3edd910U,
    0x3edda40U,0x3eddb70U,0x3eddbd0U,0x3eddc30U,0x3eddc90U,0x3eddcf0U,0x3effdb0U,0x3f00160U,0x18a76a0U,0x3f05e10U,
    0x2a652f0U,0x3eda180U,0x3690180U,0x3f00a50U,0x3f00b20U,0x3f00d30U,0x4929a60U,0x3f04f60U,0x18a76a0U,0x4930e40U,
    0x4930f30U,0x4930f40U,0x4930f90U,0x3eda8c0U,0x78b22d0U,0x4944200U,0x18a3a00U,0x3ee3d10U,0x18a3a00U,0x3efa6a0U,
    0x3f051e0U,0x3f052e0U,0x18a3a00U,0x3f056f0U,0x3f05750U,0x3ede1d0U,0x3edc1a0U,0x3edee80U,0x3edab80U,0x18a76a0U,
    0x493edc0U,0x3f05df0U,0x3edef40U,0x50d6060U,0x78b0c80U,0x3efdad0U,0x18eabb0U,0x36a7fe0U,0x4929a80U,0x4929aa0U,
    0x18a76a0U,0x3efde00U,0x3efde70U,0x3efdf10U,0x3efdfd0U,0x3efe5c0U,0x18a3a00U,0x3ee6350U,0x3efe400U,0x3efe590U,
    0x3eff7d0U,0x2a65370U,0x3efea40U,0x18a3a00U,0x3ec57b0U,0x18a3a00U,0x3ed8aa0U,0x3edb330U,0x3edb2e0U,0x18a3a00U,
    0x4943ee0U,0x3f07860U,0x3eda4e0U,0x18a3ec0U,0x7818760U,0x18a3a00U,0x3ee3d10U,0x3edb350U,0x3edb420U,0x3edb120U,
    0x3edb210U,0x3edb2a0U,0x18a3a00U,0x3ee4650U,0x18a3ec0U,0x3ee20d0U,0x3ee2670U,0x2781760U,0x3ee3d20U,0x3ee3d80U,
    0x7820fe0U,0x3ee3f50U,0x3ee4190U,0x42af610U,0x4930540U,0x3ede1c0U,0x3ee43e0U,0x3edef50U,0x3ee1030U,0x493af00U,
    0x3edeed0U,0x18a3a00U,0x3ee57b0U,0x3ee5d60U,0x3f06d30U,0x18a3ec0U,0x4938fd0U,0x18a3ec0U,0x42aeb20U,0x18a3ec0U,
    0x42ae970U,0x42ad5d0U,0x42ad710U,0x42aec00U,0x42aeb90U,0x49400f0U,0x42ad260U,0x781fc90U,0x4932b20U,0x42ad800U,
    0x78b1670U,0x78b17f0U,0x78b1fe0U,0x78b20e0U,0x492d2b0U,0x42af370U,0x4929b30U,0x4930df0U,0x42adf30U,0x42ae750U,
    0x78b2220U,0x4930b40U,0x42ae870U,0x78b2550U,0x42af0b0U,0x4933e80U,0x7820f50U,0x78b0d50U,0x42afa60U,0x42ad4f0U,
    0x42ad520U,0x42ad530U,0x42ad540U,0x42ad570U,0x42ad580U,0x4937a70U,0x18a3a00U,0x18a3ec0U,0x781e5d0U,0x18a3a00U,
    0x49441b0U,0x49440c0U,0x4944000U,0x49329e0U,0x49329c0U,0x49325e0U,0x18a3a00U,0x18a3ec0U,0x4931ae0U,0x781b5c0U,
    0x18a3ec0U,0x4938b50U,0x18a3ec0U,0x4938850U,0x18a3ec0U,0x49387d0U,0x18a3ec0U,0x4937d20U,0x18a3ec0U,0x4932c30U,
    0x18a3ec0U,0x492faf0U,0x18a3ec0U,0x4938450U,0x18a3ec0U,0x4938420U,0x18a3ec0U,0x4938540U,0x18a3ec0U,0x4938e10U,
    0x4933940U,0x49339c0U,0x4933940U,0x4933990U,0x4933940U,0x4933960U,0x4933ba0U,0x4933b40U,0x4932110U,0x49320e0U,
    0x4930870U,0x4931010U,0x492e1c0U,0x493bef0U,0x493be20U,0x18a3a00U,0x492ba30U,0x492a3e0U,0x493ef10U,0x493c430U,
    0x493c2a0U,0x493de70U,0x493df90U,0x493df10U,0x493de90U,0x4937c30U,0x4930c10U,0x493c1c0U,0x4939ab0U,0x18a3a00U,
    0x4932210U,0x493ad40U,0x492e5c0U,0x4930690U,0x493c230U,0x493a420U,0x4931260U,0x4931180U,0x493de20U,0x49312e0U,
    0x4938f90U,0x4938fb0U,0x4930790U,0x4933f40U,0x4939ba0U,0x493a8e0U,0x4932a00U,0x493ad20U,0x493b990U,0x493bb60U,
    0x493bc60U,0x493bb60U,0x493bae0U,0x493b830U,0x493b6d0U,0x493b4b0U,0x493b570U,0x493b4b0U,0x493b350U,0x493b340U,
    0x492bad0U,0x493b260U,0x4941fd0U,0x4933030U,0x18a3ec0U,0x18a3a00U,0x4938770U,0x18a3ec0U,0x4938770U,0x18a3ec0U,
    0x7824b60U,0x492fd30U,0x18a3a00U,0x4940790U,0x4931f80U,0x4932e50U,0x4932fe0U,0x781e5c0U,0x4925680U,0x49336a0U,
    0x4933820U,0x4933880U,0x49338e0U,0x4933a50U,0x4933be0U,0x4933c30U,0x50d6e90U,0x18a3a00U,0x78b0f10U,0x7818cb0U,
    0x4937b10U,0x18a3ec0U,0x78b2970U,0x42d28a0U,0x42d28b0U,0x42d28d0U,0x3804f00U,0x493af00U,0x493af20U,0x493b040U,
    0x18a3a00U,0x493f770U,0x4938c10U,0x493a580U,0x493ad60U,0x493adf0U,0x493ae10U,0x493aeb0U,0x49252e0U,0x493b280U,
    0x4925f80U,0x4926120U,0x49263a0U,0x49254c0U,0x4925ad0U,0x4925fe0U,0x4926260U,0x4925c10U,0x493bce0U,0x493bd10U,
    0x1d9aae0U,0x18a3a00U,0x18a3a00U,0x4925220U,0x493d190U,0x492df50U,0x4938470U,0x781f7b0U,0x781f690U,0x4940c90U,
    0x49413d0U,0x4941600U,0x781d900U,0x4932070U,0x4937bd0U,0x49407e0U,0x493fcd0U,0x49302f0U,0x4940960U,0x4940a80U,
    0x4940ae0U,0x4935060U,0x42d2900U,0x4935200U,0x4935080U,0x4936350U,0x4936230U,0x49372c0U,0x78b2d60U,0x1af0b50U,
    0x7820d40U,0x7820d00U,0x18a3a00U,0x18a3a00U,0x18a3a00U,0x7820c90U,0x4937f80U,0x4938370U,0x78b26a0U,0x4937880U,
    0x4937510U,0x493ddc0U,0x493e520U,0x493e730U,0x492d420U,0x78af080U,0x492d1a0U,0x18a3a00U,0x492fd50U,0x492fef0U,
    0x49303f0U,0x492efc0U,0x18a3a00U,0x4932a90U,0x49388d0U,0x4938ae0U,0x4938f40U,0x4941c70U,0x4941c90U,0x18a3a00U,
    0x4938dc0U,0x493f6d0U,0x493f640U,0x7821050U,0x492d330U,0x4937340U,0x4937350U,0x7821170U,0x42d2910U,0x493dfb0U,
    0x492f250U,0x18a3a00U,0x18a3a00U,0x493f810U,0x493f920U,0x493fc00U,0x18a76a0U,0x4932ab0U,0x50d6c60U,0x781cbd0U,
    0x78b0cf0U,0x4931fa0U,0x4932150U,0x4932180U,0x18a3a00U,0x781fb40U,0x78b2a00U,0x4944710U,0x18a3a00U,0x7824b70U,
    0x18a3a00U,0x18a3a00U,0x7824940U,0x7821220U,0x18a3a00U,0x781fd70U,0x781fd60U,0x7822b70U,0x7822af0U,0x7822a50U,
    0x18a3a00U,0x7822c10U,0x18a76a0U,0x18a3a00U,0x18a76a0U,0x18a3a00U,0x18a3a00U,0x18a3a00U,0x18a3a00U,0x18a3a00U,
    0x18a3a00U,0x18a3a00U,0x18a3a00U,0x7819a40U,0x18a3a00U,0x18a3a00U,0x781d230U,0x781d230U,0x781afc0U,0x781af80U,
    0x781aee0U,0x781adf0U,0x781ae00U,0x781aca0U,0x781ab60U,0x781ab20U,0x18a3a00U,0x18a3a00U,0x18a3a00U,0x18a3a00U,
    0x18a3a00U,0x18a76a0U,0x18a3a00U,0x781b070U,0x781b050U,0x6f781e0U,0x78197d0U,0x78198a0U,0x7819970U,0x781aa50U,
    0x64e74e0U,0x18a76a0U,0x781a0b0U,0x781a200U,0x781a360U,0x781a4b0U,0x781a650U,0x781a7d0U,0x78b14f0U,0x78af4f0U,
    0x18a3a00U,0x18a3a00U,0x18a3a00U,0x18a76a0U,0x7822e60U,0x18a3ed0U,0x18a3a00U,0x18a3a00U,0x7818690U,0x18d4100U,
    0x781e610U,0x7821160U,0x18a3a00U,0x78b3f10U,0x78b2f20U,0x78b6430U,0x18a76a0U,0x18a76a0U,0x7822d80U,0x78b2e10U,
    0x78b0240U,0x78b00c0U,0x18a3a00U,0x18a3a00U,0x78b2da0U,0x78b2d70U,0x18a3a00U,0x18a3a00U,0x78b26f0U,0x18a3a00U,
    0x18a3a00U,0x18a3a00U,0x18a3a00U,0x18a3a00U,0x18a3a00U,0x78b1350U,0x78b2520U,0x18a3a00U,0x18a3a00U,0x18a3a00U,
    0x18a3a00U,0x18a3a00U,0x18a3a00U,
};
inline constexpr uintptr_t NativePrefixRva=0x6c8e950;
inline constexpr uintptr_t AttenuationTableRva=0xb0aa830,AttenuationPrefixRva=0x3c48b10;
inline constexpr size_t AttenuationTableEntries=206,MaxAttenuations=256;
inline constexpr uint32_t AttenuationSlots[AttenuationTableEntries]{
    0x64bc3a0U,0x18a3a00U,0x1cfa620U,0x1af09d0U,0x18a76a0U,0x1d06c20U,0x1d06c40U,0x18a3a00U,0x42d2880U,0x357bd50U,
    0x412bac0U,0x1af0aa0U,0x1af0a90U,0x1af0ab0U,0x412b9d0U,0x18a3a00U,0x18a3a00U,0x18a3a00U,0x18a76a0U,0x18a3a00U,
    0x18a3a00U,0x1c53bd0U,0x18a3a00U,0x18a76a0U,0x18a3ec0U,0x412ba30U,0x1c53b00U,0x4222440U,0x18a3ec0U,0x1c53840U,
    0x3cc9d20U,0x4225840U,0x2a651e0U,0x18a3a00U,0x18a3a00U,0x412bc50U,0x18a3a00U,0x1af0b30U,0x18a3a00U,0x412c240U,
    0x412c2d0U,0x18a3ec0U,0x412c360U,0x18a76a0U,0x18a76a0U,0x4222420U,0x18a76a0U,0x1c53be0U,0x18a3a00U,0x18a3a00U,
    0x18a3a00U,0x18a3a00U,0x18a3a00U,0x412bb20U,0x1af0b50U,0x2a64ee0U,0x18a76a0U,0x1c562c0U,0x1af0b70U,0x18a3ed0U,
    0x18a3ec0U,0x1c554e0U,0x18a3a00U,0x1c56400U,0x1c56490U,0x1c56550U,0x1c56640U,0x1c56670U,0x4225930U,0x18a3a00U,
    0x4131440U,0x4131000U,0x1c5dc90U,0x4131040U,0x18a3a00U,0x4225610U,0x4225650U,0x4225690U,0x18a3a00U,0x1c54130U,
    0x18a3ed0U,0x18a3a00U,0x18a3a00U,0x1cdc850U,0x412c380U,0x412c9e0U,0x18a76a0U,0x18a3ed0U,0x18a3a00U,0x1c54c40U,
    0x3ea0540U,0x18a3a00U,0x18dd100U,0x4131420U,0x4130ae0U,0x4130bb0U,0x4130c80U,0x4130da0U,0x4130cb0U,0x4131210U,
    0x2a64f70U,0x18a3a00U,0x18a3ec0U,0x18a3a00U,0x18a3a00U,0x4131450U,0x4131460U,0x4130c70U,0x4216dc0U,0x4216e80U,
    0x18a3ec0U,0x412f690U,0x412f6a0U,0x412f6b0U,0x412f6c0U,0x412f6d0U,0x412f6e0U,0x412f6f0U,0x412f700U,0x18a76a0U,
    0x18a76a0U,0x412e340U,0x412d200U,0x412d220U,0x6f04e90U,0x6f045f0U,0x412d210U,0x37f7970U,0x2a64f90U,0x412e0d0U,
    0x412e100U,0x412e290U,0x18a3ec0U,0x18a3a00U,0x2a65000U,0x18a76a0U,0x18a3ec0U,0x18a76a0U,0x18a3a00U,0x412bee0U,
    0x18a3ed0U,0x4221720U,0x41327d0U,0x18a3a00U,0x42176c0U,0x3c17b90U,0x4218060U,0x412fb20U,0x412fb40U,0x412fb60U,
    0x412fb80U,0x4223ce0U,0x18a76a0U,0x18a76a0U,0x18a76a0U,0x4221d30U,0x42204e0U,0x4220520U,0x42218b0U,0x42219a0U,
    0x4221a20U,0x4221ab0U,0x18a76a0U,0x18a76a0U,0x18a3a00U,0x4221b30U,0x6f08680U,0x6f05890U,0x4223a60U,0x3810190U,
    0x18a3a00U,0x4222df0U,0x4223250U,0x18a3a00U,0x36c97b0U,0x2a651f0U,0x4219ee0U,0x4219f20U,0x18a76a0U,0x4221dd0U,
    0x4221b50U,0x4223630U,0x18a76a0U,0x18a3ed0U,0x18a3ed0U,0x18be7f0U,0x4221c30U,0x18a3ec0U,0x4225140U,0x2a65210U,
    0x18a3a00U,0x2a65270U,0x2a65290U,0x18a3ec0U,0x18a3a00U,0x18a76a0U,0x4226cd0U,0x18a3a00U,0x6f05d30U,0x6f05a40U,
    0x6f05900U,0x6f058e0U,0x6f04470U,0x6f04e00U,0x6f057d0U,0x6f073f0U,
};
inline constexpr uint8_t PawnLocalBody[]{0x48,0x83,0xec,0x28,0x48,0x8b,0x89,0xa0,0x4,0x0,0x0,0x48,0x85,0xc9,0x74,0x1b,0x48,0x8b,0x1,0x48,0x8b,0x80,0x0,0x8,0x0,0x0,0xff,0x15,0xf8,0x78,0x98,0x5,0x84,0xc0,0x74,0x7,0xb0,0x1,0x48,0x83,0xc4,0x28,0xc3,0x32,0xc0,0x48,0x83,0xc4,0x28,0xc3};
inline constexpr uint8_t ViewHelperBody[]{0x40,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0x1,0x48,0x8b,0xf9,0x48,0x8b,0x80,0x30,0x8,0x0,0x0,0xff,0x15,0x8f,0xc,0x97,0x2,0x84,0xc0,0x74,0x8,0xb0,0x1,0x48,0x83,0xc4,0x20,0x5f,0xc3,0x48,0x8b,0x7,0x48,0x8b,0xcf,0x48,0x89,0x5c,0x24,0x30,0x48,0x8b,0x80,0xb8,0x1,0x0,0x0,0xff,0x15,0x6b,0xc,0x97,0x2,0x48,0x85,0xc0,0x74,0x38,0x48,0x8b,0x88,0xd8,0x1,0x0,0x0,0x48,0x85,0xc9,0x74,0x2c,0x48,0x8b,0x41,0x40,0x48,0x63,0x49,0x48,0x48,0x8d,0x14,0xc8,0x48,0x3b,0xc2,0x74,0x1b,0x90,0x48,0x8b,0x8,0x48,0x85,0xc9,0x74,0x9,0x48,0x8b,0x59,0x38,0x48,0x85,0xdb,0x75,0x16,0x48,0x83,0xc0,0x8,0x48,0x3b,0xc2,0x75,0xe6,0x48,0x8b,0x5c,0x24,0x30,0x32,0xc0,0x48,0x83,0xc4,0x20,0x5f,0xc3,0xe8,0xc4,0xc4,0xbb,0xfe,0x48,0x8b,0x4b,0x10,0x48,0x8b,0xd0,0xe8,0x48,0x56,0x28,0xfa,0x84,0xc0,0x74,0xde,0x48,0x8b,0x8b,0x60,0xc,0x0,0x0,0x48,0x85,0xc9,0x74,0xd2,0xe8,0x73,0xe8,0xfe,0xff,0x48,0x8b,0x5c,0x24,0x30,0x48,0x3b,0xc7,0xf,0x94,0xc0,0x48,0x83,0xc4,0x20,0x5f,0xc3};
inline constexpr uint8_t WwiseBody[]{0x48,0x89,0x5c,0x24,0x8,0x48,0x89,0x74,0x24,0x18,0x48,0x89,0x54,0x24,0x10,0x57,0x48,0x83,0xec,0x30,0x48,0x8b,0xda,0x48,0x8b,0xf1,0x33,0xff,0x89,0x7c,0x24,0x20,0x48,0x89,0x3a,0x48,0x89,0x7a,0x8,0x8d,0x57,0x4,0x48,0x8b,0xcb,0xe8,0x7e,0x1e,0x4,0xfa,0x8b,0x53,0x8,0x8d,0x42,0x4,0x89,0x43,0x8,0x8b,0x43,0xc,0x2b,0xc2,0x83,0xf8,0x4,0x73,0x8,0x48,0x8b,0xcb,0xe8,0xe3,0x1e,0x4,0xfa,0x48,0x8b,0xb,0x48,0x8b,0x5,0xe9,0xb4,0x1c,0x4,0x48,0x89,0x1,0xc7,0x44,0x24,0x20,0x1,0x0,0x0,0x0,0x48,0x85,0xf6,0xf,0x84,0x9d,0x0,0x0,0x0,0x48,0x8b,0x6,0x48,0x8b,0xce,0x48,0x8b,0x80,0x30,0x8,0x0,0x0,0xff,0x15,0x6a,0x6,0x95,0x2,0x84,0xc0,0x75,0x18,0x48,0x8b,0xce,0xe8,0x6,0xa1,0x58,0xff,0x84,0xc0,0x74,0x7a,0x48,0x8b,0xce,0xe8,0xaa,0xf9,0xfd,0xff,0x84,0xc0,0x74,0x6e,0x48,0x8d,0x5,0x9f,0xde,0x5d,0x4,0x48,0x39,0x3,0xf,0x84,0xda,0x0,0x0,0x0,0x89,0x7b,0x8,0x83,0x7b,0xc,0xc,0x74,0xd,0xba,0xc,0x0,0x0,0x0,0x48,0x8b,0xcb,0xe8,0xf0,0x1d,0x4,0xfa,0x8b,0x53,0x8,0x8d,0x42,0xc,0x89,0x43,0x8,0x8b,0x43,0xc,0x2b,0xc2,0x83,0xf8,0xc,0x73,0x8,0x48,0x8b,0xcb,0xe8,0x55,0x1e,0x4,0xfa,0x48,0x8b,0x3,0xf,0x10,0x5,0x5b,0xde,0x5d,0x4,0xf,0x11,0x0,0xf2,0xf,0x10,0x5,0x60,0xde,0x5d,0x4,0xf2,0xf,0x11,0x40,0x10,0x48,0x8b,0xc3,0x48,0x8b,0x5c,0x24,0x40,0x48,0x8b,0x74,0x24,0x50,0x48,0x83,0xc4,0x30,0x5f,0xc3,0x48,0x8b,0x6,0x48,0x8b,0xce,0x48,0x8b,0x80,0xb8,0x1,0x0,0x0,0xff,0x15,0xcd,0x5,0x95,0x2,0x48,0x85,0xc0,0x74,0x3f,0x48,0x8b,0x80,0xd8,0x1,0x0,0x0,0x48,0x85,0xc0,0x74,0x33,0x48,0x8b,0x48,0x40,0x48,0x63,0x40,0x48,0x4c,0x8d,0x4,0xc1,0x49,0x3b,0xc8,0x74,0x22,0xf,0x1f,0x0,0x48,0x8b,0x1,0x48,0x85,0xc0,0x74,0x9,0x48,0x8b,0x50,0x38,0x48,0x85,0xd2,0x75,0xb,0x48,0x83,0xc1,0x8,0x49,0x3b,0xc8,0x75,0xe6,0xeb,0x3,0x48,0x8b,0xfa,0x48,0x8b,0xd6,0x48,0x8b,0xcf,0xe8,0x96,0xfe,0xf9,0xff,0x48,0x8b,0xcb,0x85,0xc0,0x48,0x8d,0x15,0x72,0xca,0xe2,0x4,0x74,0x7,0x48,0x8d,0x15,0x89,0xca,0xe2,0x4,0xe8,0x3c,0x6d,0x3,0xfa,0x48,0x8b,0xc3,0x48,0x8b,0x5c,0x24,0x40,0x48,0x8b,0x74,0x24,0x50,0x48,0x83,0xc4,0x30,0x5f,0xc3,0xcc};
inline constexpr uint8_t BlueprintLocalQueryThunk[]{0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0x42,0x20,0x45,0x33,0xc9,0x48,0x85,0xc0,0x49,0x8b,0xd8,0x41,0xf,0x95,0xc1,0x4c,0x3,0xc8,0x4c,0x89,0x4a,0x20,0x48,0x8b,0x1,0x48,0x8b,0x80,0x30,0x8,0x0,0x0,0xff,0x15,0x3a,0x98,0x98,0x5,0x88,0x3,0x48,0x83,0xc4,0x20,0x5b,0xc3};
inline constexpr uint8_t FrameConstructionBody[]{0x48,0x8b,0x56,0x58,0x66,0xc7,0x45,0x38,0x0,0x1,0x48,0x8d,0x5,0xd0,0x2,0xb9,0x8,0x48,0x89,0x45,0x30,0x48,0x89,0x75,0x40,0x4c,0x89,0x7d,0x48,0x48,0x8b,0x46,0x68,0x48,0x89,0x45,0x50,0x48,0x89,0x7d,0x58};
inline constexpr uint8_t AimpunchConsumerBody[]{0x48,0x89,0x5c,0x24,0x8,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x60,0xf,0x29,0x74,0x24,0x50,0xf,0x57,0xc0,0xf,0x28,0xf2,0x41,0xf,0xb6,0xf1,0xf,0x2e,0xf0,0x48,0x8b,0xda,0x48,0x8b,0xf9,0xf,0x84,0x51,0x1,0x0,0x0,0xf,0x2f,0xf0,0x73,0x31,0x80,0x3d,0x7f,0xc1,0x1,0x6,0x2,0xf,0x82,0x3f,0x1,0x0,0x0,0xf,0x57,0xd2,0x48,0x8d,0x15,0xef,0x92,0xe3,0x4,0xf3,0xf,0x5a,0xd6,0x48,0x8d,0xd,0x64,0xc1,0x1,0x6,0x66,0x49,0xf,0x7e,0xd0,0xe8,0x42,0x37,0x12,0xfa,0xe9,0x1b,0x1,0x0,0x0,0x48,0x85,0xdb,0xf,0x84,0xf6,0x0,0x0,0x0,0xe8,0xcf,0xc9,0x40,0xfe,0x48,0x85,0xc0,0xf,0x84,0xe8,0x0,0x0,0x0,0x48,0x8d,0x50,0x38,0x48,0x63,0x40,0x40,0x3b,0x43,0x40,0xf,0x8f,0xd7,0x0,0x0,0x0,0x48,0x8b,0xc8,0x48,0x8b,0x43,0x38,0x48,0x39,0x14,0xc8,0xf,0x85,0xc6,0x0,0x0,0x0,0x80,0xbc,0x24,0x98,0x0,0x0,0x0,0x0,0x74,0x3f,0x48,0x8b,0x7,0x48,0x8b,0xcf,0x48,0x8b,0x80,0xb8,0x1,0x0,0x0,0xff,0x15,0xaf,0x18,0x96,0x2,0xf2,0xf,0x10,0x88,0x40,0x7,0x0,0x0,0x66,0xf,0x5a,0xc9,0xf,0x28,0xc1,0xf3,0xf,0x5c,0x87,0x8c,0x10,0x0,0x0,0xf,0x2f,0x87,0x88,0x10,0x0,0x0,0xf,0x82,0xa1,0x0,0x0,0x0,0xf3,0xf,0x11,0x8f,0x8c,0x10,0x0,0x0,0x48,0x8b,0xcf,0xe8,0xd3,0xb,0xff,0xff,0x84,0xc0,0xf,0x84,0x89,0x0,0x0,0x0,0x48,0x8b,0x7,0x48,0x8b,0xcf,0x48,0x8b,0x80,0xb8,0x1,0x0,0x0,0xff,0x15,0x60,0x18,0x96,0x2,0x48,0x8b,0xc8,0xe8,0xd0,0x24,0x3,0x0,0x48,0x85,0xc0,0x74,0x69,0x48,0x8b,0x88,0x28,0x5,0x0,0x0,0x48,0x85,0xc9,0x74,0x5d,0x48,0x8b,0x84,0x24,0x90,0x0,0x0,0x0,0x48,0x8d,0x54,0x24,0x30,0x48,0x89,0x54,0x24,0x20,0x44,0xf,0xb6,0xce,0xf,0x28,0xd6,0x48,0x8b,0xd3,0xf,0x28,0x0,0xf2,0xf,0x10,0x48,0x10,0x48,0x8b,0x1,0xf,0x29,0x44,0x24,0x30,0xf2,0xf,0x11,0x4c,0x24,0x40,0x48,0x8b,0x80,0x10,0x9,0x0,0x0,0xff,0x15,0x8,0x18,0x96,0x2,0xeb,0x1c,0x80,0x3d,0x4f,0xc0,0x1,0x6,0x2,0x72,0x13,0x48,0x8d,0x15,0xee,0x93,0xe3,0x4,0x48,0x8d,0xd,0x3f,0xc0,0x1,0x6,0xe8,0x22,0x36,0x12,0xfa,0x48,0x8b,0x5c,0x24,0x70,0x48,0x8b,0x74,0x24,0x78,0xf,0x28,0x74,0x24,0x50,0x48,0x83,0xc4,0x60,0x5f,0xc3};
using ProcessEvent=void(*)(void*,void*,void*);
enum class Membership:uint8_t { NonLocal, Local, Unknown };
struct Entry {
    bool bound=false,installed=false,retired=false,fullPair=false,unpossessed=false;
    bool noPlayer=false,notInLocalPlayers=false,rosterValid=false,lifecycleReady=false;
    bool otherSlotsPreserved=false,actorFieldsPreserved=false,sourceUntouched=false;
    bool attenuationPresent=false,oneShotCleared=false,lastFootstepTimeValid=false;
    uint8_t cacheBefore=0,cacheAfter=0,footstepsLocalBefore=0,footstepsLocalAfter=0;
    uint8_t role=0,remoteRole=0;
    unsigned error=0;
    uint64_t epoch=0,binds=0,calls=0,nonLocalReturns=0,localForwards=0,uncertainForwards=0;
    uint64_t lifecycleCalls=0,oneShotClearCalls=0,footstepTimeChanges=0;
    float lastFootstepTime=0;
    const char* stage="npc_locality_not_bound";
    void *controller=nullptr,*pawn=nullptr,*playerState=nullptr,*type=nullptr,*owner=nullptr,*camera=nullptr;
    void *footsteps=nullptr,*attenuation=nullptr,*lifecyclePawn=nullptr;
    void *secondary468=nullptr,*secondary508=nullptr,*secondaryA68=nullptr,*secondaryA70=nullptr;
    WeakIdentity controllerId{},pawnId{},stateId{},lifecyclePawnId{};
};
struct Diagnostics {
    bool initialized=false,sourceVerified=false,sourceUntouched=false,cloneReadOnly=false,localPairValid=false;
    bool soundFrameProfile=false,attenuationProfile=false,attenuationCloneReadOnly=false,blueprintFrameProfile=false,aimpunchFrameProfile=false;
    bool listenerRegistrationProfile=false;
    bool spectatedFrameProfile=false;
    unsigned error=0,boundCount=0;
    uintptr_t exceptionRva=0;
    uint64_t installs=0,binds=0,refreshes=0,retirements=0,samples=0,callbackCalls=0,nonLocalReturns=0,originalForwards=0;
    uint64_t soundDirectReturns=0,soundViewHelperReturns=0,attenuationInstalls=0,attenuationReclaims=0,attenuationCalls=0,attenuationAddsRejected=0,attenuationCallsForwarded=0;
    uint64_t attenuationFrameCandidates=0,attenuationFrameNodeMatches=0,attenuationFrameOwnerMatches=0,attenuationFrameGateReturns=0;
    uint64_t aimpunchConsumerMatches=0,aimpunchViewGateReturns=0;
    uint64_t listenerRegistrationMatches=0,listenerRegistrationRejects=0;
    uint64_t audioFrameCandidates=0,audioFrameNodeMatches=0,audioFrameContextMatches=0;
    uint64_t audioFrameCodeRejects=0,audioFrameContextRejects=0,audioFrameReturns=0;
    uint64_t audioBindAttempts=0,audioBindMismatches=0,lastAudioBindMs=0;
    uint32_t audioFunctionsBound=0;
    uint32_t spectatedFunctionsBound=0;
    uint64_t spectatedFrameCandidates=0,spectatedFramesRecovered=0,spectatedFrameCopyRejects=0;
    uint64_t spectatedNodeMatches=0,spectatedContextRejects=0,spectatedCodeRejects=0,spectatedViewReturns=0;
    uint64_t spectatedDirectRecoveries=0,spectatedHelperRecoveries=0,lastSpectatedBindMs=0;
    void* lastAimpunchController=nullptr;
    void *lastAttenuationFrame=nullptr,*lastAttenuationNode=nullptr,*lastAttenuationObject=nullptr;
    const char* stage="original_npc_locality_uninitialized";
    void *world=nullptr,*gameInstance=nullptr,*initialLocalPlayer=nullptr,*initialLocalController=nullptr;
    WeakIdentity worldId{};
    std::array<Entry,MaxPlayers> entries{};
};
inline Diagnostics state;
inline void** clone=nullptr;
inline void* cloneAllocation=nullptr;
inline std::atomic<uint64_t> offThreadForwards{0};
struct Attenuation {
    bool bound=false;
    size_t index=0;
    void *object=nullptr,*pawn=nullptr,*controller=nullptr,*playerState=nullptr;
    WeakIdentity objectId{},pawnId{},controllerId{},stateId{};
    uint64_t calls=0,rejected=0,forwarded=0;
};
inline std::array<Attenuation,MaxAttenuations> attenuations{};
inline void** attenuationClone=nullptr;
inline void* attenuationAllocation=nullptr;
inline void *addAudioFunction=nullptr,*addNewFunction=nullptr;
inline uint16_t addAudioBytes=0,addNewBytes=0;
struct AudioFunction {
    void* function=nullptr;void* script=nullptr;
    WeakIdentity identity{};
    void* ownerClass=nullptr;void* contextDefault=nullptr;
    WeakIdentity ownerId{},contextDefaultId{};
    uint64_t nodeMatches=0,viewReturns=0;
    uint32_t lastCodeOffset=0;
};
inline std::array<AudioFunction,original_audio_view_consumers::Profiles.size()> audioFunctions{};
inline void* audioLibraryClass=nullptr;inline void* audioLibraryDefault=nullptr;
inline WeakIdentity audioLibraryClassId{},audioLibraryDefaultId{};
inline std::array<AudioFunction,original_audio_spectated_consumers::Profiles.size()> spectatedAudioFunctions{};
inline void* spectatedQueryFunction=nullptr;
inline WeakIdentity spectatedQueryId{};
inline constexpr uint8_t ListenerRegistrationBody[]{
    0xf6,0x87,0x00,0x05,0x00,0x00,0x02,0x0f,0x84,0x85,0x00,0x00,0x00,0x48,0x8b,0x07,
    0x48,0x8b,0xcf,0x48,0x8b,0x80,0x00,0x08,0x00,0x00,0xff,0x15,0x07,0xef,0x86,0x04,
    0x84,0xc0,0x74,0x6e,0x48,0x8b,0xcb,0xe8,0x03,0x6f,0x01,0x00,0x48,0x8b,0xf8,0x48,0x85,0xc0
};
inline const Diagnostics& State(){return state;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline void** Source(){return reinterpret_cast<void**>(engine::image+NativeTableRva);}
inline Predicate Original(){return engine::function<Predicate>(NativePredicateRva);}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;return false;}
inline bool EntryFail(Entry& e,unsigned error,const char* stage){e.error=error;e.stage=stage;return Fail(error,stage);}
inline bool Live(void* object,const WeakIdentity& id){return object&&original_inventory_slots::ResolveIdentity(id)==object;}
inline void ResolveAudioFunctions(bool force=false){
    const auto now=GetTickCount64();
    if(!force&&state.lastAudioBindMs&&now-state.lastAudioBindMs<1000)return;
    state.lastAudioBindMs=now;++state.audioBindAttempts;state.audioFunctionsBound=0;
    auto type=engine::Find(original_audio_view_consumers::ClassPath);
    if(!type){audioLibraryClass=nullptr;audioLibraryDefault=nullptr;return;}
    auto defaultObject=engine::DefaultObject(type);
    if(!defaultObject||Read<void*>(defaultObject,0x10)!=type
       ||!original_inventory_slots::CaptureIdentity(type,audioLibraryClassId)
       ||!original_inventory_slots::CaptureIdentity(defaultObject,audioLibraryDefaultId))return;
    audioLibraryClass=type;audioLibraryDefault=defaultObject;
    for(size_t i=0;i<audioFunctions.size();++i){
        auto& binding=audioFunctions[i];const auto& profile=original_audio_view_consumers::Profiles[i];
        auto function=engine::Find(profile.path);
        if(!function){binding.function=nullptr;binding.script=nullptr;continue;}
        const auto flags=Read<uint32_t>(function,0xb8);
        auto script=Read<void*>(function,0x68);
        const auto bytes=Read<int32_t>(function,0x70),capacity=Read<int32_t>(function,0x74);
        // The original FFrame construction below takes Code from Node+68.
        // Only Static, non-native script declarations from this exact library.
        if(Read<void*>(function,0x28)!=type||(flags&0x2400)!=0x2000
           ||bytes!=profile.scriptBytes||capacity<bytes||capacity>65536||!script
           ||!original_inventory_slots::CaptureIdentity(function,binding.identity)){
            binding.function=nullptr;binding.script=nullptr;++state.audioBindMismatches;continue;
        }
        binding.function=function;binding.script=script;++state.audioFunctionsBound;
    }
}
inline bool SpectatedNativeProfilesMatch(){
    for(const auto& profile:original_audio_spectated_consumers::NativeProfiles){
        const auto* bytes=reinterpret_cast<const uint8_t*>(engine::image+profile.rva);
        if(!original_players::Readable(bytes,profile.bytes))return false;
        uint64_t hash=14695981039346656037ULL;
        for(size_t i=0;i<profile.bytes;++i){hash^=bytes[i];hash*=1099511628211ULL;}
        if(hash!=profile.fnv)return false;
    }
    return Read<uintptr_t>(reinterpret_cast<void*>(engine::image+0xd747d20),0)==engine::image+0x1ce1dc0;
}
inline void ResolveSpectatedAudioFunctions(bool force=false){
    const auto now=GetTickCount64();
    if(!force&&state.lastSpectatedBindMs&&now-state.lastSpectatedBindMs<1000)return;
    state.lastSpectatedBindMs=now;state.spectatedFunctionsBound=0;
    auto query=engine::Find(L"/Script/ShooterGame.ShooterBlueprintLibrary.IsLocallyControlledOrSpectated");
    if(!query||(Read<uint32_t>(query,0xb8)&0x400)==0
       ||Read<void*>(query,0xe0)!=reinterpret_cast<void*>(engine::image+0x6c2ced0)
       ||!original_inventory_slots::CaptureIdentity(query,spectatedQueryId)){
        spectatedQueryFunction=nullptr;return;
    }
    spectatedQueryFunction=query;
    for(size_t i=0;i<spectatedAudioFunctions.size();++i){
        auto& binding=spectatedAudioFunctions[i];const auto& p=original_audio_spectated_consumers::Profiles[i];
        auto function=engine::Find(p.path);auto owner=engine::Find(p.owner);
        auto script=Read<void*>(function,0x68);const auto flags=Read<uint32_t>(function,0xb8);
        if(!function||!owner||Read<void*>(function,0x28)!=owner||!script
           ||Read<int32_t>(function,0x70)!=p.scriptBytes||Read<int32_t>(function,0x74)<p.scriptBytes
           ||(flags&0x2400)!=(p.staticContext?0x2000U:0U)
           ||!original_inventory_slots::CaptureIdentity(function,binding.identity)
           ||!original_inventory_slots::CaptureIdentity(owner,binding.ownerId)){
            binding.function=nullptr;binding.script=nullptr;continue;
        }
        binding.contextDefault=p.staticContext?engine::DefaultObject(owner):nullptr;
        if(p.staticContext&&(!binding.contextDefault||Read<void*>(binding.contextDefault,0x10)!=owner
           ||!original_inventory_slots::CaptureIdentity(binding.contextDefault,binding.contextDefaultId))){
            binding.function=nullptr;binding.script=nullptr;continue;
        }
        binding.function=function;binding.script=script;binding.ownerClass=owner;++state.spectatedFunctionsBound;
    }
}
inline bool SourceMatches(){
    auto source=Source();
    if(Read<uintptr_t>(source-1,0)!=engine::image+NativePrefixRva||Read<void*>(source,TableEntries*8)!=nullptr)return false;
    for(size_t i=0;i<TableEntries;++i)if(Read<uintptr_t>(source,i*8)!=engine::image+NativeSlots[i])return false;
    return true;
}
inline bool SameRoster(void* playerState){
    auto gs=Read<void*>(state.world,0x178),data=Read<void*>(gs,0x480);
    const auto count=Read<int32_t>(gs,0x488),capacity=Read<int32_t>(gs,0x48c);
    if(!gs||!original_client_lifecycle::ValidArray(count,capacity,data)||count>40)return false;
    unsigned found=0;for(int32_t i=0;i<count;++i)if(Read<void*>(data,static_cast<size_t>(i)*8)==playerState)++found;
    return found==1;
}
inline Membership LocalMembership(void* controller){
    // Protect a real UPlayer or a newly promoted actual local controller before
    // consulting an NPC record. These paths retain the complete stock predicate.
    if(Read<void*>(controller,0x510)||controller==original_players::activeController)return Membership::Local;
    if(!Live(state.world,state.worldId)||state.world!=original_players::world)return Membership::Unknown;
    auto gi=Read<void*>(state.world,0x1d8),data=Read<void*>(gi,0x40);
    const auto count=Read<int32_t>(gi,0x48),capacity=Read<int32_t>(gi,0x4c);
    if(!gi||gi!=state.gameInstance||!original_client_lifecycle::ValidArray(count,capacity,data)||count<1||count>32)return Membership::Unknown;
    for(int32_t i=0;i<count;++i){
        auto lp=Read<void*>(data,static_cast<size_t>(i)*8),pc=Read<void*>(lp,0x38);
        if(!lp||!pc)return Membership::Unknown;
        if(pc==controller)return Membership::Local;
    }
    return Membership::NonLocal;
}
inline Entry* Find(void* controller){for(auto& e:state.entries)if(e.bound&&e.controller==controller)return &e;return nullptr;}
inline bool VerifiedNPC(Entry& e){
    if(!e.installed||e.retired||!Live(e.controller,e.controllerId)||!Live(e.playerState,e.stateId))return false;
    const auto index=static_cast<size_t>(&e-state.entries.data());
    const auto& player=original_players::players[index];
    // PC and PS persist through native death / temporary unpossession. Do not
    // turn the stock Standalone-local branch back on when its Pawn is absent.
    return static_cast<int>(index)!=original_players::localIndex&&!player.local
      &&player.controller==e.controller&&player.state==e.playerState
      &&Read<void*>(e.controller,0)==clone&&Read<void*>(e.controller,0x470)==e.playerState
      &&Read<void*>(e.playerState,0x210)==e.controller&&SameRoster(e.playerState);
}
inline unsigned SoundChain(uintptr_t directReturn,const void* returnSlot){
    // Original Ak's camera-listener registration queries PC.v800 directly.
    // That presentation consumer must not register simulation-local NPCs.
    if(state.listenerRegistrationProfile&&directReturn==engine::image+0x596d2f1)return 4;
    if(!state.soundFrameProfile||directReturn!=engine::image+0x4854900)return 0;
    // The complete pinned Pawn body subtracts28h before calling PC.v800.
    // Its caller's return slot is therefore exactly30h above our return slot.
    const auto pawnCaller=Read<uintptr_t>(returnSlot,0x30);
    if(pawnCaller==engine::image+0x788bb8e)return 1;
    // Wwise's second test calls the original view helper. Its verified prologue
    // pushes RDI/subtracts20h, adding another30h. Match BOTH original returns.
    if(pawnCaller==engine::image+0x786b569){
        const auto consumer=Read<uintptr_t>(returnSlot,0x60);
        if(consumer==engine::image+0x788bba6)return 2;
        // Original ClientPlayAimpunch is also the real landing-shake consumer.
        // Only its visual local/spectated check may use actual view ownership.
        // The helper's remaining original spectator-target check still runs.
        if(state.aimpunchFrameProfile&&consumer==engine::image+0x787a97d)return 3;
    }
    return 0;
}
inline int AudioBlueprintGate(void* frame,uintptr_t directReturn,const void* returnSlot){
    if(!state.blueprintFrameProfile||directReturn!=engine::image+0x4854900
       ||Read<uintptr_t>(returnSlot,0x30)!=engine::image+0x48529be)return -1;
    ++state.audioFrameCandidates;
    if(state.audioFunctionsBound<audioFunctions.size())ResolveAudioFunctions();
    auto node=Read<void*>(frame,0x10);
    for(size_t i=0;i<audioFunctions.size();++i){auto& binding=audioFunctions[i];
        if(!binding.function||node!=binding.function||!Live(node,binding.identity))continue;
        ++state.audioFrameNodeMatches;++binding.nodeMatches;
        if(!Live(audioLibraryClass,audioLibraryClassId)||!Live(audioLibraryDefault,audioLibraryDefaultId)
           ||Read<void*>(node,0x28)!=audioLibraryClass||Read<void*>(frame,0x18)!=audioLibraryDefault){
            ++state.audioFrameContextRejects;return -1;
        }
        ++state.audioFrameContextMatches;
        const auto start=reinterpret_cast<uintptr_t>(binding.script);
        const auto code=reinterpret_cast<uintptr_t>(Read<void*>(frame,0x20));
        const auto bytes=original_audio_view_consumers::Profiles[i].scriptBytes;
        if(Read<void*>(node,0x68)!=binding.script||Read<int32_t>(node,0x70)!=bytes
           ||code<start||code-start>=static_cast<uintptr_t>(bytes)){
            ++state.audioFrameCodeRejects;return -1;
        }
        binding.lastCodeOffset=static_cast<uint32_t>(code-start);
        return static_cast<int>(i);
    }
    return -1;
}
inline int SpectatedAudioBlueprintGate(uintptr_t directReturn,const void* returnSlot){
    if(!state.spectatedFrameProfile||directReturn!=engine::image+0x4854900)return -1;
    const auto caller=Read<uintptr_t>(returnSlot,0x30);
    const bool direct=caller==engine::image+0x6c2cfb5;
    const bool helper=caller==engine::image+0x786b569
        &&Read<uintptr_t>(returnSlot,0x60)==engine::image+0x6c2cfe9;
    if(!direct&&!helper)return -1;
    ++state.spectatedFrameCandidates;
    if(Read<uintptr_t>(returnSlot,direct?0x60:0x90)!=engine::image+0x1b9c5c7
       ||Read<uintptr_t>(returnSlot,direct?0xb0:0xe0)!=engine::image+0x1cdb5c4)return -1;
    const auto nativeSaved=Read<uintptr_t>(returnSlot,direct?0x58:0x88);
    const auto callerSaved=Read<uintptr_t>(returnSlot,direct?0xa8:0xd8);
    if(nativeSaved<0x10090||nativeSaved-0x90!=callerSaved){++state.spectatedFrameCopyRejects;return -1;}
    auto frame=reinterpret_cast<void*>(callerSaved);
    if(state.spectatedFunctionsBound<spectatedAudioFunctions.size())ResolveSpectatedAudioFunctions();
    if(!Live(spectatedQueryFunction,spectatedQueryId)||Read<void*>(frame,0x90)!=spectatedQueryFunction)return -1;
    ++state.spectatedFramesRecovered;
    if(direct)++state.spectatedDirectRecoveries;else ++state.spectatedHelperRecoveries;
    auto node=Read<void*>(frame,0x10);auto object=Read<void*>(frame,0x18);
    for(size_t i=0;i<spectatedAudioFunctions.size();++i){auto& binding=spectatedAudioFunctions[i];
        if(!binding.function||node!=binding.function||!Live(node,binding.identity))continue;
        ++state.spectatedNodeMatches;++binding.nodeMatches;const auto& p=original_audio_spectated_consumers::Profiles[i];
        const bool context=Live(binding.ownerClass,binding.ownerId)&&Read<void*>(node,0x28)==binding.ownerClass
          &&(p.staticContext?(object==binding.contextDefault&&Live(object,binding.contextDefaultId))
             :original_players::Derived(Read<void*>(object,0x10),binding.ownerClass));
        if(!context){++state.spectatedContextRejects;return -1;}
        const auto start=reinterpret_cast<uintptr_t>(binding.script),code=reinterpret_cast<uintptr_t>(Read<void*>(frame,0x20));
        binding.lastCodeOffset=code>=start&&code-start<65536?static_cast<uint32_t>(code-start):0;
        if(Read<void*>(node,0x68)!=binding.script||Read<int32_t>(node,0x70)!=p.scriptBytes
           ||code<start||code-start!=p.queryResume){++state.spectatedCodeRejects;return -1;}
        return static_cast<int>(i);
    }
    return -1;
}
inline bool AttenuationBlueprintGate(void* controller,void* frame,uintptr_t directReturn,const void* returnSlot){
    if(!state.blueprintFrameProfile||directReturn!=engine::image+0x4854900
      ||Read<uintptr_t>(returnSlot,0x30)!=engine::image+0x48529be)return false;
    ++state.attenuationFrameCandidates;state.lastAttenuationFrame=frame;
    // The pinned execIsLocallyControlled and Pawn.v830 preserve RDX all the
    // way to PC.v800. It is an FFrame ONLY on this verified native-thunk chain.
    // No other call path's second register is interpreted as a frame pointer.
    auto node=Read<void*>(frame,0x10),object=Read<void*>(frame,0x18);
    state.lastAttenuationNode=node;state.lastAttenuationObject=object;
    if(!addNewFunction||node!=addNewFunction)return false;
    ++state.attenuationFrameNodeMatches;
    for(auto& a:attenuations){
        if(!a.bound||a.object!=object||a.controller!=controller)continue;
        if(!Live(object,a.objectId)||!Live(a.pawn,a.pawnId)||!Live(controller,a.controllerId)||!Live(a.playerState,a.stateId)
          ||Read<void*>(object,0)!=attenuationClone||Read<void*>(object,0xc8)!=a.pawn
          ||Read<void*>(controller,0x470)!=a.playerState||Read<void*>(addNewFunction,0x28)!=Read<void*>(object,0x10))return false;
        ++state.attenuationFrameOwnerMatches;return true;
    }
    return false;
}
inline bool Observe(void* controller,void* possibleFrame){
    const auto directReturn=reinterpret_cast<uintptr_t>(_ReturnAddress());
    const auto returnSlot=_AddressOfReturnAddress();
    if(GetCurrentThreadId()!=original_players::gameThread){offThreadForwards.fetch_add(1,std::memory_order_relaxed);return Original()(controller);}
    ++state.callbackCalls;auto e=Find(controller);
    if(e)++e->calls;
    const auto sound=SoundChain(directReturn,returnSlot);
    if(sound==3){++state.aimpunchConsumerMatches;state.lastAimpunchController=controller;}
    if(sound==4)++state.listenerRegistrationMatches;
    const bool attenuationGate=!sound&&AttenuationBlueprintGate(controller,possibleFrame,directReturn,returnSlot);
    const int audioIndex=!sound&&!attenuationGate?AudioBlueprintGate(possibleFrame,directReturn,returnSlot):-1;
    const int spectatedIndex=!sound&&!attenuationGate&&audioIndex<0?SpectatedAudioBlueprintGate(directReturn,returnSlot):-1;
    if((sound||attenuationGate||audioIndex>=0||spectatedIndex>=0)&&e&&LocalMembership(controller)==Membership::NonLocal&&VerifiedNPC(*e)){
        ++state.nonLocalReturns;++e->nonLocalReturns;
        if(sound==1)++state.soundDirectReturns;else if(sound==2)++state.soundViewHelperReturns;
        else if(sound==3)++state.aimpunchViewGateReturns;
        else if(sound==4)++state.listenerRegistrationRejects;
        else if(audioIndex>=0){++state.audioFrameReturns;++audioFunctions[static_cast<size_t>(audioIndex)].viewReturns;}
        else if(spectatedIndex>=0){++state.spectatedViewReturns;++spectatedAudioFunctions[static_cast<size_t>(spectatedIndex)].viewReturns;}
        else ++state.attenuationFrameGateReturns;
        return false;
    }
    ++state.originalForwards;
    if(e){if((!sound&&!attenuationGate&&audioIndex<0&&spectatedIndex<0)||LocalMembership(controller)==Membership::Local)++e->localForwards;else ++e->uncertainForwards;}
    // This direct pinned parent is also the exact function in the verified
    // source slot. Never dispatch through the replaced slot or a foreign table.
    return Original()(controller);
}
inline void** AttenuationSource(){return reinterpret_cast<void**>(engine::image+AttenuationTableRva);}
inline bool AttenuationSourceMatches(){
    auto source=AttenuationSource();
    if(Read<uintptr_t>(source-1,0)!=engine::image+AttenuationPrefixRva)return false;
    for(size_t i=0;i<AttenuationTableEntries;++i)if(Read<uintptr_t>(source,i*8)!=engine::image+AttenuationSlots[i])return false;
    return true;
}
inline void ObserveAttenuation(void* object,void* method,void* parameters){
    auto parent=engine::function<ProcessEvent>(0x1cdc850);
    if(GetCurrentThreadId()!=original_players::gameThread){parent(object,method,parameters);return;}
    ++state.attenuationCalls;
    Attenuation* match=nullptr;for(auto& a:attenuations)if(a.bound&&a.object==object&&Live(object,a.objectId)){match=&a;break;}
    if(match)++match->calls;
    if(match&&(method==addAudioFunction||method==addNewFunction)
      &&Live(object,match->objectId)&&Live(match->pawn,match->pawnId)&&Live(match->controller,match->controllerId)
      &&Live(match->playerState,match->stateId)&&Read<void*>(object,0xc8)==match->pawn
      &&Read<void*>(match->controller,0x470)==match->playerState
      &&LocalMembership(match->controller)==Membership::NonLocal&&SameRoster(match->playerState)){
        ++state.attenuationAddsRejected;++match->rejected;return;
    }
    ++state.attenuationCallsForwarded;if(match)++match->forwarded;
    parent(object,method,parameters);
}
inline bool AttenuationCloneMatches(){
    if(!attenuationClone||Read<uintptr_t>(attenuationClone-1,0)!=engine::image+AttenuationPrefixRva)return false;
    for(size_t i=0;i<AttenuationTableEntries;++i){
        auto wanted=i==0x298/8?reinterpret_cast<void*>(&ObserveAttenuation):reinterpret_cast<void*>(engine::image+AttenuationSlots[i]);
        if(Read<void*>(attenuationClone,i*8)!=wanted)return false;
    }
    return true;
}
inline bool InitializeAttenuation(){
    state.attenuationProfile=AttenuationSourceMatches();if(!state.attenuationProfile)return Fail(2857,"npc_view_attenuation_complete_table_mismatch");
    constexpr size_t bytes=(AttenuationTableEntries+1)*sizeof(void*);
    attenuationAllocation=VirtualAlloc(nullptr,bytes,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if(!attenuationAllocation)return Fail(2858,"npc_view_attenuation_private_table_allocation_failed");
    std::memcpy(attenuationAllocation,AttenuationSource()-1,bytes);attenuationClone=static_cast<void**>(attenuationAllocation)+1;
    attenuationClone[0x298/8]=reinterpret_cast<void*>(&ObserveAttenuation);DWORD previous=0;
    if(!AttenuationCloneMatches()||!VirtualProtect(attenuationAllocation,bytes,PAGE_READONLY,&previous)||previous!=PAGE_READWRITE)
      return Fail(2859,"npc_view_attenuation_private_table_protection_failed");
    state.attenuationCloneReadOnly=true;return true;
}
inline bool CloneMatches(){
    if(!clone||Read<uintptr_t>(clone-1,0)!=engine::image+NativePrefixRva)return false;
    for(size_t i=0;i<TableEntries;++i){
        auto wanted=i==PredicateSlot?reinterpret_cast<void*>(&Observe):reinterpret_cast<void*>(engine::image+NativeSlots[i]);
        if(Read<void*>(clone,i*8)!=wanted)return false;
    }
    return true;
}
inline bool LocalPair(){
    auto gi=Read<void*>(state.world,0x1d8),data=Read<void*>(gi,0x40);
    const auto count=Read<int32_t>(gi,0x48),capacity=Read<int32_t>(gi,0x4c);
    auto pc=original_players::activeController,lp=original_players::localPlayer;
    return gi==state.gameInstance&&original_client_lifecycle::ValidArray(count,capacity,data)&&count==1
      &&Read<void*>(data,0)==lp&&lp&&pc&&Read<void*>(lp,0x38)==pc&&Read<void*>(pc,0x510)==lp;
}
inline bool InitializeImpl(void* world){
    if(!original_players::Thread()||!world||world!=original_players::world)return Fail(2830,"npc_locality_wrong_world_or_thread");
    if(state.initialized)return state.world==world&&LocalPair()&&SourceMatches()&&CloneMatches()&&AttenuationSourceMatches()&&AttenuationCloneMatches();
    state.world=world;state.gameInstance=Read<void*>(world,0x1d8);
    state.initialLocalPlayer=original_players::localPlayer;state.initialLocalController=original_players::activeController;
    if(!original_inventory_slots::CaptureIdentity(world,state.worldId)||!LocalPair())return Fail(2831,"npc_locality_actual_local_player_invalid");
    state.sourceVerified=SourceMatches();if(!state.sourceVerified)return Fail(2832,"npc_locality_pinned_complete_table_mismatch");
    state.soundFrameProfile=std::memcmp(reinterpret_cast<const void*>(engine::image+0x48548e0),PawnLocalBody,sizeof(PawnLocalBody))==0
      &&std::memcmp(reinterpret_cast<const void*>(engine::image+0x786b550),ViewHelperBody,sizeof(ViewHelperBody))==0
      &&std::memcmp(reinterpret_cast<const void*>(engine::image+0x788bb10),WwiseBody,sizeof(WwiseBody))==0;
    if(!state.soundFrameProfile)return Fail(2860,"npc_view_pinned_sound_stack_profile_mismatch");
    state.aimpunchFrameProfile=std::memcmp(reinterpret_cast<const void*>(engine::image+0x787a890),AimpunchConsumerBody,sizeof(AimpunchConsumerBody))==0;
    if(!state.aimpunchFrameProfile)return Fail(2871,"npc_view_original_aimpunch_consumer_profile_mismatch");
    state.blueprintFrameProfile=std::memcmp(reinterpret_cast<const void*>(engine::image+0x4852990),BlueprintLocalQueryThunk,sizeof(BlueprintLocalQueryThunk))==0
      &&std::memcmp(reinterpret_cast<const void*>(engine::image+0x1cdcad7),FrameConstructionBody,sizeof(FrameConstructionBody))==0;
    if(!state.blueprintFrameProfile)return Fail(2870,"npc_view_original_blueprint_frame_profile_mismatch");
    state.listenerRegistrationProfile=std::memcmp(reinterpret_cast<const void*>(engine::image+0x596d2d1),ListenerRegistrationBody,sizeof(ListenerRegistrationBody))==0;
    if(!state.listenerRegistrationProfile)return Fail(2872,"npc_view_original_ak_registration_profile_mismatch");
    ResolveAudioFunctions(true);
    state.spectatedFrameProfile=SpectatedNativeProfilesMatch();
    if(!state.spectatedFrameProfile)return Fail(2873,"npc_view_original_spectated_audio_frame_profile_mismatch");
    ResolveSpectatedAudioFunctions(true);
    constexpr size_t bytes=(TableEntries+1)*sizeof(void*);
    cloneAllocation=VirtualAlloc(nullptr,bytes,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if(!cloneAllocation)return Fail(2833,"npc_locality_private_table_allocation_failed");
    std::memcpy(cloneAllocation,Source()-1,bytes);clone=static_cast<void**>(cloneAllocation)+1;
    clone[PredicateSlot]=reinterpret_cast<void*>(&Observe);DWORD previous=0;
    if(!CloneMatches()||!VirtualProtect(cloneAllocation,bytes,PAGE_READONLY,&previous)||previous!=PAGE_READWRITE)
      return Fail(2834,"npc_locality_private_table_protection_failed");
    state.cloneReadOnly=true;state.localPairValid=true;state.sourceUntouched=SourceMatches();
    if(!InitializeAttenuation())return false;
    state.initialized=state.sourceUntouched;state.error=0;state.stage="npc_locality_complete_private_table_ready";return state.initialized;
}
inline bool Event(void* object,const wchar_t* path,uint16_t size,void* parameters){
    auto fn=engine::Find(path);auto table=Read<void**>(object,0);auto invoke=Read<void*>(table,0x298);
    if(!object||!fn||!invoke||Read<uint16_t>(fn,0xbe)!=size)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(invoke)(object,fn,parameters);return true;
}
inline bool InstallAttenuation(Entry& e){
    auto object=e.attenuation;auto type=Read<void*>(object,0x10);auto actual=Read<void**>(object,0);
    if(!object||!type||!AttenuationSourceMatches()||!AttenuationCloneMatches())return EntryFail(e,2861,"npc_view_attenuation_profile_invalid");
    addAudioFunction=engine::Find(L"/Game/UI/Screens/InGame/HUD/Minimap/Components/ShooterCharacterAudioAttenuationComponent.ShooterCharacterAudioAttenuationComponent_C.AddAudioAttenuationVisualization");
    addNewFunction=engine::Find(L"/Game/UI/Screens/InGame/HUD/Minimap/Components/ShooterCharacterAudioAttenuationComponent.ShooterCharacterAudioAttenuationComponent_C.AddNewAttenuationVisual");
    addAudioBytes=Read<uint16_t>(addAudioFunction,0xbe);addNewBytes=Read<uint16_t>(addNewFunction,0xbe);
    // Both original cooked declarations are void. No parameter is read or
    // rewritten by this filter; retain the loaded linker sizes for diagnostics.
    if(!addAudioFunction||!addNewFunction||Read<void*>(addAudioFunction,0x28)!=type||Read<void*>(addNewFunction,0x28)!=type
      ||addAudioBytes<25||addAudioBytes>32||addNewBytes<25||addNewBytes>32)
      return EntryFail(e,2862,"npc_view_attenuation_original_void_methods_invalid");
    Attenuation* available=nullptr;
    for(auto& a:attenuations){
        if(a.bound&&a.object==object&&Live(object,a.objectId)){
            if(a.pawn!=e.pawn||a.controller!=e.controller||actual!=attenuationClone)
              return EntryFail(e,2863,"npc_view_attenuation_existing_owner_or_table_changed");
            return true;
        }
        // Reclaim ONLY an original weak-invalid/destroyed component. A live old
        // corpse keeps its own forwarding/rejection record until native death.
        if(!available&&(!a.bound||!Live(a.object,a.objectId)))available=&a;
    }
    if(!available)return EntryFail(e,2864,"npc_view_attenuation_live_generation_capacity");
    if(actual!=AttenuationSource())return EntryFail(e,2865,"npc_view_attenuation_foreign_primary_table");
    auto& a=*available;if(a.bound)++state.attenuationReclaims;a={};a.index=static_cast<size_t>(&e-state.entries.data());
    a.object=object;a.pawn=e.pawn;a.controller=e.controller;a.playerState=e.playerState;
    if(!original_inventory_slots::CaptureIdentity(object,a.objectId))return EntryFail(e,2866,"npc_view_attenuation_weak_identity_failed");
    a.pawnId=e.pawnId;a.controllerId=e.controllerId;a.stateId=e.stateId;a.bound=true;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(object),attenuationClone,actual)!=actual){a.bound=false;return EntryFail(e,2867,"npc_view_attenuation_instance_install_raced");}
    ++state.attenuationInstalls;return true;
}
inline bool RefreshFootsteps(Entry& e){
    if(e.lifecycleReady&&Live(e.lifecyclePawn,e.lifecyclePawnId)&&e.lifecyclePawn==e.pawn)return true;
    e.footsteps=Read<void*>(e.pawn,0xa78);
    if(!original_players::IsA(e.footsteps,0x685b1f0)||Read<void*>(e.footsteps,0x2f0)!=e.pawn)
      return EntryFail(e,2835,"npc_locality_original_footsteps_owner_invalid");
    e.footstepsLocalBefore=Read<uint8_t>(e.footsteps,0x28c);
    // Original UF parameter is OldController. Native6f327e0 re-evaluates the
    // CURRENT owner predicate. Outside the exact Wwise chains it retains the
    // original local-authority simulation binding, including its own28c state.
    void* oldController=e.controller;++e.lifecycleCalls;
    if(!Event(e.footsteps,L"/Script/ShooterGame.FootstepsComponent.ClientControllerChanged",8,&oldController))
      return EntryFail(e,2836,"npc_locality_original_controller_changed_abi_failed");
    e.footstepsLocalAfter=Read<uint8_t>(e.footsteps,0x28c);
    constexpr const wchar_t* base=L"/Game/UI/Screens/InGame/HUD/Minimap/Components/ShooterCharacterAudioAttenuationComponent.ShooterCharacterAudioAttenuationComponent_C";
    auto type=engine::Find(base);
    if(type){
        struct ComponentParams{void* type=nullptr;void* result=nullptr;} p{type};
        if(!Event(e.pawn,L"/Script/Engine.Actor.GetComponentByClass",16,&p))
          return EntryFail(e,2838,"npc_locality_original_attenuation_lookup_failed");
        e.attenuation=p.result;e.attenuationPresent=p.result!=nullptr;
        if(p.result){
            if(!original_players::Derived(Read<void*>(p.result,0x10),type)||Read<void*>(p.result,0xc8)!=e.pawn)
              return EntryFail(e,2839,"npc_locality_attenuation_component_owner_mismatch");
            if(!InstallAttenuation(e))return false;
            ++e.oneShotClearCalls;
            if(!Event(p.result,L"/Game/UI/Screens/InGame/HUD/Minimap/Components/ShooterCharacterAudioAttenuationComponent.ShooterCharacterAudioAttenuationComponent_C.ClearCurrentOneShotVisualization",0,nullptr))
              return EntryFail(e,2840,"npc_locality_original_one_shot_clear_failed");
            e.oneShotCleared=true;
        }
    }
    e.lifecyclePawn=e.pawn;e.lifecyclePawnId=e.pawnId;e.lifecycleReady=true;return true;
}
inline bool BindImpl(size_t index,void* controller,void* pawn,uint64_t epoch){
    if(!state.initialized||!original_players::Thread()||index>=MaxPlayers)return Fail(2841,"npc_locality_bind_requires_initialized_game_thread");
    auto& e=state.entries[index];const auto& player=original_players::players[index];++state.binds;++e.binds;
    if(static_cast<int>(index)==original_players::localIndex||player.local||!player.ready||!controller||!pawn
      ||player.controller!=controller||player.character!=pawn||!player.state||LocalMembership(controller)!=Membership::NonLocal)
      return EntryFail(e,2842,"npc_locality_bind_requires_actual_npc");
    if(e.bound&&e.controller!=controller)return EntryFail(e,2843,"npc_locality_controller_changed_without_retirement");
    if(!original_players::IsA(controller,original_players::ShooterControllerClass)||!original_players::IsA(pawn,original_players::ShooterCharacterClass)
      ||!original_players::IsA(player.state,original_players::ShooterStateClass)||!SameRoster(player.state)
      ||Read<void*>(controller,0x4b0)!=pawn||Read<void*>(pawn,0x4a0)!=controller
      ||Read<void*>(controller,0x470)!=player.state||Read<void*>(pawn,0x488)!=player.state
      ||Read<void*>(player.state,0x210)!=controller||original_client_lifecycle::ActorWorld(controller)!=state.world
      ||original_client_lifecycle::ActorWorld(pawn)!=state.world)return EntryFail(e,2844,"npc_locality_native_pair_or_roster_invalid");
    auto actual=Read<void**>(controller,0);
    if((actual!=Source()&&actual!=clone)||!SourceMatches()||!CloneMatches())return EntryFail(e,2845,"npc_locality_refuses_foreign_or_changed_table");
    if(actual==clone&&(!e.bound||!e.installed||e.controller!=controller||!Live(controller,e.controllerId)))
      return EntryFail(e,2845,"npc_locality_owned_table_requires_existing_weak_binding");
    for(size_t i=0;i<MaxPlayers;++i)if(i!=index&&state.entries[i].bound&&state.entries[i].controller==controller)
      return EntryFail(e,2845,"npc_locality_controller_already_bound_to_other_roster_index");
    if(!e.bound){
        e.controller=controller;e.playerState=player.state;e.type=Read<void*>(controller,0x10);e.owner=Read<void*>(controller,0x210);
        e.camera=Read<void*>(controller,0x528);e.role=Read<uint8_t>(controller,0x224);e.remoteRole=Read<uint8_t>(controller,0x128);
        e.secondary468=Read<void*>(controller,0x468);e.secondary508=Read<void*>(controller,0x508);
        e.secondaryA68=Read<void*>(controller,0xa68);e.secondaryA70=Read<void*>(controller,0xa70);
        if(!original_inventory_slots::CaptureIdentity(controller,e.controllerId)||!original_inventory_slots::CaptureIdentity(player.state,e.stateId))
          return EntryFail(e,2846,"npc_locality_original_weak_identity_failed");
        e.bound=true;e.retired=false;++state.boundCount;
    }
    if(e.playerState!=player.state||!Live(controller,e.controllerId)||!Live(player.state,e.stateId))return EntryFail(e,2847,"npc_locality_bound_identity_changed");
    const bool newPawn=!Live(e.pawn,e.pawnId)||e.pawn!=pawn;
    if(newPawn){e.lifecycleReady=false;e.attenuationPresent=false;e.oneShotCleared=false;e.lastFootstepTimeValid=false;}
    e.pawn=pawn;if(!original_inventory_slots::CaptureIdentity(pawn,e.pawnId))return EntryFail(e,2848,"npc_locality_pawn_identity_failed");
    if(epoch)e.epoch=epoch;else if(newPawn)++e.epoch;
    e.cacheBefore=Read<uint8_t>(controller,0x8b4);e.installed=true;
    if(actual==Source()){
        if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(controller),clone,actual)!=actual){e.installed=false;return EntryFail(e,2849,"npc_locality_instance_table_install_raced");}
        ++state.installs;
    }
    e.sourceUntouched=SourceMatches();e.otherSlotsPreserved=CloneMatches();
    e.actorFieldsPreserved=Read<void*>(controller,0x10)==e.type&&Read<void*>(controller,0x210)==e.owner
      &&Read<void*>(controller,0x528)==e.camera&&Read<uint8_t>(controller,0x224)==e.role&&Read<uint8_t>(controller,0x128)==e.remoteRole
      &&Read<void*>(controller,0x468)==e.secondary468&&Read<void*>(controller,0x508)==e.secondary508
      &&Read<void*>(controller,0xa68)==e.secondaryA68&&Read<void*>(controller,0xa70)==e.secondaryA70;
    if(!e.sourceUntouched||!e.otherSlotsPreserved||!e.actorFieldsPreserved||!VerifiedNPC(e))return EntryFail(e,2850,"npc_locality_install_postcondition_failed");
    if(!RefreshFootsteps(e))return false;
    e.cacheAfter=Read<uint8_t>(controller,0x8b4);
    e.noPlayer=true;e.notInLocalPlayers=true;e.rosterValid=true;e.fullPair=true;e.unpossessed=false;
    e.error=0;e.stage="original_npc_simulation_local_view_consumers_actual_nonlocal";
    state.error=0;state.stage=e.stage;return true;
}
inline int Exception(EXCEPTION_POINTERS* p){
    const auto at=reinterpret_cast<uintptr_t>(p&&p->ExceptionRecord?p->ExceptionRecord->ExceptionAddress:nullptr);
    state.exceptionRva=at>=engine::image?at-engine::image:at;Fail(2851,"npc_locality_native_exception");return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* world){__try{return InitializeImpl(world);}__except(Exception(GetExceptionInformation())){return false;}}
inline bool Bind(size_t index,void* controller,void* pawn,uint64_t epoch=0){__try{return BindImpl(index,controller,pawn,epoch);}__except(Exception(GetExceptionInformation())){return false;}}
inline bool Refresh(size_t index,uint64_t epoch=0){
    if(index>=MaxPlayers)return Fail(2841,"npc_locality_refresh_index_invalid");++state.refreshes;
    const auto& p=original_players::players[index];return Bind(index,p.controller,p.character,epoch);
}
// Only for final CONTROLLER retirement. Do not call for a corpse/pawn respawn:
// its persistent PC must remain nonlocal while possession is temporarily empty.
inline bool BeforeRetire(size_t index){
    if(!original_players::Thread()||index>=MaxPlayers)return false;auto& e=state.entries[index];if(!e.bound)return true;
    if(Live(e.controller,e.controllerId)){
        auto actual=Read<void**>(e.controller,0);
        if(actual==clone&&InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(e.controller),Source(),clone)!=clone)
          return EntryFail(e,2852,"npc_locality_controller_retire_raced");
        if(actual!=clone&&actual!=Source())return EntryFail(e,2853,"npc_locality_controller_retire_foreign_table");
    }
    for(auto& a:attenuations){if(!a.bound||a.controller!=e.controller)continue;
        if(Live(a.object,a.objectId)){
            auto actual=Read<void**>(a.object,0);
            if(actual==attenuationClone&&InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(a.object),AttenuationSource(),attenuationClone)!=attenuationClone)
              return EntryFail(e,2868,"npc_view_attenuation_retirement_raced");
            if(actual!=attenuationClone&&actual!=AttenuationSource())return EntryFail(e,2869,"npc_view_attenuation_retirement_foreign_table");
        }
        a.bound=false;
    }
    e.installed=false;e.bound=false;e.retired=true;e.stage="npc_locality_controller_retired";
    if(state.boundCount)--state.boundCount;++state.retirements;return true;
}
inline bool SampleImpl(){
    if(!state.initialized||!original_players::Thread())return false;
    ResolveAudioFunctions();
    ResolveSpectatedAudioFunctions();
    state.localPairValid=LocalPair();state.sourceUntouched=SourceMatches();
    if(!state.localPairValid||!state.sourceUntouched||!CloneMatches()||!AttenuationSourceMatches()||!AttenuationCloneMatches())return Fail(2854,"npc_locality_sample_local_or_table_changed");
    for(auto& e:state.entries){if(!e.bound)continue;
        const auto membership=LocalMembership(e.controller);e.noPlayer=Read<void*>(e.controller,0x510)==nullptr;
        e.notInLocalPlayers=membership==Membership::NonLocal;e.rosterValid=SameRoster(e.playerState);
        if(membership==Membership::Local)return EntryFail(e,2855,"npc_locality_npc_became_actual_local_original_forwarded");
        if(membership!=Membership::NonLocal||!VerifiedNPC(e))return EntryFail(e,2856,"npc_locality_sample_npc_identity_or_roster_changed");
        auto currentPawn=Read<void*>(e.controller,0x4b0);e.unpossessed=currentPawn==nullptr;
        e.fullPair=currentPawn&&Read<void*>(currentPawn,0x4a0)==e.controller&&Read<void*>(currentPawn,0x488)==e.playerState;
        e.cacheAfter=Read<uint8_t>(e.controller,0x8b4);
        if(Live(e.pawn,e.pawnId)&&e.footsteps&&Read<void*>(e.pawn,0xa78)==e.footsteps){
            const float time=Read<float>(e.footsteps,0xdc);
            if(e.lastFootstepTimeValid&&time!=e.lastFootstepTime)++e.footstepTimeChanges;
            e.lastFootstepTime=time;e.lastFootstepTimeValid=true;
        }
    }
    ++state.samples;state.error=0;state.stage="npc_locality_verified_original_consumers_use_actual_local_ownership";return true;
}
inline bool Sample(){__try{return SampleImpl();}__except(Exception(GetExceptionInformation())){return false;}}
inline bool WriteDiagnostics(const wchar_t* path){
    FILE* f=nullptr;if(!path||_wfopen_s(&f,path,L"wb")||!f)return false;const auto b=[](bool v){return v?"true":"false";};
    std::fprintf(f,"{\"schema_version\":2,\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"source_verified\":%s,\"source_untouched\":%s,\"clone_read_only\":%s,\"actual_local_pair_valid\":%s,\"native_table_rva\":%llu,\"native_predicate_rva\":%llu,\"table_entries\":%zu,\"replaced_slot_bytes\":2048,\"image_writes\":0,\"local_cache_writes\":0,\"world\":\"%p\",\"clone\":\"%p\",\"bound_count\":%u,\"installs\":%llu,\"binds\":%llu,\"refreshes\":%llu,\"retirements\":%llu,\"samples\":%llu,\"callback_calls\":%llu,\"nonlocal_returns\":%llu,\"original_forwards\":%llu,\"off_thread_original_forwards\":%llu,\"exception_rva\":%llu",
      state.stage,state.error,b(state.initialized),b(state.sourceVerified),b(state.sourceUntouched),b(state.cloneReadOnly),b(state.localPairValid),
      static_cast<unsigned long long>(NativeTableRva),static_cast<unsigned long long>(NativePredicateRva),TableEntries,state.world,static_cast<void*>(clone),state.boundCount,
      static_cast<unsigned long long>(state.installs),static_cast<unsigned long long>(state.binds),static_cast<unsigned long long>(state.refreshes),static_cast<unsigned long long>(state.retirements),static_cast<unsigned long long>(state.samples),
      static_cast<unsigned long long>(state.callbackCalls),static_cast<unsigned long long>(state.nonLocalReturns),static_cast<unsigned long long>(state.originalForwards),static_cast<unsigned long long>(offThreadForwards.load(std::memory_order_relaxed)),static_cast<unsigned long long>(state.exceptionRva));
    std::fprintf(f,",\"simulation_locality_preserved\":true,\"view_consumers_only\":true,\"sound_frame_profile\":%s,\"sound_direct_returns\":%llu,\"sound_view_helper_returns\":%llu,\"attenuation_complete_profile\":%s,\"attenuation_clone_read_only\":%s,\"attenuation_table_entries\":%zu,\"attenuation_installs\":%llu,\"attenuation_reclaims\":%llu,\"attenuation_calls\":%llu,\"attenuation_adds_rejected\":%llu,\"attenuation_calls_forwarded\":%llu,\"add_audio_parameter_bytes\":%u,\"add_new_parameter_bytes\":%u",
      b(state.soundFrameProfile),static_cast<unsigned long long>(state.soundDirectReturns),static_cast<unsigned long long>(state.soundViewHelperReturns),b(state.attenuationProfile),b(state.attenuationCloneReadOnly),AttenuationTableEntries,
      static_cast<unsigned long long>(state.attenuationInstalls),static_cast<unsigned long long>(state.attenuationReclaims),static_cast<unsigned long long>(state.attenuationCalls),static_cast<unsigned long long>(state.attenuationAddsRejected),static_cast<unsigned long long>(state.attenuationCallsForwarded),static_cast<unsigned>(addAudioBytes),static_cast<unsigned>(addNewBytes));
    std::fprintf(f,",\"aimpunch_frame_profile\":%s,\"aimpunch_consumer_matches\":%llu,\"aimpunch_view_gate_returns\":%llu,\"last_aimpunch_controller\":\"%p\",\"blueprint_frame_profile\":%s,\"attenuation_frame_candidates\":%llu,\"attenuation_frame_node_matches\":%llu,\"attenuation_frame_owner_matches\":%llu,\"attenuation_frame_gate_returns\":%llu,\"attenuation_target_node\":\"%p\",\"last_attenuation_frame\":\"%p\",\"last_attenuation_node\":\"%p\",\"last_attenuation_object\":\"%p\"",
      b(state.aimpunchFrameProfile),static_cast<unsigned long long>(state.aimpunchConsumerMatches),static_cast<unsigned long long>(state.aimpunchViewGateReturns),state.lastAimpunchController,b(state.blueprintFrameProfile),static_cast<unsigned long long>(state.attenuationFrameCandidates),static_cast<unsigned long long>(state.attenuationFrameNodeMatches),static_cast<unsigned long long>(state.attenuationFrameOwnerMatches),static_cast<unsigned long long>(state.attenuationFrameGateReturns),addNewFunction,state.lastAttenuationFrame,state.lastAttenuationNode,state.lastAttenuationObject);
    std::fprintf(f,",\"listener_registration_profile\":%s,\"listener_registration_matches\":%llu,\"listener_registration_rejects\":%llu,\"audio_functions_bound\":%u,\"audio_bind_attempts\":%llu,\"audio_bind_mismatches\":%llu,\"audio_frame_candidates\":%llu,\"audio_frame_node_matches\":%llu,\"audio_frame_context_matches\":%llu,\"audio_frame_context_rejects\":%llu,\"audio_frame_code_rejects\":%llu,\"audio_frame_returns\":%llu,\"audio_blueprint_functions\":[",
        b(state.listenerRegistrationProfile),static_cast<unsigned long long>(state.listenerRegistrationMatches),static_cast<unsigned long long>(state.listenerRegistrationRejects),state.audioFunctionsBound,
        static_cast<unsigned long long>(state.audioBindAttempts),static_cast<unsigned long long>(state.audioBindMismatches),static_cast<unsigned long long>(state.audioFrameCandidates),static_cast<unsigned long long>(state.audioFrameNodeMatches),
        static_cast<unsigned long long>(state.audioFrameContextMatches),static_cast<unsigned long long>(state.audioFrameContextRejects),static_cast<unsigned long long>(state.audioFrameCodeRejects),static_cast<unsigned long long>(state.audioFrameReturns));
    for(size_t i=0;i<audioFunctions.size();++i){const auto& a=audioFunctions[i];const auto& p=original_audio_view_consumers::Profiles[i];
        std::fprintf(f,"%s{\"name\":\"%s\",\"bound\":%s,\"function\":\"%p\",\"script_bytes\":%d,\"expected_query_resume\":%u,\"last_code_offset\":%u,\"node_matches\":%llu,\"view_returns\":%llu}",i?",":"",p.name,b(a.function!=nullptr),a.function,p.scriptBytes,p.queryResume,a.lastCodeOffset,static_cast<unsigned long long>(a.nodeMatches),static_cast<unsigned long long>(a.viewReturns));
    }
    std::fprintf(f,"],\"spectated_frame_profile\":%s,\"spectated_audio_functions_bound\":%u,\"spectated_frame_candidates\":%llu,\"spectated_frames_recovered\":%llu,\"spectated_frame_copy_rejects\":%llu,\"spectated_node_matches\":%llu,\"spectated_context_rejects\":%llu,\"spectated_code_rejects\":%llu,\"spectated_view_returns\":%llu,\"spectated_direct_recoveries\":%llu,\"spectated_helper_recoveries\":%llu,\"spectated_audio_functions\":[",
        b(state.spectatedFrameProfile),state.spectatedFunctionsBound,static_cast<unsigned long long>(state.spectatedFrameCandidates),static_cast<unsigned long long>(state.spectatedFramesRecovered),
        static_cast<unsigned long long>(state.spectatedFrameCopyRejects),static_cast<unsigned long long>(state.spectatedNodeMatches),static_cast<unsigned long long>(state.spectatedContextRejects),
        static_cast<unsigned long long>(state.spectatedCodeRejects),static_cast<unsigned long long>(state.spectatedViewReturns),static_cast<unsigned long long>(state.spectatedDirectRecoveries),static_cast<unsigned long long>(state.spectatedHelperRecoveries));
    for(size_t i=0;i<spectatedAudioFunctions.size();++i){const auto& a=spectatedAudioFunctions[i];const auto& p=original_audio_spectated_consumers::Profiles[i];
        std::fprintf(f,"%s{\"name\":\"%s\",\"bound\":%s,\"function\":\"%p\",\"script_bytes\":%d,\"expected_query_resume\":%u,\"last_code_offset\":%u,\"node_matches\":%llu,\"view_returns\":%llu}",i?",":"",p.name,b(a.function!=nullptr),a.function,p.scriptBytes,p.queryResume,a.lastCodeOffset,static_cast<unsigned long long>(a.nodeMatches),static_cast<unsigned long long>(a.viewReturns));
    }
    std::fputs("],\"entries\":[",f);
    bool first=true;for(size_t i=0;i<MaxPlayers;++i){const auto& e=state.entries[i];if(!e.bound&&!e.retired&&!e.error)continue;
        std::fprintf(f,"%s{\"index\":%zu,\"stage\":\"%s\",\"error\":%u,\"bound\":%s,\"installed\":%s,\"retired\":%s,\"controller\":\"%p\",\"pawn\":\"%p\",\"player_state\":\"%p\",\"epoch\":%llu,\"no_player\":%s,\"not_in_local_players\":%s,\"roster_valid\":%s,\"full_pair\":%s,\"temporarily_unpossessed\":%s,\"other_slots_preserved\":%s,\"actor_fields_preserved\":%s,\"source_untouched\":%s,\"cached_local_before\":%u,\"cached_local_after\":%u,\"calls\":%llu,\"nonlocal_returns\":%llu,\"simulation_or_local_original_forwards\":%llu,\"uncertain_original_forwards\":%llu,\"footsteps\":\"%p\",\"footsteps_local_binding_before\":%u,\"footsteps_local_binding_after\":%u,\"original_controller_changed_calls\":%llu,\"attenuation_component\":\"%p\",\"attenuation_present\":%s,\"one_shot_cleared\":%s,\"one_shot_clear_calls\":%llu,\"native_last_footstep_time\":%.9g,\"native_footstep_time_changes\":%llu,\"ground_audio_events_instrumented\":false}",
          first?"":",",i,e.stage,e.error,b(e.bound),b(e.installed),b(e.retired),e.controller,e.pawn,e.playerState,static_cast<unsigned long long>(e.epoch),b(e.noPlayer),b(e.notInLocalPlayers),b(e.rosterValid),b(e.fullPair),b(e.unpossessed),b(e.otherSlotsPreserved),b(e.actorFieldsPreserved),b(e.sourceUntouched),e.cacheBefore,e.cacheAfter,
          static_cast<unsigned long long>(e.calls),static_cast<unsigned long long>(e.nonLocalReturns),static_cast<unsigned long long>(e.localForwards),static_cast<unsigned long long>(e.uncertainForwards),e.footsteps,e.footstepsLocalBefore,e.footstepsLocalAfter,static_cast<unsigned long long>(e.lifecycleCalls),e.attenuation,b(e.attenuationPresent),b(e.oneShotCleared),static_cast<unsigned long long>(e.oneShotClearCalls),static_cast<double>(e.lastFootstepTime),static_cast<unsigned long long>(e.footstepTimeChanges));first=false;
    }
    std::fprintf(f,"],\"pid\":%lu}",GetCurrentProcessId());std::fclose(f);return true;
}
}
