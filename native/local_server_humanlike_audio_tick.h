#pragma once
#include "local_server_humanlike_audio_requests.h"
#include "humanlike_audio_tick_core.h"
#include "humanlike_audio_tick_profiles.h"
#include <array>
#include <cstring>
#include <cstdint>
#include <string>

// Per-instance observer of the original FootstepsComponent TickComponent.
// No image patch, synthetic Tick, speed-to-sound rule or audio parameter write.
namespace local_server_humanlike_audio_tick {
namespace source=native_tick_source_profiles;
namespace request=local_server_humanlike_audio_requests;
namespace core=humanlike_audio_tick_core;
namespace observation=humanlike_audio_observation;
namespace events=humanlike_bot_native_events;
using Weak=original_inventory_slots::WeakIdentity;
using Tick=void(*)(void*,float,int32_t,void*);
inline constexpr size_t MaxBackings=4096,MaxPlayers=local_server_match::MaxPlayers,ReceiptCapacity=128;
static_assert(source::TickSlot/8<source::TableEntries);
struct Backing {
    void*component=nullptr,*pawn=nullptr,*world=nullptr,*asset=nullptr,*componentClass=nullptr;
    Weak componentId{},pawnId{},worldId{},assetId{};bool assetReady=false;
    size_t index=0;uint32_t epoch=0;void*allocation=nullptr;void**table=nullptr;
    bool published=false,retired=false,busy=false,nested=false,restored=false;
};
struct State {
    bool initialized=false,closed=false,closing=false,profilesVerified=false;
    DWORD gameThread=0;void*world=nullptr;Weak worldId{};
    unsigned error=0;const char*stage="humanlike_native_audio_tick_not_initialized";
    uint64_t allocations=0,retainedBytes=0,binds=0,retirements=0,expiredRetained=0,restores=0,conflicts=0,capacityRejected=0;
    uint64_t samples=0,stampChanges=0,provenRequests=0,published=0,invalidSources=0,unchangedStamps=0,unknownStamps=0,timeMismatches=0;
    uint64_t samplingFaults=0,assetUnavailable=0,originUnavailable=0,worldRejected=0,lifeRejected=0;uint32_t lastSamplingException=0;
};
inline State state{};
inline Tick original=nullptr; // Immutable before the first publication, retained after Close.
inline std::array<Backing,MaxBackings> backings{};
inline std::array<Backing*,MaxBackings> ordered{};
inline std::array<Backing*,MaxPlayers> bindings{};
inline size_t used=0;
inline std::array<core::Receipt,ReceiptCapacity> receipts{};
inline volatile LONG64 entries=0,forwardAttempts=0,forwardCompleted=0,offThread=0,reentrant=0,activeCalls=0;
struct Snapshot {
    State state{};std::array<core::Receipt,ReceiptCapacity> receipts{};size_t retainedBackings=0,activeBindings=0;
    uint64_t entries=0,attempts=0,completed=0,offThread=0,reentrant=0,activeCalls=0;
};
inline Snapshot cached{};inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline const State& GetState(){return state;}
template<class T>inline T Read(const void*object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void*object,const Weak&id){return object&&original_inventory_slots::ResolveIdentity(id)==object;}
inline uint64_t Count(volatile LONG64&value){return static_cast<uint64_t>(InterlockedCompareExchange64(&value,0,0));}
inline bool Thread(){return state.gameThread==GetCurrentThreadId()&&original_players::Thread();}
inline void Cache(){
    AcquireSRWLockExclusive(&cacheLock);cached.state=state;cached.receipts=receipts;cached.retainedBackings=used;cached.activeBindings=0;
    for(const auto b:bindings)if(b&&!b->retired)++cached.activeBindings;
    cached.entries=Count(entries);cached.attempts=Count(forwardAttempts);cached.completed=Count(forwardCompleted);
    cached.offThread=Count(offThread);cached.reentrant=Count(reentrant);cached.activeCalls=Count(activeCalls);ReleaseSRWLockExclusive(&cacheLock);
}
inline bool Fail(unsigned error,const char*stage){state.error=error;state.stage=stage;Cache();return false;}
inline bool Context(){return Thread()&&state.initialized&&!state.closed&&!state.closing&&!state.error&&request::Context()
    &&state.world==request::GetState().world&&Live(state.world,state.worldId);}
inline void** Source(){return reinterpret_cast<void**>(engine::image+source::TableRva);}
inline bool ProfilesMatch(){
    for(const auto&p:source::Bodies){auto bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes))return false;uint64_t hash=14695981039346656037ULL;
        for(size_t n=0;n<p.bytes;++n)hash=(hash^bytes[n])*1099511628211ULL;if(hash!=p.fnv)return false;}
    const auto table=Source();if(!original_players::Readable(table-1,(source::TableEntries+1)*sizeof(void*)))return false;
    if(reinterpret_cast<uintptr_t>(Read<void*>(table-1))!=engine::image+0x63c1d70)return false;
    for(size_t n=0;n<source::TableEntries;++n)if(Read<void*>(table,n*sizeof(void*))!=engine::function<void*>(source::OriginalSlots[n]))return false;
    return true;
}
inline Backing* FindTable(void**table){
    const auto key=reinterpret_cast<uintptr_t>(table);size_t lo=0,hi=used;
    while(lo<hi){const auto mid=lo+(hi-lo)/2,value=reinterpret_cast<uintptr_t>(ordered[mid]->table);
        if(value<key)lo=mid+1;else hi=mid;}
    return lo<used&&ordered[lo]->table==table?ordered[lo]:nullptr;
}
inline void Remember(Backing*backing){
    const auto key=reinterpret_cast<uintptr_t>(backing->table);size_t lo=0,hi=used;
    while(lo<hi){const auto mid=lo+(hi-lo)/2;if(reinterpret_cast<uintptr_t>(ordered[mid]->table)<key)lo=mid+1;else hi=mid;}
    for(size_t n=used;n>lo;--n)ordered[n]=ordered[n-1];ordered[lo]=backing;
}
inline bool Current(const Backing&b){
    const auto p=local_server_match::Player(b.index);
    return !b.retired&&b.published&&p&&p->pawn==b.pawn&&p->epoch==b.epoch
        &&original_inventory_slots::SameIdentity(p->pawnIdentity,b.pawnId)&&Live(b.pawn,b.pawnId)
        &&Live(b.component,b.componentId)&&Live(b.world,b.worldId)&&b.world==state.world
        &&Read<void*>(b.pawn,0xa78)==b.component&&Read<void*>(b.component,0xc8)==b.pawn
        &&Read<void*>(b.component,0x2f0)==b.pawn&&Read<void**>(b.component)==b.table
        &&Read<void*>(b.component,0x10)==b.componentClass;
}
struct TickSample {observation::Sample source{};float stamp=0;bool stampKnown=false;};
inline bool Sample(Backing&backing,unsigned phase,TickSample&out){
    (void)phase;
    out={};if(!Context()||!Current(backing))return false;
    out.stampKnown=original_players::Read(backing.component,0xdc,out.stamp)&&std::isfinite(out.stamp);
    const auto p=local_server_match::Player(backing.index);if(!p||!p->alive){++state.lifeRejected;return false;}
    auto&s=out.source;s.request=observation::Request::Footstep;
    s.subject={static_cast<uint32_t>(backing.index),backing.epoch,reinterpret_cast<uintptr_t>(backing.pawn),backing.pawnId.index,backing.pawnId.serial};
    s.component=request::Convert(backing.componentId);s.world=request::Convert(backing.worldId);s.asset=request::Convert(backing.assetId);
    s.componentObject=reinterpret_cast<uintptr_t>(backing.component);s.worldObject=reinterpret_cast<uintptr_t>(backing.world);s.assetObject=reinterpret_cast<uintptr_t>(backing.asset);
    s.atMatchSeconds=local_server_match::Now();s.nativeWorldTimeKnown=original_players::Read(backing.world,0x740,s.nativeWorldSeconds)&&std::isfinite(s.nativeWorldSeconds);
    using GetWorld=void*(*)(void*);
    const auto objectWorld=Read<GetWorld>(Read<void*>(backing.component),0x1b8),pawnWorld=Read<GetWorld>(Read<void*>(backing.pawn),0x1b8);
    if(objectWorld!=engine::function<GetWorld>(0x2a64ee0)||pawnWorld!=engine::function<GetWorld>(0x3ed83b0)
       ||objectWorld(backing.component)!=backing.world||pawnWorld(backing.pawn)!=backing.world){++state.worldRejected;return false;}
    s.ownedWorld=true;s.sourceProfiles=state.profilesVerified&&request::GetState().profilesVerified;s.metadata=request::GetState().metadataVerified;
    if(!request::GroundOrigin(backing.pawn,backing.world,s)){++state.originUnavailable;return false;}
    if(!backing.assetReady||Read<void*>(backing.component,0x1d0)!=backing.asset||!Live(backing.asset,backing.assetId)){
        ++state.assetUnavailable;return false;}
    s.originalAsset=true;s.valid=true;return out.stampKnown&&observation::Good(s);
}
using Sampler=bool(*)(Backing&,unsigned,TickSample&);
using StampReader=bool(*)(Backing&,TickSample&);
inline bool ReadStamp(Backing&b,TickSample&out){
    out.stampKnown=Context()&&Current(b)&&original_players::Read(b.component,0xdc,out.stamp)&&std::isfinite(out.stamp);
    return out.stampKnown;
}
inline LONG SampleFault(unsigned code){++state.samplingFaults;state.lastSamplingException=code;return EXCEPTION_EXECUTE_HANDLER;}
inline bool ProtectedSample(Backing&b,unsigned phase,TickSample&out,Sampler sample=&Sample){
    __try{return sample(b,phase,out);}
    __except(SampleFault(GetExceptionCode())){out.source.valid=false;out.stampKnown=false;return false;}
}
inline bool ProtectedStamp(Backing&b,TickSample&out,StampReader read=&ReadStamp){
    __try{return read(b,out);}
    __except(SampleFault(GetExceptionCode())){out.stampKnown=false;return false;}
}
inline void Forward(void*object,float delta,int32_t type,void*tick){
    InterlockedIncrement64(&forwardAttempts);original(object,delta,type,tick);InterlockedIncrement64(&forwardCompleted);
}
struct Call {Backing*backing=nullptr;void*object=nullptr;float delta=0;int32_t tickType=0;void*tickFunction=nullptr;};
using Publisher=bool(*)(events::Event);
inline void Run(Call&call,Sampler sample=&Sample,Publisher publish=&events::PublishAudio,StampReader read=&ReadStamp){
    auto&b=*call.backing;TickSample before{},after{};
    const bool beforeOkay=ProtectedSample(b,1,before,sample);
    Forward(call.object,call.delta,call.tickType,call.tickFunction); // Exactly one original native Tick; never inside sampling SEH.
    const bool afterStamp=ProtectedStamp(b,after,read);
    const bool changed=before.stampKnown&&afterStamp&&before.stamp!=after.stamp;
    // A cheap native commit check avoids full source/position resampling on
    // every no-sound frame. A velocity change cannot reach this branch.
    const bool afterOkay=changed&&ProtectedSample(b,2,after,sample);
    core::Commit commit{before.stamp,after.stamp,true,state.profilesVerified,!b.nested,before.stampKnown,after.stampKnown};
    if(!beforeOkay||!afterStamp||(changed&&!afterOkay))++state.invalidSources;
    const auto receipt=core::Evaluate(++state.samples,before.source,after.source,commit);
    receipts[(state.samples-1)%ReceiptCapacity]=receipt;
    if(!before.stampKnown||!after.stampKnown)++state.unknownStamps;
    else if(receipt.changed)++state.stampChanges;else ++state.unchangedStamps;
    if(receipt.changed&&!receipt.savedOriginalWorldTime)++state.timeMismatches;
    if(receipt.request.requestEligible){++state.provenRequests;if(publish(receipt.request.event))++state.published;}
}
inline void Guarded(Call&call,Sampler sample=&Sample,Publisher publish=&events::PublishAudio,StampReader read=&ReadStamp){
    auto&b=*call.backing;const bool previousBusy=b.busy,previousNested=b.nested;b.busy=true;b.nested=false;
    __try{Run(call,sample,publish,read);}
    __finally{b.busy=previousBusy;b.nested=previousNested;}
}
inline void ObserveImpl(void*object,float delta,int32_t type,void*tick){
    if(!Thread()){InterlockedIncrement64(&offThread);Forward(object,delta,type,tick);return;}
    auto b=FindTable(Read<void**>(object));
    if(!b||b->component!=object||!Context()||!Current(*b)||tick!=static_cast<uint8_t*>(object)+0x48){Forward(object,delta,type,tick);return;}
    if(b->busy){b->nested=true;InterlockedIncrement64(&reentrant);Forward(object,delta,type,tick);return;}
    Call call{b,object,delta,type,tick};Guarded(call);
}
__declspec(noinline) inline void Observe(void*object,float delta,int32_t type,void*tick){
    InterlockedIncrement64(&entries);InterlockedIncrement64(&activeCalls);
    __try{ObserveImpl(object,delta,type,tick);}
    __finally{InterlockedDecrement64(&activeCalls);}
}
using IdentityLive=bool(*)(void*,const Weak&);
using VptrSwap=void*(*)(void*,void*,void*);
inline void* SwapVptr(void*object,void*desired,void*expected){return InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(object),desired,expected);}
inline bool Retire(Backing&b,IdentityLive live=&Live,VptrSwap swap=&SwapVptr){
    if(b.retired)return true;if(b.busy)return false;
    if(!live(b.component,b.componentId)){b.retired=true;++state.expiredRetained;++state.retirements;return true;}
    const auto actual=Read<void**>(b.component);
    if(actual==Source()){b.restored=b.retired=true;++state.retirements;return true;}
    if(actual!=b.table||swap(b.component,Source(),b.table)!=b.table){
        ++state.conflicts;return Fail(7980,"humanlike_audio_tick_foreign_vptr_layer_preserved");}
    b.restored=b.retired=true;++state.restores;++state.retirements;return true;
}
inline void RefreshAsset(Backing&b){
    auto asset=Read<void*>(b.component,0x1d0);
    if(b.assetReady&&asset==b.asset&&Live(asset,b.assetId))return;
    b.assetReady=false;b.asset=asset;b.assetId={};
    const auto&context=request::GetState();
    if(!asset||!Live(context.containerClass,context.containerClassId)
       ||!original_players::Derived(asset,context.containerClass))return;
    b.assetReady=original_inventory_slots::CaptureIdentity(asset,b.assetId)&&Live(asset,b.assetId);
}
inline bool Bind(size_t index){
    auto p=local_server_match::Player(index);if(!p||!Live(p->pawn,p->pawnIdentity)||!p->epoch)return false;
    if(auto b=bindings[index]){
        if(Current(*b)){RefreshAsset(*b);return true;}if(!Retire(*b))return false;bindings[index]=nullptr;
    }
    auto component=Read<void*>(p->pawn,0xa78);Weak id{};
    if(!original_players::IsA(component,0x685b1f0)||Read<void*>(component,0xc8)!=p->pawn||Read<void*>(component,0x2f0)!=p->pawn
       ||!original_inventory_slots::CaptureIdentity(component,id)||!Live(component,id))return false;
    if(Read<void**>(component)!=Source())return Fail(7981,"humanlike_audio_tick_refuses_foreign_component_table");
    if(used>=MaxBackings){++state.capacityRejected;return Fail(7989,"humanlike_audio_tick_published_backing_capacity_exhausted_observation_disabled");}
    const auto tableBytes=(source::TableEntries+1)*sizeof(void*);
    auto allocation=static_cast<void**>(VirtualAlloc(nullptr,tableBytes,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    if(!allocation)return Fail(7982,"humanlike_audio_tick_private_table_allocation_failed");
    std::memcpy(allocation,Source()-1,tableBytes);auto table=allocation+1;table[source::TickSlot/8]=reinterpret_cast<void*>(&Observe);
    DWORD old=0;if(!VirtualProtect(allocation,tableBytes,PAGE_READONLY,&old)){VirtualFree(allocation,0,MEM_RELEASE);return Fail(7983,"humanlike_audio_tick_table_readonly_protection_failed");}
    auto&b=backings[used];b={};b.component=component;b.pawn=p->pawn;b.world=state.world;b.componentId=id;b.pawnId=p->pawnIdentity;
    b.worldId=state.worldId;b.index=index;b.epoch=p->epoch;b.allocation=allocation;b.table=table;b.componentClass=Read<void*>(component,0x10);RefreshAsset(b);
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(component),table,Source())!=Source()){
        VirtualFree(allocation,0,MEM_RELEASE);b={};return Fail(7984,"humanlike_audio_tick_component_publication_raced");}
    b.published=true;Remember(&b);++used;bindings[index]=&b;++state.binds;++state.allocations;
    SYSTEM_INFO system{};GetSystemInfo(&system);state.retainedBytes+=(tableBytes+system.dwPageSize-1)/system.dwPageSize*system.dwPageSize;
    return true;
}
inline bool InitHost(void*world){
    if(state.initialized)return Context()&&state.world==world;if(state.closed)return false;
    if(!request::Context()||request::GetState().world!=world)return Fail(7985,"humanlike_audio_tick_waiting_original_request_context");
    state.gameThread=GetCurrentThreadId();state.world=world;state.worldId=request::GetState().worldId;
    if(!ProfilesMatch())return Fail(7986,"humanlike_audio_tick_full_original_source_or_table_mismatch");
    if(original&&original!=engine::function<Tick>(source::TickRva))return Fail(7987,"humanlike_audio_tick_original_parent_cannot_change");
    original=engine::function<Tick>(source::TickRva);state.profilesVerified=true;state.initialized=true;
    state.error=0;state.stage="humanlike_audio_tick_ready_for_current_owned_components";Cache();return true;
}
inline bool Poll(){
    if(!Context())return false;
    for(size_t n=0;n<MaxPlayers;++n){const auto p=local_server_match::Player(n);
        if(!p){if(auto b=bindings[n]){if(!Retire(*b))return false;bindings[n]=nullptr;}continue;}
        if(!Bind(n)&&state.error)return false;}
    state.stage="humanlike_audio_tick_observing_original_component_ticks_no_playback_claim";Cache();return true;
}
inline bool RetirePlayer(size_t index,void*pawn,uint32_t epoch){
    if(!state.initialized||state.closed||index>=MaxPlayers)return true;
    if(!Thread())return false;auto b=bindings[index];
    if(!b||b->pawn!=pawn||b->epoch!=epoch)return true; // Late old-life retirement cannot touch a new life.
    if(!Retire(*b))return false;bindings[index]=nullptr;Cache();return true;
}
inline bool Close(){
    if(state.closed)return true;if(!state.initialized){state.closed=true;Cache();return true;}
    if(!Thread())return Fail(7988,"humanlike_audio_tick_close_requires_original_game_thread");
    state.closing=true;if(Count(activeCalls)){state.stage="humanlike_audio_tick_close_waiting_inflight_original_tick";Cache();return false;}
    for(auto&b:bindings)if(b){if(!Retire(*b))return false;b=nullptr;}
    // Every published backing and static callback is retained for this owned
    // process, including weak-expired PendingKill objects and late dispatches.
    state.closed=true;state.stage="humanlike_audio_tick_restored_published_backings_retained";Cache();return true;
}
inline std::string Json(){
    AcquireSRWLockShared(&cacheLock);const auto v=cached;ReleaseSRWLockShared(&cacheLock);const auto&s=v.state;
    local_server_match::Json out;const auto b=[](bool x){return x?"true":"false";};const auto u=[](uint64_t x){return static_cast<unsigned long long>(x);};
    out.Add("{\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"closed\":%s,\"profiles_verified\":%s,\"active_bindings\":%zu,\"retained_backings\":%zu,\"retained_bytes\":%llu,\"entries\":%llu,\"original_forward_attempts\":%llu,\"original_forward_completed\":%llu,\"off_thread_original_forwards\":%llu,\"reentrant_original_forwards\":%llu,\"active_calls\":%llu,\"samples\":%llu,\"native_stamp_changes\":%llu,\"proven_requests\":%llu,\"published\":%llu,\"invalid_sources\":%llu,\"asset_unavailable\":%llu,\"origin_unavailable\":%llu,\"world_rejected\":%llu,\"life_rejected\":%llu,\"unchanged_stamps\":%llu,\"unknown_stamps\":%llu,\"time_mismatches\":%llu,\"sampling_faults\":%llu,\"vptr_installs\":%llu,\"vptr_restores\":%llu,\"expired_backings_retained\":%llu,\"restore_conflicts\":%llu,\"capacity_rejected\":%llu,\"producer\":\"original_native_Tick_ground_request_stamp_commit\",\"source_call_rva\":116603502,\"image_writes\":0,\"manual_tick_calls\":0,\"stamp_bank_or_speed_writes\":0,\"audio_parameter_or_listener_writes\":0,\"actual_audio_post_available\":false,\"hardware_playback_verified\":false,\"same_float_time_can_hide_request\":true,\"native_landing_direct_call_covered\":false,\"runtime_acceptance_verified\":false,\"last_receipts\":[",
        s.stage,s.error,b(s.initialized),b(s.closed),b(s.profilesVerified),v.activeBindings,v.retainedBackings,u(s.retainedBytes),u(v.entries),u(v.attempts),u(v.completed),u(v.offThread),u(v.reentrant),u(v.activeCalls),u(s.samples),u(s.stampChanges),u(s.provenRequests),u(s.published),u(s.invalidSources),u(s.assetUnavailable),u(s.originUnavailable),u(s.worldRejected),u(s.lifeRejected),u(s.unchangedStamps),u(s.unknownStamps),u(s.timeMismatches),u(s.samplingFaults),u(s.binds),u(s.restores),u(s.expiredRetained),u(s.conflicts),u(s.capacityRejected));
    bool comma=false;for(const auto&r:v.receipts)if(r.request.occurrence){const auto&a=r.request.before;
        out.Add("%s{\"occurrence\":%llu,\"index\":%u,\"epoch\":%u,\"pawn_weak\":[%d,%d],\"component_weak\":[%d,%d],\"world_weak\":[%d,%d],\"stamp_before\":%.9g,\"stamp_after\":%.9g,\"saved_original_time_matches\":%s,\"original_completed\":%s,\"stable_source\":%s,\"eligible\":%s,\"tick_entry_at_match_seconds\":%.17g,\"receipt_observed_at_match_seconds\":%.17g,\"world_time_opaque\":%.17g,\"ground_origin\":[%.17g,%.17g,%.17g]}",
            comma?",":"",u(r.request.occurrence),a.subject.index,a.subject.epoch,a.subject.weakIndex,a.subject.weakSerial,a.component.index,a.component.serial,a.world.index,a.world.serial,std::isfinite(r.stamp.before)?static_cast<double>(r.stamp.before):0.,std::isfinite(r.stamp.after)?static_cast<double>(r.stamp.after):0.,b(r.savedOriginalWorldTime),b(r.stamp.originalTickCompleted),b(r.request.sameLife),b(r.request.requestEligible),a.atMatchSeconds,r.request.after.atMatchSeconds,a.nativeWorldTimeKnown?a.nativeWorldSeconds:0.,a.originalPosition.x,a.originalPosition.y,a.originalPosition.z);comma=true;}
    out.Add("]}");return out.value;
}
} // namespace local_server_humanlike_audio_tick
