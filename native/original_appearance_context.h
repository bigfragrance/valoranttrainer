#pragma once
#include "original_scope.h"

// Original cosmetic preloading recursively visits character/weapon CDOs.
// Its whole-weapon collector70f0cf0 always reads PM.PregameManager+450, even
// in an offline world with no pregame match. Provide the actual original
// idle manager object, with its original ctor/defaults and GC-owned link.
// No pregame/network InitSystem, login/service call, state flag, or checker is
// replaced. Evidence: reports/all-skins-appearance-context.json.
namespace original_appearance_context {
inline constexpr uintptr_t PregameClass=0x6a974e0;
struct Diagnostics {
    void *client=nullptr,*preload=nullptr,*config=nullptr,*pregame=nullptr;
    original_effects::WeakIdentity identity{};
    unsigned checks=0,created=0,linked=0,error=0;
    unsigned onlineProviderPointers=0,retireChecks=0,retireWithPendingPreloads=0;
    int nativePendingPreloads=0;
    bool initialized=false,profileValid=false,originalConstructor=false;
    bool ownerMatches=false,strongReference=false,configMatches=false,idlePregame=false;
    const char* stage="original_appearance_context_not_initialized";
};
inline Diagnostics diagnostics;
inline const Diagnostics& State(){return diagnostics;}
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Fail(unsigned code,const char* stage){diagnostics.error=code;diagnostics.stage=stage;return false;}
inline bool Refresh(){
    auto& d=diagnostics;
    d.pregame=Read<void*>(d.preload,0x6b0);
    d.ownerMatches=original_players::IsA(d.pregame,PregameClass)&&Read<void*>(d.pregame,0x28)==d.client;
    d.strongReference=d.ownerMatches&&original_effects::ResolveIdentity(d.identity)==d.pregame;
    d.configMatches=Read<void*>(d.pregame,0xe8)==d.config;
    d.idlePregame=Read<uint8_t>(d.pregame,0x450)==0;
    d.onlineProviderPointers=0;
    constexpr size_t online[]{0xe0,0xf0,0xf8,0x100,0x108,0x110,0x118};
    for(auto field:online)if(Read<void*>(d.pregame,field))++d.onlineProviderPointers;
    // Original6eb8f50 increments this counter when scheduling an async
    // request; original6eb9610 decrements it AFTER its completion/recursion.
    // This is diagnostic only; no pending request or callback is suppressed.
    d.nativePendingPreloads=Read<int>(d.preload,0x204);
    return d.ownerMatches&&d.strongReference&&d.configMatches&&d.idlePregame;
}
inline bool Ensure(void* client,void* preload,void* config){
    if(original_players::gameThread!=GetCurrentThreadId()||!engine::image)
        return Fail(2480,"original_appearance_context_wrong_game_thread");
    auto& d=diagnostics;++d.checks;
    if(!original_players::IsA(client,0x6429080)||!original_players::IsA(preload,0x6aa5740)
        ||Read<void*>(preload,0x28)!=client||!original_players::IsA(config,0x66596c0))
        return Fail(2481,"original_appearance_context_original_service_identity_invalid");
    if(d.initialized&&(d.client!=client||d.preload!=preload||d.config!=config))
        return Fail(2482,"original_appearance_context_changed_requires_new_session");
    d.client=client;d.preload=preload;d.config=config;
    auto type=engine::Class(PregameClass);auto preloadType=Read<void*>(preload,0x10);
    // 6a98590 is the emitted no-register jmp alias for the same native class.
    // Use the actual full ctor6aa4ac0, never a base ctor or CDO as a provider.
    d.originalConstructor=Read<void*>(type,0xb8)==reinterpret_cast<void*>(engine::image+0x6aa4ac0);
    d.profileValid=d.originalConstructor&&Read<int>(type,0x60)==0x4a8
        &&original_scope::FieldOffset(preloadType,L"PregameManager")==0x6b0
        &&original_scope::FieldOffset(type,L"ConfigManager")==0xe8;
    if(!d.profileValid)return Fail(2483,"original_appearance_context_pinned_native_profile_changed");
    auto existing=Read<void*>(preload,0x6b0);
    if(d.initialized){
        if(existing!=d.pregame||!Refresh())return Fail(2484,"original_appearance_context_reference_or_idle_state_changed");
        return true;
    }
    if(existing&&(!original_players::IsA(existing,PregameClass)||Read<void*>(existing,0x28)!=client))
        return Fail(2485,"original_appearance_context_existing_pregame_has_other_owner");
    auto pregame=existing;
    if(!pregame){
        // Same original NewObject/SpawnObject factory as the validated local
        // PreloadManager/InventoryManager, Outer=the actual original ClientGI.
        // This invokes the complete original ctor, whose only callee is the
        // native UObject ctor1d140b0 and whose explicit450 initializer is zero.
        pregame=original_content_services::Construct(PregameClass,client);
        if(!original_players::IsA(pregame,PregameClass)||Read<void*>(pregame,0x28)!=client)
            return Fail(2486,"original_appearance_context_original_pregame_construction_failed");
        ++d.created;
    }
    // There is no online pregame match in this standalone local session. Its
    // original empty constructor state is a postcondition, not a value to set.
    if(Read<uint8_t>(pregame,0x450)!=0)
        return Fail(2487,"original_appearance_context_unexpected_active_pregame");
    auto oldConfig=Read<void*>(pregame,0xe8);
    if(oldConfig&&oldConfig!=config)return Fail(2488,"original_appearance_context_pregame_config_has_other_owner");
    if(!oldConfig&&!original_players::Write(pregame,0xe8,config))
        return Fail(2489,"original_appearance_context_original_config_link_failed");
    if(!original_effects::CaptureIdentity(pregame,d.identity))
        return Fail(2490,"original_appearance_context_pregame_weak_identity_failed");
    if(!existing){
        // Real reflected ObjectProperty: unlike Outer alone, this keeps the
        // manager reachable through the already-owned original PreloadManager.
        if(!original_players::Write(preload,0x6b0,pregame))
            return Fail(2491,"original_appearance_context_original_gc_link_failed");
        ++d.linked;
    }
    d.pregame=pregame;
    if(!Refresh())return Fail(2492,"original_appearance_context_original_readback_failed");
    if(d.created&&d.onlineProviderPointers)
        return Fail(2493,"original_appearance_context_new_pregame_has_unexpected_online_providers");
    d.initialized=true;d.error=0;d.stage="original_appearance_context_original_idle_pregame_ready";return true;
}
inline void ObserveRetirement(){
    if(!diagnostics.initialized)return;
    ++diagnostics.retireChecks;
    diagnostics.nativePendingPreloads=Read<int>(diagnostics.preload,0x204);
    if(diagnostics.nativePendingPreloads>0)++diagnostics.retireWithPendingPreloads;
}
inline void WriteJson(FILE* file){
    const auto& d=diagnostics;
    std::fprintf(file,"{\"stage\":\"%s\",\"error\":%u,\"initialized\":%d,\"checks\":%u,\"created\":%u,\"linked\":%u,\"profile_valid\":%d,\"original_constructor\":%d,\"pregame\":\"%p\",\"preload\":\"%p\",\"client\":\"%p\",\"owner_matches\":%d,\"strong_gc_reference\":%d,\"config_matches\":%d,\"original_idle_pregame\":%d,\"online_provider_pointers\":%u,\"native_pending_preloads\":%d,\"retire_checks\":%u,\"retire_with_pending_preloads\":%u,\"pregame_init_system_invoked\":false,\"pregame_state_or_ready_flags_written\":false,\"original_image_written\":false}",
        d.stage,d.error,d.initialized,d.checks,d.created,d.linked,d.profileValid,d.originalConstructor,d.pregame,d.preload,d.client,
        d.ownerMatches,d.strongReference,d.configMatches,d.idlePregame,d.onlineProviderPointers,d.nativePendingPreloads,d.retireChecks,d.retireWithPendingPreloads);
}
}
