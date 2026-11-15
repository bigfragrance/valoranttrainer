#pragma once
#include "local_server_character_stream.h"
#include "training_bot_controller.h"
#include <algorithm>
#include <array>
#include <cmath>

// Missing authority scheduler for this pinned client artifact only. It never
// changes Role/RemoteRole, IsLocalController, UPlayer, client poses, or actor
// exports. Original BeforeFrame, saved-move input, clock, physics and stream
// capture remain the producers. See work/local-server-playable/npc-input/.
namespace local_server_npc_input {
using Vec=engine::Vec;
using Frame=training_bot_controller::Frame;
using Weak=local_server_character_stream::WeakIdentity;
inline constexpr size_t MaxPlayers=original_players::MaxPlayers;
inline constexpr float Step=1.0f/128.0f,MaxDelta=.125f,MaxBudget=.125f;
inline constexpr uint32_t MaxSteps=16;
static_assert(MaxBudget==Step*static_cast<float>(MaxSteps)&&MaxSteps<=local_server_character_stream::MaxMoves);
inline constexpr double IntentHoldSeconds=.25;
enum class Owner:uint8_t {Unknown,Adapter,Native};
struct Hooks {bool (*weaponInput)(size_t,bool,bool)=nullptr;};
inline constexpr size_t FunctionCount=4;
struct FunctionCheck {
    const char* name="unchecked";
    bool checked=false,found=false,bytesMatch=false,flagsMatch=false,execMatch=false,ownerMatch=false;
    uint16_t expectedBytes=0,actualBytes=0,storageBytes=0;
    uint32_t expectedFlags=0,actualFlags=0;
    uintptr_t expectedExec=0,actualExec=0,ownerGetterRva=0;
    void *function=nullptr,*expectedOwner=nullptr,*actualOwner=nullptr;
};
// Read-only phase comparison. Offsets are pinned by retained Engine movement
// metadata in work/duel1-original-simulation/base-movement-properties.json and
// work/duel1-original-movement/properties.json. No setters or physics calls.
struct MovementObservation {
    uint64_t worldFrame=0;uint32_t physicalIndex=0;uint8_t stages=0;
    bool settingsKnown=false,primitiveIsRoot=false,capsuleIsRoot=false;
    uint8_t planeFlags=0,planeAxis=255,movementMode=255,customMode=255,groundMode=255;
    Vec planeNormal{},planeOrigin{};
    Vec beforeWorld{},afterWorld{},afterCapture{},afterScope{},afterDelegate{};
};
struct Entry {
    bool present=false,alive=false,crouchMetadata=false,snapshot=false,hasIntent=false;
    bool lastJump=false,ready=false,nativeLocal=false;
    Owner owner=Owner::Unknown;
    uint32_t epoch=0,error=0,index=0,baseOffset=0,frameSteps=0,playerIndex=0;
    uint32_t commonWireIndex=0,commonTargetIndex=0,commonUnservedSteps=0;
    double commonBudgetSuppliedSeconds=0;
    uint32_t rawBefore=0,stepsBefore=0,totalMovesBefore=0,totalMovesAfter=0,captureBefore=0;
    uint64_t snapshotFrame=0,submissions=0,manualFrames=0,neutralFrames=0;
    uint64_t nativeObservations=0,nativeAdvances=0,nativeCaptureObservations=0,clockAdvances=0,beforeFrames=0;
    uint64_t physicsCalls=0,physicsCompleted=0,captures=0,frameScopes=0,delegates=0;
    uint64_t walkCalls=0,crouchWrites=0,crouchCalls=0,jumpCalls=0,faceCalls=0,inputCalls=0;
    uint64_t rootPhysicsQueries=0;
    uint64_t captureGeneration=0;
    bool rootPhysicsClassValid=false,rootPhysicsOwnerMatches=false,rootPhysicsTargetMatched=false,rootPhysicsDispatched=false;
    uint8_t rootPhysicsResult=255;
    void *rootPhysicsClass=nullptr,*rootPhysicsTable=nullptr,*rootPhysicsProcessEvent=nullptr,*rootPhysicsNativePredicate=nullptr;
    uintptr_t rootPhysicsProcessEventRva=0,rootPhysicsNativePredicateRva=0;
    float accumulatedBefore=0,budget=0,droppedSeconds=0;
    double submittedAt=0;
    void *pc=nullptr,*ps=nullptr,*pawn=nullptr,*movement=nullptr,*root=nullptr;
    void *input=nullptr,*inventory=nullptr,*stream=nullptr,*prediction=nullptr,*clock=nullptr,*timeShift=nullptr;
    Weak pcId{},psId{},pawnId{},movementId{},rootId{},inputId{},inventoryId{},streamId{};
    Frame latest{};
    Vec consumedInput{},positionBefore{},positionAfter{},velocityAfter{};
    MovementObservation movementObservation{};
    const char* stage="npc_authority_absent";
};
struct State {
    bool initialized=false,profilesVerified=false,unsupported=false,frameArmed=false,inPoll=false,frameAdapterAllowed=true;
    uint32_t error=0,gameThread=0,bound=0,lastIndex=0,profileFailureRva=0;
    uint32_t functionChecksRun=0,failedFunctionIndex=static_cast<uint32_t>(FunctionCount);
    uint16_t expectedFunctionBytes=0,actualFunctionBytes=0,expectedFunctionStorageBytes=0;
    uint32_t expectedFunctionFlags=0,actualFunctionFlags=0;
    uintptr_t expectedFunctionExec=0,actualFunctionExec=0;
    const char* failedFunction="none";
    void *failedFunctionObject=nullptr,*expectedFunctionOwner=nullptr,*actualFunctionOwner=nullptr;
    std::array<FunctionCheck,FunctionCount> functionChecks{};
    uint64_t dispatchCalls=0,dispatchRejected=0;
    bool lastDispatchMatched=false;
    void *lastDispatchObject=nullptr,*lastDispatchTable=nullptr,*lastDispatchTarget=nullptr;
    uintptr_t lastDispatchTargetRva=0;
    uint64_t frames=0,polls=0,manualFrames=0,neutralFrames=0,nativeYields=0,retirements=0;
    uint64_t snapshots=0,unpairedSkips=0,physicsCompleted=0;
    uint64_t unverifiedFrameAborts=0;
    uint64_t failedManualAborts=0,failedManualAbortDtorCalls=0;
    uint32_t failedManualAbortError=0;
    uintptr_t failedManualAbortExceptionRva=0;
    bool lastFrameAbortedUnverified=false;
    uint64_t frameSnapshotAtMs=0,cleanupBoundaryAtMs=0,cleanupBoundaryCalls=0,cleanupSnapshotFrame=0;
    float cleanupSnapshotDelta=0,cleanupCurrentPostWorldDelta=0;
    bool cleanupSnapshotPending=false,cleanupDeltaDiffers=false;
    float frameDelta=0;
    double elapsed=0;
    uintptr_t exceptionRva=0;
    void *world=nullptr,*driver=nullptr,*gameState=nullptr,*fixedStep=nullptr;
    Weak worldId{},driverId{},gameStateId{},fixedStepId{};
    const char* stage="npc_authority_not_initialized";
};
struct Functions {void *rotation=nullptr,*crouchGetter=nullptr,*alive=nullptr,*simulatingPhysics=nullptr;};
inline State state;
inline Hooks hooks;
inline Functions functions;
inline std::array<Entry,MaxPlayers> entries{};
inline std::array<uint32_t,MaxPlayers> retiredEpochs{};
inline const State& GetState(){return state;}
inline const Entry* Player(size_t index){return index<entries.size()&&entries[index].present?&entries[index]:nullptr;}
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
template<class T> inline bool Put(void* p,size_t offset,const T& v){return original_players::Write(p,offset,v);}
inline bool Fail(Entry* e,unsigned code,const char* why){
    if(!state.unsupported){state.error=code;state.stage=why;state.unsupported=true;}
    if(e&&!e->error){e->error=code;e->stage=why;e->ready=false;}return false;
}
inline bool Wait(Entry& e,const char* why){e.ready=false;e.stage=why;return true;}
inline bool NpcSame(void* p,const Weak& id){return local_server_character_stream::Same(p,id);}
inline bool NpcIdentity(void* p,Weak& id){return local_server_character_stream::Identity(p,id);}
inline void* WorldOf(void* actor){return local_server_character_stream::WorldOf(actor);}
inline bool EntriesMatch();
inline bool Thread(){return state.initialized&&!state.unsupported&&state.gameThread==GetCurrentThreadId()&&original_players::Thread();}
inline bool Context(){
    const auto& stream=local_server_character_stream::GetState();
    return Thread()&&NpcSame(state.world,state.worldId)&&NpcSame(state.driver,state.driverId)
        &&stream.initialized&&!stream.unsupported&&stream.world==state.world&&stream.driver==state.driver
        &&NpcSame(state.gameState,state.gameStateId)&&NpcSame(state.fixedStep,state.fixedStepId)
        &&Read<void*>(state.world,0x40)==state.driver&&Read<void*>(state.driver,0x178)==state.world
        &&!Read<void*>(state.driver,0xc0)&&Read<void*>(state.world,0x178)==state.gameState
        &&Read<void*>(state.gameState,0xcc0)==state.fixedStep&&Read<uint8_t>(state.fixedStep,0xd9)!=0;
}
inline uintptr_t ImageRva(void* target){
    const auto address=reinterpret_cast<uintptr_t>(target);
    return address>=engine::image&&address-engine::image<0xe69c000?address-engine::image:0;
}
inline bool Dispatch(void* object,void* function,void* params){
    auto table=Read<void**>(object);auto target=Read<void*>(table,0x298);
    state.lastDispatchObject=object;state.lastDispatchTable=table;state.lastDispatchTarget=target;state.lastDispatchTargetRva=ImageRva(target);
    const bool actor=original_players::IsA(object,original_players::ShooterCharacterClass)
        ||original_players::IsA(object,original_players::ShooterControllerClass);
    const bool capsule=original_players::IsA(object,0x3685a30);
    // Actor wrapper3EDA0D0 calls the real UObject.ProcessEvent1CDC850.
    //1D07DD0 is StaticFindObject and is not a valid dispatch target.
    state.lastDispatchMatched=object&&function&&((actor&&target==reinterpret_cast<void*>(engine::image+0x3eda0d0))
        ||(capsule&&target==reinterpret_cast<void*>(engine::image+0x1cdc850)));
    if(!state.lastDispatchMatched){++state.dispatchRejected;return false;}
    ++state.dispatchCalls;
    reinterpret_cast<void(*)(void*,void*,void*)>(target)(object,function,params);return true;
}
inline bool Function(void*& out,size_t index,const char* label,const wchar_t* path,
                     uint16_t logicalBytes,uint16_t storageBytes,uint32_t flags,uintptr_t exec,uintptr_t ownerGetter){
    if(index>=state.functionChecks.size())return false;
    auto& check=state.functionChecks[index];check.name=label;check.checked=true;
    check.expectedBytes=logicalBytes;check.storageBytes=storageBytes;check.expectedFlags=flags;
    check.expectedExec=engine::image+exec;check.ownerGetterRva=ownerGetter;
    check.expectedOwner=engine::Class(ownerGetter);out=engine::Find(path);check.function=out;check.found=out!=nullptr;
    check.actualBytes=Read<uint16_t>(out,0xbe);check.actualFlags=Read<uint32_t>(out,0xb8);
    check.actualExec=Read<uintptr_t>(out,0xe0);check.actualOwner=Read<void*>(out,0x28);
    check.bytesMatch=check.found&&check.actualBytes==logicalBytes&&logicalBytes<=storageBytes;
    check.flagsMatch=check.found&&check.actualFlags==flags;check.execMatch=check.found&&check.actualExec==check.expectedExec;
    check.ownerMatch=check.found&&check.expectedOwner&&check.actualOwner==check.expectedOwner;++state.functionChecksRun;
    const bool matched=check.bytesMatch&&check.flagsMatch&&check.execMatch&&check.ownerMatch;
    if(!matched&&state.failedFunctionIndex==FunctionCount){
        state.failedFunctionIndex=static_cast<uint32_t>(index);state.failedFunction=label;state.failedFunctionObject=out;
        state.expectedFunctionBytes=logicalBytes;state.actualFunctionBytes=check.actualBytes;state.expectedFunctionStorageBytes=storageBytes;
        state.expectedFunctionFlags=flags;state.actualFunctionFlags=check.actualFlags;
        state.expectedFunctionExec=check.expectedExec;state.actualFunctionExec=check.actualExec;
        state.expectedFunctionOwner=check.expectedOwner;state.actualFunctionOwner=check.actualOwner;
    }
    return matched;
}
inline bool RawLinks(Entry& e){
    if(!e.present||!e.epoch||!NpcSame(e.pc,e.pcId)||!NpcSame(e.ps,e.psId)||!NpcSame(e.pawn,e.pawnId)
       ||!NpcSame(e.movement,e.movementId)||!NpcSame(e.root,e.rootId)||!NpcSame(e.input,e.inputId)
       ||!NpcSame(e.inventory,e.inventoryId)||!NpcSame(e.stream,e.streamId))return false;
    if(Read<uint8_t>(e.pc,0x224)!=3||Read<uint8_t>(e.pawn,0x224)!=3||Read<uint8_t>(e.pawn,0x128)!=2
       ||Read<void*>(e.pc,0x510)||Read<void*>(e.pc,0x708)||Read<void*>(e.pc,0x710)
       ||Read<void*>(e.pc,0x4b0)!=e.pawn||Read<void*>(e.pawn,0x4a0)!=e.pc
       ||Read<void*>(e.pc,0x470)!=e.ps||Read<void*>(e.pawn,0x488)!=e.ps||Read<void*>(e.ps,0x210)!=e.pc
       ||WorldOf(e.pc)!=state.world||WorldOf(e.ps)!=state.world||WorldOf(e.pawn)!=state.world
       ||Read<void*>(e.pawn,0x4f8)!=e.movement||Read<void*>(e.movement,0xc8)!=e.pawn
       ||Read<void*>(e.movement,0x190)!=e.pawn||Read<void*>(e.movement,0x1a8)!=e.pawn
       ||Read<void*>(e.pawn,0x290)!=e.root||Read<void*>(e.movement,0xd8)!=e.root||Read<void*>(e.root,0xc8)!=e.pawn
       ||Read<void*>(e.pawn,0xba0)!=e.input||Read<void*>(e.input,0xc8)!=e.pawn||Read<void*>(e.input,0x148)!=e.pawn
       ||Read<void*>(e.pawn,0xc08)!=e.inventory||Read<void*>(e.inventory,0xc8)!=e.pawn
       ||Read<void*>(e.pawn,0xba8)!=e.stream||Read<void*>(e.stream,0xc8)!=e.pawn||Read<void*>(e.stream,0xe8)!=e.pawn)return false;
    auto pawnTable=Read<void**>(e.pawn),pcTable=Read<void**>(e.pc),movementTable=Read<void**>(e.movement);
    return Read<void*>(pawnTable,0x830)==reinterpret_cast<void*>(engine::image+0x48548e0)
        &&Read<void*>(pcTable,0x800)==reinterpret_cast<void*>(engine::image+0x4929b30)
        &&Read<void*>(pcTable,0x7b8)==reinterpret_cast<void*>(engine::image+0x781fc90)
        &&Read<void*>(pawnTable,0x8a0)==reinterpret_cast<void*>(engine::image+0x4856280)
        &&Read<void*>(pawnTable,0x868)==reinterpret_cast<void*>(engine::image+0x78800d0)
        &&Read<void*>(movementTable,0x658)==reinterpret_cast<void*>(engine::image+0x41e3070)
        &&Read<void*>(movementTable,0x6c0)==reinterpret_cast<void*>(engine::image+0x4188b70)
        &&Read<void*>(movementTable,0x4f8)==reinterpret_cast<void*>(engine::image+0x41dfca0)
        &&Read<void*>(movementTable,0xa98)==reinterpret_cast<void*>(engine::image+0x78954d0)
        &&engine::function<int(*)(void*)>(0x3efc820)(e.pc)==2;
}
inline bool Links(Entry& e){return Context()&&RawLinks(e);}
inline bool RetirementContext(){
    return state.initialized&&state.gameThread==GetCurrentThreadId()&&original_players::Thread()
        &&NpcSame(state.world,state.worldId)&&NpcSame(state.driver,state.driverId)
        &&Read<void*>(state.world,0x40)==state.driver&&Read<void*>(state.driver,0x178)==state.world
        &&!Read<void*>(state.driver,0xc0);
}
inline bool CrouchReadback(Entry& e,uint8_t raw){
    uint8_t result=255;
    return e.crouchMetadata&&Dispatch(e.pawn,functions.crouchGetter,&result)&&result<=1
        &&Read<uint8_t>(e.pawn,0x1538)==raw&&result==uint8_t(raw&1);
}
inline bool CrouchMetadata(Entry& e){
    uint32_t wanted[3]{};auto type=Read<void*>(e.pawn,0x10);
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(wanted,L"bCrouchHeld",1);
    auto field=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,wanted,3);
    auto fieldClass=Read<void*>(field,8),boolClass=engine::function<void*(*)()>(0x1c77e20)();
    e.crouchMetadata=field&&fieldClass==boolClass&&Read<uint64_t>(fieldClass,0x18)==0x28001
        &&Read<uint64_t>(field,0x20)==Read<uint64_t>(wanted)&&Read<int32_t>(field,0x44)==0x1538
        &&Read<int32_t>(field,0x30)==1&&Read<int32_t>(field,0x34)==1&&Read<int32_t>(type,0x60)>0x1538
        &&Read<uint8_t>(field,0x78)==1&&Read<uint8_t>(field,0x79)==0
        &&Read<uint8_t>(field,0x7a)==1&&Read<uint8_t>(field,0x7b)==1;
    return e.crouchMetadata&&CrouchReadback(e,Read<uint8_t>(e.pawn,0x1538));
}
inline bool Clock(Entry& e){
    auto prediction=Read<void*>(e.movement,0x7f0);
    if(!prediction||prediction!=e.prediction||Read<void*>(e.movement,0x1530)!=e.timeShift
       ||e.clock!=static_cast<uint8_t*>(prediction)+0xf0)return false;
    auto table=Read<void**>(e.clock);
    auto vbtable=Read<void*>(e.clock,0x10);
    const int32_t displacement=Read<int32_t>(vbtable,4);
    if(table!=reinterpret_cast<void**>(engine::image+0xb600a60)
       ||Read<void*>(table,0x40)!=reinterpret_cast<void*>(engine::image+0x789f280)
       ||!vbtable||displacement<0||displacement>0x1000||Read<uint32_t>(e.clock,size_t(displacement)+0x1c)!=1
       ||!local_server_npc_time_domain::Validate(e.playerIndex,e.pawn,e.movement,e.epoch)
       ||!std::isfinite(Read<float>(e.clock,0x20)))return false;
    e.baseOffset=static_cast<uint32_t>(displacement);e.index=Read<uint32_t>(e.clock,e.baseOffset+0x18);
    return true;
}
inline bool Alive(Entry& e){
    uint8_t alive=255;
    if(!Dispatch(e.pawn,functions.alive,&alive)||alive>1)return Fail(&e,3801,"npc_authority_original_alive_query_failed");
    e.alive=alive!=0;return true;
}
inline bool NativeLocal(Entry& e){e.nativeLocal=engine::function<bool(*)(void*)>(0x48548e0)(e.pawn);return e.nativeLocal;}
inline bool ValidFrame(const Frame& frame){
    const double norm=frame.worldDirection.x*frame.worldDirection.x+frame.worldDirection.y*frame.worldDirection.y+frame.worldDirection.z*frame.worldDirection.z;
    return original_players::Finite(frame.worldDirection)&&original_players::Finite(frame.rotation)&&norm<=1.000001;
}
inline Frame SelectedFrame(const Entry& e){
    if(e.hasIntent&&state.elapsed-e.submittedAt<=IntentHoldSeconds)return e.latest;
    Frame neutral{};neutral.rotation=Read<Vec>(e.pc,0x4e8);return neutral;
}
inline bool ApplyInput(Entry& e,const Frame& frame,float delta,bool consume){
    if(!Links(e)||!ValidFrame(frame))return Fail(&e,3802,"npc_authority_input_identity_or_frame_invalid");
    // The original ForceMoveInputState API is local-only. This is the accepted
    // authority INPUT bit missing from its remote setter branch, not posture.
    const uint8_t old=Read<uint8_t>(e.pawn,0x1538),wanted=uint8_t((old&0xfe)|(frame.crouch?1:0));
    if(!CrouchReadback(e,old))return Fail(&e,3803,"npc_authority_crouch_input_metadata_readback_failed");
    if(wanted!=old){if(!Put(e.pawn,0x1538,wanted))return Fail(&e,3804,"npc_authority_crouch_input_store_failed");++e.crouchWrites;}
    engine::function<void(*)(void*,bool)>(0x7884be0)(e.pawn,frame.walk);++e.walkCalls;
    engine::function<void(*)(void*,bool)>(0x7883850)(e.pawn,frame.crouch);++e.crouchCalls;
    if(!CrouchReadback(e,wanted))return Fail(&e,3805,"npc_authority_crouch_input_changed_by_native_setter");
    if(frame.jump!=e.lastJump){engine::function<void(*)(void*,bool)>(0x78812f0)(e.pawn,frame.jump);e.lastJump=frame.jump;++e.jumpCalls;}
    Vec rotation=frame.rotation;
    if(!Dispatch(e.pc,functions.rotation,&rotation))return Fail(&e,3806,"npc_authority_native_control_rotation_failed");
    engine::function<void(*)(void*,const Vec*,float)>(0x78800d0)(e.pawn,&rotation,delta);++e.faceCalls;
    // AddMovementInput respects the original controller's input-ignore state.
    // Another producer's pending vector is never consumed or overwritten.
    const auto pending=Read<Vec>(e.pawn,0x4b8);
    if(!original_players::Finite(pending)||pending.x!=0||pending.y!=0||pending.z!=0)
        return Fail(&e,3807,"npc_authority_another_native_input_producer_has_pending_vector");
    engine::function<void(*)(void*,const Vec*,float,bool)>(0x4856280)(e.pawn,&frame.worldDirection,1.0f,false);++e.inputCalls;
    if(consume){
        Vec input{};engine::function<Vec*(*)(void*,Vec*)>(0x41e3070)(e.movement,&input);
        const double norm=input.x*input.x+input.y*input.y+input.z*input.z;
        if(!original_players::Finite(input)||norm>1.000001)return Fail(&e,3808,"npc_authority_consumed_input_invalid");
        e.consumedInput=input;
    }
    return Links(e)||Fail(&e,3846,"npc_authority_native_input_changed_ownership_links");
}
inline bool Initialize(void* world,void* driver,const Hooks& selected={}){
    if(state.initialized)return Context()&&state.world==world&&state.driver==driver;
    if(state.unsupported||!original_players::Thread()||!world||!driver)return false;
    state.world=world;state.driver=driver;state.gameState=Read<void*>(world,0x178);state.fixedStep=Read<void*>(state.gameState,0xcc0);
    state.gameThread=GetCurrentThreadId();hooks=selected;
    const auto& stream=local_server_character_stream::GetState();
    if(!stream.initialized||!stream.profilesVerified||stream.world!=world||stream.driver!=driver||!EntriesMatch())
        return Fail(nullptr,3809,"npc_authority_original_source_or_capture_profile_unavailable");
    state.profilesVerified=true;
    if(!NpcIdentity(world,state.worldId)||!NpcIdentity(driver,state.driverId)||!NpcIdentity(state.gameState,state.gameStateId)
       ||!NpcIdentity(state.fixedStep,state.fixedStepId))return Fail(nullptr,3810,"npc_authority_world_weak_identity_unavailable");
    const bool rotationOk=Function(functions.rotation,0,"Controller.SetControlRotation",L"/Script/Engine.Controller.SetControlRotation",24,24,0x04c20402,0x42abfd0,0x3ed67a0);
    const bool crouchOk=Function(functions.crouchGetter,1,"ShooterCharacter.GetCrouchHeld",L"/Script/ShooterGame.ShooterCharacter.GetCrouchHeld",1,1,0x54020401,0x6c64b00,0x6c5c480);
    const bool aliveOk=Function(functions.alive,2,"ShooterCharacter.IsAlive",L"/Script/ShooterGame.ShooterCharacter.IsAlive",1,1,0x54020401,0x6c62390,0x6c5c480);
    // FName12 at0 + bool return at12 => logical ParmsSize13. The original
    // generated C++ caller storage remains16; its tail padding is not a parm.
    const bool physicsOk=Function(functions.simulatingPhysics,3,"SceneComponent.IsSimulatingPhysics",L"/Script/Engine.SceneComponent.IsSimulatingPhysics",13,16,0x54020402,0x3e9ba90,0x36c7580);
    if(!rotationOk||!crouchOk||!aliveOk||!physicsOk)
        return Fail(nullptr,3811,"npc_authority_original_function_metadata_mismatch");
    state.initialized=true;
    if(!Context())return Fail(nullptr,3812,"npc_authority_requires_original_loopback_host_world");
    if(!local_server_npc_time_domain::Initialize(world,driver))
        return Fail(nullptr,local_server_npc_time_domain::GetState().error,local_server_npc_time_domain::GetState().stage);
    state.stage="npc_authority_candidate_initialized_runtime_unverified";return true;
}
// Early phase before equipment/effect event publication. It deliberately does
// not claim Crouch/Combat/stream producer readiness, which Register checks later.
inline bool PrepareFreshClock(size_t index,void* pc,void* ps,void* pawn,uint32_t epoch,bool human){
    if(human)return true;
    if(!Context()||index==0||index>=entries.size())return Fail(nullptr,3849,"npc_authority_early_clock_phase_context_invalid");
    if(!local_server_npc_time_domain::BindFresh(index,pc,ps,pawn,epoch))
        return Fail(nullptr,local_server_npc_time_domain::GetState().error,local_server_npc_time_domain::GetState().stage);
    return true;
}
inline bool Register(size_t index,void* pc,void* ps,void* pawn,uint32_t epoch,bool human){
    if(human)return true;
    if(!Context()||index==0||index>=entries.size()||!epoch||epoch<=retiredEpochs[index])
        return Fail(nullptr,3813,"npc_authority_registration_epoch_or_context_invalid");
    if(!PrepareFreshClock(index,pc,ps,pawn,epoch,human))return false;
    auto& e=entries[index];state.lastIndex=static_cast<uint32_t>(index);
    if(e.present)return (e.pc==pc&&e.ps==ps&&e.pawn==pawn&&e.epoch==epoch&&Links(e))
        ||Fail(&e,3847,"npc_authority_registration_requires_prior_epoch_retirement");
    if(!original_players::IsA(pc,original_players::ShooterControllerClass)||!original_players::IsA(ps,original_players::ShooterStateClass)
       ||!original_players::IsA(pawn,original_players::ShooterCharacterClass))return Fail(&e,3814,"npc_authority_requires_original_controller_state_character");
    e.pc=pc;e.ps=ps;e.pawn=pawn;e.epoch=epoch;e.playerIndex=static_cast<uint32_t>(index);e.movement=Read<void*>(pawn,0x4f8);e.root=Read<void*>(pawn,0x290);
    e.input=Read<void*>(pawn,0xba0);e.inventory=Read<void*>(pawn,0xc08);e.stream=Read<void*>(pawn,0xba8);e.present=true;
    if(!original_players::IsA(e.movement,0x6c7d380)||!NpcIdentity(pc,e.pcId)||!NpcIdentity(ps,e.psId)||!NpcIdentity(pawn,e.pawnId)
       ||!NpcIdentity(e.movement,e.movementId)||!NpcIdentity(e.root,e.rootId)||!NpcIdentity(e.input,e.inputId)
       ||!NpcIdentity(e.inventory,e.inventoryId)||!NpcIdentity(e.stream,e.streamId)||!Links(e)||!CrouchMetadata(e))
        return Fail(&e,3815,"npc_authority_original_registration_links_or_crouch_metadata_invalid");
    e.clock=engine::function<void*(*)(void*)>(0x7892e10)(e.movement);e.prediction=Read<void*>(e.movement,0x7f0);e.timeShift=Read<void*>(e.movement,0x1530);
    if(!e.timeShift||!Clock(e))return Fail(&e,3816,"npc_authority_original_server_prediction_clock_unavailable");
    e.lastJump=(Read<uint8_t>(pawn,0xf7b)&1)!=0;e.latest.rotation=Read<Vec>(pc,0x4e8);
    if(!original_players::Finite(e.latest.rotation))return Fail(&e,3817,"npc_authority_initial_control_rotation_nonfinite");
    ++state.bound;e.stage="npc_authority_waiting_first_paired_world_tick";return true;
}
inline void ActorSpawned(size_t index,void* pc,void* ps,void* pawn,uint32_t epoch,bool human){Register(index,pc,ps,pawn,epoch,human);}
inline bool SubmitBot(size_t index,const Frame& frame,double delta){
    if(!Context()||index==0||index>=entries.size()||!std::isfinite(delta)||delta<0||delta>.25||!ValidFrame(frame))
        return Fail(nullptr,3818,"npc_authority_latest_intent_invalid");
    auto& e=entries[index];if(!Links(e))return Fail(&e,3819,"npc_authority_latest_intent_epoch_expired");
    if(!hooks.weaponInput&&(frame.fire||frame.reload))return Fail(&e,3820,"npc_authority_weapon_input_callback_missing");
    if(hooks.weaponInput&&!hooks.weaponInput(index,frame.fire,frame.reload))return Fail(&e,3821,"npc_authority_original_weapon_input_rejected");
    e.latest=frame;e.hasIntent=true;e.submittedAt=state.elapsed;++e.submissions;return true;
}
inline bool Ready(size_t index,void* pawn,uint32_t epoch){
    if(index==0||index>=entries.size())return false;auto& e=entries[index];
    return e.present&&e.pawn==pawn&&e.epoch==epoch&&!e.error&&e.ready&&Links(e);
}
inline bool CleanupExecutionScopesIdle(){
    if(state.gameThread&&state.gameThread!=GetCurrentThreadId())return false;
    if(state.inPoll||local_server_npc_time_domain::GetState().seedScopes
       ||local_server_character_stream::GetState().inNativeCaptures
       ||local_server_character_stream::GetState().manualCaptureScopes
       ||local_server_character_stream::GetState().savedMoveDtorScopes)return false;
    for(const auto& capture:local_server_character_stream::bindings){
        if(capture.observer&&InterlockedCompareExchange(&capture.observer->inNativeCapture,0,0))return false;
    }
    return true;
}
inline bool CleanupBoundaryReady(){
    if(!CleanupExecutionScopesIdle())return false;
    for(const auto& capture:local_server_character_stream::bindings)if(capture.manualPending)return false;
    // Constructed but no-longer-in-flight scratch is deliberately left for
    // sender.Retire's original destructor; it is not an active physics scope.
    return true;
}
inline bool BeforeRetire(size_t index,void* pawn,uint32_t epoch){
    if(index==0)return true;
    if(!RetirementContext()||index>=entries.size())return Fail(nullptr,3822,"npc_authority_retire_context_invalid");
    if(!CleanupBoundaryReady())return Fail(nullptr,3824,"npc_authority_retire_native_capture_or_seed_scope_active");
    auto& e=entries[index];if(!e.present){
        if(local_server_npc_time_domain::Player(index)){
            if(!local_server_npc_time_domain::Retire(index,pawn,epoch))
                return Fail(nullptr,local_server_npc_time_domain::GetState().error,local_server_npc_time_domain::GetState().stage);
            retiredEpochs[index]=epoch;return true;
        }
        return retiredEpochs[index]==epoch;
    }
    if(e.pawn!=pawn||e.epoch!=epoch||!RawLinks(e))return Fail(&e,3823,"npc_authority_retire_epoch_or_identity_mismatch");
    auto* capture=local_server_character_stream::Existing(e.movement);
    if(capture&&capture->manualPending)return Fail(&e,3824,"npc_authority_retire_during_physical_capture");
    if(hooks.weaponInput&&!hooks.weaponInput(index,false,false))return Fail(&e,3825,"npc_authority_retire_weapon_input_cancel_failed");
    if(!local_server_npc_time_domain::Retire(index,pawn,epoch))
        return Fail(&e,local_server_npc_time_domain::GetState().error,local_server_npc_time_domain::GetState().stage);
    retiredEpochs[index]=epoch;e={};--state.bound;++state.retirements;return true;
}
inline void ObserveMovementSettings(Entry& e){
    auto& observation=e.movementObservation;
    observation.settingsKnown=original_players::Readable(e.movement,0x57b);
    if(!observation.settingsKnown)return;
    observation.planeFlags=Read<uint8_t>(e.movement,0x138);
    observation.planeAxis=Read<uint8_t>(e.movement,0x13b);
    observation.planeNormal=Read<Vec>(e.movement,0x108);observation.planeOrigin=Read<Vec>(e.movement,0x120);
    observation.movementMode=Read<uint8_t>(e.movement,0x271);observation.customMode=Read<uint8_t>(e.movement,0x272);
    observation.groundMode=Read<uint8_t>(e.movement,0x57a);
    observation.primitiveIsRoot=Read<void*>(e.movement,0xe0)==e.root;
    observation.capsuleIsRoot=Read<void*>(e.pawn,0x500)==e.root;
    observation.settingsKnown=original_players::Finite(observation.planeNormal)&&original_players::Finite(observation.planeOrigin);
}
inline bool BeforeWorldTick(float delta,bool allowInput=true){
    if(!Context()||state.frameArmed||state.inPoll||!std::isfinite(delta)||delta<0)
        return Fail(nullptr,3826,"npc_authority_world_tick_snapshot_unpaired_or_invalid");
    if(!local_server_npc_time_domain::Observe())return Fail(nullptr,local_server_npc_time_domain::GetState().error,local_server_npc_time_domain::GetState().stage);
    ++state.frames;state.frameDelta=delta;state.frameArmed=true;state.frameAdapterAllowed=allowInput;state.lastFrameAbortedUnverified=false;
    state.frameSnapshotAtMs=GetTickCount64();
    for(size_t n=1;n<entries.size();++n){auto& e=entries[n];if(!e.present)continue;state.lastIndex=static_cast<uint32_t>(n);
        if(!Links(e)||!Clock(e)||!Alive(e))return Fail(&e,3827,"npc_authority_before_world_identity_clock_or_life_changed");
        e.snapshot=false;if(!e.alive){e.ready=false;e.stage="npc_authority_native_death_waiting_epoch_retirement";continue;}
        e.movementObservation={};e.movementObservation.worldFrame=state.frames;
        e.movementObservation.beforeWorld=Read<Vec>(e.root,0x320);e.movementObservation.stages=1;
        NativeLocal(e);
        // Once any native producer is observed, the adapter never reclaims
        // physics within this spawn epoch. Its latest input is supplied before
        // the next real WorldTick so that the native owner can consume it.
        if(e.nativeLocal)e.owner=Owner::Native;
        if(allowInput&&e.owner==Owner::Native&&!ApplyInput(e,SelectedFrame(e),(std::min)(delta,MaxDelta),false))return false;
        e.rawBefore=e.index;e.accumulatedBefore=Read<float>(e.clock,0x20);e.stepsBefore=Read<uint32_t>(e.clock,0x968);
        const auto* capture=local_server_character_stream::Existing(e.movement);
        e.captureBefore=capture?capture->lastCapturedIndex:0;
        e.totalMovesBefore=Read<uint32_t>(e.movement,0x14b8);e.snapshotFrame=state.frames;e.snapshot=true;++state.snapshots;
    }return true;
}
// -1 invalid contract, 0 original dependency/skip gate not ready, 1 ready.
inline int ManualDependencies(Entry& e){
    if(!Read<uint8_t>(e.pawn,0xa90)||!Read<uint8_t>(e.movement,0x15e0)
       ||!(Read<uint8_t>(e.input,0xa0)&1)||!(Read<uint8_t>(e.inventory,0xa0)&1)||!Read<uint8_t>(e.pawn,0xc18))
        {Wait(e,"npc_authority_waiting_original_beginplay_registration_and_beforeframe_components");return 0;}
    if(!Read<uint8_t>(e.stream,0xe0)||!Read<uint8_t>(e.movement,0x1080))
        {Wait(e,"npc_authority_waiting_original_component_stream_capture_registration");return 0;}
    if(Read<uint8_t>(e.movement,0x1630)||(Read<uint8_t>(e.pawn,0x152c)&&!(Read<uint8_t>(e.movement,0x184)&2)))
        {Fail(&e,3828,"npc_authority_extra_original_input_branch_unproven");return -1;}
    struct PhysicsQuery {uint32_t name[3]{};uint8_t value=255,padding[3]{};};
    static_assert(sizeof(PhysicsQuery)==16&&offsetof(PhysicsQuery,value)==12);
    PhysicsQuery query;
    ++e.rootPhysicsQueries;e.rootPhysicsDispatched=false;e.rootPhysicsResult=query.value;
    e.rootPhysicsClass=Read<void*>(e.root,0x10);e.rootPhysicsTable=Read<void**>(e.root);
    e.rootPhysicsProcessEvent=Read<void*>(e.rootPhysicsTable,0x298);e.rootPhysicsProcessEventRva=ImageRva(e.rootPhysicsProcessEvent);
    e.rootPhysicsNativePredicate=Read<void*>(e.rootPhysicsTable,0x4d0);e.rootPhysicsNativePredicateRva=ImageRva(e.rootPhysicsNativePredicate);
    e.rootPhysicsClassValid=original_players::IsA(e.root,0x3685a30);
    e.rootPhysicsOwnerMatches=NpcSame(e.root,e.rootId)&&Read<void*>(e.root,0xc8)==e.pawn&&Read<void*>(e.movement,0xd8)==e.root;
    e.rootPhysicsTargetMatched=e.rootPhysicsProcessEvent==reinterpret_cast<void*>(engine::image+0x1cdc850);
    if(e.rootPhysicsClassValid&&e.rootPhysicsOwnerMatches&&e.rootPhysicsTargetMatched)
        e.rootPhysicsDispatched=Dispatch(e.root,functions.simulatingPhysics,&query);
    e.rootPhysicsResult=query.value;
    // The original exec3E9BA90 ends in MOV byte[r14],AL at3E9BBA2:
    // it replaces the whole return byte, not a masked FBoolProperty update.
    if(!e.rootPhysicsDispatched||query.value>1)
        {Fail(&e,3829,"npc_authority_original_root_physics_query_failed");return -1;}
    if(query.value){Fail(&e,3830,"npc_authority_rigidbody_simulation_branch_unsupported");return -1;}
    if(!engine::function<bool(*)(void*)>(0x4188b70)(e.movement)){Fail(&e,3831,"npc_authority_original_movement_data_invalid");return -1;}
    if(engine::function<bool(*)(void*,float)>(0x41dfca0)(e.movement,Step))
        {Wait(e,"npc_authority_original_movement_skip_predicate_active");return 0;}
    return 1;
}
inline bool PollImpl(float delta,bool allowAdapter=true){
    if(!Context()||!state.frameArmed||state.inPoll||!std::isfinite(delta)||delta<0||delta!=state.frameDelta)
        return Fail(nullptr,3832,"npc_authority_poll_requires_same_paired_real_world_tick");
    state.frameArmed=false;state.inPoll=true;++state.polls;state.elapsed+=delta;
    allowAdapter=allowAdapter&&state.frameAdapterAllowed;
    if(!local_server_npc_time_domain::Observe()){state.inPoll=false;
        return Fail(nullptr,local_server_npc_time_domain::GetState().error,local_server_npc_time_domain::GetState().stage);}
    for(size_t n=1;n<entries.size();++n){auto& e=entries[n];if(!e.present)continue;state.lastIndex=static_cast<uint32_t>(n);
        if(!Links(e)||!Clock(e)||!Alive(e)){state.inPoll=false;return Fail(&e,3833,"npc_authority_after_world_identity_clock_or_life_changed");}
        if(!e.snapshot||e.snapshotFrame!=state.frames){++state.unpairedSkips;continue;}e.snapshot=false;
        if(!e.alive){e.ready=false;e.stage="npc_authority_native_death_waiting_epoch_retirement";continue;}
        e.movementObservation.afterWorld=Read<Vec>(e.root,0x320);e.movementObservation.stages|=2;
        ObserveMovementSettings(e);
        e.totalMovesAfter=Read<uint32_t>(e.movement,0x14b8);
        const bool rawChanged=e.index!=e.rawBefore;
        if(rawChanged&&static_cast<int32_t>(e.index-e.rawBefore)<=0){
            state.inPoll=false;return Fail(&e,3848,"npc_authority_native_clock_regressed_within_spawn_epoch");}
        const bool nativeClockChanged=rawChanged||Read<float>(e.clock,0x20)!=e.accumulatedBefore
            ||Read<uint32_t>(e.clock,0x968)!=e.stepsBefore;
        // TotalMovesExecuted is incremented in the client send loop78999B0,
        // not the Role3 physical loop. It is an additional conflict detector,
        // never presented as an authority physics completion counter.
        if(e.totalMovesAfter!=e.totalMovesBefore){state.inPoll=false;return Fail(&e,3834,"npc_authority_unexpected_client_move_producer_on_host_npc");}
        if(nativeClockChanged||NativeLocal(e)||e.owner==Owner::Native){
            e.owner=Owner::Native;if(allowAdapter)e.budget=0;e.ready=false;++e.nativeObservations;++state.nativeYields;
            if(rawChanged)++e.nativeAdvances;
            // Clock work alone is not proof of physical movement: the retained
            // failed-world-bounds path can advance without a physical frame.
            // Only the sender's validated original789526D post-physics capture
            // can establish native physical completion for this readiness gate.
            const auto* capture=local_server_character_stream::Existing(e.movement);
            if(rawChanged&&capture&&capture->lastCapturedIndex!=e.captureBefore&&capture->lastCapturedIndex==e.index){
                ++e.nativeCaptureObservations;e.ready=true;
            }
            e.stage=e.ready?"npc_authority_yielded_to_captured_original_native_frame"
                :"npc_authority_yielded_clock_owner_waiting_native_physical_capture_proof";
            continue;
        }
        if(!allowAdapter){
            const auto* domain=local_server_npc_time_domain::Player(n);
            if(!domain||!local_server_npc_time_domain::PublicIndex(*domain,e.index,e.commonWireIndex)){
                state.inPoll=false;return Fail(&e,3884,"npc_authority_observation_common_time_proof_invalid");}
            e.commonTargetIndex=local_server_npc_time_domain::GetState().source.index;
            if(e.commonWireIndex>e.commonTargetIndex){state.inPoll=false;return Fail(&e,3881,"npc_authority_actual_clock_ahead_of_original_common_source");}
            e.commonUnservedSteps=e.commonTargetIndex-e.commonWireIndex;
            e.ready=false;e.stage="npc_authority_observed_real_world_tick_adapter_debt_preserved";
            continue;
        }
        e.owner=Owner::Adapter;
        const int dependencies=ManualDependencies(e);
        if(dependencies<0){state.inPoll=false;return false;}if(!dependencies)continue;
        // The ORIGINAL common native producer admits the physical timestamps.
        // Keep actual local raw and retain its unserved common steps as debt.
        const float previous=Read<float>(e.clock,0x20);
        // Only this adapter's already-observed clock may receive budget here.
        // Unexpected/unbounded old state remains an error, not a cap repair.
        if(!std::isfinite(previous)||previous<0||previous>MaxBudget){state.inPoll=false;return Fail(&e,3835,"npc_authority_native_budget_outside_supported_range");}
        const auto* domain=local_server_npc_time_domain::Player(n);
        if(!domain||!local_server_npc_time_domain::PublicIndex(*domain,e.index,e.commonWireIndex)){
            state.inPoll=false;return Fail(&e,3880,"npc_authority_common_time_current_mapping_invalid");}
        e.commonTargetIndex=local_server_npc_time_domain::GetState().source.index;
        if(e.commonWireIndex>e.commonTargetIndex){state.inPoll=false;return Fail(&e,3881,"npc_authority_actual_clock_ahead_of_original_common_source");}
        e.commonUnservedSteps=e.commonTargetIndex-e.commonWireIndex;
        const double required=static_cast<double>(e.commonUnservedSteps)*Step;
        const double missing=(std::max)(0.0,required-static_cast<double>(previous));
        const float supplied=static_cast<float>((std::min)(missing,static_cast<double>(MaxBudget-previous)));
        e.budget=previous+supplied;e.commonBudgetSuppliedSeconds+=supplied;
        if(!Put(e.clock,0x20,e.budget)||!Put(e.clock,0x968,uint32_t(0))){state.inPoll=false;return Fail(&e,3836,"npc_authority_native_frame_budget_store_failed");}
        e.frameSteps=0;
        while(e.frameSteps<MaxSteps&&e.commonWireIndex<e.commonTargetIndex){
            if(!Links(e)||!Clock(e)){state.inPoll=false;return Fail(&e,3837,"npc_authority_step_context_changed");}
            const uint32_t before=e.index,expected=before==UINT32_MAX?1:before+1;
            if(!engine::function<bool(*)(void*,void*,bool)>(0x789f0e0)(e.clock,e.timeShift,true)){
                e.budget=Read<float>(e.clock,0x20);e.stage="npc_authority_waiting_original_clock_readiness";break;
            }
            ++e.clockAdvances;
            if(!Clock(e)||e.index!=expected){state.inPoll=false;return Fail(&e,3838,"npc_authority_original_clock_advanced_unexpected_index");}
            const auto frame=SelectedFrame(e);
            if(!ApplyInput(e,frame,Step,true)){state.inPoll=false;return false;}
            engine::function<void(*)(void*,float)>(0x7883620)(e.pawn,Step);++e.beforeFrames;
            if(!local_server_character_stream::BeginAuthorityStep(e.pawn,e.movement,Step,e.index,e.consumedInput)){
                state.inPoll=false;return Fail(&e,3839,"npc_authority_original_pre_physics_capture_failed");}
            const auto* activeCapture=local_server_character_stream::Existing(e.movement);
            if(!activeCapture||!activeCapture->observer||!local_server_character_stream::ValidBinding(*activeCapture)){
                state.inPoll=false;return Fail(&e,3839,"npc_authority_original_capture_generation_unavailable");}
            e.captureGeneration=activeCapture->observer->generation;
            e.positionBefore=Read<Vec>(e.root,0x320);++e.physicsCalls;
            engine::function<void(*)(void*,float)>(0x78954d0)(e.movement,Step);++e.physicsCompleted;++state.physicsCompleted;
            e.positionAfter=Read<Vec>(e.root,0x320);e.velocityAfter=Read<Vec>(e.movement,0xf0);
            e.movementObservation.physicalIndex=e.index;ObserveMovementSettings(e);
            if(!original_players::Finite(e.positionAfter)||!original_players::Finite(e.velocityAfter)){
                state.inPoll=false;return Fail(&e,3840,"npc_authority_native_physical_result_nonfinite");}
            if(!local_server_character_stream::CompleteAuthorityStep(e.pawn,e.movement,e.index)){
                state.inPoll=false;return Fail(&e,3841,"npc_authority_original_post_physics_capture_failed");}
            e.movementObservation.afterCapture=Read<Vec>(e.root,0x320);e.movementObservation.stages|=4;
            ++e.captures;++e.frameSteps;
            if(!Put(e.clock,0x968,e.frameSteps)){state.inPoll=false;return Fail(&e,3842,"npc_authority_native_frame_counter_store_failed");}
            struct FrameScope {void* movement;void* fixedStep;void* clock;} scope{e.movement,state.fixedStep,e.clock};
            engine::function<void(*)(void*)>(0x789f310)(&scope);++e.frameScopes;
            e.movementObservation.afterScope=Read<Vec>(e.root,0x320);e.movementObservation.stages|=8;
            ++e.manualFrames;++state.manualFrames;
            if(!e.hasIntent||state.elapsed-e.submittedAt>IntentHoldSeconds){++e.neutralFrames;++state.neutralFrames;}
            e.budget=Read<float>(e.clock,0x20);
            if(!std::isfinite(e.budget)||e.budget<0){state.inPoll=false;return Fail(&e,3835,"npc_authority_native_budget_outside_supported_range");}
            if(!Clock(e)||e.index!=expected||!Links(e)){state.inPoll=false;return Fail(&e,3843,"npc_authority_post_frame_clock_or_links_changed");}
            if(!local_server_npc_time_domain::PublicIndex(*domain,e.index,e.commonWireIndex)){
                state.inPoll=false;return Fail(&e,3882,"npc_authority_post_physics_common_time_mapping_changed");}
            e.commonUnservedSteps=e.commonTargetIndex-e.commonWireIndex;
            e.ready=true;e.stage="npc_authority_original_step_captured_runtime_acceptance_unverified";
        }
        const uint32_t finishedIndex=e.index,finishedSteps=Read<uint32_t>(e.clock,0x968);
        const float finishedAccumulated=Read<float>(e.clock,0x20);
        if(e.frameSteps){
            const uint8_t flags=uint8_t(Read<uint8_t>(e.movement,0x594)&0xfe);
            if(!Put(e.movement,0x594,flags)){state.inPoll=false;return Fail(&e,3844,"npc_authority_original_end_frame_flag_store_failed");}
            void* movement=e.movement;engine::function<void(*)(void*,const void*)>(0x1cf69a0)(static_cast<uint8_t*>(e.movement)+0x10c0,&movement);++e.delegates;
            e.movementObservation.afterDelegate=Read<Vec>(e.root,0x320);e.movementObservation.stages|=16;
        }
        // Original time-shift handling retains its real accumulator. Common
        // debt remains an index difference; it is never discarded or padded.
        if(!Clock(e)){state.inPoll=false;return Fail(&e,3843,"npc_authority_post_frame_clock_or_links_changed");}
        e.budget=Read<float>(e.clock,0x20);
        if(e.index!=finishedIndex||Read<uint32_t>(e.clock,0x968)!=finishedSteps||e.budget!=finishedAccumulated){
            e.owner=Owner::Native;e.ready=false;e.budget=0;++e.nativeObservations;++state.nativeYields;
            e.stage="npc_authority_yielded_to_native_end_frame_clock_writer";continue;
        }
        if(!std::isfinite(e.budget)||e.budget<0){state.inPoll=false;return Fail(&e,3835,"npc_authority_native_budget_outside_supported_range");}
        if(e.budget>MaxBudget){
            state.inPoll=false;return Fail(&e,3883,"npc_authority_original_budget_exceeded_supported_real_work_capacity");
        }
    }
    state.inPoll=false;state.stage=allowAdapter?"npc_authority_candidate_polled_runtime_acceptance_unverified"
        :"npc_authority_paired_world_observed_without_adapter_simulation";return true;
}
inline int CaptureException(EXCEPTION_POINTERS* exception){
    const auto address=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=address>=engine::image?address-engine::image:address;
    state.inPoll=false;Fail(state.lastIndex<entries.size()?&entries[state.lastIndex]:nullptr,3845,"npc_authority_native_call_exception");
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Poll(float delta){__try{return PollImpl(delta);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool ObserveWorldTickOnly(float delta){__try{return PollImpl(delta,false);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool ManualAbortFailure(uint32_t code){state.failedManualAbortError=code;return false;}
inline void RecordCleanupBoundary(float currentPostWorldDelta){
    state.cleanupBoundaryAtMs=GetTickCount64();++state.cleanupBoundaryCalls;
    state.cleanupSnapshotFrame=state.frames;state.cleanupSnapshotDelta=state.frameDelta;
    state.cleanupCurrentPostWorldDelta=currentPostWorldDelta;
    state.cleanupSnapshotPending=state.frameArmed;
    for(const auto& e:entries)state.cleanupSnapshotPending=state.cleanupSnapshotPending||e.snapshot;
    state.cleanupDeltaDiffers=currentPostWorldDelta!=state.frameDelta;
}
// Cleanup proof uses the actual mapped NPC life and its already-owned
// original clock. The common producer may already have failed or changed;
// cleanup never creates a map, rewrites an origin, or requires active policy.
inline bool ManualAbortClockProof(const Entry& e){
    const auto domain=local_server_npc_time_domain::Player(e.playerIndex);
    if(!domain||domain->epoch!=e.epoch||domain->pawn!=e.pawn||domain->movement!=e.movement
       ||domain->pawnId.index!=e.pawnId.index||domain->pawnId.serial!=e.pawnId.serial
       ||domain->movementId.index!=e.movementId.index||domain->movementId.serial!=e.movementId.serial
       ||!local_server_npc_time_domain::ClockProof(*domain))return false;
    auto prediction=Read<void*>(e.movement,0x7f0);
    if(!prediction||prediction!=e.prediction||Read<void*>(e.movement,0x1530)!=e.timeShift
       ||e.clock!=static_cast<uint8_t*>(prediction)+0xf0)return false;
    auto table=Read<void**>(e.clock);auto vbtable=Read<void*>(e.clock,0x10);
    const int32_t displacement=Read<int32_t>(vbtable,4);
    return table==reinterpret_cast<void**>(engine::image+0xb600a60)
        &&Read<void*>(table,0x40)==reinterpret_cast<void*>(engine::image+0x789f280)
        &&vbtable&&displacement>=0&&displacement<=0x1000
        &&static_cast<uint32_t>(displacement)==e.baseOffset
        &&Read<uint32_t>(e.clock,size_t(displacement)+0x1c)==1
        &&Read<int32_t>(e.clock,0x24)==domain->expectedOffset&&std::isfinite(Read<float>(e.clock,0x20));
}
// Fatal-only owned scratch cleanup. The caller supplies the already proven
// CURRENT post-world boundary and retains all external human/gun/damage scope
// fences. delta is the actual current WorldTick delta; the failed transaction
// may belong to an earlier frame with a different recorded frameDelta.
// No simulated-frame/capture/ACK receipt is added, and raw/budget/queues stay as
// they were when the original transaction failed. An attempted dtor is never
// invoked again after a native exception.
inline bool AbortFailedManualTransactionsImpl(void* world,void* driver,float delta){
    const auto& sender=local_server_character_stream::GetState();
    if(!CleanupExecutionScopesIdle()||!RetirementContext()||world!=state.world||driver!=state.driver
       ||!std::isfinite(delta)||delta<0
       ||!((state.unsupported&&state.error)||(sender.unsupported&&sender.error)))return ManualAbortFailure(3885);
    RecordCleanupBoundary(delta);
    for(size_t n=1;n<entries.size();++n){auto& e=entries[n];if(!e.present)continue;
        auto* capture=local_server_character_stream::Existing(e.movement);if(!capture||!capture->manualPending)continue;
        if(!RawLinks(e)||!ManualAbortClockProof(e)||!local_server_character_stream::ValidBinding(*capture)
           ||capture->pawn!=e.pawn||!capture->observer||!e.captureGeneration
           ||capture->observer->generation!=e.captureGeneration||capture->observer->binding!=capture
           ||!capture->savedMoveConstructed||!capture->pendingIndex
           ||Read<uint32_t>(capture->savedMove,0x1c)!=capture->pendingIndex
           ||Read<void*>(capture->savedMove)!=reinterpret_cast<void*>(engine::image+0xb600b78)
           ||capture->pendingIndex>Read<uint32_t>(e.clock,size_t(e.baseOffset)+0x18))return ManualAbortFailure(3886);
        if(capture->manualAbortAttempted||(capture->savedMoveDtorAttempted&&!capture->savedMoveDtorCompleted))
            return ManualAbortFailure(3887);
        // Both flags are set before the sole original call. Any exception
        // retains the object/flags and forbids an unsafe destructor retry.
        if(!local_server_character_stream::GetState().unsupported)
            local_server_character_stream::Unsupported(3661,"character_stream_fatal_authority_manual_cleanup_fenced");
        capture->manualAbortAttempted=true;capture->savedMoveDtorAttempted=true;
        ++state.failedManualAbortDtorCalls;
        ++local_server_character_stream::state.savedMoveDtorScopes;
        __try{engine::function<void*(*)(void*,uint32_t)>(0x7891010)(capture->savedMove,0);}
        __finally{--local_server_character_stream::state.savedMoveDtorScopes;}
        capture->savedMoveDtorCompleted=true;capture->savedMoveConstructed=false;
        capture->manualPending=false;capture->pendingIndex=0;capture->manualAbortedUnverified=true;
        ++state.failedManualAborts;
    }
    if(!CleanupBoundaryReady())return ManualAbortFailure(3888);
    return true;
}
inline int ManualAbortException(EXCEPTION_POINTERS* exception){
    const auto at=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.failedManualAbortExceptionRva=at>=engine::image?at-engine::image:at;
    state.failedManualAbortError=3889;return EXCEPTION_EXECUTE_HANDLER;
}
inline bool AbortFailedManualTransactions(void* world,void* driver,float delta){
    __try{return AbortFailedManualTransactionsImpl(world,driver,delta);}
    __except(ManualAbortException(GetExceptionInformation())){return false;}
}
// Cleanup-only contract: caller is the real CURRENT post-world GT boundary.
// A fatal old pair is only abandoned as unverified metadata; its historical
// frameDelta is not passed off as the current frame's delta or completion.
// The same-delta requirement remains on healthy BeforeWorldTick/Poll only. This is
// never a successful Poll/physics receipt and cannot reopen a fatal module.
// It only removes our own incomplete pair markers once no capture is in flight.
inline bool AbandonFailedWorldFrame(void* world,void* driver,float delta){
    // Check every real scope before treating even an already-cleared pair as a
    // cleanup boundary. Never infer stack quiescence from a prior error alone.
    if(!CleanupBoundaryReady())return false;
    if(!state.initialized||!state.unsupported||!state.error
       ||!RetirementContext()||world!=state.world||driver!=state.driver
       ||!std::isfinite(delta)||delta<0)return false;
    RecordCleanupBoundary(delta);
    bool pending=state.frameArmed;for(const auto& e:entries)pending=pending||e.snapshot;
    if(!pending)return true; // Idempotent cleanup, never a physics receipt.
    state.frameArmed=false;state.frameAdapterAllowed=false;
    for(auto& e:entries)e.snapshot=false;
    ++state.unverifiedFrameAborts;state.lastFrameAbortedUnverified=true;return true;
}
// BEGIN GENERATED NPC AUTHORITY PROFILES
struct NativeProfile {uintptr_t rva;size_t bytes;uint64_t fnv;size_t relocationBegin,relocationCount;};
inline constexpr uint16_t Relocations[]{0,8,16,24,32,40,48,56,64,0,56,0,56,0,56,0,56,0,56,0,16,40,0,16,40,0,16,40,0,16,40,0,56,0,56,0,56,0,0,56,0,0};
inline constexpr NativeProfile RequiredProfiles[]{
    {0x1a7b720,196,0x4982523f72f33036ULL,0,0},
    {0x1c63870,269,0xde792136130ef719ULL,0,0},
    {0x1c77e20,158,0xc70a77070e050422ULL,0,0},
    {0x1cdc850,1396,0x74e72e0971f89c0dULL,0,0},
    {0x1cf69a0,443,0x8399700ac50f679eULL,0,0},
    {0x2830c00,5,0x8a96766f49b8bd1cULL,0,0},
    {0x3685a30,174,0x6bb2d592db3241a0ULL,0,0},
    {0x36c7580,180,0xecc03e484b1fca16ULL,0,0},
    {0x3e9ba90,284,0x78d129d21c0a5e3bULL,0,0},
    {0x3ea0510,47,0x78cbf06870977baeULL,0,0},
    {0x3ed67a0,174,0xb674d88c790b4e5eULL,0,0},
    {0x3ed83b0,1609,0x15d2d4ecc2f49cf7ULL,0,0},
    {0x3eda0d0,161,0x7a209a03d49c52eULL,0,0},
    {0x3efc820,176,0x7f1113b3f79164b2ULL,0,0},
    {0x40e6be0,8,0xb213756534b37c28ULL,0,0},
    {0x40e7460,27,0xcd276f99b3457ff1ULL,0,0},
    {0x417ea50,135,0xe5d21743d7e020a0ULL,0,0},
    {0x4188b70,38,0x8afb5ad89c10b21aULL,0,0},
    {0x418c740,298,0x880b773d5e41192aULL,0,0},
    {0x418c870,36,0x662d131ff2f84172ULL,0,0},
    {0x41a2dd0,101,0x9eb7307b1f112a31ULL,0,0},
    {0x41a2e40,1825,0x4acad26595d5af80ULL,0,0},
    {0x41a3570,2818,0x18103928d847ab43ULL,0,0},
    {0x41ac520,439,0x624d80ad54c1ad7dULL,0,0},
    {0x41dfca0,380,0xaadd7bbbb5460b42ULL,0,0},
    {0x41e2fe0,137,0xa068adc0bc778402ULL,0,0},
    {0x41e3070,146,0x1de86751b383b042ULL,0,0},
    {0x42abfd0,242,0x5995cf1fd8ef7ea8ULL,0,0},
    {0x42ac7f0,47,0xd11527a8d2997d61ULL,0,0},
    {0x42ad280,611,0x89dc6b1318db7980ULL,0,0},
    {0x48548e0,50,0x5598334b3612dde7ULL,0,0},
    {0x4856240,50,0xf557dbe1ceb3857eULL,0,0},
    {0x4856280,274,0xe81013332b88fb1fULL,0,0},
    {0x4929b30,73,0x97e911e9fecd7a6dULL,0,0},
    {0x4965380,729,0xb2a263f639c0ecf5ULL,0,0},
    {0x6c5c480,174,0xb7d7706c24b54716ULL,0,0},
    {0x6c62390,61,0xe9c7e059fc27e67fULL,0,0},
    {0x6c64b00,54,0x25fb049c322798c3ULL,0,0},
    {0x6c6b190,439,0x272b8bb73e206184ULL,0,0},
    {0x6c6f6d0,47,0xa9fe5e907852130eULL,0,0},
    {0x781fc90,198,0xda469337ca3383ccULL,0,0},
    {0x78800d0,456,0x2d3b78a5a7fd4efdULL,0,0},
    {0x78812f0,250,0xff98ec7f2255774dULL,0,0},
    {0x7883620,235,0x5b25b77e0b768b0eULL,0,0},
    {0x7883850,130,0x67aa5eea8a00eea7ULL,0,0},
    {0x7884be0,88,0xb2404ed4d6d73126ULL,0,0},
    {0x7892ad0,452,0xccaaf896bb3ec781ULL,0,0},
    {0x7892e10,142,0x773b71bd06054ce5ULL,0,0},
    {0x7894ab0,2588,0x31679692b8536d50ULL,0,0},
    {0x78954d0,295,0xf4dc48561a4f9c5cULL,0,0},
    {0x789f0e0,407,0xf2022bfb32d61cacULL,0,0},
    {0x789f280,139,0xf59a21805737c38dULL,0,0},
    {0x789f310,302,0x1b0456add87ddd53ULL,0,0},
    {0xb600a60,72,0xc8e1cdabc76c79d9ULL,0,9},
    {0xb4dd4f0,64,0x57a0f03a952829c6ULL,9,2},
    {0xc5358c8,64,0x89a7ea93cc11fd2ULL,11,2},
    {0xabb86d0,64,0xcca00599fb004e5bULL,13,2},
    {0xabbd980,64,0x304f91f394d28353ULL,15,2},
    {0xabb6e60,64,0x748406ee3f0d242bULL,17,2},
    {0xd3ce390,64,0x9b4dd7e680e0723dULL,19,3},
    {0xd659760,64,0x38bf9a2d9a37577cULL,22,3},
    {0xd65a490,64,0x76c5ea2b098a141aULL,25,3},
    {0xd3a9e70,64,0x91f986ad7f9e8129ULL,28,3},
    {0xab14200,64,0x1662f501e2280b88ULL,31,2},
    {0xb4d8e20,64,0x66d98eead274a94ULL,33,2},
    {0xb4df260,64,0x66d98eead274a94ULL,35,2},
    {0xbd1c980,64,0xbb23ed304d6bf729ULL,37,1},
    {0xaa89cd0,64,0xace2749a65dc3c82ULL,38,2},
    {0xa9fba88,8,0x94a919decd2465faULL,40,1},
    {0xa9fbcc0,8,0x20d0ed1cc9008ca7ULL,41,1},
};
inline bool EntriesMatch(){
    if(!engine::image)return false;
    for(const auto& profile:RequiredProfiles){
        const auto* source=reinterpret_cast<const uint8_t*>(engine::image+profile.rva);
        if(!original_players::Readable(source,profile.bytes)){state.profileFailureRva=static_cast<uint32_t>(profile.rva);return false;}
        std::vector<uint8_t> normalized;
        if(profile.relocationCount){
            normalized.assign(source,source+profile.bytes);
            for(size_t n=0;n<profile.relocationCount;++n){
                const size_t at=Relocations[profile.relocationBegin+n];uintptr_t value=0;
                std::memcpy(&value,normalized.data()+at,sizeof(value));value=value-engine::image+0x140000000ULL;
                std::memcpy(normalized.data()+at,&value,sizeof(value));
            }source=normalized.data();
        }
        uint64_t actual=14695981039346656037ULL;
        for(size_t n=0;n<profile.bytes;++n)actual=(actual^source[n])*1099511628211ULL;
        if(actual!=profile.fnv){state.profileFailureRva=static_cast<uint32_t>(profile.rva);return false;}
    }return true;
}
// END GENERATED NPC AUTHORITY PROFILES

}
