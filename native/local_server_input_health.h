#pragma once
#include "local_server_ip_ack_compat.h"
#include "original_inventory_slots.h"
#include <array>
#include <atomic>
#include <cstdio>

// Client-only observation of real saved moves and original ACK entrypoints.
// Only this owned Pawn's two vtable entries are wrapped; every original call
// and exception is preserved. No queue, clock, driver or error-state writes.
namespace local_server_input_health {
using Weak=original_inventory_slots::WeakIdentity;
using AckFn=void(*)(void*,int32_t,uint32_t);
using GradualFn=void(*)(void*,int32_t,uint32_t,const void*);
inline constexpr size_t PawnEntries=0xae0/8,AckSlot=0xa70/8,GradualSlot=0xa68/8;
inline constexpr size_t TraceCount=16,MaximumBindings=64;
inline constexpr uint64_t SampleMilliseconds=50;
struct QueueSample {
    uint64_t atMs=0;
    bool valid=false,clockValid=false,overflow=false;
    void *movement=nullptr,*prediction=nullptr,*moves=nullptr,*lastAckMove=nullptr;
    int32_t count=0,capacity=0,maximum=0,timestep=0;
    uint32_t firstIndex=0,lastIndex=0,lastAckIndex=0,clientRawIndex=0;
    int32_t clientMinusAck=0;
    float overflowStartedAt=0;
};
struct AckRecord {
    uint64_t ordinal=0,atMs=0,moveGapMs=0,returnedAtMs=0;
    int32_t eventId=-1,beforeCount=-1,afterCount=-1;
    uint32_t rawIndex=0,lastAckBefore=0,lastAckAfter=0;
    uint8_t kind=0; // 1=normal;2=gradual. Hard correction has no entry hook.
    bool returned=false,lastAckMatchesRequested=false;
};
struct Diagnostics {
    unsigned error=0;
    bool installed=false,profilesVerified=false,metadataVerified=false,weakVerified=false;
    bool sourceSlotsPreserved=false,hardCorrectionArrivalInstrumented=false;
    const char* stage="client_input_health_waiting_owned_role2_pawn";
    DWORD gameThread=0;
    void *world=nullptr,*pc=nullptr,*ps=nullptr,*pawn=nullptr,*connection=nullptr;
    void **sourceTable=nullptr,**privateTable=nullptr;
    uint64_t epoch=0,installs=0,retirements=0,samples=0,invalidSamples=0;
    uint64_t normalEntries=0,gradualEntries=0,eventOnlyEntries=0,movementEntries=0;
    uint64_t originalReturns=0,lastAnyAckAtMs=0,lastMovementAckAtMs=0,maxMovementAckGapMs=0;
    uint64_t nativeAckPointerChanges=0,lastNativeAckProgressObservedAtMs=0;
    uint64_t fullSamples=0,overflowSamples=0,observerReadErrors=0,retainedLayeredTables=0;
    uint64_t restoredTables=0,retainedPublishedTables=0,retiredWeakUnavailable=0;
    uint64_t unobservedNormalForwards=0,unobservedGradualForwards=0;
    int32_t highWaterSavedMoves=0,maxObservedRawLag=0;
    uint32_t lastMovementAckIndex=0;
    QueueSample latest{};
    std::array<AckRecord,TraceCount> ackTrace{};
};
struct Binding {
    void *pawn=nullptr,*pc=nullptr,*ps=nullptr,*world=nullptr,*connection=nullptr;
    void **source=nullptr,**copy=nullptr;
    Weak pawnIdentity{},pcIdentity{},stateIdentity{},connectionIdentity{};
    AckFn originalAck=nullptr;GradualFn originalGradual=nullptr;
    bool active=false,layered=false;uint32_t inFlight=0;
};
inline Diagnostics state;
inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline std::array<Binding,MaximumBindings> bindings{};
inline Binding* current=nullptr;
inline uint64_t lastPollAt=0;
// All published health tables keep static wrapper entry points until process
// exit. Pin the first verified parent function pair for that same lifetime so
// a late pending-kill callback still forwards it exactly once after the active
// diagnostic Binding slot was cleared. A different future parent pair is an
// unsupported profile, never guessed from a stale Pawn address.
inline std::atomic<AckFn> processParentAck{nullptr};
inline std::atomic<GradualFn> processParentGradual{nullptr};
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline Diagnostics Snapshot(){AcquireSRWLockShared(&cacheLock);const auto copy=state;ReleaseSRWLockShared(&cacheLock);return copy;}
inline const Diagnostics& GetState(){return state;} // same original game thread only
inline bool Fail(unsigned error,const char* reason){AcquireSRWLockExclusive(&cacheLock);state.error=error;state.stage=reason;ReleaseSRWLockExclusive(&cacheLock);return false;}
inline bool VerifyProfiles(){
    struct Profile{uintptr_t rva;size_t bytes;uint64_t fnv;};
    static constexpr Profile profiles[]{
        {0x787c310,220,0x45017c2aaf6a88adULL},
        {0x787c3f0,251,0x8ca09b346b69eda4ULL},
        {0x789bd40,561,0x08a377f04f154f78ULL},
        {0x7896e10,867,0x495b4577c0e04218ULL},
        {0x41abd40,311,0x2b126fa9a8508463ULL},
        {0x6c68f10,402,0x2095ab47a3557630ULL},
        {0x6c68cc0,584,0x53c4353a4501ec4dULL},
        {0x789f7b0,32,0x61323e7251773601ULL},
        {0x789f7d0,28,0xcffc79c87f2ee8dbULL},
    };
    for(const auto& p:profiles){
        const auto* bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        uint64_t hash=14695981039346656037ULL;
        for(size_t i=0;i<p.bytes;++i)hash=(hash^bytes[i])*1099511628211ULL;
        if(hash!=p.fnv)return false;
    }
    auto image=reinterpret_cast<void*>(engine::image);
    // Primary ends after AD8. AE0 is the complete, separately identified
    // DeathReactionObject property descriptor, not another function entry.
    return Read<uintptr_t>(image,0xb4dd028)==engine::image+0x787a070
        &&Read<uintptr_t>(image,0xb4dd030)==engine::image+0xc5253e8
        &&(Read<uint32_t>(image,0xb4dd030+0x18)&0x3f)==0x12
        &&Read<uint32_t>(image,0xb4dd030+0x32)==0x11f0;
}
inline bool FunctionABI(const wchar_t* name,uint16_t bytes,uintptr_t exec){
    auto fn=engine::Find(name);
    return fn&&Read<uint16_t>(fn,0xbe)==bytes&&Read<uintptr_t>(fn,0xe0)==engine::image+exec
        &&(Read<uint32_t>(fn,0xb8)&0x01000440U)==0x01000440U;
}
inline bool Owned(void* world,void* pc,void* ps,void* pawn,void* connection){
    auto driver=Read<void*>(world,0x40),gi=Read<void*>(world,0x1d8);
    auto locals=Read<void**>(gi,0x40);auto player=Read<int32_t>(gi,0x48)==1?Read<void*>(locals):nullptr;
    return world&&original_players::world==world
        &&original_players::IsA(pc,original_players::ShooterControllerClass)
        &&original_players::IsA(ps,original_players::ShooterStateClass)
        &&original_players::IsA(pawn,original_players::ShooterCharacterClass)
        &&Read<uint8_t>(pc,0x224)==2&&Read<uint8_t>(pawn,0x224)==2
        &&Read<void*>(pc,0x470)==ps&&Read<void*>(ps,0x210)==pc
        &&Read<void*>(pc,0x4b0)==pawn&&Read<void*>(pawn,0x4a0)==pc&&Read<void*>(pawn,0x488)==ps
        &&player&&Read<void*>(player,0x38)==pc&&Read<void*>(pc,0x510)==player
        &&original_players::IsA(connection,0x5112be0)
        &&Read<void*>(driver,0xc0)==connection&&Read<void*>(driver,0x178)==world
        &&Read<void*>(connection,0x60)==driver&&Read<void*>(connection,0xa0)==pc
        &&Read<int32_t>(connection,0x13c)==3;
}
inline bool WeakMatch(const Binding& b){
    return original_inventory_slots::ResolveIdentity(b.pawnIdentity)==b.pawn
        &&original_inventory_slots::ResolveIdentity(b.pcIdentity)==b.pc
        &&original_inventory_slots::ResolveIdentity(b.stateIdentity)==b.ps
        &&original_inventory_slots::ResolveIdentity(b.connectionIdentity)==b.connection;
}
inline QueueSample Sample(void* pawn,uint64_t now){
    QueueSample s;s.atMs=now;s.movement=Read<void*>(pawn,0x4f8);
    if(!original_players::IsA(s.movement,0x6c7d380))return s;
    s.prediction=Read<void*>(s.movement,0x7e8);if(!s.prediction)return s;
    s.moves=Read<void*>(s.prediction,0x20);s.count=Read<int32_t>(s.prediction,0x28);
    s.capacity=Read<int32_t>(s.prediction,0x2c);s.maximum=Read<int32_t>(s.prediction,0x68);
    s.lastAckMove=Read<void*>(s.prediction,0x50);
    if(s.count<0||s.count>s.capacity||s.capacity>4096||s.maximum<=0||s.maximum>4096||(s.count&&!s.moves))return s;
    if(s.count){
        auto first=Read<void*>(s.moves),last=Read<void*>(s.moves,static_cast<size_t>(s.count-1)*16);
        if(!first||!last||!original_players::Read(first,0x1c,s.firstIndex)||!original_players::Read(last,0x1c,s.lastIndex))return s;
    }
    if(s.lastAckMove&&!original_players::Read(s.lastAckMove,0x1c,s.lastAckIndex))return s;
    s.overflow=Read<uint8_t>(s.movement,0x13d8)!=0;s.overflowStartedAt=Read<float>(s.movement,0x13dc);
    if(!std::isfinite(s.overflowStartedAt)){s.overflowStartedAt=0;return s;}
    auto clock=static_cast<uint8_t*>(s.prediction)+0x1b0;
    auto table=Read<void**>(clock);auto vb=Read<void*>(clock,0x10);
    const int32_t d=Read<int32_t>(vb,4);
    s.timestep=Read<int32_t>(s.prediction,0xb78);
    if(table==reinterpret_cast<void**>(engine::image+0xb600b30)&&d>=0&&d<=0x1000
       &&s.timestep==Read<int32_t>(clock,0x9c8)
       &&original_players::Read(clock,static_cast<size_t>(d)+0x18,s.clientRawIndex)){
        s.clockValid=true;if(s.lastAckMove)s.clientMinusAck=static_cast<int32_t>(s.clientRawIndex-s.lastAckIndex);
    }
    s.valid=true;return s;
}
inline void StoreSample(const QueueSample& sample){
    if(sample.atMs-state.latest.atMs<SampleMilliseconds)return;
    if(!sample.valid){++state.invalidSamples;state.latest=sample;return;}
    if(sample.lastAckMove!=state.latest.lastAckMove||sample.lastAckIndex!=state.latest.lastAckIndex){
        ++state.nativeAckPointerChanges;state.lastNativeAckProgressObservedAtMs=sample.atMs;
    }
    ++state.samples;if(sample.count>state.highWaterSavedMoves)state.highWaterSavedMoves=sample.count;
    if(sample.clockValid&&sample.clientMinusAck>state.maxObservedRawLag)state.maxObservedRawLag=sample.clientMinusAck;
    if(sample.count>=sample.maximum)++state.fullSamples;if(sample.overflow)++state.overflowSamples;
    state.latest=sample;
}
inline uint64_t CaptureEntryImpl(Binding* b,int32_t eventId,uint32_t index,uint8_t kind){
    if(!b||b!=current||!b->active||GetCurrentThreadId()!=original_players::gameThread)return 0;
    const auto now=GetTickCount64();const auto sample=Sample(b->pawn,now);
    AcquireSRWLockExclusive(&cacheLock);
    const auto ordinal=state.normalEntries+state.gradualEntries+1;
    AckRecord record;record.ordinal=ordinal;record.atMs=now;record.eventId=eventId;record.rawIndex=index;record.kind=kind;
    if(sample.valid){record.beforeCount=sample.count;record.lastAckBefore=sample.lastAckIndex;}
    if(kind==1)++state.normalEntries;else ++state.gradualEntries;
    state.lastAnyAckAtMs=now;
    if(index){
        ++state.movementEntries;
        if(state.lastMovementAckAtMs){record.moveGapMs=now-state.lastMovementAckAtMs;
            if(record.moveGapMs>state.maxMovementAckGapMs)state.maxMovementAckGapMs=record.moveGapMs;}
        state.lastMovementAckAtMs=now;state.lastMovementAckIndex=index;
    }else ++state.eventOnlyEntries;
    state.ackTrace[(ordinal-1)%TraceCount]=record;StoreSample(sample);
    ReleaseSRWLockExclusive(&cacheLock);return ordinal;
}
inline uint64_t CaptureEntry(Binding* b,int32_t eventId,uint32_t index,uint8_t kind){
    __try{return CaptureEntryImpl(b,eventId,index,kind);}
    __except(EXCEPTION_EXECUTE_HANDLER){InterlockedIncrement64(reinterpret_cast<volatile LONG64*>(&state.observerReadErrors));return 0;}
}
inline void CaptureReturnImpl(Binding* b,uint64_t ordinal){
    if(!ordinal||!b||b!=current||GetCurrentThreadId()!=original_players::gameThread)return;
    const auto now=GetTickCount64();const auto sample=Sample(b->pawn,now);
    AcquireSRWLockExclusive(&cacheLock);
    auto& record=state.ackTrace[(ordinal-1)%TraceCount];
    if(record.ordinal==ordinal){
        record.returned=true;record.returnedAtMs=now;
        if(sample.valid){record.afterCount=sample.count;record.lastAckAfter=sample.lastAckIndex;
            record.lastAckMatchesRequested=record.rawIndex&&sample.lastAckIndex==record.rawIndex;}
    }
    ++state.originalReturns;StoreSample(sample);ReleaseSRWLockExclusive(&cacheLock);
}
inline void CaptureReturn(Binding* b,uint64_t ordinal){
    __try{CaptureReturnImpl(b,ordinal);}
    __except(EXCEPTION_EXECUTE_HANDLER){InterlockedIncrement64(reinterpret_cast<volatile LONG64*>(&state.observerReadErrors));}
}
inline Binding* FindBinding(void* pawn){for(auto& b:bindings)if(b.copy&&b.pawn==pawn)return &b;return nullptr;}
inline void NormalAck(void* pawn,int32_t eventId,uint32_t index){
    auto binding=GetCurrentThreadId()==original_players::gameThread?FindBinding(pawn):nullptr;
    const auto retained=processParentAck.load(std::memory_order_acquire);
    const auto original=binding?binding->originalAck:retained?retained:engine::function<AckFn>(0x787c310);
    if(binding)++binding->inFlight;
    else if(retained){AcquireSRWLockExclusive(&cacheLock);++state.unobservedNormalForwards;ReleaseSRWLockExclusive(&cacheLock);}
    const auto ordinal=CaptureEntry(binding,eventId,index,1);
    __try{
        original(pawn,eventId,index); // No observer SEH catch around the original.
        CaptureReturn(binding,ordinal);
    }__finally{if(binding)--binding->inFlight;}
}
inline void GradualAck(void* pawn,int32_t eventId,uint32_t index,const void* correction){
    auto binding=GetCurrentThreadId()==original_players::gameThread?FindBinding(pawn):nullptr;
    const auto retained=processParentGradual.load(std::memory_order_acquire);
    const auto original=binding?binding->originalGradual:retained?retained:engine::function<GradualFn>(0x787c3f0);
    if(binding)++binding->inFlight;
    else if(retained){AcquireSRWLockExclusive(&cacheLock);++state.unobservedGradualForwards;ReleaseSRWLockExclusive(&cacheLock);}
    const auto ordinal=CaptureEntry(binding,eventId,index,2);
    __try{
        original(pawn,eventId,index,correction); // Original exception propagation.
        CaptureReturn(binding,ordinal);
    }__finally{if(binding)--binding->inFlight;}
}
inline bool RetireClient(){
    if(GetCurrentThreadId()!=original_players::gameThread)return false;
    if(!current)return true;auto& b=*current;if(b.inFlight)return false;
    auto actual=original_inventory_slots::ResolveIdentity(b.pawnIdentity);
    bool restored=false,foreignLayer=false;
    if(actual==b.pawn){
        const auto table=Read<void**>(actual);
        if(table==b.copy){
            restored=InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(actual),b.source,b.copy)==b.copy;
            if(!restored)return Fail(5390,"client_input_health_owned_vptr_restore_conflict");
        }else if(table==b.source)restored=true;
        else {b.layered=true;foreignLayer=true;} // Never overwrite a higher layer.
    }
    b.active=false;
    AcquireSRWLockExclusive(&cacheLock);state.installed=false;++state.retirements;
    if(restored)++state.restoredTables;
    if(b.layered)++state.retainedLayeredTables;
    if(actual!=b.pawn)++state.retiredWeakUnavailable;
    ++state.retainedPublishedTables;ReleaseSRWLockExclusive(&cacheLock);
    // Do not free published backing based on an ordinary weak resolver.
    // Clear only the active observation slot; late calls use the immutable
    // process parent pair and static wrappers, not a retired Binding record.
    b={};current=nullptr;
    return foreignLayer?Fail(5390,"client_input_health_retired_table_retained_under_foreign_layer"):true;
}
inline bool Install(void* world,void* pc,void* ps,void* pawn,void* connection){
    if(!VerifyProfiles())return Fail(5391,"client_input_health_native_profile_or_primary_boundary_mismatch");
    if(!FunctionABI(L"/Script/ShooterGame.ShooterCharacter.ClientAckInputAndMovement",8,0x6c68f10)
       ||!FunctionABI(L"/Script/ShooterGame.ShooterCharacter.ClientAckInputAndMovementWithCorrection",88,0x6c68cc0))
        return Fail(5392,"client_input_health_native_ack_metadata_mismatch");
    auto source=Read<void**>(pawn);
    if((Read<uintptr_t>(source,AckSlot*8)!=engine::image+0x787c310
        &&!local_server_ip_ack_compat::IsOwnedAckEntry(pawn,Read<void*>(source,AckSlot*8)))
       ||Read<uintptr_t>(source,GradualSlot*8)!=engine::image+0x787c3f0
       ||Read<uintptr_t>(source,0xad8)!=engine::image+0x787a070
       ||!original_players::Readable(source-1,(PawnEntries+1)*8))
        return Fail(5393,"client_input_health_preceding_ack_slots_or_table_extent_changed");
    Binding* next=nullptr;
    for(auto& b:bindings){
        if(!next&&!b.copy)next=&b;
    }
    if(!next)return Fail(5394,"client_input_health_bounded_observer_binding_capacity_reached");
    auto& b=*next;b.pawn=pawn;b.pc=pc;b.ps=ps;b.world=world;b.connection=connection;b.source=source;
    b.originalAck=reinterpret_cast<AckFn>(source[AckSlot]);b.originalGradual=reinterpret_cast<GradualFn>(source[GradualSlot]);
    const auto parentAck=processParentAck.load(std::memory_order_acquire);
    const auto parentGradual=processParentGradual.load(std::memory_order_acquire);
    if((parentAck&&parentAck!=b.originalAck)||(parentGradual&&parentGradual!=b.originalGradual)){
        b={};return Fail(5401,"client_input_health_process_parent_pair_changed_with_retained_tables");
    }
    if(!parentGradual)processParentGradual.store(b.originalGradual,std::memory_order_release);
    if(!parentAck)processParentAck.store(b.originalAck,std::memory_order_release);
    if(!original_inventory_slots::CaptureIdentity(pawn,b.pawnIdentity)
       ||!original_inventory_slots::CaptureIdentity(pc,b.pcIdentity)
       ||!original_inventory_slots::CaptureIdentity(ps,b.stateIdentity)
       ||!original_inventory_slots::CaptureIdentity(connection,b.connectionIdentity)||!WeakMatch(b)){
        b={};return Fail(5395,"client_input_health_real_weak_identity_capture_failed");
    }
    auto storage=static_cast<void**>(VirtualAlloc(nullptr,(PawnEntries+1)*8,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!storage){b={};return Fail(5396,"client_input_health_owned_vtable_allocation_failed");}
    std::memcpy(storage,source-1,(PawnEntries+1)*8);b.copy=storage+1;
    b.copy[AckSlot]=reinterpret_cast<void*>(&NormalAck);b.copy[GradualSlot]=reinterpret_cast<void*>(&GradualAck);
    DWORD protection=0;
    if(!VirtualProtect(storage,(PawnEntries+1)*8,PAGE_READONLY,&protection)){
        VirtualFree(storage,0,MEM_RELEASE);b={};return Fail(5397,"client_input_health_owned_table_protection_failed");
    }
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(pawn),b.copy,source)!=source){
        VirtualFree(storage,0,MEM_RELEASE);b={};return Fail(5398,"client_input_health_owned_vptr_install_conflict");
    }
    b.active=true;current=&b;
    AcquireSRWLockExclusive(&cacheLock);
    const auto epoch=state.epoch+1,installs=state.installs+1,retirements=state.retirements,retained=state.retainedLayeredTables;
    const auto restored=state.restoredTables,published=state.retainedPublishedTables,weakUnavailable=state.retiredWeakUnavailable;
    const auto lateNormal=state.unobservedNormalForwards,lateGradual=state.unobservedGradualForwards;
    state={};state.epoch=epoch;state.installs=installs;state.retirements=retirements;state.retainedLayeredTables=retained;
    state.restoredTables=restored;state.retainedPublishedTables=published;state.retiredWeakUnavailable=weakUnavailable;
    state.unobservedNormalForwards=lateNormal;state.unobservedGradualForwards=lateGradual;
    state.installed=true;state.profilesVerified=true;state.metadataVerified=true;state.weakVerified=true;state.sourceSlotsPreserved=true;
    state.gameThread=GetCurrentThreadId();state.world=world;state.pc=pc;state.ps=ps;state.pawn=pawn;state.connection=connection;
    state.sourceTable=source;state.privateTable=b.copy;state.stage="client_input_health_original_ack_observation_installed";
    ReleaseSRWLockExclusive(&cacheLock);return true;
}
inline bool PollClientImpl(void* world,void* pc,void* pawn){
    if(!engine::image||!original_players::gameThread||GetCurrentThreadId()!=original_players::gameThread)return false;
    if(state.error)return false;
    const auto now=GetTickCount64();if(now-lastPollAt<SampleMilliseconds)return true;lastPollAt=now;
    auto ps=Read<void*>(pc,0x470),driver=Read<void*>(world,0x40),connection=Read<void*>(driver,0xc0);
    if(!Owned(world,pc,ps,pawn,connection)){
        if(current&&!RetireClient())return false;
        AcquireSRWLockExclusive(&cacheLock);state.stage="client_input_health_waiting_actual_open_owned_role2_tuple";ReleaseSRWLockExclusive(&cacheLock);return true;
    }
    if(current&&(current->pawn!=pawn||current->pc!=pc||current->ps!=ps||current->world!=world
       ||current->connection!=connection||!WeakMatch(*current))&&!RetireClient())return false;
    if(!current&&!Install(world,pc,ps,pawn,connection))return false;
    auto actual=Read<void**>(pawn);
    if(Read<void*>(actual,AckSlot*8)!=reinterpret_cast<void*>(&NormalAck)
       ||Read<void*>(actual,GradualSlot*8)!=reinterpret_cast<void*>(&GradualAck))
        return Fail(5399,"client_input_health_observer_ack_slots_changed_after_install");
    const auto sample=Sample(pawn,now);const bool weakVerified=WeakMatch(*current);
    AcquireSRWLockExclusive(&cacheLock);StoreSample(sample);state.weakVerified=weakVerified;
    state.stage=sample.valid?"client_input_health_native_saved_moves_observed":"client_input_health_waiting_native_client_prediction";
    ReleaseSRWLockExclusive(&cacheLock);return true;
}
inline bool PollClient(void* world,void* pc,void* pawn){
    __try{return PollClientImpl(world,pc,pawn);}
    __except(EXCEPTION_EXECUTE_HANDLER){return Fail(5400,"client_input_health_observer_read_exception");}
}
inline void WriteCachedState(FILE* output){
    if(!output)return;const auto d=Snapshot();const auto& q=d.latest;
    const auto yes=[](bool value){return value?"true":"false";};
    std::fprintf(output,"{\"stage\":\"%s\",\"error\":%u,\"installed\":%s,\"profiles_verified\":%s,\"metadata_verified\":%s,\"weak_verified\":%s,\"epoch\":%llu,\"samples\":%llu,\"invalid_samples\":%llu,\"normal_entries\":%llu,\"gradual_entries\":%llu,\"event_only_entries\":%llu,\"movement_entries\":%llu,\"original_returns\":%llu,\"last_any_ack_at_ms\":%llu,\"last_movement_ack_at_ms\":%llu,\"last_movement_ack_index\":%u,\"max_movement_ack_gap_ms\":%llu,\"hard_correction_arrival_instrumented\":false,\"native_ack_pointer_changes\":%llu,\"last_native_ack_progress_observed_at_ms\":%llu,\"high_water_saved_moves\":%d,\"max_observed_raw_lag\":%d,\"full_samples\":%llu,\"overflow_samples\":%llu,\"observer_read_errors\":%llu,\"retained_layered_tables\":%llu,\"queue\":{\"at_ms\":%llu,\"valid\":%s,\"count\":%d,\"capacity\":%d,\"maximum\":%d,\"first_index\":%u,\"last_index\":%u,\"last_ack_index\":%u,\"overflow\":%s,\"overflow_started_at\":%.9g,\"clock_valid\":%s,\"client_raw_index\":%u,\"client_minus_ack\":%d,\"timestep\":%d},\"ack_trace\":[",
        d.stage,d.error,yes(d.installed),yes(d.profilesVerified),yes(d.metadataVerified),yes(d.weakVerified),
        static_cast<unsigned long long>(d.epoch),static_cast<unsigned long long>(d.samples),static_cast<unsigned long long>(d.invalidSamples),
        static_cast<unsigned long long>(d.normalEntries),static_cast<unsigned long long>(d.gradualEntries),static_cast<unsigned long long>(d.eventOnlyEntries),static_cast<unsigned long long>(d.movementEntries),static_cast<unsigned long long>(d.originalReturns),
        static_cast<unsigned long long>(d.lastAnyAckAtMs),static_cast<unsigned long long>(d.lastMovementAckAtMs),d.lastMovementAckIndex,static_cast<unsigned long long>(d.maxMovementAckGapMs),
        static_cast<unsigned long long>(d.nativeAckPointerChanges),static_cast<unsigned long long>(d.lastNativeAckProgressObservedAtMs),d.highWaterSavedMoves,d.maxObservedRawLag,
        static_cast<unsigned long long>(d.fullSamples),static_cast<unsigned long long>(d.overflowSamples),static_cast<unsigned long long>(d.observerReadErrors),static_cast<unsigned long long>(d.retainedLayeredTables),
        static_cast<unsigned long long>(q.atMs),yes(q.valid),q.count,q.capacity,q.maximum,q.firstIndex,q.lastIndex,q.lastAckIndex,yes(q.overflow),static_cast<double>(q.overflowStartedAt),yes(q.clockValid),q.clientRawIndex,q.clientMinusAck,q.timestep);
    const auto total=d.normalEntries+d.gradualEntries;bool comma=false;
    const auto first=total>TraceCount?total-TraceCount+1:1;
    for(uint64_t ordinal=first;ordinal<=total;++ordinal){
        const auto& r=d.ackTrace[(ordinal-1)%TraceCount];if(r.ordinal!=ordinal)continue;
        std::fprintf(output,"%s{\"ordinal\":%llu,\"at_ms\":%llu,\"kind\":%u,\"event_id\":%d,\"raw_index\":%u,\"move_gap_ms\":%llu,\"returned\":%s,\"returned_at_ms\":%llu,\"before_count\":%d,\"after_count\":%d,\"last_ack_before\":%u,\"last_ack_after\":%u,\"native_last_ack_matches_requested\":%s}",
            comma?",":"",static_cast<unsigned long long>(r.ordinal),static_cast<unsigned long long>(r.atMs),unsigned(r.kind),r.eventId,r.rawIndex,static_cast<unsigned long long>(r.moveGapMs),yes(r.returned),static_cast<unsigned long long>(r.returnedAtMs),r.beforeCount,r.afterCount,r.lastAckBefore,r.lastAckAfter,yes(r.lastAckMatchesRequested));comma=true;
    }
    std::fprintf(output,"],\"restored_tables\":%llu,\"retained_published_tables\":%llu,\"retired_weak_unavailable\":%llu,\"unobserved_parent_normal_forwards\":%llu,\"unobserved_parent_gradual_forwards\":%llu}",
        d.restoredTables,d.retainedPublishedTables,d.retiredWeakUnavailable,d.unobservedNormalForwards,d.unobservedGradualForwards);
}
}
