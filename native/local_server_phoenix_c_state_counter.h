#pragma once
#include "local_server_phoenix_c_native_observer.h"
#include "local_server_phoenix_c_state_counter_data.h"
#include "local_server_phoenix_c_state_counter_profiles.h"
#include "local_server_phoenix_c_state_counter_readback_data.h"
#include "local_server_phoenix_c_fire_gate_readback.h"

// Source-only candidate: private vptr of current actual Phoenix C throw-state
// instances only. No image/CDO/flags/resources/timers/Tick/Fire invocation.
namespace local_server_phoenix_c_state_counter {
namespace birth=local_server_phoenix_c_native_observer;
namespace data=local_server_phoenix_c_state_counter_data;
namespace source=local_server_phoenix_c_state_counter_source;
namespace raw=local_server_phoenix_c_state_counter_readback_data;
namespace gate=local_server_phoenix_c_fire_gate_readback;
using Weak=birth::Weak;using View=birth::View;
inline constexpr size_t MaxRecords=8;
enum class Kind:uint8_t {Enter,Exit,Fire};
struct Last {
    uint64_t sequence=0;Kind kind=Kind::Enter;void *context=nullptr;
    uint32_t firstBits=0,secondBits=0;uint8_t reason=0,before=0,after=0;
    bool qualified=false,originalReturned=false,sourceStableAfter=false;
    raw::Raw rawBefore{},rawAfter{};gate::Timer timerBefore{},timerAfter{};
};
struct Record {
    // Once published, this storage and the original forwarding pointers are
    // never cleared/reused, including after restore. The module stays resident.
    bool used=false,installed=false,retired=false;void* object=nullptr;Weak identity{};View view{};
    void** originalTable=nullptr;std::array<void*,data::TableEntries+1> storage{};
    data::Enter originalEnter=nullptr;data::Exit originalExit=nullptr;data::Fire originalFire=nullptr;
    alignas(8) volatile LONG64 entries[3]{},attempts[3]{},completed[3]{},bypassed[3]{};
    volatile LONG active=0;Last last{};std::array<Last,3> lastByKind{};
};
struct State {
    bool initialized=false,closed=false,closing=false,profiles=false;void* world=nullptr;Weak worldId{};DWORD thread=0;
    uint64_t polls=0,installs=0,restores=0,retirements=0,expired=0,foreign=0,capacity=0,sequence=0,sampleFaults=0;
    uint32_t lastException=0;const char* stage="phoenix_c_state_counter_waiting_actual_producer";
};
struct Row {
    bool used=false,installed=false,retired=false;void* object=nullptr;Weak identity{};View view{};
    const void *originalTable=nullptr,*privateTable=nullptr;
    uint64_t entries[3]{},attempts[3]{},completed[3]{},bypassed[3]{};LONG active=0;Last last{};std::array<Last,3> lastByKind{};
};
struct Snapshot {State state{};std::array<Row,MaxRecords> rows{};};
inline State state{};inline std::array<Record,MaxRecords> records{};inline Snapshot cached{};
inline SRWLOCK cacheLock=SRWLOCK_INIT;inline volatile LONG sampleBusy=0;
inline const State& GetState(){return state;}
inline uint32_t Bits(float value){uint32_t bits=0;std::memcpy(&bits,&value,4);return bits;}
inline bool Thread(){return state.thread&&state.thread==GetCurrentThreadId()&&birth::Thread();}
inline bool Current(const Record& r){
    if(!Thread()||!state.initialized||state.closed||state.closing||r.retired||!r.installed
       ||!birth::Live(r.object,r.identity)||birth::Read<void*>(r.object)!=r.storage.data()+1)return false;
    const auto p=birth::FindProducer(r.object);
    return p&&birth::Same(p->componentId,r.identity)&&birth::ProducerCurrent(*p)
        &&local_server_hero_authority::Fields(r.view,p->view)&&local_server_hero_authority::WeakFields(r.view,p->view);
}
inline void CacheImpl(void*){
    AcquireSRWLockExclusive(&cacheLock);
    __try{cached.state=state;for(size_t n=0;n<records.size();++n){const auto& r=records[n];auto& d=cached.rows[n];
        d.used=r.used;d.installed=r.installed;d.retired=r.retired;d.object=r.object;d.identity=r.identity;d.view=r.view;d.last=r.last;d.lastByKind=r.lastByKind;
        d.originalTable=r.originalTable;d.privateTable=r.storage.data()+1;
        d.active=InterlockedCompareExchange(const_cast<volatile LONG*>(&r.active),0,0);
        for(size_t k=0;k<3;++k){d.entries[k]=static_cast<uint64_t>(InterlockedCompareExchange64(const_cast<volatile LONG64*>(&r.entries[k]),0,0));
            d.attempts[k]=static_cast<uint64_t>(InterlockedCompareExchange64(const_cast<volatile LONG64*>(&r.attempts[k]),0,0));
            d.completed[k]=static_cast<uint64_t>(InterlockedCompareExchange64(const_cast<volatile LONG64*>(&r.completed[k]),0,0));
            d.bypassed[k]=static_cast<uint64_t>(InterlockedCompareExchange64(const_cast<volatile LONG64*>(&r.bypassed[k]),0,0));}}}
    __finally{ReleaseSRWLockExclusive(&cacheLock);}
}
inline void Cache(){data::SafeSample(&CacheImpl,nullptr,state.sampleFaults,state.lastException);}
struct Call {size_t index=0;Kind kind=Kind::Enter;void *object=nullptr,*context=nullptr;float first=0,second=0;uint8_t reason=0;Last last{};};
inline raw::Raw ReadRaw(const Record& r){
    auto out=raw::Capture(r.object,[](const void* p,size_t offset,auto& value){return original_players::Read(p,offset,value);},engine::image,r.view.world);
    out.ownerMatchesAbility=raw::Has(out,raw::Owner)&&out.owner==r.view.ability;
    out.ownerWeakMatches=out.ownerMatchesAbility&&birth::Live(out.owner,r.view.identities[6]);return out;
}
inline void Publish(Record& r,const Call& c){raw::Publish(r.last,r.lastByKind,static_cast<size_t>(c.kind),c.last);}
inline void Before(void* opaque){auto& c=*static_cast<Call*>(opaque);auto& r=records[c.index];
    if(c.object!=r.object||!Current(r)||state.sequence==UINT64_MAX)return;
    const auto sample=ReadRaw(r);if(!raw::Has(sample,raw::Thrown))return;
    c.last.sequence=++state.sequence;c.last.kind=c.kind;c.last.context=c.context;c.last.firstBits=Bits(c.first);c.last.secondBits=Bits(c.second);c.last.reason=c.reason;
    c.last.before=sample.thrown;c.last.rawBefore=sample;c.last.timerBefore=gate::ReadTimer(r.view,c.object);
    c.last.qualified=true;Publish(r,c);
}
inline void After(void* opaque){auto& c=*static_cast<Call*>(opaque);auto& r=records[c.index];
    if(!c.last.qualified)return;c.last.originalReturned=true;
    c.last.sourceStableAfter=c.object==r.object&&Current(r);
    if(c.last.sourceStableAfter){
        c.last.rawAfter=ReadRaw(r);c.last.timerAfter=gate::ReadTimer(r.view,c.object);
        c.last.sourceStableAfter=raw::Has(c.last.rawAfter,raw::Thrown)&&Current(r);
        if(c.last.sourceStableAfter)c.last.after=c.last.rawAfter.thrown;
    }Publish(r,c);
}

template<size_t N>inline void OriginalEnter(void* object,void* context,float first,float second){auto& r=records[N];
    InterlockedIncrement64(&r.attempts[0]);r.originalEnter(object,context,first,second);InterlockedIncrement64(&r.completed[0]);}
template<size_t N>inline void OriginalExit(void* object,void* context,float first,uint8_t reason){auto& r=records[N];
    InterlockedIncrement64(&r.attempts[1]);r.originalExit(object,context,first,reason);InterlockedIncrement64(&r.completed[1]);}
template<size_t N>inline void OriginalFire(void* object,float first,float second){auto& r=records[N];
    InterlockedIncrement64(&r.attempts[2]);r.originalFire(object,first,second);InterlockedIncrement64(&r.completed[2]);}
inline bool SampleThread(){return state.thread&&state.thread==GetCurrentThreadId();}
template<size_t N>inline void Enter(void* object,void* context,float first,float second){auto& r=records[N];
    InterlockedIncrement64(&r.entries[0]);InterlockedIncrement(&r.active);
    __try{if(!SampleThread()||InterlockedCompareExchange(&sampleBusy,1,0)!=0){InterlockedIncrement64(&r.bypassed[0]);OriginalEnter<N>(object,context,first,second);return;}
        Call c{N,Kind::Enter,object,context,first,second};
        __try{data::EnterOnce(&OriginalEnter<N>,object,context,first,second,&Before,&After,&c,state.sampleFaults,state.lastException);}
        __finally{InterlockedExchange(&sampleBusy,0);Cache();}}
    __finally{InterlockedDecrement(&r.active);}
}
template<size_t N>inline void Exit(void* object,void* context,float first,uint8_t reason){auto& r=records[N];
    InterlockedIncrement64(&r.entries[1]);InterlockedIncrement(&r.active);
    __try{if(!SampleThread()||InterlockedCompareExchange(&sampleBusy,1,0)!=0){InterlockedIncrement64(&r.bypassed[1]);OriginalExit<N>(object,context,first,reason);return;}
        Call c{N,Kind::Exit,object,context,first,0,reason};
        __try{data::ExitOnce(&OriginalExit<N>,object,context,first,reason,&Before,&After,&c,state.sampleFaults,state.lastException);}
        __finally{InterlockedExchange(&sampleBusy,0);Cache();}}
    __finally{InterlockedDecrement(&r.active);}
}
template<size_t N>inline void Fire(void* object,float first,float second){auto& r=records[N];
    InterlockedIncrement64(&r.entries[2]);InterlockedIncrement(&r.active);
    __try{if(!SampleThread()||InterlockedCompareExchange(&sampleBusy,1,0)!=0){InterlockedIncrement64(&r.bypassed[2]);OriginalFire<N>(object,first,second);return;}
        Call c{N,Kind::Fire,object,nullptr,first,second};
        __try{data::FireOnce(&OriginalFire<N>,object,first,second,&Before,&After,&c,state.sampleFaults,state.lastException);}
        __finally{InterlockedExchange(&sampleBusy,0);Cache();}}
    __finally{InterlockedDecrement(&r.active);}
}
struct Replacements {data::Enter enter;data::Exit exit;data::Fire fire;};
template<size_t... I>inline auto Make(std::index_sequence<I...>){return std::array<Replacements,sizeof...(I)>{{{&Enter<I>,&Exit<I>,&Fire<I>}...}};}
inline const auto replacements=Make(std::make_index_sequence<MaxRecords>{});
inline bool Profiles(){
    for(const auto& p:source::Profiles){const auto at=reinterpret_cast<void*>(engine::image+p.rva);
        if(!original_players::Readable(at,p.bytes)||birth::Hash(at,p.bytes)!=p.fnv)return false;}
    const auto table=reinterpret_cast<void**>(engine::image+source::VtableRva);
    if(!original_players::Readable(table-1,data::TableBytes+16)||reinterpret_cast<uintptr_t>(table[-1])!=engine::image+source::OpaquePriorRva)return false;
    for(size_t n=0;n<data::TableEntries;++n)if(reinterpret_cast<uintptr_t>(table[n])!=engine::image+source::Entries[n])return false;
    // The first noncode boundary is verified as data, never copied as a method.
    return reinterpret_cast<uintptr_t>(table[data::TableEntries])==engine::image+source::FirstNoncodeRva;
}
inline bool Restore(Record& r){
    if(!r.installed)return true;if(InterlockedCompareExchange(&r.active,0,0)!=0)return false;
    if(!birth::Live(r.object,r.identity)){r.installed=false;++state.expired;return true;}
    auto current=birth::Read<void**>(r.object);if(current==r.originalTable){r.installed=false;return true;}
    if(!data::RestoreAllowed(true,false,reinterpret_cast<uintptr_t>(current),reinterpret_cast<uintptr_t>(r.storage.data()+1))
       ||!birth::Writable(r.object)){++state.foreign;return false;}
    auto slot=reinterpret_cast<void* volatile*>(r.object);
    if(InterlockedCompareExchangePointer(slot,r.originalTable,r.storage.data()+1)!=r.storage.data()+1){++state.foreign;return false;}
    r.installed=false;++state.restores;return true;
}
inline void Install(const birth::Producer& p){
    if(!birth::ProducerCurrent(p)||!birth::PhoenixActualHuman(state.world)||p.view.heroIndex!=1||p.view.sourceEpoch==0)return;
    const auto actualClass=birth::Read<void*>(p.component,0x10);
    if(p.component==birth::Read<void*>(actualClass,0x120)||!birth::Writable(p.component)||(reinterpret_cast<uintptr_t>(p.component)&7U))return;
    for(const auto& r:records)if(r.used&&r.object==p.component&&birth::Same(r.identity,p.componentId)
        &&local_server_hero_authority::Fields(r.view,p.view))return;
    const auto expected=reinterpret_cast<void**>(engine::image+source::VtableRva);
    if(birth::Read<void**>(p.component)!=expected)return;
    for(size_t n=0;n<records.size();++n){auto& r=records[n];if(r.used)continue;
        r.used=true;r.object=p.component;r.identity=p.componentId;r.view=p.view;r.originalTable=expected;
        data::CopyTable(r.storage.data(),expected);
        r.originalEnter=reinterpret_cast<data::Enter>(expected[data::EnterIndex]);
        r.originalExit=reinterpret_cast<data::Exit>(expected[data::ExitIndex]);
        r.originalFire=reinterpret_cast<data::Fire>(expected[data::FireIndex]);
        r.storage[data::EnterIndex+1]=reinterpret_cast<void*>(replacements[n].enter);
        r.storage[data::ExitIndex+1]=reinterpret_cast<void*>(replacements[n].exit);
        r.storage[data::FireIndex+1]=reinterpret_cast<void*>(replacements[n].fire);
        // All immutable forward/storage data is published before the instance CAS.
        auto slot=reinterpret_cast<void* volatile*>(r.object);
        if(InterlockedCompareExchangePointer(slot,r.storage.data()+1,expected)!=expected){r.retired=true;++state.foreign;return;}
        r.installed=true;++state.installs;return;
    }++state.capacity;
}
inline void PollImpl(void* pawn,uint32_t epoch){
    if(state.closed||state.closing||!birth::GetState().initialized||birth::GetState().closed||!birth::Thread())return;
    if(!state.initialized){
        if(!birth::PhoenixActualHuman(birth::GetState().world))return;
        state.world=birth::GetState().world;state.worldId=birth::GetState().worldId;state.thread=GetCurrentThreadId();
        if(!Profiles()){state.stage="phoenix_c_state_counter_source_profile_mismatch";return;}
        state.profiles=state.initialized=true;
    }
    if(!Thread()||InterlockedCompareExchange(&sampleBusy,0,0)!=0||!birth::Live(state.world,state.worldId))return;++state.polls;
    for(auto& r:records)if(r.used&&r.installed&&!Current(r)){r.retired=true;Restore(r);}
    for(const auto& p:birth::producers)if(p.used&&!p.retired&&p.view.pawn==pawn&&p.view.sourceEpoch==epoch)Install(p);
    state.stage="phoenix_c_state_counter_observation_only_no_runtime_claim";
}
inline void PollQuery(void* opaque){const auto q=static_cast<std::pair<void*,uint32_t>*>(opaque);PollImpl(q->first,q->second);}
inline void Poll(void* pawn,uint32_t epoch){std::pair<void*,uint32_t> q{pawn,epoch};data::SafeSample(&PollQuery,&q,state.sampleFaults,state.lastException);Cache();}
inline void RetireImpl(void* pawn,uint32_t epoch){if(!Thread())return;
    for(auto& r:records)if(r.used&&!r.retired&&r.view.pawn==pawn&&r.view.sourceEpoch==epoch){r.retired=true;++state.retirements;Restore(r);}Cache();}
inline void RetireQuery(void* opaque){const auto q=static_cast<std::pair<void*,uint32_t>*>(opaque);RetireImpl(q->first,q->second);}
inline void Retire(void* pawn,uint32_t epoch){std::pair<void*,uint32_t> q{pawn,epoch};data::SafeSample(&RetireQuery,&q,state.sampleFaults,state.lastException);Cache();}
inline bool CloseImpl(){
    if(!state.thread){state.closed=true;return true;}if(!Thread())return false;
    state.closing=true;bool okay=InterlockedCompareExchange(&sampleBusy,0,0)==0;
    for(auto& r:records)if(r.used){r.retired=true;if(!Restore(r))okay=false;}
    state.closed=okay;state.stage=okay?"phoenix_c_state_counter_original_instance_vptrs_restored":"phoenix_c_state_counter_waiting_owned_restore";return okay;
}
inline void CloseQuery(void* opaque){*static_cast<bool*>(opaque)=CloseImpl();}
inline bool Close(){bool okay=false;data::SafeSample(&CloseQuery,&okay,state.sampleFaults,state.lastException);Cache();return okay;}
inline void WriteChain(local_server_match::Json& out,const local_server_phoenix_c_fast_class_data::Chain& c){
    const auto b=[](bool v){return v?"true":"false";};out.Add("{\"known\":%s,\"matches\":",b(c.known));if(c.known)out.Add("%s",b(c.matches));else out.Add("null");
    out.Add(",\"stage\":%u,\"stable\":%s,\"singleton_read\":%s,\"candidate_read\":%s,\"singleton\":\"%p\",\"candidate_class\":\"%p\",\"expected_depth\":%d,\"candidate_depth\":%d,\"chain_array\":\"%p\",\"actual_chain_entry\":\"%p\"}",unsigned(c.stage),b(c.stable),b(c.singletonRead),b(c.candidateRead),c.singleton,c.candidate,c.depth,c.candidateDepth,c.array,c.entry);
}
inline void WriteFastClass(local_server_match::Json& out,const raw::Raw& r){
    const auto b=[](bool v){return v?"true":"false";};const auto& f=r.fast;out.Add("{\"component_object_flags_known\":%s,\"component_object_flags\":",b(r.componentFlagsKnown));if(r.componentFlagsKnown)out.Add("%u",r.componentFlags);else out.Add("null");
    out.Add(",\"component_object_flags_60000000_clear\":");if(r.componentFlagsKnown)out.Add("%s",b((r.componentFlags&0x60000000U)==0));else out.Add("null");
    out.Add(",\"owner_is_ares_equippable\":");WriteChain(out,f.ownerAres);out.Add(",\"raw_310_is_projectile_class\":");WriteChain(out,f.rawProjectile);
    out.Add(",\"selected_class_known_via_tag_zero\":%s,\"selected_class\":\"%p\",\"selected_is_projectile_class\":",b(f.selectedClassKnown),f.selectedClass);WriteChain(out,f.selectedProjectile);
    const bool expressionKnown=raw::Has(r,raw::Thrown)&&raw::Has(r,raw::Owner)&&raw::Has(r,raw::OwnerFlags)&&f.ownerAres.known&&f.selectedClassKnown&&f.selectedProjectile.known;
    out.Add(",\"can_throw_expression_at_sample_known\":%s,\"can_throw_expression_at_sample_allows\":",b(expressionKnown));if(expressionKnown)out.Add("%s",b(r.thrown==0&&r.owner&&f.ownerAres.matches&&(r.ownerFlags&0x60000000U)==0&&f.selectedClass&&f.selectedProjectile.matches));else out.Add("null");
    out.Add(",\"registered_world\":\"%p\",\"registered_world_driver_known\":%s,\"registered_world_driver_40\":\"%p\",\"world_helper_fast_path_known\":%s,\"world_helper_fast_path_native_mode\":",f.world,b(f.worldDriverKnown),f.worldDriver,b(f.worldModeKnown));if(f.worldModeKnown)out.Add("%d",f.worldMode);else out.Add("null");
    out.Add(",\"fast_class_helper_get_world_or_get_net_mode_calls\":0,\"lazy_class_getter_calls\":0}");
}
inline void WriteRaw(local_server_match::Json& out,const raw::Raw& r){
    const auto b=[](bool value){return value?"true":"false";};const auto u=[](uint64_t value){return static_cast<unsigned long long>(value);};
    out.Add("{\"sampled\":%s,\"known_mask\":%u,\"component_3c0\":",b(r.sampled),r.known);
    if(raw::Has(r,raw::Thrown))out.Add("%u",unsigned(r.thrown));else out.Add("null");out.Add(",\"delay_368_float_bits\":");
    if(raw::Has(r,raw::Delay))out.Add("%u",r.delayBits);else out.Add("null");out.Add(",\"timer_handle_3b8\":");
    if(raw::Has(r,raw::Handle))out.Add("%llu",u(r.handle));else out.Add("null");out.Add(",\"raw_class_310\":");
    if(raw::Has(r,raw::RawClass))out.Add("\"%p\"",r.rawClass);else out.Add("null");out.Add(",\"tag_318_first8\":");
    if(raw::Has(r,raw::TagFirst8))out.Add("%llu",u(r.tagFirst8));else out.Add("null");out.Add(",\"actual_owner_c8\":");
    if(raw::Has(r,raw::Owner))out.Add("\"%p\"",r.owner);else out.Add("null");out.Add(",\"owner_class\":");
    if(raw::Has(r,raw::OwnerClass))out.Add("\"%p\"",r.ownerClass);else out.Add("null");out.Add(",\"owner_object_flags\":");
    if(raw::Has(r,raw::OwnerFlags))out.Add("%u",r.ownerFlags);else out.Add("null");out.Add(",\"owner_flags_60000000_clear\":");
    if(raw::Has(r,raw::OwnerFlags))out.Add("%s",b((r.ownerFlags&0x60000000U)==0));else out.Add("null");
    out.Add(",\"owner_is_actual_ability\":");if(raw::Has(r,raw::Owner))out.Add("%s",b(r.ownerMatchesAbility));else out.Add("null");
    out.Add(",\"owner_weak_matches_source\":");if(raw::Has(r,raw::Owner))out.Add("%s",b(r.ownerWeakMatches));else out.Add("null");out.Add(",\"fast_class\":");WriteFastClass(out,r);out.Add("}");
}
inline void WriteLast(local_server_match::Json& out,const Last& r,size_t kind){
    const auto b=[](bool value){return value?"true":"false";};
    out.Add("{\"kind\":%u,\"sequence\":%llu,\"context\":\"%p\",\"qualified\":%s,\"original_returned\":%s,\"source_stable_after\":%s,\"first_float_bits\":%u,\"second_float_bits\":%u,\"exit_reason\":%u,\"before\":",
        unsigned(kind),static_cast<unsigned long long>(r.sequence),r.context,b(r.qualified),b(r.originalReturned),b(r.sourceStableAfter),r.firstBits,r.secondBits,unsigned(r.reason));
    WriteRaw(out,r.rawBefore);out.Add(",\"after\":");WriteRaw(out,r.rawAfter);out.Add(",\"timer_before\":");gate::WriteTimer(out,r.timerBefore);
    out.Add(",\"timer_after\":");gate::WriteTimer(out,r.timerAfter);out.Add("}");
}
inline std::string Json(){Snapshot s{};AcquireSRWLockShared(&cacheLock);s=cached;ReleaseSRWLockShared(&cacheLock);
    local_server_match::Json out;const auto b=[](bool value){return value?"true":"false";};const auto u=[](uint64_t value){return static_cast<unsigned long long>(value);};
    out.Add("{\"stage\":\"%s\",\"initialized\":%s,\"closed\":%s,\"profiles_verified\":%s,\"instance_vptr_install_writes\":%llu,\"instance_vptr_restore_writes\":%llu,\"sample_faults\":%llu,\"last_sample_exception\":%u,\"capacity_misses\":%llu,\"records\":[",
        s.state.stage,b(s.state.initialized),b(s.state.closed),b(s.state.profiles),u(s.state.installs),u(s.state.restores),u(s.state.sampleFaults),s.state.lastException,u(s.state.capacity));
    bool comma=false;for(const auto& r:s.rows)if(r.used){out.Add("%s{\"component\":\"%p\",\"weak\":[%d,%d],\"world\":\"%p\",\"pawn\":\"%p\",\"ability\":\"%p\",\"original_vtable\":\"%p\",\"private_vtable\":\"%p\",\"epoch\":%u,\"registration\":%llu,\"installed\":%s,\"retired\":%s,\"active\":%ld,\"onenter\":[%llu,%llu,%llu],\"onexit\":[%llu,%llu,%llu],\"v558\":[%llu,%llu,%llu],\"last_kind\":%u,\"last_context\":\"%p\",\"last_qualified\":%s,\"last_original_returned\":%s,\"last_source_stable_after\":%s,\"before_3c0\":%u,\"after_3c0\":%u,\"first_float_bits\":%u,\"second_float_bits\":%u,\"exit_reason\":%u,\"last_by_kind\":[",
        comma?",":"",r.object,r.identity.index,r.identity.serial,r.view.world,r.view.pawn,r.view.ability,r.originalTable,r.privateTable,r.view.sourceEpoch,u(r.view.registration),b(r.installed),b(r.retired),r.active,
        u(r.entries[0]),u(r.attempts[0]),u(r.completed[0]),u(r.entries[1]),u(r.attempts[1]),u(r.completed[1]),u(r.entries[2]),u(r.attempts[2]),u(r.completed[2]),unsigned(r.last.kind),r.last.context,b(r.last.qualified),b(r.last.originalReturned),b(r.last.sourceStableAfter),unsigned(r.last.before),unsigned(r.last.after),r.last.firstBits,r.last.secondBits,unsigned(r.last.reason));
        for(size_t kind=0;kind<r.lastByKind.size();++kind){if(kind)out.Add(",");WriteLast(out,r.lastByKind[kind],kind);}out.Add("]}");comma=true;}
    out.Add("],\"last_by_kind_order\":[\"onenter\",\"onexit\",\"v558\"],\"raw_310_is_selected_class\":false,\"gate_or_timer_api_calls\":0,\"method_counter_order\":[\"entries\",\"original_attempts\",\"original_completed\"],\"v4f8_is_tick\":false,\"v4e0_hooked\":false,\"manual_tick_calls\":0,\"manual_fire_calls\":0,\"replayed_state_calls\":0,\"image_or_cdo_writes\":0,\"flag_resource_role_writes\":0,\"v558_zero_does_not_exclude_direct_713a6e0\":true,\"actual_do_throw_result_verified\":false,\"skill_or_client_verified\":false,\"resident_storage_retained\":true}");return out.value;
}
} // namespace local_server_phoenix_c_state_counter
