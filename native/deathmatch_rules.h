#pragma once
#include "original_damage.h"
#include "original_scoreboard.h"
#include "original_enemy_respawn.h"
#include "original_inventory_slots.h"
#include "original_effects.h"
#include "original_hud.h"
#include "original_timed_streak.h"
#include "original_streak_effect_authority.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <limits>
#include <cstdint>
#include <climits>

// Missing local match authority only. Native shots, tuning, damage responses,
// health replication, death reactions, construction and possession remain the
// shipped implementations. This module neither makes rays nor patches images.
namespace deathmatch_rules {
inline constexpr size_t MaxPlayers=original_players::MaxPlayers;
inline constexpr const wchar_t* StateClassPath=L"/Game/GameModes/Deathmatch/DeathmatchPlayerState.DeathmatchPlayerState_C";
inline constexpr const wchar_t* GameStateClassPath=L"/Game/GameModes/Deathmatch/DeathmatchGameState.DeathmatchGameState_C";
inline constexpr const wchar_t* ControllerClassPath=L"/Game/GameModes/Deathmatch/DeathmatchController.DeathmatchController_C";
inline constexpr const wchar_t* PlayerInfoClassPath=L"/Game/GameModes/Deathmatch/DeathmatchPlayerInfo.DeathmatchPlayerInfo_C";
inline constexpr const wchar_t* FFABaseStatePath=L"/Game/GameModes/Common/FFAPlayerState.FFAPlayerState_C";
inline constexpr uintptr_t FFATeamClass=0x6849cb0,BaseTeamClass=0x65a0b40;
inline constexpr const wchar_t* TimedStreakClass=L"/Game/GameModes/Components/Comp_TimedKillStreak.Comp_TimedKillStreak_C";
inline constexpr const wchar_t* AnnouncementClass=L"/Game/UI/InGame/KillBanner/CommonAssets/KillBannerAnnouncement.KillBannerAnnouncement_C";
inline constexpr const wchar_t* AnnouncementStop=L"/Game/UI/InGame/KillBanner/CommonAssets/KillBannerAnnouncement.KillBannerAnnouncement_C.OnPlayerStateChanged";
inline constexpr const wchar_t* ManualBannerMethod=L"/Game/UI/InGame/KillBanner/CommonAssets/KillBannerAnnouncement.KillBannerAnnouncement_C.Manual Killbanner Configuration";
enum class Mode:uint8_t {Practice,FreeForAll};
enum class Phase:uint8_t {Absent,Alive,DeadWaiting,Rebuilding};
enum class EndReason:uint8_t {None,KillLimit,TimeLimit};
enum class PositionBasis:uint8_t {Feet,OriginalPawnRoot};
struct Config {
    Mode mode=Mode::Practice;
    uint32_t killLimit=40;
    double durationSeconds=540,respawnDelay=1.5,streakTimeout=5,sampleInterval=.04;
    bool botsReturnFire=false,localInvincible=false,enableRespawn=true,requireOriginalFFA=true;
};
struct SpawnSelection {
    engine::Vec position{};double yaw=0;
    PositionBasis basis=PositionBasis::Feet;
    bool originalMapAnchor=false;
};
struct Hooks {
    bool (*selectSpawn)(size_t index,uint32_t epoch,SpawnSelection& selection)=nullptr;
    bool (*beforeRespawn)(size_t index,void* oldPawn,bool local)=nullptr;
    bool (*afterSpawn)(size_t index,void* oldPawn,void* newPawn,bool local)=nullptr;
    void (*matchEnded)(int32_t winner,EndReason reason)=nullptr;
    void (*afterScoreCommit)(uint64_t token,size_t source,size_t victim)=nullptr;
    bool (*canDamage)(size_t source,size_t victim)=nullptr;
};
struct PlayerSnapshot {
    bool registered=false,local=false,alive=false,ready=false;
    Phase phase=Phase::Absent;
    void *pawn=nullptr,*controller=nullptr,*playerState=nullptr;
    original_inventory_slots::WeakIdentity identity{};
    uint32_t spawnEpoch=0,matchKills=0,matchDeaths=0,streak=0,bestStreak=0;
    int32_t totalKills=0,totalDeaths=0,initialKills=0,initialDeaths=0;
    float health=0,maxHealth=0;
    double lastSample=-1,lastKill=-1,deathAt=-1,respawnAt=-1;
    bool deathAccounted=false,feedbackResetPending=false;
    engine::Vec previousFeet{};double previousYaw=0;
};
struct PendingDamage {
    bool active=false;
    uint64_t token=0;size_t source=0,victim=0;
    void* equippable=nullptr;double now=0;
    uint32_t nextFeedbackCount=0;
};
struct BannerParameters {
    void* bannerClass=nullptr;int32_t killCount=0,maxKills=0;
    uint8_t headshot=0,preview=0,padding18[6]{};
    original_damage::Array<void*> assisters;void* equippable=nullptr;
};
static_assert(sizeof(BannerParameters)==48&&offsetof(BannerParameters,killCount)==8
              &&offsetof(BannerParameters,maxKills)==12&&offsetof(BannerParameters,assisters)==24
              &&offsetof(BannerParameters,equippable)==40);
struct Diagnostics {
    bool initialized=false,ready=false,active=false,ended=false,restartRequired=false,originalFFAConfirmed=false;
    unsigned error=0,participants=0,damageResults=0,kills=0,externalDeaths=0,respawns=0;
    unsigned localDeaths=0,localRespawns=0,duplicateTokens=0,policyRejects=0,unregistered=0,corpseTimersExtended=0;
    unsigned nativeFeedbackSets=0,nativeFeedbackResets=0,feedbackResetFailures=0;
    unsigned timedComponentsReset=0,bannersStopped=0,bannerAbsentAtReset=0,bannerParametersMapped=0;
    bool lastFeedbackResetSucceeded=false;
    int32_t leader=-1,winner=-1;uint32_t leadingKills=0,leadingTies=0;
    EndReason endReason=EndReason::None;
    double startedAt=-1,lastPoll=-1,remaining=0;
    uintptr_t exceptionRva=0;
    const char* stage="deathmatch_not_initialized";
    const char* operation="none";
};
inline Config config;
inline Hooks hooks;
inline Diagnostics diagnostics;
inline std::array<PlayerSnapshot,MaxPlayers> players{};
inline std::array<uint32_t,MaxPlayers> slotGenerations{};
inline std::array<uint64_t,1024> tokens{};
inline size_t tokenCursor=0;
inline PendingDamage pending;
inline void *world=nullptr,*timedClass=nullptr;
inline DWORD gameThread=0;
inline uint32_t randomState=0x6d2b79f5U;
inline const Diagnostics& State(){return diagnostics;}
inline const PlayerSnapshot* Player(size_t index){return index<players.size()?&players[index]:nullptr;}
inline bool Active(){return diagnostics.initialized&&diagnostics.ready&&diagnostics.active&&!diagnostics.restartRequired;}
inline double Remaining(){return diagnostics.remaining;}
inline int32_t Leader(){return diagnostics.leader;}
inline bool AbilitiesAllowed(){return config.mode==Mode::Practice&&Active();}
inline bool BotFireAllowed(){return Active()&&(config.mode==Mode::FreeForAll||config.botsReturnFire);}
template<class T> inline T Read(const void* p,size_t offset){return original_players::Value<T>(p,offset);}
inline bool Fail(unsigned code,const char* stage){diagnostics.error=code;diagnostics.stage=stage;return false;}
inline bool Thread(){return diagnostics.initialized&&!diagnostics.restartRequired&&gameThread==GetCurrentThreadId();}
inline bool UsesOriginalStreak(){return config.mode==Mode::FreeForAll;}
inline bool OriginalStreakFailure(){return Fail(original_timed_streak::State().error,original_timed_streak::State().stage);}
inline bool OriginalStreakEffectFailure(){return Fail(original_streak_effect_authority::State().error,original_streak_effect_authority::State().stage);}
inline bool Event(void* object,const wchar_t* name,uint16_t size,void* parameter){
    return original_damage::Event(object,name,size,parameter);
}
inline bool ValidConfig(const Config& c){
    return (c.mode==Mode::Practice||c.mode==Mode::FreeForAll)&&c.killLimit==40
        &&std::isfinite(c.durationSeconds)&&c.durationSeconds==540
        &&std::isfinite(c.respawnDelay)&&c.respawnDelay>=0&&c.respawnDelay<=15
        &&std::isfinite(c.streakTimeout)&&c.streakTimeout>=.5&&c.streakTimeout<=30
        &&std::isfinite(c.sampleInterval)&&c.sampleInterval>0&&c.sampleInterval<=1;
}
inline bool ReadLife(void* pawn,bool& alive,float& health,float& maximum){
    uint8_t nativeAlive=0;
    auto handler=Read<void*>(pawn,0xc70);
    if(!original_players::IsA(pawn,original_players::ShooterCharacterClass)
       ||!original_players::IsA(handler,original_damage::DamageableClass)
       ||Read<void*>(handler,0xc8)!=pawn
       ||!Event(pawn,L"/Script/ShooterGame.ShooterCharacter.IsAlive",1,&nativeAlive)
       ||!Event(pawn,L"/Script/ShooterGame.ShooterCharacter.GetHealth",4,&health)
       ||!original_damage::ReadMaxHealth(pawn,maximum)
       ||!std::isfinite(health)||!std::isfinite(maximum)||health<0||maximum<=0)
        return Fail(2301,"deathmatch_original_life_query_failed");
    alive=nativeAlive!=0;
    return alive==(Read<uint8_t>(handler,0x1f9)!=0)?true:Fail(2302,"deathmatch_original_alive_readbacks_disagree");
}
inline void RefreshLeader(){
    diagnostics.leader=-1;diagnostics.leadingKills=0;diagnostics.leadingTies=0;
    for(size_t i=0;i<players.size();++i){const auto& p=players[i];if(!p.registered)continue;
        if(diagnostics.leader<0||p.matchKills>diagnostics.leadingKills){
            diagnostics.leader=static_cast<int32_t>(i);diagnostics.leadingKills=p.matchKills;diagnostics.leadingTies=1;
        }else if(p.matchKills==diagnostics.leadingKills)++diagnostics.leadingTies;
    }
}
inline void EndMatch(EndReason reason){
    if(diagnostics.ended)return;
    RefreshLeader();diagnostics.active=false;diagnostics.ended=true;diagnostics.endReason=reason;
    diagnostics.winner=diagnostics.leadingTies==1?diagnostics.leader:-1;
    diagnostics.stage=reason==EndReason::KillLimit?"deathmatch_original_score_reached40":"deathmatch_time540_elapsed";
    if(hooks.matchEnded)hooks.matchEnded(diagnostics.winner,reason);
}
inline bool OriginalFFAAlliances(){
    if(config.mode!=Mode::FreeForAll||!config.requireOriginalFFA)return true;
    void* baseClass=original_players::LoadStateClass(FFABaseStatePath);
    if(!baseClass)return Fail(2303,"deathmatch_original_ffa_player_state_class_missing");
    for(size_t i=0;i<players.size();++i){const auto& p=players[i];if(!p.registered)continue;
        auto team=Read<void*>(p.playerState,0x6a8);
        if(!original_players::Derived(Read<void*>(p.playerState,0x10),baseClass)
           ||!original_players::IsA(team,FFATeamClass)||Read<void*>(team,0xc8)!=p.playerState)
            return Fail(2304,"deathmatch_requires_original_ffa_state_and_team_component");
        for(size_t j=0;j<players.size();++j){if(i==j||!players[j].registered||!p.alive||!players[j].alive)continue;
            struct Alliance {void* source;void* target;uint8_t enemy=0,padding[7]{};} q{p.pawn,players[j].pawn};
            if(!Event(engine::DefaultObject(engine::Class(BaseTeamClass)),L"/Script/ShooterGame.BaseTeamComponent.IsEnemy",17,&q)||!q.enemy)
                return Fail(2305,"deathmatch_original_native_alliance_is_not_free_for_all");
        }
    }
    diagnostics.originalFFAConfirmed=true;return true;
}
inline void* StreakComponent(const PlayerSnapshot& p){
    void* component=nullptr;auto info=Read<void*>(p.playerState,0xa18);
    if(!info||!Event(info,L"/Script/ShooterGame.OwnerExclusivePlayerInfo.GetKillStreakComponentMutable",8,&component)
       ||!original_players::IsA(component,0x6904f00))return nullptr;
    return component;
}
inline bool SetOriginalFeedback(size_t index){
    auto& p=players[index];void* component=StreakComponent(p);int32_t value=static_cast<int32_t>((std::min)(p.streak,uint32_t(INT32_MAX)));
    if(!component||!Event(component,L"/Script/ShooterGame.KillStreakComponent.SetKills",4,&value))
        return Fail(2306,"deathmatch_original_streak_setter_unavailable");
    int32_t observed=-1;
    if(!Event(component,L"/Script/ShooterGame.KillStreakComponent.GetNumKillsStreak",4,&observed)||observed!=value)
        return Fail(2307,"deathmatch_original_streak_setter_readback_failed");
    ++diagnostics.nativeFeedbackSets;return true;
}
inline bool ResetOriginalFeedback(size_t index){
    if(!Thread()||index>=players.size()||!players[index].registered)return false;
    auto& p=players[index];auto score=original_scoreboard::Find(p.playerState);
    if(!score||!original_scoreboard::ReadNativeScore(*score))return Fail(2308,"deathmatch_feedback_score_guard_unavailable");
    const int32_t kills=score->kills,deaths=score->deaths;
    void* component=StreakComponent(p);int32_t life=-1,streak=-1;
    bool ok=component&&(UsesOriginalStreak()?original_timed_streak::Reset(index):Event(component,L"/Script/ShooterGame.KillStreakComponent.ResetKills",0,nullptr))
        &&Event(component,L"/Script/ShooterGame.KillStreakComponent.GetNumKillsCurrentLife",4,&life)
        &&Event(component,L"/Script/ShooterGame.KillStreakComponent.GetNumKillsStreak",4,&streak)&&life==0&&streak==0;
    if(UsesOriginalStreak()&&!ok)return original_timed_streak::State().error?OriginalStreakFailure():Fail(2351,"deathmatch_original_streak_reset_native_counter_readback_failed");
    if(UsesOriginalStreak())++diagnostics.timedComponentsReset;
    if(!UsesOriginalStreak()&&!timedClass)timedClass=engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t,void*)>(0x1d0c6e0)
        (engine::Class(0x36837a0),nullptr,TimedStreakClass,nullptr,0,nullptr);
    if(!UsesOriginalStreak()&&timedClass&&p.pawn){
        struct Query {void* type;void* result=nullptr;} query{timedClass};
        if(!Event(p.pawn,L"/Script/Engine.Actor.GetComponentByClass",16,&query))ok=false;
        else if(query.result){
            if(!Event(query.result,L"/Game/GameModes/Components/Comp_TimedKillStreak.Comp_TimedKillStreak_C.ResetKillStreak",0,nullptr))ok=false;
            else ++diagnostics.timedComponentsReset;
        }
    }
    if(p.local){
        auto announcement=original_hud::FindCoordinatedWidget(AnnouncementClass);
        if(announcement){void* actualState=p.playerState;
            if(!Event(announcement,AnnouncementStop,8,&actualState))ok=false;else ++diagnostics.bannersStopped;
        }else ++diagnostics.bannerAbsentAtReset; // No mounted banner exists to stop.
    }
    ok=original_scoreboard::ReadNativeScore(*score)&&score->kills==kills&&score->deaths==deaths&&ok;
    diagnostics.lastFeedbackResetSucceeded=ok;
    if(ok){++diagnostics.nativeFeedbackResets;p.feedbackResetPending=false;return true;}
    ++diagnostics.feedbackResetFailures;p.feedbackResetPending=true;return Fail(2309,"deathmatch_original_feedback_reset_not_confirmed");
}
inline bool ResetStreak(size_t index){
    if(index>=players.size()||!players[index].registered)return false;
    auto& p=players[index];p.streak=0;p.lastKill=-1;p.feedbackResetPending=true;
    return ResetOriginalFeedback(index);
}
inline bool ExpireFeedback(size_t index,double now){
    auto& p=players[index];
    if(UsesOriginalStreak()){
        int32_t count=0;
        if(!original_timed_streak::ObserveCount(index,count))return OriginalStreakFailure();
        if(count<0||static_cast<uint32_t>(count)>p.matchKills)
            return Fail(2350,"deathmatch_original_streak_exceeds_confirmed_match_kills");
        // Original K2 timer/death callbacks own expiry. No parallel local clock.
        p.streak=static_cast<uint32_t>(count);
        if(!p.streak)p.lastKill=-1;
        return !p.feedbackResetPending||ResetOriginalFeedback(index);
    }
    if(p.streak&&p.lastKill>=0&&now-p.lastKill>=config.streakTimeout)return ResetStreak(index);
    return !p.feedbackResetPending||ResetOriginalFeedback(index);
}
inline bool Bind(size_t index){
    if(!Thread()||index>=players.size())return false;
    const auto& roster=original_players::players[index];
    if(!roster.ready||!roster.registered||!roster.character||!original_players::Registered(roster.state))return false;
    auto& p=players[index];
    if(p.registered&&p.pawn!=roster.character)return Fail(2310,"deathmatch_pawn_changed_without_respawn_commit");
    if(!original_damage::Bind(index,roster.character)||!original_scoreboard::RegisterPlayer(roster.state))
        return Fail(2311,"deathmatch_original_damage_or_score_bind_failed");
    auto score=original_scoreboard::Find(roster.state);
    if(!score||!original_scoreboard::ReadNativeScore(*score))return Fail(2312,"deathmatch_original_stats_unavailable");
    if(!p.registered){
        if(slotGenerations[index]==UINT32_MAX)return Fail(2349,"deathmatch_spawn_generation_exhausted");
        p.spawnEpoch=++slotGenerations[index];p.initialKills=score->kills;p.initialDeaths=score->deaths;++diagnostics.participants;
    }
    p.registered=true;p.local=static_cast<int>(index)==original_players::localIndex;
    p.pawn=roster.character;p.controller=roster.controller;p.playerState=roster.state;
    p.totalKills=score->kills;p.totalDeaths=score->deaths;p.previousFeet=roster.feetPosition;p.previousYaw=roster.yaw;
    if(!original_inventory_slots::CaptureIdentity(p.pawn,p.identity)||!ReadLife(p.pawn,p.alive,p.health,p.maxHealth))return false;
    if(!p.alive)return Fail(2343,"deathmatch_initial_binding_requires_original_living_character");
    if(!StreakComponent(p))return Fail(2348,"deathmatch_original_kill_streak_component_missing");
    if(UsesOriginalStreak()&&!original_timed_streak::Bind(index,p.pawn,p.playerState,p.spawnEpoch))return OriginalStreakFailure();
    p.phase=Phase::Alive;p.ready=true;
    return !diagnostics.ready||OriginalFFAAlliances();
}
inline bool CanDamage(size_t source,size_t victim){
    if(!Active()||source>=players.size()||victim>=players.size()||source==victim)return false;
    const auto& s=players[source];const auto& v=players[victim];
    return s.registered&&v.registered&&s.ready&&v.ready&&s.alive&&v.alive
        &&s.phase==Phase::Alive&&v.phase==Phase::Alive&&!(v.local&&config.localInvincible)
        &&(!hooks.canDamage||hooks.canDamage(source,victim));
}
inline bool Unregister(size_t index){
    if(!Thread()||index>=players.size())return false;
    auto& p=players[index];if(!p.registered)return true;
    // A running native response must finish. Root retries removals on the next
    // game-thread boundary; do not destroy a Pawn under a ProcessEvent callback.
    if(pending.active&&(pending.source==index||pending.victim==index))
        return Fail(2344,"deathmatch_roster_removal_waits_for_native_damage_transaction");
    if(p.local&&Active())return Fail(2345,"deathmatch_live_bot_count_cannot_remove_local_player");
    if(UsesOriginalStreak()&&!original_streak_effect_authority::BeforeRetire(index))return OriginalStreakEffectFailure();
    if(UsesOriginalStreak()&&!original_timed_streak::BeforeRetire(index))return OriginalStreakFailure();
    p.ready=false;p.alive=false;p.phase=Phase::Absent;
    // This is our bridge's pointer/token cache, not engine life/health state.
    // Root then performs actual GS.RemovePlayerState/native actor destruction
    // and clears the independent score/roster/provider registrations.
    auto& damage=original_damage::targets[index];
    if(damage.bound&&original_damage::diagnostics.boundTargets)--original_damage::diagnostics.boundTargets;
    damage={};p={};if(diagnostics.participants)--diagnostics.participants;
    ++diagnostics.unregistered;RefreshLeader();return true;
}
inline bool Unbind(size_t index){return Unregister(index);}
inline bool BeginNativeDamage(uint64_t token,size_t source,size_t victim,void* equippable,double now,original_damage::DeathData& death){
    if(!Thread()||!token||!std::isfinite(now)||pending.active)return Fail(2313,"deathmatch_native_damage_transaction_invalid");
    if(!CanDamage(source,victim)){++diagnostics.policyRejects;return false;}
    if(!ExpireFeedback(source,now))return false;
    auto killer=original_scoreboard::Find(players[source].playerState),killed=original_scoreboard::Find(players[victim].playerState);
    if(!killer||!killed||killer->round_kills==INT32_MAX)return Fail(2314,"deathmatch_original_round_ordinals_unavailable");
    // Preserve the original global ordinal. Display mapping below is separate.
    death.killsForKiller=killer->round_kills+1;death.killsForVictim=killed->round_kills;
    pending={true,token,source,victim,equippable,now,players[source].streak+1};return true;
}
inline bool MapKillBannerParameters(BannerParameters& p){
    if(!Thread()||!pending.active||p.preview||!players[pending.source].local
       ||!pending.equippable||p.equippable!=pending.equippable)return false;
    p.killCount=static_cast<int32_t>((std::min)(pending.nextFeedbackCount,uint32_t(6)));
    p.maxKills=(std::max)(5,p.killCount);++diagnostics.bannerParametersMapped;return true;
}
inline bool AlreadySeen(uint64_t token){for(auto value:tokens)if(value==token)return true;return false;}
inline bool MarkDead(size_t index,double now,bool credited){
    auto& p=players[index];
    if(!ReadLife(p.pawn,p.alive,p.health,p.maxHealth)||p.alive||p.health>.001f)
        return Fail(2315,"deathmatch_death_requires_original_native_readbacks");
    if(p.phase==Phase::DeadWaiting)return true;
    p.ready=false;p.phase=Phase::DeadWaiting;p.deathAt=now;p.respawnAt=now+config.respawnDelay;
    if(config.enableRespawn){
        float remaining=0;
        if(!Event(p.pawn,L"/Script/Engine.Actor.GetLifeSpan",4,&remaining)||!std::isfinite(remaining))
            return Fail(2346,"deathmatch_original_corpse_timer_query_failed");
        float minimum=static_cast<float>(config.respawnDelay+2);
        if(remaining>0&&remaining<minimum){
            if(!Event(p.pawn,L"/Script/Engine.Actor.SetLifeSpan",4,&minimum)
               ||!Event(p.pawn,L"/Script/Engine.Actor.GetLifeSpan",4,&remaining)||remaining<minimum-.1f)
                return Fail(2347,"deathmatch_original_corpse_timer_retention_failed");
            ++diagnostics.corpseTimersExtended;
        }
    }
    if(!p.deathAccounted){
        ++p.matchDeaths;p.deathAccounted=true;if(p.local)++diagnostics.localDeaths;
        if(!credited){
            auto score=original_scoreboard::Find(p.playerState);
            if(!score||!original_scoreboard::ReadNativeScore(*score)||score->deaths==INT32_MAX)return Fail(2316,"deathmatch_external_death_stats_unavailable");
            original_scoreboard::Put<int32_t>(score->stats,0xe8,score->deaths+1);
            if(!original_scoreboard::ReadNativeScore(*score))return false;
            p.totalDeaths=score->deaths;++diagnostics.externalDeaths;
        }
    }
    return ResetStreak(index);
}
inline bool EndNativeDamage(bool nativeSucceeded,const original_damage::Result& result){
    if(!Thread()||!pending.active)return Fail(2317,"deathmatch_no_native_damage_transaction");
    const auto event=pending;pending={};
    if(!nativeSucceeded)return Fail(2318,"deathmatch_original_damage_application_failed");
    if(result.duplicate||AlreadySeen(event.token)){++diagnostics.duplicateTokens;return true;}
    if(!result.applied)return true;
    if(!result.originalResponse||!result.originalLifeReadback||!result.response)return Fail(2319,"deathmatch_requires_actual_original_damage_result");
    tokens[tokenCursor++%tokens.size()]=event.token;++diagnostics.damageResults;
    auto& source=players[event.source];auto& victim=players[event.victim];victim.health=result.healthAfter;
    if(!result.killed)return true;
    if(!original_scoreboard::RecordKill(event.token,source.playerState,victim.playerState,result.response,event.equippable))
        return Fail(2320,"deathmatch_original_aggregate_score_commit_failed");
    if(hooks.afterScoreCommit)hooks.afterScoreCommit(event.token,event.source,event.victim);
    auto ks=original_scoreboard::Find(source.playerState),vs=original_scoreboard::Find(victim.playerState);
    if(!ks||!vs||!original_scoreboard::ReadNativeScore(*ks)||!original_scoreboard::ReadNativeScore(*vs))return false;
    ++source.matchKills;source.totalKills=ks->kills;victim.totalDeaths=vs->deaths;
    if(UsesOriginalStreak()){
        if(!original_timed_streak::AfterKill(event.source,victim.pawn,result.response,static_cast<int32_t>(event.nextFeedbackCount)))return OriginalStreakFailure();
        original_timed_streak::Snapshot observed;
        if(!original_timed_streak::Observe(event.source,observed))return OriginalStreakFailure();
        if(!original_streak_effect_authority::Ensure(event.source,observed.component,source.pawn,observed.count-1,observed.timerRate))return OriginalStreakEffectFailure();
        source.streak=static_cast<uint32_t>(observed.count);
    }else source.streak=event.nextFeedbackCount;
    source.bestStreak=(std::max)(source.bestStreak,source.streak);source.lastKill=event.now;
    ++diagnostics.kills;
    if(!SetOriginalFeedback(event.source)||!MarkDead(event.victim,event.now,true))return false;
    RefreshLeader();
    if(config.mode==Mode::FreeForAll&&source.matchKills>=config.killLimit)EndMatch(EndReason::KillLimit);
    return true;
}
inline void CancelNativeDamage(){pending={};}
inline bool NewLocalPawn(size_t index,const SpawnSelection& spawn,void*& replacement){
    auto& p=players[index];auto& roster=original_players::players[index];
    if(!p.local||original_players::activeController!=p.controller
       ||Read<void*>(original_players::localPlayer,0x38)!=p.controller||Read<void*>(p.controller,0x510)!=original_players::localPlayer)
        return Fail(2321,"deathmatch_local_respawn_requires_same_original_local_player_controller");
    auto old=p.pawn;auto oldInfo=Read<void*>(p.playerState,0xa18),oldStats=Read<void*>(p.playerState,0x898);
    original_players::CharacterDefaults defaults;
    if(!original_players::ProbeDefaults(L"/Game/Characters/Wushu/Wushu_PC.Wushu_PC_C",defaults)||defaults.abstractClass)
        return Fail(2322,"deathmatch_original_jett_defaults_missing");
    engine::Transform transform;transform.translation=spawn.position;
    if(spawn.basis==PositionBasis::Feet)transform.translation.z+=defaults.capsuleHalfHeight+2;
    constexpr double rad=3.14159265358979323846/180;
    transform.rotation[2]=std::sin(spawn.yaw*rad*.5);transform.rotation[3]=std::cos(spawn.yaw*rad*.5);
    replacement=original_players::Begin(defaults.type,transform,p.controller);
    if(!original_players::IsA(replacement,original_players::ShooterCharacterClass)||replacement==old
       ||!original_players::Write(replacement,0x488,p.playerState))return Fail(2323,"deathmatch_original_local_character_factory_failed");
    original_players::Finish(replacement,transform);
    bool alive=false;float health=0,maximum=0;
    const auto inventory=Read<void*>(replacement,0xc08),mesh=Read<void*>(replacement,0xf30);
    const auto abilities=Read<void*>(replacement,0xa20),attributes=Read<void*>(abilities,0x13a8);
    if(!ReadLife(replacement,alive,health,maximum)||!alive||health<=0||!Read<uint8_t>(replacement,0xa90)
       ||!inventory||!mesh||!abilities||!attributes||!Read<void*>(replacement,0xba0)
       ||!original_players::IsA(Read<void*>(replacement,0x4f8),0x6c7d380))
        return Fail(2324,"deathmatch_fresh_original_local_character_not_alive");
    if(!Event(p.controller,L"/Script/Engine.Controller.Possess",8,&replacement)
       ||Read<void*>(p.controller,0x4b0)!=replacement||Read<void*>(replacement,0x4a0)!=p.controller
       ||Read<void*>(replacement,0x488)!=p.playerState)return Fail(2325,"deathmatch_original_local_possession_failed");
    roster.character=replacement;roster.inventory=Read<void*>(replacement,0xc08);roster.mesh1P=Read<void*>(replacement,0xf30);
    roster.abilities=Read<void*>(replacement,0xa20);roster.attributes=Read<void*>(roster.abilities,0x13a8);
    roster.movement=Read<void*>(replacement,0x4f8);roster.characterInitialized=roster.ready=roster.enabled=true;
    roster.feetPosition=transform.translation;roster.feetPosition.z-=defaults.capsuleHalfHeight+2;roster.yaw=spawn.yaw;
    wcscpy_s(roster.classPath,L"/Game/Characters/Wushu/Wushu_PC.Wushu_PC_C");
    if(!original_enemy_respawn::ReplaceAssociations(roster,old))return false;
    original_players::activePawn=replacement;
    if(Read<void*>(p.playerState,0xa18)!=oldInfo||Read<void*>(p.playerState,0x898)!=oldStats
       ||Read<void*>(original_players::localPlayer,0x38)!=p.controller||!original_players::Registered(p.playerState))
        return Fail(2326,"deathmatch_local_respawn_changed_original_identity_or_score");
    original_enemy_respawn::Result corpse;corpse.oldPawn=old;
    return original_enemy_respawn::ScheduleCorpseCleanup(corpse);
}
inline uint32_t NextHeroId(){randomState^=randomState<<13;randomState^=randomState>>17;randomState^=randomState<<5;return randomState?randomState:1;}
inline bool Respawn(size_t index,double now){
    auto& p=players[index];if(!config.enableRespawn||p.phase!=Phase::DeadWaiting||now<p.respawnAt||!Active())return true;
    if(!ReadLife(p.pawn,p.alive,p.health,p.maxHealth)||p.alive||p.health>.001f)return Fail(2327,"deathmatch_respawn_native_death_gate_failed");
    SpawnSelection spawn;spawn.position=p.previousFeet;spawn.yaw=p.previousYaw;
    if(hooks.selectSpawn){if(!hooks.selectSpawn(index,p.spawnEpoch+1,spawn))return Fail(2328,"deathmatch_original_spawn_selection_unavailable");}
    if(config.mode==Mode::FreeForAll&&!spawn.originalMapAnchor)return Fail(2329,"deathmatch_ffa_requires_original_map_spawn_anchor");
    if(!original_players::Finite(spawn.position)||!std::isfinite(spawn.yaw)||!hooks.beforeRespawn||!hooks.afterSpawn)
        return Fail(2330,"deathmatch_respawn_callbacks_or_transform_missing");
    auto old=p.pawn;
    if(UsesOriginalStreak()&&!original_streak_effect_authority::BeforeRetire(index))return OriginalStreakEffectFailure();
    if(UsesOriginalStreak()&&!original_timed_streak::BeforeRetire(index))return OriginalStreakFailure();
    if(!hooks.beforeRespawn(index,old,p.local))return Fail(2331,"deathmatch_old_weapon_observer_retirement_failed");
    if(!original_effects::RetireActor(old)||!original_inventory_slots::RetirePawn(old))return Fail(2332,"deathmatch_old_local_observer_cache_retirement_failed");
    p.phase=Phase::Rebuilding;void* fresh=nullptr;
    if(p.local){if(!NewLocalPawn(index,spawn,fresh))return false;}
    else {
        const auto logicalId=config.mode==Mode::FreeForAll?original_enemies::AssignedLogicalId(index):NextHeroId();
        const auto hero=original_enemies::HeroFor(logicalId);
        auto feet=spawn.position;
        if(spawn.basis==PositionBasis::OriginalPawnRoot){original_players::CharacterDefaults defaults;
            if(!original_players::ProbeDefaults(original_enemies::HeroPaths[hero],defaults))return Fail(2333,"deathmatch_bot_original_hero_defaults_missing");
            feet.z-=defaults.capsuleHalfHeight+2;
        }
        original_enemy_respawn::Result result;
        if(!original_enemy_respawn::Recreate(index,logicalId,feet,spawn.yaw,result)||!result.ready||!result.newLifeVerified)
            return Fail(2334,"deathmatch_original_bot_respawn_failed");
        if(config.mode==Mode::FreeForAll&&!original_enemies::ValidateMatchHero(index))return Fail(original_enemies::error,original_enemies::stage);
        fresh=result.newPawn;
    }
    // Bind requires the old target cache's already verified dead state.
    if(!original_damage::Sample(index)||!original_damage::Bind(index,fresh))return Fail(2335,"deathmatch_damage_target_rebind_failed");
    if(slotGenerations[index]==UINT32_MAX)return Fail(2349,"deathmatch_spawn_generation_exhausted");
    p.pawn=fresh;p.spawnEpoch=++slotGenerations[index];p.deathAccounted=false;p.lastSample=-1;p.deathAt=p.respawnAt=-1;
    if(!original_inventory_slots::CaptureIdentity(fresh,p.identity)||!hooks.afterSpawn(index,old,fresh,p.local))
        return Fail(2336,"deathmatch_new_original_weapon_camera_input_bind_failed");
    if(UsesOriginalStreak()&&!original_timed_streak::Bind(index,fresh,p.playerState,p.spawnEpoch))return OriginalStreakFailure();
    if(!ReadLife(fresh,p.alive,p.health,p.maxHealth)||!p.alive||p.health<=0)return false;
    p.previousFeet=original_players::players[index].feetPosition;p.previousYaw=spawn.yaw;p.phase=Phase::Alive;p.ready=true;
    ++diagnostics.respawns;if(p.local)++diagnostics.localRespawns;return OriginalFFAAlliances();
}
inline bool Initialize(void* originalWorld,const Config& selected,const Hooks& callbacks={}){
    if(diagnostics.initialized)return diagnostics.ready&&world==originalWorld&&Thread();
    if(!ValidConfig(selected)||!original_players::Thread()||!originalWorld)return Fail(2337,"deathmatch_initial_context_or_configuration_invalid");
    if(selected.enableRespawn&&(!callbacks.beforeRespawn||!callbacks.afterSpawn))return Fail(2338,"deathmatch_requires_respawn_integration_callbacks");
    if(!original_damage::Initialize(originalWorld)||!original_scoreboard::Initialize(originalWorld))return Fail(2339,"deathmatch_original_health_score_context_unavailable");
    world=originalWorld;config=selected;hooks=callbacks;gameThread=GetCurrentThreadId();diagnostics.initialized=true;
    if(UsesOriginalStreak()&&!original_timed_streak::Initialize(world))return OriginalStreakFailure();
    if(UsesOriginalStreak()&&!original_streak_effect_authority::Initialize(world))return OriginalStreakEffectFailure();
    for(size_t i=0;i<players.size();++i)if(original_players::players[i].ready&&original_players::players[i].registered&&!Bind(i))return false;
    if(!diagnostics.participants||!OriginalFFAAlliances())return false;
    diagnostics.startedAt=static_cast<double>(GetTickCount64())/1000;diagnostics.remaining=config.durationSeconds;
    diagnostics.ready=diagnostics.active=true;diagnostics.error=0;diagnostics.stage="deathmatch_local_authority_ready";RefreshLeader();return true;
}
inline bool SetConfig(const Config& selected){
    if(!Thread()||!ValidConfig(selected)||selected.mode!=config.mode
       ||(selected.enableRespawn&&(!hooks.beforeRespawn||!hooks.afterSpawn)))return Fail(2340,"deathmatch_mode_change_requires_new_original_mode_context");
    config=selected;return true;
}
inline bool PollImpl(double now){
    if(!Thread()||!std::isfinite(now)||now<diagnostics.startedAt)return Fail(2341,"deathmatch_invalid_game_thread_clock");
    diagnostics.lastPoll=now;
    if(diagnostics.ended){RefreshLeader();return true;}
    diagnostics.remaining=config.mode==Mode::FreeForAll?(std::max)(0.0,config.durationSeconds-(now-diagnostics.startedAt)):config.durationSeconds;
    if(Active()&&config.mode==Mode::FreeForAll&&diagnostics.remaining<=0)EndMatch(EndReason::TimeLimit);
    if(diagnostics.ended){RefreshLeader();return true;}
    for(size_t i=0;i<players.size();++i){auto& p=players[i];if(!p.registered)continue;
        if(original_inventory_slots::ResolveIdentity(p.identity)!=p.pawn)return Fail(2342,"deathmatch_registered_original_pawn_expired");
        if(now-p.lastSample>=config.sampleInterval){
            if(!original_damage::Sample(i)||!ReadLife(p.pawn,p.alive,p.health,p.maxHealth))return false;
            p.lastSample=now;
            if(!p.alive&&p.phase==Phase::Alive&&!MarkDead(i,now,false))return false;
        }
        if(!ExpireFeedback(i,now)||!Respawn(i,now))return false;
    }
    RefreshLeader();diagnostics.error=0;
    if(!diagnostics.ended)diagnostics.stage="deathmatch_original_combat_loop_running";return true;
}
inline LONG Capture(EXCEPTION_POINTERS* p){
    diagnostics.error=p&&p->ExceptionRecord?p->ExceptionRecord->ExceptionCode:0xe0002300U;
    const auto address=p&&p->ExceptionRecord?reinterpret_cast<uintptr_t>(p->ExceptionRecord->ExceptionAddress):0;
    diagnostics.exceptionRva=address>=engine::image?address-engine::image:address;
    diagnostics.restartRequired=true;diagnostics.active=false;diagnostics.stage="deathmatch_native_exception_requires_new_session";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Poll(double now){__try{return PollImpl(now);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;
    auto b=[](bool v){return v?"true":"false";};
    char streakTimeoutText[32]{};if(UsesOriginalStreak())strcpy_s(streakTimeoutText,"null");else sprintf_s(streakTimeoutText,"%.9g",config.streakTimeout);
    std::fprintf(file,"{\"schema_version\":2,\"streak_policy\":\"%s\",\"practice_streak_seconds_applies\":%s,\"stage\":",UsesOriginalStreak()?"original_timed_component":"practice_configured_interval",b(!UsesOriginalStreak()));original_players::Text(file,diagnostics.stage);
    std::fprintf(file,",\"error\":%u,\"initialized\":%s,\"active\":%s,\"ended\":%s,\"original_ffa_confirmed\":%s,\"remaining_seconds\":%.9g,\"leader\":%d,\"winner\":%d,\"leading_kills\":%u,\"leading_ties\":%u,\"end_reason\":%u,\"participants\":%u,\"damage_results\":%u,\"kills\":%u,\"respawns\":%u,\"local_deaths\":%u,\"local_respawns\":%u,\"streak_timeout_seconds\":%s,\"last_feedback_reset_succeeded\":%s,\"native_feedback_resets\":%u,\"feedback_reset_failures\":%u,\"banner_parameters_mapped\":%u,\"players\":[",diagnostics.error,b(diagnostics.initialized),b(Active()),b(diagnostics.ended),b(diagnostics.originalFFAConfirmed),diagnostics.remaining,diagnostics.leader,diagnostics.winner,diagnostics.leadingKills,diagnostics.leadingTies,static_cast<unsigned>(diagnostics.endReason),diagnostics.participants,diagnostics.damageResults,diagnostics.kills,diagnostics.respawns,diagnostics.localDeaths,diagnostics.localRespawns,streakTimeoutText,b(diagnostics.lastFeedbackResetSucceeded),diagnostics.nativeFeedbackResets,diagnostics.feedbackResetFailures,diagnostics.bannerParametersMapped);
    bool first=true;for(size_t i=0;i<players.size();++i){const auto& p=players[i];if(!p.registered)continue;
        std::fprintf(file,"%s{\"index\":%zu,\"local\":%s,\"alive\":%s,\"ready\":%s,\"phase\":%u,\"spawn_epoch\":%u,\"health\":%.9g,\"max_health\":%.9g,\"match_kills\":%u,\"match_deaths\":%u,\"total_kills\":%d,\"total_deaths\":%d,\"streak\":%u,\"best_streak\":%u,\"feedback_reset_pending\":%s}",first?"":",",i,b(p.local),b(p.alive),b(p.ready),static_cast<unsigned>(p.phase),p.spawnEpoch,p.health,p.maxHealth,p.matchKills,p.matchDeaths,p.totalKills,p.totalDeaths,p.streak,p.bestStreak,b(p.feedbackResetPending));first=false;
    }
    std::fprintf(file,"],\"ready\":%s,\"native_feedback_sets\":%u,\"timed_components_reset\":%u,\"banners_stopped\":%u,\"banner_absent_at_reset\":%u,\"unregistered\":%u,\"corpse_timers_extended\":%u,\"pending_native_damage\":%s",b(diagnostics.ready),diagnostics.nativeFeedbackSets,diagnostics.timedComponentsReset,diagnostics.bannersStopped,diagnostics.bannerAbsentAtReset,diagnostics.unregistered,diagnostics.corpseTimersExtended,b(pending.active));
    std::fputs(",\"native_shots_modified\":false,\"native_health_flags_written\":false,\"image_pages_written\":false,\"local_authority\":true,\"runtime_verified\":false}\n",file);std::fclose(file);return true;
}
}
