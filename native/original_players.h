#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include "engine_api.h"
#include <array>
#include <cstdio>
#include <cwchar>
#include <string>
#include <limits>
#include <type_traits>

// Pinned client only. Evidence: duel1-original-player-integration.json and
// duel1-camera-calibration.json. Every API runs on the owned game thread.
// Initialize/Diagnose load classes/CDOs but do not create gameplay actors.
namespace original_players {
inline constexpr size_t MaxPlayers = 32;
inline constexpr size_t DefaultCount = 3;
inline constexpr const wchar_t* DefaultPaths[DefaultCount] = {
    L"/Game/Characters/_Core/BasePawn.BasePawn_C",
    L"/Game/Characters/_Core/BasePlayerCharacter.BasePlayerCharacter_C",
    L"/Game/Characters/Wushu/Wushu_PC.Wushu_PC_C"
};
inline constexpr uintptr_t ShooterCharacterClass = 0x6c5c480;
inline constexpr uintptr_t ShooterControllerClass = 0x6943db0;
inline constexpr uintptr_t ShooterStateClass = 0x6cc4040;
inline constexpr uintptr_t ShooterGameStateClass = 0x6c9d280;
struct Options {
    // Explicit development stages. Diagnostics are the initial/default mode.
    bool createGameState = false;
    bool createPlayers = false;
    bool transferLocalPlayer = false;
    // The native ShooterPlayerState has no TeamComponent subobject. The shipped
    // training blueprint inherits BombPlayerState's BombTeam SCS component.
    const wchar_t* playerStateClassPath = L"/Game/GameModes/ShootingRange/ShootingRangePlayerState.ShootingRangePlayerState_C";
    bool freeForAll = false;
    const wchar_t* controllerClassPath = nullptr;
    const wchar_t* gameStateClassPath = nullptr;
};
inline Options options;
inline unsigned error = 0;
inline const char* stage = "original_players_not_initialized";
inline bool initialized = false, restartRequired = false;
inline void* nativeExceptionAddress = nullptr;
inline const char* nativeExceptionOperation = "none";
inline DWORD gameThread = 0;
inline void* world = nullptr;
inline void* fallbackController = nullptr;
inline void* fallbackPawn = nullptr;
inline void* activeController = nullptr;
inline void* activePawn = nullptr;
inline void* localPlayer = nullptr;
inline void* createdGameState = nullptr;
inline void* previousGameState = nullptr;
inline void* selectedPlayerStateClass = nullptr;
inline void* selectedTeamTemplate = nullptr;

inline bool Readable(const void* address, size_t size) {
    uintptr_t at = reinterpret_cast<uintptr_t>(address);
    if (!at || !size || size > (std::numeric_limits<uintptr_t>::max)() - at) return false;
    const uintptr_t end = at + size;
    while (at < end) {
        MEMORY_BASIC_INFORMATION info{};
        if (!VirtualQuery(reinterpret_cast<const void*>(at), &info, sizeof(info))
            || info.State != MEM_COMMIT || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
        const uintptr_t next = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
        if (next <= at) return false;
        at = next;
    }
    return true;
}
template<class T> inline bool Read(const void* object, size_t offset, T& value) {
    static_assert(std::is_trivially_copyable_v<T>);
    const auto base=reinterpret_cast<uintptr_t>(object);
    if(base<0x10000||offset>(std::numeric_limits<uintptr_t>::max)()-base)return false;
    const auto address=base+offset;
    if(sizeof(value)>(std::numeric_limits<uintptr_t>::max)()-address)return false;
    // These are already-identified, world-owned objects on the game thread.
    // Querying the OS memory map for every eight-byte field in every frame
    // caused an observed GameThread hang inside VirtualQuery (visual-probe1).
    // A guarded POD copy keeps invalid reads contained without that hot query.
    __try { std::memcpy(&value,reinterpret_cast<const void*>(address),sizeof(value));return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
template<class T> inline T Value(const void* object, size_t offset) {
    T value{}; Read(object, offset, value); return value;
}
template<class T> inline void Put(uint8_t* buffer, size_t offset, const T& value) {
    std::memcpy(buffer + offset, &value, sizeof(value));
}
template<class T> inline bool Write(void* object, size_t offset, const T& value) {
    if (!object) return false;
    auto address = reinterpret_cast<uint8_t*>(object) + offset;
    if (!Readable(address, sizeof(value))) return false;
    std::memcpy(address, &value, sizeof(value)); return true;
}
inline bool Finite(const engine::Vec& v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
inline bool Fail(unsigned code, const char* message) { error = code; stage = message; return false; }
inline bool Thread() {
    if (!engine::image || !gameThread || gameThread != GetCurrentThreadId()) return Fail(701, "original_wrong_game_thread");
    if (restartRequired) return Fail(error ? error : 702, "original_native_exception_requires_new_session");
    return true;
}
inline bool Derived(void* type, void* base) {
    if (!type || !base) return false;
    int depth = 0, actualDepth = 0;
    void** chain = nullptr;
    if (!Read(base, 0x40, depth) || !Read(type, 0x40, actualDepth) || !Read(type, 0x38, chain)
        || depth < 0 || depth > 1024 || actualDepth < depth) return false;
    void* entry = nullptr;
    return Read(chain, static_cast<size_t>(depth) * sizeof(void*), entry)
        && entry == reinterpret_cast<uint8_t*>(base) + 0x38;
}
inline bool IsA(void* object, uintptr_t getter) {
    return object && Derived(Value<void*>(object, 0x10), engine::Class(getter));
}
inline void* LoadPawnClass(const wchar_t* path) {
    if (!path || !*path) return nullptr;
    return engine::function<void*(*)(void*, void*, const wchar_t*, const wchar_t*, uint32_t)>(0x1d0c6e0)
        (engine::Class(ShooterCharacterClass), nullptr, path, nullptr, 0);
}
inline void* LoadStateClass(const wchar_t* path) {
    if (!path || !*path) return nullptr;
    return engine::function<void*(*)(void*, void*, const wchar_t*, const wchar_t*, uint32_t)>(0x1d0c6e0)
        (engine::Class(ShooterStateClass), nullptr, path, nullptr, 0);
}
inline bool ResolvePlayerStateClass() {
    selectedPlayerStateClass = LoadStateClass(options.playerStateClassPath);
    if (options.freeForAll) {
        void* ffaClass = LoadStateClass(L"/Game/GameModes/Common/FFAPlayerState.FFAPlayerState_C");
        if (!selectedPlayerStateClass || !ffaClass || !Derived(selectedPlayerStateClass, ffaClass)
            || (Value<uint32_t>(selectedPlayerStateClass, 0xdc) & 1)) return false;
        selectedTeamTemplate = engine::function<void*(*)(void*, void*, const wchar_t*, bool)>(0x1d07dd0)
            (engine::Class(0x6849cb0), ffaClass, L"FFATeam_GEN_VARIABLE", false);
        return IsA(selectedTeamTemplate, 0x65a0b40);
    }
    void* bombClass = LoadStateClass(L"/Game/GameModes/Bomb/BombPlayerState.BombPlayerState_C");
    if (!selectedPlayerStateClass || !bombClass || !Derived(selectedPlayerStateClass, bombClass)
        || (Value<uint32_t>(selectedPlayerStateClass, 0xdc) & 1)) return false;
    selectedTeamTemplate = engine::function<void*(*)(void*, void*, const wchar_t*, bool)>(0x1d07dd0)
        (engine::Class(0x65e4040), bombClass, L"BombTeam_GEN_VARIABLE", false);
    // Exact shipped export: BombPlayerState_C owns this BombTeamComponent
    // construction template. It is instantiated by normal Blueprint construction;
    // PS.TeamComponent+6a8 is only cached later by PostInitializeComponents.
    return IsA(selectedTeamTemplate, 0x65a0b40);
}
struct ComponentDefaults {
    void* object = nullptr;
    engine::Vec location{}, rotation{}, scale{};
    bool valid = false;
};
struct CharacterDefaults {
    wchar_t path[512]{};
    void* type = nullptr;
    void* cdo = nullptr;
    uint32_t classFlags = 0;
    bool loaded = false, abstractClass = true, valid = false;
    float capsuleHalfHeight = 0, capsuleRadius = 0, baseEyeHeight = 0;
    float characterScale = 0, addedEyeHeight = 0, standingEyeOffset = 0, crouchingEyeOffset = 0;
    double unitScaleStandingViewOffset = 0;
    ComponentDefaults root, capsule, mesh, mesh1P, meshOverlay1P, meshCosmetic3P;
    void* inventory = nullptr;
    void* abilitiesAndBuffs = nullptr;
    void* inputState = nullptr;
};
inline std::array<CharacterDefaults, DefaultCount> defaults{};
inline ComponentDefaults Component(void* pointer) {
    ComponentDefaults d{}; d.object = pointer;
    // SceneComponent descriptors: RelativeLocation+170, Rotation+188, Scale3D+1a0.
    d.valid = Read(pointer, 0x170, d.location) && Read(pointer, 0x188, d.rotation)
        && Read(pointer, 0x1a0, d.scale) && Finite(d.location) && Finite(d.rotation) && Finite(d.scale);
    return d;
}
inline bool ProbeDefaults(const wchar_t* path, CharacterDefaults& d) {
    d = CharacterDefaults{};
    if (!path || wcslen(path) >= 512) return false;
    wcscpy_s(d.path, path);
    d.type = LoadPawnClass(path);
    if (!d.type || !Derived(d.type, engine::Class(ShooterCharacterClass))) return false;
    d.loaded = Read(d.type, 0xdc, d.classFlags);
    d.abstractClass = (d.classFlags & 1) != 0;
    d.cdo = engine::DefaultObject(d.type);
    if (!d.cdo) return false;
    d.root = Component(Value<void*>(d.cdo, 0x290));
    d.capsule = Component(Value<void*>(d.cdo, 0x500));
    d.mesh = Component(Value<void*>(d.cdo, 0x4f0));
    d.mesh1P = Component(Value<void*>(d.cdo, 0xf30));
    d.meshOverlay1P = Component(Value<void*>(d.cdo, 0xf38));
    d.meshCosmetic3P = Component(Value<void*>(d.cdo, 0xf40));
    d.inventory = Value<void*>(d.cdo, 0xc08);
    d.abilitiesAndBuffs = Value<void*>(d.cdo, 0xa20);
    d.inputState = Value<void*>(d.cdo, 0xba0);
    d.valid = Read(d.capsule.object, 0x6d8, d.capsuleHalfHeight)
        && Read(d.capsule.object, 0x6dc, d.capsuleRadius)
        && Read(d.cdo, 0x474, d.baseEyeHeight) && Read(d.cdo, 0x1530, d.characterScale)
        && Read(d.cdo, 0x1090, d.addedEyeHeight) && Read(d.cdo, 0x1094, d.standingEyeOffset)
        && Read(d.cdo, 0x1098, d.crouchingEyeOffset);
    d.valid = d.valid && d.root.valid && d.capsule.valid && std::isfinite(d.capsuleHalfHeight)
        && std::isfinite(d.capsuleRadius) && std::isfinite(d.characterScale)
        && std::isfinite(d.addedEyeHeight) && std::isfinite(d.standingEyeOffset)
        && std::isfinite(d.crouchingEyeOffset) && d.capsuleHalfHeight > 0 && d.capsuleRadius > 0;
    // A formula diagnostic for upright unit scale, not a measured world eye height.
    d.unitScaleStandingViewOffset = d.capsuleHalfHeight + d.characterScale * d.standingEyeOffset + d.addedEyeHeight;
    return d.loaded && d.valid;
}
struct WorldDiagnostics {
    void* mode = nullptr; void* state = nullptr; void* instance = nullptr;
    void* clientInstance = nullptr; void* contentManager = nullptr; void* settingsManager = nullptr;
    void* menuStack = nullptr; void* fixedStep = nullptr; void* camera = nullptr;
    bool originalState = false, originalInstance = false, settingsInitialized = false;
    uint8_t actualMoveRate = 0, targetMoveRate = 0, fixedStepFlags = 0;
    engine::Vec cameraPosition{}, cameraRotation{};
    float defaultFOV = 0, lockedFOV = 0, povFOV = 0;
    bool cameraValid = false;
    int nativePlayerCount = 0;
};
inline WorldDiagnostics diagnostics{};
inline void RefreshWorldDiagnostics() {
    diagnostics = WorldDiagnostics{};
    diagnostics.mode = Value<void*>(world, 0x170); diagnostics.state = Value<void*>(world, 0x178);
    diagnostics.instance = Value<void*>(world, 0x1d8);
    diagnostics.originalState = IsA(diagnostics.state, ShooterGameStateClass);
    diagnostics.originalInstance = IsA(diagnostics.instance, 0x647e8a0);
    if (diagnostics.originalInstance) {
        diagnostics.clientInstance = Value<void*>(diagnostics.instance, 0x2b8);
        diagnostics.contentManager = Value<void*>(diagnostics.instance, 0x2c8);
    } else if (IsA(diagnostics.instance, 0x6429080)) diagnostics.clientInstance = diagnostics.instance;
    if (IsA(diagnostics.clientInstance, 0x6429080)) {
        diagnostics.settingsManager = Value<void*>(diagnostics.clientInstance, 0x378);
        diagnostics.menuStack = Value<void*>(diagnostics.clientInstance, 0x580);
        diagnostics.settingsInitialized = diagnostics.settingsManager && Value<uint8_t>(diagnostics.settingsManager, 0x890) != 0;
    }
    if (diagnostics.originalState) {
        diagnostics.fixedStep = Value<void*>(diagnostics.state, 0xcc0);
        Read(diagnostics.fixedStep, 0xd8, diagnostics.targetMoveRate);
        Read(diagnostics.fixedStep, 0xd9, diagnostics.actualMoveRate);
        Read(diagnostics.fixedStep, 0xc2, diagnostics.fixedStepFlags);
    }
    diagnostics.nativePlayerCount = Value<int>(diagnostics.state, 0x488);
    diagnostics.camera = Value<void*>(activeController, 0x528);
    if (diagnostics.camera) {
        Read(diagnostics.camera, 0x484, diagnostics.defaultFOV);
        Read(diagnostics.camera, 0x48c, diagnostics.lockedFOV);
        auto table = Value<void**>(diagnostics.camera, 0);
        auto getter = Value<void*>(table, 0x7a0);
        if (getter) {
            const void* pov = reinterpret_cast<const void*(*)(void*)>(getter)(diagnostics.camera);
            diagnostics.cameraValid = Read(pov, 0, diagnostics.cameraPosition)
                && Read(pov, 0x18, diagnostics.cameraRotation) && Read(pov, 0x30, diagnostics.povFOV)
                && Finite(diagnostics.cameraPosition) && Finite(diagnostics.cameraRotation)
                && std::isfinite(diagnostics.povFOV) && diagnostics.povFOV > 1 && diagnostics.povFOV < 179;
        }
    }
}
inline bool Diagnose() {
    if (!Thread()) return false;
    stage = "original_reading_character_defaults";
    bool all = true;
    for (size_t i = 0; i < DefaultCount; ++i) if (!ProbeDefaults(DefaultPaths[i], defaults[i])) all = false;
    RefreshWorldDiagnostics();
    if (!all) return Fail(703, "original_some_character_defaults_unavailable");
    error = 0; stage = "original_diagnostics_ready"; return true;
}
inline bool Initialize(void* inWorld, void* currentController, void* currentPawn) {
    if (initialized) return Thread();
    if (!engine::image || !inWorld || !currentController || !currentPawn) return Fail(704, "original_context_missing");
    gameThread = GetCurrentThreadId(); world = inWorld;
    fallbackController = activeController = currentController;
    fallbackPawn = activePawn = currentPawn;
    localPlayer = Value<void*>(currentController, 0x510);
    initialized = true;
    return Diagnose();
}

struct PlayerEntry {
    wchar_t requestedName[64]{};
    wchar_t classPath[512]{};
    engine::Vec feetPosition{};
    double yaw = 0;
    bool configured = false, enabled = true, attempted = false, ready = false, local = false;
    bool registered = false, characterInitialized = false;
    bool controllerUIFactorySuppressed = false;
    uint8_t controllerUIFactoryBefore = 0xff, controllerUIFactoryAfter = 0xff;
    // Names here are editable training roster labels. No unverified PlayerState
    // name/GUID/team setter is called; native naming/selection is reported separately.
    bool nativeNameApplied = false;
    void* controller = nullptr; void* state = nullptr; void* character = nullptr;
    void* inventory = nullptr; void* mesh1P = nullptr; void* abilities = nullptr;
    void* attributes = nullptr; void* movement = nullptr;
    const char* stateText = "unconfigured";
};
inline std::array<PlayerEntry, MaxPlayers> players{};
inline int localIndex = -1;
inline bool fallbackHiddenByBridge = false;
inline bool fallbackCollisionDisabledByBridge = false;
inline engine::Vec savedControlRotation{};
inline float savedDefaultFOV = 103, savedLockedFOV = 103;

inline bool BindFallback(void* currentController, void* currentPawn) {
    if (!Thread() || localIndex >= 0 || !currentController || !currentPawn
        || Value<void*>(currentController, 0x4b0) != currentPawn)
        return Fail(706, "original_fallback_must_be_current_possessed_pawn");
    void* player = Value<void*>(currentController, 0x510);
    if (!player || !IsA(player, 0x3b6e680) || Value<void*>(player, 0x38) != currentController)
        return Fail(707, "original_fallback_local_player_pair_missing");
    fallbackController = activeController = currentController;
    fallbackPawn = activePawn = currentPawn; localPlayer = player;
    fallbackHiddenByBridge = false; fallbackCollisionDisabledByBridge = false;
    return true;
}

inline bool Configure(size_t index, const wchar_t* label, const wchar_t* pawnClass,
                      const engine::Vec& feetPosition, double yaw = 0, bool local = false) {
    if (!Thread() || index >= MaxPlayers || !label || !pawnClass || wcslen(label) >= 64
        || wcslen(pawnClass) >= 512 || !Finite(feetPosition) || !std::isfinite(yaw))
        return Fail(710, "original_invalid_roster_entry");
    auto& entry = players[index];
    if (entry.attempted) return Fail(711, "original_entry_already_created_or_attempted");
    entry = PlayerEntry{};
    wcscpy_s(entry.requestedName, label); wcscpy_s(entry.classPath, pawnClass);
    entry.feetPosition = feetPosition; entry.yaw = yaw; entry.local = local;
    entry.configured = true; entry.stateText = "configured"; return true;
}
inline bool SetRequestedName(size_t index, const wchar_t* name) {
    if (!Thread() || index >= MaxPlayers || !name || wcslen(name) >= 64) return false;
    wcscpy_s(players[index].requestedName, name); players[index].nativeNameApplied = false; return true;
}
inline bool Registered(void* state) {
    void* gs = Value<void*>(world, 0x178);
    void** array = Value<void**>(gs, 0x480);
    const int count = Value<int>(gs, 0x488), capacity = Value<int>(gs, 0x48c);
    if (!state || count < 0 || count > 4096 || capacity < count || (count && !array)) return false;
    for (int i = 0; i < count; ++i)
        if (Value<void*>(array, static_cast<size_t>(i) * sizeof(void*)) == state) return true;
    return false;
}
inline bool RegisterState(void* state) {
    if (!Thread() || !IsA(state, ShooterStateClass)) return Fail(712, "original_invalid_player_state");
    void* gs = Value<void*>(world, 0x178);
    if (!IsA(gs, ShooterGameStateClass)) return Fail(713, "original_game_state_required");
    if (!Registered(state)) {
        stage = "original_registering_player_state";
        auto table = Value<void**>(gs, 0);
        auto fn = Value<void*>(table, 0x7b8);
        if (!fn) return Fail(714, "original_state_registration_missing");
        reinterpret_cast<void(*)(void*, void*)>(fn)(gs, state);
    }
    return Registered(state);
}
inline bool WorldBegun() {
    void* level = Value<void*>(world, 0x38);
    return (Value<uint8_t>(world, 0x156) & 0x80) && level && Value<int>(level, 0xa8) != 0;
}
inline bool StateReady() {
    RefreshWorldDiagnostics();
    return diagnostics.originalState && diagnostics.fixedStep && diagnostics.actualMoveRate > 0
        && (diagnostics.fixedStepFlags & 0x40) != 0;
}
inline void* Begin(void* type, const engine::Transform& transform, void* owner) {
    return engine::function<void*(*)(void*, void*, const engine::Transform*, uint8_t, void*, uint8_t)>(0x43fada0)
        (world, type, &transform, 1, owner, 0);
}
inline void Finish(void* actor, const engine::Transform& transform) {
    engine::function<void(*)(void*, const engine::Transform*, bool, void*, uint8_t)>(0x3ef6cb0)
        (actor, &transform, false, nullptr, 0);
}
inline LONG RecordException(EXCEPTION_POINTERS* exception) {
    error = exception && exception->ExceptionRecord ? exception->ExceptionRecord->ExceptionCode : 0xe0000702U;
    nativeExceptionAddress = exception && exception->ExceptionRecord ? exception->ExceptionRecord->ExceptionAddress : nullptr;
    nativeExceptionOperation = stage;
    restartRequired = true; stage = "original_native_exception_requires_new_session";
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool CreateGameStateImpl() {
    if (!Thread()) return false;
    if (StateReady()) return true;
    if (!options.createGameState) return Fail(720, "original_game_state_creation_disabled");
    if (createdGameState) return Fail(721, "original_created_state_not_ready");
    if (!WorldBegun() || !IsA(diagnostics.mode, 0x439aa40)) return Fail(722, "original_running_authoritative_world_required");
    previousGameState = diagnostics.state;
    const engine::Transform transform{};
    stage = "original_begin_game_state";
    void* selectedStateClass = engine::Class(ShooterGameStateClass);
    if (options.gameStateClassPath) {
        selectedStateClass = engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t)>(0x1d0c6e0)
            (selectedStateClass,nullptr,options.gameStateClassPath,nullptr,0);
        if (!selectedStateClass || !Derived(selectedStateClass,engine::Class(ShooterGameStateClass)))
            return Fail(761,"original_selected_game_state_class_invalid");
    }
    createdGameState = Begin(selectedStateClass, transform, diagnostics.mode);
    if (!createdGameState) return Fail(723, "original_game_state_spawn_failed");
    // Exact original GameMode.PreInitializeComponents links, supplied before
    // BeginPlay. GameState.PostInitializeComponents performs World registration.
    if (!Write(createdGameState, 0x470, diagnostics.mode)) return Fail(724, "original_game_state_authority_link_failed");
    stage = "original_finish_game_state"; Finish(createdGameState, transform);
    if (Value<void*>(world, 0x178) != createdGameState) return Fail(725, "original_game_state_not_registered");
    if (!Write(diagnostics.mode, 0x4d8, createdGameState)) return Fail(726, "original_mode_state_link_failed");
    // Verified GameModeBase.InitGameState virtual+7d0 ->43cfb40: copies
    // GameModeClass/SpectatorClass and invokes the state's native notifications.
    stage = "original_initialize_game_state_mode_fields";
    auto modeTable = Value<void**>(diagnostics.mode, 0);
    auto initializeState = Value<void*>(modeTable, 0x7d0);
    if (!initializeState) return Fail(729, "original_init_game_state_entry_missing");
    reinterpret_cast<void(*)(void*)>(initializeState)(diagnostics.mode);
    if (!StateReady()) return Fail(727, "original_fixed_step_beginplay_incomplete");
    error = 0; stage = "original_game_state_ready"; return true;
}
inline bool CreateGameState() {
    __try { return CreateGameStateImpl(); }
    __except (RecordException(GetExceptionInformation())) { return false; }
}
inline bool RestoreGameState() {
    if (!Thread() || localIndex >= 0 || !createdGameState || !previousGameState) return false;
    for (const auto& entry : players) if (entry.attempted) return Fail(728, "original_state_restore_requires_no_created_players");
    stage = "original_restore_previous_game_state";
    engine::function<void(*)(void*, void*)>(0x4ce47c0)(world, previousGameState);
    void* mode = Value<void*>(world, 0x170);
    if (!Write(mode, 0x4d8, previousGameState) || Value<void*>(world, 0x178) != previousGameState) return false;
    engine::Event(createdGameState, L"/Script/Engine.Actor.K2_DestroyActor", nullptr);
    createdGameState = nullptr; previousGameState = nullptr;
    stage = "original_previous_state_restored"; return true;
}
inline bool ConfigureNonlocalControllerUI(void* controller,PlayerEntry& entry){
    if(entry.local||!options.freeForAll)return true;
    if(!IsA(controller,ShooterControllerClass)||Value<void*>(controller,0x510))
        return Fail(781,"original_npc_ui_factory_requires_nonlocal_controller");
    // The original DM Blueprint enables this NativeBool for client controllers.
    // Its native PostInitializeComponents creates a ticking UIManager/UIDirector
    // even before UPlayer is assigned. NPCs must not create local UI services.
    uint32_t name[2]{};
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(name,L"bCreateUIInstancesFromGlobals",1);
    auto property=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(Value<void*>(controller,0x10),name,3);
    if(!property||Value<int32_t>(property,0x44)!=0xc28||Value<int32_t>(property,0x30)!=1
       ||Value<int32_t>(property,0x34)!=1||Value<uint8_t>(property,0x78)!=1
       ||Value<uint8_t>(property,0x79)!=0||Value<uint8_t>(property,0x7a)!=1
       ||Value<uint8_t>(property,0x7b)!=0xff)
        return Fail(782,"original_npc_ui_factory_native_bool_profile_mismatch");
    if(Value<void*>(controller,0xc18)||Value<void*>(controller,0xc20))
        return Fail(783,"original_npc_ui_factory_must_precede_post_initialize");
    entry.controllerUIFactoryBefore=Value<uint8_t>(controller,0xc28);
    const uint8_t disabled=0;
    if(!Write(controller,0xc28,disabled)||Value<uint8_t>(controller,0xc28)!=0)
        return Fail(784,"original_npc_ui_factory_instance_configuration_failed");
    entry.controllerUIFactoryAfter=0;entry.controllerUIFactorySuppressed=true;
    return true;
}
inline void* SpawnControllerWithOriginalState(const engine::Transform& transform, void* wantedClass,PlayerEntry& entry) {
    void* mode = Value<void*>(world, 0x170);
    void* priorClass = Value<void*>(mode, 0x490);
    void* result = nullptr;
    void* controllerClass = engine::Class(ShooterControllerClass);
    if (options.controllerClassPath) {
        controllerClass = engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t)>(0x1d0c6e0)
            (controllerClass,nullptr,options.controllerClassPath,nullptr,0);
        if (!controllerClass || !Derived(controllerClass,engine::Class(ShooterControllerClass))) return nullptr;
    }
    // A normal native InitPlayerState owns allocation, Owner linkage and +470.
    // Restore the per-instance class selection even if native initialization faults.
    __try {
        if (Write(mode, 0x490, wantedClass)) {
            result = Begin(controllerClass, transform, nullptr);
            if(result&&!ConfigureNonlocalControllerUI(result,entry))result=nullptr;
            if(result){
                Finish(result, transform);
                if(entry.controllerUIFactorySuppressed&&(Value<void*>(result,0xc18)||Value<void*>(result,0xc20))){
                    Fail(785,"original_npc_ui_factory_created_unexpected_ui_services");result=nullptr;
                }
            }
        }
    } __finally { Write(mode, 0x490, priorClass); }
    return result;
}
inline bool CreatePlayerImpl(size_t index) {
    if (!Thread() || index >= MaxPlayers) return false;
    if (!options.createPlayers) return Fail(730, "original_player_creation_disabled");
    auto& entry = players[index];
    if (!entry.configured || !entry.enabled) return Fail(731, "original_roster_entry_not_enabled");
    if (entry.ready) return true;
    if (entry.attempted) return Fail(732, "original_entry_partial_no_automatic_retry");
    if (!StateReady() || !WorldBegun()) return Fail(733, "original_game_state_not_ready");
    // Observed stage5 failure7196d98: PS telemetry setup dereferences AresGI's
    // WorldContext, then the ClientGI +2f8 virtual service getter without null checks.
    // The caller supplies an actual local GI/service graph; this module never
    // launches online initialization or fabricates these objects/virtual tables.
    if (!diagnostics.originalInstance || !Value<void*>(diagnostics.instance, 0x38))
        return Fail(761, "original_ares_game_instance_world_context_required");
    if (!Value<void*>(Value<void*>(diagnostics.instance, 0x38), 0x2c8))
        return Fail(764, "original_world_context_client_link_required");
    if (!IsA(diagnostics.clientInstance, 0x6429080))
        return Fail(762, "original_client_game_instance_required_for_player_state");
    CharacterDefaults selected{};
    stage = "original_validate_pawn_defaults";
    // Reload by path at each creation. Cached diagnostic pointers are not GC roots.
    if (!ProbeDefaults(entry.classPath, selected) || selected.abstractClass)
        return Fail(734, "original_pawn_class_unavailable_or_abstract");
    if (!selected.inventory || !selected.abilitiesAndBuffs || !selected.mesh1P.object || !selected.inputState)
        return Fail(735, "original_character_default_components_missing");
    stage = "original_resolve_training_player_state";
    if (!ResolvePlayerStateClass()) return Fail(736, "original_player_state_blueprint_or_team_template_missing");
    engine::Transform transform{};
    transform.translation = entry.feetPosition;
    transform.translation.z += selected.capsuleHalfHeight + 2.0;
    constexpr double radians = 3.14159265358979323846 / 180.0;
    transform.rotation[2] = std::sin(entry.yaw * radians * 0.5);
    transform.rotation[3] = std::cos(entry.yaw * radians * 0.5);
    // Unit actor scale. Preserve original Blueprint component transforms/AnimBP.
    entry.attempted = true; entry.stateText = "creating_controller";
    stage = "original_spawn_controller_and_player_state";
    entry.controller = SpawnControllerWithOriginalState(transform, selectedPlayerStateClass,entry);
    if (!IsA(entry.controller, ShooterControllerClass)) return error?false:Fail(737, "original_controller_creation_failed");
    entry.state = Value<void*>(entry.controller, 0x470);
    if (!IsA(entry.state, ShooterStateClass) || Value<void*>(entry.state, 0x210) != entry.controller)
        return Fail(738, "original_controller_state_pairing_failed");
    if (!IsA(Value<void*>(entry.state, 0x6a8), 0x65a0b40))
        return Fail(760, "original_player_state_runtime_team_component_missing");
    entry.registered = RegisterState(entry.state);
    if (!entry.registered) return Fail(739, "original_player_state_registration_failed");
    entry.stateText = "creating_character"; stage = "original_begin_character";
    entry.character = Begin(selected.type, transform, entry.controller);
    if (!IsA(entry.character, ShooterCharacterClass)) return Fail(740, "original_character_creation_failed");
    // Local replacement for the absent authority factory's pre-BeginPlay PS association.
    // This is an instance pointer only, never a guessed team/GUID/init flag.
    if (!Write(entry.character, 0x488, entry.state)) return Fail(741, "original_pawn_state_link_failed");
    stage = "original_finish_character"; Finish(entry.character, transform);
    entry.inventory = Value<void*>(entry.character, 0xc08);
    entry.mesh1P = Value<void*>(entry.character, 0xf30);
    entry.abilities = Value<void*>(entry.character, 0xa20);
    entry.attributes = Value<void*>(entry.abilities, 0x13a8);
    entry.movement = Value<void*>(entry.character, 0x4f8);
    entry.characterInitialized = Value<uint8_t>(entry.character, 0xa90) != 0;
    if (!entry.characterInitialized || !entry.inventory || !entry.mesh1P || !entry.abilities
        || !entry.attributes || !IsA(entry.movement, 0x6c7d380)
        || !Value<void*>(entry.character, 0xba0)) return Fail(742, "original_character_beginplay_or_attributes_incomplete");
    stage = "original_possess_character";
    if (!engine::Event(entry.controller, L"/Script/Engine.Controller.Possess", &entry.character)
        || Value<void*>(entry.controller, 0x4b0) != entry.character
        || Value<void*>(entry.character, 0x488) != entry.state)
        return Fail(743, "original_character_possession_not_observed");
    entry.ready = true; entry.stateText = "original_character_ready";
    error = 0; stage = "original_player_created_not_local"; return true;
}
inline bool CreatePlayer(size_t index) {
    __try { return CreatePlayerImpl(index); }
    __except (RecordException(GetExceptionInformation())) { return false; }
}
inline bool SetView(void* pc, void* target, const engine::Vec& rotation, float defaultFOV, float lockedFOV) {
    alignas(8) uint8_t view[0x18]{}; Put(view, 0, target);
    auto rot = rotation;
    if (!engine::Event(pc, L"/Script/Engine.PlayerController.SetViewTargetWithBlend", view)
        || !engine::Event(pc, L"/Script/Engine.Controller.SetControlRotation", &rot)) return false;
    void* camera = Value<void*>(pc, 0x528);
    if (!camera || !Write(camera, 0x484, defaultFOV)) return false;
    return engine::Event(pc, L"/Script/Engine.PlayerController.FOV", &lockedFOV);
}
inline bool Transfer(void* pc, void* player) {
    if (!pc || !player) return false;
    auto table = Value<void**>(pc, 0); auto fn = Value<void*>(table, 0xe50);
    if (!fn) return false;
    reinterpret_cast<void(*)(void*, void*)>(fn)(pc, player);
    return Value<void*>(pc, 0x510) == player && Value<void*>(player, 0x38) == pc;
}
inline bool RollbackLocalImpl() {
    if (!Thread() || !localPlayer || !fallbackController || !fallbackPawn) return false;
    if (Value<void*>(fallbackController, 0x4b0) != fallbackPawn)
        return Fail(750, "original_fallback_pawn_no_longer_owned");
    stage = "original_restore_local_player";
    if (!Transfer(fallbackController, localPlayer)) return Fail(751, "original_local_player_rollback_failed");
    if (!SetView(fallbackController, fallbackPawn, savedControlRotation, savedDefaultFOV, savedLockedFOV))
        return Fail(752, "original_fallback_camera_restore_failed");
    if (fallbackHiddenByBridge) engine::Hidden(fallbackPawn, false);
    if (fallbackCollisionDisabledByBridge) { uint8_t yes = 1; engine::Event(fallbackPawn, L"/Script/Engine.Actor.SetActorEnableCollision", &yes); }
    fallbackHiddenByBridge = false; fallbackCollisionDisabledByBridge = false;
    activeController = fallbackController; activePawn = fallbackPawn;
    localIndex = -1; error = 0; stage = "original_local_player_restored"; return true;
}
inline bool RollbackLocal() {
    __try { return RollbackLocalImpl(); }
    __except (RecordException(GetExceptionInformation())) { return false; }
}
inline bool ActivateLocalImpl(size_t index) {
    if (!Thread() || index >= MaxPlayers) return false;
    if (!options.transferLocalPlayer) return Fail(753, "original_local_transfer_disabled");
    auto& entry = players[index];
    if (!entry.local || !entry.ready || !entry.characterInitialized || !entry.registered)
        return Fail(754, "original_local_entry_not_ready");
    if (localIndex >= 0) return localIndex == static_cast<int>(index);
    if (!localPlayer || !IsA(localPlayer, 0x3b6e680) || Value<void*>(localPlayer, 0x38) != fallbackController
        || Value<void*>(fallbackController, 0x510) != localPlayer
        || Value<void*>(fallbackController, 0x4b0) != fallbackPawn)
        return Fail(755, "original_local_transfer_has_no_valid_fallback");
    RefreshWorldDiagnostics();
    if (!diagnostics.cameraValid) return Fail(756, "original_fallback_camera_unavailable");
    if (diagnostics.settingsManager && !diagnostics.settingsInitialized)
        return Fail(763, "original_existing_settings_manager_not_initialized");
    savedControlRotation = diagnostics.cameraRotation;
    savedDefaultFOV = diagnostics.defaultFOV; savedLockedFOV = diagnostics.lockedFOV;
    stage = "original_transfer_local_player";
    if (!Transfer(entry.controller, localPlayer)) {
        RollbackLocalImpl(); return Fail(757, "original_local_transfer_not_observed");
    }
    activeController = entry.controller; activePawn = entry.character; localIndex = static_cast<int>(index);
    // Original CDO capsule/eye/mesh defaults remain untouched. Preserve view angle
    // and calibrated projection; obtain position from the new actual camera.
    if (!SetView(activeController, activePawn, savedControlRotation, savedDefaultFOV, savedLockedFOV)) {
        RollbackLocalImpl(); return Fail(758, "original_local_camera_setup_failed");
    }
    fallbackHiddenByBridge = engine::Hidden(fallbackPawn, true);
    uint8_t no = 0;
    fallbackCollisionDisabledByBridge = engine::Event(fallbackPawn, L"/Script/Engine.Actor.SetActorEnableCollision", &no);
    if (!fallbackHiddenByBridge || !fallbackCollisionDisabledByBridge) {
        RollbackLocalImpl(); return Fail(759, "original_fallback_visibility_transfer_failed");
    }
    error = 0; stage = "original_local_player_active"; return true;
}
inline bool ActivateLocal(size_t index) {
    __try { return ActivateLocalImpl(index); }
    __except (RecordException(GetExceptionInformation())) { return false; }
}
inline engine::Vec CameraPosition() { if (Thread()) RefreshWorldDiagnostics(); return diagnostics.cameraPosition; }
inline engine::Vec CameraRotation() { if (Thread()) RefreshWorldDiagnostics(); return diagnostics.cameraRotation; }
inline bool SetEnabled(size_t index, bool enabled) {
    if (!Thread() || index >= MaxPlayers || (localIndex >= 0 && index == static_cast<size_t>(localIndex))) return false;
    auto& entry = players[index];
    if (entry.character) {
        uint8_t collision = enabled ? 1 : 0;
        if (!engine::Hidden(entry.character, !enabled)
            || !engine::Event(entry.character, L"/Script/Engine.Actor.SetActorEnableCollision", &collision)) return false;
    }
    entry.enabled = enabled;
    // Disabling is a training visibility/collision state, not fake death or removal
    // from native PlayerArray. Native registration and state are kept explicit.
    return true;
}
inline std::string Utf8(const wchar_t* value) {
    if (!value) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    if (n <= 1) return {};
    std::string s(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value, -1, &s[0], n, nullptr, nullptr); s.pop_back(); return s;
}
inline void Text(FILE* f, const char* value) {
    fputc('"', f);
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(value); p && *p; ++p) {
        if (*p == '"' || *p == '\\') fprintf(f, "\\%c", *p);
        else if (*p < 32) fprintf(f, "\\u%04x", static_cast<unsigned>(*p));
        else fputc(*p, f);
    }
    fputc('"', f);
}
inline void Number(FILE* f, double value) { if (std::isfinite(value)) fprintf(f, "%.9g", value); else fputs("null", f); }
inline void Vector(FILE* f, const engine::Vec& v) {
    fputc('[', f); Number(f, v.x); fputc(',', f); Number(f, v.y); fputc(',', f); Number(f, v.z); fputc(']', f);
}
inline void PrintComponent(FILE* f, const ComponentDefaults& c) {
    fprintf(f, "{\"pointer\":\"%p\",\"valid\":%s,\"relative_location\":", c.object, c.valid ? "true" : "false"); Vector(f, c.location);
    fputs(",\"relative_rotation\":", f); Vector(f, c.rotation); fputs(",\"relative_scale\":", f); Vector(f, c.scale); fputc('}', f);
}
inline bool WriteDiagnostics(const wchar_t* outputPath) {
    // Export the last copied snapshot after a native fault without reentering
    // the engine. Runtime work remains disabled by restartRequired.
    if (!gameThread || gameThread != GetCurrentThreadId() || !outputPath || !*outputPath) return false;
    if (!restartRequired) RefreshWorldDiagnostics();
    FILE* f = nullptr;
    if (_wfopen_s(&f, outputPath, L"wb") || !f) return Fail(705, "original_diagnostic_export_failed");
    fputs("{\"schema_version\":1,\"stage\":", f); Text(f, stage);
    fprintf(f, ",\"error\":%u,\"restart_required\":%s,\"exception_address\":\"%p\",\"exception_operation\":", error,restartRequired?"true":"false",nativeExceptionAddress); Text(f,nativeExceptionOperation);
    bool anyActor = createdGameState != nullptr;
    for (const auto& entry : players) if (entry.attempted) anyActor = true;
    fprintf(f, ",\"actor_creation_performed\":%s,\"defaults\":[", anyActor ? "true" : "false");
    for (size_t i = 0; i < DefaultCount; ++i) {
        if (i) fputc(',', f);
        const auto& d = defaults[i]; const auto path = Utf8(d.path);
        fputs("{\"path\":", f); Text(f, path.c_str());
        fprintf(f, ",\"class\":\"%p\",\"cdo\":\"%p\",\"flags\":%u,\"abstract\":%s,\"valid\":%s", d.type, d.cdo, d.classFlags, d.abstractClass ? "true" : "false", d.valid ? "true" : "false");
        const char* names[]{"capsule_half_height","capsule_radius","base_eye_height","character_scale","added_eye_height","standing_eye_offset","crouching_eye_offset","unit_scale_standing_view_offset_from_root"};
        const double values[]{d.capsuleHalfHeight,d.capsuleRadius,d.baseEyeHeight,d.characterScale,d.addedEyeHeight,d.standingEyeOffset,d.crouchingEyeOffset,d.unitScaleStandingViewOffset};
        for (size_t j = 0; j < 8; ++j) { fputc(',', f); Text(f, names[j]); fputc(':', f); Number(f, values[j]); }
        const char* componentNames[]{"root","capsule","character_mesh","mesh1p","mesh_overlay1p","mesh_cosmetic3p"};
        const ComponentDefaults* components[]{&d.root,&d.capsule,&d.mesh,&d.mesh1P,&d.meshOverlay1P,&d.meshCosmetic3P};
        for (size_t j = 0; j < 6; ++j) { fputc(',', f); Text(f, componentNames[j]); fputc(':', f); PrintComponent(f, *components[j]); }
        fprintf(f, ",\"inventory\":\"%p\",\"abilities\":\"%p\",\"input_state\":\"%p\"}", d.inventory, d.abilitiesAndBuffs, d.inputState);
    }
    const auto& d = diagnostics;
    fprintf(f, "],\"world\":{\"mode\":\"%p\",\"game_state\":\"%p\",\"instance\":\"%p\",\"original_state\":%s,\"original_instance\":%s,\"client_instance\":\"%p\",\"content_manager\":\"%p\",\"settings_manager\":\"%p\",\"settings_initialized\":%s,\"menu_stack\":\"%p\",\"fixed_step\":\"%p\",\"target_move_rate\":%u,\"actual_move_rate\":%u,\"fixed_step_flags\":%u,\"native_player_count\":%d},\"camera\":{\"valid\":%s,\"default_fov\":", d.mode,d.state,d.instance,d.originalState?"true":"false",d.originalInstance?"true":"false",d.clientInstance,d.contentManager,d.settingsManager,d.settingsInitialized?"true":"false",d.menuStack,d.fixedStep,static_cast<unsigned>(d.targetMoveRate),static_cast<unsigned>(d.actualMoveRate),static_cast<unsigned>(d.fixedStepFlags),d.nativePlayerCount,d.cameraValid?"true":"false");
    Number(f,d.defaultFOV); fputs(",\"locked_fov\":",f); Number(f,d.lockedFOV); fputs(",\"pov_fov\":",f); Number(f,d.povFOV);
    fputs(",\"position\":",f); Vector(f,d.cameraPosition); fputs(",\"rotation\":",f); Vector(f,d.cameraRotation);
    fprintf(f,"},\"selected_player_state_class\":\"%p\",\"selected_team_template\":\"%p\",\"local_index\":%d,\"roster\":[",selectedPlayerStateClass,selectedTeamTemplate,localIndex);
    bool first = true;
    for (size_t i=0;i<MaxPlayers;++i) {
        const auto& entry=players[i]; if (!entry.configured) continue;
        if (!first) fputc(',',f); first=false;
        fprintf(f,"{\"index\":%zu,\"name\":",i); const auto label=Utf8(entry.requestedName); Text(f,label.c_str());
        fputs(",\"class\":",f); const auto path=Utf8(entry.classPath); Text(f,path.c_str());
        fprintf(f,",\"enabled\":%s,\"attempted\":%s,\"ready\":%s,\"registered\":%s,\"character_initialized\":%s,\"native_name_applied\":%s,\"controller\":\"%p\",\"player_state\":\"%p\",\"character\":\"%p\",\"inventory\":\"%p\",\"mesh1p\":\"%p\",\"abilities\":\"%p\",\"attributes\":\"%p\",\"movement\":\"%p\",\"state\":",entry.enabled?"true":"false",entry.attempted?"true":"false",entry.ready?"true":"false",entry.registered?"true":"false",entry.characterInitialized?"true":"false",entry.nativeNameApplied?"true":"false",entry.controller,entry.state,entry.character,entry.inventory,entry.mesh1P,entry.abilities,entry.attributes,entry.movement);
        Text(f,entry.stateText);
        fprintf(f,",\"local\":%s,\"controller_ui_factory_suppressed\":%s,\"controller_ui_factory_before\":%u,\"controller_ui_factory_after\":%u,\"controller_ui_manager\":\"%p\",\"controller_ui_director\":\"%p\"",entry.local?"true":"false",entry.controllerUIFactorySuppressed?"true":"false",unsigned(entry.controllerUIFactoryBefore),unsigned(entry.controllerUIFactoryAfter),Value<void*>(entry.controller,0xc18),Value<void*>(entry.controller,0xc20));
        fputc('}',f);
    }
    fputs("],\"notes\":[\"CDO transforms are original serialized defaults, not sampled animation or mesh bounds.\",\"Eye formula is a unit-scale standing offset from root, not world or floor height.\",\"No HUD, login or client GameInstance bootstrap is performed.\",\"Roster labels are local; unverified native name, team, DesiredClass GUID and inventory mutation are not faked.\",\"A native exception stops this bridge and requires a fresh owned session; partial native lifecycle rollback is not claimed.\"]}\n",f);
    const bool ok = ferror(f) == 0; fclose(f); return ok;
}
}
