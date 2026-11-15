#pragma once
#include "local_server_phoenix_c_native_observer.h"
#include "local_server_phoenix_c_actor_gate_data.h"
#include "local_server_phoenix_c_actor_gate_profiles.h"

// Current original Actor::ProcessEvent gate values only. It never makes an
// Actor.ProcessEvent/HasActorBegunPlay query through that potentially gated
// path, and never changes a world/actor/global flag or lifecycle state.
namespace local_server_phoenix_c_actor_gate {
namespace origin=local_server_phoenix_c_native_observer;namespace data=local_server_phoenix_c_actor_gate_data;
using Weak=origin::Weak;using View=origin::View;
enum class At:uint8_t {Metadata=0};
struct Row {
    bool valid=false,methodKnown=false,worldGateKnown=false,worldGateAllows=false,begun=false,functionKnown=false,functionAllows=false;
    uint64_t sequence=0,atMs=0;At at=At::Metadata;
    void *actor=nullptr,*type=nullptr,*world=nullptr,*level=nullptr,*owner=nullptr,*sourceAbility=nullptr,*beginFunction=nullptr;
    Weak actorId{},typeId{};uint32_t epoch=0,flags=0,functionFlags=0;uint64_t registration=0;int32_t levelCount=0,scriptCount=0;
    uint8_t role=0,world156=0,globalAllow=0,globalSuppress=0,lifecycle=0;uintptr_t processEventRva=0;
};
struct State {
    bool initialized=false,closed=false,profiles=false;void* world=nullptr;Weak worldId{};DWORD thread=0;
    uint64_t sequence=0,polls=0,faults=0,captures=0,lastPoll=0,invalid=0;uint32_t lastException=0;
    const char* stage="phoenix_actor_gate_not_initialized";
};
struct Snapshot {State state{};std::array<Row,24> rows{};};
inline State state{};inline std::array<Row,24> rows{};inline Snapshot cached{};inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline const State& GetState(){return state;}
template<class T>inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Thread(){return state.thread&&state.thread==GetCurrentThreadId()&&origin::Thread();}
inline void Cache(){AcquireSRWLockExclusive(&cacheLock);cached={state,rows};ReleaseSRWLockExclusive(&cacheLock);}
inline bool Capture(void* object,Weak& id){++state.captures;return original_inventory_slots::CaptureIdentity(object,id)&&origin::Live(object,id);}
inline bool Verify(){for(const auto& p:local_server_phoenix_c_actor_gate_source::Profiles){const auto code=reinterpret_cast<void*>(engine::image+p.rva);
    if(!original_players::Readable(code,p.bytes)||origin::Hash(code,p.bytes)!=p.fnv)return false;}return true;}
inline bool InitImpl(void* world){
    if(!origin::PhoenixActualHuman(world)||!origin::GetState().initialized||!origin::GetState().profiles)return false;
    if(state.initialized)return state.world==world&&!state.closed&&Thread();
    state.world=world;state.worldId=origin::GetState().worldId;state.thread=GetCurrentThreadId();
    if(!Verify()){state.stage="phoenix_actor_gate_source_profiles_unknown";return false;}
    state.profiles=state.initialized=true;state.stage="phoenix_actor_gate_ready_current_readback_not_call_history";return true;
}
struct Init {void* world=nullptr;bool okay=false;};
inline void InitQuery(void* opaque){auto& c=*static_cast<Init*>(opaque);c.okay=InitImpl(c.world);}
inline bool InitHost(void* world){Init c{world,false};local_server_phoenix_c_birth_data::SafeSample(&InitQuery,&c,state.faults,state.lastException);Cache();return c.okay;}
struct Query {void *actor=nullptr,*expectedClass=nullptr;Weak borrowed{};View source{};void* begin=nullptr;Weak beginId{};At at=At::Metadata;};
inline bool Sample(const Query& q,Row& out){
    out={};if(!state.initialized||state.closed||!Thread()||!state.profiles||!origin::Live(state.world,state.worldId)
       ||!origin::Current(q.source)||q.source.world!=state.world||!q.actor||!q.expectedClass
       ||Read<void*>(q.actor,0x10)!=q.expectedClass||!original_players::IsA(q.actor,0x3683900))return false;
    out.actor=q.actor;out.type=q.expectedClass;
    if(q.borrowed.serial){out.actorId=q.borrowed;if(!origin::Live(q.actor,out.actorId))return false;}
    else if(!Capture(q.actor,out.actorId))return false;
    if(!Capture(out.type,out.typeId))return false;
    out.world=local_server_item_ownership::ActorWorld(q.actor);out.role=Read<uint8_t>(q.actor,0x224);
    if(out.world!=state.world||out.role!=3)return false;
    out.at=q.at;out.sourceAbility=q.source.ability;out.epoch=q.source.sourceEpoch;out.registration=q.source.registration;
    out.flags=Read<uint32_t>(q.actor,8);out.owner=Read<void*>(q.actor,0x210);out.lifecycle=Read<uint8_t>(q.actor,0x11c);out.begun=data::BegunPlay(out.lifecycle);
    out.world156=Read<uint8_t>(out.world,0x156);out.level=Read<void*>(out.world,0x38);
    if(out.level){if(!original_players::Readable(static_cast<uint8_t*>(out.level)+0xa8,4))return false;out.levelCount=Read<int32_t>(out.level,0xa8);}
    out.globalAllow=Read<uint8_t>(reinterpret_cast<void*>(engine::image+0xd978c68));
    out.globalSuppress=Read<uint8_t>(reinterpret_cast<void*>(engine::image+0xd98e3e8));
    const auto method=Read<uintptr_t>(Read<void*>(q.actor),0x298);
    out.processEventRva=method>=engine::image?method-engine::image:0;out.methodKnown=method==engine::image+0x3eda0d0;
    data::Values values{out.world!=nullptr,out.level!=nullptr,out.world156,out.globalAllow,out.globalSuppress,out.lifecycle,out.flags,out.levelCount};
    out.worldGateKnown=out.methodKnown;out.worldGateAllows=out.methodKnown&&data::WorldGate(values);
    if(q.begin&&origin::Live(q.begin,q.beginId)){
        out.beginFunction=q.begin;out.functionFlags=Read<uint32_t>(q.begin,0xb8);out.scriptCount=Read<int32_t>(q.begin,0x70);
        out.functionKnown=out.scriptCount>=0;out.functionAllows=out.functionKnown&&data::FunctionGate(out.functionFlags,out.scriptCount);
    }
    out.valid=origin::Live(q.actor,out.actorId)&&origin::Live(out.type,out.typeId)&&origin::Current(q.source)
        &&Read<void*>(q.actor,0x10)==out.type&&local_server_item_ownership::ActorWorld(q.actor)==out.world
        &&Read<uintptr_t>(Read<void*>(q.actor),0x298)==method;
    return out.valid;
}
inline void ObserveQuery(void* opaque){const auto& q=*static_cast<const Query*>(opaque);Row row{};
    if(!Sample(q,row)){++state.invalid;return;}row.sequence=++state.sequence;row.atMs=GetTickCount64();
    rows[(row.sequence-1)%rows.size()]=row;
}
inline void PollQuery(void* opaque){
    const auto& current=*static_cast<const Query*>(opaque);
    if(!Thread()||!state.initialized||state.closed)return;const auto now=GetTickCount64();if(state.lastPoll&&now>=state.lastPoll&&now-state.lastPoll<25)return;state.lastPoll=now;++state.polls;
    for(const auto& p:origin::producers)if(p.used&&!p.retired&&p.view.pawn==current.actor&&p.view.sourceEpoch==current.source.sourceEpoch&&origin::ProducerCurrent(p)){
        Query ability{p.view.ability,Read<void*>(p.view.ability,0x10),p.view.identities[6],p.view,nullptr,{},At::Metadata};ObserveQuery(&ability);
        Query pawn{p.view.pawn,Read<void*>(p.view.pawn,0x10),p.view.identities[4],p.view,nullptr,{},At::Metadata};ObserveQuery(&pawn);break;
    }
}
inline void Poll(void* pawn,uint32_t epoch){Query q{};q.actor=pawn;q.source.sourceEpoch=epoch;local_server_phoenix_c_birth_data::SafeSample(&PollQuery,&q,state.faults,state.lastException);Cache();}
inline void Close(){state.closed=true;Cache();}
inline std::string Json(){Snapshot s{};AcquireSRWLockShared(&cacheLock);s=cached;ReleaseSRWLockShared(&cacheLock);
    local_server_match::Json out;const auto b=[](bool v){return v?"true":"false";};const auto u=[](uint64_t v){return static_cast<unsigned long long>(v);};
    out.Add("{\"stage\":\"%s\",\"initialized\":%s,\"closed\":%s,\"profiles_verified\":%s,\"polls\":%llu,\"sample_faults\":%llu,\"last_exception\":%u,\"weak_capture_calls\":%llu,\"invalid_samples\":%llu,\"rows\":[",s.state.stage,b(s.state.initialized),b(s.state.closed),b(s.state.profiles),u(s.state.polls),u(s.state.faults),s.state.lastException,u(s.state.captures),u(s.state.invalid));
    bool comma=false;for(const auto& r:s.rows)if(r.sequence){out.Add("%s{\"sequence\":%llu,\"at_ms\":%llu,\"at\":%u,\"valid\":%s,\"actor\":\"%p\",\"class\":\"%p\",\"world\":\"%p\",\"level\":\"%p\",\"owner\":\"%p\",\"source_ability\":\"%p\",\"epoch\":%u,\"registration\":%llu,\"actor_weak\":[%d,%d],\"role\":%u,\"object_flags_8\":%u,\"native_lifecycle_11c\":%u,\"has_actor_begun_play_native_read\":%s,\"world_156\":%u,\"world_156_bit40\":%s,\"level_a8\":%d,\"global_d978c68\":%u,\"global_d98e3e8\":%u,\"process_event_rva\":%llu,\"original_actor_gate_method_known\":%s,\"current_world_gate_known\":%s,\"current_world_gate_allows\":%s,\"begin_function\":\"%p\",\"function_predicate_known\":%s,\"function_nonempty_or_native\":%s}",
        comma?",":"",u(r.sequence),u(r.atMs),unsigned(r.at),b(r.valid),r.actor,r.type,r.world,r.level,r.owner,r.sourceAbility,r.epoch,u(r.registration),r.actorId.index,r.actorId.serial,unsigned(r.role),r.flags,unsigned(r.lifecycle),b(r.begun),unsigned(r.world156),b((r.world156&0x40U)!=0),r.levelCount,unsigned(r.globalAllow),unsigned(r.globalSuppress),u(r.processEventRva),b(r.methodKnown),b(r.worldGateKnown),b(r.worldGateAllows),r.beginFunction,b(r.functionKnown),b(r.functionAllows));comma=true;}
    out.Add("],\"current_values_are_historical_event_trace\":false,\"world_actor_global_flag_writes\":0,\"beginplay_or_process_event_calls\":0,\"frame_reads\":0,\"source_registry_or_actor_publication\":false,\"weak_serial_allocation_possible\":true,\"sample_at\":[\"metadata_poll_current_ability_and_pawn\"]}");return out.value;
}
} // namespace local_server_phoenix_c_actor_gate
