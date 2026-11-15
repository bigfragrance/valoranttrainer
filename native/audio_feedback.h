#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cmath>
#include "engine_api.h"

// Original installed Wwise event assets, posted through the pinned client's
// reflected AkAudioEvent method. Invoke only from the existing game-thread tick.
// A nonzero PlayingID means the event was accepted; it is NOT proof of audible
// output, a valid listener, loaded media, or exact original animation timing.
namespace audio_feedback {

inline constexpr uintptr_t kAkAudioEventClass = 0x592d1b0;
inline constexpr const wchar_t* kPostAtLocation = L"/Script/AkAudio.AkAudioEvent.PostAtLocation";
inline constexpr const wchar_t* kFire =
    L"/Game/WwiseAudio/Events/SFX/Weapons/Events_Guns_Firing/AK47/Play_Wp_Gun_AK47_Fire.Play_Wp_Gun_AK47_Fire";
inline constexpr const wchar_t* kReloadStart =
    L"/Game/WwiseAudio/Events/SFX/Weapons/Events_Guns_RE/Ak47_RE/Play_Wp_AK47_MagOut_FP.Play_Wp_AK47_MagOut_FP";
inline constexpr const wchar_t* kReloadEnd =
    L"/Game/WwiseAudio/Events/SFX/Weapons/Events_Guns_RE/Ak47_RE/Play_Wp_AK47_MagIn_End_FP.Play_Wp_AK47_MagIn_End_FP";

// Exact generated function parameter layout: FParams RVA 0xd4b9120,
// parameter descriptor array RVA 0xc0e9ba0. The client's own exec wrapper
// initializes an unbound 20-byte script delegate to zero. CallbackMask remains 0.
struct alignas(8) PostAtLocationParameters {
    engine::Vec location{};                 // 0x00
    engine::Vec orientation{};              // 0x18
    uint8_t callback[20]{};                 // 0x30, unbound, no callback registered
    int32_t callbackMask = 0;               // 0x44
    void* worldContext = nullptr;           // 0x48
    int32_t playingId = 0;                  // 0x50
    uint32_t trailingPadding = 0;
};
static_assert(sizeof(PostAtLocationParameters) == 0x58);
static_assert(offsetof(PostAtLocationParameters, location) == 0x00);
static_assert(offsetof(PostAtLocationParameters, orientation) == 0x18);
static_assert(offsetof(PostAtLocationParameters, callback) == 0x30);
static_assert(offsetof(PostAtLocationParameters, callbackMask) == 0x44);
static_assert(offsetof(PostAtLocationParameters, worldContext) == 0x48);
static_assert(offsetof(PostAtLocationParameters, playingId) == 0x50);

enum Error : uint32_t {
    None = 0, MissingContext = 1, MissingReflection = 2,
    MissingAsset = 3, InvalidPlayingId = 4, InvalidCoordinates = 5
};

struct Diagnostics {
    bool initialized = false;
    bool fireAssetLoaded = false;
    bool reloadStartAssetLoaded = false;
    bool reloadEndAssetLoaded = false;
    bool reflectionAvailable = false;
    uint32_t postAttempts = 0;
    uint32_t acceptedPosts = 0;
    uint32_t failedPosts = 0;
    uint32_t lastPlayingId = 0;
    uint32_t lastFirePlayingId = 0;
    uint32_t lastReloadPlayingId = 0;
    uint32_t lastObservedShots = 0;
    uint32_t droppedShotBacklog = 0;
    uint32_t lastError = None;
};

inline Diagnostics diagnostics;
inline void* worldContext = nullptr;
inline bool previousReloading = false;

inline bool Finite(const engine::Vec& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

inline void* ResolveEvent(const wchar_t* path) {
    // Resolve afresh at each post. We deliberately do not retain unrooted raw
    // asset pointers across ticks or change the game's GC/root flags.
    void* type = engine::Class(kAkAudioEventClass);
    return type ? engine::Load(type, path) : nullptr;
}

inline bool Initialize(void* world, void* pawn) {
    diagnostics = {};
    previousReloading = false;
    worldContext = world ? world : pawn;
    if (!engine::image || !worldContext) {
        diagnostics.lastError = MissingContext;
        return false;
    }
    diagnostics.reflectionAvailable = engine::Find(kPostAtLocation) != nullptr;
    if (!diagnostics.reflectionAvailable) {
        diagnostics.lastError = MissingReflection;
        return false;
    }
    diagnostics.fireAssetLoaded = ResolveEvent(kFire) != nullptr;
    diagnostics.reloadStartAssetLoaded = ResolveEvent(kReloadStart) != nullptr;
    diagnostics.reloadEndAssetLoaded = ResolveEvent(kReloadEnd) != nullptr;
    diagnostics.initialized = diagnostics.fireAssetLoaded;
    diagnostics.lastError = diagnostics.fireAssetLoaded ? None : MissingAsset;
    return diagnostics.initialized;
}

inline uint32_t Post(const wchar_t* path, const engine::Vec& position, const engine::Vec& rotation) {
    ++diagnostics.postAttempts;
    if (!worldContext || !Finite(position) || !Finite(rotation)) {
        diagnostics.lastError = worldContext ? InvalidCoordinates : MissingContext;
        ++diagnostics.failedPosts;
        return 0;
    }
    void* event = ResolveEvent(path);
    if (!event) {
        diagnostics.lastError = MissingAsset;
        ++diagnostics.failedPosts;
        return 0;
    }
    PostAtLocationParameters parameters{};
    parameters.location = position;
    parameters.orientation = rotation;
    parameters.worldContext = worldContext;
    if (!engine::Event(event, kPostAtLocation, &parameters)) {
        diagnostics.lastError = MissingReflection;
        ++diagnostics.failedPosts;
        return 0;
    }
    const uint32_t id = static_cast<uint32_t>(parameters.playingId);
    diagnostics.lastPlayingId = id;
    if (id) {
        ++diagnostics.acceptedPosts;
        diagnostics.lastError = None;
    } else {
        ++diagnostics.failedPosts;
        diagnostics.lastError = InvalidPlayingId;
    }
    return id;
}

inline bool Update(uint32_t shots, bool reloading,
                   const engine::Vec& cameraPosition, const engine::Vec& cameraRotation) {
    if (!diagnostics.initialized) return false;
    bool accepted = true;
    // Counter reset is a new local session, never a 2^32-event backlog.
    uint32_t delta = shots >= diagnostics.lastObservedShots ? shots - diagnostics.lastObservedShots : 0;
    diagnostics.lastObservedShots = shots;
    if (delta > 4) {
        diagnostics.droppedShotBacklog += delta - 4;
        delta = 4;
    }
    for (uint32_t index = 0; index < delta; ++index) {
        diagnostics.lastFirePlayingId = Post(kFire, cameraPosition, cameraRotation);
        accepted = diagnostics.lastFirePlayingId != 0 && accepted;
    }
    // Original first-person reload event fragments, scheduled on LOCAL reload
    // edges. This is not a reconstruction of original animation-notify timing.
    if (reloading != previousReloading) {
        if (reloading && diagnostics.reloadStartAssetLoaded) {
            diagnostics.lastReloadPlayingId = Post(kReloadStart, cameraPosition, cameraRotation);
            accepted = diagnostics.lastReloadPlayingId != 0 && accepted;
        } else if (!reloading && diagnostics.reloadEndAssetLoaded) {
            diagnostics.lastReloadPlayingId = Post(kReloadEnd, cameraPosition, cameraRotation);
            accepted = diagnostics.lastReloadPlayingId != 0 && accepted;
        }
    }
    previousReloading = reloading;
    return accepted;
}

inline const Diagnostics& State() { return diagnostics; }
}  // namespace audio_feedback
