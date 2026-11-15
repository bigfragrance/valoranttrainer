#pragma once
#include "original_players.h"
#include "local_server_mode_startup.h"
#include "original_inventory_slots.h"
#include "original_scope.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

// Actual host-only native login construction adaptation. Login/admission,
// SpectatorOnly/class selection, InitNewPlayer, SetPlayer, connection binding
// and client class defaults remain original. Only a verified remote login's
// deferred common PC factory suppresses this host instance's local UI creation
// before the original FinishSpawning/PostInitializeComponents. No CDO/image,
// local-player, role, admission result or client-controller edit is performed.
// Full finite source/ABI proof: work/local-server-playable/runtime-assembly/login.
namespace local_server_login {
using Vec=engine::Vec;
using Weak=original_inventory_slots::WeakIdentity;
using LoginFn=void*(*)(void* mode,void* player,int32_t role,const void* portal,const void* options,const void* uniqueId,void* error);
using CommonFn=void*(*)(void* mode,int32_t role,const Vec* location,const Vec* rotation,void* controllerClass);
inline constexpr size_t LoginSlot=0x898/8,CommonSlot=0x968/8;
inline constexpr uintptr_t LoginRva=0x43d1ac0,CommonRva=0x43d2020;
struct Hooks {void (*requestStop)()=nullptr;};
struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
inline constexpr Profile Profiles[]{
    {0x43d1ac0,645,0xc92a0b7fcd1d3c34ULL},
    {0x43d1d50,616,0xecac4526da485122ULL},
    {0x43d1fc0,37,0xc25cc9c92c8b90b4ULL},
    {0x43d1ff0,37,0xcaf15dfa031be41bULL},
    {0x43d2020,657,0x297e1879999a4b4fULL},
    {0x462e1d0,5240,0x2b1e5e873bb82824ULL},
    {0x3ef6cb0,5560,0xbe0c00750310708dULL},
    {0x19a51f0,331,0x220f8fcdbfd89aefULL},
    {0x18a31a0,44,0x07552eabec4054eaULL},
    {0x3c0a200,180,0x6b1e25340cdd023dULL},
    {0x7817400,3708,0x37e633cc8a327421ULL},
    {0x78b0c80,104,0x53d0f9d6c7ede8a5ULL},
    {0x3f02b70,2509,0x45566408b6b91869ULL},
};
struct NativeTable {uintptr_t rva;size_t entries;};
inline constexpr NativeTable Tables[]{{0xab46338,302},{0xab45410,329}};
struct Array {void** data=nullptr;int32_t count=0,capacity=0;};
struct Name {uint32_t words[3]{};};
// Native657-byte common factory reserves exactly this aligned carrier before
// its FTransform. Only the default-empty TFunction header/heap pointer are
// initialized by the native body; its unused inline storage is zero here.
struct alignas(16) SpawnParameters {uint8_t bytes[0x80]{};};
static_assert(sizeof(Array)==16&&sizeof(Name)==12&&sizeof(SpawnParameters)==0x80&&alignof(SpawnParameters)==16);
struct State {
    bool initialized=false,enabled=false,installed=false,closed=false,profilesVerified=false,restartRequired=false;
    bool modeTableReadOnly=false,otherVirtualsPreserved=false,bootstrapPreserved=false,cdoPreserved=false;
    bool inLogin=false,inCommon=false,stopNotified=false,remoteUIFlagNonReplicated=false;
    bool nativeModeStartup=false,noLocalBootstrap=false;
    bool deferredBeforeFinish=false,finishedAfterPolicy=false,nativePlayerStateCreated=false;
    bool nativeLoginAccepted=false,nativeSetPlayerStillOutside=true;
    unsigned error=0;DWORD gameThread=0;size_t tableEntries=0;
    uintptr_t tableRva=0,profileFailureRva=0,exceptionRva=0;
    uint64_t polls=0,loginCalls=0,scopedLogins=0,nativeRejections=0,forwardedLogins=0;
    uint64_t commonCalls=0,deferredFactories=0,forwardedFactories=0,uiPolicies=0,finishCalls=0,quaternionCalls=0;
    uint64_t fullAudits=0,lastAudit=0;
    int netMode=-1;int32_t lastRole=-1;
    uint8_t cdoUIFlag=255,instanceUIBefore=255,instanceUIAfter=255;
    void *world=nullptr,*driver=nullptr,*mode=nullptr,*gameInstance=nullptr,*bootstrapPlayer=nullptr,*bootstrapController=nullptr;
    void *controllerClass=nullptr,*classDefault=nullptr,*classUIManager=nullptr,*classUIDirector=nullptr;
    void *lastConnection=nullptr,*lastController=nullptr,*lastPlayerState=nullptr,*unfinishedController=nullptr;
    Weak worldId{},driverId{},modeId{},bootstrapPlayerId{},bootstrapControllerId{},lastControllerId{};
    const char* stage="local_login_not_initialized";
};
struct Context {
    bool active=false,intercepted=false;int32_t role=0;
    void* connection=nullptr;void* controller=nullptr;Weak connectionId{};
};
inline State state;
inline Context context;
inline Hooks hooks;
inline void **parentTable=nullptr,**privateTable=nullptr,**tableAllocation=nullptr;
inline LoginFn originalLogin=nullptr;
inline CommonFn originalCommon=nullptr;
inline void* playerControllerClass=nullptr;
inline const State& GetState(){return state;}
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Live(void* object,const Weak& id){return object&&original_inventory_slots::ResolveIdentity(id)==object;}
inline bool Identity(void* object,Weak& id){return original_inventory_slots::CaptureIdentity(object,id)&&Live(object,id);}
inline bool Fail(unsigned error,const char* why){
    if(!state.error){state.error=error?error:4899;state.stage=why;}state.restartRequired=true;
    if(!state.stopNotified&&hooks.requestStop){state.stopNotified=true;hooks.requestStop();}return false;
}
inline bool Thread(){return state.initialized&&!state.closed&&state.gameThread==GetCurrentThreadId()&&original_players::Thread();}
inline bool VerifyProfiles(){
    for(const auto& p:Profiles){
        const auto bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes)){state.profileFailureRva=p.rva;return Fail(4850,"local_login_native_profile_unreadable");}
        uint64_t hash=0xcbf29ce484222325ULL;
        for(size_t n=0;n<p.bytes;++n)hash=(hash^bytes[n])*0x100000001b3ULL;
        if(hash!=p.fnv){state.profileFailureRva=p.rva;return Fail(4851,"local_login_native_profile_changed");}
    }state.profilesVerified=true;return true;
}
inline bool Bootstrap(){
    const auto locals=Read<Array>(state.gameInstance,0x40);
    if(state.noLocalBootstrap){
        state.bootstrapPreserved=state.nativeModeStartup&&!state.bootstrapPlayer&&!state.bootstrapController
            &&locals.count==0&&locals.capacity>=0&&Read<void*>(state.world,0x1d8)==state.gameInstance
            &&local_server_mode_startup::ReadyForAdoption(state.world,nullptr);
        return state.bootstrapPreserved||Fail(4852,"local_login_actual_dedicated_no_local_player_context_changed");
    }
    state.bootstrapPreserved=Live(state.bootstrapPlayer,state.bootstrapPlayerId)&&Live(state.bootstrapController,state.bootstrapControllerId)
        &&Read<void*>(state.world,0x1d8)==state.gameInstance&&locals.count==1&&locals.capacity>=1&&locals.data
        &&Read<void*>(locals.data)==state.bootstrapPlayer&&Read<void*>(state.bootstrapPlayer,0x38)==state.bootstrapController
        &&Read<void*>(state.bootstrapController,0x510)==state.bootstrapPlayer;
    return state.bootstrapPreserved||Fail(4852,"local_login_actual_host_bootstrap_local_player_changed");
}
inline bool ClassDefaults(){
    state.cdoPreserved=Read<void*>(state.controllerClass,0x120)==state.classDefault
        &&Read<uint8_t>(state.classDefault,0xc28)==state.cdoUIFlag
        &&Read<void*>(state.classDefault,0xc18)==state.classUIManager&&Read<void*>(state.classDefault,0xc20)==state.classUIDirector;
    return state.cdoPreserved||Fail(4853,"local_login_original_blueprint_ui_defaults_changed");
}
inline bool OwnMode(void* mode){
    return Thread()&&!state.error&&mode==state.mode&&Live(mode,state.modeId)&&Live(state.world,state.worldId)
        &&Live(state.driver,state.driverId)&&Read<void**>(mode)==privateTable
        &&Read<void*>(state.world,0x170)==mode&&Read<void*>(state.world,0x40)==state.driver
        &&Read<void*>(state.driver,0x178)==state.world&&!Read<void*>(state.driver,0xc0)&&Bootstrap()&&ClassDefaults();
}
inline bool Connection(void* player){
    if(!original_players::IsA(player,0x3dea9c0)||Read<void*>(player,0x60)!=state.driver
       ||Read<uint8_t>(player,0x13c)!=3||player==state.bootstrapPlayer)return false;
    const auto connections=Read<Array>(state.driver,0xc8);
    return connections.count==1&&connections.capacity>=1&&connections.data&&Read<void*>(connections.data)==player;
}
inline void* CastControllerClass(void* type){
    // Same native UClass child relation that43d2020 tests via3c0a200's
    // generated PlayerController class. No class or default is rewritten.
    return type&&original_players::Derived(type,playerControllerClass)?type:nullptr;
}
inline bool UIProperty(void* pc){
    Name wanted{};engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&wanted,L"bCreateUIInstancesFromGlobals",1);
    auto field=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(Read<void*>(pc,0x10),&wanted,3);
    if(!field||Read<uint64_t>(field,0x20)!=Read<uint64_t>(&wanted)
       ||Read<int32_t>(field,0x44)!=0xc28||Read<int32_t>(field,0x30)!=1||Read<int32_t>(field,0x34)!=1
       ||Read<uint8_t>(field,0x78)!=1||Read<uint8_t>(field,0x79)!=0
       ||Read<uint8_t>(field,0x7a)!=1||Read<uint8_t>(field,0x7b)!=0xff)
        return Fail(4854,"local_login_original_ui_native_bool_layout_changed");
    state.remoteUIFlagNonReplicated=(Read<uint64_t>(field,0x38)&0x20ULL)==0;
    return state.remoteUIFlagNonReplicated||Fail(4855,"local_login_ui_factory_flag_unexpectedly_replicated");
}
inline bool BeforeFinish(void* pc){
    if(!context.active||!OwnMode(state.mode)||!Live(context.connection,context.connectionId)||!Connection(context.connection)
       ||!original_players::IsA(pc,original_players::ShooterControllerClass)||pc==state.bootstrapController
       ||pc==state.classDefault||(Read<uint32_t>(pc,8)&0x30U)!=0
       ||Read<uint8_t>(pc,0x224)!=3||Read<void*>(pc,0x510)
       ||Read<void*>(pc,0xc18)||Read<void*>(pc,0xc20)||!UIProperty(pc))
        return state.error?false:Fail(4856,"local_login_remote_instance_policy_must_precede_native_ui_factory");
    state.deferredBeforeFinish=(Read<uint8_t>(pc,0x11b)&2U)==0;
    if(!state.deferredBeforeFinish)return Fail(4857,"local_login_original_actor_already_finished_before_policy");
    state.instanceUIBefore=Read<uint8_t>(pc,0xc28);
    if(state.instanceUIBefore>1)return Fail(4854,"local_login_original_ui_native_bool_layout_changed");
    if(!original_players::Write<uint8_t>(pc,0xc28,0)||Read<uint8_t>(pc,0xc28)!=0)
        return Fail(4858,"local_login_remote_instance_ui_policy_readback_failed");
    state.instanceUIAfter=0;++state.uiPolicies;return ClassDefaults()&&Bootstrap();
}
inline bool Quaternion(const Vec* rotation,double* output){
    const auto result=engine::function<void*(*)(const Vec*,void*)>(0x19a51f0)(rotation,output);++state.quaternionCalls;
    return result==output&&std::isfinite(output[0])&&std::isfinite(output[1])&&std::isfinite(output[2])&&std::isfinite(output[3]);
}
inline void* CommonImpl(void* mode,int32_t role,const Vec* location,const Vec* rotation,void* type){
    ++state.commonCalls;
    if(!context.active||mode!=state.mode||role!=2||!original_players::Derived(type,engine::Class(original_players::ShooterControllerClass))){
        ++state.forwardedFactories;return originalCommon?originalCommon(mode,role,location,rotation,type):nullptr;
    }
    if(state.inCommon||context.intercepted||context.role!=role||!OwnMode(mode)
       ||!Live(context.connection,context.connectionId)||!Connection(context.connection)){
        if(!state.error)Fail(4859,"local_login_common_factory_outside_single_owned_native_login");return nullptr;
    }
    if(type!=Read<void*>(mode,0x488)&&type!=Read<void*>(mode,0x4c0)){
        Fail(4860,"local_login_common_class_not_selected_by_original_native_route");return nullptr;
    }
    if(!location||!rotation||!original_players::Readable(location,sizeof(Vec))||!original_players::Readable(rotation,sizeof(Vec))
       ||!original_players::Finite(*location)||!original_players::Finite(*rotation)){
        Fail(4861,"local_login_original_location_or_rotation_invalid");return nullptr;
    }
    state.inCommon=true;context.intercepted=true;state.lastRole=role;
    auto getWorld=Read<void*(*)(void*)>(Read<void**>(mode),0x1b8);
    const auto nativeWorld=getWorld?getWorld(mode):nullptr;
    if(nativeWorld!=state.world){state.inCommon=false;Fail(4869,"local_login_original_mode_world_getter_changed");return nullptr;}
    SpawnParameters parameters;
    original_players::Put(parameters.bytes,0x20,Read<void*>(mode,0x278));
    original_players::Put<uint16_t>(parameters.bytes,0x38,0x0100);
    original_players::Put<uint8_t>(parameters.bytes,0x3a,4);
    original_players::Put<uint32_t>(parameters.bytes,0x3c,0x48);
    engine::Transform initial;initial.translation=*location;
    if(!Quaternion(rotation,initial.rotation)){state.inCommon=false;Fail(4862,"local_login_original_quaternion_constructor_failed");return nullptr;}
    auto pc=engine::function<void*(*)(void*,void*,const engine::Transform*,const void*)>(0x462e1d0)
        (nativeWorld,CastControllerClass(type),&initial,&parameters);
    ++state.deferredFactories;state.unfinishedController=pc;
    if(pc){
        context.controller=pc;
        //43d2020's role==1 cached-local write is intentionally not reached in
        // this role==2 path. All local-role calls forward to the original body.
        const uint8_t replay=CastControllerClass(type)==CastControllerClass(Read<void*>(mode,0x4c0))?1:0;
        if(!original_players::Write<uint8_t>(pc,0x503,replay)||!BeforeFinish(pc)){
            if(!state.error)Fail(4863,"local_login_original_replay_flag_publication_failed");
            engine::function<void(*)(void*)>(0x18a31a0)(parameters.bytes+0x40);state.inCommon=false;return nullptr;
        }
        engine::Transform finalTransform;finalTransform.translation=*location;
        if(!Quaternion(rotation,finalTransform.rotation)){
            engine::function<void(*)(void*)>(0x18a31a0)(parameters.bytes+0x40);state.inCommon=false;
            Fail(4862,"local_login_original_quaternion_constructor_failed");return nullptr;
        }
        ++state.finishCalls;
        engine::function<void(*)(void*,const engine::Transform*,bool,void*,uint8_t)>(0x3ef6cb0)(pc,&finalTransform,false,nullptr,1);
        state.unfinishedController=nullptr;state.finishedAfterPolicy=(Read<uint8_t>(pc,0x11b)&2U)!=0;
        auto ps=Read<void*>(pc,0x470);
        state.nativePlayerStateCreated=original_players::IsA(ps,original_players::ShooterStateClass)&&Read<void*>(ps,0x210)==pc;
        if(!state.finishedAfterPolicy||Read<uint8_t>(pc,0xc28)||Read<void*>(pc,0xc18)||Read<void*>(pc,0xc20)
           ||!state.nativePlayerStateCreated||!ClassDefaults()||!Bootstrap()||!Identity(pc,state.lastControllerId)){
            if(!state.error)Fail(4863,"local_login_original_finish_or_native_state_ui_ownership_readback_failed");
            engine::function<void(*)(void*)>(0x18a31a0)(parameters.bytes+0x40);state.inCommon=false;return nullptr;
        }
        state.lastController=pc;state.lastPlayerState=ps;
    }
    // Preserve the exact native empty custom-spawn-function destructor call.
    engine::function<void(*)(void*)>(0x18a31a0)(parameters.bytes+0x40);
    state.inCommon=false;return pc;
}
inline LONG Capture(EXCEPTION_POINTERS* exception){
    const auto code=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0004860U;
    const auto at=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;state.inCommon=false;state.inLogin=false;context={};
    Fail(code,"local_login_native_exception_requires_new_owned_session");return EXCEPTION_EXECUTE_HANDLER;
}
inline void* Common(void* mode,int32_t role,const Vec* location,const Vec* rotation,void* type){
    __try{return CommonImpl(mode,role,location,rotation,type);}__except(Capture(GetExceptionInformation())){return nullptr;}
}
inline void* LoginImpl(void* mode,void* player,int32_t role,const void* portal,const void* options,const void* uniqueId,void* error){
    ++state.loginCalls;
    // Actual native local/non-connection paths retain their full original body
    // and defaults. Opaque options/portal/UniqueId/error are never inspected.
    if(role!=2||!original_players::IsA(player,0x3dea9c0)){
        ++state.forwardedLogins;return originalLogin?originalLogin(mode,player,role,portal,options,uniqueId,error):nullptr;
    }
    if(context.active||state.inLogin||!OwnMode(mode)||!Connection(player)){
        if(!state.error)Fail(4864,"local_login_remote_native_login_owner_or_scope_invalid");return nullptr;
    }
    context={};context.active=true;context.role=role;context.connection=player;
    if(!Identity(player,context.connectionId)){context={};Fail(4865,"local_login_original_connection_weak_identity_missing");return nullptr;}
    state.inLogin=true;++state.scopedLogins;state.lastConnection=player;
    // EXACTLY one unchanged native Login. Original GameSession admission,
    // v8c0 options route, v928 InitNewPlayer and every native failure remain.
    auto pc=originalLogin(mode,player,role,portal,options,uniqueId,error);
    const bool intercepted=context.intercepted;const auto expected=context.controller;
    context={};state.inLogin=false;
    if(!pc){++state.nativeRejections;if(!state.error)state.stage="local_login_original_native_login_returned_null";return nullptr;}
    if(intercepted){
        state.nativeSetPlayerStillOutside=Read<void*>(pc,0x510)==nullptr;
        if(pc!=expected||pc!=state.lastController||!Live(pc,state.lastControllerId)||!state.nativeSetPlayerStillOutside
           ||Read<void*>(pc,0xc18)||Read<void*>(pc,0xc20)||!Bootstrap()||!ClassDefaults()){
            if(!state.error)Fail(4866,"local_login_native_return_changed_remote_instance_or_local_ownership");return nullptr;
        }
    }
    state.nativeLoginAccepted=true;state.stage="local_login_actual_native_login_returned_remote_pc_without_host_ui";return pc;
}
inline void* Login(void* mode,void* player,int32_t role,const void* portal,const void* options,const void* uniqueId,void* error){
    __try{return LoginImpl(mode,player,role,portal,options,uniqueId,error);}__except(Capture(GetExceptionInformation())){return nullptr;}
}
inline bool Audit(){
    if(!Thread()||!state.installed||Read<void**>(state.mode)!=privateTable||!Live(state.mode,state.modeId))return Fail(4867,"local_login_owned_game_mode_table_changed");
    for(size_t n=0;n<state.tableEntries;++n){const auto expected=n==LoginSlot?reinterpret_cast<void*>(&Login):n==CommonSlot?reinterpret_cast<void*>(&Common):Read<void*>(parentTable,n*8);
        if(Read<void*>(privateTable,n*8)!=expected)return Fail(4868,"local_login_unrelated_game_mode_virtual_changed");}
    if(!state.nativeModeStartup&&Read<void*>(privateTable-1)!=Read<void*>(parentTable-1))return Fail(4868,"local_login_original_game_mode_rtti_changed");
    if(state.nativeModeStartup&&!local_server_mode_startup::OwnsModeTable(state.world,state.mode,privateTable))
        return Fail(4868,"local_login_owned_startup_347_table_composition_changed");
    state.otherVirtualsPreserved=true;++state.fullAudits;state.lastAudit=GetTickCount64();return Bootstrap()&&ClassDefaults();
}
inline bool InstallImpl(void* world,void* driver,const Hooks& callbacks){
    if(state.initialized)return state.world==world&&state.driver==driver&&Thread()&&!state.error;
    if(state.closed||!world||!driver||!engine::image||!original_players::Thread())return false;
    // Existing base-PC practice remains exactly the proven native path.
    if(!original_players::options.controllerClassPath){state.stage="local_login_native_base_practice_path_unchanged";return true;}
    hooks=callbacks;state.world=world;state.driver=driver;state.mode=Read<void*>(world,0x170);state.gameInstance=Read<void*>(world,0x1d8);
    if(world!=original_players::world||Read<void*>(world,0x40)!=driver||Read<void*>(driver,0x178)!=world||Read<void*>(driver,0xc0)
       ||!state.mode||Read<uint8_t>(state.mode,0x224)!=3)return Fail(4869,"local_login_actual_host_world_driver_or_mode_missing");
    if(Read<Array>(driver,0xc8).count!=0)return Fail(4880,"local_login_hook_must_precede_first_native_connection");
    state.netMode=engine::function<int(*)(void*)>(0x3efc820)(state.mode);
    if(state.netMode!=1&&state.netMode!=2)return Fail(4870,"local_login_requires_original_authority_net_mode");
    state.controllerClass=engine::Find(original_players::options.controllerClassPath);
    playerControllerClass=engine::Class(0x3c0a200);
    if(!state.controllerClass||Read<void*>(state.mode,0x488)!=state.controllerClass
       ||!original_players::Derived(state.controllerClass,engine::Class(original_players::ShooterControllerClass)))
        return Fail(4871,"local_login_original_configured_blueprint_controller_not_resolved");
    state.classDefault=engine::DefaultObject(state.controllerClass);state.cdoUIFlag=Read<uint8_t>(state.classDefault,0xc28);
    state.classUIManager=Read<void*>(state.classDefault,0xc18);state.classUIDirector=Read<void*>(state.classDefault,0xc20);
    if(!state.classDefault||state.cdoUIFlag>1)return Fail(4872,"local_login_original_controller_default_missing");
    state.nativeModeStartup=local_server_mode_startup::GetState().configured
        &&local_server_mode_startup::GetState().world==world&&local_server_mode_startup::GetState().ready;
    state.noLocalBootstrap=state.nativeModeStartup&&local_server_mode_startup::options.noLocalPlayerBootstrap!=0;
    const auto locals=Read<Array>(state.gameInstance,0x40);
    if(state.noLocalBootstrap){
        if(locals.count!=0||!local_server_mode_startup::ReadyForAdoption(world,nullptr))
            return Fail(4852,"local_login_dedicated_bootstrap_must_have_no_local_player");
    }else{
        if(locals.count!=1||locals.capacity<1||!locals.data)return Fail(4852,"local_login_actual_host_bootstrap_local_player_changed");
        state.bootstrapPlayer=Read<void*>(locals.data);state.bootstrapController=Read<void*>(state.bootstrapPlayer,0x38);
    }
    if(!Identity(world,state.worldId)||!Identity(driver,state.driverId)||!Identity(state.mode,state.modeId)
       ||(!state.noLocalBootstrap&&(!Identity(state.bootstrapPlayer,state.bootstrapPlayerId)||!Identity(state.bootstrapController,state.bootstrapControllerId))))
        return Fail(4873,"local_login_original_owned_object_weak_identity_missing");
    state.gameThread=GetCurrentThreadId();state.initialized=true;
    if(!Bootstrap()||!ClassDefaults()||!VerifyProfiles())return false;
    parentTable=Read<void**>(state.mode);
    for(const auto& candidate:Tables)if(parentTable==reinterpret_cast<void**>(engine::image+candidate.rva)){
        state.tableRva=candidate.rva;state.tableEntries=candidate.entries;break;
    }
    if(state.nativeModeStartup){
        if(parentTable!=local_server_mode_startup::GetState().table
           ||!local_server_mode_startup::OwnsModeTable(world,state.mode,parentTable))
            return Fail(4874,"local_login_parent_not_exact_owned_native_mode_startup_table");
        state.tableRva=0xb4fb520;state.tableEntries=347;
    }
    const size_t prefix=state.nativeModeStartup?0:1;
    const uintptr_t actualLoginRva=state.nativeModeStartup?0x71837b0:LoginRva;
    if(!state.tableEntries||!original_players::Readable(parentTable-prefix,(state.tableEntries+prefix)*8)
       ||Read<void*>(parentTable,LoginSlot*8)!=reinterpret_cast<void*>(engine::image+actualLoginRva)
       ||Read<void*>(parentTable,CommonSlot*8)!=reinterpret_cast<void*>(engine::image+CommonRva))
        return Fail(4874,"local_login_original_complete_game_mode_table_or_seam_changed");
    originalLogin=reinterpret_cast<LoginFn>(parentTable[LoginSlot]);originalCommon=reinterpret_cast<CommonFn>(parentTable[CommonSlot]);
    tableAllocation=static_cast<void**>(VirtualAlloc(nullptr,(state.tableEntries+prefix)*8,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!tableAllocation)return Fail(4875,"local_login_private_mode_table_allocation_failed");
    std::memcpy(tableAllocation,parentTable-prefix,(state.tableEntries+prefix)*8);privateTable=tableAllocation+prefix;
    privateTable[LoginSlot]=reinterpret_cast<void*>(&Login);privateTable[CommonSlot]=reinterpret_cast<void*>(&Common);
    DWORD previous=0;
    state.modeTableReadOnly=VirtualProtect(tableAllocation,(state.tableEntries+prefix)*8,PAGE_READONLY,&previous)!=0;
    if(!state.modeTableReadOnly)return Fail(4876,"local_login_private_mode_table_protection_failed");
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(state.mode),privateTable,parentTable)!=parentTable)
        return Fail(4877,"local_login_owned_mode_table_install_raced");
    state.installed=true;state.enabled=true;
    if(!Audit())return false;
    state.stage="local_login_configured_remote_native_factory_installed_runtime_unproven";return true;
}
inline bool Install(void* world,void* driver,const Hooks& callbacks={}){
    __try{return InstallImpl(world,driver,callbacks);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool Poll(){
    if(state.error)return false;if(!state.enabled)return !state.closed;
    if(!Thread())return Fail(4878,"local_login_audit_wrong_game_thread");++state.polls;
    if(GetTickCount64()-state.lastAudit<1000)return true;return Audit();
}
inline bool Close(){
    if(state.closed)return true;
    if(state.installed){
        if(!Thread()||state.inLogin||state.inCommon)return false;
        if(Live(state.mode,state.modeId)){
            if(Read<void**>(state.mode)!=privateTable)return Fail(4867,"local_login_owned_game_mode_table_changed");
            if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(state.mode),parentTable,privateTable)!=privateTable)
                return Fail(4879,"local_login_owned_mode_table_restore_raced");
        }
    }
    state.installed=false;state.enabled=false;state.closed=true;state.stage="local_login_closed";
    // As with other native observers, retain backing until owned process exit.
    return true;
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(file,"{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u,\"enabled\":%s,\"installed\":%s,\"profiles_verified\":%s,\"table_entries\":%zu,\"table_rva\":%llu,\"other_virtuals_preserved\":%s,\"bootstrap_preserved\":%s,\"cdo_preserved\":%s,\"ui_flag_non_replicated\":%s,\"deferred_before_finish\":%s,\"finished_after_policy\":%s,\"native_player_state_created\":%s,\"native_login_accepted\":%s,\"set_player_remains_original_outside_login\":%s,\"cdo_ui_flag\":%u,\"instance_ui_before\":%u,\"instance_ui_after\":%u,\"login_calls\":%llu,\"scoped_logins\":%llu,\"original_native_rejections\":%llu,\"common_calls\":%llu,\"deferred_factories\":%llu,\"forwarded_factories\":%llu,\"ui_policies\":%llu,\"finish_calls\":%llu,\"quaternion_calls\":%llu,\"full_audits\":%llu,\"unfinished_owned_controller\":%s,\"profile_failure_rva\":%llu,\"exception_rva\":%llu,\"client_defaults_edited\":false,\"local_player_transfers\":0,\"role_rewrites\":0,\"native_admission_bypassed\":false,\"integrity_failure_returns_null\":true,\"account_or_options_payloads_logged\":false,\"runtime_verified\":false,\"playable_verified\":false}",
        state.stage,state.error,b(state.enabled),b(state.installed),b(state.profilesVerified),state.tableEntries,static_cast<unsigned long long>(state.tableRva),
        b(state.otherVirtualsPreserved),b(state.bootstrapPreserved),b(state.cdoPreserved),b(state.remoteUIFlagNonReplicated),b(state.deferredBeforeFinish),b(state.finishedAfterPolicy),b(state.nativePlayerStateCreated),b(state.nativeLoginAccepted),b(state.nativeSetPlayerStillOutside),
        static_cast<unsigned>(state.cdoUIFlag),static_cast<unsigned>(state.instanceUIBefore),static_cast<unsigned>(state.instanceUIAfter),static_cast<unsigned long long>(state.loginCalls),static_cast<unsigned long long>(state.scopedLogins),static_cast<unsigned long long>(state.nativeRejections),static_cast<unsigned long long>(state.commonCalls),static_cast<unsigned long long>(state.deferredFactories),static_cast<unsigned long long>(state.forwardedFactories),static_cast<unsigned long long>(state.uiPolicies),static_cast<unsigned long long>(state.finishCalls),static_cast<unsigned long long>(state.quaternionCalls),static_cast<unsigned long long>(state.fullAudits),b(state.unfinishedController!=nullptr),static_cast<unsigned long long>(state.profileFailureRva),static_cast<unsigned long long>(state.exceptionRva));
    const bool okay=!std::ferror(file);std::fclose(file);return okay;
}
} // namespace local_server_login
