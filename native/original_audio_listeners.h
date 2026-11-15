#pragma once
#include "original_inventory_slots.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

// Pinned Ak plug-in registry, not AresGetActiveListenerPlayerController.
// Sample is read-only. Reconcile explicitly invokes the complete original
// RemoveDefaultListener, and only for a same-world PC-owned camera with no
// actual UPlayer. It never edits a set, a listener flag, a network/locality
// predicate, an image page, a volume or a transform.
namespace original_audio_listeners {
inline constexpr uintptr_t ModulePointerRva=0xda52738,RemoveDefaultRva=0x596cdf0;
inline constexpr size_t MaxSlots=1024,MaxListeners=256,History=64;
using Weak=original_inventory_slots::WeakIdentity;
enum class Kind:uint8_t { Unknown,Own,NonLocalPC,Special,OtherWorld,Expired };
struct SetHeader {
    void* data=nullptr;
    int32_t slots=0,capacity=0;
    std::array<uint32_t,4> inlineBits{};
    const uint32_t* heapBits=nullptr;
    int32_t bitCount=0,bitCapacity=0,firstFree=0,freeCount=0;
    int32_t inlineHash=0,pad=0;
    void* heapHash=nullptr;
    int32_t hashSize=0,tail=0;
};
static_assert(sizeof(SetHeader)==0x50);
static_assert(offsetof(SetHeader,heapBits)==0x20&&offsetof(SetHeader,bitCount)==0x28
  &&offsetof(SetHeader,freeCount)==0x34&&offsetof(SetHeader,hashSize)==0x48);
struct Row {
    Weak identity{};
    int32_t slot=-1;
    Kind kind=Kind::Unknown;
    bool akClass=false,cameraClass=false,controllerClass=false,sameWorld=false;
    bool cameraPair=false,noPlayer=false,actualLocal=false;
    uint8_t registered=0,isListener=0,isDefaultListener=0,useDefaultListeners=0;
    void *component=nullptr,*owner=nullptr,*controller=nullptr,*player=nullptr,*objectWorld=nullptr;
    engine::Vec cachedPosition{};
};
struct Snapshot {
    bool valid=false,consistent=false;
    SetHeader header{};
    std::array<uint32_t,MaxSlots/32> bits{};
    std::array<Row,MaxListeners> rows{};
    size_t count=0,own=0,nonLocalPC=0,special=0,otherWorld=0,unknown=0,expired=0;
    void* primarySpatialListener=nullptr;
};
struct Removal {
    uint64_t sequence=0,atMs=0;
    Weak identity{};
    void *component=nullptr,*camera=nullptr,*controller=nullptr;
    bool originalCompleted=false,removed=false,remainingPreserved=false;
};
struct Diagnostics {
    bool initialized=false,sourceVerified=false,localPairValid=false,deviceReady=false;
    bool waiting=false,ambiguous=false,restartRequired=false;
    unsigned error=0;
    uintptr_t exceptionRva=0;
    const char* stage="original_audio_listeners_uninitialized";
    uint64_t samples=0,reconciles=0,nativeCalls=0,nativeCompleted=0,lastPollMs=0;
    uint64_t guardedSkips=0,offThreadCalls=0,removalSequence=0;
    void *world=nullptr,*gameInstance=nullptr,*localPlayer=nullptr,*activeController=nullptr;
    void *module=nullptr,*device=nullptr,*soundEngine=nullptr;
    Weak worldId{};
    Snapshot current{},before{};
    std::array<Removal,History> removals{};
};
inline Diagnostics state;
inline const Diagnostics& State(){return state;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;return false;}
inline bool SameWeak(const Weak& a,const Weak& b){return a.index==b.index&&a.serial==b.serial;}
inline bool Contains(const Snapshot& s,const Weak& id){
    for(size_t i=0;i<s.count;++i)if(SameWeak(s.rows[i].identity,id))return true;
    return false;
}
inline bool Bytes(uintptr_t rva,size_t size,uint64_t expected){
    const auto* p=reinterpret_cast<const uint8_t*>(engine::image+rva);
    if(!original_players::Readable(p,size))return false;
    uint64_t hash=14695981039346656037ULL;
    for(size_t i=0;i<size;++i){hash^=p[i];hash*=1099511628211ULL;}
    return hash==expected;
}
// Full original-body FNV64 profiles are filled from the pinned, SHA-verified PE.
inline bool SourceMatches(){
    return Bytes(0x596cb10,730,0x205ec6160e87997cULL)
      &&Bytes(0x596cdf0,1044,0x6db3f1a0c6cce1ffULL)
      &&Bytes(0x596b440,576,0xcfabfbe4c6de3a2bULL)
      &&Bytes(0x596d210,355,0xf2dbd4e14348dd92ULL)
      &&Bytes(0x5979510,912,0xc3d2985c83ea5986ULL)
      &&Bytes(0x5978240,670,0x735e04523797de7aULL);
}
inline bool Thread(){
    if(GetCurrentThreadId()==original_players::gameThread&&!state.restartRequired)return true;
    ++state.offThreadCalls;return false;
}
inline bool LocalContext(){
    state.localPairValid=false;
    if(state.world!=original_players::world||!state.world
      ||original_inventory_slots::ResolveIdentity(state.worldId)!=state.world)return false;
    state.gameInstance=Read<void*>(state.world,0x1d8);
    auto data=Read<void*>(state.gameInstance,0x40);
    const auto count=Read<int32_t>(state.gameInstance,0x48),capacity=Read<int32_t>(state.gameInstance,0x4c);
    if(!data||count!=1||capacity<1||capacity>32)return false;
    state.localPlayer=Read<void*>(data,0);
    state.activeController=Read<void*>(state.localPlayer,0x38);
    state.localPairValid=state.localPlayer&&state.activeController
      &&state.activeController==original_players::activeController
      &&Read<void*>(state.activeController,0x510)==state.localPlayer;
    return state.localPairValid;
}
inline void* ActorWorld(void* actor){
    auto table=Read<void**>(actor,0);auto fn=Read<void*>(table,0x1b8);
    return fn?reinterpret_cast<void*(*)(void*)>(fn)(actor):nullptr;
}
inline void Describe(Row& row){
    row.component=original_inventory_slots::ResolveIdentity(row.identity);
    if(!row.component){row.kind=Kind::Expired;return;}
    row.akClass=original_players::IsA(row.component,0x592fb90);
    if(!row.akClass)return;
    row.owner=Read<void*>(row.component,0xc8);
    row.registered=Read<uint8_t>(row.component,0x37b);
    row.isListener=Read<uint8_t>(row.component,0x3dd);
    row.isDefaultListener=Read<uint8_t>(row.component,0x3de);
    row.useDefaultListeners=Read<uint8_t>(row.component,0x4b0);
    original_players::Read(row.component,0x498,row.cachedPosition);
    if(!row.owner){row.kind=Kind::Special;return;}
    row.cameraClass=original_players::IsA(row.owner,0x3e75c30);
    if(!row.cameraClass){row.kind=Kind::Special;return;}
    row.objectWorld=ActorWorld(row.owner);row.sameWorld=row.objectWorld==state.world;
    if(!row.sameWorld){row.kind=Kind::OtherWorld;return;}
    row.controller=Read<void*>(row.owner,0x210);
    row.controllerClass=original_players::IsA(row.controller,0x3c0a200);
    if(!row.controllerClass)return;
    row.cameraPair=Read<void*>(row.owner,0x468)==row.controller
      &&Read<void*>(row.controller,0x528)==row.owner;
    row.player=Read<void*>(row.controller,0x510);row.noPlayer=row.player==nullptr;
    row.actualLocal=row.controller==state.activeController;
    if(row.actualLocal&&row.cameraPair&&row.player==state.localPlayer){row.kind=Kind::Own;return;}
    if(!row.actualLocal&&row.cameraPair&&row.noPlayer&&row.registered==1
      &&row.isListener==1&&row.isDefaultListener==1)row.kind=Kind::NonLocalPC;
}
inline bool ReadSnapshot(Snapshot& out){
    out={};if(!state.device)return false;
    const auto set=reinterpret_cast<const uint8_t*>(state.device)+0x158;
    if(!original_players::Read(set,0,out.header))return false;
    const auto& h=out.header;
    if(h.slots<0||h.slots>static_cast<int32_t>(MaxSlots)||h.capacity<h.slots
      ||h.capacity>static_cast<int32_t>(MaxSlots)||h.freeCount<0||h.freeCount>h.slots
      ||h.bitCount!=h.slots||h.bitCapacity<h.bitCount||h.bitCapacity>static_cast<int32_t>(MaxSlots)
      ||(h.slots&&!h.data)||h.slots-h.freeCount>static_cast<int32_t>(MaxListeners))return false;
    const auto words=static_cast<size_t>((h.bitCount+31)/32);
    const auto* bits=h.heapBits?h.heapBits:reinterpret_cast<const uint32_t*>(set+0x10);
    for(size_t i=0;i<words;++i)if(!original_players::Read(bits,i*4,out.bits[i]))return false;
    for(int32_t slot=0;slot<h.slots;++slot){
        if(!(out.bits[static_cast<size_t>(slot)/32]&(1U<<(static_cast<unsigned>(slot)&31U))))continue;
        if(out.count>=MaxListeners)return false;
        auto& row=out.rows[out.count++];row.slot=slot;
        if(!original_players::Read(h.data,static_cast<size_t>(slot)*16,row.identity))return false;
        Describe(row);
        switch(row.kind){case Kind::Own:++out.own;break;case Kind::NonLocalPC:++out.nonLocalPC;break;
          case Kind::Special:++out.special;break;case Kind::OtherWorld:++out.otherWorld;break;
          case Kind::Expired:++out.expired;break;default:++out.unknown;break;}
    }
    if(out.count!=static_cast<size_t>(h.slots-h.freeCount))return false;
    SetHeader after{};if(!original_players::Read(set,0,after)||std::memcmp(&h,&after,sizeof(h))!=0)return false;
    for(size_t i=0;i<words;++i){uint32_t value=0;if(!original_players::Read(bits,i*4,value)||value!=out.bits[i])return false;}
    for(size_t i=0;i<out.count;++i){Weak value{};
        if(!original_players::Read(h.data,static_cast<size_t>(out.rows[i].slot)*16,value)
          ||!SameWeak(value,out.rows[i].identity))return false;
    }
    out.primarySpatialListener=Read<void*>(state.device,0x1a8);
    out.valid=true;out.consistent=true;return true;
}
inline bool SampleImpl(){
    if(!state.initialized||!Thread())return Fail(3980,"original_audio_listeners_wrong_lifecycle_or_thread");
    if(!SourceMatches())return Fail(3981,"original_audio_listener_original_code_profile_changed");
    state.sourceVerified=true;state.waiting=false;state.ambiguous=false;
    if(!LocalContext())return Fail(3982,"original_audio_listeners_requires_sole_actual_local_player");
    // Reading the module cache does not load AkAudio or create a FAkAudioDevice.
    state.module=Read<void*>(reinterpret_cast<void*>(engine::image+ModulePointerRva),0);
    state.device=Read<void*>(state.module,0x10);
    state.soundEngine=Read<void*>(reinterpret_cast<void*>(engine::image+0xda2b548),0);
    state.deviceReady=state.module&&state.device&&state.soundEngine;
    if(!state.deviceReady){state.current={};state.waiting=true;state.error=0;state.stage="original_audio_listeners_waiting_for_original_device";return true;}
    if(!ReadSnapshot(state.current))return Fail(3983,"original_audio_default_listener_snapshot_invalid_or_changed");
    ++state.samples;
    state.waiting=state.current.own==0;
    state.ambiguous=state.current.own!=1||state.current.unknown!=0||state.current.expired!=0;
    state.error=0;state.stage=state.waiting?"original_audio_waiting_for_real_own_default_listener":
      state.ambiguous?"original_audio_default_listener_membership_ambiguous":
      state.current.nonLocalPC?"original_audio_nonlocal_pc_default_listeners_observed":
      "original_audio_default_listeners_no_nonlocal_pc_observed";
    return true;
}
inline int Exception(EXCEPTION_POINTERS* p){
    state.error=3984;state.restartRequired=true;state.stage="original_audio_listener_exception_requires_new_session";
    const auto ip=reinterpret_cast<uintptr_t>(p&&p->ExceptionRecord?p->ExceptionRecord->ExceptionAddress:nullptr);
    state.exceptionRva=ip>=engine::image?ip-engine::image:ip;return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* world){
    __try {
        state={};state.world=world;
        if(GetCurrentThreadId()!=original_players::gameThread||world!=original_players::world
          ||!original_inventory_slots::CaptureIdentity(world,state.worldId))return Fail(3985,"original_audio_listener_initial_world_invalid");
        if(!SourceMatches())return Fail(3981,"original_audio_listener_original_code_profile_changed");
        state.sourceVerified=true;state.initialized=true;return SampleImpl();
    }__except(Exception(GetExceptionInformation())){return false;}
}
inline bool Sample(){__try{return SampleImpl();}__except(Exception(GetExceptionInformation())){return false;}}
inline bool ReconcileImpl(bool force){
    if(!state.initialized||!Thread())return Fail(3980,"original_audio_listeners_wrong_lifecycle_or_thread");
    const auto now=GetTickCount64();
    // Spread the original emitter/backend work across frames. Existing excess
    // listeners drain promptly, with at most one complete native removal per
    // pass; settled sessions only inspect once per second.
    const uint64_t cadence=state.current.nonLocalPC&&state.current.own==1&&!state.ambiguous?16:1000;
    if(!force&&state.lastPollMs&&now-state.lastPollMs<cadence)return true;
    state.lastPollMs=now;if(!SampleImpl())return false;
    if(!state.deviceReady||state.current.own!=1||state.ambiguous){++state.guardedSkips;return true;}
    state.before=state.current;++state.reconciles;
    for(size_t i=0;i<state.before.count;++i){
        const auto& previous=state.before.rows[i];if(previous.kind!=Kind::NonLocalPC)continue;
        if(!LocalContext()||!SourceMatches())return Fail(3986,"original_audio_listener_reconcile_context_changed");
        Row current{};current.identity=previous.identity;Describe(current);
        if(current.kind!=Kind::NonLocalPC||current.component!=previous.component
          ||current.owner!=previous.owner||current.controller!=previous.controller)
          return Fail(3987,"original_audio_listener_reconcile_identity_or_owner_changed");
        Snapshot prior{};if(!ReadSnapshot(prior)||prior.own!=1||!Contains(prior,current.identity))
          return Fail(3988,"original_audio_listener_reconcile_set_changed");
        auto& result=state.removals[static_cast<size_t>(state.removalSequence%History)];result={};
        result.sequence=++state.removalSequence;result.atMs=now;result.identity=current.identity;
        result.component=current.component;result.camera=current.owner;result.controller=current.controller;
        ++state.nativeCalls;
        engine::function<void(*)(void*,void*)>(RemoveDefaultRva)(state.device,current.component);
        result.originalCompleted=true;++state.nativeCompleted;
        Snapshot after{};if(!ReadSnapshot(after))return Fail(3989,"original_audio_listener_after_remove_snapshot_invalid");
        result.removed=!Contains(after,current.identity);
        result.remainingPreserved=after.count+1==prior.count&&after.own==1;
        for(size_t n=0;n<prior.count;++n)if(!SameWeak(prior.rows[n].identity,current.identity)
          &&!Contains(after,prior.rows[n].identity))result.remainingPreserved=false;
        if(!result.removed||!result.remainingPreserved||Read<uint8_t>(current.component,0x3dd)!=0
          ||Read<uint8_t>(current.component,0x3de)!=0)return Fail(3990,"original_audio_native_remove_postcondition_failed");
        state.current=after;
        break;
    }
    if(!SampleImpl())return false;
    state.stage=state.current.nonLocalPC?"original_audio_nonlocal_listener_pending":
      "original_audio_native_default_listener_membership_reconciled";return true;
}
inline bool Reconcile(bool force=false){__try{return ReconcileImpl(force);}__except(Exception(GetExceptionInformation())){return false;}}
inline bool Poll(bool force=false){return Reconcile(force);}
inline const char* KindName(Kind kind){
    switch(kind){case Kind::Own:return "actual_local_pc";case Kind::NonLocalPC:return "same_world_nonlocal_pc";
      case Kind::Special:return "non_pc_special_preserved";case Kind::OtherWorld:return "other_world_preserved";
      case Kind::Expired:return "expired_preserved";default:return "unknown_preserved";}
}
inline void WriteSnapshot(FILE* f,const char* name,const Snapshot& s){
    std::fprintf(f,",\"%s\":{\"valid\":%s,\"consistent\":%s,\"count\":%zu,\"own\":%zu,\"nonlocal_pc\":%zu,\"special_preserved\":%zu,\"other_world_preserved\":%zu,\"unknown_preserved\":%zu,\"expired_preserved\":%zu,\"slots\":%d,\"free_count\":%d,\"primary_spatial_listener\":\"%p\",\"listeners\":[",
      name,s.valid?"true":"false",s.consistent?"true":"false",s.count,s.own,s.nonLocalPC,s.special,s.otherWorld,s.unknown,s.expired,s.header.slots,s.header.freeCount,s.primarySpatialListener);
    for(size_t i=0;i<s.count;++i){const auto& r=s.rows[i];
        std::fprintf(f,"%s{\"slot\":%d,\"index\":%d,\"serial\":%d,\"kind\":\"%s\",\"component\":\"%p\",\"owner\":\"%p\",\"controller\":\"%p\",\"player\":\"%p\",\"world\":\"%p\",\"camera_pair\":%s,\"registered\":%u,\"is_listener\":%u,\"is_default_listener\":%u,\"use_default_listeners\":%u}",
          i?",":"",r.slot,r.identity.index,r.identity.serial,KindName(r.kind),r.component,r.owner,r.controller,r.player,r.objectWorld,
          r.cameraPair?"true":"false",static_cast<unsigned>(r.registered),static_cast<unsigned>(r.isListener),static_cast<unsigned>(r.isDefaultListener),static_cast<unsigned>(r.useDefaultListeners));
    }
    std::fputs("]}",f);
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(f,"{\"schema_version\":1,\"pid\":%lu,\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"source_verified\":%s,\"actual_local_pair_valid\":%s,\"device_ready\":%s,\"waiting\":%s,\"ambiguous\":%s,\"restart_required\":%s,\"samples\":%llu,\"reconciles\":%llu,\"native_calls\":%llu,\"native_completed\":%llu,\"guarded_skips\":%llu,\"off_thread_calls\":%llu,\"exception_rva\":%llu,\"module\":\"%p\",\"device\":\"%p\",\"sound_engine\":\"%p\",\"world\":\"%p\",\"local_player\":\"%p\",\"active_controller\":\"%p\",\"image_writes\":0,\"manual_set_writes\":0,\"volume_writes\":0,\"locality_writes\":0,\"native_remove_rva\":93769200",
      GetCurrentProcessId(),state.stage,state.error,b(state.initialized),b(state.sourceVerified),b(state.localPairValid),b(state.deviceReady),b(state.waiting),b(state.ambiguous),b(state.restartRequired),
      static_cast<unsigned long long>(state.samples),static_cast<unsigned long long>(state.reconciles),static_cast<unsigned long long>(state.nativeCalls),static_cast<unsigned long long>(state.nativeCompleted),
      static_cast<unsigned long long>(state.guardedSkips),static_cast<unsigned long long>(state.offThreadCalls),static_cast<unsigned long long>(state.exceptionRva),state.module,state.device,state.soundEngine,state.world,state.localPlayer,state.activeController);
    WriteSnapshot(f,"current",state.current);WriteSnapshot(f,"before_reconcile",state.before);
    std::fputs(",\"removals\":[",f);bool first=true;
    for(const auto& r:state.removals){if(!r.sequence)continue;
        std::fprintf(f,"%s{\"sequence\":%llu,\"at_ms\":%llu,\"index\":%d,\"serial\":%d,\"component\":\"%p\",\"camera\":\"%p\",\"controller\":\"%p\",\"original_completed\":%s,\"removed\":%s,\"remaining_preserved\":%s}",
          first?"":",",static_cast<unsigned long long>(r.sequence),static_cast<unsigned long long>(r.atMs),r.identity.index,r.identity.serial,r.component,r.camera,r.controller,b(r.originalCompleted),b(r.removed),b(r.remainingPreserved));first=false;
    }
    std::fputs("]}\n",f);const bool ok=std::ferror(f)==0;std::fclose(f);return ok;
}
} // namespace original_audio_listeners
