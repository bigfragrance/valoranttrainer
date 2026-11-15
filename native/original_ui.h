#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include "engine_api.h"

// Original installed UMG containers and original Texture2D assets, with a small
// LOCAL composition layer. This does not restore ShooterHUD, ability gameplay,
// the coordinated HUD model, settings persistence, or dynamic minimap projection.
// All methods, including Shutdown, run on the Initialize caller's game thread.
// Shutdown must run before world teardown. The owning bridge pins the image hash.
// Evidence: reports/duel1-original-hud-settings.json and its implementation appendix.
namespace original_ui {

inline constexpr const wchar_t* kImageClass =
    L"/Game/UI/Screens/OutOfGame/MainMenu/Collection/Sprays/SprayImage.SprayImage_C";
inline constexpr const wchar_t* kSettingsClass =
    L"/Game/UI/OutOfGame/Settings/Screens/WBP_Template_SettingScreen.WBP_Template_SettingScreen_C";
inline constexpr const wchar_t* kBoolClass =
    L"/Game/UI/Screens/OutOfGame/MainMenu/Settings/BaseBoolOptionWidget.BaseBoolOptionWidget_C";
inline constexpr const wchar_t* kDuel1Map =
    L"/Game/UI/InGame/Minimap/Maps/Duel1/TX_Map_Duel1_Revealed.TX_Map_Duel1_Revealed";
inline constexpr const wchar_t* kJettCloudburst =
    L"/Game/UI/InGame/HUD/Abilities/Assets/ShippingIcons/Wushu/TX_Wushu_Smoke.TX_Wushu_Smoke";
inline constexpr const wchar_t* kJettUpdraft =
    L"/Game/UI/InGame/HUD/Abilities/Assets/ShippingIcons/Wushu/TX_Wushu_Boost.TX_Wushu_Boost";
inline constexpr const wchar_t* kJettTailwind =
    L"/Game/UI/InGame/HUD/Abilities/Assets/ShippingIcons/Wushu/TX_Wushu_Dash.TX_Wushu_Dash";
inline constexpr const wchar_t* kJettBladeStorm =
    L"/Game/UI/InGame/HUD/Abilities/Assets/ShippingIcons/Wushu/TX_Wushu_Daggers.TX_Wushu_Daggers";
inline constexpr const wchar_t* kAmmoIcon =
    L"/Game/UI/InGame/HUD/Assets/TX_icon_ammo.TX_icon_ammo";
inline constexpr const wchar_t* kHealthCircle =
    L"/Game/UI/InGame/HUD/Assets/TX_HUD_healthCircle_120x120.TX_HUD_healthCircle_120x120";

struct Rect { double x = 24, y = 24, width = 240, height = 240; };
struct Vec2 { double x, y; };
using Handle = int32_t;
inline constexpr Handle InvalidHandle = -1;
inline constexpr size_t kMaximumWidgets = 24;

struct CreateParameters {
    void* world = nullptr;
    void* type = nullptr;
    void* controller = nullptr;
    void* result = nullptr;
};
struct PositionParameters {
    Vec2 position{};
    uint8_t removeDpiScale = 0;
    uint8_t padding[7]{};
};
struct BrushParameters {
    void* texture = nullptr;
    uint8_t matchSize = 0;
    uint8_t padding[7]{};
};
struct ChildParameters {
    int32_t index = 0;
    uint32_t padding = 0;
    void* result = nullptr;
};
static_assert(sizeof(CreateParameters) == 0x20);
static_assert(offsetof(CreateParameters, result) == 0x18);
static_assert(sizeof(PositionParameters) == 0x18);
static_assert(offsetof(PositionParameters, removeDpiScale) == 0x10);
static_assert(sizeof(BrushParameters) == 0x10);
static_assert(offsetof(BrushParameters, matchSize) == 8);
static_assert(sizeof(ChildParameters) == 0x10);
static_assert(offsetof(ChildParameters, result) == 8);

enum Error : uint32_t {
    None = 0, MissingContext = 501, WrongThread = 502, MissingReflection = 503,
    MissingWidgetClass = 504, CreateFailed = 505, ViewportAttachFailed = 506,
    MissingImageChild = 507, MissingTexture = 508, InvalidRectangle = 509,
    WidgetLimit = 510, InvalidHandleValue = 511, ContextChanged = 512,
    WidgetOperationFailed = 513
};
struct Diagnostics {
    bool initialized = false;
    bool reflectionAvailable = false;
    bool settingsCreated = false;
    bool settingsVisible = false;
    bool mapCreated = false;
    bool mapVisible = false;
    uint32_t createdWidgets = 0;
    uint32_t displayedImages = 0;
    uint32_t textureAssignments = 0;
    uint32_t lastTreeVisitedWidgets = 0;
    uint32_t lastTreeImageCandidates = 0;
    uint32_t lastError = None;
    const char* stage = "original_ui_not_initialized";
};
struct Element {
    void* widget = nullptr;
    void* image = nullptr;
    bool viewportAttached = false;
    bool visible = false;
    bool interactive = false;
};
inline Diagnostics diagnostics;
inline Element elements[kMaximumWidgets]{};
inline void* worldContext = nullptr;
inline void* owningController = nullptr;
inline DWORD gameThread = 0;
inline Handle settingsHandle = InvalidHandle;
inline Handle mapHandle = InvalidHandle;

inline bool Fail(Error error, const char* stage) {
    diagnostics.lastError = error; diagnostics.stage = stage; return false;
}
inline bool CheckContext() {
    if (!diagnostics.initialized || !worldContext || !owningController || !engine::image)
        return Fail(MissingContext, "original_ui_context_missing");
    if (GetCurrentThreadId() != gameThread)
        return Fail(WrongThread, "original_ui_wrong_thread");
    return true;
}
inline bool ValidRect(const Rect& r) {
    return std::isfinite(r.x) && std::isfinite(r.y) && std::isfinite(r.width) && std::isfinite(r.height)
        && r.width > 0 && r.height > 0 && r.width <= 16384 && r.height <= 16384
        && std::abs(r.x) <= 32768 && std::abs(r.y) <= 32768;
}
inline Element* Get(Handle handle) {
    if (handle < 0 || static_cast<size_t>(handle) >= kMaximumWidgets || !elements[handle].widget) {
        Fail(InvalidHandleValue, "original_ui_invalid_handle"); return nullptr;
    }
    return &elements[handle];
}
inline void* LoadWidgetClass(const wchar_t* path) {
    // Native StaticLoadClass validates the requested UserWidget superclass.
    // At 0x1d0c744 it calls typed StaticLoadObject, and 0x1d0c75a checks ancestry.
    return engine::function<void*(*)(void*, void*, const wchar_t*, const wchar_t*, uint32_t, void*)>(0x1d0c6e0)
        (engine::Class(0x3b0f5b0), nullptr, path, nullptr, 0, nullptr);
}
inline void* FindChild(void* outer, uintptr_t classGetter, const wchar_t* name) {
    if (!outer) return nullptr;
    // Exact known original subobject name and typed outer, never ANY_PACKAGE.
    return engine::function<void*(*)(void*, void*, const wchar_t*, bool)>(0x1d07dd0)
        (engine::Class(classGetter), outer, name, false);
}
template<class T> inline T Read(const void* object, size_t offset) {
    T result{};
    if (object) std::memcpy(&result, static_cast<const uint8_t*>(object) + offset, sizeof(result));
    return result;
}
inline bool IsA(void* object, void* baseClass) {
    if (!object || !baseClass) return false;
    void* type = Read<void*>(object, 0x10);
    if (!type) return false;
    // Same FStructBaseChain comparison used by pinned StaticLoadClass and Create.
    const int32_t depth = Read<int32_t>(baseClass, 0x40);
    const int32_t objectDepth = Read<int32_t>(type, 0x40);
    if (depth < 0 || depth > objectDepth || objectDepth > 1024) return false;
    void* chain = Read<void*>(type, 0x38);
    return chain && Read<void*>(chain, static_cast<size_t>(depth) * 8)
        == static_cast<uint8_t*>(baseClass) + 0x38;
}
inline void* FindOriginalImageInTree(void* userWidget) {
    diagnostics.lastTreeVisitedWidgets = diagnostics.lastTreeImageCandidates = 0;
    // Generated native descriptors: UserWidget.WidgetTree +0x298,
    // WidgetTree.RootWidget +0x38. The duplicated tree is an object reference;
    // its runtime UObject name/Outer need not match the cooked export's name.
    void* tree = Read<void*>(userWidget, 0x298);
    if (!IsA(tree, engine::Class(0x3b6ebc0))) return nullptr;
    void* root = Read<void*>(tree, 0x38);
    if (!root) return nullptr;
    void* imageClass = engine::Class(0x3b7f2f0);
    void* panelClass = engine::Class(0x3b0fae0);
    void* widgetClass = engine::Class(0x3b0f3c0);
    void* queue[64]{}; size_t count = 1, current = 0;
    queue[0] = root;
    void* result = nullptr;
    while (current < count) {
        void* widget = queue[current++];
        if (!IsA(widget, widgetClass)) return nullptr;
        ++diagnostics.lastTreeVisitedWidgets;
        if (IsA(widget, imageClass)) {
            ++diagnostics.lastTreeImageCandidates; result = widget;
            continue;
        }
        if (!IsA(widget, panelClass)) continue;
        int32_t children = 0;
        if (!engine::Event(widget, L"/Script/UMG.PanelWidget.GetChildrenCount", &children)
            || children < 0 || children > 32) return nullptr;
        for (int32_t index = 0; index < children; ++index) {
            ChildParameters child{index, 0, nullptr};
            if (!engine::Event(widget, L"/Script/UMG.PanelWidget.GetChildAt", &child)) return nullptr;
            if (!child.result) continue;
            bool seen = false;
            for (size_t n = 0; n < count; ++n) if (queue[n] == child.result) { seen = true; break; }
            if (seen) continue;
            if (count == 64) return nullptr;
            queue[count++] = child.result;
        }
    }
    // Original SprayImage contains exactly one UImage. Never select an arbitrary
    // image if an updated/replaced widget tree contains several candidates.
    return diagnostics.lastTreeImageCandidates == 1 ? result : nullptr;
}
inline bool WidgetVisible(Element& element, bool visible) {
    // ESlateVisibility: Visible=0, Collapsed=1, HitTestInvisible=3.
    // Hidden widgets remain attached; the viewport owns their UObject lifetime.
    uint8_t value = visible ? (element.interactive ? 0 : 3) : 1;
    if (!engine::Event(element.widget, L"/Script/UMG.Widget.SetVisibility", &value))
        return Fail(WidgetOperationFailed, "original_ui_visibility_failed");
    element.visible = visible; return true;
}
inline bool Position(Element& element, const Rect& r) {
    if (!ValidRect(r)) return Fail(InvalidRectangle, "original_ui_invalid_rectangle");
    PositionParameters position{{r.x, r.y}, 0, {}};
    Vec2 size{r.width, r.height};
    // Coordinates and size are both viewport logical units. No mixed DPI units.
    return engine::Event(element.widget, L"/Script/UMG.UserWidget.SetPositionInViewport", &position)
        && engine::Event(element.widget, L"/Script/UMG.UserWidget.SetDesiredSizeInViewport", &size);
}
inline bool Initialize(void* world, void* controller) {
    if (diagnostics.initialized) {
        if (world != worldContext || controller != owningController)
            return Fail(ContextChanged, "original_ui_context_changed_call_shutdown_first");
        return CheckContext();
    }
    if (!engine::image || !world || !controller)
        return Fail(MissingContext, "original_ui_initialize_context_missing");
    const wchar_t* required[] = {
        L"/Script/UMG.WidgetBlueprintLibrary.Create", L"/Script/UMG.UserWidget.AddToViewport",
        L"/Script/UMG.UserWidget.SetPositionInViewport", L"/Script/UMG.UserWidget.SetDesiredSizeInViewport",
        L"/Script/UMG.Widget.IsInViewport", L"/Script/UMG.Widget.SetVisibility",
        L"/Script/UMG.Widget.RemoveFromParent", L"/Script/UMG.Image.SetBrushFromTexture",
        L"/Script/UMG.PanelWidget.GetChildAt", L"/Script/UMG.PanelWidget.GetChildrenCount"
    };
    // Force the owning reflection classes to register before resolving methods.
    engine::Class(0x3b1d050); engine::Class(0x3b0f5b0); engine::Class(0x3b7f2f0);
    for (auto method : required)
        if (!engine::Find(method)) return Fail(MissingReflection, "original_ui_reflection_missing");
    diagnostics = {}; diagnostics.initialized = true; diagnostics.reflectionAvailable = true;
    diagnostics.stage = "original_ui_ready_no_widgets_created";
    worldContext = world; owningController = controller; gameThread = GetCurrentThreadId();
    return true;
}
inline Handle CreateWidget(const wchar_t* path, const Rect& rect, bool interactive, int32_t zOrder) {
    if (!CheckContext() || !path || !ValidRect(rect)) {
        if (diagnostics.initialized && !ValidRect(rect)) Fail(InvalidRectangle, "original_ui_invalid_rectangle");
        return InvalidHandle;
    }
    Handle freeSlot = InvalidHandle;
    for (size_t i = 0; i < kMaximumWidgets; ++i)
        if (!elements[i].widget) { freeSlot = static_cast<Handle>(i); break; }
    if (freeSlot == InvalidHandle) { Fail(WidgetLimit, "original_ui_widget_limit"); return InvalidHandle; }
    diagnostics.stage = "original_ui_loading_original_widget_class";
    void* type = LoadWidgetClass(path);
    if (!type) { Fail(MissingWidgetClass, "original_ui_widget_class_missing"); return InvalidHandle; }
    CreateParameters create{worldContext, type, owningController, nullptr};
    diagnostics.stage = "original_ui_creating_original_widget";
    void* library = engine::DefaultObject(engine::Class(0x3b1d050));
    if (!library || !engine::Event(library, L"/Script/UMG.WidgetBlueprintLibrary.Create", &create) || !create.result) {
        Fail(CreateFailed, "original_ui_widget_create_failed"); return InvalidHandle;
    }
    Element element{create.result, nullptr, false, false, interactive};
    // Attach in the same game-thread call in which the UObject is created.
    // No unrooted raw widget pointer is retained across ticks before attachment.
    diagnostics.stage = "original_ui_attaching_original_widget";
    uint8_t attached = 0;
    bool invoked = engine::Event(element.widget, L"/Script/UMG.UserWidget.AddToViewport", &zOrder);
    if (!invoked || !engine::Event(element.widget, L"/Script/UMG.Widget.IsInViewport", &attached) || !attached) {
        engine::Event(element.widget, L"/Script/UMG.Widget.RemoveFromParent", nullptr);
        Fail(ViewportAttachFailed, "original_ui_viewport_attachment_not_observed"); return InvalidHandle;
    }
    element.viewportAttached = true;
    if (!Position(element, rect) || !WidgetVisible(element, true)) {
        engine::Event(element.widget, L"/Script/UMG.Widget.RemoveFromParent", nullptr);
        Fail(WidgetOperationFailed, "original_ui_widget_layout_failed"); return InvalidHandle;
    }
    elements[freeSlot] = element; ++diagnostics.createdWidgets;
    diagnostics.lastError = None; diagnostics.stage = "original_ui_original_widget_attached";
    return freeSlot;
}
inline bool Remove(Handle handle) {
    if (!CheckContext()) return false;
    Element* element = Get(handle); if (!element) return false;
    const bool removed = engine::Event(element->widget, L"/Script/UMG.Widget.RemoveFromParent", nullptr);
    // Never retain a raw pointer after releasing viewport ownership.
    *element = {};
    if (settingsHandle == handle) {
        settingsHandle = InvalidHandle; diagnostics.settingsCreated = false; diagnostics.settingsVisible = false;
    }
    if (mapHandle == handle) {
        mapHandle = InvalidHandle; diagnostics.mapCreated = false; diagnostics.mapVisible = false;
    }
    return removed;
}
inline bool Show(Handle handle, bool visible) {
    if (!CheckContext()) return false;
    auto element = Get(handle); return element && WidgetVisible(*element, visible);
}
inline bool SetRectangle(Handle handle, const Rect& rect) {
    if (!CheckContext()) return false;
    auto element = Get(handle); return element && Position(*element, rect);
}
inline bool SetImageTexture(Handle handle, const wchar_t* texturePath) {
    if (!CheckContext() || !texturePath) return false;
    auto element = Get(handle); if (!element) return false;
    if (!element->image) return Fail(MissingImageChild, "original_ui_image_child_missing");
    diagnostics.stage = "original_ui_loading_original_texture";
    void* texture = engine::Load(engine::Class(0x2a56130), texturePath);
    if (!texture) return Fail(MissingTexture, "original_ui_original_texture_missing");
    BrushParameters brush{texture, 0, {}};
    if (!engine::Event(element->image, L"/Script/UMG.Image.SetBrushFromTexture", &brush))
        return Fail(WidgetOperationFailed, "original_ui_image_brush_failed");
    uint8_t visibility = 3;
    if (!engine::Event(element->image, L"/Script/UMG.Widget.SetVisibility", &visibility))
        return Fail(WidgetOperationFailed, "original_ui_image_visibility_failed");
    ++diagnostics.textureAssignments;
    diagnostics.lastError = None; diagnostics.stage = "original_ui_original_texture_assigned";
    return true;
}
inline Handle CreateOriginalImage(const wchar_t* texturePath, const Rect& rect, int32_t zOrder = 20) {
    Handle handle = CreateWidget(kImageClass, rect, false, zOrder);
    if (handle == InvalidHandle) return handle;
    auto& element = elements[handle];
    // Use actual referenced runtime tree and reflected panel children. Runtime
    // testing rejected the earlier exact-name/Outer lookup (error507).
    element.image = FindOriginalImageInTree(element.widget);
    if (!element.image) {
        Remove(handle); Fail(MissingImageChild, "original_ui_original_image_child_not_found"); return InvalidHandle;
    }
    if (!SetImageTexture(handle, texturePath)) {
        const auto error = diagnostics.lastError; const auto stage = diagnostics.stage;
        Remove(handle); diagnostics.lastError = error; diagnostics.stage = stage; return InvalidHandle;
    }
    ++diagnostics.displayedImages; return handle;
}
inline bool ShowDuel1Map(bool visible, const Rect& rect = {24, 24, 240, 240}) {
    if (!CheckContext()) return false;
    if (mapHandle == InvalidHandle) {
        if (!visible) return true;
        mapHandle = CreateOriginalImage(kDuel1Map, rect, 15);
        if (mapHandle == InvalidHandle) return false;
        diagnostics.mapCreated = true;
    }
    if (!SetRectangle(mapHandle, rect) || !Show(mapHandle, visible)) return false;
    diagnostics.mapVisible = visible; return true;
}
inline bool ShowSettingsTemplate(bool visible, const Rect& rect = {80, 80, 1120, 560}) {
    if (!CheckContext()) return false;
    if (settingsHandle == InvalidHandle) {
        if (!visible) return true;
        settingsHandle = CreateWidget(kSettingsClass, rect, true, 200);
        if (settingsHandle == InvalidHandle) return false;
        diagnostics.settingsCreated = true;
    }
    if (!SetRectangle(settingsHandle, rect) || !Show(settingsHandle, visible)) return false;
    diagnostics.settingsVisible = visible; return true;
}
inline Handle CreateBoolOption(const Rect& rect, int32_t zOrder = 210) {
    // An original visual two-choice control. Persistence and local option binding
    // are deliberately not asserted by this factory.
    return CreateWidget(kBoolClass, rect, true, zOrder);
}
inline const Diagnostics& State() { return diagnostics; }
inline bool Shutdown() {
    if (!diagnostics.initialized) return true;
    if (!CheckContext()) return false;
    bool ok = true;
    for (size_t i = 0; i < kMaximumWidgets; ++i)
        if (elements[i].widget) ok = Remove(static_cast<Handle>(i)) && ok;
    worldContext = nullptr; owningController = nullptr; gameThread = 0;
    settingsHandle = mapHandle = InvalidHandle;
    diagnostics.initialized = false; diagnostics.stage = "original_ui_shutdown";
    return ok;
}
} // namespace original_ui
