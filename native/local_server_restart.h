#pragma once
#include "original_inventory_slots.h"
#include "local_server_mode_startup.h"
#include <cstddef>

// The rotation portion of native GameModeBase.FinishRestartPlayer43d47d0.
// Call after the existing, single Possess. Never replay Possess/Restart here.
// Server ProcessEvent retains the original Client RPC callspace and transport.
namespace local_server_restart {
using Rotator=engine::Vec; // pitch, yaw, roll: three native float64 values
using Weak=original_inventory_slots::WeakIdentity;
struct alignas(8) ClientRotationParams {
    Rotator rotation{};uint8_t resetCamera=1;uint8_t padding[7]{};
};
static_assert(sizeof(Rotator)==24&&sizeof(ClientRotationParams)==32);
static_assert(offsetof(ClientRotationParams,resetCamera)==24);
struct Diagnostics {
    unsigned error=0;
    bool profilesVerified=false,metadataVerified=false,authorityPairVerified=false;
    bool weakVerified=false,rpcSubmitted=false,serverSetSubmitted=false;
    bool nativeReadbacksMatched=false,completed=false,restartRequired=false;
    uint64_t publications=0,idempotentCalls=0;
    uintptr_t exceptionRva=0;
    const char* stage="local_network_initial_rotation_not_published";
    const char* operation="none";
    void *world=nullptr,*mode=nullptr,*driver=nullptr,*connection=nullptr;
    void *controller=nullptr,*playerState=nullptr,*pawn=nullptr;
    void *rpc=nullptr,*setter=nullptr,*controllerGetter=nullptr,*actorGetter=nullptr;
    Weak worldIdentity{},modeIdentity{},driverIdentity{},connectionIdentity{};
    Weak controllerIdentity{},stateIdentity{},pawnIdentity{},publishedPawnIdentity{};
    Rotator requested{},rpcRotation{},pawnRotationBefore{},pawnRotationAfter{};
    Rotator serverControlBefore{},serverControlAfter{};
    uint16_t rpcParameterBytes=0,setterParameterBytes=0;
    uint32_t rpcFlags=0;
};
inline Diagnostics state;
inline const Diagnostics& State(){return state;}
inline const Diagnostics& GetState(){return state;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Fail(unsigned error,const char* reason){state.error=error;state.stage=reason;state.completed=false;return false;}
inline bool VerifyProfiles(){
    struct Profile{uintptr_t rva;size_t bytes;uint64_t fnv;};
    static constexpr Profile profiles[]{
        {0x43d47d0,520,0x39249840d48751aeULL}, // full source rotation sequence
        {0x71836e0,5,0x818248cbf5388491ULL}, // original Shooter thunk JMP43D47D0
        {0x42ab6f0,544,0xf24c43b84e8f45beULL}, // native ClientSetRotation executor
        {0x42abfd0,242,0x5995cf1fd8ef7ea8ULL}, // native SetControlRotation executor
        {0x42ac0d0,73,0xc96ab96a594e6997ULL}, // native controller rotation query
        {0x3ef1880,246,0x5688ebf065c2aa51ULL}, // native actor rotation query
        {0x42aeb20,98,0xb830c1032c9952dfULL}, // original client RPC implementation
        {0x781fc90,198,0xda469337ca3383ccULL}, // actual ShooterPC quantized setter
        {0x42ad260,27,0x3a03bd4d80195a21ULL}, // original control rotation getter
        {0x3ed83b0,1609,0x15d2d4ecc2f49cf7ULL}, // original actor GetWorld
        {0x1aeefd0,5,0xf2883a6f8498ad20ULL}, // bResetCamera parameter is byte24
    };
    for(const auto& p:profiles){
        const auto* bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        uint64_t hash=14695981039346656037ULL;
        for(size_t i=0;i<p.bytes;++i)hash=(hash^bytes[i])*1099511628211ULL;
        if(hash!=p.fnv)return false;
    }
    // UHT sizeof includes seven padding bytes; runtime ParmsSize excludes them.
    auto image=reinterpret_cast<void*>(engine::image);
    return Read<uint16_t>(image,0xd3cb7e0+0x30)==2
        &&Read<uint16_t>(image,0xd3cb7e0+0x32)==32
        &&Read<uint32_t>(image,0xd3cb7e0+0x38)==0x81820cc2U
        &&Read<uint16_t>(image,0xd3ce390+0x30)==1
        &&Read<uint16_t>(image,0xd3ce390+0x32)==24;
}
inline bool BindFunction(void*& out,const wchar_t* name,uint16_t bytes,uintptr_t exec,uintptr_t owner){
    out=engine::Find(name);
    return out&&Read<uint16_t>(out,0xbe)==bytes
        &&(Read<uint32_t>(out,0xb8)&0x400U)!=0
        &&Read<uintptr_t>(out,0xe0)==engine::image+exec
        &&Read<void*>(out,0x28)==engine::Class(owner);
}
inline bool BindMetadata(){
    if(!BindFunction(state.rpc,L"/Script/Engine.Controller.ClientSetRotation",25,0x42ab6f0,0x3ed67a0)
       ||!BindFunction(state.setter,L"/Script/Engine.Controller.SetControlRotation",24,0x42abfd0,0x3ed67a0)
       ||!BindFunction(state.controllerGetter,L"/Script/Engine.Controller.GetControlRotation",24,0x42ac0d0,0x3ed67a0)
       ||!BindFunction(state.actorGetter,L"/Script/Engine.Actor.K2_GetActorRotation",24,0x3ef1880,0x3683900))return false;
    state.rpcParameterBytes=Read<uint16_t>(state.rpc,0xbe);
    state.setterParameterBytes=Read<uint16_t>(state.setter,0xbe);
    state.rpcFlags=Read<uint32_t>(state.rpc,0xb8);
    constexpr uint32_t rpcMask=0x010004c0U; // native, Net, reliable, NetClient
    return (state.rpcFlags&rpcMask)==rpcMask
        &&(Read<uint32_t>(state.setter,0xb8)&0x440U)==0x400U;
}
inline bool Event(void* object,void* function,void* parameters){
    auto process=Read<void*>(Read<void**>(object,0),0x298);
    if(!object||!function||!process)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,function,parameters);return true;
}
inline void* ActorWorld(void* actor){
    auto target=Read<void*>(Read<void**>(actor,0),0x1b8);
    return target==reinterpret_cast<void*>(engine::image+0x3ed83b0)
        ?reinterpret_cast<void*(*)(void*)>(target)(actor):nullptr;
}
inline bool CanonicalTuple(){
    auto& d=state;
    auto clients=Read<void**>(d.driver,0xc8);
    auto modeTable=Read<void**>(d.mode,0),pcTable=Read<void**>(d.controller,0);
    const auto& startup=local_server_mode_startup::GetState();
    const bool originalModeSource=startup.configured&&startup.world==d.world;
    const bool modeSource=originalModeSource
        ? Read<void*>(d.mode,0x10)==engine::Class(0x6c8ec60)
            &&local_server_mode_startup::OwnsStartedGameState(d.world,Read<void*>(d.world,0x178),d.mode)
            &&Read<uintptr_t>(modeTable,0x948)==engine::image+0x71836e0
        : Read<void*>(d.mode,0x10)==engine::Class(0x439aa40)
            &&Read<uintptr_t>(modeTable,0x948)==engine::image+0x43d47d0;
    return d.world&&original_players::world==d.world&&(Read<uint8_t>(d.world,0x156)&0x80)
        &&Read<void*>(d.world,0x170)==d.mode
        &&modeSource
        &&Read<uint8_t>(d.mode,0x224)==3&&ActorWorld(d.mode)==d.world
        &&Read<uintptr_t>(modeTable,0x8f0)==engine::image+0x43d49e0
        &&Read<void*>(d.world,0x40)==d.driver
        &&original_players::IsA(d.driver,0x5112c90)&&Read<void*>(d.driver,0x178)==d.world
        &&!Read<void*>(d.driver,0xc0)&&Read<int32_t>(d.driver,0xd0)==1
        &&Read<int32_t>(d.driver,0xd4)>=1&&Read<void*>(clients,0)==d.connection
        &&original_players::IsA(d.connection,0x5112be0)
        &&Read<void*>(d.connection,0x60)==d.driver
        &&Read<int32_t>(d.connection,0x13c)==3&&Read<uint8_t>(d.connection,0x1d8)==3
        &&Read<void*>(d.connection,0x38)==d.controller&&Read<void*>(d.connection,0xa0)==d.controller
        &&original_players::IsA(d.controller,original_players::ShooterControllerClass)
        &&original_players::IsA(d.playerState,original_players::ShooterStateClass)
        &&original_players::IsA(d.pawn,original_players::ShooterCharacterClass)
        &&Read<uint8_t>(d.controller,0x224)==3&&Read<uint8_t>(d.playerState,0x224)==3&&Read<uint8_t>(d.pawn,0x224)==3
        &&Read<void*>(d.controller,0x510)==d.connection
        &&Read<void*>(d.controller,0x470)==d.playerState&&Read<void*>(d.playerState,0x210)==d.controller
        &&Read<void*>(d.controller,0x4b0)==d.pawn&&Read<void*>(d.pawn,0x4a0)==d.controller
        &&Read<void*>(d.pawn,0x488)==d.playerState&&original_players::Registered(d.playerState)
        &&Read<uint8_t>(d.pawn,0xa90)&&Read<void*>(d.pawn,0xc08)&&Read<void*>(d.pawn,0xba0)
        &&ActorWorld(d.controller)==d.world&&ActorWorld(d.playerState)==d.world&&ActorWorld(d.pawn)==d.world
        &&Read<uintptr_t>(pcTable,0x770)==engine::image+0x42aeb20
        &&Read<uintptr_t>(pcTable,0x7b0)==engine::image+0x42ad260
        &&Read<uintptr_t>(pcTable,0x7b8)==engine::image+0x781fc90;
}
inline bool CaptureIdentities(){
    return original_inventory_slots::CaptureIdentity(state.world,state.worldIdentity)
        &&original_inventory_slots::CaptureIdentity(state.mode,state.modeIdentity)
        &&original_inventory_slots::CaptureIdentity(state.driver,state.driverIdentity)
        &&original_inventory_slots::CaptureIdentity(state.connection,state.connectionIdentity)
        &&original_inventory_slots::CaptureIdentity(state.controller,state.controllerIdentity)
        &&original_inventory_slots::CaptureIdentity(state.playerState,state.stateIdentity)
        &&original_inventory_slots::CaptureIdentity(state.pawn,state.pawnIdentity);
}
inline bool IdentitiesMatch(){
    return original_inventory_slots::ResolveIdentity(state.worldIdentity)==state.world
        &&original_inventory_slots::ResolveIdentity(state.modeIdentity)==state.mode
        &&original_inventory_slots::ResolveIdentity(state.driverIdentity)==state.driver
        &&original_inventory_slots::ResolveIdentity(state.connectionIdentity)==state.connection
        &&original_inventory_slots::ResolveIdentity(state.controllerIdentity)==state.controller
        &&original_inventory_slots::ResolveIdentity(state.stateIdentity)==state.playerState
        &&original_inventory_slots::ResolveIdentity(state.pawnIdentity)==state.pawn;
}
inline bool NearAngle(double a,double b){return std::abs(std::remainder(a-b,360.))<=360./65536.+1.e-7;}
inline bool PublishInitialRotationImpl(void* world,void* pc,void* ps,void* pawn,const Rotator& requested){
    if(state.error||state.restartRequired)return false;
    if(!original_players::Thread()||!original_players::Finite(requested))
        return Fail(5350,"local_network_initial_rotation_requires_original_game_thread_and_finite_rotation");
    if(state.world&&(state.world!=world||state.controller!=pc||state.playerState!=ps))
        return Fail(5351,"local_network_initial_rotation_owned_session_identity_changed");
    auto mode=Read<void*>(world,0x170),driver=Read<void*>(world,0x40),connection=Read<void*>(pc,0x510);
    if(state.world&&(state.mode!=mode||state.driver!=driver||state.connection!=connection))
        return Fail(5351,"local_network_initial_rotation_actual_mode_driver_or_connection_changed");
    state.world=world;state.controller=pc;state.playerState=ps;state.pawn=pawn;
    state.mode=mode;state.driver=driver;state.connection=connection;
    state.stage="local_network_initial_rotation_validate_actual_authority_connection";
    if(!state.profilesVerified){
        if(!VerifyProfiles())return Fail(5352,"local_network_initial_rotation_native_profile_mismatch");
        state.profilesVerified=true;
    }
    if(!CanonicalTuple())return Fail(5353,"local_network_initial_rotation_actual_host_possession_or_native_virtual_mismatch");
    state.authorityPairVerified=true;
    if(!CaptureIdentities()||!IdentitiesMatch())return Fail(5354,"local_network_initial_rotation_native_weak_identity_mismatch");
    state.weakVerified=true;
    if(state.completed&&original_inventory_slots::SameIdentity(state.pawnIdentity,state.publishedPawnIdentity)){
        if(std::memcmp(&state.requested,&requested,sizeof(requested))!=0)
            return Fail(5355,"local_network_initial_rotation_same_life_requested_rotation_changed");
        ++state.idempotentCalls;return true;
    }
    state.completed=false;state.rpcSubmitted=false;state.serverSetSubmitted=false;state.nativeReadbacksMatched=false;
    state.requested=requested;
    if(!BindMetadata())return Fail(5356,"local_network_initial_rotation_reflected_native_rpc_abi_mismatch");
    state.metadataVerified=true;
    state.operation="original_native_actor_and_control_rotation_before";
    if(!Event(pawn,state.actorGetter,&state.pawnRotationBefore)
       ||!Event(pc,state.controllerGetter,&state.serverControlBefore)
       ||!original_players::Finite(state.pawnRotationBefore)||!original_players::Finite(state.serverControlBefore))
        return Fail(5357,"local_network_initial_rotation_original_before_queries_failed");
    // Exact43d48e1..43d4941 order: RPC uses actual Pawn rotation; the server
    // setter uses requested pitch/yaw and zero roll. Only original functions
    // change native state. A returned RPC submission is not client receipt.
    ClientRotationParams params;params.rotation=state.pawnRotationBefore;
    state.rpcRotation=params.rotation;
    state.operation="original_controller_client_set_rotation_rpc";
    if(!Event(pc,state.rpc,&params))return Fail(5358,"local_network_initial_rotation_original_rpc_submission_failed");
    state.rpcSubmitted=true;
    const Rotator serverRotation{requested.x,requested.y,0};
    state.operation="original_controller_server_set_control_rotation";
    if(!Event(pc,state.setter,const_cast<Rotator*>(&serverRotation))){
        state.restartRequired=true;return Fail(5359,"local_network_initial_rotation_partial_publication_requires_new_session");
    }
    state.serverSetSubmitted=true;
    state.operation="original_native_actor_and_control_rotation_after";
    if(!IdentitiesMatch()||!CanonicalTuple()||!Event(pawn,state.actorGetter,&state.pawnRotationAfter)
       ||!Event(pc,state.controllerGetter,&state.serverControlAfter)
       ||!original_players::Finite(state.pawnRotationAfter)||!original_players::Finite(state.serverControlAfter)
       ||!NearAngle(state.serverControlAfter.x,serverRotation.x)||!NearAngle(state.serverControlAfter.y,serverRotation.y)
       ||!NearAngle(state.serverControlAfter.z,0)
       ||std::memcmp(&state.pawnRotationAfter,&state.pawnRotationBefore,sizeof(Rotator))!=0){
        state.restartRequired=true;return Fail(5360,"local_network_initial_rotation_native_readbacks_or_ownership_changed");
    }
    state.nativeReadbacksMatched=true;state.completed=true;state.publishedPawnIdentity=state.pawnIdentity;
    ++state.publications;state.operation="none";state.stage="local_network_initial_rotation_rpc_submitted_and_server_control_verified";return true;
}
inline LONG CaptureException(EXCEPTION_POINTERS* e){
    state.error=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:5361;
    const auto at=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;state.restartRequired=true;
    state.stage="local_network_initial_rotation_native_exception_requires_new_session";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool PublishInitialRotation(void* world,void* pc,void* ps,void* pawn,const Rotator& requested){
    __try{return PublishInitialRotationImpl(world,pc,ps,pawn,requested);}
    __except(CaptureException(GetExceptionInformation())){return false;}
}
}
