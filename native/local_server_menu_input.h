#pragma once
#include "original_ui_input_mode.h"

// Observe only the real renderer window on its own game thread. A short press
// can begin and end between GetAsyncKeyState samples. Messages are forwarded
// once, unchanged, before a delayed fallback invokes the original close leaf.
namespace local_server_menu_input {
struct State {
    HWND window=nullptr;
    WNDPROC previous=nullptr;
    DWORD thread=0,error=0;
    unsigned dismissError=0;
    const char* dismissStage="none";
    bool escapeHeld=false,buyHeld=false,pending=false;
    bool pendingOptionsExit=false;
    uint8_t pendingKey=0;
    void* observedWidget=nullptr;
    void* pendingWidget=nullptr;
    void* observedEntry=nullptr;
    void* pendingEntry=nullptr;
    original_inventory_slots::WeakIdentity observedId{},pendingId{};
    original_inventory_slots::WeakIdentity observedEntryId{},pendingEntryId{};
    bool observedOpen=false,observedShop=false,observedOptions=false;
    ULONGLONG releasedAt=0;
    uint64_t installs=0,restores=0,forwarded=0,escapeEdges=0,buyEdges=0;
    uint64_t dismisses=0,nativeDismisses=0,focusCancels=0,destroyed=0;
    uint64_t keySelectionConsumes=0,keySelectionUnknown=0;
};
inline State state{};
inline bool Thread(){return original_players::gameThread&&original_players::gameThread==GetCurrentThreadId();}
inline void Cancel(){state.pending=false;state.pendingOptionsExit=false;state.pendingWidget=nullptr;state.pendingEntry=nullptr;state.pendingId={};state.pendingEntryId={};state.releasedAt=0;}
inline LRESULT CALLBACK WindowProcedure(HWND window,UINT message,WPARAM key,LPARAM flags){
    const auto previous=state.previous;
    const bool bound=window==state.window&&GetCurrentThreadId()==state.thread;
    const bool down=message==WM_KEYDOWN||message==WM_SYSKEYDOWN;
    const bool up=message==WM_KEYUP||message==WM_SYSKEYUP;
    const bool edge=down&&(static_cast<ULONG_PTR>(flags)&(ULONG_PTR(1)<<30))==0;
    bool closing=bound&&edge&&((key==VK_ESCAPE&&state.observedOpen)||(key=='B'&&state.observedShop));
    bool allowOptionsExit=false;
    void* const beforeWidget=state.observedWidget;
    void* const beforeEntry=state.observedEntry;
    const auto beforeId=state.observedId;
    const auto beforeEntryId=state.observedEntryId;
    // Preserve pre-dispatch binding capture: the real key selector can consume
    // Escape and clear capture before the next game tick. Only native read-only
    // getters run here; every window message is still forwarded exactly once.
    if(closing&&key==VK_ESCAPE&&state.observedOptions){
        bool complete=false,selecting=false;
        const bool queried=original_ui_input_mode::TopHasSelectingKey(beforeEntry,beforeWidget,complete,selecting);
        if(!queried||!complete||selecting){
            closing=false;Cancel();
            if(selecting)++state.keySelectionConsumes;else ++state.keySelectionUnknown;
        }else allowOptionsExit=true;
    }
    if(bound){
        if(key==VK_ESCAPE&&(down||up)){state.escapeHeld=down;if(edge)++state.escapeEdges;}
        if(key=='B'&&(down||up)){state.buyHeld=down;if(edge)++state.buyEdges;}
        if(message==WM_KILLFOCUS||(message==WM_ACTIVATEAPP&&!key)){
            state.escapeHeld=state.buyHeld=false;Cancel();++state.focusCancels;
        }
        ++state.forwarded;
    }
    const auto result=previous?CallWindowProcW(previous,window,message,key,flags):DefWindowProcW(window,message,key,flags);
    if(bound&&closing&&beforeWidget&&beforeEntry){
        state.pending=true;state.pendingKey=uint8_t(key);state.pendingWidget=beforeWidget;state.releasedAt=0;
        state.pendingId=beforeId;state.pendingEntry=beforeEntry;state.pendingEntryId=beforeEntryId;
        state.pendingOptionsExit=allowOptionsExit;
    }
    if(bound&&up&&state.pending&&key==state.pendingKey)state.releasedAt=GetTickCount64();
    if(bound&&message==WM_NCDESTROY){
        state.window=nullptr;state.previous=nullptr;Cancel();++state.destroyed;
    }
    return result;
}
inline bool OwnWindow(HWND window){
    if(!window||!IsWindow(window)||GetAncestor(window,GA_ROOT)!=window)return false;
    DWORD process=0;const auto thread=GetWindowThreadProcessId(window,&process);
    return process==GetCurrentProcessId()&&thread==GetCurrentThreadId();
}
inline bool OwnRenderer(HWND window){
    if(!OwnWindow(window)||!IsWindowVisible(window))return false;
    RECT bounds{};
    return GetClientRect(window,&bounds)&&bounds.right-bounds.left>=320&&bounds.bottom-bounds.top>=200;
}
inline BOOL CALLBACK FindWindow(HWND window,LPARAM result){
    if(!OwnRenderer(window))return TRUE;
    wchar_t title[64]{};GetWindowTextW(window,title,64);
    if(wcsncmp(title,L"VALORANT",8)!=0)return TRUE;
    *reinterpret_cast<HWND*>(result)=window;return FALSE;
}
inline bool Install(){
    if(!Thread())return false;
    if(state.window){
        if(!OwnWindow(state.window))return false;
        // Do not overwrite a later subscriber in the window's procedure chain.
        return GetWindowLongPtrW(state.window,GWLP_WNDPROC)==reinterpret_cast<LONG_PTR>(&WindowProcedure);
    }
    HWND window=nullptr;EnumThreadWindows(GetCurrentThreadId(),FindWindow,reinterpret_cast<LPARAM>(&window));
    if(!window)return true; // Renderer creation is asynchronous; retry next tick.
    const auto previous=reinterpret_cast<WNDPROC>(GetWindowLongPtrW(window,GWLP_WNDPROC));
    if(!previous||previous==&WindowProcedure)return false;
    state.window=window;state.previous=previous;state.thread=GetCurrentThreadId();
    SetLastError(0);
    const auto replaced=SetWindowLongPtrW(window,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(&WindowProcedure));
    if(!replaced||reinterpret_cast<WNDPROC>(replaced)!=previous){
        state.error=GetLastError();state.window=nullptr;state.previous=nullptr;return false;
    }
    ++state.installs;return true;
}
inline bool Poll(){
    if(!Install())return false;
    const auto& ui=original_ui_input_mode::State();
    const bool open=ui.optionsOpen||ui.shopOpen||ui.mapOpen||ui.externalObservedOpen;
    void* entry=nullptr;void* widget=nullptr;
    if(!original_ui_input_mode::ReadTopEntry(entry,widget))return false;
    if(state.pending){
        DWORD process=0;GetWindowThreadProcessId(GetForegroundWindow(),&process);
        if(process!=GetCurrentProcessId()){Cancel();++state.focusCancels;}
        else if(!open||widget!=state.pendingWidget||entry!=state.pendingEntry
            ||original_inventory_slots::ResolveIdentity(state.pendingId)!=widget
            ||original_inventory_slots::ResolveIdentity(state.pendingEntryId)!=entry){Cancel();++state.nativeDismisses;}
        else{
            const bool held=state.pendingKey==VK_ESCAPE?state.escapeHeld:state.buyHeld;
            const auto now=GetTickCount64();
            if(!held&&!state.releasedAt)state.releasedAt=now;
            if(state.releasedAt&&now-state.releasedAt>=100){
                void* const expectedEntry=state.pendingEntry;void* const expectedWidget=state.pendingWidget;
                const bool optionsExit=state.pendingOptionsExit;
                Cancel();
                state.dismissError=0;state.dismissStage="none";
                if(!original_ui_input_mode::DismissTopEntry(expectedEntry,expectedWidget,optionsExit)){
                    state.dismissError=original_ui_input_mode::State().error;
                    state.dismissStage=original_ui_input_mode::State().stage;return false;
                }
                ++state.dismisses;
            }
        }
    }
    const auto& after=original_ui_input_mode::State();
    state.observedOpen=after.optionsOpen||after.shopOpen||after.mapOpen||after.externalObservedOpen;
    state.observedOptions=after.optionsOpen;
    state.observedShop=after.shopOpen&&!after.optionsOpen&&!after.externalObservedOpen;
    if(!original_ui_input_mode::ReadTopEntry(state.observedEntry,state.observedWidget))return false;
    state.observedId={};state.observedEntryId={};
    if(state.observedWidget&&!original_inventory_slots::CaptureIdentity(state.observedWidget,state.observedId))return false;
    if(state.observedEntry&&!original_inventory_slots::CaptureIdentity(state.observedEntry,state.observedEntryId))return false;
    return true;
}
inline bool Close(){
    if(!Thread())return false;
    Cancel();state.observedOpen=state.observedShop=state.observedOptions=false;state.observedWidget=nullptr;state.observedEntry=nullptr;
    if(!state.window)return true;
    if(!IsWindow(state.window)){state.window=nullptr;state.previous=nullptr;return true;}
    if(GetWindowLongPtrW(state.window,GWLP_WNDPROC)!=reinterpret_cast<LONG_PTR>(&WindowProcedure))return false;
    SetLastError(0);
    if(!SetWindowLongPtrW(state.window,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(state.previous))){state.error=GetLastError();return false;}
    state.window=nullptr;state.previous=nullptr;++state.restores;return true;
}
inline void Write(FILE* file){
    const auto& s=state;
    const auto& commit=original_input_bindings::settingsCommit;
    std::fprintf(file,",\"settings_commit_context\":{\"ready\":%s,\"profile_verified\":%s,\"constructions\":%u,\"checks\":%u,\"dismiss_error\":%u,\"dismiss_stage\":\"%s\"}",
        commit.ready?"true":"false",commit.profileVerified?"true":"false",commit.constructions,commit.checks,s.dismissError,s.dismissStage);
    std::fprintf(file,",\"window_menu_input\":{\"installed\":%s,\"error\":%lu,\"installs\":%llu,\"restores\":%llu,\"forwarded\":%llu,\"escape_edges\":%llu,\"buy_edges\":%llu,\"dismisses\":%llu,\"native_dismisses\":%llu,\"focus_cancels\":%llu,\"destroyed\":%llu,\"pending\":%s,\"synthetic_inputs\":0}",
        s.window?"true":"false",s.error,s.installs,s.restores,s.forwarded,s.escapeEdges,s.buyEdges,s.dismisses,s.nativeDismisses,s.focusCancels,s.destroyed,s.pending?"true":"false");
    std::fprintf(file,",\"window_key_selection_consumes\":%llu,\"window_key_selection_unknown\":%llu",s.keySelectionConsumes,s.keySelectionUnknown);
    const auto capture=original_ui_input_mode::WindowSelectionSnapshot();
    std::fprintf(file,",\"window_selection_hidden_subtrees\":%u,\"window_selection_visibility_queries\":%u",capture.skippedHidden,capture.visibilityQueries);
    std::fprintf(file,",\"window_selection_probe\":{\"stage\":\"%s\",\"samples\":%llu,\"visited\":%u,\"queued\":%u,\"selectors\":%u,\"anchors\":%u,\"combos\":%u,\"metadata_failure\":%u,\"complete\":%s,\"selecting\":%s,\"schema_ready\":%s,\"profile_ready\":%s,\"tree_offset\":%d,\"root_offset\":%d,\"child_index\":%d,\"failed_widget\":%llu,\"profile_rva\":%llu}",
        capture.stage,capture.samples,capture.visited,capture.queued,capture.selectors,capture.anchors,capture.combos,capture.metadataFailure,
        capture.complete?"true":"false",capture.selecting?"true":"false",capture.schemaReady?"true":"false",capture.profileReady?"true":"false",
        capture.treeOffset,capture.rootOffset,capture.childIndex,static_cast<unsigned long long>(capture.failedWidget),static_cast<unsigned long long>(capture.profileRva));
}
} // namespace local_server_menu_input
