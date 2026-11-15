#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <atomic>
#include <cwchar>
#include <initializer_list>
#include "engine_api.h"
#include "armor_poll_performance.h"
#include "local_server_profiles.h"
#include "local_server_package_paths.h"
#include "local_server_mode_startup.h"
#include "local_server_gameplay.h"
#include "original_fault_observer.h"
#include "local_server_input.h"
#include "local_server_equipment.h"
#include "local_server_abilities.h"
#include "local_server_match.h"
#include "local_server_names.h"
#include "local_server_character_stream.h"
#include "local_server_client.h"
#include "local_server_character_lifecycle.h"
#include "local_server_game_start.h"
#include "local_server_runtime.h"
#include "humanlike_bot_weapon_feedback.h"
#include "local_server_viewtarget.h"
#include "local_server_login.h"
#include "local_server_rpc_policy.h"
#include "local_server_environment.h"
#include "local_server_bootstrap_roster.h"
#include "local_server_store_rules.h"
#include "local_server_native_test.h"
#include "local_server_input_health.h"
#include "local_server_input_warning_compat.h"
#include "local_server_input_lifecycle.h"
#include "local_server_server_predicate.h"
#include "local_server_reflection_probe.h"
#include "local_server_remote_timeline_probe.h"
#include "local_server_remote_render_probe.h"
#include "local_server_dagger_points.h"

// Independent local server/client candidate. The accepted playable trainer and
// the earlier DefaultPawn proof libraries remain separate builds.
struct Context { uint64_t magic,version,pid,image,world,controller,pawn,stage; };
struct Configuration { uint64_t magic;uint32_t version,role,port,reserved;wchar_t directory[1024]; };
struct FString { wchar_t* data=nullptr;int32_t count=0,capacity=0; };
struct FName { uint32_t words[3]{}; };
struct Pair { void* object=nullptr;void* refs=nullptr; };
struct Array { Pair* data=nullptr;int32_t count=0,capacity=0; };
struct alignas(8) FURL { uint8_t bytes[104]{}; };
static_assert(sizeof(Context)==64&&sizeof(Configuration)==2072);
static_assert(sizeof(FString)==16&&sizeof(FName)==12&&sizeof(Array)==16);
static HMODULE module=nullptr;
static Context context{};
static Configuration config{};
static std::wstring directory;
static bool OwnedSessionDirectory(){
    wchar_t library[32768]{},image[32768]{};
    const auto libraryLength=GetModuleFileNameW(module,library,32768),imageLength=GetModuleFileNameW(nullptr,image,32768);
    return libraryLength&&libraryLength<32768&&imageLength&&imageLength<32768
        &&local_server_package_paths::RoleDirectory(std::wstring(library,libraryLength),std::wstring(image,imageLength),directory,config.role);
}
static SRWLOCK fileLock=SRWLOCK_INIT;
static std::atomic<uint64_t> ticks{0};
static std::atomic<unsigned> error{0};
static std::atomic<const char*> stage{"initializing"};
static const char* failureStage="none";
static std::atomic<void*> driver{nullptr};
static void* gameInstance=nullptr;
static void* worldContext=nullptr;
static void* owningEngine=nullptr;
static void* bootstrapPlayerState=nullptr;
static FName driverName{};
static uint64_t startedAt=0;
static std::atomic<bool> actionStarted{false},actionCompleted{false},cleaned{false};
static std::atomic<bool> hostCleanupPending{false};
static bool profilesVerified=false,loopbackCandidates=false,listenReturned=false;
static unsigned candidateCount=0;
static wchar_t bindAddress[128]{},nativeError[512]{};
static uintptr_t exceptionRva=0;
static bool clientFunctionABI=false;
static unsigned clientParameterBytes=0;
static uint64_t lastReplicationAt=0;
static FName actorChannelName{};
static std::atomic<uint64_t> replicationCalls{0},replicationBits{0},channelsCreated{0},levelDeferrals{0};
static std::atomic<void*> remotePawn{nullptr},remoteController{nullptr},clientTickPawn{nullptr};
static void* defaultPawnClass=nullptr;
static void* actorChannelClass=nullptr;
static void* shooterCharacterClass=nullptr;
static engine::Vec clientRemotePosition{};
static std::atomic<unsigned> clientRemoteCount{0};
static uint64_t clientRemoteSampleAt=0;
static bool remoteAIIsLocal=false,remotePawnIsLocal=false;
static uint64_t moveSubmissions=0,lastClientScan=0;
static void* publishedViewPawn=nullptr;
static uint64_t lastViewTargetAt=0;
struct ClientPoseSample {
    bool observed=false;void* pawn=nullptr;uint64_t samples=0,sampledAt=0;
    engine::Vec position{},velocity{};
};
static ClientPoseSample clientPose{};
static SRWLOCK clientPoseLock=SRWLOCK_INIT;
static uint64_t lastClientPoseAt=0;
struct NativeWorldIdentity {
    void *world=nullptr,*package=nullptr;uint64_t sampledAt=0;
    char name[128]{},packageName[128]{};
};
static NativeWorldIdentity nativeWorldIdentity{};
static SRWLOCK nativeWorldIdentityLock=SRWLOCK_INIT;
static uint64_t lastWorldIdentityAt=0;
using TickFn=void(*)(void*,float);
static TickFn originalTick=nullptr;
static void** originalTable=nullptr;
static void** privateTable=nullptr;
using NetTickFn=void(*)(void*,float);
static NetTickFn originalNetTick=nullptr;
static NetTickFn originalNetDispatch=nullptr;
static void* observedNetDriver=nullptr;
static void** observedNetTable=nullptr;
static std::atomic<uint64_t> networkTickCalls{0};
static std::atomic<bool> clientInputReady{false},clientInputRetirementPending{false},clientShutdownRequested{false};
static std::atomic<uint64_t> clientInputSourceMask{0};
static std::atomic<uintptr_t> clientInputSourcePcTable{0};
static std::atomic<int32_t> clientInputSourceWeakFailure{-1};
static void NetTick(void* value,float delta);
static void NetDispatch(void* value,float delta);
static void Fail(unsigned code,const char* message);
static bool CloseClientInputSession();
static bool DrainClientInputLifecycle();
static bool RetireObservedClientInputEpoch(const char* reason);
static void RuntimeStop(){
    // Keep the actual host alive until the owning supervisor has completed the
    // client's postflight and requests original named-driver destruction.
    if(config.role==1){stage="native_host_runtime_stopped_waiting_owned_shutdown";return;}
    clientShutdownRequested=true;clientInputReady=false;
    local_server_native_test::Stop();
    if(!CloseClientInputSession())return;
    local_server_menu_input::Close();
    stage="original_training_user_exit_requested";
}
static bool RuntimeRetire(size_t index,void* pawn,uint32_t){
    return index!=0||!local_server_input::GetState().installed||local_server_input::RetireAuthority(pawn);
}
static bool RuntimeObserver(size_t index,void* pc,void* ps){
    const auto* human=local_server_match::Player(0);
    if(index!=0||!original_players::Thread()||!human||!human->human||human->pc!=pc||human->ps!=ps
       ||!local_server_spectator::Live(pc,human->pcIdentity)||!local_server_spectator::Live(ps,human->psIdentity)
       ||original_players::Value<void*>(pc,0x470)!=ps||original_players::Value<void*>(ps,0x210)!=pc)return false;
    uint8_t native=0;
    const bool known=local_server_spectator::Event(pc,L"/Script/ShooterGame.AresPlayerController.InSpectatorMode",1,&native,0x64eabe0);
    // The existing owned policy remains authoritative across its original
    // same-PC/PS respawn, including the transient native mode1 interval.
    const bool ownedPolicy=local_server_spectator::OwnsAuthorityPolicy()&&local_server_spectator::Context();
    return (known&&native!=0)||ownedPolicy;
}

template<class T> static T Read(const void* pointer,size_t offset=0){
    T value{};if(reinterpret_cast<uintptr_t>(pointer)<0x10000)return value;
    __try { std::memcpy(&value,static_cast<const uint8_t*>(pointer)+offset,sizeof(value)); }
    __except(EXCEPTION_EXECUTE_HANDLER) { value={}; }
    return value;
}
template<class T> static void Put(void* pointer,size_t offset,const T& value){
    std::memcpy(static_cast<uint8_t*>(pointer)+offset,&value,sizeof(value));
}
template<class T> static T Virtual(void* object,size_t slot){
    return reinterpret_cast<T>(Read<void*>(Read<void**>(object),slot));
}
static void Fail(unsigned code,const char* message){error=code;failureStage=message;stage=message;
    if(config.role==1&&driver.load()&&!cleaned.load()&&!actionCompleted.load())hostCleanupPending=true;
}
static void ClientInputFailure(const char* fallback){
    if(error.load())return;
    const auto& warning=local_server_input_warning_compat::GetState();
    const auto& health=local_server_input_health::GetState();
    const auto& ack=local_server_ip_ack_compat::GetState();
    const auto& life=local_server_input_lifecycle::GetState();
    if(warning.error)Fail(warning.error,warning.stage);
    else if(health.error)Fail(health.error,health.stage);
    else if(ack.error)Fail(ack.error,ack.stage);
    else if(life.error)Fail(life.error,life.stage);
    else Fail(85,fallback);
}
static bool RetireClientInputObservers(){
    if(local_server_input_lifecycle::Busy())return false;
    // Keep the old tracker proof through warning -> outer health -> ACK restore.
    if(!local_server_input_warning_compat::Close())return false;
    if(!local_server_input_health::RetireClient())return false;
    return local_server_ip_ack_compat::Close();
}
static local_server_input_lifecycle::Retirement RetireClientInputEpoch(
        const local_server_input_lifecycle::Epoch& e,const char*){
    namespace life=local_server_input_lifecycle;
    namespace ack=local_server_ip_ack_compat;
    namespace warning=local_server_input_warning_compat;
    namespace health=local_server_input_health;
    using Result=life::Retirement;
    clientInputReady=false;clientInputRetirementPending=true;
    if(!life::Thread()){ClientInputFailure("native_client_retirement_requires_original_thread");return Result::Failed;}
    if(life::Busy())return Result::Deferred;
    if(life::NoActiveObservers()){clientInputRetirementPending=false;return Result::Completed;}
    const auto same=[](const life::Weak& a,const life::Weak& b){return original_inventory_slots::SameIdentity(a,b);};
    const auto& a=ack::binding;
    if(!a.active||a.world!=e.world||a.pc!=e.pc||a.pawn!=e.pawn||ack::state.installs!=e.ackInstallation
       ||!same(a.identities[0],e.worldId)||!same(a.identities[1],e.pcId)||!same(a.identities[3],e.pawnId)){
        ClientInputFailure("native_client_retirement_old_ack_epoch_mismatch");return Result::Failed;}
    const auto& w=warning::binding;
    if(w.active&&(w.world!=e.world||w.pc!=e.pc||w.pawn!=e.pawn||w.input!=e.input||!same(w.inputId,e.inputId))){
        ClientInputFailure("native_client_retirement_old_input_epoch_mismatch");return Result::Failed;}
    for(const auto& h:health::bindings)if(h.active&&(h.world!=e.world||h.pc!=e.pc||h.pawn!=e.pawn
       ||h.ps!=a.ps||h.connection!=a.connection||!same(h.pawnIdentity,e.pawnId)||!same(h.pcIdentity,e.pcId)
       ||!same(h.stateIdentity,a.identities[2])||!same(h.connectionIdentity,a.identities[5]))){
        ClientInputFailure("native_client_retirement_old_health_epoch_mismatch");return Result::Failed;}
    if(!RetireClientInputObservers()){
        if(life::Busy())return Result::Deferred;
        ClientInputFailure("native_client_owned_observer_retirement_failed");return Result::Failed;}
    if(!life::NoActiveObservers()){ClientInputFailure("native_client_retirement_missing_inactive_receipt");return Result::Failed;}
    clientInputRetirementPending=false;return Result::Completed;
}
static bool DrainClientInputLifecycle(){
    namespace life=local_server_input_lifecycle;
    const auto& current=life::GetState();
    if(!current.initialized)return !current.error;
    if(current.closed&&clientShutdownRequested.load())return true;
    if(life::Drain()){
        // A source-change/shutdown fallback owns its own private retirement
        // receipt. Drain it here too, before the next original actor tick.
        if(clientInputRetirementPending.load()&&!life::GetState().pending)
            return RetireObservedClientInputEpoch("post_native_boundary_pending_observers");
        return true;
    }
    clientInputReady=false;
    const auto& result=life::GetState();
    if(result.pending&&!result.error&&!result.restartRequired){
        stage="native_client_waiting_old_input_epoch_retirement";return false;}
    ClientInputFailure(result.restartRequired?"native_client_lifecycle_requires_new_owned_pair":result.stage);return false;
}
static bool RetireObservedClientInputEpoch(const char* reason){
    namespace life=local_server_input_lifecycle;
    namespace ack=local_server_ip_ack_compat;
    namespace warning=local_server_input_warning_compat;
    clientInputReady=false;
    if(life::GetState().pending)return DrainClientInputLifecycle();
    if(life::NoActiveObservers()){clientInputRetirementPending=false;return true;}
    const auto& a=ack::binding;
    if(!a.active){ClientInputFailure("native_client_observer_retirement_requires_actual_old_ack_record");return false;}
    life::Epoch old{};old.world=a.world;old.pc=a.pc;old.pawn=a.pawn;
    old.worldId=a.identities[0];old.pcId=a.identities[1];old.pawnId=a.identities[3];old.ackInstallation=ack::state.installs;
    if(warning::binding.active){old.input=warning::binding.input;old.inputId=warning::binding.inputId;}
    // Actual source-change fallback only: no synthetic native lifecycle event.
    // Fence even while Busy, and keep retiring admission shut until Completed.
    clientInputRetirementPending=true;
    if(!warning::FenceNativeEpoch()||!ack::FenceNativeEpoch()){
        ClientInputFailure("native_client_observed_epoch_fence_failed");return false;}
    const auto result=RetireClientInputEpoch(old,reason);
    if(result==life::Retirement::Completed)return true;
    if(result==life::Retirement::Deferred)stage="native_client_waiting_quiet_old_input_epoch";
    return false;
}
static bool CloseClientInputSession(){
    namespace life=local_server_input_lifecycle;
    clientInputReady=false;
    if(life::GetState().closed&&life::NoActiveObservers())return !error.load();
    if(!DrainClientInputLifecycle()&&!error.load())return false;
    if(!RetireObservedClientInputEpoch("client_session_shutdown"))return false;
    // Final session close only, after all three real old observers are retired.
    if(!life::Close()){
        if(life::GetState().error||life::GetState().restartRequired)ClientInputFailure(life::GetState().stage);
        else stage=life::GetState().stage;
        return false;
    }
    return !error.load();
}
struct ClientInputSource {
    void *world=nullptr,*pc=nullptr,*ps=nullptr,*pawn=nullptr,*gameInstance=nullptr,*player=nullptr;
    void *driver=nullptr,*connection=nullptr,*gameState=nullptr,*input=nullptr,*movement=nullptr,*inventory=nullptr;
    std::array<local_server_input_lifecycle::Weak,12> identities{};
};
static bool ReadClientInputSource(void* world,void* actualDriver,ClientInputSource& out){
    namespace life=local_server_input_lifecycle;
    clientInputSourceMask=0;clientInputSourcePcTable=0;clientInputSourceWeakFailure=-1;
    out={};if(!life::Thread())return false;
    out.world=world;out.gameInstance=Read<void*>(world,0x1d8);
    auto locals=Read<void**>(out.gameInstance,0x40);
    const auto count=Read<int32_t>(out.gameInstance,0x48),capacity=Read<int32_t>(out.gameInstance,0x4c);
    if(count!=1||capacity<count||capacity>32||!locals||!original_players::Readable(locals,8))return false;
    uint64_t mask=1;clientInputSourceMask=mask;
    out.player=Read<void*>(locals);out.pc=Read<void*>(out.player,0x38);out.ps=Read<void*>(out.pc,0x470);
    out.pawn=Read<void*>(out.pc,0x4b0);out.driver=Read<void*>(world,0x40);out.connection=Read<void*>(out.driver,0xc0);
    out.gameState=Read<void*>(world,0x178);out.input=Read<void*>(out.pawn,0xba0);
    out.movement=Read<void*>(out.pawn,0x4f8);out.inventory=Read<void*>(out.pawn,0xc08);
    clientInputSourcePcTable=reinterpret_cast<uintptr_t>(Read<void*>(out.pc));
    const auto& game=local_server_gameplay::state;
    const bool gates[]{
        game.clientHeroBound&&game.world==world&&game.clientHero==out.pawn&&out.pawn
            &&out.pc==original_players::activeController&&out.pawn==original_players::activePawn&&out.player==original_players::localPlayer,
        original_players::IsA(out.pc,original_players::ShooterControllerClass)&&Read<uint8_t>(out.pc,0x224)==2
            &&original_players::IsA(out.ps,original_players::ShooterStateClass)&&Read<void*>(out.ps,0x210)==out.pc,
        original_players::IsA(out.pawn,original_players::ShooterCharacterClass)&&Read<uint8_t>(out.pawn,0x224)==2
            &&Read<void*>(out.pawn,0x4a0)==out.pc&&Read<void*>(out.pawn,0x488)==out.ps&&Read<uint8_t>(out.pawn,0xa90)!=0,
        Read<void*>(out.ps,0xbc8)==out.pawn&&(!Read<void*>(out.ps,0xbd0)||Read<void*>(out.ps,0xbd0)==out.pawn),
        Read<void*>(out.pc,0x510)==out.player&&out.driver==actualDriver&&Read<void*>(out.driver,0x178)==world
            &&Read<void*>(out.driver,0x10)==engine::Class(0x5112c90)&&Read<void*>(out.connection,0x60)==out.driver
            &&Read<void*>(out.connection,0x38)==out.pc&&Read<void*>(out.connection,0xa0)==out.pc&&Read<int32_t>(out.connection,0x13c)==3,
        original_players::IsA(out.gameState,original_players::ShooterGameStateClass)
            &&original_players::IsA(out.input,0x6494370)&&Read<void*>(out.input,0xc8)==out.pawn&&Read<void*>(out.input,0x148)==out.pawn,
        original_players::IsA(out.movement,0x6c7d380)&&Read<void*>(out.movement,0xc8)==out.pawn
            &&original_players::IsA(out.inventory,0x6499330)&&Read<void*>(out.inventory,0xc8)==out.pawn&&Read<void*>(out.inventory,0x2f0)==out.pawn
    };
    for(size_t n=0;n<std::size(gates);++n)if(gates[n])mask|=uint64_t(1)<<(n+1);
    clientInputSourceMask=mask;if(mask!=0xff)return false;
    const auto hero=original_content::FindCharacterByClass(Read<void*>(out.pawn,0x10));
    if(!local_server_hero_selection::Ready()||!hero.found
       ||!local_server_hero_selection::SameGuid(hero.id,local_server_hero_selection::SelectionGuid()))return false;
    mask|=uint64_t(1)<<8;clientInputSourceMask=mask;
    void* objects[]{out.world,out.pc,out.ps,out.pawn,out.gameInstance,out.player,out.driver,out.connection,out.gameState,out.input,out.movement,out.inventory};
    for(size_t n=0;n<std::size(objects);++n)if(!life::Capture(objects[n],out.identities[n])){
        clientInputSourceWeakFailure=static_cast<int32_t>(n);return false;}
    clientInputSourceMask=mask|(uint64_t(1)<<9);
    return true;
}
static bool SameClientInputSource(const ClientInputSource& before,const ClientInputSource& after){
    const void* a[]{before.world,before.pc,before.ps,before.pawn,before.gameInstance,before.player,before.driver,
        before.connection,before.gameState,before.input,before.movement,before.inventory};
    const void* b[]{after.world,after.pc,after.ps,after.pawn,after.gameInstance,after.player,after.driver,
        after.connection,after.gameState,after.input,after.movement,after.inventory};
    for(size_t n=0;n<std::size(a);++n)if(a[n]!=b[n]||!original_inventory_slots::SameIdentity(before.identities[n],after.identities[n]))return false;
    return true;
}
static bool ClientInputCanBind(const ClientInputSource& source){
    ClientInputSource actual{};
    if(clientShutdownRequested.load()||clientInputRetirementPending.load()
       ||local_server_ip_ack_compat::binding.retiring||local_server_input_warning_compat::binding.retiring
       ||!ReadClientInputSource(source.world,source.driver,actual)||!SameClientInputSource(source,actual)){
        clientInputReady=false;stage="native_client_waiting_current_replicated_input_source";return false;}
    if(!local_server_input_lifecycle::CanBind()){
        clientInputReady=false;
        const auto& life=local_server_input_lifecycle::GetState();
        if(life.error||life.restartRequired)ClientInputFailure(life.stage);
        else stage="native_client_waiting_lifecycle_input_admission";
        return false;
    }
    return true;
}
static void CopyString(const FString& value,wchar_t* output,size_t capacity){
    output[0]=0;
    if(!value.data||value.count<1||value.count>2048||value.capacity<value.count)return;
    const size_t count=(std::min)(capacity-1,static_cast<size_t>(value.count-1));
    for(size_t i=0;i<count;++i)output[i]=Read<wchar_t>(value.data,i*2);
    output[count]=0;
}
static void JsonString(FILE* f,const wchar_t* text){
    std::fputc('"',f);
    for(const auto* p=text;*p;++p){
        if(*p=='"'||*p=='\\')std::fputc('\\',f);
        std::fputc(*p>=32&&*p<127?static_cast<char>(*p):'?',f);
    }
    std::fputc('"',f);
}
static void JsonUtf8(FILE* f,const char* text){
    std::fputc('"',f);
    for(auto p=reinterpret_cast<const unsigned char*>(text);*p;++p){
        if(*p<32)std::fprintf(f,"\\u%04x",unsigned(*p));
        else{if(*p=='"'||*p=='\\')std::fputc('\\',f);std::fputc(*p,f);}
    }
    std::fputc('"',f);
}
static void ObserveClientPose(void* pawn){
    const auto now=GetTickCount64();if(now-lastClientPoseAt<50)return;lastClientPoseAt=now;
    ClientPoseSample next{};next.pawn=pawn;next.sampledAt=now;
    auto location=engine::Find(L"/Script/Engine.Actor.K2_GetActorLocation");
    auto velocity=engine::Find(L"/Script/Engine.Actor.GetVelocity");
    next.observed=pawn&&Read<uint8_t>(pawn,0x224)==2&&Read<uint16_t>(location,0xbe)==24&&Read<uint16_t>(velocity,0xbe)==24
        &&engine::Event(pawn,L"/Script/Engine.Actor.K2_GetActorLocation",&next.position)
        &&engine::Event(pawn,L"/Script/Engine.Actor.GetVelocity",&next.velocity)
        &&original_players::Finite(next.position)&&original_players::Finite(next.velocity);
    if(!next.observed){next.position={};next.velocity={};}
    AcquireSRWLockExclusive(&clientPoseLock);next.samples=clientPose.samples+1;clientPose=next;ReleaseSRWLockExclusive(&clientPoseLock);
}
static void ObserveWorldIdentity(void* world){
    const auto now=GetTickCount64();if(!world||now-lastWorldIdentityAt<1000)return;lastWorldIdentityAt=now;
    NativeWorldIdentity next{};next.world=world;next.package=Read<void*>(world,0x28);next.sampledAt=now;
    original_weapon::OriginalObjectName(world,next.name);
    original_weapon::OriginalObjectName(next.package,next.packageName);
    AcquireSRWLockExclusive(&nativeWorldIdentityLock);nativeWorldIdentity=next;ReleaseSRWLockExclusive(&nativeWorldIdentityLock);
}
static void SaveState(){
    AcquireSRWLockExclusive(&fileLock);
    local_server_peek_fixture::Drain();
    local_server_hold_fixture::Drain();
    const auto path=directory+L"bridge-state.json",temporary=path+L".tmp";
    FILE* f=nullptr;_wfopen_s(&f,temporary.c_str(),L"wb");
    if(f){
        auto currentWorld=Read<void*>(worldContext,0x2c8);
        auto pending=Read<void*>(worldContext,0x198);
        auto currentDriver=Read<void*>(currentWorld,0x40);
        if(!currentDriver)currentDriver=Read<void*>(pending,0x38);
        if(!currentDriver)currentDriver=driver.load();
        auto connection=Read<void*>(currentDriver,0xc0);
        auto locals=Read<void**>(gameInstance,0x40);
        const int localCount=Read<int32_t>(gameInstance,0x48);
        auto player=localCount==1?Read<void*>(locals):nullptr;
        auto pc=Read<void*>(player,0x38),ps=Read<void*>(pc,0x470),pawn=Read<void*>(pc,0x4b0);
        const bool ownLinks=player&&pc&&Read<void*>(pc,0x510)==player&&Read<void*>(player,0x38)==pc;
        FString pendingError=Read<FString>(pending,0xb8);wchar_t pendingText[512]{};CopyString(pendingError,pendingText,512);
        std::fprintf(f,"{\"schema_version\":1,\"pid\":%u,\"role\":\"%s\",\"stage\":\"%s\",\"error\":%u,\"ticks\":%llu,\"port\":%u,\"profiles_verified\":%s,\"loopback_candidates_verified\":%s,\"binding_candidate_count\":%u,\"listen_returned\":%s,\"action_started\":%s,\"action_completed\":%s,\"cleaned\":%s,\"exception_rva\":%llu",
            GetCurrentProcessId(),config.role==1?"host":"client",stage.load(),error.load(),
            static_cast<unsigned long long>(ticks.load()),config.port,profilesVerified?"true":"false",
            loopbackCandidates?"true":"false",candidateCount,listenReturned?"true":"false",actionStarted?"true":"false",
            actionCompleted?"true":"false",cleaned?"true":"false",static_cast<unsigned long long>(exceptionRva));
        std::fprintf(f,",\"initial_world\":\"%p\",\"game_instance\":\"%p\",\"world_context\":\"%p\",\"current_world\":\"%p\",\"pending_netgame\":\"%p\",\"net_driver\":\"%p\",\"driver_world\":\"%p\",\"driver_notify\":\"%p\",\"server_connection\":\"%p\",\"client_connections\":%d,\"connection_state\":%u,\"owning_actor\":\"%p\",\"connection_driver\":\"%p\",\"package_map\":\"%p\",\"last_receive_time\":%.9g",
            reinterpret_cast<void*>(context.world),gameInstance,worldContext,currentWorld,pending,currentDriver,
            Read<void*>(currentDriver,0x178),Read<void*>(currentDriver,0x260),connection,
            Read<int32_t>(currentDriver,0xd0),static_cast<unsigned>(Read<uint8_t>(connection,0x13c)),
            Read<void*>(connection,0xa0),Read<void*>(connection,0x60),Read<void*>(connection,0x70),Read<double>(connection,0x1e0));
        auto table=reinterpret_cast<uintptr_t>(Read<void*>(currentDriver));
        std::fprintf(f,",\"transport\":\"%s\",\"driver_table_rva\":%llu",config.reserved?"ip":"ares",
            static_cast<unsigned long long>(table>=engine::image?table-engine::image:0));
        std::fprintf(f,",\"replication_calls\":%llu,\"replication_bits\":%llu,\"channels_created\":%llu,\"level_deferrals\":%llu",
            static_cast<unsigned long long>(replicationCalls.load()),static_cast<unsigned long long>(replicationBits.load()),
            static_cast<unsigned long long>(channelsCreated.load()),static_cast<unsigned long long>(levelDeferrals.load()));
        std::fprintf(f,",\"native_replication_frame\":%u",Read<uint32_t>(currentDriver,0x4d8));
        AcquireSRWLockShared(&nativeWorldIdentityLock);const auto worldIdentity=nativeWorldIdentity;ReleaseSRWLockShared(&nativeWorldIdentityLock);
        std::fprintf(f,",\"observed_world_identity\":{\"world\":\"%p\",\"package\":\"%p\",\"sampled_at_ms\":%llu,\"name\":",
            worldIdentity.world,worldIdentity.package,static_cast<unsigned long long>(worldIdentity.sampledAt));
        JsonUtf8(f,worldIdentity.name);std::fputs(",\"package_name\":",f);JsonUtf8(f,worldIdentity.packageName);std::fputc('}',f);
        auto position=Read<engine::Vec>(Read<void*>(remotePawn.load(),0x290),0x170);
        const auto observedAt=GetTickCount64();
        std::fprintf(f,",\"remote_pawn\":\"%p\",\"remote_controller\":\"%p\",\"remote_ai_is_local\":%s,\"remote_pawn_is_local\":%s,\"remote_move_submissions\":%llu,\"server_remote_position\":[%.6f,%.6f,%.6f],\"server_remote_sample_at_ms\":%llu,\"client_tick_pawn\":\"%p\",\"client_remote_count\":%u,\"client_remote_position\":[%.6f,%.6f,%.6f],\"client_remote_sample_at_ms\":%llu",
            remotePawn.load(),remoteController.load(),remoteAIIsLocal?"true":"false",remotePawnIsLocal?"true":"false",
            static_cast<unsigned long long>(moveSubmissions),position.x,position.y,position.z,
            static_cast<unsigned long long>(observedAt),clientTickPawn.load(),clientRemoteCount.load(),
            clientRemotePosition.x,clientRemotePosition.y,clientRemotePosition.z,
            static_cast<unsigned long long>(clientRemoteSampleAt));
        std::fprintf(f,",\"pending_welcome_received\":%s,\"pending_join_path_entered\":%s,\"pending_map_loaded\":%s,\"pending_travel_failed\":%s,\"client_function_abi\":%s,\"client_parameter_bytes\":%u,\"local_player_count\":%d,\"world_authority_game_mode\":\"%p\",\"bind_address\":",
            Read<uint8_t>(pending,0xb0)?"true":"false",Read<uint8_t>(pending,0xb1)?"true":"false",
            Read<uint8_t>(pending,0xb2)?"true":"false",Read<uint8_t>(pending,0xb3)?"true":"false",
            clientFunctionABI?"true":"false",clientParameterBytes,Read<int32_t>(gameInstance,0x48),Read<void*>(currentWorld,0x170));
        JsonString(f,bindAddress);std::fputs(",\"native_error\":",f);JsonString(f,nativeError);
        std::fputs(",\"pending_error\":",f);JsonString(f,pendingText);
        std::fprintf(f,",\"failure_stage\":\"%s\",\"native_join_proven\":false,\"local_player\":\"%p\",\"local_controller\":\"%p\",\"local_player_state\":\"%p\",\"local_pawn\":\"%p\",\"own_local_links_verified\":%s,\"local_controller_role\":%u,\"local_pawn_role\":%u,\"local_controller_is_connection_owner\":%s,\"login_state\":%u,\"expected_control_message\":%u,\"host_connections\":[",
            failureStage,player,pc,ps,pawn,ownLinks?"true":"false",unsigned(Read<uint8_t>(pc,0x224)),
            unsigned(Read<uint8_t>(pawn,0x224)),pc&&Read<void*>(connection,0xa0)==pc?"true":"false",
            unsigned(Read<uint8_t>(connection,0x1d8)),unsigned(Read<uint8_t>(connection,0x1dc)));
        auto connections=Read<void**>(currentDriver,0xc8);const int connectionsCount=Read<int>(currentDriver,0xd0);
        if(config.role==1&&connections&&connectionsCount>0&&connectionsCount<=8)for(int i=0;i<connectionsCount;++i){
            auto peer=Read<void*>(connections,size_t(i)*8),remotePC=Read<void*>(peer,0x38);
            if(i)std::fputc(',',f);
            std::fprintf(f,"{\"connection\":\"%p\",\"state\":%u,\"login_state\":%u,\"controller\":\"%p\",\"player_state\":\"%p\",\"pawn\":\"%p\",\"owner_links_verified\":%s,\"controller_role\":%u,\"pawn_role\":%u}",
                peer,unsigned(Read<uint8_t>(peer,0x13c)),unsigned(Read<uint8_t>(peer,0x1d8)),remotePC,
                Read<void*>(remotePC,0x470),Read<void*>(remotePC,0x4b0),
                remotePC&&Read<void*>(peer,0xa0)==remotePC&&Read<void*>(remotePC,0x510)==peer?"true":"false",
                unsigned(Read<uint8_t>(remotePC,0x224)),unsigned(Read<uint8_t>(Read<void*>(remotePC,0x4b0),0x224)));
        }
        std::fputs("],\"level_collection_drivers\":[",f);
        auto collections=Read<uint8_t*>(currentWorld,0x1a0);const int count=Read<int>(currentWorld,0x1a8);
        if(collections&&count>=0&&count<=16)for(int i=0;i<count;++i){
            if(i)std::fputc(',',f);
            std::fprintf(f,"\"%p\"",Read<void*>(collections,size_t(i)*0x78+0x10));
        }
        const auto& gameplay=local_server_gameplay::state;
        const auto& input=local_server_input::GetState();
        const auto& names=local_server_names::GetState();
        const auto& services=local_server_services::State();
        const auto& clientView=local_server_client::GetState();
        const auto& lifecycle=local_server_character_lifecycle::State();
        std::fprintf(f,"],\"playable_stage\":\"%s\",\"playable_error\":%u,\"local_services_ready\":%s,\"host_shooter_world_ready\":%s,\"authority_hero_spawned\":%s,\"authority_hero_possessed\":%s,\"authority_hero\":\"%p\",\"authority_controller\":\"%p\",\"authority_player_state\":\"%p\",\"local_hero_received\":%s,\"network_tick_calls\":%llu,\"client_world_rebound\":%s,\"client_hero_bound\":%s,\"playable_verified\":false",
            gameplay.stage,gameplay.error,gameplay.servicesReady?"true":"false",gameplay.hostWorldReady?"true":"false",
            gameplay.heroSpawned?"true":"false",gameplay.heroPossessed?"true":"false",gameplay.hero,
            gameplay.networkController,gameplay.networkState,
            original_players::Derived(Read<void*>(pawn,0x10),shooterCharacterClass)?"true":"false",
            static_cast<unsigned long long>(networkTickCalls.load()),gameplay.clientWorldBound?"true":"false",gameplay.clientHeroBound?"true":"false");
        // Append separate input evidence to this native snapshot without
        // interpreting a received hero as a completed playable migration.
        std::fputs(",\"input_failed_rpc\":",f);JsonString(f,input.failedRPCName?input.failedRPCName:L"none");
        std::fprintf(f,",\"lifecycle_stage\":\"%s\",\"lifecycle_error\":%u,\"lifecycle_exception_rva\":%llu,\"native_begin_play_published\":%s,\"game_start_stage\":\"%s\",\"game_start_error\":%u,\"gameplay_active\":%s",
            lifecycle.stage,lifecycle.error,static_cast<unsigned long long>(lifecycle.exceptionRva),lifecycle.nativeBeginPlayPublished?"true":"false",
            local_server_game_start::GetState().stage,local_server_game_start::GetState().error,local_server_game_start::GetState().gameplayActive?"true":"false");
        std::fprintf(f,",\"world_play_flags\":%u,\"world_has_begun_play\":%s,\"current_game_state\":\"%p\"",
            unsigned(Read<uint8_t>(currentWorld,0x156)),(Read<uint8_t>(currentWorld,0x156)&0x80)?"true":"false",Read<void*>(currentWorld,0x178));
        auto currentGameState=Read<void*>(currentWorld,0x178);
        const auto stateArray=Read<void**>(currentGameState,0x480);const auto stateCount=Read<int32_t>(currentGameState,0x488);
        bool bootstrapRegistered=false;
        if(stateArray&&stateCount>=0&&stateCount<=128)for(int32_t i=0;i<stateCount;++i)
            bootstrapRegistered|=Read<void*>(stateArray,size_t(i)*8)==bootstrapPlayerState;
        std::fprintf(f,",\"game_state_player_array_count\":%d,\"bootstrap_player_state_registered\":%s",stateCount,bootstrapRegistered?"true":"false");
        std::fprintf(f,",\"client_pawn_initialized\":%u,\"client_pawn_player_state\":\"%p\",\"client_pawn_controller\":\"%p\",\"client_connection_pc\":\"%p\",\"client_pawn_inventory\":\"%p\",\"client_pawn_input_state\":\"%p\"",
            unsigned(Read<uint8_t>(pawn,0xa90)),Read<void*>(pawn,0x488),Read<void*>(pawn,0x4a0),Read<void*>(connection,0x38),Read<void*>(pawn,0xc08),Read<void*>(pawn,0xba0));
        std::fprintf(f,",\"native_client_restart_submitted\":%s",gameplay.clientRestartSubmitted?"true":"false");
        const auto& characterLinks=local_server_character_lifecycle::CharacterLinksState();
        std::fprintf(f,",\"authority_character_links\":{\"stage\":\"%s\",\"error\":%u,\"spawn_published\":%s,\"possession_published\":%s,\"native_getters_matched\":%s,\"publications\":%u,\"pawn\":\"%p\",\"expected_previous\":\"%p\",\"exception_rva\":%llu}",
            characterLinks.stage,characterLinks.error,characterLinks.spawnPublished?"true":"false",characterLinks.possessionPublished?"true":"false",
            characterLinks.nativeGettersMatched?"true":"false",characterLinks.publications,characterLinks.pawn,characterLinks.expectedPrevious,static_cast<unsigned long long>(characterLinks.exceptionRva));
        std::fprintf(f,",\"client_stage\":\"%s\",\"client_error\":%u,\"client_presentation_initialized\":%s,\"client_hud_ready\":%s,\"client_camera_ready\":%s,\"client_input_ready\":%s",
            clientView.stage,clientView.error,clientView.initialized?"true":"false",clientView.hudReady?"true":"false",clientView.cameraReady?"true":"false",clientView.nativeInputReady?"true":"false");
        std::fprintf(f,",\"local_service_stage\":\"%s\",\"local_service_error\":%u,\"local_service_exception_rva\":%llu,\"name_cache_count\":%u,\"name_cache_readbacks\":%u",
            services.stage,services.error,static_cast<unsigned long long>(services.exceptionRva),services.offlineNameCount,services.nativeNameGetterReadbacks);
        std::fprintf(f,",\"roster_stage\":\"%s\",\"roster_error\":%u,\"roster_ready\":%s,\"roster_generation\":%u,\"roster_count\":%u",
            names.stage,names.error,names.ready?"true":"false",names.appliedGeneration,local_server_names::roster.count);
        std::fputs(",\"humanlike_peek_diagnostics\":",f);std::fputs(local_server_humanlike_peek_diagnostics::Json().c_str(),f);
        std::fputs(",\"peek_fixture\":",f);local_server_peek_fixture::WriteStatus(f);
        std::fputs(",\"map_hold_fixture\":",f);local_server_hold_fixture::WriteStatus(f);
        std::fputs(",\"humanlike_nav_diagnostics\":",f);std::fputs(local_server_humanlike_nav_diagnostics::Json().c_str(),f);
        std::fputs(",\"humanlike_weapon_feedback\":",f);std::fputs(humanlike_bot_weapon_feedback::Json().c_str(),f);
        std::fputs(",\"humanlike_audio_requests\":",f);std::fputs(local_server_humanlike_audio_requests::Json().c_str(),f);
        std::fputs(",\"humanlike_audio_tick\":",f);std::fputs(local_server_humanlike_audio_tick::Json().c_str(),f);
        std::fputs(",\"humanlike_blind_metadata\":",f);std::fputs(local_server_humanlike_blind::MetadataJson().c_str(),f);
        std::fprintf(f,",\"roster_last_win32_error\":%lu,\"roster_last_io_stage\":\"%s\",\"roster_publication_pending\":%s,\"roster_pending_generation\":%u,\"roster_publication_attempts\":%u,\"roster_publication_retries\":%llu,\"roster_publication_exhaustions\":%llu",
            static_cast<unsigned long>(names.lastWin32Error),names.lastIoStage,names.publicationPending?"true":"false",names.pendingGeneration,names.publicationAttempts,
            static_cast<unsigned long long>(names.publicationRetries),static_cast<unsigned long long>(names.publicationExhaustions));
        std::fprintf(f,",\"roster_probe_attempted\":%s,\"roster_probe_win32_error\":%lu,\"roster_probe_stage\":\"%s\"",
            names.lastProbeAttempted?"true":"false",static_cast<unsigned long>(names.lastProbeWin32Error),names.lastProbeStage);
        std::fprintf(f,",\"input_owned_failed_mask\":%llu,\"input_owned_required_mask\":%llu,\"input_owned_install_failed_mask\":%llu,\"input_expected_thread\":%u,\"input_actual_thread\":%u,\"input_pawn_table\":\"%p\",\"input_private_table\":\"%p\",\"input_pawn_role\":%u,\"input_pawn_remote_role\":%u",
            static_cast<unsigned long long>(input.ownedFailedMask),static_cast<unsigned long long>(input.ownedRequiredMask),
            static_cast<unsigned long long>(input.ownedFailedMaskAtInstall),input.gameThread,input.actualThreadId,
            input.actualPawnTable,input.privatePawnTable,unsigned(input.actualPawnLocalRole),unsigned(input.actualPawnRemoteRole));
        std::fprintf(f,",\"input_stage\":\"%s\",\"input_error\":%u,\"input_installed\":%s,\"input_packets\":%llu,\"input_moves\":%llu,\"input_physics\":%llu,\"input_acks\":%llu,\"input_hard_corrections\":%llu,\"input_rpc_expected_bytes\":%u,\"input_rpc_actual_bytes\":%u,\"input_rpc_actual_flags\":%u,\"input_rpc_actual_exec\":%llu",
            input.stage,input.error,input.installed?"true":"false",static_cast<unsigned long long>(input.receivedPackets),
            static_cast<unsigned long long>(input.uniqueMoves),static_cast<unsigned long long>(input.physicsCompleted),
            static_cast<unsigned long long>(input.normalAcks+input.gradualAcks),static_cast<unsigned long long>(input.hardCorrections),
            unsigned(input.expectedRPCBytes),unsigned(input.actualRPCBytes),input.actualRPCFlags,static_cast<unsigned long long>(input.actualRPCExecRva));
        std::fputs(",\"empty_equipment_bootstrap_probe\":",f);local_server_empty_bootstrap_probe::Write(f);
        std::fputs(",\"human_funded_boundary_probe\":",f);local_server_human_funded_boundary_probe::Write(f);
        std::fputs(",\"client_presentation\":",f);local_server_client::WriteState(f);
        std::fputs(",\"remote_render_probe\":",f);local_server_remote_render_probe::Write(f);
        AcquireSRWLockShared(&clientPoseLock);const auto pose=clientPose;ReleaseSRWLockShared(&clientPoseLock);
        std::fprintf(f,",\"client_native_pose\":{\"observed\":%s,\"pawn\":\"%p\",\"samples\":%llu,\"sampled_tick_ms\":%llu,\"position\":[%.9g,%.9g,%.9g],\"velocity\":[%.9g,%.9g,%.9g]}",
            pose.observed?"true":"false",pose.pawn,static_cast<unsigned long long>(pose.samples),static_cast<unsigned long long>(pose.sampledAt),
            pose.position.x,pose.position.y,pose.position.z,pose.velocity.x,pose.velocity.y,pose.velocity.z);
        std::fputs(",\"abilities\":",f);local_server_abilities::WriteState(f);
        std::fputs(",\"hero_loadout\":",f);local_server_hero_abilities::Write(f);
        std::fputs(",\"hero_skill_readback\":",f);std::fputs(local_server_hero_skill_readback::Json().c_str(),f);
        std::fputs(",\"phoenix_c_native_birth\":",f);std::fputs(local_server_phoenix_c_native_observer::Json().c_str(),f);
        std::fputs(",\"phoenix_c_native_result\":",f);std::fputs(local_server_phoenix_c_result_observer::Json().c_str(),f);
        std::fputs(",\"phoenix_c_actual_throw_result\":",f);std::fputs(local_server_phoenix_c_throw_result::Json().c_str(),f);
        std::fputs(",\"phoenix_c_actor_gate\":",f);std::fputs(local_server_phoenix_c_actor_gate::Json().c_str(),f);
        std::fputs(",\"phoenix_c_state_counter\":",f);std::fputs(local_server_phoenix_c_state_counter::Json().c_str(),f);
        std::fputs(",\"phoenix_c_fire_gates\":",f);std::fputs(local_server_phoenix_c_fire_gate_readback::Json().c_str(),f);
        std::fputs(",\"hero_authority\":",f);local_server_hero_authority::Write(f);
        std::fputs(",\"hero_damage\":",f);local_server_ability_damage_integration::Write(f);
        std::fputs(",\"hero_damage_observer\":",f);local_server_ability_damage::Write(f);
        std::fprintf(f,",\"host_cleanup_pending\":%s",hostCleanupPending.load()?"true":"false");
        std::fputs(",\"hero_resources\":",f);local_server_hero_resource_init::Write(f);
        std::fputs(",\"hero_player_state\":",f);local_server_player_state_authority::Write(f);
        std::fputs(",\"hero_ultimate\":",f);local_server_hero_ultimate_authority::Write(f);
        std::fputs(",\"authority_clock\":",f);local_server_authority_clock_probe::Write(f);
        std::fputs(",\"npc_identity\":",f);local_server_npc_identity::Write(f);
        std::fputs(",\"dagger_points\":",f);local_server_dagger_points::Write(f);
        std::fputs(",\"npc_authority\":",f);local_server_runtime::WriteNpcState(f);
        std::fputs(",\"authority_view_target\":",f);local_server_viewtarget::WriteState(f);
        std::fputs(",\"native_rpc_policy\":",f);local_server_rpc_policy::WriteState(f);
        std::fputs(",\"native_integration_test\":",f);local_server_native_test::WriteState(f);
        std::fputs(",\"input_health\":",f);local_server_input_health::WriteCachedState(f);
        std::fputs(",\"ip_ack_compat\":",f);local_server_ip_ack_compat::Write(f);
        std::fputs(",\"input_warning_compat\":",f);local_server_input_warning_compat::Write(f);
        const auto inputLife=local_server_input_lifecycle::Snapshot();
        const bool inputReady=clientInputReady.load()&&!inputLife.pending&&!inputLife.nativeDepth
            &&!inputLife.error&&!inputLife.restartRequired&&!inputLife.closing&&!inputLife.closed;
        std::fprintf(f,",\"input_lifecycle\":{\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"installed\":%s,\"closed\":%s,\"pending_retirement\":%s,\"native_depth\":%u,\"completed\":%llu,\"deferred\":%llu,\"bridge_source_ready\":%s,\"bridge_retirement_pending\":%s,\"shutdown_requested\":%s,\"runtime_client_tick_deferred\":%s,\"runtime_verified\":false}",
            inputLife.stage,inputLife.error,inputLife.initialized?"true":"false",inputLife.installed?"true":"false",inputLife.closed?"true":"false",inputLife.pending?"true":"false",inputLife.nativeDepth,
            inputLife.completed,inputLife.deferred,inputReady?"true":"false",clientInputRetirementPending.load()?"true":"false",clientShutdownRequested.load()?"true":"false",inputReady?"false":"true");
        std::fprintf(f,",\"client_input_source\":{\"passed_mask\":%llu,\"required_mask\":1023,\"first_failed_weak_index\":%d,\"actual_pc_table\":\"0x%llx\",\"scope\":\"cached_game_thread_source_admission\",\"gate_names\":[\"one_local_player\",\"actual_polled_hero\",\"client_pc_ps\",\"owned_role2_pawn\",\"ps_spawned_possessed\",\"open_owned_generic_ip\",\"game_state_input\",\"movement_inventory\",\"selected_hero\",\"twelve_live_weaks\"]}",
            clientInputSourceMask.load(),clientInputSourceWeakFailure.load(),static_cast<unsigned long long>(clientInputSourcePcTable.load()));
        std::fputs(",\"server_predicate\":",f);local_server_server_predicate::Write(f);
        std::fputs(",\"reflection_probe\":",f);local_server_reflection_probe::Write(f);
        std::fputs(",\"remote_timeline_probe\":",f);local_server_remote_timeline_probe::Write(f);
        std::fputs(",\"hero_selection\":",f);local_server_hero_selection::Write(f);
        std::fputs(",\"starting_equipment\":",f);local_server_starting_equipment::Write(f);
        std::fputs(",\"authority_input\":",f);local_server_input::WriteState(f);
        const auto runtime=local_server_runtime::StateJson();
        std::fputs(",\"session_runtime\":",f);std::fputs(runtime.c_str(),f);std::fputs("}\n",f);
        std::fclose(f);
        MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING);
    }
    original_faults::WriteDiagnostics((directory+L"native-faults.json").c_str());
    local_server_login::WriteDiagnostics((directory+L"login-state.json").c_str());
    local_server_environment::WriteDiagnostics((directory+L"environment-state.json").c_str());
    local_server_equipment::WriteDiagnostics((directory+L"equipment-state.json").c_str());
    local_server_item_ownership::WriteDiagnostics((directory+L"item-ownership-state.json").c_str());
    local_server_cosmetics::WriteDiagnostics((directory+L"cosmetics-state.json").c_str());
    local_server_combat::WriteDiagnostics((directory+L"combat-state.json").c_str());
    local_server_store_rules::WriteDiagnostics((directory+L"store-rules-state.json").c_str());
    original_timed_streak::WriteDiagnostics((directory+L"timed-streak-state.json").c_str());
    local_server_hero_event_authority::WriteDiagnostics((directory+L"hero-event-state.json").c_str());
    local_server_input_lifecycle::WriteDiagnostics((directory+L"input-lifecycle-state.json").c_str());
    local_server_effect_authority::WriteDiagnostics((directory+L"effect-authority-state.json").c_str());
    local_server_effect_schema_probe::WriteDiagnostics((directory+L"fxc-schema.json").c_str());
    ReleaseSRWLockExclusive(&fileLock);
}
static bool VerifyProfiles(){
    for(const auto& p:NativeProfiles){
        const auto* bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        uint64_t hash=14695981039346656037ULL;
        for(unsigned i=0;i<p.bytes;++i)hash=(hash^bytes[i])*1099511628211ULL;
        if(hash!=p.fnv)return false;
    }
    return true;
}
static bool AddressText(void* addr,wchar_t* output,size_t count){
    using Format=FString*(*)(void*,FString*,bool);
    auto format=Virtual<Format>(addr,0x80);if(!format)return false;
    FString value{};format(addr,&value,false);CopyString(value,output,count);
    engine::function<void(*)(FString*)>(0x18a2f10)(&value);
    return output[0]!=0;
}
static bool Loopback(const wchar_t* text){return std::wcscmp(text,L"127.0.0.1")==0||std::wcscmp(text,L"::1")==0;}
static bool VerifyBindCandidates(void* value){
    using Subsystem=void*(*)(void*);
    if(reinterpret_cast<uintptr_t>(Virtual<Subsystem>(value,0x440))!=engine::image+0x51173c0)return false;
    auto subsystem=Virtual<Subsystem>(value,0x440)(value);if(!subsystem)return false;
    // Retain the single parsed shared address for this bounded owned process.
    // The candidate array itself is released through its exact original dtor.
    Pair explicitBind{};
    if(!engine::function<bool(*)(void*,Pair*)>(0x21a82e0)(subsystem,&explicitBind)||!explicitBind.object)return false;
    wchar_t text[128]{};
    if(!AddressText(explicitBind.object,text,128)||!Loopback(text))return false;
    Array candidates{};
    auto enumerate=Virtual<Array*(*)(void*,Array*)>(subsystem,0xd8);if(!enumerate)return false;
    enumerate(subsystem,&candidates);
    bool ok=candidates.data&&candidates.count>0&&candidates.count<=16&&candidates.capacity>=candidates.count;
    candidateCount=ok?static_cast<unsigned>(candidates.count):0;
    if(ok)for(int i=0;i<candidates.count;++i){
        auto addr=Read<void*>(candidates.data,size_t(i)*sizeof(Pair));
        if(!AddressText(addr,text,128)||!Loopback(text)){ok=false;break;}
    }
    engine::function<void(*)(Array*)>(0x18cbef0)(&candidates);
    return ok;
}
static bool ProveNativeListener(void* world,void* value,void* socket,void* address,uint32_t port){
    if(config.role!=1||!profilesVerified||!listenReturned||!loopbackCandidates||cleaned.load()
       ||world!=reinterpret_cast<void*>(context.world)||value!=driver.load()||port!=config.port
       ||Read<void*>(world,0x40)!=value||Read<void*>(value,0x178)!=world||Read<void*>(value,0xc0)
       ||socket!=Read<void*>(value,0x968)||address!=Read<void*>(value,0x778))return false;
    wchar_t effective[128]{};
    return socket&&address&&AddressText(address,effective,std::size(effective))&&Loopback(effective)
        &&Virtual<int(*)(void*)>(address,0x38)(address)==static_cast<int>(port);
}
static void CleanupHost(bool postWorld=false){
    if(config.role!=1||!driver.load()||cleaned.load())return;
    hostCleanupPending=true;
    local_server_rpc_policy::Revoke();
    if(!local_server_spectator::Close()){
        Fail(9037,"native_host_spectator_owned_order_restore_failed");return;
    }
    // The real paired frame must already have been observed by NetTick.
    // Damage upper bindings close before any lower compatibility/login table.
    if(!local_server_runtime::PrepareClose()){
        const auto& runtime=local_server_runtime::GetState();
        if(runtime.error)Fail(runtime.error,runtime.stage);
        else stage=runtime.stage;
        SaveState();return;
    }
    if(!local_server_dagger_points::Close()){
        Fail(77,"native_host_dagger_observers_restore_failed");return;
    }
    if(!local_server_login::Close()){
        Fail(local_server_login::GetState().error?local_server_login::GetState().error:54,local_server_login::GetState().stage);return;
    }
    if(!local_server_runtime::Close()){
        const auto& runtime=local_server_runtime::GetState();
        if(runtime.error)Fail(runtime.error,runtime.stage);else stage=runtime.stage;
        SaveState();return;
    }
    if(local_server_input::GetState().installed&&!local_server_input::RetireAuthority(local_server_input::GetState().pawn)){
        const auto& input=local_server_input::GetState();
        if(input.error)Fail(input.error,input.stage);else stage="native_host_close_waiting_idle_authority_input";
        SaveState();return;
    }
    if(!local_server_server_predicate::Close()){
        const auto& predicate=local_server_server_predicate::GetState();
        Fail(predicate.error?predicate.error:83,predicate.stage);return;
    }
    if(!postWorld){stage="native_host_owned_hooks_restored_waiting_post_world_driver_cleanup";SaveState();return;}
    stage="native_host_destroying_named_driver";SaveState();
    engine::function<void(*)(void*,const FName*)>(0x4c2bcd0)(worldContext,&driverName);
    if(Read<void*>(reinterpret_cast<void*>(context.world),0x40)==driver.load()){
        Fail(16,"native_host_driver_cleanup_not_observed");return;
    }
    driver=nullptr;cleaned=true;hostCleanupPending=false;stage="native_host_driver_destroyed";SaveState();
}
static void Host(){
    void* world=reinterpret_cast<void*>(context.world);
    if(Read<void*>(world,0x40)){Fail(10,"native_host_world_already_has_driver");return;}
    engine::function<void*(*)(FName*,const wchar_t*,int)>(0x1a7b720)(&driverName,L"GameNetDriver",1);
    using Factory=void*(*)(void*,void*,const FName*,const FName*);
    void* value=engine::function<Factory>(0x4c2b940)(owningEngine,worldContext,&driverName,&driverName);
    if(!value){Fail(11,"native_host_driver_factory_failed");return;}
    driver=value;stage="native_host_driver_created";SaveState();
    const auto expectedTable=engine::image+(config.reserved?0xacb4100:0xb0b5df0);
    if(reinterpret_cast<uintptr_t>(Read<void*>(value))!=expectedTable){Fail(9,"native_host_configured_driver_class_mismatch");CleanupHost();return;}
    if(reinterpret_cast<uintptr_t>(Virtual<void*>(value,0x310))!=engine::image+0x5118750
       ||reinterpret_cast<uintptr_t>(Virtual<void*>(value,0x448))!=engine::image+0x4742820){
        Fail(12,"native_host_driver_virtual_profile_mismatch");CleanupHost();return;
    }
    loopbackCandidates=VerifyBindCandidates(value);
    if(!loopbackCandidates){Fail(13,"native_host_requires_original_loopback_binding_candidates");CleanupHost();return;}
    // These three server-world links are precisely the retained LoadMap
    // Pending->World transfer contract. No client roles/global modes are changed.
    Put(world,0x40,value);
    Virtual<void(*)(void*,void*)>(value,0x448)(value,world);
    using Collection=void*(*)(void*,int32_t);
    auto collection=engine::function<Collection>(0x4ce4910);
    for(const int type:{0,2}){
        auto target=collection(world,type);
        if(!target){Fail(14,"native_host_original_level_collection_missing");CleanupHost();return;}
        Put(target,0x10,value);
    }
    wchar_t urlText[96]{};swprintf_s(urlText,L"127.0.0.1:%u",config.port);
    FURL url{};FString failure{};
    engine::function<FURL*(*)(FURL*,const FURL*,const wchar_t*,int32_t)>(0x4c54840)(&url,nullptr,urlText,0);
    if(Read<int32_t>(&url,0x24)!=1||Read<int32_t>(&url,0x20)!=static_cast<int32_t>(config.port)){
        engine::function<void(*)(FURL*)>(0x3df7850)(&url);Fail(15,"native_host_original_url_parse_failed");CleanupHost();return;
    }
    stage="native_host_entering_original_listen";SaveState();
    using Listen=bool(*)(void*,void*,FURL*,bool,FString*);
    listenReturned=Virtual<Listen>(value,0x310)(value,static_cast<uint8_t*>(world)+0x30,&url,false,&failure);
    CopyString(failure,nativeError,512);
    engine::function<void(*)(FString*)>(0x18a2f10)(&failure);
    engine::function<void(*)(FURL*)>(0x3df7850)(&url);
    if(!listenReturned){Fail(17,"native_host_original_listen_failed");CleanupHost();return;}
    auto socket=Read<void*>(value,0x968),address=Read<void*>(value,0x778);
    if(!socket||!address){Fail(18,"native_host_actual_socket_missing");CleanupHost();return;}
    Virtual<void(*)(void*,void*)>(socket,0x88)(socket,address);
    if(!AddressText(address,bindAddress,128)||!Loopback(bindAddress)
       ||Virtual<int(*)(void*)>(address,0x38)(address)!=static_cast<int>(config.port)){
        Fail(19,"native_host_effective_socket_is_not_requested_loopback");CleanupHost();return;
    }
    if(!local_server_server_predicate::Install(world,value,owningEngine,config.port,ProveNativeListener)){
        const auto& predicate=local_server_server_predicate::GetState();
        Fail(predicate.error?predicate.error:83,predicate.stage);CleanupHost();return;
    }
    stage="native_host_listener_created_waiting_runtime";SaveState();
}
static void Client(){
    void* pc=reinterpret_cast<void*>(context.controller);
    void* fn=engine::Find(L"/Script/Engine.PlayerController.ClientTravel");
    clientParameterBytes=Read<uint16_t>(fn,0xbe);
    clientFunctionABI=fn&&clientParameterBytes==36;
    if(!clientFunctionABI){Fail(20,"native_client_travel_parameter_layout_mismatch");return;}
    wchar_t url[96]{};swprintf_s(url,L"127.0.0.1:%u?Name=VT_Offline_Human",config.port);
    alignas(8) uint8_t params[40]{};
    FString urlValue{url,static_cast<int>(std::wcslen(url))+1,static_cast<int>(std::wcslen(url))+1};
    Put(params,0,urlValue); // Absolute0, non-seamless, null Guid are zero.
    stage="native_client_requesting_original_travel";SaveState();
    engine::Event(pc,L"/Script/Engine.PlayerController.ClientTravel",params);
    actionCompleted=true;stage="native_client_original_travel_requested";SaveState();
}
static bool Event(void* object,const wchar_t* name,unsigned bytes,void* parameters){
    auto fn=engine::Find(name);return fn&&Read<uint16_t>(fn,0xbe)==bytes&&engine::Event(object,name,parameters);
}
static bool SpawnRemote(){
    auto type=engine::Find(L"/Script/AIModule.AIController");
    if(!type||!defaultPawnClass)return false;
    engine::Transform transform;
    auto local=Read<engine::Vec>(Read<void*>(reinterpret_cast<void*>(context.pawn),0x290),0x170);
    transform.translation={local.x+500,local.y,local.z+100};
    using Spawn=void*(*)(void*,void*,const engine::Transform*,uint8_t,void*,uint8_t);
    using Finish=void(*)(void*,const engine::Transform*,bool,void*,uint8_t);
    auto spawn=engine::function<Spawn>(0x43fada0);auto finish=engine::function<Finish>(0x3ef6cb0);
    auto world=reinterpret_cast<void*>(context.world);
    auto pawn=spawn(world,defaultPawnClass,&transform,1,nullptr,0);
    if(!pawn)return false;
    remotePawn=pawn;finish(pawn,&transform,false,nullptr,0);
    auto ai=spawn(world,type,&transform,1,nullptr,0);if(!ai)return false;
    remoteController=ai;finish(ai,&transform,false,nullptr,0);
    uint8_t yes=1;
    if(!Event(pawn,L"/Script/Engine.Actor.SetReplicates",1,&yes)
       ||!Event(pawn,L"/Script/Engine.Actor.SetReplicateMovement",1,&yes)
       ||!Event(ai,L"/Script/Engine.Controller.Possess",8,&pawn))return false;
    uint8_t aiLocal=0,pawnLocal=0;
    if(!Event(ai,L"/Script/Engine.Controller.IsLocalController",1,&aiLocal)
       ||!Event(pawn,L"/Script/Engine.Pawn.IsLocallyControlled",1,&pawnLocal))return false;
    remoteAIIsLocal=aiLocal!=0;remotePawnIsLocal=pawnLocal!=0;
    return remoteAIIsLocal&&remotePawnIsLocal&&Read<void*>(ai,0x4b0)==pawn;
}
static void MoveRemote(){
    if(!remotePawn.load()||!remoteController.load()||!remotePawnIsLocal)return;
    struct Input {engine::Vec direction;float scale;uint8_t force=0,pad[3]{};};
    Input input{{(GetTickCount64()/1000)%2?1.0:-1.0,0,0},1,0,{}};
    if(!Event(remotePawn.load(),L"/Script/Engine.Pawn.AddMovementInput",29,&input)){
        Fail(43,"native_remote_pawn_movement_input_abi_failed");return;
    }
    ++moveSubmissions;
}
static bool IsA(void* object,void* base){
    auto type=Read<void*>(object,0x10);
    for(unsigned i=0;type&&i<32;++i){if(type==base)return true;type=Read<void*>(type,0x40);}
    return false;
}
static void ScanClientPawns(){
    // Passive original connection/channel registry, including across travel.
    // No rebinding of the client's new Pawn and no engine calls off-thread.
    if(!worldContext||!actorChannelClass||GetTickCount64()-lastClientScan<100)return;
    lastClientScan=GetTickCount64();
    auto world=Read<void*>(worldContext,0x2c8);auto currentDriver=Read<void*>(world,0x40);
    auto connection=Read<void*>(currentDriver,0xc0);
    if(!connection||Read<uint8_t>(connection,0x13c)!=3)return;
    struct Channels {void** data;int32_t count,capacity;};
    const auto channels=Read<Channels>(connection,0x78);
    if(!channels.data||channels.count<0||channels.count>512||channels.capacity<channels.count)return;
    unsigned found=0;engine::Vec position{};
    for(int i=0;i<channels.count;++i){
        auto channel=Read<void*>(channels.data,size_t(i)*8);
        if(!channel||Read<void*>(channel,0x30)!=connection||(Read<uint8_t>(channel,0x38)&2)||!IsA(channel,actorChannelClass))continue;
        auto pawn=Read<void*>(channel,0x78);
        if(Read<void*>(pawn,0x10)==defaultPawnClass&&Read<uint8_t>(pawn,0x224)==1){
            ++found;position=Read<engine::Vec>(Read<void*>(pawn,0x290),0x170);
        }
    }
    const auto again=Read<Channels>(connection,0x78);
    if(again.data!=channels.data||again.count!=channels.count||Read<void*>(worldContext,0x2c8)!=world)return;
    clientRemoteCount=found;clientRemotePosition=position;clientRemoteSampleAt=GetTickCount64();
}
static bool AdoptJoinedConnection(void* connection,void* pc){
    if(!pc||Read<int32_t>(connection,0x13c)!=3||Read<uint8_t>(connection,0x1d8)!=3
       ||Read<void*>(connection,0xa0)!=pc||Read<void*>(pc,0x510)!=connection)return false;
    if((!local_server_runtime::GetState().enabled||!local_server_runtime::GetState().humanAdopted)
       &&!local_server_names::Register(0,1,Read<void*>(pc,0x470),L"Player")){
        if(local_server_names::GetState().error)Fail(local_server_names::GetState().error,local_server_names::GetState().stage);
        return false;
    }
    if(!local_server_names::GetState().ready)return false;
    if(!local_server_gameplay::AdoptConnection(connection)){
        Fail(local_server_gameplay::state.error,local_server_gameplay::state.stage);return false;
    }
    if(!local_server_gameplay::state.heroSpawned)return false;
    if(local_server_runtime::GetState().enabled&&!local_server_runtime::GetState().humanAdopted
       &&!local_server_runtime::AdoptHuman(pc,Read<void*>(pc,0x470),Read<void*>(pc,0x4b0),connection)){
        Fail(local_server_runtime::GetState().error,local_server_runtime::GetState().stage);return false;
    }
    if(local_server_runtime::GetState().enabled&&!local_server_environment::RegisterConnection(connection,pc)){
        Fail(local_server_environment::GetState().error?local_server_environment::GetState().error:60,local_server_environment::GetState().stage);return false;
    }
    return true;
}
static void AdoptKnownConnection(){
    auto value=driver.load();auto list=Read<void**>(value,0xc8);const int count=Read<int32_t>(value,0xd0);
    if(!list||count<1||count>8)return;
    for(int i=0;i<count;++i){auto connection=Read<void*>(list,size_t(i)*8);
        AdoptJoinedConnection(connection,Read<void*>(connection,0x38));if(error.load())return;
    }
}
static void ReplicateKnownActors(){
    // Missing ordinary server traversal only. Native level readiness, weak
    // identity, channel creation, PreReplication and property encoding remain
    // intact. Only this joined connection's own actors and shared GameState.
    void* value=driver.load();if(!value||cleaned.load()||!listenReturned)return;
    const uint64_t now=GetTickCount64();if(now-lastReplicationAt<50)return;lastReplicationAt=now;
    // Includes GetActors registry traversal and original actor-channel encoding.
    armor_poll_performance::Scope measurement(armor_poll_performance::Slot::ReplicationRegistry);
    auto list=Read<void**>(value,0xc8);const int count=Read<int32_t>(value,0xd0);
    if(!list||count<1||count>8)return;
    // Original demo record frame431CF7E increments this same native frame
    // once before its actor batch. The stripped ordinary server traversal no
    // longer advances it; unchanged frames reuse the native changelist cache.
    Put<uint32_t>(value,0x4d8,Read<uint32_t>(value,0x4d8)+1U);
    if(!actorChannelName.words[0])engine::function<void*(*)(FName*,const wchar_t*,int)>(0x1a7b720)(&actorChannelName,L"Actor",1);
    for(int i=0;i<count;++i){
        auto connection=Read<void*>(list,size_t(i)*8),pc=Read<void*>(connection,0x38);
        if(!AdoptJoinedConnection(connection,pc))continue;
        if(!local_server_runtime::GetState().enabled&&!remotePawn.load()&&!SpawnRemote()){
            Fail(42,"native_remote_pawn_ai_factory_failed");return;
        }
        void* view=nullptr;
        if(!Event(pc,L"/Script/Engine.Controller.GetViewTarget",8,&view)){Fail(44,"native_replication_view_target_query_failed");return;}
        if(!view)view=Read<void*>(connection,0xa0);
        Put(connection,0x98,view);
        auto levelReady=Virtual<bool(*)(void*,void*)>(connection,0x3c8);
        if(reinterpret_cast<uintptr_t>(levelReady)!=engine::image+0x47113e0){Fail(40,"native_replication_level_gate_profile_mismatch");return;}
        std::array<void*,local_server_runtime::MaxRegistryActors+local_server_environment::MaxActors> actors{};size_t actorCount=0;
        if(local_server_runtime::GetState().enabled){
            if(!local_server_runtime::GetActors([&](void* actor){actors[actorCount++]=actor;})){
                if(local_server_runtime::GetState().error)
                    Fail(local_server_runtime::GetState().error,local_server_runtime::GetState().stage);
                return;
            }
            std::array<void*,local_server_environment::MaxActors> environmentActors{};
            const auto environmentCount=local_server_environment::RegistryActors(environmentActors.data(),environmentActors.size());
            if(environmentCount>environmentActors.size()){Fail(61,"native_environment_actor_registry_capacity_exceeded");return;}
            for(size_t n=0;n<environmentCount;++n){
                bool present=false;for(size_t k=0;k<actorCount;++k)if(actors[k]==environmentActors[n]){present=true;break;}
                if(!present)actors[actorCount++]=environmentActors[n];
            }
        }else{
            void* probeActors[]{Read<void*>(reinterpret_cast<void*>(context.world),0x178),pc,
                               Read<void*>(pc,0x470),Read<void*>(pc,0x4b0),remotePawn.load()};
            for(void* actor:probeActors)actors[actorCount++]=actor;
        }
        for(size_t actorIndex=0;actorIndex<actorCount;++actorIndex){
            auto actor=actors[actorIndex];
            if(!actor||Read<uint8_t>(actor,0x224)!=3)continue;
            if(!levelReady(connection,actor)){++levelDeferrals;continue;}
            struct Weak {int32_t index=-1,serial=0;} identity;
            identity.index=Read<int32_t>(actor,0xc);if(identity.index<0)continue;
            identity.serial=engine::function<int32_t(*)(void*,int32_t)>(0x1cf35b0)
                (reinterpret_cast<void*>(engine::image+0xd7b87c0),identity.index);
            if(identity.serial<=0)continue;
            engine::function<void(*)(void*,void*)>(0x3edad30)(actor,value);
            auto channel=engine::function<void*(*)(void*,const Weak*)>(0x4313720)(connection,&identity);
            if(!channel){
                channel=engine::function<void*(*)(void*,const FName*,uint32_t,int32_t)>(0x4717bf0)
                    (connection,&actorChannelName,2,-1);
                if(!channel)continue;
                engine::function<void(*)(void*,void*,uint32_t)>(0x42db600)(channel,actor,0);++channelsCreated;
            }
            if(Read<void*>(channel,0x30)!=connection||Read<void*>(channel,0x78)!=actor){
                Fail(41,"native_replication_channel_owner_actor_mismatch");return;
            }
            if(Read<uint8_t>(channel,0x38)&2)continue;
            const auto bits=engine::function<int64_t(*)(void*)>(0x42ddc40)(channel);++replicationCalls;
            if(bits>0)replicationBits.fetch_add(static_cast<uint64_t>(bits));
        }
        auto& gameplay=local_server_gameplay::state;
        if(gameplay.heroSpawned&&!gameplay.clientRestartSubmitted){
            auto fn=engine::Find(L"/Script/Engine.PlayerController.ClientRestart");
            if(!fn||Read<uint16_t>(fn,0xbe)!=8||(Read<uint32_t>(fn,0xb8)&0x01000040)!=0x01000040){
                Fail(49,"native_server_client_restart_rpc_abi_mismatch");return;
            }
            auto possessed=gameplay.hero;
            if(!engine::Event(pc,L"/Script/Engine.PlayerController.ClientRestart",&possessed)){
                Fail(50,"native_server_client_restart_dispatch_failed");return;
            }
            gameplay.clientRestartSubmitted=true;
        }
    }
}
static LONG Exception(EXCEPTION_POINTERS* p){
    const auto at=reinterpret_cast<uintptr_t>(p->ExceptionRecord->ExceptionAddress);
    exceptionRva=at>=engine::image&&at<engine::image+0xe000000?at-engine::image:at;
    Fail(30,"native_network_probe_exception");return EXCEPTION_EXECUTE_HANDLER;
}
static void OnTick(){
    ++ticks;
    if(!actionStarted.exchange(true)){
        profilesVerified=VerifyProfiles();
        if(!profilesVerified){Fail(2,"native_network_source_profile_mismatch");return;}
        local_server_runtime::Hooks hooks;hooks.spawnHuman=local_server_gameplay::SpawnHuman;hooks.isObserver=RuntimeObserver;
        hooks.requestStop=RuntimeStop;hooks.beforeRetire=RuntimeRetire;hooks.manageClientPresentation=true;hooks.port=config.port;
        local_server_empty_bootstrap_probe::Initialize(config.role,directory.c_str());
        if(!local_server_runtime::Load(directory,config.role==1)||!local_server_runtime::SetHooks(hooks)){
            Fail(local_server_runtime::GetState().error?local_server_runtime::GetState().error:53,local_server_runtime::GetState().stage);return;
        }
        const auto& initialPath=local_server_control::initialPath;
        const auto sidecarAt=initialPath.find_last_of(L"\\/");
        if(sidecarAt==std::wstring::npos
           ||!local_server_hero_selection::Load(initialPath.substr(0,sidecarAt)+L"\\session-hero-selection.bin",
                local_server_runtime::GetState().enabled,local_server_control::Initial().config.revision)){
            const auto& selection=local_server_hero_selection::GetState();
            Fail(selection.error?selection.error:81,selection.stage);return;
        }
        if(config.role==1&&!local_server_gameplay::SetInitialSpawnProvider(local_server_runtime::PrepareInitialHumanSpawn)){
            Fail(3135,"native_initial_human_spawn_policy_binding_failed");return;
        }
        auto world=reinterpret_cast<void*>(context.world);
        defaultPawnClass=Read<void*>(reinterpret_cast<void*>(context.pawn),0x10);
        actorChannelClass=engine::Class(0x3dd3d40);
        shooterCharacterClass=engine::Class(original_players::ShooterCharacterClass);
        gameInstance=Read<void*>(world,0x1d8);worldContext=Read<void*>(gameInstance,0x38);
        owningEngine=Read<void*>(gameInstance,0x28);
        bootstrapPlayerState=Read<void*>(reinterpret_cast<void*>(context.controller),0x470);
        if(!worldContext||Read<void*>(worldContext,0x2c8)!=world||Read<void*>(worldContext,0x210)!=gameInstance){
            Fail(3,"native_network_world_context_identity_mismatch");return;
        }
        auto contexts=Read<void**>(owningEngine,0x1120);const int count=Read<int>(owningEngine,0x1128);
        bool found=false;
        if(contexts&&count>0&&count<=16)for(int i=0;i<count;++i)found|=Read<void*>(contexts,size_t(i)*8)==worldContext;
        if(!found){Fail(4,"native_network_engine_context_membership_mismatch");return;}
    }
    if(!actionCompleted.load()){
        const auto prepared=local_server_gameplay::Prepare(reinterpret_cast<void*>(context.world),
            reinterpret_cast<void*>(context.controller),reinterpret_cast<void*>(context.pawn),config.role==1,
            local_server_runtime::ConfigureFactory);
        if(prepared==local_server_gameplay::Preparation::Failed){
            Fail(local_server_gameplay::state.error,local_server_gameplay::state.stage);return;
        }
        if(prepared!=local_server_gameplay::Preparation::Ready)return;
        if(!local_server_names::Initialize(directory,config.role==1)){
            Fail(local_server_names::GetState().error,local_server_names::GetState().stage);return;
        }
        if(!local_server_rpc_policy::Initialize(directory,config.role==1,config.port)){
            Fail(local_server_rpc_policy::GetState().error?local_server_rpc_policy::GetState().error:55,local_server_rpc_policy::GetState().stage);return;
        }
        if(config.role==1){
            auto locals=Read<void**>(gameInstance,0x40);auto localPlayer=Read<void*>(locals);
            if(!local_server_bootstrap_roster::ExcludeBootstrapState(reinterpret_cast<void*>(context.world),local_server_gameplay::state.gameState,reinterpret_cast<void*>(context.controller),localPlayer)){
                Fail(local_server_bootstrap_roster::State().error?local_server_bootstrap_roster::State().error:66,local_server_bootstrap_roster::State().stage);return;
            }
            if(!local_server_gameplay::ConfigureHostClasses()){
                Fail(local_server_gameplay::state.error,local_server_gameplay::state.stage);return;
            }
            if(!(local_server_mode_startup::GetState().configured
                 ?local_server_gameplay::PublishNativeModeMatchStarted()
                 :local_server_character_lifecycle::PublishAuthorityMatchStarted(reinterpret_cast<void*>(context.world)))){
                Fail(local_server_character_lifecycle::State().error,local_server_character_lifecycle::State().stage);return;
            }
            if(local_server_runtime::GetState().enabled&&!local_server_store_rules::InitializeOrPublish(reinterpret_cast<void*>(context.world))){
                Fail(local_server_store_rules::GetState().error,local_server_store_rules::GetState().stage);return;
            }
            Host();
            if(!error.load()&&listenReturned&&local_server_runtime::GetState().enabled
               &&!local_server_runtime::InitializeWorld(reinterpret_cast<void*>(context.world),driver.load(),true))
                Fail(local_server_runtime::GetState().error,local_server_runtime::GetState().stage);
            if(error.load())return;
            if(local_server_runtime::GetState().enabled&&!local_server_environment::Initialize(reinterpret_cast<void*>(context.world),local_server_gameplay::state.gameState,local_server_control::Initial().mapId)){
                Fail(local_server_environment::GetState().error?local_server_environment::GetState().error:62,local_server_environment::GetState().stage);return;
            }
            local_server_login::Hooks loginHooks;loginHooks.requestStop=RuntimeStop;
            if(!local_server_login::Install(reinterpret_cast<void*>(context.world),driver.load(),loginHooks)){
                Fail(local_server_login::GetState().error?local_server_login::GetState().error:56,local_server_login::GetState().stage);return;
            }
            actionCompleted=true;stage="native_host_listening_verified_loopback";SaveState();
        }else{
            if(!local_server_equipment::PrepareClientAppearance()){
                Fail(local_server_equipment::state.error,local_server_equipment::state.stage);return;
            }
            Client();
        }
    }
    // Active host simulation/replication runs from native post-world NetTick,
    // independently of the bootstrap pawn's actor tick lifetime.
}
// Fallback servicing is deliberately independent of actionCompleted:
// partial initialization can publish hooks before a NetTick shim exists.
static bool ServiceBootstrapHostCleanup(){
    if(config.role!=1)return false;
    if(cleaned.load())return true;
    if(!driver.load())return false;
    if(GetCurrentThreadId()!=original_players::gameThread){Fail(45,"native_bootstrap_cleanup_wrong_game_thread");return true;}
    const auto table=Read<void**>(driver.load());
    // Normal operation has one stop-file poll in NetTick. The bootstrap
    // fallback does not add per-frame disk reads when that real shim exists.
    if(table&&Read<void*>(table,0x398)==reinterpret_cast<void*>(NetTick))
        return hostCleanupPending.load()||local_server_runtime::GetState().closePending;
    if(!hostCleanupPending.load()&&!local_server_runtime::GetState().closePending
       &&GetFileAttributesW((directory+L"stop-network").c_str())==INVALID_FILE_ATTRIBUTES)return false;
    hostCleanupPending=true;local_server_runtime::RequestClose();
    // No flush shim exists yet. Restore safe hooks only; never pretend Actor
    // Tick is post-world or destroy the current frame's driver from here.
    CleanupHost(false);return true;
}
static void Tick(void* actor,float delta){
    originalTick(actor,delta);
    __try {
        if(ServiceBootstrapHostCleanup())return;
        if(error.load())return;
        OnTick();
    }
    __except(Exception(GetExceptionInformation())) {
        if(config.role==1&&driver.load()&&!cleaned.load())hostCleanupPending=true;
        SaveState();
    }
}
static void NetTickImpl(void* value,float delta){
    // Project post-original driver phase; not a whole world/render frame.
    armor_poll_performance::Scope measurement(armor_poll_performance::Slot::NetTickAdapter);
    if(config.role==1&&!cleaned.load()&&(hostCleanupPending.load()||local_server_runtime::GetState().closePending
       ||GetFileAttributesW((directory+L"stop-network").c_str())!=INVALID_FILE_ATTRIBUTES)){
        // This retry precedes the global error/stopping gates. Do not lose a
        // deferred Close when its caller stack unwinds or the stop file goes.
        if(GetCurrentThreadId()!=original_players::gameThread){Fail(45,"native_host_cleanup_wrong_game_thread");return;}
        hostCleanupPending=true;local_server_runtime::RequestClose();
        if(!local_server_runtime::ObservePendingWorldFrame(delta)){
            const auto& runtime=local_server_runtime::GetState();
            Fail(runtime.error?runtime.error:51,runtime.stage);
        }
        CleanupHost(true);
        return;
    }
    if(error.load())return;
    if(!local_server_names::Poll()){
        Fail(local_server_names::GetState().error,local_server_names::GetState().stage);return;
    }
        if(GetCurrentThreadId()!=original_players::gameThread){Fail(45,"native_network_tick_wrong_thread");return;}
        ObserveWorldIdentity(Read<void*>(worldContext,0x2c8));
        local_server_reflection_probe::Poll();
        if(config.role==1){
            local_server_authority_clock_probe::Poll(reinterpret_cast<void*>(context.world),value);
            if(local_server_server_predicate::GetState().installed&&!local_server_server_predicate::Poll()){
                const auto& predicate=local_server_server_predicate::GetState();
                Fail(predicate.error?predicate.error:83,predicate.stage);return;
            }
            if(!local_server_login::Poll()){
                Fail(local_server_login::GetState().error?local_server_login::GetState().error:57,local_server_login::GetState().stage);return;
            }
            if(!local_server_runtime::GetState().enabled)MoveRemote();
            AdoptKnownConnection();if(error.load())return;
            if(local_server_runtime::GetState().enabled){
                const auto connection=local_server_runtime::ObserveAdmittedConnection();
                if(connection==local_server_runtime::AdmittedConnectionObservation::Failed){
                    const auto& runtime=local_server_runtime::GetState();Fail(runtime.error,runtime.stage);return;
                }
                if(connection==local_server_runtime::AdmittedConnectionObservation::Closed){
                    if(!local_server_runtime::StopDisconnectedWorldFrame(delta)){
                        const auto& runtime=local_server_runtime::GetState();Fail(runtime.error?runtime.error:51,runtime.stage);
                    }
                    return;
                }
            }
            if(local_server_runtime::GetState().enabled&&!local_server_runtime::PreprocessDeaths()){
                Fail(local_server_runtime::GetState().error,local_server_runtime::GetState().stage);return;
            }
            if(local_server_runtime::GetState().heroDamageBoundaryPending){
                // Complete the already armed true world pair in observation
                // mode, then skip this whole adapter/input/respawn frame.
                if(!local_server_runtime::HoldPendingWorldFrame(delta)){
                    const auto& runtime=local_server_runtime::GetState();Fail(runtime.error?runtime.error:51,runtime.stage);
                }
                return;
            }
            if(!local_server_rpc_policy::Poll(reinterpret_cast<void*>(context.world),Read<void*>(local_server_gameplay::state.networkController,0x510))){
                Fail(local_server_rpc_policy::GetState().error?local_server_rpc_policy::GetState().error:58,local_server_rpc_policy::GetState().stage);return;
            }
            const auto& game=local_server_gameplay::state;
            if(game.heroSpawned&&(!local_server_runtime::GetState().enabled||local_server_runtime::ShouldPollAuthorityInput())){
                if(!local_server_input::GetState().installed)
                    local_server_input::InstallAuthority(game.world,game.networkController,game.hero,Read<void*>(game.networkController,0x510));
                if(local_server_input::GetState().error){
                    local_server_empty_bootstrap_probe::ObserveHostPoll(local_server_input::GetState());
                    Fail(local_server_input::GetState().error,local_server_input::GetState().stage);return;
                }
                if(!local_server_runtime::GetState().enabled&&local_server_input::GetState().installed
                   &&!local_server_game_start::GetState().scheduleSubmitted&&!local_server_game_start::Start(game.world,game.hero)){
                    Fail(local_server_game_start::GetState().error,local_server_game_start::GetState().stage);return;
                }
                if(local_server_runtime::GetState().enabled&&local_server_runtime::Practice()
                   &&!local_server_dagger_points::Install(game.world,local_server_combat::OnNativePoint)){
                    const auto& points=local_server_dagger_points::state;
                    Fail(points.error?points.error:78,points.stage);return;
                }
                if(local_server_input::GetState().installed&&!local_server_input::Poll(delta)
                   &&local_server_input::GetState().error){
                    local_server_empty_bootstrap_probe::ObserveHostPoll(local_server_input::GetState());
                    Fail(local_server_input::GetState().error,local_server_input::GetState().stage);return;
                }
                local_server_empty_bootstrap_probe::ObserveHostPoll(local_server_input::GetState());
                if(local_server_input::GetState().disconnected){
                    if(local_server_runtime::GetState().enabled){
                        if(!local_server_runtime::StopDisconnectedWorldFrame(delta)){
                            const auto& runtime=local_server_runtime::GetState();
                            Fail(runtime.error?runtime.error:51,runtime.stage);
                        }
                    }
                    else stage="native_host_connection_closed_waiting_owned_stop";
                    return;
                }
                if(GetTickCount64()-lastViewTargetAt>=16||publishedViewPawn!=game.hero){
                    lastViewTargetAt=GetTickCount64();
                    if(local_server_spectator::OwnsAuthorityPolicy())local_server_spectator::Poll();
                    else {
                    const auto previous=publishedViewPawn==game.hero?nullptr:publishedViewPawn;
                    const auto result=local_server_viewtarget::PublishAuthorityViewTarget(game.world,game.networkController,game.networkState,game.hero,previous);
                    if(result==local_server_viewtarget::Result::Failed){
                        Fail(local_server_viewtarget::GetState().error,local_server_viewtarget::GetState().stage);return;
                    }
                    if(result==local_server_viewtarget::Result::Published)publishedViewPawn=game.hero;
                    }
                }
            }
            if(!error.load()&&local_server_runtime::GetState().enabled&&!local_server_runtime::TickHost(delta))
                Fail(local_server_runtime::GetState().error,local_server_runtime::GetState().stage);
            if(!error.load()&&local_server_runtime::GetState().enabled&&!local_server_environment::PollAuthority())
                Fail(local_server_environment::GetState().error?local_server_environment::GetState().error:63,local_server_environment::GetState().stage);
            if(!error.load()&&local_server_runtime::GetState().enabled&&!local_server_store_rules::Poll())
                Fail(local_server_store_rules::GetState().error,local_server_store_rules::GetState().stage);
            if(!error.load())ReplicateKnownActors();
        }else{
            clientInputReady=false;
            if(clientShutdownRequested.load()){
                if(CloseClientInputSession()){
                    local_server_menu_input::Close();stage="original_training_user_exit_requested";
                }
                return;
            }
            DWORD foregroundPid=0;GetWindowThreadProcessId(GetForegroundWindow(),&foregroundPid);
            if(foregroundPid==GetCurrentProcessId()&&(GetAsyncKeyState(VK_F10)&0x8000)){
                clientShutdownRequested=true;
                local_server_native_test::Stop();
                if(!CloseClientInputSession())return;
                local_server_menu_input::Close();
                stage="original_training_user_exit_requested";SaveState();return;
            }
            auto world=Read<void*>(worldContext,0x2c8);
            // Explicit FFA fixture observes its already-bound old life before
            // missing current-owner links can skip the ordinary test Tick.
            if(!local_server_native_test::ObserveBoundBattleLife(world,value)){
                Fail(local_server_native_test::GetState().error,local_server_native_test::GetState().stage);return;
            }
            if(!DrainClientInputLifecycle())return;
            if(!local_server_gameplay::PollClient(world,value))
                Fail(local_server_gameplay::state.error,local_server_gameplay::state.stage);
            if(error.load())return;
            if(!local_server_rpc_policy::Poll(world)){
                Fail(local_server_rpc_policy::GetState().error?local_server_rpc_policy::GetState().error:59,local_server_rpc_policy::GetState().stage);return;
            }
            const auto& game=local_server_gameplay::state;
            ClientInputSource source{};
            const bool sourceReady=ReadClientInputSource(world,value,source);
            local_server_empty_bootstrap_probe::ObserveClientSource(sourceReady,clientInputSourceMask.load(),
                clientInputSourceWeakFailure.load(),clientInputSourcePcTable.load(),source);
            if(sourceReady&&!local_server_input_lifecycle::GetState().initialized
               &&!local_server_input_lifecycle::Init(world,source.pc,RetireClientInputEpoch)){
                ClientInputFailure("native_client_input_lifecycle_init_failed");return;
            }
            if(local_server_runtime::GetState().enabled&&game.clientWorldBound&&!local_server_runtime::GetState().initialized){
                if(!local_server_runtime::InitializeWorld(world,value,false)){
                    Fail(local_server_runtime::GetState().error,local_server_runtime::GetState().stage);return;
                }
            }
            if(local_server_runtime::GetState().enabled&&sourceReady){
                if(!local_server_environment::GetState().initialized
                   &&!local_server_environment::InitializeClient(world,Read<void*>(world,0x178),local_server_control::Initial().mapId)){
                    Fail(local_server_environment::GetState().error?local_server_environment::GetState().error:64,local_server_environment::GetState().stage);return;
                }
                if(local_server_environment::GetState().initialized&&!local_server_environment::PollClientPassive()){
                    Fail(local_server_environment::GetState().error?local_server_environment::GetState().error:65,local_server_environment::GetState().stage);return;
                }
            }
            const auto& previous=local_server_ip_ack_compat::binding;
            const auto& precedingInput=local_server_input_warning_compat::binding;
            const bool changed=previous.active&&(previous.world!=world||previous.pc!=source.pc||previous.ps!=source.ps
                ||previous.pawn!=source.pawn||!local_server_ip_ack_compat::WeakMatch(previous));
            const bool inputChanged=precedingInput.active&&(precedingInput.world!=world||precedingInput.pc!=source.pc
                ||precedingInput.pawn!=source.pawn||precedingInput.input!=source.input
                ||!local_server_ip_ack_compat::Live(precedingInput.input,precedingInput.inputId));
            if((changed||inputChanged||previous.retiring||precedingInput.retiring||clientInputRetirementPending.load())
               &&!RetireObservedClientInputEpoch("observed_client_input_source_changed"))return;
            if(!sourceReady){stage="native_client_waiting_complete_current_replicated_input_source";return;}
            if(game.clientHeroBound){
                const auto pc=source.pc,pawn=source.pawn;
                if(!ClientInputCanBind(source))return;
                if(!local_server_ip_ack_compat::Install(world,pc,pawn)){
                    const auto& compatibility=local_server_ip_ack_compat::GetState();
                    Fail(compatibility.error?compatibility.error:80,compatibility.stage);return;
                }
                if(!ClientInputCanBind(source))return;
                if(!local_server_input_warning_compat::Install(world,pc,pawn)){
                    const auto& compatibility=local_server_input_warning_compat::GetState();
                    Fail(compatibility.error?compatibility.error:82,compatibility.stage);return;
                }
                local_server_input::ObserveClientClock(pawn);
                local_server_empty_bootstrap_probe::ObserveClientClock(local_server_input::GetState().clientClock);
                ObserveClientPose(pawn);
                if(!ClientInputCanBind(source))return;
                if(!local_server_input_health::PollClient(world,pc,pawn)){
                    Fail(local_server_input_health::GetState().error,local_server_input_health::GetState().stage);return;
                }
                const auto observer=local_server_input_health::current;
                if(!local_server_ip_ack_compat::binding.active||!local_server_input_warning_compat::binding.active
                   ||!observer||!observer->active||observer->world!=world||observer->pc!=pc||observer->ps!=source.ps||observer->pawn!=pawn
                   ||!local_server_input_health::WeakMatch(*observer)){
                    stage="native_client_waiting_actual_current_input_observer_binding";return;}
                if(local_server_runtime::GetState().enabled){
                    if(!local_server_runtime::TickClient(delta,pc,source.ps,pawn))
                        Fail(local_server_runtime::GetState().error,local_server_runtime::GetState().stage);
                }else{
                    if(!local_server_client::GetState().initialized||local_server_client::GetState().pawn!=game.clientHero)
                        local_server_client::Initialize(world,pc,source.ps,pawn);
                    if(local_server_client::GetState().initialized)local_server_client::Poll(delta);
                }
                if(local_server_client::GetState().error)Fail(local_server_client::GetState().error,local_server_client::GetState().stage);
                if(error.load()||!ClientInputCanBind(source))return;
                clientInputReady=true;
                local_server_remote_render_probe::Poll(world,pc,pawn);
                local_server_remote_timeline_probe::Poll(world,pc,pawn);
                if(!error.load()&&local_server_runtime::GetState().enabled
                   &&(local_server_runtime::GetState().ready||local_server_native_test::GetState().bound)
                   &&!local_server_native_test::Tick(world,pc,source.ps,pawn,delta))
                    Fail(local_server_native_test::GetState().error,local_server_native_test::GetState().stage);
            }
        }
}
static void NetTick(void* value,float delta){
    originalNetTick(value,delta);++networkTickCalls;
    __try { NetTickImpl(value,delta); }
    __except(Exception(GetExceptionInformation())) { SaveState(); }
}
static void NetDispatch(void* value,float delta){
    // Native driver dispatch precedes the same world's actor/physics tick.
    // Observe genuine native progress across the whole frame before allowing
    // the missing NPC authority scheduler to own a fixed-step frame afterward.
    if(config.role==1&&actionCompleted.load()&&!cleaned.load()&&!error.load()
       &&local_server_runtime::GetState().enabled&&!local_server_runtime::BeforeWorldTick(delta))
        Fail(local_server_runtime::GetState().error?local_server_runtime::GetState().error:67,local_server_runtime::GetState().stage);
    originalNetDispatch(value,delta);
    if(config.role==2&&actionCompleted.load()&&!cleaned.load()){
        // Drain after the original dispatch has returned, before actor/input
        // processing can admit another observer. Never replay the client tick.
        __try{DrainClientInputLifecycle();}
        __except(Exception(GetExceptionInformation())){SaveState();}
    }
}
static void ArmClientNetTick(){
    if(observedNetDriver||(!actionCompleted.load()&&!(config.role==1&&hostCleanupPending.load())))return;
    auto world=Read<void*>(worldContext,0x2c8),pending=Read<void*>(worldContext,0x198);
    auto value=Read<void*>(world,0x40);if(!value)value=Read<void*>(pending,0x38);
    if(!value)return;
    auto table=Read<void**>(value);
    const uintptr_t sourceTable=config.reserved?0xacb4100:0xb0b5df0;
    const uintptr_t sourceTick=config.reserved?0x4733950:0x716ee90;
    const uintptr_t sourceDispatch=config.reserved?0x5118870:0x716e760;
    if(table!=reinterpret_cast<void**>(engine::image+sourceTable)
       ||Read<void*>(table,0x398)!=reinterpret_cast<void*>(engine::image+sourceTick)
       ||Read<void*>(table,0x388)!=reinterpret_cast<void*>(engine::image+sourceDispatch)
       ||Read<void*>(table,0x560)!=reinterpret_cast<void*>(engine::image+0x5117560)){
        if(config.role==1)hostCleanupPending=true;Fail(46,"native_client_driver_tick_table_profile_mismatch");return;
    }
    // Both exact current primary tables have 173 entries, no RTTI prefix.
    // Preserve Ares's complete statistics wrapper before its native base tail
    // call. Clone only this owned driver; SetWorld retains its tick lifetime.
    auto copy=static_cast<void**>(VirtualAlloc(nullptr,0x568,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!copy){if(config.role==1)hostCleanupPending=true;Fail(47,"native_client_driver_tick_table_allocation_failed");return;}
    std::memcpy(copy,table,0x568);originalNetTick=reinterpret_cast<NetTickFn>(copy[0x398/8]);
    originalNetDispatch=reinterpret_cast<NetTickFn>(copy[0x388/8]);
    copy[0x398/8]=reinterpret_cast<void*>(NetTick);copy[0x388/8]=reinterpret_cast<void*>(NetDispatch);DWORD old=0;
    if(!VirtualProtect(copy,0x568,PAGE_READONLY,&old)){if(config.role==1)hostCleanupPending=true;Fail(48,"native_client_driver_tick_table_protection_failed");return;}
    observedNetTable=copy;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void*volatile*>(value),copy,table)!=table){
        if(config.role==1)hostCleanupPending=true;Fail(49,"native_client_driver_tick_table_identity_changed");return;
    }
    observedNetDriver=value;
}
static DWORD WINAPI Observe(void*){
    while(true){
        Sleep(100);
        if(actionCompleted.load()||(config.role==1&&hostCleanupPending.load()))ArmClientNetTick();
        if(config.role==2&&actionCompleted.load())ScanClientPawns();
        SaveState();
    }
}
static DWORD WINAPI Initialize(void*){
    wchar_t path[32768]{};
    const DWORD count=GetModuleFileNameW(module,path,32768);if(!count||count>=32768)return 1;
    std::wstring base(path,count);base.resize(base.find_last_of(L"\\/")+1);
    const auto loadFile=[](const std::wstring& file,void* data,DWORD bytes){
        HANDLE h=CreateFileW(file.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(h==INVALID_HANDLE_VALUE)return false;
        DWORD got=0;const bool ok=ReadFile(h,data,bytes,&got,nullptr)&&got==bytes;CloseHandle(h);return ok;
    };
    const auto pid=std::to_wstring(GetCurrentProcessId());
    local_server_mode_startup::Options earlyOptions{};
    if(loadFile(base+L"mode-startup-"+pid+L".bin",&earlyOptions,sizeof(earlyOptions))){
        // Its explicit installer export binds POD config under the verified
        // startup hold. This host uses the actual Mode's native Tick; a dummy
        // DefaultPawn is neither required nor fabricated as a tick producer.
        if(earlyOptions.magic!=local_server_mode_startup::Magic||earlyOptions.pid!=GetCurrentProcessId())return 11;
        stage="native_mode_waiting_explicit_early_gi_factory_binding";return 0;
    }
    if(!loadFile(base+L"context-"+pid+L".bin",&context,sizeof(context))
       ||!loadFile(base+L"network-"+pid+L".bin",&config,sizeof(config)))return 2;
    if(context.magic!=0x3152544c41564f4cULL||context.version!=1||context.pid!=GetCurrentProcessId()
       ||context.image!=reinterpret_cast<uint64_t>(GetModuleHandleW(nullptr))
       ||config.magic!=0x3150524f5754454eULL||config.version!=1||config.role<1||config.role>2
       ||config.port<1024||config.port>65535||config.directory[1023])return 3;
    directory=config.directory;
    if(!OwnedSessionDirectory())return 4;
    engine::image=static_cast<uintptr_t>(context.image);startedAt=GetTickCount64();
    if(!local_server_peek_fixture::Init(directory.c_str(),config.role)){Fail(local_server_peek_fixture::context.error,local_server_peek_fixture::context.stage);SaveState();return 10;}
    if(!local_server_hold_fixture::Init(directory.c_str(),config.role)){Fail(local_server_hold_fixture::context.error,local_server_hold_fixture::context.stage);SaveState();return 10;}
    if(config.role==2&&!local_server_native_test::Init(directory.c_str())){
        Fail(local_server_native_test::GetState().error,local_server_native_test::GetState().stage);SaveState();return 10;
    }
    auto faultPage=static_cast<uint8_t*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!faultPage||!original_faults::Initialize(faultPage))return 9;
    auto object=reinterpret_cast<void***>(context.pawn);
    originalTable=reinterpret_cast<void**>(engine::image+0xab221a0);
    if(Read<void**>(object)!=originalTable||originalTable[0x540/8]!=reinterpret_cast<void*>(engine::image+0x3edab80)){
        Fail(1,"native_network_tick_host_mismatch");SaveState();return 5;
    }
    auto allocation=static_cast<void**>(VirtualAlloc(nullptr,288*sizeof(void*),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!allocation)return 6;
    std::memcpy(allocation,originalTable-1,288*sizeof(void*));privateTable=allocation+1;
    originalTick=reinterpret_cast<TickFn>(privateTable[0x540/8]);privateTable[0x540/8]=reinterpret_cast<void*>(Tick);
    DWORD old=0;if(!VirtualProtect(allocation,288*sizeof(void*),PAGE_READONLY,&old))return 7;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void*volatile*>(object),privateTable,originalTable)!=originalTable)return 8;
    stage="native_network_game_thread_callback_armed";SaveState();
    if(HANDLE thread=CreateThread(nullptr,0,Observe,nullptr,0,nullptr))CloseHandle(thread);
    return 0;
}
static bool startupRuntimeLoaded=false,startupObserverStarted=false;
static bool StartupLoadRuntime(){
    if(startupRuntimeLoaded)return true;
    local_server_runtime::Hooks value;value.spawnHuman=local_server_gameplay::SpawnHuman;value.isObserver=RuntimeObserver;
    value.requestStop=RuntimeStop;value.beforeRetire=RuntimeRetire;value.manageClientPresentation=true;value.port=config.port;
    if(!local_server_runtime::Load(directory,true)||!local_server_runtime::SetHooks(value)
       ||!local_server_runtime::GetState().enabled)return false;
    if(!local_server_gameplay::SetInitialSpawnProvider(local_server_runtime::PrepareInitialHumanSpawn))return false;
    startupRuntimeLoaded=true;return true;
}
static void StartupReport(){
    const auto path=local_server_mode_startup::options.report;if(!path[0])return;
    FILE* file=nullptr;if(!_wfopen_s(&file,path,L"wb")&&file){local_server_mode_startup::Write(file);std::fclose(file);}
}
static bool StartupObjects(const local_server_mode_startup::Source& source){
    const bool result=StartupLoadRuntime()&&local_server_gameplay::KickNativeStartup(source);StartupReport();return result;
}
static local_server_mode_startup::Preparation StartupPrepare(const local_server_mode_startup::Source& source){
    if(!StartupLoadRuntime())return local_server_mode_startup::Preparation::Failed;
    const auto result=local_server_gameplay::PrepareNativeStartup(source,local_server_runtime::ConfigureFactory);StartupReport();
    return result==local_server_gameplay::Preparation::Ready?local_server_mode_startup::Preparation::Ready:
        result==local_server_gameplay::Preparation::Waiting?local_server_mode_startup::Preparation::Waiting:local_server_mode_startup::Preparation::Failed;
}
static bool StartupStarted(const local_server_mode_startup::Source& source){
    if(!local_server_gameplay::AdoptNativeStartup(source))return false;
    context.world=reinterpret_cast<uint64_t>(source.world);context.controller=reinterpret_cast<uint64_t>(source.pc);context.pawn=0;
    gameInstance=source.gi;worldContext=Read<void*>(source.gi,0x38);owningEngine=Read<void*>(source.gi,0x28);
    bootstrapPlayerState=Read<void*>(source.pc,0x470);StartupReport();return true;
}
static void StartupTick(const local_server_mode_startup::Source&,float){
    __try{
        // Preserve the current bridge's error-independent bootstrap cleanup.
        // Actor Tick is still not a post-world driver destruction boundary.
        if(!ServiceBootstrapHostCleanup()&&!error.load())OnTick();
    }__except(Exception(GetExceptionInformation())){
        if(config.role==1&&driver.load()&&!cleaned.load())hostCleanupPending=true;
        SaveState();
    }
    if((actionCompleted.load()||hostCleanupPending.load())&&!startupObserverStarted){startupObserverStarted=true;
        if(HANDLE worker=CreateThread(nullptr,0,Observe,nullptr,0,nullptr))CloseHandle(worker);}
    static uint64_t nextReport=0;const auto now=GetTickCount64();if(now>=nextReport){
        nextReport=now+100;StartupReport();
        // Early listener/setup failures precede the ordinary observer thread.
        // Publish their real bridge error as well as the Mode lifecycle state.
        if(!startupObserverStarted)SaveState();
    }
}
static void StartupFailed(unsigned code,const char* message){Fail(code,message);StartupReport();}
extern "C" __declspec(dllexport) void* OfflineShooterModeCreateGameModeForURL(void* gi,void* url,void* world){
    return local_server_mode_startup::CreateGameModeForURL(gi,url,world);
}
extern "C" __declspec(dllexport) DWORD WINAPI OfflineShooterModeConfigure(void* parameter){
    local_server_mode_startup::Options input{};if(!original_players::Read(parameter,0,input))return 8713;
    if(input.magic!=local_server_mode_startup::Magic||input.pid!=GetCurrentProcessId()||input.host!=1)return 8713;
    wchar_t filename[32768]{};const auto n=GetModuleFileNameW(module,filename,32768);if(!n||n>=32768)return 8713;
    std::wstring file(filename,n);file.resize(file.find_last_of(L"\\/")+1);file+=L"network-"+std::to_wstring(input.pid)+L".bin";
    HANDLE h=CreateFileW(file.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h==INVALID_HANDLE_VALUE)return 8713;DWORD got=0;Configuration loaded{};
    const bool read=ReadFile(h,&loaded,sizeof(loaded),&got,nullptr)&&got==sizeof(loaded);CloseHandle(h);
    if(!read||loaded.magic!=0x3150524f5754454eULL||loaded.version!=1||loaded.role!=1
       ||loaded.port<1024||loaded.port>65535||loaded.directory[1023])return 8713;
    config=loaded;directory=config.directory;if(!OwnedSessionDirectory())return 8713;
    if(input.report[1023]||!local_server_package_paths::ModeStartupReport(directory,input.report))return 8714;
    // The original-host GI path returns before the ordinary worker Initialize.
    // This only validates explicit fixture files; Arm waits for the real GT.
    if(!local_server_peek_fixture::Init(directory.c_str(),config.role))return local_server_peek_fixture::context.error?local_server_peek_fixture::context.error:8713;
    if(!local_server_hold_fixture::Init(directory.c_str(),config.role))return local_server_hold_fixture::context.error?local_server_hold_fixture::context.error:8713;
    context={0x3152544c41564f4cULL,1,input.pid,input.image,0,0,0,0};startedAt=GetTickCount64();
    local_server_mode_startup::Hooks callbacks;callbacks.beforeInit=StartupObjects;callbacks.preparePlay=StartupPrepare;
    callbacks.started=StartupStarted;callbacks.tick=StartupTick;callbacks.failed=StartupFailed;
    if(!local_server_mode_startup::Configure(input,callbacks))return 8713;
    auto faultPage=static_cast<uint8_t*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!faultPage||!original_faults::Initialize(faultPage))return 8713;
    stage="native_mode_early_gi_factory_configured_without_unreal_calls";StartupReport();return 0;
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID){
    if(reason==DLL_PROCESS_ATTACH){module=instance;DisableThreadLibraryCalls(instance);
        if(HANDLE thread=CreateThread(nullptr,0,Initialize,nullptr,0,nullptr))CloseHandle(thread);
    }
    return TRUE;
}
