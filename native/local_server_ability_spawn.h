#pragma once
#include "original_inventory_slots.h"
#include "original_jett_priming.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <iterator>

// Candidate HOST only. The two pinned native wrappers evaluate their inputs
// and return null; their retail server factory bodies are absent. This fills
// only that boundary for a registered original hero source and an admitted
// original class. It never changes an original image, role, input, prediction
// state, class default, attachment or native replication flag.
// Source/ABI proof: work/local-server-playable/hero-abilities/group4-native/.
namespace local_server_ability_spawn {
using Weak=original_inventory_slots::WeakIdentity;
using Exec=void(*)(void*,void*,void*);
enum class Kind:uint8_t {AbilityChild,GameObject};
inline constexpr size_t MaxActors=512,MaxParameters=7;
struct Source {
    void *contextObject=nullptr,*world=nullptr,*controller=nullptr,*playerState=nullptr,*pawn=nullptr,*inventory=nullptr,*ability=nullptr;
    uint32_t heroIndex=0,sourceEpoch=0;uint64_t registration=0;bool pendingGrant=false;
};
struct alignas(16) Request {
    engine::Transform transform{};
    void *context=nullptr,*type=nullptr,*instigator=nullptr,*owner=nullptr,*caller=nullptr;
    Kind kind=Kind::AbilityChild;uint8_t collision=0,padding[14]{};
};
struct Hooks {
    // Root composes its actual registry/epoch resolver. False is untouched
    // original execution, not an invented local-player relationship.
    bool (*resolve)(void* caller,Source&)=nullptr;
    bool (*validate)(const Source&)=nullptr;
    // Optional replacement for the source-bound original class cases.
    // Root must admit original configured classes, never arbitrary pointers.
    bool (*admit)(const Source&,const Request&)=nullptr;
    // Actual original tuning/effect dependencies may be prepared by Root.
    // preFinish is true after the real deferred factory, before construction.
    bool (*prepare)(const Source&,void* actor,bool preFinish)=nullptr;
};
struct Entry {
    void *actor=nullptr,*owner=nullptr,*instigator=nullptr,*requestedInstigator=nullptr,*type=nullptr;
    Weak identity{},ownerIdentity{},instigatorIdentity{},requestedInstigatorIdentity{},typeIdentity{};
    std::array<Weak,7> sourceIdentities{};uint64_t receipt=0;
    Source source{};Kind kind=Kind::AbilityChild;bool reserved=false,committed=false,originalReplicates=false;
};
// This is an exact receipt for an actor returned by THIS real native factory,
// never a generic Actor.Owner-chain attribution. The source is freshly resolved
// from the original registered ability for the same birth epoch/registration.
// Its contextObject/identities[0] describe the actual child (or its component).
struct OwnedChildSource {
    Source source{};std::array<Weak,7> identities{};
    void* actor=nullptr;Weak actorIdentity{};uint64_t receipt=0;
};
struct Function {
    void* object=nullptr;Weak identity{};Exec original=nullptr,replacement=nullptr;
    std::array<void*,MaxParameters> parameters{};uint8_t inputCount=0;bool installed=false;
};
inline constexpr size_t MaxSchemaFields=32;
struct FieldSchema {
    void *field=nullptr,*fieldClass=nullptr,*next=nullptr,*structure=nullptr;
    uintptr_t raw78=0,raw80=0;uint64_t ownerBits=0,flags=0,castFlags=0,nameBits=0;
    uint32_t nameTail=0;int32_t offset=-1,bytes=0,dimensions=0,structureSize=0,structureAlignment=0;
    bool readable=false;
};
struct ParameterSchema {
    const wchar_t* name=nullptr;FieldSchema actual{};
    int32_t expectedOffset=-1,expectedBytes=0,lookupOffset=-1;
    uint64_t expectedFlags=0,expectedNameBits=0;uint32_t failures=0;
    bool expected=false,genericProperty=false,nameMatches=false,boundsValid=false,matches=false;
};
struct FunctionSchema {
    const wchar_t* path=nullptr;void *object=nullptr,*type=nullptr,*outer=nullptr,*expectedOuter=nullptr,*firstProperty=nullptr;
    uintptr_t exec=0,expectedExec=0;uint64_t nameBits=0,outerNameBits=0;
    uint32_t flags=0,ownerFailures=0;uint16_t parms=0,expectedParms=0,returnOffset=0;uint8_t numParms=0;
    int32_t propertiesSize=0,alignment=0;size_t expectedCount=0,actualCount=0;
    bool readable=false,execCellWritable=false,ownerMatches=false,parametersMatch=false,weakCaptureAttempted=false,weakCaptured=false;
    bool chainReadable=true,chainCycle=false,chainTruncated=false;
    std::array<ParameterSchema,MaxParameters> parameters{};
    std::array<FieldSchema,MaxSchemaFields> actualFields{};
};
struct SchemaSnapshot {
    uint64_t captures=0,at=0;uintptr_t imageBase=0;uint32_t expectedQueries=0,parameterFailures=0,ownerFailures=0;
    std::array<FunctionSchema,2> functions{};
};
struct State {
    const char* stage="host_ability_spawn_not_installed";unsigned error=0;
    bool installed=false,profilesVerified=false,schemaVerified=false,closed=false;
    void* world=nullptr;Weak worldIdentity{};DWORD gameThread=0;
    uint32_t active=0;uint64_t installs=0,closes=0,calls=0,originalForwards=0,eligibleCalls=0;
    uint64_t classRejections=0,sourceDeferrals=0,frameSteps=0,compiledSteps=0,scriptSteps=0;
    uint64_t factories=0,finished=0,returned=0,spawnFailures=0,discarded=0,pruned=0;
    uint64_t ownerReadbacks=0,instigatorReadbacks=0,registryVisits=0,nonreplicated=0;
    uint64_t nativeDestroyedDuringFinish=0;
    uint64_t receipts=0,childSourceQueries=0,childSourceAccepted=0;
    uint64_t nativePawnNullInstigatorToSelf=0;
    uint64_t nativeCompletedAfterSourceRetired=0;
};
inline State state{},cached{};inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline SchemaSnapshot schema{},cachedSchema{};
inline Hooks hooks{};inline std::array<Function,2> functions{};
inline std::array<Entry,MaxActors> actors{};
inline original_jett_priming::PropertyInfo replicatesField{};
inline int collisionOffset=-1;
inline const State& GetState(){return state;} // Original game thread only.
inline void Cache(){AcquireSRWLockExclusive(&cacheLock);cached=state;
    if(cachedSchema.captures!=schema.captures)cachedSchema=schema;ReleaseSRWLockExclusive(&cacheLock);}
inline State Snapshot(){AcquireSRWLockShared(&cacheLock);const auto s=cached;ReleaseSRWLockShared(&cacheLock);return s;}
struct DiagnosticSnapshot {State counters{};SchemaSnapshot metadata{};};
inline DiagnosticSnapshot Diagnostics(){AcquireSRWLockShared(&cacheLock);DiagnosticSnapshot s{cached,cachedSchema};
    ReleaseSRWLockShared(&cacheLock);return s;}
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
template<class T> inline bool Put(void* p,size_t offset,const T& value){return original_players::Write(p,offset,value);}
inline bool Thread(){return engine::image&&original_players::Thread()&&state.gameThread==GetCurrentThreadId();}
inline bool Live(void* p,const Weak& id){return p&&original_inventory_slots::ResolveIdentity(id)==p;}
inline bool Capture(void* p,Weak& id){return original_inventory_slots::CaptureIdentity(p,id)&&Live(p,id);}
inline bool NullableCapture(void* p,Weak& id){if(p)return Capture(p,id);id={};return true;}
inline bool NullableLive(void* p,const Weak& id){return p?Live(p,id):id.index==-1&&!id.serial;}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;Cache();return false;}
inline bool Writable(const void* p,size_t bytes){
    MEMORY_BASIC_INFORMATION m{};if(!p||!VirtualQuery(p,&m,sizeof(m)))return false;
    const auto at=reinterpret_cast<uintptr_t>(p),start=reinterpret_cast<uintptr_t>(m.BaseAddress);
    const auto protection=m.Protect&0xffU;
    return m.State==MEM_COMMIT&&m.Type==MEM_PRIVATE&&!(m.Protect&(PAGE_GUARD|PAGE_NOACCESS))
        &&(protection==PAGE_READWRITE||protection==PAGE_EXECUTE_READWRITE)&&at>=start
        &&bytes<=m.RegionSize&&at-start<=m.RegionSize-bytes;
}
inline bool ImageCode(const void* p){
    MEMORY_BASIC_INFORMATION m{};if(!p||!VirtualQuery(p,&m,sizeof(m)))return false;
    const auto protection=m.Protect&0xffU;
    return m.State==MEM_COMMIT&&m.Type==MEM_IMAGE&&m.AllocationBase==reinterpret_cast<void*>(engine::image)
        &&!(m.Protect&(PAGE_GUARD|PAGE_NOACCESS))&&(protection==PAGE_EXECUTE||protection==PAGE_EXECUTE_READ
            ||protection==PAGE_EXECUTE_READWRITE||protection==PAGE_EXECUTE_WRITECOPY);
}
inline uint64_t Hash(const void* p,size_t bytes){
    auto data=static_cast<const uint8_t*>(p);uint64_t h=14695981039346656037ULL;
    for(size_t n=0;n<bytes;++n)h=(h^data[n])*1099511628211ULL;return h;
}
inline bool Profile(uintptr_t rva,size_t bytes,uint64_t expected){
    auto data=reinterpret_cast<const void*>(engine::image+rva);
    return ImageCode(data)&&original_players::Readable(data,bytes)&&Hash(data,bytes)==expected;
}
inline bool Event(void* object,const wchar_t* name,uint16_t bytes,void* params){
    auto fn=engine::Find(name);auto call=Read<void*>(Read<void*>(object),0x298);
    if(!object||!fn||Read<uint16_t>(fn,0xbe)!=bytes||!call)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(call)(object,fn,params);return true;
}
inline void* ActorWorld(void* actor){
    return original_players::IsA(actor,0x3683900)?engine::function<void*(*)(void*)>(0x3ed83b0)(actor):nullptr;
}
inline bool SourceValid(const Source& s){
    return Thread()&&hooks.validate&&hooks.validate(s)&&s.world==state.world&&Live(state.world,state.worldIdentity)
        &&s.contextObject&&s.pawn&&s.ability&&s.sourceEpoch&&s.registration
        &&original_players::IsA(s.pawn,original_players::ShooterCharacterClass)
        &&original_players::IsA(s.ability,0x645c660)&&Read<uint8_t>(s.pawn,0x224)==3
        &&Read<uint8_t>(s.ability,0x224)==3&&Read<void*>(s.ability,0x210)==s.pawn
        &&Read<void*>(s.ability,0x4a8)==s.pawn&&Read<void*>(s.pawn,0x488)==s.playerState
        &&Read<void*>(s.controller,0x470)==s.playerState&&Read<void*>(s.controller,0x4b0)==s.pawn
        &&ActorWorld(s.pawn)==state.world&&ActorWorld(s.ability)==state.world;
}
inline bool HostWorld(void* world){
    auto driver=Read<void*>(world,0x40);
    return world&&driver&&Read<void*>(driver,0x178)==world&&!Read<void*>(driver,0xc0);
}
inline bool Finite(const engine::Transform& t){
    if(!original_players::Finite(t.translation)||!original_players::Finite(t.scale))return false;
    double norm=0;for(double v:t.rotation){if(!std::isfinite(v))return false;norm+=v*v;}
    return std::isfinite(norm)&&norm>0.000001;
}
// Every entry is an actual original call contract, not an arbitrary class or
// inventory-slot name. Exact source/target hashes and decoded call nodes are in
// work/local-server-playable/hero-abilities/spawn-all/.
enum class InstigatorContract:uint8_t {MyPawn,ActorInstigator};
enum class OwnerContract:uint8_t {Self,PawnOwner};
struct Contract {
    uint32_t hero;Kind kind;InstigatorContract instigator;OwnerContract owner;uint8_t collision;
    const wchar_t *source,*target,*classField;
};
inline constexpr Contract contracts[]{
    {24,Kind::AbilityChild,InstigatorContract::MyPawn,OwnerContract::Self,2,
        L"/Game/Characters/Pine/S0/Ability_E/Ability_Pine_E_RadEater.Ability_Pine_E_RadEater_C",
        L"/Game/Characters/Pine/S0/Ability_E/Pawn_Pine_E_RadEater.Pawn_Pine_E_RadEater_C",L"Deployable Actor Class"},
    {29,Kind::GameObject,InstigatorContract::MyPawn,OwnerContract::Self,0,
        L"/Game/Characters/Paprika/S0/Ability_4/Ability_Paprika_4_PullBlast.Ability_Paprika_4_PullBlast_C",
        L"/Game/Characters/Paprika/S0/Ability_4/GameObject_Paprika_4_PullBlast.GameObject_Paprika_4_PullBlast_C",nullptr},
    {13,Kind::GameObject,InstigatorContract::MyPawn,OwnerContract::Self,0,
        L"/Game/Characters/Sequoia/S0/Ability_E/WreckingBall/Ability_Sequoia_E_WreckingBall.Ability_Sequoia_E_WreckingBall_C",
        L"/Game/Characters/Sequoia/S0/Ability_E/WreckingBall/GameObject_Sequoia_E_Orb.GameObject_Sequoia_E_Orb_C",L"Orb"},
    {13,Kind::GameObject,InstigatorContract::MyPawn,OwnerContract::PawnOwner,0,
        L"/Game/Characters/Sequoia/S0/Ability_E/WreckingBall/Ability_Sequoia_E_WreckingBall.Ability_Sequoia_E_WreckingBall_C",
        L"/Game/Characters/Sequoia/S0/Ability_E/WreckingBall/GameObject_Sequoia_E_Shield.GameObject_Sequoia_E_Shield_C",nullptr},
    {21,Kind::AbilityChild,InstigatorContract::ActorInstigator,OwnerContract::Self,2,
        L"/Game/Characters/Guide/S0/Ability_X/Ability_Guide_X_Pack.Ability_Guide_X_Pack_C",
        L"/Game/Characters/Guide/S0/Ability_X/Pawn_Guide_X_Pack.Pawn_Guide_X_Pack_C",L"SpawnedPawnClass"}
};
inline bool KnownSource(const Source& s){
    auto type=Read<void*>(s.ability,0x10);
    for(const auto& c:contracts)if(s.heroIndex==c.hero&&type&&type==engine::Find(c.source))return true;
    return false;
}
inline bool KnownPolicy(const Source& s,const Request& r){
    if(r.caller!=s.contextObject||r.caller!=s.ability||r.context!=r.caller)return false;
    for(const auto& c:contracts){
        if(s.heroIndex!=c.hero||r.kind!=c.kind||(r.kind==Kind::AbilityChild&&r.collision!=c.collision))continue;
        auto source=engine::Find(c.source),target=engine::Find(c.target);
        if(!source||!target||Read<void*>(r.caller,0x10)!=source||r.type!=target)continue;
        void* instigator=s.pawn;void* owner=r.caller;
        if(c.instigator==InstigatorContract::ActorInstigator
           &&!Event(r.caller,L"/Script/Engine.Actor.GetInstigator",8,&instigator))return false;
        if(c.owner==OwnerContract::PawnOwner&&!Event(s.pawn,L"/Script/Engine.Actor.GetOwner",8,&owner))return false;
        if(r.instigator!=instigator||r.owner!=owner)return false;
        if(c.classField){
            original_jett_priming::PropertyInfo field{};
            const auto offset=original_jett_priming::FieldOffset(source,c.classField,8,&field);
            if(offset<0||field.arrayDim!=1||field.elementSize!=8
               ||field.fieldClass!=Read<void*>(functions[0].parameters[1],8)
               ||Read<void*>(r.caller,static_cast<size_t>(offset))!=target
               ||Read<void*>(engine::DefaultObject(source),static_cast<size_t>(offset))!=target)return false;
        }
        return true;
    }
    return false;
}
inline bool OriginalPawnSelfInstigatorPath(const Source& s,const Request& r,void* actor){
    // Only the exact proven Skye target currently needs this native transition.
    // Its BP BaseAIPawn ancestor is the original ShooterCharacter. The actual
    // v568 is the pinned Shooter tail-call to original Pawn.PreInitializeComponents,
    // which assigns SELF only when the original Instigator is null. No adapter
    // writes that field or treats another Pawn subclass as an equivalent case.
    return s.heroIndex==21&&r.kind==Kind::AbilityChild&&!r.instigator&&r.owner==s.ability
        &&r.context==s.ability&&r.collision==2
        &&Read<void*>(r.caller,0x10)==engine::Find(L"/Game/Characters/Guide/S0/Ability_X/Ability_Guide_X_Pack.Ability_Guide_X_Pack_C")
        &&Read<void*>(actor,0x10)==engine::Find(L"/Game/Characters/Guide/S0/Ability_X/Pawn_Guide_X_Pack.Pawn_Guide_X_Pack_C")
        &&Read<void*>(Read<void*>(actor),0x568)==reinterpret_cast<void*>(engine::image+0x786c070);
}
inline bool Resolve(void* object,Source& out){
    if(!object||!hooks.resolve||!hooks.resolve(object,out)||!SourceValid(out))return false;
    return true;
}
inline bool CopyFieldFromContainer(void* field,void* to,const void* container){
    auto copy=Read<void*>(Read<void*>(field),0x110);const auto bytes=Read<int32_t>(field,0x34);
    const auto offset=Read<int32_t>(field,0x44);
    if(!field||!to||!container||offset<0||bytes<=0||bytes>96||!ImageCode(copy)
       ||!original_players::Readable(static_cast<const uint8_t*>(container)+offset,static_cast<size_t>(bytes)))return false;
    // The original v110 method receives the CONTAINER and applies the native
    // Offset_Internal itself. Passing Locals+Offset would add the offset twice.
    reinterpret_cast<void(*)(void*,void*,const void*)>(copy)(field,to,container);return true;
}
// Identical FFrame input evaluation shape to the two pinned native wrappers.
// Native opcodes run once through the actual original GNative dispatch table.
// A const-reference Transform uses its original MostRecentPropertyAddress.
inline bool Step(void* frame,void* field,void* destination,bool reference){
    if(reference&&(!Put<void*>(frame,0x38,nullptr)||!Put<void*>(frame,0x40,nullptr)))return false;
    auto code=Read<uint8_t*>(frame,0x20);
    if(code){
        if(!original_players::Readable(code,1))return false;
        const auto opcode=*code;auto native=Read<Exec>(reinterpret_cast<void*>(engine::image+0xd747c40),size_t(opcode)*8);
        if(!ImageCode(reinterpret_cast<void*>(native))||!Put(frame,0x20,code+1))return false;
        native(Read<void*>(frame,0x18),frame,destination);++state.scriptSteps;
    }else{
        auto current=Read<void*>(frame,0x88);
        if(current!=field||!Put(frame,0x88,Read<void*>(field,0x18)))return false;
        if(Read<uint64_t>(field,0x38)&0x100ULL){
            auto out=Read<void*>(frame,0x80);void* address=nullptr;
            for(size_t n=0;out&&n<256;++n){
                if(!original_players::Readable(out,24))return false;
                if(Read<void*>(out)==field){address=Read<void*>(out,8);break;}out=Read<void*>(out,0x10);
            }
            // In these exact signatures only the const-reference Transform
            // has OutParm. No other out parameter is silently generalized.
            if(!reference||!address||!Put(frame,0x38,address))return false;
        }else{
            auto locals=Read<uint8_t*>(frame,0x28);const auto offset=Read<int32_t>(field,0x44);
            if(!locals||offset<0)return false;auto address=locals+offset;
            if(!Put(frame,0x40,locals)||!Put(frame,0x38,address)||!CopyFieldFromContainer(field,destination,locals))return false;
        }
        ++state.compiledSteps;
    }
    if(reference){
        auto address=Read<void*>(frame,0x38);
        if(address){
            // Exactly FTransform (96 bytes, plain numeric storage) in these
            // original signatures. This is a direct reference value, not a
            // container; do not invoke the offset-applying v110 a second time.
            if(Read<int32_t>(field,0x34)!=sizeof(engine::Transform)||!original_players::Readable(address,sizeof(engine::Transform)))return false;
            std::memcpy(destination,address,sizeof(engine::Transform));
        }
    }
    ++state.frameSteps;return true;
}
inline bool Decode(Function& f,void* frame,Request& r){
    if(!Writable(frame,0x90)||!Step(frame,f.parameters[0],&r.context,false)
       ||!Step(frame,f.parameters[1],&r.type,false)||!Step(frame,f.parameters[2],&r.transform,true))return false;
    const size_t instigator=r.kind==Kind::AbilityChild?4:3,owner=instigator+1;
    if(r.kind==Kind::AbilityChild&&!Step(frame,f.parameters[3],&r.collision,false))return false;
    if(!Step(frame,f.parameters[instigator],&r.instigator,false)||!Step(frame,f.parameters[owner],&r.owner,false))return false;
    auto code=Read<uint8_t*>(frame,0x20);
    if(code){if(!original_players::Readable(code,1)||*code!=0x16||!Put(frame,0x20,code+1))return false;}
    return true;
}
inline bool Replicates(void* actor){
    const auto byte=Read<uint8_t>(actor,static_cast<size_t>(replicatesField.offset)+replicatesField.boolByteOffset);
    return (byte&replicatesField.boolFieldMask)!=0;
}
inline bool Discard(Entry& e){
    const auto receipt=e.receipt;auto actor=e.actor;
    if(e.actor&&Live(e.actor,e.identity)){
        if(!Event(e.actor,L"/Script/Engine.Actor.K2_DestroyActor",0,nullptr))return Fail(6920,"host_ability_spawn_native_failed_actor_cleanup_failed");
        if(e.receipt!=receipt||e.actor!=actor)return Fail(6944,"host_ability_spawn_inflight_receipt_changed_during_cleanup");
        ++state.discarded;
    }
    e={};return true;
}
inline Entry* Reserve(){
    // A self-destroying actor can synchronously invoke another original
    // factory from BeginPlay/OnDestroyed. Its OUTER in-flight slot must remain
    // reserved even after the actor's weak identity becomes invalid.
    for(auto& e:actors){
        if(e.reserved)continue;
        if(e.actor&&!Live(e.actor,e.identity)){e={};++state.pruned;}
        if(!e.actor)return &e;
    }
    return nullptr;
}
inline bool OwnsReservation(const Entry& e,uint64_t receipt,void* actor){
    return e.reserved&&!e.committed&&e.receipt==receipt&&e.actor==actor
        ?true:Fail(6944,"host_ability_spawn_inflight_native_factory_receipt_changed");
}
inline bool CaptureSource(const Source& s,std::array<Weak,7>& identities){
    void* objects[]{s.contextObject,s.world,s.controller,s.playerState,s.pawn,s.inventory,s.ability};
    for(size_t n=0;n<identities.size();++n)if(!Capture(objects[n],identities[n]))return false;
    return true;
}
inline bool SameSourceValues(const Source& a,const Source& b){
    return a.contextObject==b.contextObject&&a.world==b.world&&a.controller==b.controller&&a.playerState==b.playerState
        &&a.pawn==b.pawn&&a.inventory==b.inventory&&a.ability==b.ability&&a.heroIndex==b.heroIndex
        &&a.sourceEpoch==b.sourceEpoch&&a.registration==b.registration&&a.pendingGrant==b.pendingGrant;
}
inline bool BirthCohort(const Entry& e,const Source& current,const std::array<Weak,7>& identities){
    // A new real invocation may follow the actual pending->published change.
    // Refresh through the registry; never mutate an old source proof or borrow
    // another life/hero/registration that happens to reuse an object address.
    const auto& born=e.source;
    if(born.world!=current.world||born.controller!=current.controller||born.playerState!=current.playerState
       ||born.pawn!=current.pawn||born.inventory!=current.inventory||born.ability!=current.ability
       ||born.heroIndex!=current.heroIndex||born.sourceEpoch!=current.sourceEpoch||born.registration!=current.registration)return false;
    for(size_t n=1;n<identities.size();++n)
        if(!original_inventory_slots::SameIdentity(e.sourceIdentities[n],identities[n]))return false;
    return true;
}
inline bool ResolveOwnedChildSource(void* object,OwnedChildSource& out){
    out={};
    if(!state.installed||state.closed||state.error||!Thread()||!hooks.resolve||!hooks.validate
       ||!HostWorld(state.world)||!Live(state.world,state.worldIdentity))return false;
    ++state.childSourceQueries;Weak contextIdentity{};
    if(!Capture(object,contextIdentity))return false;
    for(const auto& e:actors){
        if(!e.committed||!e.receipt||!Live(e.actor,e.identity)||!Live(e.type,e.typeIdentity)
           ||Read<uint8_t>(e.actor,0x224)!=3||ActorWorld(e.actor)!=state.world
           ||!original_players::Derived(Read<void*>(e.actor,0x10),e.type))continue;
        const bool direct=object==e.actor;
        const bool component=original_players::IsA(object,0x36837a0)&&Read<void*>(object,0xc8)==e.actor;
        if(!direct&&!component)continue;
        if(!NullableLive(e.owner,e.ownerIdentity)||!NullableLive(e.instigator,e.instigatorIdentity)
           ||!NullableLive(e.requestedInstigator,e.requestedInstigatorIdentity)
           ||Read<void*>(e.actor,0x210)!=e.owner)return false;
        void* instigator=nullptr;
        if(!Event(e.actor,L"/Script/Engine.Actor.GetInstigator",8,&instigator)||instigator!=e.instigator)return false;
        // Resolve the exact registered original ability from this receipt,
        // rather than recursively guessing ownership from the calling actor.
        Source current{};std::array<Weak,7> identities{};
        if(!Live(e.source.ability,e.sourceIdentities[6])||!hooks.resolve(e.source.ability,current)
           ||current.contextObject!=e.source.ability||!SourceValid(current)||!CaptureSource(current,identities)
           ||!BirthCohort(e,current,identities))return false;
        current.contextObject=object;identities[0]=contextIdentity;
        out.source=current;out.identities=identities;out.actor=e.actor;out.actorIdentity=e.identity;out.receipt=e.receipt;
        ++state.childSourceAccepted;return true;
    }
    return false;
}
inline bool ValidateOwnedChildSource(const OwnedChildSource& before){
    OwnedChildSource current{};
    if(!ResolveOwnedChildSource(before.source.contextObject,current)||!SameSourceValues(before.source,current.source)
       ||before.actor!=current.actor||before.receipt!=current.receipt
       ||!original_inventory_slots::SameIdentity(before.actorIdentity,current.actorIdentity))return false;
    for(size_t n=0;n<before.identities.size();++n)
        if(!original_inventory_slots::SameIdentity(before.identities[n],current.identities[n]))return false;
    return true;
}
// Optional source-composition visitor. A stale/dead source suppresses NEW
// authority attribution, while VisitReplicationActors below retains the real
// surviving actor's own native replication/lifecycle contract independently.
template<class Visit> inline bool VisitOwnedChildSources(Visit&& visit){
    if(!state.installed)return !state.error;
    if(!Thread())return false;
    for(const auto& e:actors){
        if(!e.actor||!e.committed)continue;OwnedChildSource source{};
        if(ResolveOwnedChildSource(e.actor,source))visit(e.actor,source);
    }
    Cache();return !state.error;
}
inline void* Spawn(const Source& s,Request& r){
    const auto policy=hooks.admit?hooks.admit(s,r):KnownPolicy(s,r);
    if(!policy||!r.type||!Finite(r.transform)||!original_players::Derived(r.type,engine::Class(0x3683900))
       ||(r.kind==Kind::GameObject&&!original_players::Derived(r.type,engine::Class(0x6480c50)))
       ||r.caller!=s.contextObject||r.context!=s.ability){++state.classRejections;return nullptr;}
    if(!SourceValid(s)||!HostWorld(s.world)){++state.sourceDeferrals;return nullptr;}
    // The actual original deferred factory has no separate Instigator input:
    // its real Pawn context supplies itself, and its real Actor context reads
    // Actor.Instigator. Select only an already-proven same-world original
    // context that supplies EXACTLY the authored request, including real null.
    void* factoryContext=r.instigator;
    if(r.instigator){
        if(!original_players::IsA(r.instigator,0x3ed66e0)||Read<uint8_t>(r.instigator,0x224)!=3
           ||ActorWorld(r.instigator)!=s.world){++state.classRejections;return nullptr;}
    }else{
        void* nativeInstigator=nullptr;
        if(!original_players::IsA(r.context,0x3683900)||ActorWorld(r.context)!=s.world
           ||!Event(r.context,L"/Script/Engine.Actor.GetInstigator",8,&nativeInstigator)||nativeInstigator){
            ++state.classRejections;return nullptr;}
        factoryContext=r.context;
    }
    if(r.owner&&(!original_players::IsA(r.owner,0x3683900)||Read<uint8_t>(r.owner,0x224)!=3
       ||ActorWorld(r.owner)!=s.world)){++state.classRejections;return nullptr;}
    auto entry=Reserve();if(!entry){Fail(6921,"host_ability_spawn_real_actor_registry_capacity");return nullptr;}
    if(r.kind==Kind::GameObject){
        // There is no collision parameter in this API. Read the actual class
        // CDO policy instead of inventing an AlwaysSpawn default.
        auto cdo=engine::DefaultObject(r.type);
        if(!cdo||collisionOffset<0){Fail(6922,"host_ability_spawn_original_class_collision_default_missing");return nullptr;}
        r.collision=Read<uint8_t>(cdo,static_cast<size_t>(collisionOffset));
    }
    if(r.collision>4){Fail(6923,"host_ability_spawn_original_collision_enum_out_of_profile");return nullptr;}
    if(state.receipts==UINT64_MAX){Fail(6943,"host_ability_spawn_exact_receipt_counter_exhausted");return nullptr;}
    entry->reserved=true;entry->source=s;entry->owner=r.owner;entry->instigator=r.instigator;
    entry->requestedInstigator=r.instigator;entry->type=r.type;entry->kind=r.kind;
    entry->receipt=++state.receipts;
    const auto receipt=entry->receipt;
    if(!NullableCapture(r.owner,entry->ownerIdentity)||!NullableCapture(r.instigator,entry->instigatorIdentity)
       ||!Capture(r.type,entry->typeIdentity)||!CaptureSource(s,entry->sourceIdentities)){
        *entry={};Fail(6924,"host_ability_spawn_original_input_weak_identity_missing");return nullptr;}
    entry->requestedInstigatorIdentity=entry->instigatorIdentity;
    // Owner remains its separate original native factory argument. Neither
    // role nor either ownership field is assigned by this adapter.
    auto actor=engine::function<void*(*)(void*,void*,const engine::Transform*,uint8_t,void*,uint8_t)>(0x43fada0)
        (factoryContext,r.type,&r.transform,r.collision,r.owner,0);
    ++state.factories;
    if(!OwnsReservation(*entry,receipt,nullptr))return nullptr;
    entry->actor=actor;
    if(!entry->actor){*entry={};++state.spawnFailures;return nullptr;}
    if(!Capture(entry->actor,entry->identity)||Read<uint8_t>(entry->actor,0x224)!=3||ActorWorld(entry->actor)!=s.world
       ||!original_players::Derived(Read<void*>(entry->actor,0x10),r.type)){
        Fail(6925,"host_ability_spawn_real_deferred_actor_identity_or_role_invalid");Discard(*entry);return nullptr;}
    uint8_t begun=0;void* instigator=nullptr;
    const bool originalPrefinish=Event(entry->actor,L"/Script/Engine.Actor.HasActorBegunPlay",1,&begun)&&!begun
       &&Read<void*>(entry->actor,0x210)==r.owner&&Event(entry->actor,L"/Script/Engine.Actor.GetInstigator",8,&instigator)
       &&instigator==r.instigator;
    if(!OwnsReservation(*entry,receipt,actor))return nullptr;
    if(!originalPrefinish){Fail(6926,"host_ability_spawn_prefinish_owner_instigator_or_lifecycle_mismatch");Discard(*entry);return nullptr;}
    ++state.ownerReadbacks;++state.instigatorReadbacks;
    const bool nativePawnSelfPath=OriginalPawnSelfInstigatorPath(s,r,entry->actor);
    if(hooks.prepare){
        const bool prepared=hooks.prepare(s,actor,true);
        if(!OwnsReservation(*entry,receipt,actor))return nullptr;
        if(!prepared){Fail(6927,"host_ability_spawn_original_prefinish_dependencies_failed");Discard(*entry);return nullptr;}
        const bool unchanged=Live(actor,entry->identity)&&Read<uint8_t>(actor,0x224)==3&&ActorWorld(actor)==s.world
            &&Read<void*>(actor,0x210)==r.owner&&Event(actor,L"/Script/Engine.Actor.GetInstigator",8,&instigator)&&instigator==r.instigator;
        if(!OwnsReservation(*entry,receipt,actor))return nullptr;
        if(!unchanged){Fail(6945,"host_ability_spawn_prepare_changed_original_actor_contract");Discard(*entry);return nullptr;}
    }
    if(!SourceValid(s)||!NullableLive(r.owner,entry->ownerIdentity)||!NullableLive(r.instigator,entry->instigatorIdentity)){
        Fail(6928,"host_ability_spawn_source_retired_during_deferred_factory");Discard(*entry);return nullptr;}
    engine::function<void(*)(void*,const engine::Transform*,bool,void*,uint8_t)>(0x3ef6cb0)(entry->actor,&r.transform,false,nullptr,0);
    ++state.finished;
    if(!OwnsReservation(*entry,receipt,actor))return nullptr;
    if(!Live(entry->actor,entry->identity)){*entry={};++state.nativeDestroyedDuringFinish;return nullptr;}
    const bool originalFinished=Read<uint8_t>(entry->actor,0x224)==3&&ActorWorld(entry->actor)==s.world&&Read<void*>(entry->actor,0x210)==r.owner
       &&Event(entry->actor,L"/Script/Engine.Actor.GetInstigator",8,&instigator)
       &&Event(entry->actor,L"/Script/Engine.Actor.HasActorBegunPlay",1,&begun)&&begun;
    if(!OwnsReservation(*entry,receipt,actor))return nullptr;
    if(!originalFinished){
        Fail(6929,"host_ability_spawn_finished_original_actor_contract_mismatch");Discard(*entry);return nullptr;}
    if(instigator!=r.instigator){
        if(!nativePawnSelfPath||instigator!=entry->actor||!OriginalPawnSelfInstigatorPath(s,r,entry->actor)
           ||!Capture(instigator,entry->instigatorIdentity)){
            Fail(6929,"host_ability_spawn_finished_original_instigator_transition_not_proven");Discard(*entry);return nullptr;}
        // Keep BOTH the exact requested value and the effective original value.
        // Original Pawn.PreInitializeComponents, not this adapter, changed it.
        entry->instigator=instigator;++state.nativePawnNullInstigatorToSelf;
    }
    if(hooks.prepare){
        const bool prepared=hooks.prepare(s,actor,false);
        if(!OwnsReservation(*entry,receipt,actor))return nullptr;
        if(!prepared){Fail(6930,"host_ability_spawn_original_finished_dependencies_failed");Discard(*entry);return nullptr;}
    }
    if(!Live(actor,entry->identity)){*entry={};++state.nativeDestroyedDuringFinish;return nullptr;}
    void* effectiveInstigator=nullptr;
    const bool unchanged=Read<uint8_t>(actor,0x224)==3&&ActorWorld(actor)==s.world&&Read<void*>(actor,0x210)==r.owner
        &&Event(actor,L"/Script/Engine.Actor.GetInstigator",8,&effectiveInstigator)&&effectiveInstigator==entry->instigator
        &&Event(actor,L"/Script/Engine.Actor.HasActorBegunPlay",1,&begun)&&begun;
    if(!OwnsReservation(*entry,receipt,actor))return nullptr;
    if(!Live(actor,entry->identity)){*entry={};++state.nativeDestroyedDuringFinish;return nullptr;}
    if(!unchanged){Fail(6945,"host_ability_spawn_finished_prepare_changed_original_actor_contract");Discard(*entry);return nullptr;}
    const bool sourceAlive=SourceValid(s);
    if(!OwnsReservation(*entry,receipt,actor))return nullptr;
    if(!Live(actor,entry->identity)){*entry={};++state.nativeDestroyedDuringFinish;return nullptr;}
    entry->originalReplicates=Replicates(entry->actor);entry->committed=true;entry->reserved=false;++state.returned;
    if(!entry->originalReplicates)++state.nonreplicated;
    if(!sourceAlive){
        // The original native construction already completed. Keep that real
        // actor's native persistence/replication, but do not return a new
        // authority grant through a retired source or invent a current epoch.
        --state.returned;++state.sourceDeferrals;++state.nativeCompletedAfterSourceRetired;
        state.stage="host_ability_spawn_original_actor_completed_source_retired_return_null";return nullptr;
    }
    state.stage="host_ability_spawn_original_actor_returned_network_execution_unverified";return entry->actor;
}
inline void Execute(size_t index,void* context,void* frame,void* result){
    auto& f=functions[index];++state.calls;Source source{};
    if(!state.installed||state.closed||state.error||!Thread()||!HostWorld(state.world)||!hooks.resolve||!hooks.validate
       ||!original_players::Readable(frame,0x90)||!Writable(result,8)||!Resolve(Read<void*>(frame,0x18),source)
       ||(!hooks.admit&&!KnownSource(source))){++state.originalForwards;f.original(context,frame,result);return;}
    if(!Live(f.object,f.identity)||Read<Exec>(f.object,0xe0)!=f.replacement){
        Fail(6931,"host_ability_spawn_native_function_binding_changed");++state.originalForwards;f.original(context,frame,result);return;}
    ++state.eligibleCalls;Request request{};request.kind=index==0?Kind::AbilityChild:Kind::GameObject;request.caller=Read<void*>(frame,0x18);
    if(!Decode(f,frame,request)){
        Fail(6932,"host_ability_spawn_original_argument_frame_not_understood");
        // A partly evaluated FFrame cannot be replayed or returned to as if its
        // native call finished. Propagate a noncontinuable diagnostic rather
        // than resuming the caller at an unknown argument boundary.
        RaiseException(0xe0006932U,EXCEPTION_NONCONTINUABLE,0,nullptr);return;
    }
    // Both pinned original implementations always write null here. Only a
    // completed real authority factory replaces that null. Original execution
    // is not replayed after evaluation, so side-effecting arguments run once.
    void* actor=nullptr;
    if(SourceValid(source))actor=Spawn(source,request);else ++state.sourceDeferrals;
    std::memcpy(result,&actor,8);
}
inline void ChildExec(void* context,void* frame,void* result){
    // Non-GT callers retain the exact native entry without touching GT-only
    // registry/counters. Metadata is initialized before exec publication.
    if(!engine::image||!original_players::Thread()||state.gameThread!=GetCurrentThreadId()){
        functions[0].original(context,frame,result);return;
    }
    ++state.active;__try{Execute(0,context,frame,result);}__finally{--state.active;Cache();}
}
inline void GameObjectExec(void* context,void* frame,void* result){
    if(!engine::image||!original_players::Thread()||state.gameThread!=GetCurrentThreadId()){
        functions[1].original(context,frame,result);return;
    }
    ++state.active;__try{Execute(1,context,frame,result);}__finally{--state.active;Cache();}
}
struct Parameter {const wchar_t* name;int32_t offset,size;uint64_t flags;};
inline FieldSchema ObserveField(void* field){
    FieldSchema r{};r.field=field;r.readable=original_players::Readable(field,0x78);
    if(!r.readable)return r;
    r.fieldClass=Read<void*>(field,8);r.next=Read<void*>(field,0x18);r.ownerBits=Read<uint64_t>(field,0x10);
    r.flags=Read<uint64_t>(field,0x38);r.castFlags=Read<uint64_t>(r.fieldClass,0x18);
    r.nameBits=Read<uint64_t>(field,0x20);r.nameTail=Read<uint32_t>(field,0x28);
    r.offset=Read<int32_t>(field,0x44);r.bytes=Read<int32_t>(field,0x34);r.dimensions=Read<int32_t>(field,0x30);
    // Opaque extension words remain raw unless the actual native cast flags
    // identify a StructProperty. No type from a different probe is assumed.
    r.raw78=Read<uintptr_t>(field,0x78);r.raw80=Read<uintptr_t>(field,0x80);
    if((r.castFlags&0x108001ULL)==0x108001ULL){
        r.structure=reinterpret_cast<void*>(r.raw78);r.structureSize=Read<int32_t>(r.structure,0x60);
        r.structureAlignment=Read<int32_t>(r.structure,0x64);
    }
    return r;
}
inline void ObserveFunction(size_t index,const wchar_t* path,uintptr_t original,const Parameter* parameters,size_t count){
    auto& r=schema.functions[index];r={};r.path=path;r.expectedCount=count;r.object=engine::Find(path);
    r.expectedOuter=engine::Class(0x6c14c60);r.expectedExec=engine::image+original;
    r.readable=original_players::Readable(r.object,0xe8);
    r.type=Read<void*>(r.object,0x10);r.outer=Read<void*>(r.object,0x28);r.firstProperty=Read<void*>(r.object,0x58);
    r.propertiesSize=Read<int32_t>(r.object,0x60);r.alignment=Read<int32_t>(r.object,0x64);
    r.flags=Read<uint32_t>(r.object,0xb8);r.numParms=Read<uint8_t>(r.object,0xbc);
    r.parms=Read<uint16_t>(r.object,0xbe);r.returnOffset=Read<uint16_t>(r.object,0xc0);
    // Actual linked UFunction.ParmsSize, observed case75: Child144/Game136.
    // Both PropertiesSize/C++ storage spans remain144; no field layout changes.
    r.expectedParms=index==0?144:136;
    r.exec=Read<uintptr_t>(r.object,0xe0);r.nameBits=Read<uint64_t>(r.object,0x18);r.outerNameBits=Read<uint64_t>(r.outer,0x18);
    r.execCellWritable=r.object&&Writable(static_cast<uint8_t*>(r.object)+0xe0,8);
    // The SAME existing strict owner predicates, each observed independently.
    // Weak bookkeeping is deferred until metadata matches; the capture itself
    // never invokes a queried UFunction or assigns any original property.
    if(!r.object)r.ownerFailures|=1U;
    if(r.outer!=r.expectedOuter)r.ownerFailures|=2U;
    if(r.parms!=r.expectedParms)r.ownerFailures|=4U;
    if(r.flags!=0x04c22405U)r.ownerFailures|=8U;
    if(r.exec!=r.expectedExec)r.ownerFailures|=16U;
    if(!r.execCellWritable)r.ownerFailures|=32U;
    if(count>MaxParameters)r.ownerFailures|=128U;
    r.ownerMatches=r.ownerFailures==0;
    auto field=r.firstProperty;
    if(!r.readable)r.chainReadable=false;
    for(size_t n=0;field&&n<r.actualFields.size()&&r.readable;++n){
        bool repeated=false;
        for(size_t k=0;k<n;++k)if(r.actualFields[k].field==field)repeated=true;
        if(repeated){r.chainCycle=true;break;}
        auto actual=ObserveField(field);r.actualFields[n]=actual;++r.actualCount;
        if(!actual.readable){r.chainReadable=false;break;}
        field=actual.next;
    }
    if(field&&r.actualCount==r.actualFields.size()&&!r.chainCycle&&r.chainReadable)r.chainTruncated=true;
    r.parametersMatch=count<=MaxParameters;
    for(size_t n=0;n<count&&n<MaxParameters;++n){
        const auto& p=parameters[n];auto& row=r.parameters[n];row.expected=true;row.name=p.name;
        row.expectedOffset=p.offset;row.expectedBytes=p.size;row.expectedFlags=p.flags;
        original_jett_priming::PropertyInfo d{};
        if(r.readable&&r.chainReadable&&!r.chainCycle&&!r.chainTruncated)
            row.lookupOffset=original_jett_priming::FieldOffset(r.object,p.name,static_cast<size_t>(p.size),&d);
        row.actual=ObserveField(d.field);row.genericProperty=d.genericProperty;row.nameMatches=d.nameMatches;
        row.boundsValid=d.boundsValid;row.expectedNameBits=d.expectedName;
        if(!d.field)row.failures|=1U;
        if(row.lookupOffset!=p.offset)row.failures|=2U;
        if(d.offset!=p.offset)row.failures|=4U;
        if(d.arrayDim!=1)row.failures|=8U;
        if(d.elementSize!=p.size)row.failures|=16U;
        if(row.actual.flags!=p.flags)row.failures|=32U;
        if(!d.genericProperty)row.failures|=64U;
        if(!d.nameMatches)row.failures|=128U;
        if(!d.boundsValid)row.failures|=256U;
        row.matches=row.lookupOffset==p.offset&&d.arrayDim==1&&d.elementSize==p.size&&row.actual.flags==p.flags;
        if(!row.matches)r.parametersMatch=false;
        ++schema.expectedQueries;
    }
}
inline bool BindFunction(size_t index,const wchar_t* path,uintptr_t original,Exec replacement,const Parameter* parameters,size_t count){
    ObserveFunction(index,path,original,parameters,count);
    auto& r=schema.functions[index];auto& f=functions[index];f.object=r.object;
    f.original=engine::function<Exec>(original);f.replacement=replacement;
    if(r.ownerMatches&&r.parametersMatch){
        r.weakCaptureAttempted=true;r.weakCaptured=Capture(f.object,f.identity);
        if(!r.weakCaptured){r.ownerFailures|=64U;r.ownerMatches=false;}
    }
    if(!r.ownerMatches||!r.parametersMatch)return false;
    for(size_t i=0;i<count;++i)f.parameters[i]=r.parameters[i].actual.field;
    f.inputCount=static_cast<uint8_t>(count-1);return true;
}
inline bool Verify(){
    if(!Profile(0x6c43650,1086,0xa559439dce92d17dULL)||!Profile(0x6c43a90,942,0x92962a68f35b4487ULL)
       ||!Profile(0x43fada0,2189,0xbce8a0b9824dcd0aULL)||!Profile(0x3ef6cb0,5560,0xbe0c00750310708dULL)
       ||!Profile(0x786c070,5,0x901eb9f4d552ed79ULL)||!Profile(0x48544e0,53,0x697b1689807c0f8bULL)
       ||!Profile(0x4854515,81,0xfc27965aaf1f49a5ULL)||!Profile(0x4854566,25,0xc85397baf2674839ULL)
       ||!Profile(0x3ef8270,7995,0xcc1498c170de14e7ULL))
        return Fail(6933,"host_ability_spawn_original_null_wrapper_or_factory_profile_mismatch");
    state.profilesVerified=true;
    // Complete reflected runtime flags captured in original case75:
    // pair-20261103T214811.068802Z/host/bridge-state.json -> spawn.schema.
    // These are exact per-type values, not a mask over extra native bits.
    constexpr uint64_t objectFlags=0x18001000000280ULL,classFlags=0x1c001000000280ULL;
    constexpr uint64_t transformFlags=0x18001048000182ULL,returnFlags=0x18001000000780ULL;
    constexpr uint64_t collisionFlags=0x18001040000280ULL;
    const Parameter child[]{{L"WorldContextObject",0,8,objectFlags},{L"ActorClass",8,8,classFlags},
        {L"SpawnTransform",16,96,transformFlags},{L"CollisionHandlingOverride",112,1,collisionFlags},
        {L"InstigatedBy",120,8,objectFlags},{L"Owner",128,8,objectFlags},{L"ReturnValue",136,8,returnFlags}};
    const Parameter game[]{{L"WorldContextObject",0,8,objectFlags},{L"GameObjectClass",8,8,classFlags},
        {L"SpawnTransform",16,96,transformFlags},{L"InstigatedBy",112,8,objectFlags},
        {L"Owner",120,8,objectFlags},{L"ReturnValue",128,8,returnFlags}};
    // Observe BOTH complete signatures before reporting any mismatch. Keep
    // exact current acceptance predicates; observed differences are evidence,
    // not permission to strip flags or substitute another type's constants.
    const auto previousCaptures=schema.captures;schema={};
    const bool childBound=BindFunction(0,L"/Script/ShooterGame.ShooterBlueprintLibrary.SpawnAbilityChildActor",0x6c43650,&ChildExec,child,std::size(child));
    const bool gameBound=BindFunction(1,L"/Script/ShooterGame.ShooterBlueprintLibrary.SpawnGameObject",0x6c43a90,&GameObjectExec,game,std::size(game));
    for(const auto& function:schema.functions){
        if(!function.ownerMatches)++schema.ownerFailures;
        for(const auto& parameter:function.parameters)if(parameter.expected&&!parameter.matches)++schema.parameterFailures;
    }
    schema.at=GetTickCount64();schema.imageBase=engine::image;schema.captures=previousCaptures+1;Cache();
    if(!childBound||!gameBound)
        return Fail(6934,"host_ability_spawn_exact_native_parameter_schema_mismatch");
    auto transform=engine::Find(L"/Script/CoreUObject.Transform");
    if(!transform||Read<void*>(functions[0].parameters[2],0x78)!=transform||Read<void*>(functions[1].parameters[2],0x78)!=transform)
        return Fail(6942,"host_ability_spawn_reference_parameter_must_be_original_transform_struct");
    auto actor=engine::Class(0x3683900);original_jett_priming::PropertyInfo collision{};
    collisionOffset=original_jett_priming::FieldOffset(actor,L"SpawnCollisionHandlingMethod",1,&collision);
    if(collisionOffset!=0x226||collision.arrayDim!=1||collision.elementSize!=1
       ||original_jett_priming::FieldOffset(actor,L"bReplicates",1,&replicatesField)<0
       ||(replicatesField.castFlags&0x28001)!=0x28001||!replicatesField.boolFieldMask)
        return Fail(6935,"host_ability_spawn_original_actor_collision_or_replication_schema_mismatch");
    state.schemaVerified=true;return true;
}
inline bool Close(){
    if(!engine::image||!original_players::Thread()||state.active)return Fail(6936,"host_ability_spawn_close_requires_idle_original_game_thread");
    for(size_t n=functions.size();n>0;--n){auto& f=functions[n-1];if(!f.installed)continue;
        if(!Live(f.object,f.identity)||Read<Exec>(f.object,0xe0)!=f.replacement)return Fail(6937,"host_ability_spawn_original_function_restore_conflict");
        if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(static_cast<uint8_t*>(f.object)+0xe0),
            reinterpret_cast<void*>(f.original),reinterpret_cast<void*>(f.replacement))!=reinterpret_cast<void*>(f.replacement))
            return Fail(6938,"host_ability_spawn_original_function_restore_race");
        f.installed=false;
    }
    state.installed=false;state.closed=true;++state.closes;state.stage="host_ability_spawn_original_execs_restored";Cache();return true;
}
inline bool Install(void* world,const Hooks& value){
    if(!engine::image||!original_players::Thread())return false;
    if(state.installed)return state.world==world&&hooks.resolve==value.resolve&&hooks.validate==value.validate
        &&hooks.admit==value.admit&&hooks.prepare==value.prepare;
    if(state.error)return false;
    state.gameThread=GetCurrentThreadId();state.world=world;hooks=value;
    if(!hooks.resolve||!hooks.validate||!HostWorld(world)||!Capture(world,state.worldIdentity))
        return Fail(6939,"host_ability_spawn_requires_actual_host_and_original_registry_hooks");
    if(!Verify())return false;
    for(auto& f:functions){
        if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(static_cast<uint8_t*>(f.object)+0xe0),
            reinterpret_cast<void*>(f.replacement),reinterpret_cast<void*>(f.original))!=reinterpret_cast<void*>(f.original)){
            Fail(6940,"host_ability_spawn_native_exec_publication_conflict");Close();return false;
        }
        f.installed=true;
    }
    state.installed=true;state.closed=false;++state.installs;
    state.stage="host_ability_spawn_original_null_factory_boundary_installed_execution_unverified";Cache();return true;
}
// Root feeds these real actors to its existing original ActorChannel traversal.
// Only an actor's own native bReplicates flag admits it. This neither marks an
// actor replicated nor represents construction/registration as client receipt.
template<class Visit> inline bool VisitReplicationActors(Visit&& visit){
    if(!state.installed)return !state.error;
    if(!Thread()||!Live(state.world,state.worldIdentity))return false;
    for(auto& e:actors){
        if(!e.actor||!e.committed)continue;
        if(!Live(e.actor,e.identity)){e={};++state.pruned;continue;}
        if(ActorWorld(e.actor)!=state.world||Read<uint8_t>(e.actor,0x224)!=3)
            return Fail(6941,"host_ability_spawn_existing_actor_world_or_authority_changed");
        if(!Replicates(e.actor))continue;
        ++state.registryVisits;visit(e.actor);
    }
    Cache();return true;
}
struct SchemaReason {uint32_t bit;const char* text;};
inline void WriteSchemaReasons(FILE* file,uint32_t mask,const SchemaReason* reasons,size_t count){
    std::fputc('[',file);bool comma=false;
    for(size_t n=0;n<count;++n)if(mask&reasons[n].bit){std::fprintf(file,"%s\"%s\"",comma?",":"",reasons[n].text);comma=true;}
    std::fputc(']',file);
}
inline void WriteSchemaField(FILE* file,const FieldSchema& r){
    std::fprintf(file,"{\"field\":\"%p\",\"readable\":%s,\"field_class\":\"%p\",\"next\":\"%p\",\"owner_variant_bits\":\"0x%llx\",\"offset\":%d,\"bytes\":%d,\"dimensions\":%d,\"flags\":\"0x%llx\",\"cast_flags\":\"0x%llx\",\"name_bits\":\"0x%llx\",\"name_tail_at_28\":\"0x%x\",\"raw_at_78\":\"0x%llx\",\"raw_at_80\":\"0x%llx\",\"struct_at_78\":\"%p\",\"struct_size_at_60\":%d,\"struct_alignment_at_64\":%d}",
        r.field,r.readable?"true":"false",r.fieldClass,r.next,static_cast<unsigned long long>(r.ownerBits),r.offset,r.bytes,r.dimensions,
        static_cast<unsigned long long>(r.flags),static_cast<unsigned long long>(r.castFlags),static_cast<unsigned long long>(r.nameBits),r.nameTail,
        static_cast<unsigned long long>(r.raw78),static_cast<unsigned long long>(r.raw80),r.structure,r.structureSize,r.structureAlignment);
}
inline void WriteSchema(FILE* file,const SchemaSnapshot& s){
    auto b=[](bool v){return v?"true":"false";};
    std::fprintf(file,"{\"captures\":%llu,\"at_ms\":%llu,\"image_base\":\"0x%llx\",\"expected_named_queries\":%u,\"parameter_failures\":%u,\"owner_failures\":%u,\"queried_ufunctions_invoked\":0,\"capture_property_writes\":0,\"capture_exec_writes\":0,\"acceptance_relaxed\":false,\"functions\":[",
        s.captures,s.at,static_cast<unsigned long long>(s.imageBase),s.expectedQueries,s.parameterFailures,s.ownerFailures);
    if(s.captures)for(size_t index=0;index<s.functions.size();++index){const auto& r=s.functions[index];char path[256]{};
        if(r.path)WideCharToMultiByte(CP_UTF8,0,r.path,-1,path,int(sizeof(path)),nullptr,nullptr);
        const auto execRva=r.exec>=s.imageBase?r.exec-s.imageBase:r.exec;
        const auto expectedRva=r.expectedExec>=s.imageBase?r.expectedExec-s.imageBase:r.expectedExec;
        const auto expectedEnd=r.expectedCount&&r.expectedCount<=MaxParameters
            ?r.parameters[r.expectedCount-1].expectedOffset+r.parameters[r.expectedCount-1].expectedBytes:0;
        std::fprintf(file,"%s{\"path\":\"%s\",\"object\":\"%p\",\"readable\":%s,\"function_class\":\"%p\",\"outer\":\"%p\",\"expected_outer\":\"%p\",\"function_name_bits\":\"0x%llx\",\"outer_name_bits\":\"0x%llx\",\"first_property\":\"%p\",\"properties_size_at_60\":%d,\"alignment_at_64\":%d,\"parms_size_at_be\":%u,\"expected_parms_size\":%u,\"expected_last_field_end\":%d,\"num_parms_at_bc\":%u,\"return_offset_at_c0\":%u,\"function_flags\":\"0x%x\",\"expected_function_flags\":\"0x4c22405\",\"exec\":\"%p\",\"exec_rva\":\"0x%llx\",\"expected_exec_rva\":\"0x%llx\",\"exec_cell_writable_private\":%s,\"owner_matches\":%s,\"parameters_match\":%s,\"weak_capture_attempted\":%s,\"weak_captured\":%s,\"owner_failure_mask\":%u,\"owner_failure_reasons\":",
            index?",":"",path,r.object,b(r.readable),r.type,r.outer,r.expectedOuter,static_cast<unsigned long long>(r.nameBits),
            static_cast<unsigned long long>(r.outerNameBits),r.firstProperty,r.propertiesSize,r.alignment,unsigned(r.parms),unsigned(r.expectedParms),expectedEnd,
            unsigned(r.numParms),unsigned(r.returnOffset),r.flags,reinterpret_cast<void*>(r.exec),static_cast<unsigned long long>(execRva),
            static_cast<unsigned long long>(expectedRva),b(r.execCellWritable),b(r.ownerMatches),b(r.parametersMatch),
            b(r.weakCaptureAttempted),b(r.weakCaptured),r.ownerFailures);
        constexpr SchemaReason ownerReasons[]{{1,"function_not_found"},{2,"function_outer_mismatch"},{4,"function_parms_size_mismatch"},
            {8,"function_flags_mismatch"},{16,"function_native_exec_mismatch"},{32,"function_exec_cell_not_private_writable"},
            {64,"binding_weak_identity_capture_failed"},{128,"expected_parameter_table_capacity"}};
        WriteSchemaReasons(file,r.ownerFailures,ownerReasons,std::size(ownerReasons));
        std::fprintf(file,",\"expected_parameter_count\":%zu,\"recorded_named_slots\":%zu,\"named_parameters\":[",r.expectedCount,r.parameters.size());
        for(size_t n=0;n<r.parameters.size();++n){const auto& p=r.parameters[n];char name[128]{};
            if(p.name)WideCharToMultiByte(CP_UTF8,0,p.name,-1,name,int(sizeof(name)),nullptr,nullptr);
            std::fprintf(file,"%s{\"slot\":%zu,\"has_expected_parameter\":%s,\"name\":\"%s\",\"expected_offset\":%d,\"expected_bytes\":%d,\"expected_dimensions\":1,\"expected_flags\":\"0x%llx\",\"lookup_offset\":%d,\"expected_name_bits\":\"0x%llx\",\"generic_property\":%s,\"name_matches\":%s,\"bounds_valid\":%s,\"matches\":%s,\"flags_xor\":\"0x%llx\",\"failure_mask\":%u,\"failure_reasons\":",
                n?",":"",n,b(p.expected),name,p.expectedOffset,p.expectedBytes,static_cast<unsigned long long>(p.expectedFlags),p.lookupOffset,
                static_cast<unsigned long long>(p.expectedNameBits),b(p.genericProperty),b(p.nameMatches),b(p.boundsValid),b(p.matches),
                static_cast<unsigned long long>(p.actual.flags^p.expectedFlags),p.failures);
            constexpr SchemaReason parameterReasons[]{{1,"parameter_not_found_or_lookup_unsafe"},{2,"validated_lookup_offset_mismatch"},
                {4,"raw_parameter_offset_mismatch"},{8,"parameter_array_dimension_mismatch"},{16,"parameter_element_size_mismatch"},
                {32,"parameter_flags_mismatch"},{64,"parameter_generic_property_cast_mismatch"},{128,"parameter_name_mismatch"},
                {256,"parameter_outside_function_properties_size"}};
            WriteSchemaReasons(file,p.failures,parameterReasons,std::size(parameterReasons));
            std::fputs(",\"actual\":",file);WriteSchemaField(file,p.actual);std::fputc('}',file);
        }
        std::fprintf(file,"],\"actual_field_count\":%zu,\"chain_readable\":%s,\"chain_cycle\":%s,\"chain_truncated\":%s,\"actual_fields\":[",
            r.actualCount,b(r.chainReadable),b(r.chainCycle),b(r.chainTruncated));
        for(size_t n=0;n<r.actualCount;++n){if(n)std::fputc(',',file);WriteSchemaField(file,r.actualFields[n]);}
        std::fputs("]}",file);
    }
    std::fputs("]}",file);
}
inline void Write(FILE* f){
    const auto diagnostics=Diagnostics();const auto& s=diagnostics.counters;auto b=[](bool v){return v?"true":"false";};
    std::fprintf(f,"{\"stage\":\"%s\",\"error\":%u,\"installed\":%s,\"source_profiles\":%s,\"native_schema\":%s,\"active\":%u,\"calls\":%llu,\"original_forwards\":%llu,\"eligible_calls\":%llu,\"class_rejections\":%llu,\"source_deferrals\":%llu,\"frame_steps\":%llu,\"script_steps\":%llu,\"compiled_steps\":%llu,\"factories\":%llu,\"finished\":%llu,\"returned\":%llu,\"spawn_failures\":%llu,\"discarded\":%llu,\"pruned\":%llu,\"owner_readbacks\":%llu,\"instigator_readbacks\":%llu,\"registry_visits\":%llu,\"authored_nonreplicated\":%llu,\"receipt_allocations\":%llu,\"owned_child_source_queries\":%llu,\"owned_child_source_accepted\":%llu,\"native_pawn_null_instigator_to_self\":%llu,\"native_completed_after_source_retired\":%llu,\"proven_starter_call_contracts\":5,\"external_projectile_source_receipts\":false,\"original_image_writes\":0,\"role_writes\":0,\"replication_flag_writes\":0,\"arguments_replayed\":false,\"network_execution_verified\":false,\"schema\":",
        s.stage,s.error,b(s.installed),b(s.profilesVerified),b(s.schemaVerified),s.active,s.calls,s.originalForwards,s.eligibleCalls,
        s.classRejections,s.sourceDeferrals,s.frameSteps,s.scriptSteps,s.compiledSteps,s.factories,s.finished,s.returned,s.spawnFailures,
        s.discarded,s.pruned,s.ownerReadbacks,s.instigatorReadbacks,s.registryVisits,s.nonreplicated,
        s.receipts,s.childSourceQueries,s.childSourceAccepted,s.nativePawnNullInstigatorToSelf,s.nativeCompletedAfterSourceRetired);
    WriteSchema(f,diagnostics.metadata);std::fputc('}',f);
}
} // namespace local_server_ability_spawn
