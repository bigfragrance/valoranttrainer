#pragma once
#include <Windows.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <type_traits>

// Scenario12 owns this cache. Neither Store nor Copy forwards an engine call.
// All pointer values below are diagnostic addresses, never dereferenced here.
namespace local_server_native_shop_snapshot {
inline constexpr size_t MaximumSwitchReceipts=36;
enum class Exit:uint8_t {NotObserved,Tick,Stop,Exception};
struct Weak {int32_t index=-1,serial=0;};
struct Bound {void* object=nullptr;Weak identity{};};
struct Field {
    bool observed=false;
    uint32_t mismatch=0;int32_t offset=-1,size=0,dim=0;
    uint64_t flags=0,castFlags=0;void *field=nullptr,*expectedEnum=nullptr,*actualEnum=nullptr;
};
struct SwitchReceipt {uint8_t slot=255;uint64_t tick=0,atMs=0;int32_t before=-1,after=-1;bool queued=false;};
struct Fixture {
    uint32_t scenario=12,error=0,phase=0,expectedThread=0,actualThread=0,profileFailureRva=0;
    uint64_t nonce=0,ticks=0,ownershipPassedMask=0,readinessPassedMask=0;
    bool initialized=false,configured=false,cliMatched=false,armed=false,profilesVerified=false,metadataVerified=false;
    bool bound=false,finished=false,failed=false,actionsCompleted=false,stopRequested=false;
    bool shopOpenKnown=false,shopOpen=false,shopOpenSubmitted=false,shopCloseSubmitted=false;
    double elapsedSeconds=0;
    char stage[128]="native_shop_prelude_not_observed_before_game_thread_publication",operation[112]="none";
};
struct Evidence {
    uint32_t phase=0,cycles=0,switchReceiptCount=0;
    bool methodsReady=false,modelReady=false,ownerBound=false,notObserved=true,releaseUnavailable=false;
    bool transition=false,stateKnown=false,oldPrelude=false,pendingHeaderRead=false,pendingRecordsRead=false;
    bool purchaseReceiptCaptured=false,purchaseReturned=false,purchaseSubmitted=false,purchaseReturnObserved=false;
    bool purchaseTransitionProven=false,beforeInputRead=false,afterInputRead=false;
    uint64_t samples=0,switchCalls=0,switchQueueProofs=0,pendingProofs=0,purchaseCalls=0,purchaseAtMs=0,purchaseTick=0;
    uint64_t openedForPurchase=0,preambleLostOnOpen=0;
    int32_t pendingCount=-1,pendingCapacity=-1,oldIndex=-1,beforeInputIndex=-1,afterInputIndex=-1;
    int32_t purchasePendingCount=-1,purchasePendingCapacity=-1,purchaseOldIndex=-1;
    uint8_t currentSlot=255,preambleSlot=255,purchasePreambleSlot=255;
    void *pendingPointer=nullptr,*purchasePendingPointer=nullptr,*purchaseMachine=nullptr,*purchaseSelectedState=nullptr;
    Weak purchaseMachineIdentity{},purchaseStateIdentity{};
    Bound model{},purchasable{},targetClass{};
    std::array<Field,4> fields{};
    std::array<std::array<uint8_t,40>,3> pendingRecords{},purchasePendingRecords{};
    std::array<SwitchReceipt,MaximumSwitchReceipts> switchReceipts{};
    char targetLabel[24]{},result[160]="not_observed_before_game_thread_publication";
};
struct Snapshot {
    bool enabled=false,published=false;
    Exit exit=Exit::NotObserved;
    uint64_t publication=0;
    uint32_t publisherThread=0;
    Fixture fixture{};
    Evidence prelude{};
};
static_assert(std::is_trivially_copyable_v<Snapshot>&&std::is_standard_layout_v<Snapshot>);
struct Cache {
    SRWLOCK lock=SRWLOCK_INIT;
    Snapshot value{};
    void Store(const Snapshot& next){AcquireSRWLockExclusive(&lock);value=next;ReleaseSRWLockExclusive(&lock);}
    Snapshot Copy(){Snapshot copy{};AcquireSRWLockShared(&lock);copy=value;ReleaseSRWLockShared(&lock);return copy;}
};
inline Snapshot Configured(uint64_t nonce){
    Snapshot s{};s.enabled=true;s.fixture.initialized=s.fixture.configured=true;s.fixture.nonce=nonce;return s;
}
inline const char* ExitName(Exit exit){
    switch(exit){case Exit::Tick:return "tick";case Exit::Stop:return "stop";case Exit::Exception:return "exception";default:return "not_observed";}
}
inline void RawRecords(FILE* file,const std::array<std::array<uint8_t,40>,3>& records,bool observed){
    if(!observed){std::fputs("null",file);return;}
    std::fputc('[',file);
    for(size_t n=0;n<records.size();++n){if(n)std::fputc(',',file);std::fputc('"',file);
        for(const auto byte:records[n])std::fprintf(file,"%02x",unsigned(byte));std::fputc('"',file);}
    std::fputc(']',file);
}
// Accept only an owned copy. No access to live NativeTest/ShopPrelude globals.
inline bool Write(FILE* file,const Snapshot& s){
    if(!file||!s.enabled)return false;
    const auto& f=s.fixture;const auto& p=s.prelude;const auto b=[](bool x){return x?"true":"false";};
    std::fprintf(file,"{\"schema_version\":1,\"scope\":\"explicit_original_native_api_fixture\",\"stage\":\"%s\",\"operation\":\"%s\",\"error\":%u,\"scenario\":%u,\"nonce\":%llu,\"initialized\":%s,\"configured\":%s,\"cli_matched\":%s,\"armed\":%s,\"profiles_verified\":%s,\"metadata_verified\":%s,\"bound\":%s,\"finished\":%s,\"failed\":%s,\"actions_completed\":%s,\"phase\":%u,\"ticks\":%llu,\"elapsed_seconds\":%.9g,\"stop_requested\":%s,\"ownership_passed_mask\":%llu,\"ownership_required_mask\":8191,\"readiness_passed_mask\":%llu,\"expected_thread\":%u,\"actual_thread\":%u,\"profile_failure_rva\":%u,\"shop_open_known\":%s,\"shop_open\":%s,\"shop_open_submitted\":%s,\"shop_close_submitted\":%s,",
        f.stage,f.operation,f.error,f.scenario,static_cast<unsigned long long>(f.nonce),b(f.initialized),b(f.configured),b(f.cliMatched),b(f.armed),b(f.profilesVerified),b(f.metadataVerified),b(f.bound),b(f.finished),b(f.failed),b(f.actionsCompleted&&s.published&&s.exit!=Exit::Exception),f.phase,static_cast<unsigned long long>(f.ticks),f.elapsedSeconds,b(f.stopRequested),static_cast<unsigned long long>(f.ownershipPassedMask),static_cast<unsigned long long>(f.readinessPassedMask),f.expectedThread,f.actualThread,f.profileFailureRva,b(f.shopOpenKnown),b(f.shopOpen),b(f.shopOpenSubmitted),b(f.shopCloseSubmitted));
    std::fprintf(file,"\"shop_prelude\":{\"enabled\":true,\"scope\":\"explicit_original_slot_inputs_and_original_model_purchase_only\",\"snapshot_published\":%s,\"snapshot_publication\":%llu,\"snapshot_exit\":\"%s\",\"snapshot_publisher_thread\":%u,\"phase\":%u,\"result\":\"%s\",\"not_observed\":%s,\"samples\":%llu,\"cycles\":%u,\"maximum_cycles\":12,\"switch_calls\":%llu,\"switch_native_two_edge_queue_proofs\":%llu,\"pending_proofs\":%llu,\"native_transition\":%s,\"native_state_known\":%s,\"old_prelude\":%s,\"current_slot\":%u,\"preamble_owned_slot\":%u,\"pending_header_observed\":%s,\"pending_records_observed\":%s,\"pending_count\":%d,\"pending_capacity\":%d,\"pending_pointer\":\"%p\",\"old_client_correction_index\":%d,\"purchase_calls\":%llu,\"purchase_return_observed\":%s,\"original_purchase_returned_true\":%s,\"purchase_submitted\":%s,\"purchase_at_ms\":%llu,\"purchase_fixture_tick\":%llu,\"target\":\"%s\",\"before_input260_observed\":%s,\"before_input260\":%d,\"after_input260_observed\":%s,\"after_input260\":%d,\"preamble_lost_on_menu_open\":%llu,\"release_unavailable\":%s,\"pending_raw_hex\":",
        b(s.published),static_cast<unsigned long long>(s.publication),ExitName(s.exit),s.publisherThread,p.phase,p.result,b(!s.published||p.notObserved||s.exit==Exit::Exception),static_cast<unsigned long long>(p.samples),p.cycles,static_cast<unsigned long long>(p.switchCalls),static_cast<unsigned long long>(p.switchQueueProofs),static_cast<unsigned long long>(p.pendingProofs),b(p.transition),b(p.stateKnown),b(p.oldPrelude),unsigned(p.currentSlot),unsigned(p.preambleSlot),b(p.pendingHeaderRead),b(p.pendingRecordsRead),p.pendingCount,p.pendingCapacity,p.pendingPointer,p.oldIndex,static_cast<unsigned long long>(p.purchaseCalls),b(p.purchaseReturnObserved),b(p.purchaseReturned),b(p.purchaseSubmitted),static_cast<unsigned long long>(p.purchaseAtMs),static_cast<unsigned long long>(p.purchaseTick),p.targetLabel,b(p.beforeInputRead),p.beforeInputIndex,b(p.afterInputRead),p.afterInputIndex,static_cast<unsigned long long>(p.preambleLostOnOpen),b(p.releaseUnavailable));
    RawRecords(file,p.pendingRecords,p.pendingRecordsRead);
    std::fprintf(file,",\"at_purchase_receipt_captured\":%s,\"at_purchase_transition_proven\":%s,\"at_purchase_pending_count\":%d,\"at_purchase_pending_capacity\":%d,\"at_purchase_pending_pointer\":\"%p\",\"at_purchase_old_client_index\":%d,\"at_purchase_preamble_owned_slot\":%u,\"at_purchase_machine\":\"%p\",\"at_purchase_state\":\"%p\",\"at_purchase_machine_weak\":[%d,%d],\"at_purchase_state_weak\":[%d,%d],\"at_purchase_pending_raw_hex\":",
        b(p.purchaseReceiptCaptured),b(p.purchaseTransitionProven),p.purchasePendingCount,p.purchasePendingCapacity,p.purchasePendingPointer,p.purchaseOldIndex,unsigned(p.purchasePreambleSlot),p.purchaseMachine,p.purchaseSelectedState,p.purchaseMachineIdentity.index,p.purchaseMachineIdentity.serial,p.purchaseStateIdentity.index,p.purchaseStateIdentity.serial);
    RawRecords(file,p.purchasePendingRecords,p.purchaseReceiptCaptured);
    std::fputs(",\"original_switch_receipts\":[",file);
    for(uint32_t n=0;n<p.switchReceiptCount&&n<p.switchReceipts.size();++n){const auto& r=p.switchReceipts[n];
        std::fprintf(file,"%s{\"slot\":%u,\"fixture_tick\":%llu,\"at_ms\":%llu,\"native_queue_before\":%d,\"native_queue_after\":%d,\"original_two_edges_proven\":%s}",n?",":"",unsigned(r.slot),static_cast<unsigned long long>(r.tick),static_cast<unsigned long long>(r.atMs),r.before,r.after,b(r.queued));}
    std::fputs("],\"parameter_mismatch_masks\":[",file);
    for(size_t n=0;n<p.fields.size();++n){if(n)std::fputc(',',file);if(p.fields[n].observed)std::fprintf(file,"%u",p.fields[n].mismatch);else std::fputs("null",file);}
    std::fprintf(file,"],\"methods_verified\":%s,\"owner_bound\":%s,\"model_ready\":%s,\"model\":\"%p\",\"model_weak\":[%d,%d],\"purchasable\":\"%p\",\"purchasable_weak\":[%d,%d],\"target_class\":\"%p\",\"target_class_weak\":[%d,%d],\"parameter_fields\":[",
        b(p.methodsReady),b(p.ownerBound),b(p.modelReady),p.model.object,p.model.identity.index,p.model.identity.serial,p.purchasable.object,p.purchasable.identity.index,p.purchasable.identity.serial,p.targetClass.object,p.targetClass.identity.index,p.targetClass.identity.serial);
    for(size_t n=0;n<p.fields.size();++n){const auto& d=p.fields[n];const uint64_t expected=(n==3?0x10000000000580ULL:0x10000000000080ULL)|0x0008001040000200ULL;
        std::fprintf(file,"%s{\"index\":%zu,\"observed\":%s,\"offset\":%d,\"size\":%d,\"dimensions\":%d,\"actual_flags\":\"%016llx\",\"expected_flags\":\"%016llx\",\"cast_flags\":\"%016llx\",\"field\":\"%p\",\"expected_enum\":\"%p\",\"actual_enum\":\"%p\",\"mismatch\":",n?",":"",n,b(d.observed),d.offset,d.size,d.dim,static_cast<unsigned long long>(d.flags),static_cast<unsigned long long>(expected),static_cast<unsigned long long>(d.castFlags),d.field,d.expectedEnum,d.actualEnum);
        if(d.observed)std::fprintf(file,"%u",d.mismatch);else std::fputs("null",file);std::fputc('}',file);}
    std::fputs("],\"pending_writes\":0,\"correction_or_ACK_writes\":0,\"extra_physics_ticks\":0,\"manual_server_purchase_RPC\":false,\"server_receipt_verified\":false,\"bug_reproduction_verified\":false},\"passed\":false,\"physical_keyboard_mouse_verified\":false,\"server_effects_verified\":false,\"extra_physics_ticks\":0,\"manual_fire_calls\":0,\"client_transform_role_health_resource_writes\":0}",file);return true;
}
} // namespace local_server_native_shop_snapshot
