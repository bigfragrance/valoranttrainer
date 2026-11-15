#pragma once
#include "original_inventory_slots.h"
#include "original_jett_priming.h"
#include "local_server_item_ownership.h"
#include <array>
#include <cstdio>

// Missing host PlayerState records only. This is not a request parser, an
// ultimate-cost policy, a replacement serializer or a client receipt. Original
// OnRep delegates and the actual native changelist comparator retain ownership.
// Evidence: work/local-server-playable/player-state-authority/.
namespace local_server_player_state_authority {
using Weak=original_inventory_slots::WeakIdentity;
inline constexpr size_t MaxSources=32;
struct Source {void *world=nullptr,*controller=nullptr,*playerState=nullptr,*pawn=nullptr;uint32_t sourceEpoch=0;};
struct Hooks {bool (*validate)(const Source&)=nullptr;};
enum class Status:uint8_t {Failed,Waiting,Ready};
enum class Field:uint8_t {Points,Active};
struct Publication {
    Status status=Status::Waiting;unsigned error=0;
    bool recordWritten=false,notified=false,nativePublished=false,nativeShadowMatches=false,deliveryVerified=false;
    bool Ready()const{return status==Status::Ready;}
};
struct Array {uint8_t* data=nullptr;int32_t count=0,capacity=0;};
struct Transaction {
    Source source{};Field field=Field::Points;uint64_t registration=0,ticket=0;
    void *channel=nullptr,*replicator=nullptr,*layout=nullptr,*manager=nullptr,*sendState=nullptr;
    void *property=nullptr,*parent=nullptr,*command=nullptr,*shadow=nullptr,*shadowData=nullptr;
    void *parents=nullptr,*commands=nullptr,*activeTracker=nullptr,*activeBits=nullptr;
    Weak channelId{};
    uint16_t repIndex=0xffff,parentIndex=0xffff,commandStart=0,commandEnd=0,handle=0;
    int32_t offset=0,bytes=0,condition=-1,before=0,shadowBefore=0;
    int32_t parentCount=0,commandCount=0,shadowOffset=0,shadowNum=0,shadowCapacity=0,activeNum=0,activeMax=0;
    uint8_t commandType=0,layoutFlags=0,connectionMode=255;
    uint32_t replicatorFlags=0,nativeFlags=0,replicationFrame=0;
    uint32_t historyStart=0,historyEndBefore=0,compareBefore=0,sendHistoryBefore=0,sendCompareBefore=0;
    bool prepared=false,committed=false,pushPolicyKnown=false,pushBased=false,originalActiveBypass=false;
};
struct FieldObservation {
    Publication last{};const char* stage="ps_authority_field_not_requested";
    void *property=nullptr,*channel=nullptr,*replicator=nullptr,*layout=nullptr,*sendState=nullptr,*manager=nullptr;
    uint16_t repIndex=0xffff,parentIndex=0xffff,handle=0;uint8_t commandType=0;
    int32_t condition=-1,before=0,after=0,shadowBefore=0,shadowAfter=0,nativeResult=-1;
    uint32_t nativeFlags=0,layoutFlags=0,replicatorFlags=0;
    uint32_t historyBefore=0,historyAfter=0,compareBefore=0,compareAfter=0,replicationFrame=0;
    bool pushPolicyKnown=false,pushBased=false,handleObserved=false,terminatorObserved=false;
    int32_t activeNum=0,activeMax=0;bool originalActiveBypass=false;
    uint64_t requests=0,deferrals=0,writes=0,notifications=0,compares=0,historyWitnesses=0,unchanged=0;
};
struct Observation {
    Source source{};uint64_t registration=0;bool registered=false,sourceValid=false,busy=false,retired=false;
    int32_t points=0;bool active=false;
    std::array<FieldObservation,2> fields{};
};
struct Record {
    Source source{};std::array<Weak,8> identities{};
    void *connection=nullptr,*driver=nullptr,*inventory=nullptr,*actualClass=nullptr;
    void* selectedClass=nullptr;Weak selectedClassId{};
    uint64_t registration=0;bool used=false,busy=false;
    Transaction pending{};Observation observation{};Publication inFlight{};Field inFlightField=Field::Points;
};
struct ContextWeakObservation {void *expected=nullptr,*resolved=nullptr;Weak identity{};bool evaluated=false,matches=false;};
struct ContextObservation {
    uint64_t sequence=0;const char* operation="none";uint32_t epoch=0,actualThread=0,expectedThread=0;
    uint32_t passed=0,required=(1U<<18)-1U;bool result=false,nativeQueriesSafe=false,validatorCalled=false;
    bool enginePresent=false,moduleInitialized=false,moduleClosed=false,restartRequired=false,originalThreadCalled=false,originalThread=false;unsigned moduleError=0;
    bool netModeCalled=false,actualHumanCalled=false,registeredCalled=false,derivedCalled=false,derivedFromSchema=false;
    int32_t netMode=-1;bool validatorResult=false,actualHumanResult=false,registeredResult=false;
    Source source{};std::array<ContextWeakObservation,8> weak{};
    ContextWeakObservation selectedWeak{},schemaWeak{};
    bool classEqualsNativeSchema=false,classEqualsSelected=false,selectedFactoryUnchanged=false;
    void *originalWorld=nullptr,*moduleWorld=nullptr,*expectedDriver=nullptr,*expectedConnection=nullptr,*actualHumanConnection=nullptr;
    void *worldDriver=nullptr,*driverWorld=nullptr,*driverServerConnection=nullptr,*connectionDriver=nullptr;
    void *pcPlayer=nullptr,*connectionPC=nullptr,*connectionActor=nullptr,*pcState=nullptr,*pcPawn=nullptr,*psOwner=nullptr;
    void *pawnController=nullptr,*pawnState=nullptr,*pawnInventory=nullptr,*inventoryOwner=nullptr;
    void *capturedClass=nullptr,*actualClass=nullptr,*schemaClass=nullptr,*selectedClass=nullptr,*capturedSelectedClass=nullptr,*psSpawnedCharacter=nullptr;
    void *gameState=nullptr,*registeredArray=nullptr;int32_t registeredCount=-1,registeredCapacity=-1;
    uintptr_t lifetime=0;uint8_t flags11a=0,pcRole=0,psRole=0,pawnRole=0,pawnRemoteRole=0;
};
struct State {
    bool initialized=false,closed=false,profilesVerified=false,schemaVerified=false,restartRequired=false;
    unsigned error=0;DWORD gameThread=0;void* world=nullptr;Weak worldId{};
    uint64_t registrations=0,tickets=0,preparations=0,deferrals=0,writes=0,notifications=0;
    uint64_t nativeCompares=0,historyWitnesses=0,retired=0,staleRetired=0,closes=0;
    uintptr_t exceptionRva=0;const char *operation="none",*stage="ps_authority_not_initialized";
    uint64_t contextChecks=0;ContextObservation context{};
};
struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
inline constexpr Profile Profiles[]{
    {0x3ed83b0,1609,0x15d2d4ecc2f49cf7ULL},{0x3efc820,176,0x7f1113b3f79164b2ULL},
    {0x282ff30,167,0x0e6532940d7d2945ULL},{0x2839df0,173,0x99d93fcb48429ad2ULL},
    {0x6cc46a0,122,0x4efc8f40f2d1cb4cULL},{0x6cc4720,99,0x509936e4b283ea64ULL},
    {0x7195030,3344,0x1d8c75856a4cb3c4ULL},
    {0x42ddc40,5897,0x8b1ad1ce6424e77dULL},{0x42ed290,229,0x86a3f9686e6af3b7ULL},
    {0x42ed380,1096,0xcf9b3a9f40d298d3ULL},{0x42e9630,708,0xf90ebeac29d8dfadULL},
    {0x49cbe90,877,0xe60b4a938100e242ULL},{0x49cb180,374,0x18ab310b6b23642dULL},
    {0x49cb300,1772,0xa989d1a6de500590ULL},{0x49cb9f0,1179,0x96467aa66c5031a1ULL},
    {0x49cc2b0,3075,0x52ee77046fadf84bULL},{0x49d5d40,402,0x687104f03fce79ccULL},
    {0x49d5ee0,1295,0xfb77600fa8350afcULL},{0x49d9c70,680,0x86361c58daae3338ULL},
    {0x49d9a70,506,0x98ff3e4f3f57f856ULL},{0x49d98a0,452,0x82fb6a080e47ee14ULL}
};
struct Schema {
    void *type=nullptr,*pushClass=nullptr,*pushSettings=nullptr;
    std::array<void*,2> onRep{};std::array<original_jett_priming::PropertyInfo,2> properties{};
    std::array<uint32_t,2> onRepFlags{};
    std::array<Weak,5> identities{};
};
inline State state{},cachedState{};inline Schema schema{};inline Hooks hooks{};
inline std::array<Record,MaxSources> records{};
inline std::array<Observation,MaxSources> cachedRecords{};
inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline const State& GetState(){return state;}
template<class T>inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Live(void* p,const Weak& id){return p&&original_inventory_slots::ResolveIdentity(id)==p;}
inline bool Capture(void* p,Weak& id){return original_inventory_slots::CaptureIdentity(p,id)&&Live(p,id);}
inline bool Same(const Source& a,const Source& b){return a.world==b.world&&a.controller==b.controller&&a.playerState==b.playerState&&a.pawn==b.pawn&&a.sourceEpoch==b.sourceEpoch;}
inline size_t Index(Field field){return static_cast<size_t>(field);}
inline int32_t Offset(Field field){return field==Field::Points?0xbdc:0xbd8;}
inline int32_t Bytes(Field field){return field==Field::Points?4:1;}
inline bool Thread(){return engine::image&&state.initialized&&!state.closed&&!state.restartRequired&&!state.error
    &&state.gameThread==GetCurrentThreadId()&&original_players::Thread();}
inline void Cache(size_t index=MaxSources){
    AcquireSRWLockExclusive(&cacheLock);cachedState=state;
    if(index<records.size())cachedRecords[index]=records[index].observation;
    ReleaseSRWLockExclusive(&cacheLock);
}
inline Publication Fail(unsigned error,const char* why,bool terminal=false){
    state.error=error;state.stage=why;state.restartRequired=state.restartRequired||terminal;Cache();
    Publication result;result.status=Status::Failed;result.error=error;return result;
}
inline bool FailBool(unsigned error,const char* why){Fail(error,why);return false;}
inline bool ValidArray(const Array& a,size_t stride,int32_t limit){
    return a.count>=0&&a.count<=limit&&a.capacity>=a.count&&a.capacity<=1048576
        &&(!a.count||(a.data&&original_players::Readable(a.data,size_t(a.count)*stride)));
}
inline Record* Find(const Source& source){for(auto& r:records)if(r.used&&Same(r.source,source))return &r;return nullptr;}
inline size_t RecordIndex(const Record& r){return static_cast<size_t>(&r-records.data());}
inline void Update(Record& r){
    r.observation.source=r.source;r.observation.registration=r.registration;
    r.observation.registered=r.used;r.observation.busy=r.busy;
    if(r.used){r.observation.points=Read<int32_t>(r.source.playerState,0xbdc);r.observation.active=Read<uint8_t>(r.source.playerState,0xbd8)!=0;}
    Cache(RecordIndex(r));
}
inline Publication Waiting(Record& r,Field field,const char* why){
    ++state.deferrals;auto& d=r.observation.fields[Index(field)];++d.deferrals;d.stage=why;
    d.last={};state.stage=why;Update(r);return d.last;
}
inline bool VerifyProfiles(){
    if(state.profilesVerified)return true;
    for(const auto& profile:Profiles){const auto code=reinterpret_cast<const uint8_t*>(engine::image+profile.rva);
        if(!original_players::Readable(code,profile.bytes))return FailBool(8501,"ps_authority_original_profile_unreadable");
        uint64_t digest=0xcbf29ce484222325ULL;
        for(size_t n=0;n<profile.bytes;++n)digest=(digest^code[n])*0x100000001b3ULL;
        if(digest!=profile.fnv)return FailBool(8502,"ps_authority_original_profile_mismatch");}
    state.profilesVerified=true;return true;
}
inline bool SchemaLive(){
    const std::array<void*,5> objects{schema.type,schema.pushClass,schema.pushSettings,schema.onRep[0],schema.onRep[1]};
    for(size_t n=0;n<objects.size();++n)if(!Live(objects[n],schema.identities[n]))return false;
    return Read<void*>(schema.onRep[0],0x28)==schema.type&&Read<void*>(schema.onRep[1],0x28)==schema.type
        &&Read<uint16_t>(schema.onRep[0],0xbe)==0&&Read<uint16_t>(schema.onRep[1],0xbe)==0
        &&Read<uintptr_t>(schema.onRep[0],0xe0)==engine::image+0x6cc46a0
        &&Read<uintptr_t>(schema.onRep[1],0xe0)==engine::image+0x6cc4720
        &&(schema.onRepFlags[0]&0x400U)&&(schema.onRepFlags[1]&0x400U)
        &&Read<uint32_t>(schema.onRep[0],0xb8)==schema.onRepFlags[0]&&Read<uint32_t>(schema.onRep[1],0xb8)==schema.onRepFlags[1]
        &&Read<void*>(schema.pushSettings,0x10)==schema.pushClass;
}
inline bool VerifySchema(){
    if(state.schemaVerified)return SchemaLive()||FailBool(8503,"ps_authority_captured_metadata_identity_changed");
    schema.type=engine::Class(original_players::ShooterStateClass);schema.pushClass=engine::Class(0x282ff30);
    schema.pushSettings=engine::DefaultObject(schema.pushClass);
    schema.onRep[0]=engine::Find(L"/Script/ShooterGame.ShooterPlayerState.OnRep_NumUltimatePoints");
    schema.onRep[1]=engine::Find(L"/Script/ShooterGame.ShooterPlayerState.OnRep_UltimateActive");
    const std::array<void*,5> objects{schema.type,schema.pushClass,schema.pushSettings,schema.onRep[0],schema.onRep[1]};
    for(size_t n=0;n<objects.size();++n)if(!Capture(objects[n],schema.identities[n]))return FailBool(8504,"ps_authority_original_metadata_identity_missing");
    for(size_t n=0;n<2;++n)schema.onRepFlags[n]=Read<uint32_t>(schema.onRep[n],0xb8);
    const wchar_t* names[]{L"NumUltimatePoints",L"bUltimateActive"};
    for(size_t n=0;n<2;++n){const auto field=static_cast<Field>(n);auto& p=schema.properties[n];
        if(original_jett_priming::FieldOffset(schema.type,names[n],size_t(Bytes(field)),&p)!=Offset(field)
           ||p.arrayDim!=1||p.elementSize!=Bytes(field)||(Read<uint64_t>(p.field,0x38)&0x20ULL)==0)
            return FailBool(8505,"ps_authority_original_net_property_schema_mismatch");}
    const auto& b=schema.properties[1];
    if(b.boolFieldSize!=1||b.boolByteOffset!=0||b.boolByteMask!=1||b.boolFieldMask!=0xff||!SchemaLive())
        return FailBool(8506,"ps_authority_original_native_bool_or_repnotify_schema_mismatch");
    state.schemaVerified=true;return true;
}
inline bool FactoryBinding(const Record& r,const ContextObservation& d){
    return r.actualClass&&r.selectedClass&&r.actualClass==r.selectedClass
        &&d.selectedClass==r.selectedClass&&d.selectedWeak.matches
        &&d.derivedCalled&&d.derivedFromSchema;
}
inline bool Context(const Record& r){
    // Capture one complete evaluation on the original GT. This diagnosis keeps
    // all source/authority/lifetime predicates. The concrete class must be the
    // exact original factory selection, which can inherit the native PS schema.
    if(!state.gameThread||GetCurrentThreadId()!=state.gameThread)return false;
    ContextObservation d{};d.sequence=++state.contextChecks;d.operation=state.operation;d.source=r.source;d.epoch=r.source.sourceEpoch;
    d.actualThread=GetCurrentThreadId();d.expectedThread=state.gameThread;d.enginePresent=engine::image!=0;
    d.moduleInitialized=state.initialized;d.moduleClosed=state.closed;d.restartRequired=state.restartRequired;d.moduleError=state.error;
    d.moduleWorld=state.world;d.originalWorld=original_players::world;
    d.originalThreadCalled=d.enginePresent&&d.moduleInitialized&&!d.moduleClosed&&!d.restartRequired&&!d.moduleError
        &&d.actualThread==d.expectedThread;
    if(d.originalThreadCalled)d.originalThread=original_players::Thread();
    const bool thread=d.originalThreadCalled&&d.originalThread;
    d.validatorCalled=thread&&r.used&&r.source.world==state.world&&r.source.sourceEpoch&&hooks.validate;
    if(d.validatorCalled)d.validatorResult=hooks.validate(r.source);
    const std::array<void*,8> objects{r.source.world,r.source.controller,r.source.playerState,r.source.pawn,r.connection,r.driver,r.inventory,r.actualClass};
    bool allWeak=true;
    for(size_t n=0;n<objects.size();++n){auto& w=d.weak[n];w.expected=objects[n];w.identity=r.identities[n];
        w.evaluated=d.enginePresent;if(w.evaluated)w.resolved=original_inventory_slots::ResolveIdentity(w.identity);
        w.matches=w.evaluated&&w.expected&&w.resolved==w.expected;allWeak=allWeak&&w.matches;}
    d.expectedDriver=r.driver;d.expectedConnection=r.connection;d.capturedClass=r.actualClass;d.schemaClass=schema.type;
    d.selectedClass=original_players::selectedPlayerStateClass;d.capturedSelectedClass=r.selectedClass;
    d.selectedWeak.expected=r.selectedClass;d.selectedWeak.identity=r.selectedClassId;d.selectedWeak.evaluated=d.enginePresent;
    if(d.selectedWeak.evaluated)d.selectedWeak.resolved=original_inventory_slots::ResolveIdentity(r.selectedClassId);
    d.selectedWeak.matches=d.selectedWeak.evaluated&&r.selectedClass&&d.selectedWeak.resolved==r.selectedClass;
    d.schemaWeak.expected=schema.type;d.schemaWeak.identity=schema.identities[0];d.schemaWeak.evaluated=d.enginePresent;
    if(d.schemaWeak.evaluated)d.schemaWeak.resolved=original_inventory_slots::ResolveIdentity(schema.identities[0]);
    d.schemaWeak.matches=d.schemaWeak.evaluated&&schema.type&&d.schemaWeak.resolved==schema.type;
    d.classEqualsNativeSchema=r.actualClass==schema.type;d.classEqualsSelected=r.actualClass==r.selectedClass;
    d.selectedFactoryUnchanged=d.selectedClass==r.selectedClass;
    // Invalid/expired weak objects are not dereferenced just to obtain a raw
    // diagnostic. The valid case captures the entire actual owner tuple.
    if(d.weak[0].matches){d.worldDriver=Read<void*>(r.source.world,0x40);d.gameState=Read<void*>(r.source.world,0x178);}
    if(d.weak[1].matches){d.pcPlayer=Read<void*>(r.source.controller,0x510);d.pcState=Read<void*>(r.source.controller,0x470);
        d.pcPawn=Read<void*>(r.source.controller,0x4b0);d.pcRole=Read<uint8_t>(r.source.controller,0x224);}
    if(d.weak[2].matches){d.actualClass=Read<void*>(r.source.playerState,0x10);d.psSpawnedCharacter=Read<void*>(r.source.playerState,0xbc8);
        d.psOwner=Read<void*>(r.source.playerState,0x210);d.flags11a=Read<uint8_t>(r.source.playerState,0x11a);
        d.psRole=Read<uint8_t>(r.source.playerState,0x224);d.lifetime=Read<uintptr_t>(Read<void*>(r.source.playerState),0x220);}
    if(d.weak[3].matches){d.pawnController=Read<void*>(r.source.pawn,0x4a0);d.pawnState=Read<void*>(r.source.pawn,0x488);
        d.pawnInventory=Read<void*>(r.source.pawn,0xc08);d.pawnRole=Read<uint8_t>(r.source.pawn,0x224);d.pawnRemoteRole=Read<uint8_t>(r.source.pawn,0x128);}
    if(d.weak[4].matches){d.connectionDriver=Read<void*>(r.connection,0x60);d.connectionPC=Read<void*>(r.connection,0x38);d.connectionActor=Read<void*>(r.connection,0xa0);}
    if(d.weak[5].matches){d.driverWorld=Read<void*>(r.driver,0x178);d.driverServerConnection=Read<void*>(r.driver,0xc0);}
    if(d.weak[6].matches)d.inventoryOwner=Read<void*>(r.inventory,0xc8);
    d.nativeQueriesSafe=thread&&r.used&&allWeak;
    if(d.nativeQueriesSafe){
        d.netModeCalled=true;d.netMode=engine::function<int(*)(void*)>(0x3efc820)(r.source.playerState);
        d.actualHumanCalled=true;d.actualHumanResult=local_server_item_ownership::ActualHuman(r.source.world,r.source.controller,r.source.playerState,r.source.pawn,r.inventory,d.actualHumanConnection);
        d.registeredCalled=true;d.registeredResult=original_players::Registered(r.source.playerState);
        d.registeredArray=Read<void*>(d.gameState,0x480);d.registeredCount=Read<int32_t>(d.gameState,0x488);d.registeredCapacity=Read<int32_t>(d.gameState,0x48c);
        if(d.schemaWeak.matches){d.derivedCalled=true;d.derivedFromSchema=original_players::Derived(r.actualClass,schema.type);}
    }
    const bool gates[]{thread,r.used,r.source.world==state.world,r.source.sourceEpoch!=0,hooks.validate!=nullptr,
        d.validatorCalled&&d.validatorResult,allWeak,d.netModeCalled&&(d.netMode==1||d.netMode==2),original_players::world==r.source.world,
        d.actualHumanCalled&&d.actualHumanResult,d.actualHumanConnection==r.connection,d.worldDriver==r.driver,
        d.actualClass==r.actualClass,d.registeredCalled&&d.registeredResult,FactoryBinding(r,d),
        d.lifetime==engine::image+0x7195030,d.psSpawnedCharacter==r.source.pawn,(d.flags11a&0x10)!=0};
    static_assert(std::size(gates)==18);
    for(size_t n=0;n<std::size(gates);++n)if(gates[n])d.passed|=1U<<n;
    d.result=d.passed==d.required;state.context=d;return d.result;
}
inline bool InitImpl(void* world,const Hooks& value){
    state.operation="init";
    if(state.initialized)return !state.closed&&state.world==world&&hooks.validate==value.validate&&Thread();
    if(!world||!engine::image||!original_players::Thread()||!value.validate)return FailBool(8500,"ps_authority_requires_game_thread_world_and_session_validator");
    state.gameThread=GetCurrentThreadId();state.world=world;hooks=value;
    if(!Capture(world,state.worldId)||!VerifyProfiles()||!VerifySchema())return false;
    state.initialized=true;state.stage="ps_authority_original_schema_ready_no_source_or_delivery_claim";Cache();return true;
}
inline bool RegisterImpl(const Source& source){
    state.operation="register";
    if(!Thread()||source.world!=state.world
       ||!source.sourceEpoch||!hooks.validate||!hooks.validate(source)||!Live(state.world,state.worldId)||!VerifySchema())
        return FailBool(8507,"ps_authority_source_not_admitted_by_current_session");
    if(auto found=Find(source))return Context(*found)||FailBool(8508,"ps_authority_registered_source_tuple_changed");
    for(const auto& r:records)if(r.used&&r.source.playerState==source.playerState)
        return FailBool(8509,"ps_authority_previous_player_state_epoch_requires_retire");
    Record* target=nullptr;for(auto& r:records)if(!r.used){target=&r;break;}
    if(!target||state.registrations==UINT64_MAX)return FailBool(8510,"ps_authority_registration_capacity_or_generation_exhausted");
    auto& r=*target;r={};r.source=source;r.inventory=Read<void*>(source.pawn,0xc08);
    if(!local_server_item_ownership::ActualHuman(source.world,source.controller,source.playerState,source.pawn,r.inventory,r.connection))
        return FailBool(8511,"ps_authority_requires_actual_admitted_native_human");
    r.driver=Read<void*>(r.connection,0x60);r.actualClass=Read<void*>(source.playerState,0x10);
    // Init can precede ResolvePlayerStateClass. Bind its actual selection only
    // when the original human has been created, never substitute a native PS.
    r.selectedClass=original_players::selectedPlayerStateClass;
    const std::array<void*,8> objects{source.world,source.controller,source.playerState,source.pawn,r.connection,r.driver,r.inventory,r.actualClass};
    for(size_t n=0;n<objects.size();++n)if(!Capture(objects[n],r.identities[n]))return FailBool(8512,"ps_authority_source_original_weak_identity_missing");
    if(!Capture(r.selectedClass,r.selectedClassId))return FailBool(8512,"ps_authority_selected_original_factory_class_weak_identity_missing");
    r.registration=++state.registrations;r.used=true;
    if(!Context(r)){r.used=false;return FailBool(8513,"ps_authority_source_world_player_state_pawn_or_epoch_mismatch");}
    r.observation.sourceValid=true;state.stage="ps_authority_source_registered_publication_not_requested";Update(r);return true;
}
inline void ObserveTransaction(Record& r,const Transaction& t){
    auto& d=r.observation.fields[Index(t.field)];d.property=t.property;d.channel=t.channel;d.replicator=t.replicator;
    d.layout=t.layout;d.sendState=t.sendState;d.manager=t.manager;d.repIndex=t.repIndex;d.parentIndex=t.parentIndex;
    d.handle=t.handle;d.commandType=t.commandType;d.condition=t.condition;d.before=t.before;d.after=t.before;
    d.shadowBefore=t.shadowBefore;d.shadowAfter=t.shadowBefore;d.nativeFlags=t.nativeFlags;
    d.layoutFlags=t.layoutFlags;d.replicatorFlags=t.replicatorFlags;d.replicationFrame=t.replicationFrame;
    d.historyBefore=t.historyEndBefore;d.historyAfter=t.historyEndBefore;d.compareBefore=t.compareBefore;d.compareAfter=t.compareBefore;
    d.pushPolicyKnown=t.pushPolicyKnown;d.pushBased=t.pushBased;d.handleObserved=false;d.terminatorObserved=false;
    d.nativeResult=-1;
    d.activeNum=t.activeNum;d.activeMax=t.activeMax;d.originalActiveBypass=t.originalActiveBypass;
}
inline Publication RejectTransaction(Record& r,const Transaction& t,unsigned error,const char* why){
    ObserveTransaction(r,t);auto result=Fail(error,why);auto& d=r.observation.fields[Index(t.field)];
    d.last=result;d.stage=why;Update(r);return result;
}
inline Publication PrepareImpl(const Source& source,Field field,Transaction& result){
    state.operation="prepare";result={};
    if(Index(field)>=2)return Fail(8514,"ps_authority_unsupported_field");
    auto r=Find(source);if(!r||!Context(*r)||!VerifySchema())return Fail(8515,"ps_authority_prepare_requires_live_registered_source");
    auto& d=r->observation.fields[Index(field)];++d.requests;
    if(r->busy)return Waiting(*r,field,"ps_authority_waiting_same_source_original_notification_return");
    r->pending={};
    Transaction t{};t.source=source;t.field=field;t.registration=r->registration;
    t.offset=Offset(field);t.bytes=Bytes(field);t.property=schema.properties[Index(field)].field;
    t.before=field==Field::Points?Read<int32_t>(source.playerState,size_t(t.offset)):Read<uint8_t>(source.playerState,size_t(t.offset));
    if(t.before<0||(field==Field::Active&&t.before>1))return RejectTransaction(*r,t,8516,"ps_authority_original_current_record_outside_native_type_domain");
    // Original7195080..508B queries the static declaring ShooterPS class.
    // The selected Blueprint class inherits it; the actual channel/layout still
    // has to identify these same native declaring FProperties and parents.
    t.pushBased=engine::function<bool(*)(void*,void*)>(0x2839df0)(schema.pushSettings,schema.type);t.pushPolicyKnown=true;
    t.replicationFrame=Read<uint32_t>(r->driver,0x4d8);
    const auto channels=Read<Array>(r->connection,0x78);
    if(!ValidArray(channels,8,512))return RejectTransaction(*r,t,8517,"ps_authority_native_connection_channel_array_invalid");
    unsigned matches=0;
    for(int32_t n=0;n<channels.count;++n){auto channel=Read<void*>(channels.data,size_t(n)*8);
        if(channel&&Read<void*>(channel,0x30)==r->connection&&Read<void*>(channel,0x78)==source.playerState
           &&original_players::IsA(channel,0x3dd3d40)){t.channel=channel;++matches;}}
    if(!matches){ObserveTransaction(*r,t);return Waiting(*r,field,"ps_authority_waiting_original_player_state_actor_channel");}
    t.connectionMode=Read<uint8_t>(r->connection,0x1730);
    if(matches!=1||t.connectionMode!=0||(Read<uint8_t>(t.channel,0x38)&2)||!Capture(t.channel,t.channelId))
        return RejectTransaction(*r,t,8518,"ps_authority_native_player_state_channel_or_connection_invalid");
    if(!(Read<uint8_t>(t.channel,0xa0)&1)){ObserveTransaction(*r,t);return Waiting(*r,field,"ps_authority_waiting_original_player_state_channel_ack");}
    // Primary ACTOR replicator, independently proved at42DDC40/42ED290.
    // The component object map/42E59D0 is deliberately not used for a PS actor.
    t.replicator=Read<void*>(t.channel,0xa8);
    if(!t.replicator){ObserveTransaction(*r,t);return Waiting(*r,field,"ps_authority_waiting_original_player_state_primary_replicator");}
    if(!original_players::Readable(t.replicator,0xe0)||Read<void*>(t.replicator,0x48)!=source.playerState
       ||Read<void*>(t.replicator,0x40)!=r->actualClass||Read<void*>(t.replicator,0x58)!=t.channel
       ||Read<void*>(t.replicator,0x50)!=r->connection||!Live(source.playerState,Read<Weak>(t.replicator,0xd8)))
        return RejectTransaction(*r,t,8519,"ps_authority_primary_replicator_object_class_channel_or_weak_mismatch");
    t.layout=Read<void*>(t.replicator,0x20);t.manager=Read<void*>(t.replicator,0x10);
    t.sendState=Read<void*>(Read<void*>(t.replicator,0x30),8);
    if(!t.layout||!t.manager||!t.sendState){ObserveTransaction(*r,t);return Waiting(*r,field,"ps_authority_waiting_original_actor_sending_state");}
    if(!original_players::Readable(t.layout,0xf8)||!original_players::Readable(t.manager,0x840)
       ||!original_players::Readable(t.sendState,0x28)||Read<void*>(t.manager,0x830)!=t.layout)
        return RejectTransaction(*r,t,8520,"ps_authority_native_actor_layout_or_changelist_manager_invalid");
    t.layoutFlags=Read<uint8_t>(t.layout,0x20);t.replicatorFlags=Read<uint32_t>(t.replicator,0xc);
    // A descriptor query is not an effective dirty ID. Unproven native skip
    // modes stay closed; no push CVar/layout/replicator flags are altered.
    if((t.layoutFlags&0x34U)||(t.replicatorFlags&0x20U))return RejectTransaction(*r,t,8521,"ps_authority_effective_native_layout_skip_mode_unproven");
    t.nativeFlags=Read<uint32_t>(t.sendState,0x1c);
    if(t.nativeFlags&0x80U)return RejectTransaction(*r,t,8522,"ps_authority_roles_only_native_comparison_not_a_property_publication");
    if(t.nativeFlags&2U){ObserveTransaction(*r,t);return Waiting(*r,field,"ps_authority_waiting_original_noninitial_actor_flags");}
    t.repIndex=Read<uint16_t>(t.property,0x40);
    const auto parents=Read<Array>(t.layout,0x28),commands=Read<Array>(t.layout,0x38);
    if(!ValidArray(parents,0x38,4096)||!ValidArray(commands,0x20,65536)||t.repIndex==0xffff||t.repIndex>=parents.count)
        return RejectTransaction(*r,t,8523,"ps_authority_actual_property_repindex_or_native_layout_arrays_invalid");
    t.parents=parents.data;t.commands=commands.data;t.parentCount=parents.count;t.commandCount=commands.count;
    t.parentIndex=t.repIndex;t.parent=parents.data+size_t(t.parentIndex)*0x38;
    unsigned matchingParents=0;
    for(int32_t n=0;n<parents.count;++n)if(Read<void*>(parents.data,size_t(n)*0x38)==t.property)++matchingParents;
    t.commandStart=Read<uint16_t>(t.parent,0x20);t.commandEnd=Read<uint16_t>(t.parent,0x22);t.condition=Read<int32_t>(t.parent,0x24);
    if(matchingParents!=1||Read<void*>(t.parent)!=t.property||Read<int32_t>(t.parent,0x14)!=0||Read<int32_t>(t.parent,0x18)!=t.offset
       ||!(Read<uint32_t>(t.parent,0x30)&1U)||uint32_t(t.commandEnd)!=uint32_t(t.commandStart)+1U||t.commandEnd>commands.count)
        return RejectTransaction(*r,t,8524,"ps_authority_target_is_not_its_actual_single_lifetime_parent");
    // Original7195030 declares both exact PS targets with ConditionNone0.
    // OwnerOnly2 is not an interchangeable publication policy for these fields.
    if(t.condition!=0)return RejectTransaction(*r,t,8525,"ps_authority_target_lifetime_condition_not_source_supported");
    t.command=commands.data+size_t(t.commandStart)*0x20;t.handle=Read<uint16_t>(t.command,0x14);
    t.commandType=Read<uint8_t>(t.command,0x1c);const auto shadowOffset=Read<int32_t>(t.command,0x10);
    if(Read<void*>(t.command)!=t.property||Read<int32_t>(t.command,0xc)!=t.offset
       ||Read<uint16_t>(t.command,0x16)!=t.parentIndex||!t.handle||Read<int32_t>(t.parent,0x1c)!=shadowOffset
       ||t.commandType!=(field==Field::Points?5:0x15))return RejectTransaction(*r,t,8526,"ps_authority_actual_integer_or_native_bool_command_schema_mismatch");
    auto shadowData=Read<uint8_t*>(t.manager,0x820);const auto shadowNum=Read<int32_t>(t.manager,0x828),shadowCapacity=Read<int32_t>(t.manager,0x82c);
    if(!shadowData||shadowOffset<0||shadowNum<t.bytes||shadowNum>1048576||shadowCapacity<shadowNum
       ||uint64_t(shadowOffset)+uint32_t(t.bytes)>uint64_t(shadowNum)||!original_players::Readable(shadowData+shadowOffset,size_t(t.bytes)))
        return RejectTransaction(*r,t,8527,"ps_authority_actual_native_shadow_allocation_invalid");
    t.shadowData=shadowData;t.shadowOffset=shadowOffset;t.shadowNum=shadowNum;t.shadowCapacity=shadowCapacity;
    t.shadow=shadowData+shadowOffset;t.shadowBefore=field==Field::Points?Read<int32_t>(t.shadow):Read<uint8_t>(t.shadow);
    if(t.shadowBefore!=t.before){ObserveTransaction(*r,t);return Waiting(*r,field,"ps_authority_waiting_original_current_value_shadow_baseline");}
    t.originalActiveBypass=(t.nativeFlags&0x800U)!=0;t.activeTracker=Read<void*>(t.sendState,0x20);
    if(t.activeTracker&&!t.originalActiveBypass){const auto active=t.activeTracker;
        auto bits=Read<const uint32_t*>(active,0x18);const bool heap=bits!=nullptr;
        if(!heap)bits=reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(active)+8);
        t.activeBits=const_cast<uint32_t*>(bits);t.activeNum=Read<int32_t>(active,0x20);t.activeMax=Read<int32_t>(active,0x24);
        //2839530/28384B0/2839A30 prove this logical bit-array allocation.
        // PS points cross into word1; ResourceComponent's word0/mask4 is wrong.
        if(t.activeNum!=parents.count||t.activeNum<=0||t.activeMax<t.activeNum||t.parentIndex>=t.activeNum
           ||(heap?t.activeMax!=((t.activeNum+31)&~31):t.activeMax!=128))
            return RejectTransaction(*r,t,8528,"ps_authority_original_active_parent_mask_logical_bounds_invalid");
        const size_t word=size_t(t.parentIndex)>>5;uint32_t value=0;
        if(!original_players::Read(bits,word*4,value))return RejectTransaction(*r,t,8528,"ps_authority_original_active_parent_mask_unreadable");
        if(!(value&(1U<<(t.parentIndex&31U)))){ObserveTransaction(*r,t);return Waiting(*r,field,"ps_authority_waiting_original_target_parent_active");}
    }
    t.historyStart=Read<uint32_t>(t.manager,0x810);t.historyEndBefore=Read<uint32_t>(t.manager,0x814);t.compareBefore=Read<uint32_t>(t.manager,0x818);
    t.sendHistoryBefore=Read<uint32_t>(t.sendState,0x10);t.sendCompareBefore=Read<uint32_t>(t.sendState,0x14);
    if(t.historyStart>t.historyEndBefore||t.historyEndBefore>=0x7fffffffU||t.compareBefore>=0x7fffffffU
       ||t.historyEndBefore-t.historyStart>=64U||t.sendHistoryBefore<t.historyStart||t.sendHistoryBefore>t.historyEndBefore||t.sendCompareBefore>t.compareBefore)
        return RejectTransaction(*r,t,8529,"ps_authority_original_history_capacity_or_consumer_invalid");
    if(t.historyEndBefore-t.historyStart==63U&&t.sendHistoryBefore==t.historyStart){ObserveTransaction(*r,t);return Waiting(*r,field,"ps_authority_waiting_original_history_consumer");}
    if(state.tickets==UINT64_MAX)return RejectTransaction(*r,t,8530,"ps_authority_transaction_generation_exhausted");
    t.ticket=++state.tickets;t.prepared=true;r->pending=t;result=t;++state.preparations;
    ObserveTransaction(*r,t);d.stage="ps_authority_actual_native_compare_transaction_prepared";d.last={};
    d.last.status=Status::Ready;d.last.nativeShadowMatches=true;state.stage=d.stage;Update(*r);return d.last;
}
inline bool SameNativeTransaction(const Record& r,const Transaction& t){
    const auto parents=Read<Array>(t.layout,0x28),commands=Read<Array>(t.layout,0x38);
    const auto& p=schema.properties[Index(t.field)];
    bool active=true;
    if(t.activeTracker&&!t.originalActiveBypass){
        auto bits=Read<void*>(t.activeTracker,0x18);if(!bits)bits=static_cast<uint8_t*>(t.activeTracker)+8;
        active=bits==t.activeBits&&Read<int32_t>(t.activeTracker,0x20)==t.activeNum&&Read<int32_t>(t.activeTracker,0x24)==t.activeMax
            &&(Read<uint32_t>(bits,size_t(t.parentIndex>>5)*4)&(1U<<(t.parentIndex&31U)))!=0;
    }
    return Context(r)&&SchemaLive()&&Live(t.channel,t.channelId)&&Read<void*>(t.channel,0x78)==r.source.playerState
        &&Read<void*>(t.channel,0x30)==r.connection&&Read<void*>(t.channel,0xa8)==t.replicator
        &&Read<void*>(t.replicator,0x48)==r.source.playerState&&Read<void*>(t.replicator,0x58)==t.channel
        &&Read<void*>(t.replicator,0x50)==r.connection&&Read<void*>(t.replicator,0x40)==r.actualClass
        &&Read<void*>(t.replicator,0x20)==t.layout&&Read<void*>(t.replicator,0x10)==t.manager
        &&Read<void*>(Read<void*>(t.replicator,0x30),8)==t.sendState&&Read<void*>(t.manager,0x830)==t.layout
        &&Read<uint8_t>(r.connection,0x1730)==t.connectionMode&&Read<uint32_t>(t.sendState,0x1c)==t.nativeFlags
        &&Read<uint8_t>(t.layout,0x20)==t.layoutFlags&&Read<uint32_t>(t.replicator,0xc)==t.replicatorFlags
        &&Read<void*>(t.sendState,0x20)==t.activeTracker&&active
        &&parents.data==t.parents&&parents.count==t.parentCount&&commands.data==t.commands&&commands.count==t.commandCount
        &&Read<void*>(t.parent)==t.property&&Read<void*>(t.command)==t.property&&t.property==p.field
        &&Read<int32_t>(t.property,0x44)==t.offset&&Read<int32_t>(t.property,0x30)==1&&Read<int32_t>(t.property,0x34)==t.bytes
        &&(Read<uint64_t>(t.property,0x38)&0x20ULL)!=0&&Read<int32_t>(t.parent,0x18)==t.offset&&Read<int32_t>(t.parent,0x1c)==t.shadowOffset
        &&Read<uint16_t>(t.parent,0x20)==t.commandStart&&Read<uint16_t>(t.parent,0x22)==t.commandEnd&&Read<int32_t>(t.parent,0x24)==t.condition
        &&Read<int32_t>(t.command,0xc)==t.offset&&Read<int32_t>(t.command,0x10)==t.shadowOffset
        &&Read<uint16_t>(t.command,0x14)==t.handle&&Read<uint16_t>(t.command,0x16)==t.parentIndex&&Read<uint8_t>(t.command,0x1c)==t.commandType
        &&Read<void*>(t.manager,0x820)==t.shadowData&&Read<int32_t>(t.manager,0x828)==t.shadowNum&&Read<int32_t>(t.manager,0x82c)==t.shadowCapacity
        &&engine::function<bool(*)(void*,void*)>(0x2839df0)(schema.pushSettings,schema.type)==t.pushBased
        &&(t.field!=Field::Active||(Read<uint8_t>(t.property,0x78)==1&&Read<uint8_t>(t.property,0x79)==0&&Read<uint8_t>(t.property,0x7a)==1&&Read<uint8_t>(t.property,0x7b)==0xff))
        &&Read<uint32_t>(t.sendState,0x10)==t.sendHistoryBefore&&Read<uint32_t>(t.sendState,0x14)==t.sendCompareBefore
        &&Read<uint32_t>(t.manager,0x810)==t.historyStart&&Read<uint32_t>(t.manager,0x814)==t.historyEndBefore
        &&Read<uint32_t>(t.manager,0x818)==t.compareBefore&&Read<uint16_t>(t.property,0x40)==t.repIndex
        &&(t.field==Field::Points?Read<int32_t>(t.shadow):Read<uint8_t>(t.shadow))==t.shadowBefore;
}
inline Publication CommitFailure(Record& r,Field field,Publication partial,unsigned error,const char* why){
    Fail(error,why,true);partial.status=Status::Failed;partial.error=error;
    auto& d=r.observation.fields[Index(field)];d.last=partial;d.stage=why;r.inFlight=partial;r.observation.sourceValid=false;return partial;
}
inline Publication CommitBody(Record& r,Transaction& transaction,int32_t desired){
    // Use this module's captured transaction, never a caller-supplied layout
    // pointer/offset/flags bundle. The caller only returns its generation token.
    auto t=r.pending;auto& d=r.observation.fields[Index(t.field)];Publication result;
    if(!t.prepared||t.ticket!=transaction.ticket||t.registration!=r.registration||!Same(t.source,transaction.source)
       ||t.field!=transaction.field||!SameNativeTransaction(r,t))return Fail(8531,"ps_authority_native_transaction_changed_before_commit");
    const auto actual=t.field==Field::Points?Read<int32_t>(r.source.playerState,size_t(t.offset)):Read<uint8_t>(r.source.playerState,size_t(t.offset));
    if(actual!=t.before)return Fail(8532,"ps_authority_original_value_changed_after_prepare");
    if(desired<0||(t.field==Field::Active&&desired>1))return Fail(8533,"ps_authority_requested_value_outside_native_type_domain");
    r.pending.prepared=false;transaction.committed=true;
    if(desired==t.before){result.status=Status::Ready;result.nativeShadowMatches=true;++d.unchanged;
        d.stage="ps_authority_native_shadow_already_matches_no_write_or_notification";d.last=result;state.stage=d.stage;return result;}
    const bool written=t.field==Field::Points?original_players::Write(r.source.playerState,size_t(t.offset),desired)
        :original_players::Write(r.source.playerState,size_t(t.offset),static_cast<uint8_t>(desired));
    if(!written)return CommitFailure(r,t.field,result,8534,"ps_authority_host_record_write_failed");
    result.recordWritten=true;++state.writes;++d.writes;r.inFlight=result;d.last=result;
    const auto after=t.field==Field::Points?Read<int32_t>(r.source.playerState,size_t(t.offset)):Read<uint8_t>(r.source.playerState,size_t(t.offset));
    if(after!=desired)return CommitFailure(r,t.field,result,8535,"ps_authority_host_record_readback_failed");
    auto process=Read<void(*)(void*,void*,void*)>(Read<void*>(r.source.playerState),0x298);
    if(!process||!SchemaLive())return CommitFailure(r,t.field,result,8536,"ps_authority_original_repnotify_dispatch_identity_changed");
    process(r.source.playerState,schema.onRep[Index(t.field)],nullptr);
    result.notified=true;++state.notifications;++d.notifications;r.inFlight=result;d.last=result;
    if(!SameNativeTransaction(r,t)||(t.field==Field::Points?Read<int32_t>(r.source.playerState,size_t(t.offset)):Read<uint8_t>(r.source.playerState,size_t(t.offset)))!=desired)
        return CommitFailure(r,t.field,result,8537,"ps_authority_original_notification_changed_compare_transaction");
    ++state.nativeCompares;++d.compares;
    // Exact original42ED380 ABI, including the complete actual native flags.
    // false is the original sixth argument. No force/owner/initial flag is made.
    d.nativeResult=engine::function<int32_t(*)(void*,void*,void*,const void*,const uint32_t*,bool)>(0x49cbe90)
        (t.layout,t.sendState,static_cast<uint8_t*>(t.manager)+8,r.source.playerState,&t.nativeFlags,false);
    d.after=desired;d.shadowAfter=t.field==Field::Points?Read<int32_t>(t.shadow):Read<uint8_t>(t.shadow);
    d.historyAfter=Read<uint32_t>(t.manager,0x814);d.compareAfter=Read<uint32_t>(t.manager,0x818);
    const auto changed=Read<Array>(static_cast<uint8_t*>(t.manager)+0x10+size_t(t.historyEndBefore&63U)*0x20);
    if(ValidArray(changed,2,65536)&&changed.count>=2){
        d.terminatorObserved=Read<uint16_t>(changed.data,size_t(changed.count-1)*2)==0;
        for(int32_t n=0;n<changed.count-1;++n)if(Read<uint16_t>(changed.data,size_t(n)*2)==t.handle){d.handleObserved=true;break;}}
    if(!Context(r)||!SchemaLive()||!Live(t.channel,t.channelId)||Read<void*>(t.channel,0xa8)!=t.replicator
       ||Read<void*>(t.replicator,0x48)!=r.source.playerState||Read<void*>(t.replicator,0x58)!=t.channel
       ||Read<void*>(t.replicator,0x50)!=r.connection||Read<void*>(t.replicator,0x20)!=t.layout
       ||Read<void*>(t.replicator,0x10)!=t.manager||Read<void*>(t.manager,0x820)!=t.shadowData
       ||(t.field==Field::Points?Read<int32_t>(r.source.playerState,size_t(t.offset)):Read<uint8_t>(r.source.playerState,size_t(t.offset)))!=desired
       ||d.nativeResult!=0||d.shadowAfter!=desired||d.compareAfter!=t.compareBefore+1U
       ||d.historyAfter!=t.historyEndBefore+1U||!d.handleObserved||!d.terminatorObserved)
        return CommitFailure(r,t.field,result,8538,"ps_authority_actual_target_shadow_and_history_witness_failed");
    result.status=Status::Ready;result.nativePublished=true;result.nativeShadowMatches=true;
    ++state.historyWitnesses;++d.historyWitnesses;d.last=result;r.inFlight=result;
    d.stage="ps_authority_original_native_history_witnessed_delivery_unverified";state.stage=d.stage;return result;
}
inline Publication CommitImpl(Transaction& transaction,int32_t desired){
    state.operation="commit";auto r=Find(transaction.source);
    if(!r||!Thread()||r->busy||!transaction.prepared||transaction.committed)
        return Fail(8539,"ps_authority_commit_requires_single_current_prepared_transaction");
    Publication result;r->busy=true;r->inFlight={};r->inFlightField=transaction.field;
    __try{result=CommitBody(*r,transaction,desired);}
    __finally{r->busy=false;Update(*r);}
    return result;
}
inline Publication SetImpl(const Source& source,Field field,int32_t desired){
    if(auto r=Find(source))if(!r->busy)r->inFlight={};
    if(!RegisterImpl(source)){Publication failed;failed.status=Status::Failed;failed.error=state.error;return failed;}
    Transaction transaction;auto result=PrepareImpl(source,field,transaction);
    return result.Ready()?CommitImpl(transaction,desired):result;
}
inline bool ReadbackImpl(const Source& source,Observation& output){
    state.operation="readback";output={};auto r=Find(source);if(!r||!Context(*r))return false;
    r->observation.sourceValid=true;Update(*r);output=r->observation;return true;
}
inline bool RetireImpl(const Source& source){
    state.operation="retire";
    // The coordinator has already fenced new admission. Retiring our own
    // cached tuple must NOT require hooks.validate(source) to become true again.
    if(!state.initialized||state.closed||state.gameThread!=GetCurrentThreadId())return false;
    auto r=Find(source);if(!r)return true;if(r->busy)return FailBool(8540,"ps_authority_retire_waits_original_notification_and_compare_return");
    bool live=true;const std::array<void*,8> objects{r->source.world,r->source.controller,r->source.playerState,r->source.pawn,r->connection,r->driver,r->inventory,r->actualClass};
    for(size_t n=0;n<objects.size();++n)if(!Live(objects[n],r->identities[n]))live=false;
    if(!live)++state.staleRetired;
    r->used=false;r->pending={};r->observation.registered=false;r->observation.sourceValid=false;r->observation.retired=true;
    ++state.retired;state.stage="ps_authority_own_cached_epoch_retired_no_native_write";Cache(RecordIndex(*r));return true;
}
inline bool CloseImpl(){
    state.operation="close";
    if(!state.initialized||state.closed)return true;
    if(state.gameThread!=GetCurrentThreadId())return false;
    for(const auto& r:records)if(r.used||r.busy)return FailBool(8541,"ps_authority_close_requires_all_cached_source_epochs_retired");
    state.closed=true;++state.closes;state.stage="ps_authority_own_caches_closed_no_native_state_changed";Cache();return true;
}
inline LONG Exception(EXCEPTION_POINTERS* e){
    state.error=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0008500U;
    const auto pc=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=pc>=engine::image?pc-engine::image:pc;state.restartRequired=true;
    state.stage="ps_authority_original_native_exception_requires_new_session";Cache();return EXCEPTION_EXECUTE_HANDLER;
}
inline Publication FaultPublication(const Source& source,Field field){
    Publication result;
    if(auto r=Find(source))if(r->inFlightField==field){result=r->inFlight;
        result.status=Status::Failed;result.error=state.error;auto& d=r->observation.fields[Index(field)];
        d.last=result;d.stage=state.stage;r->observation.sourceValid=false;Cache(RecordIndex(*r));}
    result.status=Status::Failed;result.error=state.error;return result;
}
inline bool Init(void* world,const Hooks& value){__try{return InitImpl(world,value);}__except(Exception(GetExceptionInformation())){return false;}}
inline bool Register(const Source& source){__try{return RegisterImpl(source);}__except(Exception(GetExceptionInformation())){return false;}}
inline Publication Prepare(const Source& source,Field field,Transaction& output){__try{return PrepareImpl(source,field,output);}__except(Exception(GetExceptionInformation())){Publication r;r.status=Status::Failed;r.error=state.error;return r;}}
inline Publication Commit(Transaction& transaction,int32_t desired){__try{return CommitImpl(transaction,desired);}__except(Exception(GetExceptionInformation())){return Index(transaction.field)<2?FaultPublication(transaction.source,transaction.field):Publication{Status::Failed,state.error};}}
inline Publication SetPoints(const Source& source,int32_t points){__try{return SetImpl(source,Field::Points,points);}__except(Exception(GetExceptionInformation())){return FaultPublication(source,Field::Points);}}
inline Publication SetActive(const Source& source,bool active){__try{return SetImpl(source,Field::Active,active?1:0);}__except(Exception(GetExceptionInformation())){return FaultPublication(source,Field::Active);}}
inline bool Readback(const Source& source,Observation& output){__try{return ReadbackImpl(source,output);}__except(Exception(GetExceptionInformation())){return false;}}
inline bool Retire(const Source& source){__try{return RetireImpl(source);}__except(Exception(GetExceptionInformation())){return false;}}
inline bool Close(){__try{return CloseImpl();}__except(Exception(GetExceptionInformation())){return false;}}
inline const char* StatusName(Status value){return value==Status::Ready?"ready":value==Status::Waiting?"waiting":"failed";}
inline void WritePublication(FILE* f,const Publication& p){
    auto b=[](bool value){return value?"true":"false";};
    std::fprintf(f,"{\"status\":\"%s\",\"error\":%u,\"record_written\":%s,\"notified\":%s,\"native_published\":%s,\"native_shadow_matches\":%s,\"delivery_verified\":false}",
        StatusName(p.status),p.error,b(p.recordWritten),b(p.notified),b(p.nativePublished),b(p.nativeShadowMatches));
}
inline void WriteContextWeak(FILE* f,const ContextWeakObservation& w){
    std::fprintf(f,"{\"expected_object\":\"%p\",\"index\":%d,\"serial\":%d,\"resolved_object\":\"%p\",\"evaluated\":%s,\"matches\":%s}",
        w.expected,w.identity.index,w.identity.serial,w.resolved,w.evaluated?"true":"false",w.matches?"true":"false");
}
inline void WriteContext(FILE* f,const ContextObservation& d){
    auto b=[](bool v){return v?"true":"false";};
    constexpr const char* gates[]{"thread","record_used","source_world_matches","source_epoch_nonzero","source_validator_present","source_validator_passed",
        "all_captured_weaks_match","original_actor_net_mode_server","original_players_world_matches","actual_human",
        "actual_human_connection_matches","world_driver_matches","ps_class_matches_captured","original_ps_registered",
        "exact_selected_factory_class_and_native_schema","original_native_lifetime_matches","ps_spawned_character_matches","original_breplicates"};
    std::fprintf(f,"{\"sequence\":%llu,\"operation\":\"%s\",\"epoch\":%u,\"result\":%s,\"passed_mask\":%u,\"required_mask\":%u,\"actual_thread\":%u,\"expected_thread\":%u,\"engine_present\":%s,\"module_initialized\":%s,\"module_closed\":%s,\"restart_required\":%s,\"module_error_at_evaluation\":%u,\"original_thread_called\":%s,\"original_thread\":%s,\"native_queries_safe\":%s,\"validator_called\":%s,\"validator_result\":%s,\"net_mode_called\":%s,\"net_mode\":%d,\"actual_human_called\":%s,\"actual_human_result\":%s,\"registered_called\":%s,\"registered_result\":%s,\"derived_called\":%s,\"derived_from_native_schema\":%s,\"gates\":[",
        static_cast<unsigned long long>(d.sequence),d.operation,d.epoch,b(d.result),d.passed,d.required,d.actualThread,d.expectedThread,b(d.enginePresent),b(d.moduleInitialized),b(d.moduleClosed),b(d.restartRequired),d.moduleError,b(d.originalThreadCalled),b(d.originalThread),b(d.nativeQueriesSafe),b(d.validatorCalled),b(d.validatorResult),b(d.netModeCalled),d.netMode,b(d.actualHumanCalled),b(d.actualHumanResult),b(d.registeredCalled),b(d.registeredResult),b(d.derivedCalled),b(d.derivedFromSchema));
    for(size_t n=0;n<std::size(gates);++n)std::fprintf(f,"%s{\"name\":\"%s\",\"passed\":%s}",n?",":"",gates[n],b((d.passed&(1U<<n))!=0));
    std::fputs("],\"weak_identities\":[",f);
    constexpr const char* roles[]{"world","controller","player_state","pawn","connection","driver","inventory","actual_class"};
    for(size_t n=0;n<d.weak.size();++n){const auto& w=d.weak[n];
        std::fprintf(f,"%s{\"role\":\"%s\",\"expected_object\":\"%p\",\"index\":%d,\"serial\":%d,\"resolved_object\":\"%p\",\"evaluated\":%s,\"matches\":%s}",n?",":"",roles[n],w.expected,w.identity.index,w.identity.serial,w.resolved,b(w.evaluated),b(w.matches));}
    std::fprintf(f,"],\"selected_factory_binding\":{\"captured_class\":\"%p\",\"actual_class_equals_native_schema\":%s,\"actual_class_equals_selected\":%s,\"current_factory_matches_captured\":%s,\"selected_weak\":",
        d.capturedSelectedClass,b(d.classEqualsNativeSchema),b(d.classEqualsSelected),b(d.selectedFactoryUnchanged));
    WriteContextWeak(f,d.selectedWeak);std::fputs(",\"native_schema_weak\":",f);WriteContextWeak(f,d.schemaWeak);std::fputs("}",f);
    std::fprintf(f,",\"source\":{\"world\":\"%p\",\"controller\":\"%p\",\"player_state\":\"%p\",\"pawn\":\"%p\"},\"raw\":{\"module_world\":\"%p\",\"original_players_world\":\"%p\",\"expected_driver\":\"%p\",\"expected_connection\":\"%p\",\"actual_human_connection\":\"%p\",\"world_driver\":\"%p\",\"driver_world\":\"%p\",\"driver_server_connection\":\"%p\",\"connection_driver\":\"%p\",\"pc_player\":\"%p\",\"connection_pc\":\"%p\",\"connection_actor\":\"%p\",\"pc_state\":\"%p\",\"pc_pawn\":\"%p\",\"ps_owner\":\"%p\",\"pawn_controller\":\"%p\",\"pawn_state\":\"%p\",\"pawn_inventory\":\"%p\",\"inventory_owner\":\"%p\",\"captured_class\":\"%p\",\"actual_class\":\"%p\",\"schema_class\":\"%p\",\"selected_factory_class\":\"%p\",\"ps_spawned_character\":\"%p\",\"game_state\":\"%p\",\"registered_array\":\"%p\",\"registered_count\":%d,\"registered_capacity\":%d,\"native_lifetime\":\"0x%llx\",\"expected_lifetime_rva\":\"0x7195030\",\"flags_11a\":%u,\"pc_role\":%u,\"ps_role\":%u,\"pawn_role\":%u,\"pawn_remote_role\":%u}}",
        d.source.world,d.source.controller,d.source.playerState,d.source.pawn,d.moduleWorld,d.originalWorld,d.expectedDriver,d.expectedConnection,d.actualHumanConnection,d.worldDriver,d.driverWorld,d.driverServerConnection,d.connectionDriver,d.pcPlayer,d.connectionPC,d.connectionActor,d.pcState,d.pcPawn,d.psOwner,d.pawnController,d.pawnState,d.pawnInventory,d.inventoryOwner,d.capturedClass,d.actualClass,d.schemaClass,d.selectedClass,d.psSpawnedCharacter,d.gameState,d.registeredArray,d.registeredCount,d.registeredCapacity,static_cast<unsigned long long>(d.lifetime),unsigned(d.flags11a),unsigned(d.pcRole),unsigned(d.psRole),unsigned(d.pawnRole),unsigned(d.pawnRemoteRole));
}
inline void Write(FILE* f){
    if(!f)return;State s;std::array<Observation,MaxSources> snapshot;
    AcquireSRWLockShared(&cacheLock);s=cachedState;snapshot=cachedRecords;ReleaseSRWLockShared(&cacheLock);
    auto b=[](bool value){return value?"true":"false";};
    std::fprintf(f,"{\"stage\":\"%s\",\"operation\":\"%s\",\"error\":%u,\"initialized\":%s,\"closed\":%s,\"profiles_verified\":%s,\"schema_verified\":%s,\"restart_required\":%s,\"exception_rva\":\"0x%llx\",\"registrations\":%llu,\"preparations\":%llu,\"deferrals\":%llu,\"writes\":%llu,\"notifications\":%llu,\"native_compares\":%llu,\"history_witnesses\":%llu,\"retired\":%llu,\"stale_retired\":%llu,\"context\":",
        s.stage,s.operation,s.error,b(s.initialized),b(s.closed),b(s.profilesVerified),b(s.schemaVerified),b(s.restartRequired),static_cast<unsigned long long>(s.exceptionRva),static_cast<unsigned long long>(s.registrations),static_cast<unsigned long long>(s.preparations),static_cast<unsigned long long>(s.deferrals),static_cast<unsigned long long>(s.writes),static_cast<unsigned long long>(s.notifications),static_cast<unsigned long long>(s.nativeCompares),static_cast<unsigned long long>(s.historyWitnesses),static_cast<unsigned long long>(s.retired),static_cast<unsigned long long>(s.staleRetired));
    WriteContext(f,s.context);std::fputs(",\"records\":[",f);
    bool first=true;for(const auto& r:snapshot){if(!r.registration)continue;
        std::fprintf(f,"%s{\"registration\":%llu,\"epoch\":%u,\"player_state\":\"%p\",\"pawn\":\"%p\",\"registered\":%s,\"source_valid\":%s,\"retired\":%s,\"busy\":%s,\"points\":%d,\"active\":%s,\"fields\":[",
            first?"":",",static_cast<unsigned long long>(r.registration),r.source.sourceEpoch,r.source.playerState,r.source.pawn,b(r.registered),b(r.sourceValid),b(r.retired),b(r.busy),r.points,b(r.active));first=false;
        for(size_t n=0;n<r.fields.size();++n){const auto& d=r.fields[n];
            std::fprintf(f,"%s{\"field\":\"%s\",\"stage\":\"%s\",\"publication\":",n?",":"",n==0?"NumUltimatePoints":"bUltimateActive",d.stage);WritePublication(f,d.last);
            std::fprintf(f,",\"property\":\"%p\",\"channel\":\"%p\",\"replicator\":\"%p\",\"layout\":\"%p\",\"send_state\":\"%p\",\"manager\":\"%p\",\"rep_index\":%u,\"parent_index\":%u,\"handle\":%u,\"command_type\":%u,\"condition\":%d,\"before\":%d,\"after\":%d,\"shadow_before\":%d,\"shadow_after\":%d,\"native_result\":%d,\"native_flags\":%u,\"layout_flags\":%u,\"replicator_flags\":%u,\"history_before\":%u,\"history_after\":%u,\"compare_before\":%u,\"compare_after\":%u,\"replication_frame\":%u,\"push_policy_known\":%s,\"push_based\":%s,\"handle_observed\":%s,\"terminator_observed\":%s,\"requests\":%llu,\"deferrals\":%llu,\"writes\":%llu,\"notifications\":%llu,\"compares\":%llu,\"history_witnesses\":%llu,\"unchanged\":%llu",
                d.property,d.channel,d.replicator,d.layout,d.sendState,d.manager,d.repIndex,d.parentIndex,d.handle,d.commandType,d.condition,d.before,d.after,d.shadowBefore,d.shadowAfter,d.nativeResult,d.nativeFlags,d.layoutFlags,d.replicatorFlags,d.historyBefore,d.historyAfter,d.compareBefore,d.compareAfter,d.replicationFrame,b(d.pushPolicyKnown),b(d.pushBased),b(d.handleObserved),b(d.terminatorObserved),static_cast<unsigned long long>(d.requests),static_cast<unsigned long long>(d.deferrals),static_cast<unsigned long long>(d.writes),static_cast<unsigned long long>(d.notifications),static_cast<unsigned long long>(d.compares),static_cast<unsigned long long>(d.historyWitnesses),static_cast<unsigned long long>(d.unchanged));
            std::fprintf(f,",\"active_num_bits\":%d,\"active_max_bits\":%d,\"original_active_bypass_flag\":%s}",d.activeNum,d.activeMax,b(d.originalActiveBypass));}
        std::fputs("]}",f);}
    std::fputs("],\"push_policy_scope\":\"original_static_declaring_shooter_ps_class\",\"empty_dirty_helpers_called\":false,\"policy_written\":false,\"net_or_role_flags_written\":false,\"original_image_written\":false,\"delivery_verified\":false,\"native_runtime_verified\":false}",f);
}
}
