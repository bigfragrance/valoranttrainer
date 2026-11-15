#pragma once
#include "original_hud.h"
#include "original_players.h"
#include "all_weapons.h"
#include <array>
#include <cstdio>

// Original in-match shop UI and a local missing-authority adapter. No custom
// in-game gun menu, account storefront, network initialization or shot code.
// Exact original metadata/factory sequence: reports/original-buy-menu.json.
namespace original_buy_menu {
using Guid = original_content::Guid;
using SelectedItemCallback = bool(*)(const Guid&);
using ProcessEvent = void(*)(void*, void*, void*);
inline constexpr size_t MaxModels = 128;
inline constexpr uintptr_t ControllerTable = 0xb50b540;
inline constexpr uintptr_t PlayerInfoTable = 0xb0ccd90;
inline constexpr size_t ControllerTableEntries = 623, PlayerInfoTableEntries = 246;
inline constexpr const wchar_t* StorePath = L"/Game/GameModes/Deathmatch/DeathmatchGameStore.DeathmatchGameStore_C";
inline constexpr const wchar_t* ScreenPath = L"/Game/UI/InGame/Shop/Shop_V2/WBP_Screen_PreRoundShopContainer.WBP_Screen_PreRoundShopContainer_C";
struct ObjectArray { void** data = nullptr; int32_t count = 0, capacity = 0; };
static_assert(sizeof(ObjectArray) == 16);
struct Name { uint32_t words[3]{}; };
struct ModelObservation {
    void* object = nullptr; void* purchasable = nullptr; void* weaponClass = nullptr;
    Guid id{}; int32_t price = 0;
    bool catalogMatched = false, canAfford = false, canPurchase = false;
    bool owned = false;
    bool ownedSlotsValid = false;
    uint32_t ownedSlotCount = 0;
    uint8_t ownedSlots[32]{};
};
struct Diagnostics {
    const char* stage = "original_buy_menu_not_initialized";
    unsigned error = 0;
    bool initialized = false, restartRequired = false;
    bool storeSupplied = false, screenSupplied = false, originalScreen = false;
    bool controllerHooked = false, playerInfoHooked = false;
    bool openKnown = false, open = false, nativeOpen = false, blockGameplay = false;
    bool uiOnlyApplied = false, localCursorEdgeWrite = false, cursorVisible = false;
    bool trainingAuthorityPolicy = false, shoppingEnabled = false;
    bool moneySupplied = false, moneyNotificationCalled = false;
    bool originalReadOnlyPagesUnmodified = true;
    uint32_t toggles = 0, updates = 0, selections = 0, selectionCallbacks = 0, selectionAccepted = 0;
    uint32_t canOpenPolicyQueries = 0, originalModelCount = 0, catalogModelCount = 0;
    uint32_t observedModelCount = 0, uiEdges = 0;
    uint32_t policyChecks = 0, openPrerequisites = 0;
    bool nativeCanOpen = false, reportWindowOpen = false;
    uint32_t toggleEvents = 0;
    uint32_t closeRequests = 0, closeCompletions = 0;
    bool originalCloseOnlyLeafUsed = false;
    bool coordinatorOpenBefore = false;
    bool buildProbe = false;
    int32_t money = 0, originalStoreCount = 0, shopEntryCount = 0;
    void *world = nullptr, *controller = nullptr, *pawn = nullptr, *playerState = nullptr;
    void *playerInfo = nullptr, *purchaseComponent = nullptr, *moneyComponent = nullptr;
    void *hud = nullptr, *uiCoordinator = nullptr, *coordinator = nullptr, *uiModel = nullptr;
    void *menuStack = nullptr, *screen = nullptr, *menuEntry = nullptr, *store = nullptr;
    void *selectedClass = nullptr;
    Guid selectedGuid{}, queuedGuid{};
    bool selectionQueued = false;
    bool ownedRefreshPending = false, ownedSourceVerified = false, ownedLeafVerified = false;
    uint32_t modelRefreshPasses = 0, modelRefreshCalls = 0, ownedChanges = 0, ownedRefreshDeferred = 0;
    uint32_t sampledOwnedModels = 0, lastInventoryGeneration = ~uint32_t(0), lastCompletedSequence = ~uint32_t(0);
    void* lastModelInventory = nullptr; void* ownedSourcePawn = nullptr;
    void* ownedSourceInventory = nullptr; void* purchaseComponentPlayerInfo = nullptr;
    uintptr_t exceptionRva = 0;
    std::array<ModelObservation, MaxModels> models{};
};
inline Diagnostics state{};
inline SelectedItemCallback selectedItemCallback = nullptr;
struct Hook {
    void* object = nullptr; void** originalTable = nullptr; void** copiedAllocation = nullptr;
    void** table = nullptr; ProcessEvent originalEvent = nullptr;
};
inline Hook controllerHook{}, playerInfoHook{};
struct Functions {
    void *serverPurchase = nullptr, *canOpenShop = nullptr, *toggle = nullptr;
    void *setShoppingEnabled = nullptr, *onRepMoney = nullptr, *setShopScreen = nullptr;
    void *setScreenCoordinator = nullptr, *setUIModel = nullptr;
    void *setAresInputMode = nullptr, *setUserFocus = nullptr;
};
inline Functions functions{};
inline bool inputManagedExternally = false;
inline const wchar_t* missingFunction=L"";
inline uint16_t missingFunctionExpected=0,missingFunctionActual=0;
inline const Diagnostics& State() { return state; }
inline bool BlocksGameplay() { return state.blockGameplay; }
inline void SetSelectedItemCallback(SelectedItemCallback callback) { selectedItemCallback = callback; }
template<class T> inline T Read(const void* p, size_t offset) { return original_players::Value<T>(p, offset); }
inline bool IsA(void* p, uintptr_t getter) { return original_players::IsA(p, getter); }
inline bool Fail(unsigned code, const char* stage) { state.error = code; state.stage = stage; return false; }
inline bool Thread() { return !state.restartRequired && engine::image && original_players::gameThread == GetCurrentThreadId(); }
inline void* Function(const wchar_t* path, uint16_t size) {
    void* fn = engine::Find(path);
    const auto actual=Read<uint16_t>(fn,0xbe);
    if(!fn||actual!=size){missingFunction=path;missingFunctionExpected=size;missingFunctionActual=actual;return nullptr;}
    return fn;
}
inline bool Event(void* object, void* fn, void* parameters) {
    auto table = Read<void**>(object, 0);
    auto pe = Read<ProcessEvent>(table, 0x298);
    if (!object || !fn || !pe) return false;
    pe(object, fn, parameters); return true;
}
inline void* Construct(uintptr_t base, const wchar_t* path, void* outer) {
    void* type = original_hud::LoadClass(base, path);
    return type ? original_hud::Construct(type, outer) : nullptr;
}
inline bool AppendPointer(void* object, size_t offset, void* value) {
    auto array = Read<ObjectArray>(object, offset);
    if (array.count != 0 || array.capacity < 0 || array.capacity > 4096) return false;
    auto native = static_cast<uint8_t*>(object) + offset;
    const int32_t one = 1;
    if (!original_players::Write(object, offset + 8, one)) return false;
    if (array.capacity < 1) engine::function<void(*)(void*, int32_t)>(0x18bd090)(native, 0);
    auto data = Read<void**>(object, offset);
    return data && original_players::Write(data, 0, value) && Read<void*>(data, 0) == value;
}
inline bool OpenState() {
    state.openKnown = false;
    auto entries = Read<ObjectArray>(state.menuStack, 0xb0);
    if (!state.menuEntry || entries.count < 0 || entries.count > 128
        || entries.capacity < entries.count || (entries.count && !entries.data)) return false;
    bool open = false;
    for (int32_t i = 0; i < entries.count; ++i)
        if (Read<void*>(entries.data, size_t(i) * 8) == state.menuEntry) { open = true; break; }
    state.open = open; state.openKnown = true;
    state.nativeOpen = Read<uint8_t>(state.screen, 0x31c) != 0;
    state.blockGameplay = open;
    return true;
}
inline void ObservePurchase(void* parameters) {
    if (!parameters || !state.trainingAuthorityPolicy || !OpenState() || !state.open) return;
    void* type = Read<void*>(parameters, 0);
    auto definition = all_weapons::FindByClass(type);
    if (!definition || !definition->identityVerified) return;
    state.selectedClass = type; state.selectedGuid = definition->id;
    state.queuedGuid = definition->id; state.selectionQueued = true;
    ++state.selections;
}
inline void ControllerEvent(void* object, void* fn, void* parameters) {
    if(object==state.controller&&fn==functions.toggle){
        ++state.toggleEvents;
        if(state.initialized&&parameters&&state.trainingAuthorityPolicy){
            const auto index=Read<int32_t>(parameters,0);
            if(index>=0&&index<state.shopEntryCount){
                // Use the very same original coordinator implementation called
                // by the original ToggleShop exec64eb760. The standalone local
                // controller has no online function-dispatch authority context.
                engine::function<void(*)(void*,int32_t)>(0x7a8d8e0)(state.coordinator,index);
                return;
            }
        }
    }
    // Intercept the original IN-MATCH ServerPurchase event from original shop
    // buttons only. Queue fulfillment outside the Blueprint/UMG call stack.
    if (object == state.controller && fn == functions.serverPurchase && parameters
        && state.trainingAuthorityPolicy && OpenState() && state.open
        && all_weapons::FindByClass(Read<void*>(parameters, 0))) {
        ObservePurchase(parameters);
        return; // Local missing-authority fulfillment replaces the absent RPC.
    }
    controllerHook.originalEvent(object, fn, parameters);
}
inline void PlayerInfoEvent(void* object, void* fn, void* parameters) {
    if (object == state.playerInfo && fn == functions.canOpenShop && parameters
        && state.trainingAuthorityPolicy) {
        // Explicit offline training authority policy, not a UI-ready flag.
        // The real store, controller, pawn and GS are already validated.
        auto gs=Read<void*>(state.world,0x178);
        state.policyChecks = (state.world==original_players::world ? 1U:0U)
            | (gs==original_players::createdGameState ? 2U:0U)
            | (engine::function<int(*)(void*)>(0x3efc820)(gs)==0 ? 4U:0U)
            | (Read<void*>(state.controller,0x470)==state.playerState ? 8U:0U)
            | (Read<void*>(state.playerInfo,0x4e8)==state.controller ? 16U:0U);
        *static_cast<uint8_t*>(parameters) = state.policyChecks==31 ? 1 : 0;
        ++state.canOpenPolicyQueries;
        return;
    }
    playerInfoHook.originalEvent(object, fn, parameters);
}
inline bool InstallHook(Hook& hook, void* object, uintptr_t nativeTableRva, size_t entries, ProcessEvent observer) {
    if (hook.object) return hook.object == object && Read<void**>(object, 0) == hook.table;
    auto actual = Read<void**>(object, 0);
    auto native = reinterpret_cast<void**>(engine::image + nativeTableRva);
    // Copy the complete exact pinned vtable including its preceding RTTI slot.
    // No original .text/.rdata page is patched or made private/COW.
    if (!actual || actual != native || !original_players::Readable(actual - 1, (entries + 1) * sizeof(void*))) return false;
    auto storage = static_cast<void**>(HeapAlloc(GetProcessHeap(), 0, (entries + 1) * sizeof(void*)));
    if (!storage) return false;
    std::memcpy(storage, actual - 1, (entries + 1) * sizeof(void*));
    hook.object = object; hook.originalTable = actual; hook.copiedAllocation = storage;
    hook.table = storage + 1; hook.originalEvent = reinterpret_cast<ProcessEvent>(actual[0x298 / 8]);
    hook.table[0x298 / 8] = reinterpret_cast<void*>(observer);
    if (!original_players::Write(object, 0, hook.table)) return false;
    return Read<void**>(object, 0) == hook.table;
}
inline bool PrepareStoreAndEconomy() {
    void* gs = Read<void*>(state.world, 0x178);
    if (!IsA(gs, 0x6c9d280) || Read<uint8_t>(gs, 0x224) != 3)
        return Fail(2405, "original_buy_menu_requires_original_authority_game_state");
    uint8_t enabled = 1;
    if (!Event(gs, functions.setShoppingEnabled, &enabled)) return Fail(2406, "original_buy_menu_shopping_rule_unavailable");
    state.shoppingEnabled = Read<uint8_t>(gs, 0xa08) != 0;
    // This training store switches weapons; practice abilities are granted
    // freely by the actual Jett loadout and are not shop purchases. Express
    // that as the original per-match shopping rule, retaining starting skills.
    if(original_scope::FieldOffset(Read<void*>(gs,0x10),L"bAllowCharacterSpecificEquippablesInShop")!=0x6f3)
        return Fail(2432,"original_buy_menu_character_specific_shop_rule_schema_changed");
    const uint8_t freePracticeAbilities=0;
    if(!original_players::Write(gs,0x6f3,freePracticeAbilities))return false;
    // Ask the original cache builder first. Only supply the missing local
    // store when the native array is empty; preserve an existing original list.
    engine::function<void*(*)(void*)>(0x718e090)(gs);
    auto stores = Read<ObjectArray>(gs, 0xaf8);
    if (!stores.count) {
        state.store = Construct(0x6541090, StorePath, gs);
        if (!IsA(state.store, 0x6541090) || Read<int32_t>(state.store, 0x40) <= 0)
            return Fail(2407, "original_buy_menu_original_deathmatch_store_catalog_missing");
        if (!AppendPointer(gs, 0xaf8, state.store)) return Fail(2408, "original_buy_menu_local_store_reference_failed");
        state.storeSupplied = true;
        stores = Read<ObjectArray>(gs, 0xaf8);
    }
    if (stores.count < 1 || stores.count > 16 || stores.capacity < stores.count || !stores.data)
        return Fail(2409, "original_buy_menu_original_store_array_invalid");
    state.originalStoreCount = stores.count; state.store = Read<void*>(stores.data, 0);
    if (!IsA(state.store, 0x6541090)) return Fail(2410, "original_buy_menu_original_store_type_mismatch");
    state.moneyComponent = Read<void*>(state.playerState, 0x8a0);
    if (!IsA(state.moneyComponent, 0x69cd280)) return Fail(2411, "original_buy_menu_original_money_component_missing");
    const int32_t oldMoney = Read<int32_t>(state.moneyComponent, 0xf8);
    // The client AuthSetMoney exec is an empty authority stub. Supply actual
    // local money once and invoke its real replication notification. Native
    // purchase/model predicates continue reading the actual original component.
    const int32_t trainingMoney = 9000;
    if (oldMoney < trainingMoney) {
        if (!original_players::Write(state.moneyComponent, 0xf8, trainingMoney)) return Fail(2412, "original_buy_menu_local_economy_write_failed");
        int32_t previous = oldMoney;
        if (!Event(state.moneyComponent, functions.onRepMoney, &previous)) return Fail(2413, "original_buy_menu_original_money_notification_missing");
        state.moneySupplied = true; state.moneyNotificationCalled = true;
    }
    state.money = Read<int32_t>(state.moneyComponent, 0xf8);
    state.trainingAuthorityPolicy = state.shoppingEnabled && state.money >= trainingMoney;
    return state.trainingAuthorityPolicy;
}
inline void BindMenuEvent(void* entry, size_t offset, const wchar_t* functionName) {
    Name name{};
    engine::function<void*(*)(void*, const wchar_t*, int)>(0x1a7b720)(&name, functionName, 1);
    // Same original dynamic-delegate helper/argument order as7a8cb47/cbb7.
    engine::function<void(*)(void*, void*, void*, const Name*)>(0x5ef6250)
        (static_cast<uint8_t*>(entry) + offset, state.coordinator, nullptr, &name);
}
inline bool EnsureOriginalScreen() {
    auto entries = Read<ObjectArray>(state.coordinator, 0x88);
    if (entries.count == 0) {
        // The original HUD built this coordinator before the offline GS had a
        // store. Complete the same original factory sequence with the authored
        // PC shop screen; its original categories/buttons/Blueprint remain intact.
        void* type = original_hud::LoadClass(0x6cd9450, ScreenPath);
        Name none{};
        void* widget = type ? engine::function<void*(*)(void*, void*, const Name*)>(0x3c514e0)
            (state.controller, type, &none) : nullptr;
        if (!IsA(widget, 0x6cd9450)) return Fail(2414, "original_buy_menu_original_authored_screen_creation_failed");
        void* ui = state.uiModel;
        if (!Event(widget, functions.setUIModel, &ui)) return Fail(2415, "original_buy_menu_original_screen_ui_model_binding_failed");
        void* coordinator = state.coordinator;
        if (!Event(widget, functions.setScreenCoordinator, &coordinator)
            || !Event(state.coordinator, functions.setShopScreen, &widget))
            return Fail(2416, "original_buy_menu_original_screen_coordinator_binding_failed");
        struct EmptyDelegate { void* storage = nullptr; int32_t count = 0, capacity = 0; } empty;
        void* entry = engine::function<void*(*)(void*, void*, void*, void*, bool, bool, bool, bool, void*)>(0x7ab5f30)
            (state.menuStack, widget, nullptr, nullptr, true, true, false, false, &empty);
        if (!IsA(entry, 0x699c5b0) || !AppendPointer(state.coordinator, 0x88, entry))
            return Fail(2417, "original_buy_menu_original_menu_entry_registration_failed");
        BindMenuEvent(entry, 0x48, L"OnOpenedShopScreen");
        BindMenuEvent(entry, 0x58, L"OnClosedShopScreen");
        state.screenSupplied = true;
        entries = Read<ObjectArray>(state.coordinator, 0x88);
    }
    if (entries.count < 1 || entries.count > 16 || entries.capacity < entries.count || !entries.data)
        return Fail(2418, "original_buy_menu_original_shop_entry_array_invalid");
    state.shopEntryCount = entries.count;
    state.menuEntry = Read<void*>(entries.data, 0); state.screen = Read<void*>(state.menuEntry, 0x30);
    state.originalScreen = IsA(state.menuEntry, 0x699c5b0) && IsA(state.screen, 0x6cd9450);
    return state.originalScreen ? true : Fail(2419, "original_buy_menu_original_screen_readback_failed");
}
inline bool SampleModels() {
    state.originalModelCount = 0; state.catalogModelCount = 0; state.observedModelCount = 0;
    auto groups = Read<ObjectArray>(state.coordinator, 0x98);
    if (!groups.data || groups.count < 1 || groups.count > 16 || groups.capacity < groups.count) return false;
    const auto models = Read<ObjectArray>(groups.data, 0);
    if (models.count < 0 || models.count > 512 || models.capacity < models.count || (models.count && !models.data)) return false;
    state.originalModelCount = models.count;
    for (int32_t i = 0; i < models.count; ++i) {
        void* object = Read<void*>(models.data, size_t(i) * 8);
        if (!IsA(object, 0x6cd8d70)) continue;
        void* purchasable = Read<void*>(object, 0x1c8);
        void* weaponClass = IsA(purchasable, 0x64f3360) ? Read<void*>(purchasable, 0xa0) : nullptr;
        auto d = all_weapons::FindByClass(weaponClass);
        if (d) ++state.catalogModelCount;
        if (state.observedModelCount < MaxModels) {
            auto& m = state.models[state.observedModelCount++];
            m = {}; m.object = object; m.purchasable = purchasable; m.weaponClass = weaponClass;
            m.price = Read<int32_t>(object, 0x30); m.canAfford = Read<uint8_t>(object, 0x180) != 0;
            m.canPurchase = Read<uint8_t>(object, 0x181) != 0; m.catalogMatched = d != nullptr;
            m.owned = Read<uint8_t>(object,0x183)!=0;
            struct SlotArray{uint8_t* data;int32_t count,capacity;};
            const auto ownedSlots=Read<SlotArray>(object,0x188);
            m.ownedSlotsValid=ownedSlots.count>=0&&ownedSlots.count<=32&&ownedSlots.capacity>=ownedSlots.count
                &&(!ownedSlots.count||original_players::Readable(ownedSlots.data,size_t(ownedSlots.count)));
            if(m.ownedSlotsValid){m.ownedSlotCount=uint32_t(ownedSlots.count);
                for(int32_t s=0;s<ownedSlots.count;++s)m.ownedSlots[s]=ownedSlots.data[s];}
            if (d) m.id = d->id;
        }
    }
    return true;
}
inline bool RefreshOwnedModelsIfCompleted(bool openedNow) {
    const auto& weapons=all_weapons::State();
    if(openedNow||state.lastInventoryGeneration!=weapons.generation
       ||state.lastCompletedSequence!=weapons.completedSequence||state.lastModelInventory!=weapons.inventory)
        state.ownedRefreshPending=true;
    if(!state.open||!state.ownedRefreshPending)return true;
    // Selection acceptance and weaponChanged precede retirement of the old
    // item. Use the original completed inventory edge, after RetireReplaced.
    if(weapons.busy||!weapons.ready||weapons.phase!=all_weapons::Phase::Idle){++state.ownedRefreshDeferred;return true;}
    state.ownedSourcePawn=Read<void*>(state.playerState,0xbc8);
    state.ownedSourceInventory=Read<void*>(state.ownedSourcePawn,0xc08);
    state.purchaseComponentPlayerInfo=Read<void*>(state.purchaseComponent,0xd8);
    state.ownedSourceVerified=state.ownedSourcePawn==weapons.pawn&&state.ownedSourceInventory==weapons.inventory
        &&state.purchaseComponentPlayerInfo==state.playerInfo&&Read<void*>(state.controller,0x470)==state.playerState;
    if(!state.ownedSourceVerified){++state.ownedRefreshDeferred;return true;}
    static constexpr uint8_t prefix[]{0x48,0x89,0x5c,0x24,0x18,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8d,0x6c,0x24,0xa0,0x48,0x81,0xec,0x60,0x01,0x00,0x00,0x48,0x8b,0xd9,0x33};
    if(!state.ownedLeafVerified){
        state.ownedLeafVerified=original_players::Readable(reinterpret_cast<void*>(engine::image+0x7a8e180),sizeof(prefix))
            &&std::memcmp(reinterpret_cast<void*>(engine::image+0x7a8e180),prefix,sizeof(prefix))==0;
        if(!state.ownedLeafVerified)return Fail(2434,"original_buy_menu_owned_refresh_original_leaf_profile_mismatch");
    }
    auto groups=Read<ObjectArray>(state.coordinator,0x98);
    if(groups.count<1||groups.count>16||groups.capacity<groups.count
       ||!original_players::Readable(groups.data,size_t(groups.count)*16))return Fail(2435,"original_buy_menu_owned_model_groups_invalid");
    state.sampledOwnedModels=0;
    for(int32_t g=0;g<groups.count;++g){
        // Original FShopPurchasableModelList is exactly16B, holding a pointer
        // TArray at0. Same full-group traversal as original Tick7a8fed0.
        auto models=Read<ObjectArray>(groups.data,size_t(g)*16);
        if(models.count<0||models.count>256||models.capacity<models.count
           ||(models.count&&!original_players::Readable(models.data,size_t(models.count)*8)))return Fail(2435,"original_buy_menu_owned_model_group_invalid");
        for(int32_t i=0;i<models.count;++i){auto model=models.data[i];
            if(!IsA(model,0x6cd8d70)||Read<void*>(model,0x1c0)!=state.playerInfo)
                return Fail(2436,"original_buy_menu_owned_model_player_identity_mismatch");
            const bool before=Read<uint8_t>(model,0x183)!=0;
            // ORIGINAL refresh derives owned slots/current inventory, updates
            // bIsOwned itself, then broadcasts original OnModelUpdated+1a8.
            engine::function<void(*)(void*)>(0x7a8e180)(model);++state.modelRefreshCalls;
            const bool after=Read<uint8_t>(model,0x183)!=0;
            if(before!=after)++state.ownedChanges;if(after)++state.sampledOwnedModels;
        }
    }
    ++state.modelRefreshPasses;state.lastInventoryGeneration=weapons.generation;
    state.lastCompletedSequence=weapons.completedSequence;state.lastModelInventory=weapons.inventory;
    state.ownedRefreshPending=false;return SampleModels();
}
inline bool ApplyInputEdge(bool open, bool otherUIActive) {
    if (!open && otherUIActive) { state.uiOnlyApplied = false; return true; }
    uint8_t wanted = open ? 1 : 0;
    if (!Event(state.controller, functions.setAresInputMode, &wanted)) return false;
    if (open && functions.setUserFocus) {
        void* pc = state.controller;
        Event(state.screen, functions.setUserFocus, &pc);
    }
    const uint32_t flags = Read<uint32_t>(state.controller, 0x73c);
    if (((flags & 1U) != 0) != open) {
        const uint32_t value = (flags & ~1U) | (open ? 1U : 0U);
        if (!original_players::Write(state.controller, 0x73c, value)) return false;
        state.localCursorEdgeWrite = true;
    }
    state.uiOnlyApplied = open; ++state.uiEdges;
    state.cursorVisible = (Read<uint32_t>(state.controller, 0x73c) & 1U) != 0;
    return true;
}
inline bool InitializeImpl(void* world, void* controller) {
    if (!Thread()) return Fail(2400, "original_buy_menu_wrong_game_thread");
    if (state.initialized) return state.world == world && state.controller == controller;
    if (!all_weapons::State().initialized) return Fail(2401, "original_buy_menu_requires_original_weapon_catalog");
    if (world != original_players::world || controller != original_players::activeController
        || Read<void*>(Read<void*>(controller, 0x510), 0x38) != controller || !IsA(controller, 0x6943db0))
        return Fail(2402, "original_buy_menu_real_local_controller_required");
    state.world = world; state.controller = controller; state.pawn = Read<void*>(controller, 0x4b0);
    state.playerState = Read<void*>(controller, 0x470); state.playerInfo = Read<void*>(state.playerState, 0xa18);
    state.hud = Read<void*>(controller, 0x520); state.uiCoordinator = Read<void*>(state.hud, 0x760);
    state.coordinator = Read<void*>(state.uiCoordinator, 0x68);
    state.uiModel = Read<void*>(state.uiCoordinator, 0x80); state.menuStack = Read<void*>(state.uiCoordinator, 0x50);
    const auto controllerCoordinator=Read<void*>(controller,0xc30);
    if(controllerCoordinator!=state.uiCoordinator){
        if(controllerCoordinator||Read<void*>(state.uiCoordinator,0xa0)!=controller)return Fail(2430,"original_buy_menu_controller_ui_coordinator_identity_conflict");
        auto setter=engine::Find(L"/Script/ShooterGame.AresPlayerController.SetUICoordinator");
        auto coordinator=state.uiCoordinator;
        if(setter&&Read<uint16_t>(setter,0xbe)==8){if(!Event(controller,setter,&coordinator))return false;}
        else if(!original_players::Write(controller,0xc30,coordinator))return false;
        if(Read<void*>(controller,0xc30)!=coordinator)return Fail(2431,"original_buy_menu_original_controller_ui_link_not_observed");
    }
    if (!IsA(state.playerInfo,0x64f4540))return Fail(2403,"original_buy_menu_player_info_type_missing");
    if (Read<void*>(state.playerInfo,0x4e8)!=controller)return Fail(2403,"original_buy_menu_player_info_controller_mismatch");
    if (!IsA(state.uiModel,0x6cd6540))return Fail(2403,"original_buy_menu_coordinated_model_type_missing");
    if (!IsA(state.menuStack,0x699c660))return Fail(2403,"original_buy_menu_menu_stack_type_missing");
    state.purchaseComponent = Read<void*>(state.playerInfo, 0x550);
    if (!IsA(state.purchaseComponent, 0x6a78fe0)) return Fail(2404, "original_buy_menu_original_player_purchase_component_missing");
    engine::DefaultObject(engine::Class(0x6cd95b0));
    engine::DefaultObject(engine::Class(0x6cd9450));
    // Reflected parameters end after the two byte enums at offsets8/9.
    // The aligned generated C++ storage is16 bytes; runtime ParmsSize is10.
    functions.serverPurchase = Function(L"/Script/ShooterGame.ShooterPlayerController.ServerPurchase", 10);
    functions.canOpenShop = Function(L"/Script/ShooterGame.OwnerExclusivePlayerInfo.CanOpenShop", 1);
    functions.toggle = Function(L"/Script/ShooterGame.AresPlayerController.ToggleShop", 4);
    functions.setShoppingEnabled = Function(L"/Script/ShooterGame.ShooterGameState.SetShoppingEnabled", 1);
    functions.onRepMoney = Function(L"/Script/ShooterGame.MoneyManagementComponent.OnRep_Money", 4);
    functions.setShopScreen = Function(L"/Script/ShooterGame.ShopScreenCoordinator.SetShopScreen", 8);
    functions.setScreenCoordinator = Function(L"/Script/ShooterGame.ShopScreen.SetShopScreenCoordinator", 8);
    functions.setUIModel = Function(L"/Script/ShooterGame.ShopScreen.SetCoordinatedShooterUIModel", 8);
    functions.setAresInputMode = Function(L"/Script/ShooterGame.AresPlayerController.SetAresInputMode", 1);
    functions.setUserFocus = Function(L"/Script/UMG.Widget.SetUserFocus", 8);
    if (!functions.serverPurchase || !functions.canOpenShop || !functions.toggle || !functions.setShoppingEnabled
        || !functions.onRepMoney || !functions.setShopScreen || !functions.setScreenCoordinator || !functions.setUIModel
        || !functions.setAresInputMode) return Fail(2420, "original_buy_menu_original_reflected_api_signature_mismatch");
    if (!PrepareStoreAndEconomy()) return false;
    if (!InstallHook(controllerHook, controller, ControllerTable, ControllerTableEntries, ControllerEvent)
        || !InstallHook(playerInfoHook, state.playerInfo, PlayerInfoTable, PlayerInfoTableEntries, PlayerInfoEvent))
        return Fail(2421, "original_buy_menu_instance_event_observer_profile_mismatch");
    state.controllerHooked = true; state.playerInfoHooked = true;
    if(!state.coordinator){
        // Original factory7a8c590: RDX=real PC, R8=real coordinated UI model,
        // R9=real menu stack, RAX=new original ShopScreenCoordinator. It builds
        // the authored store entries and original ToggleShop input/delegates.
        // The offline HUD's missing factory call is supplied once; no model or
        // menu-state flags are fabricated.
        state.coordinator=engine::function<void*(*)(void*,void*,void*,void*)>(0x7a8c590)
            (state.uiCoordinator,controller,state.uiModel,state.menuStack);
        if(!IsA(state.coordinator,0x6cd95b0)||!original_players::Write(state.uiCoordinator,0x68,state.coordinator))
            return Fail(2427,"original_buy_menu_original_coordinator_factory_failed");
    }
    if(Read<void*>(state.coordinator,0x80)!=controller||Read<void*>(state.coordinator,0x78)!=state.menuStack
       ||Read<void*>(state.coordinator,0x68)!=state.uiModel)return Fail(2428,"original_buy_menu_original_factory_bindings_not_observed");
    auto playerState=state.playerState;
    auto notify=Function(L"/Script/ShooterGame.ShopScreenCoordinator.HandleLocalPlayerStateChanged",8);
    if(!notify||!Event(state.coordinator,notify,&playerState))return Fail(2429,"original_buy_menu_original_player_state_notification_failed");
    if (!EnsureOriginalScreen()) return false;
    if (!selectedItemCallback) selectedItemCallback = [](const Guid& id) { return all_weapons::Equip(id); };
    OpenState(); state.initialized = true; state.error = 0;
    state.stage = "original_buy_menu_original_screen_and_local_selection_bridge_initialized";
    return true;
}
inline bool CloseImpl() {
    if(!Thread()||!state.initialized)return Fail(2422,"original_buy_menu_initialize_required");
    if(!OpenState())return Fail(2423,"original_buy_menu_actual_open_state_unknown");
    ++state.closeRequests;
    if(!state.open){++state.closeCompletions;return true;}
    auto iface=static_cast<uint8_t*>(state.coordinator)+0x30;
    auto table=Read<void**>(iface,0);
    if(table!=reinterpret_cast<void**>(engine::image+0xb523350)
       ||Read<void*>(table,0x38)!=reinterpret_cast<void*>(engine::image+0x7a8df10))
        return Fail(2432,"original_buy_menu_original_close_interface_profile_mismatch");
    // Original Shop close-only interface: CloseEntry for every real shop entry,
    // then original7ab7760 recomputes stack activation/visibility/input owner.
    // Calling CloseEntry alone omitted this required second stage.
    engine::function<void(*)(void*)>(0x7a8df10)(iface);state.originalCloseOnlyLeafUsed=true;
    if(!OpenState()||state.open)return Fail(2433,"original_buy_menu_original_close_not_observed");
    ++state.closeCompletions;state.error=0;state.stage="original_buy_menu_original_shop_closed";return true;
}
inline bool ToggleImpl() {
    if (!Thread() || !state.initialized) return Fail(2422, "original_buy_menu_initialize_required");
    if (!OpenState()) return Fail(2423, "original_buy_menu_actual_open_state_unknown");
    if (state.open) {
        if(!CloseImpl())return false;
    } else {
        auto nativeInfo=engine::function<void*(*)(void*)>(0x78b2640)(state.controller);
        auto nativeState=engine::function<void*(*)(void*)>(0x78b1f80)(state.controller);
        auto getWorld=Read<void*(*)(void*)>(Read<void**>(state.controller,0),0x1b8);
        auto nativeWorld=getWorld?getWorld(state.controller):nullptr;
        auto nativeScreen=engine::function<void*(*)(void*,int32_t)>(0x7a8d5c0)(state.coordinator,0);
        state.nativeCanOpen=nativeInfo&&engine::function<bool(*)(void*)>(0x64f74f0)(nativeInfo);
        state.openPrerequisites=(nativeInfo==state.playerInfo?1U:0U)|(nativeState==state.playerState?2U:0U)
            |(nativeWorld==state.world?4U:0U)|(Read<void*>(nativeWorld,0x178)==original_players::createdGameState?8U:0U)
            |(nativeScreen==state.screen?16U:0U)|(state.nativeCanOpen?32U:0U);
        state.reportWindowOpen=Read<uint8_t>(Read<void*>(state.uiCoordinator,0x78),0x89)!=0;
        auto iface=static_cast<uint8_t*>(state.coordinator)+0x30;
        auto isOpen=Read<bool(*)(void*)>(Read<void**>(iface,0),8);
        state.coordinatorOpenBefore=isOpen&&isOpen(iface);
        int32_t index = 0;
        if (!Event(state.controller, functions.toggle, &index)) return Fail(2424, "original_buy_menu_original_toggle_dispatch_failed");
    }
    ++state.toggles; OpenState(); SampleModels(); state.error = 0;
    state.stage = state.open ? "original_buy_menu_original_shop_open" : "original_buy_menu_original_shop_closed";
    return true;
}
inline bool UpdateImpl(bool otherUIActive) {
    if (!Thread() || !state.initialized) return false;
    ++state.updates;
    state.cursorVisible=(Read<uint32_t>(state.controller,0x73c)&1U)!=0;
    const bool wasOpen = state.open;
    if (!OpenState()) return Fail(2423, "original_buy_menu_actual_open_state_unknown");
    if (!inputManagedExternally && state.open != state.uiOnlyApplied && (!otherUIActive || state.open))
        if (!ApplyInputEdge(state.open, otherUIActive)) return Fail(2425, "original_buy_menu_original_input_edge_failed");
    if (wasOpen != state.open || (state.open && state.originalModelCount == 0)) SampleModels();
    if (state.selectionQueued && selectedItemCallback) {
        const Guid id = state.queuedGuid; state.selectionQueued = false;
        ++state.selectionCallbacks;
        if (selectedItemCallback(id)) { ++state.selectionAccepted;state.ownedRefreshPending=true; }
        // true means request accepted, never that the original equip lifecycle
        // completed. Root/all_weapons observes current GUID and rebinds HUD.
    }
    if(!RefreshOwnedModelsIfCompleted(!wasOpen&&state.open))return false;
    return true;
}
inline LONG Capture(EXCEPTION_POINTERS* exception) {
    auto at = exception && exception->ExceptionRecord ? reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress) : 0;
    state.exceptionRva = at >= engine::image ? at - engine::image : at;
    state.restartRequired = true; state.blockGameplay = true;
    Fail(2426, "original_buy_menu_native_exception_requires_new_session");
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* world, void* controller) {
    __try { return InitializeImpl(world, controller); }
    __except (Capture(GetExceptionInformation())) { return false; }
}
inline bool Toggle() {
    __try { return ToggleImpl(); }
    __except (Capture(GetExceptionInformation())) { return false; }
}
inline bool Close() {
    __try { return CloseImpl(); }
    __except (Capture(GetExceptionInformation())) { return false; }
}
inline bool Update(bool otherUIActive = false) {
    __try { return UpdateImpl(otherUIActive); }
    __except (Capture(GetExceptionInformation())) { return false; }
}
inline bool WriteDiagnostics(const wchar_t* path) {
    if (!path || wcslen(path) > 960) return false;
    wchar_t temporary[1024]{};
    if (swprintf_s(temporary, L"%ls.tmp", path) < 0) return false;
    FILE* f = nullptr;
    if (_wfopen_s(&f, temporary, L"wb") || !f) return false;
    std::fprintf(f, "{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"open_known\":%s,\"open\":%s,\"native_open\":%s,"
        "\"block_gameplay\":%s,\"original_screen\":%s,\"store_supplied\":%s,\"screen_supplied\":%s,\"training_authority_policy\":%s,"
        "\"controller_hooked\":%s,\"player_info_hooked\":%s,\"immutable_pages_unmodified\":true,\"money\":%d,\"store_count\":%d,\"entry_count\":%d,"
        "\"model_count\":%u,\"catalog_model_count\":%u,\"toggles\":%u,\"selections\":%u,\"callbacks\":%u,\"requests_accepted\":%u,"
        "\"cursor_visible\":%s,\"ui_edges\":%u,\"local_cursor_edge_write\":%s,\"exception_rva\":\"0x%llx\",\"restart_required\":%s,\"models\":[",
        state.stage, state.error, state.initialized ? "true" : "false", state.openKnown ? "true" : "false", state.open ? "true" : "false", state.nativeOpen ? "true" : "false",
        state.blockGameplay ? "true" : "false", state.originalScreen ? "true" : "false", state.storeSupplied ? "true" : "false", state.screenSupplied ? "true" : "false", state.trainingAuthorityPolicy ? "true" : "false",
        state.controllerHooked ? "true" : "false", state.playerInfoHooked ? "true" : "false", state.money, state.originalStoreCount, state.shopEntryCount,
        state.originalModelCount, state.catalogModelCount, state.toggles, state.selections, state.selectionCallbacks, state.selectionAccepted,
        state.cursorVisible ? "true" : "false", state.uiEdges, state.localCursorEdgeWrite ? "true" : "false", static_cast<unsigned long long>(state.exceptionRva), state.restartRequired ? "true" : "false");
    for (uint32_t i = 0; i < state.observedModelCount; ++i) {
        const auto& m = state.models[i];
    std::fprintf(f, "%s{\"object\":\"0x%llx\",\"weapon_class\":\"0x%llx\",\"price\":%d,\"can_afford\":%s,\"can_purchase\":%s,\"catalog_matched\":%s,\"guid_words\":[%u,%u,%u,%u],\"owned\":%s,\"owned_slots_valid\":%s,\"inventory_slots_owned_in\":[",
            i ? "," : "", original_hud::Pointer(m.object), original_hud::Pointer(m.weaponClass), m.price,
            m.canAfford ? "true" : "false", m.canPurchase ? "true" : "false", m.catalogMatched ? "true" : "false", m.id.words[0], m.id.words[1], m.id.words[2], m.id.words[3],m.owned?"true":"false",m.ownedSlotsValid?"true":"false");
        for(uint32_t s=0;s<m.ownedSlotCount&&s<32;++s)std::fprintf(f,"%s%u",s?",":"",unsigned(m.ownedSlots[s]));
        std::fputs("]}",f);
    }
    std::fprintf(f,"] ,\"graph\":{\"player_info\":\"%p\",\"player_info_controller\":\"%p\",\"controller\":\"%p\",\"coordinator\":\"%p\",\"shop_controller\":\"%p\",\"shop_menu_stack\":\"%p\",\"menu_stack\":\"%p\",\"model\":\"%p\"},\"missing_function\":",state.playerInfo,Read<void*>(state.playerInfo,0x4e8),state.controller,state.coordinator,Read<void*>(state.coordinator,0x80),Read<void*>(state.coordinator,0x78),state.menuStack,state.uiModel);
    const auto missing=original_players::Utf8(missingFunction);original_players::Text(f,missing.c_str());
    auto limiter=static_cast<uint8_t*>(state.controller)+0xed0;
    std::fprintf(f,",\"world_time\":%.9g,\"world_real_time\":%.9g,\"purchase_limiter\":{\"history\":%d,\"mode\":%u,\"gap\":%.9g,\"limit\":%d,\"window\":%.9g,\"penalty\":%.9g,\"next\":%.9g}",Read<double>(state.world,0x740),Read<double>(state.world,0x758),Read<int32_t>(limiter,0x10),static_cast<unsigned>(Read<uint8_t>(limiter,0x18)),Read<float>(limiter,0x1c),Read<int32_t>(limiter,0x20),Read<float>(limiter,0x24),Read<float>(limiter,0x28),Read<float>(limiter,0));
    std::fprintf(f,",\"ui_coordinator\":\"%p\",\"current_shop_coordinator\":\"%p\",\"toggle_events\":%u,\"toggle_exec_rva\":%llu,\"coordinator_open_before\":%s,\"build_probe\":%s",state.uiCoordinator,Read<void*>(Read<void*>(state.controller,0xc30),0x68),state.toggleEvents,reinterpret_cast<uintptr_t>(Read<void*>(functions.toggle,0xe0))-engine::image,state.coordinatorOpenBefore?"true":"false",state.buildProbe?"true":"false");
    std::fprintf(f,",\"missing_function_expected\":%u,\"missing_function_actual\":%u,\"can_open_policy_queries\":%u,\"shop_can_open\":%u,\"controller_ui_coordinator\":\"%p\",\"policy_checks\":%u,\"open_prerequisites\":%u,\"native_can_open\":%s,\"report_window_open\":%s,\"close_requests\":%u,\"close_completions\":%u,\"original_close_only_leaf_used\":%s",missingFunctionExpected,missingFunctionActual,state.canOpenPolicyQueries,static_cast<unsigned>(Read<uint8_t>(state.coordinator,0xc8)),Read<void*>(state.controller,0xc30),state.policyChecks,state.openPrerequisites,state.nativeCanOpen?"true":"false",state.reportWindowOpen?"true":"false",state.closeRequests,state.closeCompletions,state.originalCloseOnlyLeafUsed?"true":"false");
    std::fprintf(f,",\"owned_refresh_pending\":%s,\"owned_source_verified\":%s,\"owned_leaf_verified\":%s,\"model_refresh_passes\":%u,\"model_refresh_calls\":%u,\"owned_changes\":%u,\"owned_refresh_deferred\":%u,\"sampled_owned_models\":%u,\"stable_refresh_generation\":%u,\"stable_refresh_completed_sequence\":%u,\"stable_refresh_inventory\":\"%p\",\"owned_source_pawn\":\"%p\",\"owned_source_inventory\":\"%p\",\"purchase_component_player_info\":\"%p\",\"direct_owned_writes\":false}",state.ownedRefreshPending?"true":"false",state.ownedSourceVerified?"true":"false",state.ownedLeafVerified?"true":"false",state.modelRefreshPasses,state.modelRefreshCalls,state.ownedChanges,state.ownedRefreshDeferred,state.sampledOwnedModels,state.lastInventoryGeneration,state.lastCompletedSequence,state.lastModelInventory,state.ownedSourcePawn,state.ownedSourceInventory,state.purchaseComponentPlayerInfo);
    const bool ok = std::ferror(f) == 0; const bool closed = std::fclose(f) == 0;
    return ok && closed && MoveFileExW(temporary, path, MOVEFILE_REPLACE_EXISTING);
}
} // namespace original_buy_menu
