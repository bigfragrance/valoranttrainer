#pragma once
#include "original_inventory_slots.h"
#include "local_server_ip_ack_compat/generated.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <limits>

// Generic-IP, owned-client compatibility only. Original normal ACK + movement
// ACK instructions are relocated unchanged in size. Only their final native
// Ares warning callout is routed to the actual original performance tracker.
// No original image, driver class, saved move, clock, role or ACK value is set.
namespace local_server_ip_ack_compat {
using Weak=original_inventory_slots::WeakIdentity;
using AckFn=void(*)(void*,int32_t,uint32_t);
using PawnControllerFn=void(*)(void*,void*);
using BeforePawnControllerFn=void(*)(void*,void*);
using AfterPawnControllerFn=void(*)(void*,void*,bool);
inline constexpr size_t PawnEntries=0xae0/8,AckSlot=0xa70/8,GradualSlot=0xa68/8;
inline constexpr size_t PawnControllerSlot=0x7c0/8;
struct FunctionMetadata {
    uintptr_t object=0,actualOwner=0,expectedOwner=0,actualExec=0,expectedExec=0;
    uint32_t flags=0;uint16_t bytes=0,expectedBytes=0;
    bool found=false,ownerMatches=false,execMatches=false,bytesMatch=false,nativeFlag=false,valid=false;
};
struct FieldMetadata {
    uintptr_t owner=0,field=0,fieldClass=0;
    uint64_t castFlags=0,name=0,expectedName=0;
    int32_t arrayDim=0,elementSize=0,offset=0,expectedOffset=0;
    bool found=false,typeMatches=false,nameMatches=false,arrayMatches=false,sizeMatches=false,offsetMatches=false,valid=false;
};
struct CfgTargetObservation {uintptr_t offset=0,before=0,after=0;};
struct CfgObservation {
    uintptr_t provider=0,api=0,requestedBase=0,queryBase=0,allocationBase=0;
    uint64_t requestedBytes=0,queryRegionBytes=0;
    DWORD lookupError=0,callError=0,queryError=0,protect=0,allocationProtect=0,regionState=0,regionType=0;
    bool queryValid=false,callAttempted=false,succeeded=false;
    std::array<CfgTargetObservation,3> targets{};
};
struct State {
    const char* stage="ip_ack_compat_not_installed";unsigned error=0;
    bool built=false,installed=false,profiles=false,metadata=false,context=false,trackerBound=false;
    bool handlerVerified=false,unwindVerified=false,cfgEnabled=false,cfgRegistered=false;
    DWORD dynamicPolicy=0,cfgPolicy=0,shadowPolicy=0;
    uintptr_t arena=0,handlerTarget=0,tracker=0,component=0;
    uint64_t installs=0,closes=0,acks=0,returns=0,activeWarnings=0,inactiveWarnings=0;
    uint64_t originalAresForwards=0,failedWarnings=0,unwindCases=0;
    uint64_t restoredTables=0,retainedPublishedTables=0,retiredWeakUnavailable=0;
    bool retiring=false;uint64_t lifecycleFences=0,retiringOriginalForwards=0,pawnControllerEntries=0,pawnControllerReturns=0;
    uint32_t pawnLifecycleExceptionCode=0;uintptr_t pawnLifecycleExceptionRva=0;
    uint32_t metadataMatchedMask=0,metadataFirstFailure=0;
    std::array<FunctionMetadata,3> functions{};
    std::array<FieldMetadata,2> fields{};
    CfgObservation cfg{};
};
inline State state{},cached{};inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline void Cache(){AcquireSRWLockExclusive(&cacheLock);cached=state;ReleaseSRWLockExclusive(&cacheLock);}
inline State Snapshot(){AcquireSRWLockShared(&cacheLock);auto copy=cached;ReleaseSRWLockShared(&cacheLock);return copy;}
inline const State& GetState(){return state;}
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;Cache();return false;}
inline bool Thread(){return engine::image&&original_players::gameThread==GetCurrentThreadId();}
inline bool Live(void* p,const Weak& id){return p&&original_inventory_slots::ResolveIdentity(id)==p;}
inline bool Capture(void* p,Weak& id){return original_inventory_slots::CaptureIdentity(p,id)&&Live(p,id);}
template<class Fn> inline Fn Api(const wchar_t* module,const char* name){
    FARPROC address=GetProcAddress(GetModuleHandleW(module),name);Fn result=nullptr;
    static_assert(sizeof(result)==sizeof(address));std::memcpy(&result,&address,sizeof(result));return result;
}
using AddTableFn=BOOLEAN(WINAPI*)(PRUNTIME_FUNCTION,DWORD,DWORD64);
using DeleteTableFn=BOOLEAN(WINAPI*)(PRUNTIME_FUNCTION);
using LookupFn=PRUNTIME_FUNCTION(WINAPI*)(DWORD64,PDWORD64,PUNWIND_HISTORY_TABLE);
using UnwindFn=PEXCEPTION_ROUTINE(WINAPI*)(DWORD,DWORD64,DWORD64,PRUNTIME_FUNCTION,PCONTEXT,PVOID*,PDWORD64,PKNONVOLATILE_CONTEXT_POINTERS);
using PolicyFn=BOOL(WINAPI*)(HANDLE,PROCESS_MITIGATION_POLICY,PVOID,SIZE_T);
using TargetsFn=BOOL(WINAPI*)(HANDLE,PVOID,SIZE_T,ULONG,PCFG_CALL_TARGET_INFO);
struct Runtime {
    uint8_t* arena=nullptr;PRUNTIME_FUNCTION table=nullptr;bool registered=false;
    AddTableFn add=nullptr;DeleteTableFn remove=nullptr;LookupFn lookup=nullptr;UnwindFn unwind=nullptr;
    AckFn normal=nullptr;
};
inline Runtime runtime{};
struct Binding {
    void *world=nullptr,*pc=nullptr,*ps=nullptr,*pawn=nullptr,*driver=nullptr,*connection=nullptr,*player=nullptr;
    void *gameState=nullptr,*component=nullptr,*tracker=nullptr,*movement=nullptr;
    std::array<Weak,11> identities{};
    void **source=nullptr,**copy=nullptr;
    bool active=false,reportedActive=false,retiring=false;uint32_t inFlight=0;
};
inline Binding binding{};inline Binding* warningFrame=nullptr;
inline BeforePawnControllerFn beforePawnController=nullptr;
inline AfterPawnControllerFn afterPawnController=nullptr;
inline bool BindPawnControllerLifecycle(BeforePawnControllerFn callback,AfterPawnControllerFn after){
    if(!Thread())return false;
    if(beforePawnController==callback&&afterPawnController==after)return callback&&after;
    if(!callback||!after||beforePawnController||afterPawnController||binding.active||state.installed)return false;
    beforePawnController=callback;afterPawnController=after;return true;
}
inline bool FenceNativeEpoch(){
    if(!Thread())return false;if(!binding.active)return true;
    if(!binding.retiring){binding.retiring=true;state.retiring=true;++state.lifecycleFences;Cache();}return true;
}
inline LONG ObservePawnLifecycleException(EXCEPTION_POINTERS* e){
    state.pawnLifecycleExceptionCode=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0;
    const auto address=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    state.pawnLifecycleExceptionRva=address>=engine::image?address-engine::image:address;
    Fail(5995,"ip_ack_compat_original_pawn_lifecycle_exception_propagated");return EXCEPTION_CONTINUE_SEARCH;
}
inline void PawnControllerChanged(void* pawn,void* previousController){
    const auto parent=engine::function<PawnControllerFn>(0x7885040);
    if(!Thread()||!beforePawnController||!binding.active||binding.pawn!=pawn){parent(pawn,previousController);return;}
    ++state.pawnControllerEntries;
    bool returned=false;
    // The lifecycle callback only retires owned adapter layers. Original
    // cleanup/delegates and queue reset remain with this original parent once.
    __try{
        __try{beforePawnController(pawn,previousController);}
        __finally{
            __try{parent(pawn,previousController);returned=true;++state.pawnControllerReturns;}
            __finally{if(afterPawnController)afterPawnController(pawn,previousController,returned);Cache();}
        }
    }__except(ObservePawnLifecycleException(GetExceptionInformation())){}
}
inline void *provider=nullptr,*trackerGetter=nullptr,*notify=nullptr;
inline bool Executable(const void* p){
    MEMORY_BASIC_INFORMATION m{};
    if(!p||!VirtualQuery(p,&m,sizeof(m))||m.State!=MEM_COMMIT||(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
    return (m.Protect&0xff)==PAGE_EXECUTE||(m.Protect&0xff)==PAGE_EXECUTE_READ
        ||(m.Protect&0xff)==PAGE_EXECUTE_READWRITE||(m.Protect&0xff)==PAGE_EXECUTE_WRITECOPY;
}
inline bool Function(size_t index,void*& result,const wchar_t* name,uint16_t bytes,uintptr_t exec,uintptr_t owner){
    auto& observed=state.functions[index];observed={};observed.expectedBytes=bytes;
    observed.expectedExec=engine::image+exec;observed.expectedOwner=reinterpret_cast<uintptr_t>(engine::Class(owner));
    result=engine::Find(name);observed.object=reinterpret_cast<uintptr_t>(result);observed.found=result!=nullptr;
    if(result){
        observed.bytes=Read<uint16_t>(result,0xbe);observed.actualExec=Read<uintptr_t>(result,0xe0);
        observed.flags=Read<uint32_t>(result,0xb8);observed.actualOwner=Read<uintptr_t>(result,0x28);
        observed.bytesMatch=observed.bytes==bytes;observed.execMatches=observed.actualExec==observed.expectedExec;
        observed.nativeFlag=(observed.flags&0x400)!=0;observed.ownerMatches=observed.actualOwner==observed.expectedOwner;
    }
    observed.valid=observed.found&&observed.bytesMatch&&observed.execMatches&&observed.nativeFlag&&observed.ownerMatches;
    return observed.valid;
}
inline bool Event(void* object,void* function,void* params){
    auto target=Read<void*>(Read<void*>(object),0x298);
    if(!target||!function)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(target)(object,function,params);return true;
}
inline bool Field(void* type,const wchar_t* name,int32_t offset,FieldMetadata* observation=nullptr){
    FieldMetadata local{};auto& observed=observation?*observation:local;observed={};
    observed.owner=reinterpret_cast<uintptr_t>(type);observed.expectedOffset=offset;
    struct Name{uint32_t words[3]{};} wanted;
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&wanted,name,1);
    auto field=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,&wanted,3);
    observed.expectedName=Read<uint64_t>(&wanted);observed.field=reinterpret_cast<uintptr_t>(field);observed.found=field!=nullptr;
    if(field){
        observed.fieldClass=Read<uintptr_t>(field,8);observed.castFlags=Read<uint64_t>(reinterpret_cast<void*>(observed.fieldClass),0x18);
        observed.name=Read<uint64_t>(field,0x20);observed.arrayDim=Read<int32_t>(field,0x30);
        observed.elementSize=Read<int32_t>(field,0x34);observed.offset=Read<int32_t>(field,0x44);
        observed.typeMatches=(observed.castFlags&0x8001)==0x8001;observed.nameMatches=observed.name==observed.expectedName;
        observed.arrayMatches=observed.arrayDim==1;observed.sizeMatches=observed.elementSize==8;observed.offsetMatches=observed.offset==offset;
    }
    observed.valid=observed.found&&observed.typeMatches&&observed.nameMatches&&observed.arrayMatches&&observed.sizeMatches&&observed.offsetMatches;
    return observed.valid;
}
inline bool VerifySource(){
    if(state.profiles)return true;
    for(const auto& span:generated::SourceSpans){
        auto actual=reinterpret_cast<const void*>(engine::image+span.rva);
        if(!original_players::Readable(actual,span.size)||std::memcmp(actual,span.bytes,span.size))
            return Fail(5960,"ip_ack_compat_original_source_profile_mismatch");
    }
    state.profiles=true;return true;
}
inline bool VerifyMetadata(){
    if(state.metadata)return true;
    const bool matched[]{
        Function(0,provider,L"/Script/ShooterGame.ShooterGameState.GetClientPerformanceMonitoringComponent",8,0x6ca0260,0x6c9d280),
        Function(1,trackerGetter,L"/Script/ShooterGame.ClientPerformanceMonitoringComponent.GetServerResponsivenessInstabilityTrackingDetails",8,0x463fc60,0x66335c0),
        // GenParams' compiled C++ storage is 8 bytes (alignment/tail padding).
        // UFunction.ParmsSize is the last semantic field end: Int32@0 + Bool@4
        // => 5, observed exactly in case58. Keep the native call storage below
        // at sizeof(Params)==8; this gate checks reflected parameter bytes.
        Function(2,notify,L"/Script/ShooterGame.NetworkOutageInstabilityTrackingDetails.OnAresNetworkErrorStateChanged",5,0x68d9490,0x68d85b0),
        Field(engine::Class(0x6c9d280),L"LocalPerformanceMonitoringComponent",0xc58,&state.fields[0]),
        Field(engine::Class(0x66335c0),L"ServerResponsiveness",0x168,&state.fields[1])};
    state.metadataMatchedMask=0;state.metadataFirstFailure=0;
    for(size_t n=0;n<std::size(matched);++n){
        if(matched[n])state.metadataMatchedMask|=uint32_t(1)<<n;
        else if(!state.metadataFirstFailure)state.metadataFirstFailure=static_cast<uint32_t>(n+1);
    }
    if(state.metadataMatchedMask!=0x1f)
        return Fail(5961,"ip_ack_compat_original_tracker_metadata_mismatch");
    state.metadata=true;return true;
}
inline bool Owned(const Binding& b){
    auto gi=Read<void*>(b.world,0x1d8);
    auto locals=Read<void**>(gi,0x40);
    return b.world&&b.pc&&b.pawn&&b.driver&&b.connection&&b.player
        &&original_players::IsA(b.player,0x3b6e680)&&original_players::IsA(b.connection,0x5112be0)
        &&original_players::IsA(b.ps,original_players::ShooterStateClass)
        &&Read<void*>(b.world,0x40)==b.driver&&Read<void*>(b.driver,0x178)==b.world
        &&Read<void*>(b.driver,0x10)==engine::Class(0x5112c90)
        &&Read<void*>(b.driver,0xc0)==b.connection&&Read<void*>(b.connection,0x60)==b.driver
        &&Read<void*>(b.connection,0xa0)==b.pc&&Read<int32_t>(b.connection,0x13c)==3
        &&Read<uint8_t>(b.pc,0x224)==2&&Read<uint8_t>(b.pawn,0x224)==2
        &&original_players::activeController==b.pc&&original_players::activePawn==b.pawn
        &&Read<int32_t>(gi,0x48)==1&&locals&&Read<void*>(locals)==b.player
        &&b.player==original_players::localPlayer&&Read<void*>(b.player,0x38)==b.pc&&Read<void*>(b.pc,0x510)==b.player
        &&Read<void*>(b.pc,0x470)==b.ps&&Read<void*>(b.ps,0x210)==b.pc
        &&Read<void*>(b.pc,0x4b0)==b.pawn&&Read<void*>(b.pawn,0x4a0)==b.pc&&Read<void*>(b.pawn,0x488)==b.ps
        &&Read<void*>(b.pawn,0x4f8)==b.movement&&Read<void*>(b.movement,0xc8)==b.pawn
        &&Read<void*>(b.world,0x178)==b.gameState&&Read<void*>(b.gameState,0xc58)==b.component
        &&Read<void*>(b.component,0xc8)==b.gameState&&Read<void*>(b.component,0x168)==b.tracker
        &&Read<void*>(b.tracker,0x28)==b.component;
}
inline bool WeakMatch(const Binding& b){
    const void* objects[]{b.world,b.pc,b.ps,b.pawn,b.driver,b.connection,b.player,b.gameState,b.component,b.tracker,b.movement};
    for(size_t n=0;n<std::size(objects);++n)
        if(!Live(const_cast<void*>(objects[n]),b.identities[n]))return false;
    return true;
}
inline bool Deliver(Binding& b,int32_t type,uint8_t active){
    if(type<0||type>1||active>1||!WeakMatch(b)||!Owned(b)){
        ++state.failedWarnings;return Fail(5962,"ip_ack_compat_actual_warning_receiver_retired");
    }
    struct Params{int32_t type=0;uint8_t active=0,padding[3]{};} params{type,active};
    static_assert(sizeof(Params)==8&&offsetof(Params,type)==0&&offsetof(Params,active)==4);
    if(!Event(b.tracker,notify,&params)||Read<uint8_t>(b.tracker,0x5a+static_cast<size_t>(type))!=active){
        ++state.failedWarnings;return Fail(5963,"ip_ack_compat_original_warning_delivery_not_observed");
    }
    // Original ClientPerformanceMonitoringComponent tick merges +58/+59/+5A/+5B
    // into its detected/critical model and dispatches OnInstabilityStateChanged.
    // Do not directly set +50 or reinterpret clearing one source as all-clear.
    if(type==0)b.reportedActive=active!=0;
    if(active)++state.activeWarnings;else ++state.inactiveWarnings;Cache();return true;
}
inline void Warning(void* originalDriver,int32_t type,uint8_t active){
    if(originalDriver){++state.originalAresForwards;
        engine::function<void(*)(void*,int32_t,uint8_t)>(0x7170600)(originalDriver,type,active);Cache();return;}
    if(!warningFrame){++state.failedWarnings;Fail(5964,"ip_ack_compat_warning_without_owned_native_frame");return;}
    Deliver(*warningFrame,type,active);
}
inline void NormalAck(void* pawn,int32_t eventId,uint32_t index){
    auto* b=binding.active&&binding.pawn==pawn&&Thread()?&binding:nullptr;
    if(!b){engine::function<AckFn>(0x787c310)(pawn,eventId,index);return;}
    if(b->retiring){++state.retiringOriginalForwards;engine::function<AckFn>(0x787c310)(pawn,eventId,index);return;}
    if(!WeakMatch(*b)||!Owned(*b)){
        Fail(5993,"ip_ack_compat_ack_outside_owned_context_requires_stop");
        // An invalidated epoch is not eligible for this compatibility route.
        // Preserve original ACK handling/exception propagation; never accept or
        // synthesize progress by returning from the adapter instead.
        engine::function<AckFn>(0x787c310)(pawn,eventId,index);return;
    }
    ++b->inFlight;++state.acks;auto preceding=warningFrame;warningFrame=b;
    __try {
        // The actual native flag may rise/recover between poll opportunities.
        // Keep the native read/notification under the same cleanup as the ACK.
        if(Read<uint8_t>(b->movement,0x13d8)&&!b->reportedActive)Deliver(*b,0,1);
        runtime.normal(pawn,eventId,index);++state.returns;
    }
    __finally {warningFrame=preceding;--b->inFlight;Cache();}
}
inline bool IsOwnedAckEntry(void* pawn,void* entry){
    return !state.error&&state.installed&&state.built&&Thread()&&binding.active&&binding.pawn==pawn
        &&!binding.retiring
        &&entry==reinterpret_cast<void*>(&NormalAck)&&WeakMatch(binding)&&Owned(binding);
}
inline bool ShadowPolicySupported(const PROCESS_MITIGATION_USER_SHADOW_STACK_POLICY& policy){
    // The independent CetDynamicApisOutOfProcOnly flag restricts two HSP
    // configuration APIs; this module calls neither one. It does not itself
    // enable shadow stacks, context-IP validation, or binary-load enforcement.
    if(policy.ReservedFlags||policy.EnableUserShadowStack||policy.SetContextIpValidation||policy.BlockNonCetBinaries)
        return false;
    // Microsoft's documented mode/audit fields require their corresponding
    // enabling field. With those disabled, such combinations are not a known
    // valid inactive profile. Never infer support for active HSP from a mode.
    if(policy.AuditUserShadowStack||policy.AuditSetContextIpValidation||policy.EnableUserShadowStackStrictMode
       ||policy.BlockNonCetBinariesNonEhcont||policy.AuditBlockNonCetBinaries||policy.SetContextIpValidationRelaxedMode)
        return false;
    return true;
}
inline bool Mitigations(){
    auto policy=Api<PolicyFn>(L"kernel32.dll","GetProcessMitigationPolicy");
    PROCESS_MITIGATION_DYNAMIC_CODE_POLICY dynamic{};PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY cfg{};
    PROCESS_MITIGATION_USER_SHADOW_STACK_POLICY shadow{};
    if(!policy||!policy(GetCurrentProcess(),ProcessDynamicCodePolicy,&dynamic,sizeof(dynamic))
       ||!policy(GetCurrentProcess(),ProcessControlFlowGuardPolicy,&cfg,sizeof(cfg))
       ||!policy(GetCurrentProcess(),ProcessUserShadowStackPolicy,&shadow,sizeof(shadow)))
        return Fail(5965,"ip_ack_compat_process_mitigation_query_failed");
    state.dynamicPolicy=dynamic.Flags;state.cfgPolicy=cfg.Flags;state.shadowPolicy=shadow.Flags;state.cfgEnabled=cfg.EnableControlFlowGuard!=0;
    // Actual HSP/context enforcement and XFG remain unsupported, never disabled.
    // Case59's 0x100 is the independent HSP API-origin restriction, not active
    // return/context enforcement (the current SDK's relaxed-mode bit is0x200).
    if(dynamic.ProhibitDynamicCode||cfg.EnableXfg||!ShadowPolicySupported(shadow))
        return Fail(5966,"ip_ack_compat_unsupported_active_process_mitigation");
    return true;
}
inline bool Handler(){
    auto actual=Read<uintptr_t>(reinterpret_cast<void*>(engine::image),generated::HandlerIat);
    auto expected=reinterpret_cast<uintptr_t>(GetProcAddress(GetModuleHandleW(L"vcruntime140_1.dll"),"__CxxFrameHandler4"));
    state.handlerTarget=actual;
    if(!expected||actual!=expected||!Executable(reinterpret_cast<void*>(actual)))
        return Fail(5967,"ip_ack_compat_loaded_fh4_handler_identity_unverified");
    state.handlerVerified=true;return true;
}
inline bool DiscardRuntime(){
    if(runtime.registered&&(!runtime.remove||!runtime.remove(runtime.table)))
        return Fail(5992,"ip_ack_compat_unregister_failed_runtime_retained");
    if(runtime.arena)VirtualFree(runtime.arena,0,MEM_RELEASE);
    runtime={};state.built=false;state.arena=0;state.unwindVerified=false;state.cfgRegistered=false;
    return true;
}
inline uintptr_t MapTarget(uintptr_t target){
    for(const auto& body:generated::Bodies)if(target>=body.rva&&target<body.rva+body.size)
        return reinterpret_cast<uintptr_t>(runtime.arena)+body.offset+(target-body.rva);
    return engine::image+target;
}
inline bool Difference(uint8_t* field,uint8_t bytes,int64_t value){
    if(bytes==1){if(value<INT8_MIN||value>INT8_MAX)return false;const auto v=static_cast<int8_t>(value);std::memcpy(field,&v,1);return true;}
    if(bytes!=4||value<INT32_MIN||value>INT32_MAX)return false;
    const auto v=static_cast<int32_t>(value);std::memcpy(field,&v,4);return true;
}
inline bool SameContext(const CONTEXT& a,const CONTEXT& b){
    return a.Rip==b.Rip&&a.Rsp==b.Rsp&&a.Rbx==b.Rbx&&a.Rbp==b.Rbp&&a.Rsi==b.Rsi&&a.Rdi==b.Rdi
        &&a.R12==b.R12&&a.R13==b.R13&&a.R14==b.R14&&a.R15==b.R15
        &&std::memcmp(&a.Xmm6,&b.Xmm6,sizeof(M128A)*10)==0;
}
inline bool CheckUnwind(){
    alignas(16) std::array<uint64_t,128> stack{};
    for(size_t n=0;n<stack.size();++n)stack[n]=0x1122334400000000ULL+n;
    for(const auto& boundary:generated::Boundaries){
        const auto& body=generated::Bodies[boundary.body];
        const uintptr_t original=engine::image+body.rva+boundary.offset;
        const uintptr_t copied=reinterpret_cast<uintptr_t>(runtime.arena)+body.offset+boundary.offset;
        DWORD64 sourceBase=0,copyBase=0;
        auto source=runtime.lookup(original,&sourceBase,nullptr),target=runtime.lookup(copied,&copyBase,nullptr);
        if(!source||!target||sourceBase!=engine::image||copyBase!=engine::image)return false;
        for(DWORD mode: {DWORD(0),DWORD(UNW_FLAG_UHANDLER)}){
            CONTEXT a{},b{};a.ContextFlags=CONTEXT_FULL;a.Rip=original;a.Rsp=reinterpret_cast<DWORD64>(stack.data()+32);b=a;b.Rip=copied;
            void *sourceData=nullptr,*copyData=nullptr;DWORD64 sourceFrame=0,copyFrame=0;
            auto h1=runtime.unwind(mode,sourceBase,original,source,&a,&sourceData,&sourceFrame,nullptr);
            auto h2=runtime.unwind(mode,copyBase,copied,target,&b,&copyData,&copyFrame,nullptr);
            if(!SameContext(a,b)||sourceFrame!=copyFrame||h1!=h2)return false;
            if(h1&&(Read<uint32_t>(sourceData)!=Read<uint32_t>(copyData)))return false;
            ++state.unwindCases;
        }
    }
    state.unwindVerified=true;return true;
}
inline bool BuildRuntime(){
    if(state.built)return true;
    if(!VerifySource()||!Mitigations()||!Handler())return false;
    runtime.add=Api<AddTableFn>(L"ntdll.dll","RtlAddFunctionTable");runtime.remove=Api<DeleteTableFn>(L"ntdll.dll","RtlDeleteFunctionTable");
    runtime.lookup=Api<LookupFn>(L"ntdll.dll","RtlLookupFunctionEntry");runtime.unwind=Api<UnwindFn>(L"ntdll.dll","RtlVirtualUnwind");
    if(!runtime.add||!runtime.remove||!runtime.lookup||!runtime.unwind)return Fail(5968,"ip_ack_compat_dynamic_unwind_api_unavailable");
    SYSTEM_INFO sys{};GetSystemInfo(&sys);const uintptr_t gran=sys.dwAllocationGranularity;
    const uintptr_t first=(engine::image+generated::ImageSize+gran-1)&~(gran-1);
    const uintptr_t limit=first+0x40000000ULL;
    for(uintptr_t address=first;address<limit;){
        MEMORY_BASIC_INFORMATION m{};if(!VirtualQuery(reinterpret_cast<void*>(address),&m,sizeof(m)))break;
        const auto end=reinterpret_cast<uintptr_t>(m.BaseAddress)+m.RegionSize;
        if(m.State==MEM_FREE&&end>=address+generated::ArenaSize){
            // Seed CFG's invalid bitmap without ever making writable code
            // executable. The unpublished, initially zero arena becomes RW
            // before any instruction or metadata bytes are copied into it.
            runtime.arena=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(address),generated::ArenaSize,MEM_COMMIT|MEM_RESERVE,
                state.cfgEnabled?(PAGE_EXECUTE_READ|PAGE_TARGETS_INVALID):PAGE_READWRITE));
            if(runtime.arena)break;
        }
        if(end<=address)break;address=(end+gran-1)&~(gran-1);
    }
    if(!runtime.arena)return Fail(5969,"ip_ack_compat_near_original_image_arena_unavailable");
    state.arena=reinterpret_cast<uintptr_t>(runtime.arena);
    if(state.cfgEnabled){
        DWORD previous=0;
        if(!VirtualProtect(runtime.arena,generated::ArenaSize,PAGE_READWRITE,&previous)){
            DiscardRuntime();return Fail(5994,"ip_ack_compat_nonexecutable_staging_protection_failed");
        }
    }
    for(const auto& body:generated::Bodies)std::memcpy(runtime.arena+body.offset,body.bytes,body.size);
    for(const auto& patch:generated::Patches){
        auto instruction=runtime.arena+generated::Bodies[patch.body].offset+patch.instruction;
        const uintptr_t target=patch.kind==2?state.arena+generated::RelayOffset:
            (patch.kind==1?engine::image+patch.target:MapTarget(patch.target));
        const int64_t difference=static_cast<int64_t>(target)-static_cast<int64_t>(reinterpret_cast<uintptr_t>(instruction)+patch.instructionSize);
        if(!Difference(instruction+patch.field,patch.size,difference)){DiscardRuntime();return Fail(5970,"ip_ack_compat_relative_operand_out_of_range");}
    }
    // Source call -> leaf relay -> ORIGINAL CFG dispatcher -> typed callback.
    auto relay=runtime.arena+generated::RelayOffset;relay[0]=0x48;relay[1]=0xb8;
    auto callback=reinterpret_cast<uintptr_t>(&Warning);std::memcpy(relay+2,&callback,8);relay[10]=0xff;relay[11]=0x25;
    if(!Difference(relay+12,4,static_cast<int64_t>(engine::image+generated::GuardDispatch)-static_cast<int64_t>(reinterpret_cast<uintptr_t>(relay)+16))
       ||!Executable(Read<void*>(reinterpret_cast<void*>(engine::image),generated::GuardDispatch))){
        DiscardRuntime();return Fail(5971,"ip_ack_compat_original_cfg_dispatch_unverified");
    }
    runtime.table=reinterpret_cast<PRUNTIME_FUNCTION>(runtime.arena+generated::RuntimeTableOffset);
    const auto arenaRva=static_cast<uint32_t>(state.arena-engine::image);
    for(size_t n=0;n<std::size(generated::Unwinds);++n){
        const auto& u=generated::Unwinds[n];auto bytes=runtime.arena+u.offset;std::memcpy(bytes,u.bytes,u.size);
        auto& row=runtime.table[n];const auto bodyOffset=static_cast<uint32_t>(generated::Bodies[u.body].offset);
        row.BeginAddress=arenaRva+bodyOffset+u.begin;row.EndAddress=arenaRva+bodyOffset+u.end;
        row.UnwindData=arenaRva+static_cast<uint32_t>(u.offset);
        if(u.chain>=0){
            const auto& parent=generated::Unwinds[static_cast<size_t>(u.chain)];
            RUNTIME_FUNCTION chain{};chain.BeginAddress=arenaRva+static_cast<uint32_t>(generated::Bodies[parent.body].offset)+parent.begin;
            chain.EndAddress=arenaRva+static_cast<uint32_t>(generated::Bodies[parent.body].offset)+parent.end;
            chain.UnwindData=arenaRva+static_cast<uint32_t>(parent.offset);std::memcpy(bytes+u.size-sizeof(chain),&chain,sizeof(chain));
        }
    }
    const size_t leaf=std::size(generated::Unwinds);const size_t leafInfo=0x1600;
    runtime.arena[leafInfo]=1;
    runtime.table[leaf]={arenaRva+static_cast<DWORD>(generated::RelayOffset),arenaRva+static_cast<DWORD>(generated::RelayOffset+16),arenaRva+static_cast<DWORD>(leafInfo)};
    DWORD previous=0;
    if(!VirtualProtect(runtime.arena,generated::CodePageSize,PAGE_EXECUTE_READ|(state.cfgEnabled?PAGE_TARGETS_NO_UPDATE:0),&previous)
       ||!VirtualProtect(runtime.arena+generated::CodePageSize,generated::ArenaSize-generated::CodePageSize,PAGE_READONLY,&previous)
       ||!FlushInstructionCache(GetCurrentProcess(),runtime.arena,generated::CodePageSize)){
        DiscardRuntime();return Fail(5972,"ip_ack_compat_private_code_protection_failed");
    }
    if(state.cfgEnabled){
        // Official provider is KernelBase; Kernel32 has no such export on this
        // machine (controlled fixture lookup returns ERROR_PROC_NOT_FOUND).
        state.cfg.provider=reinterpret_cast<uintptr_t>(GetModuleHandleW(L"kernelbase.dll"));
        SetLastError(ERROR_SUCCESS);auto targets=Api<TargetsFn>(L"kernelbase.dll","SetProcessValidCallTargets");
        state.cfg.api=reinterpret_cast<uintptr_t>(targets);
        state.cfg.lookupError=targets?ERROR_SUCCESS:GetLastError();
        CFG_CALL_TARGET_INFO entries[3]{{0,CFG_CALL_TARGET_VALID},{0x100,CFG_CALL_TARGET_VALID},{generated::RelayOffset,CFG_CALL_TARGET_VALID}};
        state.cfg.requestedBase=reinterpret_cast<uintptr_t>(runtime.arena);state.cfg.requestedBytes=generated::CodePageSize;
        MEMORY_BASIC_INFORMATION memory{};SetLastError(ERROR_SUCCESS);
        state.cfg.queryValid=VirtualQuery(runtime.arena,&memory,sizeof(memory))!=0;
        state.cfg.queryError=state.cfg.queryValid?ERROR_SUCCESS:GetLastError();
        if(state.cfg.queryValid){
            state.cfg.queryBase=reinterpret_cast<uintptr_t>(memory.BaseAddress);state.cfg.allocationBase=reinterpret_cast<uintptr_t>(memory.AllocationBase);
            state.cfg.queryRegionBytes=memory.RegionSize;state.cfg.protect=memory.Protect;state.cfg.allocationProtect=memory.AllocationProtect;
            state.cfg.regionState=memory.State;state.cfg.regionType=memory.Type;
        }
        for(size_t n=0;n<std::size(entries);++n)state.cfg.targets[n]={entries[n].Offset,entries[n].Flags,entries[n].Flags};
        if(targets){
            SetLastError(ERROR_SUCCESS);state.cfg.callAttempted=true;
            state.cfg.succeeded=targets(GetCurrentProcess(),runtime.arena,generated::CodePageSize,3,entries)!=FALSE;
            state.cfg.callError=state.cfg.succeeded?ERROR_SUCCESS:GetLastError();
            for(size_t n=0;n<std::size(entries);++n)state.cfg.targets[n].after=entries[n].Flags;
        }
        if(!targets||!state.cfg.succeeded){
            DiscardRuntime();return Fail(5973,"ip_ack_compat_private_cfg_targets_not_registered");
        }
        state.cfgRegistered=true;
    }
    if(!runtime.add(runtime.table,static_cast<DWORD>(leaf+1),engine::image)){
        DiscardRuntime();return Fail(5974,"ip_ack_compat_runtime_function_table_not_registered");
    }
    runtime.registered=true;
    if(!CheckUnwind()){DiscardRuntime();return Fail(5975,"ip_ack_compat_original_and_relocated_unwind_differ");}
    runtime.normal=reinterpret_cast<AckFn>(runtime.arena);state.built=true;Cache();return true;
}
inline bool Close(){
    if(!Thread())return false;
    if(binding.inFlight||warningFrame)return Fail(5976,"ip_ack_compat_retire_during_native_ack");
    if(binding.copy){
        if(Live(binding.pawn,binding.identities[3])){
            if(Read<void**>(binding.pawn)!=binding.copy)return Fail(5977,"ip_ack_compat_retire_health_or_other_layer_first");
            if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(binding.pawn),binding.source,binding.copy)!=binding.copy)
                return Fail(5978,"ip_ack_compat_private_table_restore_conflict");
            ++state.restoredTables;
        }else ++state.retiredWeakUnavailable;
        // Ordinary weak-null includes pending-kill objects that can still
        // receive destructor virtual calls. Keep every published table backed
        // until this owned process exits, even after a successful live restore.
        // Its entry points are original functions or static NormalAck, never
        // a per-epoch runtime.normal address; inFlight==0 above permits retiring
        // the separate relocated-body arena without a dangling table entry.
        ++state.retainedPublishedTables;
    }
    binding={};state.installed=false;state.context=false;state.trackerBound=false;state.retiring=false;
    if(!DiscardRuntime())return false;++state.closes;
    state.stage="ip_ack_compat_closed_binding_retired";Cache();return true;
}
inline bool InstallImpl(void* world,void* pc,void* pawn){
    if(!Thread())return Fail(5979,"ip_ack_compat_wrong_original_game_thread");
    if(state.error)return false;
    if(binding.active){
        if(binding.world!=world||binding.pc!=pc||binding.pawn!=pawn)
            return Fail(5980,"ip_ack_compat_retire_health_then_compat_before_new_epoch");
        if(!WeakMatch(binding)||!Owned(binding))return Fail(5981,"ip_ack_compat_bound_original_context_changed");
        const bool active=Read<uint8_t>(binding.movement,0x13d8)!=0;
        if(active!=binding.reportedActive&&!Deliver(binding,0,active?uint8_t(1):uint8_t(0)))return false;
        return true;
    }
    if(!world||Read<uint8_t>(pc,0x224)!=2||Read<uint8_t>(pawn,0x224)!=2){
        state.stage="ip_ack_compat_waiting_actual_client";Cache();return true;
    }
    if(!VerifySource()||!VerifyMetadata())return false;
    Binding next;next.world=world;next.pc=pc;next.pawn=pawn;next.ps=Read<void*>(pc,0x470);
    next.driver=Read<void*>(world,0x40);next.connection=Read<void*>(next.driver,0xc0);next.player=Read<void*>(pc,0x510);
    next.gameState=Read<void*>(world,0x178);next.movement=Read<void*>(pawn,0x4f8);
    if(!original_players::IsA(pc,original_players::ShooterControllerClass)
       ||!original_players::IsA(pawn,original_players::ShooterCharacterClass)
       ||!original_players::IsA(next.movement,0x6c7d380)||!original_players::IsA(next.gameState,0x6c9d280))
        return Fail(5982,"ip_ack_compat_original_actor_classes_invalid");
    if(!Event(next.gameState,provider,&next.component)||next.component!=Read<void*>(next.gameState,0xc58)
       ||!original_players::IsA(next.component,0x66335c0)
       ||!Event(next.component,trackerGetter,&next.tracker)||next.tracker!=Read<void*>(next.component,0x168)
       ||!original_players::IsA(next.tracker,0x68d85b0))
        return Fail(5983,"ip_ack_compat_actual_original_tracker_provider_missing");
    if(!Owned(next))return Fail(5984,"ip_ack_compat_requires_unique_owned_generic_ip_client");
    auto componentWorld=Read<void*>(Read<void*>(next.component),0x1b8),trackerWorld=Read<void*>(Read<void*>(next.tracker),0x1b8);
    if(componentWorld!=reinterpret_cast<void*>(engine::image+0x2a64ee0)||trackerWorld!=reinterpret_cast<void*>(engine::image+0x1c53940)
       ||reinterpret_cast<void*(*)(void*)>(componentWorld)(next.component)!=world
       ||reinterpret_cast<void*(*)(void*)>(trackerWorld)(next.tracker)!=world)
        return Fail(5985,"ip_ack_compat_tracker_owner_world_mismatch");
    void* objects[]{next.world,next.pc,next.ps,next.pawn,next.driver,next.connection,next.player,next.gameState,next.component,next.tracker,next.movement};
    for(size_t n=0;n<std::size(objects);++n)if(!Capture(objects[n],next.identities[n]))return Fail(5986,"ip_ack_compat_original_weak_identity_capture_failed");
    next.source=Read<void**>(pawn);
    if(Read<uintptr_t>(next.source,AckSlot*8)!=engine::image+0x787c310
       ||Read<uintptr_t>(next.source,GradualSlot*8)!=engine::image+0x787c3f0
       ||Read<uintptr_t>(next.source,0xad8)!=engine::image+0x787a070
       ||!original_players::Readable(next.source-1,(PawnEntries+1)*8))
        return Fail(5987,"ip_ack_compat_install_before_input_health_or_unknown_ack_layer");
    if(beforePawnController&&Read<uintptr_t>(next.source,PawnControllerSlot*8)!=engine::image+0x7885040)
        return Fail(5994,"ip_ack_compat_original_pawn_controller_lifecycle_slot_changed");
    if(!BuildRuntime())return false;
    auto storage=static_cast<void**>(VirtualAlloc(nullptr,(PawnEntries+1)*8,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!storage)return Fail(5988,"ip_ack_compat_private_table_allocation_failed");
    std::memcpy(storage,next.source-1,(PawnEntries+1)*8);next.copy=storage+1;next.copy[AckSlot]=reinterpret_cast<void*>(&NormalAck);
    if(beforePawnController)next.copy[PawnControllerSlot]=reinterpret_cast<void*>(&PawnControllerChanged);
    DWORD previous=0;
    if(!VirtualProtect(storage,(PawnEntries+1)*8,PAGE_READONLY,&previous)){
        VirtualFree(storage,0,MEM_RELEASE);return Fail(5989,"ip_ack_compat_private_table_protection_failed");
    }
    binding=next;binding.active=true;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(pawn),binding.copy,binding.source)!=binding.source){
        VirtualFree(storage,0,MEM_RELEASE);binding={};return Fail(5990,"ip_ack_compat_private_table_publication_conflict");
    }
    state.installed=state.context=state.trackerBound=true;state.retiring=false;state.tracker=reinterpret_cast<uintptr_t>(next.tracker);
    state.component=reinterpret_cast<uintptr_t>(next.component);++state.installs;
    state.stage="ip_ack_compat_original_native_bodies_and_real_tracker_bound";Cache();return true;
}
inline bool Install(void* world,void* pc,void* pawn){
    __try{return InstallImpl(world,pc,pawn);}
    __except(EXCEPTION_EXECUTE_HANDLER){return Fail(5991,"ip_ack_compat_validation_exception_requires_stop");}
}
inline void WriteMetadata(FILE* file,const State& s){
    auto yes=[](bool value){return value?"true":"false";};
    constexpr const char* functions[]{"GetClientPerformanceMonitoringComponent","GetServerResponsivenessInstabilityTrackingDetails","OnAresNetworkErrorStateChanged"};
    constexpr const char* fields[]{"LocalPerformanceMonitoringComponent","ServerResponsiveness"};
    std::fprintf(file,"{\"matched_mask\":%u,\"required_mask\":31,\"first_failure\":%u,\"functions\":[",s.metadataMatchedMask,s.metadataFirstFailure);
    for(size_t n=0;n<s.functions.size();++n){
        const auto& f=s.functions[n];if(n)std::fputc(',',file);
        std::fprintf(file,"{\"name\":\"%s\",\"found\":%s,\"valid\":%s,\"object\":\"0x%llx\",\"owner\":\"0x%llx\",\"expected_owner\":\"0x%llx\",\"owner_matches\":%s,\"parameter_bytes\":%u,\"expected_parameter_bytes\":%u,\"parameter_bytes_match\":%s,\"exec\":\"0x%llx\",\"expected_exec\":\"0x%llx\",\"exec_matches\":%s,\"flags\":\"0x%x\",\"native_flag\":%s}",
            functions[n],yes(f.found),yes(f.valid),static_cast<unsigned long long>(f.object),
            static_cast<unsigned long long>(f.actualOwner),static_cast<unsigned long long>(f.expectedOwner),yes(f.ownerMatches),
            unsigned(f.bytes),unsigned(f.expectedBytes),yes(f.bytesMatch),static_cast<unsigned long long>(f.actualExec),
            static_cast<unsigned long long>(f.expectedExec),yes(f.execMatches),f.flags,yes(f.nativeFlag));
    }
    std::fputs("],\"fields\":[",file);
    for(size_t n=0;n<s.fields.size();++n){
        const auto& f=s.fields[n];if(n)std::fputc(',',file);
        std::fprintf(file,"{\"name\":\"%s\",\"found\":%s,\"valid\":%s,\"owner\":\"0x%llx\",\"field\":\"0x%llx\",\"field_class\":\"0x%llx\",\"cast_flags\":\"0x%llx\",\"type_matches\":%s,\"actual_name\":\"0x%llx\",\"expected_name\":\"0x%llx\",\"name_matches\":%s,\"array_dim\":%d,\"array_matches\":%s,\"element_size\":%d,\"size_matches\":%s,\"offset\":%d,\"expected_offset\":%d,\"offset_matches\":%s}",
            fields[n],yes(f.found),yes(f.valid),static_cast<unsigned long long>(f.owner),static_cast<unsigned long long>(f.field),
            static_cast<unsigned long long>(f.fieldClass),static_cast<unsigned long long>(f.castFlags),yes(f.typeMatches),
            static_cast<unsigned long long>(f.name),static_cast<unsigned long long>(f.expectedName),yes(f.nameMatches),
            f.arrayDim,yes(f.arrayMatches),f.elementSize,yes(f.sizeMatches),f.offset,f.expectedOffset,yes(f.offsetMatches));
    }
    std::fputs("]}",file);
}
inline void WriteCfg(FILE* file,const CfgObservation& c){
    auto yes=[](bool value){return value?"true":"false";};
    std::fprintf(file,"{\"provider_name\":\"KernelBase.dll\",\"provider\":\"0x%llx\",\"api\":\"0x%llx\",\"lookup_error\":%lu,\"call_attempted\":%s,\"succeeded\":%s,\"call_error\":%lu,\"requested_base\":\"0x%llx\",\"requested_bytes\":%llu,\"query_valid\":%s,\"query_error\":%lu,\"query_base\":\"0x%llx\",\"allocation_base\":\"0x%llx\",\"region_bytes\":%llu,\"protect\":%lu,\"allocation_protect\":%lu,\"region_state\":%lu,\"region_type\":%lu,\"targets\":[",
        static_cast<unsigned long long>(c.provider),static_cast<unsigned long long>(c.api),c.lookupError,yes(c.callAttempted),yes(c.succeeded),c.callError,
        static_cast<unsigned long long>(c.requestedBase),c.requestedBytes,yes(c.queryValid),c.queryError,
        static_cast<unsigned long long>(c.queryBase),static_cast<unsigned long long>(c.allocationBase),c.queryRegionBytes,
        c.protect,c.allocationProtect,c.regionState,c.regionType);
    for(size_t n=0;n<c.targets.size();++n){if(n)std::fputc(',',file);
        const auto& t=c.targets[n];std::fprintf(file,"{\"offset\":%llu,\"flags_before\":%llu,\"flags_after\":%llu}",
            static_cast<unsigned long long>(t.offset),static_cast<unsigned long long>(t.before),static_cast<unsigned long long>(t.after));
    }
    std::fputs("]}",file);
}
inline void Write(FILE* file){
    const auto s=Snapshot();auto yes=[](bool v){return v?"true":"false";};
    std::fprintf(file,"{\"stage\":\"%s\",\"error\":%u,\"built\":%s,\"installed\":%s,\"source_profiles\":%s,\"metadata\":%s,\"actual_context\":%s,\"actual_tracker\":%s,\"handler_verified\":%s,\"unwind_verified\":%s,\"cfg_enabled\":%s,\"cfg_registered\":%s,\"dynamic_policy\":%lu,\"cfg_policy\":%lu,\"shadow_policy\":%lu,\"arena\":\"0x%llx\",\"handler_target\":\"0x%llx\",\"tracker\":\"0x%llx\",\"component\":\"0x%llx\",\"installs\":%llu,\"closes\":%llu,\"acks\":%llu,\"original_returns\":%llu,\"active_warnings\":%llu,\"inactive_warnings\":%llu,\"original_ares_forwards\":%llu,\"failed_warnings\":%llu,\"unwind_cases\":%llu,\"original_image_writes\":0,\"adapter_queue_writes\":0,\"adapter_ack_writes\":0",
        s.stage,s.error,yes(s.built),yes(s.installed),yes(s.profiles),yes(s.metadata),yes(s.context),yes(s.trackerBound),
        yes(s.handlerVerified),yes(s.unwindVerified),yes(s.cfgEnabled),yes(s.cfgRegistered),s.dynamicPolicy,s.cfgPolicy,s.shadowPolicy,
        static_cast<unsigned long long>(s.arena),static_cast<unsigned long long>(s.handlerTarget),static_cast<unsigned long long>(s.tracker),static_cast<unsigned long long>(s.component),
        s.installs,s.closes,s.acks,s.returns,s.activeWarnings,s.inactiveWarnings,s.originalAresForwards,s.failedWarnings,s.unwindCases);
    std::fprintf(file,",\"restored_tables\":%llu,\"retained_published_tables\":%llu,\"retired_weak_unavailable\":%llu",
        s.restoredTables,s.retainedPublishedTables,s.retiredWeakUnavailable);
    std::fprintf(file,",\"native_epoch_retiring\":%s,\"lifecycle_fences\":%llu,\"retiring_original_forwards\":%llu,\"pawn_controller_entries\":%llu,\"pawn_controller_returns\":%llu",
        yes(s.retiring),s.lifecycleFences,s.retiringOriginalForwards,s.pawnControllerEntries,s.pawnControllerReturns);
    std::fprintf(file,",\"original_pawn_lifecycle_exception_code\":%u,\"original_pawn_lifecycle_exception_rva\":\"0x%llx\"",s.pawnLifecycleExceptionCode,static_cast<unsigned long long>(s.pawnLifecycleExceptionRva));
    std::fputs(",\"cfg_registration\":",file);WriteCfg(file,s.cfg);
    std::fputs(",\"metadata_diagnostics\":",file);WriteMetadata(file,s);std::fputc('}',file);
}
} // namespace local_server_ip_ack_compat
