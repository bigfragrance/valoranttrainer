#pragma once
#include "local_server_phoenix_c_native_observer.h"
#include "local_server_phoenix_c_result_data.h"
#include "local_server_phoenix_c_result_profiles.h"

// Observe the real original ActorListContext Reset inside GetContext after the
// original projectile factory has returned. No added callback, producer replay,
// Tick, factory, resource operation or replication admission exists here.
namespace local_server_phoenix_c_result_observer {
namespace origin=local_server_phoenix_c_native_observer;namespace data=local_server_phoenix_c_birth_data;
using Weak=original_inventory_slots::WeakIdentity;using View=local_server_hero_abilities::SourceView;using Exec=data::Exec;
using Receipt=origin::Receipt;using Array=origin::Array;
inline constexpr size_t MaxActors=64;
struct State {
    bool initialized=false,closed=false,closing=false,busy=false,profiles=false,metadata=false;
    void *world=nullptr,*function=nullptr,*functionOwner=nullptr,*contextType=nullptr;Weak worldId{},functionId{},ownerId{},contextTypeId{};
    DWORD thread=0;uint64_t installs=0,restores=0,captures=0,faults=0,beforeSamples=0,postSamples=0;
    uint64_t resultEvents=0,emptyEvents=0,actorResults=0,invalid=0,lastAt=0;uint32_t lastException=0;
    uint64_t nativeArrayWitnesses=0,nativeEmptyArrays=0;int32_t lastNativeCount=-1,lastNativeCapacity=-1;bool nativeCountKnown=false;
    const char* stage="phoenix_c_result_not_initialized";
};
struct Event {
    uint64_t serial=0,at=0;View source{};void *context=nullptr,*component=nullptr,*machine=nullptr,*selectedClass=nullptr;
    Weak contextId{},componentId{},machineId{},classId{};uintptr_t outerRsp=0,getterRsp=0;
    Array native{};int32_t contextBefore=0,contextAfter=0;bool valid=false,resetCompleted=false,arrayStable=false,sourceStable=false,addToContext=false;
};
struct ActorResult {uint64_t event=0;int32_t index=-1;Receipt actor{};};
struct Snapshot {State state{};Event last{};std::array<ActorResult,MaxActors> actors{};};
inline State state{};inline Event last{};inline std::array<ActorResult,MaxActors> actors{};
inline Snapshot cached{};inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline Exec original=nullptr;inline volatile LONG64 entries=0,forwards=0,completed=0;inline volatile LONG activeCallbacks=0;
inline void Observe(void* object,void* frame,void* result);
template<class T>inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Live(void* p,const Weak& id){return p&&original_inventory_slots::ResolveIdentity(id)==p;}
inline bool Same(const Weak& a,const Weak& b){return original_inventory_slots::SameIdentity(a,b);}
inline bool Capture(void* p,Weak& id){++state.captures;return original_inventory_slots::CaptureIdentity(p,id)&&Live(p,id);}
inline const State& GetState(){return state;}
inline bool Thread(){return state.thread&&GetCurrentThreadId()==state.thread&&original_players::Thread();}
inline void Cache(){AcquireSRWLockExclusive(&cacheLock);cached={state,last,actors};ReleaseSRWLockExclusive(&cacheLock);}
inline bool MetadataLive(){
    return Live(state.world,state.worldId)&&Live(state.function,state.functionId)&&Live(state.functionOwner,state.ownerId)
        &&Live(state.contextType,state.contextTypeId)&&Read<void*>(state.function,0x28)==state.functionOwner
        &&Read<uint16_t>(state.function,0xbe)==0&&Read<uint8_t>(state.function,0xbc)==0
        &&Read<uint32_t>(state.function,0xb8)==0x08020c00U&&!Read<void*>(state.function,0xd0)
        &&(!state.initialized||Read<Exec>(state.function,0xe0)==&Observe)
        &&std::memcmp(static_cast<uint8_t*>(state.function)+0x18,reinterpret_cast<void*>(engine::image+0xd86dd70),12)==0;
}
inline bool Machine(const origin::Producer& p,void* value,Weak& id){
    id={};if(!origin::ProducerCurrent(p)||Read<void*>(p.component,0xe0)!=value||Read<void*>(value,0xc8)!=p.view.ability
       ||!original_players::IsA(value,0x67722c0)||Read<void*>(value,0x1c8)!=p.component)return false;
    for(const auto& player:local_server_hero_abilities::GetPlayers())if(player.used&&!player.retired&&player.authority
        &&player.context.pawn==p.view.pawn&&player.epoch==p.view.sourceEpoch&&player.registration==p.view.registration){
        for(size_t i=0;i<player.count;++i){const auto& item=player.items[i].native;if(item.actor!=p.view.ability||item.componentCount>item.components.size())continue;
            for(size_t n=0;n<item.componentCount;++n)if(item.components[n]==value&&Live(value,item.componentIds[n])){id=item.componentIds[n];return true;}}
    }return false;
}
struct Call {
    void *object=nullptr,*frame=nullptr,*result=nullptr;Event before{};origin::Producer producer{};
    std::array<void*,MaxActors> returned{};std::array<Receipt,MaxActors> facts{};bool sampled=false;
};
inline bool ReadWitness(Call& c,Event& event){
    event={};if(!MetadataLive()||!origin::Type(c.object,state.contextType)||Read<void*>(c.object,0x10)!=state.contextType
       ||Read<uintptr_t>(Read<void*>(c.object),0x2f8)!=engine::image+0x7147b40
       ||!original_players::Readable(c.frame,0x20)||Read<void*>(c.frame,0x10)!=state.function||Read<void*>(c.frame,0x18)!=c.object)return false;
    const auto scan=origin::ScanFrames();
    for(size_t n=0;n+2<scan.count;++n){const auto& getter=scan.rows[n];const auto& wrapper=scan.rows[n+1];const auto& outer=scan.rows[n+2];
        if(getter.rip!=engine::image+0x6f77ccc||!origin::ProcessEventFrame(scan,n,c.object,state.function))continue;
        auto p=origin::FindProducer(reinterpret_cast<void*>(outer.rsi));if(!p||!p->beginSource||!origin::ProducerCurrent(*p))continue;
        auto machine=Read<void*>(p->component,0xe0);Weak machineId{};
        if(!local_server_phoenix_c_result_data::Frames(getter,wrapper,outer,engine::image,scan.stack,
            reinterpret_cast<uintptr_t>(c.object),reinterpret_cast<uintptr_t>(p->component),reinterpret_cast<uintptr_t>(machine))
           ||!origin::StackRead(scan.stack,outer.rsp,0x340)||Read<uint8_t>(p->component,0x370)>1
           ||!Machine(*p,machine,machineId)||Read<void*>(c.object,0x28)!=p->view.ability)continue;
        const auto selected=Read<void*>(reinterpret_cast<void*>(outer.rbp+0x208));
        const auto array=Read<Array>(reinterpret_cast<void*>(outer.rbp-0x60));
        const auto contextArray=Read<Array>(c.object,0x38);
        if(selected!=p->type||!Live(selected,p->typeId)||!origin::ValidArray(array)||!origin::ValidArray(contextArray))continue;
        event.source=p->view;event.context=c.object;event.component=p->component;event.machine=machine;event.selectedClass=selected;
        event.componentId=p->componentId;event.machineId=machineId;event.classId=p->typeId;event.outerRsp=outer.rsp;event.getterRsp=getter.rsp;
        event.native=array;event.contextBefore=contextArray.count;event.addToContext=Read<uint8_t>(p->component,0x370)!=0;
        if(!Capture(c.object,event.contextId))return false;c.producer=*p;event.valid=true;return true;
    }return false;
}
inline void Before(void* opaque){auto& c=*static_cast<Call*>(opaque);++state.beforeSamples;
    c.sampled=ReadWitness(c,c.before);if(!c.sampled){++state.invalid;return;}
    ++state.nativeArrayWitnesses;state.nativeCountKnown=true;state.lastNativeCount=c.before.native.count;state.lastNativeCapacity=c.before.native.capacity;
    if(!c.before.native.count)++state.nativeEmptyArrays;
    for(int32_t n=0;n<c.before.native.count;++n){const auto index=static_cast<size_t>(n);c.returned[index]=c.before.native.data[n];
        auto& fact=c.facts[index];fact.actor=c.returned[index];fact.type=c.before.selectedClass;fact.nativeRip=engine::image+0x713a23a;fact.outerRsp=c.before.outerRsp;
        if(!origin::ActorFacts(fact,c.producer)){c.sampled=false;++state.invalid;state.stage="phoenix_c_real_postfactory_array_actor_facts_unverified";return;}}
}
inline void After(void* opaque){auto& c=*static_cast<Call*>(opaque);if(!c.sampled)return;++state.postSamples;
    Event after{};if(!ReadWitness(c,after)||!Same(c.before.contextId,after.contextId)||c.before.component!=after.component
       ||!Same(c.before.componentId,after.componentId)||c.before.machine!=after.machine||!Same(c.before.machineId,after.machineId)
       ||c.before.outerRsp!=after.outerRsp||c.before.getterRsp!=after.getterRsp
       ||c.before.addToContext!=after.addToContext
       ||!local_server_hero_authority::Fields(c.before.source,after.source)||!local_server_hero_authority::WeakFields(c.before.source,after.source)
       ||!local_server_phoenix_c_result_data::SameArray(reinterpret_cast<uintptr_t>(c.before.native.data),c.before.native.count,c.before.native.capacity,
            reinterpret_cast<uintptr_t>(after.native.data),after.native.count,after.native.capacity)
       ||!data::SameElements(c.returned.data(),after.native.data,static_cast<size_t>(after.native.count))
       ||Read<int32_t>(c.object,0x40)!=0){++state.invalid;return;}
    for(int32_t n=0;n<after.native.count;++n){auto& before=c.facts[static_cast<size_t>(n)];
        if(!Live(before.actor,before.actorId)||!Live(before.type,before.typeId)||!origin::Current(before.source)
           ||Read<void*>(before.actor,0x10)!=before.type||Read<void*>(before.actor,0x210)!=before.owner
           ||Read<void*>(before.actor,0x278)!=before.instigator||!Same(Read<Weak>(before.actor,0x5b8),before.firingPSId)
           ||Read<uint8_t>(before.actor,0x224)!=3||local_server_item_ownership::ActorWorld(before.actor)!=state.world){++state.invalid;return;}}
    after.serial=++state.resultEvents;after.at=GetTickCount64();after.contextBefore=c.before.contextBefore;after.contextAfter=0;
    after.resetCompleted=after.arrayStable=after.sourceStable=true;last=after;actors={};
    if(!after.native.count)++state.emptyEvents;
    for(int32_t n=0;n<after.native.count;++n){auto& r=actors[static_cast<size_t>(n)];r.event=after.serial;r.index=n;r.actor=c.facts[static_cast<size_t>(n)];
        r.actor.originalCompleted=true;r.actor.sourceStableAfter=true;++state.actorResults;}
    state.stage="phoenix_c_original_postfactory_context_reset_result_observed";
}
inline void Forward(void* object,void* frame,void* result){InterlockedIncrement64(&forwards);original(object,frame,result);InterlockedIncrement64(&completed);}
inline void Dispatch(void* object,void* frame,void* result){
    if(!Thread()||!state.initialized||state.closed||state.closing||state.busy){Forward(object,frame,result);return;}
    Call call{};call.object=object;call.frame=frame;call.result=result;state.busy=true;
    __try{data::ForwardOnce(&Forward,object,frame,result,&Before,&After,&call,state.faults,state.lastException);}
    __finally{state.busy=false;Cache();}
}
inline void Observe(void* object,void* frame,void* result){InterlockedIncrement64(&entries);InterlockedIncrement(&activeCallbacks);
    __try{Dispatch(object,frame,result);}__finally{InterlockedDecrement(&activeCallbacks);}}
inline bool Verify(){for(const auto& p:local_server_phoenix_c_result_source::Profiles){const auto code=reinterpret_cast<void*>(engine::image+p.rva);
    if(!original_players::Readable(code,p.bytes)||origin::Hash(code,p.bytes)!=p.fnv)return false;}return true;}
inline bool InitImpl(void* world){
    if(!origin::PhoenixActualHuman(world)||!origin::GetState().initialized||!origin::GetState().profiles||!origin::GetState().schema)return false;
    if(state.initialized)return !state.closed&&state.world==world&&Thread();
    state.world=world;state.worldId=origin::GetState().worldId;state.thread=GetCurrentThreadId();
    state.function=engine::Find(L"/Script/ShooterGame.StateTransitionContext.Reset");
    state.functionOwner=engine::Class(0x63d69f0);state.contextType=engine::Class(0x6d271c0);
    if(!Verify()){state.stage="phoenix_c_result_original_source_profile_mismatch";return false;}state.profiles=true;
    if(!state.function||Read<uintptr_t>(state.function,0xe0)!=engine::image+0x2a4a930
       ||!origin::Field(state.contextType,L"Actors",0x38,16)||!origin::Field(engine::Find(L"/Script/ShooterGame.ProjectileThrowStateComponent"),L"bAddToContext",0x370,1)
       ||!Capture(state.function,state.functionId)||!Capture(state.functionOwner,state.ownerId)||!Capture(state.contextType,state.contextTypeId)
       ||!MetadataLive()||!origin::Writable(static_cast<uint8_t*>(state.function)+0xe0)){
        state.stage="phoenix_c_result_original_reset_schema_unknown";return false;}
    state.metadata=true;original=engine::function<Exec>(0x2a4a930);
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(static_cast<uint8_t*>(state.function)+0xe0),
        reinterpret_cast<void*>(&Observe),reinterpret_cast<void*>(original))!=reinterpret_cast<void*>(original)){
        state.stage="phoenix_c_result_reset_exec_foreign_layer_not_overwritten";return false;}
    ++state.installs;state.initialized=true;state.stage="phoenix_c_result_reset_observer_installed_execution_unknown";return true;
}
struct InitCall {void* world=nullptr;bool okay=false;};
inline void InitQuery(void* p){auto& c=*static_cast<InitCall*>(p);c.okay=InitImpl(c.world);}
inline bool InitHost(void* world){InitCall c{world,false};data::SafeSample(&InitQuery,&c,state.faults,state.lastException);Cache();return c.okay;}
inline bool CloseImpl(){
    if(!state.thread){state.closed=true;return true;}if(!Thread())return false;state.closing=true;
    if(state.busy||InterlockedCompareExchange(&activeCallbacks,0,0))return false;
    if(state.initialized&&Live(state.function,state.functionId)){
        const auto address=static_cast<uint8_t*>(state.function)+0xe0;const auto actual=Read<Exec>(state.function,0xe0);
        if(actual!=original){if(actual!=&Observe||!origin::Writable(address)
            ||InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(address),reinterpret_cast<void*>(original),reinterpret_cast<void*>(&Observe))!=reinterpret_cast<void*>(&Observe))return false;++state.restores;}}
    state.closed=true;state.stage="phoenix_c_result_original_exec_restored_or_expired_generation_not_written";return true;
}
inline void CloseQuery(void* p){*static_cast<bool*>(p)=CloseImpl();}
inline bool Close(){bool okay=false;data::SafeSample(&CloseQuery,&okay,state.faults,state.lastException);Cache();return okay;}
inline std::string Json(){Snapshot s{};AcquireSRWLockShared(&cacheLock);s=cached;ReleaseSRWLockShared(&cacheLock);
    local_server_match::Json out;const auto b=[](bool v){return v?"true":"false";};const auto u=[](uint64_t v){return static_cast<unsigned long long>(v);};
    out.Add("{\"stage\":\"%s\",\"initialized\":%s,\"closed\":%s,\"profiles_verified\":%s,\"metadata_verified\":%s,\"entries\":%lld,\"original_forward_attempts\":%lld,\"original_forward_completed\":%lld,\"active_callbacks\":%ld,\"exec_install_writes\":%llu,\"exec_restore_writes\":%llu,\"weak_capture_calls\":%llu,\"sample_faults\":%llu,\"last_sample_exception\":%u,\"before_samples\":%llu,\"after_samples\":%llu,\"invalid_samples\":%llu,\"real_postfactory_result_events\":%llu,\"real_empty_result_events\":%llu,\"actual_actor_results\":%llu,\"last\":",
        s.state.stage,b(s.state.initialized),b(s.state.closed),b(s.state.profiles),b(s.state.metadata),InterlockedCompareExchange64(&entries,0,0),InterlockedCompareExchange64(&forwards,0,0),InterlockedCompareExchange64(&completed,0,0),InterlockedCompareExchange(&activeCallbacks,0,0),u(s.state.installs),u(s.state.restores),u(s.state.captures),u(s.state.faults),s.state.lastException,u(s.state.beforeSamples),u(s.state.postSamples),u(s.state.invalid),u(s.state.resultEvents),u(s.state.emptyEvents),u(s.state.actorResults));
    const auto& e=s.last;if(!e.serial)out.Add("null");else out.Add("{\"serial\":%llu,\"at_ms\":%llu,\"context\":\"%p\",\"component\":\"%p\",\"machine\":\"%p\",\"selected_class\":\"%p\",\"pawn\":\"%p\",\"epoch\":%u,\"registration\":%llu,\"actual_native_result_count\":%d,\"context_before_count\":%d,\"context_after_count\":%d,\"original_add_to_context\":%s,\"original_reset_completed\":%s,\"array_stable\":%s,\"source_stable\":%s,\"outer_rsp\":%llu,\"getter_rsp\":%llu}",
        u(e.serial),u(e.at),e.context,e.component,e.machine,e.selectedClass,e.source.pawn,e.source.sourceEpoch,u(e.source.registration),e.native.count,e.contextBefore,e.contextAfter,b(e.addToContext),b(e.resetCompleted),b(e.arrayStable),b(e.sourceStable),u(e.outerRsp),u(e.getterRsp));
    out.Add(",\"actors\":[");bool comma=false;for(const auto& a:s.actors)if(a.event){const auto& r=a.actor;
        out.Add("%s{\"event\":%llu,\"index\":%d,\"actor\":\"%p\",\"class\":\"%p\",\"actor_weak\":[%d,%d],\"ability\":\"%p\",\"pawn\":\"%p\",\"player_state\":\"%p\",\"epoch\":%u,\"registration\":%llu,\"original_breplicates\":%s,\"initialized_timestamp_raw_bits\":%u,\"original_factory_completed_before_reset\":true}",
            comma?",":"",u(a.event),a.index,r.actor,r.type,r.actorId.index,r.actorId.serial,r.source.ability,r.source.pawn,r.source.playerState,r.source.sourceEpoch,u(r.source.registration),b(r.originalReplicates),r.initializedBits);comma=true;}
    out.Add("],\"actual_native_array_witnesses\":%llu,\"actual_empty_native_arrays\":%llu,\"last_native_result_count_known\":%s,\"last_native_result_count\":%d,\"last_native_result_capacity\":%d,\"frame_code_reads\":0,\"frame_step_calls\":0,\"frame_writes\":0,\"synthetic_reset_or_state_calls\":0,\"actor_publications\":0,\"source_registry_enabled\":false,\"skill_or_client_delivery_verified\":false,\"weak_serial_allocation_possible\":true,\"scope\":\"actual_phoenix_c_original_getcontext_branch_only\"}",
        u(s.state.nativeArrayWitnesses),u(s.state.nativeEmptyArrays),b(s.state.nativeCountKnown),s.state.lastNativeCount,s.state.lastNativeCapacity);return out.value;
}
} // namespace local_server_phoenix_c_result_observer
