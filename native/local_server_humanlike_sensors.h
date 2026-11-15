#pragma once
#include "armor_poll_performance.h"
#include "humanlike_bot_sensor_settings.h"
#include "local_server_combat.h"
#include "local_server_npc_input.h"
#include "local_server_armor.h"
#include "humanlike_bot_weapon_feedback.h"
#include "local_server_humanlike_blind.h"
#include <array>
#include <cmath>

// Candidate host adapter. Install only in the existing match TickBots observation
// boundary through a callback owned by runtime (avoids match/combat include cycles).
// No original actor/Role/input/clock/physics state is written by this adapter.
namespace local_server_humanlike_sensors {
namespace dto=humanlike_bot;
namespace core=humanlike_bot_sensors;
namespace cfg=humanlike_bot_config;
using Weak=original_inventory_slots::WeakIdentity;
using ProfileReader=const cfg::Profile*(*)(size_t index);
using SpaceProvider=bool(*)(size_t index,double now,const dto::OwnSnapshot&,dto::SpaceHints&);
inline constexpr size_t MaxPlayers=original_players::MaxPlayers;
static_assert(MaxPlayers==core::MaxPlayers);
struct SelfCache {
    void *pawn=nullptr,*camera=nullptr,*movement=nullptr,*weapon=nullptr;Weak pawnId{},cameraId{},movementId{},weaponId{};uint32_t epoch=0;
    dto::OwnSnapshot own{};double nextOwn=0,nextWeapon=0;
    local_server_humanlike_blind::Cache blind{};
    float cameraTime=0;bool cameraTimeKnown=false,nativeFalling=false;uint64_t lastGapOrigin=0;
};
struct State {
    bool initialized=false,profilesChecked=false,profilesReady=false;void* world=nullptr;Weak worldId{};
    ProfileReader readProfile=nullptr;SpaceProvider spaceProvider=nullptr;
    core::FeedbackPass feedbackPass=nullptr;void* feedbackContext=nullptr;
    uint64_t passes=0,ownSamples=0,weaponSamples=0,groundSamples=0,groundUnknown=0;
    uint64_t realCameraSamples=0,eyeViewSamples=0,unknownViews=0,traceQueries=0,traceFailures=0;
    uintptr_t exceptionRva=0;unsigned exception=0;
};
inline State state{};inline core::Provider provider;
inline std::array<SelfCache,MaxPlayers> selves{};
inline std::array<core::Pose,MaxPlayers> poses{};
inline std::array<dto::OwnSnapshot,MaxPlayers> own{};
inline std::array<core::Settings,MaxPlayers> settings{};
inline void *groundMethod=nullptr,*fallingMethod=nullptr,*crouchMethod=nullptr,*cameraRotationMethod=nullptr,*cameraLocationMethod=nullptr,*cameraFovMethod=nullptr,*traceMethod=nullptr;
template<class T>inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline bool Live(void* object,const Weak&id){return object&&original_inventory_slots::ResolveIdentity(id)==object;}
inline dto::Vec3 Vec(const engine::Vec&v){return {static_cast<float>(v.x),static_cast<float>(v.y),static_cast<float>(v.z)};}
inline engine::Vec Native(const dto::Vec3&v){return {v.x,v.y,v.z};}
inline uint64_t OwnToken(const Weak&id){return uint64_t(static_cast<uint32_t>(id.index))<<32|static_cast<uint32_t>(id.serial);}
inline bool Context(){return state.initialized&&original_players::Thread()&&Live(state.world,state.worldId)
    &&local_server_match::GetState().world==state.world&&!local_server_match::GetState().restartRequired;}
inline bool Profiles(){
    if(state.profilesChecked)return state.profilesReady;state.profilesChecked=true;
    struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
    // Complete fixed PE functions; source/runtime-profiles.json records ranges.
    constexpr Profile profiles[]{
        {0x3e98f20,54,0xd0a82f8d610db8fbULL},{0x41844e0,54,0x345f8a80df5fce9cULL},
        {0x4191860,22,0x242df58baab1d627ULL},{0x4191840,26,0x0f55546c661496a8ULL},
        {0x3e7a190,73,0x8cb91103b95949a2ULL},{0x3e7a1e0,73,0x92aaa979ea98db53ULL},{0x3e7a270,56,0x69efcd827c6107bbULL},
        {0x41e1880,54,0xac3cdcda36da1144ULL},{0x7899940,112,0x8dcfcb15a369d54dULL},
        {0x4856b00,117,0x353dcfd3b86c2647ULL},{0x787f9f0,23,0x6476ea846cfe8e08ULL},
        {0x787ff80,169,0xa367691188616272ULL}
    };
    for(const auto&p:profiles){const auto bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes))return false;uint64_t h=14695981039346656037ULL;
        for(size_t n=0;n<p.bytes;++n)h=(h^bytes[n])*1099511628211ULL;if(h!=p.fnv)return false;}
    state.profilesReady=true;return true;
}
inline bool Method(void*&cache,const wchar_t*name,uint16_t bytes,uintptr_t exec,uintptr_t owner){
    if(!cache)cache=engine::Find(name);
    return cache&&Read<uint16_t>(cache,0xbe)==bytes&&Read<void*>(cache,0xe0)==engine::function<void*>(exec)
        &&Read<void*>(cache,0x28)==engine::Class(owner)&&(Read<uint32_t>(cache,0xb8)&0x400U)!=0;
}
inline bool Ground(void* movement,bool&grounded,bool&falling){
    if(!Profiles()||!original_players::IsA(movement,0x6c7d380))return false;
    const auto table=Read<void*>(movement);
    if(Read<void*>(table,0x620)!=engine::function<void*>(0x4191860)||Read<void*>(table,0x628)!=engine::function<void*>(0x4191840))return false;
    if(!Method(groundMethod,L"/Script/Engine.NavMovementComponent.IsMovingOnGround",1,0x41844e0,0x41c1810)
       ||!Method(fallingMethod,L"/Script/Engine.NavMovementComponent.IsFalling",1,0x3e98f20,0x41c1810))return false;
    uint8_t ground=0,fall=0;
    if(!original_damage::EventWithFunction(movement,L"/Script/Engine.NavMovementComponent.IsMovingOnGround",groundMethod,1,&ground)
       ||!original_damage::EventWithFunction(movement,L"/Script/Engine.NavMovementComponent.IsFalling",fallingMethod,1,&fall)
       ||ground>1||fall>1||(ground&&fall))return false;
    grounded=ground!=0;falling=fall!=0;++state.groundSamples;return true;
}
inline bool Crouch(void*movement,bool&crouched){
    if(!Profiles()||!original_players::IsA(movement,0x6c7d380)
       ||Read<void*>(Read<void*>(movement),0x618)!=engine::function<void*>(0x7899940)
       ||!Method(crouchMethod,L"/Script/Engine.NavMovementComponent.IsCrouching",1,0x41e1880,0x41c1810))return false;
    uint8_t value=0;if(!original_damage::EventWithFunction(movement,L"/Script/Engine.NavMovementComponent.IsCrouching",crouchMethod,1,&value)||value>1)return false;
    crouched=value!=0;return true;
}
inline void* Movement(const local_server_match::AuthorityPlayer&p,SelfCache&cache){
    const auto movement=Read<void*>(p.pawn,0x4f8);
    if(!original_players::IsA(movement,0x6c7d380)||Read<void*>(movement,0x190)!=p.pawn||Read<void*>(movement,0x1a8)!=p.pawn)return nullptr;
    if(cache.movement!=movement||!Live(movement,cache.movementId)){
        cache.movement=movement;cache.movementId={};
        if(!original_inventory_slots::CaptureIdentity(movement,cache.movementId)||!Live(movement,cache.movementId))return nullptr;
    }
    return movement;
}
inline bool NativeCamera(const local_server_match::AuthorityPlayer&p,SelfCache&cache,dto::OwnSnapshot&out){
    auto camera=Read<void*>(p.pc,0x528);
    if(!Profiles()||!original_players::IsA(camera,original_camera::CameraClass)
       ||Read<void*>(camera,0x468)!=p.pc||Read<void*>(camera,0x580)!=p.pawn||Read<void*>(camera,0xe80))return false;
    // The same exact cache getter profile already used by original_ads::Sample.
    if(Read<void*>(Read<void*>(camera),0x7a0)!=engine::function<void*>(0x4914b80))return false;
    if(cache.camera!=camera||!Live(camera,cache.cameraId)){
        cache.camera=camera;cache.cameraId={};cache.cameraTimeKnown=false;
        if(!original_inventory_slots::CaptureIdentity(camera,cache.cameraId)||!Live(camera,cache.cameraId))return false;
    }
    const float time=Read<float>(camera,0x17b0);if(!std::isfinite(time))return false;
    const bool advanced=cache.cameraTimeKnown&&time>cache.cameraTime;cache.cameraTime=time;cache.cameraTimeKnown=true;
    if(!advanced)return false; // no fake camera tick or stale nonlocal cache
    if(!Method(cameraRotationMethod,L"/Script/Engine.PlayerCameraManager.GetCameraRotation",24,0x3e7a1e0,0x3e75c30)
       ||!Method(cameraLocationMethod,L"/Script/Engine.PlayerCameraManager.GetCameraLocation",24,0x3e7a190,0x3e75c30)
       ||!Method(cameraFovMethod,L"/Script/Engine.PlayerCameraManager.GetFOVAngle",4,0x3e7a270,0x3e75c30))return false;
    engine::Vec rotation{},location{};float fov=0;
    if(!original_damage::EventWithFunction(camera,L"/Script/Engine.PlayerCameraManager.GetCameraRotation",cameraRotationMethod,24,&rotation)
       ||!original_damage::EventWithFunction(camera,L"/Script/Engine.PlayerCameraManager.GetCameraLocation",cameraLocationMethod,24,&location)
       ||!original_damage::EventWithFunction(camera,L"/Script/Engine.PlayerCameraManager.GetFOVAngle",cameraFovMethod,4,&fov)
       ||!original_players::Finite(rotation)||!original_players::Finite(location)||!std::isfinite(fov)||fov<=0||fov>180)return false;
    out.eye=Vec(location);out.view={static_cast<float>(rotation.x),static_cast<float>(rotation.y)};
    out.viewSampleTimeKnown=true;out.viewSampledAtSeconds=out.nowSeconds;
    out.eyeValid=out.viewValid=out.nativeViewFovKnown=true;out.nativeHorizontalFovDegrees=fov;++state.realCameraSamples;return true;
}
inline bool ReadOwn(size_t index,double now,const cfg::Profile&profile){
    const auto p=local_server_match::Player(index);if(!p||p->human||!p->present||!Live(p->pawn,p->pawnIdentity)
       ||!Live(p->pc,p->pcIdentity)||!Live(p->ps,p->psIdentity)
       ||Read<void*>(p->pawn,0x4a0)!=p->pc||Read<void*>(p->pc,0x4b0)!=p->pawn)return false;
    auto&cache=selves[index];const auto&a=training_bot_controller::actors[index];const auto&o=a.observed;
    if(cache.pawn!=p->pawn||cache.epoch!=p->epoch||!original_inventory_slots::SameIdentity(cache.pawnId,p->pawnIdentity)){
        if(cache.pawn)humanlike_bot_weapon_feedback::RetireOwnLife(index,cache.pawn,cache.epoch);
        cache={};cache.pawn=p->pawn;cache.pawnId=p->pawnIdentity;cache.epoch=p->epoch;
    }
    auto&out=cache.own;out.life={static_cast<uint32_t>(index),p->epoch,OwnToken(p->pawnIdentity)};out.nowSeconds=now;
    out.alive=p->alive;out.paused=!local_server_match::Active()||(local_server_match::Configuration().config.flags&training_control::Paused)!=0;
    out.retired=false;
    if(now>=cache.nextOwn){
        cache.nextOwn=now+cfg::Get(profile,cfg::Param::OwnSampleMs)/1000.;++state.ownSamples;
        const auto weapon=out.weapon;out={};out.weapon=weapon;out.life={static_cast<uint32_t>(index),p->epoch,OwnToken(p->pawnIdentity)};
        out.shootingViewRequired=true;
        out.nowSeconds=now;out.alive=p->alive;out.paused=!local_server_match::Active()||(local_server_match::Configuration().config.flags&training_control::Paused)!=0;
        if(a.bound&&a.pawn==p->pawn&&a.epoch==p->epoch&&o.ready&&o.pawn==p->pawn&&o.spawnEpoch==p->epoch&&o.controller==p->pc){
            out.valid=true;out.feet=Vec(o.feet);out.velocity=Vec(o.velocity);out.eye=Vec(o.eye);out.eyeValid=true;
            out.sampledAtSeconds=(std::min)(p->lastSample,local_server_match::nextObservation-.05);
            out.movementValid=o.speedProbeReady;
            out.walking=o.movementType==0;out.nativeMaxSpeedCmS=static_cast<float>(o.crouched?o.crouchSpeed:(out.walking?o.walkSpeed:o.runSpeed));
            out.healthKnown=std::isfinite(p->health)&&std::isfinite(p->maxHealth)&&p->maxHealth>0;
            if(out.healthKnown){out.health=p->health;out.maxHealth=p->maxHealth;}
            const auto armor=local_server_armor::Armor(index);
            if(armor&&armor->used&&!armor->retired&&armor->ready&&armor->readback&&armor->pawn==p->pawn&&armor->epoch==p->epoch
               &&original_inventory_slots::SameIdentity(armor->pawnId,p->pawnIdentity)){
                float shield=0,maxShield=0;
                out.shieldKnown=training_bot_controller::Event(p->pawn,L"/Script/ShooterGame.ShooterCharacter.GetShield",4,&shield)
                    &&training_bot_controller::Event(p->pawn,L"/Script/ShooterGame.ShooterCharacter.GetMaxShield",4,&maxShield)
                    &&std::isfinite(shield)&&std::isfinite(maxShield)&&shield>=0&&maxShield>=shield;
                if(out.shieldKnown){out.ownShield=shield;out.maxShield=maxShield;}
            }
            struct Eyes {engine::Vec location{},rotation{};} eyes;
            if(training_bot_controller::Event(p->pawn,L"/Script/Engine.Actor.GetActorEyesViewPoint",48,&eyes)
               &&original_players::Finite(eyes.location)&&original_players::Finite(eyes.rotation)){
                out.eye=Vec(eyes.location);out.view={static_cast<float>(eyes.rotation.x),static_cast<float>(eyes.rotation.y)};
                out.viewSampleTimeKnown=true;out.viewSampledAtSeconds=now;
                out.viewValid=true;++state.eyeViewSamples;
                const auto table=Read<void*>(p->pawn);
                if(Profiles()&&Read<void*>(table,0x728)==engine::function<void*>(0x4856b00)
                   &&Read<void*>(table,0x848)==engine::function<void*>(0x787f9f0)){
                    out.shootingView=out.view;out.shootingViewKnown=true;
                    out.shootingViewSampledAtSeconds=now;
                }
            }else ++state.unknownViews;
            NativeCamera(*p,cache,out);
            const auto blind=local_server_humanlike_blind::Sample(index,p->pawn,p->pawnIdentity,p->epoch,state.world,state.worldId,cache.blind);
            out.nativeBlindnessKnown=blind.known;out.nativeBlinded=blind.known&&blind.active;
            bool falling=false;auto movement=Movement(*p,cache);
            if(movement){
                out.groundedKnown=Ground(movement,out.grounded,falling);out.stanceValid=Crouch(movement,out.crouched);}
            cache.nativeFalling=falling;
            out.nativeMaxSpeedCmS=static_cast<float>(out.crouched?o.crouchSpeed:(out.walking?o.walkSpeed:o.runSpeed));
            if(!out.groundedKnown)++state.groundUnknown;
            const auto*npc=local_server_npc_input::Player(index);
            if(npc&&npc->pawn==p->pawn&&npc->epoch==p->epoch&&npc->pawnId.index==p->pawnIdentity.index&&npc->pawnId.serial==p->pawnIdentity.serial){
                out.nativeFrame=npc->index;out.jumpObserved=falling&&o.velocity.z>0&&npc->latest.jump;
                out.crouchObserved=out.stanceValid&&out.crouched&&npc->latest.crouch;
                out.moveObserved=core::Length(out.velocity)>0&&(npc->captures||npc->nativeCaptureObservations);
                // Existing transport has no per-Intent serial receipt. Do not
                // invent a correlation from submission/clock counters.
                out.consumptionKnown=false;out.lastConsumedIntentSerial=0;
            }
        }
    }
    const auto*source=local_server_combat::firearms::Player(index);
    const auto currentGun=Read<void*>(Read<void*>(p->pawn,0xc08),0x280);
    const bool sourceMatches=source&&source->bound&&source->pawn==p->pawn&&source->spawnEpoch==p->epoch
        &&original_inventory_slots::SameIdentity(source->pawnId,p->pawnIdentity)
        &&source->weapon==currentGun&&Live(currentGun,source->weaponId);
    const bool cachedWeaponMatches=sourceMatches&&cache.weapon==currentGun&&Live(cache.weapon,cache.weaponId)
        &&original_inventory_slots::SameIdentity(cache.weaponId,source->weaponId)&&out.weapon.binding==source->binding;
    if(!cachedWeaponMatches){out.weapon={};cache.nextWeapon=now;cache.weapon=nullptr;cache.weaponId={};}
    if(now>=cache.nextWeapon){cache.nextWeapon=now+cfg::Get(profile,cfg::Param::WeaponSampleMs)/1000.;++state.weaponSamples;
        humanlike_bot_weapon_feedback::ReadWeaponFeedback(index,p->pawn,p->epoch,now,out.weapon);
        if(sourceMatches&&out.weapon.valid&&out.weapon.binding==source->binding){cache.weapon=currentGun;cache.weaponId=source->weaponId;}}
    own[index]=out;return true;
}
inline core::Ray WorldTrace(uint32_t,const dto::Vec3&from,const dto::Vec3&to,void*){
    if(!Context()||!core::Finite(from)||!core::Finite(to))return core::Ray::Unknown;
    if(!traceMethod)traceMethod=engine::Find(L"/Script/Engine.KismetSystemLibrary.LineTraceSingle");
    if(!traceMethod||Read<uint16_t>(traceMethod,0xbe)!=0x171||Read<void*>(traceMethod,0x28)!=engine::Class(0x45aa110))return core::Ray::Unknown;
    alignas(16) uint8_t params[0x178]{};original_players::Put(params,0,state.world);
    original_players::Put(params,8,Native(from));original_players::Put(params,0x20,Native(to));params[0x39]=params[0x148]=1;
    std::array<void*,128> ignored{};original_players::Put(params,0x40,training_world_geometry::Ignore(ignored));++state.traceQueries;
    if(!original_damage::EventWithFunction(engine::DefaultObject(engine::Class(0x45aa110)),L"/Script/Engine.KismetSystemLibrary.LineTraceSingle",traceMethod,0x171,params)){
        ++state.traceFailures;return core::Ray::Unknown;}
    if(!params[0x170]||!(params[0x105]&1))return core::Ray::Clear;
    const float distance=Read<float>(params,0x60);if(!std::isfinite(distance)||distance<0){++state.traceFailures;return core::Ray::Unknown;}
    return distance>=core::Length(core::Sub(to,from))-1?core::Ray::Clear:core::Ray::Blocked;
}
inline bool Initialize(uint64_t seed,ProfileReader reader,SpaceProvider space=nullptr){
    if(!original_players::Thread()||!reader)return false;const auto&match=local_server_match::GetState();
    if(!match.initialized||!match.world)return false;state={};state.world=match.world;
    if(!original_inventory_slots::CaptureIdentity(state.world,state.worldId)||!Live(state.world,state.worldId))return false;
    for(auto&self:selves)self={};for(auto&pose:poses)pose={};for(auto&o:own)o={};
    local_server_humanlike_blind::Reset();provider.Reset(seed);state.readProfile=reader;state.spaceProvider=space;state.initialized=true;return true;
}
inline bool VisibleShotSourceLive(uint32_t observer,const core::Identity&ownIdentity,const core::Identity&targetIdentity,uint64_t binding,void*){
    if(!Context()||observer==0||observer>=MaxPlayers||targetIdentity.index>=MaxPlayers||targetIdentity.index==observer||!binding)return false;
    const auto same=[&](const core::Identity&id,const local_server_match::AuthorityPlayer*p){
        return p&&p->present&&p->alive&&p->epoch==id.epoch&&reinterpret_cast<uintptr_t>(p->pawn)==id.pawn
            &&p->pawnIdentity.index==id.weakIndex&&p->pawnIdentity.serial==id.weakSerial&&Live(p->pawn,p->pawnIdentity);};
    if(ownIdentity.index!=observer||!same(ownIdentity,local_server_match::Player(observer))
       ||!same(targetIdentity,local_server_match::Player(targetIdentity.index)))return false;
    if(local_server_match::IsObserver(targetIdentity.index))return false;
    const auto*source=local_server_combat::firearms::Player(targetIdentity.index);
    const auto*p=local_server_match::Player(targetIdentity.index);
    const auto current=Read<void*>(Read<void*>(p->pawn,0xc08),0x280);
    return source&&source->bound&&source->binding==binding&&source->spawnEpoch==p->epoch&&source->pawn==p->pawn
        &&original_inventory_slots::SameIdentity(source->pawnId,p->pawnIdentity)&&source->weapon==current&&Live(current,source->weaponId)
        &&local_server_combat::firearms::SourceIdentity(*source);
}
inline bool PassImpl(double now){
    armor_poll_performance::Scope measurement(armor_poll_performance::Slot::SensorPass);
    if(!Context()||!std::isfinite(now)||now<0||!state.readProfile)return false;
    const auto*global=state.readProfile(0);if(!global)return false;
    for(size_t n=0;n<MaxPlayers;++n){poses[n]={};own[n]={};settings[n]={};const auto p=local_server_match::Player(n);
        if(!p||!p->present)continue;auto&pose=poses[n];
        pose.targetEligible=!local_server_match::IsObserver(n);
        pose.identity={static_cast<uint32_t>(n),p->epoch,reinterpret_cast<uintptr_t>(p->pawn),p->pawnIdentity.index,p->pawnIdentity.serial};
        const auto&a=training_bot_controller::actors[n];const auto&o=a.observed;
        if(Live(p->pawn,p->pawnIdentity)&&a.bound&&a.pawn==p->pawn&&a.epoch==p->epoch&&o.ready&&o.spawnEpoch==p->epoch){
            pose.valid=true;pose.alive=p->alive&&o.alive;pose.eye=Vec(o.eye);pose.feet=Vec(o.feet);
            pose.radiusCm=static_cast<float>(o.capsuleRadius);pose.halfHeightCm=static_cast<float>(o.capsuleHalfHeight);
            pose.view={static_cast<float>(o.rotation.x),static_cast<float>(o.rotation.y)};pose.viewValid=original_players::Finite(o.rotation);
            pose.sampledAtSeconds=local_server_match::nextObservation-.05;
        }
        if(pose.valid&&pose.alive){const auto*source=local_server_combat::firearms::Player(n);
            const auto current=Read<void*>(Read<void*>(p->pawn,0xc08),0x280);
            if(source&&source->bound&&source->pawn==p->pawn&&source->spawnEpoch==p->epoch
               &&original_inventory_slots::SameIdentity(source->pawnId,p->pawnIdentity)&&source->weapon==current&&Live(current,source->weaponId)){
                pose.weaponBindingKnown=source->binding!=0;pose.weaponBinding=source->binding;}}
        if(n==0||p->human){
            if(pose.valid){auto&cache=selves[n];
                if(cache.pawn!=p->pawn||cache.epoch!=p->epoch||!original_inventory_slots::SameIdentity(cache.pawnId,p->pawnIdentity)){
                    cache={};cache.pawn=p->pawn;cache.epoch=p->epoch;cache.pawnId=p->pawnIdentity;}
                if(now>=cache.nextOwn){cache.nextOwn=now+cfg::Get(*global,cfg::Param::OwnSampleMs)/1000.;
                    auto movement=Movement(*p,cache);cache.own.groundedKnown=false;cache.own.stanceValid=false;
                    if(movement){
                        cache.own.groundedKnown=Ground(movement,cache.own.grounded,cache.nativeFalling);
                        cache.own.stanceValid=Crouch(movement,cache.own.crouched);}}
                pose.postureKnown=cache.own.groundedKnown&&cache.own.stanceValid;pose.crouched=cache.own.crouched;pose.airborne=cache.nativeFalling;
            }continue;
        }
        const auto*profile=state.readProfile(n);if(!profile)continue;
        settings[n]=core::SettingsFromProfile(*profile);if(!ReadOwn(n,now,*profile))continue;
        pose.eye=own[n].eye;pose.view=own[n].view;pose.viewValid=own[n].viewValid;
        pose.postureKnown=own[n].stanceValid&&own[n].groundedKnown;pose.crouched=own[n].crouched;pose.airborne=selves[n].nativeFalling;
        pose.nativeFovKnown=own[n].nativeViewFovKnown;pose.nativeHorizontalFovDegrees=own[n].nativeHorizontalFovDegrees;
    }
    provider.Pass(now,poses,own,settings,core::BudgetFromProfile(*global),{WorldTrace,nullptr,state.feedbackPass,state.feedbackContext,VisibleShotSourceLive});
    for(size_t n=1;n<MaxPlayers;++n)if(own[n].valid){
        const auto*source=local_server_combat::firearms::Player(n);const auto*profile=state.readProfile(n);
        if(!source||!profile||!source->bound||source->spawnEpoch!=own[n].life.spawnEpoch||source->binding!=own[n].weapon.binding)continue;
        const double timeout=cfg::Get(*profile,cfg::Param::FireShotReceiptTimeoutMs)/1000.;
        for(const auto&origin:source->origins)if(origin.valid&&!origin.sensorQueued&&origin.sequence>selves[n].lastGapOrigin
            &&now>=origin.at&&now-origin.at>timeout){selves[n].lastGapOrigin=origin.sequence;provider.FlagShotGap(n);}
    }
    if(state.spaceProvider)for(size_t n=1;n<MaxPlayers;++n)if(own[n].valid){dto::SpaceHints space{};
        if(state.spaceProvider(n,now,own[n],space))provider.SetSpace(n,space);}
    humanlike_bot_weapon_feedback::CacheDiagnostics(now);++state.passes;return true;
}
inline void SetFeedbackPass(core::FeedbackPass callback,void*context=nullptr){if(Context()){state.feedbackPass=callback;state.feedbackContext=context;}}
inline LONG Capture(EXCEPTION_POINTERS*e){
    state.exception=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0007700U;
    const auto at=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Pass(double now){__try{return PassImpl(now);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool Reseed(uint64_t seed){if(!Context())return false;provider.Reseed(seed);return true;}
inline const dto::SensorFrame* Frame(size_t index){return Context()?provider.Frame(index):nullptr;}
inline const State& GetState(){return state;}
} // namespace local_server_humanlike_sensors
