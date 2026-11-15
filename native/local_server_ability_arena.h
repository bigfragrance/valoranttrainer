#pragma once
#include "local_server_hero_abilities.h"
#include "local_server_server_predicate.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

// Iso X authority preflight and the retained *real Dedicated* initialization
// path. A real IP Listen host does not satisfy the original nested Dedicated
// predicates. The original Actor NetMode2 is distinguished from this client's
// World.GetNetMode fallback3; the latter does not deny actual IP host ownership.
// This adapter reads the original predicates and both original mode routes.
// It does not change NetMode, the predicates, Roles, original graph branches,
// lifecycle state, health or a playspace. A manager object is not skill success.
// Evidence: work/local-server-playable/hero-abilities/group2-arena-*.
namespace local_server_ability_arena {
using Weak=original_inventory_slots::WeakIdentity;
using Source=local_server_hero_abilities::SourceView;
inline constexpr unsigned IsoIndex=13;
inline constexpr size_t MaximumArenas=8;
inline constexpr const wchar_t* ManagerPath=L"/Game/Characters/Sequoia/S0/Ability_X/Comp_GameState_Sequoia_ArenaManager.Comp_GameState_Sequoia_ArenaManager_C";
inline constexpr const wchar_t* ArenaPath=L"/Game/Characters/Sequoia/S0/Ability_X/Actor_Sequoia_X_StandardArena.Actor_Sequoia_X_StandardArena_C";
inline constexpr const wchar_t* ArenaBasePath=L"/Game/Characters/Sequoia/S0/Ability_X/Actor_Sequoia_X_ArenaBase.Actor_Sequoia_X_ArenaBase_C";
inline constexpr const wchar_t* CreatePath=L"/Game/Characters/Sequoia/Sequoia_PC.Sequoia_PC_C.AuthTryCreateArenaManager";
inline constexpr const wchar_t* AvailablePath=L"/Game/Characters/Sequoia/S0/Ability_X/Comp_GameState_Sequoia_ArenaManager.Comp_GameState_Sequoia_ArenaManager_C.AuthTryGetAvailableArena";

enum Pending:uint32_t {
    OriginalDedicatedPredicate=1U<<0,OriginalManagerState=1U<<1,
    LifeResetAuthority=1U<<2,TimeoutKillAuthority=1U<<3,
    PlayspaceAuthority=1U<<4,TeleportLifecycle=1U<<5,
};
// These are observed missing server operations in the original image. A true
// manager/available arena does not establish that those operations are present.
inline constexpr uint32_t KnownExecutionGaps=LifeResetAuthority|TimeoutKillAuthority|PlayspaceAuthority|TeleportLifecycle;
struct ArenaObservation {
    void* actor=nullptr;Weak identity{};
    bool live=false,originalClass=false,ownerMatches=false,worldMatches=false,begun=false;
    uint8_t role=255;
};
struct State {
    bool active=false,initialized=false,profilesVerified=false,metadataVerified=false,restartRequired=false;
    bool ownedServerPredicate=false;
    bool sourceVerified=false,originalServer=false,originalDedicated=false,originalCreateSubmitted=false;
    bool managerObserved=false,managerOwnerMatches=false,managerWorldMatches=false,managerArraySchema=false;
    bool nativeInitializationReady=false,nativeAvailable=false;
    // No execution-success claim follows from assembly or an available actor.
    bool runtimeReady=false,executionVerified=false,playableVerified=false;
    unsigned error=0;DWORD gameThread=0;uint32_t epoch=0,pending=KnownExecutionGaps|OriginalDedicatedPredicate|OriginalManagerState;
    int32_t originalActorNetMode=-1,originalWorldNetMode=-1,expectedArenas=0,arenaCount=0;
    uint64_t polls=0,predicateQueries=0,originalCreateCalls=0,managerQueries=0,availableQueries=0;
    uint64_t sourceRegistrations=0,sourceChanges=0,lastSample=0;
    void *world=nullptr,*pawn=nullptr,*gameState=nullptr,*manager=nullptr,*available=nullptr;
    uintptr_t exceptionRva=0;
    const char* stage="iso_arena_not_requested";
    std::array<ArenaObservation,MaximumArenas> arenas{};
};
struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
inline constexpr Profile Profiles[]{
    {0x45d4cf0,228,0x0841b0f9c4fcb436ULL},{0x45d4de0,228,0xc78adb8cc6553177ULL},
    {0x3ee8590,260,0x939c2a4b8e38b2ceULL},{0x3eea1f0,39,0x330c967f2063690aULL},
    {0x4129ce0,32,0x1bf314dee626758dULL},{0x4ce37a0,159,0xd78e689a34f4f854ULL},
    {0x2a64ee0,132,0x83dfe973e78a6846ULL},{0x3ed83b0,60,0xb0370ccdf6164999ULL},
    {0x3efc820,176,0x7f1113b3f79164b2ULL},
};
inline State state{},cached{};
inline Source source{};
inline Weak worldId{},stateId{},managerId{},managerTypeId{},arenaTypeId{},arenaBaseTypeId{};
inline void *system=nullptr,*isServer=nullptr,*isDedicated=nullptr,*getComponent=nullptr,*getOwner=nullptr,*hasBegun=nullptr;
inline void *managerType=nullptr,*arenaType=nullptr,*arenaBaseType=nullptr,*create=nullptr,*availableFunction=nullptr;
inline int32_t arenasOffset=-1,countOffset=-1;
inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline const State& GetState(){return state;}
inline void Cache(){AcquireSRWLockExclusive(&cacheLock);cached=state;ReleaseSRWLockExclusive(&cacheLock);}
inline State Snapshot(){AcquireSRWLockShared(&cacheLock);auto value=cached;ReleaseSRWLockShared(&cacheLock);return value;}
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Capture(void* object,Weak& identity){return original_inventory_slots::CaptureIdentity(object,identity)&&Live(object,identity);}
inline bool Thread(){return engine::image&&!state.restartRequired&&original_players::Thread()
    &&(!state.gameThread||state.gameThread==GetCurrentThreadId());}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.runtimeReady=false;state.stage=stage;return false;}
inline bool Wait(const char* stage){state.runtimeReady=false;state.stage=stage;return true;}
inline bool Inactive(const char* stage){
    state.active=false;state.sourceVerified=false;state.nativeInitializationReady=false;state.nativeAvailable=false;
    state.available=nullptr;return Wait(stage);
}
inline bool Dispatch(void* object,void* function,void* parameters){
    auto process=Read<void*>(Read<void*>(object),0x298);
    if(!object||!function||!process)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,function,parameters);return true;
}
inline bool Native(void*& function,const wchar_t* path,uint16_t bytes,uintptr_t exec,uintptr_t owner,bool allowOwnedServer=false){
    function=engine::Find(path);
    if(!function||Read<uint16_t>(function,0xbe)!=bytes||(Read<uint32_t>(function,0xb8)&0x400U)==0
       ||Read<void*>(function,0x28)!=engine::Class(owner))return false;
    auto actualExec=Read<void*>(function,0xe0);
    const bool original=actualExec==engine::function<void*>(exec);
    // Only this exact IsServer function can compose with the project's proven
    // same-world native parser/real-listener bridge. Dedicated and every other
    // native API continue to require their unchanged original exec address.
    const bool serverEntry=exec==local_server_server_predicate::ExecRva
        &&owner==local_server_server_predicate::LibraryClass;
    const bool owned=!original&&allowOwnedServer&&serverEntry
        &&local_server_server_predicate::OwnsIsServerExec(state.world,function,actualExec);
    if(serverEntry)state.ownedServerPredicate=owned;
    return original||owned;
}
inline bool Blueprint(void*& function,const wchar_t* path,void* owner,uint16_t bytes,int32_t scriptBytes){
    function=engine::Find(path);auto script=Read<void*>(function,0x68);
    return function&&Read<void*>(function,0x28)==owner&&Read<uint16_t>(function,0xbe)==bytes
        &&(Read<uint32_t>(function,0xb8)&0x400U)==0&&Read<int32_t>(function,0x70)==scriptBytes
        &&Read<int32_t>(function,0x74)>=scriptBytes&&original_players::Readable(script,static_cast<size_t>(scriptBytes));
}
inline bool Verify(){
    if(state.profilesVerified&&state.metadataVerified)return true;
    for(const auto& profile:Profiles){
        auto code=reinterpret_cast<const uint8_t*>(engine::image+profile.rva);
        if(!original_players::Readable(code,profile.bytes))return Fail(6750,"iso_arena_original_api_profile_unreadable");
        uint64_t digest=0xcbf29ce484222325ULL;
        for(size_t i=0;i<profile.bytes;++i)digest=(digest^code[i])*0x100000001b3ULL;
        if(digest!=profile.fnv)return Fail(6751,"iso_arena_original_api_profile_mismatch");
    }
    state.profilesVerified=true;
    // Actual UFunction.ParmsSize is9 (pointer@0 + bool@8). The caller's native
    // parameter container remains16 bytes because of its trailing alignment.
    if(!Native(isServer,L"/Script/Engine.KismetSystemLibrary.IsServer",9,0x45d4de0,0x45aa110,true)
       ||!Native(isDedicated,L"/Script/Engine.KismetSystemLibrary.IsDedicatedServer",9,0x45d4cf0,0x45aa110)
       ||!Native(getComponent,L"/Script/Engine.Actor.GetComponentByClass",16,0x3ee8590,0x3683900)
       ||!Native(getOwner,L"/Script/Engine.ActorComponent.GetOwner",8,0x4129ce0,0x36837a0)
       ||!Native(hasBegun,L"/Script/Engine.Actor.HasActorBegunPlay",1,0x3eea1f0,0x3683900))
        return Fail(6752,"iso_arena_original_api_metadata_mismatch");
    system=engine::DefaultObject(engine::Class(0x45aa110));
    if(!system)return Fail(6753,"iso_arena_original_kismet_default_missing");
    state.metadataVerified=true;return true;
}
inline bool Predicates(){
    // Revalidate the current entry at each native query, including a normal
    // project-owned bridge install/restore after the first metadata sample.
    // A cached metadata flag never admits a foreign replacement exec.
    if(!Native(isServer,L"/Script/Engine.KismetSystemLibrary.IsServer",9,0x45d4de0,0x45aa110,true)
       ||!Native(isDedicated,L"/Script/Engine.KismetSystemLibrary.IsDedicatedServer",9,0x45d4cf0,0x45aa110))
        return Fail(6752,"iso_arena_original_predicate_metadata_or_owned_server_entry_mismatch");
    struct Query {void* world=nullptr;uint8_t result=0,padding[7]{};} server{source.pawn},dedicated{source.pawn};
    static_assert(sizeof(Query)==16&&offsetof(Query,result)==8);
    if(!Dispatch(system,isServer,&server)||!Dispatch(system,isDedicated,&dedicated))
        return Fail(6754,"iso_arena_original_world_predicate_query_failed");
    state.originalServer=server.result!=0;state.originalDedicated=dedicated.result!=0;
    state.originalWorldNetMode=engine::function<int32_t(*)(void*)>(0x4ce37a0)(source.world);
    state.originalActorNetMode=engine::function<int32_t(*)(void*)>(0x3efc820)(source.pawn);
    ++state.predicateQueries;
    if(state.originalWorldNetMode<0||state.originalWorldNetMode>3||state.originalActorNetMode<0||state.originalActorNetMode>3)
        return Fail(6755,"iso_arena_original_netmode_outside_original_enum");
    // These two retained routes need not agree in the client-only image.
    // Record the actual results; never replace them with a consistency flag.
    return true;
}
inline bool Types(){
    state.managerArraySchema=false;
    auto actualManager=engine::Find(ManagerPath),actualArena=engine::Find(ArenaPath),actualBase=engine::Find(ArenaBasePath);
    if(!actualManager||!actualArena||!actualBase)return Wait("iso_arena_waiting_original_loaded_manager_and_arena_classes");
    if(!original_players::Derived(actualManager,engine::Class(0x36837a0))
       ||!original_players::Derived(actualBase,engine::Class(0x3683900))||!original_players::Derived(actualArena,actualBase))
        return Fail(6757,"iso_arena_original_manager_or_arena_class_invalid");
    if(managerType&&(managerType!=actualManager||!Live(managerType,managerTypeId)))
        return Fail(6758,"iso_arena_original_manager_class_changed_within_world");
    if(arenaType&&(arenaType!=actualArena||!Live(arenaType,arenaTypeId)))
        return Fail(6759,"iso_arena_original_arena_class_changed_within_world");
    if(arenaBaseType&&(arenaBaseType!=actualBase||!Live(arenaBaseType,arenaBaseTypeId)))
        return Fail(6789,"iso_arena_original_arena_base_class_changed_within_world");
    managerType=actualManager;arenaType=actualArena;arenaBaseType=actualBase;
    if(!Capture(managerType,managerTypeId)||!Capture(arenaType,arenaTypeId)||!Capture(arenaBaseType,arenaBaseTypeId))
        return Fail(6760,"iso_arena_original_class_identity_capture_failed");
    if(!Blueprint(create,CreatePath,engine::Find(original_heroes::Paths[IsoIndex]),0,217)
       ||!Blueprint(availableFunction,AvailablePath,managerType,8,484))
        return Fail(6761,"iso_arena_original_manager_blueprint_metadata_mismatch");
    original_jett_priming::PropertyInfo array{},count{};
    arenasOffset=original_jett_priming::FieldOffset(managerType,L"Arenas",16,&array);
    countOffset=original_jett_priming::FieldOffset(managerType,L"NumArenasToSpawn",4,&count);
    auto inner=Read<void*>(array.field,0x80),innerType=Read<void*>(inner,0x78);
    if(arenasOffset<0||countOffset<0||array.arrayDim!=1||array.elementSize!=16||count.arrayDim!=1||count.elementSize!=4
       ||!inner||Read<int32_t>(inner,0x30)!=1||Read<int32_t>(inner,0x34)!=8||innerType!=arenaBaseType)
        return Fail(6762,"iso_arena_original_manager_arrays_or_count_schema_mismatch");
    auto defaults=Read<void*>(managerType,0x120);
    state.expectedArenas=Read<int32_t>(defaults,static_cast<size_t>(countOffset));
    if(!defaults||state.expectedArenas<=0||state.expectedArenas>static_cast<int32_t>(MaximumArenas))
        return Fail(6763,"iso_arena_original_authored_arena_count_invalid");
    state.managerArraySchema=true;return true;
}
inline bool QueryManager(){
    struct Query {void* type=nullptr;void* result=nullptr;} query{managerType};
    if(!Dispatch(state.gameState,getComponent,&query))return Fail(6764,"iso_arena_original_manager_query_failed");
    ++state.managerQueries;
    if(!query.result){state.managerObserved=false;return true;}
    if(state.manager&&state.manager!=query.result)return Fail(6765,"iso_arena_original_manager_replaced_within_world");
    if(!original_players::IsA(query.result,0x36837a0)||!original_players::Derived(Read<void*>(query.result,0x10),managerType))
        return Fail(6766,"iso_arena_original_manager_query_returned_foreign_type");
    if(!state.manager){state.manager=query.result;if(!Capture(state.manager,managerId))return Fail(6767,"iso_arena_original_manager_identity_capture_failed");}
    if(!Live(state.manager,managerId))return Fail(6768,"iso_arena_original_manager_identity_retired");
    void* owner=nullptr;
    if(!Dispatch(state.manager,getOwner,&owner))return Fail(6769,"iso_arena_original_manager_owner_query_failed");
    state.managerOwnerMatches=owner==state.gameState&&Read<void*>(state.manager,0xc8)==owner;
    auto getWorld=Read<void*>(Read<void*>(state.manager),0x1b8);
    state.managerWorldMatches=getWorld==engine::function<void*>(0x2a64ee0)
        &&reinterpret_cast<void*(*)(void*)>(getWorld)(state.manager)==source.world;
    if(!state.managerOwnerMatches||!state.managerWorldMatches)return Fail(6770,"iso_arena_original_manager_owner_or_world_mismatch");
    state.managerObserved=true;return true;
}
inline bool ObserveArenas(){
    state.nativeInitializationReady=state.nativeAvailable=false;state.available=nullptr;state.arenas={};
    const auto array=Read<local_server_starting_equipment::Array<void*>>(state.manager,static_cast<size_t>(arenasOffset));
    if(!local_server_starting_equipment::ValidArray(array,static_cast<int32_t>(MaximumArenas)))
        return Fail(6771,"iso_arena_original_manager_arena_array_invalid");
    state.arenaCount=array.count;
    if(array.count!=state.expectedArenas)return Wait("iso_arena_waiting_original_manager_authored_arenas");
    for(int32_t i=0;i<array.count;++i){auto& observation=state.arenas[static_cast<size_t>(i)];
        observation.actor=Read<void*>(array.data,static_cast<size_t>(i)*8);
        observation.live=Capture(observation.actor,observation.identity);observation.role=Read<uint8_t>(observation.actor,0x224);
        observation.originalClass=original_players::Derived(Read<void*>(observation.actor,0x10),arenaType);
        observation.ownerMatches=Read<void*>(observation.actor,0x210)==state.gameState;
        observation.worldMatches=local_server_item_ownership::ActorWorld(observation.actor)==source.world;
        uint8_t begun=0;
        if(!observation.live||!observation.originalClass||!observation.ownerMatches||!observation.worldMatches||observation.role!=3)
            return Fail(6772,"iso_arena_original_manager_contains_invalid_authority_arena");
        for(int32_t other=0;other<i;++other)if(state.arenas[static_cast<size_t>(other)].actor==observation.actor)
            return Fail(6773,"iso_arena_original_manager_contains_duplicate_arena");
        if(!Dispatch(observation.actor,hasBegun,&begun))return Fail(6774,"iso_arena_original_arena_beginplay_query_failed");
        observation.begun=begun!=0;if(!observation.begun)return Wait("iso_arena_waiting_original_arena_beginplay");
    }
    void* available=nullptr;
    if(!Dispatch(state.manager,availableFunction,&available))return Fail(6775,"iso_arena_original_available_state_query_failed");
    ++state.availableQueries;
    if(available){
        bool found=false;for(int32_t i=0;i<array.count;++i)if(state.arenas[static_cast<size_t>(i)].actor==available)found=true;
        if(!found)return Fail(6776,"iso_arena_original_available_query_returned_foreign_arena");
        state.available=available;state.nativeAvailable=true;
    }
    state.nativeInitializationReady=true;state.pending&=~OriginalManagerState;
    return true;
}
inline bool PollImpl(void* pawn,bool skillsEnabled,bool deathmatch){
    // This policy gate precedes even native metadata/class queries. Disabled
    // modes and ordinary non-Iso characters never initialize an arena system.
    if(!skillsEnabled||deathmatch)return Inactive("iso_arena_disabled_for_session_policy");
    if(!Thread())return Fail(6777,"iso_arena_wrong_original_game_thread");
    if(state.error)return false;
    Source current{};
    if(!local_server_hero_abilities::ResolveSource(pawn,current))return Inactive("iso_arena_waiting_registered_authority_human_source");
    if(current.heroIndex!=IsoIndex)return Inactive("iso_arena_not_requested_for_other_original_hero");
    if(current.contextObject!=current.pawn||!current.sourceEpoch||!current.registration||!local_server_hero_abilities::ValidateSource(current))
        return Fail(6778,"iso_arena_registered_source_epoch_changed");
    state.active=true;
    if(state.world&&state.world!=current.world)return Fail(6779,"iso_arena_close_before_new_native_world");
    if(!state.initialized){
        state.world=current.world;state.gameThread=GetCurrentThreadId();
        if(!Capture(state.world,worldId))return Fail(6780,"iso_arena_original_world_identity_capture_failed");
        state.initialized=true;
    }
    if(!Live(state.world,worldId))return Fail(6781,"iso_arena_original_world_retired");
    if(source.registration!=current.registration||source.sourceEpoch!=current.sourceEpoch||source.pawn!=current.pawn){
        if(source.registration)++state.sourceChanges;
        source=current;state.epoch=current.sourceEpoch;state.pawn=current.pawn;++state.sourceRegistrations;
    }
    state.sourceVerified=true;++state.polls;
    const auto now=GetTickCount64();if(state.lastSample&&now-state.lastSample<100)return true;
    state.lastSample=now;
    if(!Verify()||!Predicates())return false;
    if(!state.originalDedicated){
        state.pending|=OriginalDedicatedPredicate|OriginalManagerState;
        state.nativeInitializationReady=state.nativeAvailable=false;
        return Wait(state.originalActorNetMode==2?"iso_arena_original_dedicated_predicate_false_for_real_listen_authority"
            :"iso_arena_original_dedicated_authority_required");
    }
    state.pending&=~OriginalDedicatedPredicate;
    auto gameState=Read<void*>(state.world,0x178);
    if(!original_players::IsA(gameState,original_players::ShooterGameStateClass)||Read<uint8_t>(gameState,0x224)!=3
       ||local_server_item_ownership::ActorWorld(gameState)!=state.world)
        return Wait("iso_arena_waiting_original_authority_game_state");
    if(state.gameState&&state.gameState!=gameState)return Fail(6782,"iso_arena_original_game_state_changed_within_world");
    if(!state.gameState){state.gameState=gameState;if(!Capture(gameState,stateId))return Fail(6783,"iso_arena_original_game_state_identity_capture_failed");}
    if(!Live(gameState,stateId))return Fail(6784,"iso_arena_original_game_state_retired");
    if(!Types())return false;
    if(!state.managerArraySchema)return true;
    if(!QueryManager())return false;
    if(!state.managerObserved&&!state.originalCreateSubmitted){
        if(!local_server_hero_abilities::ValidateSource(source)||!Predicates()||!state.originalDedicated)
            return Wait("iso_arena_original_dedicated_source_changed_before_creation");
        state.originalCreateSubmitted=true;++state.originalCreateCalls;
        // Original PC function checks the existing component, obtains its real
        // GameState, and invokes the retained original AddComponentByClass.
        // No equivalent object/branch is constructed by this adapter.
        if(!Dispatch(source.pawn,create,nullptr))return Fail(6785,"iso_arena_original_pc_manager_initialization_dispatch_failed");
        if(!local_server_hero_abilities::ValidateSource(source))return Fail(6788,"iso_arena_source_retired_during_original_manager_initialization");
        if(!QueryManager())return false;
    }
    if(!state.managerObserved)return Wait("iso_arena_original_pc_initialization_has_no_manager_receipt");
    if(!ObserveArenas())return false;
    if(!state.nativeInitializationReady)return true;
    // The original Arena's AuthSetLife/AuthKill/playspace/return lifecycle has
    // independent stripped or demo-only operations. Keep their explicit gaps.
    state.pending|=KnownExecutionGaps;state.runtimeReady=false;
    state.stage=state.nativeAvailable?"iso_arena_original_available_state_observed_execution_authority_pending"
        :"iso_arena_original_manager_initialized_currently_no_available_state";
    return true;
}
inline LONG Exception(EXCEPTION_POINTERS* details){
    const auto address=details&&details->ExceptionRecord?reinterpret_cast<uintptr_t>(details->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=address>=engine::image?address-engine::image:address;state.restartRequired=true;
    Fail(6786,"iso_arena_original_lifecycle_exception_requires_new_session");return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Poll(void* pawn,bool skillsEnabled,bool deathmatch){
    __try{const bool result=PollImpl(pawn,skillsEnabled,deathmatch);Cache();return result;}
    __except(Exception(GetExceptionInformation())){Cache();return false;}
}
inline bool Close(){
    if(state.initialized&&!Thread())return Fail(6787,"iso_arena_close_requires_original_game_thread");
    // Any original-created manager/arena is owned by the real GameState/world.
    // No private allocation, observer, handler or native object was substituted;
    // native world teardown retains ownership of those real instances.
    state.active=false;state.sourceVerified=false;state.nativeInitializationReady=false;
    state.nativeAvailable=false;state.runtimeReady=false;state.stage="iso_arena_source_observation_closed_native_world_owns_lifecycle";
    state.initialized=false;state.gameThread=0;state.world=state.pawn=state.gameState=state.manager=state.available=nullptr;
    state.managerObserved=state.managerOwnerMatches=state.managerWorldMatches=state.managerArraySchema=false;
    state.originalCreateSubmitted=false;state.originalServer=state.originalDedicated=false;
    state.ownedServerPredicate=false;
    state.originalActorNetMode=state.originalWorldNetMode=-1;state.epoch=0;state.expectedArenas=state.arenaCount=0;state.lastSample=0;state.arenas={};
    state.pending=KnownExecutionGaps|OriginalDedicatedPredicate|OriginalManagerState;
    source={};worldId=stateId=managerId=managerTypeId=arenaTypeId=arenaBaseTypeId={};
    managerType=arenaType=arenaBaseType=create=availableFunction=nullptr;
    arenasOffset=countOffset=-1;Cache();return true;
}
inline void Write(FILE* file){
    if(!file)return;const auto s=Snapshot();const auto yes=[](bool value){return value?"true":"false";};
    std::fprintf(file,"{\"active\":%s,\"initialized\":%s,\"stage\":\"%s\",\"error\":%u,\"restart_required\":%s,\"profiles_verified\":%s,\"metadata_verified\":%s,\"owned_server_predicate\":%s,\"source_verified\":%s,\"original_server\":%s,\"original_dedicated\":%s,\"original_actor_net_mode\":%d,\"original_world_net_mode\":%d,\"original_create_submitted\":%s,\"manager_observed\":%s,\"manager_owner_matches\":%s,\"manager_world_matches\":%s,\"manager_array_schema\":%s,\"native_initialization_ready\":%s,\"native_available\":%s,\"runtime_ready\":%s,\"execution_verified\":%s,\"playable_verified\":%s,\"pending_mask\":%u,\"expected_arenas\":%d,\"arena_count\":%d,\"epoch\":%u,\"polls\":%llu,\"predicate_queries\":%llu,\"original_create_calls\":%llu,\"manager_queries\":%llu,\"available_queries\":%llu,\"source_registrations\":%llu,\"source_changes\":%llu,\"world\":\"%p\",\"pawn\":\"%p\",\"game_state\":\"%p\",\"manager\":\"%p\",\"available\":\"%p\",\"exception_rva\":\"0x%llx\",\"arenas\":[",
        yes(s.active),yes(s.initialized),s.stage,s.error,yes(s.restartRequired),yes(s.profilesVerified),yes(s.metadataVerified),yes(s.ownedServerPredicate),yes(s.sourceVerified),
        yes(s.originalServer),yes(s.originalDedicated),s.originalActorNetMode,s.originalWorldNetMode,yes(s.originalCreateSubmitted),yes(s.managerObserved),yes(s.managerOwnerMatches),yes(s.managerWorldMatches),yes(s.managerArraySchema),
        yes(s.nativeInitializationReady),yes(s.nativeAvailable),yes(s.runtimeReady),yes(s.executionVerified),yes(s.playableVerified),s.pending,s.expectedArenas,s.arenaCount,s.epoch,
        static_cast<unsigned long long>(s.polls),static_cast<unsigned long long>(s.predicateQueries),static_cast<unsigned long long>(s.originalCreateCalls),static_cast<unsigned long long>(s.managerQueries),static_cast<unsigned long long>(s.availableQueries),static_cast<unsigned long long>(s.sourceRegistrations),static_cast<unsigned long long>(s.sourceChanges),s.world,s.pawn,s.gameState,s.manager,s.available,static_cast<unsigned long long>(s.exceptionRva));
    bool first=true;for(size_t i=0;i<s.arenas.size();++i){const auto& a=s.arenas[i];if(!a.actor)continue;
        if(!first)std::fputc(',',file);first=false;
        std::fprintf(file,"{\"actor\":\"%p\",\"live\":%s,\"original_class\":%s,\"owner_matches\":%s,\"world_matches\":%s,\"begun\":%s,\"role\":%u}",a.actor,yes(a.live),yes(a.originalClass),yes(a.ownerMatches),yes(a.worldMatches),yes(a.begun),unsigned(a.role));}
    std::fputs("],\"original_image_writes\":0,\"predicate_or_netmode_writes\":0,\"adapter_health_or_state_writes\":0}",file);
}
} // namespace local_server_ability_arena
