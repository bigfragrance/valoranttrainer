#pragma once
#include "original_players.h"

// Pinned original client. Missing actor components are added through the real
// Actor component lifecycle. No FXC, particle, material or readiness substitute
// is created here. See reports/duel1-original-effects.json for the native proof.
namespace original_effects {
inline constexpr uintptr_t ManagerClass = 0x6735e40;
inline constexpr uintptr_t LocationalClass = 0x6938560;
// Records only: original UObjects/FX are still created and reclaimed natively.
// Cover 32 participants, equipment, pooled drops and retained death actors.
inline constexpr size_t MaxManagers = 4096;
using ProcessEvent = void(*)(void*, void*, void*);
using ComponentEvent = void(*)(void*);
struct Name { uint32_t index = 0, comparison = 0, number = 0; };
static_assert(sizeof(Name) == 12);
struct WeakIdentity { int32_t index = -1, serial = 0; };
static_assert(sizeof(WeakIdentity) == 8);
struct Manager {
    const char* label = "actor";
    void *actor = nullptr, *component = nullptr, *outer = nullptr, *owner = nullptr;
    void **originalTable = nullptr, **observedTable = nullptr;
    ProcessEvent originalEvent = nullptr;
    ComponentEvent originalBeginPlay = nullptr, originalStartDeferred = nullptr;
    uintptr_t tableRva = 0;
    uint32_t tableEntries = 0;
    WeakIdentity actorIdentity{}, componentIdentity{};
    uint64_t generation = 0;
    bool identityVerified = false;
    bool locational = false, created = false, finishRequested = false;
    bool queryMatches = false, outerMatches = false, ownerMatches = false;
    bool registered = false, hasBegunPlay = false, actorHasBegunPlay = false;
    bool deferred = false, observerInstalled = false;
    uint32_t beginEntered = 0, beginCompleted = 0, startEntered = 0, startCompleted = 0;
    uint64_t processEvents = 0;
    int32_t pending = 0, pendingStart = 0, pendingStop = 0;
    int32_t serverActive = 0, clientActive = 0, pendingReplication = 0;
    uint32_t effectContainers = 0, unstoppedContainers = 0, maxEffectContainers = 0;
    void *lastContainer = nullptr, *lastContainerClass = nullptr, *lastTarget = nullptr;
};
struct Diagnostics {
    void* world = nullptr;
    bool initialized = false;
    uint32_t managerCount = 0, createdCount = 0, deferredCount = 0;
    uint32_t lifecycleCompleteCount = 0;
    uint64_t samples = 0, containerScans = 0;
    uint64_t totalManaged = 0, reusedRecordCount = 0, retireRequests = 0, retiredCount = 0, staleRetiredCount = 0;
    uint64_t restoredTableCount = 0, alreadyNativeCount = 0, expiredComponentNoWriteCount = 0;
    uint64_t identityChecks = 0, restoreConflicts = 0;
    uint64_t beginEntered = 0, beginCompleted = 0, startEntered = 0, startCompleted = 0;
    uint64_t processEvents = 0, untrackedNativeCallbacks = 0;
    uint32_t observerTableCount = 0;
    int32_t worldEffectContainers = 0;
    unsigned error = 0;
    uintptr_t exceptionRva = 0;
    const char* stage = "original_effects_not_initialized";
    const char* operation = "none";
};
inline Diagnostics state;
inline Manager managers[MaxManagers]{};
inline uint64_t slotUses[MaxManagers]{};
struct SharedObserverTable { void** original = nullptr; void** observed = nullptr; };
// At most two full tables for the process, one for each verified native type.
// A pending-kill UObject can still execute native destruction callbacks after
// its ordinary weak resolver becomes null. Its table must remain backed even
// when its diagnostic slot has been retired. Shared tables make this bounded.
inline SharedObserverTable observerTables[2]{};
inline bool restartRequired = false;
inline ULONGLONG lastContainerScan = 0;
inline const Diagnostics& State() { return state; }
template<class T> inline T Value(const void* object, size_t offset) {
    return original_players::Value<T>(object, offset);
}
inline bool Fail(unsigned code, const char* text) {
    state.error = code; state.stage = text; return false;
}
inline bool Thread() {
    if (restartRequired) return Fail(state.error ? state.error : 1600,
        "original_effects_exception_requires_new_session");
    return original_players::Thread()
        ? true : Fail(1601, "original_effects_wrong_or_missing_game_thread");
}
inline void* ResolveIdentity(const WeakIdentity& identity) {
    if (identity.index < 0 || identity.serial <= 0) return nullptr;
    return engine::function<void*(*)(const WeakIdentity*)>(0x1d4cfa0)(&identity);
}
inline bool CaptureIdentity(void* object, WeakIdentity& identity) {
    if (!original_players::Readable(object, 0x30)) return false;
    identity.index = Value<int32_t>(object, 0xc);
    if (identity.index < 0) return false;
    // Native weak serial allocator/getter and the original global object array
    // used by the original71695d7/71695ea call sites. No private GC roots.
    identity.serial = engine::function<int32_t(*)(void*, int32_t)>(0x1cf35b0)
        (reinterpret_cast<void*>(engine::image + 0xd7b87c0), identity.index);
    return identity.serial > 0 && ResolveIdentity(identity) == object;
}
inline bool Live(const WeakIdentity& identity, void* expected) {
    ++state.identityChecks;
    return expected && ResolveIdentity(identity) == expected;
}
inline bool IdentitiesLive(Manager& entry) {
    const bool actorLive = Live(entry.actorIdentity, entry.actor);
    const bool componentLive = Live(entry.componentIdentity, entry.component);
    entry.identityVerified = actorLive && componentLive;
    return entry.identityVerified;
}
inline void Recount() {
    state.managerCount = 0; state.deferredCount = 0; state.lifecycleCompleteCount = 0;
    for (const auto& entry : managers) if (entry.component) {
        ++state.managerCount;
        if (entry.deferred) ++state.deferredCount;
        if (entry.identityVerified && entry.outerMatches && entry.ownerMatches && entry.queryMatches
            && entry.registered && entry.hasBegunPlay && !entry.deferred) ++state.lifecycleCompleteCount;
    }
}
inline bool RetireEntry(Manager& entry, bool stale) {
    if (!entry.actor && !entry.component) return true;
    // Never dereference the caller's old actor pointer. Resolve the recorded
    // component identity first; a different serial at the same address is a
    // different object, and pending-kill objects are not written here either.
    void* component = ResolveIdentity(entry.componentIdentity);
    ++state.identityChecks;
    if (entry.observerInstalled && component == entry.component) {
        auto actual = Value<void**>(component, 0);
        if (actual == entry.observedTable) {
            if (InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(component),
                entry.originalTable, entry.observedTable) != entry.observedTable) {
                ++state.restoreConflicts;
                return Fail(1621, "original_effects_retire_table_changed_during_restore");
            }
            ++state.restoredTableCount;
        } else if (actual == entry.originalTable) {
            ++state.alreadyNativeCount;
        } else {
            ++state.restoreConflicts;
            return Fail(1622, "original_effects_retire_does_not_own_current_table");
        }
    } else if (entry.component && component != entry.component) {
        ++state.expiredComponentNoWriteCount;
    }
    ++state.retiredCount;
    if (stale) ++state.staleRetiredCount;
    // Only our observation record is cleared. Native component, owner arrays,
    // original deferred timer and active FXC objects remain under the engine.
    // Shared table storage stays valid for late native pending-kill callbacks.
    entry = {}; Recount(); return true;
}
inline bool SweepExpired() {
    for (auto& entry : managers) if (entry.actor || entry.component) {
        if (!IdentitiesLive(entry) && !RetireEntry(entry, true)) return false;
    }
    return true;
}
inline bool Event(void* object, const wchar_t* path, uint16_t size, void* parameters) {
    void* function = engine::Find(path);
    if (!function || Value<uint16_t>(function, 0xbe) != size) return false;
    auto table = Value<void**>(object, 0);
    auto event = Value<ProcessEvent>(table, 0x298);
    if (!event) return false;
    event(object, function, parameters); return true;
}
inline void* GetWorld(void* object) {
    auto table = Value<void**>(object, 0);
    auto fn = Value<void*(*)(void*)>(table, 0x1b8);
    return fn ? fn(object) : nullptr;
}
inline void* GetManager(void* actor, bool locational) {
    // Exact virtual query used by native6fe61a0 and6fe62e0 immediately before
    // the original "no effect manager" error. Do not invent a cached pointer.
    auto table = Value<void**>(actor, 0);
    auto fn = Value<void*(*)(void*, void*)>(table, 0x740);
    return fn ? fn(actor, engine::Class(locational ? LocationalClass : ManagerClass)) : nullptr;
}
inline Manager* Find(void* component) {
    if (!component) return nullptr;
    for (auto& entry : managers)
        if (entry.component == component && IdentitiesLive(entry)) return &entry;
    return nullptr;
}
inline void ObserveBeginPlay(void* object) {
    auto entry = Find(object);
    const auto generation = entry ? entry->generation : 0;
    auto original = entry && entry->originalBeginPlay ? entry->originalBeginPlay
        : engine::function<ComponentEvent>(0x6ef2470);
    ++state.beginEntered;
    if (entry) ++entry->beginEntered; else ++state.untrackedNativeCallbacks;
    original(object);
    ++state.beginCompleted;
    if (entry && entry->generation == generation && entry->component == object) ++entry->beginCompleted;
}
inline void ForwardStartDeferred(void* object, ComponentEvent original) {
    auto entry = Find(object);
    const auto generation = entry ? entry->generation : 0;
    ++state.startEntered;
    if (entry) ++entry->startEntered; else ++state.untrackedNativeCallbacks;
    original(object);
    ++state.startCompleted;
    if (entry && entry->generation == generation && entry->component == object) ++entry->startCompleted;
}
inline void ObserveBaseStartDeferred(void* object) {
    ForwardStartDeferred(object, engine::function<ComponentEvent>(0x6fcd170));
}
inline void ObserveLocationalStartDeferred(void* object) {
    ForwardStartDeferred(object, engine::function<ComponentEvent>(0x6fee1a0));
}
inline void ObserveEvent(void* object, void* function, void* parameters) {
    auto entry = Find(object);
    ++state.processEvents;
    if (entry) ++entry->processEvents; else ++state.untrackedNativeCallbacks;
    // A retired/pending-kill component can still receive an original engine
    // callback. Forward the exact pinned native body even without a cache row.
    auto original = entry && entry->originalEvent ? entry->originalEvent
        : engine::function<ProcessEvent>(0x1cdc850);
    original(object, function, parameters);
}
inline bool Observe(Manager& entry) {
    if (!IdentitiesLive(entry)) return Fail(1620, "original_effects_observer_identity_expired");
    if (entry.observerInstalled)
        return Value<void**>(entry.component, 0) == entry.observedTable
            ? true : Fail(1602, "original_effects_observer_was_replaced");
    struct Profile {
        uintptr_t getter, table, start, last, boundary;
        uint32_t entries;
    };
    // Full pinned primary table extents, including every derived virtual.
    // Locational has170 slots; copying the base165 would truncate it.
    constexpr Profile profiles[] = {
        {ManagerClass, 0xb21fc90, 0x6fcd170, 0x6fceef0, 0, 165},
        {LocationalClass, 0xb332af0, 0x6fee1a0, 0x6fee4d0, 0xc41a0c0, 170}
    };
    auto actual = Value<void**>(entry.component, 0);
    const Profile* selected = nullptr;
    for (const auto& candidate : profiles)
        if (actual == reinterpret_cast<void**>(engine::image + candidate.table)
            && original_players::IsA(entry.component, candidate.getter)) {
            selected = &candidate; break;
        }
    entry.tableRva = reinterpret_cast<uintptr_t>(actual) >= engine::image
        ? reinterpret_cast<uintptr_t>(actual) - engine::image : 0;
    if (!selected) return Fail(1603, "original_effects_component_table_requires_inspection");
    void* boundary = selected->boundary
        ? reinterpret_cast<void*>(engine::image + selected->boundary) : nullptr;
    if (actual[0x298 / 8] != reinterpret_cast<void*>(engine::image + 0x1cdc850)
        || actual[0x3e0 / 8] != reinterpret_cast<void*>(engine::image + 0x6ef2470)
        || actual[0x508 / 8] != reinterpret_cast<void*>(engine::image + selected->start)
        || actual[selected->entries - 1] != reinterpret_cast<void*>(engine::image + selected->last)
        || actual[selected->entries] != boundary)
        return Fail(1604, "original_effects_native_table_extent_mismatch");
    auto& shared = observerTables[selected == &profiles[1] ? 1 : 0];
    if (!shared.observed) {
        const size_t bytes = (static_cast<size_t>(selected->entries) + 1) * sizeof(void*);
        auto allocation = static_cast<void**>(VirtualAlloc(nullptr, bytes,
            MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        if (!allocation) return Fail(1605, "original_effects_observer_allocation_failed");
        // Keep the preceding compiler word and the whole primary table. The
        // native ResetForRespawn interface+d8 remains unchanged. This complete
        // read-only profile is shared by all instances, including old objects
        // already pending kill, so retirement never frees their live vtable.
        std::memcpy(allocation, actual - 1, bytes);
        auto copied = allocation + 1;
        copied[0x298 / 8] = reinterpret_cast<void*>(&ObserveEvent);
        copied[0x3e0 / 8] = reinterpret_cast<void*>(&ObserveBeginPlay);
        copied[0x508 / 8] = selected == &profiles[1]
            ? reinterpret_cast<void*>(&ObserveLocationalStartDeferred)
            : reinterpret_cast<void*>(&ObserveBaseStartDeferred);
        DWORD previous = 0;
        if (!VirtualProtect(allocation, bytes, PAGE_READONLY, &previous)) {
            VirtualFree(allocation, 0, MEM_RELEASE);
            return Fail(1623, "original_effects_shared_table_protection_failed");
        }
        shared.original = actual; shared.observed = copied; ++state.observerTableCount;
    }
    if (shared.original != actual) return Fail(1624, "original_effects_shared_native_profile_mismatch");
    auto copied = shared.observed;
    entry.originalTable = actual; entry.observedTable = copied;
    entry.tableEntries = selected->entries;
    entry.originalEvent = reinterpret_cast<ProcessEvent>(actual[0x298 / 8]);
    entry.originalBeginPlay = reinterpret_cast<ComponentEvent>(actual[0x3e0 / 8]);
    entry.originalStartDeferred = reinterpret_cast<ComponentEvent>(actual[0x508 / 8]);
    if (InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(entry.component), copied, actual) != actual) {
        entry.originalTable = nullptr; entry.observedTable = nullptr;
        entry.originalEvent = nullptr; entry.originalBeginPlay = nullptr; entry.originalStartDeferred = nullptr;
        return Fail(1606, "original_effects_component_changed_during_observer_install");
    }
    entry.observerInstalled = true; return true;
}
inline bool Refresh(Manager& entry) {
    if (!IdentitiesLive(entry)) return RetireEntry(entry, true);
    entry.outer = Value<void*>(entry.component, 0x28);
    entry.owner = Value<void*>(entry.component, 0xc8);
    entry.outerMatches = entry.outer == entry.actor;
    entry.ownerMatches = entry.owner == entry.actor;
    entry.queryMatches = GetManager(entry.actor, entry.locational) == entry.component;
    entry.registered = (Value<uint8_t>(entry.component, 0xa0) & 1) != 0;
    // Original ActorComponent::BeginPlay412d230 sets+c2 bit40.
    entry.hasBegunPlay = (Value<uint8_t>(entry.component, 0xc2) & 0x40) != 0;
    if (!entry.actorHasBegunPlay) {
        uint8_t begun = 0;
        entry.actorHasBegunPlay = Event(entry.actor, L"/Script/Engine.Actor.HasActorBegunPlay", 1, &begun) && begun;
    }
    entry.deferred = Value<uint8_t>(entry.component, 0xe0) != 0;
    entry.pending = Value<int32_t>(entry.component, 0xf0);
    entry.pendingStart = Value<int32_t>(entry.component, 0x100);
    entry.pendingStop = Value<int32_t>(entry.component, 0x110);
    entry.serverActive = Value<int32_t>(entry.component, 0x120);
    entry.clientActive = Value<int32_t>(entry.component, 0x130);
    entry.pendingReplication = Value<int32_t>(entry.component, 0x158);
    return true;
}
inline bool InitializeImpl(void* world) {
    if (!Thread()) return false;
    if (!world || world != original_players::world)
        return Fail(1607, "original_effects_requires_owned_original_world");
    if (state.initialized && state.world != world)
        return Fail(1608, "original_effects_world_changed_requires_new_session");
    state.world = world; state.initialized = true;
    state.error = 0; state.stage = "original_effects_world_bound";
    return true;
}
inline bool EnsureImpl(void* actor, bool locational, const char* label) {
    if (!Thread() || !state.initialized) return Fail(1609, "original_effects_not_initialized_on_game_thread");
    if (!SweepExpired()) return false;
    WeakIdentity actorIdentity{};
    if (!CaptureIdentity(actor, actorIdentity))
        return Fail(1625, "original_effects_actor_weak_identity_unavailable");
    if (!original_players::IsA(actor, 0x3683900) || GetWorld(actor) != state.world)
        return Fail(1610, "original_effects_actor_world_or_type_mismatch");
    if (locational && !original_players::IsA(actor, 0x3c0a200))
        return Fail(1611, "original_effects_locational_target_requires_player_controller");
    void* component = GetManager(actor, locational);
    Manager* entry = component ? Find(component) : nullptr;
    if (entry) {
        if (entry->actor != actor) return Fail(1626, "original_effects_manager_owner_record_mismatch");
        if (!Observe(*entry)) return false;
        if (!Refresh(*entry)) return false;
        Recount();
        return entry->queryMatches && entry->ownerMatches && entry->registered;
    }
    for (auto& candidate : managers) if (!candidate.component && !candidate.actor) { entry = &candidate; break; }
    if (!entry) return Fail(1612, "original_effects_manager_capacity_exhausted");
    entry->actor = actor; entry->label = label ? label : "actor"; entry->locational = locational;
    entry->actorIdentity = actorIdentity;
    if (!component) {
        struct alignas(16) Add {
            void* type; uint8_t manual = 0, pad[7]{}; engine::Transform transform;
            uint8_t deferred = 1, pad2[7]{}; void* result = nullptr;
        } add{engine::Class(locational ? LocationalClass : ManagerClass)};
        static_assert(sizeof(Add) == 128);
        state.operation = "original_actor_add_effect_manager_deferred";
        if (!Event(actor, L"/Script/Engine.Actor.AddComponentByClass", sizeof(Add), &add) || !add.result)
            return Fail(1613, "original_effects_add_component_failed");
        component = add.result; entry->created = true; ++state.createdCount;
    }
    entry->component = component;
    if (!CaptureIdentity(component, entry->componentIdentity))
        return Fail(1627, "original_effects_component_weak_identity_unavailable");
    entry->generation = ++state.totalManaged;
    if (slotUses[static_cast<size_t>(entry - managers)]++) ++state.reusedRecordCount;
    Recount();
    if (!original_players::IsA(component, locational ? LocationalClass : ManagerClass))
        return Fail(1614, "original_effects_created_component_type_mismatch");
    if (!Observe(*entry)) return false;
    if (entry->created) {
        struct alignas(16) Finish {
            void* component; uint8_t manual = 0, pad[7]{}; engine::Transform transform;
        } finish{component};
        static_assert(sizeof(Finish) == 112);
        entry->finishRequested = true;
        state.operation = "original_actor_finish_effect_manager_component";
        // Original FinishAddComponent owns registration, actor component strong
        // references and BeginPlay. BeginPlay schedules the real deferred-start
        // timer. Never handwrite+e0 or replay ResetForRespawn bindings.
        if (!Event(actor, L"/Script/Engine.Actor.FinishAddComponent", sizeof(Finish), &finish))
            return Fail(1615, "original_effects_finish_component_failed");
    }
    if (!Refresh(*entry)) return false;
    Recount();
    state.operation = "none";
    if (!entry->outerMatches || !entry->ownerMatches || !entry->queryMatches || !entry->registered)
        return Fail(1616, "original_effects_native_component_ownership_or_registration_missing");
    state.error = 0;
    state.stage = entry->deferred ? "original_effects_waiting_for_original_deferred_start"
        : "original_effects_component_lifecycle_observed";
    return true;
}
inline bool RetireActorImpl(void* actor) {
    if (!Thread()) return false;
    state.operation = "original_effects_retire_actor_observation";
    ++state.retireRequests;
    for (auto& entry : managers) if (entry.actor && entry.actor == actor) {
        const bool stale = !IdentitiesLive(entry);
        if (!RetireEntry(entry, stale)) return false;
    }
    // Idempotent even if the original actor/component was already destroyed.
    // Pointer equality chooses our cache record; only recorded native weak
    // identities can authorize restoration of the actual component table.
    Recount(); state.error = 0; state.stage = "original_effects_actor_observers_retired";
    state.operation = "none"; return true;
}
inline int CaptureException(EXCEPTION_POINTERS* exception) {
    if (exception && exception->ExceptionRecord) {
        auto at = reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress);
        state.exceptionRva = at >= engine::image ? at - engine::image : at;
    }
    restartRequired = true;
    Fail(1617, "original_effects_native_exception_requires_new_session");
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* world) {
    __try { return InitializeImpl(world); }
    __except (CaptureException(GetExceptionInformation())) { return false; }
}
inline bool EnsureActor(void* actor, const char* label = "actor") {
    __try { return EnsureImpl(actor, false, label); }
    __except (CaptureException(GetExceptionInformation())) { return false; }
}
inline bool EnsureController(void* controller, const char* label = "controller") {
    __try { return EnsureImpl(controller, true, label); }
    __except (CaptureException(GetExceptionInformation())) { return false; }
}
inline bool RetireActor(void* actor) {
    __try { return RetireActorImpl(actor); }
    __except (CaptureException(GetExceptionInformation())) { return false; }
}
inline bool ScanContainers() {
    struct Query {
        void* world; void* type; void** actors = nullptr; int32_t count = 0, capacity = 0;
    } query{state.world, engine::Class(0x648ada0)};
    static_assert(sizeof(Query) == 32);
    state.operation = "original_effect_container_actor_snapshot";
    if (!Event(engine::DefaultObject(engine::Class(0x43a9670)),
        L"/Script/Engine.GameplayStatics.GetAllActorsOfClass", sizeof(Query), &query))
        return Fail(1618, "original_effects_container_query_signature_missing");
    const bool valid = query.count >= 0 && query.count <= 65536
        && query.capacity >= query.count && (!query.count || query.actors);
    if (valid) {
        state.worldEffectContainers = query.count;
        for (auto& entry : managers) { entry.effectContainers = 0; entry.unstoppedContainers = 0; }
        for (int32_t index = 0; index < query.count; ++index) {
            void* actor = query.actors[index];
            // Reflected EffectContainer.OwningManager+618 and OriginalTarget+620;
            // native HasStopped673b910 returns the actual byte+5fc.
            auto entry = Find(Value<void*>(actor, 0x618));
            if (!entry) continue;
            ++entry->effectContainers;
            if (!Value<uint8_t>(actor, 0x5fc)) ++entry->unstoppedContainers;
            entry->lastContainer = actor; entry->lastContainerClass = Value<void*>(actor, 0x10);
            entry->lastTarget = Value<void*>(actor, 0x620);
        }
        for (auto& entry : managers) if (entry.effectContainers > entry.maxEffectContainers)
            entry.maxEffectContainers = entry.effectContainers;
        ++state.containerScans;
    }
    if (query.actors) engine::function<void(*)(void*)>(0x1915720)(query.actors);
    state.operation = "none";
    return valid ? true : Fail(1619, "original_effects_container_array_invalid");
}
inline bool PollImpl() {
    if (!Thread() || !state.initialized) return false;
    // The original weak resolver reads the object registry and cached serial,
    // before any actor/component dereference in Refresh or container matching.
    if (!SweepExpired()) return false;
    ++state.samples;
    for (auto& entry : managers) if (entry.component) {
        if (!Refresh(entry)) return false;
    }
    Recount();
    const auto now = GetTickCount64();
    if (now - lastContainerScan >= 250) {
        lastContainerScan = now;
        if (!ScanContainers()) return false;
    }
    if (!state.error) state.stage = state.deferredCount
        ? "original_effects_waiting_for_original_deferred_start" : "original_effects_native_lifecycle_sampled";
    return true;
}
inline bool Poll() {
    __try { return PollImpl(); }
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
        "\"exception_rva\":\"0x%llx\",\"world\":\"%p\",\"initialized\":%s,"
        "\"manager_count\":%u,\"created_count\":%u,\"deferred_count\":%u,\"lifecycle_complete_count\":%u,"
        "\"samples\":%llu,\"container_scans\":%llu,\"world_effect_containers\":%d",
        state.stage, state.operation, state.error, static_cast<unsigned long long>(state.exceptionRva),
        state.world, boolean(state.initialized), state.managerCount, state.createdCount,
        state.deferredCount, state.lifecycleCompleteCount, static_cast<unsigned long long>(state.samples),
        static_cast<unsigned long long>(state.containerScans), state.worldEffectContainers);
    std::fprintf(file,
        ",\"cache_capacity\":%u,\"observer_table_count\":%u,\"total_managed\":%llu,\"reused_record_count\":%llu,"
        "\"retire_requests\":%llu,\"retired_count\":%llu,\"stale_retired_count\":%llu,"
        "\"restored_table_count\":%llu,\"already_native_count\":%llu,\"expired_component_no_write_count\":%llu,"
        "\"identity_checks\":%llu,\"restore_conflicts\":%llu,\"begin_entered\":%llu,\"begin_completed\":%llu,"
        "\"start_entered\":%llu,\"start_completed\":%llu,\"process_events\":%llu,\"untracked_native_callbacks\":%llu,\"managers\":[",
        static_cast<unsigned>(MaxManagers), state.observerTableCount,
        static_cast<unsigned long long>(state.totalManaged), static_cast<unsigned long long>(state.reusedRecordCount),
        static_cast<unsigned long long>(state.retireRequests), static_cast<unsigned long long>(state.retiredCount),
        static_cast<unsigned long long>(state.staleRetiredCount), static_cast<unsigned long long>(state.restoredTableCount),
        static_cast<unsigned long long>(state.alreadyNativeCount), static_cast<unsigned long long>(state.expiredComponentNoWriteCount),
        static_cast<unsigned long long>(state.identityChecks), static_cast<unsigned long long>(state.restoreConflicts),
        static_cast<unsigned long long>(state.beginEntered), static_cast<unsigned long long>(state.beginCompleted),
        static_cast<unsigned long long>(state.startEntered), static_cast<unsigned long long>(state.startCompleted),
        static_cast<unsigned long long>(state.processEvents), static_cast<unsigned long long>(state.untrackedNativeCallbacks));
    bool first = true;
    for (const auto& entry : managers) if (entry.component || entry.actor) {
        std::fprintf(file,
            "%s{\"label\":\"%s\",\"actor\":\"%p\",\"component\":\"%p\",\"outer\":\"%p\",\"owner\":\"%p\","
            "\"locational\":%s,\"created\":%s,\"finish_requested\":%s,\"query_matches\":%s,"
            "\"outer_matches\":%s,\"owner_matches\":%s,\"registered\":%s,\"has_begun_play\":%s,"
            "\"actor_has_begun_play\":%s,\"deferred\":%s,\"observer_installed\":%s,"
            "\"native_table_rva\":\"0x%llx\",\"native_table_entries\":%u,"
            "\"begin_entered\":%u,\"begin_completed\":%u,\"start_entered\":%u,\"start_completed\":%u,"
            "\"process_events\":%llu,\"pending\":%d,\"pending_start\":%d,\"pending_stop\":%d,"
            "\"server_active\":%d,\"client_active\":%d,\"pending_replication\":%d,"
            "\"effect_containers\":%u,\"unstopped_containers\":%u,\"max_effect_containers\":%u,"
            "\"last_container\":\"%p\",\"last_container_class\":\"%p\",\"last_target\":\"%p\"",
            first ? "" : ",", entry.label, entry.actor, entry.component, entry.outer, entry.owner,
            boolean(entry.locational), boolean(entry.created), boolean(entry.finishRequested), boolean(entry.queryMatches),
            boolean(entry.outerMatches), boolean(entry.ownerMatches), boolean(entry.registered), boolean(entry.hasBegunPlay),
            boolean(entry.actorHasBegunPlay), boolean(entry.deferred), boolean(entry.observerInstalled),
            static_cast<unsigned long long>(entry.tableRva), entry.tableEntries,
            entry.beginEntered, entry.beginCompleted, entry.startEntered, entry.startCompleted,
            static_cast<unsigned long long>(entry.processEvents), entry.pending, entry.pendingStart, entry.pendingStop,
            entry.serverActive, entry.clientActive, entry.pendingReplication, entry.effectContainers,
            entry.unstoppedContainers, entry.maxEffectContainers, entry.lastContainer, entry.lastContainerClass, entry.lastTarget);
        std::fprintf(file,
            ",\"generation\":%llu,\"identity_verified\":%s,\"actor_object_index\":%d,\"actor_weak_serial\":%d,"
            "\"component_object_index\":%d,\"component_weak_serial\":%d}",
            static_cast<unsigned long long>(entry.generation), boolean(entry.identityVerified),
            entry.actorIdentity.index, entry.actorIdentity.serial, entry.componentIdentity.index, entry.componentIdentity.serial);
        first = false;
    }
    std::fputs("]}\n", file);
    const bool closed = std::fclose(file) == 0;
    return closed && MoveFileExW(temporary.c_str(), path, MOVEFILE_REPLACE_EXISTING);
}
}
