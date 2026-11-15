#pragma once
#include "local_server_match.h"
#include "local_server_abilities.h"
#include "local_server_hero_abilities.h"
#include "local_server_input.h"
#include "original_damage_tuning.h"
#include "original_inventory_slots.h"
#include "armor_poll_performance.h"
#include "humanlike_bot_native_events.h"
#include "humanlike_bot_point_impacts.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <intrin.h>
#include <limits>
#include <string>
#include <type_traits>

// Independent authority-host combat candidate, pinned to Shipping SHA-256
// 147763350504d241ee20b77607d04bc85cebfedb1d8bc2f32473ca93bf8fe92f.
// The observer/trace algorithms below are adapted from the retained standalone
// sources; their ownership, input, transport and match entry points are separate.
// No local-player adoption, role mutation, synthetic shot, spread/recoil model,
// image/CDO edit or client-side damage simulation is performed by this header.
// Static source/ABI evidence and unresolved runtime gates:
// work/local-server-playable/combat/README.md.
namespace local_server_combat {
using Weak=original_inventory_slots::WeakIdentity;
inline constexpr size_t MaxPlayers=original_players::MaxPlayers;
struct ReceivePolicy {
    bool profilesVerified=false,valueKnown=false,functionKnown=false,clientWorldObserved=false;
    bool delayUnmapped=false,multicastReliable=false;
    uint32_t processId=0,multicastFlags=0;int32_t delayUnmappedValue=0;
    uint64_t sampledAt=0;
    void *world=nullptr,*driver=nullptr,*serverConnection=nullptr,*consoleData=nullptr,*multicastFunction=nullptr;
    const char* stage="native_rpc_policy_not_sampled";
};
struct ReferenceDiagnosis {
    bool sampled=false,identityValid=false,virtualsKnown=false,stableName=false,supportsNetworking=false;
    bool packageMapKnown=false,existingGuidKnown=false,guidFound=false,nativeSupportsObject=false,guidWouldBeZero=false;
    bool replicates=false,onlyRelevantToOwner=false;
    uint8_t role=255,remoteRole=255;int32_t mapIndex=-1;
    uint64_t existingGuid=0,sampledAt=0;
    void *controller=nullptr,*connection=nullptr,*packageMap=nullptr,*guidCache=nullptr;
    Weak controllerId{};
    const char* stage="native_rpc_reference_not_sampled";
};
struct Registration {
    bool present=false,human=false;uint32_t epoch=0;uint64_t generation=0;
    void *pc=nullptr,*ps=nullptr,*pawn=nullptr,*ammoReplenisher=nullptr;
    Weak pcId{},psId{},pawnId{},replenisherId{};
    bool refillOnBind=false;
    ReferenceDiagnosis referenceDiagnosis;
};
struct NPCSubmission {
    uint64_t sequence=0,binding=0;uint32_t index=0,epoch=0,desired=0,changed=0;
    int32_t beforeBase=-1,beforeCount=0,beforeApplied=-1,afterBase=-1,afterCount=0,afterApplied=-1,built=0;
    bool bufferConsumed=false,historyMatches=false,sourceChanged=false;
    void *beforeCurrent=nullptr,*afterCurrent=nullptr;int32_t beforeAuthority=-1,afterAuthority=-1;
    const char* outcome="not_submitted";
};
struct State {
    bool initialized=false,ready=false,restartRequired=false,processing=false,profilesVerified=false;
    bool ammoPolicyKnown=false,infiniteAmmo=false,invincible=false,replicationFault=false;
    bool nativeGunObserved=false,nativeMeleeObserved=false,nativeDaggerObserved=false;
    bool rpcReferencesKnown=false,rpcReferenceGateReady=false;
    bool replicaPolicyVerified=false;
    unsigned error=0,registeredPlayers=0,activeSources=0,physicsReadySources=0;
    uint32_t configurationRevision=0;
    uint64_t polls=0,registrations=0,weaponBinds=0,retirements=0;
    uint64_t damageEvents=0,kills=0,damageRemoteCalls=0,damageRemoteAccepted=0,damageRemoteRejected=0;
    uint64_t typedPreflightQueries=0,typedPreflightRejected=0,typedDamageEvents=0,typedKills=0;
    uint64_t typedRemoteCalls=0,typedRemoteAccepted=0,typedRemoteRejected=0;
    uint64_t ammoRuleWrites=0,ammoRuleReadbacks=0,ammoRefills=0,healRequests=0,heals=0,healsDeferred=0;
    uint64_t daggerPoints=0,daggerQueued=0,daggerDuplicates=0,daggerInvalid=0,daggerStale=0;
    uint64_t daggerNoBaseline=0,daggerNativeChanged=0,daggerDamage=0,daggerKills=0;
    uint64_t primaryDaggerRefills=0;
    uint64_t foreignControllerHitsRejected=0,foreignReferenceRejected=0;
    uint64_t referenceDiagnosticSamples=0;
    ReceivePolicy replicaRPCPolicy;
    uint32_t replicaProcessId=0;
    void* replicaPolicyConnection=nullptr;Weak replicaPolicyConnectionId{};
    uint64_t npcInputRejected=0;
    NPCSubmission npcSubmission;
    int netMode=-1,lastSource=-1,lastVictim=-1;uint64_t lastToken=0;
    uint8_t lastRegion=255;float lastDamage=0;
    double lastPoll=-1;uintptr_t exceptionRva=0;
    void *world=nullptr,*gameState=nullptr,*lastDamageResponse=nullptr;
    void* lastBlockedObject=nullptr;
    const char* lastBlockedReference="none";
    const char* stage="server_combat_not_initialized";
    const char* operation="none";
};
inline State state;
inline void* damageMulticastFunction=nullptr;
inline std::array<Registration,MaxPlayers> registrations{};
inline uint64_t nextRegistration=0;
inline DWORD gameThread=0;
inline void CacheDiagnostics();
// Root supplies a read-only admission check for its real NPC authority
// scheduler. It must validate the registered weak identities and spawn epoch
// of this exact PC/PS/Pawn/AresInputState tuple, without requiring LocalPlayer
// or native IsLocallyControlled. This does not certify weapon action history.
using ExternalNPCProducer=bool(*)(size_t,void*,void*,void*,uint32_t,void*);
inline ExternalNPCProducer externalNPCProducer=nullptr;
// The NPC scheduler owns any mapped native clock. It certifies this exact
// current life and original clock; an arbitrary nonzero offset is not admitted.
struct ExternalNPCClockSource {
    size_t index=0;uint32_t epoch=0;
    void *pc=nullptr,*ps=nullptr,*pawn=nullptr,*inputState=nullptr;
    void *movement=nullptr,*prediction=nullptr,*clock=nullptr;
    int32_t baseOffset=0,offset=0;uint32_t step=0;
};
using ExternalNPCClockValidator=bool(*)(const ExternalNPCClockSource&);
inline ExternalNPCClockValidator externalNPCClockValidator=nullptr;
inline bool SetExternalNPCClockValidator(ExternalNPCClockValidator value){
    if(!value||(state.initialized&&GetCurrentThreadId()!=gameThread)
       ||(externalNPCClockValidator&&externalNPCClockValidator!=value))return false;
    externalNPCClockValidator=value;return true;
}
// A separately owned NPC Pawn layer must retire before this module restores
// its lower native view table. Null leaves the original gun path unchanged.
using PawnRetirementObserver=bool(*)(size_t,void*,uint32_t);
inline PawnRetirementObserver pawnRetirementObserver=nullptr;
inline bool SetPawnRetirementObserver(PawnRetirementObserver value){
    if(!value||(state.initialized&&GetCurrentThreadId()!=gameThread)
       ||(pawnRetirementObserver&&pawnRetirementObserver!=value))return false;
    pawnRetirementObserver=value;return true;
}
inline bool ClearPawnRetirementObserver(PawnRetirementObserver value){
    if((state.initialized&&GetCurrentThreadId()!=gameThread)||pawnRetirementObserver!=value)return false;
    pawnRetirementObserver=nullptr;return true;
}
inline void SetExternalNPCProducer(ExternalNPCProducer producer){externalNPCProducer=producer;}
inline const State& GetState(){return state;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Identity(void* object,Weak& identity){return original_inventory_slots::CaptureIdentity(object,identity)&&Live(object,identity);}
inline bool Fail(unsigned error,const char* stage,bool terminal=false){
    state.error=error?error:4599;state.stage=stage;state.ready=false;if(terminal)state.restartRequired=true;return false;
}
inline bool Thread(){return state.initialized&&!state.restartRequired&&GetCurrentThreadId()==gameThread&&original_players::Thread();}
inline double Now(){return static_cast<double>(GetTickCount64())/1000.;}
inline bool ReplicaPolicyAllowsNullableController();
inline bool RemoteSourceAllowed(size_t index){
    const auto p=local_server_match::Player(index);if(!p)return false;
    // EventInstigator is an actual AController object property. The inspected
    // original struct has no custom serializer that omits it. The owner PC is
    // exported through native login; foreign/NPC PCs must remain host-only.
    // Until an original per-connection producer/encoding is established, do
    // not fabricate null and do not apply a host-only NPC damage result.
    if(p->human){
        const auto driver=Read<void*>(local_server_match::GetState().world,0x40);
        const auto peers=Read<void**>(driver,0xc8);
        if(Read<int32_t>(driver,0xd0)==1&&Read<void*>(peers,0)==Read<void*>(p->pc,0x510))return true;
        state.lastBlockedReference="Point.EventInstigator_multi_peer_owner_scope";
        state.lastBlockedObject=p->pc;++state.foreignControllerHitsRejected;return false;
    }
    // The stock receiver can execute an RPC with decoder-produced null object
    // parameters when its actual unmapped-delay policy does not queue it. This
    // is enabled only from Root's verified, current paired-client observation;
    // no sender-side pointer or console variable is changed.
    if(ReplicaPolicyAllowsNullableController())return true;
    ++state.foreignControllerHitsRejected;state.lastBlockedReference="Point.EventInstigator";
    state.lastBlockedObject=p->pc;return false;
}

namespace policy {
inline constexpr size_t MaxPlayers=local_server_combat::MaxPlayers;
using Phase=deathmatch_rules::Phase;
struct Participant {
    bool registered=false,ready=false,alive=false,local=false;
    uint32_t spawnEpoch=0;Phase phase=Phase::Absent;
    void *pawn=nullptr,*controller=nullptr,*playerState=nullptr;
};
inline std::array<Participant,MaxPlayers> snapshots{};
inline void*& world=local_server_match::state.world;
inline const auto& State(){return local_server_match::GetState();}
inline const Participant* Player(size_t index){
    const auto p=local_server_match::Player(index);if(!p||index>=snapshots.size())return nullptr;
    auto& v=snapshots[index];v={p->present,p->ready,p->alive,p->human,p->epoch,p->phase,p->pawn,p->pc,p->ps};return &v;
}
inline bool Authority(size_t index,void* pawn,void* pc){
    const auto p=local_server_match::Player(index);
    if(!p||!p->present||p->pawn!=pawn||p->pc!=pc||!p->epoch
       ||!local_server_match::AuthorityPair(*p)||Read<uint8_t>(p->ps,0x224)!=3
       ||Read<void*>(p->pawn,0x4a0)!=p->pc||Read<void*>(p->pc,0x4b0)!=p->pawn)return false;
    auto driver=Read<void*>(local_server_match::GetState().world,0x40);
    if(!driver||Read<void*>(driver,0xc0))return false;
    if(!p->human)return Read<void*>(pc,0x510)==nullptr;
    auto connection=Read<void*>(pc,0x510);
    return connection&&Read<void*>(connection,0xa0)==pc&&Read<void*>(connection,0x38)==pc
        &&Read<void*>(connection,0x60)==driver&&Read<uint8_t>(connection,0x13c)==3;
}
inline bool NPC(size_t index){const auto p=local_server_match::Player(index);return p&&!p->human&&Authority(index,p->pawn,p->pc);}
inline bool Active(){return local_server_match::Active();}
inline bool BotFireAllowed(){return local_server_match::FreeForAll()||(local_server_match::Configuration().config.flags&training_control::ReturnFire)!=0;}
inline bool AbilitiesAllowed(){return local_server_match::AbilitiesAllowed();}
inline bool CanDamage(size_t source,size_t victim){
    const auto s=local_server_match::Player(source),v=local_server_match::Player(victim);
    return !state.replicationFault&&s&&v&&Authority(source,s->pawn,s->pc)&&Authority(victim,v->pawn,v->pc)
        &&local_server_match::CanDamage(source,victim)&&RemoteSourceAllowed(source);
}
inline bool BeginNativeDamage(uint64_t token,size_t source,size_t victim,void* equippable,double now,original_damage::DeathData& death){
    return CanDamage(source,victim)&&local_server_match::BeginNativeDamage(token,source,victim,equippable,now,death);
}
inline void CancelNativeDamage(){local_server_match::CancelNativeDamage();}
inline bool EndNativeDamage(bool okay,const original_damage::Result& result){
    namespace events=humanlike_bot_native_events;
    const auto pending=local_server_match::pending;
    const auto s=local_server_match::Player(pending.source),v=local_server_match::Player(pending.victim);
    events::Subject source{},victim{};
    if(pending.active&&s&&v){
        source={static_cast<uint32_t>(s->index),s->epoch,reinterpret_cast<uintptr_t>(s->pawn),s->pawnIdentity.index,s->pawnIdentity.serial};
        victim={static_cast<uint32_t>(v->index),v->epoch,reinterpret_cast<uintptr_t>(v->pawn),v->pawnIdentity.index,v->pawnIdentity.serial};
    }
    const auto direction=events::TakeDirection(pending.token,victim.index,victim.epoch);
    // Forward exactly once. Native result, life and score commit remain owners.
    const bool committed=local_server_match::EndNativeDamage(okay,result);
    if(committed&&okay&&pending.active&&result.applied&&!result.duplicate
       &&result.originalResponse&&result.originalLifeReadback&&result.response){
        events::PublishDamage(pending.token,static_cast<double>(GetTickCount64())/1000.,victim,source,
            result.healthBefore,result.healthAfter,result.shieldBefore,result.shieldAfter,result.killed,direction);
    }
    return committed;
}
}

inline bool ExternalNPCProducerOwns(size_t index,void* pc,void* pawn,uint32_t epoch,void* inputState){
    const auto p=local_server_match::Player(index);
    return externalNPCProducer&&p&&p->present&&!p->human&&p->pc==pc&&p->pawn==pawn&&p->epoch==epoch
        &&policy::Authority(index,pawn,pc)&&Read<uint8_t>(pawn,0x128)==2
        &&original_players::IsA(inputState,0x6494370)&&Read<void*>(pawn,0xba0)==inputState
        &&Read<void*>(inputState,0xc8)==pawn&&Read<void*>(inputState,0x148)==pawn
        &&Read<void*>(inputState,0)==engine::function<void*>(0xb093c10)
        &&externalNPCProducer(index,pc,p->ps,pawn,epoch,inputState);
}

struct NativeProfile {uint32_t rva,bytes;uint64_t fnv;};
inline constexpr NativeProfile Profiles[]{
    {0x412c380,1629,0x8aa469c170a744e8ULL},
    {0x412c9e0,1889,0xc600141e795da53cULL},
    {0x1cdc850,1396,0x74e72e0971f89c0dULL},
    {0x6f9ce10,338,0x6105a9c9175df88dULL},
    {0x6f9fe30,828,0xc397f4d5c6c3f4edULL},
    {0x6fa05a0,111,0xd99052c53d1e419fULL},
    {0x6fa0940,492,0x87133ef1d39da2baULL},
    {0x7130ce0,825,0x8488df38677eaebdULL},
    {0x711f032,3,0x4abbb919cecef0ffULL},
    {0x712279c,3,0x4abbb919cecef0ffULL},
    {0x70a7110,112,0x1045790bb6ef0d0cULL},
    {0x70a5420,32,0xc3bf080ee2479603ULL},
    {0x70aa150,169,0xa68f4cc876ab8d50ULL},
    {0x6ef0e80,129,0x9a241410bc7b5007ULL},
    {0x6ef0f60,24,0xc7b53c02abb7879eULL},
    {0x787f820,32,0xfc7775ab82f59d46ULL},
    {0x6f56010,32,0xcc07555e8b23e95dULL},
    {0x78c6d60,32,0x9160b8db2d6aac7ULL},
    {0x42c8060,32,0x1cb058789e205027ULL},
    {0x4777940,1318,0x880d9a2c084a14c2ULL},
    {0x4782920,1749,0x7a0f883c0717922aULL},
    {0x4782750,368,0x5496f5f93ae5d4deULL},
    {0x1b17ba0,11,0x95260eb936a5767dULL},
    {0x670a580,59,0x6ad8f7d7cac08be7ULL},
    {0x670a540,59,0xb383ac02dde04cdfULL},
    {0x6709f70,88,0x3c698bca8d22c9b8ULL},
    {0x670a2f0,88,0xe184c44b13b7d14ULL},
    {0x1d5cd00,25,0xcd85ecf311c5f386ULL},
    {0x1c01cf0,165,0xaedeca75a5887cdbULL},
    {0x1bfd870,60,0x64f79e164cd0f735ULL},
    {0x1c5dc90,69,0xac872c8e46bd3f12ULL},
    {0x18a3ec0,3,0xa3703b1a918cfbebULL},
    {0xae1260,139,0x254a76a2cac6c027ULL},
    {0x42ea400,2049,0x1c439ae375763620ULL},
    {0x42ead30,4395,0xbe00cf6fab54df38ULL},
    {0x6707790,47,0xf818047bc2dbd654ULL},
    {0x6f9cf70,324,0xc4d2c5b25d97a722ULL},
    {0x7881e80,1033,0x8b9c2c0cc07350bdULL},
    {0x78802a0,102,0x3937a8989c1aa9ccULL},
    {0x42ad260,27,0x3a03bd4d80195a21ULL},
    {0x454e130,368,0x396e359a9fc02453ULL},
    {0x70aa200,37,0x1067dd10c736edaeULL},
    {0x70a9b60,622,0xe31b79d1aac5930aULL},
    {0x70a7180,432,0x44822be53b5cae46ULL},
    {0x70a80b0,770,0xe97523ade31fab94ULL},
    {0x70a74e0,3012,0xb8866d557299ab54ULL},
    {0x70a8490,857,0xcd5f574e2bc06315ULL},
    {0x7892e10,142,0x773b71bd06054ce5ULL},
    {0x789df70,144,0x59af83ea9f307d53ULL},
    {0x70a33a0,218,0x81063b5beef820ULL},
    {0x70a4530,1859,0x6256e5a01ed4f880ULL},
    {0x707be70,488,0x100f5e35b9de0559ULL},
    {0x6850340,322,0xb91e49baedef88ddULL},
    {0x68506a0,473,0x7f27e11d210aa060ULL},
    {0x68504c0,475,0x5d53242564f56793ULL},
    {0x7122140,1374,0x8189e5f9d5454733ULL},
    {0x71226a0,106,0x9267c8603320a73cULL},
    {0x6ff68d0,77,0x8971fb2c88a16baULL},
    {0x70fbcc0,196,0xa0c34de8e998640dULL},
    {0x711efc0,5225,0x4867deee44e911c1ULL},
    {0x7120d80,4213,0xaf7f70b0fab882dbULL},
    {0x7120820,1363,0x2294776a9390f796ULL},
    {0x710cd10,203,0x2602df41e1706c26ULL},
    {0x1cf69a0,443,0x8399700ac50f679eULL},
    {0x684f930,63,0xe8a85fcb56f46681ULL},
};
inline bool VerifyProfiles(){
    for(const auto& profile:Profiles){
        const auto raw=reinterpret_cast<const uint8_t*>(engine::image+profile.rva);uint64_t hash=14695981039346656037ULL;
        for(uint32_t n=0;n<profile.bytes;++n)hash=(hash^raw[n])*1099511628211ULL;
        if(hash!=profile.fnv)return false;
    }return true;
}
inline bool ReadRPCPolicy(void* world,ReceivePolicy& out){
    out={};out.processId=GetCurrentProcessId();out.sampledAt=GetTickCount64();out.world=world;
    if(!engine::image){out.stage="native_rpc_policy_image_unbound";return false;}
    // Pure reads: this API is valid for Root's explicit client snapshot too.
    // It creates no UFunction, console variable, actor or network GUID.
    out.profilesVerified=VerifyProfiles();
    if(!out.profilesVerified){out.stage="native_rpc_policy_profile_mismatch";return false;}
    out.consoleData=Read<void*>(reinterpret_cast<void*>(engine::image),0xd7e5a60);
    if(!original_players::Readable(out.consoleData,sizeof(int32_t))){out.stage="native_rpc_policy_console_data_uninitialized";return false;}
    out.delayUnmappedValue=Read<int32_t>(out.consoleData,0);out.delayUnmapped=out.delayUnmappedValue>0;out.valueKnown=true;
    // Original 6707790's native cache; unlike calling its constructor accessor,
    // reading it cannot construct reflection state on a snapshot worker.
    out.multicastFunction=Read<void*>(reinterpret_cast<void*>(engine::image),0xda7d508);
    if(out.multicastFunction&&Read<uint16_t>(out.multicastFunction,0xbe)==0x148){
        out.multicastFlags=Read<uint32_t>(out.multicastFunction,0xb8);
        out.functionKnown=(out.multicastFlags&0x4440)==0x4440;
        out.multicastReliable=(out.multicastFlags&0x80)!=0;
    }
    out.driver=Read<void*>(world,0x40);out.serverConnection=Read<void*>(out.driver,0xc0);
    out.clientWorldObserved=world&&out.driver&&out.serverConnection&&Read<void*>(out.driver,0x178)==world;
    out.stage=out.clientWorldObserved?"native_client_rpc_policy_observed":"native_rpc_policy_value_observed_world_role_unconfirmed";
    return true;
}
inline bool ReadRPCPolicy(ReceivePolicy& out){return ReadRPCPolicy(original_players::world,out);}
inline bool ReplicaPolicyAllowsNullableController(){
    if(!Thread()||!state.replicaPolicyVerified||!Live(state.replicaPolicyConnection,state.replicaPolicyConnectionId))return false;
    const auto& observation=state.replicaRPCPolicy;const auto now=GetTickCount64();
    if(now<observation.sampledAt||now-observation.sampledAt>2500||!observation.clientWorldObserved
       ||!observation.functionKnown||!observation.profilesVerified||!observation.valueKnown)return false;
    const auto flags=Read<uint32_t>(damageMulticastFunction,0xb8);
    if((flags&0x4440)!=0x4440||(flags&0x80)!=(observation.multicastFlags&0x80))return false;
    if(observation.multicastReliable&&observation.delayUnmapped)return false;
    const auto human=local_server_match::Player(0);const auto driver=Read<void*>(state.world,0x40);
    const auto peers=Read<void**>(driver,0xc8);
    return human&&human->human&&Read<void*>(human->pc,0x510)==state.replicaPolicyConnection
        &&Read<int32_t>(driver,0xd0)==1&&Read<void*>(peers,0)==state.replicaPolicyConnection
        &&Read<void*>(state.replicaPolicyConnection,0xa0)==human->pc;
}
inline bool ObserveReplicaRPCPolicy(void* connection,uint32_t expectedClientPid,const ReceivePolicy& observation){
    // Root-only trust boundary: the caller MUST have validated its existing
    // local pair nonce/session and the actual owned client PID before forwarding
    // this native snapshot. This is not a reflected/network RPC and accepts no
    // hit, health, damage, score or actor-state claims from a client.
    if(!Thread()||!expectedClientPid||expectedClientPid==GetCurrentProcessId()
       ||observation.processId!=expectedClientPid||!observation.clientWorldObserved
       ||!observation.profilesVerified||!observation.valueKnown||!observation.functionKnown)return false;
    const auto now=GetTickCount64();
    if(now<observation.sampledAt||now-observation.sampledAt>2500)return false;
    const auto human=local_server_match::Player(0);const auto driver=Read<void*>(state.world,0x40);
    if(!human||!human->human||Read<void*>(human->pc,0x510)!=connection||Read<void*>(connection,0x60)!=driver
       ||Read<void*>(connection,0xa0)!=human->pc||!Identity(connection,state.replicaPolicyConnectionId))return false;
    state.replicaRPCPolicy=observation;state.replicaProcessId=expectedClientPid;
    state.replicaPolicyConnection=connection;state.replicaPolicyVerified=true;return true;
}
struct DamageMulticast {original_damage::PointData point;original_damage::DeathData death;};
static_assert(sizeof(DamageMulticast)==0x148&&offsetof(DamageMulticast,death)==0x110);
inline bool BlockReference(const char* field,void* object){
    ++state.foreignReferenceRejected;state.lastBlockedReference=field;state.lastBlockedObject=object;return false;
}
inline bool ReferencePayload(size_t victim,size_t source,const DamageMulticast& q){
    const auto s=local_server_match::Player(source),v=local_server_match::Player(victim);
    if(!s||!v||!RemoteSourceAllowed(source))return false;
    if(q.point.base.instigator!=s->pc)return BlockReference("Point.EventInstigator",q.point.base.instigator);
    if(q.point.base.instigatorPawn!=s->pawn)return BlockReference("Point.EventInstigatorPawn",q.point.base.instigatorPawn);
    if(q.point.base.damagerState!=s->ps)return BlockReference("Point.DamagerPlayerState",q.point.base.damagerState);
    if(q.point.base.killCreditState!=s->ps)return BlockReference("Point.KillCreditPlayerState",q.point.base.killCreditState);
    // These two fields are populated by the constrained original ApplyPoint
    // adapter from the actual exported weapon/ability Actor, never a projectile
    // or an owner-exclusive player-info Actor.
    if(q.point.base.causer!=q.point.base.equippable)return BlockReference("Point.DamageCauser",q.point.base.causer);
    if(!original_players::IsA(q.point.base.equippable,0x645c660)||Read<void*>(q.point.base.equippable,0x4a8)!=s->pawn)
        return BlockReference("Point.EquippableUsed",q.point.base.equippable);
    const auto& target=original_damage::targets[victim];
    if(q.point.hitComponent!=target.core)return BlockReference("Point.DamagedComponent",q.point.hitComponent);
    for(int32_t n=0;n<q.point.base.changes.count;++n){auto section=q.point.base.changes.data[n].section;
        if(section!=target.healthSection&&section!=target.shieldSection)return BlockReference("Point.LifeChangeEvents.ChangedComponent",section);
    }
    // The nested NetworkedMovementTimestamp.Character is the native weak
    // ShooterCharacter reference. Its host weak identity must resolve to the
    // very victim Pawn whose epoch/timestamp produced this notification.
    const auto& movement=q.point.base.timestamp.movement;
    const Weak character{movement.weakIndex,movement.weakSerial};
    auto timestampPawn=original_inventory_slots::ResolveIdentity(character);
    if(timestampPawn!=v->pawn)return BlockReference("Point.LifeChangeTimestamp.MovementTimestamp.Character",timestampPawn);
    if(q.death.assists.count!=0)return BlockReference("Death.AssistsList_requires_separate_native_schema",q.death.assists.data);
    if(q.death.effectOverride)return BlockReference("Death.DeathMontageEffectOverride_requires_original_producer",q.death.effectOverride);
    if(q.death.overrideContext)return BlockReference("Death.DeathMontageEffectOverrideContext",q.death.overrideContext);
    if(q.death.montage&&!original_players::IsA(q.death.montage,0x3f19390))return BlockReference("Death.DeathAnimMontage",q.death.montage);
    void* objects[]{q.point.base.causer,q.point.base.equippable,q.point.base.damageType,
        q.point.base.instigatorPawn,q.point.base.damagerState,q.point.base.killCreditState,q.point.hitComponent,q.death.montage};
    for(auto object:objects)if(object&&(original_players::IsA(object,0x3ed67a0)
        ||original_players::IsA(object,0x64f4540)))return BlockReference("Point_or_Death.foreign_controller_or_owner_exclusive_info",object);
    return true;
}
inline bool RemoteDamage(size_t victim,size_t source,const original_damage::Result& result,const original_damage::DeathData& death){
    if(!result.applied||result.duplicate)return true;
    const auto s=local_server_match::Player(source),v=local_server_match::Player(victim);
    if(!s||!v||!policy::Authority(source,s->pawn,s->pc)||!policy::Authority(victim,v->pawn,v->pc)
       ||!result.originalResponse||!result.originalLifeReadback||!original_damage::IsA(result.response,original_damage::PointResponseClass))return false;
    const auto& target=original_damage::targets[victim];
    DamageMulticast q{};q.point=Read<original_damage::PointData>(result.response,0xe8);q.death=death;
    if(result.killed)q.death.montage=target.lastMontage;
    if(q.point.base.instigatorPawn!=s->pawn||q.point.base.damagerState!=s->ps
       ||q.point.hitComponent!=target.core||q.point.base.timestamp.eventIndex!=target.lastNativeEventIndex
       ||(q.point.base.killed!=0)!=result.killed||q.point.base.changes.count<1||q.point.base.changes.count>2
       ||q.point.base.changes.capacity<q.point.base.changes.count
       ||!original_players::Readable(q.point.base.changes.data,static_cast<size_t>(q.point.base.changes.count)*sizeof(original_damage::LifeChange)))return false;
    if(!ReferencePayload(victim,source,q))return false;
    auto table=Read<void**>(target.damageable,0);
    using Callspace=int(*)(void*,void*,void*);
    using Remote=bool(*)(void*,void*,void*,void*,void*);
    const auto callspace=Read<Callspace>(table,0x2a0);
    const auto remote=Read<Remote>(table,0x2a8);
    if(callspace!=engine::function<Callspace>(0x412c380)||remote!=engine::function<Remote>(0x412c9e0)
       ||!(callspace(target.damageable,damageMulticastFunction,nullptr)&1))return false;
    ++state.damageRemoteCalls;
    // ProcessEvent1cdc850 uses this exact v2a8(this,UFunction,Parms,null,null)
    // when Callspace includes Remote. Local native life has ALREADY changed.
    // Sending only through this native remote half cannot apply a second host
    // notification. Return true means queued to native transport, not rendered.
    if(!remote(target.damageable,damageMulticastFunction,&q,nullptr,nullptr))return false;
    ++state.damageRemoteAccepted;return true;
}
inline bool ApplyAuthorityPoint(size_t victim,void* sourcePawn,void* equippable,void* damageType,
    const original_shots::Shot& shot,const original_damage::Hit& hit,float amount,uint64_t token,
    original_damage::Result& result,const original_damage::DeathData* death=nullptr,float falloff=1){
    if(!Thread()||victim>=MaxPlayers||!local_server_match::pending.active
       ||local_server_match::pending.token!=token||local_server_match::pending.victim!=victim)return false;
    const auto source=local_server_match::pending.source;
    const auto s=local_server_match::Player(source),v=local_server_match::Player(victim);
    if(!s||!v||s->pawn!=sourcePawn||!policy::CanDamage(source,victim)
       ||!policy::Authority(victim,v->pawn,v->pc)||Read<uint8_t>(equippable,0x224)!=3)return false;
    const bool okay=original_damage::ApplyPoint(victim,sourcePawn,equippable,damageType,shot,hit,amount,token,result,death,falloff);
    if(!okay)return false;
    if(result.applied&&!result.duplicate){
        humanlike_bot_native_events::StageDirection(token,static_cast<uint32_t>(victim),v->epoch,
            {static_cast<float>(shot.direction.x),static_cast<float>(shot.direction.y),static_cast<float>(shot.direction.z)});
        ++state.damageEvents;if(result.killed)++state.kills;
        state.lastSource=static_cast<int>(source);state.lastVictim=static_cast<int>(victim);state.lastToken=token;
        state.lastDamage=amount;state.lastRegion=hit.region;state.lastDamageResponse=result.response;
        const auto nativeDeath=death?*death:original_damage::DeathData{};
        if(!RemoteDamage(victim,source,result,nativeDeath)){
            ++state.damageRemoteRejected;state.replicationFault=true;
            // Preserve the successful native host result so EndNativeDamage
            // still closes the transaction and commits one actual death. Poll
            // subsequently stops instead of replaying this same damage.
        }
    }return true;
}
inline bool BeforeNativeDeath(size_t index);
inline uint64_t AllocateDamageToken();

namespace firearms {
inline constexpr size_t MaxPlayers=32,MaxEffects=128,MaxOrigins=64,MaxFiringStates=16,MaxEffectsPerState=64;
inline constexpr size_t MaxVtableFunctions=512,QueueCapacity=512,RetiredCapacity=1024;
inline constexpr size_t MaxEffectObservers=MaxPlayers*MaxEffects,EffectIndexCapacity=MaxEffectObservers*2;
static_assert(original_players::MaxPlayers==MaxPlayers&&original_damage::MaxTargets>=MaxPlayers);
using Weak=original_inventory_slots::WeakIdentity;
using NativeView=engine::Vec*(*)(void*,engine::Vec*);
using NativeEvent=void(*)(void*,void*,void*);
inline void ObserveEffectEvent(void* object,void* function,void* parameters);
inline void ObserveFiringEvent(void* object,void* function,void* parameters);
struct Options {
    double maximumDistance=100000,maximumShotAge=.30,obstructionTolerance=.5,sampleInterval=.04;
    size_t maximumShotsPerPoll=32;
};
inline Options options;
struct Vtable {
    void** previous=nullptr;
    size_t functions=0;
    bool installed=false;
    std::array<void*,MaxVtableFunctions+1> copy{};
    void** Address(){return copy.data()+1;}
    void* const* Address() const{return copy.data()+1;}
};
struct Effect {
    void* object=nullptr;Weak identity{};
    void* firing=nullptr;uint32_t firingSlot=0;
    Vtable table;
    NativeEvent forward=nullptr;
    uint64_t occurrence=0,initialCapturedOccurrence=0;
    uint64_t generation=0,ownerBinding=0;
    size_t ownerSource=MaxPlayers;
    bool present=false;
};
struct EffectIndex {void* object=nullptr;uint64_t generation=0;uint32_t slot=0;bool tombstone=false;};
static_assert(std::is_standard_layout_v<Vtable>&&std::is_standard_layout_v<Effect>);
static_assert((EffectIndexCapacity&(EffectIndexCapacity-1))==0);
// Native pre-FireShot life, not a client-reported health value. The receipt
// chain admits only this module's already-read-back native point applications
// between observation and processing, including other pellets in the batch.
struct NativeLife {
    bool valid=false,alive=false;uint32_t epoch=0;uint64_t receipt=0;
    void *pawn=nullptr,*handler=nullptr,*health=nullptr,*shield=nullptr;
    Weak pawnId{},handlerId{},healthId{},shieldId{};
    float healthValue=0,shieldValue=0;int32_t eventIndex=0,pending=0;
};
inline constexpr size_t LifeReceiptCapacity=128;
struct LifeReceipt {uint64_t serial=0;NativeLife before,after;};
inline std::array<std::array<LifeReceipt,LifeReceiptCapacity>,MaxPlayers> lifeReceipts{};
inline std::array<uint64_t,MaxPlayers> lifeReceiptSerials{};
struct Origin {
    uint64_t sequence=0,occurrence=0;
    int32_t nativeShot=-1;
    engine::Vec location{};
    engine::Vec inputAim{},controlRotation{};
    double at=-1000;
    Weak effectIdentity{};
    void* firing=nullptr;Weak firingIdentity{};uint32_t firingSlot=0;
    void* projectileClass=nullptr;
    uintptr_t nativeFrame=0;
    std::array<NativeLife,MaxPlayers> before{};
    bool valid=false,consumed=false,inputAimValid=false,weaponUseQueued=false,sensorQueued=false;
};
struct RecentRay {uint64_t origin=0;uint32_t pellet=0;};
struct FiringContext {
    void *component=nullptr,*resource=nullptr,*stability=nullptr;
    void *resourceOverride=nullptr,*stabilityOverride=nullptr;
    Weak identity{},resourceIdentity{},stabilityIdentity{};
    Vtable table;NativeEvent forward=nullptr;
    void* geometryFunction=nullptr;Weak geometryFunctionId{};
    uint32_t geometryFunctionFlags=0;
    uint64_t latestOrigin=0,geometryCaptures=0;
    int32_t lastGeometryShot=-1,lastGeometryCount=0;
    bool tuningCached=false,resourceIsAmmo=false;
    original_damage_tuning::Diagnostics tuning;
    char name[128]{};
    uint64_t originsObserved=0,raysObserved=0,effectHooks=0,resourceQueries=0;
};
struct Source {
    bool bound=false,observing=false,alive=false,physicsReady=false,current=false,tuningCached=false,ballistic=true;
    bool haveView=false,aimValid=false,inputRestartAttempted=false,inputReady=false,inputViewLease=false,externalInputProducer=false;
    bool externalPending=false,externalNativeStateMatches=false,externalCanSubmit=false,externalNativeAppliedReady=false,externalNativeAimMatches=false;
    size_t index=0;uint32_t spawnEpoch=0,held=0;
    uint64_t binding=0,originCursor=0,rayCursor=0,recentCursor=0;
    void *pawn=nullptr,*controller=nullptr,*weapon=nullptr,*inventory=nullptr,*firing=nullptr,*ammo=nullptr;
    void *input=nullptr,*inputState=nullptr,*restartPawn=nullptr,*externalMovement=nullptr,*externalClock=nullptr,*externalPrediction=nullptr;
    Weak externalMovementId{};
    Weak restartPawnId{};
    Weak pawnId{},controllerId{},weaponId{},inventoryId{},firingId{},ammoId{},inputId{},inputStateId{};
    Vtable pawnTable;
    NativeView forwardView=nullptr;
    std::array<Effect*,MaxEffects> effects{};
    std::array<Origin,MaxOrigins> origins{};
    std::array<FiringContext,MaxFiringStates> firings{};
    size_t firingCount=0;int32_t lastFiringSlot=-1;
    std::array<RecentRay,256> recent{};
    original_damage_tuning::Diagnostics tuning;
    engine::Vec nativeView{},lastView{},observedAim{},controlRotation{};
    double sampledAt=-1000;
    int32_t bodyCount=0,physicsActors=0,ammoCount=-1;
    uint64_t originsObserved=0,raysObserved=0,effectHooks=0,inputEdges=0,inputSubmissions=0,viewSubmissions=0;
    uint64_t viewObservedMatches=0,damageEvents=0,kills=0,pcInputInitializations=0,pawnClientRestarts=0;
    uint64_t externalBatches=0,externalAppliedBatches=0,externalCancelledBatches=0,externalResets=0,externalDeduplicated=0;
    int32_t externalLastEvent=-1,externalObservedEvent=-1;uint32_t externalObservedHeld=0;
    float externalMovementTime=-1,externalDeadline=-1;double externalSubmittedAt=-1;
};
struct ShotToken {
    uint32_t source=0,spawnEpoch=0;
    uint64_t binding=0,nativeOrigin=0;
    uint32_t projectile=0,firingSlot=0;
};
struct Pending {
    original_shots::Shot shot;
    ShotToken identity;
    uint64_t damageToken=0;
    void* firing=nullptr;Weak firingIdentity{};
    void* projectileClass=nullptr;
    uint32_t eligible=0;
    std::array<uint32_t,MaxPlayers> victimEpochs{};
    std::array<NativeLife,MaxPlayers> before{};
};
struct ObservedRay {
    original_shots::Shot shot;
    ShotToken identity;
    uint64_t damageToken=0;
    engine::Vec originalInputAim{},originalControlRotation{};
    bool inputAimValid=false;
    void *firing=nullptr,*resource=nullptr,*stability=nullptr,*projectileClass=nullptr;
};
struct Retired {void* actor=nullptr;Weak identity{};};
enum class RetireReason:uint8_t{WeaponChange,PawnRetire,NativeDeath,Shutdown};
struct Diagnostics {
    bool initialized=false,ready=false,restartRequired=false,processing=false,nativeEntriesVerified=false;
    unsigned error=0,boundSources=0,aliveSources=0,physicsReadySources=0;
    uint64_t binds=0,retirements=0,restoredPawnTables=0,restoredEffectTables=0;
    uint64_t originQueries=0,exactOrigins=0,effectCallbacks=0,directions=0,nativeRays=0;
    uint64_t nativeContextWalks=0,nativeContextMatches=0,nativeContextRejected=0,unknownFiringOrigins=0;
    uint64_t changedProjectileRejected=0;
    uint64_t effectObserverRecords=0,effectObserverReclaims=0,effectLeaseTransfers=0,effectObserverReinstalls=0;
    uint64_t effectOrphanRecoveries=0,effectLeaseCallbackRebinds=0,effectRegistryIdentityRejects=0;
    uint64_t effectObservedDispatches=0,effectOriginalForwardAttempts=0,effectOriginalForwardCompleted=0,effectStartCallbacks=0;
    uint64_t weaponUsedQueued=0,weaponUsedNotified=0,weaponUsedExpired=0,weaponUsedOverflow=0;
    void *lastEffectObject=nullptr,*lastEffectCurrentTable=nullptr,*lastEffectPreviousTable=nullptr;
    int32_t lastEffectOldSource=-1,lastEffectNewSource=-1;
    uint64_t lastEffectOldBinding=0,lastEffectNewBinding=0;
    uint32_t nativeContextExceptionCode=0;
    uint64_t invalidPayloads=0,unpairedOrigins=0,ambiguousOrigins=0,duplicateRays=0,expiredOrigins=0;
    uint64_t queued=0,processed=0,queueOverflow=0,staleShots=0,expiredBindings=0,retiredVictimEpochs=0;
    uint64_t physicsDeferred=0,inputDeferred=0,pendingDamageDeferred=0,bindingDeferred=0;
    uint64_t bodyTraces=0,sceneTraces=0,sceneBlocked=0,misses=0,policyRejected=0;
    uint64_t damageEvents=0,kills=0,viewSubmissions=0,inputEdges=0,inputSubmissions=0;
    uint64_t visibilityQueries=0,visibilityDeferred=0,foreignTableConflicts=0;
    uint64_t firingObservers=0,firingTablesRestored=0,geometryDispatches=0,geometryCallbacks=0;
    uint64_t geometryForwardAttempts=0,geometryForwardCompleted=0,geometryFrameMatches=0,geometryFrameRejected=0;
    uint64_t geometryCaptures=0,geometryRays=0,geometryUnpaired=0,geometryInvalid=0;
    uint64_t nativeLifeChanged=0,nativeLifeMissing=0,lifeReceipts=0;
    int32_t lastGeometryShot=-1,lastGeometryCount=0;engine::Vec lastGeometryStart{},lastGeometryDirection{};
    uint64_t lastToken=0,lastNativeOrigin=0;
    int32_t lastSource=-1,lastVictim=-1;
    float lastDamage=0;
    double lastPoll=-1,lastBodyDistance=0,lastSceneDistance=0;
    uintptr_t exceptionRva=0;
    const char* stage="server_combat_not_initialized";
    const char* operation="none";
};
inline Diagnostics diagnostics;
inline std::array<Source,MaxPlayers> sources{};
inline std::array<Effect,MaxEffectObservers> effectObservers{};
inline std::array<EffectIndex,EffectIndexCapacity> effectIndex{};
inline uint64_t nextEffectGeneration=0;
inline size_t nextEffectRecord=0;
inline std::array<Pending,QueueCapacity> queue{};
inline std::array<ObservedRay,256> observations{};
inline std::array<Retired,RetiredCapacity> retired{};
inline uint64_t readIndex=0,writeIndex=0,nextBinding=0,nextOrigin=0,nextDamageToken=0,nextOccurrence=0;
inline uint64_t observationRead=0,observationWrite=0,observationDrops=0;
struct WeaponUse {size_t source=0;uint32_t spawnEpoch=0;uint64_t binding=0,origin=0;void* pawn=nullptr;Weak pawnIdentity{};};
using WeaponUsedCallback=void(*)(size_t);
inline WeaponUsedCallback weaponUsedCallback=nullptr;
inline std::array<WeaponUse,QueueCapacity> weaponUses{};
inline uint64_t weaponUseRead=0,weaponUseWrite=0;
inline void SetWeaponUsedCallback(WeaponUsedCallback callback){weaponUsedCallback=callback;}
inline DWORD gameThread=0;inline uint64_t observerDepth=0;
inline void *world=nullptr,*shotFunction=nullptr,*startFunction=nullptr,*forceInputFunction=nullptr;
inline void *componentsFunction=nullptr,*resourceFunction=nullptr,*stabilityFunction=nullptr;
inline void* geometryFunction=nullptr;
inline void *externalForwardFunction=nullptr,*externalMathDefault=nullptr;
inline Weak externalMathIdentity{};
inline bool externalFunctionsReady=false;
using CaptureContextFn=void(WINAPI*)(PCONTEXT);
using LookupFunctionFn=PRUNTIME_FUNCTION(WINAPI*)(DWORD64,PDWORD64,PUNWIND_HISTORY_TABLE);
using VirtualUnwindFn=PEXCEPTION_ROUTINE(WINAPI*)(DWORD,DWORD64,DWORD64,PRUNTIME_FUNCTION,PCONTEXT,PVOID*,PDWORD64,PKNONVOLATILE_CONTEXT_POINTERS);
inline CaptureContextFn captureContext=nullptr;
inline LookupFunctionFn lookupFunction=nullptr;
inline VirtualUnwindFn virtualUnwind=nullptr;
inline const Diagnostics& State(){return diagnostics;}
inline const Source* Player(size_t index){return index<sources.size()?&sources[index]:nullptr;}
inline bool Ready(size_t index){const auto p=Player(index);return p&&p->bound&&p->alive&&p->physicsReady&&p->inputReady&&p->observing;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Fail(unsigned code,const char* stage,bool terminal=false){
    diagnostics.error=code;diagnostics.stage=stage;diagnostics.ready=false;
    if(terminal)diagnostics.restartRequired=true;return false;
}
inline bool Thread(){return diagnostics.initialized&&!diagnostics.restartRequired&&gameThread==GetCurrentThreadId();}
inline double Now(){return static_cast<double>(GetTickCount64())/1000.;}
inline bool Same(const engine::Vec& a,const engine::Vec& b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
inline bool Close(const engine::Vec& a,const engine::Vec& b){return std::abs(a.x-b.x)<1.e-10&&std::abs(a.y-b.y)<1.e-10&&std::abs(a.z-b.z)<1.e-10;}
inline bool ReadNativeLife(size_t index,NativeLife& out,bool requireAlive=true){
    out={};if(index>=MaxPlayers)return false;
    const auto player=policy::Player(index);const auto& target=original_damage::targets[index];
    if(!player||!player->registered||!target.bound||target.pawn!=player->pawn
       ||!target.damageable||!target.healthSection||Read<uint8_t>(player->pawn,0x224)!=3
       ||Read<uint8_t>(player->playerState,0x224)!=3
       ||(requireAlive&&(!player->ready||!player->alive||player->phase!=policy::Phase::Alive
          ||!policy::Authority(index,player->pawn,player->controller))))return false;
    out.pawn=target.pawn;out.handler=target.damageable;out.health=target.healthSection;out.shield=target.shieldSection;
    out.epoch=player->spawnEpoch;out.receipt=lifeReceiptSerials[index];
    if(!Identity(out.pawn,out.pawnId)||!Identity(out.handler,out.handlerId)||!Identity(out.health,out.healthId)
       ||(out.shield&&!Identity(out.shield,out.shieldId))||Read<void*>(out.pawn,0xc70)!=out.handler)return false;
    out.healthValue=Read<float>(out.health,0x124);out.shieldValue=out.shield?Read<float>(out.shield,0x124):0;
    out.eventIndex=Read<int32_t>(out.handler,0x1e0);out.pending=Read<int32_t>(out.handler,0x248);
    out.alive=Read<uint8_t>(out.handler,0x1f9)!=0;
    out.valid=std::isfinite(out.healthValue)&&out.healthValue>=0&&std::isfinite(out.shieldValue)
        &&out.shieldValue>=0&&(!requireAlive||(out.alive&&out.healthValue>0));return out.valid;
}
inline bool SameNativeLife(const NativeLife& a,const NativeLife& b){
    return a.valid&&b.valid&&a.epoch==b.epoch&&a.pawn==b.pawn&&a.handler==b.handler&&a.health==b.health&&a.shield==b.shield
        &&original_inventory_slots::SameIdentity(a.pawnId,b.pawnId)&&original_inventory_slots::SameIdentity(a.handlerId,b.handlerId)
        &&original_inventory_slots::SameIdentity(a.healthId,b.healthId)
        &&(!a.shield||original_inventory_slots::SameIdentity(a.shieldId,b.shieldId))
        &&a.healthValue==b.healthValue&&a.shieldValue==b.shieldValue&&a.eventIndex==b.eventIndex&&a.pending==b.pending&&a.alive==b.alive;
}
inline bool NativeLifeAllowsSupplement(size_t index,const NativeLife& before){
    if(!before.valid||index>=MaxPlayers)return false;
    NativeLife actual;if(!ReadNativeLife(index,actual,false))return false;
    const auto end=lifeReceiptSerials[index];
    if(end<before.receipt||end-before.receipt>LifeReceiptCapacity)return false;
    auto expected=before;
    for(uint64_t offset=0;offset<end-before.receipt;++offset){const auto serial=before.receipt+offset+1;
        const auto& receipt=lifeReceipts[index][(serial-1)%LifeReceiptCapacity];
        if(receipt.serial!=serial||!SameNativeLife(expected,receipt.before))return false;expected=receipt.after;
    }
    return SameNativeLife(expected,actual);
}
inline bool RecordNativeLifeReceipt(size_t index,const NativeLife& before){
    if(index>=MaxPlayers||!before.valid||before.receipt!=lifeReceiptSerials[index]
       ||lifeReceiptSerials[index]==(std::numeric_limits<uint64_t>::max)())return false;
    NativeLife after;if(!ReadNativeLife(index,after,false)||after.epoch!=before.epoch||after.pawn!=before.pawn)return false;
    const auto serial=++lifeReceiptSerials[index];after.receipt=serial;
    lifeReceipts[index][(serial-1)%LifeReceiptCapacity]={serial,before,after};++diagnostics.lifeReceipts;return true;
}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Identity(void* object,Weak& identity){return original_inventory_slots::CaptureIdentity(object,identity)&&Live(object,identity);}
inline LONG CaptureException(EXCEPTION_POINTERS* p){
    const auto code=p&&p->ExceptionRecord?p->ExceptionRecord->ExceptionCode:0xe0002400U;
    const auto address=p&&p->ExceptionRecord?reinterpret_cast<uintptr_t>(p->ExceptionRecord->ExceptionAddress):0;
    diagnostics.exceptionRva=address>=engine::image?address-engine::image:address;
    policy::CancelNativeDamage();
    Fail(code,"server_combat_native_exception_requires_new_session",true);return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Event(void* object,const wchar_t* name,uint16_t bytes,void* parameters){
    return original_damage::Event(object,name,bytes,parameters);
}

// Stop only at a real non-code delimiter; never truncate a table at the slot
// being intercepted. RTTI and every preceding/following virtual are copied.
inline bool CopyVtable(void* object,Vtable& result,size_t minimumFunctions){
    auto previous=Read<void**>(object,0);if(!previous||!minimumFunctions||minimumFunctions>MaxVtableFunctions)return false;
    MEMORY_BASIC_INFORMATION codeRegion{};
    uintptr_t codeBegin=0,codeEnd=0;
    size_t count=0;
    for(;count<MaxVtableFunctions;++count){
        void* entry=nullptr;if(!original_players::Read(previous,count*8,entry))return false;
        const auto address=reinterpret_cast<uintptr_t>(entry);
        if(!address)break;
        if(address<codeBegin||address>=codeEnd){
            if(!VirtualQuery(entry,&codeRegion,sizeof(codeRegion))||codeRegion.State!=MEM_COMMIT
               ||(codeRegion.Protect&(PAGE_GUARD|PAGE_NOACCESS)))break;
            const auto protection=codeRegion.Protect&0xffU;
            if(protection!=PAGE_EXECUTE&&protection!=PAGE_EXECUTE_READ&&protection!=PAGE_EXECUTE_READWRITE
               &&protection!=PAGE_EXECUTE_WRITECOPY)break;
            codeBegin=reinterpret_cast<uintptr_t>(codeRegion.BaseAddress);codeEnd=codeBegin+codeRegion.RegionSize;
        }
    }
    if(count<minimumFunctions||count==MaxVtableFunctions
       ||!original_players::Readable(previous-1,(count+1)*8))return false;
    result.previous=previous;result.functions=count;
    std::memcpy(result.copy.data(),previous-1,(count+1)*8);return true;
}
inline bool Install(void* object,Vtable& table,size_t slot,void* replacement){
    if(!table.previous||slot>=table.functions||table.installed)return false;
    table.copy[1+slot]=replacement;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(object),table.Address(),table.previous)!=table.previous)return false;
    table.installed=true;return true;
}
inline bool Restore(void* object,const Weak& identity,Vtable& table){
    if(!table.installed)return true;
    if(!Live(object,identity)){table.installed=false;return true;}
    auto current=Read<void**>(object,0);
    if(current==table.previous){table.installed=false;return true;}
    if(current!=table.Address()){++diagnostics.foreignTableConflicts;return Fail(2401,"server_combat_foreign_observer_must_retire_first");}
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(object),table.previous,table.Address())!=table.Address())
        return Fail(2402,"server_combat_instance_vptr_restore_raced");
    table.installed=false;return true;
}
inline bool OwnsEffect(const Source& s,const Effect& effect){
    return s.bound&&effect.ownerSource==s.index&&effect.ownerBinding==s.binding;
}
inline size_t EffectHash(void* object){
    const auto value=reinterpret_cast<uintptr_t>(object)>>4;
    return static_cast<size_t>((value^(value>>19))&(EffectIndexCapacity-1));
}
inline Effect* IndexedEffect(void* object){
    size_t at=EffectHash(object);
    for(size_t probe=0;probe<effectIndex.size();++probe,at=(at+1)&(EffectIndexCapacity-1)){
        const auto& key=effectIndex[at];if(!key.object&&!key.tombstone)return nullptr;
        if(key.object==object&&key.slot<effectObservers.size()){
            auto& effect=effectObservers[key.slot];
            return effect.object==object&&effect.generation==key.generation?&effect:nullptr;}
    }
    return nullptr;
}
inline void RemoveEffectIndex(Effect& effect){
    size_t at=EffectHash(effect.object);
    for(size_t probe=0;probe<effectIndex.size();++probe,at=(at+1)&(EffectIndexCapacity-1)){
        auto& key=effectIndex[at];if(!key.object&&!key.tombstone)return;
        if(key.object==effect.object){if(key.generation==effect.generation)key={nullptr,0,0,true};return;}
    }
}
inline bool PublishEffectIndex(Effect& effect){
    size_t at=EffectHash(effect.object),vacant=effectIndex.size();
    for(size_t probe=0;probe<effectIndex.size();++probe,at=(at+1)&(EffectIndexCapacity-1)){
        const auto& key=effectIndex[at];
        if(key.object==effect.object){vacant=at;break;}
        if(!key.object){if(vacant==effectIndex.size())vacant=at;if(!key.tombstone)break;}
    }
    if(vacant==effectIndex.size())return Fail(2462,"server_combat_effect_identity_index_full");
    effectIndex[vacant]={effect.object,effect.generation,static_cast<uint32_t>(&effect-effectObservers.data()),false};return true;
}
inline Effect* EffectByTable(void** table){
    // The backing address itself identifies our immutable instance table. This
    // does not inspect arbitrary table contents or mistake a native table for
    // ours when logical ownership changes before the next Source Poll.
    const auto first=reinterpret_cast<uintptr_t>(effectObservers.data())+offsetof(Effect,table)+offsetof(Vtable,copy)+sizeof(void*);
    const auto address=reinterpret_cast<uintptr_t>(table);
    if(address<first||(address-first)%sizeof(Effect))return nullptr;
    const auto slot=(address-first)/sizeof(Effect);
    if(slot>=effectObservers.size())return nullptr;
    auto& effect=effectObservers[slot];return effect.table.Address()==table?&effect:nullptr;
}
inline void UnleaseEffect(Effect& effect){
    effect.ownerSource=MaxPlayers;effect.ownerBinding=0;effect.firing=nullptr;effect.firingSlot=0;
    effect.present=false;effect.occurrence=0;effect.initialCapturedOccurrence=0;
    // Keep object+weak identity, previous vptr and original forwarding target
    // for this live pooled UObject. They are reclaimed only after its serial
    // expires, never merely because one gun's active-effects array drops it.
}
inline bool ReleaseEffect(Source& s,Effect*& reference){
    auto effect=reference;if(!effect)return true;
    if(OwnsEffect(s,*effect)){const bool installed=effect->table.installed;
        if(!Restore(effect->object,effect->identity,effect->table))return false;
        if(installed)++diagnostics.restoredEffectTables;UnleaseEffect(*effect);}
    reference=nullptr;return true;
}
inline bool LeaseEffect(Source& s,Effect& effect,uint32_t firingSlot,bool fromCallback=false){
    if(firingSlot>=s.firingCount||Read<void*>(effect.object,0x148)!=s.firings[firingSlot].component)return false;
    Effect** destination=nullptr;
    for(auto& reference:s.effects){if(reference==&effect){destination=&reference;break;}
        if(!destination&&(!reference||!OwnsEffect(s,*reference)))destination=&reference;}
    if(!destination)return Fail(2406,"server_combat_effect_observer_capacity_exhausted");
    const bool changed=effect.ownerSource!=s.index||effect.ownerBinding!=s.binding||effect.firingSlot!=firingSlot
        ||effect.firing!=s.firings[firingSlot].component;
    if(changed){
        diagnostics.lastEffectObject=effect.object;diagnostics.lastEffectCurrentTable=Read<void*>(effect.object,0);
        diagnostics.lastEffectPreviousTable=effect.table.previous;
        diagnostics.lastEffectOldSource=effect.ownerSource<MaxPlayers?static_cast<int32_t>(effect.ownerSource):-1;
        diagnostics.lastEffectNewSource=static_cast<int32_t>(s.index);
        diagnostics.lastEffectOldBinding=effect.ownerBinding;diagnostics.lastEffectNewBinding=s.binding;
        if(effect.ownerSource<MaxPlayers&&effect.ownerBinding){
            auto& previous=sources[effect.ownerSource];
            for(auto& reference:previous.effects)if(reference==&effect)reference=nullptr;
            ++diagnostics.effectLeaseTransfers;
        }
        effect.ownerSource=s.index;effect.ownerBinding=s.binding;
        effect.firing=s.firings[firingSlot].component;effect.firingSlot=firingSlot;effect.occurrence=++nextOccurrence;
        if(fromCallback)++diagnostics.effectLeaseCallbackRebinds;
    }
    *destination=&effect;effect.present=true;return true;
}
inline Effect* AllocateEffectRecord(){
    for(size_t scanned=0;scanned<effectObservers.size();++scanned){
        auto& effect=effectObservers[nextEffectRecord++%effectObservers.size()];
        if(effect.object&&Live(effect.object,effect.identity))continue;
        if(effect.object){RemoveEffectIndex(effect);++diagnostics.effectObserverReclaims;}
        // No live UObject can legitimately depend on a serial-expired record.
        // Clear metadata only after that native resolver has confirmed expiry.
        effect={};effect.generation=++nextEffectGeneration;return &effect;
    }
    Fail(2463,"server_combat_live_effect_identity_registry_full");return nullptr;
}
inline bool RecoverAliasedEffectTable(void* object,const Weak& identity,Effect& provenance){
    // Defensive handling for a pool restoring an old cloned vptr onto another
    // identity. The native current class CDO must independently prove the exact
    // saved prior table; an expired/other identity never authorizes a write.
    auto type=Read<void*>(object,0x10);auto cdo=Read<void*>(type,0x120);auto native=Read<void**>(cdo,0);
    if(!Live(object,identity)||!original_players::IsA(object,0x684d570)||!native
       ||native!=provenance.table.previous||provenance.forward!=engine::function<NativeEvent>(0x1cdc850)
       ||Read<void*>(native,0x298)!=reinterpret_cast<void*>(engine::image+0x1cdc850)){
        ++diagnostics.effectRegistryIdentityRejects;return Fail(2464,"server_combat_owned_effect_clone_identity_not_proven");}
    if(!provenance.table.functions||provenance.table.copy[0]!=Read<void*>(native-1,0))return Fail(2464,"server_combat_owned_effect_clone_rtti_not_proven");
    for(size_t slot=0;slot<provenance.table.functions;++slot){
        const auto expected=slot==0x298/8?reinterpret_cast<void*>(ObserveEffectEvent):Read<void*>(native,slot*8);
        if(provenance.table.copy[1+slot]!=expected)return Fail(2464,"server_combat_owned_effect_clone_contains_foreign_layer");
    }
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(object),native,provenance.table.Address())!=provenance.table.Address())
        return Fail(2465,"server_combat_owned_effect_clone_recovery_raced");
    ++diagnostics.effectOrphanRecoveries;return true;
}
inline bool RecoverEquivalentForeignTable(void* object,const Weak& identity,void** current){
    auto type=Read<void*>(object,0x10);auto cdo=Read<void*>(type,0x120);
    Vtable native;
    if(!Live(object,identity)||!cdo||!CopyVtable(cdo,native,0x298/8+1)
       ||native.previous[0x298/8]!=reinterpret_cast<void*>(engine::image+0x1cdc850)
       ||!original_players::Readable(current-1,(native.functions+1)*8))return false;
    if(Read<void*>(current-1,0)!=native.copy[0])return false;
    for(size_t slot=0;slot<native.functions;++slot){
        const auto expected=slot==0x298/8?reinterpret_cast<void*>(ObserveEffectEvent):native.previous[slot];
        if(Read<void*>(current,slot*8)!=expected)return false;
    }
    auto tail=Read<void*>(current,native.functions*8);
    if(tail){MEMORY_BASIC_INFORMATION region{};
        if(!VirtualQuery(tail,&region,sizeof(region)))return false;
        const auto protection=region.Protect&0xffU;
        if(region.State==MEM_COMMIT&&!(region.Protect&(PAGE_GUARD|PAGE_NOACCESS))
           &&(protection==PAGE_EXECUTE||protection==PAGE_EXECUTE_READ||protection==PAGE_EXECUTE_READWRITE
              ||protection==PAGE_EXECUTE_WRITECOPY))return false;
    }
    if(!Live(object,identity)||InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(object),native.previous,current)!=current)return false;
    ++diagnostics.effectOrphanRecoveries;return true;
}
inline Effect* AcquireEffect(Source& s,void* object,uint32_t firingSlot){
    Weak identity{};if(!Identity(object,identity))return nullptr;
    auto current=Read<void**>(object,0);auto owned=EffectByTable(current);
    auto previousRecord=owned?owned:IndexedEffect(object);
    diagnostics.lastEffectObject=object;diagnostics.lastEffectCurrentTable=current;
    diagnostics.lastEffectPreviousTable=previousRecord?previousRecord->table.previous:nullptr;
    diagnostics.lastEffectOldSource=previousRecord&&previousRecord->ownerSource<MaxPlayers?static_cast<int32_t>(previousRecord->ownerSource):-1;
    diagnostics.lastEffectNewSource=static_cast<int32_t>(s.index);
    diagnostics.lastEffectOldBinding=previousRecord?previousRecord->ownerBinding:0;diagnostics.lastEffectNewBinding=s.binding;
    if(owned&&(owned->object!=object||!original_inventory_slots::SameIdentity(owned->identity,identity))){
        if(!RecoverAliasedEffectTable(object,identity,*owned))return nullptr;
        current=Read<void**>(object,0);owned=nullptr;
    }
    if(!owned&&Read<void*>(current,0x298)==reinterpret_cast<void*>(ObserveEffectEvent)){
        // An external copy of our table is accepted only if its RTTI, complete
        // native prefix and boundary independently match this class's CDO,
        // with precisely our single PE override. Preserve all other layers.
        if(!RecoverEquivalentForeignTable(object,identity,current)){
            ++diagnostics.effectRegistryIdentityRejects;Fail(2466,"server_combat_effect_observer_provenance_missing");return nullptr;}
        current=Read<void**>(object,0);
    }
    auto effect=owned?owned:IndexedEffect(object);
    if(effect&&(!original_inventory_slots::SameIdentity(effect->identity,identity)||effect->object!=object))effect=nullptr;
    bool installedNow=false,newRecord=false;
    if(!effect){
        // Never call CopyVtable on one of our hook tables. It would make the
        // observer its own forwarding target and lose the native prior layer.
        if(EffectByTable(current)||Read<void*>(current,0x298)==reinterpret_cast<void*>(ObserveEffectEvent)){
            Fail(2466,"server_combat_effect_observer_provenance_missing");return nullptr;}
        Vtable previous;
        if(!CopyVtable(object,previous,0x298/8+1)){Fail(2407,"server_combat_effect_complete_vtable_unavailable");return nullptr;}
        auto forward=reinterpret_cast<NativeEvent>(previous.previous[0x298/8]);
        if(forward!=engine::function<NativeEvent>(0x1cdc850)){Fail(2408,"server_combat_effect_process_event_already_observed");return nullptr;}
        effect=AllocateEffectRecord();if(!effect)return nullptr;
        newRecord=true;
        effect->object=object;effect->identity=identity;effect->table=previous;effect->forward=forward;
        if(!PublishEffectIndex(*effect))return nullptr;++diagnostics.effectObserverRecords;
    }
    if(effect->forward!=engine::function<NativeEvent>(0x1cdc850)||!effect->table.previous){
        Fail(2467,"server_combat_effect_original_forward_target_invalid");return nullptr;}
    if(current==effect->table.Address()){
        if(!effect->table.installed)++diagnostics.effectOrphanRecoveries;
        effect->table.installed=true;
    }else if(current==effect->table.previous){
        effect->table.installed=false;
        if(!Install(object,effect->table,0x298/8,reinterpret_cast<void*>(ObserveEffectEvent))){Fail(2409,"server_combat_effect_instance_install_failed");return nullptr;}
        installedNow=true;if(!newRecord)++diagnostics.effectObserverReinstalls;
    }else {Fail(2468,"server_combat_effect_foreign_instance_vptr_conflict");return nullptr;}
    if(!LeaseEffect(s,*effect,firingSlot))return nullptr;
    if(installedNow){++s.effectHooks;++s.firings[firingSlot].effectHooks;}
    return effect;
}
inline Source* PawnSource(void* pawn){for(auto& s:sources)if(s.bound&&s.pawn==pawn)return &s;return nullptr;}
inline FiringContext* FiringRecord(Source& s,void* firing){
    if(firing)for(size_t i=0;i<s.firingCount;++i)if(s.firings[i].component==firing)return &s.firings[i];return nullptr;
}
inline Source* FiringSource(void* firing){if(firing)for(auto& s:sources)if(s.bound&&FiringRecord(s,firing))return &s;return nullptr;}
inline Effect* EffectRecord(Source& s,void* object){for(auto effect:s.effects)if(effect&&OwnsEffect(s,*effect)&&effect->object==object)return effect;return nullptr;}
inline Source* SyncEffectLease(Effect& effect){
    if(!Live(effect.object,effect.identity))return nullptr;
    auto source=FiringSource(Read<void*>(effect.object,0x148));
    if(!source||!source->observing||!Live(source->pawn,source->pawnId))return nullptr;
    auto firing=FiringRecord(*source,Read<void*>(effect.object,0x148));
    if(!firing||!Live(firing->component,firing->identity)||Read<void*>(firing->component,0xc8)!=source->weapon
       ||Read<void*>(firing->component,0x4c8)!=source->pawn)return nullptr;
    const auto slot=static_cast<uint32_t>(firing-source->firings.data());
    return LeaseEffect(*source,effect,slot,true)?source:nullptr;
}
inline void CapturePayload(Source& source,Effect& effect,int32_t nativeShot,uint32_t flags,const uint8_t* array);
inline bool RefreshFiringResources(FiringContext& firing,bool force=false);
inline bool BeforeRetire(size_t index,RetireReason reason=RetireReason::WeaponChange);
inline bool SourceIdentity(const Source& s);

// Recover this thread's real native FireShot `this`. The two shipped functions
// both retain it in nonvolatile RSI. Windows unwind restores that register at
// their exact return PCs; no stack-word search, guessed active-state getter or
// first-component fallback can authorize a shot origin.
__declspec(noinline) inline FiringContext* NativeFiringContextImpl(Source& s,uintptr_t* nativeFrame){
    ++diagnostics.nativeContextWalks;
    if(!captureContext||!lookupFunction||!virtualUnwind)return nullptr;
    alignas(16) CONTEXT context{};captureContext(&context);
    const auto tib=reinterpret_cast<const NT_TIB*>(NtCurrentTeb());
    const auto low=reinterpret_cast<uintptr_t>(tib->StackLimit),high=reinterpret_cast<uintptr_t>(tib->StackBase);
    void *helper=nullptr,*fire=nullptr;bool invalid=false;
    for(unsigned depth=0;depth<24&&context.Rip;++depth){
        if(context.Rsp<low||context.Rsp>high||high-context.Rsp<8){invalid=true;break;}
        if(context.Rip==engine::image+0x71227dd)helper=reinterpret_cast<void*>(context.Rsi);
        if(context.Rip==engine::image+0x711f0ae){fire=reinterpret_cast<void*>(context.Rsi);if(nativeFrame)*nativeFrame=static_cast<uintptr_t>(context.Rbp);break;}
        const auto previousPc=context.Rip,previousSp=context.Rsp;
        DWORD64 imageBase=0;auto function=lookupFunction(context.Rip,&imageBase,nullptr);
        if(function){PVOID handlerData=nullptr;DWORD64 frame=0;
            virtualUnwind(UNW_FLAG_NHANDLER,imageBase,context.Rip,function,&context,&handlerData,&frame,nullptr);
        }else {
            DWORD64 returnPc=0;if(!original_players::Read(reinterpret_cast<void*>(context.Rsp),0,returnPc)){invalid=true;break;}
            context.Rip=returnPc;context.Rsp+=8;
        }
        if(context.Rsp<previousSp||(context.Rip==previousPc&&context.Rsp==previousSp)){invalid=true;break;}
    }
    if(invalid||!helper||helper!=fire){++diagnostics.nativeContextRejected;return nullptr;}
    auto firing=FiringRecord(s,fire);
    if(!firing){++diagnostics.unknownFiringOrigins;return nullptr;}
    if(!Live(firing->component,firing->identity)||!original_players::IsA(firing->component,0x684d4c0)
       ||Read<void*>(firing->component,0xc8)!=s.weapon||Read<void*>(firing->component,0x4c8)!=s.pawn){
        ++diagnostics.nativeContextRejected;return nullptr;}
    ++diagnostics.nativeContextMatches;return firing;
}
inline LONG ContextException(EXCEPTION_POINTERS* p){
    ++diagnostics.nativeContextRejected;
    diagnostics.nativeContextExceptionCode=p&&p->ExceptionRecord?p->ExceptionRecord->ExceptionCode:0xe0002459U;
    return EXCEPTION_EXECUTE_HANDLER;
}
__declspec(noinline) inline FiringContext* NativeFiringContext(Source& s,uintptr_t* nativeFrame){
    __try{return NativeFiringContextImpl(s,nativeFrame);}__except(ContextException(GetExceptionInformation())){return nullptr;}
}

__declspec(noinline) inline engine::Vec* ObserveViewLocation(void* pawn,engine::Vec* output){
    const bool trackedObserver=GetCurrentThreadId()==gameThread;
    if(trackedObserver)++observerDepth;
    __try{
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    if(GetCurrentThreadId()!=gameThread)return engine::function<NativeView>(0x787f820)(pawn,output);
    auto s=PawnSource(pawn);
    auto forward=s&&s->forwardView?s->forwardView:engine::function<NativeView>(0x787f820);
    auto result=forward(pawn,output);
    if(!Thread()||!s||!s->observing)return result;
    ++diagnostics.originQueries;
    if(caller!=engine::image+0x78840ca)return result;
    uintptr_t nativeFrame=0;auto firing=NativeFiringContext(*s,&nativeFrame);if(!firing||!nativeFrame)return result;
    engine::Vec origin{};
    if(!original_players::Read(result,0,origin)||!original_shots::WorldPoint(origin)){
        ++diagnostics.invalidPayloads;return result;}
    auto& record=s->origins[s->originCursor++%s->origins.size()];record={};
    record.firing=firing->component;record.firingIdentity=firing->identity;
    record.projectileClass=Read<void*>(firing->component,0x550);
    record.firingSlot=static_cast<uint32_t>(firing-s->firings.data());s->lastFiringSlot=static_cast<int32_t>(record.firingSlot);
    record.sequence=++nextOrigin;record.nativeShot=Read<int32_t>(firing->component,0x4a8);
    record.nativeFrame=nativeFrame;firing->latestOrigin=record.sequence;
    record.location=origin;record.at=Now();record.valid=true;
    record.inputAim=Read<engine::Vec>(s->inputState,0x238);record.inputAimValid=Read<uint8_t>(s->inputState,0x128)!=0;
    record.controlRotation=Read<engine::Vec>(s->controller,0x4e8);
    for(size_t n=0;n<MaxPlayers;++n)if(n!=s->index)ReadNativeLife(n,record.before[n]);
    ++firing->originsObserved;++s->originsObserved;++diagnostics.exactOrigins;return result;

    }__finally{if(trackedObserver)--observerDepth;}
}
inline void CaptureInitial(Source& s,Effect& e){
    if(!s.observing||Read<void*>(e.object,0x148)!=e.firing||!FiringRecord(s,e.firing))return;
    if(e.initialCapturedOccurrence==e.occurrence)return;e.initialCapturedOccurrence=e.occurrence;
    const uint32_t flags=4U|(Read<uint8_t>(e.object,0x168)?1U:0U)|(Read<int32_t>(e.object,0x164)<=0?2U:0U);
    CapturePayload(s,e,Read<int32_t>(e.object,0x160),flags,static_cast<uint8_t*>(e.object)+0x150);
}
inline void ObserveEffectEvent(void* object,void* function,void* parameters){
    const bool trackedObserver=GetCurrentThreadId()==gameThread;
    if(trackedObserver)++observerDepth;
    __try{
    if(GetCurrentThreadId()!=gameThread){engine::function<NativeEvent>(0x1cdc850)(object,function,parameters);return;}
    ++diagnostics.effectObservedDispatches;if(function==startFunction)++diagnostics.effectStartCallbacks;
    Effect* effect=nullptr;uint64_t generation=0;
    auto forward=engine::function<NativeEvent>(0x1cdc850);
    __try{
        effect=EffectByTable(Read<void**>(object,0));
        if(effect&&effect->forward==engine::function<NativeEvent>(0x1cdc850))forward=effect->forward;
        const bool exact=effect&&effect->object==object&&Live(object,effect->identity);
        generation=exact?effect->generation:0;
        if(!exact)++diagnostics.effectRegistryIdentityRejects;
        auto s=Thread()&&exact?SyncEffectLease(*effect):nullptr;
        if(Thread()&&s&&effect&&s->observing&&function==shotFunction){
            ++diagnostics.effectCallbacks;
            if(Read<void*>(parameters,0)==effect->firing){
                const uint32_t flags=(Read<uint8_t>(parameters,12)?1U:0U)|(Read<uint8_t>(parameters,13)?2U:0U);
                CapturePayload(*s,*effect,Read<int32_t>(parameters,8),flags,static_cast<uint8_t*>(parameters)+16);
            }
        }
    }__except(CaptureException(GetExceptionInformation())){}
    // Exactly one original dispatch, including unknown/retired logical leases.
    // A global immutable forwarding target never points back to this observer.
    ++diagnostics.effectOriginalForwardAttempts;forward(object,function,parameters);++diagnostics.effectOriginalForwardCompleted;
    __try{
        if(Thread()&&effect&&generation&&effect->generation==generation&&effect->object==object&&function==startFunction){
            auto current=SyncEffectLease(*effect);
            if(current){effect->occurrence=++nextOccurrence;CaptureInitial(*current,*effect);}
        }
    }__except(CaptureException(GetExceptionInformation())){}

    }__finally{if(trackedObserver)--observerDepth;}
}
inline void QueueWeaponUse(Source& s,Origin& origin){
    if(!weaponUsedCallback||origin.weaponUseQueued)return;origin.weaponUseQueued=true;
    if(weaponUseWrite-weaponUseRead>=weaponUses.size()){++diagnostics.weaponUsedOverflow;return;}
    weaponUses[weaponUseWrite++%weaponUses.size()]={s.index,s.spawnEpoch,s.binding,origin.sequence,s.pawn,s.pawnId};
    ++diagnostics.weaponUsedQueued;
}
inline bool NotifyWeaponUses(){
    if(!Thread())return Fail(2469,"server_combat_weapon_use_callback_requires_owned_game_thread");
    const auto end=weaponUseWrite;
    while(weaponUseRead<end){const auto event=weaponUses[weaponUseRead++%weaponUses.size()];
        if(!weaponUsedCallback)continue;
        const auto player=policy::Player(event.source);
        // A same-life gun switch still counts as real weapon use. An old-life
        // event can never strip protection from a replacement respawned Pawn.
        if(!player||!player->registered||!player->alive||player->spawnEpoch!=event.spawnEpoch
           ||player->pawn!=event.pawn||!Live(event.pawn,event.pawnIdentity)){++diagnostics.weaponUsedExpired;continue;}
        diagnostics.operation="observed_native_weapon_use_callback";
        weaponUsedCallback(event.source);++diagnostics.weaponUsedNotified;diagnostics.operation="none";
    }
    return true;
}
inline bool QueueRay(Source& s,Origin& origin,uint32_t pellet,int32_t nativeShot,uint32_t flags,const engine::Vec& direction,const engine::Vec* nativeStart=nullptr){
    if(origin.firingSlot>=s.firingCount)return false;auto& firing=s.firings[origin.firingSlot];
    if(firing.component!=origin.firing||!original_inventory_slots::SameIdentity(firing.identity,origin.firingIdentity))return false;
    QueueWeaponUse(s,origin);
    for(const auto& ray:s.recent)if(ray.origin==origin.sequence&&ray.pellet==pellet){++diagnostics.duplicateRays;return true;}
    s.recent[s.recentCursor++%s.recent.size()]={origin.sequence,pellet};
    if(writeIndex-readIndex>=queue.size()){++diagnostics.queueOverflow;return false;}
    if(nextDamageToken==(std::numeric_limits<uint64_t>::max)())return Fail(2404,"server_combat_damage_token_exhausted",true);
    auto& pending=queue[writeIndex%queue.size()];pending={};
    pending.identity={static_cast<uint32_t>(s.index),s.spawnEpoch,s.binding,origin.sequence,pellet,origin.firingSlot};
    pending.damageToken=++nextDamageToken;
    pending.firing=firing.component;pending.firingIdentity=firing.identity;
    pending.projectileClass=origin.projectileClass;
    pending.shot={origin.at,nativeStart?*nativeStart:origin.location,direction,nativeShot,flags|8U,
        firing.resourceIsAmmo&&Live(firing.resource,firing.resourceIdentity)?Read<int32_t>(firing.resource,0x10c):-1,pellet,++s.rayCursor};
    if(observationWrite-observationRead>=observations.size()){++observationRead;++observationDrops;}
    observations[observationWrite++%observations.size()]={pending.shot,pending.identity,pending.damageToken,origin.inputAim,origin.controlRotation,
        origin.inputAimValid,firing.component,firing.resource,firing.stability,origin.projectileClass};
    // Snapshot eligibility and every target's life. A delayed shot cannot hit a
    // new Pawn that happens to occupy the same roster slot after respawn.
    for(size_t i=0;i<sources.size();++i){const auto player=policy::Player(i);
        const auto& before=origin.before[i];
        if(i==s.index||!player||!player->registered||!player->alive||!before.valid||before.epoch!=player->spawnEpoch
           ||before.pawn!=player->pawn)continue;
        pending.eligible|=uint32_t(1)<<i;pending.victimEpochs[i]=player->spawnEpoch;pending.before[i]=before;
    }
    ++writeIndex;++firing.raysObserved;++s.raysObserved;++diagnostics.nativeRays;++diagnostics.queued;
    diagnostics.lastNativeOrigin=origin.sequence;return true;
}
struct NativeFiringResult {engine::Vec direction,start;};
static_assert(sizeof(NativeFiringResult)==48&&offsetof(NativeFiringResult,start)==24);
// This original ProcessEvent call is unconditional in FireShot. The actual
// FiringResults array has already been fully populated by 7120d80; optional
// visual effects and the later original OnShot use that same array. Recover
// only the exact caller's real Windows unwind frame, never a stack-word scan.
__declspec(noinline) inline const uint8_t* NativeGeometryFrame(void* firing,uintptr_t& nativeFrame){
    nativeFrame=0;if(!captureContext||!lookupFunction||!virtualUnwind)return nullptr;
    alignas(16) CONTEXT context{};captureContext(&context);
    const auto tib=reinterpret_cast<const NT_TIB*>(NtCurrentTeb());
    const auto low=reinterpret_cast<uintptr_t>(tib->StackLimit),high=reinterpret_cast<uintptr_t>(tib->StackBase);
    for(unsigned depth=0;depth<24&&context.Rip;++depth){
        if(context.Rsp<low||context.Rsp>high||high-context.Rsp<8)break;
        if(context.Rip==engine::image+0x711f1c3){
            const auto frame=static_cast<uintptr_t>(context.Rbp);
            if(reinterpret_cast<void*>(context.Rsi)!=firing||frame<low||frame>high||high-frame<0x40
               ||frame+0x30<context.Rsp)break;
            nativeFrame=frame;++diagnostics.geometryFrameMatches;return reinterpret_cast<const uint8_t*>(frame+0x30);
        }
        const auto previousPc=context.Rip,previousSp=context.Rsp;DWORD64 imageBase=0;
        auto function=lookupFunction(context.Rip,&imageBase,nullptr);
        if(function){PVOID handlerData=nullptr;DWORD64 frame=0;
            virtualUnwind(UNW_FLAG_NHANDLER,imageBase,context.Rip,function,&context,&handlerData,&frame,nullptr);
        }else {DWORD64 returnPc=0;if(!original_players::Read(reinterpret_cast<void*>(context.Rsp),0,returnPc))break;
            context.Rip=returnPc;context.Rsp+=8;}
        if(context.Rsp<previousSp||(context.Rip==previousPc&&context.Rsp==previousSp))break;
    }
    ++diagnostics.geometryFrameRejected;return nullptr;
}
inline void CaptureNativeGeometry(Source& source,FiringContext& firing){
    uintptr_t nativeFrame=0;const auto array=NativeGeometryFrame(firing.component,nativeFrame);if(!array)return;
    const auto data=Read<const uint8_t*>(array,0);const auto count=Read<int32_t>(array,8),capacity=Read<int32_t>(array,12);
    if(count<0||count>64||capacity<count||capacity>65536
       ||(count&&!original_players::Readable(data,static_cast<size_t>(count)*sizeof(NativeFiringResult)))){
        ++diagnostics.geometryInvalid;return;}
    ++diagnostics.geometryCaptures;++firing.geometryCaptures;
    const auto nativeShot=Read<int32_t>(firing.component,0x4a8);
    firing.lastGeometryShot=diagnostics.lastGeometryShot=nativeShot;
    firing.lastGeometryCount=diagnostics.lastGeometryCount=count;
    if(!count)return;
    Origin* origin=nullptr;const double now=Now();
    for(auto& record:source.origins)if(record.valid&&!record.consumed&&record.sequence==firing.latestOrigin
       &&record.nativeFrame==nativeFrame&&record.firing==firing.component&&record.nativeShot==nativeShot
       &&original_inventory_slots::SameIdentity(record.firingIdentity,firing.identity)
       &&now-record.at>=0&&now-record.at<options.maximumShotAge){origin=&record;break;}
    if(!origin){++diagnostics.geometryUnpaired;return;}
    std::array<NativeFiringResult,64> results{};
    std::memcpy(results.data(),data,static_cast<size_t>(count)*sizeof(NativeFiringResult));
    for(int32_t n=0;n<count;++n){const auto& result=results[static_cast<size_t>(n)];
        const auto length=result.direction.x*result.direction.x+result.direction.y*result.direction.y+result.direction.z*result.direction.z;
        if(!original_shots::Finite(result.direction)||length<.9||length>1.1||!original_shots::WorldPoint(result.start)){
            ++diagnostics.geometryInvalid;return;}
    }
    diagnostics.lastGeometryStart=results[0].start;diagnostics.lastGeometryDirection=results[0].direction;
    // Independent perceptual receipt: real FireShot origin/frame/firing identity
    // plus the complete validated canonical geometry array, before supplemental
    // damage eligibility, nativeLifeChanged or damage-queue capacity can return.
    if(!origin->sensorQueued){
        origin->sensorQueued=true;humanlike_bot_native_events::Event event{};
        event.kind=humanlike_bot_native_events::Kind::Shot;
        event.subject={static_cast<uint32_t>(source.index),source.spawnEpoch,reinterpret_cast<uintptr_t>(source.pawn),source.pawnId.index,source.pawnId.serial};
        event.atMatchSeconds=origin->at;event.binding=source.binding;event.origin=origin->sequence;event.firingSlot=origin->firingSlot;
        const auto start=results[0].start,direction=results[0].direction;
        event.position={static_cast<float>(start.x),static_cast<float>(start.y),static_cast<float>(start.z)};
        event.direction={static_cast<float>(direction.x),static_cast<float>(direction.y),static_cast<float>(direction.z)};
        event.actualGeometryPaired=true;humanlike_bot_native_events::PublishShot(event);
    }
    // Native simulation may already have applied damage during 7120d80. Never
    // apply another host point in that case. Check again during deferred Poll
    // because original OnShot listeners still execute after this observation.
    bool haveBaseline=false;
    for(size_t n=0;n<MaxPlayers;++n)if(origin->before[n].valid){haveBaseline=true;
        if(!NativeLifeAllowsSupplement(n,origin->before[n])){++diagnostics.nativeLifeChanged;origin->consumed=true;return;}}
    if(!haveBaseline)++diagnostics.nativeLifeMissing;
    bool queued=false;
    for(int32_t n=0;n<count;++n){const auto& result=results[static_cast<size_t>(n)];++diagnostics.directions;
        // Bit4 is this module's canonical-native-array provenance, not either
        // of the original OnShot's tracer/last-pellet booleans.
        if(QueueRay(source,*origin,static_cast<uint32_t>(n),nativeShot,16U,result.direction,&result.start)){
            queued=true;++diagnostics.geometryRays;}
    }
    if(queued)origin->consumed=true;
}
inline void ObserveFiringEvent(void* object,void* function,void* parameters){
    const bool trackedObserver=GetCurrentThreadId()==gameThread;
    if(trackedObserver)++observerDepth;
    __try{
    auto forward=engine::function<NativeEvent>(0x1cdc850);
    if(GetCurrentThreadId()!=gameThread){forward(object,function,parameters);return;}
    ++diagnostics.geometryDispatches;
    auto source=FiringSource(object);auto firing=source?FiringRecord(*source,object):nullptr;
    const auto binding=source?source->binding:0;
    // Every admitted table was checked against the original PE entry. Its
    // immutable forwarding target cannot point back to this observer.
    if(firing&&firing->forward==forward)forward=firing->forward;
    ++diagnostics.geometryForwardAttempts;forward(object,function,parameters);++diagnostics.geometryForwardCompleted;
    __try{
        if(!Thread()||!source||!firing||!source->observing||source->binding!=binding||!SourceIdentity(*source)
           ||firing->component!=object||!Live(object,firing->identity)||Read<void*>(object,0)!=firing->table.Address()
           ||function!=firing->geometryFunction||!Live(function,firing->geometryFunctionId)
           ||Read<uint16_t>(function,0xbe)!=8||Read<uint32_t>(function,0xb8)!=firing->geometryFunctionFlags)return;
        ++diagnostics.geometryCallbacks;CaptureNativeGeometry(*source,*firing);
    }__except(ContextException(GetExceptionInformation())){}

    }__finally{if(trackedObserver)--observerDepth;}
}
inline void CapturePayload(Source& s,Effect& effect,int32_t nativeShot,uint32_t flags,const uint8_t* array){
    if(!Thread()||!s.observing||!effect.occurrence||effect.firingSlot>=s.firingCount
       ||s.firings[effect.firingSlot].component!=effect.firing||Read<void*>(effect.object,0x148)!=effect.firing)return;
    if(!RefreshFiringResources(s.firings[effect.firingSlot]))return;
    const auto data=Read<uint8_t*>(array,0);const int32_t count=Read<int32_t>(array,8),capacity=Read<int32_t>(array,12);
    if(count<1||count>64||capacity<count||!original_players::Readable(data,static_cast<size_t>(count)*48)){
        ++diagnostics.invalidPayloads;return;}
    const double now=Now();Origin* origin=nullptr;unsigned matches=0;
    // An already decoded initial effect occurrence must not steal the next
    // burst's reset native ShotIndex. Later real OnShot callbacks may use it.
    if(flags&4U)for(const auto& record:s.origins)if(record.consumed&&record.firing==effect.firing&&record.nativeShot==nativeShot
        &&record.occurrence==effect.occurrence&&original_inventory_slots::SameIdentity(record.effectIdentity,effect.identity)){
        ++diagnostics.duplicateRays;return;}
    for(auto& record:s.origins)if(record.valid&&!record.consumed&&record.firing==effect.firing
        &&original_inventory_slots::SameIdentity(record.firingIdentity,s.firings[effect.firingSlot].identity)&&record.nativeShot==nativeShot
        &&now-record.at>=0&&now-record.at<options.maximumShotAge){origin=&record;++matches;}
    if(matches!=1){if(matches)++diagnostics.ambiguousOrigins;else ++diagnostics.unpairedOrigins;return;}
    bool any=false;
    for(int32_t i=0;i<count;++i){const auto direction=Read<engine::Vec>(data+static_cast<size_t>(i)*48,0);
        const double norm=direction.x*direction.x+direction.y*direction.y+direction.z*direction.z;
        if(!original_shots::Finite(direction)||norm<.9||norm>1.1){++diagnostics.invalidPayloads;continue;}
        ++diagnostics.directions;any=true;
        // StartLocation in cosmetic initial FireData is intentionally ignored,
        // even if its uninitialized bytes happen to look finite.
        QueueRay(s,*origin,static_cast<uint32_t>(i),nativeShot,flags,direction);
    }
    if(any){origin->consumed=true;origin->occurrence=effect.occurrence;origin->effectIdentity=effect.identity;}
}

inline bool PollEffects(Source& s){
    for(auto& reference:s.effects){if(reference&&OwnsEffect(s,*reference))reference->present=false;else reference=nullptr;}
    for(size_t stateIndex=0;stateIndex<s.firingCount;++stateIndex){auto& firing=s.firings[stateIndex];
      if(!Live(firing.component,firing.identity))return Fail(2453,"server_combat_firing_component_expired_before_retirement");
      const auto objects=Read<void**>(firing.component,0x500);
      const auto count=Read<int32_t>(firing.component,0x508),capacity=Read<int32_t>(firing.component,0x50c);
      if(count<0||count>static_cast<int>(MaxEffectsPerState)||capacity<count
         ||(count&&!original_players::Readable(objects,static_cast<size_t>(count)*8)))return Fail(2405,"server_combat_original_effect_array_invalid");
      for(int32_t i=0;i<count;++i){auto object=Read<void*>(objects,static_cast<size_t>(i)*8);
        if(!original_players::IsA(object,0x684d570)||Read<void*>(object,0x148)!=firing.component)continue;
        auto effect=AcquireEffect(s,object,static_cast<uint32_t>(stateIndex));if(!effect)return false;
        CaptureInitial(s,*effect);
      }
    }
    for(auto& reference:s.effects)if(reference&&(!OwnsEffect(s,*reference)||!reference->present))
        if(!ReleaseEffect(s,reference))return false;
    return true;
}
inline bool SourceIdentity(const Source& s){
    const bool owner=policy::Authority(s.index,s.pawn,s.controller)&&s.bound&&Live(s.pawn,s.pawnId)&&Live(s.controller,s.controllerId)&&Live(s.weapon,s.weaponId)
        &&Live(s.inventory,s.inventoryId)
        &&Read<void*>(s.pawn,0x4a0)==s.controller&&Read<void*>(s.controller,0x4b0)==s.pawn
        &&Read<void*>(s.pawn,0xc08)==s.inventory&&Read<void*>(s.inventory,0xc8)==s.pawn
        &&Read<void*>(s.inventory,0x2f0)==s.pawn&&Read<void*>(s.weapon,0x4a8)==s.pawn&&Read<uint8_t>(s.weapon,0x224)==3;
    if(!owner)return false;if(!s.ballistic)return true;
    if(!s.firingCount||!Live(s.ammo,s.ammoId))return false;
    for(size_t i=0;i<s.firingCount;++i){const auto& firing=s.firings[i];
        if(!Live(firing.component,firing.identity)||Read<void*>(firing.component,0xc8)!=s.weapon
           ||Read<void*>(firing.component,0x4c8)!=s.pawn
           ||(s.observing&&(!firing.table.installed||Read<void*>(firing.component,0)!=firing.table.Address())))return false;}
    return true;
}
inline bool InputIdentity(const Source& s){
    return SourceIdentity(s)&&Live(s.input,s.inputId)&&Live(s.inputState,s.inputStateId)
        &&Read<void*>(s.pawn,0xba0)==s.inputState&&Read<void*>(s.input,0xc8)==s.pawn&&Read<void*>(s.input,0x170)==s.pawn
        &&Read<void*>(s.inputState,0xc8)==s.pawn&&Read<void*>(s.inputState,0x148)==s.pawn
        &&Read<void*>(s.input,0)==reinterpret_cast<void*>(engine::image+0xb099220)
        &&Read<void*>(s.inputState,0)==reinterpret_cast<void*>(engine::image+0xb093c10);
}
inline bool OwnedNPCInput(const Source& s){
    // Trigger release still belongs to this real NPC when native equipment
    // has already cleared an old weapon's typed owner. It cannot depend on
    // that retired weapon remaining alive, and never admits a human producer.
    return s.bound&&policy::NPC(s.index)&&policy::Authority(s.index,s.pawn,s.controller)
        &&Live(s.pawn,s.pawnId)&&Live(s.controller,s.controllerId)
        &&Live(s.input,s.inputId)&&Live(s.inputState,s.inputStateId)
        &&Read<void*>(s.pawn,0x230)==s.input&&Read<void*>(s.pawn,0xba0)==s.inputState
        &&Read<void*>(s.input,0xc8)==s.pawn&&Read<void*>(s.input,0x170)==s.pawn
        &&Read<void*>(s.inputState,0xc8)==s.pawn&&Read<void*>(s.inputState,0x148)==s.pawn
        &&Read<void*>(s.input,0)==engine::function<void*>(0xb099220)
        &&Read<void*>(s.inputState,0)==engine::function<void*>(0xb093c10);
}
inline bool OwnedExternalInput(const Source& s){
    return s.bound&&s.externalInputProducer&&Live(s.pawn,s.pawnId)&&Live(s.controller,s.controllerId)&&Live(s.inputState,s.inputStateId)
        &&ExternalNPCProducerOwns(s.index,s.controller,s.pawn,s.spawnEpoch,s.inputState);
}
inline bool ExternalInputIdentity(const Source& s){return SourceIdentity(s)&&OwnedExternalInput(s);}
inline int ExternalActionDependencies(Source& s){
    if(!OwnedExternalInput(s)){Fail(4552,"server_combat_external_npc_producer_identity_expired",true);return -1;}
    if(!externalFunctionsReady||!Live(externalMathDefault,externalMathIdentity))return 0;
    const auto movement=s.externalMovement;
    if(!Live(movement,s.externalMovementId)||Read<void*>(s.pawn,0x4f8)!=movement
       ||Read<void*>(movement,0xc8)!=s.pawn||Read<void*>(movement,0x190)!=s.pawn||Read<void*>(movement,0x1a8)!=s.pawn
       ||Read<void*>(movement,0x7f0)!=s.externalPrediction||!s.externalPrediction
       ||s.externalClock!=static_cast<uint8_t*>(s.externalPrediction)+0xf0
       ||Read<void*>(s.externalClock,0)!=engine::function<void*>(0xb600a60)){
        Fail(4556,"server_combat_external_npc_native_clock_identity_changed",true);return -1;
    }
    const auto vbtable=Read<void*>(s.externalClock,0x10);const auto displacement=Read<int32_t>(vbtable,4);
    if(!vbtable||displacement<0||displacement>0x1000
       ||Read<uint32_t>(s.externalClock,static_cast<size_t>(displacement)+0x1c)!=1
       ||!std::isfinite(Read<float>(s.externalClock,0x20))){
        Fail(4557,"server_combat_external_npc_native_clock_128hz_contract_changed",true);return -1;
    }
    const auto offset=Read<int32_t>(s.externalClock,0x24);
    bool clockOwned=!externalNPCClockValidator&&offset==0;
    if(externalNPCClockValidator){
        const auto p=local_server_match::Player(s.index);
        if(p){const ExternalNPCClockSource source{s.index,s.spawnEpoch,s.controller,p->ps,s.pawn,s.inputState,
                s.externalMovement,s.externalPrediction,s.externalClock,displacement,offset,1};
            clockOwned=externalNPCClockValidator(source);}
    }
    if(!clockOwned){Fail(4557,"server_combat_external_npc_native_clock_domain_not_owned",true);return -1;}
    auto pawnTable=Read<void**>(s.pawn,0),pcTable=Read<void**>(s.controller,0),inputTable=Read<void**>(s.inputState,0);
    if(Read<void*>(pawnTable,0x7b8)!=engine::function<void*>(0x78802a0)
       ||Read<void*>(pcTable,0x7b0)!=engine::function<void*>(0x42ad260)
       ||Read<void*>(inputTable,0x3f8)!=engine::function<void*>(0x70a7180)){
        Fail(4558,"server_combat_external_npc_native_aim_or_applicator_changed",true);return -1;
    }
    const auto history=Read<int32_t>(s.inputState,0x110),historyCapacity=Read<int32_t>(s.inputState,0x114);
    const auto ringCapacity=Read<int32_t>(s.inputState,0x11c),ringUsed=Read<int32_t>(s.inputState,0x120);
    const auto pending=Read<int32_t>(s.inputState,0xaa0),pendingCapacity=Read<int32_t>(s.inputState,0xaa4);
    if(history<0||history>4096||historyCapacity<history||historyCapacity>65536
       ||ringCapacity<0||ringCapacity>4096||ringUsed<0||ringUsed>ringCapacity
       ||pending<0||pending>4096||pendingCapacity<pending||pendingCapacity>65536
       ||(pending&&!original_players::Readable(Read<void*>(s.inputState,0xa98),static_cast<size_t>(pending)*40))){
        Fail(4559,"server_combat_external_npc_native_history_structure_invalid",true);return -1;
    }
    if(!(Read<uint8_t>(s.inputState,0xa0)&1)||!history||ringCapacity!=history
       ||!original_players::Readable(Read<void*>(s.inputState,0x108),static_cast<size_t>(history)*32))return 0;
    const auto triggerCount=Read<int32_t>(s.inputState,0x160);
    if(triggerCount<0||triggerCount>1024){Fail(4612,"server_combat_external_npc_native_trigger_delegate_invalid",true);return -1;}
    if(!triggerCount||!Read<void*>(s.inputState,0x158))return 0;
    return 1;
}
inline uint32_t ExternalNativeHeld(const Source& s){
    uint32_t held=0;for(size_t input=0;input<3;++input)if(Read<uint8_t>(s.inputState,0x265+input))held|=uint32_t(1)<<input;return held;
}
inline bool ExternalContextMatches(const Source& s){
    const auto current=Read<void*>(s.inventory,0x280);
    const auto correction=Read<int32_t>(s.inventory,0x328),applied=Read<int32_t>(s.inputState,0x260);
    return current==s.weapon&&Live(s.weapon,s.weaponId)&&correction>=0&&applied==correction
        &&Read<void*>(s.inputState,0x250)==current&&Read<void*>(s.inputState,0x258)==current;
}
inline bool NativeAimMatches(const Source& s,const engine::Vec& direction){
    if(!Read<uint8_t>(s.inputState,0x128))return false;
    const auto actual=Read<engine::Vec>(s.inputState,0x238);if(!original_players::Finite(actual)||!original_players::Finite(direction))return false;
    uint8_t left[40]{},right[40]{};left[0]=right[0]=5;std::memcpy(left+8,&actual,24);std::memcpy(right+8,&direction,24);
    return engine::function<bool(*)(const void*,const void*)>(0x707be70)(left,right);
}
inline bool NativeIncomingHistoryMatches(const Source& s,const uint8_t* events,int32_t count){
    if(count<1||count>8)return false;
    for(int32_t n=0;n<count;++n){const auto event=events+static_cast<size_t>(n)*40;const auto kind=event[0];size_t offset=0;
        if(kind==0)offset=0x210+static_cast<size_t>(event[0x10]&15)*40;
        else if(kind==4||kind==5||kind==6||kind==7)offset=8+static_cast<size_t>(kind)*40;
        else return false;
        if(!engine::function<bool(*)(const void*,const void*)>(0x707be70)(static_cast<uint8_t*>(s.inputState)+0x360+offset,event))return false;
    }return true;
}
inline bool NativeIncomingHistoryInvalidated(const Source& s,const uint8_t* events,int32_t count){
    if(count<1||count>8)return false;
    for(int32_t n=0;n<count;++n){const auto event=events+static_cast<size_t>(n)*40;const auto kind=event[0];size_t offset=0;
        if(kind==0)offset=0x210+static_cast<size_t>(event[0x10]&15)*40;
        else if(kind==4||kind==5||kind==6||kind==7)offset=8+static_cast<size_t>(kind)*40;
        else return false;
        if(Read<uint8_t>(s.inputState,0x360+offset)!=0xfe)return false;
    }return true;
}
inline void ObserveExternalApplication(Source& s){
    s.externalObservedHeld=ExternalNativeHeld(s);s.externalNativeStateMatches=s.externalObservedHeld==s.held;
    s.externalObservedEvent=Read<int32_t>(s.inputState,0x120c);
    s.externalNativeAimMatches=s.haveView&&NativeAimMatches(s,s.lastView);
    const bool context=ExternalContextMatches(s);if(!context)s.externalNativeAppliedReady=false;
    if(s.externalPending&&s.externalLastEvent>=0&&Read<int32_t>(s.inputState,0x364)>=s.externalLastEvent
       &&s.externalObservedEvent>=s.externalLastEvent&&s.externalNativeStateMatches){
        s.externalPending=false;++s.externalAppliedBatches;
        if(context&&s.externalNativeAimMatches)s.externalNativeAppliedReady=true;
    }
}
inline bool AppendExternalEvent(uint8_t* buffer,const uint8_t* event){
    const auto count=Read<int32_t>(buffer,0x740),capacity=Read<int32_t>(buffer,0x744);
    if(count<0||count>=8||capacity<count)return Fail(4600,"server_combat_external_npc_temporary_buffer_invalid");
    const int32_t next=count+1;std::memcpy(buffer+0x740,&next,4);
    if(next>capacity)engine::function<void(*)(void*,int)>(0x18f5ce0)(buffer+0x738,count);
    auto data=Read<uint8_t*>(buffer,0x738);
    if(!data||Read<int32_t>(buffer,0x744)<next)return Fail(4601,"server_combat_external_npc_temporary_buffer_allocation_failed");
    std::memcpy(data+static_cast<size_t>(count)*40,event,40);return true;
}
inline bool ExternalAim(Source& s,engine::Vec& direction){
    engine::Vec rotation{};
    const auto result=engine::function<engine::Vec*(*)(void*,engine::Vec*)>(0x78802a0)(s.pawn,&rotation);
    if(result!=&rotation||!original_players::Finite(rotation))return Fail(4602,"server_combat_external_npc_native_aim_rotation_invalid");
    struct Forward {engine::Vec rotation,direction;} parameters{rotation,{}};
    static_assert(sizeof(Forward)==48&&offsetof(Forward,direction)==24);
    if(!original_damage::EventWithFunction(externalMathDefault,L"/Script/Engine.KismetMathLibrary.GetForwardVector",externalForwardFunction,48,&parameters))
        return Fail(4603,"server_combat_external_npc_native_forward_vector_failed");
    const auto norm=parameters.direction.x*parameters.direction.x+parameters.direction.y*parameters.direction.y+parameters.direction.z*parameters.direction.z;
    if(!original_players::Finite(parameters.direction)||norm<.99||norm>1.01)return Fail(4604,"server_combat_external_npc_native_forward_vector_invalid");
    s.controlRotation=rotation;s.nativeView=direction=parameters.direction;return true;
}
inline bool ReleaseExternalInput(Source& s);
inline bool ExternalWeaponInput(Source& s,bool fire,bool reload,bool ads,bool viewOnly=false){
    const int dependencies=ExternalActionDependencies(s);if(dependencies<0)return false;
    s.externalCanSubmit=dependencies>0;
    if(!dependencies){s.inputReady=false;if(s.held)return Fail(4605,"server_combat_external_npc_held_input_not_ready");++diagnostics.inputDeferred;return true;}
    ObserveExternalApplication(s);s.inputReady=s.externalNativeAppliedReady;
    if(!SourceIdentity(s)){
        if(!ReleaseExternalInput(s))return false;
        ++diagnostics.inputDeferred;return true;
    }
    // A bootstrap already in the native queue must reach its existing128Hz
    // BeforeFrame. Repeating the same context before that step is a native
    // no-op after per-kind history coalescing, not a new acknowledged batch.
    if(!s.externalNativeAppliedReady&&s.externalPending&&Read<int32_t>(s.inputState,0xaa0)>0){
        ++diagnostics.inputDeferred;return true;
    }
    const auto player=policy::Player(s.index);
    const bool live=player&&player->registered&&player->alive&&player->pawn==s.pawn&&player->spawnEpoch==s.spawnEpoch;
    const bool active=live&&policy::Active()&&s.observing;
    if(!active||!policy::BotFireAllowed()||!s.externalNativeAppliedReady){fire=false;reload=false;ads=false;}
    if(Read<void*>(s.inventory,0x280)!=s.weapon||!s.physicsReady){fire=false;reload=false;ads=false;}
    const uint32_t desired=viewOnly?s.held:((fire?1U:0U)|(ads?2U:0U)|(reload?4U:0U));
    const auto changed=desired^s.held;
    engine::Vec direction{};bool view=false;
    if(active){if(!ExternalAim(s,direction))return false;view=changed||!s.haveView||!Close(direction,s.lastView)||!s.externalNativeAppliedReady;}
    if(!changed&&!view)return true;
    if(Read<uint8_t>(s.inputState,0x11f5))return Fail(4606,"server_combat_external_npc_reentrant_native_input_producer",true);
    const auto movementTime=engine::function<float(*)(void*)>(0x789df70)(s.externalMovement);
    const auto vbtable=Read<void*>(s.externalClock,0x10);const auto displacement=Read<int32_t>(vbtable,4);
    const auto step=Read<uint32_t>(s.externalClock,static_cast<size_t>(displacement)+0x1c);
    const float deadline=movementTime+static_cast<float>(step)*(1.0f/128.0f);
    if(!std::isfinite(movementTime)||movementTime<0||!std::isfinite(deadline)||deadline<=movementTime)
        return Fail(4607,"server_combat_external_npc_native_action_deadline_invalid");
    const auto beforeCount=Read<int32_t>(s.inputState,0xaa0),beforeBase=Read<int32_t>(s.inputState,0x364);
    if(beforeCount>4000||beforeBase<-1||beforeBase>INT_MAX-4096)return Fail(4608,"server_combat_external_npc_native_action_queue_not_advancing");
    NPCSubmission observation;observation.sequence=local_server_combat::state.npcSubmission.sequence+1;
    observation.index=static_cast<uint32_t>(s.index);observation.binding=s.binding;observation.epoch=s.spawnEpoch;
    observation.desired=desired;observation.changed=changed;observation.beforeBase=beforeBase;observation.beforeCount=beforeCount;
    observation.beforeApplied=Read<int32_t>(s.inputState,0x120c);
    observation.beforeCurrent=Read<void*>(s.inventory,0x280);observation.beforeAuthority=Read<int32_t>(s.inventory,0x328);
    const auto submittedInput=s.inputState;const auto submittedInputId=s.inputStateId;
    alignas(8) uint8_t buffer[0x748]{};engine::function<void*(*)(void*)>(0x70aa150)(buffer);
    uint8_t submittedEvents[8*40]{};bool sent=false;unsigned edges=0;
    __try{
        // Original70A4530's Role3 producer prefixes its actual correction
        // index and actual current-equippable weak identity before the time
        // barrier. Their original applicators own State260/250/258 and tag9.
        const auto correction=Read<int32_t>(s.inventory,0x328);const auto current=Read<void*>(s.inventory,0x280);
        uint8_t event[40]{};event[0]=6;std::memcpy(event+8,&correction,4);
        bool built=correction>=0&&current==s.weapon&&Live(s.weapon,s.weaponId)&&AppendExternalEvent(buffer,event);
        if(built){std::memset(event,0,sizeof(event));event[0]=4;static_assert(sizeof(Weak)==8);std::memcpy(event+8,&s.weaponId,8);built=AppendExternalEvent(buffer,event);}
        if(built){std::memset(event,0,sizeof(event));event[0]=7;std::memcpy(event+8,&deadline,4);built=AppendExternalEvent(buffer,event);}
        if(built&&view){std::memset(event,0,sizeof(event));event[0]=5;std::memcpy(event+8,&direction,24);built=AppendExternalEvent(buffer,event);}
        for(uint8_t input=0;built&&input<3;++input)if(changed&(uint32_t(1)<<input)){
            std::memset(event,0,sizeof(event));event[0]=0;event[0x10]=static_cast<uint8_t>(input|((desired&(uint32_t(1)<<input))?0x10:0));
            const float anchor=-1;std::memcpy(event+0x14,&anchor,4);built=AppendExternalEvent(buffer,event);if(built)++edges;
        }
        if(built){
            observation.built=Read<int32_t>(buffer,0x740);
            if(observation.built<1||observation.built>8)built=false;
            else std::memcpy(submittedEvents,Read<void*>(buffer,0x738),static_cast<size_t>(observation.built)*40);
        }
        if(built){engine::function<void(*)(void*,void*)>(0x70a7110)(submittedInput,buffer);sent=Read<int32_t>(buffer,0x740)==0;}
    }__finally{engine::function<void(*)(void*)>(0x70aa200)(buffer);}
    observation.bufferConsumed=sent;
    observation.sourceChanged=!s.bound||s.binding!=observation.binding||s.spawnEpoch!=observation.epoch||s.inputState!=submittedInput
        ||!original_inventory_slots::SameIdentity(s.inputStateId,submittedInputId);
    if(observation.sourceChanged){
        observation.outcome="retired_during_native_submission";local_server_combat::state.npcSubmission=observation;
        return sent?true:Fail(4609,"server_combat_external_npc_original_submission_not_observed");
    }
    if(!ExternalInputIdentity(s))return Fail(4552,"server_combat_external_npc_producer_identity_expired",true);
    const auto afterCount=Read<int32_t>(s.inputState,0xaa0),afterBase=Read<int32_t>(s.inputState,0x364);
    observation.afterCount=afterCount;observation.afterBase=afterBase;observation.afterApplied=Read<int32_t>(s.inputState,0x120c);
    observation.afterCurrent=Read<void*>(s.inventory,0x280);observation.afterAuthority=Read<int32_t>(s.inventory,0x328);
    observation.historyMatches=sent&&NativeIncomingHistoryMatches(s,submittedEvents,observation.built);
    const bool queueValid=sent&&afterBase>=beforeBase&&afterBase<=INT_MAX-4096&&afterCount>=0&&afterCount<=4096;
    const bool contextChanged=observation.afterCurrent!=observation.beforeCurrent||observation.afterAuthority!=observation.beforeAuthority;
    if(queueValid&&!observation.historyMatches&&!ExternalNativeHeld(s)
       &&((!afterCount&&NativeIncomingHistoryInvalidated(s,submittedEvents,observation.built))||contextChanged)){
        observation.outcome=contextChanged?"native_context_replaced_during_callback":"native_stream_cancelled_during_callback";
        local_server_combat::state.npcSubmission=observation;
        s.held=0;s.externalPending=false;s.externalNativeAppliedReady=false;s.inputReady=false;++s.externalCancelledBatches;return true;
    }
    const bool valid=queueValid&&observation.historyMatches;
    const int64_t beforeEnd=static_cast<int64_t>(beforeBase)+beforeCount,afterEnd=static_cast<int64_t>(afterBase)+afterCount;
    if(valid&&afterEnd==beforeEnd&&afterBase==beforeBase&&afterCount==beforeCount){
        observation.outcome="native_history_deduplicated";local_server_combat::state.npcSubmission=observation;
        // Coalescing is an accepted intent, not a new stream event or ACK.
        // Remember its requested held state so a later release is never lost,
        // but preserve the existing pending token and applied-batch counter.
        s.held=desired;if(view){s.haveView=true;s.lastView=direction;}
        ++s.externalDeduplicated;return true;
    }
    if(!valid||afterEnd<=beforeEnd){
        observation.outcome="native_submission_not_retained";local_server_combat::state.npcSubmission=observation;
        return Fail(4609,"server_combat_external_npc_original_submission_not_observed");
    }
    observation.outcome=afterBase==beforeBase?"native_queue_extended":"native_queue_advanced_during_callback";
    local_server_combat::state.npcSubmission=observation;
    s.held=desired;s.inputEdges+=edges;diagnostics.inputEdges+=edges;++s.inputSubmissions;++diagnostics.inputSubmissions;
    if(view){s.haveView=true;s.lastView=direction;++s.viewSubmissions;++diagnostics.viewSubmissions;}
    s.externalMovementTime=movementTime;s.externalDeadline=deadline;s.externalSubmittedAt=Now();
    ++s.externalBatches;s.externalPending=true;s.externalLastEvent=static_cast<int32_t>(afterEnd);
    ObserveExternalApplication(s);s.inputReady=s.externalCanSubmit&&s.externalNativeAppliedReady;
    if(!s.externalPending){local_server_combat::state.npcSubmission.outcome="native_application_observed_during_callback";}
    return true;
}
inline bool ReleaseExternalInput(Source& s){
    if(!s.held&&!s.externalPending)return true;
    if(!OwnedExternalInput(s))return Fail(4555,"server_combat_npc_held_input_lost_before_release",true);
    if(Read<uint8_t>(s.inputState,0x11f5))return Fail(4610,"server_combat_external_npc_retire_during_native_application",true);
    // Native InputState cancellation emits release delegates for held inputs,
    // invalidates its own histories and cancels the original queued stream.
    // It has no local-controller prerequisite and performs no physics step.
    engine::function<void(*)(void*)>(0x70a8490)(s.inputState);
    if(ExternalNativeHeld(s)||Read<int32_t>(s.inputState,0xaa0))return Fail(4611,"server_combat_external_npc_native_release_not_observed",true);
    if(s.externalPending)++s.externalCancelledBatches;
    s.held=0;s.externalPending=false;s.externalNativeStateMatches=true;s.externalNativeAppliedReady=false;s.inputReady=false;++s.externalResets;return true;
}
inline bool PumpInput(Source& s){
    if(!OwnedNPCInput(s))return Fail(2410,"server_combat_original_character_input_identity_invalid");
    if(!policy::NPC(s.index)||!policy::Authority(s.index,s.pawn,s.controller))return Fail(4511,"server_combat_input_submission_requires_actual_npc_authority");
    const auto count=Read<int32_t>(s.input,0x8b8),capacity=Read<int32_t>(s.input,0x8bc);
    if(count<0||count>1024||capacity<count||(count&&!Read<void*>(s.input,0x8b0)))return Fail(2412,"server_combat_original_action_buffer_invalid");
    if(!count)return true;
    engine::function<void(*)(void*,void*)>(0x70a7110)(s.inputState,static_cast<uint8_t*>(s.input)+0x178);
    if(Read<int32_t>(s.input,0x8b8))return Fail(2413,"server_combat_original_action_buffer_not_consumed");
    ++s.inputSubmissions;++diagnostics.inputSubmissions;return true;
}
inline bool SubmitViewInputImpl(size_t index){
    if(!Thread()||index>=sources.size()||!policy::NPC(index))return false;auto& s=sources[index];
    if(s.externalInputProducer)return ExternalWeaponInput(s,false,false,false,true);
    if(!InputIdentity(s))return Fail(2414,"server_combat_original_view_input_identity_invalid");
    const auto history=Read<int32_t>(s.inputState,0x11c);
    if(!Read<void*>(s.inputState,0x108)||history<=0||history>4096)return Fail(2415,"server_combat_original_aim_history_not_ready");
    s.controlRotation=Read<engine::Vec>(s.controller,0x4e8);
    engine::Vec direction{};
    engine::function<engine::Vec*(*)(void*,engine::Vec*,void*)>(0x70a5420)(s.input,&direction,s.controller);
    const double norm=direction.x*direction.x+direction.y*direction.y+direction.z*direction.z;
    if(!original_shots::Finite(direction)||norm<.99||norm>1.01)return Fail(2416,"server_combat_original_view_sampler_invalid");
    s.nativeView=direction;s.aimValid=Read<uint8_t>(s.inputState,0x128)!=0;s.observedAim=Read<engine::Vec>(s.inputState,0x238);
    if(s.aimValid&&s.haveView&&Close(s.observedAim,s.lastView))++s.viewObservedMatches;
    if(s.haveView&&Close(direction,s.lastView))return true;
    const auto pending=Read<int32_t>(s.inputState,0xaa0),capacity=Read<int32_t>(s.inputState,0xaa4);
    if(pending<0||pending>1024||capacity<pending||(pending&&!Read<void*>(s.inputState,0xa98)))return Fail(2417,"server_combat_original_aim_pending_invalid");
    struct ViewEvent{uint8_t type=5,padding[7]{};engine::Vec direction;uint8_t tail[8]{};} event{5,{},direction,{}};
    static_assert(sizeof(ViewEvent)==40&&offsetof(ViewEvent,direction)==8);
    alignas(8) uint8_t buffer[0x748]{};
    engine::function<void*(*)(void*)>(0x70aa150)(buffer);bool sent=false;
    __try{
        const int32_t one=1;std::memcpy(buffer+0x740,&one,4);
        engine::function<void(*)(void*,int)>(0x18f5ce0)(buffer+0x738,0);
        auto data=Read<void*>(buffer,0x738);
        if(data&&Read<int32_t>(buffer,0x744)>=1){
            std::memcpy(data,&event,sizeof(event));engine::function<void(*)(void*,const void*)>(0x70a9a90)(buffer,&event);
            engine::function<void(*)(void*,void*)>(0x70a7110)(s.inputState,buffer);sent=Read<int32_t>(buffer,0x740)==0;
        }
    }__finally{engine::function<void(*)(void*)>(0x70aa200)(buffer);}
    if(!sent||Read<int32_t>(s.inputState,0xaa0)<pending)return Fail(2418,"server_combat_original_aim_append_failed");
    s.haveView=true;s.lastView=direction;++s.viewSubmissions;++diagnostics.viewSubmissions;return true;
}
inline bool SubmitViewInput(size_t index){__try{return SubmitViewInputImpl(index);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool InputEdges(Source& s,bool fire,bool reload,bool ads=false){
    if(!InputIdentity(s))return Fail(2419,"server_combat_original_force_input_context_invalid");
    const uint32_t desired=(fire?1U:0U)|(ads?2U:0U)|(reload?4U:0U);
    for(uint8_t input=0;input<3;++input){const uint32_t bit=uint32_t(1)<<input;
        if((desired&bit)==(s.held&bit))continue;
        struct Force{uint8_t input,state;} parameters{input,static_cast<uint8_t>((desired&bit)?1:0)};
        if(!original_damage::EventWithFunction(s.pawn,L"/Script/ShooterGame.ShooterCharacter.ForceInputState",forceInputFunction,2,&parameters))
            return Fail(2420,"server_combat_original_force_input_failed");
        s.held=(s.held&~bit)|(desired&bit);++s.inputEdges;++diagnostics.inputEdges;
    }
    return PumpInput(s);
}
inline bool ReleaseAllInput(Source& s){
    if(!OwnedNPCInput(s))return Fail(2442,"server_combat_release_requires_original_input_identity");
    // The local physical producer can also have pressed ADS. Release every
    // original character trigger once at retirement, regardless of our cache.
    for(uint8_t input=0;input<3;++input){
        struct Force{uint8_t input,state;} parameters{input,0};
        if(!original_damage::EventWithFunction(s.pawn,L"/Script/ShooterGame.ShooterCharacter.ForceInputState",forceInputFunction,2,&parameters))
            return Fail(2443,"server_combat_original_release_input_failed");
        ++s.inputEdges;++diagnostics.inputEdges;
    }
    s.held=0;return PumpInput(s);
}
inline bool WeaponInputImpl(size_t index,bool fire,bool reload,bool ads){
    if(!Thread()||index>=sources.size()||!sources[index].bound||!policy::NPC(index))return false;auto& s=sources[index];
    if(s.externalInputProducer)return ExternalWeaponInput(s,fire,reload,ads);
    const auto player=policy::Player(index);
    const bool live=player&&player->registered&&player->alive&&player->pawn==s.pawn&&player->spawnEpoch==s.spawnEpoch;
    const bool active=live&&policy::Active()&&s.observing;
    if(!active||(!player->local&&!policy::BotFireAllowed())){fire=false;reload=false;ads=false;}
    // Release always works while an original ability is current. New trigger
    // presses wait for this binding's real gun to be equipped and body ready.
    auto current=Read<void*>(s.inventory,0x280);
    const bool currentAllowed=current==s.weapon||(player&&player->local&&policy::AbilitiesAllowed()
        &&original_players::IsA(current,0x645c660)&&Read<void*>(current,0x4a8)==s.pawn);
    if(!currentAllowed||!s.physicsReady||!s.inputReady){fire=false;ads=false;}
    if(!s.inputReady){reload=false;++diagnostics.inputDeferred;}
    if(active&&s.inputReady&&!SubmitViewInputImpl(index))return false;
    return InputEdges(s,fire,reload,ads);
}
inline bool WeaponInput(size_t index,bool fire,bool reload){__try{return WeaponInputImpl(index,fire,reload,false);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool WeaponInput(size_t index,bool fire,bool reload,bool ads){__try{return WeaponInputImpl(index,fire,reload,ads);}__except(CaptureException(GetExceptionInformation())){return false;}}

inline bool CacheTuning(Source& s,FiringContext& firing,original_damage_tuning::Calculation* calculation=nullptr,
                        const original_shots::Shot* shot=nullptr,const original_damage::Hit* hit=nullptr){
    // The old tuning adapter is a singleton. Save/restore it on this one game
    // thread, with one cache per exact firing component. Never share local AK
    // damage data with a bot's different gun or projectile tuning.
    const auto saved=original_damage_tuning::diagnostics;const auto savedThread=original_damage_tuning::gameThread;
    original_damage_tuning::diagnostics=firing.tuningCached?firing.tuning:original_damage_tuning::Diagnostics{};
    original_damage_tuning::gameThread=gameThread;
    bool okay=true;
    if(!firing.tuningCached||original_damage_tuning::diagnostics.firing!=firing.component
       ||original_damage_tuning::diagnostics.projectileClass!=Read<void*>(firing.component,0x550)){
        original_damage_tuning::diagnostics={};original_damage_tuning::gameThread=gameThread;
        okay=original_damage_tuning::Initialize(firing.component);
    }
    if(okay&&calculation&&shot&&hit)okay=original_damage_tuning::Calculate(*shot,*hit,*calculation);
    firing.tuning=original_damage_tuning::diagnostics;firing.tuningCached=okay&&firing.tuning.ready;
    // These compatibility summaries describe the most recent actual source;
    // all future calculations select the explicit firing context again.
    s.tuning=firing.tuning;s.tuningCached=firing.tuningCached;
    const auto error=firing.tuning.error;const auto stage=firing.tuning.stage;const bool fatal=firing.tuning.restartRequired;
    original_damage_tuning::diagnostics=saved;original_damage_tuning::gameThread=savedThread;
    return okay?true:Fail(error?error:2421,stage,fatal);
}
inline bool RefreshFiringResources(FiringContext& firing,bool force){
    const auto resourceOverride=Read<void*>(firing.component,0x498),stabilityOverride=Read<void*>(firing.component,0x490);
    if(!force&&firing.resourceOverride==resourceOverride&&firing.stabilityOverride==stabilityOverride
       &&Live(firing.resource,firing.resourceIdentity)&&Live(firing.stability,firing.stabilityIdentity))return true;
    void *resource=nullptr,*stability=nullptr;
    if(!original_damage::EventWithFunction(firing.component,L"/Script/ShooterGame.FiringStateComponent.GetResourceComponent",resourceFunction,8,&resource)
       ||!original_damage::EventWithFunction(firing.component,L"/Script/ShooterGame.FiringStateComponent.GetStabilityComponent",stabilityFunction,8,&stability))
        return Fail(2454,"server_combat_original_firing_dependency_query_failed");
    ++firing.resourceQueries;firing.resourceOverride=resourceOverride;firing.stabilityOverride=stabilityOverride;
    if((resource&&!original_players::IsA(resource,0x637ed10))||(stability&&!original_players::IsA(stability,0x6d1e460)))
        return Fail(2455,"server_combat_original_firing_dependency_wrong_class");
    if(resource!=firing.resource||!Live(resource,firing.resourceIdentity)){
        firing.resource=resource;firing.resourceIdentity={};
        if(resource&&!Identity(resource,firing.resourceIdentity))return Fail(2456,"server_combat_original_resource_weak_identity_failed");}
    if(stability!=firing.stability||!Live(stability,firing.stabilityIdentity)){
        firing.stability=stability;firing.stabilityIdentity={};
        if(stability&&!Identity(stability,firing.stabilityIdentity))return Fail(2457,"server_combat_original_stability_weak_identity_failed");}
    firing.resourceIsAmmo=resource&&original_players::IsA(resource,0x63c7e00);return true;
}
inline bool InstallFiringObservers(Source& source){
    if(!geometryFunction)return Fail(4650,"server_combat_original_geometry_function_missing");
    original_damage::Name nativeName;std::memcpy(nativeName.bytes,static_cast<uint8_t*>(geometryFunction)+0x18,sizeof(nativeName));
    for(size_t n=0;n<source.firingCount;++n){auto& firing=source.firings[n];
        for(const auto& other:sources)if(other.bound&& &other!=&source)
            for(size_t j=0;j<other.firingCount;++j)if(other.firings[j].component==firing.component)
                return Fail(4651,"server_combat_original_firing_component_has_two_sources",true);
        // Use the original per-object function lookup, including a real BP
        // override when present. The name comes from the original function;
        // no synthetic name or delegate target is introduced.
        firing.geometryFunction=engine::function<void*(*)(void*,const original_damage::Name*)>(0x1cdc710)(firing.component,&nativeName);
        firing.geometryFunctionFlags=Read<uint32_t>(firing.geometryFunction,0xb8);
        if(!Identity(firing.geometryFunction,firing.geometryFunctionId)||Read<uint16_t>(firing.geometryFunction,0xbe)!=8
           ||std::memcmp(static_cast<uint8_t*>(firing.geometryFunction)+0x18,nativeName.bytes,sizeof(nativeName))!=0
           ||(firing.geometryFunctionFlags&0x48020800U)!=0x48020800U||(firing.geometryFunctionFlags&0xc0U)
           ||!original_players::Derived(Read<void*>(firing.component,0x10),Read<void*>(firing.geometryFunction,0x28)))
            return Fail(4652,"server_combat_original_geometry_override_abi_mismatch");
        if(!CopyVtable(firing.component,firing.table,0x298/8+1)
           ||Read<void*>(firing.table.previous,0x298)!=engine::function<void*>(0x1cdc850))
            return Fail(4653,"server_combat_original_firing_pe_owner_conflict");
        firing.forward=engine::function<NativeEvent>(0x1cdc850);
        if(!Install(firing.component,firing.table,0x298/8,reinterpret_cast<void*>(ObserveFiringEvent)))
            return Fail(4654,"server_combat_original_firing_pe_install_failed");
        ++diagnostics.firingObservers;
    }
    return true;
}
inline bool GatherFiringStates(Source& s){
    struct Components {void* type;void** data=nullptr;int32_t count=0,capacity=0;} query{engine::Class(0x684d4c0)};
    static_assert(sizeof(Components)==24&&offsetof(Components,data)==8&&offsetof(Components,count)==16);
    bool valid=false;s.firingCount=0;
    __try{
        valid=original_damage::EventWithFunction(s.weapon,L"/Script/Engine.Actor.K2_GetComponentsByClass",componentsFunction,24,&query)
            &&query.count>=0&&query.count<=static_cast<int32_t>(MaxFiringStates)&&query.capacity>=query.count
            &&(!query.count||original_players::Readable(query.data,static_cast<size_t>(query.count)*8));
        if(valid)for(int32_t i=0;i<query.count;++i){
            auto component=Read<void*>(query.data,static_cast<size_t>(i)*8);
            if(!original_players::IsA(component,0x684d4c0)||Read<void*>(component,0xc8)!=s.weapon
               ||Read<void*>(component,0x4c8)!=s.pawn){valid=false;break;}
            bool duplicate=false;for(size_t j=0;j<s.firingCount;++j)duplicate|=s.firings[j].component==component;
            if(duplicate)continue;
            auto& context=s.firings[s.firingCount];context={};context.component=component;
            original_weapon::OriginalObjectName(component,context.name);
            if(!Identity(component,context.identity)||!RefreshFiringResources(context,true)){valid=false;break;}
            ++s.firingCount;
        }
    }__finally{if(query.data)engine::function<void(*)(void*)>(0x1915720)(query.data);}
    if(!valid)return Fail(diagnostics.error?diagnostics.error:2458,"server_combat_all_original_firing_states_not_ready");
    // Compatibility pointers remain summaries only. No observer or damage
    // calculation uses this first component to infer the shot-producing one.
    s.firing=s.firingCount?s.firings[0].component:nullptr;s.firingId=s.firingCount?s.firings[0].identity:Weak{};
    return true;
}
inline bool RememberRetired(void* object,const Weak& identity){
    if(!object||!Live(object,identity))return true;
    for(auto& r:retired){if(r.actor&&!Live(r.actor,r.identity))r={};
        if(r.actor==object&&original_inventory_slots::SameIdentity(r.identity,identity))return true;}
    for(auto& r:retired)if(!r.actor){r={object,identity};return true;}
    return Fail(2422,"server_combat_live_retired_actor_cache_full");
}
inline bool BeforeRetireImpl(size_t index,RetireReason reason){
    if(!Thread()||index>=sources.size())return false;auto& s=sources[index];if(!s.bound)return true;
    const auto player=policy::Player(index);
    if(player&&!player->local&&pawnRetirementObserver
       &&!pawnRetirementObserver(index,s.pawn,s.spawnEpoch)){
        diagnostics.stage="server_combat_waiting_upper_npc_pawn_observer_retirement";return false;
    }
    s.observing=false;
    if(policy::NPC(s.index)){
        if(s.externalInputProducer){if(!ReleaseExternalInput(s))return false;}
        else if(OwnedNPCInput(s)){if(!ReleaseAllInput(s))return false;}
        else if(s.held)return Fail(4555,"server_combat_npc_held_input_lost_before_release",true);
    }
    for(size_t n=0;n<s.firingCount;++n){auto& firing=s.firings[n];const bool installed=firing.table.installed;
        if(!Restore(firing.component,firing.identity,firing.table))return false;
        if(installed)++diagnostics.firingTablesRestored;}
    for(auto& reference:s.effects)if(!ReleaseEffect(s,reference))return false;
    const bool installed=s.pawnTable.installed||s.inputViewLease;
    if(s.inputViewLease){
        if(!local_server_input::ClearViewLocationObserver(s.pawn,ObserveViewLocation))return Fail(4534,"server_combat_input_owner_origin_retirement_failed");
        s.inputViewLease=false;
    }else if(!Restore(s.pawn,s.pawnId,s.pawnTable))return false;
    if(installed)++diagnostics.restoredPawnTables;
    // A live Pawn that is merely switching guns remains a real damage target.
    // Do not put it in a corpse/scene-ignore cache. Its damage binding survives.
    if(reason!=RetireReason::WeaponChange&&!original_damage::targets[index].alive
       &&!RememberRetired(s.pawn,s.pawnId))return false;
    if(!RememberRetired(s.weapon,s.weaponId))return false;
    // All callbacks have been removed before any address/weak identity is
    // reused. Keep table backing storage allocated for the whole owned session.
    for(auto& reference:s.effects)reference=nullptr;
    for(auto& context:s.firings)context={};s.firingCount=0;s.lastFiringSlot=-1;
    for(auto& origin:s.origins)origin={};for(auto& ray:s.recent)ray={};
    s.bound=false;s.alive=false;s.physicsReady=false;s.inputReady=false;s.current=false;s.tuningCached=false;s.haveView=false;s.held=0;
    s.inputRestartAttempted=false;s.externalInputProducer=false;s.externalPending=false;s.restartPawn=nullptr;s.restartPawnId={};
    s.externalMovement=s.externalClock=s.externalPrediction=nullptr;s.externalMovementId={};
    s.pawn=s.controller=s.weapon=s.inventory=s.firing=s.ammo=s.input=s.inputState=nullptr;
    s.pawnId=s.controllerId=s.weaponId=s.inventoryId=s.firingId=s.ammoId=s.inputId=s.inputStateId={};
    s.forwardView=nullptr;s.pawnTable.previous=nullptr;s.pawnTable.functions=0;
    ++diagnostics.retirements;return true;
}
inline bool BeforeRetire(size_t index,RetireReason reason){__try{return BeforeRetireImpl(index,reason);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool Refresh(Source& s,double now,bool force=false){
    if(!SourceIdentity(s))return Fail(2423,"server_combat_original_source_identity_expired",true);
    const auto player=policy::Player(s.index);
    if(player&&player->registered&&(player->pawn!=s.pawn||player->spawnEpoch!=s.spawnEpoch))return Fail(2424,"server_combat_roster_epoch_changed_before_retire",true);
    s.current=Read<void*>(s.inventory,0x280)==s.weapon;
    const auto historyCapacity=Read<int32_t>(s.inputState,0x11c);
    if(s.externalInputProducer&&!ExternalInputIdentity(s))return Fail(4552,"server_combat_external_npc_producer_identity_expired",true);
    if(s.externalInputProducer){
        const int dependencies=ExternalActionDependencies(s);if(dependencies<0)return false;s.externalCanSubmit=dependencies>0;
        ObserveExternalApplication(s);
        s.inputReady=s.externalCanSubmit&&s.externalNativeAppliedReady;
    }else s.inputReady=(policy::NPC(s.index)?InputIdentity(s):Live(s.inputState,s.inputStateId))
        &&Read<void*>(s.inputState,0x108)&&historyCapacity>0&&historyCapacity<=4096;
    s.aimValid=Read<uint8_t>(s.inputState,0x128)!=0;s.observedAim=Read<engine::Vec>(s.inputState,0x238);
    if(s.haveView&&s.aimValid&&Close(s.observedAim,s.lastView))++s.viewObservedMatches;
    if(force||now-s.sampledAt>=options.sampleInterval){
        for(size_t i=0;i<s.firingCount;++i)if(!RefreshFiringResources(s.firings[i]))return false;
        if(!original_damage::Sample(s.index))return Fail(original_damage::State().error,original_damage::State().stage,original_damage::State().restartRequired);
        const auto& target=original_damage::targets[s.index];s.alive=target.alive;s.physicsReady=target.originalPhysics;
        s.bodyCount=target.bodyCount;s.physicsActors=target.physicsActors;s.ammoCount=s.ballistic?Read<int32_t>(s.ammo,0x10c):-1;
        if(s.lastFiringSlot>=0&&static_cast<size_t>(s.lastFiringSlot)<s.firingCount){const auto& firing=s.firings[static_cast<size_t>(s.lastFiringSlot)];
            s.ammoCount=firing.resourceIsAmmo&&Live(firing.resource,firing.resourceIdentity)?Read<int32_t>(firing.resource,0x10c):-1;}
        s.sampledAt=now;
    }
    return true;
}
inline bool PrepareOriginalInput(Source& s){
    if(!policy::Authority(s.index,s.pawn,s.controller))return Fail(4544,"server_combat_requires_actual_authority_possession");
    // Human action state is supplied by the original client RPC/native stream
    // authority adapter. Never construct, submit, restart or release its input.
    if(!policy::NPC(s.index)){
        s.input=original_weapon::Component(s.pawn,0x6492f30);
        s.inputState=Read<void*>(s.pawn,0xba0);
        return original_players::IsA(s.inputState,0x6494370);
    }
    if(externalNPCProducer){
        s.inputState=Read<void*>(s.pawn,0xba0);
        if(!ExternalNPCProducerOwns(s.index,s.controller,s.pawn,s.spawnEpoch,s.inputState))
            return Fail(4552,"server_combat_external_npc_producer_identity_expired",true);
        // The authority scheduler already drives this original state. A local
        // AresInput component/ClientRestart is neither required nor created.
        s.externalMovement=Read<void*>(s.pawn,0x4f8);s.externalPrediction=Read<void*>(s.externalMovement,0x7f0);
        if(!original_players::IsA(s.externalMovement,0x6c7d380)||!Identity(s.externalMovement,s.externalMovementId))
            return Fail(4613,"server_combat_external_npc_native_movement_identity_invalid",true);
        s.externalClock=engine::function<void*(*)(void*)>(0x7892e10)(s.externalMovement);
        if(!s.externalPrediction||s.externalClock!=static_cast<uint8_t*>(s.externalPrediction)+0xf0)
            return Fail(4614,"server_combat_external_npc_native_clock_resolution_failed",true);
        s.input=nullptr;s.externalInputProducer=true;s.inputReady=false;s.externalPending=false;s.externalNativeStateMatches=false;
        s.externalCanSubmit=false;s.externalNativeAppliedReady=false;s.externalNativeAimMatches=false;return true;
    }
    const auto human=policy::Player(0);const auto humanPC=human?human->controller:nullptr;
    const auto humanConnection=Read<void*>(humanPC,0x510);
    if(!Read<void*>(s.controller,0x5e8)||!Read<void*>(s.controller,0x230)){
        auto table=Read<void**>(s.controller,0);
        if(Read<void*>(table,0xfe8)!=engine::function<void*>(0x781cbd0))return Fail(4545,"server_combat_npc_native_input_initializer_changed");
        engine::function<void(*)(void*)>(0x781cbd0)(s.controller);++s.pcInputInitializations;
    }
    auto input=original_weapon::Component(s.pawn,0x6492f30);
    if(!input){
        if(Read<void*>(s.pawn,0x230))return Fail(4547,"server_combat_npc_existing_input_is_not_ares");
        if(s.restartPawn!=s.pawn||!Live(s.restartPawn,s.restartPawnId)){
            s.restartPawn=s.pawn;s.inputRestartAttempted=false;if(!Identity(s.pawn,s.restartPawnId))return false;
        }
        if(s.inputRestartAttempted)return Fail(4548,"server_combat_npc_native_input_pending");
        auto table=Read<void**>(s.pawn,0);
        if(Read<void*>(table,0x828)!=engine::function<void*>(0x786d960)
           ||Read<void*>(table,0x888)!=engine::function<void*>(0x787c520)
           ||Read<void*>(table,0x898)!=engine::function<void*>(0x787c8d0))return Fail(4549,"server_combat_npc_input_lifecycle_changed");
        s.inputRestartAttempted=true;engine::function<void(*)(void*)>(0x786d960)(s.pawn);++s.pawnClientRestarts;
        input=original_weapon::Component(s.pawn,0x6492f30);
    }
    if(!policy::NPC(s.index)||!policy::Authority(s.index,s.pawn,s.controller)
       ||Read<void*>(humanPC,0x510)!=humanConnection)return Fail(4550,"server_combat_npc_input_changed_ownership",true);
    s.input=input;s.inputState=Read<void*>(s.pawn,0xba0);
    return original_players::IsA(s.input,0x6492f30)&&Read<void*>(s.pawn,0x230)==s.input
        &&original_players::IsA(s.inputState,0x6494370)
        ?true:Fail(4551,"server_combat_npc_original_input_not_ready");
}
inline bool BindImpl(size_t index,void* pawn,void* weapon,uint32_t spawnEpoch){
    if(!Thread()||index>=sources.size()||!spawnEpoch)return Fail(2425,"server_combat_bind_context_invalid");
    auto& s=sources[index];
    if(s.bound){if(s.pawn==pawn&&s.weapon==weapon&&s.spawnEpoch==spawnEpoch)return Refresh(s,Now(),true);
        return Fail(2426,"server_combat_before_retire_required_for_new_binding");}
    if(!original_players::IsA(pawn,original_players::ShooterCharacterClass)||!original_players::IsA(weapon,0x645c660)
       ||Read<void*>(weapon,0x4a8)!=pawn)return Fail(2427,"server_combat_bind_requires_original_owned_gun");
    s.index=index;s.spawnEpoch=spawnEpoch;s.binding=++nextBinding;s.pawn=pawn;s.weapon=weapon;
    s.controller=Read<void*>(pawn,0x4a0);s.inventory=Read<void*>(pawn,0xc08);
    s.ammo=original_weapon::Component(weapon,0x63c7e00);
    if(!GatherFiringStates(s))return false;
    s.ballistic=s.firingCount>0;
    if(!s.ballistic){
        // The authored melee equippable is a real non-ballistic context. It
        // keeps native trigger/animation and can be damaged; no fake ranged
        // component or guessed melee hit/damage is supplied by this bridge.
        auto melee=engine::Find(L"/Game/Equippables/Melee/Ability_Melee_Base.Ability_Melee_Base_C");
        if(s.firing||s.ammo||!melee||!original_players::Derived(Read<void*>(weapon,0x10),melee))
            return Fail(2452,"server_combat_original_firearm_components_not_ready");
    }
    if(!PrepareOriginalInput(s))return false;
    if(!original_players::IsA(s.controller,original_players::ShooterControllerClass)
       ||(s.ballistic&&(!original_players::IsA(s.firing,0x684d4c0)||!original_players::IsA(s.ammo,0x63c7e00)))
       ||(policy::NPC(index)&&!s.externalInputProducer&&!original_players::IsA(s.input,0x6492f30))||!original_players::IsA(s.inputState,0x6494370))
        return Fail(2428,"server_combat_original_pawn_client_input_lifecycle_required");
    if(!Identity(pawn,s.pawnId)||!Identity(s.controller,s.controllerId)||!Identity(weapon,s.weaponId)
       ||!Identity(s.inventory,s.inventoryId)||(s.ballistic&&(!Identity(s.firing,s.firingId)||!Identity(s.ammo,s.ammoId)))
       ||(policy::NPC(index)&&!s.externalInputProducer&&!Identity(s.input,s.inputId))||!Identity(s.inputState,s.inputStateId))return Fail(2429,"server_combat_original_weak_identity_failed");
    s.bound=true;s.observing=false;s.held=0;s.haveView=false;s.tuningCached=false;s.sampledAt=-1000;
    s.originCursor=s.recentCursor=s.rayCursor=0;for(auto& r:s.origins)r={};for(auto& r:s.recent)r={};
    if(!policy::Authority(index,pawn,s.controller)||(policy::NPC(index)&&!(s.externalInputProducer?ExternalInputIdentity(s):InputIdentity(s)))||!original_damage::Bind(index,pawn)){
        s.bound=false;return Fail(diagnostics.error?diagnostics.error:2430,"server_combat_native_input_damage_tuning_bind_failed");}
    if(s.ballistic&&policy::NPC(index)&&!CopyVtable(pawn,s.pawnTable,0x850/8+1)){
        s.bound=false;return Fail(2431,"server_combat_original_complete_pawn_vtable_unavailable");}
    auto currentTable=Read<void**>(pawn,0);
    s.forwardView=reinterpret_cast<NativeView>(Read<void*>(currentTable,0x850));
    if(s.forwardView!=engine::function<NativeView>(0x787f820)){
        s.bound=false;return Fail(2432,"server_combat_legacy_or_other_origin_observer_active");}
    if(s.ballistic){
        if(policy::NPC(index)){
            if(!Install(pawn,s.pawnTable,0x850/8,reinterpret_cast<void*>(ObserveViewLocation))){s.bound=false;return Fail(2433,"server_combat_npc_origin_install_failed");}
        }else{
            NativeView previous=nullptr;
            if(!local_server_input::SetViewLocationObserver(pawn,ObserveViewLocation,&previous)){
                s.bound=false;return Fail(4533,"server_combat_input_owner_origin_lease_failed");
            }
            s.forwardView=previous;s.inputViewLease=true;
        }
    }
    if(s.ballistic&&!InstallFiringObservers(s)){
        const auto savedError=diagnostics.error;const auto savedStage=diagnostics.stage;
        if(!BeforeRetire(index,RetireReason::WeaponChange))return false;
        return Fail(savedError,savedStage);
    }
    s.observing=true;++diagnostics.binds;
    if(!Refresh(s,Now(),true))return false;
    diagnostics.error=0;diagnostics.stage="server_combat_original_source_bound";return true;
}
inline bool Bind(size_t index,void* pawn,void* weapon,uint32_t spawnEpoch){__try{return BindImpl(index,pawn,weapon,spawnEpoch);}__except(CaptureException(GetExceptionInformation())){return false;}}

inline int SceneTrace(const engine::Vec& start,const engine::Vec& direction,double maximum,double& distance){
    std::array<void*,MaxPlayers*2+RetiredCapacity> ignored{};int32_t count=0;
    const auto add=[&](void* actor){if(!actor)return;for(int32_t i=0;i<count;++i)if(ignored[i]==actor)return;
        if(static_cast<size_t>(count)<ignored.size())ignored[count++]=actor;};
    for(const auto& s:sources)if(s.bound){
        // Live, unready original bodies stay native scene obstructions. The
        // pending shot is deferred before tracing; no capsule can award damage.
        if((!s.alive||s.physicsReady)&&Live(s.pawn,s.pawnId))add(s.pawn);
        if(Live(s.weapon,s.weaponId))add(s.weapon);
    }
    for(auto& r:retired)if(r.actor){if(Live(r.actor,r.identity))add(r.actor);else r={};}
    alignas(16) uint8_t parameters[0x178]{};const auto end=engine::Add(start,engine::Mul(direction,maximum));
    std::memcpy(parameters,&world,8);std::memcpy(parameters+8,&start,24);std::memcpy(parameters+0x20,&end,24);
    parameters[0x39]=1;parameters[0x148]=1;
    struct Actors{void** data;int32_t count,capacity;} actors{ignored.data(),count,count};
    std::memcpy(parameters+0x40,&actors,sizeof(actors));++diagnostics.sceneTraces;
    if(!Event(engine::DefaultObject(engine::Class(0x45aa110)),L"/Script/Engine.KismetSystemLibrary.LineTraceSingle",0x171,parameters))return -1;
    if(!parameters[0x170]||!(parameters[0x105]&1)){distance=(std::numeric_limits<double>::infinity)();return 0;}
    distance=Read<float>(parameters,0x60);return std::isfinite(distance)&&distance>=0?1:-1;
}

// Diagnostic-only receipts. A receipt records an existing Process branch;
// no extra engine call, ray, eligibility change or damage write is permitted.
enum class RayReason:uint8_t {
    Unknown,InvalidSource,ExpiredBinding,ChangedProjectile,Stale,RefreshFailure,
    SourceInactive,PhysicsDeferred,NativeLifeChanged,BindingDeferred,BodyTraceFailure,
    BodyMiss,SceneTraceFailure,SceneBlocked,PolicyRejected,DamageDeferred,TuningFailure,
    InvalidDamageAmount,BeginDamageFailure,ApplyDamageFailure,CommitDamageFailure,
    LifeReceiptFailure,RetireFailure,DamageApplied,NativeDuplicate,DamageNotApplied,Count
};
inline const char* RayReasonName(RayReason reason){
    constexpr const char* names[]{"unknown","invalid_source","expired_binding","changed_projectile","stale",
        "refresh_failure","source_inactive","physics_deferred","native_life_changed","binding_deferred",
        "body_trace_failure","body_miss","scene_trace_failure","scene_blocked","policy_rejected",
        "damage_deferred","tuning_failure","invalid_damage_amount","begin_damage_failure","apply_damage_failure",
        "commit_damage_failure","life_receipt_failure","retire_failure","damage_applied","native_duplicate","damage_not_applied"};
    const auto n=static_cast<size_t>(reason);return n<std::size(names)?names[n]:"invalid_reason";
}
struct RayReceipt {
    ShotToken identity;original_shots::Shot shot;
    uint64_t damageToken=0;
    uint32_t eligibleBefore=0,eligibleAfter=0,bodyTraceMask=0,bodyHitMask=0;
    int32_t nearest=-1;uint32_t error=0;
    double completedAt=0,bodyDistance=-1,sceneDistance=-1;
    float damage=0;uint8_t region=255;RayReason reason=RayReason::Unknown;
    bool gateSampled=false,sourceRegistered=false,sourceRosterAlive=false,sourceNativeAlive=false,sourceReady=false;
    bool matchStarted=false,matchPhaseActive=false,matchReady=false,matchEnded=false,damagePhaseActive=false;
    uint32_t sourcePhase=255;double matchStartedAt=-1;
};
struct SourceRaySummary {
    uint64_t attempts=0,deferredAttempts=0,terminals=0,damageEvents=0,kills=0;
    std::array<uint64_t,static_cast<size_t>(RayReason::Count)> reasons{};
};
struct RayDiagnostics {
    std::array<SourceRaySummary,MaxPlayers> sources{};
    std::array<RayReceipt,64> humanTerminals{};
    uint64_t humanTerminalCount=0;
};
inline RayDiagnostics rayDiagnostics;

// Insert inside local_server_combat::firearms after ProcessObserved's types.
// Passive evidence only: a rejected model receipt never changes real damage.
inline bool PublishVisibleImpactCandidate(const Pending& pending,const Source& source,size_t victim,
    const original_damage::Hit& hit,const original_damage::Result& result,double now){
    namespace impacts=humanlike_bot_point_impacts;
    const auto& key=pending.identity;
    if(!Thread()||!result.applied||result.duplicate||result.killed||!std::isfinite(result.taken)||result.taken<=0
       ||!result.originalResponse||!result.originalLifeReadback||!result.response
       ||key.source>=MaxPlayers||victim>=MaxPlayers||key.source==victim||(pending.shot.flags&16U)==0
       ||source.index!=key.source||source.binding!=key.binding||source.spawnEpoch!=key.spawnEpoch||!SourceIdentity(source)
       ||!original_damage::IsA(result.response,original_damage::PointResponseClass)
       ||!original_shots::WorldPoint(hit.location))return false;
    const auto s=local_server_match::Player(key.source),v=local_server_match::Player(victim);
    if(!s||!v||!s->present||!v->present||s->epoch!=key.spawnEpoch
       ||v->epoch!=pending.victimEpochs[victim]||s->pawn!=source.pawn||!Live(v->pawn,v->pawnIdentity))return false;
    // Read only provenance and the exact impact from the real constructed
    // PointResponse, not target HP, life section contents or a current pose.
    constexpr size_t base=0xe8;
    const auto originalInstigator=Read<void*>(result.response,base+offsetof(original_damage::ResponseData,instigatorPawn));
    const auto originalWeapon=Read<void*>(result.response,base+offsetof(original_damage::ResponseData,equippable));
    const auto component=Read<void*>(result.response,base+offsetof(original_damage::PointData,hitComponent));
    const auto timestampVictim=Read<Weak>(result.response,base+offsetof(original_damage::ResponseData,timestamp));
    const auto impact=Read<engine::Vec>(result.response,base+offsetof(original_damage::PointData,impact));
    if(originalInstigator!=s->pawn||originalWeapon!=source.weapon||component!=hit.component
       ||timestampVictim.index!=v->pawnIdentity.index||timestampVictim.serial!=v->pawnIdentity.serial
       ||!original_shots::WorldPoint(impact)||impact.x!=hit.location.x||impact.y!=hit.location.y||impact.z!=hit.location.z)return false;
    impacts::ImpactReceipt receipt{};
    receipt.source={static_cast<uint32_t>(s->index),s->epoch,reinterpret_cast<uintptr_t>(s->pawn),s->pawnIdentity.index,s->pawnIdentity.serial};
    receipt.victim={static_cast<uint32_t>(v->index),v->epoch,reinterpret_cast<uintptr_t>(v->pawn),v->pawnIdentity.index,v->pawnIdentity.serial};
    receipt.damageToken=pending.damageToken;receipt.binding=key.binding;receipt.nativeOrigin=key.nativeOrigin;
    receipt.pellet=key.projectile;receipt.firingSlot=key.firingSlot;receipt.acquiredAtSeconds=now;
    receipt.impact={static_cast<float>(impact.x),static_cast<float>(impact.y),static_cast<float>(impact.z)};
    receipt.actualGeometryPaired=(pending.shot.flags&16U)!=0;
    receipt.nativePointResponseVerified=receipt.nativeCommitVerified=true;
    return impacts::Publish(receipt);
}

enum class Outcome:uint8_t{Done,Deferred,Failure};
inline Outcome ProcessObserved(Pending& pending,double now,RayReceipt& receipt){
    const auto finish=[&](RayReason reason,Outcome outcome=Outcome::Done){receipt.reason=reason;receipt.error=diagnostics.error;return outcome;};
    const auto& key=pending.identity;const auto& shot=pending.shot;
    if(key.source>=sources.size())return finish(RayReason::InvalidSource,Outcome::Failure);auto& source=sources[key.source];
    if(!source.bound||source.binding!=key.binding||source.spawnEpoch!=key.spawnEpoch){++diagnostics.expiredBindings;return finish(RayReason::ExpiredBinding);}
    if(key.firingSlot>=source.firingCount){++diagnostics.expiredBindings;return finish(RayReason::ExpiredBinding);}
    auto& firing=source.firings[key.firingSlot];
    if(firing.component!=pending.firing||!original_inventory_slots::SameIdentity(firing.identity,pending.firingIdentity)){
        ++diagnostics.expiredBindings;return finish(RayReason::ExpiredBinding);}
    if(Read<void*>(firing.component,0x550)!=pending.projectileClass){++diagnostics.changedProjectileRejected;return finish(RayReason::ChangedProjectile);}
    if(now-shot.observed_at<-.05||now-shot.observed_at>options.maximumShotAge){++diagnostics.staleShots;return finish(RayReason::Stale);}
    if(!Refresh(source,now,true))return finish(RayReason::RefreshFailure,Outcome::Failure);
    const auto sourcePlayer=policy::Player(key.source);
    const auto& match=policy::State();receipt.gateSampled=true;
    receipt.sourceRegistered=sourcePlayer&&sourcePlayer->registered;
    receipt.sourceRosterAlive=sourcePlayer&&sourcePlayer->alive;
    receipt.sourceReady=sourcePlayer&&sourcePlayer->ready;receipt.sourceNativeAlive=source.alive;
    receipt.sourcePhase=sourcePlayer?static_cast<uint32_t>(sourcePlayer->phase):255;
    receipt.matchStarted=match.started;receipt.matchStartedAt=match.startedAt;
    receipt.matchPhaseActive=match.active;receipt.matchReady=match.ready;receipt.matchEnded=match.ended;
    receipt.damagePhaseActive=policy::Active();
    if(!receipt.sourceRegistered||!receipt.sourceRosterAlive||!receipt.sourceNativeAlive||!receipt.damagePhaseActive)
        return finish(RayReason::SourceInactive);
    if(!source.physicsReady){++diagnostics.physicsDeferred;return finish(RayReason::PhysicsDeferred,Outcome::Deferred);}
    // A native hit may complete in original OnShot after the observed getter.
    // Admission is conservative for the entire ray: do not skip an already
    // damaged/dead front victim and incorrectly damage someone behind it.
    for(size_t n=0;n<MaxPlayers;++n)if(pending.eligible&(uint32_t(1)<<n)){
        if(!NativeLifeAllowsSupplement(n,pending.before[n])){++diagnostics.nativeLifeChanged;return finish(RayReason::NativeLifeChanged);}}
    size_t nearest=sources.size();original_damage::Hit nearestHit;double nearestDistance=options.maximumDistance;
    for(size_t i=0;i<sources.size();++i){if(!(pending.eligible&(uint32_t(1)<<i)))continue;
        auto& target=sources[i];const auto p=policy::Player(i);
        if(!p||!p->registered||p->spawnEpoch!=pending.victimEpochs[i]){
            ++diagnostics.retiredVictimEpochs;pending.eligible&=~(uint32_t(1)<<i);continue;}
        if(!p->alive)continue;
        if(!target.bound){++diagnostics.bindingDeferred;return finish(RayReason::BindingDeferred,Outcome::Deferred);}
        if(target.spawnEpoch!=pending.victimEpochs[i]){
            ++diagnostics.retiredVictimEpochs;pending.eligible&=~(uint32_t(1)<<i);continue;}
        if(!Refresh(target,now,true))return finish(RayReason::RefreshFailure,Outcome::Failure);if(!target.alive)continue;
        if(!target.physicsReady){++diagnostics.physicsDeferred;return finish(RayReason::PhysicsDeferred,Outcome::Deferred);}
        original_damage::Hit hit;++diagnostics.bodyTraces;receipt.bodyTraceMask|=uint32_t(1)<<i;
        const int traced=original_damage::Trace(i,shot,options.maximumDistance,hit);
        if(traced<0){if(original_damage::State().error==1134){target.physicsReady=false;++diagnostics.physicsDeferred;return finish(RayReason::PhysicsDeferred,Outcome::Deferred);}
            Fail(original_damage::State().error,original_damage::State().stage,original_damage::State().restartRequired);return finish(RayReason::BodyTraceFailure,Outcome::Failure);}
        if(traced==1)receipt.bodyHitMask|=uint32_t(1)<<i;
        if(traced==1&&hit.distance<nearestDistance){nearest=i;nearestHit=hit;nearestDistance=hit.distance;
            receipt.nearest=static_cast<int32_t>(i);receipt.bodyDistance=hit.distance;receipt.region=hit.region;}
    }
    if(nearest==sources.size()){++diagnostics.misses;return finish(RayReason::BodyMiss);}
    double sceneDistance=0;const int scene=SceneTrace(shot.start,shot.direction,options.maximumDistance,sceneDistance);
    receipt.sceneDistance=std::isfinite(sceneDistance)?sceneDistance:-1;
    if(scene<0){Fail(2434,"server_combat_original_scene_obstruction_trace_failed");return finish(RayReason::SceneTraceFailure,Outcome::Failure);}
    diagnostics.lastBodyDistance=nearestDistance;diagnostics.lastSceneDistance=std::isfinite(sceneDistance)?sceneDistance:-1;
    if(scene&&sceneDistance+options.obstructionTolerance<nearestDistance){++diagnostics.sceneBlocked;return finish(RayReason::SceneBlocked);}
    if(!policy::CanDamage(key.source,nearest)){++diagnostics.policyRejected;return finish(RayReason::PolicyRejected);}
    if(original_damage::targets[nearest].pendingDamageEvents){++diagnostics.pendingDamageDeferred;return finish(RayReason::DamageDeferred,Outcome::Deferred);}
    original_damage_tuning::Calculation amount;diagnostics.operation="per_source_original_damage_tuning";
    if(!CacheTuning(source,firing,&amount,&shot,&nearestHit))return finish(RayReason::TuningFailure,Outcome::Failure);
    receipt.damage=amount.totalDamage;
    if(!amount.valid||!amount.originalRegionVerified||amount.totalDamage<=0)return finish(RayReason::InvalidDamageAmount);
    original_damage::DeathData death;
    NativeLife beforePoint;
    if(!NativeLifeAllowsSupplement(nearest,pending.before[nearest])||!ReadNativeLife(nearest,beforePoint)){
        ++diagnostics.nativeLifeChanged;return finish(RayReason::NativeLifeChanged);}
    if(!policy::BeginNativeDamage(pending.damageToken,key.source,nearest,source.weapon,now,death)){
        if(!policy::CanDamage(key.source,nearest)){++diagnostics.policyRejected;return finish(RayReason::PolicyRejected);}
        Fail(policy::State().error,policy::State().stage);return finish(RayReason::BeginDamageFailure,Outcome::Failure);}
    original_damage::Result result;diagnostics.operation="original_point_damage_and_native_rule_commit";
    const bool applied=ApplyAuthorityPoint(nearest,source.pawn,source.weapon,amount.damageType,shot,nearestHit,
        amount.totalDamage,pending.damageToken,result,&death,amount.falloffMultiplier);
    // End is mandatory even on a native failure; it clears the transaction.
    const bool committed=policy::EndNativeDamage(applied,result);
    if(!applied||!committed){Fail(!applied?original_damage::State().error:policy::State().error,
        !applied?original_damage::State().stage:policy::State().stage);return finish(!applied?RayReason::ApplyDamageFailure:RayReason::CommitDamageFailure,Outcome::Failure);}
    if(!result.duplicate&&!RecordNativeLifeReceipt(nearest,beforePoint)){
        Fail(4655,"server_combat_native_point_life_receipt_unavailable",true);return finish(RayReason::LifeReceiptFailure,Outcome::Failure);}
    // Optional project-perception evidence only; its failure cannot change
    // the already committed native damage, score, ray receipt or input path.
    (void)PublishVisibleImpactCandidate(pending,source,nearest,nearestHit,result,now);
    diagnostics.lastSource=static_cast<int32_t>(key.source);diagnostics.lastVictim=static_cast<int32_t>(nearest);
    diagnostics.lastToken=pending.damageToken;diagnostics.lastDamage=amount.totalDamage;
    if(result.applied&&!result.duplicate){++source.damageEvents;++diagnostics.damageEvents;}
    if(result.killed&&!result.duplicate){++source.kills;++diagnostics.kills;
        // Stop/restore our instance observers immediately after the original
        // death notification, before the match factory can retire that Pawn.
        if(!BeforeRetire(nearest,RetireReason::NativeDeath))return finish(RayReason::RetireFailure,Outcome::Failure);}
    diagnostics.operation="none";return finish(result.duplicate?RayReason::NativeDuplicate:
        (result.applied?RayReason::DamageApplied:RayReason::DamageNotApplied));
}

inline Outcome Process(Pending& pending,double now){
    RayReceipt receipt{};receipt.identity=pending.identity;receipt.shot=pending.shot;
    receipt.damageToken=pending.damageToken;receipt.eligibleBefore=pending.eligible;receipt.completedAt=now;
    const auto outcome=ProcessObserved(pending,now,receipt);receipt.eligibleAfter=pending.eligible;
    if(receipt.identity.source<rayDiagnostics.sources.size()){
        auto& summary=rayDiagnostics.sources[receipt.identity.source];++summary.attempts;
        if(outcome==Outcome::Deferred)++summary.deferredAttempts;
        else {++summary.terminals;++summary.reasons[static_cast<size_t>(receipt.reason)];
            if(receipt.reason==RayReason::DamageApplied)++summary.damageEvents;
            const auto p=policy::Player(receipt.identity.source);
            if(p&&p->local){
                rayDiagnostics.humanTerminals[rayDiagnostics.humanTerminalCount++%rayDiagnostics.humanTerminals.size()]=receipt;
            }
        }
    }
    return outcome;
}
inline bool Initialize(void* originalWorld){
    if(diagnostics.initialized)return world==originalWorld&&Thread();
    if(!original_players::Thread()||!originalWorld||originalWorld!=original_players::world)return Fail(2435,"server_combat_original_world_and_thread_required");
    if(!std::isfinite(options.maximumDistance)||options.maximumDistance<=0||options.maximumDistance>1000000
       ||!std::isfinite(options.maximumShotAge)||options.maximumShotAge<=0||options.maximumShotAge>1
       ||!std::isfinite(options.obstructionTolerance)||options.obstructionTolerance<0||options.obstructionTolerance>5
       ||!std::isfinite(options.sampleInterval)||options.sampleInterval<=0||options.sampleInterval>1
       ||!options.maximumShotsPerPoll||options.maximumShotsPerPoll>QueueCapacity)return Fail(2436,"server_combat_local_policy_invalid");
    if(original_shots::State().initialized)return Fail(2437,"server_combat_v2_requires_legacy_single_shot_observer_disabled");
    shotFunction=engine::Find(L"/Script/ShooterGame.FiringEffectComponent.FiringStateOnShot");
    startFunction=engine::Find(L"/Script/ShooterGame.FiringEffectComponent.HandleStartEffect");
    forceInputFunction=engine::Find(L"/Script/ShooterGame.ShooterCharacter.ForceInputState");
    componentsFunction=engine::Find(L"/Script/Engine.Actor.K2_GetComponentsByClass");
    resourceFunction=engine::Find(L"/Script/ShooterGame.FiringStateComponent.GetResourceComponent");
    stabilityFunction=engine::Find(L"/Script/ShooterGame.FiringStateComponent.GetStabilityComponent");
    geometryFunction=engine::Find(L"/Script/ShooterGame.FiringStateComponent.GetFiringEffectTypeOverride");
    if(!geometryFunction||Read<uint16_t>(geometryFunction,0xbe)!=8||Read<uint32_t>(geometryFunction,0xb8)!=0x48020c00U
       ||Read<void*>(geometryFunction,0xe0)!=engine::function<void*>(0x684f930)
       ||Read<void*>(geometryFunction,0x28)!=engine::Class(0x684d4c0))
        return Fail(4650,"server_combat_original_geometry_function_missing");
    if(!shotFunction||!startFunction||!forceInputFunction||Read<uint16_t>(shotFunction,0xbe)!=32||Read<uint16_t>(forceInputFunction,0xbe)!=2)
        return Fail(2438,"server_combat_original_event_abi_mismatch");
    if(!componentsFunction||!resourceFunction||!stabilityFunction||Read<uint16_t>(componentsFunction,0xbe)!=24
       ||Read<uint16_t>(resourceFunction,0xbe)!=8||Read<uint16_t>(stabilityFunction,0xbe)!=8)
        return Fail(2460,"server_combat_original_firing_context_abi_mismatch");
    externalForwardFunction=engine::Find(L"/Script/Engine.KismetMathLibrary.GetForwardVector");
    const auto mathClass=engine::Class(0x4526580);externalMathDefault=mathClass?engine::DefaultObject(mathClass):nullptr;
    externalFunctionsReady=externalForwardFunction&&externalMathDefault
        &&Read<uint16_t>(externalForwardFunction,0xbe)==48&&Read<uint32_t>(externalForwardFunction,0xb8)==0x14822403
        &&Read<void*>(externalForwardFunction,0xe0)==engine::function<void*>(0x454e130)
        &&Read<void*>(externalForwardFunction,0x28)==mathClass&&Identity(externalMathDefault,externalMathIdentity);
    if(!externalFunctionsReady)return Fail(4615,"server_combat_external_npc_original_forward_math_abi_mismatch");
    auto ntdllModule=GetModuleHandleW(L"ntdll.dll");
    if(ntdllModule){captureContext=reinterpret_cast<CaptureContextFn>(GetProcAddress(ntdllModule,"RtlCaptureContext"));
        lookupFunction=reinterpret_cast<LookupFunctionFn>(GetProcAddress(ntdllModule,"RtlLookupFunctionEntry"));
        virtualUnwind=reinterpret_cast<VirtualUnwindFn>(GetProcAddress(ntdllModule,"RtlVirtualUnwind"));}
    if(!captureContext||!lookupFunction||!virtualUnwind)return Fail(2461,"server_combat_current_thread_native_unwind_unavailable");
    constexpr uint8_t sampler[]{0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x48,0x89,0x70,0x20,0x57,0x48,0x81,0xec,0x60};
    constexpr uint8_t constructor[]{0x48,0x89,0x4c,0x24,0x08,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9,0xc7,0x41,0x04};
    constexpr uint8_t submit[]{0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0xf9,0x48,0x8b,0xda};
    constexpr uint8_t retainedThis[]{0x48,0x8b,0xf1};
    diagnostics.nativeEntriesVerified=std::memcmp(reinterpret_cast<void*>(engine::image+0x70a5420),sampler,sizeof(sampler))==0
        &&std::memcmp(reinterpret_cast<void*>(engine::image+0x70aa150),constructor,sizeof(constructor))==0
        &&std::memcmp(reinterpret_cast<void*>(engine::image+0x70a7110),submit,sizeof(submit))==0
        &&std::memcmp(reinterpret_cast<void*>(engine::image+0x711f032),retainedThis,sizeof(retainedThis))==0
        &&std::memcmp(reinterpret_cast<void*>(engine::image+0x712279c),retainedThis,sizeof(retainedThis))==0;
    if(!diagnostics.nativeEntriesVerified)return Fail(2439,"server_combat_original_native_transport_entry_mismatch");
    if(!original_damage::Initialize(originalWorld))return Fail(original_damage::State().error,original_damage::State().stage);
    world=originalWorld;gameThread=GetCurrentThreadId();diagnostics.initialized=true;
    diagnostics.error=0;diagnostics.stage="server_combat_waiting_original_source_bindings";return true;
}
inline bool PollImpl(double now){
    if(!Thread()||!std::isfinite(now)||now<0||now+.05<diagnostics.lastPoll)return Fail(2440,"server_combat_invalid_game_thread_or_clock");
    diagnostics.lastPoll=now;diagnostics.boundSources=diagnostics.aliveSources=diagnostics.physicsReadySources=0;
    bool missingBinding=false,missingInput=false;
    for(size_t i=0;i<sources.size();++i){auto& s=sources[i];const auto player=policy::Player(i);
        if(!s.bound){if(player&&player->registered&&player->alive&&player->ready){missingBinding=true;++diagnostics.bindingDeferred;}continue;}
        if(!Refresh(s,now))return false;
        if(!s.alive){if(!BeforeRetire(i,RetireReason::NativeDeath))return false;continue;}
        // Runtime's public weapon callback waits for Ready. Bootstrap only a
        // neutral original context here so that its first real BeforeFrame
        // application can establish readiness without an input-ready cycle.
        if(s.externalInputProducer&&s.externalCanSubmit&&!s.externalNativeAppliedReady
           &&!ExternalWeaponInput(s,false,false,false))return false;
        if(!s.inputReady)missingInput=true;
        ++diagnostics.boundSources;++diagnostics.aliveSources;if(s.physicsReady)++diagnostics.physicsReadySources;
        if(s.ballistic&&!PollEffects(s))return false;
    }
    // Handlers run outside FireShot/ProcessEvent and before any point-damage
    // transaction, so native protection removal cannot recursively fire/score.
    if(!NotifyWeaponUses())return false;
    if(!missingBinding){const auto end=writeIndex;size_t budget=options.maximumShotsPerPoll;
        while(readIndex<end&&budget--){auto& pending=queue[readIndex%queue.size()];const auto result=Process(pending,now);
            if(result==Outcome::Failure)return false;if(result==Outcome::Deferred)break;
            ++readIndex;++diagnostics.processed;}}
    diagnostics.ready=policy::Active()&&!missingBinding&&!missingInput&&diagnostics.physicsReadySources>0;
    diagnostics.error=0;diagnostics.stage=diagnostics.ready?"server_combat_original_multi_source_active":"server_combat_waiting_original_bindings_physics_or_match";
    diagnostics.operation="none";return true;
}
inline bool Poll(double now){
    if(diagnostics.processing)return Fail(2441,"server_combat_reentrant_poll_rejected");diagnostics.processing=true;bool okay=false;
    __try{okay=PollImpl(now);}__except(CaptureException(GetExceptionInformation())){okay=false;}
    diagnostics.processing=false;return okay;
}
inline bool VisibleImpl(size_t source,size_t target){
    ++diagnostics.visibilityQueries;
    if(!Thread()||source>=sources.size()||target>=sources.size()||source==target)return false;
    auto& s=sources[source];auto& t=sources[target];const double now=Now();
    if(!s.bound||!t.bound||!Refresh(s,now)||!Refresh(t,now)||!s.alive||!t.alive||!s.physicsReady||!t.physicsReady){++diagnostics.visibilityDeferred;return false;}
    engine::Vec start{},end{};
    if(!s.forwardView||!s.forwardView(s.pawn,&start)||!original_shots::WorldPoint(start)
       ||!Event(t.pawn,L"/Script/Engine.Actor.K2_GetActorLocation",24,&end)||!original_shots::WorldPoint(end))return false;
    const auto delta=engine::Add(end,engine::Mul(start,-1));const double distance=std::sqrt(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z);
    if(!std::isfinite(distance)||distance<1||distance>options.maximumDistance)return false;
    const auto direction=engine::Mul(delta,1./distance);double sceneDistance=0;
    const int hit=SceneTrace(start,direction,distance,sceneDistance);
    return hit>=0&&(!hit||sceneDistance+options.obstructionTolerance>=distance);
}
inline bool Visible(size_t source,size_t target){__try{return VisibleImpl(source,target);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool Pop(ObservedRay& ray){
    if(!Thread()||observationRead==observationWrite)return false;
    ray=observations[observationRead++%observations.size()];return true;
}
} // namespace firearms

namespace melee {
inline constexpr size_t MaxPlayers=32,MaxStates=16,MaxVtableFunctions=512,QueueCapacity=256;
static_assert(policy::MaxPlayers==MaxPlayers&&original_damage::MaxTargets>=MaxPlayers);
inline constexpr uintptr_t AttackClass=0x699b9e0,OriginalAttackTick=0x7130ce0,OriginalEvent=0x1cdc850;
inline constexpr size_t AttackTickSlot=0x558/8,EventSlot=0x298/8;
inline constexpr const wchar_t* MeleeClassPath=L"/Game/Equippables/Melee/Ability_Melee_Base.Ability_Melee_Base_C";
inline constexpr const wchar_t* ImpactPath=L"/Script/ShooterGame.MeleeAttackStateComponent.MulticastHitImpact";
using Weak=original_inventory_slots::WeakIdentity;
// ScriptState's actual7143730 dispatcher passes XMM1 and XMM2, ignores the
// return, then broadcasts OnStateTick. Preserve both original float inputs.
using NativeAttackTick=void(*)(void*,float,float);
using NativeEvent=void(*)(void*,void*,void*);
struct Hooks {
    // Must share the firearm bridge's globally monotonic token allocator.
    uint64_t (*allocateDamageToken)()=nullptr;
    // Retire other observers on the victim after a verified native death.
    // This module separately restores its own melee component observers.
    bool (*beforeNativeDeath)(size_t index)=nullptr;
    // Optional original weapon-use policy (including a native swing/miss).
    // Called only from Poll, before any queued contact damage is processed.
    void (*weaponUsed)(size_t index)=nullptr;
};
struct Options {double maximumContactAge=1.0;size_t maximumContactsPerPoll=32;};
struct Vtable {
    void** previous=nullptr;size_t count=0;bool installed=false;
    std::array<void*,MaxVtableFunctions+1> entries{};
    void** Address(){return entries.data()+1;}
};
struct AttackState {
    void* object=nullptr;Weak identity{};Vtable table;
    NativeAttackTick forwardAttackTick=nullptr;NativeEvent forwardEvent=nullptr;
    uint64_t entries=0,contacts=0;
};
struct Source {
    bool bound=false;size_t index=0,stateCount=0;uint32_t spawnEpoch=0;uint64_t binding=0;
    void *pawn=nullptr,*weapon=nullptr;Weak pawnIdentity{},weaponIdentity{};
    std::array<AttackState,MaxStates> states{};
    uint64_t entries=0,contacts=0,damageEvents=0,kills=0;
};
struct LifeSnapshot {
    bool valid=false,alive=false;uint32_t epoch=0;void* pawn=nullptr;
    void *handler=nullptr,*health=nullptr,*shield=nullptr;
    float healthValue=0,shieldValue=0;int32_t eventIndex=0,pendingEvents=0;
};
struct Contact {
    bool present=false,nativeLifeUnchanged=false,backstab=false;
    uint32_t source=0,victim=0,sourceEpoch=0,victimEpoch=0;
    uint64_t binding=0,occurrence=0,token=0;double at=0;
    void *sourcePawn=nullptr,*victimPawn=nullptr,*weapon=nullptr,*state=nullptr,*damageType=nullptr;
    Weak sourceIdentity{},victimIdentity{},weaponIdentity{},stateIdentity{},damageTypeIdentity{};
    float attackDamage=0;
    original_shots::Shot nativeContact;
    original_damage::Hit nativeHit;
};
struct PendingWeaponUse {
    bool pending=false;uint32_t spawnEpoch=0;void* pawn=nullptr;Weak identity{};
};
struct AttackScope {
    Source* source=nullptr;AttackState* state=nullptr;uint64_t occurrence=0;
    std::array<LifeSnapshot,MaxPlayers> before{};
    uint32_t impactEvents=0;Contact contact;
};
struct Diagnostics {
    bool initialized=false,ready=false,restartRequired=false,processing=false;
    unsigned error=0,boundSources=0;
    uint64_t binds=0,retirements=0,restoredTables=0,foreignTables=0;
    uint64_t entries=0,impactCallbacks=0,exactAuthorityContacts=0,worldContacts=0;
    uint64_t invalidPayloads=0,ambiguousContacts=0,nonCoreContacts=0,missingVictimBindings=0;
    uint64_t nativeLifeChanges=0,queued=0,queueOverflow=0,processed=0,staleContacts=0;
    uint64_t policyRejected=0,pendingDamageDeferred=0,damageEvents=0,kills=0;
    uint64_t weaponUseTicks=0,weaponUseNotifications=0,staleWeaponUses=0;
    uint64_t lastToken=0,lastOccurrence=0;
    int32_t lastSource=-1,lastVictim=-1;float lastDamage=0;
    bool lastBackstab=false;uint8_t lastRegion=255;
    void *lastNativeHitActor=nullptr,*lastNativeHitComponent=nullptr,*lastAttackState=nullptr;
    const char* lastNativeComponentKind="none";
    uintptr_t exceptionRva=0;double lastPoll=-1;
    const char* stage="server_melee_not_initialized";
    const char* operation="none";
};
inline Hooks hooks;inline Options options;inline Diagnostics diagnostics;
inline DWORD gameThread=0;inline uint64_t observerDepth=0;inline void *world=nullptr,*impactFunction=nullptr;
inline std::array<Source,MaxPlayers> sources{};
inline std::array<Contact,QueueCapacity> queue{};
inline std::array<PendingWeaponUse,MaxPlayers> pendingWeaponUses{};
inline uint64_t nextBinding=0,nextOccurrence=0,readIndex=0,writeIndex=0;
inline AttackScope* activeScope=nullptr;
inline const Diagnostics& State(){return diagnostics;}
inline const Source* Player(size_t index){return index<sources.size()?&sources[index]:nullptr;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Fail(unsigned code,const char* stage,bool terminal=false){
    diagnostics.error=code;diagnostics.stage=stage;diagnostics.ready=false;
    if(terminal)diagnostics.restartRequired=true;return false;
}
inline bool Thread(){return diagnostics.initialized&&!diagnostics.restartRequired&&GetCurrentThreadId()==gameThread;}
inline double Now(){return static_cast<double>(GetTickCount64())/1000.;}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Identity(void* object,Weak& identity){return original_inventory_slots::CaptureIdentity(object,identity)&&Live(object,identity);}
inline LONG Capture(EXCEPTION_POINTERS* e){
    const auto at=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    diagnostics.exceptionRva=at>=engine::image?at-engine::image:at;
    policy::CancelNativeDamage();
    Fail(e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0003100U,"server_melee_native_exception_requires_new_session",true);
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool IsMeleeWeapon(void* weapon){
    auto type=engine::Find(MeleeClassPath);
    return weapon&&type&&original_players::Derived(Read<void*>(weapon,0x10),type);
}
inline bool SourceIdentity(const Source& source){
    const auto p=policy::Player(source.index);
    return policy::Authority(source.index,source.pawn,p?p->controller:nullptr)&&source.bound&&p&&p->registered&&p->pawn==source.pawn&&p->spawnEpoch==source.spawnEpoch
        &&Live(source.pawn,source.pawnIdentity)&&Live(source.weapon,source.weaponIdentity)
        &&Read<void*>(source.weapon,0x4a8)==source.pawn&&Read<uint8_t>(source.weapon,0x224)==3;
}
inline AttackState* FindState(void* object,Source** owner=nullptr){
    for(auto& source:sources)if(source.bound)for(size_t i=0;i<source.stateCount;++i)
        if(source.states[i].object==object){if(owner)*owner=&source;return &source.states[i];}
    return nullptr;
}
// Copy RTTI plus the complete original virtual table, stopping only at a real
// non-executable delimiter. The original actor/Pawn tables are not involved.
inline bool CopyVtable(void* object,Vtable& table){
    auto original=Read<void**>(object,0);if(!original)return false;
    MEMORY_BASIC_INFORMATION region{};uintptr_t begin=0,end=0;size_t count=0;
    for(;count<MaxVtableFunctions;++count){
        void* entry=nullptr;if(!original_players::Read(original,count*8,entry))return false;
        const auto address=reinterpret_cast<uintptr_t>(entry);if(!address)break;
        if(address<begin||address>=end){
            if(!VirtualQuery(entry,&region,sizeof(region))||region.State!=MEM_COMMIT||(region.Protect&(PAGE_GUARD|PAGE_NOACCESS)))break;
            const auto protection=region.Protect&0xffU;
            if(protection!=PAGE_EXECUTE&&protection!=PAGE_EXECUTE_READ&&protection!=PAGE_EXECUTE_READWRITE&&protection!=PAGE_EXECUTE_WRITECOPY)break;
            begin=reinterpret_cast<uintptr_t>(region.BaseAddress);end=begin+region.RegionSize;
        }
    }
    if(count<=0x598/8||count==MaxVtableFunctions||!original_players::Readable(original-1,(count+1)*8))return false;
    table.previous=original;table.count=count;std::memcpy(table.entries.data(),original-1,(count+1)*8);return true;
}
inline bool Restore(AttackState& attackState){
    if(!attackState.table.installed)return true;
    if(!Live(attackState.object,attackState.identity)){attackState.table.installed=false;return true;}
    auto current=Read<void**>(attackState.object,0);
    if(current==attackState.table.previous){attackState.table.installed=false;return true;}
    if(current!=attackState.table.Address()){++diagnostics.foreignTables;return Fail(3101,"server_melee_other_component_observer_must_retire_first");}
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(attackState.object),attackState.table.previous,attackState.table.Address())!=attackState.table.Address())
        return Fail(3102,"server_melee_component_restore_raced");
    attackState.table.installed=false;++diagnostics.restoredTables;return true;
}
inline bool BeforeRetireImpl(size_t index){
    if(!Thread()||index>=sources.size())return false;auto& source=sources[index];if(!source.bound)return true;
    if(activeScope&&activeScope->source==&source)return Fail(3103,"server_melee_cannot_retire_inside_native_attack");
    for(size_t i=0;i<source.stateCount;++i)if(!Restore(source.states[i]))return false;
    source.bound=false;++diagnostics.retirements;if(diagnostics.boundSources)--diagnostics.boundSources;return true;
}
inline bool BeforeRetire(size_t index){__try{return BeforeRetireImpl(index);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool ReadLife(size_t index,LifeSnapshot& snapshot){
    snapshot={};const auto p=policy::Player(index);const auto& target=original_damage::targets[index];
    if(!p||!p->registered||!p->ready||p->phase!=policy::Phase::Alive||!p->alive
       ||!target.bound||target.pawn!=p->pawn||!target.healthSection)return false;
    snapshot.pawn=p->pawn;snapshot.epoch=p->spawnEpoch;snapshot.handler=target.damageable;
    snapshot.health=target.healthSection;snapshot.shield=target.shieldSection;
    snapshot.healthValue=Read<float>(snapshot.health,0x124);snapshot.shieldValue=snapshot.shield?Read<float>(snapshot.shield,0x124):0;
    snapshot.eventIndex=Read<int32_t>(snapshot.handler,0x1e0);snapshot.pendingEvents=Read<int32_t>(snapshot.handler,0x248);
    snapshot.alive=Read<uint8_t>(snapshot.handler,0x1f9)!=0;
    snapshot.valid=std::isfinite(snapshot.healthValue)&&snapshot.healthValue>0&&std::isfinite(snapshot.shieldValue)
        &&snapshot.shieldValue>=0&&snapshot.alive;return snapshot.valid;
}
inline bool SameLife(const LifeSnapshot& before){
    return before.valid&&Read<void*>(before.pawn,0xc70)==before.handler
        &&Read<float>(before.health,0x124)==before.healthValue
        &&(!before.shield||Read<float>(before.shield,0x124)==before.shieldValue)
        &&Read<int32_t>(before.handler,0x1e0)==before.eventIndex
        &&Read<int32_t>(before.handler,0x248)==before.pendingEvents
        &&(Read<uint8_t>(before.handler,0x1f9)!=0)==before.alive;
}
inline bool CaptureContact(AttackScope& scope,void* parameters){
    auto& source=*scope.source;auto& attackState=*scope.state;
    if(!parameters||!original_players::Readable(parameters,0xf1)||!SourceIdentity(source)
       ||!Live(attackState.object,attackState.identity)||Read<void*>(attackState.object,0xc8)!=source.weapon){++diagnostics.invalidPayloads;return false;}
    const auto native=static_cast<const uint8_t*>(parameters);
    auto victim=engine::function<void*(*)(const void*)>(0x437b290)(native+0xb8);
    diagnostics.lastNativeHitActor=victim;diagnostics.lastAttackState=attackState.object;
    size_t victimIndex=sources.size();
    for(size_t i=0;i<MaxPlayers;++i){const auto p=policy::Player(i);if(p&&p->registered&&p->pawn==victim){victimIndex=i;break;}}
    if(victimIndex==sources.size()){++diagnostics.worldContacts;return false;}
    const auto& before=scope.before[victimIndex];const auto target=policy::Player(victimIndex);
    if(!before.valid||before.pawn!=victim||!target||target->spawnEpoch!=before.epoch){++diagnostics.missingVictimBindings;return false;}
    Weak componentIdentity{};std::memcpy(&componentIdentity,native+0xd0,sizeof(componentIdentity));
    auto component=original_inventory_slots::ResolveIdentity(componentIdentity);
    diagnostics.lastNativeHitComponent=component;
    diagnostics.lastNativeComponentKind=component==original_damage::targets[victimIndex].core?"original_core_mesh":
        component==Read<void*>(victim,0x500)?"original_capsule":"other_original_component";
    if(!component||component!=original_damage::targets[victimIndex].core){++diagnostics.nonCoreContacts;return false;}
    Contact contact;contact.present=true;contact.source=static_cast<uint32_t>(source.index);contact.victim=static_cast<uint32_t>(victimIndex);
    contact.sourceEpoch=source.spawnEpoch;contact.victimEpoch=before.epoch;contact.binding=source.binding;contact.occurrence=scope.occurrence;
    contact.at=Now();contact.backstab=native[0xf0]!=0;contact.sourcePawn=source.pawn;contact.victimPawn=victim;
    contact.weapon=source.weapon;contact.state=attackState.object;contact.sourceIdentity=source.pawnIdentity;
    contact.weaponIdentity=source.weaponIdentity;contact.stateIdentity=attackState.identity;
    const auto tuning=static_cast<const uint8_t*>(attackState.object)+(contact.backstab?0x328:0x310);
    contact.attackDamage=Read<float>(tuning,0);contact.damageType=Read<void*>(tuning,8);
    if(!std::isfinite(contact.attackDamage)||contact.attackDamage<=0||contact.attackDamage>1000000
       ||!original_players::Derived(contact.damageType,engine::Class(original_damage::DamageTypeClass))
       ||!Identity(victim,contact.victimIdentity)||!Identity(contact.damageType,contact.damageTypeIdentity)){
        ++diagnostics.invalidPayloads;return false;}
    auto& hit=contact.nativeHit;hit.hit=true;hit.blocking=(native[0xad]&1U)!=0;hit.component=component;
    hit.distance=Read<float>(native,8);hit.location=Read<engine::Vec>(native,0x28);hit.normal=Read<engine::Vec>(native,0x58);
    std::memcpy(hit.bone.bytes,native+0xd8,sizeof(hit.bone));std::memcpy(hit.nativeHit,native,sizeof(hit.nativeHit));
    // The real FPointDamageEvent at7130e02..e4d sets ShotDirection to the
    // negative of this exact original ImpactNormal. Do not substitute aim.
    contact.nativeContact.observed_at=contact.at;contact.nativeContact.start=Read<engine::Vec>(native,0x70);
    contact.nativeContact.direction={-hit.normal.x,-hit.normal.y,-hit.normal.z};
    contact.nativeContact.sequence=scope.occurrence;
    if(!std::isfinite(hit.distance)||hit.distance<0||!original_players::Finite(hit.location)
       ||!original_players::Finite(hit.normal)||!original_players::Finite(contact.nativeContact.start)){
        ++diagnostics.invalidPayloads;return false;}
    // Classification reuses the original native bone/region query. Melee's
    // native attack does not apply a firearm-style head/leg multiplier.
    alignas(8) uint8_t regional[0xf8]{};std::memcpy(regional,native,0xf0);
    if(!original_damage::Event(victim,L"/Script/ShooterGame.ShooterCharacter.GetRegionalDamage",0xf2,regional)||regional[0xf1]>2){
        ++diagnostics.invalidPayloads;return false;}
    hit.region=regional[0xf1];scope.contact=contact;++attackState.contacts;++source.contacts;++diagnostics.exactAuthorityContacts;return true;
}
__declspec(noinline) inline void ObserveEvent(void* object,void* function,void* parameters){
    const bool trackedObserver=GetCurrentThreadId()==gameThread;
    if(trackedObserver)++observerDepth;
    __try{
    auto attackState=GetCurrentThreadId()==gameThread?FindState(object):nullptr;
    const auto forward=attackState&&attackState->forwardEvent?attackState->forwardEvent:engine::function<NativeEvent>(OriginalEvent);
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    auto scope=activeScope;
    if(Thread()&&attackState&&scope&&scope->state==attackState&&function==impactFunction
       &&caller==engine::image+0x7130fa6){
        ++diagnostics.impactCallbacks;++scope->impactEvents;
        if(scope->impactEvents==1)CaptureContact(*scope,parameters);
        else{scope->contact.present=false;++diagnostics.ambiguousContacts;}
    }
    forward(object,function,parameters);

    }__finally{if(trackedObserver)--observerDepth;}
}
__declspec(noinline) inline void ObserveAttackTick(void* object,float nativeTime0,float nativeTime1){
    const bool trackedObserver=GetCurrentThreadId()==gameThread;
    if(trackedObserver)++observerDepth;
    __try{
    Source* source=nullptr;auto attackState=GetCurrentThreadId()==gameThread?FindState(object,&source):nullptr;
    const auto forward=attackState&&attackState->forwardAttackTick?attackState->forwardAttackTick:engine::function<NativeAttackTick>(OriginalAttackTick);
    if(!Thread()||!attackState||!source||!SourceIdentity(*source)){forward(object,nativeTime0,nativeTime1);return;}
    AttackScope scope;scope.source=source;scope.state=attackState;scope.occurrence=++nextOccurrence;
    ++attackState->entries;++source->entries;++diagnostics.entries;
    for(size_t i=0;i<MaxPlayers;++i)ReadLife(i,scope.before[i]);
    const auto previous=activeScope;activeScope=&scope;
    // Preserve nesting and exception propagation of the original attack.
    __try{forward(object,nativeTime0,nativeTime1);}__finally{activeScope=previous;}
    if(hooks.weaponUsed){
        // The shipped attack tick really completed, including its miss/wall
        // branches. Coalesce by original Pawn/epoch, not by a requested input
        // or successful hit. Keep this independent of weapon retirement so a
        // same-life switch cannot erase an actual use before the next Poll.
        pendingWeaponUses[source->index]={true,source->spawnEpoch,source->pawn,source->pawnIdentity};
        ++diagnostics.weaponUseTicks;
    }
    if(!scope.contact.present||scope.impactEvents!=1)return;
    auto& contact=scope.contact;
    contact.nativeLifeUnchanged=SameLife(scope.before[contact.victim]);
    if(!contact.nativeLifeUnchanged){++diagnostics.nativeLifeChanges;Fail(3104,"server_melee_native_life_already_changed_no_second_damage",true);return;}
    contact.token=hooks.allocateDamageToken?hooks.allocateDamageToken():0;
    if(!contact.token){Fail(3105,"server_melee_shared_damage_token_unavailable",true);return;}
    if(writeIndex-readIndex>=queue.size()){++diagnostics.queueOverflow;Fail(3106,"server_melee_contact_queue_full",true);return;}
    queue[writeIndex++%queue.size()]=contact;++diagnostics.queued;

    }__finally{if(trackedObserver)--observerDepth;}
}
inline bool InitializeImpl(void* originalWorld,const Hooks& callbacks,const Options& settings){
    if(diagnostics.initialized)return Thread()&&world==originalWorld;
    if(!original_players::Thread()||!originalWorld||!policy::State().initialized
       ||policy::world!=originalWorld||!callbacks.allocateDamageToken||!callbacks.beforeNativeDeath
       ||!std::isfinite(settings.maximumContactAge)||settings.maximumContactAge<=0||settings.maximumContactAge>10
       ||!settings.maximumContactsPerPoll||settings.maximumContactsPerPoll>QueueCapacity)
        return Fail(3107,"server_melee_original_match_and_shared_hooks_required");
    auto type=engine::Class(AttackClass);impactFunction=engine::Find(ImpactPath);
    if(!type||!impactFunction||Read<uint16_t>(impactFunction,0xbe)!=0xf1||Read<void*>(impactFunction,0x28)!=type)
        return Fail(3108,"server_melee_original_impact_abi_missing");
    if(!original_damage::Initialize(originalWorld))return Fail(original_damage::State().error,original_damage::State().stage);
    hooks=callbacks;options=settings;world=originalWorld;gameThread=GetCurrentThreadId();
    diagnostics.initialized=true;diagnostics.ready=true;diagnostics.error=0;
    diagnostics.stage="server_melee_waiting_for_real_equipped_state_instances";return true;
}
inline bool Initialize(void* originalWorld,const Hooks& callbacks,const Options& settings=Options{}){
    __try{return InitializeImpl(originalWorld,callbacks,settings);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool BindImpl(size_t index,void* pawn,void* weapon,uint32_t epoch){
    if(!Thread()||index>=sources.size()||!epoch)return Fail(3109,"server_melee_bind_context_invalid");
    auto& source=sources[index];
    if(source.bound){if(source.pawn==pawn&&source.weapon==weapon&&source.spawnEpoch==epoch)return SourceIdentity(source);
        return Fail(3110,"server_melee_before_retire_required_for_new_binding");}
    if(!IsMeleeWeapon(weapon))return true;
    const auto player=policy::Player(index);
    if(!player||!player->registered||player->pawn!=pawn||player->spawnEpoch!=epoch
       ||!original_players::IsA(pawn,original_players::ShooterCharacterClass)||Read<void*>(weapon,0x4a8)!=pawn
       ||Read<uint8_t>(weapon,0x224)!=3)return Fail(3111,"server_melee_requires_real_authority_owned_equippable");
    source={};source.index=index;source.pawn=pawn;source.weapon=weapon;source.spawnEpoch=epoch;source.binding=++nextBinding;
    if(!Identity(pawn,source.pawnIdentity)||!Identity(weapon,source.weaponIdentity))return Fail(3112,"server_melee_original_source_identity_failed");
    struct Query {void* type=nullptr;void** data=nullptr;int32_t count=0,capacity=0;} query{engine::Class(AttackClass)};
    static_assert(sizeof(Query)==24);
    const bool queried=original_damage::Event(weapon,L"/Script/Engine.Actor.K2_GetComponentsByClass",24,&query);
    bool valid=queried&&query.count>0&&query.count<=static_cast<int32_t>(MaxStates)&&query.capacity>=query.count
        &&original_players::Readable(query.data,static_cast<size_t>(query.count)*8);
    if(valid){
        source.stateCount=static_cast<size_t>(query.count);
        for(size_t i=0;i<source.stateCount;++i){
            auto& attackState=source.states[i];attackState.object=query.data[i];
            if(!original_players::IsA(attackState.object,AttackClass)||Read<void*>(attackState.object,0xc8)!=weapon
               ||!Identity(attackState.object,attackState.identity)||!CopyVtable(attackState.object,attackState.table)){valid=false;break;}
            attackState.forwardAttackTick=reinterpret_cast<NativeAttackTick>(attackState.table.previous[AttackTickSlot]);
            attackState.forwardEvent=reinterpret_cast<NativeEvent>(attackState.table.previous[EventSlot]);
            if(attackState.forwardAttackTick!=engine::function<NativeAttackTick>(OriginalAttackTick)
               ||attackState.forwardEvent!=engine::function<NativeEvent>(OriginalEvent)
               ||attackState.table.previous[0x598/8]!=engine::function<void*>(0x7132200)){valid=false;break;}
        }
    }
    if(query.data)engine::function<void(*)(void*)>(0x1915720)(query.data);
    if(!valid)return Fail(3113,"server_melee_exact_native_attack_components_not_ready");
    if(!original_damage::Bind(index,pawn))return Fail(original_damage::State().error,original_damage::State().stage);
    source.bound=true;
    for(size_t i=0;i<source.stateCount;++i){auto& attackState=source.states[i];
        attackState.table.entries[1+AttackTickSlot]=reinterpret_cast<void*>(ObserveAttackTick);
        attackState.table.entries[1+EventSlot]=reinterpret_cast<void*>(ObserveEvent);
        if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(attackState.object),attackState.table.Address(),attackState.table.previous)!=attackState.table.previous){
            for(size_t n=0;n<i;++n)Restore(source.states[n]);source.bound=false;
            return Fail(3114,"server_melee_component_observer_install_raced",true);}
        attackState.table.installed=true;
    }
    ++diagnostics.binds;++diagnostics.boundSources;diagnostics.ready=true;diagnostics.error=0;
    diagnostics.stage="server_melee_real_attack_observers_bound";return true;
}
inline bool Bind(size_t index,void* pawn,void* weapon,uint32_t epoch){
    __try{return BindImpl(index,pawn,weapon,epoch);}__except(Capture(GetExceptionInformation())){return false;}
}
enum class Outcome:uint8_t{Done,Deferred,Failure};
inline Outcome ApplyContact(Contact& contact,double now){
    if(!contact.present||!contact.nativeLifeUnchanged||!contact.token)return Outcome::Done;
    if(contact.source>=sources.size()||contact.victim>=sources.size())return Outcome::Done;
    auto& source=sources[contact.source];const auto victim=policy::Player(contact.victim);
    if(!source.bound||source.binding!=contact.binding||source.spawnEpoch!=contact.sourceEpoch
       ||!SourceIdentity(source)||!victim||!victim->registered||victim->spawnEpoch!=contact.victimEpoch||victim->pawn!=contact.victimPawn
       ||!Live(contact.victimPawn,contact.victimIdentity)||!Live(contact.state,contact.stateIdentity)
       ||!Live(contact.damageType,contact.damageTypeIdentity)||now-contact.at>options.maximumContactAge||now<contact.at){
        ++diagnostics.staleContacts;return Outcome::Done;}
    if(!policy::CanDamage(contact.source,contact.victim)){++diagnostics.policyRejected;return Outcome::Done;}
    if(!original_damage::Sample(contact.victim)){
        Fail(original_damage::State().error,original_damage::State().stage);return Outcome::Failure;}
    if(original_damage::targets[contact.victim].pendingDamageEvents){++diagnostics.pendingDamageDeferred;return Outcome::Deferred;}
    firearms::NativeLife beforePoint{};
    if(!firearms::ReadNativeLife(contact.victim,beforePoint)){
        ++diagnostics.staleContacts;return Outcome::Done;
    }
    original_damage::DeathData death;
    if(!policy::BeginNativeDamage(contact.token,contact.source,contact.victim,contact.weapon,now,death)){
        if(!policy::CanDamage(contact.source,contact.victim)){++diagnostics.policyRejected;return Outcome::Done;}
        Fail(policy::State().error,policy::State().stage);return Outcome::Failure;}
    original_damage::Result result;diagnostics.operation="server_melee_contact_to_native_point_notification";
    const bool applied=ApplyAuthorityPoint(contact.victim,contact.sourcePawn,contact.weapon,contact.damageType,
        contact.nativeContact,contact.nativeHit,contact.attackDamage,contact.token,result,&death,1.0f);
    const bool committed=policy::EndNativeDamage(applied,result);
    if(!applied||!committed){Fail(!applied?original_damage::State().error:policy::State().error,
        !applied?original_damage::State().stage:policy::State().stage);return Outcome::Failure;}
    if(!result.duplicate&&!firearms::RecordNativeLifeReceipt(contact.victim,beforePoint)){
        Fail(4664,"server_melee_native_point_life_receipt_unavailable",true);return Outcome::Failure;
    }
    diagnostics.lastSource=static_cast<int32_t>(contact.source);diagnostics.lastVictim=static_cast<int32_t>(contact.victim);
    diagnostics.lastToken=contact.token;diagnostics.lastOccurrence=contact.occurrence;diagnostics.lastDamage=contact.attackDamage;
    diagnostics.lastBackstab=contact.backstab;diagnostics.lastRegion=contact.nativeHit.region;
    if(result.applied&&!result.duplicate){++source.damageEvents;++diagnostics.damageEvents;}
    if(result.killed&&!result.duplicate){++source.kills;++diagnostics.kills;
        if(!BeforeRetire(contact.victim)||!hooks.beforeNativeDeath(contact.victim))return Outcome::Failure;}
    diagnostics.operation="none";return Outcome::Done;
}
inline void DispatchWeaponUses(){
    for(size_t index=0;index<pendingWeaponUses.size();++index){auto& pending=pendingWeaponUses[index];
        if(!pending.pending)continue;
        const auto record=pending;pending={};
        if(!hooks.weaponUsed)continue;
        const auto player=policy::Player(index);
        if(!player||!player->registered||player->pawn!=record.pawn||player->spawnEpoch!=record.spawnEpoch
           ||!Live(record.pawn,record.identity)){++diagnostics.staleWeaponUses;continue;}
        // Clear before the external call; a newly observed attack can queue
        // another use without recursion or being erased after the callback.
        hooks.weaponUsed(index);++diagnostics.weaponUseNotifications;
    }
}
inline bool PollImpl(double now){
    if(!Thread()||!std::isfinite(now)||now<0||diagnostics.processing)return false;
    diagnostics.processing=true;diagnostics.lastPoll=now;
    DispatchWeaponUses();
    size_t processed=0;
    while(readIndex<writeIndex&&processed<options.maximumContactsPerPoll){
        auto& contact=queue[readIndex%queue.size()];const auto result=ApplyContact(contact,now);
        if(result==Outcome::Failure){diagnostics.processing=false;return false;}
        if(result==Outcome::Deferred)break;
        contact={};++readIndex;++processed;++diagnostics.processed;
    }
    diagnostics.processing=false;diagnostics.ready=true;diagnostics.error=0;
    diagnostics.stage="server_melee_observing_native_contacts";return true;
}
inline bool Poll(double now){__try{return PollImpl(now);}__except(Capture(GetExceptionInformation())){diagnostics.processing=false;return false;}}
inline bool Shutdown(){
    if(!Thread()||activeScope)return false;for(size_t i=0;i<sources.size();++i)if(!BeforeRetire(i))return false;
    for(auto& pending:pendingWeaponUses)pending={};
    readIndex=writeIndex;diagnostics.ready=false;diagnostics.stage="server_melee_observers_restored";return true;
}
} // namespace melee

inline uint64_t AllocateDamageToken(){
    if(!Thread()||firearms::nextDamageToken==UINT64_MAX){Fail(4560,"server_combat_damage_token_exhausted",true);return 0;}
    return ++firearms::nextDamageToken;
}
inline LONG Capture(EXCEPTION_POINTERS* e){
    const auto code=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0004500U;
    const auto at=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;
    policy::CancelNativeDamage();Fail(code,"server_combat_native_exception_requires_new_session",true);return EXCEPTION_EXECUTE_HANDLER;
}
inline bool SubsystemResult(bool okay,unsigned error,const char* stage,bool fatal=false){return okay?true:Fail(error,stage,fatal);}
inline bool CurrentRegistration(size_t index){
    if(index>=registrations.size())return false;const auto& r=registrations[index];const auto p=local_server_match::Player(index);
    return r.present&&p&&p->epoch==r.epoch&&p->ps==r.ps&&p->pawn==r.pawn&&p->pc==r.pc
        &&Live(r.pawn,r.pawnId)&&Live(r.pc,r.pcId)&&Live(r.ps,r.psId)&&policy::Authority(index,r.pawn,r.pc);
}
inline bool DiagnoseReferenceImpl(size_t index,void* connection,ReferenceDiagnosis& out){
    out={};out.sampled=true;out.sampledAt=GetTickCount64();out.connection=connection;
    if(!Thread()||!CurrentRegistration(index)){out.stage="native_rpc_reference_authority_identity_unavailable";return false;}
    const auto& r=registrations[index];out.controller=r.pc;out.controllerId=r.pcId;out.identityValid=true;
    out.role=Read<uint8_t>(r.pc,0x224);out.remoteRole=Read<uint8_t>(r.pc,0x128);
    // Exact original Actor generated bool setters retained in Actor-properties:
    // bReplicates=11a&10; bOnlyRelevantToOwner=64&4. Observation only.
    out.replicates=(Read<uint8_t>(r.pc,0x11a)&0x10)!=0;
    out.onlyRelevantToOwner=(Read<uint8_t>(r.pc,0x64)&4)!=0;
    auto table=Read<void**>(r.pc,0);
    using Predicate=bool(*)(void*);
    const auto stable=Read<Predicate>(table,0x240),supported=Read<Predicate>(table,0x248);
    if(stable!=engine::function<Predicate>(0x1c5dc90)||supported!=engine::function<Predicate>(0x18a3ec0)){
        out.stage="native_rpc_reference_controller_virtuals_unrecognized";return false;
    }
    out.virtualsKnown=true;out.stableName=stable(r.pc);out.supportsNetworking=supported(r.pc);
    const auto driver=Read<void*>(state.world,0x40);
    if(!connection||Read<void*>(connection,0x60)!=driver||Read<void*>(driver,0xc0)){
        out.stage="native_rpc_reference_requires_actual_host_connection";return false;
    }
    const auto peers=Read<void**>(driver,0xc8);const int32_t peerCount=Read<int32_t>(driver,0xd0);bool ownedPeer=false;
    if(peerCount<1||peerCount>8||!original_players::Readable(peers,static_cast<size_t>(peerCount)*8))return false;
    for(int32_t n=0;n<peerCount;++n)ownedPeer|=Read<void*>(peers,static_cast<size_t>(n)*8)==connection;
    if(!ownedPeer){out.stage="native_rpc_reference_connection_not_in_host_driver";return false;}
    out.packageMap=Read<void*>(connection,0x70);
    if(!original_players::IsA(out.packageMap,0x3cf26c0)
       ||Read<void*>(Read<void**>(out.packageMap,0),0x2f0)!=engine::function<void*>(0x4777940)){
        out.stage="native_rpc_reference_package_map_layout_unrecognized";return false;
    }
    out.guidCache=Read<void*>(out.packageMap,0x318);
    if(!original_players::Readable(out.guidCache,0x158)){out.stage="native_rpc_reference_guid_cache_unavailable";return false;}
    out.packageMapKnown=true;
    // The same existing weak identity/hash and 24-byte Weak->GUID map used by
    // GetOrAssign4782920 at478297f..2a07. Only the original FindIndex is called;
    // this path NEVER calls GetOrAssign, registration, export or GUID allocation.
    const uint32_t hash=static_cast<uint32_t>(r.pcId.index)^static_cast<uint32_t>(r.pcId.serial);
    const auto key=r.pcId;
    out.mapIndex=engine::function<int32_t(*)(void*,uint32_t,const Weak*)>(0x1c01cf0)
        (static_cast<uint8_t*>(out.guidCache)+0x50,hash,&key);
    if(out.mapIndex>=0){
        const auto data=Read<uint8_t*>(out.guidCache,0x50);const auto count=Read<int32_t>(out.guidCache,0x58);
        if(!data||count<1||count>1048576||out.mapIndex>=count||!original_players::Readable(data+static_cast<size_t>(out.mapIndex)*24,24)){
            out.stage="native_rpc_reference_existing_guid_map_bounds_invalid";return false;
        }
        const auto pair=data+static_cast<size_t>(out.mapIndex)*24;
        if(!original_inventory_slots::SameIdentity(Read<Weak>(pair,0),key)){
            out.stage="native_rpc_reference_existing_guid_key_mismatch";return false;
        }
        out.existingGuid=Read<uint64_t>(pair,8);out.guidFound=out.existingGuid!=0;
    }
    out.existingGuidKnown=true;
    // Passing the existing weak key avoids SupportsObject's optional serial-
    // lookup branch. The original routine only examines the cache/predicates.
    out.nativeSupportsObject=engine::function<bool(*)(void*,void*,const Weak*)>(0x4782750)(out.guidCache,r.pc,&key);
    out.guidWouldBeZero=!out.guidFound&&!out.nativeSupportsObject;
    out.stage=out.guidWouldBeZero?"native_rpc_reference_original_unsupported_guid_zero"
        :out.guidFound?"native_rpc_reference_existing_guid_observed":"native_rpc_reference_supported_guid_not_yet_assigned";
    return true;
}
inline bool DiagnoseReference(size_t index,void* connection,ReferenceDiagnosis& out){
    bool okay=false;
    __try{okay=DiagnoseReferenceImpl(index,connection,out);}__except(Capture(GetExceptionInformation())){okay=false;}
    if(index<registrations.size())registrations[index].referenceDiagnosis=out;
    ++state.referenceDiagnosticSamples;return okay;
}
inline bool BeforeNativeDeath(size_t index){
    return melee::BeforeRetire(index)&&firearms::BeforeRetire(index,firearms::RetireReason::NativeDeath);
}
inline bool BeforeRetireImpl(size_t index,void* pawn,uint32_t epoch){
    if(!Thread()||index>=registrations.size())return false;auto& r=registrations[index];
    if(!r.present)return true;
    if((pawn&&r.pawn!=pawn)||(epoch&&r.epoch!=epoch))return Fail(4561,"server_combat_retirement_identity_or_epoch_mismatch");
    if(!melee::BeforeRetire(index)||!firearms::BeforeRetire(index,firearms::RetireReason::PawnRetire))return false;
    r.present=false;++state.retirements;if(state.registeredPlayers)--state.registeredPlayers;state.ready=false;return true;
}
inline bool BeforeRetire(size_t index,void* pawn=nullptr,uint32_t epoch=0){
    __try{const bool okay=BeforeRetireImpl(index,pawn,epoch);CacheDiagnostics();return okay;}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool RegisterPlayerImpl(size_t index,void* pc,void* ps,void* pawn,uint32_t epoch){
    if(!Thread()||index>=registrations.size()||!epoch||!policy::Authority(index,pawn,pc))return Fail(4562,"server_combat_registration_requires_real_host_player");
    const auto p=local_server_match::Player(index);
    if(!p||p->epoch!=epoch||p->ps!=ps)return Fail(4563,"server_combat_registration_match_tuple_mismatch");
    auto& r=registrations[index];
    if(r.present){
        if(r.pc==pc&&r.ps==ps&&r.pawn==pawn&&r.epoch==epoch)return CurrentRegistration(index);
        return Fail(4564,"server_combat_retirement_required_before_registration");
    }
    Registration candidate;candidate.present=true;candidate.human=p->human;candidate.epoch=epoch;candidate.generation=++nextRegistration;
    candidate.pc=pc;candidate.ps=ps;candidate.pawn=pawn;
    candidate.refillOnBind=state.ammoPolicyKnown&&state.infiniteAmmo&&p->human;
    if(!Identity(pc,candidate.pcId)||!Identity(ps,candidate.psId)||!Identity(pawn,candidate.pawnId))return Fail(4565,"server_combat_registration_weak_identity_failed");
    if(!original_damage::Bind(index,pawn))return Fail(original_damage::State().error,original_damage::State().stage,original_damage::State().restartRequired);
    r=candidate;++state.registeredPlayers;++state.registrations;state.error=0;state.stage="server_combat_player_registered_waiting_equipment";return true;
}
inline bool RegisterPlayer(size_t index,void* pc,void* ps,void* pawn,uint32_t epoch=1){
    __try{const bool okay=RegisterPlayerImpl(index,pc,ps,pawn,epoch);CacheDiagnostics();return okay;}__except(Capture(GetExceptionInformation())){return false;}
}
// A current published authored starter may be a legal non-firearm item.
// This is an identity/admission proof, not a replacement gun context or a
// firearm-readiness claim. A real firing component retains the normal strict
// firearm path, including hero-provided guns; pending/foreign items do not pass.
inline bool AuthoredNonFirearmItem(size_t index,void* item){
    if(!Thread()||index>=MaxPlayers||!CurrentRegistration(index))return false;
    const auto player=local_server_match::Player(index);const auto& r=registrations[index];
    if(!player||!player->human||!player->hero||local_server_match::FreeForAll())return false;
    namespace hero=local_server_hero_abilities;hero::SourceView source{};
    if(!hero::ResolveSource(item,source)||!hero::ValidateSource(source)||source.pendingGrant
       ||source.contextObject!=item||source.ability!=item||source.world!=state.world
       ||source.controller!=r.pc||source.playerState!=r.ps||source.pawn!=r.pawn
       ||source.sourceEpoch!=r.epoch||source.heroIndex!=player->hero||!source.registration
       ||!original_inventory_slots::SameIdentity(source.identities[2],r.pcId)
       ||!original_inventory_slots::SameIdentity(source.identities[3],r.psId)
       ||!original_inventory_slots::SameIdentity(source.identities[4],r.pawnId)
       ||Read<void*>(source.inventory,0x280)!=item)return false;
    struct ComponentQuery{void* type;void* component=nullptr;} query{engine::Class(0x684d4c0)};
    static_assert(sizeof(ComponentQuery)==16);
    if(!original_damage::Event(item,L"/Script/Engine.Actor.GetComponentByClass",16,&query)||query.component)return false;
    hero::SourceView current{};
    if(!hero::ResolveSource(item,current)||current.pendingGrant||!hero::ValidateSource(current)
       ||current.ability!=item||current.contextObject!=item||current.registration!=source.registration
       ||current.sourceEpoch!=source.sourceEpoch||current.world!=source.world||current.heroIndex!=source.heroIndex
       ||current.controller!=source.controller||current.playerState!=source.playerState
       ||current.pawn!=source.pawn||current.inventory!=source.inventory||!CurrentRegistration(index)
       ||Read<void*>(current.inventory,0x280)!=item)return false;
    for(size_t n=0;n<source.identities.size();++n)
        if(!original_inventory_slots::SameIdentity(source.identities[n],current.identities[n]))return false;
    return true;
}
inline bool BindWeaponImpl(size_t index,void* weapon){
    if(!Thread()||!CurrentRegistration(index))return Fail(4566,"server_combat_weapon_bind_requires_current_authority_registration");
    auto& r=registrations[index];auto& s=firearms::sources[index];
    if(!weapon)return true;
    if(!original_players::IsA(weapon,0x645c660)||Read<uint8_t>(weapon,0x224)!=3
       ||Read<void*>(weapon,0x4a8)!=r.pawn)return Fail(4567,"server_combat_weapon_role_or_typed_owner_invalid");
    // An authored Q/E/X may temporarily occupy CurrentEquippable. Its native
    // action/firing logic remains with the ability module; retain the real gun
    // or melee observer instead of inventing a firearm context for that item.
    if(const auto abilityPlayer=local_server_abilities::Find(r.pawn))
        for(const auto& skill:abilityPlayer->skills)if(skill.actor==weapon)return true;
    if(AuthoredNonFirearmItem(index,weapon))return true;
    if(s.bound&&s.weapon==weapon&&s.pawn==r.pawn&&s.spawnEpoch==r.epoch)return true;
    if(!melee::BeforeRetire(index)||!firearms::BeforeRetire(index,firearms::RetireReason::WeaponChange))return false;
    if(!firearms::Bind(index,r.pawn,weapon,r.epoch))return Fail(firearms::State().error,firearms::State().stage,firearms::State().restartRequired);
    if(!melee::Bind(index,r.pawn,weapon,r.epoch)){
        firearms::BeforeRetire(index,firearms::RetireReason::WeaponChange);
        return Fail(melee::State().error,melee::State().stage,melee::State().restartRequired);
    }
    ++state.weaponBinds;r.refillOnBind=state.ammoPolicyKnown&&state.infiniteAmmo&&r.human;
    state.error=0;state.stage="server_combat_native_weapon_observers_bound";return true;
}
inline bool BindWeapon(size_t index,void* weapon){__try{const bool okay=BindWeaponImpl(index,weapon);CacheDiagnostics();return okay;}__except(Capture(GetExceptionInformation())){return false;}}
inline bool WeaponInput(size_t index,bool fire,bool reload){
    if(!Thread()||!policy::NPC(index)||!CurrentRegistration(index)){++state.npcInputRejected;return false;}
    // An admitted new NPC can receive a neutral scheduler callback before its
    // first weapon binding. No action was ever produced by this unbound source.
    // A surviving held bit must still fail instead of losing a release.
    const auto& source=firearms::sources[index];
    if(!source.bound&&!fire&&!reload&&!source.held&&!source.inputEdges&&!source.inputSubmissions&&!source.viewSubmissions){
        ++firearms::diagnostics.inputDeferred;return true;
    }
    const bool okay=firearms::WeaponInput(index,fire,reload);
    const bool result=SubsystemResult(okay,firearms::State().error,firearms::State().stage,firearms::State().restartRequired);
    __try{CacheDiagnostics();}__except(Capture(GetExceptionInformation())){return false;}
    return result;
}
inline bool SubmitViewInput(size_t index){
    if(!Thread()||!policy::NPC(index)){++state.npcInputRejected;return false;}
    const bool okay=firearms::SubmitViewInput(index);
    return SubsystemResult(okay,firearms::State().error,firearms::State().stage,firearms::State().restartRequired);
}
inline bool Visible(size_t source,size_t target){return Thread()&&firearms::Visible(source,target);}
inline bool Ready(size_t index){return Thread()&&CurrentRegistration(index)&&firearms::Ready(index);}

struct HealParameters {
    float amount=0;uint32_t pad=0;original_damage::Array<original_damage::LifeChange> changes;
    void *instigator=nullptr,*instigatorPawn=nullptr,*causer=nullptr;
};
static_assert(sizeof(HealParameters)==0x30&&offsetof(HealParameters,changes)==8&&offsetof(HealParameters,causer)==40);
inline bool ActualCanSetLife(void* section,bool& allowed){
    auto fn=engine::Find(L"/Script/ShooterGame.DamageSectionComponent.CanSetLife");if(!fn)return false;
    original_damage::Name name;std::memcpy(name.bytes,static_cast<uint8_t*>(fn)+0x18,12);
    auto actual=engine::function<void*(*)(void*,const original_damage::Name*)>(0x1cdc710)(section,&name);uint8_t result=0;
    if(!original_damage::EventWithFunction(section,L"/Script/ShooterGame.DamageSectionComponent.CanSetLife",actual,1,&result))return false;
    allowed=result!=0;return true;
}
inline bool RestoreMaximumImpl(size_t index){
    ++state.healRequests;
    if(!Thread()||!CurrentRegistration(index)||!local_server_match::Active())return false;
    const auto p=local_server_match::Player(index);if(!p||!p->alive||p->phase!=policy::Phase::Alive)return false;
    if(local_server_match::pending.active||Read<int32_t>(Read<void*>(p->pawn,0xc70),0x248)){
        ++state.healsDeferred;return false;
    }
    original_damage::SectionAudit audit;
    if(!original_damage::Sample(index)||!original_damage::DescribeSections(p->pawn,Read<void*>(p->pawn,0xc70),audit))
        return Fail(4570,"server_combat_heal_original_sections_unavailable");
    std::array<original_damage::LifeChange,2> changes{};int32_t count=0;float amount=0;unsigned health=0,shield=0;
    for(int32_t n=0;n<audit.count;++n){const auto& row=audit.entries[n];
        if(row.emptyShield)continue;
        if(row.type>1){if(row.nativeLife>0)return Fail(4571,"server_combat_heal_nonstandard_living_section");continue;}
        if((row.type==0&&++health>1)||(row.type==1&&++shield>1)||row.nativeMaximum<=0
           ||row.nativeLife>row.nativeMaximum+.01f||row.damageScale!=1||!Read<uint8_t>(row.object,0x149))
            return Fail(4572,"server_combat_heal_original_section_policy_not_supported");
        if(row.nativeMaximum-row.nativeLife<=.0001f)continue;
        bool allowed=false;if(!ActualCanSetLife(row.object,allowed)||!allowed)return false;
        if(count>=static_cast<int32_t>(changes.size()))return false;
        auto& change=changes[count++];change.section=row.object;change.life=row.nativeMaximum;
        change.delta=row.nativeMaximum-row.nativeLife;change.alive=1;amount+=change.delta;
    }
    if(health!=1)return Fail(4573,"server_combat_heal_requires_one_original_health_section");
    if(!count)return true;
    auto score=original_scoreboard::Find(p->ps);if(!score||!original_scoreboard::ReadNativeScore(*score))return false;
    const auto kills=score->kills,deaths=score->deaths;const auto epoch=p->epoch;const auto pawn=p->pawn;
    HealParameters heal;heal.amount=amount;heal.changes={changes.data(),count,static_cast<int32_t>(changes.size())};
    // One original multicast, including its host implementation and native
    // transport. Unlike the retained standalone helper, no local-only leaf is
    // used here. Positive LifeChange records never resurrect a dead character.
    if(!original_damage::Event(audit.damageable,L"/Script/ShooterGame.DamageableComponent.MulticastNotifyHeal",0x30,&heal)
       ||!original_damage::Sample(index)||!original_scoreboard::ReadNativeScore(*score))return Fail(4574,"server_combat_original_heal_multicast_failed");
    const auto current=local_server_match::Player(index);
    if(!current||current->pawn!=pawn||current->epoch!=epoch||!original_damage::targets[index].alive
       ||score->kills!=kills||score->deaths!=deaths)return Fail(4575,"server_combat_heal_life_or_score_guard_failed",true);
    for(int32_t n=0;n<count;++n)if(std::abs(Read<float>(changes[n].section,0x124)-changes[n].life)>.01f)
        return Fail(4576,"server_combat_heal_native_maximum_readback_failed");
    ++state.heals;return true;
}
inline bool RestoreMaximum(size_t index){__try{return RestoreMaximumImpl(index);}__except(Capture(GetExceptionInformation())){return false;}}

inline constexpr const wchar_t* AmmoClassPath=L"/Game/GameModes/Components/Comp_PlayerCharacter_AuthAmmoReplenisher.Comp_PlayerCharacter_AuthAmmoReplenisher_C";
inline constexpr const wchar_t* AmmoAuthorityPath=L"/Game/GameModes/Components/Comp_PlayerCharacter_AuthAmmoReplenisher.Comp_PlayerCharacter_AuthAmmoReplenisher_C.Auth Replenish Ammo";
inline void* ammoClass=nullptr;
struct AmmoRefillReadback {
    bool valid=false,reservePresent=false;size_t index=0;
    void *pawn=nullptr,*weapon=nullptr;uint32_t epoch=0;
    int32_t magazineBefore=0,magazineAfter=0,magazineMaximum=0;
    int32_t reserveBefore=0,reserveAfter=0,reserveMaximum=0;
};
inline AmmoRefillReadback lastAmmoRefill;
// Shared authority operation. The queued kill may refer to an owned firearm
// that stopped being current during the authored 0.1 s delay. No equip/input
// change is made to call its actual original resource/RPC path.
inline bool RefillAmmoForWeaponImpl(size_t index,void* pawn,void* weapon){
    lastAmmoRefill={};
    if(!Thread()||!CurrentRegistration(index))return false;
    auto& r=registrations[index];const auto p=local_server_match::Player(index);
    const auto pawnId=r.pawnId;const auto epoch=r.epoch;
    if(r.pawn!=pawn||!p||!p->alive||p->phase!=policy::Phase::Alive
       ||!original_players::IsA(weapon,0x645c660)||Read<void*>(weapon,0x4a8)!=pawn
       ||Read<uint8_t>(weapon,0x224)!=3)return false;
    const auto inventory=Read<void*>(pawn,0xc08);
    auto ammo=original_weapon::Component(weapon,0x63c7e00);
    Weak weaponId{},inventoryId{},ammoId{},reserveId{};
    if(!Identity(weapon,weaponId)||!Identity(inventory,inventoryId)||!Identity(ammo,ammoId)
       ||Read<void*>(inventory,0xc8)!=pawn||Read<void*>(inventory,0x2f0)!=pawn
       ||Read<void*>(ammo,0xc8)!=weapon)return false;
    uint8_t alive=0;
    if(!original_damage::Event(pawn,L"/Script/ShooterGame.ShooterCharacter.IsAlive",1,&alive)||alive!=1)return false;
    if(!ammoClass)ammoClass=engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t,void*)>(0x1d0c6e0)
        (engine::Class(0x36837a0),nullptr,AmmoClassPath,nullptr,0,nullptr);
    if(!ammoClass)return Fail(4577,"server_combat_original_ammo_replenisher_class_missing");
    if(!Live(r.ammoReplenisher,r.replenisherId)){
        struct Query {void* type;void* result=nullptr;} q{ammoClass};
        if(!original_damage::Event(r.pawn,L"/Script/Engine.Actor.GetComponentByClass",16,&q))return false;
        r.ammoReplenisher=q.result;
        if(!r.ammoReplenisher){
            struct alignas(16) Add {void* type;uint8_t manual=0,pad[7]{};engine::Transform transform;uint8_t deferred=0,pad2[7]{};void* result=nullptr;} add{};
            static_assert(sizeof(Add)==128);add.type=ammoClass;
            if(!original_damage::Event(r.pawn,L"/Script/Engine.Actor.AddComponentByClass",128,&add))return false;
            r.ammoReplenisher=add.result;
        }
        if(Read<void*>(r.ammoReplenisher,0xc8)!=r.pawn||!Identity(r.ammoReplenisher,r.replenisherId))return Fail(4578,"server_combat_native_ammo_replenisher_owner_invalid");
    }
    if(!original_players::Derived(Read<void*>(r.ammoReplenisher,0x10),ammoClass)
       ||Read<void*>(r.ammoReplenisher,0xc8)!=pawn)return Fail(4578,"server_combat_native_ammo_replenisher_owner_invalid");
    int32_t maximum=0,before=0,after=0;
    if(!original_damage::Event(ammo,L"/Script/ShooterGame.AmmoComponent.GetAmmo",4,&before)
       ||!original_damage::Event(ammo,L"/Script/ShooterGame.AmmoComponent.GetMaxAmmo",4,&maximum)
       ||maximum<=0||maximum>65535||before<0||before>maximum)return false;
    const auto reserve=Read<void*>(ammo,0x138);int32_t reserveMaximum=0,reserveBefore=0,reserveAfter=0;
    if(reserve&&(!original_players::IsA(reserve,0x63c7e00)
       ||!Identity(reserve,reserveId)||Read<void*>(reserve,0xc8)!=weapon
       ||!original_damage::Event(reserve,L"/Script/ShooterGame.AmmoComponent.GetAmmo",4,&reserveBefore)
       ||!original_damage::Event(reserve,L"/Script/ShooterGame.AmmoComponent.GetMaxAmmo",4,&reserveMaximum)
       ||reserveMaximum<0||reserveMaximum>65535||reserveBefore<0||reserveBefore>reserveMaximum))return false;
    struct AmmoParameters {void* gun;int32_t magazine=0,reserve=0;} q{weapon,maximum,reserveMaximum};
    static_assert(sizeof(AmmoParameters)==16);
    auto fn=engine::Find(AmmoAuthorityPath);
    if(!fn||Read<uint16_t>(fn,0xbe)!=16||Read<int32_t>(fn,0x70)!=72)return Fail(4579,"server_combat_ammo_authority_blueprint_abi_changed");
    // The original host event applies ReplenishAmmo once and sends its actual
    // replenished counts through the original Local Client Force Update RPC.
    // The standalone leaf is deliberately not called as well.
    if(!original_damage::EventWithFunction(r.ammoReplenisher,AmmoAuthorityPath,fn,16,&q)
       ||!CurrentRegistration(index)||r.pawn!=pawn||r.epoch!=epoch||!original_inventory_slots::SameIdentity(r.pawnId,pawnId)
       ||!Live(weapon,weaponId)||!Live(inventory,inventoryId)||!Live(ammo,ammoId)
       ||Read<void*>(weapon,0x4a8)!=pawn||Read<void*>(pawn,0xc08)!=inventory
       ||Read<void*>(ammo,0xc8)!=weapon||Read<void*>(ammo,0x138)!=reserve
       ||!Live(r.ammoReplenisher,r.replenisherId)||Read<void*>(r.ammoReplenisher,0xc8)!=pawn
       ||!original_damage::Event(ammo,L"/Script/ShooterGame.AmmoComponent.GetAmmo",4,&after)||after!=maximum
       ||(reserve&&(!Live(reserve,reserveId)||!original_damage::Event(reserve,L"/Script/ShooterGame.AmmoComponent.GetAmmo",4,&reserveAfter)||reserveAfter!=reserveMaximum)))
        return Fail(4580,"server_combat_native_ammo_refill_readback_failed");
    lastAmmoRefill={true,reserve!=nullptr,index,pawn,weapon,r.epoch,before,after,maximum,reserveBefore,reserveAfter,reserveMaximum};
    ++state.ammoRefills;return true;
}
inline bool RefillAmmoForWeapon(size_t index,void* pawn,void* weapon){__try{return RefillAmmoForWeaponImpl(index,pawn,weapon);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool RefillAmmoImpl(size_t index){
    if(!Thread()||!CurrentRegistration(index))return false;
    auto& r=registrations[index];auto& s=firearms::sources[index];
    if(!s.bound||!s.ballistic||!firearms::SourceIdentity(s))return true;
    if(!s.alive||!local_server_match::Player(index)->alive)return false;
    if(!RefillAmmoForWeaponImpl(index,r.pawn,s.weapon))return false;
    r.refillOnBind=false;return true;
}
inline bool RefillAmmo(size_t index){__try{return RefillAmmoImpl(index);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool PublishAmmoPolicy(){
    const auto requested=(local_server_match::Configuration().config.flags&training_control::InfiniteAmmo)!=0;
    uint8_t actual=0;
    if(!original_damage::Event(state.gameState,L"/Script/ShooterGame.ShooterGameState.IsInfiniteAmmoEnabled",1,&actual))return Fail(4581,"server_combat_native_ammo_rule_getter_missing");
    const bool enableEdge=requested&&(!state.ammoPolicyKnown||!state.infiniteAmmo);
    if((actual!=0)!=requested){
        auto previous=Read<std::array<uint8_t,64>>(state.gameState,0x7d8);
        if(previous[0x33]>1||previous[0x34]>1)return Fail(4582,"server_combat_original_game_info_bool_invalid");
        auto expected=previous;expected[0x33]=requested?1:0;
        if(!original_players::Write(state.gameState,0x7d8+0x33,expected[0x33])
           ||Read<std::array<uint8_t,64>>(state.gameState,0x7d8)!=expected
           ||!original_damage::Event(state.gameState,L"/Script/ShooterGame.ShooterGameState.OnRep_GameInfo",64,previous.data()))
            return Fail(4583,"server_combat_native_game_info_ammo_publication_failed");
        ++state.ammoRuleWrites;
    }
    if(!original_damage::Event(state.gameState,L"/Script/ShooterGame.ShooterGameState.IsInfiniteAmmoEnabled",1,&actual)
       ||(actual!=0)!=requested)return Fail(4584,"server_combat_native_ammo_rule_readback_mismatch");
    state.ammoPolicyKnown=true;state.infiniteAmmo=actual!=0;++state.ammoRuleReadbacks;
    state.invincible=(local_server_match::Configuration().config.flags&training_control::Invincible)!=0;
    if(enableEdge)for(auto& r:registrations)if(r.present&&r.human)r.refillOnBind=true;
    return true;
}
inline bool ResetStreakImpl(size_t index){
    if(!Thread()||!CurrentRegistration(index))return false;auto& p=local_server_match::players[index];
    if(local_server_match::FreeForAll()){
        if(!original_timed_streak::Reset(index)||!local_server_match::RefreshStreak(index))return false;
    }else{
        void* component=nullptr;int32_t count=-1;
        if(!original_damage::Event(p.info,L"/Script/ShooterGame.OwnerExclusivePlayerInfo.GetKillStreakComponentMutable",8,&component)
           ||!original_damage::Event(component,L"/Script/ShooterGame.KillStreakComponent.ResetKills",0,nullptr)
           ||!original_damage::Event(component,L"/Script/ShooterGame.KillStreakComponent.GetNumKillsStreak",4,&count)||count!=0)return false;
        p.streak=0;
    }
    p.lastKill=-1;return true;
}
inline bool ResetStreak(size_t index){__try{return ResetStreakImpl(index);}__except(Capture(GetExceptionInformation())){return false;}}

struct DaggerSeen {Weak projectile{},component{};uint32_t initialized=0;int32_t index=-1;uint64_t hash=0;};
struct DaggerContact {
    size_t source=MaxPlayers,victim=MaxPlayers;uint32_t sourceEpoch=0,victimEpoch=0;
    uint64_t sourceGeneration=0,token=0,fireSequence=0;double at=0;
    void *pawn=nullptr,*victimPawn=nullptr,*ability=nullptr,*damageType=nullptr;
    Weak pawnId{},victimId{},abilityId{},damageTypeId{};
    original_shots::Shot shot;original_damage::Hit hit;float damage=0,falloff=1;bool primary=false;
    melee::LifeSnapshot before{};
};
inline std::array<DaggerSeen,2048> daggerSeen{};
inline std::array<DaggerContact,256> daggerQueue{};
inline std::array<melee::LifeSnapshot,MaxPlayers> daggerBaseline{};
struct FireBoundary {
    bool active=false;uint32_t epoch=0;uint64_t generation=0,sequence=0;
    void* fireState=nullptr;Weak fireId{};
    std::array<melee::LifeSnapshot,MaxPlayers> lives{};
};
inline std::array<FireBoundary,MaxPlayers> fireBoundaries{};
inline uint64_t daggerRead=0,daggerWrite=0,daggerSeenCursor=0;
inline void OnFireBoundaryImpl(void* pawn,void* fireState,uint64_t sequence,bool before){
    if(!Thread()||!sequence)return;
    size_t source=MaxPlayers;
    for(size_t n=0;n<registrations.size();++n)if(registrations[n].present&&registrations[n].pawn==pawn&&CurrentRegistration(n)){source=n;break;}
    if(source==MaxPlayers)return;auto& boundary=fireBoundaries[source];
    if(!before){
        if(boundary.sequence==sequence&&boundary.fireState==fireState)boundary.active=false;
        return;
    }
    boundary={};boundary.active=true;boundary.epoch=registrations[source].epoch;
    boundary.generation=registrations[source].generation;boundary.sequence=sequence;boundary.fireState=fireState;
    if(!Identity(fireState,boundary.fireId)){boundary.active=false;return;}
    for(size_t n=0;n<MaxPlayers;++n)melee::ReadLife(n,boundary.lives[n]);
}
inline void OnFireBoundary(void* pawn,void* fireState,uint64_t sequence,bool before){
    __try{OnFireBoundaryImpl(pawn,fireState,sequence,before);}__except(Capture(GetExceptionInformation())){}
}
inline uint64_t Hash(const void* input,size_t bytes){
    const auto data=static_cast<const uint8_t*>(input);uint64_t value=14695981039346656037ULL;
    for(size_t n=0;n<bytes;++n)value=(value^data[n])*1099511628211ULL;return value;
}
inline uint8_t OriginalDaggerClass(void* ability,void* projectileClass){
    constexpr const wchar_t* names[]{L"ProjectileArray",L"BurstProjectileArray"};
    uint8_t result=0;
    for(size_t kind=0;kind<2;++kind){const auto name=names[kind];
        const int32_t offset=original_jett_priming::FieldOffset(Read<void*>(ability,0x10),name,16);if(offset<0)continue;
        auto data=Read<void**>(ability,static_cast<size_t>(offset));
        const auto count=Read<int32_t>(ability,static_cast<size_t>(offset)+8),capacity=Read<int32_t>(ability,static_cast<size_t>(offset)+12);
        if(count<1||count>64||capacity<count||!original_players::Readable(data,static_cast<size_t>(count)*8))continue;
        for(int32_t n=0;n<count;++n)if(data[n]==projectileClass)result|=static_cast<uint8_t>(uint8_t(1)<<kind);
    }return result;
}
inline void OnNativePointImpl(const local_server_abilities::NativePoint& point){
    if(!Thread()||!point.geometryValid||!local_server_match::AbilitiesAllowed())return;
    ++state.daggerPoints;state.nativeDaggerObserved=true;
    size_t source=MaxPlayers,victim=MaxPlayers;
    for(size_t n=0;n<registrations.size();++n){const auto& r=registrations[n];
        if(r.present&&r.pawn==point.pawn&&r.ps==point.playerState&&CurrentRegistration(n))source=n;
        if(r.present&&r.pawn==point.victim&&CurrentRegistration(n))victim=n;
    }
    if(source==MaxPlayers||victim==MaxPlayers||source==victim||point.nativeIndex<0
       ||!Live(point.projectile,point.projectileId)||!Live(point.component,point.componentId)||!Live(point.victim,point.victimId)
       ||Read<uint8_t>(point.projectile,0x224)!=3||Read<uint8_t>(point.ability,0x224)!=3
       ||Read<void*>(point.component,0xc8)!=point.projectile||Read<void*>(point.ability,0x4a8)!=point.pawn
       ||!OriginalDaggerClass(point.ability,Read<void*>(point.projectile,0x10))){++state.daggerInvalid;return;}
    const auto abilityPlayer=local_server_abilities::Find(point.pawn);
    if(!abilityPlayer||!local_server_abilities::ProjectileSource(*abilityPlayer,point.projectile,point.playerState)){
        ++state.daggerInvalid;return;
    }
    auto data=Read<uint8_t*>(point.component,0x370);const auto count=Read<int32_t>(point.component,0x378),capacity=Read<int32_t>(point.component,0x37c);
    const auto initialized=Read<float>(point.projectile,0x518);uint32_t initializedBits=0;std::memcpy(&initializedBits,&initialized,4);
    if(!std::isfinite(initialized)||initializedBits!=point.initializedBits||count<1||count>8192||capacity<count||point.nativeIndex>=count
       ||!original_players::Readable(data,static_cast<size_t>(count)*0x130)
       ||std::memcmp(data+static_cast<size_t>(point.nativeIndex)*0x130,point.bytes.data(),0x130)!=0){++state.daggerInvalid;return;}
    void* firingState=nullptr;
    if(!original_damage::Event(point.projectile,L"/Script/ShooterGame.Projectile.GetFiringPlayerState",8,&firingState)
       ||firingState!=point.playerState){++state.daggerInvalid;return;}
    const auto hash=Hash(point.bytes.data(),point.bytes.size());
    for(const auto& seen:daggerSeen)if(seen.index==point.nativeIndex&&seen.initialized==point.initializedBits&&seen.hash==hash
       &&original_inventory_slots::SameIdentity(seen.projectile,point.projectileId)
       &&original_inventory_slots::SameIdentity(seen.component,point.componentId)){++state.daggerDuplicates;return;}
    const auto& fire=fireBoundaries[source];
    if(point.fromFactory&&(!fire.active||!point.fireSequence||point.fireSequence!=fire.sequence||point.fireState!=fire.fireState
       ||fire.epoch!=registrations[source].epoch||fire.generation!=registrations[source].generation||!Live(fire.fireState,fire.fireId))){++state.daggerNoBaseline;return;}
    const auto& before=point.fromFactory?fire.lives[victim]:daggerBaseline[victim];
    if(!before.valid||before.pawn!=point.victim||before.epoch!=registrations[victim].epoch){++state.daggerNoBaseline;return;}
    if(!melee::SameLife(before)){++state.daggerNativeChanged;return;}
    const auto native=point.bytes.data();
    const auto hitComponent=original_inventory_slots::ResolveIdentity(Read<Weak>(native,0x30+0xd0));
    if(hitComponent!=original_damage::targets[victim].core){++state.daggerInvalid;return;}
    DaggerContact contact;contact.source=source;contact.victim=victim;
    contact.before=before;contact.fireSequence=point.fireSequence;
    contact.sourceEpoch=registrations[source].epoch;contact.victimEpoch=registrations[victim].epoch;contact.sourceGeneration=registrations[source].generation;
    contact.pawn=point.pawn;contact.victimPawn=point.victim;contact.ability=point.ability;contact.damageType=Read<void*>(native,8);
    contact.pawnId=registrations[source].pawnId;contact.victimId=point.victimId;
    if(!Identity(point.ability,contact.abilityId)||!Identity(contact.damageType,contact.damageTypeId))return;
    contact.damage=Read<float>(native,0x10);contact.falloff=Read<float>(native,0x124);contact.at=Now();
    contact.shot.observed_at=contact.at;contact.shot.start=Read<engine::Vec>(point.projectile,0x570);contact.shot.direction=Read<engine::Vec>(native,0x18);
    contact.hit.hit=true;contact.hit.blocking=(native[0x30+0xad]&1)!=0;contact.hit.region=native[0x120];contact.hit.component=hitComponent;
    contact.hit.distance=Read<float>(native,0x38);contact.hit.location=Read<engine::Vec>(native,0x58);contact.hit.normal=Read<engine::Vec>(native,0x88);
    std::memcpy(contact.hit.bone.bytes,native+0x108,12);std::memcpy(contact.hit.nativeHit,native+0x30,0xf0);
    const auto primary=engine::Find(L"/Game/Characters/Wushu/S0/Ability_X/DmgType_Wushu_X_Dagger.DmgType_Wushu_X_Dagger_C");
    const auto burst=engine::Find(L"/Game/Characters/Wushu/S0/Ability_X/DmgType_Wushu_X_Dagger_Burst.DmgType_Wushu_X_Dagger_Burst_C");
    contact.primary=OriginalDaggerClass(point.ability,Read<void*>(point.projectile,0x10))==1
        &&primary&&original_players::Derived(contact.damageType,primary)
        &&(!burst||!original_players::Derived(contact.damageType,burst));
    if(!original_players::Derived(contact.damageType,engine::Class(original_damage::DamageTypeClass))
       ||(!original_players::Derived(contact.damageType,primary)&&!original_players::Derived(contact.damageType,burst))
       ||!std::isfinite(contact.damage)||contact.damage<=0||contact.damage>1000000||!std::isfinite(contact.falloff)||contact.falloff<0
       ||contact.hit.region>2||!std::isfinite(contact.hit.distance)||contact.hit.distance<0
       ||!original_players::Finite(contact.hit.location)||!original_players::Finite(contact.hit.normal)){++state.daggerInvalid;return;}
    if(daggerWrite-daggerRead>=daggerQueue.size()){Fail(4585,"server_combat_dagger_queue_full",true);return;}
    contact.token=AllocateDamageToken();if(!contact.token)return;contact.shot.sequence=contact.token;
    daggerSeen[daggerSeenCursor++%daggerSeen.size()]={point.projectileId,point.componentId,point.initializedBits,point.nativeIndex,hash};
    daggerQueue[daggerWrite++%daggerQueue.size()]=contact;++state.daggerQueued;
}
inline void OnNativePoint(const local_server_abilities::NativePoint& point){
    __try{OnNativePointImpl(point);}__except(Capture(GetExceptionInformation())){}
}
inline bool RefillPrimaryDaggers(const DaggerContact& contact){
    if(!contact.primary||!CurrentRegistration(contact.source))return true;
    auto player=local_server_abilities::Find(contact.pawn);
    if(!player||!player->authority||player->skills[3].actor!=contact.ability)return false;
    auto resource=player->skills[3].resource;int32_t before=-1,after=-1;
    const auto offset=original_jett_priming::FieldOffset(Read<void*>(resource,0x10),L"MaxDaggers",4);
    if(offset<0||!original_damage::Event(resource,original_jett::DaggerRemaining,4,&before))return false;
    const auto maximum=Read<int32_t>(resource,static_cast<size_t>(offset));
    if(maximum<1||maximum>64||before<0||before>maximum)return false;
    if(before==maximum)return true;
    const auto disable=std::wstring(original_jett::DaggerResourcePath)+L".DisableDaggerReset";
    const auto fn=engine::Find(disable.c_str());
    if(!fn||Read<void*>(fn,0x28)!=Read<void*>(resource,0x10)||Read<uint16_t>(fn,0xbe)!=0||Read<int32_t>(fn,0x70)!=14
       ||!original_damage::EventWithFunction(resource,disable.c_str(),fn,0,nullptr)
       ||!original_damage::Event(resource,original_jett::DaggerReplenish,0,nullptr)
       ||!original_damage::Event(resource,original_jett::DaggerRemaining,4,&after)||after!=maximum)
        return Fail(4594,"server_combat_original_primary_kill_dagger_refill_unobserved");
    ++state.primaryDaggerRefills;return true;
}
inline bool ProcessDaggers(double now){
    const auto end=daggerWrite;size_t budget=32;
    while(daggerRead<end&&budget--){const auto& c=daggerQueue[daggerRead%daggerQueue.size()];
        if(!CurrentRegistration(c.source)||!CurrentRegistration(c.victim)||registrations[c.source].generation!=c.sourceGeneration
           ||registrations[c.source].epoch!=c.sourceEpoch||registrations[c.victim].epoch!=c.victimEpoch
           ||!Live(c.pawn,c.pawnId)||!Live(c.victimPawn,c.victimId)||!Live(c.ability,c.abilityId)||!Live(c.damageType,c.damageTypeId)
           ||now<c.at||now-c.at>.5){++state.daggerStale;++daggerRead;continue;}
        if(!policy::CanDamage(c.source,c.victim)){++daggerRead;continue;}
        if(!original_damage::Sample(c.victim))return false;
        if(original_damage::targets[c.victim].pendingDamageEvents)break;
        // Recheck after native queued damage has had its turn. The original
        // callback's unchanged life at admission does not prove it stayed so.
        // The now-empty native event queue may have consumed pending entries.
        auto expected=c.before;expected.pendingEvents=0;
        if(!melee::SameLife(expected)){++state.daggerNativeChanged;++daggerRead;continue;}
        firearms::NativeLife beforePoint{};
        if(!firearms::ReadNativeLife(c.victim,beforePoint)){++state.daggerNativeChanged;++daggerRead;continue;}
        original_damage::DeathData death;
        if(!policy::BeginNativeDamage(c.token,c.source,c.victim,c.ability,now,death))return false;
        original_damage::Result result;
        const bool applied=ApplyAuthorityPoint(c.victim,c.pawn,c.ability,c.damageType,c.shot,c.hit,c.damage,c.token,result,&death,c.falloff);
        const bool committed=policy::EndNativeDamage(applied,result);
        if(!applied||!committed)return Fail(!applied?original_damage::State().error:local_server_match::GetState().error,
            !applied?original_damage::State().stage:local_server_match::GetState().stage);
        if(!result.duplicate&&!firearms::RecordNativeLifeReceipt(c.victim,beforePoint))
            return Fail(4665,"server_dagger_native_point_life_receipt_unavailable",true);
        if(result.applied&&!result.duplicate){
            ++state.daggerDamage;
            // Several authored burst daggers can hit one victim in one fire
            // boundary. Advance only those identical baselines after OUR own
            // verified point, so this guard does not discard the other real
            // contacts merely because the preceding adapter point changed life.
            melee::LifeSnapshot after;
            if(!result.killed&&melee::ReadLife(c.victim,after)&&after.epoch==c.victimEpoch
               &&after.pawn==c.before.pawn&&after.handler==c.before.handler
               &&after.health==c.before.health&&after.shield==c.before.shield
               &&after.healthValue==result.healthAfter&&after.shieldValue==result.shieldAfter
               &&after.eventIndex==original_damage::targets[c.victim].lastNativeEventIndex
               &&after.pendingEvents==0){
                for(auto n=daggerRead+1;n<end;++n){auto& next=daggerQueue[n%daggerQueue.size()];const auto& old=next.before;
                    if(next.source!=c.source||next.victim!=c.victim||next.sourceGeneration!=c.sourceGeneration
                       ||!c.fireSequence||next.fireSequence!=c.fireSequence)continue;
                    if(old.valid==c.before.valid&&old.alive==c.before.alive&&old.pawn==c.before.pawn&&old.epoch==c.before.epoch
                       &&old.handler==c.before.handler&&old.health==c.before.health&&old.shield==c.before.shield
                       &&old.healthValue==c.before.healthValue&&old.shieldValue==c.before.shieldValue
                       &&old.eventIndex==c.before.eventIndex&&old.pendingEvents==c.before.pendingEvents)next.before=after;
                }
            }
        }
        if(result.killed&&!result.duplicate){
            ++state.daggerKills;
            if(!RefillPrimaryDaggers(c)||!BeforeNativeDeath(c.victim))return false;
        }
        ++daggerRead;
    }return true;
}
// Real Base/Radial response records have distinct payloads and original RPCs.
// The compositor supplies an exact-current registered source proof; this layer
// independently rechecks the real match tuples, native identities and refs.
struct NativeDamageContext {
    size_t source=MaxPlayers,victim=MaxPlayers;uint32_t sourceEpoch=0,victimEpoch=0;
    void *sourcePawn=nullptr,*controller=nullptr,*playerState=nullptr,*equippable=nullptr;
    void *causer=nullptr,*damageType=nullptr,*victimPawn=nullptr;
    Weak sourceId{},controllerId{},playerStateId{},equippableId{},causerId{},damageTypeId{},victimId{};
};
struct BaseDamageMulticast {original_damage::ResponseData base;original_damage::DeathData death;};
struct RadialDamageMulticast {original_damage::RadialData radial;original_damage::DeathData death;};
static_assert(sizeof(BaseDamageMulticast)==0xc8&&offsetof(BaseDamageMulticast,death)==0x90);
static_assert(sizeof(RadialDamageMulticast)==0x178&&offsetof(RadialDamageMulticast,death)==0x140);
inline bool NativeDamageReferenceProfiles(){
    struct Profile {uintptr_t rva;size_t bytes;uint64_t hash;};
    constexpr Profile profiles[]{{0x437af30,75,0xc179b4a8261c54ebULL},{0x437b290,276,0x1c59110e9b3f5241ULL}};
    for(const auto& p:profiles){auto bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes))return Fail(4663,"server_combat_native_event_reference_profile_unreadable",true);
        uint64_t hash=14695981039346656037ULL;
        for(size_t n=0;n<p.bytes;++n)hash=(hash^bytes[n])*1099511628211ULL;
        if(hash!=p.hash)return Fail(4663,"server_combat_native_event_reference_profile_mismatch",true);
    }
    return true;
}
inline bool NativeDamageTuple(const NativeDamageContext& c){
    if(!Thread()||state.error||state.replicationFault||c.source>=MaxPlayers||c.victim>=MaxPlayers
       ||c.source==c.victim||!CurrentRegistration(c.source)||!CurrentRegistration(c.victim))return false;
    const auto s=local_server_match::Player(c.source),v=local_server_match::Player(c.victim);
    return s&&v&&s->human&&s->epoch==c.sourceEpoch&&v->epoch==c.victimEpoch
        &&s->pawn==c.sourcePawn&&s->pc==c.controller&&s->ps==c.playerState&&v->pawn==c.victimPawn
        &&Live(c.sourcePawn,c.sourceId)&&Live(c.controller,c.controllerId)&&Live(c.playerState,c.playerStateId)
        &&Live(c.equippable,c.equippableId)&&Live(c.causer,c.causerId)&&Live(c.damageType,c.damageTypeId)
        &&Live(c.victimPawn,c.victimId)&&original_players::IsA(c.equippable,0x645c660)
        &&Read<void*>(c.equippable,0x4a8)==s->pawn&&Read<uint8_t>(c.equippable,0x224)==3
        &&Read<uint8_t>(c.causer,0x224)==3
        &&original_players::Derived(c.damageType,engine::Class(original_damage::DamageTypeClass))
        &&policy::CanDamage(c.source,c.victim);
}
inline bool NativeDamageReference(void* connection,void* object,const char* field){
    if(!object)return true;Weak identity{};
    if(!Identity(object,identity))return BlockReference(field,object);
    const auto driver=Read<void*>(state.world,0x40);
    if(!connection||Read<void*>(connection,0x60)!=driver||Read<void*>(driver,0xc0))return BlockReference(field,object);
    const auto peers=Read<void**>(driver,0xc8);
    if(Read<int32_t>(driver,0xd0)!=1||Read<void*>(peers,0)!=connection)return BlockReference(field,object);
    auto packageMap=Read<void*>(connection,0x70);
    if(!original_players::IsA(packageMap,0x3cf26c0)
       ||Read<void*>(Read<void**>(packageMap,0),0x2f0)!=engine::function<void*>(0x4777940))return BlockReference(field,object);
    auto guidCache=Read<void*>(packageMap,0x318);
    // This is the same pure SupportsObject query already profiled by combat.
    // Passing the actual weak key avoids its optional identity lookup branch.
    // It neither allocates a GUID nor marks/exports an unreplicated actor.
    if(!original_players::Readable(guidCache,0x158)
       ||!engine::function<bool(*)(void*,void*,const Weak*)>(0x4782750)(guidCache,object,&identity))return BlockReference(field,object);
    if(original_players::IsA(object,0x3ed67a0)||original_players::IsA(object,0x64f4540))return BlockReference(field,object);
    return true;
}
inline bool NativeRadialHitReferences(const NativeDamageContext& c,const original_damage::NativeHitResult& h,void* connection){
    // FHitObjectHandle also carries an unreflected instance-manager weak ref.
    // GetActor alone can ignore it. Admit only the evidenced direct-actor form;
    // preserve every original byte, rejecting instance-manager variants.
    if(Read<int32_t>(h.bytes,0xc0)!=0||Read<int32_t>(h.bytes,0xc4)!=0
       ||Read<int32_t>(h.bytes,0xc8)!=-1||Read<uint32_t>(h.bytes,0xcc)!=0)
        return BlockReference("Radial.ComponentHits.instance_manager_handle_unsupported",nullptr);
    if(original_inventory_slots::ResolveIdentity(Read<Weak>(h.bytes,0xb8))!=c.victimPawn)
        return BlockReference("Radial.ComponentHits.direct_actor_weak_invalid",nullptr);
    auto actor=engine::function<void*(*)(const void*)>(0x437b290)(h.bytes+0xb8);
    const auto componentKey=Read<Weak>(h.bytes,0xd0);
    auto component=original_inventory_slots::ResolveIdentity(componentKey);
    if(actor!=c.victimPawn||!component||Read<void*>(component,0xc8)!=c.victimPawn)
        return BlockReference("Radial.ComponentHits.actual_victim_actor_or_component",component);
    const auto materialKey=Read<Weak>(h.bytes,0xb0);
    auto material=original_inventory_slots::ResolveIdentity(materialKey);
    if(!material&&((materialKey.index!=-1&&materialKey.index!=0)||materialKey.serial))return BlockReference("Radial.ComponentHits.stale_physical_material",nullptr);
    return NativeDamageReference(connection,actor,"Radial.ComponentHits.HitObjectHandle.Actor")
        &&NativeDamageReference(connection,component,"Radial.ComponentHits.Component")
        &&NativeDamageReference(connection,material,"Radial.ComponentHits.PhysMaterial");
}
inline bool NativeDamageGeometryReferences(const NativeDamageContext& c,const original_damage::NativeEventGeometry& g,void* connection){
    if(g.kind==original_damage::NativeEventKind::Base)return true;
    if(g.kind!=original_damage::NativeEventKind::Radial)return false;
    const auto& a=g.radial.componentHits;
    if(g.radial.table!=reinterpret_cast<void*>(engine::image+0xab31980)||g.radial.damageType!=c.damageType
       ||a.count<1||a.count>128||a.capacity<a.count
       ||!original_players::Readable(a.data,static_cast<size_t>(a.count)*sizeof(a.data[0]))
       ||!g.hit.component||Read<void*>(g.hit.component,0xc8)!=c.victimPawn)return false;
    if(!NativeDamageReference(connection,g.hit.component,"Radial.DamagedComponent"))return false;
    for(int32_t n=0;n<a.count;++n)if(!NativeRadialHitReferences(c,a.data[n],connection))return false;
    return true;
}
inline void* NativeDamageMulticastFunction(original_damage::NativeEventKind kind){
    using K=original_damage::NativeEventKind;
    const wchar_t* path=kind==K::Base?L"/Script/ShooterGame.DamageableComponent.MulticastNotifyDamage_Base":
        kind==K::Radial?L"/Script/ShooterGame.DamageableComponent.MulticastNotifyDamage_Radial":nullptr;
    if(!path)return nullptr;auto fn=engine::Find(path);
    const uint16_t bytes=kind==K::Base?0xc8:0x178;
    const uintptr_t exec=kind==K::Base?0x6705c20:0x67056a0;
    const bool valid=fn&&Read<uint16_t>(fn,0xbe)==bytes&&(Read<uint32_t>(fn,0xb8)&0x4440)==0x4440
        &&Read<uintptr_t>(fn,0xe0)==engine::image+exec
        &&Read<void*>(fn,0x28)==engine::Class(original_damage::DamageableClass);
    if(!valid){Fail(4666,"server_combat_native_event_typed_multicast_metadata_changed",true);return nullptr;}
    return fn;
}
inline bool PreflightAuthorityNativeEvent(const NativeDamageContext& c,const original_damage::NativeEventGeometry& g){
    ++state.typedPreflightQueries;
    if(!NativeDamageTuple(c)||!NativeDamageReferenceProfiles()||!original_damage::VerifyNativeEventResponseProfiles()||!NativeDamageMulticastFunction(g.kind)){
        ++state.typedPreflightRejected;return false;
    }
    const auto& target=original_damage::targets[c.victim];
    auto connection=Read<void*>(c.controller,0x510);
    if(!target.bound||target.pawn!=c.victimPawn||!target.damageable||!target.healthSection
       ||!NativeDamageReference(connection,c.causer,"Native.DamageCauser")
       ||!NativeDamageReference(connection,c.equippable,"Native.EquippableUsed")
       ||!NativeDamageReference(connection,c.damageType,"Native.DamageTypeClass")
       ||!NativeDamageReference(connection,c.sourcePawn,"Native.InstigatorPawn")
       ||!NativeDamageReference(connection,c.playerState,"Native.PlayerState")
       ||!NativeDamageReference(connection,target.healthSection,"Native.HealthSection")
       ||!NativeDamageReference(connection,target.shieldSection,"Native.ShieldSection")
       ||!NativeDamageGeometryReferences(c,g,connection)){
        ++state.typedPreflightRejected;return false;
    }
    return true;
}
inline bool NativeDamageResponseReferences(const NativeDamageContext& c,const original_damage::ResponseData& base,
                                          const original_damage::DeathData& death,const original_damage::Result& result){
    const auto& target=original_damage::targets[c.victim];
    if(base.instigator!=c.controller||base.instigatorPawn!=c.sourcePawn||base.damagerState!=c.playerState
       ||base.killCreditState!=c.playerState||base.causer!=c.causer||base.equippable!=c.equippable||base.damageType!=c.damageType
       ||(base.killed!=0)!=result.killed||base.timestamp.eventIndex!=target.lastNativeEventIndex
       ||base.changes.count<1||base.changes.count>2||base.changes.capacity<base.changes.count
       ||!original_players::Readable(base.changes.data,static_cast<size_t>(base.changes.count)*sizeof(original_damage::LifeChange)))return false;
    const auto& movement=base.timestamp.movement;
    if(original_inventory_slots::ResolveIdentity(Weak{movement.weakIndex,movement.weakSerial})!=c.victimPawn)return false;
    for(int32_t n=0;n<base.changes.count;++n){auto section=base.changes.data[n].section;
        if(section!=target.healthSection&&section!=target.shieldSection)return BlockReference("Native.LifeChangeEvents.ChangedComponent",section);
    }
    if(death.assists.count!=0||death.effectOverride||death.overrideContext
       ||(death.montage&&!original_players::IsA(death.montage,0x3f19390)))return false;
    return NativeDamageReference(Read<void*>(c.controller,0x510),death.montage,"Native.DeathAnimMontage");
}
inline bool RemoteNativeDamage(const NativeDamageContext& c,const original_damage::NativeEventGeometry& g,
                               const original_damage::Result& result,const original_damage::DeathData& originalDeath){
    if(!result.applied||result.duplicate)return true;
    const auto s=local_server_match::Player(c.source),v=local_server_match::Player(c.victim);
    if(!s||!v||s->epoch!=c.sourceEpoch||v->epoch!=c.victimEpoch||s->pawn!=c.sourcePawn||v->pawn!=c.victimPawn
       ||!policy::Authority(c.source,s->pawn,s->pc)||!policy::Authority(c.victim,v->pawn,v->pc)
       ||!result.originalResponse||!result.originalLifeReadback||!Live(c.causer,c.causerId)
       ||!Live(c.sourcePawn,c.sourceId)||!Live(c.controller,c.controllerId)||!Live(c.playerState,c.playerStateId)
       ||!Live(c.equippable,c.equippableId)||!Live(c.damageType,c.damageTypeId)||!Live(c.victimPawn,c.victimId))return false;
    auto fn=NativeDamageMulticastFunction(g.kind);if(!fn)return false;
    const auto& target=original_damage::targets[c.victim];
    BaseDamageMulticast base{};RadialDamageMulticast radial{};void* parameters=nullptr;
    auto death=originalDeath;if(result.killed)death.montage=target.lastMontage;
    if(g.kind==original_damage::NativeEventKind::Base){
        if(!original_damage::IsA(result.response,0x64f43e0))return false;
        base.base=Read<original_damage::ResponseData>(result.response,0x40);base.death=death;parameters=&base;
        if(!NativeDamageResponseReferences(c,base.base,death,result))return false;
    }else if(g.kind==original_damage::NativeEventKind::Radial){
        if(!original_damage::IsA(result.response,0x6501dc0))return false;
        radial.radial=Read<original_damage::RadialData>(result.response,0xd8);radial.death=death;parameters=&radial;
        auto geometry=g;geometry.radial=radial.radial.event;geometry.hit.component=radial.radial.hitComponent;
        if(!NativeDamageResponseReferences(c,radial.radial.base,death,result)
           ||!NativeDamageGeometryReferences(c,geometry,Read<void*>(c.controller,0x510)))return false;
    }else return false;
    auto table=Read<void**>(target.damageable,0);
    using Callspace=int(*)(void*,void*,void*);using Remote=bool(*)(void*,void*,void*,void*,void*);
    auto callspace=Read<Callspace>(table,0x2a0);auto remote=Read<Remote>(table,0x2a8);
    if(callspace!=engine::function<Callspace>(0x412c380)||remote!=engine::function<Remote>(0x412c9e0)
       ||!(callspace(target.damageable,fn,nullptr)&1))return false;
    ++state.damageRemoteCalls;++state.typedRemoteCalls;
    // Native local life was already notified exactly once by ApplyNativeEvent.
    // Invoke only the real remote half with its actual distinct wire type.
    if(!remote(target.damageable,fn,parameters,nullptr,nullptr))return false;
    ++state.damageRemoteAccepted;++state.typedRemoteAccepted;return true;
}
inline bool ApplyAuthorityNativeEvent(const NativeDamageContext& c,const original_damage::NativeEventGeometry& geometry,
    float acceptedNativeDamage,uint64_t token,original_damage::Result& result,const original_damage::DeathData* death=nullptr){
    result={};
    if(!Thread()||!local_server_match::pending.active||local_server_match::pending.token!=token
       ||local_server_match::pending.source!=c.source||local_server_match::pending.victim!=c.victim
       ||local_server_match::pending.equippable!=c.equippable||!PreflightAuthorityNativeEvent(c,geometry))return false;
    firearms::NativeLife before{};
    if(!firearms::ReadNativeLife(c.victim,before)||before.pending)return false;
    const bool okay=original_damage::ApplyNativeEvent(c.victim,c.sourcePawn,c.equippable,c.causer,c.damageType,
        geometry,acceptedNativeDamage,token,result,death);
    if(!okay)return false;
    if(result.applied&&!result.duplicate){
        const auto victimLife=policy::Player(c.victim);
        if(victimLife)humanlike_bot_native_events::StageDirection(token,static_cast<uint32_t>(c.victim),victimLife->spawnEpoch,
            {static_cast<float>(geometry.direction.x),static_cast<float>(geometry.direction.y),static_cast<float>(geometry.direction.z)});
        ++state.damageEvents;++state.typedDamageEvents;if(result.killed){++state.kills;++state.typedKills;}
        state.lastSource=static_cast<int>(c.source);state.lastVictim=static_cast<int>(c.victim);state.lastToken=token;
        state.lastDamage=acceptedNativeDamage;state.lastRegion=geometry.hit.region;state.lastDamageResponse=result.response;
        if(!firearms::RecordNativeLifeReceipt(c.victim,before)){
            Fail(4661,"server_combat_native_event_life_receipt_missing_after_real_apply",true);
            state.replicationFault=true;
        }
        const auto nativeDeath=death?*death:original_damage::DeathData{};
        if(!RemoteNativeDamage(c,geometry,result,nativeDeath)){
            ++state.damageRemoteRejected;++state.typedRemoteRejected;state.replicationFault=true;
            // Preserve successful host receipt for mandatory EndNativeDamage.
            // Stop the pair on the next boundary; do not replay this token.
        }
    }
    return true;
}
inline bool AbilityDamageCommitBoundary(){
    if(!Thread()||state.error||state.replicationFault||state.processing||local_server_match::pending.active
       ||firearms::diagnostics.processing||melee::diagnostics.processing||melee::activeScope
       ||firearms::observerDepth||melee::observerDepth
       ||firearms::readIndex!=firearms::writeIndex||melee::readIndex!=melee::writeIndex
       ||daggerRead!=daggerWrite||firearms::weaponUseRead!=firearms::weaponUseWrite
       ||firearms::diagnostics.effectOriginalForwardAttempts!=firearms::diagnostics.effectOriginalForwardCompleted
       ||firearms::diagnostics.geometryForwardAttempts!=firearms::diagnostics.geometryForwardCompleted)return false;
    for(const auto& b:fireBoundaries)if(b.active)return false;
    for(const auto& pending:melee::pendingWeaponUses)if(pending.pending)return false;
    return true;
}

inline bool InitializeImpl(void* world){
    if(state.initialized)return Thread()&&state.world==world;
    const auto& match=local_server_match::GetState();
    if(!original_players::Thread()||!match.initialized||match.world!=world||match.netMode<1||match.netMode>2
       ||Read<void*>(world,0x170)==nullptr||Read<uint8_t>(Read<void*>(world,0x178),0x224)!=3)
        return Fail(4586,"server_combat_requires_actual_shooter_authority_host");
    if(!VerifyProfiles())return Fail(4587,"server_combat_pinned_native_profile_mismatch");
    damageMulticastFunction=engine::Find(L"/Script/ShooterGame.DamageableComponent.MulticastNotifyDamage_Point");
    if(!damageMulticastFunction||Read<uint16_t>(damageMulticastFunction,0xbe)!=0x148
       ||(Read<uint32_t>(damageMulticastFunction,0xb8)&0x4440)!=0x4440
       ||Read<void*>(damageMulticastFunction,0x28)!=engine::Class(original_damage::DamageableClass))return Fail(4588,"server_combat_native_damage_multicast_abi_mismatch");
    if(!firearms::Initialize(world))return Fail(firearms::State().error,firearms::State().stage,firearms::State().restartRequired);
    state.world=world;state.gameState=Read<void*>(world,0x178);state.netMode=match.netMode;gameThread=GetCurrentThreadId();state.initialized=true;
    melee::Hooks callbacks;callbacks.allocateDamageToken=AllocateDamageToken;callbacks.beforeNativeDeath=BeforeNativeDeath;
    if(!melee::Initialize(world,callbacks))return Fail(melee::State().error,melee::State().stage,melee::State().restartRequired);
    local_server_abilities::SetNativePointSink(OnNativePoint);
    local_server_abilities::SetFireBoundarySink(OnFireBoundary);
    state.profilesVerified=true;state.rpcReferencesKnown=true;
    humanlike_bot_native_events::BeginSession();
    humanlike_bot_point_impacts::BeginSession();
    state.error=0;state.stage="server_combat_waiting_actual_authority_registrations";return true;
}
inline bool Initialize(void* world){__try{const bool okay=InitializeImpl(world);CacheDiagnostics();return okay;}__except(Capture(GetExceptionInformation())){return false;}}
inline bool PollImpl(double delta){
    if(!Thread()||!std::isfinite(delta)||delta<0||delta>1)return Fail(4589,"server_combat_invalid_game_thread_or_delta");
    const auto now=Now();if(state.lastPoll>=0&&now+.05<state.lastPoll)return Fail(4590,"server_combat_clock_reversed",true);
    state.lastPoll=now;++state.polls;
    if(!PublishAmmoPolicy())return false;
    for(size_t n=0;n<registrations.size();++n){auto& r=registrations[n];if(!r.present)continue;
        if(!CurrentRegistration(n))return Fail(4591,"server_combat_registered_authority_tuple_changed_before_retirement",true);
        if(r.refillOnBind&&firearms::sources[n].bound&&firearms::sources[n].alive&&!RefillAmmoImpl(n))return false;
    }
    if(!firearms::Poll(now))return Fail(firearms::State().error,firearms::State().stage,firearms::State().restartRequired);
    if(!melee::Poll(now))return Fail(melee::State().error,melee::State().stage,melee::State().restartRequired);
    if(!ProcessDaggers(now))return false;
    for(size_t n=0;n<daggerBaseline.size();++n)melee::ReadLife(n,daggerBaseline[n]);
    state.activeSources=firearms::State().aliveSources;state.physicsReadySources=firearms::State().physicsReadySources;
    state.nativeGunObserved=firearms::State().nativeRays>0;state.nativeMeleeObserved=melee::State().entries>0;
    if(state.replicationFault)return Fail(4592,"server_combat_native_damage_remote_transport_rejected_requires_review",true);
    const auto flags=local_server_match::Configuration().config.flags;
    const bool needsNPCDamage=local_server_match::FreeForAll()
        ||((flags&training_control::ReturnFire)!=0&&(flags&training_control::Invincible)==0);
    const auto human=local_server_match::Player(0);const auto driver=Read<void*>(state.world,0x40);
    const auto peers=Read<void**>(driver,0xc8);
    const bool singleOwner=human&&Read<int32_t>(driver,0xd0)==1&&Read<void*>(peers,0)==Read<void*>(human->pc,0x510);
    state.rpcReferenceGateReady=state.rpcReferencesKnown&&singleOwner&&(!needsNPCDamage||ReplicaPolicyAllowsNullableController());
    state.ready=firearms::State().ready&&state.ammoPolicyKnown&&state.profilesVerified&&state.rpcReferenceGateReady;
    state.configurationRevision=local_server_match::Configuration().config.revision;
    state.error=0;state.operation="none";
    state.stage=!state.rpcReferenceGateReady?"server_combat_foreign_controller_rpc_encoding_unresolved"
        :state.ready?"server_combat_authority_sources_ready":"server_combat_waiting_native_sources_or_physics";return true;
}
inline bool Poll(double delta){
    if(state.processing)return Fail(4593,"server_combat_reentrant_poll_rejected");state.processing=true;bool okay=false;
    __try{okay=PollImpl(delta);}__except(Capture(GetExceptionInformation())){okay=false;}
    state.processing=false;
    __try{CacheDiagnostics();}__except(Capture(GetExceptionInformation())){okay=false;}
    return okay;
}
inline size_t RegistryActors(void** output,size_t capacity){
    size_t count=0;
    const auto add=[&](void* object){if(!object)return;if(output&&count<capacity)output[count]=object;++count;};
    for(size_t n=0;n<registrations.size();++n){const auto& r=registrations[n];if(!r.present)continue;
        if(Live(r.pawn,r.pawnId))add(r.pawn);
        const auto& s=firearms::sources[n];if(s.bound&&Live(s.weapon,s.weaponId))add(s.weapon);
    }return count;
}
inline size_t RegistryObjects(void** output,size_t capacity){
    size_t count=0;const auto add=[&](void* object){if(!object)return;if(output&&count<capacity)output[count]=object;++count;};
    for(size_t n=0;n<registrations.size();++n){const auto& r=registrations[n];if(!r.present)continue;
        const auto& d=original_damage::targets[n];
        if(d.bound&&d.pawn==r.pawn){add(d.damageable);add(d.healthSection);add(d.shieldSection);add(d.death);}
        if(Live(r.ammoReplenisher,r.replenisherId))add(r.ammoReplenisher);
    }return count;
}
inline constexpr size_t InspectionMachines=8,InspectionKeys=16,InspectionFirings=4,InspectionEffects=4;
struct MachineInspection {
    void *object=nullptr,*owner=nullptr,*starting=nullptr,*authority=nullptr,*predicted=nullptr,*selected=nullptr,*selectedClass=nullptr;
    bool typed=false,enabled=false,ownerMatches=false,selectedOwnerMatches=false,selectedMachineMatches=false;
    uint8_t ownerRole=255,firstInput=255,firstAction=255,firstActivation=255;
    int32_t pending=0;float subframe=0,firstTimestamp=0;
    uint32_t selectedNameRaw[3]{},selectedClassNameRaw[3]{};
    char name[128]{},selectedName[128]{},selectedClassName[128]{};
};
struct RegistryKeyInspection {void* type=nullptr;bool live=false,nativeContains=false;int32_t events=0;char name[128]{};};
struct EffectInspection {
    void* object=nullptr;bool typed=false,ownerMatches=false,observerInstalled=false;
    int32_t nativeShot=0,directions=0,capacity=0,loopCount=0;uint8_t active=0;
};
struct FiringInspection {
    void *object=nullptr,*resource=nullptr,*shotDelegateData=nullptr;
    bool live=false,queriesKnown=false,effectArrayValid=false;uint8_t canFire0=255,canFire1=255;
    uint8_t triggerPressedMask=0,releaseMask=0;
    int32_t shotIndex=0,effects=0,shotDelegateKind=0,ammoPredicted=-1,ammoAuthority=-1;
    uint64_t origins=0,rays=0,effectHooks=0;char name[128]{};
    bool geometryObserver=false,geometryFunctionKnown=false;uint32_t geometryFlags=0;
    uint64_t geometryCaptures=0;int32_t geometryShot=-1,geometryCount=0;
    std::array<EffectInspection,InspectionEffects> effect{};size_t effectsCaptured=0;
};
struct WeaponInspection {
    uint64_t binding=0,sampledAt=0,samples=0;void* weapon=nullptr;
    bool valid=false,inputBoundReady=false,inputCallbackOriginal=false,inputTargetMatches=false;
    int32_t inputStorageKind=0;void *inputDelegate=nullptr,*inputTarget=nullptr;uintptr_t inputCallbackRva=0;
    uint8_t nativeHeld[3]{},weaponTickState=0,weaponTickContents=0;
    uint64_t sampledAppliedChanges=0,sampledHeldChanges[3]{},sampledDown[3]{};int32_t sampledApplied=-1;
    void *eventComponent=nullptr,*registry=nullptr,*registryOwner=nullptr;
    bool eventEnabled=false,registryGetterMatches=false,registryLayoutValid=false;
    int32_t registrySlots=0,registryFree=0,registryCount=0,machineCount=0,firingCount=0;
    size_t keysCaptured=0,machinesCaptured=0,firingsCaptured=0;
    std::array<RegistryKeyInspection,InspectionKeys> keys{};
    std::array<MachineInspection,InspectionMachines> machines{};
    std::array<FiringInspection,InspectionFirings> firings{};
};
inline std::array<WeaponInspection,MaxPlayers> weaponInspections{};
inline void *inspectionCanFire=nullptr,*inspectionTrigger=nullptr,*inspectionRelease=nullptr;
inline bool InspectionQuery(void*& fn,const wchar_t* path,uint16_t bytes,uintptr_t exec){
    if(!fn)fn=engine::Find(path);
    return fn&&Read<uint16_t>(fn,0xbe)==bytes&&Read<uint32_t>(fn,0xb8)==0x54020401
        &&Read<void*>(fn,0xe0)==engine::function<void*>(exec)&&Read<void*>(fn,0x28)==engine::Class(0x684d4c0);
}
inline void InspectMachine(WeaponInspection& d,void* object,void* weapon){
    if(!object||d.machinesCaptured>=d.machines.size())return;
    for(size_t n=0;n<d.machinesCaptured;++n)if(d.machines[n].object==object)return;
    auto& m=d.machines[d.machinesCaptured++];
    const bool changed=m.object!=object;if(changed){m={};m.object=object;}
    m.typed=original_players::IsA(object,0x67722c0);if(!m.typed)return;
    if(changed)original_weapon::OriginalObjectName(object,m.name);
    m.owner=Read<void*>(object,0xc8);m.ownerMatches=m.owner==weapon;m.ownerRole=Read<uint8_t>(m.owner,0x224);
    m.enabled=Read<uint8_t>(object,0x348)!=0;m.starting=Read<void*>(object,0x1e8);
    m.authority=Read<void*>(object,0x1c8);m.predicted=Read<void*>(object,0x1b0);
    auto selected=m.ownerRole==3?m.authority:m.predicted;Weak selectedId{};const bool selectedLive=Identity(selected,selectedId);
    if(m.selected!=selected){m.selected=selected;m.selectedName[0]=0;if(selectedLive)original_weapon::OriginalObjectName(selected,m.selectedName);}
    auto type=selectedLive?Read<void*>(selected,0x10):nullptr;
    if(m.selectedClass!=type){m.selectedClass=type;original_weapon::OriginalObjectName(type,m.selectedClassName);}
    m.selectedOwnerMatches=m.selectedMachineMatches=false;
    if(selectedLive){m.selectedOwnerMatches=Read<void*>(selected,0xc8)==weapon;m.selectedMachineMatches=Read<void*>(selected,0xe0)==object;
        std::memcpy(m.selectedNameRaw,static_cast<uint8_t*>(selected)+0x18,12);}
    if(type)std::memcpy(m.selectedClassNameRaw,static_cast<uint8_t*>(type)+0x18,12);
    m.subframe=Read<float>(object,0x1e0);m.pending=Read<int32_t>(object,0x388);
    const auto input=Read<uint8_t*>(object,0x380);const auto capacity=Read<int32_t>(object,0x38c);
    m.firstInput=m.firstAction=m.firstActivation=255;m.firstTimestamp=0;
    if(m.pending>0&&m.pending<=1024&&capacity>=m.pending&&original_players::Readable(input,8)){
        m.firstInput=input[0];m.firstAction=input[1];m.firstActivation=input[2];m.firstTimestamp=Read<float>(input,4);
    }
}
inline void CaptureWeaponInspection(const firearms::Source& s,WeaponInspection& d){
    const auto now=GetTickCount64();
    if(!s.bound||!firearms::SourceIdentity(s)){d.valid=false;return;}
    if(d.binding==s.binding&&d.weapon==s.weapon&&d.sampledAt&&now-d.sampledAt<100)return;
    if(d.binding!=s.binding||d.weapon!=s.weapon){d={};d.binding=s.binding;d.weapon=s.weapon;}
    d.sampledAt=now;++d.samples;d.valid=true;
    d.inputStorageKind=Read<int32_t>(s.inputState,0x160);d.inputDelegate=Read<void*>(s.inputState,0x158);
    d.inputTarget=nullptr;d.inputCallbackRva=0;d.inputBoundReady=d.inputCallbackOriginal=d.inputTargetMatches=false;
    const auto applied=Read<int32_t>(s.inputState,0x120c);if(applied!=d.sampledApplied){++d.sampledAppliedChanges;d.sampledApplied=applied;}
    for(size_t n=0;n<3;++n){const auto held=Read<uint8_t>(s.inputState,0x265+n);if(held!=d.nativeHeld[n])++d.sampledHeldChanges[n];if(held)++d.sampledDown[n];d.nativeHeld[n]=held;}
    if(d.inputStorageKind==3&&original_players::Readable(d.inputDelegate,0x28)){
        const auto callback=reinterpret_cast<uintptr_t>(Read<void*>(d.inputDelegate,0x20));
        d.inputCallbackRva=callback>=engine::image?callback-engine::image:callback;
        d.inputCallbackOriginal=callback==engine::image+0x70fbcc0;
        d.inputTarget=engine::function<void*(*)(void*)>(0x1d4cfa0)(static_cast<uint8_t*>(d.inputDelegate)+0x18);
        d.inputTargetMatches=d.inputTarget==s.inventory;d.inputBoundReady=d.inputCallbackOriginal&&d.inputTargetMatches;
    }
    d.weaponTickState=Read<uint8_t>(s.weapon,0x4f0);d.weaponTickContents=Read<uint8_t>(s.weapon,0x4f1);
    d.machinesCaptured=0;InspectMachine(d,Read<void*>(s.weapon,0xe00),s.weapon);
    const auto machines=Read<void**>(s.weapon,0xe30);d.machineCount=Read<int32_t>(s.weapon,0xe38);
    if(d.machineCount>=0&&d.machineCount<=64&&Read<int32_t>(s.weapon,0xe3c)>=d.machineCount
       &&(!d.machineCount||original_players::Readable(machines,static_cast<size_t>(d.machineCount)*8)))
        for(int32_t n=0;n<d.machineCount;++n)InspectMachine(d,machines[n],s.weapon);
    d.eventComponent=Read<void*>(s.weapon,0xe18);d.registry=Read<void*>(d.eventComponent,0x198);
    d.eventEnabled=original_players::IsA(d.eventComponent,0x6768f30)&&Read<void*>(d.eventComponent,0xc8)==s.weapon&&Read<uint8_t>(d.eventComponent,0x189)!=0;
    // Equipment already admitted the registry through the original getter.
    // Its native pointer-protection helper advances global bookkeeping; this
    // read-only inspector samples the existing component field instead.
    d.registryGetterMatches=false;
    d.registryOwner=Read<void*>(d.registry,0xc8);d.registryLayoutValid=false;d.keysCaptured=0;
    if(d.eventEnabled&&d.registry&&d.registryOwner==s.weapon){
        const auto entries=Read<uint8_t*>(d.registry,0x138);d.registrySlots=Read<int32_t>(d.registry,0x140);
        const auto capacity=Read<int32_t>(d.registry,0x144),bits=Read<int32_t>(d.registry,0x160);d.registryFree=Read<int32_t>(d.registry,0x16c);
        const auto bitmapPointer=Read<uint32_t*>(d.registry,0x158);
        const auto bitmap=bitmapPointer?bitmapPointer:reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(d.registry)+0x148);
        d.registryLayoutValid=d.registrySlots>=0&&d.registrySlots<=1024&&capacity>=d.registrySlots&&bits>=d.registrySlots
            &&d.registryFree>=0&&d.registryFree<=d.registrySlots
            &&(!d.registrySlots||(original_players::Readable(entries,static_cast<size_t>(d.registrySlots)*32)
                &&original_players::Readable(bitmap,static_cast<size_t>((d.registrySlots+31)/32)*4)));
        d.registryCount=d.registrySlots-d.registryFree;
        if(d.registryLayoutValid)for(int32_t n=0;n<d.registrySlots&&d.keysCaptured<d.keys.size();++n){
            if(!(bitmap[n/32]&(uint32_t(1)<<(n%32))))continue;
            auto& key=d.keys[d.keysCaptured++];const auto type=Read<void*>(entries,static_cast<size_t>(n)*32);
            Weak typeId{};key.live=Identity(type,typeId);
            if(key.type!=type){key.type=type;key.name[0]=0;if(key.live)original_weapon::OriginalObjectName(type,key.name);}
            key.events=Read<int32_t>(entries,static_cast<size_t>(n)*32+16);
            key.nativeContains=key.live&&engine::function<bool(*)(void*,void*)>(0x6ff68d0)(d.registry,type);
        }
    }
    const bool queryABI=InspectionQuery(inspectionCanFire,L"/Script/ShooterGame.FiringStateComponent.CanFire",2,0x6850340)
        &&InspectionQuery(inspectionTrigger,L"/Script/ShooterGame.FiringStateComponent.IsTriggerInput",3,0x68506a0)
        &&InspectionQuery(inspectionRelease,L"/Script/ShooterGame.FiringStateComponent.IsReleaseInput",3,0x68504c0);
    d.firingCount=static_cast<int32_t>(s.firingCount);d.firingsCaptured=(std::min)(s.firingCount,d.firings.size());
    for(size_t n=0;n<d.firingsCaptured;++n){const auto& source=s.firings[n];auto& f=d.firings[n];
        if(f.object!=source.component){f={};f.object=source.component;original_weapon::OriginalObjectName(f.object,f.name);}
        f.live=Live(f.object,source.identity);if(!f.live)continue;
        f.origins=source.originsObserved;f.rays=source.raysObserved;f.effectHooks=source.effectHooks;
        f.geometryObserver=source.table.installed&&Read<void*>(f.object,0)==source.table.Address();
        f.geometryFunctionKnown=Live(source.geometryFunction,source.geometryFunctionId);
        f.geometryFlags=source.geometryFunctionFlags;f.geometryCaptures=source.geometryCaptures;
        f.geometryShot=source.lastGeometryShot;f.geometryCount=source.lastGeometryCount;
        f.shotIndex=Read<int32_t>(f.object,0x4a8);f.resource=source.resource;
        f.ammoPredicted=Read<int32_t>(f.resource,0x10c);f.ammoAuthority=Read<int32_t>(f.resource,0x110);
        f.shotDelegateData=Read<void*>(f.object,0x3e8);f.shotDelegateKind=Read<int32_t>(f.object,0x3f0);
        f.queriesKnown=queryABI;f.canFire0=f.canFire1=255;f.triggerPressedMask=f.releaseMask=0;
        if(queryABI){
            struct CanFire {uint8_t option=0,result=0;} query;
            static_assert(sizeof(CanFire)==2);
            if(original_damage::EventWithFunction(f.object,L"/Script/ShooterGame.FiringStateComponent.CanFire",inspectionCanFire,2,&query))f.canFire0=query.result;else f.queriesKnown=false;
            query={1,0};if(original_damage::EventWithFunction(f.object,L"/Script/ShooterGame.FiringStateComponent.CanFire",inspectionCanFire,2,&query))f.canFire1=query.result;else f.queriesKnown=false;
            for(uint8_t input=0;input<3;++input){struct Input {uint8_t input,action,result;} parameters{input,0,0};static_assert(sizeof(Input)==3);
                if(original_damage::EventWithFunction(f.object,L"/Script/ShooterGame.FiringStateComponent.IsTriggerInput",inspectionTrigger,3,&parameters)){if(parameters.result)f.triggerPressedMask|=static_cast<uint8_t>(1U<<input);}else f.queriesKnown=false;
                parameters={input,2,0};if(original_damage::EventWithFunction(f.object,L"/Script/ShooterGame.FiringStateComponent.IsReleaseInput",inspectionRelease,3,&parameters)){if(parameters.result)f.releaseMask|=static_cast<uint8_t>(1U<<input);}else f.queriesKnown=false;
            }
        }
        const auto effects=Read<void**>(f.object,0x500);f.effects=Read<int32_t>(f.object,0x508);const auto capacity=Read<int32_t>(f.object,0x50c);
        f.effectsCaptured=0;f.effectArrayValid=f.effects>=0&&f.effects<=128&&capacity>=f.effects
            &&(!f.effects||original_players::Readable(effects,static_cast<size_t>(f.effects)*8));
        if(f.effectArrayValid)for(int32_t e=0;e<f.effects&&f.effectsCaptured<f.effect.size();++e){auto& effect=f.effect[f.effectsCaptured++];effect={};effect.object=effects[e];
            effect.typed=original_players::IsA(effect.object,0x684d570);if(!effect.typed)continue;
            effect.ownerMatches=Read<void*>(effect.object,0x148)==f.object;effect.nativeShot=Read<int32_t>(effect.object,0x160);
            effect.directions=Read<int32_t>(effect.object,0x158);effect.capacity=Read<int32_t>(effect.object,0x15c);effect.loopCount=Read<int32_t>(effect.object,0x164);effect.active=Read<uint8_t>(effect.object,0x168);
            const firearms::Effect* observer=nullptr;
            for(auto candidate:s.effects)if(candidate&&firearms::OwnsEffect(s,*candidate)&&candidate->object==effect.object){observer=candidate;break;}
            effect.observerInstalled=observer&&observer->table.installed&&Read<void*>(effect.object,0)==observer->table.Address();
        }
    }
}
struct DiagnosticSource {
    void* weapon=nullptr;
    void *nativeInventoryCurrent=nullptr,*nativeInputCurrent=nullptr,*nativeInputDeclared=nullptr,*nativeCorrected=nullptr;
    bool bound=false,observing=false,physicsReady=false,inputReady=false,current=false,externalInputProducer=false;
    bool localInputComponent=false,externalPending=false,externalNativeStateMatches=false;
    bool externalCanSubmit=false,externalNativeAppliedReady=false,externalNativeAimMatches=false,contextMatches=false;
    bool nativeAimValid=false,initialCorrectionPublished=false,initialCorrectionProcessed=false;
    uint32_t held=0,externalObservedHeld=0;
    uint64_t originsObserved=0,raysObserved=0,inputEdges=0,inputSubmissions=0,viewSubmissions=0;
    uint64_t externalBatches=0,externalAppliedBatches=0,externalCancelledBatches=0,externalResets=0,externalDeduplicated=0;
    int32_t historyCapacity=0,pendingInput=0,triggerDelegates=0,ammoCount=-1,lastFiringSlot=-1;
    int32_t externalLastEvent=-1,externalObservedEvent=-1;
    int32_t inputCorrection=-1,authorityCorrection=-1,correctionOrigin=-1,incomingBase=-1;
    uint8_t correctionState=255;
    engine::Vec nativeAim{},submittedAim{};
    float externalMovementTime=-1,externalDeadline=-1;
    WeaponInspection inspection;
};
struct DiagnosticPlayer {
    Registration registration;DiagnosticSource source;
    bool actualAuthority=false,alive=false;float health=0,rawShield=0;
};
struct DiagnosticSnapshot {
    State summary;firearms::Diagnostics guns;melee::Diagnostics knife;
    firearms::RayDiagnostics rayTerminals;
    bool valid=false,policyCurrent=false;uint64_t sampledAt=0;
    std::array<DiagnosticPlayer,MaxPlayers> players{};
};
inline SRWLOCK diagnosticLock=SRWLOCK_INIT;
inline DiagnosticSnapshot diagnosticSnapshot;
inline DiagnosticSnapshot diagnosticScratch;
static_assert(sizeof(DiagnosticSnapshot)<320U*1024U);
inline void CacheDiagnostics(){
    if(!state.initialized||GetCurrentThreadId()!=gameThread)return;
    armor_poll_performance::Scope measurementCombatDiagnosticRefresh(armor_poll_performance::Slot::CombatDiagnosticRefresh);
    auto& observed=diagnosticScratch;
    for(auto& player:observed.players)player.registration.present=false;
    observed.summary=state;observed.guns=firearms::State();observed.knife=melee::State();
    observed.rayTerminals=firearms::rayDiagnostics;
    for(size_t n=0;n<firearms::sources.size();++n){
        observed.rayTerminals.sources[n].damageEvents=firearms::sources[n].damageEvents;
        observed.rayTerminals.sources[n].kills=firearms::sources[n].kills;}
    observed.valid=true;observed.sampledAt=GetTickCount64();observed.policyCurrent=ReplicaPolicyAllowsNullableController();
    for(size_t n=0;n<registrations.size();++n){const auto& r=registrations[n];if(!r.present)continue;
        auto& p=observed.players[n];p.registration=r;p.actualAuthority=CurrentRegistration(n);
        const auto& target=original_damage::targets[n];p.alive=target.alive;p.health=target.health;p.rawShield=target.rawShield;
        const auto& s=firearms::sources[n];auto& output=p.source;
        CaptureWeaponInspection(s,weaponInspections[n]);output.inspection=weaponInspections[n];
        output.weapon=s.weapon;output.bound=s.bound;output.observing=s.observing;output.physicsReady=s.physicsReady;output.inputReady=s.inputReady;
        output.current=s.current;output.externalInputProducer=s.externalInputProducer;output.localInputComponent=s.input!=nullptr;
        output.externalPending=s.externalPending;output.externalNativeStateMatches=s.externalNativeStateMatches;
        output.externalCanSubmit=s.externalCanSubmit;output.externalNativeAppliedReady=s.externalNativeAppliedReady;
        output.externalNativeAimMatches=s.externalNativeAimMatches;output.submittedAim=s.lastView;
        output.held=s.held;output.externalObservedHeld=s.externalObservedHeld;
        output.originsObserved=s.originsObserved;output.raysObserved=s.raysObserved;output.inputEdges=s.inputEdges;
        output.inputSubmissions=s.inputSubmissions;output.viewSubmissions=s.viewSubmissions;
        output.externalBatches=s.externalBatches;output.externalAppliedBatches=s.externalAppliedBatches;
        output.externalCancelledBatches=s.externalCancelledBatches;output.externalResets=s.externalResets;
        output.externalDeduplicated=s.externalDeduplicated;
        output.ammoCount=s.ammoCount;output.lastFiringSlot=s.lastFiringSlot;
        output.externalLastEvent=s.externalLastEvent;output.externalObservedEvent=s.externalObservedEvent;
        output.externalMovementTime=s.externalMovementTime;output.externalDeadline=s.externalDeadline;
        if(s.bound&&Live(s.inputState,s.inputStateId)){
            output.historyCapacity=Read<int32_t>(s.inputState,0x11c);output.pendingInput=Read<int32_t>(s.inputState,0xaa0);
            output.triggerDelegates=Read<int32_t>(s.inputState,0x160);
            output.inputCorrection=Read<int32_t>(s.inputState,0x260);output.incomingBase=Read<int32_t>(s.inputState,0x364);
            output.nativeInputCurrent=Read<void*>(s.inputState,0x250);output.nativeInputDeclared=Read<void*>(s.inputState,0x258);
            output.nativeAim=Read<engine::Vec>(s.inputState,0x238);output.nativeAimValid=Read<uint8_t>(s.inputState,0x128)!=0;
            output.authorityCorrection=Read<int32_t>(s.inventory,0x328);output.correctionOrigin=Read<int32_t>(s.inventory,0x32c);
            output.nativeInventoryCurrent=Read<void*>(s.inventory,0x280);output.nativeCorrected=Read<void*>(s.inventory,0x320);
            output.initialCorrectionPublished=Read<uint8_t>(s.inventory,0x30c)!=0;output.initialCorrectionProcessed=Read<uint8_t>(s.inventory,0x30d)!=0;
            output.correctionState=Read<uint8_t>(s.inventory,0x2fc);output.contextMatches=firearms::ExternalContextMatches(s);
        }
    }
    AcquireSRWLockExclusive(&diagnosticLock);diagnosticSnapshot=observed;ReleaseSRWLockExclusive(&diagnosticLock);
}
inline DiagnosticSnapshot DiagnosticsSnapshot(){
    DiagnosticSnapshot observed;AcquireSRWLockShared(&diagnosticLock);observed=diagnosticSnapshot;ReleaseSRWLockShared(&diagnosticLock);return observed;
}

// Only TickHost's consecutive current-equippable checks use this optional
// publisher. Every call still executes the entire original BindWeaponImpl.
// The default BindWeapon and all other diagnostic publishers remain unchanged.
inline bool hostTickBindCallActive=false,hostTickBindReentered=false;
inline bool HostTickBindInspectionFresh(bool verifyIdentity=false){
    for(size_t n=0;n<registrations.size();++n)if(registrations[n].present){
        const auto now=GetTickCount64();
        const auto& s=firearms::sources[n];const auto& d=weaponInspections[n];
        // Deep inspection invokes original getters. Only merge publications
        // while all existing inspections would use their original 100ms gate.
        // Unknown, changed, invalid, unbound, due or clock-wrap cases refresh.
        if(!s.bound||!d.valid||d.binding!=s.binding||d.weapon!=s.weapon
           ||!d.sampledAt||now<d.sampledAt||now-d.sampledAt>=100
           ||(verifyIdentity&&!firearms::SourceIdentity(s)))return false;
    }
    return true;
}
inline bool HostTickBindSameSource(size_t index,void* weapon){
    if(index>=registrations.size()||index>=firearms::sources.size())return false;
    const auto& r=registrations[index];const auto& s=firearms::sources[index];
    return weapon&&r.present&&s.bound&&s.weapon==weapon&&s.pawn==r.pawn&&s.spawnEpoch==r.epoch;
}
inline bool BindWeaponForHostTick(size_t index,void* weapon,bool& diagnosticsPending){
    __try{
        // Off-thread and recursive calls retain the original immediate path.
        // No private coalescing state is accessed before the normal GT gate.
        if(!Thread())return BindWeapon(index,weapon);
        if(hostTickBindCallActive){hostTickBindReentered=true;return BindWeapon(index,weapon);}
        hostTickBindCallActive=true;hostTickBindReentered=false;
        __try{
            const auto binds=state.weaponBinds;
            const auto binding=index<firearms::sources.size()?firearms::sources[index].binding:0;
            const bool steady=binds!=UINT64_MAX&&HostTickBindSameSource(index,weapon)&&HostTickBindInspectionFresh();
            const bool okay=BindWeaponImpl(index,weapon);
            if(okay&&steady&&!hostTickBindReentered&&state.weaponBinds==binds
               &&HostTickBindSameSource(index,weapon)&&firearms::sources[index].binding==binding
               &&HostTickBindInspectionFresh(true)){
                diagnosticsPending=true;return true;
            }
            // Includes false returns, true weapon/life changes, counter limits,
            // recursive callbacks, and any due original inspection.
            CacheDiagnostics();diagnosticsPending=false;return okay;
        }__finally{hostTickBindCallActive=false;}
    }__except(Capture(GetExceptionInformation())){
        // As in BindWeapon, an SEH exception is terminal, never a successful
        // diagnostic refresh or binding. __finally releases the private guard.
        diagnosticsPending=false;return false;
    }
}
inline bool FinishHostTickBindDiagnostics(bool& diagnosticsPending){
    if(!diagnosticsPending)return true;
    __try{
        if(!Thread()){CacheDiagnostics();diagnosticsPending=false;return false;}
        const bool recursive=hostTickBindCallActive;
        if(recursive)hostTickBindReentered=true;
        hostTickBindCallActive=true;
        __try{CacheDiagnostics();diagnosticsPending=false;return true;}
        __finally{hostTickBindCallActive=recursive;}
    }
    __except(Capture(GetExceptionInformation())){diagnosticsPending=false;return false;}
}

inline void WriteInspectionText(FILE* file,const char* value){
    std::fputc('"',file);if(value)for(const unsigned char* p=reinterpret_cast<const unsigned char*>(value);*p;++p){
        if(*p=='"'||*p=='\\'){std::fputc('\\',file);std::fputc(*p,file);}
        else if(*p<0x20)std::fprintf(file,"\\u%04x",static_cast<unsigned>(*p));else std::fputc(*p,file);
    }std::fputc('"',file);
}
inline void WriteInspectionNumber(FILE* file,double value){if(std::isfinite(value))std::fprintf(file,"%.17g",value);else std::fputs("null",file);}
inline void WriteWeaponInspection(FILE* file,const WeaponInspection& d){
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(file,",\"weapon_inspection\":{\"valid\":%s,\"sampled_at_ms\":%llu,\"samples\":%llu,\"binding\":%llu,\"primary_input_id\":0,\"secondary_input_id\":1,\"reload_input_id\":2,\"input_storage_kind_160\":%d,\"input_delegate_158\":\"%p\",\"input_callback_rva\":%llu,\"input_callback_original\":%s,\"input_target\":\"%p\",\"input_target_matches_inventory\":%s,\"input_bound_ready\":%s,\"native_held\":[%u,%u,%u],\"weapon_tick_state_4f0\":%u,\"weapon_tick_contents_4f1\":%u,\"machines_native_count\":%d,\"machines_captured\":%zu,\"machines\":[",
        b(d.valid),static_cast<unsigned long long>(d.sampledAt),static_cast<unsigned long long>(d.samples),static_cast<unsigned long long>(d.binding),d.inputStorageKind,d.inputDelegate,
        static_cast<unsigned long long>(d.inputCallbackRva),b(d.inputCallbackOriginal),d.inputTarget,b(d.inputTargetMatches),b(d.inputBoundReady),
        unsigned(d.nativeHeld[0]),unsigned(d.nativeHeld[1]),unsigned(d.nativeHeld[2]),unsigned(d.weaponTickState),unsigned(d.weaponTickContents),d.machineCount,d.machinesCaptured);
    for(size_t n=0;n<d.machinesCaptured;++n){const auto& m=d.machines[n];
        std::fprintf(file,"%s{\"object\":\"%p\",\"name\":",n?",":"",m.object);WriteInspectionText(file,m.name);
        std::fprintf(file,",\"typed\":%s,\"enabled\":%s,\"owner\":\"%p\",\"owner_matches\":%s,\"owner_role\":%u,\"starting\":\"%p\",\"authority\":\"%p\",\"predicted\":\"%p\",\"selected\":\"%p\",\"selected_class\":\"%p\",\"selected_name\":",
            b(m.typed),b(m.enabled),m.owner,b(m.ownerMatches),unsigned(m.ownerRole),m.starting,m.authority,m.predicted,m.selected,m.selectedClass);
        WriteInspectionText(file,m.selectedName);std::fputs(",\"selected_class_name\":",file);WriteInspectionText(file,m.selectedClassName);
        std::fprintf(file,",\"selected_name_raw\":[%u,%u,%u],\"selected_class_name_raw\":[%u,%u,%u],\"selected_owner_matches\":%s,\"selected_machine_matches\":%s,\"subframe\":",
            m.selectedNameRaw[0],m.selectedNameRaw[1],m.selectedNameRaw[2],m.selectedClassNameRaw[0],m.selectedClassNameRaw[1],m.selectedClassNameRaw[2],b(m.selectedOwnerMatches),b(m.selectedMachineMatches));
        WriteInspectionNumber(file,m.subframe);
        std::fprintf(file,",\"pending_input\":%d,\"first_input\":%u,\"first_action\":%u,\"first_activation\":%u,\"first_timestamp\":",m.pending,unsigned(m.firstInput),unsigned(m.firstAction),unsigned(m.firstActivation));
        WriteInspectionNumber(file,m.firstTimestamp);std::fputc('}',file);
    }
    std::fprintf(file,"],\"sampled_applied_event\":%d,\"sampled_applied_event_changes\":%llu,\"sampled_held_changes\":[%llu,%llu,%llu],\"sampled_down_observations\":[%llu,%llu,%llu],\"event_component\":\"%p\",\"event_enabled\":%s,\"registry\":\"%p\",\"registry_owner\":\"%p\",\"registry_getter_queried\":false,\"registry_getter_matches\":%s,\"registry_layout_valid\":%s,\"registry_slots\":%d,\"registry_free\":%d,\"registry_key_count\":%d,\"registry_keys_captured\":%zu,\"registry_keys\":[",
        d.sampledApplied,static_cast<unsigned long long>(d.sampledAppliedChanges),static_cast<unsigned long long>(d.sampledHeldChanges[0]),static_cast<unsigned long long>(d.sampledHeldChanges[1]),static_cast<unsigned long long>(d.sampledHeldChanges[2]),
        static_cast<unsigned long long>(d.sampledDown[0]),static_cast<unsigned long long>(d.sampledDown[1]),static_cast<unsigned long long>(d.sampledDown[2]),
        d.eventComponent,b(d.eventEnabled),d.registry,d.registryOwner,b(d.registryGetterMatches),b(d.registryLayoutValid),d.registrySlots,d.registryFree,d.registryCount,d.keysCaptured);
    for(size_t n=0;n<d.keysCaptured;++n){const auto& key=d.keys[n];
        std::fprintf(file,"%s{\"type\":\"%p\",\"name\":",n?",":"",key.type);WriteInspectionText(file,key.name);
        std::fprintf(file,",\"live\":%s,\"native_contains\":%s,\"queued_event_objects\":%d}",b(key.live),b(key.nativeContains),key.events);
    }
    std::fprintf(file,"],\"firing_native_count\":%d,\"firing_captured\":%zu,\"firings\":[",d.firingCount,d.firingsCaptured);
    for(size_t n=0;n<d.firingsCaptured;++n){const auto& f=d.firings[n];
        std::fprintf(file,"%s{\"object\":\"%p\",\"name\":",n?",":"",f.object);WriteInspectionText(file,f.name);
        std::fprintf(file,",\"native_geometry_observer\":%s,\"native_geometry_function_known\":%s,\"native_geometry_function_flags\":%u,\"native_geometry_captures\":%llu,\"native_geometry_last_shot\":%d,\"native_geometry_last_count\":%d",
            b(f.geometryObserver),b(f.geometryFunctionKnown),f.geometryFlags,static_cast<unsigned long long>(f.geometryCaptures),f.geometryShot,f.geometryCount);
        std::fprintf(file,",\"live\":%s,\"native_queries_known\":%s,\"can_fire_option0\":%u,\"can_fire_option1\":%u,\"trigger_pressed_input_mask\":%u,\"release_input_mask\":%u,\"shot_index_4a8\":%d,\"resource\":\"%p\",\"resource_predicted_10c\":%d,\"resource_authority_110\":%d,\"origins\":%llu,\"rays\":%llu,\"effect_hooks\":%llu,\"on_shot_delegate_data_3e8\":\"%p\",\"on_shot_delegate_storage_3f0\":%d,\"active_effects_508\":%d,\"active_effect_array_valid\":%s,\"effects_captured\":%zu,\"effects\":[",
            b(f.live),b(f.queriesKnown),unsigned(f.canFire0),unsigned(f.canFire1),unsigned(f.triggerPressedMask),unsigned(f.releaseMask),f.shotIndex,f.resource,f.ammoPredicted,f.ammoAuthority,
            static_cast<unsigned long long>(f.origins),static_cast<unsigned long long>(f.rays),static_cast<unsigned long long>(f.effectHooks),f.shotDelegateData,f.shotDelegateKind,f.effects,b(f.effectArrayValid),f.effectsCaptured);
        for(size_t e=0;e<f.effectsCaptured;++e){const auto& effect=f.effect[e];
            std::fprintf(file,"%s{\"object\":\"%p\",\"typed\":%s,\"firing_owner_matches\":%s,\"observer_installed\":%s,\"native_shot_160\":%d,\"directions_158\":%d,\"capacity_15c\":%d,\"loop_count_164\":%d,\"active_168\":%u}",
                e?",":"",effect.object,b(effect.typed),b(effect.ownerMatches),b(effect.observerInstalled),effect.nativeShot,effect.directions,effect.capacity,effect.loopCount,unsigned(effect.active));
        }std::fputs("]}",file);
    }std::fputs("]}",file);
}

inline void WriteRayTerminals(FILE* file,const DiagnosticSnapshot& observed){
    const auto& g=observed.guns;const auto& t=observed.rayTerminals;
    const auto u=[](uint64_t v){return static_cast<unsigned long long>(v);};
    std::fprintf(file,",\"native_firearm_pipeline\":{\"queued\":%llu,\"processed\":%llu,\"body_traces\":%llu,\"scene_traces\":%llu,\"body_misses\":%llu,\"scene_blocked\":%llu,\"policy_rejected\":%llu,\"stale\":%llu,\"expired_bindings\":%llu,\"retired_victim_epochs\":%llu,\"physics_deferred_attempts\":%llu,\"binding_deferred_attempts\":%llu,\"damage_deferred_attempts\":%llu,\"queue_overflow\":%llu,\"sources\":[",
        u(g.queued),u(g.processed),u(g.bodyTraces),u(g.sceneTraces),u(g.misses),u(g.sceneBlocked),u(g.policyRejected),u(g.staleShots),u(g.expiredBindings),u(g.retiredVictimEpochs),u(g.physicsDeferred),u(g.bindingDeferred),u(g.pendingDamageDeferred),u(g.queueOverflow));
    bool comma=false;for(size_t n=0;n<t.sources.size();++n){const auto& s=t.sources[n];
        if(!observed.players[n].registration.present&&!s.attempts)continue;
        std::fprintf(file,"%s{\"index\":%zu,\"human\":%s,\"attempts\":%llu,\"deferred_attempts\":%llu,\"terminal_rays\":%llu,\"damage_events\":%llu,\"kills\":%llu,\"terminal_reasons\":{",
            comma?",":"",n,observed.players[n].registration.human?"true":"false",u(s.attempts),u(s.deferredAttempts),u(s.terminals),u(s.damageEvents),u(s.kills));
        bool next=false;for(size_t r=0;r<s.reasons.size();++r)if(s.reasons[r]){
            std::fprintf(file,"%s\"%s\":%llu",next?",":"",firearms::RayReasonName(static_cast<firearms::RayReason>(r)),u(s.reasons[r]));next=true;}
        std::fputs("}}",file);comma=true;
    }
    std::fprintf(file,"],\"human_terminal_count\":%llu,\"human_terminal_capacity\":%zu,\"human_terminals\":[",u(t.humanTerminalCount),t.humanTerminals.size());
    const uint64_t count=(std::min)(t.humanTerminalCount,static_cast<uint64_t>(t.humanTerminals.size()));
    for(uint64_t n=t.humanTerminalCount-count;n<t.humanTerminalCount;++n){const auto& r=t.humanTerminals[n%t.humanTerminals.size()];
        std::fprintf(file,"%s{\"source\":%u,\"epoch\":%u,\"binding\":%llu,\"origin\":%llu,\"pellet\":%u,\"token\":%llu,\"native_shot_index\":%d,\"observed_at\":%.17g,\"completed_at\":%.17g,\"start\":[%.17g,%.17g,%.17g],\"direction\":[%.17g,%.17g,%.17g],\"eligible_before\":%u,\"eligible_after\":%u,\"body_trace_mask\":%u,\"body_hit_mask\":%u,\"nearest\":%d,\"body_distance\":%.17g,\"scene_distance\":%.17g,\"damage\":%.9g,\"region\":%u,\"reason\":\"%s\",\"error\":%u",
            n==t.humanTerminalCount-count?"":",",r.identity.source,r.identity.spawnEpoch,u(r.identity.binding),u(r.identity.nativeOrigin),r.identity.projectile,u(r.damageToken),r.shot.native_shot_index,r.shot.observed_at,r.completedAt,
            r.shot.start.x,r.shot.start.y,r.shot.start.z,r.shot.direction.x,r.shot.direction.y,r.shot.direction.z,r.eligibleBefore,r.eligibleAfter,r.bodyTraceMask,r.bodyHitMask,r.nearest,r.bodyDistance,r.sceneDistance,r.damage,unsigned(r.region),firearms::RayReasonName(r.reason),r.error);
        const auto b=[](bool value){return value?"true":"false";};
        std::fprintf(file,",\"gate\":{\"sampled\":%s,\"source_registered\":%s,\"source_roster_alive\":%s,\"source_native_alive\":%s,\"source_ready\":%s,\"source_phase\":%u,\"match_started\":%s,\"match_started_at\":%.17g,\"match_phase_active\":%s,\"match_ready\":%s,\"match_ended\":%s,\"damage_phase_active\":%s}}",
            b(r.gateSampled),b(r.sourceRegistered),b(r.sourceRosterAlive),b(r.sourceNativeAlive),b(r.sourceReady),r.sourcePhase,
            b(r.matchStarted),r.matchStartedAt,b(r.matchPhaseActive),b(r.matchReady),b(r.matchEnded),b(r.damagePhaseActive));
    }
    std::fputs("],\"receipt_changes_damage_rules\":false,\"deferred_is_terminal\":false,\"native_exception_is_terminal_receipt\":false,\"runtime_verified\":false}",file);
}

inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;
    const auto observed=DiagnosticsSnapshot();const auto& summary=observed.summary;
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(file,"{\"schema_version\":1,\"stage\":\"%s\",\"operation\":\"%s\",\"error\":%u,\"initialized\":%s,\"ready\":%s,\"restart_required\":%s,\"profiles_verified\":%s,\"net_mode\":%d,\"registered_players\":%u,\"active_sources\":%u,\"physics_ready_sources\":%u,\"ammo_policy_known\":%s,\"infinite_ammo\":%s,\"invincible\":%s,\"configuration_revision\":%u,\"polls\":%llu,\"damage_events\":%llu,\"kills\":%llu,\"native_remote_calls\":%llu,\"native_remote_accepted\":%llu,\"native_remote_rejected\":%llu,\"replication_fault\":%s,\"native_gun_observed\":%s,\"native_melee_observed\":%s,\"native_dagger_observed\":%s,\"native_gun_rays\":%llu,\"native_melee_entries\":%llu,\"native_melee_contacts\":%llu,\"native_dagger_points\":%llu,\"dagger_queued\":%llu,\"dagger_duplicates\":%llu,\"dagger_no_baseline\":%llu,\"dagger_native_life_changed\":%llu,\"dagger_damage\":%llu,\"dagger_kills\":%llu,\"ammo_refills\":%llu,\"heals\":%llu,\"last_source\":%d,\"last_victim\":%d,\"last_token\":%llu,\"last_damage\":%.9g,\"last_region\":%u,\"exception_rva\":%llu,\"players\":[",
        summary.stage,summary.operation,summary.error,b(summary.initialized),b(summary.ready),b(summary.restartRequired),b(summary.profilesVerified),summary.netMode,summary.registeredPlayers,summary.activeSources,summary.physicsReadySources,
        b(summary.ammoPolicyKnown),b(summary.infiniteAmmo),b(summary.invincible),summary.configurationRevision,static_cast<unsigned long long>(summary.polls),static_cast<unsigned long long>(summary.damageEvents),static_cast<unsigned long long>(summary.kills),
        static_cast<unsigned long long>(summary.damageRemoteCalls),static_cast<unsigned long long>(summary.damageRemoteAccepted),static_cast<unsigned long long>(summary.damageRemoteRejected),b(summary.replicationFault),
        b(summary.nativeGunObserved),b(summary.nativeMeleeObserved),b(summary.nativeDaggerObserved),static_cast<unsigned long long>(observed.guns.nativeRays),static_cast<unsigned long long>(observed.knife.entries),static_cast<unsigned long long>(observed.knife.exactAuthorityContacts),
        static_cast<unsigned long long>(summary.daggerPoints),static_cast<unsigned long long>(summary.daggerQueued),static_cast<unsigned long long>(summary.daggerDuplicates),static_cast<unsigned long long>(summary.daggerNoBaseline),static_cast<unsigned long long>(summary.daggerNativeChanged),static_cast<unsigned long long>(summary.daggerDamage),static_cast<unsigned long long>(summary.daggerKills),
        static_cast<unsigned long long>(summary.ammoRefills),static_cast<unsigned long long>(summary.heals),summary.lastSource,summary.lastVictim,static_cast<unsigned long long>(summary.lastToken),summary.lastDamage,static_cast<unsigned>(summary.lastRegion),static_cast<unsigned long long>(summary.exceptionRva));
    bool comma=false;for(size_t n=0;n<observed.players.size();++n){const auto& r=observed.players[n].registration;if(!r.present)continue;
        const auto& s=observed.players[n].source;const auto& d=observed.players[n];
        std::fprintf(file,"%s{\"index\":%zu,\"epoch\":%u,\"human\":%s,\"actual_authority_tuple\":%s,\"pawn\":\"%p\",\"weapon\":\"%p\",\"observing\":%s,\"alive\":%s,\"physics_ready\":%s,\"input_ready\":%s,\"native_origins\":%llu,\"native_rays\":%llu,\"health\":%.9g,\"shield\":%.9g",
            comma?",":"",n,r.epoch,b(r.human),b(observed.players[n].actualAuthority),r.pawn,s.weapon,b(s.observing),b(d.alive),b(s.physicsReady),b(s.inputReady),static_cast<unsigned long long>(s.originsObserved),static_cast<unsigned long long>(s.raysObserved),d.health,d.rawShield);
        std::fprintf(file,",\"external_input_producer\":%s,\"input_held\":%u,\"combat_input_edges\":%llu,\"combat_input_submissions\":%llu,\"combat_view_submissions\":%llu,\"native_history_capacity\":%d,\"native_pending_input\":%d,\"local_input_component\":%s",
            b(s.externalInputProducer),s.held,static_cast<unsigned long long>(s.inputEdges),static_cast<unsigned long long>(s.inputSubmissions),
            static_cast<unsigned long long>(s.viewSubmissions),s.historyCapacity,s.pendingInput,b(s.localInputComponent));
        std::fprintf(file,",\"bound\":%s,\"current\":%s,\"ammo_component_count_at_10c\":%d,\"last_firing_slot\":%d,\"native_trigger_delegates\":%d,\"external_batches\":%llu,\"external_applied_batches\":%llu,\"external_cancelled_batches\":%llu,\"external_native_resets\":%llu,\"external_pending\":%s,\"external_native_state_matches\":%s,\"external_observed_held\":%u,\"external_last_event\":%d,\"external_observed_event\":%d,\"external_movement_time\":%.9g,\"external_deadline\":%.9g",
            b(s.bound),b(s.current),s.ammoCount,s.lastFiringSlot,s.triggerDelegates,static_cast<unsigned long long>(s.externalBatches),
            static_cast<unsigned long long>(s.externalAppliedBatches),static_cast<unsigned long long>(s.externalCancelledBatches),static_cast<unsigned long long>(s.externalResets),
            b(s.externalPending),b(s.externalNativeStateMatches),s.externalObservedHeld,s.externalLastEvent,s.externalObservedEvent,s.externalMovementTime,s.externalDeadline);
        std::fprintf(file,",\"external_can_submit\":%s,\"external_native_applied_ready\":%s,\"external_native_aim_matches\":%s,\"native_context_matches\":%s,\"native_input_correction_260\":%d,\"native_authority_correction_328\":%d,\"native_correction_origin_32c\":%d,\"native_incoming_base_364\":%d,\"native_input_current_250\":\"%p\",\"native_input_declared_258\":\"%p\",\"native_inventory_current_280\":\"%p\",\"native_corrected_320\":\"%p\",\"native_correction_state_2fc\":%u,\"native_initial_correction_published_30c\":%s,\"native_initial_correction_processed_30d\":%s,\"native_aim_valid_128\":%s,\"native_aim_238\":[%.17g,%.17g,%.17g],\"submitted_aim\":[%.17g,%.17g,%.17g]",
            b(s.externalCanSubmit),b(s.externalNativeAppliedReady),b(s.externalNativeAimMatches),b(s.contextMatches),s.inputCorrection,s.authorityCorrection,s.correctionOrigin,s.incomingBase,
            s.nativeInputCurrent,s.nativeInputDeclared,s.nativeInventoryCurrent,s.nativeCorrected,static_cast<unsigned>(s.correctionState),b(s.initialCorrectionPublished),b(s.initialCorrectionProcessed),b(s.nativeAimValid),
            s.nativeAim.x,s.nativeAim.y,s.nativeAim.z,s.submittedAim.x,s.submittedAim.y,s.submittedAim.z);
        std::fprintf(file,",\"external_deduplicated\":%llu",static_cast<unsigned long long>(s.externalDeduplicated));
        WriteWeaponInspection(file,s.inspection);
        const auto& reference=r.referenceDiagnosis;
        std::fprintf(file,",\"reference_sampled\":%s,\"controller_role\":%u,\"controller_remote_role\":%u,\"controller_replicates\":%s,\"controller_only_relevant_to_owner\":%s,\"controller_native_network_supported\":%s,\"controller_guid_known\":%s,\"controller_guid_found\":%s,\"controller_guid\":%llu,\"controller_native_guid_would_be_zero\":%s,\"reference_stage\":\"%s\"}",
            b(reference.sampled),static_cast<unsigned>(reference.role),static_cast<unsigned>(reference.remoteRole),b(reference.replicates),b(reference.onlyRelevantToOwner),b(reference.nativeSupportsObject),b(reference.existingGuidKnown),b(reference.guidFound),static_cast<unsigned long long>(reference.existingGuid),b(reference.guidWouldBeZero),reference.stage);comma=true;
    }
    std::fprintf(file,"],\"rpc_references_known\":%s,\"rpc_reference_gate_ready\":%s,\"foreign_controller_hits_rejected\":%llu,\"foreign_references_rejected\":%llu,\"last_blocked_reference\":\"%s\",\"last_blocked_object\":\"%p\"",
        b(summary.rpcReferencesKnown),b(summary.rpcReferenceGateReady),static_cast<unsigned long long>(summary.foreignControllerHitsRejected),
        static_cast<unsigned long long>(summary.foreignReferenceRejected),summary.lastBlockedReference,summary.lastBlockedObject);
    std::fprintf(file,",\"replica_policy_verified\":%s,\"replica_process_id\":%u,\"replica_policy_current\":%s,\"replica_delay_unmapped_value\":%d,\"replica_multicast_flags\":%u,\"replica_policy_sampled_at_ms\":%llu",
        b(summary.replicaPolicyVerified),summary.replicaProcessId,b(observed.policyCurrent),summary.replicaRPCPolicy.delayUnmappedValue,
        summary.replicaRPCPolicy.multicastFlags,static_cast<unsigned long long>(summary.replicaRPCPolicy.sampledAt));
    std::fprintf(file,",\"cached_snapshot\":true,\"snapshot_valid\":%s,\"snapshot_sampled_at_ms\":%llu,\"weapon_input_readiness_is_delivery_proof\":false",
        b(observed.valid),static_cast<unsigned long long>(observed.sampledAt));
    std::fprintf(file,",\"native_trigger_delegates_field_is_storage_kind\":true,\"native_origin_queries\":%llu,\"native_origin_pairs\":%llu,\"native_context_matches\":%llu,\"native_context_rejected\":%llu,\"effect_observer_records\":%llu,\"effect_callbacks\":%llu,\"effect_start_callbacks\":%llu,\"effect_observed_dispatches\":%llu,\"native_direction_records\":%llu,\"invalid_ray_payloads\":%llu,\"unpaired_origins\":%llu,\"ambiguous_origins\":%llu,\"duplicate_rays\":%llu",
        static_cast<unsigned long long>(observed.guns.originQueries),static_cast<unsigned long long>(observed.guns.exactOrigins),static_cast<unsigned long long>(observed.guns.nativeContextMatches),static_cast<unsigned long long>(observed.guns.nativeContextRejected),
        static_cast<unsigned long long>(observed.guns.effectObserverRecords),static_cast<unsigned long long>(observed.guns.effectCallbacks),static_cast<unsigned long long>(observed.guns.effectStartCallbacks),static_cast<unsigned long long>(observed.guns.effectObservedDispatches),
        static_cast<unsigned long long>(observed.guns.directions),static_cast<unsigned long long>(observed.guns.invalidPayloads),static_cast<unsigned long long>(observed.guns.unpairedOrigins),static_cast<unsigned long long>(observed.guns.ambiguousOrigins),static_cast<unsigned long long>(observed.guns.duplicateRays));
    const auto& geometry=observed.guns;
    std::fprintf(file,",\"native_geometry\":{\"observers_installed\":%llu,\"tables_restored\":%llu,\"dispatches\":%llu,\"getter_callbacks\":%llu,\"original_forward_attempts\":%llu,\"original_forward_completed\":%llu,\"exact_frames\":%llu,\"rejected_frames\":%llu,\"captures\":%llu,\"rays_queued\":%llu,\"unpaired\":%llu,\"invalid\":%llu,\"native_life_changed_no_second_damage\":%llu,\"no_life_baseline\":%llu,\"own_native_life_receipts\":%llu,\"last_shot\":%d,\"last_count\":%d,\"last_start\":[%.17g,%.17g,%.17g],\"last_direction\":[%.17g,%.17g,%.17g],\"requires_visual_fxc\":false,\"capture_is_damage_receipt\":false}",
        static_cast<unsigned long long>(geometry.firingObservers),static_cast<unsigned long long>(geometry.firingTablesRestored),
        static_cast<unsigned long long>(geometry.geometryDispatches),static_cast<unsigned long long>(geometry.geometryCallbacks),
        static_cast<unsigned long long>(geometry.geometryForwardAttempts),static_cast<unsigned long long>(geometry.geometryForwardCompleted),
        static_cast<unsigned long long>(geometry.geometryFrameMatches),static_cast<unsigned long long>(geometry.geometryFrameRejected),
        static_cast<unsigned long long>(geometry.geometryCaptures),static_cast<unsigned long long>(geometry.geometryRays),
        static_cast<unsigned long long>(geometry.geometryUnpaired),static_cast<unsigned long long>(geometry.geometryInvalid),
        static_cast<unsigned long long>(geometry.nativeLifeChanged),static_cast<unsigned long long>(geometry.nativeLifeMissing),static_cast<unsigned long long>(geometry.lifeReceipts),
        geometry.lastGeometryShot,geometry.lastGeometryCount,geometry.lastGeometryStart.x,geometry.lastGeometryStart.y,geometry.lastGeometryStart.z,
        geometry.lastGeometryDirection.x,geometry.lastGeometryDirection.y,geometry.lastGeometryDirection.z);
    WriteRayTerminals(file,observed);
    const auto& submission=summary.npcSubmission;
    std::fprintf(file,",\"last_npc_submission\":{\"sequence\":%llu,\"index\":%u,\"epoch\":%u,\"binding\":%llu,\"outcome\":\"%s\",\"desired_held\":%u,\"changed\":%u,\"built\":%d,\"before_base\":%d,\"before_count\":%d,\"before_applied\":%d,\"after_base\":%d,\"after_count\":%d,\"after_applied\":%d,\"buffer_consumed\":%s,\"native_history_matches\":%s,\"source_changed\":%s,\"before_current\":\"%p\",\"after_current\":\"%p\",\"before_authority\":%d,\"after_authority\":%d}",
        static_cast<unsigned long long>(submission.sequence),submission.index,submission.epoch,static_cast<unsigned long long>(submission.binding),submission.outcome,
        submission.desired,submission.changed,submission.built,submission.beforeBase,submission.beforeCount,submission.beforeApplied,submission.afterBase,submission.afterCount,submission.afterApplied,
        b(submission.bufferConsumed),b(submission.historyMatches),b(submission.sourceChanged),submission.beforeCurrent,submission.afterCurrent,submission.beforeAuthority,submission.afterAuthority);
    std::fprintf(file,",\"typed_ability_damage\":{\"preflight_queries\":%llu,\"preflight_rejected\":%llu,\"damage_events\":%llu,\"kills\":%llu,\"remote_calls\":%llu,\"remote_accepted\":%llu,\"remote_rejected\":%llu,\"remote_acceptance_is_delivery_proof\":false}",
        static_cast<unsigned long long>(summary.typedPreflightQueries),static_cast<unsigned long long>(summary.typedPreflightRejected),
        static_cast<unsigned long long>(summary.typedDamageEvents),static_cast<unsigned long long>(summary.typedKills),
        static_cast<unsigned long long>(summary.typedRemoteCalls),static_cast<unsigned long long>(summary.typedRemoteAccepted),
        static_cast<unsigned long long>(summary.typedRemoteRejected));
    std::fputs(",\"authority\":\"original_native_host_plus_missing_authority_records\",\"native_remote_acceptance_is_delivery_proof\":false,\"native_prediction_preserved\":true,\"direct_health_writes\":false,\"client_damage_simulation\":false,\"runtime_verified\":false}\n",file);
    const bool okay=std::ferror(file)==0;std::fclose(file);return okay;
}
} // namespace local_server_combat
