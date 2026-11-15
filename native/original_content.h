#pragma once
#include "original_players.h"

// Pinned image evidence: work/duel1-original-content-static. The original
// AresGameInstance constructor helper creates and owns this real index. Its
// synchronous initializer reads the installed AssetManager catalog; no platform
// initialization, account inventory, or online ClientGI state machine is used.
namespace original_content {
inline constexpr uintptr_t IndexClass = 0x66711f0;
inline constexpr uintptr_t CharacterDataClass = 0x640f850;
inline constexpr size_t MaxCharacters = 256;
struct Guid { uint32_t words[4]{}; };
static_assert(sizeof(Guid) == 16);
struct CharacterIdentity {
    void* dataAsset = nullptr;
    void* characterClass = nullptr;
    Guid id{};
    bool found = false;
};
struct Diagnostics {
    void* world = nullptr;
    void* gameInstance = nullptr;
    void* index = nullptr;
    void* indexOuter = nullptr;
    void* assetManager = nullptr;
    bool originalGameInstance = false;
    bool originalIndex = false;
    bool created = false;
    bool ownerReference = false;
    bool initializerRequested = false;
    bool initialized = false;
    bool shouldStoreCDOs = false;
    bool nativeCharacterEnumeration = false;
    bool characterIdentitiesVerified = false;
    int32_t characterCount = 0;
    int32_t verifiedCharacterCount = 0;
    int32_t equippableCount = 0;
    int32_t skinCount = 0;
    int32_t skinLevelCount = 0;
    int32_t chromaCount = 0;
    unsigned error = 0;
    uintptr_t exceptionRva = 0;
    const char* stage = "original_content_not_initialized";
    const char* operation = "none";
};
inline Diagnostics state;
inline DWORD gameThread = 0;
inline bool restartRequired = false;
inline CharacterIdentity characters[MaxCharacters]{};
inline const Diagnostics& State() { return state; }
template<class T> inline T Read(const void* object, size_t offset) {
    return original_players::Value<T>(object, offset);
}
inline bool Fail(unsigned code, const char* text) {
    state.error = code; state.stage = text; return false;
}
inline bool Thread() {
    if (restartRequired) return Fail(state.error ? state.error : 980,
        "original_content_native_exception_requires_new_session");
    return engine::image && gameThread && gameThread == GetCurrentThreadId()
        ? true : Fail(981, "original_content_wrong_game_thread");
}
inline bool Valid(const Guid& id) {
    return (id.words[0] | id.words[1] | id.words[2] | id.words[3]) != 0;
}
inline int32_t MapCount(void* index, size_t offset) {
    // This pinned TMap uses TSparseArray's Data.Num (+8) and NumFreeIndices
    // (+34), confirmed by its constructor and native sparse-map iteration.
    const int32_t allocated = Read<int32_t>(index, offset + 8);
    const int32_t free = Read<int32_t>(index, offset + 0x34);
    const int32_t bits = Read<int32_t>(index, offset + 0x28);
    if (allocated < 0 || allocated > 1000000 || free < 0 || free > allocated
        || bits != allocated) return -1;
    return allocated - free;
}
inline bool Refresh() {
    if (!Thread() || !state.gameInstance) return false;
    state.index = Read<void*>(state.gameInstance, 0x2c8);
    state.originalIndex = original_players::IsA(state.index, IndexClass);
    state.indexOuter = Read<void*>(state.index, 0x28);
    state.ownerReference = state.originalIndex && state.indexOuter == state.gameInstance;
    if (!state.originalIndex || !state.ownerReference)
        return Fail(982, "original_content_native_owner_reference_missing");
    state.initialized = Read<uint8_t>(state.index, 0xf8) != 0;
    state.shouldStoreCDOs = Read<uint8_t>(state.index, 0xf9) != 0;
    state.characterCount = MapCount(state.index, 0x330);
    state.equippableCount = MapCount(state.index, 0x100);
    state.skinCount = MapCount(state.index, 0x150);
    state.skinLevelCount = MapCount(state.index, 0x1a0);
    state.chromaCount = MapCount(state.index, 0x240);
    return true;
}
inline bool EnumerateCharacters() {
    struct PointerArray { void** data = nullptr; int32_t count = 0, capacity = 0; } result;
    static_assert(sizeof(PointerArray) == 16);
    state.operation = "original_content_get_all_characters";
    // Native ContentIndex.GetAllCharacters implementation: RCX=index,
    // RDX=out TArray<CharacterDataAsset*>. It also returns the out pointer.
    engine::function<void*(*)(void*, PointerArray*)>(0x71c5e60)(state.index, &result);
    state.nativeCharacterEnumeration = true;
    const bool validArray = result.count > 0
        && result.count <= static_cast<int32_t>(MaxCharacters)
        && result.count <= result.capacity && result.capacity <= 1000000
        && original_players::Readable(result.data, static_cast<size_t>(result.count) * sizeof(void*));
    state.verifiedCharacterCount = 0;
    if (validArray) {
        for (int32_t i = 0; i < result.count; ++i) {
            CharacterIdentity entry;
            entry.dataAsset = result.data[i];
            if (original_players::IsA(entry.dataAsset, CharacterDataClass)) {
                entry.id = Read<Guid>(entry.dataAsset, 0x38);
                // Native index lookup verifies that the GUID is the catalog's
                // actual identity, rather than a guessed identifier or label.
                void* indexed = engine::function<void*(*)(void*, const Guid*)>(0x71d1510)
                    (state.index, &entry.id);
                entry.found = Valid(entry.id) && indexed == entry.dataAsset;
            }
            characters[i] = entry;
            if (entry.found) ++state.verifiedCharacterCount;
        }
    }
    // The returned array owns a plain pointer allocation, not the asset
    // objects. The original index's reflected map retains the assets for GC.
    if (result.data) engine::function<void(*)(void*)>(0x1915720)(result.data);
    state.characterIdentitiesVerified = validArray && result.count == state.characterCount
        && state.verifiedCharacterCount == result.count;
    state.operation = "none";
    return state.characterIdentitiesVerified
        ? true : Fail(983, "original_content_character_catalog_not_verified");
}
inline bool InitializeImpl(void* world, bool storeCDOs) {
    if (!engine::image || !world) return Fail(984, "original_content_world_missing");
    if (!gameThread) gameThread = GetCurrentThreadId();
    if (!Thread()) return false;
    if (state.world && state.world != world)
        return Fail(985, "original_content_world_changed_requires_new_session");
    state.world = world;
    state.gameInstance = Read<void*>(world, 0x1d8);
    state.originalGameInstance = original_players::IsA(state.gameInstance, 0x647e8a0);
    if (!state.originalGameInstance) return Fail(986, "original_content_requires_real_ares_game_instance");
    void* context = Read<void*>(state.gameInstance, 0x38);
    if (!context || Read<void*>(context, 0x2c8) != world)
        return Fail(987, "original_content_game_instance_world_mismatch");
    // Follow the real Outer chain, as in the working original settings setup;
    // do not manufacture an unregistered AssetManager fallback singleton.
    void* nativeEngine = Read<void*>(state.gameInstance, 0x28);
    for (unsigned depth = 0; nativeEngine && depth < 8
        && !original_players::IsA(nativeEngine, 0x3df7bf0); ++depth)
        nativeEngine = Read<void*>(nativeEngine, 0x28);
    if (!original_players::IsA(nativeEngine, 0x3df7bf0))
        return Fail(988, "original_content_owning_engine_missing");
    state.assetManager = Read<void*>(nativeEngine, 0x3d8);
    if (!original_players::IsA(state.assetManager, 0x63eb090))
        return Fail(988, "original_content_original_asset_manager_missing");
    state.index = Read<void*>(state.gameInstance, 0x2c8);
    if (state.index && !original_players::IsA(state.index, IndexClass))
        return Fail(989, "original_content_existing_index_type_mismatch");
    if (!state.index) {
        state.stage = "original_content_constructing_original_index";
        state.operation = "original_ares_game_instance_construct_content_index";
        // Exact leaf called by original AresGI.Init at70471ca. It invokes the
        // original object constructor with Outer=GI and assigns GI+2c8 itself.
        // It does not call GI.Init or the ClientGI platform/auth lifecycle.
        engine::function<void(*)(void*)>(0x70470d0)(state.gameInstance);
        state.created = true;
        state.operation = "none";
    }
    if (!Refresh()) return false;
    if (!state.initialized) {
        if (state.initializerRequested)
            return Fail(990, "original_content_initializer_did_not_complete");
        state.stage = "original_content_running_original_synchronous_initializer";
        state.operation = "original_content_index_init_synchronous";
        state.initializerRequested = true;
        // This is the original sync branch of ContentIndexManager.InitSystem
        // (71e24b9). The original body populates all maps and owns ready+f8.
        engine::function<void(*)(void*, bool)>(0x71c4300)(state.index, storeCDOs);
        state.operation = "none";
        if (!Refresh()) return false;
    }
    if (!state.initialized) return Fail(991, "original_content_index_not_initialized");
    if (!EnumerateCharacters()) return false;
    state.error = 0; state.stage = "original_content_catalog_initialized_and_verified";
    return true;
}
inline int CaptureException(EXCEPTION_POINTERS* exception) {
    if (exception && exception->ExceptionRecord) {
        auto address = reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress);
        state.exceptionRva = address >= engine::image ? address - engine::image : address;
    }
    restartRequired = true;
    Fail(992, "original_content_native_exception_requires_new_session");
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* world, bool storeCDOs = true) {
    __try { return InitializeImpl(world, storeCDOs); }
    __except (CaptureException(GetExceptionInformation())) { return false; }
}
inline CharacterIdentity FindCharacterByClass(void* loadedCharacterClass) {
    CharacterIdentity result;
    if (!Thread() || !state.initialized || !state.characterIdentitiesVerified
        || !original_players::Derived(loadedCharacterClass, engine::Class(original_players::ShooterCharacterClass)))
        return result;
    for (int32_t i = 0; i < state.characterCount; ++i) {
        auto entry = characters[i];
        if (!entry.found) continue;
        // CharacterDataAsset.Character is the original TSoftClassPtr at+50.
        // Resolve only an already-loaded object: first its native weak pointer,
        // then FSoftObjectPath.ResolveObject(+58). Neither path loads every hero.
        auto weak = static_cast<uint8_t*>(entry.dataAsset) + 0x50;
        void* type = engine::function<void*(*)(void*)>(0x1d4cfa0)(weak);
        if (!type) type = engine::function<void*(*)(void*)>(0x1ce5c40)(weak + 8);
        if (type != loadedCharacterClass) continue;
        entry.characterClass = type;
        return entry;
    }
    return result;
}
inline bool WriteDiagnostics(const wchar_t* path) {
    if (!path || !*path) return false;
    std::wstring temporary(path); temporary += L".tmp";
    FILE* file = nullptr;
    if (_wfopen_s(&file, temporary.c_str(), L"wb") || !file) return false;
    const auto boolean = [](bool value) { return value ? "true" : "false"; };
    std::fprintf(file,
        "{\"schema_version\":1,\"stage\":\"%s\",\"operation\":\"%s\",\"error\":%u,"
        "\"exception_rva\":\"0x%llx\",\"world\":\"%p\",\"game_instance\":\"%p\","
        "\"index\":\"%p\",\"index_outer\":\"%p\",\"asset_manager\":\"%p\","
        "\"original_game_instance\":%s,\"original_index\":%s,\"created\":%s,"
        "\"owner_reference\":%s,\"initializer_requested\":%s,\"initialized\":%s,"
        "\"should_store_cdos\":%s,\"native_character_enumeration\":%s,"
        "\"character_identities_verified\":%s,\"character_count\":%d,"
        "\"verified_character_count\":%d,\"equippable_count\":%d,\"skin_count\":%d,"
        "\"skin_level_count\":%d,\"chroma_count\":%d}\n",
        state.stage, state.operation, state.error, static_cast<unsigned long long>(state.exceptionRva),
        state.world, state.gameInstance, state.index, state.indexOuter, state.assetManager,
        boolean(state.originalGameInstance), boolean(state.originalIndex), boolean(state.created),
        boolean(state.ownerReference), boolean(state.initializerRequested), boolean(state.initialized),
        boolean(state.shouldStoreCDOs), boolean(state.nativeCharacterEnumeration),
        boolean(state.characterIdentitiesVerified), state.characterCount, state.verifiedCharacterCount,
        state.equippableCount, state.skinCount, state.skinLevelCount, state.chromaCount);
    const bool closed = std::fclose(file) == 0;
    return closed && MoveFileExW(temporary.c_str(), path, MOVEFILE_REPLACE_EXISTING);
}
}
