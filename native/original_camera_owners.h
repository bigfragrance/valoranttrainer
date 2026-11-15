#pragma once
#include "original_client_lifecycle.h"
#include "original_enemy_respawn.h"

// The shipped UWorld camera loops enumerate World.PlayerControllerList, not
// Actor Tick. A standalone authority NPC still owns its real ShooterPC/PS/Pawn,
// but it has no ULocalPlayer and must not update the local player's camera rig.
// Only original UWorld.RemoveController4cd14c0 mutates registration here.
// Sample can observe an original dead/unpossessed corpse while its caller waits
// to recreate the NPC; it never repairs possession or writes life/owner flags.
namespace original_camera_owners {
using RegistrationList=original_client_lifecycle::RegistrationList;
inline constexpr size_t MaxPlayers=original_players::MaxPlayers;
inline constexpr unsigned MaxControllers=original_client_lifecycle::MaxControllers;
struct ActorSnapshot {
    void *object=nullptr,*type=nullptr,*table=nullptr,*owner=nullptr,*root=nullptr;
    uint32_t tickFlags=0;
    uint8_t role=0,remoteRole=0;
};
struct RosterSnapshot {
    void *gameState=nullptr,*data=nullptr;
    int32_t count=0,capacity=0;
    bool valid=false;
    std::array<void*,MaxControllers> states{};
};
struct Entry {
    bool attempted=false,applied=false,alreadyDetached=false,nativeEntryVerified=false;
    bool noPlayer=false,notInLocalPlayers=false,possessionPreserved=false,actorStatePreserved=false;
    bool gameStateRosterPreserved=false,allOrderPreserved=false,playerOrderPreserved=false;
    bool localLinksPreserved=false,onlyLocalCameraOwner=false,currentLinksValid=false;
    bool fullPossessionLinks=false,deadUnpossessedWait=false,deathRosterPreserved=false;
    unsigned error=0;
    uint64_t calls=0,samples=0,deadWaitSamples=0;
    const char* stage="npc_camera_owner_not_observed";
    void *controller=nullptr,*pawn=nullptr,*playerState=nullptr,*camera=nullptr,*currentPawn=nullptr;
    void *currentControllerPawn=nullptr,*currentPawnController=nullptr,*currentPawnState=nullptr;
    original_enemy_respawn::Life deathLife{};
    ActorSnapshot pcBefore{},pawnBefore{},stateBefore{};
    RosterSnapshot rosterBefore{},rosterAfter{};
    RegistrationList allBefore{},allAfter{},playersBefore{},playersAfter{};
};
struct Diagnostics {
    bool initialized=false,soleLocalPlayerValid=false,onlyLocalCameraOwner=false;
    unsigned error=0;
    const char* stage="original_camera_owners_unbound";
    uintptr_t exceptionRva=0;
    void *world=nullptr,*activeController=nullptr,*activePawn=nullptr,*localPlayer=nullptr;
    uint64_t nativeCalls=0,samples=0;
    unsigned deathWaitingTargets=0;
    RegistrationList currentAll{},currentPlayers{};
    RosterSnapshot expectedRoster{};
    std::array<Entry,MaxPlayers> entries{};
};
inline Diagnostics state;
inline const Diagnostics& State(){return state;}
template<class T> inline T Value(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;return false;}
inline bool EntryFail(Entry& entry,unsigned error,const char* stage){entry.error=error;entry.stage=stage;return Fail(error,stage);}
inline ActorSnapshot Actor(void* object){
    ActorSnapshot out;out.object=object;out.type=Value<void*>(object,0x10);out.table=Value<void*>(object,0);
    out.owner=Value<void*>(object,0x210);out.root=Value<void*>(object,0x290);
    out.tickFlags=Value<uint32_t>(object,0x38);out.role=Value<uint8_t>(object,0x224);out.remoteRole=Value<uint8_t>(object,0x128);return out;
}
inline bool SameActor(const ActorSnapshot& before){
    const auto after=Actor(before.object);
    return before.object&&before.type==after.type&&before.table==after.table&&before.owner==after.owner
      &&before.root==after.root&&before.tickFlags==after.tickFlags&&before.role==after.role&&before.remoteRole==after.remoteRole
      &&original_client_lifecycle::ActorWorld(before.object)==state.world;
}
inline RosterSnapshot Roster(){
    RosterSnapshot out;out.gameState=Value<void*>(state.world,0x178);out.data=Value<void*>(out.gameState,0x480);
    out.count=Value<int32_t>(out.gameState,0x488);out.capacity=Value<int32_t>(out.gameState,0x48c);
    out.valid=out.gameState&&original_client_lifecycle::ValidArray(out.count,out.capacity,out.data)
      &&out.count<=static_cast<int32_t>(MaxControllers);
    if(out.valid)for(int32_t i=0;i<out.count;++i)out.states[i]=Value<void*>(out.data,static_cast<size_t>(i)*8);
    return out;
}
inline unsigned StateOccurrences(const RosterSnapshot& roster,void* playerState){
    unsigned count=0;if(roster.valid)for(int32_t i=0;i<roster.count;++i)if(roster.states[i]==playerState)++count;return count;
}
inline bool SameRoster(const RosterSnapshot& before,const RosterSnapshot& after){
    if(!before.valid||!after.valid||before.gameState!=after.gameState||before.data!=after.data
      ||before.count!=after.count||before.capacity!=after.capacity)return false;
    for(int32_t i=0;i<before.count;++i)if(before.states[i]!=after.states[i])return false;return true;
}
inline bool Bind(){
    if(!original_players::Thread())return Fail(1780,"original_camera_owners_wrong_thread");
    auto world=original_players::world,active=original_players::activeController,pawn=original_players::activePawn;
    if(!world||!active||!pawn||original_players::localIndex<0
      ||!original_players::IsA(active,original_players::ShooterControllerClass)
      ||!original_players::IsA(pawn,original_players::ShooterCharacterClass)
      ||Value<void*>(active,0x4b0)!=pawn||Value<void*>(pawn,0x4a0)!=active)
      return Fail(1781,"original_camera_owners_active_pair_invalid");
    if(state.initialized&&(state.world!=world||state.activeController!=active||state.localPlayer!=original_players::localPlayer))
      return Fail(1782,"original_camera_owners_session_changed");
    state.world=world;state.activeController=active;state.activePawn=pawn;state.localPlayer=original_players::localPlayer;
    auto instance=Value<void*>(world,0x1d8),locals=Value<void*>(instance,0x40);
    const int32_t count=Value<int32_t>(instance,0x48),capacity=Value<int32_t>(instance,0x4c);
    state.soleLocalPlayerValid=original_client_lifecycle::ValidArray(count,capacity,locals)&&count==1
      &&Value<void*>(locals,0)==state.localPlayer&&original_players::IsA(state.localPlayer,0x3b6e680)
      &&Value<void*>(state.localPlayer,0x38)==active&&Value<void*>(active,0x510)==state.localPlayer;
    if(!state.soleLocalPlayerValid)return Fail(1783,"original_camera_owners_local_player_link_invalid");
    state.initialized=true;return true;
}
inline bool NotInLocalPlayers(void* controller){
    auto instance=Value<void*>(state.world,0x1d8),locals=Value<void*>(instance,0x40);
    const auto count=Value<int32_t>(instance,0x48),capacity=Value<int32_t>(instance,0x4c);
    if(!original_client_lifecycle::ValidArray(count,capacity,locals)||count!=1)return false;
    for(int32_t n=0;n<count;++n)if(Value<void*>(Value<void*>(locals,static_cast<size_t>(n)*8),0x38)==controller)return false;
    return true;
}
inline bool ApplyImpl(size_t index){
    if(index>=MaxPlayers)return Fail(1784,"original_camera_owners_index_invalid");
    auto& entry=state.entries[index];entry.attempted=true;
    if(!Bind())return false;
    const auto& player=original_players::players[index];
    if(static_cast<int>(index)==original_players::localIndex||player.local||!player.ready
      ||!player.controller||player.controller==state.activeController||!player.character||!player.state
      ||!original_players::IsA(player.controller,original_players::ShooterControllerClass)
      ||!original_players::IsA(player.character,original_players::ShooterCharacterClass)
      ||!original_players::IsA(player.state,original_players::ShooterStateClass))
      return EntryFail(entry,1785,"original_camera_owners_requires_ready_npc");
    if(entry.controller&&entry.controller!=player.controller)return EntryFail(entry,1786,"original_camera_owners_controller_replaced");
    entry.controller=player.controller;entry.pawn=entry.currentPawn=player.character;entry.playerState=player.state;
    entry.camera=Value<void*>(entry.controller,0x528);
    entry.noPlayer=Value<void*>(entry.controller,0x510)==nullptr;
    entry.notInLocalPlayers=NotInLocalPlayers(entry.controller);
    if(!entry.noPlayer||!entry.notInLocalPlayers)return EntryFail(entry,1787,"original_camera_owners_npc_has_local_player");
    if(Value<void*>(entry.controller,0x4b0)!=entry.pawn||Value<void*>(entry.pawn,0x4a0)!=entry.controller
      ||Value<void*>(entry.controller,0x470)!=entry.playerState||Value<void*>(entry.pawn,0x488)!=entry.playerState
      ||original_client_lifecycle::ActorWorld(entry.controller)!=state.world
      ||original_client_lifecycle::ActorWorld(entry.pawn)!=state.world)
      return EntryFail(entry,1788,"original_camera_owners_npc_links_invalid");
    entry.pcBefore=Actor(entry.controller);entry.pawnBefore=Actor(entry.pawn);entry.stateBefore=Actor(entry.playerState);
    entry.rosterBefore=Roster();entry.allBefore=original_client_lifecycle::Registration(state.world,0x208);
    entry.playersBefore=original_client_lifecycle::Registration(state.world,0x218);
    if(!entry.rosterBefore.valid||StateOccurrences(entry.rosterBefore,entry.playerState)!=1
      ||!entry.allBefore.valid||!entry.playersBefore.valid)
      return EntryFail(entry,1789,"original_camera_owners_registration_invalid");
    const auto allCount=original_client_lifecycle::Occurrences(entry.allBefore,entry.controller);
    const auto pcCount=original_client_lifecycle::Occurrences(entry.playersBefore,entry.controller);
    if(allCount==0&&pcCount==0){entry.alreadyDetached=true;entry.allAfter=entry.allBefore;entry.playersAfter=entry.playersBefore;}
    else{
      if(allCount!=1||pcCount!=1||entry.playersBefore.count!=2
        ||entry.playersBefore.controllers[0]!=reinterpret_cast<uintptr_t>(state.activeController)
        ||entry.playersBefore.controllers[1]!=reinterpret_cast<uintptr_t>(entry.controller)
        ||original_client_lifecycle::Occurrences(entry.allBefore,state.activeController)!=1)
        return EntryFail(entry,1790,"original_camera_owners_expected_local_npc_order_invalid");
      constexpr uint8_t signature[]{0x48,0x89,0x54,0x24,0x10,0x55,0x56,0x41,0x57,0x48,0x83,0xec,0x40,0x45,0x33,0xff};
      entry.nativeEntryVerified=std::memcmp(reinterpret_cast<const void*>(engine::image+0x4cd14c0),signature,sizeof(signature))==0;
      if(!entry.nativeEntryVerified)return EntryFail(entry,1791,"original_camera_owners_native_entry_mismatch");
      ++entry.calls;++state.nativeCalls;entry.stage="original_camera_owners_removing_original_world_registration";
      engine::function<void(*)(void*,void*)>(0x4cd14c0)(state.world,entry.controller);
      entry.allAfter=original_client_lifecycle::Registration(state.world,0x208);
      entry.playersAfter=original_client_lifecycle::Registration(state.world,0x218);
    }
    entry.rosterAfter=Roster();
    entry.allOrderPreserved=entry.alreadyDetached||original_client_lifecycle::OriginalRemovalOrder(entry.allBefore,entry.allAfter,entry.controller);
    entry.playerOrderPreserved=entry.alreadyDetached||original_client_lifecycle::OriginalRemovalOrder(entry.playersBefore,entry.playersAfter,entry.controller);
    entry.possessionPreserved=Value<void*>(entry.controller,0x4b0)==entry.pawn&&Value<void*>(entry.pawn,0x4a0)==entry.controller
      &&Value<void*>(entry.controller,0x470)==entry.playerState&&Value<void*>(entry.pawn,0x488)==entry.playerState
      &&Value<void*>(entry.controller,0x510)==nullptr&&Value<void*>(entry.controller,0x528)==entry.camera;
    entry.actorStatePreserved=SameActor(entry.pcBefore)&&SameActor(entry.pawnBefore)&&SameActor(entry.stateBefore);
    entry.gameStateRosterPreserved=SameRoster(entry.rosterBefore,entry.rosterAfter);
    entry.localLinksPreserved=Value<void*>(state.activeController,0x4b0)==state.activePawn
      &&Value<void*>(state.activePawn,0x4a0)==state.activeController&&Value<void*>(state.activeController,0x510)==state.localPlayer
      &&Value<void*>(state.localPlayer,0x38)==state.activeController;
    entry.onlyLocalCameraOwner=entry.playersAfter.valid&&entry.playersAfter.count==1
      &&entry.playersAfter.controllers[0]==reinterpret_cast<uintptr_t>(state.activeController)
      &&original_client_lifecycle::Occurrences(entry.allAfter,entry.controller)==0;
    entry.applied=entry.allOrderPreserved&&entry.playerOrderPreserved&&entry.possessionPreserved&&entry.actorStatePreserved
      &&entry.gameStateRosterPreserved&&entry.localLinksPreserved&&entry.onlyLocalCameraOwner;
    if(!entry.applied)return EntryFail(entry,1792,"original_camera_owners_original_removal_postcondition_failed");
    entry.currentLinksValid=true;entry.fullPossessionLinks=true;entry.deadUnpossessedWait=false;entry.deathRosterPreserved=false;entry.deathLife={};
    entry.currentControllerPawn=Value<void*>(entry.controller,0x4b0);entry.currentPawnController=Value<void*>(entry.pawn,0x4a0);entry.currentPawnState=Value<void*>(entry.pawn,0x488);
    state.expectedRoster=entry.rosterAfter;
    entry.error=0;entry.stage="original_npc_world_camera_registration_removed_actors_preserved";
    state.error=0;state.stage=entry.stage;return true;
}
inline int Exception(EXCEPTION_POINTERS* pointers){
    state.error=1793;state.stage="original_camera_owners_native_exception";
    const auto at=reinterpret_cast<uintptr_t>(pointers&&pointers->ExceptionRecord?pointers->ExceptionRecord->ExceptionAddress:nullptr);
    state.exceptionRva=at>=engine::image&&at-engine::image<0x10000000?at-engine::image:0;return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Apply(size_t index){__try{return ApplyImpl(index);}__except(Exception(GetExceptionInformation())){return false;}}
inline bool DetachFromCameraLoops(size_t index){return Apply(index);}
inline bool SampleImpl(){
    if(!Bind())return false;
    state.currentAll=original_client_lifecycle::Registration(state.world,0x208);
    state.currentPlayers=original_client_lifecycle::Registration(state.world,0x218);
    state.onlyLocalCameraOwner=state.currentAll.valid&&state.currentPlayers.valid&&state.currentAll.count==1&&state.currentPlayers.count==1
      &&state.currentAll.controllers[0]==reinterpret_cast<uintptr_t>(state.activeController)
      &&state.currentPlayers.controllers[0]==reinterpret_cast<uintptr_t>(state.activeController);
    if(!state.onlyLocalCameraOwner)return Fail(1794,"original_camera_owners_world_camera_list_not_local_only");
    const auto roster=Roster();if(!roster.valid)return Fail(1795,"original_camera_owners_current_roster_invalid");
    state.deathWaitingTargets=0;
    for(size_t i=0;i<MaxPlayers;++i){const auto& player=original_players::players[i];
      if(!player.ready||static_cast<int>(i)==original_players::localIndex||player.local)continue;
      auto& entry=state.entries[i];entry.currentPawn=player.character;
      const bool previouslyWaiting=entry.deadUnpossessedWait;
      entry.deadUnpossessedWait=false;entry.deathRosterPreserved=false;entry.deathLife={};
      entry.currentControllerPawn=Value<void*>(player.controller,0x4b0);
      entry.currentPawnController=Value<void*>(player.character,0x4a0);
      entry.currentPawnState=Value<void*>(player.character,0x488);
      const bool ownerAndRosterValid=entry.applied&&entry.controller==player.controller&&entry.playerState==player.state
        &&Value<void*>(player.controller,0x510)==nullptr&&NotInLocalPlayers(player.controller)
        &&Value<void*>(player.controller,0x470)==player.state
        &&StateOccurrences(roster,player.state)==1
        &&original_client_lifecycle::Occurrences(state.currentAll,player.controller)==0
        &&original_client_lifecycle::Occurrences(state.currentPlayers,player.controller)==0;
      entry.fullPossessionLinks=entry.currentControllerPawn==player.character&&entry.currentPawnController==player.controller
        &&entry.currentPawnState==player.state;
      entry.currentLinksValid=ownerAndRosterValid&&entry.fullPossessionLinks;
      if(ownerAndRosterValid&&!entry.fullPossessionLinks){
        // Original death observers may release possession before the caller's
        // missing local character factory recreates the victim. This read-only
        // wait accepts only the same previously verified corpse and null/original
        // links, never a controller/pawn now associated with another actor.
        entry.deathRosterPreserved=SameRoster(state.expectedRoster,roster);
        const bool expectedReleasedPair=player.character==entry.pawn&&player.character
          &&(!entry.currentControllerPawn||entry.currentControllerPawn==player.character)
          &&(!entry.currentPawnController||entry.currentPawnController==player.controller)
          &&(!entry.currentPawnState||entry.currentPawnState==player.state)
          &&Value<void*>(player.state,0x210)==player.controller;
        if(expectedReleasedPair&&entry.deathRosterPreserved
          &&original_enemy_respawn::ReadLife(player.character,entry.deathLife)
          &&original_enemy_respawn::Dead(entry.deathLife)&&entry.deathLife.health<=0.0f
          &&Value<void*>(entry.deathLife.handler,0xc8)==player.character){
          entry.deadUnpossessedWait=true;entry.currentLinksValid=true;
          ++entry.deadWaitSamples;++state.deathWaitingTargets;
          entry.stage="original_dead_npc_unpossessed_waiting_for_native_respawn";
        }
      }
      if(entry.currentLinksValid){entry.error=0;if(previouslyWaiting&&!entry.deadUnpossessedWait)entry.stage="original_npc_full_possession_links_restored";}
      ++entry.samples;if(!entry.currentLinksValid)return EntryFail(entry,1796,"original_camera_owners_npc_current_links_invalid");
    }
    ++state.samples;state.error=0;state.stage="original_world_camera_loops_have_only_real_local_player";return true;
}
inline bool Sample(){__try{return SampleImpl();}__except(Exception(GetExceptionInformation())){return false;}}
inline void Pointer(FILE* file,const char* name,void* value){std::fprintf(file,",\"%s\":\"%p\"",name,value);}
inline void List(FILE* file,const char* name,const RegistrationList& list){
    std::fprintf(file,",\"%s\":{\"valid\":%s,\"count\":%d,\"capacity\":%d,\"controllers\":[",name,list.valid?"true":"false",list.count,list.capacity);
    for(int32_t i=0;i<list.count&&i<static_cast<int32_t>(MaxControllers);++i)std::fprintf(file,"%s\"%p\"",i?",":"",reinterpret_cast<void*>(list.controllers[i]));std::fputs("]}",file);
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;const auto b=[](bool x){return x?"true":"false";};
    std::fprintf(file,"{\"schema_version\":2,\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"sole_local_player_valid\":%s,\"only_local_camera_owner\":%s,\"native_calls\":%llu,\"samples\":%llu,\"exception_rva\":%llu,\"original_remove_controller_rva\":80549056,\"world_camera_loop_rva\":73730944,\"world_projection_loop_rva\":73728464",state.stage,state.error,b(state.initialized),b(state.soleLocalPlayerValid),b(state.onlyLocalCameraOwner),static_cast<unsigned long long>(state.nativeCalls),static_cast<unsigned long long>(state.samples),static_cast<unsigned long long>(state.exceptionRva));
    std::fprintf(file,",\"death_waiting_targets\":%u,\"expected_game_state_player_count\":%d,\"death_wait_health_must_be_nonpositive\":true",state.deathWaitingTargets,state.expectedRoster.count);
    Pointer(file,"world",state.world);Pointer(file,"active_controller",state.activeController);Pointer(file,"active_pawn",state.activePawn);Pointer(file,"local_player",state.localPlayer);
    List(file,"current_all_controllers",state.currentAll);List(file,"current_player_controllers",state.currentPlayers);std::fputs(",\"npcs\":[",file);bool first=true;
    for(size_t i=0;i<MaxPlayers;++i){const auto& entry=state.entries[i];if(!entry.attempted)continue;
      std::fprintf(file,"%s{\"roster_index\":%zu,\"stage\":\"%s\",\"error\":%u,\"applied\":%s,\"already_detached\":%s,\"native_entry_verified\":%s,\"no_player\":%s,\"not_in_local_players\":%s,\"possession_preserved\":%s,\"actor_state_preserved\":%s,\"game_state_roster_preserved\":%s,\"all_order_preserved\":%s,\"player_order_preserved\":%s,\"local_links_preserved\":%s,\"only_local_camera_owner\":%s,\"current_links_valid\":%s,\"calls\":%llu,\"samples\":%llu,\"game_state_players_before\":%d,\"game_state_players_after\":%d",first?"":",",i,entry.stage,entry.error,b(entry.applied),b(entry.alreadyDetached),b(entry.nativeEntryVerified),b(entry.noPlayer),b(entry.notInLocalPlayers),b(entry.possessionPreserved),b(entry.actorStatePreserved),b(entry.gameStateRosterPreserved),b(entry.allOrderPreserved),b(entry.playerOrderPreserved),b(entry.localLinksPreserved),b(entry.onlyLocalCameraOwner),b(entry.currentLinksValid),static_cast<unsigned long long>(entry.calls),static_cast<unsigned long long>(entry.samples),entry.rosterBefore.count,entry.rosterAfter.count);first=false;
      std::fprintf(file,",\"full_possession_links\":%s,\"dead_unpossessed_wait\":%s,\"death_roster_preserved\":%s,\"death_readback_queried\":%s,\"death_alive\":%s,\"death_handler_alive\":%s,\"death_readback_consistent\":%s,\"dead_wait_samples\":%llu,\"death_health\":",b(entry.fullPossessionLinks),b(entry.deadUnpossessedWait),b(entry.deathRosterPreserved),b(entry.deathLife.queried),b(entry.deathLife.alive),b(entry.deathLife.handlerAlive),b(entry.deathLife.consistent),static_cast<unsigned long long>(entry.deadWaitSamples));
      if(entry.deathLife.queried&&std::isfinite(entry.deathLife.health))std::fprintf(file,"%.9g",entry.deathLife.health);else std::fputs("null",file);
      Pointer(file,"controller",entry.controller);Pointer(file,"pawn_at_detach",entry.pawn);Pointer(file,"current_pawn",entry.currentPawn);Pointer(file,"player_state",entry.playerState);Pointer(file,"camera",entry.camera);
      Pointer(file,"current_controller_pawn",entry.currentControllerPawn);Pointer(file,"current_pawn_controller",entry.currentPawnController);Pointer(file,"current_pawn_player_state",entry.currentPawnState);Pointer(file,"death_damage_handler",entry.deathLife.handler);
      List(file,"all_before",entry.allBefore);List(file,"all_after",entry.allAfter);List(file,"players_before",entry.playersBefore);List(file,"players_after",entry.playersAfter);std::fputc('}',file);
    }
    std::fputs("],\"actors_destroyed\":false,\"tick_flags_changed\":false,\"roles_changed\":false,\"projection_matrices_written\":false,\"transforms_written\":false}\n",file);const bool result=std::ferror(file)==0;std::fclose(file);return result;
}
}
