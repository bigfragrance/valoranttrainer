#pragma once
#include "original_hud.h"
#include "local_server_mode_startup.h"
#include "original_players.h"
#include "original_content.h"
#include "original_content_services.h"
#include "original_render.h"
#include "original_effects.h"
#include "original_inventory_slots.h"
#include "original_input_bindings.h"
#include "local_server_services.h"
#include "deathmatch_rules.h"
#include "local_server_character_lifecycle.h"
#include "local_server_restart.h"
#include "local_server_hero_selection.h"

// Network-session adaptation only. The host adopts the controller accepted by
// the native connection; it never creates a second controller or transfers an
// Engine.LocalPlayer to it. The old standalone trainer remains an independent
// regression build.
namespace local_server_gameplay {
enum class Preparation { Waiting, Ready, Failed };
struct State {
    bool servicesStarted=false,settingsStarted=false,servicesReady=false;
    bool hostWorldReady=false,modeClassesConfigured=false,heroSpawnAttempted=false;
    bool heroSpawned=false,heroPossessed=false;
    bool clientRestartSubmitted=false;
    bool initialSpawnPolicyReady=false;
    engine::Vec initialSpawnLocation{};
    engine::Vec initialSpawnRotation{};
    bool clientWorldBound=false,clientHeroBound=false,clientHUDReady=false;
    unsigned error=0;
    uint64_t startedAt=0;
    const char* stage="network_gameplay_not_started";
    void *world=nullptr,*bootstrapController=nullptr,*bootstrapPawn=nullptr;
    void *gameState=nullptr,*networkController=nullptr,*networkState=nullptr,*hero=nullptr;
    void* selectedHeroClass=nullptr;
    void* clientHero=nullptr;
};
inline State state;
// A configured offline host can choose its initial supported map position
// before deferred spawn. False means failure; true/ready=false defers login
// adoption until the original map layers are actually available. A null policy
// preserves the independent basic-network probe's original bootstrap pose.
using InitialSpawnProvider=bool(*)(void* world,void* pc,void* ps,void* pawnClass,
                                  engine::Transform& transform,engine::Vec& rotation,bool& ready);
inline InitialSpawnProvider initialSpawnProvider=nullptr;
inline bool SetInitialSpawnProvider(InitialSpawnProvider provider){
    if(initialSpawnProvider==provider)return true;
    if(state.heroSpawnAttempted||state.heroSpawned)return false;
    initialSpawnProvider=provider;return true;
}
template<class T> inline T Read(void* p,size_t o){return original_players::Value<T>(p,o);}
inline bool Fail(unsigned code,const char* reason){state.error=code;state.stage=reason;return false;}

inline Preparation Prepare(void* world,void* controller,void* pawn,bool host,
                           bool (*configureFactory)(original_players::Options&)=nullptr){
    if(state.error)return Preparation::Failed;
    if(state.servicesReady)return Preparation::Ready;
    if(!state.servicesStarted){
        state.startedAt=GetTickCount64();state.world=world;
        state.bootstrapController=controller;state.bootstrapPawn=pawn;
        state.stage="network_gameplay_original_local_services";
        if(!original_hud::InitializeServices(world,controller)
           ||!original_hud::InitializeUIServiceObjects()
           ||!original_hud::ConfigureInGameOnlyShell()){
            Fail(original_hud::error,original_hud::stage);return Preparation::Failed;
        }
        state.servicesStarted=true;
    }
    if(!state.settingsStarted){
        if(!original_hud::BeginSettingsInitialization()){
            Fail(original_hud::error,original_hud::stage);return Preparation::Failed;
        }
        state.settingsStarted=true;
    }
    if(!original_hud::PollSettings()){
        state.stage="network_gameplay_waiting_original_settings";
        if(GetTickCount64()-state.startedAt>30000){Fail(3101,"network_gameplay_settings_timeout");return Preparation::Failed;}
        return Preparation::Waiting;
    }
    state.stage="network_gameplay_original_content_services";
    if(!original_content::Initialize(world)){
        Fail(original_content::State().error,original_content::State().stage);return Preparation::Failed;
    }
    if(!original_content_services::Initialize(original_hud::State().clientInstance)){
        Fail(original_content_services::State().error,original_content_services::State().stage);return Preparation::Failed;
    }
    // Network PlayerState RepNotifies can run before any local HUD exists.
    // Bind the same real local provider graph before travel/listen, rather than
    // waiting for the later HUD install as the standalone version did.
    if(!original_hud::BindExistingLocalProviderReferencesImpl()){
        Fail(original_hud::error,original_hud::stage);return Preparation::Failed;
    }
    if(!original_players::Initialize(world,controller,pawn)){
        Fail(original_players::error,original_players::stage);return Preparation::Failed;
    }
    if(!local_server_services::Prepare(world,original_hud::State().clientInstance,host)){
        Fail(local_server_services::State().error,local_server_services::State().stage);return Preparation::Failed;
    }
    if(!host&&!original_input_bindings::Initialize(original_hud::State().clientInstance)){
        Fail(original_input_bindings::State().error,original_input_bindings::State().stage);return Preparation::Failed;
    }
    if(!original_render::Initialize(world)){
        Fail(original_render::State().error,original_render::State().stage);return Preparation::Failed;
    }
    if(!original_effects::Initialize(world)){
        Fail(original_effects::State().error,original_effects::State().stage);return Preparation::Failed;
    }
    if(host){
        original_players::options={true,false,false};
        if(configureFactory&&!configureFactory(original_players::options)){
            Fail(3120,"network_gameplay_session_factory_configuration_failed");return Preparation::Failed;
        }
        if(!original_players::CreateGameState()){
            Fail(original_players::error,original_players::stage);return Preparation::Failed;
        }
        state.gameState=Read<void*>(world,0x178);
        state.hostWorldReady=original_players::StateReady();
        if(!state.hostWorldReady){Fail(3102,"network_gameplay_shooter_game_state_not_ready");return Preparation::Failed;}
    }
    state.servicesReady=true;state.stage="network_gameplay_services_ready";
    return Preparation::Ready;
}

// Work-only native-Mode path: local constructors/services are staged before
// first StartPlay; an already created selected native GS is adopted, never rebuilt.
inline bool KickNativeStartup(const local_server_mode_startup::Source& source){
    if(state.error||!source.world||!source.gi||!source.mode)return false;
    if(state.world&&state.world!=source.world)return Fail(3140,"native_mode_local_service_world_changed");
    state.world=source.world;
    if(!state.servicesStarted){
        state.startedAt=GetTickCount64();state.bootstrapController=source.pc;state.bootstrapPawn=nullptr;
        if(!original_hud::InitializeServices(source.world,source.pc)||!original_hud::InitializeUIServiceObjects()
           ||!original_hud::ConfigureInGameOnlyShell())return Fail(original_hud::error,original_hud::stage);
        state.servicesStarted=true;
    }
    if(!state.settingsStarted){if(!original_hud::BeginSettingsInitialization())return Fail(original_hud::error,original_hud::stage);
        state.settingsStarted=true;}
    return true;
}
inline bool InitializeNativePawnlessContext(const local_server_mode_startup::Source& source){
    if(!local_server_mode_startup::AdmittedBootstrapContext(source.world,source.pc)||Read<void*>(source.pc,0x4b0))
        return Fail(3141,"native_mode_pawnless_context_requires_actual_startup_bootstrap_receipt");
    if(original_players::initialized)return original_players::Thread()&&original_players::world==source.world
        &&original_players::localPlayer==source.localPlayer&&original_players::fallbackController==source.pc
        &&!original_players::fallbackPawn;
    // Project-owned context caches only. No actor/PC/local-player/ready flags.
    original_players::gameThread=GetCurrentThreadId();original_players::world=source.world;
    original_players::fallbackController=original_players::activeController=source.pc;
    original_players::fallbackPawn=original_players::activePawn=nullptr;
    original_players::localPlayer=source.localPlayer;original_players::initialized=true;
    return original_players::Diagnose();
}
inline Preparation PrepareNativeStartup(const local_server_mode_startup::Source& source,
                                         bool (*configureFactory)(original_players::Options&)){
    if(!KickNativeStartup(source))return Preparation::Failed;
    if(!source.pc&&!local_server_mode_startup::options.noLocalPlayerBootstrap)return Preparation::Waiting;
    if(!original_hud::InitializeServices(source.world,source.pc)){
        Fail(original_hud::error,original_hud::stage);return Preparation::Failed;}
    if(!original_hud::PollSettings()){
        if(original_hud::error){Fail(original_hud::error,original_hud::stage);return Preparation::Failed;}
        state.stage="native_mode_waiting_actual_local_settings_before_first_gs_begin";return Preparation::Waiting;}
    if(!original_content::Initialize(source.world)||!original_content_services::Initialize(original_hud::State().clientInstance)
       ||!original_hud::BindExistingLocalProviderReferencesImpl()||!InitializeNativePawnlessContext(source)
       ||!local_server_services::Prepare(source.world,original_hud::State().clientInstance,true)
       ||!original_render::Initialize(source.world)||!original_effects::Initialize(source.world)){
        if(!state.error)Fail(3142,"native_mode_actual_local_service_preparation_failed");return Preparation::Failed;}
    original_players::options={false,false,false};
    if(!configureFactory||!configureFactory(original_players::options)){
        Fail(3143,"native_mode_existing_factory_configuration_unavailable");return Preparation::Failed;}
    const auto& startup=local_server_mode_startup::GetState();
    if(!original_players::options.gameStateClassPath
       ||std::wcscmp(original_players::options.gameStateClassPath,local_server_mode_startup::options.gameStateClass)!=0
       ||source.gs!=startup.gs||Read<void*>(source.gs,0x10)!=startup.selectedStateClass
       ||Read<void*>(source.world,0x178)!=source.gs||Read<void*>(source.mode,0x4d8)!=source.gs
       ||original_players::createdGameState){
        Fail(3144,"native_mode_selected_existing_gs_mismatch_or_second_manual_factory");return Preparation::Failed;}
    original_players::options.createGameState=false;state.gameState=source.gs;
    state.bootstrapController=source.pc;state.bootstrapPawn=nullptr;
    state.servicesReady=true;state.stage="native_mode_local_services_ready_gs_begin_pending";return Preparation::Ready;
}
inline bool AdoptNativeStartup(const local_server_mode_startup::Source& source){
    if(!local_server_mode_startup::ReadyForAdoption(source.world,source.pc)||state.gameState!=source.gs
       ||original_players::createdGameState||!original_players::StateReady())
        return Fail(3145,"native_mode_existing_selected_gs_lifecycle_adoption_failed");
    state.hostWorldReady=true;state.servicesReady=true;state.stage="native_mode_original_selected_gs_adopted_without_replacement";return true;
}
inline bool PublishNativeModeMatchStarted(){
    if(!local_server_mode_startup::ReadyForAdoption(state.world,state.bootstrapController))return false;
    const auto& startup=local_server_mode_startup::GetState();
    using Name=local_server_character_lifecycle::NativeName;
    const auto progress=Read<Name>(reinterpret_cast<void*>(engine::image),0xd7e6d70);
    if(!progress.comparison)return Fail(3146,"native_mode_original_inprogress_name_unavailable");
    const auto isProgress=[&](const Name& value){return local_server_character_lifecycle::Same(value,progress);};
    if(!isProgress(Read<Name>(startup.mode,0x518))||!isProgress(Read<Name>(state.gameState,0x4c0))){
        // Explicit offline match-start rule through the complete original API
        // on an actual AGameMode subclass. Original mode handlers, GS notify and
        // WorldSettings own the resulting behavior; no GS-only state write.
        if(Read<uintptr_t>(Read<void*>(startup.mode,0),0x9a8)!=engine::image+0x43cb7e0)
            return Fail(3146,"native_mode_original_set_match_state_entry_changed");
        engine::function<void(*)(void*,const Name*)>(0x43cb7e0)(startup.mode,&progress);
    }
    return isProgress(Read<Name>(startup.mode,0x518))&&isProgress(Read<Name>(state.gameState,0x4c0))
        &&Read<uint8_t>(state.gameState,0x490)==1
        ?true:Fail(3146,"native_mode_original_full_match_start_not_observed");
}
inline bool ObjectClassProperty(void* object,const wchar_t* name,void* value){
    uint32_t nativeName[3]{};
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(nativeName,name,1);
    auto property=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)
        (Read<void*>(object,0x10),nativeName,3);
    const int32_t offset=Read<int32_t>(property,0x44);
    if(!property||Read<int32_t>(property,0x30)!=1||Read<int32_t>(property,0x34)!=8||offset<0x30||offset>0x1000)
        return Fail(3103,"network_gameplay_mode_class_property_layout_mismatch");
    if(!original_players::Write(object,static_cast<size_t>(offset),value)||Read<void*>(object,static_cast<size_t>(offset))!=value)
        return Fail(3104,"network_gameplay_mode_class_property_update_failed");
    return true;
}

inline bool ConfigureHostClasses(){
    if(state.modeClassesConfigured)return true;
    if(!state.hostWorldReady)return Fail(3105,"network_gameplay_host_world_not_ready");
    state.stage="network_gameplay_configure_native_login_classes";
    auto mode=Read<void*>(state.world,0x170);
    if(!original_players::ResolvePlayerStateClass())return Fail(original_players::error?original_players::error:3106,"network_gameplay_player_state_class_unavailable");
    if(!local_server_hero_selection::Ready())return Fail(3136,"network_gameplay_selected_hero_sidecar_not_loaded");
    state.selectedHeroClass=original_players::LoadPawnClass(local_server_hero_selection::SelectionPath());
    if(!state.selectedHeroClass)return Fail(3107,"network_gameplay_selected_original_hero_class_unavailable");
    // The stripped Shooter PostLogin is supplied below. Suppress automatic
    // generic pawn construction until its real network PS is associated before
    // the hero's BeginPlay; do not manufacture a local player on the server.
    if(local_server_mode_startup::GetState().configured&&local_server_mode_startup::GetState().world==state.world
       &&!original_players::options.controllerClassPath)
        original_players::options.controllerClassPath=L"/Script/ShooterGame.ShooterPlayerController";
    auto controllerClass=engine::Class(original_players::ShooterControllerClass);
    if(original_players::options.controllerClassPath){
        controllerClass=engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t)>(0x1d0c6e0)
            (controllerClass,nullptr,original_players::options.controllerClassPath,nullptr,0);
        if(!controllerClass||!original_players::Derived(controllerClass,engine::Class(original_players::ShooterControllerClass)))
            return Fail(3121,"network_gameplay_login_controller_class_invalid");
    }
    if(!ObjectClassProperty(mode,L"PlayerControllerClass",controllerClass)
       ||!ObjectClassProperty(mode,L"PlayerStateClass",original_players::selectedPlayerStateClass)
       ||!ObjectClassProperty(mode,L"DefaultPawnClass",nullptr))return false;
    state.modeClassesConfigured=true;state.stage="network_gameplay_waiting_native_player_login";return true;
}

inline void* SpawnHuman(void* pc,void* ps,const engine::Vec& feet,double yaw){
    if(!original_players::Thread()||pc!=state.networkController||ps!=state.networkState
       ||!state.selectedHeroClass||Read<void*>(pc,0x470)!=ps){
        Fail(3122,"network_gameplay_respawn_requires_same_unpossessed_admitted_player");return nullptr;
    }
    if(auto previous=Read<void*>(pc,0x4b0)){
        bool alive=true;float health=0,maxHealth=0;
        if(previous!=state.hero||!deathmatch_rules::ReadLife(previous,alive,health,maxHealth)||alive||health>.001f){
            Fail(3126,"network_gameplay_respawn_existing_pawn_is_not_confirmed_dead");return nullptr;
        }
    }
    original_players::CharacterDefaults defaults;
    if(!local_server_hero_selection::Ready()
       ||!original_players::ProbeDefaults(local_server_hero_selection::SelectionPath(),defaults)
       ||defaults.type!=state.selectedHeroClass){
        Fail(3123,"network_gameplay_respawn_original_hero_defaults_unavailable");return nullptr;
    }
    engine::Transform transform;transform.translation=feet;transform.translation.z+=defaults.capsuleHalfHeight+2.;
    const double half=yaw*3.14159265358979323846/360.;transform.rotation[2]=std::sin(half);transform.rotation[3]=std::cos(half);
    auto pawn=original_players::Begin(state.selectedHeroClass,transform,pc);
    if(!original_players::IsA(pawn,original_players::ShooterCharacterClass)||!original_players::Write(pawn,0x488,ps)){
        Fail(3124,"network_gameplay_respawn_hero_or_player_state_association_failed");return nullptr;
    }
    original_players::Finish(pawn,transform);
    if(!Read<uint8_t>(pawn,0xa90)||!engine::Event(pc,L"/Script/Engine.Controller.Possess",&pawn)
       ||Read<void*>(pc,0x4b0)!=pawn||Read<void*>(pawn,0x488)!=ps||!original_inventory_slots::EnsureForPawn(pawn)){
        Fail(3125,"network_gameplay_respawn_native_possession_incomplete");return nullptr;
    }
    if(!local_server_character_lifecycle::PublishAuthorityCharacterLinks(state.world,pc,ps,pawn,state.hero)){
        Fail(local_server_character_lifecycle::CharacterLinksState().error,local_server_character_lifecycle::CharacterLinksState().stage);return nullptr;
    }
    const engine::Vec requestedRotation{0,yaw,0};
    if(!local_server_restart::PublishInitialRotation(state.world,pc,ps,pawn,requestedRotation)){
        Fail(local_server_restart::GetState().error,local_server_restart::GetState().stage);return nullptr;
    }
    state.hero=pawn;state.clientRestartSubmitted=false;state.stage="network_gameplay_authority_human_respawned";
    return pawn;
}

inline bool AdoptConnection(void* connection){
    if(!state.modeClassesConfigured||!connection)return false;
    auto pc=Read<void*>(connection,0x38),ps=Read<void*>(pc,0x470);
    if(Read<uint8_t>(connection,0x13c)!=3||Read<uint8_t>(connection,0x1d8)!=3)return true;
    if(!original_players::IsA(pc,original_players::ShooterControllerClass)
       ||Read<void*>(pc,0x510)!=connection||Read<void*>(connection,0xa0)!=pc)
        return Fail(3108,"network_gameplay_admitted_controller_ownership_mismatch");
    if(state.heroSpawned)return state.networkController==pc||Fail(3109,"network_gameplay_unexpected_second_client");
    if(state.heroSpawnAttempted)return Fail(3110,"network_gameplay_partial_hero_requires_new_session");
    if(!original_players::IsA(ps,original_players::ShooterStateClass)||Read<void*>(ps,0x210)!=pc
       ||!original_players::IsA(Read<void*>(ps,0x6a8),0x65a0b40))
        return Fail(3111,"network_gameplay_native_login_player_state_incomplete");
    state.networkController=pc;state.networkState=ps;
    if(!original_players::RegisterState(ps))return Fail(original_players::error,original_players::stage);
    if(Read<void*>(pc,0x4b0))return Fail(3112,"network_gameplay_unexpected_native_default_pawn");
    engine::Transform transform;
    const bool nativeMode=local_server_mode_startup::GetState().configured
        &&local_server_mode_startup::GetState().world==state.world;
    if(nativeMode){
        if(!local_server_mode_startup::ReadyForAdoption(state.world,state.bootstrapController)||!initialSpawnProvider)
            return Fail(3147,"native_mode_initial_human_requires_real_map_spawn_provider");
        state.initialSpawnRotation={};
    }else{
        if(!engine::Event(state.bootstrapPawn,L"/Script/Engine.Actor.K2_GetActorLocation",&transform.translation)
           ||!engine::Event(state.bootstrapPawn,L"/Script/Engine.Actor.K2_GetActorRotation",&state.initialSpawnRotation)
           ||!original_players::Finite(transform.translation)||!original_players::Finite(state.initialSpawnRotation))
            return Fail(3132,"network_gameplay_original_initial_spawn_pose_unavailable");
    }
    if(initialSpawnProvider){
        bool ready=false;
        if(!initialSpawnProvider(state.world,pc,ps,state.selectedHeroClass,transform,state.initialSpawnRotation,ready))
            return Fail(3133,"network_gameplay_host_initial_spawn_policy_failed");
        if(!ready){state.stage="network_gameplay_waiting_supported_initial_host_spawn";return true;}
        if(!original_players::Finite(transform.translation)||!original_players::Finite(state.initialSpawnRotation))
            return Fail(3134,"network_gameplay_host_initial_spawn_policy_returned_nonfinite_pose");
        state.initialSpawnPolicyReady=true;
    }
    state.initialSpawnLocation=transform.translation;
    state.heroSpawnAttempted=true;state.stage="network_gameplay_spawn_authority_selected_hero";
    const double half=state.initialSpawnRotation.y*3.14159265358979323846/360.;
    transform.rotation[2]=std::sin(half);transform.rotation[3]=std::cos(half);
    state.hero=original_players::Begin(state.selectedHeroClass,transform,pc);
    if(!original_players::IsA(state.hero,original_players::ShooterCharacterClass))return Fail(3113,"network_gameplay_authority_hero_spawn_failed");
    if(!original_players::Write(state.hero,0x488,ps))return Fail(3114,"network_gameplay_hero_ps_before_begin_play_failed");
    original_players::Finish(state.hero,transform);
    if(!Read<uint8_t>(state.hero,0xa90)||!Read<void*>(state.hero,0xc08)||!Read<void*>(state.hero,0xba0))
        return Fail(3115,"network_gameplay_original_hero_begin_play_incomplete");
    if(!engine::Event(pc,L"/Script/Engine.Controller.Possess",&state.hero)
       ||Read<void*>(pc,0x4b0)!=state.hero||Read<void*>(state.hero,0x488)!=ps)
        return Fail(3116,"network_gameplay_authority_hero_possession_failed");
    state.heroPossessed=true;
    if(!original_inventory_slots::EnsureForPawn(state.hero))
        return Fail(original_inventory_slots::State().error,original_inventory_slots::State().stage);
    if(!local_server_character_lifecycle::PublishAuthorityCharacterLinks(state.world,pc,ps,state.hero))
        return Fail(local_server_character_lifecycle::CharacterLinksState().error,local_server_character_lifecycle::CharacterLinksState().stage);
    if(!local_server_restart::PublishInitialRotation(state.world,pc,ps,state.hero,state.initialSpawnRotation))
        return Fail(local_server_restart::GetState().error,local_server_restart::GetState().stage);
    state.heroSpawned=true;state.stage="network_gameplay_authority_hero_possessed";return true;
}

inline bool BindClientWorld(void* world){
    if(state.clientWorldBound)return state.world==world||Fail(3117,"network_gameplay_second_client_travel_not_implemented");
    if(!world||world==state.world)return true;
    if(Read<void*>(world,0x1d8)!=original_hud::State().gameInstance)
        return Fail(3118,"network_gameplay_client_travel_game_instance_changed");
    // These are adapter caches, not engine ownership or replicated state.
    // The original GameInstance/sidecar survives non-seamless network travel.
    state.world=world;state.clientWorldBound=true;
    original_players::world=world;original_hud::world=world;
    original_content::state.world=world;
    original_effects::state={};
    if(!original_render::Initialize(world)||!original_effects::Initialize(world))
        return Fail(3119,"network_gameplay_client_world_service_rebind_failed");
    state.stage="network_gameplay_client_waiting_replicated_hero";return true;
}

inline bool PollClient(void* world,void* driver){
    if(!state.servicesReady||!world||!driver)return true;
    if(!BindClientWorld(world))return false;
    if(!state.clientWorldBound)return true;
    auto connection=Read<void*>(driver,0xc0);
    auto gi=Read<void*>(world,0x1d8);
    auto locals=Read<void**>(gi,0x40);
    auto lp=Read<int32_t>(gi,0x48)==1?Read<void*>(locals,0):nullptr;
    auto pc=Read<void*>(lp,0x38);
    auto ps=Read<void*>(pc,0x470),pawn=Read<void*>(pc,0x4b0);
    if(!original_players::IsA(pawn,original_players::ShooterCharacterClass))return true;
    if(state.clientHeroBound&&state.clientHero==pawn)return true;
    if(Read<void*>(pc,0x510)!=lp)return Fail(3121,"network_gameplay_client_local_player_back_reference_invalid");
    if(Read<int32_t>(gi,0x48)!=1||!lp||Read<void*>(locals,0)!=lp||Read<void*>(lp,0x38)!=pc
       ||Read<void*>(connection,0xa0)!=pc||Read<uint8_t>(pc,0x224)!=2||Read<uint8_t>(pawn,0x224)!=2)
        return Fail(3121,"network_gameplay_client_owned_hero_links_invalid");
    if(!original_players::IsA(ps,original_players::ShooterStateClass)||Read<void*>(pawn,0x488)!=ps
       ||Read<void*>(pawn,0x4a0)!=pc||!Read<uint8_t>(pawn,0xa90))return true;
    if(!local_server_hero_selection::Ready())return Fail(3136,"network_gameplay_selected_hero_sidecar_not_loaded");
    const auto actualClass=Read<void*>(pawn,0x10);
    const auto identity=original_content::FindCharacterByClass(actualClass);
    if(!identity.found||!local_server_hero_selection::SameGuid(identity.id,local_server_hero_selection::SelectionGuid()))
        return Fail(3137,"network_gameplay_replicated_own_hero_does_not_match_selected_catalog_entry");
    original_players::activeController=pc;original_players::activePawn=pawn;
    original_players::localPlayer=lp;original_players::localIndex=0;
    auto& entry=original_players::players[0];entry={};entry.local=true;
    entry.controller=pc;entry.state=ps;entry.character=pawn;
    entry.inventory=Read<void*>(pawn,0xc08);entry.mesh1P=Read<void*>(pawn,0xf30);
    entry.abilities=Read<void*>(pawn,0xa20);entry.movement=Read<void*>(pawn,0x4f8);
    entry.attributes=Read<void*>(entry.abilities,0x13a8);entry.configured=entry.ready=true;
    wcscpy_s(entry.classPath,local_server_hero_selection::SelectionPath());
    wcscpy_s(entry.requestedName,original_heroes::Names[local_server_hero_selection::SelectionIndex()]);
    entry.characterInitialized=true;entry.stateText="network_replicated_own_character";
    original_hud::controller=pc;
    state.gameState=Read<void*>(world,0x178);
    state.clientHero=pawn;state.clientHeroBound=true;
    state.stage="network_gameplay_client_received_owned_hero";
    return true;
}
}
