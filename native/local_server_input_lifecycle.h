#pragma once
#include "local_server_input_health.h"
#include "local_server_input_warning_compat.h"
#include "local_server_input_lifecycle.generated.h"
#include <array>
#include <cstdio>
#include <cstring>

// Candidate client native owner-transition observer. A controller gets one
// dedicated copy only from its exact original PC table. Unknown existing
// private layers require their real owner API; they are never borrowed by type.
namespace local_server_input_lifecycle {
namespace ack=local_server_ip_ack_compat;
namespace warning=local_server_input_warning_compat;
namespace health=local_server_input_health;
using Weak=original_inventory_slots::WeakIdentity;
using Method0=void(*)(void*);using Method2=void(*)(void*,void*);
enum class Retirement:uint8_t {Completed,Deferred,Failed};
struct Epoch {
    void *world=nullptr,*pc=nullptr,*pawn=nullptr,*input=nullptr;
    Weak worldId{},pcId{},pawnId{},inputId{};uint64_t ackInstallation=0;
};
using Retirer=Retirement(*)(const Epoch&,const char*);
struct Context {
    void *world=nullptr,*pc=nullptr,*driver=nullptr,*connection=nullptr,*player=nullptr,*ps=nullptr;
    std::array<Weak,6> identities{};
};
struct State {
    bool initialized=false,installed=false,closed=false,closing=false,restartRequired=false,profiles=false;
    bool controllerAttached=false,controllerRestored=false,retainedPublishedBacking=false,unknownParent=false,pending=false;
    unsigned error=0;DWORD gameThread=0;uint32_t nativeDepth=0;
    uintptr_t world=0,pc=0,initialPcTable=0,actualPcTable=0,actualRtti=0,expectedPcTable=0,privateTable=0;
    DWORD tableMemoryType=0,tableProtection=0;std::array<uintptr_t,5> actualEntries{};
    uint64_t nativeEntries=0,nativeReturns=0,fences=0,completed=0,deferred=0,failed=0,prerequisiteSkipped=0;
    uint64_t drainCalls=0,attaches=0,detaches=0,sourceEpochAlreadyRetired=0,closedCallbacks=0;
    Epoch epoch{};const char* pendingReason="none";const char* stage="input_lifecycle_not_initialized";
    uint32_t exceptionCode=0;uintptr_t exceptionRva=0;
};
inline Context context{};inline Retirer retirer=nullptr;
inline State state{},cached{};inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline void** sourceTable=nullptr;inline void** copiedTable=nullptr;inline void* allocation=nullptr;
inline constexpr size_t ControllerEntries=623,PrerequisiteSlot=0x7a0/8,PawnSlot=0x7d0/8,UnpossessSlot=0x828/8,RestartSlot=0xab0/8;
inline const State& GetState(){return state;}
inline void Cache(){AcquireSRWLockExclusive(&cacheLock);cached=state;ReleaseSRWLockExclusive(&cacheLock);}
inline State Snapshot(){AcquireSRWLockShared(&cacheLock);auto value=cached;ReleaseSRWLockShared(&cacheLock);return value;}
template<class T>inline T Read(const void* p,size_t offset=0){return ack::Read<T>(p,offset);}
inline bool Live(void* p,const Weak& id){return ack::Live(p,id);}
inline bool Capture(void* p,Weak& id){return ack::Capture(p,id);}
inline bool Thread(){return engine::image&&original_players::gameThread&&GetCurrentThreadId()==original_players::gameThread;}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;Cache();return false;}
inline bool PersistentContext(){
    const void* objects[]{context.world,context.pc,context.driver,context.connection,context.player,context.ps};
    for(size_t n=0;n<std::size(objects);++n)if(!Live(const_cast<void*>(objects[n]),context.identities[n]))return false;
    auto gi=Read<void*>(context.world,0x1d8);auto locals=Read<void**>(gi,0x40);
    return original_players::IsA(context.pc,original_players::ShooterControllerClass)&&Read<uint8_t>(context.pc,0x224)==2
        &&Read<void*>(context.world,0x40)==context.driver&&Read<void*>(context.driver,0x178)==context.world
        &&Read<void*>(context.driver,0x10)==engine::Class(0x5112c90)&&Read<void*>(context.driver,0xc0)==context.connection
        &&Read<void*>(context.connection,0x60)==context.driver&&Read<void*>(context.connection,0xa0)==context.pc
        &&Read<int32_t>(context.connection,0x13c)==3&&Read<void*>(context.pc,0x510)==context.player
        &&Read<void*>(context.player,0x38)==context.pc&&Read<void*>(context.pc,0x470)==context.ps&&Read<void*>(context.ps,0x210)==context.pc
        &&Read<int32_t>(gi,0x48)==1&&locals&&Read<void*>(locals)==context.player
        &&original_players::activeController==context.pc&&original_players::localPlayer==context.player;
}
inline bool Busy(){
    if(ack::binding.inFlight||ack::warningFrame||warning::binding.inFlight||warning::notificationFrame)return true;
    for(const auto& b:health::bindings)if(b.inFlight)return true;return false;
}
inline bool NoActiveObservers(){
    if(ack::binding.active||warning::binding.active)return false;
    for(const auto& b:health::bindings)if(b.active)return false;return true;
}
inline bool SamePendingEpoch(){
    const auto& b=ack::binding;const auto& t=state.epoch;
    return b.active&&b.world==t.world&&b.pc==t.pc&&b.pawn==t.pawn&&ack::state.installs==t.ackInstallation
        &&original_inventory_slots::SameIdentity(b.identities[0],t.worldId)
        &&original_inventory_slots::SameIdentity(b.identities[1],t.pcId)
        &&original_inventory_slots::SameIdentity(b.identities[3],t.pawnId);
}
inline bool CompleteRetirement(){
    if(!state.pending)return true;
    if(NoActiveObservers()){state.pending=false;++state.sourceEpochAlreadyRetired;state.stage="input_lifecycle_prior_epoch_already_retired";Cache();return true;}
    if(!SamePendingEpoch())return Fail(8450,"input_lifecycle_pending_epoch_changed_without_retirement_receipt");
    if(Busy()){++state.deferred;state.stage="input_lifecycle_retirement_deferred_existing_native_frames_keep_arena";Cache();return false;}
    if(!retirer)return Fail(8451,"input_lifecycle_retirement_callback_missing");
    const auto result=retirer(state.epoch,state.pendingReason);
    if(result==Retirement::Deferred){++state.deferred;state.stage="input_lifecycle_root_retirement_deferred";Cache();return false;}
    if(result!=Retirement::Completed){++state.failed;return Fail(8452,"input_lifecycle_root_retirement_failed");}
    if(!NoActiveObservers()){++state.failed;return Fail(8453,"input_lifecycle_root_completed_without_actual_old_observer_retirement");}
    state.pending=false;++state.completed;state.stage="input_lifecycle_old_epoch_actual_retirement_observed";Cache();return true;
}
inline void BeforeNative(void* pc,void* pawnArgument,const char* reason,bool requireOldPawn){
    if(!state.initialized||!state.installed||state.closed||state.closing||state.error||state.restartRequired||pc!=context.pc)return;
    const auto& b=ack::binding;
    if(!b.active||b.pc!=pc)return;
    if(requireOldPawn&&pawnArgument!=b.pawn){++state.prerequisiteSkipped;return;}
    if(state.pending){if(!SamePendingEpoch())Fail(8450,"input_lifecycle_nested_transition_has_different_pending_epoch");return;}
    if(!PersistentContext()||b.world!=context.world||b.pc!=context.pc
       ||!original_inventory_slots::SameIdentity(b.identities[0],context.identities[0])
       ||!original_inventory_slots::SameIdentity(b.identities[1],context.identities[1])){
        Fail(8454,"input_lifecycle_native_transition_persistent_owner_context_unproven");return;
    }
    state.epoch={};state.epoch.world=b.world;state.epoch.pc=b.pc;state.epoch.pawn=b.pawn;
    state.epoch.worldId=b.identities[0];state.epoch.pcId=b.identities[1];state.epoch.pawnId=b.identities[3];state.epoch.ackInstallation=ack::state.installs;
    if(warning::binding.active){
        if(warning::binding.world!=b.world||warning::binding.pc!=b.pc||warning::binding.pawn!=b.pawn){Fail(8455,"input_lifecycle_warning_and_ack_have_different_prior_epochs");return;}
        state.epoch.input=warning::binding.input;state.epoch.inputId=warning::binding.inputId;
    }
    state.pending=true;state.pendingReason=reason;++state.fences;
    // Only private admission flags change. Existing frames retain their arena;
    // late old entries run their original image algorithm, never a new clone.
    if(!warning::FenceNativeEpoch()||!ack::FenceNativeEpoch()){Fail(8456,"input_lifecycle_admission_fence_failed");return;}
    CompleteRetirement();
}
inline LONG ObserveException(EXCEPTION_POINTERS* e){
    state.restartRequired=true;state.exceptionCode=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0;
    const auto address=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=address>=engine::image?address-engine::image:address;state.stage="input_lifecycle_original_exception_propagated";Cache();return EXCEPTION_CONTINUE_SEARCH;
}
inline void Invoke0(void* pc,uintptr_t parentRva,const char* reason){
    const auto parent=engine::function<Method0>(parentRva);
    if(!Thread()){parent(pc);return;}++state.nativeEntries;++state.nativeDepth;
    __try{__try{
        __try{BeforeNative(pc,nullptr,reason,false);}__finally{parent(pc);++state.nativeReturns;}
    }__except(ObserveException(GetExceptionInformation())){}}
    __finally{--state.nativeDepth;Cache();}
}
inline void Invoke2(void* pc,void* pawn,uintptr_t parentRva,const char* reason,bool requireOldPawn){
    const auto parent=engine::function<Method2>(parentRva);
    if(!Thread()){parent(pc,pawn);return;}++state.nativeEntries;++state.nativeDepth;
    __try{__try{
        __try{BeforeNative(pc,pawn,reason,requireOldPawn);}__finally{parent(pc,pawn);++state.nativeReturns;}
    }__except(ObserveException(GetExceptionInformation())){}}
    __finally{--state.nativeDepth;Cache();}
}
inline void OldPawnPrerequisite(void* pc,void* oldPawn){Invoke2(pc,oldPawn,0x42aeb90,"pc_old_pawn_prerequisite",true);}
inline void OnRepPawn(void* pc){Invoke0(pc,0x78b1670,"pc_onrep_pawn");}
inline void OnUnpossess(void* pc){Invoke0(pc,0x4930b40,"pc_on_unpossess");}
inline void ClientRestart(void* pc,void* newPawn){Invoke2(pc,newPawn,0x492e5c0,"pc_client_restart",false);}
inline void PawnController(void* pawn,void*){
    if(!Thread())return;++state.nativeEntries;++state.nativeDepth;
    if(!ack::binding.active||ack::binding.pawn!=pawn)return;
    BeforeNative(ack::binding.pc,pawn,"pawn_onrep_controller",true);
}
inline void PawnControllerReturned(void*,void*,bool returned){
    if(!Thread())return;if(state.nativeDepth)--state.nativeDepth;
    if(returned)++state.nativeReturns;else{state.restartRequired=true;state.stage="input_lifecycle_original_pawn_parent_did_not_return";}
    Cache();
}
inline bool OriginalControllerTable(void** table){
    if(table!=reinterpret_cast<void**>(engine::image+0xb50b540)||!original_players::Readable(table-1,(ControllerEntries+1)*8)
       ||Read<uintptr_t>(table-1)!=engine::image+local_server_input_lifecycle_source::ControllerRtti)return false;
    for(size_t n=0;n<ControllerEntries;++n)if(reinterpret_cast<uintptr_t>(table[n])!=engine::image+local_server_input_lifecycle_source::ControllerTargets[n])return false;return true;
}
inline bool ComposedControllerTable(void** table){
    if(!table||!original_players::Readable(table-1,(ControllerEntries+1)*8)
       ||Read<uintptr_t>(table-1)!=engine::image+local_server_input_lifecycle_source::ControllerRtti)return false;
    for(size_t n=0;n<ControllerEntries;++n){void* expected=reinterpret_cast<void*>(engine::image+local_server_input_lifecycle_source::ControllerTargets[n]);
        if(n==PrerequisiteSlot)expected=reinterpret_cast<void*>(&OldPawnPrerequisite);
        else if(n==PawnSlot)expected=reinterpret_cast<void*>(&OnRepPawn);
        else if(n==UnpossessSlot)expected=reinterpret_cast<void*>(&OnUnpossess);
        else if(n==RestartSlot)expected=reinterpret_cast<void*>(&ClientRestart);
        if(table[n]!=expected)return false;
    }return true;
}
inline void ObserveActualTable(){
    auto table=Read<void**>(context.pc);state.actualPcTable=reinterpret_cast<uintptr_t>(table);state.actualRtti=Read<uintptr_t>(table?table-1:nullptr);
    constexpr size_t slots[]{PrerequisiteSlot,PawnSlot,UnpossessSlot,RestartSlot,0x298/8};
    for(size_t n=0;n<std::size(slots);++n)state.actualEntries[n]=Read<uintptr_t>(table,slots[n]*8);
    MEMORY_BASIC_INFORMATION m{};if(table&&VirtualQuery(table,&m,sizeof(m))){state.tableMemoryType=m.Type;state.tableProtection=m.Protect;}
}
inline bool ReadonlyControllerAllocation(){
    if(!allocation||!copiedTable)return false;
    auto at=reinterpret_cast<uintptr_t>(allocation);const auto end=at+(ControllerEntries+1)*8;
    if(end<at)return false;
    while(at<end){MEMORY_BASIC_INFORMATION m{};
        if(!VirtualQuery(reinterpret_cast<void*>(at),&m,sizeof(m))||m.State!=MEM_COMMIT||m.Type!=MEM_PRIVATE
           ||m.AllocationBase!=allocation||m.Protect!=PAGE_READONLY)return false;
        const auto next=reinterpret_cast<uintptr_t>(m.BaseAddress)+m.RegionSize;if(next<=at)return false;at=next;
    }return true;
}
inline bool CarrierOwned(){
    if(!Thread()||!state.controllerAttached||!Live(context.pc,context.identities[1])||!PersistentContext())return false;
    ObserveActualTable();return Read<void**>(context.pc)==copiedTable&&ComposedControllerTable(copiedTable)&&ReadonlyControllerAllocation();
}
inline bool Init(void* world,void* pc,Retirer callback){
    if(!Thread())return false;
    if(state.closed||state.closing||state.error||state.restartRequired)return false;
    if(state.initialized)return context.world==world&&context.pc==pc&&retirer==callback&&CarrierOwned();
    if(!world||!pc||!callback||ack::binding.active||warning::binding.active||health::current)
        return Fail(8457,"input_lifecycle_init_before_original_input_observers_required");
    Context next{};next.world=world;next.pc=pc;next.driver=Read<void*>(world,0x40);next.connection=Read<void*>(next.driver,0xc0);
    next.player=Read<void*>(pc,0x510);next.ps=Read<void*>(pc,0x470);
    void* objects[]{next.world,next.pc,next.driver,next.connection,next.player,next.ps};
    for(size_t n=0;n<std::size(objects);++n)if(!Capture(objects[n],next.identities[n]))return Fail(8458,"input_lifecycle_persistent_client_identity_missing");
    context=next;state.world=reinterpret_cast<uintptr_t>(world);
    state.gameThread=GetCurrentThreadId();state.pc=reinterpret_cast<uintptr_t>(pc);retirer=callback;
    if(!PersistentContext())return Fail(8458,"input_lifecycle_requires_actual_single_generic_ip_client");
    for(const auto& p:local_server_input_lifecycle_source::Profiles){auto data=reinterpret_cast<const uint8_t*>(engine::image+p.rva);uint64_t hash=14695981039346656037ULL;
        if(!original_players::Readable(data,p.bytes))return Fail(8459,"input_lifecycle_original_parent_source_unreadable");
        for(uint32_t n=0;n<p.bytes;++n)hash=(hash^data[n])*1099511628211ULL;if(hash!=p.hash)return Fail(8459,"input_lifecycle_original_parent_source_profile_mismatch");
    }state.profiles=true;ObserveActualTable();sourceTable=Read<void**>(pc);state.initialPcTable=reinterpret_cast<uintptr_t>(sourceTable);state.expectedPcTable=engine::image+0xb50b540;
    if(!OriginalControllerTable(sourceTable)){state.unknownParent=true;return Fail(8460,"input_lifecycle_existing_controller_layer_requires_actual_owner_api");}
    if(!ack::BindPawnControllerLifecycle(&PawnController,&PawnControllerReturned))return Fail(8461,"input_lifecycle_pawn_callback_requires_binding_before_ack_publication");
    auto storage=static_cast<void**>(VirtualAlloc(nullptr,(ControllerEntries+1)*8,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!storage)return Fail(8462,"input_lifecycle_private_controller_table_allocation_failed");
    std::memcpy(storage,sourceTable-1,(ControllerEntries+1)*8);allocation=storage;copiedTable=storage+1;
    copiedTable[PrerequisiteSlot]=reinterpret_cast<void*>(&OldPawnPrerequisite);copiedTable[PawnSlot]=reinterpret_cast<void*>(&OnRepPawn);
    copiedTable[UnpossessSlot]=reinterpret_cast<void*>(&OnUnpossess);copiedTable[RestartSlot]=reinterpret_cast<void*>(&ClientRestart);
    DWORD oldProtection=0;
    if(!ComposedControllerTable(copiedTable)||!VirtualProtect(storage,(ControllerEntries+1)*8,PAGE_READONLY,&oldProtection)){
        VirtualFree(storage,0,MEM_RELEASE);allocation=nullptr;copiedTable=nullptr;return Fail(8463,"input_lifecycle_complete_private_table_or_protection_readback_failed");}
    state.initialized=true;state.privateTable=reinterpret_cast<uintptr_t>(copiedTable);
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(pc),copiedTable,sourceTable)!=sourceTable){
        VirtualFree(storage,0,MEM_RELEASE);allocation=nullptr;copiedTable=nullptr;return Fail(8464,"input_lifecycle_original_controller_publication_conflict");}
    state.retainedPublishedBacking=true;state.controllerAttached=state.installed=true;++state.attaches;
    if(!CarrierOwned())return Fail(8466,"input_lifecycle_published_private_controller_carrier_readback_failed");
    state.stage="input_lifecycle_exact_native_controller_table_owned_four_callbacks_installed";Cache();return true;
}
inline bool Drain(){
    if(!Thread())return false;if(!state.initialized)return !state.error;++state.drainCalls;
    if(state.closed||state.error||state.restartRequired)return false;
    __try{return CompleteRetirement();}__except(ObserveException(GetExceptionInformation())){return false;}
}
inline bool CanBind(){
    if(!Thread()||!state.initialized||!state.controllerAttached||state.nativeDepth||state.closed||state.closing||state.pending||state.error||state.restartRequired
       ||ack::state.error||warning::state.error||health::GetState().error)return false;
    if(!CarrierOwned()){state.unknownParent=true;ObserveActualTable();return Fail(8466,"input_lifecycle_controller_carrier_or_persistent_context_changed");}
    return true;
}
inline bool Close(){
    if(!Thread())return false;
    if(!state.initialized){state.closed=true;Cache();return true;}
    if(state.nativeDepth||Busy()){state.closing=true;state.stage="input_lifecycle_close_deferred_native_frames_backing_retained";Cache();return false;}
    state.closing=true;
    if(!NoActiveObservers()){state.stage="input_lifecycle_close_requires_prior_observer_retirement";Cache();return false;}
    if(Live(context.pc,context.identities[1])){
        auto actual=Read<void**>(context.pc);
        if(actual==copiedTable){
            if(!ComposedControllerTable(copiedTable)||InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(context.pc),sourceTable,copiedTable)!=copiedTable)
                return Fail(8465,"input_lifecycle_owned_controller_restore_conflict");
            state.controllerRestored=true;state.controllerAttached=false;++state.detaches;
        }else if(actual==sourceTable){state.controllerRestored=true;state.controllerAttached=false;}
        else {state.stage="input_lifecycle_foreign_controller_layer_retained_until_process_exit";Cache();return false;}
    }else state.stage="input_lifecycle_pendingkill_controller_backing_retained_until_process_exit";
    if(Live(context.pc,context.identities[1]))ObserveActualTable();
    // No free: pending-kill callbacks may still use any published table.
    state.installed=false;state.closed=true;state.stage="input_lifecycle_closed_published_backing_retained_original_parents_once";Cache();return true;
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path||!*path)return false;const auto s=Snapshot();FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;
    auto b=[](bool value){return value?"true":"false";};
    std::fprintf(file,"{\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"installed\":%s,\"closed\":%s,\"closing\":%s,\"restart_required\":%s,\"profiles\":%s,\"controller_attached\":%s,\"controller_restored\":%s,\"retained_published_backing\":%s,\"unknown_parent\":%s,\"pending_retirement\":%s,\"pc\":\"0x%llx\",\"actual_pc_table\":\"0x%llx\",\"actual_rtti\":\"0x%llx\",\"expected_pc_table\":\"0x%llx\",\"private_table\":\"0x%llx\",\"table_memory_type\":%lu,\"table_protection\":%lu,\"native_depth\":%u,\"native_entries\":%llu,\"native_returns\":%llu,\"fences\":%llu,\"completed\":%llu,\"deferred\":%llu,\"failed\":%llu,\"prerequisite_skipped\":%llu,\"drain_calls\":%llu,\"pending_reason\":\"%s\",\"pending_epoch\":{\"world\":\"%p\",\"pc\":\"%p\",\"pawn\":\"%p\",\"input\":\"%p\",\"ack_installation\":%llu},\"original_image_writes\":0,\"input_or_queue_writes\":0,\"native_cleanup_replays\":0,\"runtime_verified\":false,",
        s.stage,s.error,b(s.initialized),b(s.installed),b(s.closed),b(s.closing),b(s.restartRequired),b(s.profiles),b(s.controllerAttached),b(s.controllerRestored),b(s.retainedPublishedBacking),b(s.unknownParent),b(s.pending),
        static_cast<unsigned long long>(s.pc),static_cast<unsigned long long>(s.actualPcTable),static_cast<unsigned long long>(s.actualRtti),static_cast<unsigned long long>(s.expectedPcTable),static_cast<unsigned long long>(s.privateTable),
        s.tableMemoryType,s.tableProtection,s.nativeDepth,s.nativeEntries,s.nativeReturns,s.fences,s.completed,s.deferred,s.failed,s.prerequisiteSkipped,s.drainCalls,s.pendingReason,
        s.epoch.world,s.epoch.pc,s.epoch.pawn,s.epoch.input,s.epoch.ackInstallation);
    std::fprintf(file,"\"initial_pc_table\":\"0x%llx\",\"actual_entries\":[",static_cast<unsigned long long>(s.initialPcTable));
    for(size_t n=0;n<s.actualEntries.size();++n)std::fprintf(file,"%s\"0x%llx\"",n?",":"",static_cast<unsigned long long>(s.actualEntries[n]));
    std::fputs("]}\n",file);
    const bool ok=std::ferror(file)==0;return std::fclose(file)==0&&ok;
}
} // namespace local_server_input_lifecycle
