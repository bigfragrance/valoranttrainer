#pragma once
#include "bot_zoom_transition.h"
#include "local_server_match.h"
#include "local_server_viewtarget.h"
#include <array>
#include <cstdio>
#include <string>

// The client build initializes the complete native predicted zoom transition
// on NPC BeginPlay but leaves its authority record at the class default. Supply
// that absent server publication, then use the untouched native writer/OnRep.
// No zoom/FOV/level/time is synthesized and no client state or camera is written.
namespace local_server_zoom_authority {
using Transition=bot_zoom_transition::Transition;
using Weak=original_inventory_slots::WeakIdentity;
enum class Result{Waiting,Unchanged,Published,Failed};
struct Record{
    void* object=nullptr;void* type=nullptr;Weak identity{},typeIdentity{};
    Transition lastPublished{},defaults{};
    bool owned=false,nativeAuthorityObserved=false,schemaVerified=false;uint8_t reserved[5]{};
};
struct State{
    bool profiles=false,metadata=false,owner=false,route=false,defaultMatched=false,nativeInitialized=false;
    uint32_t index=0;uint64_t polls=0,publications=0,preservedNativeAuthority=0;
    const char* stage="native_zoom_authority_not_observed";
    Transition before{},prediction{},after{};Weak component{};
};
inline std::array<Record,32> records{};
inline State state{},published{};inline SRWLOCK snapshotLock=SRWLOCK_INIT;
inline void Snapshot(){AcquireSRWLockExclusive(&snapshotLock);published=state;ReleaseSRWLockExclusive(&snapshotLock);}
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void* object,const Weak& id){return object&&original_inventory_slots::ResolveIdentity(id)==object;}
inline bool Identity(void* object,Weak& id){return object&&original_inventory_slots::CaptureIdentity(object,id)&&Live(object,id);}
inline Result Wait(const char* reason){state.stage=reason;return Result::Waiting;}
inline bool Profiles(){
    struct Profile{uintptr_t rva;size_t bytes;uint64_t fnv;};
    constexpr Profile values[]{
        {0x6e9a6f0,254,0x9f96daa78c17689eULL},{0x6e9abe0,268,0x1296e00668d56923ULL},
        {0x6e9acf0,1937,0xb46793701d33fd88ULL},{0x6e03e50,112,0x721f8382b96c64adULL},
        {0x6e9a5d0,280,0xf43453bdf7abd6d5ULL},{0x6e9a840,652,0xc237f0d975e33937ULL},
    };
    for(const auto& value:values){
        const auto code=reinterpret_cast<const uint8_t*>(engine::image+value.rva);
        if(!original_players::Readable(code,value.bytes))return false;
        uint64_t hash=14695981039346656037ULL;
        for(size_t n=0;n<value.bytes;++n)hash=(hash^code[n])*1099511628211ULL;
        if(hash!=value.fnv)return false;
    }
    return true;
}
inline bool Schema(void* component,Transition& defaults){
    const auto method=engine::Find(L"/Script/ShooterGame.ZoomComponent.OnRep_ZoomTransition");
    const auto type=Read<void*>(component,0x10);
    if(!method||Read<uint16_t>(method,0xbe)!=0||Read<uint32_t>(method,0xb8)!=0x40401
       ||Read<uintptr_t>(method,0xe0)!=engine::image+0x6e03e50
       ||!original_players::Derived(type,Read<void*>(method,0x28)))return false;
    const auto auth=local_server_match::Property(type,L"AuthZoomTransition",36,0x4b8);
    const auto prediction=local_server_match::Property(type,L"ClientPredictedZoomTransition",36,0x4dc);
    const auto structure=engine::Class(0x6e03a70),description=engine::Class(0x6e03750);
    const auto cdo=Read<void*>(type,0x120);Weak cdoIdentity{};
    constexpr uint64_t netNotify=0x100000020ULL;
    if(!auth||!prediction||(Read<uint64_t>(auth,0x38)&netNotify)!=netNotify||(Read<uint64_t>(prediction,0x38)&0x20)
       ||Read<void*>(auth,0x78)!=structure||Read<void*>(prediction,0x78)!=structure
       ||Read<int32_t>(structure,0x60)!=36||Read<int32_t>(structure,0x64)!=4
       ||Read<int32_t>(description,0x60)!=32||Read<int32_t>(description,0x64)!=4
       ||!local_server_match::Property(structure,L"TransitionDescription",32,0)
       ||!local_server_match::Property(structure,L"TransitionState",1,32)
       ||!Identity(cdo,cdoIdentity)||Read<void*>(cdo,0x10)!=type)return false;
    defaults=Read<Transition>(cdo,0x4b8);return true;
}
inline bool Route(void* weapon,void* component){
    if((Read<uint8_t>(component,0xa0)&0x20)==0||Read<void*>(component,0xc8)!=weapon
       ||Read<void*>(Read<void*>(component),0x220)!=reinterpret_cast<void*>(engine::image+0x6e9a5d0)
       ||(Read<uint8_t>(weapon,0x11b)&1)!=0
       ||Read<void*>(Read<void*>(weapon),0x350)!=reinterpret_cast<void*>(engine::image+0x3f06b50))return false;
    const auto components=Read<local_server_viewtarget::Array>(weapon,0x330);
    if(!local_server_viewtarget::ValidArray(components,8,64))return false;
    unsigned matches=0;
    for(int32_t n=0;n<components.count;++n)if(Read<void*>(components.data,size_t(n)*8)==component)++matches;
    return matches==1;
}
inline Result PublishImpl(const local_server_match::AuthorityPlayer& player){
    ++state.polls;state.index=static_cast<uint32_t>(player.index);
    state.metadata=state.owner=state.route=state.defaultMatched=state.nativeInitialized=false;
    const auto& scope=local_server_viewtarget::GetState();const auto world=local_server_match::GetState().world;
    const auto& binding=local_server_viewtarget::binding;const auto human=local_server_match::Player(0);
    if(!original_players::Thread()||!world||!scope.bound||!scope.ownerVerified||scope.error||scope.world!=world
       ||!scope.profilesVerified||!scope.metadataVerified||!human||!human->human||human->pc!=scope.controller||human->ps!=scope.playerState
       ||!Live(world,binding.world)||!Live(scope.controller,binding.controller)||!Live(scope.playerState,binding.playerState)
       ||!Live(scope.driver,binding.driver)||!Live(scope.connection,binding.connection)||!Live(scope.component,binding.component)
       ||!scope.replicationRouteVerified||Read<void*>(world,0xf8)||Read<void*>(world,0x40)!=scope.driver
       ||Read<void*>(scope.driver,0x178)!=world||Read<void*>(scope.driver,0xc0)
       ||Read<void*>(scope.controller,0x510)!=scope.connection||Read<void*>(scope.connection,0x38)!=scope.controller
       ||Read<void*>(scope.connection,0xa0)!=scope.controller)return Wait("native_zoom_waiting_owned_normal_ip_authority");
    if(!player.index||player.index>=records.size()||player.human||!player.present||!player.ready||!player.alive
       ||!Live(player.pawn,player.pawnIdentity)||!Live(player.pc,player.pcIdentity)||!Live(player.ps,player.psIdentity)
       ||Read<uint8_t>(player.pawn,0x224)!=3||Read<uint8_t>(player.pc,0x224)!=3||Read<void*>(player.pc,0x510)
       ||Read<void*>(player.pawn,0x488)!=player.ps||Read<void*>(player.pawn,0x4a0)!=player.pc
       ||Read<void*>(player.ps,0xbc8)!=player.pawn)return Wait("native_zoom_waiting_current_authority_npc");
    if(!state.profiles){state.profiles=Profiles();if(!state.profiles)return Wait("native_zoom_source_profile_mismatch");}
    const auto inventory=Read<void*>(player.pawn,0xc08),weapon=Read<void*>(inventory,0x280);
    Weak weaponIdentity{},managerIdentity{},componentIdentity{};
    if(!Identity(weapon,weaponIdentity)||Read<uint8_t>(weapon,0x224)!=3
       ||Read<void*>(weapon,0x210)!=player.pawn||Read<void*>(weapon,0x4a8)!=player.pawn)return Wait("native_zoom_waiting_current_owned_npc_weapon");
    struct Query{void* type=nullptr;void* result=nullptr;} query{engine::Class(0x6e06bb0),nullptr};
    const auto get=engine::Find(L"/Script/Engine.Actor.GetComponentByClass");
    if(!get||Read<uint16_t>(get,0xbe)!=16||!engine::Event(weapon,L"/Script/Engine.Actor.GetComponentByClass",&query)
       ||!Identity(query.result,managerIdentity)||!original_players::IsA(query.result,0x6e06bb0)
       ||Read<void*>(query.result,0xc8)!=weapon||Read<void*>(query.result,0x100)!=player.pawn)return Wait("native_zoom_waiting_original_zoom_manager_owner");
    const auto component=Read<void*>(query.result,0xe0);
    if(!Identity(component,componentIdentity))return Wait("native_zoom_waiting_original_zoom_component");
    auto& record=records[player.index];
    const auto type=Read<void*>(component,0x10);
    if(record.object!=component||!Live(record.object,record.identity)||record.type!=type||!Live(type,record.typeIdentity)){
        record={};record.object=component;record.identity=componentIdentity;record.type=type;
        if(!Identity(type,record.typeIdentity))return Wait("native_zoom_current_class_identity_unverified");
    }
    state.component=componentIdentity;
    if(!record.schemaVerified)record.schemaVerified=Schema(component,record.defaults);
    state.metadata=record.schemaVerified;if(!state.metadata)return Wait("native_zoom_schema_unverified");
    state.owner=true;state.route=Route(weapon,component);if(!state.route)return Wait("native_zoom_original_component_replication_route_unverified");
    state.before=Read<Transition>(component,0x4b8);state.prediction=Read<Transition>(component,0x4dc);state.after=state.before;
    state.nativeInitialized=bot_zoom_transition::Initialized(state.prediction);state.defaultMatched=bot_zoom_transition::Equal(state.before,record.defaults);
    if(!state.nativeInitialized)return Wait("native_zoom_waiting_original_prediction_initialization");
    if(bot_zoom_transition::Equal(state.before,state.prediction)){
        if(!record.owned)record.nativeAuthorityObserved=true;
        state.stage="native_zoom_authority_already_matches_original_prediction";return Result::Unchanged;
    }
    if(!bot_zoom_transition::MayPublish(state.before,state.prediction,record.defaults,record.owned,record.lastPublished,record.nativeAuthorityObserved)){
        ++state.preservedNativeAuthority;record.owned=false;record.nativeAuthorityObserved=true;
        state.stage="native_zoom_existing_authority_preserved";return Result::Unchanged;
    }
    if(!Live(player.pawn,player.pawnIdentity)||!Live(weapon,weaponIdentity)||!Live(query.result,managerIdentity)
       ||!Live(component,componentIdentity)||Read<void*>(inventory,0x280)!=weapon
       ||!bot_zoom_transition::Equal(Read<Transition>(component,0x4dc),state.prediction))return Wait("native_zoom_original_producer_changed_before_publication");
    if(!original_players::Write(component,0x4b8,state.prediction)){
        state.stage="native_zoom_authority_publication_failed";return Result::Failed;
    }
    state.after=Read<Transition>(component,0x4b8);
    if(!bot_zoom_transition::Equal(state.after,state.prediction)){
        state.stage="native_zoom_authority_publication_readback_failed";return Result::Failed;
    }
    record.owned=true;record.lastPublished=state.after;++state.publications;
    state.stage="native_zoom_original_transition_published_waiting_native_replication";return Result::Published;
}
inline Result Publish(const local_server_match::AuthorityPlayer& player){const auto result=PublishImpl(player);Snapshot();return result;}
inline std::string Hex(const Transition& value){constexpr char digits[]="0123456789abcdef";std::string text;for(auto byte:value.bytes){text+=digits[byte>>4];text+=digits[byte&15];}return text;}
inline std::string Json(){
    AcquireSRWLockShared(&snapshotLock);const auto value=published;ReleaseSRWLockShared(&snapshotLock);
    const auto b=[](bool v){return v?"true":"false";};char text[1100]{};
    std::snprintf(text,sizeof(text),"{\"stage\":\"%s\",\"profiles_verified\":%s,\"metadata_verified\":%s,\"owner_verified\":%s,\"native_replication_route\":%s,\"class_default_matched\":%s,\"native_prediction_initialized\":%s,\"player_index\":%u,\"publications\":%llu,\"preserved_native_authority\":%llu,\"before_auth\":\"%s\",\"native_prediction\":\"%s\",\"after_auth\":\"%s\",\"client_writes\":0,\"onrep_calls\":0,\"zoom_requests\":0,\"camera_calls\":0}",
        value.stage,b(value.profiles),b(value.metadata),b(value.owner),b(value.route),b(value.defaultMatched),b(value.nativeInitialized),value.index,
        static_cast<unsigned long long>(value.publications),static_cast<unsigned long long>(value.preservedNativeAuthority),Hex(value.before).c_str(),Hex(value.prediction).c_str(),Hex(value.after).c_str());
    return text;
}
}
