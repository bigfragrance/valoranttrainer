#pragma once
#include "original_players.h"
#include "local_server_npc_identity.h"
#include "training_control.h"
#include "training_bot_controller.h"
#include "training_world_geometry.h"
#include "training_maps.h"
#include "deathmatch_rules.h"
#include "local_server_hero_selection.h"
#include "local_server_bot_roster_initial.h"
#include "local_server_feedback_ledger.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>

// Authority adaptation for the separate loopback host. No Engine.LocalPlayer
// is created, transferred, or borrowed here. Slot zero is the PC/PS/Pawn that
// native login admitted; NPC PCs have Player == nullptr and native Role 3.
// Existing standalone helpers keep their guards and remain regression code.
namespace local_server_match {
using Vec=engine::Vec;
using Weak=original_inventory_slots::WeakIdentity;
using Phase=deathmatch_rules::Phase;
using EndReason=deathmatch_rules::EndReason;
using Frame=training_bot_controller::Frame;
inline constexpr size_t MaxPlayers=original_players::MaxPlayers;
inline constexpr uint32_t KillLimit=40;
inline constexpr double DurationSeconds=540;

struct Hooks {
    bool (*registerEquipment)(size_t,void* pc,void* ps,void* pawn,uint32_t epoch,bool human)=nullptr;
    bool (*beforeRetire)(size_t,void* pawn,uint32_t epoch)=nullptr;
    bool (*submitBot)(size_t,const Frame&,double delta)=nullptr;
    // Sole perception pass, after current original observations and before decisions.
    bool (*sensorPass)(double matchNow)=nullptr;
    // Optional replacement inside the same NPC scheduler. Unhandled uses the
    // legacy strategy; a handled all-false frame is an explicit release.
    bool (*decideBot)(size_t,double matchNow,Frame&,bool&handled)=nullptr;
    bool (*botSubmitted)(size_t,const Frame&)=nullptr;
    bool (*weaponInput)(size_t,bool fire,bool reload)=nullptr;
    bool (*canDamage)(size_t source,size_t victim)=nullptr;
    void (*actorSpawned)(size_t,void* pc,void* ps,void* pawn,uint32_t epoch,bool human)=nullptr;
    void (*matchEnded)(int32_t winner,EndReason reason)=nullptr;
    void* (*spawnHuman)(void* pc,void* ps,const Vec& feet,double yaw)=nullptr;
    void (*afterScoreCommit)(uint64_t token,size_t source,size_t victim)=nullptr;
    // Registration may enqueue an asynchronous native inventory/loadout. The
    // optional completion query keeps the initial match clock behind that work.
    bool (*equipmentReady)(size_t,void* pawn,uint32_t epoch)=nullptr;
    // A separately verified nonlocal authority driver can own native fixed
    // steps without changing the real Pawn.IsLocallyControlled result.
    bool (*npcAuthorityReady)(size_t,void* pawn,uint32_t epoch)=nullptr;
    bool (*afterDeath)(size_t,void* pawn,void* ps,uint32_t epoch)=nullptr;
    // Read-only current-PC/PS policy. Null preserves historical integrations.
    bool (*isObserver)(size_t index,void* pc,void* ps)=nullptr;
};
struct AuthorityPlayer {
    bool present=false,human=false,ready=false,alive=false,equipmentRegistered=false,equipmentReady=false;
    bool inputPrepared=false,nativeLocallyControlled=false,deathAccounted=false;
    bool externalAuthorityReady=false;
    bool movementObserved=false,nativeStreakBound=false;
    bool playerIdAssigned=false,uniqueIdPreserved=false,teamPointsVerified=false;
    bool assignedTeamStateVerified=false,nativeSelectionVerified=false,nativePortraitReady=false;
    size_t index=0;uint32_t epoch=0,hero=0,logicalId=0;
    int32_t playerId=-1,initialPlayerId=-1;
    void *pc=nullptr,*ps=nullptr,*pawn=nullptr,*info=nullptr,*stats=nullptr;
    void *team=nullptr,*teamActor=nullptr;
    void *nativePortrait=nullptr;
    Weak pawnIdentity{},pcIdentity{},psIdentity{};
    Weak teamIdentity{},teamActorIdentity{};
    Phase phase=Phase::Absent;
    uint32_t kills=0,deaths=0,streak=0,bestStreak=0,playerIdRevalidations=0;
    uint64_t inputFrames=0,moveRequests=0,movingSamples=0,faceCalls=0;
    uint64_t jumpRequests=0,crouchRequests=0;
    uint64_t externalInputSubmissions=0,externalMoveRequests=0,externalJumpRequests=0,externalCrouchRequests=0;
    int32_t pendingInputCommands=0;
    uint8_t role=0,remoteRole=0;
    float health=0,maxHealth=0,nativeWindowRate=-1,nativeWindowRemaining=-1;
    Vec feet{},spawnFeet{},lastFeet{};double yaw=0;
    double lastSample=-1,lastKill=-1,deathAt=-1,respawnAt=-1,firstMoveAt=-1,nextPresentationRead=0;
    int spawnRoute=-1;
    const char* stage="server_player_absent";
};
struct State {
    bool initialized=false,started=false,ready=false,active=false,ended=false;
    bool humanRegistered=false,mapsReady=false,rosterReady=false,restartRequired=false;
    bool originalFFAConfirmed=false,uniqueHeroes=false,npcSimulationObserved=false;
    bool identityProfileReady=false,teamProfileReady=false,teamRecordsReady=false;
    unsigned error=0;uint32_t participants=0,wantedPlayers=0,spawnedNPCs=0;
    uint32_t respawns=0,humanRespawns=0,kills=0,externalDeaths=0,duplicateTokens=0,policyRejects=0;
    uint32_t shuffleSeed=0,polls=0,movementObservedNPCs=0;
    uint32_t playerIdsAssigned=0,playerIdReadbacks=0,teamActorsCreated=0,teamPointsWrites=0,teamPointsNotifications=0;
    uint32_t teamMembershipNotifications=0,assignedTeamStateWrites=0,assignedTeamStateReadbacks=0,practiceTeams=0;
    int netMode=-1,leader=-1,winner=-1;uint32_t leadingKills=0,leadingTies=0;
    double now=0,startedAt=-1,remaining=DurationSeconds,accumulatedDelta=0;
    uint64_t botInputFrames=0,externalBotInputSubmissions=0;uintptr_t exceptionRva=0;
    EndReason endReason=EndReason::None;
    void *world=nullptr,*gameState=nullptr;
    const char* stage="server_match_not_initialized";
    const char* operation="none";
};
struct PendingDamage {
    bool active=false;uint64_t token=0;size_t source=0,victim=0;
    void* equippable=nullptr;double now=0;uint32_t nextStreak=0;
    int32_t killerOrdinal=0,victimOrdinal=0,victimFeedbackCount=0;
};
struct Score {
    uint32_t index=0,epoch=0,kills=0,deaths=0,streak=0;
    void* playerState=nullptr;bool human=false,alive=false;
    float nativeWindowRate=-1,nativeWindowRemaining=-1;
    int32_t playerId=-1;
};
struct ObjectArray {void** data=nullptr;int32_t count=0,capacity=0;};
struct TeamInfo {uint8_t initialRole=0,teamRole=0,padding[2]{};int32_t wins=0,points=0,teamLife=0;};
static_assert(sizeof(ObjectArray)==16&&sizeof(TeamInfo)==16&&offsetof(TeamInfo,points)==8);
struct NativeRecords {
    void *getPlayerId=nullptr,*onRepPlayerId=nullptr;
    void *getTeamState=nullptr,*getPoints=nullptr,*onRepTeamComponent=nullptr;
    void *onRepTeamPlayers=nullptr,*onRepTeamStates=nullptr,*onRepTeamInfo=nullptr;
    void *teamStateClass=nullptr,*getAssignedTeamState=nullptr,*onRepAssignedTeamState=nullptr;
    void *getDesiredClass=nullptr,*getDesiredData=nullptr,*getPortrait=nullptr;
    int32_t uniqueIdBytes=0,pointsLimitOffset=-1;
};
inline State state;
inline Hooks hooks;
inline training_control::Initial initial{};
inline std::array<AuthorityPlayer,MaxPlayers> players{};
inline std::array<uint32_t,MaxPlayers> assignments{};
inline std::array<uint64_t,1024> committedTokens{};
inline size_t tokenCursor=0;
inline PendingDamage pending;
inline NativeRecords records;
inline DWORD gameThread=0;
inline double nextObservation=0;
inline const State& GetState(){return state;}
inline const std::array<AuthorityPlayer,MaxPlayers>& AuthorityPlayers(){return players;}
inline const AuthorityPlayer* Player(size_t index){return index<players.size()&&players[index].present?&players[index]:nullptr;}
inline const training_control::Initial& Configuration(){return initial;}
inline bool IsObserver(size_t index){
    const auto* p=Player(index);
    return index==0&&p&&p->human&&hooks.isObserver&&hooks.isObserver(index,p->pc,p->ps);
}
inline bool TargetAllowed(size_t source,size_t target){
    return Player(source)&&Player(target)&&!IsObserver(source)&&!IsObserver(target);
}
template<class T> inline T Read(const void* p,size_t offset){return original_players::Value<T>(p,offset);}
inline double Now(){return static_cast<double>(GetTickCount64())/1000.;}
inline bool Thread(){return state.initialized&&!state.restartRequired&&gameThread==GetCurrentThreadId()&&original_players::Thread();}
inline bool Fail(unsigned code,const char* why){state.error=code?code:3290;state.stage=why;state.ready=false;return false;}
inline bool Event(void* object,const wchar_t* method,uint16_t bytes,void* params){return original_enemies::Event(object,method,bytes,params);}
inline bool FreeForAll(){return initial.config.mode==static_cast<uint32_t>(training_control::Mode::Deathmatch);}
// The initial all-player barrier starts the match exactly once in PollImpl.
// Later readiness describes individual spawn/equipment work. A dead or
// rebuilding NPC must not disable damage between other live participants.
// CanDamage and the runtime still validate both current player lives/loadouts.
inline bool Active(){return Thread()&&state.started&&state.startedAt>=0&&state.active&&!state.ended;}
inline bool AbilitiesAllowed(){return Active()&&!FreeForAll();}
inline Vec Vector(const training_control::Point& p){return {p.x,p.y,p.z};}
inline bool Valid(const training_control::Configuration& c){
    const auto finite=[](float x,float low,float high){return std::isfinite(x)&&x>=low&&x<=high;};
    return c.mode<=1&&c.totalPlayers>=2&&c.totalPlayers<=MaxPlayers&&(c.flags&~15U)==0&&c.style<=5&&c.seed
        &&(c.mode==0||c.totalPlayers<=original_heroes::PlayableHeroCount)
        &&finite(c.peeksPerMinute,.5f,180)&&finite(c.respawnSeconds,.2f,15)&&finite(c.streakSeconds,.5f,30)
        &&finite(c.reactionMs,60,2000)&&finite(c.reactionSpreadMs,0,1000)
        &&finite(c.aimDegreesPerSecond,20,1080)&&finite(c.aimSpread,0,800)
        &&finite(c.accuracy,.02f,1)&&finite(c.accuracySpread,.01f,.7f)
        &&finite(c.movement,.05f,1)&&finite(c.movementSpread,.01f,.7f);
}
inline bool ValidRoutes(const training_control::RouteSet& r){
    if(r.count>training_bot_controller::MaxRoutes)return false;
    for(uint32_t i=0;i<r.count;++i){const auto& p=r.items[i];
        if(!p.id||p.style>8||p.enabled>1||!std::isfinite(p.weight)||p.weight<.1f||p.weight>100
           ||!original_players::Finite(Vector(p.start))||!original_players::Finite(Vector(p.expose))
           ||!original_players::Finite(Vector(p.retreat)))return false;
        for(uint32_t j=0;j<i;++j)if(r.items[j].id==p.id)return false;
    }return true;
}
inline bool ValidInitial(const training_control::Initial& c){
    if(c.magic!=training_control::Magic||c.version!=training_control::Version
       ||(c.bytes&&c.bytes!=sizeof(c))||!Valid(c.config)||!ValidRoutes(c.routes)
       ||!c.mapId[0]||!std::memchr(c.mapId,0,sizeof(c.mapId))||c.spawnCount>64)return false;
    const auto map=training_maps::Find(c.mapId);
    if(!map||(c.config.mode==1&&(!map->supportsFFA||!c.spawnCount)))return false;
    for(uint32_t n=0;n<c.spawnCount;++n)if(!original_players::Finite(Vector(c.spawns[n]))||!std::isfinite(c.spawnYaws[n]))return false;
    return true;
}
inline void* Property(void* type,const wchar_t* name,int32_t bytes,int32_t exact){
    uint32_t nativeName[3]{};
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(nativeName,name,1);
    auto field=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,nativeName,3);
    const int32_t offset=Read<int32_t>(field,0x44),size=Read<int32_t>(field,0x34),total=Read<int32_t>(type,0x60);
    if(!field||(Read<uint64_t>(Read<void*>(field,8),0x18)&0x8001)!=0x8001
       ||Read<int32_t>(field,0x30)!=1||(bytes>0&&size!=bytes)||(exact>=0&&offset!=exact)
       ||offset<((exact>=0&&exact<0x30)?0:0x30)||size<=0||total<size||offset>total-size)return nullptr;
    return field;
}
inline bool BindRecordMethod(void*& result,const wchar_t* name,uint16_t bytes,uintptr_t exec=0){
    result=engine::Find(name);
    return result&&Read<uint16_t>(result,0xbe)==bytes
        &&(!exec||((Read<uint32_t>(result,0xb8)&0x400)&&Read<void*>(result,0xe0)==reinterpret_cast<void*>(engine::image+exec)));
}
inline bool InitializeIdentityProfile(){
    if(state.identityProfileReady)return true;
    auto type=engine::Class(0x42aae70);
    auto id=Property(type,L"PlayerId",4,0x468),unique=Property(type,L"UniqueId",0,0x498);
    if(!id||!unique||(Read<uint64_t>(id,0x38)&0x20)==0
       ||!BindRecordMethod(records.getPlayerId,L"/Script/Engine.PlayerState.GetPlayerId",4,0x49452a0)
       ||!BindRecordMethod(records.onRepPlayerId,L"/Script/Engine.PlayerState.OnRep_PlayerId",0,0x3e7a620))
        return Fail(3291,"server_identity_original_player_id_reflection_profile_mismatch");
    records.uniqueIdBytes=Read<int32_t>(unique,0x34);
    if(records.uniqueIdBytes<=0||records.uniqueIdBytes>0x50)return Fail(3292,"server_identity_original_unique_id_boundary_mismatch");
    state.identityProfileReady=true;return true;
}
inline bool NativePlayerId(void* ps,int32_t& value){
    if(!InitializeIdentityProfile()||!Event(ps,L"/Script/Engine.PlayerState.GetPlayerId",4,&value)
       ||value!=Read<int32_t>(ps,0x468))return Fail(3293,"server_identity_native_player_id_getter_readback_failed");
    ++state.playerIdReadbacks;return true;
}
inline bool PlayerIdInUse(int32_t id,void* except){
    const auto roster=Read<ObjectArray>(state.gameState,0x480);
    if(roster.count<0||roster.count>4096||roster.capacity<roster.count
       ||(roster.count&&!original_players::Readable(roster.data,static_cast<size_t>(roster.count)*8)))return true;
    for(int32_t n=0;n<roster.count;++n)if(roster.data[n]!=except&&Read<int32_t>(roster.data[n],0x468)==id)return true;
    return false;
}
inline bool AssignPlayerIdentity(AuthorityPlayer& p){
    int32_t actual=-1;if(!NativePlayerId(p.ps,actual))return false;
    if(p.playerIdAssigned){if(actual!=p.playerId)return Fail(3294,"server_identity_player_state_id_changed_across_spawn_epochs");
        ++p.playerIdRevalidations;return true;}
    p.initialPlayerId=actual;
    if(p.human){p.playerId=actual;p.playerIdAssigned=p.uniqueIdPreserved=true;return true;}
    const auto roster=Read<ObjectArray>(state.gameState,0x480);
    if(roster.count<0||roster.count>4096||roster.capacity<roster.count||roster.capacity>4096
       ||(roster.count&&!original_players::Readable(roster.data,static_cast<size_t>(roster.count)*8)))
        return Fail(3296,"server_identity_npc_player_id_allocation_requires_valid_native_roster");
    std::array<uint8_t,0x50> uniqueBefore{},uniqueAfter{};
    if(!original_players::Readable(static_cast<uint8_t*>(p.ps)+0x498,static_cast<size_t>(records.uniqueIdBytes)))
        return Fail(3295,"server_identity_original_unique_id_storage_unreadable");
    std::memcpy(uniqueBefore.data(),static_cast<uint8_t*>(p.ps)+0x498,static_cast<size_t>(records.uniqueIdBytes));
    if(Read<void*>(Read<void**>(p.ps,0),0x770)!=reinterpret_cast<void*>(engine::image+0x18a3a00))
        return Fail(3313,"server_identity_original_player_id_notification_leaf_changed");
    int32_t candidate=1;while(candidate<=4097&&PlayerIdInUse(candidate,p.ps))++candidate;
    if(candidate>4097)return Fail(3296,"server_identity_no_free_positive_session_player_id_or_invalid_roster");
    // Canonical missing authority data only. Original GameSession.RegisterPlayer
    // itself stores this int at443a873, then modifies UniqueId/account fields.
    // NPCs intentionally receive only the session-local integer, not that flow.
    if(!original_players::Write(p.ps,0x468,candidate)
       ||!Event(p.ps,L"/Script/Engine.PlayerState.OnRep_PlayerId",0,nullptr)||!NativePlayerId(p.ps,actual)||actual!=candidate)
        return Fail(3297,"server_identity_npc_player_id_assignment_not_observed");
    std::memcpy(uniqueAfter.data(),static_cast<uint8_t*>(p.ps)+0x498,static_cast<size_t>(records.uniqueIdBytes));
    p.uniqueIdPreserved=std::memcmp(uniqueBefore.data(),uniqueAfter.data(),static_cast<size_t>(records.uniqueIdBytes))==0;
    if(!p.uniqueIdPreserved)return Fail(3298,"server_identity_npc_player_id_changed_account_identity_storage");
    p.playerId=actual;p.playerIdAssigned=true;++state.playerIdsAssigned;return true;
}
inline bool ReadLife(AuthorityPlayer& p){
    return deathmatch_rules::ReadLife(p.pawn,p.alive,p.health,p.maxHealth)
        ?true:Fail(deathmatch_rules::State().error,deathmatch_rules::State().stage);
}
inline bool AuthorityPair(const AuthorityPlayer& p,bool requirePossession=true){
    if(!p.present||!original_players::IsA(p.pc,original_players::ShooterControllerClass)
       ||!original_players::IsA(p.ps,original_players::ShooterStateClass)
       ||!original_players::IsA(p.pawn,original_players::ShooterCharacterClass)
       ||Read<uint8_t>(p.pc,0x224)!=3||Read<uint8_t>(p.pawn,0x224)!=3
       ||Read<void*>(p.pc,0x470)!=p.ps||Read<void*>(p.ps,0x210)!=p.pc
       ||Read<void*>(p.pawn,0x488)!=p.ps||!original_players::Registered(p.ps))return false;
    if(requirePossession&&(Read<void*>(p.pc,0x4b0)!=p.pawn||Read<void*>(p.pawn,0x4a0)!=p.pc))return false;
    if(!p.human&&Read<void*>(p.pc,0x510))return false;
    if(p.playerIdAssigned&&Read<int32_t>(p.ps,0x468)!=p.playerId)return false;
    return original_inventory_slots::ResolveIdentity(p.pcIdentity)==p.pc
        &&original_inventory_slots::ResolveIdentity(p.psIdentity)==p.ps
        &&original_inventory_slots::ResolveIdentity(p.pawnIdentity)==p.pawn;
}
// Admission is checked against the actual current match registry and its
// existing native weak identities, not a caller-provided NPC pointer alone.
inline bool ValidateNpcIdentityTuple(const local_server_npc_identity::Tuple& t,bool firstNpcMark){
    if(!Thread()||state.world!=local_server_npc_identity::GetState().world||t.index>=players.size()
       ||!t.epoch||t.human!=(t.index==0)||(!t.human&&t.index>=state.wantedPlayers))return false;
    const auto& p=players[t.index];
    if(!p.present||p.index!=t.index||p.pc!=t.pc||p.ps!=t.ps||p.pawn!=t.pawn||p.epoch!=t.epoch
       ||p.human!=t.human||!AuthorityPair(p))return false;
    // This is called synchronously from BindPlayer, before CacheRoster,
    // equipment registration, actorSpawned or the first registry collection.
    return !firstNpcMark||(!p.human&&p.epoch==1&&!p.ready&&!p.equipmentRegistered);
}
inline bool Project(const Vec& point,Vec& projected){
    struct Query {void* world;Vec point;Vec projected{};void* nav=nullptr;void* filter=nullptr;Vec extent{160,160,800};uint8_t result=0,pad[7]{};} q{state.world,point};
    static_assert(sizeof(Query)==104&&offsetof(Query,result)==96);
    if(!Event(engine::DefaultObject(engine::Class(0x4f7fff0)),L"/Script/NavigationSystem.NavigationSystemV1.K2_ProjectPointToNavigation",97,&q)
       ||!q.result||!original_players::Finite(q.projected))return false;
    projected=q.projected;return true;
}
inline bool Supported(const Vec& point,const original_players::CharacterDefaults& d,Vec& feet){
    Vec projected{};
    const bool floor=Project(point,projected)?training_world_geometry::Ground(projected,feet,30,100)
                                           :training_world_geometry::Ground(point,feet);
    return floor&&training_world_geometry::CapsuleClear(feet,feet,d.capsuleRadius,d.capsuleHalfHeight);
}
inline int PickSpawnRoute(size_t index,uint32_t epoch){
    double total=0;for(uint32_t n=0;n<initial.routes.count;++n)if(initial.routes.items[n].enabled)total+=initial.routes.items[n].weight;
    if(total<=0)return -1;
    const auto sample=training_bot_controller::Mix(initial.config.seed^static_cast<uint32_t>(index*0x9e3779b9ULL)^epoch*0x85ebca6bU);
    double choice=(static_cast<double>(sample)+.5)/4294967296.*total;int last=-1;
    for(uint32_t n=0;n<initial.routes.count;++n)if(initial.routes.items[n].enabled&&initial.routes.items[n].weight>0){
        last=static_cast<int>(n);choice-=initial.routes.items[n].weight;if(choice<=0)return last;
    }return last;
}
inline bool SelectSpawn(size_t index,uint32_t epoch,deathmatch_rules::SpawnSelection& out){
    if(index>=state.wantedPlayers||!epoch)return Fail(3200,"server_spawn_invalid_slot_or_epoch");
    const uint32_t hero=index?assignments[index]:local_server_hero_selection::SelectionIndex();
    original_players::CharacterDefaults defaults;
    if(hero>=original_heroes::PlayableHeroCount||!original_players::ProbeDefaults(original_heroes::Paths[hero],defaults))
        return Fail(3201,"server_spawn_original_hero_dimensions_unavailable");
    out={};out.basis=deathmatch_rules::PositionBasis::Feet;players[index].spawnRoute=-1;
    if(!FreeForAll()&&index&&initial.routes.count){
        const int route=PickSpawnRoute(index,epoch);
        if(route>=0){const auto& r=initial.routes.items[static_cast<size_t>(route)];
            if(!Supported(Vector(r.start),defaults,out.position))return Fail(3202,"server_spawn_selected_route_has_no_native_ground_or_capsule_clearance");
            const auto expose=Vector(r.expose);out.yaw=std::atan2(expose.y-out.position.y,expose.x-out.position.x)*180/3.14159265358979323846;
            players[index].spawnRoute=route;return true;
        }
    }
    if(initial.spawnCount){
        const size_t first=(index+static_cast<size_t>(epoch-1)*13+initial.config.seed)%initial.spawnCount;
        bool found=false;double best=-1;
        for(size_t n=0;n<initial.spawnCount;++n){const size_t at=(first+n)%initial.spawnCount;Vec candidate{};
            if(!Supported(Vector(initial.spawns[at]),defaults,candidate))continue;
            double nearest=1e9;
            for(const auto& other:players)if(other.present&&other.alive&&other.index!=index){
                nearest=(std::min)(nearest,training_bot_controller::Distance(other.feet,candidate));
            }
            if(nearest>best){best=nearest;out.position=candidate;out.yaw=initial.spawnYaws[at];found=true;}
            if(!FreeForAll()||nearest>2500)break;
        }
        if(!found)return Fail(3203,"server_spawn_no_original_anchor_with_native_ground_and_capsule_clearance");
        out.originalMapAnchor=true;return true;
    }
    // The existing Duel practice defaults are still subjected to the real map
    // ground and capsule queries. No actor is teleported during an AI route.
    Vec candidate=index?Vec{-900,(static_cast<double>(index)-2)*300,200}:players[0].feet;
    if(!Supported(candidate,defaults,out.position))return Fail(3204,"server_practice_spawn_requires_supported_native_location");
    out.yaw=index?180:players[0].yaw;return true;
}
inline training_bot_controller::Config BotConfig(){
    const auto& c=initial.config;training_bot_controller::Config b;
    b.mode=FreeForAll()?training_bot_controller::Mode::Deathmatch:training_bot_controller::Mode::HoldAngle;
    b.style=static_cast<training_bot_controller::Style>(c.style);b.active=state.active&&!state.ended&&!(c.flags&training_control::Paused);
    b.returnFire=FreeForAll()||(c.flags&training_control::ReturnFire)!=0;b.localInvincible=(c.flags&training_control::Invincible)!=0;
    b.peeksPerMinute=c.peeksPerMinute;b.reactionMs=c.reactionMs;b.reactionSpreadMs=c.reactionSpreadMs;
    b.aimSpeed=c.aimDegreesPerSecond;b.aimSpeedSpread=c.aimSpread;b.accuracy=c.accuracy;b.accuracySpread=c.accuracySpread;
    b.movement=c.movement;b.movementSpread=c.movementSpread;b.seed=c.seed;return b;
}
inline bool Observe(size_t index,training_bot_controller::Observation& o){
    if(index>=players.size()||!players[index].present){o={};return true;}
    if(!training_bot_controller::NativeObserve(index,o))return false;
    o.spawnEpoch=players[index].epoch;return true;
}
inline bool Visible(size_t source,size_t target,const Vec& from,const Vec& to){
    if(!TargetAllowed(source,target)||!players[source].alive||!players[target].alive)return false;
    alignas(16) uint8_t params[0x178]{};
    original_players::Put(params,0,state.world);original_players::Put(params,8,from);original_players::Put(params,0x20,to);
    params[0x39]=1;params[0x148]=1;std::array<void*,128> ignored{};
    const auto list=training_world_geometry::Ignore(ignored);original_players::Put(params,0x40,list);
    if(!Event(engine::DefaultObject(engine::Class(0x45aa110)),L"/Script/Engine.KismetSystemLibrary.LineTraceSingle",0x171,params))return false;
    if(!params[0x170]||!(params[0x105]&1))return true;
    return Read<float>(params,0x60)>=training_bot_controller::Distance(from,to)-1;
}
inline bool ApplyRoutes(){
    std::array<training_bot_controller::Route,training_bot_controller::MaxRoutes> routes{};
    for(uint32_t n=0;n<initial.routes.count;++n){const auto& source=initial.routes.items[n];auto& r=routes[n];
        r.id=source.id;strcpy_s(r.mapKey,initial.mapId);r.start=Vector(source.start);r.expose=Vector(source.expose);r.retreat=Vector(source.retreat);
        r.enabled=source.enabled!=0;r.weight=source.weight;r.style=static_cast<training_bot_controller::Style>(source.style);
    }
    return training_bot_controller::ValidPracticePlan(initial.routes)&&training_bot_controller::SetRoutes(initial.mapId,routes.data(),initial.routes.count)&&training_bot_controller::SetPracticePlan(initial.routes)
        ?true:Fail(training_bot_controller::State().error,training_bot_controller::State().stage);
}
inline bool PrepareNPCInput(size_t index){
    auto& p=players[index];if(p.human||!AuthorityPair(p))return Fail(3205,"server_npc_input_requires_authority_controller_without_player");
    uint8_t local=0;
    if(!Event(p.pawn,L"/Script/Engine.Pawn.IsLocallyControlled",1,&local))return Fail(3206,"server_npc_original_local_control_query_failed");
    p.nativeLocallyControlled=local!=0;
    if(!p.nativeLocallyControlled){p.stage="server_npc_original_authority_tick_gate_unconfirmed";return true;}
    auto table=Read<void**>(p.pc,0);auto input=Read<void*>(p.pc,0x5e8);
    if(Read<void*>(table,0xfe8)!=reinterpret_cast<void*>(engine::image+0x781cbd0))
        return Fail(3207,"server_npc_original_input_initializer_changed");
    const auto humanPC=players[0].pc,humanPawn=players[0].pawn,humanConnection=Read<void*>(humanPC,0x510);
    if(!input||!Read<void*>(p.pc,0x230))engine::function<void(*)(void*)>(0x781cbd0)(p.pc);
    input=Read<void*>(p.pc,0x5e8);
    if(Read<void*>(p.pc,0x510)||Read<uint8_t>(p.pawn,0x224)!=3||players[0].pc!=humanPC||players[0].pawn!=humanPawn
       ||Read<void*>(humanPC,0x510)!=humanConnection)return Fail(3208,"server_npc_input_initializer_changed_native_ownership");
    p.inputPrepared=original_players::IsA(input,0x64f9c00)&&Read<void*>(input,0x28)==p.pc&&Read<void*>(p.pc,0x230);
    return p.inputPrepared?true:Fail(3209,"server_npc_original_input_objects_not_ready");
}
inline bool BindAI(size_t index){
    auto& p=players[index];if(!p.human&&!PrepareNPCInput(index))return false;
    auto& a=training_bot_controller::actors[index];a={};a.bound=true;a.bot=!p.human;a.pawn=p.pawn;a.epoch=p.epoch;
    a.rosterIndex=static_cast<uint32_t>(index);a.rng=training_bot_controller::IdentitySeed(a);if(!a.rng)a.rng=1;
    a.nativeInputReady=p.inputPrepared;a.localInputUnchanged=true;
    training_bot_controller::MakeProfile(a);training_bot_controller::ResetPlan(a,state.now);a.reason="server_authority_bot_pending_native_observation";
    if(!FreeForAll()&&!p.human&&p.spawnRoute>=0
       &&!training_bot_controller::SetSpawnRouteHint(index,p.epoch,static_cast<size_t>(p.spawnRoute)))
        return Fail(training_bot_controller::State().error,training_bot_controller::State().stage);
    nextObservation=0;return true;
}

// This binder has a real server ownership contract. It does not call or relax
// original_timed_streak::Bind, whose Standalone-only contract remains intact.
// All native construction, persistent frame/timer queries, StartKillWindow,
// reset and destruction below still use the shipped component implementation.
inline bool BindAuthorityStreak(size_t index){
    auto& methods=original_timed_streak::methods;
    auto& p=players[index];
    if(!FreeForAll())return true;
    if(!AuthorityPair(p)||state.netMode<1||state.netMode>2||!original_timed_streak::Initialize(state.world))
        return Fail(3210,"server_streak_requires_live_authority_player_and_native_factory");
    auto& e=original_timed_streak::entries[index];
    if(e.used)return Fail(3211,"server_streak_previous_spawn_was_not_retired");
    void *actual=nullptr,*info=nullptr,*native=nullptr;
    if(!original_timed_streak::Event(p.pawn,methods.getState,&actual)||actual!=p.ps
       ||!original_timed_streak::Event(p.ps,methods.getInfo,&info)||info!=p.info
       ||!original_timed_streak::Event(info,methods.getStreakComponent,&native)||!original_players::IsA(native,0x6904f00))
        return Fail(3212,"server_streak_original_player_info_identity_mismatch");
    struct Query {void* type;void* result=nullptr;} q{original_timed_streak::state.type};
    if(!original_timed_streak::Event(p.pawn,methods.query,&q))return Fail(3213,"server_streak_original_component_query_failed");
    e.pawn=p.pawn;e.playerState=p.ps;e.playerInfo=info;e.nativeComponent=native;e.epoch=p.epoch;
    if(!original_inventory_slots::CaptureIdentity(p.pawn,e.pawnIdentity)||!original_inventory_slots::CaptureIdentity(p.ps,e.stateIdentity)
       ||!original_inventory_slots::CaptureIdentity(info,e.infoIdentity)||!original_inventory_slots::CaptureIdentity(native,e.nativeIdentity))
        return Fail(3214,"server_streak_original_weak_identity_capture_failed");
    e.used=true;e.component=q.result;
    if(!e.component){struct Add {void* actor;void* type;void* result=nullptr;} add{p.pawn,original_timed_streak::state.type};
        if(!original_timed_streak::Event(original_timed_streak::state.library,methods.add,&add)||!add.result)
            return Fail(3215,"server_streak_original_component_factory_failed");
        e.component=add.result;e.created=true;++original_timed_streak::state.created;
    }else ++original_timed_streak::state.reused;
    if(Read<void*>(e.component,0x10)!=original_timed_streak::state.type||!original_inventory_slots::CaptureIdentity(e.component,e.componentIdentity))
        return Fail(3216,"server_streak_original_component_identity_invalid");
    q.result=nullptr;
    if(!original_timed_streak::Event(p.pawn,methods.query,&q)||q.result!=e.component)return Fail(3217,"server_streak_component_not_returned_by_original_query");
    e.resetDelegate.object=e.componentIdentity;
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&e.resetDelegate.function,L"ResetKillStreak",1);
    e.bound=true;++original_timed_streak::state.bound;++original_timed_streak::state.binds;
    original_timed_streak::Snapshot observed;
    if(!original_timed_streak::Observe(index,observed))return Fail(original_timed_streak::State().error,original_timed_streak::State().stage);
    p.nativeStreakBound=true;p.streak=static_cast<uint32_t>(observed.count);
    p.nativeWindowRate=observed.timerRate;p.nativeWindowRemaining=observed.timerRemaining;return true;
}
inline bool RefreshStreak(size_t index){
    auto& p=players[index];if(FreeForAll()){
        original_timed_streak::Snapshot observed;
        if(!original_timed_streak::Observe(index,observed)||observed.count<0||static_cast<uint32_t>(observed.count)>p.kills)
            return Fail(original_timed_streak::State().error?original_timed_streak::State().error:3218,"server_streak_native_count_or_timer_readback_failed");
        p.streak=static_cast<uint32_t>(observed.count);p.nativeWindowRate=observed.timerRate;p.nativeWindowRemaining=observed.timerRemaining;
        if(!p.streak)p.lastKill=-1;return true;
    }
    if(p.streak&&p.lastKill>=0&&state.now-p.lastKill>=initial.config.streakSeconds){
        void* component=nullptr;
        if(!Event(p.info,L"/Script/ShooterGame.OwnerExclusivePlayerInfo.GetKillStreakComponentMutable",8,&component)
           ||!Event(component,L"/Script/ShooterGame.KillStreakComponent.ResetKills",0,nullptr))return Fail(3219,"server_practice_native_streak_reset_failed");
        p.streak=0;p.lastKill=-1;
    }return true;
}
inline bool PublishLinks(size_t index,void* previous){
    auto& roster=original_players::players[index];
    if(!original_enemy_respawn::ReplaceAssociations(roster,previous))return Fail(original_enemy_respawn::State().error,original_enemy_respawn::State().stage);
    if(!original_enemies::PublishSelection(index))return Fail(original_enemies::error,original_enemies::stage);
    return true;
}
inline bool PublishTeam(size_t index){
    auto& p=players[index];auto team=Read<void*>(p.ps,0x6a8);
    if(FreeForAll())return original_players::IsA(team,deathmatch_rules::FFATeamClass)&&Read<void*>(team,0xc8)==p.ps
        ?true:Fail(3220,"server_ffa_requires_original_ffa_team_component");
    if(!original_players::IsA(team,original_enemies::BombTeamClass)||Read<void*>(team,0xc8)!=p.ps)
        return Fail(3221,"server_practice_original_team_component_missing");
    const uint8_t wanted=p.human?original_enemies::LocalTeam:original_enemies::EnemyTeam;
    if(Read<uint8_t>(team,0xe8)!=wanted&&(!original_players::Write(team,0xe8,wanted)
       ||!Event(team,L"/Script/ShooterGame.BombTeamComponent.OnRep_TeamChanged",0,nullptr)))return Fail(3222,"server_practice_team_notification_failed");
    return Read<uint8_t>(team,0xe8)==wanted?true:Fail(3222,"server_practice_team_readback_mismatch");
}
inline bool ArrayValid(const ObjectArray& a,int32_t maximum){
    return a.count>=0&&a.count<=maximum&&a.capacity>=a.count&&a.capacity<=4096
        &&(!a.count||original_players::Readable(a.data,static_cast<size_t>(a.count)*8));
}
inline bool Contains(const ObjectArray& a,void* object){
    if(!ArrayValid(a,4096))return false;for(int32_t n=0;n<a.count;++n)if(a.data[n]==object)return true;return false;
}
inline bool Append(void* object,size_t offset,void* value,int32_t maximum){
    auto a=Read<ObjectArray>(object,offset);
    if(!value||!ArrayValid(a,maximum))return false;if(Contains(a,value))return true;
    if(a.count>=maximum)return false;const int32_t previous=a.count,next=previous+1;
    if(!original_players::Write(object,offset+8,next))return false;
    if(next>a.capacity)engine::function<void(*)(void*,int32_t)>(0x18bd090)(static_cast<uint8_t*>(object)+offset,previous);
    a=Read<ObjectArray>(object,offset);
    return ArrayValid(a,maximum)&&a.count==next&&a.data
        &&original_players::Write(a.data,static_cast<size_t>(previous)*8,value)&&a.data[previous]==value;
}
inline bool InitializeTeamProfile(){
    if(state.teamProfileReady)return true;
    auto teamClass=engine::Class(0x65af170),gameClass=Read<void*>(state.gameState,0x10);
    auto teamInfo=engine::Find(L"/Script/ShooterGame.AresTeamInfo");
    auto assigned=Property(engine::Class(0x64f4490),L"AssignedTeamState",8,0x758);
    auto desired=Property(engine::Class(original_players::ShooterStateClass),L"DesiredClass",16,0xbf8);
    if(!teamClass||Read<int32_t>(teamClass,0x60)!=0x4d8
       ||!Property(teamClass,L"TeamPlayerStates",16,0x468)||!Property(teamClass,L"TeamComponent",8,0x478)
       ||!Property(teamClass,L"TeamInfo",16,0x4b0)||!Property(gameClass,L"TeamStates",16,0x720)
       ||!Property(gameClass,L"TeamStateClass",8,0x730)
       ||!assigned||!desired||(Read<uint64_t>(assigned,0x38)&0x20)==0||(Read<uint64_t>(desired,0x38)&0x20)==0
       ||!Property(teamInfo,L"Points",4,8)||!Property(gameClass,L"NumPointsToWinGame",4,0x7c8)
       ||!BindRecordMethod(records.getTeamState,L"/Script/ShooterGame.ShooterGameState.GetTeamStateForTeam",16,0x6ca6610)
       ||!BindRecordMethod(records.getPoints,L"/Script/ShooterGame.ShooterGameState.GetPointsForTeam",12,0x6ca3db0)
       ||!BindRecordMethod(records.onRepTeamComponent,L"/Script/ShooterGame.BaseTeamState.OnRep_TeamComponent",0,0x65af5d0)
       ||!BindRecordMethod(records.onRepTeamPlayers,L"/Script/ShooterGame.BaseTeamState.OnRep_TeamPlayerStates",16)
       ||!BindRecordMethod(records.onRepTeamStates,L"/Script/ShooterGame.ShooterGameState.OnRep_TeamStates",0,0x6ca51f0)
       ||!BindRecordMethod(records.onRepTeamInfo,L"/Script/ShooterGame.BaseTeamState.OnRep_TeamInfo",16,0x65af760)
       ||!BindRecordMethod(records.getAssignedTeamState,L"/Script/ShooterGame.AresPlayerStateBase.GetTeamState",8,0x64fad10)
       ||!BindRecordMethod(records.onRepAssignedTeamState,L"/Script/ShooterGame.AresPlayerStateBase.OnRep_AssignedTeamState",0,0x64facd0)
       ||!BindRecordMethod(records.getDesiredClass,L"/Script/ShooterGame.ShooterPlayerState.GetDesiredClass",8,0x6cc6a40)
       ||!BindRecordMethod(records.getDesiredData,L"/Script/ShooterGame.ShooterPlayerState.TryGetDesiredClassDataAsset",8,0x6cc6580)
       ||!BindRecordMethod(records.getPortrait,L"/Script/ShooterGame.ShooterPlayerState.GetDefaultClassCharacterIcon",8,0x6cc6250))
        return Fail(3299,"server_native_team_record_reflection_or_event_profile_mismatch");
    // Keep the existing FFA BaseTeamState factory. Practice uses the actual
    // original GS class setting (native ShooterGS defaults to BaseTeamState).
    records.teamStateClass=FreeForAll()?teamClass:Read<void*>(state.gameState,0x730);
    if(!original_players::Derived(records.teamStateClass,teamClass))return Fail(3313,"server_original_game_state_team_actor_class_invalid");
    if(FreeForAll()){
        const int32_t limit=static_cast<int32_t>(KillLimit);records.pointsLimitOffset=0x7c8;
        if(!original_players::Write(state.gameState,0x7c8,limit)||Read<int32_t>(state.gameState,0x7c8)!=limit)
            return Fail(3300,"server_ffa_native_points_to_win_configuration_failed");
    }
    state.teamProfileReady=true;return true;
}
inline bool SameNativeTeam(const AuthorityPlayer& p,void* component){
    if(FreeForAll())return component==p.team;
    return original_players::IsA(component,original_enemies::BombTeamClass)
        &&engine::function<int(*)(void*,void*)>(0x782bb10)(component,p.team)==0;
}
inline bool PublishAssignedTeamState(AuthorityPlayer& p){
    // Original AuthSetTeamState exec64fad50 calls v898, which is the
    // retained client RET leaf on both Ares/ShooterPlayerState. Supply only
    // its missing canonical pointer; preserve the original getter/notification.
    const auto table=Read<void*>(p.ps,0);
    if(Read<void*>(table,0x898)!=reinterpret_cast<void*>(engine::image+0x18a3a00)
       ||Read<void*>(table,0x8a0)!=reinterpret_cast<void*>(engine::image+0x717e390))
        return Fail(3314,"server_original_assigned_team_state_native_setter_or_getter_profile_mismatch");
    auto previous=Read<void*>(p.ps,0x758);
    if(previous&&previous!=p.teamActor)return Fail(3315,"server_native_player_assigned_team_state_changed_outside_authority_roster");
    if(previous!=p.teamActor){
        if(!original_players::Write(p.ps,0x758,p.teamActor)
           ||!Event(p.ps,L"/Script/ShooterGame.AresPlayerStateBase.OnRep_AssignedTeamState",0,nullptr))
            return Fail(3316,"server_original_assigned_team_state_notification_failed");
        ++state.assignedTeamStateWrites;
    }
    void* actual=nullptr;
    if(!Event(p.ps,L"/Script/ShooterGame.AresPlayerStateBase.GetTeamState",8,&actual)||actual!=p.teamActor)
        return Fail(3317,"server_original_assigned_team_state_getter_readback_failed");
    p.assignedTeamStateVerified=true;++state.assignedTeamStateReadbacks;return true;
}
inline bool ObserveNativeSelection(AuthorityPlayer& p,bool force=false){
    if(!force&&state.now<p.nextPresentationRead)return true;p.nextPresentationRead=state.now+.25;
    const auto type=Read<void*>(p.pawn,0x10);const auto content=original_content::FindCharacterByClass(type);
    void *actualClass=nullptr,*actualData=nullptr,*portrait=nullptr;
    if(!content.found||!content.dataAsset||content.characterClass!=type
       ||!Event(p.ps,L"/Script/ShooterGame.ShooterPlayerState.GetDesiredClass",8,&actualClass)
       ||!Event(p.ps,L"/Script/ShooterGame.ShooterPlayerState.TryGetDesiredClassDataAsset",8,&actualData)
       ||actualClass!=type||actualData!=content.dataAsset
       ||!Event(p.ps,L"/Script/ShooterGame.ShooterPlayerState.GetDefaultClassCharacterIcon",8,&portrait))
        return Fail(3318,"server_native_presentation_selection_content_or_getter_readback_failed");
    p.nativeSelectionVerified=true;p.nativePortrait=portrait;p.nativePortraitReady=portrait!=nullptr;
    // A texture can still be pending locally. Record the native result; do not
    // create an icon, substitute a brush, or turn it into a gameplay gate.
    return true;
}
inline bool EnsureNativeTeamRecords(){
    if(!InitializeTeamProfile())return false;
    if(!FreeForAll()&&Read<void*>(state.gameState,0x730)!=records.teamStateClass)return Fail(3319,"server_original_team_state_factory_class_changed");
    bool added=false;
    std::array<void*,2> practiceActors{};state.practiceTeams=0;
    for(auto& p:players)if(p.present){
        p.team=Read<void*>(p.ps,0x6a8);
        if(!original_players::IsA(p.team,FreeForAll()?deathmatch_rules::FFATeamClass:original_enemies::BombTeamClass)||Read<void*>(p.team,0xc8)!=p.ps)
            return Fail(3301,"server_native_team_record_requires_existing_player_owned_team_component");
        struct Query {void* team;void* result=nullptr;} q{p.team};
        if(!Event(state.gameState,L"/Script/ShooterGame.ShooterGameState.GetTeamStateForTeam",16,&q))return Fail(3302,"server_native_team_state_query_failed");
        p.teamActor=q.result;bool created=false,memberAdded=false;
        if(p.teamActor&&(!original_players::IsA(p.teamActor,0x65af170)||Read<uint8_t>(p.teamActor,0x224)!=3
           ||!SameNativeTeam(p,Read<void*>(p.teamActor,0x478))))
            return Fail(3323,"server_existing_native_team_actor_class_authority_or_alliance_mismatch");
        std::array<void*,MaxPlayers> previousItems{};ObjectArray previousPlayers{};
        if(!p.teamActor){
            engine::Transform transform;p.teamActor=original_players::Begin(records.teamStateClass,transform,state.gameState);
            if(!original_players::IsA(p.teamActor,0x65af170)||Read<uint8_t>(p.teamActor,0x224)!=3
               ||!original_players::Write(p.teamActor,0x478,p.team)||!Append(p.teamActor,0x468,p.ps,static_cast<int32_t>(MaxPlayers)))
                return Fail(3303,"server_ffa_original_team_actor_factory_or_pre_beginplay_links_failed");
            original_players::Finish(p.teamActor,transform);
            if(!Append(state.gameState,0x720,p.teamActor,128))return Fail(3304,"server_ffa_original_team_state_roster_registration_failed");
            ++state.teamActorsCreated;created=added=true;
        }else if(!Contains(Read<ObjectArray>(p.teamActor,0x468),p.ps)){
            const auto previous=Read<ObjectArray>(p.teamActor,0x468);
            if(!ArrayValid(previous,static_cast<int32_t>(MaxPlayers)))return Fail(3320,"server_native_existing_team_membership_array_invalid");
            for(int32_t n=0;n<previous.count;++n)previousItems[static_cast<size_t>(n)]=previous.data[n];
            previousPlayers={previousItems.data(),previous.count,previous.count};
            if(!Append(p.teamActor,0x468,p.ps,static_cast<int32_t>(MaxPlayers)))return Fail(3321,"server_native_team_membership_append_failed");
            memberAdded=true;
        }
        if(!original_players::IsA(p.teamActor,0x65af170)||!SameNativeTeam(p,Read<void*>(p.teamActor,0x478))
           ||!Contains(Read<ObjectArray>(p.teamActor,0x468),p.ps)||!Contains(Read<ObjectArray>(state.gameState,0x720),p.teamActor)
           ||!original_inventory_slots::CaptureIdentity(p.team,p.teamIdentity)||!original_inventory_slots::CaptureIdentity(p.teamActor,p.teamActorIdentity))
            return Fail(3305,"server_ffa_native_team_actor_or_player_state_identity_mismatch");
        if(created&&!Event(p.teamActor,L"/Script/ShooterGame.BaseTeamState.OnRep_TeamComponent",0,nullptr))
            return Fail(3306,"server_native_team_component_notification_failed");
        if(created||memberAdded){
            if(!Event(p.teamActor,L"/Script/ShooterGame.BaseTeamState.OnRep_TeamPlayerStates",16,&previousPlayers))
                return Fail(3306,"server_ffa_native_team_record_notification_failed");
            ++state.teamMembershipNotifications;
        }
        if(!PublishAssignedTeamState(p)||!ObserveNativeSelection(p,true))return false;
        if(!FreeForAll()){
            bool known=false;for(size_t n=0;n<state.practiceTeams;++n)known=known||practiceActors[n]==p.teamActor;
            if(!known){if(state.practiceTeams>=practiceActors.size())return Fail(3322,"server_practice_original_two_team_grouping_exceeded");practiceActors[state.practiceTeams++]=p.teamActor;}
        }
    }
    if(!FreeForAll()&&state.practiceTeams!=2)return Fail(3324,"server_practice_requires_two_original_native_team_groups");
    if(added&&!Event(state.gameState,L"/Script/ShooterGame.ShooterGameState.OnRep_TeamStates",0,nullptr))
        return Fail(3307,"server_ffa_native_game_state_team_roster_notification_failed");
    state.teamRecordsReady=true;return true;
}
inline bool SynchronizeNativeTeamPoints(){
    if(!state.teamRecordsReady)return Fail(3308,"server_native_team_points_require_authority_team_roster");
    for(auto& p:players)if(p.present){
        if(original_inventory_slots::ResolveIdentity(p.teamIdentity)!=p.team
           ||original_inventory_slots::ResolveIdentity(p.teamActorIdentity)!=p.teamActor
           ||Read<void*>(p.ps,0x6a8)!=p.team||!SameNativeTeam(p,Read<void*>(p.teamActor,0x478)))
            return Fail(3309,"server_native_team_points_identity_changed");
        // Original two-team HUD reads TeamInfo.Points. Practice has no round
        // award policy: preserve native Points/Wins/roles, rather than invent
        // a kill-based team score for this unlimited training session.
        const auto previous=Read<TeamInfo>(p.teamActor,0x4b0);
        // FFA ranking cards show whole-match kills. The original timed
        // streak remains independent for feedback and native window expiry.
        const int32_t points=FreeForAll()?static_cast<int32_t>(p.kills):previous.points;
        if(FreeForAll()&&previous.points!=points){
            if(!original_players::Write(p.teamActor,0x4b8,points))return Fail(3310,"server_ffa_native_team_points_record_write_failed");
            ++state.teamPointsWrites;auto old=previous;
            if(!Event(p.teamActor,L"/Script/ShooterGame.BaseTeamState.OnRep_TeamInfo",16,&old))return Fail(3311,"server_ffa_original_team_points_notification_failed");
            ++state.teamPointsNotifications;
        }
        struct Points {void* team;int32_t result=-1,padding=0;} value{p.team};
        struct Identity {void* team;void* result=nullptr;} identity{p.team};
        if(!Event(state.gameState,L"/Script/ShooterGame.ShooterGameState.GetPointsForTeam",12,&value)
           ||!Event(state.gameState,L"/Script/ShooterGame.ShooterGameState.GetTeamStateForTeam",16,&identity)
           ||identity.result!=p.teamActor||value.result!=points)
            return Fail(3312,"server_ffa_original_team_points_getter_readback_failed");
        void* assigned=nullptr;
        if(!Event(p.ps,L"/Script/ShooterGame.AresPlayerStateBase.GetTeamState",8,&assigned)||assigned!=p.teamActor)
            return Fail(3325,"server_native_assigned_team_state_changed_after_roster_publication");
        p.assignedTeamStateVerified=true;p.teamPointsVerified=true;
        if(!ObserveNativeSelection(p))return false;
    }return true;
}
inline bool CheckAlliances(){
    if(!players[0].present)return true;
    void* ffaClass=FreeForAll()?original_players::LoadStateClass(deathmatch_rules::FFABaseStatePath):nullptr;
    if(FreeForAll()&&!ffaClass)return Fail(3223,"server_ffa_original_player_state_base_missing");
    for(const auto& source:players)if(source.present){
        if(FreeForAll()&&!original_players::Derived(Read<void*>(source.ps,0x10),ffaClass))return Fail(3224,"server_ffa_admitted_player_state_is_not_ffa_class");
        for(const auto& target:players)if(target.present&&source.alive&&target.alive&&source.index!=target.index&&(FreeForAll()||source.human||target.human)){
            struct Alliance {void* source;void* target;uint8_t enemy=0,pad[7]{};} query{source.pawn,target.pawn};
            if(!Event(engine::DefaultObject(engine::Class(deathmatch_rules::BaseTeamClass)),L"/Script/ShooterGame.BaseTeamComponent.IsEnemy",17,&query)||!query.enemy)
                return Fail(3225,"server_original_alliance_query_does_not_confirm_enemy");
        }
    }state.originalFFAConfirmed=FreeForAll();return true;
}
inline void CacheRoster(size_t index,void* pc,void* ps,void* pawn,const Vec& feet,double yaw,uint32_t hero){
    auto& e=original_players::players[index];
    e.controller=pc;e.state=ps;e.character=pawn;e.inventory=Read<void*>(pawn,0xc08);e.mesh1P=Read<void*>(pawn,0xf30);
    e.abilities=Read<void*>(pawn,0xa20);e.attributes=Read<void*>(e.abilities,0x13a8);e.movement=Read<void*>(pawn,0x4f8);
    e.characterInitialized=Read<uint8_t>(pawn,0xa90)!=0;e.configured=e.attempted=e.registered=e.ready=e.enabled=true;
    e.local=false;e.feetPosition=feet;e.yaw=yaw;wcscpy_s(e.classPath,original_heroes::Paths[hero]);
    if(!e.requestedName[0]){if(!index)wcscpy_s(e.requestedName,original_heroes::Names[hero]);else swprintf_s(e.requestedName,L"Target %zu - %s",index,original_heroes::Names[hero]);}
    e.stateText=index?"network_authority_npc":"network_admitted_human";
}
inline bool BindPlayer(size_t index,void* pc,void* ps,void* pawn,uint32_t epoch,bool human,const Vec& feet,double yaw,uint32_t hero,void* previous=nullptr){
    auto& p=players[index];const bool added=!p.present;
    if(!epoch||hero>=original_heroes::PlayableHeroCount||!original_players::Registered(ps)
       ||human!=(index==0)||(human&&hero!=local_server_hero_selection::SelectionIndex()))
        return Fail(3226,"server_player_native_roster_or_spawn_epoch_missing");
    p.present=true;p.human=human;p.index=index;p.pc=pc;p.ps=ps;p.pawn=pawn;p.epoch=epoch;p.hero=hero;
    p.info=Read<void*>(ps,0xa18);p.stats=Read<void*>(ps,0x898);p.feet=p.spawnFeet=p.lastFeet=feet;p.yaw=yaw;
    if(!original_inventory_slots::CaptureIdentity(pc,p.pcIdentity)||!original_inventory_slots::CaptureIdentity(ps,p.psIdentity)
       ||!original_inventory_slots::CaptureIdentity(pawn,p.pawnIdentity)||!AuthorityPair(p))return Fail(3227,"server_player_authority_ownership_or_weak_identity_mismatch");
    if(!local_server_npc_identity::BindContext(state.world,ValidateNpcIdentityTuple)
       ||!local_server_npc_identity::Mark(index,pc,ps,pawn,epoch,human)){
        const auto& identity=local_server_npc_identity::GetState();
        state.restartRequired=state.restartRequired||identity.restartRequired;
        return Fail(identity.error?identity.error:3394,identity.stage);
    }
    CacheRoster(index,pc,ps,pawn,feet,yaw,hero);
    p.assignedTeamStateVerified=p.nativeSelectionVerified=p.nativePortraitReady=false;p.nativePortrait=nullptr;p.nextPresentationRead=0;
    if(!AssignPlayerIdentity(p))return false;
    if(!ReadLife(p)||!p.alive||p.health<=0||!original_inventory_slots::EnsureForPawn(pawn)||!original_damage::Bind(index,pawn)
       ||!original_scoreboard::RegisterPlayer(ps))return Fail(3228,"server_player_original_life_inventory_damage_or_score_bind_failed");
    p.info=Read<void*>(ps,0xa18);p.stats=Read<void*>(ps,0x898);
    if(!p.info||!p.stats||!PublishLinks(index,previous))return false;
    p.role=Read<uint8_t>(pawn,0x224);p.remoteRole=Read<uint8_t>(pawn,0x128);p.ready=true;p.phase=Phase::Alive;
    p.deathAccounted=false;p.lastSample=-1;p.deathAt=p.respawnAt=-1;p.inputPrepared=false;p.nativeLocallyControlled=false;p.externalAuthorityReady=false;
    p.movementObserved=false;p.firstMoveAt=-1;p.inputFrames=p.moveRequests=p.movingSamples=p.faceCalls=0;
    p.jumpRequests=p.crouchRequests=0;p.pendingInputCommands=0;p.nativeWindowRate=p.nativeWindowRemaining=-1;
    p.externalInputSubmissions=p.externalMoveRequests=p.externalJumpRequests=p.externalCrouchRequests=0;
    if(!human){auto& e=original_enemies::enemies[index];e={};e.attempted=e.ready=e.enabled=true;
        e.logicalId=p.logicalId?p.logicalId:static_cast<uint32_t>(index);e.heroIndex=hero;e.anchor=feet;e.yaw=yaw;
        e.pawn=pawn;e.controller=pc;e.playerState=ps;e.team=Read<void*>(ps,0x6a8);e.spawnPublished=e.possessionPublished=e.selectionPublished=true;
        if(!original_enemies::ApplyRosterName(index,original_players::players[index].requestedName))return Fail(original_enemies::error,original_enemies::stage);
    }
    if(state.started&&(!PublishTeam(index)||!BindAI(index)||!BindAuthorityStreak(index)))return false;
    p.equipmentRegistered=hooks.registerEquipment&&hooks.registerEquipment(index,pc,ps,pawn,epoch,human);
    p.equipmentReady=p.equipmentRegistered&&!hooks.equipmentReady;
    if(hooks.registerEquipment&&!p.equipmentRegistered)return Fail(3229,"server_player_authority_equipment_registration_failed");
    if(added)++state.participants;
    p.stage="server_authority_character_bound_runtime_pending";
    if(hooks.actorSpawned)hooks.actorSpawned(index,pc,ps,pawn,epoch,human);return true;
}
inline bool InitializeImpl(void* world,const Hooks& callbacks){
    if(state.initialized)return state.world==world&&Thread();
    if(!world||world!=original_players::world||!original_players::Thread())return Fail(3230,"server_match_original_world_or_thread_missing");
    state.world=world;state.gameState=Read<void*>(world,0x178);
    if(!original_players::IsA(state.gameState,original_players::ShooterGameStateClass))return Fail(3231,"server_match_original_game_state_missing");
    state.netMode=engine::function<int(*)(void*)>(0x3efc820)(state.gameState);
    if(state.netMode!=1&&state.netMode!=2)return Fail(3232,"server_match_requires_real_network_authority_world");
    if(!original_damage::Initialize(world)||!original_scoreboard::Initialize(world)||!training_world_geometry::Initialize(world))
        return Fail(3233,"server_match_original_health_score_or_geometry_context_failed");
    if(!InitializeIdentityProfile())return false;
    hooks=callbacks;gameThread=GetCurrentThreadId();state.now=Now();state.initialized=true;state.stage="server_match_waiting_configuration_and_native_human";return true;
}
inline bool RegisterHumanImpl(void* pc,void* ps,void* pawn,uint32_t epoch){
    if(!Thread()||!pc||!ps||!pawn)return Fail(3234,"server_match_admitted_human_context_missing");
    if(players[0].present){if(players[0].pc==pc&&players[0].ps==ps&&players[0].pawn==pawn&&players[0].epoch==epoch)return true;
        return Fail(3235,"server_match_human_replacement_requires_respawn_lifecycle");}
    auto type=Read<void*>(pawn,0x10);const auto id=original_content::FindCharacterByClass(type);
    if(!local_server_hero_selection::Ready()||!id.found
       ||!local_server_hero_selection::SameGuid(id.id,local_server_hero_selection::SelectionGuid()))
        return Fail(3236,"server_match_admitted_human_must_match_selected_original_hero");
    Vec feet{},rotation{};
    if(!Event(pawn,L"/Script/Engine.Actor.K2_GetActorLocation",24,&feet)||!Event(pc,L"/Script/Engine.Controller.GetControlRotation",24,&rotation))
        return Fail(3237,"server_match_admitted_human_transform_query_failed");
    feet.z-=Read<float>(Read<void*>(pawn,0x500),0x6d8);
    if(!BindPlayer(0,pc,ps,pawn,epoch,true,feet,rotation.y,local_server_hero_selection::SelectionIndex()))return false;
    state.humanRegistered=true;state.stage="server_match_admitted_selected_hero_registered";return true;
}
inline bool AllocateHeroes(){
    if(!local_server_hero_selection::Ready())return Fail(3330,"server_match_selected_hero_sidecar_not_loaded");
    const auto humanHero=local_server_hero_selection::SelectionIndex();
    assignments.fill(0);assignments[0]=humanHero;
    if(local_server_bot_roster_initial::GetState().error)
        return Fail(local_server_bot_roster_initial::GetState().error,local_server_bot_roster_initial::GetState().stage);
    if(local_server_bot_roster_initial::Fixed()){
        if(!local_server_bot_roster_initial::Matches(&initial,sizeof(initial),state.wantedPlayers,initial.config.revision,initial.config.seed))
            return Fail(9036,"fixed_bot_roster_initial_or_human_selection_mismatch");
        if(!original_heroes::Initialize())return Fail(original_heroes::State().error,original_heroes::State().stage);
        const auto context=local_server_bot_roster_initial::Context(&initial,sizeof(initial),state.wantedPlayers,initial.config.revision,initial.config.seed);
        if(!local_server_bot_roster_wire::Assign(local_server_bot_roster_initial::GetState().plan,context,humanHero,
            local_server_bot_roster_initial::CatalogGuid(local_server_hero_selection::SelectionGuid()),assignments))
            return Fail(9036,"fixed_bot_roster_assignment_context_changed");
        state.shuffleSeed=initial.config.seed;state.uniqueHeroes=true;local_server_bot_roster_initial::NativeCatalogVerified();return true;
    }
    if(!FreeForAll()){
        for(size_t n=1;n<state.wantedPlayers;++n)assignments[n]=training_bot_controller::Mix(initial.config.seed^static_cast<uint32_t>(n))%static_cast<uint32_t>(original_enemies::HeroCount);
        return true;
    }
    if(!original_heroes::Initialize())return Fail(original_heroes::State().error,original_heroes::State().stage);
    std::array<uint32_t,original_heroes::PlayableHeroCount-1> pool{};
    size_t pooled=0;
    for(uint32_t hero=0;hero<original_heroes::PlayableHeroCount;++hero)if(hero!=humanHero)pool[pooled++]=hero;
    if(pooled!=pool.size())return Fail(3331,"server_match_selected_hero_exclusion_pool_invalid");
    LARGE_INTEGER nonce{};QueryPerformanceCounter(&nonce);
    uint32_t rng=initial.config.seed^nonce.LowPart^static_cast<uint32_t>(nonce.HighPart)^GetCurrentProcessId()^static_cast<uint32_t>(GetTickCount64());
    state.shuffleSeed=rng;
    for(size_t n=pool.size();n>1;--n){const size_t at=original_heroes::BoundedRandom(rng,static_cast<uint32_t>(n));std::swap(pool[n-1],pool[at]);}
    for(size_t n=1;n<state.wantedPlayers;++n)assignments[n]=pool[n-1];
    state.uniqueHeroes=true;return true;
}
inline bool StartImpl(const training_control::Initial& selected){
    if(!Thread()||!ValidInitial(selected))return Fail(3238,"server_match_invalid_initial_configuration");
    if(state.started)return Fail(3239,"server_match_population_or_map_restart_requires_new_native_session");
    initial=selected;initial.bytes=sizeof(initial);state.wantedPlayers=initial.config.totalPlayers;
    if(!AllocateHeroes())return false;
    training_bot_controller::Hooks callbacks;callbacks.observe=Observe;callbacks.visible=Visible;callbacks.validateSegment=training_world_geometry::ValidateSegment;
    callbacks.targetAllowed=TargetAllowed;
    if(!training_bot_controller::Initialize(state.world,callbacks)||!training_bot_controller::Configure(BotConfig())||!ApplyRoutes())
        return Fail(training_bot_controller::State().error,training_bot_controller::State().stage);
    if(!training_maps::Initialize(state.world,initial.mapId))return Fail(training_maps::State().error,training_maps::State().stage);
    state.started=true;state.active=false;state.remaining=DurationSeconds;
    if(players[0].present&&(!PublishTeam(0)||!BindAI(0)||!BindAuthorityStreak(0)))return false;
    state.stage="server_match_waiting_original_map_layers_and_admitted_human";return true;
}

inline bool ConfigureNPCUI(void* pc){
    // Only an instance-level documented controller option is configured, before
    // PostInitializeComponents. No client widget or UI service is created.
    auto type=Read<void*>(pc,0x10);const int offset=original_scope::FieldOffset(type,L"bCreateUIInstancesFromGlobals");
    if(offset!=0xc28||Read<void*>(pc,0xc18)||Read<void*>(pc,0xc20))return Fail(3240,"server_npc_ui_factory_configuration_profile_mismatch");
    const uint8_t disabled=0;
    return original_players::Write(pc,0xc28,disabled)&&Read<uint8_t>(pc,0xc28)==0;
}
inline void* CreateNPCController(const engine::Transform& transform,void* pcClass,void* psClass){
    auto mode=Read<void*>(state.world,0x170);void* previous=Read<void*>(mode,0x490);void* pc=nullptr;
    if(!mode||original_scope::FieldOffset(Read<void*>(mode,0x10),L"PlayerStateClass")!=0x490){Fail(3241,"server_npc_game_mode_player_state_class_profile_mismatch");return nullptr;}
    __try{
        if(original_players::Write(mode,0x490,psClass)){
            pc=original_players::Begin(pcClass,transform,nullptr);
            if(pc&&!ConfigureNPCUI(pc))pc=nullptr;
            if(pc)original_players::Finish(pc,transform);
        }
    }__finally{original_players::Write(mode,0x490,previous);}
    return pc;
}
inline bool SpawnNPC(size_t index){
    if(index==0||index>=state.wantedPlayers||players[index].present)return Fail(3242,"server_npc_spawn_requires_unused_nonhuman_slot");
    deathmatch_rules::SpawnSelection spawn;
    if(!SelectSpawn(index,1,spawn))return false;
    const auto route=players[index].spawnRoute;const uint32_t hero=assignments[index];
    original_players::CharacterDefaults d;
    if(!original_players::ProbeDefaults(original_heroes::Paths[hero],d)||d.abstractClass||!d.inventory||!d.inputState)
        return Fail(3243,"server_npc_original_hero_defaults_missing");
    void* psClass=original_players::LoadStateClass(FreeForAll()?deathmatch_rules::StateClassPath:original_players::options.playerStateClassPath);
    void* pcClass=engine::Class(original_players::ShooterControllerClass);
    if(FreeForAll())pcClass=engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t)>(0x1d0c6e0)
        (pcClass,nullptr,deathmatch_rules::ControllerClassPath,nullptr,0);
    if(!psClass||!pcClass)return Fail(3244,"server_npc_original_controller_or_state_class_missing");
    engine::Transform t;t.translation=spawn.position;t.translation.z+=d.capsuleHalfHeight+2;
    constexpr double rad=3.14159265358979323846/180;t.rotation[2]=std::sin(spawn.yaw*rad*.5);t.rotation[3]=std::cos(spawn.yaw*rad*.5);
    state.operation="server_npc_original_controller_and_player_state_factory";
    auto pc=CreateNPCController(t,pcClass,psClass);auto ps=Read<void*>(pc,0x470);
    if(!original_players::IsA(pc,original_players::ShooterControllerClass)||Read<void*>(pc,0x510)||Read<uint8_t>(pc,0x224)!=3
       ||!original_players::IsA(ps,original_players::ShooterStateClass)||Read<void*>(ps,0x210)!=pc
       ||!original_players::RegisterState(ps)||Read<void*>(pc,0xc18)||Read<void*>(pc,0xc20))
        return Fail(3245,"server_npc_native_controller_state_or_player_ownership_failed");
    state.operation="server_npc_original_hero_factory";auto pawn=original_players::Begin(d.type,t,pc);
    if(!original_players::IsA(pawn,original_players::ShooterCharacterClass)||!original_players::Write(pawn,0x488,ps))
        return Fail(3246,"server_npc_original_hero_spawn_or_pre_beginplay_state_link_failed");
    original_players::Finish(pawn,t);
    if(!Read<uint8_t>(pawn,0xa90)||!Read<void*>(pawn,0xc08)||!Read<void*>(pawn,0xba0)
       ||!Event(pc,L"/Script/Engine.Controller.Possess",8,&pawn)||Read<void*>(pc,0x4b0)!=pawn||Read<void*>(pawn,0x4a0)!=pc)
        return Fail(3247,"server_npc_original_hero_beginplay_or_possession_failed");
    players[index].logicalId=static_cast<uint32_t>(index);players[index].spawnRoute=route;
    if(!BindPlayer(index,pc,ps,pawn,1,false,spawn.position,spawn.yaw,hero))return false;
    ++state.spawnedNPCs;state.operation="none";return true;
}
inline bool NativeSubmitBot(size_t index,const Frame& frame,double delta){
    auto& p=players[index];auto& a=training_bot_controller::actors[index];
    if(p.human||!AuthorityPair(p)||!p.alive||!p.nativeLocallyControlled)return Fail(3248,"server_npc_submit_requires_original_local_authority_tick_eligibility");
    if(!original_enemies::SubmitWorldInput(index,frame.worldDirection,frame.rotation.y,frame.walk,frame.crouch)
       ||!Event(p.pc,L"/Script/Engine.Controller.SetControlRotation",24,const_cast<Vec*>(&frame.rotation)))return Fail(3249,"server_npc_native_world_input_submission_failed");
    auto table=Read<void**>(p.pawn,0);auto face=Read<void*>(table,original_npc_facing::FaceSlot);
    if(face!=reinterpret_cast<void*>(engine::image+original_npc_facing::NativeFace))return Fail(3250,"server_npc_native_face_rotation_entry_changed");
    const auto humanPC=players[0].pc,humanPawn=players[0].pawn,humanPlayer=Read<void*>(humanPC,0x510);
    reinterpret_cast<original_npc_facing::NativeFaceRotation>(face)(p.pawn,&frame.rotation,static_cast<float>(delta));++p.faceCalls;
    if(Read<void*>(p.pc,0x510)||Read<uint8_t>(p.pawn,0x224)!=3||players[0].pc!=humanPC||players[0].pawn!=humanPawn
       ||Read<void*>(humanPC,0x510)!=humanPlayer)return Fail(3251,"server_npc_native_input_changed_network_ownership");
    // Direct original jump input API. Its normal native authority movement tick
    // owns Jump/cooldowns/gravity. The standalone-only queue pump is not used.
    if(frame.jump!=a.requestedJump){uint8_t jump=frame.jump?1:0;
        if(!Event(p.pawn,L"/Script/ShooterGame.ShooterCharacter.SetJumpHeld",1,&jump))return Fail(3252,"server_npc_original_jump_input_failed");
        a.requestedJump=frame.jump;++a.jumpSetterCalls;
    }
    if(hooks.weaponInput&&!hooks.weaponInput(index,frame.fire,frame.reload))return Fail(3253,"server_npc_authority_weapon_input_failed");
    if(!hooks.weaponInput&&(frame.fire||frame.reload))++training_bot_controller::diagnostics.weaponUnavailable;
    auto pawnInput=Read<void*>(p.pawn,0x230);p.pendingInputCommands=Read<int32_t>(pawnInput,0x8b8);
    ++p.inputFrames;++a.inputFrames;++state.botInputFrames;
    if(training_bot_controller::Length2D(frame.worldDirection)>.1){++p.moveRequests;if(p.firstMoveAt<0)p.firstMoveAt=state.now;}
    if(frame.jump)++p.jumpRequests;if(frame.crouch)++p.crouchRequests;return true;
}
inline bool TickBots(){
    using namespace training_bot_controller;
    if(!training_bot_controller::Configure(BotConfig()))return Fail(diagnostics.error,diagnostics.stage);
    if(routeValidationProgress.active&&!PollRouteValidations(state.now))return Fail(diagnostics.error,diagnostics.stage);
    if(state.now>=nextObservation){nextObservation=state.now+.05;
        for(size_t n=0;n<MaxPlayers;++n){auto& a=actors[n];if(!a.bound)continue;Observation o;
            if(!Observe(n,o)){++diagnostics.observationFailures;return Fail(3254,"server_npc_native_observation_failed");}
            if(o.pawn!=a.pawn||(o.spawnEpoch&&o.spawnEpoch!=a.epoch))return Fail(3255,"server_npc_observation_requires_explicit_spawn_epoch_rebind");
            a.observed=o;
        }
    }
    PracticeClock(state.now);
    if(config.active&&config.mode==Mode::HoldAngle)ScheduleWave(state.now);
    if(hooks.sensorPass&&!hooks.sensorPass(state.now))return Fail(3325,"server_npc_sensor_pass_failed");
    for(size_t n=1;n<state.wantedPlayers;++n){auto& p=players[n];auto& a=actors[n];if(!p.present||!a.bound||!p.alive)continue;
        if(!p.nativeLocallyControlled&&!p.externalAuthorityReady){p.stage=hooks.npcAuthorityReady
            ?"server_npc_verified_external_authority_driver_pending":"server_npc_original_authority_tick_gate_unconfirmed";continue;}
        const double delta=a.lastTick<0?.016:Clamp(state.now-a.lastTick,0,.1);a.lastTick=state.now;
        const auto rotation=a.output.rotation;a.output={};a.output.rotation=(a.inputFrames||p.externalInputSubmissions)?rotation:a.observed.rotation;
        if(!a.observed.ready||!a.observed.alive){a.reason="awaiting_original_alive_ready_epoch";continue;}
        bool customDecision=false;
        if(hooks.decideBot&&!hooks.decideBot(n,state.now,a.output,customDecision))
            return Fail(3326,"server_npc_custom_decision_failed");
        if(!customDecision&&config.active){
            if(config.mode==Mode::HoldAngle)HoldStrategy(n,state.now);
            Combat(n,state.now,delta);if(config.mode==Mode::Deathmatch)DeathmatchStrategy(n,state.now);
            if(a.target<0&&Length2D(a.output.worldDirection)>.1){const double desired=std::atan2(a.output.worldDirection.y,a.output.worldDirection.x)*180/3.14159265358979323846;
                a.output.rotation.y+=Clamp(Delta(desired,a.output.rotation.y),-a.profile.turnSpeed*delta,a.profile.turnSpeed*delta);}
        }
        if(a.output.crouch){++a.crouchRequestedSamples;if(a.observed.crouched){++a.crouchObservedSamples;a.crouchSince=-1;}
            else if(a.crouchSince<0)a.crouchSince=state.now;
            else if(!customDecision&&state.now-a.crouchSince>.5){Change(a,training_bot_controller::Phase::Blocked,state.now);a.waitUntil=state.now+2;a.reason="server_original_crouch_input_not_consumed";a.output={};a.output.rotation=rotation;}
        }else a.crouchSince=-1;
        if(a.output.jump){++a.jumpRequestedSamples;if(a.observed.jumping||a.observed.movementType==2||a.observed.velocity.z>10)++a.jumpObservedSamples;}
        const bool submitted=hooks.submitBot?hooks.submitBot(n,a.output,delta):NativeSubmitBot(n,a.output,delta);
        if(!submitted)return state.error?false:Fail(3285,"server_npc_authority_submit_callback_failed");
        if(hooks.botSubmitted&&!hooks.botSubmitted(n,a.output))return Fail(3327,"server_npc_submitted_intent_consumer_failed");
        if(hooks.submitBot){
            // The external callback caches intent. Count that successful
            // submission once, separately from the default native transport
            // and all actual fixed-step/physics observations in its owner.
            ++p.externalInputSubmissions;++state.externalBotInputSubmissions;
            if(Length2D(a.output.worldDirection)>.1){++p.externalMoveRequests;if(p.firstMoveAt<0)p.firstMoveAt=state.now;}
            if(a.output.jump)++p.externalJumpRequests;if(a.output.crouch)++p.externalCrouchRequests;
        }
        p.stage=a.reason;
    }
    ++diagnostics.ticks;return true;
}
inline void RefreshLeader(){
    state.leader=-1;state.leadingKills=state.leadingTies=0;
    for(const auto& p:players)if(p.present){if(state.leader<0||p.kills>state.leadingKills){state.leader=static_cast<int>(p.index);state.leadingKills=p.kills;state.leadingTies=1;}
        else if(p.kills==state.leadingKills)++state.leadingTies;}
}
inline void EndMatch(EndReason reason){
    if(state.ended)return;RefreshLeader();state.active=false;state.ended=true;state.endReason=reason;
    state.winner=state.leadingTies==1?state.leader:-1;state.stage=reason==EndReason::KillLimit?"server_match_native_score_reached_40":"server_match_540_seconds_elapsed";
    if(hooks.matchEnded)hooks.matchEnded(state.winner,reason);
}
inline bool CanDamage(size_t source,size_t victim){
    if(!Active()||source>=players.size()||victim>=players.size()||source==victim)return false;
    if(IsObserver(source)||IsObserver(victim))return false;
    const auto& s=players[source];const auto& v=players[victim];
    return s.present&&v.present&&s.ready&&v.ready&&s.alive&&v.alive&&s.phase==Phase::Alive&&v.phase==Phase::Alive
        &&!(v.human&&(initial.config.flags&training_control::Invincible))
        &&(s.human||FreeForAll()||(initial.config.flags&training_control::ReturnFire))
        &&(FreeForAll()||s.human||v.human)&&(!hooks.canDamage||hooks.canDamage(source,victim));
}
inline bool MarkDead(size_t index,double now,bool scoreCommitted){
    auto& p=players[index];if(!ReadLife(p)||p.alive||p.health>.001f)return Fail(3256,"server_death_requires_native_health_and_alive_readbacks");
    if(p.phase==Phase::DeadWaiting)return true;
    p.ready=false;p.phase=Phase::DeadWaiting;p.deathAt=now;p.respawnAt=now+initial.config.respawnSeconds;
    if(!p.deathAccounted){++p.deaths;p.deathAccounted=true;
        if(!scoreCommitted){auto score=original_scoreboard::Find(p.ps);
            if(!score||!original_scoreboard::ReadNativeScore(*score)||score->deaths==INT32_MAX)return Fail(3257,"server_external_death_original_score_unavailable");
            original_scoreboard::Put<int32_t>(score->stats,0xe8,score->deaths+1);
            if(!original_scoreboard::ReadNativeScore(*score))return Fail(3258,"server_external_death_original_score_readback_failed");++state.externalDeaths;
        }
    }
    float remaining=0,minimum=initial.config.respawnSeconds+2;
    if(!Event(p.pawn,L"/Script/Engine.Actor.GetLifeSpan",4,&remaining)||!std::isfinite(remaining))return Fail(3259,"server_corpse_original_lifespan_query_failed");
    if(remaining>0&&remaining<minimum&&(!Event(p.pawn,L"/Script/Engine.Actor.SetLifeSpan",4,&minimum)
       ||!Event(p.pawn,L"/Script/Engine.Actor.GetLifeSpan",4,&remaining)||remaining<minimum-.1f))return Fail(3260,"server_corpse_native_timer_does_not_cover_respawn");
    p.streak=0;p.lastKill=-1;
    if(FreeForAll()){
        if(!original_timed_streak::Reset(index))return Fail(original_timed_streak::State().error,original_timed_streak::State().stage);
    }else{void* component=nullptr;
        if(!Event(p.info,L"/Script/ShooterGame.OwnerExclusivePlayerInfo.GetKillStreakComponentMutable",8,&component)
           ||!Event(component,L"/Script/ShooterGame.KillStreakComponent.ResetKills",0,nullptr))return Fail(3261,"server_death_original_feedback_reset_failed");
    }
    p.stage="server_native_dead_waiting_respawn";
    if(hooks.afterDeath&&!hooks.afterDeath(index,p.pawn,p.ps,p.epoch))
        return Fail(3320,"server_death_pickup_authority_callback_failed");
    return true;
}
inline bool BeginNativeDamage(uint64_t token,size_t source,size_t victim,void* equippable,double now,original_damage::DeathData& death){
    if(!Thread()||!token||pending.active||!std::isfinite(now))return Fail(3262,"server_damage_transaction_invalid");
    if(!CanDamage(source,victim)){++state.policyRejects;return false;}
    if(!RefreshStreak(source)||(FreeForAll()&&!RefreshStreak(victim)))return false;
    const auto killer=original_scoreboard::Find(players[source].ps),killed=original_scoreboard::Find(players[victim].ps);
    if(!killer||!killed||!original_scoreboard::ReadNativeScore(*killer)||!original_scoreboard::ReadNativeScore(*killed)
       ||killer->round_kills==INT32_MAX)return Fail(3263,"server_damage_original_score_ordinals_missing");
    death.killsForKiller=killer->round_kills+1;death.killsForVictim=killed->round_kills;
    pending={true,token,source,victim,equippable,now,players[source].streak+1,death.killsForKiller,death.killsForVictim,static_cast<int32_t>(players[victim].streak)};return true;
}
inline void CancelNativeDamage(){pending={};}
inline bool EndNativeDamage(bool succeeded,const original_damage::Result& result){
    if(!Thread()||!pending.active)return Fail(3264,"server_damage_no_pending_native_transaction");
    const auto event=pending;pending={};
    if(!succeeded)return Fail(3265,"server_original_damage_application_failed");
    bool duplicate=result.duplicate;for(auto token:committedTokens)duplicate=duplicate||token==event.token;
    if(duplicate){++state.duplicateTokens;return true;}
    if(!result.applied)return true;
    if(!result.originalResponse||!result.originalLifeReadback||!result.response)return Fail(3266,"server_damage_requires_original_response_and_life_readback");
    committedTokens[tokenCursor++%committedTokens.size()]=event.token;
    auto& source=players[event.source];auto& victim=players[event.victim];
    if(!ReadLife(victim))return false;if(!result.killed)return true;
    if(victim.alive||victim.health>.001f)return Fail(3267,"server_kill_native_victim_still_alive");
    if(!original_scoreboard::RecordKill(event.token,source.ps,victim.ps,result.response,event.equippable,event.killerOrdinal,event.victimOrdinal))
        return Fail(original_scoreboard::State().error,original_scoreboard::State().stage);
    ++source.kills;++state.kills;
    if(FreeForAll()){
        if(!original_timed_streak::AfterKill(event.source,victim.pawn,result.response,static_cast<int32_t>(event.nextStreak))||!RefreshStreak(event.source))
            return Fail(original_timed_streak::State().error,original_timed_streak::State().stage);
    }else source.streak=event.nextStreak;
    source.bestStreak=(std::max)(source.bestStreak,source.streak);source.lastKill=state.now;
    void* component=nullptr;int32_t count=static_cast<int32_t>(source.streak),observed=-1;
    if(!Event(source.info,L"/Script/ShooterGame.OwnerExclusivePlayerInfo.GetKillStreakComponentMutable",8,&component)
       ||!Event(component,L"/Script/ShooterGame.KillStreakComponent.SetKills",4,&count)
       ||!Event(component,L"/Script/ShooterGame.KillStreakComponent.GetNumKillsStreak",4,&observed)||observed!=count)
        return Fail(3268,"server_kill_original_streak_counter_readback_failed");
    if(!MarkDead(event.victim,state.now,true)||!SynchronizeNativeTeamPoints())return false;
    // Publish presentation only after the original lethal result, native
    // scoreboard, real timed-window callback and life/team-point commit succeed.
    // DeathData and original KillEvent retain their cumulative ordinals.
    if(FreeForAll()&&local_server_feedback_ledger::Enabled()){
        local_server_feedback_wire::Record record{};record.damageToken=event.token;
        record.killerId=source.playerId;record.victimId=victim.playerId;
        record.killerEpoch=source.epoch;record.victimEpoch=victim.epoch;
        record.killerOrdinal=event.killerOrdinal;record.victimOrdinal=event.victimOrdinal;
        record.killerWindow=count;record.victimWindow=event.victimFeedbackCount;
        record.nativeWindowRate=source.nativeWindowRate;record.nativeWindowRemaining=source.nativeWindowRemaining;
        const bool lifeNumbers=Event(source.pawn,L"/Script/ShooterGame.ShooterCharacter.GetRespawnNumber",4,&record.killerRespawn)
            &&Event(victim.pawn,L"/Script/ShooterGame.ShooterCharacter.GetRespawnNumber",4,&record.victimRespawn);
        for(const auto& native:original_damage::targets)
            if(native.bound&&native.pawn==victim.pawn&&native.lastToken==event.token&&native.lastResponse==result.response)
                record.nativeDamageEvent=native.lastNativeEventIndex;
        if(lifeNumbers)local_server_feedback_ledger::Commit(record);
        else local_server_feedback_ledger::RejectCommittedRecord("feedback_ledger_native_life_query_unavailable");
    }
    if(hooks.afterScoreCommit)hooks.afterScoreCommit(event.token,event.source,event.victim);
    RefreshLeader();if(FreeForAll()&&source.kills>=KillLimit)local_server_match::EndMatch(EndReason::KillLimit);return true;
}
inline void* RespawnNPCPawn(size_t index,const deathmatch_rules::SpawnSelection& spawn,uint32_t hero){
    auto& p=players[index];original_players::CharacterDefaults d;
    if(!original_players::ProbeDefaults(original_heroes::Paths[hero],d))return nullptr;
    engine::Transform t;t.translation=spawn.position;t.translation.z+=d.capsuleHalfHeight+2;
    constexpr double rad=3.14159265358979323846/180;t.rotation[2]=std::sin(spawn.yaw*rad*.5);t.rotation[3]=std::cos(spawn.yaw*rad*.5);
    auto fresh=original_players::Begin(d.type,t,p.pc);
    if(!original_players::IsA(fresh,original_players::ShooterCharacterClass)||fresh==p.pawn||!original_players::Write(fresh,0x488,p.ps))return nullptr;
    original_players::Finish(fresh,t);
    return Event(p.pc,L"/Script/Engine.Controller.Possess",8,&fresh)&&Read<void*>(p.pc,0x4b0)==fresh?fresh:nullptr;
}
inline bool Respawn(size_t index){
    auto& p=players[index];
    // A real human can die while initial asynchronous gear is still loading.
    // Rebuild that confirmed dead life once the configured original map is
    // ready, without manufacturing an active match or starting its clock.
    const bool initialHumanLife=p.human&&state.started&&state.mapsReady&&state.startedAt<0;
    if((!state.active&&!initialHumanLife)||state.ended||p.phase!=Phase::DeadWaiting||state.now<p.respawnAt)return true;
    if(!ReadLife(p)||p.alive||p.health>.001f||pending.active)return Fail(3269,"server_respawn_requires_confirmed_native_death_outside_damage_dispatch");
    if(p.epoch==UINT32_MAX)return Fail(3270,"server_respawn_epoch_exhausted");
    uint32_t nextHero=p.hero;
    if(!p.human&&local_server_bot_roster_initial::Fixed()){
        const auto*entry=local_server_bot_roster_initial::Entry(index);
        if(!entry)return Fail(9037,"fixed_bot_roster_respawn_slot_missing");nextHero=entry->heroIndex;
    }else if(!p.human&&!FreeForAll())nextHero=
        training_bot_controller::Mix(initial.config.seed^static_cast<uint32_t>(index*0x9e3779b9ULL)^(p.epoch+1)*0x85ebca6bU)%static_cast<uint32_t>(original_enemies::HeroCount);
    assignments[index]=nextHero;
    deathmatch_rules::SpawnSelection spawn;if(!SelectSpawn(index,p.epoch+1,spawn))return false;
    if(FreeForAll()&&!spawn.originalMapAnchor)return Fail(3271,"server_ffa_respawn_requires_original_map_anchor");
    if(p.human&&!hooks.spawnHuman)return Fail(3272,"server_human_respawn_requires_admitted_connection_factory_callback");
    void* old=p.pawn;void* oldInfo=p.info;void* oldStats=p.stats;auto pc=p.pc,ps=p.ps;const auto pcPlayer=Read<void*>(pc,0x510);
    auto nativeScore=original_scoreboard::Find(ps);
    if(!nativeScore||!original_scoreboard::ReadNativeScore(*nativeScore))return Fail(3273,"server_respawn_original_aggregate_score_missing");
    const auto killsBefore=nativeScore->kills,deathsBefore=nativeScore->deaths;
    if(hooks.beforeRetire&&!hooks.beforeRetire(index,old,p.epoch))return Fail(3274,"server_respawn_equipment_retirement_failed");
    if(FreeForAll()&&!original_timed_streak::BeforeRetire(index))return Fail(original_timed_streak::State().error,original_timed_streak::State().stage);
    training_bot_controller::actors[index]={};p.nativeStreakBound=false;
    if(!original_inventory_slots::RetirePawn(old))return Fail(3275,"server_respawn_original_inventory_cache_retirement_failed");
    p.phase=Phase::Rebuilding;state.operation=p.human?"server_respawn_same_admitted_human_controller":"server_respawn_same_npc_controller";
    auto fresh=p.human?hooks.spawnHuman(pc,ps,spawn.position,spawn.yaw):RespawnNPCPawn(index,spawn,nextHero);
    if(!fresh||fresh==old)return Fail(3276,"server_respawn_fresh_native_character_factory_failed");
    const auto epoch=p.epoch+1;
    if(!original_damage::Sample(index))return Fail(original_damage::State().error,original_damage::State().stage);
    if(!BindPlayer(index,pc,ps,fresh,epoch,p.human,spawn.position,spawn.yaw,nextHero,old))return false;
    if(p.info!=oldInfo||p.stats!=oldStats||Read<void*>(pc,0x510)!=pcPlayer
       ||!original_scoreboard::ReadNativeScore(*nativeScore)||nativeScore->kills!=killsBefore||nativeScore->deaths!=deathsBefore)
        return Fail(3277,"server_respawn_changed_connection_identity_or_aggregate_score");
    original_enemy_respawn::Result corpse;corpse.oldPawn=old;
    if(!original_enemy_respawn::ScheduleCorpseCleanup(corpse))return Fail(original_enemy_respawn::State().error,original_enemy_respawn::State().stage);
    ++state.respawns;if(p.human)++state.humanRespawns;
    state.operation="none";return CheckAlliances();
}
// Run before the root's strict owned-human input poll. This is a lifecycle
// observation boundary only: no AI, NPC loop, match-clock update, frame counter,
// delta accumulation or native physics is advanced. Runtime retires the old
// human's combat/input/ability/equipment bindings after observing DeadWaiting,
// then calls RespawnHuman when that retirement has completed.
inline bool PreprocessDeathsImpl(){
    if(!Thread())return Fail(3314,"server_human_death_preprocess_requires_authority_game_thread");
    if(state.error)return false;
    if(!state.started||!state.humanRegistered||!players[0].present||pending.active)return true;
    auto& p=players[0];state.now=Now();
    if(original_inventory_slots::ResolveIdentity(p.pawnIdentity)!=p.pawn)
        return Fail(3315,"server_human_death_preprocess_original_pawn_identity_expired");
    if(!original_damage::Sample(0))return Fail(original_damage::State().error,original_damage::State().stage);
    if(!ReadLife(p))return false;p.lastSample=state.now;
    if(!p.alive&&p.phase==Phase::Alive){
        if(!MarkDead(0,state.now,false))return false;
        if(state.teamRecordsReady&&!SynchronizeNativeTeamPoints())return false;
    }
    return true;
}
inline bool RespawnHumanImpl(){
    if(!Thread())return Fail(3316,"server_human_respawn_boundary_requires_authority_game_thread");
    if(state.error)return false;
    if(!state.started||!state.humanRegistered||!players[0].present||pending.active)return true;
    state.now=Now();return Respawn(0);
}
inline bool Configure(const training_control::Configuration& selected){
    if(!Thread()||!state.started||!Valid(selected))return false;
    if(selected.mode!=initial.config.mode||selected.totalPlayers!=initial.config.totalPlayers)return false;
    initial.config=selected;return training_bot_controller::Configure(BotConfig());
}
inline bool SetRoutes(const training_control::RouteSet& routes){
    if(!Thread()||!state.started||!ValidRoutes(routes))return false;
    const auto previous=initial.routes;initial.routes=routes;
    if(!ApplyRoutes()){initial.routes=previous;return false;}
    return training_bot_controller::ValidateRoutes(state.now)
        ?true:Fail(training_bot_controller::State().error,training_bot_controller::State().stage);
}
inline bool PrepareAnchors(){
    std::array<Vec,64> anchors{};size_t count=0;
    for(uint32_t n=0;n<initial.spawnCount;++n){Vec projected{},feet{};const auto point=Vector(initial.spawns[n]);
        if(Project(point,projected)?training_world_geometry::Ground(projected,feet,30,100):training_world_geometry::Ground(point,feet))anchors[count++]=feet;
    }
    if(FreeForAll()&&!count)return Fail(3279,"server_ffa_requires_native_grounded_original_roaming_anchors");
    return training_bot_controller::SetAnchors(anchors.data(),count);
}
inline bool PollImpl(double delta){
    if(!Thread()||!std::isfinite(delta)||delta<0)return Fail(3280,"server_match_poll_delta_or_game_thread_invalid");
    if(state.error)return false;state.now=Now();state.accumulatedDelta+=delta;++state.polls;
    if(!state.started){state.stage="server_match_waiting_configuration";return true;}
    if(!training_maps::Poll())return Fail(training_maps::State().error,training_maps::State().stage);
    state.mapsReady=training_maps::Ready();
    if(!state.mapsReady){state.stage="server_match_waiting_native_map_layers";return true;}
    if(!state.humanRegistered){state.stage="server_match_waiting_native_admitted_selected_hero";return true;}
    if(!state.rosterReady){
        for(size_t n=1;n<state.wantedPlayers;++n)if(!players[n].present){state.stage="server_match_building_authority_npc_roster";return SpawnNPC(n);}
        if(!CheckAlliances()||!PrepareAnchors()||!EnsureNativeTeamRecords()||!SynchronizeNativeTeamPoints()
           ||!training_bot_controller::ValidateRoutes(state.now))return false;
        state.rosterReady=true;state.stage="server_match_waiting_authority_equipment_and_tick_readbacks";
    }
    bool bound=true,simulation=true;state.npcSimulationObserved=false;state.movementObservedNPCs=0;
    for(size_t n=0;n<state.wantedPlayers;++n){auto& p=players[n];if(!p.present)return Fail(3281,"server_match_roster_slot_disappeared");
        if(original_inventory_slots::ResolveIdentity(p.pawnIdentity)!=p.pawn)return Fail(3282,"server_match_native_pawn_expired_before_retirement");
        if(state.now-p.lastSample>=.04){
            if(!original_damage::Sample(n))return Fail(original_damage::State().error,original_damage::State().stage);
            if(!ReadLife(p))return false;p.lastSample=state.now;
            Vec center{};if(!Event(p.pawn,L"/Script/Engine.Actor.K2_GetActorLocation",24,&center))return Fail(3283,"server_match_native_pawn_location_query_failed");
            center.z-=Read<float>(Read<void*>(p.pawn,0x500),0x6d8);p.lastFeet=p.feet;p.feet=center;
            if(!p.human){uint8_t local=0;
                if(!Event(p.pawn,L"/Script/Engine.Pawn.IsLocallyControlled",1,&local))return Fail(3284,"server_npc_native_control_eligibility_query_failed");
                p.nativeLocallyControlled=local!=0;
                if(p.nativeLocallyControlled&&!p.inputPrepared&&!PrepareNPCInput(n))return false;
                const auto displacement=training_bot_controller::Sub(p.feet,p.lastFeet);
                const auto& submitted=training_bot_controller::actors[n].output.worldDirection;
                if(training_bot_controller::Length2D(displacement)>.1&&(p.moveRequests||p.externalMoveRequests)
                   &&displacement.x*submitted.x+displacement.y*submitted.y>0){++p.movingSamples;p.movementObserved=true;}
            }
            if(!p.alive&&p.phase==Phase::Alive&&!MarkDead(n,state.now,false))return false;
        }
        if(!RefreshStreak(n)||!Respawn(n))return false;
        p.externalAuthorityReady=!p.human&&hooks.submitBot&&hooks.npcAuthorityReady&&hooks.npcAuthorityReady(n,p.pawn,p.epoch);
        p.equipmentReady=p.equipmentRegistered&&(!hooks.equipmentReady||hooks.equipmentReady(n,p.pawn,p.epoch));
        bound=bound&&p.equipmentRegistered&&(state.startedAt>=0||p.equipmentReady);
        simulation=simulation&&(p.human||p.nativeLocallyControlled||p.externalAuthorityReady);
        if(!p.human&&p.movementObserved)++state.movementObservedNPCs;
    }
    state.npcSimulationObserved=state.spawnedNPCs>0&&state.movementObservedNPCs==state.spawnedNPCs;
    if(!SynchronizeNativeTeamPoints())return false;
    state.ready=bound&&simulation;
    if(state.ready&&state.startedAt<0){state.startedAt=state.now;state.active=true;}
    if(state.active&&FreeForAll())state.remaining=(std::max)(0.,DurationSeconds-(state.now-state.startedAt));
    if(state.active&&FreeForAll()&&state.remaining<=0)local_server_match::EndMatch(EndReason::TimeLimit);
    if(!TickBots())return false;RefreshLeader();
    if(!state.ended)state.stage=!bound?"server_match_waiting_authority_equipment_binding":!simulation?"server_match_npc_original_authority_tick_gate_unconfirmed"
        :!FreeForAll()&&!initial.routes.count?"server_match_practice_waiting_saved_routes"
        :state.npcSimulationObserved?"server_match_authority_inputs_and_native_movement_observed":"server_match_authority_inputs_submitted_native_movement_unproven";
    return true;
}
inline LONG Capture(EXCEPTION_POINTERS* e){
    const auto at=e&&e->ExceptionRecord?reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;state.restartRequired=true;state.active=false;
    Fail(e&&e->ExceptionRecord?e->ExceptionRecord->ExceptionCode:0xe0003200U,"server_match_native_exception_requires_new_session");return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* world,const Hooks& callbacks={}){__try{return InitializeImpl(world,callbacks);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool RegisterHuman(void* pc,void* ps,void* pawn,uint32_t epoch=1){__try{return RegisterHumanImpl(pc,ps,pawn,epoch);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool Start(const training_control::Initial& selected){__try{return StartImpl(selected);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool Poll(double delta){__try{return PollImpl(delta);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool PreprocessDeaths(){__try{return PreprocessDeathsImpl();}__except(Capture(GetExceptionInformation())){return false;}}
inline bool RespawnHuman(){__try{return RespawnHumanImpl();}__except(Capture(GetExceptionInformation())){return false;}}
inline size_t RegistryActors(void** output,size_t capacity){
    size_t count=0;
    const auto add=[&](void* object){if(!object)return;for(size_t n=0;n<(std::min)(count,capacity);++n)if(output&&output[n]==object)return;
        if(output&&count<capacity)output[count]=object;++count;};
    add(state.gameState);
    // A remote client owns only its admitted PC. NPC controllers are private
    // server decision/input owners and must never be manually channelled as
    // foreign PlayerControllers. Their real PS/Pawn and team actors replicate.
    for(const auto& p:players)if(p.present){if(p.human)add(p.pc);add(p.ps);add(p.pawn);add(p.info);add(p.teamActor);}
    if(state.gameState){const auto teams=Read<ObjectArray>(state.gameState,0x720);
        if(ArrayValid(teams,128))for(int32_t n=0;n<teams.count;++n)if(original_players::IsA(teams.data[n],0x65af170))add(teams.data[n]);}
    return count;
}
inline size_t RegistrySubobjects(void** output,size_t capacity){
    size_t count=0;const auto add=[&](void* object){if(!object)return;for(size_t n=0;n<(std::min)(count,capacity);++n)if(output&&output[n]==object)return;
        if(output&&count<capacity)output[count]=object;++count;};
    for(const auto& p:players)if(p.present){add(Read<void*>(p.ps,0x6a8));add(Read<void*>(p.teamActor,0x478));add(p.stats);
        void* native=nullptr;
        if(Event(p.info,L"/Script/ShooterGame.OwnerExclusivePlayerInfo.GetKillStreakComponentMutable",8,&native))add(native);
        if(p.nativeStreakBound){const auto entry=original_timed_streak::Player(p.index);if(entry&&entry->bound)add(entry->component);}
    }
    if(state.gameState){const auto teams=Read<ObjectArray>(state.gameState,0x720);
        if(ArrayValid(teams,128))for(int32_t n=0;n<teams.count;++n)if(original_players::IsA(teams.data[n],0x65af170))add(Read<void*>(teams.data[n],0x478));}
    return count;
}
inline size_t GetScores(Score* output,size_t capacity){
    size_t count=0;for(const auto& p:players)if(p.present){if(output&&count<capacity)output[count]={static_cast<uint32_t>(p.index),p.epoch,p.kills,p.deaths,p.streak,p.ps,p.human,p.alive,p.nativeWindowRate,p.nativeWindowRemaining,p.playerId};++count;}return count;
}
struct Json {
    std::string value;
    void Add(const char* format,...){char buffer[2048]{};va_list arguments;va_start(arguments,format);const int n=vsnprintf(buffer,sizeof(buffer),format,arguments);va_end(arguments);if(n>0&&n<static_cast<int>(sizeof(buffer)))value.append(buffer,static_cast<size_t>(n));}
    void String(const char* source){value+='"';for(auto p=reinterpret_cast<const unsigned char*>(source?source:"");*p;++p){if(*p=='"'||*p=='\\'){value+='\\';value+=static_cast<char>(*p);}else if(*p<32)Add("\\u%04x",static_cast<unsigned>(*p));else value+=static_cast<char>(*p);}value+='"';}
    void Vector(const Vec& p){Add("[%.5f,%.5f,%.5f]",p.x,p.y,p.z);}
};
inline std::string StateJson(){
    Json out;const auto b=[](bool v){return v?"true":"false";};
    out.Add("{\"schema_version\":1,\"ready\":%s,\"active\":%s,\"started\":%s,\"ended\":%s,\"mode\":\"%s\",\"map_id\":",b(state.ready),b(state.active),b(state.started),b(state.ended),FreeForAll()?"deathmatch":"practice");out.String(initial.mapId);
    out.Add(",\"stage\":");out.String(state.stage);out.Add(",\"operation\":");out.String(state.operation);
    out.Add(",\"configured_bot_roster\":");local_server_bot_roster_initial::WriteJson(out,[](size_t slot){const auto*p=local_server_match::Player(slot);return p&&p->playerIdAssigned&&p->playerId>0?p->playerId:0;});
    out.Add(",\"error_code\":%u,\"net_mode\":%d,\"participants\":%u,\"wanted_players\":%u,\"npc_simulation_observed\":%s,\"native_ffa_alliances\":%s,\"unique_heroes\":%s,\"shuffle_seed\":%u,\"time_remaining\":%.4f,\"winner\":%d,\"leader\":%d,\"end_reason\":%u,\"kills\":%u,\"respawns\":%u,\"human_respawns\":%u,\"players\":[",
        state.error,state.netMode,state.participants,state.wantedPlayers,b(state.npcSimulationObserved),b(state.originalFFAConfirmed),b(state.uniqueHeroes),state.shuffleSeed,state.remaining,state.winner,state.leader,static_cast<unsigned>(state.endReason),state.kills,state.respawns,state.humanRespawns);
    bool comma=false;for(const auto& p:players)if(p.present){const auto& ai=training_bot_controller::actors[p.index];
        out.Add("%s{\"index\":%zu,\"player_id\":%d,\"player_id_assigned\":%s,\"player_id_respawn_revalidations\":%u,\"account_unique_id_unchanged\":%s,\"team_points_verified\":%s,\"bot\":%s,\"alive\":%s,\"ready\":%s,\"epoch\":%u,\"hero_index\":%u,\"health\":%.4f,\"kills\":%u,\"deaths\":%u,\"streak\":%u,\"feet\":",comma?",":"",p.index,p.playerId,b(p.playerIdAssigned),p.playerIdRevalidations,b(p.uniqueIdPreserved),b(p.teamPointsVerified),b(!p.human),b(p.alive),b(p.ready),p.epoch,p.hero,p.health,p.kills,p.deaths,p.streak);out.Vector(p.feet);
        local_server_bot_roster_initial::PlayerFields(out,p.index,p.human,p.playerId,p.playerIdAssigned);
        out.Add(",\"native_team_actor\":\"%p\",\"assigned_team_state_verified\":%s,\"native_selection_verified\":%s,\"native_portrait_ready\":%s,\"native_portrait\":\"%p\",\"native_team_points\":%d",
            p.teamActor,b(p.assignedTeamStateVerified),b(p.nativeSelectionVerified),b(p.nativePortraitReady),p.nativePortrait,Read<int32_t>(p.teamActor,0x4b8));
        out.Add(",\"external_authority_ready\":%s,\"external_intent_submissions\":%llu,\"external_move_requests\":%llu,\"external_jump_requests\":%llu,\"external_crouch_requests\":%llu",
            b(p.externalAuthorityReady),static_cast<unsigned long long>(p.externalInputSubmissions),static_cast<unsigned long long>(p.externalMoveRequests),
            static_cast<unsigned long long>(p.externalJumpRequests),static_cast<unsigned long long>(p.externalCrouchRequests));
        out.Add(",\"name\":");const auto label=original_players::Utf8(original_players::players[p.index].requestedName);out.String(label.c_str());
        out.Add(",\"stage\":");out.String(p.stage);
        out.Add(",\"native_locally_controlled\":%s,\"native_movement_observed\":%s,\"input_prepared\":%s,\"equipment_registered\":%s,\"equipment_ready\":%s,\"input_frames\":%llu,\"move_requests\":%llu,\"pending_input_commands\":%d,\"role\":%u,\"remote_role\":%u,\"native_window_rate\":%.5f,\"native_window_remaining\":%.5f,\"profile\":{\"reaction_ms\":%.4f,\"aim_speed\":%.4f,\"accuracy\":%.5f,\"movement\":%.5f}}",
            b(p.nativeLocallyControlled),b(p.movementObserved),b(p.inputPrepared),b(p.equipmentRegistered),b(p.equipmentReady),static_cast<unsigned long long>(p.inputFrames),static_cast<unsigned long long>(p.moveRequests),p.pendingInputCommands,static_cast<unsigned>(p.role),static_cast<unsigned>(p.remoteRole),p.nativeWindowRate,p.nativeWindowRemaining,ai.profile.reaction*1000,ai.profile.turnSpeed,ai.profile.accuracy,ai.profile.movement);comma=true;
    }
    out.Add("],\"routes\":[");for(uint32_t n=0;n<initial.routes.count;++n){if(n)out.Add(",");out.Add("{\"id\":%u,\"status\":",initial.routes.items[n].id);
        const auto result=training_bot_controller::RouteValidationAt(n);out.String(result?training_bot_controller::RouteCheckStatusName(result->status):"pending");out.Add(",\"message\":");out.String(result?result->reason:"awaiting_native_route_validation");out.Add("}");}
    out.Add("],\"identity_profile_ready\":%s,\"native_team_records_ready\":%s,\"npc_player_ids_assigned\":%u,\"player_id_getter_readbacks\":%u,\"native_team_actors_created\":%u,\"native_team_points_writes\":%u,\"native_team_points_notifications\":%u,\"native_team_membership_notifications\":%u,\"assigned_team_state_writes\":%u,\"assigned_team_state_readbacks\":%u,\"native_practice_team_groups\":%u,\"practice_team_points_preserved\":%s,\"external_intent_submissions\":%llu,\"foreign_npc_controller_replication_requested\":false,\"original_native_replication_validation_required\":true,\"runtime_acceptance_claimed\":false}",
        b(state.identityProfileReady),b(state.teamRecordsReady),state.playerIdsAssigned,state.playerIdReadbacks,state.teamActorsCreated,state.teamPointsWrites,state.teamPointsNotifications,
        state.teamMembershipNotifications,state.assignedTeamStateWrites,state.assignedTeamStateReadbacks,state.practiceTeams,b(!FreeForAll()),static_cast<unsigned long long>(state.externalBotInputSubmissions));return out.value;
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* out=nullptr;if(_wfopen_s(&out,path,L"wb")||!out)return false;
    const auto json=StateJson();const auto written=std::fwrite(json.data(),1,json.size(),out);const bool okay=written==json.size()&&!std::ferror(out);std::fclose(out);return okay;
}
} // namespace local_server_match
