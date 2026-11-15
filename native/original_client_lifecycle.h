#pragma once
#include "original_players.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>

// Pinned-image, game-thread-only topology observation and a narrowly guarded
// original UWorld.RemoveController call for the superseded bootstrap PC.
// No player transfer, OnRep, restart, unpossess, actor destruction, Role/NetMode
// writes, or local implementation of an engine list mutation.
// Evidence: reports/duel1-original-client-lifecycle.json.
namespace original_client_lifecycle {
inline constexpr unsigned MaxControllers=40;
struct ControllerRecord {
    uintptr_t controller=0,pawn=0,player_state=0,player=0,player_backlink=0;
    uintptr_t camera=0,view_target=0;
    bool valid=false,is_expected=false,is_fallback=false,has_local_player=false,local_player_backlink_valid=false;
    bool native_local_controller_cached=false;
};
struct LocalPlayerRecord {uintptr_t player=0,controller=0;bool valid=false;};
struct AnimationRecord {
    uintptr_t mesh=0,instance=0,owning_actor=0,owning_component=0;
    bool query_valid=false,actor_matches=false,component_matches=false;
};
struct ABIRecord {uint16_t expected=0,actual=0;bool found=false,valid=false;};
struct Diagnostics {
    bool initialized=false,reflection_abi_valid=false,getter_agrees=false,world_context_matches=false;
    bool getpc0_is_expected=false,getpc0_is_fallback=false,getpc0_matches_first_local_player=false;
    bool pawn_context_agrees=false,equipment_context_agrees=false;
    unsigned error=0;
    uintptr_t exception_rva=0;
    uintptr_t world=0,game_instance=0,world_context=0,context_current_world=0;
    uintptr_t expected_controller=0,expected_pawn=0,fallback_controller=0,fallback_pawn=0;
    uintptr_t native_getpc0=0,reflected_getpc0=0,pawn_context_getpc0=0,equipment_context_getpc0=0;
    int native_index_count=0,world_controller_count=0,game_state_player_count=0,local_player_count=0;
    unsigned captured_indices=0,captured_world_controllers=0,captured_state_controllers=0,captured_local_players=0;
    uint64_t probes=0,observed_at_ms=0;
    const char* stage="original_client_topology_not_observed";
    std::array<ControllerRecord,MaxControllers> by_index{},world_controllers{},state_controllers{};
    std::array<LocalPlayerRecord,MaxControllers> local_players{};
    AnimationRecord first_person{},third_person{},cosmetic_third_person{};
    ABIRecord get_controller_abi{},get_count_abi{},owning_actor_abi{},owning_component_abi{};
};
inline Diagnostics state;
inline const Diagnostics& State(){return state;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;return false;}
inline bool ValidArray(int count,int capacity,const void* data){return count>=0&&count<=1024&&capacity>=count&&(!count||data);}
inline bool FunctionABI(const wchar_t* path,uint16_t bytes){auto fn=engine::Find(path);return fn&&Read<uint16_t>(fn,0xbe)==bytes;}
inline ABIRecord InspectABI(const wchar_t* path,uint16_t bytes){ABIRecord result;result.expected=bytes;auto fn=engine::Find(path);result.found=fn!=nullptr;result.actual=Read<uint16_t>(fn,0xbe);result.valid=result.found&&result.actual==bytes;return result;}
inline ControllerRecord InspectController(void* controller){
    ControllerRecord result;result.controller=reinterpret_cast<uintptr_t>(controller);
    if(!original_players::IsA(controller,0x3c0a200))return result;
    result.valid=true;result.is_expected=result.controller==state.expected_controller;result.is_fallback=result.controller==state.fallback_controller;
    auto pawn=Read<void*>(controller,0x4b0),player=Read<void*>(controller,0x510),camera=Read<void*>(controller,0x528);
    result.pawn=reinterpret_cast<uintptr_t>(pawn);result.player=reinterpret_cast<uintptr_t>(player);
    result.player_state=reinterpret_cast<uintptr_t>(Read<void*>(controller,0x470));
    result.camera=reinterpret_cast<uintptr_t>(camera);result.view_target=reinterpret_cast<uintptr_t>(Read<void*>(camera,0x580));
    result.has_local_player=original_players::IsA(player,0x3b6e680);
    result.player_backlink=reinterpret_cast<uintptr_t>(Read<void*>(player,0x38));
    result.local_player_backlink_valid=result.has_local_player&&Read<void*>(player,0x38)==controller;
    // Do not invoke IsLocalController here: original4929b30 itself sets8b4=1
    // for Standalone even for a PC with no ULocalPlayer. Observe its cache only.
    result.native_local_controller_cached=Read<uint8_t>(controller,0x8b4)!=0;return result;
}
inline AnimationRecord InspectAnimation(void* actor,size_t meshOffset){
    AnimationRecord result;auto mesh=Read<void*>(actor,meshOffset),instance=Read<void*>(mesh,0xa70);
    result.mesh=reinterpret_cast<uintptr_t>(mesh);result.instance=reinterpret_cast<uintptr_t>(instance);
    if(!mesh||!instance)return result;
    void* ownerActor=nullptr;void* ownerComponent=nullptr;
    const bool actorQuery=engine::Event(instance,L"/Script/Engine.AnimInstance.GetOwningActor",&ownerActor);
    const bool componentQuery=engine::Event(instance,L"/Script/Engine.AnimInstance.GetOwningComponent",&ownerComponent);
    result.query_valid=actorQuery&&componentQuery;
    result.owning_actor=reinterpret_cast<uintptr_t>(ownerActor);result.owning_component=reinterpret_cast<uintptr_t>(ownerComponent);
    result.actor_matches=ownerActor==actor;result.component_matches=ownerComponent==mesh;return result;
}
inline bool ProbeImpl(void* world,void* expectedController,void* expectedPawn,void* fallbackController,void* fallbackPawn,void* equipmentActor){
    if(!engine::image||original_players::gameThread!=GetCurrentThreadId())return Fail(1401,"original_client_topology_wrong_thread");
    const uint64_t probes=state.probes;state=Diagnostics{};state.probes=probes+1;state.observed_at_ms=GetTickCount64();
    state.world=reinterpret_cast<uintptr_t>(world);state.expected_controller=reinterpret_cast<uintptr_t>(expectedController);
    state.expected_pawn=reinterpret_cast<uintptr_t>(expectedPawn);state.fallback_controller=reinterpret_cast<uintptr_t>(fallbackController);
    state.fallback_pawn=reinterpret_cast<uintptr_t>(fallbackPawn);
    if(!world||!original_players::IsA(expectedController,0x3c0a200)||!expectedPawn
      ||Read<void*>(expectedController,0x4b0)!=expectedPawn)return Fail(1402,"original_client_topology_expected_pair_invalid");
    state.get_controller_abi=InspectABI(L"/Script/Engine.GameplayStatics.GetPlayerController",24);
    // Reflected ParmsSize excludes the C++ buffer's trailing alignment pad.
    state.get_count_abi=InspectABI(L"/Script/Engine.GameplayStatics.GetNumPlayerControllers",12);
    state.owning_actor_abi=InspectABI(L"/Script/Engine.AnimInstance.GetOwningActor",8);
    state.owning_component_abi=InspectABI(L"/Script/Engine.AnimInstance.GetOwningComponent",8);
    state.reflection_abi_valid=state.get_controller_abi.valid&&state.get_count_abi.valid
      &&state.owning_actor_abi.valid&&state.owning_component_abi.valid;
    auto library=engine::DefaultObject(engine::Class(0x43a9670));
    struct GetController{void* context;int32_t index=0,pad=0;void* result=nullptr;} query{world};
    static_assert(sizeof(GetController)==24&&offsetof(GetController,index)==8&&offsetof(GetController,result)==16);
    struct GetCount{void* context;int32_t result=0,pad=0;} count{world};
    static_assert(sizeof(GetCount)==16&&offsetof(GetCount,result)==8);
    bool reflectedGetter=false;
    if(state.get_controller_abi.valid)reflectedGetter=engine::Event(library,L"/Script/Engine.GameplayStatics.GetPlayerController",&query);
    const bool reflectedCount=state.get_count_abi.valid&&engine::Event(library,L"/Script/Engine.GameplayStatics.GetNumPlayerControllers",&count);
    if(!reflectedCount)count.result=engine::function<int32_t(*)(const void*)>(0x43f2960)(world);
    auto nativeGetter=engine::function<void*(*)(const void*,int32_t)>(0x43f2f80);
    auto nativeZero=nativeGetter(world,0);
    state.reflected_getpc0=reinterpret_cast<uintptr_t>(query.result);state.native_getpc0=reinterpret_cast<uintptr_t>(nativeZero);
    state.getter_agrees=reflectedGetter&&nativeZero==query.result;state.getpc0_is_expected=nativeZero==expectedController;
    state.getpc0_is_fallback=fallbackController&&nativeZero==fallbackController;
    state.native_index_count=count.result;
    if(count.result<0||count.result>1024)return Fail(1405,"original_client_topology_index_count_invalid");
    state.captured_indices=(std::min)(static_cast<unsigned>(count.result),MaxControllers);
    for(unsigned n=0;n<state.captured_indices;++n)state.by_index[n]=InspectController(n?nativeGetter(world,static_cast<int32_t>(n)):nativeZero);
    state.pawn_context_getpc0=reinterpret_cast<uintptr_t>(nativeGetter(expectedPawn,0));
    state.pawn_context_agrees=state.pawn_context_getpc0==state.native_getpc0;
    if(equipmentActor){state.equipment_context_getpc0=reinterpret_cast<uintptr_t>(nativeGetter(equipmentActor,0));state.equipment_context_agrees=state.equipment_context_getpc0==state.native_getpc0;}
    auto instance=Read<void*>(world,0x1d8),context=Read<void*>(instance,0x38);
    state.game_instance=reinterpret_cast<uintptr_t>(instance);state.world_context=reinterpret_cast<uintptr_t>(context);
    state.context_current_world=reinterpret_cast<uintptr_t>(Read<void*>(context,0x2c8));state.world_context_matches=Read<void*>(context,0x2c8)==world;
    auto locals=Read<void**>(instance,0x40);const int localCount=Read<int>(instance,0x48),localCapacity=Read<int>(instance,0x4c);
    state.local_player_count=localCount;
    if(!ValidArray(localCount,localCapacity,locals))return Fail(1406,"original_client_topology_local_player_array_invalid");
    state.captured_local_players=(std::min)(static_cast<unsigned>(localCount),MaxControllers);
    for(unsigned n=0;n<state.captured_local_players;++n){auto player=locals[n];auto& record=state.local_players[n];
      record.player=reinterpret_cast<uintptr_t>(player);record.controller=reinterpret_cast<uintptr_t>(Read<void*>(player,0x38));
      record.valid=original_players::IsA(player,0x3b6e680);
    }
    state.getpc0_matches_first_local_player=localCount>0&&state.local_players[0].controller==state.native_getpc0;
    auto weak=Read<uint8_t*>(world,0x218);const int worldCount=Read<int>(world,0x220),worldCapacity=Read<int>(world,0x224);
    state.world_controller_count=worldCount;
    if(!ValidArray(worldCount,worldCapacity,weak))return Fail(1407,"original_client_topology_world_controller_array_invalid");
    state.captured_world_controllers=(std::min)(static_cast<unsigned>(worldCount),MaxControllers);
    for(unsigned n=0;n<state.captured_world_controllers;++n){
      auto pc=engine::function<void*(*)(const void*)>(0x1d4cfa0)(weak+n*8);state.world_controllers[n]=InspectController(pc);
    }
    auto gameState=Read<void*>(world,0x178);auto players=Read<void**>(gameState,0x480);
    const int playerCount=Read<int>(gameState,0x488),playerCapacity=Read<int>(gameState,0x48c);state.game_state_player_count=playerCount;
    if(!ValidArray(playerCount,playerCapacity,players))return Fail(1408,"original_client_topology_game_state_players_invalid");
    state.captured_state_controllers=(std::min)(static_cast<unsigned>(playerCount),MaxControllers);
    for(unsigned n=0;n<state.captured_state_controllers;++n){
      auto pc=players[n]?engine::function<void*(*)(void*)>(0x4947060)(players[n]):nullptr;state.state_controllers[n]=InspectController(pc);
    }
    if(state.owning_actor_abi.valid&&state.owning_component_abi.valid){
      state.first_person=InspectAnimation(expectedPawn,0xf30);state.third_person=InspectAnimation(expectedPawn,0x4f0);
      state.cosmetic_third_person=InspectAnimation(expectedPawn,0xf40);
    }
    state.initialized=true;state.stage=state.getpc0_is_expected?"original_client_index_zero_matches_active_player":"original_client_index_zero_differs_from_active_player";
    return state.getter_agrees;
}
inline int RecordException(EXCEPTION_POINTERS* pointers){
    state.error=1409;state.stage="original_client_topology_query_exception";
    const auto at=reinterpret_cast<uintptr_t>(pointers&&pointers->ExceptionRecord?pointers->ExceptionRecord->ExceptionAddress:nullptr);
    state.exception_rva=at>=engine::image&&at-engine::image<0x10000000?at-engine::image:0;return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Probe(void* world,void* expectedController,void* expectedPawn,void* fallbackController=nullptr,void* fallbackPawn=nullptr,void* equipmentActor=nullptr){
    __try{return ProbeImpl(world,expectedController,expectedPawn,fallbackController,fallbackPawn,equipmentActor);}
    __except(RecordException(GetExceptionInformation())){return false;}
}
struct RegistrationList {
    uintptr_t data=0;
    int32_t count=0,capacity=0;
    bool valid=false;
    std::array<uint64_t,MaxControllers> weak{};
    std::array<uintptr_t,MaxControllers> controllers{};
};
struct DetachDiagnostics {
    bool attempted=false,entry_verified=false,native_call_performed=false,already_detached=false,success=false;
    bool bootstrap_player_null=false,sole_local_player_valid=false,bootstrap_links_preserved=false,active_links_preserved=false;
    bool all_controller_order_preserved=false,player_controller_order_preserved=false,first_controller_is_active=false;
    bool bootstrap_actor_alive=false,bootstrap_pawn_alive=false;
    unsigned error=0;
    uintptr_t world=0,active_controller=0,active_pawn=0,bootstrap_controller=0,bootstrap_pawn=0,local_player=0,exception_rva=0;
    uintptr_t bootstrap_controller_vtable=0,bootstrap_pawn_vtable=0;
    uint64_t calls=0;
    const char* stage="original_bootstrap_controller_not_detached";
    RegistrationList all_before{},all_after{},players_before{},players_after{};
};
inline DetachDiagnostics detach;
inline const DetachDiagnostics& DetachState(){return detach;}
inline bool DetachFail(unsigned code,const char* stage){detach.error=code;detach.stage=stage;return false;}
inline RegistrationList Registration(void* world,size_t offset){
    RegistrationList result;auto data=Read<uint8_t*>(world,offset);result.data=reinterpret_cast<uintptr_t>(data);
    result.count=Read<int32_t>(world,offset+8);result.capacity=Read<int32_t>(world,offset+12);
    result.valid=ValidArray(result.count,result.capacity,data)&&result.count<=static_cast<int>(MaxControllers);
    if(!result.valid)return result;
    for(int n=0;n<result.count;++n){result.weak[n]=Read<uint64_t>(data,n*8);
      result.controllers[n]=reinterpret_cast<uintptr_t>(engine::function<void*(*)(const void*)>(0x1d4cfa0)(data+n*8));}
    return result;
}
inline unsigned Occurrences(const RegistrationList& list,void* controller){unsigned result=0;
    for(int n=0;n<list.count&&n<static_cast<int>(MaxControllers);++n)if(list.controllers[n]==reinterpret_cast<uintptr_t>(controller))++result;return result;}
inline bool OriginalRemovalOrder(const RegistrationList& before,const RegistrationList& after,void* removed){
    if(!before.valid||!after.valid||before.count!=after.count+1)return false;int out=0;
    for(int n=0;n<before.count;++n){if(before.controllers[n]==reinterpret_cast<uintptr_t>(removed))continue;
      if(out>=after.count||before.weak[n]!=after.weak[out]||before.controllers[n]!=after.controllers[out])return false;++out;}
    return out==after.count;
}
inline void* ActorWorld(void* actor){auto table=Read<void**>(actor,0);auto entry=Read<void*>(table,0x1b8);
    return entry?reinterpret_cast<void*(*)(void*)>(entry)(actor):nullptr;}
inline bool DetachBootstrapControllerImpl(void* world,void* activePC,void* activePawn,void* oldPC,void* oldPawn){
    const auto calls=detach.calls;detach=DetachDiagnostics{};detach.calls=calls;detach.attempted=true;
    detach.world=reinterpret_cast<uintptr_t>(world);detach.active_controller=reinterpret_cast<uintptr_t>(activePC);
    detach.active_pawn=reinterpret_cast<uintptr_t>(activePawn);detach.bootstrap_controller=reinterpret_cast<uintptr_t>(oldPC);detach.bootstrap_pawn=reinterpret_cast<uintptr_t>(oldPawn);
    if(!engine::image||original_players::gameThread!=GetCurrentThreadId())return DetachFail(1451,"original_bootstrap_detach_wrong_thread");
    if(!world||world!=original_players::world||activePC!=original_players::activeController||activePawn!=original_players::activePawn
      ||oldPC!=original_players::fallbackController||oldPawn!=original_players::fallbackPawn||activePC==oldPC||activePawn==oldPawn)
      return DetachFail(1452,"original_bootstrap_detach_context_mismatch");
    if(!original_players::IsA(activePC,original_players::ShooterControllerClass)||!original_players::IsA(activePawn,original_players::ShooterCharacterClass)
      ||Read<void*>(oldPC,0x10)!=engine::Class(0x3c0a200)||!oldPawn||ActorWorld(activePC)!=world||ActorWorld(oldPC)!=world
      ||ActorWorld(activePawn)!=world||ActorWorld(oldPawn)!=world)
      return DetachFail(1453,"original_bootstrap_detach_actor_identity_invalid");
    if(Read<void*>(activePC,0x4b0)!=activePawn||Read<void*>(activePawn,0x4a0)!=activePC
      ||Read<void*>(oldPC,0x4b0)!=oldPawn||Read<void*>(oldPawn,0x4a0)!=oldPC)
      return DetachFail(1454,"original_bootstrap_detach_possession_pair_invalid");
    detach.bootstrap_controller_vtable=reinterpret_cast<uintptr_t>(Read<void*>(oldPC,0));detach.bootstrap_pawn_vtable=reinterpret_cast<uintptr_t>(Read<void*>(oldPawn,0));
    detach.bootstrap_player_null=Read<void*>(oldPC,0x510)==nullptr;
    auto instance=Read<void*>(world,0x1d8);auto locals=Read<void**>(instance,0x40);
    const int count=Read<int>(instance,0x48),capacity=Read<int>(instance,0x4c);
    if(!detach.bootstrap_player_null||!ValidArray(count,capacity,locals)||count!=1)
      return DetachFail(1455,"original_bootstrap_detach_not_single_local_player");
    auto player=locals[0];detach.local_player=reinterpret_cast<uintptr_t>(player);
    detach.sole_local_player_valid=original_players::IsA(player,0x3b6e680)&&Read<void*>(player,0x38)==activePC
      &&Read<void*>(activePC,0x510)==player&&player==original_players::localPlayer;
    if(!detach.sole_local_player_valid)return DetachFail(1456,"original_bootstrap_detach_local_player_link_invalid");
    detach.all_before=Registration(world,0x208);detach.players_before=Registration(world,0x218);
    if(!detach.all_before.valid||!detach.players_before.valid)return DetachFail(1457,"original_bootstrap_detach_registration_invalid");
    const auto oldAll=Occurrences(detach.all_before,oldPC),oldPlayers=Occurrences(detach.players_before,oldPC);
    if(oldAll==0&&oldPlayers==0&&detach.players_before.count>0&&detach.players_before.controllers[0]==reinterpret_cast<uintptr_t>(activePC)){
      detach.already_detached=true;detach.all_after=detach.all_before;detach.players_after=detach.players_before;
    }else{
      if(oldAll!=1||oldPlayers!=1||Occurrences(detach.all_before,activePC)!=1||Occurrences(detach.players_before,activePC)!=1
        ||detach.players_before.count<2||detach.players_before.controllers[0]!=reinterpret_cast<uintptr_t>(oldPC)
        ||detach.players_before.controllers[1]!=reinterpret_cast<uintptr_t>(activePC))
        return DetachFail(1458,"original_bootstrap_detach_expected_order_invalid");
      constexpr uint8_t signature[]{0x48,0x89,0x54,0x24,0x10,0x55,0x56,0x41,0x57,0x48,0x83,0xec,0x40,0x45,0x33,0xff};
      constexpr uint8_t caller[]{0xe8,0x34,0x25,0xa2,0x00};
      detach.entry_verified=std::memcmp(reinterpret_cast<const void*>(engine::image+0x4cd14c0),signature,sizeof(signature))==0
        &&std::memcmp(reinterpret_cast<const void*>(engine::image+0x42aef87),caller,sizeof(caller))==0;
      if(!detach.entry_verified)return DetachFail(1459,"original_bootstrap_detach_native_entry_mismatch");
      detach.native_call_performed=true;++detach.calls;detach.stage="original_bootstrap_detach_calling_original_remove_controller";
      // Exact native call used by AController.v660/42aef60. It removes the two
      // weak registrations in stable order and performs its original local
      // deregistration notification. Neither Actor nor Pawn is destroyed.
      engine::function<void(*)(void*,void*)>(0x4cd14c0)(world,oldPC);
      detach.all_after=Registration(world,0x208);detach.players_after=Registration(world,0x218);
    }
    detach.all_controller_order_preserved=detach.already_detached||OriginalRemovalOrder(detach.all_before,detach.all_after,oldPC);
    detach.player_controller_order_preserved=detach.already_detached||OriginalRemovalOrder(detach.players_before,detach.players_after,oldPC);
    detach.first_controller_is_active=detach.players_after.valid&&detach.players_after.count>0
      &&detach.players_after.controllers[0]==reinterpret_cast<uintptr_t>(activePC)&&Read<void*>(activePC,0x510)==player;
    detach.bootstrap_actor_alive=Read<void*>(oldPC,0x10)==engine::Class(0x3c0a200)
      &&Read<void*>(oldPC,0)==reinterpret_cast<void*>(detach.bootstrap_controller_vtable)&&ActorWorld(oldPC)==world;
    detach.bootstrap_pawn_alive=Read<void*>(oldPawn,0)==reinterpret_cast<void*>(detach.bootstrap_pawn_vtable)&&ActorWorld(oldPawn)==world;
    detach.bootstrap_links_preserved=Read<void*>(oldPC,0x4b0)==oldPawn&&Read<void*>(oldPawn,0x4a0)==oldPC&&Read<void*>(oldPC,0x510)==nullptr;
    detach.active_links_preserved=Read<void*>(activePC,0x4b0)==activePawn&&Read<void*>(activePawn,0x4a0)==activePC
      &&Read<void*>(activePC,0x510)==player&&Read<void*>(player,0x38)==activePC&&Read<void*>(instance,0x40)==locals&&Read<int>(instance,0x48)==1&&locals[0]==player;
    detach.success=detach.all_controller_order_preserved&&detach.player_controller_order_preserved&&detach.first_controller_is_active
      &&detach.bootstrap_actor_alive&&detach.bootstrap_pawn_alive&&detach.bootstrap_links_preserved&&detach.active_links_preserved
      &&Occurrences(detach.all_after,oldPC)==0&&Occurrences(detach.players_after,oldPC)==0;
    if(!detach.success)return DetachFail(1460,"original_bootstrap_detach_postcondition_failed");
    detach.stage=detach.already_detached?"original_bootstrap_controller_already_detached":"original_bootstrap_controller_detached_actor_preserved";return true;
}
inline int DetachException(EXCEPTION_POINTERS* pointers){detach.error=1461;detach.stage="original_bootstrap_detach_native_exception";
    const auto at=reinterpret_cast<uintptr_t>(pointers&&pointers->ExceptionRecord?pointers->ExceptionRecord->ExceptionAddress:nullptr);
    detach.exception_rva=at>=engine::image&&at-engine::image<0x10000000?at-engine::image:0;return EXCEPTION_EXECUTE_HANDLER;}
inline bool DetachBootstrapController(void* world,void* activePC,void* activePawn,void* oldPC,void* oldPawn){
    __try{return DetachBootstrapControllerImpl(world,activePC,activePawn,oldPC,oldPawn);}
    __except(DetachException(GetExceptionInformation())){return false;}
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;const auto& s=state;
    std::fprintf(file,"{\"schema_version\":2,\"stage\":\"%s\",\"error\":%u,\"mutations_performed\":%s,\"cleanup_performed\":%s",s.stage,s.error,detach.native_call_performed?"true":"false",detach.success?"true":"false");
#define LIFECYCLE_BOOL(k) std::fprintf(file,",\"" #k "\":%s",s.k?"true":"false")
#define LIFECYCLE_INT(k) std::fprintf(file,",\"" #k "\":%lld",static_cast<long long>(s.k))
    LIFECYCLE_BOOL(initialized);LIFECYCLE_BOOL(reflection_abi_valid);LIFECYCLE_BOOL(getter_agrees);LIFECYCLE_BOOL(world_context_matches);
    LIFECYCLE_BOOL(getpc0_is_expected);LIFECYCLE_BOOL(getpc0_is_fallback);LIFECYCLE_BOOL(getpc0_matches_first_local_player);
    LIFECYCLE_BOOL(pawn_context_agrees);LIFECYCLE_BOOL(equipment_context_agrees);
    LIFECYCLE_INT(exception_rva);LIFECYCLE_INT(world);LIFECYCLE_INT(game_instance);LIFECYCLE_INT(world_context);LIFECYCLE_INT(context_current_world);
    LIFECYCLE_INT(expected_controller);LIFECYCLE_INT(expected_pawn);LIFECYCLE_INT(fallback_controller);LIFECYCLE_INT(fallback_pawn);
    LIFECYCLE_INT(native_getpc0);LIFECYCLE_INT(reflected_getpc0);LIFECYCLE_INT(pawn_context_getpc0);LIFECYCLE_INT(equipment_context_getpc0);
    LIFECYCLE_INT(native_index_count);LIFECYCLE_INT(world_controller_count);LIFECYCLE_INT(game_state_player_count);LIFECYCLE_INT(local_player_count);
    LIFECYCLE_INT(probes);LIFECYCLE_INT(observed_at_ms);
    const auto records=[&](const char* name,const auto& array,unsigned count){
      std::fprintf(file,",\"%s\":[",name);
      for(unsigned n=0;n<count;++n){if(n)std::fputc(',',file);const auto& item=array[n];std::fprintf(file,"{\"index\":%u",n);
#define LIFECYCLE_PC_INT(k) std::fprintf(file,",\"" #k "\":%llu",static_cast<unsigned long long>(item.k))
#define LIFECYCLE_PC_BOOL(k) std::fprintf(file,",\"" #k "\":%s",item.k?"true":"false")
        LIFECYCLE_PC_INT(controller);LIFECYCLE_PC_INT(pawn);LIFECYCLE_PC_INT(player_state);LIFECYCLE_PC_INT(player);LIFECYCLE_PC_INT(player_backlink);
        LIFECYCLE_PC_INT(camera);LIFECYCLE_PC_INT(view_target);LIFECYCLE_PC_BOOL(valid);LIFECYCLE_PC_BOOL(is_expected);LIFECYCLE_PC_BOOL(is_fallback);
        LIFECYCLE_PC_BOOL(has_local_player);LIFECYCLE_PC_BOOL(local_player_backlink_valid);LIFECYCLE_PC_BOOL(native_local_controller_cached);
#undef LIFECYCLE_PC_INT
#undef LIFECYCLE_PC_BOOL
        std::fputc('}',file);
      }std::fputc(']',file);
    };
    records("by_original_index",s.by_index,s.captured_indices);records("raw_world_controller_list",s.world_controllers,s.captured_world_controllers);
    records("game_state_player_controllers",s.state_controllers,s.captured_state_controllers);
    std::fprintf(file,",\"game_instance_local_players\":[");
    for(unsigned n=0;n<s.captured_local_players;++n){if(n)std::fputc(',',file);const auto& p=s.local_players[n];
      std::fprintf(file,"{\"index\":%u,\"player\":%llu,\"controller\":%llu,\"valid\":%s}",n,static_cast<unsigned long long>(p.player),static_cast<unsigned long long>(p.controller),p.valid?"true":"false");}
    std::fputc(']',file);
    const auto animation=[&](const char* name,const AnimationRecord& a){std::fprintf(file,",\"%s\":{\"mesh\":%llu,\"instance\":%llu,\"owning_actor\":%llu,\"owning_component\":%llu,\"query_valid\":%s,\"actor_matches\":%s,\"component_matches\":%s}",name,static_cast<unsigned long long>(a.mesh),static_cast<unsigned long long>(a.instance),static_cast<unsigned long long>(a.owning_actor),static_cast<unsigned long long>(a.owning_component),a.query_valid?"true":"false",a.actor_matches?"true":"false",a.component_matches?"true":"false");};
    animation("first_person_animation",s.first_person);animation("third_person_animation",s.third_person);animation("cosmetic_third_person_animation",s.cosmetic_third_person);
    const auto abi=[&](const char* name,const ABIRecord& a){std::fprintf(file,",\"%s\":{\"expected\":%u,\"actual\":%u,\"found\":%s,\"valid\":%s}",name,a.expected,a.actual,a.found?"true":"false",a.valid?"true":"false");};
    abi("get_player_controller_abi",s.get_controller_abi);abi("get_num_controllers_abi",s.get_count_abi);
    abi("anim_owning_actor_abi",s.owning_actor_abi);abi("anim_owning_component_abi",s.owning_component_abi);
    const auto& d=detach;
    std::fprintf(file,",\"bootstrap_detach\":{\"stage\":\"%s\",\"error\":%u,\"original_remove_controller_rva\":80549056",d.stage,d.error);
#define LIFECYCLE_DETACH_BOOL(k) std::fprintf(file,",\"" #k "\":%s",d.k?"true":"false")
#define LIFECYCLE_DETACH_INT(k) std::fprintf(file,",\"" #k "\":%llu",static_cast<unsigned long long>(d.k))
    LIFECYCLE_DETACH_BOOL(attempted);LIFECYCLE_DETACH_BOOL(entry_verified);LIFECYCLE_DETACH_BOOL(native_call_performed);
    LIFECYCLE_DETACH_BOOL(already_detached);LIFECYCLE_DETACH_BOOL(success);LIFECYCLE_DETACH_BOOL(bootstrap_player_null);
    LIFECYCLE_DETACH_BOOL(sole_local_player_valid);LIFECYCLE_DETACH_BOOL(bootstrap_links_preserved);LIFECYCLE_DETACH_BOOL(active_links_preserved);
    LIFECYCLE_DETACH_BOOL(all_controller_order_preserved);LIFECYCLE_DETACH_BOOL(player_controller_order_preserved);LIFECYCLE_DETACH_BOOL(first_controller_is_active);
    LIFECYCLE_DETACH_BOOL(bootstrap_actor_alive);LIFECYCLE_DETACH_BOOL(bootstrap_pawn_alive);
    LIFECYCLE_DETACH_INT(world);LIFECYCLE_DETACH_INT(active_controller);LIFECYCLE_DETACH_INT(active_pawn);
    LIFECYCLE_DETACH_INT(bootstrap_controller);LIFECYCLE_DETACH_INT(bootstrap_pawn);LIFECYCLE_DETACH_INT(local_player);
    LIFECYCLE_DETACH_INT(calls);LIFECYCLE_DETACH_INT(exception_rva);
#undef LIFECYCLE_DETACH_BOOL
#undef LIFECYCLE_DETACH_INT
    const auto registration=[&](const char* name,const RegistrationList& list){
      std::fprintf(file,",\"%s\":{\"valid\":%s,\"data\":%llu,\"count\":%d,\"capacity\":%d,\"entries\":[",name,list.valid?"true":"false",static_cast<unsigned long long>(list.data),list.count,list.capacity);
      for(int n=0;n<list.count&&n<static_cast<int>(MaxControllers);++n){if(n)std::fputc(',',file);std::fprintf(file,"{\"weak\":%llu,\"controller\":%llu}",static_cast<unsigned long long>(list.weak[n]),static_cast<unsigned long long>(list.controllers[n]));}
      std::fprintf(file,"]}");
    };
    registration("all_controllers_before",d.all_before);registration("all_controllers_after",d.all_after);
    registration("player_controllers_before",d.players_before);registration("player_controllers_after",d.players_after);
    std::fputc('}',file);
    std::fprintf(file,"}\n");std::fclose(file);
#undef LIFECYCLE_BOOL
#undef LIFECYCLE_INT
    return true;
}
}
