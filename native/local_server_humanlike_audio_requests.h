#pragma once
#include "local_server_match.h"
#include "humanlike_audio_observation_core.h"
#include "humanlike_audio_source_profiles.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <string>

// Three fixed Native UFunction boundaries, not GNatives/a VM hook. Incoming
// object/FFrame/result are forwarded unchanged once. No original sound event,
// listener, locality, attenuation, role, volume or function flags are changed.
namespace local_server_humanlike_audio_requests {
namespace core=humanlike_audio_observation;
namespace events=humanlike_bot_native_events;
using Weak=original_inventory_slots::WeakIdentity;
using Exec=void(*)(void*,void*,void*);
inline constexpr size_t FunctionCount=3,ReceiptCapacity=128;
inline constexpr uint32_t NativeFlags=0x04020401;
inline constexpr uintptr_t ExecRva[FunctionCount]{0x685cac0,0x685be40,0x685be60};
inline constexpr const wchar_t* Paths[FunctionCount]{
    L"/Script/ShooterGame.FootstepsComponent.PlayFootstepSound",
    L"/Script/ShooterGame.FootstepsComponent.PlayJumpLandingSound",
    L"/Script/ShooterGame.FootstepsComponent.PlayJumpTakeoffSound"};
struct Hook {
    void* function=nullptr;Weak identity{};Exec original=nullptr;
    bool installed=false,publishedEver=false,restored=false,retained=false;
};
struct State {
    bool initialized=false,closed=false,closing=false,busy=false,profilesVerified=false,metadataVerified=false;
    DWORD gameThread=0;void* world=nullptr,*containerClass=nullptr;Weak worldId{},containerClassId{};
    unsigned error=0;const char* stage="humanlike_audio_requests_not_initialized";
    uint64_t weakCaptureCalls=0,installWrites=0,restoreWrites=0,receipts=0,eligible=0,published=0;
    uint64_t invalidSamples=0,sampleFaults=0,beforeFaults=0,afterFaults=0,ignoredTakeoff=0;
    uint32_t lastSampleException=0;
};
inline State state{};
inline std::array<Hook,FunctionCount> hooks{};
inline std::array<core::Receipt,ReceiptCapacity> receipts{};
inline volatile LONG64 entries=0,forwards=0,completed=0,bypassed=0,reentrant=0;
struct Snapshot {
    State state{};std::array<core::Receipt,ReceiptCapacity> receipts{};
    uint64_t entries=0,forwards=0,completed=0,bypassed=0,reentrant=0;
    std::array<bool,FunctionCount> installed{},restored{},retained{};
};
inline Snapshot cached{};inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline const State& GetState(){return state;}
template<class T>inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void* object,const Weak&id){return object&&original_inventory_slots::ResolveIdentity(id)==object;}
inline bool Capture(void* object,Weak&id){
    ++state.weakCaptureCalls;
    // Normal original weak bookkeeping may allocate a missing object serial.
    return object&&original_inventory_slots::CaptureIdentity(object,id)&&Live(object,id);
}
inline uint64_t Counter(volatile LONG64&v){return static_cast<uint64_t>(InterlockedCompareExchange64(&v,0,0));}
inline void Cache(){
    AcquireSRWLockExclusive(&cacheLock);
    cached.state=state;cached.receipts=receipts;cached.entries=Counter(entries);cached.forwards=Counter(forwards);
    cached.completed=Counter(completed);cached.bypassed=Counter(bypassed);cached.reentrant=Counter(reentrant);
    for(size_t n=0;n<FunctionCount;++n){cached.installed[n]=hooks[n].installed;cached.restored[n]=hooks[n].restored;cached.retained[n]=hooks[n].retained;}
    ReleaseSRWLockExclusive(&cacheLock);
}
inline bool Fail(unsigned code,const char*why){state.error=code;state.stage=why;Cache();return false;}
inline bool Thread(){return engine::image&&state.gameThread==GetCurrentThreadId()&&original_players::Thread();}
inline bool Context(){
    const auto&m=local_server_match::GetState();
    return Thread()&&state.initialized&&!state.closed&&!state.closing&&!state.error&&m.initialized&&!m.restartRequired
        &&m.world==state.world&&original_players::world==state.world&&Live(state.world,state.worldId);
}
inline bool Writable(void*slot){
    if(!slot||reinterpret_cast<uintptr_t>(slot)%alignof(void*))return false;
    MEMORY_BASIC_INFORMATION memory{};
    if(!VirtualQuery(slot,&memory,sizeof(memory))||memory.State!=MEM_COMMIT||memory.Protect&(PAGE_GUARD|PAGE_NOACCESS))return false;
    const auto p=memory.Protect&0xffU;
    return (p==PAGE_READWRITE||p==PAGE_WRITECOPY||p==PAGE_EXECUTE_READWRITE||p==PAGE_EXECUTE_WRITECOPY)
        &&original_players::Readable(slot,sizeof(void*));
}
inline bool ProfilesMatch(){
    for(const auto&p:humanlike_audio_source_profiles::Profiles){
        auto bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes))return false;uint64_t hash=14695981039346656037ULL;
        for(uint32_t n=0;n<p.bytes;++n)hash=(hash^bytes[n])*1099511628211ULL;
        if(hash!=p.fnv)return false;
    }return true;
}
inline core::Weak Convert(const Weak&id){return {id.index,id.serial};}
inline bool GroundOrigin(void*pawn,void*world,core::Sample&sample){
    // Exact original6f33150 branches. This is a ground request origin, not a
    // skeletal foot socket or a screen/camera position. No velocity threshold.
    auto movement=engine::function<void*(*)(void*)>(0x787b6c0)(pawn);
    if(!movement||Read<void*>(pawn,0x4f8)!=movement)return false;
    engine::Vec point{};
    const bool alternate=(Read<uint32_t>(movement,0x478)&3U)==3U
        &&!engine::function<bool(*)(void*)>(0x4cd7160)(world);
    if(alternate){
        if(!original_players::Read(movement,0x4b0,point))return false;
        sample.positionSource=core::PositionSource::OriginalGroundMovement;
    }else{
        auto root=Read<void*>(pawn,0x290),capsule=Read<void*>(pawn,0x500);
        if(!root||!capsule||!original_players::Read(root,0x320,point))return false;
        const float halfHeight=Read<float>(capsule,0x6d8);
        if(!std::isfinite(halfHeight)||halfHeight<=0)return false;
        point.z-=static_cast<double>(halfHeight);sample.positionSource=core::PositionSource::OriginalGroundRoot;
    }
    // Read the actual pinned original constant; do not substitute a guessed
    // actor origin for the body-authored +20cm request offset.
    point.z+=Read<double>(reinterpret_cast<void*>(engine::image+0xc8d1f10));
    if(!original_players::Finite(point))return false;
    sample.originalPosition={point.x,point.y,point.z};
    sample.position={static_cast<float>(point.x),static_cast<float>(point.y),static_cast<float>(point.z)};
    return events::Finite(sample.position);
}
inline bool SampleSource(size_t kind,void*object,core::Sample&out){
    out={};if(kind>=FunctionCount||!Context()||!original_players::IsA(object,0x685b1f0))return false;
    auto pawn=Read<void*>(object,0x2f0);
    if(!pawn||Read<void*>(object,0xc8)!=pawn)return false;
    const local_server_match::AuthorityPlayer*player=nullptr;size_t index=0;
    for(size_t n=0;n<local_server_match::MaxPlayers;++n){const auto p=local_server_match::Player(n);
        if(p&&p->pawn==pawn){if(player)return false;player=p;index=n;}}
    if(!player||!player->alive||!player->epoch||!Live(pawn,player->pawnIdentity)
       ||!original_players::IsA(pawn,original_players::ShooterCharacterClass))return false;
    using GetWorld=void*(*)(void*);
    const auto objectWorld=Read<GetWorld>(Read<void*>(object),0x1b8),pawnWorld=Read<GetWorld>(Read<void*>(pawn),0x1b8);
    if(objectWorld!=engine::function<GetWorld>(0x2a64ee0)||pawnWorld!=engine::function<GetWorld>(0x3ed83b0)
       ||objectWorld(object)!=state.world||pawnWorld(pawn)!=state.world)return false;
    const uintptr_t assetOffset=kind==0?0x1d0:kind==1?0x230:0x200;
    auto asset=Read<void*>(object,assetOffset);Weak componentId{},assetId{};
    if(!asset||!Capture(object,componentId)||!Capture(asset,assetId))return false;
    // This is the original selected FXC class object, held by the component.
    // Its bytes are not decoded as an audio event or a successful PlayingID.
    if(!Live(state.containerClass,state.containerClassId)||!original_players::Derived(asset,state.containerClass))return false;
    out.request=static_cast<core::Request>(kind);
    out.subject={static_cast<uint32_t>(index),player->epoch,reinterpret_cast<uintptr_t>(pawn),player->pawnIdentity.index,player->pawnIdentity.serial};
    out.component=Convert(componentId);out.world=Convert(state.worldId);out.asset=Convert(assetId);
    out.componentObject=reinterpret_cast<uintptr_t>(object);out.worldObject=reinterpret_cast<uintptr_t>(state.world);out.assetObject=reinterpret_cast<uintptr_t>(asset);
    out.atMatchSeconds=local_server_match::Now();
    out.nativeWorldTimeKnown=original_players::Read(state.world,0x740,out.nativeWorldSeconds)&&std::isfinite(out.nativeWorldSeconds);
    if(!GroundOrigin(pawn,state.world,out))return false;
    out.sourceProfiles=state.profilesVerified;out.metadata=state.metadataVerified;
    out.ownedWorld=true;out.originalAsset=true;out.valid=true;return core::Good(out);
}
struct SampleCall {size_t kind=0;void*object=nullptr;core::Sample*output=nullptr;};
inline LONG SampleFault(unsigned phase,unsigned code){
    ++state.sampleFaults;state.lastSampleException=code;if(phase==1)++state.beforeFaults;else ++state.afterFaults;
    return EXCEPTION_EXECUTE_HANDLER;
}
using Sampling=bool(*)(void*);
inline bool ProtectedSample(unsigned phase,Sampling query,void*opaque){
    __try{return query(opaque);}
    __except(SampleFault(phase,GetExceptionCode())){return false;}
}
inline bool SampleQuery(void*opaque){auto&c=*static_cast<SampleCall*>(opaque);return SampleSource(c.kind,c.object,*c.output);}
inline bool SafeSample(unsigned phase,size_t kind,void*object,core::Sample&out){
    SampleCall call{kind,object,&out};const bool okay=ProtectedSample(phase,&SampleQuery,&call);
    if(!okay)out.valid=false;return okay;
}
inline void Forward(size_t kind,void*object,void*frame,void*result){
    InterlockedIncrement64(&forwards);hooks[kind].original(object,frame,result);InterlockedIncrement64(&completed);
}
struct Call {size_t kind=0;void*object=nullptr,*frame=nullptr,*result=nullptr;};
inline void Run(Call&call){
    core::Sample before{},after{};const bool beforeValid=SafeSample(1,call.kind,call.object,before);
    Forward(call.kind,call.object,call.frame,call.result); // No native exception catch/retry.
    const bool afterValid=beforeValid&&SafeSample(2,call.kind,call.object,after);
    auto receipt=core::Evaluate(++state.receipts,before,after,true);
    receipts[(receipt.occurrence-1)%ReceiptCapacity]=receipt;
    if(!beforeValid||!afterValid)++state.invalidSamples;
    if(beforeValid&&call.kind==2)++state.ignoredTakeoff;
    if(receipt.requestEligible){++state.eligible;if(events::PublishAudio(receipt.event))++state.published;}
}
inline void Guarded(Call&call){const bool previous=state.busy;state.busy=true;__try{Run(call);}__finally{state.busy=previous;}}
inline void Observe(size_t kind,void*object,void*frame,void*result);
inline void ObserveFootstep(void*object,void*frame,void*result){Observe(0,object,frame,result);}
inline void ObserveLanding(void*object,void*frame,void*result){Observe(1,object,frame,result);}
inline void ObserveTakeoff(void*object,void*frame,void*result){Observe(2,object,frame,result);}
inline constexpr Exec Observers[FunctionCount]{&ObserveFootstep,&ObserveLanding,&ObserveTakeoff};
inline void Observe(size_t kind,void*object,void*frame,void*result){
    InterlockedIncrement64(&entries);const auto&h=hooks[kind];
    if(!Thread()||!h.installed||state.closed||state.closing||state.busy
       ||!Live(h.function,h.identity)||Read<Exec>(h.function,0xe0)!=Observers[kind]){
        InterlockedIncrement64(&bypassed);if(Thread()&&state.busy)InterlockedIncrement64(&reentrant);
        Forward(kind,object,frame,result);return;
    }
    Call call{kind,object,frame,result};Guarded(call);
}
inline bool Metadata(){
    auto type=engine::Class(0x685b1f0);if(!type)return false;
    state.containerClass=engine::Find(L"/Script/ShooterGame.EffectContainer");
    if(!Capture(state.containerClass,state.containerClassId))return false;
    for(size_t n=0;n<FunctionCount;++n){auto&h=hooks[n];
        if(h.publishedEver)return false; // A published generation is never recycled.
        auto fn=engine::Find(Paths[n]);Weak id{};
        if(!fn||Read<void*>(fn,0x28)!=type||Read<uint16_t>(fn,0xbe)!=0
           ||Read<uint32_t>(fn,0xb8)!=NativeFlags||Read<Exec>(fn,0xe0)!=engine::function<Exec>(ExecRva[n])
           ||!Capture(fn,id))return false;
        h.function=fn;h.identity=id;h.original=engine::function<Exec>(ExecRva[n]);
    }return true;
}
inline bool Close();
inline bool InitHost(void*world){
    if(state.initialized)return Context()&&state.world==world;
    if(state.closed)return false;
    const auto&m=local_server_match::GetState();
    if(!world||!original_players::Thread()||!m.initialized||m.restartRequired||m.world!=world
       ||original_players::world!=world)return Fail(7950,"humanlike_audio_waiting_owned_host_world");
    state.gameThread=GetCurrentThreadId();state.world=world;
    if(!Capture(world,state.worldId))return Fail(7951,"humanlike_audio_world_identity_missing");
    if(!ProfilesMatch())return Fail(7952,"humanlike_audio_fixed_original_source_mismatch");state.profilesVerified=true;
    if(!Metadata())return Fail(7953,"humanlike_audio_exact_native_function_metadata_missing");state.metadataVerified=true;
    state.initialized=true;
    for(size_t n=0;n<FunctionCount;++n){auto&h=hooks[n];auto address=static_cast<uint8_t*>(h.function)+0xe0;
        if(!Writable(address)||InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(address),
             reinterpret_cast<void*>(Observers[n]),reinterpret_cast<void*>(h.original))!=reinterpret_cast<void*>(h.original)){
            Fail(7954,"humanlike_audio_native_exec_publication_conflict");Close();return false;
        }
        h.installed=h.publishedEver=true;++state.installWrites;
    }
    events::SetAudioAvailability(true,true);
    state.error=0;state.stage="humanlike_authored_ground_request_observers_installed_no_playback_claim";Cache();return true;
}
inline bool Poll(){
    if(!Context())return false;
    for(size_t n=0;n<FunctionCount;++n)if(hooks[n].installed&&(!Live(hooks[n].function,hooks[n].identity)
        ||Read<Exec>(hooks[n].function,0xe0)!=Observers[n])){
        events::SetAudioAvailability(false,false);return Fail(7955,"humanlike_audio_native_function_generation_or_dispatch_changed");}
    // BeginSession may legitimately have reset the own event diagnostics after
    // native world setup. Reflect this live installed capability without ever
    // creating a cue or an audio listener.
    events::SetAudioAvailability(events::diagnostics.installed,events::diagnostics.installed);
    Cache();return true;
}
inline bool Close(){
    if(state.closed)return true;
    if(!state.initialized){state.closed=true;events::SetAudioAvailability(false,false);Cache();return true;}
    if(!Thread())return Fail(7956,"humanlike_audio_close_requires_original_game_thread");
    state.closing=true;events::SetAudioAvailability(false,false);
    if(state.busy){state.stage="humanlike_audio_close_waiting_original_exec_return";Cache();return false;}
    for(size_t n=0;n<FunctionCount;++n){auto&h=hooks[n];if(!h.installed)continue;
        if(!Live(h.function,h.identity)){h.retained=true;h.installed=false;continue;}
        const auto actual=Read<Exec>(h.function,0xe0);
        if(actual==h.original){h.restored=true;h.installed=false;continue;}
        auto address=static_cast<uint8_t*>(h.function)+0xe0;
        if(actual!=Observers[n]||!Writable(address)||InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(address),
             reinterpret_cast<void*>(h.original),reinterpret_cast<void*>(Observers[n]))!=reinterpret_cast<void*>(Observers[n]))
            return Fail(7957,"humanlike_audio_foreign_exec_layer_preserved_at_close");
        ++state.restoreWrites;h.restored=true;h.installed=false;
    }
    state.closed=true;state.stage="humanlike_audio_native_exec_restored_static_late_forwarders_retained";Cache();return true;
}
inline std::string Json(){
    AcquireSRWLockShared(&cacheLock);const auto v=cached;ReleaseSRWLockShared(&cacheLock);
    const auto&s=v.state;local_server_match::Json out;const auto b=[](bool x){return x?"true":"false";};
    const auto u=[](uint64_t x){return static_cast<unsigned long long>(x);};
    out.Add("{\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"closed\":%s,\"profiles_verified\":%s,\"metadata_verified\":%s,\"entries\":%llu,\"original_forward_attempts\":%llu,\"original_forward_completed\":%llu,\"bypassed\":%llu,\"reentrant\":%llu,\"receipts\":%llu,\"eligible_requests\":%llu,\"published\":%llu,\"invalid_samples\":%llu,\"sample_faults\":%llu,\"exec_install_writes\":%llu,\"exec_restore_writes\":%llu,\"weak_capture_calls\":%llu,\"request_producer\":\"original_native_FootstepsComponent_Play_exec\",\"evidence\":\"authored_audio_request_logical_propagation\",\"actual_audio_post_available\":false,\"hardware_playback_verified\":false,\"incoming_frame_reads_or_writes\":0,\"function_flag_writes\":0,\"listener_or_audio_parameter_changes\":0,\"runtime_acceptance_verified\":false,\"last_receipts\":[",
        s.stage,s.error,b(s.initialized),b(s.closed),b(s.profilesVerified),b(s.metadataVerified),u(v.entries),u(v.forwards),u(v.completed),u(v.bypassed),u(v.reentrant),u(s.receipts),u(s.eligible),u(s.published),u(s.invalidSamples),u(s.sampleFaults),u(s.installWrites),u(s.restoreWrites),u(s.weakCaptureCalls));
    bool comma=false;for(const auto&r:v.receipts)if(r.occurrence){const auto&a=r.before;
        out.Add("%s{\"occurrence\":%llu,\"request\":%u,\"source_index\":%u,\"source_epoch\":%u,\"pawn_weak\":[%d,%d],\"world_weak\":[%d,%d],\"component_weak\":[%d,%d],\"asset_weak\":[%d,%d],\"original_completed\":%s,\"same_life\":%s,\"eligible\":%s,\"at_match_seconds\":%.17g,\"native_world_seconds_known\":%s,\"native_world_seconds_opaque\":%.17g,\"ground_origin_source\":%u,\"ground_origin_cm\":[%.17g,%.17g,%.17g],\"actual_post_observed\":false}",
            comma?",":"",u(r.occurrence),unsigned(a.request),a.subject.index,a.subject.epoch,a.subject.weakIndex,a.subject.weakSerial,a.world.index,a.world.serial,a.component.index,a.component.serial,a.asset.index,a.asset.serial,b(r.originalForwardCompleted),b(r.sameLife),b(r.requestEligible),a.atMatchSeconds,b(a.nativeWorldTimeKnown),a.nativeWorldTimeKnown?a.nativeWorldSeconds:0.,unsigned(a.positionSource),a.originalPosition.x,a.originalPosition.y,a.originalPosition.z);comma=true;
    }out.Add("]}");return out.value;
}
} // namespace local_server_humanlike_audio_requests
