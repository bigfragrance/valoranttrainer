#pragma once
#include "training_control.h"
#include "training_maps.h"
#include "all_skins.h"
#include "weapon_runtime_bindings.h"
#include "skin_effect_controls.h"
#include "training_bot_controller.h"
#include "training_combat_runtime.h"
#include "deathmatch_rules.h"
#include "original_buy_menu.h"
#include "original_ui_input_mode.h"
#include "kill_feedback_view.h"
#include "original_infinite_ammo.h"
#include "original_jett_priming.h"
#include "original_jett_world.h"
#include "original_jett_sync.h"
#include "original_dagger_combat.h"
#include "training_player_roles_probe.h"
#include "original_npc_locality.h"
#include "original_audio_listeners.h"
#include "original_streak_hud.h"
#include "training_world_geometry.h"
#include "original_armor.h"
#include "original_melee_combat.h"
#include "deathmatch_pickups.h"
#include "original_deathmatch_topbar.h"
#include "training_performance.h"
#include "sound_locator_fixture.h"
#include "sound_locator_nav_export.h"
#include <cstdarg>

// Local match orchestration. The original client owns presentation, input,
// physics, weapons and their shot data. Commands are consumed on this thread.
namespace training_session {
using Vec=engine::Vec;
using Guid=original_content::Guid;
struct Hooks {bool (*rebindLocal)(void* oldPawn,void* newPawn)=nullptr;};
inline training_control::Initial initial{};
inline bool enabled=false,initialized=false,ready=false,restartRequested=false;
inline unsigned error=0,participants=0;
inline const char* stage="training_session_disabled";
inline const char* commandStage="none";
inline Hooks hooks;
inline void* world=nullptr;
inline uint64_t lastPublish=0,lastDiagnostics=0,lastLocalitySample=0;
inline bool pendingCombat[32]{};
inline Guid pendingWeapon{};
inline bool hasPendingWeapon=false;
inline training_control::Skin pendingSkin{};
inline bool hasPendingSkin=false;
inline bool ammoToggleHeld=false;
inline bool testActionsAllowed=false;
inline uint32_t testPoseSequence=0;
inline double lastSpawnYaw=0;
inline bool matchClockStarted=false;
inline all_skins::CatalogValidation skinCatalogValidation{};
inline int spawnRouteChoice[32]{};

inline bool Fail(unsigned code,const char* why){error=code;stage=why;ready=false;return false;}
inline Vec Vector(const training_control::Point& p){return {p.x,p.y,p.z};}
inline Guid Id(const training_control::Guid& id){Guid result;std::memcpy(&result,&id,sizeof(result));return result;}
inline bool FreeForAll(){return initial.config.mode==1;}
inline bool UpdateOriginalDeathmatchTopbar(bool initialize=false){
    if(!FreeForAll())return true;
    std::array<original_deathmatch_topbar::PlayerScore,32> scores{};size_t count=0;
    for(size_t i=0;i<32;++i){const auto p=deathmatch_rules::Player(i);if(!p||!p->registered)continue;
        scores[count++]={static_cast<uint32_t>(i),p->playerState,static_cast<int32_t>(p->streak),static_cast<int32_t>(p->matchKills)};}
    const bool ok=initialize?original_deathmatch_topbar::Initialize(world,original_hud::State().hudModel,scores.data(),count)
                            :original_deathmatch_topbar::Poll(scores.data(),count);
    return ok||Fail(original_deathmatch_topbar::State().error,original_deathmatch_topbar::State().stage);
}
inline bool Valid(const training_control::Configuration& c){
    const auto finite=[](float v,float lo,float hi){return std::isfinite(v)&&v>=lo&&v<=hi;};
    return c.mode<=1&&c.totalPlayers>=2&&c.totalPlayers<=32&&(c.flags&~15U)==0&&c.style<=5&&c.seed
        &&(c.mode==0||c.totalPlayers<=original_enemies::HeroCatalogCount)
        &&finite(c.peeksPerMinute,.5f,180)&&finite(c.respawnSeconds,.2f,15)&&finite(c.streakSeconds,.5f,30)
        &&finite(c.reactionMs,60,2000)&&finite(c.reactionSpreadMs,0,1000)
        &&finite(c.aimDegreesPerSecond,20,1080)&&finite(c.aimSpread,0,800)
        &&finite(c.accuracy,.02f,1)&&finite(c.accuracySpread,.01f,.7f)
        &&finite(c.movement,.05f,1)&&finite(c.movementSpread,.01f,.7f);
}
inline bool Load(const std::wstring& directory){
    const auto path=directory+L"initial-control-"+std::to_wstring(GetCurrentProcessId())+L".bin";
    HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return GetLastError()==ERROR_FILE_NOT_FOUND;
    DWORD count=0;const auto bytes=GetFileSize(file,nullptr);
    const bool ok=bytes==sizeof(initial)&&ReadFile(file,&initial,sizeof(initial),&count,nullptr)&&count==sizeof(initial);
    CloseHandle(file);
    if(!ok||initial.magic!=training_control::Magic||initial.version!=training_control::Version||initial.bytes!=sizeof(initial)
       ||!Valid(initial.config)||initial.spawnCount>64||initial.routes.count>64||!initial.mapId[0]||initial.mapId[63])
        return Fail(2600,"training_session_initial_configuration_invalid");
    enabled=true;stage="training_session_waiting_original_world";
    if(!training_control::Initialize())return Fail(2601,"training_session_control_channel_unavailable");
    constexpr char state[]="{\"ready\":false,\"stage\":\"waiting_original_world\",\"players\":[]}";
    training_control::Publish(state,sizeof(state)-1,initial.config.revision);
    return true;
}
inline void ConfigureFactory(){
    if(!enabled)return;
    original_players::options.freeForAll=FreeForAll();
    if(FreeForAll()){
        original_players::options.playerStateClassPath=deathmatch_rules::StateClassPath;
        original_players::options.controllerClassPath=deathmatch_rules::ControllerClassPath;
        original_players::options.gameStateClassPath=deathmatch_rules::GameStateClassPath;
    }
}
inline bool PrepareWorld(void* originalWorld){
    world=originalWorld;
    if(!training_world_geometry::Initialize(world))return false;
    if(!training_maps::Initialize(world,initial.mapId))return Fail(training_maps::State().error,training_maps::State().stage);
    if(!training_maps::Poll())return Fail(training_maps::State().error,training_maps::State().stage);
    return true;
}
inline bool WorldReady(){return !enabled||training_maps::Ready();}
inline bool Project(const Vec& point,Vec& projected){
    struct Query {void* world;Vec point;Vec projected{};void* nav=nullptr;void* filter=nullptr;Vec extent{160,160,800};uint8_t result=0,pad[7]{};} q{world,point};
    static_assert(sizeof(Query)==104&&offsetof(Query,result)==96);
    auto fn=engine::Find(L"/Script/NavigationSystem.NavigationSystemV1.K2_ProjectPointToNavigation");
    if(!fn||original_players::Value<uint16_t>(fn,0xbe)!=97)return false;
    auto cdo=engine::DefaultObject(engine::Class(0x4f7fff0));
    if(!engine::Event(cdo,L"/Script/NavigationSystem.NavigationSystemV1.K2_ProjectPointToNavigation",&q)||!q.result
        ||!original_players::Finite(q.projected))return false;
    projected=q.projected;return true;
}
inline Vec SpawnFeet(size_t index,uint32_t epoch,const wchar_t* heroPath){
    original_players::CharacterDefaults defaults;
    if(!original_players::ProbeDefaults(heroPath,defaults)){Fail(2611,"training_spawn_original_hero_dimensions_unavailable");return {};}
    Vec point=index?Vec{-900,(static_cast<double>(index)-2)*300,200}:Vec{-2050,0,200};
    if(index<32)spawnRouteChoice[index]=-1;
    if(!FreeForAll()&&(std::strcmp(initial.mapId,"duel1")==0||(index&&initial.routes.count))){
        int selected=-1;
        if(index&&initial.routes.count){
            double total=0;for(uint32_t i=0;i<initial.routes.count;++i)if(initial.routes.items[i].enabled&&initial.routes.items[i].weight>0)total+=initial.routes.items[i].weight;
            uint32_t sample=initial.config.seed^static_cast<uint32_t>(index*0x9e3779b9ULL)^epoch*0x85ebca6bU;
            sample^=sample>>16;sample*=0x7feb352dU;sample^=sample>>15;sample*=0x846ca68bU;sample^=sample>>16;
            double choice=(static_cast<double>(sample)+.5)/4294967296.0*total;
            for(uint32_t i=0;i<initial.routes.count;++i)if(initial.routes.items[i].enabled&&initial.routes.items[i].weight>0){
                selected=static_cast<int>(i);choice-=initial.routes.items[i].weight;if(choice<=0)break;
            }
            if(selected>=0)point=Vector(initial.routes.items[selected].start);
        }
        Vec projected{},candidate{};lastSpawnYaw=index?180:0;
        const bool supported=Project(point,projected)?training_world_geometry::Ground(projected,candidate,30,100):training_world_geometry::Ground(point,candidate);
        if(supported&&training_world_geometry::CapsuleClear(candidate,candidate,defaults.capsuleRadius,defaults.capsuleHalfHeight)){
            if(index<32)spawnRouteChoice[index]=selected;
            return candidate;
        }
    }
    if(initial.spawnCount){
        const size_t first=(index+static_cast<size_t>(epoch-1)*13+initial.config.seed)%initial.spawnCount;
        bool found=false;double best=-1;Vec bestFeet{};
        for(size_t n=0;n<initial.spawnCount;++n){const auto at=(first+n)%initial.spawnCount;const auto center=Vector(initial.spawns[at]);Vec projected{},candidate{};
            if(Project(center,projected)){if(!training_world_geometry::Ground(projected,candidate,30,100))continue;}
            else if(!training_world_geometry::Ground(center,candidate))continue;
            if(!training_world_geometry::CapsuleClear(candidate,candidate,defaults.capsuleRadius,defaults.capsuleHalfHeight))continue;
            double nearest=1e9;
            for(size_t p=0;p<32;++p)if(p!=index&&original_players::players[p].ready){
                Vec other{};if(!engine::Event(original_players::players[p].character,L"/Script/Engine.Actor.K2_GetActorLocation",&other))continue;
                const double dx=other.x-candidate.x,dy=other.y-candidate.y,dz=other.z-candidate.z;
                nearest=(std::min)(nearest,std::sqrt(dx*dx+dy*dy+dz*dz));
            }
            if(nearest>best){best=nearest;bestFeet=candidate;lastSpawnYaw=initial.spawnYaws[at];found=true;}
            if(!FreeForAll()||nearest>2500)break;
        }
        if(!found){Fail(2612,"training_spawn_requires_original_ground_and_capsule_clearance");return {};}
        point=bestFeet;
    }
    return point;
}
inline deathmatch_rules::Config RulesConfig(){
    deathmatch_rules::Config c;c.mode=FreeForAll()?deathmatch_rules::Mode::FreeForAll:deathmatch_rules::Mode::Practice;
    c.respawnDelay=initial.config.respawnSeconds;c.streakTimeout=initial.config.streakSeconds;
    c.botsReturnFire=(initial.config.flags&training_control::ReturnFire)!=0;
    c.localInvincible=(initial.config.flags&training_control::Invincible)!=0;return c;
}
inline training_bot_controller::Config BotConfig(){
    const auto& source=initial.config;training_bot_controller::Config c;
    c.mode=FreeForAll()?training_bot_controller::Mode::Deathmatch:training_bot_controller::Mode::HoldAngle;
    c.style=static_cast<training_bot_controller::Style>(source.style);c.active=matchClockStarted&&(source.flags&training_control::Paused)==0&&deathmatch_rules::Active();
    c.returnFire=FreeForAll()||(source.flags&training_control::ReturnFire)!=0;c.localInvincible=(source.flags&training_control::Invincible)!=0;
    c.peeksPerMinute=source.peeksPerMinute;c.reactionMs=source.reactionMs;c.reactionSpreadMs=source.reactionSpreadMs;
    c.aimSpeed=source.aimDegreesPerSecond;c.aimSpeedSpread=source.aimSpread;c.accuracy=source.accuracy;c.accuracySpread=source.accuracySpread;
    c.movement=source.movement;c.movementSpread=source.movementSpread;c.seed=source.seed;return c;
}
inline bool ApplyRoutes(){
    std::array<training_bot_controller::Route,64> routes{};
    for(uint32_t i=0;i<initial.routes.count;++i){const auto& source=initial.routes.items[i];auto& out=routes[i];
        out.id=source.id;strcpy_s(out.mapKey,initial.mapId);out.start=Vector(source.start);out.expose=Vector(source.expose);out.retreat=Vector(source.retreat);
        out.enabled=source.enabled!=0;out.weight=source.weight;out.style=static_cast<training_bot_controller::Style>(source.style);
    }
    return training_bot_controller::ValidPracticePlan(initial.routes)&&training_bot_controller::SetRoutes(initial.mapId,routes.data(),initial.routes.count)&&training_bot_controller::SetPracticePlan(initial.routes);
}
inline bool SelectSpawn(size_t index,uint32_t epoch,deathmatch_rules::SpawnSelection& out){
    const auto hero=original_players::players[index].classPath;
    out.position=SpawnFeet(index,epoch,hero);out.yaw=lastSpawnYaw;out.basis=deathmatch_rules::PositionBasis::Feet;
    out.originalMapAnchor=initial.spawnCount>0;return !error&&original_players::Finite(out.position);
}
inline bool PublishCharacterLinks(size_t index,void* previous,void* pawn){
    auto ps=original_players::players[index].state;
    auto type=original_players::Value<void*>(ps,0x10);
    if(!original_players::IsA(ps,original_players::ShooterStateClass)
       ||original_scope::FieldOffset(type,L"SpawnedCharacter")!=0xbc8
       ||original_scope::FieldOffset(type,L"PossessedCharacter")!=0xbd0)return false;
    for(const auto offset:{size_t(0xbc8),size_t(0xbd0)}){
        auto old=original_players::Value<void*>(ps,offset);
        if(old==pawn)continue;
        if(old&&old!=previous)return false;
        // Complete the absent local authority's actual PS -> character links,
        // then let the original replication notifications update original UI
        // models and character-change subscribers.
        if(!original_players::Write(ps,offset,pawn))return false;
        const auto name=offset==0xbc8?L"/Script/ShooterGame.ShooterPlayerState.OnRep_SpawnedCharacter":L"/Script/ShooterGame.ShooterPlayerState.OnRep_PossessedCharacter";
        auto fn=engine::Find(name);
        if(original_players::Value<uint16_t>(fn,0xbe)!=8||!engine::Event(ps,name,&old))return false;
    }
    return true;
}
inline bool BeforeRespawn(size_t index,void* oldPawn,bool local){
    if(local&&!FreeForAll()&&!original_dagger_combat::BeforeRetire(oldPawn))return false;
    if(!training_bot_controller::Unbind(index)||!original_melee_combat::BeforeRetire(index)
       ||!training_combat_runtime::BeforeRetire(index,training_combat_runtime::RetireReason::NativeDeath))return false;
    if(FreeForAll()&&!original_armor::Retire(index))return false;
    pendingCombat[index]=false;
    if(local){
        if(!skin_effect_controls::PrepareForBindingChange()||!weapon_runtime_bindings::Quiesce(original_weapon::weapon))return false;
        original_jett::ReleaseInputs();original_camera::state.bound=false;
    }
    (void)oldPawn;return true;
}
inline bool AfterSpawn(size_t index,void* oldPawn,void* newPawn,bool local){
    if(!original_inventory_slots::EnsureForPawn(newPawn)||!original_effects::EnsureActor(newPawn,local?"local_character":"enemy_character"))return false;
    if(FreeForAll()&&!original_armor::GrantHeavy(index,newPawn))return false;
    if(local){
        if(!hooks.rebindLocal||!hooks.rebindLocal(oldPawn,newPawn)||!all_weapons::OnPawnReplaced(world,oldPawn,newPawn))return false;
        if(!skin_effect_controls::Initialize(world,newPawn))return false;
        if(!FreeForAll()&&!original_dagger_combat::Bind(newPawn,original_jett::skills[3].actor,deathmatch_rules::Player(index)->spawnEpoch))return false;
    }else{
        original_enemy_weapon::entries[index]={};
        if(!original_npc_locality::Refresh(index,deathmatch_rules::Player(index)->spawnEpoch))
            return Fail(original_npc_locality::State().error,original_npc_locality::State().stage);
        if(!original_camera_owners::Apply(index)||!original_enemy_view::Apply(index)||!original_enemy_weapon::SpawnAK(index))return false;
    }
    if(!PublishCharacterLinks(index,oldPawn,newPawn))return false;
    const auto p=deathmatch_rules::Player(index);
    if(!training_bot_controller::Bind(index,newPawn,p?p->spawnEpoch:1,!local))return false;
    if(!FreeForAll()&&!local&&spawnRouteChoice[index]>=0
       &&!training_bot_controller::SetSpawnRouteHint(index,p?p->spawnEpoch:1,static_cast<size_t>(spawnRouteChoice[index])))return false;
    if(FreeForAll()&&!deathmatch_pickups::ApplySpawnProtection(index))return false;
    pendingCombat[index]=true;return true;
}
inline bool Quiesce(void* oldWeapon){
    if(!original_melee_combat::BeforeRetire(0)||!training_combat_runtime::BeforeRetire(0)||!skin_effect_controls::PrepareForBindingChange())return false;
    return weapon_runtime_bindings::Quiesce(oldWeapon);
}
inline bool AfterBind(void*,void* pawn,void* weapon,const all_weapons::Definition&){
    const auto p=deathmatch_rules::Player(0);
    if(!training_combat_runtime::Bind(0,pawn,weapon,p?p->spawnEpoch:1))return false;
    if(!original_melee_combat::Bind(0,pawn,weapon,p?p->spawnEpoch:1))return false;
    if(weapon_runtime_bindings::State().firearm&&!original_infinite_ammo::Initialize(world,original_weapon::ammo,
        (initial.config.flags&training_control::InfiniteAmmo)!=0))return false;
    pendingCombat[0]=false;return true;
}
inline bool WeaponChanged(void* oldWeapon,void* newWeapon,const all_weapons::Definition& definition){
    if(!all_skins::OnWeaponChanged(oldWeapon,newWeapon,definition)||!weapon_runtime_bindings::Bind(oldWeapon,newWeapon,definition))return false;
    const auto skin=all_skins::CurrentBinding();
    return skin&&skin_effect_controls::OnWeaponChanged(*skin);
}
inline bool RetireWeapon(void* weapon){return all_skins::BeforeRetire(weapon)&&weapon_runtime_bindings::BeforeRetire(weapon);}
inline bool Visible(size_t source,size_t target,const Vec&,const Vec&){return training_combat_runtime::Visible(source,target);}
inline bool BotWeaponInput(size_t index,bool fire,bool reload){
    // Original CurrentWeapon/physics/input readiness is asynchronous after a
    // normal native grant. Keep moving; never invent a shot before it is ready.
    if(!training_combat_runtime::Ready(index))return true;
    return training_combat_runtime::WeaponInput(index,fire,reload);
}
inline bool EndOriginalGameplay(double afterSeconds){
    auto gs=original_players::createdGameState;auto clock=engine::function<void*(*)(void*)>(0x787b6c0)(original_players::activePawn);
    if(!gs||!clock)return false;
    const float time=engine::function<float(*)(void*)>(0x789df70)(clock);
    const float end=time+static_cast<float>(afterSeconds);
    if(!std::isfinite(end))return false;
    std::array<uint8_t,original_gameplay::GameInfoBytes> previous{};
    std::memcpy(previous.data(),static_cast<uint8_t*>(gs)+original_gameplay::GameInfoOffset,previous.size());
    return original_players::Write(gs,original_gameplay::AuthEnd,end)
        &&original_gameplay::Event(gs,L"/Script/ShooterGame.ShooterGameState.OnRep_GameInfo",64,previous.data());
}
inline void MatchEnded(int32_t,deathmatch_rules::EndReason){
    EndOriginalGameplay(0);
    if(FreeForAll()&&!deathmatch_pickups::Stop())Fail(deathmatch_pickups::State().error?deathmatch_pickups::State().error:2622,deathmatch_pickups::State().stage);
}
inline bool GameplayCanDamage(size_t,size_t victim){return !FreeForAll()||deathmatch_pickups::CanDamage(victim);}
inline void WeaponUsed(size_t index){
    if(FreeForAll()&&!deathmatch_pickups::OnWeaponUsed(index))Fail(deathmatch_pickups::State().error?deathmatch_pickups::State().error:2623,"deathmatch_original_fire_did_not_release_spawn_protection");
}
inline void AfterScoreCommit(uint64_t token,size_t source,size_t victim){
    if(!training_champion_state::Poll(world,true))Fail(training_champion_state::State().error,training_champion_state::State().stage);
    if(FreeForAll()&&!deathmatch_pickups::OnKill(source,victim,token))Fail(deathmatch_pickups::State().error?deathmatch_pickups::State().error:2624,deathmatch_pickups::State().stage);
}
inline uint64_t MeleeToken(){return ++training_combat_runtime::nextDamageToken;}
inline bool MeleeDeath(size_t index){return training_combat_runtime::BeforeRetire(index,training_combat_runtime::RetireReason::NativeDeath);}
inline bool Initialize(const Hooks& callbacks){
    if(!enabled||!WorldReady()||!original_players::Thread())return false;
    hooks=callbacks;participants=FreeForAll()?initial.config.totalPlayers:2;
    if(!all_weapons::Initialize(world,original_players::activePawn)||!all_skins::Initialize()
        ||!skin_effect_controls::Initialize(world,original_players::activePawn))return Fail(2602,"training_session_weapon_skin_services_unavailable");
    if(!original_npc_locality::Initialize(world))return Fail(original_npc_locality::State().error,original_npc_locality::State().stage);
    if(FreeForAll()){
        LARGE_INTEGER nonce{};QueryPerformanceCounter(&nonce);
        auto shuffleSeed=initial.config.seed^nonce.LowPart^static_cast<uint32_t>(nonce.HighPart)^GetCurrentProcessId()^static_cast<uint32_t>(GetTickCount64());
        if(!shuffleSeed)shuffleSeed=1;
        if(!original_enemies::BeginUniqueHeroMatch(participants,shuffleSeed))return Fail(original_enemies::error,original_enemies::stage);
    }
    all_weapons::SetHooks({Quiesce,all_skins::BeforeFinishSpawn,all_skins::PrepareWeapon,WeaponChanged,RetireWeapon});
    weapon_runtime_bindings::SetAfterBind(AfterBind);
    if(FreeForAll()){
        original_scoreboard::playerInfoClass=all_weapons::LoadClass(0x64f4540,deathmatch_rules::PlayerInfoClassPath);
        if(!original_scoreboard::playerInfoClass)return Fail(2603,"training_session_original_deathmatch_player_info_class_missing");
    }
    for(size_t i=1;i<participants;++i){
        uint32_t logical=FreeForAll()?original_enemies::AssignedLogicalId(i):static_cast<uint32_t>(i)+initial.config.seed;
        const auto hero=original_enemies::HeroFor(logical);
        const auto feet=SpawnFeet(i,1,original_enemies::HeroPaths[hero]);
        if(error||!original_enemies::Spawn(i,logical,feet,lastSpawnYaw))return Fail(2604,"training_session_original_bot_roster_or_weapon_failed");
        if(!original_npc_locality::Bind(i,original_players::players[i].controller,original_players::players[i].character,1))
            return Fail(original_npc_locality::State().error,original_npc_locality::State().stage);
        if(!original_camera_owners::Apply(i)||!original_enemy_view::Apply(i)
           ||!original_inventory_slots::EnsureForPawn(original_players::players[i].character)
           ||!original_effects::EnsureActor(original_players::players[i].character,"enemy_character")||!original_enemy_weapon::SpawnAK(i))
            return Fail(2604,"training_session_original_bot_roster_or_weapon_failed");
        pendingCombat[i]=true;
    }
    for(size_t i=0;i<participants;++i)if(!PublishCharacterLinks(i,nullptr,original_players::players[i].character))
        return Fail(2620,"training_session_original_player_state_character_links_failed");
    if(!original_audio_listeners::Initialize(world))
        return Fail(original_audio_listeners::State().error,original_audio_listeners::State().stage);
    if(FreeForAll()){
        if(!original_armor::Initialize(world))return Fail(original_armor::State().error,original_armor::State().stage);
        for(size_t i=0;i<participants;++i)if(!original_armor::GrantHeavy(i))return Fail(original_armor::State().error,original_armor::State().stage);
    }
    if(!deathmatch_rules::Initialize(world,RulesConfig(),{SelectSpawn,BeforeRespawn,AfterSpawn,MatchEnded,AfterScoreCommit,GameplayCanDamage}))
        return Fail(deathmatch_rules::State().error,deathmatch_rules::State().stage);
    if(FreeForAll()){
        if(!deathmatch_pickups::Initialize(world))return Fail(deathmatch_pickups::State().error?deathmatch_pickups::State().error:2625,deathmatch_pickups::State().stage);
        for(size_t i=0;i<participants;++i)if(!deathmatch_pickups::ApplySpawnProtection(i))return Fail(deathmatch_pickups::State().error?deathmatch_pickups::State().error:2626,deathmatch_pickups::State().stage);
    }
    if(!training_champion_state::Initialize(world)||!training_champion_state::Poll(world,true))
        return Fail(training_champion_state::State().error,training_champion_state::State().stage);
    if(!training_combat_runtime::Initialize(world))return Fail(2605,"training_session_combat_runtime_initialize_failed");
    training_combat_runtime::SetWeaponUsedCallback(WeaponUsed);
    if(!original_melee_combat::Initialize(world,{MeleeToken,MeleeDeath,WeaponUsed}))return Fail(2617,"training_session_original_melee_initialize_failed");
    if(!FreeForAll()&&(!original_dagger_combat::Initialize(world)
        ||!original_dagger_combat::Bind(original_players::activePawn,original_jett::skills[3].actor,deathmatch_rules::Player(0)->spawnEpoch)))
        return Fail(original_dagger_combat::State().error,original_dagger_combat::State().stage);
    training_bot_controller::Hooks botHooks;botHooks.visible=Visible;botHooks.weaponInput=BotWeaponInput;
    botHooks.validateSegment=training_world_geometry::ValidateSegment;
    if(!training_bot_controller::Initialize(world,botHooks)||!training_bot_controller::Configure(BotConfig())||!ApplyRoutes())
        return Fail(2606,"training_session_original_bot_input_or_routes_failed");
    std::array<Vec,64> anchors{};for(uint32_t i=0;i<initial.spawnCount;++i){auto center=Vector(initial.spawns[i]);if(!Project(center,anchors[i]))anchors[i]=SpawnFeet(i,1,original_enemies::HeroPaths[0]);}
    if(!training_bot_controller::SetAnchors(anchors.data(),initial.spawnCount))return false;
    for(size_t i=0;i<participants;++i)if(!training_bot_controller::Bind(i,original_players::players[i].character,1,i!=0))return Fail(2607,"training_session_bot_input_binding_failed");
    if(!FreeForAll())for(size_t i=1;i<participants;++i)if(spawnRouteChoice[i]>=0
        &&!training_bot_controller::SetSpawnRouteHint(i,1,static_cast<size_t>(spawnRouteChoice[i])))return Fail(2627,"training_session_spawn_route_hint_failed");
    if(!training_bot_controller::ValidateRoutes(static_cast<double>(GetTickCount64())/1000.0))return Fail(2621,"training_session_route_validation_unavailable");
    if(!original_hud::InstallOriginalHUD(original_players::activeController,original_players::activePawn)
       ||!original_camera::RestoreWatchPossessedLifecycle())return Fail(2608,"training_session_original_hud_lifecycle_failed");
    if(!original_deathmatch_topbar::SetInspectionCadence(testActionsAllowed?1000:0)
       ||!UpdateOriginalDeathmatchTopbar(true))return false;
    if(FreeForAll()&&(!original_streak_hud::SetInspectionCadence(testActionsAllowed?100:1000)
       ||!original_streak_hud::Initialize(world)))return Fail(original_streak_hud::State().error,original_streak_hud::State().stage);
    original_buy_menu::inputManagedExternally=true;
    if(!original_buy_menu::Initialize(world,original_players::activeController))return Fail(original_buy_menu::State().error,original_buy_menu::State().stage);
    initialized=true;stage="training_session_equipping_original_three_slot_loadout";
    return all_weapons::RestoreLoadout(all_weapons::DefaultLoadout());
}

inline training_control::Result Dispatch(training_control::Command command,const uint8_t* data,uint32_t count,char* message,size_t capacity){
    using Result=training_control::Result;
    const auto reply=[&](Result result,const char* text){strncpy_s(message,capacity,text,_TRUNCATE);commandStage=text;return result;};
    if(static_cast<uint32_t>(command)==19){
        if(!testActionsAllowed||!ready)return reply(Result::Unavailable,"sound_fixture_requires_ready_explicit_stage9");
        return sound_locator_fixture::Dispatch(data,count,message,capacity,world,original_players::activeController);
    }
    if(static_cast<uint32_t>(command)==20){
        if(!testActionsAllowed||!ready)return reply(Result::Unavailable,"sound_nav_export_requires_ready_explicit_stage9");
        return sound_locator_nav_export::Dispatch(data,count,message,capacity,world,original_players::activePawn,initial.mapId);
    }
    if(!initialized)return reply(Result::Unavailable,"original_session_not_ready");
    if(command==training_control::Command::Configure){
        training_control::Configuration c;if(count!=sizeof(c))return reply(Result::Invalid,"invalid_configuration_size");std::memcpy(&c,data,sizeof(c));
        if(!Valid(c))return reply(Result::Invalid,"invalid_configuration_values");
        if(c.mode!=initial.config.mode||c.totalPlayers!=initial.config.totalPlayers)return reply(Result::Unavailable,"map_mode_population_change_requires_original_session_restart");
        initial.config=c;
        if(!deathmatch_rules::SetConfig(RulesConfig())||!training_bot_controller::Configure(BotConfig()))return reply(Result::Failed,"configuration_native_rule_update_failed");
        if(original_infinite_ammo::State().initialized&&!original_infinite_ammo::SetEnabled((c.flags&training_control::InfiniteAmmo)!=0))return reply(Result::Failed,"original_infinite_ammo_rule_update_failed");
        return reply(Result::Applied,"configuration_applied");
    }
    if(command==training_control::Command::Routes){
        if(count<8)return reply(Result::Invalid,"routes_header_missing");
        uint32_t routeCount=0;std::memcpy(&routeCount,data,4);
        if(routeCount>64||(count!=8+routeCount*sizeof(training_control::Route)&&count!=sizeof(training_control::RouteSet)))return reply(Result::Invalid,"invalid_routes_length");
        training_control::RouteSet next{};std::memcpy(&next,data,count);
        for(uint32_t i=0;i<next.count;++i)if(!next.items[i].id||next.items[i].style>8||next.items[i].enabled>1
            ||!std::isfinite(next.items[i].weight)||next.items[i].weight<.1||next.items[i].weight>100
            ||!original_players::Finite(Vector(next.items[i].start))||!original_players::Finite(Vector(next.items[i].expose))||!original_players::Finite(Vector(next.items[i].retreat)))
            return reply(Result::Invalid,"invalid_route_point_or_options");
        const auto previous=initial.routes;initial.routes=next;
        if(!ApplyRoutes()){initial.routes=previous;return reply(Result::Invalid,"route_native_controller_rejected");}
        if(!training_bot_controller::ValidateRoutes(static_cast<double>(GetTickCount64())/1000.0))return reply(Result::Failed,"route_validation_reset_failed");
        return reply(Result::Applied,"routes_saved_navigation_checked_when_selected");
    }
    if(command==training_control::Command::Weapon){
        if(count!=16)return reply(Result::Invalid,"invalid_weapon_guid_size");Guid id;std::memcpy(&id,data,16);
        auto d=all_weapons::Find(id);if(!d||!d->identityVerified)return reply(Result::Invalid,"weapon_not_in_original_content_index");
        pendingWeapon=id;hasPendingWeapon=true;return reply(Result::Pending,"original_weapon_switch_queued");
    }
    if(command==training_control::Command::Skin||command==training_control::Command::SaveSkinSelection){
        if(count!=sizeof(pendingSkin))return reply(Result::Invalid,"invalid_skin_selection_size");
        training_control::Skin selected;std::memcpy(&selected,data,sizeof(selected));
        if(!all_skins::SetSelection(Id(selected.weapon),Id(selected.skin),Id(selected.level),Id(selected.chroma)))return reply(Result::Invalid,all_skins::State().stage);
        if(command==training_control::Command::SaveSkinSelection)return reply(Result::Applied,"original_skin_selection_saved");
        pendingSkin=selected;hasPendingSkin=true;return reply(Result::Pending,"original_skin_loadout_validated_switch_queued");
    }
    if(command==training_control::Command::Effect){
        training_control::Effect effect;if(count!=sizeof(effect))return reply(Result::Invalid,"invalid_effect_size");std::memcpy(&effect,data,count);
        if(effect.key[79]||effect.mode>2||!std::isfinite(effect.value))return reply(Result::Invalid,"invalid_effect_parameters");
        return skin_effect_controls::SetOverride(effect.key,effect.mode,effect.value)?reply(Result::Pending,"original_effect_override_submitted"):reply(Result::Unavailable,skin_effect_controls::State().commandResult);
    }
    if(command==training_control::Command::RestartMatch){restartRequested=true;return reply(Result::Pending,"original_session_restart_requested");}
    if(command==training_control::Command::ValidateSkinCatalog){
        if(count)return reply(Result::Invalid,"skin_catalog_validation_has_no_payload");
        return all_skins::ValidateCatalog(skinCatalogValidation)?reply(Result::Applied,skinCatalogValidation.stage):reply(Result::Failed,skinCatalogValidation.stage);
    }
    if(command==training_control::Command::ResolveMapPoint){
        if(count!=sizeof(training_control::Point))return reply(Result::Invalid,"invalid_map_point_size");
        training_control::Point input;std::memcpy(&input,data,count);auto source=Vector(input);
        if(!original_players::Finite(source)||std::abs(source.x)>1000000||std::abs(source.y)>1000000||std::abs(source.z)>1000000)
            return reply(Result::Invalid,"invalid_map_point_coordinates");
        Vec projected{},feet{};const bool nav=Project(source,projected);
        const bool ground=nav?training_world_geometry::Ground(projected,feet,30,100):training_world_geometry::Ground(source,feet);
        original_players::CharacterDefaults dimensions;
        if(!ground||!original_players::ProbeDefaults(original_players::players[0].classPath,dimensions)
           ||!training_world_geometry::CapsuleClear(feet,feet,dimensions.capsuleRadius,dimensions.capsuleHalfHeight))
            return reply(Result::Unavailable,"map_point_has_no_original_ground_or_capsule_clearance");
        snprintf(message,capacity,"{\"position\":[%.5f,%.5f,%.5f],\"navigation\":%s,\"ground\":true,\"capsule\":true}",feet.x,feet.y,feet.z,nav?"true":"false");
        commandStage="original_map_point_ground_and_capsule_verified";return Result::Applied;
    }
    if(command==training_control::Command::ToggleBuyMenu){
        if(count)return reply(Result::Invalid,"toggle_buy_menu_has_no_payload");
        return original_buy_menu::Toggle()?reply(Result::Applied,"original_buy_menu_toggled"):reply(Result::Failed,original_buy_menu::State().stage);
    }
    if(command==training_control::Command::ToggleOptions){
        if(count)return reply(Result::Invalid,"toggle_options_has_no_payload");
        return original_hud::ToggleOriginalOptions()?reply(Result::Applied,"original_options_toggled"):reply(Result::Failed,original_hud::stage);
    }
    if(command==training_control::Command::CloseActiveUI){
        if(count)return reply(Result::Invalid,"close_active_ui_has_no_payload");
        return original_ui_input_mode::CloseActiveUI()?reply(Result::Applied,"original_active_ui_closed"):reply(Result::Failed,original_ui_input_mode::State().stage);
    }
    if(command==training_control::Command::TestAudioRegistration){
        if(!testActionsAllowed)return reply(Result::Unavailable,"audio_registration_fixture_requires_explicit_stage9");
        if(count!=4)return reply(Result::Invalid,"audio_registration_fixture_requires_npc_index");
        uint32_t index=0;std::memcpy(&index,data,4);
        if(!index||index>=participants||!original_players::players[index].ready)
            return reply(Result::Invalid,"audio_registration_fixture_target_not_registered");
        if(!original_audio_listeners::Sample())return reply(Result::Failed,original_audio_listeners::State().stage);
        const auto before=original_audio_listeners::State().current;
        if(!before.valid||before.own!=1||before.nonLocalPC)return reply(Result::Unavailable,"audio_registration_fixture_waits_for_reconciled_registry");
        const auto& entry=original_players::players[index];auto pc=entry.controller;
        auto camera=original_players::Value<void*>(pc,0x528);
        if(entry.local||original_players::Value<void*>(pc,0x510)||!original_npc_locality::Find(pc)
           ||original_players::Value<void*>(camera,0x468)!=pc||original_players::Value<void*>(camera,0x210)!=pc)
            return reply(Result::Invalid,"audio_registration_fixture_nonlocal_camera_pair_required");
        auto table=original_players::Value<void**>(pc,0);auto predicate=original_players::Value<void*>(table,0x800);
        if(!predicate||!reinterpret_cast<bool(*)(void*)>(predicate)(pc))
            return reply(Result::Failed,"audio_registration_fixture_simulation_locality_not_preserved");
        const auto blockedBefore=original_npc_locality::State().listenerRegistrationRejects;
        engine::function<void(*)(void*,void*)>(0x596d210)(original_audio_listeners::State().device,camera);
        if(!original_audio_listeners::Sample())return reply(Result::Failed,original_audio_listeners::State().stage);
        const auto& after=original_audio_listeners::State().current;
        bool preserved=after.valid&&after.own==1&&!after.nonLocalPC&&after.count==before.count;
        for(size_t i=0;i<before.count;++i)if(!original_audio_listeners::Contains(after,before.rows[i].identity))preserved=false;
        if(!preserved)return reply(Result::Failed,"audio_registration_fixture_registry_changed");
        return original_npc_locality::State().listenerRegistrationRejects>blockedBefore
            ?reply(Result::Applied,"original_audio_registration_view_gate_exercised_simulation_preserved")
            :reply(Result::Pending,"original_audio_registration_worker_eligibility_not_reached");
    }
    if(command==training_control::Command::TestPlayerNearBot){
        if(!testActionsAllowed)return reply(Result::Unavailable,"pose_fixture_requires_explicit_stage9");
        const auto local=deathmatch_rules::Player(0);
        if(count!=4||!local||!local->alive||!local->ready)return reply(Result::Invalid,"pose_fixture_requires_living_local_player_and_target_index");
        uint32_t targetIndex=0;std::memcpy(&targetIndex,data,4);
        const auto target=deathmatch_rules::Player(targetIndex);
        if(targetIndex==0||!target||!target->registered||!target->alive)return reply(Result::Unavailable,"pose_fixture_target_not_alive");
        Vec targetRoot{};if(!engine::Event(target->pawn,L"/Script/Engine.Actor.K2_GetActorLocation",&targetRoot))return reply(Result::Failed,"pose_fixture_target_location_unavailable");
        original_players::CharacterDefaults dimensions;
        if(!original_players::ProbeDefaults(original_players::players[0].classPath,dimensions))return reply(Result::Failed,"pose_fixture_original_capsule_unavailable");
        const auto teleport=engine::Find(L"/Script/Engine.Actor.K2_TeleportTo");
        if(original_players::Value<uint16_t>(teleport,0xbe)!=49)return reply(Result::Failed,"pose_fixture_original_teleport_abi_changed");
        struct Teleport {Vec location{},rotation{};uint8_t result=0,padding[7]{};} parameters;
        static_assert(sizeof(Teleport)==56&&offsetof(Teleport,result)==48);
        constexpr double tau=6.28318530717958647692;
        const uint32_t first=testPoseSequence++;
        for(uint32_t step=0;step<16;++step){const double angle=((first+step)%8)*tau/8;
            const double radius=step<8?350.:550.;Vec nearbyPoint=targetRoot;nearbyPoint.x+=radius*std::cos(angle);nearbyPoint.y+=radius*std::sin(angle);
            Vec projected{},feet{};if(!Project(nearbyPoint,projected)||!training_world_geometry::Ground(projected,feet)
                ||!training_world_geometry::CapsuleClear(feet,feet,dimensions.capsuleRadius,dimensions.capsuleHalfHeight))continue;
            parameters.location=feet;parameters.location.z+=dimensions.capsuleHalfHeight+2;
            parameters.rotation.y=std::atan2(targetRoot.y-feet.y,targetRoot.x-feet.x)*360/tau;parameters.result=0;
            if(engine::Event(original_players::activePawn,L"/Script/Engine.Actor.K2_TeleportTo",&parameters)&&parameters.result)
                return reply(Result::Applied,"pose_fixture_original_teleport_ground_and_capsule_verified");
        }
        return reply(Result::Unavailable,"pose_fixture_no_original_valid_ground_near_target");
    }
    if(command==training_control::Command::BuyWeapon){
        if(count!=16)return reply(Result::Invalid,"invalid_purchase_guid_size");Guid id;std::memcpy(&id,data,16);
        if(!original_buy_menu::State().open&&!original_buy_menu::Toggle())return reply(Result::Failed,original_buy_menu::State().stage);
        if(!original_buy_menu::SampleModels())return reply(Result::Failed,"original_buy_menu_models_unavailable");
        const auto& shop=original_buy_menu::State();
        for(uint32_t i=0;i<shop.observedModelCount;++i)if(shop.models[i].catalogMatched&&all_weapons::Same(shop.models[i].id,id)){
            auto fn=engine::Find(L"/Script/ShooterGame.ShopPurchasableModel.TryPurchase");uint8_t args[2]{};
            if(original_players::Value<uint16_t>(fn,0xbe)!=2||!engine::Event(shop.models[i].object,L"/Script/ShooterGame.ShopPurchasableModel.TryPurchase",args))return reply(Result::Failed,"original_buy_model_purchase_dispatch_failed");
            return reply(Result::Pending,"original_buy_model_purchase_submitted");
        }
        return reply(Result::Unavailable,"original_buy_menu_has_no_matching_model");
    }
    if(command==training_control::Command::ValidateRoutes){
        if(!ApplyRoutes()||!training_bot_controller::ValidateRoutes(static_cast<double>(GetTickCount64())/1000.0))
            return reply(Result::Failed,"route_validation_reset_failed");
        return reply(Result::Pending,"route_validation_requested");
    }
    return reply(Result::Invalid,"unknown_control_command");
}

struct Json {
    std::string text;
    void Add(const char* format,...){char buffer[2048]{};va_list args;va_start(args,format);const int size=vsnprintf(buffer,sizeof(buffer),format,args);va_end(args);if(size>0&&size<static_cast<int>(sizeof(buffer)))text.append(buffer,static_cast<size_t>(size));}
    void String(const char* value){text+='"';for(auto p=reinterpret_cast<const unsigned char*>(value?value:"");*p;++p){if(*p=='"'||*p=='\\'){text+='\\';text+=static_cast<char>(*p);}else if(*p<32)Add("\\u%04x",static_cast<unsigned>(*p));else text+=static_cast<char>(*p);}text+='"';}
    void Vector(const Vec& p){Add("[%.4f,%.4f,%.4f]",p.x,p.y,p.z);}
    void GuidValue(const Guid& id){const auto p=reinterpret_cast<const uint32_t*>(&id);Add("\"%08x%08x%08x%08x\"",p[0],p[1],p[2],p[3]);}
};
inline void Publish(uint64_t now){
    if(now-lastPublish<100)return;lastPublish=now;
    Json out;out.Add("{\"ready\":%s,\"mode\":\"%s\",\"map_id\":",ready?"true":"false",FreeForAll()?"deathmatch":"practice");out.String(initial.mapId);
    out.Add(",\"stage\":");out.String(stage);out.Add(",\"error\":");if(error)out.String(stage);else out.Add("null");
    const auto& rules=deathmatch_rules::State();
    out.Add(",\"ended\":%s,\"winner\":%d,\"end_reason\":%u,\"time_remaining\":%.3f,\"infinite_ammo\":%s,\"players\":[",rules.ended?"true":"false",rules.winner,static_cast<unsigned>(rules.endReason),deathmatch_rules::Remaining(),(initial.config.flags&training_control::InfiniteAmmo)?"true":"false");
    bool comma=false;
    const auto writePlayer=[&](size_t i,bool includeName){
        const auto p=deathmatch_rules::Player(i);const auto& b=training_bot_controller::Player(i);
        float shield=0;engine::Event(p->pawn,L"/Script/ShooterGame.ShooterCharacter.GetShield",&shield);
        float protection=0;if(FreeForAll())deathmatch_pickups::ReadProtection(i,protection);
        Vec feet=b.observed.feet;if(!b.observed.ready){engine::Event(p->pawn,L"/Script/Engine.Actor.K2_GetActorLocation",&feet);auto capsule=original_players::Value<void*>(p->pawn,0x500);feet.z-=original_players::Value<float>(capsule,0x6d8);}
        out.Add("{\"index\":%zu,\"bot\":%s,\"alive\":%s,\"health\":%.3f,\"shield\":%.3f,\"spawn_protected\":%s,\"kills\":%u,\"deaths\":%u,\"streak\":%u,\"epoch\":%u,\"feet\":",i,p->local?"false":"true",p->alive?"true":"false",p->health,shield,protection>0?"true":"false",p->matchKills,p->matchDeaths,p->streak,p->spawnEpoch);out.Vector(feet);
        if(includeName){out.Add(",\"name\":");const auto name=original_players::Utf8(original_players::players[i].requestedName);out.String(name.c_str());}
        out.Add(",\"hero_class\":");const auto heroClass=original_players::Utf8(original_players::players[i].classPath);out.String(heroClass.c_str());
        out.Add(",\"phase\":");out.String(b.reason);out.Add(",\"profile\":{\"reaction_ms\":%.3f,\"aim_speed\":%.3f,\"accuracy\":%.5f,\"movement\":%.5f}}",b.profile.reaction*1000,b.profile.turnSpeed,b.profile.accuracy,b.profile.movement);
    };
    for(size_t i=0;i<32;++i){const auto p=deathmatch_rules::Player(i);if(!p||!p->registered)continue;if(comma)out.Add(",");comma=true;writePlayer(i,true);}
    out.Add("],\"player\":");const auto local=deathmatch_rules::Player(0);if(local&&local->registered)writePlayer(0,false);else out.Add("null");
    out.Add(",\"weapon\":{\"ready\":%s,\"stage\":",all_weapons::State().ready?"true":"false");out.String(all_weapons::State().stage);out.Add(",\"id\":");out.GuidValue(all_weapons::State().currentGuid);
    if(const auto current=all_weapons::CurrentDefinition()){out.Add(",\"name\":");out.String(current->label);}
    if(const auto skin=all_skins::CurrentBinding()){out.Add(",\"skin_id\":");out.GuidValue(skin->skinId);out.Add(",\"level_id\":");out.GuidValue(skin->levelId);out.Add(",\"chroma_id\":");out.GuidValue(skin->chromaId);}
    out.Add("},\"routes\":[");
    for(uint32_t i=0;i<initial.routes.count;++i){if(i)out.Add(",");const auto& r=initial.routes.items[i];const char* status="pending";const char* reason="waiting_for_original_path_validation";
        if(const auto check=training_bot_controller::RouteValidationAt(i)){status=training_bot_controller::RouteCheckStatusName(check->status);reason=check->reason;}
        out.Add("{\"id\":%u,\"status\":",r.id);out.String(status);out.Add(",\"message\":");out.String(reason);out.Add("}");
    }
    const auto& combat=training_combat_runtime::State();
    out.Add("],\"combat\":{\"native_rays\":%llu,\"damage_events\":%llu,\"kills\":%llu,\"sources\":%u},\"effects\":[",static_cast<unsigned long long>(combat.nativeRays),static_cast<unsigned long long>(combat.damageEvents),static_cast<unsigned long long>(combat.kills),combat.boundSources);
    for(size_t i=0;i<skin_effect_controls::State().controls;++i){if(i)out.Add(",");const auto& c=skin_effect_controls::controls[i];out.Add("{\"key\":");out.String(c.key);out.Add(",\"label\":");out.String(c.label);out.Add(",\"available\":%s,\"supported\":%s,\"requested\":%u,\"applied\":%u,\"verified\":%s,\"description\":",c.available?"true":"false",c.supported?"true":"false",static_cast<unsigned>(c.requested),static_cast<unsigned>(c.applied),c.appliedVerified?"true":"false");out.String(c.reason);out.Add(",\"numeric\":%s,\"value\":%.9g,\"minimum\":%.9g,\"maximum\":%.9g",c.numeric?"true":"false",c.value,c.minimum,c.maximum);out.Add("}");}
    out.Add("]}");training_control::Publish(out.text.c_str(),out.text.size(),initial.config.revision,ready?1U:0U);
}
inline bool OriginalAbilityCurrent(){
    if(FreeForAll()||!original_jett::State().loadoutGranted
        ||original_jett::State().pawn!=original_players::activePawn)return false;
    const auto current=all_weapons::CurrentWeapon();
    for(const auto& skill:original_jett::skills)
        if(current&&skill.actor==current&&original_players::Value<void*>(current,0x4a8)==original_players::activePawn)return true;
    return false;
}
inline bool Poll(double seconds){
    if(!initialized)return false;
    training_performance::Scope timing(training_performance::Slot::Rules);
    training_control::Poll(Dispatch);
    if(!deathmatch_rules::Poll(seconds))return Fail(deathmatch_rules::State().error,deathmatch_rules::State().stage);
    if(FreeForAll()&&!original_timed_streak::SampleDiagnostics())return Fail(original_timed_streak::State().error,original_timed_streak::State().stage);
    timing.Step(training_performance::Slot::OtherSession);
    const auto localityNow=GetTickCount64();
    if(localityNow-lastLocalitySample>=1000){
        lastLocalitySample=localityNow;
        if(!original_npc_locality::Sample())return Fail(original_npc_locality::State().error,original_npc_locality::State().stage);
    }
    if(!original_audio_listeners::Poll())
        return Fail(original_audio_listeners::State().error,original_audio_listeners::State().stage);
    if(FreeForAll()&&!deathmatch_pickups::Poll(seconds))return Fail(deathmatch_pickups::State().error?deathmatch_pickups::State().error:2628,deathmatch_pickups::State().stage);
    if(!all_weapons::Tick())return Fail(all_weapons::State().error,all_weapons::State().stage);
    if(!all_skins::Tick())return Fail(all_skins::State().error,all_skins::State().stage);
    if(!all_weapons::State().busy&&hasPendingWeapon){if(all_weapons::Equip(pendingWeapon))hasPendingWeapon=false;}
    if(!all_weapons::State().busy&&hasPendingSkin){
        const auto d=all_weapons::CurrentDefinition();
        if(d&&all_weapons::Same(d->id,Id(pendingSkin.weapon))){if(!all_skins::ApplySkin(all_weapons::CurrentWeapon(),Id(pendingSkin.weapon),Id(pendingSkin.skin),Id(pendingSkin.level),Id(pendingSkin.chroma)))return Fail(all_skins::State().error,all_skins::State().stage);}
        hasPendingSkin=false;
    }
    for(size_t i=1;i<participants;++i)if(pendingCombat[i]){
        auto inventory=original_players::players[i].inventory;auto weapon=original_players::Value<void*>(inventory,0x280);
        if(weapon&&training_combat_runtime::Bind(i,original_players::players[i].character,weapon,deathmatch_rules::Player(i)->spawnEpoch)){
            if(!original_melee_combat::Bind(i,original_players::players[i].character,weapon,deathmatch_rules::Player(i)->spawnEpoch))return Fail(2618,"training_session_original_melee_binding_failed");
            pendingCombat[i]=false;
        }
    }
    timing.Step(training_performance::Slot::Combat);
    if(!training_combat_runtime::Poll(seconds))return Fail(training_combat_runtime::State().error,training_combat_runtime::State().stage);
    timing.Step(training_performance::Slot::Topbar);
    if(!UpdateOriginalDeathmatchTopbar())return false;
    timing.Step(training_performance::Slot::StreakHUD);
    if(FreeForAll()&&!original_streak_hud::Poll())return Fail(original_streak_hud::State().error,original_streak_hud::State().stage);
    timing.Step(training_performance::Slot::OtherSession);
    if(!original_melee_combat::Poll(seconds))return Fail(original_melee_combat::State().error,original_melee_combat::State().stage);
    if(error)return false;
    if(!training_champion_state::Poll(world))return Fail(training_champion_state::State().error,training_champion_state::State().stage);
    timing.Step(training_performance::Slot::Bots);
    if(!training_bot_controller::Configure(BotConfig())||!training_bot_controller::Tick(seconds))return Fail(training_bot_controller::State().error,training_bot_controller::State().stage);
    timing.Step(training_performance::Slot::OtherSession);
    if(!skin_effect_controls::Poll())return Fail(skin_effect_controls::State().error,skin_effect_controls::State().stage);
    if(!kill_feedback_view::Poll())return Fail(kill_feedback_view::error,kill_feedback_view::stage);
    const bool heldReady=all_weapons::State().ready||(OriginalAbilityCurrent()
        &&weapon_runtime_bindings::State().ready&&training_combat_runtime::Ready(0));
    ready=heldReady&&!hasPendingWeapon&&!all_weapons::State().busy&&(!FreeForAll()||deathmatch_pickups::Ready());
    if(ready&&!matchClockStarted){
        matchClockStarted=true;
        deathmatch_rules::diagnostics.startedAt=seconds;deathmatch_rules::diagnostics.remaining=540;
        if(FreeForAll()&&!EndOriginalGameplay(540))return Fail(2619,"training_session_original_match_timer_notification_failed");
    }
    stage=deathmatch_rules::State().ended?"training_session_match_ended":ready?"training_session_running_original_gameplay":"training_session_waiting_original_equip";
    Publish(GetTickCount64());return true;
}
inline bool LocalAlive(){const auto p=deathmatch_rules::Player(0);return p&&p->registered&&p->alive&&p->ready&&p->phase==deathmatch_rules::Phase::Alive;}
inline void WriteDiagnostics(const std::wstring& directory){
    // Explicit stage9 verification retains complete one-second checkpoints.
    // Play sessions spread observational files across frames: at most one every
    //100ms. UI/game state for the WebUI is independently published through IPC.
    // Diagnostic snapshots retain atomic replacement without forcing the disk
    // cache to stable storage on the game thread.
    const bool complete=testActionsAllowed;
    const auto now=GetTickCount64();if(now-lastDiagnostics<(complete?1000U:100U))return;lastDiagnostics=now;
    static uint32_t next=0;uint32_t at=0;
    const auto write=[&](bool applicable,auto writer,const wchar_t* name){
        if(!applicable)return;
        if(complete||at==next)(void)writer((directory+name).c_str());++at;
    };
    write(true,all_weapons::WriteDiagnostics,L"all-weapons.json");
    write(true,all_skins::WriteDiagnostics,L"all-skins.json");
    write(true,weapon_runtime_bindings::WriteDiagnostics,L"weapon-runtime-bindings.json");
    write(true,skin_effect_controls::WriteDiagnostics,L"skin-effect-controls.json");
    write(true,training_bot_controller::WriteDiagnostics,L"training-bot-controller.json");
    write(true,training_combat_runtime::WriteDiagnostics,L"training-combat-runtime.json");
    write(true,deathmatch_rules::WriteDiagnostics,L"deathmatch-rules.json");
    write(true,original_buy_menu::WriteDiagnostics,L"original-buy-menu.json");
    write(FreeForAll(),original_timed_streak::WriteDiagnostics,L"original-timed-streak.json");
    write(FreeForAll(),original_streak_effect_authority::WriteDiagnostics,L"original-streak-effect-authority.json");
    write(FreeForAll(),original_streak_hud::WriteDiagnostics,L"original-streak-hud.json");
    write(true,kill_feedback_view::WriteDiagnostics,L"kill-feedback-view.json");
    write(true,training_maps::WriteDiagnostics,L"training-maps.json");
    write(true,training_player_roles_probe::WriteDiagnostics,L"training-player-roles.json");
    write(true,original_npc_locality::WriteDiagnostics,L"original-npc-locality.json");
    write(true,original_audio_listeners::WriteDiagnostics,L"original-audio-listeners.json");
    write(FreeForAll(),original_deathmatch_topbar::WriteDiagnostics,L"original-deathmatch-topbar.json");
    write(FreeForAll(),original_heroes::WriteDiagnostics,L"original-heroes.json");
    write(training_local_posture_probe::State().enabled,training_local_posture_probe::WriteDiagnostics,L"training-local-posture-probe.json");
    write(FreeForAll(),original_armor::WriteDiagnostics,L"original-armor.json");
    write(true,original_melee_combat::WriteDiagnostics,L"original-melee-combat.json");
    write(true,training_champion_state::WriteDiagnostics,L"training-champion-state.json");
    write(skinCatalogValidation.complete||skinCatalogValidation.failures,
        [](const wchar_t* path){(void)all_skins::WriteCatalogValidation(path,skinCatalogValidation);},L"skin-catalog-validation.json");
    write(FreeForAll(),deathmatch_pickups::WriteDiagnostics,L"deathmatch-pickups.json");
    write(FreeForAll(),original_recovery::WriteDiagnostics,L"original-recovery.json");
    write(!FreeForAll(),original_jett::WriteDiagnostics,L"original-jett.json");
    write(!FreeForAll(),original_jett_world::WriteDiagnostics,L"original-jett-world.json");
    write(!FreeForAll(),original_jett_sync::WriteDiagnostics,L"original-jett-sync.json");
    write(!FreeForAll(),original_jett_priming::WriteDiagnostics,L"original-jett-priming.json");
    write(!FreeForAll(),original_dagger_combat::WriteDiagnostics,L"original-dagger-combat.json");
    if(!complete)next=at?(next+1)%at:0;
}
}
