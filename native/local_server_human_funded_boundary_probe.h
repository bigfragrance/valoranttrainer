#pragma once
#include "local_server_empty_bootstrap_probe.h"
#include "human_funded_boundary_core.h"
#include <array>
#include <atomic>
#include <type_traits>

// First-life diagnostic subordinate to the explicit copied-exe empty probe.
// It has no prediction/clock factory, native clock getter, tick/fund method,
// receiver, ACK, offset/RPC setter or formal-publication release operation.
namespace local_server_human_funded_boundary_probe {
namespace empty=local_server_empty_bootstrap_probe;
namespace core=human_funded_boundary_core;
using Weak=original_inventory_slots::WeakIdentity;
using Phase=core::Phase;
struct PublicSource {
    bool verified=false,gameStartSubmitted=false,nativeStartScheduleSubmitted=false;uint32_t sourceEpoch=0,c=0,step=0;int32_t offset=0;
    void *world=nullptr,*driver=nullptr,*mode=nullptr,*gameState=nullptr,*timing=nullptr,*fixed=nullptr,*clock=nullptr;
    Weak worldId{},driverId{},modeId{},gameStateId{},timingId{},fixedId{};
    float accumulated=0;const char* reason="original_mode_source_reader_not_bound";
};
using SourceReader=bool(*)(void* expectedWorld,void* expectedDriver,PublicSource& out);
inline std::atomic<SourceReader> sourceReader{nullptr};
inline bool SetSourceReader(SourceReader reader){
    if(!empty::armed||empty::snapshot.role!=1)return false;
    const auto previous=sourceReader.load(std::memory_order_acquire);
    if(!reader||(previous&&previous!=reader))return false;sourceReader.store(reader,std::memory_order_release);return true;
}
struct ForceInventory {
    bool forceTyped=false,forceOwners=false,forceArraysValid=false,forceArraysEmpty=false;
    void* force=nullptr;int32_t forceCount=-1,forceCapacity=-1,predictedCount=-1,predictedCapacity=-1;
    empty::EmptyInventory inventory{};
};
struct Record {
    bool attempted=false,sameThread=false,owned=false,samePoint=false,actualFunding=false,currentPredictionOwnsClock=false,clockShapeVerified=false,distinctClockInstances=false;
    bool inReceive=false,inPoll=false,wireAligned=false,mappingConstructed=false,timestepRpcSubmitted=false;
    bool firstDecodedMoveSupplied=false,packetAcceptedAtPoint=false,formalBoundTupleStillPending=false;
    uint32_t gameThread=0,actualThread=0,rawBefore=0,rawAfter=0,step=0,firstClientIndex=0;
    uint64_t epoch=0,inputPollSerial=0,receivedPackets=0,acceptedPackets=0,fundingCalls=0,nativeSeedCalls=0,physicsCompleted=0,clockAdvances=0;
    void *world=nullptr,*driver=nullptr,*pc=nullptr,*ps=nullptr,*pawn=nullptr,*connection=nullptr,*movement=nullptr,*predictionBefore=nullptr,*predictionAfter=nullptr,*clock=nullptr,*timeShift=nullptr;
    Weak pawnId{},movementId{},inventoryId{};
    int32_t actualOffsetBefore=0,actualOffsetAfter=0,humanClockVbase=0;uint32_t cachedClockVbase=0;
    void* humanClockTable=nullptr;void* humanClockVbtable=nullptr;
    bool humanClockLayoutRead=false,humanClockFieldsRead=false,primaryClockTableVerified=false,humanVbtableVerified=false,cachedVbaseMatches=false,humanLayoutStableAfterRead=false;
    float aBefore=0,aAfter=0,cachedHumanAccumulator=0;bool cacheMatchesActualAccumulator=false;
    double initialAccumulator=0,creditedSeconds=0,unfundedSeconds=0,preOriginSeconds=0,excludedSeconds=0,fundingRoundoff=0,externalAccumulator=0;
    double nativeConservationError=0,timeConservationError=0;
    double originCredit=0,postOriginSeconds=0,nativeAdvanceAdjustment=0,nativePostAdvanceAdjustment=0,previousPollAccumulator=0;
    PublicSource sourceBefore{},sourceAfter{};ForceInventory publication{};core::Arithmetic arithmetic{};
    const char* reason="not_observed";
};
static_assert(std::is_trivially_copyable_v<Record>);
inline SRWLOCK recordLock=SRWLOCK_INIT;
inline std::array<Record,static_cast<size_t>(Phase::Count)> records{};
inline bool Scope(){
    return empty::armed&&empty::snapshot.role==1&&empty::snapshot.hostBound&&!empty::snapshot.invalidated
        &&empty::marker.expectedEpoch==1&&empty::snapshot.epoch==1;
}
template<class T> inline T Read(const void* p,size_t o=0){return original_players::Value<T>(p,o);}
inline bool Live(void* object,const Weak& weak){return object&&original_inventory_slots::ResolveIdentity(weak)==object;}
inline bool SameSource(const PublicSource& a,const PublicSource& b){
    return a.verified&&b.verified&&a.sourceEpoch==b.sourceEpoch&&a.world==b.world&&a.driver==b.driver&&a.mode==b.mode
        &&a.gameState==b.gameState&&a.timing==b.timing&&a.fixed==b.fixed&&a.clock==b.clock&&a.c==b.c&&a.step==b.step&&a.offset==b.offset
        &&a.gameStartSubmitted==b.gameStartSubmitted&&a.nativeStartScheduleSubmitted==b.nativeStartScheduleSubmitted
        &&core::Bits(a.accumulated)==core::Bits(b.accumulated)&&original_inventory_slots::SameIdentity(a.worldId,b.worldId)&&original_inventory_slots::SameIdentity(a.driverId,b.driverId)
        &&original_inventory_slots::SameIdentity(a.modeId,b.modeId)&&original_inventory_slots::SameIdentity(a.gameStateId,b.gameStateId)
        &&original_inventory_slots::SameIdentity(a.timingId,b.timingId)&&original_inventory_slots::SameIdentity(a.fixedId,b.fixedId);
}
inline ForceInventory ReadPublication(void* pawn,void* inventory,const Weak& pawnId,const Weak& inventoryId){
    ForceInventory out{};out.inventory=empty::ReadEmpty(pawn,inventory,pawnId,inventoryId,true);
    out.force=Read<void*>(pawn,0x1100);out.forceTyped=original_players::IsA(out.force,0x685f510);
    out.forceOwners=Read<void*>(out.force,0xc8)==pawn&&Read<void*>(out.force,0x28)==pawn;
    struct Array{void* data;int32_t count,capacity;};
    if(out.forceTyped&&out.forceOwners){const auto a=Read<Array>(out.force,0x150),b=Read<Array>(out.force,0x160);
        out.forceCount=a.count;out.forceCapacity=a.capacity;out.predictedCount=b.count;out.predictedCapacity=b.capacity;
        out.forceArraysValid=a.count>=0&&a.capacity>=a.count&&a.capacity<=4096&&(!a.count||a.data)
            &&b.count>=0&&b.capacity>=b.count&&b.capacity<=4096&&(!b.count||b.data);
        out.forceArraysEmpty=out.forceArraysValid&&a.count==0&&b.count==0;}
    return out;
}
template<class Input> inline void Capture(Phase phase,const Input& input,uint32_t firstClientIndex,bool decodedMoveSupplied){
    const auto slot=static_cast<size_t>(phase);if(slot>=records.size()||records[slot].attempted||!Scope())return;
    if(input.epochNumber!=1||input.pawn!=empty::snapshot.pawn||input.pc!=empty::snapshot.pc||input.playerState!=empty::snapshot.ps
       ||input.world!=empty::snapshot.world||input.connection!=empty::snapshot.connection)return;
    Record r{};r.attempted=true;r.actualThread=GetCurrentThreadId();r.gameThread=input.gameThread;
    r.sameThread=r.actualThread==input.gameThread&&r.actualThread==original_players::gameThread;
    r.epoch=input.epochNumber;r.inputPollSerial=input.polls;r.inReceive=input.inReceive;r.inPoll=input.inPoll;
    r.wireAligned=input.wireAligned;r.mappingConstructed=input.mappingClockConstructed;r.timestepRpcSubmitted=input.timestepRpcSubmitted;
    r.firstClientIndex=firstClientIndex;r.firstDecodedMoveSupplied=decodedMoveSupplied;
    r.receivedPackets=input.receivedPackets;r.acceptedPackets=input.acceptedPackets;r.packetAcceptedAtPoint=input.acceptedPackets>0;
    r.fundingCalls=input.hostFundingCalls;r.nativeSeedCalls=input.nativeSeedCalls;r.physicsCompleted=input.physicsCompleted;r.clockAdvances=input.clockAdvances;
    r.world=input.world;r.driver=input.nativeNetDriver;r.pc=input.pc;r.ps=input.playerState;r.pawn=input.pawn;r.connection=input.connection;r.movement=input.movement;r.clock=input.clock;r.timeShift=input.timeShift;
    r.pawnId={input.pawnIdentity.index,input.pawnIdentity.serial};r.movementId={input.movementIdentity.index,input.movementIdentity.serial};r.inventoryId=empty::snapshot.inventoryId;
    r.formalBoundTupleStillPending=input.pawn==empty::snapshot.pawn&&input.epochNumber==empty::marker.expectedEpoch;
    // No extra game-thread or source reads until the exact original tuple is
    // accepted. Resolve only already-captured Weak values, never allocate one.
    if(r.sameThread&&input.installed&&!input.error&&input.ownedLinks&&input.weakIdentitiesVerified
       &&input.epochNumber==1&&input.pawn==empty::snapshot.pawn&&input.pc==empty::snapshot.pc&&input.playerState==empty::snapshot.ps
       &&input.world==empty::snapshot.world&&input.connection==empty::snapshot.connection){
        const auto inventory=Read<void*>(input.pawn,0xc08);
        r.owned=Live(input.pawn,r.pawnId)&&Live(input.movement,r.movementId)&&Live(inventory,r.inventoryId);
        const auto reader=sourceReader.load(std::memory_order_acquire);
        if(r.owned&&reader){
            if(!reader(input.world,input.nativeNetDriver,r.sourceBefore))r.sourceBefore.verified=false;
            r.predictionBefore=Read<void*>(input.movement,0x7f0);
            r.currentPredictionOwnsClock=r.predictionBefore&&input.clock==static_cast<uint8_t*>(r.predictionBefore)+0xf0;
            r.humanClockLayoutRead=true;r.humanClockTable=Read<void*>(input.clock);r.humanClockVbtable=Read<void*>(input.clock,0x10);
            const int32_t base=Read<int32_t>(r.humanClockVbtable,4);r.humanClockVbase=base;r.cachedClockVbase=input.clockBaseOffset;
            r.primaryClockTableVerified=r.humanClockTable==reinterpret_cast<void*>(engine::image+0xb600a60);
            // Role3 concrete server prediction keeps EB8 {-10,9F0}: its
            // common ctor receives EDX=0. Mode/private generic E90/990 is distinct.
            r.humanVbtableVerified=r.humanClockVbtable==reinterpret_cast<void*>(engine::image+0xc6b3eb8)
                &&Read<int32_t>(r.humanClockVbtable,0)==-0x10&&base==0x9f0;
            r.cachedVbaseMatches=base>0&&static_cast<uint32_t>(base)==input.clockBaseOffset;
            r.clockShapeVerified=r.primaryClockTableVerified&&r.humanVbtableVerified&&r.cachedVbaseMatches;
            if(r.clockShapeVerified&&r.currentPredictionOwnsClock){
                r.humanClockFieldsRead=true;
                r.rawBefore=Read<uint32_t>(input.clock,static_cast<size_t>(base)+0x18);r.step=Read<uint32_t>(input.clock,static_cast<size_t>(base)+0x1c);
                r.actualOffsetBefore=Read<int32_t>(input.clock,0x24);r.aBefore=Read<float>(input.clock,0x20);
                r.cachedHumanAccumulator=input.clockAccumulated;r.cacheMatchesActualAccumulator=core::Bits(r.cachedHumanAccumulator)==core::Bits(r.aBefore);
                r.publication=ReadPublication(input.pawn,inventory,r.pawnId,r.inventoryId);
                r.initialAccumulator=input.nativeInitialAccumulatorSeconds;r.creditedSeconds=input.hostCreditedSeconds;r.unfundedSeconds=input.hostUnfundedSeconds;
                r.preOriginSeconds=input.hostPreOriginSeconds;r.excludedSeconds=input.hostOriginExcludedSeconds;
                r.fundingRoundoff=input.nativeFundingRoundoffSeconds;r.externalAccumulator=input.nativeExternalAccumulatorSeconds;
                r.nativeConservationError=input.hostCreditConservationError;r.timeConservationError=input.hostTimeConservationError;
                r.originCredit=input.hostOriginCreditSeconds;r.postOriginSeconds=input.hostPostOriginSeconds;
                r.nativeAdvanceAdjustment=input.nativeAdvanceAdjustmentSeconds;r.nativePostAdvanceAdjustment=input.nativePostAdvanceAccumulatorSeconds;
                r.previousPollAccumulator=input.nativeAccumulatorAtPollEnd;
                r.actualFunding=input.hostClockAccountingStarted&&input.hostFundingCalls>0&&input.hostCreditedSeconds>0;
                r.rawAfter=Read<uint32_t>(input.clock,static_cast<size_t>(base)+0x18);r.aAfter=Read<float>(input.clock,0x20);
                r.actualOffsetAfter=Read<int32_t>(input.clock,0x24);r.predictionAfter=Read<void*>(input.movement,0x7f0);
                if(!reader(input.world,input.nativeNetDriver,r.sourceAfter))r.sourceAfter.verified=false;
                r.distinctClockInstances=r.sourceBefore.clock&&r.sourceBefore.clock!=r.clock&&r.sourceAfter.clock==r.sourceBefore.clock;
                r.humanLayoutStableAfterRead=Read<void*>(input.clock)==r.humanClockTable&&Read<void*>(input.clock,0x10)==r.humanClockVbtable
                    &&Read<int32_t>(r.humanClockVbtable,0)==-0x10&&Read<int32_t>(r.humanClockVbtable,4)==base&&input.clockBaseOffset==r.cachedClockVbase;
                r.samePoint=r.humanLayoutStableAfterRead&&r.distinctClockInstances&&r.cacheMatchesActualAccumulator&&core::Bits(input.clockAccumulated)==core::Bits(r.cachedHumanAccumulator)
                    &&SameSource(r.sourceBefore,r.sourceAfter)&&r.predictionBefore&&r.predictionBefore==r.predictionAfter
                    &&input.clock==r.clock&&Read<void*>(input.movement,0x7f0)==r.predictionBefore&&r.actualOffsetBefore==r.actualOffsetAfter
                    &&Live(input.pawn,r.pawnId)&&Live(input.movement,r.movementId)&&Live(inventory,r.inventoryId);
            }
        }
    }
    const core::ArithmeticInput arithmetic{r.sameThread,r.owned&&r.currentPredictionOwnsClock&&r.clockShapeVerified,r.sourceBefore.verified&&r.sourceAfter.verified,r.samePoint,r.actualFunding,
        r.sourceBefore.c,r.sourceAfter.c,r.rawBefore,r.rawAfter,r.step,r.actualOffsetBefore,r.aBefore,r.aAfter};
    r.arithmetic=core::Diagnose(arithmetic);r.reason=r.arithmetic.reason;
    if(r.sameThread&&r.owned&&!r.currentPredictionOwnsClock)r.reason="current_prediction_clock_membership_not_verified";
    else if(r.sameThread&&r.owned&&!r.primaryClockTableVerified)r.reason="original_human_clock_primary_vtable_not_verified";
    else if(r.sameThread&&r.owned&&!r.humanVbtableVerified)r.reason="original_human_clock_EB8_9F0_virtual_base_not_verified";
    else if(r.sameThread&&r.owned&&!r.cachedVbaseMatches)r.reason="human_clock_virtual_base_differs_from_original_ClockState_cache";
    AcquireSRWLockExclusive(&recordLock);records[slot]=r;ReleaseSRWLockExclusive(&recordLock);
}
template<class Input> inline void AfterSupply(const Input& input){
    if(!Scope()||input.epochNumber!=1||!input.hostClockAccountingStarted||!input.hostFundingCalls||!input.hostCreditedSeconds)return;
    if(!records[static_cast<size_t>(Phase::FirstAuditedFundedPoll)].attempted)Capture(Phase::FirstAuditedFundedPoll,input,input.wireOriginClientIndex,false);
    if(input.wireAligned&&!input.physicsCompleted&&!records[static_cast<size_t>(Phase::BeforeFirstNativeAdvance)].attempted)
        Capture(Phase::BeforeFirstNativeAdvance,input,input.wireOriginClientIndex,false);
}
template<class Input> inline void BeforePrivateSeed(const Input& input,uint32_t firstClientIndex){
    if(!Scope()||input.wireAligned||input.epochNumber!=1)return;Capture(Phase::BeforeFirstPrivateSeed,input,firstClientIndex,true);
}
template<class Input> inline void AfterAccepted(const Input& input){
    if(!Scope()||input.epochNumber!=1||input.acceptedPackets!=1)return;
    Capture(Phase::AfterFirstAcceptedPacket,input,input.wireOriginClientIndex,false);
}
inline const char* B(bool v){return v?"true":"false";}
inline void Number(FILE* f,double value){if(std::isfinite(value))std::fprintf(f,"%.17g",value);else std::fputs("null",f);}
inline void LedgerJson(FILE* f,const Record& r){
    std::fputs("{\"policy_eligibility_checked\":false",f);
    const struct {const char* name;double value;} values[]{
        {"initial_accumulator",r.initialAccumulator},{"credited_seconds",r.creditedSeconds},{"unfunded_seconds",r.unfundedSeconds},
        {"pre_origin_seconds",r.preOriginSeconds},{"excluded_seconds",r.excludedSeconds},{"funding_roundoff",r.fundingRoundoff},
        {"external_accumulator",r.externalAccumulator},{"native_conservation_error",r.nativeConservationError},{"time_conservation_error",r.timeConservationError},
        {"origin_credit_seconds",r.originCredit},{"post_origin_seconds",r.postOriginSeconds},{"native_advance_adjustment",r.nativeAdvanceAdjustment},
        {"native_post_advance_adjustment",r.nativePostAdvanceAdjustment},{"previous_poll_accumulator",r.previousPollAccumulator}};
    for(const auto& field:values){std::fprintf(f,",\"%s\":",field.name);Number(f,field.value);}std::fputc('}',f);
}
inline void SourceJson(FILE* f,const PublicSource& s){
    std::fprintf(f,"{\"verified\":%s,\"reason\":\"%s\",\"source_epoch\":%u,\"C\":%u,\"step\":%u,\"offset\":%d,\"A_mode_unused_for_Q\":%.9g,\"game_start_submitted_at_read\":%s,\"native_start_schedule_submitted_at_read\":%s,\"world\":\"%p\",\"driver\":\"%p\",\"mode\":\"%p\",\"game_state\":\"%p\",\"timing\":\"%p\",\"fixed\":\"%p\",\"clock\":\"%p\",\"mode_weak\":[%d,%d],\"timing_weak\":[%d,%d],\"game_state_weak\":[%d,%d],\"fixed_weak\":[%d,%d]}",
        B(s.verified),s.reason,s.sourceEpoch,s.c,s.step,s.offset,double(std::isfinite(s.accumulated)?s.accumulated:0),B(s.gameStartSubmitted),B(s.nativeStartScheduleSubmitted),s.world,s.driver,s.mode,s.gameState,s.timing,s.fixed,s.clock,
        s.modeId.index,s.modeId.serial,s.timingId.index,s.timingId.serial,s.gameStateId.index,s.gameStateId.serial,s.fixedId.index,s.fixedId.serial);
}
inline void Write(FILE* f){
    std::array<Record,static_cast<size_t>(Phase::Count)> copy{};AcquireSRWLockShared(&recordLock);copy=records;ReleaseSRWLockShared(&recordLock);
    AcquireSRWLockShared(&empty::snapshotLock);const bool enabled=empty::snapshot.armed&&empty::snapshot.role==1;const auto pid=empty::snapshot.pid;char nonce[33]{};std::memcpy(nonce,empty::snapshot.nonceHex,33);ReleaseSRWLockShared(&empty::snapshotLock);
    std::fprintf(f,"{\"schema_version\":1,\"enabled\":%s,\"owner_marker_pid\":%u,\"owner_marker_nonce\":\"%s\",\"source_reader_bound\":%s,\"scope\":\"explicit_empty_probe_host_human0_life1_observation_only\",\"policy_applied\":false,\"formal_publications_released\":false,\"game_clock_writes_by_observer\":0,\"new_native_clock_getter_calls\":0,\"frame_scope\":\"single_synchronous_game_thread_call_with_double_readback_not_a_global_frame_id\",\"records\":[",B(enabled),pid,nonce,B(sourceReader.load(std::memory_order_acquire)!=nullptr));
    for(size_t i=0;i<copy.size();++i){const auto& r=copy[i];if(i)std::fputc(',',f);
        std::fprintf(f,"{\"phase\":\"%s\",\"attempted\":%s,\"same_thread\":%s,\"owned\":%s,\"same_point\":%s,\"reason\":\"%s\",\"arithmetic_valid\":%s,\"A_before\":%.9g,\"A_after\":%.9g,\"A_bits_before\":%u,\"A_bits_after\":%u,\"Q_funded_whole_steps\":%u,\"O_diagnostic_candidate\":%u,\"fractional_native_credit\":%.9g,\"Q_is_executable_work\":false,\"O_is_retail_birth\":false,\"actual_funding\":%s,\"input_poll_serial\":%llu,\"actual_thread\":%u,\"game_thread\":%u,\"epoch\":%llu,\"in_receive\":%s,\"in_poll\":%s,\"raw_before\":%u,\"raw_after\":%u,\"step\":%u,\"actual_offset_before\":%d,\"actual_offset_after\":%d,\"first_client_index\":%u,\"decoded_move_supplied\":%s,\"packet_accepted_at_point\":%s,\"received_packets\":%llu,\"accepted_packets\":%llu,\"funding_calls\":%llu,\"native_seed_calls\":%llu,\"physics_completed\":%llu,\"wire_aligned\":%s,\"mapping_constructed\":%s,\"timestep_rpc_submitted\":%s,\"bound_tuple_formal_pending\":%s,\"pawn\":\"%p\",\"movement\":\"%p\",\"clock\":\"%p\",\"prediction_before\":\"%p\",\"prediction_after\":\"%p\",\"time_shift\":\"%p\",\"pawn_weak\":[%d,%d],\"movement_weak\":[%d,%d],\"inventory_weak\":[%d,%d]",
            core::Name(static_cast<Phase>(i)),B(r.attempted),B(r.sameThread),B(r.owned),B(r.samePoint),r.reason,B(r.arithmetic.valid),double(std::isfinite(r.aBefore)?r.aBefore:0),double(std::isfinite(r.aAfter)?r.aAfter:0),core::Bits(r.aBefore),core::Bits(r.aAfter),r.arithmetic.wholeFundedSteps,r.arithmetic.originCandidate,double(r.arithmetic.remainder),B(r.actualFunding),
            static_cast<unsigned long long>(r.inputPollSerial),r.actualThread,r.gameThread,static_cast<unsigned long long>(r.epoch),B(r.inReceive),B(r.inPoll),r.rawBefore,r.rawAfter,r.step,r.actualOffsetBefore,r.actualOffsetAfter,r.firstClientIndex,B(r.firstDecodedMoveSupplied),B(r.packetAcceptedAtPoint),
            static_cast<unsigned long long>(r.receivedPackets),static_cast<unsigned long long>(r.acceptedPackets),static_cast<unsigned long long>(r.fundingCalls),static_cast<unsigned long long>(r.nativeSeedCalls),static_cast<unsigned long long>(r.physicsCompleted),B(r.wireAligned),B(r.mappingConstructed),B(r.timestepRpcSubmitted),B(r.formalBoundTupleStillPending),
            r.pawn,r.movement,r.clock,r.predictionBefore,r.predictionAfter,r.timeShift,r.pawnId.index,r.pawnId.serial,r.movementId.index,r.movementId.serial,r.inventoryId.index,r.inventoryId.serial);
        std::fprintf(f,",\"human_clock_layout_read\":%s,\"human_clock_fields_read\":%s,\"human_clock_primary_table\":\"%p\",\"human_clock_vbtable\":\"%p\",\"human_clock_vbase\":%d,\"original_ClockState_cached_vbase\":%u,\"human_primary_table_verified\":%s,\"human_EB8_vbtable_verified\":%s,\"cached_vbase_matches\":%s,\"human_layout_stable_after_read\":%s,\"unread_numeric_fields_are_default_not_observed_zero\":%s",
            B(r.humanClockLayoutRead),B(r.humanClockFieldsRead),r.humanClockTable,r.humanClockVbtable,r.humanClockVbase,r.cachedClockVbase,B(r.primaryClockTableVerified),B(r.humanVbtableVerified),B(r.cachedVbaseMatches),B(r.humanLayoutStableAfterRead),B(!r.humanClockFieldsRead));
        std::fprintf(f,",\"human_clock_shape_verified\":%s,\"current_prediction_owns_human_clock\":%s,\"A_scope\":\"actual_human_prediction_clock_plus20_funded_accumulator\"",B(r.clockShapeVerified),B(r.currentPredictionOwnsClock));
        std::fprintf(f,",\"human_and_mode_clock_instances_distinct\":%s",B(r.distinctClockInstances));
        std::fprintf(f,",\"world\":\"%p\",\"driver\":\"%p\",\"pc\":\"%p\",\"player_state\":\"%p\",\"connection\":\"%p\",\"cached_human_A\":%.9g,\"cache_matches_actual_human_A\":%s,\"clock_advances\":%llu",r.world,r.driver,r.pc,r.ps,r.connection,double(std::isfinite(r.cachedHumanAccumulator)?r.cachedHumanAccumulator:0),B(r.cacheMatchesActualAccumulator),static_cast<unsigned long long>(r.clockAdvances));
        std::fputs(",\"ledger\":",f);LedgerJson(f,r);
        std::fprintf(f,",\"publication\":{\"inventory_empty\":%s,\"slots_verified\":%u,\"slots_empty\":%u,\"force_typed\":%s,\"force_owners\":%s,\"force_arrays_valid\":%s,\"force_arrays_empty\":%s,\"force_count\":%d,\"force_capacity\":%d,\"predicted_count\":%d,\"predicted_capacity\":%d,\"all_timestamp_sources_unpublished_proven\":false},\"source_before\":",
            B(r.publication.inventory.empty),r.publication.inventory.slotsVerified,r.publication.inventory.slotsEmpty,
            B(r.publication.forceTyped),B(r.publication.forceOwners),B(r.publication.forceArraysValid),B(r.publication.forceArraysEmpty),r.publication.forceCount,r.publication.forceCapacity,r.publication.predictedCount,r.publication.predictedCapacity);
        SourceJson(f,r.sourceBefore);std::fputs(",\"source_after\":",f);SourceJson(f,r.sourceAfter);std::fputc('}',f);
    }
    std::fputs("]}",f);
}
} // namespace local_server_human_funded_boundary_probe
