#pragma once
#include "local_server_hero_abilities.h"
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>

// Candidate HOST policy for the removed AbilitySubequippable grant/delete
// bodies. Only real, current registered-hero source components are admitted.
// The configured native class/slot, original factories, owner callbacks,
// reflected object setters, RepNotify and native history producer are reused.
// No client factory, synthetic FFrame, Role/clock/ACK/driver/NetGUID is supplied.
// Actual retail server inventory policy is not recovered: publishing the real
// authored slot13 child is an explicit local-host design, not a retail claim.
// Evidence: work/local-server-playable/hero-abilities/group1*.json/.md.
namespace local_server_ability_subequippable {
namespace heroes=local_server_hero_abilities;
namespace loadout=local_server_starting_equipment;
namespace resource=local_server_resource_authority;
using Weak=original_inventory_slots::WeakIdentity;
using SourceView=heroes::SourceView;
using ExecFn=void(*)(void*,void*,void*);
using SourceAllowed=bool(*)(const SourceView&);
inline constexpr size_t MaxRecords=64;
inline constexpr uintptr_t ComponentClass=0x63813f0;
inline constexpr size_t BackpackSlot=13;

struct FieldHistory {
    bool initial=false,prepared=false,deferred=false,published=false,handleObserved=false,terminator=false;
    void *connection=nullptr,*channel=nullptr,*replicator=nullptr,*layout=nullptr,*manager=nullptr,*send=nullptr;
    void *property=nullptr,*parent=nullptr,*command=nullptr,*shadow=nullptr,*before=nullptr,*after=nullptr,*objectGetter=nullptr;
    Weak channelId{};
    uint32_t flags=0,frame=0,historyStart=0,historyEnd=0,compare=0,sendHistory=0,sendCompare=0;
    uint16_t handle=0,commandIndex=0;int32_t shadowOffset=-1;
    const char* stage="subequippable_reference_history_not_requested";
};
struct Record {
    bool used=false,retired=false,ready=false,initialRequestSent=false,grantRequested=false,deleteRequested=false;
    bool grantInFlight=false,deleteInFlight=false,notifyInFlight=false,referenceNotified=false,deleteNotified=false,unequipRequested=false,ammoPrepared=false;
    bool componentRoute=false;
    SourceView source{};
    loadout::Player context{};
    loadout::Starter child{};
    heroes::Item graph{};
    void *component=nullptr,*ability=nullptr,*defaults=nullptr;
    Weak componentId{},abilityId{},defaultsId{};
    uint64_t generation=0,grants=0,deletes=0,grantRequests=0,deleteRequests=0,notifications=0,ammoRegistration=0;
    uint64_t deleteAt=0;
    unsigned error=0;
    FieldHistory history{};
    const char* stage="subequippable_component_not_registered";
};
struct State {
    bool initialized=false,host=false,installed=false,profiles=false,schema=false,restartRequired=false,closed=false;
    bool grantRestored=false,deleteRestored=false,guardConfigured=false;
    void *world=nullptr;Weak worldId{};DWORD gameThread=0;
    unsigned error=0,records=0,readyRecords=0;
    uint32_t execInFlight=0;
    uint64_t registrations=0,retirements=0,grantExecs=0,deleteExecs=0,originalForwards=0;
    uint64_t codeParameters=0,compiledParameters=0,originalFactories=0,originalDeletes=0;
    uint64_t fieldComparisons=0,fieldHistoryWitnesses=0,fieldDeferrals=0,lastScanAt=0,componentScans=0;
    uint64_t guardQueries=0,guardDeferrals=0;
    uintptr_t exceptionRva=0;
    const char* stage="subequippable_host_not_initialized";
};
struct Row {
    bool used=false,retired=false,ready=false,grantRequested=false,deleteRequested=false,published=false,ammoPrepared=false;
    void *component=nullptr,*ability=nullptr,*pawn=nullptr,*item=nullptr,*configuredClass=nullptr;
    uint32_t epoch=0;unsigned hero=0,error=0;uint8_t slot=255,role=255,remoteRole=255;
    uint64_t generation=0,grants=0,deletes=0,notifications=0;
    const char* stage="not_sampled";FieldHistory history{};
};
struct Diagnostics {State summary{};std::array<Row,MaxRecords> rows{};};
struct FunctionBinding {void* function=nullptr;Weak identity{};ExecFn original=nullptr,replacement=nullptr;};
inline State state{};
inline std::array<Record,MaxRecords> records{};
// The shared graph helpers need a real source context, not their64-item player
// cache. Keep one guarded scratch instance in BSS, never on the game stack.
inline heroes::Player graphContext{};
inline bool graphBusy=false;
inline Diagnostics cached{};
inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline FunctionBinding grantBinding{},deleteBinding{};
inline SourceAllowed sourceAllowed=nullptr;
inline void *notify=nullptr,*getter=nullptr,*grantParameter=nullptr,*subequippableProperty=nullptr;
inline loadout::Name subequippableName{};
inline const State& GetState(){return state;}
template<class T> inline T Read(const void* p,size_t at=0){return original_players::Value<T>(p,at);}
inline bool Live(void* p,const Weak& id){return p&&original_inventory_slots::ResolveIdentity(id)==p;}
inline bool Capture(void* p,Weak& id){return original_inventory_slots::CaptureIdentity(p,id)&&Live(p,id);}
inline bool Thread(){return state.initialized&&state.host&&!state.restartRequired&&original_players::Thread()
    &&state.gameThread==GetCurrentThreadId()&&Live(state.world,state.worldId);}
inline bool Fail(unsigned code,const char* stage){state.error=code;state.stage=stage;return false;}
inline bool Fail(Record& r,unsigned code,const char* stage){r.error=code;r.ready=false;r.stage=stage;return Fail(code,stage);}
inline bool Wait(Record& r,const char* stage){r.ready=false;r.stage=stage;state.stage=stage;return true;}
inline bool NativeResult(Record& r,bool result){if(result)return true;
    const auto& s=loadout::GetState();return Fail(r,r.context.error?r.context.error:(s.error?s.error:6200),
        r.context.error?r.context.stage:(s.error?s.stage:"subequippable_original_loadout_helper_failed"));}
inline void Cache(){
    AcquireSRWLockExclusive(&cacheLock);cached.summary=state;
    for(size_t i=0;i<records.size();++i){const auto& r=records[i];auto& d=cached.rows[i];d={};
        d.used=r.used;d.retired=r.retired;d.ready=r.ready;d.grantRequested=r.grantRequested;d.deleteRequested=r.deleteRequested;
        d.component=r.component;d.ability=r.ability;d.pawn=r.source.pawn;d.item=r.child.actor;d.configuredClass=r.child.type;
        d.epoch=r.source.sourceEpoch;d.hero=r.source.heroIndex;d.error=r.error;d.slot=r.child.slotType;
        d.published=r.child.published;d.ammoPrepared=r.ammoPrepared;d.role=r.child.role;d.remoteRole=r.child.remoteRole;
        d.generation=r.generation;d.grants=r.grants;d.deletes=r.deletes;d.notifications=r.notifications;d.stage=r.stage;d.history=r.history;
    }
    ReleaseSRWLockExclusive(&cacheLock);
}
inline Diagnostics Snapshot(){AcquireSRWLockShared(&cacheLock);auto result=cached;ReleaseSRWLockShared(&cacheLock);return result;}
inline bool ProofAlive(const SourceView& source,unsigned depth=0);
inline bool ResolveOwnedSubequippable(void* object,SourceView& out);
// Additional coordinator admission only, never a replacement for ProofAlive.
// Ownership-preserving delete/retirement deliberately use the original raw
// source proof even while new grants are fenced or Close has begun.
inline bool GrantAllowed(const SourceView& source){
    if(!Thread()||state.error||state.closed||!sourceAllowed)return false;
    ++state.guardQueries;
    const bool allowed=sourceAllowed(source);if(!allowed)++state.guardDeferrals;return allowed;
}
inline bool NativeRecord(const Record& r){
    return r.used&&!r.retired&&!r.error&&Live(r.component,r.componentId)&&Live(r.ability,r.abilityId)
        &&Read<void*>(r.component,0x10)==engine::Class(ComponentClass)&&Read<void*>(r.component,0xc8)==r.ability
        &&Read<void*>(r.ability,0x210)==r.source.pawn&&Read<void*>(r.ability,0x4a8)==r.source.pawn
        &&Read<uint8_t>(r.ability,0x224)==3&&local_server_item_ownership::ActorWorld(r.ability)==r.source.world;
}
inline bool ChildProof(const Record& r,void* object,SourceView& out){
    if(!NativeRecord(r)
       ||!Live(r.child.actor,r.child.actorId)||!Live(r.child.type,r.child.typeId)
       ||!Live(r.defaults,r.defaultsId)||Read<void*>(r.child.type,0x120)!=r.defaults
       ||Read<uint8_t>(r.defaults,0x4a0)!=BackpackSlot||Read<uint8_t>(r.child.actor,0x4a0)!=BackpackSlot
       ||r.child.slotType!=BackpackSlot||Read<void*>(r.source.inventory,0x1b8+BackpackSlot*8)!=r.child.slot
       ||!Live(r.child.slot,r.child.slotId)||!local_server_item_ownership::ValidSlot(r.source.inventory,r.child.slot,BackpackSlot)
       ||Read<void*>(r.component,0xc8)!=r.ability||Read<void*>(r.component,0xe0)!=r.child.type
       ||Read<uint8_t>(r.child.actor,0x224)!=3||Read<void*>(r.child.actor,0x210)!=r.source.pawn
       ||!original_players::Derived(Read<void*>(r.child.actor,0x10),r.child.type)
       ||local_server_item_ownership::ActorWorld(r.child.actor)!=r.source.world)return false;
    const bool pending=r.grantInFlight&&r.child.spawned&&!r.child.published;
    if(!pending&&(Read<void*>(r.component,0xf8)!=r.child.actor||!loadout::Contains(r.child)
       ||Read<void*>(r.child.actor,0x4a8)!=r.source.pawn))return false;
    const bool direct=object==r.child.actor;
    const bool component=original_players::IsA(object,0x36837a0)&&Read<void*>(object,0xc8)==r.child.actor;
    if(!direct&&!component)return false;Weak id{};if(!Capture(object,id))return false;
    out=r.source;out.contextObject=object;out.identities[0]=id;out.pendingGrant=pending;return true;
}
inline bool SameSource(const SourceView& a,const SourceView& b){
    if(a.contextObject!=b.contextObject||a.world!=b.world||a.controller!=b.controller||a.playerState!=b.playerState
       ||a.pawn!=b.pawn||a.inventory!=b.inventory||a.ability!=b.ability||a.heroIndex!=b.heroIndex
       ||a.sourceEpoch!=b.sourceEpoch||a.registration!=b.registration)return false;
    for(size_t i=0;i<a.identities.size();++i)if(i!=6||a.ability)
        if(!original_inventory_slots::SameIdentity(a.identities[i],b.identities[i]))return false;
    return true;
}
inline bool ProofAlive(const SourceView& source,unsigned depth){
    if(!Thread()||depth>=8||!source.sourceEpoch||!source.registration)return false;
    if(heroes::ValidateSource(source))return true;
    for(const auto& r:records){SourceView after{};
        if(r.used&&!r.retired&&ChildProof(r,source.contextObject,after)&&SameSource(source,after)
           &&ProofAlive(r.source,depth+1))return true;}
    return false;
}
inline bool ResolveOwnedSubequippable(void* object,SourceView& out){
    out={};if(!Thread()||state.error||state.closed||!object)return false;
    for(const auto& r:records)if(r.used&&!r.retired&&ProofAlive(r.source)&&ChildProof(r,object,out)
        &&GrantAllowed(out))return true;
    out={};return false;
}
inline bool ValidateOwnedSource(const SourceView& source){SourceView current{};
    return ResolveOwnedSubequippable(source.contextObject,current)&&SameSource(source,current);}
inline Record* Find(void* component){for(auto& r:records)if(r.used&&!r.retired&&r.component==component&&Live(component,r.componentId))return &r;return nullptr;}
inline bool Source(Record& r){
    if(!Thread()||!ProofAlive(r.source)||!NativeRecord(r))return false;
    return r.source.world==state.world&&NativeResult(r,loadout::Source(r.context))&&r.context.sourceReady;
}
inline bool ExecutableImage(const void* p){MEMORY_BASIC_INFORMATION m{};
    if(!p||!VirtualQuery(p,&m,sizeof(m))||m.State!=MEM_COMMIT||m.Type!=MEM_IMAGE
       ||m.AllocationBase!=reinterpret_cast<void*>(engine::image)||(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
    const auto mode=m.Protect&0xff;return mode==PAGE_EXECUTE||mode==PAGE_EXECUTE_READ||mode==PAGE_EXECUTE_READWRITE||mode==PAGE_EXECUTE_WRITECOPY;
}
struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
inline constexpr Profile Profiles[]{
    {0x36d8d20,204,0x544a8d84897aceb7ULL},{0x3b866f0,20,0x09fe87de40670e75ULL},
    {0x1c75610,158,0x53040a68dd75221cULL},{0x63816d0,81,0xd58e4fd1b166ab55ULL},
    {0x485c950,32,0x4a42c757f42524ddULL},{0x6388010,133,0x54d1ab3d0271fc2eULL},
    {0x6eef150,272,0x8f39b3dfd82f4374ULL},{0x4516e40,579,0xae8f58fae2d3dff1ULL},
    {0x1b80dc0,21,0x94ec83e60996bb32ULL},{0x1ca8ca0,158,0x59d24cfb91d395ceULL},
    {0x1cad5f0,158,0x18688069f6764b7bULL}};
inline bool VerifyProfiles(){
    if(state.profiles)return true;
    if(!loadout::VerifyProfiles()||!loadout::VerifySchema()||!resource::VerifyScalarTrackerProfiles())return false;
    for(const auto& p:Profiles){auto code=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(code,p.bytes))return Fail(6201,"subequippable_original_profile_unreadable");
        uint64_t hash=0xcbf29ce484222325ULL;for(size_t n=0;n<p.bytes;++n)hash=(hash^code[n])*0x100000001b3ULL;
        if(hash!=p.fnv)return Fail(6202,"subequippable_original_profile_mismatch");}
    state.profiles=true;return true;
}
inline bool VerifyFunction(void*& function,const wchar_t* path,uint16_t bytes,uintptr_t exec){
    function=engine::Find(path);
    return function&&Read<void*>(function,0x28)==engine::Class(ComponentClass)
        &&Read<uint16_t>(function,0xbe)==bytes&&Read<uint32_t>(function,0xb8)==0x80401U
        &&Read<uintptr_t>(function,0xe0)==engine::image+exec;
}
inline bool PlainObjectProperty(void* property);
inline bool VerifySchema(){
    if(state.schema)return true;
    void *grant=nullptr,*remove=nullptr;original_jett_priming::PropertyInfo parameter{},reference{};
    auto type=engine::Class(ComponentClass);
    if(!VerifyFunction(grant,L"/Script/ShooterGame.AbilitySubequippableComponent.AuthGrantSubequippable",8,0x36d8d20)
       ||!VerifyFunction(remove,L"/Script/ShooterGame.AbilitySubequippableComponent.AuthDeleteSubequippable",0,0x3b866f0)
       ||!VerifyFunction(notify,L"/Script/ShooterGame.AbilitySubequippableComponent.OnRep_Subequippable",0,0x63816d0)
       ||!loadout::Field(type,L"SubequippableClass",0xe0,8,1,0)
       ||!loadout::Field(type,L"OnSubequippableGranted",0xe8,16,1,0)
       ||original_jett_priming::FieldOffset(type,L"Subequippable",8,&reference)!=0xf8
       ||reference.arrayDim!=1||reference.elementSize!=8||!PlainObjectProperty(reference.field)
       ||(Read<uint64_t>(reference.field,0x38)&0x100000020ULL)!=0x100000020ULL
       ||Read<uint16_t>(reference.field,0x40)!=2||Read<void*>(reference.field,0x78)!=engine::Class(0x645c660)
       ||original_jett_priming::FieldOffset(grant,L"OwningCharacter",8,&parameter)!=0
       ||parameter.arrayDim!=1||parameter.elementSize!=8||!PlainObjectProperty(parameter.field)
       ||(Read<uint64_t>(parameter.field,0x38)&0x180ULL)!=0x80ULL
       ||Read<void*>(parameter.field,0x78)!=engine::Class(original_players::ShooterCharacterClass))
        return Fail(6203,"subequippable_exact_original_function_or_property_schema_mismatch");
    getter=engine::Find(L"/Script/ShooterGame.AbilitySubequippableComponent.GetSubequippable");
    if(!getter||Read<void*>(getter,0x28)!=type||Read<uint16_t>(getter,0xbe)!=8
       ||Read<uintptr_t>(getter,0xe0)!=engine::image+0x485c950)
        return Fail(6204,"subequippable_original_getter_metadata_mismatch");
    grantBinding.function=grant;deleteBinding.function=remove;grantParameter=parameter.field;subequippableProperty=reference.field;
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&subequippableName,L"Subequippable",1);
    state.schema=true;return true;
}
inline bool PrepareField(Record& r,void* before);
inline bool CommitField(Record& r,void* actual);
inline bool EnsureGrant(Record& r);
inline bool EnsureDelete(Record& r);

// Mirrors the pinned generated stub's one real FFrame parameter parse. It does
// not replay the old exec, construct a frame or read consumed bytecode twice.
// Unknown/unregistered calls go through the saved original exec instead.
inline bool CanParse(void* frame){
    if(!frame||!original_players::Readable(frame,0x90))return false;
    auto code=Read<uint8_t*>(frame,0x20);
    if(code){if(!original_players::Readable(code,1))return false;
        auto step=Read<void*>(reinterpret_cast<void*>(engine::image+0xd747c40),size_t(*code)*8);
        return Read<void*>(frame,0x18)&&ExecutableImage(step);}
    return Read<void*>(frame,0x88)==grantParameter&&Read<void*>(frame,0x28)
        &&ExecutableImage(Read<void*>(Read<void*>(grantParameter),0x110));
}
inline void* ParseOwner(void* frame){
    void* owner=nullptr;(void)engine::function<void*(*)()>(0x1c75610)();
    auto code=Read<uint8_t*>(frame,0x20);
    if(code){const auto opcode=*code;++code;original_players::Write(frame,0x20,code);
        auto step=Read<void*>(reinterpret_cast<void*>(engine::image+0xd747c40),size_t(opcode)*8);
        reinterpret_cast<ExecFn>(step)(Read<void*>(frame,0x18),frame,&owner);++state.codeParameters;
    }else{
        auto property=Read<void*>(frame,0x88);auto next=Read<void*>(property,0x18);auto locals=Read<uint8_t*>(frame,0x28);
        auto address=locals+Read<int32_t>(property,0x44);auto copy=Read<void*>(Read<void*>(property),0x110);
        original_players::Write(frame,0x88,next);original_players::Write(frame,0x40,locals);original_players::Write(frame,0x38,address);
        reinterpret_cast<void(*)(void*,void*,const void*)>(copy)(property,&owner,address);++state.compiledParameters;
    }
    code=Read<uint8_t*>(frame,0x20);if(code){++code;original_players::Write(frame,0x20,code);}return owner;
}
inline void GrantExec(void* object,void* frame,void* result){
    auto r=Thread()&&!state.error&&!state.closed?Find(object):nullptr;
    if(!r||!Source(*r)||!GrantAllowed(r->source)||!CanParse(frame)){
        if(GetCurrentThreadId()==state.gameThread)++state.originalForwards;
        grantBinding.original(object,frame,result);return;}
    ++state.execInFlight;++state.grantExecs;
    __try {
        const auto before=r->source;const auto componentId=r->componentId;
        auto owner=ParseOwner(frame);
        // Parameter expressions can reenter native lifecycle code. A declined
        // policy after P_FINISH retains the original no-op body; it is never
        // replayed or parsed again, and cannot queue a new authority action.
        if(!SameSource(before,r->source)||!original_inventory_slots::SameIdentity(componentId,r->componentId)){
            ++state.guardDeferrals;return; // Do not alter a reused record's new epoch.
        }
        if(!Source(*r)||!GrantAllowed(r->source)){
            if(!state.error)Wait(*r,"subequippable_evaluated_grant_deferred_after_source_or_policy_change");return;}
        if(owner!=r->source.pawn){Fail(*r,6205,"subequippable_native_grant_parameter_has_foreign_or_null_owner");return;}
        r->grantRequested=true;++r->grantRequests;
        if(!r->deleteRequested&&!r->grantInFlight)EnsureGrant(*r);
    }__finally {--state.execInFlight;Cache();}
}
inline void DeleteExec(void* object,void* frame,void* result){
    auto r=Thread()&&!state.error?Find(object):nullptr;
    if(!r||!Source(*r)){
        if(GetCurrentThreadId()==state.gameThread)++state.originalForwards;
        deleteBinding.original(object,frame,result);return;}
    ++state.execInFlight;++state.deleteExecs;
    __try {
        // Zero parameters: the exact original P_FINISH is executed once.
        deleteBinding.original(object,frame,result);const bool alreadyRequested=r->deleteRequested;
        r->deleteRequested=true;r->grantRequested=false;++r->deleteRequests;
        if(!alreadyRequested)r->deleteAt=GetTickCount64();
        if(!r->grantInFlight&&!r->deleteInFlight&&!r->notifyInFlight)EnsureDelete(*r);
    }__finally {--state.execInFlight;Cache();}
}
inline bool WritableExec(void* function){
    MEMORY_BASIC_INFORMATION m{};auto p=static_cast<uint8_t*>(function)+0xe0;
    if(!VirtualQuery(p,&m,sizeof(m))||m.State!=MEM_COMMIT||m.Type!=MEM_PRIVATE||(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
    auto mode=m.Protect&0xff;
    return (mode==PAGE_READWRITE||mode==PAGE_EXECUTE_READWRITE)
        &&reinterpret_cast<uintptr_t>(p)+8<=reinterpret_cast<uintptr_t>(m.BaseAddress)+m.RegionSize;
}
inline bool Install(FunctionBinding& b,ExecFn replacement,uintptr_t expected){
    if(!b.function||Read<uintptr_t>(b.function,0xe0)!=engine::image+expected||!WritableExec(b.function)||!Capture(b.function,b.identity))
        return Fail(6206,"subequippable_exec_binding_is_not_original_writable_private_ufunction");
    b.original=reinterpret_cast<ExecFn>(engine::image+expected);b.replacement=replacement;
    return InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(static_cast<uint8_t*>(b.function)+0xe0),
        reinterpret_cast<void*>(replacement),reinterpret_cast<void*>(b.original))==reinterpret_cast<void*>(b.original)
        ||Fail(6207,"subequippable_original_native_exec_publication_conflict");
}
inline bool PlainObjectProperty(void* property){
    auto object=engine::function<void*(*)()>(0x1c75610)();
    auto soft=engine::function<void*(*)()>(0x1ca8ca0)();
    auto weak=engine::function<void*(*)()>(0x1cad5f0)();
    if(!object||!soft||!weak||!Read<uint64_t>(object,0x10)||!Read<uint64_t>(soft,0x10)||!Read<uint64_t>(weak,0x10))return false;
    auto is=engine::function<bool(*)(void*,void*)>(0x1b80dc0);
    return is(property,object)&&!is(property,soft)&&!is(property,weak);
}
inline bool ComponentRoute(Record& r){
    r.componentRoute=false;
    if(!NativeRecord(r)||(Read<uint8_t>(r.component,0xa0)&0x20)==0
       ||Read<void*>(Read<void*>(r.component),0x220)!=reinterpret_cast<void*>(engine::image+0x6eef150)
       ||Read<void*>(Read<void*>(r.component),0x318)!=reinterpret_cast<void*>(engine::image+0x4131210)
       ||Read<void*>(Read<void*>(r.component),0x320)!=reinterpret_cast<void*>(engine::image+0x2a64f70))
        return Fail(r,6208,"subequippable_original_component_replication_policy_mismatch");
    const auto replicated=Read<resource::Array>(r.ability,0x330),registered=Read<resource::Array>(r.ability,0x320);
    if(!resource::ValidArray(replicated,8,512)||!resource::ValidArray(registered,0x28,512))
        return Fail(r,6209,"subequippable_original_owner_component_registry_invalid");
    unsigned references=0;
    if(Read<uint8_t>(r.ability,0x11b)&1){
        for(int32_t n=0;n<registered.count;++n){auto row=registered.data+size_t(n)*0x28;if(Read<void*>(row)!=r.component)continue;
            const auto condition=Read<int32_t>(row,0x10);
            if(!Live(r.component,Read<Weak>(row,8))||(condition!=0&&condition!=2&&condition!=7&&condition!=13))
                return Fail(r,6210,"subequippable_original_registered_component_identity_or_condition_unsupported");
            ++references;}
    }else for(int32_t n=0;n<replicated.count;++n)if(Read<void*>(replicated.data,size_t(n)*8)==r.component)++references;
    r.componentRoute=references==1;
    return r.componentRoute||Fail(r,6211,"subequippable_original_component_is_not_uniquely_registered");
}
inline bool DeferField(Record& r,const char* stage){
    r.history.deferred=true;r.history.stage=stage;++state.fieldDeferrals;return Wait(r,stage);
}
inline void* FieldObject(const FieldHistory& h,const void* address){
    return reinterpret_cast<void*(*)(void*,const void*)>(h.objectGetter)(h.property,address);
}
inline bool PrepareField(Record& r,void* before){
    auto& h=r.history;h={};h.before=before;h.property=subequippableProperty;
    if(!Source(r)||!ComponentRoute(r)||Read<void*>(r.component,0xf8)!=before||!PlainObjectProperty(h.property))
        return Fail(r,6212,"subequippable_reference_source_or_plain_object_property_changed");
    h.connection=Read<void*>(r.source.controller,0x510);
    const auto channels=Read<resource::Array>(h.connection,0x78);
    if(!resource::ValidArray(channels,8,512))return Fail(r,6213,"subequippable_original_channel_array_invalid");
    unsigned matches=0;
    for(int32_t n=0;n<channels.count;++n){auto channel=Read<void*>(channels.data,size_t(n)*8);
        if(channel&&original_players::IsA(channel,0x3dd3d40)&&Read<void*>(channel,0x30)==h.connection
           &&Read<void*>(channel,0x78)==r.ability){h.channel=channel;++matches;}}
    h.frame=Read<uint32_t>(Read<void*>(h.connection,0x60),0x4d8);
    // Absence could also mean a retired/dormant channel. Never manufacture an
    // initial classification. Wait for the parent's real null-reference route
    // to be established before constructing/publishing any new child firearm.
    if(!matches)return DeferField(r,"subequippable_reference_waiting_actual_parent_actor_channel");
    if(matches!=1||Read<uint8_t>(h.connection,0x1730)!=0||(Read<uint8_t>(h.channel,0x38)&2)||!Capture(h.channel,h.channelId))
        return Fail(r,6214,"subequippable_reference_original_channel_or_transport_mode_unsupported");
    if(!(Read<uint8_t>(h.channel,0xa0)&1))return DeferField(r,"subequippable_reference_waiting_actual_channel_open_ack");
    auto shared=engine::function<void**(*)(void*,void*)>(0x42e59d0)(h.channel,r.component);
    h.replicator=Read<void*>(shared);
    if(!h.replicator)return DeferField(r,"subequippable_reference_waiting_actual_component_replicator");
    if(!original_players::Readable(h.replicator,0xe0)||Read<void*>(h.replicator,0x48)!=r.component
       ||Read<void*>(h.replicator,0x40)!=Read<void*>(r.component,0x10)||Read<void*>(h.replicator,0x58)!=h.channel
       ||Read<void*>(h.replicator,0x50)!=h.connection||!Live(r.component,Read<Weak>(h.replicator,0xd8)))
        return Fail(r,6215,"subequippable_reference_native_replicator_identity_mismatch");
    h.layout=Read<void*>(h.replicator,0x20);h.manager=Read<void*>(h.replicator,0x10);
    h.send=Read<void*>(Read<void*>(h.replicator,0x30),8);
    if(!h.layout||!h.manager||!h.send)return DeferField(r,"subequippable_reference_waiting_original_changelist_or_sending_state");
    if(!original_players::Readable(h.layout,0xf8)||!original_players::Readable(h.manager,0x840)
       ||!original_players::Readable(h.send,0x28)||Read<void*>(h.manager,0x830)!=h.layout
       ||(Read<uint8_t>(h.layout,0x20)&0x34U)||(Read<uint32_t>(h.replicator,0xc)&0x20U))
        return Fail(r,6216,"subequippable_reference_native_layout_or_effective_skip_mode_unproven");
    h.flags=Read<uint32_t>(h.send,0x1c);
    if(h.flags&0x80U)return Fail(r,6217,"subequippable_reference_roles_only_native_flags_unsupported");
    if(h.flags&2U)return DeferField(r,"subequippable_reference_waiting_noninitial_original_sending_flags");
    const auto parents=Read<resource::Array>(h.layout,0x28),commands=Read<resource::Array>(h.layout,0x38);
    if(!resource::ValidArray(parents,0x38,512)||parents.count<=2||!resource::ValidArray(commands,0x20,8192))
        return Fail(r,6218,"subequippable_reference_native_parent_or_command_arrays_invalid");
    auto parent=parents.data+2*0x38;const auto start=Read<uint16_t>(parent,0x20),end=Read<uint16_t>(parent,0x22);
    if(Read<void*>(parent)!=h.property||Read<int32_t>(parent,0x18)!=0xf8||Read<int32_t>(parent,0x24)!=0
       ||!(Read<uint32_t>(parent,0x30)&1U)||uint32_t(end)!=uint32_t(start)+1U||int32_t(end)>commands.count)
        return Fail(r,6219,"subequippable_reference_parent_is_not_actual_unconditional_single_object");
    h.parent=parent;h.commandIndex=start;h.command=commands.data+size_t(start)*0x20;h.handle=Read<uint16_t>(h.command,0x14);
    const auto shadowOffset=Read<int32_t>(h.command,0x10);
    h.shadowOffset=shadowOffset;
    // Original49D5EE0 classifies plain object properties as8, after excluding
    // native soft/weak properties.49CB44F compares original v190 object values.
    if(Read<void*>(h.command)!=h.property||Read<int32_t>(h.command,0xc)!=0xf8||Read<uint16_t>(h.command,0x16)!=2
       ||Read<uint8_t>(h.command,0x1c)!=8||!h.handle||Read<int32_t>(parent,0x1c)!=shadowOffset)
        return Fail(r,6220,"subequippable_reference_original_object_command_or_handle_mismatch");
    auto shadowData=Read<uint8_t*>(h.manager,0x820);const auto count=Read<int32_t>(h.manager,0x828),capacity=Read<int32_t>(h.manager,0x82c);
    if(!shadowData||shadowOffset<0||count<8||count>1048576||capacity<count||uint64_t(shadowOffset)+8U>uint64_t(count)
       ||!original_players::Readable(shadowData+shadowOffset,8))return Fail(r,6221,"subequippable_reference_original_shadow_allocation_invalid");
    h.shadow=shadowData+shadowOffset;h.objectGetter=Read<void*>(Read<void*>(h.property),0x190);
    if(!ExecutableImage(h.objectGetter)||FieldObject(h,static_cast<uint8_t*>(r.component)+0xf8)!=before)
        return Fail(r,6222,"subequippable_reference_actual_property_object_getter_mismatch");
    if(FieldObject(h,h.shadow)!=before)return DeferField(r,"subequippable_reference_waiting_current_original_object_shadow");
    if(auto active=Read<void*>(h.send,0x20)){auto bits=Read<const uint32_t*>(active,0x18);
        if(!bits)bits=reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(active)+8);
        if(!original_players::Readable(bits,4))return Fail(r,6223,"subequippable_reference_original_active_tracker_invalid");
        if((Read<uint32_t>(bits)&4U)==0)return DeferField(r,"subequippable_reference_waiting_original_active_parent");}
    h.historyStart=Read<uint32_t>(h.manager,0x810);h.historyEnd=Read<uint32_t>(h.manager,0x814);h.compare=Read<uint32_t>(h.manager,0x818);
    h.sendHistory=Read<uint32_t>(h.send,0x10);h.sendCompare=Read<uint32_t>(h.send,0x14);
    if(h.historyStart>h.historyEnd||h.historyEnd>=0x7fffffffU||h.compare>=0x7fffffffU||h.historyEnd-h.historyStart>=64U
       ||h.sendHistory<h.historyStart||h.sendHistory>h.historyEnd||h.sendCompare>h.compare)
        return Fail(r,6224,"subequippable_reference_original_history_bounds_invalid");
    if(h.historyEnd-h.historyStart==63U&&h.sendHistory==h.historyStart)
        return DeferField(r,"subequippable_reference_waiting_original_history_consumer");
    h.prepared=true;h.stage="subequippable_reference_original_object_shadow_and_history_prepared";return true;
}
inline bool CommitField(Record& r,void* actual){
    auto& h=r.history;
    if(h.deferred||!h.prepared)return Fail(r,6225,"subequippable_reference_commit_without_prepared_original_tracker");
    const auto currentParents=Read<resource::Array>(h.layout,0x28),currentCommands=Read<resource::Array>(h.layout,0x38);
    auto shadowData=Read<uint8_t*>(h.manager,0x820);const auto shadowCount=Read<int32_t>(h.manager,0x828),shadowCapacity=Read<int32_t>(h.manager,0x82c);
    if(!resource::ValidArray(currentParents,0x38,512)||currentParents.count<=2
       ||!resource::ValidArray(currentCommands,0x20,8192)||currentCommands.count<=h.commandIndex
       ||currentParents.data+2*0x38!=h.parent||currentCommands.data+size_t(h.commandIndex)*0x20!=h.command
       ||Read<void*>(h.command)!=h.property||Read<int32_t>(h.command,0x10)!=h.shadowOffset
       ||!shadowData||h.shadowOffset<0||shadowCount<8||shadowCount>1048576||shadowCapacity<shadowCount
       ||uint64_t(h.shadowOffset)+8U>uint64_t(shadowCount)||shadowData+h.shadowOffset!=h.shadow){
        state.restartRequired=true;return Fail(r,6295,"subequippable_reference_native_layout_or_shadow_allocation_changed_after_setter");}
    if(!Source(r)||!Live(h.channel,h.channelId)||Read<void*>(r.component,0xf8)!=actual||actual==h.before
       ||Read<void*>(h.replicator,0x48)!=r.component||Read<void*>(h.replicator,0x58)!=h.channel
       ||Read<void*>(h.replicator,0x20)!=h.layout||Read<void*>(h.replicator,0x10)!=h.manager
       ||Read<void*>(h.replicator,0x50)!=h.connection||Read<void*>(Read<void*>(h.replicator,0x30),8)!=h.send
       ||Read<void*>(h.manager,0x830)!=h.layout||Read<uint32_t>(h.send,0x1c)!=h.flags
       ||Read<uint32_t>(h.send,0x10)!=h.sendHistory||Read<uint32_t>(h.send,0x14)!=h.sendCompare
       ||Read<uint8_t>(h.connection,0x1730)!=0||FieldObject(h,h.shadow)!=h.before
       ||Read<uint32_t>(h.manager,0x814)!=h.historyEnd||Read<uint32_t>(h.manager,0x818)!=h.compare){
        state.restartRequired=true;return Fail(r,6226,"subequippable_reference_changed_after_actual_authority_commit_requires_stop");}
    ++state.fieldComparisons;
    const auto result=engine::function<int32_t(*)(void*,void*,void*,const void*,const uint32_t*,bool)>(0x49cbe90)
        (h.layout,h.send,static_cast<uint8_t*>(h.manager)+8,r.component,&h.flags,false);
    h.after=FieldObject(h,h.shadow);
    const auto changed=Read<resource::Array>(static_cast<uint8_t*>(h.manager)+0x10+size_t(h.historyEnd&63U)*0x20);
    if(resource::ValidArray(changed,2,65536)&&changed.count>=2){h.terminator=Read<uint16_t>(changed.data,size_t(changed.count-1)*2)==0;
        for(int32_t n=0;n<changed.count-1;++n)if(Read<uint16_t>(changed.data,size_t(n)*2)==h.handle){h.handleObserved=true;break;}}
    if(result!=0||h.after!=actual||Read<uint32_t>(h.manager,0x818)!=h.compare+1U||Read<uint32_t>(h.manager,0x814)!=h.historyEnd+1U
       ||!h.handleObserved||!h.terminator){state.restartRequired=true;return Fail(r,6227,"subequippable_reference_native_object_shadow_or_history_receipt_failed");}
    h.published=true;h.stage="subequippable_reference_native_object_handle_history_observed_delivery_unverified";
    ++state.fieldHistoryWitnesses;return true;
}
inline bool NotifyReference(Record& r){
    if(r.referenceNotified)return true;
    if(r.notifyInFlight)return Fail(r,6229,"subequippable_original_notification_reentrancy_conflict");
    r.notifyInFlight=true;bool delivered=false;
    __try {delivered=loadout::Event(r.component,L"/Script/ShooterGame.AbilitySubequippableComponent.OnRep_Subequippable",0,nullptr);}
    __finally {r.notifyInFlight=false;}
    if(!delivered)return Fail(r,6230,"subequippable_original_onrep_dispatch_failed");
    ++r.notifications;r.referenceNotified=true;return true;
}
inline bool SetReference(Record& r,void* actual,bool notifyNow=true){
    auto before=Read<void*>(r.component,0xf8);if(before==actual)return true;
    if(!PrepareField(r,before))return false;if(r.history.deferred)return true;
    loadout::ObjectSet set{r.component,subequippableName,0,actual};
    if(!loadout::Event(loadout::systemLibrary,L"/Script/Engine.KismetSystemLibrary.SetObjectPropertyByName",32,&set)
       ||Read<void*>(r.component,0xf8)!=actual){state.restartRequired=true;return Fail(r,6228,"subequippable_original_reflected_object_setter_readback_failed");}
    if(!CommitField(r,actual))return false;
    r.referenceNotified=false;return !notifyNow||NotifyReference(r);
}
inline bool Configured(Record& r){
    auto type=Read<void*>(r.component,0xe0);if(!type)return Wait(r,"subequippable_waiting_original_configured_class");
    if(r.child.type&&(r.child.type!=type||!Live(type,r.child.typeId)))
        return Fail(r,6231,"subequippable_original_configured_class_changed_before_child_retirement");
    if(!original_players::IsA(type,0x1af0eb0)||!original_players::Derived(type,engine::Class(0x645c660)))
        return Fail(r,6232,"subequippable_configured_class_is_not_original_equippable");
    auto defaults=Read<void*>(type,0x120);
    if(!defaults)return Wait(r,"subequippable_waiting_original_loaded_child_defaults");
    if(!original_players::IsA(defaults,0x645c660)||Read<void*>(defaults,0x10)!=type)
        return Fail(r,6233,"subequippable_original_child_default_object_type_mismatch");
    if(r.defaults&&(r.defaults!=defaults||!Live(defaults,r.defaultsId)))
        return Fail(r,6234,"subequippable_original_child_defaults_changed_before_retirement");
    const auto slot=Read<uint8_t>(defaults,0x4a0);
    if(slot!=BackpackSlot)return Fail(r,6235,"subequippable_authored_nonbackpack_child_slot_requires_source_policy");
    if(!r.child.type){r.child.type=type;if(!Capture(type,r.child.typeId))return Fail(r,6236,"subequippable_child_class_identity_capture_failed");}
    if(!r.defaults){r.defaults=defaults;if(!Capture(defaults,r.defaultsId))return Fail(r,6237,"subequippable_child_defaults_identity_capture_failed");}
    auto target=Read<void*>(r.source.inventory,0x1b8+BackpackSlot*8);
    if(!local_server_item_ownership::ValidSlot(r.source.inventory,target,BackpackSlot)
       ||!original_players::IsA(target,0x69cf3d0))return Wait(r,"subequippable_waiting_original_backpack_multi_slot");
    if(r.child.slot&&(r.child.slot!=target||!Live(target,r.child.slotId)))
        return Fail(r,6238,"subequippable_original_backpack_slot_changed_during_epoch");
    if(!r.child.slot){r.child.slot=target;if(!Capture(target,r.child.slotId))return Fail(r,6239,"subequippable_original_slot_identity_capture_failed");}
    r.child.slotType=uint8_t(BackpackSlot);r.child.multi=true;
    auto table=Read<void*>(target);
    if(Read<uintptr_t>(table,0x2f8)!=engine::image+0x18a76a0||Read<uintptr_t>(table,0x300)!=engine::image+0x18a76a0
       ||Read<uintptr_t>(table,0x328)!=engine::image+0x7132d50)
        return Fail(r,6240,"subequippable_original_backpack_dispatch_or_absent_authority_boundary_changed");
    return true;
}
inline bool ChildGraph(Record& r){
    if(graphBusy)return Wait(r,"subequippable_waiting_original_graph_initializer_return");
    graphBusy=true;bool okay=false;
    __try {
        graphContext.context=r.context;graphContext.used=true;graphContext.authority=true;graphContext.retired=false;
        graphContext.hero=r.source.heroIndex;graphContext.epoch=r.source.sourceEpoch;graphContext.error=0;
        r.graph.native=r.child;r.graph.defaults=r.defaults;r.graph.defaultsId=r.defaultsId;
        okay=heroes::CommonEquip(graphContext,r.graph)&&heroes::Graph(graphContext,r.graph);
        r.child=r.graph.native;
    }__finally {graphBusy=false;}
    if(!okay){const auto& shared=heroes::GetState();return Fail(r,shared.error?shared.error:6241,
        shared.error?shared.stage:"subequippable_original_common_equip_or_machine_graph_failed");}
    r.child.eventReady=r.graph.commonEquipRegistered;
    if(!r.graph.commonEquipRegistered||!r.graph.graphReady)return Wait(r,"subequippable_waiting_original_common_equip_and_state_graph");
    // Reuse only an actual authored AutoEquip response when one is present.
    // A different authored Equip0 graph is preserved without adding a target.
    auto machine=Read<void*>(r.child.actor,0xe00),inactive=Read<void*>(machine,0x350);
    auto entries=Read<uint8_t*>(inactive,0x360);const auto count=Read<int32_t>(inactive,0x368),capacity=Read<int32_t>(inactive,0x36c);
    if(count<0||count>128||capacity<count||(count&&!original_players::Readable(entries,size_t(count)*0x60)))
        return Fail(r,6242,"subequippable_original_inactive_equip_response_array_invalid");
    bool autoResponse=false,equipResponse=false;
    for(int32_t n=0;n<count;++n){const auto kind=Read<uint8_t>(entries,size_t(n)*0x60);autoResponse|=kind==2;equipResponse|=kind==0;}
    if(autoResponse){original_weapon::EquipEventRegistration receipt;
        if(!original_weapon::SupplyOriginalLocalEquipResponse(r.child.actor,receipt))
            return Fail(r,6243,"subequippable_authored_auto_equip_response_cannot_be_reused");
    }else if(!equipResponse)return Wait(r,"subequippable_waiting_original_authored_equip_response");
    return true;
}
inline bool EnsureGrantBody(Record& r){
    if(!Source(r))return Fail(r,6244,"subequippable_grant_requires_current_registered_source_epoch");
    if(!GrantAllowed(r.source))return Wait(r,"subequippable_queued_grant_waiting_coordinator_source_policy");
    if(r.deleteRequested)return true;
    if(!Configured(r))return false;if(!r.child.type||!r.child.slot)return true;
    const auto existing=Read<void*>(r.component,0xf8);
    if(!r.child.actor&&existing)return Wait(r,"subequippable_existing_native_child_preserved_not_owned_by_this_producer");
    if(r.child.actor&&existing&&existing!=r.child.actor)
        return Fail(r,6245,"subequippable_actual_component_reference_replaced_by_another_producer");
    if(!r.child.actor){
        // Do not create an uninitialized firearm while its parent's reference
        // route is still initial or absent. This also ensures its own finite
        // ammo is prepared before the new child gets an ActorChannel.
        if(!PrepareField(r,nullptr))return false;if(r.history.deferred)return true;
        auto found=engine::function<void*(*)(void*,void*,uint8_t)>(0x70f82b0)(r.source.inventory,r.child.type,0xfd);
        if(found)return Wait(r,"subequippable_configured_child_already_in_inventory_without_component_receipt");
        if(!GrantAllowed(r.source))return Wait(r,"subequippable_factory_waiting_current_coordinator_source_policy");
        if(r.generation==UINT64_MAX)return Fail(r,6246,"subequippable_child_generation_exhausted");
        ++r.generation;const bool spawned=NativeResult(r,loadout::Spawn(r.context,r.child));
        if(!spawned)return false;++state.originalFactories;
        if(!GrantAllowed(r.source))return Fail(r,6295,"subequippable_source_policy_closed_inside_original_factory_requires_stop");
        if(!original_ability_tuning::EnsureActor(r.child.actor)||!original_effects::EnsureActor(r.child.actor,"host_authored_subequippable"))
            return Fail(r,6247,"subequippable_original_tuning_or_effect_context_unavailable");
    }
    {
        if(!GrantAllowed(r.source))return Fail(r,6295,"subequippable_source_policy_closed_before_original_child_publication_requires_stop");
        if(!NativeResult(r,loadout::Owner(r.context,r.child))||!NativeResult(r,loadout::Policy(r.context,r.child)))return false;
        if(!GrantAllowed(r.source))return Fail(r,6295,"subequippable_source_policy_closed_during_original_owner_callback_requires_stop");
        if(!r.child.ownerReady||!r.child.ownershipReady){
            if(!r.ammoPrepared)return Fail(r,6286,"subequippable_new_child_owner_policy_did_not_complete_before_first_ammo_record");
            return Wait(r,"subequippable_waiting_original_typed_owner_and_remote_policy");}
        if(!NativeResult(r,loadout::Publish(r.context,r.child)))return false;
        if(!GrantAllowed(r.source))return Fail(r,6295,"subequippable_source_policy_closed_during_original_slot_publication_requires_stop");
        if(Read<void*>(r.component,0xf8)!=r.child.actor){
            if(!SetReference(r,r.child.actor,false))return false;
            if(r.history.deferred){
                if(!r.ammoPrepared)return Fail(r,6287,"subequippable_new_child_reference_route_changed_inside_prechannel_grant_requires_stop");
                return true;}
        }
        if(!GrantAllowed(r.source))return Fail(r,6295,"subequippable_source_policy_closed_during_original_reference_publication_requires_stop");
        if(!r.ammoPrepared){
            if(!resource::PrepareAuthoredSubequippableAmmo(r.source.world,r.source.controller,r.source.playerState,r.source.pawn,
                r.source.inventory,r.child.actor,r.source.sourceEpoch,r.component,r.ability)){
                const auto& s=resource::GetState();return Fail(r,s.error?s.error:6248,s.error?s.stage:"subequippable_original_finite_ammo_preparation_failed");}
            auto record=resource::FindCached(r.child.actor);
            if(!record||!record->weaponAmmo||!record->authoredSubequippable||!record->ammoInitialPrepared
               ||record->sourceEpoch!=r.source.sourceEpoch||record->pawn!=r.source.pawn||!record->registration
               ||!original_inventory_slots::SameIdentity(record->identities[3],r.source.identities[4])
               ||!original_inventory_slots::SameIdentity(record->identities[5],r.child.actorId))
                return Fail(r,6288,"subequippable_actual_initial_ammo_registration_receipt_not_observed");
            r.ammoRegistration=record->registration;r.ammoPrepared=true;
        }
        // All ordinary waits are after finite ammo has been initialized in
        // the same GT grant as slot/reference publication. The real child may
        // now acquire its first ActorChannel while its original graph settles.
        if(!ChildGraph(r))return false;
        if(!r.graph.commonEquipRegistered||!r.graph.graphReady)return true;
        if(!GrantAllowed(r.source))return Wait(r,"subequippable_finite_child_waiting_policy_after_original_graph");
        if(!NotifyReference(r))return false;
        if(!Source(r)||!Live(r.child.actor,r.child.actorId)||Read<void*>(r.component,0xf8)!=r.child.actor
           ||!loadout::Contains(r.child))return Fail(r,6249,"subequippable_original_grant_receipt_changed_during_notification");
        r.child.role=Read<uint8_t>(r.child.actor,0x224);r.child.remoteRole=Read<uint8_t>(r.child.actor,0x128);
        if(r.child.role!=3||r.child.remoteRole!=2)return Fail(r,6250,"subequippable_native_role_policy_not_observed");
        r.ready=!r.deleteRequested;r.grantRequested=false;r.stage="subequippable_original_child_assembled_delivery_and_execution_unverified";
        ++r.grants;
    }
    return true;
}
inline bool EnsureGrant(Record& r){
    if(r.grantInFlight||r.deleteInFlight||r.notifyInFlight)return true;
    r.grantInFlight=true;
    __try{return EnsureGrantBody(r);}__finally{r.grantInFlight=false;}
}
inline bool InventoryRefers(const Record& r){
    for(size_t offset:{size_t(0x280),size_t(0x268),size_t(0x320),size_t(0x2a0)})
        if(Read<void*>(r.source.inventory,offset)==r.child.actor)return true;
    return false;
}
inline bool OwnedChild(const Record& r,bool requireMembership,bool allowClearedReference){
    if(!NativeRecord(r)||!ProofAlive(r.source)||!Live(r.child.actor,r.child.actorId)||!Live(r.child.type,r.child.typeId)
       ||!Live(r.defaults,r.defaultsId)||Read<void*>(r.child.type,0x120)!=r.defaults
       ||Read<uint8_t>(r.defaults,0x4a0)!=BackpackSlot||r.child.slotType!=BackpackSlot
       ||Read<void*>(r.component,0xe0)!=r.child.type||!original_players::Derived(Read<void*>(r.child.actor,0x10),r.child.type)
       ||Read<uint8_t>(r.child.actor,0x224)!=3||Read<uint8_t>(r.child.actor,0x128)!=2
       ||Read<void*>(r.child.actor,0x210)!=r.source.pawn||Read<void*>(r.child.actor,0x4a8)!=r.source.pawn
       ||Read<uint8_t>(r.child.actor,0x4a0)!=BackpackSlot||local_server_item_ownership::ActorWorld(r.child.actor)!=r.source.world
       ||Read<void*>(r.source.inventory,0x1b8+BackpackSlot*8)!=r.child.slot||!Live(r.child.slot,r.child.slotId)
       ||!local_server_item_ownership::ValidSlot(r.source.inventory,r.child.slot,BackpackSlot))return false;
    auto actual=Read<void*>(r.component,0xf8);
    if(actual!=r.child.actor&&!(allowClearedReference&&r.deleteNotified&&!actual))return false;
    return !requireMembership||loadout::Contains(r.child);
}
inline bool RetireOwnedAmmo(Record& r){
    if(r.ammoPrepared&&r.child.actor&&r.ammoRegistration){
        auto record=resource::FindCached(r.child.actor);
        // A weak-invalid old address may already hold a newer UObject/epoch.
        // Registration+epoch+both original weak identities select only this
        // producer's exact saved receipt, even if the old actor has expired.
        if(record&&record->weaponAmmo&&record->authoredSubequippable&&record->registration==r.ammoRegistration
           &&record->sourceEpoch==r.source.sourceEpoch&&record->pawn==r.source.pawn
           &&original_inventory_slots::SameIdentity(record->identities[3],r.source.identities[4])
           &&original_inventory_slots::SameIdentity(record->identities[5],r.child.actorId)){
            if(!resource::RetireWeaponAmmo(r.child.actor))return Fail(r,6257,"subequippable_exact_native_ammo_record_retirement_failed");}
    }
    r.ammoPrepared=false;r.ammoRegistration=0;return true;
}
inline bool EnsureDeleteBody(Record& r){
    if(!Source(r))return Fail(r,6251,"subequippable_delete_requires_current_registered_source_epoch");
    r.ready=false;
    if(!r.child.actor){
        if(Read<void*>(r.component,0xf8))return Wait(r,"subequippable_external_native_child_preserved_delete_not_owned");
        r.deleteRequested=false;r.stage="subequippable_original_component_already_empty";return true;
    }
    if(!r.child.spawned)return Fail(r,6252,"subequippable_delete_rejects_actor_not_created_by_this_producer");
    if(!Live(r.child.actor,r.child.actorId))return Fail(r,6253,"subequippable_child_expired_before_requested_delete");
    if(!OwnedChild(r,true,false))return Fail(r,6289,"subequippable_delete_rejects_transferred_or_rebound_child");
    if(InventoryRefers(r)){
        if(!r.unequipRequested){struct Unequip{uint8_t speed=1,ignoreAutoEquip=0,result=0;} params;
            if(!loadout::Event(r.child.actor,L"/Script/ShooterGame.AresEquippable.UnequipEquippable",3,&params)||!params.result)
                return Fail(r,6254,"subequippable_original_unequip_request_rejected");
            if(!OwnedChild(r,true,false))return Fail(r,6290,"subequippable_child_owner_changed_during_original_unequip");
            r.unequipRequested=true;}
        if(r.deleteAt&&GetTickCount64()-r.deleteAt>20000)return Fail(r,6255,"subequippable_native_inventory_retirement_timeout_no_state_forced");
        return Wait(r,"subequippable_waiting_original_inventory_equip_references_to_retire");
    }
    if(Read<void*>(r.component,0xf8)==r.child.actor){
        if(!SetReference(r,nullptr))return false;if(r.history.deferred)return true;
        r.deleteNotified=true;
    }else if(Read<void*>(r.component,0xf8))return Fail(r,6256,"subequippable_component_reference_changed_before_delete");
    if(!OwnedChild(r,true,true))return Fail(r,6291,"subequippable_child_owner_changed_during_original_reference_notification");
    if(!RetireOwnedAmmo(r))return false;
    auto array=Read<loadout::Array<void*>>(r.child.slot,0x50);
    if(!loadout::ValidArray(array,256))return Fail(r,6258,"subequippable_original_backpack_array_invalid_at_delete");
    int32_t selected=-1;
    for(int32_t n=0;n<array.count;++n)if(Read<void*>(array.data,size_t(n)*8)==r.child.actor){
        if(selected>=0)return Fail(r,6259,"subequippable_duplicate_native_backpack_child_membership");selected=n;}
    if(selected>=0){
        engine::function<void(*)(void*,void*,int32_t)>(0x4516e40)(static_cast<uint8_t*>(r.child.slot)+0x50,loadout::multiContentsProperty,selected);
        if(Read<int32_t>(r.child.slot,0x58)!=array.count-1||loadout::Contains(r.child))
            return Fail(r,6260,"subequippable_original_array_remove_readback_failed");
    }
    if(!OwnedChild(r,false,true))return Fail(r,6292,"subequippable_child_owner_changed_during_original_array_remove");
    if(!original_players::Write<uint8_t>(r.child.actor,0x4b0,0)
       ||!loadout::Event(r.child.slot,L"/Script/ShooterGame.ItemSlot.OnRep_Contents",0,nullptr))
        return Fail(r,6261,"subequippable_original_slot_delete_notification_failed");
    if(!OwnedChild(r,false,true))return Fail(r,6293,"subequippable_child_owner_changed_during_original_slot_notification");
    if(!original_effects::RetireActor(r.child.actor))return Fail(r,6262,"subequippable_original_child_effect_retirement_failed");
    if(!OwnedChild(r,false,true))return Fail(r,6294,"subequippable_child_owner_changed_during_original_effect_cleanup");
    const bool destroyed=loadout::Event(r.child.actor,L"/Script/Engine.Actor.K2_DestroyActor",0,nullptr);
    if(!destroyed)return Fail(r,6262,"subequippable_original_child_actor_destroy_failed");
    ++state.originalDeletes;++r.deletes;r.child={};r.graph={};r.defaults=nullptr;r.defaultsId={};
    r.deleteRequested=r.deleteNotified=r.unequipRequested=r.referenceNotified=false;r.deleteAt=0;
    r.stage="subequippable_owned_child_deleted_original_slot_and_component_empty";return true;
}
inline bool EnsureDelete(Record& r){
    if(r.grantInFlight||r.deleteInFlight||r.notifyInFlight)return true;
    r.deleteInFlight=true;
    __try{return EnsureDeleteBody(r);}__finally{r.deleteInFlight=false;}
}
inline bool ResolveParent(void* component,SourceView& source){
    source={};
    if(heroes::ResolveSource(component,source)&&GrantAllowed(source))return true;
    return ResolveOwnedSubequippable(component,source);
}
inline bool RegisterImpl(void* component){
    if(!Thread()||state.error)return false;
    if(state.closed)return Fail(6296,"subequippable_registration_after_close_requires_new_pair");
    if(!component||Read<void*>(component,0x10)!=engine::Class(ComponentClass))
        return Fail(6263,"subequippable_register_rejects_unknown_component_class_or_override");
    SourceView source{};
    if(!ResolveParent(component,source)||!ProofAlive(source)||source.world!=state.world)
        return Fail(6264,"subequippable_register_requires_real_current_registered_hero_source");
    if(auto existing=Find(component))return SameSource(existing->source,source)
        ||Fail(*existing,6265,"subequippable_registered_source_changed_without_epoch_retirement");
    auto ability=Read<void*>(component,0xc8);
    if(!original_players::IsA(ability,0x645c660)||Read<uint8_t>(ability,0x224)!=3||Read<void*>(ability,0x210)!=source.pawn
       ||local_server_item_ownership::ActorWorld(ability)!=source.world)
        return Fail(6266,"subequippable_actual_component_owner_is_not_source_owned_authority_item");
    if(Read<void*>(ability,0x4a8)!=source.pawn){state.stage="subequippable_waiting_original_parent_typed_owner";return true;}
    Record* target=nullptr;for(auto& r:records)if(!r.used||r.retired){target=&r;break;}
    if(!target)return Fail(6267,"subequippable_bounded_component_registry_full");
    *target={};auto& r=*target;r.used=true;r.source=source;r.component=component;r.ability=ability;r.componentId=source.identities[0];
    if(!Capture(ability,r.abilityId))return Fail(r,6268,"subequippable_actual_parent_item_identity_capture_failed");
    auto& c=r.context;c.used=true;c.epoch=source.sourceEpoch;c.world=source.world;c.controller=source.controller;
    c.playerState=source.playerState;c.pawn=source.pawn;c.inventory=source.inventory;
    c.worldId=source.identities[1];c.controllerId=source.identities[2];c.playerStateId=source.identities[3];
    c.pawnId=source.identities[4];c.inventoryId=source.identities[5];
    if(!Source(r)||!NativeResult(r,loadout::Slots(c))||!c.slotsReady||!ComponentRoute(r))
        return state.error?false:Wait(r,"subequippable_waiting_original_source_and_native_slots");
    ++state.registrations;r.stage="subequippable_current_hero_component_registered";return true;
}
inline bool InitialRequest(Record& r){
    if(r.initialRequestSent)return true;
    if(!GrantAllowed(r.source))return Wait(r,"subequippable_initial_request_waiting_coordinator_source_policy");
    if(Read<void*>(r.component,0xf8))return Wait(r,"subequippable_existing_native_child_reference_preserved");
    const auto before=r.grantRequests;
    if(!loadout::Event(r.component,L"/Script/ShooterGame.AbilitySubequippableComponent.AuthGrantSubequippable",8,&r.source.pawn))
        return Fail(r,6269,"subequippable_original_grant_invocation_failed");
    if(r.grantRequests<=before)return Fail(r,6270,"subequippable_actual_native_grant_boundary_or_parameter_parse_not_observed");
    r.initialRequestSent=true;return !state.error;
}
inline bool RetireRecord(Record& r){
    if(r.grantInFlight||r.deleteInFlight||r.notifyInFlight)return Fail(r,6271,"subequippable_epoch_retirement_during_original_native_callback");
    if(!RetireOwnedAmmo(r))return false;
    // Match/Equipment owns epoch actor/slot destruction. Explicit AuthDelete
    // above is the only path that performs this producer's live child removal.
    r.retired=true;r.ready=false;r.grantRequested=false;r.deleteRequested=false;r.ammoPrepared=false;
    r.stage="subequippable_epoch_receipts_retired_native_actor_teardown_owned_by_match";++state.retirements;return true;
}
inline bool ScanItem(void* item){
    SourceView source{};if(!ResolveParent(item,source))return true;
    struct Query{void* type;void** data=nullptr;int32_t count=0,capacity=0;} q{engine::Class(ComponentClass)};
    if(!loadout::Event(item,L"/Script/Engine.Actor.K2_GetComponentsByClass",24,&q))return Fail(6273,"subequippable_original_component_query_failed");
    struct Allocation{void* data;~Allocation(){if(data)engine::function<void(*)(void*)>(0x1915720)(data);}} allocation{q.data};
    if(q.count<0||q.count>32||q.capacity<q.count||(q.count&&!original_players::Readable(q.data,size_t(q.count)*8)))
        return Fail(6274,"subequippable_original_component_query_array_invalid");
    for(int32_t n=0;n<q.count;++n){auto component=q.data[n];
        if(Read<void*>(component,0xc8)!=item)return Fail(6275,"subequippable_original_component_query_returned_foreign_owner");
        if(!RegisterImpl(component))return false;}
    ++state.componentScans;
    return true;
}
inline bool PollImpl(){
    if(!state.initialized||!state.host)return true;
    if(!Thread()||state.error)return false;
    if(state.closed){state.stage="subequippable_closed_no_new_discovery_or_grants";return true;}
    // Traverse only currently registered authored hero items, not world-wide
    // component lists or actors that merely happen to claim the same Owner.
    const auto now=GetTickCount64();const bool scan=!state.lastScanAt||now-state.lastScanAt>=100;
    if(scan)state.lastScanAt=now;
    for(const auto& p:heroes::players)if(scan&&p.used&&!p.retired&&p.authority&&p.catalogBound&&!p.error){
        for(size_t i=0;i<p.count;++i){const auto& n=p.items[i].native;
            if(p.items[i].equipmentManaged||!Live(n.actor,n.actorId))continue;
            if(!ScanItem(n.actor))return false;}}
    state.records=state.readyRecords=0;
    for(auto& r:records)if(r.used&&!r.retired){
        if(!ProofAlive(r.source)||!Live(r.component,r.componentId)||!Live(r.ability,r.abilityId)){
            if(!RetireRecord(r))return false;continue;}
        ++state.records;
        if(!Source(r))return Fail(r,6276,"subequippable_current_source_or_native_parent_owner_changed");
        if(r.deleteRequested){if(!EnsureDelete(r))return false;}
        else if(GrantAllowed(r.source)){
            if(!InitialRequest(r))return false;
            if(r.grantRequested&&!EnsureGrant(r))return false;
        }else if(r.grantRequested)Wait(r,"subequippable_queued_grant_waiting_coordinator_source_policy");
        if(r.ammoPrepared&&Live(r.child.actor,r.child.actorId)){
            if(!resource::PollWeaponAmmo(r.child.actor)){
                const auto& s=resource::GetState();return Fail(r,s.error?s.error:6277,s.error?s.stage:"subequippable_ongoing_original_ammo_failed");}
        }
        if(scan&&r.child.actor&&r.child.published&&Live(r.child.actor,r.child.actorId)&&Read<void*>(r.component,0xf8)==r.child.actor){
            if(!ScanItem(r.child.actor))return false;}
        if(r.ready)++state.readyRecords;
    }
    state.stage="subequippable_original_host_children_polled_delivery_and_execution_unverified";return true;
}
inline bool Restore(FunctionBinding& binding){
    if(!binding.function||!binding.original||!binding.replacement)return true;
    if(!Live(binding.function,binding.identity)||!WritableExec(binding.function))
        return Fail(6278,"subequippable_original_ufunction_retired_before_exec_restoration");
    auto address=reinterpret_cast<void* volatile*>(static_cast<uint8_t*>(binding.function)+0xe0);
    auto actual=Read<void*>(binding.function,0xe0);
    if(actual==reinterpret_cast<void*>(binding.original))return true;
    if(actual!=reinterpret_cast<void*>(binding.replacement)||InterlockedCompareExchangePointer(address,
        reinterpret_cast<void*>(binding.original),reinterpret_cast<void*>(binding.replacement))!=reinterpret_cast<void*>(binding.replacement))
        return Fail(6279,"subequippable_native_exec_restoration_conflicts_with_another_owner");
    return true;
}
inline bool InitImpl(void* world,bool host,SourceAllowed guard){
    if(state.initialized)return !state.error&&!state.restartRequired&&!state.closed&&state.world==world&&state.host==host
        &&sourceAllowed==guard&&(!host||state.installed);
    if(!world||!original_players::Thread()||!Capture(world,state.worldId))return Fail(6280,"subequippable_init_requires_actual_world_and_game_thread");
    const auto& h=heroes::GetState();
    if(!h.initialized||h.world!=world||h.host!=host||h.gameThread!=GetCurrentThreadId())
        return Fail(6281,"subequippable_init_requires_matching_original_hero_framework");
    if(host&&!guard)return Fail(6297,"subequippable_host_requires_explicit_coordinator_source_guard");
    state.world=world;state.host=host;state.gameThread=GetCurrentThreadId();state.initialized=true;
    sourceAllowed=guard;state.guardConfigured=guard!=nullptr;state.closed=false;
    if(!host){state.stage="subequippable_receiving_client_original_replication_untouched";return true;}
    if(!VerifyProfiles()||!VerifySchema()){state.closed=true;return false;}
    if(!Install(grantBinding,&GrantExec,0x36d8d20)){state.closed=true;return false;}
    if(!Install(deleteBinding,&DeleteExec,0x3b866f0)){
        state.closed=true;state.deleteRestored=Restore(deleteBinding);state.grantRestored=Restore(grantBinding);return false;}
    state.installed=true;state.stage="subequippable_original_stub_bound_to_registered_host_policy";return true;
}
inline bool CloseImpl(){
    if(!state.initialized||!state.host)return true;
    if(GetCurrentThreadId()!=state.gameThread||!original_players::Thread())return Fail(6282,"subequippable_close_requires_original_game_thread");
    state.closed=true;
    if(state.execInFlight||graphBusy)return Fail(6283,"subequippable_close_during_native_exec_or_graph_callback");
    for(const auto& r:records)if(r.grantInFlight||r.deleteInFlight||r.notifyInFlight)
        return Fail(6284,"subequippable_close_during_native_child_lifecycle");
    state.deleteRestored=Restore(deleteBinding);state.grantRestored=Restore(grantBinding);
    if(!state.deleteRestored||!state.grantRestored)return false;
    for(auto& r:records)if(r.used&&!r.retired&&!RetireRecord(r))return false;
    state.installed=false;state.stage="subequippable_original_native_execs_restored";return true;
}
inline LONG Exception(EXCEPTION_POINTERS* e){
    state.error=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0006200U;
    const auto address=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=address>=engine::image?address-engine::image:address;state.restartRequired=true;
    state.stage="subequippable_host_exception_requires_new_owned_pair";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Init(void* world,bool host,SourceAllowed guard=nullptr){__try{const auto result=InitImpl(world,host,guard);Cache();return result;}__except(Exception(GetExceptionInformation())){Cache();return false;}}
inline bool Poll(){__try{const auto result=PollImpl();Cache();return result;}__except(Exception(GetExceptionInformation())){Cache();return false;}}
inline bool Register(void* component){__try{const auto result=RegisterImpl(component);Cache();return result;}__except(Exception(GetExceptionInformation())){Cache();return false;}}
inline bool Register(void* world,void* pc,void* ps,void* pawn,uint32_t epoch,void* ability,void* component){
    SourceView source{};
    if(!ResolveParent(component,source)||source.world!=world||source.controller!=pc||source.playerState!=ps
       ||source.pawn!=pawn||source.sourceEpoch!=epoch||Read<void*>(component,0xc8)!=ability)
        return Fail(6285,"subequippable_explicit_registration_tuple_does_not_match_current_hero_source");
    return Register(component);
}
inline bool RetirePlayer(void* pawn,uint32_t epoch){
    __try {if(!Thread())return false;for(auto& r:records)if(r.used&&!r.retired&&r.source.pawn==pawn&&r.source.sourceEpoch==epoch)
        if(!RetireRecord(r)){Cache();return false;}Cache();return true;}
    __except(Exception(GetExceptionInformation())){Cache();return false;}
}
inline bool Close(){__try{const auto result=CloseImpl();Cache();return result;}__except(Exception(GetExceptionInformation())){Cache();return false;}}
inline bool Ready(void* component){
    if(!Thread()||state.error||state.closed)return false;auto r=Find(component);
    return r&&r->ready&&r->ammoPrepared&&r->ammoRegistration&&OwnedChild(*r,true,false);
}
template<class Visit> inline bool VisitReplicationGraph(Visit&& visit){
    if(!state.initialized||!state.host)return true;if(!Thread()||state.error)return false;
    if(state.closed)return true;
    for(const auto& r:records)if(r.used&&!r.retired&&ProofAlive(r.source)&&NativeRecord(r)&&Live(r.component,r.componentId)){
        const bool admitted=GrantAllowed(r.source);
        const bool cleanupPending=r.deleteRequested&&r.child.actor&&r.child.spawned&&OwnedChild(r,true,true);
        const bool cleanup=cleanupPending||(r.deletes&&!r.child.actor&&!Read<void*>(r.component,0xf8));
        if(!admitted&&!cleanup)continue;
        visit(r.ability,r.component);
        if(!admitted){
            // An already-owned delete must still publish its real cleared
            // component/slot reference. This path never admits the child Actor
            // or creates a new channel/actor while new grants are fenced.
            if(Live(r.child.slot,r.child.slotId))visit(r.source.pawn,r.child.slot);
            continue;
        }
        if(!r.child.published||!r.child.ownershipReady||!Live(r.child.actor,r.child.actorId))continue;
        visit(r.child.actor,r.child.actor);if(Live(r.child.slot,r.child.slotId))visit(r.source.pawn,r.child.slot);
        for(size_t n=0;n<r.child.componentCount;++n)if(Live(r.child.components[n],r.child.componentIds[n]))visit(r.child.actor,r.child.components[n]);
    }
    return true;
}
inline void Write(FILE* file){
    if(!file)return;const auto d=Snapshot();const auto& s=d.summary;auto b=[](bool value){return value?"true":"false";};
    std::fputs("{\"stage\":",file);original_players::Text(file,s.stage);
    std::fprintf(file,",\"closed\":%s,\"guard_configured\":%s,\"grant_exec_restored\":%s,\"delete_exec_restored\":%s,\"guard_queries\":%llu,\"guard_deferrals\":%llu",
        b(s.closed),b(s.guardConfigured),b(s.grantRestored),b(s.deleteRestored),static_cast<unsigned long long>(s.guardQueries),static_cast<unsigned long long>(s.guardDeferrals));
    std::fprintf(file,",\"error\":%u,\"initialized\":%s,\"host\":%s,\"installed\":%s,\"profiles\":%s,\"schema\":%s,\"restart_required\":%s,\"records\":%u,\"ready_records\":%u,\"registrations\":%llu,\"retirements\":%llu,\"grant_execs\":%llu,\"delete_execs\":%llu,\"original_forwards\":%llu,\"actual_code_parameters\":%llu,\"actual_compiled_parameters\":%llu,\"original_factories\":%llu,\"original_deletes\":%llu,\"native_field_comparisons\":%llu,\"native_field_history_witnesses\":%llu,\"native_field_deferrals\":%llu,\"exception_rva\":%llu,\"original_image_writes\":0,\"client_factory\":false,\"synthetic_frame\":false,\"native_shadow_writes\":0,\"retail_server_inventory_policy_recovered\":false,\"client_delivery_verified\":false,\"skill_execution_verified\":false,\"cached_snapshot\":true,\"entries\":[",
        s.error,b(s.initialized),b(s.host),b(s.installed),b(s.profiles),b(s.schema),b(s.restartRequired),s.records,s.readyRecords,
        static_cast<unsigned long long>(s.registrations),static_cast<unsigned long long>(s.retirements),static_cast<unsigned long long>(s.grantExecs),static_cast<unsigned long long>(s.deleteExecs),
        static_cast<unsigned long long>(s.originalForwards),static_cast<unsigned long long>(s.codeParameters),static_cast<unsigned long long>(s.compiledParameters),
        static_cast<unsigned long long>(s.originalFactories),static_cast<unsigned long long>(s.originalDeletes),static_cast<unsigned long long>(s.fieldComparisons),
        static_cast<unsigned long long>(s.fieldHistoryWitnesses),static_cast<unsigned long long>(s.fieldDeferrals),static_cast<unsigned long long>(s.exceptionRva));
    bool comma=false;for(const auto& r:d.rows)if(r.used){if(comma)std::fputc(',',file);comma=true;
        std::fprintf(file,"{\"component\":\"%p\",\"ability\":\"%p\",\"pawn\":\"%p\",\"item\":\"%p\",\"configured_class\":\"%p\",\"hero\":%u,\"epoch\":%u,\"error\":%u,\"slot\":%u,\"role\":%u,\"remote_role\":%u,\"retired\":%s,\"assembly_ready\":%s,\"grant_requested\":%s,\"delete_requested\":%s,\"published\":%s,\"ammo_prepared\":%s,\"generation\":%llu,\"grants\":%llu,\"deletes\":%llu,\"original_notifications\":%llu,\"stage\":",
            r.component,r.ability,r.pawn,r.item,r.configuredClass,r.hero,r.epoch,r.error,unsigned(r.slot),unsigned(r.role),unsigned(r.remoteRole),b(r.retired),b(r.ready),b(r.grantRequested),b(r.deleteRequested),b(r.published),b(r.ammoPrepared),
            static_cast<unsigned long long>(r.generation),static_cast<unsigned long long>(r.grants),static_cast<unsigned long long>(r.deletes),static_cast<unsigned long long>(r.notifications));
        original_players::Text(file,r.stage);const auto& h=r.history;
        std::fprintf(file,",\"reference_history\":{\"prepared\":%s,\"deferred\":%s,\"native_history_published\":%s,\"original_flags\":%u,\"original_handle\":%u,\"handle_observed\":%s,\"terminator_observed\":%s,\"before\":\"%p\",\"after\":\"%p\",\"stage\":",
            b(h.prepared),b(h.deferred),b(h.published),h.flags,unsigned(h.handle),b(h.handleObserved),b(h.terminator),h.before,h.after);
        original_players::Text(file,h.stage);std::fputs("}}",file);
    }
    std::fputs("]}",file);
}
} // namespace local_server_ability_subequippable
