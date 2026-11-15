#pragma once
#include "local_server_hero_abilities.h"
#include <array>
#include <cstdio>
#include <cstring>

// Candidate local-host producers only. RegisterEvent's retained native body
// omits ServerOnly=1, although both the requested real class and its original
// registration/pool implementations remain. Fill that missing branch for a
// live registered original skill; never emit an event or manufacture a class.
// Resource initialization is an explicit practice policy, using actual native
// resource APIs and the real PlayerState ultimate record, not client copies.
// Source proof: work/local-server-playable/hero-abilities/authority-init/.
namespace local_server_hero_authority_init {
using Weak=original_inventory_slots::WeakIdentity;
using Source=local_server_hero_abilities::SourceView;
using Exec=void(*)(void*,void*,void*);
inline constexpr size_t MaxRequests=256,MaxResourceItems=64,MaxMapClasses=512;
struct Hooks {
    bool (*resolve)(void*,Source&)=nullptr;
    bool (*validate)(const Source&)=nullptr;
};
struct Function {
    void* object=nullptr;Weak identity{};Exec original=nullptr,replacement=nullptr;
    std::array<void*,2> parameters{};uint8_t count=0;bool installed=false;
};
struct EventRequest {
    bool used=false,ready=false;void *component=nullptr,*type=nullptr;
    Weak componentId{},typeId{};Source source{};
};
struct ResourceItem {
    bool used=false,chargesInitialized=false,ultimateInitialized=false,ultimateOwnerReady=false;
    void *actor=nullptr,*resource=nullptr;Weak actorId{},resourceId{};Source source{};
    int32_t chargesBefore=-1,maximum=-1,chargesAfter=-1,ultimateRequired=-1,pointsBefore=-1,pointsAfter=-1;
    uint64_t nativeReplenishes=0;const char* stage="hero_resource_item_not_initialized";
};
struct State {
    bool initialized=false,installed=false,host=false,practice=false,profiles=false,schema=false,closed=false;
    unsigned error=0,active=0;DWORD gameThread=0;void* world=nullptr;Weak worldId{};
    uint64_t installs=0,closes=0,execCalls=0,originalForwards=0,eligibleCalls=0,frameSteps=0,scriptSteps=0,compiledSteps=0;
    uint64_t serverOnlyRequests=0,nativeRegistrations=0,poolReceipts=0,poolDeferrals=0,mapDiscoveries=0,mapSamples=0;
    uint64_t nativeMode0=0,nativeMode2=0,sourceDeferrals=0,retiredRequests=0,retiredResources=0;
    uint64_t chargeItemsInitialized=0,nativeReplenishes=0,ultimateInitializations=0,ultimateConsumes=0,ultimateActiveUpdates=0;
    uint64_t originalOwnerCallbacks=0,pointNotifications=0,polls=0,unsupportedResources=0;
    const char* stage="hero_authority_init_not_initialized";
};
struct Profile{uintptr_t rva;size_t bytes;uint64_t fnv;};
inline constexpr Profile Profiles[]{
    {0x6769430,467,0xd1abc42e4378607dULL},{0x6ff5f40,101,0xeb154c7c237fd429ULL},
    {0x6ff68d0,77,0x08971fb2c88a16baULL},{0x6ff6000,1609,0x432b5aad0a634331ULL},
    {0x40c52d0,901,0x668627b81d0344b8ULL},{0x3b866f0,20,0x09fe87de40670e75ULL},
    {0x36ab340,204,0xf250d2336a5eef37ULL},{0x6cc46a0,122,0x4efc8f40f2d1cb4cULL},
    {0x6cc4720,99,0x509936e4b283ea64ULL},{0x653b9b0,54,0x1d78983e12a69fc8ULL},
    {0x637fea0,31,0x39e4fedf016a5ebeULL},{0x6b95970,258,0xb03cb98ad507ee92ULL},
    {0x6f7e3e0,71,0x5636022b717c0729ULL},{0x6f7e430,70,0xe366246d57c069a8ULL},
    {0x6f7e480,195,0x82ab1dcb9f50f158ULL}
};
inline State state{},cached{};inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline Hooks hooks{};inline std::array<Function,4> functions{};
inline std::array<EventRequest,MaxRequests> requests{};
inline std::array<ResourceItem,MaxResourceItems> resourceItems{};
inline void* listenersProperty=nullptr;inline void* listenersKey=nullptr;
inline original_jett_priming::PropertyInfo ultimateActiveProperty{};
inline const State& GetState(){return state;}
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Live(void* p,const Weak& id){return p&&original_inventory_slots::ResolveIdentity(id)==p;}
inline bool Capture(void* p,Weak& id){return original_inventory_slots::CaptureIdentity(p,id)&&Live(p,id);}
inline void Cache(){AcquireSRWLockExclusive(&cacheLock);cached=state;ReleaseSRWLockExclusive(&cacheLock);}
inline State Snapshot(){AcquireSRWLockShared(&cacheLock);auto s=cached;ReleaseSRWLockShared(&cacheLock);return s;}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;Cache();return false;}
inline bool Thread(){return state.initialized&&!state.closed&&original_players::Thread()&&state.gameThread==GetCurrentThreadId();}
inline bool ImageCode(const void* p){
    MEMORY_BASIC_INFORMATION m{};if(!p||!VirtualQuery(p,&m,sizeof(m)))return false;
    const auto protection=m.Protect&0xffU;
    return m.State==MEM_COMMIT&&m.Type==MEM_IMAGE&&m.AllocationBase==reinterpret_cast<void*>(engine::image)
        &&!(m.Protect&(PAGE_GUARD|PAGE_NOACCESS))&&(protection==PAGE_EXECUTE||protection==PAGE_EXECUTE_READ
            ||protection==PAGE_EXECUTE_READWRITE||protection==PAGE_EXECUTE_WRITECOPY);
}
inline bool Writable(const void* p,size_t bytes){
    MEMORY_BASIC_INFORMATION m{};if(!p||!VirtualQuery(p,&m,sizeof(m)))return false;
    const auto at=reinterpret_cast<uintptr_t>(p),begin=reinterpret_cast<uintptr_t>(m.BaseAddress);const auto protection=m.Protect&0xffU;
    return m.State==MEM_COMMIT&&m.Type==MEM_PRIVATE&&!(m.Protect&(PAGE_GUARD|PAGE_NOACCESS))
        &&(protection==PAGE_READWRITE||protection==PAGE_EXECUTE_READWRITE)&&at>=begin
        &&bytes<=m.RegionSize&&at-begin<=m.RegionSize-bytes;
}
inline bool Event(void* object,const wchar_t* path,uint16_t bytes,void* params){
    auto fn=engine::Find(path),call=Read<void*>(Read<void*>(object),0x298);
    if(!object||!fn||Read<uint16_t>(fn,0xbe)!=bytes||!call)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(call)(object,fn,params);return true;
}
inline bool Resolve(void* object,Source& source){
    return hooks.resolve?hooks.resolve(object,source):local_server_hero_abilities::ResolveSource(object,source);
}
inline bool Validate(const Source& source){
    return source.world==state.world&&Live(state.world,state.worldId)
        &&(hooks.validate?hooks.validate(source):local_server_hero_abilities::ValidateSource(source));
}
inline bool HostWorld(){auto driver=Read<void*>(state.world,0x40);
    return state.host&&driver&&Read<void*>(driver,0x178)==state.world&&!Read<void*>(driver,0xc0);}
inline bool EventSource(void* component,Source& source){
    if(!Thread()||!state.host||!state.practice||!HostWorld()||!original_players::IsA(component,0x6768f30)
       ||!Resolve(component,source)||!Validate(source)||!source.ability||Read<void*>(component,0xc8)!=source.ability)return false;
    auto registry=Read<void*>(component,0x198),gs=Read<void*>(state.world,0x178);
    if(!original_players::IsA(gs,0x6c9d280))return false;
    if(registry&&(registry!=Read<void*>(gs,0xce0)||!original_players::IsA(registry,0x6769940)
        ||Read<void*>(registry,0xc8)!=gs))return false;
    return true;
}
inline bool EventClass(void* type){return original_players::IsA(type,0x1af0eb0)
    &&original_players::Derived(type,engine::Class(0x67657b0));}
inline bool PoolHas(void* component,void* type){
    auto registry=Read<void*>(component,0x198);return registry
        &&engine::function<bool(*)(void*,void*)>(0x6ff68d0)(registry,type);
}
inline EventRequest* Request(void* component,void* type,const Source& source){
    Weak componentId{},typeId{};if(!Capture(component,componentId)||!Capture(type,typeId))return nullptr;
    EventRequest* empty=nullptr;
    for(auto& request:requests){
        if(request.used&&(!Live(request.component,request.componentId)||!Live(request.type,request.typeId)
            ||!Validate(request.source))){request={};++state.retiredRequests;}
        if(request.used&&request.component==component&&request.type==type
           &&original_inventory_slots::SameIdentity(request.componentId,componentId))return &request;
        if(!request.used&&!empty)empty=&request;
    }
    if(!empty){Fail(8110,"hero_authority_event_request_capacity");return nullptr;}
    empty->used=true;empty->component=component;empty->type=type;empty->componentId=componentId;empty->typeId=typeId;empty->source=source;
    return empty;
}
inline bool RegisterActual(void* component,void* type,const Source& source,bool serverOnly){
    if(!EventClass(type)||!Validate(source))return Fail(8111,"hero_authority_requested_event_class_or_source_invalid");
    auto request=Request(component,type,source);if(!request)return false;
    if(serverOnly)++state.serverOnlyRequests;
    // Even an already-existing central pool needs this actual item's original
    // RegisteredEvents set. The original native TSet add is idempotent.
    engine::function<void(*)(void*,void*,uint8_t)>(0x6ff5f40)(component,type,0);++state.nativeRegistrations;
    if(!Live(component,request->componentId)||!Live(type,request->typeId)||!Validate(source))
        return Fail(8112,"hero_authority_source_retired_during_original_event_registration");
    const bool ready=PoolHas(component,type);
    if(ready&&!request->ready)++state.poolReceipts;
    request->ready=ready;if(!ready)++state.poolDeferrals;return true;
}
// Read the original native FScriptMap key layout. No guessed pair stride and
// no differently typed temporary FArrayProperty is supplied to a generic API.
// The exact native Map_Keys implementation establishes +9C element stride,
// +A0 allocator flag and the ordinary sparse-array allocation bit layout.
inline bool ListenerClasses(void* component,std::array<void*,MaxMapClasses>& classes,size_t& count){
    count=0;auto map=static_cast<uint8_t*>(component)+0xd8;
    if(Read<uint8_t>(listenersProperty,0xa0)&1)return Fail(8113,"hero_authority_memory_image_listener_map_not_profiled");
    const auto stride=Read<int32_t>(listenersProperty,0x9c),slots=Read<int32_t>(map,8),capacity=Read<int32_t>(map,12);
    const auto free=Read<int32_t>(map,0x34),bitsCount=Read<int32_t>(map,0x28),bitsCapacity=Read<int32_t>(map,0x2c);
    if(stride<8||stride>512||slots<0||slots>static_cast<int32_t>(MaxMapClasses)||capacity<slots||capacity>8192
       ||free<0||free>slots||bitsCount!=slots||bitsCapacity<bitsCount)
        return Fail(8114,"hero_authority_actual_listener_map_layout_invalid");
    auto data=Read<uint8_t*>(map);auto bits=Read<const uint32_t*>(map,0x20);
    if(!bits)bits=reinterpret_cast<const uint32_t*>(map+0x10);
    if((slots&&!original_players::Readable(data,size_t(slots)*size_t(stride)))
       ||(slots&&!original_players::Readable(bits,size_t((slots+31)/32)*4)))
        return Fail(8115,"hero_authority_actual_listener_map_storage_unreadable");
    for(int32_t i=0;i<slots;++i){if(!(bits[i/32]&(1U<<(i%32))))continue;
        auto type=Read<void*>(data,size_t(i)*size_t(stride));
        if(!EventClass(type))return Fail(8116,"hero_authority_listener_key_not_original_event_class");classes[count++]=type;}
    return count==static_cast<size_t>(slots-free)||Fail(8117,"hero_authority_listener_map_live_count_mismatch");
}
inline bool ScanComponent(void* component){
    Source source{};if(!EventSource(component,source)){++state.sourceDeferrals;return true;}
    std::array<void*,MaxMapClasses> classes{};size_t count=0;
    if(!ListenerClasses(component,classes,count))return false;++state.mapSamples;
    for(size_t i=0;i<count;++i){const bool existing=PoolHas(component,classes[i]);
        if(!RegisterActual(component,classes[i],source,false))return false;if(!existing)++state.mapDiscoveries;}
    return true;
}
inline bool CopyField(void* field,void* destination,const void* container){
    auto copy=Read<void*>(Read<void*>(field),0x110);const auto bytes=Read<int32_t>(field,0x34),offset=Read<int32_t>(field,0x44);
    if(!field||!destination||!container||bytes<=0||bytes>8||offset<0||!ImageCode(copy)
       ||!original_players::Readable(static_cast<const uint8_t*>(container)+offset,static_cast<size_t>(bytes)))return false;
    reinterpret_cast<void(*)(void*,void*,const void*)>(copy)(field,destination,container);return true;
}
inline bool Step(void* frame,void* field,void* destination){
    auto code=Read<uint8_t*>(frame,0x20);
    if(code){
        if(!original_players::Readable(code,1))return false;
        const auto opcode=*code;auto native=Read<Exec>(reinterpret_cast<void*>(engine::image+0xd747c40),size_t(opcode)*8);
        if(!ImageCode(reinterpret_cast<void*>(native))||!original_players::Write(frame,0x20,code+1))return false;
        native(Read<void*>(frame,0x18),frame,destination);++state.scriptSteps;
    }else{
        if(Read<void*>(frame,0x88)!=field||Read<uint64_t>(field,0x38)&0x100ULL
           ||!original_players::Write(frame,0x88,Read<void*>(field,0x18)))return false;
        auto locals=Read<uint8_t*>(frame,0x28);const auto offset=Read<int32_t>(field,0x44);
        if(!locals||offset<0||!original_players::Write<void*>(frame,0x40,locals)
           ||!original_players::Write<void*>(frame,0x38,locals+offset)||!CopyField(field,destination,locals))return false;
        ++state.compiledSteps;
    }
    ++state.frameSteps;return true;
}
inline bool EndFrame(void* frame){auto code=Read<uint8_t*>(frame,0x20);return !code
    ||(original_players::Readable(code,1)&&*code==0x16&&original_players::Write(frame,0x20,code+1));}
inline bool StateSource(void* ps,Source& source){
    for(const auto& player:local_server_hero_abilities::GetPlayers())
        if(player.used&&!player.retired&&player.authority&&player.context.playerState==ps
           &&Live(ps,player.context.playerStateId)&&Resolve(player.context.pawn,source)&&Validate(source)
           &&source.playerState==ps&&source.sourceEpoch==player.epoch)return true;
    return false;
}
inline bool NotifyPoints(void* ps){
    if(!Event(ps,L"/Script/ShooterGame.ShooterPlayerState.OnRep_NumUltimatePoints",0,nullptr))
        return Fail(8118,"hero_authority_original_ultimate_points_notification_failed");++state.pointNotifications;return true;
}
inline bool SetPoints(const Source& source,int32_t points){
    if(!Validate(source)||points<0||points>100)return Fail(8119,"hero_authority_ultimate_record_requires_actual_source_and_bounded_value");
    if(Read<int32_t>(source.playerState,0xbdc)==points)return true;
    if(!original_players::Write(source.playerState,0xbdc,points)||Read<int32_t>(source.playerState,0xbdc)!=points)
        return Fail(8120,"hero_authority_ultimate_host_record_readback_failed");
    return NotifyPoints(source.playerState);
}
inline bool SetActive(const Source& source,uint8_t value){
    if(!Validate(source)||value>1)return Fail(8121,"hero_authority_ultimate_active_requires_actual_source");
    const auto offset=size_t(ultimateActiveProperty.offset)+ultimateActiveProperty.boolByteOffset;
    const auto before=Read<uint8_t>(source.playerState,offset),mask=ultimateActiveProperty.boolFieldMask;
    const auto desired=static_cast<uint8_t>((before&~mask)|(value?ultimateActiveProperty.boolByteMask:0));if(before==desired)return true;
    if(!original_players::Write(source.playerState,offset,desired)
       ||!Event(source.playerState,L"/Script/ShooterGame.ShooterPlayerState.OnRep_UltimateActive",0,nullptr))
        return Fail(8122,"hero_authority_original_ultimate_active_notification_failed");++state.ultimateActiveUpdates;return true;
}
inline void ExecuteImpl(size_t index,void* object,void* frame,void* result){
    auto& f=functions[index];++state.execCalls;Source source{};
    const bool eligible=state.installed&&!state.closed&&!state.error&&Thread()&&HostWorld()&&state.practice
        &&(index==0?EventSource(object,source):StateSource(object,source));
    if(!eligible){++state.originalForwards;f.original(object,frame,result);return;}
    if(!Live(f.object,f.identity)||Read<Exec>(f.object,0xe0)!=f.replacement||!Writable(frame,0x90)){
        Fail(8123,"hero_authority_owned_native_exec_binding_or_frame_changed");return;}
    ++state.eligibleCalls;
    if(index==0){void* type=nullptr;uint8_t mode=0;
        if(!Step(frame,f.parameters[0],&type)||!Step(frame,f.parameters[1],&mode)||!EndFrame(frame)){
            Fail(8124,"hero_authority_original_register_event_argument_frame_invalid");return;}
        if(mode==1){RegisterActual(object,type,source,true);return;}
        // The exact same retained implementation remains authoritative for
        // ServerAndClient=0, ClientOnly=2 and any original invalid-mode no-op.
        engine::function<void(*)(void*,void*,uint8_t)>(0x6ff5f40)(object,type,mode);
        if(mode==0)++state.nativeMode0;else if(mode==2)++state.nativeMode2;return;
    }
    if(index==1){if(!EndFrame(frame)){Fail(8125,"hero_authority_original_ultimate_consume_frame_invalid");return;}
        if(SetPoints(source,0))++state.ultimateConsumes;return;}
    uint8_t active=0;
    if(!Step(frame,f.parameters[0],&active)||active>1||!EndFrame(frame)){
        Fail(8126,"hero_authority_original_ultimate_active_argument_frame_invalid");return;}
    SetActive(source,active);
}
inline void Execute(size_t index,void* object,void* frame,void* result){
    ++state.active;__try{ExecuteImpl(index,object,frame,result);}__finally{--state.active;Cache();}
}
inline void RegisterExec(void* o,void* f,void* r){Execute(0,o,f,r);}
inline void ConsumeExec(void* o,void* f,void* r){Execute(1,o,f,r);}
inline void ActiveExec(void* o,void* f,void* r){Execute(2,o,f,r);}
inline void UltimateActiveExec(void* o,void* f,void* r){Execute(3,o,f,r);}
inline bool Parameter(Function& f,size_t index,const wchar_t* name,int32_t offset,int32_t size){
    original_jett_priming::PropertyInfo p{};
    if(original_jett_priming::FieldOffset(f.object,name,size_t(size),&p)!=offset||p.arrayDim!=1||p.elementSize!=size
       ||!(Read<uint64_t>(p.field,0x38)&0x80ULL)||Read<uint64_t>(p.field,0x38)&0x100ULL)return false;
    f.parameters[index]=p.field;return true;
}
inline bool Bind(size_t index,const wchar_t* path,uint16_t bytes,uintptr_t owner,uintptr_t exec,Exec replacement){
    auto& f=functions[index];f.object=engine::Find(path);f.original=engine::function<Exec>(exec);f.replacement=replacement;
    return f.object&&Read<void*>(f.object,0x28)==engine::Class(owner)&&Read<uint16_t>(f.object,0xbe)==bytes
        &&(Read<uint32_t>(f.object,0xb8)&0x400U)&&Read<Exec>(f.object,0xe0)==f.original
        &&Writable(static_cast<uint8_t*>(f.object)+0xe0,8)&&Capture(f.object,f.identity);
}
inline bool Verify(){
    if(state.schema)return true;
    for(const auto& profile:Profiles){auto bytes=reinterpret_cast<const uint8_t*>(engine::image+profile.rva);
        if(!ImageCode(bytes)||!original_players::Readable(bytes,profile.bytes))return Fail(8127,"hero_authority_original_native_profile_unreadable");
        uint64_t h=0xcbf29ce484222325ULL;for(size_t i=0;i<profile.bytes;++i)h=(h^bytes[i])*0x100000001b3ULL;
        if(h!=profile.fnv)return Fail(8128,"hero_authority_original_native_profile_mismatch");}
    state.profiles=true;
    if(!Bind(0,L"/Script/ShooterGame.EquippableEventEquippableComponent.RegisterEvent",16,0x6768f30,0x6769430,&RegisterExec)
       ||!Bind(1,L"/Script/ShooterGame.ShooterPlayerState.AuthConsumeUltimatePoints",0,original_players::ShooterStateClass,0x3b866f0,&ConsumeExec)
       ||!Bind(2,L"/Script/ShooterGame.ShooterPlayerState.AuthSetUltActive",1,original_players::ShooterStateClass,0x36ab340,&ActiveExec)
       ||!Bind(3,L"/Script/ShooterGame.ShooterPlayerState.AuthSetUltimateActive",1,original_players::ShooterStateClass,0x36ab340,&UltimateActiveExec)
       ||!Parameter(functions[0],0,L"EventType",0,8)||!Parameter(functions[0],1,L"EmitMode",8,1)
       ||!Parameter(functions[2],0,L"UltActive",0,1)||!Parameter(functions[3],0,L"bNewUltimateActive",0,1))
        return Fail(8129,"hero_authority_original_native_function_parameter_schema_mismatch");
    original_jett_priming::PropertyInfo listeners{},points{};
    if(original_jett_priming::FieldOffset(engine::Class(0x67658a0),L"EventListeners",80,&listeners)!=0xd8
       ||listeners.arrayDim!=1||listeners.elementSize!=80
       ||original_jett_priming::FieldOffset(engine::Class(original_players::ShooterStateClass),L"NumUltimatePoints",4,&points)!=0xbdc
       ||points.arrayDim!=1||points.elementSize!=4||(Read<uint64_t>(points.field,0x38)&0x20ULL)==0
       ||original_jett_priming::FieldOffset(engine::Class(original_players::ShooterStateClass),L"bUltimateActive",1,&ultimateActiveProperty)!=0xbd8
       ||ultimateActiveProperty.boolByteOffset!=0||ultimateActiveProperty.boolFieldMask!=0xff)
        return Fail(8130,"hero_authority_original_event_map_or_ultimate_record_schema_mismatch");
    listenersProperty=listeners.field;listenersKey=Read<void*>(listenersProperty,0x78);
    if(!listenersKey||Read<int32_t>(listenersKey,0x30)!=1||Read<int32_t>(listenersKey,0x34)!=8
       ||Read<void*>(listenersKey,0x80)!=engine::Class(0x67657b0))
        return Fail(8131,"hero_authority_original_listener_key_must_be_equippable_event_class");
    state.schema=true;return true;
}
inline bool Close(){
    if(!state.initialized)return true;
    if(!original_players::Thread()||state.active)return Fail(8132,"hero_authority_close_requires_idle_original_game_thread");
    for(size_t i=functions.size();i>0;--i){auto& f=functions[i-1];if(!f.installed)continue;
        if(!Live(f.object,f.identity)||Read<Exec>(f.object,0xe0)!=f.replacement
           ||InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(static_cast<uint8_t*>(f.object)+0xe0),
                reinterpret_cast<void*>(f.original),reinterpret_cast<void*>(f.replacement))!=reinterpret_cast<void*>(f.replacement))
            return Fail(8133,"hero_authority_original_native_exec_restore_conflict");f.installed=false;}
    state.installed=false;state.closed=true;++state.closes;state.stage="hero_authority_original_execs_restored";Cache();return true;
}
inline bool Init(void* world,bool host,bool practice,const Hooks& value={}){
    if(!engine::image||!world||!original_players::Thread())return false;
    if(state.initialized)return state.world==world&&state.host==host&&state.practice==practice&&!state.closed;
    state.gameThread=GetCurrentThreadId();state.world=world;state.host=host;state.practice=practice;hooks=value;
    if(!Capture(world,state.worldId))return Fail(8134,"hero_authority_original_world_identity_unavailable");
    state.initialized=true;
    if(!practice){state.stage="hero_authority_practice_disabled";Cache();return true;}
    if(!Verify())return false;
    if(host){if(!HostWorld())return Fail(8135,"hero_authority_requires_actual_native_listen_host");
        for(auto& f:functions){
            if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(static_cast<uint8_t*>(f.object)+0xe0),
                reinterpret_cast<void*>(f.replacement),reinterpret_cast<void*>(f.original))!=reinterpret_cast<void*>(f.original)){
                Fail(8136,"hero_authority_original_native_exec_publication_conflict");Close();return false;}f.installed=true;}
        state.installed=true;++state.installs;
    }
    state.stage=host?"hero_authority_source_scoped_original_execs_installed":"hero_authority_client_observation_only";Cache();return true;
}
inline bool Poll(){
    if(!Thread()||state.error)return false;
    if(!state.practice||!state.host)return true;
    if(!HostWorld()||!Live(state.world,state.worldId))return Fail(8137,"hero_authority_actual_host_world_retired");
    if(!local_server_hero_abilities::VisitRegisteredComponents([](const Source&,void* component){
        return !original_players::IsA(component,0x6768f30)||ScanComponent(component);}))return false;
    for(auto& request:requests){if(!request.used)continue;
        if(!Live(request.component,request.componentId)||!Live(request.type,request.typeId)||!Validate(request.source)){
            request={};++state.retiredRequests;continue;}
        if(!request.ready&&!RegisterActual(request.component,request.type,request.source,false))return false;}
    ++state.polls;state.stage="hero_authority_actual_event_registrations_observed_execution_unverified";Cache();return true;
}
inline bool RetirePlayer(void* pawn,uint32_t epoch){
    if(!Thread())return false;
    for(auto& r:requests)if(r.used&&r.source.pawn==pawn&&r.source.sourceEpoch==epoch){r={};++state.retiredRequests;}
    for(auto& r:resourceItems)if(r.used&&r.source.pawn==pawn&&r.source.sourceEpoch==epoch){r={};++state.retiredResources;}
    Cache();return true;
}
inline void Write(FILE* file){
    const auto s=Snapshot();auto b=[](bool v){return v?"true":"false";};
    std::fprintf(file,"{\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"host\":%s,\"practice\":%s,\"installed\":%s,\"profiles\":%s,\"schema\":%s,\"active\":%u,\"exec_calls\":%llu,\"original_forwards\":%llu,\"eligible_calls\":%llu,\"server_only_requests\":%llu,\"native_registrations\":%llu,\"pool_receipts\":%llu,\"pool_deferrals\":%llu,\"map_samples\":%llu,\"map_discoveries\":%llu,\"source_deferrals\":%llu,\"retired_requests\":%llu,\"retired_resources\":%llu,\"native_mode0\":%llu,\"native_mode2\":%llu,\"frame_steps\":%llu,\"script_steps\":%llu,\"compiled_steps\":%llu,\"charge_items_initialized\":%llu,\"native_replenishes\":%llu,\"ultimate_initializations\":%llu,\"ultimate_consumes\":%llu,\"ultimate_active_updates\":%llu,\"point_notifications\":%llu,\"original_owner_callbacks\":%llu,\"client_resource_writes\":0,\"synthetic_event_emissions\":0,\"network_execution_verified\":false}",
        s.stage,s.error,b(s.initialized),b(s.host),b(s.practice),b(s.installed),b(s.profiles),b(s.schema),s.active,
        s.execCalls,s.originalForwards,s.eligibleCalls,s.serverOnlyRequests,s.nativeRegistrations,s.poolReceipts,s.poolDeferrals,
        s.mapSamples,s.mapDiscoveries,s.sourceDeferrals,s.retiredRequests,s.retiredResources,s.nativeMode0,s.nativeMode2,
        s.frameSteps,s.scriptSteps,s.compiledSteps,s.chargeItemsInitialized,s.nativeReplenishes,s.ultimateInitializations,
        s.ultimateConsumes,s.ultimateActiveUpdates,s.pointNotifications,s.originalOwnerCallbacks);
}
} // namespace local_server_hero_authority_init
