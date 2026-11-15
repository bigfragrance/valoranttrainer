#pragma once
#include "original_enemies.h"

// The client has no authority character factory. Recreate only that missing
// local factory after three original health/alive readbacks establish death.
// Keep the real PC/PS/native roster/OEPI/combat aggregates; construct a fresh
// shipped hero, then run the original possession and replication notifications.
// No health/alive flag, movement tuning, animation pose or material is copied.
namespace original_enemy_respawn {
struct Options {float fallbackCorpseLifetime=10.0f;};
inline Options options;
struct Life {
    bool queried=false,alive=false,handlerAlive=false,consistent=false;
    float health=0;
    void* handler=nullptr;
};
struct Result {
    bool attempted=false,ready=false,deathVerified=false,newLifeVerified=false;
    bool sameController=false,sameState=false,sameInfo=false,sameStats=false,sameRoster=false;
    bool localPlayerPreserved=false,associationsPublished=false,heroSelectionPublished=false;
    bool corpseCleanupRequested=false,corpseAlreadyTimed=false,corpseMeshUnchanged=false;
    size_t rosterIndex=0;
    uint32_t previousLogicalId=0,logicalId=0,previousHero=0,hero=0;
    void *oldPawn=nullptr,*newPawn=nullptr,*controller=nullptr,*playerState=nullptr;
    void *oldInventory=nullptr,*newInventory=nullptr,*playerInfo=nullptr,*stats=nullptr;
    void *corpseMeshAssetBefore=nullptr,*corpseMeshAssetAfter=nullptr;
    int32_t rosterCountBefore=0,rosterCountAfter=0;
    int32_t killsBefore=0,killsAfter=0,deathsBefore=0,deathsAfter=0,assistsBefore=0,assistsAfter=0;
    Life previousLife,newLife;
    float corpseOriginalLifetime=0,corpseRemainingBefore=0,corpseRemainingAfter=0;
};
struct Diagnostics {
    unsigned error=0,requests=0,completed=0,deniedLiving=0,corpseTimers=0;
    bool restartRequired=false;
    uintptr_t exceptionRva=0;
    const char* stage="original_enemy_respawn_not_requested";
    const char* operation="none";
};
inline Diagnostics diagnostics;
inline std::array<Result,original_players::MaxPlayers> results{};
inline const Diagnostics& State(){return diagnostics;}
template<class T> inline T Value(const void* p,size_t offset){return original_players::Value<T>(p,offset);}
inline bool Fail(unsigned code,const char* text){diagnostics.error=code;diagnostics.stage=text;return false;}
inline bool Thread(){return !diagnostics.restartRequired&&original_enemies::Thread();}
inline LONG Capture(EXCEPTION_POINTERS* exception){
    diagnostics.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0001300U;
    const auto at=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    diagnostics.exceptionRva=at>=engine::image?at-engine::image:at;
    diagnostics.restartRequired=true;diagnostics.stage="original_enemy_respawn_exception_requires_new_session";
    return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Event(void* object,const wchar_t* name,uint16_t size,void* parameter){
    return original_enemies::Event(object,name,size,parameter);
}
inline bool ReadLife(void* pawn,Life& result){
    result={};
    if(!Thread()||!original_players::IsA(pawn,original_players::ShooterCharacterClass))
        return Fail(1301,"respawn_original_character_required");
    result.handler=Value<void*>(pawn,0xc70);
    if(!original_players::IsA(result.handler,0x6702670))return Fail(1302,"respawn_original_damageable_missing");
    uint8_t alive=0;
    if(!Event(pawn,L"/Script/ShooterGame.ShooterCharacter.IsAlive",1,&alive)
       ||!Event(pawn,L"/Script/ShooterGame.ShooterCharacter.GetHealth",4,&result.health)
       ||!std::isfinite(result.health))return Fail(1303,"respawn_original_life_query_failed");
    result.alive=alive!=0;result.handlerAlive=Value<uint8_t>(result.handler,0x1f9)!=0;
    result.consistent=result.alive==result.handlerAlive;result.queried=true;
    return result.consistent?true:Fail(1304,"respawn_original_alive_readbacks_disagree");
}
inline bool Dead(const Life& life){return life.queried&&life.consistent&&!life.alive&&!life.handlerAlive&&life.health<=.001f;}
inline bool Living(const Life& life){return life.queried&&life.consistent&&life.alive&&life.handlerAlive&&life.health>.001f;}
inline int32_t RosterCount(){return Value<int32_t>(Value<void*>(original_players::world,0x178),0x488);}
inline bool ReplaceAssociations(original_players::PlayerEntry& entry,void* oldPawn){
    auto previousSpawn=Value<void*>(entry.state,0xbc8);
    auto previousPossessed=Value<void*>(entry.state,0xbd0);
    if((previousSpawn&&previousSpawn!=oldPawn&&previousSpawn!=entry.character)
       ||(previousPossessed&&previousPossessed!=oldPawn&&previousPossessed!=entry.character))
        return Fail(1305,"respawn_original_association_belongs_to_other_character");
    // The expected old->new replacement is deliberately separate from the
    // generic BindRosterPlayer guard, which continues to reject occupied slots.
    if(previousSpawn!=entry.character){
        diagnostics.operation="respawn_original_spawned_character_notification";
        if(!original_players::Write(entry.state,0xbc8,entry.character)
           ||!Event(entry.state,L"/Script/ShooterGame.ShooterPlayerState.OnRep_SpawnedCharacter",8,&previousSpawn))
            return Fail(1306,"respawn_original_spawn_notification_failed");
    }
    if(previousPossessed!=entry.character){
        diagnostics.operation="respawn_original_possessed_character_notification";
        if(!original_players::Write(entry.state,0xbd0,entry.character)
           ||!Event(entry.state,L"/Script/ShooterGame.ShooterPlayerState.OnRep_PossessedCharacter",8,&previousPossessed))
            return Fail(1307,"respawn_original_possession_notification_failed");
    }
    void *spawned=nullptr,*possessed=nullptr;
    if(!Event(entry.state,L"/Script/ShooterGame.ShooterPlayerState.GetSpawnedCharacter",8,&spawned)
       ||!Event(entry.state,L"/Script/ShooterGame.ShooterPlayerState.GetPossessedCharacter",8,&possessed)
       ||spawned!=entry.character||possessed!=entry.character)
        return Fail(1308,"respawn_original_association_readback_mismatch");
    return true;
}
inline bool ScheduleCorpseCleanup(Result& result){
    diagnostics.operation="respawn_original_corpse_lifespan_query";
    if(!Event(result.oldPawn,L"/Script/Engine.Actor.GetLifeSpan",4,&result.corpseRemainingBefore)
       ||!std::isfinite(result.corpseRemainingBefore))return Fail(1309,"respawn_original_corpse_lifespan_query_failed");
    const float originalLifetime=Value<float>(result.oldPawn,0x1528);
    result.corpseOriginalLifetime=std::isfinite(originalLifetime)?originalLifetime:0;
    if(result.corpseRemainingBefore>0){
        // Preserve a timer already selected by the original death/finisher path.
        result.corpseAlreadyTimed=true;result.corpseRemainingAfter=result.corpseRemainingBefore;
        result.corpseCleanupRequested=true;return true;
    }
    float delay=result.corpseOriginalLifetime;
    if(!std::isfinite(delay)||delay<=0)delay=options.fallbackCorpseLifetime;
    if(!std::isfinite(delay)||delay<=0)return Fail(1310,"respawn_invalid_corpse_cleanup_duration");
    diagnostics.operation="respawn_original_set_corpse_lifespan";
    if(!Event(result.oldPawn,L"/Script/Engine.Actor.SetLifeSpan",4,&delay)
       ||!Event(result.oldPawn,L"/Script/Engine.Actor.GetLifeSpan",4,&result.corpseRemainingAfter)
       ||!std::isfinite(result.corpseRemainingAfter)||result.corpseRemainingAfter<=0)
        return Fail(1311,"respawn_original_corpse_cleanup_timer_not_observed");
    // The engine timer owns its actor handle/lifetime and invokes normal native
    // LifeSpanExpired/Destroy. No raw-pointer corpse queue is polled after GC.
    result.corpseCleanupRequested=true;++diagnostics.corpseTimers;return true;
}
inline bool RecreateImpl(size_t index,uint32_t logicalId,const engine::Vec& feet,double yaw,Result& output){
    output={};
    if(!Thread()||index>=results.size()||!logicalId||!original_players::Finite(feet)||!std::isfinite(yaw))
        return Fail(1312,"respawn_invalid_request");
    if(static_cast<int>(index)==original_players::localIndex)return Fail(1313,"respawn_cannot_replace_local_player");
    auto& entry=original_players::players[index];auto& enemy=original_enemies::enemies[index];
    if(!entry.ready||!entry.registered||!entry.characterInitialized||!enemy.ready
       ||entry.character!=enemy.pawn||!original_players::Registered(entry.state))
        return Fail(1314,"respawn_original_enemy_roster_not_ready");
    auto& result=results[index];result={};result.attempted=true;result.rosterIndex=index;
    ++diagnostics.requests;diagnostics.operation="respawn_verify_original_death";
    result.oldPawn=entry.character;result.controller=entry.controller;result.playerState=entry.state;
    result.oldInventory=entry.inventory;result.playerInfo=Value<void*>(entry.state,0xa18);result.stats=Value<void*>(entry.state,0x898);
    result.previousLogicalId=enemy.logicalId;result.logicalId=logicalId;result.previousHero=enemy.heroIndex;result.hero=original_enemies::HeroFor(logicalId);
    if(!ReadLife(result.oldPawn,result.previousLife))return false;
    result.deathVerified=Dead(result.previousLife);
    if(!result.deathVerified){++diagnostics.deniedLiving;output=result;return Fail(1315,"respawn_requires_actual_original_death");}
    auto currentPawn=Value<void*>(result.controller,0x4b0);
    if(currentPawn&&currentPawn!=result.oldPawn)return Fail(1316,"respawn_controller_owns_other_character");
    if(Value<void*>(result.controller,0x470)!=result.playerState
       ||Value<void*>(result.playerState,0x210)!=result.controller)return Fail(1317,"respawn_original_controller_state_pair_changed");
    result.rosterCountBefore=RosterCount();
    if(result.rosterCountBefore<1||result.rosterCountBefore>4096||!original_players::IsA(result.stats,0x65b0cd0))
        return Fail(1318,"respawn_original_roster_or_stats_missing");
    diagnostics.operation="respawn_capture_original_corpse_mesh_asset";
    auto corpseCosmetic=Value<void*>(result.oldPawn,0xf40);
    if(!original_enemies::ReadAsset(corpseCosmetic,result.corpseMeshAssetBefore))
        return Fail(1326,"respawn_original_corpse_asset_query_failed");
    result.killsBefore=Value<int32_t>(result.stats,0xe4);result.deathsBefore=Value<int32_t>(result.stats,0xe8);result.assistsBefore=Value<int32_t>(result.stats,0xec);
    const auto activePawn=original_players::activePawn,activeController=original_players::activeController;
    const int localIndex=original_players::localIndex;
    original_players::CharacterDefaults selected;
    diagnostics.operation="respawn_load_original_hero_defaults";
    if(!original_players::ProbeDefaults(original_enemies::HeroPaths[result.hero],selected)||selected.abstractClass
       ||!selected.inventory||!selected.abilitiesAndBuffs||!selected.mesh1P.object||!selected.inputState)
        return Fail(1319,"respawn_original_hero_defaults_missing");
    engine::Transform transform;
    transform.translation={feet.x,feet.y,feet.z+selected.capsuleHalfHeight+2};
    constexpr double radians=3.14159265358979323846/180.0;
    transform.rotation[2]=std::sin(yaw*radians*.5);transform.rotation[3]=std::cos(yaw*radians*.5);
    diagnostics.operation="respawn_begin_original_hero";
    result.newPawn=original_players::Begin(selected.type,transform,result.controller);
    if(!original_players::IsA(result.newPawn,original_players::ShooterCharacterClass)||result.newPawn==result.oldPawn)
        return Fail(1320,"respawn_original_hero_spawn_failed");
    if(!original_players::Write(result.newPawn,0x488,result.playerState))return Fail(1321,"respawn_original_pawn_state_preassociation_failed");
    diagnostics.operation="respawn_finish_original_hero_scs_beginplay";
    original_players::Finish(result.newPawn,transform);
    result.newInventory=Value<void*>(result.newPawn,0xc08);
    const auto mesh1P=Value<void*>(result.newPawn,0xf30),abilities=Value<void*>(result.newPawn,0xa20);
    const auto attributes=Value<void*>(abilities,0x13a8),movement=Value<void*>(result.newPawn,0x4f8);
    if(!Value<uint8_t>(result.newPawn,0xa90)||!result.newInventory||!mesh1P||!abilities||!attributes
       ||!original_players::IsA(movement,0x6c7d380)||!Value<void*>(result.newPawn,0xba0))
        return Fail(1322,"respawn_original_hero_initialization_incomplete");
    diagnostics.operation="respawn_verify_fresh_original_life";
    if(!ReadLife(result.newPawn,result.newLife))return false;
    result.newLifeVerified=Living(result.newLife);
    if(!result.newLifeVerified)return Fail(1323,"respawn_fresh_original_hero_not_alive");
    diagnostics.operation="respawn_original_controller_possess_new_hero";
    if(!Event(result.controller,L"/Script/Engine.Controller.Possess",8,&result.newPawn)
       ||Value<void*>(result.controller,0x4b0)!=result.newPawn
       ||Value<void*>(result.newPawn,0x4a0)!=result.controller
       ||Value<void*>(result.newPawn,0x488)!=result.playerState)
        return Fail(1324,"respawn_original_possession_readback_failed");
    // Commit only the actual new native instances to the existing roster slot.
    // No new PlayerState, controller, scoreboard info or PlayerArray entry exists.
    entry.character=result.newPawn;entry.inventory=result.newInventory;entry.mesh1P=mesh1P;
    entry.abilities=abilities;entry.attributes=attributes;entry.movement=movement;
    entry.characterInitialized=true;entry.enabled=true;entry.ready=true;entry.feetPosition=feet;entry.yaw=yaw;
    wcscpy_s(entry.classPath,original_enemies::HeroPaths[result.hero]);
    entry.stateText="original_respawn_character_created";
    diagnostics.operation="respawn_original_old_to_new_player_state_notifications";
    if(!ReplaceAssociations(entry,result.oldPawn))return false;
    result.associationsPublished=true;
    // Disconnect the old pawn's PS notifications before changing DesiredClass,
    // so a different requested hero cannot re-skin the original old corpse.
    enemy={};enemy.attempted=true;enemy.logicalId=logicalId;enemy.heroIndex=result.hero;
    enemy.anchor=feet;enemy.yaw=yaw;enemy.pawn=result.newPawn;enemy.controller=result.controller;enemy.playerState=result.playerState;
    enemy.spawnPublished=enemy.possessionPublished=true;
    if(!original_enemies::PublishSelection(index))return Fail(original_enemies::error,original_enemies::stage);
    result.heroSelectionPublished=true;
    if(!original_enemies::VerifyEnemy(enemy)
       ||!original_enemies::ApplyRosterNameImpl(index,entry.requestedName)
       ||!original_enemies::SampleImpl(index,static_cast<double>(GetTickCount64())/1000.0)
       ||!original_enemies::ConfigureOutline(enemy))return Fail(original_enemies::error,original_enemies::stage);
    enemy.originalCapsuleHalfHeight=selected.capsuleHalfHeight;enemy.originalCapsuleRadius=selected.capsuleRadius;
    diagnostics.operation="respawn_verify_original_corpse_kept_its_hero_asset";
    if(!original_enemies::ReadAsset(corpseCosmetic,result.corpseMeshAssetAfter))return Fail(1326,"respawn_original_corpse_asset_query_failed");
    result.corpseMeshUnchanged=result.corpseMeshAssetAfter==result.corpseMeshAssetBefore;
    result.sameController=entry.controller==result.controller&&Value<void*>(entry.state,0x210)==result.controller;
    result.sameState=entry.state==result.playerState&&Value<void*>(entry.controller,0x470)==result.playerState;
    result.sameInfo=Value<void*>(entry.state,0xa18)==result.playerInfo;
    result.killsAfter=Value<int32_t>(result.stats,0xe4);result.deathsAfter=Value<int32_t>(result.stats,0xe8);result.assistsAfter=Value<int32_t>(result.stats,0xec);
    result.sameStats=Value<void*>(entry.state,0x898)==result.stats&&result.killsAfter==result.killsBefore
        &&result.deathsAfter==result.deathsBefore&&result.assistsAfter==result.assistsBefore;
    result.rosterCountAfter=RosterCount();
    result.sameRoster=result.rosterCountAfter==result.rosterCountBefore&&original_players::Registered(result.playerState);
    result.localPlayerPreserved=original_players::activePawn==activePawn&&original_players::activeController==activeController
        &&original_players::localIndex==localIndex&&original_players::localPlayer
        &&Value<void*>(original_players::localPlayer,0x38)==activeController;
    if(!result.sameController||!result.sameState||!result.sameInfo||!result.sameStats||!result.sameRoster||!result.localPlayerPreserved||!result.corpseMeshUnchanged)
        return Fail(1325,"respawn_original_roster_or_local_player_invariant_changed");
    if(!ScheduleCorpseCleanup(result))return false;
    enemy.ready=enemy.enabled=true;entry.stateText="original_respawn_ready_for_equipment_and_damage_rebind";
    result.ready=true;output=result;++diagnostics.completed;diagnostics.error=0;
    diagnostics.stage="original_enemy_respawn_ready_for_equipment_and_damage_rebind";diagnostics.operation="none";return true;
}
inline bool Recreate(size_t index,uint32_t logicalId,const engine::Vec& feet,double yaw,Result& result){
    __try{return RecreateImpl(index,logicalId,feet,yaw,result);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    const auto b=[](bool value){return value?"true":"false";};
    std::fputs("{\"schema_version\":1,\"stage\":",f);original_players::Text(f,diagnostics.stage);
    std::fprintf(f,",\"error\":%u,\"requests\":%u,\"completed\":%u,\"denied_living\":%u,\"corpse_timers_requested\":%u,\"restart_required\":%s,\"exception_rva\":%llu,\"operation\":",diagnostics.error,diagnostics.requests,diagnostics.completed,diagnostics.deniedLiving,diagnostics.corpseTimers,b(diagnostics.restartRequired),static_cast<unsigned long long>(diagnostics.exceptionRva));original_players::Text(f,diagnostics.operation);
    std::fputs(",\"respawns\":[",f);bool first=true;
    for(const auto& r:results){
        if(!r.attempted)continue;if(!first)std::fputc(',',f);first=false;
        std::fprintf(f,"{\"roster_index\":%zu,\"ready\":%s,\"death_verified\":%s,\"fresh_life_verified\":%s,\"same_controller\":%s,\"same_player_state\":%s,\"same_player_info\":%s,\"same_aggregate_stats\":%s,\"same_native_roster\":%s,\"local_player_preserved\":%s,\"associations_published\":%s,\"hero_selection_published\":%s,\"corpse_cleanup_requested\":%s,\"corpse_already_timed\":%s,\"corpse_mesh_unchanged\":%s",r.rosterIndex,b(r.ready),b(r.deathVerified),b(r.newLifeVerified),b(r.sameController),b(r.sameState),b(r.sameInfo),b(r.sameStats),b(r.sameRoster),b(r.localPlayerPreserved),b(r.associationsPublished),b(r.heroSelectionPublished),b(r.corpseCleanupRequested),b(r.corpseAlreadyTimed),b(r.corpseMeshUnchanged));
        std::fprintf(f,",\"old_logical_id\":%u,\"logical_id\":%u,\"old_hero_index\":%u,\"hero_index\":%u,\"roster_count_before\":%d,\"roster_count_after\":%d,\"kills_before\":%d,\"kills_after\":%d,\"deaths_before\":%d,\"deaths_after\":%d,\"assists_before\":%d,\"assists_after\":%d",r.previousLogicalId,r.logicalId,r.previousHero,r.hero,r.rosterCountBefore,r.rosterCountAfter,r.killsBefore,r.killsAfter,r.deathsBefore,r.deathsAfter,r.assistsBefore,r.assistsAfter);
        std::fprintf(f,",\"old_health\":%.9g,\"old_alive\":%s,\"old_handler_alive\":%s,\"new_health\":%.9g,\"new_alive\":%s,\"new_handler_alive\":%s,\"corpse_original_lifetime\":%.9g,\"corpse_remaining_before\":%.9g,\"corpse_remaining_after\":%.9g",r.previousLife.health,b(r.previousLife.alive),b(r.previousLife.handlerAlive),r.newLife.health,b(r.newLife.alive),b(r.newLife.handlerAlive),r.corpseOriginalLifetime,r.corpseRemainingBefore,r.corpseRemainingAfter);
        std::fprintf(f,",\"old_pawn\":\"%p\",\"new_pawn\":\"%p\",\"controller\":\"%p\",\"player_state\":\"%p\",\"old_inventory\":\"%p\",\"new_inventory\":\"%p\",\"player_info\":\"%p\",\"stats\":\"%p\",\"corpse_mesh_asset_before\":\"%p\",\"corpse_mesh_asset_after\":\"%p\"}",r.oldPawn,r.newPawn,r.controller,r.playerState,r.oldInventory,r.newInventory,r.playerInfo,r.stats,r.corpseMeshAssetBefore,r.corpseMeshAssetAfter);
    }
    std::fputs("],\"caller_must_rebind_original_damage_and_equipment\":true,\"health_or_alive_state_written\":false,\"runtime_verified\":false}\n",f);
    const bool ok=std::ferror(f)==0;std::fclose(f);return ok;
}
}
