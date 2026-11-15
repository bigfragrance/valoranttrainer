#pragma once
#include "local_server_ip_ack_compat.h"
#include "local_server_input_warning_compat/generated.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <iterator>

// Candidate-only compatibility for the actual own generic-IP client. Five
// original input functions retain their input, protection, threshold, queue,
// clock and return algorithms. Only the setter's Ares-driver notification
// fragment is replaced with a real IP query + actual original tracker delivery.
// Original image bytes and native input/protection/queue data are never written.
namespace local_server_input_warning_compat {
namespace ack=local_server_ip_ack_compat;
using Weak=original_inventory_slots::WeakIdentity;
using TickFn=void(*)(void*,float,int32_t,void*);
using SerializeFn=uint8_t(*)(void*,void*);
inline constexpr size_t TickSlot=0x3f8/8,SerializeSlot=0x10/8;
inline constexpr size_t PrimaryEntries=155,InterfaceEntries=5;
struct OwnershipFailure {
    bool observed=false,getWorldCalled=false;const char* entry="none";uint64_t at=0,ordinal=0;
    uint64_t warningMask=0,ackMask=0,ackWeakMask=0;uint32_t thread=0,expectedThread=0,ackError=0;
    std::array<uintptr_t,40> pointers{};std::array<Weak,11> ackWeak{};std::array<uintptr_t,11> ackResolved{};
    Weak inputWeak{};uintptr_t inputResolved=0;uint32_t warningInFlight=0,ackInFlight=0;
    uint8_t pcRole=0,pawnRole=0,protection=0;int32_t connectionState=0,localCount=0;
};
struct State {
    unsigned error=0;const char* stage="input_warning_compat_not_installed";
    bool profiles=false,metadata=false,built=false,installed=false,unwindVerified=false,cfgRegistered=false;
    bool actualAckContext=false,actualInput=false,retiring=false;
    DWORD dynamicPolicy=0,cfgPolicy=0,shadowPolicy=0;
    uintptr_t arena=0,input=0;
    uint64_t installs=0,closes=0,ticks=0,tickReturns=0,serializations=0,serializeReturns=0;
    uint64_t warnings=0,activeWarnings=0,inactiveWarnings=0,serverQueries=0,failedWarnings=0,unwindCases=0;
    uint64_t retiredWeakUnavailable=0;uint32_t retainedSharedTables=0;
    uint64_t lifecycleFences=0,retiringOriginalTicks=0,retiringOriginalSerializations=0;
    uint8_t lastNativeProtection=0;
    ack::CfgObservation cfg{};
    OwnershipFailure ownershipFailure{};
};
inline State state{},cached{};inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline uint64_t lastCacheAt=0;
inline void Cache(){AcquireSRWLockExclusive(&cacheLock);cached=state;ReleaseSRWLockExclusive(&cacheLock);lastCacheAt=GetTickCount64();}
inline void CacheIfDue(){if(GetTickCount64()-lastCacheAt>=100)Cache();}
inline State Snapshot(){AcquireSRWLockShared(&cacheLock);auto copy=cached;ReleaseSRWLockShared(&cacheLock);return copy;}
inline const State& GetState(){return state;}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;Cache();return false;}
inline bool Thread(){return ack::Thread();}
template<class T> inline T Read(const void* p,size_t offset=0){return ack::Read<T>(p,offset);}
struct Runtime {
    uint8_t* arena=nullptr;PRUNTIME_FUNCTION table=nullptr;bool registered=false;
    ack::AddTableFn add=nullptr;ack::DeleteTableFn remove=nullptr;
    ack::LookupFn lookup=nullptr;ack::UnwindFn unwind=nullptr;
    TickFn tick=nullptr;SerializeFn serialize=nullptr;
};
inline Runtime runtime{};
// There are exactly two verified source tables for this native component type.
// Reuse their immutable private copies for every epoch, and keep the backing
// until the owned process exits. An ordinary weak resolver may reject an
// object pending destruction while it can still receive native virtual calls.
// The tables contain only original-image pointers and our static wrappers;
// they never contain a pointer into a per-binding relocated-code arena.
struct SharedTables {
    void **primarySource=nullptr,**primaryCopy=nullptr,**interfaceSource=nullptr,**interfaceCopy=nullptr;
};
inline SharedTables sharedTables{};
struct Binding {
    void *world=nullptr,*pc=nullptr,*pawn=nullptr,*input=nullptr,*interfaceObject=nullptr;
    Weak inputId{};
    void **primarySource=nullptr,**primaryCopy=nullptr,**interfaceSource=nullptr,**interfaceCopy=nullptr;
    bool active=false,primaryPublished=false,interfacePublished=false,retiring=false;
    uint32_t inFlight=0;
};
inline Binding binding{};inline Binding* notificationFrame=nullptr;
inline bool FenceNativeEpoch(){
    if(!Thread())return false;if(!binding.active)return true;
    if(!binding.retiring){binding.retiring=true;state.retiring=true;++state.lifecycleFences;Cache();}return true;
}
inline bool Owned(const Binding& b){
    return ack::IsOwnedAckEntry(b.pawn,reinterpret_cast<void*>(&ack::NormalAck))
        &&ack::binding.world==b.world&&ack::binding.pc==b.pc&&ack::binding.pawn==b.pawn
        &&ack::Live(b.input,b.inputId)&&Read<void*>(b.pawn,0xba0)==b.input
        &&original_players::IsA(b.input,0x6494370)
        &&Read<void*>(b.input,0xc8)==b.pawn&&Read<void*>(b.input,0x148)==b.pawn
        &&b.interfaceObject==static_cast<uint8_t*>(b.input)+0xd8
        &&Read<void*>(Read<void*>(b.input),0x1b8)==reinterpret_cast<void*>(engine::image+0x2a64ee0)
        &&engine::function<void*(*)(void*)>(0x2a64ee0)(b.input)==b.world;
}
inline constexpr const char* AckGateNames[]{
    "ack_no_error","ack_installed","ack_built","original_game_thread","ack_binding_active","ack_pawn_matches_warning","all_ack_weak_live",
    "actual_local_player_type","actual_ip_connection_type","actual_player_state_type","world_driver","driver_world","original_ip_driver_class",
    "driver_server_connection","connection_driver","connection_owning_pc","connection_open_state3","pc_role2","pawn_role2","adapter_active_pc",
    "adapter_active_pawn","one_local_player","local_array_first","adapter_local_player","local_player_pc","pc_local_player","pc_player_state",
    "player_state_owner_pc","pc_pawn","pawn_controller","pawn_player_state","pawn_movement","movement_owner","world_game_state",
    "game_state_monitor","monitor_owner","monitor_tracker","tracker_outer"
};
inline constexpr const char* WarningGateNames[]{"ack_owned_entry","same_world","same_pc","same_pawn","input_weak_live","pawn_input",
    "original_input_type","component_owner","character_owner","interface_address","original_world_getter","actual_input_world"};
inline constexpr const char* FailurePointerNames[]{"warning_world","warning_pc","warning_pawn","warning_input","warning_interface",
    "ack_world","ack_pc","ack_ps","ack_pawn","ack_driver","ack_connection","ack_local_player","ack_game_state","ack_monitor","ack_tracker","ack_movement",
    "adapter_active_pc","adapter_active_pawn","adapter_local_player","actual_pc_pawn","actual_pc_ps","actual_pawn_controller","actual_pawn_ps",
    "actual_ps_spawned","actual_ps_possessed","actual_pawn_input","actual_input_owner","actual_input_character","actual_pawn_table","actual_primary_table",
    "actual_interface_table","actual_pawn_ack_entry","ack_expected_table","warning_expected_primary","warning_expected_interface","actual_input_world",
    "actual_game_instance","actual_local_player_array","actual_driver_server_connection","actual_world_driver"};
inline constexpr const char* AckWeakNames[]{"world","pc","ps","pawn","driver","connection","local_player","game_state","monitor","tracker","movement"};
inline void CaptureOwnershipFailure(const Binding& b,const char* entry,uint64_t ordinal){
    if(state.ownershipFailure.observed)return;
    OwnershipFailure d{};d.observed=true;d.entry=entry;d.at=GetTickCount64();d.ordinal=ordinal;
    d.thread=GetCurrentThreadId();d.expectedThread=original_players::gameThread;d.ackError=ack::state.error;
    const auto& a=ack::binding;auto gi=Read<void*>(a.world,0x1d8);auto locals=Read<void**>(gi,0x40);
    const void* objects[]{a.world,a.pc,a.ps,a.pawn,a.driver,a.connection,a.player,a.gameState,a.component,a.tracker,a.movement};
    d.ackWeak=a.identities;
    for(size_t n=0;n<std::size(objects);++n){auto resolved=original_inventory_slots::ResolveIdentity(a.identities[n]);d.ackResolved[n]=reinterpret_cast<uintptr_t>(resolved);
        if(objects[n]&&resolved==objects[n])d.ackWeakMask|=uint64_t(1)<<n;}
    d.inputWeak=b.inputId;d.inputResolved=reinterpret_cast<uintptr_t>(original_inventory_slots::ResolveIdentity(b.inputId));
    const bool ackTests[]{!ack::state.error,ack::state.installed,ack::state.built,Thread(),a.active,a.pawn==b.pawn,d.ackWeakMask==0x7ff,
        original_players::IsA(a.player,0x3b6e680),original_players::IsA(a.connection,0x5112be0),original_players::IsA(a.ps,original_players::ShooterStateClass),
        Read<void*>(a.world,0x40)==a.driver,Read<void*>(a.driver,0x178)==a.world,Read<void*>(a.driver,0x10)==engine::Class(0x5112c90),
        Read<void*>(a.driver,0xc0)==a.connection,Read<void*>(a.connection,0x60)==a.driver,Read<void*>(a.connection,0xa0)==a.pc,Read<int32_t>(a.connection,0x13c)==3,
        Read<uint8_t>(a.pc,0x224)==2,Read<uint8_t>(a.pawn,0x224)==2,original_players::activeController==a.pc,original_players::activePawn==a.pawn,
        Read<int32_t>(gi,0x48)==1,locals&&Read<void*>(locals)==a.player,a.player==original_players::localPlayer,
        Read<void*>(a.player,0x38)==a.pc,Read<void*>(a.pc,0x510)==a.player,Read<void*>(a.pc,0x470)==a.ps,Read<void*>(a.ps,0x210)==a.pc,
        Read<void*>(a.pc,0x4b0)==a.pawn,Read<void*>(a.pawn,0x4a0)==a.pc,Read<void*>(a.pawn,0x488)==a.ps,
        Read<void*>(a.pawn,0x4f8)==a.movement,Read<void*>(a.movement,0xc8)==a.pawn,Read<void*>(a.world,0x178)==a.gameState,
        Read<void*>(a.gameState,0xc58)==a.component,Read<void*>(a.component,0xc8)==a.gameState,Read<void*>(a.component,0x168)==a.tracker,Read<void*>(a.tracker,0x28)==a.component};
    static_assert(std::size(ackTests)==std::size(AckGateNames));
    for(size_t n=0;n<std::size(ackTests);++n)if(ackTests[n])d.ackMask|=uint64_t(1)<<n;
    auto getter=Read<void*>(Read<void*>(b.input),0x1b8);void* actualWorld=nullptr;
    if(b.input&&d.inputResolved==reinterpret_cast<uintptr_t>(b.input)&&a.pawn==b.pawn&&d.ackWeakMask==0x7ff
       &&getter==reinterpret_cast<void*>(engine::image+0x2a64ee0)){
        actualWorld=engine::function<void*(*)(void*)>(0x2a64ee0)(b.input);d.getWorldCalled=true;
    }
    const bool warningTests[]{ack::IsOwnedAckEntry(b.pawn,reinterpret_cast<void*>(&ack::NormalAck)),a.world==b.world,a.pc==b.pc,a.pawn==b.pawn,
        b.input&&d.inputResolved==reinterpret_cast<uintptr_t>(b.input),Read<void*>(b.pawn,0xba0)==b.input,original_players::IsA(b.input,0x6494370),
        Read<void*>(b.input,0xc8)==b.pawn,Read<void*>(b.input,0x148)==b.pawn,b.input&&b.interfaceObject==static_cast<uint8_t*>(b.input)+0xd8,
        getter==reinterpret_cast<void*>(engine::image+0x2a64ee0),d.getWorldCalled&&actualWorld==b.world};
    static_assert(std::size(warningTests)==std::size(WarningGateNames));
    for(size_t n=0;n<std::size(warningTests);++n)if(warningTests[n])d.warningMask|=uint64_t(1)<<n;
    const void* pointers[]{b.world,b.pc,b.pawn,b.input,b.interfaceObject,a.world,a.pc,a.ps,a.pawn,a.driver,a.connection,a.player,a.gameState,a.component,a.tracker,a.movement,
        original_players::activeController,original_players::activePawn,original_players::localPlayer,Read<void*>(b.pc,0x4b0),Read<void*>(b.pc,0x470),Read<void*>(b.pawn,0x4a0),Read<void*>(b.pawn,0x488),
        Read<void*>(a.ps,0xbc8),Read<void*>(a.ps,0xbd0),Read<void*>(b.pawn,0xba0),Read<void*>(b.input,0xc8),Read<void*>(b.input,0x148),Read<void*>(b.pawn),Read<void*>(b.input),
        Read<void*>(b.interfaceObject),Read<void*>(Read<void*>(b.pawn),ack::AckSlot*8),a.copy,b.primaryCopy,b.interfaceCopy,actualWorld,gi,locals,Read<void*>(a.driver,0xc0),Read<void*>(a.world,0x40)};
    static_assert(std::size(pointers)==std::size(FailurePointerNames)&&std::size(pointers)==40);
    for(size_t n=0;n<std::size(pointers);++n)d.pointers[n]=reinterpret_cast<uintptr_t>(pointers[n]);
    d.pcRole=Read<uint8_t>(b.pc,0x224);d.pawnRole=Read<uint8_t>(b.pawn,0x224);d.protection=Read<uint8_t>(b.input,0x12c);
    d.connectionState=Read<int32_t>(a.connection,0x13c);d.localCount=Read<int32_t>(gi,0x48);d.warningInFlight=b.inFlight;d.ackInFlight=a.inFlight;
    state.ownershipFailure=d;
}
inline bool VerifySource(){
    if(state.profiles)return true;
    for(const auto& span:generated::SourceSpans){
        const auto actual=reinterpret_cast<void*>(engine::image+span.rva);
        if(!original_players::Readable(actual,span.size)||std::memcmp(actual,span.bytes,span.size))
            return Fail(6004,"input_warning_compat_original_source_profile_mismatch");
    }
    state.profiles=true;return true;
}
inline bool VerifyMetadata(){
    if(state.metadata)return true;
    if(!ack::Field(engine::Class(original_players::ShooterCharacterClass),L"InputStateComponent",0xba0)
       ||!ack::Field(engine::Class(0x6494370),L"ShooterCharacterOwner",0x148))
        return Fail(6003,"input_warning_compat_original_input_owner_metadata_mismatch");
    state.metadata=true;return true;
}
inline bool OwnedEnteredRetiringFrame(const Binding& b){
    // No new relocation admission. This permits only the already-entered frame
    // while its FULL original tuple still owns the same old tracker. It never
    // substitutes a new pawn/tracker when a native parent changes the links.
    return notificationFrame==&b&&b.inFlight&&b.active&&b.retiring&&ack::binding.retiring
        &&!ack::state.error&&ack::state.installed&&ack::state.built&&ack::binding.active
        &&ack::WeakMatch(ack::binding)&&ack::Owned(ack::binding)
        &&ack::binding.world==b.world&&ack::binding.pc==b.pc&&ack::binding.pawn==b.pawn
        &&ack::Live(b.input,b.inputId)&&Read<void*>(b.pawn,0xba0)==b.input&&original_players::IsA(b.input,0x6494370)
        &&Read<void*>(b.input,0xc8)==b.pawn&&Read<void*>(b.input,0x148)==b.pawn
        &&b.interfaceObject==static_cast<uint8_t*>(b.input)+0xd8
        &&Read<void*>(Read<void*>(b.input),0x1b8)==reinterpret_cast<void*>(engine::image+0x2a64ee0)
        &&engine::function<void*(*)(void*)>(0x2a64ee0)(b.input)==b.world;
}
inline void Notify(void* input,uint8_t active){
    auto* b=notificationFrame;
    if(!b||!b->active||b->input!=input||!Thread()||(!Owned(*b)&&!OwnedEnteredRetiringFrame(*b))||active>1){
        if(b&&b->active)CaptureOwnershipFailure(*b,b->retiring?"retiring_inflight_notify":"notify",state.warnings+1);
        ++state.failedWarnings;Fail(6026,"input_warning_compat_notification_outside_actual_owned_frame");return;
    }
    // This is the value set by the immediately preceding ORIGINAL setter.
    // Never edit or mask the actual input-protection state.
    state.lastNativeProtection=Read<uint8_t>(input,0x12c);
    if(state.lastNativeProtection!=active){
        ++state.failedWarnings;Fail(6027,"input_warning_compat_original_protection_value_not_observed");return;
    }
    const auto driver=ack::binding.driver;
    auto query=Read<void*>(Read<void*>(driver),0x430);
    if(query!=reinterpret_cast<void*>(engine::image+0x47387b0)){
        ++state.failedWarnings;Fail(6028,"input_warning_compat_actual_ip_server_query_not_original");return;
    }
    ++state.serverQueries;
    // Original inherited UNetDriver::IsServer checks the real +C0 connection.
    // Preserve that original predicate on the real IpNetDriver, without an
    // Ares cast, replacement driver, role write or counterfeit return value.
    if(!reinterpret_cast<bool(*)(void*)>(query)(driver)){
        if(!ack::Deliver(ack::binding,1,active)){
            ++state.failedWarnings;Fail(6029,"input_warning_compat_actual_original_tracker_delivery_failed");return;
        }
        ++state.warnings;if(active)++state.activeWarnings;else ++state.inactiveWarnings;
    }
    Cache();
}
inline void Tick(void* input,float delta,int32_t tickType,void* tickFunction){
    auto* b=binding.active&&binding.input==input&&Thread()?&binding:nullptr;
    if(!b){engine::function<TickFn>(0x70a7180)(input,delta,tickType,tickFunction);return;}
    if(b->retiring){++state.retiringOriginalTicks;engine::function<TickFn>(0x70a7180)(input,delta,tickType,tickFunction);return;}
    if(!Owned(*b)){
        CaptureOwnershipFailure(*b,"tick",state.ticks+1);
        Fail(6032,"input_warning_compat_tick_epoch_changed_requires_stop");
        engine::function<TickFn>(0x70a7180)(input,delta,tickType,tickFunction);return;
    }
    ++b->inFlight;++state.ticks;auto previous=notificationFrame;notificationFrame=b;
    __try{runtime.tick(input,delta,tickType,tickFunction);++state.tickReturns;}
    __finally{notificationFrame=previous;--b->inFlight;CacheIfDue();}
}
inline uint8_t Serialize(void* inputInterface,void* archive){
    auto* b=binding.active&&binding.interfaceObject==inputInterface&&Thread()?&binding:nullptr;
    if(!b)return engine::function<SerializeFn>(0x70a8c30)(inputInterface,archive);
    if(b->retiring){++state.retiringOriginalSerializations;return engine::function<SerializeFn>(0x70a8c30)(inputInterface,archive);}
    if(!Owned(*b)){
        CaptureOwnershipFailure(*b,"serialize",state.serializations+1);
        Fail(6033,"input_warning_compat_serializer_epoch_changed_requires_stop");
        return engine::function<SerializeFn>(0x70a8c30)(inputInterface,archive);
    }
    uint8_t result=0;++b->inFlight;++state.serializations;auto previous=notificationFrame;notificationFrame=b;
    __try{result=runtime.serialize(inputInterface,archive);++state.serializeReturns;}
    __finally{notificationFrame=previous;--b->inFlight;CacheIfDue();}
    return result;
}
inline bool DiscardRuntime(){
    if(runtime.registered&&(!runtime.remove||!runtime.remove(runtime.table)))
        return Fail(6024,"input_warning_compat_unregister_failed_runtime_retained");
    if(runtime.arena)VirtualFree(runtime.arena,0,MEM_RELEASE);
    runtime={};state.built=false;state.arena=0;state.unwindVerified=false;state.cfgRegistered=false;return true;
}
inline uintptr_t MapTarget(uintptr_t target){
    for(const auto& body:generated::Bodies)if(target>=body.rva&&target<body.rva+body.size)
        return reinterpret_cast<uintptr_t>(runtime.arena)+body.offset+target-body.rva;
    return engine::image+target;
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
        for(DWORD mode:{DWORD(0),DWORD(UNW_FLAG_UHANDLER)}){
            CONTEXT a{},b{};a.ContextFlags=CONTEXT_FULL;a.Rip=original;a.Rsp=reinterpret_cast<DWORD64>(stack.data()+32);b=a;b.Rip=copied;
            void *sourceData=nullptr,*copyData=nullptr;DWORD64 sourceFrame=0,copyFrame=0;
            auto h1=runtime.unwind(mode,sourceBase,original,source,&a,&sourceData,&sourceFrame,nullptr);
            auto h2=runtime.unwind(mode,copyBase,copied,target,&b,&copyData,&copyFrame,nullptr);
            if(!ack::SameContext(a,b)||sourceFrame!=copyFrame||h1!=h2)return false;
            if(h1&&Read<uint32_t>(sourceData)!=Read<uint32_t>(copyData))return false;
            ++state.unwindCases;
        }
    }
    // Validate the production relay's own leaf record too. It changes no
    // stack/nonvolatile state; an unwind must pop exactly the caller return.
    for(size_t offset:{size_t(0),size_t(10)}){
        const uintptr_t pc=reinterpret_cast<uintptr_t>(runtime.arena)+generated::RelayOffset+offset;
        DWORD64 imageBase=0;auto entry=runtime.lookup(pc,&imageBase,nullptr);
        if(!entry||imageBase!=engine::image)return false;
        for(DWORD mode:{DWORD(0),DWORD(UNW_FLAG_UHANDLER)}){
            CONTEXT actual{};actual.ContextFlags=CONTEXT_FULL;actual.Rip=pc;actual.Rsp=reinterpret_cast<DWORD64>(stack.data()+32);
            auto expected=actual;expected.Rip=stack[32];expected.Rsp+=8;
            void* data=nullptr;DWORD64 frame=0;
            auto handler=runtime.unwind(mode,imageBase,pc,entry,&actual,&data,&frame,nullptr);
            if(handler||!ack::SameContext(actual,expected)||frame!=reinterpret_cast<DWORD64>(stack.data()+32))return false;
            ++state.unwindCases;
        }
    }
    state.unwindVerified=true;return true;
}
inline bool Mitigations(){
    auto policy=ack::Api<ack::PolicyFn>(L"kernel32.dll","GetProcessMitigationPolicy");
    PROCESS_MITIGATION_DYNAMIC_CODE_POLICY dynamic{};PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY cfg{};
    PROCESS_MITIGATION_USER_SHADOW_STACK_POLICY shadow{};
    if(!policy||!policy(GetCurrentProcess(),ProcessDynamicCodePolicy,&dynamic,sizeof(dynamic))
       ||!policy(GetCurrentProcess(),ProcessControlFlowGuardPolicy,&cfg,sizeof(cfg))
       ||!policy(GetCurrentProcess(),ProcessUserShadowStackPolicy,&shadow,sizeof(shadow)))
        return Fail(6008,"input_warning_compat_actual_mitigation_query_failed");
    state.dynamicPolicy=dynamic.Flags;state.cfgPolicy=cfg.Flags;state.shadowPolicy=shadow.Flags;
    const auto& original=ack::GetState();
    if(dynamic.ProhibitDynamicCode||cfg.EnableXfg||!ack::ShadowPolicySupported(shadow)
       ||dynamic.Flags!=original.dynamicPolicy||cfg.Flags!=original.cfgPolicy||shadow.Flags!=original.shadowPolicy)
        return Fail(6008,"input_warning_compat_unsupported_or_changed_actual_mitigation");
    return true;
}
inline bool BuildRuntime(){
    if(state.built)return true;
    if(!VerifySource()||!Mitigations())return false;
    const auto& original=ack::GetState();
    if(!original.installed||!original.built||!original.handlerVerified||!original.unwindVerified)
        return Fail(6002,"input_warning_compat_actual_ack_runtime_not_validated");
    runtime.add=ack::runtime.add;runtime.remove=ack::runtime.remove;
    runtime.lookup=ack::runtime.lookup;runtime.unwind=ack::runtime.unwind;
    if(!runtime.add||!runtime.remove||!runtime.lookup||!runtime.unwind)
        return Fail(6009,"input_warning_compat_actual_unwind_apis_missing");
    const bool cfg=original.cfgEnabled;
    SYSTEM_INFO sys{};GetSystemInfo(&sys);const uintptr_t gran=sys.dwAllocationGranularity;
    const uintptr_t first=(engine::image+generated::ImageSize+gran-1)&~(gran-1),limit=first+0x40000000ULL;
    for(uintptr_t address=first;address<limit;){
        MEMORY_BASIC_INFORMATION m{};if(!VirtualQuery(reinterpret_cast<void*>(address),&m,sizeof(m)))break;
        const uintptr_t end=reinterpret_cast<uintptr_t>(m.BaseAddress)+m.RegionSize;
        if(m.State==MEM_FREE&&end>=address+generated::ArenaSize){
            runtime.arena=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(address),generated::ArenaSize,MEM_COMMIT|MEM_RESERVE,
                cfg?(PAGE_EXECUTE_READ|PAGE_TARGETS_INVALID):PAGE_READWRITE));
            if(runtime.arena)break;
        }
        if(end<=address)break;address=(end+gran-1)&~(gran-1);
    }
    if(!runtime.arena)return Fail(6010,"input_warning_compat_near_original_arena_unavailable");
    state.arena=reinterpret_cast<uintptr_t>(runtime.arena);
    if(cfg){DWORD previous=0;
        if(!VirtualProtect(runtime.arena,generated::ArenaSize,PAGE_READWRITE,&previous)){
            DiscardRuntime();return Fail(6011,"input_warning_compat_nonexecutable_staging_failed");
        }
    }
    for(const auto& body:generated::Bodies)std::memcpy(runtime.arena+body.offset,body.bytes,body.size);
    std::memcpy(runtime.arena+generated::Bodies[generated::ReplacementBody].offset+generated::ReplacementOffset,
        generated::NotificationReplacement,sizeof(generated::NotificationReplacement));
    for(const auto& patch:generated::Patches){
        auto instruction=runtime.arena+generated::Bodies[patch.body].offset+patch.instruction;
        const uintptr_t target=patch.kind==2?state.arena+generated::RelayOffset:patch.kind==1?engine::image+patch.target:MapTarget(patch.target);
        const int64_t difference=static_cast<int64_t>(target)-static_cast<int64_t>(reinterpret_cast<uintptr_t>(instruction)+patch.instructionSize);
        if(!ack::Difference(instruction+patch.field,patch.size,difference)){
            DiscardRuntime();return Fail(6012,"input_warning_compat_relative_operand_out_of_range");
        }
    }
    auto relay=runtime.arena+generated::RelayOffset;relay[0]=0x48;relay[1]=0xb8;
    const auto callback=reinterpret_cast<uintptr_t>(&Notify);std::memcpy(relay+2,&callback,8);relay[10]=0xff;relay[11]=0x25;
    if(!ack::Difference(relay+12,4,static_cast<int64_t>(engine::image+generated::GuardDispatch)-static_cast<int64_t>(reinterpret_cast<uintptr_t>(relay)+16))
       ||!ack::Executable(Read<void*>(reinterpret_cast<void*>(engine::image),generated::GuardDispatch))){
        DiscardRuntime();return Fail(6013,"input_warning_compat_original_cfg_dispatch_unverified");
    }
    runtime.table=reinterpret_cast<PRUNTIME_FUNCTION>(runtime.arena+generated::RuntimeTableOffset);
    const auto arenaRva=static_cast<uint32_t>(state.arena-engine::image);
    for(size_t n=0;n<std::size(generated::Unwinds);++n){
        const auto& u=generated::Unwinds[n];auto bytes=runtime.arena+u.offset;std::memcpy(bytes,u.bytes,u.size);
        const auto bodyOffset=static_cast<uint32_t>(generated::Bodies[u.body].offset);
        runtime.table[n]={arenaRva+bodyOffset+u.begin,arenaRva+bodyOffset+u.end,arenaRva+static_cast<uint32_t>(u.offset)};
        if(u.chain>=0){
            const auto& parent=generated::Unwinds[static_cast<size_t>(u.chain)];
            RUNTIME_FUNCTION chain{arenaRva+static_cast<uint32_t>(generated::Bodies[parent.body].offset)+parent.begin,
                arenaRva+static_cast<uint32_t>(generated::Bodies[parent.body].offset)+parent.end,arenaRva+static_cast<uint32_t>(parent.offset)};
            std::memcpy(bytes+u.size-sizeof(chain),&chain,sizeof(chain));
        }
    }
    const size_t leaf=std::size(generated::Unwinds);runtime.arena[generated::LeafUnwindOffset]=1;
    runtime.table[leaf]={arenaRva+static_cast<DWORD>(generated::RelayOffset),arenaRva+static_cast<DWORD>(generated::RelayOffset+16),
        arenaRva+static_cast<DWORD>(generated::LeafUnwindOffset)};
    DWORD previous=0;
    if(!VirtualProtect(runtime.arena,generated::CodePageSize,PAGE_EXECUTE_READ|(cfg?PAGE_TARGETS_NO_UPDATE:0),&previous)
       ||!VirtualProtect(runtime.arena+generated::CodePageSize,generated::ArenaSize-generated::CodePageSize,PAGE_READONLY,&previous)
       ||!FlushInstructionCache(GetCurrentProcess(),runtime.arena,generated::CodePageSize)){
        DiscardRuntime();return Fail(6014,"input_warning_compat_final_code_and_metadata_protection_failed");
    }
    if(cfg){
        state.cfg.provider=reinterpret_cast<uintptr_t>(GetModuleHandleW(L"kernelbase.dll"));
        SetLastError(ERROR_SUCCESS);auto targets=ack::Api<ack::TargetsFn>(L"kernelbase.dll","SetProcessValidCallTargets");
        state.cfg.api=reinterpret_cast<uintptr_t>(targets);state.cfg.lookupError=targets?ERROR_SUCCESS:GetLastError();
        CFG_CALL_TARGET_INFO entries[3]{{generated::Bodies[0].offset,CFG_CALL_TARGET_VALID},
            {generated::Bodies[3].offset,CFG_CALL_TARGET_VALID},{generated::RelayOffset,CFG_CALL_TARGET_VALID}};
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
            DiscardRuntime();return Fail(6015,"input_warning_compat_private_cfg_entries_not_registered");
        }
        state.cfgRegistered=true;
    }
    if(!runtime.add(runtime.table,static_cast<DWORD>(leaf+1),engine::image)){
        DiscardRuntime();return Fail(6016,"input_warning_compat_runtime_function_table_not_registered");
    }
    runtime.registered=true;
    if(!CheckUnwind()){DiscardRuntime();return Fail(6017,"input_warning_compat_original_and_relocated_unwind_differ");}
    runtime.tick=reinterpret_cast<TickFn>(runtime.arena+generated::Bodies[0].offset);
    runtime.serialize=reinterpret_cast<SerializeFn>(runtime.arena+generated::Bodies[3].offset);
    state.built=true;Cache();return true;
}
inline bool Restore(void* object,void** copy,void** source){
    const auto current=Read<void**>(object);
    if(current==source)return true;
    return current==copy&&InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(object),source,copy)==copy;
}
inline bool Close(){
    if(!Thread())return Fail(6000,"input_warning_compat_retire_wrong_original_thread");
    if(binding.inFlight||notificationFrame)return Fail(6023,"input_warning_compat_retire_inside_native_frame");
    if(binding.input&&ack::Live(binding.input,binding.inputId)){
        if(binding.interfaceCopy&&!Restore(binding.interfaceObject,binding.interfaceCopy,binding.interfaceSource))
            return Fail(6022,"input_warning_compat_retire_higher_interface_observer_first");
        binding.interfacePublished=false;
        if(binding.primaryCopy&&!Restore(binding.input,binding.primaryCopy,binding.primarySource))
            return Fail(6022,"input_warning_compat_retire_higher_primary_observer_first");
        binding.primaryPublished=false;
    }else if(binding.input)++state.retiredWeakUnavailable;
    // Weak-null is not proof that pending-kill destructor virtual calls have
    // finished. Never free the two shared instance-table backing pages here.
    // With inFlight==0, no table points directly into the code arena below.
    binding={};state.installed=false;state.actualAckContext=false;state.actualInput=false;state.retiring=false;
    if(!DiscardRuntime())return false;
    ++state.closes;state.stage="input_warning_compat_closed_binding_retired_shared_backing_retained";Cache();return true;
}
inline bool Retire(){return Close();}
template<size_t Count> inline bool TableMatches(void** source,uintptr_t rva,uintptr_t prefix,const uintptr_t(&targets)[Count]){
    if(source!=reinterpret_cast<void**>(engine::image+rva)||!original_players::Readable(source-1,(Count+1)*8)
       ||Read<uintptr_t>(source-1)!=engine::image+prefix)return false;
    for(size_t n=0;n<Count;++n)if(Read<uintptr_t>(source,n*8)!=engine::image+targets[n])return false;
    return true;
}
inline bool CopyTable(void** source,size_t count,void*** result){
    auto storage=static_cast<void**>(VirtualAlloc(nullptr,(count+1)*8,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!storage)return Fail(6018,"input_warning_compat_private_instance_table_allocation_failed");
    std::memcpy(storage,source-1,(count+1)*8);*result=storage+1;return true;
}
inline bool PrepareSharedTables(Binding& next){
    if(sharedTables.primaryCopy||sharedTables.interfaceCopy){
        if(!sharedTables.primaryCopy||!sharedTables.interfaceCopy||sharedTables.primarySource!=next.primarySource
           ||sharedTables.interfaceSource!=next.interfaceSource
           ||Read<void*>(sharedTables.primaryCopy,TickSlot*8)!=reinterpret_cast<void*>(&Tick)
           ||Read<void*>(sharedTables.interfaceCopy,SerializeSlot*8)!=reinterpret_cast<void*>(&Serialize))
            return Fail(6034,"input_warning_compat_shared_original_table_identity_changed");
        next.primaryCopy=sharedTables.primaryCopy;next.interfaceCopy=sharedTables.interfaceCopy;return true;
    }
    if(!CopyTable(next.primarySource,PrimaryEntries,&next.primaryCopy))return false;
    if(!CopyTable(next.interfaceSource,InterfaceEntries,&next.interfaceCopy)){
        VirtualFree(next.primaryCopy-1,0,MEM_RELEASE);next.primaryCopy=nullptr;return false;
    }
    next.primaryCopy[TickSlot]=reinterpret_cast<void*>(&Tick);
    next.interfaceCopy[SerializeSlot]=reinterpret_cast<void*>(&Serialize);
    DWORD previous=0;
    if(!VirtualProtect(next.primaryCopy-1,(PrimaryEntries+1)*8,PAGE_READONLY,&previous)
       ||!VirtualProtect(next.interfaceCopy-1,(InterfaceEntries+1)*8,PAGE_READONLY,&previous)){
        // Neither table has ever been published at this point.
        VirtualFree(next.primaryCopy-1,0,MEM_RELEASE);VirtualFree(next.interfaceCopy-1,0,MEM_RELEASE);
        next.primaryCopy=next.interfaceCopy=nullptr;
        return Fail(6019,"input_warning_compat_private_instance_table_protection_failed");
    }
    sharedTables={next.primarySource,next.primaryCopy,next.interfaceSource,next.interfaceCopy};
    state.retainedSharedTables=2;return true;
}
inline bool InstallImpl(void* world,void* pc,void* pawn){
    if(!Thread())return Fail(6000,"input_warning_compat_install_wrong_original_thread");
    if(state.error)return false;
    if(binding.active){
        if(binding.world!=world||binding.pc!=pc||binding.pawn!=pawn||!Owned(binding))
            return Fail(6025,"input_warning_compat_retire_old_input_epoch_before_rebind");
        if(Read<void**>(binding.input)!=binding.primaryCopy||Read<void**>(binding.interfaceObject)!=binding.interfaceCopy)
            return Fail(6030,"input_warning_compat_original_observer_tables_changed_after_install");
        state.lastNativeProtection=Read<uint8_t>(binding.input,0x12c);CacheIfDue();return true;
    }
    if(!world||Read<uint8_t>(pc,0x224)!=2||Read<uint8_t>(pawn,0x224)!=2){
        state.stage="input_warning_compat_waiting_actual_own_client";Cache();return true;
    }
    if(!ack::IsOwnedAckEntry(pawn,reinterpret_cast<void*>(&ack::NormalAck))
       ||ack::binding.world!=world||ack::binding.pc!=pc)
        return Fail(6002,"input_warning_compat_requires_actual_ack_tracker_owner_evidence");
    if(!VerifySource()||!VerifyMetadata())return false;
    Binding next;next.world=world;next.pc=pc;next.pawn=pawn;next.input=Read<void*>(pawn,0xba0);
    if(!next.input||!original_players::IsA(next.input,0x6494370)
       ||Read<void*>(next.input,0xc8)!=pawn||Read<void*>(next.input,0x148)!=pawn)
        return Fail(6005,"input_warning_compat_actual_inputstate_owner_mismatch");
    next.interfaceObject=static_cast<uint8_t*>(next.input)+0xd8;
    if(!ack::Capture(next.input,next.inputId))return Fail(6006,"input_warning_compat_actual_inputstate_weak_identity_unavailable");
    next.primarySource=Read<void**>(next.input);next.interfaceSource=Read<void**>(next.interfaceObject);
    if(!TableMatches(next.primarySource,0xb093c10,0x6cd1700,generated::PrimaryTargets)
       ||!TableMatches(next.interfaceSource,0xb0987c0,0x4bd2640,generated::InterfaceTargets))
        return Fail(6007,"input_warning_compat_requires_original_inputstate_tables_before_other_layers");
    auto getWorld=Read<void*>(next.primarySource,0x1b8);
    if(getWorld!=reinterpret_cast<void*>(engine::image+0x2a64ee0)
       ||reinterpret_cast<void*(*)(void*)>(getWorld)(next.input)!=world||!Owned(next))
        return Fail(6005,"input_warning_compat_actual_inputstate_world_or_owner_changed");
    if(!BuildRuntime())return false;
    if(!PrepareSharedTables(next))return false;
    binding=next;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(binding.input),binding.primaryCopy,binding.primarySource)!=binding.primarySource){
        binding={};
        return Fail(6020,"input_warning_compat_private_primary_publication_conflict");
    }
    binding.primaryPublished=true;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(binding.interfaceObject),binding.interfaceCopy,binding.interfaceSource)!=binding.interfaceSource){
        // Retain both allocations if an unknown concurrent owner prevents the
        // rollback. Cleanup must restore/retire that layer before freeing them.
        if(Restore(binding.input,binding.primaryCopy,binding.primarySource)){
            binding={};
        }
        return Fail(6021,"input_warning_compat_private_interface_publication_conflict");
    }
    binding.interfacePublished=true;binding.active=true;
    state.installed=true;state.actualAckContext=true;state.actualInput=true;state.retiring=false;
    state.input=reinterpret_cast<uintptr_t>(binding.input);state.lastNativeProtection=Read<uint8_t>(binding.input,0x12c);
    ++state.installs;state.stage="input_warning_compat_original_input_algorithms_and_real_tracker_bound";Cache();return true;
}
inline bool Install(void* world,void* pc,void* pawn){
    __try{return InstallImpl(world,pc,pawn);}
    __except(EXCEPTION_EXECUTE_HANDLER){return Fail(6031,"input_warning_compat_validation_exception_requires_stop");}
}
inline void Write(FILE* file){
    const auto s=Snapshot();auto yes=[](bool value){return value?"true":"false";};
    std::fprintf(file,"{\"stage\":\"%s\",\"error\":%u,\"source_profiles\":%s,\"metadata\":%s,\"built\":%s,\"installed\":%s,\"unwind_verified\":%s,\"cfg_registered\":%s,\"actual_ack_context\":%s,\"actual_inputstate\":%s,\"dynamic_policy\":%lu,\"cfg_policy\":%lu,\"shadow_policy\":%lu,\"arena\":\"0x%llx\",\"inputstate\":\"0x%llx\",\"installs\":%llu,\"closes\":%llu,\"native_ticks\":%llu,\"native_tick_returns\":%llu,\"native_serializations\":%llu,\"native_serialize_returns\":%llu,\"warnings\":%llu,\"active_warnings\":%llu,\"inactive_warnings\":%llu,\"original_server_queries\":%llu,\"failed_warnings\":%llu,\"unwind_cases\":%llu,\"last_original_protection_value\":%u,\"retired_weak_unavailable\":%llu,\"retained_shared_tables\":%u,\"original_image_writes\":0,\"adapter_input_writes\":0,\"adapter_protection_writes\":0,\"adapter_queue_writes\":0,\"adapter_threshold_writes\":0",
        s.stage,s.error,yes(s.profiles),yes(s.metadata),yes(s.built),yes(s.installed),yes(s.unwindVerified),yes(s.cfgRegistered),
        yes(s.actualAckContext),yes(s.actualInput),s.dynamicPolicy,s.cfgPolicy,s.shadowPolicy,
        static_cast<unsigned long long>(s.arena),static_cast<unsigned long long>(s.input),s.installs,s.closes,s.ticks,s.tickReturns,
        s.serializations,s.serializeReturns,s.warnings,s.activeWarnings,s.inactiveWarnings,s.serverQueries,s.failedWarnings,s.unwindCases,
        unsigned(s.lastNativeProtection),s.retiredWeakUnavailable,s.retainedSharedTables);
    std::fputs(",\"cfg_registration\":",file);ack::WriteCfg(file,s.cfg);
    std::fprintf(file,",\"native_epoch_retiring\":%s,\"lifecycle_fences\":%llu,\"retiring_original_ticks\":%llu,\"retiring_original_serializations\":%llu",
        yes(s.retiring),s.lifecycleFences,s.retiringOriginalTicks,s.retiringOriginalSerializations);
    const auto& d=s.ownershipFailure;
    std::fprintf(file,",\"ownership_failure\":{\"observed\":%s,\"entry\":\"%s\",\"at_ms\":%llu,\"ordinal\":%llu,\"thread\":%u,\"expected_thread\":%u,\"ack_error\":%u,\"warning_inflight\":%u,\"ack_inflight\":%u,\"pc_role\":%u,\"pawn_role\":%u,\"input_protection\":%u,\"connection_state\":%d,\"local_count\":%d,\"warning_mask\":\"0x%llx\",\"ack_mask\":\"0x%llx\",\"ack_weak_mask\":\"0x%llx\",\"warning_gates\":{",
        yes(d.observed),d.entry,d.at,d.ordinal,d.thread,d.expectedThread,d.ackError,d.warningInFlight,d.ackInFlight,unsigned(d.pcRole),unsigned(d.pawnRole),unsigned(d.protection),d.connectionState,d.localCount,d.warningMask,d.ackMask,d.ackWeakMask);
    for(size_t n=0;n<std::size(WarningGateNames);++n)std::fprintf(file,"%s\"%s\":%s",n?",":"",WarningGateNames[n],yes((d.warningMask&(uint64_t(1)<<n))!=0));
    std::fputs("},\"ack_gates\":{",file);
    for(size_t n=0;n<std::size(AckGateNames);++n)std::fprintf(file,"%s\"%s\":%s",n?",":"",AckGateNames[n],yes((d.ackMask&(uint64_t(1)<<n))!=0));
    std::fputs("},\"pointers\":{",file);
    for(size_t n=0;n<std::size(FailurePointerNames);++n)std::fprintf(file,"%s\"%s\":\"0x%llx\"",n?",":"",FailurePointerNames[n],static_cast<unsigned long long>(d.pointers[n]));
    std::fputs("},\"ack_weak\":[",file);
    for(size_t n=0;n<std::size(AckWeakNames);++n)std::fprintf(file,"%s{\"name\":\"%s\",\"expected_index\":%d,\"expected_serial\":%d,\"resolved\":\"0x%llx\"}",n?",":"",AckWeakNames[n],d.ackWeak[n].index,d.ackWeak[n].serial,static_cast<unsigned long long>(d.ackResolved[n]));
    std::fprintf(file,"],\"input_weak\":{\"expected_index\":%d,\"expected_serial\":%d,\"resolved\":\"0x%llx\"},\"native_world_getter_called\":%s}}",d.inputWeak.index,d.inputWeak.serial,static_cast<unsigned long long>(d.inputResolved),yes(d.getWorldCalled));
}
} // namespace local_server_input_warning_compat
