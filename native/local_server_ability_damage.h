#pragma once
#include "local_server_hero_abilities.h"
#include "local_server_combat.h"
#include <array>
#include <cstdio>
#include <cstring>

// Actual authority Actor.TakeDamage observations only. Original actor filters,
// native world/radial queries, event geometry and callbacks run exactly once.
// The adapter supplies no projectile, target list, area test, falloff or damage
// amount. It queues only the actual accepted native return with unchanged life.
// Static provenance: work/local-server-playable/hero-abilities/group0-damage.
namespace local_server_ability_damage {
using Weak=original_inventory_slots::WeakIdentity;
using Source=local_server_hero_abilities::SourceView;
using Life=local_server_combat::melee::LifeSnapshot;
using NativeLife=local_server_combat::firearms::NativeLife;
using Kind=original_damage::NativeEventKind;
using Geometry=original_damage::NativeEventGeometry;
using TakeDamageFn=float(*)(void*,float,const void*,void*,void*);
inline constexpr size_t MaxPlayers=original_players::MaxPlayers,MaxHits=128,MaxQueue=256,MaxDepth=8;
inline constexpr size_t PawnEntries=0xae0/8,TakeDamageSlot=0x6b8/8;

struct Contact {
    uint64_t occurrence=0,token=0,sourceReceipt=0;uint32_t victim=0,victimEpoch=0;
    Source source{};Life before{};NativeLife nativeBefore{};
    void *victimPawn=nullptr,*causer=nullptr,*instigator=nullptr,*damageType=nullptr;
    Weak victimId{},causerId{},damageTypeId{};
    float requested=0,accepted=0;Geometry geometry{};
};
enum class Opportunity:uint8_t {Defer,Handled,Failure};
struct Hooks {
    // Resolver must be a read-only exact-current registry query. The root may
    // combine the authored loadout and the separately owned child-spawn registry.
    bool (*resolveSource)(void* actualCauser,Source& result)=nullptr;
    // Optional immutable registry receipt for a real spawned causer. A direct
    // authored source may use zero; re-registration must not revive old hits.
    bool (*captureSourceReceipt)(void*,const Source&,uint64_t&)=nullptr;
    uint64_t (*allocateToken)()=nullptr;
    // Root owns policy Begin/EndNativeDamage, typed remote multicast and death
    // retirement. True may also mean the native sections rejected the amount.
    bool (*commit)(const Contact&,double,original_damage::Result&)=nullptr;
    // The actual human authority input module already owns this pawn's table.
    // Compose ONLY its +6B8 entry rather than cloning that dynamic table.
    bool (*installSharedTakeDamage)(void*,TakeDamageFn,TakeDamageFn&)=nullptr;
    bool (*clearSharedTakeDamage)(void*,TakeDamageFn)=nullptr;
    // Optional. The root must first prove no active native damage transaction
    // or weapon-observer scope. It may commit while the real caller/causer is
    // still live after original TakeDamage returned. Defer must be read-only.
    Opportunity (*tryCommitBeforeCallerCleanup)(const Contact&,double,original_damage::Result&)=nullptr;
    // Optional compositor gate: the lower input/combat Pawn table must already
    // belong to this exact current epoch before an upper observer is installed.
    bool (*victimObserverReady)(size_t)=nullptr;
    // Read-only global transaction/order gate. A false return retains the
    // queued contact for a later safe boundary rather than dropping/replaying.
    bool (*commitBoundaryReady)()=nullptr;
};
struct Binding {
    bool used=false,sharedTable=false,counted=false,retirePending=false;
    uint32_t epoch=0;size_t index=0;uint64_t inFlight=0;
    void *pc=nullptr,*ps=nullptr,*pawn=nullptr;Weak pcId{},psId{},pawnId{};
    void **previous=nullptr,**copy=nullptr;TakeDamageFn original=nullptr;
};
struct Entry {bool used=false;Contact contact{};std::array<original_damage::NativeHitResult,MaxHits> hits{};};
struct Frame {bool capture=false;Contact contact{};std::array<original_damage::NativeHitResult,MaxHits> hits{};};
struct State {
    bool initialized=false,profilesVerified=false,processing=false,restartRequired=false;
    bool immediateEnabled=false,inImmediateCommit=false,closePending=false;
    void* world=nullptr;DWORD gameThread=0;unsigned error=0,boundTargets=0,waitingTargets=0;
    uint64_t installs=0,retirements=0,nativeCalls=0,nativeReturns=0,accepted=0,queued=0;
    uint64_t applied=0,kills=0,notApplied=0,unsupportedPointEvents=0,unsupportedEvents=0;
    uint64_t unresolvedSources=0,excludedExistingSources=0,invalidEvents=0,invalidSources=0;
    uint64_t rejectedNative=0,nativeLifeChanges=0,staleSources=0,staleVictims=0;
    uint64_t pendingDeferrals=0,baselineUpdates=0,ownedLifeAdvances=0,queueOverflow=0,depthOverflow=0;
    uint64_t retainedPendingTables=0;
    uint64_t immediateAttempts=0,immediateHandled=0,immediateApplied=0,immediateDeferred=0,immediateUnavailable=0;
    uint64_t immediateOrderDeferrals=0;
    uint64_t deferredRetirements=0,flushedRetirements=0,immediateToken=0;
    uint64_t activeWrappers=0;
    uint64_t readCursor=0,writeCursor=0,occurrence=0;
    uintptr_t exceptionRva=0;const char* stage="ability_damage_not_initialized";
};
inline State state{},cached{};inline Hooks hooks{};
inline SRWLOCK cacheLock=SRWLOCK_INIT;
inline std::array<Binding,MaxPlayers> bindings{};
inline std::array<Entry,MaxQueue> queue{};
// BSS frames avoid copying 30 KB of original radial hits onto the game stack.
inline std::array<Frame,MaxDepth> frames{};
inline size_t depth=0;
inline volatile LONG64 activeWrappers=0;
inline uint64_t WrapperCount(){return static_cast<uint64_t>(InterlockedCompareExchange64(&activeWrappers,0,0));}
inline void Cache(){state.activeWrappers=WrapperCount();AcquireSRWLockExclusive(&cacheLock);cached=state;ReleaseSRWLockExclusive(&cacheLock);}
inline const State& GetState(){return state;}
inline State Snapshot(){AcquireSRWLockShared(&cacheLock);auto s=cached;ReleaseSRWLockShared(&cacheLock);return s;}
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Live(void* p,const Weak& id){return p&&original_inventory_slots::ResolveIdentity(id)==p;}
inline bool Identity(void* p,Weak& id){return original_inventory_slots::CaptureIdentity(p,id)&&Live(p,id);}
inline bool Thread(){return state.initialized&&!state.restartRequired&&engine::image
    &&GetCurrentThreadId()==state.gameThread&&original_players::Thread();}
inline bool CleanupThread(){return state.initialized&&engine::image&&state.gameThread
    &&GetCurrentThreadId()==state.gameThread&&original_players::gameThread==state.gameThread;}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;Cache();return false;}
inline LONG Capture(EXCEPTION_POINTERS* ex){
    const auto at=ex&&ex->ExceptionRecord?reinterpret_cast<uintptr_t>(ex->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;state.restartRequired=true;
    Fail(6320,"ability_damage_observer_exception_requires_stop");return EXCEPTION_EXECUTE_HANDLER;
}
inline bool SameSource(const Source& a,const Source& b){
    if(a.contextObject!=b.contextObject||a.world!=b.world||a.controller!=b.controller||a.playerState!=b.playerState
       ||a.pawn!=b.pawn||a.inventory!=b.inventory||a.ability!=b.ability||a.heroIndex!=b.heroIndex
       ||a.sourceEpoch!=b.sourceEpoch||a.registration!=b.registration||a.pendingGrant||b.pendingGrant)return false;
    for(size_t i=0;i<a.identities.size();++i)
        if(!original_inventory_slots::SameIdentity(a.identities[i],b.identities[i]))return false;
    return true;
}
inline bool CurrentSource(const Contact& c){
    if(!hooks.resolveSource||!Live(c.causer,c.causerId)||!Live(c.damageType,c.damageTypeId))return false;
    Source now{};uint64_t receipt=0;
    return hooks.resolveSource(c.causer,now)&&SameSource(c.source,now)
        &&(!hooks.captureSourceReceipt||(hooks.captureSourceReceipt(c.causer,now,receipt)&&receipt==c.sourceReceipt))
        &&now.world==state.world&&c.instigator==now.controller&&Read<uint8_t>(c.causer,0x224)==3;
}
inline bool Victim(const Binding& b){
    const auto p=local_server_match::Player(b.index);
    return b.used&&p&&p->present&&p->epoch==b.epoch&&p->pc==b.pc&&p->ps==b.ps&&p->pawn==b.pawn
        &&Live(b.pc,b.pcId)&&Live(b.ps,b.psId)&&Live(b.pawn,b.pawnId)
        &&local_server_combat::policy::Authority(b.index,b.pawn,b.pc);
}
inline bool CurrentVictim(const Contact& c){
    if(c.victim>=bindings.size())return false;const auto& b=bindings[c.victim];
    return b.pawn==c.victimPawn&&b.epoch==c.victimEpoch&&Victim(b)&&Live(c.victimPawn,c.victimId);
}
inline bool EqualLife(const Life& a,const Life& b){
    return a.valid&&b.valid&&a.pawn==b.pawn&&a.epoch==b.epoch&&a.handler==b.handler&&a.health==b.health
        &&a.shield==b.shield&&a.healthValue==b.healthValue&&a.shieldValue==b.shieldValue
        &&a.eventIndex==b.eventIndex&&a.pendingEvents==b.pendingEvents&&a.alive==b.alive;
}
inline bool VerifyProfiles(){
    if(state.profilesVerified)return true;
    struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
    constexpr Profile profiles[]{
        {0x787b3d0,421,0x416bd6f3c997e56fULL},{0x6f9fc00,167,0x22f9f6af08dc6e2bULL},
        {0x6f9fcb0,374,0xc29e5126e0dd0512ULL},
        {0x437af30,75,0xd33ce03aa7f092f1ULL},{0x437b290,276,0xffeacb2858544b3fULL}
    };
    for(const auto& p:profiles){auto bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes))return Fail(6321,"ability_damage_native_profile_unreadable");
        uint64_t hash=1469598103934665603ULL;for(size_t i=0;i<p.bytes;++i)hash=(hash^bytes[i])*1099511628211ULL;
        if(hash!=p.fnv)return Fail(6322,"ability_damage_native_take_damage_profile_mismatch");
    }
    struct Table {uintptr_t rva,typeId,isType,bestHit;};
    constexpr Table tables[]{
        {0xaade850,0x18a3ed0,0x410a2f0,0x437ce60},
        {0xaae04f0,0x18be7f0,0x410a330,0x437d160},
        {0xab31980,0x1914910,0x437c650,0x437d210}
    };
    for(const auto& t:tables){auto table=reinterpret_cast<void*>(engine::image+t.rva);
        if(Read<uintptr_t>(table,8)!=engine::image+t.typeId||Read<uintptr_t>(table,16)!=engine::image+t.isType
           ||Read<uintptr_t>(table,24)!=engine::image+t.bestHit)
            return Fail(6323,"ability_damage_original_damage_event_dispatch_table_mismatch");
    }
    if(!original_damage::VerifyNativeEventResponseProfiles())
        return Fail(original_damage::State().error,original_damage::State().stage);
    state.profilesVerified=true;return true;
}
inline bool ReadGeometry(Contact& c,const void* nativeEvent,Frame& frame){
    if(!nativeEvent||!original_players::Readable(nativeEvent,16))return false;
    const auto table=Read<uintptr_t>(nativeEvent);auto type=Read<void*>(nativeEvent,8);
    if(type!=c.damageType||!original_players::Derived(type,engine::Class(original_damage::DamageTypeClass)))return false;
    if(table==engine::image+0xaade850)c.geometry.kind=Kind::Base;
    else if(table==engine::image+0xab31980)c.geometry.kind=Kind::Radial;
    else if(table==engine::image+0xaae04f0){++state.unsupportedPointEvents;return false;}
    else{++state.unsupportedEvents;return false;}
    if(c.geometry.kind==Kind::Radial){
        if(!original_players::Readable(nativeEvent,sizeof(original_damage::NativeRadialEvent)))return false;
        c.geometry.radial=Read<original_damage::NativeRadialEvent>(nativeEvent);
        const auto& params=c.geometry.radial.params;
        if(!std::isfinite(params.base)||!std::isfinite(params.minimum)||!std::isfinite(params.inner)
           ||!std::isfinite(params.outer)||!std::isfinite(params.falloff))return false;
        auto& array=c.geometry.radial.componentHits;
        if(array.count<0||array.count>static_cast<int32_t>(MaxHits)||array.capacity<array.count
           ||(array.count&&!original_players::Readable(array.data,static_cast<size_t>(array.count)*sizeof(frame.hits[0]))))return false;
        if(array.count)std::memcpy(frame.hits.data(),array.data,static_cast<size_t>(array.count)*sizeof(frame.hits[0]));
        array.data=frame.hits.data();array.capacity=static_cast<int32_t>(MaxHits);
    }
    original_damage::NativeHitResult best{};engine::Vec direction{};
    using BestHit=void(*)(const void*,void*,void*,void*,engine::Vec*);
    const auto bestHit=Read<BestHit>(reinterpret_cast<void*>(table),24);
    bestHit(nativeEvent,c.victimPawn,c.causer,&best,&direction);
    auto& hit=c.geometry.hit;hit.hit=true;std::memcpy(hit.nativeHit,best.bytes,sizeof(hit.nativeHit));
    hit.component=original_inventory_slots::ResolveIdentity(Read<Weak>(best.bytes,0xd0));
    std::memcpy(hit.bone.bytes,best.bytes+0xd8,sizeof(hit.bone.bytes));
    hit.location=Read<engine::Vec>(best.bytes,0x28);hit.normal=Read<engine::Vec>(best.bytes,0x58);
    hit.distance=Read<float>(best.bytes,8);hit.blocking=(best.bytes[0xad]&1)!=0;
    // The native handle getter's cold branch can touch an instance manager and
    // write the handle. Our current continuation supports only its actual
    // direct-actor hot branch; do not invoke it for an unknown/stale handle.
    if(Read<int32_t>(best.bytes,0xc0)!=0||Read<int32_t>(best.bytes,0xc4)!=0
       ||Read<int32_t>(best.bytes,0xc8)!=-1||Read<uint32_t>(best.bytes,0xcc)!=0
       ||original_inventory_slots::ResolveIdentity(Read<Weak>(best.bytes,0xb8))!=c.victimPawn)return false;
    auto hitActor=engine::function<void*(*)(const void*)>(0x437b290)(best.bytes+0xb8);
    if((hitActor&&hitActor!=c.victimPawn)||(hit.component&&Read<void*>(hit.component,0xc8)!=c.victimPawn)
       ||(c.geometry.kind==Kind::Radial&&(!hit.component||hitActor!=c.victimPawn)))return false;
    if(c.geometry.kind==Kind::Radial)c.geometry.origin=c.geometry.radial.origin;
    else{
        // Native Base GetBestHitInfo does not populate TraceStart. Base events
        // have no origin member, so the missing host record uses the actual
        // causer's original native actor-location query, never the zero buffer.
        if(!original_damage::Event(c.causer,L"/Script/Engine.Actor.K2_GetActorLocation",24,&c.geometry.origin))return false;
    }
    c.geometry.direction=direction;
    if(!original_players::Finite(hit.location)||!original_players::Finite(hit.normal)
       ||!original_players::Finite(c.geometry.origin)||!original_players::Finite(direction)
       ||!std::isfinite(hit.distance)||hit.distance<0)return false;
    // This asks the original character to classify its canonical event hit.
    // No capsule-to-bone guess or manual region multiplier is introduced.
    alignas(8) uint8_t regional[0xf8]{};std::memcpy(regional,best.bytes,sizeof(best.bytes));
    if(!original_damage::Event(c.victimPawn,L"/Script/ShooterGame.ShooterCharacter.GetRegionalDamage",0xf2,regional)
       ||regional[0xf1]>2)return false;
    hit.region=regional[0xf1];return true;
}
inline bool Prepare(Binding& binding,float amount,const void* nativeEvent,void* instigator,void* causer,Frame& frame){
    frame.capture=false;frame.contact={};
    if(!Thread()||state.error||!Victim(binding)||!hooks.resolveSource||!nativeEvent||!causer
       ||!std::isfinite(amount)||amount<=0||amount>1000000)return false;
    auto& c=frame.contact;
    if(!hooks.resolveSource(causer,c.source)){++state.unresolvedSources;return false;}
    if(hooks.captureSourceReceipt&&!hooks.captureSourceReceipt(causer,c.source,c.sourceReceipt)){
        ++state.unresolvedSources;return false;
    }
    if(c.source.pendingGrant||c.source.world!=state.world||instigator!=c.source.controller
       ||!c.source.ability||Read<uint8_t>(causer,0x224)!=3){++state.invalidSources;return false;}
    // Existing gun/melee and Jett dagger producers retain sole ownership of
    // their already evidenced canonical point records.
    const auto slot=Read<uint8_t>(c.source.ability,0x4a0);
    if(slot==0||slot==1||slot==2||slot==10
       ||(c.source.heroIndex==0&&c.source.ability
          &&Read<void*>(c.source.ability,0x10)==engine::Find(original_jett::SkillPaths[3]))){
        ++state.excludedExistingSources;return false;
    }
    c.victim=static_cast<uint32_t>(binding.index);c.victimEpoch=binding.epoch;c.victimPawn=binding.pawn;
    c.victimId=binding.pawnId;c.causer=causer;c.instigator=instigator;c.damageType=Read<void*>(nativeEvent,8);c.requested=amount;
    if(!Identity(causer,c.causerId)||!Identity(c.damageType,c.damageTypeId)
       ||!local_server_combat::melee::ReadLife(binding.index,c.before)||c.before.pendingEvents!=0
       ||!local_server_combat::firearms::ReadNativeLife(binding.index,c.nativeBefore)||c.nativeBefore.pending!=0
       ||!ReadGeometry(c,nativeEvent,frame)){++state.invalidEvents;return false;}
    if(state.occurrence==UINT64_MAX)return Fail(6324,"ability_damage_native_occurrence_exhausted");
    c.occurrence=++state.occurrence;frame.capture=true;return true;
}
inline bool RecordCommit(const Contact& c,const original_damage::Result& result,bool immediate){
    if(result.applied&&(!result.originalResponse||!result.originalLifeReadback||!result.response))
        return Fail(6349,"ability_damage_root_commit_missing_original_response_receipt");
    if(!result.applied||result.duplicate){if(!result.duplicate)++state.notApplied;return true;}
    ++state.applied;if(result.killed)++state.kills;if(immediate)++state.immediateApplied;
    Life after{};const auto& receipt=original_damage::targets[c.victim];
    if(!result.killed&&result.originalResponse&&result.originalLifeReadback
       &&local_server_combat::melee::ReadLife(c.victim,after)
       &&after.pawn==c.before.pawn&&after.epoch==c.before.epoch&&after.handler==c.before.handler
       &&after.health==c.before.health&&after.shield==c.before.shield&&after.alive
       &&std::abs(after.healthValue-result.healthAfter)<.01f
       &&std::abs(after.shieldValue-result.shieldAfter)<.01f&&after.pendingEvents==0
       &&receipt.lastToken==c.token&&after.eventIndex==receipt.lastNativeEventIndex){
        const auto before=c.before;
        for(auto cursor=state.readCursor;cursor<state.writeCursor;++cursor){auto& later=queue[cursor%queue.size()];
            if(later.used&&later.contact.token!=c.token&&later.contact.victim==c.victim
               &&later.contact.victimEpoch==c.victimEpoch&&EqualLife(later.contact.before,before)){
                later.contact.before=after;++state.baselineUpdates;
            }
        }
    }return true;
}
inline Opportunity InvokeImmediate(const Contact& c,double now,original_damage::Result& result){
    state.inImmediateCommit=true;state.immediateToken=c.token;Opportunity outcome=Opportunity::Failure;
    __try{outcome=hooks.tryCommitBeforeCallerCleanup(c,now,result);}
    __finally{
        if(AbnormalTermination()&&local_server_match::pending.active&&local_server_match::pending.token==c.token)
            local_server_match::CancelNativeDamage();
        state.inImmediateCommit=false;state.immediateToken=0;
    }
    if(local_server_match::pending.active&&local_server_match::pending.token==c.token){
        local_server_match::CancelNativeDamage();
        Fail(6346,"ability_damage_immediate_root_transaction_not_closed");return Opportunity::Failure;
    }
    return outcome;
}
inline void After(Frame& frame,float accepted){
    if(!frame.capture||state.error||!Thread())return;auto& c=frame.contact;c.accepted=accepted;
    if(!std::isfinite(accepted)||accepted<=0||accepted>1000000){++state.rejectedNative;return;}
    ++state.accepted;
    if(!CurrentSource(c)){++state.staleSources;return;}
    if(!CurrentVictim(c)){++state.staleVictims;return;}
    if(!local_server_combat::melee::SameLife(c.before)){++state.nativeLifeChanges;return;}
    if(!hooks.allocateToken||(c.token=hooks.allocateToken())==0){Fail(6326,"ability_damage_shared_damage_token_unavailable");return;}
    if(hooks.tryCommitBeforeCallerCleanup){
        // Preserve global accepted-return order, including kill ordinals and
        // match-end ordering across different victims/sources.
        const bool earlier=state.readCursor!=state.writeCursor;
        if(earlier)++state.immediateOrderDeferrals;
        if(!earlier&&WrapperCount()==1&&depth==1&&!state.processing&&!state.inImmediateCommit&&!local_server_match::pending.active){
            ++state.immediateAttempts;original_damage::Result result{};
            const auto outcome=InvokeImmediate(c,local_server_match::Now(),result);
            if(outcome==Opportunity::Failure){if(!state.error)Fail(6347,"ability_damage_immediate_root_commit_failed");return;}
            if(outcome==Opportunity::Handled){++state.immediateHandled;RecordCommit(c,result,true);return;}
            if(outcome!=Opportunity::Defer){Fail(6350,"ability_damage_immediate_root_disposition_unknown");return;}
            if(result.applied||result.killed||result.duplicate||result.originalResponse||result.originalLifeReadback
               ||result.originalDeathMontage||result.response||result.montage||result.healthBefore!=0
               ||result.healthAfter!=0||result.taken!=0||result.shieldBefore!=0||result.shieldAfter!=0
               ||result.healthTaken!=0||result.shieldTaken!=0||result.sectionChanges!=0||result.region!=0){
                Fail(6348,"ability_damage_deferred_root_hook_returned_mutated_result");return;
            }
            ++state.immediateDeferred;
            if(!CurrentSource(c)){++state.staleSources;return;}
            if(!CurrentVictim(c)){++state.staleVictims;return;}
            if(!local_server_combat::melee::SameLife(c.before)){++state.nativeLifeChanges;return;}
        }else ++state.immediateUnavailable;
    }
    if(state.writeCursor-state.readCursor>=queue.size()){++state.queueOverflow;Fail(6325,"ability_damage_actual_native_event_queue_full");return;}
    auto& entry=queue[state.writeCursor%queue.size()];entry.used=true;entry.contact=c;
    if(c.geometry.kind==Kind::Radial){const auto count=c.geometry.radial.componentHits.count;
        if(count)std::memcpy(entry.hits.data(),frame.hits.data(),static_cast<size_t>(count)*sizeof(frame.hits[0]));
        entry.contact.geometry.radial.componentHits={entry.hits.data(),count,static_cast<int32_t>(MaxHits)};
    }
    ++state.writeCursor;++state.queued;
}
inline void PrepareSafe(Binding& b,float amount,const void* event,void* instigator,void* causer,Frame& frame){
    __try{Prepare(b,amount,event,instigator,causer,frame);}__except(Capture(GetExceptionInformation())){frame.capture=false;}
}
inline void AfterSafe(Frame& frame,float accepted){__try{After(frame,accepted);}__except(Capture(GetExceptionInformation())){}}
inline bool FlushDeferredRetirements();
__declspec(noinline) inline float Observe(void* pawn,float amount,const void* event,void* instigator,void* causer){
    // Count fallback wrappers as well: a stopped observer can still be on the
    // original native stack while Close is requested. No binding is replaced
    // or backing released until every entered wrapper has returned.
    InterlockedIncrement64(&activeWrappers);
    if(!state.gameThread||GetCurrentThreadId()!=state.gameThread){
        // Native gameplay admission is game-thread-only. A foreign-thread
        // fallback forwards the one proven native entry without touching the
        // game-thread-owned binding records or attempting an authority append.
        float accepted=0;
        __try{accepted=engine::function<TakeDamageFn>(0x787b3d0)(pawn,amount,event,instigator,causer);}
        __finally{InterlockedDecrement64(&activeWrappers);}
        return accepted;
    }
    Binding* binding=nullptr;for(auto& b:bindings)if(b.used&&b.pawn==pawn){binding=&b;break;}
    const auto original=binding?binding->original:engine::function<TakeDamageFn>(0x787b3d0);
    if(binding)InterlockedIncrement64(reinterpret_cast<volatile LONG64*>(&binding->inFlight));
    const bool observe=binding&&Thread();
    if(observe)++state.nativeCalls;
    Frame* frame=nullptr;if(observe){if(depth<frames.size())frame=&frames[depth++];else ++state.depthOverflow;}
    if(frame)PrepareSafe(*binding,amount,event,instigator,causer,*frame);
    float accepted=0;
    __try{accepted=original(pawn,amount,event,instigator,causer);if(observe)++state.nativeReturns;
        if(frame)AfterSafe(*frame,accepted);}
    __finally{
        if(frame)--depth;
        if(binding)InterlockedDecrement64(reinterpret_cast<volatile LONG64*>(&binding->inFlight));
        if(InterlockedDecrement64(&activeWrappers)==0&&state.initialized
           &&GetCurrentThreadId()==state.gameThread)FlushDeferredRetirements();
    }
    return accepted;
}
inline bool RestorePrivateDamageEntry(Binding& b){
    if(!b.copy)return false;
    DWORD previous=0;
    if(!VirtualProtect(b.copy-1,(PawnEntries+1)*8,PAGE_READWRITE,&previous))
        return Fail(6342,"ability_damage_owned_table_restore_write_protection_failed");
    auto slot=reinterpret_cast<void* volatile*>(b.copy+TakeDamageSlot);
    auto actual=InterlockedCompareExchangePointer(slot,reinterpret_cast<void*>(b.original),reinterpret_cast<void*>(&Observe));
    DWORD ignored=0;const bool protectedAgain=VirtualProtect(b.copy-1,(PawnEntries+1)*8,PAGE_READONLY,&ignored)!=0;
    if((actual!=reinterpret_cast<void*>(&Observe)&&actual!=reinterpret_cast<void*>(b.original))||!protectedAgain
       ||Read<TakeDamageFn>(b.copy,0x6b8)!=b.original)
        return Fail(6343,"ability_damage_owned_table_original_entry_restore_failed");
    return true;
}
inline bool RetireImpl(size_t index){
    if(index>=bindings.size()||!bindings[index].used)return true;
    if(!CleanupThread())return false;auto& b=bindings[index];
    if(WrapperCount()||b.inFlight){
        if(!b.retirePending){b.retirePending=true;++state.deferredRetirements;}
        state.stage="ability_damage_waiting_outermost_native_return_to_retire";return true;
    }
    if(b.sharedTable){
        if(!hooks.clearSharedTakeDamage||!hooks.clearSharedTakeDamage(b.pawn,&Observe))
            return Fail(6340,"ability_damage_owned_input_slot_restore_failed");
    }else{
        // Weak Get rejects pending-kill before actual destruction. Restore only
        // our owned slot first. Never read that retired UObject or free a table
        // which it may still reference; Windows releases this small retained
        // allocation with the owned process. A genuinely live pawn can restore
        // its original vptr and release the allocation immediately.
        if(!RestorePrivateDamageEntry(b))return false;
        if(Live(b.pawn,b.pawnId)){
            if(Read<void**>(b.pawn)!=b.copy)return Fail(6328,"ability_damage_retire_later_vtable_layer_first");
            if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(b.pawn),b.previous,b.copy)!=b.copy)
                return Fail(6329,"ability_damage_owned_vtable_restore_conflict");
            VirtualFree(b.copy-1,0,MEM_RELEASE);
        }else ++state.retainedPendingTables;
    }
    const bool counted=b.counted;b={};
    if(counted&&state.boundTargets)--state.boundTargets;++state.retirements;return true;
}
inline bool Retire(size_t index){__try{return RetireImpl(index);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool FinishClose(){
    if(WrapperCount()||depth||state.processing||state.inImmediateCommit)return false;
    for(const auto& binding:bindings)if(binding.used)return false;
    for(auto& entry:queue)entry.used=false;state.readCursor=state.writeCursor;
    state.initialized=false;state.closePending=false;hooks={};
    state.stage=state.retainedPendingTables?"ability_damage_original_slots_restored_pending_tables_retained":
        "ability_damage_original_tables_restored";Cache();return true;
}
inline bool FlushDeferredRetirementsImpl(){
    if(WrapperCount()||depth||!CleanupThread())return false;
    for(size_t i=0;i<bindings.size();++i)if(bindings[i].used&&bindings[i].retirePending){
        if(!Retire(i))return false;++state.flushedRetirements;
    }
    if(state.closePending)return FinishClose();
    return true;
}
inline bool FlushDeferredRetirements(){
    __try{return FlushDeferredRetirementsImpl();}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool HasDeferredRetirements(){
    for(const auto& binding:bindings)if(binding.used&&binding.retirePending)return true;
    return false;
}
inline bool Install(size_t index){
    if(index>=bindings.size()||!Thread()||!VerifyProfiles())return false;
    if(WrapperCount()||depth||state.inImmediateCommit||state.processing){++state.waitingTargets;return true;}
    auto player=local_server_match::Player(index);
    if(!player||!player->present||!player->ready||!player->epoch||!player->pawn||!player->alive
       ||player->phase!=local_server_match::Phase::Alive)return Retire(index);
    if(hooks.victimObserverReady&&!hooks.victimObserverReady(index)){++state.waitingTargets;return true;}
    auto& b=bindings[index];if(b.used){
        if(b.epoch==player->epoch&&b.pawn==player->pawn&&Victim(b)){
            if(Read<void**>(b.pawn)!=b.copy)return Fail(6330,"ability_damage_actual_pawn_table_changed_inside_epoch");
            return true;
        }
        if(!Retire(index))return false;
        // A successful deferred retirement request is not permission to
        // overwrite the backing still used by the active original wrapper.
        if(b.used){++state.waitingTargets;return true;}
    }
    Binding next{};next.used=true;next.index=index;next.epoch=player->epoch;
    next.pc=player->pc;next.ps=player->ps;next.pawn=player->pawn;
    if(!Identity(next.pc,next.pcId)||!Identity(next.ps,next.psId)||!Identity(next.pawn,next.pawnId)||!Victim(next))
        return Fail(6331,"ability_damage_actual_authority_victim_identity_invalid");
    next.previous=Read<void**>(next.pawn);next.original=Read<TakeDamageFn>(next.previous,0x6b8);
    if(next.original!=engine::function<TakeDamageFn>(0x787b3d0)
       ||!original_players::Readable(next.previous-1,(PawnEntries+1)*8))
        return Fail(6332,"ability_damage_original_take_damage_or_table_extent_mismatch");
    if(player->human){
        const auto& input=local_server_input::GetState();
        if(!hooks.installSharedTakeDamage||!hooks.clearSharedTakeDamage||!input.installed
           ||input.epochRetired||input.pawn!=next.pawn){++state.waitingTargets;return true;}
        // Publish a recoverable callback Binding before the shared slot can
        // become active. Even a later protection/readback failure retains its
        // original forward target and can run Clear during shutdown.
        next.sharedTable=true;next.copy=next.previous;b=next;
        TakeDamageFn forward=nullptr;
        const bool installed=hooks.installSharedTakeDamage(next.pawn,&Observe,forward);
        const auto actual=Read<TakeDamageFn>(b.copy,0x6b8);
        if(actual==&Observe){b.counted=true;++state.installs;++state.boundTargets;}
        if(!installed){
            if(actual==next.original)b={};
            else if(actual!=&Observe)return Fail(6344,"ability_damage_failed_shared_publication_owner_unknown");
            if(input.error)return Fail(input.error,input.stage);
            if(actual==&Observe)return Fail(6345,"ability_damage_shared_slot_active_without_install_receipt");
            ++state.waitingTargets;return true;
        }
        if(forward!=next.original||Read<void**>(next.pawn)!=b.copy||b.copy!=b.previous||actual!=&Observe)
            return Fail(6341,"ability_damage_shared_original_input_slot_readback_mismatch");
        b.original=forward;return true;
    }
    auto storage=static_cast<void**>(VirtualAlloc(nullptr,(PawnEntries+1)*8,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!storage)return Fail(6333,"ability_damage_private_vtable_allocation_failed");
    std::memcpy(storage,next.previous-1,(PawnEntries+1)*8);next.copy=storage+1;
    next.copy[TakeDamageSlot]=reinterpret_cast<void*>(&Observe);
    DWORD previous=0;if(!VirtualProtect(storage,(PawnEntries+1)*8,PAGE_READONLY,&previous)){
        VirtualFree(storage,0,MEM_RELEASE);return Fail(6334,"ability_damage_private_vtable_protection_failed");
    }
    b=next;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(b.pawn),b.copy,b.previous)!=b.previous){
        VirtualFree(storage,0,MEM_RELEASE);b={};return Fail(6335,"ability_damage_owned_vtable_publication_conflict");
    }
    b.counted=true;++state.installs;++state.boundTargets;return true;
}
inline bool PollImpl(double now){
    if((state.closePending||HasDeferredRetirements())&&CleanupThread()&&!WrapperCount()
       &&!state.processing&&!state.inImmediateCommit){
        if(!FlushDeferredRetirements())return false;
        if(!state.initialized)return true;
    }
    if(!Thread()||state.error||!std::isfinite(now)||now<0)return false;
    if(WrapperCount()||depth||state.inImmediateCommit||state.processing){
        state.stage="ability_damage_waiting_nonreentrant_native_boundary";Cache();return true;
    }
    if(state.closePending)return FlushDeferredRetirements();
    state.waitingTargets=0;
    for(size_t i=0;i<bindings.size();++i)if(!Install(i))return false;
    if(state.processing)return Fail(6336,"ability_damage_queue_processing_reentered");
    state.processing=true;
    while(state.readCursor<state.writeCursor){auto& entry=queue[state.readCursor%queue.size()];auto& c=entry.contact;
        if(!entry.used){state.processing=false;return Fail(6337,"ability_damage_native_queue_sequence_missing");}
        if(local_server_match::pending.active||(hooks.commitBoundaryReady&&!hooks.commitBoundaryReady())){
            ++state.pendingDeferrals;break;
        }
        if(!CurrentVictim(c)){++state.staleVictims;entry.used=false;++state.readCursor;continue;}
        if(!CurrentSource(c)){++state.staleSources;entry.used=false;++state.readCursor;continue;}
        const auto pending=Read<int32_t>(c.before.handler,0x248);
        if(pending>0){++state.pendingDeferrals;break;}
        // Older genuine gun/melee/dagger supplements may have committed since
        // this actual event returned. Advance only through their exact native
        // before/after receipt chain; an unrecorded original life write still
        // rejects this continuation rather than applying damage a second time.
        if(!local_server_combat::firearms::NativeLifeAllowsSupplement(c.victim,c.nativeBefore)){
            ++state.nativeLifeChanges;entry.used=false;++state.readCursor;continue;
        }
        Life current{};NativeLife nativeCurrent{};
        if(!local_server_combat::melee::ReadLife(c.victim,current)
           ||!local_server_combat::firearms::ReadNativeLife(c.victim,nativeCurrent)
           ||current.pendingEvents||nativeCurrent.pending){++state.staleVictims;entry.used=false;++state.readCursor;continue;}
        if(!EqualLife(c.before,current))++state.ownedLifeAdvances;
        c.before=current;c.nativeBefore=nativeCurrent;
        if(!local_server_combat::melee::SameLife(c.before)){++state.nativeLifeChanges;entry.used=false;++state.readCursor;continue;}
        original_damage::Result result{};
        if(!hooks.commit||!hooks.commit(c,now,result)){state.processing=false;return Fail(6338,"ability_damage_typed_host_commit_failed");}
        if(!RecordCommit(c,result,false)){state.processing=false;return false;}
        entry.used=false;++state.readCursor;
    }
    state.processing=false;if(state.closePending)return FinishClose();
    state.stage=state.waitingTargets?"ability_damage_waiting_owned_human_input_table":
        "ability_damage_original_base_radial_event_observer_ready";Cache();return true;
}
inline bool Poll(double now){
    bool okay=false;
    __try{okay=PollImpl(now);}__except(Capture(GetExceptionInformation())){state.processing=false;}
    // A failure is not permission to strand a previously requested Close.
    // Preserve the failure result while completing safe owned cleanup.
    if(state.closePending&&CleanupThread()&&!WrapperCount()&&!state.processing&&!state.inImmediateCommit)
        FlushDeferredRetirements();
    return okay;
}
inline bool Initialize(void* world,const Hooks& callbacks){
    if(state.initialized)return state.world==world&&!state.error;
    if(!world||!callbacks.resolveSource||!callbacks.allocateToken||!callbacks.commit||!original_players::Thread())return false;
    state.initialized=true;state.world=world;state.gameThread=GetCurrentThreadId();hooks=callbacks;
    state.immediateEnabled=callbacks.tryCommitBeforeCallerCleanup!=nullptr;
    if(!VerifyProfiles())return false;state.stage="ability_damage_waiting_actual_authority_victims";Cache();return true;
}
inline bool Close(){
    if(!state.initialized)return true;
    state.closePending=true;
    for(size_t i=0;i<bindings.size();++i)if(!Retire(i))return false;
    if(WrapperCount()||depth||state.processing||state.inImmediateCommit){
        state.stage="ability_damage_close_deferred_to_outermost_native_return";Cache();return false;
    }
    return FinishClose();
}
inline void Write(FILE* file){const auto s=Snapshot();auto yes=[](bool v){return v?"true":"false";};
    std::fprintf(file,"{\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"profiles_verified\":%s,\"restart_required\":%s,\"bound_targets\":%u,\"waiting_targets\":%u,\"installs\":%llu,\"retirements\":%llu,\"native_calls\":%llu,\"native_returns\":%llu,\"accepted_native_events\":%llu,\"queued\":%llu,\"applied\":%llu,\"kills\":%llu,\"not_applied\":%llu,\"unsupported_point_events\":%llu,\"unsupported_events\":%llu,\"unresolved_sources\":%llu,\"excluded_existing_sources\":%llu,\"invalid_events\":%llu,\"invalid_sources\":%llu,\"rejected_native\":%llu,\"native_life_changes\":%llu,\"stale_sources\":%llu,\"stale_victims\":%llu,\"pending_deferrals\":%llu,\"baseline_updates\":%llu,\"queue_overflow\":%llu,\"depth_overflow\":%llu,\"retained_pending_tables\":%llu,\"pending\":%llu,\"exception_rva\":%llu,\"point_supported\":false,\"runtime_damage_verified\":false",
        s.stage,s.error,yes(s.initialized),yes(s.profilesVerified),yes(s.restartRequired),s.boundTargets,s.waitingTargets,s.installs,s.retirements,
        s.nativeCalls,s.nativeReturns,s.accepted,s.queued,s.applied,s.kills,s.notApplied,s.unsupportedPointEvents,s.unsupportedEvents,
        s.unresolvedSources,s.excludedExistingSources,s.invalidEvents,s.invalidSources,s.rejectedNative,s.nativeLifeChanges,
        s.staleSources,s.staleVictims,s.pendingDeferrals,s.baselineUpdates,s.queueOverflow,s.depthOverflow,s.retainedPendingTables,
        s.writeCursor-s.readCursor,static_cast<unsigned long long>(s.exceptionRva));
    std::fprintf(file,",\"owned_native_life_advances\":%llu,\"immediate_enabled\":%s,\"immediate_attempts\":%llu,\"immediate_handled\":%llu,\"immediate_applied\":%llu,\"immediate_deferred\":%llu,\"immediate_unavailable\":%llu,\"immediate_order_deferrals\":%llu,\"deferred_retirements\":%llu,\"flushed_retirements\":%llu,\"active_wrappers\":%llu,\"close_pending\":%s}",
        s.ownedLifeAdvances,yes(s.immediateEnabled),s.immediateAttempts,s.immediateHandled,s.immediateApplied,s.immediateDeferred,
        s.immediateUnavailable,s.immediateOrderDeferrals,s.deferredRetirements,s.flushedRetirements,s.activeWrappers,yes(s.closePending));
}
} // namespace local_server_ability_damage
