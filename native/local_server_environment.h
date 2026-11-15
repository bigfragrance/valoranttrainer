#pragma once
#include "original_players.h"
#include "original_inventory_slots.h"
#include "original_scope.h"
#include "training_maps.h"
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>

// Late authoritative GameState catchup and original streaming publication.
// A separate client entry point is a passive readback observer only.
// The original level actors may
// already have begun play against the bootstrap Engine.GameState. Call this
// on HOST after original map readiness and native gameplay-active maturation.
// Client level actors retain their normal BeginPlay/GS notification paths.
// No collision/visibility/phase/role/locality/timer field is written here.
namespace local_server_environment {
using Weak=original_inventory_slots::WeakIdentity;
inline constexpr size_t MaxActors=512;
inline constexpr size_t MaxConnections=8,MaxLayers=64;
enum class Kind:uint8_t {Barrier,ProjectileBarrier,DeprecatedBarrier,Door};
struct Array {void** data=nullptr;int32_t count=0,capacity=0;};
struct Name {uint32_t words[3]{};};
struct Delegate {Weak actor;Name function;};
static_assert(sizeof(Name)==12&&sizeof(Delegate)==20&&offsetof(Delegate,function)==8);
struct Layer {
    const training_maps::LayerSpec* spec=nullptr;Name package{};Weak identity{},levelIdentity{};
    void *object=nullptr,*level=nullptr;bool loaded=false,visible=false,shouldLoad=false,shouldVisible=false;
    bool blockLoad=false,blockUnload=false,satisfied=false;int32_t lod=-1;uint64_t revision=0;
};
struct Connection {
    bool used=false,ready=false;void *connection=nullptr,*controller=nullptr;Weak identity{},controllerIdentity{};
    std::array<uint64_t,MaxLayers> sentRevision{};std::array<bool,MaxLayers> visibleAcknowledged{};
    uint64_t originalRequests=0,visibilityQueries=0;size_t sent=0,acknowledged=0,required=0;
};
struct Entry {
    bool used=false,seen=false,registered=false,collisionMatches=false,boundToCurrentGS=false;
    bool originalSetupCalled=false,nativeMulticastRequested=false,replicatedActor=false;
    Kind kind=Kind::Barrier;Weak actorIdentity{},meshIdentity{},setupGSIdentity{};
    void *actor=nullptr,*mesh=nullptr,*setupGS=nullptr;
    int8_t lastActive=-1;uint8_t collision=255,role=0,remoteRole=0;
    uint16_t doorStatus=0;float doorSpeed=0,doorPosition=0,doorOpenStart=0,doorCloseStart=0;
    uint64_t setupCalls=0,activeCallbacks=0,readbacks=0,multicastConnectionRevision=0;
    const char* stage="environment_actor_not_observed";
};
struct State {
    bool initialized=false,profileReady=false,hostLayersReady=false,hostReady=false,clientPassive=false;
    bool clientLayersReady=false,clientReadbacksMatch=false,streamingProfileReady=false,connectionsReady=false;
    bool nativeActive=false,restartRequired=false,clientCollisionVerified=false;
    unsigned error=0;uint64_t polls=0,queries=0,setupCalls=0,activeCallbacks=0,multicastRequests=0,discoveries=0;
    uint64_t layerReads=0,layerRevision=0,streamingRequests=0,nativeVisibilityQueries=0,connectionRevision=0,legacyChannelDeferrals=0;
    uint16_t streamingActualParms=0;size_t layerCount=0,connections=0,connectionsAcknowledged=0;
    uint32_t barriers=0,projectileBarriers=0,deprecatedBarriers=0,doors=0,unresolved=0,profileFailed=0;
    uintptr_t exceptionRva=0;void *world=nullptr,*gameState=nullptr;
    Weak worldIdentity{},gameStateIdentity{};DWORD gameThread=0;
    Weak barrierClassIdentity{},projectileClassIdentity{},deprecatedClassIdentity{},doorClassIdentity{};
    char mapId[64]{};const char* stage="authority_environment_not_initialized";
};
struct Methods {
    void *allActors=nullptr,*collision=nullptr;
    void *barrierInit=nullptr,*barrierActive=nullptr,*projectileSet=nullptr,*projectileActive=nullptr,*deprecatedDrop=nullptr;
    void *streamingLevel=nullptr,*worldPackage=nullptr,*levelLoaded=nullptr,*levelVisible=nullptr,*shouldLoad=nullptr,*loadedLevel=nullptr;
    void *clientStreaming=nullptr,*serverVisibility=nullptr,*clientVisibilityAck=nullptr;
};
inline State state;inline Methods methods;
inline std::array<Entry,MaxActors> entries{};
inline std::array<Layer,MaxLayers> layers{};
inline std::array<Connection,MaxConnections> connections{};
inline void *barrierClass=nullptr,*projectileClass=nullptr,*deprecatedClass=nullptr,*doorClass=nullptr;
inline uint64_t nextObserve=0;
inline bool discoveryPending=true;
inline const State& GetState(){return state;}
inline const std::array<Entry,MaxActors>& GetEntries(){return entries;}
inline const std::array<Layer,MaxLayers>& GetLayers(){return layers;}
inline const std::array<Connection,MaxConnections>& GetConnections(){return connections;}
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Fail(unsigned code,const char* stage){state.error=code;state.stage=stage;state.hostReady=false;return false;}
inline bool Thread(){return state.initialized&&!state.restartRequired&&state.gameThread==GetCurrentThreadId()&&original_players::Thread();}
inline bool Dispatch(void* object,void* function,void* params){
    if(!object||!function)return false;
    auto entry=Read<void*>(Read<void**>(object),0x298);
    if(!entry)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(entry)(object,function,params);return true;
}
inline bool Bind(void*& function,const wchar_t* name,uint16_t bytes,void* outer=nullptr,uint32_t flags=0,uint32_t mask=0){
    function=engine::Find(name);
    return function&&Read<uint16_t>(function,0xbe)==bytes&&(!outer||Read<void*>(function,0x28)==outer)
        &&(!mask||(Read<uint32_t>(function,0xb8)&mask)==flags);
}
inline void* Property(void* type,const wchar_t* name,int32_t bytes,int32_t offset){
    Name nativeName{};engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&nativeName,name,1);
    auto field=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,&nativeName,3);
    const int32_t total=Read<int32_t>(type,0x60);
    return field&&(Read<uint64_t>(Read<void*>(field,8),0x18)&0x8001)==0x8001
        &&Read<int32_t>(field,0x30)==1&&Read<int32_t>(field,0x34)==bytes&&Read<int32_t>(field,0x44)==offset
        &&offset>=0&&total>=bytes&&offset<=total-bytes?field:nullptr;
}
inline bool NativeMethod(void*& fn,const wchar_t* name,uintptr_t exec,uint32_t flags,uint32_t mask){
    fn=engine::Find(name);return fn&&Read<void*>(fn,0xe0)==reinterpret_cast<void*>(engine::image+exec)
        &&(Read<uint32_t>(fn,0xb8)&mask)==flags;
}
inline bool InitializeStreamingProfile(){
    if(!Bind(methods.streamingLevel,L"/Script/Engine.GameplayStatics.GetStreamingLevel",32)
       ||!Bind(methods.worldPackage,L"/Script/Engine.LevelStreaming.GetWorldAssetPackageFName",12)
       ||!Bind(methods.levelLoaded,L"/Script/Engine.LevelStreaming.IsLevelLoaded",1)
       ||!Bind(methods.levelVisible,L"/Script/Engine.LevelStreaming.IsLevelVisible",1)
       ||!Bind(methods.shouldLoad,L"/Script/Engine.LevelStreaming.ShouldBeLoaded",1)
       ||!Bind(methods.loadedLevel,L"/Script/Engine.LevelStreaming.GetLoadedLevel",8))
        return Fail(4625,"authority_environment_original_streaming_observation_methods_missing");
    const uintptr_t expected[]{0x43eb1d0,0x463fcc0,0x463fbf0,0x463fc20,0x463fc80,0x463fc60};
    void* observed[]{methods.streamingLevel,methods.worldPackage,methods.levelLoaded,methods.levelVisible,methods.shouldLoad,methods.loadedLevel};
    for(size_t n=0;n<std::size(expected);++n)if(Read<void*>(observed[n],0xe0)!=reinterpret_cast<void*>(engine::image+expected[n]))
        return Fail(4626,"authority_environment_original_streaming_observation_exec_mismatch");
    auto type=engine::Class(0x3e270d0);
    if(!Property(type,L"LevelLODIndex",4,0xf4)||!Property(type,L"LoadedLevel",8,0x168))
        return Fail(4627,"authority_environment_original_streaming_level_layout_mismatch");
    // Original generated storage is28 bytes; the final field ends at25.
    // Check the actual loaded UFunction and all fields before sending. No
    // sizeof-based ParmsSize assumption, synthetic ACK, or transaction token.
    if(!NativeMethod(methods.clientStreaming,L"/Script/Engine.PlayerController.ClientUpdateLevelStreamingStatus",0x491ac40,0x01020cc2,0x01ffffff)
       ||!NativeMethod(methods.serverVisibility,L"/Script/Engine.PlayerController.ServerUpdateLevelVisibility",0x49195c0,0x80220cc3,0xffffffff)
       ||!NativeMethod(methods.clientVisibilityAck,L"/Script/Engine.PlayerController.ClientAckUpdateLevelVisibility",0x4921570,0x01020cc2,0x01ffffff))
        return Fail(4628,"authority_environment_original_streaming_rpc_profile_mismatch");
    state.streamingActualParms=Read<uint16_t>(methods.clientStreaming,0xbe);
    const struct Field {const wchar_t* name;int32_t bytes,offset;} fields[]{
        {L"PackageName",12,0},{L"bNewShouldBeLoaded",1,12},{L"bNewShouldBeVisible",1,13},
        {L"bNewShouldBlockOnLoad",1,14},{L"LODIndex",4,16},{L"TransactionId",4,20},{L"bNewShouldBlockOnUnload",1,24}};
    if(state.streamingActualParms<25||state.streamingActualParms>28)return Fail(4629,"authority_environment_original_streaming_rpc_parms_size_outside_recovered_span");
    for(const auto& f:fields){auto field=Property(methods.clientStreaming,f.name,f.bytes,f.offset);
        if(!field||(Read<uint64_t>(field,0x38)&0x80)==0)return Fail(4630,"authority_environment_original_streaming_rpc_field_mismatch");}
    auto visibility=Property(methods.serverVisibility,L"LevelVisibility",32,0);
    if(!visibility||Read<uint16_t>(methods.serverVisibility,0xbe)!=32
       ||!Property(methods.clientVisibilityAck,L"PackageName",12,0)||!Property(methods.clientVisibilityAck,L"TransactionId",4,12)
       ||!Property(methods.clientVisibilityAck,L"bClientAckCanMakeVisible",1,16)
       ||Read<uint16_t>(methods.clientVisibilityAck,0xbe)<17||Read<uint16_t>(methods.clientVisibilityAck,0xbe)>20)
        return Fail(4631,"authority_environment_original_visibility_ack_rpc_layout_mismatch");
    state.streamingProfileReady=true;return true;
}
inline void* LoadActorClass(const wchar_t* path){
    auto base=engine::Class(0x3683900);
    auto type=engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t)>(0x1d0c6e0)(base,nullptr,path,nullptr,0);
    return original_players::Derived(type,base)?type:nullptr;
}
inline bool InitializeProfile(){
    barrierClass=LoadActorClass(L"/Game/Blueprint/SpawnBarrier.SpawnBarrier_C");
    projectileClass=LoadActorClass(L"/Game/Blueprint/SpawnBarrierProjectile.SpawnBarrierProjectile_C");
    deprecatedClass=LoadActorClass(L"/Game/Environment/SpawnBarrierDeprecated.SpawnBarrierDeprecated_C");
    doorClass=engine::Class(0x6455860);
    if(!barrierClass||!projectileClass||!deprecatedClass||!doorClass)return Fail(4600,"authority_environment_original_actor_classes_missing");
    const bool meshLayout=original_scope::FieldOffset(engine::Find(L"/Script/ShooterGame.GameObject"),L"Mesh")==0x470;
    if(!meshLayout||original_scope::FieldOffset(Read<void*>(state.gameState,0x10),L"GameInfo")!=0x7d8
       ||original_scope::FieldOffset(Read<void*>(state.gameState,0x10),L"OnGameplayActiveChanged")!=0xa40
       ||original_scope::FieldOffset(engine::Class(0x3683900),L"bReplicates")!=0x11a
       ||original_scope::FieldOffset(doorClass,L"DoorStatus")!=0x514||original_scope::FieldOffset(doorClass,L"DoorSpeed")!=0x508
       ||original_scope::FieldOffset(doorClass,L"LocalDoorPos")!=0x50c||original_scope::FieldOffset(doorClass,L"DoorOpenStartTime")!=0x518
       ||original_scope::FieldOffset(doorClass,L"DoorCloseStartTime")!=0x51c)
        return Fail(4601,"authority_environment_original_game_info_or_delegate_layout_mismatch");
    if(!Bind(methods.allActors,L"/Script/Engine.GameplayStatics.GetAllActorsOfClass",32)
       ||!Bind(methods.collision,L"/Script/Engine.PrimitiveComponent.GetCollisionEnabled",1)
       ||!Bind(methods.barrierInit,L"/Game/Blueprint/SpawnBarrier.SpawnBarrier_C.InitializeSpawnBarrier",0,barrierClass,0,0x40)
       ||!Bind(methods.barrierActive,L"/Game/Blueprint/SpawnBarrier.SpawnBarrier_C.OnGameplayActiveChanged_Event_0",1,barrierClass,0,0x40)
       ||!Bind(methods.projectileSet,L"/Game/Blueprint/SpawnBarrierProjectile.SpawnBarrierProjectile_C.On GameStateSet",16,projectileClass,0,0x40)
       ||!Bind(methods.projectileActive,L"/Game/Blueprint/SpawnBarrierProjectile.SpawnBarrierProjectile_C.OnGameplayActiveChanged_Event_0",1,projectileClass,0,0x40)
       ||!Bind(methods.deprecatedDrop,L"/Game/Environment/SpawnBarrierDeprecated.SpawnBarrierDeprecated_C.DropSpawnBarrier",0,deprecatedClass,0x40c0,0x40c0))
        return Fail(4602,"authority_environment_original_blueprint_or_observation_function_profile_mismatch");
    if(Read<void*>(methods.allActors,0xe0)!=reinterpret_cast<void*>(engine::image+0x43eece0)
       ||Read<void*>(methods.collision,0xe0)!=reinterpret_cast<void*>(engine::image+0x3e807c0)
       ||!original_inventory_slots::CaptureIdentity(barrierClass,state.barrierClassIdentity)
       ||!original_inventory_slots::CaptureIdentity(projectileClass,state.projectileClassIdentity)
       ||!original_inventory_slots::CaptureIdentity(deprecatedClass,state.deprecatedClassIdentity)
       ||!original_inventory_slots::CaptureIdentity(doorClass,state.doorClassIdentity))
        return Fail(4624,"authority_environment_native_collision_getter_or_class_identity_profile_mismatch");
    state.profileReady=true;return true;
}
inline bool Same(const Weak& a,const Weak& b){return a.index==b.index&&a.serial==b.serial;}
inline bool ActiveBinding(const Entry& e,bool& bound){
    bound=false;
    struct Delegates {Delegate* data;int32_t count,capacity;};
    const auto list=Read<Delegates>(state.gameState,0xa40);
    if(list.count<0||list.count>4096||list.capacity<list.count||list.capacity>8192
       ||(list.count&&!original_players::Readable(list.data,static_cast<size_t>(list.count)*sizeof(Delegate))))
        return Fail(4603,"authority_environment_native_active_delegate_array_invalid");
    Name name{};engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&name,L"OnGameplayActiveChanged_Event_0",1);
    for(int32_t n=0;n<list.count;++n){const auto current=Read<Delegate>(list.data,static_cast<size_t>(n)*sizeof(Delegate));
        if(Same(current.actor,e.actorIdentity)&&std::memcmp(&current.function,&name,sizeof(name))==0){bound=true;return true;}}
    return true;
}
inline bool Observe(Entry& e){
    if(!Live(e.actor,e.actorIdentity))return Fail(4604,"authority_environment_actor_identity_expired_during_observation");
    e.role=Read<uint8_t>(e.actor,0x224);e.remoteRole=Read<uint8_t>(e.actor,0x128);
    if(e.kind==Kind::DeprecatedBarrier){
        const int offset=original_scope::FieldOffset(Read<void*>(e.actor,0x10),L"StaticMesh3");
        if(offset<0x30)return Fail(4605,"authority_environment_deprecated_original_mesh_property_missing");
        e.mesh=Read<void*>(e.actor,static_cast<size_t>(offset));
    }else e.mesh=Read<void*>(e.actor,0x470);
    if(e.kind==Kind::Door){
        // Door movement/collision can belong to Blueprint child components.
        // Observe the canonical replicated native record; do not infer a door
        // authority implementation from the optional base GameObject mesh.
        e.registered=e.mesh&&(Read<uint8_t>(e.mesh,0xa0)&1)!=0;
        e.replicatedActor=(Read<uint8_t>(e.actor,0x11a)&0x10)!=0;
        e.doorStatus=Read<uint16_t>(e.actor,0x514);e.doorSpeed=Read<float>(e.actor,0x508);
        e.doorPosition=Read<float>(e.actor,0x50c);e.doorOpenStart=Read<float>(e.actor,0x518);e.doorCloseStart=Read<float>(e.actor,0x51c);
        e.collisionMatches=true;e.stage="authority_environment_original_door_replicated_fields_observed_no_authority_override";
        ++e.readbacks;return true;
    }
    auto primitive=engine::Find(L"/Script/Engine.PrimitiveComponent");
    if(!e.mesh||!original_players::Derived(Read<void*>(e.mesh,0x10),primitive)
       ||Read<void*>(e.mesh,0xc8)!=e.actor||!original_inventory_slots::CaptureIdentity(e.mesh,e.meshIdentity))
        return Fail(4606,"authority_environment_original_mesh_owner_or_type_mismatch");
    e.collision=255;
    if(!Dispatch(e.mesh,methods.collision,&e.collision)||e.collision==255)
        return Fail(4607,"authority_environment_original_collision_or_actor_state_query_failed");
    // Same original ActorComponent registration bit used by the retained
    // native timed-streak lifecycle auditor; observation only. Actor bReplicates
    // byte11a/bit10 is independently recovered from its UHT bool setter3ef5850.
    e.registered=(Read<uint8_t>(e.mesh,0xa0)&1)!=0;
    e.replicatedActor=(Read<uint8_t>(e.actor,0x11a)&0x10)!=0;
    const uint8_t wanted=state.nativeActive?0:3;
    e.collisionMatches=e.registered&&e.collision==wanted;
    if(e.kind==Kind::Barrier||e.kind==Kind::ProjectileBarrier){if(!ActiveBinding(e,e.boundToCurrentGS))return false;}
    ++e.readbacks;return true;
}
inline bool Ownership(const Entry& e){
    return Live(state.world,state.worldIdentity)&&Live(state.gameState,state.gameStateIdentity)
        &&Read<void*>(state.world,0x178)==state.gameState&&Live(e.actor,e.actorIdentity)
        &&e.role==3&&(engine::function<int(*)(void*)>(0x3efc820)(e.actor)==1||engine::function<int(*)(void*)>(0x3efc820)(e.actor)==2);
}
inline bool RefreshNativeActive(){
    const uint8_t active=Read<uint8_t>(state.gameState,0x811);
    if(active>1)return Fail(4622,"authority_environment_original_game_info_active_value_invalid");
    state.nativeActive=active!=0;return true;
}
inline bool LegacyDeliveryReady(const Entry& e);
inline bool Catchup(Entry& e){
    if(!RefreshNativeActive()||!Observe(e))return false;
    if(e.kind==Kind::Door)return true;
    if(!e.registered){e.stage="authority_environment_original_mesh_unregistered_not_modified";return true;}
    if(!Ownership(e))return Fail(4608,"authority_environment_catchup_requires_live_real_host_authority");
    const bool newGS=e.setupGS!=state.gameState||!Same(e.setupGSIdentity,state.gameStateIdentity);
    const bool transition=e.lastActive!=static_cast<int8_t>(state.nativeActive);
    const bool newLegacyConnection=e.kind==Kind::DeprecatedBarrier&&e.multicastConnectionRevision!=state.connectionRevision;
    if(!newGS&&!transition&&!newLegacyConnection){e.stage=e.collisionMatches?"authority_environment_original_barrier_state_observed":"authority_environment_same_transition_readback_pending";return true;}
    if(e.kind==Kind::Barrier){
        if(!e.boundToCurrentGS&&(newGS||!e.originalSetupCalled)){
            if(!Dispatch(e.actor,methods.barrierInit,nullptr))return Fail(4609,"authority_environment_original_barrier_initialize_failed");
            e.originalSetupCalled=true;++e.setupCalls;++state.setupCalls;
        }
        // Its original IsGameplayActive query may itself mature a pending
        // native timeline. Re-read the actual native result before any catchup
        // callback; never replay an earlier observed active value over it.
        if(!RefreshNativeActive()||!Observe(e))return false;
        if(!e.collisionMatches){uint8_t active=state.nativeActive?1:0;
            if(!Dispatch(e.actor,methods.barrierActive,&active))return Fail(4610,"authority_environment_original_barrier_active_catchup_failed");
            ++e.activeCallbacks;++state.activeCallbacks;
        }
    }else if(e.kind==Kind::ProjectileBarrier){
        if(!e.boundToCurrentGS){struct Set {void* target;void* gameState;} parameters{e.actor,state.gameState};
            if(!Dispatch(e.actor,methods.projectileSet,&parameters))return Fail(4611,"authority_environment_original_projectile_game_state_binding_failed");
            e.originalSetupCalled=true;++e.setupCalls;++state.setupCalls;
            if(!ActiveBinding(e,e.boundToCurrentGS)||!e.boundToCurrentGS)return Fail(4612,"authority_environment_original_projectile_delegate_not_observed");
        }
        if(!e.collisionMatches){uint8_t active=state.nativeActive?1:0;
            if(!Dispatch(e.actor,methods.projectileActive,&active))return Fail(4613,"authority_environment_original_projectile_active_catchup_failed");
            ++e.activeCallbacks;++state.activeCallbacks;
        }
    }else if(state.nativeActive&&(!e.collisionMatches||newLegacyConnection)){
        // The shipped legacy event is Net|Reliable|NetMulticast. Never replace
        // it with local-only collision calls when the actor is not networked.
        if(!e.replicatedActor||e.remoteRole!=1){e.stage="authority_environment_legacy_original_multicast_actor_not_network_eligible";return true;}
        if(!LegacyDeliveryReady(e)){++state.legacyChannelDeferrals;e.stage="authority_environment_legacy_waiting_original_actor_channel_and_level_ack";return true;}
        if(!Dispatch(e.actor,methods.deprecatedDrop,nullptr))return Fail(4614,"authority_environment_original_legacy_drop_multicast_failed");
        e.nativeMulticastRequested=true;e.multicastConnectionRevision=state.connectionRevision;++state.multicastRequests;
    }
    if(!Live(e.actor,e.actorIdentity)||!Observe(e))return false;
    e.setupGS=state.gameState;e.setupGSIdentity=state.gameStateIdentity;e.lastActive=state.nativeActive?1:0;
    e.stage=e.collisionMatches?"authority_environment_original_host_barrier_catchup_readback_confirmed_client_unproven"
        :"authority_environment_original_host_barrier_readback_pending";return true;
}
inline bool Query(void* type,Kind kind){
    struct QueryActors {void* world;void* type;Array actors{};} query{state.world,type};
    static_assert(sizeof(QueryActors)==32);
    if(!Dispatch(engine::DefaultObject(engine::Class(0x43a9670)),methods.allActors,&query))return Fail(4615,"authority_environment_original_actor_query_failed");
    ++state.queries;
    const bool valid=query.actors.count>=0&&query.actors.count<=static_cast<int32_t>(MaxActors)
        &&query.actors.capacity>=query.actors.count&&(!query.actors.count||original_players::Readable(query.actors.data,static_cast<size_t>(query.actors.count)*8));
    bool okay=valid;
    if(valid)for(int32_t n=0;n<query.actors.count&&okay;++n){void* actor=query.actors.data[n];Weak identity{};
        if(!original_inventory_slots::CaptureIdentity(actor,identity)){okay=Fail(4616,"authority_environment_original_actor_weak_identity_capture_failed");break;}
        Entry* entry=nullptr;
        for(auto& candidate:entries)if(candidate.used&&candidate.actor==actor&&Same(candidate.actorIdentity,identity)){entry=&candidate;break;}
        if(!entry)for(auto& candidate:entries)if(!candidate.used||!Live(candidate.actor,candidate.actorIdentity)){
            candidate={};candidate.used=true;candidate.actor=actor;candidate.actorIdentity=identity;candidate.kind=kind;entry=&candidate;break;}
        if(!entry){okay=Fail(4617,"authority_environment_original_actor_registry_capacity_exceeded");break;}
        entry->seen=true;okay=state.clientPassive?Observe(*entry):Catchup(*entry);
        if(okay&&state.clientPassive&&entry->kind!=Kind::Door)entry->stage=entry->collisionMatches
            ?"authority_environment_client_passive_original_barrier_readback_matches_native_active"
            :"authority_environment_client_passive_original_barrier_collision_or_registration_pending";
    }
    if(query.actors.data)engine::function<void(*)(void*)>(0x1915720)(query.actors.data);
    return okay?true:(state.error?false:Fail(4618,"authority_environment_original_actor_query_array_invalid"));
}
inline bool SameName(const Name& a,const Name& b){return a.words[0]==b.words[0]&&a.words[2]==b.words[2];}
inline bool ObserveLayers(){
    state.clientLayersReady=true;
    for(size_t n=0;n<state.layerCount;++n){auto& layer=layers[n];
        const auto oldObject=layer.object,oldLevel=layer.level;const bool oldVisible=layer.visible,oldLoaded=layer.loaded;
        const bool oldShouldLoad=layer.shouldLoad,oldShouldVisible=layer.shouldVisible,oldBlockLoad=layer.blockLoad,oldBlockUnload=layer.blockUnload;
        const auto oldLod=layer.lod;
        if(!Live(layer.object,layer.identity)){
            struct Lookup {void* world;Name package;uint32_t pad=0;void* result=nullptr;} query{state.world,layer.package};
            static_assert(sizeof(Lookup)==32&&offsetof(Lookup,result)==24);
            if(!Dispatch(engine::DefaultObject(engine::Class(0x43a9670)),methods.streamingLevel,&query))
                return Fail(4632,"authority_environment_original_streaming_lookup_failed");
            layer.object=query.result;layer.level=nullptr;
            if(layer.object){Name actual{};
                if(!original_players::IsA(layer.object,layer.spec->alwaysLoaded?0x3e271a0:0x3e27300)
                   ||!training_maps::WorldContains(state.world,layer.object)
                   ||!Dispatch(layer.object,methods.worldPackage,&actual)||!SameName(actual,layer.package)
                   ||!original_inventory_slots::CaptureIdentity(layer.object,layer.identity))
                    return Fail(4633,"authority_environment_original_streaming_object_identity_or_package_mismatch");
            }
        }
        layer.loaded=layer.visible=layer.shouldLoad=layer.shouldVisible=false;layer.level=nullptr;
        if(layer.object){uint8_t loaded=0,visible=0,shouldLoad=0;
            if(!Dispatch(layer.object,methods.levelLoaded,&loaded)||!Dispatch(layer.object,methods.levelVisible,&visible)
               ||!Dispatch(layer.object,methods.shouldLoad,&shouldLoad)||!Dispatch(layer.object,methods.loadedLevel,&layer.level)
               ||loaded>1||visible>1||shouldLoad>1)return Fail(4634,"authority_environment_original_streaming_readback_failed");
            layer.loaded=loaded!=0;layer.visible=visible!=0;layer.shouldLoad=shouldLoad!=0;
            layer.shouldVisible=(Read<uint8_t>(layer.object,0xf8)&8)!=0;layer.lod=Read<int32_t>(layer.object,0xf4);
            layer.blockLoad=(Read<uint8_t>(layer.object,0xfb)&4)!=0;layer.blockUnload=(Read<uint8_t>(layer.object,0xfb)&8)!=0;
            if(layer.level&&!original_inventory_slots::CaptureIdentity(layer.level,layer.levelIdentity))
                return Fail(4635,"authority_environment_original_loaded_level_identity_capture_failed");
        }
        const auto action=layer.spec->action;
        layer.satisfied=action==training_maps::Action::Preserve||(layer.object&&(training_maps::IsLoad(action)
            ?(layer.loaded&&layer.visible&&layer.level):(!layer.visible&&(layer.spec->alwaysLoaded||!layer.shouldLoad))));
        state.clientLayersReady=state.clientLayersReady&&layer.satisfied;
        if(oldObject!=layer.object||oldLevel!=layer.level||oldVisible!=layer.visible||oldLoaded!=layer.loaded)discoveryPending=true;
        if(!layer.revision||oldObject!=layer.object||oldShouldLoad!=layer.shouldLoad||oldShouldVisible!=layer.shouldVisible
           ||oldBlockLoad!=layer.blockLoad||oldBlockUnload!=layer.blockUnload||oldLod!=layer.lod){
            layer.revision=++state.layerRevision;
        }
        ++state.layerReads;
    }return true;
}
inline bool ConnectionOwnership(const Connection& entry){
    const auto driver=Read<void*>(state.world,0x40);
    return driver&&Read<void*>(driver,0x178)==state.world&&Read<void*>(entry.connection,0x60)==driver
        &&Live(entry.connection,entry.identity)&&Live(entry.controller,entry.controllerIdentity)
        &&original_players::IsA(entry.connection,0x3dea9c0)&&original_players::IsA(entry.controller,0x3c0a200)
        &&Read<void*>(entry.connection,0x38)==entry.controller&&Read<void*>(entry.connection,0xa0)==entry.controller
        &&Read<void*>(entry.controller,0x510)==entry.connection
        &&Read<uint8_t>(entry.controller,0x224)==3;
}
inline bool LegacyDeliveryReady(const Entry& e){
    size_t valid=0;
    for(const auto& entry:connections)if(entry.used&&ConnectionOwnership(entry)){
        ++valid;auto gate=Read<void*>(Read<void**>(entry.connection),0x3c8);
        if(gate!=reinterpret_cast<void*>(engine::image+0x47113e0)
           ||!reinterpret_cast<bool(*)(void*,void*)>(gate)(entry.connection,e.actor))return false;
        auto channel=engine::function<void*(*)(void*,const Weak*)>(0x4313720)(entry.connection,&e.actorIdentity);
        if(!channel||Read<void*>(channel,0x30)!=entry.connection||Read<void*>(channel,0x78)!=e.actor||(Read<uint8_t>(channel,0x38)&2))return false;
    }return valid>0;
}
inline bool RegisterConnectionImpl(void* connection,void* controller){
    if(!Thread()||state.clientPassive||state.error)return false;
    for(auto& item:connections)if(item.used&&item.connection==connection&&item.controller==controller&&ConnectionOwnership(item))return true;
    Connection candidate{};candidate.used=true;candidate.connection=connection;candidate.controller=controller;
    if(!original_inventory_slots::CaptureIdentity(connection,candidate.identity)
       ||!original_inventory_slots::CaptureIdentity(controller,candidate.controllerIdentity)||!ConnectionOwnership(candidate))
        return Fail(4636,"authority_environment_streaming_requires_actual_admitted_connection_controller_pair");
    auto gate=Read<void*>(Read<void**>(connection),0x3c8);
    if(gate!=reinterpret_cast<void*>(engine::image+0x47113e0))return Fail(4637,"authority_environment_original_actor_level_ack_gate_profile_mismatch");
    for(auto& item:connections)if(!item.used||!ConnectionOwnership(item)){item=candidate;++state.connectionRevision;nextObserve=0;return true;}
    return Fail(4638,"authority_environment_connection_registry_capacity_exceeded");
}
inline bool PublishStreaming(){
    state.connections=state.connectionsAcknowledged=0;
    for(auto& entry:connections)if(entry.used){
        if(!ConnectionOwnership(entry)){entry.used=false;continue;}
        ++state.connections;entry.required=entry.acknowledged=entry.sent=0;size_t batch=0;
        for(size_t n=0;n<state.layerCount;++n){const auto& layer=layers[n];
            if(!layer.object||!layer.revision)continue;
            // Preserve remains the authored client lifecycle. All requests are
            // for this pinned map's managed original streaming objects only.
            if(layer.spec->action==training_maps::Action::Preserve)continue;
            ++entry.required;
            if(entry.sentRevision[n]!=layer.revision&&batch<8){
                struct Status {Name package;uint8_t load=0,visible=0,blockLoad=0,pad=0;int32_t lod=-1;uint32_t transaction=0;uint8_t blockUnload=0,pad2[3]{};};
                static_assert(sizeof(Status)==28&&offsetof(Status,lod)==16&&offsetof(Status,transaction)==20&&offsetof(Status,blockUnload)==24);
                Status parameters{};parameters.package=layer.package;parameters.load=layer.shouldLoad?1:0;parameters.visible=layer.shouldVisible?1:0;
                parameters.blockLoad=layer.blockLoad?1:0;parameters.blockUnload=layer.blockUnload?1:0;parameters.lod=layer.lod;
                // Zero is the original non-transaction request. Native
                // LevelStreaming4642e80 handles it; no client token is forged.
                if(!Dispatch(entry.controller,methods.clientStreaming,&parameters))return Fail(4639,"authority_environment_original_client_streaming_rpc_dispatch_failed");
                entry.sentRevision[n]=layer.revision;++entry.originalRequests;++state.streamingRequests;++batch;
            }
            if(entry.sentRevision[n]==layer.revision)++entry.sent;
            // Exact original package visibility query used by v3C8. It reads
            // the ACK-maintained native ClientVisibleLevelNames and retains
            // its own original cache updates. Never write either collection.
            const bool visible=engine::function<bool(*)(void*,const Name*)>(0x4711550)(entry.connection,&layer.package);
            ++entry.visibilityQueries;++state.nativeVisibilityQueries;
            entry.visibleAcknowledged[n]=visible==layer.shouldVisible&&entry.sentRevision[n]==layer.revision;
            if(entry.visibleAcknowledged[n])++entry.acknowledged;
        }
        entry.ready=entry.required>0&&entry.required==entry.sent&&entry.required==entry.acknowledged;
        if(entry.ready)++state.connectionsAcknowledged;
    }
    state.connectionsReady=state.connections>0&&state.connections==state.connectionsAcknowledged;return true;
}
inline bool InitializeImpl(void* world,void* gameState,const char* mapId,bool passive=false){
    if(state.initialized)return state.world==world&&state.gameState==gameState&&state.clientPassive==passive&&std::strcmp(state.mapId,mapId?mapId:"")==0&&Thread();
    if(!original_players::Thread()||world!=original_players::world||Read<void*>(world,0x178)!=gameState||!mapId
       ||std::strlen(mapId)>=sizeof(state.mapId)||!training_maps::Find(mapId)
       ||!original_players::IsA(gameState,original_players::ShooterGameStateClass)
       ||(!passive&&Read<uint8_t>(gameState,0x224)!=3))
        return Fail(4619,"authority_environment_initial_world_game_state_or_map_identity_invalid");
    const int mode=engine::function<int(*)(void*)>(0x3efc820)(gameState);
    if(passive?mode!=3:(mode!=1&&mode!=2))return Fail(4620,"authority_environment_native_host_or_passive_client_netmode_mismatch");
    state.world=world;state.gameState=gameState;state.clientPassive=passive;strcpy_s(state.mapId,mapId);state.gameThread=GetCurrentThreadId();
    const auto spec=training_maps::Find(mapId);
    if(spec->count>MaxLayers)return Fail(4640,"authority_environment_pinned_map_layer_capacity_exceeded");
    char package[256]{};wchar_t wide[256]{};
    if(!training_maps::WorldPackage(world,package,wide)||_wcsicmp(wide,spec->worldPath)!=0)
        return Fail(4641,"authority_environment_actual_main_world_package_mismatch");
    if(!original_inventory_slots::CaptureIdentity(world,state.worldIdentity)||!original_inventory_slots::CaptureIdentity(gameState,state.gameStateIdentity)
       ||!InitializeProfile()||!InitializeStreamingProfile())return false;
    state.layerCount=spec->count;
    for(size_t n=0;n<spec->count;++n){layers[n].spec=&spec->layers[n];engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&layers[n].package,spec->layers[n].package,1);}
    state.initialized=true;state.stage=passive?"authority_environment_client_passive_observer_ready":"authority_environment_ready_for_original_late_game_state_catchup";return true;
}
inline bool PollImpl(bool passive){
    if(!Thread()||state.error||state.clientPassive!=passive)return false;
    if(!Live(state.world,state.worldIdentity)||!Live(state.gameState,state.gameStateIdentity)||Read<void*>(state.world,0x178)!=state.gameState)
        return Fail(4621,"authority_environment_world_or_game_state_changed_requires_new_native_context");
    if(!Live(barrierClass,state.barrierClassIdentity)||!Live(projectileClass,state.projectileClassIdentity)
       ||!Live(deprecatedClass,state.deprecatedClassIdentity)||!Live(doorClass,state.doorClassIdentity)){
        if(!InitializeProfile())return false;nextObserve=0;discoveryPending=true;
    }
    const uint64_t now=GetTickCount64();const uint8_t active=Read<uint8_t>(state.gameState,0x811);
    if(active>1)return Fail(4622,"authority_environment_original_game_info_active_value_invalid");
    const bool changed=state.nativeActive!=(active!=0);state.nativeActive=active!=0;
    if(now<nextObserve&&!changed)return true;nextObserve=now+250;++state.polls;
    if(!ObserveLayers())return false;
    state.hostLayersReady=!passive&&training_maps::Ready()&&training_maps::State().world==state.world
        &&training_maps::State().map==training_maps::Find(state.mapId);
    if(!passive&&!state.hostLayersReady){state.hostReady=false;state.stage="authority_environment_waiting_original_host_map_layers";return true;}
    if(!passive&&!PublishStreaming())return false;
    // Original world actor discovery happens only at initial observation or
    // a real loaded/visible level transition. Ordinary250ms polls operate on
    // this bounded weak registry. No recurring whole-world/GUObject scan.
    if(discoveryPending){
        for(auto& e:entries)e.seen=false;
        if(!Query(barrierClass,Kind::Barrier)||!Query(projectileClass,Kind::ProjectileBarrier)
           ||!Query(deprecatedClass,Kind::DeprecatedBarrier)||!Query(doorClass,Kind::Door))return false;
        discoveryPending=false;++state.discoveries;
    }else for(auto& e:entries)if(e.used&&e.seen){
        if(!Live(e.actor,e.actorIdentity)){e.seen=false;continue;}
        if(!(passive?Observe(e):Catchup(e)))return false;
        if(passive&&e.kind!=Kind::Door)e.stage=e.collisionMatches?"authority_environment_client_passive_original_barrier_readback_matches_native_active"
            :"authority_environment_client_passive_original_barrier_collision_or_registration_pending";
    }
    state.barriers=state.projectileBarriers=state.deprecatedBarriers=state.doors=state.unresolved=0;
    for(const auto& e:entries)if(e.used&&e.seen){switch(e.kind){case Kind::Barrier:++state.barriers;break;
        case Kind::ProjectileBarrier:++state.projectileBarriers;break;case Kind::DeprecatedBarrier:++state.deprecatedBarriers;break;case Kind::Door:++state.doors;break;}
        if(!e.collisionMatches)++state.unresolved;
    }
    state.hostReady=!passive&&state.unresolved==0;
    state.clientReadbacksMatch=passive&&state.clientLayersReady&&state.unresolved==0;
    state.stage=passive?(state.clientReadbacksMatch?"authority_environment_original_client_passive_readbacks_match_local_native_state"
        :"authority_environment_original_client_passive_layer_or_barrier_readbacks_pending")
        :(state.hostReady?"authority_environment_original_host_readbacks_confirmed_client_collision_unproven"
        :"authority_environment_original_host_barrier_catchup_incomplete");return true;
}
inline int Capture(EXCEPTION_POINTERS* e){
    const auto at=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;state.restartRequired=true;
    Fail(e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:4623,"authority_environment_native_exception_requires_new_session");return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* world,void* gameState,const char* mapId){__try{return InitializeImpl(world,gameState,mapId);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool InitializeClient(void* world,void* gameState,const char* mapId){__try{return InitializeImpl(world,gameState,mapId,true);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool RegisterConnection(void* connection,void* controller){__try{return RegisterConnectionImpl(connection,controller);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool PollAuthority(){__try{return PollImpl(false);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool PollClientPassive(){__try{return PollImpl(true);}__except(Capture(GetExceptionInformation())){return false;}}
inline size_t RegistryActors(void** output,size_t capacity){
    if(state.clientPassive)return 0;
    size_t count=0;for(const auto& e:entries)if(e.used&&e.seen&&Live(e.actor,e.actorIdentity)
        &&e.replicatedActor&&e.remoteRole==1&&(e.kind==Kind::Door||e.kind==Kind::DeprecatedBarrier)){
        if(output&&count<capacity)output[count]=e.actor;++count;
    }return count;
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* out=nullptr;if(_wfopen_s(&out,path,L"wb")||!out)return false;
    const auto b=[](bool v){return v?"true":"false";};
    std::fprintf(out,"{\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"host_ready\":%s,\"map_id\":\"%s\",\"native_active\":%s,\"barriers\":%u,\"projectile_barriers\":%u,\"deprecated_barriers\":%u,\"doors\":%u,\"unresolved\":%u,\"setup_calls\":%llu,\"active_callbacks\":%llu,\"original_multicast_requests\":%llu,\"client_collision_verified\":false,\"runtime_acceptance_claimed\":false,\"actors\":[",
        state.stage,state.error,b(state.initialized),b(state.hostReady),state.mapId,b(state.nativeActive),state.barriers,state.projectileBarriers,state.deprecatedBarriers,state.doors,state.unresolved,
        static_cast<unsigned long long>(state.setupCalls),static_cast<unsigned long long>(state.activeCallbacks),static_cast<unsigned long long>(state.multicastRequests));
    bool comma=false;for(const auto& e:entries)if(e.used&&e.seen){std::fprintf(out,"%s{\"kind\":%u,\"actor\":\"%p\",\"stage\":\"%s\",\"registered\":%s,\"collision_enabled\":%u,\"host_readback_matches_active\":%s,\"bound_to_current_gs\":%s,\"native_replicated_actor\":%s,\"role\":%u,\"remote_role\":%u,\"native_door_status\":%u}",
        comma?",":"",static_cast<unsigned>(e.kind),e.actor,e.stage,b(e.registered),static_cast<unsigned>(e.collision),b(e.collisionMatches),b(e.boundToCurrentGS),b(e.replicatedActor),static_cast<unsigned>(e.role),static_cast<unsigned>(e.remoteRole),static_cast<unsigned>(e.doorStatus));comma=true;}
    std::fprintf(out,"],\"passive_client\":%s,\"client_layers_ready\":%s,\"client_local_readbacks_match\":%s,\"actor_discoveries\":%llu,\"original_actor_queries\":%llu,\"layer_reads\":%llu,\"native_streaming_actual_parms\":%u,\"original_streaming_requests\":%llu,\"native_visibility_queries\":%llu,\"connections_ready\":%s,\"connections\":[",
        b(state.clientPassive),b(state.clientLayersReady),b(state.clientReadbacksMatch),static_cast<unsigned long long>(state.discoveries),
        static_cast<unsigned long long>(state.queries),static_cast<unsigned long long>(state.layerReads),static_cast<unsigned>(state.streamingActualParms),
        static_cast<unsigned long long>(state.streamingRequests),static_cast<unsigned long long>(state.nativeVisibilityQueries),b(state.connectionsReady));
    comma=false;for(const auto& e:connections)if(e.used){std::fprintf(out,"%s{\"connection\":\"%p\",\"controller\":\"%p\",\"ready\":%s,\"required\":%zu,\"sent\":%zu,\"native_visibility_matches\":%zu,\"original_requests\":%llu}",
        comma?",":"",e.connection,e.controller,b(e.ready),e.required,e.sent,e.acknowledged,static_cast<unsigned long long>(e.originalRequests));comma=true;}
    std::fputs("],\"layers\":[",out);comma=false;
    for(size_t n=0;n<state.layerCount;++n){const auto& e=layers[n];std::fprintf(out,"%s{\"package\":\"%ls\",\"action\":\"%s\",\"object\":\"%p\",\"level\":\"%p\",\"loaded\":%s,\"visible\":%s,\"should_load\":%s,\"should_visible\":%s,\"lod\":%d,\"catalog_policy_satisfied\":%s,\"revision\":%llu}",
        comma?",":"",e.spec->package,training_maps::ActionName(e.spec->action),e.object,e.level,b(e.loaded),b(e.visible),b(e.shouldLoad),b(e.shouldVisible),e.lod,b(e.satisfied),static_cast<unsigned long long>(e.revision));comma=true;}
    std::fputs("]}\n",out);const bool okay=!std::ferror(out);std::fclose(out);return okay;
}
} // namespace local_server_environment
