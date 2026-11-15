#pragma once
#include "original_enemies.h"
#include "training_control.h"
#include "original_npc_facing.h"
#include "training_local_posture_probe.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

// Authorized local decisions drive real hero inputs. This is not transplanted
// tactical AI. Original movement/physics/stances/AnimBP and root-owned original
// weapon, damage and respawn code retain their responsibilities.
namespace training_bot_controller {
using Vec=engine::Vec;
inline constexpr size_t MaxActors=original_players::MaxPlayers,MaxRoutes=64,MaxPath=64,MaxAnchors=128;
inline constexpr unsigned DeathmatchKills=40;
inline constexpr double DeathmatchSeconds=540;
enum class Mode:uint8_t {HoldAngle,Deathmatch};
enum class Style:uint8_t {JumpWide,Wide,Small,Crouch,Peek,Random,Global,WideCrouch,SmallCrouch};
enum class Phase:uint8_t {ToStart,Waiting,Exposing,Holding,Withdrawing,Returning,Blocked,Roaming,Engaging,Inactive};
struct Route {uint32_t id=0;char mapKey[96]{};Vec start{},expose{},retreat{};bool enabled=true;Style style=Style::Global;double weight=1;};
enum class RouteCheckStatus:uint8_t {Pending,Valid,Blocked};
enum class RouteCheckMethod:uint8_t {None,OriginalNavigation,OriginalCapsuleAndGround};
inline constexpr size_t MaxValidationSegments=5;
struct SegmentValidation {
    RouteCheckStatus status=RouteCheckStatus::Pending;RouteCheckMethod method=RouteCheckMethod::None;
    bool navAttempted=false,navPartial=false,localAttempted=false;
    int32_t navPointCount=0;double navStartErrorCm=-1,navEndErrorCm=-1;
    Vec from{},to{};const char* reason="not_checked";const char* navReason="not_queried";
};
struct RouteValidation {
    uint32_t id=0;uint64_t request=0,validatorEpoch=0;bool requested=false,complete=false;
    RouteCheckStatus status=RouteCheckStatus::Pending;size_t validatorIndex=MaxActors,segmentCount=3;
    int failedSegment=-1;double checkedAt=-1,capsuleRadius=0,capsuleHalfHeight=0;
    Style effectiveStyle=Style::Random;
    std::array<SegmentValidation,MaxValidationSegments> segments{};
    const char* reason="route_not_validated";
};
struct RouteValidationProgress {
    uint64_t request=0;bool active=false,complete=false;size_t total=0,checked=0,valid=0,blocked=0,pending=0,cursor=0;
    size_t preferredValidator=MaxActors;double requestedAt=-1,completedAt=-1;
    const char* reason="route_validation_not_requested";
};
struct Config {
    Mode mode=Mode::HoldAngle;Style style=Style::Random;
    bool active=false,returnFire=false,localInvincible=false;
    double peeksPerMinute=24,strength=.5,strengthSpread=.3,arrivalCm=18,stallSeconds=1.8;
    double localSegmentLimitCm=1800,maxLocalHeightChangeCm=60,sightRangeCm=6000;
    uint32_t seed=0x51f15eadu;
    double reactionMs=320,reactionSpreadMs=100,aimSpeed=280,aimSpeedSpread=100;
    double accuracy=.6,accuracySpread=.2,movement=.55,movementSpread=.25;
};
struct Observation {
    bool ready=false,alive=false,crouched=false,jumping=false,playerInputPresent=false,speedProbeReady=false;
    void *pawn=nullptr,*controller=nullptr;
    uint64_t spawnEpoch=0;Vec feet{},navAgent{},eye{},rotation{},velocity{};
    double capsuleHalfHeight=0,capsuleRadius=0,walkSpeed=0,runSpeed=0,crouchSpeed=0,jumpVelocity=0;
    int ammo=-1,reserve=-1;uint8_t movementType=0;
};
struct Frame {Vec worldDirection{},rotation{};bool walk=false,crouch=false,jump=false,fire=false,reload=false;};
struct Hooks {
    bool (*observe)(size_t,Observation&)=nullptr;
    bool (*visible)(size_t,size_t,const Vec&,const Vec&)=nullptr;
    // Must test original capsule clearance AND supporting ground/height, not
    // merely test endpoints or draw a line on the map image.
    bool (*validateSegment)(size_t,const Vec&,const Vec&,double,double)=nullptr;
    // Edges/held states go to the real per-actor original weapon input adapter.
    // No shot, hit, damage, health or respawn is fabricated here.
    bool (*weaponInput)(size_t,bool,bool)=nullptr;
    bool (*targetAllowed)(size_t source,size_t target)=nullptr;
};
struct Profile {double strength=0,reaction=0,turnSpeed=0,errorDegrees=0,burst=0,pause=0,headProbability=0,accuracy=0,movement=0;};
struct Actor {
    bool bound=false,bot=false,requestedJump=false,navPath=false,localPath=false,visible=false,combatCrouch=false;
    bool spawnRouteHintPending=false,spawnRouteHintCompleted=false;uint32_t spawnRouteHintId=0;
    uint64_t epoch=0;void* pawn=nullptr;uint32_t rng=1,cycles=0,blocked=0,navQueries=0,inputFrames=0,rosterIndex=0,profileSeed=0;
    bool nativeInputReady=false,localInputUnchanged=true;uint32_t inputInitializations=0,inputTransports=0,inputEvents=0,jumpSetterCalls=0,jumpQueueCalls=0;
    uint32_t crouchRequestedSamples=0,crouchObservedSamples=0,jumpRequestedSamples=0,jumpObservedSamples=0;
    int route=-1,target=-1;uint64_t targetEpoch=0;Style style=Style::Wide;Phase phase=Phase::Inactive;
    Profile profile;Observation observed;Frame output;
    Vec goal{},lastPosition{},bias{};std::array<Vec,MaxPath> path{};size_t pathCount=0,pathAt=0;
    double nextDecision=0,lastTick=-1,phaseSince=0,waitUntil=0,cycleStart=0,jumpUntil=0;
    double reactionUntil=0,burstUntil=0,nextBurst=0,nextBias=0,nextMove=0,nextPlan=0,nextSense=0;
    double lastProgress=0,bestDistance=1e30,crouchSince=-1,jumpSince=-1;
    const char* reason="not_bound";
};
struct Diagnostics {
    bool initialized=false,navSystemPresent=false,originalPathSucceeded=false,externalSegmentSucceeded=false;
    bool restartRequired=false;unsigned error=0;uint32_t ticks=0,observationFailures=0,weaponUnavailable=0,configUpdates=0,profileRefreshes=0,planResets=0;
    uint64_t routeValidationRequests=0,routeValidationChecks=0,routeValidationNavQueries=0,routeValidationLocalQueries=0;
    uint64_t spawnRouteHints=0,spawnRouteHintCompletions=0,spawnRouteHintInvalidations=0;
    uintptr_t exceptionRva=0;const char* stage="training_bots_not_initialized";
    const wchar_t* failedEvent=L"";uint16_t expectedSize=0,actualSize=0;
};
inline Diagnostics diagnostics;inline Config config;inline Hooks hooks;
inline std::array<Actor,MaxActors> actors{};inline std::array<Route,MaxRoutes> routes{};
inline std::array<RouteValidation,MaxRoutes> routeValidations{};inline RouteValidationProgress routeValidationProgress;
inline uint64_t nextRouteValidationRequest=0;
inline std::array<Vec,MaxAnchors> anchors{};inline size_t routeCount=0,anchorCount=0;
inline char currentMap[96]{};inline void* world=nullptr;inline void* navigation=nullptr;
inline double nextObservation=0;
inline const Diagnostics& State(){return diagnostics;}
inline const Actor& Player(size_t index){return actors[index<MaxActors?index:0];}
inline const RouteValidationProgress& ValidationState(){return routeValidationProgress;}
inline const RouteValidation* RouteValidationAt(size_t index){return index<routeCount?&routeValidations[index]:nullptr;}
inline const RouteValidation* RouteValidationById(uint32_t id){for(size_t n=0;n<routeCount;++n)if(routeValidations[n].id==id)return &routeValidations[n];return nullptr;}
inline const char* RouteCheckStatusName(RouteCheckStatus value){return value==RouteCheckStatus::Valid?"valid":value==RouteCheckStatus::Blocked?"blocked":"pending";}
inline const char* RouteCheckMethodName(RouteCheckMethod value){return value==RouteCheckMethod::OriginalNavigation?"original_navigation":value==RouteCheckMethod::OriginalCapsuleAndGround?"original_capsule_and_ground":"none";}
inline Style EffectiveRouteStyle(const Route& route){return route.style==Style::Global?config.style:route.style;}
inline void RecountRouteValidations(){
    auto& progress=routeValidationProgress;progress.total=routeCount;progress.checked=progress.valid=progress.blocked=progress.pending=0;
    for(size_t n=0;n<routeCount;++n){const auto& result=routeValidations[n];
        if(result.complete)++progress.checked;
        if(result.status==RouteCheckStatus::Valid)++progress.valid;
        else if(result.status==RouteCheckStatus::Blocked)++progress.blocked;else ++progress.pending;
    }
    progress.complete=progress.checked==progress.total;
}
inline void InvalidateRouteValidation(size_t index,const char* reason){
    auto& result=routeValidations[index];result={};result.id=routes[index].id;result.reason=reason;
}
inline void InvalidateRouteValidations(const char* reason){
    for(size_t n=0;n<routeCount;++n)InvalidateRouteValidation(n,reason);
    routeValidationProgress.active=false;routeValidationProgress.completedAt=-1;routeValidationProgress.reason=reason;RecountRouteValidations();
}
inline double Clamp(double v,double a,double b){return v<a?a:v>b?b:v;}
inline Vec Add(const Vec&a,const Vec&b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec Sub(const Vec&a,const Vec&b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec Mul(const Vec&a,double b){return {a.x*b,a.y*b,a.z*b};}
inline double Length2D(const Vec&v){return std::sqrt(v.x*v.x+v.y*v.y);}
inline double Distance(const Vec&a,const Vec&b){auto d=Sub(a,b);return std::sqrt(d.x*d.x+d.y*d.y+d.z*d.z);}
inline Vec Direction(const Vec&a){const double n=Length2D(a);return n>.0001?Vec{a.x/n,a.y/n,0}:Vec{};}
inline double Delta(double a,double b){double d=std::fmod(a-b+540.0,360.0);if(d<0)d+=360;return d-180;}
inline uint32_t Random(Actor&a){uint32_t v=a.rng;v^=v<<13;v^=v>>17;v^=v<<5;a.rng=v?v:1;return a.rng;}
inline double Unit(Actor&a){return (Random(a)>>8)*(1.0/16777216.0);}
inline double Range(Actor&a,double lo,double hi){return lo+(hi-lo)*Unit(a);}
inline bool Fail(unsigned code,const char* text){diagnostics.error=code;diagnostics.stage=text;return false;}
template<class T> inline T Value(const void* p,size_t off){return original_players::Value<T>(p,off);}
inline bool Thread(){return !diagnostics.restartRequired&&original_players::Thread();}
inline bool Event(void* object,const wchar_t* name,uint16_t size,void* parameters){
    auto method=engine::Find(name);const uint16_t actual=Value<uint16_t>(method,0xbe);
    if(!object||!method||actual!=size){diagnostics.failedEvent=name;diagnostics.expectedSize=size;diagnostics.actualSize=actual;return false;}
    auto table=Value<void**>(object,0);auto process=Value<void*>(table,0x298);if(!process)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,method,parameters);return true;
}
inline LONG Capture(EXCEPTION_POINTERS* e){
    diagnostics.error=e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0002400U;
    const auto at=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    diagnostics.exceptionRva=at>=engine::image?at-engine::image:at;diagnostics.restartRequired=true;
    diagnostics.stage="training_bots_exception_requires_new_session";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool NativeObserve(size_t index,Observation& o){
    auto& roster=original_players::players[index];o={};o.pawn=roster.character;o.controller=roster.controller;
    if(!roster.ready||!roster.characterInitialized||!o.pawn||!o.controller)return true;
    uint8_t alive=0,crouch=0,jump=0;Vec center{};
    struct Eyes {Vec location{},rotation{};} eyes;
    if(!Event(o.pawn,L"/Script/Engine.Actor.K2_GetActorLocation",24,&center)
       ||!Event(o.pawn,L"/Script/Engine.Actor.GetActorEyesViewPoint",48,&eyes)
       ||!Event(o.pawn,L"/Script/Engine.Pawn.GetNavAgentLocation",24,&o.navAgent)
       ||!Event(o.pawn,L"/Script/Engine.Actor.GetVelocity",24,&o.velocity)
       ||!Event(o.controller,L"/Script/Engine.Controller.GetControlRotation",24,&o.rotation)
       ||!Event(o.pawn,L"/Script/ShooterGame.ShooterCharacter.IsAlive",1,&alive)
       ||!Event(o.pawn,L"/Script/ShooterGame.ShooterCharacter.GetCrouchHeld",1,&crouch)
       ||!Event(o.pawn,L"/Script/ShooterGame.ShooterCharacter.GetJumpHeld",1,&jump))return false;
    auto capsule=Value<void*>(o.pawn,0x500);o.capsuleHalfHeight=Value<float>(capsule,0x6d8);o.capsuleRadius=Value<float>(capsule,0x6dc);
    // Actual current native capsule geometry defines feet; no guessed hero
    // dimensions, mesh rescaling or writes to capsule/eye-height are involved.
    o.feet=center;o.feet.z-=o.capsuleHalfHeight;o.eye=eyes.location;o.alive=alive!=0;o.crouched=crouch!=0;o.jumping=jump!=0;
    o.playerInputPresent=Value<void*>(o.controller,0x5e8)!=nullptr;
    auto movement=Value<void*>(o.pawn,0x4f8);auto abilities=Value<void*>(o.pawn,0xa20);
    auto attributes=Value<void*>(abilities,0x13a8);auto tuning=Value<void*>(movement,0x1398);
    auto force=Value<void*>(o.pawn,0x1100);auto gs=Value<void*>(world,0x178);auto fixed=Value<void*>(gs,0xcc0);
    o.speedProbeReady=Value<void*>(movement,0x190)==o.pawn&&Value<void*>(movement,0x1a8)==o.pawn
        &&Value<void*>(movement,0xd8)&&attributes&&tuning&&force&&Value<uint8_t>(fixed,0xd9);
    if(o.speedProbeReady){
        float modifier=0,jumpVelocity=0;
        if(!Event(movement,L"/Script/ShooterGame.ShooterCharacterMovement.GetFullMovementSpeedModifier",4,&modifier)
           ||!Event(movement,L"/Script/ShooterGame.ShooterCharacterMovement.GetJumpVelocity",4,&jumpVelocity)
           ||!Event(movement,L"/Script/ShooterGame.ShooterCharacterMovement.GetMovementType",1,&o.movementType))return false;
        o.jumpVelocity=jumpVelocity;double* output[]{&o.walkSpeed,&o.runSpeed,&o.crouchSpeed};const uint8_t types[]{0,1,3};
        for(size_t n=0;n<3;++n){struct Speed {uint8_t type=0,pad[3]{};float modifier=0,result=0;} p;
            p.type=types[n];p.modifier=modifier;
            if(!Event(movement,L"/Script/ShooterGame.ShooterCharacterMovement.GetMaxSpeedForMovementType",12,&p))return false;
            *output[n]=p.result;
        }
    }
    auto inventory=Value<void*>(o.pawn,0xc08);auto gun=Value<void*>(inventory,0x280);
    if(gun){
        struct Component {void* type;void* result=nullptr;} q{engine::Class(0x63c7e00)};
        if(!Event(gun,L"/Script/Engine.Actor.GetComponentByClass",16,&q))return false;
        if(original_players::IsA(q.result,0x63c7e00)){
            if(!Event(q.result,L"/Script/ShooterGame.AmmoComponent.GetAmmo",4,&o.ammo))return false;
            auto reserve=Value<void*>(q.result,0x138);
            if(reserve&&!Event(reserve,L"/Script/ShooterGame.AmmoComponent.GetAmmo",4,&o.reserve))return false;
        }
    }
    o.ready=original_players::Finite(o.feet)&&original_players::Finite(o.eye)&&original_players::Finite(o.velocity)
        &&o.capsuleHalfHeight>0&&o.capsuleRadius>0;
    return true;
}
inline uint32_t Mix(uint32_t v){v^=v>>16;v*=0x85ebca6bu;v^=v>>13;v*=0xc2b2ae35u;return v^(v>>16);}
// Match identity fixes the actor's ability quantiles across deaths. Behavior
// RNG keeps the original full-epoch seed, so movement/aim noise can vary.
inline uint32_t ProfileIdentitySeed(const Actor&a){return Mix(config.seed^Mix(a.rosterIndex+1));}
inline uint32_t IdentitySeed(const Actor&a){return Mix(config.seed^Mix(a.rosterIndex+1)^Mix(static_cast<uint32_t>(a.epoch))^Mix(static_cast<uint32_t>(a.epoch>>32)+0x9e3779b9u));}
inline double ProfileUnit(uint32_t& seed){seed=Mix(seed+0x9e3779b9u);return (seed>>8)*(1.0/16777216.0);}
inline double Distributed(uint32_t& seed,double center,double spread,double lo,double hi){return Clamp(center+(ProfileUnit(seed)+ProfileUnit(seed)-1)*spread,lo,hi);}
inline void MakeProfile(Actor&a){
    uint32_t sample=ProfileIdentitySeed(a);a.profileSeed=sample;auto& p=a.profile;
    p.reaction=Distributed(sample,config.reactionMs,config.reactionSpreadMs,40,5000)/1000;
    p.turnSpeed=Distributed(sample,config.aimSpeed,config.aimSpeedSpread,1,1440);
    p.accuracy=Distributed(sample,config.accuracy,config.accuracySpread,0,1);
    p.movement=Distributed(sample,config.movement,config.movementSpread,0,1);
    p.errorDegrees=.12+5.5*(1-p.accuracy)*(1-p.accuracy);
    p.burst=.15+.32*p.accuracy;p.pause=.12+.45*(1-p.accuracy);p.headProbability=.1+.55*p.accuracy;
    p.strength=(p.accuracy+p.movement+Clamp(p.turnSpeed/500,0,1)+Clamp(1-p.reaction,0,1))/4;
    ++diagnostics.profileRefreshes;
}
inline void ResetPlan(Actor&a,double now){const auto rotation=a.output.rotation;a.pathCount=a.pathAt=0;a.target=-1;a.targetEpoch=0;a.phase=Phase::ToStart;a.phaseSince=now;a.waitUntil=now;a.bestDistance=1e30;a.lastProgress=now;a.nextPlan=now;a.output={};a.output.rotation=rotation;++diagnostics.planResets;}
inline bool PrepareNPCInput(size_t index);
inline bool Bind(size_t index,void* pawn,uint64_t epoch,bool bot){
    if(!Thread()||index>=MaxActors||!original_players::IsA(pawn,original_players::ShooterCharacterClass))return false;
    if(bot&&static_cast<int>(index)==original_players::localIndex)return Fail(2401,"training_bots_cannot_drive_local_player");
    if(bot&&(original_players::players[index].character!=pawn||!PrepareNPCInput(index)))return false;
    auto& a=actors[index];if(a.bound&&a.pawn==pawn&&a.epoch==epoch&&a.bot==bot)return true;
    const auto initCalls=a.inputInitializations;const bool prepared=a.nativeInputReady,localPreserved=a.localInputUnchanged;
    a={};a.bound=true;a.bot=bot;a.pawn=pawn;a.epoch=epoch;a.rosterIndex=static_cast<uint32_t>(index);a.rng=IdentitySeed(a);if(!a.rng)a.rng=1;
    a.inputInitializations=initCalls;a.nativeInputReady=prepared;a.localInputUnchanged=localPreserved;
    MakeProfile(a);ResetPlan(a,0);nextObservation=0;a.reason="bound_pending_observation";return true;
}
// The root already chose a route for a real fresh Pawn's spawn position. Call
// after Bind and SetRoutes, before this Pawn's first Tick. This only associates
// that first cycle with the same saved route; no path, input or pose is made.
inline bool SetSpawnRouteHint(size_t index,uint64_t epoch,size_t routeIndex){
    if(!Thread()||!diagnostics.initialized||index>=MaxActors||routeIndex>=routeCount||config.mode!=Mode::HoldAngle)
        return Fail(2414,"training_bots_spawn_route_hint_context_invalid");
    auto& a=actors[index];
    if(!a.bound||!a.bot||a.epoch!=epoch||!epoch||a.pawn!=original_players::players[index].character)
        return Fail(2414,"training_bots_spawn_route_hint_requires_matching_new_bot_epoch");
    if(a.lastTick>=0||a.inputFrames||a.cycles||a.phase!=Phase::ToStart||a.pathCount)
        return Fail(2415,"training_bots_spawn_route_hint_must_precede_first_tick");
    const auto& selected=routes[routeIndex];
    if(!selected.enabled||selected.weight<=0||std::strcmp(selected.mapKey,currentMap))
        return Fail(2416,"training_bots_spawn_route_hint_must_select_enabled_weighted_map_route");
    if(a.spawnRouteHintPending&&a.spawnRouteHintId!=selected.id)
        return Fail(2417,"training_bots_spawn_route_hint_already_associated_with_this_pawn");
    if(!a.spawnRouteHintPending)++diagnostics.spawnRouteHints;
    a.spawnRouteHintId=selected.id;a.spawnRouteHintPending=true;a.spawnRouteHintCompleted=false;
    a.route=static_cast<int>(routeIndex);a.reason="spawn_route_hint_waiting_for_first_cycle";return true;
}
inline bool SetSkill(size_t index,double strength){
    if(!Thread()||index>=MaxActors||!std::isfinite(strength))return false;
    auto& p=actors[index].profile;const double s=Clamp(strength,0,1);
    p.reaction=.16+.4*(1-s);p.turnSpeed=100+400*s;p.accuracy=p.movement=p.strength=s;
    p.errorDegrees=.12+5.5*(1-s)*(1-s);p.burst=.15+.32*s;p.pause=.12+.45*(1-s);p.headProbability=.1+.55*s;return true;
}
inline bool SameProfiles(const Config&a,const Config&b){
    return a.seed==b.seed&&a.reactionMs==b.reactionMs&&a.reactionSpreadMs==b.reactionSpreadMs&&a.aimSpeed==b.aimSpeed
        &&a.aimSpeedSpread==b.aimSpeedSpread&&a.accuracy==b.accuracy&&a.accuracySpread==b.accuracySpread&&a.movement==b.movement&&a.movementSpread==b.movementSpread;
}
inline bool SameConfig(const Config&a,const Config&b){
    return SameProfiles(a,b)&&a.mode==b.mode&&a.style==b.style&&a.active==b.active&&a.returnFire==b.returnFire&&a.localInvincible==b.localInvincible
        &&a.peeksPerMinute==b.peeksPerMinute&&a.strength==b.strength&&a.strengthSpread==b.strengthSpread&&a.arrivalCm==b.arrivalCm
        &&a.stallSeconds==b.stallSeconds&&a.localSegmentLimitCm==b.localSegmentLimitCm&&a.maxLocalHeightChangeCm==b.maxLocalHeightChangeCm&&a.sightRangeCm==b.sightRangeCm;
}
inline bool Configure(const Config& c){
    if(!Thread()||!std::isfinite(c.peeksPerMinute)||c.peeksPerMinute<.5||c.peeksPerMinute>180
       ||!std::isfinite(c.strength)||c.strength<0||c.strength>1||!std::isfinite(c.strengthSpread)||c.strengthSpread<0||c.strengthSpread>1
       ||!std::isfinite(c.arrivalCm)||c.arrivalCm<5||c.arrivalCm>60||!std::isfinite(c.stallSeconds)||c.stallSeconds<.5||c.stallSeconds>10
       ||!std::isfinite(c.localSegmentLimitCm)||c.localSegmentLimitCm<=0||c.localSegmentLimitCm>5000
       ||!std::isfinite(c.maxLocalHeightChangeCm)||c.maxLocalHeightChangeCm<0||c.maxLocalHeightChangeCm>100
       ||!std::isfinite(c.sightRangeCm)||c.sightRangeCm<=0||c.sightRangeCm>50000
       ||!std::isfinite(c.reactionMs)||c.reactionMs<0||c.reactionMs>5000||!std::isfinite(c.reactionSpreadMs)||c.reactionSpreadMs<0||c.reactionSpreadMs>5000
       ||!std::isfinite(c.aimSpeed)||c.aimSpeed<1||c.aimSpeed>1440||!std::isfinite(c.aimSpeedSpread)||c.aimSpeedSpread<0||c.aimSpeedSpread>1440
       ||!std::isfinite(c.accuracy)||c.accuracy<0||c.accuracy>1||!std::isfinite(c.accuracySpread)||c.accuracySpread<0||c.accuracySpread>1
       ||!std::isfinite(c.movement)||c.movement<0||c.movement>1||!std::isfinite(c.movementSpread)||c.movementSpread<0||c.movementSpread>1
       ||static_cast<unsigned>(c.mode)>1||static_cast<unsigned>(c.style)>5)return Fail(2402,"training_bots_invalid_configuration");
    if(SameConfig(config,c))return true;
    const bool profiles=!SameProfiles(config,c),reseed=config.seed!=c.seed;
    const bool reset=config.mode!=c.mode||config.arrivalCm!=c.arrivalCm||config.stallSeconds!=c.stallSeconds
        ||config.localSegmentLimitCm!=c.localSegmentLimitCm||config.maxLocalHeightChangeCm!=c.maxLocalHeightChangeCm;
    const bool validationLimits=config.localSegmentLimitCm!=c.localSegmentLimitCm||config.maxLocalHeightChangeCm!=c.maxLocalHeightChangeCm;
    const bool validationStyle=config.style!=c.style;
    config=c;++diagnostics.configUpdates;
    for(auto& a:actors)if(a.bound){if(profiles)MakeProfile(a);if(reseed){a.rng=IdentitySeed(a);if(!a.rng)a.rng=1;}if(reset)ResetPlan(a,a.lastTick<0?0:a.lastTick);}
    if(validationLimits)InvalidateRouteValidations("validation_limits_changed_recheck_required");
    else if(validationStyle){
        for(size_t n=0;n<routeCount;++n)if(routes[n].style==Style::Global)InvalidateRouteValidation(n,"global_route_style_changed_recheck_required");
        routeValidationProgress.active=false;routeValidationProgress.completedAt=-1;RecountRouteValidations();
    }
    return true;
}
inline bool SetConfig(const Config& c){return Configure(c);}
inline bool SamePoint(const Vec&a,const Vec&b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
inline bool SameRouteGeometry(const Route&a,const Route&b){return a.id==b.id&&!std::strcmp(a.mapKey,b.mapKey)&&SamePoint(a.start,b.start)&&SamePoint(a.expose,b.expose)&&SamePoint(a.retreat,b.retreat);}
inline bool SetRoutes(const char* mapKey,const Route* input,size_t count){
    if(!Thread()||!mapKey||!mapKey[0]||strnlen_s(mapKey,96)>=96||count>MaxRoutes||(!input&&count))return false;
    for(size_t n=0;n<count;++n){const auto& r=input[n];
        if(!r.id||strnlen_s(r.mapKey,96)>=96||std::strcmp(r.mapKey,mapKey)||!original_players::Finite(r.start)
           ||!original_players::Finite(r.expose)||!original_players::Finite(r.retreat)||Distance(r.start,r.expose)<20
           ||Distance(r.start,r.expose)>20000||Distance(r.expose,r.retreat)>20000||Distance(r.retreat,r.start)>20000
           ||static_cast<unsigned>(r.style)>8||!std::isfinite(r.weight)||r.weight<0||r.weight>1000)return Fail(2403,"training_bots_invalid_map_route");
        for(size_t k=0;k<n;++k)if(input[k].id==r.id)return Fail(2403,"training_bots_duplicate_route_id");
    }
    bool same=count==routeCount&&!std::strcmp(currentMap,mapKey);
    if(same)for(size_t n=0;n<count;++n)if(!SameRouteGeometry(routes[n],input[n])||routes[n].enabled!=input[n].enabled||routes[n].style!=input[n].style||routes[n].weight!=input[n].weight){same=false;break;}
    if(same)return true;
    // Reorder cached checks by stable ID plus exact geometry. Metadata-only
    // edits preserve unrelated results; these copies never become actor paths.
    std::array<RouteValidation,MaxRoutes> retained{};
    for(size_t n=0;n<count;++n){
        if(!std::strcmp(currentMap,mapKey))for(size_t k=0;k<routeCount;++k)
            if(SameRouteGeometry(routes[k],input[n])&&EffectiveRouteStyle(routes[k])==EffectiveRouteStyle(input[n])){retained[n]=routeValidations[k];break;}
        retained[n].id=input[n].id;
    }
    for(auto& a:actors)if(a.bound){
        if(a.spawnRouteHintPending){
            bool retainedHint=false;
            if(!std::strcmp(currentMap,mapKey))for(size_t n=0;n<count;++n)
                if(input[n].id==a.spawnRouteHintId&&input[n].enabled&&input[n].weight>0){retainedHint=true;break;}
            if(!retainedHint){a.spawnRouteHintPending=false;++diagnostics.spawnRouteHintInvalidations;}
        }
        int replacement=-1;
        if(!std::strcmp(currentMap,mapKey)&&a.route>=0&&static_cast<size_t>(a.route)<routeCount){
            const auto& oldRoute=routes[static_cast<size_t>(a.route)];
            for(size_t n=0;n<count;++n)if(input[n].id==oldRoute.id&&input[n].enabled&&input[n].weight>0){replacement=static_cast<int>(n);
                if(!SameRouteGeometry(oldRoute,input[n]))ResetPlan(a,a.lastTick<0?0:a.lastTick);break;}
        }
        if(replacement<0)ResetPlan(a,a.lastTick<0?0:a.lastTick);a.route=replacement;
    }
    std::memcpy(currentMap,mapKey,std::strlen(mapKey)+1);routeCount=count;
    for(size_t n=0;n<count;++n){routes[n]=input[n];routeValidations[n]=retained[n];}
    for(size_t n=count;n<MaxRoutes;++n)routeValidations[n]={};
    routeValidationProgress.active=false;routeValidationProgress.completedAt=-1;
    routeValidationProgress.reason="saved_routes_require_explicit_validation";RecountRouteValidations();return true;
}
inline bool SetAnchors(const Vec* input,size_t count){
    if(!Thread()||count>MaxAnchors||(!input&&count))return false;
    for(size_t n=0;n<count;++n)if(!original_players::Finite(input[n]))return false;
    anchorCount=count;for(size_t n=0;n<count;++n)anchors[n]=input[n];return true;
}
inline bool Initialize(void* originalWorld,const Hooks& callbacks={}){
    if(!Thread()||originalWorld!=original_players::world||!originalWorld)return false;
    if(diagnostics.initialized&&world!=originalWorld)return Fail(2405,"training_bots_world_changed_requires_new_session");
    if(!original_npc_facing::Initialize())return Fail(2418,"training_bots_original_facing_initialization_failed");
    world=originalWorld;hooks=callbacks;navigation=nullptr;diagnostics.initialized=true;
    struct Nav {void* world;void* result=nullptr;} query{world};
    if(Event(engine::DefaultObject(engine::Class(0x4f7fff0)),L"/Script/NavigationSystem.NavigationSystemV1.GetNavigationSystem",16,&query))navigation=query.result;
    diagnostics.navSystemPresent=original_players::IsA(navigation,0x4f7fff0);
    InvalidateRouteValidations("navigation_context_initialized_recheck_required");
    diagnostics.stage="training_bots_ready_native_navigation_unproven";diagnostics.error=0;return true;
}
inline bool PrepareNPCInputImpl(size_t index){
    if(!Thread()||index>=MaxActors||static_cast<int>(index)==original_players::localIndex)return false;
    const auto& roster=original_players::players[index];auto pc=roster.controller;auto pawn=roster.character;auto& a=actors[index];
    if(!roster.ready||!original_players::IsA(pc,original_players::ShooterControllerClass)
       ||!original_players::IsA(pawn,original_players::ShooterCharacterClass)
       ||Value<void*>(pc,0x4b0)!=pawn||Value<void*>(pawn,0x4a0)!=pc||Value<void*>(pc,0x510))
        return Fail(2406,"training_bots_npc_original_controller_pair_required");
    const auto localPC=original_players::activeController,localPawn=original_players::activePawn,localPlayer=original_players::localPlayer;
    const auto localInput=Value<void*>(localPC,0x5e8);const uint8_t role=Value<uint8_t>(pawn,0x224);
    const auto local3P=Value<uint8_t>(localPawn,0x1270);const auto npc3P=Value<uint8_t>(pawn,0x1270);
    auto table=Value<void**>(pc,0);
    if(Value<void*>(table,0xfe8)!=reinterpret_cast<void*>(engine::image+0x781cbd0))
        return Fail(2407,"training_bots_original_input_initializer_changed");
    auto input=Value<void*>(pc,0x5e8);
    if(input&&(!original_players::IsA(input,0x64f9c00)||Value<void*>(input,0x28)!=pc))
        return Fail(2408,"training_bots_existing_player_input_owner_invalid");
    if(!input||!Value<void*>(pc,0x230)){
        // Same original InitInputSystem virtual used by SetPlayer/local setup.
        //781cbd0 constructs AresPlayerInput owned by THIS PC, calls original
        // Engine492dd80, then title setup. It does not require/assign UPlayer.
        engine::function<void(*)(void*)>(0x781cbd0)(pc);++a.inputInitializations;
    }
    input=Value<void*>(pc,0x5e8);
    a.localInputUnchanged=original_players::activeController==localPC&&original_players::activePawn==localPawn
        &&original_players::localPlayer==localPlayer&&Value<void*>(localPlayer,0x38)==localPC
        &&Value<void*>(localPC,0x5e8)==localInput&&Value<void*>(pc,0x510)==nullptr
        &&Value<uint8_t>(pawn,0x224)==role&&Value<uint8_t>(localPawn,0x1270)==local3P&&Value<uint8_t>(pawn,0x1270)==npc3P;
    a.nativeInputReady=original_players::IsA(input,0x64f9c00)&&Value<void*>(input,0x28)==pc&&Value<void*>(pc,0x230);
    if(!a.localInputUnchanged)return Fail(2409,"training_bots_input_init_changed_local_or_visual_identity");
    return a.nativeInputReady?true:Fail(2410,"training_bots_original_npc_player_input_not_ready");
}
inline bool PrepareNPCInput(size_t index){__try{return PrepareNPCInputImpl(index);}__except(Capture(GetExceptionInformation())){return false;}}
inline void* PawnInput(const Actor&a){
    auto input=Value<void*>(a.pawn,0x230);
    return original_players::IsA(input,0x6492f30)&&Value<void*>(input,0xc8)==a.pawn&&Value<void*>(input,0x170)==a.pawn?input:nullptr;
}
inline bool PumpNPCInput(size_t index){
    auto& a=actors[index];auto input=PawnInput(a);if(!input)return true;
    if(engine::function<int(*)(void*)>(0x3efc820)(a.pawn)!=0)return true;
    const int32_t pending=Value<int32_t>(input,0x8b8),capacity=Value<int32_t>(input,0x8bc);
    if(pending<0||pending>1024||capacity<pending||(pending&&!Value<void*>(input,0x8b0)))return Fail(2411,"training_bots_original_input_queue_invalid");
    if(!pending)return true;
    auto inputState=Value<void*>(a.pawn,0xba0);
    if(Value<void**>(input,0)!=reinterpret_cast<void**>(engine::image+0xb099220)
       ||Value<void**>(inputState,0)!=reinterpret_cast<void**>(engine::image+0xb093c10)
       ||Value<void*>(inputState,0xc8)!=a.pawn||Value<void*>(inputState,0x148)!=a.pawn)
        return Fail(2412,"training_bots_original_input_transport_identity_invalid");
    // Only the missing standalone transport; normal original fixed-step input
    // state tick still interprets the produced jump/weapon commands.
    engine::function<void(*)(void*,void*)>(0x70a7110)(inputState,static_cast<uint8_t*>(input)+0x178);
    ++a.inputTransports;a.inputEvents+=static_cast<uint32_t>(pending);
    return Value<int32_t>(input,0x8b8)==0?true:Fail(2413,"training_bots_original_input_transport_not_consumed");
}
struct RouteValidationContext {size_t index=MaxActors;uint64_t epoch=0;void* pawn=nullptr;double radius=0,half=0;bool nativeStandingContext=false;};
inline bool ValidationContextFor(size_t index,RouteValidationContext& context){
    if(index>=MaxActors)return false;const auto& a=actors[index];const auto& entry=original_players::players[index];
    if(!a.bound||!entry.ready||!entry.characterInitialized||a.pawn!=entry.character
       ||!original_players::IsA(a.pawn,original_players::ShooterCharacterClass))return false;
    uint8_t alive=0;if(!Event(a.pawn,L"/Script/ShooterGame.ShooterCharacter.IsAlive",1,&alive)||!alive)return false;
    auto capsule=Value<void*>(a.pawn,0x500);auto cdo=engine::DefaultObject(Value<void*>(a.pawn,0x10));
    auto originalStandingCapsule=Value<void*>(cdo,0x500);
    const double radius=Value<float>(capsule,0x6dc),half=Value<float>(capsule,0x6d8);
    const double standingRadius=Value<float>(originalStandingCapsule,0x6dc),standingHalf=Value<float>(originalStandingCapsule,0x6d8);
    if(!std::isfinite(radius)||!std::isfinite(half)||!std::isfinite(standingRadius)||!std::isfinite(standingHalf)
       ||radius<=0||half<radius||standingRadius<=0||standingHalf<standingRadius)return false;
    // A crouch in progress must not certify a route for a smaller body and
    // subsequently expose it as suitable for Wide/Random. Both dimensions are
    // read from this actual original hero; no actor/capsule field is changed.
    context={index,a.epoch,a.pawn,(std::max)(radius,standingRadius),(std::max)(half,standingHalf),radius>=standingRadius&&half>=standingHalf};return true;
}
inline bool SelectValidationContext(size_t preferred,RouteValidationContext& context){
    if(preferred<MaxActors)return ValidationContextFor(preferred,context);
    for(size_t n=0;n<MaxActors;++n)if(actors[n].bot&&ValidationContextFor(n,context))return true;
    for(size_t n=0;n<MaxActors;++n)if(!actors[n].bot&&ValidationContextFor(n,context))return true;
    return false;
}
inline void RefreshValidationNavigation(){
    struct Query {void* context=nullptr;void* result=nullptr;} query{world};
    navigation=nullptr;
    if(Event(engine::DefaultObject(engine::Class(0x4f7fff0)),L"/Script/NavigationSystem.NavigationSystemV1.GetNavigationSystem",16,&query))navigation=query.result;
    diagnostics.navSystemPresent=original_players::IsA(navigation,0x4f7fff0);
}
inline SegmentValidation CheckRouteSegment(const RouteValidationContext& context,const Vec& from,const Vec& to){
    SegmentValidation result;result.from=from;result.to=to;
    if(!original_players::Finite(from)||!original_players::Finite(to)){result.status=RouteCheckStatus::Blocked;result.reason="nonfinite_route_endpoint";return result;}
    if(diagnostics.navSystemPresent&&context.nativeStandingContext){
        struct Query {void* worldContext;Vec start,end;void* pawn;void* filter=nullptr;void* result=nullptr;} query{world,from,to,context.pawn};
        static_assert(sizeof(Query)==80&&offsetof(Query,result)==72);
        result.navAttempted=true;++diagnostics.routeValidationNavQueries;
        if(!Event(engine::DefaultObject(engine::Class(0x4f7fff0)),L"/Script/NavigationSystem.NavigationSystemV1.FindPathToLocationSynchronously",80,&query))result.navReason="original_path_query_abi_unavailable";
        else if(!original_players::IsA(query.result,0x4f80dd0))result.navReason="original_navigation_path_unavailable";
        else{
            uint8_t valid=0,partial=1;auto points=Value<Vec*>(query.result,0x40);
            const int32_t count=Value<int32_t>(query.result,0x48),capacity=Value<int32_t>(query.result,0x4c);result.navPointCount=count;
            if(!Event(query.result,L"/Script/NavigationSystem.NavigationPath.IsValid",1,&valid)
               ||!Event(query.result,L"/Script/NavigationSystem.NavigationPath.IsPartial",1,&partial))result.navReason="original_path_status_abi_unavailable";
            else if(!valid)result.navReason="original_navigation_path_invalid";
            else if(partial){result.navPartial=true;result.navReason="original_navigation_path_partial";}
            else if(count<(SamePoint(from,to)?1:2)||count>static_cast<int32_t>(MaxPath)||capacity<count
                    ||!original_players::Readable(points,static_cast<size_t>(count)*sizeof(Vec)))result.navReason="original_navigation_points_invalid_or_too_many";
            else{
                bool finite=true;for(int32_t n=0;n<count;++n)finite=finite&&original_players::Finite(points[n]);
                if(!finite)result.navReason="original_navigation_point_nonfinite";
                else{
                    result.navStartErrorCm=Distance(points[0],from);result.navEndErrorCm=Distance(points[count-1],to);
                    if(result.navStartErrorCm>=60||result.navEndErrorCm>=60)result.navReason="original_navigation_endpoint_projection_too_far";
                    else{result.status=RouteCheckStatus::Valid;result.method=RouteCheckMethod::OriginalNavigation;
                        result.reason=result.navReason="original_full_navigation_path_verified";return result;}
                }
            }
        }
    }else result.navReason=diagnostics.navSystemPresent?"native_nav_context_currently_smaller_than_standing_body":"original_navigation_system_unavailable";
    const Vec delta=Sub(to,from);
    if(Length2D(delta)>config.localSegmentLimitCm){result.status=RouteCheckStatus::Blocked;result.reason="local_segment_limit_exceeded_without_full_native_path";return result;}
    if(std::abs(delta.z)>config.maxLocalHeightChangeCm){result.status=RouteCheckStatus::Blocked;result.reason="local_height_limit_exceeded_without_full_native_path";return result;}
    if(!hooks.validateSegment){result.reason="awaiting_original_nav_or_capsule_ground_validator";return result;}
    result.localAttempted=true;++diagnostics.routeValidationLocalQueries;
    if(hooks.validateSegment(context.index,from,to,context.radius,context.half)){
        result.status=RouteCheckStatus::Valid;result.method=RouteCheckMethod::OriginalCapsuleAndGround;
        result.reason="original_capsule_and_supporting_ground_verified";
    }else{result.status=RouteCheckStatus::Blocked;result.reason="original_capsule_or_supporting_ground_check_failed";}
    return result;
}
inline void CheckSavedRoute(size_t index,const RouteValidationContext& context,double now){
    const auto& route=routes[index];auto& result=routeValidations[index];
    const auto request=result.request;result={};result.id=route.id;result.request=request;result.requested=true;
    result.validatorIndex=context.index;result.validatorEpoch=context.epoch;result.checkedAt=now;
    result.capsuleRadius=context.radius;result.capsuleHalfHeight=context.half;result.effectiveStyle=EffectiveRouteStyle(route);
    result.segments[0]=CheckRouteSegment(context,route.start,route.expose);
    result.segments[1]=CheckRouteSegment(context,route.expose,route.retreat);
    result.segments[2]=CheckRouteSegment(context,route.retreat,route.start);
    if(result.effectiveStyle==Style::Small||result.effectiveStyle==Style::SmallCrouch||result.effectiveStyle==Style::Random){
        const auto small=training_bot_controller::Add(route.start,training_bot_controller::Mul(Sub(route.expose,route.start),.45));
        result.segmentCount=5;result.segments[3]=CheckRouteSegment(context,route.start,small);
        result.segments[4]=CheckRouteSegment(context,small,route.retreat);
    }
    result.complete=true;bool pending=false;
    for(size_t n=0;n<result.segmentCount;++n){const auto status=result.segments[n].status;
        if(status==RouteCheckStatus::Pending){pending=true;result.complete=false;}
        if(status==RouteCheckStatus::Blocked&&result.failedSegment<0)result.failedSegment=static_cast<int>(n);
    }
    if(result.failedSegment>=0){
        constexpr const char* blockedReasons[]{"start_to_expose_blocked","expose_to_retreat_blocked","retreat_to_start_blocked","start_to_small_exposure_blocked","small_exposure_to_retreat_blocked"};
        result.complete=true;result.status=RouteCheckStatus::Blocked;result.reason=blockedReasons[static_cast<size_t>(result.failedSegment)];
    }else if(pending){result.status=RouteCheckStatus::Pending;result.reason="awaiting_original_navigation_or_geometry_validator";}
    else{result.status=RouteCheckStatus::Valid;result.reason="all_saved_route_segments_verified";}
    ++diagnostics.routeValidationChecks;
}
inline bool PollRouteValidationsImpl(double now,size_t maximumRoutes){
    if(!Thread()||!diagnostics.initialized||!std::isfinite(now)||now<0||!maximumRoutes||maximumRoutes>MaxRoutes)return false;
    auto& progress=routeValidationProgress;if(!progress.active)return true;
    RouteValidationContext context;
    if(!SelectValidationContext(progress.preferredValidator,context)){
        progress.reason="validation_waiting_for_alive_original_hero_capsule";
        for(size_t n=0;n<routeCount;++n)if(!routeValidations[n].complete)routeValidations[n].reason=progress.reason;
        return true;
    }
    RefreshValidationNavigation();size_t checked=0,scanned=0;
    while(scanned<routeCount&&checked<maximumRoutes){const size_t index=progress.cursor++%routeCount;++scanned;
        auto& result=routeValidations[index];if(result.complete||!result.requested||result.request!=progress.request)continue;
        CheckSavedRoute(index,context,now);++checked;
    }
    RecountRouteValidations();
    if(progress.complete){progress.active=false;progress.completedAt=now;progress.reason=progress.blocked?"route_validation_complete_with_blocked_routes":"all_routes_verified";}
    else progress.reason="route_validation_in_progress";return true;
}
inline bool PollRouteValidations(double now,size_t maximumRoutes=2){
    __try{return PollRouteValidationsImpl(now,maximumRoutes);}__except(Capture(GetExceptionInformation())){return false;}
}
// Explicitly rechecks even byte-identical saved data. Tick runs the actual GT
// queries, bounded to two routes per tick. No actor plan, phase,
// RNG, input or transform is changed by any validation function.
inline bool ValidateRoutes(double now,size_t validatorIndex=MaxActors){
    if(!Thread()||!diagnostics.initialized||!std::isfinite(now)||now<0||validatorIndex>MaxActors)return false;
    auto& progress=routeValidationProgress;progress={};progress.request=++nextRouteValidationRequest;
    progress.active=routeCount>0;progress.preferredValidator=validatorIndex;progress.requestedAt=now;
    progress.reason=routeCount?"route_validation_requested":"no_saved_routes_for_this_map";
    for(size_t n=0;n<routeCount;++n){InvalidateRouteValidation(n,"route_validation_pending");
        routeValidations[n].request=progress.request;routeValidations[n].requested=true;}
    ++diagnostics.routeValidationRequests;RecountRouteValidations();
    if(!routeCount)progress.completedAt=now;return true;
}
inline bool ValidateRouteImpl(size_t index,double now,size_t validatorIndex){
    if(!Thread()||!diagnostics.initialized||index>=routeCount||!std::isfinite(now)||now<0||validatorIndex>MaxActors)return false;
    RouteValidationContext context;auto& result=routeValidations[index];InvalidateRouteValidation(index,"route_validation_pending");
    result.request=routeValidationProgress.active?routeValidationProgress.request:++nextRouteValidationRequest;result.requested=true;
    ++diagnostics.routeValidationRequests;
    if(!SelectValidationContext(validatorIndex,context)){result.reason="validation_waiting_for_alive_original_hero_capsule";RecountRouteValidations();return true;}
    RefreshValidationNavigation();CheckSavedRoute(index,context,now);RecountRouteValidations();
    auto& progress=routeValidationProgress;
    if(progress.active&&progress.complete){progress.active=false;progress.completedAt=now;progress.reason=progress.blocked?"route_validation_complete_with_blocked_routes":"all_routes_verified";}
    return true;
}
inline bool ValidateRoute(size_t index,double now,size_t validatorIndex=MaxActors){
    __try{return ValidateRouteImpl(index,now,validatorIndex);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool Plan(size_t index,const Vec& destination,double now){
    auto& a=actors[index];a.pathCount=a.pathAt=0;a.navPath=a.localPath=false;a.goal=destination;++a.navQueries;
    if(diagnostics.navSystemPresent){
        struct Query {void* context;Vec start,end;void* pawn;void* filter=nullptr;void* result=nullptr;} q{world,a.observed.feet,destination,a.pawn};
        static_assert(sizeof(Query)==80&&offsetof(Query,result)==72);
        if(Event(engine::DefaultObject(engine::Class(0x4f7fff0)),L"/Script/NavigationSystem.NavigationSystemV1.FindPathToLocationSynchronously",80,&q)
           &&original_players::IsA(q.result,0x4f80dd0)){
            uint8_t valid=0,partial=1;auto points=Value<Vec*>(q.result,0x40);const int32_t count=Value<int32_t>(q.result,0x48),capacity=Value<int32_t>(q.result,0x4c);
            if(Event(q.result,L"/Script/NavigationSystem.NavigationPath.IsValid",1,&valid)
               &&Event(q.result,L"/Script/NavigationSystem.NavigationPath.IsPartial",1,&partial)&&valid&&!partial
               &&count>=2&&count<=static_cast<int32_t>(MaxPath)&&capacity>=count&&original_players::Readable(points,static_cast<size_t>(count)*sizeof(Vec))
               &&Distance(points[count-1],destination)<60){
                bool finite=true;for(int32_t n=0;n<count;++n){a.path[static_cast<size_t>(n)]=points[n];finite=finite&&original_players::Finite(points[n]);}
                if(finite){a.pathCount=static_cast<size_t>(count);a.pathAt=1;a.navPath=true;diagnostics.originalPathSucceeded=true;}
            }
        }
    }
    const Vec delta=Sub(destination,a.observed.feet);
    if(!a.pathCount&&Length2D(delta)<=config.localSegmentLimitCm&&std::abs(delta.z)<=config.maxLocalHeightChangeCm
       &&hooks.validateSegment&&hooks.validateSegment(index,a.observed.feet,destination,a.observed.capsuleRadius,a.observed.capsuleHalfHeight)){
        a.path[0]=destination;a.pathCount=1;a.pathAt=0;a.localPath=true;diagnostics.externalSegmentSucceeded=true;
    }
    a.lastProgress=now;a.bestDistance=1e30;
    if(!a.pathCount){a.phase=Phase::Blocked;a.phaseSince=now;a.waitUntil=now+2;a.reason="route_requires_original_nav_or_capsule_ground_validation";++a.blocked;return false;}
    a.reason=a.navPath?"following_original_navigation_path":"following_externally_validated_local_segment";return true;
}
inline void Change(Actor&a,Phase phase,double now){a.phase=phase;a.phaseSince=now;a.pathCount=a.pathAt=0;a.bestDistance=1e30;a.lastProgress=now;}
inline bool Move(size_t index,const Vec& goal,double now){
    auto& a=actors[index];a.output.worldDirection={};
    if(Distance(a.observed.feet,goal)<=config.arrivalCm){a.pathCount=0;return true;}
    if(!a.pathCount&&now>=a.nextPlan){a.nextPlan=now+.5;if(!Plan(index,goal,now))return false;}
    if(!a.pathCount)return false;
    while(a.pathAt<a.pathCount){const double tolerance=a.pathAt+1<a.pathCount?45:config.arrivalCm;
        if(Distance(a.observed.feet,a.path[a.pathAt])>tolerance)break;++a.pathAt;}
    if(a.pathAt>=a.pathCount){a.pathCount=0;return Distance(a.observed.feet,goal)<=60;}
    const double distance=Distance(a.observed.feet,a.path[a.pathAt]);
    if(distance<a.bestDistance-4){a.bestDistance=distance;a.lastProgress=now;}
    if(now-a.lastProgress>config.stallSeconds){Change(a,Phase::Blocked,now);a.waitUntil=now+2;a.reason="native_collision_or_path_stall_no_teleport";++a.blocked;return false;}
    a.output.worldDirection=Direction(Sub(a.path[a.pathAt],a.observed.feet));return false;
}
inline training_control::PracticePlan practicePlan{};
inline double practiceStarted=-1,nextExposure=0,practiceLastTick=-1;
inline bool practiceWasActive=false;
inline uint32_t practiceWave=0;
inline std::array<int,MaxActors> practiceAssignment{};
inline std::array<bool,MaxActors> practiceDone{},practiceExposed{};
inline std::array<double,MaxActors> practiceDue{};
inline bool practiceWaveReady=false,practiceFinished=false;
inline bool ValidPracticePlan(const training_control::RouteSet& source){
    const auto next=training_control::ReadPracticePlan(source);
    if(next.magic&&next.magic!=0x504c414e)return false;
    if(next.magic){
        if(next.version!=1||next.mode>1||next.count>64||!std::isfinite(next.interval)||next.interval<0||next.interval>120)return false;
        for(uint32_t n=0;n<next.count;++n){const auto& e=next.entries[n];
            if(e.wave>=64||e.slot>=MaxActors-1||e.style>8||e.fire>1||!std::isfinite(e.minimum)||!std::isfinite(e.maximum)||e.minimum<0||e.maximum>120||e.minimum>e.maximum)return false;
            bool found=false;for(size_t r=0;r<source.count;++r)if(source.items[r].id==e.route)found=true;if(!found)return false;
            for(uint32_t k=0;k<n;++k)if(next.mode&&next.entries[k].wave==e.wave&&next.entries[k].slot==e.slot)return false;
        }
    }
    return true;
}
inline bool SetPracticePlan(const training_control::RouteSet& source){
    if(!ValidPracticePlan(source))return false;
    const auto next=training_control::ReadPracticePlan(source);
    if(!std::memcmp(&practicePlan,&next,sizeof(next)))return true;
    practicePlan=next;practiceLastTick=-1;practiceStarted=-1;nextExposure=0;practiceWave=0;practiceWaveReady=false;practiceFinished=false;
    practiceAssignment.fill(-1);practiceDone.fill(false);practiceExposed.fill(false);
    for(auto& a:actors)if(a.bound&&a.bot){a.route=-1;ResetPlan(a,a.lastTick<0?0:a.lastTick);}
    return true;
}
inline int PracticeEntryFor(const Actor& a){
    if(!practicePlan.magic||a.route<0||static_cast<size_t>(a.route)>=routeCount)return -1;for(uint32_t n=0;n<practicePlan.count;++n)if(practicePlan.entries[n].route==routes[a.route].id)return static_cast<int>(n);return -1;
}
inline void PracticeClock(double now){
    const double elapsed=practiceLastTick<0?0:std::max(0.0,now-practiceLastTick);practiceLastTick=now;
    if(practicePlan.magic&&(!config.active||!practiceWasActive)){
        practiceStarted+=elapsed;nextExposure+=elapsed;
        for(size_t n=0;n<MaxActors;++n){practiceDue[n]+=elapsed;if(actors[n].bound&&actors[n].bot)actors[n].waitUntil+=elapsed;}
    }practiceWasActive=config.active;
}
inline void ScheduleWave(double now){
    if(!practicePlan.magic||!practicePlan.mode||practiceFinished)return;
    if(!practiceWaveReady){
        size_t available=0,required=0;for(const auto& a:actors)if(a.bound&&a.bot)++available;
        for(uint32_t n=0;n<practicePlan.count;++n)if(practicePlan.entries[n].wave==practiceWave)required=std::max(required,static_cast<size_t>(practicePlan.entries[n].slot)+1);
        if(available<required)return;
        practiceStarted=now;practiceAssignment.fill(-1);practiceDone.fill(false);practiceExposed.fill(false);
        size_t slot=0;for(size_t i=0;i<MaxActors;++i){auto& a=actors[i];if(!a.bound||!a.bot)continue;
            for(uint32_t n=0;n<practicePlan.count;++n){const auto& e=practicePlan.entries[n];if(e.wave!=practiceWave||e.slot!=slot)continue;
                practiceAssignment[i]=static_cast<int>(n);practiceDue[i]=now+Range(a,e.minimum,e.maximum);a.route=-1;
                for(size_t r=0;r<routeCount;++r)if(routes[r].id==e.route)a.route=static_cast<int>(r);
                ResetPlan(a,now);break;
            }++slot;
        }practiceWaveReady=true;
    }
    bool all=true;for(size_t i=0;i<MaxActors;++i)if(practiceAssignment[i]>=0){
        if(practiceExposed[i]&&!actors[i].observed.alive)practiceDone[i]=true;
        // A blocked route or a stalled lifecycle must not trap the wave forever.
        if(now>practiceDue[i]+30)practiceDone[i]=true;
        if(!practiceDone[i])all=false;
    }
    if(all){++practiceWave;practiceWaveReady=false;bool more=false;for(uint32_t n=0;n<practicePlan.count;++n)if(practicePlan.entries[n].wave>=practiceWave)more=true;practiceFinished=!more;}
}
inline int PickRoute(Actor&a){
    double total=0;for(size_t n=0;n<routeCount;++n)if(routes[n].enabled)total+=routes[n].weight;
    if(total<=0)return -1;double selected=Unit(a)*total;
    for(size_t n=0;n<routeCount;++n)if(routes[n].enabled&&routes[n].weight>0){selected-=routes[n].weight;if(selected<0)return static_cast<int>(n);}
    for(size_t n=routeCount;n>0;--n)if(routes[n-1].enabled&&routes[n-1].weight>0)return static_cast<int>(n-1);return -1;
}
inline int PickInitialOrRandomRoute(Actor&a){
    if(a.spawnRouteHintPending){
        for(size_t n=0;n<routeCount;++n)if(routes[n].id==a.spawnRouteHintId&&routes[n].enabled&&routes[n].weight>0)
            return static_cast<int>(n);
        // An explicit route edit/removal may invalidate the original choice.
        // Preserve the normal enabled/weight policy instead of a stale index.
        a.spawnRouteHintPending=false;++diagnostics.spawnRouteHintInvalidations;
    }
    return PickRoute(a);
}
inline void HoldStrategy(size_t index,double now){
    auto& a=actors[index];
    const bool waves=practicePlan.magic&&practicePlan.mode;
    if(waves&&(practiceFinished||!practiceWaveReady||practiceAssignment[index]<0||practiceDone[index]))return;
    if(waves&&a.route<0){const auto id=practicePlan.entries[practiceAssignment[index]].route;
        for(size_t r=0;r<routeCount;++r)if(routes[r].id==id)a.route=static_cast<int>(r);}
    if(!routeCount){Change(a,Phase::Blocked,now);a.reason="no_saved_routes_for_this_map";return;}
    if(a.route<0||static_cast<size_t>(a.route)>=routeCount)a.route=PickInitialOrRandomRoute(a);
    if(a.route<0){Change(a,Phase::Blocked,now);a.reason="no_enabled_positive_weight_routes";return;}
    const auto& r=routes[static_cast<size_t>(a.route)];
    const int entryIndex=waves?practiceAssignment[index]:PracticeEntryFor(a);
    const auto* entry=entryIndex>=0?&practicePlan.entries[entryIndex]:nullptr;
    if(a.phase==Phase::Blocked){if(now>=a.waitUntil){if(!waves)a.route=PickInitialOrRandomRoute(a);Change(a,Phase::ToStart,now);}return;}
    if(a.phase==Phase::ToStart&&Move(index,r.start,now)){Change(a,Phase::Waiting,now);a.waitUntil=waves?practiceDue[index]:std::max(a.waitUntil,now+Range(a,entry?entry->minimum:.1,entry?entry->maximum:.4));}
    if(a.phase==Phase::Waiting&&now>=a.waitUntil&&(waves||now>=nextExposure)){
        const auto requested=entry?static_cast<Style>(entry->style):r.style;
        const auto selectedStyle=requested==Style::Global?config.style:requested;
        a.style=selectedStyle==Style::Random?static_cast<Style>(Random(a)%5):selectedStyle;
        a.cycleStart=now;nextExposure=now+(practicePlan.magic?practicePlan.interval:60/config.peeksPerMinute);if(waves)practiceExposed[index]=true;Change(a,Phase::Exposing,now);if(a.style==Style::JumpWide)a.jumpUntil=now+.13;
    }
    Vec exposed=r.expose;if(a.style==Style::Small||a.style==Style::SmallCrouch)exposed=training_bot_controller::Add(r.start,training_bot_controller::Mul(Sub(r.expose,r.start),.45));
    if(a.phase==Phase::Exposing){a.output.crouch=a.style==Style::Crouch;a.output.walk=a.style==Style::Peek;
        a.output.jump=a.style==Style::JumpWide&&now<a.jumpUntil;
        if(Move(index,exposed,now)){Change(a,Phase::Holding,now);a.waitUntil=now+(a.style==Style::Peek?.06:Range(a,.12,.35));}}
    if(a.phase==Phase::Holding){a.output.crouch=a.style==Style::Crouch||a.style==Style::WideCrouch||a.style==Style::SmallCrouch;if(now>=a.waitUntil)Change(a,Phase::Withdrawing,now);}
    if(a.phase==Phase::Withdrawing&&Move(index,r.retreat,now))Change(a,Phase::Returning,now);
    if(a.phase==Phase::Returning&&Move(index,r.start,now)){
        ++a.cycles;if(waves)practiceDone[index]=true;
        if(a.spawnRouteHintPending){a.spawnRouteHintPending=false;a.spawnRouteHintCompleted=true;++diagnostics.spawnRouteHintCompletions;}
        Change(a,Phase::ToStart,now);a.waitUntil=std::max(now+.05,a.cycleStart+(practicePlan.magic?0:60/config.peeksPerMinute));a.route=-1;
    }
}
inline bool Sees(size_t index,size_t target){
    return hooks.visible&&hooks.visible(index,target,actors[index].observed.eye,actors[target].observed.eye);
}
inline void Combat(size_t index,double now,double dt){
    auto& a=actors[index];
    const int entryIndex=practicePlan.mode?practiceAssignment[index]:PracticeEntryFor(a);
    const bool assigned=!practicePlan.mode||(practiceWaveReady&&!practiceFinished&&entryIndex>=0&&!practiceDone[index]&&practiceExposed[index]);
    const bool permitted=config.mode==Mode::Deathmatch||(assigned&&(practicePlan.magic&&entryIndex>=0?practicePlan.entries[entryIndex].fire!=0:config.returnFire));
    if(!hooks.visible){a.target=-1;return;}
    int selected=a.target;
    // Revoke an existing target even between the normal sensing slots.
    if(selected>=0&&hooks.targetAllowed&&!hooks.targetAllowed(index,static_cast<size_t>(selected))){
        selected=-1;a.target=-1;a.visible=false;a.output.fire=false;
    }
    if(now>=a.nextSense){
        a.nextSense=now+.08;selected=-1;double best=1e30;
        for(size_t n=0;n<MaxActors;++n){const auto& candidate=actors[n];
            if(n==index||!candidate.bound||!candidate.observed.ready||!candidate.observed.alive
               ||(config.mode==Mode::HoldAngle&&candidate.bot)
               ||(hooks.targetAllowed&&!hooks.targetAllowed(index,n)))continue;
            const double distance=Distance(a.observed.eye,candidate.observed.eye);
            if(distance>config.sightRangeCm||distance>=best||!Sees(index,n))continue;best=distance;selected=static_cast<int>(n);
        }
    }
    a.visible=selected>=0;if(selected<0){a.target=-1;a.output.fire=false;return;}
    auto& target=actors[static_cast<size_t>(selected)];
    if(!target.observed.ready||!target.observed.alive){a.target=-1;a.visible=false;return;}
    if(selected!=a.target||a.targetEpoch!=target.epoch){a.target=selected;a.targetEpoch=target.epoch;a.reactionUntil=now+a.profile.reaction+Range(a,.02,.12);a.nextBurst=a.reactionUntil;a.nextBias=0;}
    if(now>=a.nextBias){
        const double error=a.profile.errorDegrees+(Length2D(a.observed.velocity)>80?.8:0);
        a.bias={Range(a,-error,error),Range(a,-error,error),Unit(a)<a.profile.headProbability?1.0:0.0};a.nextBias=now+Range(a,.1,.25);
    }
    Vec point=target.observed.eye;if(a.bias.z<.5)point=training_bot_controller::Add(target.observed.feet,training_bot_controller::Mul(Sub(point,target.observed.feet),.68));
    const Vec aim=Sub(point,a.observed.eye);const double yaw=std::atan2(aim.y,aim.x)*180/3.14159265358979323846+a.bias.y;
    const double pitch=std::atan2(aim.z,std::max(.001,Length2D(aim)))*180/3.14159265358979323846+a.bias.x;
    if(now>=a.reactionUntil){const double step=a.profile.turnSpeed*dt;
        a.output.rotation.y+=Clamp(Delta(yaw,a.output.rotation.y),-step,step);
        a.output.rotation.x=Clamp(a.output.rotation.x+Clamp(Delta(pitch,a.output.rotation.x),-step,step),-85,85);
        if(now>=a.nextBurst){a.burstUntil=now+a.profile.burst*Range(a,.75,1.25);a.nextBurst=a.burstUntil+a.profile.pause*Range(a,.7,1.5);}
        const double tolerance=.6+2.5*(1-a.profile.accuracy);
        a.output.fire=permitted&&now<a.burstUntil&&std::abs(Delta(yaw,a.output.rotation.y))<tolerance&&std::abs(Delta(pitch,a.output.rotation.x))<tolerance;
    }
    if(permitted&&a.observed.ammo==0&&a.observed.reserve>0){a.output.fire=false;a.output.reload=true;}
}
inline void DeathmatchStrategy(size_t index,double now){
    auto& a=actors[index];
    if(a.target>=0&&a.visible){a.phase=Phase::Engaging;
        const double movement=a.profile.movement;
        if(now>=a.nextMove){a.nextMove=now+Range(a,.8-.5*movement,1.4-.6*movement);const double yaw=a.output.rotation.y*3.14159265358979323846/180;
            const double side=Unit(a)>.5?1:-1,stride=Range(a,35+50*movement,80+150*movement);
            const Vec offset{-std::sin(yaw)*side*stride,std::cos(yaw)*side*stride,0};
            a.goal=training_bot_controller::Add(a.observed.feet,offset);a.pathCount=0;a.combatCrouch=Unit(a)<.05+.22*movement;
            if(Unit(a)<.06*movement)a.jumpUntil=now+.13;}
        Move(index,a.goal,now);a.output.crouch=a.combatCrouch;a.output.jump=now<a.jumpUntil;
    }else{
        if(a.phase==Phase::Blocked&&now<a.waitUntil)return;
        if(!anchorCount){a.reason="deathmatch_requires_verified_map_anchors";a.phase=Phase::Blocked;a.waitUntil=now+2;return;}
        if(a.phase!=Phase::Roaming||Distance(a.observed.feet,a.goal)<50){Change(a,Phase::Roaming,now);a.goal=anchors[Random(a)%static_cast<uint32_t>(anchorCount)];}
        if(Move(index,a.goal,now))a.phase=Phase::Inactive;
    }
}
inline bool NativeSubmit(size_t index,float deltaSeconds=0){
    auto& a=actors[index];auto& f=a.output;
    training_local_posture_probe::Capture("npc_before",static_cast<int>(index),f.crouch);
    if(!original_enemies::SubmitWorldInput(index,f.worldDirection,f.rotation.y,f.walk,f.crouch))return false;
    if(!Event(a.observed.controller,L"/Script/Engine.Controller.SetControlRotation",24,&f.rotation))return false;
    if(!original_npc_facing::Apply(index,a.pawn,a.observed.controller,a.epoch,deltaSeconds))return false;
    if(f.jump!=a.requestedJump){uint8_t jump=f.jump?1:0;auto input=PawnInput(a);const int32_t before=Value<int32_t>(input,0x8b8);
        if(input){
            if(!Event(a.pawn,L"/Script/ShooterGame.ShooterCharacter.ForceJumpInputState",1,&jump))return false;
            if(Value<int32_t>(input,0x8b8)>before)++a.jumpQueueCalls;else input=nullptr;
        }
        if(!input){
            // Genuine original direct jump input API, also used by local
            // original_movement. Native78812f0 retains its cooldown/Jump/
            // delegates; no hand-written held bit or capsule change.
            if(!Event(a.pawn,L"/Script/ShooterGame.ShooterCharacter.SetJumpHeld",1,&jump))return false;++a.jumpSetterCalls;
        }
        a.requestedJump=f.jump;
    }
    if(hooks.weaponInput){if(!hooks.weaponInput(index,f.fire,f.reload))return false;}
    else if(f.fire||f.reload){f.fire=f.reload=false;++diagnostics.weaponUnavailable;}
    if(!PumpNPCInput(index))return false;
    training_local_posture_probe::Capture("npc_after",static_cast<int>(index),f.crouch);
    ++a.inputFrames;return true;
}
inline bool TickImpl(double now){
    if(!Thread()||!diagnostics.initialized||!std::isfinite(now))return false;
    if(routeValidationProgress.active&&!PollRouteValidations(now))return false;
    if(now>=nextObservation){nextObservation=now+.05;
    for(size_t index=0;index<MaxActors;++index){auto& a=actors[index];if(!a.bound)continue;
        Observation sample;const bool ok=hooks.observe?hooks.observe(index,sample):NativeObserve(index,sample);
        if(!ok){++diagnostics.observationFailures;sample={};}
        if(sample.pawn&&sample.pawn!=a.pawn){a.observed={};a.output={};a.reason="new_pawn_requires_bind_epoch";continue;}
        if(sample.spawnEpoch&&sample.spawnEpoch!=a.epoch){a.observed={};a.output={};a.reason="new_spawn_epoch_requires_bind";continue;}
        a.observed=sample;
    }
    }
    PracticeClock(now);
    if(config.active&&config.mode==Mode::HoldAngle)ScheduleWave(now);
    for(size_t index=0;index<MaxActors;++index){auto& a=actors[index];if(!a.bound||!a.bot)continue;
        const double dt=a.lastTick<0?.016:Clamp(now-a.lastTick,0,.1);a.lastTick=now;
        const auto rotation=a.output.rotation;a.output={};a.output.rotation=a.inputFrames?rotation:a.observed.rotation;
        if(!a.observed.ready||!a.observed.alive){a.reason="awaiting_original_alive_ready_epoch";a.target=-1;a.pathCount=0;continue;}
        if(config.active){
            if(config.mode==Mode::HoldAngle)HoldStrategy(index,now);
            Combat(index,now,dt);if(config.mode==Mode::Deathmatch)DeathmatchStrategy(index,now);
            if(a.target<0&&Length2D(a.output.worldDirection)>.1){const double desired=std::atan2(a.output.worldDirection.y,a.output.worldDirection.x)*180/3.14159265358979323846;
                a.output.rotation.y+=Clamp(Delta(desired,a.output.rotation.y),-a.profile.turnSpeed*dt,a.profile.turnSpeed*dt);}
        }
        if(a.output.crouch){
            ++a.crouchRequestedSamples;if(a.observed.crouched){++a.crouchObservedSamples;a.crouchSince=-1;}
            else if(a.crouchSince<0)a.crouchSince=now;
            else if(now-a.crouchSince>.5){Change(a,Phase::Blocked,now);a.waitUntil=now+2;a.reason="original_crouch_input_not_consumed";a.output={};a.output.rotation=rotation;}
        }else a.crouchSince=-1;
        if(a.output.jump){++a.jumpRequestedSamples;if(a.observed.jumping||a.observed.movementType==2||a.observed.velocity.z>10)++a.jumpObservedSamples;}
        if(!NativeSubmit(index,static_cast<float>(dt)))return Fail(2404,"training_bots_original_input_submission_failed");
    }
    ++diagnostics.ticks;diagnostics.error=0;diagnostics.stage="training_bots_original_inputs_submitted_runtime_acceptance_pending";return true;
}
inline bool Tick(double now){__try{return TickImpl(now);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool Stop(){
    if(!Thread())return false;config.active=false;
    for(size_t index=0;index<MaxActors;++index){auto& a=actors[index];const auto rotation=a.output.rotation;a.output={};a.output.rotation=rotation;a.target=-1;
        if(a.bound&&a.bot&&a.observed.ready&&a.observed.alive&&!NativeSubmit(index))return false;}
    return true;
}
inline bool Unbind(size_t index){
    if(!Thread()||index>=MaxActors)return false;
    auto& a=actors[index];
    // Root calls this while the old pawn's identity is still valid, before
    // damage/respawn replaces it. No native call dereferences a stale corpse.
    if(a.bound&&a.bot){
        if(hooks.weaponInput&&!hooks.weaponInput(index,false,false))return false;
        if(a.observed.ready&&a.pawn==original_players::players[index].character){
            const auto rotation=a.output.rotation;a.output={};a.output.rotation=rotation;
            if(!NativeSubmit(index))return false;
        }
    }
    a={};nextObservation=0;return true;
}
inline void WriteRouteValidations(FILE* f){
    const auto& progress=routeValidationProgress;
    std::fprintf(f,",\"route_validation\":{\"request\":%llu,\"active\":%s,\"complete\":%s,\"total\":%zu,\"checked\":%zu,\"valid\":%zu,\"blocked\":%zu,\"pending\":%zu,\"requested_at\":%.9g,\"completed_at\":%.9g,\"reason\":",
        static_cast<unsigned long long>(progress.request),progress.active?"true":"false",progress.complete?"true":"false",
        progress.total,progress.checked,progress.valid,progress.blocked,progress.pending,progress.requestedAt,progress.completedAt);
    original_players::Text(f,progress.reason);
    std::fprintf(f,",\"requests\":%llu,\"route_checks\":%llu,\"nav_queries\":%llu,\"capsule_ground_queries\":%llu,\"routes\":[",
        static_cast<unsigned long long>(diagnostics.routeValidationRequests),static_cast<unsigned long long>(diagnostics.routeValidationChecks),
        static_cast<unsigned long long>(diagnostics.routeValidationNavQueries),static_cast<unsigned long long>(diagnostics.routeValidationLocalQueries));
    for(size_t index=0;index<routeCount;++index){const auto& result=routeValidations[index];
        std::fprintf(f,"%s{\"id\":%u,\"index\":%zu,\"request\":%llu,\"status\":",index?",":"",result.id,index,static_cast<unsigned long long>(result.request));
        original_players::Text(f,RouteCheckStatusName(result.status));std::fputs(",\"reason\":",f);original_players::Text(f,result.reason);
        std::fprintf(f,",\"complete\":%s,\"checked_at\":%.9g,\"validator_index\":%d,\"validator_epoch\":%llu,\"standing_capsule_radius\":%.9g,\"standing_capsule_half_height\":%.9g,\"effective_style\":%u,\"failed_segment\":%d,\"segments\":[",
            result.complete?"true":"false",result.checkedAt,result.validatorIndex<MaxActors?static_cast<int>(result.validatorIndex):-1,
            static_cast<unsigned long long>(result.validatorEpoch),result.capsuleRadius,result.capsuleHalfHeight,static_cast<unsigned>(result.effectiveStyle),result.failedSegment);
        for(size_t n=0;n<result.segmentCount;++n){const auto& segment=result.segments[n];
            std::fprintf(f,"%s{\"index\":%zu,\"status\":",n?",":"",n);original_players::Text(f,RouteCheckStatusName(segment.status));
            std::fputs(",\"method\":",f);original_players::Text(f,RouteCheckMethodName(segment.method));
            std::fputs(",\"reason\":",f);original_players::Text(f,segment.reason);std::fputs(",\"nav_reason\":",f);original_players::Text(f,segment.navReason);
            std::fprintf(f,",\"nav_attempted\":%s,\"nav_partial\":%s,\"nav_point_count\":%d,\"nav_start_error_cm\":%.9g,\"nav_end_error_cm\":%.9g,\"capsule_ground_attempted\":%s,\"from\":",
                segment.navAttempted?"true":"false",segment.navPartial?"true":"false",segment.navPointCount,segment.navStartErrorCm,segment.navEndErrorCm,segment.localAttempted?"true":"false");
            original_players::Vector(f,segment.from);std::fputs(",\"to\":",f);original_players::Vector(f,segment.to);std::fputc('}',f);
        }
        std::fputs("]}",f);
    }
    std::fputs("]}",f);
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    const auto b=[](bool v){return v?"true":"false";};
    std::fputs("{\"schema_version\":1,\"stage\":",f);original_players::Text(f,diagnostics.stage);
    std::fprintf(f,",\"error\":%u,\"initialized\":%s,\"active\":%s,\"mode\":%u,\"style\":%u,\"return_fire\":%s,\"local_invincible_rule_requested\":%s,\"peeks_per_minute\":%.9g,\"route_count\":%zu,\"anchor_count\":%zu,\"nav_system_present\":%s,\"original_path_succeeded\":%s,\"validated_local_segment_succeeded\":%s,\"ticks\":%u,\"observation_failures\":%u,\"weapon_adapter_missing\":%u,\"map_key\":",diagnostics.error,b(diagnostics.initialized),b(config.active),static_cast<unsigned>(config.mode),static_cast<unsigned>(config.style),b(config.returnFire),b(config.localInvincible),config.peeksPerMinute,routeCount,anchorCount,b(diagnostics.navSystemPresent),b(diagnostics.originalPathSucceeded),b(diagnostics.externalSegmentSucceeded),diagnostics.ticks,diagnostics.observationFailures,diagnostics.weaponUnavailable);original_players::Text(f,currentMap);
    std::fprintf(f,",\"seed\":%u,\"reaction_ms\":%.9g,\"reaction_spread_ms\":%.9g,\"aim_speed\":%.9g,\"aim_speed_spread\":%.9g,\"accuracy\":%.9g,\"accuracy_spread\":%.9g,\"movement\":%.9g,\"movement_spread\":%.9g,\"config_updates\":%u,\"profile_refreshes\":%u,\"plan_resets\":%u",config.seed,config.reactionMs,config.reactionSpreadMs,config.aimSpeed,config.aimSpeedSpread,config.accuracy,config.accuracySpread,config.movement,config.movementSpread,diagnostics.configUpdates,diagnostics.profileRefreshes,diagnostics.planResets);
    std::fprintf(f,",\"spawn_route_hints\":%llu,\"spawn_route_hint_completions\":%llu,\"spawn_route_hint_invalidations\":%llu,\"profile_identity\":\"match_seed_and_roster_index\",\"behavior_identity\":\"match_seed_roster_index_full_spawn_epoch\"",
        static_cast<unsigned long long>(diagnostics.spawnRouteHints),static_cast<unsigned long long>(diagnostics.spawnRouteHintCompletions),static_cast<unsigned long long>(diagnostics.spawnRouteHintInvalidations));
    WriteRouteValidations(f);
    std::fputs(",\"native_facing\":",f);original_npc_facing::WriteSummary(f);
    std::fputs(",\"actors\":[",f);bool first=true;
    for(size_t index=0;index<MaxActors;++index){const auto& a=actors[index];if(!a.bound)continue;if(!first)std::fputc(',',f);first=false;
        std::fprintf(f,"{\"index\":%zu,\"bot\":%s,\"spawn_epoch\":%llu,\"ready\":%s,\"alive\":%s,\"phase\":%u,\"route_index\":%d,\"target\":%d,\"nav_path\":%s,\"validated_local_path\":%s,\"cycles\":%u,\"blocked\":%u,\"path_queries\":%u,\"input_frames\":%u,\"crouch_requested_samples\":%u,\"crouch_observed_samples\":%u,\"jump_requested_samples\":%u,\"jump_observed_samples\":%u,\"player_input_present\":%s,\"walk_speed\":%.9g,\"run_speed\":%.9g,\"crouch_speed\":%.9g,\"jump_velocity\":%.9g,\"strength\":%.9g,\"reaction_seconds\":%.9g,\"turn_degrees_per_second\":%.9g,\"aim_error_degrees\":%.9g,\"reason\":",index,b(a.bot),static_cast<unsigned long long>(a.epoch),b(a.observed.ready),b(a.observed.alive),static_cast<unsigned>(a.phase),a.route,a.target,b(a.navPath),b(a.localPath),a.cycles,a.blocked,a.navQueries,a.inputFrames,a.crouchRequestedSamples,a.crouchObservedSamples,a.jumpRequestedSamples,a.jumpObservedSamples,b(a.observed.playerInputPresent),a.observed.walkSpeed,a.observed.runSpeed,a.observed.crouchSpeed,a.observed.jumpVelocity,a.profile.strength,a.profile.reaction,a.profile.turnSpeed,a.profile.errorDegrees);original_players::Text(f,a.reason);
        std::fprintf(f,",\"profile_seed\":%u,\"sampled_accuracy\":%.9g,\"sampled_movement\":%.9g,\"native_input_ready\":%s,\"local_input_unchanged\":%s,\"input_initializations\":%u,\"input_transports\":%u,\"input_events\":%u,\"jump_queue_calls\":%u,\"jump_setter_calls\":%u",a.profileSeed,a.profile.accuracy,a.profile.movement,b(a.nativeInputReady),b(a.localInputUnchanged),a.inputInitializations,a.inputTransports,a.inputEvents,a.jumpQueueCalls,a.jumpSetterCalls);
        std::fprintf(f,",\"spawn_route_hint_id\":%u,\"spawn_route_hint_pending\":%s,\"spawn_route_hint_completed\":%s",a.spawnRouteHintId,b(a.spawnRouteHintPending),b(a.spawnRouteHintCompleted));
        std::fputs(",\"native_facing\":",f);original_npc_facing::WriteEntry(f,index);
        std::fputs(",\"feet\":",f);original_players::Vector(f,a.observed.feet);std::fputs(",\"nav_agent_location\":",f);original_players::Vector(f,a.observed.navAgent);std::fputs(",\"goal\":",f);original_players::Vector(f,a.goal);std::fputc('}',f);
    }
    std::fprintf(f,"],\"restart_required\":%s,\"exception_rva\":%llu,\"failed_event\":",b(diagnostics.restartRequired),static_cast<unsigned long long>(diagnostics.exceptionRva));const auto method=original_players::Utf8(diagnostics.failedEvent);original_players::Text(f,method.c_str());
    std::fprintf(f,",\"expected_event_size\":%u,\"actual_event_size\":%u,\"native_tactical_ai_reused\":false,\"runtime_pixels_verified\":false}\n",static_cast<unsigned>(diagnostics.expectedSize),static_cast<unsigned>(diagnostics.actualSize));
    const bool ok=std::ferror(f)==0;std::fclose(f);return ok;
}
}
