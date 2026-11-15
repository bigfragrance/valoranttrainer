#pragma once
#include "original_weapon.h"
#include "original_effects.h"
#include <array>

// Missing local authority assembly only. The actual original component owns
// Assign Kill Champion, native sorted roster selection, GetKills, tie retention,
// multicast field updates and its original notification delegates. No score,
// champion field, material, color, shader, or original class/CDO is written here.
// Static proof: work/training-champion-state/component.{json,txt}.
namespace training_champion_state {
inline constexpr const wchar_t* ClassPath=L"/Game/GameModes/Components/Comp_GameState_KillChampion.Comp_GameState_KillChampion_C";
inline constexpr const wchar_t* MethodPrefix=L"/Game/GameModes/Components/Comp_GameState_KillChampion.Comp_GameState_KillChampion_C.";
inline constexpr size_t MaxPlayers=32;
struct Player {
    void *playerState=nullptr,*info=nullptr,*team=nullptr;
    original_effects::WeakIdentity identity{};
    int32_t kills=0;
    uint8_t teamRole=0;
};
struct Diagnostics {
    bool initialized=false,created=false,creationAttempted=false,contextValid=false,registered=false,hasBegunPlay=false;
    bool originalScriptsVerified=false,originalGameModeAvailable=false,leaderFieldsVerified=false,restartRequired=false;
    uint32_t error=0;
    uint64_t polls=0,originalAssignments=0,rosterChanges=0,leaderChanges=0,exceptions=0,lastFingerprint=0,lastPoll=0;
    uintptr_t exceptionRva=0;
    void *world=nullptr,*gameState=nullptr,*component=nullptr,*componentClass=nullptr;
    void *champion=nullptr,*attackingChampion=nullptr,*defendingChampion=nullptr,*lastKiller=nullptr;
    int32_t sortedPlayers=0,gameStatePlayers=0;
    int32_t expectedScriptBytes=0,actualScriptBytes=0;
    uint8_t expectedScriptFirst=0,actualScriptFirst=0;
    const char* stage="training_champion_state_unbound";
    wchar_t failedEvent[512]{};
    uint16_t expectedEventSize=0,actualEventSize=0;
};
inline Diagnostics state;
inline std::array<Player,MaxPlayers> roster{};
inline original_effects::WeakIdentity gameStateIdentity{},componentIdentity{};
inline const Diagnostics& State(){return state;}
template<class T> inline T Read(const void* p,size_t offset){return original_players::Value<T>(p,offset);}
inline bool Thread(){return !state.restartRequired&&original_players::Thread();}
inline bool Fail(uint32_t code,const char* stage){state.error=code;state.stage=stage;return false;}
inline bool Event(void* object,const wchar_t* name,uint16_t size,void* parameters,void* outer=nullptr){
    auto fn=engine::Find(name);const auto actual=Read<uint16_t>(fn,0xbe);
    if(!object||!fn||actual!=size||(outer&&Read<void*>(fn,0x28)!=outer)){
        wcsncpy_s(state.failedEvent,name,_TRUNCATE);state.expectedEventSize=size;state.actualEventSize=actual;return false;
    }
    auto call=Read<void*>(Read<void**>(object,0),0x298);if(!call)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(call)(object,fn,parameters);return true;
}
inline void* Field(void* object,const wchar_t* wanted){
    const auto target=original_players::Utf8(wanted);auto type=Read<void*>(object,0x10);
    for(unsigned depth=0;type&&depth<128;++depth){
        auto field=Read<void*>(type,0x58);
        for(unsigned count=0;field&&count<4096;++count){
            char name[128]{};original_weapon::OriginalNameValue(static_cast<uint8_t*>(field)+0x20,name);
            if(target==name)return field;auto next=Read<void*>(field,0x18);if(next==field)return nullptr;field=next;
        }
        auto query=Read<void*>(Read<void**>(type,0),0x2f8);if(!query)return nullptr;
        auto next=reinterpret_cast<void*(*)(void*)>(query)(type);if(next==type)return nullptr;type=next;
    }
    return nullptr;
}
inline bool ObjectField(void* object,const wchar_t* name,void*& value){
    value=nullptr;auto property=Field(object,name);if(!property||Read<int32_t>(property,0x34)!=8)return false;
    const auto flags=Read<uint64_t>(Read<void*>(property,8),0x18);
    if((flags&0x4008001ULL)!=0x4008001ULL)return false;
    const auto offset=Read<int32_t>(property,0x44),size=Read<int32_t>(Read<void*>(object,0x10),0x60);
    if(offset<0x30||size<offset+8||size>2*1024*1024)return false;
    value=Read<void*>(object,static_cast<size_t>(offset));return true;
}
inline bool Scripts(){
    struct Expected {const wchar_t* name;int32_t bytes;uint16_t parameters;uint8_t first;};
    constexpr Expected expected[]{
        {L"Assign Kill Champion",1761,8,0x4c},
        {L"GetHighestPlayer",735,97,0x31},
        {L"ExecuteUbergraph_Comp_GameState_KillChampion",452,4,0x4e},
        {L"MulticastUpdateChampion",36,8,0x64},
        {L"MulticastUpdateTeamChampion",54,16,0x64},
        {L"ReceiveBeginPlay",18,0,0x46}
    };
    for(const auto& e:expected){
        const auto name=std::wstring(MethodPrefix)+e.name;auto fn=engine::Find(name.c_str());
        auto code=Read<const uint8_t*>(fn,0x68);
        if(!fn||Read<void*>(fn,0x28)!=state.componentClass||Read<uint16_t>(fn,0xbe)!=e.parameters
           ||Read<int32_t>(fn,0x70)!=e.bytes||!original_players::Readable(code,static_cast<size_t>(e.bytes))
           ||code[0]!=e.first||code[e.bytes-1]!=0x53){
            wcsncpy_s(state.failedEvent,name.c_str(),_TRUNCATE);state.expectedEventSize=e.parameters;state.actualEventSize=Read<uint16_t>(fn,0xbe);
            state.expectedScriptBytes=e.bytes;state.actualScriptBytes=Read<int32_t>(fn,0x70);state.expectedScriptFirst=e.first;
            state.actualScriptFirst=Read<uint8_t>(code,0);return false;
        }
    }
    return true;
}
inline void* QueryComponent(){
    struct Query {void* type;void* component=nullptr;} q{state.componentClass};
    return Event(state.gameState,L"/Script/Engine.Actor.GetComponentByClass",16,&q)?q.component:nullptr;
}
inline bool ReadLeaders(){
    state.leaderFieldsVerified=ObjectField(state.component,L"Champion",state.champion)
        &&ObjectField(state.component,L"AttackingChampion",state.attackingChampion)
        &&ObjectField(state.component,L"DefendingChampion",state.defendingChampion);
    for(auto pointer:{state.champion,state.attackingChampion,state.defendingChampion})
        if(pointer&&!original_players::IsA(pointer,original_players::ShooterStateClass))state.leaderFieldsVerified=false;
    return state.leaderFieldsVerified;
}
inline bool Context(){
    state.contextValid=state.initialized&&state.world&&Read<void*>(state.world,0x178)==state.gameState
        &&original_effects::ResolveIdentity(gameStateIdentity)==state.gameState
        &&original_effects::ResolveIdentity(componentIdentity)==state.component
        &&Read<void*>(state.component,0xc8)==state.gameState&&Read<void*>(state.component,0x28)==state.gameState
        &&QueryComponent()==state.component;
    state.registered=state.contextValid&&(Read<uint8_t>(state.component,0xa0)&1)!=0;
    state.hasBegunPlay=state.contextValid&&(Read<uint8_t>(state.component,0xc2)&0x40)!=0;
    return state.contextValid&&state.registered;
}
inline bool InitializeImpl(void* world){
    if(!Thread()||!world)return false;
    if(state.initialized)return state.world==world&&Context();
    if(state.creationAttempted)return Fail(3301,"champion_component_partial_creation_requires_new_session");
    state.world=world;state.gameState=Read<void*>(world,0x178);
    if(!original_players::IsA(state.gameState,original_players::ShooterGameStateClass)
       ||engine::function<int(*)(void*)>(0x3efc820)(state.gameState)!=0)
        return Fail(3302,"champion_state_requires_original_offline_shooter_game_state");
    state.componentClass=engine::Find(ClassPath);
    if(!state.componentClass)state.componentClass=engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t)>(0x1d0c6e0)
        (engine::Class(0x36837a0),nullptr,ClassPath,nullptr,0);
    if(!state.componentClass||!original_players::Derived(state.componentClass,engine::Class(0x36837a0)))
        return Fail(3303,"original_champion_component_class_unavailable");
    state.originalScriptsVerified=Scripts();if(!state.originalScriptsVerified)return Fail(3304,"original_champion_script_or_abi_changed");
    state.originalGameModeAvailable=original_players::IsA(Read<void*>(world,0x170),0x6c8ec60);
    state.component=QueryComponent();
    if(!state.component){
        struct alignas(16) Add {void* type;uint8_t manual=0,pad[7]{};engine::Transform transform;uint8_t deferred=1,pad2[7]{};void* result=nullptr;} add{state.componentClass};
        static_assert(sizeof(Add)==128);
        state.creationAttempted=true;
        if(!Event(state.gameState,L"/Script/Engine.Actor.AddComponentByClass",128,&add)||!add.result)
            return Fail(3305,"original_champion_component_add_failed");
        state.component=add.result;state.created=true;
        struct alignas(16) Finish {void* component;uint8_t manual=0,pad[7]{};engine::Transform transform;} finish{state.component};
        static_assert(sizeof(Finish)==112);
        if(!Event(state.gameState,L"/Script/Engine.Actor.FinishAddComponent",112,&finish))
            return Fail(3306,"original_champion_component_finish_failed");
    }
    if(!original_effects::CaptureIdentity(state.gameState,gameStateIdentity)
       ||!original_effects::CaptureIdentity(state.component,componentIdentity))return Fail(3307,"original_champion_component_identity_unavailable");
    state.initialized=true;
    if(!Context()||!ReadLeaders())return Fail(3308,"original_champion_component_owner_registration_or_fields_invalid");
    state.error=0;state.stage="original_champion_component_ready";return true;
}
inline uint64_t Mix(uint64_t hash,uint64_t value){for(unsigned i=0;i<8;++i){hash^=static_cast<uint8_t>(value>>(i*8));hash*=1099511628211ULL;}return hash;}
inline bool Snapshot(uint64_t& fingerprint,void*& changedKiller){
    struct Query {void* world;void* viewer;uint8_t alliance=3,omitBots=1,omitCoaches=1,pad[5]{};void** players=nullptr;int32_t count=0,capacity=0;} q{state.component,state.champion};
    static_assert(sizeof(Query)==40&&offsetof(Query,players)==24);
    auto library=engine::DefaultObject(engine::Class(0x6c14c60));
    if(!Event(library,L"/Script/ShooterGame.ShooterBlueprintLibrary.GetShooterPlayerStatesSortedByScore",40,&q))return Fail(3309,"original_champion_sorted_roster_query_failed");
    const bool bounded=q.count>=0&&q.count<=static_cast<int32_t>(MaxPlayers)&&q.capacity>=q.count
        &&(!q.count||original_players::Readable(q.players,static_cast<size_t>(q.count)*8));
    bool valid=bounded;std::array<Player,MaxPlayers> next{};fingerprint=14695981039346656037ULL;changedKiller=nullptr;
    if(bounded)for(int32_t i=0;i<q.count;++i){
        auto& p=next[static_cast<size_t>(i)];p.playerState=q.players[i];
        if(!original_players::IsA(p.playerState,original_players::ShooterStateClass)
           ||!original_effects::CaptureIdentity(p.playerState,p.identity)
           ||!Event(p.playerState,L"/Script/ShooterGame.ShooterPlayerState.GetOwnerExclusivePlayerInfo",8,&p.info)||!p.info
           ||!Event(p.info,L"/Script/ShooterGame.OwnerExclusivePlayerInfo.GetKills",4,&p.kills)||p.kills<0){valid=false;break;}
        struct Team {void* actor;void* result=nullptr;} team{p.info};
        if(!Event(engine::DefaultObject(engine::Class(0x65a0b40)),L"/Script/ShooterGame.BaseTeamComponent.GetTeamComponent",16,&team)
           ||!team.result||!Event(team.result,L"/Script/ShooterGame.BaseTeamComponent.GetTeamRole",1,&p.teamRole)){valid=false;break;}
        p.team=team.result;
        for(const auto& before:roster)if(before.playerState==p.playerState&&before.identity.index==p.identity.index
           &&before.identity.serial==p.identity.serial&&p.kills>before.kills)changedKiller=p.playerState;
        fingerprint=Mix(Mix(Mix(Mix(fingerprint,static_cast<uint32_t>(p.identity.index)),static_cast<uint32_t>(p.identity.serial)),static_cast<uint32_t>(p.kills)),p.teamRole);
    }
    if(q.players)engine::function<void(*)(void*)>(0x1915720)(q.players);
    if(!valid)return Fail(3310,"original_champion_sorted_roster_info_kills_or_team_unavailable");
    state.sortedPlayers=q.count;state.gameStatePlayers=Read<int32_t>(state.gameState,0x488);roster=next;
    return true;
}
inline bool PollImpl(void* world,bool force){
    if(!InitializeImpl(world))return false;++state.polls;
    const auto now=GetTickCount64();if(!force&&state.lastPoll&&now-state.lastPoll<250)return ReadLeaders();state.lastPoll=now;
    uint64_t fingerprint=0;void* killer=nullptr;if(!ReadLeaders()||!Snapshot(fingerprint,killer))return false;
    if(!state.sortedPlayers){state.error=0;state.stage="original_champion_waiting_for_eligible_roster";return true;}
    if(!force&&state.originalAssignments&&state.lastFingerprint==fingerprint){state.error=0;state.stage="original_champion_rules_current";return true;}
    const auto oldChampion=state.champion,oldAttacking=state.attackingChampion,oldDefending=state.defendingChampion;
    const auto name=std::wstring(MethodPrefix)+L"Assign Kill Champion";
    if(!Event(state.component,name.c_str(),8,&killer,state.componentClass))return Fail(3311,"original_assign_kill_champion_failed");
    ++state.originalAssignments;++state.rosterChanges;state.lastKiller=killer;
    if(!ReadLeaders())return Fail(3312,"original_champion_notification_fields_unavailable");
    if(oldChampion!=state.champion||oldAttacking!=state.attackingChampion||oldDefending!=state.defendingChampion)++state.leaderChanges;
    state.lastFingerprint=fingerprint;state.error=0;state.stage="original_assign_rules_and_multicast_state_observed";return true;
}
inline LONG Capture(EXCEPTION_POINTERS* exception){
    state.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0003300U;
    const auto pc=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=pc>=engine::image?pc-engine::image:pc;++state.exceptions;state.restartRequired=true;
    state.stage="original_champion_native_exception_requires_new_session";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* world){__try{return InitializeImpl(world);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool Poll(void* world,bool force=false){__try{return PollImpl(world,force);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool ReadChampionState(void* world,void*& champion,void*& attacking,void*& defending){
    champion=attacking=defending=nullptr;if(!Poll(world))return false;
    champion=state.champion;attacking=state.attackingChampion;defending=state.defendingChampion;return state.leaderFieldsVerified;
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;std::wstring temporary(path);temporary+=L".tmp";FILE* f=nullptr;if(_wfopen_s(&f,temporary.c_str(),L"wb")||!f)return false;
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(f,"{\"schema_version\":1,\"pid\":%lu,\"stage\":",GetCurrentProcessId());original_players::Text(f,state.stage);
    std::fprintf(f,",\"error\":%u,\"initialized\":%s,\"created_missing_component\":%s,\"context_valid\":%s,\"registered\":%s,\"has_begun_play\":%s,\"original_scripts_verified\":%s,\"original_game_mode_available\":%s,\"leader_fields_verified\":%s,\"restart_required\":%s,\"game_state\":\"%p\",\"component\":\"%p\",\"champion\":\"%p\",\"attacking_champion\":\"%p\",\"defending_champion\":\"%p\",\"last_killer\":\"%p\",\"sorted_players\":%d,\"game_state_players\":%d,\"original_assignments\":%llu,\"leader_changes\":%llu,\"exception_rva\":%llu,\"last_failed_original_event\":",
        state.error,b(state.initialized),b(state.created),b(state.contextValid),b(state.registered),b(state.hasBegunPlay),b(state.originalScriptsVerified),b(state.originalGameModeAvailable),b(state.leaderFieldsVerified),b(state.restartRequired),state.gameState,state.component,state.champion,state.attackingChampion,state.defendingChampion,state.lastKiller,state.sortedPlayers,state.gameStatePlayers,static_cast<unsigned long long>(state.originalAssignments),static_cast<unsigned long long>(state.leaderChanges),static_cast<unsigned long long>(state.exceptionRva));
    const auto failed=original_players::Utf8(state.failedEvent);original_players::Text(f,failed.c_str());
    std::fprintf(f,",\"expected_event_size\":%u,\"actual_event_size\":%u,\"roster\":[",static_cast<unsigned>(state.expectedEventSize),static_cast<unsigned>(state.actualEventSize));
    bool comma=false;for(const auto& p:roster)if(p.playerState){if(comma)std::fputc(',',f);comma=true;
        std::fprintf(f,"{\"player_state\":\"%p\",\"object_index\":%d,\"weak_serial\":%d,\"native_kills\":%d,\"original_team_role\":%u}",p.playerState,p.identity.index,p.identity.serial,p.kills,static_cast<unsigned>(p.teamRole));}
    std::fprintf(f,"],\"expected_script_bytes\":%d,\"actual_script_bytes\":%d,\"expected_script_first\":%u,\"actual_script_first\":%u",state.expectedScriptBytes,state.actualScriptBytes,static_cast<unsigned>(state.expectedScriptFirst),static_cast<unsigned>(state.actualScriptFirst));
    std::fputs(",\"ranking_and_tie_rule\":\"original_AssignKillChampion_GetHighestPlayer\",\"notifications\":\"original_MulticastUpdateChampion_and_Team\",\"writes_scores_or_champion_fields\":false,\"original_rules_may_update_champion_fields\":true,\"writes_materials\":false,\"native_rule_omit_bots\":true,\"native_rule_omit_coaches\":true,\"rendered_pixels_verified\":false}\n",f);
    const bool ok=std::ferror(f)==0,closed=std::fclose(f)==0;return ok&&closed&&MoveFileExW(temporary.c_str(),path,MOVEFILE_REPLACE_EXISTING);
}
}
