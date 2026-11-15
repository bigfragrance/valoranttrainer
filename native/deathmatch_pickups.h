#pragma once
#include "original_recovery.h"
#include "original_effects.h"
#include <array>
#include <cmath>
#include <cstring>
#include <cwchar>

// Original ordinary-DM assets and client leaves, plus the missing local
// authority records. No NetMode/role changes, script edits, synthetic overlap,
// damage-based healing, custom particles or replacement materials.
// Evidence: work/deathmatch-gameplay-audit and
// work/deathmatch-spawn-protection-static. Root owns the game loop and build.
namespace deathmatch_pickups {
using Vec=engine::Vec;
using Weak=original_effects::WeakIdentity;
inline constexpr size_t InitialPool=20,MaxPool=32,MaxPending=128,MaxPlayers=original_players::MaxPlayers;
inline constexpr double Lifetime=8.0,DropOffset=25.0,KillReloadDelay=0.10000000149011612;
inline constexpr const wchar_t* PackClassPath=L"/Game/GameModes/Deathmatch/GameObject_HealthPack.GameObject_HealthPack_C";
inline constexpr const wchar_t* PoolClassPath=L"/Game/GameModes/Deathmatch/PoolingGameObject.PoolingGameObject_C";
inline constexpr const wchar_t* AmmoClassPath=L"/Game/GameModes/Components/Comp_PlayerCharacter_AuthAmmoReplenisher.Comp_PlayerCharacter_AuthAmmoReplenisher_C";
inline constexpr const wchar_t* AmmoLeaf=L"/Game/GameModes/Components/Comp_PlayerCharacter_AuthAmmoReplenisher.Comp_PlayerCharacter_AuthAmmoReplenisher_C.ReplenishAmmo";
inline constexpr const wchar_t* EnableMethod=L"/Game/GameModes/Deathmatch/GameObject_HealthPack.GameObject_HealthPack_C.Multicast Enable";
inline constexpr const wchar_t* DisableMethod=L"/Game/GameModes/Deathmatch/GameObject_HealthPack.GameObject_HealthPack_C.Multicast Disable";
inline constexpr const wchar_t* OnRepEnabled=L"/Game/GameModes/Deathmatch/PoolingGameObject.PoolingGameObject_C.OnRep_IsEnabled";
inline constexpr const wchar_t* SpawnProtection=L"/Game/Characters/_Core/BasePawn.BasePawn_C.ApplySpawnInvulnerability";
inline constexpr const wchar_t* ProtectionClass=L"/Game/Characters/_Core/Comp_PlayerCharacter_SpawnInvulnerability.Comp_PlayerCharacter_SpawnInvulnerability_C";
inline constexpr const wchar_t* ProtectionDisable=L"/Game/Characters/_Core/Comp_PlayerCharacter_SpawnInvulnerability.Comp_PlayerCharacter_SpawnInvulnerability_C.DisableInvulnerability";
struct Name {uint32_t index=0,display=0,number=0;};
struct Array {void** data=nullptr;int32_t count=0,capacity=0;};
struct Pack {void* actor=nullptr;void* sphere=nullptr;Weak identity{};bool active=false;float sphereRadius=0;double expires=0;uint64_t token=0;int32_t lastPicker=-1;Vec location{};};
struct Pending {bool used=false,reloaded=false,dropped=false;uint64_t token=0;size_t source=0,victim=0;Weak gun{},sourcePawn{},victimPawn{};Vec location{};double reloadAt=0;};
struct Protection {void* pawn=nullptr;Weak identity{};bool requested=false,observed=false,protectedNow=false;float invulnerable=0;double requestedAt=0;uint32_t applies=0,fireCancels=0;};
struct Diagnostics {
    bool initialized=false,ready=false,restartRequired=false,defaultsVerified=false,boolLayoutVerified=false,nativeBindingsVerified=false,stopped=false;
    unsigned error=0;const char* stage="deathmatch_pickups_not_initialized";const wchar_t* failedEvent=L"";
    uint16_t expectedSize=0,actualSize=0;uintptr_t exceptionRva=0;
    void *world=nullptr,*gameState=nullptr,*packClass=nullptr,*poolClass=nullptr,*ammoClass=nullptr,*pickupFx=nullptr,*ammoFx=nullptr,*hudBuff=nullptr,*protectionClass=nullptr,*killReloadComponent=nullptr;
    Weak worldIdentity{};int enabledOffset=-1,sphereOffset=-1;
    uint32_t poolCreated=0,active=0,pending=0,killEvents=0,duplicateTokens=0,drops=0,pickups=0,expired=0;
    uint32_t overlapQueries=0,overlapActors=0,recoveryDeferred=0,recoveryRejected=0,ammoLeafCalls=0,ammoUnitsAdded=0;
    uint32_t killReloads=0,killReloadSkipped=0,pickupReloads=0,fxRequests=0,hudBuffRequests=0,hudBuffHandles=0;
    uint32_t authorityEnabledWrites=0,originalEnables=0,originalDisables=0,protectionApplies=0,protectedReads=0,protectionReadFailures=0,fireCancels=0;
    uint32_t protectionConfirmed=0,protectionNotObserved=0;
    int32_t magazineRestored=30,reserveRestored=30,lastMagazineBefore=0,lastMagazineAfter=0,lastReserveBefore=0,lastReserveAfter=0;
    bool lastAmmoReadback=false;
    double worldTime=0,callerSeconds=0;size_t initialPool=InitialPool;
    bool healByOriginalNotify=true,physicsOverlapOnly=true,netModeChanged=false,sourceScriptsChanged=false;
};
inline Diagnostics state{};
inline std::array<Pack,MaxPool> pool{};
inline std::array<Pending,MaxPending> pending{};
inline std::array<Protection,MaxPlayers> protections{};
inline std::array<uint64_t,1024> tokens{};inline size_t nextToken=0;
inline double nextOverlap=0;
inline const Diagnostics& State(){return state;}
inline bool Ready(){return state.initialized&&state.ready&&!state.restartRequired;}
template<class T> inline T Read(const void* p,size_t offset){return original_players::Value<T>(p,offset);}
inline bool Thread(){return !state.restartRequired&&original_players::Thread();}
inline bool Fail(unsigned code,const char* stage){state.error=code;state.stage=stage;state.ready=false;return false;}
inline bool Event(void* object,const wchar_t* path,uint16_t bytes,void* params){
    auto fn=engine::Find(path);const auto actual=Read<uint16_t>(fn,0xbe);
    auto table=Read<void*>(object,0),process=Read<void*>(table,0x298);
    if(!object||!fn||actual!=bytes||!process){state.failedEvent=path;state.expectedSize=bytes;state.actualSize=actual;return false;}
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,fn,params);return true;
}
inline bool Profile(const wchar_t* path,uint16_t params,int scriptBytes,uint8_t first){
    auto fn=engine::Find(path);auto code=Read<uint8_t*>(fn,0x68);
    if(!fn||Read<uint16_t>(fn,0xbe)!=params||Read<int32_t>(fn,0x70)!=scriptBytes||!code
       ||Read<uint8_t>(code,0)!=first||Read<uint8_t>(code,static_cast<size_t>(scriptBytes-1))!=0x53){
        state.failedEvent=path;state.expectedSize=params;state.actualSize=Read<uint16_t>(fn,0xbe);return false;
    }
    return true;
}
inline void* LoadClass(uintptr_t base,const wchar_t* path){return engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t,void*)>(0x1d0c6e0)(engine::Class(base),nullptr,path,nullptr,0,nullptr);}
inline int Field(void* type,const wchar_t* text,int bytes){
    Name name{};engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&name,text,1);
    auto p=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,&name,3);
    auto cls=Read<void*>(p,8);const auto flags=Read<uint64_t>(cls,0x18);
    const int offset=Read<int32_t>(p,0x44),size=Read<int32_t>(type,0x60);
    if(!p||(flags&0x8001)!=0x8001||Read<uint64_t>(p,0x20)!=Read<uint64_t>(&name,0)
       ||Read<int32_t>(p,0x34)!=bytes||offset<0x30||size<=0||size>65536||offset+bytes>size)return -1;
    return offset;
}
inline bool EnabledBoolLayout(){
    Name name{};engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&name,L"IsEnabled",1);
    auto p=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(state.packClass,&name,3);
    auto cls=Read<void*>(p,8);
    // Pinned FBoolProperty ctor1c77ec0 / class getter1c77e20, not a guessed
    // generic property layout. This source field is a full native byte bool.
    state.boolLayoutVerified=p&&(Read<uint64_t>(cls,0x18)&0x28001)==0x28001
        &&Read<int32_t>(p,0x30)==1&&Read<int32_t>(p,0x34)==1&&Read<int32_t>(p,0x44)==state.enabledOffset
        &&Read<uint8_t>(p,0x78)==1&&Read<uint8_t>(p,0x79)==0&&Read<uint8_t>(p,0x7a)==1&&Read<uint8_t>(p,0x7b)==0xff;
    return state.boolLayoutVerified;
}
inline bool NativeBindings(){
    struct Binding{const wchar_t* path;uint16_t size;uintptr_t exec;};
    constexpr Binding bindings[]={
        {L"/Script/Engine.PrimitiveComponent.GetOverlappingActors",24,0x3e89b20},
        {L"/Script/Engine.PrimitiveComponent.SetGenerateOverlapEvents",1,0x3e8cae0},
        {L"/Script/Engine.PrimitiveComponent.GetGenerateOverlapEvents",1,0x3e8cc00},
        {L"/Script/Engine.PrimitiveComponent.GetCollisionEnabled",1,0x3e807c0},
        {L"/Script/Engine.Actor.K2_GetActorLocation",24,0x3ef1cd0},
        {L"/Script/ShooterGame.ShooterCharacter.CreateAndApplyBuff",40,0x6c66f70},
        {L"/Script/ShooterGame.EffectsBlueprintLibrary.PlayOneShotEffect_Blueprint",200,0x6751cf0},
        {L"/Script/ShooterGame.AresInventory.GetCurrentEquippable",8,0x649bcd0},
        {L"/Script/ShooterGame.AresInventory.GetFirstItemFromSlot",16,0x649c5a0}
    };
    engine::Class(0x674a040);engine::Class(0x2a653a0);engine::Class(0x6480c50);
    for(const auto& binding:bindings){auto fn=engine::Find(binding.path);
        if(!fn||Read<uint16_t>(fn,0xbe)!=binding.size||(Read<uint32_t>(fn,0xb8)&0x400)==0||Read<uintptr_t>(fn,0xe0)!=engine::image+binding.exec){
            state.failedEvent=binding.path;state.expectedSize=binding.size;state.actualSize=Read<uint16_t>(fn,0xbe);return false;
        }
    }
    state.nativeBindingsVerified=true;return true;
}
inline bool Defaults(){
    auto cdo=engine::DefaultObject(state.packClass);
    const auto life=Field(state.packClass,L"Lifespan",8),reload=Field(state.packClass,L"ShouldReload",1);
    const auto hp=Field(state.packClass,L"ShouldFullyRefreshHealth",1),armor=Field(state.packClass,L"ShouldFullyRefreshArmor",1);
    const auto radar=Field(state.packClass,L"ShouldRadarPing",1),fx=Field(state.packClass,L"PickupFXC",8);
    state.enabledOffset=Field(state.packClass,L"IsEnabled",1);state.sphereOffset=Field(state.packClass,L"Sphere",8);
    state.defaultsVerified=cdo&&life>=0&&reload>=0&&hp>=0&&armor>=0&&radar>=0&&fx>=0&&state.enabledOffset>=0&&state.sphereOffset>=0
        &&Read<double>(cdo,life)==Lifetime&&Read<uint8_t>(cdo,reload)==1&&Read<uint8_t>(cdo,hp)==1&&Read<uint8_t>(cdo,armor)==1&&Read<uint8_t>(cdo,radar)==0
        &&Read<void*>(cdo,fx)==state.pickupFx&&Read<uint8_t>(cdo,state.enabledOffset)==0&&EnabledBoolLayout();
    return state.defaultsVerified;
}
inline bool PublishEnabled(Pack& pack,bool enabled){
    const auto old=Read<uint8_t>(pack.actor,state.enabledOffset);if(old>1)return false;
    const uint8_t value=enabled?1:0;if(old==value)return true;
    // Original PoolingGameObject.SetEnabled has a dedicated-server-only gate.
    // Publish only that missing authority bool, then execute its original OnRep.
    if(!original_players::Write(pack.actor,state.enabledOffset,value)||!Event(pack.actor,OnRepEnabled,0,nullptr)
       ||Read<uint8_t>(pack.actor,state.enabledOffset)!=value)return false;
    ++state.authorityEnabledWrites;return true;
}
inline bool Live(const Pack& pack){return pack.actor&&original_effects::ResolveIdentity(pack.identity)==pack.actor;}
inline bool EffectsReady(void* actor){
    for(const auto& e:original_effects::managers)if(e.actor==actor&&e.component)
        return e.identityVerified&&e.registered&&e.hasBegunPlay&&!e.deferred;
    return false;
}
inline bool Disable(Pack& pack,bool expiration=false){
    if(!Live(pack))return false;
    if(!PublishEnabled(pack,false)||!Event(pack.actor,DisableMethod,0,nullptr))return false;
    ++state.originalDisables;pack.active=false;if(expiration)++state.expired;return true;
}
inline bool CreatePack(){
    if(state.poolCreated>=MaxPool)return false;
    engine::Transform transform;
    auto actor=engine::function<void*(*)(void*,void*,const engine::Transform*,uint8_t,void*,uint8_t)>(0x43fada0)
        (state.world,state.packClass,&transform,1,nullptr,0);
    if(!actor)return Fail(2921,"deathmatch_original_health_pack_spawn_failed");
    engine::function<void(*)(void*,const engine::Transform*,bool,void*,uint8_t)>(0x3ef6cb0)(actor,&transform,false,nullptr,0);
    auto& pack=pool[state.poolCreated];pack={};pack.actor=actor;
    pack.sphere=Read<void*>(actor,state.sphereOffset);
    if(Read<void*>(actor,0x10)!=state.packClass||Read<uint8_t>(actor,0x224)!=3
       ||!original_players::IsA(pack.sphere,0x41c1670)||Read<void*>(pack.sphere,0xc8)!=actor
       ||(Read<uint8_t>(pack.sphere,0xa0)&1)==0
       ||!Event(pack.sphere,L"/Script/Engine.SphereComponent.GetUnscaledSphereRadius",4,&pack.sphereRadius)||pack.sphereRadius<=0
       ||!original_effects::CaptureIdentity(actor,pack.identity)||!original_effects::EnsureActor(actor,"deathmatch_health_pack")
       ||!Disable(pack))return Fail(2922,"deathmatch_original_health_pack_components_invalid");
    ++state.poolCreated;return true;
}
inline bool Fx(void* actor,void* type){
    if(!actor||!type||!original_effects::EnsureActor(actor,"deathmatch_pickup_recipient"))return false;
    alignas(8) uint8_t p[200]{};std::memcpy(p+0x10,&type,8);std::memcpy(p+0x18,&actor,8);
    std::memcpy(p+0x80,&state.gameState,8);p[0xc5]=3;
    if(!Event(engine::DefaultObject(engine::Class(0x674a040)),L"/Script/ShooterGame.EffectsBlueprintLibrary.PlayOneShotEffect_Blueprint",200,p))return false;
    ++state.fxRequests;return true;
}
inline void* AmmoReplenisher(void* pawn){
    struct Query {void* type;void* result=nullptr;} q{state.ammoClass};
    if(!Event(pawn,L"/Script/Engine.Actor.GetComponentByClass",16,&q))return nullptr;
    if(q.result)return Read<void*>(q.result,0xc8)==pawn?q.result:nullptr;
    struct alignas(16) Add {void* type;uint8_t manual=0,pad[7]{};engine::Transform transform;uint8_t deferred=0,pad2[7]{};void* result=nullptr;} p{};
    static_assert(sizeof(Add)==128);p.type=state.ammoClass;
    if(!Event(pawn,L"/Script/Engine.Actor.AddComponentByClass",128,&p)||Read<void*>(p.result,0xc8)!=pawn)return nullptr;
    return p.result;
}
inline bool Replenish(void* gun,int magazineUnits,int reserveUnits,bool pickup){
    auto pawn=Read<void*>(gun,0x4a8);if(!pawn)return true;
    auto component=AmmoReplenisher(pawn);if(!component)return false;
    struct Params {void* gun;int32_t magazine,reserve,magazineAdded=0,reserveAdded=0;} p{gun,magazineUnits,reserveUnits};
    static_assert(sizeof(Params)==24);
    const auto type=Read<void*>(gun,0x10);const int magOffset=Field(type,L"MagazineAmmo",8),resOffset=Field(type,L"ReserveAmmo",8);
    auto magazine=magOffset>=0?Read<void*>(gun,magOffset):nullptr;auto reserve=resOffset>=0?Read<void*>(gun,resOffset):nullptr;
    state.lastAmmoReadback=magazine&&Event(magazine,L"/Script/ShooterGame.AmmoComponent.GetAmmo",4,&state.lastMagazineBefore);
    state.lastReserveBefore=state.lastReserveAfter=0;
    if(reserve)state.lastAmmoReadback=Event(reserve,L"/Script/ShooterGame.AmmoComponent.GetAmmo",4,&state.lastReserveBefore)&&state.lastAmmoReadback;
    // The server and client are one object in standalone. Calling the original
    // non-RPC mutating leaf once avoids replaying Local Client Force Update on
    // the same resource a second time. Original resource commits/limits remain.
    if(!Event(component,AmmoLeaf,24,&p)||p.magazineAdded<0||p.reserveAdded<0)return false;
    state.lastAmmoReadback=Event(magazine,L"/Script/ShooterGame.AmmoComponent.GetAmmo",4,&state.lastMagazineAfter)&&state.lastAmmoReadback;
    if(reserve)state.lastAmmoReadback=Event(reserve,L"/Script/ShooterGame.AmmoComponent.GetAmmo",4,&state.lastReserveAfter)&&state.lastAmmoReadback;
    ++state.ammoLeafCalls;state.ammoUnitsAdded+=static_cast<uint32_t>(p.magazineAdded+p.reserveAdded);
    if(pickup)++state.pickupReloads;return true;
}
inline bool PickupAmmo(size_t index){
    auto pawn=original_players::players[index].character;auto inventory=Read<void*>(pawn,0xc08);
    for(uint8_t slot=0;slot<2;++slot){struct Item {uint8_t slot=0,pad[7]{};void* result=nullptr;} p;p.slot=slot;
        if(!Event(inventory,L"/Script/ShooterGame.AresInventory.GetFirstItemFromSlot",16,&p))return false;
        if(!original_players::IsA(p.result,0x645c660))continue;
        const int magOffset=Field(Read<void*>(p.result,0x10),L"MagazineAmmo",8);
        if(magOffset<0)continue;auto magazine=Read<void*>(p.result,magOffset);int32_t maximum=0;
        if(!original_players::IsA(magazine,0x63c7e00)||!Event(magazine,L"/Script/ShooterGame.AmmoComponent.GetMaxAmmo",4,&maximum)||maximum<=0||maximum>65535)return false;
        if(!Replenish(p.result,maximum,maximum,true))return false;
    }
    return true;
}
inline bool Pickup(Pack& pack,size_t index){
    if(!original_recovery::RestoreMaximum(index)){
        if(original_recovery::State().busy){++state.recoveryDeferred;return true;}
        if(original_recovery::State().error)return Fail(original_recovery::State().error,"deathmatch_original_recovery_failed");
        ++state.recoveryRejected;return true;
    }
    auto pawn=original_players::players[index].character;
    struct Buff {void* type;void* causer=nullptr;void* instigator=nullptr;float level=1;uint8_t dead=0,pad[3]{};uint64_t result=0;} p{state.hudBuff};
    static_assert(sizeof(Buff)==40);
    if(!Event(pawn,L"/Script/ShooterGame.ShooterCharacter.CreateAndApplyBuff",40,&p)
       ||!PickupAmmo(index)||!Fx(pawn,state.pickupFx)||!Disable(pack))return Fail(2923,"deathmatch_original_pickup_leaves_failed");
    ++state.hudBuffRequests;if(static_cast<int32_t>(p.result)>=0)++state.hudBuffHandles;
    ++state.pickups;pack.lastPicker=static_cast<int32_t>(index);return true;
}
inline bool Overlaps(Pack& pack){
    struct Query {Array actors;void* filter;} q{{},engine::Class(original_players::ShooterCharacterClass)};
    static_assert(sizeof(Query)==24);
    if(!Event(pack.sphere,L"/Script/Engine.PrimitiveComponent.GetOverlappingActors",24,&q))return false;
    ++state.overlapQueries;bool valid=q.actors.count>=0&&q.actors.count<=128&&q.actors.capacity>=q.actors.count&&(!q.actors.count||q.actors.data);
    if(valid)for(int32_t n=0;n<q.actors.count&&pack.active;++n){auto actor=Read<void*>(q.actors.data,static_cast<size_t>(n)*8);++state.overlapActors;
        for(size_t i=0;i<MaxPlayers;++i)if(original_players::players[i].ready&&original_players::players[i].character==actor){
            uint8_t alive=0;if(!Event(actor,L"/Script/ShooterGame.ShooterCharacter.IsAlive",1,&alive)){valid=false;break;}
            if(alive&&!Pickup(pack,i)){valid=false;break;}
        }
    }
    if(q.actors.data)engine::function<void(*)(void*)>(0x1915720)(q.actors.data);return valid;
}
inline bool Enable(Pending& event){
    Pack* selected=nullptr;for(size_t i=0;i<state.poolCreated;++i)if(!pool[i].active&&Live(pool[i])){selected=&pool[i];break;}
    if(!selected&&state.poolCreated<MaxPool){if(!CreatePack())return false;selected=&pool[state.poolCreated-1];}
    if(!selected)return true; // Bounded local pool: keep the real kill queued, never fake a pickup.
    if(!EffectsReady(selected->actor))return true;
    auto& pack=*selected;auto victim=original_effects::ResolveIdentity(event.victimPawn);
    struct alignas(16) Parameters {engine::Transform transform;void* pawn=nullptr;void* playerInfo=nullptr;} p;
    static_assert(sizeof(Parameters)==112);p.transform.translation=event.location;p.pawn=victim;
    p.playerInfo=Read<void*>(original_players::players[event.victim].state,0xa18);
    if(!PublishEnabled(pack,true)||!Event(pack.actor,EnableMethod,112,&p))return false;
    Vec actual{};if(!Event(pack.actor,L"/Script/Engine.Actor.K2_GetActorLocation",24,&actual)
       ||std::abs(actual.x-event.location.x)>0.01||std::abs(actual.y-event.location.y)>0.01||std::abs(actual.z-event.location.z)>0.01)return false;
    uint8_t yes=1,collision=0,overlap=0;
    if(!Event(pack.sphere,L"/Script/Engine.PrimitiveComponent.SetGenerateOverlapEvents",1,&yes)
       ||!Event(pack.sphere,L"/Script/Engine.PrimitiveComponent.GetCollisionEnabled",1,&collision)
       ||!Event(pack.sphere,L"/Script/Engine.PrimitiveComponent.GetGenerateOverlapEvents",1,&overlap)||collision!=1||!overlap)return false;
    pack.active=true;pack.location=event.location;pack.expires=state.worldTime+Lifetime;pack.token=event.token;pack.lastPicker=-1;
    event.dropped=true;++state.originalEnables;++state.drops;return true;
}
inline bool InitializeImpl(void* world){
    if(!Thread()||world!=original_players::world)return false;
    if(state.initialized)return state.world==world;
    state.world=world;state.gameState=Read<void*>(world,0x178);
    if(!original_players::IsA(state.gameState,original_players::ShooterGameStateClass)||!original_effects::CaptureIdentity(world,state.worldIdentity))return Fail(2924,"deathmatch_pickups_real_world_required");
    state.packClass=LoadClass(0x6480c50,PackClassPath);state.poolClass=LoadClass(0x6480c50,PoolClassPath);
    state.ammoClass=LoadClass(0x36837a0,AmmoClassPath);
    state.pickupFx=LoadClass(0x648ada0,L"/Game/GameModes/Deathmatch/FXC_HealthPack_Pickup.FXC_HealthPack_Pickup_C");
    state.ammoFx=LoadClass(0x648ada0,L"/Game/GameModes/FXC/FXC_GameMode_AmmoReplenish.FXC_GameMode_AmmoReplenish_C");
    state.hudBuff=LoadClass(0x6481590,L"/Game/GameModes/Deathmatch/Buff_HealthPack_HUD.Buff_HealthPack_HUD_C");
    state.protectionClass=LoadClass(0x36837a0,ProtectionClass);
    auto dmClass=LoadClass(original_players::ShooterGameStateClass,L"/Game/GameModes/Deathmatch/DeathmatchGameState.DeathmatchGameState_C");
    if(!dmClass||!original_players::Derived(Read<void*>(state.gameState,0x10),dmClass)||Read<uint8_t>(state.gameState,0x224)!=3)return Fail(2939,"deathmatch_pickups_requires_actual_deathmatch_game_state");
    if(!state.packClass||!state.poolClass||!state.ammoClass||!state.pickupFx||!state.ammoFx||!state.hudBuff||!state.protectionClass||!Defaults()||!NativeBindings())return Fail(2925,"deathmatch_original_pickup_defaults_or_classes_changed");
    if(!Profile(EnableMethod,112,72,0x64)||!Profile(DisableMethod,0,18,0x46)||!Profile(OnRepEnabled,0,26,0x45)
       ||!Profile(AmmoLeaf,24,227,0x0f)||!Profile(SpawnProtection,16,72,0x64)||!Profile(ProtectionDisable,0,18,0x46))return Fail(2926,"deathmatch_original_client_graph_profile_changed");
    if(!original_effects::Initialize(world))return Fail(2927,"deathmatch_original_effect_lifecycle_missing");
    auto ruleClass=LoadClass(0x36837a0,L"/Game/GameModes/Components/Comp_GameMode_KillReload.Comp_GameMode_KillReload_C");
    if(!ruleClass)return Fail(2936,"deathmatch_original_kill_reload_class_missing");
    struct alignas(16) AddRule {void* type;uint8_t manual=0,pad[7]{};engine::Transform transform;uint8_t deferred=0,pad2[7]{};void* result=nullptr;} rule{};
    static_assert(sizeof(AddRule)==128);rule.type=ruleClass;
    // A real world-owned component holds the original rule defaults and hard
    // FX class references for GC. Its stock BeginPlay GameMode-owner cast is
    // false on this real GameState; no fake GameMode or automatic bind is made.
    if(!Event(state.gameState,L"/Script/Engine.Actor.AddComponentByClass",128,&rule)||Read<void*>(rule.result,0xc8)!=state.gameState)return Fail(2937,"deathmatch_original_rule_component_construction_failed");
    state.killReloadComponent=rule.result;
    const int mag=Field(ruleClass,L"MagazineAmmoRestored",4),reserve=Field(ruleClass,L"ReserveAmmoRestored",4);
    if(mag<0||reserve<0||Read<int32_t>(rule.result,mag)!=30||Read<int32_t>(rule.result,reserve)!=30)return Fail(2938,"deathmatch_original_kill_reload_defaults_changed");
    state.magazineRestored=Read<int32_t>(rule.result,mag);state.reserveRestored=Read<int32_t>(rule.result,reserve);
    for(size_t n=0;n<InitialPool;++n)if(!CreatePack())return false;
    state.initialized=true;state.stage="deathmatch_original_pickup_pool_initialized";return true;
}
inline bool OnKillImpl(size_t source,size_t victim,uint64_t token){
    if(!Thread()||!state.initialized||!token||source>=MaxPlayers||victim>=MaxPlayers||source==victim)return false;
    if(state.stopped)return true;
    for(auto seen:tokens)if(seen==token){++state.duplicateTokens;return true;}
    Pending* entry=nullptr;for(auto& e:pending)if(!e.used){entry=&e;break;}
    if(!entry)return Fail(2928,"deathmatch_pickup_kill_queue_full");
    auto s=original_players::players[source].character,v=original_players::players[victim].character;
    *entry={};entry->used=true;entry->token=token;entry->source=source;entry->victim=victim;
    if(!original_effects::CaptureIdentity(s,entry->sourcePawn)||!original_effects::CaptureIdentity(v,entry->victimPawn)
       ||!Event(v,L"/Script/Engine.Actor.K2_GetActorLocation",24,&entry->location))return Fail(2929,"deathmatch_original_kill_actor_context_missing");
    entry->location.z-=DropOffset;state.worldTime=Read<double>(state.world,0x740);entry->reloadAt=state.worldTime+KillReloadDelay;
    auto inventory=Read<void*>(s,0xc08);void* gun=nullptr;
    if(!Event(inventory,L"/Script/ShooterGame.AresInventory.GetCurrentEquippable",8,&gun))return false;
    if(original_players::IsA(gun,0x645c660)&&Field(Read<void*>(gun,0x10),L"MagazineAmmo",8)>=0){
        if(!original_effects::CaptureIdentity(gun,entry->gun))return false;
    }else entry->reloaded=true;
    tokens[nextToken++%tokens.size()]=token;++state.killEvents;return true;
}
inline bool ReadProtection(size_t index,float& value){
    if(index>=MaxPlayers)return false;auto& entry=original_players::players[index];
    if(!entry.ready||!original_players::IsA(entry.attributes,0x63e58b0)||!original_players::Read(entry.attributes,0x234,value)||!std::isfinite(value)){
        ++state.protectionReadFailures;return false;
    }
    ++state.protectedReads;auto& p=protections[index];p.invulnerable=value;p.protectedNow=value>0;p.observed=p.observed||p.protectedNow;return true;
}
inline bool CanDamage(size_t victim){float current=0;return Thread()&&ReadProtection(victim,current)&&current<=0;}
inline bool ApplySpawnProtectionImpl(size_t index){
    if(!Thread()||!state.initialized||index>=MaxPlayers)return false;auto pawn=original_players::players[index].character;
    auto& p=protections[index];p={};p.pawn=pawn;if(!original_effects::CaptureIdentity(pawn,p.identity))return false;
    struct Params {uint8_t fire=1,move=1,pad[6]{};double duration=5;} parameters;
    static_assert(sizeof(Params)==16);
    if(!Event(pawn,SpawnProtection,16,&parameters))return Fail(2930,"deathmatch_original_spawn_protection_entry_failed");
    p.requested=true;p.requestedAt=Read<double>(state.world,0x740);++p.applies;++state.protectionApplies;
    float actual=0;if(!ReadProtection(index,actual))return false;
    if(actual>0){++state.protectionConfirmed;return true;}
    ++state.protectionNotObserved;state.stage="deathmatch_original_spawn_protection_attribute_not_active";return false;
}
inline bool OnWeaponUsedImpl(size_t index){
    if(!Thread()||!state.initialized||index>=MaxPlayers)return false;float actual=0;if(!ReadProtection(index,actual))return false;if(actual<=0)return true;
    struct Query {void* type;void* result=nullptr;} q{state.protectionClass};auto pawn=original_players::players[index].character;
    if(!Event(pawn,L"/Script/Engine.Actor.GetComponentByClass",16,&q)||Read<void*>(q.result,0xc8)!=pawn||!Event(q.result,ProtectionDisable,0,nullptr))return false;
    if(!ReadProtection(index,actual)||actual>0)return false;
    ++state.fireCancels;++protections[index].fireCancels;return true;
}
inline bool PollImpl(double seconds){
    if(!Thread()||!state.initialized||original_effects::ResolveIdentity(state.worldIdentity)!=state.world)return false;
    if(state.stopped)return true;
    state.callerSeconds=seconds;state.worldTime=Read<double>(state.world,0x740);if(!std::isfinite(state.worldTime))return false;
    for(auto& event:pending)if(event.used){
        if(!event.dropped&&!Enable(event))return Fail(2931,"deathmatch_original_health_pack_enable_failed");
        if(!event.reloaded&&state.worldTime>=event.reloadAt){auto gun=original_effects::ResolveIdentity(event.gun);
            auto pawn=original_effects::ResolveIdentity(event.sourcePawn);
            if(gun&&pawn&&Read<void*>(gun,0x4a8)==pawn){if(!Replenish(gun,state.magazineRestored,state.reserveRestored,false)||!Fx(pawn,state.ammoFx))return Fail(2932,"deathmatch_original_kill_reload_failed");++state.killReloads;}
            else ++state.killReloadSkipped;event.reloaded=true;
        }
        if(event.reloaded&&event.dropped)event.used=false;
    }
    const bool query=state.worldTime>=nextOverlap;if(query)nextOverlap=state.worldTime+.05;
    state.active=state.pending=0;bool managers=true;
    for(size_t n=0;n<state.poolCreated;++n){auto& pack=pool[n];if(!Live(pack))return Fail(2933,"deathmatch_original_pool_actor_expired");
        if(pack.active){uint8_t collision=0;if(!Event(pack.sphere,L"/Script/Engine.PrimitiveComponent.GetCollisionEnabled",1,&collision))return false;
            if(collision==0){if(!PublishEnabled(pack,false))return false;pack.active=false;++state.expired;}
            else if(state.worldTime>=pack.expires){if(!Disable(pack,true))return false;}
            else if(query&&!Overlaps(pack))return Fail(2934,"deathmatch_original_overlap_pickup_failed");
        }
        if(pack.active)++state.active;
        managers=managers&&EffectsReady(pack.actor);
    }
    for(const auto& e:pending)if(e.used)++state.pending;
    for(size_t i=0;i<MaxPlayers;++i)if(protections[i].requested){float actual=0;if(original_effects::ResolveIdentity(protections[i].identity)==protections[i].pawn)ReadProtection(i,actual);}
    state.ready=state.poolCreated>=InitialPool&&managers;state.error=0;
    state.stage=state.ready?"deathmatch_original_pickups_running":"deathmatch_original_pickups_waiting_effect_lifecycle";return true;
}
inline bool StopImpl(){
    if(!Thread()||!state.initialized)return false;
    for(size_t n=0;n<state.poolCreated;++n)if(pool[n].active&&!Disable(pool[n]))return false;
    for(auto& event:pending)event.used=false;
    state.active=state.pending=0;state.stopped=true;state.stage="deathmatch_original_pickups_match_stopped";return true;
}
inline LONG CaptureException(EXCEPTION_POINTERS* e){
    state.error=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0002935U;
    const auto at=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;state.restartRequired=true;state.ready=false;
    state.stage="deathmatch_pickups_native_exception_requires_new_session";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* world){__try{return InitializeImpl(world);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool OnKill(size_t source,size_t victim,uint64_t token){__try{return OnKillImpl(source,victim,token);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool Poll(double seconds){__try{return PollImpl(seconds);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool ApplySpawnProtection(size_t index){__try{return ApplySpawnProtectionImpl(index);}__except(CaptureException(GetExceptionInformation())){return false;}}
// Call only for an actual original equipped-item use/shot, if its automatic
// original OnAuthUsedEquippable delegate was observed to be missing locally.
inline bool OnWeaponUsed(size_t index){__try{return OnWeaponUsedImpl(index);}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool Stop(){__try{return StopImpl();}__except(CaptureException(GetExceptionInformation())){return false;}}
inline bool WriteDiagnostics(const wchar_t* path){
    FILE* file=nullptr;if(!path||_wfopen_s(&file,path,L"wb")||!file)return false;
    const auto b=[](bool value){return value?"true":"false";};
    std::fputs("{\"schema_version\":1,\"stage\":",file);original_players::Text(file,state.stage);
    std::fprintf(file,",\"error\":%u,\"initialized\":%s,\"ready\":%s,\"stopped\":%s,\"restart_required\":%s,\"defaults_verified\":%s,\"bool_layout_verified\":%s,\"native_bindings_verified\":%s,\"world_time\":%.9g,\"pool_created\":%u,\"initial_pool_size\":%zu,\"pool_capacity\":%zu,\"active\":%u,\"pending\":%u,\"kill_events\":%u,\"duplicates\":%u,\"drops\":%u,\"pickups\":%u,\"expired\":%u,\"overlap_queries\":%u,\"overlap_actors\":%u,\"recovery_deferred\":%u,\"recovery_rejected\":%u,\"ammo_leaf_calls\":%u,\"ammo_units_added\":%u,\"kill_reloads\":%u,\"kill_reload_skipped\":%u,\"pickup_reloads\":%u,\"fx_requests\":%u,\"hud_buff_requests\":%u,\"hud_buff_handles\":%u,\"authority_enabled_writes\":%u,\"original_enables\":%u,\"original_disables\":%u,\"protection_applies\":%u,\"protection_confirmed\":%u,\"protection_not_observed\":%u,\"fire_cancels\":%u,\"protection_read_failures\":%u,\"last_ammo_readback_valid\":%s,\"last_magazine_before\":%d,\"last_magazine_after\":%d,\"last_reserve_before\":%d,\"last_reserve_after\":%d,\"exception_rva\":%llu,\"failed_event\":",
        state.error,b(state.initialized),b(Ready()),b(state.stopped),b(state.restartRequired),b(state.defaultsVerified),b(state.boolLayoutVerified),b(state.nativeBindingsVerified),state.worldTime,
        state.poolCreated,InitialPool,MaxPool,state.active,state.pending,state.killEvents,state.duplicateTokens,state.drops,state.pickups,state.expired,
        state.overlapQueries,state.overlapActors,state.recoveryDeferred,state.recoveryRejected,state.ammoLeafCalls,state.ammoUnitsAdded,state.killReloads,state.killReloadSkipped,state.pickupReloads,state.fxRequests,state.hudBuffRequests,state.hudBuffHandles,
        state.authorityEnabledWrites,state.originalEnables,state.originalDisables,state.protectionApplies,state.protectionConfirmed,state.protectionNotObserved,state.fireCancels,state.protectionReadFailures,b(state.lastAmmoReadback),state.lastMagazineBefore,state.lastMagazineAfter,state.lastReserveBefore,state.lastReserveAfter,
        static_cast<unsigned long long>(state.exceptionRva));
    const auto event=original_players::Utf8(state.failedEvent);original_players::Text(file,event.c_str());
    std::fprintf(file,",\"expected_size\":%u,\"actual_size\":%u,\"physics_overlap_only\":true,\"changes_net_mode\":false,\"changes_original_scripts\":false,\"pool\":[",state.expectedSize,state.actualSize);
    for(size_t n=0;n<state.poolCreated;++n){const auto& p=pool[n];if(n)std::fputc(',',file);
        std::fprintf(file,"{\"actor\":\"%p\",\"sphere\":\"%p\",\"sphere_radius\":%.9g,\"live\":%s,\"active\":%s,\"effects_ready\":%s,\"original_enabled\":%u,\"location\":[%.9g,%.9g,%.9g],\"expires\":%.9g,\"token\":%llu,\"last_picker\":%d}",
            p.actor,p.sphere,static_cast<double>(p.sphereRadius),b(Live(p)),b(p.active),b(EffectsReady(p.actor)),Live(p)?static_cast<unsigned>(Read<uint8_t>(p.actor,state.enabledOffset)):0,
            p.location.x,p.location.y,p.location.z,p.expires,static_cast<unsigned long long>(p.token),p.lastPicker);
    }
    std::fputs("],\"protection\":[",file);bool comma=false;
    for(size_t n=0;n<MaxPlayers;++n){const auto& p=protections[n];if(!p.pawn&&!p.requested)continue;if(comma)std::fputc(',',file);comma=true;
        std::fprintf(file,"{\"index\":%zu,\"pawn\":\"%p\",\"requested\":%s,\"ever_observed_active\":%s,\"protected\":%s,\"invulnerable_current_value\":%.9g,\"requested_at\":%.9g,\"fire_cancels\":%u}",
            n,p.pawn,b(p.requested),b(p.observed),b(p.protectedNow),static_cast<double>(p.invulnerable),p.requestedAt,p.fireCancels);
    }
    std::fputs("]}\n",file);const bool good=std::ferror(file)==0;std::fclose(file);return good;
}
} // namespace deathmatch_pickups
