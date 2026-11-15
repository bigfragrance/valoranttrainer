#pragma once
#include "local_server_hero_authority.h"
#include "local_server_hero_skill_readback.h"
#include "local_server_phoenix_c_birth_data.h"
#include "local_server_phoenix_c_birth_profiles.h"

// Passive, HOST-only Phoenix C native lifecycle observation. No factory,
// original image/flag/input/resource/actor publication or Tick is changed.
// Only actual existing UFunction.E0 cells are installed/restored. Capture of
// native Weak identities can assign an original serial and is counted below.
namespace local_server_phoenix_c_native_observer {
namespace data=local_server_phoenix_c_birth_data;
namespace fw=local_server_hero_abilities;
using View=fw::SourceView;using Weak=original_inventory_slots::WeakIdentity;
using Exec=data::Exec;using Frame=data::Frame;using Stack=data::Stack;
using CaptureContext=void(WINAPI*)(PCONTEXT);
using Lookup=PRUNTIME_FUNCTION(WINAPI*)(DWORD64,PDWORD64,PUNWIND_HISTORY_TABLE);
using Unwind=PEXCEPTION_ROUTINE(WINAPI*)(DWORD,DWORD64,DWORD64,PRUNTIME_FUNCTION,PCONTEXT,PVOID*,PDWORD64,PKNONVOLATILE_CONTEXT_POINTERS);
inline constexpr size_t MaxFunctions=12,MaxProducers=4,MaxReceipts=64,MaxFrames=96;
inline constexpr wchar_t AbilityPath[]=L"/Game/Characters/Phoenix/S0/Ability_Q/Production/Ability_Phoenix_Q_FireballWall_Production.Ability_Phoenix_Q_FireballWall_Production_C";
enum class Purpose:uint8_t {BeginPlay,ThrowResult,PoolInitialize};
struct Array {void** data=nullptr;int32_t count=0,capacity=0;};
static_assert(sizeof(Array)==16);
struct Function {
    void *object=nullptr,*owner=nullptr;Weak id{},ownerId{};Exec original=nullptr,replacement=nullptr;
    uint32_t flags=0;uint16_t bytes=0;Purpose purpose=Purpose::BeginPlay;bool installed=false;
};
struct Producer {
    bool used=false,retired=false;View view{};void *component=nullptr,*type=nullptr,*target=nullptr,*postFunction=nullptr;
    void *beginSource=nullptr,*beginDispatch=nullptr,*postDispatch=nullptr;
    Weak componentId{},typeId{},targetId{},postId{},beginSourceId{},beginDispatchId{};
    int32_t beginEntry=-1,delegateIndex=-1,delegateCount=0;uint64_t lastDiscoverAt=0;
};
struct Receipt {
    uint64_t serial=0,at=0;View source{};void *actor=nullptr,*type=nullptr,*component=nullptr,*owner=nullptr,*instigator=nullptr,*firingPS=nullptr;
    Weak actorId{},typeId{},componentId{},ownerId{},instigatorId{},firingPSId{};
    uintptr_t nativeRip=0,nativeRsp=0,outerRsp=0;uint32_t initializedBits=0;
    data::Origin origin=data::Origin::Unknown;bool valid=false,originalCompleted=false,sourceStableAfter=false;
    void *sourceFunction=nullptr,*dispatchFunction=nullptr;bool redirected=false,entryKnown=false;int32_t graphEntry=0;
    bool originalReplicates=false;uint8_t role=0;
};
struct State {
    bool initialized=false,closed=false,closing=false,busy=false,profiles=false,schema=false;
    void* world=nullptr;Weak worldId{};DWORD thread=0;const char* stage="phoenix_c_birth_not_initialized";
    uint64_t polls=0,skipped=0,installs=0,restores=0,captures=0,getterCalls=0,serial=0;
    uint64_t fresh=0,pool=0,resultEvents=0,resultActors=0;
    uint64_t sampleFaults=0,invalidSamples=0,unwindMisses=0,retirements=0;uint32_t lastSampleException=0;
    uint64_t frameHeaderSamples=0,frameMetadataBytes=0,graphEntrySamples=0,redirectedSamples=0;
    uint64_t lastMetadataAt=0;void* lastMetadataPawn=nullptr;uint32_t lastMetadataEpoch=0;
};
struct Snapshot {State state{};std::array<Receipt,MaxReceipts> receipts{};std::array<Producer,MaxProducers> producers{};};
inline State state{};inline Snapshot cached{};inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline std::array<Function,MaxFunctions> functions{};inline std::array<Producer,MaxProducers> producers{};
inline std::array<Receipt,MaxReceipts> receipts{};
inline CaptureContext captureContext=nullptr;inline Lookup lookup=nullptr;inline Unwind unwind=nullptr;
inline void *abilityType=nullptr,*throwType=nullptr,*projectileType=nullptr,*beginName=nullptr,*classGetter=nullptr;
inline original_jett_priming::PropertyInfo replicates{};
inline volatile LONG64 entries=0,forwards=0,completed=0,bypassed=0;inline volatile LONG activeCallbacks=0;
inline const State& GetState(){return state;}
template<class T>inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void* object,const Weak& id){return object&&original_inventory_slots::ResolveIdentity(id)==object;}
inline bool Same(const Weak& a,const Weak& b){return original_inventory_slots::SameIdentity(a,b);}
inline bool Capture(void* object,Weak& id){++state.captures;return original_inventory_slots::CaptureIdentity(object,id)&&Live(object,id);}
inline bool Thread(){return state.thread&&state.thread==GetCurrentThreadId()&&original_players::Thread();}
inline void Cache(){AcquireSRWLockExclusive(&cacheLock);cached={state,receipts,producers};ReleaseSRWLockExclusive(&cacheLock);}
inline bool Writable(const void* pointer){MEMORY_BASIC_INFORMATION info{};if(!pointer||!VirtualQuery(pointer,&info,sizeof(info)))return false;
    const auto at=reinterpret_cast<uintptr_t>(pointer),start=reinterpret_cast<uintptr_t>(info.BaseAddress);const auto p=info.Protect&255U;
    return info.State==MEM_COMMIT&&info.Type==MEM_PRIVATE&&!(info.Protect&(PAGE_GUARD|PAGE_NOACCESS))
        &&(p==PAGE_READWRITE||p==PAGE_EXECUTE_READWRITE)&&at>=start&&info.RegionSize>=8&&at-start<=info.RegionSize-8;}
inline bool Type(void* object,void* type){return object&&type&&original_players::Derived(Read<void*>(object,0x10),type);}
inline uint64_t Hash(const void* p,size_t count){auto bytes=static_cast<const uint8_t*>(p);uint64_t h=0xcbf29ce484222325ULL;
    for(size_t n=0;n<count;++n)h=(h^bytes[n])*0x100000001b3ULL;return h;}
inline bool Current(const View& view){return Thread()&&state.initialized&&!state.closed&&!state.closing
    &&view.world==state.world&&Live(state.world,state.worldId)&&!view.pendingGrant&&view.ability
    &&Read<void*>(view.ability,0x10)==abilityType&&Read<uint8_t>(view.ability,0x4a0)==3
    // Use the original pure predicates under THIS observer's diagnostic SEH
    // boundary. The public wrapper turns sampling faults into gameplay errors.
    &&local_server_hero_authority::ValidateHostSourceImpl(view);}
inline bool ProducerCurrent(const Producer& p){
    if(!p.used||p.retired||!Current(p.view)||!Live(p.component,p.componentId)||(p.type&&!Live(p.type,p.typeId))
       ||Read<void*>(p.component,0xc8)!=p.view.ability||!Type(p.component,throwType))return false;
    View now{};return fw::ResolveSource(p.component,now)&&fw::ValidateSource(now)
        &&local_server_hero_authority::Fields(now,p.view)&&local_server_hero_authority::WeakFields(now,p.view);
}
inline bool OriginalEvent(void* object,void* function,void* params){
    const auto method=Read<void*>(Read<void*>(object),0x298);
    if(method!=reinterpret_cast<void*>(engine::image+0x1cdc850))return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(method)(object,function,params);return true;
}
inline Stack CurrentStack(){const auto tib=reinterpret_cast<const NT_TIB*>(NtCurrentTeb());
    return {reinterpret_cast<uintptr_t>(tib->StackLimit),reinterpret_cast<uintptr_t>(tib->StackBase)};}
inline bool StackRead(const Stack& s,uintptr_t at,size_t bytes){return data::InStack(s,at,bytes)
    &&original_players::Readable(reinterpret_cast<void*>(at),bytes);}
inline bool Step(CONTEXT& c,const Stack& stack){
    if(!StackRead(stack,c.Rsp,8))return false;const auto before=c.Rsp;DWORD64 image=0;
    const auto entry=lookup(c.Rip,&image,nullptr);
    if(entry){PVOID handler=nullptr;DWORD64 establisher=0;
        unwind(UNW_FLAG_NHANDLER,image,c.Rip,entry,&c,&handler,&establisher,nullptr);
    }else{c.Rip=Read<DWORD64>(reinterpret_cast<void*>(c.Rsp));c.Rsp+=8;}
    return c.Rip&&c.Rsp>before&&c.Rsp<=stack.high;
}
struct Scan {std::array<Frame,MaxFrames> rows{};size_t count=0;Stack stack{};};
inline Scan ScanFrames(){Scan s{};s.stack=CurrentStack();CONTEXT c{};captureContext(&c);
    for(size_t n=0;n<MaxFrames&&c.Rip;++n){s.rows[s.count++]={c.Rip,c.Rsp,c.Rbp,c.Rbx,c.Rsi,c.Rdi,c.R12,c.R13,c.R14,c.R15};
        if(!Step(c,s.stack))break;}return s;}
inline Producer* FindProducer(void* component){for(auto& p:producers)if(p.used&&p.component==component&&ProducerCurrent(p))return &p;return nullptr;}
inline bool Field(void* type,const wchar_t* name,int32_t offset,int32_t bytes);
inline bool Incoming(void* rawFunction,void* actualFunction,void* frame,void* receiver,int32_t expectedEntry,Receipt& r){
    if(!rawFunction||!actualFunction||!original_players::Readable(frame,0x30))return false;
    auto graph=Read<void*>(rawFunction,0xd0);const bool redirected=graph!=nullptr;
    if((redirected?graph:rawFunction)!=actualFunction)return false;
    ++state.frameHeaderSamples;state.frameMetadataBytes+=16;
    const auto node=Read<void*>(frame,0x10),object=Read<void*>(frame,0x18);
    bool known=false;int32_t entry=0;
    if(redirected){
        if(Read<int32_t>(rawFunction,0xd8)!=expectedEntry||Read<uint16_t>(actualFunction,0xbe)!=4
           ||!Field(actualFunction,L"EntryPoint",0,4))return false;
        const auto locals=Read<void*>(frame,0x28);state.frameMetadataBytes+=8;
        if(!original_players::Readable(locals,4))return false;
        entry=Read<int32_t>(locals);state.frameMetadataBytes+=4;++state.graphEntrySamples;known=true;
    }
    if(!data::IncomingMatches(reinterpret_cast<uintptr_t>(node),reinterpret_cast<uintptr_t>(object),
        reinterpret_cast<uintptr_t>(actualFunction),reinterpret_cast<uintptr_t>(receiver),redirected,known,entry,expectedEntry))return false;
    r.sourceFunction=rawFunction;r.dispatchFunction=actualFunction;r.redirected=redirected;r.entryKnown=known;r.graphEntry=entry;
    if(redirected)++state.redirectedSamples;return true;
}
inline bool ProcessEventFrame(const Scan& s,size_t before,void* receiver,void* function){
    for(size_t n=0;n<before;++n){const auto& f=s.rows[n];
        if(f.rip==engine::image+0x1cdccce&&f.rsi==reinterpret_cast<uintptr_t>(function)
           &&f.r15==reinterpret_cast<uintptr_t>(receiver))return true;
    }return false;
}
inline bool ActorFacts(Receipt& r,const Producer& p){
    r.valid=false;if(!ProducerCurrent(p)||!Type(r.actor,projectileType)||Read<void*>(r.actor,0x10)!=r.type)return false;
    r.source=p.view;r.component=p.component;r.componentId=p.componentId;r.role=Read<uint8_t>(r.actor,0x224);
    r.owner=Read<void*>(r.actor,0x210);r.instigator=Read<void*>(r.actor,0x278);
    r.firingPSId=Read<Weak>(r.actor,0x5b8);r.firingPS=original_inventory_slots::ResolveIdentity(r.firingPSId);
    r.ownerId=p.view.identities[6];r.instigatorId=p.view.identities[4];
    if(r.role!=3||r.owner!=p.view.ability||r.instigator!=p.view.pawn||r.firingPS!=p.view.playerState
       ||!Same(r.firingPSId,p.view.identities[3])||local_server_item_ownership::ActorWorld(r.actor)!=state.world
       ||!Live(r.owner,r.ownerId)||!Live(r.instigator,r.instigatorId)||!Capture(r.actor,r.actorId)||!Capture(r.type,r.typeId))return false;
    r.initializedBits=Read<uint32_t>(r.actor,0x518);
    r.originalReplicates=(Read<uint8_t>(r.actor,size_t(replicates.offset)+replicates.boolByteOffset)&replicates.boolFieldMask)!=0;
    r.valid=ProducerCurrent(p)&&Live(r.actor,r.actorId)&&Live(r.type,r.typeId)&&Read<void*>(r.actor,0x10)==r.type;return r.valid;
}
inline void Store(Receipt& r){if(!r.valid||state.serial==UINT64_MAX)return;r.serial=++state.serial;r.at=GetTickCount64();receipts[(r.serial-1)%receipts.size()]=r;}
inline bool ObserveInit(void* actor,void* function,void* frame,bool pooled,Receipt& r){
    if(!original_players::Readable(frame,0x30))return false;
    const auto scan=ScanFrames();
    for(size_t n=0;n<scan.count;++n){const size_t common=pooled?n+1:n;
        if(common+3>=scan.count)break;const auto& native=scan.rows[common];const auto& outer=scan.rows[common+3];
        if(native.rip!=engine::image+(pooled?0x78cf00d:0x78cf16a))continue;
        if(!data::AddEquals(outer.rsp,0x130,outer.rbp)||!StackRead(scan.stack,outer.rsp,0x340))continue;
        const auto type=Read<void*>(reinterpret_cast<void*>(outer.rbp+0x208));
        const auto owner=Read<void*>(reinterpret_cast<void*>(outer.rbp+0x1f8));
        const bool frames=pooled?data::PoolFrames(scan.rows[n],native,scan.rows[common+1],scan.rows[common+2],outer,
            engine::image,scan.stack,reinterpret_cast<uintptr_t>(actor),reinterpret_cast<uintptr_t>(type),reinterpret_cast<uintptr_t>(owner))
            :data::FreshFrames(native,scan.rows[common+1],scan.rows[common+2],outer,
            engine::image,scan.stack,reinterpret_cast<uintptr_t>(actor),reinterpret_cast<uintptr_t>(type),reinterpret_cast<uintptr_t>(owner));
        if(!frames)continue;auto p=FindProducer(reinterpret_cast<void*>(outer.rsi));if(!p||p->type!=type||owner!=p->view.ability)continue;
        if(!ProcessEventFrame(scan,common,actor,function))continue;
        r={};r.actor=actor;r.type=type;r.nativeRip=native.rip;r.nativeRsp=native.rsp;r.outerRsp=outer.rsp;
        if(!pooled){if(p->beginDispatch!=function||!Live(p->beginSource,p->beginSourceId)||!Live(p->beginDispatch,p->beginDispatchId)
            ||Read<void*>(p->beginSource,0x28)!=p->type||Read<uint16_t>(p->beginSource,0xbe)!=0
            ||Read<uint32_t>(p->beginSource,0xb8)!=0x08080800U||Read<int32_t>(p->beginSource,0x70)!=18
            ||!Incoming(p->beginSource,function,frame,actor,p->beginEntry,r))continue;}
        else{if(Read<void*>(frame,0x10)!=function||Read<void*>(frame,0x18)!=actor)continue;
            ++state.frameHeaderSamples;state.frameMetadataBytes+=16;r.sourceFunction=function;r.dispatchFunction=function;}
        r.origin=pooled?data::Origin::PoolPending:data::Origin::FreshPending;
        if(ActorFacts(r,*p)){Store(r);if(pooled)++state.pool;else ++state.fresh;return true;}
    }++state.unwindMisses;return false;
}
inline bool ValidArray(const Array& a){return data::ArrayShape(reinterpret_cast<uintptr_t>(a.data),a.count,a.capacity,64)
    &&(!a.count||original_players::Readable(a.data,static_cast<size_t>(a.count)*8));}
inline bool DelegateEntry(const Producer& p,uintptr_t entry){
    if(!original_players::Readable(reinterpret_cast<void*>(entry),20)||!Live(p.target,p.targetId)||!Live(p.postFunction,p.postId))return false;
    return Same(Read<Weak>(reinterpret_cast<void*>(entry)),p.targetId)
        &&std::memcmp(reinterpret_cast<void*>(entry+8),static_cast<uint8_t*>(p.postFunction)+0x18,12)==0;
}
inline bool ObserveResult(void* object,void* function,void* frame,Receipt& last){
    const auto scan=ScanFrames();
    for(size_t at=0;at<scan.count;++at){const auto& c=scan.rows[at];
        if(c.rip!=engine::image+0x713a622||!data::AddEquals(c.rsp,0x130,c.rbp)||!StackRead(scan.stack,c.rsp,0x340))continue;
        auto p=FindProducer(reinterpret_cast<void*>(c.rsi));if(!p||p->target!=object||p->postDispatch!=function||!ProcessEventFrame(scan,at,object,function))continue;
        Receipt route{};if(!Live(p->postFunction,p->postId)||!Incoming(p->postFunction,function,frame,object,Read<int32_t>(p->postFunction,0xd8),route))continue;
        bool actualDispatch=false;
        for(size_t n=0;n<at;++n){const auto& d=scan.rows[n];if(d.rip==engine::image+0x1cf6ac9
            &&d.rdi==reinterpret_cast<uintptr_t>(object)&&d.r15==c.rbp-0x30&&d.r14==c.rsi+0x300
            &&d.rsi==static_cast<uintptr_t>(p->delegateIndex)&&data::AddEquals(d.rsp,0xb9,d.rbp)&&DelegateEntry(*p,d.rbx)){
                actualDispatch=true;break;}}
        if(!actualDispatch||Read<void*>(reinterpret_cast<void*>(c.rbp-0x30))!=p->component)continue;
        const auto native=Read<Array>(reinterpret_cast<void*>(c.rbp-0x60)),copy=Read<Array>(reinterpret_cast<void*>(c.rbp-0x28));
        const auto type=Read<void*>(reinterpret_cast<void*>(c.rbp+0x208));
        if(!ValidArray(native)||!ValidArray(copy)||native.count!=copy.count||type!=p->type
           ||c.rdi!=reinterpret_cast<uintptr_t>(native.data)||static_cast<uint32_t>(c.r15)!=static_cast<uint32_t>(native.count))continue;
        if(!data::SameElements(native.data,copy.data,static_cast<size_t>(native.count)))return false;
        std::array<void*,64> returned{};for(int32_t n=0;n<native.count;++n)returned[static_cast<size_t>(n)]=native.data[n];
        ++state.resultEvents;
        for(int32_t n=0;n<native.count;++n){Receipt r=route;r.actor=returned[static_cast<size_t>(n)];r.type=type;r.nativeRip=c.rip;r.outerRsp=c.rsp;
            if(!ActorFacts(r,*p)){++state.invalidSamples;continue;}
            // The actual returned array is proven here. A historical Weak plus
            // stack address is NOT a unique call id: both may be reused. Do
            // not promote or label a prior pending entry as this birth.
            r.origin=data::Origin::ThrowResult;
            Store(r);last=r;++state.resultActors;
        }return true;
    }++state.unwindMisses;return false;
}
struct Call {size_t index=0;void *object=nullptr,*frame=nullptr,*result=nullptr;Receipt receipt{};uint64_t serialBegin=0,serialEnd=0;};
inline void Before(void* opaque){auto& c=*static_cast<Call*>(opaque);const auto& f=functions[c.index];
    if(!state.initialized||state.closed||state.closing||!state.profiles||!state.schema
       ||!Live(f.object,f.id)||!Live(f.owner,f.ownerId)||Read<void*>(f.object,0x28)!=f.owner
       ||Read<uint32_t>(f.object,0xb8)!=f.flags||Read<uint16_t>(f.object,0xbe)!=f.bytes
       ||Read<Exec>(f.object,0xe0)!=f.replacement)return;
    bool valid=false;c.serialBegin=state.serial;
    if(f.purpose==Purpose::BeginPlay)valid=ObserveInit(c.object,f.object,c.frame,false,c.receipt);
    else if(f.purpose==Purpose::PoolInitialize)valid=ObserveInit(c.object,f.object,c.frame,true,c.receipt);
    else valid=ObserveResult(c.object,f.object,c.frame,c.receipt);
    if(!valid)++state.invalidSamples;c.serialEnd=state.serial;
}
inline void After(void* opaque){const auto& c=*static_cast<const Call*>(opaque);if(c.serialEnd<=c.serialBegin)return;
    for(auto& saved:receipts)if(saved.serial>c.serialBegin&&saved.serial<=c.serialEnd){saved.originalCompleted=true;
        saved.sourceStableAfter=Current(saved.source)&&Live(saved.actor,saved.actorId)&&Live(saved.type,saved.typeId)
            &&Read<void*>(saved.actor,0x10)==saved.type&&Read<void*>(saved.actor,0x210)==saved.owner;
    }
}
inline void Forward(size_t index,void* object,void* frame,void* result){
    InterlockedIncrement64(&forwards);functions[index].original(object,frame,result);InterlockedIncrement64(&completed);
}
template<size_t Index>inline void OriginalForward(void* object,void* frame,void* result){Forward(Index,object,frame,result);}
template<size_t Index>inline void Dispatch(void* object,void* frame,void* result){
    if(!Thread()||state.busy||!state.initialized||state.closed||state.closing){InterlockedIncrement64(&bypassed);Forward(Index,object,frame,result);return;}
    Call c{Index,object,frame,result};state.busy=true;
    __try{data::ForwardOnce(&OriginalForward<Index>,object,frame,result,&Before,&After,&c,state.sampleFaults,state.lastSampleException);}
    __finally{state.busy=false;Cache();}
}
template<size_t Index>inline void Observer(void* object,void* frame,void* result){
    InterlockedIncrement64(&entries);InterlockedIncrement(&activeCallbacks);
    __try{Dispatch<Index>(object,frame,result);}
    __finally{InterlockedDecrement(&activeCallbacks);}
}
template<size_t... I>inline auto Observers(std::index_sequence<I...>){return std::array<Exec,sizeof...(I)>{&Observer<I>...};}
inline const auto replacements=Observers(std::make_index_sequence<MaxFunctions>{});
inline Function* Install(void* function,void* owner,uint16_t bytes,uint32_t flags,uintptr_t exec,Purpose purpose){
    for(auto& f:functions)if(f.object==function){return Live(f.object,f.id)&&f.installed&&f.owner==owner&&f.bytes==bytes
        &&f.flags==flags&&f.purpose==purpose&&Read<Exec>(f.object,0xe0)==f.replacement?&f:nullptr;}
    if(!function||!owner||Read<void*>(function,0x28)!=owner||Read<uint16_t>(function,0xbe)!=bytes
       ||Read<uint32_t>(function,0xb8)!=flags||Read<uintptr_t>(function,0xe0)!=engine::image+exec
       ||!Writable(static_cast<uint8_t*>(function)+0xe0))return nullptr;
    for(size_t n=0;n<functions.size();++n){auto& f=functions[n];if(f.object)continue;
        f.object=function;f.owner=owner;f.original=engine::function<Exec>(exec);f.replacement=replacements[n];f.bytes=bytes;f.flags=flags;f.purpose=purpose;
        if(!Capture(function,f.id)||!Capture(owner,f.ownerId)){f={};return nullptr;}
        if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(static_cast<uint8_t*>(function)+0xe0),
            reinterpret_cast<void*>(f.replacement),reinterpret_cast<void*>(f.original))!=reinterpret_cast<void*>(f.original)){f={};return nullptr;}
        f.installed=true;++state.installs;return &f;
    }return nullptr;
}
inline bool Field(void* type,const wchar_t* name,int32_t offset,int32_t bytes){original_jett_priming::PropertyInfo p{};
    return original_jett_priming::FieldOffset(type,name,static_cast<size_t>(bytes),&p)==offset&&p.arrayDim==1&&p.elementSize==bytes;}
inline void Discover(Producer& p){
    const auto now=GetTickCount64();if(p.lastDiscoverAt&&now-p.lastDiscoverAt<100)return;p.lastDiscoverAt=now;
    void* configured=nullptr;++state.getterCalls;
    if(!OriginalEvent(p.component,classGetter,&configured)||!Type(Read<void*>(configured,0x120),projectileType))return;
    if(p.type&&configured!=p.type){state.stage="phoenix_c_actual_configured_class_changed_unknown";return;}
    if(!p.type){p.type=configured;if(!Capture(configured,p.typeId))return;}
    auto begin=engine::function<void*(*)(void*,const void*)>(0x1b9ab40)(configured,static_cast<uint8_t*>(beginName)+0x18);
    // Only the two retained original Phoenix C projectile BP overrides qualify.
    const auto expectedA=engine::Find(L"/Game/Characters/Phoenix/S0/Ability_Q/Production/Projectile_Phoenix_Q_FlameWall_Production.Projectile_Phoenix_Q_FlameWall_Production_C");
    const auto expectedB=engine::Find(L"/Game/Characters/Phoenix/S0/Ability_Q/Production/Projectile_Phoenix_Q_FlameWall_ThroughWall.Projectile_Phoenix_Q_FlameWall_ThroughWall_C");
    if(configured!=expectedA&&configured!=expectedB)return;
    if(!begin||Read<void*>(begin,0x28)!=configured||Read<uint16_t>(begin,0xbe)!=0||Read<uint8_t>(begin,0xbc)!=0
       ||Read<uint32_t>(begin,0xb8)!=0x08080800U||Read<int32_t>(begin,0x70)!=18||Read<void*>(begin,0x58))return;
    auto dispatch=Read<void*>(begin,0xd0);Function* installedBegin=nullptr;
    p.beginEntry=configured==expectedA?905:1111;
    if(dispatch){
        if(Read<int32_t>(begin,0xd8)!=p.beginEntry||Read<int32_t>(dispatch,0x70)!=(configured==expectedA?1338:1486)
           ||!Field(dispatch,L"EntryPoint",0,4))return;
        installedBegin=Install(dispatch,configured,4,configured==expectedA?0x00808001U:0x00008001U,0x1cdb7c0,Purpose::BeginPlay);
    }else{dispatch=begin;installedBegin=Install(begin,configured,0,0x08080800U,0x1cdb7c0,Purpose::BeginPlay);}
    if(!installedBegin||!Capture(begin,p.beginSourceId))return;
    p.beginSource=begin;p.beginDispatch=dispatch;p.beginDispatchId=installedBegin->id;
    const auto delegates=Read<Array>(p.component,0x300);p.delegateCount=delegates.count;
    if(delegates.count<0||delegates.count>64||delegates.capacity<delegates.count||delegates.capacity>65536
       ||(delegates.count&&!original_players::Readable(delegates.data,static_cast<size_t>(delegates.count)*20)))return;
    for(int32_t n=0;n<delegates.count;++n){auto entry=reinterpret_cast<uint8_t*>(delegates.data)+static_cast<size_t>(n)*20;
        const auto id=Read<Weak>(entry);const auto target=original_inventory_slots::ResolveIdentity(id);
        if(target!=p.view.ability||!Same(id,p.view.identities[6]))continue;
        auto fn=engine::function<void*(*)(void*,const void*)>(0x1b9ab40)(Read<void*>(target,0x10),entry+8);
        if(!fn||Read<uint16_t>(fn,0xbe)!=24
           ||!Field(fn,L"ThrowState",0,8)||!Field(fn,L"Projectiles",8,16))continue;
        const auto flags=Read<uint32_t>(fn,0xb8);if(flags&0x400U)continue;
        auto post=Read<void*>(fn,0xd0);if(!post)post=fn;
        if(post!=fn&&(!Field(post,L"EntryPoint",0,4)||Read<uint16_t>(post,0xbe)!=4))continue;
        auto installed=Install(post,Read<void*>(target,0x10),post==fn?24:4,Read<uint32_t>(post,0xb8),0x1cdb7c0,Purpose::ThrowResult);if(!installed)continue;
        if(!Capture(fn,p.postId))continue;
        p.target=target;p.targetId=id;p.postFunction=fn;p.postDispatch=post;p.delegateIndex=n;break;
    }
}
inline void PollImpl(void* pawn,uint32_t epoch){
    if(!Thread()||!state.initialized||state.closed||state.closing||state.busy)return;++state.polls;
    for(const auto& player:fw::GetPlayers())if(player.used&&!player.retired&&player.authority&&player.context.pawn==pawn&&player.epoch==epoch){
        for(size_t i=0;i<player.count;++i){const auto& item=player.items[i];const auto& native=item.native;
            if(native.type!=abilityType||native.slotType!=3||!Live(native.actor,native.actorId))continue;
            for(size_t k=0;k<native.componentCount;++k){auto component=native.components[k];
                if(!Live(component,native.componentIds[k])||!Type(component,throwType))continue;View view{};
                if(!fw::ResolveSource(component,view)||!Current(view))continue;
                Producer* p=nullptr;for(auto& old:producers)if(old.used&&!old.retired&&old.component==component&&Same(old.componentId,native.componentIds[k])){p=&old;break;}
                if(!p)for(auto& free:producers)if(!free.used||free.retired||!Live(free.component,free.componentId)){free={};p=&free;break;}if(!p)continue;
                if(!p->used){p->used=true;p->view=view;p->component=component;p->componentId=native.componentIds[k];}
                if(ProducerCurrent(*p))Discover(*p);
            }
        }
    }
    state.stage="phoenix_c_native_observer_ready_no_execution_claim";
    for(const auto& p:producers)if(p.used&&!p.retired&&p.type&&!p.postFunction){state.stage="phoenix_c_fresh_observer_ready_post_result_boundary_unbound";break;}
}
inline void PollQuery(void* opaque){const auto p=static_cast<const std::pair<void*,uint32_t>*>(opaque);PollImpl(p->first,p->second);}
inline bool MetadataSampleDue(uint64_t previous,void* previousPawn,uint32_t previousEpoch,uint64_t now,void* pawn,uint32_t epoch){
    return !previous||pawn!=previousPawn||epoch!=previousEpoch||now<previous||now-previous>=25;
}
inline void Poll(void* pawn,uint32_t epoch){
    if(!state.initialized||state.closed||state.closing||!Thread())return;
    const auto now=GetTickCount64();
    if(!MetadataSampleDue(state.lastMetadataAt,state.lastMetadataPawn,state.lastMetadataEpoch,now,pawn,epoch))return;
    state.lastMetadataAt=now;state.lastMetadataPawn=pawn;state.lastMetadataEpoch=epoch;
    std::pair<void*,uint32_t> query{pawn,epoch};
    data::SafeSample(&PollQuery,&query,state.sampleFaults,state.lastSampleException);Cache();}
inline bool Verify(){
    for(const auto& p:local_server_phoenix_c_birth_source::Profiles){const auto bytes=reinterpret_cast<const void*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes)||Hash(bytes,p.bytes)!=p.fnv)return false;}
    for(const auto& p:local_server_item_ownership::Profiles){const auto bytes=reinterpret_cast<const void*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes)||Hash(bytes,p.bytes)!=p.fnv)return false;}
    state.profiles=true;const auto actor=engine::Class(0x3683900);
    return Field(actor,L"Owner",0x210,8)&&Field(actor,L"Instigator",0x278,8)
        &&Field(throwType,L"OnThrow",0x300,16)&&Field(projectileType,L"FiringPlayerState",0x5b8,8)
        &&Field(projectileType,L"InitializedForUseTimestamp",0x518,4)
        &&original_jett_priming::FieldOffset(actor,L"bReplicates",1,&replicates)>=0&&replicates.boolFieldMask;
}
// This pre-install proof resolves the actual currently registered human Pawn.
// It runs before any Phoenix asset lookup or global projectile UF installation.
inline bool PhoenixActualHuman(void* world){
    const auto& framework=fw::GetState();
    if(!world||!framework.initialized||!framework.host||framework.world!=world||framework.error||framework.restartRequired
       ||!framework.profilesVerified||!framework.schemaVerified||!original_players::Thread()
       ||framework.gameThread!=GetCurrentThreadId())return false;
    const fw::Player* actual=nullptr;
    for(const auto& p:fw::GetPlayers()){
        if(!p.used||p.retired||!p.authority||p.hero!=1||!p.catalogBound||!p.assemblyReady
           ||p.context.world!=world||!Live(p.context.pawn,p.context.pawnId))continue;
        View current{};
        if(!fw::ResolveSource(p.context.pawn,current)||!fw::ValidateSource(current)
           ||current.heroIndex!=1||current.contextObject!=p.context.pawn||current.pawn!=p.context.pawn
           ||current.world!=world||current.ability||current.pendingGrant||current.sourceEpoch!=p.epoch
           ||current.registration!=p.registration||!Same(current.identities[4],p.context.pawnId)
           ||!local_server_hero_authority::ValidateHostSourceImpl(current))continue;
        if(actual)return false;actual=&p;
    }return actual!=nullptr;
}
inline bool InitHostImpl(void* world){
    if(!PhoenixActualHuman(world)){state.stage="phoenix_c_native_observer_waiting_actual_phoenix_human";Cache();return false;}
    if(state.initialized)return !state.closed&&state.world==world&&Thread();
    if(!world||!original_players::Thread()||local_server_hero_abilities::GetState().world!=world
       ||!local_server_hero_abilities::GetState().host)return false;
    state.world=world;state.worldId=local_server_hero_abilities::GetState().worldId;state.thread=GetCurrentThreadId();
    abilityType=engine::Find(AbilityPath);throwType=engine::Find(L"/Script/ShooterGame.ProjectileThrowStateComponent");projectileType=engine::Class(0x644d990);
    beginName=engine::Find(L"/Script/Engine.Actor.ReceiveBeginPlay");classGetter=engine::Find(L"/Script/ShooterGame.ProjectileThrowStateComponent.GetProjectileClass");
    const auto nt=GetModuleHandleW(L"ntdll.dll");captureContext=reinterpret_cast<CaptureContext>(GetProcAddress(nt,"RtlCaptureContext"));
    lookup=reinterpret_cast<Lookup>(GetProcAddress(nt,"RtlLookupFunctionEntry"));unwind=reinterpret_cast<Unwind>(GetProcAddress(nt,"RtlVirtualUnwind"));
    if(!abilityType||!throwType||!projectileType||!beginName||!classGetter||!captureContext||!lookup||!unwind||!Live(world,state.worldId)
       ||Read<uint16_t>(classGetter,0xbe)!=8||Read<uintptr_t>(classGetter,0xe0)!=engine::image+0x6b2f680||!Verify()){
        state.stage="phoenix_c_native_observer_source_or_metadata_unavailable";Cache();return false;}
    auto pool=engine::Find(L"/Script/ShooterGame.Projectile.MulticastInitializeForUse");
    if(!Field(pool,L"SpawnTransform",0,96)||!Field(pool,L"NewOwner",0x60,8)||!Field(pool,L"NewInstigator",0x68,8)
       ||!Install(pool,projectileType,112,0x00824cc0U,0x6b28620,Purpose::PoolInitialize)){
        state.stage="phoenix_c_native_observer_pool_metadata_unavailable";Cache();return false;}
    state.schema=true;state.initialized=true;state.stage="phoenix_c_native_observer_waiting_actual_producer";Cache();return true;
}
struct InitCall {void* world=nullptr;bool result=false;};
inline void InitQuery(void* opaque){auto& c=*static_cast<InitCall*>(opaque);c.result=InitHostImpl(c.world);}
inline bool InitHost(void* world){InitCall c{world,false};
    data::SafeSample(&InitQuery,&c,state.sampleFaults,state.lastSampleException);Cache();return c.result;}
inline void Retire(void* pawn,uint32_t epoch){if(!Thread())return;
    for(auto& p:producers)if(p.used&&!p.retired&&p.view.pawn==pawn&&p.view.sourceEpoch==epoch){p.retired=true;++state.retirements;}Cache();}
inline bool CloseImpl(){
    if(!state.thread){state.closed=true;Cache();return true;}if(!Thread())return false;
    state.closing=true;if(state.busy||InterlockedCompareExchange(&activeCallbacks,0,0)!=0)return false;bool okay=true;
    for(auto& f:functions)if(f.installed){if(!Live(f.object,f.id)){f.installed=false;continue;}
        auto address=static_cast<uint8_t*>(f.object)+0xe0;auto slot=reinterpret_cast<void* volatile*>(address);const auto current=Read<Exec>(f.object,0xe0);
        if(current==f.original){f.installed=false;continue;}
        if(current!=f.replacement||!Writable(address)||InterlockedCompareExchangePointer(slot,reinterpret_cast<void*>(f.original),reinterpret_cast<void*>(f.replacement))!=reinterpret_cast<void*>(f.replacement)){
            okay=false;continue;}f.installed=false;++state.restores;
    }state.closed=okay;state.stage=okay?"phoenix_c_native_observer_original_execs_restored":"phoenix_c_native_observer_foreign_exec_not_overwritten";Cache();return okay;
}
inline void CloseQuery(void* opaque){*static_cast<bool*>(opaque)=CloseImpl();}
inline bool Close(){bool okay=false;data::SafeSample(&CloseQuery,&okay,state.sampleFaults,state.lastSampleException);Cache();return okay;}
inline std::string Json(){Snapshot s{};AcquireSRWLockShared(&cacheLock);s=cached;ReleaseSRWLockShared(&cacheLock);
    local_server_match::Json out;const auto b=[](bool v){return v?"true":"false";};const auto u=[](uint64_t v){return static_cast<unsigned long long>(v);};
    out.Add("{\"stage\":\"%s\",\"initialized\":%s,\"closed\":%s,\"profiles_verified\":%s,\"schema_verified\":%s,\"polls\":%llu,\"entries\":%lld,\"original_forward_attempts\":%lld,\"original_forward_completed\":%lld,\"bypassed\":%lld,\"active_callbacks\":%ld,\"exec_slot_install_writes\":%llu,\"exec_slot_restore_writes\":%llu,\"weak_capture_calls\":%llu,\"weak_serial_allocation_possible\":true,\"original_projectile_class_getter_calls\":%llu,\"fresh_before_beginplay\":%llu,\"pool_before_original_reset\":%llu,\"actual_throw_result_events\":%llu,\"actual_throw_result_actors\":%llu,\"sample_faults\":%llu,\"last_sample_exception\":%u,\"unwind_misses\":%llu,\"invalid_samples\":%llu,\"retirements\":%llu,\"producers\":[",
        s.state.stage,b(s.state.initialized),b(s.state.closed),b(s.state.profiles),b(s.state.schema),u(s.state.polls),InterlockedCompareExchange64(&entries,0,0),InterlockedCompareExchange64(&forwards,0,0),InterlockedCompareExchange64(&completed,0,0),InterlockedCompareExchange64(&bypassed,0,0),InterlockedCompareExchange(&activeCallbacks,0,0),u(s.state.installs),u(s.state.restores),u(s.state.captures),u(s.state.getterCalls),u(s.state.fresh),u(s.state.pool),u(s.state.resultEvents),u(s.state.resultActors),u(s.state.sampleFaults),s.state.lastSampleException,u(s.state.unwindMisses),u(s.state.invalidSamples),u(s.state.retirements));
    bool comma=false;for(const auto& p:s.producers)if(p.used){out.Add("%s{\"component\":\"%p\",\"configured_class\":\"%p\",\"actual_onthrow_delegate_count\":%d,\"existing_post_delegate_bound\":%s,\"retired\":%s,\"begin_source_function\":\"%p\",\"begin_dispatch_function\":\"%p\",\"begin_expected_graph_entry\":%d,\"post_source_function\":\"%p\",\"post_dispatch_function\":\"%p\"}",comma?",":"",p.component,p.type,p.delegateCount,b(p.postFunction!=nullptr),b(p.retired),p.beginSource,p.beginDispatch,p.beginEntry,p.postFunction,p.postDispatch);comma=true;}
    out.Add("],\"receipts\":[");comma=false;
    for(const auto& r:s.receipts)if(r.serial){out.Add("%s{\"serial\":%llu,\"at_ms\":%llu,\"origin\":%u,\"valid\":%s,\"original_callback_completed\":%s,\"source_stable_after\":%s,\"actor\":\"%p\",\"class\":\"%p\",\"producer\":\"%p\",\"world\":\"%p\",\"owner_ability\":\"%p\",\"instigator_pawn\":\"%p\",\"firing_player_state\":\"%p\",\"actor_weak\":[%d,%d],\"producer_weak\":[%d,%d],\"pawn_weak\":[%d,%d],\"epoch\":%u,\"registration\":%llu,\"native_return_rva\":%llu,\"outer_rsp\":%llu,\"native_rsp\":%llu,\"initialized_timestamp_raw_bits\":%u,\"original_breplicates\":%s,\"role\":%u",
        comma?",":"",u(r.serial),u(r.at),unsigned(r.origin),b(r.valid),b(r.originalCompleted),b(r.sourceStableAfter),r.actor,r.type,r.component,r.source.world,r.owner,r.instigator,r.firingPS,r.actorId.index,r.actorId.serial,r.componentId.index,r.componentId.serial,r.source.identities[4].index,r.source.identities[4].serial,r.source.sourceEpoch,u(r.source.registration),u(r.nativeRip>=engine::image?r.nativeRip-engine::image:0),u(r.outerRsp),u(r.nativeRsp),r.initializedBits,b(r.originalReplicates),unsigned(r.role));
        out.Add(",\"source_function\":\"%p\",\"dispatch_function\":\"%p\",\"graph_redirected\":%s,\"actual_entrypoint_known\":%s,\"actual_entrypoint\":%d}",r.sourceFunction,r.dispatchFunction,b(r.redirected),b(r.entryKnown),r.graphEntry);comma=true;}
    out.Add("],\"frame_header_samples\":%llu,\"incoming_frame_metadata_bytes\":%llu,\"graph_entry_samples\":%llu,\"redirected_graph_samples\":%llu,\"incoming_frame_code_reads\":0,\"frame_step_calls\":0,\"incoming_frame_writes\":0,\"function_flag_writes\":0,\"resource_writes\":0,\"synthetic_lifecycle_calls\":0,\"actor_publications\":0,\"client_receipt_verified\":false,\"phoenix_c_skill_execution_verified\":false,\"same_native_call_init_result_pair_verified\":false,\"pool_observer_is_conditional_original_callback_only\":true,\"native_origin_view_exported\":false,\"shared_source_resolver_may_capture_native_weak_serials\":true}",
        u(s.state.frameHeaderSamples),u(s.state.frameMetadataBytes),u(s.state.graphEntrySamples),u(s.state.redirectedSamples));return out.value;
}
} // namespace local_server_phoenix_c_native_observer
