#pragma once
#include "original_players.h"
#include "local_server_mode_startup.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cmath>

// Explicit local-authority replacement for the absent registered
// AuthSetGameplayEnabled body. Evidence: work/local-server-playable/
// input-authority/active-gate/. Only the two Auth* timestamps are published.
// Original OnRep/GameInfo getter performs local clock maturation and +811
// changes. This module never writes +811, input masks, Phase or client state.
namespace local_server_game_start {
struct Config { uint32_t nativeTickRate=128,maxClockBaseOffset=0x1000; };
inline constexpr Config Configuration{};
struct NativeProfile { uint32_t rva,bytes;uint64_t fnv; };
inline constexpr NativeProfile RequiredProfiles[]{
    {0x3efc820,176,0x7f1113b3f79164b2ULL},
    {0x787b6c0,99,0x2fa4f38ac8e06b6aULL},
    {0x7892e10,142,0x773b71bd06054ce5ULL},
    {0x789df70,144,0x59af83ea9f307d53ULL},
    {0x79618f0,1630,0xb29eb435f6999f3bULL},
    {0x71911e0,282,0xfd9afc64a82e265bULL},
    {0x7191300,205,0x5545ff97a8f003c9ULL},
    {0x6ca0980,62,0xdc7acb690b95ab97ULL},
    {0x6ca43d0,522,0x4b7c0fd5f0277fe9ULL},
};
inline constexpr size_t GameInfoOffset=0x7d8,AuthTimelineOffset=0x7f4;
inline constexpr uintptr_t NetConnectionClass=0x3dea9c0;
inline constexpr int32_t NativeConnectionOpen=3;
struct alignas(8) GameInfo {
    void* matchTimeoutCallingPlayer=nullptr;
    float displayRemaining=0,localDisplayRemaining=0,stateRemaining=0,localStateRemaining=0;
    float phaseElapsed=0,authStart=-1,authEnd=-1,localStart=-1,localEnd=-1,matchTimeoutExpiry=-1;
    uint8_t timeAdvancing=0,store=0,infiniteAbilities=0,infiniteAmmo=0,infiniteReserve=0,infiniteMoney=0;
    uint8_t ignoreShopping=0,matchPaused=0,intention=0,active=0,dropInSystem=0,dropIn=0;
    uint8_t megamapPings=0,matchTimeoutState=0,padding[2]{};
};
struct AuthTimeline {float start=-1,end=-1;};
static_assert(sizeof(GameInfo)==64&&alignof(GameInfo)==8&&sizeof(AuthTimeline)==8);
static_assert(offsetof(GameInfo,authStart)==0x1c&&offsetof(GameInfo,authEnd)==0x20);
static_assert(offsetof(GameInfo,localStart)==0x24&&offsetof(GameInfo,localEnd)==0x28);
static_assert(offsetof(GameInfo,active)==0x39&&offsetof(GameInfo,matchTimeoutState)==0x3d);
enum OwnerCheck : uint8_t {
    WorldStateIdentity,CreatedStateIdentity,NativeTypes,AuthorityRoles,AutonomousRemoteRole,
    PCPlayerConnection,ConnectionController,ConnectionActor,PCPawn,PawnController,
    PlayerStateAgreement,PlayerStateOwner,MovementOwner,MovementCharacter,MovementPawn,
    InputOwner,InputPawn,GameStateAuthorityMode,ModeState,OwnerCheckCount
};
inline constexpr uint64_t OwnerBit(OwnerCheck check){return uint64_t(1)<<check;}
inline constexpr uint64_t RequiredOwnerMask=(uint64_t(1)<<OwnerCheckCount)-1;
struct State {
    bool initialized=false,profilesVerified=false,ownersVerified=false,threadMatches=false;
    bool scheduleSubmitted=false,localAuthorityStartSupplied=false,preservedExistingSchedule=false;
    bool alreadyActive=false,gameplayActive=false,originalQueryValid=false,nativeOnRepCompleted=false;
    bool nativeUpdateObserved=false,nativeLocalControllerSampled=false,noLocalControllerWithActiveObserved=false;
    bool disconnected=false,restartRequired=false,runtime_unproven=true,clientMaturationUnproven=true;
    uint32_t error=0,profileFailureRva=0,gameThread=0,actualThreadId=0,playerRegistryThreadId=0;
    uint32_t polls=0,queryCalls=0,onRepCalls=0,scheduleWrites=0,pawnAdoptions=0,clockIndex=0,clockStep=0,clockBaseOffset=0;
    uint64_t passedOwnerMask=0,failedOwnerMask=0;
    float requestedEndSeconds=-1,sourceClockTime=0,submittedStart=-1,submittedEnd=-1;
    uint8_t actualPawnRole=0,actualPawnRemoteRole=0,actualPCRole=0,actualGSRole=0;
    int32_t connectionState=0;
    int pawnNetMode=-1,stateNetMode=-1;
    GameInfo before{},after{};
    void *world=nullptr,*pawn=nullptr,*previousPawn=nullptr,*gameState=nullptr,*mode=nullptr,*pc=nullptr,*connection=nullptr;
    void *playerState=nullptr,*movement=nullptr,*inputState=nullptr,*fixedStep=nullptr;
    void *clock=nullptr,*serverPrediction=nullptr,*nativeMovement=nullptr,*nativeLocalController=nullptr;
    void *actualWorldState=nullptr,*actualPCPlayer=nullptr,*actualConnectionPC=nullptr,*actualConnectionActor=nullptr;
    void *actualPCPawn=nullptr,*actualPawnController=nullptr,*actualPCState=nullptr,*actualPawnState=nullptr;
    void *actualStateOwner=nullptr,*actualMovementOwner=nullptr,*actualMovementCharacter=nullptr,*actualMovementPawn=nullptr;
    void *actualInputOwner=nullptr,*actualInputPawn=nullptr,*actualStateMode=nullptr,*actualModeState=nullptr;
    void *actualPawnWorld=nullptr,*actualStateWorld=nullptr,*onRepFunction=nullptr,*activeFunction=nullptr;
    const wchar_t* failedEventName=nullptr;
    uint16_t expectedEventBytes=0,actualEventBytes=0;
    uint32_t actualEventFlags=0;
    uintptr_t expectedEventExecRva=0,actualEventExecRva=0,exceptionRva=0;
    const char* stage="network_game_start_not_initialized";
    const char* operation="none";
};
inline State state;
inline const State& GetState(){return state;}
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Fail(uint32_t error,const char* stage){
    state.error=error;state.stage=stage;state.restartRequired=true;
    state.gameplayActive=false;state.originalQueryValid=false;return false;
}
inline bool Thread(){
    state.actualThreadId=GetCurrentThreadId();state.playerRegistryThreadId=original_players::gameThread;
    state.threadMatches=engine::image&&state.gameThread&&state.actualThreadId==state.gameThread
        &&state.playerRegistryThreadId==state.gameThread;
    return state.threadMatches;
}
inline bool VerifyProfiles(){
    for(const auto& p:RequiredProfiles){
        const auto* bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        uint64_t hash=14695981039346656037ULL;
        if(!original_players::Readable(bytes,p.bytes)){state.profileFailureRva=p.rva;
            return Fail(3401,"network_game_start_native_profile_unreadable");}
        for(uint32_t i=0;i<p.bytes;++i){hash^=bytes[i];hash*=1099511628211ULL;}
        if(hash!=p.fnv){state.profileFailureRva=p.rva;return Fail(3402,"network_game_start_native_profile_changed");}
    }
    state.profilesVerified=true;return true;
}
inline void* NativeWorld(void* actor){
    auto table=Read<void**>(actor);auto entry=Read<void*>(table,0x1b8);
    return entry?reinterpret_cast<void*(*)(void*)>(entry)(actor):nullptr;
}
inline bool OwnerLinks(){
    state.actualWorldState=Read<void*>(state.world,0x178);
    state.actualPCPlayer=Read<void*>(state.pc,0x510);
    state.actualConnectionPC=Read<void*>(state.connection,0x38);state.actualConnectionActor=Read<void*>(state.connection,0xa0);
    state.actualPCPawn=Read<void*>(state.pc,0x4b0);state.actualPawnController=Read<void*>(state.pawn,0x4a0);
    state.actualPCState=Read<void*>(state.pc,0x470);state.actualPawnState=Read<void*>(state.pawn,0x488);
    state.actualStateOwner=Read<void*>(state.playerState,0x210);
    state.actualMovementOwner=Read<void*>(state.movement,0xc8);state.actualMovementCharacter=Read<void*>(state.movement,0x1a8);
    state.actualMovementPawn=Read<void*>(state.movement,0x190);
    state.actualInputOwner=Read<void*>(state.inputState,0xc8);state.actualInputPawn=Read<void*>(state.inputState,0x148);
    state.actualStateMode=Read<void*>(state.gameState,0x470);state.actualModeState=Read<void*>(state.mode,0x4d8);
    state.actualPawnRole=Read<uint8_t>(state.pawn,0x224);state.actualPawnRemoteRole=Read<uint8_t>(state.pawn,0x128);
    state.actualPCRole=Read<uint8_t>(state.pc,0x224);state.actualGSRole=Read<uint8_t>(state.gameState,0x224);
    const bool types=original_players::IsA(state.pawn,original_players::ShooterCharacterClass)
        &&original_players::IsA(state.gameState,original_players::ShooterGameStateClass)
        &&original_players::IsA(state.pc,original_players::ShooterControllerClass)
        &&original_players::IsA(state.playerState,original_players::ShooterStateClass)
        &&original_players::IsA(state.movement,0x6c7d380)&&original_players::IsA(state.inputState,0x6494370)
        &&original_players::IsA(state.connection,NetConnectionClass);
    uint64_t mask=0;const auto mark=[&](OwnerCheck check,bool passed){if(passed)mask|=OwnerBit(check);};
    mark(WorldStateIdentity,state.world&&state.actualWorldState==state.gameState);
    const auto& startup=local_server_mode_startup::GetState();
    const bool originalModeSource=startup.configured&&startup.world==state.world;
    // The real Mode's sole native GS factory is a separate provenance path.
    // Keep the project manual-factory cache null; require the actual started
    // Mode/GS/world tuple and its exact owned347-entry composed table instead.
    const bool stateProvenance=originalModeSource
        ? local_server_mode_startup::OwnsStartedGameState(state.world,state.gameState,state.mode)
        : state.gameState&&state.gameState==original_players::createdGameState;
    mark(CreatedStateIdentity,stateProvenance);
    mark(NativeTypes,types);mark(AuthorityRoles,state.actualPawnRole==3&&state.actualPCRole==3&&state.actualGSRole==3);
    mark(AutonomousRemoteRole,state.actualPawnRemoteRole==2);
    mark(PCPlayerConnection,state.connection&&state.actualPCPlayer==state.connection);
    mark(ConnectionController,state.actualConnectionPC==state.pc);mark(ConnectionActor,state.actualConnectionActor==state.pc);
    mark(PCPawn,state.actualPCPawn==state.pawn);mark(PawnController,state.actualPawnController==state.pc);
    mark(PlayerStateAgreement,state.playerState&&state.actualPCState==state.playerState&&state.actualPawnState==state.playerState);
    mark(PlayerStateOwner,state.actualStateOwner==state.pc);
    mark(MovementOwner,state.actualMovementOwner==state.pawn);mark(MovementCharacter,state.actualMovementCharacter==state.pawn);
    mark(MovementPawn,state.actualMovementPawn==state.pawn);
    mark(InputOwner,state.actualInputOwner==state.pawn);mark(InputPawn,state.actualInputPawn==state.pawn);
    mark(GameStateAuthorityMode,state.mode&&Read<void*>(state.world,0x170)==state.mode&&state.actualStateMode==state.mode);
    mark(ModeState,state.actualModeState==state.gameState);
    state.passedOwnerMask=mask;state.failedOwnerMask=RequiredOwnerMask&~mask;
    state.ownersVerified=state.failedOwnerMask==0;return state.ownersVerified;
}
inline bool Context(){
    const bool thread=Thread(),owners=OwnerLinks();
    if(!thread)return Fail(3403,"network_game_start_owned_game_thread_changed");
    if(!original_players::Read(state.connection,0x13c,state.connectionState))
        return Fail(3404,"network_game_start_connection_state_unreadable");
    if(state.connectionState!=NativeConnectionOpen){
        state.disconnected=true;state.gameplayActive=false;state.originalQueryValid=false;
        state.stage="network_game_start_original_connection_closed";return false;
    }
    if(state.disconnected)return Fail(3405,"network_game_start_connection_epoch_reopened");
    if(!owners)return Fail(3406,"network_game_start_native_owner_links_changed");
    state.actualPawnWorld=NativeWorld(state.pawn);state.actualStateWorld=NativeWorld(state.gameState);
    if(state.actualPawnWorld!=state.world||state.actualStateWorld!=state.world)
        return Fail(3407,"network_game_start_native_world_owner_mismatch");
    state.pawnNetMode=engine::function<int(*)(void*)>(0x3efc820)(state.pawn);
    state.stateNetMode=engine::function<int(*)(void*)>(0x3efc820)(state.gameState);
    if((state.pawnNetMode!=1&&state.pawnNetMode!=2)||state.stateNetMode!=state.pawnNetMode)
        return Fail(3408,"network_game_start_requires_original_server_netmode");
    return true;
}
inline bool Function(void*& result,const wchar_t* name,uint16_t bytes,uintptr_t exec){
    result=engine::Find(name);const uint16_t actualBytes=Read<uint16_t>(result,0xbe);
    const uint32_t flags=Read<uint32_t>(result,0xb8);const uintptr_t native=Read<uintptr_t>(result,0xe0);
    if(result&&actualBytes==bytes&&native==engine::image+exec&&(flags&0x400)&&!(flags&0x40))return true;
    state.failedEventName=name;state.expectedEventBytes=bytes;state.actualEventBytes=actualBytes;
    state.actualEventFlags=flags;state.expectedEventExecRva=exec;
    state.actualEventExecRva=native>=engine::image?native-engine::image:native;
    return Fail(3409,"network_game_start_original_event_abi_changed");
}
inline bool Dispatch(void* function,void* parameters){
    auto table=Read<void**>(state.gameState);auto process=Read<void*>(table,0x298);
    if(!function||!process)return Fail(3410,"network_game_start_original_event_dispatch_missing");
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(state.gameState,function,parameters);return true;
}
inline bool SampleInfo(){
    if(!original_players::Read(state.gameState,GameInfoOffset,state.after))
        return Fail(3411,"network_game_start_game_info_unreadable");
    const auto valid=[](float value){return std::isfinite(value)&&(value==-1.0f||value>=0);};
    if(!valid(state.after.authStart)||!valid(state.after.authEnd)||!valid(state.after.localStart)
       ||!valid(state.after.localEnd)||state.after.active>1)
        return Fail(3412,"network_game_start_original_timestamp_domain_invalid");
    return true;
}
inline bool SampleClock(){
    state.nativeMovement=engine::function<void*(*)(void*)>(0x787b6c0)(state.pawn);
    if(state.nativeMovement!=state.movement)return Fail(3413,"network_game_start_original_movement_getter_mismatch");
    state.clock=engine::function<void*(*)(void*)>(0x7892e10)(state.movement);
    state.serverPrediction=Read<void*>(state.movement,0x7f0);
    if(!state.clock||!state.serverPrediction
       ||state.clock!=static_cast<uint8_t*>(state.serverPrediction)+0xf0
       ||Read<void*>(state.clock)!=reinterpret_cast<void*>(engine::image+0xb600a60))
        return Fail(3414,"network_game_start_original_authority_clock_changed");
    auto vb=Read<void*>(state.clock,0x10);const int32_t offset=Read<int32_t>(vb,4);
    if(!vb||offset<0||static_cast<uint32_t>(offset)>Configuration.maxClockBaseOffset)
        return Fail(3415,"network_game_start_native_clock_vbase_invalid");
    state.clockBaseOffset=static_cast<uint32_t>(offset);
    state.clockIndex=Read<uint32_t>(state.clock,state.clockBaseOffset+0x18);
    state.clockStep=Read<uint32_t>(state.clock,state.clockBaseOffset+0x1c);
    if(Read<void*>(state.gameState,0xcc0)!=state.fixedStep||!state.fixedStep
       ||Read<uint8_t>(state.fixedStep,0xd9)!=Configuration.nativeTickRate||state.clockStep!=1)
        return Fail(3416,"network_game_start_requires_original_128hz_clock");
    state.sourceClockTime=engine::function<float(*)(void*)>(0x789df70)(state.movement);
    const float expected=static_cast<float>(state.clockIndex)*(1.0f/128.0f);
    if(!std::isfinite(state.sourceClockTime)||state.sourceClockTime<0||state.sourceClockTime!=expected)
        return Fail(3417,"network_game_start_original_host_clock_time_mismatch");
    return true;
}
inline bool PollImpl(){
    if(state.error||state.restartRequired)return false;
    if(!state.initialized){state.stage="network_game_start_waiting_authority_context";return false;}
    if(!Context()||!SampleClock()||!SampleInfo())return false;
    const bool pendingStart=state.after.localStart!=-1&&state.after.active==0;
    const bool pendingEnd=state.after.localStart==-1&&state.after.localEnd!=-1&&state.after.active!=0;
    state.operation="network_game_start_original_active_query";
    // Bound the extra diagnostic lookup to one pending native transition.
    // Stable active polling only calls the original active getter below.
    if((pendingStart||pendingEnd)&&!state.nativeLocalControllerSampled){
        state.nativeLocalController=engine::function<void*(*)(void*)>(0x79618f0)(state.gameState);
        state.nativeLocalControllerSampled=true;
    }
    uint8_t active=255;++state.queryCalls;
    state.originalQueryValid=Dispatch(state.activeFunction,&active);
    if(!state.originalQueryValid||active>1)return Fail(3418,"network_game_start_native_active_query_failed");
    state.nativeUpdateObserved=true;
    if(!SampleInfo())return false;
    if(state.after.active!=active)return Fail(3419,"network_game_start_native_active_readback_mismatch");
    state.gameplayActive=active!=0;++state.polls;
    if(pendingStart&&state.nativeLocalControllerSampled&&!state.nativeLocalController&&state.gameplayActive)
        state.noLocalControllerWithActiveObserved=true;
    state.operation="none";
    state.stage=state.gameplayActive?"network_game_start_native_active_observed_runtime_unproven":
        (state.after.localEnd==-1&&state.after.authEnd>=0?"network_game_start_original_end_observed":
         "network_game_start_schedule_submitted_waiting_original_maturation");
    return true;
}
inline bool StartImpl(void* world,void* pawn,float endSeconds){
    if(state.error||state.restartRequired)return false;
    if(state.initialized){
        if(world!=state.world||pawn!=state.pawn||endSeconds!=state.requestedEndSeconds)
            return Fail(3420,"network_game_start_existing_epoch_context_changed");
        return PollImpl();
    }
    if(!world||!pawn||!engine::image||!original_players::gameThread
       ||GetCurrentThreadId()!=original_players::gameThread)
        return Fail(3421,"network_game_start_requires_native_game_thread_context");
    if(!std::isfinite(endSeconds)||(endSeconds<0&&endSeconds!=-1))
        return Fail(3422,"network_game_start_end_timestamp_invalid");
    state.gameThread=GetCurrentThreadId();state.world=world;state.pawn=pawn;state.requestedEndSeconds=endSeconds;
    state.gameState=Read<void*>(world,0x178);state.mode=Read<void*>(world,0x170);
    state.pc=Read<void*>(pawn,0x4a0);state.connection=Read<void*>(state.pc,0x510);
    state.playerState=Read<void*>(pawn,0x488);state.movement=Read<void*>(pawn,0x4f8);
    state.inputState=Read<void*>(pawn,0xba0);state.fixedStep=Read<void*>(state.gameState,0xcc0);
    if(!VerifyProfiles()||!Context()||!SampleClock()||!SampleInfo())return false;
    if(!Function(state.onRepFunction,L"/Script/ShooterGame.ShooterGameState.OnRep_GameInfo",64,0x6ca43d0)
       ||!Function(state.activeFunction,L"/Script/ShooterGame.ShooterGameState.IsGameplayActive",1,0x6ca0980))return false;
    state.before=state.after;state.initialized=true;
    // Preserve an existing authority timeline. Poll never republishes it or
    // replays its RepNotify to manufacture another active transition.
    if(state.before.authStart>=0){
        state.preservedExistingSchedule=true;state.scheduleSubmitted=true;
        state.submittedStart=state.before.authStart;state.submittedEnd=state.before.authEnd;
        return PollImpl();
    }
    if(state.before.active){state.alreadyActive=true;return PollImpl();}
    if(state.before.authEnd!=-1)return Fail(3423,"network_game_start_existing_end_timeline_preserved");
    if(state.before.localStart!=-1||state.before.localEnd!=-1)
        return Fail(3424,"network_game_start_existing_local_pending_timeline_unproven");
    if(endSeconds!=-1&&endSeconds<state.sourceClockTime)
        return Fail(3425,"network_game_start_end_before_host_start");
    const AuthTimeline timeline{state.sourceClockTime,endSeconds};
    state.operation="network_game_start_supply_missing_authority_timestamps";
    if(!original_players::Write(state.gameState,AuthTimelineOffset,timeline))
        return Fail(3426,"network_game_start_authority_timestamp_publish_failed");
    const auto readback=Read<AuthTimeline>(state.gameState,AuthTimelineOffset);
    if(readback.start!=timeline.start||readback.end!=timeline.end)
        return Fail(3427,"network_game_start_authority_timestamp_readback_failed");
    state.localAuthorityStartSupplied=true;state.scheduleSubmitted=true;++state.scheduleWrites;
    state.submittedStart=timeline.start;state.submittedEnd=timeline.end;
    state.operation="network_game_start_original_game_info_rep_notify";++state.onRepCalls;
    if(!Dispatch(state.onRepFunction,&state.before))return false;
    state.nativeOnRepCompleted=true;
    return PollImpl();
}
inline bool AdoptPawnImpl(void* world,void* pawn){
    if(state.error||state.restartRequired)return false;
    if(!state.initialized||!Thread())return Fail(3428,"network_game_start_pawn_adoption_requires_existing_game_thread_epoch");
    if(!original_players::Read(state.connection,0x13c,state.connectionState))
        return Fail(3404,"network_game_start_connection_state_unreadable");
    if(state.connectionState!=NativeConnectionOpen){
        state.disconnected=true;state.gameplayActive=false;state.originalQueryValid=false;
        state.stage="network_game_start_original_connection_closed";return false;
    }
    if(state.disconnected)return Fail(3405,"network_game_start_connection_epoch_reopened");
    // Respawn may replace the Pawn under the same real connection, PC and PS.
    // It does not create a new GameInfo schedule or authorize another player.
    if(!pawn||world!=state.world||Read<void*>(world,0x178)!=state.gameState
       ||Read<void*>(pawn,0x4a0)!=state.pc||Read<void*>(pawn,0x488)!=state.playerState
       ||Read<void*>(state.pc,0x510)!=state.connection||Read<void*>(state.pc,0x4b0)!=pawn)
        return Fail(3429,"network_game_start_replacement_pawn_owner_epoch_mismatch");
    if(pawn==state.pawn)return PollImpl();
    AuthTimeline previousTimeline{};
    if(!original_players::Read(state.gameState,AuthTimelineOffset,previousTimeline))
        return Fail(3411,"network_game_start_game_info_unreadable");
    state.previousPawn=state.pawn;state.pawn=pawn;
    state.movement=Read<void*>(pawn,0x4f8);state.inputState=Read<void*>(pawn,0xba0);
    if(!Context()||!SampleClock()||!SampleInfo())return false;
    if(state.after.authStart!=previousTimeline.start||state.after.authEnd!=previousTimeline.end)
        return Fail(3430,"network_game_start_authority_timeline_changed_during_pawn_adoption");
    ++state.pawnAdoptions;
    return PollImpl();
}
inline LONG Capture(EXCEPTION_POINTERS* exception){
    const auto code=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0003400U;
    const auto pc=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=pc>=engine::image?pc-engine::image:pc;
    Fail(code,"network_game_start_native_exception_requires_new_session");return EXCEPTION_EXECUTE_HANDLER;
}
// true means the original submission/query completed. GetState().gameplayActive
// is the independent actual active gate. endSeconds is an absolute native clock
// timestamp; -1 keeps the native no-end sentinel. No runtime gameplay is claimed.
inline bool Start(void* world,void* pawn,float endSeconds=-1.0f){
    __try{return StartImpl(world,pawn,endSeconds);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool AdoptPawn(void* world,void* pawn){
    __try{return AdoptPawnImpl(world,pawn);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool Poll(){__try{return PollImpl();}__except(Capture(GetExceptionInformation())){return false;}}
}
