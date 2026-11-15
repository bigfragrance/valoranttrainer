#pragma once
#include "deathmatch_rules.h"
#include "original_inventory_slots.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <intrin.h>

// Offline authority for an actual shipped melee contact. The original attack
// state, animation trigger, collision query, selected victim, bone, backstab,
// range, tuning, and impact event all run first. There is no synthetic ray or
// knife damage table here. Only an unchanged native life record may receive
// the missing local authority notification through original_damage.
// Evidence: reports/original-melee-combat.json; image SHA14776335...8fe92f.
namespace original_melee_combat {
inline constexpr size_t MaxPlayers=32,MaxStates=16,MaxVtableFunctions=512,QueueCapacity=256;
static_assert(deathmatch_rules::MaxPlayers==MaxPlayers&&original_damage::MaxTargets>=MaxPlayers);
inline constexpr uintptr_t AttackClass=0x699b9e0,OriginalAttackTick=0x7130ce0,OriginalEvent=0x1cdc850;
inline constexpr size_t AttackTickSlot=0x558/8,EventSlot=0x298/8;
inline constexpr const wchar_t* MeleeClassPath=L"/Game/Equippables/Melee/Ability_Melee_Base.Ability_Melee_Base_C";
inline constexpr const wchar_t* ImpactPath=L"/Script/ShooterGame.MeleeAttackStateComponent.MulticastHitImpact";
using Weak=original_inventory_slots::WeakIdentity;
// ScriptState's actual7143730 dispatcher passes XMM1 and XMM2, ignores the
// return, then broadcasts OnStateTick. Preserve both original float inputs.
using NativeAttackTick=void(*)(void*,float,float);
using NativeEvent=void(*)(void*,void*,void*);
struct Hooks {
    // Must share the firearm bridge's globally monotonic token allocator.
    uint64_t (*allocateDamageToken)()=nullptr;
    // Retire other observers on the victim after a verified native death.
    // This module separately restores its own melee component observers.
    bool (*beforeNativeDeath)(size_t index)=nullptr;
    // Optional original weapon-use policy (including a native swing/miss).
    // Called only from Poll, before any queued contact damage is processed.
    void (*weaponUsed)(size_t index)=nullptr;
};
struct Options {double maximumContactAge=1.0;size_t maximumContactsPerPoll=32;};
struct Vtable {
    void** previous=nullptr;size_t count=0;bool installed=false;
    std::array<void*,MaxVtableFunctions+1> entries{};
    void** Address(){return entries.data()+1;}
};
struct AttackState {
    void* object=nullptr;Weak identity{};Vtable table;
    NativeAttackTick forwardAttackTick=nullptr;NativeEvent forwardEvent=nullptr;
    uint64_t entries=0,contacts=0;
};
struct Source {
    bool bound=false;size_t index=0,stateCount=0;uint32_t spawnEpoch=0;uint64_t binding=0;
    void *pawn=nullptr,*weapon=nullptr;Weak pawnIdentity{},weaponIdentity{};
    std::array<AttackState,MaxStates> states{};
    uint64_t entries=0,contacts=0,damageEvents=0,kills=0;
};
struct LifeSnapshot {
    bool valid=false,alive=false;uint32_t epoch=0;void* pawn=nullptr;
    void *handler=nullptr,*health=nullptr,*shield=nullptr;
    float healthValue=0,shieldValue=0;int32_t eventIndex=0,pendingEvents=0;
};
struct Contact {
    bool present=false,nativeLifeUnchanged=false,backstab=false;
    uint32_t source=0,victim=0,sourceEpoch=0,victimEpoch=0;
    uint64_t binding=0,occurrence=0,token=0;double at=0;
    void *sourcePawn=nullptr,*victimPawn=nullptr,*weapon=nullptr,*state=nullptr,*damageType=nullptr;
    Weak sourceIdentity{},victimIdentity{},weaponIdentity{},stateIdentity{},damageTypeIdentity{};
    float attackDamage=0;
    original_shots::Shot nativeContact;
    original_damage::Hit nativeHit;
};
struct PendingWeaponUse {
    bool pending=false;uint32_t spawnEpoch=0;void* pawn=nullptr;Weak identity{};
};
struct AttackScope {
    Source* source=nullptr;AttackState* state=nullptr;uint64_t occurrence=0;
    std::array<LifeSnapshot,MaxPlayers> before{};
    uint32_t impactEvents=0;Contact contact;
};
struct Diagnostics {
    bool initialized=false,ready=false,restartRequired=false,processing=false;
    unsigned error=0,boundSources=0;
    uint64_t binds=0,retirements=0,restoredTables=0,foreignTables=0;
    uint64_t entries=0,impactCallbacks=0,exactAuthorityContacts=0,worldContacts=0;
    uint64_t invalidPayloads=0,ambiguousContacts=0,nonCoreContacts=0,missingVictimBindings=0;
    uint64_t nativeLifeChanges=0,queued=0,queueOverflow=0,processed=0,staleContacts=0;
    uint64_t policyRejected=0,pendingDamageDeferred=0,damageEvents=0,kills=0;
    uint64_t weaponUseTicks=0,weaponUseNotifications=0,staleWeaponUses=0;
    uint64_t lastToken=0,lastOccurrence=0;
    int32_t lastSource=-1,lastVictim=-1;float lastDamage=0;
    bool lastBackstab=false;uint8_t lastRegion=255;
    void *lastNativeHitActor=nullptr,*lastNativeHitComponent=nullptr,*lastAttackState=nullptr;
    const char* lastNativeComponentKind="none";
    uintptr_t exceptionRva=0;double lastPoll=-1;
    const char* stage="original_melee_not_initialized";
    const char* operation="none";
};
inline Hooks hooks;inline Options options;inline Diagnostics diagnostics;
inline DWORD gameThread=0;inline void *world=nullptr,*impactFunction=nullptr;
inline std::array<Source,MaxPlayers> sources{};
inline std::array<Contact,QueueCapacity> queue{};
inline std::array<PendingWeaponUse,MaxPlayers> pendingWeaponUses{};
inline uint64_t nextBinding=0,nextOccurrence=0,readIndex=0,writeIndex=0;
inline AttackScope* activeScope=nullptr;
inline const Diagnostics& State(){return diagnostics;}
inline const Source* Player(size_t index){return index<sources.size()?&sources[index]:nullptr;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Fail(unsigned code,const char* stage,bool terminal=false){
    diagnostics.error=code;diagnostics.stage=stage;diagnostics.ready=false;
    if(terminal)diagnostics.restartRequired=true;return false;
}
inline bool Thread(){return diagnostics.initialized&&!diagnostics.restartRequired&&GetCurrentThreadId()==gameThread;}
inline double Now(){return static_cast<double>(GetTickCount64())/1000.;}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Identity(void* object,Weak& identity){return original_inventory_slots::CaptureIdentity(object,identity)&&Live(object,identity);}
inline LONG Capture(EXCEPTION_POINTERS* e){
    const auto at=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    diagnostics.exceptionRva=at>=engine::image?at-engine::image:at;
    deathmatch_rules::CancelNativeDamage();
    Fail(e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0003100U,"original_melee_native_exception_requires_new_session",true);
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool IsMeleeWeapon(void* weapon){
    auto type=engine::Find(MeleeClassPath);
    return weapon&&type&&original_players::Derived(Read<void*>(weapon,0x10),type);
}
inline bool SourceIdentity(const Source& source){
    const auto p=deathmatch_rules::Player(source.index);
    return source.bound&&p&&p->registered&&p->pawn==source.pawn&&p->spawnEpoch==source.spawnEpoch
        &&Live(source.pawn,source.pawnIdentity)&&Live(source.weapon,source.weaponIdentity)
        &&Read<void*>(source.weapon,0x4a8)==source.pawn&&Read<uint8_t>(source.weapon,0x224)==3;
}
inline AttackState* FindState(void* object,Source** owner=nullptr){
    for(auto& source:sources)if(source.bound)for(size_t i=0;i<source.stateCount;++i)
        if(source.states[i].object==object){if(owner)*owner=&source;return &source.states[i];}
    return nullptr;
}
// Copy RTTI plus the complete original virtual table, stopping only at a real
// non-executable delimiter. The original actor/Pawn tables are not involved.
inline bool CopyVtable(void* object,Vtable& table){
    auto original=Read<void**>(object,0);if(!original)return false;
    MEMORY_BASIC_INFORMATION region{};uintptr_t begin=0,end=0;size_t count=0;
    for(;count<MaxVtableFunctions;++count){
        void* entry=nullptr;if(!original_players::Read(original,count*8,entry))return false;
        const auto address=reinterpret_cast<uintptr_t>(entry);if(!address)break;
        if(address<begin||address>=end){
            if(!VirtualQuery(entry,&region,sizeof(region))||region.State!=MEM_COMMIT||(region.Protect&(PAGE_GUARD|PAGE_NOACCESS)))break;
            const auto protection=region.Protect&0xffU;
            if(protection!=PAGE_EXECUTE&&protection!=PAGE_EXECUTE_READ&&protection!=PAGE_EXECUTE_READWRITE&&protection!=PAGE_EXECUTE_WRITECOPY)break;
            begin=reinterpret_cast<uintptr_t>(region.BaseAddress);end=begin+region.RegionSize;
        }
    }
    if(count<=0x598/8||count==MaxVtableFunctions||!original_players::Readable(original-1,(count+1)*8))return false;
    table.previous=original;table.count=count;std::memcpy(table.entries.data(),original-1,(count+1)*8);return true;
}
inline bool Restore(AttackState& state){
    if(!state.table.installed)return true;
    if(!Live(state.object,state.identity)){state.table.installed=false;return true;}
    auto current=Read<void**>(state.object,0);
    if(current==state.table.previous){state.table.installed=false;return true;}
    if(current!=state.table.Address()){++diagnostics.foreignTables;return Fail(3101,"original_melee_other_component_observer_must_retire_first");}
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(state.object),state.table.previous,state.table.Address())!=state.table.Address())
        return Fail(3102,"original_melee_component_restore_raced");
    state.table.installed=false;++diagnostics.restoredTables;return true;
}
inline bool BeforeRetireImpl(size_t index){
    if(!Thread()||index>=sources.size())return false;auto& source=sources[index];if(!source.bound)return true;
    if(activeScope&&activeScope->source==&source)return Fail(3103,"original_melee_cannot_retire_inside_native_attack");
    for(size_t i=0;i<source.stateCount;++i)if(!Restore(source.states[i]))return false;
    source.bound=false;++diagnostics.retirements;if(diagnostics.boundSources)--diagnostics.boundSources;return true;
}
inline bool BeforeRetire(size_t index){__try{return BeforeRetireImpl(index);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool ReadLife(size_t index,LifeSnapshot& snapshot){
    snapshot={};const auto p=deathmatch_rules::Player(index);const auto& target=original_damage::targets[index];
    if(!p||!p->registered||!p->ready||p->phase!=deathmatch_rules::Phase::Alive||!p->alive
       ||!target.bound||target.pawn!=p->pawn||!target.healthSection)return false;
    snapshot.pawn=p->pawn;snapshot.epoch=p->spawnEpoch;snapshot.handler=target.damageable;
    snapshot.health=target.healthSection;snapshot.shield=target.shieldSection;
    snapshot.healthValue=Read<float>(snapshot.health,0x124);snapshot.shieldValue=snapshot.shield?Read<float>(snapshot.shield,0x124):0;
    snapshot.eventIndex=Read<int32_t>(snapshot.handler,0x1e0);snapshot.pendingEvents=Read<int32_t>(snapshot.handler,0x248);
    snapshot.alive=Read<uint8_t>(snapshot.handler,0x1f9)!=0;
    snapshot.valid=std::isfinite(snapshot.healthValue)&&snapshot.healthValue>0&&std::isfinite(snapshot.shieldValue)
        &&snapshot.shieldValue>=0&&snapshot.alive;return snapshot.valid;
}
inline bool SameLife(const LifeSnapshot& before){
    return before.valid&&Read<void*>(before.pawn,0xc70)==before.handler
        &&Read<float>(before.health,0x124)==before.healthValue
        &&(!before.shield||Read<float>(before.shield,0x124)==before.shieldValue)
        &&Read<int32_t>(before.handler,0x1e0)==before.eventIndex
        &&Read<int32_t>(before.handler,0x248)==before.pendingEvents
        &&(Read<uint8_t>(before.handler,0x1f9)!=0)==before.alive;
}
inline bool CaptureContact(AttackScope& scope,void* parameters){
    auto& source=*scope.source;auto& state=*scope.state;
    if(!parameters||!original_players::Readable(parameters,0xf1)||!SourceIdentity(source)
       ||!Live(state.object,state.identity)||Read<void*>(state.object,0xc8)!=source.weapon){++diagnostics.invalidPayloads;return false;}
    const auto native=static_cast<const uint8_t*>(parameters);
    auto victim=engine::function<void*(*)(const void*)>(0x437b290)(native+0xb8);
    diagnostics.lastNativeHitActor=victim;diagnostics.lastAttackState=state.object;
    size_t victimIndex=sources.size();
    for(size_t i=0;i<MaxPlayers;++i){const auto p=deathmatch_rules::Player(i);if(p&&p->registered&&p->pawn==victim){victimIndex=i;break;}}
    if(victimIndex==sources.size()){++diagnostics.worldContacts;return false;}
    const auto& before=scope.before[victimIndex];const auto target=deathmatch_rules::Player(victimIndex);
    if(!before.valid||before.pawn!=victim||!target||target->spawnEpoch!=before.epoch){++diagnostics.missingVictimBindings;return false;}
    Weak componentIdentity{};std::memcpy(&componentIdentity,native+0xd0,sizeof(componentIdentity));
    auto component=original_inventory_slots::ResolveIdentity(componentIdentity);
    diagnostics.lastNativeHitComponent=component;
    diagnostics.lastNativeComponentKind=component==original_damage::targets[victimIndex].core?"original_core_mesh":
        component==Read<void*>(victim,0x500)?"original_capsule":"other_original_component";
    if(!component||component!=original_damage::targets[victimIndex].core){++diagnostics.nonCoreContacts;return false;}
    Contact contact;contact.present=true;contact.source=static_cast<uint32_t>(source.index);contact.victim=static_cast<uint32_t>(victimIndex);
    contact.sourceEpoch=source.spawnEpoch;contact.victimEpoch=before.epoch;contact.binding=source.binding;contact.occurrence=scope.occurrence;
    contact.at=Now();contact.backstab=native[0xf0]!=0;contact.sourcePawn=source.pawn;contact.victimPawn=victim;
    contact.weapon=source.weapon;contact.state=state.object;contact.sourceIdentity=source.pawnIdentity;
    contact.weaponIdentity=source.weaponIdentity;contact.stateIdentity=state.identity;
    const auto tuning=static_cast<const uint8_t*>(state.object)+(contact.backstab?0x328:0x310);
    contact.attackDamage=Read<float>(tuning,0);contact.damageType=Read<void*>(tuning,8);
    if(!std::isfinite(contact.attackDamage)||contact.attackDamage<=0||contact.attackDamage>1000000
       ||!original_players::Derived(contact.damageType,engine::Class(original_damage::DamageTypeClass))
       ||!Identity(victim,contact.victimIdentity)||!Identity(contact.damageType,contact.damageTypeIdentity)){
        ++diagnostics.invalidPayloads;return false;}
    auto& hit=contact.nativeHit;hit.hit=true;hit.blocking=(native[0xad]&1U)!=0;hit.component=component;
    hit.distance=Read<float>(native,8);hit.location=Read<engine::Vec>(native,0x28);hit.normal=Read<engine::Vec>(native,0x58);
    std::memcpy(hit.bone.bytes,native+0xd8,sizeof(hit.bone));std::memcpy(hit.nativeHit,native,sizeof(hit.nativeHit));
    // The real FPointDamageEvent at7130e02..e4d sets ShotDirection to the
    // negative of this exact original ImpactNormal. Do not substitute aim.
    contact.nativeContact.observed_at=contact.at;contact.nativeContact.start=Read<engine::Vec>(native,0x70);
    contact.nativeContact.direction={-hit.normal.x,-hit.normal.y,-hit.normal.z};
    contact.nativeContact.sequence=scope.occurrence;
    if(!std::isfinite(hit.distance)||hit.distance<0||!original_players::Finite(hit.location)
       ||!original_players::Finite(hit.normal)||!original_players::Finite(contact.nativeContact.start)){
        ++diagnostics.invalidPayloads;return false;}
    // Classification reuses the original native bone/region query. Melee's
    // native attack does not apply a firearm-style head/leg multiplier.
    alignas(8) uint8_t regional[0xf8]{};std::memcpy(regional,native,0xf0);
    if(!original_damage::Event(victim,L"/Script/ShooterGame.ShooterCharacter.GetRegionalDamage",0xf2,regional)||regional[0xf1]>2){
        ++diagnostics.invalidPayloads;return false;}
    hit.region=regional[0xf1];scope.contact=contact;++state.contacts;++source.contacts;++diagnostics.exactAuthorityContacts;return true;
}
__declspec(noinline) inline void ObserveEvent(void* object,void* function,void* parameters){
    auto state=GetCurrentThreadId()==gameThread?FindState(object):nullptr;
    const auto forward=state&&state->forwardEvent?state->forwardEvent:engine::function<NativeEvent>(OriginalEvent);
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    auto scope=activeScope;
    if(Thread()&&state&&scope&&scope->state==state&&function==impactFunction
       &&caller==engine::image+0x7130fa6){
        ++diagnostics.impactCallbacks;++scope->impactEvents;
        if(scope->impactEvents==1)CaptureContact(*scope,parameters);
        else{scope->contact.present=false;++diagnostics.ambiguousContacts;}
    }
    forward(object,function,parameters);
}
__declspec(noinline) inline void ObserveAttackTick(void* object,float nativeTime0,float nativeTime1){
    Source* source=nullptr;auto state=GetCurrentThreadId()==gameThread?FindState(object,&source):nullptr;
    const auto forward=state&&state->forwardAttackTick?state->forwardAttackTick:engine::function<NativeAttackTick>(OriginalAttackTick);
    if(!Thread()||!state||!source||!SourceIdentity(*source)){forward(object,nativeTime0,nativeTime1);return;}
    AttackScope scope;scope.source=source;scope.state=state;scope.occurrence=++nextOccurrence;
    ++state->entries;++source->entries;++diagnostics.entries;
    for(size_t i=0;i<MaxPlayers;++i)ReadLife(i,scope.before[i]);
    const auto previous=activeScope;activeScope=&scope;
    // Preserve nesting and exception propagation of the original attack.
    __try{forward(object,nativeTime0,nativeTime1);}__finally{activeScope=previous;}
    if(hooks.weaponUsed){
        // The shipped attack tick really completed, including its miss/wall
        // branches. Coalesce by original Pawn/epoch, not by a requested input
        // or successful hit. Keep this independent of weapon retirement so a
        // same-life switch cannot erase an actual use before the next Poll.
        pendingWeaponUses[source->index]={true,source->spawnEpoch,source->pawn,source->pawnIdentity};
        ++diagnostics.weaponUseTicks;
    }
    if(!scope.contact.present||scope.impactEvents!=1)return;
    auto& contact=scope.contact;
    contact.nativeLifeUnchanged=SameLife(scope.before[contact.victim]);
    if(!contact.nativeLifeUnchanged){++diagnostics.nativeLifeChanges;Fail(3104,"original_melee_native_life_already_changed_no_second_damage",true);return;}
    contact.token=hooks.allocateDamageToken?hooks.allocateDamageToken():0;
    if(!contact.token){Fail(3105,"original_melee_shared_damage_token_unavailable",true);return;}
    if(writeIndex-readIndex>=queue.size()){++diagnostics.queueOverflow;Fail(3106,"original_melee_contact_queue_full",true);return;}
    queue[writeIndex++%queue.size()]=contact;++diagnostics.queued;
}
inline bool InitializeImpl(void* originalWorld,const Hooks& callbacks,const Options& settings){
    if(diagnostics.initialized)return Thread()&&world==originalWorld;
    if(!original_players::Thread()||!originalWorld||!deathmatch_rules::State().initialized
       ||deathmatch_rules::world!=originalWorld||!callbacks.allocateDamageToken||!callbacks.beforeNativeDeath
       ||!std::isfinite(settings.maximumContactAge)||settings.maximumContactAge<=0||settings.maximumContactAge>10
       ||!settings.maximumContactsPerPoll||settings.maximumContactsPerPoll>QueueCapacity)
        return Fail(3107,"original_melee_original_match_and_shared_hooks_required");
    auto type=engine::Class(AttackClass);impactFunction=engine::Find(ImpactPath);
    if(!type||!impactFunction||Read<uint16_t>(impactFunction,0xbe)!=0xf1||Read<void*>(impactFunction,0x28)!=type)
        return Fail(3108,"original_melee_original_impact_abi_missing");
    if(!original_damage::Initialize(originalWorld))return Fail(original_damage::State().error,original_damage::State().stage);
    hooks=callbacks;options=settings;world=originalWorld;gameThread=GetCurrentThreadId();
    diagnostics.initialized=true;diagnostics.ready=true;diagnostics.error=0;
    diagnostics.stage="original_melee_waiting_for_real_equipped_state_instances";return true;
}
inline bool Initialize(void* originalWorld,const Hooks& callbacks,const Options& settings=Options{}){
    __try{return InitializeImpl(originalWorld,callbacks,settings);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool BindImpl(size_t index,void* pawn,void* weapon,uint32_t epoch){
    if(!Thread()||index>=sources.size()||!epoch)return Fail(3109,"original_melee_bind_context_invalid");
    auto& source=sources[index];
    if(source.bound){if(source.pawn==pawn&&source.weapon==weapon&&source.spawnEpoch==epoch)return SourceIdentity(source);
        return Fail(3110,"original_melee_before_retire_required_for_new_binding");}
    if(!IsMeleeWeapon(weapon))return true;
    const auto player=deathmatch_rules::Player(index);
    if(!player||!player->registered||player->pawn!=pawn||player->spawnEpoch!=epoch
       ||!original_players::IsA(pawn,original_players::ShooterCharacterClass)||Read<void*>(weapon,0x4a8)!=pawn
       ||Read<uint8_t>(weapon,0x224)!=3)return Fail(3111,"original_melee_requires_real_authority_owned_equippable");
    source={};source.index=index;source.pawn=pawn;source.weapon=weapon;source.spawnEpoch=epoch;source.binding=++nextBinding;
    if(!Identity(pawn,source.pawnIdentity)||!Identity(weapon,source.weaponIdentity))return Fail(3112,"original_melee_original_source_identity_failed");
    struct Query {void* type=nullptr;void** data=nullptr;int32_t count=0,capacity=0;} query{engine::Class(AttackClass)};
    static_assert(sizeof(Query)==24);
    const bool queried=original_damage::Event(weapon,L"/Script/Engine.Actor.K2_GetComponentsByClass",24,&query);
    bool valid=queried&&query.count>0&&query.count<=static_cast<int32_t>(MaxStates)&&query.capacity>=query.count
        &&original_players::Readable(query.data,static_cast<size_t>(query.count)*8);
    if(valid){
        source.stateCount=static_cast<size_t>(query.count);
        for(size_t i=0;i<source.stateCount;++i){
            auto& state=source.states[i];state.object=query.data[i];
            if(!original_players::IsA(state.object,AttackClass)||Read<void*>(state.object,0xc8)!=weapon
               ||!Identity(state.object,state.identity)||!CopyVtable(state.object,state.table)){valid=false;break;}
            state.forwardAttackTick=reinterpret_cast<NativeAttackTick>(state.table.previous[AttackTickSlot]);
            state.forwardEvent=reinterpret_cast<NativeEvent>(state.table.previous[EventSlot]);
            if(state.forwardAttackTick!=engine::function<NativeAttackTick>(OriginalAttackTick)
               ||state.forwardEvent!=engine::function<NativeEvent>(OriginalEvent)
               ||state.table.previous[0x598/8]!=engine::function<void*>(0x7132200)){valid=false;break;}
        }
    }
    if(query.data)engine::function<void(*)(void*)>(0x1915720)(query.data);
    if(!valid)return Fail(3113,"original_melee_exact_native_attack_components_not_ready");
    if(!original_damage::Bind(index,pawn))return Fail(original_damage::State().error,original_damage::State().stage);
    source.bound=true;
    for(size_t i=0;i<source.stateCount;++i){auto& state=source.states[i];
        state.table.entries[1+AttackTickSlot]=reinterpret_cast<void*>(ObserveAttackTick);
        state.table.entries[1+EventSlot]=reinterpret_cast<void*>(ObserveEvent);
        if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(state.object),state.table.Address(),state.table.previous)!=state.table.previous){
            for(size_t n=0;n<i;++n)Restore(source.states[n]);source.bound=false;
            return Fail(3114,"original_melee_component_observer_install_raced",true);}
        state.table.installed=true;
    }
    ++diagnostics.binds;++diagnostics.boundSources;diagnostics.ready=true;diagnostics.error=0;
    diagnostics.stage="original_melee_real_attack_observers_bound";return true;
}
inline bool Bind(size_t index,void* pawn,void* weapon,uint32_t epoch){
    __try{return BindImpl(index,pawn,weapon,epoch);}__except(Capture(GetExceptionInformation())){return false;}
}
enum class Outcome:uint8_t{Done,Deferred,Failure};
inline Outcome ApplyContact(Contact& contact,double now){
    if(!contact.present||!contact.nativeLifeUnchanged||!contact.token)return Outcome::Done;
    if(contact.source>=sources.size()||contact.victim>=sources.size())return Outcome::Done;
    auto& source=sources[contact.source];const auto victim=deathmatch_rules::Player(contact.victim);
    if(!source.bound||source.binding!=contact.binding||source.spawnEpoch!=contact.sourceEpoch
       ||!SourceIdentity(source)||!victim||!victim->registered||victim->spawnEpoch!=contact.victimEpoch||victim->pawn!=contact.victimPawn
       ||!Live(contact.victimPawn,contact.victimIdentity)||!Live(contact.state,contact.stateIdentity)
       ||!Live(contact.damageType,contact.damageTypeIdentity)||now-contact.at>options.maximumContactAge||now<contact.at){
        ++diagnostics.staleContacts;return Outcome::Done;}
    if(!deathmatch_rules::CanDamage(contact.source,contact.victim)){++diagnostics.policyRejected;return Outcome::Done;}
    if(!original_damage::Sample(contact.victim)){
        Fail(original_damage::State().error,original_damage::State().stage);return Outcome::Failure;}
    if(original_damage::targets[contact.victim].pendingDamageEvents){++diagnostics.pendingDamageDeferred;return Outcome::Deferred;}
    original_damage::DeathData death;
    if(!deathmatch_rules::BeginNativeDamage(contact.token,contact.source,contact.victim,contact.weapon,now,death)){
        if(!deathmatch_rules::CanDamage(contact.source,contact.victim)){++diagnostics.policyRejected;return Outcome::Done;}
        Fail(deathmatch_rules::State().error,deathmatch_rules::State().stage);return Outcome::Failure;}
    original_damage::Result result;diagnostics.operation="original_melee_contact_to_native_point_notification";
    const bool applied=original_damage::ApplyPoint(contact.victim,contact.sourcePawn,contact.weapon,contact.damageType,
        contact.nativeContact,contact.nativeHit,contact.attackDamage,contact.token,result,&death,1.0f);
    const bool committed=deathmatch_rules::EndNativeDamage(applied,result);
    if(!applied||!committed){Fail(!applied?original_damage::State().error:deathmatch_rules::State().error,
        !applied?original_damage::State().stage:deathmatch_rules::State().stage);return Outcome::Failure;}
    diagnostics.lastSource=static_cast<int32_t>(contact.source);diagnostics.lastVictim=static_cast<int32_t>(contact.victim);
    diagnostics.lastToken=contact.token;diagnostics.lastOccurrence=contact.occurrence;diagnostics.lastDamage=contact.attackDamage;
    diagnostics.lastBackstab=contact.backstab;diagnostics.lastRegion=contact.nativeHit.region;
    if(result.applied&&!result.duplicate){++source.damageEvents;++diagnostics.damageEvents;}
    if(result.killed&&!result.duplicate){++source.kills;++diagnostics.kills;
        if(!BeforeRetire(contact.victim)||!hooks.beforeNativeDeath(contact.victim))return Outcome::Failure;}
    diagnostics.operation="none";return Outcome::Done;
}
inline void DispatchWeaponUses(){
    for(size_t index=0;index<pendingWeaponUses.size();++index){auto& pending=pendingWeaponUses[index];
        if(!pending.pending)continue;
        const auto record=pending;pending={};
        if(!hooks.weaponUsed)continue;
        const auto player=deathmatch_rules::Player(index);
        if(!player||!player->registered||player->pawn!=record.pawn||player->spawnEpoch!=record.spawnEpoch
           ||!Live(record.pawn,record.identity)){++diagnostics.staleWeaponUses;continue;}
        // Clear before the external call; a newly observed attack can queue
        // another use without recursion or being erased after the callback.
        hooks.weaponUsed(index);++diagnostics.weaponUseNotifications;
    }
}
inline bool PollImpl(double now){
    if(!Thread()||!std::isfinite(now)||now<0||diagnostics.processing)return false;
    diagnostics.processing=true;diagnostics.lastPoll=now;
    DispatchWeaponUses();
    size_t processed=0;
    while(readIndex<writeIndex&&processed<options.maximumContactsPerPoll){
        auto& contact=queue[readIndex%queue.size()];const auto result=ApplyContact(contact,now);
        if(result==Outcome::Failure){diagnostics.processing=false;return false;}
        if(result==Outcome::Deferred)break;
        contact={};++readIndex;++processed;++diagnostics.processed;
    }
    diagnostics.processing=false;diagnostics.ready=true;diagnostics.error=0;
    diagnostics.stage="original_melee_observing_native_contacts";return true;
}
inline bool Poll(double now){__try{return PollImpl(now);}__except(Capture(GetExceptionInformation())){diagnostics.processing=false;return false;}}
inline bool Shutdown(){
    if(!Thread()||activeScope)return false;for(size_t i=0;i<sources.size();++i)if(!BeforeRetire(i))return false;
    for(auto& pending:pendingWeaponUses)pending={};
    readIndex=writeIndex;diagnostics.ready=false;diagnostics.stage="original_melee_observers_restored";return true;
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;
    const auto boolean=[](bool value){return value?"true":"false";};
    std::fputs("{\"schema_version\":1,\"stage\":",file);original_players::Text(file,diagnostics.stage);
    std::fputs(",\"operation\":",file);original_players::Text(file,diagnostics.operation);
    size_t pendingUses=0;for(const auto& pending:pendingWeaponUses)if(pending.pending)++pendingUses;
    std::fprintf(file,",\"weapon_use_hook_enabled\":%s,\"weapon_use_ticks\":%llu,\"weapon_use_notifications\":%llu,\"stale_weapon_uses\":%llu,\"pending_weapon_uses\":%zu",
        boolean(hooks.weaponUsed!=nullptr),static_cast<unsigned long long>(diagnostics.weaponUseTicks),
        static_cast<unsigned long long>(diagnostics.weaponUseNotifications),static_cast<unsigned long long>(diagnostics.staleWeaponUses),pendingUses);
    std::fprintf(file,",\"initialized\":%s,\"ready\":%s,\"restart_required\":%s,\"error\":%u,\"exception_rva\":%llu,\"bound_sources\":%u,\"binds\":%llu,\"retirements\":%llu,\"restored_tables\":%llu,\"foreign_tables\":%llu",
        boolean(diagnostics.initialized),boolean(diagnostics.ready),boolean(diagnostics.restartRequired),diagnostics.error,
        static_cast<unsigned long long>(diagnostics.exceptionRva),diagnostics.boundSources,static_cast<unsigned long long>(diagnostics.binds),
        static_cast<unsigned long long>(diagnostics.retirements),static_cast<unsigned long long>(diagnostics.restoredTables),static_cast<unsigned long long>(diagnostics.foreignTables));
    std::fprintf(file,",\"native_entries\":%llu,\"impact_callbacks\":%llu,\"exact_authority_contacts\":%llu,\"world_contacts\":%llu,\"invalid_payloads\":%llu,\"ambiguous_contacts\":%llu,\"non_core_contacts\":%llu,\"missing_victim_bindings\":%llu,\"native_life_changes\":%llu",
        static_cast<unsigned long long>(diagnostics.entries),static_cast<unsigned long long>(diagnostics.impactCallbacks),static_cast<unsigned long long>(diagnostics.exactAuthorityContacts),
        static_cast<unsigned long long>(diagnostics.worldContacts),static_cast<unsigned long long>(diagnostics.invalidPayloads),static_cast<unsigned long long>(diagnostics.ambiguousContacts),
        static_cast<unsigned long long>(diagnostics.nonCoreContacts),static_cast<unsigned long long>(diagnostics.missingVictimBindings),static_cast<unsigned long long>(diagnostics.nativeLifeChanges));
    std::fprintf(file,",\"queued\":%llu,\"queue_overflow\":%llu,\"processed\":%llu,\"stale_contacts\":%llu,\"policy_rejected\":%llu,\"pending_damage_deferred\":%llu,\"damage_events\":%llu,\"kills\":%llu,\"last_source\":%d,\"last_victim\":%d,\"last_damage\":%.9g,\"last_backstab\":%s,\"last_region\":%u,\"last_token\":%llu,\"last_occurrence\":%llu,\"pending_contacts\":%llu,\"sources\":[",
        static_cast<unsigned long long>(diagnostics.queued),static_cast<unsigned long long>(diagnostics.queueOverflow),static_cast<unsigned long long>(diagnostics.processed),
        static_cast<unsigned long long>(diagnostics.staleContacts),static_cast<unsigned long long>(diagnostics.policyRejected),static_cast<unsigned long long>(diagnostics.pendingDamageDeferred),
        static_cast<unsigned long long>(diagnostics.damageEvents),static_cast<unsigned long long>(diagnostics.kills),diagnostics.lastSource,diagnostics.lastVictim,diagnostics.lastDamage,
        boolean(diagnostics.lastBackstab),static_cast<unsigned>(diagnostics.lastRegion),static_cast<unsigned long long>(diagnostics.lastToken),static_cast<unsigned long long>(diagnostics.lastOccurrence),
        static_cast<unsigned long long>(writeIndex-readIndex));
    bool comma=false;for(const auto& source:sources)if(source.bound){
        std::fprintf(file,"%s{\"index\":%zu,\"epoch\":%u,\"binding\":%llu,\"pawn\":\"%p\",\"weapon\":\"%p\",\"state_count\":%zu,\"native_entries\":%llu,\"native_contacts\":%llu,\"damage_events\":%llu,\"kills\":%llu}",
            comma?",":"",source.index,source.spawnEpoch,static_cast<unsigned long long>(source.binding),source.pawn,source.weapon,source.stateCount,
            static_cast<unsigned long long>(source.entries),static_cast<unsigned long long>(source.contacts),static_cast<unsigned long long>(source.damageEvents),static_cast<unsigned long long>(source.kills));comma=true;
    }
    std::fprintf(file,"],\"last_native_hit_actor\":\"%p\",\"last_native_hit_component\":\"%p\",\"last_attack_state\":\"%p\",\"last_native_component_kind\":",
        diagnostics.lastNativeHitActor,diagnostics.lastNativeHitComponent,diagnostics.lastAttackState);
    original_players::Text(file,diagnostics.lastNativeComponentKind);
    std::fputs(",\"native_attack_trace_reused\":true,\"native_backstab_reused\":true,\"damage_source\":\"actual_attack_state_AresMeleeTuning\",\"camera_ray_used\":false,\"knife_distance_or_damage_table_written\":false,\"local_authority_notification\":true,\"original_server_damage_claimed\":false,\"runtime_acceptance_recorded\":false}\n",file);
    const bool okay=std::ferror(file)==0;std::fclose(file);return okay;
}
}
