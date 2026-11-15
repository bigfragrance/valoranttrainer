#pragma once
#include "local_server_match.h"
#include "local_server_recovery.h"
#include "local_server_kill_ammo.h"
#include "original_effects.h"
#include "original_jett_priming.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

// Original ordinary-DM HealthPack actor, sphere, Enable/Disable graphs and FX.
// Only the stripped authority drop/overlap policy is supplied here. References:
// deathmatch_pickups.h and work/deathmatch-gameplay-audit. No synthetic overlap,
// client-authoritative healing, replacement mesh/material or actor world scan.
namespace local_server_pickups {
using Vec=engine::Vec;
using Weak=original_inventory_slots::WeakIdentity;
inline constexpr size_t InitialPool=20,Capacity=128,PendingCapacity=256;
inline constexpr double Lifetime=8.,DropOffset=25.;
inline constexpr const wchar_t* PackClassPath=L"/Game/GameModes/Deathmatch/GameObject_HealthPack.GameObject_HealthPack_C";
inline constexpr const wchar_t* EnablePath=L"/Game/GameModes/Deathmatch/GameObject_HealthPack.GameObject_HealthPack_C.Multicast Enable";
inline constexpr const wchar_t* DisablePath=L"/Game/GameModes/Deathmatch/GameObject_HealthPack.GameObject_HealthPack_C.Multicast Disable";
inline constexpr const wchar_t* EnabledNotify=L"/Game/GameModes/Deathmatch/PoolingGameObject.PoolingGameObject_C.OnRep_IsEnabled";
struct Array {void** data=nullptr;int32_t count=0,capacity=0;};
struct Pack {
    void *actor=nullptr,*sphere=nullptr,*manager=nullptr;Weak identity{},managerId{};bool active=false;
    uint64_t generation=0;size_t deathPlayer=0;uint32_t deathEpoch=0;
    float radius=0;double expires=0;Vec location{};int32_t lastPicker=-1;
};
struct Drop {
    bool used=false;size_t index=0;uint32_t epoch=0;void *pawn=nullptr,*ps=nullptr;
    Weak pawnId{},stateId{};Vec location{};double observedAt=0;
};
struct Receipt {
    size_t picker=0;uint32_t epoch=0;uint64_t generation=0;
    float hpBefore=0,hpAfter=0,shieldBefore=0,shieldAfter=0;
    bool healthReadback=false,clientDeliveryClaimed=false;
};
struct State {
    bool initialized=false,stopped=false,ready=false,busy=false;
    DWORD gameThread=0;unsigned error=0;const char* stage="server_pickups_not_initialized";
    void *world=nullptr,*driver=nullptr,*gameState=nullptr,*packClass=nullptr,*pickupFx=nullptr,*hudBuff=nullptr;
    Weak worldId{},classId{};int enabledOffset=-1,sphereOffset=-1;
    size_t created=0,active=0,pending=0;uint64_t nextGeneration=0,lastCache=0;
    uint64_t deathEvents=0,duplicates=0,drops=0,pickups=0,expired=0,queueExpired=0,poolDeferrals=0;
    uint64_t overlaps=0,overlapActors=0,recoveryDeferred=0,recoveryRejected=0,enableCalls=0,disableCalls=0;
    uint64_t enabledPublications=0,fxCalls=0,buffCalls=0,healApplied=0,alreadyFull=0;
    uint64_t clientSampleAt=0;uint32_t clientActors=0,clientEnabled=0;unsigned clientError=0;
    double worldTime=0,nextOverlap=0;
};
struct PublicationName {uint32_t words[3]{};};
struct PublicationParams {void* object=nullptr;int32_t repIndex=0;PublicationName propertyName{};};
static_assert(sizeof(PublicationName)==12&&sizeof(PublicationParams)==24&&offsetof(PublicationParams,propertyName)==12);
struct PublicationFunctionSample {
    void *function=nullptr,*owner=nullptr,*child=nullptr;uintptr_t execRva=0;
    uint32_t flags=0;uint16_t bytes=0,returnOffset=0;uint8_t parameters=0;int32_t propertiesBytes=0;
};
struct PublicationFieldSample {
    void *field=nullptr,*fieldClass=nullptr,*reference=nullptr;uint64_t actualName=0,expectedName=0,cast=0,flags=0,requiredCast=0;
    int32_t offset=-1,bytes=0,dimension=0,ownerBytes=0;uint8_t boolSize=0,boolOffset=0,boolByteMask=0,boolFieldMask=0;
};
struct PublicationRepSample {
    void *type=nullptr,*data=nullptr,*indexedField=nullptr;uint32_t flags=0;uint16_t propertyIndex=0;
    int32_t count=0,capacity=0,indexedArray=-1,matches=0;bool readable=false;
};
struct Publication {
    bool profiles=false,bound=false,setupAttempted=false,setupInFlight=false,publishing=false;
    int32_t netMode=-1,baseReps=0,derivedReps=0;uint16_t repIndex=0;uint32_t flagsBefore=0,flagsAfter=0;
    void *actorType=nullptr,*objectType=nullptr,*helperType=nullptr,*poolType=nullptr,*helper=nullptr;
    void *flush=nullptr,*force=nullptr,*dirty=nullptr,*notify=nullptr,*enabledField=nullptr;
    PublicationName propertyName{};std::array<void*,9> objects{};std::array<Weak,9> identities{};
    uint64_t setupCalls=0,flushCalls=0,dirtyStubCalls=0,notifyCalls=0,forceCalls=0;
    const char* diagnosticStage="not_started";int32_t diagnosticField=-1;
    uint64_t propertyNameObserved=0,propertyNameExpected=0;
    std::array<PublicationFunctionSample,4> functionSamples{};
    std::array<PublicationFieldSample,5> fieldSamples{};
    std::array<PublicationRepSample,2> repSamples{};
};
inline Publication publication;
inline State state;
inline std::array<Pack,Capacity> pool{};
inline std::array<Drop,PendingCapacity> pending{};
inline std::array<uint32_t,original_players::MaxPlayers> observedDeaths{};
inline std::array<Receipt,16> receipts{};
inline uint64_t receiptCount=0;
inline SRWLOCK snapshotLock=SRWLOCK_INIT;
inline std::string snapshotJson="{\"stage\":\"server_pickups_not_sampled\",\"runtime_verified\":false}";
inline void Cache();
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void* object,const Weak& id){return object&&original_inventory_slots::ResolveIdentity(id)==object;}
inline bool Capture(void* object,Weak& id){return object&&original_inventory_slots::CaptureIdentity(object,id)&&Live(object,id);}
inline bool Thread(){return original_players::Thread()&&(!state.gameThread||state.gameThread==GetCurrentThreadId());}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;state.ready=false;Cache();return false;}
inline const State& GetState(){return state;}
inline bool Event(void* object,const wchar_t* path,uint16_t bytes,void* params){
    auto fn=engine::Find(path);auto table=Read<void**>(object);auto process=Read<void*>(table,0x298);
    if(!object||!fn||Read<uint16_t>(fn,0xbe)!=bytes||!process)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,fn,params);return true;
}
inline void* Load(uintptr_t base,const wchar_t* path){return engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t,void*)>(0x1d0c6e0)(engine::Class(base),nullptr,path,nullptr,0,nullptr);}
inline int Field(void* type,const wchar_t* name,int bytes,original_jett_priming::PropertyInfo* observation=nullptr){
    original_jett_priming::PropertyInfo local{};auto& p=observation?*observation:local;
    const int offset=original_jett_priming::FieldOffset(type,name,static_cast<size_t>(bytes),&p);
    return offset>=0&&p.elementSize==bytes&&p.arrayDim==1?offset:-1;
}
inline bool Profile(const wchar_t* path,uint16_t bytes,int scriptBytes,uint8_t first){
    auto fn=engine::Find(path);auto script=Read<uint8_t*>(fn,0x68);
    return fn&&Read<uint16_t>(fn,0xbe)==bytes&&Read<int32_t>(fn,0x70)==scriptBytes&&script
        &&Read<uint8_t>(script)==first&&Read<uint8_t>(script,static_cast<size_t>(scriptBytes-1))==0x53;
}
inline bool PublicationProfiles(){
    if(publication.profiles)return true;
    struct Native {uintptr_t rva;size_t bytes;uint64_t fnv;};
    constexpr Native originals[]{
        {0x3ee97b0,26,0x34a79d2e6041161fULL}, // original Actor.FlushNetDormancy exec
        {0x3ee8bf0,38,0x3b40f91deafc7c0eULL}, // original Actor.ForceNetUpdate exec
        {0x47601e0,513,0x1c609c74f38774daULL}, // original parser-only dirty wrapper
        {0x1b979c0,1033,0x16a648ec490bf709ULL}, // original UClass runtime replication setup
        {0x3efc820,176,0x7f1113b3f79164b2ULL}, // actual Actor.GetNetMode
        {0x1c75610,158,0x53040a68dd75221cULL}, // FObjectPropertyBase getter, as used by the VM parser
        {0x1c9d710,158,0x04b0af17596ede1aULL}, // FIntProperty class getter
        {0x1c9c580,158,0x9c5248fa4f440bb0ULL}, // FNameProperty class getter
    };
    if(!engine::image)return false;
    for(const auto& p:originals){uint64_t hash=14695981039346656037ULL;
        const auto* bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        for(size_t n=0;n<p.bytes;++n)hash=(hash^bytes[n])*1099511628211ULL;
        if(hash!=p.fnv)return false;
    }
    publication.profiles=true;return true;
}
inline bool PublicationObjectsLive(){
    for(size_t n=0;n<publication.objects.size();++n)
        if(!Live(publication.objects[n],publication.identities[n]))return false;
    return Live(state.packClass,state.classId);
}
inline void ObservePublicationFunction(size_t index,void* fn){
    auto& s=publication.functionSamples[index];s={};s.function=fn;
    s.owner=Read<void*>(fn,0x28);s.child=Read<void*>(fn,0x58);s.flags=Read<uint32_t>(fn,0xb8);
    s.parameters=Read<uint8_t>(fn,0xbc);s.bytes=Read<uint16_t>(fn,0xbe);s.returnOffset=Read<uint16_t>(fn,0xc0);
    s.propertiesBytes=Read<int32_t>(fn,0x60);const auto exec=Read<uintptr_t>(fn,0xe0);
    s.execRva=exec>=engine::image?exec-engine::image:exec;
}
inline void ObservePublicationField(size_t index,const original_jett_priming::PropertyInfo& info,uint64_t required=0){
    auto& s=publication.fieldSamples[index];s={};s.field=info.field;s.fieldClass=info.fieldClass;
    s.reference=Read<void*>(info.field,0x78);s.actualName=info.actualName;s.expectedName=info.expectedName;
    s.cast=info.castFlags;s.flags=Read<uint64_t>(info.field,0x38);s.requiredCast=required;
    s.offset=info.offset;s.bytes=info.elementSize;s.dimension=info.arrayDim;s.ownerBytes=info.propertiesSize;
    s.boolSize=info.boolFieldSize;s.boolOffset=info.boolByteOffset;s.boolByteMask=info.boolByteMask;s.boolFieldMask=info.boolFieldMask;
}
inline bool PublicationFunction(void* fn,void* owner,uint32_t flags,uint16_t bytes,uint8_t count,uintptr_t exec){
    publication.diagnosticStage=fn==publication.flush?"uf_flush":fn==publication.force?"uf_force":fn==publication.dirty?"uf_dirty":"uf_notify";
    return fn&&Read<void*>(fn,0x28)==owner&&Read<uint32_t>(fn,0xb8)==flags
        &&Read<uint16_t>(fn,0xbe)==bytes&&Read<uint8_t>(fn,0xbc)==count
        &&Read<uintptr_t>(fn,0xe0)==engine::image+exec;
}
inline bool PublicationField(void* fn,const wchar_t* name,int offset,int bytes,void* fieldClass,void*& field){
    original_jett_priming::PropertyInfo info{};
    const auto required=Read<uint64_t>(fieldClass,0x18);
    // The original parser asks for FObjectPropertyBase, not equality with its
    // derived FObjectProperty/FObjectPtrProperty class. Preserve that contract.
    const int actualOffset=required?Field(fn,name,bytes,&info):-1;
    const size_t sample=offset==0?0:offset==8?1:2;publication.diagnosticField=static_cast<int32_t>(sample);
    ObservePublicationField(sample,info,required);
    if(!required||actualOffset!=offset||(info.castFlags&required)!=required
       ||(Read<uint64_t>(info.field,0x38)&0x580ULL)!=0x80ULL)return false;
    field=info.field;return true;
}
inline bool PublicationRecords(void* type,int32_t& count){
    const auto a=Read<Array>(type,0x100);count=a.count;
    const uint16_t actual=Read<uint16_t>(publication.enabledField,0x40);
    const size_t sample=type==publication.poolType?1:0;auto& observed=publication.repSamples[sample];observed={};
    publication.diagnosticStage=sample?"class_reps_base":"class_reps_derived";
    observed.type=type;observed.data=a.data;observed.count=a.count;observed.capacity=a.capacity;
    observed.flags=Read<uint32_t>(type,0xdc);observed.propertyIndex=actual;
    observed.readable=a.count>0&&a.count<=16384&&original_players::Readable(a.data,static_cast<size_t>(a.count)*16);
    if(observed.readable&&actual<a.count){observed.indexedField=Read<void*>(a.data,static_cast<size_t>(actual)*16);
        observed.indexedArray=Read<int32_t>(a.data,static_cast<size_t>(actual)*16+8);}
    if(!(Read<uint32_t>(type,0xdc)&0x800U)||actual!=10||actual>=a.count
       ||a.count<=0||a.count>16384||a.capacity<a.count||a.capacity>16384
       ||!original_players::Readable(a.data,static_cast<size_t>(a.count)*16))return false;
    int matches=0;
    for(int32_t n=0;n<a.count;++n)if(Read<void*>(a.data,static_cast<size_t>(n)*16)==publication.enabledField){
        if(n!=actual||Read<int32_t>(a.data,static_cast<size_t>(n)*16+8)!=0)return false;++matches;
    }
    observed.matches=matches;publication.repIndex=actual;return matches==1;
}
inline bool PublicationMetadata(){
    auto& p=publication;p.diagnosticStage="profiles";if(!PublicationProfiles())return false;
    p.diagnosticStage="setup_reentry";if(p.setupInFlight)return false;
    if(p.bound)return PublicationObjectsLive()&&PublicationRecords(state.packClass,p.derivedReps)
        &&PublicationRecords(p.poolType,p.baseReps);
    p.diagnosticStage="thread_or_pack_class_weak";if(!Thread()||!Live(state.packClass,state.classId))return false;
    p.diagnosticStage="native_classes";p.actorType=engine::Class(0x3683900);p.objectType=engine::Class(0x1af05b0);p.helperType=engine::Class(0x4760130);
    p.poolType=engine::Find(L"/Game/GameModes/Deathmatch/PoolingGameObject.PoolingGameObject_C");
    if(!p.actorType||!p.objectType||!p.helperType||!p.poolType)return false;
    p.helper=engine::DefaultObject(p.helperType);
    p.flush=engine::Find(L"/Script/Engine.Actor.FlushNetDormancy");
    p.force=engine::Find(L"/Script/Engine.Actor.ForceNetUpdate");
    p.dirty=engine::Find(L"/Script/Engine.NetPushModelHelpers.MarkPropertyDirtyFromRepIndex");
    p.notify=engine::Find(EnabledNotify);
    ObservePublicationFunction(0,p.flush);ObservePublicationFunction(1,p.force);
    ObservePublicationFunction(2,p.dirty);ObservePublicationFunction(3,p.notify);
    p.diagnosticStage="expected_helper_class_and_derivation";
    if(!p.actorType||!p.objectType||!p.helperType||!p.poolType||!p.helper
       ||Read<void*>(p.helper,0x10)!=p.helperType||!original_players::Derived(state.packClass,p.poolType)
       ||!PublicationFunction(p.flush,p.actorType,0x04020407U,0,0,0x3ee97b0)
       ||!PublicationFunction(p.force,p.actorType,0x04020402U,0,0,0x3ee8bf0)
       ||!PublicationFunction(p.dirty,p.helperType,0x04022403U,24,3,0x47601e0)
       ||!PublicationFunction(p.notify,p.poolType,0x0c000000U,0,0,0x1cdb7c0))return false;
    p.diagnosticStage="zero_parameter_child_properties";
    if(Read<void*>(p.flush,0x58)||Read<void*>(p.force,0x58)||Read<void*>(p.notify,0x58))return false;
    p.diagnosticStage="dirty_properties_size";if(Read<int32_t>(p.dirty,0x60)!=24)return false;
    auto objectClass=engine::function<void*(*)()>(0x1c75610)();
    auto intClass=engine::function<void*(*)()>(0x1c9d710)();
    auto nameClass=engine::function<void*(*)()>(0x1c9c580)();
    p.diagnosticStage="dirty_parameter_fields";std::array<void*,3> fields{};
    if(!PublicationField(p.dirty,L"Object",0,8,objectClass,fields[0])
       ||Read<void*>(fields[0],0x78)!=p.objectType
       ||!PublicationField(p.dirty,L"RepIndex",8,4,intClass,fields[1])
       ||!PublicationField(p.dirty,L"PropertyName",12,12,nameClass,fields[2]))return false;
    p.diagnosticStage="dirty_parameter_chain";auto field=Read<void*>(p.dirty,0x58);uint32_t seen=0;std::array<void*,3> visited{};
    while(field){if(seen>=visited.size())return false;
        for(uint32_t n=0;n<seen;++n)if(visited[n]==field)return false;
        bool expected=false;for(auto expectedField:fields)expected=expected||field==expectedField;
        if(!expected)return false;visited[seen++]=field;field=Read<void*>(field,0x18);
    }
    if(seen!=fields.size())return false;
    p.diagnosticStage="enabled_field_shape";original_jett_priming::PropertyInfo enabled{},baseEnabled{};
    const int derivedOffset=Field(state.packClass,L"IsEnabled",1,&enabled);ObservePublicationField(3,enabled);
    if(derivedOffset!=state.enabledOffset)return false;
    const int baseOffset=Field(p.poolType,L"IsEnabled",1,&baseEnabled);ObservePublicationField(4,baseEnabled);
    if(baseOffset!=state.enabledOffset||baseEnabled.field!=enabled.field
       ||enabled.boolFieldSize!=1||enabled.boolByteOffset||enabled.boolByteMask!=1||enabled.boolFieldMask!=0xff
       // The fixed original loader adds native linked-bool flags to the
       // cooked declaration. Pin the actual linked value, preserving both
       // original Net (20) and RepNotify (100000000) semantics explicitly.
       ||Read<uint64_t>(enabled.field,0x38)!=0x8001140010225ULL
       ||(Read<uint64_t>(enabled.field,0x38)&0x100000020ULL)!=0x100000020ULL)return false;
    p.enabledField=enabled.field;
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&p.propertyName,L"IsEnabled",0);
    p.diagnosticStage="enabled_fname";p.propertyNameObserved=Read<uint64_t>(&p.propertyName);p.propertyNameExpected=Read<uint64_t>(enabled.field,0x20);
    if(Read<uint64_t>(&p.propertyName)!=Read<uint64_t>(enabled.field,0x20))return false;
    p.objects={p.actorType,p.objectType,p.helperType,p.poolType,p.helper,p.flush,p.force,p.dirty,p.notify};
    p.diagnosticStage="metadata_weak_capture";for(size_t n=0;n<p.objects.size();++n)if(!Capture(p.objects[n],p.identities[n]))return false;
    p.diagnosticStage="native_class_setup";p.flagsBefore=Read<uint32_t>(state.packClass,0xdc);
    if(!(p.flagsBefore&0x800U)){
        if(p.setupAttempted||p.setupInFlight)return false;p.setupAttempted=true;p.setupInFlight=true;++p.setupCalls;
        // Same real API invoked by RepLayout49D6695; it owns ClassReps,
        // FProperty.RepIndex and setup flags. No adapter writes those fields.
        __try{engine::function<void(*)(void*)>(0x1b979c0)(state.packClass);}
        __finally{p.setupInFlight=false;}
    }
    p.flagsAfter=Read<uint32_t>(state.packClass,0xdc);
    p.diagnosticStage="post_setup_weak_and_records";if(!PublicationObjectsLive()||!PublicationRecords(state.packClass,p.derivedReps)
       ||!PublicationRecords(p.poolType,p.baseReps))return false;
    p.bound=true;p.diagnosticStage="metadata_verified";return true;
}
inline bool PublicationCall(void* object,void* fn,void* parameters){
    if(!object||!fn||!PublicationObjectsLive())return false;
    auto process=Read<void*>(Read<void**>(object),0x298);
    if(!process)return false;reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,fn,parameters);
    return PublicationObjectsLive();
}

inline void PublicationDiagnosticJson(local_server_match::Json& out){
    const auto& p=publication;out.Add("{\"stage\":\"%s\",\"last_parameter\":%d,\"name_observed\":%llu,\"name_expected\":%llu,\"functions\":[",
        p.diagnosticStage,p.diagnosticField,p.propertyNameObserved,p.propertyNameExpected);
    constexpr const char* methods[]{"flush","force","dirty","notify"};
    for(size_t n=0;n<p.functionSamples.size();++n){const auto& s=p.functionSamples[n];
        out.Add("%s{\"role\":\"%s\",\"function\":\"%p\",\"owner\":\"%p\",\"child\":\"%p\",\"exec_rva\":%llu,\"flags\":%u,\"bytes\":%u,\"num_parms\":%u,\"return_offset\":%u,\"properties_bytes\":%d}",n?",":"",methods[n],s.function,s.owner,s.child,
            static_cast<unsigned long long>(s.execRva),s.flags,unsigned(s.bytes),unsigned(s.parameters),unsigned(s.returnOffset),s.propertiesBytes);
    }
    out.Add("],\"fields\":[");
    for(size_t n=0;n<p.fieldSamples.size();++n){const auto& s=p.fieldSamples[n];
        out.Add("%s{\"slot\":%zu,\"field\":\"%p\",\"field_class\":\"%p\",\"reference\":\"%p\",\"actual_name\":%llu,\"expected_name\":%llu,\"cast\":%llu,\"required_cast\":%llu,\"flags\":%llu,\"offset\":%d,\"bytes\":%d,\"dimension\":%d,\"owner_bytes\":%d,\"bool\":[%u,%u,%u,%u]}",n?",":"",n,s.field,s.fieldClass,s.reference,
            s.actualName,s.expectedName,s.cast,s.requiredCast,s.flags,s.offset,s.bytes,s.dimension,s.ownerBytes,unsigned(s.boolSize),unsigned(s.boolOffset),unsigned(s.boolByteMask),unsigned(s.boolFieldMask));
    }
    out.Add("],\"class_reps\":[");
    for(size_t n=0;n<p.repSamples.size();++n){const auto& s=p.repSamples[n];
        out.Add("%s{\"base\":%s,\"type\":\"%p\",\"data\":\"%p\",\"indexed_field\":\"%p\",\"flags\":%u,\"property_index\":%u,\"count\":%d,\"capacity\":%d,\"indexed_array\":%d,\"matches\":%d,\"readable\":%s}",n?",":"",n?"true":"false",s.type,s.data,s.indexedField,s.flags,unsigned(s.propertyIndex),s.count,s.capacity,s.indexedArray,s.matches,s.readable?"true":"false");
    }out.Add("]}");
}

inline bool Context(){
    return Thread()&&state.initialized&&Live(state.world,state.worldId)
        &&state.world==original_players::world&&local_server_match::GetState().world==state.world
        &&Read<void*>(state.world,0x178)==state.gameState&&Read<uint8_t>(state.gameState,0x224)==3
        &&!Read<void*>(state.driver,0xc0)&&local_server_match::FreeForAll()
        &&publication.profiles&&engine::function<int(*)(void*)>(0x3efc820)(state.gameState)==2;
}
inline bool PublishEnabledImpl(Pack& pack,bool enabled){
    if(!Context()||!Live(pack.actor,pack.identity)||!PublicationMetadata()
       ||Read<void*>(pack.actor,0x10)!=state.packClass||Read<uint8_t>(pack.actor,0x224)!=3)return false;
    const auto old=Read<uint8_t>(pack.actor,static_cast<size_t>(state.enabledOffset));
    if(old>1)return false;const uint8_t desired=enabled?1:0;
    // Original SetEnabled VM35..107: wake, authority bool, original dirty
    // wrapper, RepNotify, ForceNetUpdate. Its dedicated predicate is unchanged.
    if(!PublicationCall(pack.actor,publication.flush,nullptr)||!Live(pack.actor,pack.identity))return false;
    ++publication.flushCalls;
    if(!original_players::Write(pack.actor,static_cast<size_t>(state.enabledOffset),desired))return false;
    PublicationParams parameters;parameters.object=pack.actor;parameters.repIndex=publication.repIndex;
    parameters.propertyName=publication.propertyName;
    // This fixed client retains a parser-only dirty wrapper. Preserve the
    // original sequence, but never claim it dirtied PushModel or delivered data.
    if(!PublicationCall(publication.helper,publication.dirty,&parameters)||!Live(pack.actor,pack.identity))return false;
    ++publication.dirtyStubCalls;
    if(!PublicationCall(pack.actor,publication.notify,nullptr)||!Live(pack.actor,pack.identity))return false;
    ++publication.notifyCalls;
    if(!PublicationCall(pack.actor,publication.force,nullptr)||!Live(pack.actor,pack.identity)
       ||Read<uint8_t>(pack.actor,static_cast<size_t>(state.enabledOffset))!=desired)return false;
    ++publication.forceCalls;++state.enabledPublications;return true;
}
inline bool PublishEnabled(Pack& pack,bool enabled){
    if(publication.publishing)return Fail(7922,"server_pickup_enabled_publication_reentered");
    bool result=false;publication.publishing=true;
    __try{result=PublishEnabledImpl(pack,enabled);}
    __finally{publication.publishing=false;}
    return result;
}
inline bool Disable(Pack& pack,bool expired){
    if(!PublishEnabled(pack,false)||!Event(pack.actor,DisablePath,0,nullptr))return false;
    ++state.disableCalls;if(expired)++state.expired;pack.active=false;return true;
}
inline bool EffectsReady(Pack& pack){
    // EnsureActor runs once at creation. Ordinary frames inspect this exact
    // native component rather than rescanning all tracked FX managers per pack.
    return Live(pack.manager,pack.managerId)&&Read<void*>(pack.manager,0xc8)==pack.actor
        &&(Read<uint8_t>(pack.manager,0xa0)&1)&&(Read<uint8_t>(pack.manager,0xc2)&0x40)
        &&Read<uint8_t>(pack.manager,0xe0)==0;
}
inline bool CreatePack(){
    if(state.created>=pool.size())return false;engine::Transform transform;
    auto actor=engine::function<void*(*)(void*,void*,const engine::Transform*,uint8_t,void*,uint8_t)>(0x43fada0)
        (state.world,state.packClass,&transform,1,nullptr,0);
    if(!actor)return Fail(7901,"server_pickup_original_factory_failed");
    uint8_t yes=1;
    if(!Event(actor,L"/Script/Engine.Actor.SetReplicates",1,&yes)
       ||!Event(actor,L"/Script/Engine.Actor.SetReplicateMovement",1,&yes))return Fail(7902,"server_pickup_native_replication_enable_failed");
    engine::function<void(*)(void*,const engine::Transform*,bool,void*,uint8_t)>(0x3ef6cb0)(actor,&transform,false,nullptr,0);
    auto& p=pool[state.created];p={};p.actor=actor;p.sphere=Read<void*>(actor,static_cast<size_t>(state.sphereOffset));
    if(Read<void*>(actor,0x10)!=state.packClass||Read<uint8_t>(actor,0x224)!=3||original_effects::GetWorld(actor)!=state.world
       ||!Capture(actor,p.identity)||!original_players::IsA(p.sphere,0x41c1670)||Read<void*>(p.sphere,0xc8)!=actor
       ||!Event(p.sphere,L"/Script/Engine.SphereComponent.GetUnscaledSphereRadius",4,&p.radius)||p.radius<=0
       ||!original_effects::EnsureActor(actor,"network_deathmatch_health_pack"))
        return Fail(7903,"server_pickup_original_components_or_lifecycle_invalid");
    p.manager=original_effects::GetManager(actor,false);
    if(!Capture(p.manager,p.managerId)||!Disable(p,false))return Fail(7903,"server_pickup_original_effect_manager_missing");
    ++state.created;return true;
}
inline bool Initialize(void* world,void* driver){
    if(state.initialized)return state.world==world&&state.driver==driver;
    if(!Thread()||!world||world!=original_players::world||!driver||Read<void*>(driver,0xc0)
       ||!local_server_match::FreeForAll())return false;
    if(!PublicationProfiles())return Fail(7923,"server_pickup_original_publication_profiles_changed");
    auto gameState=Read<void*>(world,0x178);
    if(!original_players::IsA(gameState,original_players::ShooterGameStateClass)
       ||Read<uint8_t>(gameState,0x224)!=3)return Fail(7923,"server_pickup_original_authority_game_state_missing");
    publication.netMode=engine::function<int(*)(void*)>(0x3efc820)(gameState);
    // The original dedicated-server overlap policy must never run together
    // with this explicit replacement for the missing listen-server policy.
    if(publication.netMode!=2)return Fail(7923,"server_pickup_requires_original_listen_server_mode");
    state.world=world;state.driver=driver;state.gameState=gameState;state.gameThread=GetCurrentThreadId();
    state.packClass=Load(0x6480c50,PackClassPath);
    state.pickupFx=Load(0x648ada0,L"/Game/GameModes/Deathmatch/FXC_HealthPack_Pickup.FXC_HealthPack_Pickup_C");
    state.hudBuff=Load(0x6481590,L"/Game/GameModes/Deathmatch/Buff_HealthPack_HUD.Buff_HealthPack_HUD_C");
    if(!state.packClass||!state.pickupFx||!state.hudBuff||!Capture(world,state.worldId)||!Capture(state.packClass,state.classId))
        return Fail(7904,"server_pickup_original_assets_missing");
    auto cdo=engine::DefaultObject(state.packClass);original_jett_priming::PropertyInfo enabled{};
    state.enabledOffset=Field(state.packClass,L"IsEnabled",1,&enabled);state.sphereOffset=Field(state.packClass,L"Sphere",8);
    const int life=Field(state.packClass,L"Lifespan",8),reload=Field(state.packClass,L"ShouldReload",1);
    const int health=Field(state.packClass,L"ShouldFullyRefreshHealth",1),shield=Field(state.packClass,L"ShouldFullyRefreshArmor",1);
    if(!cdo||life<0||reload<0||health<0||shield<0||state.enabledOffset<0||state.sphereOffset<0
       ||Read<double>(cdo,static_cast<size_t>(life))!=Lifetime||Read<uint8_t>(cdo,static_cast<size_t>(reload))!=1
       ||Read<uint8_t>(cdo,static_cast<size_t>(health))!=1||Read<uint8_t>(cdo,static_cast<size_t>(shield))!=1
       ||enabled.boolFieldSize!=1||enabled.boolByteOffset||enabled.boolByteMask!=1||enabled.boolFieldMask!=0xff
       ||!Profile(EnablePath,112,72,0x64)||!Profile(DisablePath,0,18,0x46)||!Profile(EnabledNotify,0,26,0x45))
        return Fail(7905,"server_pickup_original_defaults_or_graph_metadata_changed");
    if(!PublicationMetadata())return Fail(7924,"server_pickup_original_replication_publication_schema_or_records_unproven");
    state.initialized=true;
    for(size_t n=0;n<InitialPool;++n)if(!CreatePack())return false;
    state.stage="server_pickup_pool_waiting_native_lifecycle";return true;
}
inline bool OnDeath(size_t index,void* pawn,void* ps,uint32_t epoch){
    if(!Context()||!epoch||index>=observedDeaths.size())return false;if(state.stopped)return true;
    if(observedDeaths[index]==epoch){++state.duplicates;return true;}
    const auto p=local_server_match::Player(index);
    if(!p||p->pawn!=pawn||p->ps!=ps||p->epoch!=epoch||p->alive||p->phase!=local_server_match::Phase::DeadWaiting
       ||!Live(pawn,p->pawnIdentity))return Fail(7906,"server_pickup_death_requires_actual_dead_current_life");
    Drop* free=nullptr;for(auto& d:pending)if(!d.used){free=&d;break;}
    if(!free)return Fail(7907,"server_pickup_death_queue_capacity");
    Drop d{};d.used=true;d.index=index;d.epoch=epoch;d.pawn=pawn;d.ps=ps;d.observedAt=Read<double>(state.world,0x740);
    if(!std::isfinite(d.observedAt)||!Capture(pawn,d.pawnId)||!Capture(ps,d.stateId)
       ||!Event(pawn,L"/Script/Engine.Actor.K2_GetActorLocation",24,&d.location)||!original_players::Finite(d.location))
        return Fail(7908,"server_pickup_death_location_or_native_identity_missing");
    d.location.z-=DropOffset;*free=d;observedDeaths[index]=epoch;++state.deathEvents;return true;
}
inline bool Enable(Drop& d){
    if(state.worldTime-d.observedAt>=Lifetime){d.used=false;++state.queueExpired;return true;}
    Pack* next=nullptr;for(size_t n=0;n<state.created;++n)if(!pool[n].active&&Live(pool[n].actor,pool[n].identity)){next=&pool[n];break;}
    if(!next&&state.created<pool.size()){if(!CreatePack())return false;next=&pool[state.created-1];}
    if(!next){++state.poolDeferrals;return true;}
    if(!EffectsReady(*next))return original_effects::State().error==0;
    auto& pack=*next;
    struct alignas(16) EnableParams {engine::Transform transform;void* pawn=nullptr;void* info=nullptr;} args;
    static_assert(sizeof(EnableParams)==112);args.transform.translation=d.location;
    args.pawn=Live(d.pawn,d.pawnId)?d.pawn:nullptr;args.info=Live(d.ps,d.stateId)?Read<void*>(d.ps,0xa18):nullptr;
    if(!PublishEnabled(pack,true)||!Event(pack.actor,EnablePath,112,&args))return Fail(7909,"server_pickup_original_enable_failed");
    Vec actual{};uint8_t yes=1,collision=0,overlap=0;
    if(!Event(pack.actor,L"/Script/Engine.Actor.K2_GetActorLocation",24,&actual)
       ||std::abs(actual.x-d.location.x)>.01||std::abs(actual.y-d.location.y)>.01||std::abs(actual.z-d.location.z)>.01
       ||!Event(pack.sphere,L"/Script/Engine.PrimitiveComponent.SetGenerateOverlapEvents",1,&yes)
       ||!Event(pack.sphere,L"/Script/Engine.PrimitiveComponent.GetGenerateOverlapEvents",1,&overlap)
       ||!Event(pack.sphere,L"/Script/Engine.PrimitiveComponent.GetCollisionEnabled",1,&collision)||!overlap||collision!=1)
        return Fail(7910,"server_pickup_original_position_and_overlap_readback_failed");
    pack.active=true;pack.location=actual;pack.expires=state.worldTime+Lifetime;pack.generation=++state.nextGeneration;
    pack.deathPlayer=d.index;pack.deathEpoch=d.epoch;pack.lastPicker=-1;d.used=false;++state.drops;++state.enableCalls;return true;
}
inline bool Fx(void* pawn){
    alignas(8) uint8_t p[200]{};std::memcpy(p+0x10,&state.pickupFx,8);std::memcpy(p+0x18,&pawn,8);
    std::memcpy(p+0x80,&state.gameState,8);p[0xc5]=3;
    if(!Event(engine::DefaultObject(engine::Class(0x674a040)),L"/Script/ShooterGame.EffectsBlueprintLibrary.PlayOneShotEffect_Blueprint",200,p))return false;
    ++state.fxCalls;return true;
}
inline bool Pickup(Pack& pack,size_t index){
    const auto p=local_server_match::Player(index);if(!p||!p->alive||p->phase!=local_server_match::Phase::Alive||!local_server_match::AuthorityPair(*p))return true;
    const auto pawn=p->pawn,ps=p->ps;const auto epoch=p->epoch;float before=0,beforeShield=0;
    if(!Event(pawn,L"/Script/ShooterGame.ShooterCharacter.GetHealth",4,&before)
       ||!Event(pawn,L"/Script/ShooterGame.ShooterCharacter.GetShield",4,&beforeShield))return false;
    const auto outcome=local_server_recovery::RestoreForPawn(index,pawn,ps,epoch);
    if(outcome==local_server_recovery::Outcome::Deferred){++state.recoveryDeferred;return true;}
    if(outcome==local_server_recovery::Outcome::Rejected){++state.recoveryRejected;return true;}
    if(outcome==local_server_recovery::Outcome::Failed)return Fail(7911,"server_pickup_original_network_recovery_failed");
    // Once consumed, a later native ammo/FX failure cannot apply the heal twice.
    if(!Disable(pack,false))return Fail(7912,"server_pickup_original_disable_failed");
    pack.lastPicker=static_cast<int32_t>(index);++state.pickups;
    if(outcome==local_server_recovery::Outcome::Applied)++state.healApplied;else ++state.alreadyFull;
    auto& receipt=receipts[receiptCount++%receipts.size()];receipt={};receipt.picker=index;receipt.epoch=epoch;
    receipt.generation=pack.generation;receipt.hpBefore=before;receipt.shieldBefore=beforeShield;
    receipt.healthReadback=Event(pawn,L"/Script/ShooterGame.ShooterCharacter.GetHealth",4,&receipt.hpAfter)
        &&Event(pawn,L"/Script/ShooterGame.ShooterCharacter.GetShield",4,&receipt.shieldAfter);
    struct Buff {void* type=nullptr;void* causer=nullptr;void* instigator=nullptr;float level=1;uint8_t dead=0,pad[3]{};uint64_t result=0;} buff;
    static_assert(sizeof(Buff)==40);buff.type=state.hudBuff;
    if(!receipt.healthReadback||!local_server_kill_ammo::RefillForPawn(index,pawn)
       ||!Event(pawn,L"/Script/ShooterGame.ShooterCharacter.CreateAndApplyBuff",40,&buff)||!Fx(pawn))
        return Fail(7913,"server_pickup_original_ammo_or_visual_leaves_failed");
    ++state.buffCalls;return true;
}
inline bool Overlaps(Pack& pack){
    struct Query {Array actors;void* filter=nullptr;} q;q.filter=engine::Class(original_players::ShooterCharacterClass);
    static_assert(sizeof(Query)==24);bool valid=Event(pack.sphere,L"/Script/Engine.PrimitiveComponent.GetOverlappingActors",24,&q);++state.overlaps;
    valid=valid&&q.actors.count>=0&&q.actors.count<=128&&q.actors.capacity>=q.actors.count&&(!q.actors.count||q.actors.data);
    if(valid)for(int32_t n=0;n<q.actors.count&&pack.active;++n){void* actor=q.actors.data[n];++state.overlapActors;
        for(const auto& p:local_server_match::AuthorityPlayers())if(p.present&&p.alive&&p.pawn==actor&&p.phase==local_server_match::Phase::Alive){
            if(!Pickup(pack,p.index))valid=false;break;
        }
        if(!valid)break;
    }
    if(q.actors.data)engine::function<void(*)(void*)>(0x1915720)(q.actors.data);return valid;
}
inline void Cache(){
    local_server_match::Json out;const auto b=[](bool v){return v?"true":"false";};
    out.Add("{\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"ready\":%s,\"stopped\":%s,\"created\":%zu,\"active\":%zu,\"pending\":%zu,\"death_events\":%llu,\"duplicates\":%llu,\"drops\":%llu,\"pickups\":%llu,\"heal_applied\":%llu,\"already_full\":%llu,\"expired\":%llu,\"queue_expired\":%llu,\"overlap_queries\":%llu,\"overlap_actors\":%llu,\"recovery_deferred\":%llu,\"recovery_rejected\":%llu,\"enable_calls\":%llu,\"disable_calls\":%llu,\"client_observed_actors\":%u,\"client_enabled_actors\":%u,\"client_sample_at_ms\":%llu,\"client_error\":%u,\"synthetic_overlaps\":0,\"world_scans\":0,\"runtime_verified\":false,\"packs\":[",
        state.stage,state.error,b(state.initialized),b(state.ready),b(state.stopped),state.created,state.active,state.pending,
        state.deathEvents,state.duplicates,state.drops,state.pickups,state.healApplied,state.alreadyFull,state.expired,state.queueExpired,
        state.overlaps,state.overlapActors,state.recoveryDeferred,state.recoveryRejected,state.enableCalls,state.disableCalls,
        state.clientActors,state.clientEnabled,static_cast<unsigned long long>(state.clientSampleAt),state.clientError);
    for(size_t n=0;n<state.created;++n){const auto& p=pool[n];out.Add("%s{\"slot\":%zu,\"actor\":\"%p\",\"active\":%s,\"generation\":%llu,\"death_player\":%zu,\"death_epoch\":%u,\"last_picker\":%d,\"location\":[%.6f,%.6f,%.6f]}",
        n?",":"",n,p.actor,b(p.active),p.generation,p.deathPlayer,p.deathEpoch,p.lastPicker,p.location.x,p.location.y,p.location.z);}
    out.Add("],\"receipts\":[");const auto count=(std::min)(receiptCount,uint64_t(receipts.size()));
    for(uint64_t n=0;n<count;++n){const auto& r=receipts[static_cast<size_t>((receiptCount-count+n)%receipts.size())];
        out.Add("%s{\"picker\":%zu,\"epoch\":%u,\"generation\":%llu,\"health_before\":%.4f,\"health_after\":%.4f,\"shield_before\":%.4f,\"shield_after\":%.4f,\"native_readback\":%s,\"client_delivery_claimed\":false}",
            n?",":"",r.picker,r.epoch,r.generation,double(r.hpBefore),double(r.hpAfter),double(r.shieldBefore),double(r.shieldAfter),b(r.healthReadback));}
    out.Add("],\"publication\":{\"profiles_verified\":%s,\"metadata_bound\":%s,\"native_net_mode\":%d,\"actual_rep_index\":%u,\"base_class_reps\":%d,\"derived_class_reps\":%d,\"native_class_setup_calls\":%llu,\"flush_calls\":%llu,\"original_dirty_wrapper_calls\":%llu,\"notify_calls\":%llu,\"force_update_calls\":%llu,\"dirty_wrapper_effective_mutation_proved\":false,\"class_flags_or_rep_index_adapter_writes\":0,\"client_delivery_claimed\":false,\"diagnostic\":",
        b(publication.profiles),b(publication.bound),publication.netMode,unsigned(publication.repIndex),publication.baseReps,publication.derivedReps,
        publication.setupCalls,publication.flushCalls,publication.dirtyStubCalls,publication.notifyCalls,publication.forceCalls);
    PublicationDiagnosticJson(out);out.Add("}}");
    AcquireSRWLockExclusive(&snapshotLock);snapshotJson=std::move(out.value);ReleaseSRWLockExclusive(&snapshotLock);
}
inline std::string Json(){AcquireSRWLockShared(&snapshotLock);auto result=snapshotJson;ReleaseSRWLockShared(&snapshotLock);return result;}
inline bool PollImpl(){
    if(!state.initialized)return true;if(!Context())return Fail(7914,"server_pickup_world_or_thread_changed");if(state.error)return false;
    if(state.stopped)return true;
    state.worldTime=Read<double>(state.world,0x740);if(!std::isfinite(state.worldTime))return false;
    if(!local_server_match::Active())return true;
    bool managers=true;for(size_t n=0;n<state.created;++n){auto& p=pool[n];if(!Live(p.actor,p.identity))return Fail(7916,"server_pickup_owned_pool_actor_expired");
        managers=EffectsReady(p)&&managers;
        if(p.active&&state.worldTime>=p.expires){if(!Disable(p,true))return Fail(7917,"server_pickup_expiration_failed");}
    }
    for(auto& d:pending)if(d.used&&!Enable(d))return false;
    const bool query=state.worldTime>=state.nextOverlap;if(query)state.nextOverlap=state.worldTime+.05;
    state.active=state.pending=0;
    for(size_t n=0;n<state.created;++n){auto& p=pool[n];if(p.active&&query&&!Overlaps(p))return Fail(7918,"server_pickup_native_overlap_failed");if(p.active)++state.active;}
    for(const auto& d:pending)if(d.used)++state.pending;
    state.ready=state.created>=InitialPool&&managers;state.stage=state.ready?"server_original_death_pickups_running":"server_pickups_waiting_original_effect_lifecycle";
    if(GetTickCount64()-state.lastCache>=500){state.lastCache=GetTickCount64();Cache();}return true;
}
inline bool Poll(){
    if(state.busy)return Fail(7915,"server_pickup_poll_reentered");
    bool result=false;state.busy=true;
    __try{result=PollImpl();}__finally{state.busy=false;}
    return result;
}
inline bool Stop(){
    if(!state.initialized||state.stopped)return true;if(!Context())return false;
    for(size_t n=0;n<state.created;++n)if(pool[n].active&&!Disable(pool[n],false))return false;
    for(auto& d:pending)d={};state.stopped=true;state.active=state.pending=0;state.stage="server_pickups_stopped";Cache();return true;
}
template<class Visit> inline bool VisitActors(Visit&& visit){
    if(!state.initialized)return true;if(!Context())return false;
    for(size_t n=0;n<state.created;++n){const auto& p=pool[n];if(!Live(p.actor,p.identity))return false;visit(p.actor);}return true;
}
inline void PollClient(void* world,void* connection){
    if(!original_players::Thread()||!world||!connection||GetTickCount64()-state.clientSampleAt<1000)return;
    state.clientSampleAt=GetTickCount64();state.clientActors=state.clientEnabled=0;state.clientError=0;
    state.stage="client_observing_original_death_pickup_channels";
    auto type=engine::Find(PackClassPath);if(!type){Cache();return;}
    const int enabled=Field(type,L"IsEnabled",1);const auto channels=Read<Array>(connection,0x78);
    if(enabled<0||channels.count<0||channels.count>65536||channels.capacity<channels.count||(channels.count&&!channels.data)){state.clientError=7919;Cache();return;}
    for(int32_t n=0;n<channels.count;++n){auto channel=channels.data[n];if(!original_players::IsA(channel,0x3dd3d40)||Read<void*>(channel,0x30)!=connection)continue;
        auto actor=Read<void*>(channel,0x78);if(!actor||!original_players::Derived(Read<void*>(actor,0x10),type)||original_effects::GetWorld(actor)!=world)continue;
        ++state.clientActors;const auto value=Read<uint8_t>(actor,static_cast<size_t>(enabled));if(value==1)++state.clientEnabled;else if(value>1)state.clientError=7919;
    }
    Cache();
}
} // namespace local_server_pickups
