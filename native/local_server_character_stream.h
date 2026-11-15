#pragma once
#include "original_players.h"
#include "local_server_npc_time_domain.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <intrin.h>
#include <new>
#include <vector>

// Pinned image only. See work/local-server-playable/character-stream/report.md.
// This is a candidate authority capture/RPC producer. It does not establish that
// the peer consumed a stream. All client decoding, interpolation and FOW stay
// native. No client transform, role, local-player or visibility write occurs.
namespace local_server_character_stream {
inline constexpr size_t MaxPawns=32,MaxMoves=16,MovementTableEntries=400;
inline constexpr uintptr_t MovementTableRva=0xb4ecc50;
inline constexpr wchar_t ReceiveFunction[]=L"/Script/ShooterGame.AresPlayerController.ClientReceiveRemoteCharacterUpdatesSingleArrayNoAutonomous";
struct WeakIdentity { int32_t index=-1,serial=0; };
struct Array { void* data=nullptr;int32_t count=0,capacity=0; };
struct ComponentDataStream { uint8_t* data=nullptr;int32_t count=0,capacity=0;uint8_t unusedBits=0,pad[7]{}; };
struct RemoteCharacterUpdate { uint32_t guid=0,pad=0;ComponentDataStream stream{}; };
// The input form and full authority form share7893150.789CA00 supplies all
// common fields and velocity from a real native post-physics FSavedMove.
struct MoveRecord {
    uint8_t full=0,flags=0,roll=0,packedMovementMode=0,pad04[4]{};
    engine::Vec normalizedAcceleration{};
    uint32_t timeIndex=0,pad24=0;
    engine::Vec location{};
    float crouch=0;uint32_t pitchYaw=0;
    uint8_t firstAfterReset=0,pad49[7]{};
    void* movementBase=nullptr;uint8_t baseBone[12]{};float timeShift=0;
    uint8_t authTeleport=0,pad69[7]{};engine::Vec velocity{};uint8_t pad88[8]{};
};
static_assert(sizeof(WeakIdentity)==8&&sizeof(Array)==16&&sizeof(ComponentDataStream)==24);
static_assert(sizeof(RemoteCharacterUpdate)==32&&offsetof(RemoteCharacterUpdate,stream)==8);
static_assert(sizeof(MoveRecord)==0x90&&offsetof(MoveRecord,normalizedAcceleration)==8
    &&offsetof(MoveRecord,timeIndex)==0x20&&offsetof(MoveRecord,location)==0x28
    &&offsetof(MoveRecord,crouch)==0x40&&offsetof(MoveRecord,pitchYaw)==0x44
    &&offsetof(MoveRecord,firstAfterReset)==0x48&&offsetof(MoveRecord,authTeleport)==0x68
    &&offsetof(MoveRecord,velocity)==0x70);
struct State {
    bool initialized=false,profilesVerified=false,unsupported=false;
    bool rpcLayoutVerified=false,captureHookInstalled=false,lastRPCSubmitted=false;
    unsigned error=0,boundPawns=0,lastUpdateCount=0,lastMoveCount=0,lastBytes=0;
    uint64_t captureCallbacks=0,capturedMoves=0,nativeConverterCalls=0,nativeQueueResets=0;
    uint64_t rpcSubmissions=0,submittedMoves=0,ownPawnSkips=0,channelDeferrals=0;
    uint64_t retireCalls=0,retiredBindings=0,retireUnbound=0,retireIdempotent=0;
    uint64_t retireVptrRestored=0,retireForeignVptrPreserved=0,retireExpiredMovements=0;
    uint64_t retireRejectedInFlight=0,retireScratchDestructions=0,retiredMovesDiscarded=0;
    uint64_t retireUnpublished=0;
    uint64_t lateRetiredCallbacks=0,captureObserverMismatches=0,observerAllocations=0;
    uint64_t retainedBackingRequestedBytes=0,slotReuses=0;
    uint64_t lastRetiredGeneration=0;
    WeakIdentity lastRetiredPawnId{},lastRetiredMovementId{};
    void *lastRetiredPawn=nullptr,*lastRetiredMovement=nullptr,*lastRetirePreviousTable=nullptr;
    unsigned inNativeCaptures=0,manualCaptureScopes=0,savedMoveDtorScopes=0;
    uint32_t lastTimeIndex=0,lastGuid=0;
    const char* stage="character_stream_not_initialized";
    void *world=nullptr,*driver=nullptr,*connection=nullptr,*controller=nullptr,*rpc=nullptr;
};
inline State state;
inline const State& GetState(){return state;}
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Unsupported(unsigned error,const char* message){
    if(state.unsupported)return false; // Preserve the first invalidated contract.
    state.error=error;state.unsupported=true;state.stage=message;return false;
}
inline bool Identity(void* p,WeakIdentity& out){
    out.index=Read<int32_t>(p,0xc);if(!p||out.index<0)return false;
    out.serial=engine::function<int32_t(*)(void*,int32_t)>(0x1cf35b0)
        (reinterpret_cast<void*>(engine::image+0xd7b87c0),out.index);
    return out.serial>0&&engine::function<void*(*)(const WeakIdentity*)>(0x1d4cfa0)(&out)==p;
}
inline bool Same(void* p,const WeakIdentity& id){return p&&id.index>=0&&id.serial>0
    &&engine::function<void*(*)(const WeakIdentity*)>(0x1d4cfa0)(&id)==p;}
inline WeakIdentity worldId{},driverId{},connectionId{};
inline void* WorldOf(void* actor){
    // Verified in BOTH original ShooterCharacter and ShooterPlayerController
    // tables: v1B8 is Actor.GetWorld3ED83B0, which resolves Level->World+C0.
    //3EDAB80 is Actor.Tick(float), not a getter; never call it here.
    auto target=Read<void*>(Read<void**>(actor),0x1b8);
    if(target!=reinterpret_cast<void*>(engine::image+0x3ed83b0))return nullptr;
    return reinterpret_cast<void*(*)(void*)>(target)(actor);
}
inline bool HostContext(){
    return original_players::Thread()&&Same(state.world,worldId)&&Same(state.driver,driverId)
        &&Read<void*>(state.world,0x40)==state.driver&&Read<void*>(state.driver,0x178)==state.world
        &&Read<void*>(state.driver,0xc0)==nullptr;
}

struct CaptureObserver;
struct Binding {
    void *pawn=nullptr,*movement=nullptr,*stream=nullptr;WeakIdentity pawnId{},movementId{};
    void** privateTable=nullptr;
    CaptureObserver* observer=nullptr;
    std::array<MoveRecord,MaxMoves> moves{};size_t moveCount=0;
    uint32_t lastCapturedIndex=0,pendingIndex=0;
    bool manualPending=false,savedMoveConstructed=false,savedMoveDtorAttempted=false,savedMoveDtorCompleted=false;
    bool manualAbortAttempted=false,manualAbortedUnverified=false;
    uint8_t savedMoveAlignmentPadding[2]{};
    alignas(16) uint8_t savedMove[0x480]{};
    uint32_t lastWireIndex=0;
};
static_assert(alignof(Binding)==16&&offsetof(Binding,savedMove)==0x950&&sizeof(Binding)==0xde0);
inline std::array<Binding,MaxPawns> bindings{};
// An observer and its executable forwarding/table allocation are never freed
// or reused after publication. A retired callback therefore retains its OWN
// closed token, even if a Binding slot and UObject address have been reused.
struct CaptureObserver {
    Binding* binding=nullptr;
    void *pawn=nullptr,*movement=nullptr,*backing=nullptr;
    WeakIdentity pawnId{},movementId{};
    void **privateTable=nullptr,**originalTable=nullptr;
    uint64_t generation=0;
    volatile LONG retired=1,inNativeCapture=0;
    bool published=false;
    CaptureObserver* next=nullptr;
};
inline CaptureObserver* observers=nullptr;
inline std::array<uint64_t,MaxPawns> bindingUses{};
inline bool SameIdentity(const WeakIdentity& a,const WeakIdentity& b){return a.index==b.index&&a.serial==b.serial&&a.index>=0&&a.serial>0;}
inline Binding* Existing(void* movement){for(auto& e:bindings)if(e.movement==movement)return &e;return nullptr;}
inline bool ValidBinding(const Binding& e){
    return e.observer&&InterlockedCompareExchange(&e.observer->retired,0,0)==0
        &&e.observer->binding==&e&&e.observer->pawn==e.pawn&&e.observer->movement==e.movement
        &&SameIdentity(e.observer->pawnId,e.pawnId)&&SameIdentity(e.observer->movementId,e.movementId)
        &&e.observer->privateTable==e.privateTable&&Same(e.pawn,e.pawnId)&&Same(e.movement,e.movementId)
        &&Read<uint8_t>(e.pawn,0x224)==3&&WorldOf(e.pawn)==state.world
        &&Read<void*>(e.pawn,0x4f8)==e.movement&&Read<void*>(e.movement,0xc8)==e.pawn
        &&Read<void*>(e.movement,0x190)==e.pawn&&Read<void*>(e.movement,0x1a8)==e.pawn
        &&Read<void*>(e.pawn,0xba8)==e.stream&&Read<void*>(e.stream,0xc8)==e.pawn
        &&Read<void*>(e.stream,0xe8)==e.pawn&&Read<void**>(e.movement)==e.privateTable;
}
inline bool ValidRecord(const MoveRecord& m){
    const double n=m.normalizedAcceleration.x*m.normalizedAcceleration.x
        +m.normalizedAcceleration.y*m.normalizedAcceleration.y+m.normalizedAcceleration.z*m.normalizedAcceleration.z;
    return m.full==1&&m.timeIndex&&original_players::Finite(m.normalizedAcceleration)
        &&n<=1.001&&original_players::Finite(m.location)&&original_players::Finite(m.velocity)
        &&std::isfinite(m.crouch)&&m.crouch>=0&&m.crouch<=1
        &&m.firstAfterReset<=1&&m.authTeleport<=1;
}
inline bool CaptureRecord(Binding& e,const void* savedMove){
    if(!HostContext()||!ValidBinding(e))return Unsupported(3601,"character_stream_authority_capture_identity_changed");
    // Both the normal native Role3 stack record and the explicit constructor
    // use this exact Shooter FSavedMove table. In particular789CA00 calls v40
    // to read compressed flags; a generic FSavedMove has different behavior.
    if(Read<void*>(savedMove)!=reinterpret_cast<void*>(engine::image+0xb600b78)
       ||Read<void*>(Read<void**>(savedMove),0x40)!=reinterpret_cast<void*>(engine::image+0x7891270))
        return Unsupported(3645,"character_stream_unrecognized_saved_move_capture_interface");
    //789CA00 uses the native stream-registration bit. Never forge that bit or
    // reset another producer's pending queue to make the conversion succeed.
    if(!Read<uint8_t>(e.stream,0xe0)||!Read<uint8_t>(e.movement,0x1080))
        return Unsupported(3602,"character_stream_native_component_capture_not_enabled");
    if(Read<int32_t>(e.movement,0x1410)!=0)
        return Unsupported(3603,"character_stream_native_outgoing_queue_not_empty");
    const uint32_t timeIndex=Read<uint32_t>(savedMove,0x1c);
    if(!timeIndex||(e.lastCapturedIndex&&static_cast<int32_t>(timeIndex-e.lastCapturedIndex)<=0))
        return Unsupported(3608,"character_stream_capture_time_index_did_not_advance");
    const auto domain=local_server_npc_time_domain::Owned(e.pawn,e.movement);
    if(!domain){
        auto pc=Read<void*>(e.pawn,0x4a0),connection=Read<void*>(pc,0x510);
        if(!original_players::IsA(pc,original_players::ShooterControllerClass)||Read<void*>(pc,0x4b0)!=e.pawn
           ||!connection||Read<void*>(connection,0x38)!=pc||Read<void*>(connection,0x60)!=state.driver)
            return Unsupported(3656,"character_stream_npc_has_no_registered_native_common_time_domain");
    }else if(!local_server_npc_time_domain::CapturePhysical(e.pawn,e.movement,timeIndex))
        return Unsupported(local_server_npc_time_domain::GetState().error,local_server_npc_time_domain::GetState().stage);
    // Only an independent native deep copy is mapped; actual physics/raw state
    // and the original FSavedMove remain untouched. Original force/equipment
    // timestamps already use this same actual clock's v30 conversion.
    alignas(16) uint8_t copied[0x480]{};bool built=false,carry=false;
    uint32_t expectedWire=timeIndex;
    __try {
        const void* convertSource=savedMove;
        if(domain){
            engine::function<void*(*)(void*)>(0x7890fa0)(copied);built=true;
            engine::function<void*(*)(void*,const void*)>(0x78946d0)(copied,savedMove);
            if(!local_server_npc_time_domain::MapCopiedMove(e.pawn,e.movement,savedMove,copied,carry,expectedWire))
                return Unsupported(local_server_npc_time_domain::GetState().error,local_server_npc_time_domain::GetState().stage);
            convertSource=copied;
        }
        engine::function<void(*)(void*,const void*)>(0x789ca00)(e.movement,convertSource);++state.nativeConverterCalls;
        const auto array=Read<Array>(e.movement,0x1408);
        if(!array.data||array.count!=1||array.capacity<1||array.capacity>4096)
            return Unsupported(3604,"character_stream_native_converter_did_not_produce_one_record");
        MoveRecord record{};
        if(!original_players::Read(array.data,0,record))return Unsupported(3605,"character_stream_native_record_unreadable");
        record.full=1;record.location=Read<engine::Vec>(savedMove,0x250);
        record.authTeleport=Read<uint8_t>(convertSource,0x468);
        engine::function<void(*)(void*)>(0x78938b0)(static_cast<uint8_t*>(e.movement)+0x1078);++state.nativeQueueResets;
        if(Read<int32_t>(e.movement,0x1410)!=0)return Unsupported(3606,"character_stream_native_queue_reset_failed");
        if(record.timeIndex!=expectedWire||!ValidRecord(record)||Read<uint32_t>(savedMove,0x1c)!=timeIndex)
            return Unsupported(3607,"character_stream_native_capture_record_invalid");
        if(e.lastWireIndex&&static_cast<int32_t>(record.timeIndex-e.lastWireIndex)<=0)
            return Unsupported(3657,"character_stream_public_time_index_did_not_advance");
        const bool own=state.controller&&Read<void*>(state.controller,0x4b0)==e.pawn;
        if(!own&&e.moveCount==MaxMoves)return Unsupported(3609,"character_stream_authority_history_overflow");
        if(domain&&!local_server_npc_time_domain::CommitConverted(e.pawn,e.movement,timeIndex,expectedWire,carry))
            return Unsupported(local_server_npc_time_domain::GetState().error,local_server_npc_time_domain::GetState().stage);
        e.lastCapturedIndex=timeIndex;e.lastWireIndex=record.timeIndex;++state.capturedMoves;state.lastTimeIndex=record.timeIndex;
        if(!own)e.moves[e.moveCount++]=record;return true;
    }__finally {
        if(built)engine::function<void*(*)(void*,uint32_t)>(0x7891010)(copied,0);
    }
}
__declspec(noinline) inline void CaptureAfterNativeFrame(void* movement,const void* savedMove,CaptureObserver* observer){
    if(!observer||!original_players::Thread())return;
    ++state.captureCallbacks;
    if(InterlockedCompareExchange(&observer->retired,0,0)){++state.lateRetiredCallbacks;return;}
    if(InterlockedCompareExchange(&observer->inNativeCapture,1,0)!=0){
        Unsupported(3648,"character_stream_reentrant_native_capture_observer");return;
    }
    ++state.inNativeCaptures;
    __try{
    // Recheck after taking the observer lease. Retirement closes the token
    // before releasing its slot; no closed observer can read recycled metadata.
    if(InterlockedCompareExchange(&observer->retired,0,0)){++state.lateRetiredCallbacks;return;}
    auto* entry=observer->binding;
    if(state.unsupported)return;
    if(!entry||entry->observer!=observer||entry->movement!=movement||observer->movement!=movement
       ||entry->pawn!=observer->pawn||!SameIdentity(entry->pawnId,observer->pawnId)
       ||!SameIdentity(entry->movementId,observer->movementId)
       ||!Same(observer->pawn,observer->pawnId)||!Same(movement,observer->movementId)){
        ++state.captureObserverMismatches;Unsupported(3649,"character_stream_capture_observer_epoch_or_weak_identity_mismatch");return;
    }
    // Sole retained call is the real Role3 local movement tick after native
    // PostUpdate7891280. Manual authority frames use the explicit hooks below.
    if(_ReturnAddress()!=reinterpret_cast<void*>(engine::image+0x789526d)){
        Unsupported(3610,"character_stream_unrecognized_native_capture_call_site");return;
    }
    if(entry->manualPending){Unsupported(3611,"character_stream_nested_authority_capture");return;}
    CaptureRecord(*entry,savedMove);
    }__finally{--state.inNativeCaptures;InterlockedExchange(&observer->inNativeCapture,0);}
}
inline Binding* Bind(void* pawn,void* movement){
    if(auto* found=Existing(movement))return ValidBinding(*found)?found:nullptr;
    if(!HostContext()||!original_players::IsA(pawn,original_players::ShooterCharacterClass)
       ||!original_players::IsA(movement,0x6c7d380)||Read<uint8_t>(pawn,0x224)!=3
       ||WorldOf(pawn)!=state.world||Read<void*>(pawn,0x4f8)!=movement
       ||Read<void*>(movement,0xc8)!=pawn||Read<void*>(movement,0x190)!=pawn||Read<void*>(movement,0x1a8)!=pawn){
        Unsupported(3612,"character_stream_requires_actual_authority_shooter_movement");return nullptr;
    }
    auto component=Read<void*>(pawn,0xba8);
    if(!original_players::IsA(component,0x69d5850)||Read<void*>(component,0xc8)!=pawn||Read<void*>(component,0xe8)!=pawn){
        Unsupported(3613,"character_stream_native_component_owner_not_initialized");return nullptr;
    }
    auto expected=reinterpret_cast<void**>(engine::image+MovementTableRva);
    if(Read<void**>(movement)!=expected||Read<void*>(expected,0xc28)!=reinterpret_cast<void*>(engine::image+0x18a3a00)
       ||Read<void*>(expected,0x3f8)!=reinterpret_cast<void*>(engine::image+0x7894ab0)){
        Unsupported(3614,"character_stream_unrecognized_movement_table_or_authority_capture");return nullptr;
    }
    Binding* entry=nullptr;for(auto& e:bindings)if(!e.pawn){entry=&e;break;}
    if(!entry){Unsupported(3615,"character_stream_authority_binding_capacity");return nullptr;}
    entry->pawn=pawn;entry->movement=movement;entry->stream=component;
    if(!Identity(pawn,entry->pawnId)||!Identity(movement,entry->movementId)){
        Unsupported(3616,"character_stream_authority_weak_identity_unavailable");return nullptr;
    }
    auto observer=new(std::nothrow) CaptureObserver;
    if(!observer){Unsupported(3650,"character_stream_capture_observer_allocation_failed");return nullptr;}
    state.retainedBackingRequestedBytes+=sizeof(CaptureObserver);
    observer->binding=entry;observer->pawn=pawn;observer->movement=movement;
    observer->pawnId=entry->pawnId;observer->movementId=entry->movementId;observer->originalTable=expected;
    observer->generation=++state.observerAllocations;observer->next=observers;observers=observer;
    constexpr size_t tableBytes=(MovementTableEntries+1)*sizeof(void*),stubOffset=(tableBytes+15)&~size_t(15),stubBytes=26;
    auto storage=static_cast<void**>(VirtualAlloc(nullptr,stubOffset+stubBytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!storage){Unsupported(3617,"character_stream_capture_table_allocation_failed");return nullptr;}
    observer->backing=storage;state.retainedBackingRequestedBytes+=stubOffset+stubBytes;
    std::memcpy(storage,expected-1,tableBytes);
    // Windows x64 leaf forwarding ABI: preserve native RCX/RDX/RSP and its
    // original return address; supply this immutable life token in volatile R8.
    // ENDBR64; MOV R8,observer; MOV RAX,our helper; JMP RAX. No game text changes.
    uint8_t code[stubBytes]{0xf3,0x0f,0x1e,0xfa,0x49,0xb8,0,0,0,0,0,0,0,0,0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xe0};
    const auto token=reinterpret_cast<uintptr_t>(observer),helper=reinterpret_cast<uintptr_t>(&CaptureAfterNativeFrame);
    std::memcpy(code+6,&token,8);std::memcpy(code+16,&helper,8);
    auto stub=static_cast<uint8_t*>(observer->backing)+stubOffset;std::memcpy(stub,code,sizeof(code));
    auto copied=storage+1;copied[0xc28/8]=stub;observer->privateTable=copied;
    entry->privateTable=copied;entry->observer=observer;
    DWORD old=0;
    if(!VirtualProtect(storage,stubOffset+stubBytes,PAGE_EXECUTE_READ,&old)
       ||!FlushInstructionCache(GetCurrentProcess(),stub,stubBytes)){
        Unsupported(3618,"character_stream_capture_table_install_failed");return nullptr;
    }
    InterlockedExchange(&observer->retired,0);
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(movement),copied,expected)!=expected){
        InterlockedExchange(&observer->retired,1);Unsupported(3618,"character_stream_capture_table_install_failed");return nullptr;
    }
    observer->published=true;
    const size_t slot=static_cast<size_t>(entry-bindings.data());if(bindingUses[slot]++)++state.slotReuses;
    ++state.boundPawns;state.captureHookInstalled=true;return entry;
}

// Called after its scheduler has quiesced, before native actor/component
// destruction. No Role, local-control, possession or alive predicate is used.
// Unknown/unbound weak identities (normally the owning human) are idempotent.
inline bool Retire(void* pawn){
    if(state.manualCaptureScopes||state.savedMoveDtorScopes)return Unsupported(3659,"character_stream_retire_during_original_manual_builder_or_destructor");
    ++state.retireCalls;
    if(!pawn){++state.retireUnbound;return true;}
    if(!state.initialized&&!state.boundPawns){++state.retireUnbound;return true;}
    if(!original_players::Thread())return Unsupported(3651,"character_stream_retire_requires_owned_game_thread");
    WeakIdentity requested{};Identity(pawn,requested);
    Binding* entry=nullptr;
    for(auto& e:bindings)if(e.pawn==pawn&&SameIdentity(e.pawnId,requested)){entry=&e;break;}
    if(!entry){
        for(auto* observer=observers;observer;observer=observer->next)
            if(observer->pawn==pawn&&SameIdentity(observer->pawnId,requested)
               &&InterlockedCompareExchange(&observer->retired,0,0)){
                ++state.retireIdempotent;return true;
            }
        ++state.retireUnbound;return true;
    }
    auto* observer=entry->observer;
    if(!observer||observer->binding!=entry||observer->pawn!=pawn||observer->movement!=entry->movement
       ||!SameIdentity(observer->pawnId,entry->pawnId)||!SameIdentity(observer->movementId,entry->movementId)
       ||observer->privateTable!=entry->privateTable)
        return Unsupported(3652,"character_stream_retire_observer_identity_mismatch");
    if(entry->manualPending||InterlockedCompareExchange(&observer->inNativeCapture,0,0)){
        ++state.retireRejectedInFlight;return Unsupported(3653,"character_stream_retire_capture_still_in_flight");
    }
    if(observer->published&&!state.boundPawns)return Unsupported(3655,"character_stream_retire_active_binding_count_invalid");
    // Close the lifetime token before detaching. Keep it closed if a concurrent
    // callback acquired its lease, and do not recycle until a quiescent retry.
    InterlockedExchange(&observer->retired,1);
    if(InterlockedCompareExchange(&observer->inNativeCapture,0,0)){
        ++state.retireRejectedInFlight;return Unsupported(3653,"character_stream_retire_capture_still_in_flight");
    }
    if(entry->savedMoveConstructed){
        if(entry->savedMoveDtorAttempted&&!entry->savedMoveDtorCompleted)
            return Unsupported(3660,"character_stream_owned_destructor_failed_no_replay");
        if(Read<void*>(entry->savedMove)!=reinterpret_cast<void*>(engine::image+0xb600b78))
            return Unsupported(3654,"character_stream_retire_owned_saved_move_type_changed");
        entry->savedMoveDtorAttempted=true;
        ++state.savedMoveDtorScopes;
        __try{engine::function<void*(*)(void*,uint32_t)>(0x7891010)(entry->savedMove,0);}
        __finally{--state.savedMoveDtorScopes;}
        entry->savedMoveDtorCompleted=true;entry->savedMoveConstructed=false;++state.retireScratchDestructions;
    }
    // A dead original weak movement is never dereferenced or resolved through
    // its old address. In particular this cannot overwrite an ABA replacement.
    if(Same(entry->movement,entry->movementId)){
        auto previous=InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(entry->movement),observer->originalTable,entry->privateTable);
        state.lastRetirePreviousTable=previous;
        if(previous==entry->privateTable)
            ++state.retireVptrRestored;
        else ++state.retireForeignVptrPreserved; // Preserve every foreign slot.
    }else {++state.retireExpiredMovements;state.lastRetirePreviousTable=nullptr;}
    state.lastRetiredGeneration=observer->generation;state.lastRetiredPawnId=observer->pawnId;state.lastRetiredMovementId=observer->movementId;
    state.lastRetiredPawn=observer->pawn;state.lastRetiredMovement=observer->movement;
    state.retiredMovesDiscarded+=entry->moveCount;
    observer->binding=nullptr;
    *entry={};if(observer->published){--state.boundPawns;++state.retiredBindings;}else ++state.retireUnpublished;
    state.captureHookInstalled=state.boundPawns!=0;
    if(!state.unsupported)state.stage="character_stream_capture_life_retired_backing_retained";
    return true;
}

// Optional integration with the separately implemented network-input authority.
// The caller must advance the real native server prediction clock and supply
// its actual normalized input; this module never invents a time index/clock.
inline bool BeginAuthorityStepImpl(void* pawn,void* movement,float delta,uint32_t index,const engine::Vec& normalizedInput){
    if(!state.initialized||state.unsupported)return false;
    auto* e=Bind(pawn,movement);if(!e)return false;
    if(e->manualPending||e->savedMoveConstructed||!index||!std::isfinite(delta)||delta<=0||delta>0.25f||!original_players::Finite(normalizedInput))
        return Unsupported(3619,"character_stream_manual_capture_arguments_unsupported");
    //78913A0 reads this original force-module collection without null guards.
    // Require the original BeginPlay and collection before calling SetMoveFor.
    auto forces=Read<void*>(pawn,0x1100);const auto modules=Read<Array>(forces,0x150);
    if(!Read<uint8_t>(pawn,0xa90)||!forces||modules.count<0||modules.count>1024
       ||modules.capacity<modules.count||(modules.count&&!modules.data))
        return Unsupported(3646,"character_stream_native_capture_force_modules_not_initialized");
    for(int32_t i=0;i<modules.count;++i)if(!Read<void*>(modules.data,size_t(i)*8))
        return Unsupported(3647,"character_stream_native_capture_force_module_missing");
    auto prediction=Read<void*>(movement,0x7f0),vbtable=Read<void*>(prediction,8);
    const int32_t displacement=Read<int32_t>(vbtable,4);
    if(!prediction||displacement<0||displacement>0x1000)return Unsupported(3620,"character_stream_native_server_clock_unavailable");
    auto clockBase=static_cast<uint8_t*>(prediction)+8+displacement;
    if(Read<uint32_t>(clockBase,8)!=index)return Unsupported(3621,"character_stream_native_clock_command_index_mismatch");
    e->savedMoveDtorAttempted=e->savedMoveDtorCompleted=e->manualAbortAttempted=e->manualAbortedUnverified=false;
    engine::function<void*(*)(void*)>(0x7890fa0)(e->savedMove);e->savedMoveConstructed=true;
    engine::function<void(*)(void*,void*,float,const engine::Vec*,void*)>(0x78913a0)
        (e->savedMove,pawn,delta,&normalizedInput,clockBase);
    if(Read<uint32_t>(e->savedMove,0x1c)!=index)return Unsupported(3622,"character_stream_native_set_move_timestamp_mismatch");
    e->manualPending=true;e->pendingIndex=index;return true;
}
inline bool BeginAuthorityStep(void* pawn,void* movement,float delta,uint32_t index,const engine::Vec& normalizedInput){
    if(!original_players::Thread())return false;
    if(state.manualCaptureScopes)return Unsupported(3658,"character_stream_reentrant_original_manual_builder");
    ++state.manualCaptureScopes;
    __try{return BeginAuthorityStepImpl(pawn,movement,delta,index,normalizedInput);}
    __finally{--state.manualCaptureScopes;}
}
inline bool CompleteAuthorityStep(void* pawn,void* movement,uint32_t index){
    auto* e=Existing(movement);
    if(!state.initialized||state.unsupported||!e||e->pawn!=pawn||!e->manualPending||e->pendingIndex!=index)
        return Unsupported(3623,"character_stream_manual_post_physics_capture_unpaired");
    if(e->savedMoveDtorAttempted&&!e->savedMoveDtorCompleted)return Unsupported(3660,"character_stream_owned_destructor_failed_no_replay");
    engine::function<void(*)(void*,void*,uint32_t)>(0x7891280)(e->savedMove,pawn,0);
    const bool ok=CaptureRecord(*e,e->savedMove);
    e->savedMoveDtorAttempted=true;
    ++state.savedMoveDtorScopes;
    __try{engine::function<void*(*)(void*,uint32_t)>(0x7891010)(e->savedMove,0);}
    __finally{--state.savedMoveDtorScopes;}
    e->savedMoveDtorCompleted=true;e->savedMoveConstructed=false;e->manualPending=false;e->pendingIndex=0;return ok;
}

inline bool ActorChannelExists(void* connection,void* actor){
    const auto channels=Read<Array>(connection,0x78);
    if(channels.count<0||channels.count>4096||channels.capacity<channels.count||!channels.data)return false;
    for(int32_t i=0;i<channels.count;++i){
        auto channel=Read<void*>(channels.data,size_t(i)*sizeof(void*));
        if(channel&&Read<void*>(channel,0x30)==connection&&!(Read<uint8_t>(channel,0x38)&2)
           &&original_players::IsA(channel,0x3dd3d40)&&Read<void*>(channel,0x78)==actor)return true;
    }
    return false;
}
inline bool ExistingGuid(void* pawn,const WeakIdentity& identity,uint32_t& value){
    auto cache=Read<void*>(state.driver,0x188);
    if(!cache||!Same(pawn,identity)||!engine::function<bool(*)(void*,void*,const WeakIdentity*)>(0x4782750)(cache,pawn,&identity))return false;
    auto map=static_cast<uint8_t*>(cache)+0x50;
    const int32_t index=engine::function<int32_t(*)(void*,uint32_t,const WeakIdentity*)>(0x1c01cf0)
        (map,uint32_t(identity.index)^uint32_t(identity.serial),&identity);
    const int32_t sparseCount=Read<int32_t>(map,8);
    if(index<0||index>=sparseCount||sparseCount>1000000)return false;
    auto entries=Read<uint8_t*>(map);const uint64_t guid=Read<uint64_t>(entries,size_t(index)*24+8);
    if(guid<2||guid>UINT32_MAX)return false;
    uint64_t resolveGuid=guid;
    if(engine::function<void*(*)(void*,const uint64_t*,bool)>(0x4785360)(cache,&resolveGuid,false)!=pawn)return false;
    value=static_cast<uint32_t>(guid);return true;
}

struct alignas(16) BitWriter {
    uint8_t bytes[0xb8]{};bool constructed=false;uint8_t storageAlignmentPadding[7]{};
    BitWriter(){engine::function<void*(*)(void*,int64_t,bool)>(0x1a5e2d0)(bytes,4096,true);constructed=true;}
    ~BitWriter(){if(constructed)engine::function<void*(*)(void*,uint32_t)>(0x1a5e3d0)(bytes,0);}
    void Bits(const void* value,int64_t count){engine::function<void(*)(void*,const void*,int64_t)>(0x1a5e630)(bytes,value,count);}
    int64_t Count()const{return Read<int64_t>(bytes,0xa0);}
    bool Error()const{return (Read<uint8_t>(bytes,0x29)&4)!=0;}
    uint8_t* Data()const{return Read<uint8_t*>(bytes,0x90);}
};
static_assert(alignof(BitWriter)==16&&offsetof(BitWriter,bytes)==0
    &&offsetof(BitWriter,constructed)==0xb8&&sizeof(BitWriter)==0xc0);
inline bool WriterByteAt(const BitWriter& writer,int64_t position,uint8_t expected){
    if(position<0||writer.Error()||!writer.Data()||position+8>writer.Count())return false;
    uint8_t value=0;
    for(unsigned bit=0;bit<8;++bit){const int64_t at=position+bit;
        value=static_cast<uint8_t>(value|(((writer.Data()[at>>3]>>(at&7))&1u)<<bit));}
    return value==expected;
}
inline bool BuildStream(Binding& entry,std::vector<uint8_t>& buffer,uint8_t& unusedBits){
    if(!ValidBinding(entry)||entry.moveCount==0||entry.moveCount>MaxMoves)return false;
    auto input=Read<void*>(entry.pawn,0xba0),ps=Read<void*>(entry.pawn,0x488),pdd=Read<void*>(ps,0x8d0);
    if(!original_players::IsA(input,0x6494370)||Read<void*>(input,0xc8)!=entry.pawn
       ||!original_players::IsA(ps,0x6cc4040)||!original_players::IsA(pdd,0x6a60cc0)
       ||Read<void*>(pdd,0xc8)!=ps)return Unsupported(3624,"character_stream_input_or_pdd_owner_mismatch");
    auto inputInterface=static_cast<uint8_t*>(input)+0xd8,pddInterface=static_cast<uint8_t*>(pdd)+0xd8;
    if(Read<void*>(Read<void**>(inputInterface),0x10)!=reinterpret_cast<void*>(engine::image+0x70a8c30)
       ||Read<void*>(Read<void**>(pddInterface),0x10)!=reinterpret_cast<void*>(engine::image+0x6f4f3a0))
        return Unsupported(3625,"character_stream_original_component_serializer_mismatch");
    BitWriter writer;const uint8_t header=0x52;writer.Bits(&header,8);
    uint8_t sequence=1;
    for(size_t i=0;i<entry.moveCount;++i){
        if(!ValidRecord(entry.moves[i]))return Unsupported(3626,"character_stream_queued_snapshot_invalid");
        writer.Bits(&sequence,3);
        if(engine::function<uint8_t(*)(void*,void*,MoveRecord*)>(0x7893760)(entry.movement,writer.bytes,&entry.moves[i])==2)
            return Unsupported(3627,"character_stream_native_move_serialization_error");
        sequence=static_cast<uint8_t>((std::max)(1,(int(sequence)+1)&7));
    }
    const uint8_t terminator=0;writer.Bits(&terminator,3);
    const int64_t boundaryPosition=writer.Count();uint16_t boundary=0;writer.Bits(&boundary,16);
    const int64_t inputStart=writer.Count();
    if(engine::function<uint8_t(*)(void*,void*)>(0x70a8c30)(inputInterface,writer.bytes)==2)
        return Unsupported(3628,"character_stream_native_input_serialization_error");
    if(!WriterByteAt(writer,inputStart,0xa3))
        return Unsupported(3648,"character_stream_native_input_writer_omitted_expected_header");
    if(writer.Error()||!writer.Data()||writer.Count()<0||writer.Count()>=0xffff)
        return Unsupported(3629,"character_stream_input_boundary_out_of_range");
    boundary=static_cast<uint16_t>(writer.Count());
    // Same unaligned bit patch as71717E0; patch only this temporary writer.
    for(unsigned bit=0;bit<16;++bit){const int64_t at=boundaryPosition+bit;
        const uint8_t mask=static_cast<uint8_t>(1u<<(at&7));auto& byte=writer.Data()[at>>3];
        byte=static_cast<uint8_t>((byte&~mask)|(((boundary>>bit)&1)?mask:0));}
    const int64_t pddStart=writer.Count();
    if(engine::function<uint8_t(*)(void*,void*)>(0x6f4f3a0)(pddInterface,writer.bytes)==2)
        return Unsupported(3630,"character_stream_native_pdd_serialization_error");
    if(!WriterByteAt(writer,pddStart,0xa4))
        return Unsupported(3649,"character_stream_native_pdd_writer_omitted_expected_header");
    const int64_t bits=writer.Count(),bytes=(bits+7)/8;
    if(writer.Error()||bits<=0||bytes<=0||bytes>0x177a||!writer.Data())
        return Unsupported(3631,"character_stream_native_writer_error_or_size_limit");
    buffer.assign(writer.Data(),writer.Data()+bytes);unusedBits=static_cast<uint8_t>((8-(bits&7))&7);return true;
}

// RequiredRvas/profiles are supplied by the retained-byte manifest generated
// alongside this header. The outer bridge must validate that manifest first;
// these exact entry bytes also reject an accidental different image locally.
inline bool EntriesMatch();
inline bool Initialize(void* world,void* driver){
    if(state.initialized)return state.world==world&&state.driver==driver&&!state.unsupported;
    if(!original_players::Thread())return Unsupported(3632,"character_stream_initialize_wrong_thread");
    if(!EntriesMatch())return Unsupported(3633,"character_stream_source_profile_mismatch");
    state.profilesVerified=true;state.world=world;state.driver=driver;
    if(!Identity(world,worldId)||!Identity(driver,driverId)||!HostContext())return Unsupported(3634,"character_stream_requires_current_native_server_world");
    state.rpc=engine::Find(ReceiveFunction);
    // Runtime UFunction parameter bytes were already used by this project's
    // native travel probe; generated pinned params are exactly one const TArray.
    state.rpcLayoutVerified=state.rpc&&Read<uint16_t>(state.rpc,0xbe)==16
        &&Read<uint32_t>(state.rpc,0xb8)==0x01080c40
        &&Read<void*>(state.rpc,0xe0)==reinterpret_cast<void*>(engine::image+0x64e98d0);
    if(!state.rpcLayoutVerified)return Unsupported(3635,"character_stream_original_rpc_parameter_layout_mismatch");
    state.initialized=true;state.stage="character_stream_waiting_authority_capture";return true;
}
inline bool Send(void* connection,void* controller,void* const* pawns,size_t count){
    state.lastRPCSubmitted=false;state.lastUpdateCount=state.lastMoveCount=state.lastBytes=0;
    if(!state.initialized||state.unsupported||!HostContext())return false;
    const auto clients=Read<Array>(state.driver,0xc8);
    // This candidate handles one owned localhost recipient. Additional peers
    // require independent capture cursors rather than silently losing updates.
    if(clients.count!=1||clients.capacity<1||Read<void*>(clients.data)!=connection)
        return Unsupported(3636,"character_stream_multiple_or_unregistered_recipients_unsupported");
    if(!original_players::IsA(controller,original_players::ShooterControllerClass)
       ||WorldOf(controller)!=state.world||Read<uint8_t>(controller,0x224)!=3
       ||Read<void*>(connection,0x60)!=state.driver||Read<uint8_t>(connection,0x13c)!=3
       ||Read<uint8_t>(connection,0x1d8)!=3||Read<void*>(connection,0x38)!=controller
       ||Read<void*>(connection,0xa0)!=controller||Read<void*>(controller,0x510)!=connection
       ||!Read<void*>(connection,0x70)||!ActorChannelExists(connection,controller))
        return Unsupported(3637,"character_stream_admitted_connection_owner_or_channel_mismatch");
    const int mode=engine::function<int(*)(void*)>(0x3efc820)(controller);
    if(mode!=1&&mode!=2)return Unsupported(3638,"character_stream_recipient_not_in_native_server_netmode");
    if(state.connection&&(state.connection!=connection||!Same(connection,connectionId)))
        return Unsupported(3639,"character_stream_recipient_replacement_requires_new_session");
    if(!state.connection&&!Identity(connection,connectionId))return Unsupported(3640,"character_stream_connection_weak_identity_unavailable");
    state.connection=connection;state.controller=controller;
    if(count>MaxPawns||(count&&!pawns))return Unsupported(3641,"character_stream_pawn_list_invalid");
    std::array<RemoteCharacterUpdate,MaxPawns> updates{};
    std::array<std::vector<uint8_t>,MaxPawns> buffers{};
    std::array<Binding*,MaxPawns> submitted{};size_t used=0;unsigned moves=0,bytes=0;
    for(size_t i=0;i<count;++i){
        void* pawn=pawns[i];if(!pawn)return Unsupported(3642,"character_stream_null_pawn_in_roster");
        for(size_t j=0;j<i;++j)if(pawns[j]==pawn)return Unsupported(3643,"character_stream_duplicate_authority_pawn");
        if(pawn==Read<void*>(controller,0x4b0)){++state.ownPawnSkips;
            if(auto* e=Existing(Read<void*>(pawn,0x4f8)))e->moveCount=0;continue;}
        auto* e=Bind(pawn,Read<void*>(pawn,0x4f8));if(!e)return false;
        if(!ActorChannelExists(connection,pawn)){++state.channelDeferrals;state.stage="character_stream_waiting_original_actor_channel";continue;}
        uint32_t guid=0;if(!ExistingGuid(pawn,e->pawnId,guid)){
            ++state.channelDeferrals;state.stage="character_stream_waiting_existing_native_actor_guid";continue;}
        if(!e->moveCount)continue;
        uint8_t unused=0;if(!BuildStream(*e,buffers[used],unused))return false;
        auto& update=updates[used];update.guid=guid;
        update.stream.data=buffers[used].data();update.stream.count=update.stream.capacity=static_cast<int32_t>(buffers[used].size());
        update.stream.unusedBits=unused;submitted[used]=e;moves+=static_cast<unsigned>(e->moveCount);
        bytes+=static_cast<unsigned>(buffers[used].size());state.lastGuid=guid;++used;
    }
    if(!used){state.stage="character_stream_waiting_authority_capture_or_actor_export";return true;}
    Array params{updates.data(),static_cast<int32_t>(used),static_cast<int32_t>(used)};
    if(!engine::Event(controller,ReceiveFunction,&params))return Unsupported(3644,"character_stream_original_client_rpc_dispatch_unavailable");
    for(size_t i=0;i<used;++i){auto* entry=submitted[i];const auto domain=local_server_npc_time_domain::Owned(entry->pawn,entry->movement);
        if(domain){bool containsCarry=false;
            for(size_t n=0;n<entry->moveCount;++n)if(entry->moves[n].timeIndex==domain->carryWire&&entry->moves[n].authTeleport)containsCarry=true;
            if(!entry->moveCount||!local_server_npc_time_domain::Submitted(entry->pawn,entry->movement,
                entry->moves[0].timeIndex,entry->moves[entry->moveCount-1].timeIndex,containsCarry))
                return Unsupported(local_server_npc_time_domain::GetState().error,local_server_npc_time_domain::GetState().stage);
        }
        entry->moveCount=0;
    }
    ++state.rpcSubmissions;state.submittedMoves+=moves;state.lastRPCSubmitted=true;
    state.lastUpdateCount=static_cast<unsigned>(used);state.lastMoveCount=moves;state.lastBytes=bytes;
    state.stage="character_stream_original_rpc_submitted_ingestion_unverified";return true;
}
// BEGIN GENERATED CHARACTER STREAM PROFILES
inline bool EntriesMatch(){
    struct Profile { uintptr_t rva;size_t bytes;uint64_t fnv;size_t relocBegin,relocCount; };
    static constexpr uint16_t relocations[]{0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,200,208,216,224,232,240,248,256,264,272,280,288,296,304,312,320,328,336,344,352,360,368,376,384,392,400,408,416,424,432,440,448,456,464,472,480,488,496,504,512,520,528,536,544,552,560,568,576,584,592,600,608,616,624,632,640,648,656,664,672,680,688,696,704,712,720,728,736,744,752,760,768,776,784,792,800,808,816,824,832,840,848,856,864,872,880,888,896,904,912,920,928,936,944,952,960,968,976,984,992,1000,1008,1016,1024,1032,1040,1048,1056,1064,1072,1080,1088,1096,1104,1112,1120,1128,1136,1144,1152,1160,1168,1176,1184,1192,1200,1208,1216,1224,1232,1240,1248,1256,1264,1272,1280,1288,1296,1304,1312,1320,1328,1336,1344,1352,1360,1368,1376,1384,1392,1400,1408,1416,1424,1432,1440,1448,1456,1464,1472,1480,1488,1496,1504,1512,1520,1528,1536,1544,1552,1560,1568,1576,1584,1592,1600,1608,1616,1624,1632,1640,1648,1656,1664,1672,1680,1688,1696,1704,1712,1720,1728,1736,1744,1752,1760,1768,1776,1784,1792,1800,1808,1816,1824,1832,1840,1848,1856,1864,1872,1880,1888,1896,1904,1912,1920,1928,1936,1944,1952,1960,1968,1976,1984,1992,2000,2008,2016,2024,2032,2040,2048,2056,2064,2072,2080,2088,2096,2104,2112,2120,2128,2136,2144,2152,2160,2168,2176,2184,2192,2200,2208,2216,2224,2232,2240,2248,2256,2264,2272,2280,2288,2296,2304,2312,2320,2328,2336,2344,2352,2360,2368,2376,2384,2392,2400,2408,2416,2424,2432,2440,2448,2456,2464,2472,2480,2488,2496,2504,2512,2520,2528,2536,2544,2552,2560,2568,2576,2584,2592,2600,2608,2616,2624,2632,2640,2648,2656,2664,2672,2680,2688,2696,2704,2712,2720,2728,2736,2744,2752,2760,2768,2776,2784,2792,2800,2808,2816,2824,2832,2840,2848,2856,2864,2872,2880,2888,2896,2904,2912,2920,2928,2936,2944,2952,2960,2968,2976,2984,2992,3000,3008,3016,3024,3032,3040,3048,3056,3064,3072,3080,3088,3096,3104,3112,3120,3128,3136,3144,3152,3160,3168,3176,3184,3192,3200,0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,200,208,216,224,232,240,248,256,264,272,280,288,296,304,312,320,328,336,344,352,360,368,376,384,392,400,408,416,0,8,16,24,32,40,48,56,64,0,16,40};
    static constexpr Profile profiles[]{
        {0x18a3a00,3,0x69bc481c23de2d4dULL,0,0},
        {0x1a5e2d0,251,0x6c07ad2acb2ab0fcULL,0,0},
        {0x1a5e3d0,70,0xb8848ff2c9064389ULL,0,0},
        {0x1a5e630,235,0x979b53cebacac86aULL,0,0},
        {0x1a5e8b0,308,0x29420cdd5237000ULL,0,0},
        {0x1a5ecb0,458,0xacc4f5d0caede35eULL,0,0},
        {0x1c01cf0,165,0xaedeca75a5887cdbULL,0,0},
        {0x1cf35b0,1571,0x40614eae57888396ULL,0,0},
        {0x1d07dd0,1669,0x3adabc6786ee6b60ULL,0,0},
        {0x1d4cfa0,1562,0xb050d7efb4cf1a25ULL,0,0},
        {0x3dd3d40,174,0x979298a1c726633cULL,0,0},
        {0x3e565c0,304,0x77d8efa0e80d5008ULL,0,0},
        {0x3e5c9b0,395,0x47ce0684727f13e0ULL,0,0},
        {0x3e5cb40,395,0x4a05262cc6343969ULL,0,0},
        {0x3e5d7e0,110,0x9dbd49b9eab5fd70ULL,0,0},
        {0x3ed83b0,1609,0x15d2d4ecc2f49cf7ULL,0,0},
        {0x3efc820,176,0x7f1113b3f79164b2ULL,0,0},
        {0x41e3260,20,0xc6726e8b916083e1ULL,0,0},
        {0x41e3280,89,0xae10b05284a590ecULL,0,0},
        {0x4782750,368,0x5496f5f93ae5d4deULL,0,0},
        {0x4785360,4159,0x9bc2e864781cfb07ULL,0,0},
        {0x6494370,174,0x91d2dca31a723cdeULL,0,0},
        {0x64e98d0,381,0x37858a4bc9b11bb7ULL,0,0},
        {0x6943db0,174,0x2f06235cee967932ULL,0,0},
        {0x69d5850,174,0x6e3655e148524e00ULL,0,0},
        {0x6a60cc0,174,0x6787a95ab7e529e2ULL,0,0},
        {0x6c5c480,174,0xb7d7706c24b54716ULL,0,0},
        {0x6c7d380,174,0xaeec39550aff18afULL,0,0},
        {0x6c7d430,124,0xe6e17077ed6929d6ULL,0,0},
        {0x6cc4040,174,0x22d64e6011d4b830ULL,0,0},
        {0x6f2fcd0,55,0x4bc198bdc6d74844ULL,0,0},
        {0x6f30950,142,0xc765862d5b81223aULL,0,0},
        {0x6f4f3a0,263,0x14907b42d37f12bcULL,0,0},
        {0x70a8c30,479,0xc3f93c75c812503aULL,0,0},
        {0x7170f50,597,0xb7d5ee61c43498cdULL,0,0},
        {0x7171c00,717,0xf1a28df919e14cddULL,0,0},
        {0x7822b70,66,0x4148b6626d1d06dULL,0,0},
        {0x7866400,388,0x406b4c6c25a0d421ULL,0,0},
        {0x78665f0,2194,0x29d783cb7c5d7bbULL,0,0},
        {0x788bdd0,68,0xfb6a4dce7d622198ULL,0,0},
        {0x788be20,348,0xc5fdb70cff6a1f3fULL,0,0},
        {0x7890fa0,98,0x9b94cb0594ed7fa4ULL,0,0},
        {0x7891010,48,0xe8abc591b29ab71eULL,0,0},
        {0x7891270,8,0xb3fbf96b10b6203cULL,0,0},
        {0x7891280,238,0x862d6647c415794ULL,0,0},
        {0x78913a0,318,0x453ad149ae134966ULL,0,0},
        {0x78914e0,153,0x97c751b244c20378ULL,0,0},
        {0x78924b0,239,0x5f2be1d81dbcfefeULL,0,0},
        {0x7893150,771,0xfab44c3408d48ec4ULL,0,0},
        {0x7893550,527,0xdb6ee46a5eba91b3ULL,0,0},
        {0x7893760,334,0xf9b43a29c6ebd304ULL,0,0},
        {0x78938b0,25,0x5837beb471e2e6d0ULL,0,0},
        {0x7893c40,970,0xc6ed6b56e03db669ULL,0,0},
        {0x7894010,1714,0x57c150b56d3eecfbULL,0,0},
        {0x7894ab0,2588,0x31679692b8536d50ULL,0,0},
        {0x789ca00,728,0x27d92b1565b3b2ddULL,0,0},
        {0x789df70,144,0x59af83ea9f307d53ULL,0,0},
        {0x789f0e0,407,0xf2022bfb32d61cacULL,0,0},
        {0xb4ecc48,3208,0xb0f83e70aed03f58ULL,0,401},
        {0xa8571d0,424,0x2062103fc3eafd7cULL,401,53},
        {0xb600b78,72,0xb22f3fcb408f517eULL,454,9},
        {0xd528d40,64,0xc2cb783e0d65104fULL,463,3},
    };
    if(!engine::image)return false;
    for(const auto& p:profiles){
        const auto* bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes))return false;
        std::vector<uint8_t> normalized;
        if(p.relocCount){
            normalized.assign(bytes,bytes+p.bytes);
            for(size_t n=0;n<p.relocCount;++n){
                const size_t offset=relocations[p.relocBegin+n];uintptr_t value=0;
                std::memcpy(&value,normalized.data()+offset,sizeof(value));
                value=value-engine::image+0x140000000ULL;
                std::memcpy(normalized.data()+offset,&value,sizeof(value));
            }
            bytes=normalized.data();
        }
        uint64_t hash=14695981039346656037ULL;
        for(size_t i=0;i<p.bytes;++i)hash=(hash^bytes[i])*1099511628211ULL;
        if(hash!=p.fnv)return false;
    }
    return true;
}
// END GENERATED CHARACTER STREAM PROFILES

}
