#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <array>
#include <cstdio>
#include <limits>
#include "engine_api.h"

// Local authority data for ACTUAL original PlayerState/BasicCombatStats objects.
// No account identity, authentication status or player GUID is synthesized.
// Original champion Blueprint remains responsible for ranking/tie/glow rules.
// Evidence: reports/duel1-original-scoreboard.json. Game-thread calls only.
namespace original_scoreboard {
inline constexpr size_t MaxPlayers=32;
inline constexpr const wchar_t* ChampionEffectClass=L"/Game/Equippables/Guns/Rifles/AK/Champions/FXC/FXC_AK_Champions_Champion_Effect.FXC_AK_Champions_Champion_Effect_C";
inline constexpr const wchar_t* RefreshChampionMethod=L"/Game/Equippables/Guns/Rifles/AK/Champions/FXC/FXC_AK_Champions_Champion_Effect.FXC_AK_Champions_Champion_Effect_C.Find All Players Kills";
struct Entry {
    void *playerState=nullptr,*controller=nullptr,*info=nullptr,*stats=nullptr;
    bool info_created=false,attempted=false,ready=false;
    int32_t kills=0,deaths=0,assists=0,round_kills=0,round_deaths=0;
};
struct Diagnostics {
    bool initialized=false;
    unsigned error=0,registered=0,infos_created=0,confirmed_getters=0;
    unsigned kills_recorded=0,duplicate_kill_events=0,original_kill_broadcasts=0,champion_refreshes=0;
    unsigned active_champion_effects=0;
    bool champion_refresh_is_zero_parameter=false,last_kill_had_original_response=false;
    const char* stage="original_scoreboard_not_initialized";
};
inline Diagnostics diagnostics;
inline const Diagnostics& State(){return diagnostics;}
inline std::array<Entry,MaxPlayers> players{};
inline std::array<uint64_t,128> killTokens{};
inline size_t nextToken=0;
inline DWORD gameThread=0;
inline void *world=nullptr,*gameState=nullptr,*championClass=nullptr,*refreshFunction=nullptr;
inline void* playerInfoClass = nullptr; // Optional verified original mode subclass.
inline bool Readable(const void* pointer,size_t size){
    if(!pointer||!size)return false;
    MEMORY_BASIC_INFORMATION r{};
    if(!VirtualQuery(pointer,&r,sizeof(r))||r.State!=MEM_COMMIT||r.Protect&(PAGE_GUARD|PAGE_NOACCESS))return false;
    const uintptr_t at=reinterpret_cast<uintptr_t>(pointer),base=reinterpret_cast<uintptr_t>(r.BaseAddress);
    return at>=base && size<=r.RegionSize && at-base<=r.RegionSize-size;
}
template<class T> inline T Read(const void* object,size_t offset){
    T value{};if(object){auto p=static_cast<const uint8_t*>(object)+offset;if(Readable(p,sizeof(value)))std::memcpy(&value,p,sizeof(value));}return value;
}
template<class T> inline void Put(void* object,size_t offset,const T& value){std::memcpy(static_cast<uint8_t*>(object)+offset,&value,sizeof(value));}
inline bool IsA(void* object,uintptr_t getter){
    if(!Readable(object,0x30))return false;
    auto type=engine::Class(getter),current=Read<void*>(object,0x10);
    for(unsigned i=0;current&&i<64;++i){if(current==type)return true;current=Read<void*>(current,0x48);}return false;
}
inline bool Fail(unsigned code,const char* stage){diagnostics.error=code;diagnostics.stage=stage;return false;}
inline bool Thread(){return diagnostics.initialized && gameThread==GetCurrentThreadId();}
inline bool RegisteredInOriginalRoster(void* state){
    auto array=Read<void**>(gameState,0x480);
    const int count=Read<int>(gameState,0x488),capacity=Read<int>(gameState,0x48c);
    if(count<0||count>256||capacity<count||count && !Readable(array,count*sizeof(void*)))return false;
    for(int i=0;i<count;++i)if(array[i]==state)return true;return false;
}
inline Entry* Find(void* state){for(auto& e:players)if(e.playerState==state)return &e;return nullptr;}
inline bool ReadNativeScore(Entry& e){
    if(!IsA(e.stats,0x65b0cd0)||!IsA(e.info,0x64f4540))return false;
    void* resolved=nullptr;
    if(!engine::Event(e.info,L"/Script/ShooterGame.OwnerExclusivePlayerInfo.GetPlayerState",&resolved)||resolved!=e.playerState)return false;
    int32_t kills=-1;
    if(!engine::Event(e.info,L"/Script/ShooterGame.OwnerExclusivePlayerInfo.GetKills",&kills))return false;
    if(kills!=Read<int32_t>(e.stats,0xe4))return false;
    e.kills=kills;e.deaths=Read<int32_t>(e.stats,0xe8);e.assists=Read<int32_t>(e.stats,0xec);
    return true;
}
inline bool Initialize(void* inWorld){
    if(diagnostics.initialized)return world==inWorld && Thread();
    if(!engine::image||!Readable(inWorld,0x180))return Fail(841,"scoreboard_world_missing");
    world=inWorld;gameState=Read<void*>(world,0x178);
    if(!IsA(gameState,0x6c9d280))return Fail(842,"scoreboard_requires_original_shooter_game_state");
    gameThread=GetCurrentThreadId();diagnostics.initialized=true;diagnostics.stage="original_scoreboard_ready_no_players";return true;
}
inline bool RegisterPlayer(void* state,bool createMissingInfo=true){
    if(!Thread())return Fail(843,"scoreboard_wrong_thread");
    if(!IsA(state,0x6cc4040)||!RegisteredInOriginalRoster(state))return Fail(844,"scoreboard_requires_registered_original_player_state");
    Entry* entry=Find(state);
    if(entry&&entry->ready)return ReadNativeScore(*entry);
    if(entry&&entry->attempted)return Fail(845,"scoreboard_partial_player_no_retry");
    if(!entry)for(auto& e:players)if(!e.playerState){entry=&e;break;}
    if(!entry)return Fail(846,"scoreboard_roster_capacity");
    auto& e=*entry;e.playerState=state;e.stats=Read<void*>(state,0x898);e.info=Read<void*>(state,0xa18);
    if(!IsA(e.stats,0x65b0cd0))return Fail(847,"original_basic_combat_stats_component_missing");
    e.controller=Read<void*>(state,0x210);
    if(e.controller && (!IsA(e.controller,0x6943db0)||Read<void*>(e.controller,0x470)!=state))
        return Fail(848,"scoreboard_original_controller_state_pair_mismatch");
    if(!e.info){
        if(!createMissingInfo)return Fail(849,"original_owner_exclusive_info_missing");
        e.attempted=true;diagnostics.stage="creating_original_owner_exclusive_info";
        engine::Transform transform;
        e.info=engine::function<void*(*)(void*,void*,const engine::Transform*,uint8_t,void*,uint8_t)>(0x43fada0)
          (world,playerInfoClass ? playerInfoClass : engine::Class(0x64f4540),&transform,1,e.controller,0);
        if(!IsA(e.info,0x64f4540))return Fail(850,"original_owner_exclusive_info_spawn_failed");
        // The original AuthSetController entry is empty in this client. These
        // links supply missing local authority state; native getters remain intact.
        Put(e.info,0x4e8,e.controller);Put(state,0xa18,e.info);
        engine::function<void(*)(void*,const engine::Transform*,bool,void*,uint8_t)>(0x3ef6cb0)
          (e.info,&transform,false,nullptr,0);
        e.info_created=true;++diagnostics.infos_created;
        if(!engine::Event(state,L"/Script/ShooterGame.ShooterPlayerState.OnRepPlayerInfo",nullptr))
            return Fail(851,"original_player_info_notification_missing");
    }
    if(!ReadNativeScore(e))return Fail(852,"original_owner_exclusive_getters_not_confirmed");
    e.ready=true;++diagnostics.registered;++diagnostics.confirmed_getters;
    diagnostics.error=0;diagnostics.stage="original_scoreboard_player_registered";return true;
}
inline bool RefreshChampionEffects(){
    if(!Thread())return false;
    if(!championClass){
        championClass=engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t,void*)>(0x1d0c6e0)
          (engine::Class(0x3683900),nullptr,ChampionEffectClass,nullptr,0,nullptr);
    }
    if(!championClass)return Fail(853,"original_champion_effect_class_missing");
    refreshFunction=engine::Find(RefreshChampionMethod);
    // UFunction.ParmsSize+be is used by the pinned ProcessEvent memcpy path;
    // shipped custom event also has zero serialized property/child counts.
    diagnostics.champion_refresh_is_zero_parameter=refreshFunction && Read<uint16_t>(refreshFunction,0xbe)==0;
    if(!diagnostics.champion_refresh_is_zero_parameter)return Fail(854,"original_champion_refresh_signature_changed");
    struct Query {void* world;void* type;void** actors=nullptr;int count=0,capacity=0;} query{world,championClass};
    if(!engine::Event(engine::DefaultObject(engine::Class(0x43a9670)),L"/Script/Engine.GameplayStatics.GetAllActorsOfClass",&query))
        return Fail(855,"original_champion_effect_query_failed");
    diagnostics.active_champion_effects=0;
    if(query.count<0||query.count>64||query.capacity<query.count)return Fail(856,"original_champion_effect_count_invalid");
    for(int i=0;i<query.count;++i){
        if(!query.actors[i])continue;
        engine::Event(query.actors[i],RefreshChampionMethod,nullptr);
        ++diagnostics.active_champion_effects;++diagnostics.champion_refreshes;
    }
    // Native result TArray uses the engine allocator. Same release dispatch
    // seen in original GetShooterPlayerStates consumers; bounded query is not leaked.
    if(query.actors)engine::function<void(*)(void*)>(0x1915720)(query.actors);
    return true;
}
struct KillEvent {
    void* killer=nullptr;void* victim=nullptr;int32_t killerRoundKills=0,victimRoundDeaths=0;
    void* response=nullptr;void* weapon=nullptr;void* assists=nullptr;int32_t assistCount=0,assistCapacity=0;
};
static_assert(sizeof(KillEvent)==56 && offsetof(KillEvent,response)==24 && offsetof(KillEvent,assists)==40);
inline bool RecordKill(uint64_t localKillToken,void* killerState,void* victimState,
                       void* actualDamageResponse=nullptr,void* actualEquippable=nullptr,
                       int32_t originalKillerKillNumber=-1,int32_t originalVictimKillNumber=-1){
    if(!Thread()||!localKillToken)return Fail(857,"scoreboard_kill_context_invalid");
    for(auto token:killTokens)if(token==localKillToken){++diagnostics.duplicate_kill_events;return true;}
    if(killerState==victimState)return Fail(858,"scoreboard_training_kill_must_have_distinct_players");
    auto killer=Find(killerState),victim=Find(victimState);
    if(!killer||!victim||!killer->ready||!victim->ready)return Fail(859,"scoreboard_kill_players_not_registered");
    if(!ReadNativeScore(*killer)||!ReadNativeScore(*victim))return Fail(860,"scoreboard_kill_native_score_unavailable");
    if(killer->kills<0||victim->deaths<0||killer->kills==(std::numeric_limits<int32_t>::max)()||
       victim->deaths==(std::numeric_limits<int32_t>::max)())return Fail(861,"scoreboard_score_out_of_range");
    // Original AuthGrantKillToPlayer has no local implementation. This updates
    // the precise original replicated aggregate fields used by native GetKills.
    Put<int32_t>(killer->stats,0xe4,killer->kills+1);Put<int32_t>(victim->stats,0xe8,victim->deaths+1);
    ++killer->round_kills;++victim->round_deaths;
    killTokens[nextToken++%killTokens.size()]=localKillToken;++diagnostics.kills_recorded;
    if(!ReadNativeScore(*killer)||!ReadNativeScore(*victim))return Fail(862,"scoreboard_score_write_not_observed_by_original_getter");
    diagnostics.last_kill_had_original_response=IsA(actualDamageResponse,0x64f43e0) && IsA(actualEquippable,0x645c660);
    if(diagnostics.last_kill_had_original_response && originalKillerKillNumber>=0 && originalVictimKillNumber>=0){
        KillEvent event{killerState,victimState,originalKillerKillNumber,originalVictimKillNumber,actualDamageResponse,actualEquippable};
        // Same original multicast dispatch used by client gameplay; no fabricated
        // DamageResponse or guessed native kill-list indices are sent to unrelated
        // original HUD/FXC listeners. Local aggregate totals are not these indices.
        engine::function<void(*)(void*,const void*)>(0x1cf69a0)(static_cast<uint8_t*>(gameState)+0x918,&event);
        ++diagnostics.original_kill_broadcasts;
    }
    if(!RefreshChampionEffects())return false;
    diagnostics.error=0;diagnostics.stage="original_score_updated_and_original_champion_graph_refreshed";return true;
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!Thread()||!path)return false;
    FILE* out=nullptr;if(_wfopen_s(&out,path,L"wb")||!out)return false;
    std::fprintf(out,"{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u,\"registered\":%u,\"infos_created\":%u,\"kills_recorded\":%u,\"original_kill_broadcasts\":%u,\"champion_refreshes\":%u,\"active_champion_effects\":%u,\"players\":[",
      diagnostics.stage,diagnostics.error,diagnostics.registered,diagnostics.infos_created,diagnostics.kills_recorded,
      diagnostics.original_kill_broadcasts,diagnostics.champion_refreshes,diagnostics.active_champion_effects);
    bool comma=false;
    for(auto& e:players)if(e.playerState){
      if(e.ready)ReadNativeScore(e);
      std::fprintf(out,"%s{\"ready\":%d,\"info_created\":%d,\"kills\":%d,\"deaths\":%d,\"assists\":%d}",comma?",":"",e.ready,e.info_created,e.kills,e.deaths,e.assists);comma=true;
    }
    std::fprintf(out,"],\"native_champion_visual_runtime_verified\":false}\n");std::fclose(out);return true;
}
}
