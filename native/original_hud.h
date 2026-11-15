#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cwchar>
#include <cmath>
#include <limits>
#include "engine_api.h"

// Original local service graph, staged for the owned pinned client. Stage 1 is
// a REAL BPAresClientGameInstance sidecar, not a replacement HUD or fake ready
// flag. No ClientGI.Init, platform login/auth, telemetry Init, or network API is
// called here. The root launcher supplies a real AresGI with base Engine GI
// lifecycle; never hot-swap an Engine.GameInstance pointer into another type.
namespace original_hud {
inline constexpr const wchar_t* ClientClassPath =
    L"/Game/Globals/BPAresClientGameInstance.BPAresClientGameInstance_C";
inline constexpr uintptr_t AresGIClass = 0x647e8a0;
inline constexpr uintptr_t ClientGIClass = 0x6429080;
inline unsigned error = 0;
inline const char* stage = "original_hud_services_not_initialized";
inline DWORD gameThread = 0;
inline void* world = nullptr;
inline void* controller = nullptr;
inline bool restartRequired = false;
inline void* exceptionAddress = nullptr;
inline const char* exceptionOperation = "none";
struct ExceptionFrame { uint64_t pc = 0, sp = 0, imageRva = ~uint64_t(0); };
struct ExceptionDiagnostics {
    uint64_t rip = 0, rsp = 0, rcx = 0, rdx = 0, r8 = 0, r9 = 0;
    uint64_t rax = 0, rbx = 0, rsi = 0, rdi = 0;
    uint32_t frameCount = 0;
    bool unwindStoppedByException = false;
    ExceptionFrame frames[96]{};
};
inline ExceptionDiagnostics nativeException{};
inline constexpr size_t MaxHUDElements = 64;
struct HUDElementObservation {
    void* object = nullptr;
    void* type = nullptr;
    void* outer = nullptr;
    char objectName[128]{};
    char className[128]{};
    char classPackage[128]{};
    bool originalCoordinatedElement = false;
    bool viewportQueried = false, inViewport = false;
    bool visibilityQueried = false, visible = false;
    uint8_t slateVisibility = 0xff;
    bool desiredSizeQueried = false;
    double desiredWidth = 0, desiredHeight = 0;
};
struct Diagnostics {
    void* gameInstance = nullptr;
    void* clientInstance = nullptr;
    void* clientClass = nullptr;
    void* telemetrySampler = nullptr;
    void* telemetryManager = nullptr;
    void* configManager = nullptr;
    void* settingsManager = nullptr;
    void* menuStack = nullptr;
    void* navigationManager = nullptr;
    bool originalGameInstance = false;
    bool clientCreated = false;
    bool clientLinked = false;
    bool originalTelemetryGetters = false;
    bool telemetryConsumersVerified = false;
    bool settingsInitialized = false;
    bool readyForHUD = false;
    bool originalHUDInstalled = false;
    bool uiServiceObjectsCreated = false;
    bool settingsInitRequested = false;
    bool settingsReady = false;
    bool navigationQueuesCreated = false;
    bool settingsMenuContainersReady = false;
    void* gameUserSettings = nullptr;
    void* assetManager = nullptr;
    void* rsoManager = nullptr;
    bool rsoConstructorImportsVerified = false;
    bool rsoLocalIdentityEmpty = false;
    uint32_t clientLoadState = 0;
    int32_t settingDataCount = 0;
    void* playerState = nullptr;
    void* character = nullptr;
    void* inventory = nullptr;
    void* hud = nullptr;
    void* coordinator = nullptr;
    void* elementCoordinator = nullptr;
    void* hudModel = nullptr;
    void* firstPersonModel = nullptr;
    void* selfModel = nullptr;
    void* firstPersonWidget = nullptr;
    void* minimapWidget = nullptr;
    void* abilityWidget = nullptr;
    void* abilityParent = nullptr;
    int32_t hudElementCount = 0;
    bool playerModelBound = false;
    bool originalRootObjectsPresent = false;
    bool inGameOnlyShellSelected = false;
    void* crosshairManager = nullptr;
    void* socialExperienceModel = nullptr;
    void* partyModel = nullptr;
    void* premierRosterController = nullptr;
    void* rosterPlayerModel = nullptr;
    bool socialLocalDependenciesBound = false;
    void* socialPlayerListLock = nullptr;
    void* socialContextMenuLock = nullptr;
    void* platformPlayerManager = nullptr;
    uint32_t knownLocalReferenceCount = 0;
    bool knownLocalReferencesBound = false;
    void* socialIntegrationManager = nullptr;
    bool socialIntegrationEnabled = false;
    void* platformLinkState = nullptr;
    void* platformLinkModalState = nullptr;
    uint8_t platformLinkStatus[3]{};
    bool crosshairInitialized = false;
    int32_t crosshairProfileCount = 0, crosshairProfileIndex = -1;
    void* optionsCoordinator = nullptr;
    void* optionsMenuEntry = nullptr;
    void* optionsWidget = nullptr;
    int32_t openedMenuCount = 0;
    bool optionsBound = false, optionsOpen = false;
    bool firstPersonViewportQueried = false, firstPersonInViewport = false;
    bool minimapViewportQueried = false, minimapInViewport = false;
    uint32_t originalOptionsInputCalls = 0;
    void* inventoryManager = nullptr;
    int32_t inventoryCharacterModelCount = 0;
    int32_t inventoryCharacterMapSlots = 0, inventoryCharacterMapFreeSlots = 0;
    void* expectedController = nullptr;
    void* expectedGameState = nullptr;
    void* modelController = nullptr;
    void* modelGameState = nullptr;
    void* originalViewTarget = nullptr;
    void* firstPersonPlayerState = nullptr;
    void* firstPersonSpawnedCharacter = nullptr;
    void* firstPersonCurrentEquippable = nullptr;
    void* selfPlayerState = nullptr;
    void* selfSpawnedCharacter = nullptr;
    void* playerStateSpawnedCharacter = nullptr;
    void* playerStatePossessedCharacter = nullptr;
    int32_t modelPlayerCount = 0, modelListenedPlayerStateCount = 0;
    bool modelControllerBound = false, modelGameStateBound = false;
    bool firstPersonPlayerStateBound = false, firstPersonCharacterBound = false;
    uint32_t originalGameStateNotificationCalls = 0;
    uint32_t originalViewTargetNotificationCalls = 0;
    uint32_t originalSpawnedCharacterNotificationCalls = 0;
    bool hudElementArrayValid = false, hudElementObservationsTruncated = false;
    uint32_t observedHUDElementCount = 0;
    HUDElementObservation elements[MaxHUDElements]{};
    bool minimapFoundInOriginalWidgetTree = false;
    bool originalWidgetTreeTraversalTruncated = false;
    uint32_t originalWidgetTreeVisitedCount = 0;
    uint32_t minimapCandidateCount = 0;
    HUDElementObservation minimapCandidates[16]{};
    const char* minimapLookupStatus = "not_attempted";
    uint32_t minimapRootLookups = 0;
    uint32_t minimapDiscoveryPasses = 0, minimapCacheHits = 0, minimapHashQueries = 0;
    bool minimapDiscoveryThrottled = false;
};
inline Diagnostics diagnostics{};
struct OriginalMinimapCache {
    void* coordinator = nullptr;
    void* requiredClass = nullptr;
    void* ownerWidget = nullptr;
    void* tree = nullptr;
    void* widget = nullptr;
    void* widgetClass = nullptr;
    int32_t objectIndex = -1;
    ULONGLONG lastAttempt = 0;
    bool attempted = false;
};
inline OriginalMinimapCache minimapCache{};
inline bool Fail(unsigned code, const char* text) { error = code; stage = text; return false; }
inline bool Readable(const void* pointer, size_t bytes) {
    uintptr_t at = reinterpret_cast<uintptr_t>(pointer);
    if (!at || !bytes || bytes > (std::numeric_limits<uintptr_t>::max)() - at) return false;
    const uintptr_t end = at + bytes;
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
template<class T> inline T Read(const void* object, size_t offset) {
    T value{};
    if (object) {
        const auto address = static_cast<const uint8_t*>(object) + offset;
        if (Readable(address, sizeof(value))) std::memcpy(&value, address, sizeof(value));
    }
    return value;
}
template<class T> inline bool Write(void* object, size_t offset, const T& value) {
    if (!object) return false;
    auto address = static_cast<uint8_t*>(object) + offset;
    if (!Readable(address, sizeof(value))) return false;
    std::memcpy(address, &value, sizeof(value)); return true;
}
inline bool Derived(void* type, void* base) {
    if (!type || !base) return false;
    const int32_t depth = Read<int32_t>(base, 0x40), actual = Read<int32_t>(type, 0x40);
    if (depth < 0 || depth > 1024 || actual < depth || actual > 1024) return false;
    void* chain = Read<void*>(type, 0x38);
    return chain && Read<void*>(chain, static_cast<size_t>(depth) * 8)
        == static_cast<uint8_t*>(base) + 0x38;
}
inline bool IsA(void* object, uintptr_t getter) {
    return object && Derived(Read<void*>(object, 0x10), engine::Class(getter));
}
inline bool Thread() {
    if (restartRequired) return Fail(error ? error : 902, "original_hud_native_exception_requires_new_session");
    return engine::image && gameThread == GetCurrentThreadId()
        ? true : Fail(901, "original_hud_wrong_game_thread");
}
inline void* LoadClass(uintptr_t requiredBase, const wchar_t* path) {
    return engine::function<void*(*)(void*, void*, const wchar_t*, const wchar_t*, uint32_t)>(0x1d0c6e0)
        (engine::Class(requiredBase), nullptr, path, nullptr, 0);
}
struct SpawnParameters { void* type; void* outer; void* result = nullptr; };
static_assert(sizeof(SpawnParameters) == 0x18);
static_assert(offsetof(SpawnParameters, result) == 0x10);
inline void* Construct(void* type, void* outer) {
    if (!type || !outer || (Read<uint32_t>(type, 0xdc) & 1)) return nullptr;
    SpawnParameters parameters{type, outer, nullptr};
    void* library = engine::DefaultObject(engine::Class(0x43a9670));
    return library && engine::Event(library, L"/Script/Engine.GameplayStatics.SpawnObject", &parameters)
        ? parameters.result : nullptr;
}
inline void RefreshServices() {
    void* client = diagnostics.clientInstance;
    if (!client) return;
    diagnostics.clientLinked = Read<void*>(diagnostics.gameInstance, 0x2b8) == client
        && Read<void*>(client, 0x28) == diagnostics.gameInstance
        && Read<void*>(client, 0x2c8) == diagnostics.gameInstance;
    diagnostics.clientLoadState = Read<uint32_t>(client, 0x804);
    diagnostics.telemetrySampler = Read<void*>(client, 0x2d8);
    diagnostics.telemetryManager = Read<void*>(client, 0x2d0);
    void* table = Read<void*>(client, 0);
    diagnostics.originalTelemetryGetters = Read<void*>(table, 0x2f8) == reinterpret_cast<void*>(engine::image + 0x6429150)
        && Read<void*>(table, 0x300) == reinterpret_cast<void*>(engine::image + 0x6429140);
    diagnostics.configManager = Read<void*>(client, 0x748);
    diagnostics.settingsManager = Read<void*>(client, 0x378);
    diagnostics.menuStack = Read<void*>(client, 0x580);
    diagnostics.navigationManager = Read<void*>(client, 0x588);
    diagnostics.crosshairManager = Read<void*>(client, 0x4e8);
    diagnostics.socialExperienceModel = Read<void*>(client, 0x420);
    diagnostics.partyModel = Read<void*>(client, 0x400);
    diagnostics.premierRosterController = Read<void*>(client, 0x6c0);
    diagnostics.rosterPlayerModel = Read<void*>(diagnostics.premierRosterController, 0xc8);
    diagnostics.socialLocalDependenciesBound = diagnostics.socialExperienceModel
        && Read<void*>(diagnostics.socialExperienceModel, 0x118) == diagnostics.partyModel
        && diagnostics.partyModel
        && Read<void*>(diagnostics.socialExperienceModel, 0x120) == diagnostics.settingsManager
        && diagnostics.settingsManager
        && Read<void*>(diagnostics.socialExperienceModel, 0x128) == Read<void*>(client, 0x7a8)
        && Read<void*>(client, 0x7a8)
        && Read<void*>(diagnostics.socialExperienceModel, 0x130) == diagnostics.configManager;
    diagnostics.socialPlayerListLock = Read<void*>(diagnostics.socialExperienceModel, 0x158);
    diagnostics.socialContextMenuLock = Read<void*>(diagnostics.socialExperienceModel, 0x160);
    diagnostics.platformPlayerManager = Read<void*>(client, 0x408);
    diagnostics.socialIntegrationManager = Read<void*>(client, 0x428);
    diagnostics.socialIntegrationEnabled = Read<uint8_t>(diagnostics.socialIntegrationManager, 0x128) != 0;
    diagnostics.platformLinkState = Read<void*>(client, 0x7d0);
    diagnostics.platformLinkModalState = Read<void*>(diagnostics.socialExperienceModel, 0x168);
    for (size_t n = 0; n < 3; ++n)
        diagnostics.platformLinkStatus[n] = Read<uint8_t>(diagnostics.platformLinkState, 0xf0 + n);
    diagnostics.rsoManager = Read<void*>(client, 0x750);
    diagnostics.rsoLocalIdentityEmpty = diagnostics.rsoManager
        && Read<int32_t>(diagnostics.rsoManager, 0x160) == 0
        && Read<int32_t>(diagnostics.rsoManager, 0x170) == 0
        && Read<int32_t>(diagnostics.rsoManager, 0x180) == 0
        && Read<int32_t>(diagnostics.rsoManager, 0x190) == 0;
    diagnostics.settingsInitialized = diagnostics.settingsManager
        && Read<uint8_t>(diagnostics.settingsManager, 0x890) != 0;
    diagnostics.settingDataCount = Read<int32_t>(diagnostics.settingsManager, 0xf8);
    // Stage 1 has no complete HUD services. Do not turn pointer existence into
    // a fake initialized/full-original-HUD result.
    diagnostics.readyForHUD = false;
}
inline bool InitializeServicesImpl(void* inWorld, void* inController) {
    if (!engine::image || !inWorld) return Fail(903, "original_hud_world_missing");
    if (!gameThread) gameThread = GetCurrentThreadId();
    if (!Thread()) return false;
    if (world && world != inWorld) return Fail(904, "original_hud_world_changed_requires_new_session");
    world = inWorld; controller = inController;
    diagnostics.gameInstance = Read<void*>(world, 0x1d8);
    diagnostics.originalGameInstance = IsA(diagnostics.gameInstance, AresGIClass);
    if (!diagnostics.originalGameInstance)
        return Fail(905, "original_hud_requires_original_game_instance_restart");
    void* context = Read<void*>(diagnostics.gameInstance, 0x38);
    if (!context || Read<void*>(context, 0x2c8) != world)
        return Fail(906, "original_hud_game_instance_world_context_mismatch");
    stage = "original_hud_loading_client_sidecar_class";
    diagnostics.clientClass = LoadClass(ClientGIClass, ClientClassPath);
    if (!diagnostics.clientClass || !Derived(diagnostics.clientClass, engine::Class(ClientGIClass)))
        return Fail(907, "original_hud_client_sidecar_class_missing");
    void* client = Read<void*>(diagnostics.gameInstance, 0x2b8);
    if (client && !IsA(client, ClientGIClass))
        return Fail(908, "original_hud_existing_client_has_wrong_type");
    if (!client) {
        stage = "original_hud_constructing_client_sidecar";
        client = Construct(diagnostics.clientClass, diagnostics.gameInstance);
        if (!client || !IsA(client, ClientGIClass))
            return Fail(909, "original_hud_client_sidecar_construction_failed");
        if (Read<void*>(client, 0x28) != diagnostics.gameInstance)
            return Fail(910, "original_hud_client_sidecar_outer_mismatch");
        // Both are real reflected ObjectProperty references. In particular the
        // owner->client reference keeps the sidecar alive; Outer alone would not.
        if (!Write(client, 0x2c8, diagnostics.gameInstance)
            || !Write(diagnostics.gameInstance, 0x2b8, client))
            return Fail(911, "original_hud_client_sidecar_link_failed");
        diagnostics.clientCreated = true;
    }
    diagnostics.clientInstance = client;
    RefreshServices();
    if (!diagnostics.clientLinked) return Fail(912, "original_hud_client_sidecar_link_not_observed");
    if (!diagnostics.originalTelemetryGetters)
        return Fail(913, "original_hud_unexpected_client_telemetry_virtuals");
    // No call to the sidecar's online initialization state machine. Its original
    // constructor leaves load state 5; no fake ready/initialized flags are set.
    error = 0; stage = "original_hud_client_sidecar_ready_telemetry_null_consumers_unverified";
    return true;
}
inline bool UnwindOne(CONTEXT& context, bool& leaf) {
    __try {
        DWORD64 imageBase = 0;
        PRUNTIME_FUNCTION function = RtlLookupFunctionEntry(context.Rip, &imageBase, nullptr);
        if (!function) {
            leaf = true;
            if (!Readable(reinterpret_cast<void*>(context.Rsp), sizeof(DWORD64))) return false;
            context.Rip = *reinterpret_cast<const DWORD64*>(context.Rsp);
            context.Rsp += sizeof(DWORD64);
        } else {
            leaf = false;
            PVOID handlerData = nullptr;
            DWORD64 establisher = 0;
            RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, context.Rip, function,
                &context, &handlerData, &establisher, nullptr);
        }
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        nativeException.unwindStoppedByException = true;
        return false;
    }
}
inline void CaptureExceptionContext(const CONTEXT* original) {
    nativeException = {};
    if (!original) return;
    nativeException.rip = original->Rip; nativeException.rsp = original->Rsp;
    nativeException.rcx = original->Rcx; nativeException.rdx = original->Rdx;
    nativeException.r8 = original->R8; nativeException.r9 = original->R9;
    nativeException.rax = original->Rax; nativeException.rbx = original->Rbx;
    nativeException.rsi = original->Rsi; nativeException.rdi = original->Rdi;
    CONTEXT cursor = *original;
    const uint32_t peOffset = Read<uint32_t>(reinterpret_cast<void*>(engine::image), 0x3c);
    const uint32_t imageSize = peOffset < 0x1000
        ? Read<uint32_t>(reinterpret_cast<void*>(engine::image), peOffset + 0x50) : 0;
    while (cursor.Rip && nativeException.frameCount < 96) {
        auto& frame = nativeException.frames[nativeException.frameCount++];
        frame.pc = cursor.Rip; frame.sp = cursor.Rsp;
        if (cursor.Rip >= engine::image && cursor.Rip - engine::image < imageSize)
            frame.imageRva = cursor.Rip - engine::image;
        const auto oldRip = cursor.Rip, oldRsp = cursor.Rsp;
        bool leaf = false;
        if (!UnwindOne(cursor, leaf) || cursor.Rsp < oldRsp
            || (cursor.Rip == oldRip && cursor.Rsp == oldRsp)) break;
    }
}
inline LONG RecordException(EXCEPTION_POINTERS* exception) {
    error = exception && exception->ExceptionRecord ? exception->ExceptionRecord->ExceptionCode : 0xe0000902U;
    exceptionAddress = exception && exception->ExceptionRecord ? exception->ExceptionRecord->ExceptionAddress : nullptr;
    exceptionOperation = stage; restartRequired = true;
    CaptureExceptionContext(exception ? exception->ContextRecord : nullptr);
    stage = "original_hud_native_exception_requires_new_session";
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool InitializeServices(void* inWorld, void* inController = nullptr) {
    __try { return InitializeServicesImpl(inWorld, inController); }
    __except (RecordException(GetExceptionInformation())) { return false; }
}
inline bool BootstrapLocalServices(void* inWorld, void* inController = nullptr) {
    return InitializeServices(inWorld, inController);
}
inline bool EnsureService(size_t offset, uintptr_t getter, const wchar_t* blueprintPath = nullptr) {
    void* client = diagnostics.clientInstance;
    if (!client) return Fail(920, "original_hud_client_sidecar_required");
    void* value = Read<void*>(client, offset);
    if (value) return IsA(value, getter) ? true : Fail(921, "original_hud_service_type_mismatch");
    void* type = blueprintPath ? LoadClass(getter, blueprintPath) : engine::Class(getter);
    if (!type) return Fail(922, "original_hud_service_class_missing");
    value = Construct(type, client);
    if (!value || !IsA(value, getter)) return Fail(923, "original_hud_service_construction_failed");
    // All these offsets are ObjectProperties inside ClientGI.Systems/AllInitSystems.
    return Write(client, offset, value) || Fail(924, "original_hud_service_reference_failed");
}
inline bool InitializeUIServiceObjectsImpl() {
    if (!Thread() || !diagnostics.clientLinked) return false;
    if (diagnostics.uiServiceObjectsCreated) return true;
    stage = "original_hud_constructing_config_manager";
    if (!EnsureService(0x748, 0x66596c0)) return false;
    stage = "original_hud_verifying_empty_identity_provider_constructor";
    // RSOManager's original constructor6bd4630 only initializes local response
    // values and its trailing CRITICAL_SECTION. The encoded on-disk import slots
    // cannot prove the API names; confirm the two resolved runtime pointers before
    // constructing it. No RSO Init/Login/Auth method is invoked or patched.
    HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
    const void* initializeCriticalSection = kernel ? reinterpret_cast<const void*>(GetProcAddress(kernel, "InitializeCriticalSection")) : nullptr;
    const void* setCriticalSectionSpinCount = kernel ? reinterpret_cast<const void*>(GetProcAddress(kernel, "SetCriticalSectionSpinCount")) : nullptr;
    diagnostics.rsoConstructorImportsVerified = initializeCriticalSection && setCriticalSectionSpinCount
        && Read<void*>(reinterpret_cast<void*>(engine::image), 0xa1da3d8) == initializeCriticalSection
        && Read<void*>(reinterpret_cast<void*>(engine::image), 0xa1da3e8) == setCriticalSectionSpinCount;
    if (!diagnostics.rsoConstructorImportsVerified)
        return Fail(934, "original_hud_identity_constructor_sync_imports_not_verified");
    stage = "original_hud_constructing_empty_identity_provider";
    if (!EnsureService(0x750, 0x6bd2cb0)) return false;
    stage = "original_hud_constructing_empty_friend_settings_provider";
    // Shipped settings viewmodel7d7d440 binds this provider's native delegates
    // (+f8/+d0). Constructor6505de0 only calls UObjectCtor1d140b0 then initializes
    // local maps/delegates/flags; no external client, Init, request or socket call.
    // This supplies a real empty settings-data provider, not online friends.
    if (!EnsureService(0x7a8, 0x6504fe0)) return false;
    void* friends = Read<void*>(diagnostics.clientInstance, 0x7a8);
    // Native ObjectProperty descriptors b0db040/b0d9270: the original settings
    // viewmodel reads friends.ConfigManager+1a0, even with an empty friend list.
    if (!Write(friends, 0x198, diagnostics.gameInstance)
        || !Write(friends, 0x1a0, Read<void*>(diagnostics.clientInstance, 0x748)))
        return Fail(935, "original_hud_friend_settings_dependencies_failed");
    stage = "original_hud_constructing_empty_social_state_provider";
    // HUD's original local viewmodel7b5a490 binds the native delegate+228 on
    // ClientGI.SocialExperienceModel+420 even when the PCShell is disabled.
    // Constructor652f300 calls only UObjectCtor1d140b0 and initializes local
    // arrays/maps/delegates. Construct its real empty object; never call its
    // online InitSystem or fabricate social participants/initialized state.
    if (!EnsureService(0x420, 0x652d6d0)) return false;
    // Original ObjectProperties b0ee430/b0f12b0. Native social viewmodel7693e90
    // reads ConfigManager+130 even with an empty social state; bind actual
    // local providers without starting the model's online initialization.
    if (!Write(Read<void*>(diagnostics.clientInstance, 0x420), 0x128, friends)
        || !Write(Read<void*>(diagnostics.clientInstance, 0x420), 0x130,
            Read<void*>(diagnostics.clientInstance, 0x748)))
        return Fail(984, "original_hud_social_local_provider_references_failed");
    const size_t originalSocialLockProperties[] = {0x158, 0x160};
    for (const size_t lockProperty : originalSocialLockProperties) {
        void* social = Read<void*>(diagnostics.clientInstance, 0x420);
        void* lock = Read<void*>(social, lockProperty);
        if (!lock) {
            // Both original ObjectProperties b0eef60/b0f35c0 are real
            // AresSocialUpdateLockManager references. Original ctor6542d40
            // initializes only UObject, local delegates and zero lock count.
            lock = Construct(engine::Class(0x6538220), social);
            if (!IsA(lock, 0x6538220) || !Write(social, lockProperty, lock))
                return Fail(987, "original_hud_original_social_update_lock_creation_failed");
        } else if (!IsA(lock, 0x6538220)) {
            return Fail(988, "original_hud_original_social_update_lock_type_mismatch");
        }
    }
    stage = "original_hud_constructing_empty_party_state_provider";
    // OutboundFriendRequestViewModel7d643d0 binds the original PartyModel+30
    // delegate through ClientGI+400. This is the local model, distinct from the
    // online PartyManager+770. Constructor74b32d0 calls only UObjectCtor1d13f70
    // and its own native local delegate binder7538d50. No party is fabricated.
    if (!EnsureService(0x400, 0x6a11030)) return false;
    if (!Write(Read<void*>(diagnostics.clientInstance, 0x420), 0x118,
        Read<void*>(diagnostics.clientInstance, 0x400)))
        return Fail(985, "original_hud_social_party_reference_failed");
    stage = "original_hud_constructing_empty_platform_player_state_provider";
    // Original SocialPanelPlayerListViewModel7d6da10 binds this provider's+80
    // delegate even with an empty list. Constructor6a59080 only initializes
    // UObject, local containers/delegates and native FName keys; no platform
    // initialization, identity lookup, login or remote player data is requested.
    if (!EnsureService(0x408, 0x6a56d90)) return false;
    stage = "original_hud_constructing_empty_social_integration_state_provider";
    // The original player-list refresh7d6e2b0 reads this provider's+128 even
    // when there are no external social entries. Ctor6ceecf0 calls only UObject
    // and initializes local state/containers. Its original integration-enable
    // value remains untouched; never call external integration/online Init.
    if (!EnsureService(0x428, 0x6cec9b0)) return false;
    stage = "original_hud_constructing_empty_platform_link_ui_state";
    // Original WPlatformLinkViewModel7d66160 binds the status delegate. Ctors
    //6a68ce0/21b4e00 call only UObject and initialize local cached state. Its
    // original refresh7d66780 reads f0/f1/f2 only; do not create link services,
    // API clients, account data or invoke any linking/online Init operation.
    if (!EnsureService(0x7d0, 0x6a5b7f0)) return false;
    void* socialLinkOwner = Read<void*>(diagnostics.clientInstance, 0x420);
    void* linkModal = Read<void*>(socialLinkOwner, 0x168);
    if (!linkModal) {
        linkModal = Construct(engine::Class(0x64e3080), socialLinkOwner);
        if (!IsA(linkModal, 0x64e3080) || !Write(socialLinkOwner, 0x168, linkModal))
            return Fail(995, "original_hud_platform_link_modal_state_construction_failed");
    } else if (!IsA(linkModal, 0x64e3080)) {
        return Fail(996, "original_hud_platform_link_modal_state_type_mismatch");
    }
    stage = "original_hud_constructing_original_local_roster_view_controller";
    if (!Read<void*>(diagnostics.clientInstance, 0x6c0)) {
        void* localClient = diagnostics.clientInstance;
        // Exact eight-argument callsite7031334..7031380. Original factory77747c0
        // selects its compiled roster implementation through ConfigManager;
        // its default(false) branch constructs PremierRosterViewControllerOld
        // AND its real RosterPlayerModel through7798a10. It records dependency
        // pointers locally; it never invokes the controller/manager InitSystem.
        // Preserve actual absent online dependencies as null, not fabricated
        // accounts, rosters, party managers or initialized flags.
        void* roster = engine::function<void*(*)(void*, void*, void*, void*, void*, void*, void*, void*)>(0x77747c0)
            (Read<void*>(localClient, 0x748), Read<void*>(localClient, 0x6b0),
             Read<void*>(localClient, 0x6b8), Read<void*>(localClient, 0x680),
             Read<void*>(localClient, 0x408), Read<void*>(localClient, 0x770),
             Read<void*>(localClient, 0x750), Read<void*>(localClient, 0x600));
        if (!IsA(roster, 0x6af2260) || !Write(localClient, 0x6c0, roster))
            return Fail(938, "original_hud_original_roster_controller_factory_failed");
    }
    if (!IsA(Read<void*>(diagnostics.clientInstance, 0x6c0), 0x6af2260)
        || !IsA(Read<void*>(Read<void*>(diagnostics.clientInstance, 0x6c0), 0xc8), 0x6bcc820))
        return Fail(939, "original_hud_original_roster_player_model_not_created");
    stage = "original_hud_constructing_client_events";
    if (!EnsureService(0x550, 0x6428ed0)) return false;
    stage = "original_hud_constructing_map_load_model";
    if (!EnsureService(0x578, 0x6944ab0)) return false;
    void* client = diagnostics.clientInstance;
    void* config = Read<void*>(client, 0x748);
    void* events = Read<void*>(client, 0x550);
    void* mapLoad = Read<void*>(client, 0x578);
    if (!Write(mapLoad, 0x70, config)) return Fail(925, "original_hud_map_config_reference_failed");
    // ConfigManager's original empty configuration maps return caller defaults.
    // Its initialized flag is not forged. MapLoadModel's init flag stays original.
    stage = "original_hud_constructing_original_settings_manager";
    if (!EnsureService(0x378, 0x6510860, L"/Game/Globals/BPAresSettingsManager.BPAresSettingsManager_C")) return false;
    if (!Write(Read<void*>(client, 0x420), 0x120, Read<void*>(client, 0x378)))
        return Fail(986, "original_hud_social_settings_reference_failed");
    // AresRGIFriendsClient.AresSettingsManager ObjectProperty b0dae60.
    if (!Write(friends, 0x268, Read<void*>(client, 0x378)))
        return Fail(936, "original_hud_friend_settings_manager_reference_failed");
    stage = "original_hud_constructing_original_crosshair_manager";
    if (!EnsureService(0x4e8, 0x66e2130)) return false;
    if (!Write(Read<void*>(client, 0x4e8), 0xa0, Read<void*>(client, 0x378)))
        return Fail(937, "original_hud_crosshair_settings_reference_failed");
    stage = "original_hud_constructing_menu_stack";
    if (!Read<void*>(client, 0x580)) {
        void* menu = engine::function<void*(*)(void*, void*)>(0x7ab5580)(events, mapLoad);
        if (!menu || !IsA(menu, 0x699c660) || !Write(client, 0x580, menu))
            return Fail(926, "original_hud_menu_stack_factory_failed");
    }
    if (!IsA(Read<void*>(client, 0x580), 0x699c660)) return Fail(927, "original_hud_menu_stack_type_mismatch");
    stage = "original_hud_constructing_navigation";
    if (!Read<void*>(client, 0x588)) {
        void* navigation = engine::function<void*(*)(void*, void*, void*, void*, void*)>(0x7ac2c50)
            (client, events, mapLoad, config, nullptr);
        if (!navigation || !IsA(navigation, 0x6db70a0) || !Write(client, 0x588, navigation))
            return Fail(928, "original_hud_navigation_factory_failed");
        // This original local method registers UI tags and creates three modal
        // queues. It does not initialize telemetry, login, or account services.
        engine::function<void(*)(void*)>(0x7ac32f0)(navigation);
    }
    RefreshServices();
    void* navigation = diagnostics.navigationManager;
    if (!IsA(navigation, 0x6db70a0)) return Fail(929, "original_hud_navigation_type_mismatch");
    diagnostics.navigationQueuesCreated = Read<void*>(navigation, 0x50)
        && Read<void*>(navigation, 0x58) && Read<void*>(navigation, 0x60);
    diagnostics.uiServiceObjectsCreated = true;
    error = 0; stage = "original_hud_ui_objects_created_settings_not_initialized";
    return true;
}
inline bool InitializeUIServiceObjects() {
    __try { return InitializeUIServiceObjectsImpl(); }
    __except (RecordException(GetExceptionInformation())) { return false; }
}
struct BorrowedString { wchar_t* data = nullptr; int32_t count = 0, capacity = 0; };
struct ConfigOverrideParameters { uint8_t name[12]{}; uint32_t padding = 0; BorrowedString value{}; };
struct ConfigBoolParameters { uint8_t name[12]{}; uint8_t defaultValue = 1, result = 1; uint8_t padding[2]{}; };
static_assert(sizeof(ConfigOverrideParameters) == 0x20);
static_assert(offsetof(ConfigOverrideParameters, value) == 0x10);
static_assert(sizeof(ConfigBoolParameters) == 0x10);
inline bool ConfigureInGameOnlyShellImpl() {
    if (!Thread() || !diagnostics.uiServiceObjectsCreated || !IsA(diagnostics.configManager, 0x66596c0))
        return Fail(960, "original_hud_config_manager_required");
    if (diagnostics.originalHUDInstalled) return Fail(961, "original_hud_select_shell_before_creation");
    stage = "original_hud_selecting_shipping_ingame_shell_branch";
    ConfigOverrideParameters set{};
    engine::function<void*(*)(void*, const wchar_t*, int)>(0x1a7b720)
        (set.name, L"ui.ingame.pcshell.enabled", 1);
    wchar_t value[] = L"false";
    // ProcessEvent's native exec copies this input FString into its own temporary,
    // copies that into the original override map, then destroys its own temporary.
    set.value = {value, 6, 6};
    if (!engine::Event(diagnostics.configManager, L"/Script/ShooterGame.ConfigManager.CheatAddConfiguration", &set))
        return Fail(962, "original_hud_original_config_override_api_missing");
    ConfigBoolParameters get{};
    std::memcpy(get.name, set.name, sizeof(get.name));
    if (!engine::Event(diagnostics.configManager, L"/Script/ShooterGame.ConfigManager.GetConfigurationBool", &get))
        return Fail(963, "original_hud_original_config_readback_missing");
    if (get.result) return Fail(964, "original_hud_original_shell_selection_not_observed");
    // This is an existing shipped branch: skip PCShell/party chrome, retain the
    // original subsequent HUDElement creation loop; OptionsCoordinator creates
    // its original InGameMenu via7a87c70. It does not hide/repaint key HUD widgets.
    diagnostics.inGameOnlyShellSelected = true;
    error = 0; stage = "original_hud_shipping_ingame_shell_branch_selected";
    return true;
}
inline bool ConfigureInGameOnlyShell() {
    __try { return ConfigureInGameOnlyShellImpl(); }
    __except (RecordException(GetExceptionInformation())) { return false; }
}
struct NativeMulticast {
    void* entries = nullptr;
    int32_t count = 0, capacity = 0;
    int32_t compactThreshold = 2, invocationLockCount = 0;
};
static_assert(sizeof(NativeMulticast) == 0x18);
inline bool PollSettings() {
    if (!Thread() || !diagnostics.clientLinked) return false;
    RefreshServices();
    const int32_t count = diagnostics.settingDataCount;
    const int32_t capacity = Read<int32_t>(diagnostics.settingsManager, 0xfc);
    diagnostics.settingsReady = diagnostics.settingsInitialized && count > 0 && count <= 4096
        && capacity >= count && Read<void*>(diagnostics.settingsManager, 0xf0) != nullptr;
    if (diagnostics.settingsReady) { error = 0; stage = "original_hud_original_settings_initialized"; }
    return diagnostics.settingsReady;
}
inline bool BeginSettingsInitializationImpl() {
    if (!Thread() || !diagnostics.uiServiceObjectsCreated)
        return Fail(930, "original_hud_ui_service_objects_required");
    if (diagnostics.settingsInitRequested) return true; // asynchronous: poll separately
    void* gus = nullptr;
    void* library = engine::DefaultObject(engine::Class(0x4440490));
    if (!library || !engine::Event(library, L"/Script/Engine.GameUserSettings.GetGameUserSettings", &gus)
        || !IsA(gus, 0x6cac940)) return Fail(931, "original_hud_original_shooter_game_user_settings_required");
    diagnostics.gameUserSettings = gus;
    // AresAssetManager::Get (6e458a0) can manufacture an unregistered fallback
    // when Engine.AssetManager is missing. Read the actual owning Engine field
    // instead; its+3d8 identity/type check is present in6e45c24..6e45c67.
    void* owningEngine = Read<void*>(diagnostics.gameInstance, 0x28);
    for (unsigned depth = 0; owningEngine && depth < 8 && !IsA(owningEngine, 0x3df7bf0); ++depth)
        owningEngine = Read<void*>(owningEngine, 0x28);
    if (!IsA(owningEngine, 0x3df7bf0)) return Fail(932, "original_hud_owning_engine_missing");
    diagnostics.assetManager = Read<void*>(owningEngine, 0x3d8);
    if (!IsA(diagnostics.assetManager, 0x63eb090))
        return Fail(933, "original_hud_registered_ares_asset_manager_required");
    // The original manager initialization registers itself in ShooterGameUserSettings.
    // It may migrate LocalSettingsVersion and flush the local GameUserSettings INI;
    // use the root's isolated training Saved/Profile directory.
    stage = "original_hud_initializing_original_settings";
    NativeMulticast success{}, failure{};
    engine::function<void(*)(void*, NativeMulticast*, NativeMulticast*)>(0x75e2b30)
        (diagnostics.settingsManager, &success, &failure);
    diagnostics.settingsInitRequested = true;
    PollSettings();
    if (!diagnostics.settingsReady) stage = "original_hud_waiting_original_setting_data";
    return true;
}
inline bool BeginSettingsInitialization() {
    __try { return BeginSettingsInitializationImpl(); }
    __except (RecordException(GetExceptionInformation())) { return false; }
}
inline bool InitializeCrosshairFromSettingsImpl() {
    if (!Thread() || !PollSettings()) return Fail(970, "original_hud_crosshair_requires_real_settings");
    if (diagnostics.crosshairInitialized) return true;
    if (!IsA(diagnostics.crosshairManager, 0x66e2130))
        return Fail(971, "original_hud_original_crosshair_manager_missing");
    void* settings = diagnostics.settingsManager;
    const int32_t index = Read<int32_t>(settings, 0x8b8);
    const int32_t count = Read<int32_t>(settings, 0x8c8);
    const int32_t capacity = Read<int32_t>(settings, 0x8cc);
    void* profiles = Read<void*>(settings, 0x8c0);
    diagnostics.crosshairProfileIndex = index; diagnostics.crosshairProfileCount = count;
    if (index < 0 || index >= count || count > 1024 || capacity < count || !profiles
        || !Readable(static_cast<uint8_t*>(profiles) + static_cast<size_t>(index) * 0x180, 0x180))
        return Fail(972, "original_hud_original_crosshair_profiles_not_ready");
    if (Read<void*>(diagnostics.crosshairManager, 0xa0) != settings)
        return Fail(973, "original_hud_crosshair_manager_settings_link_mismatch");
    stage = "original_hud_initializing_original_crosshair_from_real_profile";
    NativeMulticast success{}, failure{};
    // Original InitSystem method binds local settings changes, calls7a91560 to
    // copy the selected real0x180-byte profile, broadcasts success, and consumes
    // both native delegate temporaries. No login or remote profile fetch here.
    engine::function<void(*)(void*, NativeMulticast*, NativeMulticast*)>(0x7a91610)
        (diagnostics.crosshairManager, &success, &failure);
    diagnostics.crosshairInitialized = true;
    error = 0; stage = "original_hud_original_crosshair_initialized";
    return true;
}
inline bool InitializeCrosshairFromSettings() {
    __try { return InitializeCrosshairFromSettingsImpl(); }
    __except (RecordException(GetExceptionInformation())) { return false; }
}
inline bool BindExistingLocalProviderReferencesImpl() {
    if (!Thread() || !diagnostics.uiServiceObjectsCreated || !PollSettings())
        return Fail(989, "original_hud_local_provider_binding_requires_real_settings");
    RefreshServices();
    diagnostics.knownLocalReferenceCount = 0;
    diagnostics.knownLocalReferencesBound = false;
    void* client = diagnostics.clientInstance;
    void* platform = Read<void*>(client, 0x408);
    void* social = Read<void*>(client, 0x420);
    void* friends = Read<void*>(client, 0x7a8);
    void* config = Read<void*>(client, 0x748);
    void* settings = Read<void*>(client, 0x378);
    void* rso = Read<void*>(client, 0x750);
    void* party = Read<void*>(client, 0x400);
    void* content = Read<void*>(diagnostics.gameInstance, 0x2c8);
    void* integration = Read<void*>(client, 0x428);
    void* linkState = Read<void*>(client, 0x7d0);
    void* linkModal = Read<void*>(social, 0x168);
    if (!IsA(platform, 0x6a56d90) || !IsA(social, 0x652d6d0)
        || !IsA(friends, 0x6504fe0) || !IsA(config, 0x66596c0)
        || !IsA(settings, 0x6510860) || !IsA(rso, 0x6bd2cb0)
        || !IsA(party, 0x6a11030) || !IsA(content, 0x66711f0)
        || !IsA(integration, 0x6cec9b0) || !IsA(linkState, 0x6a5b7f0)
        || !IsA(linkModal, 0x64e3080))
        return Fail(990, "original_hud_existing_local_provider_type_mismatch");
    // Generated native ObjectProperty metadata and the original factory's
    // assignments are retained in platform-player-properties.json,
    // social-state-properties.json and hud-local-provider-properties.json.
    // Bind AFTER real settings/content initialization, so late dependencies
    // cannot remain null because their earlier constructors ran first.
    struct Reference { void* owner; size_t offset; void* value; uintptr_t type; };
    const Reference references[] = {
        {platform, 0xa0, rso, 0x6bd2cb0},
        {platform, 0xb0, config, 0x66596c0},
        {platform, 0xc0, settings, 0x6510860},
        {platform, 0xc8, party, 0x6a11030},
        {platform, 0xd0, content, 0x66711f0},
        {platform, 0x108, diagnostics.gameInstance, AresGIClass},
        {platform, 0x110, friends, 0x6504fe0},
        {platform, 0x128, integration, 0x6cec9b0},
        {integration, 0xa0, config, 0x66596c0},
        {integration, 0xa8, rso, 0x6bd2cb0},
        {platform, 0x118, linkState, 0x6a5b7f0},
        {social, 0x168, linkModal, 0x64e3080},
        {linkState, 0xd0, config, 0x66596c0},
        {linkState, 0xd8, settings, 0x6510860},
        {linkModal, 0x30, platform, 0x6a56d90},
        {linkModal, 0x38, linkState, 0x6a5b7f0},
        {social, 0x118, party, 0x6a11030},
        {social, 0x120, settings, 0x6510860},
        {social, 0x128, friends, 0x6504fe0},
        {social, 0x130, config, 0x66596c0},
        {friends, 0x198, diagnostics.gameInstance, AresGIClass},
        {friends, 0x1a0, config, 0x66596c0},
        {friends, 0x268, settings, 0x6510860},
        {config, 0x1d8, rso, 0x6bd2cb0},
        {Read<void*>(client, 0x4e8), 0xa0, settings, 0x6510860},
        {Read<void*>(client, 0x578), 0x70, config, 0x66596c0},
        {Read<void*>(client, 0x588), 0x68, config, 0x66596c0},
        {social, 0x158, Read<void*>(social, 0x158), 0x6538220},
        {social, 0x160, Read<void*>(social, 0x160), 0x6538220},
    };
    stage = "original_hud_binding_existing_local_provider_references";
    for (const auto& reference : references) {
        if (!reference.owner || !IsA(reference.value, reference.type))
            return Fail(991, "original_hud_local_dependency_value_type_mismatch");
        void* previous = Read<void*>(reference.owner, reference.offset);
        if (previous && previous != reference.value)
            return Fail(992, "original_hud_local_dependency_conflicts_with_existing_provider");
        if ((!previous && !Write(reference.owner, reference.offset, reference.value))
            || Read<void*>(reference.owner, reference.offset) != reference.value)
            return Fail(993, "original_hud_local_dependency_readback_failed");
        ++diagnostics.knownLocalReferenceCount;
    }
    // A real locally supplied GameFlowStateManager can be shared if present.
    // Do not manufacture it or start its state machine merely to paint HUD.
    void* flow = Read<void*>(client, 0x3c0);
    if (flow) {
        if (!IsA(flow, 0x6881a80) || !Write(platform, 0xb8, flow) || !Write(social, 0x110, flow))
            return Fail(994, "original_hud_existing_game_flow_reference_failed");
        diagnostics.knownLocalReferenceCount += 2;
    }
    // These are reference/readback receipts only. No InitSystem, provider
    // methods, account identity, player cache entries or ready flags are set.
    diagnostics.knownLocalReferencesBound = true;
    error = 0; stage = "original_hud_existing_local_provider_references_bound";
    return true;
}
struct LookupWidgetParameters { void* type = nullptr; void* result = nullptr; };
static_assert(sizeof(LookupWidgetParameters) == 0x10);
inline void* FindCoordinatedWidget(const wchar_t* classPath) {
    if (!diagnostics.coordinator) return nullptr;
    LookupWidgetParameters parameters{LoadClass(0x63c3d90, classPath), nullptr};
    if (!parameters.type || !engine::Event(diagnostics.coordinator,
        L"/Script/ShooterGame.ShooterUICoordinator.GetCoordinatedHudElementByClass", &parameters)) return nullptr;
    return parameters.result;
}
inline bool QueryWidgetInViewport(void* widget, bool& result) {
    result = false;
    if (!widget || !IsA(widget, 0x3b0f3c0)) return false;
    // The pinned client's generated descriptor d37c140 resolves outer3bc3510
    // (Widget). Looking up UserWidget.IsInViewport cannot resolve this method.
    void* function = engine::Find(L"/Script/UMG.Widget.IsInViewport");
    if (!function || Read<uint16_t>(function, 0xbe) != 1) return false;
    uint8_t inViewport = 0;
    if (!engine::Event(widget, L"/Script/UMG.Widget.IsInViewport", &inViewport)) return false;
    result = inViewport != 0;
    return true;
}
inline void OriginalObjectName(void* object, char (&out)[128]) {
    out[0] = 0;
    if (!object || !Readable(static_cast<uint8_t*>(object) + 0x18, 12)) return;
    struct NativeString { wchar_t* data = nullptr; int32_t count = 0, capacity = 0; } text;
    static_assert(sizeof(NativeString) == 16);
    // The pinned native FName formatter/destructor are also used by the
    // original weapon diagnostics. Names are cached on the game thread only.
    engine::function<void*(*)(const void*, void*)>(0x1a7bf60)
        (static_cast<uint8_t*>(object) + 0x18, &text);
    if (text.count > 0 && text.count <= 1024 && text.capacity >= text.count
        && Readable(text.data, static_cast<size_t>(text.count) * sizeof(wchar_t))) {
        const int size = WideCharToMultiByte(CP_UTF8, 0, text.data, text.count - 1,
            out, 127, nullptr, nullptr);
        if (size > 0) out[size] = 0;
    }
    engine::function<void(*)(void*)>(0x18a2f10)(&text);
}
inline void ObserveHUDElements() {
    diagnostics.hudElementArrayValid = false;
    diagnostics.hudElementObservationsTruncated = false;
    diagnostics.observedHUDElementCount = 0;
    void* elements = Read<void*>(diagnostics.elementCoordinator, 0x58);
    const int32_t count = Read<int32_t>(diagnostics.elementCoordinator, 0x60);
    const int32_t capacity = Read<int32_t>(diagnostics.elementCoordinator, 0x64);
    if (count < 0 || count > 256 || capacity < count
        || (count && !Readable(elements, static_cast<size_t>(count) * sizeof(void*)))) return;
    diagnostics.hudElementArrayValid = true;
    const uint32_t observed = static_cast<uint32_t>(count) < MaxHUDElements
        ? static_cast<uint32_t>(count) : static_cast<uint32_t>(MaxHUDElements);
    diagnostics.hudElementObservationsTruncated = observed != static_cast<uint32_t>(count);
    for (uint32_t n = 0; n < observed; ++n) {
        auto& entry = diagnostics.elements[n];
        void* object = Read<void*>(elements, static_cast<size_t>(n) * sizeof(void*));
        void* type = Read<void*>(object, 0x10);
        if (entry.object != object || entry.type != type) {
            entry = {};
            entry.object = object; entry.type = type;
            entry.outer = Read<void*>(object, 0x28);
            entry.originalCoordinatedElement = IsA(object, 0x63c3d90);
            if (entry.originalCoordinatedElement) {
                OriginalObjectName(object, entry.objectName);
                OriginalObjectName(type, entry.className);
                OriginalObjectName(Read<void*>(type, 0x28), entry.classPackage);
            }
        }
        if (entry.originalCoordinatedElement) {
            entry.viewportQueried = QueryWidgetInViewport(object, entry.inViewport);
            uint8_t visibility = 0xff, visible = 0;
            void* visibilityFunction = engine::Find(L"/Script/UMG.Widget.GetVisibility");
            void* visibleFunction = engine::Find(L"/Script/UMG.Widget.IsVisible");
            entry.visibilityQueried = visibilityFunction && visibleFunction
                && Read<uint16_t>(visibilityFunction, 0xbe) == 1
                && Read<uint16_t>(visibleFunction, 0xbe) == 1
                && engine::Event(object, L"/Script/UMG.Widget.GetVisibility", &visibility)
                && engine::Event(object, L"/Script/UMG.Widget.IsVisible", &visible);
            entry.slateVisibility = entry.visibilityQueried ? visibility : 0xff;
            entry.visible = entry.visibilityQueried && visible != 0;
            struct Size { double x = 0, y = 0; } size;
            static_assert(sizeof(Size) == 16);
            void* sizeFunction = engine::Find(L"/Script/UMG.Widget.GetDesiredSize");
            entry.desiredSizeQueried = sizeFunction && Read<uint16_t>(sizeFunction, 0xbe) == sizeof(Size)
                && engine::Event(object, L"/Script/UMG.Widget.GetDesiredSize", &size)
                && std::isfinite(size.x) && std::isfinite(size.y);
            entry.desiredWidth = entry.desiredSizeQueried ? size.x : 0;
            entry.desiredHeight = entry.desiredSizeQueried ? size.y : 0;
        }
        ++diagnostics.observedHUDElementCount;
    }
}
inline void* FindMinimapByOriginalOuter(void* requiredClass, void* tree) {
    if (!requiredClass || !IsA(tree, 0x3b6ebc0)) return nullptr;
    ++diagnostics.minimapHashQueries;
    // StaticFindObject is the same original lookup used by engine::Find, here
    // with an exact existing WidgetTree Outer and an observed instance name.
    // It does not construct a widget, walk children, or execute a Blueprint.
    void* object = engine::function<void*(*)(void*, void*, const wchar_t*, bool)>(0x1d07dd0)
        (requiredClass, tree, L"MinimapHud", false);
    return object && Read<void*>(object, 0x28) == tree
        && Derived(Read<void*>(object, 0x10), requiredClass) ? object : nullptr;
}
inline void ObserveExactMinimap(void* object) {
    auto& entry = diagnostics.minimapCandidates[0];
    void* type = Read<void*>(object, 0x10);
    if (entry.object != object || entry.type != type) {
        entry = {};
        entry.object = object; entry.type = type; entry.outer = Read<void*>(object, 0x28);
        OriginalObjectName(object, entry.objectName);
        OriginalObjectName(type, entry.className);
        OriginalObjectName(Read<void*>(type, 0x28), entry.classPackage);
        entry.originalCoordinatedElement = IsA(object, 0x63c3d90);
    }
    entry.viewportQueried = QueryWidgetInViewport(object, entry.inViewport);
    uint8_t visibility = 0xff, visible = 0;
    entry.visibilityQueried = engine::Event(object, L"/Script/UMG.Widget.GetVisibility", &visibility)
        && engine::Event(object, L"/Script/UMG.Widget.IsVisible", &visible);
    entry.slateVisibility = entry.visibilityQueried ? visibility : 0xff;
    entry.visible = entry.visibilityQueried && visible != 0;
    diagnostics.minimapCandidateCount = 1;
}
inline void* FindMinimapInOriginalWidgetTrees() {
    diagnostics.minimapFoundInOriginalWidgetTree = false;
    diagnostics.originalWidgetTreeTraversalTruncated = false;
    diagnostics.originalWidgetTreeVisitedCount = 0;
    diagnostics.minimapCandidateCount = 0;
    diagnostics.minimapRootLookups = 0;
    diagnostics.minimapDiscoveryThrottled = false;
    if (!diagnostics.hudElementArrayValid) {
        diagnostics.minimapLookupStatus = "unknown_original_root_array_unavailable";
        return nullptr;
    }
    if (minimapCache.coordinator != diagnostics.elementCoordinator) {
        minimapCache = {};
        minimapCache.coordinator = diagnostics.elementCoordinator;
    }
    bool ownerStillCoordinated = false;
    for (uint32_t n = 0; n < diagnostics.observedHUDElementCount; ++n)
        if (diagnostics.elements[n].object == minimapCache.ownerWidget) {
            ownerStillCoordinated = true; break;
        }
    // One exact native hash lookup validates the cached object's identity. This
    // avoids retaining a raw object through removal/GC/address reuse merely
    // because its old allocation is still readable. No descendant enumeration.
    if (minimapCache.widget && ownerStillCoordinated
        && Read<void*>(minimapCache.ownerWidget, 0x298) == minimapCache.tree
        && FindMinimapByOriginalOuter(minimapCache.requiredClass, minimapCache.tree) == minimapCache.widget
        && Read<void*>(minimapCache.widget, 0x10) == minimapCache.widgetClass
        && Read<int32_t>(minimapCache.widget, 0xc) == minimapCache.objectIndex) {
        ++diagnostics.minimapCacheHits;
        diagnostics.minimapFoundInOriginalWidgetTree = true;
        diagnostics.minimapLookupStatus = "cached_exact_original_instance";
        ObserveExactMinimap(minimapCache.widget);
        return minimapCache.widget;
    }
    minimapCache.widget = nullptr;
    const ULONGLONG now = GetTickCount64();
    if (minimapCache.attempted && now - minimapCache.lastAttempt < 2000) {
        diagnostics.minimapDiscoveryThrottled = true;
        diagnostics.minimapLookupStatus = "unknown_discovery_retry_throttled";
        return nullptr;
    }
    minimapCache.attempted = true;
    minimapCache.lastAttempt = now;
    ++diagnostics.minimapDiscoveryPasses;
    if (!minimapCache.requiredClass) {
        void* loadedClass = engine::Find(
            L"/Game/UI/Screens/InGame/HUD/Minimap/AresMinimap_V2.AresMinimap_V2_C");
        if (Derived(loadedClass, engine::Class(0x63c3d90))) minimapCache.requiredClass = loadedClass;
    }
    if (!minimapCache.requiredClass) {
        diagnostics.minimapLookupStatus = "unknown_original_minimap_class_unavailable";
        return nullptr;
    }
    // Actual successful runs establish MinimapHud's Outer as the existing
    // FirstPersonHUDPC.WidgetTree. Search only the <=64 original coordinated
    // root WidgetTrees by that exact name/class. Never scan child panels or
    // format thousands of object names, and stop immediately on the first match.
    for (uint32_t n = 0; n < diagnostics.observedHUDElementCount && n < MaxHUDElements; ++n) {
        void* owner = diagnostics.elements[n].object;
        void* tree = Read<void*>(owner, 0x298);
        if (!IsA(tree, 0x3b6ebc0)) continue;
        ++diagnostics.minimapRootLookups;
        void* widget = FindMinimapByOriginalOuter(minimapCache.requiredClass, tree);
        if (!widget) continue;
        minimapCache.ownerWidget = owner; minimapCache.tree = tree; minimapCache.widget = widget;
        minimapCache.widgetClass = Read<void*>(widget, 0x10);
        minimapCache.objectIndex = Read<int32_t>(widget, 0xc);
        diagnostics.minimapFoundInOriginalWidgetTree = true;
        diagnostics.minimapLookupStatus = "found_exact_original_instance";
        ObserveExactMinimap(widget);
        return widget;
    }
    diagnostics.minimapLookupStatus = "unknown_original_instance_not_found";
    return nullptr;
}
inline void CaptureModelBindings() {
    diagnostics.inventoryManager = Read<void*>(diagnostics.clientInstance, 0x448);
    diagnostics.inventoryCharacterModelCount = Read<int32_t>(diagnostics.inventoryManager, 0x868);
    diagnostics.inventoryCharacterMapSlots = Read<int32_t>(diagnostics.inventoryManager, 0x898);
    diagnostics.inventoryCharacterMapFreeSlots = Read<int32_t>(diagnostics.inventoryManager, 0x8c4);
    diagnostics.expectedController = controller;
    diagnostics.expectedGameState = Read<void*>(world, 0x178);
    diagnostics.modelController = Read<void*>(diagnostics.hudModel, 0x250);
    diagnostics.modelGameState = Read<void*>(diagnostics.hudModel, 0x240);
    diagnostics.firstPersonPlayerState = Read<void*>(diagnostics.firstPersonModel, 0x260);
    diagnostics.firstPersonSpawnedCharacter = Read<void*>(diagnostics.firstPersonModel, 0x270);
    diagnostics.firstPersonCurrentEquippable = Read<void*>(diagnostics.firstPersonModel, 0x278);
    diagnostics.selfPlayerState = Read<void*>(diagnostics.selfModel, 0x260);
    diagnostics.selfSpawnedCharacter = Read<void*>(diagnostics.selfModel, 0x270);
    diagnostics.playerStateSpawnedCharacter = Read<void*>(diagnostics.playerState, 0xbc8);
    diagnostics.playerStatePossessedCharacter = Read<void*>(diagnostics.playerState, 0xbd0);
    diagnostics.modelPlayerCount = Read<int32_t>(diagnostics.hudModel, 0x58);
    diagnostics.modelListenedPlayerStateCount = Read<int32_t>(diagnostics.hudModel, 0x260);
    diagnostics.modelControllerBound = diagnostics.modelController == controller;
    diagnostics.modelGameStateBound = diagnostics.modelGameState == diagnostics.expectedGameState
        && IsA(diagnostics.expectedGameState, 0x6c9d280);
    diagnostics.firstPersonPlayerStateBound = IsA(diagnostics.firstPersonModel, 0x68ba540)
        && diagnostics.firstPersonPlayerState == diagnostics.playerState;
    diagnostics.firstPersonCharacterBound = diagnostics.firstPersonSpawnedCharacter == diagnostics.character;
    diagnostics.playerModelBound = diagnostics.modelControllerBound && diagnostics.modelGameStateBound
        && diagnostics.firstPersonPlayerStateBound && diagnostics.firstPersonCharacterBound;
}
inline bool ReplayMissingOriginalHUDLifecycle() {
    CaptureModelBindings();
    if (!diagnostics.modelControllerBound || !IsA(diagnostics.firstPersonModel, 0x68ba540)
        || !IsA(diagnostics.selfModel, 0x68ba540) || !IsA(diagnostics.expectedGameState, 0x6c9d280))
        return true; // Export the actual mismatch; never patch another model's fields.
    if (!diagnostics.modelGameState) {
        // Original7a7c190 registers this callback on World+858 after the local
        // GameState has already been created. The registration does not replay
        // it. Invoke the SAME type-checked original callback with that real GS;
        //7a7c8f0 owns all GS/player-list bindings and broadcasts. No ready/model
        // storage is populated by this bridge.
        stage = "original_hud_replaying_existing_world_game_state_notification";
        ++diagnostics.originalGameStateNotificationCalls;
        engine::function<void(*)(void*, void*)>(0x7a7c880)
            (diagnostics.hudModel, diagnostics.expectedGameState);
        CaptureModelBindings();
    }
    auto table = Read<void*>(controller, 0);
    auto viewTargetGetter = Read<void*>(table, 0x7f0);
    if (viewTargetGetter)
        diagnostics.originalViewTarget = reinterpret_cast<void*(*)(void*)>(viewTargetGetter)(controller);
    if (!diagnostics.firstPersonPlayerStateBound) {
        stage = "original_hud_replaying_original_view_target_notification";
        ++diagnostics.originalViewTargetNotificationCalls;
        engine::function<void(*)(void*, void*)>(0x7a7e630)
            (diagnostics.hudModel, diagnostics.originalViewTarget);
        CaptureModelBindings();
    }
    if (diagnostics.firstPersonPlayerStateBound && !diagnostics.firstPersonCharacterBound
        && diagnostics.playerStateSpawnedCharacter == diagnostics.character) {
        void* function = engine::Find(L"/Script/ShooterGame.HUDPlayerModel.OnNewSpawnedCharacter");
        if (!function || Read<uint16_t>(function, 0xbe) != sizeof(void*))
            return Fail(996, "original_hud_original_spawned_character_notification_signature_mismatch");
        stage = "original_hud_replaying_real_player_spawn_notification";
        ++diagnostics.originalSpawnedCharacterNotificationCalls;
        void* spawned = diagnostics.playerStateSpawnedCharacter;
        if (!engine::Event(diagnostics.firstPersonModel,
            L"/Script/ShooterGame.HUDPlayerModel.OnNewSpawnedCharacter", &spawned))
            return Fail(997, "original_hud_original_spawned_character_notification_missing");
        CaptureModelBindings();
    }
    return true;
}
inline void RefreshOptionsState() {
    // Reflected native ObjectProperties: ShooterUICoordinator+78,
    // OptionsScreenCoordinator+68/+70/+78/+80, MenuStackEntry.Content+30.
    // The actual MenuStackManager.OpenedMenus TArray is the open-state source.
    diagnostics.optionsCoordinator = Read<void*>(diagnostics.coordinator, 0x78);
    diagnostics.optionsBound = IsA(diagnostics.optionsCoordinator, 0x69f41a0)
        && Read<void*>(diagnostics.optionsCoordinator, 0x68) == diagnostics.coordinator
        && Read<void*>(diagnostics.optionsCoordinator, 0x70) == controller
        && Read<void*>(diagnostics.optionsCoordinator, 0x78) == diagnostics.menuStack;
    diagnostics.optionsMenuEntry = diagnostics.optionsBound
        ? Read<void*>(diagnostics.optionsCoordinator, 0x80) : nullptr;
    diagnostics.optionsWidget = IsA(diagnostics.optionsMenuEntry, 0x699c5b0)
        ? Read<void*>(diagnostics.optionsMenuEntry, 0x30) : nullptr;
    diagnostics.optionsOpen = false;
    diagnostics.openedMenuCount = Read<int32_t>(diagnostics.menuStack, 0xb8);
    const int32_t capacity = Read<int32_t>(diagnostics.menuStack, 0xbc);
    void* entries = Read<void*>(diagnostics.menuStack, 0xb0);
    const int32_t count = diagnostics.openedMenuCount;
    if (!diagnostics.optionsMenuEntry || count <= 0 || count > 128 || capacity < count
        || !Readable(entries, static_cast<size_t>(count) * sizeof(void*))) return;
    for (int32_t n = 0; n < count; ++n) {
        if (Read<void*>(entries, static_cast<size_t>(n) * sizeof(void*)) == diagnostics.optionsMenuEntry) {
            diagnostics.optionsOpen = true; break;
        }
    }
}
inline bool InspectHUDImpl() {
    if (!Thread() || !controller) return false;
    diagnostics.hud = Read<void*>(controller, 0x520);
    if (!IsA(diagnostics.hud, 0x6cae7d0)) return Fail(950, "original_hud_original_actor_not_present");
    diagnostics.coordinator = Read<void*>(diagnostics.hud, 0x760);
    if (!IsA(diagnostics.coordinator, 0x6cd6490)) return Fail(951, "original_hud_original_coordinator_not_created");
    diagnostics.elementCoordinator = Read<void*>(diagnostics.coordinator, 0x60);
    if (!IsA(diagnostics.elementCoordinator, 0x68ba840)) return Fail(952, "original_hud_original_element_coordinator_missing");
    diagnostics.hudModel = Read<void*>(diagnostics.elementCoordinator, 0x68);
    if (!IsA(diagnostics.hudModel, 0x68ba790)) return Fail(953, "original_hud_original_data_model_missing");
    diagnostics.hudElementCount = Read<int32_t>(diagnostics.elementCoordinator, 0x60);
    diagnostics.firstPersonModel = Read<void*>(diagnostics.hudModel, 0x40);
    diagnostics.selfModel = Read<void*>(diagnostics.hudModel, 0x48);
    ObserveHUDElements();
    if (!ReplayMissingOriginalHUDLifecycle()) return false;
    diagnostics.firstPersonWidget = FindCoordinatedWidget(
        L"/Game/UI/InGame/HUD/FirstPersonHUD.FirstPersonHUD_C");
    diagnostics.minimapWidget = FindMinimapInOriginalWidgetTrees();
    diagnostics.abilityParent = Read<void*>(diagnostics.hud, 0xa58);
    diagnostics.abilityWidget = Read<void*>(diagnostics.character, 0xb90);
    diagnostics.originalRootObjectsPresent = diagnostics.firstPersonWidget && diagnostics.minimapWidget
        && diagnostics.abilityParent && IsA(diagnostics.abilityWidget, 0x6621d40);
    diagnostics.firstPersonViewportQueried = QueryWidgetInViewport(
        diagnostics.firstPersonWidget, diagnostics.firstPersonInViewport);
    diagnostics.minimapViewportQueried = QueryWidgetInViewport(
        diagnostics.minimapWidget, diagnostics.minimapInViewport);
    RefreshOptionsState();
    // Object/data acceptance only. The parent still verifies pixels, original HP,
    // ammo/ability updates and actual minimap behavior in the owned viewport.
    error = 0;
    stage = diagnostics.playerModelBound && diagnostics.originalRootObjectsPresent
        ? "original_hud_native_roots_and_real_player_model_present"
        : "original_hud_installed_waiting_original_models_and_roots";
    return true;
}
inline bool InspectHUD() {
    __try { return InspectHUDImpl(); }
    __except (RecordException(GetExceptionInformation())) { return false; }
}
inline bool InstallOriginalHUDImpl(void* localController, void* nativeCharacter) {
    if (!Thread() || !diagnostics.uiServiceObjectsCreated || !PollSettings())
        return Fail(940, "original_hud_initialized_original_settings_required");
    if (!IsA(localController, 0x6943db0) || !IsA(nativeCharacter, 0x6c5c480))
        return Fail(941, "original_hud_original_local_controller_and_character_required");
    void* player = Read<void*>(localController, 0x510);
    if (!player || Read<void*>(player, 0x38) != localController
        || Read<void*>(localController, 0x4b0) != nativeCharacter)
        return Fail(942, "original_hud_local_player_possession_required");
    void* state = Read<void*>(localController, 0x470);
    if (!IsA(state, 0x6cc4040) || Read<void*>(nativeCharacter, 0x488) != state
        || Read<uint8_t>(nativeCharacter, 0xa90) == 0)
        return Fail(943, "original_hud_initialized_original_character_state_required");
    void* inventory = Read<void*>(nativeCharacter, 0xc08);
    if (!IsA(inventory, 0x6499330) || !IsA(Read<void*>(nativeCharacter, 0xa20), 0x63d6b50)
        || !IsA(Read<void*>(world, 0x178), 0x6c9d280))
        return Fail(944, "original_hud_real_inventory_abilities_and_game_state_required");
    void* worldSettings = engine::function<void*(*)(void*)>(0x3efc090)(nativeCharacter);
    if (!IsA(worldSettings, 0x65807e0)) return Fail(945, "original_hud_ares_world_settings_required");
    RefreshServices();
    if (!diagnostics.clientLinked || !IsA(diagnostics.configManager, 0x66596c0)
        || !IsA(diagnostics.menuStack, 0x699c660) || !IsA(diagnostics.navigationManager, 0x6db70a0))
        return Fail(946, "original_hud_local_ui_services_required");
    diagnostics.inventoryManager = Read<void*>(diagnostics.clientInstance, 0x448);
    // The original deferred character-UI task7a3437e unconditionally calls
    // InventoryManager's+890 character-map lookup71e6d40. Its World getter
    //71e4790 returns ClientGI+448. A missing service crashes the next engine
    // tick after an otherwise successful HUD factory; require the real service.
    if (!IsA(diagnostics.inventoryManager, 0x68e4630))
        return Fail(995, "original_hud_original_inventory_catalog_provider_required");
    if (!InitializeCrosshairFromSettingsImpl()) return false;
    if (!BindExistingLocalProviderReferencesImpl()) return false;
    controller = localController;
    diagnostics.character = nativeCharacter; diagnostics.playerState = state; diagnostics.inventory = inventory;
    diagnostics.readyForHUD = true;
    if (diagnostics.originalHUDInstalled) return InspectHUDImpl();
    stage = "original_hud_loading_original_actor_class";
    void* type = LoadClass(0x6cae7d0, L"/Game/UI/InGame/HUD/AresHUD.AresHUD_C");
    if (!type) return Fail(947, "original_hud_original_actor_class_missing");
    stage = "original_hud_client_set_original_hud";
    // The original ClientSetHUD implementation owns actor creation and BeginPlay.
    // ShooterHUD itself builds the original coordinator and complete element tree.
    // This replaces the current controller HUD; it is never auto-called in Stage1/2.
    if (!engine::Event(controller, L"/Script/Engine.PlayerController.ClientSetHUD", &type))
        return Fail(948, "original_hud_client_set_hud_reflection_missing");
    diagnostics.hud = Read<void*>(controller, 0x520);
    if (!IsA(diagnostics.hud, 0x6cae7d0) || Read<void*>(diagnostics.hud, 0x468) != controller)
        return Fail(949, "original_hud_actor_owner_binding_not_observed");
    diagnostics.originalHUDInstalled = true;
    return InspectHUDImpl();
}
inline bool InstallOriginalHUD(void* localController, void* nativeCharacter) {
    __try { return InstallOriginalHUDImpl(localController, nativeCharacter); }
    __except (RecordException(GetExceptionInformation())) { return false; }
}
inline bool ToggleOriginalOptionsImpl() {
    if (!Thread() || !diagnostics.originalHUDInstalled || !diagnostics.playerModelBound)
        return Fail(980, "original_options_requires_bound_original_hud");
    RefreshOptionsState();
    if (!diagnostics.optionsBound || !diagnostics.inGameOnlyShellSelected)
        return Fail(981, "original_options_original_ingame_coordinator_required");
    void* function = engine::Find(L"/Script/ShooterGame.OptionsScreenCoordinator.OnOpenOptionsScreenInput");
    if (!function || Read<uint16_t>(function, 0xbe) != 0)
        return Fail(982, "original_options_input_signature_mismatch");
    stage = "original_options_dispatching_original_input_handler";
    // Original body7a86a20 owns popup dismissal, open/close, input focus and
    // the native InGameMenu. This is a dispatch receipt, not proof of opening:
    // callers inspect optionsOpen and the real opened-menu list after the call.
    if (!engine::Event(diagnostics.optionsCoordinator,
        L"/Script/ShooterGame.OptionsScreenCoordinator.OnOpenOptionsScreenInput", nullptr))
        return Fail(983, "original_options_input_handler_unavailable");
    ++diagnostics.originalOptionsInputCalls;
    RefreshOptionsState();
    error = 0;
    stage = diagnostics.optionsOpen ? "original_options_original_menu_open" : "original_options_original_menu_closed";
    return true;
}
inline bool ToggleOriginalOptions() {
    __try { return ToggleOriginalOptionsImpl(); }
    __except (RecordException(GetExceptionInformation())) { return false; }
}
inline bool NotifyNativeValuesChanged(bool vitality, bool inventoryItems) {
    if (!Thread() || !diagnostics.playerModelBound) return false;
    // A data bridge only: callers first change REAL native character/inventory
    // values. Original Blueprint controls then reread those values via their own
    // existing model delegates. This does not inject replacement HUD text/icons.
    bool ok = true;
    if (vitality) ok = engine::Event(diagnostics.firstPersonModel,
        L"/Script/ShooterGame.HUDPlayerModel.RebroadcastOnVitalityDirtied", nullptr) && ok;
    if (inventoryItems) ok = engine::Event(diagnostics.firstPersonModel,
        L"/Script/ShooterGame.HUDPlayerModel.RebroadcastOnInventoryItemsChanged", nullptr) && ok;
    return ok;
}
inline const Diagnostics& State() { return diagnostics; }
inline void WriteJSONText(FILE* file, const char* text) {
    std::fputc('"', file);
    if (text) for (const unsigned char* at = reinterpret_cast<const unsigned char*>(text); *at; ++at) {
        if (*at == '"' || *at == '\\') { std::fputc('\\', file); std::fputc(*at, file); }
        else if (*at < 0x20) std::fprintf(file, "\\u%04x", static_cast<unsigned>(*at));
        else std::fputc(*at, file);
    }
    std::fputc('"', file);
}
inline unsigned long long Pointer(const void* value) {
    return static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(value));
}
inline void WriteModelAndElementDiagnostics(FILE* file) {
    const auto& d = diagnostics;
    std::fprintf(file, ",\"model_bindings\":{\"controller\":\"0x%llx\",\"game_state\":\"0x%llx\","
        "\"hud\":\"0x%llx\",\"coordinator\":\"0x%llx\",\"element_coordinator\":\"0x%llx\","
        "\"hud_model\":\"0x%llx\",\"first_person_model\":\"0x%llx\",\"self_model\":\"0x%llx\","
        "\"model_controller\":\"0x%llx\",\"model_game_state\":\"0x%llx\",\"original_view_target\":\"0x%llx\","
        "\"expected_player_state\":\"0x%llx\",\"expected_character\":\"0x%llx\","
        "\"first_person_player_state\":\"0x%llx\",\"first_person_spawned_character\":\"0x%llx\","
        "\"first_person_current_equippable\":\"0x%llx\",\"self_player_state\":\"0x%llx\",\"self_spawned_character\":\"0x%llx\","
        "\"player_state_spawned_character\":\"0x%llx\",\"player_state_possessed_character\":\"0x%llx\","
        "\"model_player_count\":%d,\"model_listened_player_state_count\":%d,"
        "\"model_controller_bound\":%s,\"model_game_state_bound\":%s,"
        "\"first_person_player_state_bound\":%s,\"first_person_character_bound\":%s,"
        "\"original_game_state_notification_calls\":%u,\"original_view_target_notification_calls\":%u,"
        "\"original_spawned_character_notification_calls\":%u,"
        "\"inventory_manager\":\"0x%llx\",\"inventory_character_model_count\":%d,"
        "\"inventory_character_map_slots\":%d,\"inventory_character_map_free_slots\":%d,"
        "\"first_person_widget\":\"0x%llx\",\"minimap_widget\":\"0x%llx\","
        "\"ability_widget\":\"0x%llx\",\"ability_parent\":\"0x%llx\"},"
        "\"hud_element_array_valid\":%s,\"hud_element_observations_truncated\":%s,\"coordinated_elements\":[",
        Pointer(d.expectedController), Pointer(d.expectedGameState), Pointer(d.hud), Pointer(d.coordinator),
        Pointer(d.elementCoordinator), Pointer(d.hudModel), Pointer(d.firstPersonModel), Pointer(d.selfModel),
        Pointer(d.modelController), Pointer(d.modelGameState), Pointer(d.originalViewTarget),
        Pointer(d.playerState), Pointer(d.character), Pointer(d.firstPersonPlayerState),
        Pointer(d.firstPersonSpawnedCharacter), Pointer(d.firstPersonCurrentEquippable),
        Pointer(d.selfPlayerState), Pointer(d.selfSpawnedCharacter), Pointer(d.playerStateSpawnedCharacter),
        Pointer(d.playerStatePossessedCharacter), d.modelPlayerCount, d.modelListenedPlayerStateCount,
        d.modelControllerBound ? "true" : "false", d.modelGameStateBound ? "true" : "false",
        d.firstPersonPlayerStateBound ? "true" : "false", d.firstPersonCharacterBound ? "true" : "false",
        d.originalGameStateNotificationCalls, d.originalViewTargetNotificationCalls,
        d.originalSpawnedCharacterNotificationCalls, Pointer(d.inventoryManager), d.inventoryCharacterModelCount,
        d.inventoryCharacterMapSlots, d.inventoryCharacterMapFreeSlots, Pointer(d.firstPersonWidget),
        Pointer(d.minimapWidget), Pointer(d.abilityWidget), Pointer(d.abilityParent),
        d.hudElementArrayValid ? "true" : "false", d.hudElementObservationsTruncated ? "true" : "false");
    for (uint32_t n = 0; n < d.observedHUDElementCount && n < MaxHUDElements; ++n) {
        const auto& e = d.elements[n];
        std::fprintf(file, "%s{\"index\":%u,\"object\":\"0x%llx\",\"class\":\"0x%llx\",\"outer\":\"0x%llx\",\"name\":",
            n ? "," : "", n, Pointer(e.object), Pointer(e.type), Pointer(e.outer));
        WriteJSONText(file, e.objectName);
        std::fprintf(file, ",\"class_name\":"); WriteJSONText(file, e.className);
        std::fprintf(file, ",\"class_package\":"); WriteJSONText(file, e.classPackage);
        std::fprintf(file, ",\"original_coordinated_element\":%s,\"viewport_queried\":%s,\"in_viewport\":%s,"
            "\"visibility_queried\":%s,\"visible\":%s,\"slate_visibility\":%u,"
            "\"desired_size_queried\":%s,\"desired_width\":%.3f,\"desired_height\":%.3f}",
            e.originalCoordinatedElement ? "true" : "false", e.viewportQueried ? "true" : "false",
            e.inViewport ? "true" : "false", e.visibilityQueried ? "true" : "false", e.visible ? "true" : "false",
            static_cast<unsigned>(e.slateVisibility), e.desiredSizeQueried ? "true" : "false",
            e.desiredWidth, e.desiredHeight);
    }
    std::fprintf(file, "],\"minimap_tree_search\":{\"found_exact_original_class\":%s,"
        "\"lookup_mode\":\"cached_original_outer_and_instance_name\",\"lookup_status\":\"%s\","
        "\"full_tree_traversal_performed\":false,\"root_lookups_this_sample\":%u,"
        "\"discovery_passes\":%u,\"cache_hits\":%u,\"native_hash_queries\":%u,\"discovery_throttled\":%s,"
        "\"traversal_truncated\":%s,\"visited_widgets\":%u,\"candidates\":[",
        d.minimapFoundInOriginalWidgetTree ? "true" : "false",
        d.minimapLookupStatus, d.minimapRootLookups, d.minimapDiscoveryPasses,
        d.minimapCacheHits, d.minimapHashQueries, d.minimapDiscoveryThrottled ? "true" : "false",
        d.originalWidgetTreeTraversalTruncated ? "true" : "false", d.originalWidgetTreeVisitedCount);
    for (uint32_t n = 0; n < d.minimapCandidateCount && n < _countof(d.minimapCandidates); ++n) {
        const auto& e = d.minimapCandidates[n];
        std::fprintf(file, "%s{\"object\":\"0x%llx\",\"class\":\"0x%llx\",\"outer\":\"0x%llx\",\"name\":",
            n ? "," : "", Pointer(e.object), Pointer(e.type), Pointer(e.outer));
        WriteJSONText(file, e.objectName);
        std::fprintf(file, ",\"class_name\":"); WriteJSONText(file, e.className);
        std::fprintf(file, ",\"class_package\":"); WriteJSONText(file, e.classPackage);
        std::fprintf(file, ",\"original_coordinated_element\":%s,\"viewport_queried\":%s,\"in_viewport\":%s,"
            "\"visibility_queried\":%s,\"visible\":%s,\"slate_visibility\":%u}",
            e.originalCoordinatedElement ? "true" : "false", e.viewportQueried ? "true" : "false",
            e.inViewport ? "true" : "false", e.visibilityQueried ? "true" : "false", e.visible ? "true" : "false",
            static_cast<unsigned>(e.slateVisibility));
    }
    std::fprintf(file, "]}}");
}
inline bool WriteDiagnostics(const wchar_t* path) {
    // Pure cached export: no engine reads, lookups, function calls, or new unwind.
    if (!path || wcslen(path) > 960) return false;
    wchar_t temporary[1024]{};
    if (swprintf_s(temporary, L"%ls.tmp", path) < 0) return false;
    FILE* file = nullptr;
    if (_wfopen_s(&file, temporary, L"wb") != 0 || !file) return false;
    std::fprintf(file, "{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u,\"restart_required\":%s,"
        "\"exception_operation\":\"%s\",\"registers\":{\"rip\":\"0x%llx\",\"rsp\":\"0x%llx\","
        "\"rcx\":\"0x%llx\",\"rdx\":\"0x%llx\",\"r8\":\"0x%llx\",\"r9\":\"0x%llx\","
        "\"rax\":\"0x%llx\",\"rbx\":\"0x%llx\",\"rsi\":\"0x%llx\",\"rdi\":\"0x%llx\"},"
        "\"unwind_stopped_by_exception\":%s,\"frames\":[", stage, error, restartRequired ? "true" : "false",
        exceptionOperation, nativeException.rip, nativeException.rsp, nativeException.rcx, nativeException.rdx,
        nativeException.r8, nativeException.r9, nativeException.rax, nativeException.rbx,
        nativeException.rsi, nativeException.rdi, nativeException.unwindStoppedByException ? "true" : "false");
    for (uint32_t n = 0; n < nativeException.frameCount; ++n) {
        const auto& f = nativeException.frames[n];
        std::fprintf(file, "%s{\"pc\":\"0x%llx\",\"sp\":\"0x%llx\",\"image_rva\":", n ? "," : "", f.pc, f.sp);
        if (f.imageRva == ~uint64_t(0)) std::fprintf(file, "null}");
        else std::fprintf(file, "\"0x%llx\"}", f.imageRva);
    }
    std::fprintf(file, "],\"services\":{\"client_linked\":%s,\"settings_ready\":%s,\"setting_data_count\":%d,"
        "\"original_hud_installed\":%s,\"hud_element_count\":%d,\"player_model_bound\":%s,"
        "\"original_roots_present\":%s,\"crosshair_manager\":\"0x%llx\",\"crosshair_initialized\":%s,"
        "\"social_experience_model\":\"0x%llx\",\"party_model\":\"0x%llx\","
        "\"premier_roster_controller\":\"0x%llx\",\"roster_player_model\":\"0x%llx\","
        "\"social_local_dependencies_bound\":%s,\"social_player_list_lock\":\"0x%llx\",\"social_context_menu_lock\":\"0x%llx\","
        "\"platform_player_manager\":\"0x%llx\","
        "\"known_local_references_bound\":%s,\"known_local_reference_count\":%u,"
        "\"social_integration_manager\":\"0x%llx\",\"social_integration_enabled\":%s,"
        "\"platform_link_ui_state\":\"0x%llx\",\"platform_link_modal_state\":\"0x%llx\",\"platform_link_status_bytes\":[%u,%u,%u],"
        "\"crosshair_profile_count\":%d,\"crosshair_profile_index\":%d,\"ingame_only_shell_selected\":%s,"
        "\"first_person_viewport_queried\":%s,\"first_person_in_viewport\":%s,"
        "\"minimap_viewport_queried\":%s,\"minimap_in_viewport\":%s,"
        "\"options_bound\":%s,\"options_open\":%s,\"opened_menu_count\":%d,\"original_options_input_calls\":%u,"
        "\"options_coordinator\":\"0x%llx\",\"options_menu_entry\":\"0x%llx\",\"options_widget\":\"0x%llx\"}", diagnostics.clientLinked ? "true" : "false",
        diagnostics.settingsReady ? "true" : "false", diagnostics.settingDataCount,
        diagnostics.originalHUDInstalled ? "true" : "false", diagnostics.hudElementCount,
        diagnostics.playerModelBound ? "true" : "false", diagnostics.originalRootObjectsPresent ? "true" : "false",
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(diagnostics.crosshairManager)),
        diagnostics.crosshairInitialized ? "true" : "false",
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(diagnostics.socialExperienceModel)),
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(diagnostics.partyModel)),
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(diagnostics.premierRosterController)),
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(diagnostics.rosterPlayerModel)),
        diagnostics.socialLocalDependenciesBound ? "true" : "false",
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(diagnostics.socialPlayerListLock)),
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(diagnostics.socialContextMenuLock)),
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(diagnostics.platformPlayerManager)),
        diagnostics.knownLocalReferencesBound ? "true" : "false", diagnostics.knownLocalReferenceCount,
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(diagnostics.socialIntegrationManager)),
        diagnostics.socialIntegrationEnabled ? "true" : "false",
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(diagnostics.platformLinkState)),
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(diagnostics.platformLinkModalState)),
        static_cast<unsigned>(diagnostics.platformLinkStatus[0]), static_cast<unsigned>(diagnostics.platformLinkStatus[1]),
        static_cast<unsigned>(diagnostics.platformLinkStatus[2]), diagnostics.crosshairProfileCount,
        diagnostics.crosshairProfileIndex, diagnostics.inGameOnlyShellSelected ? "true" : "false",
        diagnostics.firstPersonViewportQueried ? "true" : "false", diagnostics.firstPersonInViewport ? "true" : "false",
        diagnostics.minimapViewportQueried ? "true" : "false", diagnostics.minimapInViewport ? "true" : "false",
        diagnostics.optionsBound ? "true" : "false", diagnostics.optionsOpen ? "true" : "false",
        diagnostics.openedMenuCount, diagnostics.originalOptionsInputCalls,
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(diagnostics.optionsCoordinator)),
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(diagnostics.optionsMenuEntry)),
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(diagnostics.optionsWidget)));
    WriteModelAndElementDiagnostics(file);
    const bool written = std::ferror(file) == 0;
    const bool closed = std::fclose(file) == 0;
    return written && closed && MoveFileExW(temporary, path, MOVEFILE_REPLACE_EXISTING);
}
} // namespace original_hud
