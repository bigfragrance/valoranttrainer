#pragma once
#include "original_players.h"
#include "local_server_empty_bootstrap_probe.h"
#include "local_server_human_funded_boundary_probe.h"
#include "native_input_checkpoint_frame.h"
#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <cstdio>

// Static contract: work/local-server-playable/input-authority/report.md.
// This is an explicit missing local-authority adapter, runtime_unproven until
// the separate host/client receipts establish each gate. It hooks only the
// owned host Pawn's empty ServerReceiveNetworkStream implementation. The
// original install, image code, class defaults, roles and LocalPlayer stay
// untouched. Native physics, input codecs and own correction/replay are reused.
namespace local_server_input {
struct Config {
    uint32_t tickRate=128,maxPacketBytes=8192,maxPacketBits=65535;
    uint32_t maxMovesPerPacket=16,queueCapacity=128,maxFutureSteps=128;
    uint32_t maxStepsPerPoll=32,maxInputEvents=100,maxWatermarks=128,maxPddPlayerIds=32;
    // A bounded funded-work buffer and long-delta diagnostic threshold. Real
    // post-origin elapsed time is retained in the double debt ledger, not cut.
    float maxBufferedSeconds=0.25f,maxPollDeltaSeconds=0.25f;
    float lostMovementHoldDelaySeconds=0.25f;
    double maxIntentMagnitude=2.0,gradualCorrectionDistance=0.5,hardCorrectionDistance=20.0;
};
inline constexpr Config Configuration{};
struct NativeProfile { uint32_t rva,bytes;uint64_t fnv; };
inline constexpr NativeProfile RequiredProfiles[]{
    {0x18a3a00,3,0x69bc481c23de2d4dULL},
    {0x1a5d990,286,0x938cfbf14f4b138fULL},
    {0x1a5dab0,70,0x47a2d5def94c14ecULL},
    {0x1a5d4c0,243,0xda9e9f3e140a96ddULL},
    {0x7893760,334,0xf9b43a29c6ebd304ULL},
    {0x70a9e50,763,0x6a75c16f2765cb48ULL},
    {0x70a9dd0,123,0xe8c5dcc9b399178aULL},
    {0x70aa150,169,0xa68f4cc876ab8d50ULL},
    {0x70a80b0,770,0xe97523ade31fab94ULL},
    {0x7883620,235,0x5b25b77e0b768b0eULL},
    {0x78954d0,295,0xf4dc48561a4f9c5cULL},
    {0x7894a20,131,0x8e5838acb2d670e1ULL},
    {0x7892e10,142,0x773b71bd06054ce5ULL},
    {0x789f0e0,407,0xf2022bfb32d61cacULL},
    {0x789f310,302,0x1b0456add87ddd53ULL},
    {0x3efc0d0,1867,0xec7f862288814032ULL},
    {0x1cf69a0,443,0x8399700ac50f679eULL},
    {0x789d6f0,458,0xf715377179fd84baULL},
    {0x41a8150,2118,0x55cc4089135d4e57ULL},
    {0x787c310,220,0x45017c2aaf6a88adULL},
    {0x787c3f0,251,0x8ca09b346b69eda4ULL},
    {0x6c68f10,402,0x2095ab47a3557630ULL},
    {0x6c68cc0,584,0x53c4353a4501ec4dULL},
    {0x4182420,2069,0x227d36c2bf5a9770ULL},
    {0x4189be0,47,0xa32cce441decb5a4ULL},
    {0x7883790,179,0x2d0d6c3cb3e60e48ULL},
    {0x3efc820,176,0x7f1113b3f79164b2ULL},
    {0x6f4f6c0,986,0x75e025d9bff582adULL},
    {0x7883850,130,0x67aa5eea8a00eea7ULL},
    {0x6c64b00,54,0x25fb049c322798c3ULL},
    {0x6c6f6c0,8,0x9faeae9496aeeb81ULL},
    {0x1a7b720,196,0x4982523f72f33036ULL},
    {0x1c63870,269,0xde792136130ef719ULL},
    {0x1c77e20,158,0xc70a77070e050422ULL},
    {0x1cf35b0,1571,0x40614eae57888396ULL},
    {0x1d4cfa0,1562,0xb050d7efb4cf1a25ULL},
    {0x70aa200,37,0x1067dd10c736edaeULL},
    {0x787f820,450,0xc0e54150052a72f3ULL},
    {0x789e910,656,0x166cc623dda3986ULL},
    {0x789edd0,32,0x649dd97431ecfca8ULL},
    {0x789ee20,24,0xbec1e00a7c6c83f2ULL},
    {0x789ee40,26,0x9031464499c99ce7ULL},
    {0x789f090,25,0xe9040b5725abd43aULL},
    {0x789f0b0,29,0x212a448438acad98ULL},
    {0x7890fa0,98,0x9b94cb0594ed7fa4ULL},
    {0x7891010,48,0xe8abc591b29ab71eULL},
    {0x78913a0,318,0x453ad149ae134966ULL},
    {0x7891280,238,0x862d6647c415794ULL},
    {0x7897e70,59,0x753f59865d822fd8ULL},
    {0x789ee60,550,0x909a57b98d314de2ULL},
    {0x6c84330,248,0x554bb09563a067edULL},
    {0x78a1f30,40,0xd89daa68719816f0ULL},
    {0x789f7b0,32,0x61323e7251773601ULL},
    {0x789f7d0,28,0xcffc79c87f2ee8dbULL},
    {0x789e6c0,262,0xe9b02e3ff4dd30cdULL},
    {0x7856ca0,77,0x99260be4810a2a20ULL},
    {0x7856c30,105,0x5094670251d736ecULL},
    {0x70a74e0,3012,0xb8866d557299ab54ULL},
    {0x70a4530,1859,0x6256e5a01ed4f880ULL}, // checkpoint frame/state wire contract
    {0x70a9210,1668,0xb78fc876a8b23752ULL}, // checkpoint frame/state wire contract
    {0x70a9b60,622,0xe31b79d1aac5930aULL}, // checkpoint frame/state wire contract
    {0x70a87f0,162,0xd905cf93c2bb36b6ULL},
    {0x70a90b0,344,0x6890be87a0fdb6ebULL},
    {0x70fa910,539,0x72500757cacd5f9fULL},
    {0x70fa670,469,0xb09df98847fe568dULL},
    {0x6499330,174,0x2423cf21a48d620cULL},
    {0x645c660,174,0x8c1cf8478d56bc9dULL},
    {0x70a9910,383,0x4fd51381a40efb9eULL},
    {0x7894ab0,2588,0x31679692b8536d50ULL},
    {0x78999b0,1347,0xc380b322eb918c24ULL},
    {0x785e8c0,98,0x32e6b03df29b1745ULL},
};
inline constexpr uintptr_t PawnTableRva=0xb4dc550,MovementTableRva=0xb4ecc50;
inline constexpr size_t PawnTableEntries=0xae0/8,StreamSlot=0xa58;
inline constexpr size_t CrouchHeldOffset=0x1538;
inline constexpr uint8_t CrouchHeldMask=1,CompressedCrouchMask=2;
inline constexpr int32_t NativeConnectionOpen=3;
struct CrouchNativeName {uint32_t comparison=0,display=0,number=0;};
static_assert(sizeof(CrouchNativeName)==12);
struct WeakIdentity {int32_t index=-1,serial=0;};
static_assert(sizeof(WeakIdentity)==8);
using NativeView=engine::Vec*(*)(void*,engine::Vec*);
using NativeTakeDamage=float(*)(void*,float,const void*,void*,void*);

struct ComponentDataStream {
    const uint8_t* data=nullptr;int32_t bytes=0,capacity=0;
    uint8_t unusedBits=0,padding[7]{};
};
struct alignas(8) Move {
    uint8_t type=0,flags=0,roll=0,mode=0,pad0[4]{};
    engine::Vec intent{};uint32_t index=0,pad1=0;
    engine::Vec claimedLocation{};float crouch=0;uint32_t pitchYaw=0;
    uint8_t firstStep=0,pad2[7]{};void* base=nullptr;
    uint32_t baseBone[3]{};float timeShift=0;
    uint8_t remoteFlag=0,pad3[7]{};engine::Vec remoteVelocity{};uint64_t tail=0;
};
struct Ack { int32_t eventId=-1;uint32_t index=0; };
struct alignas(8) Gmc { engine::Vec difference{},velocity{};uint8_t nativeTail[32]{}; };
struct GradualAck { Ack ack;Gmc correction; };
struct JumpVariables { uint8_t bytes[52]{}; };
struct AdjustmentDebug { uint8_t reason=0,pad[3]{};uint32_t correctionId=0; };
struct alignas(8) HardCorrection {
    uint32_t index=0,pad0=0;engine::Vec location{};
    float crouch=0;uint32_t pad1=0;engine::Vec velocity{};
    void* base=nullptr;uint32_t bone[3]{};
    uint8_t hasBase=0,baseRelative=0,mode=0,pad2=0;
    JumpVariables jump;AdjustmentDebug debug;
    uint8_t teleport=0,pad3[3]{};
};
static_assert(sizeof(ComponentDataStream)==24&&sizeof(Move)==0x90);
static_assert(offsetof(Move,intent)==8&&offsetof(Move,index)==0x20&&offsetof(Move,claimedLocation)==0x28);
static_assert(offsetof(Move,pitchYaw)==0x44&&offsetof(Move,base)==0x50&&offsetof(Move,timeShift)==0x64);
static_assert(offsetof(Move,remoteVelocity)==0x70&&sizeof(Ack)==8&&sizeof(GradualAck)==88);
static_assert(sizeof(HardCorrection)==152&&offsetof(HardCorrection,jump)==0x58);
static_assert(offsetof(HardCorrection,debug)==0x8c&&offsetof(HardCorrection,teleport)==0x94);
// Runtime UFunction.ParmsSize is the logical last-field end. Generated native
// sizeof(Params) additionally includes three bytes of trailing alignment.
inline constexpr uint16_t HardCorrectionLogicalBytes=0x95;

// A callback receives real host state on each accepted fixed step. Capture
// modules can use their own original FSavedMove SetMoveFor/PostUpdate around
// physics. Null callbacks do not assert that remote state emission is complete.
using StepCallback=bool(*)(void* pawn,void* movement,float dt,uint32_t index,const engine::Vec* intent);
inline StepCallback beforeStep=nullptr,afterStep=nullptr;
inline void SetStepCallbacks(StepCallback before,StepCallback after){beforeStep=before;afterStep=after;}
struct QueueEntry { bool present=false;uint32_t clientIndex=0;Move move; };
struct Watermark { uint32_t index=0;int32_t eventId=-1; };
struct BarrierTime {bool present=false;int32_t eventId=-1;float clientSeconds=0;};
struct alignas(16) MappingClockStorage {uint8_t data[0x9b0]{};};
static_assert(sizeof(MappingClockStorage)==0x9b0&&alignof(MappingClockStorage)==16);
struct SequencePacket {
    uint64_t packet=0;uint32_t count=0,first=0,last=0,expected=0,clock=0,consumed=0,queued=0;
    uint8_t firstStep=0;float timeShift=0;
};
struct SequenceFault {
    bool captured=false;uint32_t error=0,ordinal=0,index=0,expected=0,clock=0,consumed=0,lastReceived=0;
    uint32_t packetCount=0,queueCount=0,queueMinimum=0,queueMaximum=0,readerBit=0;
    uint32_t mappedIndex=0;int32_t ahead=0,clockOffset=0;uint64_t packet=0;
    float budgetSeconds=0,clockAccumulated=0;double hostElapsedSeconds=0;
    std::array<Move,16> moves{};
};
struct ClientClockDiagnostic {
    bool observed=false,localOwner=false,profilesVerified=false,roundtrip=false;
    uint32_t thread=0,rawIndex=0;int32_t timestep=0;
    uint64_t observations=0;
    void *pawn=nullptr,*movement=nullptr,*prediction=nullptr,*clock=nullptr;
    float hostSecondsBefore=0,clientSecondsAfter=0,hostSecondsAfter=0;
    const char* stage="network_input_client_clock_not_observed";
};
struct InputWireTrace {
    uint64_t packet=0;
    bool headerKnown=false,forwardGap=false;
    uint8_t nativeResult=0;
    int32_t firstEventId=-1,baseBefore=-1,countBefore=0,baseAfter=-1,countAfter=0;
};
struct InputEventDiagnostic {int32_t eventId=-1;std::array<uint8_t,40> bytes{};};
struct InputApplyDiagnostic {
    uint64_t attempt=0;
    uint32_t hostIndex=0,prefix=0,copiedEvents=0;
    float hostSeconds=0;
    int32_t base=-1,count=0,eligible=-1,appliedBefore=-1,appliedAfter=-1;
    int32_t acknowledgedCorrectionIndex=-1,authCorrectionIndex=-1,clientCorrectionIndex=-1,originCorrectionIndex=-1;
    uint8_t correctionResetPending=0,inventoryCorrectionState=0,initialCorrectionSent=0,initialCorrectionProcessed=0;
    void *inventory=nullptr,*reportedTarget=nullptr,*clientPredicted=nullptr,*current=nullptr,*desired=nullptr,*corrected=nullptr;
    bool nativeAttemptCacheReadable=false;
    std::array<uint8_t,40> nativeAttempt{};
    std::array<InputEventDiagnostic,16> events{};
};
struct CorrectionPublication {
    bool observed=false,consumed=false;
    int32_t authIndex=-1,originIndex=-1;
    uint64_t observations=0,packet=0;
    void *inventory=nullptr,*corrected=nullptr;
    WeakIdentity inventoryIdentity{},correctedIdentity{};
};
// Diagnostic-only bounded copies of a fully native-decoded scratch stream.
// These records never authorize cancellation or enter native Apply.
struct InputCheckpointDecodeDiagnostic {
    bool captured=false,buffersReadable=false;
    uint64_t packet=0;
    uint64_t epoch=0;
    int32_t authIndex=-1,originIndex=-1,acknowledgedIndex=-1;
    int32_t wireCount=0,retainedCount=0;
    uint32_t copiedWire=0,copiedRetained=0;
    InputWireTrace intake{};
    std::array<InputEventDiagnostic,100> wire{},retained{};
};
struct InputCancellationDiagnostic {
    uint64_t packet=0,passedMask=0;
    int32_t firstEventId=-1,oldBase=-1,oldCount=0,newBase=-1,newCount=0;
    int32_t authIndex=-1,originIndex=-1,acknowledgedIndex=-1;
    uint32_t cancelledCount=0;
    native_input_checkpoint_frame::Proof replacementFrame{};
    void *reported=nullptr,*corrected=nullptr;
    WeakIdentity reportedIdentity{};
    const char* stage="network_input_correction_cancellation_not_observed";
};
enum OwnedCheck : uint8_t {
    OwnersPresent,PCPlayerConnection,ConnectionPC,ConnectionOwningActor,PCPawn,PawnController,
    PlayerStateAgreement,PlayerStateOwner,PawnAuthorityRole,PawnAutonomousRemoteRole,PCAuthorityRole,
    MovementOwner,MovementCharacter,InputOwner,InputPawn,WorldGameState,PawnPrivateVptr,
    MovementPhysicsEntry,MovementModeEntry,OriginalWeakIdentities,OwnedCheckCount
};
struct OwnedCheckName { OwnedCheck check;const char* name; };
inline constexpr OwnedCheckName OwnedChecks[]{
    {OwnersPresent,"owners_present"},{PCPlayerConnection,"pc_player_is_connection"},
    {ConnectionPC,"connection_player_controller"},{ConnectionOwningActor,"connection_owning_actor"},
    {PCPawn,"pc_pawn"},{PawnController,"pawn_controller"},
    {PlayerStateAgreement,"pc_pawn_player_state_agreement"},{PlayerStateOwner,"player_state_owner"},
    {PawnAuthorityRole,"pawn_local_role_authority"},{PawnAutonomousRemoteRole,"pawn_remote_role_autonomous"},
    {PCAuthorityRole,"pc_local_role_authority"},{MovementOwner,"movement_owner"},
    {MovementCharacter,"movement_character_owner"},{InputOwner,"input_owner"},
    {InputPawn,"input_pawn"},{WorldGameState,"world_game_state"},
    {PawnPrivateVptr,"pawn_private_vptr"},{MovementPhysicsEntry,"movement_physics_entry"},
    {MovementModeEntry,"movement_packed_mode_entry"},
    {OriginalWeakIdentities,"original_weak_identity_agreement"},
};
inline constexpr uint64_t OwnedCheckBit(OwnedCheck check){return uint64_t(1)<<check;}
inline constexpr uint64_t AllOwnedChecks=(uint64_t(1)<<OwnedCheckCount)-1;
struct State {
    bool installed=false,profilesVerified=false,ownedLinks=false,privateTableReadOnly=false;
    bool runtime_unproven=true,soleQueueConsumer=true,inReceive=false,inPoll=false;
    bool restartRequired=false,forceHardCorrection=false;
    bool disconnected=false,connectionStateReadable=false;
    bool epochRetired=false,identitiesCaptured=false,weakIdentitiesVerified=false,incomingEventsConstructed=false;
    bool retiredSourceRestored=false,retiredPawnExpired=false,viewObserverInstalled=false;
    bool takeDamageObserverInstalled=false;
    bool crouchMetadataVerified=false,crouchReadbackVerified=false;
    int32_t actualConnectionState=-1;
    uint32_t error=0,profileFailureRva=0,gameThread=0,clockBaseOffset=0;
    uint32_t actualThreadId=0,playerRegistryThreadId=0;
    bool threadMatches=false;
    uint64_t ownedPassedMask=0,ownedFailedMask=0,ownedRequiredMask=0;
    uint64_t ownedPassedMaskAtInstall=0,ownedFailedMaskAtInstall=0;
    uint8_t actualPawnLocalRole=0,actualPawnRemoteRole=0,actualPCLocalRole=0,actualPCRemoteRole=0;
    void *actualPCPlayer=nullptr,*actualConnectionPC=nullptr,*actualConnectionOwningActor=nullptr;
    void *actualPCPawn=nullptr,*actualPawnController=nullptr,*actualPCState=nullptr,*actualPawnState=nullptr,*actualStateOwner=nullptr;
    void *actualMovementOwner=nullptr,*actualMovementCharacter=nullptr,*actualInputOwner=nullptr,*actualInputPawn=nullptr;
    void *actualWorldGameState=nullptr,*actualMovementPhysicsEntry=nullptr,*actualMovementModeEntry=nullptr;
    void **actualPawnTable=nullptr,**actualMovementTable=nullptr;
    void **pawnTableAfterInstall=nullptr,**movementTableAtInstall=nullptr;
    const wchar_t* failedRPCName=nullptr;
    uint16_t expectedRPCBytes=0,actualRPCBytes=0;
    uint32_t actualRPCFlags=0;
    uintptr_t expectedRPCExecRva=0,actualRPCExecRva=0;
    uint32_t queueCount=0,watermarkCount=0,lastReceivedIndex=0,lastConsumedIndex=0,lastAcknowledgedIndex=0;
    uint32_t clockIndex=0,clockStep=0,lastPacketBytes=0,lastPacketBits=0,lastInputEndBit=0;
    uint32_t lastReaderBit=0,pendingEvents=0,correctionId=0;
    uint32_t pddPlayerCount=0;
    uint8_t pddTeamPresent=0,pddTeamBit=0,pddAttributeFlag=0,pddStatus=0;
    int32_t latestAppliedEventId=-1,lastAckEventId=-1;
    uint64_t receivedPackets=0,acceptedPackets=0,rejectedPackets=0,decodedMoves=0,uniqueMoves=0;
    uint64_t duplicateMoves=0,physicsCalls=0,physicsCompleted=0,clockAdvances=0,frameScopes=0;
    uint64_t inputApplyCalls=0,inputEventsApplied=0,beforeFrameCalls=0,delegateCalls=0;
    uint64_t normalAcks=0,gradualAcks=0,hardCorrections=0,eventOnlyAcks=0;
    bool teleportMetadataVerified=false,teleportCorrectionInFlight=false;
    uint8_t teleportSinceMove=0,teleportSinceCorrection=0,lastCorrectionTeleport=0,teleportClearReadback=255;
    uint64_t teleportFlagSamples=0,teleportCorrectionsSubmitted=0,teleportFlagConsumptions=0;
    uint64_t teleportFlagAlreadyConsumed=0,teleportSourceEpoch=0;
    uint32_t teleportCorrectionClientIndex=0;
    uint64_t polls=0,budgetCaps=0,queueWaitPolls=0,pddHeaderOnlyPackets=0,pddAbsentPackets=0,pddPayloadPackets=0;
    uint64_t closedConnectionPolls=0,closedConnectionPackets=0;
    uint64_t epochNumber=0,retiredEpochs=0,retiredPacketsDiscarded=0,retiredQueuedMoves=0,retiredInputEvents=0;
    uint64_t retirementCalls=0,streamClearCalls=0,streamResetCalls=0,viewObserverInstalls=0,viewObserverRestores=0;
    uint32_t sequencePacketCount=0;
    bool firstMoveBatchCaptured=false;
    bool wireAligned=false,lastKnownInputValid=false,timestepRpcSubmitted=false,seedTeleportPending=false;
    bool mappingClockConstructed=false;
    int32_t clockIndexOffset=0,actualClockIndexOffset=0,timestepRpcValue=0;
    uint32_t mappingClockRawIndex=0,mappingClockStep=0,mappingClockBaseOffset=0;
    uint32_t wireOriginClientIndex=0,lastObservedClientIndex=0,lastReceivedClientIndex=0,unobservedInitialClientSteps=0;
    uint32_t seedRawBefore=0,seedRawAfter=0,seedHistoryAfter=0,seedPendingAfter=0,nativeFrameSteps=0;
    uint64_t nativeSeedCalls=0,timestepRpcCalls=0,observedCommandFrames=0,heldStateFrames=0,heldGapFrames=0,heldSilenceFrames=0;
    uint64_t clockResynchronizations=0,expiredClientMoves=0;
    uint32_t heldSinceObserved=0,lastWireSyncClientIndex=0;
    uint64_t mappedBarrierEvents=0,nativeFrameStepResets=0;
    uint64_t mappingClockAdvances=0,mappingClockDestructions=0;
    float seedClientSeconds=0,seedMappedHostSeconds=0,seedInverseClientSeconds=0;
    double lastFreshMovementHostSeconds=0;
    Move lastKnownInput{};
    std::array<BarrierTime,100> barrierTimes{};
    ClientClockDiagnostic clientClock{};
    InputWireTrace lastInputWire{};
    uint32_t inputWireTraceCount=0;
    uint64_t nativeInputWireGaps=0,inputBlockedApplyAttempts=0;
    std::array<InputWireTrace,32> inputWireTraces{};
    InputApplyDiagnostic inputApplyDiagnostic{};
    bool correctionMetadataVerified=false;
    uint64_t nativeCorrectionCancellations=0,nativeCorrectionCancelledEvents=0;
    uint64_t nativeCheckpointDecodes=0,nativeCheckpointDestructions=0;
    int32_t lastCancelledCorrectionIndex=-1;
    CorrectionPublication correctionPublication{};
    InputCancellationDiagnostic correctionCancellation{};
    InputCheckpointDecodeDiagnostic firstEligibleCheckpointDecode{},latestCheckpointDecode{};
    SequencePacket firstMoveBatch{};
    std::array<Move,16> firstMoveBatchRecords{};
    std::array<SequencePacket,32> sequencePackets{};
    SequenceFault sequenceFault{};
    uint32_t firstMovePacketBytes=0,firstMovePacketBits=0,sequenceFaultPacketBytes=0,sequenceFaultPacketBits=0;
    std::array<uint8_t,8192> firstMovePacketData{},sequenceFaultPacketData{};
    std::array<uint8_t,8192> failedDecoderPacketData{};
    uint32_t failedDecoderPacketBytes=0,failedDecoderPacketBits=0,failedDecoderResult=0;
    uint64_t failedDecoderPacket=0,failedDecoderStart=0,failedDecoderPosition=0;
    bool failedDecoderReaderError=false;
    uint64_t crouchInputCommands=0,crouchInputTransitions=0,crouchAuthorityWrites=0;
    uint64_t crouchGetterCalls=0,crouchReadbackFailures=0,originalCompressedFlagCalls=0;
    uint8_t crouchBeforeRaw=0,crouchDesiredRaw=0,crouchAfterRaw=0,crouchRequested=0,crouchGetterValue=255;
    uint8_t crouchFieldSize=0,crouchByteOffset=0,crouchByteMask=0,crouchFieldMask=0;
    int32_t crouchFieldOffset=-1,crouchArrayDim=0,crouchElementSize=0,crouchTypeSize=0;
    uint64_t crouchCastFlags=0;
    uintptr_t exceptionRva=0;
    float hostBudgetSeconds=0,lastPhysicsDelta=0,clockAccumulated=0;
    double droppedHostSeconds=0,lastClaimError=0,hostElapsedSeconds=0;
    bool hostTimeOriginStarted=false,hostClockAccountingStarted=false;
    uint64_t hostFundingCalls=0,hostCatchupPolls=0,hostWorkQuotaHits=0,hostDebtRecoveryTransfers=0;
    uint32_t lastHostPollSteps=0;
    float lastHostDeltaSeconds=0,lastHostFundingSeconds=0;
    double hostUnfundedSeconds=0,hostFundedWorkSeconds=0,hostServicedSeconds=0,hostCreditedSeconds=0;
    double hostPreOriginSeconds=0,hostOriginCreditSeconds=0,hostOriginExcludedSeconds=0,hostPostOriginSeconds=0;
    double hostDebtPeakSeconds=0,hostUnprocessedPeakSeconds=0,hostCatchupServicedSeconds=0;
    double hostDebtTransferredBacklogSeconds=0;
    double hostRetiredFundedSeconds=0,hostRetiredUnfundedSeconds=0;
    double hostCreditConservationError=0,hostTimeConservationError=0;
    double nativeFundingRoundoffSeconds=0,nativeAdvanceAdjustmentSeconds=0,nativeExternalAccumulatorSeconds=0;
    double nativePostAdvanceAccumulatorSeconds=0;
    double nativeInitialAccumulatorSeconds=0;
    float nativeAccumulatorAtPollEnd=0;
    engine::Vec positionBefore{},positionAfter{},velocityAfter{},lastClaim{},lastIntent{};
    engine::Vec installedPosition{},installedVelocity{};
    const char* stage="network_input_not_installed";
    void *world=nullptr,*pc=nullptr,*pawn=nullptr,*connection=nullptr,*movement=nullptr,*inputState=nullptr;
    void *gameState=nullptr,*fixedStep=nullptr,*clock=nullptr,*timeShift=nullptr,*nativeNetDriver=nullptr;
    void* mappingClock=nullptr;
    void *ackFunction=nullptr,*gradualFunction=nullptr,*hardFunction=nullptr;
    void* timestepFunction=nullptr;
    void *crouchGetterFunction=nullptr,*crouchType=nullptr,*crouchField=nullptr,*crouchFieldClass=nullptr;
    void *crouchNativeBoolClass=nullptr,*actualMovementPawnOwner=nullptr;
    void *playerState=nullptr,*retiredWeakResolvedPawn=nullptr,*lastRetiredPawn=nullptr,*viewObserver=nullptr;
    void* takeDamageObserver=nullptr;
    WeakIdentity worldIdentity{},pcIdentity{},pawnIdentity{},connectionIdentity{},playerStateIdentity{},movementIdentity{},inputIdentity{};
    void **sourcePawnTable=nullptr,**privatePawnTable=nullptr;
    std::array<QueueEntry,128> queue{};
    std::array<Watermark,128> watermarks{};
    std::array<int32_t,32> pddReportedPlayerIds{};
    alignas(8) uint8_t incomingEvents[0x748]{};
};
inline State state;
inline MappingClockStorage mappingClockStorage;
struct RetiredBinding {void* pawn=nullptr;WeakIdentity identity{};void* originalReceiver=nullptr;};
struct EpochLedger {
    bool anchored=false;uintptr_t image=0;uint32_t gameThread=0,blockedError=0;
    const char* blockedStage=nullptr;
    void *world=nullptr,*pc=nullptr,*playerState=nullptr,*connection=nullptr,*lastRetiredPawn=nullptr;
    WeakIdentity worldIdentity{},pcIdentity{},playerStateIdentity{},connectionIdentity{},lastRetiredIdentity{};
    void** reusablePawnTable=nullptr;
    uint64_t started=0,retired=0,latePackets=0,discardedMoves=0,discardedEvents=0;
    std::vector<RetiredBinding> bindings;
};
inline EpochLedger epochs;
inline const State& GetState(){return state;}
inline void RequestHardCorrection(){state.forceHardCorrection=true;}
template<class T> inline T Read(const void* p,size_t off=0){return original_players::Value<T>(p,off);}
template<class T> inline void Put(void* p,size_t off,const T& value){std::memcpy(static_cast<uint8_t*>(p)+off,&value,sizeof(value));}
inline bool Fail(uint32_t code,const char* stage){state.error=code;state.stage=stage;state.restartRequired=true;return false;}
inline bool Wait(const char* stage){state.stage=stage;return false;}
inline bool Thread(){
    state.actualThreadId=GetCurrentThreadId();
    state.playerRegistryThreadId=original_players::gameThread;
    state.threadMatches=state.gameThread&&state.actualThreadId==state.gameThread;
    return state.threadMatches;
}
inline uint32_t NextIndex(uint32_t value){++value;return value?value:1;}
inline bool Finite(const engine::Vec& v){return original_players::Finite(v);}
template<class F> inline F Virtual(void* object,size_t slot){return reinterpret_cast<F>(Read<void*>(Read<void**>(object),slot));}
inline bool Profile(uintptr_t rva,uint32_t bytes,uint64_t expected){
    const auto data=reinterpret_cast<const uint8_t*>(engine::image+rva);
    if(!original_players::Readable(data,bytes))return false;
    uint64_t hash=14695981039346656037ULL;
    for(uint32_t i=0;i<bytes;++i){hash^=data[i];hash*=1099511628211ULL;}
    return hash==expected;
}
inline bool VerifyProfiles(){
    for(const auto& p:RequiredProfiles)if(!Profile(p.rva,p.bytes,p.fnv)){
        state.profileFailureRva=p.rva;return Fail(3201,"network_input_native_profile_mismatch");
    }
    state.profilesVerified=true;return true;
}
inline void* ResolveWeak(const WeakIdentity& identity){
    return identity.index>=0&&identity.serial>0?
        engine::function<void*(*)(const WeakIdentity*)>(0x1d4cfa0)(&identity):nullptr;
}
inline bool CaptureWeak(void* object,WeakIdentity& identity){
    if(!original_players::Readable(object,0x30))return false;
    identity.index=Read<int32_t>(object,0xc);
    if(identity.index<0)return false;
    identity.serial=engine::function<int32_t(*)(void*,int32_t)>(0x1cf35b0)
        (reinterpret_cast<void*>(engine::image+0xd7b87c0),identity.index);
    return identity.serial>0&&ResolveWeak(identity)==object;
}
inline bool SameWeak(const WeakIdentity& a,const WeakIdentity& b){return a.index==b.index&&a.serial==b.serial&&a.serial>0;}
inline void EpochCounters(){
    state.epochNumber=epochs.started;state.retiredEpochs=epochs.retired;
    state.retiredPacketsDiscarded=epochs.latePackets;state.retiredQueuedMoves=epochs.discardedMoves;
    state.retiredInputEvents=epochs.discardedEvents;state.lastRetiredPawn=epochs.lastRetiredPawn;
}
inline bool CaptureEpochIdentities(){
    state.playerState=Read<void*>(state.pc,0x470);
    state.identitiesCaptured=CaptureWeak(state.world,state.worldIdentity)&&CaptureWeak(state.pc,state.pcIdentity)
        &&CaptureWeak(state.pawn,state.pawnIdentity)&&CaptureWeak(state.connection,state.connectionIdentity)
        &&CaptureWeak(state.playerState,state.playerStateIdentity)&&CaptureWeak(state.movement,state.movementIdentity)
        &&CaptureWeak(state.inputState,state.inputIdentity);
    return state.identitiesCaptured||Fail(3270,"network_input_original_epoch_weak_identity_capture_failed");
}
inline bool OwnedLinks(){
    const auto pc=state.pc,pawn=state.pawn,connection=state.connection;
    // Capture every original predicate before combining them. A failed early
    // link must not suppress the remaining actual values in the crash receipt.
    state.actualPCPlayer=Read<void*>(pc,0x510);
    state.actualConnectionPC=Read<void*>(connection,0x38);
    state.actualConnectionOwningActor=Read<void*>(connection,0xa0);
    state.actualPCPawn=Read<void*>(pc,0x4b0);state.actualPawnController=Read<void*>(pawn,0x4a0);
    state.actualPCState=Read<void*>(pc,0x470);state.actualPawnState=Read<void*>(pawn,0x488);
    state.actualStateOwner=Read<void*>(state.actualPCState,0x210);
    state.actualPawnLocalRole=Read<uint8_t>(pawn,0x224);state.actualPawnRemoteRole=Read<uint8_t>(pawn,0x128);
    state.actualPCLocalRole=Read<uint8_t>(pc,0x224);state.actualPCRemoteRole=Read<uint8_t>(pc,0x128);
    state.actualMovementOwner=Read<void*>(state.movement,0xc8);
    state.actualMovementCharacter=Read<void*>(state.movement,0x1a8);
    state.actualInputOwner=Read<void*>(state.inputState,0xc8);state.actualInputPawn=Read<void*>(state.inputState,0x148);
    state.actualWorldGameState=Read<void*>(state.world,0x178);
    state.actualPawnTable=Read<void**>(pawn);state.actualMovementTable=Read<void**>(state.movement);
    state.actualMovementPhysicsEntry=Read<void*>(state.actualMovementTable,0xa98);
    state.actualMovementModeEntry=Read<void*>(state.actualMovementTable,0x6a0);
    uint64_t mask=0;
    const auto mark=[&](OwnedCheck check,bool passed){if(passed)mask|=OwnedCheckBit(check);};
    mark(OwnersPresent,pc&&pawn&&connection);
    mark(PCPlayerConnection,state.actualPCPlayer==connection);
    mark(ConnectionPC,state.actualConnectionPC==pc);
    mark(ConnectionOwningActor,state.actualConnectionOwningActor==pc);
    mark(PCPawn,state.actualPCPawn==pawn);mark(PawnController,state.actualPawnController==pc);
    mark(PlayerStateAgreement,state.actualPCState==state.actualPawnState);
    mark(PlayerStateOwner,state.actualStateOwner==pc);
    mark(PawnAuthorityRole,state.actualPawnLocalRole==3);
    mark(PawnAutonomousRemoteRole,state.actualPawnRemoteRole==2);
    mark(PCAuthorityRole,state.actualPCLocalRole==3);
    mark(MovementOwner,state.actualMovementOwner==pawn);mark(MovementCharacter,state.actualMovementCharacter==pawn);
    mark(InputOwner,state.actualInputOwner==pawn);mark(InputPawn,state.actualInputPawn==pawn);
    mark(WorldGameState,state.actualWorldGameState==state.gameState);
    mark(PawnPrivateVptr,state.actualPawnTable==state.privatePawnTable);
    mark(MovementPhysicsEntry,state.actualMovementPhysicsEntry==reinterpret_cast<void*>(engine::image+0x78954d0));
    mark(MovementModeEntry,state.actualMovementModeEntry==reinterpret_cast<void*>(engine::image+0x4189be0));
    state.weakIdentitiesVerified=state.identitiesCaptured&&ResolveWeak(state.worldIdentity)==state.world
        &&ResolveWeak(state.pcIdentity)==pc&&ResolveWeak(state.pawnIdentity)==pawn
        &&ResolveWeak(state.connectionIdentity)==connection&&ResolveWeak(state.playerStateIdentity)==state.playerState
        &&ResolveWeak(state.movementIdentity)==state.movement&&ResolveWeak(state.inputIdentity)==state.inputState
        &&state.actualPCState==state.playerState&&state.actualPawnState==state.playerState
        &&Read<void*>(pawn,0x4f8)==state.movement&&Read<void*>(pawn,0xba0)==state.inputState;
    mark(OriginalWeakIdentities,state.weakIdentitiesVerified);
    state.ownedRequiredMask=AllOwnedChecks&~(state.installed?uint64_t(0):OwnedCheckBit(PawnPrivateVptr));
    state.ownedPassedMask=mask;
    state.ownedFailedMask=state.ownedRequiredMask&~mask;
    state.ownedLinks=state.ownedFailedMask==0;
    return state.ownedLinks;
}
inline bool Function(void*& result,const wchar_t* path,uint16_t bytes,uintptr_t exec){
    result=engine::Find(path);
    const uint16_t actualBytes=Read<uint16_t>(result,0xbe);
    const uint32_t flags=Read<uint32_t>(result,0xb8);
    const uintptr_t native=Read<uintptr_t>(result,0xe0);
    const bool valid=result&&actualBytes==bytes&&native==engine::image+exec
        &&(flags&0x01000440)==0x01000440;
    if(!valid){
        state.failedRPCName=path;state.expectedRPCBytes=bytes;state.actualRPCBytes=actualBytes;
        state.actualRPCFlags=flags;state.expectedRPCExecRva=exec;
        state.actualRPCExecRva=native>=engine::image?native-engine::image:native;
    }
    return valid;
}
inline bool Dispatch(void* target,void* function,void* parameters){
    auto call=Virtual<void(*)(void*,void*,void*)>(target,0x298);
    if(!call||!function)return Fail(3202,"network_input_original_rpc_dispatch_unavailable");
    call(target,function,parameters);return true;
}
// 1=open, 0=the original connection has closed, -1=unreadable/reopened.
// A closed session keeps its queue/ACK counters for the parent's native cleanup;
// it cannot become a fresh authority epoch by reopening the retained pointer.
inline int ConnectionStatus(){
    state.connectionStateReadable=original_players::Read(state.connection,0x13c,state.actualConnectionState);
    if(!state.connectionStateReadable)return -1;
    if(state.actualConnectionState!=NativeConnectionOpen){
        state.disconnected=true;state.stage="network_input_connection_closed";return 0;
    }
    return state.disconnected?-1:1;
}
inline bool CrouchReadback(uint8_t expectedRaw){
    uint8_t result=255;++state.crouchGetterCalls;
    const bool called=Dispatch(state.pawn,state.crouchGetterFunction,&result);
    state.crouchGetterValue=result;state.crouchAfterRaw=Read<uint8_t>(state.pawn,CrouchHeldOffset);
    state.crouchReadbackVerified=called&&state.crouchMetadataVerified&&result<=1
        &&Read<uint8_t>(state.pawn,0x224)==3&&state.crouchAfterRaw==expectedRaw
        &&result==uint8_t((expectedRaw&CrouchHeldMask)!=0);
    if(!state.crouchReadbackVerified){++state.crouchReadbackFailures;
        return Fail(3261,"network_input_authority_crouch_getter_readback_mismatch");}
    return true;
}
inline bool BindCrouchMetadata(){
    state.crouchType=Read<void*>(state.pawn,0x10);
    CrouchNativeName wanted{};
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&wanted,L"bCrouchHeld",1);
    state.crouchField=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(state.crouchType,&wanted,3);
    // Original FindFProperty1C63870 compares the eight-byte minimal FField
    // identity, not all twelve bytes of its FName argument.
    const auto actualName=Read<uint64_t>(state.crouchField,0x20);
    state.crouchFieldClass=Read<void*>(state.crouchField,8);
    state.crouchNativeBoolClass=engine::function<void*(*)()>(0x1c77e20)();
    state.crouchCastFlags=Read<uint64_t>(state.crouchFieldClass,0x18);
    state.crouchFieldOffset=Read<int32_t>(state.crouchField,0x44);
    state.crouchArrayDim=Read<int32_t>(state.crouchField,0x30);
    state.crouchElementSize=Read<int32_t>(state.crouchField,0x34);
    state.crouchTypeSize=Read<int32_t>(state.crouchType,0x60);
    state.crouchFieldSize=Read<uint8_t>(state.crouchField,0x78);
    state.crouchByteOffset=Read<uint8_t>(state.crouchField,0x79);
    state.crouchByteMask=Read<uint8_t>(state.crouchField,0x7a);
    state.crouchFieldMask=Read<uint8_t>(state.crouchField,0x7b);
    state.actualMovementPawnOwner=Read<void*>(state.movement,0x190);
    state.crouchMetadataVerified=state.crouchField&&state.crouchFieldClass==state.crouchNativeBoolClass
        &&state.crouchCastFlags==0x28001&&actualName==Read<uint64_t>(&wanted)
        &&state.crouchFieldOffset==static_cast<int32_t>(CrouchHeldOffset)
        &&state.crouchTypeSize>static_cast<int32_t>(CrouchHeldOffset)
        &&state.crouchArrayDim==1&&state.crouchElementSize==1&&state.crouchFieldSize==1
        &&state.crouchByteOffset==0&&state.crouchByteMask==CrouchHeldMask&&state.crouchFieldMask==CrouchHeldMask
        &&state.actualMovementPawnOwner==state.pawn;
    if(!state.crouchMetadataVerified)return Fail(3260,"network_input_authority_crouch_bool_metadata_mismatch");
    state.crouchGetterFunction=engine::Find(L"/Script/ShooterGame.ShooterCharacter.GetCrouchHeld");
    if(!state.crouchGetterFunction||Read<uint16_t>(state.crouchGetterFunction,0xbe)!=1
       ||(Read<uint32_t>(state.crouchGetterFunction,0xb8)&0x400)==0
       ||Read<uintptr_t>(state.crouchGetterFunction,0xe0)!=engine::image+0x6c64b00)
        return Fail(3259,"network_input_original_crouch_getter_abi_changed");
    return CrouchReadback(Read<uint8_t>(state.pawn,CrouchHeldOffset));
}
inline bool ApplyAuthorityCrouchInput(uint8_t flags,bool observed=true){
    // Missing authority input application only. The retained setter assigns
    // this intent bit for local pawns, so the real remote host supplies its
    // accepted request here. Native CanCrouch, posture, collision and capsule
    // interpolation remain in the original physical frame.
    state.actualMovementPawnOwner=Read<void*>(state.movement,0x190);
    if(!state.crouchMetadataVerified||state.actualMovementPawnOwner!=state.pawn)
        return Fail(3260,"network_input_authority_crouch_bool_metadata_mismatch");
    if(observed)++state.crouchInputCommands;
    state.crouchBeforeRaw=Read<uint8_t>(state.pawn,CrouchHeldOffset);
    if(!CrouchReadback(state.crouchBeforeRaw))return false;
    state.crouchRequested=uint8_t((flags&CompressedCrouchMask)!=0);
    state.crouchDesiredRaw=uint8_t((state.crouchBeforeRaw&uint8_t(~CrouchHeldMask))|state.crouchRequested);
    if(state.crouchDesiredRaw!=state.crouchBeforeRaw){
        Put<uint8_t>(state.pawn,CrouchHeldOffset,state.crouchDesiredRaw);
        ++state.crouchAuthorityWrites;++state.crouchInputTransitions;
    }
    if(Read<uint8_t>(state.pawn,CrouchHeldOffset)!=state.crouchDesiredRaw)
        return Fail(3262,"network_input_authority_crouch_masked_store_failed");
    return true;
}
inline bool ClockState(){
    auto vb=Read<void*>(state.clock,0x10);const int32_t offset=Read<int32_t>(vb,4);
    if(!vb||offset<0||offset>0x1000)return Fail(3203,"network_input_native_clock_vbase_invalid");
    state.clockBaseOffset=static_cast<uint32_t>(offset);
    state.clockIndex=Read<uint32_t>(state.clock,offset+0x18);
    state.clockStep=Read<uint32_t>(state.clock,offset+0x1c);
    state.clockAccumulated=Read<float>(state.clock,0x20);
    state.actualClockIndexOffset=Read<int32_t>(state.clock,0x24);
    if(state.clockStep!=1||!std::isfinite(state.clockAccumulated))return Fail(3204,"network_input_requires_original_128hz_single_step_clock");
    if(state.actualClockIndexOffset)return Fail(3313,"network_input_actual_authority_clock_must_remain_host_canonical");
    return true;
}
inline bool AuditHostTime(){
    if(!state.hostClockAccountingStarted)state.nativeInitialAccumulatorSeconds=double(state.clockAccumulated);
    // Real elapsed time is conserved before/after transfer. Native TimeShift
    // has its own equation; it is not misclassified as real elapsed or debt.
    const double eligible=state.hostTimeOriginStarted?state.hostOriginCreditSeconds+state.hostPostOriginSeconds:state.hostCreditedSeconds;
    state.hostTimeConservationError=eligible-state.hostCreditedSeconds-state.hostUnfundedSeconds-state.hostRetiredUnfundedSeconds;
    state.hostCreditConservationError=state.nativeInitialAccumulatorSeconds+state.hostCreditedSeconds+state.nativeFundingRoundoffSeconds
        +state.nativeAdvanceAdjustmentSeconds+state.nativeExternalAccumulatorSeconds+state.nativePostAdvanceAccumulatorSeconds
        -double(state.clockAccumulated)-double(state.clockAdvances)*(1.0/128.0);
    if(!state.epochRetired)state.hostFundedWorkSeconds=(std::max)(0.0,double(state.clockAccumulated));
    state.hostBudgetSeconds=static_cast<float>(state.hostFundedWorkSeconds);
    const double outstanding=state.hostFundedWorkSeconds+state.hostUnfundedSeconds;
    state.hostUnprocessedPeakSeconds=(std::max)(state.hostUnprocessedPeakSeconds,outstanding);
    const double tolerance=(std::max)(0.00001,std::abs(eligible)*1e-12);
    if(!std::isfinite(state.hostFundedWorkSeconds)||!std::isfinite(state.hostUnfundedSeconds)
       ||!std::isfinite(state.hostCreditConservationError)||!std::isfinite(state.hostTimeConservationError)
       ||state.hostFundedWorkSeconds<0||state.hostUnfundedSeconds<0
       ||std::abs(state.hostCreditConservationError)>tolerance||std::abs(state.hostTimeConservationError)>tolerance)
        return state.error?true:Fail(3340,"network_input_host_time_and_native_shift_ledger_conservation_failed");
    return true;
}
inline bool FundNativeClock(double available){
    const float before=Read<float>(state.clock,0x20);
    if(!std::isfinite(before))return Fail(3341,"network_input_original_clock_funding_nonfinite");
    const double room=(std::max)(0.0,double(Configuration.maxBufferedSeconds)-(std::max)(0.0,double(before)));
    const double request=(std::min)(available,room);
    if(request<=0)return true;
    float transfer=static_cast<float>(request);
    // Never manufacture host time by rounding a double debt upward to float.
    if(double(transfer)>request)transfer=std::nextafter(transfer,0.0f);
    if(transfer<=0)return true;
    const float after=before+transfer;
    if(!std::isfinite(before)||!std::isfinite(after))return Fail(3341,"network_input_original_clock_funding_nonfinite");
    // Preserve the native remainder, including its own signed TimeShift.
    // .25 limits each refilled working buffer; it must not truncate clock+20.
    Put<float>(state.clock,0x20,after);state.clockAccumulated=after;
    state.nativeFundingRoundoffSeconds+=double(after)-double(before)-double(transfer);
    state.hostCreditedSeconds+=double(transfer);
    state.lastHostFundingSeconds+=transfer;++state.hostFundingCalls;
    if(state.hostTimeOriginStarted)state.hostUnfundedSeconds-=double(transfer);
    return true;
}
inline bool SupplyHostTime(float deltaSeconds){
    state.lastHostDeltaSeconds=deltaSeconds;state.lastHostFundingSeconds=0;state.lastHostPollSteps=0;
    const double debtBefore=state.hostUnfundedSeconds;
    if(!ClockState())return false;
    if(state.hostClockAccountingStarted)
        state.nativeExternalAccumulatorSeconds+=double(state.clockAccumulated)-double(state.nativeAccumulatorAtPollEnd);
    else{state.hostClockAccountingStarted=true;state.nativeInitialAccumulatorSeconds=double(state.clockAccumulated);}
    if(deltaSeconds>Configuration.maxPollDeltaSeconds)++state.budgetCaps; // Diagnostic only; no time is discarded.
    if(!state.hostTimeOriginStarted&&!state.wireAligned){
        state.hostPreOriginSeconds+=double(deltaSeconds);
        // Preserve only the previous bounded startup credit. Waiting for a
        // client's first epoch is not an unlimited authority simulation debt.
        const double target=(std::min)(state.hostPreOriginSeconds,double(Configuration.maxBufferedSeconds));
        if(!FundNativeClock((std::max)(0.0,target-state.hostCreditedSeconds)))return false;
        state.hostOriginExcludedSeconds=state.hostPreOriginSeconds-state.hostCreditedSeconds;
    }else{
        if(!state.wireAligned)return Fail(3342,"network_input_active_host_time_origin_lost");
        if(!state.hostTimeOriginStarted){
            state.hostTimeOriginStarted=true;state.hostOriginCreditSeconds=state.hostCreditedSeconds;
            state.hostOriginExcludedSeconds=state.hostPreOriginSeconds-state.hostOriginCreditSeconds;
        }
        state.hostPostOriginSeconds+=double(deltaSeconds);state.hostUnfundedSeconds+=double(deltaSeconds);
        if(!FundNativeClock(state.hostUnfundedSeconds))return false;
        state.hostDebtPeakSeconds=(std::max)(state.hostDebtPeakSeconds,state.hostUnfundedSeconds);
        if(state.hostUnfundedSeconds<debtBefore){
            ++state.hostDebtRecoveryTransfers;state.hostDebtTransferredBacklogSeconds+=debtBefore-state.hostUnfundedSeconds;
        }
    }
    return AuditHostTime();
}
inline bool DebitCompletedHostStep(float step){
    if(state.physicsCompleted>state.clockAdvances)return Fail(3343,"network_input_completed_physics_without_original_clock_advance");
    state.hostServicedSeconds+=double(step);
    return AuditHostTime();
}
struct alignas(16) NativeSavedMove {
    uint8_t data[0x480]{};bool constructed=false;uint8_t alignmentPadding[15]{};
    NativeSavedMove(){engine::function<void*(*)(void*)>(0x7890fa0)(data);constructed=true;}
    ~NativeSavedMove(){if(constructed)engine::function<void*(*)(void*,uint32_t)>(0x7891010)(data,0);}
};
static_assert(sizeof(NativeSavedMove)==0x490&&alignof(NativeSavedMove)==16);
static_assert(offsetof(NativeSavedMove,constructed)==0x480);
inline uint32_t ClientToHostIndex(uint32_t clientIndex){
    engine::function<void(*)(void*,uint32_t*)>(0x789ee20)(state.mappingClock,&clientIndex);return clientIndex;
}
inline uint32_t HostToClientIndex(uint32_t hostIndex){
    engine::function<void(*)(void*,uint32_t*)>(0x789ee40)(state.mappingClock,&hostIndex);return hostIndex;
}
inline bool InitializeWireOrigin(const Move& first){
    const bool initial=!state.wireAligned;
    if(!ClockState())return false;
    local_server_human_funded_boundary_probe::BeforePrivateSeed(state,first.index);
    if(!state.mappingClockConstructed){
        state.mappingClock=mappingClockStorage.data;
        engine::function<void*(*)(void*,int32_t)>(0x789e6c0)(state.mappingClock,1);
        state.mappingClockConstructed=true;
        auto vb=Read<void*>(state.mappingClock,0x10);
        const int32_t displacement=Read<int32_t>(vb,4);
        if(displacement!=0x990)return Fail(3314,"network_input_private_clock_virtual_base_changed");
        state.mappingClockBaseOffset=static_cast<uint32_t>(displacement);
    }
    state.mappingClockRawIndex=Read<uint32_t>(state.mappingClock,state.mappingClockBaseOffset+0x18);
    state.mappingClockStep=Read<uint32_t>(state.mappingClock,state.mappingClockBaseOffset+0x1c);
    if(state.queueCount||Read<int32_t>(state.mappingClock,0x4d0)||Read<int32_t>(state.mappingClock,0x96c)
       ||state.mappingClockStep!=1||state.mappingClockRawIndex!=state.clockIndex
       ||(initial&&(state.clockIndex||state.lastConsumedIndex||Read<int32_t>(state.mappingClock,0x24)||Read<uint8_t>(state.mappingClock,0x4d8)))
       ||(!initial&&(!state.lastKnownInputValid||!state.heldSinceObserved)))
        return Fail(3300,"network_input_clock_origin_seed_context_invalid");
    const double initialBound=std::floor(state.hostElapsedSeconds*128.0)+Configuration.maxFutureSteps;
    if(!first.index||first.index>INT32_MAX||(initial&&static_cast<double>(first.index)>initialBound))
        return Fail(3301,"network_input_initial_client_index_exceeds_host_elapsed_bound");
    const uint32_t expectedHostIndex=NextIndex(state.clockIndex),rawBefore=state.clockIndex;
    auto table=Read<void**>(state.mappingClock);
    if(Read<void*>(table,0)!=reinterpret_cast<void*>(engine::image+0x789e910)
       ||Read<void*>(table,8)!=reinterpret_cast<void*>(engine::image+0x789ee20)
       ||Read<void*>(table,0x10)!=reinterpret_cast<void*>(engine::image+0x789edd0)
       ||Read<void*>(table,0x18)!=reinterpret_cast<void*>(engine::image+0x789ee40)
       ||Read<void*>(table,0x28)!=reinterpret_cast<void*>(engine::image+0x789f090)
       ||Read<void*>(table,0x30)!=reinterpret_cast<void*>(engine::image+0x789f0b0)
       ||!Read<void*>(state.mappingClock,0x4b8)||Read<int32_t>(state.mappingClock,0x4cc)!=32)
        return Fail(3302,"network_input_initial_native_clock_mapping_profile_changed");
    auto forces=Read<void*>(state.pawn,0x1100);const int32_t forceCount=Read<int32_t>(forces,0x158),forceCapacity=Read<int32_t>(forces,0x15c);
    auto forceData=Read<void*>(forces,0x150);
    if(!Read<uint8_t>(state.pawn,0xa90)||!forces||forceCount<0||forceCount>1024||forceCapacity<forceCount
       ||(forceCount&&!forceData))return Fail(3303,"network_input_origin_requires_original_begin_play_force_collection");
    for(int32_t i=0;i<forceCount;++i)if(!Read<void*>(forceData,size_t(i)*8))
        return Fail(3303,"network_input_origin_requires_original_begin_play_force_collection");
    auto prediction=Read<void*>(state.movement,0x7f0),vbtable=Read<void*>(prediction,8);
    const int32_t displacement=Read<int32_t>(vbtable,4);
    if(!prediction||!vbtable||displacement<0||displacement>0x1000)return Fail(3304,"network_input_origin_saved_move_clock_base_invalid");
    auto clockBase=static_cast<uint8_t*>(prediction)+8+displacement;
    if(Read<uint32_t>(clockBase,8)!=rawBefore)return Fail(3304,"network_input_origin_saved_move_clock_base_invalid");
    NativeSavedMove seed,popped;
    // This captures actual authority state. SetMoveFor supplies accepted intent;
    // PostUpdate also consumes the real authority teleport latch. Its captured
    // value is retained for the first honest correction, never restored/faked.
    engine::function<void(*)(void*,void*,float,const engine::Vec*,void*)>(0x78913a0)
        (seed.data,state.pawn,1.0f/128.0f,&first.intent,clockBase);
    engine::function<void(*)(void*,void*,uint32_t)>(0x7891280)(seed.data,state.pawn,0);
    state.seedTeleportPending=state.seedTeleportPending||Read<uint8_t>(seed.data,0x468)!=0;
    state.seedRawBefore=Read<uint32_t>(state.clock,state.clockBaseOffset+0x18);
    const float accumulatedBefore=Read<float>(state.clock,0x20);
    Put<uint32_t>(seed.data,0x1c,first.index);++state.nativeSeedCalls;
    engine::function<void(*)(void*,void*,int32_t)>(0x789e910)(state.mappingClock,seed.data,0);
    const bool poppedSeed=engine::function<bool(*)(void*,void*)>(0x789ee60)(state.mappingClock,popped.data);
    state.clockIndexOffset=Read<int32_t>(state.mappingClock,0x24);state.seedRawAfter=Read<uint32_t>(state.clock,state.clockBaseOffset+0x18);
    state.mappingClockRawIndex=Read<uint32_t>(state.mappingClock,state.mappingClockBaseOffset+0x18);
    state.seedHistoryAfter=Read<uint32_t>(state.mappingClock,0x4d0);state.seedPendingAfter=Read<uint32_t>(state.mappingClock,0x96c);
    if(!poppedSeed||Read<uint32_t>(seed.data,0x1c)!=expectedHostIndex||Read<uint32_t>(popped.data,0x1c)!=expectedHostIndex
       ||state.seedRawBefore!=rawBefore||state.seedRawAfter!=rawBefore||state.seedHistoryAfter||state.seedPendingAfter
       ||Read<float>(state.clock,0x20)!=accumulatedBefore||Read<int32_t>(state.clock,0x24)
       ||state.mappingClockRawIndex!=rawBefore||state.clockIndexOffset!=int64_t(expectedHostIndex)-first.index
       ||ClientToHostIndex(first.index)!=expectedHostIndex||HostToClientIndex(expectedHostIndex)!=first.index)
        return Fail(3305,"network_input_original_clock_origin_roundtrip_failed");
    state.seedClientSeconds=static_cast<float>(first.index)*(1.0f/128.0f);
    state.seedMappedHostSeconds=state.seedClientSeconds;
    engine::function<void(*)(void*,float*)>(0x789f090)(state.mappingClock,&state.seedMappedHostSeconds);
    state.seedInverseClientSeconds=state.seedMappedHostSeconds;
    engine::function<void(*)(void*,float*)>(0x789f0b0)(state.mappingClock,&state.seedInverseClientSeconds);
    if(state.seedMappedHostSeconds!=static_cast<float>(expectedHostIndex)*(1.0f/128.0f)||state.seedInverseClientSeconds!=state.seedClientSeconds)
        return Fail(3306,"network_input_original_float_clock_origin_roundtrip_failed");
    state.timestepRpcValue=state.clockIndexOffset;
    if(!Dispatch(state.movement,state.timestepFunction,&state.timestepRpcValue))return false;
    ++state.timestepRpcCalls;state.timestepRpcSubmitted=true;state.lastWireSyncClientIndex=first.index;
    if(initial){state.wireOriginClientIndex=first.index;state.unobservedInitialClientSteps=first.index-1;}
    else ++state.clockResynchronizations;
    state.wireAligned=true;
    if(!initial||state.unobservedInitialClientSteps||state.seedTeleportPending)state.forceHardCorrection=true;
    return true;
}
inline bool MapBarrierDeadlines(){
    if(!state.wireAligned)return true;
    if(Read<int32_t>(state.mappingClock,0x24)!=state.clockIndexOffset||Read<int32_t>(state.clock,0x24))
        return Fail(3307,"network_input_native_clock_origin_changed");
    const int32_t base=Read<int32_t>(state.incomingEvents,4),count=Read<int32_t>(state.incomingEvents,0x740);
    auto data=Read<uint8_t*>(state.incomingEvents,0x738);
    for(auto& item:state.barrierTimes)if(item.present&&item.eventId<=base)item={};
    for(int32_t i=0;i<count;++i){
        auto event=data+size_t(i)*40;if(Read<uint8_t>(event)!=7)continue;
        const int64_t id=int64_t(base)+1+i;
        if(id<0||id>INT32_MAX)return Fail(3308,"network_input_barrier_event_identity_invalid");
        BarrierTime* record=nullptr;
        for(auto& item:state.barrierTimes)if(item.present&&item.eventId==id){record=&item;break;}
        if(!record){for(auto& item:state.barrierTimes)if(!item.present){record=&item;break;}
            if(!record)return Fail(3309,"network_input_barrier_time_records_full");
            record->present=true;record->eventId=static_cast<int32_t>(id);record->clientSeconds=Read<float>(event,8);
            if(!std::isfinite(record->clientSeconds)||record->clientSeconds<0)return Fail(3310,"network_input_barrier_client_time_invalid");}
        float mapped=record->clientSeconds;
        engine::function<void(*)(void*,float*)>(0x789f090)(state.mappingClock,&mapped);
        if(!std::isfinite(mapped))return Fail(3311,"network_input_barrier_mapped_time_invalid");
        Put<float>(event,8,mapped);++state.mappedBarrierEvents;
    }
    return true;
}
inline bool AdvanceMappingClock(uint32_t expectedIndex){
    if(!state.mappingClockConstructed||Read<int32_t>(state.mappingClock,0x24)!=state.clockIndexOffset
       ||Read<uint32_t>(state.mappingClock,state.mappingClockBaseOffset+0x18)!=state.lastConsumedIndex)
        return Fail(3315,"network_input_private_clock_and_authority_diverged");
    // Private adapter metadata receives exactly the fixed-step budget already
    // granted by the real authority clock. A null shift avoids consuming the
    // original authority time-shift object twice. No live clock index is set.
    Put<float>(state.mappingClock,0x20,1.0f/128.0f);
    if(!engine::function<bool(*)(void*,void*,bool)>(0x789f0e0)(state.mappingClock,nullptr,true))
        return Fail(3316,"network_input_private_native_clock_advance_failed");
    state.mappingClockRawIndex=Read<uint32_t>(state.mappingClock,state.mappingClockBaseOffset+0x18);
    ++state.mappingClockAdvances;
    if(state.mappingClockRawIndex!=expectedIndex||Read<float>(state.mappingClock,0x20)!=0
       ||Read<uint32_t>(state.mappingClock,state.mappingClockBaseOffset+0x1c)!=1)
        return Fail(3317,"network_input_private_native_clock_advanced_unexpected_index");
    return true;
}
inline void Receive(void* pawn,const ComponentDataStream* stream);
inline bool PrepareNextEpoch(void* world,void* pc,void* pawn,void* connection){
    if(epochs.blockedError)return Fail(epochs.blockedError,epochs.blockedStage?epochs.blockedStage:"network_input_retired_fatal_epoch_requires_new_session");
    if(!epochs.anchored||state.installed)return true;
    if(GetCurrentThreadId()!=epochs.gameThread||original_players::gameThread!=epochs.gameThread
       ||engine::image!=epochs.image||world!=epochs.world||pc!=epochs.pc||connection!=epochs.connection
       ||Read<int32_t>(connection,0x13c)!=NativeConnectionOpen
       ||Read<void*>(pc,0x470)!=epochs.playerState||ResolveWeak(epochs.worldIdentity)!=world
       ||ResolveWeak(epochs.pcIdentity)!=pc||ResolveWeak(epochs.connectionIdentity)!=connection
       ||ResolveWeak(epochs.playerStateIdentity)!=epochs.playerState)
        return Fail(3271,"network_input_replacement_epoch_session_identity_changed");
    WeakIdentity nextPawn{};
    if(!CaptureWeak(pawn,nextPawn)||SameWeak(nextPawn,epochs.lastRetiredIdentity))
        return Fail(3272,"network_input_replacement_requires_new_native_pawn_identity");
    // Every packet, input history, clock, budget, ACK and correction field starts
    // fresh. Only separate lifetime diagnostics and session anchors survive.
    if(state.epochRetired){state=State{};EpochCounters();}
    return true;
}
inline bool InstallAuthorityImpl(void* world,void* pc,void* pawn,void* connection){
    if(state.error)return false;
    if(!PrepareNextEpoch(world,pc,pawn,connection))return false;
    if(state.installed)return world==state.world&&pc==state.pc&&pawn==state.pawn&&connection==state.connection
        &&OwnedLinks()||Fail(3205,"network_input_installed_owner_changed");
    if(!engine::image||!original_players::gameThread||GetCurrentThreadId()!=original_players::gameThread)
        return Wait("network_input_waiting_owned_game_thread");
    state.gameThread=GetCurrentThreadId();state.world=world;state.pc=pc;state.pawn=pawn;state.connection=connection;
    if(!original_players::IsA(pawn,original_players::ShooterCharacterClass)
       ||!original_players::IsA(pc,original_players::ShooterControllerClass))return Wait("network_input_waiting_shooter_owner");
    state.movement=Read<void*>(pawn,0x4f8);state.inputState=Read<void*>(pawn,0xba0);
    state.gameState=Read<void*>(world,0x178);state.fixedStep=Read<void*>(state.gameState,0xcc0);
    if(!original_players::IsA(state.movement,0x6c7d380)||!original_players::IsA(state.inputState,0x6494370)
       ||!original_players::IsA(state.gameState,original_players::ShooterGameStateClass))
        return Wait("network_input_waiting_native_components");
    if(!state.profilesVerified&&!VerifyProfiles())return false;
    if(!CaptureEpochIdentities())return false;
    if(!OwnedLinks())return Fail(3206,"network_input_authority_connection_links_invalid");
    const int mode=engine::function<int(*)(void*)>(0x3efc820)(pawn);
    if(mode!=1&&mode!=2)return Fail(3207,"network_input_requires_actual_server_netmode");
    uint8_t local=0;
    if(!engine::Event(pawn,L"/Script/Engine.Pawn.IsLocallyControlled",&local)||local)
        return Fail(3208,"network_input_requires_real_remote_authority_pawn");
    if(!BindCrouchMetadata())return false;
    if(!Read<uint8_t>(state.fixedStep,0xd9))return Wait("network_input_waiting_native_fixed_step_rate");
    if(Read<uint8_t>(state.fixedStep,0xd9)!=Configuration.tickRate)
        return Fail(3209,"network_input_native_fixed_step_rate_not_128");
    state.clock=engine::function<void*(*)(void*)>(0x7892e10)(state.movement);
    state.timeShift=Read<void*>(state.movement,0x1530);
    if(!state.clock||!state.timeShift)return Wait("network_input_waiting_original_clock_dependencies");
    if(Read<void*>(state.clock)!=reinterpret_cast<void*>(engine::image+0xb600a60)
       ||Read<void*>(Read<void**>(state.clock),0x40)!=reinterpret_cast<void*>(engine::image+0x789f280))
        return Fail(3210,"network_input_original_server_clock_profile_changed");
    if(!ClockState())return false;
    if(state.clockIndex!=0)return Fail(3211,"network_input_requires_fresh_authority_timeline");
    auto getWorld=Virtual<void*(*)(void*)>(pawn,0x1b8);
    if(!getWorld||getWorld(pawn)!=world)return Fail(3212,"network_input_native_world_owner_mismatch");
    state.nativeNetDriver=engine::function<void*(*)(void*,const void*)>(0x3efc0d0)
        (world,static_cast<const uint8_t*>(pawn)+0x218);
    if(!state.nativeNetDriver)return Wait("network_input_waiting_original_actor_net_driver");
    if(!Function(state.ackFunction,L"/Script/ShooterGame.ShooterCharacter.ClientAckInputAndMovement",8,0x6c68f10)
       ||!Function(state.gradualFunction,L"/Script/ShooterGame.ShooterCharacter.ClientAckInputAndMovementWithCorrection",88,0x6c68cc0)
       ||!Function(state.hardFunction,L"/Script/Engine.CharacterMovementComponent.ClientAdjustPosition",HardCorrectionLogicalBytes,0x4182420)
       ||!Function(state.timestepFunction,L"/Script/ShooterGame.ShooterCharacterMovement.ClientUpdateClientToServerTimestep",4,0x6c84330))
        return Fail(3213,"network_input_original_ack_correction_rpc_abi_changed");
    if(Read<uint32_t>(state.timestepFunction,0xb8)!=0x01080cc0){
        state.failedRPCName=L"/Script/ShooterGame.ShooterCharacterMovement.ClientUpdateClientToServerTimestep";
        state.expectedRPCBytes=4;state.actualRPCBytes=Read<uint16_t>(state.timestepFunction,0xbe);
        state.actualRPCFlags=Read<uint32_t>(state.timestepFunction,0xb8);state.expectedRPCExecRva=0x6c84330;
        const uintptr_t actual=Read<uintptr_t>(state.timestepFunction,0xe0);
        state.actualRPCExecRva=actual>=engine::image?actual-engine::image:actual;
        return Fail(3213,"network_input_original_timestep_rpc_flags_changed");
    }
    state.sourcePawnTable=Read<void**>(pawn);
    if(state.sourcePawnTable!=reinterpret_cast<void**>(engine::image+PawnTableRva)
       ||Read<void*>(state.sourcePawnTable,StreamSlot)!=reinterpret_cast<void*>(engine::image+0x18a3a00)
       ||Read<void*>(state.sourcePawnTable,0x850)!=reinterpret_cast<void*>(engine::image+0x787f820)
       ||!original_players::Readable(state.sourcePawnTable-1,(PawnTableEntries+1)*8))
        return Fail(3214,"network_input_missing_receiver_seam_changed");
    const bool reused=epochs.reusablePawnTable!=nullptr;
    auto copied=epochs.reusablePawnTable;
    if(reused){
        epochs.reusablePawnTable=nullptr;
        if(Read<void*>(copied,StreamSlot)!=reinterpret_cast<void*>(&Receive)
           ||Read<void*>(copied,0x850)!=reinterpret_cast<void*>(engine::image+0x787f820))
            return Fail(3273,"network_input_retired_receiver_table_not_pristine");
    }else{
        auto storage=static_cast<void**>(VirtualAlloc(nullptr,(PawnTableEntries+1)*8,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
        if(!storage)return Fail(3215,"network_input_private_receiver_table_allocation_failed");
        std::memcpy(storage,state.sourcePawnTable-1,(PawnTableEntries+1)*8);
        copied=storage+1;copied[StreamSlot/8]=reinterpret_cast<void*>(&Receive);
        DWORD previous=0;
        if(!VirtualProtect(storage,(PawnTableEntries+1)*8,PAGE_READONLY,&previous)){
            VirtualFree(storage,0,MEM_RELEASE);return Fail(3216,"network_input_private_receiver_table_protection_failed");
        }
    }
    engine::function<void*(*)(void*)>(0x70aa150)(state.incomingEvents);
    state.incomingEventsConstructed=true;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(pawn),copied,state.sourcePawnTable)!=state.sourcePawnTable){
        if(!reused)VirtualFree(copied-1,0,MEM_RELEASE);
        return Fail(3217,"network_input_pawn_table_changed_during_install");
    }
    state.privatePawnTable=copied;state.privateTableReadOnly=true;state.installed=true;
    state.installedPosition=Read<engine::Vec>(Read<void*>(state.movement,0xd8),0x320);
    state.installedVelocity=Read<engine::Vec>(state.movement,0xf0);
    if(!epochs.anchored){
        epochs.anchored=true;epochs.image=engine::image;epochs.gameThread=state.gameThread;
        epochs.world=world;epochs.pc=pc;epochs.connection=connection;epochs.playerState=state.playerState;
        epochs.worldIdentity=state.worldIdentity;epochs.pcIdentity=state.pcIdentity;
        epochs.connectionIdentity=state.connectionIdentity;epochs.playerStateIdentity=state.playerStateIdentity;
    }
    ++epochs.started;EpochCounters();
    state.pawnTableAfterInstall=Read<void**>(pawn);state.movementTableAtInstall=Read<void**>(state.movement);
    Thread();OwnedLinks();
    state.ownedPassedMaskAtInstall=state.ownedPassedMask;state.ownedFailedMaskAtInstall=state.ownedFailedMask;
    state.stage="network_input_receiver_installed_runtime_unproven";return true;
}

struct alignas(16) Reader { uint8_t data[0xc0]{}; };
inline bool ReaderError(Reader& r){return (Read<uint8_t>(r.data,0x29)&4)!=0;}
inline uint64_t Position(Reader& r){return Read<uint64_t>(r.data,0xb0);}
inline bool Bits(Reader& r,void* value,uint64_t count){
    const auto pos=Position(r),total=Read<uint64_t>(r.data,0xa8);
    if(count>total||pos>total-count)return false;
    engine::function<void(*)(void*,void*,int64_t)>(0x1a5d4c0)(r.data,value,static_cast<int64_t>(count));
    return !ReaderError(r)&&Position(r)==pos+count;
}
inline bool PeekInputEventId(Reader& reader,uint16_t componentEnd,int32_t& value){
    // Diagnostic peek only. FBitReader's bit order is LSB first; the original
    // codec consumes this same four-byte first-event field. No archive cursor,
    // queue, cancellation or acceptance decision is changed by this sample.
    const uint64_t pos=Position(reader);
    if(pos+32>componentEnd)return false;
    const auto data=Read<const uint8_t*>(reader.data,0x98);
    if(!data)return false;
    uint32_t sampled=0;
    for(uint32_t bit=0;bit<32;++bit){const auto source=pos+bit;
        sampled|=uint32_t((data[source/8]>>(source%8))&1)<<bit;}
    std::memcpy(&value,&sampled,sizeof(value));return true;
}
struct alignas(8) NativeCheckpointStream {
    uint8_t data[0x748]{};bool constructed=false;uint8_t alignmentPadding[7]{};
    NativeCheckpointStream(){engine::function<void*(*)(void*)>(0x70aa150)(data);constructed=true;}
    ~NativeCheckpointStream(){if(constructed){engine::function<void(*)(void*)>(0x70aa200)(data);++state.nativeCheckpointDestructions;}}
};
struct alignas(16) NativeCheckpointReader {
    Reader reader;bool constructed=false;uint8_t alignmentPadding[15]{};
    NativeCheckpointReader(const uint8_t* bytes,uint64_t bits){
        engine::function<void*(*)(void*,const uint8_t*,int64_t)>(0x1a5d990)(reader.data,bytes,static_cast<int64_t>(bits));constructed=true;
    }
    ~NativeCheckpointReader(){if(constructed)engine::function<void*(*)(void*,uint32_t)>(0x1a5dab0)(reader.data,0);}
};
static_assert(sizeof(NativeCheckpointStream)==0x750&&alignof(NativeCheckpointStream)==8);
static_assert(sizeof(NativeCheckpointReader)==0xd0&&alignof(NativeCheckpointReader)==16);
inline bool CorrectionField(void* type,const wchar_t* name,int32_t offset,int32_t bytes,uint64_t flags=0){
    CrouchNativeName wanted{};engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&wanted,name,1);
    auto field=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,&wanted,3);
    return field&&Read<int32_t>(field,0x44)==offset&&Read<int32_t>(field,0x34)==bytes&&Read<int32_t>(field,0x30)==1
        &&(Read<uint64_t>(field,0x38)&flags)==flags;
}
inline bool BindCorrectionMetadata(void* inventory){
    if(state.correctionMetadataVerified)return true;
    auto type=Read<void*>(inventory,0x10);
    state.correctionMetadataVerified=original_players::IsA(inventory,0x6499330)
        &&CorrectionField(type,L"CurrentEquippable",0x280,8)&&CorrectionField(type,L"DesiredEquippable",0x268,8)
        &&CorrectionField(type,L"AuthServerCorrectRepVariables",0x320,16,0x100000020ULL);
    return state.correctionMetadataVerified||Fail(3330,"network_input_original_inventory_correction_schema_changed");
}
inline bool TeleportBoolField(void* type,const wchar_t* name,int32_t offset){
    CrouchNativeName wanted{};engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&wanted,name,1);
    auto field=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,&wanted,3);
    auto nativeBool=engine::function<void*(*)()>(0x1c77e20)();
    return field&&Read<void*>(field,8)==nativeBool&&Read<uint64_t>(nativeBool,0x18)==0x28001
        &&Read<uint64_t>(field,0x20)==Read<uint64_t>(&wanted)
        &&Read<int32_t>(field,0x44)==offset&&Read<int32_t>(field,0x34)==1&&Read<int32_t>(field,0x30)==1
        &&Read<uint8_t>(field,0x78)==1&&Read<uint8_t>(field,0x79)==0
        &&Read<uint8_t>(field,0x7a)==1&&Read<uint8_t>(field,0x7b)==0xff;
}
inline bool ReadTeleportCorrectionFlags(){
    if(!state.teleportMetadataVerified){
        auto type=Read<void*>(state.movement,0x10);
        state.teleportMetadataVerified=original_players::IsA(state.movement,0x6c7d380)
            &&Read<int32_t>(type,0x60)>0xdd1
            &&TeleportBoolField(type,L"bAuthTeleportSinceLastMove",0xdd0)
            &&TeleportBoolField(type,L"bAuthTeleportSinceLastCorrectionSent",0xdd1)
            &&Read<uintptr_t>(Read<void**>(state.movement),0x5d0)==engine::image+0x7897e70
            &&Read<uintptr_t>(Read<void**>(state.pawn),0x610)==engine::image+0x788e210
            &&Read<uintptr_t>(Read<void**>(state.pawn),0x618)==engine::image+0x788e310;
        if(!state.teleportMetadataVerified)return Fail(3380,"network_input_original_teleport_flag_schema_changed");
    }
    state.teleportSinceMove=Read<uint8_t>(state.movement,0xdd0);
    state.teleportSinceCorrection=Read<uint8_t>(state.movement,0xdd1);++state.teleportFlagSamples;
    return (state.teleportSinceMove<=1&&state.teleportSinceCorrection<=1)
        ||Fail(3381,"network_input_original_teleport_flag_value_invalid");
}
inline bool DispatchTeleportCorrection(HardCorrection& parameters,bool nativeTeleport){
    if(!nativeTeleport)return Dispatch(state.movement,state.hardFunction,&parameters);
    if(state.teleportCorrectionInFlight)return Fail(3382,"network_input_reentrant_native_teleport_correction");
    const auto epoch=state.epochNumber;
    const auto pawn=state.pawn,movement=state.movement,connection=state.connection;
    const auto pawnId=state.pawnIdentity,movementId=state.movementIdentity,connectionId=state.connectionIdentity;
    const auto sourceIndex=state.lastConsumedIndex;
    state.teleportCorrectionInFlight=true;
    const bool submitted=Dispatch(movement,state.hardFunction,&parameters);
    state.teleportCorrectionInFlight=false;
    if(!submitted||state.error)return false;
    // This is the missing host sender's SinceLastCorrectionSent consumption,
    // not a recovered native server producer. Original successful TeleportTo
    // sets both bytes in7897E70; original SavedMove7891280 alone owns DD0.
    // The submitted RPC carries that real DD1 as bTeleport. It is not a
    // client receipt or fabricated ACK. Never consume another/retired epoch.
    if(!Thread()||!state.installed||state.epochRetired||state.epochNumber!=epoch
       ||state.pawn!=pawn||state.movement!=movement||state.connection!=connection
       ||!SameWeak(state.pawnIdentity,pawnId)||!SameWeak(state.movementIdentity,movementId)
       ||!SameWeak(state.connectionIdentity,connectionId)||ResolveWeak(pawnId)!=pawn
       ||ResolveWeak(movementId)!=movement||ResolveWeak(connectionId)!=connection
       ||state.lastConsumedIndex!=sourceIndex||ConnectionStatus()!=1||!OwnedLinks())
        return Fail(3383,"network_input_teleport_rpc_epoch_changed_flag_retained");
    ++state.teleportCorrectionsSubmitted;state.teleportSourceEpoch=epoch;
    state.teleportCorrectionClientIndex=parameters.index;
    const auto pending=Read<uint8_t>(movement,0xdd1);
    if(pending>1)return Fail(3381,"network_input_original_teleport_flag_value_invalid");
    if(pending){Put<uint8_t>(movement,0xdd1,0);++state.teleportFlagConsumptions;}
    else ++state.teleportFlagAlreadyConsumed;
    state.teleportClearReadback=Read<uint8_t>(movement,0xdd1);
    return state.teleportClearReadback==0||Fail(3384,"network_input_host_sent_teleport_flag_consumption_failed");
}
inline bool ObserveCorrectionPublication(void* inventory,void* corrected,int32_t auth,int32_t origin){
    auto& publication=state.correctionPublication;
    if(publication.observed&&auth==publication.authIndex){
        if(publication.inventory!=inventory||publication.corrected!=corrected||publication.originIndex!=origin
           ||ResolveWeak(publication.inventoryIdentity)!=inventory
           ||(corrected&&ResolveWeak(publication.correctedIdentity)!=corrected))
            return Fail(3331,"network_input_original_correction_publication_changed_without_new_counter");
        ++publication.observations;return true;
    }
    if(publication.observed&&auth<publication.authIndex)
        return Fail(3332,"network_input_original_correction_counter_rewound");
    WeakIdentity inventoryIdentity{},correctedIdentity{};
    if(!CaptureWeak(inventory,inventoryIdentity)||(corrected&&!CaptureWeak(corrected,correctedIdentity)))
        return Fail(3333,"network_input_original_correction_object_identity_unavailable");
    publication.observed=true;publication.consumed=false;publication.authIndex=auth;publication.originIndex=origin;
    publication.inventory=inventory;publication.corrected=corrected;publication.inventoryIdentity=inventoryIdentity;
    publication.correctedIdentity=correctedIdentity;publication.packet=state.receivedPackets;++publication.observations;
    return true;
}
inline bool SameCheckpointField(const uint8_t* retained,const uint8_t* wire,uint8_t type){
    if(Read<uint8_t>(retained)!=type||Read<uint8_t>(wire)!=type)return false;
    if(type==6)return Read<int32_t>(retained,8)==Read<int32_t>(wire,8);
    if(type==9)return Read<uint32_t>(retained,8)==Read<uint32_t>(wire,8)
        &&(Read<uint16_t>(retained,12)&0x1ff)==(Read<uint16_t>(wire,12)&0x1ff);
    return type==4&&Read<uint64_t>(retained,8)==Read<uint64_t>(wire,8);
}
inline bool SameReplacementFrame(const uint8_t* retained,const uint8_t* wire,int32_t firstEventId,uint32_t retainedCount,uint32_t wireCount){
    auto& proof=state.correctionCancellation.replacementFrame;proof={};
    if(retainedCount<5||wireCount<5||retainedCount>Configuration.maxInputEvents*2||wireCount>Configuration.maxInputEvents)return false;
    if(!retained||!wire||!original_players::Readable(retained,size_t(retainedCount)*40)
       ||!original_players::Readable(wire,size_t(wireCount)*40)){
        proof.reason=native_input_checkpoint_frame::Reason::BufferUnreadable;return false;
    }
    const auto read=[&](bool fromWire,uint32_t ordinal){
        const auto event=(fromWire?wire:retained)+size_t(ordinal)*40;
        native_input_checkpoint_frame::Event value{};value.type=Read<uint8_t>(event);
        if(value.type==7){value.deadline=Read<float>(event,8);
            if(!fromWire&&ordinal==3)for(const auto& barrier:state.barrierTimes)
                if(barrier.present&&barrier.eventId==firstEventId+3){value.deadline=barrier.clientSeconds;break;}
        }else if(value.type==0){
            value.equippableBits=static_cast<uint8_t>(Read<uint8_t>(event,16)&0x1f);
            value.extraBits=Read<uint32_t>(event,20);value.extra=Read<float>(event,20);
        }else if(value.type==1){value.activationBits=static_cast<uint16_t>(Read<uint16_t>(event,20)&0x3f3f);
        }else if(value.type==2){value.movementBit=static_cast<uint8_t>(Read<uint8_t>(event,8)&1);
        // Original codec70A9210 dispatch3 (70A9723) serializes byte8 bits1/0.
        // Dispatch10 (70A96A1) serializes byte8 and the boolean at byte9.
        // Inactive union bytes and padding differ between the two native reads.
        }else if(value.type==3){value.type3Flags=static_cast<uint8_t>(Read<uint8_t>(event,8)&3);
        }else if(value.type==10){value.type10Value=Read<uint8_t>(event,8);value.type10Flag=Read<uint8_t>(event,9)?1:0;
        }else if(value.type==5)for(size_t n=0;n<value.aim.size();++n)value.aim[n]=Read<uint64_t>(event,8+n*8);
        return value;
    };
    // Original Frame70A4530 inserts 7, then merges native state histories
    // before view history. Compare every intervening proven action's active
    // wire fields; do not skip it. Unknown types/second frames stay unproven.
    proof=native_input_checkpoint_frame::Prove(retainedCount,wireCount,Configuration.maxInputEvents,read);
    return proof.matched;
}
inline void CaptureCheckpointDecodeDiagnostic(const uint8_t* wire,int32_t wireCount,
        const uint8_t* retained,int32_t retainedCount,const InputWireTrace& intake){
    // The native decoder/end checks and all original admission gates remain in
    // their original caller. This path writes only the diagnostic POD below.
    InputCheckpointDecodeDiagnostic next{};next.captured=true;next.packet=state.receivedPackets;next.epoch=state.epochNumber;
    const auto& d=state.correctionCancellation;next.authIndex=d.authIndex;next.originIndex=d.originIndex;
    next.acknowledgedIndex=d.acknowledgedIndex;next.intake=intake;
    next.wireCount=wireCount;next.retainedCount=retainedCount;
    next.buffersReadable=wire&&retained&&intake.firstEventId>=0
        &&int64_t(intake.firstEventId)+(std::max)(wireCount,(std::min)(retainedCount,100))-1<=INT32_MAX
        &&wireCount>0&&wireCount<=100
        &&retainedCount>0&&retainedCount<=200
        &&original_players::Readable(wire,size_t(wireCount)*40)
        &&original_players::Readable(retained,size_t(retainedCount)*40);
    if(next.buffersReadable){
        next.copiedWire=static_cast<uint32_t>(wireCount);
        next.copiedRetained=static_cast<uint32_t>((std::min)(retainedCount,100));
        for(uint32_t i=0;i<next.copiedWire;++i){next.wire[i].eventId=intake.firstEventId+static_cast<int32_t>(i);
            std::memcpy(next.wire[i].bytes.data(),wire+size_t(i)*40,40);}
        for(uint32_t i=0;i<next.copiedRetained;++i){next.retained[i].eventId=intake.firstEventId+static_cast<int32_t>(i);
            std::memcpy(next.retained[i].bytes.data(),retained+size_t(i)*40,40);}
    }
    if(!state.firstEligibleCheckpointDecode.captured||state.firstEligibleCheckpointDecode.epoch!=next.epoch
       ||state.firstEligibleCheckpointDecode.authIndex!=next.authIndex)
        state.firstEligibleCheckpointDecode=next;
    state.latestCheckpointDecode=next;
}
inline bool HonorCorrectionCancellation(Reader& source,uint64_t inputStart,uint16_t componentEnd){
    const auto& trace=state.lastInputWire;
    const int32_t base=Read<int32_t>(state.incomingEvents,4),count=Read<int32_t>(state.incomingEvents,0x740);
    if(!trace.headerKnown||trace.packet!=state.receivedPackets||count<=0||int64_t(trace.firstEventId)<=int64_t(base)+1)return true;
    auto& d=state.correctionCancellation;d={};d.packet=state.receivedPackets;d.firstEventId=trace.firstEventId;
    d.oldBase=base;d.oldCount=count;d.newBase=base;d.newCount=count;
    d.stage="network_input_waiting_original_correction_cancellation_gates";
    const auto pass=[&](uint32_t bit,bool condition){if(condition)d.passedMask|=uint64_t(1)<<bit;return condition;};
    auto inventory=Read<void*>(state.pawn,0xc08);
    if(!pass(0,original_players::IsA(inventory,0x6499330)))return true;
    if(!BindCorrectionMetadata(inventory))return false;
    pass(1,Read<void*>(inventory,0xc8)==state.pawn&&Read<void*>(inventory,0x2f0)==state.pawn);
    pass(2,Read<uint8_t>(inventory,0x30c)==1);pass(3,Read<uint8_t>(inventory,0x2fc)==3);
    pass(4,Read<uint8_t>(inventory,0x30d)==1);
    d.authIndex=Read<int32_t>(inventory,0x328);d.originIndex=Read<int32_t>(inventory,0x32c);
    d.acknowledgedIndex=Read<int32_t>(state.inputState,0x260);d.corrected=Read<void*>(inventory,0x320);
    pass(5,d.authIndex>=0&&d.authIndex>d.acknowledgedIndex&&d.authIndex>state.lastCancelledCorrectionIndex);
    pass(6,d.originIndex==d.acknowledgedIndex);
    pass(7,d.corrected==Read<void*>(inventory,0x280)&&d.corrected==Read<void*>(inventory,0x268));
    if(d.passedMask!=0xff)return true;
    if(!ObserveCorrectionPublication(inventory,d.corrected,d.authIndex,d.originIndex))return false;
    pass(8,state.correctionPublication.observed&&!state.correctionPublication.consumed);
    const int64_t offset=int64_t(trace.firstEventId)-base-1;
    // Bounded proven case: the native client cancelled a known pending prefix.
    // Forward gaps containing unseen old input remain an explicit unproven gate.
    if(!pass(9,trace.baseBefore==base&&!trace.forwardGap&&trace.firstEventId>=0&&trace.firstEventId<=INT32_MAX-4
                &&offset>0&&offset<=trace.countBefore
                &&offset+5<=count&&count<=int32_t(Configuration.maxInputEvents*2))){
        d.stage="network_input_correction_checkpoint_outside_retained_native_prefix_unproven";return true;
    }
    NativeCheckpointStream wire;
    NativeCheckpointReader replay(Read<const uint8_t*>(source.data,0x98),Read<uint64_t>(source.data,0xa8));
    std::array<uint8_t,256> discarded{};
    uint64_t remaining=inputStart;
    while(remaining){const auto chunk=(std::min)(remaining,uint64_t(discarded.size()*8));
        if(!Bits(replay.reader,discarded.data(),chunk))return Fail(3334,"network_input_correction_checkpoint_reader_prefix_failed");
        remaining-=chunk;
    }
    ++state.nativeCheckpointDecodes;
    const auto result=engine::function<uint8_t(*)(void*,void*,void*,uintptr_t)>(0x70a9e50)
        (wire.data,replay.reader.data,state.nativeNetDriver,0);
    const int32_t wireCount=Read<int32_t>(wire.data,0x740);auto wireEvents=Read<const uint8_t*>(wire.data,0x738);
    if(result==2||ReaderError(replay.reader)||Position(replay.reader)!=componentEnd
       ||Read<int32_t>(wire.data,4)!=trace.firstEventId-1||wireCount<5
       ||wireCount>static_cast<int32_t>(Configuration.maxInputEvents)||!wireEvents)
        return Fail(3335,"network_input_original_correction_checkpoint_wire_decode_failed");
    CaptureCheckpointDecodeDiagnostic(wireEvents,wireCount,
        Read<const uint8_t*>(state.incomingEvents,0x738)+static_cast<size_t>(offset)*40,
        count-static_cast<int32_t>(offset),trace);
    pass(10,Read<uint8_t>(wireEvents)==6&&Read<uint8_t>(wireEvents,40)==9&&Read<uint8_t>(wireEvents,80)==4);
    pass(11,Read<int32_t>(wireEvents,8)==d.authIndex);
    d.reportedIdentity=Read<WeakIdentity>(wireEvents,88);d.reported=ResolveWeak(d.reportedIdentity);
    pass(12,d.reported==d.corrected&&(!d.corrected||(original_players::IsA(d.corrected,0x645c660)
        &&Read<void*>(d.corrected,0x210)==state.pawn&&Read<void*>(d.corrected,0x4a8)==state.pawn
        &&SameWeak(d.reportedIdentity,state.correctionPublication.correctedIdentity))));
    auto retained=Read<const uint8_t*>(state.incomingEvents,0x738)+static_cast<size_t>(offset)*40;
    pass(13,SameCheckpointField(retained,wireEvents,6)&&SameCheckpointField(retained+40,wireEvents+40,9)
        &&SameCheckpointField(retained+80,wireEvents+80,4));
    pass(14,SameReplacementFrame(retained,wireEvents,trace.firstEventId,static_cast<uint32_t>(count-offset),static_cast<uint32_t>(wireCount)));
    if(d.passedMask!=0x7fff){d.stage="network_input_waiting_matching_original_correction_checkpoint";return true;}
    const int32_t cancelThrough=trace.firstEventId-1;
    engine::function<void(*)(void*,int32_t)>(0x70a9dd0)(state.incomingEvents,cancelThrough);
    d.newBase=Read<int32_t>(state.incomingEvents,4);d.newCount=Read<int32_t>(state.incomingEvents,0x740);
    retained=Read<const uint8_t*>(state.incomingEvents,0x738);
    if(d.newBase!=cancelThrough||d.newCount!=count-static_cast<int32_t>(offset)
       ||!SameCheckpointField(retained,wireEvents,6)||!SameCheckpointField(retained+40,wireEvents+40,9)
       ||!SameCheckpointField(retained+80,wireEvents+80,4)||!SameReplacementFrame(retained,wireEvents,trace.firstEventId,static_cast<uint32_t>(d.newCount),static_cast<uint32_t>(wireCount)))
        return Fail(3336,"network_input_original_correction_cancellation_prune_readback_failed");
    d.cancelledCount=static_cast<uint32_t>(offset);++state.nativeCorrectionCancellations;
    state.nativeCorrectionCancelledEvents+=d.cancelledCount;state.lastCancelledCorrectionIndex=d.authIndex;
    state.correctionPublication.consumed=true;
    for(auto& barrier:state.barrierTimes)if(barrier.present&&barrier.eventId<=cancelThrough)barrier={};
    uint32_t write=0;
    for(uint32_t i=0;i<state.watermarkCount;++i)if(state.watermarks[i].eventId>cancelThrough)
        state.watermarks[write++]=state.watermarks[i];
    state.watermarkCount=write;
    // No applied-ID, input target, correction counter or RPC is supplied here.
    // Original Apply must execute all of the retained 6/9/4 checkpoint itself.
    d.stage="network_input_original_correction_cancelled_known_prefix_checkpoint_retained";
    return true;
}
inline bool ValidMove(const Move& m){
    const double limit=Configuration.maxIntentMagnitude;
    if(m.type!=0||!m.index||!Finite(m.intent)||!Finite(m.claimedLocation)||!std::isfinite(m.crouch)||!std::isfinite(m.timeShift))
        return Fail(3220,"network_input_move_payload_invalid");
    if(m.intent.x*m.intent.x+m.intent.y*m.intent.y+m.intent.z*m.intent.z>limit*limit)
        return Fail(3221,"network_input_intent_outside_bounded_native_domain");
    if(m.base)return Fail(3222,"network_input_moving_base_authority_branch_unproven");
    return true;
}
inline uint32_t SampleSequenceClock(){return state.clock?Read<uint32_t>(state.clock,state.clockBaseOffset+0x18):0;}
inline void CaptureSequencePacket(const std::array<Move,16>& moves,uint32_t count,Reader& reader){
    SequencePacket packet;packet.packet=state.receivedPackets;packet.count=count;
    packet.expected=NextIndex(state.lastConsumedIndex);packet.clock=SampleSequenceClock();
    packet.consumed=state.lastConsumedIndex;packet.queued=state.queueCount;
    if(count){packet.first=moves[0].index;packet.last=moves[count-1].index;
        packet.firstStep=moves[0].firstStep;packet.timeShift=moves[0].timeShift;
        if(!state.firstMoveBatchCaptured){
            state.firstMoveBatchCaptured=true;state.firstMoveBatch=packet;state.firstMoveBatchRecords=moves;
            state.firstMovePacketBytes=state.lastPacketBytes;state.firstMovePacketBits=state.lastPacketBits;
            std::memcpy(state.firstMovePacketData.data(),Read<void*>(reader.data,0x98),state.firstMovePacketBytes);
        }}
    if(state.sequencePacketCount<state.sequencePackets.size())state.sequencePackets[state.sequencePacketCount++]=packet;
}
inline bool SequenceFailure(uint32_t error,const char* stage,const std::array<Move,16>& moves,uint32_t count,uint32_t ordinal,Reader& reader){
    auto& fault=state.sequenceFault;
    if(!fault.captured){
        fault.captured=true;fault.error=error;fault.ordinal=ordinal;fault.packet=state.receivedPackets;
        fault.packetCount=count;fault.index=ordinal<count?moves[ordinal].index:0;
        fault.expected=NextIndex(state.lastConsumedIndex);fault.clock=SampleSequenceClock();
        fault.consumed=state.lastConsumedIndex;fault.lastReceived=state.lastReceivedIndex;
        fault.clockOffset=state.clockIndexOffset;
        fault.mappedIndex=state.wireAligned?ClientToHostIndex(fault.index):fault.index;
        fault.ahead=static_cast<int32_t>(fault.mappedIndex-state.lastConsumedIndex);
        fault.queueCount=state.queueCount;fault.readerBit=static_cast<uint32_t>(Position(reader));fault.moves=moves;
        fault.budgetSeconds=state.hostBudgetSeconds;fault.clockAccumulated=state.clockAccumulated;fault.hostElapsedSeconds=state.hostElapsedSeconds;
        state.sequenceFaultPacketBytes=state.lastPacketBytes;state.sequenceFaultPacketBits=state.lastPacketBits;
        std::memcpy(state.sequenceFaultPacketData.data(),Read<void*>(reader.data,0x98),state.sequenceFaultPacketBytes);
        bool first=true;
        for(const auto& slot:state.queue)if(slot.present){
            if(first){fault.queueMinimum=fault.queueMaximum=slot.move.index;first=false;}
            else{fault.queueMinimum=(std::min)(fault.queueMinimum,slot.move.index);fault.queueMaximum=(std::max)(fault.queueMaximum,slot.move.index);}
        }
    }
    return Fail(error,stage);
}
inline bool Discrepancy(Reader& reader){
    const uint64_t total=Read<uint64_t>(reader.data,0xa8),position=Position(reader);
    if(position==total){++state.pddAbsentPackets;return true;}
    uint8_t tag=0;
    if(!Bits(reader,&tag,8)||tag!=0xa4)return Fail(3233,"network_input_discrepancy_component_tag_invalid");
    const uint64_t payload=total-Position(reader);
    if(!payload){++state.pddHeaderOnlyPackets;return true;}
    // Exact writer6F4F6C0: optional team bit, attribute bit, native status byte,
    // int32 count and int32 IDs. Optional team changes the total modulo32 from
    //9 to10. This is a structural decode of retained client reports only: no
    // movement, visibility or permission decision trusts those claims.
    if((Read<uint8_t>(reader.data,0x2a)&2)!=0)
        return Fail(3234,"network_input_discrepancy_swapped_archive_unproven");
    const uint64_t form=payload&31;
    if(payload<41||(form!=9&&form!=10))return Fail(3259,"network_input_discrepancy_payload_shape_unproven");
    const bool team=form==10;uint8_t teamBit=0,attribute=0,status=0;int32_t count=0;
    if((team&&!Bits(reader,&teamBit,1))||!Bits(reader,&attribute,1)||!Bits(reader,&status,8)||!Bits(reader,&count,32))
        return Fail(3260,"network_input_discrepancy_payload_truncated");
    if(count<0||static_cast<uint32_t>(count)>Configuration.maxPddPlayerIds
       ||payload!=(team?42ULL:41ULL)+static_cast<uint64_t>(count)*32)
        return Fail(3261,"network_input_discrepancy_player_count_invalid");
    for(int32_t i=0;i<count;++i)if(!Bits(reader,&state.pddReportedPlayerIds[static_cast<size_t>(i)],32))
        return Fail(3262,"network_input_discrepancy_player_ids_truncated");
    if(Position(reader)!=total)return Fail(3263,"network_input_discrepancy_trailing_bits_unknown");
    state.pddTeamPresent=static_cast<uint8_t>(team);state.pddTeamBit=teamBit;
    state.pddAttributeFlag=attribute;state.pddStatus=status;state.pddPlayerCount=static_cast<uint32_t>(count);
    ++state.pddPayloadPackets;return true;
}
inline bool Decode(Reader& reader){
    std::array<Move,16> moves{};uint32_t count=0;uint8_t tag=0,marker=0,expected=1;
    if(!Bits(reader,&tag,8)||tag!=0x52)return Fail(3223,"network_input_movement_tag_invalid");
    for(;;){
        marker=0;if(!Bits(reader,&marker,3))return Fail(3224,"network_input_movement_marker_truncated");
        if(!marker)break;
        if(count>=Configuration.maxMovesPerPacket||marker!=expected)return Fail(3225,"network_input_movement_marker_sequence_invalid");
        auto& move=moves[count];
        const uint8_t result=engine::function<uint8_t(*)(void*,void*,Move*)>(0x7893760)(state.movement,reader.data,&move);
        if(result==2||ReaderError(reader)||!ValidMove(move))return state.error?false:Fail(3226,"network_input_native_move_decoder_failed");
        if(count&&move.index!=NextIndex(moves[count-1].index))return Fail(3227,"network_input_packet_move_indices_not_consecutive");
        ++count;expected=expected==7?1:expected+1;
    }
    CaptureSequencePacket(moves,count,reader);
    uint16_t end=0;
    if(!Bits(reader,&end,16)||end<Position(reader)+9||end>Read<uint64_t>(reader.data,0xa8))
        return Fail(3228,"network_input_input_component_end_invalid");
    state.lastInputEndBit=end;tag=0;
    if(!Bits(reader,&tag,8)||tag!=0xa3)return Fail(3229,"network_input_input_component_tag_invalid");
    uint8_t present=0;
    if(!Bits(reader,&present,1))return Fail(3230,"network_input_input_presence_truncated");
    const uint64_t nativeInputStart=Position(reader);
    const int32_t eventsBefore=Read<int32_t>(state.incomingEvents,0x740);
    const int64_t eventEndBefore=int64_t(Read<int32_t>(state.incomingEvents,4))+eventsBefore;
    if(present){
        InputWireTrace trace;trace.packet=state.receivedPackets;
        trace.baseBefore=Read<int32_t>(state.incomingEvents,4);trace.countBefore=eventsBefore;
        trace.headerKnown=PeekInputEventId(reader,end,trace.firstEventId);
        trace.forwardGap=trace.headerKnown&&int64_t(trace.firstEventId)>int64_t(trace.baseBefore)+trace.countBefore+1;
        const uint8_t result=engine::function<uint8_t(*)(void*,void*,void*,uintptr_t)>(0x70a9e50)
            (state.incomingEvents,reader.data,state.nativeNetDriver,0);
        trace.nativeResult=result;trace.baseAfter=Read<int32_t>(state.incomingEvents,4);
        trace.countAfter=Read<int32_t>(state.incomingEvents,0x740);state.lastInputWire=trace;
        if(trace.forwardGap)++state.nativeInputWireGaps;
        if(state.inputWireTraceCount<state.inputWireTraces.size())state.inputWireTraces[state.inputWireTraceCount++]=trace;
        if(result==2||ReaderError(reader)){
            // Retain the one actual rejected local packet and exact original
            // codec result. This adds no acceptance/rewind/cancellation path.
            if(!state.failedDecoderPacketBytes){
                state.failedDecoderPacket=state.receivedPackets;state.failedDecoderResult=result;
                state.failedDecoderStart=nativeInputStart;state.failedDecoderPosition=Position(reader);
                state.failedDecoderReaderError=ReaderError(reader);
                state.failedDecoderPacketBytes=state.lastPacketBytes<=state.failedDecoderPacketData.size()?state.lastPacketBytes:0;
                state.failedDecoderPacketBits=state.lastPacketBits;
                if(state.failedDecoderPacketBytes)std::memcpy(state.failedDecoderPacketData.data(),Read<void*>(reader.data,0x98),state.failedDecoderPacketBytes);
            }
            return Fail(3231,"network_input_native_input_decoder_failed");
        }
    }
    if(Position(reader)>end||(!present&&Position(reader)!=end))return Fail(3232,"network_input_input_decoder_crossed_component_bound");
    // The native stream reader can return early for an already-received event
    // range. 7171C00 uses this same absolute component end to advance the reader.
    Put<uint64_t>(reader.data,0xb0,end);
    if(!Discrepancy(reader))return false;
    int32_t eventCount=Read<int32_t>(state.incomingEvents,0x740);
    if(eventCount<0||eventCount>static_cast<int32_t>(Configuration.maxInputEvents*2)
       ||Read<int32_t>(state.incomingEvents,0x744)<eventCount)
        return Fail(3235,"network_input_native_event_queue_invalid");
    if(present&&!HonorCorrectionCancellation(reader,nativeInputStart,end))return false;
    eventCount=Read<int32_t>(state.incomingEvents,0x740);
    if(eventCount<0||eventCount>static_cast<int32_t>(Configuration.maxInputEvents))
        return Fail(3235,"network_input_native_event_queue_invalid");
    if(count&&!state.wireAligned){if(!InitializeWireOrigin(moves[0]))return false;}
    else if(count&&state.wireAligned&&!state.queueCount&&state.lastKnownInputValid&&state.heldSinceObserved
            &&static_cast<int32_t>(moves[count-1].index-state.lastReceivedClientIndex)>0){
        // After an authority-only hold, a newly observed client command can be
        // behind the already simulated host. Rebase only an empty queue and
        // only a recent, genuinely new command, using the same native seed.
        for(uint32_t i=0;i<count;++i)if(static_cast<int32_t>(moves[i].index-state.lastObservedClientIndex)>0){
            const uint32_t mapped=ClientToHostIndex(moves[i].index);
            const int32_t ahead=static_cast<int32_t>(mapped-state.lastConsumedIndex);
            if(ahead<=0&&static_cast<uint32_t>(-int64_t(ahead))<=Configuration.maxFutureSteps)
                if(!InitializeWireOrigin(moves[i]))return false;
            break;
        }
    }
    if(!MapBarrierDeadlines())return false;
    std::array<Move,16> hostMoves=moves;
    for(uint32_t i=0;i<count;++i)hostMoves[i].index=ClientToHostIndex(moves[i].index);
    uint32_t newCount=0;
    for(uint32_t i=0;i<count;++i){
        const auto& move=hostMoves[i];const int32_t ahead=static_cast<int32_t>(move.index-state.lastConsumedIndex);
        if(ahead<=0){++state.duplicateMoves;
            if(static_cast<uint32_t>(-int64_t(ahead))>Configuration.maxFutureSteps)++state.expiredClientMoves;
            continue;}
        if(static_cast<uint32_t>(ahead)>Configuration.maxFutureSteps)
            return SequenceFailure(3236,"network_input_move_outside_host_sequence_window",moves,count,i,reader);
        auto& slot=state.queue[move.index&(Configuration.queueCapacity-1)];
        if(slot.present){
            if(slot.move.index!=move.index)return SequenceFailure(3237,"network_input_bounded_queue_collision",moves,count,i,reader);
            if(std::memcmp(&slot.move,&move,sizeof(move))!=0)return SequenceFailure(3238,"network_input_duplicate_move_payload_changed",moves,count,i,reader);
            ++state.duplicateMoves;continue;
        }
        ++newCount;
    }
    if(state.queueCount+newCount>Configuration.queueCapacity)return Fail(3239,"network_input_move_queue_full");
    for(uint32_t i=0;i<count;++i){
        const auto& move=hostMoves[i];if(static_cast<int32_t>(move.index-state.lastConsumedIndex)<=0)continue;
        auto& slot=state.queue[move.index&(Configuration.queueCapacity-1)];
        if(!slot.present){slot.move=move;slot.clientIndex=moves[i].index;slot.present=true;++state.queueCount;++state.uniqueMoves;}
        if(static_cast<int32_t>(move.index-state.lastReceivedIndex)>0)state.lastReceivedIndex=move.index;
        if(static_cast<int32_t>(moves[i].index-state.lastReceivedClientIndex)>0)state.lastReceivedClientIndex=moves[i].index;
    }
    if(newCount)state.lastFreshMovementHostSeconds=state.hostElapsedSeconds;
    const int64_t eventEnd=int64_t(Read<int32_t>(state.incomingEvents,4))+eventCount;
    const bool cancelledThisPacket=state.correctionCancellation.packet==state.receivedPackets
        &&state.correctionCancellation.cancelledCount!=0;
    if(eventCount&&(eventEnd>eventEndBefore||cancelledThisPacket)){
        if(state.watermarkCount>=Configuration.maxWatermarks)return Fail(3240,"network_input_event_watermark_queue_full");
        // Adapter timing rule: a newly received event range becomes eligible
        // at the packet's last movement index; input-only packets use the next
        // command index. This explicit policy is not a recovered server body.
        auto& watermark=state.watermarks[state.watermarkCount++];
        watermark.index=count?hostMoves[count-1].index:NextIndex(state.lastReceivedIndex);
        watermark.eventId=Read<int32_t>(state.incomingEvents,4)+eventCount;
    }
    state.pendingEvents=static_cast<uint32_t>(eventCount);state.decodedMoves+=count;
    state.lastReaderBit=static_cast<uint32_t>(Position(reader));++state.acceptedPackets;
    local_server_human_funded_boundary_probe::AfterAccepted(state);
    state.stage="network_input_received_native_commands_runtime_unproven";return true;
}
inline bool ReceiveImpl(void* pawn,const ComponentDataStream* input){
    ++state.receivedPackets;
    if(state.error)return false;
    const bool threadMatches=Thread(),linksMatch=OwnedLinks();
    const int connectionStatus=ConnectionStatus();
    if(!state.installed||!threadMatches||pawn!=state.pawn||connectionStatus<0)
        return Fail(3241,"network_input_rpc_owner_or_thread_invalid");
    if(!connectionStatus){++state.closedConnectionPackets;return false;}
    if(!linksMatch)return Fail(3241,"network_input_rpc_owner_or_thread_invalid");
    if(state.inReceive)return Fail(3242,"network_input_reentrant_native_receiver");
    state.inReceive=true;
    const ComponentDataStream stream=Read<ComponentDataStream>(input);
    if(stream.bytes<=0||stream.bytes>static_cast<int32_t>(Configuration.maxPacketBytes)||stream.capacity<stream.bytes
       ||stream.unusedBits>=8||!stream.data||!original_players::Readable(stream.data,static_cast<size_t>(stream.bytes))){
        state.inReceive=false;return Fail(3243,"network_input_component_stream_storage_invalid");
    }
    const uint32_t bits=static_cast<uint32_t>(stream.bytes)*8-stream.unusedBits;
    if(!bits||bits>Configuration.maxPacketBits){state.inReceive=false;return Fail(3244,"network_input_component_stream_bit_count_invalid");}
    state.lastPacketBytes=static_cast<uint32_t>(stream.bytes);state.lastPacketBits=bits;
    Reader reader;
    engine::function<void*(*)(void*,const uint8_t*,int64_t)>(0x1a5d990)(reader.data,stream.data,bits);
    const bool decoded=Decode(reader);
    engine::function<void*(*)(void*,uint32_t)>(0x1a5dab0)(reader.data,0);
    state.inReceive=false;if(!decoded)++state.rejectedPackets;
    return decoded;
}
inline LONG Capture(EXCEPTION_POINTERS* e){
    const auto pc=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=pc>=engine::image?pc-engine::image:pc;
    state.inReceive=false;state.inPoll=false;
    Fail(e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0003200U,"network_input_native_exception_requires_new_session");
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool ForwardRetiredPacket(void* pawn,const ComponentDataStream* stream){
    if(state.installed&&pawn==state.pawn)return false;
    for(auto it=epochs.bindings.rbegin();it!=epochs.bindings.rend();++it)if(it->pawn==pawn){
        if(GetCurrentThreadId()!=epochs.gameThread||original_players::gameThread!=epochs.gameThread){
            Fail(3241,"network_input_retired_rpc_wrong_game_thread");return true;
        }
        // Original ActorNetGUID/channel routing selects the UObject. A retired
        // instance never enters the new epoch's codec, sequence or ACK path.
        state.retiredWeakResolvedPawn=ResolveWeak(it->identity);
        ++epochs.latePackets;EpochCounters();
        if(it->originalReceiver)reinterpret_cast<void(*)(void*,const ComponentDataStream*)>(it->originalReceiver)(pawn,stream);
        return true;
    }
    return false;
}
inline void Receive(void* pawn,const ComponentDataStream* stream){
    __try {if(!ForwardRetiredPacket(pawn,stream))ReceiveImpl(pawn,stream);}__except(Capture(GetExceptionInformation())){}
}
inline bool InstallAuthority(void* world,void* pc,void* pawn,void* connection){
    __try {return InstallAuthorityImpl(world,pc,pawn,connection);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool ReplaceViewSlot(void* expected,void* replacement){
    if(!state.privatePawnTable||!state.privateTableReadOnly
       ||Read<void*>(state.privatePawnTable,StreamSlot)!=reinterpret_cast<void*>(&Receive))
        return Fail(3280,"network_input_composed_view_table_owner_invalid");
    DWORD previous=0;
    if(!VirtualProtect(state.privatePawnTable-1,(PawnTableEntries+1)*8,PAGE_READWRITE,&previous))
        return Fail(3281,"network_input_composed_view_table_write_protection_failed");
    auto slot=reinterpret_cast<void* volatile*>(state.privatePawnTable+0x850/8);
    auto actual=InterlockedCompareExchangePointer(slot,replacement,expected);
    DWORD ignored=0;
    state.privateTableReadOnly=VirtualProtect(state.privatePawnTable-1,(PawnTableEntries+1)*8,PAGE_READONLY,&ignored)!=0;
    if(actual!=expected||!state.privateTableReadOnly||Read<void*>(state.privatePawnTable,0x850)!=replacement
       ||Read<void*>(state.privatePawnTable,StreamSlot)!=reinterpret_cast<void*>(&Receive))
        return Fail(3282,"network_input_composed_view_slot_readback_failed");
    return true;
}
inline bool SetViewLocationObserverImpl(void* pawn,NativeView observer,NativeView* previous){
    const auto original=reinterpret_cast<void*>(engine::image+0x787f820);
    if(previous)*previous=nullptr;
    if(state.error||!state.installed||state.epochRetired||pawn!=state.pawn||!Thread()||!OwnedLinks()
       ||ConnectionStatus()!=1||!observer||reinterpret_cast<void*>(observer)==original)
        return Fail(3283,"network_input_composed_view_observer_owner_invalid");
    if(state.viewObserverInstalled){
        if(state.viewObserver!=reinterpret_cast<void*>(observer)||Read<void*>(state.privatePawnTable,0x850)!=state.viewObserver)
            return Fail(3284,"network_input_composed_view_observer_conflict");
    }else{
        if(!ReplaceViewSlot(original,reinterpret_cast<void*>(observer)))return false;
        state.viewObserver=reinterpret_cast<void*>(observer);state.viewObserverInstalled=true;++state.viewObserverInstalls;
    }
    if(previous)*previous=engine::function<NativeView>(0x787f820);
    return true;
}
inline bool ClearViewLocationObserverImpl(void* pawn,NativeView observer){
    if(!Thread())return Fail(3285,"network_input_composed_view_restore_wrong_thread");
    if(pawn!=state.pawn){
        for(const auto& old:epochs.bindings)if(old.pawn==pawn)return true;
        return Fail(3286,"network_input_composed_view_restore_unknown_pawn");
    }
    // Cleanup may follow unpossess or pending-kill. Only our own table memory
    // is changed here; an expired old Actor is never read or written.
    auto resolved=ResolveWeak(state.pawnIdentity);
    if(resolved&&resolved!=pawn)return Fail(3287,"network_input_composed_view_restore_weak_identity_changed");
    const auto original=reinterpret_cast<void*>(engine::image+0x787f820);
    if(!state.viewObserverInstalled)return state.privatePawnTable&&Read<void*>(state.privatePawnTable,0x850)==original;
    if(state.viewObserver!=reinterpret_cast<void*>(observer))return Fail(3284,"network_input_composed_view_observer_conflict");
    if(!ReplaceViewSlot(state.viewObserver,original))return false;
    state.viewObserverInstalled=false;state.viewObserver=nullptr;++state.viewObserverRestores;return true;
}
inline bool SetViewLocationObserver(void* pawn,NativeView observer,NativeView* previous){
    __try{return SetViewLocationObserverImpl(pawn,observer,previous);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool ClearViewLocationObserver(void* pawn,NativeView observer){
    __try{return ClearViewLocationObserverImpl(pawn,observer);}__except(Capture(GetExceptionInformation())){return false;}
}

// A composed authority observer changes only this already-owned table's +6B8.
// Other mutable entries, notably the +850 view observer, retain their identity.
inline bool OriginalAuthorityTakeDamageProfile(){
    auto code=reinterpret_cast<const uint8_t*>(engine::image+0x787b3d0);
    if(!original_players::Readable(code,421))return false;
    uint64_t hash=1469598103934665603ULL;
    for(size_t i=0;i<421;++i)hash=(hash^code[i])*1099511628211ULL;
    return hash==0x416bd6f3c997e56fULL;
}
inline bool ReplaceAuthorityTakeDamageSlot(void* expected,void* replacement,bool recovering=false){
    if(!state.privatePawnTable||(!recovering&&!state.privateTableReadOnly)
       ||Read<void*>(state.privatePawnTable,StreamSlot)!=reinterpret_cast<void*>(&Receive))
        return Fail(3490,"network_input_composed_damage_table_owner_invalid");
    DWORD previous=0;
    if(!VirtualProtect(state.privatePawnTable-1,(PawnTableEntries+1)*8,PAGE_READWRITE,&previous))
        return Fail(3491,"network_input_composed_damage_write_protection_failed");
    auto slot=reinterpret_cast<void* volatile*>(state.privatePawnTable+0x6b8/8);
    auto actual=InterlockedCompareExchangePointer(slot,replacement,expected);
    // Publication is recorded before a later protection operation can fail.
    // The root callback owner also publishes its Binding before entering here.
    if(actual==expected){
        const auto original=reinterpret_cast<void*>(engine::image+0x787b3d0);
        state.takeDamageObserver=replacement==original?nullptr:replacement;
        state.takeDamageObserverInstalled=replacement!=original;
    }
    DWORD ignored=0;
    state.privateTableReadOnly=VirtualProtect(state.privatePawnTable-1,(PawnTableEntries+1)*8,PAGE_READONLY,&ignored)!=0;
    if(actual!=expected||!state.privateTableReadOnly||Read<void*>(state.privatePawnTable,0x6b8)!=replacement
       ||Read<void*>(state.privatePawnTable,StreamSlot)!=reinterpret_cast<void*>(&Receive))
        return Fail(3492,"network_input_composed_damage_slot_readback_failed");
    return true;
}
inline bool InstallAuthorityTakeDamageObserverImpl(void* pawn,NativeTakeDamage observer,NativeTakeDamage& forward){
    forward=nullptr;
    if(!state.installed||state.epochRetired||pawn!=state.pawn)
        return Wait("network_input_composed_damage_waiting_installed_authority_epoch");
    const auto original=reinterpret_cast<void*>(engine::image+0x787b3d0);
    if(!Thread()||state.error||!OwnedLinks()||Read<void**>(pawn)!=state.privatePawnTable
       ||!observer||reinterpret_cast<void*>(observer)==original||!OriginalAuthorityTakeDamageProfile())
        return Fail(3493,"network_input_composed_damage_observer_source_invalid");
    if(state.inPoll||state.inReceive||ConnectionStatus()!=1)
        return Wait("network_input_composed_damage_waiting_nonreentrant_open_connection");
    if(state.takeDamageObserverInstalled){
        if(state.takeDamageObserver!=reinterpret_cast<void*>(observer)
           ||Read<void*>(state.privatePawnTable,0x6b8)!=state.takeDamageObserver)
            return Fail(3494,"network_input_composed_damage_observer_conflict");
    }else{
        if(!ReplaceAuthorityTakeDamageSlot(original,reinterpret_cast<void*>(observer)))return false;
        state.takeDamageObserver=reinterpret_cast<void*>(observer);state.takeDamageObserverInstalled=true;
    }
    forward=engine::function<NativeTakeDamage>(0x787b3d0);return true;
}
inline bool ClearAuthorityTakeDamageObserverImpl(void* pawn,NativeTakeDamage observer){
    if(!Thread())return Fail(3495,"network_input_composed_damage_restore_wrong_thread");
    if(pawn!=state.pawn){
        for(const auto& old:epochs.bindings)if(old.pawn==pawn)return true;
        return Fail(3496,"network_input_composed_damage_restore_unknown_epoch");
    }
    // The original Pawn can already be unpossessed or pending-kill. Touch only
    // our still-owned table storage, not a retired UObject or replacement Pawn.
    auto resolved=ResolveWeak(state.pawnIdentity);
    if(resolved&&resolved!=pawn)return Fail(3497,"network_input_composed_damage_restore_weak_identity_changed");
    const auto original=reinterpret_cast<void*>(engine::image+0x787b3d0);
    if(!state.takeDamageObserverInstalled){
        if(!state.privatePawnTable||Read<void*>(state.privatePawnTable,0x6b8)!=original)return false;
        return state.privateTableReadOnly||ReplaceAuthorityTakeDamageSlot(original,original,true);
    }
    if(state.takeDamageObserver!=reinterpret_cast<void*>(observer))
        return Fail(3494,"network_input_composed_damage_observer_conflict");
    if(!ReplaceAuthorityTakeDamageSlot(state.takeDamageObserver,original,true))return false;
    state.takeDamageObserver=nullptr;state.takeDamageObserverInstalled=false;return true;
}
inline bool InstallAuthorityTakeDamageObserver(void* pawn,NativeTakeDamage observer,NativeTakeDamage& forward){
    __try{return InstallAuthorityTakeDamageObserverImpl(pawn,observer,forward);}
    __except(Capture(GetExceptionInformation())){forward=nullptr;return false;}
}
inline bool ClearAuthorityTakeDamageObserver(void* pawn,NativeTakeDamage observer){
    __try{return ClearAuthorityTakeDamageObserverImpl(pawn,observer);}
    __except(Capture(GetExceptionInformation())){return false;}
}

inline bool RetireAuthorityImpl(void* pawn){
    ++state.retirementCalls;
    const uint32_t expectedThread=state.gameThread?state.gameThread:original_players::gameThread;
    if(!expectedThread||GetCurrentThreadId()!=expectedThread||original_players::gameThread!=expectedThread)
        return Fail(3274,"network_input_retirement_wrong_game_thread");
    if(state.inReceive||state.inPoll)return Wait("network_input_waiting_nonreentrant_epoch_retirement");
    if(pawn!=state.pawn){
        for(const auto& old:epochs.bindings)if(old.pawn==pawn)return true;
        if(!state.installed&&!state.incomingEventsConstructed&&!state.pawn)return true;
        return Fail(3275,"network_input_retirement_unknown_pawn");
    }
    if(state.epochRetired)return true;
    if(state.viewObserverInstalled)return Fail(3288,"network_input_retirement_requires_view_observer_clear");
    if(state.takeDamageObserverInstalled)return Fail(3498,"network_input_retirement_requires_damage_observer_clear");
    if(!state.installed&&!state.incomingEventsConstructed){
        if(!state.error){state=State{};state.epochRetired=epochs.anchored;EpochCounters();}
        return true;
    }
    if(!state.identitiesCaptured)return Fail(3276,"network_input_retirement_missing_original_weak_identity");
    state.retiredWeakResolvedPawn=ResolveWeak(state.pawnIdentity);
    if(state.retiredWeakResolvedPawn&&state.retiredWeakResolvedPawn!=pawn)
        return Fail(3277,"network_input_retirement_original_weak_identity_changed");
    state.retiredPawnExpired=state.retiredWeakResolvedPawn==nullptr;
    const int32_t pending=Read<int32_t>(state.incomingEvents,0x740),capacity=Read<int32_t>(state.incomingEvents,0x744);
    if(pending<0||capacity<pending||capacity>4096)
        return Fail(3278,"network_input_retirement_native_event_storage_invalid");
    if(state.incomingEventsConstructed){
        engine::function<void(*)(void*)>(0x70aa200)(state.incomingEvents);++state.streamClearCalls;
        if(Read<void*>(state.incomingEvents,0x738)||Read<int32_t>(state.incomingEvents,0x740)||Read<int32_t>(state.incomingEvents,0x744))
            return Fail(3279,"network_input_retirement_native_event_clear_failed");
        engine::function<void*(*)(void*)>(0x70aa150)(state.incomingEvents);++state.streamResetCalls;
        if(Read<int32_t>(state.incomingEvents,4)!=-1)return Fail(3290,"network_input_retirement_native_event_reset_failed");
        for(size_t i=0;i<46;++i)if(Read<uint8_t>(state.incomingEvents,8+i*40)!=0xfe)
            return Fail(3290,"network_input_retirement_native_event_reset_failed");
    }
    state.retiredSourceRestored=false;
    state.actualPawnTable=state.retiredWeakResolvedPawn?Read<void**>(pawn):nullptr;
    if(state.retiredWeakResolvedPawn&&state.privatePawnTable&&state.actualPawnTable==state.privatePawnTable){
        if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(pawn),state.sourcePawnTable,state.privatePawnTable)!=state.privatePawnTable)
            return Fail(3291,"network_input_retirement_pawn_table_changed_during_restore");
        state.actualPawnTable=Read<void**>(pawn);
        if(state.actualPawnTable!=state.sourcePawnTable)return Fail(3291,"network_input_retirement_pawn_source_restore_readback_failed");
        state.retiredSourceRestored=true;epochs.reusablePawnTable=state.privatePawnTable;
    }
    const auto originalReceiver=Read<void*>(state.sourcePawnTable,StreamSlot);
    if(originalReceiver!=reinterpret_cast<void*>(engine::image+0x18a3a00))
        return Fail(3292,"network_input_retirement_original_receiver_changed");
    epochs.bindings.push_back({pawn,state.pawnIdentity,originalReceiver});
    if(state.mappingClockConstructed){
        // Standalone common clock has an original virtual base at +9A0. The
        // original deleting destructor with flags zero frees its owned history
        // while retaining this module's aligned object storage.
        engine::function<void*(*)(void*,uint32_t)>(0x7856ca0)
            (static_cast<uint8_t*>(state.mappingClock)+0x9a0,0);
        state.mappingClockConstructed=false;++state.mappingClockDestructions;
    }
    if(state.error||state.restartRequired){epochs.blockedError=state.error?state.error:3293;epochs.blockedStage=state.stage;}
    epochs.discardedMoves+=state.queueCount;epochs.discardedEvents+=static_cast<uint32_t>(pending);
    epochs.lastRetiredPawn=pawn;epochs.lastRetiredIdentity=state.pawnIdentity;++epochs.retired;
    state.queue={};state.watermarks={};state.barrierTimes={};state.queueCount=0;state.watermarkCount=0;state.pendingEvents=0;
    state.lastReceivedIndex=state.lastConsumedIndex=state.lastAcknowledgedIndex=0;
    state.latestAppliedEventId=state.lastAckEventId=-1;state.correctionId=0;state.forceHardCorrection=false;
    state.hostRetiredFundedSeconds+=(std::max)(0.0,double(state.clockAccumulated));
    state.hostRetiredUnfundedSeconds+=state.hostUnfundedSeconds;
    state.hostFundedWorkSeconds=state.hostUnfundedSeconds=0;state.hostBudgetSeconds=0;
    state.wireAligned=false;state.lastKnownInputValid=false;state.heldSinceObserved=0;
    state.installed=false;state.epochRetired=true;EpochCounters();
    if(!AuditHostTime())return false;
    state.stage=epochs.blockedError?"network_input_epoch_retired_requires_new_session":"network_input_epoch_retired";
    return true;
}
inline bool RetireAuthority(void* pawn){
    __try{return RetireAuthorityImpl(pawn);}__except(Capture(GetExceptionInformation())){return false;}
}

inline void CaptureInputApplyDiagnostic(uint32_t index,int32_t eligible,int32_t prefix){
    auto& d=state.inputApplyDiagnostic;d.attempt=state.inputApplyCalls+1;d.hostIndex=index;
    d.hostSeconds=static_cast<float>(index)*(1.0f/128.0f);d.eligible=eligible;d.prefix=static_cast<uint32_t>(prefix);
    d.base=Read<int32_t>(state.incomingEvents,4);d.count=Read<int32_t>(state.incomingEvents,0x740);
    d.appliedBefore=Read<int32_t>(state.inputState,0x120c);d.appliedAfter=d.appliedBefore;
    d.reportedTarget=Read<void*>(state.inputState,0x250);d.clientPredicted=Read<void*>(state.inputState,0x258);
    d.acknowledgedCorrectionIndex=Read<int32_t>(state.inputState,0x260);
    d.correctionResetPending=Read<uint8_t>(state.inputState,0x264);
    d.inventory=Read<void*>(state.pawn,0xc08);d.current=Read<void*>(d.inventory,0x280);
    d.desired=Read<void*>(d.inventory,0x268);d.corrected=Read<void*>(d.inventory,0x320);
    d.authCorrectionIndex=Read<int32_t>(d.inventory,0x328);d.clientCorrectionIndex=Read<int32_t>(d.inventory,0x2f8);
    d.originCorrectionIndex=Read<int32_t>(d.inventory,0x32c);d.inventoryCorrectionState=Read<uint8_t>(d.inventory,0x2fc);
    d.initialCorrectionSent=Read<uint8_t>(d.inventory,0x30c);d.initialCorrectionProcessed=Read<uint8_t>(d.inventory,0x30d);
    d.copiedEvents=static_cast<uint32_t>((std::min)(d.count,static_cast<int32_t>(d.events.size())));
    auto events=Read<const uint8_t*>(state.incomingEvents,0x738);
    for(uint32_t i=0;i<d.copiedEvents;++i){auto& event=d.events[i];event.eventId=d.base+1+static_cast<int32_t>(i);
        std::memcpy(event.bytes.data(),events+size_t(i)*40,event.bytes.size());}
}

inline bool ApplyEvents(uint32_t index){
    int32_t eligible=-1;
    for(uint32_t i=0;i<state.watermarkCount;++i)if(static_cast<int32_t>(index-state.watermarks[i].index)>=0)
        eligible=(std::max)(eligible,state.watermarks[i].eventId);
    const int32_t base=Read<int32_t>(state.incomingEvents,4),count=Read<int32_t>(state.incomingEvents,0x740);
    if(eligible<=base||count<=0)return true;
    const int32_t prefix=(std::min)(count,eligible-base);
    CaptureInputApplyDiagnostic(index,eligible,prefix);
    // The original event array is private to this adapter. Limit native apply
    // to the accepted time watermark, then restore the entire pending array.
    Put<int32_t>(state.incomingEvents,0x740,prefix);
    // The retail component has no remote receiver to initialize this authority
    // counter. Seed it with the known consumed prefix, so a native blocked
    // event cannot be mistaken for event zero successfully being applied.
    Put<int32_t>(state.inputState,0x120c,base);
    ++state.inputApplyCalls;
    engine::function<void(*)(void*,void*,float)>(0x70a80b0)
        (state.inputState,state.incomingEvents,static_cast<float>(index)*(1.0f/128.0f));
    local_server_empty_bootstrap_probe::OriginalApplyReturned(state.pawn,state.epochNumber);
    Put<int32_t>(state.incomingEvents,0x740,count);
    const int32_t applied=Read<int32_t>(state.inputState,0x120c);
    state.inputApplyDiagnostic.appliedAfter=applied;
    if(applied<=base)++state.inputBlockedApplyAttempts;
    state.inputApplyDiagnostic.nativeAttemptCacheReadable=original_players::Readable
        (reinterpret_cast<const void*>(engine::image+0xd723fe8),40);
    if(state.inputApplyDiagnostic.nativeAttemptCacheReadable)
        std::memcpy(state.inputApplyDiagnostic.nativeAttempt.data(),reinterpret_cast<const void*>(engine::image+0xd723fe8),40);
    if(applied<base||applied>eligible)return Fail(3245,"network_input_native_applied_event_id_outside_eligible_range");
    if(applied>base){
        state.inputEventsApplied+=static_cast<uint32_t>(applied-base);state.latestAppliedEventId=applied;
        engine::function<void(*)(void*,int32_t)>(0x70a9dd0)(state.incomingEvents,applied);
    }
    uint32_t write=0;
    for(uint32_t i=0;i<state.watermarkCount;++i)if(state.watermarks[i].eventId>state.latestAppliedEventId)
        state.watermarks[write++]=state.watermarks[i];
    state.watermarkCount=write;state.pendingEvents=static_cast<uint32_t>(Read<int32_t>(state.incomingEvents,0x740));
    return true;
}
inline bool SendAcknowledgement(){
    if(state.teleportCorrectionInFlight)return Fail(3382,"network_input_reentrant_native_teleport_correction");
    const int connectionStatus=ConnectionStatus();
    if(!connectionStatus)return true;
    if(connectionStatus<0||!OwnedLinks())return Fail(3263,"network_input_ack_owner_or_connection_changed");
    // The native client looks up its original SavedMove ID. The live host raw
    // clock is a different domain and is never used as an ACK identifier.
    const uint32_t index=state.lastObservedClientIndex;
    if(!index||index==state.lastAcknowledgedIndex)return true;
    if(!state.wireAligned||HostToClientIndex(state.lastConsumedIndex)!=index)
        return Fail(3312,"network_input_ack_requires_current_observed_client_command");
    if(!ReadTeleportCorrectionFlags())return false;
    const bool nativeTeleport=state.teleportSinceCorrection!=0;
    const engine::Vec difference{state.positionAfter.x-state.lastClaim.x,state.positionAfter.y-state.lastClaim.y,state.positionAfter.z-state.lastClaim.z};
    state.lastClaimError=std::sqrt(difference.x*difference.x+difference.y*difference.y+difference.z*difference.z);
    if(!std::isfinite(state.lastClaimError))return Fail(3246,"network_input_authority_claim_error_nonfinite");
    if(state.forceHardCorrection||nativeTeleport||state.lastClaimError>Configuration.hardCorrectionDistance){
        HardCorrection p;p.index=index;p.location=state.positionAfter;p.velocity=state.velocityAfter;
        // Same original Pawn.v900 sample used by native move converter789CA00.
        p.crouch=engine::function<float(*)(void*)>(0x7883790)(state.pawn);
        if(!std::isfinite(p.crouch))return Fail(3258,"network_input_native_crouch_sample_nonfinite");
        p.mode=engine::function<uint8_t(*)(void*)>(0x4189be0)(state.movement);
        std::memcpy(p.jump.bytes,static_cast<uint8_t*>(state.movement)+0x1218,sizeof(p.jump));
        p.debug.correctionId=++state.correctionId;
        p.teleport=uint8_t(state.seedTeleportPending||nativeTeleport);
        state.lastCorrectionTeleport=p.teleport;
        if(!DispatchTeleportCorrection(p,nativeTeleport))return false;
        ++state.hardCorrections;state.forceHardCorrection=false;state.seedTeleportPending=false;
        // Hard correction itself ACKs movement N before requesting replay. A
        // normal movement ACK for N here would destroy the lookup it needs.
        if(state.latestAppliedEventId>=0){Ack events{state.latestAppliedEventId,0};
            if(!Dispatch(state.pawn,state.ackFunction,&events))return false;++state.eventOnlyAcks;}
    }else if(state.lastClaimError>Configuration.gradualCorrectionDistance){
        GradualAck p;p.ack={state.latestAppliedEventId,index};p.correction.difference=difference;p.correction.velocity=state.velocityAfter;
        if(!Dispatch(state.pawn,state.gradualFunction,&p))return false;++state.gradualAcks;
    }else{
        Ack p{state.latestAppliedEventId,index};if(!Dispatch(state.pawn,state.ackFunction,&p))return false;++state.normalAcks;
    }
    state.lastAcknowledgedIndex=index;state.lastAckEventId=state.latestAppliedEventId;
    return true;
}
inline bool SendAppliedEventAcknowledgement(){
    if(state.latestAppliedEventId<0||state.latestAppliedEventId<=state.lastAckEventId)return true;
    const int connectionStatus=ConnectionStatus();
    if(!connectionStatus)return true;
    if(connectionStatus<0||!OwnedLinks())return Fail(3263,"network_input_event_ack_owner_or_connection_changed");
    Ack events{state.latestAppliedEventId,0};
    if(!Dispatch(state.pawn,state.ackFunction,&events))return false;
    ++state.eventOnlyAcks;state.lastAckEventId=state.latestAppliedEventId;
    return true;
}
inline bool PollImpl(float deltaSeconds){
    if(state.error)return false;
    if(!state.installed)return Wait("network_input_waiting_receiver_install");
    const bool threadMatches=Thread(),linksMatch=OwnedLinks();
    const int connectionStatus=ConnectionStatus();
    if(!threadMatches||connectionStatus<0)return Fail(3247,"network_input_poll_owner_or_thread_changed");
    if(!connectionStatus){++state.closedConnectionPolls;return true;}
    if(!linksMatch)return Fail(3247,"network_input_poll_owner_or_thread_changed");
    if(state.inPoll||state.inReceive)return Fail(3248,"network_input_reentrant_authority_consumer");
    if(!std::isfinite(deltaSeconds)||deltaSeconds<0)return Fail(3249,"network_input_host_delta_invalid");
    state.inPoll=true;++state.polls;
    state.hostElapsedSeconds+=deltaSeconds;
    if(!SupplyHostTime(deltaSeconds)){state.inPoll=false;return false;}
    local_server_human_funded_boundary_probe::AfterSupply(state);
    const double servicedBeforePoll=state.hostServicedSeconds;
    // Same per-component-frame bookkeeping as the retained original loops.
    Put<uint32_t>(state.clock,0x968,0);state.nativeFrameSteps=0;++state.nativeFrameStepResets;
    constexpr float step=1.0f/128.0f;
    uint32_t processed=0,observedThisPoll=0;
    while(processed<Configuration.maxStepsPerPoll&&state.wireAligned){
        const uint32_t index=NextIndex(state.lastConsumedIndex);
        auto& slot=state.queue[index&(Configuration.queueCapacity-1)];
        const bool observed=slot.present&&slot.move.index==index;
        bool holdForGap=false;
        if(!observed){
            // Explicit missing authority policy, not a recovered server body:
            // bridge known packet loss using only the last accepted input;
            // silence may hold after .25s. Neither branch invents a received
            // command or movement ACK. Stop after observed steps so the pose
            // used by an ACK still belongs to that exact observed command.
            holdForGap=state.queueCount!=0;
            const bool holdForSilence=state.hostElapsedSeconds-state.lastFreshMovementHostSeconds
                >=Configuration.lostMovementHoldDelaySeconds;
            if(observedThisPoll||!state.lastKnownInputValid||(!holdForGap&&!holdForSilence)){
                ++state.queueWaitPolls;state.stage="network_input_waiting_fresh_command_or_bounded_loss_hold";break;
            }
        }
        Move command=observed?slot.move:state.lastKnownInput;command.index=index;
        if(Read<uint8_t>(state.movement,0x1630)){
            state.inPoll=false;return Fail(3250,"network_input_extra_native_movement_branch_unproven");
        }
        if(!ClockState()||state.clockIndex!=state.lastConsumedIndex){state.inPoll=false;return Fail(3252,"network_input_native_clock_and_consumed_sequence_diverged");}
        const float beforeAdvance=state.clockAccumulated;
        if(!engine::function<bool(*)(void*,void*,bool)>(0x789f0e0)(state.clock,state.timeShift,true)){
            state.stage="network_input_waiting_native_clock_readiness";break;
        }
        ++state.clockAdvances;
        if(!ClockState()||state.clockIndex!=index){state.inPoll=false;return Fail(3253,"network_input_native_clock_advanced_unexpected_index");}
        const float afterAdvance=state.clockAccumulated;
        state.nativeAdvanceAdjustmentSeconds+=double(afterAdvance)-double(beforeAdvance)+double(step);
        if(!AdvanceMappingClock(index)){state.inPoll=false;return false;}
        if(!ApplyEvents(index)){state.inPoll=false;return false;}
        // Only server native intent/state is supplied. Client position is a
        // comparison sample; no received transform/velocity is committed.
        Put<engine::Vec>(state.movement,0x368,command.intent);
        if(!ApplyAuthorityCrouchInput(command.flags,observed)){state.inPoll=false;return false;}
        // Exactly once: preserves native walk + SetCrouchHeld side effects.
        engine::function<void(*)(void*,uint8_t)>(0x7894a20)(state.movement,command.flags);
        ++state.originalCompressedFlagCalls;
        if(!CrouchReadback(state.crouchDesiredRaw)){state.inPoll=false;return false;}
        engine::Vec rotation{static_cast<double>(command.pitchYaw&0xffff)*(360.0/65536.0),
            static_cast<double>(command.pitchYaw>>16)*(360.0/65536.0),static_cast<double>(command.roll)*(360.0/256.0)};
        auto setRotation=engine::Find(L"/Script/Engine.Controller.SetControlRotation");
        if(!setRotation||Read<uint16_t>(setRotation,0xbe)!=24||!Dispatch(state.pc,setRotation,&rotation)){
            state.inPoll=false;return Fail(3254,"network_input_native_host_control_rotation_abi_unavailable");
        }
        engine::function<void(*)(void*,float)>(0x7883620)(state.pawn,step);++state.beforeFrameCalls;
        if(beforeStep&&!beforeStep(state.pawn,state.movement,step,index,&command.intent)){
            state.inPoll=false;return Fail(3255,"network_input_before_step_capture_failed");
        }
        auto updated=Read<void*>(state.movement,0xd8);
        state.positionBefore=Read<engine::Vec>(updated,0x320);
        ++state.physicsCalls;state.lastPhysicsDelta=step;
        engine::function<void(*)(void*,float)>(0x78954d0)(state.movement,step);
        ++state.physicsCompleted;
        if(!DebitCompletedHostStep(step)){state.inPoll=false;return false;}
        state.positionAfter=Read<engine::Vec>(updated,0x320);state.velocityAfter=Read<engine::Vec>(state.movement,0xf0);
        if(!Finite(state.positionAfter)||!Finite(state.velocityAfter)){
            state.inPoll=false;return Fail(3256,"network_input_native_authority_physics_nonfinite");
        }
        if(afterStep&&!afterStep(state.pawn,state.movement,step,index,&command.intent)){
            state.inPoll=false;return Fail(3257,"network_input_after_step_capture_failed");
        }
        ++state.nativeFrameSteps;Put<uint32_t>(state.clock,0x968,state.nativeFrameSteps);
        struct FrameScope {void* movement;void* fixedStep;void* clock;} scope{state.movement,state.fixedStep,state.clock};
        engine::function<void(*)(void*)>(0x789f310)(&scope);++state.frameScopes;
        if(!ClockState()){state.inPoll=false;return false;}
        state.nativePostAdvanceAccumulatorSeconds+=double(state.clockAccumulated)-double(afterAdvance);
        state.lastIntent=command.intent;state.lastConsumedIndex=index;
        if(observed){
            state.lastClaim=command.claimedLocation;state.lastObservedClientIndex=slot.clientIndex;
            state.lastKnownInput=command;state.lastKnownInputValid=true;state.heldSinceObserved=0;
            slot.present=false;--state.queueCount;++state.observedCommandFrames;++observedThisPoll;
        }else{
            ++state.heldStateFrames;++state.heldSinceObserved;state.forceHardCorrection=true;
            if(holdForGap)++state.heldGapFrames;else ++state.heldSilenceFrames;
        }
        ++processed;
    }
    state.lastHostPollSteps=processed;state.nativeAccumulatorAtPollEnd=state.clockAccumulated;
    const double servicedThisPoll=state.hostServicedSeconds-servicedBeforePoll;
    if(servicedThisPoll>double(deltaSeconds)){
        ++state.hostCatchupPolls;state.hostCatchupServicedSeconds+=servicedThisPoll-double(deltaSeconds);
    }
    if(processed==Configuration.maxStepsPerPoll&&state.hostFundedWorkSeconds+state.hostUnfundedSeconds>=double(step))++state.hostWorkQuotaHits;
    if(!AuditHostTime()){state.inPoll=false;return false;}
    if(processed){
        void* movement=state.movement;
        engine::function<void(*)(void*,const void*)>(0x1cf69a0)
            (static_cast<uint8_t*>(state.movement)+0x10c0,&movement);++state.delegateCalls;
        if(observedThisPoll){
            if(!SendAcknowledgement()){state.inPoll=false;return false;}
            if(!state.disconnected)state.stage="network_input_native_authority_step_and_ack_runtime_unproven";
        }else{
            if(!SendAppliedEventAcknowledgement()){state.inPoll=false;return false;}
            if(!state.disconnected)state.stage="network_input_native_authority_held_state_without_movement_ack_runtime_unproven";
        }
    }
    state.inPoll=false;return !state.error;
}
inline bool Poll(float deltaSeconds){
    __try {return PollImpl(deltaSeconds);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool ObserveClientClockImpl(void* pawn){
    auto& d=state.clientClock;d.thread=GetCurrentThreadId();d.pawn=pawn;
    d.observed=false;d.roundtrip=false;d.localOwner=false;
    if(!engine::image||!original_players::gameThread||d.thread!=original_players::gameThread
       ||!original_players::IsA(pawn,original_players::ShooterCharacterClass)||Read<uint8_t>(pawn,0x224)!=2){
        d.stage="network_input_client_clock_waiting_local_role2_owner";return false;
    }
    uint8_t local=0;
    if(!engine::Event(pawn,L"/Script/Engine.Pawn.IsLocallyControlled",&local)||local!=1){
        d.stage="network_input_client_clock_waiting_local_control";return false;
    }
    d.localOwner=true;
    if(!d.profilesVerified){
        d.profilesVerified=Profile(0x789f7b0,32,0x61323e7251773601ULL)
            &&Profile(0x789f7d0,28,0xcffc79c87f2ee8dbULL);
        if(!d.profilesVerified){d.stage="network_input_client_clock_original_profile_changed";return false;}
    }
    d.movement=Read<void*>(pawn,0x4f8);d.prediction=Read<void*>(d.movement,0x7e8);
    if(!d.prediction){d.stage="network_input_client_clock_waiting_original_prediction";return false;}
    d.clock=static_cast<uint8_t*>(d.prediction)+0x1b0;auto table=Read<void**>(d.clock);
    auto vb=Read<void*>(d.clock,0x10);const int32_t displacement=Read<int32_t>(vb,4);
    if(table!=reinterpret_cast<void**>(engine::image+0xb600b30)||displacement<0||displacement>0x1000
       ||Read<void*>(table,0x28)!=reinterpret_cast<void*>(engine::image+0x789f7b0)
       ||Read<void*>(table,0x30)!=reinterpret_cast<void*>(engine::image+0x789f7d0)){
        d.stage="network_input_client_clock_original_layout_changed";return false;
    }
    d.timestep=Read<int32_t>(d.prediction,0xb78);
    if(d.timestep!=Read<int32_t>(d.clock,0x9c8)){
        d.stage="network_input_client_clock_timestep_alias_mismatch";return false;
    }
    d.rawIndex=Read<uint32_t>(d.clock,static_cast<size_t>(displacement)+0x18);
    d.hostSecondsBefore=1.0f/128.0f;d.clientSecondsAfter=d.hostSecondsBefore;
    // Observation invokes the retained conversions on local scratch floats.
    // The only client timestep assignment remains the original reliable RPC.
    engine::function<void(*)(void*,float*)>(0x789f7b0)(d.clock,&d.clientSecondsAfter);
    d.hostSecondsAfter=d.clientSecondsAfter;
    engine::function<void(*)(void*,float*)>(0x789f7d0)(d.clock,&d.hostSecondsAfter);
    d.roundtrip=std::isfinite(d.clientSecondsAfter)&&d.hostSecondsAfter==d.hostSecondsBefore;
    d.observed=true;++d.observations;
    d.stage=d.roundtrip?"network_input_client_native_clock_roundtrip_observed":"network_input_client_native_clock_roundtrip_failed";
    return d.roundtrip;
}
inline bool ObserveClientClock(void* pawn){
    __try{return ObserveClientClockImpl(pawn);}__except(EXCEPTION_EXECUTE_HANDLER){
        state.clientClock.observed=false;state.clientClock.stage="network_input_client_clock_observation_exception";return false;
    }
}
// Pure diagnostic formatting. No native getters, object reads, mutation or
// clock/engine calls are performed from this serializer.
inline void DiagnosticText(FILE* f,const char* value){
    if(!value){std::fputs("null",f);return;}std::fputc('"',f);
    for(const auto* p=reinterpret_cast<const unsigned char*>(value);*p;++p){
        if(*p=='"'||*p=='\\'){std::fputc('\\',f);std::fputc(*p,f);}
        else if(*p<0x20)std::fprintf(f,"\\u%04x",unsigned(*p));else std::fputc(*p,f);
    }std::fputc('"',f);
}
inline void DiagnosticNumber(FILE* f,double value){if(std::isfinite(value))std::fprintf(f,"%.17g",value);else std::fputs("null",f);}
struct DiagnosticJson {
    FILE* f;bool first=true;
    explicit DiagnosticJson(FILE* value):f(value){std::fputc('{',f);}
    ~DiagnosticJson(){std::fputc('}',f);}
    void key(const char* name){if(!first)std::fputc(',',f);first=false;DiagnosticText(f,name);std::fputc(':',f);}
    void u(const char* name,uint64_t value){key(name);std::fprintf(f,"%llu",static_cast<unsigned long long>(value));}
    void i(const char* name,int64_t value){key(name);std::fprintf(f,"%lld",static_cast<long long>(value));}
    void b(const char* name,bool value){key(name);std::fputs(value?"true":"false",f);}
    void n(const char* name,double value){key(name);DiagnosticNumber(f,value);}
    void t(const char* name,const char* value){key(name);DiagnosticText(f,value);}
    void p(const char* name,const void* value){key(name);std::fprintf(f,"\"%p\"",value);}
    void v(const char* name,const engine::Vec& value){key(name);std::fputc('[',f);DiagnosticNumber(f,value.x);std::fputc(',',f);
        DiagnosticNumber(f,value.y);std::fputc(',',f);DiagnosticNumber(f,value.z);std::fputc(']',f);}
};
inline void WriteMoveDiagnostic(FILE* f,const Move& move){
    DiagnosticJson j(f);j.u("index",move.index);j.u("type",move.type);j.u("flags",move.flags);j.u("mode",move.mode);
    j.u("roll",move.roll);j.u("pitch_yaw",move.pitchYaw);j.u("first_step",move.firstStep);j.n("time_shift",move.timeShift);
    j.n("crouch",move.crouch);j.v("intent",move.intent);j.v("claimed_location",move.claimedLocation);j.p("base",move.base);
    j.key("base_bone");std::fprintf(f,"[%u,%u,%u]",move.baseBone[0],move.baseBone[1],move.baseBone[2]);
}
inline void WritePacketDiagnostic(FILE* f,const SequencePacket& packet){
    DiagnosticJson j(f);j.u("packet",packet.packet);j.u("count",packet.count);j.u("first",packet.first);j.u("last",packet.last);
    j.u("expected",packet.expected);j.u("clock",packet.clock);j.u("consumed",packet.consumed);j.u("queued",packet.queued);
    j.u("first_step",packet.firstStep);j.n("time_shift",packet.timeShift);
}
inline void WriteInputWireDiagnostic(FILE* f,const InputWireTrace& trace){
    DiagnosticJson j(f);j.u("packet",trace.packet);j.b("header_known",trace.headerKnown);j.b("forward_gap",trace.forwardGap);
    j.i("first_event_id",trace.firstEventId);j.i("base_before",trace.baseBefore);j.i("count_before",trace.countBefore);
    j.i("base_after",trace.baseAfter);j.i("count_after",trace.countAfter);j.u("native_result",trace.nativeResult);
}
inline void WriteInputEventDiagnostic(FILE* f,int32_t eventId,const std::array<uint8_t,40>& raw){
    int32_t firstWord=0,secondWord=0;float firstFloat=0,otherFloat=0;
    std::memcpy(&firstWord,raw.data()+8,sizeof(firstWord));std::memcpy(&secondWord,raw.data()+12,sizeof(secondWord));
    std::memcpy(&firstFloat,raw.data()+8,sizeof(firstFloat));std::memcpy(&otherFloat,raw.data()+0x14,sizeof(otherFloat));
    DiagnosticJson j(f);j.i("event_id",eventId);j.u("type",raw[0]);j.i("i32_at8",firstWord);j.i("i32_at12",secondWord);
    j.n("f32_at8",firstFloat);j.n("f32_at20",otherFloat);j.key("raw_hex");std::fputc('"',f);
    for(auto byte:raw)std::fprintf(f,"%02x",unsigned(byte));std::fputc('"',f);
}
inline void WriteState(FILE* f){
    if(!f)return;const auto& s=state;DiagnosticJson j(f);
    j.u("schema_version",7);j.t("stage",s.stage);j.u("error",s.error);j.u("exception_rva",s.exceptionRva);
    const struct {const char* name;bool value;} flags[]{
        {"installed",s.installed},{"epoch_retired",s.epochRetired},{"disconnected",s.disconnected},{"restart_required",s.restartRequired},
        {"runtime_unproven",s.runtime_unproven},{"profiles_verified",s.profilesVerified},{"owned_links",s.ownedLinks},
        {"weak_identities_captured",s.identitiesCaptured},{"weak_identities_verified",s.weakIdentitiesVerified},
        {"private_table_read_only",s.privateTableReadOnly},{"sole_queue_consumer",s.soleQueueConsumer},{"in_receive",s.inReceive},{"in_poll",s.inPoll},
        {"connection_state_readable",s.connectionStateReadable},{"thread_matches",s.threadMatches},
        {"crouch_metadata_verified",s.crouchMetadataVerified},{"crouch_readback_verified",s.crouchReadbackVerified},
        {"retired_source_restored",s.retiredSourceRestored},{"retired_pawn_expired",s.retiredPawnExpired},
        {"view_observer_installed",s.viewObserverInstalled},{"incoming_events_constructed",s.incomingEventsConstructed},
        {"first_move_batch_captured",s.firstMoveBatchCaptured},{"force_hard_correction",s.forceHardCorrection},
        {"wire_aligned",s.wireAligned},{"mapping_clock_constructed",s.mappingClockConstructed},
        {"last_known_input_valid",s.lastKnownInputValid},{"timestep_rpc_submitted",s.timestepRpcSubmitted},
        {"seed_teleport_pending",s.seedTeleportPending},{"teleport_metadata_verified",s.teleportMetadataVerified},
        {"teleport_correction_in_flight",s.teleportCorrectionInFlight}
    };for(const auto& flag:flags)j.b(flag.name,flag.value);
    const struct {const char* name;uint64_t value;} numbers[]{
        {"epoch",s.epochNumber},{"retired_epochs",s.retiredEpochs},{"retired_packets_discarded",s.retiredPacketsDiscarded},
        {"retired_queued_moves",s.retiredQueuedMoves},{"retired_input_events",s.retiredInputEvents},{"retirement_calls",s.retirementCalls},
        {"stream_clear_calls",s.streamClearCalls},{"stream_reset_calls",s.streamResetCalls},{"view_observer_installs",s.viewObserverInstalls},
        {"view_observer_restores",s.viewObserverRestores},{"game_thread",s.gameThread},{"actual_thread",s.actualThreadId},
        {"player_registry_thread",s.playerRegistryThreadId},{"owned_passed_mask",s.ownedPassedMask},{"owned_failed_mask",s.ownedFailedMask},
        {"owned_required_mask",s.ownedRequiredMask},{"owned_passed_mask_at_install",s.ownedPassedMaskAtInstall},
        {"owned_failed_mask_at_install",s.ownedFailedMaskAtInstall},{"pawn_role",s.actualPawnLocalRole},{"pawn_remote_role",s.actualPawnRemoteRole},
        {"pc_role",s.actualPCLocalRole},{"pc_remote_role",s.actualPCRemoteRole},{"profile_failure_rva",s.profileFailureRva},
        {"clock_index",s.clockIndex},{"clock_step",s.clockStep},{"clock_base_offset",s.clockBaseOffset},
        {"mapping_clock_raw_index",s.mappingClockRawIndex},{"mapping_clock_step",s.mappingClockStep},
        {"mapping_clock_base_offset",s.mappingClockBaseOffset},{"mapping_clock_advances",s.mappingClockAdvances},
        {"mapping_clock_destructions",s.mappingClockDestructions},{"wire_origin_client_index",s.wireOriginClientIndex},
        {"last_observed_client_index",s.lastObservedClientIndex},{"last_received_client_index",s.lastReceivedClientIndex},
        {"unobserved_initial_client_steps",s.unobservedInitialClientSteps},{"seed_raw_before",s.seedRawBefore},
        {"seed_raw_after",s.seedRawAfter},{"seed_history_after",s.seedHistoryAfter},{"seed_pending_after",s.seedPendingAfter},
        {"native_frame_steps",s.nativeFrameSteps},{"native_seed_calls",s.nativeSeedCalls},{"timestep_rpc_calls",s.timestepRpcCalls},
        {"observed_command_frames",s.observedCommandFrames},{"held_state_frames",s.heldStateFrames},
        {"held_gap_frames",s.heldGapFrames},{"held_silence_frames",s.heldSilenceFrames},
        {"clock_resynchronizations",s.clockResynchronizations},{"expired_client_moves",s.expiredClientMoves},
        {"held_since_observed",s.heldSinceObserved},{"last_wire_sync_client_index",s.lastWireSyncClientIndex},
        {"mapped_barrier_events",s.mappedBarrierEvents},{"native_frame_step_resets",s.nativeFrameStepResets},
        {"native_input_wire_gaps",s.nativeInputWireGaps},{"input_blocked_apply_attempts",s.inputBlockedApplyAttempts},
        {"native_correction_cancellations",s.nativeCorrectionCancellations},{"native_correction_cancelled_events",s.nativeCorrectionCancelledEvents},
        {"native_checkpoint_decodes",s.nativeCheckpointDecodes},{"native_checkpoint_destructions",s.nativeCheckpointDestructions},
        {"host_funding_calls",s.hostFundingCalls},{"host_catchup_polls",s.hostCatchupPolls},{"host_work_quota_hits",s.hostWorkQuotaHits},
        {"host_debt_recovery_transfers",s.hostDebtRecoveryTransfers},{"last_host_poll_steps",s.lastHostPollSteps},
        {"queue_count",s.queueCount},{"watermark_count",s.watermarkCount},{"last_received_index",s.lastReceivedIndex},
        {"last_consumed_index",s.lastConsumedIndex},{"last_acknowledged_index",s.lastAcknowledgedIndex},{"pending_events",s.pendingEvents},
        {"correction_id",s.correctionId},{"received_packets",s.receivedPackets},{"accepted_packets",s.acceptedPackets},
        {"rejected_packets",s.rejectedPackets},{"decoded_moves",s.decodedMoves},{"unique_moves",s.uniqueMoves},{"duplicate_moves",s.duplicateMoves},
        {"physics_calls",s.physicsCalls},{"physics_completed",s.physicsCompleted},{"clock_advances",s.clockAdvances},{"frame_scopes",s.frameScopes},
        {"input_apply_calls",s.inputApplyCalls},{"input_events_applied",s.inputEventsApplied},{"before_frame_calls",s.beforeFrameCalls},
        {"delegate_calls",s.delegateCalls},{"normal_acks",s.normalAcks},{"gradual_acks",s.gradualAcks},{"hard_corrections",s.hardCorrections},
        {"event_only_acks",s.eventOnlyAcks},{"polls",s.polls},{"budget_caps",s.budgetCaps},{"queue_wait_polls",s.queueWaitPolls},
        {"teleport_flag_samples",s.teleportFlagSamples},{"native_teleport_corrections_submitted",s.teleportCorrectionsSubmitted},
        {"host_sent_teleport_flag_consumptions",s.teleportFlagConsumptions},{"teleport_flag_already_consumed",s.teleportFlagAlreadyConsumed},
        {"teleport_since_move",s.teleportSinceMove},{"teleport_since_correction",s.teleportSinceCorrection},
        {"last_correction_teleport",s.lastCorrectionTeleport},{"teleport_flag_clear_readback",s.teleportClearReadback},
        {"teleport_source_epoch",s.teleportSourceEpoch},{"teleport_correction_client_index",s.teleportCorrectionClientIndex},
        {"closed_connection_polls",s.closedConnectionPolls},{"closed_connection_packets",s.closedConnectionPackets},
        {"last_packet_bytes",s.lastPacketBytes},{"last_packet_bits",s.lastPacketBits},{"last_reader_bit",s.lastReaderBit},
        {"last_input_end_bit",s.lastInputEndBit},{"pdd_header_only_packets",s.pddHeaderOnlyPackets},{"pdd_absent_packets",s.pddAbsentPackets},
        {"pdd_payload_packets",s.pddPayloadPackets},{"pdd_player_count",s.pddPlayerCount},
        {"crouch_input_commands",s.crouchInputCommands},{"crouch_transitions",s.crouchInputTransitions},{"crouch_authority_writes",s.crouchAuthorityWrites},
        {"crouch_getter_calls",s.crouchGetterCalls},{"crouch_readback_failures",s.crouchReadbackFailures},
        {"original_compressed_flag_calls",s.originalCompressedFlagCalls},{"crouch_before_raw",s.crouchBeforeRaw},
        {"crouch_desired_raw",s.crouchDesiredRaw},{"crouch_after_raw",s.crouchAfterRaw},{"crouch_requested",s.crouchRequested},
        {"crouch_getter_value",s.crouchGetterValue},{"crouch_field_size",s.crouchFieldSize},{"crouch_byte_offset",s.crouchByteOffset},
        {"crouch_byte_mask",s.crouchByteMask},{"crouch_field_mask",s.crouchFieldMask},{"crouch_cast_flags",s.crouchCastFlags}
    };for(const auto& number:numbers)j.u(number.name,number.value);
    j.i("connection_state",s.actualConnectionState);j.i("latest_applied_event_id",s.latestAppliedEventId);j.i("last_ack_event_id",s.lastAckEventId);
    j.i("input_mapping_clock_offset",s.clockIndexOffset);j.i("actual_authority_clock_offset",s.actualClockIndexOffset);
    j.i("timestep_rpc_value",s.timestepRpcValue);
    j.b("correction_metadata_verified",s.correctionMetadataVerified);j.i("last_cancelled_correction_index",s.lastCancelledCorrectionIndex);
    j.i("crouch_field_offset",s.crouchFieldOffset);j.i("crouch_array_dim",s.crouchArrayDim);j.i("crouch_element_size",s.crouchElementSize);j.i("crouch_type_size",s.crouchTypeSize);
    j.n("host_elapsed_seconds",s.hostElapsedSeconds);j.n("host_budget_seconds",s.hostBudgetSeconds);j.n("clock_accumulated",s.clockAccumulated);
    j.t("host_budget_domain","original_native_accumulator");j.b("host_time_origin_started",s.hostTimeOriginStarted);
    j.b("post_origin_delta_discarded",false);j.u("max_steps_per_poll",Configuration.maxStepsPerPoll);
    j.n("last_host_delta_seconds",s.lastHostDeltaSeconds);j.n("last_host_funding_seconds",s.lastHostFundingSeconds);
    j.n("host_unfunded_seconds",s.hostUnfundedSeconds);j.n("native_work_budget_seconds",s.hostFundedWorkSeconds);
    j.n("host_serviced_seconds",s.hostServicedSeconds);j.n("host_credited_seconds",s.hostCreditedSeconds);
    j.n("host_pre_origin_seconds",s.hostPreOriginSeconds);j.n("host_origin_credit_seconds",s.hostOriginCreditSeconds);
    j.n("host_origin_excluded_seconds",s.hostOriginExcludedSeconds);j.n("host_post_origin_seconds",s.hostPostOriginSeconds);
    j.n("host_debt_peak_seconds",s.hostDebtPeakSeconds);j.n("host_outstanding_work_peak_seconds",s.hostUnprocessedPeakSeconds);
    j.n("host_catchup_serviced_seconds",s.hostCatchupServicedSeconds);j.n("host_debt_transferred_backlog_seconds",s.hostDebtTransferredBacklogSeconds);
    j.n("retired_native_work_budget_seconds",s.hostRetiredFundedSeconds);j.n("retired_unfunded_host_seconds",s.hostRetiredUnfundedSeconds);
    j.n("host_real_time_conservation_error_seconds",s.hostTimeConservationError);
    j.n("native_clock_conservation_error_seconds",s.hostCreditConservationError);
    j.n("native_initial_accumulator_seconds",s.nativeInitialAccumulatorSeconds);j.n("native_funding_roundoff_seconds",s.nativeFundingRoundoffSeconds);
    j.n("native_advance_adjustment_seconds",s.nativeAdvanceAdjustmentSeconds);j.n("native_external_accumulator_seconds",s.nativeExternalAccumulatorSeconds);
    j.n("native_post_advance_accumulator_seconds",s.nativePostAdvanceAccumulatorSeconds);
    j.n("last_fresh_movement_host_seconds",s.lastFreshMovementHostSeconds);j.n("lost_movement_hold_delay_seconds",Configuration.lostMovementHoldDelaySeconds);
    j.n("seed_client_seconds",s.seedClientSeconds);j.n("seed_mapped_host_seconds",s.seedMappedHostSeconds);j.n("seed_inverse_client_seconds",s.seedInverseClientSeconds);
    j.n("last_physics_delta",s.lastPhysicsDelta);j.n("dropped_host_seconds",s.droppedHostSeconds);j.n("last_claim_error",s.lastClaimError);
    j.v("installed_position",s.installedPosition);j.v("installed_velocity",s.installedVelocity);j.v("position_before",s.positionBefore);
    j.v("position_after",s.positionAfter);j.v("velocity_after",s.velocityAfter);j.v("last_claim",s.lastClaim);j.v("last_intent",s.lastIntent);
    const struct {const char* name;const void* value;} pointers[]{
        {"world",s.world},{"pc",s.pc},{"pawn",s.pawn},{"player_state",s.playerState},{"connection",s.connection},{"movement",s.movement},
        {"input_state",s.inputState},{"game_state",s.gameState},{"fixed_step",s.fixedStep},{"clock",s.clock},{"time_shift",s.timeShift},
        {"private_mapping_clock",s.mappingClock},{"timestep_function",s.timestepFunction},
        {"native_net_driver",s.nativeNetDriver},{"source_pawn_table",s.sourcePawnTable},{"private_pawn_table",s.privatePawnTable},
        {"actual_pawn_table",s.actualPawnTable},{"actual_movement_table",s.actualMovementTable},{"pawn_table_after_install",s.pawnTableAfterInstall},
        {"movement_table_at_install",s.movementTableAtInstall},{"actual_pc_player",s.actualPCPlayer},{"actual_connection_pc",s.actualConnectionPC},
        {"actual_connection_actor",s.actualConnectionOwningActor},{"actual_pc_pawn",s.actualPCPawn},{"actual_pawn_controller",s.actualPawnController},
        {"actual_pc_state",s.actualPCState},{"actual_pawn_state",s.actualPawnState},{"actual_state_owner",s.actualStateOwner},
        {"actual_movement_owner",s.actualMovementOwner},{"actual_movement_character",s.actualMovementCharacter},{"actual_movement_pawn",s.actualMovementPawnOwner},
        {"actual_input_owner",s.actualInputOwner},{"actual_input_pawn",s.actualInputPawn},{"actual_world_game_state",s.actualWorldGameState},
        {"actual_physics_entry",s.actualMovementPhysicsEntry},{"actual_packed_mode_entry",s.actualMovementModeEntry},
        {"last_retired_pawn",s.lastRetiredPawn},{"retired_weak_resolved_pawn",s.retiredWeakResolvedPawn},{"view_observer",s.viewObserver},
        {"crouch_field",s.crouchField},{"crouch_field_class",s.crouchFieldClass},{"crouch_native_bool_class",s.crouchNativeBoolClass},
        {"crouch_getter_function",s.crouchGetterFunction},{"ack_function",s.ackFunction},{"gradual_function",s.gradualFunction},{"hard_function",s.hardFunction}
    };for(const auto& pointer:pointers)j.p(pointer.name,pointer.value);
    j.key("weak_identities");{DiagnosticJson ids(f);const struct {const char* name;WeakIdentity value;} values[]{
        {"world",s.worldIdentity},{"pc",s.pcIdentity},{"pawn",s.pawnIdentity},{"connection",s.connectionIdentity},
        {"player_state",s.playerStateIdentity},{"movement",s.movementIdentity},{"input",s.inputIdentity}};
        for(const auto& value:values){ids.key(value.name);std::fprintf(f,"[%d,%d]",value.value.index,value.value.serial);}}
    j.key("queued_indices");std::fputc('[',f);bool first=true;for(const auto& slot:s.queue)if(slot.present){if(!first)std::fputc(',',f);first=false;std::fprintf(f,"%u",slot.move.index);}std::fputc(']',f);
    j.key("queued_host_client_indices");std::fputc('[',f);first=true;for(const auto& slot:s.queue)if(slot.present){if(!first)std::fputc(',',f);first=false;std::fprintf(f,"[%u,%u]",slot.move.index,slot.clientIndex);}std::fputc(']',f);
    j.key("barrier_client_times");std::fputc('[',f);first=true;for(const auto& item:s.barrierTimes)if(item.present){
        if(!first)std::fputc(',',f);first=false;std::fprintf(f,"[%d,",item.eventId);
        if(std::isfinite(item.clientSeconds))std::fprintf(f,"%.9g",double(item.clientSeconds));else std::fputs("null",f);
        std::fputc(']',f);}std::fputc(']',f);
    j.key("last_known_input");WriteMoveDiagnostic(f,s.lastKnownInput);
    j.key("last_input_wire");WriteInputWireDiagnostic(f,s.lastInputWire);
    j.key("original_correction_publication");{const auto& p=s.correctionPublication;DiagnosticJson a(f);
        a.b("observed",p.observed);a.b("consumed",p.consumed);a.i("auth_index",p.authIndex);a.i("origin_index",p.originIndex);
        a.u("observations",p.observations);a.u("packet",p.packet);a.p("inventory",p.inventory);a.p("corrected",p.corrected);
        a.key("inventory_identity");std::fprintf(f,"[%d,%d]",p.inventoryIdentity.index,p.inventoryIdentity.serial);
        a.key("corrected_identity");std::fprintf(f,"[%d,%d]",p.correctedIdentity.index,p.correctedIdentity.serial);}
    j.key("correction_cancellation");{const auto& d=s.correctionCancellation;DiagnosticJson a(f);
        a.t("stage",d.stage);a.u("packet",d.packet);a.u("passed_mask",d.passedMask);a.u("required_mask",0x7fff);
        a.i("first_event_id",d.firstEventId);a.i("old_base",d.oldBase);a.i("old_count",d.oldCount);
        a.i("new_base",d.newBase);a.i("new_count",d.newCount);a.i("auth_index",d.authIndex);
        a.i("origin_index",d.originIndex);a.i("acknowledged_index",d.acknowledgedIndex);a.u("cancelled_count",d.cancelledCount);
        a.p("reported",d.reported);a.p("corrected",d.corrected);a.key("reported_identity");
        std::fprintf(f,"[%d,%d]",d.reportedIdentity.index,d.reportedIdentity.serial);
        a.b("cancelled_events_counted_applied",false);a.b("ack_submitted_by_cancellation",false);
        a.key("replacement_frame");{const auto& proof=d.replacementFrame;DiagnosticJson frame(f);
            frame.b("matched",proof.matched);frame.t("reason",native_input_checkpoint_frame::Name(proof.reason));
            frame.u("compared",proof.compared);frame.u("aim_ordinal",proof.aimOrdinal);
            frame.u("intervening_states",proof.interveningStates);frame.u("equippable_states",proof.equippableStates);
            frame.u("activation_states",proof.activationStates);frame.u("movement_states",proof.movementStates);
            frame.u("native_type3_states",proof.type3States);frame.u("native_type10_states",proof.type10States);
            frame.u("failed_ordinal",proof.failedOrdinal);frame.u("retained_type",proof.retainedType);frame.u("wire_type",proof.wireType);}}
    const auto writeCheckpointDecode=[&](const char* key,const InputCheckpointDecodeDiagnostic& d){
        j.key(key);DiagnosticJson a(f);a.b("captured",d.captured);a.b("buffers_readable",d.buffersReadable);
        a.u("packet",d.packet);a.u("epoch",d.epoch);a.i("auth_index",d.authIndex);a.i("origin_index",d.originIndex);
        a.i("acknowledged_index",d.acknowledgedIndex);a.i("wire_count",d.wireCount);a.i("retained_count",d.retainedCount);
        a.u("copied_wire",d.copiedWire);a.u("copied_retained",d.copiedRetained);
        a.t("scope","fully_native_decoded_scratch_and_retained_overlap_readonly");
        a.t("retained_barrier_time_domain","may_already_be_mapped_use_barrier_client_times_for_raw_comparison");
        a.b("authorizes_cancellation",false);a.b("first_actual_append_claimed",false);
        a.key("intake");WriteInputWireDiagnostic(f,d.intake);
        a.key("wire_native_decode");std::fputc('[',f);
        for(uint32_t i=0;i<d.copiedWire&&i<d.wire.size();++i){if(i)std::fputc(',',f);
            WriteInputEventDiagnostic(f,d.wire[i].eventId,d.wire[i].bytes);}std::fputc(']',f);
        a.key("retained_overlap_native_decode");std::fputc('[',f);
        for(uint32_t i=0;i<d.copiedRetained&&i<d.retained.size();++i){if(i)std::fputc(',',f);
            WriteInputEventDiagnostic(f,d.retained[i].eventId,d.retained[i].bytes);}std::fputc(']',f);
    };
    writeCheckpointDecode("first_eligible_checkpoint_decode",s.firstEligibleCheckpointDecode);
    writeCheckpointDecode("latest_checkpoint_decode",s.latestCheckpointDecode);
    j.key("input_wire_prefix");std::fputc('[',f);for(uint32_t i=0;i<s.inputWireTraceCount&&i<s.inputWireTraces.size();++i){
        if(i)std::fputc(',',f);WriteInputWireDiagnostic(f,s.inputWireTraces[i]);}std::fputc(']',f);
    j.key("native_input_apply");{const auto& d=s.inputApplyDiagnostic;DiagnosticJson a(f);
        a.u("attempt",d.attempt);a.u("host_index",d.hostIndex);a.n("host_seconds",d.hostSeconds);a.u("prefix",d.prefix);
        a.i("base",d.base);a.i("count",d.count);a.i("eligible",d.eligible);
        a.i("applied_before",d.appliedBefore);a.i("applied_after",d.appliedAfter);
        a.p("inventory_before",d.inventory);a.p("reported_target_before",d.reportedTarget);a.p("client_predicted_before",d.clientPredicted);
        a.p("current_before",d.current);a.p("desired_before",d.desired);a.p("corrected_before",d.corrected);
        a.i("acknowledged_correction_index_before",d.acknowledgedCorrectionIndex);a.i("auth_correction_index_before",d.authCorrectionIndex);
        a.i("client_correction_index_before",d.clientCorrectionIndex);a.i("origin_correction_index_before",d.originCorrectionIndex);
        a.u("correction_reset_pending_before",d.correctionResetPending);a.u("inventory_correction_state_before",d.inventoryCorrectionState);
        a.u("initial_correction_sent_before",d.initialCorrectionSent);a.u("initial_correction_processed_before",d.initialCorrectionProcessed);
        a.b("native_attempt_cache_readable",d.nativeAttemptCacheReadable);a.key("last_native_attempt");WriteInputEventDiagnostic(f,-1,d.nativeAttempt);
        a.key("pending_prefix_before");std::fputc('[',f);for(uint32_t i=0;i<d.copiedEvents&&i<d.events.size();++i){
            if(i)std::fputc(',',f);WriteInputEventDiagnostic(f,d.events[i].eventId,d.events[i].bytes);}std::fputc(']',f);}
    j.key("watermarks");std::fputc('[',f);for(uint32_t i=0;i<s.watermarkCount&&i<s.watermarks.size();++i){if(i)std::fputc(',',f);std::fprintf(f,"[%u,%d]",s.watermarks[i].index,s.watermarks[i].eventId);}std::fputc(']',f);
    j.key("first_move_batch");WritePacketDiagnostic(f,s.firstMoveBatch);
    j.key("first_move_records");std::fputc('[',f);for(uint32_t i=0;i<s.firstMoveBatch.count&&i<16;++i){if(i)std::fputc(',',f);WriteMoveDiagnostic(f,s.firstMoveBatchRecords[i]);}std::fputc(']',f);
    j.key("packet_prefix");std::fputc('[',f);for(uint32_t i=0;i<s.sequencePacketCount&&i<32;++i){if(i)std::fputc(',',f);WritePacketDiagnostic(f,s.sequencePackets[i]);}std::fputc(']',f);
    j.key("sequence_fault");{const auto& a=s.sequenceFault;DiagnosticJson fault(f);fault.b("captured",a.captured);fault.u("error",a.error);
        fault.u("packet",a.packet);fault.u("ordinal",a.ordinal);fault.u("index",a.index);fault.u("mapped_host_index",a.mappedIndex);
        fault.i("mapping_offset",a.clockOffset);fault.i("ahead",a.ahead);fault.u("expected",a.expected);
        fault.u("clock",a.clock);fault.u("consumed",a.consumed);fault.u("last_received",a.lastReceived);fault.u("packet_count",a.packetCount);
        fault.u("queue_count",a.queueCount);fault.u("queue_minimum",a.queueMinimum);fault.u("queue_maximum",a.queueMaximum);fault.u("reader_bit",a.readerBit);
        fault.n("budget_seconds",a.budgetSeconds);fault.n("clock_accumulated",a.clockAccumulated);fault.n("host_elapsed_seconds",a.hostElapsedSeconds);
        fault.key("moves");std::fputc('[',f);for(uint32_t i=0;i<a.packetCount&&i<16;++i){if(i)std::fputc(',',f);WriteMoveDiagnostic(f,a.moves[i]);}std::fputc(']',f);}
    j.key("client_native_clock");{const auto& d=s.clientClock;DiagnosticJson clock(f);
        clock.t("stage",d.stage);clock.b("observed",d.observed);clock.b("local_owner",d.localOwner);
        clock.b("profiles_verified",d.profilesVerified);clock.b("roundtrip",d.roundtrip);clock.u("observations",d.observations);
        clock.u("thread",d.thread);clock.u("raw_index",d.rawIndex);clock.i("timestep",d.timestep);
        clock.p("pawn",d.pawn);clock.p("movement",d.movement);clock.p("prediction",d.prediction);clock.p("clock",d.clock);
        clock.n("host_seconds_before",d.hostSecondsBefore);clock.n("client_seconds_after",d.clientSecondsAfter);clock.n("host_seconds_after",d.hostSecondsAfter);}
    const auto bytes=[&](const char* name,const auto& data,uint32_t count){j.key(name);std::fputc('"',f);for(uint32_t i=0;i<count&&i<data.size();++i)std::fprintf(f,"%02x",unsigned(data[i]));std::fputc('"',f);};
    j.u("first_move_packet_bytes",s.firstMovePacketBytes);j.u("first_move_packet_bits",s.firstMovePacketBits);bytes("first_move_packet_hex",s.firstMovePacketData,s.firstMovePacketBytes);
    j.u("sequence_fault_packet_bytes",s.sequenceFaultPacketBytes);j.u("sequence_fault_packet_bits",s.sequenceFaultPacketBits);bytes("sequence_fault_packet_hex",s.sequenceFaultPacketData,s.sequenceFaultPacketBytes);
    j.u("failed_decoder_packet",s.failedDecoderPacket);j.u("failed_decoder_result",s.failedDecoderResult);
    j.u("failed_decoder_start_bit",s.failedDecoderStart);j.u("failed_decoder_position_bit",s.failedDecoderPosition);
    j.b("failed_decoder_reader_error",s.failedDecoderReaderError);j.u("failed_decoder_packet_bytes",s.failedDecoderPacketBytes);
    j.u("failed_decoder_packet_bits",s.failedDecoderPacketBits);bytes("failed_decoder_packet_hex",s.failedDecoderPacketData,s.failedDecoderPacketBytes);
    j.u("expected_rpc_bytes",s.expectedRPCBytes);j.u("actual_rpc_bytes",s.actualRPCBytes);j.u("actual_rpc_flags",s.actualRPCFlags);
    j.u("expected_rpc_exec_rva",s.expectedRPCExecRva);j.u("actual_rpc_exec_rva",s.actualRPCExecRva);j.key("failed_rpc_name");
    if(!s.failedRPCName)std::fputs("null",f);else{std::fputc('"',f);for(const auto* p=s.failedRPCName;*p;++p){
        const unsigned c=static_cast<unsigned>(*p);if(c=='"'||c=='\\')std::fprintf(f,"\\%c",char(c));
        else if(c<0x20||c>=0x7f)std::fprintf(f,"\\u%04x",c&0xffff);else std::fputc(char(c),f);}std::fputc('"',f);}
}
}
