#pragma once
#include "original_inventory_slots.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

// Truthful identity for NPCs actually created by this local authority. The
// caller must admit the exact match tuple, including its existing native weak
// identities, before this module can invoke the retained typed bool setter.
// InitialOnly publication belongs before the first native PlayerState snapshot.
// Evidence: work/local-server-playable/hero-abilities/group2-npc-identity/.
namespace local_server_npc_identity {
using Weak=original_inventory_slots::WeakIdentity;
inline constexpr size_t MaxPlayers=original_players::MaxPlayers;
inline constexpr uint16_t BotRepIndex=12;
inline constexpr int32_t BotOffset=0x46f;
inline constexpr uint8_t BotMask=8;
struct Tuple {
    size_t index=0;void *pc=nullptr,*ps=nullptr,*pawn=nullptr;uint32_t epoch=0;bool human=false;
};
// firstNpcMark is true only for a previously unrecorded PS. The match callback
// must additionally prove its synchronous epoch1 BindPlayer publication fence.
using Validator=bool(*)(const Tuple&,bool firstNpcMark);
struct Name {uint32_t words[3]{};};
struct Array {uint8_t* data=nullptr;int32_t count=0,capacity=0;};
struct SetterParams {void* object=nullptr;Name property{};uint8_t value=1,padding[3]{};};
static_assert(sizeof(Name)==12&&sizeof(Array)==16&&sizeof(SetterParams)==24
    &&offsetof(SetterParams,property)==8&&offsetof(SetterParams,value)==20);
struct FieldObservation {
    void *field=nullptr,*fieldClass=nullptr,*referencedType=nullptr;
    Name expectedName{},actualName{};
    uint64_t castFlags=0,flags=0;int32_t offset=-1,bytes=0,arrayDim=0;
    uint16_t repIndex=0;
    uint8_t boolSize=0,boolOffset=0,boolByteMask=0,boolFieldMask=0;
    bool found=false,readable=false,nameMatches=false;
};
struct FunctionObservation {
    void *function=nullptr,*owner=nullptr,*firstProperty=nullptr,*unexpectedField=nullptr;
    uintptr_t exec=0;uint32_t flags=0;uint16_t parmsSize=0,returnOffset=0;
    int32_t propertiesSize=0;uint8_t numParms=0;
    uint32_t childCount=0,parameterCount=0;bool chainValid=false;
    std::array<FieldObservation,3> fields{};
};
struct Metadata {
    bool observed=false,valid=false;
    void *playerStateClass=nullptr,*systemClass=nullptr,*system=nullptr,*objectClass=nullptr;
    FieldObservation bot{};std::array<FunctionObservation,2> functions{};
};
struct Entry {
    bool used=false,human=false,marked=false,verified=false,setterAttempted=false;
    bool originalBefore=false,originalAfter=false,neighborPreserved=false,initialChannelAbsent=false;
    uint32_t epoch=0;size_t index=0;
    void *pc=nullptr,*ps=nullptr,*pawn=nullptr,*actualClass=nullptr;
    Weak pcId{},psId{},pawnId{},classId{};
    uint8_t before=0,after=0,neighbors=0;
    uint64_t getterCalls=0,setterCalls=0,verifications=0,respawnChecks=0;
};
struct State {
    bool contextBound=false,profilesVerified=false,schemaVerified=false,restartRequired=false,closed=false;
    bool active=false,lastSourceValid=false,lastWeakValid=false,lastClassRepsValid=false;
    bool deliveryVerified=false;unsigned error=0;DWORD gameThread=0;
    void *world=nullptr,*driver=nullptr,*lastClass=nullptr,*lastChannel=nullptr;
    Weak worldId{},driverId{};
    uint64_t binds=0,marks=0,verifyRequests=0,metadataObservations=0,getterCalls=0,setterCalls=0;
    uint64_t npcMarked=0,humanChecks=0,respawnChecks=0,setupCalls=0,setupReadbacks=0;
    uint64_t channelChecks=0,lateMarkerRejects=0;
    uint32_t setupFlagsBefore=0,setupFlagsAfter=0;
    uint16_t lastRepIndex=0;int32_t lastClassReps=0,lastBaseClassReps=0,lastArrayIndex=-1;
    int32_t lastClassMatches=0,lastBaseMatches=0,lastActorNetMode=-1;
    uintptr_t profileFailureRva=0,exceptionRva=0;uint32_t exceptionCode=0;
    const char* stage="npc_identity_not_bound";const char* operation="none";
    Metadata metadata{};std::array<Entry,MaxPlayers> entries{};
};
struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
inline constexpr Profile Profiles[]{
    {0x49451d0,37,0x115a9a08d8f62185ULL},{0x45c9e70,739,0x189fe9ed2749ba2bULL},
    {0x1b979c0,1033,0x16a648ec490bf709ULL},
    {0x4947c00,1354,0xe159dc7996f1fab4ULL},{0x717d000,1254,0x74c321616f5a6538ULL},
    {0x7195030,3344,0x1d8c75856a4cb3c4ULL},
    {0x1a7b720,196,0x4982523f72f33036ULL},{0x1c63870,269,0xde792136130ef719ULL},
    {0x3ed83b0,60,0xb0370ccdf6164999ULL},{0x3efc820,176,0x7f1113b3f79164b2ULL},
};
struct ClassReceipt {bool used=false,attempted=false,inFlight=false;void* type=nullptr;Weak identity{};};
inline State state{},cached{};
inline Metadata boundMetadata{};
inline std::array<Weak,6> metadataIds{};
inline std::array<ClassReceipt,64> classes{};
inline Validator validator=nullptr;
inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline const State& GetState(){return state;}
inline void Cache(){AcquireSRWLockExclusive(&cacheLock);cached=state;ReleaseSRWLockExclusive(&cacheLock);}
inline State Snapshot(){AcquireSRWLockShared(&cacheLock);auto result=cached;ReleaseSRWLockShared(&cacheLock);return result;}
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void* object,const Weak& id){return object&&original_inventory_slots::ResolveIdentity(id)==object;}
inline bool Capture(void* object,Weak& id){return original_inventory_slots::CaptureIdentity(object,id)&&Live(object,id);}
inline bool Thread(){return engine::image&&original_players::Thread()&&!state.restartRequired
    &&(!state.gameThread||state.gameThread==GetCurrentThreadId());}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;Cache();return false;}
inline bool SameName(const Name& a,const Name& b){return std::memcmp(&a,&b,sizeof(Name))==0;}
inline Name NativeName(const wchar_t* name){Name result{};
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&result,name,1);return result;}
inline bool ValidArray(const Array& a,size_t stride,int32_t maximum){
    return a.count>=0&&a.count<=maximum&&a.capacity>=a.count&&a.capacity<=maximum
        &&(!a.count||(a.data&&original_players::Readable(a.data,size_t(a.count)*stride)));
}
inline bool VerifyProfiles(){
    if(state.profilesVerified)return true;
    for(const auto& p:Profiles){const auto data=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        uint64_t hash=0xcbf29ce484222325ULL;
        if(!original_players::Readable(data,p.bytes)){state.profileFailureRva=p.rva;
            return Fail(3390,"npc_identity_original_native_profile_unreadable");}
        for(size_t n=0;n<p.bytes;++n)hash=(hash^data[n])*0x100000001b3ULL;
        if(hash!=p.fnv){state.profileFailureRva=p.rva;return Fail(3390,"npc_identity_original_native_profile_mismatch");}
    }state.profilesVerified=true;return true;
}
inline void* ActorWorld(void* actor){
    auto fn=Read<void*>(Read<void*>(actor),0x1b8);
    return fn==reinterpret_cast<void*>(engine::image+0x3ed83b0)
        ?reinterpret_cast<void*(*)(void*)>(fn)(actor):nullptr;
}
inline bool HostWorld(){
    if(!Live(state.world,state.worldId)||!Live(state.driver,state.driverId)||original_players::world!=state.world
       ||Read<void*>(state.world,0x40)!=state.driver||Read<void*>(state.driver,0x178)!=state.world
       ||Read<void*>(state.driver,0xc0))return false;
    return ValidArray(Read<Array>(state.driver,0xc8),8,32);
}
inline bool BindContextImpl(void* world,Validator admit){
    if(!Thread()||state.closed||state.error||!world||!admit)return false;
    if(state.contextBound)return state.world==world&&validator==admit&&HostWorld();
    state.operation="bind_context";state.gameThread=GetCurrentThreadId();state.world=world;validator=admit;
    state.driver=Read<void*>(world,0x40);
    if(!Capture(world,state.worldId)||!Capture(state.driver,state.driverId)||!HostWorld()||!VerifyProfiles())
        return state.error?false:Fail(3391,"npc_identity_requires_actual_original_host_world_and_driver");
    state.contextBound=true;++state.binds;state.stage="npc_identity_host_bound_no_npc_or_delivery_claim";return true;
}
inline bool Source(const Tuple& t,std::array<Weak,4>& identities,bool firstNpcMark){
    state.lastSourceValid=state.lastWeakValid=false;
    if(!Thread()||!state.contextBound||state.closed||state.error||!HostWorld()||!validator
       ||t.index>=MaxPlayers||!t.epoch||t.human!=(t.index==0)||!validator(t,firstNpcMark))return false;
    const std::array<void*,4> objects{t.pc,t.ps,t.pawn,Read<void*>(t.ps,0x10)};
    for(size_t n=0;n<objects.size();++n)if(!Capture(objects[n],identities[n]))return false;
    state.lastWeakValid=true;
    if(!original_players::IsA(t.pc,original_players::ShooterControllerClass)
       ||!original_players::IsA(t.ps,original_players::ShooterStateClass)
       ||!original_players::IsA(t.pawn,original_players::ShooterCharacterClass)
       ||Read<uint8_t>(t.pc,0x224)!=3||Read<uint8_t>(t.ps,0x224)!=3||Read<uint8_t>(t.pawn,0x224)!=3
       ||Read<void*>(t.pc,0x470)!=t.ps||Read<void*>(t.pc,0x4b0)!=t.pawn||Read<void*>(t.ps,0x210)!=t.pc
       ||Read<void*>(t.pawn,0x4a0)!=t.pc||Read<void*>(t.pawn,0x488)!=t.ps||!original_players::Registered(t.ps)
       ||ActorWorld(t.pc)!=state.world||ActorWorld(t.ps)!=state.world||ActorWorld(t.pawn)!=state.world
       ||Read<uintptr_t>(Read<void*>(t.ps),0x220)!=engine::image+0x7195030)return false;
    state.lastActorNetMode=engine::function<int(*)(void*)>(0x3efc820)(t.ps);
    if(state.lastActorNetMode!=1&&state.lastActorNetMode!=2)return false;
    auto player=Read<void*>(t.pc,0x510);
    if(!t.human){if(player||(firstNpcMark&&t.epoch!=1))return false;}
    else{
        const auto peers=Read<Array>(state.driver,0xc8);int matches=0;
        for(int32_t n=0;n<peers.count;++n)if(Read<void*>(peers.data,size_t(n)*8)==player)++matches;
        if(!player||matches!=1||Read<void*>(player,0x60)!=state.driver
           ||Read<void*>(player,0x38)!=t.pc||Read<void*>(player,0xa0)!=t.pc)return false;
    }
    for(size_t n=0;n<objects.size();++n)if(!Live(objects[n],identities[n]))return false;
    state.lastSourceValid=true;return true;
}
inline FieldObservation ObserveField(void* owner,const wchar_t* name){
    FieldObservation o{};o.expectedName=NativeName(name);
    if(!owner)return o;
    o.field=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(owner,&o.expectedName,3);
    o.found=o.field!=nullptr;o.readable=o.field&&original_players::Readable(o.field,0x80);
    if(!o.readable)return o;
    o.fieldClass=Read<void*>(o.field,8);o.castFlags=Read<uint64_t>(o.fieldClass,0x18);
    o.actualName=Read<Name>(o.field,0x20);o.nameMatches=SameName(o.expectedName,o.actualName);
    o.arrayDim=Read<int32_t>(o.field,0x30);o.bytes=Read<int32_t>(o.field,0x34);
    o.flags=Read<uint64_t>(o.field,0x38);o.repIndex=Read<uint16_t>(o.field,0x40);o.offset=Read<int32_t>(o.field,0x44);
    o.referencedType=Read<void*>(o.field,0x78);
    o.boolSize=Read<uint8_t>(o.field,0x78);o.boolOffset=Read<uint8_t>(o.field,0x79);
    o.boolByteMask=Read<uint8_t>(o.field,0x7a);o.boolFieldMask=Read<uint8_t>(o.field,0x7b);return o;
}
inline FunctionObservation ObserveFunction(const wchar_t* path,const wchar_t* const* names,size_t count){
    FunctionObservation o{};o.function=engine::Find(path);o.owner=Read<void*>(o.function,0x28);
    o.exec=Read<uintptr_t>(o.function,0xe0);o.flags=Read<uint32_t>(o.function,0xb8);
    o.numParms=Read<uint8_t>(o.function,0xbc);o.parmsSize=Read<uint16_t>(o.function,0xbe);
    o.returnOffset=Read<uint16_t>(o.function,0xc0);o.propertiesSize=Read<int32_t>(o.function,0x60);
    o.firstProperty=Read<void*>(o.function,0x58);
    // Observe every expected field even if a previous field or UF disagrees.
    for(size_t n=0;n<count&&n<o.fields.size();++n)o.fields[n]=ObserveField(o.function,names[n]);
    std::array<void*,16> seen{};auto field=o.firstProperty;o.chainValid=true;
    while(field){
        if(o.childCount>=seen.size()||!original_players::Readable(field,0x78)){o.chainValid=false;break;}
        bool repeated=false;for(uint32_t n=0;n<o.childCount;++n)if(seen[n]==field)repeated=true;
        if(repeated){o.chainValid=false;break;}
        seen[o.childCount++]=field;
        if(Read<uint64_t>(field,0x38)&0x80ULL)++o.parameterCount;
        bool expected=false;for(size_t n=0;n<count&&n<o.fields.size();++n)if(o.fields[n].field==field)expected=true;
        if(!expected&&!o.unexpectedField)o.unexpectedField=field;
        field=Read<void*>(field,0x18);
    }return o;
}
inline bool BasicField(const FieldObservation& p,int32_t offset,int32_t bytes,uint64_t cast){
    return p.readable&&p.nameMatches&&p.arrayDim==1&&p.offset==offset&&p.bytes==bytes
        &&(p.castFlags&cast)==cast;
}
inline bool NativeBool(const FieldObservation& p,int32_t offset){
    return BasicField(p,offset,1,0x28001)&&p.boolSize==1&&p.boolOffset==0&&p.boolByteMask==1&&p.boolFieldMask==0xff;
}
inline bool SameField(const FieldObservation& a,const FieldObservation& b){
    return a.field==b.field&&a.fieldClass==b.fieldClass&&a.referencedType==b.referencedType
        &&SameName(a.expectedName,b.expectedName)&&SameName(a.actualName,b.actualName)
        &&a.castFlags==b.castFlags&&a.flags==b.flags&&a.offset==b.offset&&a.bytes==b.bytes&&a.arrayDim==b.arrayDim
        &&a.boolSize==b.boolSize&&a.boolOffset==b.boolOffset&&a.boolByteMask==b.boolByteMask&&a.boolFieldMask==b.boolFieldMask
        &&a.found==b.found&&a.readable==b.readable&&a.nameMatches==b.nameMatches;
}
inline bool SameFunction(const FunctionObservation& a,const FunctionObservation& b,size_t fields){
    if(a.function!=b.function||a.owner!=b.owner||a.exec!=b.exec||a.flags!=b.flags||a.parmsSize!=b.parmsSize
       ||a.returnOffset!=b.returnOffset||a.propertiesSize!=b.propertiesSize||a.numParms!=b.numParms
       ||a.firstProperty!=b.firstProperty||a.childCount!=b.childCount||a.parameterCount!=b.parameterCount
       ||a.unexpectedField!=b.unexpectedField||a.chainValid!=b.chainValid)return false;
    for(size_t n=0;n<fields;++n)if(!SameField(a.fields[n],b.fields[n]))return false;return true;
}
inline std::array<void*,6> MetadataObjects(const Metadata& m){
    return {m.playerStateClass,m.systemClass,m.system,m.objectClass,m.functions[0].function,m.functions[1].function};
}
inline bool ObserveMetadata(){
    Metadata m{};m.observed=true;
    m.playerStateClass=engine::Class(0x42aae70);m.systemClass=engine::Class(0x45aa110);
    m.objectClass=engine::Class(0x1af05b0);m.system=m.systemClass?engine::DefaultObject(m.systemClass):nullptr;
    m.bot=ObserveField(m.playerStateClass,L"bIsABot");
    const wchar_t* getterNames[]{L"ReturnValue"};
    const wchar_t* setterNames[]{L"Object",L"PropertyName",L"Value"};
    m.functions[0]=ObserveFunction(L"/Script/Engine.PlayerState.IsABot",getterNames,1);
    m.functions[1]=ObserveFunction(L"/Script/Engine.KismetSystemLibrary.SetBoolPropertyByName",setterNames,3);
    // Cache the complete actual observation before evaluating a single gate.
    state.metadata=m;++state.metadataObservations;Cache();
    const auto& b=m.bot;const auto& g=m.functions[0];const auto& s=m.functions[1];
    const auto& object=s.fields[0];const auto& name=s.fields[1];const auto& value=s.fields[2];
    const auto& result=g.fields[0];
    const bool bot=BasicField(b,BotOffset,1,0x28001)&&(b.flags&0x34ULL)==0x34ULL
        &&b.boolSize==1&&b.boolOffset==0&&b.boolByteMask==BotMask&&b.boolFieldMask==BotMask;
    const bool getter=g.function&&g.owner==m.playerStateClass&&g.exec==engine::image+0x49451d0
        &&(g.flags&0x400U)!=0&&(g.flags&0x12060c0U)==0&&g.numParms==1&&g.parmsSize==1&&g.returnOffset==0
        &&g.propertiesSize>=1&&g.chainValid&&!g.unexpectedField&&g.childCount==1&&g.parameterCount==1
        &&NativeBool(result,0)&&(result.flags&0x580ULL)==0x580ULL;
    // The reflected UF size and flags are actual observations, not the padded
    // generated24-byte C++ container or a guessed21-byte runtime constant.
    const int32_t parameterEnd=value.offset+value.bytes;
    const bool setter=s.function&&s.owner==m.systemClass&&s.exec==engine::image+0x45c9e70
        &&(s.flags&0x2400U)==0x2400U&&(s.flags&0x12040c0U)==0&&s.numParms==3
        &&parameterEnd>0&&s.parmsSize>=parameterEnd&&s.parmsSize<=sizeof(SetterParams)
        &&s.propertiesSize>=s.parmsSize&&s.propertiesSize<=sizeof(SetterParams)
        &&s.chainValid&&!s.unexpectedField&&s.childCount==3&&s.parameterCount==3
        &&BasicField(object,0,8,0x18001)&&object.referencedType==m.objectClass
        &&BasicField(name,8,12,0xa001)&&NativeBool(value,20)
        &&(object.flags&0x580ULL)==0x80ULL&&(name.flags&0x580ULL)==0x80ULL&&(value.flags&0x580ULL)==0x80ULL;
    m.valid=m.playerStateClass&&m.systemClass&&m.objectClass&&m.system&&Read<void*>(m.system,0x10)==m.systemClass
        &&bot&&getter&&setter;state.metadata=m;Cache();
    if(!m.valid)return Fail(3392,"npc_identity_observed_original_bool_property_or_function_schema_mismatch");
    if(state.schemaVerified){
        const auto objects=MetadataObjects(m);const auto previous=MetadataObjects(boundMetadata);
        for(size_t n=0;n<objects.size();++n)if(objects[n]!=previous[n]||!Live(objects[n],metadataIds[n]))
            return Fail(3392,"npc_identity_captured_metadata_weak_identity_changed");
        if(!SameField(m.bot,boundMetadata.bot)||!SameFunction(g,boundMetadata.functions[0],1)
           ||!SameFunction(s,boundMetadata.functions[1],3))return Fail(3392,"npc_identity_captured_original_schema_changed");
    }return true;
}
inline bool RepRecords(void* type,int32_t& count,int32_t& matches){
    const auto a=Read<Array>(type,0x100);count=a.count;matches=0;state.lastArrayIndex=-1;
    if(!(Read<uint32_t>(type,0xdc)&0x800U)||!ValidArray(a,16,16384)||a.count<=BotRepIndex
       ||Read<uint16_t>(state.metadata.bot.field,0x40)!=BotRepIndex)return false;
    for(int32_t n=0;n<a.count;++n)if(Read<void*>(a.data,size_t(n)*16)==state.metadata.bot.field){
        ++matches;state.lastArrayIndex=Read<int32_t>(a.data,size_t(n)*16+8);
        if(n!=BotRepIndex||state.lastArrayIndex!=0)return false;
    }return matches==1&&Read<void*>(a.data,size_t(BotRepIndex)*16)==state.metadata.bot.field
        &&Read<int32_t>(a.data,size_t(BotRepIndex)*16+8)==0;
}
inline bool PrepareReplication(void* type,bool allowSetup){
    state.lastClassRepsValid=false;state.lastClass=type;state.setupFlagsBefore=Read<uint32_t>(type,0xdc);
    if(!type||!original_players::IsA(type,0x1af0eb0)
       ||!original_players::Derived(type,state.metadata.playerStateClass))return Fail(3393,"npc_identity_actual_player_state_uclass_required");
    Weak id{};if(!Capture(type,id))return Fail(3393,"npc_identity_actual_replication_class_weak_missing");
    ClassReceipt *receipt=nullptr,*empty=nullptr;
    for(auto& r:classes){if(r.used&&r.type==type&&Live(r.type,r.identity)){receipt=&r;break;}
        if(!empty&&(!r.used||!Live(r.type,r.identity)))empty=&r;}
    if(!receipt){if(!empty)return Fail(3393,"npc_identity_original_replication_class_receipts_exhausted");
        *empty={};empty->used=true;empty->type=type;empty->identity=id;receipt=empty;}
    if(receipt->inFlight)return Fail(3393,"npc_identity_original_replication_setup_reentered");
    if(!(state.setupFlagsBefore&0x800U)){
        if(!allowSetup||receipt->attempted)return Fail(3393,"npc_identity_original_replication_data_not_ready_no_forced_retry");
        receipt->attempted=true;receipt->inFlight=true;++state.setupCalls;
        // Original49D6695 calls this exact API. It recursively prepares actual
        // classes and writes their native indices, records and setup flag.
        __try{engine::function<void(*)(void*)>(0x1b979c0)(type);}
        __finally{receipt->inFlight=false;}
    }
    state.setupFlagsAfter=Read<uint32_t>(type,0xdc);state.lastRepIndex=Read<uint16_t>(state.metadata.bot.field,0x40);
    const bool actualRecords=RepRecords(type,state.lastClassReps,state.lastClassMatches);
    const bool baseRecords=RepRecords(state.metadata.playerStateClass,state.lastBaseClassReps,state.lastBaseMatches);
    if(!Live(type,id)||!actualRecords||!baseRecords){
        state.restartRequired=true;return Fail(3393,"npc_identity_original_replication_class_index_or_record_readback_failed");}
    ++state.setupReadbacks;state.lastClassRepsValid=true;state.metadata.bot.repIndex=state.lastRepIndex;
    if(!state.schemaVerified){
        const auto objects=MetadataObjects(state.metadata);
        for(size_t n=0;n<objects.size();++n)if(!Capture(objects[n],metadataIds[n]))
            return Fail(3392,"npc_identity_original_metadata_weak_capture_failed");
        boundMetadata=state.metadata;state.schemaVerified=true;
    }return true;
}
inline bool NoInitialChannel(const Tuple& t){
    state.lastChannel=nullptr;const auto peers=Read<Array>(state.driver,0xc8);
    if(!ValidArray(peers,8,32))return false;
    for(int32_t n=0;n<peers.count;++n){auto peer=Read<void*>(peers.data,size_t(n)*8);Weak peerId{};
        if(!Capture(peer,peerId)||Read<void*>(peer,0x60)!=state.driver)return false;
        const auto channels=Read<Array>(peer,0x78);if(!ValidArray(channels,8,512))return false;
        for(int32_t c=0;c<channels.count;++c){auto channel=Read<void*>(channels.data,size_t(c)*8);if(!channel)continue;
            Weak id{};if(!Capture(channel,id)||Read<void*>(channel,0x30)!=peer)return false;
            ++state.channelChecks;
            if(original_players::IsA(channel,0x3dd3d40)&&Read<void*>(channel,0x78)==t.ps){
                state.lastChannel=channel;++state.lateMarkerRejects;return false;}
        }
        if(!Live(peer,peerId))return false;
    }return true;
}
inline bool Dispatch(void* object,void* function,void* params){
    auto process=Read<void*>(Read<void*>(object),0x298);
    if(!object||!function||!process)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,function,params);return true;
}
inline bool NativeGetter(const Tuple& t,bool& result,Entry& e){
    uint8_t returned=255;++state.getterCalls;++e.getterCalls;
    if(!Dispatch(t.ps,boundMetadata.functions[0].function,&returned)||returned>1)return false;
    result=returned!=0;return result==bool(Read<uint8_t>(t.ps,BotOffset)&BotMask);
}
inline bool ReceiptSource(const Tuple& t,const Entry& e,bool respawn){
    if(!e.used||e.index!=t.index||e.human!=t.human||e.pc!=t.pc||e.ps!=t.ps||!Live(e.pc,e.pcId)
       ||!Live(e.ps,e.psId)||!Live(e.actualClass,e.classId)||Read<void*>(t.ps,0x10)!=e.actualClass)return false;
    if(respawn)return e.epoch<UINT32_MAX&&t.epoch==e.epoch+1&&t.pawn!=e.pawn;
    return t.epoch==e.epoch&&t.pawn==e.pawn&&Live(e.pawn,e.pawnId);
}
inline bool MarkImpl(const Tuple& t,bool verifyOnly){
    if(!Thread()||!state.contextBound||state.closed||state.error)return false;
    if(state.active)return Fail(3394,"npc_identity_mark_or_verify_reentered");
    state.operation=verifyOnly?"verify":"mark";
    if(verifyOnly)++state.verifyRequests;else ++state.marks;
    if(t.index>=MaxPlayers)return Fail(3394,"npc_identity_roster_index_outside_actual_capacity");
    auto& entry=state.entries[t.index];const bool fresh=!entry.used;
    if(verifyOnly&&fresh)return Fail(3394,"npc_identity_verify_requires_previous_player_state_receipt");
    std::array<Weak,4> ids{};
    if(!Source(t,ids,fresh&&!t.human)||!VerifyProfiles())
        return state.error?false:Fail(3394,"npc_identity_requires_actual_admitted_match_tuple_and_weak_identities");
    if(!ObserveMetadata()||!PrepareReplication(Read<void*>(t.ps,0x10),!verifyOnly))return false;
    const bool respawn=!fresh&&t.epoch!=entry.epoch;
    if(!fresh&&!ReceiptSource(t,entry,!verifyOnly&&respawn))
        return Fail(3394,"npc_identity_persistent_player_state_or_spawn_epoch_changed");
    if(fresh&&!t.human&&!NoInitialChannel(t))return Fail(3395,"npc_identity_initialonly_marker_requires_pre_snapshot_factory_fence");
    Entry candidate=fresh?Entry{}:entry;
    candidate.index=t.index;candidate.human=t.human;candidate.pc=t.pc;candidate.ps=t.ps;candidate.pawn=t.pawn;
    candidate.actualClass=Read<void*>(t.ps,0x10);candidate.pcId=ids[0];candidate.psId=ids[1];
    candidate.pawnId=ids[2];candidate.classId=ids[3];candidate.epoch=t.epoch;
    candidate.before=Read<uint8_t>(t.ps,BotOffset);
    if(!NativeGetter(t,candidate.originalBefore,candidate))return Fail(3396,"npc_identity_original_getter_or_packed_property_disagrees");
    if(!fresh&&candidate.originalBefore==t.human)
        return Fail(3396,"npc_identity_persistent_native_marker_changed_no_rewrite");
    if(t.human&&candidate.originalBefore)return Fail(3396,"npc_identity_actual_human_original_bot_marker_must_be_false");
    candidate.initialChannelAbsent=fresh&&!t.human?true:candidate.initialChannelAbsent;
    // Other packed flags keep their original lifetime. Compare this operation's
    // before/after bytes; do not freeze legitimate spectator/inactive changes
    // that occurred between separate calls or player spawn epochs.
    candidate.neighbors=candidate.before&uint8_t(~BotMask);
    state.active=true;
    if(fresh&&!t.human&&!candidate.originalBefore){
        candidate.setterAttempted=true;++candidate.setterCalls;++state.setterCalls;
        // Actual native metadata determined ParmsSize and all three offsets.
        // The sole target write is this retained typed FBoolProperty setter.
        SetterParams params{};params.object=t.ps;params.property=boundMetadata.bot.expectedName;
        if(!Dispatch(boundMetadata.system,boundMetadata.functions[1].function,&params)){
            state.active=false;return Fail(3397,"npc_identity_original_typed_bool_setter_dispatch_failed");}
    }
    state.active=false;
    candidate.after=Read<uint8_t>(t.ps,BotOffset);
    if(state.error||!NativeGetter(t,candidate.originalAfter,candidate))return Fail(3397,"npc_identity_original_marker_getter_after_setter_failed");
    candidate.neighborPreserved=((candidate.after^candidate.before)&uint8_t(~BotMask))==0
        &&(candidate.after&uint8_t(~BotMask))==candidate.neighbors;
    std::array<Weak,4> afterIds{};
    if(!candidate.neighborPreserved||candidate.originalAfter==t.human||!Source(t,afterIds,fresh&&!t.human)
       ||!ObserveMetadata()||!PrepareReplication(candidate.actualClass,false)){
        state.restartRequired=true;return state.error?false:Fail(3397,"npc_identity_original_marker_neighbor_or_source_readback_failed");}
    for(size_t n=0;n<ids.size();++n)if(!original_inventory_slots::SameIdentity(ids[n],afterIds[n])){
        state.restartRequired=true;return Fail(3397,"npc_identity_source_weak_changed_during_original_marker_transaction");}
    candidate.used=candidate.verified=true;candidate.marked=!t.human;++candidate.verifications;
    if(respawn){++candidate.respawnChecks;++state.respawnChecks;}
    entry=candidate;if(fresh&&!t.human)++state.npcMarked;if(t.human)++state.humanChecks;
    state.stage=t.human?"npc_identity_original_human_false_verified_without_write":
        respawn?"npc_identity_same_player_state_bot_true_reverified_without_rewrite":
        "npc_identity_actual_nonhuman_original_bot_marker_true_delivery_unverified";
    return true;
}
inline LONG Exception(EXCEPTION_POINTERS* e){
    state.active=false;state.restartRequired=true;state.error=3398;
    state.exceptionCode=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0;
    const auto address=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=address>=engine::image?address-engine::image:address;
    state.stage="npc_identity_original_native_exception_requires_new_owned_session";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool BindContext(void* world,Validator admit){
    if(!Thread())return false;
    __try{const bool result=BindContextImpl(world,admit);Cache();return result;}
    __except(Exception(GetExceptionInformation())){Cache();return false;}
}
inline bool Mark(size_t index,void* pc,void* ps,void* pawn,uint32_t epoch,bool human){
    if(!Thread())return false;
    __try{const bool result=MarkImpl({index,pc,ps,pawn,epoch,human},false);Cache();return result;}
    __except(Exception(GetExceptionInformation())){Cache();return false;}
}
inline bool Verify(size_t index,void* pc,void* ps,void* pawn,uint32_t epoch,bool human){
    if(!Thread())return false;
    __try{const bool result=MarkImpl({index,pc,ps,pawn,epoch,human},true);Cache();return result;}
    __except(Exception(GetExceptionInformation())){Cache();return false;}
}
inline bool Close(){
    if(!Thread()||state.active)return false;
    state.closed=true;state.stage="npc_identity_closed_original_markers_and_actor_lifetimes_retained";Cache();return true;
}
inline const char* Yes(bool value){return value?"true":"false";}
inline void WriteField(FILE* file,const FieldObservation& p){
    std::fprintf(file,"{\"field\":\"%p\",\"field_class\":\"%p\",\"referenced_type\":\"%p\",\"found\":%s,\"readable\":%s,\"name_matches\":%s,\"name\":[%u,%u,%u],\"expected_name\":[%u,%u,%u],\"cast_flags\":\"0x%llx\",\"flags\":\"0x%llx\",\"offset\":%d,\"bytes\":%d,\"array_dim\":%d,\"rep_index\":%u,\"bool_size\":%u,\"bool_offset\":%u,\"bool_byte_mask\":%u,\"bool_field_mask\":%u}",
        p.field,p.fieldClass,p.referencedType,Yes(p.found),Yes(p.readable),Yes(p.nameMatches),p.actualName.words[0],p.actualName.words[1],p.actualName.words[2],
        p.expectedName.words[0],p.expectedName.words[1],p.expectedName.words[2],static_cast<unsigned long long>(p.castFlags),static_cast<unsigned long long>(p.flags),
        p.offset,p.bytes,p.arrayDim,unsigned(p.repIndex),unsigned(p.boolSize),unsigned(p.boolOffset),unsigned(p.boolByteMask),unsigned(p.boolFieldMask));
}
inline void WriteFunction(FILE* file,const FunctionObservation& p,size_t fields){
    std::fprintf(file,"{\"function\":\"%p\",\"owner\":\"%p\",\"exec\":\"0x%llx\",\"flags\":%u,\"parms_size\":%u,\"return_offset\":%u,\"properties_size\":%d,\"num_parms\":%u,\"first_property\":\"%p\",\"unexpected_field\":\"%p\",\"chain_valid\":%s,\"child_count\":%u,\"parameter_count\":%u,\"fields\":[",
        p.function,p.owner,static_cast<unsigned long long>(p.exec),p.flags,unsigned(p.parmsSize),unsigned(p.returnOffset),p.propertiesSize,unsigned(p.numParms),
        p.firstProperty,p.unexpectedField,Yes(p.chainValid),p.childCount,p.parameterCount);
    for(size_t n=0;n<fields;++n){if(n)std::fputc(',',file);WriteField(file,p.fields[n]);}std::fputs("]}",file);
}
inline void Write(FILE* file){
    if(!file)return;const auto s=Snapshot();
    std::fprintf(file,"{\"stage\":\"%s\",\"operation\":\"%s\",\"error\":%u,\"context_bound\":%s,\"profiles_verified\":%s,\"schema_verified\":%s,\"restart_required\":%s,\"closed\":%s,\"active\":%s,\"world\":\"%p\",\"driver\":\"%p\",\"last_source_valid\":%s,\"last_weak_valid\":%s,\"actor_net_mode\":%d,\"metadata_observations\":%llu,\"getter_calls\":%llu,\"setter_calls\":%llu,\"npc_marked\":%llu,\"human_checks\":%llu,\"respawn_checks\":%llu,\"marks\":%llu,\"verify_requests\":%llu,\"setup_calls\":%llu,\"setup_readbacks\":%llu,\"last_class\":\"%p\",\"setup_flags_before\":%u,\"setup_flags_after\":%u,\"rep_index\":%u,\"class_reps\":%d,\"base_class_reps\":%d,\"class_matches\":%d,\"base_matches\":%d,\"array_index\":%d,\"class_reps_valid\":%s,\"channel_checks\":%llu,\"late_marker_rejects\":%llu,\"last_channel\":\"%p\",\"profile_failure_rva\":%llu,\"exception_code\":%u,\"exception_rva\":%llu,\"delivery_verified\":false,\"client_flag_writes\":0,\"role_writes\":0,\"locality_writes\":0,\"account_writes\":0,\"netmode_writes\":0,\"raw_bool_writes\":0,\"cached_snapshot\":true,\"metadata\":{\"observed\":%s,\"valid\":%s,\"player_state_class\":\"%p\",\"system_class\":\"%p\",\"system\":\"%p\",\"object_class\":\"%p\",\"bot\":",
        s.stage,s.operation,s.error,Yes(s.contextBound),Yes(s.profilesVerified),Yes(s.schemaVerified),Yes(s.restartRequired),Yes(s.closed),Yes(s.active),
        s.world,s.driver,Yes(s.lastSourceValid),Yes(s.lastWeakValid),s.lastActorNetMode,
        static_cast<unsigned long long>(s.metadataObservations),static_cast<unsigned long long>(s.getterCalls),static_cast<unsigned long long>(s.setterCalls),
        static_cast<unsigned long long>(s.npcMarked),static_cast<unsigned long long>(s.humanChecks),static_cast<unsigned long long>(s.respawnChecks),
        static_cast<unsigned long long>(s.marks),static_cast<unsigned long long>(s.verifyRequests),static_cast<unsigned long long>(s.setupCalls),static_cast<unsigned long long>(s.setupReadbacks),
        s.lastClass,s.setupFlagsBefore,s.setupFlagsAfter,unsigned(s.lastRepIndex),s.lastClassReps,s.lastBaseClassReps,s.lastClassMatches,s.lastBaseMatches,
        s.lastArrayIndex,Yes(s.lastClassRepsValid),static_cast<unsigned long long>(s.channelChecks),static_cast<unsigned long long>(s.lateMarkerRejects),s.lastChannel,
        static_cast<unsigned long long>(s.profileFailureRva),s.exceptionCode,static_cast<unsigned long long>(s.exceptionRva),Yes(s.metadata.observed),Yes(s.metadata.valid),
        s.metadata.playerStateClass,s.metadata.systemClass,s.metadata.system,s.metadata.objectClass);
    WriteField(file,s.metadata.bot);std::fputs(",\"is_a_bot\":",file);WriteFunction(file,s.metadata.functions[0],1);
    std::fputs(",\"set_bool_property_by_name\":",file);WriteFunction(file,s.metadata.functions[1],3);std::fputs("},\"entries\":[",file);
    bool first=true;for(const auto& e:s.entries){if(!e.used)continue;if(!first)std::fputc(',',file);first=false;
        std::fprintf(file,"{\"index\":%zu,\"epoch\":%u,\"human\":%s,\"marked\":%s,\"verified\":%s,\"pc\":\"%p\",\"ps\":\"%p\",\"pawn\":\"%p\",\"actual_class\":\"%p\",\"before\":%u,\"after\":%u,\"neighbors\":%u,\"original_before\":%s,\"original_after\":%s,\"neighbors_preserved\":%s,\"initial_channel_absent\":%s,\"setter_attempted\":%s,\"getter_calls\":%llu,\"setter_calls\":%llu,\"verifications\":%llu,\"respawn_checks\":%llu}",
            e.index,e.epoch,Yes(e.human),Yes(e.marked),Yes(e.verified),e.pc,e.ps,e.pawn,e.actualClass,unsigned(e.before),unsigned(e.after),unsigned(e.neighbors),
            Yes(e.originalBefore),Yes(e.originalAfter),Yes(e.neighborPreserved),Yes(e.initialChannelAbsent),Yes(e.setterAttempted),
            static_cast<unsigned long long>(e.getterCalls),static_cast<unsigned long long>(e.setterCalls),static_cast<unsigned long long>(e.verifications),static_cast<unsigned long long>(e.respawnChecks));
    }std::fputs("]}",file);
}
} // namespace local_server_npc_identity
