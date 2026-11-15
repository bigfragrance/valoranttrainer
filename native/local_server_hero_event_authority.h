#pragma once
#include "local_server_hero_abilities.h"
#include "local_server_match.h"
#include "local_server_hero_event_authority.generated.h"
#include <array>
#include <cstdio>
#include <cstring>

// Candidate host-only continuation of observed original RegisterEvent(mode1)
// requests. It never discovers a mode from listener-map keys, emits an event,
// replays BeginPlay, or edits a resource/PlayerState/pool lifecycle.
namespace local_server_hero_event_authority {
namespace framework=local_server_hero_abilities;
using View=framework::SourceView;
using Weak=original_inventory_slots::WeakIdentity;
using Exec=void(*)(void*,void*,void*);
inline constexpr size_t MaxStarters=framework::MaxItems,MaxRequests=512;
struct Hooks {bool(*resolve)(void*,View&)=nullptr;bool(*validate)(const View&)=nullptr;};
struct Starter {void *type=nullptr,*defaults=nullptr;Weak typeId{},defaultsId{};uint8_t slot=255;};
struct Human {
    bool registered=false,retiring=false;unsigned hero=0;uint32_t epoch=0;uint64_t receipt=0;
    void *pc=nullptr,*ps=nullptr,*pawn=nullptr,*inventory=nullptr,*heroType=nullptr,*actualType=nullptr,*defaults=nullptr;
    Weak pcId{},psId{},pawnId{},inventoryId{},heroId{},actualTypeId{},defaultsId{};
    size_t count=0;std::array<Starter,MaxStarters> starters{};
};
// Observed private origin, deliberately not a fabricated framework SourceView.
// A real framework registration is recorded only after the composed resolver.
struct ObservedOrigin {
    void *world=nullptr,*pc=nullptr,*ps=nullptr,*pawn=nullptr,*inventory=nullptr;
    void *heroType=nullptr,*actualType=nullptr,*heroDefaults=nullptr,*starterDefaults=nullptr;
    Weak worldId{},pcId{},psId{},pawnId{},inventoryId{},heroId{},actualTypeId{},heroDefaultsId{},starterDefaultsId{};
    unsigned hero=0;uint32_t epoch=0;uint64_t humanReceipt=0;
};
struct Observation {
    void *component=nullptr,*ability=nullptr,*abilityType=nullptr;Weak componentId{},abilityId{},abilityTypeId{};
    uint64_t humanReceipt=0;uint32_t epoch=0;size_t starter=0;ObservedOrigin origin{};
};
struct Request {
    bool used=false,retired=false,ready=false,inFlight=false,setDone=false,centralAttempted=false,sourceBound=false;
    Observation observed{};void* type=nullptr;Weak typeId{};uint8_t originalMode=1;
    uint64_t serial=0,occurrences=0;View source{};void* attemptedRegistry=nullptr;Weak attemptedRegistryId{};
    int32_t setIndex=-1;const char* stage="original_request_not_observed";
};
struct Binding {void* object=nullptr;Weak identity{};Exec original=nullptr,replacement=nullptr;
    void *eventType=nullptr,*emitMode=nullptr;bool installed=false;};
struct Metadata {
    void *function=nullptr,*owner=nullptr,*eventField=nullptr,*modeField=nullptr,*setField=nullptr;
    uintptr_t exec=0;uint32_t flags=0;uint16_t parms=0;int32_t eventOffset=-1,eventBytes=0,modeOffset=-1,modeBytes=0,setOffset=-1,setBytes=0;
    uint64_t eventFlags=0,modeFlags=0,setFlags=0;
};
struct RequestDiagnostic {
    bool observed=false,ready=false,retired=false,sourceBound=false,pendingGrant=false;
    Observation observation{};void* type=nullptr;Weak typeId{};uint8_t originalMode=0;
    uint64_t serial=0,frameworkRegistration=0;int32_t setIndex=-1;const char* stage="none";
};
struct State {
    bool initialized=false,installed=false,closed=false,closing=false,restartRequired=false;
    bool profiles=false,schema=false,policyKnown=false,practice=false,skillsEnabled=false;
    DWORD gameThread=0;void *world=nullptr,*driver=nullptr;Weak worldId{},driverId{};unsigned error=0,active=0,producerDepth=0;
    uint64_t session=0,humanRegistrations=0,nextRequest=0,nextPoll=0;
    uint64_t calls=0,originalForwards=0,typedForwards=0,scriptSteps=0,compiledSteps=0,observedMode1=0;
    uint64_t captured=0,duplicateRequests=0,ready=0,pending=0,retiredRequests=0,sourceDeferrals=0,phaseTransitions=0;
    uint64_t nativeSetAdds=0,nativeCentralCalls=0,setReadbacks=0,poolKeyReadbacks=0,reentryDeferrals=0;
    uint64_t nativeReturnedRetired=0,retirements=0,closes=0,restores=0,foreignLayers=0,polls=0;
    uint32_t exceptionCode=0;uintptr_t exceptionRva=0;Metadata metadata{};
    RequestDiagnostic firstRequest{},lastRequest{};
    const char* stage="hero_event_authority_not_initialized";
};
inline State state{},cached{};inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline Hooks hooks{};inline Human human{};inline Binding binding{};inline std::array<Request,MaxRequests> requests{};
inline void* eventBase=nullptr;
inline const State& GetState(){return state;}
inline void Cache(){AcquireSRWLockExclusive(&cacheLock);cached=state;ReleaseSRWLockExclusive(&cacheLock);}
inline State Snapshot(){AcquireSRWLockShared(&cacheLock);auto value=cached;ReleaseSRWLockShared(&cacheLock);return value;}
template<class T>inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Capture(void* object,Weak& identity){return original_inventory_slots::CaptureIdentity(object,identity)&&Live(object,identity);}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;Cache();return false;}
inline bool Thread(){return state.gameThread&&state.gameThread==GetCurrentThreadId()&&original_players::Thread();}
inline bool HostWorld(){return state.world&&state.driver&&Live(state.world,state.worldId)&&Live(state.driver,state.driverId)
    &&Read<void*>(state.world,0x40)==state.driver&&Read<void*>(state.driver,0x178)==state.world&&!Read<void*>(state.driver,0xc0);}
inline bool Policy(){return state.initialized&&state.installed&&!state.closed&&!state.closing&&!state.error&&!state.restartRequired
    &&state.policyKnown&&state.practice&&state.skillsEnabled&&!local_server_match::FreeForAll()&&HostWorld();}
inline bool Writable(void* p,size_t bytes){
    if(!p||!bytes)return false;auto at=reinterpret_cast<uintptr_t>(p);if(at+bytes<at)return false;const auto end=at+bytes;
    while(at<end){MEMORY_BASIC_INFORMATION m{};if(!VirtualQuery(reinterpret_cast<void*>(at),&m,sizeof(m))||m.State!=MEM_COMMIT||m.Protect&(PAGE_GUARD|PAGE_NOACCESS))return false;
        const auto protection=m.Protect&0xffU;if(protection!=PAGE_READWRITE&&protection!=PAGE_WRITECOPY&&protection!=PAGE_EXECUTE_READWRITE&&protection!=PAGE_EXECUTE_WRITECOPY)return false;
        const auto next=reinterpret_cast<uintptr_t>(m.BaseAddress)+m.RegionSize;if(next<=at)return false;at=next;
    }return true;
}
template<class T>inline bool Put(void* p,size_t offset,const T& value){auto to=static_cast<uint8_t*>(p)+offset;
    if(!Writable(to,sizeof(T)))return false;std::memcpy(to,&value,sizeof(T));return true;}
inline bool ImageCode(const void* p){MEMORY_BASIC_INFORMATION m{};if(!p||!VirtualQuery(p,&m,sizeof(m)))return false;
    const auto access=m.Protect&0xffU;return m.State==MEM_COMMIT&&m.Type==MEM_IMAGE&&m.AllocationBase==reinterpret_cast<void*>(engine::image)
        &&!(m.Protect&(PAGE_GUARD|PAGE_NOACCESS))&&(access==PAGE_EXECUTE||access==PAGE_EXECUTE_READ||access==PAGE_EXECUTE_READWRITE||access==PAGE_EXECUTE_WRITECOPY);}
inline void Require(bool valid,unsigned code,const char* stage){if(valid)return;Fail(code,stage);state.restartRequired=true;Cache();RaiseException(0xe0008400U+(code-8400U),EXCEPTION_NONCONTINUABLE,0,nullptr);}
inline bool HumanCurrent(){
    const auto p=local_server_match::Player(0);void* connection=nullptr;
    return human.registered&&!human.retiring&&human.hero>0&&human.hero<original_heroes::PlayableHeroCount&&human.epoch&&human.receipt
        &&HostWorld()&&p&&p->human&&p->hero==human.hero&&p->epoch==human.epoch&&p->pc==human.pc&&p->ps==human.ps&&p->pawn==human.pawn
        &&local_server_match::GetState().world==state.world&&local_server_match::AuthorityPair(*p)
        &&Live(human.pc,human.pcId)&&Live(human.ps,human.psId)&&Live(human.pawn,human.pawnId)&&Live(human.inventory,human.inventoryId)
        &&Live(human.heroType,human.heroId)&&Live(human.actualType,human.actualTypeId)&&Live(human.defaults,human.defaultsId)
        &&Read<void*>(human.pawn,0x10)==human.actualType&&Read<void*>(human.actualType,0x120)==human.defaults
        &&local_server_item_ownership::ActualHuman(state.world,human.pc,human.ps,human.pawn,human.inventory,connection);
}
inline bool Candidate(void* component,Observation& out){
    out={};if(!state.policyKnown||!state.practice||local_server_match::FreeForAll()||!HumanCurrent()
       ||!original_players::IsA(component,0x6768f30))return false;
    auto ability=Read<void*>(component,0xc8),type=Read<void*>(ability,0x10);
    if(!original_players::IsA(ability,0x645c660)||Read<uint8_t>(ability,0x224)!=3
       ||Read<void*>(ability,0x210)!=human.pawn||Read<void*>(ability,0xe18)!=component
       ||local_server_item_ownership::ActorWorld(ability)!=state.world)return false;
    auto owning=Read<void*>(ability,0x4a8);if(owning&&owning!=human.pawn)return false;
    for(size_t n=0;n<human.count;++n){const auto& s=human.starters[n];
        if(type!=s.type||s.slot<=2||s.slot>=17||!Live(s.type,s.typeId)||!Live(s.defaults,s.defaultsId)
           ||Read<void*>(type,0x120)!=s.defaults||Read<void*>(s.defaults,0x10)!=s.type
           ||!original_players::IsA(s.defaults,0x645c660)||Read<uint8_t>(s.defaults,0x4a0)!=s.slot)continue;
        if(!Capture(component,out.componentId)||!Capture(ability,out.abilityId)||!Capture(type,out.abilityTypeId))return false;
        out.component=component;out.ability=ability;out.abilityType=type;out.humanReceipt=human.receipt;out.epoch=human.epoch;out.starter=n;
        auto& o=out.origin;o.world=state.world;o.worldId=state.worldId;o.pc=human.pc;o.pcId=human.pcId;o.ps=human.ps;o.psId=human.psId;
        o.pawn=human.pawn;o.pawnId=human.pawnId;o.inventory=human.inventory;o.inventoryId=human.inventoryId;
        o.heroType=human.heroType;o.heroId=human.heroId;o.actualType=human.actualType;o.actualTypeId=human.actualTypeId;
        o.heroDefaults=human.defaults;o.heroDefaultsId=human.defaultsId;o.starterDefaults=s.defaults;o.starterDefaultsId=s.defaultsId;
        o.hero=human.hero;o.epoch=human.epoch;o.humanReceipt=human.receipt;return true;
    }return false;
}
inline bool ObservationCurrent(const Observation& before){
    if(before.humanReceipt!=human.receipt||before.epoch!=human.epoch||!Live(before.component,before.componentId)
       ||!Live(before.ability,before.abilityId)||!Live(before.abilityType,before.abilityTypeId))return false;
    const auto& o=before.origin;
    if(o.world!=state.world||o.pc!=human.pc||o.ps!=human.ps||o.pawn!=human.pawn||o.inventory!=human.inventory
       ||o.heroType!=human.heroType||o.actualType!=human.actualType||o.heroDefaults!=human.defaults||o.hero!=human.hero
       ||o.epoch!=human.epoch||o.humanReceipt!=human.receipt||!Live(o.world,o.worldId)||!Live(o.pc,o.pcId)||!Live(o.ps,o.psId)
       ||!Live(o.pawn,o.pawnId)||!Live(o.inventory,o.inventoryId)||!Live(o.heroType,o.heroId)||!Live(o.actualType,o.actualTypeId)
       ||!Live(o.heroDefaults,o.heroDefaultsId)||!Live(o.starterDefaults,o.starterDefaultsId))return false;
    Observation after{};return Candidate(before.component,after)&&after.ability==before.ability&&after.abilityType==before.abilityType
        &&after.starter==before.starter&&original_inventory_slots::SameIdentity(after.componentId,before.componentId)
        &&original_inventory_slots::SameIdentity(after.abilityId,before.abilityId)&&original_inventory_slots::SameIdentity(after.abilityTypeId,before.abilityTypeId);
}
inline bool SameSource(const View& a,const View& b){
    if(a.contextObject!=b.contextObject||a.world!=b.world||a.controller!=b.controller||a.playerState!=b.playerState
       ||a.pawn!=b.pawn||a.inventory!=b.inventory||a.ability!=b.ability||a.heroIndex!=b.heroIndex
       ||a.sourceEpoch!=b.sourceEpoch||a.registration!=b.registration)return false;
    for(size_t n=0;n<a.identities.size();++n)if(!original_inventory_slots::SameIdentity(a.identities[n],b.identities[n]))return false;
    return true;
}
inline bool CurrentView(Request& request,View& out){
    out={};if(!Policy()||request.retired||!ObservationCurrent(request.observed)||!Live(request.type,request.typeId)
       ||!hooks.resolve||!hooks.validate||!hooks.resolve(request.observed.component,out)||!hooks.validate(out))return false;
    if(out.contextObject!=request.observed.component||out.world!=state.world||out.controller!=human.pc||out.playerState!=human.ps
       ||out.pawn!=human.pawn||out.inventory!=human.inventory||out.ability!=request.observed.ability||out.heroIndex!=human.hero
       ||out.sourceEpoch!=human.epoch||!out.registration)return false;
    const void* objects[]{out.contextObject,out.world,out.controller,out.playerState,out.pawn,out.inventory,out.ability};
    for(size_t n=0;n<out.identities.size();++n)if(!Live(const_cast<void*>(objects[n]),out.identities[n]))return false;
    if(!original_inventory_slots::SameIdentity(out.identities[0],request.observed.componentId)
       ||!original_inventory_slots::SameIdentity(out.identities[6],request.observed.abilityId)
       ||!original_inventory_slots::SameIdentity(out.identities[4],human.pawnId))return false;
    if(request.sourceBound){
        if(!SameSource(request.source,out)||(!request.source.pendingGrant&&out.pendingGrant))return false;
        if(request.source.pendingGrant&&!out.pendingGrant)++state.phaseTransitions;
    }
    // This is a freshly resolved, coordinator-validated native phase. No old
    // view's pendingGrant bit is edited to manufacture an unchanged proof.
    request.source=out;request.sourceBound=true;return true;
}
inline bool EventType(void* type){return original_players::IsA(type,0x1af0eb0)&&original_players::Derived(type,eventBase);}
struct SetView {uint8_t* data=nullptr;uint32_t* bits=nullptr;int32_t slots=0;};
inline bool SetStorage(void* component,SetView& view){
    auto set=static_cast<uint8_t*>(component)+0x138;auto data=Read<uint8_t*>(set);
    const auto slots=Read<int32_t>(set,8),capacity=Read<int32_t>(set,12),free=Read<int32_t>(set,0x34);
    const auto bitNum=Read<int32_t>(set,0x28),bitMax=Read<int32_t>(set,0x2c);auto heapBits=Read<uint32_t*>(set,0x20);
    auto bits=heapBits?heapBits:reinterpret_cast<uint32_t*>(set+0x10);
    if(slots<0||slots>16384||capacity<slots||capacity>16384||free<0||free>slots||bitNum<slots||bitMax<bitNum
       ||bitNum>16384||bitMax>65536||(!heapBits&&bitMax>128)||!Writable(set,0x50)
       ||(capacity&&(!data||!original_players::Readable(data,size_t(capacity)*16)))
       ||(bitMax&&!original_players::Readable(bits,((size_t(bitMax)+31U)/32U)*4U)))return false;
    view={data,bits,slots};return true;
}
inline bool SetIndex(void* component,int32_t index,void* type){
    SetView view{};if(!SetStorage(component,view)||index<0||index>=view.slots)return false;
    return (view.bits[uint32_t(index)/32]&(1U<<(uint32_t(index)%32)))!=0&&Read<void*>(view.data,size_t(index)*16)==type;
}
inline int32_t FindSet(void* component,void* type){
    SetView view{};if(!SetStorage(component,view))return -2;
    for(int32_t n=0;n<view.slots;++n)if((view.bits[uint32_t(n)/32]&(1U<<(uint32_t(n)%32)))!=0
        &&Read<void*>(view.data,size_t(n)*16)==type)return n;return -1;
}
inline void* Registry(void* component){
    auto gs=Read<void*>(state.world,0x178),registry=Read<void*>(component,0x198);
    if(!registry)return nullptr;
    if(!original_players::IsA(gs,0x6c9d280)||gs!=local_server_match::GetState().gameState
       ||registry!=Read<void*>(gs,0xce0)||!original_players::IsA(registry,0x6769940)||Read<void*>(registry,0xc8)!=gs)return nullptr;
    return registry;
}
inline bool PoolKey(void* registry,void* type){
    // Original only promises AL (setne al). An int/Bool32 declaration would
    // incorrectly treat FFFFFF00 as true for a missing native map key.
    return engine::function<bool(*)(void*,void*)>(0x6ff68d0)(registry,type);
}
inline RequestDiagnostic Diagnostic(const Request& request){
    RequestDiagnostic d{};d.observed=request.used;d.ready=request.ready;d.retired=request.retired;d.sourceBound=request.sourceBound;
    d.pendingGrant=request.source.pendingGrant;d.observation=request.observed;d.type=request.type;d.typeId=request.typeId;
    d.originalMode=request.originalMode;d.serial=request.serial;d.frameworkRegistration=request.source.registration;
    d.setIndex=request.setIndex;d.stage=request.stage;return d;
}
inline void RefreshRequestEvidence(){
    state.ready=state.pending=0;
    for(const auto& request:requests)if(request.used){
        if(!request.retired){if(request.ready)++state.ready;else ++state.pending;}
        if(state.firstRequest.observed&&state.firstRequest.serial==request.serial)state.firstRequest=Diagnostic(request);
        if(state.lastRequest.observed&&state.lastRequest.serial==request.serial)state.lastRequest=Diagnostic(request);
    }
}
inline void RetireRequest(Request& request,const char* stage){
    if(!request.retired){request.retired=true;request.ready=false;++state.retiredRequests;}request.stage=stage;
    RefreshRequestEvidence();
}
inline Request* VacantRequest(){
    for(auto& request:requests)if(!request.used||(request.retired&&!request.inFlight))return &request;return nullptr;
}
inline bool Complete(Request& request){
    if(request.retired||request.ready)return true;
    if(request.inFlight||state.producerDepth){++state.reentryDeferrals;request.stage="original_pool_factory_in_flight_request_retained";return true;}
    View before{};if(!CurrentView(request,before)){++state.sourceDeferrals;request.stage="waiting_actual_framework_and_coordinator_source";return true;}
    void* registry=Registry(request.observed.component);Weak registryId{};
    if(registry&&!Capture(registry,registryId)){request.stage="waiting_actual_registry_weak_identity";return true;}
    request.inFlight=true;++state.producerDepth;
    __try{
        if(!request.setDone){
            const auto existing=FindSet(request.observed.component,request.type);
            if(existing==-2){request.stage="pending_unproven_original_set_layout";return true;}
            if(existing>=0)request.setIndex=existing;
            else{
                int32_t index=-1;auto type=request.type;
                auto result=engine::function<int32_t*(*)(void*,int32_t*,void**)>(0x7008150)
                    (static_cast<uint8_t*>(request.observed.component)+0x138,&index,&type);++state.nativeSetAdds;
                Require(result==&index,8401,"hero_event_original_set_add_return_abi_mismatch");request.setIndex=index;
            }
            View after{};if(!CurrentView(request,after)){++state.nativeReturnedRetired;RetireRequest(request,"source_changed_during_original_set_registration");return true;}
            Require(SetIndex(request.observed.component,request.setIndex,request.type),8402,"hero_event_original_set_registration_readback_failed");
            request.setDone=true;++state.setReadbacks;
        }
        // The original helper also leaves local-only registration when198 is
        // null. Native6FF5D20 later links/replays that set. Never invent198.
        registry=Registry(request.observed.component);
        if(!registry){request.stage="waiting_native_game_state_registry_link";return true;}
        if(!Capture(registry,registryId)){request.stage="waiting_actual_registry_weak_identity";return true;}
        if(!PoolKey(registry,request.type)){
            if(request.centralAttempted&&request.attemptedRegistry==registry&&Live(registry,request.attemptedRegistryId)){
                request.stage="pending_original_central_registration_returned_without_class_key";return true;
            }
            request.centralAttempted=true;request.attemptedRegistry=registry;request.attemptedRegistryId=registryId;
            engine::function<void(*)(void*,void*)>(0x6ff67f0)(registry,request.type);++state.nativeCentralCalls;
        }
        View after{};if(!CurrentView(request,after)||!Live(registry,registryId)||Registry(request.observed.component)!=registry){
            ++state.nativeReturnedRetired;RetireRequest(request,"source_or_registry_changed_during_original_pool_registration");return true;
        }
        Require(SetIndex(request.observed.component,request.setIndex,request.type),8403,"hero_event_original_set_changed_during_pool_registration");
        if(!PoolKey(registry,request.type)){request.stage="pending_original_central_class_key_missing";return true;}
        ++state.poolKeyReadbacks;request.ready=true;request.stage="original_mode1_request_native_set_and_class_pool_observed_no_event_emitted";
    }__finally{--state.producerDepth;request.inFlight=false;state.lastRequest=Diagnostic(request);RefreshRequestEvidence();}
    return true;
}
inline void Observe(const Observation& before,void* type,uint8_t mode){
    ++state.observedMode1;
    if(mode!=1||!EventType(type)||!ObservationCurrent(before))return;
    Weak typeId{};if(!Capture(type,typeId))return;
    for(auto& r:requests){
        if(r.used&&!r.retired&&r.observed.humanReceipt==before.humanReceipt&&r.observed.component==before.component&&r.type==type
           &&original_inventory_slots::SameIdentity(r.observed.componentId,before.componentId)&&original_inventory_slots::SameIdentity(r.typeId,typeId)){
            ++r.occurrences;++state.duplicateRequests;Complete(r);RefreshRequestEvidence();return;
        }
    }
    auto empty=VacantRequest();
    Require(empty&&state.nextRequest!=UINT64_MAX,8404,"hero_event_original_request_capacity_exhausted");
    *empty={};empty->used=true;empty->observed=before;empty->type=type;empty->typeId=typeId;empty->originalMode=mode;
    empty->serial=++state.nextRequest;empty->occurrences=1;empty->stage="observed_original_server_only_request";++state.captured;
    if(!state.firstRequest.observed)state.firstRequest=Diagnostic(*empty);state.lastRequest=Diagnostic(*empty);Complete(*empty);
    if(state.firstRequest.serial==empty->serial)state.firstRequest=Diagnostic(*empty);state.lastRequest=Diagnostic(*empty);RefreshRequestEvidence();
}
inline bool Step(void* frame,void* field,void* destination){
    auto code=Read<uint8_t*>(frame,0x20);
    if(code){if(!original_players::Readable(code,1))return false;auto opcode=*code;
        auto native=Read<Exec>(reinterpret_cast<void*>(engine::image+0xd747c40),size_t(opcode)*8);
        if(!ImageCode(reinterpret_cast<void*>(native))||!Put(frame,0x20,code+1))return false;
        native(Read<void*>(frame,0x18),frame,destination);++state.scriptSteps;
    }else{auto locals=Read<uint8_t*>(frame,0x28);const auto offset=Read<int32_t>(field,0x44),bytes=Read<int32_t>(field,0x34);
        auto copy=Read<void*>(Read<void*>(field),0x110);
        if(Read<void*>(frame,0x88)!=field||!locals||offset<0||bytes<1||bytes>8||!ImageCode(copy)
           ||!original_players::Readable(locals+offset,size_t(bytes))||!Put(frame,0x88,Read<void*>(field,0x18))
           ||!Put<void*>(frame,0x40,locals)||!Put<void*>(frame,0x38,locals+offset))return false;
        reinterpret_cast<void(*)(void*,void*,const void*)>(copy)(field,destination,locals);++state.compiledSteps;
    }return true;
}
inline bool End(void* frame){auto code=Read<uint8_t*>(frame,0x20);return !code
    ||(original_players::Readable(code,1)&&*code==0x16&&Put(frame,0x20,code+1));}
inline void Execute(void* object,void* frame,void* result){
    ++state.calls;Observation before{};
    const bool eligible=state.installed&&!state.closed&&!state.closing&&!state.error&&HostWorld()
        &&original_players::Readable(frame,0x90)&&Candidate(object,before);
    if(!eligible){++state.originalForwards;binding.original(object,frame,result);return;}
    if(!Live(binding.object,binding.identity)||Read<Exec>(binding.object,0xe0)!=binding.replacement||!Writable(frame,0x90)){
        Fail(8405,"hero_event_native_binding_or_untouched_frame_not_owned");++state.originalForwards;binding.original(object,frame,result);return;
    }
    void* type=nullptr;uint8_t mode=0;
    Require(Step(frame,binding.eventType,&type)&&Step(frame,binding.emitMode,&mode)&&End(frame),8406,"hero_event_partial_argument_evaluation_failed_no_replay");
    if(mode==1){Observe(before,type,mode);return;}
    // Original6FF6920 holds a map-element address across its native factory.
    // An actual nested mode0 request would rehash that same registry. Until
    // that original construction branch has a proven continuation, stop with
    // an explicit diagnostic rather than mutate it or fake a completed call.
    Require(!state.producerDepth||mode!=0,8417,"hero_event_pending_unproven_original_mode0_reentry_during_pool_factory");
    // All original non-target modes retain their exact real enum value and
    // typed implementation. No original exec replay after argument evaluation.
    ++state.typedForwards;engine::function<void(*)(void*,void*,uint8_t)>(0x6ff5f40)(object,type,mode);
}
inline LONG ObserveException(EXCEPTION_POINTERS* e){
    state.restartRequired=true;state.exceptionCode=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0;
    const auto address=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=address>=engine::image?address-engine::image:address;
    state.stage="hero_event_original_exception_propagated_requires_new_pair";RefreshRequestEvidence();Cache();return EXCEPTION_CONTINUE_SEARCH;
}
inline void RegisterExec(void* object,void* frame,void* result){
    // A non-GT call forwards before ANY non-atomic counter, active or cache write.
    if(!engine::image||!state.gameThread||state.gameThread!=GetCurrentThreadId()){
        binding.original(object,frame,result);return;
    }
    if(!Thread()){binding.original(object,frame,result);return;}
    ++state.active;__try{__try{Execute(object,frame,result);}__except(ObserveException(GetExceptionInformation())){}}
    __finally{--state.active;Cache();}
}
inline bool Verify(){
    for(const auto& p:local_server_hero_event_authority_source::Profiles){auto data=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!ImageCode(data)||!original_players::Readable(data,p.bytes))return Fail(8407,"hero_event_source_profile_unreadable");uint64_t hash=14695981039346656037ULL;
        for(uint32_t n=0;n<p.bytes;++n)hash=(hash^data[n])*1099511628211ULL;if(hash!=p.hash)return Fail(8407,"hero_event_source_profile_mismatch");
    }state.profiles=true;
    binding.object=engine::Find(L"/Script/ShooterGame.EquippableEventEquippableComponent.RegisterEvent");
    binding.original=engine::function<Exec>(0x6769430);binding.replacement=&RegisterExec;eventBase=engine::Class(0x67657b0);
    auto& m=state.metadata;m.function=binding.object;m.owner=Read<void*>(binding.object,0x28);m.exec=Read<uintptr_t>(binding.object,0xe0);
    m.flags=Read<uint32_t>(binding.object,0xb8);m.parms=Read<uint16_t>(binding.object,0xbe);
    original_jett_priming::PropertyInfo event{},mode{},set{};
    m.eventOffset=original_jett_priming::FieldOffset(binding.object,L"EventType",8,&event);m.modeOffset=original_jett_priming::FieldOffset(binding.object,L"EmitMode",1,&mode);
    m.setOffset=original_jett_priming::FieldOffset(engine::Class(0x6768f30),L"RegisteredEvents",0x50,&set);
    m.eventField=event.field;m.eventBytes=event.elementSize;m.eventFlags=Read<uint64_t>(event.field,0x38);
    m.modeField=mode.field;m.modeBytes=mode.elementSize;m.modeFlags=Read<uint64_t>(mode.field,0x38);
    m.setField=set.field;m.setBytes=set.elementSize;m.setFlags=Read<uint64_t>(set.field,0x38);
    // Runtime ParmsSize9 was read in case67. The static params container is16;
    // its tail padding is not an extra evaluated input.
    if(!binding.object||m.owner!=engine::Class(0x6768f30)||m.parms!=9||m.flags!=0x4020401U||m.exec!=engine::image+0x6769430
       ||m.eventOffset!=0||m.eventBytes!=8||event.arrayDim!=1||m.modeOffset!=8||m.modeBytes!=1||mode.arrayDim!=1
       ||m.eventFlags!=0x1c001000000280ULL||m.modeFlags!=0x18001040000280ULL
       ||m.setOffset!=0x138||m.setBytes!=0x50||set.arrayDim!=1||!eventBase
       ||!Writable(static_cast<uint8_t*>(binding.object)+0xe0,8)||!Capture(binding.object,binding.identity))
        return Fail(8408,"hero_event_original_runtime_schema_mismatch");
    binding.eventType=event.field;binding.emitMode=mode.field;state.schema=true;return true;
}
inline bool Init(void* world,const Hooks& value){
    if(!original_players::gameThread||GetCurrentThreadId()!=original_players::gameThread)return false;
    if(state.closed||state.closing||state.error||state.restartRequired)return false;
    if(state.initialized)return state.world==world&&state.installed&&!state.closed&&!state.error&&!state.restartRequired
        &&hooks.resolve==value.resolve&&hooks.validate==value.validate;
    if(!engine::image||!original_players::gameThread||GetCurrentThreadId()!=original_players::gameThread||!world
       ||!value.resolve||!value.validate||world!=local_server_match::GetState().world)return Fail(8409,"hero_event_init_requires_actual_host_and_composed_source_hooks");
    state.world=world;state.driver=Read<void*>(world,0x40);state.gameThread=GetCurrentThreadId();hooks=value;
    if(!Capture(world,state.worldId)||!Capture(state.driver,state.driverId)||!HostWorld())return Fail(8409,"hero_event_actual_host_weak_identity_missing");
    if(!Verify())return false;state.initialized=true;state.session=1;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(static_cast<uint8_t*>(binding.object)+0xe0),
        reinterpret_cast<void*>(binding.replacement),reinterpret_cast<void*>(binding.original))!=reinterpret_cast<void*>(binding.original))
        return Fail(8410,"hero_event_native_exec_publication_conflict");
    binding.installed=true;state.installed=true;state.stage="hero_event_original_register_boundary_installed_not_runtime_verified";Cache();return true;
}
inline bool SetPolicy(bool practice,bool skillsEnabled){
    if(!state.initialized||!Thread()||!state.installed||!state.schema||state.error||state.closed||state.closing)return false;
    state.policyKnown=true;state.practice=practice;state.skillsEnabled=skillsEnabled;
    state.stage=practice?"hero_event_practice_policy_observed":"hero_event_nonpractice_original_forwarding";Cache();return true;
}
inline bool RegisterHuman(void* pc,void* ps,void* pawn,unsigned hero,uint32_t epoch){
    if(!state.initialized||!Thread()||state.closed||state.closing||state.error||!HostWorld()||!hero||hero>=original_heroes::PlayableHeroCount||!epoch)return false;
    const auto p=local_server_match::Player(0);void* connection=nullptr;auto inventory=Read<void*>(pawn,0xc08);
    if(!p||!p->human||p->pc!=pc||p->ps!=ps||p->pawn!=pawn||p->epoch!=epoch||p->hero!=hero||!local_server_match::AuthorityPair(*p)
       ||hero!=local_server_hero_selection::SelectionIndex()||!local_server_item_ownership::ActualHuman(state.world,pc,ps,pawn,inventory,connection))
        return Fail(8411,"hero_event_requires_actual_session_nonjett_human_tuple");
    if(human.registered&&!human.retiring&&human.pc==pc&&human.ps==ps&&human.pawn==pawn&&human.hero==hero&&human.epoch==epoch)return HumanCurrent();
    if(human.registered&&!human.retiring)return Fail(8412,"hero_event_previous_human_epoch_not_fenced");
    Human next{};next.registered=true;next.pc=pc;next.ps=ps;next.pawn=pawn;next.inventory=inventory;next.hero=hero;next.epoch=epoch;
    next.heroType=engine::Find(original_heroes::Catalog[hero].path);next.actualType=Read<void*>(pawn,0x10);next.defaults=Read<void*>(next.actualType,0x120);
    if(!original_players::Derived(next.actualType,next.heroType)||!next.defaults||Read<void*>(next.defaults,0x10)!=next.actualType
       ||!Capture(pc,next.pcId)||!Capture(ps,next.psId)||!Capture(pawn,next.pawnId)||!Capture(inventory,next.inventoryId)
       ||!Capture(next.heroType,next.heroId)||!Capture(next.actualType,next.actualTypeId)||!Capture(next.defaults,next.defaultsId))
        return Fail(8413,"hero_event_original_human_cdo_weak_proof_missing");
    const auto authored=Read<framework::Array>(next.defaults,0xbd8),actual=Read<framework::Array>(pawn,0xbd8);
    if(!local_server_starting_equipment::ValidArray(authored,int32_t(MaxStarters))||!local_server_starting_equipment::ValidArray(actual,int32_t(MaxStarters))
       ||authored.count<=0||authored.count!=actual.count)return Fail(8414,"hero_event_original_starter_array_unproven");
    next.count=size_t(authored.count);
    for(size_t n=0;n<next.count;++n){auto& starter=next.starters[n];starter.type=authored.data[n];starter.defaults=Read<void*>(starter.type,0x120);
        if(starter.type!=actual.data[n]||!original_players::IsA(starter.type,0x1af0eb0)||!original_players::Derived(starter.type,engine::Class(0x645c660))
           ||!starter.defaults||Read<void*>(starter.defaults,0x10)!=starter.type||!original_players::IsA(starter.defaults,0x645c660)
           ||!Capture(starter.type,starter.typeId)||!Capture(starter.defaults,starter.defaultsId))
            return Fail(8414,"hero_event_original_starter_class_or_cdo_unproven");
        starter.slot=Read<uint8_t>(starter.defaults,0x4a0);if(starter.slot>=17)return Fail(8414,"hero_event_original_starter_slot_outside_native_enum");
    }
    if(state.humanRegistrations==UINT64_MAX)return Fail(8415,"hero_event_private_human_receipt_exhausted");next.receipt=++state.humanRegistrations;human=next;
    state.stage="hero_event_actual_nonjett_human_and_original_starters_observed";Cache();return true;
}
inline bool Retire(void* pawn,uint32_t epoch){
    if(!original_players::gameThread||GetCurrentThreadId()!=original_players::gameThread)return false;
    if(!state.initialized)return true;if(!Thread()||!pawn||!epoch)return false;
    if(human.registered&&human.pawn==pawn&&human.epoch==epoch){human.retiring=true;
        for(auto& r:requests)if(r.used&&r.observed.humanReceipt==human.receipt)RetireRequest(r,"original_source_epoch_retired_native_pool_lifetime_unchanged");
        ++state.retirements;state.stage="hero_event_source_fenced_without_native_pool_deletion";Cache();return true;
    }return true;
}
inline bool PollImpl(){
    if(!state.initialized)return !state.error;if(!Thread()||state.error||state.restartRequired)return false;
    if(state.closed||state.closing)return true;++state.polls;const auto now=GetTickCount64();if(now<state.nextPoll)return true;state.nextPoll=now+100;
    for(auto& r:requests)if(r.used&&!r.retired){
        if(!ObservationCurrent(r.observed)||!Live(r.type,r.typeId)){RetireRequest(r,"original_observed_source_or_class_expired");continue;}
        if(!r.ready&&!Complete(r))return false;
    }RefreshRequestEvidence();state.stage=state.pending?"hero_event_observed_requests_waiting_original_prerequisites":"hero_event_observed_registration_state_sampled_no_events_emitted";Cache();return true;
}
inline bool Poll(){
    if(!original_players::gameThread||GetCurrentThreadId()!=original_players::gameThread)return false;
    __try{return PollImpl();}__except(ObserveException(GetExceptionInformation())){return false;}
}
inline bool Close(){
    if(!original_players::gameThread||GetCurrentThreadId()!=original_players::gameThread)return false;
    ++state.closes;if(!state.initialized){state.closed=true;Cache();return true;}
    if(!Thread()||state.active||state.producerDepth)return false;state.closing=true;
    if(human.registered)Retire(human.pawn,human.epoch);
    if(binding.installed){
        if(!Live(binding.object,binding.identity)||Read<Exec>(binding.object,0xe0)!=binding.replacement){++state.foreignLayers;return Fail(8416,"hero_event_foreign_or_pendingkill_binding_retained");}
        if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(static_cast<uint8_t*>(binding.object)+0xe0),
            reinterpret_cast<void*>(binding.original),reinterpret_cast<void*>(binding.replacement))!=reinterpret_cast<void*>(binding.replacement)){
            ++state.foreignLayers;return Fail(8416,"hero_event_native_exec_restore_conflict");}
        binding.installed=false;++state.restores;
    }state.installed=false;state.closed=true;state.stage="hero_event_original_exec_restored_original_registered_pools_untouched";Cache();return true;
}
inline void WriteRequest(FILE* file,const RequestDiagnostic& d){
    const auto& a=d.observation;const auto& o=a.origin;
    std::fprintf(file,"{\"observed\":%s,\"ready\":%s,\"retired\":%s,\"stage\":\"%s\",\"original_mode\":%u,\"component\":\"%p\",\"ability\":\"%p\",\"ability_type\":\"%p\",\"event_type\":\"%p\",\"serial\":%llu,\"source_bound\":%s,\"actual_framework_registration\":%llu,\"actual_pending_grant\":%s,\"set_index\":%d,\"origin\":{\"world\":\"%p\",\"pc\":\"%p\",\"ps\":\"%p\",\"pawn\":\"%p\",\"inventory\":\"%p\",\"hero_type\":\"%p\",\"actual_pawn_type\":\"%p\",\"hero_cdo\":\"%p\",\"starter_cdo\":\"%p\",\"hero\":%u,\"epoch\":%u,\"private_human_receipt\":%llu},\"weak_order\":[\"component\",\"ability\",\"ability_type\",\"event_type\",\"world\",\"pc\",\"ps\",\"pawn\",\"inventory\",\"hero_type\",\"actual_pawn_type\",\"hero_cdo\",\"starter_cdo\"],\"weak\":[",
        d.observed?"true":"false",d.ready?"true":"false",d.retired?"true":"false",d.stage,unsigned(d.originalMode),a.component,a.ability,a.abilityType,d.type,
        d.serial,d.sourceBound?"true":"false",d.frameworkRegistration,d.pendingGrant?"true":"false",d.setIndex,
        o.world,o.pc,o.ps,o.pawn,o.inventory,o.heroType,o.actualType,o.heroDefaults,o.starterDefaults,o.hero,o.epoch,o.humanReceipt);
    const Weak values[]{a.componentId,a.abilityId,a.abilityTypeId,d.typeId,o.worldId,o.pcId,o.psId,o.pawnId,o.inventoryId,o.heroId,o.actualTypeId,o.heroDefaultsId,o.starterDefaultsId};
    for(size_t n=0;n<std::size(values);++n)std::fprintf(file,"%s[%d,%d]",n?",":"",values[n].index,values[n].serial);
    std::fputs("]}",file);
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path||!*path)return false;const auto s=Snapshot();FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;
    const auto b=[](bool v){return v?"true":"false";};const auto& m=s.metadata;
    std::fprintf(file,"{\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"installed\":%s,\"closed\":%s,\"closing\":%s,\"restart_required\":%s,\"profiles\":%s,\"schema\":%s,\"policy_known\":%s,\"practice\":%s,\"skills_enabled\":%s,\"world\":\"%p\",\"active\":%u,\"producer_depth\":%u,\"observed_mode1\":%llu,\"captured_requests\":%llu,\"ready\":%llu,\"pending\":%llu,\"duplicate_requests\":%llu,\"native_set_adds\":%llu,\"native_central_registrations\":%llu,\"set_readbacks\":%llu,\"pool_key_readbacks\":%llu,\"reentry_deferrals\":%llu,\"source_deferrals\":%llu,\"phase_transitions\":%llu,\"exception_code\":%u,\"exception_rva\":\"0x%llx\",",
        s.stage,s.error,b(s.initialized),b(s.installed),b(s.closed),b(s.closing),b(s.restartRequired),b(s.profiles),b(s.schema),b(s.policyKnown),b(s.practice),b(s.skillsEnabled),s.world,s.active,s.producerDepth,
        s.observedMode1,s.captured,s.ready,s.pending,s.duplicateRequests,s.nativeSetAdds,s.nativeCentralCalls,s.setReadbacks,s.poolKeyReadbacks,s.reentryDeferrals,s.sourceDeferrals,s.phaseTransitions,
        s.exceptionCode,static_cast<unsigned long long>(s.exceptionRva));
    std::fprintf(file,"\"metadata\":{\"function\":\"%p\",\"owner\":\"%p\",\"exec\":\"0x%llx\",\"flags\":\"0x%x\",\"parms_size\":%u,\"event_offset\":%d,\"event_bytes\":%d,\"event_flags\":\"0x%llx\",\"mode_offset\":%d,\"mode_bytes\":%d,\"mode_flags\":\"0x%llx\",\"set_offset\":%d,\"set_bytes\":%d,\"set_flags\":\"0x%llx\"},\"actual_original_mode_retained\":true,\"listener_map_scans\":0,\"beginplay_replays\":0,\"events_emitted\":0,\"resource_or_ps_writes\":0,\"pool_deletions\":0,\"runtime_verified\":false,\"all_hero_skills_verified\":false,",
        m.function,m.owner,static_cast<unsigned long long>(m.exec),m.flags,unsigned(m.parms),m.eventOffset,m.eventBytes,static_cast<unsigned long long>(m.eventFlags),
        m.modeOffset,m.modeBytes,static_cast<unsigned long long>(m.modeFlags),m.setOffset,m.setBytes,static_cast<unsigned long long>(m.setFlags));
    std::fputs("\"first_request\":",file);WriteRequest(file,s.firstRequest);std::fputs(",\"last_request\":",file);WriteRequest(file,s.lastRequest);std::fputs("}\n",file);
    const bool ok=std::ferror(file)==0;return std::fclose(file)==0&&ok;
}
} // namespace local_server_hero_event_authority
