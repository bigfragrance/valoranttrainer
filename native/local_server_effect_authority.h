#pragma once
#include "original_effects.h"
#include "original_jett_priming.h"
#include "local_server_match.h"
#include "local_server_empty_bootstrap_probe.h"
#include "local_server_effect_authority.generated.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>

// Candidate HOST producer for the three original continuous-effect Blueprint
// natives. Original handles, data operations, reflected arrays, RPCs and client
// receive implementations remain native. Direct C++ callers of the typed
// library functions are outside this boundary; see effect-authority/entry-review.
namespace local_server_effect_authority {
using Weak=original_inventory_slots::WeakIdentity;
using Exec=void(*)(void*,void*,void*);
using Name=original_effects::Name;
struct alignas(8) Data {uint8_t bytes[88]{};};
struct alignas(8) Handle {uint8_t bytes[24]{};};
struct Array {uint8_t* data=nullptr;int32_t count=0,capacity=0;};
struct alignas(16) Active {
    Handle id{};void* type=nullptr;Data data{};uint8_t padding78[8]{};
    engine::Transform transform{};Name socket{};float timestamp=0;
    void* container=nullptr;uint64_t proxyTimer=0;uint8_t alliance=3,padding101[15]{};
};
struct Play {
    void *type=nullptr,*actor=nullptr,*context=nullptr;Data data{};void* worldContext=nullptr;
    engine::Vec location{},rotation{};Name socket{};
    uint8_t local=0,alliance=0,cosmetic=0,replay=0;Handle result{};
};
struct Update {void *actor=nullptr,*worldContext=nullptr;Handle id{};Data data{};};
struct Stop {void *actor=nullptr,*worldContext=nullptr;Handle id{};uint8_t type=0,padding[7]{};};
struct alignas(16) MulticastPlay {
    void* type=nullptr;Data data{};engine::Transform transform{};Name socket{};uint8_t paddingCC[4]{};
    Handle id{};void* triggered=nullptr;float movementTime=0;uint8_t alliance=3,paddingF5[11]{};
};
struct MulticastUpdate {Handle id{};Data data{};void* triggered=nullptr;};
struct MulticastStop {Handle id{};void* triggered=nullptr;uint8_t type=0,padding[3]{};float movementTime=0;};
static_assert(sizeof(Data)==88&&sizeof(Handle)==24&&sizeof(Array)==16);
static_assert(sizeof(Active)==0x110&&alignof(Active)==16&&offsetof(Active,data)==0x20
    &&offsetof(Active,transform)==0x80&&offsetof(Active,socket)==0xe0&&offsetof(Active,timestamp)==0xec
    &&offsetof(Active,container)==0xf0&&offsetof(Active,proxyTimer)==0xf8&&offsetof(Active,alliance)==0x100);
static_assert(sizeof(Play)==208&&offsetof(Play,data)==24&&offsetof(Play,result)==184);
static_assert(sizeof(Update)==128&&sizeof(Stop)==48&&offsetof(Stop,type)==40);
static_assert(sizeof(MulticastPlay)==256&&offsetof(MulticastPlay,id)==208&&offsetof(MulticastPlay,alliance)==244);
static_assert(sizeof(MulticastUpdate)==120&&sizeof(MulticastStop)==40);

inline constexpr size_t MaxSources=4096,MaxEffects=8192;
struct Source {
    bool used=false,retiring=false;size_t index=0;uint32_t epoch=0;uint64_t registration=0;
    void *pc=nullptr,*ps=nullptr,*pawn=nullptr;Weak pcId{},psId{},pawnId{};
};
struct Receipt {
    bool used=false,stopped=false;uint32_t epoch=0;uint64_t registration=0;
    void *manager=nullptr,*actor=nullptr,*type=nullptr,*caller=nullptr;
    Weak managerId{},actorId{},typeId{},callerId{};Handle id{};Source origin{};
};
struct Function {
    void* object=nullptr;Weak identity{};Exec original=nullptr,replacement=nullptr;
    std::array<void*,13> parameters{};uint8_t inputs=0;bool published=false;
};
struct MetadataFailure {
    bool observed=false;const char* kind="none";const wchar_t* name=L"";
    void *object=nullptr,*expectedOwner=nullptr,*actualOwner=nullptr,*field=nullptr,*fieldClass=nullptr;
    uintptr_t expectedExec=0,actualExec=0;uint64_t expectedFlags=0,actualFlags=0,expectedName=0,actualName=0;
    int32_t expectedOffset=-1,actualOffset=-1,expectedBytes=0,actualBytes=0,arrayDim=0;uint16_t parmsSize=0;
};
struct State {
    bool initialized=false,installed=false,closing=false,closed=false,restartRequired=false;
    bool cleanupRetained=false,execsRestored=false;
    bool profilesVerified=false,schemaVerified=false,hostVerified=false;
    void *world=nullptr,*driver=nullptr,*managerType=nullptr,*componentType=nullptr,*fxcType=nullptr;
    void *recordType=nullptr,*dataType=nullptr,*idType=nullptr,*arrayProperty=nullptr;
    void *rpcPlay=nullptr,*rpcUpdate=nullptr,*rpcStop=nullptr;Weak worldId{},driverId{};
    DWORD gameThread=0;unsigned error=0,active=0;uint32_t sources=0,effects=0;
    uint64_t nextRegistration=0,calls=0,originalForwards=0,typedForwards=0,decodedCalls=0;
    uint64_t scriptSteps=0,compiledSteps=0,dataCopies=0,dataDestroys=0,calleeOwnedTransfers=0;
    uint64_t plays=0,updates=0,stops=0,retirements=0,sourcePruned=0,effectPruned=0;
    uint64_t routeRefreshes=0,routeReadbacks=0,arrayReadbacks=0,rpcCalls=0,rpcReturns=0;
    uint64_t actorChannelSamples=0,actorChannelMatches=0,rpcWithNativeChannel=0;
    uint64_t registryVisits=0,foreignBindings=0,restoredBindings=0,unknownUpdates=0;
    uint64_t rejectedRetiringPlay=0,unsupportedPlay=0,closeCalls=0;
    uint64_t reentrantStops=0,nativeLifetimeEndedDuringRpc=0,closeUnknownLifetimes=0;
    MetadataFailure metadata{};
    uint64_t replicationSetupCalls=0,replicationSetupReadbacks=0;
    void* lastReplicationClass=nullptr;uint32_t replicationFlagsBefore=0,replicationFlagsAfter=0;
    int32_t replicationClassReps=0;uint16_t replicationPropertyIndex=0;
    uintptr_t exceptionRva=0;uint32_t exceptionCode=0;const char* stage="effect_authority_not_initialized";
};
inline State state{},cached{};
inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline std::array<Source,MaxSources> sources{};
inline std::array<Receipt,MaxEffects> effects{};
// Static compiled callbacks and these immutable parent addresses remain backed
// for the process lifetime, including a late callback after UF restoration.
inline std::array<Function,3> functions{};
struct ReplicationClassReceipt {void* type=nullptr;Weak identity{};bool used=false,attempted=false,verified=false,inFlight=false;};
inline std::array<ReplicationClassReceipt,64> replicationClasses{};
inline const State& GetState(){return state;}
inline void Cache(){AcquireSRWLockExclusive(&cacheLock);cached=state;ReleaseSRWLockExclusive(&cacheLock);}
inline State Snapshot(){AcquireSRWLockShared(&cacheLock);auto value=cached;ReleaseSRWLockShared(&cacheLock);return value;}
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Capture(void* object,Weak& identity){return original_inventory_slots::CaptureIdentity(object,identity)&&Live(object,identity);}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;Cache();return false;}
inline bool Thread(){return engine::image&&original_players::Thread()&&state.gameThread==GetCurrentThreadId();}
inline bool Writable(void* p,size_t bytes){
    if(!p||!bytes)return false;auto at=reinterpret_cast<uintptr_t>(p);if(at+bytes<at)return false;
    const auto end=at+bytes;while(at<end){MEMORY_BASIC_INFORMATION m{};
        if(!VirtualQuery(reinterpret_cast<void*>(at),&m,sizeof(m))||m.State!=MEM_COMMIT||m.Protect&(PAGE_GUARD|PAGE_NOACCESS))return false;
        const auto protection=m.Protect&0xffU;
        if(protection!=PAGE_READWRITE&&protection!=PAGE_WRITECOPY&&protection!=PAGE_EXECUTE_READWRITE&&protection!=PAGE_EXECUTE_WRITECOPY)return false;
        const auto next=reinterpret_cast<uintptr_t>(m.BaseAddress)+m.RegionSize;if(next<=at)return false;at=next;
    }return true;
}
template<class T> inline bool Put(void* object,size_t offset,const T& value){
    auto to=static_cast<uint8_t*>(object)+offset;if(!Writable(to,sizeof(T)))return false;std::memcpy(to,&value,sizeof(T));return true;
}
inline bool ImageCode(const void* p){
    MEMORY_BASIC_INFORMATION m{};if(!p||!VirtualQuery(p,&m,sizeof(m)))return false;
    const auto protection=m.Protect&0xffU;
    return m.State==MEM_COMMIT&&m.Type==MEM_IMAGE&&m.AllocationBase==reinterpret_cast<void*>(engine::image)
        &&!(m.Protect&(PAGE_GUARD|PAGE_NOACCESS))&&(protection==PAGE_EXECUTE||protection==PAGE_EXECUTE_READ
            ||protection==PAGE_EXECUTE_READWRITE||protection==PAGE_EXECUTE_WRITECOPY);
}
inline bool HostWorld(){
    return state.world&&state.driver&&Live(state.world,state.worldId)&&Live(state.driver,state.driverId)
        &&Read<void*>(state.world,0x40)==state.driver&&Read<void*>(state.driver,0x178)==state.world
        &&!Read<void*>(state.driver,0xc0)&&Read<int32_t>(state.driver,0xd0)>=0&&Read<int32_t>(state.driver,0xd0)<=32;
}
inline bool ValidArray(const Array& a,size_t stride,int32_t maximum=16384){
    return a.count>=0&&a.count<=maximum&&a.capacity>=a.count&&a.capacity<=maximum
        &&(!a.count||(a.data&&original_players::Readable(a.data,size_t(a.count)*stride)));
}
inline bool SameID(const Handle& a,const Handle& b){return Read<uint64_t>(&a)==Read<uint64_t>(&b)&&Read<uint64_t>(&a,8)==Read<uint64_t>(&b,8);}
inline bool ValidID(const Handle& id){const auto value=Read<uint64_t>(&id);return value!=0&&value!=1;}
inline bool Local(const Handle& id){return Read<uint8_t>(&id,0x14)!=0;}
inline bool Field(void* owner,const wchar_t* name,int32_t offset,int32_t bytes,void** field=nullptr,uint64_t flags=0){
    original_jett_priming::PropertyInfo d{};
    if(original_jett_priming::FieldOffset(owner,name,size_t(bytes),&d)!=offset||d.arrayDim!=1||d.elementSize!=bytes
       ||(flags&&Read<uint64_t>(d.field,0x38)!=flags)){
        if(!state.metadata.observed){auto& m=state.metadata;m.observed=true;m.kind="field";m.name=name;m.object=owner;
            m.field=d.field;m.fieldClass=d.fieldClass;m.expectedOffset=offset;m.actualOffset=d.offset;
            m.expectedBytes=bytes;m.actualBytes=d.elementSize;m.arrayDim=d.arrayDim;
            m.expectedFlags=flags;m.actualFlags=Read<uint64_t>(d.field,0x38);m.expectedName=d.expectedName;m.actualName=d.actualName;
        }return false;
    }
    if(field)*field=d.field;return true;
}
inline bool SourceBase(const Source& s){
    return s.used&&s.epoch&&s.registration&&HostWorld()&&Live(s.pawn,s.pawnId)
        &&original_players::IsA(s.pawn,original_players::ShooterCharacterClass)&&Read<uint8_t>(s.pawn,0x224)==3
        &&original_effects::GetWorld(s.pawn)==state.world;
}
inline bool CurrentSource(const Source& s){
    const auto p=local_server_match::Player(s.index);
    return !s.retiring&&SourceBase(s)&&p&&p->epoch==s.epoch&&p->pc==s.pc&&p->ps==s.ps&&p->pawn==s.pawn
        &&local_server_match::GetState().world==state.world&&local_server_match::AuthorityPair(*p)
        &&Live(s.pc,s.pcId)&&Live(s.ps,s.psId);
}
inline Source* FindPawn(void* pawn){
    for(auto& s:sources)if(s.used&&s.pawn==pawn&&SourceBase(s))return &s;return nullptr;
}
inline Source* ResolveCaller(void* object){
    // Read only real actor/component ownership. No CDO, name or hero whitelist.
    auto original=object;
    std::array<void*,9> seen{};
    for(size_t depth=0;object&&depth<seen.size();++depth){
        if(!original_players::Readable(object,0x30))break;
        for(size_t n=0;n<depth;++n)if(seen[n]==object)return nullptr;seen[depth]=object;
        if(auto source=FindPawn(object))return source;
        for(auto& source:sources)if(source.used&&(source.pc==object||source.ps==object)&&CurrentSource(source))return &source;
        auto type=Read<void*>(object,0x10);
        if(original_players::Derived(type,state.componentType))object=Read<void*>(object,0xc8);
        else if(original_players::IsA(object,0x3683900))object=Read<void*>(object,0x210);
        else break;
    }
    // A real original caller can outlive its source pawn. An owning receipt is
    // cleanup evidence, not permission to start a new effect in that old epoch.
    for(auto& e:effects)if(e.used&&e.caller==original&&Live(e.caller,e.callerId)
        &&original_effects::GetWorld(e.caller)==state.world){
        e.origin.retiring=true;return &e.origin;
    }return nullptr;
}
inline void Prune(){
    for(auto& e:effects)if(e.used&&e.stopped&&(!Live(e.manager,e.managerId)||!Live(e.actor,e.actorId))){
        e={};++state.effectPruned;if(state.effects)--state.effects;
    }
    for(auto& s:sources)if(s.used&&!Live(s.pawn,s.pawnId)){s={};++state.sourcePruned;if(state.sources)--state.sources;}
}
inline bool RegisterPlayer(size_t index,void* pc,void* ps,void* pawn,uint32_t epoch){
    if(!Thread()||!state.installed||state.closing||state.closed||state.error||!HostWorld())return Fail(7601,"effect_authority_registration_context_invalid");
    const auto p=local_server_match::Player(index);
    if(!p||p->index!=index||p->epoch!=epoch||p->pc!=pc||p->ps!=ps||p->pawn!=pawn||!epoch
       ||local_server_match::GetState().world!=state.world||!local_server_match::AuthorityPair(*p)
       ||original_effects::GetWorld(pawn)!=state.world)return Fail(7602,"effect_authority_requires_actual_match_authority_tuple");
    Prune();for(auto& s:sources)if(s.used&&s.pawn==pawn&&s.epoch==epoch)
        return s.index==index&&s.pc==pc&&s.ps==ps&&CurrentSource(s);
    for(auto& s:sources)if(s.used&&s.index==index)s.retiring=true;
    for(auto& e:effects)if(e.used&&e.origin.index==index)e.origin.retiring=true;
    Source value{};value.used=true;value.index=index;value.pc=pc;value.ps=ps;value.pawn=pawn;value.epoch=epoch;
    if(!Capture(pc,value.pcId)||!Capture(ps,value.psId)||!Capture(pawn,value.pawnId))return Fail(7603,"effect_authority_player_weak_identity_missing");
    if(state.nextRegistration==UINT64_MAX)return Fail(7604,"effect_authority_receipt_counter_exhausted");
    value.registration=++state.nextRegistration;
    for(auto& s:sources)if(!s.used){s=value;++state.sources;state.stage="effect_authority_actual_match_source_registered";Cache();return true;}
    return Fail(7604,"effect_authority_live_source_capacity_exhausted");
}
inline bool RetirePlayer(size_t index,void* pawn,uint32_t epoch){
    if(!state.initialized)return true;
    if(!Thread()||!pawn||!epoch)return Fail(7605,"effect_authority_retirement_context_invalid");
    bool found=false;
    for(auto& s:sources)if(s.used&&s.index==index&&s.pawn==pawn&&s.epoch==epoch){s.retiring=true;found=true;}
    for(auto& e:effects)if(e.used&&e.origin.index==index&&e.origin.pawn==pawn&&e.origin.epoch==epoch){e.origin.retiring=true;found=true;}
    if(found){++state.retirements;state.stage="effect_authority_epoch_fenced_cleanup_receipts_retained";Cache();return true;}
    return Fail(7605,"effect_authority_retirement_receipt_missing");
}
inline int32_t FindActive(void* manager,const Handle& id){
    const auto a=Read<Array>(manager,0x118);if(!ValidArray(a,sizeof(Active)))return -2;int32_t result=-1;
    for(int32_t n=a.count;n-->0;)if(SameID(Read<Handle>(a.data,size_t(n)*sizeof(Active)),id)){
        if(result>=0)return -3;result=n;
    }return result;
}
inline Receipt* FindReceipt(void* manager,const Handle& id,uint64_t registration){
    for(auto& e:effects)if(e.used&&e.manager==manager&&(!registration||e.registration==registration)&&SameID(e.id,id)
        &&Live(e.manager,e.managerId)&&Live(e.actor,e.actorId)&&Live(e.type,e.typeId))return &e;return nullptr;
}
inline Receipt* FindActorReceipt(void* actor,const Handle& id){
    for(auto& e:effects)if(e.used&&e.actor==actor&&SameID(e.id,id)&&Live(e.actor,e.actorId)
        &&Live(e.manager,e.managerId)&&Live(e.type,e.typeId))return &e;return nullptr;
}
inline bool OriginalRecordIdentical(const void* a,const void* b){
    Array left{const_cast<uint8_t*>(static_cast<const uint8_t*>(a)),1,1};
    Array right{const_cast<uint8_t*>(static_cast<const uint8_t*>(b)),1,1};
    return engine::function<bool(*)(const void*,void*,const void*,void*)>(0x4516720)(&left,state.arrayProperty,&right,state.arrayProperty);
}
inline bool PostRpcReceipt(const Receipt& expected){
    // Original FXC/BP callbacks can synchronously Stop or destroy their owner.
    // Such native lifetime changes are not an unchanged-live-record witness.
    if(!Live(expected.manager,expected.managerId)||!Live(expected.actor,expected.actorId)){
        ++state.nativeLifetimeEndedDuringRpc;return true;
    }
    const auto actual=FindReceipt(expected.manager,expected.id,expected.registration);
    if(!actual||actual->type!=expected.type||actual->epoch!=expected.epoch)return false;
    const auto index=FindActive(expected.manager,expected.id);
    if(actual->stopped&&index==-1){++state.reentrantStops;return true;}
    return !actual->stopped&&index>=0;
}
inline bool ReplicationClassReadback(void* type){
    state.lastReplicationClass=type;state.replicationFlagsAfter=Read<uint32_t>(type,0xdc);
    state.replicationPropertyIndex=Read<uint16_t>(state.arrayProperty,0x40);
    const auto reps=Read<Array>(type,0x100);state.replicationClassReps=reps.count;
    return (state.replicationFlagsAfter&0x800U)!=0&&state.replicationPropertyIndex==2
        &&ValidArray(reps,0x10,16384)&&reps.count>2
        &&Read<void*>(reps.data,2*0x10)==state.arrayProperty&&Read<int32_t>(reps.data,2*0x10+8)==0;
}
inline bool PrepareOriginalReplicationClass(void* type){
    if(!Thread()||!type||!original_players::IsA(type,0x1af0eb0)||!original_players::Derived(type,state.managerType))
        return Fail(7635,"effect_authority_runtime_replication_class_not_original_manager_type");
    Weak identity{};if(!Capture(type,identity))return Fail(7635,"effect_authority_runtime_replication_class_weak_missing");
    ReplicationClassReceipt* receipt=nullptr;ReplicationClassReceipt* empty=nullptr;
    for(auto& r:replicationClasses){
        if(r.used&&r.type==type&&Live(r.type,r.identity)){receipt=&r;break;}
        if(!empty&&(!r.used||!Live(r.type,r.identity)))empty=&r;
    }
    if(!receipt){if(!empty)return Fail(7635,"effect_authority_runtime_replication_class_receipts_exhausted");
        *empty={};empty->used=true;empty->type=type;empty->identity=identity;receipt=empty;}
    state.replicationFlagsBefore=Read<uint32_t>(type,0xdc);state.lastReplicationClass=type;
    if(receipt->inFlight)return Fail(7635,"effect_authority_runtime_replication_setup_reentered_before_completion");
    if(!receipt->verified&&(state.replicationFlagsBefore&0x800U)==0){
        if(receipt->attempted)return Fail(7635,"effect_authority_runtime_replication_setup_already_attempted_no_retry");
        receipt->attempted=true;receipt->inFlight=true;++state.replicationSetupCalls;
        // The exact original RepLayout49D65C0 calls this at49D6695.
        // It recursively prepares superclasses, builds original ClassReps,
        // assigns the real FProperty RepIndex and sets its native setup bit.
        __try{engine::function<void(*)(void*)>(0x1b979c0)(type);}
        __finally{receipt->inFlight=false;}
    }
    // Never clear flags, fabricate an index or retry a failed native setup.
    if(!Live(type,identity)||!ReplicationClassReadback(type)){
        state.restartRequired=true;return Fail(7635,"effect_authority_original_runtime_replication_setup_readback_failed");
    }
    receipt->verified=true;++state.replicationSetupReadbacks;return true;
}
inline bool Route(void* actor,void* manager,bool refresh){
    Weak actorIdentity{},managerIdentity{};
    if(!Capture(actor,actorIdentity)||!Capture(manager,managerIdentity))return false;
    const auto actualClass=Read<void*>(manager,0x10);
    if(!actor||!manager||Read<uint8_t>(actor,0x224)!=3||original_effects::GetWorld(actor)!=state.world
       ||!original_players::Derived(Read<void*>(manager,0x10),state.managerType)||Read<void*>(manager,0xc8)!=actor
       ||original_effects::GetManager(actor,false)!=manager||(Read<uint8_t>(manager,0xa0)&0x20U)==0
       ||Read<void*>(Read<void*>(manager),0x220)!=reinterpret_cast<void*>(engine::image+0x6fcd330))return false;
    if(!PrepareOriginalReplicationClass(actualClass)||!Live(actor,actorIdentity)||!Live(manager,managerIdentity)
       ||Read<void*>(manager,0x10)!=actualClass||Read<void*>(manager,0xc8)!=actor
       ||Read<uint8_t>(actor,0x224)!=3||original_effects::GetWorld(actor)!=state.world
       ||original_effects::GetManager(actor,false)!=manager)return false;
    const auto replicated=Read<Array>(actor,0x330),registered=Read<Array>(actor,0x320);
    if(!ValidArray(replicated,8,4096)||!ValidArray(registered,0x28,4096))return false;
    int replicatedMatches=0,registeredMatches=0;
    for(int32_t n=0;n<replicated.count;++n)if(Read<void*>(replicated.data,size_t(n)*8)==manager)++replicatedMatches;
    for(int32_t n=0;n<registered.count;++n){auto at=registered.data+size_t(n)*0x28;
        if(Read<void*>(at)!=manager)continue;
        if(!Live(manager,Read<Weak>(at,8)))return false;++registeredMatches;
    }
    if(replicatedMatches==1&&registeredMatches==1){++state.routeReadbacks;return true;}
    if(!refresh||replicatedMatches>1||registeredMatches>1)return false;
    // Original constructor6FCCD70 requests true. Its setter returns early when
    // already true; this is the original owner registration refresh, not a flag
    // toggle or a hand-written registered-subobject list.
    engine::function<void(*)(void*,void*)>(0x3ee5170)(actor,manager);++state.routeRefreshes;
    return Route(actor,manager,false);
}
inline bool ManagerFor(const Source& s,void* actor,void*& manager){
    manager=nullptr;
    if(!s.used||!s.epoch||!s.registration||!HostWorld()||!original_players::IsA(actor,0x3683900)
       ||Read<uint8_t>(actor,0x224)!=3||original_effects::GetWorld(actor)!=state.world)return false;
    manager=original_effects::GetManager(actor,false);return manager&&Route(actor,manager,true);
}
inline bool ObserveChannels(void* actor){
    ++state.actorChannelSamples;bool observed=false;const auto peers=Read<Array>(state.driver,0xc8);
    if(!ValidArray(peers,8,32))return false;
    for(int32_t p=0;p<peers.count;++p){auto connection=Read<void*>(peers.data,size_t(p)*8);
        if(!connection||Read<void*>(connection,0x60)!=state.driver)continue;
        const auto channels=Read<Array>(connection,0x78);if(!ValidArray(channels,8,65536))continue;
        for(int32_t n=0;n<channels.count;++n){auto channel=Read<void*>(channels.data,size_t(n)*8);
            if(channel&&Read<void*>(channel,0x30)==connection&&Read<void*>(channel,0x78)==actor
               &&original_players::IsA(channel,0x3dd3d40)){observed=true;++state.actorChannelMatches;}
        }
    }return observed;
}
inline void CopyData(Data& to,const Data& from){engine::function<void*(*)(void*,const void*)>(0x64611d0)(&to,&from);++state.dataCopies;}
inline void DestroyData(Data& value){engine::function<void(*)(void*)>(0x6461100)(&value);++state.dataDestroys;}
inline void Submitted(void* actor,void* manager,void* function,void* parameters){
    const auto event=Read<Exec>(Read<void*>(manager),0x298);
    if(!event||!Route(actor,manager,false)){Fail(7606,"effect_authority_original_rpc_route_changed");RaiseException(0xe0007606U,EXCEPTION_NONCONTINUABLE,0,nullptr);return;}
    if(ObserveChannels(actor))++state.rpcWithNativeChannel;
    ++state.rpcCalls;event(manager,function,parameters);++state.rpcReturns;
    // A void ProcessEvent return proves dispatch returned, not transport or FXC
    // delivery. Client receive/render witnesses are deliberately separate.
}
inline bool ReadTimestamp(void* manager,float& result){
    auto world=engine::function<void*(*)(void*)>(0x2a64ee0)(manager);if(world!=state.world)return false;
    auto gs=Read<void*>(world,0x178);double now=0;
    if(gs){auto method=Read<void*>(Read<void*>(gs),0x770);if(!ImageCode(method))return false;
        now=reinterpret_cast<double(*)(void*)>(method)(gs);
    }else now=Read<double>(world,0x740);
    if(!std::isfinite(now)||!std::isfinite(static_cast<float>(now)))return false;result=static_cast<float>(now);return true;
}
inline void Require(bool condition,unsigned error,const char* stage){
    if(condition)return;Fail(error,stage);state.restartRequired=true;Cache();RaiseException(0xe0007600U+(error-7600U),EXCEPTION_NONCONTINUABLE,0,nullptr);
}
inline Handle NativePlay(Play& p){
    Data value{};CopyData(value,p.data);++state.calleeOwnedTransfers;alignas(16) Handle result{};
    using Call=Handle*(*)(Handle*,void*,void*,void*,Data*,void*,const engine::Vec*,const engine::Vec*,const Name*,uint8_t,uint32_t,uint8_t,uint8_t);
    engine::function<Call>(0x6fe3730)(&result,p.type,p.actor,p.context,&value,p.worldContext,&p.location,&p.rotation,&p.socket,p.local,p.alliance,p.cosmetic,p.replay);
    return result; // Original typed native owns/destructs its independent value.
}
inline void NativeUpdate(Update& p){
    Data value{};CopyData(value,p.data);++state.calleeOwnedTransfers;alignas(16) Handle id=p.id;
    // Both original execs copy24B to16-aligned scratch before typed calls;
    // their typed native paths use MOVAPS on the first16B of this parameter.
    engine::function<void(*)(void*,void*,const Handle*,Data*)>(0x6fe50e0)(p.actor,p.worldContext,&id,&value);
}
inline void NativeStop(Stop& p){alignas(16) Handle id=p.id;engine::function<void(*)(void*,void*,const Handle*,uint8_t)>(0x6fe5840)(p.actor,p.worldContext,&id,p.type);}
inline Handle ProducePlay(const Source& source,void* caller,Play& p){
    if(source.retiring||state.closing||!CurrentSource(source)){
        // Admission rejection, not successful effect creation. Zero is the
        // original invalid-ID representation; never mint an ID for a fenced
        // epoch. Execute still destroys its evaluated Data exactly once.
        ++state.rejectedRetiringPlay;state.stage="effect_authority_retiring_epoch_new_play_rejected_invalid_id";return {};
    }
    // Explicit probe admission rejection: original invalid ID, no queued/replayed effect.
    // Execute still destroys evaluated Data exactly once; old Stop cleanup stays native.
    if(local_server_empty_bootstrap_probe::RejectNewEffectPlay(source.index,source.pawn,source.epoch))return {};
    const auto id=NativePlay(p);if(!ValidID(id)||Local(id)){++state.typedForwards;return id;}
    // No synthetic ID, source name or returned flag. NativePlay is the actual
    // original ID generator even for the client-cropped nonlocal branch.
    Require(CurrentSource(source),7608,"effect_authority_play_source_changed_after_native_generation");
    void* manager=nullptr;
    const bool scope=ManagerFor(source,p.actor,manager)&&ResolveCaller(p.worldContext)
        &&ResolveCaller(p.worldContext)->registration==source.registration
        &&original_players::Derived(p.type,state.fxcType);
    if(!scope)++state.unsupportedPlay;
    Require(scope,7609,"effect_authority_nonlocal_attached_play_scope_unproven");
    Require(FindActive(manager,id)==-1,7610,"effect_authority_original_handle_already_present_or_array_invalid");
    Receipt* receipt=nullptr;Prune();for(auto& e:effects)if(!e.used){receipt=&e;break;}
    Require(receipt!=nullptr,7611,"effect_authority_live_effect_receipt_capacity_exhausted");
    Receipt value{};value.used=true;value.manager=manager;value.actor=p.actor;value.type=p.type;
    value.id=id;value.epoch=source.epoch;value.registration=source.registration;value.caller=caller;value.origin=source;
    Require(Capture(manager,value.managerId)&&Capture(p.actor,value.actorId)&&Capture(p.type,value.typeId)
        &&Capture(caller,value.callerId),7612,"effect_authority_original_manager_class_or_caller_weak_identity_missing");
    Active record{};engine::function<void*(*)(void*)>(0x6fccca0)(&record);
    __try{
        record.id=id;record.type=p.type;record.socket=p.socket;record.alliance=p.alliance;
        // AddEffectContext mutates only this independent original FEffectData;
        // the consumed NativePlay value already received the same operation.
        engine::function<void(*)(void*,void*)>(0x6fe9cf0)(&p.data,p.context);
        engine::function<void*(*)(void*,const void*)>(0x6468640)(&record.data,&p.data);
        auto quaternion=engine::function<void*(*)(const engine::Vec*,void*)>(0x19a51f0)(&p.rotation,record.transform.rotation);
        Require(quaternion==record.transform.rotation&&original_players::Finite(p.location),7613,"effect_authority_original_transform_not_constructed");
        record.transform.translation=p.location;
        Require(ReadTimestamp(manager,record.timestamp),7614,"effect_authority_original_movement_timestamp_unavailable");
        const auto before=Read<Array>(manager,0x118);
        const auto index=engine::function<int32_t(*)(void*,void*,const void*)>(0x4516300)(static_cast<uint8_t*>(manager)+0x118,state.arrayProperty,&record);
        const auto after=Read<Array>(manager,0x118);
        Require(ValidArray(after,sizeof(Active))&&index==before.count&&after.count==before.count+1
            &&FindActive(manager,id)==index&&Read<void*>(after.data,size_t(index)*sizeof(Active)+0x18)==p.type
            &&OriginalRecordIdentical(after.data+size_t(index)*sizeof(Active),&record),
            7615,"effect_authority_original_array_add_readback_failed");
        ++state.arrayReadbacks;*receipt=value;++state.effects;
        MulticastPlay rpc{};rpc.type=p.type;rpc.transform=record.transform;rpc.socket=p.socket;rpc.id=id;rpc.alliance=p.alliance;
        CopyData(rpc.data,record.data);
        __try{Submitted(p.actor,manager,state.rpcPlay,&rpc);}__finally{DestroyData(rpc.data);}
        Require(PostRpcReceipt(value),7616,"effect_authority_play_record_changed_during_original_multicast");
        ++state.plays;state.stage="effect_authority_original_handle_authority_record_and_multicast_submitted_delivery_unverified";
    }__finally{engine::function<void(*)(void*,void*)>(0x6742e30)(nullptr,&record);++state.dataDestroys;}
    return id;
}
inline bool CleanupOrigin(const Source& source,void* caller,const Receipt& receipt){
    if(CurrentSource(source)&&receipt.registration==source.registration)return true;
    return receipt.caller==caller&&Live(caller,receipt.callerId);
}
inline void ProduceUpdate(const Source& source,void* caller,Update& p){
    if(Local(p.id)||!ValidID(p.id)){++state.typedForwards;NativeUpdate(p);return;}
    auto receipt=FindActorReceipt(p.actor,p.id);
    if(!receipt||receipt->stopped)++state.unknownUpdates;
    Require(receipt&&!receipt->stopped&&receipt->actor==p.actor&&CleanupOrigin(source,caller,*receipt),7618,"effect_authority_update_has_no_owned_original_authority_handle");
    void* manager=nullptr;Require(ManagerFor(source,p.actor,manager)&&manager==receipt->manager,7617,"effect_authority_update_manager_or_source_unproven");
    const auto index=FindActive(manager,p.id);Require(index>=0,7619,"effect_authority_update_authority_record_missing");
    const auto a=Read<Array>(manager,0x118);auto record=a.data+size_t(index)*sizeof(Active);
    Require(Read<void*>(record,0x18)==receipt->type,7620,"effect_authority_update_original_record_class_changed");
    const auto owned=*receipt;Active expected{};
    engine::function<void*(*)(void*,const void*)>(0x693efa0)(&expected,record);
    __try{
        // Native key-wise merge preserves first-play context and untouched keys.
        // The independently owned expected copy and native reflected Identical
        // witness the actual destination; a void merge return is not a witness.
        engine::function<void(*)(void*,const void*)>(0x6fe9e80)(&expected.data,&p.data);
        engine::function<void(*)(void*,const void*)>(0x6fe9e80)(record+0x20,&p.data);
        Require(OriginalRecordIdentical(record,&expected),7620,"effect_authority_native_merge_data_readback_failed");
        MulticastUpdate rpc{};rpc.id=p.id;CopyData(rpc.data,p.data);
        __try{Submitted(p.actor,manager,state.rpcUpdate,&rpc);}__finally{DestroyData(rpc.data);}
        Require(PostRpcReceipt(owned),7621,"effect_authority_update_record_changed_during_original_multicast");
    }__finally{engine::function<void(*)(void*,void*)>(0x6742e30)(nullptr,&expected);++state.dataDestroys;}
    ++state.updates;++state.arrayReadbacks;state.stage="effect_authority_original_merge_and_multicast_update_submitted_delivery_unverified";
}
inline void ProduceStop(const Source& source,void* caller,Stop& p){
    if(Local(p.id)||!ValidID(p.id)){++state.typedForwards;NativeStop(p);return;}
    auto receipt=FindActorReceipt(p.actor,p.id);
    if(!receipt||receipt->stopped){++state.typedForwards;NativeStop(p);return;}
    Require(receipt->actor==p.actor&&CleanupOrigin(source,caller,*receipt),7623,"effect_authority_stop_origin_does_not_own_original_handle");
    void* manager=nullptr;Require(ManagerFor(source,p.actor,manager)&&manager==receipt->manager,7622,"effect_authority_stop_manager_or_source_unproven");
    const auto index=FindActive(manager,p.id);Require(index>=0,7623,"effect_authority_stop_authority_record_missing");
    // The original multicast receiver may itself remove118 if no Client128
    // record exists. Remove authority FIRST through its actual FArrayProperty;
    // never cache an index and remove again after a reentrant native RPC.
    engine::function<void(*)(void*,void*,int32_t)>(0x4516e40)(static_cast<uint8_t*>(manager)+0x118,state.arrayProperty,index);
    Require(FindActive(manager,p.id)==-1,7624,"effect_authority_original_array_remove_readback_failed");
    receipt->stopped=true;++state.arrayReadbacks;
    MulticastStop rpc{};rpc.id=p.id;rpc.type=p.type;Submitted(p.actor,manager,state.rpcStop,&rpc);
    ++state.stops;state.stage="effect_authority_original_array_remove_and_multicast_stop_submitted_delivery_unverified";
}

// Exact input evaluation shape of these three pinned exec wrappers. Every
// input is a value parameter. Compiled v110 takes the CONTAINER, not Locals+
// offset. Script GNatives execute side effects once. No consumed frame replay.
inline bool Step(void* frame,void* field,void* destination){
    auto code=Read<uint8_t*>(frame,0x20);
    if(code){
        if(!original_players::Readable(code,1))return false;const auto opcode=*code;
        auto native=Read<Exec>(reinterpret_cast<void*>(engine::image+0xd747c40),size_t(opcode)*8);
        if(!ImageCode(reinterpret_cast<void*>(native))||!Put(frame,0x20,code+1))return false;
        native(Read<void*>(frame,0x18),frame,destination);++state.scriptSteps;
    }else{
        if(Read<void*>(frame,0x88)!=field||!Put(frame,0x88,Read<void*>(field,0x18)))return false;
        const auto offset=Read<int32_t>(field,0x44),bytes=Read<int32_t>(field,0x34);auto locals=Read<uint8_t*>(frame,0x28);
        auto copy=Read<void*>(Read<void*>(field),0x110);
        if(!locals||offset<0||bytes<=0||bytes>88||!ImageCode(copy)
           ||!original_players::Readable(locals+offset,size_t(bytes))
           ||!Put<void*>(frame,0x40,locals)||!Put<void*>(frame,0x38,locals+offset))return false;
        reinterpret_cast<void(*)(void*,void*,const void*)>(copy)(field,destination,locals);++state.compiledSteps;
    }return true;
}
inline bool End(void* frame){
    auto code=Read<uint8_t*>(frame,0x20);return !code||(original_players::Readable(code,1)&&*code==0x16&&Put(frame,0x20,code+1));
}
inline bool DecodePlay(void* frame,Play& p){
    auto& f=functions[0];uint32_t local=0,cosmetic=0,replay=0;
    const bool ok=Step(frame,f.parameters[0],&p.type)&&Step(frame,f.parameters[1],&p.actor)
        &&Step(frame,f.parameters[2],&p.context)&&Step(frame,f.parameters[3],&p.data)
        &&Step(frame,f.parameters[4],&p.worldContext)&&Step(frame,f.parameters[5],&p.location)
        &&Step(frame,f.parameters[6],&p.rotation)&&Step(frame,f.parameters[7],&p.socket)
        &&Step(frame,f.parameters[8],&local)&&Step(frame,f.parameters[9],&p.alliance)
        &&Step(frame,f.parameters[10],&cosmetic)&&Step(frame,f.parameters[11],&replay)&&End(frame);
    p.local=local!=0;p.cosmetic=cosmetic!=0;p.replay=replay!=0;return ok;
}
inline bool DecodeUpdate(void* frame,Update& p){auto& f=functions[1];return Step(frame,f.parameters[0],&p.actor)
    &&Step(frame,f.parameters[1],&p.worldContext)&&Step(frame,f.parameters[2],&p.id)&&Step(frame,f.parameters[3],&p.data)&&End(frame);}
inline bool DecodeStop(void* frame,Stop& p){auto& f=functions[2];return Step(frame,f.parameters[0],&p.actor)
    &&Step(frame,f.parameters[1],&p.worldContext)&&Step(frame,f.parameters[2],&p.id)&&Step(frame,f.parameters[3],&p.type)&&End(frame);}
inline void Execute(size_t index,void* context,void* frame,void* result){
    auto& f=functions[index];++state.calls;
    auto source=state.installed&&(!state.closed||state.cleanupRetained)&&HostWorld()&&original_players::Readable(frame,0x90)
        ?ResolveCaller(Read<void*>(frame,0x18)):nullptr;
    if(!source){++state.originalForwards;f.original(context,frame,result);return;}
    const auto receipt=*source;const auto caller=Read<void*>(frame,0x18);
    Require(!state.error&&!state.restartRequired&&Live(f.object,f.identity)&&Read<Exec>(f.object,0xe0)==f.replacement
        &&Writable(frame,0x90)&&(index!=0||Writable(result,sizeof(Handle))),7625,"effect_authority_owned_native_call_binding_or_frame_changed");
    ++state.decodedCalls;
    if(index==0){Play p{};__try{
            Require(DecodePlay(frame,p),7626,"effect_authority_play_argument_evaluation_failed_no_replay");
            const auto id=ProducePlay(receipt,caller,p);std::memcpy(result,&id,sizeof(id));
        }__finally{DestroyData(p.data);}
    }else if(index==1){Update p{};__try{
            Require(DecodeUpdate(frame,p),7626,"effect_authority_update_argument_evaluation_failed_no_replay");ProduceUpdate(receipt,caller,p);
        }__finally{DestroyData(p.data);}
    }else{Stop p{};Require(DecodeStop(frame,p),7626,"effect_authority_stop_argument_evaluation_failed_no_replay");ProduceStop(receipt,caller,p);}
}
inline LONG ObserveException(EXCEPTION_POINTERS* e){
    state.restartRequired=true;state.exceptionCode=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0;
    const auto address=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=address>=engine::image?address-engine::image:address;
    state.stage="effect_authority_original_exception_propagated_requires_new_owned_pair";Cache();return EXCEPTION_CONTINUE_SEARCH;
}
inline void Entry(size_t index,void* context,void* frame,void* result){
    // Check the immutable recorded GT before the original helper: that helper
    // records an error for a wrong thread, which would mutate fallback state.
    if(!engine::image||!state.gameThread||state.gameThread!=GetCurrentThreadId()){
        functions[index].original(context,frame,result);return;
    }
    if(!Thread()){functions[index].original(context,frame,result);return;}
    ++state.active;__try{__try{Execute(index,context,frame,result);}__except(ObserveException(GetExceptionInformation())){}}
    __finally{--state.active;Cache();}
}
inline void PlayExec(void* context,void* frame,void* result){Entry(0,context,frame,result);}
inline void UpdateExec(void* context,void* frame,void* result){Entry(1,context,frame,result);}
inline void StopExec(void* context,void* frame,void* result){Entry(2,context,frame,result);}

struct Parameter {const wchar_t* name;int32_t offset,bytes;uint64_t flags;};
inline bool NativeMethod(void*& object,const wchar_t* path,void* owner,uint16_t bytes,uintptr_t exec,uint32_t required){
    object=engine::Find(path);const bool match=object&&Read<void*>(object,0x28)==owner&&Read<uint16_t>(object,0xbe)==bytes
        &&Read<uintptr_t>(object,0xe0)==engine::image+exec&&(Read<uint32_t>(object,0xb8)&required)==required;
    if(!match&&!state.metadata.observed){auto& m=state.metadata;m.observed=true;m.kind="function";m.name=path;m.object=object;
        m.expectedOwner=owner;m.actualOwner=Read<void*>(object,0x28);m.expectedBytes=bytes;m.parmsSize=Read<uint16_t>(object,0xbe);
        m.expectedExec=engine::image+exec;m.actualExec=Read<uintptr_t>(object,0xe0);m.expectedFlags=required;m.actualFlags=Read<uint32_t>(object,0xb8);
    }return match;
}
inline bool Parameters(void* function,const Parameter* schema,size_t count){
    for(size_t n=0;n<count;++n){const auto& p=schema[n];if(!Field(function,p.name,p.offset,p.bytes,nullptr,p.flags))return false;}
    return true;
}
inline bool Bind(size_t index,const wchar_t* path,uint16_t bytes,uintptr_t exec,Exec replacement,const Parameter* schema,size_t count){
    auto& f=functions[index];auto owner=engine::Find(L"/Script/ShooterGame.EffectsBlueprintLibrary");
    if(count>f.parameters.size()||!NativeMethod(f.object,path,owner,bytes,exec,0x400)
       ||!Writable(static_cast<uint8_t*>(f.object)+0xe0,8)||!Capture(f.object,f.identity))return false;
    f.original=engine::function<Exec>(exec);f.replacement=replacement;
    for(size_t n=0;n<count;++n){const auto& p=schema[n];
        if(!Field(f.object,p.name,p.offset,p.bytes,&f.parameters[n],p.flags))return false;
        if(n+1<count&&(p.flags&(0x100ULL|0x8000000ULL)))return false;
    }f.inputs=uint8_t(count);return true;
}
inline bool VerifySchema(){
    state.managerType=engine::Class(original_effects::ManagerClass);state.componentType=engine::Find(L"/Script/Engine.ActorComponent");
    state.fxcType=engine::Class(0x648ada0);state.recordType=engine::Class(0x673ea40);
    state.dataType=engine::Class(0x6753fe0);state.idType=engine::Class(0x67535c0);
    if(!state.managerType||!state.componentType||!state.fxcType||Read<int32_t>(state.recordType,0x60)!=0x110
       ||Read<int32_t>(state.dataType,0x60)!=88||Read<int32_t>(state.idType,0x60)!=24
       ||!Field(state.managerType,L"ServerActiveEffects",0x118,16,&state.arrayProperty,0x40000000000220ULL)
       )return false;
    auto inner=Read<void*>(state.arrayProperty,0x80);
    if(!inner||Read<int32_t>(inner,0x34)!=0x110||Read<void*>(inner,0x78)!=state.recordType
       ||!Field(state.recordType,L"EffectID",0,24)||!Field(state.recordType,L"EffectType",0x18,8)
       ||!Field(state.recordType,L"EffectData",0x20,88)||!Field(state.recordType,L"Transform",0x80,96)
       ||!Field(state.recordType,L"Socket",0xe0,12)||!Field(state.recordType,L"StartTimeStamp",0xec,4)
       ||!Field(state.recordType,L"EffectContainer",0xf0,8)||!Field(state.recordType,L"ProxyTimerHandle",0xf8,8)
       ||!Field(state.recordType,L"AllianceFilter",0x100,1))return false;
    const Parameter play[]{{L"EffectContainer",0,8,0x1c001000000280ULL},{L"AttachToActor",8,8,0x18001000000280ULL},{L"EffectContext",16,8,0x18001000000280ULL},
        {L"EffectData",24,88,0x10000000000080ULL},{L"WorldContextObject",112,8,0x18001000000280ULL},{L"Location",120,24,0x18001040000280ULL},
        {L"Rotation",144,24,0x10001040000280ULL},{L"AttachSocket",168,12,0x18001040000280ULL},{L"bLocalOnly",180,1,0x18041040000280ULL},
        {L"AllianceFilter",181,1,0x18041040000280ULL},{L"bIsCosmetic",182,1,0x18041040000280ULL},{L"bShouldRecordToReplay",183,1,0x18041040000280ULL},{L"ReturnValue",184,24,0x10001000000780ULL}};
    const Parameter update[]{{L"AttachToActor",0,8,0x18001000000280ULL},{L"WorldContextObject",8,8,0x18001000000280ULL},{L"EffectID",16,24,0x10001000000280ULL},{L"EffectData",40,88,0x10000000000080ULL}};
    const Parameter stop[]{{L"AttachToActor",0,8,0x18001000000280ULL},{L"WorldContextObject",8,8,0x18001000000280ULL},{L"EffectID",16,24,0x10001000000280ULL},{L"StopEffectType",40,1,0x18001040000280ULL}};
    if(!Bind(0,L"/Script/ShooterGame.EffectsBlueprintLibrary.PlayContinuousEffect",208,0x67511f0,&PlayExec,play,std::size(play))
       ||!Bind(1,L"/Script/ShooterGame.EffectsBlueprintLibrary.UpdateContinuousEffect",128,0x6750bb0,&UpdateExec,update,std::size(update))
       ||!Bind(2,L"/Script/ShooterGame.EffectsBlueprintLibrary.StopContinuousEffect",41,0x67506a0,&StopExec,stop,std::size(stop)))return false;
    if(!(NativeMethod(state.rpcPlay,L"/Script/ShooterGame.EffectManagerComponent.MulticastPlayContinuousEffect",state.managerType,245,0x673fc80,0x4440)
        &&NativeMethod(state.rpcUpdate,L"/Script/ShooterGame.EffectManagerComponent.MulticastUpdateContinuousEffect",state.managerType,120,0x673f850,0x4440)
        &&NativeMethod(state.rpcStop,L"/Script/ShooterGame.EffectManagerComponent.MulticastStopContinuousEffect",state.managerType,40,0x673f560,0x4440)))return false;
    const Parameter multicastPlay[]{{L"EffectContainer",0,8,0x1c001000000280ULL},{L"EffectData",8,88,0x10000008000082ULL},{L"Transform",96,96,0x18001040000080ULL},
        {L"AttachSocket",192,12,0x18001040000280ULL},{L"EffectID",208,24,0x10001000000280ULL},{L"ClientControllerThatTriggered",232,8,0x18001000000280ULL},
        {L"StartMovementTime",240,4,0x18001040000280ULL},{L"AllianceFilter",244,1,0x18001040000280ULL}};
    const Parameter multicastUpdate[]{{L"EffectID",0,24,0x10001000000280ULL},{L"EffectData",24,88,0x10000008000082ULL},{L"ClientControllerThatTriggered",112,8,0x18001000000280ULL}};
    const Parameter multicastStop[]{{L"EffectID",0,24,0x10001000000280ULL},{L"ClientControllerThatTriggered",24,8,0x18001000000280ULL},
        {L"StopEffectType",32,1,0x18001040000280ULL},{L"StopMovementTime",36,4,0x18001040000280ULL}};
    if(!(Parameters(state.rpcPlay,multicastPlay,std::size(multicastPlay))
        &&Parameters(state.rpcUpdate,multicastUpdate,std::size(multicastUpdate))
        &&Parameters(state.rpcStop,multicastStop,std::size(multicastStop))))return false;
    // All six UF parameter schemas are established before original class setup.
    return PrepareOriginalReplicationClass(state.managerType);
}
inline bool Initialize(void* world){
    if(state.initialized)return state.world==world&&state.installed&&Thread()&&!state.error;
    if(!engine::image||!original_players::Thread()||!world||world!=local_server_match::GetState().world)
        return Fail(7627,"effect_authority_requires_actual_match_host_world");
    state.world=world;state.driver=Read<void*>(world,0x40);state.gameThread=GetCurrentThreadId();
    if(!Capture(world,state.worldId)||!Capture(state.driver,state.driverId)||!HostWorld())return Fail(7627,"effect_authority_host_driver_weak_identity_missing");
    state.hostVerified=true;
    for(const auto& p:local_server_effect_authority_source::Profiles){
        auto data=reinterpret_cast<const uint8_t*>(engine::image+p.rva);uint64_t hash=14695981039346656037ULL;
        if(!ImageCode(data)||!original_players::Readable(data,p.bytes))return Fail(7628,"effect_authority_pinned_source_profile_unreadable");
        for(uint32_t n=0;n<p.bytes;++n)hash=(hash^data[n])*1099511628211ULL;
        if(hash!=p.hash)return Fail(7628,"effect_authority_pinned_source_profile_mismatch");
    }
    // Same source gate as the generated profiles, for the original UClass API.
    {constexpr uintptr_t rva=0x1b979c0;constexpr uint32_t bytes=1033;constexpr uint64_t expected=0x16a648ec490bf709ULL;
        auto data=reinterpret_cast<const uint8_t*>(engine::image+rva);uint64_t hash=14695981039346656037ULL;
        if(!ImageCode(data)||!original_players::Readable(data,bytes))return Fail(7628,"effect_authority_original_runtime_setup_source_unreadable");
        for(uint32_t n=0;n<bytes;++n)hash=(hash^data[n])*1099511628211ULL;
        if(hash!=expected)return Fail(7628,"effect_authority_original_runtime_setup_source_profile_mismatch");
    }state.profilesVerified=true;
    if(!VerifySchema())return state.error?false:Fail(7629,"effect_authority_original_reflection_schema_mismatch");state.schemaVerified=true;
    state.initialized=true;
    for(auto& f:functions){
        if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(static_cast<uint8_t*>(f.object)+0xe0),
            reinterpret_cast<void*>(f.replacement),reinterpret_cast<void*>(f.original))!=reinterpret_cast<void*>(f.original)){
            ++state.foreignBindings;return Fail(7630,"effect_authority_native_exec_publication_conflict");
        }f.published=true;
    }state.installed=true;state.stage="effect_authority_original_native_boundaries_installed_not_runtime_verified";Cache();return true;
}
inline bool Close(){
    ++state.closeCalls;if(!state.initialized){state.closed=true;Cache();return true;}
    if(!Thread())return Fail(7631,"effect_authority_close_requires_original_game_thread");
    if(state.active){state.stage="effect_authority_close_waiting_current_native_callback";Cache();return false;}
    state.closing=true;
    // Root must stop new match/streak producers and run native Reset/retirement
    // before this final restore. Published callbacks retain original parents;
    // no UObject array, surviving FXC or timer is destroyed by adapter Close.
    for(auto& s:sources)if(s.used&&!s.retiring&&SourceBase(s)){
        state.stage="effect_authority_close_waiting_source_epoch_fence";Cache();return false;
    }
    bool retain=false;
    for(const auto& e:effects)if(e.used&&!e.stopped){
        // Weak null includes pending-kill. It cannot prove native teardown or
        // that the published world will never execute another cleanup callback.
        if(!Live(e.manager,e.managerId)||!Live(e.actor,e.actorId)){retain=true;++state.closeUnknownLifetimes;continue;}
        if(FindActive(e.manager,e.id)!=-1)retain=true;
    }
    if(retain){
        state.cleanupRetained=true;state.execsRestored=false;state.closed=true;
        state.stage="effect_authority_producers_closed_static_cleanup_execs_retained_until_process_exit";Cache();return true;
    }
    bool restored=true;
    for(auto& f:functions)if(f.published){
        if(!Live(f.object,f.identity)){++state.foreignBindings;restored=false;continue;}
        auto current=Read<Exec>(f.object,0xe0);
        if(current==f.original){f.published=false;continue;}
        if(current!=f.replacement){++state.foreignBindings;restored=false;continue;}
        if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(static_cast<uint8_t*>(f.object)+0xe0),
            reinterpret_cast<void*>(f.original),reinterpret_cast<void*>(f.replacement))==reinterpret_cast<void*>(f.replacement)){
            f.published=false;++state.restoredBindings;
        }else{++state.foreignBindings;restored=false;}
    }
    if(!restored)return Fail(7633,"effect_authority_foreign_native_layer_or_pending_kill_binding_retained");
    state.installed=false;state.closed=true;state.cleanupRetained=false;state.execsRestored=true;
    state.stage="effect_authority_original_execs_restored_original_effect_lifetimes_retained";Cache();return true;
}
template<class Visit> inline bool VisitReplicationGraph(Visit&& visit){
    if(!state.initialized||state.closed)return !state.error;
    if(!Thread()||!HostWorld())return false;++state.registryVisits;Prune();
    for(const auto& e:effects)if(e.used&&!e.stopped&&Live(e.manager,e.managerId)&&Live(e.actor,e.actorId)){
        if(!Route(e.actor,e.manager,false))return Fail(7634,"effect_authority_existing_native_component_replication_route_changed");
        visit(e.actor,e.manager);
    }Cache();return true;
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path||!*path)return false;const auto s=Snapshot();FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;
    const auto b=[](bool v){return v?"true":"false";};
    std::fprintf(file,"{\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"installed\":%s,\"closing\":%s,\"closed\":%s,\"restart_required\":%s,\"profiles_verified\":%s,\"schema_verified\":%s,\"host_verified\":%s,\"world\":\"%p\",\"active\":%u,\"sources\":%u,\"effect_receipts\":%u,\"calls\":%llu,\"original_forwards\":%llu,\"typed_forwards\":%llu,\"decoded_calls\":%llu,\"plays\":%llu,\"updates\":%llu,\"stops\":%llu,\"retirements\":%llu,\"route_refreshes\":%llu,\"route_readbacks\":%llu,\"array_readbacks\":%llu,\"rpc_calls\":%llu,\"rpc_returns\":%llu,\"rpc_with_native_actor_channel\":%llu,\"actor_channel_matches\":%llu,\"data_copies\":%llu,\"data_destroys\":%llu,\"callee_owned_value_transfers\":%llu,\"exception_code\":%u,\"exception_rva\":\"0x%llx\",\"original_image_writes\":0,\"handle_flag_writes\":0,\"role_writes\":0,\"replication_flag_writes\":0,\"replication_condition\":\"original_initial_only\",\"dynamic_code\":false,\"direct_cpp_callers_covered\":false,\"rpc_dispatch_return_is_delivery\":false,\"client_receive_modified\":false,\"network_delivery_verified\":false,\"runtime_verified\":false,",
        s.stage,s.error,b(s.initialized),b(s.installed),b(s.closing),b(s.closed),b(s.restartRequired),b(s.profilesVerified),b(s.schemaVerified),b(s.hostVerified),s.world,s.active,s.sources,s.effects,
        static_cast<unsigned long long>(s.calls),static_cast<unsigned long long>(s.originalForwards),static_cast<unsigned long long>(s.typedForwards),static_cast<unsigned long long>(s.decodedCalls),
        static_cast<unsigned long long>(s.plays),static_cast<unsigned long long>(s.updates),static_cast<unsigned long long>(s.stops),static_cast<unsigned long long>(s.retirements),
        static_cast<unsigned long long>(s.routeRefreshes),static_cast<unsigned long long>(s.routeReadbacks),static_cast<unsigned long long>(s.arrayReadbacks),static_cast<unsigned long long>(s.rpcCalls),static_cast<unsigned long long>(s.rpcReturns),
        static_cast<unsigned long long>(s.rpcWithNativeChannel),static_cast<unsigned long long>(s.actorChannelMatches),static_cast<unsigned long long>(s.dataCopies),static_cast<unsigned long long>(s.dataDestroys),
        static_cast<unsigned long long>(s.calleeOwnedTransfers),s.exceptionCode,static_cast<unsigned long long>(s.exceptionRva));
    char name[384]{};if(s.metadata.name)WideCharToMultiByte(CP_UTF8,0,s.metadata.name,-1,name,int(sizeof(name)),nullptr,nullptr);
    const auto& m=s.metadata;
    std::fprintf(file,"\"runtime_replication_setup\":{\"calls\":%llu,\"readbacks\":%llu,\"class\":\"%p\",\"flags_before\":\"0x%x\",\"flags_after\":\"0x%x\",\"property_rep_index\":%u,\"class_reps_count\":%d},",
        static_cast<unsigned long long>(s.replicationSetupCalls),static_cast<unsigned long long>(s.replicationSetupReadbacks),s.lastReplicationClass,
        s.replicationFlagsBefore,s.replicationFlagsAfter,unsigned(s.replicationPropertyIndex),s.replicationClassReps);
    std::fprintf(file,"\"cleanup_retained\":%s,\"execs_restored\":%s,\"reentrant_stops\":%llu,\"native_lifetime_ended_during_rpc\":%llu,\"close_unknown_lifetimes\":%llu,\"metadata_failure\":{\"observed\":%s,\"kind\":\"%s\",\"expected_name\":\"%s\",\"object\":\"%p\",\"expected_owner\":\"%p\",\"actual_owner\":\"%p\",\"expected_exec\":\"0x%llx\",\"actual_exec\":\"0x%llx\",\"field\":\"%p\",\"field_class\":\"%p\",\"expected_offset\":%d,\"actual_offset\":%d,\"expected_bytes\":%d,\"actual_bytes\":%d,\"actual_parms_size\":%u,\"array_dim\":%d,\"expected_flags\":\"0x%llx\",\"actual_flags\":\"0x%llx\",\"expected_name_bits\":\"0x%llx\",\"actual_name_bits\":\"0x%llx\"}}\n",
        b(s.cleanupRetained),b(s.execsRestored),static_cast<unsigned long long>(s.reentrantStops),
        static_cast<unsigned long long>(s.nativeLifetimeEndedDuringRpc),static_cast<unsigned long long>(s.closeUnknownLifetimes),
        b(m.observed),m.kind,name,m.object,m.expectedOwner,m.actualOwner,
        static_cast<unsigned long long>(m.expectedExec),static_cast<unsigned long long>(m.actualExec),m.field,m.fieldClass,
        m.expectedOffset,m.actualOffset,m.expectedBytes,m.actualBytes,unsigned(m.parmsSize),m.arrayDim,
        static_cast<unsigned long long>(m.expectedFlags),static_cast<unsigned long long>(m.actualFlags),
        static_cast<unsigned long long>(m.expectedName),static_cast<unsigned long long>(m.actualName));
    const bool ok=std::ferror(file)==0;return std::fclose(file)==0&&ok;
}
} // namespace local_server_effect_authority
