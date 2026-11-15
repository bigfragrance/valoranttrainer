#pragma once
#include "original_damage_tuning.h"
#include "original_inventory_slots.h"
#include "deathmatch_rules.h"
#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <intrin.h>

// Local combat authority/transport for the offline V2 modes. Every direction
// and origin below comes from native FireShot/FiringEffect. This module does
// not fire a gun, calculate spread/recoil, write health or use a camera ray.
// The V2 caller must not initialize/poll the legacy original_shots singleton.
namespace training_combat_runtime {
inline constexpr size_t MaxPlayers=32,MaxEffects=128,MaxOrigins=64,MaxFiringStates=16,MaxEffectsPerState=64;
inline constexpr size_t MaxVtableFunctions=512,QueueCapacity=512,RetiredCapacity=1024;
inline constexpr size_t MaxEffectObservers=MaxPlayers*MaxEffects,EffectIndexCapacity=MaxEffectObservers*2;
static_assert(original_players::MaxPlayers==MaxPlayers&&original_damage::MaxTargets>=MaxPlayers);
using Weak=original_inventory_slots::WeakIdentity;
using NativeView=engine::Vec*(*)(void*,engine::Vec*);
using NativeEvent=void(*)(void*,void*,void*);
inline void ObserveEffectEvent(void* object,void* function,void* parameters);
struct Options {
    double maximumDistance=100000,maximumShotAge=.30,obstructionTolerance=.5,sampleInterval=.04;
    size_t maximumShotsPerPoll=32;
};
inline Options options;
struct Vtable {
    void** previous=nullptr;
    size_t functions=0;
    bool installed=false;
    std::array<void*,MaxVtableFunctions+1> copy{};
    void** Address(){return copy.data()+1;}
};
struct Effect {
    void* object=nullptr;Weak identity{};
    void* firing=nullptr;uint32_t firingSlot=0;
    Vtable table;
    NativeEvent forward=nullptr;
    uint64_t occurrence=0,initialCapturedOccurrence=0;
    uint64_t generation=0,ownerBinding=0;
    size_t ownerSource=MaxPlayers;
    bool present=false;
};
struct EffectIndex {void* object=nullptr;uint64_t generation=0;uint32_t slot=0;bool tombstone=false;};
static_assert(std::is_standard_layout_v<Vtable>&&std::is_standard_layout_v<Effect>);
static_assert((EffectIndexCapacity&(EffectIndexCapacity-1))==0);
struct Origin {
    uint64_t sequence=0,occurrence=0;
    int32_t nativeShot=-1;
    engine::Vec location{};
    engine::Vec inputAim{},controlRotation{};
    double at=-1000;
    Weak effectIdentity{};
    void* firing=nullptr;Weak firingIdentity{};uint32_t firingSlot=0;
    void* projectileClass=nullptr;
    bool valid=false,consumed=false,inputAimValid=false,weaponUseQueued=false;
};
struct RecentRay {uint64_t origin=0;uint32_t pellet=0;};
struct FiringContext {
    void *component=nullptr,*resource=nullptr,*stability=nullptr;
    void *resourceOverride=nullptr,*stabilityOverride=nullptr;
    Weak identity{},resourceIdentity{},stabilityIdentity{};
    bool tuningCached=false,resourceIsAmmo=false;
    original_damage_tuning::Diagnostics tuning;
    char name[128]{};
    uint64_t originsObserved=0,raysObserved=0,effectHooks=0,resourceQueries=0;
};
struct Source {
    bool bound=false,observing=false,alive=false,physicsReady=false,current=false,tuningCached=false,ballistic=true;
    bool haveView=false,aimValid=false,inputRestartAttempted=false,inputReady=false;
    size_t index=0;uint32_t spawnEpoch=0,held=0;
    uint64_t binding=0,originCursor=0,rayCursor=0,recentCursor=0;
    void *pawn=nullptr,*controller=nullptr,*weapon=nullptr,*inventory=nullptr,*firing=nullptr,*ammo=nullptr;
    void *input=nullptr,*inputState=nullptr,*restartPawn=nullptr;
    Weak restartPawnId{};
    Weak pawnId{},controllerId{},weaponId{},inventoryId{},firingId{},ammoId{},inputId{},inputStateId{};
    Vtable pawnTable;
    NativeView forwardView=nullptr;
    std::array<Effect*,MaxEffects> effects{};
    std::array<Origin,MaxOrigins> origins{};
    std::array<FiringContext,MaxFiringStates> firings{};
    size_t firingCount=0;int32_t lastFiringSlot=-1;
    std::array<RecentRay,256> recent{};
    original_damage_tuning::Diagnostics tuning;
    engine::Vec nativeView{},lastView{},observedAim{},controlRotation{};
    double sampledAt=-1000;
    int32_t bodyCount=0,physicsActors=0,ammoCount=-1;
    uint64_t originsObserved=0,raysObserved=0,effectHooks=0,inputEdges=0,inputSubmissions=0,viewSubmissions=0;
    uint64_t viewObservedMatches=0,damageEvents=0,kills=0,pcInputInitializations=0,pawnClientRestarts=0;
};
struct ShotToken {
    uint32_t source=0,spawnEpoch=0;
    uint64_t binding=0,nativeOrigin=0;
    uint32_t projectile=0,firingSlot=0;
};
struct Pending {
    original_shots::Shot shot;
    ShotToken identity;
    uint64_t damageToken=0;
    void* firing=nullptr;Weak firingIdentity{};
    void* projectileClass=nullptr;
    uint32_t eligible=0;
    std::array<uint32_t,MaxPlayers> victimEpochs{};
};
struct ObservedRay {
    original_shots::Shot shot;
    ShotToken identity;
    uint64_t damageToken=0;
    engine::Vec originalInputAim{},originalControlRotation{};
    bool inputAimValid=false;
    void *firing=nullptr,*resource=nullptr,*stability=nullptr,*projectileClass=nullptr;
};
struct Retired {void* actor=nullptr;Weak identity{};};
enum class RetireReason:uint8_t{WeaponChange,PawnRetire,NativeDeath,Shutdown};
struct Diagnostics {
    bool initialized=false,ready=false,restartRequired=false,processing=false,nativeEntriesVerified=false;
    unsigned error=0,boundSources=0,aliveSources=0,physicsReadySources=0;
    uint64_t binds=0,retirements=0,restoredPawnTables=0,restoredEffectTables=0;
    uint64_t originQueries=0,exactOrigins=0,effectCallbacks=0,directions=0,nativeRays=0;
    uint64_t nativeContextWalks=0,nativeContextMatches=0,nativeContextRejected=0,unknownFiringOrigins=0;
    uint64_t changedProjectileRejected=0;
    uint64_t effectObserverRecords=0,effectObserverReclaims=0,effectLeaseTransfers=0,effectObserverReinstalls=0;
    uint64_t effectOrphanRecoveries=0,effectLeaseCallbackRebinds=0,effectRegistryIdentityRejects=0;
    uint64_t effectObservedDispatches=0,effectOriginalForwardAttempts=0,effectOriginalForwardCompleted=0,effectStartCallbacks=0;
    uint64_t weaponUsedQueued=0,weaponUsedNotified=0,weaponUsedExpired=0,weaponUsedOverflow=0;
    void *lastEffectObject=nullptr,*lastEffectCurrentTable=nullptr,*lastEffectPreviousTable=nullptr;
    int32_t lastEffectOldSource=-1,lastEffectNewSource=-1;
    uint64_t lastEffectOldBinding=0,lastEffectNewBinding=0;
    uint32_t nativeContextExceptionCode=0;
    uint64_t invalidPayloads=0,unpairedOrigins=0,ambiguousOrigins=0,duplicateRays=0,expiredOrigins=0;
    uint64_t queued=0,processed=0,queueOverflow=0,staleShots=0,expiredBindings=0,retiredVictimEpochs=0;
    uint64_t physicsDeferred=0,inputDeferred=0,pendingDamageDeferred=0,bindingDeferred=0;
    uint64_t bodyTraces=0,sceneTraces=0,sceneBlocked=0,misses=0,policyRejected=0;
    uint64_t damageEvents=0,kills=0,viewSubmissions=0,inputEdges=0,inputSubmissions=0;
    uint64_t visibilityQueries=0,visibilityDeferred=0,foreignTableConflicts=0;
    uint64_t lastToken=0,lastNativeOrigin=0;
    int32_t lastSource=-1,lastVictim=-1;
    float lastDamage=0;
    double lastPoll=-1,lastBodyDistance=0,lastSceneDistance=0;
    uintptr_t exceptionRva=0;
    const char* stage="training_combat_not_initialized";
    const char* operation="none";
};
inline Diagnostics diagnostics;
inline std::array<Source,MaxPlayers> sources{};
inline std::array<Effect,MaxEffectObservers> effectObservers{};
inline std::array<EffectIndex,EffectIndexCapacity> effectIndex{};
inline uint64_t nextEffectGeneration=0;
inline size_t nextEffectRecord=0;
inline std::array<Pending,QueueCapacity> queue{};
inline std::array<ObservedRay,256> observations{};
inline std::array<Retired,RetiredCapacity> retired{};
inline uint64_t readIndex=0,writeIndex=0,nextBinding=0,nextOrigin=0,nextDamageToken=0,nextOccurrence=0;
inline uint64_t observationRead=0,observationWrite=0,observationDrops=0;
struct WeaponUse {size_t source=0;uint32_t spawnEpoch=0;uint64_t binding=0,origin=0;void* pawn=nullptr;Weak pawnIdentity{};};
using WeaponUsedCallback=void(*)(size_t);
inline WeaponUsedCallback weaponUsedCallback=nullptr;
inline std::array<WeaponUse,QueueCapacity> weaponUses{};
inline uint64_t weaponUseRead=0,weaponUseWrite=0;
inline void SetWeaponUsedCallback(WeaponUsedCallback callback){weaponUsedCallback=callback;}
inline DWORD gameThread=0;
inline void *world=nullptr,*shotFunction=nullptr,*startFunction=nullptr,*forceInputFunction=nullptr;
inline void *componentsFunction=nullptr,*resourceFunction=nullptr,*stabilityFunction=nullptr;
using CaptureContextFn=void(WINAPI*)(PCONTEXT);
using LookupFunctionFn=PRUNTIME_FUNCTION(WINAPI*)(DWORD64,PDWORD64,PUNWIND_HISTORY_TABLE);
using VirtualUnwindFn=PEXCEPTION_ROUTINE(WINAPI*)(DWORD,DWORD64,DWORD64,PRUNTIME_FUNCTION,PCONTEXT,PVOID*,PDWORD64,PKNONVOLATILE_CONTEXT_POINTERS);
inline CaptureContextFn captureContext=nullptr;
inline LookupFunctionFn lookupFunction=nullptr;
inline VirtualUnwindFn virtualUnwind=nullptr;
inline const Diagnostics& State(){return diagnostics;}
inline const Source* Player(size_t index){return index<sources.size()?&sources[index]:nullptr;}
inline bool Ready(size_t index){const auto p=Player(index);return p&&p->bound&&p->alive&&p->physicsReady&&p->inputReady&&p->observing;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Fail(unsigned code,const char* stage,bool terminal=false){
    diagnostics.error=code;diagnostics.stage=stage;diagnostics.ready=false;
    if(terminal)diagnostics.restartRequired=true;return false;
}
inline bool Thread(){return diagnostics.initialized&&!diagnostics.restartRequired&&gameThread==GetCurrentThreadId();}
inline double Now(){return static_cast<double>(GetTickCount64())/1000.;}
inline bool Same(const engine::Vec& a,const engine::Vec& b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
inline bool Close(const engine::Vec& a,const engine::Vec& b){return std::abs(a.x-b.x)<1.e-10&&std::abs(a.y-b.y)<1.e-10&&std::abs(a.z-b.z)<1.e-10;}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Identity(void* object,Weak& identity){return original_inventory_slots::CaptureIdentity(object,identity)&&Live(object,identity);}
inline LONG CaptureException(EXCEPTION_POINTERS* p){
    const auto code=p&&p->ExceptionRecord?p->ExceptionRecord->ExceptionCode:0xe0002400U;
    const auto address=p&&p->ExceptionRecord?reinterpret_cast<uintptr_t>(p->ExceptionRecord->ExceptionAddress):0;
    diagnostics.exceptionRva=address>=engine::image?address-engine::image:address;
    deathmatch_rules::CancelNativeDamage();
    Fail(code,"training_combat_native_exception_requires_new_session",true);return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Event(void* object,const wchar_t* name,uint16_t bytes,void* parameters){
    return original_damage::Event(object,name,bytes,parameters);
}

// Stop only at a real non-code delimiter; never truncate a table at the slot
// being intercepted. RTTI and every preceding/following virtual are copied.
inline bool CopyVtable(void* object,Vtable& result,size_t minimumFunctions){
    auto previous=Read<void**>(object,0);if(!previous||!minimumFunctions||minimumFunctions>MaxVtableFunctions)return false;
    MEMORY_BASIC_INFORMATION codeRegion{};
    uintptr_t codeBegin=0,codeEnd=0;
    size_t count=0;
    for(;count<MaxVtableFunctions;++count){
        void* entry=nullptr;if(!original_players::Read(previous,count*8,entry))return false;
        const auto address=reinterpret_cast<uintptr_t>(entry);
        if(!address)break;
        if(address<codeBegin||address>=codeEnd){
            if(!VirtualQuery(entry,&codeRegion,sizeof(codeRegion))||codeRegion.State!=MEM_COMMIT
               ||(codeRegion.Protect&(PAGE_GUARD|PAGE_NOACCESS)))break;
            const auto protection=codeRegion.Protect&0xffU;
            if(protection!=PAGE_EXECUTE&&protection!=PAGE_EXECUTE_READ&&protection!=PAGE_EXECUTE_READWRITE
               &&protection!=PAGE_EXECUTE_WRITECOPY)break;
            codeBegin=reinterpret_cast<uintptr_t>(codeRegion.BaseAddress);codeEnd=codeBegin+codeRegion.RegionSize;
        }
    }
    if(count<minimumFunctions||count==MaxVtableFunctions
       ||!original_players::Readable(previous-1,(count+1)*8))return false;
    result.previous=previous;result.functions=count;
    std::memcpy(result.copy.data(),previous-1,(count+1)*8);return true;
}
inline bool Install(void* object,Vtable& table,size_t slot,void* replacement){
    if(!table.previous||slot>=table.functions||table.installed)return false;
    table.copy[1+slot]=replacement;
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(object),table.Address(),table.previous)!=table.previous)return false;
    table.installed=true;return true;
}
inline bool Restore(void* object,const Weak& identity,Vtable& table){
    if(!table.installed)return true;
    if(!Live(object,identity)){table.installed=false;return true;}
    auto current=Read<void**>(object,0);
    if(current==table.previous){table.installed=false;return true;}
    if(current!=table.Address()){++diagnostics.foreignTableConflicts;return Fail(2401,"training_combat_foreign_observer_must_retire_first");}
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(object),table.previous,table.Address())!=table.Address())
        return Fail(2402,"training_combat_instance_vptr_restore_raced");
    table.installed=false;return true;
}
inline bool OwnsEffect(const Source& s,const Effect& effect){
    return s.bound&&effect.ownerSource==s.index&&effect.ownerBinding==s.binding;
}
inline size_t EffectHash(void* object){
    const auto value=reinterpret_cast<uintptr_t>(object)>>4;
    return static_cast<size_t>((value^(value>>19))&(EffectIndexCapacity-1));
}
inline Effect* IndexedEffect(void* object){
    size_t at=EffectHash(object);
    for(size_t probe=0;probe<effectIndex.size();++probe,at=(at+1)&(EffectIndexCapacity-1)){
        const auto& key=effectIndex[at];if(!key.object&&!key.tombstone)return nullptr;
        if(key.object==object&&key.slot<effectObservers.size()){
            auto& effect=effectObservers[key.slot];
            return effect.object==object&&effect.generation==key.generation?&effect:nullptr;}
    }
    return nullptr;
}
inline void RemoveEffectIndex(Effect& effect){
    size_t at=EffectHash(effect.object);
    for(size_t probe=0;probe<effectIndex.size();++probe,at=(at+1)&(EffectIndexCapacity-1)){
        auto& key=effectIndex[at];if(!key.object&&!key.tombstone)return;
        if(key.object==effect.object){if(key.generation==effect.generation)key={nullptr,0,0,true};return;}
    }
}
inline bool PublishEffectIndex(Effect& effect){
    size_t at=EffectHash(effect.object),vacant=effectIndex.size();
    for(size_t probe=0;probe<effectIndex.size();++probe,at=(at+1)&(EffectIndexCapacity-1)){
        const auto& key=effectIndex[at];
        if(key.object==effect.object){vacant=at;break;}
        if(!key.object){if(vacant==effectIndex.size())vacant=at;if(!key.tombstone)break;}
    }
    if(vacant==effectIndex.size())return Fail(2462,"training_combat_effect_identity_index_full");
    effectIndex[vacant]={effect.object,effect.generation,static_cast<uint32_t>(&effect-effectObservers.data()),false};return true;
}
inline Effect* EffectByTable(void** table){
    // The backing address itself identifies our immutable instance table. This
    // does not inspect arbitrary table contents or mistake a native table for
    // ours when logical ownership changes before the next Source Poll.
    const auto first=reinterpret_cast<uintptr_t>(effectObservers.data())+offsetof(Effect,table)+offsetof(Vtable,copy)+sizeof(void*);
    const auto address=reinterpret_cast<uintptr_t>(table);
    if(address<first||(address-first)%sizeof(Effect))return nullptr;
    const auto slot=(address-first)/sizeof(Effect);
    if(slot>=effectObservers.size())return nullptr;
    auto& effect=effectObservers[slot];return effect.table.Address()==table?&effect:nullptr;
}
inline void UnleaseEffect(Effect& effect){
    effect.ownerSource=MaxPlayers;effect.ownerBinding=0;effect.firing=nullptr;effect.firingSlot=0;
    effect.present=false;effect.occurrence=0;effect.initialCapturedOccurrence=0;
    // Keep object+weak identity, previous vptr and original forwarding target
    // for this live pooled UObject. They are reclaimed only after its serial
    // expires, never merely because one gun's active-effects array drops it.
}
inline bool ReleaseEffect(Source& s,Effect*& reference){
    auto effect=reference;if(!effect)return true;
    if(OwnsEffect(s,*effect)){const bool installed=effect->table.installed;
        if(!Restore(effect->object,effect->identity,effect->table))return false;
        if(installed)++diagnostics.restoredEffectTables;UnleaseEffect(*effect);}
    reference=nullptr;return true;
}
inline bool LeaseEffect(Source& s,Effect& effect,uint32_t firingSlot,bool fromCallback=false){
    if(firingSlot>=s.firingCount||Read<void*>(effect.object,0x148)!=s.firings[firingSlot].component)return false;
    Effect** destination=nullptr;
    for(auto& reference:s.effects){if(reference==&effect){destination=&reference;break;}
        if(!destination&&(!reference||!OwnsEffect(s,*reference)))destination=&reference;}
    if(!destination)return Fail(2406,"training_combat_effect_observer_capacity_exhausted");
    const bool changed=effect.ownerSource!=s.index||effect.ownerBinding!=s.binding||effect.firingSlot!=firingSlot
        ||effect.firing!=s.firings[firingSlot].component;
    if(changed){
        diagnostics.lastEffectObject=effect.object;diagnostics.lastEffectCurrentTable=Read<void*>(effect.object,0);
        diagnostics.lastEffectPreviousTable=effect.table.previous;
        diagnostics.lastEffectOldSource=effect.ownerSource<MaxPlayers?static_cast<int32_t>(effect.ownerSource):-1;
        diagnostics.lastEffectNewSource=static_cast<int32_t>(s.index);
        diagnostics.lastEffectOldBinding=effect.ownerBinding;diagnostics.lastEffectNewBinding=s.binding;
        if(effect.ownerSource<MaxPlayers&&effect.ownerBinding){
            auto& previous=sources[effect.ownerSource];
            for(auto& reference:previous.effects)if(reference==&effect)reference=nullptr;
            ++diagnostics.effectLeaseTransfers;
        }
        effect.ownerSource=s.index;effect.ownerBinding=s.binding;
        effect.firing=s.firings[firingSlot].component;effect.firingSlot=firingSlot;effect.occurrence=++nextOccurrence;
        if(fromCallback)++diagnostics.effectLeaseCallbackRebinds;
    }
    *destination=&effect;effect.present=true;return true;
}
inline Effect* AllocateEffectRecord(){
    for(size_t scanned=0;scanned<effectObservers.size();++scanned){
        auto& effect=effectObservers[nextEffectRecord++%effectObservers.size()];
        if(effect.object&&Live(effect.object,effect.identity))continue;
        if(effect.object){RemoveEffectIndex(effect);++diagnostics.effectObserverReclaims;}
        // No live UObject can legitimately depend on a serial-expired record.
        // Clear metadata only after that native resolver has confirmed expiry.
        effect={};effect.generation=++nextEffectGeneration;return &effect;
    }
    Fail(2463,"training_combat_live_effect_identity_registry_full");return nullptr;
}
inline bool RecoverAliasedEffectTable(void* object,const Weak& identity,Effect& provenance){
    // Defensive handling for a pool restoring an old cloned vptr onto another
    // identity. The native current class CDO must independently prove the exact
    // saved prior table; an expired/other identity never authorizes a write.
    auto type=Read<void*>(object,0x10);auto cdo=Read<void*>(type,0x120);auto native=Read<void**>(cdo,0);
    if(!Live(object,identity)||!original_players::IsA(object,0x684d570)||!native
       ||native!=provenance.table.previous||provenance.forward!=engine::function<NativeEvent>(0x1cdc850)
       ||Read<void*>(native,0x298)!=reinterpret_cast<void*>(engine::image+0x1cdc850)){
        ++diagnostics.effectRegistryIdentityRejects;return Fail(2464,"training_combat_owned_effect_clone_identity_not_proven");}
    if(!provenance.table.functions||provenance.table.copy[0]!=Read<void*>(native-1,0))return Fail(2464,"training_combat_owned_effect_clone_rtti_not_proven");
    for(size_t slot=0;slot<provenance.table.functions;++slot){
        const auto expected=slot==0x298/8?reinterpret_cast<void*>(ObserveEffectEvent):Read<void*>(native,slot*8);
        if(provenance.table.copy[1+slot]!=expected)return Fail(2464,"training_combat_owned_effect_clone_contains_foreign_layer");
    }
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(object),native,provenance.table.Address())!=provenance.table.Address())
        return Fail(2465,"training_combat_owned_effect_clone_recovery_raced");
    ++diagnostics.effectOrphanRecoveries;return true;
}
inline bool RecoverEquivalentForeignTable(void* object,const Weak& identity,void** current){
    auto type=Read<void*>(object,0x10);auto cdo=Read<void*>(type,0x120);
    Vtable native;
    if(!Live(object,identity)||!cdo||!CopyVtable(cdo,native,0x298/8+1)
       ||native.previous[0x298/8]!=reinterpret_cast<void*>(engine::image+0x1cdc850)
       ||!original_players::Readable(current-1,(native.functions+1)*8))return false;
    if(Read<void*>(current-1,0)!=native.copy[0])return false;
    for(size_t slot=0;slot<native.functions;++slot){
        const auto expected=slot==0x298/8?reinterpret_cast<void*>(ObserveEffectEvent):native.previous[slot];
        if(Read<void*>(current,slot*8)!=expected)return false;
    }
    auto tail=Read<void*>(current,native.functions*8);
    if(tail){MEMORY_BASIC_INFORMATION region{};
        if(!VirtualQuery(tail,&region,sizeof(region)))return false;
        const auto protection=region.Protect&0xffU;
        if(region.State==MEM_COMMIT&&!(region.Protect&(PAGE_GUARD|PAGE_NOACCESS))
           &&(protection==PAGE_EXECUTE||protection==PAGE_EXECUTE_READ||protection==PAGE_EXECUTE_READWRITE
              ||protection==PAGE_EXECUTE_WRITECOPY))return false;
    }
    if(!Live(object,identity)||InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(object),native.previous,current)!=current)return false;
    ++diagnostics.effectOrphanRecoveries;return true;
}
inline Effect* AcquireEffect(Source& s,void* object,uint32_t firingSlot){
    Weak identity{};if(!Identity(object,identity))return nullptr;
    auto current=Read<void**>(object,0);auto owned=EffectByTable(current);
    auto previousRecord=owned?owned:IndexedEffect(object);
    diagnostics.lastEffectObject=object;diagnostics.lastEffectCurrentTable=current;
    diagnostics.lastEffectPreviousTable=previousRecord?previousRecord->table.previous:nullptr;
    diagnostics.lastEffectOldSource=previousRecord&&previousRecord->ownerSource<MaxPlayers?static_cast<int32_t>(previousRecord->ownerSource):-1;
    diagnostics.lastEffectNewSource=static_cast<int32_t>(s.index);
    diagnostics.lastEffectOldBinding=previousRecord?previousRecord->ownerBinding:0;diagnostics.lastEffectNewBinding=s.binding;
    if(owned&&(owned->object!=object||!original_inventory_slots::SameIdentity(owned->identity,identity))){
        if(!RecoverAliasedEffectTable(object,identity,*owned))return nullptr;
        current=Read<void**>(object,0);owned=nullptr;
    }
    if(!owned&&Read<void*>(current,0x298)==reinterpret_cast<void*>(ObserveEffectEvent)){
        // An external copy of our table is accepted only if its RTTI, complete
        // native prefix and boundary independently match this class's CDO,
        // with precisely our single PE override. Preserve all other layers.
        if(!RecoverEquivalentForeignTable(object,identity,current)){
            ++diagnostics.effectRegistryIdentityRejects;Fail(2466,"training_combat_effect_observer_provenance_missing");return nullptr;}
        current=Read<void**>(object,0);
    }
    auto effect=owned?owned:IndexedEffect(object);
    if(effect&&(!original_inventory_slots::SameIdentity(effect->identity,identity)||effect->object!=object))effect=nullptr;
    bool installedNow=false,newRecord=false;
    if(!effect){
        // Never call CopyVtable on one of our hook tables. It would make the
        // observer its own forwarding target and lose the native prior layer.
        if(EffectByTable(current)||Read<void*>(current,0x298)==reinterpret_cast<void*>(ObserveEffectEvent)){
            Fail(2466,"training_combat_effect_observer_provenance_missing");return nullptr;}
        Vtable previous;
        if(!CopyVtable(object,previous,0x298/8+1)){Fail(2407,"training_combat_effect_complete_vtable_unavailable");return nullptr;}
        auto forward=reinterpret_cast<NativeEvent>(previous.previous[0x298/8]);
        if(forward!=engine::function<NativeEvent>(0x1cdc850)){Fail(2408,"training_combat_effect_process_event_already_observed");return nullptr;}
        effect=AllocateEffectRecord();if(!effect)return nullptr;
        newRecord=true;
        effect->object=object;effect->identity=identity;effect->table=previous;effect->forward=forward;
        if(!PublishEffectIndex(*effect))return nullptr;++diagnostics.effectObserverRecords;
    }
    if(effect->forward!=engine::function<NativeEvent>(0x1cdc850)||!effect->table.previous){
        Fail(2467,"training_combat_effect_original_forward_target_invalid");return nullptr;}
    if(current==effect->table.Address()){
        if(!effect->table.installed)++diagnostics.effectOrphanRecoveries;
        effect->table.installed=true;
    }else if(current==effect->table.previous){
        effect->table.installed=false;
        if(!Install(object,effect->table,0x298/8,reinterpret_cast<void*>(ObserveEffectEvent))){Fail(2409,"training_combat_effect_instance_install_failed");return nullptr;}
        installedNow=true;if(!newRecord)++diagnostics.effectObserverReinstalls;
    }else {Fail(2468,"training_combat_effect_foreign_instance_vptr_conflict");return nullptr;}
    if(!LeaseEffect(s,*effect,firingSlot))return nullptr;
    if(installedNow){++s.effectHooks;++s.firings[firingSlot].effectHooks;}
    return effect;
}
inline Source* PawnSource(void* pawn){for(auto& s:sources)if(s.bound&&s.pawn==pawn)return &s;return nullptr;}
inline FiringContext* FiringRecord(Source& s,void* firing){
    if(firing)for(size_t i=0;i<s.firingCount;++i)if(s.firings[i].component==firing)return &s.firings[i];return nullptr;
}
inline Source* FiringSource(void* firing){if(firing)for(auto& s:sources)if(s.bound&&FiringRecord(s,firing))return &s;return nullptr;}
inline Effect* EffectRecord(Source& s,void* object){for(auto effect:s.effects)if(effect&&OwnsEffect(s,*effect)&&effect->object==object)return effect;return nullptr;}
inline Source* SyncEffectLease(Effect& effect){
    if(!Live(effect.object,effect.identity))return nullptr;
    auto source=FiringSource(Read<void*>(effect.object,0x148));
    if(!source||!source->observing||!Live(source->pawn,source->pawnId))return nullptr;
    auto firing=FiringRecord(*source,Read<void*>(effect.object,0x148));
    if(!firing||!Live(firing->component,firing->identity)||Read<void*>(firing->component,0xc8)!=source->weapon
       ||Read<void*>(firing->component,0x4c8)!=source->pawn)return nullptr;
    const auto slot=static_cast<uint32_t>(firing-source->firings.data());
    return LeaseEffect(*source,effect,slot,true)?source:nullptr;
}
inline void CapturePayload(Source& source,Effect& effect,int32_t nativeShot,uint32_t flags,const uint8_t* array);
inline bool RefreshFiringResources(FiringContext& firing,bool force=false);
inline bool BeforeRetire(size_t index,RetireReason reason=RetireReason::WeaponChange);

// Recover this thread's real native FireShot `this`. The two shipped functions
// both retain it in nonvolatile RSI. Windows unwind restores that register at
// their exact return PCs; no stack-word search, guessed active-state getter or
// first-component fallback can authorize a shot origin.
__declspec(noinline) inline FiringContext* NativeFiringContextImpl(Source& s){
    ++diagnostics.nativeContextWalks;
    if(!captureContext||!lookupFunction||!virtualUnwind)return nullptr;
    alignas(16) CONTEXT context{};captureContext(&context);
    const auto tib=reinterpret_cast<const NT_TIB*>(NtCurrentTeb());
    const auto low=reinterpret_cast<uintptr_t>(tib->StackLimit),high=reinterpret_cast<uintptr_t>(tib->StackBase);
    void *helper=nullptr,*fire=nullptr;bool invalid=false;
    for(unsigned depth=0;depth<24&&context.Rip;++depth){
        if(context.Rsp<low||context.Rsp>high||high-context.Rsp<8){invalid=true;break;}
        if(context.Rip==engine::image+0x71227dd)helper=reinterpret_cast<void*>(context.Rsi);
        if(context.Rip==engine::image+0x711f0ae){fire=reinterpret_cast<void*>(context.Rsi);break;}
        const auto previousPc=context.Rip,previousSp=context.Rsp;
        DWORD64 imageBase=0;auto function=lookupFunction(context.Rip,&imageBase,nullptr);
        if(function){PVOID handlerData=nullptr;DWORD64 frame=0;
            virtualUnwind(UNW_FLAG_NHANDLER,imageBase,context.Rip,function,&context,&handlerData,&frame,nullptr);
        }else {
            DWORD64 returnPc=0;if(!original_players::Read(reinterpret_cast<void*>(context.Rsp),0,returnPc)){invalid=true;break;}
            context.Rip=returnPc;context.Rsp+=8;
        }
        if(context.Rsp<previousSp||(context.Rip==previousPc&&context.Rsp==previousSp)){invalid=true;break;}
    }
    if(invalid||!helper||helper!=fire){++diagnostics.nativeContextRejected;return nullptr;}
    auto firing=FiringRecord(s,fire);
    if(!firing){++diagnostics.unknownFiringOrigins;return nullptr;}
    if(!Live(firing->component,firing->identity)||!original_players::IsA(firing->component,0x684d4c0)
       ||Read<void*>(firing->component,0xc8)!=s.weapon||Read<void*>(firing->component,0x4c8)!=s.pawn){
        ++diagnostics.nativeContextRejected;return nullptr;}
    ++diagnostics.nativeContextMatches;return firing;
}
inline LONG ContextException(EXCEPTION_POINTERS* p){
    ++diagnostics.nativeContextRejected;
    diagnostics.nativeContextExceptionCode=p&&p->ExceptionRecord?p->ExceptionRecord->ExceptionCode:0xe0002459U;
    return EXCEPTION_EXECUTE_HANDLER;
}
__declspec(noinline) inline FiringContext* NativeFiringContext(Source& s){
    __try{return NativeFiringContextImpl(s);}__except(ContextException(GetExceptionInformation())){return nullptr;}
}

__declspec(noinline) inline engine::Vec* ObserveViewLocation(void* pawn,engine::Vec* output){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    if(GetCurrentThreadId()!=gameThread)return engine::function<NativeView>(0x787f820)(pawn,output);
    auto s=PawnSource(pawn);
    auto forward=s&&s->forwardView?s->forwardView:engine::function<NativeView>(0x787f820);
    auto result=forward(pawn,output);
    if(!Thread()||!s||!s->observing)return result;
    ++diagnostics.originQueries;
    if(caller!=engine::image+0x78840ca)return result;
    auto firing=NativeFiringContext(*s);if(!firing)return result;
    engine::Vec origin{};
    if(!original_players::Read(result,0,origin)||!original_shots::WorldPoint(origin)){
        ++diagnostics.invalidPayloads;return result;}
    auto& record=s->origins[s->originCursor++%s->origins.size()];record={};
    record.firing=firing->component;record.firingIdentity=firing->identity;
    record.projectileClass=Read<void*>(firing->component,0x550);
    record.firingSlot=static_cast<uint32_t>(firing-s->firings.data());s->lastFiringSlot=static_cast<int32_t>(record.firingSlot);
    record.sequence=++nextOrigin;record.nativeShot=Read<int32_t>(firing->component,0x4a8);
    record.location=origin;record.at=Now();record.valid=true;
    record.inputAim=Read<engine::Vec>(s->inputState,0x238);record.inputAimValid=Read<uint8_t>(s->inputState,0x128)!=0;
    record.controlRotation=Read<engine::Vec>(s->controller,0x4e8);
    ++firing->originsObserved;++s->originsObserved;++diagnostics.exactOrigins;return result;
}
inline void CaptureInitial(Source& s,Effect& e){
    if(!s.observing||Read<void*>(e.object,0x148)!=e.firing||!FiringRecord(s,e.firing))return;
    if(e.initialCapturedOccurrence==e.occurrence)return;e.initialCapturedOccurrence=e.occurrence;
    const uint32_t flags=4U|(Read<uint8_t>(e.object,0x168)?1U:0U)|(Read<int32_t>(e.object,0x164)<=0?2U:0U);
    CapturePayload(s,e,Read<int32_t>(e.object,0x160),flags,static_cast<uint8_t*>(e.object)+0x150);
}
inline void ObserveEffectEvent(void* object,void* function,void* parameters){
    if(GetCurrentThreadId()!=gameThread){engine::function<NativeEvent>(0x1cdc850)(object,function,parameters);return;}
    ++diagnostics.effectObservedDispatches;if(function==startFunction)++diagnostics.effectStartCallbacks;
    Effect* effect=nullptr;uint64_t generation=0;
    auto forward=engine::function<NativeEvent>(0x1cdc850);
    __try{
        effect=EffectByTable(Read<void**>(object,0));
        if(effect&&effect->forward==engine::function<NativeEvent>(0x1cdc850))forward=effect->forward;
        const bool exact=effect&&effect->object==object&&Live(object,effect->identity);
        generation=exact?effect->generation:0;
        if(!exact)++diagnostics.effectRegistryIdentityRejects;
        auto s=Thread()&&exact?SyncEffectLease(*effect):nullptr;
        if(Thread()&&s&&effect&&s->observing&&function==shotFunction){
            ++diagnostics.effectCallbacks;
            if(Read<void*>(parameters,0)==effect->firing){
                const uint32_t flags=(Read<uint8_t>(parameters,12)?1U:0U)|(Read<uint8_t>(parameters,13)?2U:0U);
                CapturePayload(*s,*effect,Read<int32_t>(parameters,8),flags,static_cast<uint8_t*>(parameters)+16);
            }
        }
    }__except(CaptureException(GetExceptionInformation())){}
    // Exactly one original dispatch, including unknown/retired logical leases.
    // A global immutable forwarding target never points back to this observer.
    ++diagnostics.effectOriginalForwardAttempts;forward(object,function,parameters);++diagnostics.effectOriginalForwardCompleted;
    __try{
        if(Thread()&&effect&&generation&&effect->generation==generation&&effect->object==object&&function==startFunction){
            auto current=SyncEffectLease(*effect);
            if(current){effect->occurrence=++nextOccurrence;CaptureInitial(*current,*effect);}
        }
    }__except(CaptureException(GetExceptionInformation())){}
}
inline void QueueWeaponUse(Source& s,Origin& origin){
    if(!weaponUsedCallback||origin.weaponUseQueued)return;origin.weaponUseQueued=true;
    if(weaponUseWrite-weaponUseRead>=weaponUses.size()){++diagnostics.weaponUsedOverflow;return;}
    weaponUses[weaponUseWrite++%weaponUses.size()]={s.index,s.spawnEpoch,s.binding,origin.sequence,s.pawn,s.pawnId};
    ++diagnostics.weaponUsedQueued;
}
inline bool NotifyWeaponUses(){
    if(!Thread())return Fail(2469,"training_combat_weapon_use_callback_requires_owned_game_thread");
    const auto end=weaponUseWrite;
    while(weaponUseRead<end){const auto event=weaponUses[weaponUseRead++%weaponUses.size()];
        if(!weaponUsedCallback)continue;
        const auto player=deathmatch_rules::Player(event.source);
        // A same-life gun switch still counts as real weapon use. An old-life
        // event can never strip protection from a replacement respawned Pawn.
        if(!player||!player->registered||!player->alive||player->spawnEpoch!=event.spawnEpoch
           ||player->pawn!=event.pawn||!Live(event.pawn,event.pawnIdentity)){++diagnostics.weaponUsedExpired;continue;}
        diagnostics.operation="observed_native_weapon_use_callback";
        weaponUsedCallback(event.source);++diagnostics.weaponUsedNotified;diagnostics.operation="none";
    }
    return true;
}
inline bool QueueRay(Source& s,Origin& origin,uint32_t pellet,int32_t nativeShot,uint32_t flags,const engine::Vec& direction){
    if(origin.firingSlot>=s.firingCount)return false;auto& firing=s.firings[origin.firingSlot];
    if(firing.component!=origin.firing||!original_inventory_slots::SameIdentity(firing.identity,origin.firingIdentity))return false;
    QueueWeaponUse(s,origin);
    for(const auto& ray:s.recent)if(ray.origin==origin.sequence&&ray.pellet==pellet){++diagnostics.duplicateRays;return true;}
    s.recent[s.recentCursor++%s.recent.size()]={origin.sequence,pellet};
    if(writeIndex-readIndex>=queue.size()){++diagnostics.queueOverflow;return false;}
    if(nextDamageToken==(std::numeric_limits<uint64_t>::max)())return Fail(2404,"training_combat_damage_token_exhausted",true);
    auto& pending=queue[writeIndex%queue.size()];pending={};
    pending.identity={static_cast<uint32_t>(s.index),s.spawnEpoch,s.binding,origin.sequence,pellet,origin.firingSlot};
    pending.damageToken=++nextDamageToken;
    pending.firing=firing.component;pending.firingIdentity=firing.identity;
    pending.projectileClass=origin.projectileClass;
    pending.shot={origin.at,origin.location,direction,nativeShot,flags|8U,
        firing.resourceIsAmmo&&Live(firing.resource,firing.resourceIdentity)?Read<int32_t>(firing.resource,0x10c):-1,pellet,++s.rayCursor};
    if(observationWrite-observationRead>=observations.size()){++observationRead;++observationDrops;}
    observations[observationWrite++%observations.size()]={pending.shot,pending.identity,pending.damageToken,origin.inputAim,origin.controlRotation,
        origin.inputAimValid,firing.component,firing.resource,firing.stability,origin.projectileClass};
    // Snapshot eligibility and every target's life. A delayed shot cannot hit a
    // new Pawn that happens to occupy the same roster slot after respawn.
    for(size_t i=0;i<sources.size();++i){const auto player=deathmatch_rules::Player(i);
        if(i==s.index||!player||!player->registered||!player->alive)continue;
        pending.eligible|=uint32_t(1)<<i;pending.victimEpochs[i]=player->spawnEpoch;
    }
    ++writeIndex;++firing.raysObserved;++s.raysObserved;++diagnostics.nativeRays;++diagnostics.queued;
    diagnostics.lastNativeOrigin=origin.sequence;return true;
}
inline void CapturePayload(Source& s,Effect& effect,int32_t nativeShot,uint32_t flags,const uint8_t* array){
    if(!Thread()||!s.observing||!effect.occurrence||effect.firingSlot>=s.firingCount
       ||s.firings[effect.firingSlot].component!=effect.firing||Read<void*>(effect.object,0x148)!=effect.firing)return;
    if(!RefreshFiringResources(s.firings[effect.firingSlot]))return;
    const auto data=Read<uint8_t*>(array,0);const int32_t count=Read<int32_t>(array,8),capacity=Read<int32_t>(array,12);
    if(count<1||count>64||capacity<count||!original_players::Readable(data,static_cast<size_t>(count)*48)){
        ++diagnostics.invalidPayloads;return;}
    const double now=Now();Origin* origin=nullptr;unsigned matches=0;
    // An already decoded initial effect occurrence must not steal the next
    // burst's reset native ShotIndex. Later real OnShot callbacks may use it.
    if(flags&4U)for(const auto& record:s.origins)if(record.consumed&&record.firing==effect.firing&&record.nativeShot==nativeShot
        &&record.occurrence==effect.occurrence&&original_inventory_slots::SameIdentity(record.effectIdentity,effect.identity)){
        ++diagnostics.duplicateRays;return;}
    for(auto& record:s.origins)if(record.valid&&!record.consumed&&record.firing==effect.firing
        &&original_inventory_slots::SameIdentity(record.firingIdentity,s.firings[effect.firingSlot].identity)&&record.nativeShot==nativeShot
        &&now-record.at>=0&&now-record.at<options.maximumShotAge){origin=&record;++matches;}
    if(matches!=1){if(matches)++diagnostics.ambiguousOrigins;else ++diagnostics.unpairedOrigins;return;}
    bool any=false;
    for(int32_t i=0;i<count;++i){const auto direction=Read<engine::Vec>(data+static_cast<size_t>(i)*48,0);
        const double norm=direction.x*direction.x+direction.y*direction.y+direction.z*direction.z;
        if(!original_shots::Finite(direction)||norm<.9||norm>1.1){++diagnostics.invalidPayloads;continue;}
        ++diagnostics.directions;any=true;
        // StartLocation in cosmetic initial FireData is intentionally ignored,
        // even if its uninitialized bytes happen to look finite.
        QueueRay(s,*origin,static_cast<uint32_t>(i),nativeShot,flags,direction);
    }
    if(any){origin->consumed=true;origin->occurrence=effect.occurrence;origin->effectIdentity=effect.identity;}
}

inline bool PollEffects(Source& s){
    for(auto& reference:s.effects){if(reference&&OwnsEffect(s,*reference))reference->present=false;else reference=nullptr;}
    for(size_t stateIndex=0;stateIndex<s.firingCount;++stateIndex){auto& firing=s.firings[stateIndex];
      if(!Live(firing.component,firing.identity))return Fail(2453,"training_combat_firing_component_expired_before_retirement");
      const auto objects=Read<void**>(firing.component,0x500);
      const auto count=Read<int32_t>(firing.component,0x508),capacity=Read<int32_t>(firing.component,0x50c);
      if(count<0||count>static_cast<int>(MaxEffectsPerState)||capacity<count
         ||(count&&!original_players::Readable(objects,static_cast<size_t>(count)*8)))return Fail(2405,"training_combat_original_effect_array_invalid");
      for(int32_t i=0;i<count;++i){auto object=Read<void*>(objects,static_cast<size_t>(i)*8);
        if(!original_players::IsA(object,0x684d570)||Read<void*>(object,0x148)!=firing.component)continue;
        auto effect=AcquireEffect(s,object,static_cast<uint32_t>(stateIndex));if(!effect)return false;
        CaptureInitial(s,*effect);
      }
    }
    for(auto& reference:s.effects)if(reference&&(!OwnsEffect(s,*reference)||!reference->present))
        if(!ReleaseEffect(s,reference))return false;
    return true;
}
inline bool SourceIdentity(const Source& s){
    const bool owner=s.bound&&Live(s.pawn,s.pawnId)&&Live(s.controller,s.controllerId)&&Live(s.weapon,s.weaponId)
        &&Live(s.inventory,s.inventoryId)
        &&Read<void*>(s.pawn,0x4a0)==s.controller&&Read<void*>(s.controller,0x4b0)==s.pawn
        &&Read<void*>(s.pawn,0xc08)==s.inventory&&Read<void*>(s.inventory,0xc8)==s.pawn
        &&Read<void*>(s.inventory,0x2f0)==s.pawn&&Read<void*>(s.weapon,0x4a8)==s.pawn;
    if(!owner)return false;if(!s.ballistic)return true;
    if(!s.firingCount||!Live(s.ammo,s.ammoId))return false;
    for(size_t i=0;i<s.firingCount;++i){const auto& firing=s.firings[i];
        if(!Live(firing.component,firing.identity)||Read<void*>(firing.component,0xc8)!=s.weapon
           ||Read<void*>(firing.component,0x4c8)!=s.pawn)return false;}
    return true;
}
inline bool InputIdentity(const Source& s){
    return SourceIdentity(s)&&Live(s.input,s.inputId)&&Live(s.inputState,s.inputStateId)
        &&Read<void*>(s.pawn,0xba0)==s.inputState&&Read<void*>(s.input,0xc8)==s.pawn&&Read<void*>(s.input,0x170)==s.pawn
        &&Read<void*>(s.inputState,0xc8)==s.pawn&&Read<void*>(s.inputState,0x148)==s.pawn
        &&Read<void*>(s.input,0)==reinterpret_cast<void*>(engine::image+0xb099220)
        &&Read<void*>(s.inputState,0)==reinterpret_cast<void*>(engine::image+0xb093c10);
}
inline bool PumpInput(Source& s){
    if(!InputIdentity(s))return Fail(2410,"training_combat_original_character_input_identity_invalid");
    if(engine::function<int(*)(void*)>(0x3efc820)(s.pawn)!=0)return Fail(2411,"training_combat_requires_standalone_local_transport");
    const auto count=Read<int32_t>(s.input,0x8b8),capacity=Read<int32_t>(s.input,0x8bc);
    if(count<0||count>1024||capacity<count||(count&&!Read<void*>(s.input,0x8b0)))return Fail(2412,"training_combat_original_action_buffer_invalid");
    if(!count)return true;
    engine::function<void(*)(void*,void*)>(0x70a7110)(s.inputState,static_cast<uint8_t*>(s.input)+0x178);
    if(Read<int32_t>(s.input,0x8b8))return Fail(2413,"training_combat_original_action_buffer_not_consumed");
    ++s.inputSubmissions;++diagnostics.inputSubmissions;return true;
}
inline bool SubmitViewInputImpl(size_t index){
    if(!Thread()||index>=sources.size())return false;auto& s=sources[index];
    if(!InputIdentity(s))return Fail(2414,"training_combat_original_view_input_identity_invalid");
    const auto history=Read<int32_t>(s.inputState,0x11c);
    if(!Read<void*>(s.inputState,0x108)||history<=0||history>4096)return Fail(2415,"training_combat_original_aim_history_not_ready");
    s.controlRotation=Read<engine::Vec>(s.controller,0x4e8);
    engine::Vec direction{};
    engine::function<engine::Vec*(*)(void*,engine::Vec*,void*)>(0x70a5420)(s.input,&direction,s.controller);
    const double norm=direction.x*direction.x+direction.y*direction.y+direction.z*direction.z;
    if(!original_shots::Finite(direction)||norm<.99||norm>1.01)return Fail(2416,"training_combat_original_view_sampler_invalid");
    s.nativeView=direction;s.aimValid=Read<uint8_t>(s.inputState,0x128)!=0;s.observedAim=Read<engine::Vec>(s.inputState,0x238);
    if(s.aimValid&&s.haveView&&Close(s.observedAim,s.lastView))++s.viewObservedMatches;
    if(s.haveView&&Close(direction,s.lastView))return true;
    const auto pending=Read<int32_t>(s.inputState,0xaa0),capacity=Read<int32_t>(s.inputState,0xaa4);
    if(pending<0||pending>1024||capacity<pending||(pending&&!Read<void*>(s.inputState,0xa98)))return Fail(2417,"training_combat_original_aim_pending_invalid");
    struct ViewEvent{uint8_t type=5,padding[7]{};engine::Vec direction;uint8_t tail[8]{};} event{5,{},direction,{}};
    static_assert(sizeof(ViewEvent)==40&&offsetof(ViewEvent,direction)==8);
    alignas(8) uint8_t buffer[0x748]{};
    engine::function<void*(*)(void*)>(0x70aa150)(buffer);bool sent=false;
    __try{
        const int32_t one=1;std::memcpy(buffer+0x740,&one,4);
        engine::function<void(*)(void*,int)>(0x18f5ce0)(buffer+0x738,0);
        auto data=Read<void*>(buffer,0x738);
        if(data&&Read<int32_t>(buffer,0x744)>=1){
            std::memcpy(data,&event,sizeof(event));engine::function<void(*)(void*,const void*)>(0x70a9a90)(buffer,&event);
            engine::function<void(*)(void*,void*)>(0x70a7110)(s.inputState,buffer);sent=Read<int32_t>(buffer,0x740)==0;
        }
    }__finally{engine::function<void(*)(void*)>(0x70aa200)(buffer);}
    if(!sent||Read<int32_t>(s.inputState,0xaa0)<pending)return Fail(2418,"training_combat_original_aim_append_failed");
    s.haveView=true;s.lastView=direction;++s.viewSubmissions;++diagnostics.viewSubmissions;return true;
}
inline bool SubmitViewInput(size_t index){__try{return SubmitViewInputImpl(index);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool InputEdges(Source& s,bool fire,bool reload,bool ads=false){
    if(!InputIdentity(s))return Fail(2419,"training_combat_original_force_input_context_invalid");
    const uint32_t desired=(fire?1U:0U)|(ads?2U:0U)|(reload?4U:0U);
    for(uint8_t input=0;input<3;++input){const uint32_t bit=uint32_t(1)<<input;
        if((desired&bit)==(s.held&bit))continue;
        struct Force{uint8_t input,state;} parameters{input,static_cast<uint8_t>((desired&bit)?1:0)};
        if(!original_damage::EventWithFunction(s.pawn,L"/Script/ShooterGame.ShooterCharacter.ForceInputState",forceInputFunction,2,&parameters))
            return Fail(2420,"training_combat_original_force_input_failed");
        s.held=(s.held&~bit)|(desired&bit);++s.inputEdges;++diagnostics.inputEdges;
    }
    return PumpInput(s);
}
inline bool ReleaseAllInput(Source& s){
    if(!InputIdentity(s))return Fail(2442,"training_combat_release_requires_original_input_identity");
    // The local physical producer can also have pressed ADS. Release every
    // original character trigger once at retirement, regardless of our cache.
    for(uint8_t input=0;input<3;++input){
        struct Force{uint8_t input,state;} parameters{input,0};
        if(!original_damage::EventWithFunction(s.pawn,L"/Script/ShooterGame.ShooterCharacter.ForceInputState",forceInputFunction,2,&parameters))
            return Fail(2443,"training_combat_original_release_input_failed");
        ++s.inputEdges;++diagnostics.inputEdges;
    }
    s.held=0;return PumpInput(s);
}
inline bool WeaponInputImpl(size_t index,bool fire,bool reload,bool ads){
    if(!Thread()||index>=sources.size()||!sources[index].bound)return false;auto& s=sources[index];
    const auto player=deathmatch_rules::Player(index);
    const bool live=player&&player->registered&&player->alive&&player->pawn==s.pawn&&player->spawnEpoch==s.spawnEpoch;
    const bool active=live&&deathmatch_rules::Active()&&s.observing;
    if(!active||(!player->local&&!deathmatch_rules::BotFireAllowed())){fire=false;reload=false;ads=false;}
    // Release always works while an original ability is current. New trigger
    // presses wait for this binding's real gun to be equipped and body ready.
    auto current=Read<void*>(s.inventory,0x280);
    const bool currentAllowed=current==s.weapon||(player&&player->local&&deathmatch_rules::AbilitiesAllowed()
        &&original_players::IsA(current,0x645c660)&&Read<void*>(current,0x4a8)==s.pawn);
    if(!currentAllowed||!s.physicsReady||!s.inputReady){fire=false;ads=false;}
    if(!s.inputReady){reload=false;++diagnostics.inputDeferred;}
    if(active&&s.inputReady&&!SubmitViewInputImpl(index))return false;
    return InputEdges(s,fire,reload,ads);
}
inline bool WeaponInput(size_t index,bool fire,bool reload){__try{return WeaponInputImpl(index,fire,reload,false);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool WeaponInput(size_t index,bool fire,bool reload,bool ads){__try{return WeaponInputImpl(index,fire,reload,ads);}__except(CaptureException(GetExceptionInformation())){return false;}}

inline bool CacheTuning(Source& s,FiringContext& firing,original_damage_tuning::Calculation* calculation=nullptr,
                        const original_shots::Shot* shot=nullptr,const original_damage::Hit* hit=nullptr){
    // The old tuning adapter is a singleton. Save/restore it on this one game
    // thread, with one cache per exact firing component. Never share local AK
    // damage data with a bot's different gun or projectile tuning.
    const auto saved=original_damage_tuning::diagnostics;const auto savedThread=original_damage_tuning::gameThread;
    original_damage_tuning::diagnostics=firing.tuningCached?firing.tuning:original_damage_tuning::Diagnostics{};
    original_damage_tuning::gameThread=gameThread;
    bool okay=true;
    if(!firing.tuningCached||original_damage_tuning::diagnostics.firing!=firing.component
       ||original_damage_tuning::diagnostics.projectileClass!=Read<void*>(firing.component,0x550)){
        original_damage_tuning::diagnostics={};original_damage_tuning::gameThread=gameThread;
        okay=original_damage_tuning::Initialize(firing.component);
    }
    if(okay&&calculation&&shot&&hit)okay=original_damage_tuning::Calculate(*shot,*hit,*calculation);
    firing.tuning=original_damage_tuning::diagnostics;firing.tuningCached=okay&&firing.tuning.ready;
    // These compatibility summaries describe the most recent actual source;
    // all future calculations select the explicit firing context again.
    s.tuning=firing.tuning;s.tuningCached=firing.tuningCached;
    const auto error=firing.tuning.error;const auto stage=firing.tuning.stage;const bool fatal=firing.tuning.restartRequired;
    original_damage_tuning::diagnostics=saved;original_damage_tuning::gameThread=savedThread;
    return okay?true:Fail(error?error:2421,stage,fatal);
}
inline bool RefreshFiringResources(FiringContext& firing,bool force){
    const auto resourceOverride=Read<void*>(firing.component,0x498),stabilityOverride=Read<void*>(firing.component,0x490);
    if(!force&&firing.resourceOverride==resourceOverride&&firing.stabilityOverride==stabilityOverride
       &&Live(firing.resource,firing.resourceIdentity)&&Live(firing.stability,firing.stabilityIdentity))return true;
    void *resource=nullptr,*stability=nullptr;
    if(!original_damage::EventWithFunction(firing.component,L"/Script/ShooterGame.FiringStateComponent.GetResourceComponent",resourceFunction,8,&resource)
       ||!original_damage::EventWithFunction(firing.component,L"/Script/ShooterGame.FiringStateComponent.GetStabilityComponent",stabilityFunction,8,&stability))
        return Fail(2454,"training_combat_original_firing_dependency_query_failed");
    ++firing.resourceQueries;firing.resourceOverride=resourceOverride;firing.stabilityOverride=stabilityOverride;
    if((resource&&!original_players::IsA(resource,0x637ed10))||(stability&&!original_players::IsA(stability,0x6d1e460)))
        return Fail(2455,"training_combat_original_firing_dependency_wrong_class");
    if(resource!=firing.resource||!Live(resource,firing.resourceIdentity)){
        firing.resource=resource;firing.resourceIdentity={};
        if(resource&&!Identity(resource,firing.resourceIdentity))return Fail(2456,"training_combat_original_resource_weak_identity_failed");}
    if(stability!=firing.stability||!Live(stability,firing.stabilityIdentity)){
        firing.stability=stability;firing.stabilityIdentity={};
        if(stability&&!Identity(stability,firing.stabilityIdentity))return Fail(2457,"training_combat_original_stability_weak_identity_failed");}
    firing.resourceIsAmmo=resource&&original_players::IsA(resource,0x63c7e00);return true;
}
inline bool GatherFiringStates(Source& s){
    struct Components {void* type;void** data=nullptr;int32_t count=0,capacity=0;} query{engine::Class(0x684d4c0)};
    static_assert(sizeof(Components)==24&&offsetof(Components,data)==8&&offsetof(Components,count)==16);
    bool valid=false;s.firingCount=0;
    __try{
        valid=original_damage::EventWithFunction(s.weapon,L"/Script/Engine.Actor.K2_GetComponentsByClass",componentsFunction,24,&query)
            &&query.count>=0&&query.count<=static_cast<int32_t>(MaxFiringStates)&&query.capacity>=query.count
            &&(!query.count||original_players::Readable(query.data,static_cast<size_t>(query.count)*8));
        if(valid)for(int32_t i=0;i<query.count;++i){
            auto component=Read<void*>(query.data,static_cast<size_t>(i)*8);
            if(!original_players::IsA(component,0x684d4c0)||Read<void*>(component,0xc8)!=s.weapon
               ||Read<void*>(component,0x4c8)!=s.pawn){valid=false;break;}
            bool duplicate=false;for(size_t j=0;j<s.firingCount;++j)duplicate|=s.firings[j].component==component;
            if(duplicate)continue;
            auto& context=s.firings[s.firingCount];context={};context.component=component;
            original_weapon::OriginalObjectName(component,context.name);
            if(!Identity(component,context.identity)||!RefreshFiringResources(context,true)){valid=false;break;}
            ++s.firingCount;
        }
    }__finally{if(query.data)engine::function<void(*)(void*)>(0x1915720)(query.data);}
    if(!valid)return Fail(diagnostics.error?diagnostics.error:2458,"training_combat_all_original_firing_states_not_ready");
    // Compatibility pointers remain summaries only. No observer or damage
    // calculation uses this first component to infer the shot-producing one.
    s.firing=s.firingCount?s.firings[0].component:nullptr;s.firingId=s.firingCount?s.firings[0].identity:Weak{};
    return true;
}
inline bool RememberRetired(void* object,const Weak& identity){
    if(!object||!Live(object,identity))return true;
    for(auto& r:retired){if(r.actor&&!Live(r.actor,r.identity))r={};
        if(r.actor==object&&original_inventory_slots::SameIdentity(r.identity,identity))return true;}
    for(auto& r:retired)if(!r.actor){r={object,identity};return true;}
    return Fail(2422,"training_combat_live_retired_actor_cache_full");
}
inline bool BeforeRetireImpl(size_t index,RetireReason reason){
    if(!Thread()||index>=sources.size())return false;auto& s=sources[index];if(!s.bound)return true;
    s.observing=false;
    if(InputIdentity(s)&&!ReleaseAllInput(s))return false;
    for(auto& reference:s.effects)if(!ReleaseEffect(s,reference))return false;
    const bool installed=s.pawnTable.installed;
    if(!Restore(s.pawn,s.pawnId,s.pawnTable))return false;
    if(installed)++diagnostics.restoredPawnTables;
    // A live Pawn that is merely switching guns remains a real damage target.
    // Do not put it in a corpse/scene-ignore cache. Its damage binding survives.
    if(reason!=RetireReason::WeaponChange&&!original_damage::targets[index].alive
       &&!RememberRetired(s.pawn,s.pawnId))return false;
    if(!RememberRetired(s.weapon,s.weaponId))return false;
    // All callbacks have been removed before any address/weak identity is
    // reused. Keep table backing storage allocated for the whole owned session.
    for(auto& reference:s.effects)reference=nullptr;
    for(auto& context:s.firings)context={};s.firingCount=0;s.lastFiringSlot=-1;
    for(auto& origin:s.origins)origin={};for(auto& ray:s.recent)ray={};
    s.bound=false;s.alive=false;s.physicsReady=false;s.inputReady=false;s.current=false;s.tuningCached=false;s.haveView=false;s.held=0;
    s.inputRestartAttempted=false;s.restartPawn=nullptr;s.restartPawnId={};
    s.pawn=s.controller=s.weapon=s.inventory=s.firing=s.ammo=s.input=s.inputState=nullptr;
    s.pawnId=s.controllerId=s.weaponId=s.inventoryId=s.firingId=s.ammoId=s.inputId=s.inputStateId={};
    s.forwardView=nullptr;s.pawnTable.previous=nullptr;s.pawnTable.functions=0;
    ++diagnostics.retirements;return true;
}
inline bool BeforeRetire(size_t index,RetireReason reason){__try{return BeforeRetireImpl(index,reason);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool Refresh(Source& s,double now,bool force=false){
    if(!SourceIdentity(s))return Fail(2423,"training_combat_original_source_identity_expired",true);
    const auto player=deathmatch_rules::Player(s.index);
    if(player&&player->registered&&(player->pawn!=s.pawn||player->spawnEpoch!=s.spawnEpoch))return Fail(2424,"training_combat_roster_epoch_changed_before_retire",true);
    s.current=Read<void*>(s.inventory,0x280)==s.weapon;
    const auto historyCapacity=Read<int32_t>(s.inputState,0x11c);
    s.inputReady=InputIdentity(s)&&Read<void*>(s.inputState,0x108)&&historyCapacity>0&&historyCapacity<=4096;
    s.aimValid=Read<uint8_t>(s.inputState,0x128)!=0;s.observedAim=Read<engine::Vec>(s.inputState,0x238);
    if(s.haveView&&s.aimValid&&Close(s.observedAim,s.lastView))++s.viewObservedMatches;
    if(force||now-s.sampledAt>=options.sampleInterval){
        for(size_t i=0;i<s.firingCount;++i)if(!RefreshFiringResources(s.firings[i]))return false;
        if(!original_damage::Sample(s.index))return Fail(original_damage::State().error,original_damage::State().stage,original_damage::State().restartRequired);
        const auto& target=original_damage::targets[s.index];s.alive=target.alive;s.physicsReady=target.originalPhysics;
        s.bodyCount=target.bodyCount;s.physicsActors=target.physicsActors;s.ammoCount=s.ballistic?Read<int32_t>(s.ammo,0x10c):-1;
        if(s.lastFiringSlot>=0&&static_cast<size_t>(s.lastFiringSlot)<s.firingCount){const auto& firing=s.firings[static_cast<size_t>(s.lastFiringSlot)];
            s.ammoCount=firing.resourceIsAmmo&&Live(firing.resource,firing.resourceIdentity)?Read<int32_t>(firing.resource,0x10c):-1;}
        s.sampledAt=now;
    }
    return true;
}
inline bool PrepareOriginalInput(Source& s){
    if(!original_players::IsA(s.controller,original_players::ShooterControllerClass)
       ||Read<void*>(s.controller,0x4b0)!=s.pawn||Read<void*>(s.pawn,0x4a0)!=s.controller)
        return Fail(2444,"training_combat_input_requires_real_possession");
    auto local=original_players::localPlayer;auto localController=local?Read<void*>(local,0x38):nullptr;
    const auto role=Read<uint8_t>(s.pawn,0x224),remoteRole=Read<uint8_t>(s.pawn,0x128);
    // A real NPC can have no UPlayer and still use its real PC/Pawn input
    // objects. Never transfer the one LocalPlayer or call online GI startup.
    if(!Read<void*>(s.controller,0x5e8)||!Read<void*>(s.controller,0x230)){
        auto pcTable=Read<void**>(s.controller,0);auto init=Read<void*>(pcTable,0xfe8);
        if(init!=reinterpret_cast<void*>(engine::image+0x781cbd0))return Fail(2445,"training_combat_original_pc_input_initializer_changed");
        reinterpret_cast<void(*)(void*)>(init)(s.controller);++s.pcInputInitializations;
    }
    if(!original_players::IsA(Read<void*>(s.controller,0x5e8),0x64f9c00)||!Read<void*>(s.controller,0x230))
        return Fail(2446,"training_combat_original_pc_input_initialization_pending");
    auto input=original_weapon::Component(s.pawn,0x6492f30);
    if(!input){
        if(Read<void*>(s.pawn,0x230))return Fail(2447,"training_combat_existing_pawn_input_is_not_ares");
        if(s.restartPawn!=s.pawn||!Live(s.restartPawn,s.restartPawnId)){
            s.restartPawn=s.pawn;s.inputRestartAttempted=false;if(!Identity(s.pawn,s.restartPawnId))return false;
        }
        if(s.inputRestartAttempted)return Fail(2448,"training_combat_original_pawn_input_initialization_pending");
        auto pawnTable=Read<void**>(s.pawn,0);
        if(Read<void*>(pawnTable,0x828)!=reinterpret_cast<void*>(engine::image+0x786d960)
           ||Read<void*>(pawnTable,0x888)!=reinterpret_cast<void*>(engine::image+0x787c520)
           ||Read<void*>(pawnTable,0x898)!=reinterpret_cast<void*>(engine::image+0x787c8d0))
            return Fail(2449,"training_combat_original_pawn_input_lifecycle_changed");
        s.inputRestartAttempted=true;
        // The same native leaf used by PC.ClientRestart. It creates the real
        // AresInputComponent through v888, calls its original setup v898,
        // registers it and binds Blueprint delegates. It owns Pawn+230 writes.
        reinterpret_cast<void(*)(void*)>(Read<void*>(pawnTable,0x828))(s.pawn);++s.pawnClientRestarts;
        input=original_weapon::Component(s.pawn,0x6492f30);
    }
    if((local&&Read<void*>(local,0x38)!=localController)||Read<uint8_t>(s.pawn,0x224)!=role
       ||Read<uint8_t>(s.pawn,0x128)!=remoteRole)return Fail(2450,"training_combat_input_lifecycle_changed_original_identity",true);
    s.input=input;s.inputState=Read<void*>(s.pawn,0xba0);
    return original_players::IsA(s.input,0x6492f30)&&Read<void*>(s.pawn,0x230)==s.input
        &&original_players::IsA(s.inputState,0x6494370)
        ?true:Fail(2451,"training_combat_original_pawn_input_lifecycle_not_ready");
}
inline bool BindImpl(size_t index,void* pawn,void* weapon,uint32_t spawnEpoch){
    if(!Thread()||index>=sources.size()||!spawnEpoch)return Fail(2425,"training_combat_bind_context_invalid");
    auto& s=sources[index];
    if(s.bound){if(s.pawn==pawn&&s.weapon==weapon&&s.spawnEpoch==spawnEpoch)return Refresh(s,Now(),true);
        return Fail(2426,"training_combat_before_retire_required_for_new_binding");}
    if(!original_players::IsA(pawn,original_players::ShooterCharacterClass)||!original_players::IsA(weapon,0x645c660)
       ||Read<void*>(weapon,0x4a8)!=pawn)return Fail(2427,"training_combat_bind_requires_original_owned_gun");
    s.index=index;s.spawnEpoch=spawnEpoch;s.binding=++nextBinding;s.pawn=pawn;s.weapon=weapon;
    s.controller=Read<void*>(pawn,0x4a0);s.inventory=Read<void*>(pawn,0xc08);
    s.ammo=original_weapon::Component(weapon,0x63c7e00);
    if(!GatherFiringStates(s))return false;
    s.ballistic=s.firingCount>0;
    if(!s.ballistic){
        // The authored melee equippable is a real non-ballistic context. It
        // keeps native trigger/animation and can be damaged; no fake ranged
        // component or guessed melee hit/damage is supplied by this bridge.
        auto melee=engine::Find(L"/Game/Equippables/Melee/Ability_Melee_Base.Ability_Melee_Base_C");
        if(s.firing||s.ammo||!melee||!original_players::Derived(Read<void*>(weapon,0x10),melee))
            return Fail(2452,"training_combat_original_firearm_components_not_ready");
    }
    if(!PrepareOriginalInput(s))return false;
    if(!original_players::IsA(s.controller,original_players::ShooterControllerClass)
       ||(s.ballistic&&(!original_players::IsA(s.firing,0x684d4c0)||!original_players::IsA(s.ammo,0x63c7e00)))
       ||!original_players::IsA(s.input,0x6492f30)||!original_players::IsA(s.inputState,0x6494370))
        return Fail(2428,"training_combat_original_pawn_client_input_lifecycle_required");
    if(!Identity(pawn,s.pawnId)||!Identity(s.controller,s.controllerId)||!Identity(weapon,s.weaponId)
       ||!Identity(s.inventory,s.inventoryId)||(s.ballistic&&(!Identity(s.firing,s.firingId)||!Identity(s.ammo,s.ammoId)))
       ||!Identity(s.input,s.inputId)||!Identity(s.inputState,s.inputStateId))return Fail(2429,"training_combat_original_weak_identity_failed");
    s.bound=true;s.observing=false;s.held=0;s.haveView=false;s.tuningCached=false;s.sampledAt=-1000;
    s.originCursor=s.recentCursor=s.rayCursor=0;for(auto& r:s.origins)r={};for(auto& r:s.recent)r={};
    if(!InputIdentity(s)||!original_damage::Bind(index,pawn)){
        s.bound=false;return Fail(diagnostics.error?diagnostics.error:2430,"training_combat_native_input_damage_tuning_bind_failed");}
    if(s.ballistic&&!CopyVtable(pawn,s.pawnTable,0x850/8+1)){
        s.bound=false;return Fail(2431,"training_combat_original_complete_pawn_vtable_unavailable");}
    auto currentTable=Read<void**>(pawn,0);
    s.forwardView=reinterpret_cast<NativeView>(Read<void*>(currentTable,0x850));
    if(s.forwardView!=engine::function<NativeView>(0x787f820)){
        s.bound=false;return Fail(2432,"training_combat_legacy_or_other_origin_observer_active");}
    if(s.ballistic&&!Install(pawn,s.pawnTable,0x850/8,reinterpret_cast<void*>(ObserveViewLocation))){s.bound=false;return Fail(2433,"training_combat_pawn_instance_origin_install_failed");}
    s.observing=true;++diagnostics.binds;
    if(!Refresh(s,Now(),true))return false;
    diagnostics.error=0;diagnostics.stage="training_combat_original_source_bound";return true;
}
inline bool Bind(size_t index,void* pawn,void* weapon,uint32_t spawnEpoch){__try{return BindImpl(index,pawn,weapon,spawnEpoch);}__except(CaptureException(GetExceptionInformation())){return false;}}

inline int SceneTrace(const engine::Vec& start,const engine::Vec& direction,double maximum,double& distance){
    std::array<void*,MaxPlayers*2+RetiredCapacity> ignored{};int32_t count=0;
    const auto add=[&](void* actor){if(!actor)return;for(int32_t i=0;i<count;++i)if(ignored[i]==actor)return;
        if(static_cast<size_t>(count)<ignored.size())ignored[count++]=actor;};
    for(const auto& s:sources)if(s.bound){
        // Live, unready original bodies stay native scene obstructions. The
        // pending shot is deferred before tracing; no capsule can award damage.
        if((!s.alive||s.physicsReady)&&Live(s.pawn,s.pawnId))add(s.pawn);
        if(Live(s.weapon,s.weaponId))add(s.weapon);
    }
    for(auto& r:retired)if(r.actor){if(Live(r.actor,r.identity))add(r.actor);else r={};}
    alignas(16) uint8_t parameters[0x178]{};const auto end=engine::Add(start,engine::Mul(direction,maximum));
    std::memcpy(parameters,&world,8);std::memcpy(parameters+8,&start,24);std::memcpy(parameters+0x20,&end,24);
    parameters[0x39]=1;parameters[0x148]=1;
    struct Actors{void** data;int32_t count,capacity;} actors{ignored.data(),count,count};
    std::memcpy(parameters+0x40,&actors,sizeof(actors));++diagnostics.sceneTraces;
    if(!Event(engine::DefaultObject(engine::Class(0x45aa110)),L"/Script/Engine.KismetSystemLibrary.LineTraceSingle",0x171,parameters))return -1;
    if(!parameters[0x170]||!(parameters[0x105]&1)){distance=(std::numeric_limits<double>::infinity)();return 0;}
    distance=Read<float>(parameters,0x60);return std::isfinite(distance)&&distance>=0?1:-1;
}
enum class Outcome:uint8_t{Done,Deferred,Failure};
inline Outcome Process(Pending& pending,double now){
    const auto& key=pending.identity;const auto& shot=pending.shot;
    if(key.source>=sources.size())return Outcome::Failure;auto& source=sources[key.source];
    if(!source.bound||source.binding!=key.binding||source.spawnEpoch!=key.spawnEpoch){++diagnostics.expiredBindings;return Outcome::Done;}
    if(key.firingSlot>=source.firingCount){++diagnostics.expiredBindings;return Outcome::Done;}
    auto& firing=source.firings[key.firingSlot];
    if(firing.component!=pending.firing||!original_inventory_slots::SameIdentity(firing.identity,pending.firingIdentity)){
        ++diagnostics.expiredBindings;return Outcome::Done;}
    if(Read<void*>(firing.component,0x550)!=pending.projectileClass){++diagnostics.changedProjectileRejected;return Outcome::Done;}
    if(now-shot.observed_at<-.05||now-shot.observed_at>options.maximumShotAge){++diagnostics.staleShots;return Outcome::Done;}
    if(!Refresh(source,now,true))return Outcome::Failure;
    const auto sourcePlayer=deathmatch_rules::Player(key.source);
    if(!sourcePlayer||!sourcePlayer->registered||!sourcePlayer->alive||!source.alive||!deathmatch_rules::Active())return Outcome::Done;
    if(!source.physicsReady){++diagnostics.physicsDeferred;return Outcome::Deferred;}
    size_t nearest=sources.size();original_damage::Hit nearestHit;double nearestDistance=options.maximumDistance;
    for(size_t i=0;i<sources.size();++i){if(!(pending.eligible&(uint32_t(1)<<i)))continue;
        auto& target=sources[i];const auto p=deathmatch_rules::Player(i);
        if(!p||!p->registered||p->spawnEpoch!=pending.victimEpochs[i]){
            ++diagnostics.retiredVictimEpochs;pending.eligible&=~(uint32_t(1)<<i);continue;}
        if(!p->alive)continue;
        if(!target.bound){++diagnostics.bindingDeferred;return Outcome::Deferred;}
        if(target.spawnEpoch!=pending.victimEpochs[i]){
            ++diagnostics.retiredVictimEpochs;pending.eligible&=~(uint32_t(1)<<i);continue;}
        if(!Refresh(target,now,true))return Outcome::Failure;if(!target.alive)continue;
        if(!target.physicsReady){++diagnostics.physicsDeferred;return Outcome::Deferred;}
        original_damage::Hit hit;++diagnostics.bodyTraces;
        const int traced=original_damage::Trace(i,shot,options.maximumDistance,hit);
        if(traced<0){if(original_damage::State().error==1134){target.physicsReady=false;++diagnostics.physicsDeferred;return Outcome::Deferred;}
            Fail(original_damage::State().error,original_damage::State().stage,original_damage::State().restartRequired);return Outcome::Failure;}
        if(traced==1&&hit.distance<nearestDistance){nearest=i;nearestHit=hit;nearestDistance=hit.distance;}
    }
    if(nearest==sources.size()){++diagnostics.misses;return Outcome::Done;}
    double sceneDistance=0;const int scene=SceneTrace(shot.start,shot.direction,options.maximumDistance,sceneDistance);
    if(scene<0){Fail(2434,"training_combat_original_scene_obstruction_trace_failed");return Outcome::Failure;}
    diagnostics.lastBodyDistance=nearestDistance;diagnostics.lastSceneDistance=std::isfinite(sceneDistance)?sceneDistance:-1;
    if(scene&&sceneDistance+options.obstructionTolerance<nearestDistance){++diagnostics.sceneBlocked;return Outcome::Done;}
    if(!deathmatch_rules::CanDamage(key.source,nearest)){++diagnostics.policyRejected;return Outcome::Done;}
    if(original_damage::targets[nearest].pendingDamageEvents){++diagnostics.pendingDamageDeferred;return Outcome::Deferred;}
    original_damage_tuning::Calculation amount;diagnostics.operation="per_source_original_damage_tuning";
    if(!CacheTuning(source,firing,&amount,&shot,&nearestHit))return Outcome::Failure;
    if(!amount.valid||!amount.originalRegionVerified||amount.totalDamage<=0)return Outcome::Done;
    original_damage::DeathData death;
    if(!deathmatch_rules::BeginNativeDamage(pending.damageToken,key.source,nearest,source.weapon,now,death)){
        if(!deathmatch_rules::CanDamage(key.source,nearest)){++diagnostics.policyRejected;return Outcome::Done;}
        Fail(deathmatch_rules::State().error,deathmatch_rules::State().stage);return Outcome::Failure;}
    original_damage::Result result;diagnostics.operation="original_point_damage_and_native_rule_commit";
    const bool applied=original_damage::ApplyPoint(nearest,source.pawn,source.weapon,amount.damageType,shot,nearestHit,
        amount.totalDamage,pending.damageToken,result,&death,amount.falloffMultiplier);
    // End is mandatory even on a native failure; it clears the transaction.
    const bool committed=deathmatch_rules::EndNativeDamage(applied,result);
    if(!applied||!committed){Fail(!applied?original_damage::State().error:deathmatch_rules::State().error,
        !applied?original_damage::State().stage:deathmatch_rules::State().stage);return Outcome::Failure;}
    diagnostics.lastSource=static_cast<int32_t>(key.source);diagnostics.lastVictim=static_cast<int32_t>(nearest);
    diagnostics.lastToken=pending.damageToken;diagnostics.lastDamage=amount.totalDamage;
    if(result.applied&&!result.duplicate){++source.damageEvents;++diagnostics.damageEvents;}
    if(result.killed&&!result.duplicate){++source.kills;++diagnostics.kills;
        // Stop/restore our instance observers immediately after the original
        // death notification, before the match factory can retire that Pawn.
        if(!BeforeRetire(nearest,RetireReason::NativeDeath))return Outcome::Failure;}
    diagnostics.operation="none";return Outcome::Done;
}
inline bool Initialize(void* originalWorld){
    if(diagnostics.initialized)return world==originalWorld&&Thread();
    if(!original_players::Thread()||!originalWorld||originalWorld!=original_players::world)return Fail(2435,"training_combat_original_world_and_thread_required");
    if(!std::isfinite(options.maximumDistance)||options.maximumDistance<=0||options.maximumDistance>1000000
       ||!std::isfinite(options.maximumShotAge)||options.maximumShotAge<=0||options.maximumShotAge>1
       ||!std::isfinite(options.obstructionTolerance)||options.obstructionTolerance<0||options.obstructionTolerance>5
       ||!std::isfinite(options.sampleInterval)||options.sampleInterval<=0||options.sampleInterval>1
       ||!options.maximumShotsPerPoll||options.maximumShotsPerPoll>QueueCapacity)return Fail(2436,"training_combat_local_policy_invalid");
    if(original_shots::State().initialized)return Fail(2437,"training_combat_v2_requires_legacy_single_shot_observer_disabled");
    shotFunction=engine::Find(L"/Script/ShooterGame.FiringEffectComponent.FiringStateOnShot");
    startFunction=engine::Find(L"/Script/ShooterGame.FiringEffectComponent.HandleStartEffect");
    forceInputFunction=engine::Find(L"/Script/ShooterGame.ShooterCharacter.ForceInputState");
    componentsFunction=engine::Find(L"/Script/Engine.Actor.K2_GetComponentsByClass");
    resourceFunction=engine::Find(L"/Script/ShooterGame.FiringStateComponent.GetResourceComponent");
    stabilityFunction=engine::Find(L"/Script/ShooterGame.FiringStateComponent.GetStabilityComponent");
    if(!shotFunction||!startFunction||!forceInputFunction||Read<uint16_t>(shotFunction,0xbe)!=32||Read<uint16_t>(forceInputFunction,0xbe)!=2)
        return Fail(2438,"training_combat_original_event_abi_mismatch");
    if(!componentsFunction||!resourceFunction||!stabilityFunction||Read<uint16_t>(componentsFunction,0xbe)!=24
       ||Read<uint16_t>(resourceFunction,0xbe)!=8||Read<uint16_t>(stabilityFunction,0xbe)!=8)
        return Fail(2460,"training_combat_original_firing_context_abi_mismatch");
    auto ntdllModule=GetModuleHandleW(L"ntdll.dll");
    if(ntdllModule){captureContext=reinterpret_cast<CaptureContextFn>(GetProcAddress(ntdllModule,"RtlCaptureContext"));
        lookupFunction=reinterpret_cast<LookupFunctionFn>(GetProcAddress(ntdllModule,"RtlLookupFunctionEntry"));
        virtualUnwind=reinterpret_cast<VirtualUnwindFn>(GetProcAddress(ntdllModule,"RtlVirtualUnwind"));}
    if(!captureContext||!lookupFunction||!virtualUnwind)return Fail(2461,"training_combat_current_thread_native_unwind_unavailable");
    constexpr uint8_t sampler[]{0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x48,0x89,0x70,0x20,0x57,0x48,0x81,0xec,0x60};
    constexpr uint8_t constructor[]{0x48,0x89,0x4c,0x24,0x08,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9,0xc7,0x41,0x04};
    constexpr uint8_t submit[]{0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0xf9,0x48,0x8b,0xda};
    constexpr uint8_t retainedThis[]{0x48,0x8b,0xf1};
    diagnostics.nativeEntriesVerified=std::memcmp(reinterpret_cast<void*>(engine::image+0x70a5420),sampler,sizeof(sampler))==0
        &&std::memcmp(reinterpret_cast<void*>(engine::image+0x70aa150),constructor,sizeof(constructor))==0
        &&std::memcmp(reinterpret_cast<void*>(engine::image+0x70a7110),submit,sizeof(submit))==0
        &&std::memcmp(reinterpret_cast<void*>(engine::image+0x711f032),retainedThis,sizeof(retainedThis))==0
        &&std::memcmp(reinterpret_cast<void*>(engine::image+0x712279c),retainedThis,sizeof(retainedThis))==0;
    if(!diagnostics.nativeEntriesVerified)return Fail(2439,"training_combat_original_native_transport_entry_mismatch");
    if(!original_damage::Initialize(originalWorld))return Fail(original_damage::State().error,original_damage::State().stage);
    world=originalWorld;gameThread=GetCurrentThreadId();diagnostics.initialized=true;
    diagnostics.error=0;diagnostics.stage="training_combat_waiting_original_source_bindings";return true;
}
inline bool PollImpl(double now){
    if(!Thread()||!std::isfinite(now)||now<0||now+.05<diagnostics.lastPoll)return Fail(2440,"training_combat_invalid_game_thread_or_clock");
    diagnostics.lastPoll=now;diagnostics.boundSources=diagnostics.aliveSources=diagnostics.physicsReadySources=0;
    bool missingBinding=false;
    for(size_t i=0;i<sources.size();++i){auto& s=sources[i];const auto player=deathmatch_rules::Player(i);
        if(!s.bound){if(player&&player->registered&&player->alive&&player->ready){missingBinding=true;++diagnostics.bindingDeferred;}continue;}
        if(!Refresh(s,now))return false;
        if(!s.alive){if(!BeforeRetire(i,RetireReason::NativeDeath))return false;continue;}
        ++diagnostics.boundSources;++diagnostics.aliveSources;if(s.physicsReady)++diagnostics.physicsReadySources;
        if(s.ballistic&&!PollEffects(s))return false;
    }
    // Handlers run outside FireShot/ProcessEvent and before any point-damage
    // transaction, so native protection removal cannot recursively fire/score.
    if(!NotifyWeaponUses())return false;
    if(!missingBinding){const auto end=writeIndex;size_t budget=options.maximumShotsPerPoll;
        while(readIndex<end&&budget--){auto& pending=queue[readIndex%queue.size()];const auto result=Process(pending,now);
            if(result==Outcome::Failure)return false;if(result==Outcome::Deferred)break;
            ++readIndex;++diagnostics.processed;}}
    diagnostics.ready=deathmatch_rules::Active()&&!missingBinding&&diagnostics.physicsReadySources>0;
    diagnostics.error=0;diagnostics.stage=diagnostics.ready?"training_combat_original_multi_source_active":"training_combat_waiting_original_bindings_physics_or_match";
    diagnostics.operation="none";return true;
}
inline bool Poll(double now){
    if(diagnostics.processing)return Fail(2441,"training_combat_reentrant_poll_rejected");diagnostics.processing=true;bool okay=false;
    __try{okay=PollImpl(now);}__except(CaptureException(GetExceptionInformation())){okay=false;}
    diagnostics.processing=false;return okay;
}
inline bool VisibleImpl(size_t source,size_t target){
    ++diagnostics.visibilityQueries;
    if(!Thread()||source>=sources.size()||target>=sources.size()||source==target)return false;
    auto& s=sources[source];auto& t=sources[target];const double now=Now();
    if(!s.bound||!t.bound||!Refresh(s,now)||!Refresh(t,now)||!s.alive||!t.alive||!s.physicsReady||!t.physicsReady){++diagnostics.visibilityDeferred;return false;}
    engine::Vec start{},end{};
    if(!s.forwardView||!s.forwardView(s.pawn,&start)||!original_shots::WorldPoint(start)
       ||!Event(t.pawn,L"/Script/Engine.Actor.K2_GetActorLocation",24,&end)||!original_shots::WorldPoint(end))return false;
    const auto delta=engine::Add(end,engine::Mul(start,-1));const double distance=std::sqrt(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z);
    if(!std::isfinite(distance)||distance<1||distance>options.maximumDistance)return false;
    const auto direction=engine::Mul(delta,1./distance);double sceneDistance=0;
    const int hit=SceneTrace(start,direction,distance,sceneDistance);
    return hit>=0&&(!hit||sceneDistance+options.obstructionTolerance>=distance);
}
inline bool Visible(size_t source,size_t target){__try{return VisibleImpl(source,target);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool Pop(ObservedRay& ray){
    if(!Thread()||observationRead==observationWrite)return false;
    ray=observations[observationRead++%observations.size()];return true;
}
inline void JsonVector(FILE* file,const engine::Vec& value){
    std::fputc('[',file);const double values[]{value.x,value.y,value.z};
    for(size_t i=0;i<3;++i){if(i)std::fputc(',',file);
        if(std::isfinite(values[i]))std::fprintf(file,"%.17g",values[i]);else std::fputs("null",file);}
    std::fputc(']',file);
}
inline bool WriteDiagnostics(const wchar_t* path){
    FILE* file=nullptr;if(!path||_wfopen_s(&file,path,L"wb")||!file)return false;const auto& d=diagnostics;
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(file,"{\"schema_version\":1,\"initialized\":%s,\"ready\":%s,\"restart_required\":%s,\"native_entries_verified\":%s,\"error\":%u,\"stage\":",
        b(d.initialized),b(d.ready),b(d.restartRequired),b(d.nativeEntriesVerified),d.error);original_players::Text(file,d.stage);
    std::fputs(",\"operation\":",file);original_players::Text(file,d.operation);
#define COMBAT_COUNT(field) std::fprintf(file,",\"" #field "\":%llu",static_cast<unsigned long long>(d.field))
    COMBAT_COUNT(boundSources);COMBAT_COUNT(aliveSources);COMBAT_COUNT(physicsReadySources);COMBAT_COUNT(binds);COMBAT_COUNT(retirements);
    COMBAT_COUNT(restoredPawnTables);COMBAT_COUNT(restoredEffectTables);COMBAT_COUNT(originQueries);COMBAT_COUNT(exactOrigins);
    COMBAT_COUNT(nativeContextWalks);COMBAT_COUNT(nativeContextMatches);COMBAT_COUNT(nativeContextRejected);COMBAT_COUNT(unknownFiringOrigins);
    COMBAT_COUNT(changedProjectileRejected);
    COMBAT_COUNT(effectObserverRecords);COMBAT_COUNT(effectObserverReclaims);COMBAT_COUNT(effectLeaseTransfers);COMBAT_COUNT(effectObserverReinstalls);
    COMBAT_COUNT(effectOrphanRecoveries);COMBAT_COUNT(effectLeaseCallbackRebinds);COMBAT_COUNT(effectRegistryIdentityRejects);
    COMBAT_COUNT(effectObservedDispatches);COMBAT_COUNT(effectOriginalForwardAttempts);COMBAT_COUNT(effectOriginalForwardCompleted);COMBAT_COUNT(effectStartCallbacks);
    COMBAT_COUNT(weaponUsedQueued);COMBAT_COUNT(weaponUsedNotified);COMBAT_COUNT(weaponUsedExpired);COMBAT_COUNT(weaponUsedOverflow);
    COMBAT_COUNT(effectCallbacks);COMBAT_COUNT(directions);COMBAT_COUNT(nativeRays);COMBAT_COUNT(invalidPayloads);COMBAT_COUNT(unpairedOrigins);
    COMBAT_COUNT(ambiguousOrigins);COMBAT_COUNT(duplicateRays);COMBAT_COUNT(queued);COMBAT_COUNT(processed);COMBAT_COUNT(queueOverflow);
    COMBAT_COUNT(staleShots);COMBAT_COUNT(expiredBindings);COMBAT_COUNT(retiredVictimEpochs);COMBAT_COUNT(physicsDeferred);
    COMBAT_COUNT(inputDeferred);COMBAT_COUNT(pendingDamageDeferred);COMBAT_COUNT(bindingDeferred);COMBAT_COUNT(bodyTraces);COMBAT_COUNT(sceneTraces);
    COMBAT_COUNT(sceneBlocked);COMBAT_COUNT(misses);COMBAT_COUNT(policyRejected);COMBAT_COUNT(damageEvents);COMBAT_COUNT(kills);
    COMBAT_COUNT(viewSubmissions);COMBAT_COUNT(inputEdges);COMBAT_COUNT(inputSubmissions);COMBAT_COUNT(visibilityQueries);
    COMBAT_COUNT(visibilityDeferred);COMBAT_COUNT(foreignTableConflicts);COMBAT_COUNT(lastToken);COMBAT_COUNT(lastNativeOrigin);
#undef COMBAT_COUNT
    std::fprintf(file,",\"weapon_used_handler_registered\":%s,\"last_effect_object\":\"%p\",\"last_effect_current_table\":\"%p\",\"last_effect_previous_table\":\"%p\",\"last_effect_old_source\":%d,\"last_effect_new_source\":%d,\"last_effect_old_binding\":%llu,\"last_effect_new_binding\":%llu",
        b(weaponUsedCallback!=nullptr),d.lastEffectObject,d.lastEffectCurrentTable,d.lastEffectPreviousTable,d.lastEffectOldSource,d.lastEffectNewSource,
        static_cast<unsigned long long>(d.lastEffectOldBinding),static_cast<unsigned long long>(d.lastEffectNewBinding));
    std::fprintf(file,",\"pending_shots\":%llu,\"last_source\":%d,\"last_victim\":%d,\"last_damage\":%.9g,\"last_body_distance\":%.9g,\"last_scene_distance\":%.9g,\"exception_rva\":%llu,\"native_context_exception_code\":%u,\"players\":[",
        static_cast<unsigned long long>(writeIndex-readIndex),d.lastSource,d.lastVictim,d.lastDamage,d.lastBodyDistance,d.lastSceneDistance,static_cast<unsigned long long>(d.exceptionRva),d.nativeContextExceptionCode);
    bool comma=false;
    for(const auto& s:sources)if(s.bound){std::fprintf(file,"%s{\"index\":%zu,\"spawn_epoch\":%u,\"binding\":%llu,\"pawn\":\"%p\",\"controller\":\"%p\",\"weapon\":\"%p\",\"firing\":\"%p\",\"alive\":%s,\"physics_ready\":%s,\"input_ready\":%s,\"ballistic\":%s,\"current_gun\":%s,\"body_count\":%d,\"physics_actors\":%d,\"ammo\":%d,\"pawn_vtable_functions\":%zu,\"origins\":%llu,\"rays\":%llu,\"effect_hooks\":%llu,\"view_submissions\":%llu,\"view_observed_matches\":%llu,\"aim_valid\":%s,\"damage_events\":%llu,\"kills\":%llu,\"pc_input_initializations\":%llu,\"pawn_client_restarts\":%llu,\"native_sample\":",
        comma?",":"",s.index,s.spawnEpoch,static_cast<unsigned long long>(s.binding),s.pawn,s.controller,s.weapon,s.firing,b(s.alive),b(s.physicsReady),b(s.inputReady),b(s.ballistic),b(s.current),s.bodyCount,s.physicsActors,s.ammoCount,s.pawnTable.functions,
        static_cast<unsigned long long>(s.originsObserved),static_cast<unsigned long long>(s.raysObserved),static_cast<unsigned long long>(s.effectHooks),static_cast<unsigned long long>(s.viewSubmissions),static_cast<unsigned long long>(s.viewObservedMatches),b(s.aimValid),
        static_cast<unsigned long long>(s.damageEvents),static_cast<unsigned long long>(s.kills),static_cast<unsigned long long>(s.pcInputInitializations),static_cast<unsigned long long>(s.pawnClientRestarts));
        JsonVector(file,s.nativeView);std::fputs(",\"observed_aim\":",file);JsonVector(file,s.observedAim);
        std::fputs(",\"controller_rotation\":",file);JsonVector(file,s.controlRotation);
        std::fprintf(file,",\"firing_state_count\":%zu,\"last_firing_slot\":%d,\"firing_states\":[",s.firingCount,s.lastFiringSlot);
        for(size_t i=0;i<s.firingCount;++i){const auto& firing=s.firings[i];
            std::fprintf(file,"%s{\"slot\":%zu,\"component\":\"%p\",\"name\":",i?",":"",i,firing.component);original_players::Text(file,firing.name);
            std::fprintf(file,",\"weak_index\":%d,\"weak_serial\":%d,\"resource\":\"%p\",\"stability\":\"%p\",\"registered_resource\":\"%p\",\"registered_stability\":\"%p\",\"resource_is_ammo\":%s,\"origins\":%llu,\"rays\":%llu,\"effect_hooks\":%llu,\"dependency_queries\":%llu,\"tuning_cached\":%s,\"tuning_projectile_class\":\"%p\"}",
                firing.identity.index,firing.identity.serial,firing.resource,firing.stability,firing.resourceOverride,firing.stabilityOverride,b(firing.resourceIsAmmo),
                static_cast<unsigned long long>(firing.originsObserved),static_cast<unsigned long long>(firing.raysObserved),static_cast<unsigned long long>(firing.effectHooks),static_cast<unsigned long long>(firing.resourceQueries),b(firing.tuningCached),firing.tuning.projectileClass);
        }
        std::fputs("]}",file);comma=true;}
    std::fprintf(file,"],\"observation_drops\":%llu,\"rays\":[",static_cast<unsigned long long>(observationDrops));
    const uint64_t first=observationWrite>64?observationWrite-64:0;
    for(uint64_t i=first;i<observationWrite;++i){const auto& r=observations[i%observations.size()];const auto& s=r.shot;
        std::fprintf(file,"%s{\"source\":%u,\"spawn_epoch\":%u,\"binding\":%llu,\"native_origin\":%llu,\"projectile\":%u,\"firing_slot\":%u,\"firing_component\":\"%p\",\"resource\":\"%p\",\"stability\":\"%p\",\"projectile_class\":\"%p\",\"native_shot_index\":%d,\"damage_token\":%llu,\"observed_at\":%.17g,\"ammo\":%d,\"input_aim_valid\":%s,\"start\":",
            i==first?"":",",r.identity.source,r.identity.spawnEpoch,static_cast<unsigned long long>(r.identity.binding),static_cast<unsigned long long>(r.identity.nativeOrigin),r.identity.projectile,r.identity.firingSlot,r.firing,r.resource,r.stability,r.projectileClass,s.native_shot_index,static_cast<unsigned long long>(r.damageToken),s.observed_at,s.ammo,
            b(r.inputAimValid));
        JsonVector(file,s.start);std::fputs(",\"direction\":",file);JsonVector(file,s.direction);
        std::fputs(",\"original_input_aim\":",file);JsonVector(file,r.originalInputAim);
        std::fputs(",\"original_control_rotation\":",file);JsonVector(file,r.originalControlRotation);std::fputc('}',file);
    }
    std::fputs("],\"shot_direction_source\":\"original_FiringEffect\",\"shot_origin_source\":\"exact_original_FireShot_native_view_call_chain\",\"firing_identity_source\":\"matching_RSI_from_Windows_unwind_of_original_71227dd_and_711f0ae_frames\",\"all_original_firing_components_observed\":true,\"direct_aim_write\":false,\"replacement_spread_recoil\":false,\"camera_damage_rays\":false,\"image_vtable_writes\":false,\"score_commit_owner\":\"deathmatch_rules\"}\n",file);
    const bool okay=std::ferror(file)==0;std::fclose(file);return okay;
}
}
