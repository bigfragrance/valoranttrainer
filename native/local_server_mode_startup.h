#pragma once
#include "original_inventory_slots.h"
#include "local_server_mode_startup.generated.h"
#include "local_server_package_paths.h"
#include <array>
#include <cstdio>
#include <cwchar>
#include <cstring>

// WORK-ONLY candidate. Single owner must be the early-loaded bridge DLL.
// The GI installer calls its exported original-factory wrapper on the actual
// game thread; no UClass getter is called from the install/loader thread.
// Actual original class/factory/lifecycles remain native. Missing offline host
// dependency ordering is explicit; no component replay or rate/clock write.
namespace local_server_login {
inline void* Login(void*,void*,int32_t,const void*,const void*,const void*,void*);
inline void* Common(void*,int32_t,const engine::Vec*,const engine::Vec*,void*);
}
namespace local_server_mode_startup {
using Weak=original_inventory_slots::WeakIdentity;
using Factory=void*(*)(void*,void* urlValueCarrier,void* world);
using Init=void(*)(void*,const void*,const void*,void*);
using Simple=void(*)(void*);
using TickFunction=void(*)(void*,float);
inline constexpr size_t Entries=347,TickSlot=0x540/8,PreInitSlot=0x568/8,InitSlot=0x7c8/8;
inline constexpr size_t InitStateSlot=0x7d0/8,StartSlot=0x7f0/8,LoginSlot=0x898/8,CommonSlot=0x968/8;
inline constexpr uint64_t Magic=0x3154524154534d53ULL;
enum class Preparation:uint32_t{Failed,Waiting,Ready};
struct Options {
    uint64_t magic=Magic;uint32_t version=1,bytes=sizeof(Options),pid=0,primaryThread=0;
    uint64_t image=0;uint32_t host=1,bootstrapEngineClasses=1,allowStateInstanceOverride=0,timeoutMs=30000;
    uint32_t noLocalPlayerBootstrap=0,requireDedicated=0,reserved[2]{};
    wchar_t package[512]{},modeClass[512]{},gameStateClass[512]{},report[1024]{};
};
struct Source {void *world=nullptr,*gi=nullptr,*mode=nullptr,*gs=nullptr,*localPlayer=nullptr,*pc=nullptr;};
struct Hooks {
    // Only synchronous local constructor/reference prerequisites; no online GI
    // Init, login/account method, or ready-flag publication belongs here.
    bool (*beforeInit)(const Source&)=nullptr;
    Preparation (*preparePlay)(const Source&)=nullptr;
    bool (*started)(const Source&)=nullptr;
    void (*tick)(const Source&,float)=nullptr;
    void (*failed)(unsigned,const char*)=nullptr;
};
struct Array{void** data=nullptr;int32_t count=0,capacity=0;};
struct State {
    bool configured=false,profilesVerified=false,tablePublished=false,tableReadOnly=false;
    bool initEntered=false,initReturned=false,preInitEntered=false,preInitReturned=false;
    bool initStateEntered=false,initStateReturned=false,startRequested=false,startForwarded=false,startReturned=false;
    bool gsDispatchAttempted=false,gsDispatchReturned=false,ready=false,failed=false,busy=false;
    bool giObjectsReady=false,playServicesReady=false,modeTickObserved=false,selectedStateAdopted=false;
    bool sourceValid=false,bootstrapValid=false,registeredFixed=false,registeredTiming=false,noLocalPlayerObserved=false;
    bool instanceStateOverride=false,originalBpStateDefaultVerified=false,bootstrapClassesConfigured=false,originalTablePreserved=false;
    unsigned error=0;DWORD gameThread=0;uint64_t boundAt=0,readyAt=0;
    uint64_t factoryCalls=0,foreignWorldForwards=0,initCalls=0,preInitCalls=0,initStateCalls=0;
    uint64_t startRequests=0,startForwards=0,gsDispatchCalls=0,pumps=0,ticks=0,withheldTicks=0,parentTicks=0;
    void *world=nullptr,*gi=nullptr,*mode=nullptr,*gs=nullptr,*fixed=nullptr,*timing=nullptr,*megapacket=nullptr;
    void *selectedStateClass=nullptr,*selectedModeClass=nullptr,*modeCDO=nullptr,*cdoStateClass=nullptr;
    void *bootstrapPlayer=nullptr,*bootstrapPC=nullptr,*bootstrapPS=nullptr;
    void** table=nullptr;
    Weak worldId{},giId{},modeId{},gsId{},fixedId{},timingId{},megaId{},stateClassId{},modeClassId{},cdoId{},playerId{},pcId{},psId{};
    uint8_t modeLifecycle=0,gsLifecycle=0,fixedFlags=0,timingFlags=0,rateBefore=0,rateAfter=0;
    int32_t scheduleCount=-1,initErrorCharacters=-1,actorNetMode=-1,worldNetMode=-1;uintptr_t profileFailureRva=0,exceptionRva=0;
    uint32_t exceptionCode=0;const char* stage="mode_startup_not_configured";
};
inline Options options{};
inline Hooks hooks{};
inline State state{},cached{};
inline SRWLOCK lock=SRWLOCK_INIT;
inline void InitGame(void*,const void*,const void*,void*);
inline void PreInitialize(void*);
inline void InitGameState(void*);
inline void StartPlay(void*);
inline void Tick(void*,float);
template<class T> inline T Read(const void* p,size_t off=0){return original_players::Value<T>(p,off);}
inline bool Live(void* p,const Weak& id){return p&&original_inventory_slots::ResolveIdentity(id)==p;}
inline bool Capture(void* p,Weak& id){return original_inventory_slots::CaptureIdentity(p,id)&&Live(p,id);}
inline void Cache(){AcquireSRWLockExclusive(&lock);cached=state;ReleaseSRWLockExclusive(&lock);}
inline State Snapshot(){AcquireSRWLockShared(&lock);auto s=cached;ReleaseSRWLockShared(&lock);return s;}
inline const State& GetState(){return state;}
inline bool Thread(){return state.configured&&!state.failed&&GetCurrentThreadId()==options.primaryThread
    &&engine::image==options.image;}
inline bool Fail(unsigned error,const char* stage){
    if(!state.failed){state.failed=true;state.error=error;state.stage=stage;state.ready=false;
        Cache();if(hooks.failed)hooks.failed(error,stage);}Cache();return false;
}
inline bool Configure(const Options& value,const Hooks& callbacks){
    // Pure POD/configuration only. This API is safe at the verified early hold.
    if(state.configured||value.magic!=Magic||value.version!=1||value.bytes!=sizeof(Options)
       ||value.pid!=GetCurrentProcessId()||!value.primaryThread||value.image!=reinterpret_cast<uint64_t>(GetModuleHandleW(nullptr))
       ||value.host!=1||value.bootstrapEngineClasses>1||value.allowStateInstanceOverride>1
       ||value.noLocalPlayerBootstrap>1||value.requireDedicated>1||value.reserved[0]||value.reserved[1]
       ||(value.noLocalPlayerBootstrap&&(!value.requireDedicated||value.bootstrapEngineClasses))
       ||value.timeoutMs<1000||value.timeoutMs>120000||!value.package[0]||value.package[511]
       ||!value.modeClass[0]||value.modeClass[511]||!value.gameStateClass[0]||value.gameStateClass[511]
       ||!value.report[0]||value.report[1023]
       ||!local_server_package_paths::Absolute(local_server_package_paths::Normalize(value.report))
       ||!callbacks.beforeInit||!callbacks.preparePlay||!callbacks.started||!callbacks.tick||!callbacks.failed)return false;
    options=value;hooks=callbacks;engine::image=static_cast<uintptr_t>(value.image);
    state.configured=true;state.gameThread=value.primaryThread;state.stage="mode_startup_waiting_original_gi_factory";Cache();return true;
}
inline bool Profiles(){
    if(state.profilesVerified)return true;
    for(const auto& p:local_server_mode_startup_source::Profiles){
        const auto bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);uint64_t hash=0xcbf29ce484222325ULL;
        if(!original_players::Readable(bytes,p.bytes)){state.profileFailureRva=p.rva;return Fail(8700,"mode_startup_original_profile_unreadable");}
        for(size_t i=0;i<p.bytes;++i)hash=(hash^bytes[i])*0x100000001b3ULL;
        if(hash!=p.fnv){state.profileFailureRva=p.rva;return Fail(8700,"mode_startup_original_profile_mismatch");}
    }state.profilesVerified=true;return true;
}
inline void* WorldOf(void* actor){
    const auto f=Read<void*>(Read<void*>(actor),0x1b8);
    return f==reinterpret_cast<void*>(engine::image+0x3ed83b0)?reinterpret_cast<void*(*)(void*)>(f)(actor):nullptr;
}
inline bool ROPrivate(void* p,size_t bytes){
    MEMORY_BASIC_INFORMATION m{};return p&&VirtualQuery(p,&m,sizeof(m))&&m.State==MEM_COMMIT&&m.Type==MEM_PRIVATE
        &&(m.Protect&0xffU)==PAGE_READONLY&&reinterpret_cast<uintptr_t>(p)+bytes>=reinterpret_cast<uintptr_t>(p)
        &&reinterpret_cast<uintptr_t>(p)+bytes<=reinterpret_cast<uintptr_t>(m.BaseAddress)+m.RegionSize;
}
inline void* Expected(size_t slot,bool loginLayer){
    if(slot==TickSlot)return reinterpret_cast<void*>(&Tick);
    if(slot==PreInitSlot)return reinterpret_cast<void*>(&PreInitialize);
    if(slot==InitSlot)return reinterpret_cast<void*>(&InitGame);
    if(slot==InitStateSlot)return reinterpret_cast<void*>(&InitGameState);
    if(slot==StartSlot)return reinterpret_cast<void*>(&StartPlay);
    if(loginLayer&&slot==LoginSlot)return reinterpret_cast<void*>(&local_server_login::Login);
    if(loginLayer&&slot==CommonSlot)return reinterpret_cast<void*>(&local_server_login::Common);
    return reinterpret_cast<void*>(engine::image+local_server_mode_startup_source::Slots[slot]);
}
inline bool Table(void** table,bool loginLayer){
    if(!ROPrivate(table,Entries*8))return false;
    for(size_t n=0;n<Entries;++n)if(Read<void*>(table,n*8)!=Expected(n,loginLayer))return false;return true;
}
inline bool OwnsModeTable(void* world,void* mode,void** actual){
    if(!Thread()||!state.tablePublished||world!=state.world||mode!=state.mode||!Live(world,state.worldId)
       ||!Live(mode,state.modeId)||Read<void**>(mode)!=actual||Read<void*>(world,0x170)!=mode)return false;
    return actual==state.table?Table(actual,false):state.ready&&Table(actual,true);
}
inline bool SourceLive(bool requireModeLink=true){
    state.sourceValid=false;
    if(!Thread()||!Live(state.world,state.worldId)||!Live(state.gi,state.giId)||!Live(state.mode,state.modeId)
       ||!Live(state.selectedModeClass,state.modeClassId)||!Live(state.selectedStateClass,state.stateClassId)
       ||!Live(state.modeCDO,state.cdoId)||Read<void*>(state.world,0x1d8)!=state.gi||Read<void*>(state.mode,0x10)!=state.selectedModeClass
       ||Read<void*>(state.selectedModeClass,0x120)!=state.modeCDO||(Read<uint32_t>(state.mode,8)&0x30U)
       ||Read<uint8_t>(state.mode,0x224)!=3||WorldOf(state.mode)!=state.world
       ||Read<void*>(state.mode,0x480)!=state.selectedStateClass||(Read<uint8_t>(state.mode,0x119)&1)==0)return false;
    // Exact normal488 bootstrap route: the original native ctor cleared this
    // real Megapacket field. Never change it or replace spectator class4C0;
    // its other branch writes a Shooter-specific PC+DD8 outside Engine PC.
    if(options.bootstrapEngineClasses&&(!Live(state.megapacket,state.megaId)
       ||Read<void*>(state.mode,0xa40)!=state.megapacket||!original_players::IsA(state.megapacket,0x698a220)
       ||Read<void*>(state.megapacket,0xc8)!=state.mode||Read<void*>(state.megapacket,0x28)!=state.mode
       ||(!state.bootstrapPC&&Read<uint8_t>(state.megapacket,0x470)!=0)))return false;
    if(requireModeLink&&!OwnsModeTable(state.world,state.mode,Read<void**>(state.mode)))return false;
    state.actorNetMode=engine::function<int(*)(void*)>(0x3efc820)(state.mode);
    state.worldNetMode=engine::function<int(*)(void*)>(0x4ce37a0)(state.world);
    // True source observations only. This fixed client body returns3 when a
    // World driver exists; no flag/query replacement is used to conceal that
    // dedicated-startup gap. A no-LP candidate cannot call Listen a Dedicated.
    if(options.requireDedicated&&(state.actorNetMode!=1||state.worldNetMode!=1))
        return Fail(8712,"mode_startup_original_actor_and_world_not_true_dedicated_no_query_override");
    state.sourceValid=true;return true;
}
inline bool MatchesPackage(void* world){
    const auto expected=engine::Find(options.package);if(!expected)return false;
    auto current=world;for(size_t depth=0;current&&depth<16;++depth){
        if(current==expected)return true;current=Read<void*>(current,0x28);
    }return false;
}
inline bool ClassProperty(void* mode,const wchar_t* text,size_t offset,void* value,uintptr_t expectedBase){
    uint32_t name[3]{};engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(name,text,1);
    auto f=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(Read<void*>(mode,0x10),name,3);
    if(!f||Read<uint64_t>(f,0x20)!=Read<uint64_t>(name)||Read<int32_t>(f,0x30)!=1
       ||Read<int32_t>(f,0x34)!=8||Read<int32_t>(f,0x44)!=int32_t(offset)
       ||(Read<uint64_t>(Read<void*>(f,8),0x18)&0x8401ULL)!=0x8401ULL
       ||Read<void*>(f,0x80)!=engine::Class(expectedBase)
       ||(value&&(!original_players::IsA(value,0x1af0eb0)||!original_players::Derived(value,engine::Class(expectedBase)))))return false;
    return original_players::Write(mode,offset,value)&&Read<void*>(mode,offset)==value;
}
inline Source Current(){return {state.world,state.gi,state.mode,state.gs,state.bootstrapPlayer,state.bootstrapPC};}
inline bool Bootstrap(){
    state.bootstrapValid=false;const auto locals=Read<Array>(state.gi,0x40);
    if(options.noLocalPlayerBootstrap){
        if(!options.requireDedicated||state.actorNetMode!=1||state.worldNetMode!=1||locals.count!=0
           ||locals.capacity<0||state.bootstrapPlayer||state.bootstrapPC||state.bootstrapPS)return false;
        state.noLocalPlayerObserved=true;state.bootstrapValid=true;return true;
    }
    if(locals.count!=1||locals.capacity<1||!original_players::Readable(locals.data,8))return false;
    auto lp=Read<void*>(locals.data),pc=Read<void*>(lp,0x38),ps=Read<void*>(pc,0x470);
    if(!original_players::IsA(lp,0x3b6e680)||!original_players::IsA(pc,0x3c0a200)||!original_players::IsA(ps,0x42aae70)
       ||Read<void*>(pc,0x510)!=lp||Read<void*>(ps,0x210)!=pc||Read<uint8_t>(pc,0x224)!=3
       ||Read<uint8_t>(ps,0x224)!=3||WorldOf(pc)!=state.world||WorldOf(ps)!=state.world||Read<void*>(pc,0x4b0))return false;
    // BaseGameMode's unchanged ctor defaults are4C0=EnginePC/4C8=EnginePS.
    // The original common factory compares the selected PC class with4C0,
    // naturally setting503=1 for this normal488 bootstrap. InitPlayerState
    // consequently reads4C8. Observe that real choice; never clear503 or
    // change4C0/4C8 merely to force the other state-class branch.
    const auto actualPcClass=Read<void*>(pc,0x10);
    const auto replay=Read<uint8_t>(pc,0x503);
    const auto selectedState=Read<void*>(state.mode,replay?0x4c8:0x490);
    if(options.bootstrapEngineClasses&&(actualPcClass!=engine::Class(0x3c0a200)
       ||replay>1||replay!=(actualPcClass==Read<void*>(state.mode,0x4c0)?1:0)
       ||selectedState!=engine::Class(0x42aae70)||Read<void*>(ps,0x10)!=selectedState
       ||Read<void*>(state.mode,0x4c0)!=Read<void*>(state.modeCDO,0x4c0)
       ||Read<void*>(state.mode,0x4c8)!=Read<void*>(state.modeCDO,0x4c8)))return false;
    if(state.bootstrapPlayer){if(lp!=state.bootstrapPlayer||pc!=state.bootstrapPC||ps!=state.bootstrapPS
        ||!Live(lp,state.playerId)||!Live(pc,state.pcId)||!Live(ps,state.psId))return false;}
    else{
        if(!Capture(lp,state.playerId)||!Capture(pc,state.pcId)||!Capture(ps,state.psId))return false;
        state.bootstrapPlayer=lp;state.bootstrapPC=pc;state.bootstrapPS=ps;
    }state.bootstrapValid=true;return true;
}
inline bool GameStateLive(){
    if(!state.gs||!Live(state.gs,state.gsId)||Read<void*>(state.world,0x178)!=state.gs
       ||Read<void*>(state.mode,0x4d8)!=state.gs||Read<void*>(state.gs,0x470)!=state.mode
       ||Read<void*>(state.gs,0x10)!=state.selectedStateClass||Read<uint8_t>(state.gs,0x224)!=3
       ||WorldOf(state.gs)!=state.world)return false;
    return state.selectedStateAdopted=true;
}
inline bool Components(){
    if(!GameStateLive())return false;
    auto fixed=Read<void*>(state.gs,0xcc0),timing=Read<void*>(state.mode,0x868);
    if(!original_players::IsA(fixed,0x684d360)||!original_players::IsA(timing,0x6bdf750)
       ||Read<void*>(fixed,0xc8)!=state.gs||Read<void*>(fixed,0x28)!=state.gs
       ||Read<void*>(timing,0xc8)!=state.mode||Read<void*>(timing,0x28)!=state.mode
       ||Read<uintptr_t>(Read<void*>(fixed),0x3e0)!=engine::image+0x717e920
       ||Read<uintptr_t>(Read<void*>(timing),0x3e0)!=engine::image+0x7868330)return false;
    if(state.fixed){if(fixed!=state.fixed||timing!=state.timing||!Live(fixed,state.fixedId)||!Live(timing,state.timingId))return false;}
    else{if(!Capture(fixed,state.fixedId)||!Capture(timing,state.timingId))return false;state.fixed=fixed;state.timing=timing;}
    state.registeredFixed=(Read<uint8_t>(fixed,0xa0)&1)!=0;state.registeredTiming=(Read<uint8_t>(timing,0xa0)&1)!=0;
    state.fixedFlags=Read<uint8_t>(fixed,0xc2);state.timingFlags=Read<uint8_t>(timing,0xc2);
    state.rateAfter=Read<uint8_t>(fixed,0xd9);state.modeLifecycle=Read<uint8_t>(state.mode,0x11c)&0x30;
    state.gsLifecycle=Read<uint8_t>(state.gs,0x11c)&0x30;state.scheduleCount=Read<int32_t>(fixed,0xe8);
    return state.registeredFixed&&state.registeredTiming;
}
inline bool AdmittedBootstrapContext(void* world,void* pc){
    return Thread()&&world==state.world&&pc==state.bootstrapPC&&state.startRequested
        &&state.initReturned&&state.preInitReturned&&state.initStateReturned
        &&SourceLive()&&Bootstrap()&&GameStateLive();
}
inline bool ReadyForAdoption(void* world,void* pc){
    return Thread()&&state.ready&&state.startReturned&&world==state.world&&pc==state.bootstrapPC
        &&SourceLive()&&Bootstrap()&&GameStateLive()&&Components()&&state.gsLifecycle==0x20
        &&state.modeLifecycle==0x20&&(state.fixedFlags&0x40)&&(state.timingFlags&0x40)&&state.rateAfter==128
        &&Read<void*>(state.timing,0x108)==state.fixed&&(Read<uint8_t>(world,0x156)&0x80)!=0;
}
inline bool OwnsStartedGameState(void* world,void* gs,void* mode){
    // This GS came from the sole original Mode PreInit factory, not the
    // separate manual trainer factory. Never manufacture that other receipt.
    return state.configured&&state.ready&&!state.failed&&!original_players::createdGameState
        &&world==state.world&&gs==state.gs&&mode==state.mode
        &&ReadyForAdoption(world,state.bootstrapPC)
        &&Read<void*>(world,0x170)==mode&&Read<void*>(world,0x178)==gs
        &&Read<void*>(mode,0x4d8)==gs&&Read<void*>(gs,0x470)==mode
        &&OwnsModeTable(world,mode,Read<void**>(mode));
}
inline bool Pump(){
    if(!SourceLive()||!state.initReturned||!state.preInitReturned||!state.initStateReturned||!GameStateLive())
        return Fail(8705,"mode_startup_actual_original_init_or_state_links_missing");
    if(state.startForwarded)return state.ready;
    ++state.pumps;
    if(GetTickCount64()-state.boundAt>options.timeoutMs)return Fail(8705,"mode_startup_real_service_or_bootstrap_timeout");
    if(!Bootstrap()){state.stage="mode_startup_waiting_actual_pawnless_local_engine_controller";return true;}
    const auto preparation=hooks.preparePlay(Current());
    if(preparation==Preparation::Failed)return Fail(8706,"mode_startup_local_preparation_failed");
    if(preparation!=Preparation::Ready){state.stage="mode_startup_waiting_actual_local_services";return true;}
    state.playServicesReady=true;
    if(!SourceLive()||!Bootstrap()||!Components()||(Read<uint8_t>(state.mode,0x11b)&4)==0
       ||(Read<uint8_t>(state.gs,0x11b)&4)==0)return Fail(8707,"mode_startup_original_actors_not_registered_initialized");
    if(state.modeLifecycle!=0||(state.timingFlags&0x40))return Fail(8707,"mode_startup_mode_or_timing_already_entered_no_replay");
    state.rateBefore=state.rateAfter;
    if(state.gsLifecycle==0&&(state.fixedFlags&0x40)==0){
        if(state.gsDispatchAttempted)return Fail(8707,"mode_startup_partial_gs_first_dispatch_never_retried");
        state.gsDispatchAttempted=true;++state.gsDispatchCalls;
        // Original WorldSettings4D48E20 and Level461E5E3 both pass DL=1.
        // A full, never-started GS actor lifecycle owns the FixedMgr's first
        // BeginPlay717E920 and its original zero->128 rate initialization.
        engine::function<void(*)(void*,uint8_t)>(0x3efa7d0)(state.gs,1);
        state.gsDispatchReturned=true;
    }else if(state.gsLifecycle!=0x20||(state.fixedFlags&0x40)==0)
        return Fail(8707,"mode_startup_gs_or_fixed_partial_lifecycle_no_replay");
    if(!SourceLive()||!Bootstrap()||!Components()||state.gsLifecycle!=0x20||(state.fixedFlags&0x40)==0
       ||state.rateAfter!=128||state.scheduleCount!=0)return Fail(8708,"mode_startup_original_gs_fixed_rate_readback_failed");
    state.startForwarded=true;++state.startForwards;
    engine::function<Simple>(0x43ca9e0)(state.mode);
    state.startReturned=true;
    if(!SourceLive()||!Components()||state.modeLifecycle!=0x20||(state.timingFlags&0x40)==0
       ||Read<void*>(state.timing,0x108)!=state.fixed||(Read<uint8_t>(state.world,0x156)&0x80)==0)
        return Fail(8709,"mode_startup_original_start_play_or_timing_readback_failed");
    state.ready=true;state.readyAt=GetTickCount64();
    Cache();
    if(!hooks.started(Current()))return Fail(8709,"mode_startup_actual_native_state_adoption_failed");
    state.stage="mode_startup_original_mode_gs_fixed_timing_started_delivery_unverified";return true;
}
inline void InitGameImpl(void* mode,const void* map,const void* opts,void* error){
    if(!SourceLive()||mode!=state.mode||state.initEntered){Fail(8703,"mode_startup_init_game_owner_or_once_gate_failed");return;}
    state.initEntered=true;++state.initCalls;
    if(!hooks.beforeInit(Current())){Fail(8703,"mode_startup_real_local_gi_object_prerequisites_missing");return;}
    state.giObjectsReady=true;
    // All original map/options/error carriers and original GameSession setup.
    engine::function<Init>(0x71812d0)(mode,map,opts,error);state.initReturned=true;
    const auto message=Read<Array>(error);state.initErrorCharacters=message.count>0?message.count-1:0;
    // FString.Num includes the terminator. Preserve the carrier and its text;
    // accept both empty forms (Num0 or Num1/L'\0'), never erase native errors.
    if(!error||message.count<0||message.capacity<message.count||message.count>1048576
       ||(message.count&&(!message.data||!original_players::Readable(message.data,size_t(message.count)*sizeof(wchar_t))))
       ||message.count>1||(message.count==1&&Read<wchar_t>(message.data)!=0))
        Fail(8703,"mode_startup_original_init_game_error_or_malformed_error_carrier");
    Cache();
}
inline void PreInitializeImpl(void* mode){
    if(!SourceLive()||mode!=state.mode||!state.initReturned||state.preInitEntered
       ||Read<void*>(mode,0x4d8)||Read<void*>(state.world,0x178)){
        Fail(8703,"mode_startup_preinitialize_requires_fresh_actual_mode_without_state");return;}
    state.preInitEntered=true;++state.preInitCalls;
    engine::function<Simple>(0x7182ec0)(mode);state.preInitReturned=true;
    if(!state.initStateReturned||!GameStateLive())Fail(8704,"mode_startup_original_state_factory_not_observed");Cache();
}
inline void InitGameStateImpl(void* mode){
    if(!SourceLive()||mode!=state.mode||!state.preInitEntered||state.preInitReturned||state.initStateEntered){
        Fail(8704,"mode_startup_init_state_outside_single_original_preinitialize");return;}
    state.gs=Read<void*>(mode,0x4d8);++state.initStateCalls;state.initStateEntered=true;
    if(!Capture(state.gs,state.gsId)||!GameStateLive()){Fail(8704,"mode_startup_original_selected_gs_factory_links_mismatch");return;}
    engine::function<Simple>(0x43cfb40)(mode);state.initStateReturned=true;Cache();
}
inline void StartPlayImpl(void* mode){
    ++state.startRequests;
    if(!SourceLive()||mode!=state.mode||state.startRequested){Fail(8705,"mode_startup_original_start_play_called_more_than_once");return;}
    state.startRequested=true;
    if(!Pump()&&!state.failed)Fail(8705,"mode_startup_first_start_play_dependency_failed");Cache();
}
inline void TickImpl(void* mode,float delta){
    if(!Thread()||mode!=state.mode||!SourceLive()){Fail(8710,"mode_startup_tick_not_current_original_authority_source");return;}
    ++state.ticks;state.modeTickObserved=true;
    if(state.busy){Fail(8710,"mode_startup_native_tick_or_first_start_reentered");return;}
    state.busy=true;
    if(!state.ready){++state.withheldTicks;if(state.startRequested)Pump();}
    if(state.ready&&!state.failed){
        engine::function<TickFunction>(0x43cb9f0)(mode,delta);++state.parentTicks;
        if(SourceLive())hooks.tick(Current(),delta);
    }state.busy=false;Cache();
}
inline LONG Exception(EXCEPTION_POINTERS* p){
    state.busy=false;state.exceptionCode=p&&p->ExceptionRecord?p->ExceptionRecord->ExceptionCode:0;
    const auto at=p&&p->ExceptionRecord?reinterpret_cast<uintptr_t>(p->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;Fail(8711,"mode_startup_original_exception_no_retry_or_component_replay");return EXCEPTION_EXECUTE_HANDLER;
}
inline void InitGame(void* m,const void* a,const void* b,void* e){__try{InitGameImpl(m,a,b,e);}__except(Exception(GetExceptionInformation())){}}
inline void PreInitialize(void* m){__try{PreInitializeImpl(m);}__except(Exception(GetExceptionInformation())){}}
inline void InitGameState(void* m){__try{InitGameStateImpl(m);}__except(Exception(GetExceptionInformation())){}}
inline void StartPlay(void* m){__try{StartPlayImpl(m);}__except(Exception(GetExceptionInformation())){}}
inline void Tick(void* m,float d){__try{TickImpl(m,d);}__except(Exception(GetExceptionInformation())){}}
inline void* FactoryImpl(void* gi,void* urlValueCarrier,void* world){
    // FURL is a native by-value carrier. Correct forwards invoke the original
    // exactly once; its original tail3DF7850 consumes/destroys that copy.
    if(!Thread()||!Profiles()){Fail(8701,"mode_startup_factory_not_on_configured_original_game_thread");return nullptr;}
    if(!MatchesPackage(world)){++state.foreignWorldForwards;
        return engine::function<Factory>(0x43c84b0)(gi,urlValueCarrier,world);}
    if(state.mode||Read<void*>(world,0x170)||(Read<uint8_t>(world,0x156)&0xc0)!=0
       ||Read<void*>(world,0x1d8)!=gi||!original_players::IsA(gi,0x647e8a0)){
        Fail(8701,"mode_startup_factory_requires_fresh_original_world_without_existing_mode");return nullptr;}
    state.world=world;state.gi=gi;state.boundAt=GetTickCount64();
    if(!Capture(world,state.worldId)||!Capture(gi,state.giId)){Fail(8701,"mode_startup_factory_original_world_gi_weak_missing");return nullptr;}
    ++state.factoryCalls;auto mode=engine::function<Factory>(0x43c84b0)(gi,urlValueCarrier,world);
    state.mode=mode;
    state.selectedModeClass=engine::Find(options.modeClass);
    state.selectedStateClass=engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t)>(0x1d0c6e0)
        (engine::Class(original_players::ShooterGameStateClass),nullptr,options.gameStateClass,nullptr,0);
    if(!mode||!state.selectedModeClass||Read<void*>(mode,0x10)!=state.selectedModeClass
       ||!original_players::IsA(mode,0x6c8ec60)||!state.selectedStateClass
       ||!original_players::Derived(state.selectedStateClass,engine::Class(original_players::ShooterGameStateClass))
       ||Read<void**>(mode)!=reinterpret_cast<void**>(engine::image+0xb4fb520)
       ||Read<uint8_t>(mode,0x224)!=3||(Read<uint8_t>(mode,0x11b)&4)||(Read<uint8_t>(mode,0x11c)&0x30)
       ||WorldOf(mode)!=world){Fail(8702,"mode_startup_original_factory_did_not_return_unstarted_selected_shooter_mode");return nullptr;}
    state.modeCDO=Read<void*>(state.selectedModeClass,0x120);state.cdoStateClass=Read<void*>(state.modeCDO,0x480);
    if(!state.modeCDO||Read<void*>(state.modeCDO,0x10)!=state.selectedModeClass
       ||(Read<uint32_t>(state.modeCDO,8)&0x10U)==0){
        Fail(8702,"mode_startup_original_selected_mode_class_default_identity_mismatch");return nullptr;}
    state.originalBpStateDefaultVerified=state.cdoStateClass==state.selectedStateClass;
    if(!state.originalBpStateDefaultVerified){
        if(!options.allowStateInstanceOverride){Fail(8702,"mode_startup_original_bp_state_cdo_not_selected");return nullptr;}
        state.instanceStateOverride=true;
    }
    if(!Capture(mode,state.modeId)||!Capture(state.modeCDO,state.cdoId)||!Capture(state.selectedStateClass,state.stateClassId)
       ||!Capture(state.selectedModeClass,state.modeClassId)){Fail(8702,"mode_startup_actual_mode_class_default_weak_missing");return nullptr;}
    for(size_t n=0;n<Entries;++n)if(Read<uintptr_t>(Read<void**>(mode),n*8)!=engine::image+local_server_mode_startup_source::Slots[n]){
        Fail(8702,"mode_startup_complete_original_347_entry_table_mismatch");return nullptr;}
    if(Read<void*>(mode,0x480)!=state.selectedStateClass){
        if(!options.allowStateInstanceOverride||!ClassProperty(mode,L"GameStateClass",0x480,state.selectedStateClass,0x43c9490)){
            Fail(8702,"mode_startup_original_bp_mode_state_default_not_selected_runtime_verified");return nullptr;}
        state.instanceStateOverride=true;
    }
    if(options.bootstrapEngineClasses){
        state.megapacket=Read<void*>(mode,0xa40);
        if(!Capture(state.megapacket,state.megaId)||!original_players::IsA(state.megapacket,0x698a220)
           ||Read<void*>(state.megapacket,0xc8)!=mode||Read<void*>(state.megapacket,0x28)!=mode
           ||Read<uint8_t>(state.megapacket,0x470)!=0){
            Fail(8702,"mode_startup_base_bootstrap_requires_original_normal_megapacket_route");return nullptr;}
        if(Read<void*>(mode,0x4c0)!=engine::Class(0x3c0a200)
           ||Read<void*>(mode,0x4c8)!=engine::Class(0x42aae70)
           ||Read<void*>(mode,0x4c0)!=Read<void*>(state.modeCDO,0x4c0)
           ||Read<void*>(mode,0x4c8)!=Read<void*>(state.modeCDO,0x4c8)){
            Fail(8702,"mode_startup_original_engine_bootstrap_alternate_classes_changed");return nullptr;}
        if(!ClassProperty(mode,L"PlayerControllerClass",0x488,engine::Class(0x3c0a200),0x3c0a200)
           ||!ClassProperty(mode,L"PlayerStateClass",0x490,engine::Class(0x42aae70),0x42aae70)
           ||!ClassProperty(mode,L"DefaultPawnClass",0x4a0,nullptr,0x3ed66e0)){
            Fail(8702,"mode_startup_original_typed_bootstrap_class_configuration_failed");return nullptr;}
        state.bootstrapClassesConfigured=true;
    }else if(Read<void*>(mode,0x4a0)){
        Fail(8702,"mode_startup_unowned_default_pawn_factory_not_admitted");return nullptr;
    }
    if(Read<void*>(state.modeCDO,0x480)!=state.cdoStateClass||!SourceLive(false)){
        Fail(8702,"mode_startup_original_class_default_or_instance_configuration_changed");return nullptr;}
    auto copy=static_cast<void**>(VirtualAlloc(nullptr,Entries*8,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!copy){Fail(8702,"mode_startup_full_private_table_allocation_failed");return nullptr;}
    for(size_t n=0;n<Entries;++n)copy[n]=Expected(n,false);
    DWORD previous=0;if(!VirtualProtect(copy,Entries*8,PAGE_READONLY,&previous)){
        Fail(8702,"mode_startup_private_table_readonly_failed");return nullptr;}
    state.table=copy;state.tableReadOnly=true;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(mode),copy,reinterpret_cast<void*>(engine::image+0xb4fb520))
       !=reinterpret_cast<void*>(engine::image+0xb4fb520)){Fail(8702,"mode_startup_actual_mode_table_publication_conflict");return nullptr;}
    state.tablePublished=true;state.originalTablePreserved=Table(copy,false);
    state.stage="mode_startup_real_shooter_mode_factory_returned_preinit_guard_armed";Cache();return mode;
}
inline void* CreateGameModeForURL(void* gi,void* url,void* world){
    __try{auto result=FactoryImpl(gi,url,world);Cache();return result;}
    __except(Exception(GetExceptionInformation())){Cache();return nullptr;}
}
inline const char* Yes(bool b){return b?"true":"false";}
inline void Write(FILE* file){
    if(!file)return;const auto s=Snapshot();
    std::fprintf(file,"{\"stage\":\"%s\",\"error\":%u,\"configured\":%s,\"failed\":%s,\"ready\":%s,\"profiles\":%s,\"table_entries\":347,\"table_published\":%s,\"table_readonly\":%s,\"world\":\"%p\",\"mode\":\"%p\",\"gs\":\"%p\",\"fixed\":\"%p\",\"timing\":\"%p\",\"selected_mode_class\":\"%p\",\"selected_gs_class\":\"%p\",\"mode_cdo_gs_class\":\"%p\",\"bootstrap_pc\":\"%p\",\"bootstrap_ps\":\"%p\",\"bootstrap_pawn\":null,\"source_valid\":%s,\"bootstrap_valid\":%s,\"init_returned\":%s,\"preinit_returned\":%s,\"init_state_returned\":%s,\"gi_objects_ready\":%s,\"play_services_ready\":%s,\"gs_dispatch_attempted\":%s,\"gs_dispatch_returned\":%s,\"start_forwarded\":%s,\"start_returned\":%s,\"factory_calls\":%llu,\"init_calls\":%llu,\"preinit_calls\":%llu,\"init_state_calls\":%llu,\"start_requests\":%llu,\"start_forwards\":%llu,\"gs_dispatch_calls\":%llu,\"pumps\":%llu,\"native_mode_tick_observed\":%s,\"ticks\":%llu,\"withheld_ticks\":%llu,\"parent_ticks\":%llu,\"mode_lifecycle\":%u,\"gs_lifecycle\":%u,\"fixed_c2\":%u,\"timing_c2\":%u,\"rate_before\":%u,\"rate_after\":%u,\"schedule_count\":%d,\"state_instance_override\":%s,\"bootstrap_classes_configured\":%s,\"selected_state_adopted\":%s,\"profile_failure_rva\":%llu,\"exception_code\":%u,\"exception_rva\":%llu,\"cached_snapshot\":true,\"rate_writes\":0,\"clock_writes\":0,\"role_writes\":0,\"netmode_writes\":0,\"component_replays\":0,\"runtime_validated\":false,\"client_ready_verified\":false",
        s.stage,s.error,Yes(s.configured),Yes(s.failed),Yes(s.ready),Yes(s.profilesVerified),Yes(s.tablePublished),Yes(s.tableReadOnly),
        s.world,s.mode,s.gs,s.fixed,s.timing,s.selectedModeClass,s.selectedStateClass,s.cdoStateClass,s.bootstrapPC,s.bootstrapPS,
        Yes(s.sourceValid),Yes(s.bootstrapValid),Yes(s.initReturned),Yes(s.preInitReturned),Yes(s.initStateReturned),Yes(s.giObjectsReady),Yes(s.playServicesReady),
        Yes(s.gsDispatchAttempted),Yes(s.gsDispatchReturned),Yes(s.startForwarded),Yes(s.startReturned),
        static_cast<unsigned long long>(s.factoryCalls),static_cast<unsigned long long>(s.initCalls),static_cast<unsigned long long>(s.preInitCalls),static_cast<unsigned long long>(s.initStateCalls),
        static_cast<unsigned long long>(s.startRequests),static_cast<unsigned long long>(s.startForwards),static_cast<unsigned long long>(s.gsDispatchCalls),static_cast<unsigned long long>(s.pumps),
        Yes(s.modeTickObserved),static_cast<unsigned long long>(s.ticks),static_cast<unsigned long long>(s.withheldTicks),static_cast<unsigned long long>(s.parentTicks),
        unsigned(s.modeLifecycle),unsigned(s.gsLifecycle),unsigned(s.fixedFlags),unsigned(s.timingFlags),unsigned(s.rateBefore),unsigned(s.rateAfter),s.scheduleCount,
        Yes(s.instanceStateOverride),Yes(s.bootstrapClassesConfigured),Yes(s.selectedStateAdopted),static_cast<unsigned long long>(s.profileFailureRva),s.exceptionCode,static_cast<unsigned long long>(s.exceptionRva));
    std::fprintf(file,",\"original_bp_gs_cdo_verified\":%s,\"original_init_error_characters\":%d,\"original_actor_net_mode\":%d,\"original_world_net_mode\":%d,\"no_local_player_observed\":%s}",
        Yes(s.originalBpStateDefaultVerified),s.initErrorCharacters,s.actorNetMode,s.worldNetMode,Yes(s.noLocalPlayerObserved));
}
} // namespace local_server_mode_startup
