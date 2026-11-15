#pragma once
#include "original_inventory_slots.h"
#include <cstddef>
#include <cstdio>
#include <cstring>

// Missing local-authority publication only. The shipped ViewTarget selector
// and ShooterPC's Info.TargetActor assignment are replay-gated. For the actual
// admitted native-IP owner, publish only the replicated logical target after
// the full original candidate/eligibility queries accept its current pawn.
// The original client RepNotify owns target application and camera scheduling.
// Static evidence: work/local-server-playable/client-presentation/viewtarget.md.
namespace local_server_viewtarget {
using Weak=original_inventory_slots::WeakIdentity;
enum class Result {Waiting,Published,Failed};
struct Array {uint8_t* data=nullptr;int32_t count=0,capacity=0;};
struct Info {void* target=nullptr;uint8_t mode=0,reserved[7]{};};
static_assert(sizeof(Info)==16&&alignof(Info)==8&&offsetof(Info,mode)==8);
struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
inline constexpr Profile RequiredProfiles[]{
    {0x7859fe0,96,0xe2457381930782e2ULL},
    {0x785a040,991,0xb5a6c244c388d123ULL},
    {0x1a7b720,196,0x4982523f72f33036ULL},
    {0x1c63870,269,0xde792136130ef719ULL},
    {0x1cf35b0,1571,0x40614eae57888396ULL},
    {0x1d4cfa0,1562,0xb050d7efb4cf1a25ULL},
    {0x3efc820,176,0x7f1113b3f79164b2ULL},
    {0x3ed83b0,1609,0x15d2d4ecc2f49cf7ULL},
    {0x3f06b50,475,0xb2c819ad7afdb41aULL},
    {0x4131210,31,0x1acf0c18fa27cc39ULL},
    {0x2a64f70,21,0xe351780b84707f42ULL},
    {0x7859b80,280,0x6aa6558516a28dfeULL},
};
struct Diagnostics {
    bool profilesVerified=false,metadataVerified=false,ownerVerified=false,bound=false;
    bool componentReplicates=false,usesRegisteredList=false,componentInReplicationList=false;
    bool replicationRouteVerified=false,candidateMatches=false,originalEligible=false;
    bool published=false,preservedOriginalPolicy=false,restartRequired=false;
    unsigned error=0;
    uint32_t calls=0,waits=0,publications=0,idempotentCalls=0,replicatedComponents=0,registeredComponents=0;
    int32_t netMode=-1,registeredCondition=-1;
    uint8_t tickModeEnabled=0,allowedModes=0,defaultMode=0,hasBeenCameraReady=0;
    uint8_t canBeViewTarget=0,cameraState=255,componentFlagsA0=0,componentFlagsC1=0;
    uintptr_t exceptionRva=0;
    const char* stage="authority_view_target_not_published";
    const char* operation="none";
    void *world=nullptr,*controller=nullptr,*playerState=nullptr,*pawn=nullptr,*expectedPrevious=nullptr;
    void *driver=nullptr,*connection=nullptr,*component=nullptr,*candidate=nullptr,*property=nullptr,*onRep=nullptr;
    void *localBefore=nullptr,*localAfter=nullptr;
    Info before{},after{};
};
inline Diagnostics state{};
struct Binding {Weak world{},controller{},playerState{},driver{},connection{},component{};};
inline Binding binding{};
inline const Diagnostics& GetState(){return state;}
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline Result Fail(unsigned error,const char* reason){state.error=error;state.published=false;state.stage=reason;return Result::Failed;}
inline Result Waiting(const char* reason){++state.waits;state.published=false;state.stage=reason;return Result::Waiting;}
inline bool Live(void* p,const Weak& id){return p&&original_inventory_slots::ResolveIdentity(id)==p;}
inline bool Identity(void* p,Weak& id){return original_inventory_slots::CaptureIdentity(p,id)&&Live(p,id);}
inline bool ValidArray(const Array& a,size_t stride,int32_t maximum){
    return a.count>=0&&a.count<=maximum&&a.capacity>=a.count&&a.capacity<=4096
        &&(!a.count||(a.data&&original_players::Readable(a.data,size_t(a.count)*stride)));
}
inline bool VerifyProfiles(){
    for(const auto& profile:RequiredProfiles){
        const auto* code=reinterpret_cast<const uint8_t*>(engine::image+profile.rva);
        if(!original_players::Readable(code,profile.bytes))return false;
        uint64_t hash=14695981039346656037ULL;
        for(size_t i=0;i<profile.bytes;++i)hash=(hash^code[i])*1099511628211ULL;
        if(hash!=profile.fnv)return false;
    }
    return true;
}
inline void* ActorWorld(void* actor){
    auto target=Read<void*>(Read<void*>(actor),0x1b8);
    return target==reinterpret_cast<void*>(engine::image+0x3ed83b0)
        ?reinterpret_cast<void*(*)(void*)>(target)(actor):nullptr;
}
inline bool Metadata(){
    uint32_t name[3]{};
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(name,L"ViewTargetInfo",1);
    state.property=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)
        (Read<void*>(state.component,0x10),name,3);
    state.onRep=engine::Find(L"/Script/ShooterGame.ViewTargetComponent.OnRep_ViewTargetInfo");
    constexpr uint64_t netAndRepNotify=0x100000020ULL;
    const auto p=state.property;
    return p&&Read<uint64_t>(p,0x20)==Read<uint64_t>(name)
        &&Read<int32_t>(p,0x30)==1&&Read<int32_t>(p,0x34)==16&&Read<int32_t>(p,0x44)==0x170
        &&(Read<uint64_t>(p,0x38)&netAndRepNotify)==netAndRepNotify
        &&state.onRep&&Read<uint16_t>(state.onRep,0xbe)==16
        &&Read<uint32_t>(state.onRep,0xb8)==0x40401
        &&Read<void*>(state.onRep,0xe0)==reinterpret_cast<void*>(engine::image+0x6dd7720)
        &&Read<void*>(state.onRep,0x28)==engine::Class(0x6dd74a0);
}
inline bool ReplicationRoute(){
    auto& d=state;
    const auto componentTable=Read<void*>(d.component),controllerTable=Read<void*>(d.controller);
    d.replicationRouteVerified=false;d.componentInReplicationList=false;d.registeredCondition=-1;
    d.componentFlagsA0=Read<uint8_t>(d.component,0xa0);d.componentFlagsC1=Read<uint8_t>(d.component,0xc1);
    // ActorComponent.SetIsReplicated4131070 owns A0 bit20 and the owner's
    // UpdateReplicatedComponent3EE5170 lists. C1 bit10 alone is not evidence.
    d.componentReplicates=(d.componentFlagsA0&0x20)!=0;
    d.usesRegisteredList=(Read<uint8_t>(d.controller,0x11b)&1)!=0;
    const auto components=Read<Array>(d.controller,0x330),registered=Read<Array>(d.controller,0x320);
    if(!ValidArray(components,8,512)||!ValidArray(registered,0x28,512))return false;
    d.replicatedComponents=uint32_t(components.count);d.registeredComponents=uint32_t(registered.count);
    unsigned matches=0;
    if(d.usesRegisteredList){
        for(int32_t i=0;i<registered.count;++i){
            const auto* row=registered.data+size_t(i)*0x28;
            if(Read<void*>(row)!=d.component)continue;
            ++matches;d.registeredCondition=Read<int32_t>(row,0x10);
            if(!Live(d.component,Read<Weak>(row,8)))return false;
            // Ordinary admitted owner supports None, OwnerOnly,
            // InitialOrOwner and SkipReplay. Other policies remain pending.
            if(d.registeredCondition!=0&&d.registeredCondition!=2
               &&d.registeredCondition!=7&&d.registeredCondition!=13)return false;
        }
    }else{
        for(int32_t i=0;i<components.count;++i)if(Read<void*>(components.data,size_t(i)*8)==d.component)++matches;
    }
    d.componentInReplicationList=matches==1;
    // Native ActorChannel42DDC40 ->42DF350 -> either registered records or
    // Actor.v3503F06B50 -> component.v318 ->42E5F20 ->42ED290. A component's
    // v318 false return does not suppress replication of the component itself.
    d.replicationRouteVerified=d.componentReplicates&&d.componentInReplicationList
        &&Read<void*>(controllerTable,0x350)==reinterpret_cast<void*>(engine::image+0x3f06b50)
        &&Read<void*>(componentTable,0x220)==reinterpret_cast<void*>(engine::image+0x7859b80)
        &&Read<void*>(componentTable,0x318)==reinterpret_cast<void*>(engine::image+0x4131210)
        &&Read<void*>(componentTable,0x320)==reinterpret_cast<void*>(engine::image+0x2a64f70)
        &&(Read<uint32_t>(d.component,8)&0x60000000U)==0;
    return d.replicationRouteVerified;
}
inline Result PublishAuthorityViewTargetImpl(void* world,void* controller,void* playerState,
                                              void* pawn,void* expectedPrevious){
    auto& d=state;
    if(d.error||d.restartRequired)return Result::Failed;
    ++d.calls;d.published=false;d.candidateMatches=false;d.originalEligible=false;d.ownerVerified=false;
    if(!original_players::Thread())return Fail(3980,"authority_view_target_requires_original_game_thread");
    if(d.bound&&(!Live(d.world,binding.world)||!Live(d.controller,binding.controller)
       ||!Live(d.playerState,binding.playerState)||!Live(d.driver,binding.driver)
       ||!Live(d.connection,binding.connection)||!Live(d.component,binding.component)
       ||d.world!=world||d.controller!=controller||d.playerState!=playerState
       ||Read<void*>(world,0x40)!=d.driver||Read<void*>(controller,0x510)!=d.connection
       ||Read<void*>(controller,0xc60)!=d.component))
        return Fail(3981,"authority_view_target_context_changed_requires_new_session");
    d.world=world;d.controller=controller;d.playerState=playerState;d.pawn=pawn;d.expectedPrevious=expectedPrevious;
    if(!d.profilesVerified){
        d.profilesVerified=VerifyProfiles();
        if(!d.profilesVerified)return Fail(3982,"authority_view_target_pinned_source_profile_mismatch");
    }
    if(!world||original_players::world!=world||!original_players::IsA(controller,original_players::ShooterControllerClass)
       ||!original_players::IsA(playerState,original_players::ShooterStateClass)
       ||!original_players::IsA(pawn,original_players::ShooterCharacterClass)
       ||Read<uint8_t>(controller,0x224)!=3||Read<uint8_t>(playerState,0x224)!=3||Read<uint8_t>(pawn,0x224)!=3
       ||Read<uint8_t>(pawn,0x128)!=2||Read<void*>(controller,0x470)!=playerState
       ||Read<void*>(playerState,0x210)!=controller||Read<void*>(controller,0x4b0)!=pawn
       ||Read<void*>(pawn,0x4a0)!=controller||Read<void*>(pawn,0x488)!=playerState
       ||ActorWorld(controller)!=world||ActorWorld(playerState)!=world||ActorWorld(pawn)!=world)
        return Fail(3983,"authority_view_target_requires_actual_network_authority_possession");
    d.netMode=engine::function<int32_t(*)(void*)>(0x3efc820)(controller);
    d.driver=Read<void*>(world,0x40);d.connection=Read<void*>(controller,0x510);
    if((d.netMode!=1&&d.netMode!=2)||!d.driver||Read<void*>(d.driver,0x178)!=world
       ||Read<void*>(d.driver,0xc0)||Read<void*>(world,0xf8)
       ||!original_players::IsA(d.connection,0x3dea9c0)||Read<void*>(d.connection,0x60)!=d.driver
       ||Read<void*>(d.connection,0x38)!=controller||Read<void*>(d.connection,0xa0)!=controller
       ||Read<uint8_t>(d.connection,0x13c)!=3||Read<uint8_t>(d.connection,0x1d8)!=3
       ||!Read<void*>(d.connection,0x70))
        return Fail(3984,"authority_view_target_requires_admitted_native_ip_connection_owner");
    const auto clients=Read<Array>(d.driver,0xc8);
    if(!ValidArray(clients,8,8))return Fail(3985,"authority_view_target_native_recipient_list_invalid");
    unsigned connections=0;
    for(int32_t i=0;i<clients.count;++i)if(Read<void*>(clients.data,size_t(i)*8)==d.connection)++connections;
    if(connections!=1)return Fail(3986,"authority_view_target_connection_not_uniquely_registered");
    d.ownerVerified=true;
    if((Read<uint8_t>(world,0x156)&0x80)==0||!Read<uint8_t>(pawn,0xa90)
       ||!original_players::Registered(playerState)||Read<void*>(playerState,0xbc8)!=pawn||Read<void*>(playerState,0xbd0)!=pawn)
        return Waiting("authority_view_target_waiting_original_begun_pawn_and_playerstate_links");
    if(expectedPrevious==pawn)return Fail(3987,"authority_view_target_previous_pawn_is_current_pawn");
    auto component=Read<void*>(controller,0xc60);
    if(d.bound&&d.component!=component)return Fail(3988,"authority_view_target_component_changed_requires_new_session");
    d.component=component;
    if(!original_players::IsA(component,0x6dd74a0)||Read<void*>(component,0xc8)!=controller)
        return Fail(3989,"authority_view_target_original_default_subobject_or_owner_missing");
    if(!d.bound){
        if(!Identity(world,binding.world)||!Identity(controller,binding.controller)||!Identity(playerState,binding.playerState)
           ||!Identity(d.driver,binding.driver)||!Identity(d.connection,binding.connection)||!Identity(component,binding.component))
            return Fail(3990,"authority_view_target_context_weak_identities_unavailable");
        d.bound=true;
    }
    Weak pawnId{};
    if(!Identity(pawn,pawnId))return Fail(3991,"authority_view_target_current_pawn_identity_unavailable");
    d.metadataVerified=Metadata();
    if(!d.metadataVerified)return Fail(3992,"authority_view_target_original_net_repnotify_metadata_mismatch");
    d.tickModeEnabled=Read<uint8_t>(component,0x168);d.allowedModes=Read<uint8_t>(component,0x16a);
    d.defaultMode=Read<uint8_t>(component,0x16b);d.hasBeenCameraReady=Read<uint8_t>(component,0x169);
    d.canBeViewTarget=Read<uint8_t>(pawn,0x1290);d.cameraState=Read<uint8_t>(pawn,0x12a0);
    d.before=Read<Info>(component,0x170);d.localBefore=Read<void*>(component,0x180);
    d.after=d.before;d.localAfter=d.localBefore;
    if(!ReplicationRoute())return Waiting("authority_view_target_waiting_original_component_replication_route");
    if(d.before.mode!=1||(d.allowedModes&1)==0||!d.tickModeEnabled)
        return Waiting("authority_view_target_waiting_original_watch_possessed_policy");
    if(d.before.target&&d.before.target!=pawn&&d.before.target!=expectedPrevious)
        return Fail(3993,"authority_view_target_existing_logical_target_conflicts_with_expected_transition");
    if(d.localBefore&&d.localBefore!=controller&&d.localBefore!=pawn&&d.localBefore!=expectedPrevious)
        return Waiting("authority_view_target_existing_local_target_has_other_native_policy");
    d.operation="original_watch_possessed_candidate_and_full_eligibility";
    d.candidate=engine::function<void*(*)(void*)>(0x7859fe0)(component);
    d.candidateMatches=d.candidate==pawn;
    if(!d.candidateMatches)return Waiting("authority_view_target_original_candidate_not_current_pawn");
    d.originalEligible=engine::function<bool(*)(void*,void*,uint8_t)>(0x785a040)(component,pawn,1);
    if(!d.originalEligible)return Waiting("authority_view_target_original_candidate_not_eligible");
    if(!Live(pawn,pawnId)||Read<void*>(controller,0x4b0)!=pawn||Read<void*>(playerState,0xbc8)!=pawn
       ||Read<void*>(playerState,0xbd0)!=pawn||Read<Info>(component,0x170).target!=d.before.target
       ||Read<uint8_t>(component,0x178)!=d.before.mode)
        return Fail(3994,"authority_view_target_candidate_or_logical_record_changed_during_query");
    if(d.before.target!=pawn){
        // Sole engine-state write: the authority's replicated logical actor
        // reference. Keep the original mode and padding exactly as received.
        // Root's existing ActorChannel writer sends the registered component;
        // the real client's original OnRep applies its own local camera path.
        d.operation="publish_authority_replicated_view_target_actor";
        if(!original_players::Write(component,0x170,pawn))
            return Fail(3995,"authority_view_target_logical_reference_publication_failed");
        ++d.publications;
    }else ++d.idempotentCalls;
    d.after=Read<Info>(component,0x170);d.localAfter=Read<void*>(component,0x180);
    d.preservedOriginalPolicy=d.after.mode==d.before.mode
        &&std::memcmp(d.after.reserved,d.before.reserved,sizeof(d.before.reserved))==0
        &&d.localAfter==d.localBefore&&Read<uint8_t>(component,0x168)==d.tickModeEnabled
        &&Read<uint8_t>(component,0x169)==d.hasBeenCameraReady&&Read<uint8_t>(component,0x16a)==d.allowedModes
        &&Read<uint8_t>(component,0x16b)==d.defaultMode;
    if(d.after.target!=pawn||!d.preservedOriginalPolicy)
        return Fail(3996,"authority_view_target_publication_or_original_policy_readback_mismatch");
    d.published=true;d.operation="none";
    d.stage="authority_view_target_published_waiting_native_client_replication";
    return Result::Published;
}
inline LONG CaptureException(EXCEPTION_POINTERS* exception){
    if(exception&&exception->ExceptionRecord){
        state.error=exception->ExceptionRecord->ExceptionCode;
        const auto address=reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress);
        state.exceptionRva=address>=engine::image?address-engine::image:address;
    }else state.error=3997;
    state.restartRequired=true;state.published=false;
    state.stage="authority_view_target_native_exception_requires_new_session";
    return EXCEPTION_EXECUTE_HANDLER;
}
inline Result PublishAuthorityViewTarget(void* world,void* controller,void* playerState,
                                          void* pawn,void* expectedPrevious=nullptr){
    __try {return PublishAuthorityViewTargetImpl(world,controller,playerState,pawn,expectedPrevious);}
    __except(CaptureException(GetExceptionInformation())) {return Result::Failed;}
}
inline void WriteState(FILE* f){
    if(!f)return;const auto& d=state;auto b=[](bool v){return v?"true":"false";};
    std::fprintf(f,"{\"stage\":\"%s\",\"error\":%u,\"calls\":%u,\"waits\":%u,\"publications\":%u,\"idempotent_calls\":%u,\"published\":%s,\"profiles_verified\":%s,\"metadata_verified\":%s,\"owner_verified\":%s,\"component_replicates\":%s,\"uses_registered_list\":%s,\"component_in_replication_list\":%s,\"replication_route_verified\":%s,\"replicated_components\":%u,\"registered_components\":%u,\"registered_condition\":%d,\"native_candidate_matches\":%s,\"native_eligible\":%s,\"original_policy_preserved\":%s,\"net_mode\":%d,\"world\":\"%p\",\"controller\":\"%p\",\"player_state\":\"%p\",\"pawn\":\"%p\",\"component\":\"%p\",\"candidate\":\"%p\",\"before_target\":\"%p\",\"after_target\":\"%p\",\"local_before\":\"%p\",\"local_after\":\"%p\",\"mode\":%u,\"default_mode\":%u,\"allowed_modes\":%u,\"tick_mode_enabled\":%u,\"has_been_camera_ready\":%u,\"can_be_view_target\":%u,\"character_camera_state\":%u,\"component_flags_a0\":%u,\"component_flags_c1\":%u,\"operation\":\"%s\",\"exception_rva\":%llu,\"restart_required\":%s,\"client_state_writes\":0,\"camera_calls\":0,\"client_replication_verified\":false,\"playable_verified\":false}",
        d.stage,d.error,d.calls,d.waits,d.publications,d.idempotentCalls,b(d.published),b(d.profilesVerified),b(d.metadataVerified),b(d.ownerVerified),b(d.componentReplicates),b(d.usesRegisteredList),b(d.componentInReplicationList),b(d.replicationRouteVerified),d.replicatedComponents,d.registeredComponents,d.registeredCondition,b(d.candidateMatches),b(d.originalEligible),b(d.preservedOriginalPolicy),d.netMode,d.world,d.controller,d.playerState,d.pawn,d.component,d.candidate,d.before.target,d.after.target,d.localBefore,d.localAfter,unsigned(d.after.mode),unsigned(d.defaultMode),unsigned(d.allowedModes),unsigned(d.tickModeEnabled),unsigned(d.hasBeenCameraReady),unsigned(d.canBeViewTarget),unsigned(d.cameraState),unsigned(d.componentFlagsA0),unsigned(d.componentFlagsC1),d.operation,static_cast<unsigned long long>(d.exceptionRva),b(d.restartRequired));
}
}
