#pragma once
#include "local_server_owned_paths.h"
#include "original_players.h"
#include "original_weapon.h"
#include "original_input_bindings.h"
#include "local_server_spectator_view.h"
#include "local_server_peek_fixture_contract.h"
#include "local_server_hold_fixture_contract.h"
#include "all_weapons.h"
#include "local_server_native_shop_plan.h"
#include "local_server_native_shop_snapshot.h"
#include <atomic>
#include "local_server_hero_skill_readback.h"
#include "local_server_abilities.h"
#include "local_server_native_jett_plan.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>

// Explicit development fixture only. No marker + matching original engine
// command-line token means no engine actions. It exercises original local input
// APIs; normal prediction, native ticks and socket transport remain the producer.
// It never supplies a shot, server hit, actor transform, role, resource or result.
// Evidence: work/local-server-playable/input-authority/native-test/.
namespace local_server_native_test {
inline constexpr uint64_t MarkerMagic=0x31545345544e534cULL; // LSNTEST1
inline constexpr uint32_t MarkerVersion=1,ExplicitCLIFlag=1;
enum class Scenario:uint32_t {None=0,Shoot=1,Shop=2,ShootTarget=3,FfaBattle=4,PhoenixCPrimary=5,JettC=6,JettQ=7,JettE=8,JettX=9,JettXTarget=10,PeekFixture=13,ShopCorrectionPrelude=12,MapHoldFixture=14,SpectatorInput=15};
struct Marker {
    uint64_t magic=MarkerMagic;
    uint32_t version=MarkerVersion,bytes=64,flags=ExplicitCLIFlag,scenario=0;
    uint64_t nonce=0;
    uint32_t warmupMs=500,holdMs=500,waitMs=10000,totalMs=20000;
    uint32_t reserved[4]{};
};
static_assert(sizeof(Marker)==64&&offsetof(Marker,nonce)==24&&offsetof(Marker,reserved)==48);
struct Profile {uint32_t rva,bytes;uint64_t fnv;};
inline constexpr Profile RequiredProfiles[]{
    {0x19d0320,44,0xaa94462619c48c39ULL},
    {0x6c6b8a0,393,0x504fa7e948e4c592ULL},
    {0x7881e80,1033,0x8b9c2c0cc07350bdULL},
    {0x6c6a460,263,0x4220890dd921b864ULL},
    {0x48548e0,50,0x5598334b3612dde7ULL},
    {0x40e6df0,385,0xccee9eb5e245e130ULL},
    {0x6492f30,174,0x7ebed034600be7a7ULL},
    {0x64eb760,255,0xaa13aa46b67b5f51ULL},
    {0x64eb4a0,97,0xab7fd5198f6cb527ULL},
    {0x64dc0b0,32,0x5bd6a1a8bd134236ULL},
    {0x64a6650,54,0x7df377088311cc9fULL},
    {0x6cb7500,44,0xe29e22c700e3717eULL},
    {0x78b2640,90,0x0fb2684cecd2a5f7ULL},
    {0x7a8d8e0,101,0x6af654c68ea25a8aULL},
    {0x3efc820,176,0x7f1113b3f79164b2ULL},
    {0x1cf35b0,1571,0x40614eae57888396ULL},
    {0x1d4cfa0,1562,0xb050d7efb4cf1a25ULL},
};
// Only the explicit target scenario requires these additional original query
// and controller-input APIs. Old shoot/shop gates remain independent.
inline constexpr Profile TargetProfiles[]{
    {0x42ac0d0,73,0x0c96ab96a594e6997ULL},
    {0x42ab2c0,431,0x6f0dd70e71d78ae7ULL},
    {0x42abfd0,242,0x5995cf1fd8ef7ea8ULL},
    {0x3ef1cd0,121,0x265b561c6c7e873dULL},
    {0x6c62390,56,0x29153e2fde2f0f5eULL},
    {0x454fbb0,535,0xb1003b9a7e2b38deULL},
    {0x45c3a90,2090,0x1aa61200f055b8baULL},
    {0x781fc90,198,0xda469337ca3383ccULL},
    {0x3ed83b0,1609,0x15d2d4ecc2f49cf7ULL},
    {0x4526580,174,0x30f13d115e1c707cULL},
    {0x45aa110,174,0xba5c7c6db308bcf3ULL},
};
struct Weak {int32_t index=-1,serial=0;};
static_assert(sizeof(Weak)==8);
struct Bound {void* object=nullptr;Weak identity{};};
struct Array {void** data=nullptr;int32_t count=0,capacity=0;};
enum class Phase:uint8_t {Disabled,WaitingOwner,Warming,ShootHeld,ShootReleased,ShopOpening,ShopHolding,ShopClosing,Complete,Failed,TargetCooldown,BattleReloadHeld,BattleReloadReleased,BattleReloading,BattleRespawn};
enum class TargetMethod:size_t {ViewPoint,Location,Alive,LookAt,SetRotation,GetRotation,Trace,Count};
inline constexpr uint32_t MaximumTargetBursts=6;
inline constexpr double TargetAimInterval=.01,TargetCooldownSeconds=1.;
inline constexpr uint32_t BattleTotalMs=660000; // 9 min original match + 2 min setup/grace.
enum class BattleMethod:size_t {BoneIndex,Ammo,CanReload,Active,Winner,Bot,ForceInput,CurrentInput,Component,Count};
struct BattleMethodCheck {
    bool checked=false,found=false,weakVerified=false;
    uint16_t expectedBytes=0,actualBytes=0;
    uint32_t expectedFlags=0,actualFlags=0,failureMask=0;
    uintptr_t expectedExec=0,actualExec=0,ownerGetter=0;
    void *function=nullptr,*expectedOwner=nullptr,*actualOwner=nullptr;
};
inline const char* BattleMethodName(BattleMethod method){
    constexpr const char* names[]{"SkinnedMeshComponent.GetBoneIndex","AmmoComponent.GetAmmo","AmmoComponent.CanReload",
        "ShooterGameState.IsGameplayActive","ShooterGameState.GetWinningTeam","PlayerState.IsABot",
        "ShooterCharacter.ForceInputState","ShooterCharacter.GetCurrentInputState","Actor.GetComponentByClass"};
    const auto n=static_cast<size_t>(method);return n<std::size(names)?names[n]:"invalid";
}
struct BattleName {uint32_t words[3]{};};
struct BattleResult {void* winningTeam=nullptr;int32_t winningPoints=0;uint8_t completion=0,padding[3]{};};
struct alignas(8) BattleInfo {
    void* timeoutPlayer=nullptr;
    float displayRemaining=0,localDisplayRemaining=0,stateRemaining=0,localStateRemaining=0;
    float phaseElapsed=0,authStart=-1,authEnd=-1,localStart=-1,localEnd=-1,timeoutExpiry=-1;
    uint8_t timeAdvancing=0,store=0,infiniteAbilities=0,infiniteAmmo=0,infiniteReserve=0,infiniteMoney=0;
    uint8_t ignoreShopping=0,matchPaused=0,intention=0,active=0,dropInSystem=0,dropIn=0;
    uint8_t megamapPings=0,timeoutState=0,padding[2]{};
};
static_assert(sizeof(BattleName)==12&&sizeof(BattleResult)==16&&sizeof(BattleInfo)==64);
struct BattleState {
    bool metadataVerified=false,botMetadataVerified=false,sessionBound=false,nativeActive=false;
    bool activeObserved=false,matchEndObserved=false,endRuleObserved=false,lifeWaiting=false,deathObserved=false;
    bool reloadHeld=false,reloadHeldObserved=false,releaseUnavailable=false;
    std::array<Bound,7> session{};
    std::array<Bound,static_cast<size_t>(BattleMethod::Count)> methods{};
    std::array<BattleMethodCheck,static_cast<size_t>(BattleMethod::Count)> methodChecks{};
    Bound forceBinding{},inputBinding{},gameState{},gameStateClass{};
    Bound observedDeadPawn{},deathQuery{};
    void *deathSamplePcPawn=nullptr,*deathSampleController=nullptr;
    uint64_t boundLifeSamples=0,originalLifeQueries=0,unownedOldLifeSamples=0,deathSampleAtMs=0;
    uint64_t oldLifeNativeReleaseBatches=0,oldLifePrimaryReleaseObserved=0,oldLifeReloadReleaseObserved=0;
    uint64_t oldLifePrivateIntentRetirements=0,retiredPrimaryIntents=0,retiredReloadIntents=0;
    uint8_t lastLifeAlive=255,deathAlive=255;bool completedAfterRetiredDeath=false;
    BattleName inProgress{},postMatch{};BattleInfo info{};
    uint8_t activeBeforeQuery=255,activeQueryResult=255,activeAfterQuery=255;
    uint64_t activeQueryRefreshes=0;float activeQueryAuthStart=-1,activeQueryAuthEnd=-1;
    void *botProperty=nullptr,*winningTeam=nullptr,*headMesh=nullptr,*ammoComponent=nullptr;
    uint64_t wallStartMs=0,wallElapsedMs=0,deathAtMs=0,endReleaseAtMs=0,lifeBindings=0,deathsObserved=0,respawnsObserved=0;
    uint64_t matchObservations=0,npcProofs=0,npcProofMissing=0,headQueries=0,headPointsProved=0,headLeaderPending=0,headPoseFrame=0;
    uint64_t ammoQueries=0,reloadQueries=0,reloadPresses=0,reloadReleases=0,reloadEdgesCompleted=0,reloadMagazineObserved=0;
    int32_t headBone=-1,ammo=-1,minimumAmmo=-1,winningPoints=0;
    uint8_t completion=0,reloadNow=255;bool canReload=false;float authStart=-1;
};

inline constexpr uint32_t SkillPreparationMs=30000,SkillTotalMs=45000,SkillReleaseMs=10000;
struct SkillPrimaryState {
    Bound alive{},ability{},machine{},respond{};
    bool metadata=false,selectionBound=false,afterSampled=false,beforeInputKnown=false,afterInputKnown=false;
    bool privateIntentRetired=false,releaseUnavailable=false;
    uint32_t epoch=0;uint64_t registration=0,startedAtMs=0,readyAtMs=0,pressedAtMs=0,releasedAtMs=0,samples=0;
    int32_t beforeAccepted=-1,afterAccepted=-1;
};

inline constexpr Profile JettProfiles[]{
    {0x6493750,47,0x8938d23c827db2a4ULL},
    {0x64a7370,47,0x7565ad1a3aeac49fULL},
    {0x646e9a0,47,0xd3e53c5feb94e836ULL},
    {0x7882290,1063,0x28b4834d58e79bcfULL},
    {0x70a4d30,213,0x7ac764b4b030092ULL},
    {0x6c6b5e0,694,0xf694cd8426218e40ULL},
    {0x6c6a350,263,0x3dae1dc43c5e438aULL},
};
struct JettHeadState {
    Bound boneIndex{},pawn{},mesh{},asset{};BattleMethodCheck method{};
    original_jett_priming::PropertyInfo namesField{};std::array<original_jett_priming::PropertyInfo,2> parameters{};
    bool profilesVerified=false,metadataVerified=false,pointValid=false,rootObserved=false;
    uint32_t fieldRejectMask=0;uint64_t queries=0,proved=0,leaderPending=0,posePending=0,poseFrame=0;
    int32_t bone=-1,poseBuffer=-1,poseCount=0;BattleName boneName{};
    engine::Vec rootPoint{},headPoint{},componentBone{};
    const char* stage="native_jett_head_not_queried";
    bool losAttempted=false,losReturned=false,losBlocking=false,losDistanceFinite=false,losClear=false;
    uint8_t losReturnValue=255;void* losPawn=nullptr;Weak losPawnId{},hitActor{},hitComponent{};
    uint64_t losQueries=0,losAtTick=0;double losLength=0,losDistance=0;
    engine::Vec losStart{},losEnd{};std::array<uint8_t,0xf0> hitBytes{};
};
struct JettTargetState {
    Bound enemyFunction{},teamDefault{},pawnAtPress{},psAtPress{},gsAtPress{};
    std::array<original_jett_priming::PropertyInfo,3> enemyFields{};
    bool metadataVerified=false,prePrimaryReady=false,enemyAtPress=false,holdLost=false,deadRelease=false;
    uint32_t fieldRejectMask=0;uint8_t lastAimResult=255,lastEnemyValue=255;
    uint64_t enemyQueries=0,enemyRejected=0,aimSamples=0,readyAtTick=0,primaryAtTick=0,readbacksAtPress=0;
};
struct JettState {
    Bound activation{},currentActivation{},ability{};
    local_server_native_jett_plan::Plan plan{};
    local_server_native_jett_plan::PendingInput pending{};
    std::array<local_server_native_jett_plan::FieldShape,6> parameterChecks{};
    uint32_t fieldRejectMask=0;
    bool fieldsVerified=false,pressQueueKnown=false,releaseQueueKnown=false,pressQueuedMatches=false,releaseQueuedMatches=false;
    int32_t pressQueueBefore=-1,pressQueueAfter=-1,releaseQueueBefore=-1,releaseQueueAfter=-1;
    uint8_t pressImmediate=255,releaseImmediate=255;
    uint64_t pendingSamples=0,activationReadbackTick=0,primaryReadbackTick=0;
    bool methodsReady=false,bound=false,activationHeld=false,inputComplete=false,primed=false,equipped=false;
    bool privateIntentRetired=false,releaseUnavailable=false,primaryPressObserved=false,primaryReleaseObserved=false;
    bool primedObserved=false,equippedObserved=false;
    uint8_t slot=255,activationNow=255;uint32_t epoch=0;uint64_t registration=0,startedAt=0,samples=0;
    uint64_t activationCalls=0,activationQueries=0,activationPressReadbacks=0,activationReleaseReadbacks=0;
    int32_t chargeBefore=-1,chargeAfter=-1;uint64_t transitionsBefore=0,transitionsAfter=0;
};

struct State {
    bool initialized=false,configured=false,cliMatched=false,armed=false,profilesVerified=false;
    bool bound=false,finished=false,failed=false,actionsCompleted=false,stopRequested=false;
    bool nativeLocal=false,primaryHeldRequested=false,releaseSubmitted=false,releaseObserved=false;
    bool pressSubmitted=false,primaryHeldObserved=false;
    bool shopOpenKnown=false,shopOpen=false,shopOpenObserved=false,shopClosedObserved=false;
    bool shopOpenSubmitted=false,shopCloseSubmitted=false,shopEligibilityKnown=false,shopEligible=false;
    bool ownerEligibilityKnown=false,ownerEligible=false,metadataVerified=false;
    uint32_t error=0,profileFailureRva=0,expectedThread=0,actualThread=0;
    uint64_t ticks=0,ownershipPassedMask=0,readinessPassedMask=0,forceCalls=0,releaseCalls=0;
    uint64_t inputStateQueries=0,toggleCalls=0,shopOpenQueries=0,eligibilityQueries=0;
    uint64_t idleFrames=0;
    void *idleWeapon=nullptr,*predictedMachine=nullptr,*predictedState=nullptr;
    char predictedStateName[128]{};
    uint8_t primaryBefore=0xfe,primaryNow=0xfe,pawnRole=0,pcRole=0,pawnRemoteRole=0;
    int32_t netMode=-1,connectionState=-1,localPlayers=0;
    Phase phase=Phase::Disabled;
    Marker marker{};
    double elapsedSeconds=0,readySeconds=0,phaseSeconds=0;
    const char* stage="native_test_disabled";
    const char* operation="none";
    const wchar_t* failedFunction=nullptr;
    uint16_t actualFunctionBytes=0;uint32_t actualFunctionFlags=0;uintptr_t actualFunctionExec=0;
    void *world=nullptr,*pc=nullptr,*ps=nullptr,*pawn=nullptr,*localPlayer=nullptr,*gameInstance=nullptr;
    void *driver=nullptr,*connection=nullptr,*input=nullptr,*inputState=nullptr,*inventory=nullptr,*current=nullptr;
    void *playerInfo=nullptr,*ui=nullptr,*coordinator=nullptr;
    void *forceFunction=nullptr,*currentInputFunction=nullptr,*toggleFunction=nullptr,*isOpenFunction=nullptr;
    void *canOpenFunction=nullptr,*ownerCanOpenFunction=nullptr,*getPlayerInfoFunction=nullptr;
    std::array<Bound,11> identities{};
    BattleState battle{};
    SkillPrimaryState skill{};
    JettState jett{};
    JettTargetState jettTarget{};
    JettHeadState jettHead{};
    bool targetProfilesVerified=false,targetMetadataVerified=false,targetSelected=false,targetVisible=false;
    bool targetEverVisible=false,targetAlive=false,targetDeadObserved=false,burstHeldObserved=false;
    uint32_t targetBurstsStarted=0,targetBurstsCompleted=0;
    uint64_t targetScans=0,targetCandidates=0,targetInvalid=0,targetBlocked=0,targetLocalRejected=0,targetRetirements=0;
    uint64_t targetTraceQueries=0,targetAimCalls=0,targetAimReadbacks=0;
    double nextTargetAimSeconds=0,lastTargetAimSeconds=-1,targetDistance=0,lastObstructionDistance=-1;
    engine::Vec targetView{},targetPoint{},targetRequestedRotation{},targetControlReadback{};
    void *targetPawn=nullptr,*targetPS=nullptr,*targetGS=nullptr,*targetMathDefault=nullptr,*targetSystemDefault=nullptr;
    Weak targetPawnId{},targetPSId{},targetGSId{},targetMathId{},targetSystemId{};
    std::array<Bound,static_cast<size_t>(TargetMethod::Count)> targetMethods{};
};
inline State state;
inline const State& GetState(){return state;}
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Fail(uint32_t error,const char* stage){state.error=error;state.stage=stage;state.failed=true;state.finished=true;state.phase=Phase::Failed;return false;}
inline bool Wait(const char* stage){state.stage=stage;return true;}
inline bool SameWeak(const Weak& a,const Weak& b){return a.index==b.index&&a.serial==b.serial;}
inline void* Resolve(const Weak& value){return value.index>=0&&value.serial>0?
    engine::function<void*(*)(const Weak*)>(0x1d4cfa0)(&value):nullptr;}
inline bool Identity(void* object,Weak& value){
    value.index=Read<int32_t>(object,0xc);
    if(!object||value.index<0)return false;
    value.serial=engine::function<int32_t(*)(void*,int32_t)>(0x1cf35b0)
        (reinterpret_cast<void*>(engine::image+0xd7b87c0),value.index);
    return value.serial>0&&Resolve(value)==object;
}
inline bool ProfileMatches(const Profile& profile){
    const auto data=reinterpret_cast<const uint8_t*>(engine::image+profile.rva);
    if(!original_players::Readable(data,profile.bytes))return false;
    uint64_t value=14695981039346656037ULL;
    for(uint32_t i=0;i<profile.bytes;++i)value=(value^data[i])*1099511628211ULL;
    return value==profile.fnv;
}
inline bool VerifyProfiles(){
    if(state.profilesVerified)return true;
    for(const auto& profile:RequiredProfiles)if(!ProfileMatches(profile)){
        state.profileFailureRva=profile.rva;return Fail(4901,"native_test_original_profile_changed");
    }
    state.profilesVerified=true;return true;
}
inline bool TokenAt(const wchar_t* command,const wchar_t* token){
    if(!command||!token)return false;
    const size_t length=std::wcslen(token);
    size_t total=0;while(total<32768&&command[total])++total;if(total==32768)return false;
    for(size_t i=0;i<total;++i){
        if(i&&command[i-1]!=L' '&&command[i-1]!=L'\t'&&command[i-1]!=L'\n')continue;
        if(length>total-i)continue;
        if(std::wcsncmp(command+i,token,length))continue;
        const auto after=command[i+length];
        if(!after||after==L' '||after==L'\t'||after==L'\n')return true;
    }
    return false;
}
inline bool ArmFromOriginalCommandLine(){
    if(state.armed)return true;
    if(!Read<uint8_t>(reinterpret_cast<void*>(engine::image),0xd958b90))
        return Wait("native_test_waiting_original_command_line_initialized");
    const auto command=engine::function<const wchar_t*(*)()>(0x19d0320)();
    const bool shoot=TokenAt(command,L"-LocalServerNativeTest=shoot"),shop=TokenAt(command,L"-LocalServerNativeTest=shop");
    const bool target=TokenAt(command,L"-LocalServerNativeTest=shoot-target");
    const bool battle=TokenAt(command,L"-LocalServerNativeTest=ffa-battle");
    const bool skill=TokenAt(command,L"-LocalServerNativeTest=phoenix-c-primary");
    const bool jettC=TokenAt(command,L"-LocalServerNativeTest=jett-c");
    const bool jettQ=TokenAt(command,L"-LocalServerNativeTest=jett-q");
    const bool jettE=TokenAt(command,L"-LocalServerNativeTest=jett-e");
    const bool jettX=TokenAt(command,L"-LocalServerNativeTest=jett-x");
    const bool jettXTarget=TokenAt(command,L"-LocalServerNativeTest=jett-x-target");
    const bool shopPreludeToken=TokenAt(command,L"-LocalServerNativeTest=shop-correction-prelude");
    const bool peekFixture=TokenAt(command,L"-LocalServerNativeTest=peek-fixture");
    const bool mapHoldFixture=TokenAt(command,L"-LocalServerNativeTest=map-hold-fixture");
    const bool spectatorInput=TokenAt(command,L"-LocalServerNativeTest=spectator-input-fixture");
    const unsigned tokens=unsigned(spectatorInput)+unsigned(peekFixture)+unsigned(shoot)+unsigned(shop)+unsigned(target)+unsigned(battle)+unsigned(skill)
        +unsigned(jettC)+unsigned(jettQ)+unsigned(jettE)+unsigned(jettX)+unsigned(jettXTarget)+unsigned(shopPreludeToken)+unsigned(mapHoldFixture);
    state.cliMatched=tokens==1&&(state.marker.scenario==15?spectatorInput:state.marker.scenario==14?mapHoldFixture:state.marker.scenario==13?peekFixture:state.marker.scenario==12?shopPreludeToken:state.marker.scenario==6?jettC:state.marker.scenario==7?jettQ
        :state.marker.scenario==8?jettE:state.marker.scenario==9?jettX:state.marker.scenario==10?jettXTarget
        :state.marker.scenario==uint32_t(Scenario::PhoenixCPrimary)?skill:(state.marker.scenario==uint32_t(Scenario::Shoot)?shoot
        :state.marker.scenario==uint32_t(Scenario::Shop)?shop
        :state.marker.scenario==uint32_t(Scenario::ShootTarget)?target:battle));
    if(!state.cliMatched)return Fail(4902,"native_test_marker_requires_matching_explicit_engine_cli_token");
    state.armed=true;state.phase=Phase::WaitingOwner;state.stage="native_test_waiting_actual_role2_owner";return true;
}
inline void ConfigureShopPreludeSnapshot(uint64_t nonce);
inline bool Init(const wchar_t* directory){
    if(state.initialized)return !state.error;
    state.initialized=true;
    if(!directory||!directory[0])return true;
    const size_t length=std::wcslen(directory);
    std::wstring canonical;
    if(length>1000||!local_server_owned_paths::CanonicalAbsolute(directory,canonical)
       ||(!local_server_owned_paths::RoleDirectory(canonical,1)&&!local_server_owned_paths::RoleDirectory(canonical,2)))
        return Fail(4903,"native_test_directory_must_be_explicit_owned_pair_path");
    std::wstring path(directory);if(path.back()!=L'\\'&&path.back()!=L'/')path+=L'\\';path+=L"native-test-scenario.bin";
    FILE* file=nullptr;if(_wfopen_s(&file,path.c_str(),L"rb")||!file)return true;
    Marker marker{};const size_t read=std::fread(&marker,1,sizeof(marker),file);const int extra=std::fgetc(file);std::fclose(file);
    if(marker.scenario==15){
        if(read!=sizeof(marker)||extra!=EOF||marker.magic!=MarkerMagic||marker.version!=MarkerVersion||marker.bytes!=sizeof(Marker)
           ||marker.flags!=ExplicitCLIFlag||!marker.nonce||marker.warmupMs!=500||marker.holdMs!=0||marker.waitMs!=10000||marker.totalMs!=45000
           ||marker.reserved[0]>7||marker.reserved[1]||marker.reserved[2]||marker.reserved[3])return Fail(4904,"native_spectator_input_explicit_marker_schema_invalid");
    }else if(marker.scenario==14){
        if(read!=sizeof(marker)||extra!=EOF||marker.magic!=MarkerMagic||marker.version!=MarkerVersion||marker.bytes!=sizeof(Marker)
           ||marker.flags!=ExplicitCLIFlag||!marker.nonce||marker.warmupMs!=500||marker.holdMs!=500||marker.waitMs!=10000||marker.totalMs!=45000
           ||marker.reserved[0]||marker.reserved[1]||marker.reserved[2]||marker.reserved[3])return Fail(4904,"native_map_hold_explicit_marker_schema_invalid");
    }else if(marker.scenario==13){
        if(read!=sizeof(marker)||extra!=EOF||marker.magic!=MarkerMagic||marker.version!=MarkerVersion||marker.bytes!=sizeof(Marker)
           ||marker.flags!=ExplicitCLIFlag||!marker.nonce||marker.warmupMs!=500||marker.holdMs!=500||marker.waitMs!=10000||marker.totalMs!=30000
           ||marker.reserved[0]||marker.reserved[1]||marker.reserved[2]||marker.reserved[3])return Fail(4904,"native_peek_explicit_marker_schema_invalid");
    }else if(marker.scenario==12){
        if(read!=sizeof(marker)||extra!=EOF||marker.magic!=MarkerMagic||marker.version!=MarkerVersion||marker.bytes!=sizeof(Marker)
           ||marker.flags!=ExplicitCLIFlag||!marker.nonce||marker.warmupMs!=500||marker.holdMs!=500
           ||marker.waitMs!=30000||marker.totalMs!=45000
           ||marker.reserved[0]||marker.reserved[1]||marker.reserved[2]||marker.reserved[3])
            return Fail(4904,"native_test_explicit_marker_schema_invalid");
    }else if(marker.scenario>=5&&marker.scenario<=10){
        if(read!=sizeof(marker)||extra!=EOF||marker.magic!=MarkerMagic||marker.version!=MarkerVersion||marker.bytes!=sizeof(Marker)
           ||marker.flags!=ExplicitCLIFlag||!marker.nonce||marker.warmupMs>5000||marker.holdMs!=500
           ||marker.waitMs!=SkillPreparationMs||marker.totalMs!=SkillTotalMs
           ||marker.reserved[0]||marker.reserved[1]||marker.reserved[2]||marker.reserved[3])
            return Fail(4904,"native_test_explicit_marker_schema_invalid");
    }else if(read!=sizeof(marker)||extra!=EOF||marker.magic!=MarkerMagic||marker.version!=MarkerVersion||marker.bytes!=sizeof(Marker)
       ||marker.flags!=ExplicitCLIFlag||!marker.nonce||(marker.scenario<1||marker.scenario>4)
       ||marker.warmupMs>5000||marker.holdMs!=500||marker.waitMs<1000||marker.waitMs>15000
       ||marker.totalMs<marker.waitMs+marker.holdMs||(marker.scenario==4?marker.totalMs!=BattleTotalMs:marker.totalMs>30000)
       ||marker.reserved[0]||marker.reserved[1]||marker.reserved[2]||marker.reserved[3])
        return Fail(4904,"native_test_explicit_marker_schema_invalid");
    state.marker=marker;state.configured=true;state.phase=Phase::WaitingOwner;
    state.stage="native_test_explicit_marker_waiting_engine_cli";
    if(marker.scenario==12)ConfigureShopPreludeSnapshot(marker.nonce);return true;
}
inline bool Function(void*& result,const wchar_t* path,uint16_t bytes,uint32_t flags,uintptr_t exec){
    result=engine::Find(path);
    const auto actualBytes=Read<uint16_t>(result,0xbe);
    const auto actualFlags=Read<uint32_t>(result,0xb8);
    const auto actualExec=Read<uintptr_t>(result,0xe0);
    if(result&&actualBytes==bytes&&actualFlags==flags&&actualExec==engine::image+exec)return true;
    state.failedFunction=path;state.actualFunctionBytes=actualBytes;state.actualFunctionFlags=actualFlags;
    state.actualFunctionExec=actualExec>=engine::image?actualExec-engine::image:actualExec;
    return Fail(4905,"native_test_original_function_abi_changed");
}
inline bool ResolveMethods(){
    if(state.metadataVerified)return true;
    if(!Function(state.forceFunction,L"/Script/ShooterGame.ShooterCharacter.ForceInputState",2,0x04020401,0x6c6b8a0)
       ||!Function(state.currentInputFunction,L"/Script/ShooterGame.ShooterCharacter.GetCurrentInputState",2,0x54020401,0x6c6a460)
       ||!Function(state.toggleFunction,L"/Script/ShooterGame.AresPlayerController.ToggleShop",4,0x04020401,0x64eb760)
       ||!Function(state.isOpenFunction,L"/Script/ShooterGame.AresPlayerController.IsShopOpen",1,0x54020401,0x64eb4a0)
       ||!Function(state.canOpenFunction,L"/Script/ShooterGame.ShopScreenCoordinator.GetCanOpenShop",1,0x54020401,0x64dc0b0)
       ||!Function(state.ownerCanOpenFunction,L"/Script/ShooterGame.OwnerExclusivePlayerInfo.CanOpenShop",1,0x5c020c00,0x64a6650)
       ||!Function(state.getPlayerInfoFunction,L"/Script/ShooterGame.ShooterPlayerController.GetOwnerExclusivePlayerInfo",8,0x54020401,0x6cb7500))return false;
    state.metadataVerified=true;return true;
}
inline bool Event(void* object,void* function,void* parameters){
    auto table=Read<void**>(object);auto dispatch=Read<void*>(table,0x298);
    if(!object||!function||!dispatch)return Fail(4906,"native_test_original_process_event_unavailable");
    reinterpret_cast<void(*)(void*,void*,void*)>(dispatch)(object,function,parameters);return true;
}
inline bool ValidateOwner(void* world,void* pc,void* ps,void* pawn){
    state.actualThread=GetCurrentThreadId();state.expectedThread=original_players::gameThread;
    if(!state.expectedThread||state.actualThread!=state.expectedThread)return Fail(4907,"native_test_requires_original_game_thread");
    state.world=world;state.pc=pc;state.ps=ps;state.pawn=pawn;state.ownershipPassedMask=0;
    const auto pass=[&](uint32_t bit,bool value){if(value)state.ownershipPassedMask|=uint64_t(1)<<bit;return value;};
    if(!pass(0,world&&original_players::IsA(pc,original_players::ShooterControllerClass)
        &&original_players::IsA(ps,original_players::ShooterStateClass)&&original_players::IsA(pawn,original_players::ShooterCharacterClass)))return true;
    state.pawnRole=Read<uint8_t>(pawn,0x224);state.pcRole=Read<uint8_t>(pc,0x224);state.pawnRemoteRole=Read<uint8_t>(pawn,0x128);
    pass(1,state.pawnRole==2&&state.pcRole==2);
    pass(2,Read<void*>(pc,0x4b0)==pawn&&Read<void*>(pawn,0x4a0)==pc);
    pass(3,Read<void*>(pc,0x470)==ps&&Read<void*>(pawn,0x488)==ps&&Read<void*>(ps,0x210)==pc);
    state.localPlayer=Read<void*>(pc,0x510);state.gameInstance=Read<void*>(world,0x1d8);
    const auto locals=Read<Array>(state.gameInstance,0x40);state.localPlayers=locals.count;
    pass(4,original_players::IsA(state.localPlayer,0x6cb4740)&&Read<void*>(state.localPlayer,0x38)==pc
        &&locals.count==1&&locals.capacity>=1&&locals.capacity<=32&&Read<void*>(locals.data)==state.localPlayer);
    state.driver=Read<void*>(world,0x40);state.connection=Read<void*>(state.driver,0xc0);
    state.connectionState=Read<int32_t>(state.connection,0x13c);
    pass(5,state.driver&&state.connection&&state.connectionState==3&&Read<void*>(state.driver,0x178)==world
        &&Read<void*>(state.connection,0x60)==state.driver&&Read<void*>(state.connection,0x38)==pc&&Read<void*>(state.connection,0xa0)==pc);
    state.netMode=engine::function<int32_t(*)(void*)>(0x3efc820)(pc);
    pass(6,state.netMode==3&&!Read<void*>(world,0x170));
    auto pawnTable=Read<void**>(pawn);
    pass(7,Read<void*>(pawnTable,0x830)==reinterpret_cast<void*>(engine::image+0x48548e0)
        &&Read<void*>(pawnTable,0x740)==reinterpret_cast<void*>(engine::image+0x40e6df0));
    if((state.ownershipPassedMask&0xff)!=0xff)return true;
    state.nativeLocal=engine::function<bool(*)(void*)>(0x48548e0)(pawn);pass(8,state.nativeLocal);
    state.input=engine::function<void*(*)(void*,void*)>(0x40e6df0)(pawn,engine::Class(0x6492f30));
    state.inputState=Read<void*>(pawn,0xba0);state.inventory=Read<void*>(pawn,0xc08);state.current=Read<void*>(state.inventory,0x280);
    pass(9,original_players::IsA(state.input,0x6492f30)&&Read<void*>(state.input,0xc8)==pawn&&Read<void*>(state.input,0x170)==pawn
        &&original_players::IsA(state.inputState,0x6494370)&&Read<void*>(state.inputState,0xc8)==pawn&&Read<void*>(state.inputState,0x148)==pawn);
    pass(10,original_players::IsA(state.inventory,0x6499330)&&Read<void*>(state.inventory,0xc8)==pawn&&Read<void*>(state.inventory,0x2f0)==pawn);
    pass(11,Read<uint8_t>(pawn,0xa90)==1);
    auto getWorld=Read<void*>(pawnTable,0x1b8);
    pass(12,getWorld&&reinterpret_cast<void*(*)(void*)>(getWorld)(pawn)==world);
    if(state.ownershipPassedMask!=0x1fff)return true;
    const std::array<void*,11> objects{world,pc,ps,pawn,state.localPlayer,state.gameInstance,state.driver,state.connection,state.input,state.inputState,state.inventory};
    if(!state.bound){
        std::array<Bound,11> captured{};
        for(size_t i=0;i<objects.size();++i)if(!Identity(objects[i],captured[i].identity))return true;else captured[i].object=objects[i];
        state.identities=captured;state.bound=true;
    }else for(size_t i=0;i<objects.size();++i)if(state.identities[i].object!=objects[i]||Resolve(state.identities[i].identity)!=objects[i])
        return Fail(4908,"native_test_owned_epoch_weak_identity_changed");
    return true;
}
inline bool QueryPrimary(){
    struct Params {uint8_t input=0,value=0xfe;} parameters;
    if(!Event(state.pawn,state.currentInputFunction,&parameters))return false;
    state.primaryNow=parameters.value;++state.inputStateQueries;return true;
}
inline bool PredictedIdle(){
    auto machine=Read<void*>(state.current,0xe00);
    auto selected=Read<void*>(machine,0x1b0);
    Weak machineIdentity{},selectedIdentity{};
    const bool valid=original_players::IsA(machine,0x67722c0)&&Identity(machine,machineIdentity)
        &&Identity(selected,selectedIdentity)&&Read<void*>(machine,0xc8)==state.current
        &&Read<uint8_t>(machine,0x348)!=0&&Read<void*>(selected,0xc8)==state.current
        &&Read<void*>(selected,0xe0)==machine;
    state.predictedStateName[0]=0;
    if(valid)original_weapon::OriginalObjectName(selected,state.predictedStateName);
    const bool idle=valid&&std::strcmp(state.predictedStateName,"IdleState")==0;
    const bool same=state.idleWeapon==state.current&&state.predictedMachine==machine&&state.predictedState==selected;
    state.idleFrames=idle?(same?state.idleFrames+1:1):0;
    state.idleWeapon=state.current;state.predictedMachine=machine;state.predictedState=selected;
    return state.idleFrames>=2;
}
inline bool ForcePrimary(bool held){
    struct Params {uint8_t input=0,value=0;} parameters{0,uint8_t(held?1:0)};
    state.operation=held?"original_force_primary_press":"original_force_primary_release";
    if(!Event(state.pawn,state.forceFunction,&parameters))return false;
    ++state.forceCalls;if(held){state.pressSubmitted=true;state.primaryHeldRequested=true;}
    else{++state.releaseCalls;state.releaseSubmitted=true;state.primaryHeldRequested=false;}
    state.operation="none";return true;
}
inline bool IsBattle();
inline bool BattleCleanup();
inline bool BattleNPC(void* ps);
inline bool BattleHeadPoint(void* pawn,engine::Vec& point);
inline bool TargetFail(uint32_t error,const char* stage){
    if(IsBattle()){const bool released=BattleCleanup();if(!released)state.battle.releaseUnavailable=true;return Fail(error,stage);}
    // Release through the already validated original input API before exposing
    // a target-test failure. Never leave the development fixture holding fire.
    if(state.primaryHeldRequested&&!ForcePrimary(false))return false;
    return Fail(error,stage);
}
inline void* TargetFunction(TargetMethod method){return state.targetMethods[static_cast<size_t>(method)].object;}
inline bool ResolveTargetMethods(){
    if(!state.targetProfilesVerified){
        for(const auto& profile:TargetProfiles)if(!ProfileMatches(profile)){
            state.profileFailureRva=profile.rva;return TargetFail(4921,"native_target_original_profile_changed");
        }
        state.targetProfilesVerified=true;
    }
    if(state.targetMetadataVerified){
        for(const auto& method:state.targetMethods)if(Resolve(method.identity)!=method.object)
            return TargetFail(4922,"native_target_original_method_weak_identity_changed");
        if(Resolve(state.targetMathId)!=state.targetMathDefault||Resolve(state.targetSystemId)!=state.targetSystemDefault)
            return TargetFail(4923,"native_target_original_query_default_identity_changed");
        return true;
    }
    struct Method {const wchar_t* path;uint16_t bytes;uint32_t flags,exec;};
    static constexpr Method methods[]{
        {L"/Script/Engine.Controller.GetPlayerViewPoint",48,0x54c20402,0x42ab2c0},
        {L"/Script/Engine.Actor.K2_GetActorLocation",24,0x54820403,0x3ef1cd0},
        {L"/Script/ShooterGame.ShooterCharacter.IsAlive",1,0x54020401,0x6c62390},
        {L"/Script/Engine.KismetMathLibrary.FindLookAtRotation",72,0x14c22403,0x454fbb0},
        {L"/Script/Engine.Controller.SetControlRotation",24,0x04c20402,0x42abfd0},
        {L"/Script/Engine.Controller.GetControlRotation",24,0x54820402,0x42ac0d0},
        // Generated storage376 includes seven trailing bytes. Runtime logical
        // ParmsSize369 ends immediately after ReturnValue@368.
        {L"/Script/Engine.KismetSystemLibrary.LineTraceSingle",0x171,0x04c22403,0x45c3a90},
    };
    static_assert(sizeof(methods)/sizeof(methods[0])==static_cast<size_t>(TargetMethod::Count));
    for(size_t i=0;i<state.targetMethods.size();++i){auto& method=state.targetMethods[i];const auto& expected=methods[i];
        if(!Function(method.object,expected.path,expected.bytes,expected.flags,expected.exec))return false;
        if(!Identity(method.object,method.identity))return TargetFail(4924,"native_target_original_method_identity_unavailable");
    }
    state.targetMathDefault=engine::DefaultObject(engine::Class(0x4526580));
    state.targetSystemDefault=engine::DefaultObject(engine::Class(0x45aa110));
    if(!original_players::IsA(state.targetMathDefault,0x4526580)||!Identity(state.targetMathDefault,state.targetMathId)
       ||!original_players::IsA(state.targetSystemDefault,0x45aa110)||!Identity(state.targetSystemDefault,state.targetSystemId))
        return TargetFail(4925,"native_target_original_query_default_unavailable");
    if(Read<void*>(Read<void**>(state.pc),0x7b8)!=reinterpret_cast<void*>(engine::image+0x781fc90))
        return TargetFail(4926,"native_target_original_control_rotation_setter_changed");
    state.targetMetadataVerified=true;return true;
}
inline bool TargetRoster(Array& roster,void*& gs){
    gs=Read<void*>(state.world,0x178);roster=Read<Array>(gs,0x480);
    Weak gsId{};
    return original_players::IsA(gs,original_players::ShooterGameStateClass)&&Identity(gs,gsId)
        &&roster.count>=0&&roster.count<=64&&roster.capacity>=roster.count&&roster.capacity<=256
        &&(!roster.count||original_players::Readable(roster.data,static_cast<size_t>(roster.count)*8));
}
inline bool JettTargetEnemy(void* pawn); // Defined in the explicit Jett fixture below.
inline bool JettTargetHeadPoint(void* pawn,engine::Vec& point);
inline bool TargetBody(void* ps,void* pawn,Weak& psId,Weak& pawnId,engine::Vec& point,bool& alive){
    alive=false;
    if(ps==state.ps||pawn==state.pawn||!original_players::IsA(ps,original_players::ShooterStateClass)
       ||!original_players::IsA(pawn,original_players::ShooterCharacterClass)||Read<uint8_t>(pawn,0x224)!=1
       ||!Identity(ps,psId)||!Identity(pawn,pawnId)||Read<void*>(ps,0xbc8)!=pawn||Read<void*>(pawn,0x488)!=ps)return false;
    // A simulated proxy legitimately has no replicated remote controller.
    // A non-null possessed-character reference must still match this body.
    const auto possessed=Read<void*>(ps,0xbd0);if(possessed&&possessed!=pawn)return false;
    const auto table=Read<void**>(pawn);
    if(Read<void*>(table,0x1b8)!=reinterpret_cast<void*>(engine::image+0x3ed83b0)
       ||engine::function<void*(*)(void*)>(0x3ed83b0)(pawn)!=state.world)return false;
    if(Read<void*>(table,0x830)!=reinterpret_cast<void*>(engine::image+0x48548e0))return false;
    if(engine::function<bool(*)(void*)>(0x48548e0)(pawn)){++state.targetLocalRejected;return false;}
    uint8_t value=0;
    if(!Event(pawn,TargetFunction(TargetMethod::Alive),&value)
       ||!Event(pawn,TargetFunction(TargetMethod::Location),&point))return false;
    alive=value!=0;
    if(state.marker.scenario==uint32_t(Scenario::JettXTarget)){
        auto& h=state.jettHead;h.pointValid=false;h.rootObserved=original_players::Finite(point);h.rootPoint=h.rootObserved?point:engine::Vec{};
        if(!JettTargetEnemy(pawn)||(alive&&!JettTargetHeadPoint(pawn,point)))return false;
    }
    if(IsBattle()&&(!BattleNPC(ps)||(alive&&!BattleHeadPoint(pawn,point))))return false;
    return original_players::Finite(point);
}
inline bool TargetView(){
    struct View {engine::Vec location{},rotation{};} view;
    static_assert(sizeof(View)==48);
    if(!Event(state.pc,TargetFunction(TargetMethod::ViewPoint),&view))return false;
    if(!original_players::Finite(view.location)||!original_players::Finite(view.rotation))
        return TargetFail(4927,"native_target_original_player_viewpoint_nonfinite");
    state.targetView=view.location;return true;
}
inline bool TargetWorldClear(void* pawn,const engine::Vec& point){
    const double dx=point.x-state.targetView.x,dy=point.y-state.targetView.y,dz=point.z-state.targetView.z;
    const double length=std::sqrt(dx*dx+dy*dy+dz*dz);
    if(state.marker.scenario==uint32_t(Scenario::JettXTarget)){
        auto& h=state.jettHead;h.losAttempted=true;h.losReturned=h.losBlocking=h.losClear=h.losDistanceFinite=false;
        h.losReturnValue=255;h.losDistance=0;h.losLength=std::isfinite(length)?length:0;
        h.losStart=state.targetView;h.losEnd=point;h.losPawn=pawn;Identity(pawn,h.losPawnId);
        h.losAtTick=state.ticks;h.hitBytes.fill(0);h.hitActor={};h.hitComponent={};
    }
    if(!std::isfinite(length)||length<25.||length>100000.)return false;
    std::array<void*,4> ignored{};int32_t count=0;
    const auto add=[&](void* actor){if(!actor)return;for(int32_t n=0;n<count;++n)if(ignored[static_cast<size_t>(n)]==actor)return;
        if(static_cast<size_t>(count)<ignored.size())ignored[static_cast<size_t>(count++)]=actor;};
    add(state.pawn);add(pawn);
    for(auto wearer:{state.pawn,pawn}){
        auto inventory=Read<void*>(wearer,0xc08),item=Read<void*>(inventory,0x280);Weak identity{};
        if(original_players::IsA(item,0x645c660)&&Identity(item,identity)&&Read<void*>(item,0x4a8)==wearer)add(item);
    }
    const Array actors{ignored.data(),count,count};
    alignas(16) uint8_t parameters[0x178]{};
    std::memcpy(parameters,&state.world,8);std::memcpy(parameters+8,&state.targetView,24);
    std::memcpy(parameters+0x20,&point,24);std::memcpy(parameters+0x40,&actors,sizeof(actors));
    parameters[0x39]=1;parameters[0x148]=1;++state.targetTraceQueries;state.lastObstructionDistance=-1;
    if(state.marker.scenario==uint32_t(Scenario::JettXTarget))++state.jettHead.losQueries;
    if(!Event(state.targetSystemDefault,TargetFunction(TargetMethod::Trace),parameters))return false;
    if(state.marker.scenario==uint32_t(Scenario::JettXTarget)){
        auto& h=state.jettHead;h.losReturned=true;h.losReturnValue=parameters[0x170];h.losBlocking=(parameters[0x105]&1)!=0;
        const auto distance=Read<float>(parameters,0x60);h.losDistanceFinite=std::isfinite(distance);
        h.losDistance=h.losDistanceFinite?distance:0;
        // Original LineTraceSingle.OutHit@58 is240B. Preserve the complete
        // returned native record; these Weak fields are only POD observations.
        std::memcpy(h.hitBytes.data(),parameters+0x58,h.hitBytes.size());
        h.hitActor=Read<Weak>(parameters,0x58+0xc8);h.hitComponent=Read<Weak>(parameters,0x58+0xd0);
        h.losClear=!(parameters[0x170]&&h.losBlocking)||(h.losDistanceFinite&&static_cast<double>(distance)>=length-1.);
    }
    // This is the same original scene-obstruction query/channel as authority
    // combat. Both characters are excluded here, so this only establishes a
    // clear world segment; it never synthesizes a character hit or damage.
    if(parameters[0x170]&&(parameters[0x105]&1)){
        const float obstruction=Read<float>(parameters,0x60);state.lastObstructionDistance=obstruction;
        if(!std::isfinite(obstruction)||obstruction<0)return false;
        if(static_cast<double>(obstruction)<length-1.){++state.targetBlocked;return false;}
    }
    return true;
}
enum class AimResult:uint8_t {Waiting,Ready,Dead,Failure};
inline void ClearTarget(){
    if(state.targetSelected)++state.targetRetirements;
    state.targetSelected=false;state.targetVisible=false;state.targetAlive=false;state.targetDeadObserved=false;
    state.targetPawn=state.targetPS=state.targetGS=nullptr;
    state.targetPawnId={};state.targetPSId={};state.targetGSId={};
}
inline double RotationDistance(double a,double b){
    double value=std::fmod(a-b+180.,360.);if(value<0)value+=360.;return std::abs(value-180.);
}
inline AimResult AimTarget(){
    if(state.lastTargetAimSeconds>=0&&state.elapsedSeconds<state.nextTargetAimSeconds)
        return state.targetVisible?AimResult::Ready:AimResult::Waiting;
    state.nextTargetAimSeconds=state.elapsedSeconds+(IsBattle()?.04:TargetAimInterval);state.lastTargetAimSeconds=state.elapsedSeconds;
    if(!ResolveTargetMethods()||!TargetView())return AimResult::Failure;
    Array roster{};void* gs=nullptr;
    if(!TargetRoster(roster,gs)){state.targetVisible=false;return AimResult::Waiting;}
    bool selectedPresent=false;
    for(int32_t n=0;n<roster.count;++n)if(Read<void*>(roster.data,static_cast<size_t>(n)*8)==state.targetPS)selectedPresent=true;
    if(state.targetSelected){
        Weak psId{},pawnId{};engine::Vec point{};bool alive=false;
        const bool valid=selectedPresent&&gs==state.targetGS&&Resolve(state.targetGSId)==gs
            &&Resolve(state.targetPSId)==state.targetPS&&Resolve(state.targetPawnId)==state.targetPawn
            &&TargetBody(state.targetPS,state.targetPawn,psId,pawnId,point,alive)
            &&SameWeak(psId,state.targetPSId)&&SameWeak(pawnId,state.targetPawnId);
        if(valid&&!alive){state.targetAlive=false;state.targetVisible=false;state.targetDeadObserved=true;return AimResult::Dead;}
        if(!valid){ClearTarget();if(state.primaryHeldRequested)return AimResult::Waiting;}
        else{state.targetAlive=true;state.targetPoint=point;state.targetVisible=TargetWorldClear(state.targetPawn,point);
            // Do not switch bodies inside a burst, including when a doorway
            // becomes obstructed. Release first, then rescan during cooldown.
            if(!state.targetVisible&&state.primaryHeldRequested)return AimResult::Waiting;
        }
    }
    if(!state.targetSelected||!state.targetVisible){
        ++state.targetScans;void *bestPawn=nullptr,*bestPS=nullptr;Weak bestPawnId{},bestPSId{};
        engine::Vec bestPoint{};double bestSquared=1e20;
        for(int32_t n=0;n<roster.count;++n){auto ps=Read<void*>(roster.data,static_cast<size_t>(n)*8);
            auto pawn=Read<void*>(ps,0xbc8);if(ps==state.ps||pawn==state.pawn)continue;
            ++state.targetCandidates;Weak psId{},pawnId{};engine::Vec point{};bool alive=false;
            if(!TargetBody(ps,pawn,psId,pawnId,point,alive)||!alive){++state.targetInvalid;continue;}
            const double dx=point.x-state.targetView.x,dy=point.y-state.targetView.y,dz=point.z-state.targetView.z;
            const double squared=dx*dx+dy*dy+dz*dz;
            if(squared>=bestSquared||!TargetWorldClear(pawn,point))continue;
            bestPawn=pawn;bestPS=ps;bestPawnId=pawnId;bestPSId=psId;bestPoint=point;bestSquared=squared;
        }
        if(!bestPawn){state.targetVisible=false;return AimResult::Waiting;}
        if(state.targetPawn!=bestPawn)ClearTarget();
        state.targetPawn=bestPawn;state.targetPS=bestPS;state.targetGS=gs;
        state.targetPawnId=bestPawnId;state.targetPSId=bestPSId;state.targetPoint=bestPoint;
        if(!Identity(gs,state.targetGSId))return AimResult::Waiting;
        state.targetSelected=true;state.targetVisible=true;state.targetAlive=true;
    }
    struct LookAt {engine::Vec start{},target{},result{};} look{state.targetView,state.targetPoint,{}};
    static_assert(sizeof(LookAt)==72);
    if(!Event(state.targetMathDefault,TargetFunction(TargetMethod::LookAt),&look))return AimResult::Failure;
    if(!original_players::Finite(look.result)){
        TargetFail(4928,"native_target_original_look_at_nonfinite");return AimResult::Failure;
    }
    state.targetRequestedRotation=look.result;
    if(!Event(state.pc,TargetFunction(TargetMethod::SetRotation),&look.result))return AimResult::Failure;
    ++state.targetAimCalls;
    if(!Event(state.pc,TargetFunction(TargetMethod::GetRotation),&state.targetControlReadback))return AimResult::Failure;
    if(!original_players::Finite(state.targetControlReadback)
       ||RotationDistance(state.targetControlReadback.x,look.result.x)>.1
       ||RotationDistance(state.targetControlReadback.y,look.result.y)>.1){state.targetVisible=false;return AimResult::Waiting;}
    ++state.targetAimReadbacks;state.targetEverVisible=true;
    const double dx=state.targetPoint.x-state.targetView.x,dy=state.targetPoint.y-state.targetView.y,dz=state.targetPoint.z-state.targetView.z;
    state.targetDistance=std::sqrt(dx*dx+dy*dy+dz*dz);return AimResult::Ready;
}
// ffa-battle only: native queries and ordinary input edges. No authority event,
// actor/health/resource/clock/score write is permitted in this fixture.
inline bool IsBattle(){return state.marker.scenario==uint32_t(Scenario::FfaBattle);}
inline bool BattleSameName(const BattleName& a,const BattleName& b){return std::memcmp(&a,&b,sizeof(a))==0;}
inline bool BattleLive(const Bound& b){return b.object&&Resolve(b.identity)==b.object;}
inline bool BattleBound(void* object,Bound& out){out.object=object;return Identity(object,out.identity);}
// This session proof is independent of a current Pawn. A dead Pawn can lose
// its owning Controller while the actual local PC/PS/connection remain alive.
// It permits observation only; original input still requires the full current
// owner/action proof below. No Weak failure is interpreted as a death.
inline bool BattlePersistentSession(void* world,void* driver){
    const auto& b=state.battle;if(!b.sessionBound||GetCurrentThreadId()!=original_players::gameThread)return false;
    for(const auto& object:b.session)if(!BattleLive(object))return false;
    auto pc=b.session[1].object,ps=b.session[2].object,player=b.session[3].object;
    auto gi=b.session[4].object,connection=b.session[6].object;
    if(world!=b.session[0].object||driver!=b.session[5].object
       ||!original_players::IsA(pc,original_players::ShooterControllerClass)||Read<uint8_t>(pc,0x224)!=2
       ||!original_players::IsA(ps,original_players::ShooterStateClass)
       ||Read<void*>(world,0x1d8)!=gi||Read<void*>(world,0x40)!=driver||Read<void*>(world,0x170)
       ||Read<void*>(driver,0x178)!=world||Read<void*>(driver,0xc0)!=connection
       ||Read<void*>(connection,0x60)!=driver||Read<int32_t>(connection,0x13c)!=3
       ||Read<void*>(connection,0x38)!=pc||Read<void*>(connection,0xa0)!=pc
       ||Read<void*>(pc,0x510)!=player||Read<void*>(player,0x38)!=pc
       ||Read<void*>(pc,0x470)!=ps||Read<void*>(ps,0x210)!=pc)return false;
    const auto locals=Read<Array>(gi,0x40);
    return locals.count==1&&locals.capacity>=1&&locals.capacity<=32&&locals.data
        &&original_players::Readable(locals.data,8)&&Read<void*>(locals.data)==player;
}
inline bool BattleActionContext(){
    const auto& b=state.battle;
    return state.bound&&BattlePersistentSession(state.world,state.driver)
        &&state.pc==b.session[1].object&&state.ps==b.session[2].object
        &&BattleLive(state.identities[3])&&state.identities[3].object==state.pawn
        &&original_players::IsA(state.pawn,original_players::ShooterCharacterClass)
        &&Read<uint8_t>(state.pawn,0x224)==2&&Read<void*>(state.pc,0x4b0)==state.pawn
        &&Read<void*>(state.pawn,0x4a0)==state.pc&&Read<void*>(state.pawn,0x488)==state.ps
        &&Read<void*>(Read<void**>(state.pawn),0x830)==reinterpret_cast<void*>(engine::image+0x48548e0)
        &&engine::function<bool(*)(void*)>(0x48548e0)(state.pawn)
        &&BattleLive(state.identities[8])&&BattleLive(state.identities[9])
        &&Read<void*>(state.pawn,0xba0)==state.identities[9].object
        &&Read<void*>(state.identities[8].object,0xc8)==state.pawn
        &&Read<void*>(state.identities[8].object,0x170)==state.pawn
        &&Read<void*>(state.identities[9].object,0xc8)==state.pawn
        &&Read<void*>(state.identities[9].object,0x148)==state.pawn
        &&BattleLive(b.forceBinding)&&b.forceBinding.object==state.forceFunction
        &&Read<uint16_t>(state.forceFunction,0xbe)==2
        &&Read<void*>(state.forceFunction,0xe0)==reinterpret_cast<void*>(engine::image+0x6c6b8a0);
}
inline bool BattleForceReload(bool held){
    if(!BattleActionContext()){state.battle.releaseUnavailable=true;return false;}
    struct Params{uint8_t input=2,value=0;} p{2,uint8_t(held?1:0)};
    state.operation=held?"original_force_reload_press":"original_force_reload_release";
    if(!Event(state.pawn,state.forceFunction,&p))return false;
    if(held){state.battle.reloadHeld=true;++state.battle.reloadPresses;}
    else{state.battle.reloadHeld=false;++state.battle.reloadReleases;}
    state.operation="none";return true;
}
inline bool BattleCleanup(){
    if(!state.primaryHeldRequested&&!state.battle.reloadHeld)return true;
    if(!BattleActionContext()){state.battle.releaseUnavailable=true;return false;}
    bool okay=true;
    if(state.primaryHeldRequested&&!ForcePrimary(false))okay=false;
    if(state.battle.reloadHeld&&!BattleForceReload(false))okay=false;
    return okay;
}
inline bool BattleAbort(uint32_t error,const char* stage){
    const bool released=BattleCleanup();
    if(!released)state.battle.releaseUnavailable=true;
    return Fail(error?error:4940,stage);
}
inline void* BattleProperty(void* type,const wchar_t* name,int32_t bytes,int32_t offset,uint64_t flags=0){
    BattleName key{};engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&key,name,1);
    auto field=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,&key,3);
    return field&&(Read<uint64_t>(Read<void*>(field,8),0x18)&0x8001)==0x8001
        &&Read<int32_t>(field,0x30)==1&&Read<int32_t>(field,0x34)==bytes
        &&Read<int32_t>(field,0x44)==offset&&(Read<uint64_t>(field,0x38)&flags)==flags?field:nullptr;
}
inline bool BattleMethodAt(BattleMethod method,const wchar_t* path,uint16_t bytes,uint32_t flags,uintptr_t exec,uintptr_t owner){
    const auto index=static_cast<size_t>(method);auto& entry=state.battle.methods[index];
    auto& check=state.battle.methodChecks[index];check={};check.checked=true;
    check.expectedBytes=bytes;check.expectedFlags=flags;check.expectedExec=exec;check.ownerGetter=owner;
    check.function=engine::Find(path);check.found=check.function!=nullptr;
    check.actualBytes=Read<uint16_t>(check.function,0xbe);check.actualFlags=Read<uint32_t>(check.function,0xb8);
    const auto native=Read<uintptr_t>(check.function,0xe0);
    check.actualExec=native>=engine::image?native-engine::image:native;
    // UObject's 12-byte FName occupies18..23; Outer is28, not20.
    check.actualOwner=Read<void*>(check.function,0x28);
    check.expectedOwner=check.found?engine::Class(owner):nullptr;
    if(!check.found)check.failureMask|=1U;
    if(check.actualBytes!=bytes)check.failureMask|=2U;
    if(check.actualFlags!=flags)check.failureMask|=4U;
    if(!check.expectedOwner||check.actualOwner!=check.expectedOwner)check.failureMask|=8U;
    if(native!=engine::image+exec)check.failureMask|=16U;
    if(check.failureMask){
        state.failedFunction=path;state.actualFunctionBytes=check.actualBytes;
        state.actualFunctionFlags=check.actualFlags;state.actualFunctionExec=check.actualExec;
        return BattleAbort(4941,"native_ffa_original_query_metadata_changed");
    }
    if(entry.object){
        check.weakVerified=entry.object==check.function&&BattleLive(entry);
        if(!check.weakVerified){check.failureMask|=32U;return BattleAbort(4942,"native_ffa_original_query_weak_identity_changed");}
        return true;
    }
    check.weakVerified=BattleBound(check.function,entry);
    if(!check.weakVerified){check.failureMask|=32U;return BattleAbort(4943,"native_ffa_original_query_identity_unavailable");}
    return true;
}
inline void* BattleFunction(BattleMethod method){return state.battle.methods[static_cast<size_t>(method)].object;}
inline bool BattleMethods(){
    if(!BattleMethodAt(BattleMethod::ForceInput,L"/Script/ShooterGame.ShooterCharacter.ForceInputState",2,0x04020401,0x6c6b8a0,original_players::ShooterCharacterClass)
       ||!BattleMethodAt(BattleMethod::CurrentInput,L"/Script/ShooterGame.ShooterCharacter.GetCurrentInputState",2,0x54020401,0x6c6a460,original_players::ShooterCharacterClass))return false;
    state.forceFunction=BattleFunction(BattleMethod::ForceInput);state.currentInputFunction=BattleFunction(BattleMethod::CurrentInput);
    auto& b=state.battle;
    if(!b.forceBinding.object&&(!BattleBound(state.forceFunction,b.forceBinding)
        ||!BattleBound(state.currentInputFunction,b.inputBinding)))return BattleAbort(4943,"native_ffa_input_method_identity_unavailable");
    if(!BattleLive(b.forceBinding)||!BattleLive(b.inputBinding))return BattleAbort(4942,"native_ffa_input_method_weak_identity_changed");
    if(!BattleMethodAt(BattleMethod::BoneIndex,L"/Script/Engine.SkinnedMeshComponent.GetBoneIndex",16,0x54020403,0x3ea8d10,0x3ea3e80)
       ||!BattleMethodAt(BattleMethod::Ammo,L"/Script/ShooterGame.AmmoComponent.GetAmmo",4,0x54020401,0x637fea0,0x63c7e00)
       ||!BattleMethodAt(BattleMethod::CanReload,L"/Script/ShooterGame.AmmoComponent.CanReload",1,0x54020401,0x63c7f30,0x63c7e00)
       ||!BattleMethodAt(BattleMethod::Active,L"/Script/ShooterGame.ShooterGameState.IsGameplayActive",1,0x54020401,0x6ca0980,original_players::ShooterGameStateClass)
       ||!BattleMethodAt(BattleMethod::Winner,L"/Script/ShooterGame.ShooterGameState.GetWinningTeam",8,0x54020401,0x6ca6c40,original_players::ShooterGameStateClass)
       ||!BattleMethodAt(BattleMethod::Bot,L"/Script/Engine.PlayerState.IsABot",1,0x54020401,0x49451d0,0x42aae70)
       ||!BattleMethodAt(BattleMethod::Component,L"/Script/Engine.Actor.GetComponentByClass",16,0x54020403,0x3ee8590,0x3683900))return false;
    state.metadataVerified=b.metadataVerified=true;return true;
}
inline bool BattleSchema(void* gs){
    auto& b=state.battle;auto type=Read<void*>(gs,0x10);
    auto ffa=engine::Find(L"/Game/GameModes/Deathmatch/DeathmatchGameState.DeathmatchGameState_C");
    if(b.gameState.object)return b.gameState.object==gs&&BattleLive(b.gameState)&&BattleLive(b.gameStateClass)
        &&b.gameStateClass.object==type&&Read<uint8_t>(gs,0x224)==1?true:BattleAbort(4945,"native_ffa_game_state_epoch_changed");
    if(!ffa||!original_players::Derived(type,ffa)||Read<uint8_t>(gs,0x224)!=1)return false;
    constexpr uint64_t net=0x20;
    auto bot=BattleProperty(engine::Class(original_players::ShooterStateClass),L"bIsABot",1,0x46f,net);
    if(!BattleProperty(type,L"MatchState",12,0x4c0,net)||!BattleProperty(type,L"PreviousMatchState",12,0x4cc)
       ||!BattleProperty(type,L"MatchResult",16,0x710,net)||!BattleProperty(type,L"GameInfo",64,0x7d8,net)
       ||!BattleProperty(type,L"NumPointsToWinGame",4,0x7c8,net)
       ||!BattleProperty(engine::Class(original_players::ShooterCharacterClass),L"HeadDamageBoneNames",16,0x11a8)
       ||!bot||(Read<uint64_t>(Read<void*>(bot,8),0x18)&0x28001)!=0x28001||Read<uint16_t>(bot,0x40)!=12
       ||Read<uint8_t>(bot,0x78)!=1||Read<uint8_t>(bot,0x79)!=0
       ||Read<uint8_t>(bot,0x7a)!=8||Read<uint8_t>(bot,0x7b)!=8)
        return BattleAbort(4944,"native_ffa_original_replicated_schema_changed");
    if(!BattleBound(gs,b.gameState)||!BattleBound(type,b.gameStateClass))return BattleAbort(4945,"native_ffa_game_state_identity_unavailable");
    b.botProperty=bot;b.botMetadataVerified=true;
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&b.inProgress,L"InProgress",1);
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&b.postMatch,L"WaitingPostMatch",1);
    return b.inProgress.words[0]&&b.postMatch.words[0];
}
inline bool BattleNPC(void* ps){
    const auto& b=state.battle;
    if(!b.botMetadataVerified||!b.botProperty||Read<int32_t>(b.botProperty,0x44)!=0x46f
       ||Read<uint8_t>(b.botProperty,0x7b)!=8)return false;
    uint8_t native=255;if(!Event(ps,BattleFunction(BattleMethod::Bot),&native)||native>1)return false;
    const bool bot=native!=0&&(Read<uint8_t>(ps,0x46f)&8)!=0;
    if(bot)++state.battle.npcProofs;else ++state.battle.npcProofMissing;
    return bot;
}
inline bool BattleHeadPoint(void* pawn,engine::Vec& point){
    ++state.battle.headQueries;auto mesh=Read<void*>(pawn,0x4f0);Bound meshId{},assetId{};
    if(!original_players::IsA(mesh,0x385f750)||Read<void*>(mesh,0xc8)!=pawn||!BattleBound(mesh,meshId))return false;
    auto asset=Read<void*>(mesh,0x710);if(!asset||!BattleBound(asset,assetId))return false;
    // A follower pose is deliberately pending until its exact world-space
    // mapping is independently proved. Never borrow another mesh's bone index.
    Weak leader=Read<Weak>(mesh,0x718);if(Resolve(leader)){++state.battle.headLeaderPending;return false;}
    struct Names {BattleName* data=nullptr;int32_t count=0,capacity=0;};
    const auto names=Read<Names>(pawn,0x11a8);
    if(!names.data||names.count<1||names.count>32||names.capacity<names.count||names.capacity>128
        ||!original_players::Readable(names.data,static_cast<size_t>(names.count)*sizeof(BattleName)))return false;
    for(int32_t n=0;n<names.count;++n){
        struct Index {BattleName name{};int32_t value=-1;} query{names.data[n],-1};
        static_assert(sizeof(Index)==16);
        if(!query.name.words[0]||!Event(mesh,BattleFunction(BattleMethod::BoneIndex),&query)||query.value<0)continue;
        const auto frame=Read<uint64_t>(mesh,0xd78);const auto buffer=Read<int32_t>(mesh,0x7a8);
        if(!frame||buffer<0||buffer>1)continue;
        const size_t at=0x760+static_cast<size_t>(buffer)*16;
        auto data=Read<void*>(mesh,at);const auto count=Read<int32_t>(mesh,at+8),capacity=Read<int32_t>(mesh,at+12);
        if(!data||count<1||count>4096||capacity<count||capacity>8192||query.value>=count)continue;
        engine::Transform bone{},world{},after{};
        if(!original_players::Read(data,static_cast<size_t>(query.value)*sizeof(bone),bone)
            ||!original_players::Read(mesh,0x300,world))continue;
        const double qx=world.rotation[0],qy=world.rotation[1],qz=world.rotation[2],qw=world.rotation[3];
        const double norm=qx*qx+qy*qy+qz*qz+qw*qw;
        if(!std::isfinite(norm)||std::abs(norm-1.)>.01||!original_players::Finite(world.translation)
            ||!original_players::Finite(world.scale)||!original_players::Finite(bone.translation)
            ||std::abs(world.scale.x)<.01||std::abs(world.scale.y)<.01||std::abs(world.scale.z)<.01
            ||std::abs(world.scale.x)>100||std::abs(world.scale.y)>100||std::abs(world.scale.z)>100)continue;
        const engine::Vec v{bone.translation.x*world.scale.x,bone.translation.y*world.scale.y,bone.translation.z*world.scale.z};
        const engine::Vec t{2.*(qy*v.z-qz*v.y),2.*(qz*v.x-qx*v.z),2.*(qx*v.y-qy*v.x)};
        const engine::Vec result{world.translation.x+v.x+qw*t.x+qy*t.z-qz*t.y,
            world.translation.y+v.y+qw*t.y+qz*t.x-qx*t.z,world.translation.z+v.z+qw*t.z+qx*t.y-qy*t.x};
        if(!original_players::Finite(result)||!BattleLive(meshId)||!BattleLive(assetId)||Read<void*>(mesh,0x710)!=asset
            ||Read<uint64_t>(mesh,0xd78)!=frame||Read<int32_t>(mesh,0x7a8)!=buffer||Read<void*>(mesh,at)!=data
            ||Read<int32_t>(mesh,at+8)!=count||!original_players::Read(mesh,0x300,after)
            ||std::memcmp(&world,&after,sizeof(world))!=0||Resolve(Read<Weak>(mesh,0x718)))continue;
        point=result;state.battle.headMesh=mesh;state.battle.headPoseFrame=frame;state.battle.headBone=query.value;
        ++state.battle.headPointsProved;return true;
    }
    return false;
}
inline bool BattleSession(void* world,void* pc,void* ps){
    auto& b=state.battle;if(!b.sessionBound)return true;
    const std::array<void*,7> objects{world,pc,ps,Read<void*>(pc,0x510),Read<void*>(world,0x1d8),
        Read<void*>(world,0x40),Read<void*>(Read<void*>(world,0x40),0xc0)};
    for(size_t n=0;n<objects.size();++n)if(objects[n]!=b.session[n].object||!BattleLive(b.session[n]))
        return BattleAbort(4946,"native_ffa_stable_session_identity_changed");
    return true;
}
inline void BattleCaptureSession(){
    constexpr size_t indices[]{0,1,2,4,5,6,7};
    for(size_t n=0;n<state.battle.session.size();++n)state.battle.session[n]=state.identities[indices[n]];
    state.battle.sessionBound=true;++state.battle.lifeBindings;
}
inline bool BattleObserveMatch(){
    auto& b=state.battle;auto gs=Read<void*>(state.world,0x178);
    if(!BattleSchema(gs))return !state.error;
    if(Read<int32_t>(gs,0x7c8)!=40)return BattleAbort(4947,"native_ffa_original_kill_limit_not_40");
    BattleInfo info{};if(!original_players::Read(gs,0x7d8,info))return BattleAbort(4948,"native_ffa_original_game_info_unreadable");
    b.activeBeforeQuery=info.active;b.activeQueryResult=b.activeAfterQuery=255;
    // Original IsGameplayActive is a refreshing getter:6CA0980 calls7191300,
    // which can mature a true start/end and change this same GameInfo.active.
    // Compare its returned value with an actual read AFTER that original call,
    // never with a copy taken before its native refresh/delegate publication.
    uint8_t active=255;const bool queried=Event(gs,BattleFunction(BattleMethod::Active),&active);
    b.activeQueryResult=active;
    if(!queried||active>1)return BattleAbort(4948,"native_ffa_original_active_query_invalid");
    if(!original_players::Read(gs,0x7d8,info))return BattleAbort(4948,"native_ffa_original_post_query_game_info_unreadable");
    b.activeAfterQuery=info.active;b.activeQueryAuthStart=info.authStart;b.activeQueryAuthEnd=info.authEnd;
    if(active!=info.active)return BattleAbort(4948,"native_ffa_original_active_readback_mismatch");
    if(b.activeBeforeQuery!=b.activeAfterQuery)++b.activeQueryRefreshes;
    b.nativeActive=active!=0;b.info=info;++b.matchObservations;
    if(info.infiniteAmmo||info.infiniteReserve||info.matchPaused)return BattleAbort(4949,"native_ffa_requires_original_finite_unpaused_ammo_policy");
    const auto match=Read<BattleName>(gs,0x4c0),previous=Read<BattleName>(gs,0x4cc);
    const auto result=Read<BattleResult>(gs,0x710);b.completion=result.completion;b.winningPoints=result.winningPoints;
    if(b.activeObserved&&(!std::isfinite(info.authStart)||info.authStart!=b.authStart))
        return BattleAbort(4950,"native_ffa_match_timeline_restarted");
    if(BattleSameName(match,b.inProgress)&&active){
        if(!std::isfinite(info.authStart)||info.authStart<0)return true;
        if(b.activeObserved&&b.authStart!=info.authStart)return BattleAbort(4950,"native_ffa_match_timeline_restarted");
        b.activeObserved=true;b.authStart=info.authStart;
    }
    if(!b.activeObserved||active||!BattleSameName(match,b.postMatch)||!BattleSameName(previous,b.postMatch)
        ||!std::isfinite(info.authEnd)||info.authEnd<b.authStart)return true;
    const bool completed=result.completion==1&&result.winningTeam&&result.winningPoints>=0;
    const bool draw=result.completion==7&&!result.winningTeam&&result.winningPoints==0;
    void* winner=nullptr;
    if((!completed&&!draw)||!Event(gs,BattleFunction(BattleMethod::Winner),&winner)||winner!=result.winningTeam)return true;
    if(winner){Weak id{};if(!original_players::IsA(winner,0x6849cb0)||!Identity(winner,id))return true;}
    b.endRuleObserved=(completed&&result.winningPoints>=40)||(info.authEnd-b.authStart>=539.98f);
    if(!b.endRuleObserved)return true;
    b.matchEndObserved=true;b.winningTeam=winner;return true;
}
inline bool BattleAmmo(){
    auto& b=state.battle;b.ammo=-1;b.canReload=false;
    if(!state.current||Read<void*>(state.current,0x210)!=state.pawn||Read<void*>(state.current,0x4a8)!=state.pawn
        ||Read<void*>(state.inventory,0x268)!=state.current)return true;
    Bound item{};if(!BattleBound(state.current,item))return true;
    // The weapon has its own native component-lookup virtual. Invoke the
    // original Actor UF, which dispatches through this actual receiver's740.
    //40E6DF0 is the validated character override, not a weapon entry point.
    state.operation="original_ffa_weapon_get_component_by_class";
    struct ComponentQuery {void* type=nullptr;void* result=nullptr;};
    static_assert(sizeof(ComponentQuery)==16&&offsetof(ComponentQuery,result)==8);
    ComponentQuery query{engine::Class(0x63c7e00),nullptr};
    if(!Event(state.current,BattleFunction(BattleMethod::Component),&query))return BattleAbort(state.error,state.stage);
    state.operation="original_ffa_weapon_ammo_component_readback";
    auto ammo=query.result;
    Bound id{};if(!original_players::IsA(ammo,0x63c7e00)||Read<void*>(ammo,0xc8)!=state.current||!BattleBound(ammo,id)){
        state.operation="none";return true;}
    state.operation="original_ffa_ammo_and_reload_queries";
    int32_t value=-1;uint8_t can=255;
    if(!Event(ammo,BattleFunction(BattleMethod::Ammo),&value)||!Event(ammo,BattleFunction(BattleMethod::CanReload),&can)
        ||!BattleLive(id)||!BattleLive(item)||Read<void*>(state.inventory,0x280)!=item.object
        ||Read<void*>(state.inventory,0x268)!=item.object||Read<void*>(item.object,0x4a8)!=state.pawn
        ||can>1||value<0||value>65535)return BattleAbort(4951,"native_ffa_original_ammo_query_invalid");
    b.ammo=value;b.canReload=can!=0;b.ammoComponent=ammo;++b.ammoQueries;
    if(b.minimumAmmo<0||value<b.minimumAmmo)b.minimumAmmo=value;state.operation="none";return true;
}
inline bool BattleQueryReload(){
    struct Params{uint8_t input=2,value=255;} p;
    if(!Event(state.pawn,state.currentInputFunction,&p)||p.value>1)return BattleAbort(4952,"native_ffa_reload_input_readback_invalid");
    state.battle.reloadNow=p.value;++state.battle.reloadQueries;return true;
}
inline void BattleResetLife(){
    ClearTarget();state.lastTargetAimSeconds=-1;state.nextTargetAimSeconds=0;state.readySeconds=state.phaseSeconds=0;
    state.phase=Phase::Warming;state.idleFrames=0;state.idleWeapon=state.predictedMachine=state.predictedState=nullptr;
    state.battle.lifeWaiting=false;state.battle.deathObserved=false;state.battle.deathAtMs=0;
}
// Retiring the fixture's private requested intent is not an engine release.
// ForceInput is permitted only while this exact old Pawn is still the genuine
// local owned Pawn. Once ownership is gone, discard only fixture cache flags.
inline bool BattleRetireOldLifeIntent(){
    auto& b=state.battle;const bool primary=state.primaryHeldRequested,reload=b.reloadHeld;
    if(!primary&&!reload)return true;
    if(BattleActionContext()){
        ++b.oldLifeNativeReleaseBatches;
        if(!BattleCleanup())return false;
        // Separate actual native readback from submitted edges. A submitted
        // release that has not yet been consumed is never called observed.
        if(BattleActionContext()){
            if(!QueryPrimary()||!BattleQueryReload()||state.primaryNow>1)return false;
            if(primary&&state.primaryNow==0){++b.oldLifePrimaryReleaseObserved;state.releaseObserved=true;}
            if(reload&&b.reloadNow==0)++b.oldLifeReloadReleaseObserved;
        }
        return true;
    }
    if(primary)++b.retiredPrimaryIntents;if(reload)++b.retiredReloadIntents;
    state.primaryHeldRequested=false;b.reloadHeld=false;
    ++b.oldLifePrivateIntentRetirements;
    // No ForceInput/QueryPrimary/QueryReload and no releaseSubmitted,
    // releaseObserved, releaseCalls or reloadReleases are changed here.
    return true;
}
inline bool BattleRecordObservedDeath(const Bound& pawn,uint8_t nativeAlive,uint64_t now){
    auto& b=state.battle;
    if(nativeAlive!=0||!BattleLive(pawn)||pawn.object!=state.identities[3].object
       ||pawn.identity.index!=state.identities[3].identity.index||pawn.identity.serial!=state.identities[3].identity.serial)
        return BattleAbort(4955,"native_ffa_death_requires_real_same_life_native_alive_zero");
    if(!b.deathObserved){
        b.deathObserved=b.lifeWaiting=true;b.deathAtMs=now;++b.deathsObserved;
        b.observedDeadPawn=pawn;b.deathQuery=state.targetMethods[static_cast<size_t>(TargetMethod::Alive)];
        b.deathSampleAtMs=now;b.deathAlive=nativeAlive;
        b.deathSamplePcPawn=Read<void*>(b.session[1].object,0x4b0);
        b.deathSampleController=Read<void*>(pawn.object,0x4a0);
        ClearTarget();state.phase=Phase::BattleRespawn;
    }
    return BattleRetireOldLifeIntent();
}
// Called before the bridge's full-current-source early returns. It performs
// passive life/match queries for the already armed/bound FFA fixture only.
inline bool ObserveBoundBattleLife(void* world,void* driver){
    if(!state.initialized||!state.configured||!state.armed||state.finished||!IsBattle()
       ||!state.bound||!state.battle.sessionBound)return !state.error;
    auto& b=state.battle;++b.boundLifeSamples;
    if(!BattlePersistentSession(world,driver))return BattleAbort(4946,"native_ffa_life_observer_stable_session_changed");
    const auto now=GetTickCount64();b.wallElapsedMs=now-b.wallStartMs;
    if(state.stopRequested)return BattleAbort(4912,"native_ffa_explicit_stop_actions_released");
    if(b.wallElapsedMs>=state.marker.totalMs)return BattleAbort(4913,"native_ffa_bounded_wall_timeout_match_unverified");
    if(!ResolveTargetMethods()||!BattleMethods()||!BattleObserveMatch())return false;
    const auto old=state.identities[3];
    if(!b.deathObserved){
        if(!BattleLive(old))return BattleAbort(4954,"native_ffa_old_pawn_expired_before_death_observed");
        auto getWorld=Read<void*(*)(void*)>(Read<void**>(old.object),0x1b8);
        if(!original_players::IsA(old.object,original_players::ShooterCharacterClass)||!getWorld||getWorld(old.object)!=world)
            return BattleAbort(4955,"native_ffa_life_observer_old_pawn_world_or_class_changed");
        uint8_t alive=255;if(!Event(old.object,TargetFunction(TargetMethod::Alive),&alive)||alive>1)
            return BattleAbort(4955,"native_ffa_original_own_life_query_invalid");
        ++b.originalLifeQueries;b.lastLifeAlive=alive;
        if(alive==0){if(!BattleRecordObservedDeath(old,alive,now))return false;}
        else if(!BattleActionContext()){
            ++b.unownedOldLifeSamples;
            if(!BattleRetireOldLifeIntent())return false;
        }
    }else if(!BattleRetireOldLifeIntent())return false;
    if(b.deathObserved&&b.lifeWaiting){
        // Actual native post-match/result proof may arrive while a dead life
        // has no replacement. Its retired private intents are not native
        // release proof; retain the separate counters above.
        if(b.matchEndObserved&&!state.primaryHeldRequested&&!b.reloadHeld){
            b.completedAfterRetiredDeath=true;state.finished=true;
            state.actionsCompleted=state.targetBurstsCompleted>0;state.phase=Phase::Complete;
            state.stage="native_ffa_real_match_end_observed_after_retired_dead_life";return true;
        }
        if(now-b.deathAtMs>=state.marker.waitMs)return BattleAbort(4957,"native_ffa_original_respawn_timeout");
    }
    return true;
}
inline bool TickBattle(void* world,void* pc,void* ps,void* pawn,float deltaSeconds){
    auto& b=state.battle;const auto now=GetTickCount64();if(!b.wallStartMs)b.wallStartMs=now;
    b.wallElapsedMs=now-b.wallStartMs;
    if(GetCurrentThreadId()!=original_players::gameThread)return BattleAbort(4907,"native_ffa_requires_original_game_thread");
    if(state.stopRequested)return BattleAbort(4912,"native_ffa_explicit_stop_actions_released");
    if(b.wallElapsedMs>=state.marker.totalMs)return BattleAbort(4913,"native_ffa_bounded_wall_timeout_match_unverified");
    if(!BattleSession(world,pc,ps)||!BattleMethods())return false;
    if(!b.sessionBound){
        if(!ValidateOwner(world,pc,ps,pawn))return BattleAbort(state.error,state.stage);
        if(!state.bound||state.ownershipPassedMask!=0x1fff){
            if(b.wallElapsedMs>=state.marker.waitMs)return BattleAbort(4911,"native_ffa_actual_role2_owner_timeout");
            return Wait("native_ffa_waiting_actual_role2_socket_owner");}
        BattleCaptureSession();
    }
    if(!ResolveTargetMethods()||!BattleObserveMatch())return BattleAbort(state.error,state.stage);
    if(b.matchEndObserved){if(!BattleCleanup())return BattleAbort(4953,"native_ffa_match_end_release_unavailable");
        if(!b.endReleaseAtMs)b.endReleaseAtMs=now;
        if(!BattleActionContext()||!QueryPrimary()||!BattleQueryReload())return BattleAbort(4953,"native_ffa_match_end_release_readback_unavailable");
        if(state.primaryNow||b.reloadNow){
            if(now-b.endReleaseAtMs>=state.marker.waitMs)return BattleAbort(4953,"native_ffa_match_end_release_not_observed");
            return Wait("native_ffa_real_match_end_waiting_original_released_input");}
        state.finished=true;state.actionsCompleted=state.targetBurstsCompleted>0;state.phase=Phase::Complete;
        state.stage="native_ffa_real_match_end_observed_external_host_validation_required";return true;}
    const auto old=state.identities[3];uint8_t alive=255;
    if(!BattleLive(old)){if(!b.deathObserved)return BattleAbort(4954,"native_ffa_old_pawn_expired_before_death_observed");}
    else if(!Event(old.object,TargetFunction(TargetMethod::Alive),&alive)||alive>1)return BattleAbort(4955,"native_ffa_original_own_life_query_invalid");
    if(alive==0&&!b.deathObserved&&!BattleRecordObservedDeath(old,alive,now))
        return BattleAbort(4956,"native_ffa_dead_life_intent_retirement_unavailable");
    const bool currentIsOld=pawn==old.object&&BattleLive(old);
    if(!currentIsOld||b.lifeWaiting){
        if(!BattleCleanup())return BattleAbort(4956,"native_ffa_life_change_release_unavailable");
        if(!b.deathAtMs)b.deathAtMs=now;
        if(now-b.deathAtMs>=state.marker.waitMs)return BattleAbort(4957,"native_ffa_original_respawn_timeout");
        if(!b.deathObserved||!pawn||currentIsOld)return Wait("native_ffa_waiting_observed_death_then_original_respawn");
        Weak fresh{};if(!Identity(pawn,fresh)||SameWeak(fresh,old.identity))return Wait("native_ffa_waiting_fresh_original_pawn_identity");
        if(!original_players::IsA(pawn,original_players::ShooterCharacterClass)||Read<uint8_t>(pawn,0x224)!=2)
            return Wait("native_ffa_waiting_original_role2_respawn");
        uint8_t freshAlive=255;if(!Event(pawn,TargetFunction(TargetMethod::Alive),&freshAlive)||freshAlive>1)return BattleAbort(4955,"native_ffa_respawn_life_query_invalid");
        if(!freshAlive)return Wait("native_ffa_waiting_live_respawn");
        const bool wasBound=state.bound;state.bound=false;
        if(!ValidateOwner(world,pc,ps,pawn)){state.bound=wasBound;return BattleAbort(state.error,state.stage);}
        if(state.ownershipPassedMask!=0x1fff||!state.bound){state.bound=wasBound;return Wait("native_ffa_waiting_complete_respawn_owner_links");}
        const auto& reboundPawn=state.identities[3];
        if(!BattleLive(reboundPawn)||reboundPawn.object!=pawn
           ||(reboundPawn.identity.index==old.identity.index&&reboundPawn.identity.serial==old.identity.serial))
            return BattleAbort(4964,"native_ffa_respawn_requires_fresh_owned_pawn_weak_identity");
        ++b.lifeBindings;++b.respawnsObserved;BattleResetLife();
    }else if(!ValidateOwner(world,pc,ps,pawn))return BattleAbort(state.error,state.stage);
    if(!b.nativeActive){if(!BattleCleanup())return BattleAbort(4953,"native_ffa_inactive_release_unavailable");
        if(!b.activeObserved&&b.wallElapsedMs>=60000)return BattleAbort(4958,"native_ffa_original_active_start_unverified");
        return Wait("native_ffa_waiting_original_active_or_post_match");}
    if(!QueryPrimary()||!BattleQueryReload()||!BattleAmmo())return BattleAbort(state.error,state.stage);
    if(state.primaryNow>1)return BattleAbort(4952,"native_ffa_primary_input_readback_invalid");
    state.phaseSeconds+=deltaSeconds;
    if(state.phase==Phase::BattleReloadHeld){
        b.reloadHeldObserved=b.reloadHeldObserved||b.reloadNow==1;
        if(state.phaseSeconds<.1)return true;
        if(!BattleForceReload(false))return BattleAbort(4959,"native_ffa_reload_release_unavailable");
        state.phase=Phase::BattleReloadReleased;state.phaseSeconds=0;return true;
    }
    if(state.phase==Phase::BattleReloadReleased){
        if(b.reloadNow){if(state.phaseSeconds*1000>=state.marker.waitMs)return BattleAbort(4960,"native_ffa_reload_release_not_observed");return true;}
        if(!b.reloadHeldObserved)return BattleAbort(4961,"native_ffa_reload_press_not_consumed");
        ++b.reloadEdgesCompleted;state.phase=Phase::BattleReloading;state.phaseSeconds=0;return true;
    }
    if(state.phase==Phase::BattleReloading){
        if(b.ammo>0&&PredictedIdle()){++b.reloadMagazineObserved;state.phase=Phase::Warming;state.phaseSeconds=state.readySeconds=0;return true;}
        if(state.phaseSeconds*1000>=state.marker.waitMs)return BattleAbort(4962,"native_ffa_original_reload_magazine_timeout");return true;
    }
    const auto aim=AimTarget();if(aim==AimResult::Failure)return BattleAbort(state.error,state.stage);
    if(state.phase==Phase::ShootHeld){
        state.primaryHeldObserved=state.primaryHeldObserved||state.primaryNow==1;state.burstHeldObserved=state.burstHeldObserved||state.primaryNow==1;
        // ForceInputState queues an edge; only the original current-input
        // getter proves consumption. A transient obstruction must not submit
        // release before native sampling has had the existing bounded hold.
        if(!state.burstHeldObserved){
            if(state.phaseSeconds*1000<state.marker.holdMs)return true;
            return BattleAbort(4920,"native_ffa_primary_press_not_consumed");
        }
        if(aim==AimResult::Ready&&b.ammo>0&&state.phaseSeconds*1000<state.marker.holdMs)return true;
        if(!ForcePrimary(false))return BattleAbort(state.error,state.stage);
        state.phase=Phase::ShootReleased;state.phaseSeconds=0;return true;
    }
    if(state.phase==Phase::ShootReleased){
        if(state.primaryNow){if(state.phaseSeconds*1000>=state.marker.waitMs)return BattleAbort(4915,"native_ffa_primary_release_not_observed");return true;}
        if(!state.burstHeldObserved)return BattleAbort(4920,"native_ffa_primary_press_not_consumed");
        ++state.targetBurstsCompleted;state.releaseObserved=true;ClearTarget();state.phase=Phase::TargetCooldown;state.phaseSeconds=0;return true;
    }
    if(state.phase==Phase::TargetCooldown){if(state.phaseSeconds<.15)return true;state.phase=Phase::Warming;state.phaseSeconds=state.readySeconds=0;}
    if(aim==AimResult::Dead){ClearTarget();return true;}
    if(state.phase==Phase::WaitingOwner||state.phase==Phase::Warming){
        state.phase=Phase::Warming;const bool idle=state.primaryNow==0&&b.reloadNow==0&&b.ammo>=0&&PredictedIdle();
        if(idle&&!b.ammo&&b.canReload){
            if(!BattleForceReload(true))return BattleAbort(4959,"native_ffa_reload_press_unavailable");
            b.reloadHeldObserved=false;state.phase=Phase::BattleReloadHeld;state.phaseSeconds=state.readySeconds=0;return true;}
        const bool ready=idle&&b.ammo>0&&aim==AimResult::Ready;
        state.readySeconds=ready?state.readySeconds+deltaSeconds:0;
        if(!ready||state.readySeconds*1000<state.marker.warmupMs)return Wait("native_ffa_waiting_proven_live_npc_head_and_original_idle");
        if(!BattleActionContext()||!ForcePrimary(true))return BattleAbort(4963,"native_ffa_original_primary_press_unavailable");
        ++state.targetBurstsStarted;state.burstHeldObserved=false;state.phase=Phase::ShootHeld;state.phaseSeconds=0;
    }
    return true;
}

inline bool Complete();
inline bool TickTarget(float deltaSeconds){
    if(!QueryPrimary())return false;
    const auto aim=AimTarget();
    if(aim==AimResult::Failure){if(state.primaryHeldRequested)ForcePrimary(false);return false;}
    if(state.phase==Phase::ShootHeld){
        state.primaryHeldObserved=state.primaryHeldObserved||state.primaryNow==1;
        state.burstHeldObserved=state.burstHeldObserved||state.primaryNow==1;
        if(aim==AimResult::Ready&&state.phaseSeconds*1000<state.marker.holdMs)return true;
        if(!ForcePrimary(false))return false;state.phase=Phase::ShootReleased;state.phaseSeconds=0;
        return Wait("native_target_release_waiting_original_consumption");
    }
    if(state.phase==Phase::ShootReleased){
        if(state.primaryNow!=0){if(state.phaseSeconds*1000>=state.marker.waitMs)
                return TargetFail(4915,"native_target_release_not_observed");return true;}
        state.releaseObserved=true;++state.targetBurstsCompleted;
        if(!state.burstHeldObserved)return TargetFail(4920,"native_target_press_not_consumed_release_completed");
        if(state.targetDeadObserved||state.targetBurstsCompleted>=MaximumTargetBursts)return Complete();
        state.phase=Phase::TargetCooldown;state.phaseSeconds=0;state.readySeconds=0;
        return Wait("native_target_original_burst_released_cooldown");
    }
    if(state.phase==Phase::TargetCooldown){
        if(state.targetDeadObserved&&state.targetBurstsStarted)return Complete();
        if(state.phaseSeconds<TargetCooldownSeconds)return true;
        state.phase=Phase::Warming;state.phaseSeconds=0;state.readySeconds=0;
    }
    if(state.phase==Phase::WaitingOwner||state.phase==Phase::Warming){
        state.phase=Phase::Warming;state.primaryBefore=state.primaryNow;
        if(state.targetDeadObserved&&state.targetBurstsStarted)return Complete();
        const bool inputReady=state.current&&Read<void*>(state.current,0x210)==state.pawn&&Read<void*>(state.current,0x4a8)==state.pawn
            &&Read<void*>(state.inventory,0x268)==state.current&&state.primaryNow==0&&PredictedIdle();
        const bool ready=inputReady&&aim==AimResult::Ready;
        state.readinessPassedMask=(inputReady?1U:0U)|(aim==AimResult::Ready?2U:0U);
        state.readySeconds=ready?state.readySeconds+deltaSeconds:0;
        if(!ready||state.readySeconds*1000<state.marker.warmupMs){
            if(!state.targetBurstsStarted&&state.elapsedSeconds*1000>=state.marker.waitMs)
                return TargetFail(4929,"native_target_visible_original_body_and_input_timeout");
            return Wait("native_target_waiting_visible_role1_body_and_original_idle");
        }
        if(!ForcePrimary(true))return false;
        ++state.targetBurstsStarted;state.burstHeldObserved=false;state.phase=Phase::ShootHeld;state.phaseSeconds=0;
        return Wait("native_target_aimed_primary_held_original_native_ticks");
    }
    return true;
}
inline bool QueryShop(){
    uint8_t opened=0;
    if(!Event(state.pc,state.isOpenFunction,&opened))return false;
    state.shopOpenKnown=true;state.shopOpen=opened!=0;++state.shopOpenQueries;return true;
}
inline bool ShopReady(){
    state.readinessPassedMask=0;
    void* info=nullptr;if(!Event(state.pc,state.getPlayerInfoFunction,&info))return false;
    state.playerInfo=info;state.ui=Read<void*>(state.pc,0xc30);state.coordinator=Read<void*>(state.ui,0x68);
    if(original_players::IsA(info,0x64f4540)&&info==Read<void*>(state.ps,0xa18)&&Read<void*>(info,0x4e8)==state.pc)state.readinessPassedMask|=1;
    if(original_players::IsA(state.coordinator,0x6cd95b0)&&Read<void*>(state.coordinator,0x80)==state.pc)state.readinessPassedMask|=2;
    if(state.readinessPassedMask!=3)return true;
    uint8_t canOpen=0,ownerCanOpen=0;
    if(!Event(state.coordinator,state.canOpenFunction,&canOpen)||!Event(info,state.ownerCanOpenFunction,&ownerCanOpen))return false;
    state.shopEligibilityKnown=true;state.ownerEligibilityKnown=true;state.shopEligible=canOpen!=0;state.ownerEligible=ownerCanOpen!=0;
    // Native ToggleShop's opening chain checks OEPI eligibility. Coordinator C8
    // is a separately ticked UI cache; do not invent a stronger opening gate.
    ++state.eligibilityQueries;if(state.ownerEligible)state.readinessPassedMask|=4;
    return true;
}
inline bool Toggle(bool opening){
    int32_t shopIndex=0;state.operation=opening?"original_toggle_shop_open":"original_toggle_shop_close";
    if(!Event(state.pc,state.toggleFunction,&shopIndex))return false;
    ++state.toggleCalls;if(opening)state.shopOpenSubmitted=true;else state.shopCloseSubmitted=true;
    state.operation="none";return true;
}
inline bool IsPeek();
inline bool PeekCleanup();
inline bool IsSpectatorInput();
inline bool SpectatorInputCleanup();
inline bool Cleanup(){
    if(IsSpectatorInput())return SpectatorInputCleanup();
    if(IsPeek())return PeekCleanup();
    if(state.primaryHeldRequested&&!ForcePrimary(false))return false;
    if(state.shopOpenSubmitted&&!state.shopCloseSubmitted){
        if(!QueryShop())return false;
        if(state.shopOpen&&!Toggle(false))return false;
    }
    return true;
}
inline bool Complete(){state.finished=true;state.actionsCompleted=true;state.phase=Phase::Complete;
    state.stage="native_test_actions_complete_external_server_validation_required";return true;}
#include "local_server_native_skill_primary.inl"
#include "local_server_native_jett.inl"
#include "local_server_native_peek_fixture.inl"
#include "local_server_native_hold_fixture.inl"
#include "local_server_native_shop_prelude.inl"
#include "local_server_native_spectator_input.inl"

inline bool TickImpl(void* world,void* pc,void* ps,void* pawn,float deltaSeconds){
    if(!state.initialized||!state.configured||state.finished)return !state.error;
    if(!engine::image||!original_players::gameThread)return Wait("native_test_waiting_original_engine_thread");
    if(!std::isfinite(deltaSeconds)||deltaSeconds<0)return IsPeek()?PeekAbort(4909,"native_peek_delta_invalid"):IsJettSkill()?JettAbort(4909,"native_test_delta_invalid"):IsSkillPrimary()?SkillAbort(4909,"native_test_delta_invalid"):IsBattle()
        ?BattleAbort(4909,"native_test_delta_invalid"):Fail(4909,"native_test_delta_invalid");
    if(!VerifyProfiles()||!ArmFromOriginalCommandLine())return false;
    if(!state.armed)return true;
    ++state.ticks;state.elapsedSeconds+=deltaSeconds;
    if(IsBattle())return TickBattle(world,pc,ps,pawn,deltaSeconds);
    if(IsSkillPrimary())return TickSkillPrimary(world,pc,ps,pawn,deltaSeconds);
    if(IsJettSkill())return TickJettSkill(world,pc,ps,pawn,deltaSeconds);
    if(IsPeek())return TickPeek(world,pc,ps,pawn,deltaSeconds);
    if(IsMapHold())return TickMapHold(world,pc,ps,pawn);
    if(IsSpectatorInput())return TickSpectatorInput(world,pc,ps,pawn);
    if(IsShopPrelude()&&shopPrelude.ownerBound&&(shopPrelude.pawn.object!=pawn||shopPrelude.pc.object!=pc
       ||Resolve(shopPrelude.pawn.identity)!=pawn||Resolve(shopPrelude.pc.identity)!=pc))
        return ShopPreludeNotObserved("owned_life_changed_before_reproduction");
    if(!ValidateOwner(world,pc,ps,pawn))return false;
    const bool ownerReady=state.bound&&state.ownershipPassedMask==0x1fff;
    if(!ownerReady){
        if(state.primaryHeldRequested||state.shopOpenSubmitted)return Fail(4910,"native_test_owner_lost_action_release_unavailable");
        if(state.elapsedSeconds*1000>=state.marker.waitMs)return Fail(4911,"native_test_actual_role2_owner_timeout");
        return Wait("native_test_waiting_actual_role2_socket_owner");
    }
    if(!ResolveMethods())return false;
    if(IsShopPrelude()){
        if(state.stopRequested||state.elapsedSeconds*1000>=state.marker.totalMs)
            return ShopPreludeNotObserved("explicit_stop_or_fixture_deadline");
        return TickShopPrelude(deltaSeconds);
    }
    if(state.stopRequested){if(!Cleanup())return false;return Fail(4912,"native_test_explicit_stop_released_actions");}
    if(state.elapsedSeconds*1000>=state.marker.totalMs){if(!Cleanup())return false;return Fail(4913,"native_test_total_timeout_actions_released");}
    state.phaseSeconds+=deltaSeconds;
    if(state.marker.scenario==uint32_t(Scenario::ShootTarget))return TickTarget(deltaSeconds);
    if(state.marker.scenario==uint32_t(Scenario::Shoot)){
        if(!QueryPrimary())return false;
        if(state.phase==Phase::WaitingOwner||state.phase==Phase::Warming){
            state.phase=Phase::Warming;state.primaryBefore=state.primaryNow;
            const bool ready=state.current&&Read<void*>(state.current,0x210)==pawn&&Read<void*>(state.current,0x4a8)==pawn
                &&Read<void*>(state.inventory,0x268)==state.current&&state.primaryNow==0&&PredictedIdle();
            state.readinessPassedMask=ready?1:0;
            state.readySeconds=ready?state.readySeconds+deltaSeconds:0;
            if(!ready||state.readySeconds*1000<state.marker.warmupMs){
                if(state.elapsedSeconds*1000>=state.marker.waitMs)return Fail(4914,"native_test_shoot_original_input_not_ready");
                return Wait("native_test_shoot_waiting_original_current_and_released_input");
            }
            if(!ForcePrimary(true))return false;state.phase=Phase::ShootHeld;state.phaseSeconds=0;
            return Wait("native_test_shoot_primary_held_original_native_ticks");
        }
        if(state.phase==Phase::ShootHeld){
            state.primaryHeldObserved=state.primaryHeldObserved||state.primaryNow==1;
            if(state.phaseSeconds*1000<state.marker.holdMs)return true;
            if(!ForcePrimary(false))return false;state.phase=Phase::ShootReleased;state.phaseSeconds=0;
            return Wait("native_test_shoot_release_waiting_original_consumption");
        }
        if(state.phase==Phase::ShootReleased){
            if(state.primaryNow==0){state.releaseObserved=true;
                if(!state.primaryHeldObserved)return Fail(4920,"native_test_shoot_press_not_consumed_release_completed");
                return Complete();}
            if(state.phaseSeconds*1000>=state.marker.waitMs)return Fail(4915,"native_test_shoot_release_not_observed");
            return true;
        }
    }else{
        if(!QueryShop()||!ShopReady())return false;
        if(state.phase==Phase::WaitingOwner||state.phase==Phase::Warming){
            state.phase=Phase::Warming;
            const bool ready=state.readinessPassedMask==7&&!state.shopOpen;
            state.readySeconds=ready?state.readySeconds+deltaSeconds:0;
            if(!ready||state.readySeconds*1000<state.marker.warmupMs){
                if(state.elapsedSeconds*1000>=state.marker.waitMs)return Fail(4916,"native_test_shop_original_eligibility_or_closed_state_timeout");
                return Wait("native_test_shop_waiting_original_can_open_and_closed_state");
            }
            if(!Toggle(true))return false;state.phase=Phase::ShopOpening;state.phaseSeconds=0;
            return Wait("native_test_shop_original_toggle_waiting_open_readback");
        }
        if(state.phase==Phase::ShopOpening){
            if(state.shopOpen){state.shopOpenObserved=true;state.phase=Phase::ShopHolding;state.phaseSeconds=0;
                return Wait("native_test_shop_open_observed_native_ticks");}
            if(state.phaseSeconds*1000>=state.marker.waitMs)return Fail(4917,"native_test_shop_open_not_observed");
            return true;
        }
        if(state.phase==Phase::ShopHolding){
            if(!state.shopOpen)return Fail(4918,"native_test_shop_closed_before_test_release");
            if(state.phaseSeconds*1000<state.marker.holdMs)return true;
            if(!Toggle(false))return false;state.phase=Phase::ShopClosing;state.phaseSeconds=0;
            return Wait("native_test_shop_original_close_waiting_readback");
        }
        if(state.phase==Phase::ShopClosing){
            if(!state.shopOpen){state.shopClosedObserved=true;return Complete();}
            if(state.phaseSeconds*1000>=state.marker.waitMs)return Fail(4919,"native_test_shop_close_not_observed");
            return true;
        }
    }
    return true;
}
#include "local_server_native_shop_dispatch.inl"

inline void WriteBattleMethodChecks(FILE* file,const BattleState& battle){
    const auto b=[](bool value){return value?"true":"false";};
    std::fputs("\"battle_query_metadata\":{\"outer_field_offset\":40,\"failure_bits\":{\"missing\":1,\"parms_size\":2,\"flags\":4,\"exact_owner\":8,\"exec\":16,\"weak_identity\":32},\"methods\":[",file);
    for(size_t n=0;n<battle.methodChecks.size();++n){const auto& q=battle.methodChecks[n];
        std::fprintf(file,"%s{\"method\":\"%s\",\"checked\":%s,\"found\":%s,\"weak_verified\":%s,\"failure_mask\":%u,\"function\":\"%p\",\"expected_parms_size\":%u,\"actual_parms_size\":%u,\"expected_flags\":%u,\"actual_flags\":%u,\"expected_exec_rva\":%llu,\"actual_exec_rva\":%llu,\"owner_class_getter_rva\":%llu,\"expected_owner\":\"%p\",\"actual_owner\":\"%p\"}",
            n?",":"",BattleMethodName(static_cast<BattleMethod>(n)),b(q.checked),b(q.found),b(q.weakVerified),q.failureMask,q.function,unsigned(q.expectedBytes),unsigned(q.actualBytes),q.expectedFlags,q.actualFlags,
            static_cast<unsigned long long>(q.expectedExec),static_cast<unsigned long long>(q.actualExec),static_cast<unsigned long long>(q.ownerGetter),q.expectedOwner,q.actualOwner);
    }
    std::fputs("]},",file);
}

inline void WriteState(FILE* file){
    if(!file)return;
    if(WriteShopPrelude(file))return;
    if(IsPeek()){WritePeekState(file);return;}
    if(IsMapHold()){WriteMapHoldState(file);return;}
    if(IsSpectatorInput()){WriteSpectatorInputState(file);return;}
    const auto& s=state;const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(file,"{\"schema_version\":1,\"scope\":\"explicit_original_native_api_fixture\",\"stage\":\"%s\",\"operation\":\"%s\",\"error\":%u,\"scenario\":%u,\"nonce\":%llu,\"initialized\":%s,\"configured\":%s,\"cli_matched\":%s,\"armed\":%s,\"profiles_verified\":%s,\"metadata_verified\":%s,\"bound\":%s,\"finished\":%s,\"failed\":%s,\"actions_completed\":%s,\"phase\":%u,\"ticks\":%llu,\"elapsed_seconds\":%.9g,\"phase_seconds\":%.9g,\"ready_seconds\":%.9g",
        s.stage,s.operation,s.error,s.marker.scenario,static_cast<unsigned long long>(s.marker.nonce),b(s.initialized),b(s.configured),b(s.cliMatched),b(s.armed),b(s.profilesVerified),b(s.metadataVerified),b(s.bound),b(s.finished),b(s.failed),b(s.actionsCompleted),unsigned(s.phase),static_cast<unsigned long long>(s.ticks),s.elapsedSeconds,s.phaseSeconds,s.readySeconds);
    std::fprintf(file,",\"predicted_idle_frames\":%llu,\"predicted_state_name\":\"%s\"",
        static_cast<unsigned long long>(s.idleFrames),s.predictedStateName);
    std::fprintf(file,",\"target_test\":{\"scope\":\"original_client_aim_and_input_only\",\"profiles_verified\":%s,\"metadata_verified\":%s,\"selected\":%s,\"visible\":%s,\"ever_visible\":%s,\"alive\":%s,\"dead_observed\":%s,\"maximum_bursts\":%u,\"bursts_started\":%u,\"bursts_completed\":%u,\"scans\":%llu,\"candidates\":%llu,\"invalid_candidates\":%llu,\"blocked_segments\":%llu,\"local_candidates_rejected\":%llu,\"target_retirements\":%llu,\"trace_queries\":%llu,\"original_control_rotation_calls\":%llu,\"rotation_readbacks_matched\":%llu,\"distance\":%.9g,\"last_obstruction_distance\":%.9g,\"pawn\":\"%p\",\"player_state\":\"%p\",\"game_state\":\"%p\",\"pawn_weak\":[%d,%d],\"state_weak\":[%d,%d],\"view\":[%.9g,%.9g,%.9g],\"body_point\":[%.9g,%.9g,%.9g],\"requested_rotation\":[%.9g,%.9g,%.9g],\"control_rotation_readback\":[%.9g,%.9g,%.9g],\"world_trace_is_character_hit_proof\":false,\"damage_or_kill_verified\":false,\"direct_transform_or_life_writes\":0}",
        b(s.targetProfilesVerified),b(s.targetMetadataVerified),b(s.targetSelected),b(s.targetVisible),b(s.targetEverVisible),b(s.targetAlive),b(s.targetDeadObserved),MaximumTargetBursts,s.targetBurstsStarted,s.targetBurstsCompleted,
        static_cast<unsigned long long>(s.targetScans),static_cast<unsigned long long>(s.targetCandidates),static_cast<unsigned long long>(s.targetInvalid),static_cast<unsigned long long>(s.targetBlocked),static_cast<unsigned long long>(s.targetLocalRejected),static_cast<unsigned long long>(s.targetRetirements),static_cast<unsigned long long>(s.targetTraceQueries),static_cast<unsigned long long>(s.targetAimCalls),static_cast<unsigned long long>(s.targetAimReadbacks),
        s.targetDistance,s.lastObstructionDistance,s.targetPawn,s.targetPS,s.targetGS,s.targetPawnId.index,s.targetPawnId.serial,s.targetPSId.index,s.targetPSId.serial,
        s.targetView.x,s.targetView.y,s.targetView.z,s.targetPoint.x,s.targetPoint.y,s.targetPoint.z,s.targetRequestedRotation.x,s.targetRequestedRotation.y,s.targetRequestedRotation.z,s.targetControlReadback.x,s.targetControlReadback.y,s.targetControlReadback.z);
    std::fprintf(file,",\"ownership_passed_mask\":%llu,\"ownership_required_mask\":8191,\"readiness_passed_mask\":%llu,\"expected_thread\":%u,\"actual_thread\":%u,\"native_local\":%s,\"net_mode\":%d,\"pawn_role\":%u,\"pawn_remote_role\":%u,\"pc_role\":%u,\"connection_state\":%d,\"local_players\":%d,\"profile_failure_rva\":%u",
        static_cast<unsigned long long>(s.ownershipPassedMask),static_cast<unsigned long long>(s.readinessPassedMask),s.expectedThread,s.actualThread,b(s.nativeLocal),s.netMode,unsigned(s.pawnRole),unsigned(s.pawnRemoteRole),unsigned(s.pcRole),s.connectionState,s.localPlayers,s.profileFailureRva);
    std::fprintf(file,",\"primary_before\":%u,\"primary_now\":%u,\"press_submitted\":%s,\"held_requested\":%s,\"held_observed\":%s,\"release_submitted\":%s,\"release_observed\":%s,\"force_calls\":%llu,\"release_calls\":%llu,\"input_state_queries\":%llu,\"shop_open_known\":%s,\"shop_open\":%s,\"shop_open_submitted\":%s,\"shop_close_submitted\":%s,\"shop_open_observed\":%s,\"shop_closed_observed\":%s,\"shop_eligible\":%s,\"owner_eligible\":%s,\"toggle_calls\":%llu,\"shop_queries\":%llu,\"eligibility_queries\":%llu",
        unsigned(s.primaryBefore),unsigned(s.primaryNow),b(s.pressSubmitted),b(s.primaryHeldRequested),b(s.primaryHeldObserved),b(s.releaseSubmitted),b(s.releaseObserved),static_cast<unsigned long long>(s.forceCalls),static_cast<unsigned long long>(s.releaseCalls),static_cast<unsigned long long>(s.inputStateQueries),b(s.shopOpenKnown),b(s.shopOpen),b(s.shopOpenSubmitted),b(s.shopCloseSubmitted),b(s.shopOpenObserved),b(s.shopClosedObserved),b(s.shopEligible),b(s.ownerEligible),static_cast<unsigned long long>(s.toggleCalls),static_cast<unsigned long long>(s.shopOpenQueries),static_cast<unsigned long long>(s.eligibilityQueries));
    std::fprintf(file,",\"world\":\"%p\",\"pc\":\"%p\",\"ps\":\"%p\",\"pawn\":\"%p\",\"local_player\":\"%p\",\"driver\":\"%p\",\"connection\":\"%p\",\"input\":\"%p\",\"input_state\":\"%p\",\"inventory\":\"%p\",\"current\":\"%p\",\"player_info\":\"%p\",\"coordinator\":\"%p\",\"actual_function_bytes\":%u,\"actual_function_flags\":%u,\"actual_function_exec_rva\":%llu,\"weak_identities\":[",
        s.world,s.pc,s.ps,s.pawn,s.localPlayer,s.driver,s.connection,s.input,s.inputState,s.inventory,s.current,s.playerInfo,s.coordinator,unsigned(s.actualFunctionBytes),s.actualFunctionFlags,static_cast<unsigned long long>(s.actualFunctionExec));
    for(size_t i=0;i<s.identities.size();++i){if(i)std::fputc(',',file);const auto& id=s.identities[i];
        std::fprintf(file,"{\"object\":\"%p\",\"index\":%d,\"serial\":%d}",id.object,id.identity.index,id.identity.serial);}
    std::fprintf(file,"],\"ffa_battle\":{\"enabled\":%s,\"maximum_wall_ms\":%u,\"wall_elapsed_ms\":%llu,\"session_bound\":%s,\"active_observed\":%s,\"match_end_observed\":%s,\"end_rule_observed\":%s,\"completion\":%u,\"winning_points\":%d,\"life_bindings\":%llu,\"deaths_observed\":%llu,\"respawns_observed\":%llu,\"npc_proofs\":%llu,\"npc_proof_missing\":%llu,\"head_points_proved\":%llu,\"head_leader_pending\":%llu,\"ammo\":%d,\"minimum_ammo\":%d,\"reload_presses\":%llu,\"reload_releases\":%llu,\"reload_edges_completed\":%llu,\"reload_magazine_observed\":%llu,\"release_unavailable\":%s,\"host_result_verified\":false,\"playable_verified\":false},",
        b(IsBattle()),BattleTotalMs,static_cast<unsigned long long>(s.battle.wallElapsedMs),b(s.battle.sessionBound),b(s.battle.activeObserved),b(s.battle.matchEndObserved),b(s.battle.endRuleObserved),unsigned(s.battle.completion),s.battle.winningPoints,
        static_cast<unsigned long long>(s.battle.lifeBindings),static_cast<unsigned long long>(s.battle.deathsObserved),static_cast<unsigned long long>(s.battle.respawnsObserved),static_cast<unsigned long long>(s.battle.npcProofs),static_cast<unsigned long long>(s.battle.npcProofMissing),static_cast<unsigned long long>(s.battle.headPointsProved),static_cast<unsigned long long>(s.battle.headLeaderPending),s.battle.ammo,s.battle.minimumAmmo,
        static_cast<unsigned long long>(s.battle.reloadPresses),static_cast<unsigned long long>(s.battle.reloadReleases),static_cast<unsigned long long>(s.battle.reloadEdgesCompleted),static_cast<unsigned long long>(s.battle.reloadMagazineObserved),b(s.battle.releaseUnavailable));
    std::fprintf(file,"\"ffa_active_readback\":{\"before_getter\":%u,\"getter_result\":%u,\"after_getter\":%u,\"native_refresh_transitions\":%llu,\"auth_start_after_getter\":%.9g,\"auth_end_after_getter\":%.9g,\"strict_post_getter_comparison\":true,\"end_rule_unchanged\":true},",
        unsigned(s.battle.activeBeforeQuery),unsigned(s.battle.activeQueryResult),unsigned(s.battle.activeAfterQuery),static_cast<unsigned long long>(s.battle.activeQueryRefreshes),
        static_cast<double>(s.battle.activeQueryAuthStart),static_cast<double>(s.battle.activeQueryAuthEnd));
    std::fprintf(file,"\"ffa_life_observer\":{\"samples\":%llu,\"original_life_queries\":%llu,\"unowned_old_life_samples\":%llu,\"last_alive\":%u,\"last_death_sample_at_ms\":%llu,\"last_death_alive\":%u,\"last_death_pawn\":\"%p\",\"last_death_weak\":[%d,%d],\"last_death_query\":\"%p\",\"last_death_query_weak\":[%d,%d],\"pc_pawn_at_last_death\":\"%p\",\"controller_at_last_death\":\"%p\",\"native_release_batches\":%llu,\"native_primary_releases_observed\":%llu,\"native_reload_releases_observed\":%llu,\"private_intent_retirements\":%llu,\"retired_primary_intents\":%llu,\"retired_reload_intents\":%llu,\"completed_after_retired_death\":%s,\"weak_expiry_is_death_proof\":false,\"private_retirement_is_native_release_proof\":false},",
        static_cast<unsigned long long>(s.battle.boundLifeSamples),static_cast<unsigned long long>(s.battle.originalLifeQueries),static_cast<unsigned long long>(s.battle.unownedOldLifeSamples),unsigned(s.battle.lastLifeAlive),
        static_cast<unsigned long long>(s.battle.deathSampleAtMs),unsigned(s.battle.deathAlive),s.battle.observedDeadPawn.object,s.battle.observedDeadPawn.identity.index,s.battle.observedDeadPawn.identity.serial,
        s.battle.deathQuery.object,s.battle.deathQuery.identity.index,s.battle.deathQuery.identity.serial,s.battle.deathSamplePcPawn,s.battle.deathSampleController,
        static_cast<unsigned long long>(s.battle.oldLifeNativeReleaseBatches),static_cast<unsigned long long>(s.battle.oldLifePrimaryReleaseObserved),static_cast<unsigned long long>(s.battle.oldLifeReloadReleaseObserved),
        static_cast<unsigned long long>(s.battle.oldLifePrivateIntentRetirements),static_cast<unsigned long long>(s.battle.retiredPrimaryIntents),static_cast<unsigned long long>(s.battle.retiredReloadIntents),b(s.battle.completedAfterRetiredDeath));
    WriteBattleMethodChecks(file,s.battle);
    WriteSkillPrimary(file,s);
    WriteJettSkill(file,s);
    std::fputs("\"passed\":false,\"physical_keyboard_mouse_verified\":false,\"server_effects_verified\":false,\"extra_physics_ticks\":0,\"manual_fire_calls\":0,\"client_transform_role_health_resource_writes\":0}",file);
}
}
