#pragma once
#include "original_ui_input_mode.h"
#include <array>
#include <cstdio>

// Passive, menu-only observations. No input, focus, visibility or capture writes.
namespace training_menu_probe {
struct Entry {void* object=nullptr;void* owner=nullptr;char name[128]{};uint8_t visibility=0xff;
    bool enabled=false,hovered=false,captured=false,focused=false,descendants=false;};
inline std::array<Entry,65> entries{};
inline size_t count=0;
inline uint64_t sampledAt=0,samples=0;
inline void* getVisibility=nullptr;inline void* getEnabled=nullptr;inline void* getOwner=nullptr;
inline void* getMousePosition=nullptr;
inline POINT desktopMouse{},clientMouse{};
inline bool windowOwned=false,mouseQueried=false;
struct MousePosition {float x=0,y=0;uint8_t valid=0,padding[3]{};};
inline MousePosition nativeMouse{};
inline void* controllerWorld=nullptr;inline void* contextViewport=nullptr;inline void* playerViewport=nullptr;
inline void* sceneViewport=nullptr;inline void* sceneTable=nullptr;inline void* sceneMouseGetter=nullptr;
inline uint8_t inputLock=0xff;
inline uint8_t ignoreInput=0xff,hideCursor=0xff,mouseLock=0xff;
inline void* viewportWidgetWeak=nullptr;inline void* viewportWidgetControl=nullptr;
inline uintptr_t Rva(void* p){const auto at=reinterpret_cast<uintptr_t>(p);return at>=engine::image&&at-engine::image<0x10000000?at-engine::image:0;}
inline bool valid=false;
inline bool enabled=false;
inline void Observe(Entry& entry,void* object){
    if(entry.object!=object){entry={};entry.object=object;original_hud::OriginalObjectName(object,entry.name);}
    entry.owner=nullptr;entry.visibility=0xff;entry.enabled=entry.hovered=entry.captured=entry.focused=entry.descendants=false;
    const auto& f=original_ui_input_mode::functions;
    (void)original_ui_input_mode::Dispatch(object,getVisibility,&entry.visibility);
    (void)original_ui_input_mode::Dispatch(object,getEnabled,&entry.enabled);
    (void)original_ui_input_mode::Dispatch(object,getOwner,&entry.owner);
    (void)original_ui_input_mode::Dispatch(object,f.isHovered,&entry.hovered);
    (void)original_ui_input_mode::Dispatch(object,f.hasMouseCapture,&entry.captured);
    original_ui_input_mode::FocusQuery q{original_players::activeController};
    if(original_ui_input_mode::Dispatch(object,f.hasUserFocus,&q))entry.focused=q.result!=0;
    q.result=0;if(original_ui_input_mode::Dispatch(object,f.hasFocusedDescendants,&q))entry.descendants=q.result!=0;
}
inline void Sample(){
    if(!enabled||!original_ui_input_mode::BlocksGameplay())return;
    const auto now=GetTickCount64();if(now-sampledAt<500)return;sampledAt=now;++samples;valid=false;
    if(!getVisibility){getVisibility=original_ui_input_mode::Function(L"/Script/UMG.Widget.GetVisibility",1);
        getEnabled=original_ui_input_mode::Function(L"/Script/UMG.Widget.GetIsEnabled",1);
        getOwner=original_ui_input_mode::Function(L"/Script/UMG.Widget.GetOwningPlayer",8);
        getMousePosition=original_ui_input_mode::Function(L"/Script/Engine.PlayerController.GetMousePosition",9);}
    if(!getVisibility||!getEnabled||!getOwner)return;
    DWORD owner=0;auto window=GetForegroundWindow();GetWindowThreadProcessId(window,&owner);
    windowOwned=owner==GetCurrentProcessId();
    if(windowOwned){GetCursorPos(&desktopMouse);clientMouse=desktopMouse;ScreenToClient(window,&clientMouse);}
    nativeMouse={};mouseQueried=getMousePosition&&original_ui_input_mode::Dispatch(original_players::activeController,getMousePosition,&nativeMouse);
    auto pc=original_players::activeController;auto table=original_hud::Read<void**>(pc,0);
    auto getWorld=original_hud::Read<void*>(table,0x1b8);
    controllerWorld=getWorld?reinterpret_cast<void*(*)(void*)>(getWorld)(pc):nullptr;
    contextViewport=controllerWorld?engine::function<void*(*)(void*)>(0x4cd0be0)(controllerWorld):nullptr;
    playerViewport=original_hud::Read<void*>(original_hud::Read<void*>(pc,0x510),0x80);
    sceneViewport=original_hud::Read<void*>(contextViewport,0xf0);sceneTable=original_hud::Read<void*>(sceneViewport,0);
    sceneMouseGetter=original_hud::Read<void*>(sceneTable,0xf8);inputLock=original_hud::Read<uint8_t>(pc,0xbe8);
    ignoreInput=original_hud::Read<uint8_t>(contextViewport,0x3d9);
    hideCursor=original_hud::Read<uint8_t>(contextViewport,0x3da);mouseLock=original_hud::Read<uint8_t>(contextViewport,0x3dc);
    auto sceneBase=sceneViewport?static_cast<uint8_t*>(sceneViewport)-8:nullptr;
    viewportWidgetWeak=original_hud::Read<void*>(sceneBase,0x260);viewportWidgetControl=original_hud::Read<void*>(sceneBase,0x268);
    auto coordinator=original_hud::State().elementCoordinator;
    auto data=original_hud::Read<void**>(coordinator,0x58);
    const auto n=original_hud::Read<int32_t>(coordinator,0x60);
    if(n<0||n>64||(n&&!original_hud::Readable(data,size_t(n)*8)))return;
    count=0;
    for(int32_t i=0;i<n;++i)Observe(entries[count++],data[i]);
    if(auto menu=original_ui_input_mode::FocusWidget())Observe(entries[count++],menu);
    valid=true;
}
inline void WriteDiagnostics(const wchar_t* path){
    FILE* file=nullptr;if(!enabled||!path||_wfopen_s(&file,path,L"wb")||!file)return;
    const auto b=[](bool x){return x?"true":"false";};
    std::fprintf(file,"{\"pid\":%lu,\"passive\":true,\"valid\":%s,\"samples\":%llu,\"sampled_at_ms\":%llu,\"foreground_owned\":%s,\"desktop_cursor\":[%ld,%ld],\"client_cursor\":[%ld,%ld],\"native_mouse_query\":%s,\"native_mouse_valid\":%s,\"native_mouse\":[%.6g,%.6g],",GetCurrentProcessId(),b(valid),static_cast<unsigned long long>(samples),static_cast<unsigned long long>(sampledAt),b(windowOwned),desktopMouse.x,desktopMouse.y,clientMouse.x,clientMouse.y,b(mouseQueried),b(nativeMouse.valid!=0),double(nativeMouse.x),double(nativeMouse.y));
    std::fprintf(file,"\"controller_world\":\"%p\",\"context_viewport\":\"%p\",\"player_viewport\":\"%p\",\"scene_viewport\":\"%p\",\"scene_table_rva\":%llu,\"scene_mouse_getter_rva\":%llu,\"input_mode_locked\":%u,\"viewport_ignore_input\":%u,\"viewport_hide_cursor_during_capture\":%u,\"viewport_mouse_lock_mode\":%u,\"viewport_widget_weak\":\"%p\",\"viewport_widget_control\":\"%p\",\"roots\":[",controllerWorld,contextViewport,playerViewport,sceneViewport,static_cast<unsigned long long>(Rva(sceneTable)),static_cast<unsigned long long>(Rva(sceneMouseGetter)),unsigned(inputLock),unsigned(ignoreInput),unsigned(hideCursor),unsigned(mouseLock),viewportWidgetWeak,viewportWidgetControl);
    for(size_t i=0;i<count;++i){const auto& e=entries[i];
        std::fprintf(file,"%s{\"name\":\"%s\",\"object\":\"%p\",\"owner\":\"%p\",\"visibility\":%u,\"enabled\":%s,\"hovered\":%s,\"captured\":%s,\"focused\":%s,\"focused_descendants\":%s}",i?",":"",e.name,e.object,e.owner,unsigned(e.visibility),b(e.enabled),b(e.hovered),b(e.captured),b(e.focused),b(e.descendants));}
    std::fputs("]}\n",file);std::fclose(file);
}
}
