#pragma once
#include "original_content.h"

// A local bootstrap of the two ORIGINAL, differently typed client services used
// by weapon attachments. Evidence: work/duel1-original-content-static/preload-*.
// This does not run InventoryManager's account/entitlement lifecycle or the
// PreloadManager InitSystem graph (party, platform, pregame and telemetry).
namespace original_content_services {
inline constexpr uintptr_t PreloadClass = 0x6aa5740;
inline constexpr uintptr_t InventoryClass = 0x68e4630;
inline constexpr size_t MaxObservedAttachments = 8;
struct NativeDelegate { void* storage = nullptr; int32_t size = 0, capacity = 0; };
static_assert(sizeof(NativeDelegate) == 16);
struct Name { uint32_t index = 0, comparison = 0, number = 0; };
static_assert(sizeof(Name) == 12);
using ProcessEvent = void(*)(void*, void*, void*);
struct AttachmentObservation {
    void* component = nullptr;
    void** originalTable = nullptr;
    void** observedTable = nullptr;
    ProcessEvent originalEvent = nullptr;
    uintptr_t nativeTableRva = 0;
    uint32_t nativeTableEntries = 0;
    bool requestIssued = false;
    uint32_t callbackEntered = 0, callbackCompleted = 0, lastAssetTypes = 0;
    uint32_t processEventCalls = 0, pathsEntered = 0, pathsCompleted = 0;
    int32_t lastPathCount = -1;
};
struct Diagnostics {
    void* clientInstance = nullptr;
    void* inventoryManager = nullptr;
    void* preloadManager = nullptr;
    void* contentIndex = nullptr;
    void* configManager = nullptr;
    void* preloadWorld = nullptr;
    bool inventoryCreated = false, preloadCreated = false;
    bool inventoryOwnerReference = false, preloadOwnerReference = false;
    bool inventoryCatalogRequested = false, inventoryCatalogPresent = false;
    bool preloadDependenciesBound = false, requestPathReady = false;
    int32_t equippableModels = 0, skinModels = 0, levelModels = 0, chromaModels = 0;
    uint32_t requestCount = 0, callbackEntered = 0, callbackCompleted = 0;
    unsigned error = 0;
    uintptr_t exceptionRva = 0;
    uintptr_t lastAttachmentTableRva = 0;
    void* lastAttachmentComponent = nullptr;
    const char* stage = "original_content_services_not_initialized";
    const char* operation = "none";
};
inline Diagnostics state;
inline bool restartRequired = false;
inline AttachmentObservation attachments[MaxObservedAttachments]{};
inline Name onAssetsLoadedName{};
inline Name getAssetPathsName{};
inline const Diagnostics& State() { return state; }
template<class T> inline T Read(const void* object, size_t offset) {
    return original_players::Value<T>(object, offset);
}
inline bool Fail(unsigned error, const char* text) {
    state.error = error; state.stage = text; return false;
}
inline bool Thread() {
    if (restartRequired) return Fail(state.error ? state.error : 1100,
        "original_content_services_exception_requires_new_session");
    return original_content::Thread()
        ? true : Fail(1124, "original_skin_services_wrong_or_missing_game_thread");
}
inline void* Construct(uintptr_t getter, void* outer) {
    struct Params { void* type; void* outer; void* result = nullptr; } p{engine::Class(getter), outer};
    static_assert(sizeof(Params) == 24);
    if (!p.type || (Read<uint32_t>(p.type, 0xdc) & 1)) return nullptr;
    auto library = engine::DefaultObject(engine::Class(0x43a9670));
    return library && engine::Event(library, L"/Script/Engine.GameplayStatics.SpawnObject", &p)
        ? p.result : nullptr;
}
inline bool Refresh() {
    if (!Thread()) return false;
    if (!state.clientInstance) return Fail(1125, "original_skin_services_not_initialized");
    state.inventoryManager = Read<void*>(state.clientInstance, 0x448);
    state.preloadManager = Read<void*>(state.clientInstance, 0x3f0);
    state.inventoryOwnerReference = original_players::IsA(state.inventoryManager, InventoryClass)
        && Read<void*>(state.inventoryManager, 0x28) == state.clientInstance;
    state.preloadOwnerReference = original_players::IsA(state.preloadManager, PreloadClass)
        && Read<void*>(state.preloadManager, 0x28) == state.clientInstance;
    if (!state.inventoryOwnerReference || !state.preloadOwnerReference)
        return Fail(1101, "original_skin_service_owner_reference_missing");
    state.equippableModels = Read<int32_t>(state.inventoryManager, 0x108);
    state.skinModels = original_content::MapCount(state.inventoryManager, 0x160);
    state.levelModels = original_content::MapCount(state.inventoryManager, 0x1b0);
    state.chromaModels = original_content::MapCount(state.inventoryManager, 0x250);
    state.inventoryCatalogPresent = state.equippableModels > 0 && state.skinModels > 0
        && state.levelModels > 0 && state.chromaModels > 0;
    state.preloadDependenciesBound = Read<void*>(state.preloadManager, 0x6b8) == state.contentIndex
        && Read<void*>(state.preloadManager, 0x6d0) == state.configManager;
    auto table = Read<void**>(state.preloadManager, 0);
    auto getWorld = Read<void*>(table, 0x1b8);
    state.preloadWorld = getWorld == reinterpret_cast<void*>(engine::image + 0x1c53940)
        ? reinterpret_cast<void*(*)(void*)>(getWorld)(state.preloadManager) : nullptr;
    state.requestPathReady = state.inventoryCatalogPresent && state.preloadDependenciesBound
        && state.preloadWorld == original_content::State().world;
    return true;
}
inline bool InitializeImpl(void* clientInstance) {
    if (!Thread()) return false;
    if (!original_content::State().initialized || !original_content::State().characterIdentitiesVerified)
        return Fail(1102, "original_skin_services_require_verified_original_content");
    if (!original_players::IsA(clientInstance, 0x6429080)
        || Read<void*>(original_content::State().gameInstance, 0x2b8) != clientInstance)
        return Fail(1103, "original_skin_services_client_instance_mismatch");
    if (state.clientInstance && state.clientInstance != clientInstance)
        return Fail(1104, "original_skin_services_client_changed_requires_new_session");
    state.clientInstance = clientInstance;
    state.contentIndex = original_content::State().index;
    state.configManager = Read<void*>(clientInstance, 0x748);
    if (!original_players::IsA(state.configManager, 0x66596c0))
        return Fail(1105, "original_skin_services_config_manager_missing");
    void* inventory = Read<void*>(clientInstance, 0x448);
    if (inventory && !original_players::IsA(inventory, InventoryClass))
        return Fail(1106, "original_skin_existing_inventory_type_mismatch");
    if (!inventory) {
        state.stage = "original_skin_constructing_original_inventory_manager";
        state.operation = "original_inventory_manager_constructor";
        inventory = Construct(InventoryClass, clientInstance);
        if (!original_players::IsA(inventory, InventoryClass))
            return Fail(1107, "original_skin_inventory_construction_failed");
        if (!original_players::Write(clientInstance, 0x448, inventory))
            return Fail(1108, "original_skin_inventory_owner_reference_failed");
        state.inventoryCreated = true;
    }
    state.inventoryManager = inventory;
    // Exact reflected provider fields of InventoryManager; no account, store,
    // ownership or platform identity is invented or initialized.
    if (!original_players::Write(inventory, 0xa8, state.contentIndex)
        || !original_players::Write(inventory, 0xc0, state.configManager))
        return Fail(1109, "original_skin_inventory_catalog_links_failed");
    if (!state.inventoryCatalogRequested) {
        if (Read<int32_t>(inventory, 0x108) || original_content::MapCount(inventory, 0x160))
            return Fail(1110, "original_skin_existing_inventory_catalog_requires_inspection");
        state.stage = "original_skin_building_original_equippable_catalog_models";
        state.operation = "original_inventory_build_equippable_models";
        state.inventoryCatalogRequested = true;
        // Original local catalog builder; only ContentIndex and local native
        // model constructors are used. The broader71e8050 account-model update
        // sequence and71e32b0 online event graph are deliberately not invoked.
        engine::function<void(*)(void*)>(0x71e81a0)(inventory);
    }
    void* preload = Read<void*>(clientInstance, 0x3f0);
    if (preload && !original_players::IsA(preload, PreloadClass))
        return Fail(1111, "original_skin_existing_preload_type_mismatch");
    if (!preload) {
        state.stage = "original_skin_constructing_original_preload_manager";
        state.operation = "original_preload_manager_constructor";
        preload = Construct(PreloadClass, clientInstance);
        if (!original_players::IsA(preload, PreloadClass))
            return Fail(1112, "original_skin_preload_construction_failed");
        if (!original_players::Write(clientInstance, 0x3f0, preload))
            return Fail(1113, "original_skin_preload_owner_reference_failed");
        state.preloadCreated = true;
    }
    state.preloadManager = preload;
    if (!original_players::Write(preload, 0x6b8, state.contentIndex)
        || !original_players::Write(preload, 0x6d0, state.configManager))
        return Fail(1114, "original_skin_preload_local_dependencies_failed");
    state.operation = "none";
    if (!Refresh()) return false;
    if (!state.requestPathReady) return Fail(1115, "original_skin_preload_path_dependencies_not_observed");
    state.error = 0; state.stage = "original_skin_catalog_and_preload_request_path_ready";
    return true;
}
inline int CaptureException(EXCEPTION_POINTERS* exception) {
    if (exception && exception->ExceptionRecord) {
        auto address = reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress);
        state.exceptionRva = address >= engine::image ? address - engine::image : address;
    }
    restartRequired = true;
    Fail(1116, "original_skin_services_native_exception_requires_new_session");
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* clientInstance) {
    __try { return InitializeImpl(clientInstance); }
    __except (CaptureException(GetExceptionInformation())) { return false; }
}
inline void ObserveAttachmentEvent(void* object, void* function, void* parameters) {
    AttachmentObservation* observed = nullptr;
    for (auto& entry : attachments) if (entry.component == object) { observed = &entry; break; }
    if (!observed || !observed->originalEvent) return;
    ++observed->processEventCalls;
    const auto name = Read<Name>(function, 0x18);
    const bool loaded = std::memcmp(&name, &onAssetsLoadedName, sizeof(Name)) == 0;
    const bool getPaths = std::memcmp(&name, &getAssetPathsName, sizeof(Name)) == 0;
    if (loaded) {
        ++observed->callbackEntered; ++state.callbackEntered;
        observed->lastAssetTypes = parameters ? *static_cast<const uint32_t*>(parameters) : 0;
    }
    if (getPaths) ++observed->pathsEntered;
    observed->originalEvent(object, function, parameters);
    if (loaded) { ++observed->callbackCompleted; ++state.callbackCompleted; }
    if (getPaths) {
        ++observed->pathsCompleted;
        // Original32-byte event parameters: flags0, TArray<SoftObjectPath>8,
        // native PreloadManager24. Read only the original returned count.
        observed->lastPathCount = Read<int32_t>(parameters, 0x10);
    }
}
inline AttachmentObservation* ObserveAttachment(void* component) {
    for (auto& entry : attachments) if (entry.component == component) return &entry;
    AttachmentObservation* slot = nullptr;
    for (auto& entry : attachments) if (!entry.component) { slot = &entry; break; }
    if (!slot) { Fail(1117, "original_skin_attachment_observer_capacity_exhausted"); return nullptr; }
    struct NativeTableProfile {
        uintptr_t classGetter, tableRva;
        uint32_t entries;
        uintptr_t lastCodeRva, boundaryRva;
    };
    // Exact native constructors and complete executable table extents are in
    // chroma-vtables.json. ChromaSkinOverride has a distinct156-slot primary
    // table; treating it as AresWeaponAttachment's157-slot table is incorrect.
    constexpr NativeTableProfile profiles[] = {
        {0x6574f50, 0xb114e20, 157, 0x6c8e950, 0},
        {0x662e900, 0xb1858d0, 156, 0x18a3a00, 0xc319d10},
        {0x662d8d0, 0xb184ed0, 156, 0x18a3a00, 0xc31a068}
    };
    auto actual = Read<void**>(component, 0);
    state.lastAttachmentComponent = component;
    state.lastAttachmentTableRva = reinterpret_cast<uintptr_t>(actual) >= engine::image
        ? reinterpret_cast<uintptr_t>(actual) - engine::image : 0;
    const NativeTableProfile* profile = nullptr;
    for (const auto& candidate : profiles)
        if (actual == reinterpret_cast<void**>(engine::image + candidate.tableRva)
            && original_players::IsA(component, candidate.classGetter)) { profile = &candidate; break; }
    if (!profile) {
        Fail(1118, "original_skin_attachment_observer_table_mismatch"); return nullptr;
    }
    auto expected = reinterpret_cast<void**>(engine::image + profile->tableRva);
    void* boundary = profile->boundaryRva ? reinterpret_cast<void*>(engine::image + profile->boundaryRva) : nullptr;
    if (expected[0x298 / 8] != reinterpret_cast<void*>(engine::image + 0x1cdc850)
        || expected[profile->entries - 1] != reinterpret_cast<void*>(engine::image + profile->lastCodeRva)
        || expected[profile->entries] != boundary) {
        Fail(1118, "original_skin_attachment_observer_extent_mismatch"); return nullptr;
    }
    void* fn = engine::Find(L"/Script/ShooterGame.AresWeaponAttachmentComponent.K2_OnAssetsLoaded");
    if (!fn || Read<uint16_t>(fn, 0xbe) != 4) {
        Fail(1119, "original_skin_assets_loaded_signature_missing"); return nullptr;
    }
    onAssetsLoadedName = Read<Name>(fn, 0x18);
    void* getPaths = engine::Find(L"/Script/ShooterGame.AresWeaponAttachmentComponent.K2_GetPreloadAssetPaths");
    if (!getPaths || Read<uint16_t>(getPaths, 0xbe) != 32) {
        Fail(1127, "original_skin_asset_paths_signature_missing"); return nullptr;
    }
    getAssetPathsName = Read<Name>(getPaths, 0x18);
    // Copy the entire pinned type-specific primary table and its preceding
    // compiler word. Observe only ProcessEvent; the+d8 interface is unchanged.
    const size_t tableBytes = (static_cast<size_t>(profile->entries) + 1) * sizeof(void*);
    auto allocation = static_cast<void**>(VirtualAlloc(nullptr, tableBytes,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!allocation) { Fail(1120, "original_skin_attachment_observer_allocation_failed"); return nullptr; }
    std::memcpy(allocation, expected - 1, tableBytes);
    auto copied = allocation + 1;
    slot->component = component; slot->originalTable = expected; slot->observedTable = copied;
    slot->nativeTableRva = profile->tableRva; slot->nativeTableEntries = profile->entries;
    slot->originalEvent = reinterpret_cast<ProcessEvent>(copied[0x298 / 8]);
    copied[0x298 / 8] = reinterpret_cast<void*>(&ObserveAttachmentEvent);
    if (InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(component), copied, expected) != expected) {
        *slot = {}; VirtualFree(allocation, 0, MEM_RELEASE);
        Fail(1121, "original_skin_attachment_changed_before_observer"); return nullptr;
    }
    return slot;
}
inline bool RequestAttachmentImpl(void* component) {
    if (!Thread() || !Refresh()) return false;
    if (!state.requestPathReady) return Fail(1115, "original_skin_preload_path_dependencies_not_observed");
    auto observed = ObserveAttachment(component);
    if (!observed) return false;
    if (observed->requestIssued) return Fail(1122, "original_skin_attachment_preload_already_requested");
    void* nativeInterface = engine::function<void*(*)(void*, void*)>(0x1cff3d0)
        (component, engine::Class(0x68ed750));
    if (nativeInterface != static_cast<uint8_t*>(component) + 0xd8)
        return Fail(1123, "original_skin_attachment_native_preload_interface_missing");
    auto interfaceTable = Read<void**>(nativeInterface, 0);
    if (Read<void*>(interfaceTable, 0x10) != reinterpret_cast<void*>(engine::image + 0x710aaa0)
        || Read<void*>(interfaceTable, 0x18) != reinterpret_cast<void*>(engine::image + 0x710ab80))
        return Fail(1126, "original_skin_attachment_native_preload_interface_changed");
    // Match native callers: meshes/materials/effects bitmask7, ordinary preload
    // priority0, an empty owned native delegate. The original method requests
    // actual AssetManager loads and owns the later OnAssetsLoaded dispatch.
    NativeDelegate completion;
    state.stage = "original_skin_requesting_original_attachment_assets";
    state.operation = "original_preload_from_preloadable_asset";
    observed->requestIssued = true; ++state.requestCount;
    engine::function<void(*)(void*, void*, uint8_t, int32_t, NativeDelegate*)>(0x6eb4fc0)
        (state.preloadManager, nativeInterface, 7, 0, &completion);
    state.operation = "none";
    state.error = 0; state.stage = "original_skin_waiting_for_original_asset_callbacks";
    return true;
}
inline bool RequestAttachment(void* component) {
    __try { return RequestAttachmentImpl(component); }
    __except (CaptureException(GetExceptionInformation())) { return false; }
}
inline bool WriteDiagnostics(const wchar_t* path) {
    if (!path || !*path) return false;
    std::wstring temporary(path); temporary += L".tmp";
    FILE* file = nullptr;
    if (_wfopen_s(&file, temporary.c_str(), L"wb") || !file) return false;
    const auto boolean = [](bool value) { return value ? "true" : "false"; };
    std::fprintf(file,
        "{\"schema_version\":1,\"stage\":\"%s\",\"operation\":\"%s\",\"error\":%u,"
        "\"exception_rva\":\"0x%llx\",\"last_attachment_component\":\"%p\",\"last_attachment_table_rva\":\"0x%llx\","
        "\"client_instance\":\"%p\",\"inventory_manager\":\"%p\","
        "\"preload_manager\":\"%p\",\"content_index\":\"%p\",\"config_manager\":\"%p\",\"preload_world\":\"%p\","
        "\"inventory_created\":%s,\"preload_created\":%s,\"inventory_owner_reference\":%s,"
        "\"preload_owner_reference\":%s,\"inventory_catalog_requested\":%s,\"inventory_catalog_present\":%s,"
        "\"preload_dependencies_bound\":%s,\"request_path_ready\":%s,\"equippable_models\":%d,"
        "\"skin_models\":%d,\"level_models\":%d,\"chroma_models\":%d,\"request_count\":%u,"
        "\"callback_entered\":%u,\"callback_completed\":%u,\"attachments\":[",
        state.stage, state.operation, state.error, static_cast<unsigned long long>(state.exceptionRva),
        state.lastAttachmentComponent, static_cast<unsigned long long>(state.lastAttachmentTableRva),
        state.clientInstance, state.inventoryManager, state.preloadManager, state.contentIndex, state.configManager,
        state.preloadWorld, boolean(state.inventoryCreated), boolean(state.preloadCreated),
        boolean(state.inventoryOwnerReference), boolean(state.preloadOwnerReference),
        boolean(state.inventoryCatalogRequested), boolean(state.inventoryCatalogPresent),
        boolean(state.preloadDependenciesBound), boolean(state.requestPathReady), state.equippableModels,
        state.skinModels, state.levelModels, state.chromaModels, state.requestCount, state.callbackEntered, state.callbackCompleted);
    bool first = true;
    for (const auto& entry : attachments) if (entry.component) {
        std::fprintf(file, "%s{\"component\":\"%p\",\"native_table_rva\":\"0x%llx\",\"native_table_entries\":%u,"
            "\"request_issued\":%s,\"callback_entered\":%u,"
            "\"callback_completed\":%u,\"last_asset_types\":%u,\"process_event_calls\":%u,"
            "\"paths_entered\":%u,\"paths_completed\":%u,\"last_path_count\":%d}", first ? "" : ",", entry.component,
            static_cast<unsigned long long>(entry.nativeTableRva), entry.nativeTableEntries,
            boolean(entry.requestIssued), entry.callbackEntered, entry.callbackCompleted, entry.lastAssetTypes,
            entry.processEventCalls, entry.pathsEntered, entry.pathsCompleted, entry.lastPathCount);
        first = false;
    }
    std::fputs("]}\n", file);
    const bool closed = std::fclose(file) == 0;
    return closed && MoveFileExW(temporary.c_str(), path, MOVEFILE_REPLACE_EXISTING);
}
}
