#pragma once
#include "local_server_match.h"
#include "humanlike_blind_evidence.h"
#include "humanlike_blind_source_profiles.h"
#include "humanlike_blind_metadata_diagnostics.h"
#include <array>
#include <cstdint>

// Read the existing own BlindManager state at the existing OwnSample boundary.
// No hook, actor/Role write, extra Tick, effect mutation, target list or timer.
namespace local_server_humanlike_blind {
namespace core=humanlike_blind_evidence;
using Weak=original_inventory_slots::WeakIdentity;
inline constexpr uintptr_t ManagerClass=0x65b3db0,QueryExec=0x65b54e0;
inline constexpr const wchar_t* QueryPath=L"/Script/ShooterGame.BlindManagerComponent.IsBlinded";
struct Cache {void*manager=nullptr;Weak managerId{};};
struct State {
    bool checked=false,profiles=false;void *method=nullptr,*owner=nullptr,*returnField=nullptr;
    Weak methodId{},ownerId{};uint64_t samples=0,known=0,active=0,unknown=0,queries=0,faults=0;
    uint64_t metadataRejects=0,lifeRejects=0,worldRejects=0,sourceRejects=0,generationRejects=0;
    uint32_t lastException=0;uintptr_t lastExceptionRva=0;
    std::array<uint32_t,3> expectedReturnName{};bool expectedReturnNameKnown=false;
};
inline State state{};
inline void ClearMetadataDiagnostics();
inline void Reset(){state={};ClearMetadataDiagnostics();}
template<class T>inline T Read(const void*object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void*object,const Weak&id){return object&&original_inventory_slots::ResolveIdentity(id)==object;}
inline core::Weak Id(const Weak&id){return {id.index,id.serial};}
inline bool Profiles(){
    if(state.checked)return state.profiles;state.checked=true;
    for(const auto&p:humanlike_blind_source_profiles::Spans){
        const auto bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes))return false;uint64_t hash=0xcbf29ce484222325ULL;
        for(size_t n=0;n<p.bytes;++n)hash=(hash^bytes[n])*0x100000001b3ULL;
        if(hash!=p.hash)return false;
    }
    state.profiles=true;return true;
}
inline bool ReturnField(){
    auto field=state.returnField;if(!field||Read<void*>(state.method,0x58)!=field||Read<void*>(field,0x18))return false;
    auto cls=Read<void*>(field,8);
    return original_players::Readable(field,0x7c)&&Read<uint64_t>(cls,0x18)==0x28001ULL
        &&Read<int32_t>(field,0x44)==0&&Read<int32_t>(field,0x30)==1&&Read<int32_t>(field,0x34)==1
        &&Read<uint64_t>(field,0x38)==humanlike_blind_metadata_diagnostics::ExpectedReturnFlags
        &&Read<uint8_t>(field,0x78)==1&&Read<uint8_t>(field,0x79)==0
        &&Read<uint8_t>(field,0x7a)!=0&&Read<uint8_t>(field,0x7b)!=0;
}

struct MetadataDiagnostics {humanlike_blind_metadata_diagnostics::Snapshot last{};uint64_t captures=0,mask=0;};
inline MetadataDiagnostics metadataDiagnostics{};inline SRWLOCK metadataLock=SRWLOCK_INIT;
inline void ClearMetadataDiagnostics(){
    AcquireSRWLockExclusive(&metadataLock);metadataDiagnostics={};ReleaseSRWLockExclusive(&metadataLock);
}
inline void CaptureMetadataImpl(humanlike_blind_metadata_diagnostics::Snapshot&out,void*attempt){
    namespace d=humanlike_blind_metadata_diagnostics;
    out.image=engine::image;out.function=reinterpret_cast<uintptr_t>(attempt?attempt:state.method);
    out.cachedOwner=reinterpret_cast<uintptr_t>(state.owner);out.cachedField=reinterpret_cast<uintptr_t>(state.returnField);
    out.expectedName=state.expectedReturnName;out.nameKnown=state.expectedReturnNameKnown;
    out.functionLive=Live(state.method,state.methodId);out.ownerLive=Live(state.owner,state.ownerId);
    if(out.functionLive&&original_players::Readable(state.method,0xe8)){
        out.functionKnown=true;out.outer=Read<uintptr_t>(state.method,0x28);out.exec=Read<uintptr_t>(state.method,0xe0);
        out.functionFlags=Read<uint32_t>(state.method,0xb8);out.parameterBytes=Read<uint16_t>(state.method,0xbe);
        out.propertiesBytes=Read<int32_t>(state.method,0x60);out.firstChild=Read<uintptr_t>(state.method,0x58);out.firstChildKnown=true;
        out.expectedOwner=reinterpret_cast<uintptr_t>(engine::Class(ManagerClass));out.ownerComparisonKnown=out.expectedOwner!=0;
        // This fallback is only an inspected diagnostic child. It is never
        // substituted into state.returnField or the unchanged runtime gate.
        out.inspectedField=out.cachedField?out.cachedField:out.firstChild;out.fieldFromFirstChild=!out.cachedField&&out.firstChild;
        auto field=reinterpret_cast<void*>(out.inspectedField);
        if(field&&original_players::Readable(field,0x7c)){
            out.fieldKnown=true;out.fieldClass=Read<uintptr_t>(field,8);out.next=Read<uintptr_t>(field,0x18);
            out.offset=Read<int32_t>(field,0x44);out.size=Read<int32_t>(field,0x34);out.dimensions=Read<int32_t>(field,0x30);
            out.fieldFlags=Read<uint64_t>(field,0x38);out.boolSize=Read<uint8_t>(field,0x78);out.boolOffset=Read<uint8_t>(field,0x79);
            out.byteMask=Read<uint8_t>(field,0x7a);out.fieldMask=Read<uint8_t>(field,0x7b);
            for(size_t n=0;n<out.actualName.size();++n)out.actualName[n]=Read<uint32_t>(field,0x20+n*4);
            if(original_players::Readable(reinterpret_cast<void*>(out.fieldClass),0x20)){
                out.fieldClassKnown=true;out.fieldCastFlags=Read<uint64_t>(reinterpret_cast<void*>(out.fieldClass),0x18);
            }
        }
    }
}
inline void RecordMetadataRejection(humanlike_blind_metadata_diagnostics::Stage stage,void*attempt=nullptr){
    namespace d=humanlike_blind_metadata_diagnostics;
    if(!engine::image||GetCurrentThreadId()!=original_players::gameThread)return;
    d::Snapshot value{};value.stage=stage;
    __try {CaptureMetadataImpl(value,attempt);}
    __except(EXCEPTION_EXECUTE_HANDLER){value.stage=d::Stage::CaptureFault;}
    d::Assess(value);
    AcquireSRWLockExclusive(&metadataLock);value.serial=++metadataDiagnostics.captures;
    metadataDiagnostics.mask|=value.mismatches;metadataDiagnostics.last=value;ReleaseSRWLockExclusive(&metadataLock);
}
inline std::string MetadataJson(){
    // Writer thread copies POD only. No UObject, FName or native query here.
    AcquireSRWLockShared(&metadataLock);const auto value=metadataDiagnostics;ReleaseSRWLockShared(&metadataLock);
    return humanlike_blind_metadata_diagnostics::Json(value.last,value.captures,value.mask);
}

inline bool Metadata(){
    namespace d=humanlike_blind_metadata_diagnostics;
    if(state.method&&!Live(state.method,state.methodId)){
        state.method=state.owner=state.returnField=nullptr;state.methodId={};state.ownerId={};state.expectedReturnName={};state.expectedReturnNameKnown=false;++state.generationRejects;
    }
    const auto reject=[](d::Stage reason){RecordMetadataRejection(reason);state.method=state.owner=state.returnField=nullptr;state.methodId={};state.ownerId={};state.expectedReturnName={};state.expectedReturnNameKnown=false;return false;};
    if(!state.method){
        auto method=engine::Find(QueryPath);if(!method){RecordMetadataRejection(d::Stage::FunctionMissing);return false;}
        if(!original_inventory_slots::CaptureIdentity(method,state.methodId)){RecordMetadataRejection(d::Stage::FunctionWeakCapture,method);return false;}
        state.method=method;state.owner=Read<void*>(method,0x28);
        if(!state.owner||!original_inventory_slots::CaptureIdentity(state.owner,state.ownerId))return reject(d::Stage::OwnerWeakCapture);
        struct Name {uint32_t words[3]{};} name;
        engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&name,L"ReturnValue",0);
        for(size_t n=0;n<state.expectedReturnName.size();++n)state.expectedReturnName[n]=name.words[n];
        state.expectedReturnNameKnown=name.words[0]!=0||name.words[1]!=0;
        if(!name.words[0]&&!name.words[1])return reject(d::Stage::NameLookup);
        state.returnField=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(method,&name,3);
        if(!state.returnField||Read<uint64_t>(state.returnField,0x20)!=Read<uint64_t>(&name))return reject(d::Stage::ReturnLookup);
    }
    const bool valid=Live(state.method,state.methodId)&&Live(state.owner,state.ownerId)
        &&Read<void*>(state.method,0x28)==state.owner&&state.owner==engine::Class(ManagerClass)
        &&Read<uint16_t>(state.method,0xbe)==1&&Read<uint32_t>(state.method,0xb8)==0x54020401U
        &&Read<void*>(state.method,0xe0)==engine::function<void*>(QueryExec)&&Read<int32_t>(state.method,0x60)>=1&&ReturnField();
    return valid||reject(d::Stage::FinalGate);
}
inline bool ObjectWorld(void*object,uintptr_t expected,void*world){
    if(!object||Read<void*>(Read<void*>(object),0x1b8)!=engine::function<void*>(expected))return false;
    return engine::function<void*(*)(void*)>(expected)(object)==world;
}
inline bool Snapshot(size_t index,void*pawn,const Weak&pawnId,uint32_t epoch,void*world,const Weak&worldId,Cache&cache,core::Snapshot&out){
    out={};const auto*p=local_server_match::Player(index);
    if(!p||!p->present||p->human||p->pawn!=pawn||p->epoch!=epoch||!original_inventory_slots::SameIdentity(p->pawnIdentity,pawnId)
       ||!Live(pawn,pawnId)||!original_players::IsA(pawn,original_players::ShooterCharacterClass)){
        ++state.lifeRejects;return false;
    }
    auto manager=Read<void*>(pawn,0xa28);
    if(!manager||!original_players::IsA(manager,ManagerClass)||Read<void*>(manager,0xc8)!=pawn){++state.sourceRejects;return false;}
    if(cache.manager!=manager||!Live(cache.manager,cache.managerId)){
        cache={};cache.manager=manager;
        if(!original_inventory_slots::CaptureIdentity(manager,cache.managerId)||!Live(manager,cache.managerId)){++state.generationRejects;return false;}
    }
    if(!Live(world,worldId)||local_server_match::GetState().world!=world
       ||!ObjectWorld(manager,0x2a64ee0,world)||!ObjectWorld(pawn,0x3ed83b0,world)){++state.worldRejects;return false;}
    if(!Metadata()){++state.metadataRejects;return false;}
    const auto role=Read<uint8_t>(pawn,0x224);if(role>3){++state.sourceRejects;return false;}
    const size_t selected=role==1||role==2?0x118:0x100;
    const int32_t count=Read<int32_t>(manager,selected+8),capacity=Read<int32_t>(manager,selected+12);
    if(count<0||capacity<count||capacity>4096||(count&&!Read<void*>(manager,selected))){++state.sourceRejects;return false;}
    out.valid=out.profiles=out.metadata=out.worldKnown=true;
    out.index=static_cast<uint32_t>(index);out.epoch=epoch;out.pawn=reinterpret_cast<uintptr_t>(pawn);
    out.manager=reinterpret_cast<uintptr_t>(manager);out.world=reinterpret_cast<uintptr_t>(world);out.function=reinterpret_cast<uintptr_t>(state.method);
    out.pawnId=Id(pawnId);out.managerId=Id(cache.managerId);out.worldId=Id(worldId);out.functionId=Id(state.methodId);
    out.role=role;out.activeCount=count;out.capacity=capacity;return true;
}
inline core::Reading SampleImpl(size_t index,void*pawn,const Weak&pawnId,uint32_t epoch,void*world,const Weak&worldId,Cache&cache){
    if(!engine::image||GetCurrentThreadId()!=original_players::gameThread||index==0||index>=32||!Profiles())return {};
    core::Snapshot before{},after{};
    if(!Snapshot(index,pawn,pawnId,epoch,world,worldId,cache,before))return {};
    // Use the actual object's original ProcessEvent entry and unchanged typed
    // one-byte parameter storage; no fabricated FFrame or combat diagnostics.
    const auto process=Read<void*>(Read<void*>(cache.manager),0x298);if(!process)return {};
    uint8_t result=255;++state.queries;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(cache.manager,state.method,&result);
    const bool completed=true;
    if(!Snapshot(index,pawn,pawnId,epoch,world,worldId,cache,after))return {};
    return core::Evaluate(before,after,completed,result);
}
inline int Capture(EXCEPTION_POINTERS*e){
    ++state.faults;state.lastException=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0007776U;
    const auto at=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    state.lastExceptionRva=at>=engine::image?at-engine::image:at;return EXCEPTION_EXECUTE_HANDLER;
}
inline core::Reading Sample(size_t index,void*pawn,const Weak&pawnId,uint32_t epoch,void*world,const Weak&worldId,Cache&cache){
    ++state.samples;core::Reading out{};
    __try {out=SampleImpl(index,pawn,pawnId,epoch,world,worldId,cache);}
    __except(Capture(GetExceptionInformation())){}
    if(out.known){++state.known;if(out.active)++state.active;}else ++state.unknown;
    return out;
}
} // namespace local_server_humanlike_blind
