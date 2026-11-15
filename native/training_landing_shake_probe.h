#pragma once
#include "original_inventory_slots.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>

// Optional stage9 observation only. Default disabled. The one owned real PCM
// retains every original virtual except a transparent StartCameraShake wrapper.
// Every call forwards all five original arguments and the original return value.
namespace training_landing_shake_probe {
inline constexpr uintptr_t TableRva=0xb4db5c0,StartRva=0x4914370,ImageBytes=0xe69c000;
inline constexpr size_t TableEntries=305,StartSlot=0x910/8,Capacity=64,Depth=32;
inline constexpr const wchar_t* LandingPath=L"/Game/Blueprints/Weapons/LandingCameraShakeEffect.LandingCameraShakeEffect_C";
inline constexpr uintptr_t PrefixRva=0x4928760; // Original preceding word preserved verbatim.
inline constexpr uint32_t NativeSlots[TableEntries]{
    0x6c5c350U,0x18a3a00U,0x1cfa620U,0x1af09d0U,0x18a76a0U,0x3ed82e0U,0x1d06c40U,0x18a3a00U,0x42d2880U,0x463ef70U,
    0x3ed7050U,0x1af0aa0U,0x1af0a90U,0x1af0ab0U,0x3ed8280U,0x18a3a00U,0x18a3a00U,0x3ed83a0U,0x18a76a0U,0x3ed83a0U,
    0x18a3a00U,0x36afd10U,0x18a3a00U,0x18a76a0U,0x18a3ec0U,0x3ed9b40U,0x3ed9c00U,0x3ed9030U,0x36c7460U,0x1c53840U,
    0x3ef6ca0U,0x3ed9770U,0x3ed98d0U,0x18a3a00U,0x18a3a00U,0x3f00330U,0x18a3a00U,0x1af0b30U,0x18a3a00U,0x1c53720U,
    0x1c53700U,0x18a3ec0U,0x18a76a0U,0x3ed82f0U,0x18a76a0U,0x18a76a0U,0x18a76a0U,0x1c53be0U,0x18a3a00U,0x18a3a00U,
    0x18a3a00U,0x18a3a00U,0x18a3a00U,0x3eda8f0U,0x1af0b50U,0x3ed83b0U,0x18a76a0U,0x1c562c0U,0x1af0b70U,0x18a3ed0U,
    0x18a3ec0U,0x1c554e0U,0x18a3a00U,0x3ed8300U,0x1c56490U,0x1c56550U,0x1c56640U,0x1c56670U,0x3f06430U,0x3f06af0U,
    0x3f07590U,0x3f07520U,0x1c5dc90U,0x18a3ec0U,0x3f07140U,0x3f050b0U,0x3f050f0U,0x18a3a00U,0x18a3a00U,0x1c54130U,
    0x18a3ed0U,0x18a3a00U,0x18a3a00U,0x3eda0d0U,0x3efc9b0U,0x3efd360U,0x18a76a0U,0x18a3ed0U,0x18a3a00U,0x3ed7960U,
    0x3ef58f0U,0x18a3a00U,0x18dd100U,0x18a3a00U,0x18a3a00U,0x3f01d20U,0x3ee1720U,0x3ede520U,0x3ede430U,0x18a3a00U,
    0x3ef3450U,0x3efa480U,0x3efa7a0U,0x3ede560U,0x18a76a0U,0x18a3a00U,0x3f06b50U,0x3f073b0U,0x3f07480U,0x3edac90U,
    0x3edb100U,0x18a3a00U,0x3efabc0U,0x3efacb0U,0x3efadc0U,0x3efbfd0U,0x3ec57c0U,0x3efbce0U,0x3ee3d10U,0x3ed8fb0U,
    0x3ed8fd0U,0x3ed8ff0U,0x3ed9010U,0x7868a30U,0x49109e0U,0x3edd790U,0x3edd7f0U,0x3edd850U,0x3edd8b0U,0x3edd910U,
    0x3edda40U,0x3eddb70U,0x3eddbd0U,0x3eddc30U,0x3eddc90U,0x3eddcf0U,0x3effdb0U,0x3f00160U,0x18a76a0U,0x3f05e10U,
    0x2a652f0U,0x4913a30U,0x3690180U,0x3f00a50U,0x3f00b20U,0x3f00d30U,0x3f04d90U,0x3f04f60U,0x18a76a0U,0x18a3a00U,
    0x18a76a0U,0x18a3a00U,0x18a3a00U,0x3eda8c0U,0x3edab20U,0x2a65300U,0x18a3a00U,0x3ee3d10U,0x18a3a00U,0x3efa6a0U,
    0x3f051e0U,0x3f052e0U,0x18a3a00U,0x3f056f0U,0x3f05750U,0x3ede1d0U,0x3edc1a0U,0x3edee80U,0x3edab80U,0x490e1f0U,
    0x3f05b40U,0x3f05df0U,0x3edef40U,0x3f00260U,0x49107d0U,0x3efdad0U,0x2a65390U,0x3edaaf0U,0x3edaad0U,0x18a76a0U,
    0x18a76a0U,0x3efde00U,0x3efde70U,0x3efdf10U,0x3efdfd0U,0x3efe5c0U,0x18a3a00U,0x3ee6350U,0x3efe400U,0x3efe590U,
    0x3eff7d0U,0x2a65370U,0x3efea40U,0x18a3a00U,0x3ed8b20U,0x18a3a00U,0x3ed8aa0U,0x3edb330U,0x3edb2e0U,0x18a3a00U,
    0x3f07830U,0x3f07860U,0x3eda4e0U,0x18a3ec0U,0x4910a40U,0x3ee1eb0U,0x3ee3d10U,0x3edb350U,0x3edb420U,0x3edb120U,
    0x3edb210U,0x3edb2a0U,0x18a3a00U,0x3ee4650U,0x18a3ec0U,0x3ee20d0U,0x3ee2670U,0x2781760U,0x3ee3d20U,0x3ee3d80U,
    0x3ee3de0U,0x3ee3f50U,0x3ee4190U,0x1b97710U,0x3ee1e50U,0x3ede1c0U,0x3ee43e0U,0x3edef50U,0x3ee1030U,0x3ee4500U,
    0x3edeed0U,0x18a3a00U,0x3ee57b0U,0x3ee5d60U,0x3f06d30U,0x18a3ec0U,0x49134e0U,0x18a3a00U,0x18a3a00U,0x18a3a00U,
    0x18a3a00U,0x490d860U,0x4914b60U,0x4914b70U,0x4914b80U,0x4914b90U,0x4914ba0U,0x4914bd0U,0x490d930U,0x7868ab0U,
    0x7868c30U,0x4911920U,0x4911390U,0x4910420U,0x4910550U,0x4910740U,0x49105e0U,0x49106f0U,0x490e200U,0x4910a70U,
    0x4910c10U,0x4910c40U,0x4910c50U,0x4910c60U,0x4910c70U,0x4910c80U,0x4910ca0U,0x4910cb0U,0x4910cc0U,0x4910d10U,
    0x4910d50U,0x4910d90U,0x4911400U,0x4911a40U,0x4911a00U,0x18a3ec0U,0x490f710U,0x490fd90U,0x490e510U,0x490f680U,
    0x786a100U,0x7868a50U,0x4911b40U,0x4913c80U,0x4913e30U,0x49141e0U,0x4914200U,0x49142b0U,0x49142e0U,0x4914310U,
    0x4914370U,0x49144a0U,0x49145d0U,0x4914600U,0x4914680U,0x49146a0U,0x4914730U,0x49149e0U,0x4914ac0U,0x4914b00U,
    0x3e75ce0U,0x4913060U,0x49131e0U,0x4913360U,0x7869570U,
};
inline constexpr uint8_t StartBody[]{0x48,0x89,0x5c,0x24,0x8,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x70,0xf,0x29,0x74,0x24,0x60,0x41,0xf,0xb6,0xf1,0xf,0x28,0xf2,0x48,0x8b,0xda,0x48,0x8b,0xf9,0x48,0x85,0xd2,0xf,0x84,0xe4,0x0,0x0,0x0,0xe8,0x31,0xfe,0xff,0xfe,0x48,0x85,0xc0,0xf,0x84,0xd6,0x0,0x0,0x0,0x48,0x8d,0x50,0x38,0x48,0x63,0x42,0x8,0x3b,0x43,0x40,0xf,0x8f,0xc5,0x0,0x0,0x0,0x48,0x8b,0xc8,0x48,0x8b,0x43,0x38,0x48,0x39,0x14,0xc8,0xf,0x85,0xb4,0x0,0x0,0x0,0x48,0x8b,0x8f,0x38,0x2a,0x0,0x0,0x48,0x85,0xc9,0xf,0x84,0xa4,0x0,0x0,0x0,0xf,0x57,0xc0,0xf,0x2f,0xf0,0xf,0x86,0x98,0x0,0x0,0x0,0x48,0x8b,0x1,0xf3,0xf,0x11,0x74,0x24,0x20,0x40,0x88,0x74,0x24,0x24,0x48,0x8b,0x94,0x24,0xa0,0x0,0x0,0x0,0xf,0x10,0x2,0xf,0x11,0x44,0x24,0x28,0xf2,0xf,0x10,0x4a,0x10,0xf2,0xf,0x11,0x4c,0x24,0x38,0xf,0x57,0xc0,0xf3,0xf,0x7f,0x44,0x24,0x40,0x33,0xff,0x89,0x7c,0x24,0x50,0x4c,0x8d,0x44,0x24,0x20,0x48,0x8b,0xd3,0x48,0x8b,0x80,0x50,0x3,0x0,0x0,0xff,0x15,0xc5,0x7d,0x8c,0x5,0x48,0x8b,0xd8,0x48,0x8b,0x4c,0x24,0x48,0x39,0x7c,0x24,0x50,0x74,0x2d,0x48,0x85,0xc9,0x74,0x28,0x48,0x8b,0x11,0x48,0x8b,0x2,0x33,0xd2,0xff,0x15,0xa4,0x7d,0x8c,0x5,0x44,0x8d,0x4f,0x10,0x45,0x33,0xc0,0x48,0x8d,0x4c,0x24,0x48,0xe8,0xcb,0x8d,0xfb,0xfc,0x89,0x7c,0x24,0x50,0x48,0x8b,0x4c,0x24,0x48,0x48,0x85,0xc9,0x74,0x6,0xe8,0xa8,0x12,0x0,0xfd,0x90,0x48,0x8b,0xc3,0xeb,0x2,0x33,0xc0,0x4c,0x8d,0x5c,0x24,0x70,0x49,0x8b,0x5b,0x10,0x49,0x8b,0x73,0x18,0xf,0x28,0x74,0x24,0x60,0x49,0x8b,0xe3,0x5f,0xc3};
using WeakIdentity=original_inventory_slots::WeakIdentity;
using Start=void*(*)(void*,void*,float,uint8_t,const engine::Vec*);
struct Sample {
    uint64_t sequence=0,atMs=0;
    DWORD thread=0;
    bool actualOwner=false,rotationRead=false,attempted=false,completed=false;
    void *camera=nullptr,*type=nullptr,*controller=nullptr,*result=nullptr;
    float scale=0;
    uint8_t playSpace=0;
    uint16_t frames=0;
    engine::Vec rotation{};
    std::array<uintptr_t,Depth> stack{};
};
struct Diagnostics {
    bool enabled=false,initialized=false,bound=false,sourceVerified=false,cloneReadOnly=false;
    bool sourceUnchanged=false,ownerValid=false,profileValid=false;
    unsigned error=0;
    const char* stage="landing_shake_probe_disabled";
    uint64_t calls=0,forwardAttempts=0,forwardCompleted=0,landingCalls=0,otherCalls=0;
    uint64_t guardFailures=0,observerExceptions=0,retirements=0,overwritten=0;
    DWORD gameThread=0;
    void *world=nullptr,*controller=nullptr,*camera=nullptr,*localPlayer=nullptr,*landingClass=nullptr;
    WeakIdentity worldId{},controllerId{},cameraId{},playerId{};
    uintptr_t module=0,moduleBytes=0,observerExceptionAddress=0;
    std::array<Sample,Capacity> samples{};
};
inline Diagnostics state;
inline void** clone=nullptr;
inline void* allocation=nullptr;
inline std::atomic<uint64_t> offThreadCalls{0};
inline const Diagnostics& State(){return state;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline void** Source(){return reinterpret_cast<void**>(engine::image+TableRva);}
inline Start Original(){return engine::function<Start>(StartRva);}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;return false;}
inline bool Live(void* object,const WeakIdentity& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool SourceMatches(){
    auto source=Source();if(Read<uintptr_t>(source-1,0)!=engine::image+PrefixRva)return false;
    for(size_t i=0;i<TableEntries;++i)if(Read<uintptr_t>(source,i*8)!=engine::image+NativeSlots[i])return false;
    return std::memcmp(reinterpret_cast<const void*>(engine::image+StartRva),StartBody,sizeof(StartBody))==0;
}
inline bool ActualOwner(void* camera){
    if(!state.bound||camera!=state.camera||state.world!=original_players::world
      ||!Live(camera,state.cameraId)||!Live(state.controller,state.controllerId)||!Live(state.world,state.worldId)
      ||!Live(state.localPlayer,state.playerId)||state.controller!=original_players::activeController
      ||Read<void*>(camera,0x468)!=state.controller||Read<void*>(state.controller,0x528)!=camera
      ||Read<void*>(state.controller,0x510)!=state.localPlayer||Read<void*>(state.localPlayer,0x38)!=state.controller)return false;
    auto gi=Read<void*>(state.world,0x1d8),locals=Read<void*>(gi,0x40);
    return gi&&Read<int32_t>(gi,0x48)==1&&Read<int32_t>(gi,0x4c)>=1&&Read<void*>(locals,0)==state.localPlayer;
}
inline Sample* Capture(void* camera,void* type,float scale,uint8_t playSpace,const engine::Vec* rotation){
    ++state.calls;
    if(!state.landingClass)state.landingClass=engine::Find(LandingPath);
    if(!state.landingClass||type!=state.landingClass){++state.otherCalls;return nullptr;}
    ++state.landingCalls;
    auto& s=state.samples[static_cast<size_t>((state.landingCalls-1)%Capacity)];s={};
    s.sequence=state.landingCalls;s.atMs=GetTickCount64();s.thread=GetCurrentThreadId();
    s.camera=camera;s.type=type;s.controller=Read<void*>(camera,0x468);s.scale=scale;s.playSpace=playSpace;
    s.actualOwner=ActualOwner(camera);if(!s.actualOwner)++state.guardFailures;
    s.rotationRead=rotation&&original_players::Read(rotation,0,s.rotation);
    void* frames[Depth]{};s.frames=CaptureStackBackTrace(0,static_cast<DWORD>(Depth),frames,nullptr);
    for(size_t i=0;i<s.frames&&i<Depth;++i)s.stack[i]=reinterpret_cast<uintptr_t>(frames[i]);
    if(state.landingCalls>Capacity)++state.overwritten;
    return &s;
}
inline int ObserveException(EXCEPTION_POINTERS* p){
    ++state.observerExceptions;
    state.observerExceptionAddress=reinterpret_cast<uintptr_t>(p&&p->ExceptionRecord?p->ExceptionRecord->ExceptionAddress:nullptr);
    return EXCEPTION_EXECUTE_HANDLER;
}
inline Sample* CaptureSafely(void* camera,void* type,float scale,uint8_t playSpace,const engine::Vec* rotation){
    __try{return Capture(camera,type,scale,playSpace,rotation);}
    __except(ObserveException(GetExceptionInformation())){return nullptr;}
}
inline void* Observe(void* camera,void* type,float scale,uint8_t playSpace,const engine::Vec* rotation){
    if(GetCurrentThreadId()!=state.gameThread){offThreadCalls.fetch_add(1,std::memory_order_relaxed);return Original()(camera,type,scale,playSpace,rotation);}
    auto sample=CaptureSafely(camera,type,scale,playSpace,rotation);
    ++state.forwardAttempts;if(sample)sample->attempted=true;
    // Deliberately outside the observation SEH: a real original exception is
    // neither hidden nor converted into a synthetic shake result.
    auto result=Original()(camera,type,scale,playSpace,rotation);
    ++state.forwardCompleted;if(sample){sample->result=result;sample->completed=true;}
    return result;
}
inline bool CloneMatches(){
    if(!clone||Read<uintptr_t>(clone-1,0)!=engine::image+PrefixRva)return false;
    for(size_t i=0;i<TableEntries;++i){
        auto wanted=i==StartSlot?reinterpret_cast<void*>(&Observe):reinterpret_cast<void*>(engine::image+NativeSlots[i]);
        if(Read<void*>(clone,i*8)!=wanted)return false;
    }
    return true;
}
inline bool Initialize(bool enabled=false){
    if(!enabled){if(state.bound)return Fail(2920,"landing_shake_probe_restore_before_disabling");state.enabled=false;state.stage="landing_shake_probe_disabled";return true;}
    if(!original_players::Thread())return Fail(2921,"landing_shake_probe_wrong_game_thread");
    if(state.initialized)return state.enabled&&state.gameThread==GetCurrentThreadId()&&SourceMatches()&&CloneMatches();
    state.enabled=true;state.gameThread=GetCurrentThreadId();state.sourceVerified=SourceMatches();state.profileValid=state.sourceVerified;
    if(!state.sourceVerified)return Fail(2922,"landing_shake_probe_complete_pinned_table_or_entry_mismatch");
    constexpr size_t bytes=(TableEntries+1)*sizeof(void*);
    allocation=VirtualAlloc(nullptr,bytes,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if(!allocation)return Fail(2923,"landing_shake_probe_private_table_allocation_failed");
    std::memcpy(allocation,Source()-1,bytes);clone=static_cast<void**>(allocation)+1;clone[StartSlot]=reinterpret_cast<void*>(&Observe);
    DWORD previous=0;
    if(!CloneMatches()||!VirtualProtect(allocation,bytes,PAGE_READONLY,&previous)||previous!=PAGE_READWRITE)
      return Fail(2924,"landing_shake_probe_private_table_protection_failed");
    state.cloneReadOnly=true;state.sourceUnchanged=SourceMatches();
    HMODULE module=nullptr;
    if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&Observe),&module)){
        state.module=reinterpret_cast<uintptr_t>(module);const auto ntOffset=Read<int32_t>(module,0x3c);
        if(ntOffset>0&&ntOffset<4096)state.moduleBytes=Read<uint32_t>(module,static_cast<size_t>(ntOffset)+0x50);
    }
    state.landingClass=engine::Find(LandingPath);state.initialized=true;state.error=0;state.stage="landing_shake_probe_initialized_not_bound";return true;
}
inline bool Bind(void* camera){
    if(!state.enabled)return true;
    if(!state.initialized||!original_players::Thread()||!camera)return Fail(2925,"landing_shake_probe_bind_requires_initialized_game_thread");
    auto controller=original_players::activeController,player=original_players::localPlayer,world=original_players::world;
    if(!original_players::IsA(camera,0x6c5b630)||Read<void*>(camera,0x468)!=controller||Read<void*>(controller,0x528)!=camera
      ||!controller||!player||!world||Read<void*>(controller,0x510)!=player||Read<void*>(player,0x38)!=controller)
      return Fail(2926,"landing_shake_probe_requires_actual_owned_local_pcm");
    if(state.bound){state.ownerValid=ActualOwner(camera);return state.ownerValid&&Read<void*>(camera,0)==clone&&SourceMatches()&&CloneMatches();}
    if(Read<void*>(camera,0)!=Source()||!SourceMatches()||!CloneMatches())return Fail(2927,"landing_shake_probe_refuses_foreign_or_changed_table");
    state.camera=camera;state.controller=controller;state.localPlayer=player;state.world=world;
    if(!original_inventory_slots::CaptureIdentity(camera,state.cameraId)||!original_inventory_slots::CaptureIdentity(controller,state.controllerId)
      ||!original_inventory_slots::CaptureIdentity(player,state.playerId)||!original_inventory_slots::CaptureIdentity(world,state.worldId))
      return Fail(2928,"landing_shake_probe_original_weak_identity_failed");
    state.bound=true;state.ownerValid=ActualOwner(camera);
    if(!state.ownerValid){state.bound=false;return Fail(2929,"landing_shake_probe_local_player_membership_invalid");}
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(camera),clone,Source())!=Source()){
        state.bound=false;return Fail(2930,"landing_shake_probe_instance_install_raced");}
    state.sourceUnchanged=SourceMatches();state.error=0;state.stage="landing_shake_probe_observing_original_calls";return true;
}
inline bool Refresh(){return !state.enabled||Bind(state.camera);}
inline bool BeforeRetire(){
    if(!state.enabled||!state.bound)return true;
    if(!original_players::Thread())return false;
    if(Live(state.camera,state.cameraId)){
        auto actual=Read<void**>(state.camera,0);
        if(actual!=clone&&actual!=Source())return Fail(2931,"landing_shake_probe_retirement_foreign_table");
        if(actual==clone&&InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(state.camera),Source(),clone)!=clone)
          return Fail(2932,"landing_shake_probe_retirement_raced");
    }
    state.bound=false;++state.retirements;state.stage="landing_shake_probe_retired_original_table_restored";return true;
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(state.enabled&&state.initialized){state.sourceUnchanged=SourceMatches();
        if(GetCurrentThreadId()==state.gameThread&&state.bound)state.ownerValid=ActualOwner(state.camera);}
    FILE* f=nullptr;if(!path||_wfopen_s(&f,path,L"wb")||!f)return false;const auto b=[](bool v){return v?"true":"false";};
    std::fprintf(f,"{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u,\"enabled\":%s,\"bound\":%s,\"source_verified\":%s,\"source_unchanged\":%s,\"clone_read_only\":%s,\"owner_valid\":%s,\"native_table_entries\":%zu,\"slot_bytes\":2320,\"native_start_rva\":%llu,\"image_writes\":0,\"arguments_modified\":false,\"return_modified\":false,\"camera\":\"%p\",\"controller\":\"%p\",\"landing_class\":\"%p\",\"calls\":%llu,\"forward_attempts\":%llu,\"forward_completed\":%llu,\"landing_calls\":%llu,\"other_calls\":%llu,\"off_thread_original_forwards\":%llu,\"guard_failures\":%llu,\"observer_exceptions\":%llu,\"observer_exception_address\":\"%p\",\"records_overwritten\":%llu,\"records\":[",
      state.stage,state.error,b(state.enabled),b(state.bound),b(state.sourceVerified),b(state.sourceUnchanged),b(state.cloneReadOnly),b(state.ownerValid),TableEntries,static_cast<unsigned long long>(StartRva),state.camera,state.controller,state.landingClass,
      static_cast<unsigned long long>(state.calls),static_cast<unsigned long long>(state.forwardAttempts),static_cast<unsigned long long>(state.forwardCompleted),static_cast<unsigned long long>(state.landingCalls),static_cast<unsigned long long>(state.otherCalls),static_cast<unsigned long long>(offThreadCalls.load(std::memory_order_relaxed)),static_cast<unsigned long long>(state.guardFailures),static_cast<unsigned long long>(state.observerExceptions),reinterpret_cast<void*>(state.observerExceptionAddress),static_cast<unsigned long long>(state.overwritten));
    const uint64_t count=(std::min)(state.landingCalls,static_cast<uint64_t>(Capacity));
    const uint64_t begin=state.landingCalls-count;
    for(uint64_t n=0;n<count;++n){const auto& s=state.samples[static_cast<size_t>((begin+n)%Capacity)];
        std::fprintf(f,"%s{\"sequence\":%llu,\"at_ms\":%llu,\"thread\":%lu,\"actual_owner\":%s,\"scale\":%.9g,\"play_space\":%u,\"rotation_read\":%s,\"rotation\":[%.9g,%.9g,%.9g],\"original_attempted\":%s,\"original_completed\":%s,\"original_result\":\"%p\",\"frames\":[",n?",":"",static_cast<unsigned long long>(s.sequence),static_cast<unsigned long long>(s.atMs),s.thread,b(s.actualOwner),static_cast<double>(s.scale),static_cast<unsigned>(s.playSpace),b(s.rotationRead),s.rotation.x,s.rotation.y,s.rotation.z,b(s.attempted),b(s.completed),s.result);
        for(size_t i=0;i<s.frames&&i<Depth;++i){const auto at=s.stack[i];const auto native=at>=engine::image&&at-engine::image<ImageBytes?at-engine::image:0;
            const auto owned=state.module&&at>=state.module&&at-state.module<state.moduleBytes?at-state.module:0;
            std::fprintf(f,"%s{\"address\":\"%p\",\"native_rva\":%llu,\"owned_module_rva\":%llu}",i?",":"",reinterpret_cast<void*>(at),static_cast<unsigned long long>(native),static_cast<unsigned long long>(owned));}
        std::fputs("]}",f);
    }
    std::fputs("]}",f);std::fclose(f);return true;
}
}
