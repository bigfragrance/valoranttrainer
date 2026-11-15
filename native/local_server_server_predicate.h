#pragma once
#include "original_inventory_slots.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>

// Candidate host only. The actual Kismet.IsServer UFunction retains its full
// original exec parameter parser. A private instruction-preserving copy only
// routes its final bool helper through a truthful, owned-listener check.
// IsDedicatedServer and native direct calls are never intercepted or changed.
namespace local_server_server_predicate {
using Weak=original_inventory_slots::WeakIdentity;
using Exec=void(*)(void*,void*,void*);
using ListenerProof=bool(*)(void* world,void* driver,void* socket,void* address,uint32_t port);
inline constexpr uintptr_t ExecRva=0x45d4de0,OriginalBoolRva=0x45dac30,WorldResolverRva=0x4c2a420;
inline constexpr uintptr_t DriverServerRva=0x47387b0,LibraryClass=0x45aa110;
inline constexpr size_t ArenaBytes=0x2000,CodeBytes=0x1000,RelayOffset=0x100;
inline constexpr size_t UnwindOffset=0x1000,LeafUnwindOffset=0x1010,TableOffset=0x1020;
// BEGIN GENERATED ORIGINAL SERVER PREDICATE PROFILE
inline constexpr uintptr_t ImageBytes=0xe69c000;
inline constexpr uintptr_t OriginalUnwindRva=0xc9abc50,GuardDispatchIat=0xa1dc1f8;
inline constexpr uint8_t OriginalExecBytes[]{
    0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x18,0x57,0x48,0x83,0xec,0x20,0x33,
    0xff,0x49,0x8b,0xf0,0x48,0x89,0x7c,0x24,0x38,0x48,0x8b,0xda,0xe8,0x0f,0x08,0x6a,
    0xfd,0x48,0x8b,0x43,0x20,0x48,0x85,0xc0,0x74,0x25,0x0f,0xb6,0x08,0x4c,0x8d,0x0d,
    0x2c,0x2e,0x17,0x09,0x48,0xff,0xc0,0x4c,0x8d,0x44,0x24,0x38,0x48,0x89,0x43,0x20,
    0x48,0x8b,0xd3,0x8b,0xc1,0x48,0x8b,0x4b,0x18,0x49,0x8b,0x04,0xc1,0xeb,0x61,0x48,
    0x8b,0x8b,0x88,0x00,0x00,0x00,0x48,0x8b,0x41,0x18,0x48,0x89,0x83,0x88,0x00,0x00,
    0x00,0x8b,0x41,0x38,0x48,0x0f,0xba,0xe0,0x08,0x73,0x23,0x48,0x8b,0x83,0x80,0x00,
    0x00,0x00,0x48,0x39,0x08,0x74,0x09,0x48,0x8b,0x40,0x10,0x48,0x39,0x08,0x75,0xf7,
    0x48,0x8b,0x48,0x08,0x48,0x89,0x4b,0x38,0x48,0x89,0x7b,0x40,0xeb,0x28,0x48,0x63,
    0x41,0x44,0x48,0x8d,0x54,0x24,0x38,0x4c,0x8b,0x43,0x28,0x49,0x03,0xc0,0x4c,0x89,
    0x43,0x40,0x48,0x89,0x43,0x38,0x48,0x8b,0x01,0x48,0x8b,0x80,0x10,0x01,0x00,0x00,
    0xff,0x15,0x62,0x73,0xc0,0x05,0x48,0x8b,0x43,0x20,0x48,0x8b,0x4c,0x24,0x38,0x48,
    0x85,0xc0,0x40,0x0f,0x95,0xc7,0x48,0x03,0xf8,0x48,0x89,0x7b,0x20,0xe8,0x7e,0x5d,
    0x00,0x00,0x48,0x8b,0x5c,0x24,0x30,0x88,0x06,0x48,0x8b,0x74,0x24,0x40,0x48,0x83,
    0xc4,0x20,0x5f,0xc3,
};
inline constexpr uint8_t OriginalUnwindBytes[]{
    0x01,0x0f,0x06,0x00,0x0f,0x64,0x08,0x00,0x0f,0x34,0x06,0x00,0x0f,0x32,0x0b,0x70,
};
inline constexpr uint16_t InstructionBoundaries[]{0x0,0x5,0xa,0xb,0xf,0x11,0x14,0x19,0x1c,0x21,0x25,0x28,0x2a,0x2d,0x34,0x37,0x3c,0x40,0x43,0x45,0x49,0x4d,0x4f,0x56,0x5a,0x61,0x64,0x69,0x6b,0x72,0x75,0x77,0x7b,0x7e,0x80,0x84,0x88,0x8c,0x8e,0x92,0x97,0x9b,0x9e,0xa2,0xa6,0xa9,0xb0,0xb6,0xba,0xbf,0xc2,0xc6,0xc9,0xcd,0xd2,0xd7,0xd9,0xde,0xe2,0xe3};
struct Relocation {uint16_t instruction;uint8_t bytes,field;uintptr_t target;bool predicate;};
inline constexpr Relocation Relocations[]{
    {0x1c,5,1,0x1c75610,false},
    {0x2d,7,3,0xd747c40,false},
    {0xb0,6,2,0xa1dc1f8,false},
    {0xcd,5,1,0x45dac30,true},
};
// END GENERATED ORIGINAL SERVER PREDICATE PROFILE
struct State {
    bool installed=false,closed=false,sourceVerified=false,metadataVerified=false,contextVerified=false;
    bool built=false,unwindRegistered=false,unwindVerified=false,cfgEnabled=false,cfgRegistered=false;
    bool restored=false,actualListenProof=false;
    unsigned error=0;DWORD gameThread=0,dynamicPolicy=0,cfgPolicy=0,shadowPolicy=0;
    int actorNetMode=-1;
    uint32_t port=0;
    uint16_t actualFunctionParmsSize=0;uint32_t actualFunctionFlags=0;
    uintptr_t actualFunctionExec=0,actualFunctionExecRva=0,actualFunctionOwner=0,expectedFunctionOwner=0;
    uint64_t entries=0,originalExecForwards=0,originalBoolCalls=0,truthfulServerResults=0;
    uint64_t foreignWorldResults=0,proofMisses=0,installs=0,closes=0,unwindCases=0;
    uintptr_t arena=0,world=0,driver=0,engine=0,function=0;
    const char* stage="native_server_predicate_not_installed";
};
inline State state{},cached{};
inline SRWLOCK cacheLock=SRWLOCK_INIT,callLease=SRWLOCK_INIT;
inline thread_local uint32_t execDepth=0;
alignas(8) inline volatile LONG64 entries=0,originalExecForwards=0,originalBoolCalls=0;
alignas(8) inline volatile LONG64 truthfulServerResults=0,foreignWorldResults=0,proofMisses=0;
inline uint64_t Count(volatile LONG64& value){return static_cast<uint64_t>(InterlockedCompareExchange64(&value,0,0));}
inline void Cache(){
    State copy=state;copy.entries=Count(entries);copy.originalExecForwards=Count(originalExecForwards);
    copy.originalBoolCalls=Count(originalBoolCalls);copy.truthfulServerResults=Count(truthfulServerResults);
    copy.foreignWorldResults=Count(foreignWorldResults);copy.proofMisses=Count(proofMisses);
    AcquireSRWLockExclusive(&cacheLock);cached=copy;ReleaseSRWLockExclusive(&cacheLock);
}
inline State Snapshot(){AcquireSRWLockShared(&cacheLock);const auto copy=cached;ReleaseSRWLockShared(&cacheLock);return copy;}
inline const State& GetState(){return state;}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;Cache();return false;}
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
template<class Fn> inline Fn Api(const wchar_t* module,const char* name){
    const FARPROC pointer=GetProcAddress(GetModuleHandleW(module),name);Fn result=nullptr;
    static_assert(sizeof(result)==sizeof(pointer));std::memcpy(&result,&pointer,sizeof(result));return result;
}
using AddTable=BOOLEAN(WINAPI*)(PRUNTIME_FUNCTION,DWORD,DWORD64);
using DeleteTable=BOOLEAN(WINAPI*)(PRUNTIME_FUNCTION);
using Lookup=PRUNTIME_FUNCTION(WINAPI*)(DWORD64,PDWORD64,PUNWIND_HISTORY_TABLE);
using Unwind=PEXCEPTION_ROUTINE(WINAPI*)(DWORD,DWORD64,DWORD64,PRUNTIME_FUNCTION,PCONTEXT,PVOID*,PDWORD64,PKNONVOLATILE_CONTEXT_POINTERS);
using QueryPolicy=BOOL(WINAPI*)(HANDLE,PROCESS_MITIGATION_POLICY,PVOID,SIZE_T);
using RegisterTargets=BOOL(WINAPI*)(HANDLE,PVOID,SIZE_T,ULONG,PCFG_CALL_TARGET_INFO);
struct Runtime {
    uint8_t* arena=nullptr;PRUNTIME_FUNCTION table=nullptr;Exec invoke=nullptr;
    AddTable add=nullptr;DeleteTable remove=nullptr;Lookup lookup=nullptr;Unwind unwind=nullptr;
    bool registered=false;
};
inline Runtime runtime{};
struct Binding {
    void *world=nullptr,*driver=nullptr,*engine=nullptr,*gameInstance=nullptr,*function=nullptr,*ownerClass=nullptr;
    void *socket=nullptr,*address=nullptr;uint32_t port=0;DWORD thread=0;
    std::array<Weak,6> identities{};ListenerProof proof=nullptr;bool active=false;
};
inline Binding binding{};
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Capture(void* object,Weak& identity){return original_inventory_slots::CaptureIdentity(object,identity)&&Live(object,identity);}
inline bool WeakMatch(const Binding& b){
    void* objects[]{b.world,b.driver,b.engine,b.gameInstance,b.function,b.ownerClass};
    for(size_t n=0;n<std::size(objects);++n)if(!Live(objects[n],b.identities[n]))return false;
    return true;
}
inline bool Executable(const void* pointer){
    MEMORY_BASIC_INFORMATION info{};
    if(!pointer||!VirtualQuery(pointer,&info,sizeof(info))||info.State!=MEM_COMMIT||(info.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
    const auto protect=info.Protect&0xff;
    return protect==PAGE_EXECUTE||protect==PAGE_EXECUTE_READ||protect==PAGE_EXECUTE_READWRITE||protect==PAGE_EXECUTE_WRITECOPY;
}
inline bool WritableNativeField(void* function){
    const auto field=reinterpret_cast<uintptr_t>(function)+0xe0;
    MEMORY_BASIC_INFORMATION info{};
    return !(field&7)&&VirtualQuery(reinterpret_cast<void*>(field),&info,sizeof(info))
        &&info.State==MEM_COMMIT&&info.Type==MEM_PRIVATE&&!(info.Protect&(PAGE_GUARD|PAGE_NOACCESS))
        &&(info.Protect&0xff)==PAGE_READWRITE&&field>=reinterpret_cast<uintptr_t>(info.BaseAddress)
        &&field+sizeof(void*)<=reinterpret_cast<uintptr_t>(info.BaseAddress)+info.RegionSize;
}
inline bool Profile(uintptr_t rva,size_t bytes,uint64_t wanted){
    const auto data=reinterpret_cast<const uint8_t*>(engine::image+rva);
    if(!original_players::Readable(data,bytes))return false;
    uint64_t hash=0xcbf29ce484222325ULL;for(size_t n=0;n<bytes;++n){hash^=data[n];hash*=0x100000001b3ULL;}
    return hash==wanted;
}
inline bool VerifySource(){
    if(state.sourceVerified)return true;
    if(!engine::image||!Profile(ExecRva,sizeof(OriginalExecBytes),0xc78adb8cc6553177ULL)
       ||!Profile(OriginalBoolRva,1574,0x07a507c34c6e109cULL)
       ||!Profile(WorldResolverRva,1974,0xc34a453259316442ULL)
       ||!Profile(0x3efc820,176,0x7f1113b3f79164b2ULL)
       ||!Profile(DriverServerRva,12,0x94d91abb077040c3ULL)
       ||std::memcmp(reinterpret_cast<void*>(engine::image+ExecRva),OriginalExecBytes,sizeof(OriginalExecBytes))
       ||std::memcmp(reinterpret_cast<void*>(engine::image+OriginalUnwindRva),OriginalUnwindBytes,sizeof(OriginalUnwindBytes)))
        return Fail(6460,"native_server_predicate_original_source_profile_mismatch");
    state.sourceVerified=true;return true;
}
inline bool EngineWorld(const Binding& b){
    if(Read<void*>(b.world,0x1d8)!=b.gameInstance||Read<void*>(b.gameInstance,0x28)!=b.engine)return false;
    const auto context=Read<void*>(b.gameInstance,0x38);
    if(!context||Read<void*>(context,0x2c8)!=b.world||Read<void*>(context,0x210)!=b.gameInstance)return false;
    const auto contexts=Read<void**>(b.engine,0x1120);const int32_t count=Read<int32_t>(b.engine,0x1128);
    if(!contexts||count<1||count>16)return false;
    for(int32_t n=0;n<count;++n)if(Read<void*>(contexts,static_cast<size_t>(n)*8)==context)return true;
    return false;
}
inline bool ActualListener(const Binding& b){
    if(!WeakMatch(b)||!EngineWorld(b)||!b.proof||b.port<1024||b.port>65535
       ||Read<void*>(b.world,0x40)!=b.driver||Read<void*>(b.driver,0x178)!=b.world||Read<void*>(b.driver,0xc0)
       ||Read<void*>(b.driver,0x10)!=engine::Class(0x5112c90)
       ||!b.socket||!b.address||Read<void*>(b.driver,0x968)!=b.socket||Read<void*>(b.driver,0x778)!=b.address
       ||Read<void*>(Read<void*>(b.driver),0x430)!=reinterpret_cast<void*>(engine::image+DriverServerRva))return false;
    if(!engine::function<bool(*)(void*)>(DriverServerRva)(b.driver))return false;
    return b.proof(b.world,b.driver,b.socket,b.address,b.port);
}
inline bool TruthfulIsServer(void* context){
    // Keep the original implementation, including its native world lookup and
    // original side effects, exactly once for every original wrapper call.
    InterlockedIncrement64(&originalBoolCalls);
    const bool original=engine::function<bool(*)(void*)>(OriginalBoolRva)(context);
    if(original||!binding.active||GetCurrentThreadId()!=binding.thread)return original;
    // Mode0 is the retained ReturnNull path: null/no-world emits no log and
    // never enters mode2's assertion path. The original context is not changed.
    const auto world=engine::function<void*(*)(void*,void*,uint32_t)>(WorldResolverRva)(binding.engine,context,0);
    if(world!=binding.world){InterlockedIncrement64(&foreignWorldResults);return original;}
    if(!ActualListener(binding)){InterlockedIncrement64(&proofMisses);return original;}
    InterlockedIncrement64(&truthfulServerResults);return true;
}
inline void ExecAdapter(void* context,void* frame,void* result){
    InterlockedIncrement64(&entries);
    // Foreign threads never use the private arena and preserve the complete
    // original exec. A late callback after restoration is also safe below.
    if(GetCurrentThreadId()!=state.gameThread){
        InterlockedIncrement64(&originalExecForwards);
        engine::function<Exec>(ExecRva)(context,frame,result);return;
    }
    const bool outer=execDepth==0;if(outer)AcquireSRWLockShared(&callLease);++execDepth;
    __try {
        if(binding.active&&runtime.invoke)runtime.invoke(context,frame,result);
        else {InterlockedIncrement64(&originalExecForwards);engine::function<Exec>(ExecRva)(context,frame,result);}
    }
    __finally {--execDepth;if(outer)ReleaseSRWLockShared(&callLease);if(!execDepth)Cache();}
}
// Read-only composition proof for consumers that validate this exact native
// IsServer UFunction. It never accepts a foreign wrapper or a passed-in pointer
// without re-reading the actual function field and the real listener tuple.
inline bool OwnsIsServerExec(void* world,void* function,void* actualExec){
    if(state.error||!state.installed||state.closed||!binding.active||!state.built||!state.unwindVerified
       ||GetCurrentThreadId()!=state.gameThread||world!=binding.world||function!=binding.function
       ||actualExec!=reinterpret_cast<void*>(&ExecAdapter)||!WeakMatch(binding)
       ||Read<void*>(function,0xe0)!=actualExec)return false;
    return ActualListener(binding);
}
inline bool ShadowPolicySupported(const PROCESS_MITIGATION_USER_SHADOW_STACK_POLICY& policy){
    // CetDynamicApisOutOfProcOnly restricts two configuration APIs used by
    // neither this adapter nor its original exec. It does not enable HSP.
    if(policy.ReservedFlags||policy.EnableUserShadowStack||policy.SetContextIpValidation||policy.BlockNonCetBinaries)
        return false;
    if(policy.AuditUserShadowStack||policy.AuditSetContextIpValidation||policy.EnableUserShadowStackStrictMode
       ||policy.BlockNonCetBinariesNonEhcont||policy.AuditBlockNonCetBinaries||policy.SetContextIpValidationRelaxedMode)
        return false;
    return true;
}
inline bool Mitigations(){
    const auto query=Api<QueryPolicy>(L"kernel32.dll","GetProcessMitigationPolicy");
    PROCESS_MITIGATION_DYNAMIC_CODE_POLICY dynamic{};PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY cfg{};
    PROCESS_MITIGATION_USER_SHADOW_STACK_POLICY shadow{};
    if(!query||!query(GetCurrentProcess(),ProcessDynamicCodePolicy,&dynamic,sizeof(dynamic))
       ||!query(GetCurrentProcess(),ProcessControlFlowGuardPolicy,&cfg,sizeof(cfg))
       ||!query(GetCurrentProcess(),ProcessUserShadowStackPolicy,&shadow,sizeof(shadow)))
        return Fail(6461,"native_server_predicate_mitigation_query_failed");
    state.dynamicPolicy=dynamic.Flags;state.cfgPolicy=cfg.Flags;state.shadowPolicy=shadow.Flags;state.cfgEnabled=cfg.EnableControlFlowGuard!=0;
    if(dynamic.ProhibitDynamicCode||cfg.EnableXfg||cfg.StrictMode||!ShadowPolicySupported(shadow))
        return Fail(6462,"native_server_predicate_unsupported_active_mitigation");
    return true;
}
inline bool Difference(uint8_t* field,int64_t value){
    if(value<INT32_MIN||value>INT32_MAX)return false;const auto converted=static_cast<int32_t>(value);
    std::memcpy(field,&converted,sizeof(converted));return true;
}
inline bool DiscardRuntime(){
    if(runtime.registered&&(!runtime.remove||!runtime.remove(runtime.table)))
        return Fail(6463,"native_server_predicate_unwind_unregister_failed_arena_retained");
    if(runtime.arena)VirtualFree(runtime.arena,0,MEM_RELEASE);
    runtime={};state.built=state.unwindRegistered=state.unwindVerified=state.cfgRegistered=false;state.arena=0;return true;
}
inline bool SameContext(const CONTEXT& a,const CONTEXT& b){
    return a.Rip==b.Rip&&a.Rsp==b.Rsp&&a.Rbx==b.Rbx&&a.Rbp==b.Rbp&&a.Rsi==b.Rsi&&a.Rdi==b.Rdi
        &&a.R12==b.R12&&a.R13==b.R13&&a.R14==b.R14&&a.R15==b.R15
        &&std::memcmp(&a.Xmm6,&b.Xmm6,sizeof(M128A)*10)==0;
}
inline bool CheckUnwind(){
    alignas(16) std::array<uint64_t,128> stack{};
    for(size_t n=0;n<stack.size();++n)stack[n]=0x1122334400000000ULL+n;
    for(const uint16_t offset:InstructionBoundaries){
        const auto original=engine::image+ExecRva+offset,copied=reinterpret_cast<uintptr_t>(runtime.arena)+offset;
        DWORD64 sourceBase=0,targetBase=0;
        const auto source=runtime.lookup(original,&sourceBase,nullptr),target=runtime.lookup(copied,&targetBase,nullptr);
        if(!source||!target||sourceBase!=engine::image||targetBase!=reinterpret_cast<uintptr_t>(runtime.arena)
           ||source->BeginAddress!=ExecRva||source->EndAddress!=ExecRva+sizeof(OriginalExecBytes)
           ||source->UnwindData!=OriginalUnwindRva)return false;
        for(DWORD mode:{DWORD(0),DWORD(UNW_FLAG_UHANDLER)}){
            CONTEXT a{},b{};a.ContextFlags=CONTEXT_FULL;a.Rip=original;a.Rsp=reinterpret_cast<DWORD64>(stack.data()+32);
            a.Rbx=0x1122112211221122ULL;a.Rsi=0x2233223322332233ULL;a.Rdi=0x3344334433443344ULL;b=a;b.Rip=copied;
            void *sourceData=nullptr,*targetData=nullptr;DWORD64 sourceFrame=0,targetFrame=0;
            const auto h1=runtime.unwind(mode,sourceBase,original,source,&a,&sourceData,&sourceFrame,nullptr);
            const auto h2=runtime.unwind(mode,targetBase,copied,target,&b,&targetData,&targetFrame,nullptr);
            if(h1||h2||!SameContext(a,b)||sourceFrame!=targetFrame)return false;
            ++state.unwindCases;
        }
    }
    state.unwindVerified=true;return true;
}
inline bool BuildRuntime(){
    if(state.built)return true;if(!VerifySource()||!Mitigations())return false;
    runtime.add=Api<AddTable>(L"ntdll.dll","RtlAddFunctionTable");runtime.remove=Api<DeleteTable>(L"ntdll.dll","RtlDeleteFunctionTable");
    runtime.lookup=Api<Lookup>(L"ntdll.dll","RtlLookupFunctionEntry");runtime.unwind=Api<Unwind>(L"ntdll.dll","RtlVirtualUnwind");
    if(!runtime.add||!runtime.remove||!runtime.lookup||!runtime.unwind)return Fail(6464,"native_server_predicate_unwind_api_unavailable");
    SYSTEM_INFO system{};GetSystemInfo(&system);const uintptr_t gran=system.dwAllocationGranularity;
    const uintptr_t first=(engine::image+ImageBytes+gran-1)&~(gran-1),limit=first+0x40000000ULL;
    for(uintptr_t address=first;address<limit;){
        MEMORY_BASIC_INFORMATION memory{};if(!VirtualQuery(reinterpret_cast<void*>(address),&memory,sizeof(memory)))break;
        const auto end=reinterpret_cast<uintptr_t>(memory.BaseAddress)+memory.RegionSize;
        if(memory.State==MEM_FREE&&end>=address+ArenaBytes){
            runtime.arena=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(address),ArenaBytes,MEM_COMMIT|MEM_RESERVE,
                state.cfgEnabled?(PAGE_EXECUTE_READ|PAGE_TARGETS_INVALID):PAGE_READWRITE));
            if(runtime.arena)break;
        }
        if(end<=address)break;address=(end+gran-1)&~(gran-1);
    }
    if(!runtime.arena)return Fail(6465,"native_server_predicate_near_image_arena_unavailable");
    state.arena=reinterpret_cast<uintptr_t>(runtime.arena);
    if(state.cfgEnabled){DWORD before=0;if(!VirtualProtect(runtime.arena,ArenaBytes,PAGE_READWRITE,&before)){
        DiscardRuntime();return Fail(6466,"native_server_predicate_nonexecutable_staging_failed");}}
    std::memcpy(runtime.arena,OriginalExecBytes,sizeof(OriginalExecBytes));
    for(const auto& patch:Relocations){
        const uintptr_t target=patch.predicate?state.arena+RelayOffset:engine::image+patch.target;
        auto instruction=runtime.arena+patch.instruction;
        if(!Difference(instruction+patch.field,static_cast<int64_t>(target)
            -static_cast<int64_t>(reinterpret_cast<uintptr_t>(instruction)+patch.bytes))){
            DiscardRuntime();return Fail(6467,"native_server_predicate_relative_operand_out_of_range");}
    }
    // Source CALL -> leaf relay -> actual original CFG dispatcher -> typed
    // bool(WorldContextObject*) callback. The original CALL return is retained.
    auto relay=runtime.arena+RelayOffset;relay[0]=0x48;relay[1]=0xb8;
    const auto callback=reinterpret_cast<uintptr_t>(&TruthfulIsServer);std::memcpy(relay+2,&callback,8);relay[10]=0xff;relay[11]=0x25;
    if(!Executable(Read<void*>(reinterpret_cast<void*>(engine::image),GuardDispatchIat))
       ||!Difference(relay+12,static_cast<int64_t>(engine::image+GuardDispatchIat)
          -static_cast<int64_t>(reinterpret_cast<uintptr_t>(relay)+16))){
        DiscardRuntime();return Fail(6468,"native_server_predicate_original_cfg_dispatch_unverified");}
    std::memcpy(runtime.arena+UnwindOffset,OriginalUnwindBytes,sizeof(OriginalUnwindBytes));runtime.arena[LeafUnwindOffset]=1;
    runtime.table=reinterpret_cast<PRUNTIME_FUNCTION>(runtime.arena+TableOffset);
    runtime.table[0]={0,static_cast<DWORD>(sizeof(OriginalExecBytes)),static_cast<DWORD>(UnwindOffset)};
    runtime.table[1]={static_cast<DWORD>(RelayOffset),static_cast<DWORD>(RelayOffset+16),static_cast<DWORD>(LeafUnwindOffset)};
    DWORD before=0;
    if(!VirtualProtect(runtime.arena,CodeBytes,PAGE_EXECUTE_READ|(state.cfgEnabled?PAGE_TARGETS_NO_UPDATE:0),&before)
       ||!VirtualProtect(runtime.arena+CodeBytes,ArenaBytes-CodeBytes,PAGE_READONLY,&before)
       ||!FlushInstructionCache(GetCurrentProcess(),runtime.arena,CodeBytes)){
        DiscardRuntime();return Fail(6469,"native_server_predicate_private_code_protection_failed");}
    if(state.cfgEnabled){
        const auto targets=Api<RegisterTargets>(L"KernelBase.dll","SetProcessValidCallTargets");
        CFG_CALL_TARGET_INFO valid[2]{{0,CFG_CALL_TARGET_VALID},{RelayOffset,CFG_CALL_TARGET_VALID}};
        if(!targets||!targets(GetCurrentProcess(),runtime.arena,CodeBytes,2,valid)){
            DiscardRuntime();return Fail(6470,"native_server_predicate_cfg_targets_not_registered");}
        state.cfgRegistered=true;
    }
    if(!runtime.add(runtime.table,2,reinterpret_cast<DWORD64>(runtime.arena))){
        DiscardRuntime();return Fail(6471,"native_server_predicate_unwind_table_not_registered");}
    runtime.registered=true;state.unwindRegistered=true;
    if(!CheckUnwind()){DiscardRuntime();return Fail(6472,"native_server_predicate_original_and_private_unwind_differ");}
    runtime.invoke=reinterpret_cast<Exec>(runtime.arena);state.built=true;Cache();return true;
}
inline bool InstallImpl(void* world,void* driver,void* owningEngine,uint32_t port,ListenerProof proof){
    if(state.error||state.closed)return false;
    if(!engine::image||!original_players::gameThread||GetCurrentThreadId()!=original_players::gameThread)
        return Fail(6473,"native_server_predicate_install_requires_original_game_thread");
    state.gameThread=GetCurrentThreadId();
    if(binding.active){
        if(binding.world!=world||binding.driver!=driver||binding.engine!=owningEngine||binding.port!=port||binding.proof!=proof)
            return Fail(6474,"native_server_predicate_owned_listen_epoch_changed");
        if(!ActualListener(binding)||Read<void*>(binding.function,0xe0)!=reinterpret_cast<void*>(&ExecAdapter))
            return Fail(6475,"native_server_predicate_bound_listener_or_exec_changed");
        Cache();return true;
    }
    if(!VerifySource())return false;
    Binding next;next.world=world;next.driver=driver;next.engine=owningEngine;next.port=port;next.proof=proof;
    next.thread=GetCurrentThreadId();next.gameInstance=Read<void*>(world,0x1d8);
    next.socket=Read<void*>(driver,0x968);next.address=Read<void*>(driver,0x778);
    next.function=engine::Find(L"/Script/Engine.KismetSystemLibrary.IsServer");next.ownerClass=engine::Class(LibraryClass);
    state.function=reinterpret_cast<uintptr_t>(next.function);
    state.actualFunctionParmsSize=Read<uint16_t>(next.function,0xbe);state.actualFunctionFlags=Read<uint32_t>(next.function,0xb8);
    state.actualFunctionExec=Read<uintptr_t>(next.function,0xe0);
    state.actualFunctionExecRva=state.actualFunctionExec>=engine::image&&state.actualFunctionExec<engine::image+ImageBytes
        ?state.actualFunctionExec-engine::image:state.actualFunctionExec;
    state.actualFunctionOwner=Read<uintptr_t>(next.function,0x28);state.expectedFunctionOwner=reinterpret_cast<uintptr_t>(next.ownerClass);
    if(!next.function||Read<void*>(next.function,0x28)!=next.ownerClass
       // Actual pinned-game UFunction metadata is9: UObject*@0 + bool@8.
       // The generated C++ parameter container is16 after trailing alignment.
       ||Read<uint16_t>(next.function,0xbe)!=9||(Read<uint32_t>(next.function,0xb8)&0x400)==0
       ||Read<void*>(next.function,0xe0)!=reinterpret_cast<void*>(engine::image+ExecRva)||!WritableNativeField(next.function))
        return Fail(6476,"native_server_predicate_original_ufunction_metadata_or_writable_instance_field_mismatch");
    state.metadataVerified=true;
    void* objects[]{next.world,next.driver,next.engine,next.gameInstance,next.function,next.ownerClass};
    for(size_t n=0;n<std::size(objects);++n)if(!Capture(objects[n],next.identities[n]))
        return Fail(6477,"native_server_predicate_original_weak_identity_unavailable");
    if(!ActualListener(next))return Fail(6478,"native_server_predicate_requires_actual_owned_open_loopback_ip_listener");
    const auto gameState=Read<void*>(world,0x178);
    if(!original_players::IsA(gameState,original_players::ShooterGameStateClass))
        return Fail(6479,"native_server_predicate_original_game_state_unavailable");
    state.actorNetMode=engine::function<int(*)(void*)>(0x3efc820)(gameState);
    if(state.actorNetMode!=1&&state.actorNetMode!=2)return Fail(6480,"native_server_predicate_original_actor_does_not_confirm_server");
    if(!BuildRuntime())return false;
    binding=next;binding.active=true;state.gameThread=next.thread;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(static_cast<uint8_t*>(binding.function)+0xe0),
        reinterpret_cast<void*>(&ExecAdapter),reinterpret_cast<void*>(engine::image+ExecRva))!=reinterpret_cast<void*>(engine::image+ExecRva)){
        binding={};DiscardRuntime();return Fail(6481,"native_server_predicate_original_exec_publication_conflict");}
    state.installed=state.contextVerified=state.actualListenProof=true;state.port=port;
    state.world=reinterpret_cast<uintptr_t>(world);state.driver=reinterpret_cast<uintptr_t>(driver);
    state.engine=reinterpret_cast<uintptr_t>(owningEngine);state.function=reinterpret_cast<uintptr_t>(binding.function);
    ++state.installs;state.stage="native_server_predicate_original_exec_with_truthful_owned_host_context";Cache();return true;
}
inline bool Install(void* world,void* driver,void* owningEngine,uint32_t port,ListenerProof proof){
    __try{return InstallImpl(world,driver,owningEngine,port,proof);}
    __except(EXCEPTION_EXECUTE_HANDLER){return Fail(6482,"native_server_predicate_install_validation_exception_requires_stop");}
}
inline bool Poll(){
    if(state.error)return false;
    if(!binding.active)return !state.closed;
    if(GetCurrentThreadId()!=state.gameThread)return Fail(6483,"native_server_predicate_poll_wrong_original_game_thread");
    if(!ActualListener(binding)||Read<void*>(binding.function,0xe0)!=reinterpret_cast<void*>(&ExecAdapter))
        return Fail(6475,"native_server_predicate_bound_listener_or_exec_changed");
    Cache();return true;
}
inline bool Close(){
    if(state.closed)return true;
    if(!state.installed&&!runtime.arena){state.closed=true;Cache();return true;}
    if(GetCurrentThreadId()!=state.gameThread||execDepth)
        return Fail(6484,"native_server_predicate_close_requires_nonreentrant_original_game_thread");
    AcquireSRWLockExclusive(&callLease);
    bool restored=true;
    if(binding.active){
        restored=Live(binding.function,binding.identities[4])&&WritableNativeField(binding.function)
            &&InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(static_cast<uint8_t*>(binding.function)+0xe0),
               reinterpret_cast<void*>(engine::image+ExecRva),reinterpret_cast<void*>(&ExecAdapter))==reinterpret_cast<void*>(&ExecAdapter);
    }
    if(!restored){ReleaseSRWLockExclusive(&callLease);return Fail(6485,"native_server_predicate_owned_exec_restore_conflict_runtime_retained");}
    binding.active=false;state.restored=true;state.installed=state.actualListenProof=state.contextVerified=false;
    // No clone address was ever published. Late ExecAdapter callers obtain a
    // shared lease after this deactivation and invoke the original exec only.
    const bool discarded=DiscardRuntime();
    if(discarded){binding={};state.closed=true;++state.closes;state.stage="native_server_predicate_original_exec_restored";}
    ReleaseSRWLockExclusive(&callLease);Cache();return discarded;
}
inline void Write(FILE* file){
    if(!file)return;const auto s=Snapshot();const auto yes=[](bool value){return value?"true":"false";};
    std::fprintf(file,"{\"stage\":\"%s\",\"error\":%u,\"installed\":%s,\"closed\":%s,\"source_verified\":%s,\"metadata_verified\":%s,\"context_verified\":%s,\"actual_listen_proof\":%s,\"actor_net_mode\":%d,\"port\":%u,\"built\":%s,\"unwind_registered\":%s,\"unwind_verified\":%s,\"cfg_enabled\":%s,\"cfg_registered\":%s,\"restored\":%s,\"dynamic_policy\":%lu,\"cfg_policy\":%lu,\"shadow_policy\":%lu,\"entries\":%llu,\"original_exec_forwards\":%llu,\"original_bool_calls\":%llu,\"truthful_server_results\":%llu,\"foreign_world_results\":%llu,\"proof_misses\":%llu,\"installs\":%llu,\"closes\":%llu,\"unwind_cases\":%llu,\"arena\":\"0x%llx\",\"world\":\"0x%llx\",\"driver\":\"0x%llx\",\"function\":\"0x%llx\",\"dedicated_predicate_intercepted\":false,\"original_image_writes\":0,\"role_writes\":0,\"driver_state_writes\":0",
        s.stage,s.error,yes(s.installed),yes(s.closed),yes(s.sourceVerified),yes(s.metadataVerified),yes(s.contextVerified),yes(s.actualListenProof),s.actorNetMode,s.port,
        yes(s.built),yes(s.unwindRegistered),yes(s.unwindVerified),yes(s.cfgEnabled),yes(s.cfgRegistered),yes(s.restored),s.dynamicPolicy,s.cfgPolicy,s.shadowPolicy,
        s.entries,s.originalExecForwards,s.originalBoolCalls,s.truthfulServerResults,s.foreignWorldResults,s.proofMisses,s.installs,s.closes,s.unwindCases,
        static_cast<unsigned long long>(s.arena),static_cast<unsigned long long>(s.world),static_cast<unsigned long long>(s.driver),static_cast<unsigned long long>(s.function));
    std::fprintf(file,",\"actual_function_parms_size\":%u,\"actual_function_flags\":%u,\"actual_function_exec\":\"0x%llx\",\"actual_function_exec_rva\":\"0x%llx\",\"actual_function_owner\":\"0x%llx\",\"expected_function_owner\":\"0x%llx\"}",
        static_cast<unsigned>(s.actualFunctionParmsSize),s.actualFunctionFlags,static_cast<unsigned long long>(s.actualFunctionExec),
        static_cast<unsigned long long>(s.actualFunctionExecRva),static_cast<unsigned long long>(s.actualFunctionOwner),static_cast<unsigned long long>(s.expectedFunctionOwner));
}
} // namespace local_server_server_predicate
