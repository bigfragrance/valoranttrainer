#pragma once
#include "local_server_phoenix_c_native_observer.h"
#include "local_server_phoenix_c_fire_gate_data.h"
#include "local_server_phoenix_c_fire_gate_profiles.h"

// Current owned Phoenix C metadata/remaining-time readback only. No timer API,
// gate invocation, input, factory, state replay or gameplay setter is called.
namespace local_server_phoenix_c_fire_gate_readback {
namespace birth=local_server_phoenix_c_native_observer;namespace pure=local_server_phoenix_c_fire_gate_data;
using View=birth::View;using Weak=birth::Weak;
struct Timer {
    bool known=false,active=false,lookupKnown=false,internalKnown=false,remainingKnown=false,positive=false,sourceStable=false;
    void *world=nullptr,*gi=nullptr,*manager=nullptr,*slot=nullptr;uint64_t handle=0,slotHandle=0;
    int32_t count=0;uint32_t index=0,occupancy=0;uint8_t kind=255;double expiry=0,internal=0;float remaining=0;
    const char* stage="timer_not_sampled";
};
struct Delegate {
    bool used=false,targetResolved=false,functionResolved=false,stable=false,metadataKnown=false;
    bool targetIsAbility=false,targetIsComponent=false,native=false,execInImage=false;
    void *component=nullptr,*target=nullptr,*targetType=nullptr,*function=nullptr,*owner=nullptr,*dispatch=nullptr;
    Weak targetId{},functionId{},ownerId{};uint32_t name[3]{},flags=0,targetFlags=0;uint16_t bytes=0;uint8_t params=0;
    int32_t scriptCount=-1,scriptCapacity=-1,delegateIndex=-1;uintptr_t execRva=0,dispatchExecRva=0,targetEventRva=0;uint32_t epoch=0;uint64_t registration=0;
    char targetName[96]{},targetClass[96]{},functionName[96]{},functionOwner[96]{};bool labelsKnown=false;
};
struct State {bool initialized=false,closed=false,profiles=false;DWORD thread=0;void* world=nullptr;Weak worldId{};
    bool truncated=false;uint64_t polls=0,lastPoll=0,faults=0,captures=0,unknownArrays=0;uint32_t lastException=0;const char* stage="phoenix_fire_gates_not_initialized";};
struct Snapshot {State state{};std::array<Delegate,32> delegates{};};
inline State state{};inline std::array<Delegate,32> delegates{};inline Snapshot cached{};inline SRWLOCK lock=SRWLOCK_INIT;
template<class T>inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool GT(){return state.initialized&&!state.closed&&state.thread==GetCurrentThreadId()&&birth::Thread();}
inline bool Live(void* p,const Weak& id){return birth::Live(p,id);}
inline bool Capture(void* p,Weak& id){++state.captures;return birth::Capture(p,id);}
inline const State& GetState(){return state;}
inline void Cache(){AcquireSRWLockExclusive(&lock);cached={state,delegates};ReleaseSRWLockExclusive(&lock);}
inline bool Profiles(){for(const auto& p:local_server_phoenix_c_fire_gate_source::Profiles){const auto code=reinterpret_cast<void*>(engine::image+p.rva);
    if(!original_players::Readable(code,p.bytes)||birth::Hash(code,p.bytes)!=p.fnv)return false;}return true;}
inline bool InitImpl(void* world){
    if(!birth::PhoenixActualHuman(world)||!birth::GetState().initialized||!birth::GetState().profiles||!birth::GetState().schema)return false;
    if(state.initialized)return !state.closed&&state.world==world&&GT();state.world=world;state.worldId=birth::GetState().worldId;state.thread=GetCurrentThreadId();
    if(!Profiles()){state.stage="phoenix_fire_gates_original_source_unknown";return false;}state.profiles=state.initialized=true;state.stage="phoenix_fire_gates_current_metadata_only";return true;
}
struct Init {void* world=nullptr;bool okay=false;};inline void InitQuery(void* p){auto& c=*static_cast<Init*>(p);c.okay=InitImpl(c.world);}
inline bool InitHost(void* world){Init c{world,false};local_server_phoenix_c_birth_data::SafeSample(&InitQuery,&c,state.faults,state.lastException);Cache();return c.okay;}
inline Timer ReadTimer(const View& view,void* component){
    Timer out{};if(!GT()||!state.profiles||!birth::Current(view)||view.world!=state.world)return out;
    const auto producer=birth::FindProducer(component);if(!producer||!birth::ProducerCurrent(*producer)
        ||!local_server_hero_authority::Fields(view,producer->view)||!local_server_hero_authority::WeakFields(view,producer->view))return out;
    // The original component GetWorld route must be the pinned native getter.
    if(Read<uintptr_t>(Read<void*>(component),0x1b8)!=engine::image+0x2a64ee0
       ||engine::function<void*(*)(void*)>(0x2a64ee0)(component)!=view.world)return out;
    out.world=view.world;out.gi=Read<void*>(out.world,0x1d8);out.manager=out.gi?Read<void*>(out.gi,0xf8):Read<void*>(out.world,0x478);
    out.handle=Read<uint64_t>(component,0x3b8);if(!out.manager||!original_players::Readable(out.manager,0x148)){out.stage="timer_manager_unreadable";return out;}
    out.count=Read<int32_t>(out.manager,0x10);if(out.count<0||out.count>1048576){out.stage="timer_count_unknown";return out;}
    out.internal=Read<double>(out.manager,0x140);out.internalKnown=std::isfinite(out.internal);
    bool found=false;void* bitPointer=nullptr;void* dataPointer=nullptr;void* allocationBits=nullptr;
    if(pure::HandleIndex(out.handle,out.count,out.index)){
        const auto bits=Read<void*>(out.manager,0x28);const auto bitBase=bits?static_cast<uint8_t*>(bits):static_cast<uint8_t*>(out.manager)+0x18;
        const auto bitWord=bitBase+static_cast<size_t>(out.index>>5)*4;
        if(!original_players::Readable(bitWord,4)){out.stage="timer_occupancy_unreadable";return out;}
        out.occupancy=Read<uint32_t>(bitWord);bitPointer=bitWord;allocationBits=bits;
        if(out.occupancy&(1U<<(out.index&31))){
            const auto data=Read<void*>(out.manager,8);dataPointer=data;if(!data){out.stage="timer_data_unreadable";return out;}
            out.slot=static_cast<uint8_t*>(data)+static_cast<size_t>(out.index)*0xb0;
            if(!original_players::Readable(out.slot,0x88)){out.stage="timer_slot_unreadable";return out;}
            out.slotHandle=Read<uint64_t>(out.slot,0x80);out.kind=Read<uint8_t>(out.slot,1);out.expiry=Read<double>(out.slot,8);
            found=out.slotHandle==out.handle&&out.kind!=4;
        }
    }
    out.lookupKnown=true;const auto value=pure::Remaining(found,out.kind,out.expiry,out.internal);out.active=found;out.remainingKnown=value.known;
    out.remaining=value.remaining;out.positive=value.positive;
    out.sourceStable=birth::ProducerCurrent(*producer)&&birth::Current(view)&&Read<uint64_t>(component,0x3b8)==out.handle
        &&Read<void*>(out.world,0x1d8)==out.gi&&(out.gi?Read<void*>(out.gi,0xf8):Read<void*>(out.world,0x478))==out.manager
        &&Read<int32_t>(out.manager,0x10)==out.count&&Read<double>(out.manager,0x140)==out.internal
        &&(!bitPointer||(Read<void*>(out.manager,0x28)==allocationBits&&Read<uint32_t>(bitPointer)==out.occupancy))
        &&(!out.slot||(Read<void*>(out.manager,8)==dataPointer&&Read<uint64_t>(out.slot,0x80)==out.slotHandle
            &&Read<uint8_t>(out.slot,1)==out.kind&&Read<double>(out.slot,8)==out.expiry));
    out.known=out.sourceStable&&out.remainingKnown;out.stage=out.known?"original_timer_remaining_expression_read":"original_timer_value_unknown";return out;
}
inline void ReadDelegates(const birth::Producer& producer,size_t& used){
    if(!birth::ProducerCurrent(producer))return;const auto array=Read<birth::Array>(producer.component,0x300);
    if(array.count<0||array.count>64||array.capacity<array.count||array.capacity>65536
       ||(array.count&&!original_players::Readable(array.data,static_cast<size_t>(array.count)*20))){++state.unknownArrays;return;}
    if(static_cast<size_t>(array.count)>delegates.size()-used)state.truncated=true;
    for(int32_t n=0;n<array.count&&used<delegates.size();++n){auto& out=delegates[used++];out={};out.used=true;out.component=producer.component;out.delegateIndex=n;
        const auto entry=reinterpret_cast<uint8_t*>(array.data)+static_cast<size_t>(n)*20;out.targetId=Read<Weak>(entry);
        std::memcpy(out.name,entry+8,sizeof(out.name));out.epoch=producer.view.sourceEpoch;out.registration=producer.view.registration;
        out.target=original_inventory_slots::ResolveIdentity(out.targetId);out.targetResolved=out.target&&Live(out.target,out.targetId);if(!out.targetResolved)continue;
        out.targetType=Read<void*>(out.target,0x10);out.targetFlags=Read<uint32_t>(out.target,8);
        const auto targetEvent=Read<uintptr_t>(Read<void*>(out.target),0x298);out.targetEventRva=targetEvent>=engine::image&&targetEvent<engine::image+local_server_phoenix_c_fire_gate_source::ImageBytes?targetEvent-engine::image:0;out.targetIsAbility=out.target==producer.view.ability;out.targetIsComponent=out.target==producer.component;
        out.function=engine::function<void*(*)(void*,const void*)>(0x1b9ab40)(out.targetType,out.name);if(!out.function||!Capture(out.function,out.functionId))continue;
        out.functionResolved=true;out.owner=Read<void*>(out.function,0x28);if(!Capture(out.owner,out.ownerId))continue;
        out.flags=Read<uint32_t>(out.function,0xb8);out.native=(out.flags&0x400U)!=0;out.params=Read<uint8_t>(out.function,0xbc);out.bytes=Read<uint16_t>(out.function,0xbe);
        out.scriptCount=Read<int32_t>(out.function,0x70);out.scriptCapacity=Read<int32_t>(out.function,0x74);const auto exec=Read<uintptr_t>(out.function,0xe0);
        out.execInImage=exec>=engine::image&&exec<engine::image+local_server_phoenix_c_fire_gate_source::ImageBytes;out.execRva=out.execInImage?exec-engine::image:0;out.dispatch=Read<void*>(out.function,0xd0);
        if(out.dispatch){const auto d=Read<uintptr_t>(out.dispatch,0xe0);out.dispatchExecRva=d>=engine::image&&d<engine::image+local_server_phoenix_c_fire_gate_source::ImageBytes?d-engine::image:0;}
        out.metadataKnown=out.scriptCount>=0&&out.scriptCapacity>=out.scriptCount&&out.scriptCapacity<=1048576&&out.bytes<=4096;
        if(local_server_hero_skill_readback::GetState().profiles){
            const auto t=local_server_hero_skill_readback::Label(out.target,&out.targetId),f=local_server_hero_skill_readback::Label(out.function,&out.functionId),o=local_server_hero_skill_readback::Label(out.owner,&out.ownerId);
            std::memcpy(out.targetName,t.name,sizeof(out.targetName));std::memcpy(out.targetClass,t.className,sizeof(out.targetClass));
            std::memcpy(out.functionName,f.name,sizeof(out.functionName));std::memcpy(out.functionOwner,o.name,sizeof(out.functionOwner));out.labelsKnown=t.nameKnown&&t.classNameKnown&&f.nameKnown&&o.nameKnown;
        }
        const auto after=Read<birth::Array>(producer.component,0x300);
        out.stable=birth::ProducerCurrent(producer)&&Live(out.target,out.targetId)&&Live(out.function,out.functionId)&&Live(out.owner,out.ownerId)
            &&Read<void*>(out.function,0x28)==out.owner&&Read<void*>(out.target,0x10)==out.targetType&&Read<uintptr_t>(out.function,0xe0)==exec
            &&Read<uint32_t>(out.target,8)==out.targetFlags&&Read<uintptr_t>(Read<void*>(out.target),0x298)==targetEvent
            &&after.data==array.data&&after.count==array.count&&after.capacity==array.capacity
            &&std::memcmp(entry,&out.targetId,sizeof(Weak))==0&&std::memcmp(entry+8,out.name,sizeof(out.name))==0;
    }
}
struct PollArgs {void* pawn=nullptr;uint32_t epoch=0;};
inline void PollQuery(void* p){const auto& q=*static_cast<PollArgs*>(p);if(!GT()||!state.profiles)return;const auto now=GetTickCount64();if(state.lastPoll&&now>=state.lastPoll&&now-state.lastPoll<100)return;
    state.lastPoll=now;++state.polls;state.truncated=false;delegates={};size_t used=0;for(const auto& producer:birth::producers)
        if(producer.used&&!producer.retired&&producer.view.pawn==q.pawn&&producer.view.sourceEpoch==q.epoch)ReadDelegates(producer,used);
}
inline void Poll(void* pawn,uint32_t epoch){PollArgs q{pawn,epoch};local_server_phoenix_c_birth_data::SafeSample(&PollQuery,&q,state.faults,state.lastException);Cache();}
inline void Close(){state.closed=true;Cache();}
inline void WriteTimer(local_server_match::Json& out,const Timer& r){const auto b=[](bool v){return v?"true":"false";};out.Add("{\"stage\":\"%s\",\"known\":%s,\"lookup_known\":%s,\"active_slot\":",r.stage,b(r.known),b(r.lookupKnown&&r.sourceStable));
    if(r.lookupKnown&&r.sourceStable)out.Add("%s",b(r.active));else out.Add("null");out.Add(",\"remaining_known\":%s,\"positive_remaining\":",b(r.remainingKnown&&r.sourceStable));
    if(r.known)out.Add("%s",b(r.positive));else out.Add("null");out.Add(",\"world\":\"%p\",\"game_instance\":\"%p\",\"manager\":\"%p\",\"slot\":\"%p\",\"handle\":%llu,\"slot_handle\":%llu,\"count\":%d,\"index\":%u,\"occupancy_word\":%u,\"kind\":%u,\"remaining\":",r.world,r.gi,r.manager,r.slot,static_cast<unsigned long long>(r.handle),static_cast<unsigned long long>(r.slotHandle),r.count,r.index,r.occupancy,unsigned(r.kind));
    if(r.known)out.Add("%.9g",double(r.remaining));else out.Add("null");out.Add(",\"expiry\":");if(r.lookupKnown&&r.sourceStable&&r.active&&std::isfinite(r.expiry))out.Add("%.17g",r.expiry);else out.Add("null");out.Add(",\"internal\":");if(r.internalKnown&&r.sourceStable)out.Add("%.17g",r.internal);else out.Add("null");out.Add(",\"source_stable\":%s}",b(r.sourceStable));
}
inline std::string Json(){Snapshot s{};AcquireSRWLockShared(&lock);s=cached;ReleaseSRWLockShared(&lock);local_server_match::Json out;const auto b=[](bool v){return v?"true":"false";};
    out.Add("{\"stage\":\"%s\",\"initialized\":%s,\"closed\":%s,\"profiles_verified\":%s,\"sample_faults\":%llu,\"last_exception\":%u,\"polls\":%llu,\"weak_capture_calls\":%llu,\"truncated\":%s,\"unknown_delegate_arrays\":%llu,\"delegates\":[",s.state.stage,b(s.state.initialized),b(s.state.closed),b(s.state.profiles),static_cast<unsigned long long>(s.state.faults),s.state.lastException,static_cast<unsigned long long>(s.state.polls),static_cast<unsigned long long>(s.state.captures),b(s.state.truncated),static_cast<unsigned long long>(s.state.unknownArrays));
    bool comma=false;for(const auto& r:s.delegates)if(r.used){out.Add("%s{\"component\":\"%p\",\"target\":\"%p\",\"target_class\":\"%p\",\"target_resolved\":%s,\"function_resolved\":%s,\"target_is_ability\":%s,\"target_is_component\":%s,\"metadata_known\":%s,\"stable\":%s,\"fname\":[%u,%u,%u],\"function\":\"%p\",\"function_owner\":\"%p\",\"flags\":%u,\"target_object_flags\":%u,\"target_process_event_rva\":%llu,\"delegate_index\":%d,\"native\":%s,\"params_count\":%u,\"params_bytes\":%u,\"script_count\":%d,\"script_capacity\":%d,\"exec_in_fixed_image\":%s,\"exec_rva\":%llu,\"dispatch\":\"%p\",\"dispatch_exec_rva\":%llu,\"epoch\":%u,\"registration\":%llu,\"labels_known\":%s,\"target_name\":",
        comma?",":"",r.component,r.target,r.targetType,b(r.targetResolved),b(r.functionResolved),r.stable?b(r.targetIsAbility):"null",r.stable?b(r.targetIsComponent):"null",b(r.metadataKnown),b(r.stable),r.name[0],r.name[1],r.name[2],r.function,r.owner,r.flags,r.targetFlags,static_cast<unsigned long long>(r.targetEventRva),r.delegateIndex,r.metadataKnown&&r.stable?b(r.native):"null",unsigned(r.params),unsigned(r.bytes),r.scriptCount,r.scriptCapacity,r.metadataKnown&&r.stable?b(r.execInImage):"null",static_cast<unsigned long long>(r.execRva),r.dispatch,static_cast<unsigned long long>(r.dispatchExecRva),r.epoch,static_cast<unsigned long long>(r.registration),b(r.labelsKnown));
        out.String(r.targetName);out.Add(",\"target_class_name\":");out.String(r.targetClass);out.Add(",\"function_name\":");out.String(r.functionName);out.Add(",\"function_owner_name\":");out.String(r.functionOwner);out.Add("}");comma=true;}
    out.Add("],\"delegate_invocation_or_gameplay_verified\":false,\"new_hooks\":0,\"timer_or_throw_calls\":0,\"gameplay_field_writes\":0,\"weak_serial_allocation_possible\":true}");return out.value;
}
} // namespace local_server_phoenix_c_fire_gate_readback
