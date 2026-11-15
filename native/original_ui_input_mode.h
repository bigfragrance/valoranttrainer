#pragma once
#include "original_players.h"
#include "original_hud.h"
#include "original_inventory_slots.h"
#include "original_input_bindings.h"
#include <cstdio>
#include <cwchar>
#include <cstring>

// Original UI input-mode lifecycle, driven by actual native menu/map state.
// Original menus and original viewport modes. Physical dismiss edges are checked
// while gameplay is blocked; no replacement UI/cursor or camera code is used.
namespace original_ui_input_mode {
struct DetailedWidget {
    void* widget=nullptr;void* owningController=nullptr;
    char name[128]{},type[128]{};
    bool button=false,enabled=false,hovered=false,captured=false,focused=false,focusedChild=false;
    bool ownerQueried=false,enabledQueried=false,visibilityQueried=false;
    uint8_t visibility=0xff;
};
struct Diagnostics {
    const char* stage = "original_ui_input_mode_not_initialized";
    unsigned error = 0;
    bool initialized = false, localBindingsValid = false, restartRequired = false;
    bool optionsKnown = false, optionsOpen = false, mapKnown = false, mapOpen = false;
    bool mapVisibilityQueried = false, mapVisible = false;
    bool blockGameplay = false, desiredUIOnly = false, appliedUIOnly = false;
    bool appliedModeKnown = false, enteredUI = false, leftUI = false;
    bool cursorVisible = false, cursorReadbackMatchesLastEdge = false;
    bool reflectedCursorSetterFound = false, localCursorPropertyFallbackUsed = false;
    bool focusDispatched = false;
    uint8_t nativeAresMode = 0xff, defaultMouseCursor = 0xff, currentMouseCursor = 0xff;
    uint32_t cursorFlags = 0, cursorFlagsBeforeLastEdge = 0, cursorFlagsAfterLastEdge = 0;
    uint32_t updateCalls = 0, ownerChanges = 0, modeEdges = 0;
    uint32_t originalModeDispatches = 0, reflectedCursorSetterDispatches = 0;
    uint32_t localCursorPropertyEdgeWrites = 0, originalFocusDispatches = 0;
    int32_t openedMenuCount = 0;
    void* controller = nullptr;
    void* localPlayer = nullptr;
    void* pawn = nullptr;
    void* hud = nullptr;
    void* coordinator = nullptr;
    void* menuStack = nullptr;
    void* optionsCoordinator = nullptr;
    void* optionsEntry = nullptr;
    void* optionsWidget = nullptr;
    void* mapCoordinator = nullptr;
    void* mapWidget = nullptr;
    void* lastFocusedWidget = nullptr;
    bool shopKnown = false, shopOpen = false, externalObservedOpen = false;
    bool gameAndUIApplied = false, focusTargetChanged = false;
    bool widgetHasFocus = false, widgetHasFocusedDescendants = false;
    bool physicalEscapeHeld = false, physicalBuyHeld = false;
    bool closeRequested = false, closeObserved = false;
    void* shopCoordinator = nullptr; void* shopWidget = nullptr;
    uint32_t viewportModeDispatches = 0, focusTargetChanges = 0;
    uint32_t escapeEdges = 0, buyEdges = 0, dismissFallbacks = 0, nativeDismissesObserved = 0;
    uint32_t closeRequests = 0, closeCompletions = 0, originalOptionsExits = 0, originalShopCloses = 0;
    uint32_t originalContentCloses = 0;
    uint32_t pawnModeInvalidations = 0, modeDriftReapplies = 0;
    bool modeReadbackDrift = false;
    bool menuHovered = false, menuMouseCaptured = false, foregroundOwned = false;
    bool primaryButtonDown = false, secondaryButtonDown = false;
    uint32_t interactionSamples = 0, hoveredSamples = 0, mouseCaptureSamples = 0, focusPreservedRepairs = 0;
    ULONGLONG lastInteractionSample = 0;
    bool pendingPhysicalDismiss=false;void* pendingDismissWidget=nullptr;
    uint8_t pendingDismissKey=0;ULONGLONG dismissReleasedAt=0;
    uint32_t delayedDismisses=0,dismissCancelled=0;
    bool detailedProbe=false,detailedProbeTruncated=false;
    bool detailedProfileChecked=false,detailedProfileValid=false;
    int32_t detailedTreeOffset=-1,detailedRootOffset=-1;
    bool inputLockApiVerified=false,inputLockScopeActive=false;
    uint8_t inputLockBefore=0xff,inputLockAfterAcquire=0xff,inputLockAfterRestore=0xff,inputLockNow=0xff;
    void* inputLockController=nullptr;
    original_inventory_slots::WeakIdentity inputLockIdentity{};
    uint32_t inputLockAcquisitions=0,inputLockRestores=0,inputLockReassertions=0,inputLockStaleRetires=0,inputLockApiCalls=0;
    uint32_t detailedSamples=0,detailedVisited=0,detailedCount=0;
    ULONGLONG lastDetailedSample=0;
    DetailedWidget detailedWidgets[64]{};
    uintptr_t exceptionRva = 0;
};
inline Diagnostics state{};
struct Functions {
    bool resolved = false;
    void* setAresInputMode = nullptr;
    void* setShowMouseCursor = nullptr;
    void* setUserFocus = nullptr;
    void* setFocusToGameViewport = nullptr;
    void* isVisible = nullptr;
    void* gameAndUI = nullptr; void* gameOnly = nullptr;
    void* hasUserFocus = nullptr; void* hasFocusedDescendants = nullptr;
    void* optionsExit = nullptr; void* closeContent = nullptr; void* requestPop = nullptr;
    void* isHovered = nullptr;void* hasMouseCapture = nullptr;
    void* uiOnly=nullptr;void* getIsEnabled=nullptr;void* getVisibility=nullptr;
    void* childCount=nullptr;void* childAt=nullptr;void* getOwningPlayer=nullptr;
    void* setLockInputMode=nullptr;
};
inline Functions functions{};
inline bool externalMenuOpen = false;
inline void* externalMenuWidget = nullptr;
inline void SetExternalMenu(bool open,void* widget=nullptr) { externalMenuOpen=open;externalMenuWidget=widget; }
inline const Diagnostics& State() { return state; }
inline bool BlocksGameplay() { return state.blockGameplay; }
inline void SetDetailedProbe(bool enabled) {
    if(state.detailedProbe==enabled)return;
    state.detailedProbe=enabled;if(enabled)state.lastDetailedSample=0;
}
template<class T> inline T Read(const void* object, size_t offset) {
    return original_players::Value<T>(object, offset);
}
inline bool IsA(void* object, uintptr_t getter) { return original_players::IsA(object, getter); }
inline bool Fail(unsigned error, const char* stage) {
    state.error = error; state.stage = stage; return false;
}
inline void* Function(const wchar_t* path, uint16_t bytes) {
    void* function = engine::Find(path);
    return function && Read<uint16_t>(function, 0xbe) == bytes ? function : nullptr;
}
inline bool Dispatch(void* object, void* function, void* parameters) {
    if (!object || !function) return false;
    void* table = Read<void*>(object, 0);
    void* processEvent = Read<void*>(table, 0x298);
    if (!processEvent) return false;
    reinterpret_cast<void(*)(void*, void*, void*)>(processEvent)(object, function, parameters);
    return true;
}
inline bool ResolveFunctions() {
    if (functions.resolved) return functions.setAresInputMode && functions.uiOnly && functions.gameOnly && functions.closeContent;
    functions.resolved = true;
    // Descriptor d528070, AresPlayerController, one native bool parameter.
    // Original body781dbd0 invokes the real local-player input router, or
    // the original PC.SetInputMode virtual+dd0 (override781d900).
    functions.setAresInputMode = Function(L"/Script/ShooterGame.AresPlayerController.SetAresInputMode", 1);
    functions.setShowMouseCursor = Function(L"/Script/Engine.PlayerController.SetShowMouseCursor", 1);
    if (!functions.setShowMouseCursor) functions.setShowMouseCursor = Function(
        L"/Script/ShooterGame.AresPlayerController.SetShowMouseCursor", 1);
    state.reflectedCursorSetterFound = functions.setShowMouseCursor != nullptr;
    functions.setUserFocus = Function(L"/Script/UMG.Widget.SetUserFocus", 8);
    functions.setFocusToGameViewport = Function(L"/Script/UMG.WidgetBlueprintLibrary.SetFocusToGameViewport", 0);
    functions.isVisible = Function(L"/Script/UMG.Widget.IsVisible", 1);
    // These are runtime parameter ENDS, not padded generated C++ sizeof.
    functions.gameAndUI = Function(L"/Script/UMG.WidgetBlueprintLibrary.SetInputMode_GameAndUIEx", 19);
    functions.gameOnly = Function(L"/Script/UMG.WidgetBlueprintLibrary.SetInputMode_GameOnly", 9);
    functions.hasUserFocus = Function(L"/Script/UMG.Widget.HasUserFocus", 9);
    functions.hasFocusedDescendants = Function(L"/Script/UMG.Widget.HasUserFocusedDescendants", 9);
    functions.optionsExit = Function(L"/Game/UI/InGame/Options/OptionsScreen.OptionsScreen_C.Exit", 0);
    functions.closeContent = Function(L"/Script/ShooterGame.MenuStackManager.Close", 8);
    functions.requestPop = Function(L"/Script/ShooterGame.MenuStackManager.RequestPop", 1);
    functions.isHovered = Function(L"/Script/UMG.Widget.IsHovered", 1);
    functions.hasMouseCapture = Function(L"/Script/UMG.Widget.HasMouseCapture", 1);
    functions.uiOnly=Function(L"/Script/UMG.WidgetBlueprintLibrary.SetInputMode_UIOnlyEx",18);
    functions.getIsEnabled=Function(L"/Script/UMG.Widget.GetIsEnabled",1);
    functions.getVisibility=Function(L"/Script/UMG.Widget.GetVisibility",1);
    functions.childCount=Function(L"/Script/UMG.PanelWidget.GetChildrenCount",4);
    functions.childAt=Function(L"/Script/UMG.PanelWidget.GetChildAt",16);
    functions.getOwningPlayer=Function(L"/Script/UMG.Widget.GetOwningPlayer",8);
    functions.setLockInputMode=Function(L"/Script/ShooterGame.AresPlayerController.SetLockInputMode",1);
    state.inputLockApiVerified=functions.setLockInputMode
        &&Read<void*>(functions.setLockInputMode,0x28)==engine::Class(0x6427aa0)
        &&Read<uintptr_t>(functions.setLockInputMode,0xe0)==engine::image+0x64ea070;
    if(!state.inputLockApiVerified)functions.setLockInputMode=nullptr;
    return functions.setAresInputMode && functions.uiOnly && functions.gameOnly && functions.closeContent;
}
struct GameAndUIParameters {
    void* controller = nullptr; void* focus = nullptr;
    uint8_t mouseLock = 0, hideCursorDuringCapture = 0, flush = 0, padding[5]{};
};
struct GameOnlyParameters { void* controller = nullptr; uint8_t flush = 1, padding[7]{}; };
struct FocusQuery { void* controller = nullptr; uint8_t result = 0, padding[7]{}; };
struct UIOnlyParameters {void* controller=nullptr;void* focus=nullptr;uint8_t mouseLock=0,flush=0,padding[6]{};};
static_assert(sizeof(UIOnlyParameters)==24&&offsetof(UIOnlyParameters,flush)+1==18);
inline bool ReadActualOptions();
inline bool ReadActualMegamap();
static_assert(sizeof(GameAndUIParameters) == 24 && offsetof(GameAndUIParameters, flush) + 1 == 19);
static_assert(sizeof(GameOnlyParameters) == 16 && offsetof(GameOnlyParameters, flush) + 1 == 9);
inline bool ContentOpen(void* widget) {
    auto data=Read<void**>(state.menuStack,0xb0);const auto count=Read<int32_t>(state.menuStack,0xb8),cap=Read<int32_t>(state.menuStack,0xbc);
    if(!widget||count<0||count>128||cap<count||(count&&!original_players::Readable(data,size_t(count)*8)))return false;
    for(int32_t i=0;i<count;++i)if(Read<void*>(data[i],0x30)==widget)return true;
    return false;
}
inline bool ReadActualShop() {
    state.shopKnown=false;state.shopOpen=false;state.shopWidget=nullptr;
    state.shopCoordinator=Read<void*>(state.coordinator,0x68);
    if(!IsA(state.shopCoordinator,0x6cd95b0)||Read<void*>(state.shopCoordinator,0x80)!=state.controller
       ||Read<void*>(state.shopCoordinator,0x78)!=state.menuStack)return false;
    auto data=Read<void**>(state.shopCoordinator,0x88);const auto count=Read<int32_t>(state.shopCoordinator,0x90),cap=Read<int32_t>(state.shopCoordinator,0x94);
    if(count<0||count>32||cap<count||(count&&!original_players::Readable(data,size_t(count)*8)))return false;
    for(int32_t i=0;i<count;++i){auto widget=Read<void*>(data[i],0x30);if(!state.shopWidget)state.shopWidget=widget;
        if(ContentOpen(widget)){state.shopOpen=true;state.shopWidget=widget;}}
    state.shopKnown=true;return true;
}
inline bool ExternalOpen() {
    // The root may notify the old shop state before this frame's dismiss. Prefer
    // its actual menu-stack membership when that known original shop is present.
    return externalMenuOpen && (!state.shopKnown || externalMenuWidget!=state.shopWidget);
}
inline void* FocusWidget() {
    return state.optionsOpen?state.optionsWidget:state.shopOpen?state.shopWidget:
        state.mapOpen?state.mapWidget:externalMenuWidget;
}
inline bool DetailedObjectField(void* type,const wchar_t* name,int32_t expected,int32_t& observed) {
    struct Name {uint32_t comparison=0,display=0,number=0;} wanted;
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&wanted,name,1);
    auto field=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,&wanted,3);
    auto fieldClass=Read<void*>(field,8);const auto flags=Read<uint64_t>(fieldClass,0x18);
    observed=field?Read<int32_t>(field,0x44):-1;
    return field&&(flags&0x8001)==0x8001&&Read<uint64_t>(field,0x20)==Read<uint64_t>(&wanted,0)
        &&Read<int32_t>(field,0x30)==1&&Read<int32_t>(field,0x34)==8&&observed==expected
        &&Read<int32_t>(type,0x60)>=observed+8;
}
inline bool ResolveDetailedProfile() {
    if(state.detailedProfileChecked)return state.detailedProfileValid;
    state.detailedProfileChecked=true;
    auto widget=engine::Class(0x3b0f3c0),user=engine::Class(0x3b0f5b0);
    auto panel=engine::Class(0x3b0fae0),tree=engine::Class(0x3b6ebc0);
    state.detailedProfileValid=widget==engine::Find(L"/Script/UMG.Widget")
        &&user==engine::Find(L"/Script/UMG.UserWidget")
        &&panel==engine::Find(L"/Script/UMG.PanelWidget")
        &&tree==engine::Find(L"/Script/UMG.WidgetTree")
        &&DetailedObjectField(user,L"WidgetTree",0x298,state.detailedTreeOffset)
        &&DetailedObjectField(tree,L"RootWidget",0x38,state.detailedRootOffset)
        &&functions.getOwningPlayer&&Read<void*>(functions.getOwningPlayer,0x28)==widget
        &&Read<uintptr_t>(functions.getOwningPlayer,0xe0)==engine::image+0x3bbeb60;
    return state.detailedProfileValid;
}
inline void SampleDetailedWidgets() {
    if(!state.detailedProbe||!state.desiredUIOnly||!functions.childCount||!functions.childAt)return;
    const auto now=GetTickCount64();if(now-state.lastDetailedSample<500)return;
    state.lastDetailedSample=now;++state.detailedSamples;state.detailedCount=state.detailedVisited=0;
    state.detailedProbeTruncated=false;
    if(!ResolveDetailedProfile())return;
    void* queue[256]{};uint32_t count=0;
    const auto add=[&](void* object){if(!object)return;
        for(uint32_t i=0;i<count;++i)if(queue[i]==object)return;
        if(count<256)queue[count++]=object;else state.detailedProbeTruncated=true;};
    add(FocusWidget());
    auto menus=Read<void**>(state.menuStack,0xb0);const auto menuCount=Read<int32_t>(state.menuStack,0xb8);
    if(menuCount>=0&&menuCount<=16&&(!menuCount||original_players::Readable(menus,size_t(menuCount)*8)))
        for(int i=0;i<menuCount;++i)add(Read<void*>(menus[i],0x30));
    for(uint32_t at=0;at<count&&at<256;++at){auto widget=queue[at];++state.detailedVisited;
        if(!IsA(widget,0x3b0f3c0))continue;
        uint8_t hovered=0,captured=0,enabled=0,visibility=0xff;
        if(functions.isHovered)Dispatch(widget,functions.isHovered,&hovered);
        if(functions.hasMouseCapture)Dispatch(widget,functions.hasMouseCapture,&captured);
        const bool enabledQueried=functions.getIsEnabled&&Dispatch(widget,functions.getIsEnabled,&enabled);
        const bool visibilityQueried=functions.getVisibility&&Dispatch(widget,functions.getVisibility,&visibility);
        const bool user=IsA(widget,0x3b0f5b0),button=IsA(widget,0x3b31f50);
        if(state.detailedCount>=64&&(button||hovered||captured))state.detailedProbeTruncated=true;
        if(state.detailedCount<64&&(button||hovered||captured||at==0)){
            auto& o=state.detailedWidgets[state.detailedCount++];o={};o.widget=widget;o.button=button;
            o.enabled=enabled!=0;o.hovered=hovered!=0;o.captured=captured!=0;o.visibility=visibility;
            o.enabledQueried=enabledQueried;o.visibilityQueried=visibilityQueried;
            original_hud::OriginalObjectName(widget,o.name);original_hud::OriginalObjectName(Read<void*>(widget,0x10),o.type);
            FocusQuery focus{state.controller},children{state.controller};
            if(functions.hasUserFocus&&Dispatch(widget,functions.hasUserFocus,&focus))o.focused=focus.result!=0;
            if(functions.hasFocusedDescendants&&Dispatch(widget,functions.hasFocusedDescendants,&children))o.focusedChild=children.result!=0;
            if(functions.getOwningPlayer)o.ownerQueried=Dispatch(widget,functions.getOwningPlayer,&o.owningController);
        }
        if(user){auto tree=Read<void*>(widget,state.detailedTreeOffset);
            if(IsA(tree,0x3b6ebc0))add(Read<void*>(tree,state.detailedRootOffset));}
        if(IsA(widget,0x3b0fae0)){int32_t children=0;
            if(Dispatch(widget,functions.childCount,&children)&&children>=0&&children<=256)
                for(int32_t i=0;i<children;++i){struct Child{int32_t index,pad;void* result;} child{i,0,nullptr};
                    if(Dispatch(widget,functions.childAt,&child))add(child.result);}
        }
    }
}
inline bool CloseCurrentMenu() {
    state.closeRequested=true;state.closeObserved=false;++state.closeRequests;
    const auto closedContent=state.optionsOpen?state.optionsWidget:state.shopOpen?state.shopWidget:
        state.mapOpen?state.mapWidget:externalMenuWidget;
    const bool closingMap=!state.optionsOpen&&!state.shopOpen&&state.mapOpen;
    if(state.optionsOpen){
        if(!original_input_bindings::EnsureSettingsCommitContext(original_hud::State().clientInstance))
            return Fail(original_input_bindings::State().error,original_input_bindings::State().stage);
        auto owner=Read<void*>(functions.optionsExit,0x28);
        if(functions.optionsExit&&original_hud::Derived(Read<void*>(state.optionsWidget,0x10),owner)){
            if(!Dispatch(state.optionsWidget,functions.optionsExit,nullptr))return false;++state.originalOptionsExits;
        }else{void* content=state.optionsWidget;if(!Dispatch(state.menuStack,functions.closeContent,&content))return false;++state.originalContentCloses;}
    }else if(state.shopOpen){
        auto iface=static_cast<uint8_t*>(state.shopCoordinator)+0x30;
        auto table=Read<void**>(iface,0);
        if(table!=reinterpret_cast<void**>(engine::image+0xb523350)
           ||Read<void*>(table,0x38)!=reinterpret_cast<void*>(engine::image+0x7a8df10))return false;
        // Same original close-only interface leaf as ToggleShop's open branch:
        // closes every original shop entry AND recomputes native stack/input owner.
        engine::function<void(*)(void*)>(0x7a8df10)(iface);++state.originalShopCloses;
    }else if(ExternalOpen()&&ContentOpen(externalMenuWidget)){
        void* content=externalMenuWidget;if(!Dispatch(state.menuStack,functions.closeContent,&content))return false;
        ++state.originalContentCloses;
    }else if(state.mapOpen&&functions.requestPop){
        uint8_t popped=0;if(!Dispatch(state.menuStack,functions.requestPop,&popped))return false;
        // A widget may intercept original RequestCloseMenu. Never force-pop it.
    }else{state.closeObserved=true;return true;}
    ReadActualOptions();ReadActualShop();ReadActualMegamap();
    state.closeObserved=closingMap?!state.mapOpen:!ContentOpen(closedContent);
    if(state.closeObserved)++state.closeCompletions;
    return true;
}
inline bool ReadActualOptions() {
    state.optionsKnown = false;
    state.optionsCoordinator = Read<void*>(state.coordinator, 0x78);
    if (!IsA(state.optionsCoordinator, 0x69f41a0)
        || Read<void*>(state.optionsCoordinator, 0x68) != state.coordinator
        || Read<void*>(state.optionsCoordinator, 0x70) != state.controller
        || Read<void*>(state.optionsCoordinator, 0x78) != state.menuStack) return false;
    state.optionsEntry = Read<void*>(state.optionsCoordinator, 0x80);
    state.optionsWidget = Read<void*>(state.optionsEntry, 0x30);
    if (!IsA(state.optionsEntry, 0x699c5b0) || !IsA(state.optionsWidget, 0x3b0f5b0)) return false;
    void* entries = Read<void*>(state.menuStack, 0xb0);
    const int32_t count = Read<int32_t>(state.menuStack, 0xb8);
    const int32_t capacity = Read<int32_t>(state.menuStack, 0xbc);
    state.openedMenuCount = count;
    if (count < 0 || count > 128 || capacity < count || (count && !entries)) return false;
    bool open = false;
    for (int32_t n = 0; n < count; ++n)
        if (Read<void*>(entries, static_cast<size_t>(n) * sizeof(void*)) == state.optionsEntry) { open = true; break; }
    state.optionsOpen = open;
    state.optionsKnown = true;
    return true;
}
inline bool ReadActualMegamap() {
    state.mapKnown = false;
    state.mapVisibilityQueried = false;
    state.mapCoordinator = Read<void*>(state.coordinator, 0x88);
    if (!IsA(state.mapCoordinator, 0x698fd70)
        || Read<void*>(state.mapCoordinator, 0x58) != state.controller
        || Read<void*>(state.mapCoordinator, 0x68) != state.menuStack) return false;
    state.mapWidget = Read<void*>(state.mapCoordinator, 0x70);
    if (!IsA(state.mapWidget, 0x698fcc0)) return false;
    uint8_t open = 0;
    if (!original_players::Read(state.mapWidget, 0x328, open) || open > 1) return false;
    // Native Megamap.bIsOpen, exact generated bool setter writes+328.
    // Visibility is a separate observation, not an inferred hotkey state.
    state.mapOpen = open != 0;
    state.mapKnown = true;
    return true;
}
inline void ReadNativeCursor() {
    state.cursorFlags = Read<uint32_t>(state.controller, 0x73c);
    state.cursorVisible = (state.cursorFlags & 1U) != 0;
    state.defaultMouseCursor = Read<uint8_t>(state.controller, 0x770);
    state.currentMouseCursor = Read<uint8_t>(state.controller, 0x771);
    state.nativeAresMode = Read<uint8_t>(state.controller, 0xd61);
    state.inputLockNow=Read<uint8_t>(state.controller,0xbe8);
    if (state.appliedModeKnown)
        state.cursorReadbackMatchesLastEdge = state.cursorVisible == state.appliedUIOnly;
}
inline bool RestoreMenuInputLock() {
    if(!state.inputLockScopeActive)return true;
    auto controller=original_inventory_slots::ResolveIdentity(state.inputLockIdentity);
    if(!controller||controller!=state.inputLockController){
        // A retired PC may already be pending-kill. Never read or restore its
        // old raw allocation, even when that address has become readable again.
        state.inputLockScopeActive=false;++state.inputLockStaleRetires;return true;
    }
    if(!functions.setLockInputMode||state.inputLockBefore>1)
        return Fail(3131,"original_ui_input_original_lock_restore_profile_missing");
    uint8_t locked=state.inputLockBefore;
    if(!Dispatch(controller,functions.setLockInputMode,&locked))
        return Fail(3132,"original_ui_input_original_lock_restore_dispatch_failed");
    ++state.inputLockApiCalls;
    state.inputLockAfterRestore=Read<uint8_t>(controller,0xbe8);
    if(state.inputLockAfterRestore!=state.inputLockBefore)
        return Fail(3133,"original_ui_input_original_lock_restore_readback_mismatch");
    state.inputLockScopeActive=false;++state.inputLockRestores;return true;
}
inline bool AcquireMenuInputLock() {
    if(state.inputLockScopeActive&&state.inputLockController!=state.controller
        &&!RestoreMenuInputLock())return false;
    const bool entering=!state.inputLockScopeActive;
    if(entering){
        state.inputLockBefore=Read<uint8_t>(state.controller,0xbe8);
        if(state.inputLockBefore>1||!original_inventory_slots::CaptureIdentity(state.controller,state.inputLockIdentity)
           ||original_inventory_slots::ResolveIdentity(state.inputLockIdentity)!=state.controller)
            return Fail(3134,"original_ui_input_original_lock_controller_identity_invalid");
        state.inputLockController=state.controller;state.inputLockAfterRestore=0xff;
    }
    if(!functions.setLockInputMode)return Fail(3135,"original_ui_input_original_set_lock_input_mode_missing");
    // Only original menu entry or an already-required mode repair reaches here.
    // Do not recapture the pre-menu value or write this lock on every UI tick.
    if(entering||Read<uint8_t>(state.controller,0xbe8)!=1){
        uint8_t locked=1;
        if(!Dispatch(state.controller,functions.setLockInputMode,&locked))
            return Fail(3136,"original_ui_input_original_lock_acquire_dispatch_failed");
        ++state.inputLockApiCalls;
        state.inputLockAfterAcquire=Read<uint8_t>(state.controller,0xbe8);
        if(state.inputLockAfterAcquire!=1)return Fail(3137,"original_ui_input_original_lock_acquire_readback_mismatch");
        if(entering)++state.inputLockAcquisitions;else ++state.inputLockReassertions;
    }
    state.inputLockScopeActive=true;return true;
}
inline bool ApplyEdge(bool uiOnly, bool preserveChildFocus = false) {
    uint8_t value = uiOnly ? 1 : 0;
    state.stage = uiOnly ? "original_ui_input_entering_original_ui_only" : "original_ui_input_restoring_original_game_only";
    state.cursorFlagsBeforeLastEdge = Read<uint32_t>(state.controller, 0x73c);
    state.focusDispatched = false;
    if(uiOnly){if(!AcquireMenuInputLock())return false;}
    else if(!RestoreMenuInputLock())return false;
    if (!Dispatch(state.controller, functions.setAresInputMode, &value))
        return Fail(3121, "original_ui_input_original_mode_dispatch_failed");
    ++state.originalModeDispatches;
    // SetAresInputMode may return early when its CommonUI local router is absent.
    // Complete the ORIGINAL viewport mode through UMG; cursor bit alone is not
    // proof of released capture. Match the original Ares UI-only policy; physical
    // dismiss fallback below waits until key release and native dispatch settle.
    auto library=engine::DefaultObject(engine::Class(0x3b1d050));
    if(uiOnly){UIOnlyParameters parameters{state.controller,preserveChildFocus?nullptr:FocusWidget()};
        if(!Dispatch(library,functions.uiOnly,&parameters))return Fail(3128,"original_ui_input_ui_only_viewport_dispatch_failed");
    }else{GameOnlyParameters parameters{state.controller};
        if(!Dispatch(library,functions.gameOnly,&parameters))return Fail(3129,"original_ui_input_game_only_viewport_dispatch_failed");}
    ++state.viewportModeDispatches;state.gameAndUIApplied=false;
    if (functions.setShowMouseCursor) {
        if (!Dispatch(state.controller, functions.setShowMouseCursor, &value))
            return Fail(3122, "original_ui_input_original_cursor_setter_dispatch_failed");
        ++state.reflectedCursorSetterDispatches;
    }
    const uint32_t flags = Read<uint32_t>(state.controller, 0x73c);
    if (((flags & 1U) != 0) != uiOnly) {
        // This pinned client has a reflected bShowMouseCursor property, but no
        // native reflected SetShowMouseCursor function was found statically.
        // Preserve every other packed property bit. This is ONE local UI bit
        // assignment on a real open/close/bootstrap edge, never a per-frame force.
        const uint32_t wanted = (flags & ~1U) | (uiOnly ? 1U : 0U);
        if (!original_players::Write(state.controller, 0x73c, wanted))
            return Fail(3123, "original_ui_input_local_cursor_property_edge_write_failed");
        state.localCursorPropertyFallbackUsed = true;
        ++state.localCursorPropertyEdgeWrites;
    }
    if (uiOnly && !preserveChildFocus) {
        void* widget = FocusWidget();
        if (IsA(widget, 0x3b0f3c0) && functions.setUserFocus) {
            void* playerController = state.controller;
            state.focusDispatched = Dispatch(widget, functions.setUserFocus, &playerController);
            if (state.focusDispatched) { ++state.originalFocusDispatches; state.lastFocusedWidget = widget; }
        }
    } else if (!uiOnly && functions.setFocusToGameViewport) {
        state.focusDispatched = Dispatch(library, functions.setFocusToGameViewport, nullptr);
        if (state.focusDispatched) { ++state.originalFocusDispatches; state.lastFocusedWidget = nullptr; }
    }
    if(uiOnly&&preserveChildFocus)++state.focusPreservedRepairs;
    state.appliedUIOnly = uiOnly;
    state.appliedModeKnown = true;
    ++state.modeEdges;
    state.cursorFlagsAfterLastEdge = Read<uint32_t>(state.controller, 0x73c);
    ReadNativeCursor();
    return state.cursorVisible == uiOnly
        ? true : Fail(3124, "original_ui_input_cursor_readback_did_not_match_edge");
}
inline bool UpdateImpl(bool pollPhysical = true) {
    if (state.restartRequired) return false;
    if (!engine::image || !original_players::gameThread
        || original_players::gameThread != GetCurrentThreadId())
        return Fail(3120, "original_ui_input_wrong_game_thread");
    ++state.updateCalls;
    state.enteredUI = false; state.leftUI = false;
    state.localBindingsValid = false;
    void* pc = original_players::activeController;
    void* pawn = Read<void*>(pc,0x4b0);
    void* player = Read<void*>(pc, 0x510);
    if (!IsA(pc, 0x6943db0) || !IsA(player, 0x6cb4740)
        || player != original_players::localPlayer || Read<void*>(player, 0x38) != pc)
        return Fail(3125, "original_ui_input_real_local_player_possession_mismatch");
    const bool previouslyOpen=state.optionsOpen||state.shopOpen||state.mapOpen||state.externalObservedOpen;
    const bool previouslyShopOpen=state.shopOpen&&!state.optionsOpen;
    if (state.controller != pc || state.localPlayer != player) {
        if(!RestoreMenuInputLock())return false;
        state.controller = pc; state.localPlayer = player; state.pawn = pawn;
        state.initialized = false; state.appliedModeKnown = false; state.appliedUIOnly = false;
        state.optionsKnown = false; state.optionsOpen = false;
        state.mapKnown = false; state.mapOpen = false;
        ++state.ownerChanges;
    }
    if(state.pawn!=pawn){
        // Original ClientRestart on the same controller can reset its viewport
        // mode/cursor. Invalidate the cached application, without requiring an
        // alive Pawn or closing any actual menu during death/respawn.
        state.appliedModeKnown=false;++state.pawnModeInvalidations;
    }
    state.pawn=pawn; // UI remains operable while its real controller has no alive Pawn.
    state.localBindingsValid = true;
    state.hud = Read<void*>(pc, 0x520);
    state.coordinator = Read<void*>(state.hud, 0x760);
    state.menuStack = Read<void*>(original_hud::State().clientInstance, 0x580);
    if (!IsA(state.hud, 0x6cae7d0) || !IsA(state.coordinator, 0x6cd6490)
        || !IsA(state.menuStack, 0x699c660)) {
        state.stage = "original_ui_input_waiting_original_hud_coordinator";
        state.blockGameplay = state.appliedUIOnly || state.optionsOpen || state.mapOpen || externalMenuOpen;
        return true;
    }
    if (!ResolveFunctions()) return Fail(3126, "original_ui_input_original_set_ares_input_mode_missing");
    ReadActualOptions();
    ReadActualMegamap();
    ReadActualShop();state.externalObservedOpen=ExternalOpen();
    if(pollPhysical){
        DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
        const bool focused=foreground==GetCurrentProcessId();
        const bool escape=focused&&(GetAsyncKeyState(VK_ESCAPE)&0x8000)!=0;
        const bool buy=focused&&(GetAsyncKeyState('B')&0x8000)!=0;
        const bool escapeEdge=escape&&!state.physicalEscapeHeld,buyEdge=buy&&!state.physicalBuyHeld;
        state.physicalEscapeHeld=escape;state.physicalBuyHeld=buy;
        if(escapeEdge)++state.escapeEdges;if(buyEdge)++state.buyEdges;
        const bool dismiss=(escapeEdge&&previouslyOpen)||(buyEdge&&previouslyShopOpen);
        if(dismiss){
            state.pendingPhysicalDismiss=true;state.pendingDismissWidget=FocusWidget();
            state.pendingDismissKey=escapeEdge?uint8_t(VK_ESCAPE):uint8_t('B');state.dismissReleasedAt=0;
        }
        if(state.pendingPhysicalDismiss&&!focused){state.pendingPhysicalDismiss=false;++state.dismissCancelled;}
        if(state.pendingPhysicalDismiss){
            const bool stillOpen=state.optionsOpen||state.shopOpen||state.mapOpen||state.externalObservedOpen;
            if(!stillOpen||FocusWidget()!=state.pendingDismissWidget){
                state.pendingPhysicalDismiss=false;++state.nativeDismissesObserved;++state.dismissCancelled;
            }else{
                const bool held=state.pendingDismissKey==VK_ESCAPE?escape:buy;
                const auto now=GetTickCount64();
                if(!held&&!state.dismissReleasedAt)state.dismissReleasedAt=now;
                if(state.dismissReleasedAt&&now-state.dismissReleasedAt>=100){
                    state.pendingPhysicalDismiss=false;
                    if(!CloseCurrentMenu())return Fail(3130,"original_ui_input_original_menu_dismiss_failed");
                    ++state.dismissFallbacks;++state.delayedDismisses;
                }
            }
        }
    }
    state.externalObservedOpen=ExternalOpen();
    state.desiredUIOnly = state.optionsOpen || state.mapOpen || state.shopOpen || state.externalObservedOpen;
    state.blockGameplay = state.desiredUIOnly || (state.appliedModeKnown && state.appliedUIOnly);
    ReadNativeCursor();
    if (!state.optionsKnown || (!state.mapKnown && state.mapOpen)) {
        state.stage = "original_ui_input_state_unknown_preserving_previous_capture";
        return true;
    }
    state.focusTargetChanged=state.desiredUIOnly&&FocusWidget()!=state.lastFocusedWidget;
    // Entering UI from GameOnly is a normal edge, not evidence of NPC drift.
    state.modeReadbackDrift=state.appliedModeKnown&&state.appliedUIOnly&&state.desiredUIOnly&&!state.cursorVisible;
    if (!state.appliedModeKnown || state.desiredUIOnly != state.appliedUIOnly || state.focusTargetChanged || state.modeReadbackDrift) {
        const bool previous = state.appliedModeKnown && state.appliedUIOnly;
        const bool preserveFocus=state.desiredUIOnly&&state.appliedUIOnly&&!state.focusTargetChanged;
        if (!ApplyEdge(state.desiredUIOnly,preserveFocus)) return false;
        if(state.focusTargetChanged)++state.focusTargetChanges;
        if(state.modeReadbackDrift)++state.modeDriftReapplies;
        state.enteredUI = state.appliedUIOnly && !previous;
        state.leftUI = !state.appliedUIOnly && previous;
    }
    state.widgetHasFocus=false;state.widgetHasFocusedDescendants=false;
    if(state.desiredUIOnly&&FocusWidget()){
        FocusQuery focus{state.controller};if(functions.hasUserFocus&&Dispatch(FocusWidget(),functions.hasUserFocus,&focus))state.widgetHasFocus=focus.result!=0;
        FocusQuery children{state.controller};if(functions.hasFocusedDescendants&&Dispatch(FocusWidget(),functions.hasFocusedDescendants,&children))state.widgetHasFocusedDescendants=children.result!=0;
    }
    const auto sampleTime=GetTickCount64();
    if(state.desiredUIOnly&&sampleTime-state.lastInteractionSample>=200){
        state.lastInteractionSample=sampleTime;++state.interactionSamples;
        uint8_t hovered=0,captured=0;
        state.menuHovered=functions.isHovered&&Dispatch(FocusWidget(),functions.isHovered,&hovered)&&hovered!=0;
        state.menuMouseCaptured=functions.hasMouseCapture&&Dispatch(FocusWidget(),functions.hasMouseCapture,&captured)&&captured!=0;
        if(state.menuHovered)++state.hoveredSamples;if(state.menuMouseCaptured)++state.mouseCaptureSamples;
        DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);state.foregroundOwned=foreground==GetCurrentProcessId();
        state.primaryButtonDown=state.foregroundOwned&&(GetAsyncKeyState(VK_LBUTTON)&0x8000)!=0;
        state.secondaryButtonDown=state.foregroundOwned&&(GetAsyncKeyState(VK_RBUTTON)&0x8000)!=0;
    }
    SampleDetailedWidgets();
    if (state.mapKnown && state.enteredUI && state.mapOpen && functions.isVisible) {
        uint8_t visible = 0;
        state.mapVisibilityQueried = Dispatch(state.mapWidget, functions.isVisible, &visible);
        state.mapVisible = state.mapVisibilityQueried && visible != 0;
    }
    state.initialized = true;
    state.blockGameplay = state.optionsOpen || state.mapOpen || state.shopOpen || state.externalObservedOpen || state.appliedUIOnly || state.leftUI;
    state.error = 0;
    state.stage = state.blockGameplay ? "original_ui_input_ui_active_gameplay_blocked" : "original_ui_input_original_game_only_active";
    return true;
}
inline int CaptureException(EXCEPTION_POINTERS* exception) {
    if (exception && exception->ExceptionRecord) {
        const auto at = reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress);
        state.exceptionRva = at >= engine::image ? at - engine::image : at;
    }
    state.restartRequired = true;
    state.blockGameplay = true;
    Fail(3127, "original_ui_input_native_exception_requires_new_session");
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Update() {
    __try { return UpdateImpl(); }
    __except (CaptureException(GetExceptionInformation())) { return false; }
}
// The local-server client observes actual window messages. Keep its dismiss
// queue independent of the legacy trainer's asynchronous-key polling above.
inline bool UpdateFromWindowEvents() {
    __try { return UpdateImpl(false); }
    __except (CaptureException(GetExceptionInformation())) { return false; }
}
struct WindowSelectionDiagnostics {
    const char* stage="not_queried";
    uint64_t samples=0;
    uint32_t visited=0,queued=0,selectors=0,anchors=0,combos=0,metadataFailure=0;
    uint32_t skippedHidden=0,visibilityQueries=0;
    bool complete=false,selecting=false,schemaReady=false,profileReady=false;
    int32_t treeOffset=-1,rootOffset=-1,childIndex=-1;
    uintptr_t failedWidget=0,profileRva=0;
};
inline SRWLOCK windowSelectionLock=SRWLOCK_INIT;
inline WindowSelectionDiagnostics windowSelectionDiagnostics{};
inline constexpr size_t WindowSelectionMaximumWidgets=16384,WindowSelectionHashSlots=32768;
struct WindowSelectionScratch {
    bool busy=false;
    void* widgets[WindowSelectionMaximumWidgets]{};
    void* hashes[WindowSelectionHashSlots]{};
};
// Only the original game thread scans. Keep the bounded scratch off the window
// callback stack; a reentrant scan is reported unknown instead of sharing it.
inline WindowSelectionScratch windowSelectionScratch{};
inline void CacheWindowSelection(WindowSelectionDiagnostics observed) {
    AcquireSRWLockExclusive(&windowSelectionLock);
    observed.samples=windowSelectionDiagnostics.samples+1;windowSelectionDiagnostics=observed;
    ReleaseSRWLockExclusive(&windowSelectionLock);
}
inline WindowSelectionDiagnostics WindowSelectionSnapshot() {
    AcquireSRWLockShared(&windowSelectionLock);auto observed=windowSelectionDiagnostics;
    ReleaseSRWLockShared(&windowSelectionLock);return observed;
}
struct WindowDismissFunctions {
    bool verified=false;
    void* getTop=nullptr;void* handleDismiss=nullptr;
    bool selectingVerified=false;void* getSelecting=nullptr;
    void* anchorIsOpen=nullptr;void* comboIsOpen=nullptr;void* visibility=nullptr;
    int32_t treeOffset=-1,rootOffset=-1;
    uint64_t requests=0,staleRequests=0;
};
inline WindowDismissFunctions windowDismiss{};
inline bool ResolveWindowDismissFunctions() {
    if(windowDismiss.verified)return true;
    // Original native wrappers and the complete RequestPop implementation.
    // GetTop has a second RET for the empty-stack branch; include both paths.
    struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
    static constexpr Profile profiles[]{
        {0x69a00c0,62,0x427bba0dc947ca4aULL},
        {0x699d450,26,0x02e6ad79cd3884d8ULL},
        {0x7ab69d0,497,0x7dc63e75ee00842aULL}
    };
    for(const auto& profile:profiles){
        auto code=reinterpret_cast<const uint8_t*>(engine::image+profile.rva);
        if(!original_players::Readable(code,profile.bytes))return false;
        uint64_t hash=14695981039346656037ULL;
        for(size_t i=0;i<profile.bytes;++i)hash=(hash^code[i])*1099511628211ULL;
        if(hash!=profile.fnv)return false;
    }
    windowDismiss.getTop=Function(L"/Script/ShooterGame.MenuStackManager.GetTopOpenWidgetEntry",8);
    windowDismiss.handleDismiss=Function(L"/Script/ShooterGame.MenuStackManager.HandleDismissInput",0);
    auto owner=engine::Class(0x699c660);
    if(!windowDismiss.getTop||!windowDismiss.handleDismiss||!owner
        ||Read<void*>(windowDismiss.getTop,0x28)!=owner
        ||Read<void*>(windowDismiss.handleDismiss,0x28)!=owner
        ||(Read<uint32_t>(windowDismiss.getTop,0xb8)&0x400)==0
        ||(Read<uint32_t>(windowDismiss.handleDismiss,0xb8)&0x400)==0
        ||Read<uintptr_t>(windowDismiss.getTop,0xe0)!=engine::image+0x69a00c0
        ||Read<uintptr_t>(windowDismiss.handleDismiss,0xe0)!=engine::image+0x699d450)return false;
    windowDismiss.verified=true;return true;
}
inline bool ReadTopEntryImpl(void*& entry,void*& content) {
    entry=nullptr;content=nullptr;
    if(state.restartRequired)return false;
    if(!engine::image||!original_players::gameThread||original_players::gameThread!=GetCurrentThreadId())
        return Fail(3140,"original_ui_window_dismiss_wrong_game_thread");
    auto pc=original_players::activeController;
    auto player=Read<void*>(pc,0x510);
    if(state.controller!=pc||!state.localBindingsValid||!IsA(pc,0x6943db0)
        ||!IsA(player,0x6cb4740)||player!=original_players::localPlayer||Read<void*>(player,0x38)!=pc
        ||!IsA(state.menuStack,0x699c660)
        ||state.menuStack!=Read<void*>(original_hud::State().clientInstance,0x580))
        return Fail(3141,"original_ui_window_dismiss_actual_owner_or_stack_invalid");
    if(!ResolveWindowDismissFunctions())return Fail(3142,"original_ui_window_dismiss_original_api_profile_mismatch");
    auto entries=Read<void**>(state.menuStack,0xb0);
    const auto count=Read<int32_t>(state.menuStack,0xb8),capacity=Read<int32_t>(state.menuStack,0xbc);
    if(count<0||count>128||capacity<count||(count&&!original_players::Readable(entries,count*sizeof(void*))))
        return Fail(3143,"original_ui_window_dismiss_actual_stack_array_invalid");
    void* top=nullptr;
    if(!Dispatch(state.menuStack,windowDismiss.getTop,&top)
        ||top!=(count?Read<void*>(entries,(count-1)*sizeof(void*)):nullptr))
        return Fail(3144,"original_ui_window_dismiss_native_top_readback_mismatch");
    if(!top)return true;
    auto widget=Read<void*>(top,0x30);
    if(!IsA(top,0x699c5b0)||!IsA(widget,0x3b0f3c0))
        return Fail(3145,"original_ui_window_dismiss_top_entry_content_invalid");
    entry=top;content=widget;return true;
}
inline bool ReadTopEntry(void*& entry,void*& content) {
    __try { return ReadTopEntryImpl(entry,content); }
    __except (CaptureException(GetExceptionInformation())) { entry=nullptr;content=nullptr;return false; }
}
inline bool ResolveWindowSelectingFunctions(WindowSelectionDiagnostics& observed) {
    if(windowDismiss.selectingVerified){
        observed.treeOffset=windowDismiss.treeOffset;observed.rootOffset=windowDismiss.rootOffset;
        observed.schemaReady=observed.profileReady=true;return true;
    }
    // Selection traversal needs these two original object fields, not the
    // unrelated GetOwningPlayer probe required by the old detailed diagnostics.
    auto user=engine::Class(0x3b0f5b0),tree=engine::Class(0x3b6ebc0);
    if(!user||!tree||user!=engine::Find(L"/Script/UMG.UserWidget")||tree!=engine::Find(L"/Script/UMG.WidgetTree")){
        observed.stage="tree_class_metadata";observed.metadataFailure=0x100;return false;
    }
    if(!DetailedObjectField(user,L"WidgetTree",0x298,windowDismiss.treeOffset)){
        observed.treeOffset=windowDismiss.treeOffset;observed.stage="widget_tree_field_schema";
        observed.metadataFailure=0x200;return false;
    }
    observed.treeOffset=windowDismiss.treeOffset;
    if(!DetailedObjectField(tree,L"RootWidget",0x38,windowDismiss.rootOffset)){
        observed.rootOffset=windowDismiss.rootOffset;observed.stage="root_widget_field_schema";
        observed.metadataFailure=0x400;return false;
    }
    observed.rootOffset=windowDismiss.rootOffset;observed.schemaReady=true;
    auto selecting=Function(L"/Script/UMG.InputKeySelector.GetIsSelectingKey",1);
    auto anchor=Function(L"/Script/UMG.MenuAnchor.IsOpen",1);
    auto combo=Function(L"/Script/UMG.ComboBoxString.IsOpen",1);
    auto type=engine::Class(0x3b832f0),panel=engine::Class(0x3b0fae0);
    auto anchorType=engine::Class(0x3b8c4f0),comboType=engine::Class(0x3b6aa20);
    auto widgetType=engine::Class(0x3b0f3c0);
    auto method=[&](void* function,void* owner,uint16_t bytes,uintptr_t exec)->bool{
        return function&&owner&&Read<void*>(function,0x28)==owner
            &&Read<uint16_t>(function,0xbe)==bytes&&(Read<uint32_t>(function,0xb8)&0x400)!=0
            &&Read<uintptr_t>(function,0xe0)==engine::image+exec;
    };
    if(!type||type!=engine::Find(L"/Script/UMG.InputKeySelector")||!method(selecting,type,1,0x3b83e60))observed.metadataFailure|=1;
    if(!anchorType||anchorType!=engine::Find(L"/Script/UMG.MenuAnchor")||!method(anchor,anchorType,1,0x3b8c700))observed.metadataFailure|=2;
    if(!comboType||comboType!=engine::Find(L"/Script/UMG.ComboBoxString")||!method(combo,comboType,1,0x3b6acd0))observed.metadataFailure|=4;
    if(!panel||panel!=engine::Find(L"/Script/UMG.PanelWidget")||!method(functions.childCount,panel,4,0x3b964a0))observed.metadataFailure|=8;
    if(!panel||!method(functions.childAt,panel,16,0x3b96370))observed.metadataFailure|=16;
    if(!widgetType||widgetType!=engine::Find(L"/Script/UMG.Widget")
        ||!method(functions.getVisibility,widgetType,1,0x3bc1640))observed.metadataFailure|=32;
    if(observed.metadataFailure){observed.stage="getter_metadata";return false;}
    // Bool returns at0: selecting descriptor d37a480 (both RETs), anchor
    // d3793b0 and combo d379950. No selected key, popup or focus state is set.
    struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
    static constexpr Profile profiles[]{
        {0x3b83e60,49,0x90470d7e2d6ea2ccULL},
        {0x3b8c700,76,0x2b6ea070aead4da3ULL},
        {0x3b6acd0,76,0xa30269962b6e661aULL},
        {0x3bc1640,43,0x883a95da75c6ef54ULL},
        {0x3bc4580,220,0xa3f3fff3acc423f3ULL}
    };
    for(const auto& profile:profiles){
        auto code=reinterpret_cast<const uint8_t*>(engine::image+profile.rva);
        if(!original_players::Readable(code,profile.bytes)){
            observed.stage="getter_code_unreadable";observed.profileRva=profile.rva;observed.metadataFailure=0x800;return false;
        }
        uint64_t hash=14695981039346656037ULL;
        for(size_t i=0;i<profile.bytes;++i)hash=(hash^code[i])*1099511628211ULL;
        if(hash!=profile.fnv){
            observed.stage="getter_code_profile";observed.profileRva=profile.rva;observed.metadataFailure=0x1000;return false;
        }
    }
    windowDismiss.getSelecting=selecting;windowDismiss.anchorIsOpen=anchor;windowDismiss.comboIsOpen=combo;
    windowDismiss.visibility=functions.getVisibility;
    windowDismiss.selectingVerified=true;observed.profileReady=true;return true;
}
inline bool TopHasSelectingKeyImpl(void* expectedEntry,void* expectedContent,bool& complete,bool& selecting) {
    complete=false;selecting=false;
    WindowSelectionDiagnostics observed;
    bool ownsScratch=false;
    auto finish=[&](const char* stage,bool okay=true)->bool{
        if(ownsScratch){windowSelectionScratch.busy=false;ownsScratch=false;}
        observed.stage=stage;observed.complete=complete;observed.selecting=selecting;CacheWindowSelection(observed);return okay;
    };
    void *entry=nullptr,*content=nullptr;
    if(!ReadTopEntryImpl(entry,content))return finish("top_native_read",false);
    if(!expectedEntry||!expectedContent||entry!=expectedEntry||content!=expectedContent)return finish("top_changed");
    if(!ResolveWindowSelectingFunctions(observed))return finish(observed.stage);
    // Search only the actual original UMG subtree. Key-selection MenuAnchors
    // and combo dropdowns are not MenuStack entries. Here "selecting" also means
    // one of these native popups is open and should consume this back event.
    // Snapshot BEFORE forwarding Esc; the original handler may close it on return.
    if(windowSelectionScratch.busy)return finish("scan_reentered");
    windowSelectionScratch.busy=true;ownsScratch=true;
    constexpr size_t MaximumWidgets=WindowSelectionMaximumWidgets,HashSlots=WindowSelectionHashSlots;
    auto& widgets=windowSelectionScratch.widgets;auto& hashes=windowSelectionScratch.hashes;
    std::memset(hashes,0,sizeof(hashes));size_t count=0;
    auto enqueue=[&](void* widget)->bool{
        if(!widget)return true;
        const auto key=reinterpret_cast<uintptr_t>(widget)>>4;
        size_t slot=static_cast<size_t>((key^(key>>17))*0x9e3779b97f4a7c15ULL)&(HashSlots-1);
        for(size_t probes=0;probes<HashSlots;++probes){
            if(hashes[slot]==widget)return true;
            if(!hashes[slot]){
                if(count==MaximumWidgets)return false;
                hashes[slot]=widget;widgets[count++]=widget;return true;
            }
            slot=(slot+1)&(HashSlots-1);
        }
        return false;
    };
    if(!enqueue(content))return finish("root_widget_limit");
    struct Child {int32_t index=0;uint32_t padding=0;void* widget=nullptr;};
    static_assert(sizeof(Child)==16);
    for(size_t n=0;n<count;++n){
        auto widget=widgets[n];
        observed.visited=static_cast<uint32_t>(n+1);observed.queued=static_cast<uint32_t>(count);
        observed.failedWidget=reinterpret_cast<uintptr_t>(widget);observed.childIndex=-1;
        if(!IsA(widget,0x3b0f3c0))return finish("widget_type");
        uint8_t visibility=0xff;++observed.visibilityQueries;
        if(!Dispatch(widget,windowDismiss.visibility,&visibility)||visibility>4)return finish("visibility_result");
        // Original ESlateVisibility table BC76050: Collapsed=1, Hidden=2.
        // Visible=0, HitTestInvisible=3 and SelfHitTestInvisible=4 all retain
        // their complete child/capture scan. Only invisible ancestor subtrees
        // cannot own an active visible selector/popup for this event.
        if(visibility==1||visibility==2){++observed.skippedHidden;continue;}
        void* query=IsA(widget,0x3b832f0)?windowDismiss.getSelecting:
            (IsA(widget,0x3b8c4f0)?windowDismiss.anchorIsOpen:
             (IsA(widget,0x3b6aa20)?windowDismiss.comboIsOpen:nullptr));
        if(query){
            if(query==windowDismiss.getSelecting)++observed.selectors;
            else if(query==windowDismiss.anchorIsOpen)++observed.anchors;else ++observed.combos;
            uint8_t active=0xff;
            if(!Dispatch(widget,query,&active)||active>1)return finish("getter_result");
            if(active){selecting=true;return finish("native_capture_or_popup");}
        }
        if(IsA(widget,0x3b0f5b0)){
            auto tree=Read<void*>(widget,static_cast<size_t>(windowDismiss.treeOffset));
            if(tree){
                if(!IsA(tree,0x3b6ebc0))return finish("widget_tree_type");
                if(!enqueue(Read<void*>(tree,static_cast<size_t>(windowDismiss.rootOffset))))return finish("tree_widget_limit");
            }
        }
        if(IsA(widget,0x3b0fae0)){
            int32_t children=-1;
            if(!Dispatch(widget,functions.childCount,&children)||children<0
                ||children>static_cast<int32_t>(MaximumWidgets))return finish("panel_child_count");
            for(int32_t index=0;index<children;++index){
                observed.childIndex=index;Child child{index};
                if(!Dispatch(widget,functions.childAt,&child))return finish("panel_child_dispatch");
                if(!child.widget)return finish("panel_null_child");
                if(!enqueue(child.widget))return finish("panel_widget_limit");
            }
        }
    }
    complete=true;observed.queued=static_cast<uint32_t>(count);observed.failedWidget=0;observed.childIndex=-1;
    return finish("complete_no_native_capture");
}
inline bool TopHasSelectingKey(void* expectedEntry,void* expectedContent,bool& complete,bool& selecting) {
    __try { return TopHasSelectingKeyImpl(expectedEntry,expectedContent,complete,selecting); }
    __except (CaptureException(GetExceptionInformation())) {
        windowSelectionScratch.busy=false;complete=false;selecting=false;
        WindowSelectionDiagnostics observed;observed.stage="native_exception";CacheWindowSelection(observed);return false;
    }
}
inline bool DismissTopEntryImpl(void* expectedEntry,void* expectedContent,bool allowOptionsExit) {
    if(!UpdateImpl(false))return false;
    void *entry=nullptr,*content=nullptr;
    if(!ReadTopEntryImpl(entry,content))return false;
    if(!expectedEntry||!expectedContent||entry!=expectedEntry||content!=expectedContent){
        ++windowDismiss.staleRequests;return true;
    }
    if(state.optionsKnown&&state.optionsOpen&&entry==state.optionsEntry&&content==state.optionsWidget){
        // Original AllocateOptionsScreen7a87c70 -> factory7ab5f30 creates
        // Options Flags=8, with dismissable bit0 clear. RequestPop cannot close
        // it. Original OnOpened instead binds InGameMenu to OptionsScreen.Exit,
        // the same graph as its close button (ApplySettingsCustomizations/Close).
        // A caller must have observed a complete, popup-free PRE-event tree.
        // Rechecking here also protects a popup opened since the event snapshot.
        if(!allowOptionsExit){++windowDismiss.staleRequests;return true;}
        bool complete=false,selecting=false;
        if(!TopHasSelectingKeyImpl(entry,content,complete,selecting))return false;
        if(!complete||selecting){++windowDismiss.staleRequests;return true;}
        if(!original_input_bindings::EnsureSettingsCommitContext(original_hud::State().clientInstance))
            return Fail(original_input_bindings::State().error,original_input_bindings::State().stage);
        auto owner=Read<void*>(functions.optionsExit,0x28);
        if(!functions.optionsExit||!original_hud::Derived(Read<void*>(content,0x10),owner))
            return Fail(3147,"original_ui_window_dismiss_original_options_exit_missing");
        if(!Dispatch(content,functions.optionsExit,nullptr))
            return Fail(3148,"original_ui_window_dismiss_original_options_exit_failed");
        ++state.originalOptionsExits;++windowDismiss.requests;return UpdateImpl(false);
    }
    // HandleDismissInput -> RequestPop: the original top entry must allow
    // dismiss and its original RequestCloseMenu may intercept. A still-open
    // entry is a legitimate native result; never follow it with Close or Pop.
    if(!Dispatch(state.menuStack,windowDismiss.handleDismiss,nullptr))
        return Fail(3146,"original_ui_window_dismiss_native_dispatch_failed");
    ++windowDismiss.requests;return UpdateImpl(false);
}
inline bool DismissTopEntry(void* expectedEntry,void* expectedContent,bool allowOptionsExit=false) {
    __try { return DismissTopEntryImpl(expectedEntry,expectedContent,allowOptionsExit); }
    __except (CaptureException(GetExceptionInformation())) { return false; }
}
inline bool CloseActiveUIImpl() {
    state.pendingPhysicalDismiss=false;state.dismissReleasedAt=0;
    if(!UpdateImpl(false))return false;
    if(!CloseCurrentMenu())return state.error?false:Fail(3130,"original_ui_input_original_menu_dismiss_failed");
    return UpdateImpl(false);
}
// IPC/readback tests use the same original close leaves as physical Escape.
// This never opens a menu and remains usable during death/respawn.
inline bool CloseActiveUI() {
    __try { return CloseActiveUIImpl(); }
    __except (CaptureException(GetExceptionInformation())) { return false; }
}
inline bool WriteDiagnostics(const wchar_t* path) {
    if (!path || wcslen(path) > 960) return false;
    wchar_t temporary[1024]{};
    if (swprintf_s(temporary, L"%ls.tmp", path) < 0) return false;
    FILE* file = nullptr;
    if (_wfopen_s(&file, temporary, L"wb") != 0 || !file) return false;
    const auto& s = state;
    std::fprintf(file, "{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,"
        "\"local_bindings_valid\":%s,\"options_known\":%s,\"options_open\":%s,\"map_known\":%s,\"map_open\":%s,"
        "\"map_visibility_queried_on_edge\":%s,\"map_visible_on_last_queried_edge\":%s,"
        "\"block_gameplay\":%s,\"desired_ui_only\":%s,\"applied_ui_only\":%s,\"applied_mode_known\":%s,"
        "\"cursor_visible\":%s,\"cursor_readback_matches_last_edge\":%s,\"reflected_cursor_setter_found\":%s,"
        "\"local_cursor_property_fallback_used\":%s,\"cursor_flags\":%u,\"cursor_flags_before_edge\":%u,\"cursor_flags_after_edge\":%u,"
        "\"native_ares_mode\":%u,\"default_mouse_cursor\":%u,\"current_mouse_cursor\":%u,"
        "\"updates\":%u,\"owner_changes\":%u,\"mode_edges\":%u,\"original_mode_dispatches\":%u,"
        "\"reflected_cursor_setter_dispatches\":%u,\"local_cursor_property_edge_writes\":%u,\"original_focus_dispatches\":%u,"
        "\"controller\":\"0x%llx\",\"local_player\":\"0x%llx\",\"pawn\":\"0x%llx\","
        "\"options_widget\":\"0x%llx\",\"map_coordinator\":\"0x%llx\",\"map_widget\":\"0x%llx\","
        "\"exception_rva\":\"0x%llx\",\"restart_required\":%s,\"hotkey_state_inference\":false,\"per_frame_cursor_force\":false",
        s.stage, s.error, s.initialized ? "true" : "false", s.localBindingsValid ? "true" : "false",
        s.optionsKnown ? "true" : "false", s.optionsOpen ? "true" : "false", s.mapKnown ? "true" : "false", s.mapOpen ? "true" : "false",
        s.mapVisibilityQueried ? "true" : "false", s.mapVisible ? "true" : "false",
        s.blockGameplay ? "true" : "false", s.desiredUIOnly ? "true" : "false", s.appliedUIOnly ? "true" : "false", s.appliedModeKnown ? "true" : "false",
        s.cursorVisible ? "true" : "false", s.cursorReadbackMatchesLastEdge ? "true" : "false", s.reflectedCursorSetterFound ? "true" : "false",
        s.localCursorPropertyFallbackUsed ? "true" : "false", s.cursorFlags, s.cursorFlagsBeforeLastEdge, s.cursorFlagsAfterLastEdge,
        static_cast<unsigned>(s.nativeAresMode), static_cast<unsigned>(s.defaultMouseCursor), static_cast<unsigned>(s.currentMouseCursor),
        s.updateCalls, s.ownerChanges, s.modeEdges, s.originalModeDispatches, s.reflectedCursorSetterDispatches,
        s.localCursorPropertyEdgeWrites, s.originalFocusDispatches,
        original_hud::Pointer(s.controller), original_hud::Pointer(s.localPlayer), original_hud::Pointer(s.pawn),
        original_hud::Pointer(s.optionsWidget), original_hud::Pointer(s.mapCoordinator), original_hud::Pointer(s.mapWidget),
        static_cast<unsigned long long>(s.exceptionRva), s.restartRequired ? "true" : "false");
    std::fprintf(file,",\"viewport_mode\":\"%s\",\"game_and_ui_applied\":%s,\"viewport_mode_dispatches\":%u,\"shop_known\":%s,\"shop_open\":%s,\"external_observed_open\":%s,\"focus_target_changes\":%u,\"widget_has_focus\":%s,\"widget_has_focused_descendants\":%s,\"escape_edges\":%u,\"buy_edges\":%u,\"dismiss_fallbacks\":%u,\"native_dismisses_observed\":%u,\"close_requests\":%u,\"close_completions\":%u,\"close_observed\":%s,\"original_options_exits\":%u,\"original_shop_closes\":%u,\"original_content_closes\":%u,\"pawn_mode_invalidations\":%u,\"mode_drift_reapplies\":%u,\"mode_readback_drift\":%s,\"uses_ui_only\":true",(s.appliedModeKnown&&s.appliedUIOnly)?"ui_only":"game_only",s.gameAndUIApplied?"true":"false",s.viewportModeDispatches,s.shopKnown?"true":"false",s.shopOpen?"true":"false",s.externalObservedOpen?"true":"false",s.focusTargetChanges,s.widgetHasFocus?"true":"false",s.widgetHasFocusedDescendants?"true":"false",s.escapeEdges,s.buyEdges,s.dismissFallbacks,s.nativeDismissesObserved,s.closeRequests,s.closeCompletions,s.closeObserved?"true":"false",s.originalOptionsExits,s.originalShopCloses,s.originalContentCloses,s.pawnModeInvalidations,s.modeDriftReapplies,s.modeReadbackDrift?"true":"false");
    std::fprintf(file,",\"interaction_samples\":%u,\"last_interaction_sample_ms\":%llu,\"last_menu_hovered\":%s,\"last_menu_mouse_captured\":%s,\"hovered_samples\":%u,\"mouse_capture_samples\":%u,\"last_foreground_owned\":%s,\"last_primary_button_down\":%s,\"last_secondary_button_down\":%s,\"focus_preserved_repairs\":%u",s.interactionSamples,static_cast<unsigned long long>(s.lastInteractionSample),s.menuHovered?"true":"false",s.menuMouseCaptured?"true":"false",s.hoveredSamples,s.mouseCaptureSamples,s.foregroundOwned?"true":"false",s.primaryButtonDown?"true":"false",s.secondaryButtonDown?"true":"false",s.focusPreservedRepairs);
    std::fprintf(file,",\"ui_only_applied\":%s,\"pending_physical_dismiss\":%s,\"delayed_dismisses\":%u,\"dismiss_cancelled\":%u,\"dismiss_release_settle_ms\":100,\"detailed_probe\":%s,\"detailed_samples\":%u,\"detailed_visited\":%u,\"detailed_truncated\":%s,\"detailed_enabled_query_available\":%s,\"menu_widget_observations\":[",s.appliedModeKnown&&s.appliedUIOnly?"true":"false",s.pendingPhysicalDismiss?"true":"false",s.delayedDismisses,s.dismissCancelled,s.detailedProbe?"true":"false",s.detailedSamples,s.detailedVisited,s.detailedProbeTruncated?"true":"false",functions.getIsEnabled?"true":"false");
    for(uint32_t i=0;i<s.detailedCount&&i<64;++i){if(i)std::fputc(',',file);const auto& o=s.detailedWidgets[i];
        std::fprintf(file,"{\"widget\":\"%p\",\"owner_controller\":\"%p\",\"name\":",o.widget,o.owningController);
        original_hud::WriteJSONText(file,o.name);std::fputs(",\"class\":",file);original_hud::WriteJSONText(file,o.type);
        std::fprintf(file,",\"button\":%s,\"enabled\":%s,\"visibility\":%u,\"hovered\":%s,\"mouse_captured\":%s,\"focused\":%s,\"focused_descendant\":%s,\"owner_queried\":%s,\"enabled_queried\":%s,\"visibility_queried\":%s}",o.button?"true":"false",o.enabled?"true":"false",unsigned(o.visibility),o.hovered?"true":"false",o.captured?"true":"false",o.focused?"true":"false",o.focusedChild?"true":"false",o.ownerQueried?"true":"false",o.enabledQueried?"true":"false",o.visibilityQueried?"true":"false");
    }
    std::fprintf(file,"],\"detailed_count\":%u,\"last_detailed_sample_ms\":%llu,\"detailed_profile_checked\":%s,\"detailed_profile_valid\":%s,\"detailed_tree_offset\":%d,\"detailed_root_offset\":%d",s.detailedCount,static_cast<unsigned long long>(s.lastDetailedSample),s.detailedProfileChecked?"true":"false",s.detailedProfileValid?"true":"false",s.detailedTreeOffset,s.detailedRootOffset);
    std::fprintf(file,",\"input_lock_api_verified\":%s,\"input_lock_scope_active\":%s,\"input_lock_controller\":\"%p\",\"input_lock_identity_index\":%d,\"input_lock_identity_serial\":%d,\"input_lock_before\":%u,\"input_lock_after_acquire\":%u,\"input_lock_after_restore\":%u,\"input_lock_now\":%u,\"input_lock_acquisitions\":%u,\"input_lock_restores\":%u,\"input_lock_reassertions\":%u,\"input_lock_stale_retires\":%u,\"input_lock_api_calls\":%u,\"input_lock_per_frame_force\":false}",s.inputLockApiVerified?"true":"false",s.inputLockScopeActive?"true":"false",s.inputLockController,s.inputLockIdentity.index,s.inputLockIdentity.serial,unsigned(s.inputLockBefore),unsigned(s.inputLockAfterAcquire),unsigned(s.inputLockAfterRestore),unsigned(s.inputLockNow),s.inputLockAcquisitions,s.inputLockRestores,s.inputLockReassertions,s.inputLockStaleRetires,s.inputLockApiCalls);
    const bool written = std::ferror(file) == 0;
    const bool closed = std::fclose(file) == 0;
    return written && closed && MoveFileExW(temporary, path, MOVEFILE_REPLACE_EXISTING);
}
} // namespace original_ui_input_mode
