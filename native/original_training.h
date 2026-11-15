#pragma once
#include "original_effects.h"
#include "original_damage_tuning.h"
#include "original_scoreboard.h"
#include "original_enemy_respawn.h"
#include "original_enemy_weapon.h"
#include "original_inventory_slots.h"
#include "original_enemy_view.h"
#include "original_render.h"
#include "original_camera_owners.h"
#include <array>
#include <limits>

// Missing local training authority only. Original FireShot provides both the
// direction and its exact source; original body physics, authored damage curves,
// native DamageResponse/OnDeath and fresh original heroes do the actual work.
// No shot/recoil generator, proxy hitbox, direct health write or replacement
// montage is present. The observer Sink only copies an event for the next Poll.
namespace original_training {
inline constexpr size_t MaxTargets=original_players::MaxPlayers,QueueCapacity=128,RetiredCapacity=128;
static_assert(MaxTargets<=32);
struct Options {
    double maximumDistance=100000,maximumShotAge=.25,respawnDelay=1.5,sampleInterval=.05;
    double obstructionTolerance=.5;
    float oldGunLifetime=10;
    size_t maximumShotsPerPoll=16;
};
inline Options options;
enum class Phase:uint8_t {WaitingPhysics,Alive,DeadWaiting,Rebuilding};
inline const char* PhaseName(Phase p){
    switch(p){case Phase::WaitingPhysics:return "waiting_original_body_physics";
        case Phase::Alive:return "original_alive";case Phase::DeadWaiting:return "original_dead_waiting_respawn";
        default:return "rebuilding_original_character";}
}
struct Target {
    bool bound=false,alive=false,physicsReady=false,weaponReady=false,inputsStopped=false;
    Phase phase=Phase::WaitingPhysics;
    void *pawn=nullptr,*playerState=nullptr,*weapon=nullptr,*lastCorpse=nullptr,*lastOldGun=nullptr;
    original_inventory_slots::WeakIdentity pawnIdentity{},weaponIdentity{};
    engine::Vec spawnFeet{};double spawnYaw=180,lastSampleAt=-1000,deathAt=-1,respawnAt=-1;
    uint64_t acceptAfterSequence=0,lastDamageSequence=0,lastKillSequence=0;
    uint32_t logicalId=0,respawns=0,damageEvents=0,kills=0,physicsWaitSamples=0;
    int32_t bodyCount=0,physicsActors=0;
    float health=0;
};
struct Pending {original_shots::Shot shot;uint32_t eligibleTargets=0;};
struct Retired {
    bool occupied=false;void* actor=nullptr;original_inventory_slots::WeakIdentity identity{};
};
struct Diagnostics {
    bool initialized=false,ready=false,restartRequired=false,testStationary=false,processing=false;
    unsigned error=0,managedTargets=0,aliveTargets=0,physicsReadyTargets=0,equippedTargets=0;
    uint64_t callbacks=0,queued=0,processed=0,unpairedRejected=0,invalidRejected=0,duplicates=0;
    uint64_t staleRejected=0,queueOverflow=0,emptyEligibility=0,traceQueries=0,traceHits=0,misses=0;
    uint64_t worldQueries=0,sceneOnlyQueries=0,worldBlocked=0,physicsDeferred=0,pendingDamageDeferred=0;
    uint64_t damageEvents=0,sectionRejected=0,kills=0,externalDeaths=0,scoreUpdates=0,respawns=0;
    uint64_t oldGunTimers=0,oldGunAlreadyTimed=0,oldGunAlreadyGone=0,oldGunTransferred=0;
    uint64_t lastQueuedSequence=0,lastProcessedSequence=0,lastKillSequence=0;
    double lastPollAt=-1,lastWorldDistance=0,lastBodyDistance=0;
    float lastDamage=0,lastFalloff=0;
    uint8_t lastRegion=0;
    int32_t lastTarget=-1;
    bool lastNativeFalloffFeature=false,lastOriginalResponse=false,lastOriginalLifeReadback=false;
    bool lastOriginalDeathMontage=false;
    uintptr_t exceptionRva=0;
    const char* stage="original_training_not_initialized";
    const char* operation="none";
    const char* lastOutcome="none";
};
inline Diagnostics diagnostics;
inline const Diagnostics& State(){return diagnostics;}
inline std::array<Target,MaxTargets> targets{};
inline std::array<Pending,QueueCapacity> queue{};
inline std::array<Retired,RetiredCapacity> retired{};
inline uint64_t readIndex=0,writeIndex=0;
inline DWORD gameThread=0;
inline void *world=nullptr,*sourcePawn=nullptr,*sourceState=nullptr,*sourceWeapon=nullptr,*sourceFiring=nullptr;
inline uint32_t randomState=1;

template<class T> inline T Read(const void* p,size_t offset){return original_players::Value<T>(p,offset);}
inline bool Fail(unsigned error,const char* stage){
    diagnostics.error=error?error:1800;diagnostics.stage=stage;diagnostics.restartRequired=true;return false;
}
inline bool Thread(){return diagnostics.initialized&&!diagnostics.restartRequired&&gameThread==GetCurrentThreadId();}
inline LONG CaptureException(EXCEPTION_POINTERS* exception){
    const auto error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0001800U;
    const auto pc=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    diagnostics.exceptionRva=pc>=engine::image?pc-engine::image:pc;
    Fail(error,"original_training_native_exception_requires_new_session");return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Event(void* object,const wchar_t* name,uint16_t size,void* parameters){
    return original_damage::Event(object,name,size,parameters);
}
inline void QueueShot(const original_shots::Shot& shot){
    // Called inside the original event dispatch. No native calls or damage here.
    if(!Thread())return;
    ++diagnostics.callbacks;
    if((shot.flags&12U)!=12U){++diagnostics.unpairedRejected;return;}
    const double norm=shot.direction.x*shot.direction.x+shot.direction.y*shot.direction.y+shot.direction.z*shot.direction.z;
    if(!shot.sequence||!std::isfinite(shot.observed_at)||!original_shots::WorldPoint(shot.start)
       ||!original_shots::Finite(shot.direction)||norm<.9||norm>1.1){++diagnostics.invalidRejected;return;}
    if(shot.sequence<=diagnostics.lastQueuedSequence){++diagnostics.duplicates;return;}
    diagnostics.lastQueuedSequence=shot.sequence;
    if(writeIndex-readIndex>=queue.size()){++diagnostics.queueOverflow;return;}
    uint32_t eligible=0;
    for(size_t i=0;i<targets.size();++i){const auto& t=targets[i];
        if(t.bound&&t.alive&&t.physicsReady&&t.phase==Phase::Alive)eligible|=uint32_t(1)<<i;
    }
    queue[writeIndex++%queue.size()]={shot,eligible};++diagnostics.queued;
}
inline bool StopInput(size_t index){
    auto& t=targets[index];if(t.inputsStopped)return true;
    if(!t.bound||original_inventory_slots::ResolveIdentity(t.pawnIdentity)!=t.pawn)
        return Fail(1827,"original_training_stop_input_requires_current_original_pawn");
    const engine::Vec stopped{};
    if(!original_enemies::SubmitWorldInput(index,stopped,original_enemies::enemies[index].yaw))
        return Fail(original_enemies::error?original_enemies::error:1801,"original_training_original_stop_input_failed");
    t.inputsStopped=true;return true;
}
inline bool RememberRetired(void* actor,const original_inventory_slots::WeakIdentity& identity){
    if(!actor||identity.index<0||identity.serial<=0)return true;
    for(auto& r:retired){
        if(r.occupied&&original_inventory_slots::SameIdentity(r.identity,identity))return true;
        if(r.occupied&&original_inventory_slots::ResolveIdentity(r.identity)!=r.actor)r={};
    }
    for(auto& r:retired)if(!r.occupied){r={true,actor,identity};return true;}
    return Fail(1802,"original_training_live_corpse_weak_cache_full");
}
inline bool MarkDead(size_t index,double now,bool credited){
    auto& t=targets[index];original_enemy_respawn::Life life;
    if(!original_enemy_respawn::ReadLife(t.pawn,life)||!original_enemy_respawn::Dead(life))
        return Fail(original_enemy_respawn::State().error?original_enemy_respawn::State().error:1803,
                    "original_training_death_requires_original_health_and_alive_readbacks");
    t.alive=false;t.physicsReady=false;t.phase=Phase::DeadWaiting;t.deathAt=now;t.respawnAt=now+options.respawnDelay;
    original_enemies::enemies[index].enabled=false; // AI cache only; keep native corpse rendering.
    if(!StopInput(index))return false;
    if(!credited)++diagnostics.externalDeaths;
    return true;
}
inline bool RefreshTarget(size_t index,double now,bool force=false){
    auto& t=targets[index];if(!t.bound)return true;
    if(original_players::players[index].character!=t.pawn||original_enemies::enemies[index].pawn!=t.pawn
       ||original_inventory_slots::ResolveIdentity(t.pawnIdentity)!=t.pawn)
        return Fail(1804,"original_training_original_target_identity_changed_or_expired");
    if(!force&&now-t.lastSampleAt<options.sampleInterval)return true;
    if(!original_damage::Sample(index))return Fail(original_damage::State().error,original_damage::State().stage);
    const auto& d=original_damage::targets[index];t.lastSampleAt=now;t.health=d.health;t.alive=d.alive;
    t.bodyCount=d.bodyCount;t.physicsActors=d.physicsActors;t.physicsReady=d.originalPhysics;
    if(!t.alive){if(t.phase!=Phase::DeadWaiting)return MarkDead(index,now,false);return true;}
    if(t.phase==Phase::DeadWaiting)return Fail(1805,"original_training_dead_character_became_alive_without_respawn");
    if(!original_enemy_weapon::Sample(index))return Fail(original_enemy_weapon::error?original_enemy_weapon::error:1806,
                                                       "original_training_original_enemy_weapon_sample_failed");
    const auto& gun=original_enemy_weapon::entries[index];
    if(gun.pawn!=t.pawn||gun.weapon!=t.weapon)return Fail(1807,"original_training_original_enemy_weapon_identity_changed");
    t.weaponReady=gun.ready;t.phase=t.physicsReady?Phase::Alive:Phase::WaitingPhysics;
    original_enemies::enemies[index].enabled=t.physicsReady;
    if(!t.physicsReady){++t.physicsWaitSamples;++diagnostics.physicsDeferred;if(!StopInput(index))return false;}
    else if(diagnostics.testStationary||!t.weaponReady){if(!StopInput(index))return false;}
    else t.inputsStopped=false;
    return true;
}
inline bool BindTarget(size_t index,double now,bool replacement){
    auto& t=targets[index];const auto& roster=original_players::players[index];const auto& enemy=original_enemies::enemies[index];
    if(!roster.ready||roster.local||!roster.registered||!enemy.ready||roster.character!=enemy.pawn)
        return Fail(1808,"original_training_requires_registered_original_enemy");
    if(!original_damage::Bind(index,roster.character))return Fail(original_damage::State().error,original_damage::State().stage);
    if(!original_scoreboard::RegisterPlayer(roster.state))return Fail(original_scoreboard::State().error,original_scoreboard::State().stage);
    if(!original_enemy_weapon::entries[index].weapon||original_enemy_weapon::entries[index].pawn!=roster.character)
        return Fail(1809,"original_training_original_enemy_ak_required");
    if(!t.bound){t.spawnFeet=enemy.anchor;t.spawnYaw=enemy.yaw;++diagnostics.managedTargets;}
    t.bound=true;t.pawn=roster.character;t.playerState=roster.state;t.weapon=original_enemy_weapon::entries[index].weapon;
    if(!original_inventory_slots::CaptureIdentity(t.pawn,t.pawnIdentity)
       ||!original_inventory_slots::CaptureIdentity(t.weapon,t.weaponIdentity))
        return Fail(1810,"original_training_original_weak_identity_capture_failed");
    t.phase=Phase::WaitingPhysics;t.inputsStopped=false;t.logicalId=enemy.logicalId;
    t.acceptAfterSequence=original_shots::State().rays;t.lastSampleAt=-1000;t.deathAt=t.respawnAt=-1;
    if(replacement){++t.respawns;++diagnostics.respawns;}
    return RefreshTarget(index,now,true);
}
inline uint32_t NextLogicalId(uint32_t oldHero){
    // Explicit local bot-selection policy; this PRNG never affects firing.
    for(unsigned attempt=0;attempt<32;++attempt){
        randomState^=randomState<<13;randomState^=randomState>>17;randomState^=randomState<<5;
        if(randomState&&original_enemies::HeroFor(randomState)!=oldHero)return randomState;
    }
    uint32_t id=randomState?randomState:1;
    while(original_enemies::HeroFor(id)==oldHero)if(!++id)++id;
    randomState=id;return id;
}
inline bool CleanupOldGun(Target& t,void* oldPawn){
    t.lastOldGun=t.weapon;
    if(!t.weapon){++diagnostics.oldGunAlreadyGone;return true;}
    // RetireActor resolves its cached weak identity before touching any object.
    if(t.weapon&&!original_effects::RetireActor(t.weapon))return Fail(original_effects::State().error,original_effects::State().stage);
    auto live=original_inventory_slots::ResolveIdentity(t.weaponIdentity);
    if(live!=t.weapon){++diagnostics.oldGunAlreadyGone;return true;}
    auto owner=Read<void*>(live,0x4a8);
    if(owner&&owner!=oldPawn){++diagnostics.oldGunTransferred;return true;}
    float remaining=0;
    if(!Event(live,L"/Script/Engine.Actor.GetLifeSpan",4,&remaining)||!std::isfinite(remaining))
        return Fail(1811,"original_training_old_gun_original_lifespan_query_failed");
    if(remaining>0)++diagnostics.oldGunAlreadyTimed;
    else {
        float delay=options.oldGunLifetime;
        if(!Event(live,L"/Script/Engine.Actor.SetLifeSpan",4,&delay)
           ||!Event(live,L"/Script/Engine.Actor.GetLifeSpan",4,&remaining)||!std::isfinite(remaining)||remaining<=0)
            return Fail(1812,"original_training_old_gun_original_cleanup_timer_failed");
        ++diagnostics.oldGunTimers;
    }
    return RememberRetired(live,t.weaponIdentity);
}
inline bool Respawn(size_t index,double now){
    auto& t=targets[index];if(t.phase!=Phase::DeadWaiting||now<t.respawnAt)return true;
    original_enemy_respawn::Life life;
    if(original_inventory_slots::ResolveIdentity(t.pawnIdentity)!=t.pawn
       ||!original_enemy_respawn::ReadLife(t.pawn,life)||!original_enemy_respawn::Dead(life))
        return Fail(1813,"original_training_respawn_requires_existing_original_dead_character");
    const auto oldPawn=t.pawn;const auto oldIdentity=t.pawnIdentity;
    const auto oldHero=original_enemies::enemies[index].heroIndex;
    if(!StopInput(index)||!CleanupOldGun(t,oldPawn))return false;
    if(!original_effects::RetireActor(oldPawn))return Fail(original_effects::State().error,original_effects::State().stage);
    if(!original_inventory_slots::RetirePawn(oldPawn))return Fail(original_inventory_slots::State().error,original_inventory_slots::State().stage);
    if(!RememberRetired(oldPawn,oldIdentity))return false;
    t.phase=Phase::Rebuilding;diagnostics.operation="original_respawn_preserve_roster";
    original_enemy_respawn::Result result;
    if(!original_enemy_respawn::Recreate(index,NextLogicalId(oldHero),t.spawnFeet,t.spawnYaw,result))
        return Fail(original_enemy_respawn::State().error,original_enemy_respawn::State().stage);
    if(!result.ready||!result.deathVerified||!result.newLifeVerified||result.oldPawn!=oldPawn||result.newPawn==oldPawn
       ||!result.sameController||!result.sameState||!result.sameInfo||!result.sameStats||!result.sameRoster
       ||!result.localPlayerPreserved||!result.corpseMeshUnchanged)
        return Fail(1814,"original_training_respawn_original_identity_invariants_failed");
    t.lastCorpse=oldPawn;
    if(!original_camera_owners::Apply(index))return Fail(original_camera_owners::State().error,original_camera_owners::State().stage);
    diagnostics.operation="original_respawn_inventory_effects_view_equipment";
    if(!original_inventory_slots::EnsureForPawn(result.newPawn))return Fail(original_inventory_slots::State().error,original_inventory_slots::State().stage);
    if(!original_effects::EnsureActor(result.newPawn,"respawned_enemy_character"))return Fail(original_effects::State().error,original_effects::State().stage);
    if(!original_enemy_view::Apply(index))return Fail(original_enemy_view::State().error,original_enemy_view::State().stage);
    // Retire only our grant cache after confirmed native death and a DIFFERENT
    // fresh original Pawn. The old original inventory and corpse are preserved.
    auto& gun=original_enemy_weapon::entries[index];
    if(gun.pawn&&gun.pawn!=oldPawn)return Fail(1815,"original_training_enemy_weapon_cache_belongs_to_other_pawn");
    gun={};
    if(!original_enemy_weapon::SpawnAK(index))return Fail(original_enemy_weapon::error,original_enemy_weapon::stage);
    if(!original_enemy_view::Apply(index))return Fail(original_enemy_view::State().error,original_enemy_view::State().stage);
    if(!BindTarget(index,now,true))return false;
    diagnostics.operation="none";return true;
}
inline int SceneTrace(const original_shots::Shot& shot,double& distance){
    std::array<void*,MaxTargets*2+4+RetiredCapacity> ignored{};int32_t count=0;
    const auto add=[&](void* actor){if(!actor)return;for(int32_t j=0;j<count;++j)if(ignored[j]==actor)return;
        if(static_cast<size_t>(count)<ignored.size())ignored[count++]=actor;};
    add(sourcePawn);add(sourceWeapon);
    for(const auto& t:targets)if(t.bound){
        // A live actor whose real body physics is unready remains native scene
        // obstruction. It is never replaced by capsule-based damage.
        if((!t.alive||t.physicsReady)&&original_inventory_slots::ResolveIdentity(t.pawnIdentity)==t.pawn)add(t.pawn);
        if(original_inventory_slots::ResolveIdentity(t.weaponIdentity)==t.weapon)add(t.weapon);
    }
    for(auto& r:retired)if(r.occupied){
        if(original_inventory_slots::ResolveIdentity(r.identity)==r.actor)add(r.actor);else r={};
    }
    // Same original Kismet LineTraceSingle ABI as engine::Trace. Exclude only
    // owned training actors so their capsules/guns/corpses cannot masquerade as
    // scene cover. Hero hits still require their actual native19-body trace.
    alignas(16) uint8_t params[0x178]{};
    const auto end=engine::Add(shot.start,engine::Mul(shot.direction,options.maximumDistance));
    std::memcpy(params,&world,8);std::memcpy(params+8,&shot.start,24);std::memcpy(params+0x20,&end,24);
    params[0x39]=1;params[0x148]=1;
    struct Array {void** data;int32_t count,capacity;} array{ignored.data(),count,count};
    std::memcpy(params+0x40,&array,sizeof(array));++diagnostics.sceneOnlyQueries;
    auto receiver=engine::DefaultObject(engine::Class(0x45aa110));
    if(!Event(receiver,L"/Script/Engine.KismetSystemLibrary.LineTraceSingle",0x171,params))return -1;
    if(!params[0x170]||!(params[0x105]&1)){distance=(std::numeric_limits<double>::infinity)();return 0;}
    distance=Read<float>(params,0x60);
    return std::isfinite(distance)&&distance>=0?1:-1;
}
inline bool Process(const Pending& pending,double now){
    const auto& shot=pending.shot;++diagnostics.processed;diagnostics.lastProcessedSequence=shot.sequence;
    original_render::CaptureViewSample(shot.sequence,shot.observed_at,&shot.start,&shot.direction);
    diagnostics.lastTarget=-1;diagnostics.lastOutcome="native_body_miss";
    if(now-shot.observed_at<-.05||now-shot.observed_at>options.maximumShotAge){
        ++diagnostics.staleRejected;diagnostics.lastOutcome="expired_original_shot";return true;
    }
    if(!pending.eligibleTargets){++diagnostics.emptyEligibility;++diagnostics.misses;
        diagnostics.lastOutcome="no_original_body_physics_ready_at_shot";return true;}
    size_t nearest=targets.size();original_damage::Hit nearestHit;
    double nearestDistance=options.maximumDistance;
    for(size_t i=0;i<targets.size();++i){
        if(!(pending.eligibleTargets&(uint32_t(1)<<i)))continue;
        if(!RefreshTarget(i,now,true))return false;
        auto& t=targets[i];
        if(!t.alive||!t.physicsReady||t.phase!=Phase::Alive||shot.sequence<=t.acceptAfterSequence)continue;
        original_damage::Hit hit;++diagnostics.traceQueries;
        const int result=original_damage::Trace(i,shot,options.maximumDistance,hit);
        if(result<0){if(original_damage::State().error==1134){t.physicsReady=false;t.phase=Phase::WaitingPhysics;
                ++diagnostics.physicsDeferred;++diagnostics.misses;
                diagnostics.lastOutcome="original_body_physics_became_unready";return true;}
            return Fail(original_damage::State().error,original_damage::State().stage);}
        if(result==1&&hit.distance<nearestDistance){nearest=i;nearestHit=hit;nearestDistance=hit.distance;}
    }
    if(nearest==targets.size()){++diagnostics.misses;return true;}
    ++diagnostics.traceHits;diagnostics.lastTarget=static_cast<int32_t>(nearest);diagnostics.lastBodyDistance=nearestDistance;
    double obstruction=0;++diagnostics.worldQueries;
    int worldHit=engine::Trace(world,shot.start,shot.direction,options.maximumDistance,obstruction,sourcePawn);
    if(worldHit<0)return Fail(1816,"original_training_original_world_trace_failed");
    if(worldHit&&obstruction+options.obstructionTolerance<nearestDistance){
        worldHit=SceneTrace(shot,obstruction);
        if(worldHit<0)return Fail(1817,"original_training_original_scene_only_trace_failed");
    }
    diagnostics.lastWorldDistance=std::isfinite(obstruction)?obstruction:-1;
    if(worldHit&&obstruction+options.obstructionTolerance<nearestDistance){++diagnostics.worldBlocked;
        diagnostics.lastOutcome="original_scene_obstructed";return true;}
    if(original_damage::targets[nearest].pendingDamageEvents){++diagnostics.pendingDamageDeferred;
        diagnostics.lastOutcome="original_damage_notification_pending";return true;}
    original_damage_tuning::Calculation amount;diagnostics.operation="original_native_damage_curve_calculation";
    if(!original_damage_tuning::Calculate(shot,nearestHit,amount))return Fail(original_damage_tuning::State().error,original_damage_tuning::State().stage);
    if(!amount.valid||!amount.originalRegionVerified||amount.totalDamage<=0){++diagnostics.sectionRejected;
        diagnostics.lastOutcome="original_damage_tuning_rejected";return true;}
    auto killer=original_scoreboard::Find(sourceState),victim=original_scoreboard::Find(targets[nearest].playerState);
    if(!killer||!victim||killer->round_kills==(std::numeric_limits<int32_t>::max)())
        return Fail(1818,"original_training_original_score_entries_unavailable");
    // Explicit missing-local-authority round counters. The killed player's
    // value is their own round KILLS, never their death count or a guessed index.
    original_damage::DeathData death;death.killsForKiller=killer->round_kills+1;death.killsForVictim=victim->round_kills;
    original_damage::Result result;diagnostics.operation="original_native_damage_response_and_on_death";
    if(!original_damage::ApplyPoint(nearest,sourcePawn,sourceWeapon,amount.damageType,shot,nearestHit,
                                   amount.totalDamage,shot.sequence,result,&death,amount.falloffMultiplier))
        return Fail(original_damage::State().error,original_damage::State().stage);
    diagnostics.lastDamage=amount.totalDamage;diagnostics.lastFalloff=amount.falloffMultiplier;
    diagnostics.lastNativeFalloffFeature=amount.nativeFalloffFeature;diagnostics.lastRegion=amount.region;
    diagnostics.lastOriginalResponse=result.originalResponse;diagnostics.lastOriginalLifeReadback=result.originalLifeReadback;
    diagnostics.lastOriginalDeathMontage=result.originalDeathMontage;
    if(result.duplicate){++diagnostics.duplicates;diagnostics.lastOutcome="duplicate_native_damage_token";return true;}
    if(!result.applied){++diagnostics.sectionRejected;diagnostics.lastOutcome="original_damage_section_rejected";return true;}
    auto& t=targets[nearest];++diagnostics.damageEvents;++t.damageEvents;t.lastDamageSequence=shot.sequence;
    t.health=result.healthAfter;diagnostics.lastOutcome="original_native_damage_applied";
    if(result.killed){
        if(!MarkDead(nearest,now,true))return false;
        t.lastKillSequence=shot.sequence;++t.kills;++diagnostics.kills;diagnostics.lastKillSequence=shot.sequence;
        // The native damage/death path already broadcasts its genuine response.
        // Default -1/-1 keeps this aggregate/Champion update from broadcasting twice.
        if(!original_scoreboard::RecordKill(shot.sequence,sourceState,t.playerState,result.response,sourceWeapon))
            return Fail(original_scoreboard::State().error,original_scoreboard::State().stage);
        ++diagnostics.scoreUpdates;diagnostics.lastOutcome="original_death_and_score_confirmed";
    }
    diagnostics.operation="none";return true;
}
inline bool InitializeImpl(void* originalWorld){
    if(diagnostics.restartRequired)return false;
    if(diagnostics.initialized)return world==originalWorld&&Thread();
    if(!original_players::Thread()||originalWorld!=original_players::world)
        return Fail(1819,"original_training_original_world_or_thread_required");
    if(!std::isfinite(options.maximumDistance)||options.maximumDistance<=0||options.maximumDistance>1000000
       ||!std::isfinite(options.maximumShotAge)||options.maximumShotAge<=0||options.maximumShotAge>1
       ||!std::isfinite(options.respawnDelay)||options.respawnDelay<0
       ||!std::isfinite(options.sampleInterval)||options.sampleInterval<=0||options.sampleInterval>1
       ||!std::isfinite(options.obstructionTolerance)||options.obstructionTolerance<0||options.obstructionTolerance>5
       ||!std::isfinite(options.oldGunLifetime)||options.oldGunLifetime<=0
       ||!options.maximumShotsPerPoll||options.maximumShotsPerPoll>QueueCapacity)
        return Fail(1826,"original_training_invalid_local_policy_options");
    world=originalWorld;gameThread=GetCurrentThreadId();sourcePawn=original_players::activePawn;
    sourceState=Read<void*>(sourcePawn,0x488);sourceWeapon=original_weapon::weapon;sourceFiring=original_weapon::firing;
    if(!original_players::IsA(sourcePawn,original_players::ShooterCharacterClass)
       ||!original_players::IsA(sourceWeapon,0x645c660)||Read<void*>(sourceWeapon,0x4a8)!=sourcePawn
       ||!original_shots::State().initialized||original_shots::firing!=sourceFiring)
        return Fail(1820,"original_training_requires_original_local_ak_and_shot_observer");
    if(original_shots::sink&&original_shots::sink!=QueueShot)return Fail(1821,"original_training_original_shot_sink_already_owned");
    if(!original_damage::Initialize(world))return Fail(original_damage::State().error,original_damage::State().stage);
    if(!original_damage_tuning::Initialize(sourceFiring))return Fail(original_damage_tuning::State().error,original_damage_tuning::State().stage);
    if(!original_scoreboard::Initialize(world)||!original_scoreboard::RegisterPlayer(sourceState))
        return Fail(original_scoreboard::State().error,original_scoreboard::State().stage);
    randomState=static_cast<uint32_t>(GetTickCount64())^0x6d2b79f5U;if(!randomState)randomState=1;
    const double now=static_cast<double>(GetTickCount64())/1000.;
    for(size_t i=0;i<targets.size();++i){
        if(static_cast<int>(i)==original_players::localIndex||!original_enemies::enemies[i].ready)continue;
        if(!BindTarget(i,now,false))return false;
    }
    if(!diagnostics.managedTargets)return Fail(1822,"original_training_original_enemies_required");
    diagnostics.initialized=true;diagnostics.error=0;diagnostics.stage="original_training_waiting_for_original_body_physics";
    original_shots::sink=QueueShot;return true;
}
inline bool Initialize(void* originalWorld){
    __try{return InitializeImpl(originalWorld);}__except(CaptureException(GetExceptionInformation())){return false;}
}
inline bool PollImpl(double now){
    if(!Thread()||!std::isfinite(now)||now<0||now+.05<diagnostics.lastPollAt)
        return Fail(1823,"original_training_invalid_thread_or_monotonic_seconds");
    if(original_players::activePawn!=sourcePawn||Read<void*>(sourcePawn,0x488)!=sourceState
       ||original_weapon::weapon!=sourceWeapon||original_weapon::firing!=sourceFiring||original_shots::firing!=sourceFiring)
        return Fail(1824,"original_training_original_local_source_changed");
    diagnostics.lastPollAt=now;
    for(size_t i=0;i<targets.size();++i)if(targets[i].bound){
        if(!RefreshTarget(i,now)||!Respawn(i,now))return false;
    }
    // A fixed end prevents a native callback produced while applying damage
    // from recursively consuming newly queued work in this same Poll.
    const auto end=writeIndex;size_t budget=options.maximumShotsPerPoll;
    while(readIndex<end&&budget--){const Pending pending=queue[readIndex++%queue.size()];if(!Process(pending,now))return false;}
    diagnostics.aliveTargets=diagnostics.physicsReadyTargets=diagnostics.equippedTargets=0;
    for(const auto& t:targets)if(t.bound){if(t.alive)++diagnostics.aliveTargets;
        if(t.alive&&t.physicsReady)++diagnostics.physicsReadyTargets;if(t.alive&&t.weaponReady)++diagnostics.equippedTargets;}
    diagnostics.ready=diagnostics.physicsReadyTargets>0;diagnostics.error=0;
    diagnostics.stage=diagnostics.ready?"original_training_native_combat_active":"original_training_waiting_original_physics_or_respawn";
    diagnostics.operation="none";return true;
}
inline bool Poll(double monotonicSeconds){
    if(diagnostics.restartRequired)return false;
    if(diagnostics.processing)return Fail(1825,"original_training_reentrant_poll_rejected");
    diagnostics.processing=true;bool result=false;
    __try{result=PollImpl(monotonicSeconds);}__except(CaptureException(GetExceptionInformation())){result=false;}
    diagnostics.processing=false;return result;
}
inline bool ShouldUpdateEnemy(size_t index){
    if(!Thread()||index>=targets.size()||diagnostics.testStationary)return false;
    const auto& t=targets[index];return t.bound&&t.alive&&t.physicsReady&&t.weaponReady&&t.phase==Phase::Alive;
}
inline bool SetTestStationary(bool stationary){
    if(!Thread())return false;if(diagnostics.testStationary==stationary)return true;
    diagnostics.testStationary=stationary;
    for(size_t i=0;i<targets.size();++i){auto& t=targets[i];if(!t.bound||!t.alive)continue;
        if(stationary){t.inputsStopped=false;if(!StopInput(i))return false;}else t.inputsStopped=false;}
    return true;
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    const auto b=[](bool v){return v?"true":"false";};const auto& d=diagnostics;
    std::fputs("{\"schema_version\":1,\"stage\":",f);original_players::Text(f,d.stage);
    std::fprintf(f,",\"initialized\":%s,\"ready\":%s,\"restart_required\":%s,\"test_stationary\":%s,\"error\":%u,\"exception_rva\":%llu,\"operation\":",
        b(d.initialized),b(d.ready),b(d.restartRequired),b(d.testStationary),d.error,static_cast<unsigned long long>(d.exceptionRva));
    original_players::Text(f,d.operation);std::fputs(",\"last_outcome\":",f);original_players::Text(f,d.lastOutcome);
#define TRAIN_COUNT(field) std::fprintf(f,",\"" #field "\":%llu",static_cast<unsigned long long>(d.field))
    TRAIN_COUNT(managedTargets);TRAIN_COUNT(aliveTargets);TRAIN_COUNT(physicsReadyTargets);TRAIN_COUNT(equippedTargets);
    TRAIN_COUNT(callbacks);TRAIN_COUNT(queued);TRAIN_COUNT(processed);TRAIN_COUNT(unpairedRejected);TRAIN_COUNT(invalidRejected);
    TRAIN_COUNT(duplicates);TRAIN_COUNT(staleRejected);TRAIN_COUNT(queueOverflow);TRAIN_COUNT(emptyEligibility);
    TRAIN_COUNT(traceQueries);TRAIN_COUNT(traceHits);TRAIN_COUNT(misses);TRAIN_COUNT(worldQueries);TRAIN_COUNT(sceneOnlyQueries);
    TRAIN_COUNT(worldBlocked);TRAIN_COUNT(physicsDeferred);TRAIN_COUNT(pendingDamageDeferred);TRAIN_COUNT(damageEvents);
    TRAIN_COUNT(sectionRejected);TRAIN_COUNT(kills);TRAIN_COUNT(externalDeaths);TRAIN_COUNT(scoreUpdates);TRAIN_COUNT(respawns);
    TRAIN_COUNT(oldGunTimers);TRAIN_COUNT(oldGunAlreadyTimed);TRAIN_COUNT(oldGunAlreadyGone);TRAIN_COUNT(oldGunTransferred);
    TRAIN_COUNT(lastQueuedSequence);TRAIN_COUNT(lastProcessedSequence);TRAIN_COUNT(lastKillSequence);
#undef TRAIN_COUNT
    std::fprintf(f,",\"pending_shots\":%llu,\"last_poll_at\":%.9g,\"last_target\":%d,\"last_body_distance\":%.9g,\"last_world_distance\":%.9g,\"last_damage\":%.9g,\"last_falloff\":%.9g,\"last_region\":%u,\"last_native_falloff_feature\":%s,\"last_original_response\":%s,\"last_original_life_readback\":%s,\"last_original_death_montage\":%s,\"targets\":[",
        static_cast<unsigned long long>(writeIndex-readIndex),d.lastPollAt,d.lastTarget,d.lastBodyDistance,d.lastWorldDistance,
        d.lastDamage,d.lastFalloff,static_cast<unsigned>(d.lastRegion),b(d.lastNativeFalloffFeature),b(d.lastOriginalResponse),b(d.lastOriginalLifeReadback),b(d.lastOriginalDeathMontage));
    bool comma=false;
    for(size_t i=0;i<targets.size();++i){const auto& t=targets[i];if(!t.bound)continue;
        std::fprintf(f,"%s{\"roster_index\":%zu,\"phase\":\"%s\",\"alive\":%s,\"health\":%.9g,\"original_physics_ready\":%s,\"body_count\":%d,\"physics_actors\":%d,\"weapon_ready\":%s,\"respawn_at\":%.9g,\"logical_id\":%u,\"respawns\":%u,\"damage_events\":%u,\"kills\":%u,\"physics_wait_samples\":%u,\"accept_after_sequence\":%llu,\"last_damage_sequence\":%llu,\"last_kill_sequence\":%llu,\"pawn\":\"%p\",\"player_state\":\"%p\",\"weapon\":\"%p\",\"last_corpse\":\"%p\",\"last_old_gun\":\"%p\"}",
            comma?",":"",i,PhaseName(t.phase),b(t.alive),t.health,b(t.physicsReady),t.bodyCount,t.physicsActors,b(t.weaponReady),
            t.respawnAt,t.logicalId,t.respawns,t.damageEvents,t.kills,t.physicsWaitSamples,static_cast<unsigned long long>(t.acceptAfterSequence),
            static_cast<unsigned long long>(t.lastDamageSequence),static_cast<unsigned long long>(t.lastKillSequence),
            t.pawn,t.playerState,t.weapon,t.lastCorpse,t.lastOldGun);comma=true;
    }
    std::fputs("],\"requires_exact_native_origin_flag\":8,\"original_body_physics_required\":true,\"proxy_hitboxes\":false,\"wall_penetration\":false,\"health_or_alive_written_directly\":false,\"original_server_damage_claimed\":false,\"runtime_verified\":false}\n",f);
    const bool okay=std::ferror(f)==0;std::fclose(f);return okay;
}
}
