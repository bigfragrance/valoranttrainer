#pragma once
#include "local_server_combat.h"
#include "all_weapons.h"
#include "humanlike_bot_weapon_feedback_data.h"
#include "humanlike_bot_weapon_feedback_profiles.h"
#include "humanlike_bot_native_events.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

// Own-NPC observation only. The existing native input, state machines, clock,
// firing, recoil, spread and damage producers retain exclusive ownership.
namespace humanlike_bot_weapon_feedback {
namespace combat=local_server_combat;
namespace firearms=local_server_combat::firearms;
using Weak=original_inventory_slots::WeakIdentity;
inline constexpr size_t MaxPlayers=local_server_match::MaxPlayers;
inline constexpr double DiagnosticsSeconds=.5;
struct NativeStabilityValues {float slot0=0,firing=0,movement=0,pitch=0,yaw=0,slot20=0;};
static_assert(sizeof(NativeStabilityValues)==24&&offsetof(NativeStabilityValues,firing)==4
    &&offsetof(NativeStabilityValues,movement)==8&&offsetof(NativeStabilityValues,pitch)==12&&offsetof(NativeStabilityValues,yaw)==16);
enum class MethodIndex:size_t {Ammo,MaxAmmo,Reserve,State,Trigger,CanFire,Cooldown,MovementError,FiringError,Pitch,Yaw,Velocity,Automatic,Burst,InputDown,Count};
struct Method {
    const wchar_t* path;uintptr_t exec,owner;uint16_t bytes;uint32_t flags;
    void* function=nullptr;Weak identity{};bool valid=false;
};
inline std::array<Method,static_cast<size_t>(MethodIndex::Count)> methods{{
    {L"/Script/ShooterGame.AmmoComponent.GetAmmo",0x637fea0,0x63c7e00,4,0x54020401},
    {L"/Script/ShooterGame.AmmoComponent.GetMaxAmmo",0x63c84c0,0x63c7e00,4,0x54020401},
    {L"/Script/ShooterGame.AmmoComponent.GetReserveComponent",0x37e4230,0x63c7e00,8,0x54020401},
    {L"/Script/ShooterGame.StateMachineComponent.GetCurrentState",0x6d25830,0x63ab7f0,8,0x54020401},
    {L"/Script/ShooterGame.FiringStateComponent.IsTriggerInput",0x68506a0,0x684d4c0,3,0x54020401},
    {L"/Script/ShooterGame.FiringStateComponent.CanFire",0x6850340,0x684d4c0,2,0x54020401},
    {L"/Script/ShooterGame.FiringStateComponent.GetCooldownTimeRemaining",0x6850880,0x684d4c0,4,0x54020401},
    {L"/Script/ShooterGame.StabilityComponent.GetMovementError",0x6d20170,0x6d1e460,4,0x54020401},
    {L"/Script/ShooterGame.StabilityComponent.GetFiringError",0x6d201a0,0x6d1e460,4,0x54020401},
    {L"/Script/ShooterGame.StabilityComponent.GetPitchRecoil",0x6d20320,0x6d1e460,4,0x54020401},
    {L"/Script/ShooterGame.StabilityComponent.GetYawRecoil",0x6d20280,0x6d1e460,4,0x54020401},
    {L"/Script/Engine.Actor.GetVelocity",0x3ef13b0,0x3683900,24,0x54820402},
    {L"/Script/ShooterGame.FiringStateComponent.IsAutomatic",0x684fbb0,0x684d4c0,1,0x54020401},
    {L"/Script/ShooterGame.FiringStateComponent.IsBurstFire",0x63a1770,0x684d4c0,1,0x54020401},
    {L"/Script/ShooterGame.AresEquippable.IsInputDown",0x64634f0,0x645c660,2,0x54020401}
}};
struct Scope {
    size_t index=MaxPlayers;uint32_t epoch=0;uint64_t binding=0;
    void *world=nullptr,*pawn=nullptr,*pc=nullptr,*ps=nullptr,*weapon=nullptr,*inventory=nullptr,*input=nullptr;
    Weak worldId{},pawnId{},pcId{},psId{},weaponId{},inventoryId{},inputId{};
};
struct HolderEvidence {
    void *pawn=nullptr,*abilities=nullptr,*attributes=nullptr;
    Weak abilitiesId{},attributesId{};
    float minimumErrorDegrees=0,fireRateModifier=0;bool known=false;
};
struct ContextEvidence {
    Candidate candidate{};
    void *firing=nullptr,*resource=nullptr,*stability=nullptr,*machine=nullptr,*nativeState=nullptr;
    void *resourceOverride=nullptr,*stabilityOverride=nullptr;
    Weak firingId{},resourceId{},stabilityId{},machineId{},stateId{};
    bool live=false,machineKnown=false,reloadObserved=false,cooldownKnown=false;
    float cooldown=0;int32_t nativeShotIndex=-1;uint8_t primaryActionMask=0;
    bool refireSampled=false,refireIntervalKnown=false,burstTuningObserved=false;
    void* refireOwner=nullptr;float nativeFiringRate=0,nativeRefireInterval=0,nativeBurstEndWait=0;
    int32_t nativeBurstCount=0;uint8_t nativeBurstFlag=255;
};
struct Evidence {
    bool attempted=false,stable=false,busy=false,velocityKnown=false,movementMatches=false,clockTupleValid=false;
    uint64_t samples=0,faults=0;uintptr_t exceptionRva=0;
    Scope scope{};double sampledAt=-1;size_t count=0;Selection selection{};
    std::array<ContextEvidence,MaxContexts> contexts{};
    void *movement=nullptr,*prediction=nullptr,*clock=nullptr,*stabilityMovement=nullptr,*reserve=nullptr;
    Weak movementId{},reserveId{};float nativeClockTime=0;
    engine::Vec velocity{};NativeStabilityValues nativeStability{};HolderEvidence holder{};
    uint8_t nativePrimaryHeld=255;int32_t nativeAppliedInput=-1;
    bool latestShotKnown=false;uint64_t shotSerial=0,shotToken=0,shotOrigin=0;uint32_t shotFiringSlot=0;double shotAt=-1;
    humanlike_bot::WeaponReadback readback{};
    const char* stage="own_weapon_not_sampled";
};
struct Diagnostics {
    bool profilesChecked=false,profilesVerified=false;
    uint64_t samples=0,rejected=0,sampleFaults=0,nativeQueries=0,aggregateQueries=0,weakCaptureCalls=0;
    double lastDiagnosticsAt=-1;const char* stage="own_weapon_not_sampled";
};
inline Diagnostics state;
inline std::array<Evidence,MaxPlayers> entries{};
inline SRWLOCK jsonLock=SRWLOCK_INIT;
inline std::string jsonSnapshot="{\"stage\":\"own_weapon_not_sampled\",\"runtime_acceptance_verified\":false}";
inline const Diagnostics& GetState(){return state;}
inline const Evidence* Sample(size_t index){return index<entries.size()?&entries[index]:nullptr;}
template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Capture(void* object,Weak& identity){++state.weakCaptureCalls;return original_inventory_slots::CaptureIdentity(object,identity)&&Live(object,identity);}
inline bool GameThread(){
    return engine::image&&original_players::gameThread&&GetCurrentThreadId()==original_players::gameThread
        &&!original_players::restartRequired&&combat::state.initialized&&!combat::state.restartRequired
        &&GetCurrentThreadId()==combat::gameThread&&GetCurrentThreadId()==firearms::gameThread;
}
inline bool Profiles(){
    if(state.profilesChecked)return state.profilesVerified;state.profilesChecked=true;
    for(const auto& p:NativeProfiles){
        const auto address=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(address,p.bytes))return false;
        uint64_t hash=14695981039346656037ULL;
        for(size_t n=0;n<p.bytes;++n)hash=(hash^address[n])*1099511628211ULL;
        if(hash!=p.fnv)return false;
    }
    state.profilesVerified=true;return true;
}
inline bool MethodValid(const Method& m){
    return Live(m.function,m.identity)&&Read<uint16_t>(m.function,0xbe)==m.bytes&&Read<uint32_t>(m.function,0xb8)==m.flags
        &&Read<uintptr_t>(m.function,0xe0)==engine::image+m.exec&&Read<void*>(m.function,0x28)==engine::Class(m.owner);
}
inline bool PrepareMethods(){
    bool complete=true;
    for(auto& m:methods){
        if(!Live(m.function,m.identity)){m.function=engine::Find(m.path);m.identity={};if(m.function&&!Capture(m.function,m.identity))m.function=nullptr;}
        m.valid=MethodValid(m);complete=complete&&m.valid;
    }
    return complete;
}
inline Method& GetMethod(MethodIndex index){return methods[static_cast<size_t>(index)];}
template<class Parameters> inline bool Query(void* object,MethodIndex index,Parameters& parameters){
    auto& method=GetMethod(index);
    if(!object||sizeof(parameters)!=method.bytes||!method.valid||!MethodValid(method))return false;
    original_damage::Name name;std::memcpy(name.bytes,static_cast<uint8_t*>(method.function)+0x18,sizeof(name));
    if(engine::function<void*(*)(void*,const original_damage::Name*)>(0x1cdc710)(object,&name)!=method.function)return false;
    auto process=Read<void(*)(void*,void*,void*)>(Read<void**>(object),0x298);if(!process)return false;
    ++state.nativeQueries;process(object,method.function,&parameters);return MethodValid(method);
}
inline bool CurrentSource(size_t index,void* pawn,uint32_t epoch){
    if(index==0||index>=MaxPlayers||!pawn||!epoch||!local_server_match::Active()||!local_server_match::FreeForAll())return false;
    const auto p=local_server_match::Player(index);const auto& s=firearms::sources[index];
    return p&&!p->human&&p->alive&&p->ready&&p->equipmentReady&&p->phase==local_server_match::Phase::Alive
        &&p->pawn==pawn&&p->epoch==epoch&&combat::CurrentRegistration(index)&&firearms::SourceIdentity(s)
        &&s.index==index&&s.spawnEpoch==epoch&&s.pawn==pawn&&s.controller==p->pc&&s.alive&&s.current&&s.physicsReady
        &&s.inputReady&&s.externalInputProducer&&firearms::OwnedExternalInput(s)
        &&Read<void*>(pawn,0xba0)==s.inputState
        &&Read<void*>(s.inventory,0x280)==s.weapon&&Read<void*>(s.weapon,0x210)==pawn
        &&combat::state.world==local_server_match::state.world&&firearms::world==combat::state.world
        &&original_effects::GetWorld(pawn)==combat::state.world&&original_effects::GetWorld(s.controller)==combat::state.world
        &&original_effects::GetWorld(s.weapon)==combat::state.world;
}
inline bool ScopeMatches(const Scope& v){
    if(!CurrentSource(v.index,v.pawn,v.epoch))return false;
    const auto p=local_server_match::Player(v.index);const auto& s=firearms::sources[v.index];
    return v.binding==s.binding&&v.pc==s.controller&&v.ps==p->ps&&v.weapon==s.weapon&&v.inventory==s.inventory&&v.input==s.inputState
        &&v.world==combat::state.world&&Live(v.world,v.worldId)&&Live(v.pawn,v.pawnId)&&Live(v.pc,v.pcId)&&Live(v.ps,v.psId)
        &&Live(v.weapon,v.weaponId)&&Live(v.inventory,v.inventoryId)&&Live(v.input,v.inputId)
        &&original_inventory_slots::SameIdentity(v.pawnId,s.pawnId)&&original_inventory_slots::SameIdentity(v.weaponId,s.weaponId);
}
inline bool TakeScope(size_t index,Scope& v){
    const auto p=local_server_match::Player(index);const auto& s=firearms::sources[index];
    v={};v.index=index;v.epoch=s.spawnEpoch;v.binding=s.binding;v.world=combat::state.world;
    v.pawn=s.pawn;v.pc=s.controller;v.ps=p->ps;v.weapon=s.weapon;v.inventory=s.inventory;v.input=s.inputState;
    v.pawnId=s.pawnId;v.pcId=s.controllerId;v.psId=p->psIdentity;v.weaponId=s.weaponId;v.inventoryId=s.inventoryId;v.inputId=s.inputStateId;
    return Capture(v.world,v.worldId)&&ScopeMatches(v);
}
inline bool TakeMovement(const firearms::Source& s,Evidence& e){
    e.movement=s.externalMovement;e.movementId=s.externalMovementId;e.prediction=s.externalPrediction;e.clock=s.externalClock;
    if(!Live(e.movement,e.movementId)||Read<void*>(s.pawn,0x4f8)!=e.movement||Read<void*>(e.movement,0xc8)!=s.pawn
       ||Read<void*>(e.movement,0x190)!=s.pawn||Read<void*>(e.movement,0x1a8)!=s.pawn
       ||!e.prediction||Read<void*>(e.movement,0x7f0)!=e.prediction||e.clock!=static_cast<uint8_t*>(e.prediction)+0xf0
       ||Read<uintptr_t>(e.clock)!=engine::image+0xb600a60)return false;
    const auto vbtable=Read<void*>(e.clock,0x10);const auto displacement=Read<int32_t>(vbtable,4);
    if(!vbtable||displacement<0||displacement>0x1000||Read<uint32_t>(e.clock,static_cast<size_t>(displacement)+0x1c)!=1)return false;
    e.nativeClockTime=Read<float>(e.clock,0x20);if(!std::isfinite(e.nativeClockTime))return false;
    e.clockTupleValid=true;
    // The original helper can return null/defaults unless the actual current
    // Pawn exposes its real ShooterCharacterMovement through its original getter.
    e.movementMatches=engine::function<void*(*)(void*)>(0x787b6c0)(s.pawn)==e.movement;
    if(e.movementMatches){engine::Vec velocity{};
        if(Query(s.pawn,MethodIndex::Velocity,velocity)&&original_players::Finite(velocity)){e.velocity=velocity;e.velocityKnown=true;}}
    return e.movementMatches;
}
inline bool SameNativeFloat(float left,float right){
    uint32_t a=0,b=0;std::memcpy(&a,&left,sizeof(a));std::memcpy(&b,&right,sizeof(b));return a==b;
}
inline bool HolderLive(const Evidence& e){
    const auto& h=e.holder;
    return h.known&&h.pawn==e.scope.pawn&&Live(e.movement,e.movementId)
        &&Read<void*>(e.movement,0x190)==h.pawn&&Read<void*>(e.movement,0x1a8)==h.pawn
        &&engine::function<void*(*)(void*)>(0x7897410)(e.movement)==h.pawn
        &&Live(h.abilities,h.abilitiesId)&&Live(h.attributes,h.attributesId)
        &&Read<void*>(h.pawn,0xa20)==h.abilities&&Read<void*>(h.abilities,0xc8)==h.pawn
        &&Read<void*>(h.abilities,0x13a8)==h.attributes
        &&SameNativeFloat(Read<float>(h.attributes,0x2f4),h.minimumErrorDegrees)
        &&SameNativeFloat(Read<float>(h.attributes,0x104),h.fireRateModifier);
}
inline bool TakeHolder(Evidence& e){
    auto& h=e.holder;h={};
    if(!e.movementMatches||!ScopeMatches(e.scope))return false;
    h.pawn=engine::function<void*(*)(void*)>(0x7897410)(e.movement);
    if(h.pawn!=e.scope.pawn)return false;
    h.abilities=Read<void*>(h.pawn,0xa20);h.attributes=Read<void*>(h.abilities,0x13a8);
    if(!original_players::IsA(h.abilities,0x63d6b50)||Read<void*>(h.abilities,0xc8)!=h.pawn
       ||!original_players::IsA(h.attributes,0x63e58b0)
       ||!Capture(h.abilities,h.abilitiesId)||!Capture(h.attributes,h.attributesId))return false;
    if(!original_players::Read(h.attributes,0x2f4,h.minimumErrorDegrees)
       ||!original_players::Read(h.attributes,0x104,h.fireRateModifier)
       ||!std::isfinite(h.minimumErrorDegrees))return false;
    h.known=true;return HolderLive(e);
}
inline bool ContextLive(const Scope& scope,const ContextEvidence& c){
    return Live(c.firing,c.firingId)&&Read<void*>(c.firing,0xc8)==scope.weapon&&Read<void*>(c.firing,0x4c8)==scope.pawn
        &&Live(c.resource,c.resourceId)&&Read<void*>(c.resource,0xc8)==scope.weapon
        &&Live(c.stability,c.stabilityId)&&Read<void*>(c.stability,0xc8)==scope.weapon
        &&Read<void*>(c.firing,0x498)==c.resourceOverride&&Read<void*>(c.firing,0x490)==c.stabilityOverride;
}
inline void ObserveLatestShot(Evidence& e){
    namespace events=humanlike_bot_native_events;
    e.latestShotKnown=false;e.shotSerial=e.shotToken=e.shotOrigin=0;e.shotFiringSlot=0;e.shotAt=-1;
    if(!events::diagnostics.installed)return;const auto event=events::LatestShot(e.scope.index);
    if(!event||!event->actualGeometryPaired||event->subject.index!=e.scope.index||event->subject.epoch!=e.scope.epoch
       ||event->subject.pawn!=reinterpret_cast<uintptr_t>(e.scope.pawn)||event->subject.weakIndex!=e.scope.pawnId.index
       ||event->subject.weakSerial!=e.scope.pawnId.serial||event->binding!=e.scope.binding||!event->origin
       ||event->firingSlot>=e.count)return;
    e.latestShotKnown=true;e.shotSerial=event->serial;e.shotToken=event->token;e.shotOrigin=event->origin;
    e.shotFiringSlot=event->firingSlot;e.shotAt=event->atMatchSeconds;
}
inline bool ReadImpl(size_t index,void* pawn,uint32_t epoch,double now,humanlike_bot::WeaponReadback& out){
    if(!std::isfinite(now)||now<0||!CurrentSource(index,pawn,epoch)){
        ++state.rejected;auto& e=entries[index];e.readback={};e.stable=false;e.latestShotKnown=false;e.stage="own_weapon_current_source_unavailable";state.stage=e.stage;return true;
    }
    auto& old=entries[index];
    const auto samples=old.samples+1,faults=old.faults;const bool busy=old.busy;
    old={};auto& e=old;e.busy=busy;e.attempted=true;e.samples=samples;e.faults=faults;e.sampledAt=now;++state.samples;
    const auto pending=[&](const char* reasonText){e.readback={};e.stage=reasonText;state.stage=reasonText;return true;};
    if(!TakeScope(index,e.scope))return pending("own_weapon_current_source_scope_unavailable");
    if(!Profiles())return pending("own_weapon_fixed_native_profiles_unverified");
    PrepareMethods();
    const auto& s=firearms::sources[index];NativeValues values{};values.sourceValid=true;values.binding=s.binding;values.sampledAtSeconds=now;
    if(const auto definition=all_weapons::FindByClass(Read<void*>(s.weapon,0x10))){
        const auto position=definition-all_weapons::definitions;
        if(definition->identityVerified&&position>=0&&position<21&&all_weapons::Count==21
           &&definition->runtimeClass==Read<void*>(s.weapon,0x10)&&definition->dataAsset==Read<void*>(s.weapon,0xfa8)){
            values.catalogWeaponIndex=static_cast<uint16_t>(position);values.kind=Kind(definition->categoryValue);
        }
    }
    const bool movement=TakeMovement(s,e);if(movement&&s.ballistic)TakeHolder(e);
    struct InputDown {uint8_t input=0,result=0;} inputDown;static_assert(sizeof(InputDown)==2);
    e.nativePrimaryHeld=Read<uint8_t>(s.inputState,0x265);e.nativeAppliedInput=Read<int32_t>(s.inputState,0x120c);
    values.triggerHeldKnown=Query(s.weapon,MethodIndex::InputDown,inputDown)&&inputDown.result<=1&&e.nativePrimaryHeld<=1
        &&(inputDown.result!=0)==(e.nativePrimaryHeld==1);
    values.triggerHeld=inputDown.result==1;
    if(s.ballistic){
        if(!s.firingCount||s.firingCount>MaxContexts)return pending("own_weapon_firing_context_count_unavailable");
        e.count=s.firingCount;std::array<Candidate,MaxContexts> candidates{};
        for(size_t n=0;n<e.count;++n){const auto& f=s.firings[n];auto& c=e.contexts[n];
            c.firing=f.component;c.firingId=f.identity;c.resource=f.resource;c.resourceId=f.resourceIdentity;c.stability=f.stability;c.stabilityId=f.stabilityIdentity;
            c.resourceOverride=f.resourceOverride;c.stabilityOverride=f.stabilityOverride;
            c.candidate.valid=Live(c.firing,c.firingId)&&Read<void*>(c.firing,0xc8)==e.scope.weapon&&Read<void*>(c.firing,0x4c8)==pawn;
            if(!c.candidate.valid){candidates[n]=c.candidate;continue;}
            c.live=ContextLive(e.scope,c);
            if(c.live){c.candidate.resource=reinterpret_cast<uintptr_t>(c.resource);c.candidate.stability=reinterpret_cast<uintptr_t>(c.stability);}
            c.nativeShotIndex=Read<int32_t>(c.firing,0x4a8);
            struct Trigger {uint8_t input=0,action=0,result=0;};static_assert(sizeof(Trigger)==3);
            c.candidate.primaryKnown=true;
            for(uint8_t action=0;action<4;++action){Trigger trigger{0,action,0};
                if(!Query(c.firing,MethodIndex::Trigger,trigger)||trigger.result>1)c.candidate.primaryKnown=false;
                else if(trigger.result)c.primaryActionMask|=static_cast<uint8_t>(1U<<action);
            }
            c.candidate.primary=c.primaryActionMask!=0;
            uint8_t automatic=0,burst=0;
            c.candidate.behaviorKnown=Query(c.firing,MethodIndex::Automatic,automatic)&&automatic<=1
                &&Query(c.firing,MethodIndex::Burst,burst)&&burst<=1;
            c.candidate.automatic=automatic==1;c.candidate.nativeBurst=burst==1;
            c.machine=Read<void*>(c.firing,0xe0);
            if(original_players::IsA(c.machine,0x63ab7f0)&&Read<void*>(c.machine,0xc8)==s.weapon&&Capture(c.machine,c.machineId)
               &&Query(c.machine,MethodIndex::State,c.nativeState)&&Capture(c.nativeState,c.stateId)
               &&Read<void*>(c.nativeState,0xc8)==s.weapon&&Read<void*>(c.nativeState,0xe0)==c.machine){
                c.machineKnown=true;c.candidate.activeKnown=true;c.candidate.active=c.nativeState==c.firing;
                c.reloadObserved=original_players::IsA(c.nativeState,0x6b648b0);values.reloadObserved=values.reloadObserved||c.reloadObserved;
            }
            if(movement&&c.machineKnown){struct CanFire {uint8_t cooldownCheckOption=0,result=0;} query;static_assert(sizeof(CanFire)==2);
                c.candidate.canFireKnown=Query(c.firing,MethodIndex::CanFire,query)&&query.result<=1;c.candidate.canFire=query.result==1;
                c.cooldownKnown=Query(c.firing,MethodIndex::Cooldown,c.cooldown)&&std::isfinite(c.cooldown)&&c.cooldown>=0;
            }
            candidates[n]=c.candidate;
        }
        e.selection=SelectPrimary(candidates,e.count);
        if(e.selection.valid){const auto& selected=e.contexts[e.selection.slot];
            values.canFireKnown=e.selection.canFireKnown;values.canFire=e.selection.canFire;
            values.behaviorKnown=e.selection.behaviorKnown;values.automatic=e.selection.automatic;values.nativeBurst=e.selection.nativeBurst;
            if(ContextLive(e.scope,selected)&&original_players::IsA(selected.resource,0x63c7e00)){
                values.ammoQueried=Query(selected.resource,MethodIndex::Ammo,values.magazine)&&Query(selected.resource,MethodIndex::MaxAmmo,values.maximum);
                if(Query(selected.resource,MethodIndex::Reserve,e.reserve)&&e.reserve&&original_players::IsA(e.reserve,0x63c7e00)
                   &&Read<void*>(e.reserve,0xc8)==s.weapon&&Capture(e.reserve,e.reserveId))
                    values.reserveQueried=Query(e.reserve,MethodIndex::Ammo,values.reserve);
            }
            const bool stabilityMethods=GetMethod(MethodIndex::MovementError).valid&&GetMethod(MethodIndex::FiringError).valid
                &&GetMethod(MethodIndex::Pitch).valid&&GetMethod(MethodIndex::Yaw).valid;
            if(movement&&HolderLive(e)&&ContextLive(e.scope,selected)&&stabilityMethods){
                e.stabilityMovement=engine::function<void*(*)(void*)>(0x7000190)(selected.stability);
                values.stabilityMovementMatches=e.stabilityMovement==e.movement;values.stabilityHolderKnown=HolderLive(e);
                if(values.stabilityMovementMatches&&values.stabilityHolderKnown){
                    auto returned=engine::function<NativeStabilityValues*(*)(void*,NativeStabilityValues*)>(0x6fff5e0)(selected.stability,&e.nativeStability);
                    ++state.aggregateQueries;values.stabilityQueried=returned==&e.nativeStability;
                    values.firingError=e.nativeStability.firing;values.movementError=e.nativeStability.movement;
                    values.pitch=e.nativeStability.pitch;values.yaw=e.nativeStability.yaw;
                }
            }
            if(NativeCadenceContextProven(e.selection)&&HolderLive(e)&&ContextLive(e.scope,selected)){
                auto& c=e.contexts[e.selection.slot];
                c.refireOwner=engine::function<void*(*)(void*)>(0x7115010)(c.firing);
                if(c.refireOwner==e.scope.pawn){
                    c.nativeFiringRate=Read<float>(c.firing,0x44c);
                    c.nativeBurstFlag=Read<uint8_t>(c.firing,0x578);
                    c.nativeBurstCount=Read<int32_t>(c.firing,0x57c);
                    c.nativeBurstEndWait=Read<float>(c.firing,0x580);
                    c.nativeRefireInterval=engine::function<float(*)(void*)>(0x711ead0)(c.firing);c.refireSampled=true;
                    c.refireIntervalKnown=NativeRefireIntervalValid(c.nativeFiringRate,e.holder.fireRateModifier,c.nativeRefireInterval);
                    c.burstTuningObserved=c.candidate.behaviorKnown&&c.candidate.nativeBurst&&c.nativeBurstFlag<=1
                        &&(c.nativeBurstFlag!=0)==c.candidate.nativeBurst&&c.nativeBurstCount>0
                        &&c.nativeBurstCount<=65535&&std::isfinite(c.nativeBurstEndWait)&&c.nativeBurstEndWait>=0;
                    // Reading authored tuning does not prove an entire original
                    // burst cycle or that input release can interrupt that cycle.
                    // Keep the DTO nativeBurstSizeKnown false until that proof.
                }
            }
        }
    }
    e.stable=ScopeMatches(e.scope);
    for(size_t n=0;n<e.count&&e.stable;++n){const auto& c=e.contexts[n];
        if(c.live)e.stable=ContextLive(e.scope,c);
        if(c.refireSampled)e.stable=e.stable&&HolderLive(e)
            &&engine::function<void*(*)(void*)>(0x7115010)(c.firing)==c.refireOwner
            &&SameNativeFloat(Read<float>(c.firing,0x44c),c.nativeFiringRate)
            &&Read<uint8_t>(c.firing,0x578)==c.nativeBurstFlag
            &&Read<int32_t>(c.firing,0x57c)==c.nativeBurstCount
            &&SameNativeFloat(Read<float>(c.firing,0x580),c.nativeBurstEndWait);
        if(c.machineKnown){void* selected=nullptr;e.stable=e.stable&&Live(c.machine,c.machineId)&&Live(c.nativeState,c.stateId)
            &&Query(c.machine,MethodIndex::State,selected)&&selected==c.nativeState;}
    }
    if(e.velocityKnown||values.stabilityQueried)e.stable=e.stable&&Live(e.movement,e.movementId)
        &&Read<void*>(pawn,0x4f8)==e.movement&&Read<void*>(e.movement,0x7f0)==e.prediction&&Read<float>(e.clock,0x20)==e.nativeClockTime;
    if(values.stabilityQueried)e.stable=e.stable&&HolderLive(e)&&engine::function<void*(*)(void*)>(0x7000190)(e.contexts[e.selection.slot].stability)==e.stabilityMovement;
    if(values.reserveQueried)e.stable=e.stable&&Live(e.reserve,e.reserveId)&&Read<void*>(e.reserve,0xc8)==e.scope.weapon
        &&Read<void*>(e.contexts[e.selection.slot].resource,0x138)==e.reserve;
    if(values.triggerHeldKnown)e.stable=e.stable&&Read<uint8_t>(e.scope.input,0x265)==e.nativePrimaryHeld
        &&Read<int32_t>(e.scope.input,0x120c)==e.nativeAppliedInput;
    if(!e.stable)return pending("own_weapon_native_getters_changed_current_source_or_state");
    values.stable=true;e.readback=MakeReadback(values);ObserveLatestShot(e);out=e.readback;
    if(e.selection.valid&&s.ballistic&&!e.holder.known){e.stage="own_weapon_holder_dependencies_unknown";state.stage=e.stage;return true;}
    e.stage=e.selection.valid?"own_weapon_original_primary_state_readback":"own_weapon_primary_context_or_recovery_unknown";state.stage=e.stage;return true;
}
inline bool GuardedRead(size_t index,void* pawn,uint32_t epoch,double now,humanlike_bot::WeaponReadback& out){
    if(entries[index].busy){++state.rejected;return true;}
    entries[index].busy=true;
    __try{return ReadImpl(index,pawn,epoch,now,out);}__finally{entries[index].busy=false;}
}
inline LONG ObservationFault(size_t index,EXCEPTION_POINTERS* info){
    ++state.sampleFaults;auto& e=entries[index];++e.faults;e.stable=false;e.latestShotKnown=false;e.readback={};
    const auto at=info&&info->ExceptionRecord?reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress):0;
    e.exceptionRva=at>=engine::image?at-engine::image:at;e.stage="own_weapon_invalid_diagnostic_sample_exception";state.stage=e.stage;
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool ReadWeaponFeedback(size_t index,void* pawn,uint32_t epoch,double now,humanlike_bot::WeaponReadback& out){
    out={};if(!GameThread()||index>=MaxPlayers||combat::state.processing||local_server_match::pending.active||firearms::observerDepth)return true;
    __try{return GuardedRead(index,pawn,epoch,now,out);}__except(ObservationFault(index,GetExceptionInformation())){out={};return true;}
}
inline bool RetireOwnLife(size_t index,void* pawn,uint32_t epoch){
    if(!GameThread()||index>=MaxPlayers)return false;auto& e=entries[index];
    if(e.scope.pawn==pawn&&e.scope.epoch==epoch){e.stable=false;e.readback={};e.stage="own_weapon_life_retired";}return true;
}
inline std::string Number(double value){if(!std::isfinite(value))return "null";char text[64]{};std::snprintf(text,sizeof(text),"%.17g",value);return text;}
inline std::string FormatDiagnostics(const Diagnostics& status,const std::array<Evidence,MaxPlayers>& rows){
    const auto b=[](bool v){return v?"true":"false";};std::string out;char text[4096]{};
    std::snprintf(text,sizeof(text),"{\"stage\":\"%s\",\"profiles_verified\":%s,\"samples\":%llu,\"sample_faults\":%llu,\"native_queries\":%llu,\"aggregate_queries\":%llu,\"weak_capture_calls\":%llu,\"players\":[",
        status.stage,b(status.profilesVerified),static_cast<unsigned long long>(status.samples),static_cast<unsigned long long>(status.sampleFaults),
        static_cast<unsigned long long>(status.nativeQueries),static_cast<unsigned long long>(status.aggregateQueries),static_cast<unsigned long long>(status.weakCaptureCalls));out=text;
    bool comma=false;for(const auto& e:rows)if(e.attempted){const auto& w=e.readback;
        std::snprintf(text,sizeof(text),"%s{\"index\":%zu,\"epoch\":%u,\"binding\":%llu,\"stage\":\"%s\",\"sampled_at_match_seconds\":%s,\"stable\":%s,\"pawn_weak\":[%d,%d],\"gun_weak\":[%d,%d],\"valid\":%s,\"catalog_index\":%u,\"kind\":%u,\"primary_selection\":%u,\"selected_slot\":%zu,\"primary_count\":%zu,\"active_primary_count\":%zu,\"ammo_known\":%s,\"magazine\":%d,\"magazine_maximum\":%d,\"reserve_known\":%s,\"reserve\":%d,\"can_fire_known\":%s,\"native_can_fire_option0\":%s,\"reload_known\":%s,\"reloading\":%s,\"recovery_known\":%s,\"recoil_known\":%s,\"firing_error_degrees\":%s,\"movement_error_degrees\":%s,\"pitch_recoil_degrees\":%s,\"yaw_recoil_degrees\":%s,\"movement_matches\":%s,\"clock_tuple_valid\":%s,\"velocity_known\":%s,\"velocity_cm_s\":[%s,%s,%s],\"latest_actual_shot_known\":%s,\"shot_serial\":%llu,\"shot_token\":%llu,\"native_origin\":%llu,\"shot_firing_slot\":%u,\"shot_at_match_seconds\":%s,\"contexts\":[",
            comma?",":"",e.scope.index,e.scope.epoch,static_cast<unsigned long long>(e.scope.binding),e.stage,Number(e.sampledAt).c_str(),b(e.stable),e.scope.pawnId.index,e.scope.pawnId.serial,e.scope.weaponId.index,e.scope.weaponId.serial,
            b(w.valid),static_cast<unsigned>(w.catalogWeaponIndex),static_cast<unsigned>(w.kind),static_cast<unsigned>(e.selection.reason),e.selection.slot,e.selection.primaryCount,e.selection.activeCount,
            b(w.ammoKnown),w.magazine,w.magazineMaximum,b(w.reserveKnown),w.reserve,b(w.canFireKnown),b(w.canFire),b(w.reloadKnown),b(w.reloading),b(w.recoveryKnown),b(w.recoilKnown),
            w.recoveryKnown?Number(w.firingErrorDegrees).c_str():"null",w.recoveryKnown?Number(w.movementErrorDegrees).c_str():"null",w.recoilKnown?Number(w.pitchRecoilDegrees).c_str():"null",w.recoilKnown?Number(w.yawRecoilDegrees).c_str():"null",
            b(e.movementMatches),b(e.clockTupleValid),b(e.velocityKnown),e.velocityKnown?Number(e.velocity.x).c_str():"null",e.velocityKnown?Number(e.velocity.y).c_str():"null",e.velocityKnown?Number(e.velocity.z).c_str():"null",
            b(e.latestShotKnown),static_cast<unsigned long long>(e.shotSerial),static_cast<unsigned long long>(e.shotToken),static_cast<unsigned long long>(e.shotOrigin),e.shotFiringSlot,Number(e.shotAt).c_str());
        out+=text;comma=true;
        // Only POD cached above; serialization never calls original getters.
        for(size_t n=0;n<e.count;++n){const auto& c=e.contexts[n];
            std::snprintf(text,sizeof(text),"%s{\"slot\":%zu,\"primary_known\":%s,\"primary\":%s,\"primary_action_mask\":%u,\"machine_known\":%s,\"active\":%s,\"current_state_weak\":[%d,%d],\"reload_observed\":%s,\"can_fire_known\":%s,\"can_fire\":%s,\"cooldown_known\":%s,\"cooldown_native\":%s,\"native_shot_index\":%d}",
                n?",":"",n,b(c.candidate.primaryKnown),b(c.candidate.primary),static_cast<unsigned>(c.primaryActionMask),b(c.machineKnown),b(c.candidate.active),c.stateId.index,c.stateId.serial,b(c.reloadObserved),b(c.candidate.canFireKnown),b(c.candidate.canFire),b(c.cooldownKnown),c.cooldownKnown?Number(c.cooldown).c_str():"null",c.nativeShotIndex);out+=text;
            if(!out.empty()&&out.back()=='}')out.pop_back();
            std::snprintf(text,sizeof(text),",\"native_refire_interval_known\":%s,\"native_refire_interval_seconds\":%s,\"native_firing_rate_scalar\":%s,\"native_burst_tuning_observed\":%s,\"native_burst_count_scalar\":%s,\"native_burst_end_wait_seconds\":%s,\"native_burst_cycle_proven\":false}",
                b(e.stable&&c.refireIntervalKnown),e.stable&&c.refireIntervalKnown?Number(c.nativeRefireInterval).c_str():"null",
                e.stable&&c.refireSampled&&std::isfinite(c.nativeFiringRate)?Number(c.nativeFiringRate).c_str():"null",
                b(e.stable&&c.burstTuningObserved),e.stable&&c.burstTuningObserved?std::to_string(c.nativeBurstCount).c_str():"null",
                e.stable&&c.burstTuningObserved?Number(c.nativeBurstEndWait).c_str():"null");out+=text;
        }
        std::snprintf(text,sizeof(text),"],\"fire_behavior_known\":%s,\"automatic\":%s,\"native_burst\":%s,\"native_burst_size_known\":%s,\"trigger_held_known\":%s,\"trigger_held\":%s,\"native_primary_held_byte\":%u,\"native_applied_input\":%d,\"native_clock_time\":%s}",
            b(w.fireBehaviorKnown),b(w.automatic),b(w.nativeBurst),b(w.nativeBurstSizeKnown),b(w.triggerHeldKnown),b(w.triggerHeld),static_cast<unsigned>(e.nativePrimaryHeld),e.nativeAppliedInput,e.clockTupleValid?Number(e.nativeClockTime).c_str():"null");out+=text;
        if(!out.empty()&&out.back()=='}')out.pop_back();
        std::snprintf(text,sizeof(text),",\"stability_holder_dependencies_known\":%s,\"abilities_weak\":[%d,%d],\"attributes_weak\":[%d,%d],\"native_min_error_degrees\":%s,\"native_attribute_fire_rate_modifier\":%s}",
            b(e.stable&&e.holder.known),e.holder.abilitiesId.index,e.holder.abilitiesId.serial,e.holder.attributesId.index,e.holder.attributesId.serial,
            e.stable&&e.holder.known?Number(e.holder.minimumErrorDegrees).c_str():"null",
            e.stable&&e.holder.known?Number(e.holder.fireRateModifier).c_str():"null");out+=text;
    }
    out+="],\"provider_owns_weapon_sample_cadence\":true,\"internal_weapon_sample_throttles\":0,\"borrowed_combat_dependency_cache_minimum_seconds\":0.04,\"diagnostics_interval_seconds\":0.5,\"shot_event_owner\":\"humanlike_bot_native_events\",\"actual_window_shots_owned_by_brain\":true,\"held_time_or_ammo_delta_used_as_shot_count\":false,\"native_weapon_parameter_writes\":0,\"new_native_observers\":0,\"native_physics_ticks\":0,\"velocity_used_as_weapon_recovery\":false,\"weak_serial_allocation_possible\":true,\"original_query_cache_may_refresh\":true,\"runtime_acceptance_verified\":false}";
    return out;
}
inline void CacheDiagnostics(double now){
    if(!GameThread()||!std::isfinite(now)||now<0||(state.lastDiagnosticsAt>=0&&now-state.lastDiagnosticsAt<DiagnosticsSeconds))return;
    state.lastDiagnosticsAt=now;auto encoded=FormatDiagnostics(state,entries);
    struct Lock {Lock(){AcquireSRWLockExclusive(&jsonLock);}~Lock(){ReleaseSRWLockExclusive(&jsonLock);}} lock;
    jsonSnapshot=std::move(encoded);
}
inline std::string Json(){struct Lock{Lock(){AcquireSRWLockShared(&jsonLock);}~Lock(){ReleaseSRWLockShared(&jsonLock);}} lock;return jsonSnapshot;}
} // namespace humanlike_bot_weapon_feedback
