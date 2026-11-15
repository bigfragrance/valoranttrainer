#pragma once
#include "local_server_phoenix_c_native_observer.h"
#include "local_server_phoenix_c_throw_result_data.h"
#include "local_server_phoenix_c_throw_result_profiles.h"

// Actual existing native TryReportNoiseEvent callback. Observe the real OnThrow
// broadcast result, including empty arrays; do not report noise or throw again.
namespace local_server_phoenix_c_throw_result {
namespace birth=local_server_phoenix_c_native_observer;namespace data=local_server_phoenix_c_birth_data;namespace proof=local_server_phoenix_c_throw_result_data;
using Weak=birth::Weak;using View=birth::View;using Array=birth::Array;using Exec=data::Exec;using Receipt=birth::Receipt;
inline constexpr size_t MaxActors=64;
struct State {
    bool initialized=false,closed=false,closing=false,busy=false,profiles=false,metadata=false;DWORD thread=0;
    void *world=nullptr,*function=nullptr,*owner=nullptr;Weak worldId{},functionId{},ownerId{};
    uint64_t installs=0,restores=0,captures=0,faults=0,qualified=0,completed=0,empty=0,actorFacts=0,invalid=0,frameMatches=0,currentSources=0,multicasts=0;uint32_t lastException=0;
    uint32_t initFlags=0;uint16_t initBytes=0,initReturn=0;uint8_t initParams=0;void* initScript=nullptr;int32_t initScriptCount=0,initScriptCapacity=0;uintptr_t initExecRva=0;
    const char* witnessStage="native_onthrow_not_sampled";
    const char* stage="phoenix_c_throw_result_not_initialized";
};
struct Event {
    bool valid=false,originalReturned=false,sourceStable=false,arrayStable=false;uint64_t serial=0,at=0;
    View source{};void *component=nullptr,*type=nullptr,*locals=nullptr;Weak componentId{},typeId{};
    uintptr_t outerRsp=0,outerRbp=0;size_t delegateIndex=0;void* snapshotData=nullptr;int32_t snapshotCount=0,snapshotCapacity=0;Array native{},copy{},params{};
};
struct Actor {void* pointer=nullptr;Receipt receipt{};bool factsBefore=false,factsAfter=false;};
struct Snapshot {State state{};Event last{};std::array<Actor,MaxActors> actors{};};
inline State state{};inline Event last{};inline std::array<Actor,MaxActors> actors{};inline Snapshot cached{};inline SRWLOCK lock=SRWLOCK_INIT;
inline Exec original=nullptr;inline volatile LONG64 entries=0,forwards=0,originalCompleted=0;inline volatile LONG active=0;
inline void Observer(void*,void*,void*);
template<class T>inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Live(void* p,const Weak& id){return birth::Live(p,id);}inline bool Same(const Weak& a,const Weak& b){return birth::Same(a,b);}
inline bool Capture(void* p,Weak& id){++state.captures;return birth::Capture(p,id);}
inline bool GT(){return state.thread&&state.thread==GetCurrentThreadId()&&birth::Thread();}
inline const State& GetState(){return state;}
inline void Cache(){AcquireSRWLockExclusive(&lock);cached={state,last,actors};ReleaseSRWLockExclusive(&lock);}
inline bool Metadata(){
    return Live(state.world,state.worldId)&&Live(state.function,state.functionId)&&Live(state.owner,state.ownerId)
        &&Read<void*>(state.function,0x28)==state.owner&&state.owner==birth::throwType
        &&Read<uint32_t>(state.function,0xb8)==0x00440401U&&Read<uint8_t>(state.function,0xbc)==2&&Read<uint16_t>(state.function,0xbe)==24
        &&Read<uint16_t>(state.function,0xc0)==0xffff&&!Read<void*>(state.function,0xd0)
        &&!Read<void*>(state.function,0x68)&&Read<int32_t>(state.function,0x70)==0&&Read<int32_t>(state.function,0x74)==0
        &&(!state.initialized||Read<Exec>(state.function,0xe0)==&Observer);
}
struct Call {
    void *object=nullptr,*frame=nullptr,*result=nullptr;Event before{};birth::Producer producer{};
    std::array<void*,MaxActors> pointers{};std::array<Actor,MaxActors> facts{};bool qualified=false;
};
inline bool Witness(Call& call,Event& out){
    out={};state.witnessStage="native_callback_frame_metadata_unknown";if(!GT()||!Metadata()||!state.profiles||!original_players::Readable(call.frame,0x30)
        ||Read<void*>(call.frame,0x10)!=state.function||Read<void*>(call.frame,0x18)!=call.object)return false;
    ++state.frameMatches;state.witnessStage="receiver_not_current_phoenix_c_producer";auto producer=birth::FindProducer(call.object);if(!producer||producer->retired||!birth::ProducerCurrent(*producer)
        ||producer->component!=call.object||Read<void*>(call.object,0xc8)!=producer->view.ability||producer->view.world!=state.world)return false;
    ++state.currentSources;const auto scan=birth::ScanFrames();
    state.witnessStage="original_dothrow_process_event_stack_not_matched";for(size_t at=0;at<scan.count;++at){const auto& outer=scan.rows[at];if(outer.rip!=engine::image+0x713a622
        ||!birth::ProcessEventFrame(scan,at,call.object,state.function))continue;
        state.witnessStage="original_multicast_stack_not_matched";for(size_t d=0;d<at;++d){const auto& delegate=scan.rows[d];const auto index=static_cast<size_t>(delegate.rsi);
            if(!proof::Frames(delegate,outer,engine::image,scan.stack,reinterpret_cast<uintptr_t>(call.object),index)
                ||!birth::StackRead(scan.stack,delegate.rbp-0x39,96))continue;
            state.witnessStage="original_delegate_snapshot_unknown";
            const auto heap=Read<void*>(reinterpret_cast<void*>(delegate.rbp+0x17));
            const auto snapshotData=heap?heap:reinterpret_cast<void*>(delegate.rbp-0x39);
            const auto snapshotCount=Read<int32_t>(reinterpret_cast<void*>(delegate.rbp+0x1f)),snapshotCapacity=Read<int32_t>(reinterpret_cast<void*>(delegate.rbp+0x23));
            if(!proof::SnapshotEntry(delegate.rbp,reinterpret_cast<uintptr_t>(heap),snapshotCount,snapshotCapacity,index,delegate.rbx,scan.stack)
                ||!original_players::Readable(snapshotData,static_cast<size_t>(snapshotCount)*20))continue;
            state.witnessStage="actual_delegate_snapshot_entry_not_matched";const auto entry=static_cast<uint8_t*>(snapshotData)+index*20;
            if(delegate.rbx!=reinterpret_cast<uintptr_t>(entry)||!Same(Read<Weak>(entry),producer->componentId)
                ||original_inventory_slots::ResolveIdentity(Read<Weak>(entry))!=call.object
                ||std::memcmp(entry+8,static_cast<uint8_t*>(state.function)+0x18,12)!=0)continue;
            ++state.multicasts;state.witnessStage="original_callback_params_not_matched";const auto component=Read<void*>(reinterpret_cast<void*>(outer.rbp-0x30));
            const auto selected=Read<void*>(reinterpret_cast<void*>(outer.rbp+0x208));
            const auto native=Read<Array>(reinterpret_cast<void*>(outer.rbp-0x60)),copy=Read<Array>(reinterpret_cast<void*>(outer.rbp-0x28));
            const auto locals=Read<void*>(call.frame,0x28);if(component!=call.object||!original_players::Readable(locals,24)||Read<void*>(locals)!=call.object)continue;
            const auto params=Read<Array>(locals,8);state.witnessStage="original_selected_class_or_array_witness_not_matched";
            if(selected!=producer->type||!Live(selected,producer->typeId)||!birth::ValidArray(native)||!birth::ValidArray(copy)
                ||!proof::Headers(reinterpret_cast<uintptr_t>(native.data),native.count,native.capacity,reinterpret_cast<uintptr_t>(copy.data),copy.count,copy.capacity,
                    reinterpret_cast<uintptr_t>(params.data),params.count,params.capacity)
                ||outer.rdi!=reinterpret_cast<uintptr_t>(native.data)||static_cast<uint32_t>(outer.r15)!=static_cast<uint32_t>(native.count)
                ||!data::SameElements(native.data,copy.data,static_cast<size_t>(native.count)))continue;
            out.valid=true;out.source=producer->view;out.component=call.object;out.componentId=producer->componentId;out.type=selected;out.typeId=producer->typeId;
            out.locals=locals;out.snapshotData=snapshotData;out.snapshotCount=snapshotCount;out.snapshotCapacity=snapshotCapacity;out.native=native;out.copy=copy;out.params=params;out.outerRsp=outer.rsp;out.outerRbp=outer.rbp;out.delegateIndex=index;
            call.producer=*producer;state.witnessStage="actual_original_dothrow_array_and_native_callback_matched";return true;
        }
    }return false;
}
inline void Before(void* opaque){auto& call=*static_cast<Call*>(opaque);call.qualified=Witness(call,call.before);if(!call.qualified){++state.invalid;return;}
    call.before.serial=++state.qualified;call.before.at=GetTickCount64();last=call.before;actors={};if(!call.before.native.count)++state.empty;
    for(int32_t n=0;n<call.before.native.count;++n){const auto k=static_cast<size_t>(n);auto& a=call.facts[k];a.pointer=call.before.native.data[n];call.pointers[k]=a.pointer;
        a.receipt.actor=a.pointer;a.receipt.type=call.before.type;a.receipt.nativeRip=engine::image+0x713a622;a.receipt.outerRsp=call.before.outerRsp;
        a.factsBefore=birth::ActorFacts(a.receipt,call.producer);actors[k]=a;if(a.factsBefore)++state.actorFacts;}
    state.stage="phoenix_c_real_postfactory_onthrow_array_before_original_native_callback";
}
inline void After(void* opaque){auto& call=*static_cast<Call*>(opaque);if(!call.qualified)return;
    Event after{};if(!Witness(call,after)||after.outerRsp!=call.before.outerRsp||after.outerRbp!=call.before.outerRbp
        ||after.component!=call.before.component||!Same(after.componentId,call.before.componentId)||after.type!=call.before.type||!Same(after.typeId,call.before.typeId)
        ||!local_server_hero_authority::Fields(after.source,call.before.source)||!local_server_hero_authority::WeakFields(after.source,call.before.source)
        ||after.native.data!=call.before.native.data||after.native.count!=call.before.native.count||after.native.capacity!=call.before.native.capacity
        ||after.snapshotData!=call.before.snapshotData||after.snapshotCount!=call.before.snapshotCount||after.snapshotCapacity!=call.before.snapshotCapacity
        ||after.copy.data!=call.before.copy.data||after.copy.count!=call.before.copy.count||after.copy.capacity!=call.before.copy.capacity
        ||!data::SameElements(call.pointers.data(),after.native.data,static_cast<size_t>(after.native.count))){++state.invalid;return;}
    after.serial=call.before.serial;after.at=call.before.at;after.originalReturned=after.sourceStable=after.arrayStable=true;last=after;++state.completed;
    for(int32_t n=0;n<after.native.count;++n){const auto k=static_cast<size_t>(n);auto a=call.facts[k];if(a.factsBefore){Receipt fresh{};fresh.actor=a.pointer;fresh.type=after.type;
        a.factsAfter=birth::ActorFacts(fresh,call.producer)&&Same(a.receipt.actorId,fresh.actorId)&&Same(a.receipt.typeId,fresh.typeId);
        a.receipt.originalCompleted=true;a.receipt.sourceStableAfter=a.factsAfter;}actors[k]=a;}
    state.stage="phoenix_c_real_postfactory_array_and_original_native_callback_returned";
}
inline void Forward(void* object,void* frame,void* result){InterlockedIncrement64(&forwards);original(object,frame,result);InterlockedIncrement64(&originalCompleted);}
inline void Dispatch(void* object,void* frame,void* result){
    if(!GT()||!state.initialized||state.closed||state.closing||state.busy){Forward(object,frame,result);return;}
    Call call{};call.object=object;call.frame=frame;call.result=result;state.busy=true;
    __try{data::ForwardOnce(&Forward,object,frame,result,&Before,&After,&call,state.faults,state.lastException);}__finally{state.busy=false;Cache();}
}
inline void Observer(void* object,void* frame,void* result){InterlockedIncrement64(&entries);InterlockedIncrement(&active);
    __try{Dispatch(object,frame,result);}__finally{InterlockedDecrement(&active);}}
inline bool Profiles(){for(const auto& p:local_server_phoenix_c_throw_result_source::Profiles){const auto code=reinterpret_cast<void*>(engine::image+p.rva);
    if(!original_players::Readable(code,p.bytes)||birth::Hash(code,p.bytes)!=p.fnv)return false;}return true;}
inline bool InitImpl(void* world){
    if(!birth::PhoenixActualHuman(world)||!birth::GetState().initialized||!birth::GetState().profiles||!birth::GetState().schema)return false;
    if(state.initialized)return !state.closed&&state.world==world&&GT();state.world=world;state.worldId=birth::GetState().worldId;state.thread=GetCurrentThreadId();
    if(!Profiles()){state.stage="phoenix_c_throw_native_original_profile_unknown";return false;}state.profiles=true;
    state.function=engine::Find(L"/Script/ShooterGame.ProjectileThrowStateComponent.TryReportNoiseEvent");state.owner=birth::throwType;
    original=engine::function<Exec>(0x6b2f4c0);
    if(state.function){state.initFlags=Read<uint32_t>(state.function,0xb8);state.initParams=Read<uint8_t>(state.function,0xbc);state.initBytes=Read<uint16_t>(state.function,0xbe);state.initReturn=Read<uint16_t>(state.function,0xc0);
        state.initScript=Read<void*>(state.function,0x68);state.initScriptCount=Read<int32_t>(state.function,0x70);state.initScriptCapacity=Read<int32_t>(state.function,0x74);const auto e=Read<uintptr_t>(state.function,0xe0);state.initExecRva=e>=engine::image?e-engine::image:0;}
    if(!Capture(state.function,state.functionId)||!Capture(state.owner,state.ownerId)||!Metadata()
        ||Read<Exec>(state.function,0xe0)!=original||!birth::Field(state.function,L"ThrowState",0,8)||!birth::Field(state.function,L"Projectiles",8,16)
        ||!birth::Writable(static_cast<uint8_t*>(state.function)+0xe0)){state.stage="phoenix_c_throw_actual_native_schema_unknown";return false;}
    state.metadata=true;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(static_cast<uint8_t*>(state.function)+0xe0),reinterpret_cast<void*>(&Observer),reinterpret_cast<void*>(original))!=reinterpret_cast<void*>(original)){
        state.stage="phoenix_c_throw_foreign_exec_not_overwritten";return false;}
    ++state.installs;state.initialized=true;state.stage="phoenix_c_actual_native_onthrow_observer_installed_execution_unknown";return true;
}
struct Init {void* world=nullptr;bool okay=false;};inline void InitQuery(void* p){auto& c=*static_cast<Init*>(p);c.okay=InitImpl(c.world);}
inline bool InitHost(void* world){Init c{world,false};data::SafeSample(&InitQuery,&c,state.faults,state.lastException);Cache();return c.okay;}
inline bool CloseImpl(){
    if(!state.thread){state.closed=true;return true;}if(!GT())return false;state.closing=true;
    if(state.busy||InterlockedCompareExchange(&active,0,0))return false;
    if(state.initialized&&Live(state.function,state.functionId)){const auto slot=static_cast<uint8_t*>(state.function)+0xe0;const auto current=Read<Exec>(state.function,0xe0);
        if(current!=original){if(current!=&Observer||!birth::Writable(slot)
            ||InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(slot),reinterpret_cast<void*>(original),reinterpret_cast<void*>(&Observer))!=reinterpret_cast<void*>(&Observer))return false;++state.restores;}}
    state.closed=true;state.stage="phoenix_c_actual_native_onthrow_observer_restored_or_expired_not_written";return true;
}
inline void CloseQuery(void* p){*static_cast<bool*>(p)=CloseImpl();}inline bool Close(){bool okay=false;data::SafeSample(&CloseQuery,&okay,state.faults,state.lastException);Cache();return okay;}
inline std::string Json(){Snapshot s{};AcquireSRWLockShared(&lock);s=cached;ReleaseSRWLockShared(&lock);local_server_match::Json out;const auto b=[](bool v){return v?"true":"false";};const auto u=[](uint64_t v){return static_cast<unsigned long long>(v);};
    out.Add("{\"stage\":\"%s\",\"initialized\":%s,\"closed\":%s,\"profiles_verified\":%s,\"metadata_verified\":%s,\"entries\":%lld,\"original_forwards\":%lld,\"original_returns\":%lld,\"active\":%ld,\"exec_installs\":%llu,\"exec_restores\":%llu,\"sample_faults\":%llu,\"last_exception\":%u,\"weak_capture_calls\":%llu,\"qualified_arrays\":%llu,\"qualified_native_returns\":%llu,\"real_empty_arrays\":%llu,\"actors_with_source_facts_before\":%llu,\"invalid_samples\":%llu,",
        s.state.stage,b(s.state.initialized),b(s.state.closed),b(s.state.profiles),b(s.state.metadata),InterlockedCompareExchange64(&entries,0,0),InterlockedCompareExchange64(&forwards,0,0),InterlockedCompareExchange64(&originalCompleted,0,0),InterlockedCompareExchange(&active,0,0),u(s.state.installs),u(s.state.restores),u(s.state.faults),s.state.lastException,u(s.state.captures),u(s.state.qualified),u(s.state.completed),u(s.state.empty),u(s.state.actorFacts),u(s.state.invalid));
    out.Add("\"last_witness_stage\":\"%s\",\"frame_metadata_matches\":%llu,\"current_source_callbacks\":%llu,\"actual_multicast_witnesses\":%llu,\"init_metadata\":{\"flags\":%u,\"params\":%u,\"bytes\":%u,\"return_offset\":%u,\"script_data\":\"%p\",\"script_count\":%d,\"script_capacity\":%d,\"exec_rva\":%llu},\"last\":",s.state.witnessStage,u(s.state.frameMatches),u(s.state.currentSources),u(s.state.multicasts),s.state.initFlags,unsigned(s.state.initParams),unsigned(s.state.initBytes),unsigned(s.state.initReturn),s.state.initScript,s.state.initScriptCount,s.state.initScriptCapacity,u(s.state.initExecRva));
    const auto& e=s.last;if(!e.valid)out.Add("null");else out.Add("{\"serial\":%llu,\"at_ms\":%llu,\"component\":\"%p\",\"selected_class\":\"%p\",\"owner_ability\":\"%p\",\"pawn\":\"%p\",\"epoch\":%u,\"registration\":%llu,\"native_result_count_known\":true,\"native_result_count\":%d,\"native_result_capacity\":%d,\"copy_count\":%d,\"parameter_count\":%d,\"delegate_index\":%llu,\"delegate_snapshot_data\":\"%p\",\"delegate_snapshot_count\":%d,\"delegate_snapshot_capacity\":%d,\"factory_returned_before_broadcast\":true,\"original_native_callback_returned\":%s,\"source_stable\":%s,\"array_stable\":%s}",
        u(e.serial),u(e.at),e.component,e.type,e.source.ability,e.source.pawn,e.source.sourceEpoch,u(e.source.registration),e.native.count,e.native.capacity,e.copy.count,e.params.count,u(e.delegateIndex),e.snapshotData,e.snapshotCount,e.snapshotCapacity,b(e.originalReturned),b(e.sourceStable),b(e.arrayStable));
    out.Add(",\"actors\":[");bool comma=false;for(const auto& a:s.actors)if(a.pointer){out.Add("%s{\"pointer\":\"%p\",\"facts_before\":%s,\"facts_after\":%s,\"weak\":[%d,%d]}",comma?",":"",a.pointer,b(a.factsBefore),b(a.factsAfter),a.receipt.actorId.index,a.receipt.actorId.serial);comma=true;}
    out.Add("],\"frame_code_reads\":0,\"frame_step_calls\":0,\"frame_writes\":0,\"function_flag_writes\":0,\"delegate_writes\":0,\"extra_throw_or_noise_calls\":0,\"actor_publications\":0,\"weak_serial_allocation_possible\":true,\"skill_or_client_delivery_verified\":false}");return out.value;
}
} // namespace local_server_phoenix_c_throw_result
