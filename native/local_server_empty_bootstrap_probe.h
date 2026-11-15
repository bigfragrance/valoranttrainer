#pragma once
#include "original_inventory_slots.h"
#include "empty_bootstrap_probe_core.h"
#include <Windows.h>
#include <array>
#include <cstdio>
#include <cwchar>
#include <string>

// Explicit, bounded life-one empty-equipment packet probe. It does not grant,
// advance, fund, tick, initialize prediction, set readiness or submit an ACK.
// No marker: every engine-facing entry returns before touching engine objects.
namespace local_server_empty_bootstrap_probe {
namespace core=empty_bootstrap_probe_core;
using Weak=original_inventory_slots::WeakIdentity;
struct EmptyInventory {
    bool observed=false,typed=false,owners=false,weakVerified=false,hostSlotCache=false,empty=false;
    uint32_t slotsVerified=0,slotsEmpty=0,multiSlots=0;
    void* inventory=nullptr;std::array<void*,5> equipped{};
    uint64_t slotValidMask=0,slotEmptyMask=0;
    const char* guard="inventory_not_observed";
};
struct Snapshot {
    bool initialized=false,requested=false,armed=false,hostBound=false,clientBound=false,scopeRejected=false,expired=false,invalidated=false,observationComplete=false;
    uint32_t role=0,pid=0,polls=0,maxPolls=0,expectedEpoch=0,epoch=0;
    char nonceHex[33]{};
    uint64_t scopeAttempts=0,loadoutDeferrals=0,armorDeferrals=0,skillDeferrals=0,effectPlayRejections=0;
    uint64_t originalApplyReturns=0,originalApplyReturnsAtLastPoll=0;
    core::Admission admission=core::Admission::Off;
    const char* stage="disabled_no_explicit_marker";
    const char* scopeGuard="not_examined";
    void *world=nullptr,*pc=nullptr,*ps=nullptr,*pawn=nullptr,*connection=nullptr,*movement=nullptr,*prediction=nullptr,*clock=nullptr;
    void *firstPawn=nullptr,*firstMovement=nullptr,*firstInventory=nullptr,*firstPrediction=nullptr;
    Weak firstPawnId{},firstMovementId{},firstInventoryId{};
    Weak pcId{},psId{},pawnId{},inventoryId{},movementId{};
    bool sourceReady=false;uint64_t sourceMask=0;int32_t weakFailure=-1;uintptr_t pcTable=0;
    std::array<Weak,12> clientIdentities{};
    EmptyInventory inventory{};
    core::HostEvidence host{};core::ClientEvidence client{};
    uint64_t rejectedPackets=0,decodedMoves=0,uniqueMoves=0,inputEventsApplied=0,blockedApplyAttempts=0;
    uint64_t firstPacket=0;uint32_t firstCount=0,firstIndex=0,lastIndex=0;uint8_t firstStep=0;float firstTimeShift=0;
    int32_t applyBase=-1,applyBefore=-1,applyAfter=-1,applyEligible=-1;
    uint32_t inputError=0,clientRawIndex=0;int32_t clientTimestep=0;
    const char* inputStage="not_observed";const char* clientClockStage="not_observed";
    uint32_t exceptionCode=0;
};
inline SRWLOCK snapshotLock=SRWLOCK_INIT;
inline Snapshot snapshot{};
inline bool markerChecked=false,armed=false;
inline core::Marker marker{};
inline core::ObservationCadence observationCadence{};
inline bool clientClockObservationPending=false;
inline bool ObservationDue(){return observationCadence.Due(GetTickCount64());}
struct Exclusive {SRWLOCK* value;explicit Exclusive(SRWLOCK& lock):value(&lock){AcquireSRWLockExclusive(value);}~Exclusive(){ReleaseSRWLockExclusive(value);}};
inline std::u16string Utf16(const std::wstring& s){return std::u16string(reinterpret_cast<const char16_t*>(s.data()),s.size());}
inline bool FullPath(const wchar_t* path,std::wstring& result){
    wchar_t value[32768]{};const DWORD n=GetFullPathNameW(path,32768,value,nullptr);
    if(!n||n>=32768)return false;result.assign(value,n);return true;
}
inline void Initialize(uint32_t role,const wchar_t* roleDirectory){
    if(markerChecked)return;markerChecked=true;Exclusive guard(snapshotLock);snapshot.initialized=true;snapshot.role=role;snapshot.pid=GetCurrentProcessId();
    wchar_t markerPath[32768]{},nonceText[64]{};
    const DWORD pathBytes=GetEnvironmentVariableW(L"TRAINER_EMPTY_BOOTSTRAP_MARKER",markerPath,32768);
    const DWORD nonceBytes=GetEnvironmentVariableW(L"TRAINER_EMPTY_BOOTSTRAP_NONCE",nonceText,64);
    if(!pathBytes&&!nonceBytes)return;
    snapshot.requested=true;
    auto reject=[](core::Admission code){snapshot.admission=code;snapshot.stage=core::Name(code);};
    uint8_t nonce[16]{};
    if(!pathBytes||pathBytes>=32768||!nonceBytes||nonceBytes>=64||!core::ParseNonce(Utf16(nonceText),nonce)){
        reject(core::Admission::InvalidNonce);return;
    }
    std::wstring actualMarker,actualDirectory;
    if(!roleDirectory||!FullPath(markerPath,actualMarker)||!FullPath(roleDirectory,actualDirectory)){
        reject(core::Admission::InvalidMarkerPath);return;
    }
    // Validate the destination before opening it, even for a rejected marker.
    const auto markerName=Utf16(actualMarker);std::u16string directoryName;
    if(!core::NormalizeRoleDirectory(Utf16(actualDirectory),role,directoryName)){
        reject(core::Admission::InvalidMarkerPath);return;
    }
    const auto slash=directoryName.find_last_of(u'\\');
    if(slash==std::u16string::npos||!core::PairPath(directoryName.substr(0,slash))
       ||!core::EqualPath(directoryName.substr(slash+1),role==1?u"host":role==2?u"client":u"invalid")
       ||!core::EqualPath(markerName,directoryName+u"\\empty-equipment-bootstrap.bin")){
        reject(core::Admission::InvalidMarkerPath);return;
    }
    HANDLE file=CreateFileW(actualMarker.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){reject(core::Admission::InvalidSize);return;}
    LARGE_INTEGER size{};DWORD got=0;core::Marker value{};wchar_t finalPath[32768]{};
    const DWORD finalBytes=GetFinalPathNameByHandleW(file,finalPath,32768,FILE_NAME_NORMALIZED|VOLUME_NAME_DOS);
    std::wstring resolved=finalBytes&&finalBytes<32768?std::wstring(finalPath,finalBytes):L"";
    if(resolved.rfind(L"\\\\?\\",0)==0)resolved.erase(0,4);
    const bool canonical=core::EqualPath(Utf16(resolved),markerName);
    const bool read=canonical&&GetFileSizeEx(file,&size)&&size.QuadPart==sizeof(value)
        &&ReadFile(file,&value,sizeof(value),&got,nullptr)&&got==sizeof(value);CloseHandle(file);
    if(!canonical){reject(core::Admission::InvalidMarkerPath);return;}
    if(!read){reject(core::Admission::InvalidSize);return;}
    wchar_t currentImage[32768]{};DWORD imageBytes=32768;
    if(!QueryFullProcessImageNameW(GetCurrentProcess(),0,currentImage,&imageBytes)){
        reject(core::Admission::InvalidImage);return;
    }
    const auto admission=core::Admit(value,sizeof(value),role,GetCurrentProcessId(),nonce,Utf16(std::wstring(currentImage,imageBytes)),directoryName,markerName);
    snapshot.admission=admission;snapshot.stage=core::Name(admission);if(admission!=core::Admission::Armed)return;
    marker=value;armed=true;snapshot.armed=true;snapshot.maxPolls=marker.maxPolls;snapshot.expectedEpoch=marker.expectedEpoch;
    constexpr char hex[]="0123456789abcdef";
    for(size_t i=0;i<16;++i){snapshot.nonceHex[i*2]=hex[marker.nonce[i]>>4];snapshot.nonceHex[i*2+1]=hex[marker.nonce[i]&15];}
}
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline EmptyInventory ReadEmpty(void* pawn,void* inventory,const Weak& pawnId,const Weak& inventoryId,bool host){
    EmptyInventory out;out.observed=true;out.inventory=inventory;
    out.weakVerified=Live(pawn,pawnId)&&Live(inventory,inventoryId);
    if(!out.weakVerified){out.guard="current_pawn_inventory_weak_identity_failed";return out;}
    out.typed=original_players::IsA(inventory,0x6499330);
    out.owners=Read<void*>(pawn,0xc08)==inventory&&Read<void*>(inventory,0xc8)==pawn&&Read<void*>(inventory,0x2f0)==pawn;
    if(!out.typed||!out.owners){out.guard="typed_inventory_or_owner_failed";return out;}
    const original_inventory_slots::Inventory* cache=nullptr;
    if(host){for(const auto& i:original_inventory_slots::inventories)if(i.ready&&i.pawn==pawn&&i.object==inventory
        &&original_inventory_slots::SameIdentity(i.pawnIdentity,pawnId)&&original_inventory_slots::SameIdentity(i.inventoryIdentity,inventoryId)){cache=&i;break;}
        out.hostSlotCache=cache!=nullptr;if(!cache){out.guard="original_17_slot_ready_cache_missing";return out;}}
    constexpr size_t offsets[]{0x268,0x278,0x280,0x2a0,0x320};bool pointersEmpty=true;
    for(size_t i=0;i<std::size(offsets);++i){out.equipped[i]=Read<void*>(inventory,offsets[i]);pointersEmpty&=out.equipped[i]==nullptr;}
    struct Array {void* data;int32_t count,capacity;};
    for(size_t i=0;i<original_inventory_slots::SlotCount;++i){auto slot=Read<void*>(inventory,0x1b8+i*8);
        const bool valid=(!host||slot==cache->slots[i].object)&&original_players::IsA(slot,0x68f63c0)
            &&Read<void*>(slot,0x28)==inventory&&Read<void*>(slot,0x40)==inventory&&Read<uint8_t>(slot,0x30)==i;
        if(!valid)continue;++out.slotsVerified;out.slotValidMask|=uint64_t(1)<<i;
        bool empty=Read<void*>(slot,0x38)==nullptr;
        if(original_players::IsA(slot,0x69cf3d0)){++out.multiSlots;const auto contents=Read<Array>(slot,0x50);
            empty&=contents.count==0&&contents.capacity>=0&&contents.capacity<=4096;}
        if(empty){++out.slotsEmpty;out.slotEmptyMask|=uint64_t(1)<<i;}
    }
    out.empty=pointersEmpty&&out.slotsVerified==original_inventory_slots::SlotCount&&out.slotsEmpty==original_inventory_slots::SlotCount;
    out.guard=out.empty?"real_17_slots_and_equipped_pointers_empty":!pointersEmpty?"real_equipped_pointer_not_null":out.slotsVerified!=17?"original_slot_owner_type_or_cache_failed":"real_slot_contents_not_empty";
    return out;
}
template<class Player> inline bool PendingHuman(const core::Scope& scope,const Player* player){
    if(!armed||snapshot.role!=1)return false;
    if(!scope.enabled||!scope.host||!scope.freeForAll||!scope.human||scope.index!=0||scope.epoch!=marker.expectedEpoch)return false;
    core::Scope actual=scope;actual.armed=true;actual.role=snapshot.role;
    actual.baselineRegistered=player&&player->registered&&!player->error&&!player->retiring&&player->sourceEpoch==scope.epoch
        &&reinterpret_cast<uintptr_t>(player->controller)==scope.pc&&reinterpret_cast<uintptr_t>(player->playerState)==scope.ps
        &&reinterpret_cast<uintptr_t>(player->pawn)==scope.pawn&&reinterpret_cast<uintptr_t>(player->inventory)==scope.inventory;
    actual.weakVerified=actual.baselineRegistered&&Live(player->controller,player->controllerIdentity)&&Live(player->playerState,player->stateIdentity)
        &&Live(player->pawn,player->pawnIdentity)&&Live(player->inventory,player->inventoryIdentity);
    const bool hold=core::Hold(actual);
    AcquireSRWLockExclusive(&snapshotLock);++snapshot.scopeAttempts;
    if(!hold){snapshot.scopeRejected=true;snapshot.scopeGuard="host_ffa_human_life1_current_baseline_scope_rejected";ReleaseSRWLockExclusive(&snapshotLock);return false;}
    if(snapshot.hostBound&&(snapshot.world!=reinterpret_cast<void*>(scope.world)||snapshot.pc!=player->controller||snapshot.ps!=player->playerState
       ||snapshot.pawn!=player->pawn||snapshot.connection!=reinterpret_cast<void*>(scope.connection))){
        snapshot.invalidated=true;snapshot.scopeGuard="bound_probe_tuple_changed";ReleaseSRWLockExclusive(&snapshotLock);return false;}
    snapshot.hostBound=true;snapshot.epoch=scope.epoch;snapshot.world=reinterpret_cast<void*>(scope.world);snapshot.pc=player->controller;
    snapshot.ps=player->playerState;snapshot.pawn=player->pawn;snapshot.connection=reinterpret_cast<void*>(scope.connection);
    snapshot.pcId=player->controllerIdentity;snapshot.psId=player->stateIdentity;snapshot.pawnId=player->pawnIdentity;snapshot.inventoryId=player->inventoryIdentity;
    snapshot.scopeGuard="exact_owned_ffa_human_life1_baseline_registered";snapshot.stage="human_formal_publications_pending_real_empty_packet";
    ++snapshot.armorDeferrals;ReleaseSRWLockExclusive(&snapshotLock);return true;
}
inline bool HoldBound(size_t index,void* pawn,uint32_t epoch){
    if(!armed||snapshot.role!=1||!snapshot.hostBound||index!=0||epoch!=marker.expectedEpoch||snapshot.pawn!=pawn)return false;
    AcquireSRWLockExclusive(&snapshotLock);++snapshot.loadoutDeferrals;ReleaseSRWLockExclusive(&snapshotLock);return true;
}
inline bool RejectNewEffectPlay(size_t index,void* pawn,uint32_t epoch){
    if(!armed||snapshot.role!=1||!snapshot.hostBound||index!=0||epoch!=marker.expectedEpoch||snapshot.pawn!=pawn)return false;
    AcquireSRWLockExclusive(&snapshotLock);++snapshot.effectPlayRejections;ReleaseSRWLockExclusive(&snapshotLock);return true;
}
inline void OriginalApplyReturned(void* pawn,uint64_t epoch){
    if(!armed||snapshot.role!=1||!snapshot.hostBound||snapshot.invalidated||snapshot.observationComplete||snapshot.expired||snapshot.pawn!=pawn||epoch!=marker.expectedEpoch)return;
    AcquireSRWLockExclusive(&snapshotLock);++snapshot.originalApplyReturns;ReleaseSRWLockExclusive(&snapshotLock);
}
template<class Input> inline void ObserveHostPoll(const Input& input){
    if(!armed||snapshot.role!=1||!snapshot.hostBound||snapshot.observationComplete||snapshot.expired)return;
    if(!ObservationDue())return;
    if(snapshot.polls>=marker.maxPolls){Exclusive guard(snapshotLock);snapshot.expired=true;snapshot.stage="probe_poll_bound_reached_publications_still_pending";return;}
    const auto inventory=Read<void*>(input.pawn,0xc08);
    const bool current=input.pawn==snapshot.pawn&&input.pc==snapshot.pc&&input.playerState==snapshot.ps
        &&input.world==snapshot.world&&input.connection==snapshot.connection&&input.epochNumber==marker.expectedEpoch
        &&Live(input.pawn,snapshot.pawnId)&&Live(inventory,snapshot.inventoryId)&&Live(input.movement,{input.movementIdentity.index,input.movementIdentity.serial});
    EmptyInventory empty{};void* prediction=nullptr;
    if(current){empty=ReadEmpty(input.pawn,inventory,snapshot.pawnId,snapshot.inventoryId,true);prediction=Read<void*>(input.movement,0x7f0);}
    AcquireSRWLockExclusive(&snapshotLock);++snapshot.polls;
    if(current&&prediction){if(!snapshot.firstPrediction)snapshot.firstPrediction=prediction;else if(snapshot.firstPrediction!=prediction)snapshot.invalidated=true;}
    snapshot.invalidated|=!current;snapshot.inventory=empty;snapshot.movement=input.movement;snapshot.prediction=prediction;snapshot.clock=input.clock;
    snapshot.movementId={input.movementIdentity.index,input.movementIdentity.serial};snapshot.originalApplyReturnsAtLastPoll=snapshot.originalApplyReturns;
    snapshot.host={input.installed,input.ownedLinks&&input.weakIdentitiesVerified,input.firstMoveBatchCaptured,empty.empty,current&&prediction&&input.clock,
        static_cast<uint32_t>(input.epochNumber),input.error,input.clockIndex,input.lastConsumedIndex,input.clockStep,
        input.receivedPackets,input.acceptedPackets,input.inputApplyCalls,snapshot.originalApplyReturns,input.physicsCompleted};
    snapshot.rejectedPackets=input.rejectedPackets;snapshot.decodedMoves=input.decodedMoves;snapshot.uniqueMoves=input.uniqueMoves;
    snapshot.inputEventsApplied=input.inputEventsApplied;snapshot.blockedApplyAttempts=input.inputBlockedApplyAttempts;
    snapshot.firstPacket=input.firstMoveBatch.packet;snapshot.firstCount=input.firstMoveBatch.count;snapshot.firstIndex=input.firstMoveBatch.first;
    snapshot.lastIndex=input.firstMoveBatch.last;snapshot.firstStep=input.firstMoveBatch.firstStep;snapshot.firstTimeShift=input.firstMoveBatch.timeShift;
    snapshot.applyBase=input.inputApplyDiagnostic.base;snapshot.applyBefore=input.inputApplyDiagnostic.appliedBefore;
    snapshot.applyAfter=input.inputApplyDiagnostic.appliedAfter;snapshot.applyEligible=input.inputApplyDiagnostic.eligible;
    snapshot.inputError=input.error;snapshot.inputStage=input.stage;
    snapshot.stage=snapshot.invalidated?"probe_failed_current_host_identity_or_prediction_changed":input.error?"probe_failed_original_input_guard":snapshot.expired?"probe_poll_bound_reached_publications_still_pending"
        :core::HostWitness(snapshot.host)?"real_empty_equipment_native_packet_apply_physics_witnessed":!empty.empty?"probe_waiting_real_empty_inventory"
        :!input.acceptedPackets?"probe_waiting_first_real_accepted_packet":!snapshot.originalApplyReturns?"probe_waiting_original_apply_return":"probe_waiting_completed_physics_clock_consumed_agreement";
    snapshot.observationComplete=!snapshot.invalidated&&core::HostWitness(snapshot.host);
    ReleaseSRWLockExclusive(&snapshotLock);
}
template<class Source> inline void ObserveClientSource(bool ready,uint64_t mask,int32_t weakFailure,uintptr_t pcTable,const Source& source){
    if(!armed||snapshot.role!=2)return;
    clientClockObservationPending=false;
    if(snapshot.observationComplete||snapshot.expired||!ObservationDue())return;
    if(snapshot.polls>=marker.maxPolls){Exclusive guard(snapshotLock);snapshot.expired=true;snapshot.stage="probe_poll_bound_reached_client_observation_only";return;}
    EmptyInventory empty{};if(ready)empty=ReadEmpty(source.pawn,source.inventory,source.identities[3],source.identities[11],false);
    AcquireSRWLockExclusive(&snapshotLock);++snapshot.polls;
    if(ready){
        if(!snapshot.clientBound){snapshot.clientBound=true;snapshot.firstPawn=source.pawn;snapshot.firstMovement=source.movement;snapshot.firstInventory=source.inventory;
            snapshot.firstPawnId=source.identities[3];snapshot.firstMovementId=source.identities[10];snapshot.firstInventoryId=source.identities[11];}
        else if(snapshot.firstPawn!=source.pawn||snapshot.firstMovement!=source.movement||snapshot.firstInventory!=source.inventory
            ||!original_inventory_slots::SameIdentity(snapshot.firstPawnId,source.identities[3])
            ||!original_inventory_slots::SameIdentity(snapshot.firstMovementId,source.identities[10])
            ||!original_inventory_slots::SameIdentity(snapshot.firstInventoryId,source.identities[11]))snapshot.invalidated=true;
    }
    snapshot.sourceReady=ready;snapshot.sourceMask=mask;snapshot.weakFailure=weakFailure;snapshot.pcTable=pcTable;snapshot.inventory=empty;
    snapshot.world=source.world;snapshot.pc=source.pc;snapshot.ps=source.ps;snapshot.pawn=source.pawn;snapshot.connection=source.connection;snapshot.movement=source.movement;
    snapshot.clientIdentities=source.identities;snapshot.pcId=source.identities[1];snapshot.psId=source.identities[2];
    snapshot.pawnId=source.identities[3];snapshot.movementId=source.identities[10];snapshot.inventoryId=source.identities[11];
    snapshot.client.ready=ready;snapshot.client.sourceMask=mask;snapshot.client.empty=empty.empty;
    clientClockObservationPending=ready&&!snapshot.invalidated;
    snapshot.stage=snapshot.invalidated?"probe_failed_current_client_source_changed":snapshot.expired?"probe_poll_bound_reached_client_observation_only":!ready?"probe_waiting_unchanged_original_client_source_gates":!empty.empty?"probe_waiting_real_client_empty_17_slots":"real_client_source_ready_empty_inventory_waiting_original_prediction";
    ReleaseSRWLockExclusive(&snapshotLock);
}
template<class Diagnostic> inline void ObserveClientClock(const Diagnostic& d){
    if(!armed||snapshot.role!=2||!snapshot.sourceReady||snapshot.observationComplete||snapshot.expired||!clientClockObservationPending)return;
    clientClockObservationPending=false;
    const bool current=d.pawn==snapshot.pawn&&d.movement==snapshot.movement&&d.prediction
        &&Read<void*>(d.movement,0x7e8)==d.prediction&&Live(d.pawn,snapshot.pawnId)&&Live(d.movement,snapshot.movementId);
    AcquireSRWLockExclusive(&snapshotLock);snapshot.prediction=d.prediction;snapshot.clock=d.clock;snapshot.clientRawIndex=d.rawIndex;snapshot.clientTimestep=d.timestep;
    if(current){if(!snapshot.firstPrediction)snapshot.firstPrediction=d.prediction;else if(snapshot.firstPrediction!=d.prediction)snapshot.invalidated=true;}
    snapshot.client.currentPrediction=current;snapshot.client.localOwner=d.localOwner;snapshot.client.observed=d.observed;snapshot.client.roundtrip=d.roundtrip;
    snapshot.clientClockStage=d.stage;
    snapshot.observationComplete=core::ClientWitness(snapshot.client)&&!snapshot.expired&&!snapshot.invalidated;
    if(snapshot.observationComplete)snapshot.stage="real_empty_client_source_and_current_original_prediction_observed";
    else if(snapshot.invalidated)snapshot.stage="probe_failed_current_client_prediction_changed";
    ReleaseSRWLockExclusive(&snapshotLock);
}
inline const char* Bool(bool x){return x?"true":"false";}
inline void WeakJson(FILE* f,const char* key,const Weak& weak){std::fprintf(f,"\"%s\":[%d,%d]",key,weak.index,weak.serial);}
inline void Write(FILE* f){
    AcquireSRWLockShared(&snapshotLock);const Snapshot s=snapshot;ReleaseSRWLockShared(&snapshotLock);
    // Cached POD only: serialization never reads an engine object or creates a
    // source/prediction/Weak identity. Host/client witnesses stay independent.
    std::fprintf(f,"{\"schema_version\":1,\"initialized\":%s,\"requested\":%s,\"armed\":%s,\"role\":%u,\"pid\":%u,\"admission\":\"%s\",\"stage\":\"%s\",\"scope_guard\":\"%s\",\"host_bound\":%s,\"scope_rejected\":%s,\"invalidated\":%s,\"expired\":%s,\"polls\":%u,\"max_polls\":%u,\"expected_epoch\":%u,\"epoch\":%u,\"nonce\":\"%s\",\"host_witness\":%s,\"client_witness\":%s,\"pair_acceptance\":false,\"formal_publications_pending\":%s,\"scope_attempts\":%llu,\"loadout_deferrals\":%llu,\"armor_deferrals\":%llu,\"skill_deferrals\":%llu,\"effect_play_invalid_id_rejections\":%llu",
        Bool(s.initialized),Bool(s.requested),Bool(s.armed),s.role,s.pid,core::Name(s.admission),s.stage,s.scopeGuard,Bool(s.hostBound),Bool(s.scopeRejected),Bool(s.invalidated),Bool(s.expired),s.polls,s.maxPolls,s.expectedEpoch,s.epoch,
        s.nonceHex,Bool(s.armed&&!s.expired&&!s.invalidated&&core::HostWitness(s.host)),Bool(s.armed&&!s.expired&&!s.invalidated&&core::ClientWitness(s.client)),Bool(s.hostBound),
        static_cast<unsigned long long>(s.scopeAttempts),static_cast<unsigned long long>(s.loadoutDeferrals),static_cast<unsigned long long>(s.armorDeferrals),static_cast<unsigned long long>(s.skillDeferrals),static_cast<unsigned long long>(s.effectPlayRejections));
    std::fprintf(f,",\"sample_interval_ms\":%u,\"polls_scope\":\"spaced_diagnostic_samples_not_game_ticks\",\"observation_time_scope\":\"process_monotonic_ms_diagnostics_only\"",core::ObservationIntervalMs);
    std::fprintf(f,",\"observation_complete\":%s,\"receipt_scope\":\"first_completed_observation_within_poll_bound_not_live_status\",\"owners\":{\"world\":\"%p\",\"pc\":\"%p\",\"ps\":\"%p\",\"pawn\":\"%p\",\"connection\":\"%p\",\"movement\":\"%p\",\"prediction_allocation\":\"%p\",\"first_prediction_allocation\":\"%p\",\"clock\":\"%p\",",Bool(s.observationComplete),s.world,s.pc,s.ps,s.pawn,s.connection,s.movement,s.prediction,s.firstPrediction,s.clock);
    WeakJson(f,"pc_weak",s.pcId);std::fputc(',',f);WeakJson(f,"ps_weak",s.psId);std::fputc(',',f);WeakJson(f,"pawn_weak",s.pawnId);std::fputc(',',f);WeakJson(f,"movement_weak",s.movementId);std::fputc(',',f);WeakJson(f,"inventory_weak",s.inventoryId);std::fputc('}',f);
    std::fprintf(f,",\"empty_inventory\":{\"observed\":%s,\"typed\":%s,\"owners\":%s,\"weak_verified\":%s,\"host_original_slot_cache\":%s,\"empty\":%s,\"guard\":\"%s\",\"inventory\":\"%p\",\"slots_verified\":%u,\"slots_empty\":%u,\"multi_slots\":%u,\"slot_valid_mask\":%llu,\"slot_empty_mask\":%llu,\"desired_268\":\"%p\",\"pointer_278\":\"%p\",\"current_280\":\"%p\",\"pointer_2a0\":\"%p\",\"corrected_320\":\"%p\"}",
        Bool(s.inventory.observed),Bool(s.inventory.typed),Bool(s.inventory.owners),Bool(s.inventory.weakVerified),Bool(s.inventory.hostSlotCache),Bool(s.inventory.empty),s.inventory.guard,s.inventory.inventory,s.inventory.slotsVerified,s.inventory.slotsEmpty,s.inventory.multiSlots,
        static_cast<unsigned long long>(s.inventory.slotValidMask),static_cast<unsigned long long>(s.inventory.slotEmptyMask),s.inventory.equipped[0],s.inventory.equipped[1],s.inventory.equipped[2],s.inventory.equipped[3],s.inventory.equipped[4]);
    std::fprintf(f,",\"client_source\":{\"ready\":%s,\"passed_mask\":%llu,\"required_mask\":1023,\"weak_failure_index\":%d,\"pc_table\":\"%p\",\"identities\":[",Bool(s.sourceReady),static_cast<unsigned long long>(s.sourceMask),s.weakFailure,reinterpret_cast<void*>(s.pcTable));
    for(size_t i=0;i<s.clientIdentities.size();++i)std::fprintf(f,"%s[%d,%d]",i?",":"",s.clientIdentities[i].index,s.clientIdentities[i].serial);
    std::fprintf(f,"],\"current_prediction\":%s,\"local_owner\":%s,\"clock_observed\":%s,\"roundtrip\":%s,\"raw_index\":%u,\"timestep\":%d,\"clock_stage\":\"%s\"}",Bool(s.client.currentPrediction),Bool(s.client.localOwner),Bool(s.client.observed),Bool(s.client.roundtrip),s.clientRawIndex,s.clientTimestep,s.clientClockStage);
    std::fprintf(f,",\"host_input\":{\"installed\":%s,\"owned\":%s,\"epoch\":%u,\"error\":%u,\"stage\":\"%s\",\"received_packets\":%llu,\"accepted_packets\":%llu,\"rejected_packets\":%llu,\"decoded_moves\":%llu,\"unique_moves\":%llu,\"original_apply_attempts\":%llu,\"original_apply_returns\":%llu,\"input_events_applied\":%llu,\"blocked_apply_attempts\":%llu,\"physics_completed\":%llu,\"clock_index\":%u,\"clock_step\":%u,\"last_consumed_index\":%u,\"current_prediction\":%s,\"apply_base\":%d,\"apply_before\":%d,\"apply_after\":%d,\"apply_eligible\":%d,\"first_batch\":{\"captured\":%s,\"packet\":%llu,\"count\":%u,\"first\":%u,\"last\":%u,\"first_step\":%u,\"time_shift\":%.9g}}}",
        Bool(s.host.installed),Bool(s.host.owned),s.host.epoch,s.inputError,s.inputStage,static_cast<unsigned long long>(s.host.received),static_cast<unsigned long long>(s.host.accepted),static_cast<unsigned long long>(s.rejectedPackets),
        static_cast<unsigned long long>(s.decodedMoves),static_cast<unsigned long long>(s.uniqueMoves),static_cast<unsigned long long>(s.host.applyCalls),static_cast<unsigned long long>(s.originalApplyReturnsAtLastPoll),static_cast<unsigned long long>(s.inputEventsApplied),static_cast<unsigned long long>(s.blockedApplyAttempts),
        static_cast<unsigned long long>(s.host.physicsCompleted),s.host.clockIndex,s.host.clockStep,s.host.consumed,Bool(s.host.currentPrediction),s.applyBase,s.applyBefore,s.applyAfter,s.applyEligible,Bool(s.host.firstBatch),static_cast<unsigned long long>(s.firstPacket),s.firstCount,s.firstIndex,s.lastIndex,unsigned(s.firstStep),double(s.firstTimeShift));
}
} // namespace local_server_empty_bootstrap_probe
