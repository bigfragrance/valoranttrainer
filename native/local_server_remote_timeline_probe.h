#pragma once
#include "local_server_remote_render_probe.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <intrin.h>

// Candidate diagnostic only. Source: npc-timeline-case61/verified-contract.json
// and probe-profiles.json. Poll never advances clocks, consumes queues, repeats
// a receiver, allocates weak serials, or requests/creates a NetGUID. Write reads
// cached POD only. Missing layouts are observations, never gameplay failures.
namespace local_server_remote_timeline_probe {
inline constexpr size_t MaxRemote=30;
inline constexpr uint64_t MinimumIntervalMs=100;
struct Weak {int32_t index=-1,serial=0;};
struct Name {uint32_t words[3]{};};
struct Array {void** data=nullptr;int32_t count=0,capacity=0;};
static_assert(sizeof(Weak)==8&&sizeof(Name)==12&&sizeof(Array)==16);
struct Label {
    void* object=nullptr;void* type=nullptr;int32_t internalIndex=-1;
    Name objectName{},className{};
    bool rawKnown=false,nameKnown=false,classNameKnown=false;
    char name[128]{},classText[128]{};
};
struct Manager {
    void *object=nullptr,*owner=nullptr;
    bool typed=false,schema=false,fieldsKnown=false;
    uint32_t currentIndex=0,previousIndex=0;
    float currentFraction=0,previousFraction=0,atWorldTime=0;
    int32_t numRebases=0,numRebasesFromOverqueue=0;
    uint8_t raw100=0,raw101=0,raw118=0,raw119=0;
};
struct Move {
    bool known=false;uint32_t timeIndex=0,sourceIndex470=0;
    uint8_t firstAfterReset469=0;void* object=nullptr;
};
struct Queue {
    bool shapeKnown=false,entriesKnown=false;
    void* data=nullptr;int32_t nextWrite=0,capacity=0,count=0;
    Move oldest{},newest{};
};
struct Clock {
    void *object=nullptr,*table=nullptr,*vbtable=nullptr;
    bool schema=false,fieldsKnown=false,inverseKnown=false;
    int32_t baseOffset=0,indexOffset=0;
    uint32_t rawIndex=0,step=0,serverDomainIndex=0;
    float accumulated=0;
    uint8_t raw28=0,initialized4d8=0;
    int32_t historyNext=0,historyCapacity=0,historyCount=0,pendingMoves=0,frameOrdinal=0,lead9a0=0;
    uint32_t lastQueuedMappedIndex=0,lastQueuedServerDomainIndex=0;
    bool lastQueuedInverseKnown=false;
    Move reference{};
};
struct Delay {
    bool fieldsKnown=false,worldSourceKnown=false,chainKnown=false,zeroBranchKnown=false,valueKnown=false;
    bool normalFallback=false,normalContextKnown=false,usedFeatureOverride=false,conversionKnown=false;
    void *movementWorldCache=nullptr,*worldF8=nullptr,*connectionC0=nullptr;
    uint8_t rawCEE=0;int32_t state13c=0,value=0;
    float configured1580=0,previous1584=0,effectiveMilliseconds=0;
    uint32_t conversionMxcsr=0;
};
struct DelayContext {
    const char* stage="normal_delay_context_not_sampled";
    bool schema=false,strongLiveChain=false,clientDirect=false,settingsReady=false,mapKnown=false;
    bool settingFound=false,getterCalled=false,getterAgrees=false,selectorKnown=false;
    bool worldSettingsKnown=false,featurePresent=false,featureKnown=false,known=false;
    void *gameInstance=nullptr,*clientGameInstance=nullptr,*settingsManager=nullptr,*settingsOuter=nullptr;
    void *level=nullptr,*worldSettings=nullptr,*feature=nullptr,*featureOwner=nullptr;
    int32_t mapCount=0,mapCapacity=0,mapFree=0,hashSize=0,mapHops=0,settingValue=0,observedMapValue=0;
    uint8_t selector=0,featureRegistered=0;
    float featureMilliseconds=0;
    uint64_t getterCalls=0;
};
struct Row {
    bool links=false,pawnWeakKnown=false,stateWeakKnown=false,playerIdKnown=false,movementWorldKnown=false;
    bool managerMatchesShared=false,nativeCountersKnown=false,diffKnown=false;
    uint8_t role=255;int32_t playerId=-1,oldestDiff=0,newestDiff=0;
    Weak pawnWeak{},stateWeak{};Label actor{},playerState{};
    void *movement=nullptr,*movementOwner=nullptr,*movementCharacter=nullptr;
    Manager manager{};Clock clock{};Queue queue{};Delay delay{};
    uint32_t rawSerialized14d0=0,rawMisses15cc=0,rawConsecutive1588=0,rawOverqueue158c=0;
};
// Native registered movements can outlive PS.SpawnedCharacter after a death.
// These are observation records only; the original TSet is never modified.
inline constexpr size_t MaxRetainedRegistryRows=32,MaxPawnEvidence=120;
struct RegistryRow {
    void *movement=nullptr,*pawn=nullptr,*playerState=nullptr,*currentPawn=nullptr,*stream=nullptr;
    Weak pawnWeak{},stateWeak{};
    uint64_t lastCurrentAtMs=0;
    bool pawnWeakKnown=false,stateWeakKnown=false,links=false,worldKnown=false,currentKnown=false,current=false,wasOwn=false;
    bool aliveKnown=false,alive=false,destroyingKnown=false,destroying=false,lifeSpanKnown=false,respawnKnown=false;
    uint8_t role=255,streamProcessing=255;
    int32_t respawnNumber=-1,oldestDiff=0,newestDiff=0;
    float lifeSpan=-1;
    bool streamKnown=false,diffKnown=false,registeredFlagKnown=false,referenceMovement=false;
    uint8_t registeredFlag15e0=255;
    Clock clock{};Queue queue{};Delay delay{};
};
struct Registry {
    bool schema=false,shape=false,complete=false,lifecycleMetadata=false;
    bool referenceKnown=false;void* referenceMovement180=nullptr;
    int32_t slots=0,capacity=0,free=0,hashSize=0;
    uint32_t allocated=0,currentRole1=0,nonCurrentRole1=0,nonRole1=0,unknownIdentity=0,invalid=0;
    uint32_t knownDeadNonCurrent=0,queuedNonCurrent=0,sampled=0,truncated=0;
    std::array<RegistryRow,MaxRetainedRegistryRows> rows{};
};
struct PawnEvidence {
    void *world=nullptr,*pawn=nullptr,*playerState=nullptr,*movement=nullptr;
    Weak pawnWeak{},stateWeak{};uint64_t lastCurrentAtMs=0;bool wasOwn=false;
};
inline std::array<PawnEvidence,MaxPawnEvidence> pawnEvidence{};
inline void* pawnEvidenceWorld=nullptr;

struct Snapshot {
    const char* stage="remote_timeline_probe_not_sampled";
    bool profiles=false,schema=false,ownContext=false,rosterValid=false,managersSame=false;
    uint64_t atMs=0,samples=0,readExceptions=0,renderCacheAtMs=0;
    uint32_t profileFailureRva=0,sampled=0,invalid=0,skippedOwn=0,truncated=0;
    void *world=nullptr,*controller=nullptr,*ownPawn=nullptr,*gameState=nullptr,*connection=nullptr;
    void *pcManager=nullptr,*gsManager=nullptr;
    int32_t rosterCount=0;double worldTime=0;
    bool worldTimeKnown=false;Manager manager{};
    DelayContext normalDelay{};
    std::array<Row,MaxRemote> rows{};
    Registry registry{};
};
static_assert(std::is_trivially_copyable_v<Snapshot>);
struct Profile {uint32_t rva,bytes;uint64_t fnv;};
inline constexpr Profile Profiles[]{
    {0x1d4cfa0,1562,0xb050d7efb4cf1a25ULL},
    {0x1a7bf60,346,0xfdc2ceeff007caecULL},{0x18a2f10,61,0xe7b90b4f44d3a19fULL},
    {0x1a7b720,196,0x4982523f72f33036ULL},{0x1a7b4a0,217,0x7d57ad7174619f8fULL},
    {0x1a7b5a0,288,0x4fd28351a24aac64ULL},{0x1a79e80,395,0x488f4372a0e7dab9ULL},
    {0x1c63870,269,0xde792136130ef719ULL},{0x2a64ee0,132,0x83dfe973e78a6846ULL},
    {0x3ed83b0,1609,0x15d2d4ecc2f49cf7ULL},
    {0x6b662e0,174,0xcf0948dc72eaf2cbULL},{0x6c7d380,174,0xaeec39550aff18afULL},
    {0x6c5c480,174,0xb7d7706c24b54716ULL},{0x6cc4040,174,0x22d64e6011d4b830ULL},
    {0x6c9d280,174,0x618ffa5a020f15b6ULL},{0x6943db0,174,0x2f06235cee967932ULL},
    {0x42aae70,174,0x307bd36745be5788ULL},
    {0x7867050,592,0x849abd976af319aaULL},{0x78665f0,2194,0x29d783cb7c5d7bbULL},
    {0x7867460,1878,0xd7587abd52fddcf6ULL},{0x7867bc0,119,0x1f581be3f2f0009aULL},
    {0x7892ca0,366,0x716d776158a0b08dULL},{0x7894010,1714,0x57c150b56d3eecfbULL},
    {0x7893c40,970,0xc6ed6b56e03db669ULL},{0x789f7f0,43,0xea0bf92f43283ccaULL},
    {0x789edf0,35,0xb5911dd35a38dd26ULL},{0x789ec70,338,0x33d704226aa6bfe5ULL},
    {0x789ee60,550,0x909a57b98d314de2ULL},{0x6b6dec0,326,0xc20efdef30a606e4ULL},
    {0x789e050,309,0xf7862346225fb6f1ULL},{0x789e910,656,0x166cc623dda3986ULL},
    {0x703f040,157,0x51036d4d64c4de74ULL},{0x6e08e10,85,0x83f365a2d18de55eULL},
    {0x6429080,174,0x3b6a5fc91f8055bdULL},{0x647e8a0,174,0x6f07bcab93ebb2daULL},
    {0x65807e0,174,0xa5dd8138f49c087cULL},{0x6510860,174,0xc8b36d53c247bc29ULL},
    {0x6876390,174,0x6722909494a7297eULL},{0x3122cf0,167,0xdcc1332e6ac4befaULL},
    {0x37bf0a0,173,0x484ad53b27b9e36cULL},{0x1af0d20,174,0x1baa33679278c3b2ULL},
    {0x1c75610,158,0x53040a68dd75221cULL},{0x1ca8ca0,158,0x59d24cfb91d395ceULL},
    {0x1cad5f0,158,0x18688069f6764b7bULL},{0x1b80dc0,21,0x94ec83e60996bb32ULL},
    {0x1c9de40,158,0x309791e42c80c769ULL},{0x1cab2e0,158,0xbc1f3a6d3a81a26dULL},
    // The complete getter includes its success tail after the default RET.
    {0x75e67d0,116,0xe6f6753d49a10469ULL},
    {0xc6b3c28,3,0xb6b15a185877e1ecULL},{0xc8d0f10,4,0xd07fd5b3bb4b0561ULL},
    {0xc8d2c88,4,0x4d24c77f9dccea98ULL},
    {0x6c62390,56,0x29153e2fde2f0f5eULL},
    {0x6c5f2d0,31,0xaa685832e8f186f1ULL},
    {0x3eea190,37,0x31e9421eb0765366ULL},
    {0x3ee9d40,56,0xe4614b1703187333ULL},
};
inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline Snapshot cached{};
inline uint64_t lastSampleAt=0;
inline uintptr_t checkedImage=0;
inline bool profilesMatch=false;
inline uint32_t failedProfile=0;
struct SchemaCache {uintptr_t image=0;void *world=nullptr,*pcType=nullptr,*pawnType=nullptr,*gsType=nullptr;uint64_t triedAt=0;bool known=false;};
inline SchemaCache schemaCache{};
struct DelaySchemaCache {uintptr_t image=0;uint64_t triedAt=0;bool known=false;};
inline DelaySchemaCache delaySchemaCache{};
struct LabelCache {void *object=nullptr,*type=nullptr;int32_t index=-1;Name name{},typeName{};Label label{};};
inline std::array<LabelCache,MaxRemote*2> labelCache{};

template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
template<class T> inline bool ReadInto(const void* p,size_t offset,T& value){return original_players::Read(p,offset,value);}
inline void Publish(const Snapshot& s){AcquireSRWLockExclusive(&cacheLock);cached=s;ReleaseSRWLockExclusive(&cacheLock);}
inline Snapshot GetSnapshot(){AcquireSRWLockShared(&cacheLock);const auto s=cached;ReleaseSRWLockShared(&cacheLock);return s;}
inline bool VerifyProfiles(){
    if(checkedImage==engine::image)return profilesMatch;
    checkedImage=engine::image;profilesMatch=false;failedProfile=0;
    if(!engine::image)return false;
    for(const auto& p:Profiles){const auto bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes)){failedProfile=p.rva;return false;}
        uint64_t value=14695981039346656037ULL;
        for(uint32_t i=0;i<p.bytes;++i)value=(value^bytes[i])*1099511628211ULL;
        if(value!=p.fnv){failedProfile=p.rva;return false;}}
    profilesMatch=true;return true;
}
inline void* Property(void* type,const wchar_t* text,int32_t offset,int32_t bytes=0){
    Name name{};
    // 1A7B5A0: mode1 takes Add; mode0 takes existing-name Find 1A79E80.
    // A missing metadata name stays unknown. No FName pool entry is added.
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&name,text,0);
    if(!name.words[0]&&!name.words[1])return nullptr;
    auto property=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,&name,3);
    const auto fieldType=Read<void*>(property,8);
    const bool matches=property&&(Read<uint64_t>(fieldType,0x18)&0x8001)==0x8001
        &&Read<uint64_t>(property,0x20)==Read<uint64_t>(&name)
        &&Read<int32_t>(property,0x44)==offset&&(!bytes||Read<int32_t>(property,0x34)==bytes);
    return matches?property:nullptr;
}
inline bool Field(void* type,const wchar_t* text,int32_t offset,int32_t bytes=0){return Property(type,text,offset,bytes)!=nullptr;}
inline bool PlainObjectProperty(void* field){
    if(!field)return false;
    const auto object=engine::function<void*(*)()>(0x1c75610)();
    const auto soft=engine::function<void*(*)()>(0x1ca8ca0)();
    const auto weak=engine::function<void*(*)()>(0x1cad5f0)();
    const auto is=engine::function<bool(*)(void*,void*)>(0x1b80dc0);
    return object&&soft&&weak&&is(field,object)&&!is(field,soft)&&!is(field,weak);
}
inline bool StrongField(void* type,const wchar_t* text,int32_t offset,void* objectClass=nullptr){
    const auto field=Property(type,text,offset,8);
    return PlainObjectProperty(field)&&Read<int32_t>(field,0x30)==1
        &&(!objectClass||Read<void*>(field,0x78)==objectClass);
}
inline bool NormalDelaySchema(uint64_t now){
    if(delaySchemaCache.image==engine::image&&(delaySchemaCache.known||now-delaySchemaCache.triedAt<1000))
        return delaySchemaCache.known;
    delaySchemaCache={engine::image,now,false};
    const auto world=engine::Class(0x3122cf0),level=engine::Class(0x37bf0a0);
    const auto gi=engine::Class(0x647e8a0),client=engine::Class(0x6429080);
    const auto settings=engine::Class(0x6510860),ws=engine::Class(0x65807e0),feature=engine::Class(0x6876390);
    const auto systems=Property(client,L"Systems",0x2d0,0x520);
    const auto systemsType=Read<void*>(systems,0x78);
    const auto floats=Property(feature,L"GameFeatureFloatConfigValues",0x138,4);
    const auto structProperty=engine::function<void*(*)()>(0x1cab2e0)();
    const auto floatProperty=engine::function<void*(*)()>(0x1c9de40)();
    delaySchemaCache.known=StrongField(world,L"PersistentLevel",0x38,level)
        &&StrongField(world,L"OwningGameInstance",0x1d8)&&StrongField(world,L"DemoNetDriver",0xf8)
        &&StrongField(level,L"WorldSettings",0x2a8)
        &&StrongField(gi,L"ClientGameInstance",0x2b8,client)&&StrongField(client,L"MyGameInstance",0x2c8,gi)
        &&systems&&structProperty&&Read<void*>(systems,8)==structProperty
        &&Read<int32_t>(systems,0x30)==1&&original_players::IsA(systemsType,0x1af0d20)
        &&Read<int32_t>(systemsType,0x60)==0x520&&StrongField(systemsType,L"AresSettingsManager",0xa8,settings)
        &&StrongField(ws,L"GameFeatureTogglesComponent",0x800,feature)
        &&floats&&floatProperty&&Read<void*>(floats,8)==floatProperty
        &&Read<int32_t>(floats,0x30)==72&&Read<int32_t>(feature,0x60)==0x23c0
        &&Read<int32_t>(settings,0x60)==0x918;
    return delaySchemaCache.known;
}
inline bool TypedInstance(void* object,uintptr_t getter){
    int32_t index=-1;uint32_t flags=0;
    return object&&ReadInto(object,0xc,index)&&index>=0&&ReadInto(object,8,flags)
        &&(flags&0x10)==0&&original_players::IsA(object,getter);
}
inline bool ReadSettingsMap(DelayContext& d){
    const auto settings=d.settingsManager;
    if(!ReadInto(settings,0x3f0,d.mapCount)||!ReadInto(settings,0x3f4,d.mapCapacity)
       ||!ReadInto(settings,0x41c,d.mapFree)||!ReadInto(settings,0x430,d.hashSize)
       ||d.mapCount<0||d.mapCount>4096||d.mapCapacity<d.mapCount||d.mapCapacity>8192
       ||d.mapFree<0||d.mapFree>d.mapCount)return false;
    // The original complete getter returns0 before any bucket access for an
    // empty map. This is a real category result, never a default delay value.
    if(d.mapCount==d.mapFree){d.observedMapValue=0;return true;}
    if(d.hashSize<1||d.hashSize>8192||(d.hashSize&(d.hashSize-1)))return false;
    auto data=Read<uint8_t*>(settings,0x3e8),external=Read<uint8_t*>(settings,0x428);
    if(!data)return false;
    if(!external&&d.hashSize!=1)return false;
    auto buckets=external?external:static_cast<uint8_t*>(settings)+0x420;
    int32_t index=-1;
    if(!ReadInto(buckets,size_t(uint32_t(d.hashSize-1)&0x42U)*4,index))return false;
    while(index!=-1){
        if(index<0||index>=d.mapCount||d.mapHops>=d.mapCount)return false;++d.mapHops;
        const auto item=data+static_cast<size_t>(index)*16;
        uint8_t key=0;if(!ReadInto(item,0,key))return false;
        if(key==0x42){d.settingFound=true;return ReadInto(item,4,d.observedMapValue);}
        if(!ReadInto(item,8,index))return false;
    }
    d.observedMapValue=0;return true;
}
inline uint64_t nativeSettingsGetterCalls=0;
inline DelayContext SampleNormalDelayContext(void* world,uint64_t now){
    DelayContext d{};d.getterCalls=nativeSettingsGetterCalls;d.schema=NormalDelaySchema(now);
    if(!d.schema){d.stage="normal_delay_reflected_layout_unknown";return d;}
    if(!TypedInstance(world,0x3122cf0)){d.stage="normal_delay_world_type_unknown";return d;}
    d.gameInstance=Read<void*>(world,0x1d8);
    if(TypedInstance(d.gameInstance,0x6429080)){d.clientGameInstance=d.gameInstance;d.clientDirect=true;}
    else if(TypedInstance(d.gameInstance,0x647e8a0)){
        d.clientGameInstance=Read<void*>(d.gameInstance,0x2b8);
        if(!TypedInstance(d.clientGameInstance,0x6429080)||Read<void*>(d.clientGameInstance,0x2c8)!=d.gameInstance){
            d.stage="normal_delay_client_game_instance_link_unknown";return d;}
    }else {d.stage="normal_delay_game_instance_unknown";return d;}
    d.settingsManager=Read<void*>(d.clientGameInstance,0x378);d.settingsOuter=Read<void*>(d.settingsManager,0x28);
    if(!TypedInstance(d.settingsManager,0x6510860)||d.settingsOuter!=d.clientGameInstance){
        d.stage="normal_delay_settings_owned_instance_unknown";return d;}
    d.settingsReady=Read<uint8_t>(d.settingsManager,0x890)!=0;
    if(!d.settingsReady){d.stage="normal_delay_settings_not_initialized";return d;}
    d.mapKnown=ReadSettingsMap(d);
    if(!d.mapKnown){d.stage="normal_delay_settings_map_shape_unknown";return d;}
    // Retained75E67D0 through6843 is a no-call/no-store scalar getter. The
    // exact same chain is bounded/validated above before executing it once.
    d.settingValue=engine::function<int32_t(*)(void*,uint8_t)>(0x75e67d0)(d.settingsManager,0x42);
    d.getterCalled=true;d.getterCalls=++nativeSettingsGetterCalls;d.getterAgrees=d.settingValue==d.observedMapValue;
    if(!d.getterAgrees){d.stage="normal_delay_settings_getter_readback_changed";return d;}
    if(d.settingValue<0||d.settingValue>2){d.stage="normal_delay_settings_selector_outside_recovered_table";return d;}
    d.selector=Read<uint8_t>(reinterpret_cast<void*>(engine::image),0xc6b3c28+static_cast<size_t>(d.settingValue));
    d.selectorKnown=d.selector==uint8_t(d.settingValue+4);
    if(!d.selectorKnown){d.stage="normal_delay_selector_table_unknown";return d;}
    d.level=Read<void*>(world,0x38);
    if(!TypedInstance(d.level,0x37bf0a0)){d.stage="normal_delay_persistent_level_unknown";return d;}
    d.worldSettings=Read<void*>(d.level,0x2a8);
    d.worldSettingsKnown=TypedInstance(d.worldSettings,0x65807e0)
        &&Read<void*>(Read<void**>(d.worldSettings),0x1b8)==reinterpret_cast<void*>(engine::image+0x3ed83b0)
        &&engine::function<void*(*)(void*)>(0x3ed83b0)(d.worldSettings)==world;
    if(!d.worldSettingsKnown){d.stage="normal_delay_world_settings_owner_unknown";return d;}
    d.feature=Read<void*>(d.worldSettings,0x800);d.featurePresent=d.feature!=nullptr;
    if(d.featurePresent){
        d.featureOwner=Read<void*>(d.feature,0xc8);const auto featureWorld=Read<void*>(d.feature,0xd0);
        d.featureKnown=TypedInstance(d.feature,0x6876390)&&d.featureOwner==d.worldSettings
            &&Read<void*>(d.feature,0x28)==d.worldSettings
            &&(featureWorld==world||!featureWorld)
            &&Read<void*>(Read<void**>(d.feature),0x1b8)==reinterpret_cast<void*>(engine::image+0x2a64ee0)
            &&ReadInto(d.feature,0x138+size_t(d.selector)*4,d.featureMilliseconds);
        d.featureRegistered=uint8_t(Read<uint8_t>(d.feature,0xa0)&1);
        if(!d.featureKnown){d.stage="normal_delay_feature_toggle_owned_instance_unknown";return d;}
    }
    // Every instance remains held by the validated original strong-property
    // chain on this same GT. No weak serial is allocated for these objects.
    d.strongLiveChain=true;d.known=true;d.stage="normal_delay_actual_settings_and_feature_value_cached";return d;
}
inline bool Schema(const Snapshot& s){
    const auto pcType=Read<void*>(s.controller,0x10),pawnType=Read<void*>(s.ownPawn,0x10),gsType=Read<void*>(s.gameState,0x10);
    const bool same=schemaCache.image==engine::image&&schemaCache.world==s.world&&schemaCache.pcType==pcType
        &&schemaCache.pawnType==pawnType&&schemaCache.gsType==gsType;
    if(same&&(schemaCache.known||s.atMs-schemaCache.triedAt<1000))return schemaCache.known;
    schemaCache={engine::image,s.world,pcType,pawnType,gsType,s.atMs,false};
    const auto ps=engine::Class(original_players::ShooterStateClass),movement=engine::Class(0x6c7d380),manager=engine::Class(0x6b662e0);
    schemaCache.known=Field(pcType,L"RemoteClientMovementComponent",0xd10,8)
        &&Field(gsType,L"RemoteClientMovementComponent",0xcc8,8)&&Field(gsType,L"PlayerArray",0x480,16)
        &&Field(pawnType,L"PlayerState",0x488,8)&&Field(pawnType,L"CharacterMovement",0x4f8,8)
        &&Field(ps,L"SpawnedCharacter",0xbc8,8)&&Field(engine::Class(0x42aae70),L"PlayerId",0x468,4)
        &&Field(movement,L"RemoteClientMovementComponent",0x15d8,8)
        &&Field(movement,L"SerializedMovesReceivedSinceLastTimestep",0x14d0,4)
        &&Field(manager,L"NumRebases",0xe0,4)&&Field(manager,L"NumRebasesFromOverqueue",0xe4,4)
        &&Field(manager,L"RemoteCharacterMovementComponents",0x120);
    return schemaCache.known;
}
inline bool SameName(const Name& a,const Name& b){return std::memcmp(&a,&b,sizeof(Name))==0;}
inline bool FormatName(const Name& name,char (&out)[128]){
    struct String {wchar_t* data=nullptr;int32_t count=0,capacity=0;} text;
    out[0]=0;
    engine::function<void*(*)(const void*,void*)>(0x1a7bf60)(&name,&text);
    bool ok=false;
    if(text.data&&text.count>0&&text.count<=1024&&text.capacity>=text.count){
        const int count=WideCharToMultiByte(CP_UTF8,0,text.data,text.count-1,out,127,nullptr,nullptr);
        if(count>0){out[count]=0;ok=true;}}
    engine::function<void(*)(void*)>(0x18a2f10)(&text);
    return ok;
}
inline Label CaptureLabel(void* object,size_t cacheIndex){
    Label result{};result.object=object;
    result.rawKnown=object&&ReadInto(object,0xc,result.internalIndex)&&result.internalIndex>=0
        &&ReadInto(object,0x10,result.type)&&result.type&&ReadInto(object,0x18,result.objectName)
        &&ReadInto(result.type,0x18,result.className);
    if(!result.rawKnown)return result;
    auto& cache=labelCache[cacheIndex];
    if(cache.object==object&&cache.type==result.type&&cache.index==result.internalIndex
       &&SameName(cache.name,result.objectName)&&SameName(cache.typeName,result.className))return cache.label;
    result.nameKnown=FormatName(result.objectName,result.name);
    result.classNameKnown=FormatName(result.className,result.classText);
    cache={object,result.type,result.internalIndex,result.objectName,result.className,result};return result;
}
inline bool ExistingWeak(void* object,const local_server_remote_render_probe::Weak& source,Weak& output){
    output={Read<int32_t>(object,0xc),0};
    if(!object||source.index<0||source.serial<=0||output.index!=source.index)return false;
    Weak candidate{source.index,source.serial};
    if(engine::function<void*(*)(const Weak*)>(0x1d4cfa0)(&candidate)!=object)return false;
    output=candidate;return true;
}
inline Manager SampleManager(void* object,bool schema){
    Manager m{};m.object=object;m.schema=schema;
    m.typed=original_players::IsA(object,0x6b662e0);if(!m.typed||!schema)return m;
    m.fieldsKnown=ReadInto(object,0xc8,m.owner)&&ReadInto(object,0x104,m.currentIndex)
        &&ReadInto(object,0x108,m.currentFraction)&&ReadInto(object,0x10c,m.previousIndex)
        &&ReadInto(object,0x110,m.previousFraction)&&ReadInto(object,0x114,m.atWorldTime)
        &&ReadInto(object,0xe0,m.numRebases)&&ReadInto(object,0xe4,m.numRebasesFromOverqueue)
        &&ReadInto(object,0x100,m.raw100)&&ReadInto(object,0x101,m.raw101)
        &&ReadInto(object,0x118,m.raw118)&&ReadInto(object,0x119,m.raw119);
    return m;
}
inline Move SampleMove(void* object){
    Move m{};m.object=object;
    m.known=Read<void*>(object)==reinterpret_cast<void*>(engine::image+0xb600b78)
        &&ReadInto(object,0x1c,m.timeIndex)&&ReadInto(object,0x469,m.firstAfterReset469)
        &&ReadInto(object,0x470,m.sourceIndex470)&&m.firstAfterReset469<=1;
    return m;
}
inline uint32_t InverseIndex(uint32_t raw,int32_t offset){
    // Original 789EDF0, including its zero-skipping and modulo32 arithmetic.
    const uint32_t delta=uint32_t(0)-static_cast<uint32_t>(offset),mapped=raw+delta;
    return mapped||!delta?mapped:((delta&0x80000000U)?UINT32_MAX:uint32_t(1));
}
inline Clock SampleClock(void* movement){
    Clock c{};c.object=Read<void*>(movement,0x13f0);if(!c.object)return c;
    c.table=Read<void*>(c.object);c.vbtable=Read<void*>(c.object,0x10);
    c.schema=c.table==reinterpret_cast<void*>(engine::image+0xb600e70)
        &&c.vbtable==reinterpret_cast<void*>(engine::image+0xc6b3c30)
        &&Read<int32_t>(c.vbtable,4)==0xe20
        &&Read<void*>(c.table,0)==reinterpret_cast<void*>(engine::image+0x789e910)
        &&Read<void*>(c.table,0x10)==reinterpret_cast<void*>(engine::image+0x789f7f0)
        &&Read<void*>(c.table,0x20)==reinterpret_cast<void*>(engine::image+0x789edf0);
    if(!c.schema)return c;
    c.baseOffset=0xe20;
    c.fieldsKnown=ReadInto(c.object,0xe38,c.rawIndex)&&ReadInto(c.object,0xe3c,c.step)
        &&ReadInto(c.object,0x20,c.accumulated)&&ReadInto(c.object,0x24,c.indexOffset)
        &&ReadInto(c.object,0x28,c.raw28)&&ReadInto(c.object,0x4d8,c.initialized4d8)
        &&ReadInto(c.object,0x4c8,c.historyNext)&&ReadInto(c.object,0x4cc,c.historyCapacity)
        &&ReadInto(c.object,0x4d0,c.historyCount)&&ReadInto(c.object,0x4dc,c.lastQueuedMappedIndex)
        &&ReadInto(c.object,0x968,c.frameOrdinal)&&ReadInto(c.object,0x96c,c.pendingMoves)
        &&ReadInto(c.object,0x9a0,c.lead9a0);
    if(!c.fieldsKnown)return c;
    c.inverseKnown=c.step>0&&c.step<=128&&std::isfinite(c.accumulated);
    if(c.inverseKnown){c.serverDomainIndex=InverseIndex(c.rawIndex,c.indexOffset);
        c.lastQueuedServerDomainIndex=InverseIndex(c.lastQueuedMappedIndex,c.indexOffset);
        c.lastQueuedInverseKnown=c.initialized4d8!=0;}
    c.reference=SampleMove(static_cast<uint8_t*>(c.object)+0x30);return c;
}
inline Queue SampleQueue(void* movement){
    Queue q{};
    q.shapeKnown=ReadInto(movement,0x1488,q.data)&&ReadInto(movement,0x1498,q.nextWrite)
        &&ReadInto(movement,0x149c,q.capacity)&&ReadInto(movement,0x14a0,q.count)
        &&q.capacity>=0&&q.capacity<=128&&q.count>=0&&q.count<=q.capacity;
    if(!q.shapeKnown)return q;
    if(!q.capacity){q.shapeKnown=!q.count&&!q.nextWrite;return q;}
    q.shapeKnown=q.data&&q.nextWrite>=0&&q.nextWrite<q.capacity;if(!q.shapeKnown||!q.count)return q;
    const int32_t oldest=(q.nextWrite-q.count+q.capacity)%q.capacity,newest=(q.nextWrite-1+q.capacity)%q.capacity;
    q.oldest=SampleMove(static_cast<uint8_t*>(q.data)+static_cast<size_t>(oldest)*0x480);
    q.newest=SampleMove(static_cast<uint8_t*>(q.data)+static_cast<size_t>(newest)*0x480);
    q.entriesKnown=q.oldest.known&&q.newest.known;return q;
}
inline bool ConvertDelayMilliseconds(float milliseconds,uint32_t step,Delay& d){
    uint32_t bits=0;std::memcpy(&bits,&milliseconds,sizeof(bits));
    if((bits&0x7f800000U)==0x7f800000U||((bits&0x80000000U)&&(bits&0x7fffffffU))||!step||step>128)return false;
    const uint32_t incomingMxcsr=_mm_getcsr();d.conversionMxcsr=incomingMxcsr;
    bool known=false;
    __try {
        // Match789E12C..17B exactly, including current SSE rounding, signed
        // arithmetic-shift semantics, modulo32 multiplication and final int ->
        // float -> truncation. Do not alter the thread's FP control/status.
        const auto stepFloat=_mm_cvtsi32_ss(_mm_setzero_ps(),static_cast<int32_t>(step));
        auto scaled=_mm_div_ss(_mm_set_ss(Read<float>(reinterpret_cast<void*>(engine::image),0xc8d0f10)),stepFloat);
        scaled=_mm_mul_ss(scaled,_mm_set_ss(milliseconds));scaled=_mm_add_ss(scaled,scaled);
        scaled=_mm_sub_ss(_mm_set_ss(Read<float>(reinterpret_cast<void*>(engine::image),0xc8d2c88)),scaled);
        const float roundingInput=_mm_cvtss_f32(scaled);
        if(std::isfinite(roundingInput)&&roundingInput>-2147483000.f&&roundingInput<2147483000.f){
            const int32_t rounded=_mm_cvtss_si32(scaled);uint32_t raw=0;std::memcpy(&raw,&rounded,sizeof(raw));
            const uint32_t half=(raw>>1)|(raw&0x80000000U),negated=uint32_t(0)-half*step;
            int32_t integer=0;std::memcpy(&integer,&negated,sizeof(integer));
            const auto converted=_mm_cvtsi32_ss(_mm_setzero_ps(),integer);
            const float convertedFloat=_mm_cvtss_f32(converted);
            if(integer>=0&&convertedFloat<2147483000.f){d.value=_mm_cvttss_si32(converted);known=d.value>=0;}
        }
    }__finally {_mm_setcsr(incomingMxcsr);}
    return known;
}
inline Delay SampleDelay(void* world,void* movement,bool worldSourceKnown,const Clock& clock,const DelayContext& context){
    Delay d{};d.worldSourceKnown=worldSourceKnown;
    d.fieldsKnown=ReadInto(movement,0x1580,d.configured1580)&&ReadInto(movement,0x1584,d.previous1584)
        &&ReadInto(movement,0xd0,d.movementWorldCache);
    if(!d.fieldsKnown||!worldSourceKnown)return d;
    // World+F8 is DemoNetDriver, not the normal NetDriver. Only its exact
    // original replay branch can establish zero without the normal config.
    d.chainKnown=ReadInto(world,0xf8,d.worldF8);
    if(!d.chainKnown)return d;
    if(d.worldF8){
        d.chainKnown=ReadInto(d.worldF8,0xcee,d.rawCEE);if(!d.chainKnown)return d;
        if(d.rawCEE){d.zeroBranchKnown=d.valueKnown=true;return d;}
        d.chainKnown=ReadInto(d.worldF8,0xc0,d.connectionC0);if(!d.chainKnown)return d;
        if(d.connectionC0){d.chainKnown=ReadInto(d.connectionC0,0x13c,d.state13c);if(!d.chainKnown)return d;
            if(d.state13c!=1){d.zeroBranchKnown=d.valueKnown=true;return d;}}
    }
    d.normalFallback=true;d.normalContextKnown=context.known;
    if(!context.known)return d;
    d.usedFeatureOverride=context.featurePresent&&context.featureKnown;
    d.effectiveMilliseconds=d.usedFeatureOverride?context.featureMilliseconds:d.configured1580;
    if(!clock.schema||!clock.fieldsKnown)return d;
    d.conversionKnown=ConvertDelayMilliseconds(d.effectiveMilliseconds,clock.step,d);
    d.valueKnown=d.conversionKnown;
    return d;
}
inline int32_t NativeDiff(uint32_t incoming,uint32_t step,int32_t delay,uint32_t anchor){
    const uint32_t raw=incoming-step-static_cast<uint32_t>(delay)-anchor;
    int32_t result=0;std::memcpy(&result,&raw,sizeof(result));return result;
}
inline bool RegistryWeakLive(void* object,const Weak& token){
    return object&&token.index>=0&&token.serial>0
        &&engine::function<void*(*)(const Weak*)>(0x1d4cfa0)(&token)==object;
}
inline void RememberRegistryPawn(void* world,void* pawn,void* playerState,void* movement,
                                 const Weak& pawnId,const Weak& stateId,uint64_t atMs,bool own){
    if(!engine::image||original_players::gameThread!=GetCurrentThreadId()
       ||!RegistryWeakLive(pawn,pawnId)||!RegistryWeakLive(playerState,stateId))return;
    if(pawnEvidenceWorld!=world){pawnEvidence={};pawnEvidenceWorld=world;}
    PawnEvidence* available=nullptr;PawnEvidence* oldest=nullptr;
    for(auto& value:pawnEvidence){
        if(value.pawn==pawn&&value.pawnWeak.index==pawnId.index&&value.pawnWeak.serial==pawnId.serial){
            value={world,pawn,playerState,movement,pawnId,stateId,atMs,value.wasOwn||own};return;
        }
        if(!available&&(!value.pawn||!RegistryWeakLive(value.pawn,value.pawnWeak)))available=&value;
        if(!oldest||value.lastCurrentAtMs<oldest->lastCurrentAtMs)oldest=&value;
    }
    if(!available)available=oldest;
    if(available)*available={world,pawn,playerState,movement,pawnId,stateId,atMs,own};
}
// Root supplies already-captured native identity tokens for its actual owner.
// This retains evidence of an old autonomous Pawn if it later becomes Role1;
// neither CaptureIdentity nor a serial allocator is called by this observer.
inline void RememberBorrowedOwnLife(void* world,void* pawn,int32_t pawnIndex,int32_t pawnSerial,
                                   void* playerState,int32_t stateIndex,int32_t stateSerial){
    const Weak pawnId{pawnIndex,pawnSerial},stateId{stateIndex,stateSerial};
    if(!engine::image||original_players::gameThread!=GetCurrentThreadId()
       ||!RegistryWeakLive(pawn,pawnId)||!RegistryWeakLive(playerState,stateId))return;
    RememberRegistryPawn(world,pawn,playerState,Read<void*>(pawn,0x4f8),pawnId,stateId,GetTickCount64(),true);
}
inline const PawnEvidence* RegistryPawnEvidence(void* world,void* pawn,void* movement){
    for(const auto& value:pawnEvidence)if(value.world==world&&value.pawn==pawn&&value.movement==movement
       &&RegistryWeakLive(pawn,value.pawnWeak)&&RegistryWeakLive(value.playerState,value.stateWeak))return &value;
    return nullptr;
}
struct RegistryLifecycleMethods {void *alive=nullptr,*respawn=nullptr,*lifeSpan=nullptr,*destroying=nullptr;bool known=false;};
inline RegistryLifecycleMethods RegistryLifeMethods(){
    RegistryLifecycleMethods result{};
    const auto character=engine::Class(original_players::ShooterCharacterClass),actor=engine::Class(0x3683900);
    const auto method=[](void*& out,const wchar_t* path,void* owner,uint16_t bytes,uint32_t flags,uintptr_t exec){
        out=engine::Find(path);return out&&Read<void*>(out,0x28)==owner&&Read<uint16_t>(out,0xbe)==bytes
            &&Read<uint32_t>(out,0xb8)==flags&&Read<void*>(out,0xe0)==reinterpret_cast<void*>(engine::image+exec);
    };
    result.known=method(result.alive,L"/Script/ShooterGame.ShooterCharacter.IsAlive",character,1,0x54020401,0x6c62390)
        &&method(result.respawn,L"/Script/ShooterGame.ShooterCharacter.GetRespawnNumber",character,4,0x54020401,0x6c5f2d0)
        &&method(result.lifeSpan,L"/Script/Engine.Actor.GetLifeSpan",actor,4,0x54020402,0x3ee9d40)
        &&method(result.destroying,L"/Script/Engine.Actor.IsActorBeingDestroyed",actor,1,0x54020401,0x3eea190);
    return result;
}
inline bool RegistryQuery(void* object,void* method,void* params){
    auto dispatch=Read<void*>(Read<void**>(object),0x298);if(!object||!method||!dispatch)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(dispatch)(object,method,params);return true;
}
inline void SampleRetainedRegistryRow(Snapshot& next,void* movement,const RegistryLifecycleMethods& methods){
    auto& registry=next.registry;
    if(!original_players::IsA(movement,0x6c7d380)){++registry.invalid;return;}
    const auto pawn=Read<void*>(movement,0xc8);const auto known=RegistryPawnEvidence(next.world,pawn,movement);
    RegistryRow row{};row.movement=movement;row.pawn=pawn;
    row.referenceMovement=registry.referenceKnown&&registry.referenceMovement180==movement;
    row.registeredFlagKnown=ReadInto(movement,0x15e0,row.registeredFlag15e0);
    if(!known){
        ++registry.unknownIdentity;
        if(registry.sampled<registry.rows.size())registry.rows[registry.sampled++]=row;else ++registry.truncated;
        return;
    }
    row.playerState=known->playerState;row.pawnWeak=known->pawnWeak;row.stateWeak=known->stateWeak;
    row.pawnWeakKnown=row.stateWeakKnown=true;row.lastCurrentAtMs=known->lastCurrentAtMs;row.wasOwn=known->wasOwn;
    row.role=Read<uint8_t>(pawn,0x224);
    row.links=Read<void*>(pawn,0x4f8)==movement&&Read<void*>(movement,0x1a8)==pawn
        &&Read<void*>(pawn,0x488)==row.playerState;
    row.worldKnown=row.links&&Read<void*>(Read<void**>(pawn),0x1b8)==reinterpret_cast<void*>(engine::image+0x3ed83b0)
        &&engine::function<void*(*)(void*)>(0x3ed83b0)(pawn)==next.world;
    row.currentKnown=row.worldKnown&&ReadInto(row.playerState,0xbc8,row.currentPawn);
    row.current=row.currentKnown&&row.currentPawn==pawn;
    if(row.role!=1){++registry.nonRole1;return;}
    if(!row.links||!row.worldKnown||!row.currentKnown){++registry.invalid;}
    else if(row.current){++registry.currentRole1;return;}else ++registry.nonCurrentRole1;
    if(row.links&&row.worldKnown){
        row.clock=SampleClock(movement);row.queue=SampleQueue(movement);
        const auto cachedWorld=Read<void*>(movement,0xd0);
        const bool movementWorldKnown=Read<void*>(Read<void**>(movement),0x1b8)==reinterpret_cast<void*>(engine::image+0x2a64ee0)
            &&(cachedWorld==next.world||(!cachedWorld&&(Read<uint32_t>(pawn,8)&0x10)==0));
        row.delay=SampleDelay(next.world,movement,movementWorldKnown,row.clock,next.normalDelay);
        row.diffKnown=next.manager.fieldsKnown&&next.manager.currentIndex&&row.clock.fieldsKnown&&row.clock.step>0
            &&row.queue.entriesKnown&&row.delay.valueKnown;
        if(row.diffKnown){row.oldestDiff=NativeDiff(row.queue.oldest.timeIndex,row.clock.step,row.delay.value,next.manager.currentIndex);
            row.newestDiff=NativeDiff(row.queue.newest.timeIndex,row.clock.step,row.delay.value,next.manager.currentIndex);}
        row.stream=Read<void*>(pawn,0xba8);
        row.streamKnown=original_players::IsA(row.stream,0x69d5850)
            &&Field(Read<void*>(row.stream,0x10),L"bIsProcessingStreamData",0xe0,1)
            &&Read<void*>(row.stream,0xc8)==pawn;
        if(row.streamKnown)row.streamProcessing=Read<uint8_t>(row.stream,0xe0);
        if(methods.known){
            uint8_t alive=255,destroying=255;
            row.aliveKnown=RegistryQuery(pawn,methods.alive,&alive)&&alive<=1;row.alive=alive==1;
            row.destroyingKnown=RegistryQuery(pawn,methods.destroying,&destroying)&&destroying<=1;row.destroying=destroying==1;
            row.lifeSpanKnown=RegistryQuery(pawn,methods.lifeSpan,&row.lifeSpan)&&std::isfinite(row.lifeSpan);
            row.respawnKnown=RegistryQuery(pawn,methods.respawn,&row.respawnNumber);
        }
        if(!row.current&&row.aliveKnown&&!row.alive)++registry.knownDeadNonCurrent;
        if(!row.current&&row.queue.shapeKnown&&row.queue.count>0)++registry.queuedNonCurrent;
    }
    if(registry.sampled<registry.rows.size())registry.rows[registry.sampled++]=row;else ++registry.truncated;
}
inline void SampleRegisteredMovements(Snapshot& next){
    auto& registry=next.registry;registry.schema=next.schema&&next.managersSame&&next.manager.typed;
    if(!registry.schema)return;
    for(uint32_t n=0;n<next.sampled&&n<MaxRemote;++n){const auto& row=next.rows[n];
        if(row.pawnWeakKnown&&row.stateWeakKnown)
            RememberRegistryPawn(next.world,row.actor.object,row.playerState.object,row.movement,row.pawnWeak,row.stateWeak,next.atMs,false);
    }
    auto manager=next.gsManager;void* elements=nullptr;void* heapBuckets=nullptr;
    registry.referenceKnown=ReadInto(manager,0x180,registry.referenceMovement180);
    if(!ReadInto(manager,0x120,elements)||!ReadInto(manager,0x128,registry.slots)||!ReadInto(manager,0x12c,registry.capacity)
       ||!ReadInto(manager,0x154,registry.free)||!ReadInto(manager,0x160,heapBuckets)||!ReadInto(manager,0x168,registry.hashSize))return;
    if(registry.slots<0||registry.slots>512||registry.capacity<registry.slots||registry.free<0||registry.free>registry.slots
       ||registry.hashSize<1||registry.hashSize>8192||(registry.hashSize&(registry.hashSize-1)))return;
    const auto buckets=heapBuckets?heapBuckets:static_cast<uint8_t*>(manager)+0x158;
    if((!heapBuckets&&registry.hashSize!=1)||!original_players::Readable(buckets,static_cast<size_t>(registry.hashSize)*4)
       ||(registry.slots&&(!elements||!original_players::Readable(elements,static_cast<size_t>(registry.slots)*16))))return;
    registry.shape=true;const auto methods=RegistryLifeMethods();registry.lifecycleMetadata=methods.known;
    std::array<bool,512> visited{};
    for(int32_t bucket=0;bucket<registry.hashSize;++bucket){int32_t index=Read<int32_t>(buckets,static_cast<size_t>(bucket)*4);
        while(index!=-1){
            if(index<0||index>=registry.slots||visited[static_cast<size_t>(index)]){registry.shape=false;return;}
            visited[static_cast<size_t>(index)]=true;++registry.allocated;
            auto entry=static_cast<uint8_t*>(elements)+static_cast<size_t>(index)*16;
            auto movement=Read<void*>(entry);if(movement)SampleRetainedRegistryRow(next,movement,methods);else ++registry.invalid;
            index=Read<int32_t>(entry,8);
        }
    }
    registry.complete=registry.allocated==static_cast<uint32_t>(registry.slots-registry.free);
}

inline bool PollImpl(void* world,void* pc,void* ownPawn,uint64_t now){
    Snapshot next{};next.atMs=now;next.world=world;next.controller=pc;next.ownPawn=ownPawn;
    {const auto previous=GetSnapshot();next.samples=previous.samples+1;next.readExceptions=previous.readExceptions;}
    next.profiles=VerifyProfiles();next.profileFailureRva=failedProfile;
    if(!next.profiles){next.stage="remote_timeline_probe_profile_unknown";Publish(next);return true;}
    next.connection=Read<void*>(Read<void*>(world,0x40),0xc0);
    next.ownContext=original_players::IsA(pc,original_players::ShooterControllerClass)
        &&original_players::IsA(ownPawn,original_players::ShooterCharacterClass)
        &&Read<uint8_t>(pc,0x224)==2&&Read<uint8_t>(ownPawn,0x224)==2
        &&Read<void*>(pc,0x4b0)==ownPawn&&Read<void*>(ownPawn,0x4a0)==pc
        &&next.connection&&Read<void*>(next.connection,0x38)==pc
        &&Read<void*>(next.connection,0x60)==Read<void*>(world,0x40)&&Read<uint8_t>(next.connection,0x13c)==3;
    if(!next.ownContext){next.stage="remote_timeline_probe_waiting_received_owned_client";Publish(next);return true;}
    next.gameState=Read<void*>(world,0x178);
    if(!original_players::IsA(next.gameState,original_players::ShooterGameStateClass)){
        next.stage="remote_timeline_probe_game_state_unknown";Publish(next);return true;}
    next.schema=Schema(next);
    if(!next.schema){next.stage="remote_timeline_probe_property_schema_unknown";Publish(next);return true;}
    const auto roster=Read<Array>(next.gameState,0x480);next.rosterCount=roster.count;
    next.rosterValid=roster.count>=0&&roster.count<=128&&roster.capacity>=roster.count&&roster.capacity<=256
        &&(!roster.count||original_players::Readable(roster.data,static_cast<size_t>(roster.count)*8));
    if(!next.rosterValid){next.stage="remote_timeline_probe_roster_unknown";Publish(next);return true;}
    next.worldTimeKnown=ReadInto(world,0x740,next.worldTime)&&std::isfinite(next.worldTime);
    next.pcManager=Read<void*>(pc,0xd10);next.gsManager=Read<void*>(next.gameState,0xcc8);
    next.managersSame=next.pcManager&&next.pcManager==next.gsManager;
    next.manager=SampleManager(next.gsManager,next.schema);
    next.normalDelay=SampleNormalDelayContext(world,now);
    const auto render=local_server_remote_render_probe::GetSnapshot();next.renderCacheAtMs=render.atMs;
    const bool renderContext=render.world==world&&render.controller==pc&&render.ownPawn==ownPawn
        &&render.profiles&&render.schema&&render.ownContext&&now>=render.atMs&&now-render.atMs<=500;
    const auto ownPS=Read<void*>(pc,0x470);
    for(int32_t n=0;n<roster.count;++n){
        auto ps=Read<void*>(roster.data,static_cast<size_t>(n)*8),pawn=Read<void*>(ps,0xbc8);
        if(ps==ownPS||pawn==ownPawn){++next.skippedOwn;continue;}
        if(next.sampled>=MaxRemote){++next.truncated;continue;}
        if(!original_players::IsA(ps,original_players::ShooterStateClass)
           ||!original_players::IsA(pawn,original_players::ShooterCharacterClass)
           ||Read<uint8_t>(pawn,0x224)!=1||Read<void*>(pawn,0x488)!=ps
           ||Read<void*>(Read<void**>(pawn),0x1b8)!=reinterpret_cast<void*>(engine::image+0x3ed83b0)
           ||engine::function<void*(*)(void*)>(0x3ed83b0)(pawn)!=world){++next.invalid;continue;}
        Row row{};row.role=1;row.links=true;
        row.actor=CaptureLabel(pawn,static_cast<size_t>(next.sampled)*2);
        row.playerState=CaptureLabel(ps,static_cast<size_t>(next.sampled)*2+1);
        row.pawnWeak.index=row.actor.internalIndex;row.stateWeak.index=row.playerState.internalIndex;
        if(renderContext)for(uint32_t i=0;i<render.sampled&&i<render.rows.size();++i){const auto& r=render.rows[i];
            if(r.pawn==pawn&&r.playerState==ps){row.pawnWeakKnown=ExistingWeak(pawn,r.pawnId,row.pawnWeak);
                row.stateWeakKnown=ExistingWeak(ps,r.playerStateId,row.stateWeak);break;}}
        row.playerIdKnown=ReadInto(ps,0x468,row.playerId);
        row.movement=Read<void*>(pawn,0x4f8);row.movementOwner=Read<void*>(row.movement,0xc8);
        row.movementCharacter=Read<void*>(row.movement,0x1a8);
        if(!original_players::IsA(row.movement,0x6c7d380)||row.movementOwner!=pawn||row.movementCharacter!=pawn){++next.invalid;continue;}
        const auto cachedWorld=Read<void*>(row.movement,0xd0);
        row.movementWorldKnown=Read<void*>(Read<void**>(row.movement),0x1b8)==reinterpret_cast<void*>(engine::image+0x2a64ee0)
            &&(cachedWorld==world||(!cachedWorld&&(Read<uint32_t>(pawn,8)&0x10)==0));
        row.manager=SampleManager(Read<void*>(row.movement,0x15d8),next.schema);
        row.managerMatchesShared=next.managersSame&&row.manager.object==next.gsManager;
        row.clock=SampleClock(row.movement);row.queue=SampleQueue(row.movement);
        row.delay=SampleDelay(world,row.movement,row.movementWorldKnown,row.clock,next.normalDelay);
        row.nativeCountersKnown=ReadInto(row.movement,0x14d0,row.rawSerialized14d0)
            &&ReadInto(row.movement,0x15cc,row.rawMisses15cc)&&ReadInto(row.movement,0x1588,row.rawConsecutive1588)
            &&ReadInto(row.movement,0x158c,row.rawOverqueue158c);
        row.diffKnown=row.manager.fieldsKnown&&row.manager.currentIndex&&row.clock.fieldsKnown
            &&row.clock.step>0&&row.queue.entriesKnown&&row.delay.valueKnown;
        if(row.diffKnown){row.oldestDiff=NativeDiff(row.queue.oldest.timeIndex,row.clock.step,row.delay.value,row.manager.currentIndex);
            row.newestDiff=NativeDiff(row.queue.newest.timeIndex,row.clock.step,row.delay.value,row.manager.currentIndex);}
        next.rows[next.sampled++]=row;
    }
    SampleRegisteredMovements(next);
    next.stage="remote_timeline_probe_cached_actual_role1_native_time_domains";Publish(next);return true;
}
inline bool Poll(void* world,void* ownPC,void* ownPawn){
    if(!engine::image||!original_players::gameThread||GetCurrentThreadId()!=original_players::gameThread)return true;
    const auto now=GetTickCount64();if(now-lastSampleAt<MinimumIntervalMs)return true;lastSampleAt=now;
    __try{return PollImpl(world,ownPC,ownPawn,now);}
    __except(EXCEPTION_EXECUTE_HANDLER){const auto previous=GetSnapshot();Snapshot failed{};
        failed.atMs=now;failed.samples=previous.samples+1;failed.readExceptions=previous.readExceptions+1;failed.invalid=1;
        failed.world=world;failed.controller=ownPC;failed.ownPawn=ownPawn;
        failed.stage="remote_timeline_probe_read_exception_unknown";Publish(failed);return true;}
}
inline const char* Bool(bool value){return value?"true":"false";}
inline void Text(FILE* file,const char* value){
    std::fputc('"',file);
    if(value)for(const unsigned char* p=reinterpret_cast<const unsigned char*>(value);*p;++p){
        if(*p=='"'||*p=='\\'){std::fputc('\\',file);std::fputc(*p,file);}
        else if(*p<0x20)std::fprintf(file,"\\u%04x",static_cast<unsigned>(*p));else std::fputc(*p,file);}
    std::fputc('"',file);
}
inline void Number(FILE* file,double value,bool known=true){if(known&&std::isfinite(value))std::fprintf(file,"%.12g",value);else std::fputs("null",file);}
inline void WriteName(FILE* file,const Name& name){std::fprintf(file,"[%u,%u,%u]",name.words[0],name.words[1],name.words[2]);}
inline void WriteLabel(FILE* file,const Label& l){
    std::fprintf(file,"{\"object\":\"%p\",\"class\":\"%p\",\"internal_index\":%d,\"raw_name_known\":%s,\"fname_words\":",l.object,l.type,l.internalIndex,Bool(l.rawKnown));
    WriteName(file,l.objectName);std::fputs(",\"class_fname_words\":",file);WriteName(file,l.className);
    std::fprintf(file,",\"name_known\":%s,\"name\":",Bool(l.nameKnown));Text(file,l.name);
    std::fprintf(file,",\"class_name_known\":%s,\"class_name\":",Bool(l.classNameKnown));Text(file,l.classText);std::fputc('}',file);
}
inline void WriteManager(FILE* file,const Manager& m){
    std::fprintf(file,"{\"object\":\"%p\",\"owner\":\"%p\",\"typed\":%s,\"schema\":%s,\"fields_known\":%s,\"current_server_index104\":%u,\"current_fraction108\":",m.object,m.owner,Bool(m.typed),Bool(m.schema),Bool(m.fieldsKnown),m.currentIndex);
    Number(file,m.currentFraction,m.fieldsKnown);std::fprintf(file,",\"previous_server_index10c\":%u,\"previous_fraction110\":",m.previousIndex);Number(file,m.previousFraction,m.fieldsKnown);
    std::fputs(",\"sampled_world_time114\":",file);Number(file,m.atWorldTime,m.fieldsKnown);
    std::fprintf(file,",\"native_num_rebases_e0\":%d,\"native_num_rebases_from_overqueue_e4\":%d,\"raw100\":%u,\"raw101\":%u,\"raw118\":%u,\"raw119\":%u}",m.numRebases,m.numRebasesFromOverqueue,unsigned(m.raw100),unsigned(m.raw101),unsigned(m.raw118),unsigned(m.raw119));
}
inline void WriteMove(FILE* file,const Move& m){std::fprintf(file,"{\"known\":%s,\"object\":\"%p\",\"time_index1c\":%u,\"source_index470\":%u,\"first_after_reset469\":%u}",Bool(m.known),m.object,m.timeIndex,m.sourceIndex470,unsigned(m.firstAfterReset469));}
inline void WriteDelayContext(FILE* file,const DelayContext& d){
    std::fprintf(file,"{\"stage\":\"%s\",\"schema\":%s,\"strong_property_live_chain\":%s,\"native_object_weak_serial_allocations\":0,\"client_direct\":%s,\"game_instance\":\"%p\",\"client_game_instance\":\"%p\",\"settings_manager\":\"%p\",\"settings_outer\":\"%p\",\"settings_ready\":%s,\"map_known\":%s,\"map_count\":%d,\"map_capacity\":%d,\"map_free\":%d,\"hash_size\":%d,\"map_hops\":%d,\"setting_key\":66,\"setting_found\":%s,\"original_getter_rva\":\"0x75e67d0\",\"original_getter_complete_bytes\":116,\"original_getter_called\":%s,\"original_getter_calls_total\":%llu,\"original_getter_agrees_with_bounded_map_read\":%s,\"setting_value\":%d,\"observed_map_value\":%d,\"selector_known\":%s,\"selector\":%u,\"level\":\"%p\",\"world_settings\":\"%p\",\"world_settings_known\":%s,\"feature_component\":\"%p\",\"feature_owner\":\"%p\",\"feature_present\":%s,\"feature_known\":%s,\"feature_registered\":%u,\"feature_float_array_dim\":72,\"feature_milliseconds\":",
        d.stage,Bool(d.schema),Bool(d.strongLiveChain),Bool(d.clientDirect),d.gameInstance,d.clientGameInstance,d.settingsManager,d.settingsOuter,Bool(d.settingsReady),Bool(d.mapKnown),d.mapCount,d.mapCapacity,d.mapFree,d.hashSize,d.mapHops,Bool(d.settingFound),Bool(d.getterCalled),static_cast<unsigned long long>(d.getterCalls),Bool(d.getterAgrees),d.settingValue,d.observedMapValue,Bool(d.selectorKnown),unsigned(d.selector),d.level,d.worldSettings,Bool(d.worldSettingsKnown),d.feature,d.featureOwner,Bool(d.featurePresent),Bool(d.featureKnown),unsigned(d.featureRegistered));
    Number(file,d.featureMilliseconds,d.featureKnown);
    std::fprintf(file,",\"known\":%s,\"settings_writes\":0,\"feature_toggle_writes\":0,\"original_delay_getter_calls\":0}",Bool(d.known));
}
inline void WriteRegisteredMovements(FILE* file,const Registry& r){
    std::fprintf(file,"{\"scope\":\"existing_native_remote_movement_tset\",\"schema\":%s,\"shape_known\":%s,\"complete\":%s,\"lifecycle_metadata\":%s,\"slots\":%d,\"capacity\":%d,\"free\":%d,\"hash_size\":%d,\"allocated\":%u,\"current_role1\":%u,\"noncurrent_role1\":%u,\"non_role1\":%u,\"unknown_owner_identity\":%u,\"invalid\":%u,\"known_dead_noncurrent\":%u,\"queued_noncurrent\":%u,\"sampled_noncurrent_or_unknown\":%u,\"truncated\":%u,\"world_actor_scans\":0,\"weak_serial_allocations\":0,\"set_writes\":0,\"clock_writes\":0,\"queue_writes\":0,\"role_writes\":0,\"exact_native_admission_receipts\":false,\"rows\":[",
        Bool(r.schema),Bool(r.shape),Bool(r.complete),Bool(r.lifecycleMetadata),r.slots,r.capacity,r.free,r.hashSize,r.allocated,r.currentRole1,r.nonCurrentRole1,r.nonRole1,r.unknownIdentity,r.invalid,r.knownDeadNonCurrent,r.queuedNonCurrent,r.sampled,r.truncated);
    for(uint32_t n=0;n<r.sampled&&n<r.rows.size();++n){const auto& q=r.rows[n];
        std::fprintf(file,"%s{\"movement\":\"%p\",\"movement_weak_known\":false,\"pawn\":\"%p\",\"player_state\":\"%p\",\"ps_current_pawn\":\"%p\",\"pawn_weak_known\":%s,\"pawn_weak\":[%d,%d],\"state_weak_known\":%s,\"state_weak\":[%d,%d],\"links\":%s,\"world_known\":%s,\"current_known\":%s,\"is_ps_current_pawn\":%s,\"was_owned_client_life\":%s,\"last_current_at_ms\":%llu,\"role\":%u,\"alive_known\":%s,\"alive\":%s,\"destroying_known\":%s,\"being_destroyed\":%s,\"life_span_known\":%s,\"life_span\":",
            n?",":"",q.movement,q.pawn,q.playerState,q.currentPawn,Bool(q.pawnWeakKnown),q.pawnWeak.index,q.pawnWeak.serial,Bool(q.stateWeakKnown),q.stateWeak.index,q.stateWeak.serial,Bool(q.links),Bool(q.worldKnown),Bool(q.currentKnown),Bool(q.current),Bool(q.wasOwn),static_cast<unsigned long long>(q.lastCurrentAtMs),unsigned(q.role),Bool(q.aliveKnown),Bool(q.alive),Bool(q.destroyingKnown),Bool(q.destroying),Bool(q.lifeSpanKnown));
        Number(file,q.lifeSpan,q.lifeSpanKnown);
        std::fprintf(file,",\"registered_flag15e0_known\":%s,\"registered_flag15e0\":%u,\"manager_reference_known\":%s,\"is_manager_reference_movement180\":%s",Bool(q.registeredFlagKnown),unsigned(q.registeredFlag15e0),Bool(r.referenceKnown),Bool(q.referenceMovement));
        std::fprintf(file,",\"native_respawn_known\":%s,\"native_respawn\":%d,\"stream\":\"%p\",\"stream_known\":%s,\"stream_processing_e0\":%u,\"clock\":{\"object\":\"%p\",\"schema\":%s,\"fields_known\":%s,\"raw_index\":%u,\"step\":%u,\"offset24\":%d,\"server_domain_index\":%u,\"initialized4d8\":%u,\"history_count4d0\":%d,\"pending_moves96c\":%d},\"incoming_queue\":{\"shape_known\":%s,\"entries_known\":%s,\"count\":%d,\"capacity\":%d,\"oldest\":",
            Bool(q.respawnKnown),q.respawnNumber,q.stream,Bool(q.streamKnown),unsigned(q.streamProcessing),q.clock.object,Bool(q.clock.schema),Bool(q.clock.fieldsKnown),q.clock.rawIndex,q.clock.step,q.clock.indexOffset,q.clock.serverDomainIndex,unsigned(q.clock.initialized4d8),q.clock.historyCount,q.clock.pendingMoves,Bool(q.queue.shapeKnown),Bool(q.queue.entriesKnown),q.queue.count,q.queue.capacity);
        WriteMove(file,q.queue.oldest);std::fputs(",\"newest\":",file);WriteMove(file,q.queue.newest);
        std::fprintf(file,"},\"delay_known\":%s,\"delay_steps\":%d,\"sampled_diff_known\":%s,\"oldest_diff\":%d,\"newest_diff\":%d,\"sampled_diff_is_native_call_receipt\":false}",Bool(q.delay.valueKnown),q.delay.value,Bool(q.diffKnown),q.oldestDiff,q.newestDiff);
    }
    std::fprintf(file,"],\"reference_movement180_known\":%s,\"reference_movement180\":\"%p\"}",Bool(r.referenceKnown),r.referenceMovement180);
}

inline void Write(FILE* file){
    if(!file)return;const auto s=GetSnapshot();
    std::fprintf(file,"{\"scope\":\"passive_native_remote_movement_time_domains\",\"schema_version\":2,\"stage\":\"%s\",\"error\":0,\"samples\":%llu,\"sampled_at_ms\":%llu,\"minimum_interval_ms\":100,\"maximum_remote\":30,\"read_exceptions\":%llu,\"profiles_verified\":%s,\"profile_failure_rva\":%u,\"schema_verified\":%s,\"own_context\":%s,\"roster_valid\":%s,\"roster_count\":%d,\"sampled\":%u,\"invalid\":%u,\"skipped_own\":%u,\"truncated\":%u,\"world\":\"%p\",\"controller\":\"%p\",\"own_pawn\":\"%p\",\"game_state\":\"%p\",\"connection\":\"%p\",\"world_time740\":",
        s.stage,static_cast<unsigned long long>(s.samples),static_cast<unsigned long long>(s.atMs),static_cast<unsigned long long>(s.readExceptions),Bool(s.profiles),s.profileFailureRva,Bool(s.schema),Bool(s.ownContext),Bool(s.rosterValid),s.rosterCount,s.sampled,s.invalid,s.skippedOwn,s.truncated,s.world,s.controller,s.ownPawn,s.gameState,s.connection);
    Number(file,s.worldTime,s.worldTimeKnown);
    std::fprintf(file,",\"pc_manager\":\"%p\",\"gs_manager\":\"%p\",\"managers_same\":%s,\"weak_source_render_cache_at_ms\":%llu,\"weak_serial_allocation_calls\":0,\"net_guid_known\":false,\"net_guid_creation_calls\":0,\"receiver_replay_calls\":0,\"clock_writes\":0,\"role_writes\":0,\"queue_writes\":0,\"exact_insert_receipts_known\":false,\"host_epoch_known\":false,\"inverse_mapping_source\":\"original_789edf0_arithmetic_on_cached_fields_no_native_call\",\"raw_miss_counter_is_packet_drop_count\":false,\"shared_manager\":",s.pcManager,s.gsManager,Bool(s.managersSame),static_cast<unsigned long long>(s.renderCacheAtMs));
    WriteManager(file,s.manager);std::fputs(",\"normal_delay_context\":",file);WriteDelayContext(file,s.normalDelay);std::fputs(",\"registered_movements\":",file);WriteRegisteredMovements(file,s.registry);std::fputs(",\"characters\":[",file);
    for(uint32_t n=0;n<s.sampled&&n<MaxRemote;++n){const auto& r=s.rows[n];if(n)std::fputc(',',file);
        std::fputs("{\"actor\":",file);WriteLabel(file,r.actor);std::fputs(",\"player_state\":",file);WriteLabel(file,r.playerState);
        std::fprintf(file,",\"role\":%u,\"links\":%s,\"pawn_weak_known\":%s,\"pawn_weak\":[%d,%d],\"state_weak_known\":%s,\"state_weak\":[%d,%d],\"player_id_known\":%s,\"player_id\":%d,\"movement\":\"%p\",\"movement_owner\":\"%p\",\"movement_character\":\"%p\",\"movement_world_source_known\":%s,\"manager_matches_shared\":%s,\"manager\":",
            unsigned(r.role),Bool(r.links),Bool(r.pawnWeakKnown),r.pawnWeak.index,r.pawnWeak.serial,Bool(r.stateWeakKnown),r.stateWeak.index,r.stateWeak.serial,Bool(r.playerIdKnown),r.playerId,r.movement,r.movementOwner,r.movementCharacter,Bool(r.movementWorldKnown),Bool(r.managerMatchesShared));
        WriteManager(file,r.manager);const auto& c=r.clock;
        std::fprintf(file,",\"clock\":{\"object\":\"%p\",\"table\":\"%p\",\"vbtable\":\"%p\",\"schema\":%s,\"fields_known\":%s,\"base_offset\":%d,\"raw_index\":%u,\"step\":%u,\"offset24\":%d,\"inverse_mapping_known\":%s,\"server_domain_index\":%u,\"accumulated20\":",
            c.object,c.table,c.vbtable,Bool(c.schema),Bool(c.fieldsKnown),c.baseOffset,c.rawIndex,c.step,c.indexOffset,Bool(c.inverseKnown),c.serverDomainIndex);
        Number(file,c.accumulated,c.fieldsKnown);
        std::fprintf(file,",\"raw28\":%u,\"initialized4d8\":%u,\"history_next4c8\":%d,\"history_capacity4cc\":%d,\"history_count4d0\":%d,\"last_queued_mapped_index4dc\":%u,\"last_queued_inverse_known\":%s,\"last_queued_server_domain_index\":%u,\"native_frame_ordinal968\":%d,\"pending_moves96c\":%d,\"lead9a0\":%d,\"reference\":",
            unsigned(c.raw28),unsigned(c.initialized4d8),c.historyNext,c.historyCapacity,c.historyCount,c.lastQueuedMappedIndex,Bool(c.lastQueuedInverseKnown),c.lastQueuedServerDomainIndex,c.frameOrdinal,c.pendingMoves,c.lead9a0);
        WriteMove(file,c.reference);const auto& q=r.queue;
        std::fprintf(file,"},\"incoming_queue\":{\"shape_known\":%s,\"entries_known\":%s,\"data1488\":\"%p\",\"next_write1498\":%d,\"capacity149c\":%d,\"queued_count14a0\":%d,\"oldest\":",Bool(q.shapeKnown),Bool(q.entriesKnown),q.data,q.nextWrite,q.capacity,q.count);
        WriteMove(file,q.oldest);std::fputs(",\"newest\":",file);WriteMove(file,q.newest);const auto& d=r.delay;
        std::fprintf(file,"},\"delay\":{\"fields_known\":%s,\"configured1580\":",Bool(d.fieldsKnown));Number(file,d.configured1580,d.fieldsKnown);
        std::fputs(",\"previous1584\":",file);Number(file,d.previous1584,d.fieldsKnown);
        std::fprintf(file,",\"movement_world_cache_d0\":\"%p\",\"world_source_known\":%s,\"world_f8\":\"%p\",\"world_f8_is_demo_net_driver\":true,\"connection_c0\":\"%p\",\"raw_cee\":%u,\"state13c\":%d,\"chain_known\":%s,\"zero_branch_known\":%s,\"normal_fallback\":%s,\"normal_context_known\":%s,\"used_feature_override\":%s,\"effective_milliseconds\":",
            d.movementWorldCache,Bool(d.worldSourceKnown),d.worldF8,d.connectionC0,unsigned(d.rawCEE),d.state13c,Bool(d.chainKnown),Bool(d.zeroBranchKnown),Bool(d.normalFallback),Bool(d.normalContextKnown),Bool(d.usedFeatureOverride));
        Number(file,d.effectiveMilliseconds,d.normalContextKnown);
        std::fprintf(file,",\"conversion_known\":%s,\"conversion_mxcsr\":%u,\"incoming_fp_context_preserved\":true,\"value_known\":%s,\"value\":%d},\"native_counters_known\":%s,\"native_serialized_moves_received14d0\":%u,\"raw_insert_misses15cc\":%u,\"raw_consecutive1588\":%u,\"raw_overqueue158c\":%u,\"sampled_diff_known\":%s,\"sampled_oldest_diff\":%d,\"sampled_newest_diff\":%d,\"sampled_diff_is_original_call_receipt\":false}",
            Bool(d.conversionKnown),d.conversionMxcsr,Bool(d.valueKnown),d.value,Bool(r.nativeCountersKnown),r.rawSerialized14d0,r.rawMisses15cc,r.rawConsecutive1588,r.rawOverqueue158c,Bool(r.diffKnown),r.oldestDiff,r.newestDiff);
    }
    std::fputs("]}",file);
}
} // namespace local_server_remote_timeline_probe
