#pragma once
#include "local_server_recovery.h"
#include "local_server_client.h"
#include "local_server_remote_render_probe.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>

// Actual client Native UFunction boundary only. Incoming object/FFrame/result
// pointers are forwarded unchanged exactly once. No incoming frame is read,
// copied, reset, advanced or decoded. This is not a replacement heal producer.
namespace local_server_heal_observer {
using Weak=original_inventory_slots::WeakIdentity;
using Exec=void(*)(void*,void*,void*);
inline constexpr uintptr_t OriginalExecRva=0x6704dc0;
struct Sample {
    bool valid=false,alive=false,borrowedIdentities=false;uint8_t role=255;
    void *damageable=nullptr,*pawn=nullptr,*ps=nullptr;Weak damageableId{},pawnId{},psId{};
    int32_t playerId=-1,respawn=-1;float health=0,shield=0,maximumHealth=0,maximumShield=0;
    uint64_t sampledAt=0;
};
struct Receipt {
    uint64_t sequence=0;Sample before{},after{};
    bool sameLife=false,healthIncreased=false,shieldIncreased=false,afterMaximum=false;
};
inline bool Near(float a,float b){return std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<.01f;}
inline bool SameLife(const Sample& a,const Sample& b){
    return a.valid&&b.valid&&a.damageable==b.damageable&&a.pawn==b.pawn&&a.ps==b.ps
        &&original_inventory_slots::SameIdentity(a.damageableId,b.damageableId)
        &&original_inventory_slots::SameIdentity(a.pawnId,b.pawnId)&&original_inventory_slots::SameIdentity(a.psId,b.psId)
        &&a.playerId==b.playerId&&a.respawn==b.respawn&&a.role==b.role;
}
inline Receipt Evaluate(uint64_t sequence,const Sample& before,const Sample& after){
    Receipt r{sequence,before,after};r.sameLife=SameLife(before,after);
    if(r.sameLife&&before.alive&&after.alive){
        r.healthIncreased=after.health>before.health+.001f;r.shieldIncreased=after.shield>before.shield+.001f;
        r.afterMaximum=Near(after.health,after.maximumHealth)&&Near(after.shield,after.maximumShield);
    }return r;
}
struct State {
    bool initialized=false,installed=false,published=false,closed=false,closing=false,busy=false;
    bool profilesVerified=false,metadataVerified=false,restored=false,retained=false;
    DWORD gameThread=0;unsigned error=0;const char* stage="client_heal_observer_not_initialized";
    void *world=nullptr,*driver=nullptr,*connection=nullptr,*controller=nullptr,*function=nullptr;
    Weak worldId{},driverId{},connectionId{},controllerId{},functionId{};
    uint32_t flags=0;uint16_t bytes=0;uintptr_t execRva=0;
    uint64_t weakCaptureCalls=0,borrowedPawnStatePairs=0,originalAggregateQueries=0;
    uint64_t installWrites=0,restoreWrites=0,closeCalls=0,receipts=0,stableReceipts=0;
    uint64_t role1Receipts=0,role2Receipts=0,healthIncreases=0,shieldIncreases=0,fullAfterIncreases=0,invalidSamples=0;
    uint64_t sampleFaults=0,beforeSampleFaults=0,afterSampleFaults=0;uint32_t lastSampleException=0;
};
inline State state;
inline Exec original=nullptr; // Immutable after the first successful publication.
inline std::array<Receipt,64> receipts{};
inline volatile LONG64 entries=0,forwards=0,completed=0,bypassed=0,reentrant=0;
struct Snapshot {State state;std::array<Receipt,64> receipts;uint64_t entries=0,forwards=0,completed=0,bypassed=0,reentrant=0;};
inline Snapshot cached{};
inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline const State& GetState(){return state;}
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void* object,const Weak& id){return object&&original_inventory_slots::ResolveIdentity(id)==object;}
inline bool Capture(void* object,Weak& id){
    ++state.weakCaptureCalls;
    // This normal native weak capture MAY allocate a missing object serial.
    // Diagnostics count calls and never label this as zero native-object writes.
    return object&&original_inventory_slots::CaptureIdentity(object,id)&&Live(object,id);
}
inline uint64_t Counter(volatile LONG64& n){return static_cast<uint64_t>(InterlockedCompareExchange64(&n,0,0));}
inline void Cache(){
    AcquireSRWLockExclusive(&cacheLock);cached={state,receipts,Counter(entries),Counter(forwards),Counter(completed),Counter(bypassed),Counter(reentrant)};
    ReleaseSRWLockExclusive(&cacheLock);
}
inline bool Fail(unsigned code,const char* why){state.error=code;state.stage=why;Cache();return false;}
inline bool Thread(){return engine::image&&state.gameThread==GetCurrentThreadId()&&original_players::Thread();}
inline bool Writable(void* slot){
    if(!slot||reinterpret_cast<uintptr_t>(slot)%alignof(void*))return false;MEMORY_BASIC_INFORMATION m{};
    if(!VirtualQuery(slot,&m,sizeof(m))||m.State!=MEM_COMMIT||m.Protect&(PAGE_GUARD|PAGE_NOACCESS))return false;
    const auto protection=m.Protect&0xffU;
    return (protection==PAGE_READWRITE||protection==PAGE_WRITECOPY||protection==PAGE_EXECUTE_READWRITE||protection==PAGE_EXECUTE_WRITECOPY)
        &&original_players::Readable(slot,sizeof(void*));
}
struct Profile {uint32_t rva,bytes;uint64_t fnv;};
inline constexpr Profile Profiles[]{
    {0x6704dc0,948,0x027cf4b95ff1ea4aULL},{0x6fa0940,492,0x87133ef1d39da2baULL},
    {0x1cdc850,1396,0x74e72e0971f89c0dULL},{0x3efc820,176,0x7f1113b3f79164b2ULL},
    {0x6fa0d40,362,0xfcdeecfa08ce4b93ULL},{0x1cf35b0,1571,0x40614eae57888396ULL},
    {0x1d4cfa0,1562,0xb050d7efb4cf1a25ULL},
};
inline bool ProfilesMatch(){
    for(const auto& p:Profiles){auto data=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(data,p.bytes))return false;uint64_t hash=14695981039346656037ULL;
        for(uint32_t n=0;n<p.bytes;++n)hash=(hash^data[n])*1099511628211ULL;
        if(hash!=p.fnv)return false;
    }return true;
}
inline bool ClientWorld(){
    return !state.closed&&!state.closing&&Live(state.world,state.worldId)&&Live(state.driver,state.driverId)
        &&Live(state.connection,state.connectionId)&&Live(state.controller,state.controllerId)
        &&Read<void*>(state.world,0x40)==state.driver&&Read<void*>(state.driver,0x178)==state.world
        &&Read<void*>(state.driver,0xc0)==state.connection&&Read<void*>(state.connection,0x60)==state.driver
        &&Read<void*>(state.connection,0x38)==state.controller&&Read<void*>(state.connection,0xa0)==state.controller
        &&Read<uint8_t>(state.connection,0x13c)==3&&Read<uint8_t>(state.controller,0x224)==2
        &&!Read<void*>(state.world,0x170)&&engine::function<int32_t(*)(void*)>(0x3efc820)(state.controller)==3;
}
inline bool Borrow(void* pawn,void* ps,Weak& pawnId,Weak& psId){
    const auto& local=local_server_client::GetState();
    if(local.world==state.world&&local.pawn==pawn&&local.playerState==ps){pawnId=local.pawnId;psId=local.playerStateId;}
    else {
        bool found=false;AcquireSRWLockShared(&local_server_remote_render_probe::cacheLock);
        const auto& render=local_server_remote_render_probe::cached;
        if(render.world==state.world)for(const auto& row:render.rows)if(row.pawn==pawn&&row.playerState==ps){
            pawnId={row.pawnId.index,row.pawnId.serial};psId={row.playerStateId.index,row.playerStateId.serial};found=true;break;
        }ReleaseSRWLockShared(&local_server_remote_render_probe::cacheLock);if(!found)return false;
    }
    if(!Live(pawn,pawnId)||!Live(ps,psId))return false;++state.borrowedPawnStatePairs;return true;
}
inline bool Values(Sample& sample){
    // These are the exact queries made by native Character.GetHealth/GetShield.
    // The original getter may refresh its own normal aggregate query cache.
    using Query=void*(*)(void*,uint8_t,bool);
    const auto query=engine::function<Query>(0x6fa0d40);
    auto hp=query(sample.damageable,0,false);++state.originalAggregateQueries;
    auto armor=query(sample.damageable,1,false);++state.originalAggregateQueries;
    if(hp!=static_cast<uint8_t*>(sample.damageable)+0x200||armor!=static_cast<uint8_t*>(sample.damageable)+0x208)return false;
    sample.alive=Read<uint8_t>(sample.damageable,0x1f9)!=0;
    sample.health=sample.alive?Read<float>(hp):0;sample.maximumHealth=Read<float>(hp,4);
    sample.shield=Read<float>(armor);sample.maximumShield=Read<float>(armor,4);
    sample.playerId=Read<int32_t>(sample.ps,0x468);sample.respawn=Read<int32_t>(sample.pawn,0x143c);
    sample.role=Read<uint8_t>(sample.pawn,0x224);sample.sampledAt=GetTickCount64();
    return sample.playerId>=0&&sample.respawn>=0&&(sample.role==1||sample.role==2)
        &&std::isfinite(sample.health)&&sample.health>=0&&std::isfinite(sample.maximumHealth)&&sample.maximumHealth>0
        &&std::isfinite(sample.shield)&&sample.shield>=0&&std::isfinite(sample.maximumShield)&&sample.maximumShield>=0;
}
inline bool Before(void* object,Sample& sample){
    sample={};if(!ClientWorld()||!original_players::IsA(object,original_damage::DamageableClass))return false;
    sample.damageable=object;sample.pawn=Read<void*>(object,0xc8);sample.ps=Read<void*>(sample.pawn,0x488);
    if(!Borrow(sample.pawn,sample.ps,sample.pawnId,sample.psId)||!Capture(object,sample.damageableId)
       ||!original_players::IsA(sample.pawn,original_players::ShooterCharacterClass)
       ||!original_players::IsA(sample.ps,original_players::ShooterStateClass)
       ||Read<void*>(sample.pawn,0xc70)!=object||Read<void*>(sample.ps,0xbc8)!=sample.pawn
       ||original_effects::GetWorld(sample.pawn)!=state.world
       ||engine::function<int32_t(*)(void*)>(0x3efc820)(sample.pawn)!=3)return false;
    sample.borrowedIdentities=true;sample.valid=Values(sample);return sample.valid;
}
inline bool After(const Sample& before,Sample& after){
    after={};
    if(!before.valid||!ClientWorld()||!Live(before.damageable,before.damageableId)
       ||!Live(before.pawn,before.pawnId)||!Live(before.ps,before.psId))return false;
    after=before;after.valid=false;
    if(Read<void*>(after.damageable,0xc8)!=after.pawn||Read<void*>(after.pawn,0xc70)!=after.damageable
       ||Read<void*>(after.pawn,0x488)!=after.ps||Read<void*>(after.ps,0xbc8)!=after.pawn)return false;
    after.valid=Values(after);return after.valid;
}
using Sampling=bool(*)(void*);
inline LONG SampleFault(unsigned phase,unsigned exceptionCode){
    ++state.sampleFaults;state.lastSampleException=exceptionCode;
    if(phase==1)++state.beforeSampleFaults;else if(phase==2)++state.afterSampleFaults;
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool ProtectedSample(unsigned phase,Sampling query,void* opaque){
    // Only diagnostic sampling is inside this SEH boundary. The actual original
    // heal exec is never called from a sampling function or this catch scope.
    __try{return query(opaque);}
    __except(local_server_heal_observer::SampleFault(phase,GetExceptionCode())){return false;}
}
struct SampleCall {void* object=nullptr;const Sample* before=nullptr;Sample* output=nullptr;};
inline bool BeforeQuery(void* opaque){auto& call=*static_cast<SampleCall*>(opaque);return Before(call.object,*call.output);}
inline bool AfterQuery(void* opaque){auto& call=*static_cast<SampleCall*>(opaque);return After(*call.before,*call.output);}
inline bool SafeBefore(void* object,Sample& output){
    SampleCall call{object,nullptr,&output};const bool okay=ProtectedSample(1,&BeforeQuery,&call);if(!okay)output.valid=false;return okay;
}
inline bool SafeAfter(const Sample& before,Sample& output){
    SampleCall call{nullptr,&before,&output};const bool okay=ProtectedSample(2,&AfterQuery,&call);if(!okay)output.valid=false;return okay;
}
inline void ForwardOriginal(void* object,void* frame,void* result){
    InterlockedIncrement64(&forwards);original(object,frame,result);InterlockedIncrement64(&completed);
}
using Work=void(*)(void*);
inline void Guarded(bool& busy,Work work,void* opaque){
    const bool previous=busy;busy=true;
    __try{work(opaque);}
    __finally{busy=previous;}
}
struct Call {void *object=nullptr,*frame=nullptr,*result=nullptr;Receipt receipt{};};
using BeforeSampler=bool(*)(void*,Sample&);
using AfterSampler=bool(*)(const Sample&,Sample&);
inline void RunObserved(Call& call,BeforeSampler sampleBefore,AfterSampler sampleAfter){
    Sample before{},after{};const bool beforeValid=sampleBefore(call.object,before);
    ForwardOriginal(call.object,call.frame,call.result); // Exactly one untouched original exec.
    const bool afterValid=beforeValid&&sampleAfter(before,after);
    call.receipt=Evaluate(++state.receipts,before,after);
    if(!beforeValid||!afterValid)++state.invalidSamples;
}
inline void ObservedCall(void* opaque){RunObserved(*static_cast<Call*>(opaque),&SafeBefore,&SafeAfter);}
inline void Observe(void* object,void* frame,void* result){
    InterlockedIncrement64(&entries);
    // Immutable original remains usable for late/foreign-thread callbacks,
    // including after Close or expiry of the cached UFunction generation.
    if(!Thread()||!state.installed||state.closed||state.closing||state.busy
       ||!Live(state.function,state.functionId)||Read<Exec>(state.function,0xe0)!=&Observe){
        InterlockedIncrement64(&bypassed);if(Thread()&&state.busy)InterlockedIncrement64(&reentrant);
        ForwardOriginal(object,frame,result);return;
    }
    Call call{object,frame,result};Guarded(state.busy,&ObservedCall,&call);
    const auto& receipt=call.receipt;receipts[(receipt.sequence-1)%receipts.size()]=receipt;
    if(receipt.sameLife){++state.stableReceipts;
        if(receipt.before.role==1)++state.role1Receipts;else if(receipt.before.role==2)++state.role2Receipts;
        if(receipt.healthIncreased)++state.healthIncreases;if(receipt.shieldIncreased)++state.shieldIncreases;
        if(receipt.afterMaximum&&(receipt.healthIncreased||receipt.shieldIncreased))++state.fullAfterIncreases;
    }state.stage="client_heal_original_native_exec_observed";Cache();
}
inline bool Metadata(){
    const auto fn=engine::Find(L"/Script/ShooterGame.DamageableComponent.MulticastNotifyHeal");
    state.function=fn;state.bytes=Read<uint16_t>(fn,0xbe);state.flags=Read<uint32_t>(fn,0xb8);
    const auto address=Read<uintptr_t>(fn,0xe0);state.execRva=address>=engine::image?address-engine::image:0;
    if(!fn||Read<void*>(fn,0x28)!=engine::Class(original_damage::DamageableClass)||state.bytes!=48
       ||state.flags!=0x44cc1||state.execRva!=OriginalExecRva)return false;
    using local_server_recovery::Field;original_jett_priming::PropertyInfo array{};
    if(!Field(fn,L"HealTaken",0,4)||!Field(fn,L"LifeChangeBySection",8,16,&array)
       ||!Field(fn,L"EventInstigator",24,8)||!Field(fn,L"EventInstigatorPawn",32,8)||!Field(fn,L"HealCauser",40,8))return false;
    auto inner=Read<void*>(array.field,0x80),type=Read<void*>(inner,0x78);
    return inner&&Read<int32_t>(inner,0x34)==24&&Read<int32_t>(inner,0x30)==1
        &&type==engine::Find(L"/Script/ShooterGame.LifeChangeEvent")&&Capture(fn,state.functionId);
}
inline bool InitClient(void* world){
    if(state.initialized)return !state.closed&&!state.closing&&state.world==world&&Thread()
        &&state.installed&&Live(state.function,state.functionId)&&Read<Exec>(state.function,0xe0)==&Observe;
    const auto& client=local_server_client::GetState();
    if(!engine::image||!original_players::Thread()||!world||client.world!=world||!client.nativeClient||!client.localPairVerified)
        return Fail(7290,"client_heal_observer_waiting_real_client_world");
    state.gameThread=GetCurrentThreadId();state.world=world;state.worldId=client.worldId;
    state.controller=client.controller;state.controllerId=client.controllerId;state.driver=client.driver;state.connection=client.connection;
    if(!Capture(state.driver,state.driverId)||!Capture(state.connection,state.connectionId)||!ClientWorld())
        return Fail(7291,"client_heal_observer_real_client_connection_identity_missing");
    if(!ProfilesMatch())return Fail(7292,"client_heal_observer_original_source_profile_mismatch");state.profilesVerified=true;
    if(!Metadata())return Fail(7293,"client_heal_observer_original_native_heal_metadata_mismatch");state.metadataVerified=true;
    auto address=static_cast<uint8_t*>(state.function)+0xe0;auto slot=reinterpret_cast<void* volatile*>(address);
    if(!Writable(address))return Fail(7294,"client_heal_observer_original_exec_slot_not_writable");
    const auto parent=engine::function<Exec>(OriginalExecRva);
    if(original&&original!=parent)return Fail(7295,"client_heal_observer_original_parent_changed");original=parent;
    if(InterlockedCompareExchangePointer(slot,reinterpret_cast<void*>(&Observe),reinterpret_cast<void*>(parent))!=reinterpret_cast<void*>(parent))
        return Fail(7296,"client_heal_observer_original_exec_publication_conflict");
    ++state.installWrites;state.initialized=true;state.installed=true;state.published=true;state.error=0;
    state.stage="client_heal_native_exec_observer_installed_no_receive_claim";Cache();return true;
}
inline bool Close(){
    ++state.closeCalls;if(!state.initialized){state.closed=true;Cache();return true;}
    if(!Thread())return Fail(7297,"client_heal_observer_close_requires_original_game_thread");
    state.closing=true;if(state.busy){state.stage="client_heal_observer_close_waiting_original_exec_return";Cache();return false;}
    if(!Live(state.function,state.functionId)){
        state.retained=true;state.installed=false;state.closed=true;
        state.stage="client_heal_observer_expired_function_not_dereferenced_static_forwarder_retained";Cache();return true;
    }
    auto address=static_cast<uint8_t*>(state.function)+0xe0;const auto slot=reinterpret_cast<void* volatile*>(address);
    auto actual=Read<Exec>(state.function,0xe0);
    if(actual==original){state.restored=true;state.published=false;}
    else if(actual==&Observe){
        if(!Writable(address))return Fail(7299,"client_heal_observer_original_exec_slot_not_writable_at_close");
        if(InterlockedCompareExchangePointer(slot,reinterpret_cast<void*>(original),reinterpret_cast<void*>(&Observe))!=reinterpret_cast<void*>(&Observe))
            return Fail(7298,"client_heal_observer_foreign_exec_layer_not_overwritten");
        ++state.restoreWrites;state.restored=true;state.published=false;
    }else return Fail(7298,"client_heal_observer_foreign_exec_layer_not_overwritten");
    state.installed=false;state.closed=true;state.stage="client_heal_observer_original_native_exec_restored";Cache();return true;
}
inline std::string Json(){
    AcquireSRWLockShared(&cacheLock);const auto value=cached;ReleaseSRWLockShared(&cacheLock);
    const auto& s=value.state;local_server_match::Json out;const auto b=[](bool v){return v?"true":"false";};
    const auto u=[](uint64_t v){return static_cast<unsigned long long>(v);};
    const auto number=[&](float v){if(std::isfinite(v))out.Add("%.9g",static_cast<double>(v));else out.Add("null");};
    out.Add("{\"stage\":\"%s\",\"error\":%u,\"installed\":%s,\"closed\":%s,\"original_exec_restored\":%s,\"static_forwarder_retained\":%s,\"profiles_verified\":%s,\"metadata_verified\":%s,\"entries\":%llu,\"original_forward_attempts\":%llu,\"original_forward_completed\":%llu,\"bypassed\":%llu,\"reentrant\":%llu,\"receipts\":%llu,\"stable_receipts\":%llu,\"role1_receipts\":%llu,\"role2_receipts\":%llu,\"health_increases\":%llu,\"shield_increases\":%llu,\"maximum_after_increases\":%llu,\"invalid_samples\":%llu,\"weak_capture_calls\":%llu,\"borrowed_pawn_state_pairs\":%llu,\"original_aggregate_queries\":%llu,\"exec_slot_install_writes\":%llu,\"exec_slot_restore_writes\":%llu,\"last_receipts\":[",
        s.stage,s.error,b(s.installed),b(s.closed),b(s.restored),b(s.retained),b(s.profilesVerified),b(s.metadataVerified),u(value.entries),u(value.forwards),u(value.completed),u(value.bypassed),u(value.reentrant),u(s.receipts),u(s.stableReceipts),u(s.role1Receipts),u(s.role2Receipts),u(s.healthIncreases),u(s.shieldIncreases),u(s.fullAfterIncreases),u(s.invalidSamples),u(s.weakCaptureCalls),u(s.borrowedPawnStatePairs),u(s.originalAggregateQueries),u(s.installWrites),u(s.restoreWrites));
    bool comma=false;for(const auto& r:value.receipts)if(r.sequence){const auto& a=r.before;const auto& z=r.after;
        out.Add("%s{\"sequence\":%llu,\"player_id\":%d,\"respawn_number\":%d,\"role\":%u,\"pawn\":\"%p\",\"player_state\":\"%p\",\"damageable\":\"%p\",\"pawn_weak\":[%d,%d],\"state_weak\":[%d,%d],\"damageable_weak\":[%d,%d],\"before_at_ms\":%llu,\"after_at_ms\":%llu,\"before_valid\":%s,\"after_valid\":%s,\"alive_before\":%s,\"alive_after\":%s,\"same_life\":%s,\"health_increased\":%s,\"shield_increased\":%s,\"after_at_maximum\":%s,\"health_before\":",
            comma?",":"",u(r.sequence),a.playerId,a.respawn,unsigned(a.role),a.pawn,a.ps,a.damageable,a.pawnId.index,a.pawnId.serial,a.psId.index,a.psId.serial,a.damageableId.index,a.damageableId.serial,u(a.sampledAt),u(z.sampledAt),b(a.valid),b(z.valid),b(a.alive),b(z.alive),b(r.sameLife),b(r.healthIncreased),b(r.shieldIncreased),b(r.afterMaximum));comma=true;
        number(a.health);out.Add(",\"health_after\":");number(z.health);out.Add(",\"shield_before\":");number(a.shield);out.Add(",\"shield_after\":");number(z.shield);
        out.Add(",\"maximum_health_after\":");number(z.maximumHealth);out.Add(",\"maximum_shield_after\":");number(z.maximumShield);out.Add("}");
    }
    out.Add("],\"original_function\":\"%p\",\"function_weak\":[%d,%d],\"function_flags\":%u,\"parameter_bytes\":%u,\"original_exec_rva\":%llu,\"sample_faults\":%llu,\"before_sample_faults\":%llu,\"after_sample_faults\":%llu,\"last_sample_exception\":%u,\"incoming_frame_read_or_decode_calls\":0,\"incoming_frame_writes\":0,\"synthetic_heal_calls\":0,\"direct_health_armor_role_clock_writes\":0,\"function_flag_writes\":0,\"weak_serial_allocation_possible\":true,\"weak_serial_allocations\":null,\"original_query_cache_may_refresh\":true,\"receipt_origin\":\"actual_native_exec_on_client_not_payload_or_network_stack_decode\",\"paired_host_event_correlation_verified\":false,\"runtime_acceptance_verified\":false}",s.function,s.functionId.index,s.functionId.serial,s.flags,unsigned(s.bytes),u(s.execRva),u(s.sampleFaults),u(s.beforeSampleFaults),u(s.afterSampleFaults),s.lastSampleException);
    return out.value;
}
} // namespace local_server_heal_observer
