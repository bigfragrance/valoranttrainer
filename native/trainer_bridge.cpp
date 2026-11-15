#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <atomic>
#include <algorithm>
#include "engine_api.h"
#include "presentation.h"
#include "player_avatar.h"
#include "enemy_presentation.h"
#include "audio_feedback.h"
#include "jett_abilities.h"
#include "original_ui.h"
#include "training_menu_probe.h"
#include "original_players.h"
#include "original_weapon.h"
#include "original_movement.h"
#include "original_shot_observer.h"
#include "original_champions.h"
#include "enemy_hitboxes.h"
#include "original_hud.h"
#include "original_simulation.h"
#include "original_probe_channel.h"
#include "original_content.h"
#include "original_enemies.h"
#include "original_camera.h"
#include "original_scoreboard.h"
#include "original_enemy_weapon.h"
#include "original_inventory_slots.h"
#include "original_jett_native.h"
#include "original_jett_priming.h"
#include "original_render.h"
#include "original_effects.h"
#include "original_client_lifecycle.h"
#include "original_enemy_view.h"
#include "original_camera_owners.h"
#include "original_training.h"
#include "original_input_bindings.h"
#include "original_gameplay.h"
#include "original_jett_world.h"
#include "original_jett_events.h"
#include "original_jett_sync.h"
#include "original_infinite_ammo.h"
#include "original_ui_input_mode.h"
#include "original_ads.h"
#include "original_fault_observer.h"
#include "original_scope.h"
#include "original_ads_refresh.h"
#include "training_session.h"
#include "training_landing_shake_probe.h"
#pragma comment(lib, "user32.lib")

struct Context { uint64_t magic, version, pid, image, world, controller, pawn, stage; };
static_assert(sizeof(Context) == 64);
static HMODULE gModule = nullptr;
static Context gContext{};
static std::wstring gDirectory;
static SRWLOCK gFileLock = SRWLOCK_INIT;
static std::atomic<unsigned long long> gTicks{0};
static DWORD gTickThread = 0;
static unsigned long gError = 0;
static const char* gStage = "module_loaded";
using TickFn = void (*)(void*, float);
static TickFn gOriginalTick = nullptr;
static void* gWeapon = nullptr;
static void* gBot = nullptr;
static bool gResourcesCreated = false;
static HANDLE gMapping = nullptr;
static uint8_t* gShared = nullptr;
static uint32_t gCommandSequence = 0;
static uint32_t gAimSequence = 0;
static uint32_t gOriginalAimSequence = 0;
static bool gOriginalAmmoToggleHeld = false;
struct RenderState {
    uint32_t flags = 0; engine::Vec position{}; double yaw = 180;
    double pitchRecoil = 0, yawRecoil = 0, kick = 0;
    uint32_t shots = 0, ammo = 25, kills = 0, hits = 0, botId = 0; float health = 100;
};
static RenderState gRender;
static bool gReloading = false;
static double gReloadStarted = 0;
static int gBarriersDisabled = 0;
static ULONGLONG gLastRenderUpdate = 0;
static ULONGLONG gLastOriginalDiagnosticsUpdate = 0;
static bool gInputPaused = false;
static uint32_t gSettingsSequence = 0;
static float gActualFov = 0;
static const char* gOperation = "initializing";
static uintptr_t gExceptionRva = 0;
static uint32_t gNativeUIFlags = 0;
static bool gOriginalProbeAttempted = false;
static bool gOriginalProbeReady = false;
static bool gOriginalWaitingSettings = false;
static bool gOriginalWaitingWorld = false;
static ULONGLONG gOriginalSettingsStarted = 0;
static uint8_t gNativeLocalRole = 255, gNativeRemoteRole = 255, gNativeActorTick = 0, gNativeMovementTick = 0;
static uint8_t gNativeBeginPlayFlags = 0, gNativeHasBegunPlay = 255;
using OriginalActorTickFn = void(*)(void*, float, int, void*);
static OriginalActorTickFn gNativeActorTickFunction = nullptr;
static void* gNativeActorTable[349]{};
static uint64_t gNativeActorTickCalls = 0;
static float gNativeActorDelta = 0;
static int gNativeActorTickType = -1;
static engine::Vec gOriginalLocation{}, gOriginalVelocity{};
static void ObserveOriginalActorTick(void* actor, float delta, int tickType, void* tickFunction) {
    ++gNativeActorTickCalls; gNativeActorDelta = delta; gNativeActorTickType = tickType;
    gNativeActorTickFunction(actor, delta, tickType, tickFunction);
    training_local_posture_probe::Capture("own_camera_before");
    original_camera::AfterOriginalCharacterTick(actor,delta);
    training_local_posture_probe::Capture("own_camera_after");
}
static bool AttachOriginalActorObserver(void* pawn) {
    // Pinned primary ShooterCharacter table: 348 code pointers then next RTTI.
    auto expected = reinterpret_cast<void**>(engine::image + 0xb4dc550);
    auto object = reinterpret_cast<void***>(pawn);
    if (*object != expected || expected[0x4d0/8] != reinterpret_cast<void*>(engine::image + 0x787d440)) return false;
    std::memcpy(gNativeActorTable, expected - 1, sizeof(gNativeActorTable));
    gNativeActorTickFunction = reinterpret_cast<OriginalActorTickFn>(expected[0x4d0/8]);
    gNativeActorTable[1+0x4d0/8] = reinterpret_cast<void*>(ObserveOriginalActorTick);
    return InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(object), gNativeActorTable+1, expected) == expected;
}

template<class T> static T ReadAt(const uint8_t* source, size_t offset) {
    T value; std::memcpy(&value, source + offset, sizeof(T)); return value;
}
template<class T> static void WriteAt(uint8_t* target, size_t offset, const T& value) {
    std::memcpy(target + offset, &value, sizeof(T));
}

static void SaveState(bool includeSubsystems=true);
static void Error(unsigned long code, const char* stage) {
    gError=code;gStage=stage;
    if(training_session::enabled&&original_players::gameThread==GetCurrentThreadId()){
        training_session::lastDiagnostics=0;training_session::WriteDiagnostics(gDirectory);
    }
    SaveState();
}
static bool RebindTrainingLocal(void* oldPawn,void* newPawn);

static void OriginalPlayerProbe() {
    engine::image = static_cast<uintptr_t>(gContext.image);
    gOriginalProbeAttempted = true;
    const auto dump = []() { original_players::WriteDiagnostics((gDirectory + L"original-player-probe.json").c_str()); };
    gStage = "original_probe_client_services"; SaveState();
    if (!original_hud::InitializeServices(reinterpret_cast<void*>(gContext.world), reinterpret_cast<void*>(gContext.controller))) {
        Error(original_hud::error, original_hud::stage); return;
    }
    if (!original_hud::InitializeUIServiceObjects()) {
        Error(original_hud::error, original_hud::stage); return;
    }
    if (!original_hud::ConfigureInGameOnlyShell()) {
        Error(original_hud::error, original_hud::stage); return;
    }
    if (!original_hud::BeginSettingsInitialization()) {
        Error(original_hud::error, original_hud::stage); return;
    }
    if (!original_hud::PollSettings()) {
        if (!gOriginalSettingsStarted) gOriginalSettingsStarted = GetTickCount64();
        gOriginalWaitingSettings = true;
        gStage = "original_probe_waiting_settings"; SaveState(); return;
    }
    {
        gStage = "original_probe_content_catalog"; SaveState();
        const bool content=original_content::Initialize(reinterpret_cast<void*>(gContext.world));
        original_content::WriteDiagnostics((gDirectory+L"original-content.json").c_str());
        if (!content) { Error(original_content::State().error,original_content::State().stage);return; }
    }
    if (gContext.stage == 6 || gContext.stage >= 8) {
        gStage="original_probe_content_services";SaveState();
        if (!original_content_services::Initialize(original_hud::diagnostics.clientInstance)) {
            original_content_services::WriteDiagnostics((gDirectory+L"original-content-services.json").c_str());
            Error(original_content_services::State().error,original_content_services::State().stage);return;
        }
    }
    gStage = "original_probe_defaults"; SaveState();
    if (!original_players::Initialize(reinterpret_cast<void*>(gContext.world), reinterpret_cast<void*>(gContext.controller), reinterpret_cast<void*>(gContext.pawn))) {
        dump(); Error(original_players::error, original_players::stage); return;
    }
    if(gContext.stage==6||gContext.stage>=8) {
        if(!original_input_bindings::Initialize(original_hud::diagnostics.clientInstance)) {
            Error(original_input_bindings::State().error,original_input_bindings::State().stage);return;
        }
    }
    original_players::options = {true, true, true};
    if(training_session::enabled){
        training_session::ConfigureFactory();
        if(!training_session::PrepareWorld(reinterpret_cast<void*>(gContext.world))){
            Error(training_session::error,training_session::stage);return;
        }
        if(!training_session::WorldReady()){
            gOriginalWaitingWorld=true;gStage="original_training_waiting_map_layers";SaveState();return;
        }
    }
    if(!original_render::Initialize(reinterpret_cast<void*>(gContext.world))) {
        Error(original_render::State().error,original_render::State().stage);return;
    }
    if(!original_effects::Initialize(reinterpret_cast<void*>(gContext.world))) {
        Error(original_effects::State().error,original_effects::State().stage);return;
    }
    gStage="original_probe_remove_spawn_barriers";SaveState();
    gBarriersDisabled=engine::DisableSpawnBarriers(reinterpret_cast<void*>(gContext.world));
    if(gBarriersDisabled<0){Error(955,"original_probe_spawn_barrier_setup_failed");return;}
    // The diagnostic fallback uses Engine's default 90 degrees. Set the
    // requested training projection before native LocalPlayer transfer copies it.
    const float originalFov = 103.0f;
    void* priorCamera = original_players::Value<void*>(reinterpret_cast<void*>(gContext.controller), 0x528);
    original_players::Write(priorCamera, 0x484, originalFov);
    engine::Event(reinterpret_cast<void*>(gContext.controller), L"/Script/Engine.PlayerController.FOV", const_cast<float*>(&originalFov));
    gStage = "original_probe_game_state"; SaveState();
    if (!original_players::CreateGameState()) { dump(); Error(original_players::error, original_players::stage); return; }
    const auto localFeet=training_session::enabled?training_session::SpawnFeet(0,1,L"/Game/Characters/Wushu/Wushu_PC.Wushu_PC_C"):engine::Vec{-2050,0,200};
    if(training_session::enabled&&training_session::error){Error(training_session::error,training_session::stage);return;}
    original_players::Configure(0, L"Jett", L"/Game/Characters/Wushu/Wushu_PC.Wushu_PC_C", localFeet, training_session::enabled?training_session::lastSpawnYaw:0, true);
    gStage = "original_probe_player"; SaveState();
    if (!original_players::CreatePlayer(0)) {
        dump(); Error(original_players::error, original_players::stage); return;
    }
    if(!original_inventory_slots::EnsureForPawn(original_players::players[0].character)) {
        original_inventory_slots::WriteDiagnostics((gDirectory+L"original-inventory-slots.json").c_str());
        Error(original_inventory_slots::State().error,original_inventory_slots::State().stage);return;
    }
    if(!original_effects::EnsureActor(original_players::players[0].character,"local_character")) {
        Error(original_effects::State().error,original_effects::State().stage);return;
    }
    if (!original_players::ActivateLocal(0)) {
        dump(); Error(original_players::error, original_players::stage); return;
    }
    if (!original_enemies::BindRosterPlayer(0)) {
        original_enemies::WriteDiagnostics((gDirectory+L"original-enemies.json").c_str());
        Error(original_enemies::error,original_enemies::stage);return;
    }
    if(!original_effects::EnsureController(original_players::activeController,"local_controller")) {
        Error(original_effects::State().error,original_effects::State().stage);return;
    }
    if ((gContext.stage==6||gContext.stage>=8)&&(!training_session::enabled||!training_session::FreeForAll())) {
        gStage="original_probe_jett_loadout";SaveState();
        if(!original_jett::Initialize(reinterpret_cast<void*>(gContext.world),original_players::activePawn)
            ||!original_jett::GrantMissingLoadout()||!original_jett::Refill()) {
            original_jett::WriteDiagnostics((gDirectory+L"original-jett.json").c_str());
            Error(original_jett::State().error,original_jett::State().stage);return;
        }
        if(!original_jett_priming::Initialize(original_players::activePawn,original_jett::skills[2].actor)){
            original_jett_priming::WriteDiagnostics((gDirectory+L"original-jett-priming.json").c_str());
            Error(original_jett_priming::State().error,original_jett_priming::State().stage);return;
        }
    }
    gStage = "original_probe_local_restart"; SaveState();
    void* restart = engine::Find(L"/Script/Engine.PlayerController.ClientRestart");
    if (!restart || original_players::Value<uint16_t>(restart, 0xbe) != 8) {
        Error(952, "original_client_restart_signature_unavailable"); return;
    }
    void* restartedPawn = original_players::activePawn;
    engine::Event(original_players::activeController, L"/Script/Engine.PlayerController.ClientRestart", &restartedPawn);
    if(gContext.stage>=8&&!original_input_bindings::InitializeLocalPC()) {
        Error(original_input_bindings::LocalPCState().error,original_input_bindings::LocalPCState().stage);return;
    }
    gStage="original_probe_detach_bootstrap_controller";SaveState();
    if(!original_client_lifecycle::DetachBootstrapController(reinterpret_cast<void*>(gContext.world),
        original_players::activeController,restartedPawn,reinterpret_cast<void*>(gContext.controller),reinterpret_cast<void*>(gContext.pawn))) {
        original_client_lifecycle::WriteDiagnostics((gDirectory+L"original-client-lifecycle.json").c_str());
        Error(original_client_lifecycle::State().error,original_client_lifecycle::State().stage);return;
    }
    if (!original_camera::Initialize(reinterpret_cast<void*>(gContext.world),original_players::activeController,restartedPawn)) {
        Error(original_camera::State().error,original_camera::State().stage);return;
    }
    if (!AttachOriginalActorObserver(restartedPawn)) { Error(953, "original_actor_tick_observer_table_mismatch"); return; }
    dump();
    gStage = "original_probe_movement"; SaveState();
    if (!original_movement::Initialize(original_players::activeController, original_players::activePawn)) {
        Error(original_movement::State().error, original_movement::State().stage); return;
    }
    if (gContext.stage != 6 && !original_simulation::Initialize(original_players::activePawn)) {
        Error(original_simulation::State().error, original_simulation::State().stage); return;
    }
    if (gContext.stage != 6 && !original_simulation::SelectOriginalLocalMovementBranch()) {
        Error(original_simulation::State().error, original_simulation::State().stage); return;
    }
    if (gContext.stage != 6 && !original_simulation::InstallMovementTickObserver()) {
        Error(original_simulation::State().error, original_simulation::State().stage); return;
    }
    if(gContext.stage>=8&&!original_gameplay::StartTraining(reinterpret_cast<void*>(gContext.world),original_players::activePawn)) {
        Error(original_gameplay::State().error,original_gameplay::State().stage);return;
    }
    if (gContext.stage == 6) {
        gStage = "original_probe_full_hud"; SaveState();
        if (!original_hud::InstallOriginalHUD(original_players::activeController, original_players::activePawn)) {
            Error(original_hud::error, original_hud::stage); return;
        }
        gOriginalProbeReady = true; gStage = "original_player_and_hud_probe_active"; SaveState(); return;
    }
    if(training_session::enabled){
        gStage="original_training_initializing_local_match";SaveState();
        training_session::testActionsAllowed=gContext.stage==9;
        training_menu_probe::enabled=gContext.stage==9
            ||GetFileAttributesW((gDirectory+L"enable-menu-probe").c_str())!=INVALID_FILE_ATTRIBUTES;
        original_ui_input_mode::SetDetailedProbe(training_menu_probe::enabled);
        if(!training_session::Initialize({RebindTrainingLocal})){
            training_session::WriteDiagnostics(gDirectory);
            Error(training_session::error,training_session::stage);return;
        }
        if(!training_session::FreeForAll()){
            if(!original_jett_world::Initialize(reinterpret_cast<void*>(gContext.world),original_players::activePawn)
               ||!original_jett_events::Initialize(original_players::activePawn)||!original_jett_sync::Initialize(original_players::activePawn)){
                Error(2610,"original_training_jett_lifecycle_failed");return;
            }
        }
        if(gContext.stage==9){auto pc=original_players::activeController;engine::Event(pc,L"/Script/Engine.Actor.DisableInput",&pc);
            training_local_posture_probe::SetEnabled(true,true);
            if(!training_landing_shake_probe::Initialize(true)||!training_landing_shake_probe::Bind(original_players::Value<void*>(pc,0x528))){
                training_landing_shake_probe::WriteDiagnostics((gDirectory+L"training-landing-shake-probe.json").c_str());
                Error(training_landing_shake_probe::State().error,training_landing_shake_probe::State().stage);return;
            }}
        gOriginalProbeReady=true;gStage="original_training_v2_active";SaveState();return;
    }
    gStage = "original_probe_weapon"; SaveState();
    const bool weapon = original_weapon::SpawnAndBindAK(reinterpret_cast<void*>(gContext.world), original_players::activePawn);
    original_weapon::WriteDiagnostics((gDirectory + L"original-weapon-probe.json").c_str());
    if (!weapon) { Error(original_weapon::State().error, original_weapon::State().stage); return; }
    original_client_lifecycle::Probe(reinterpret_cast<void*>(gContext.world),original_players::activeController,
        original_players::activePawn,reinterpret_cast<void*>(gContext.controller),reinterpret_cast<void*>(gContext.pawn),original_weapon::weapon);
    if (!original_shots::Initialize(original_weapon::firing, original_weapon::ammo)) {
        Error(original_shots::State().error, original_shots::State().stage); return;
    }
    original_champions::Diagnose();
    if (gContext.stage >= 8) {
        if(!original_infinite_ammo::Initialize(reinterpret_cast<void*>(gContext.world),
                original_weapon::ammo,gContext.stage==8)) {
            Error(original_infinite_ammo::State().error,original_infinite_ammo::State().stage);return;
        }
        gStage="original_probe_enemy_roster";SaveState();
        bool used[original_enemies::HeroCount]{};
        uint32_t logicalId=1;
        for (size_t index=1;index<=original_enemies::HeroCount;++index) {
            while (used[original_enemies::HeroFor(logicalId)]) ++logicalId;
            used[original_enemies::HeroFor(logicalId)]=true;
            if (!original_enemies::Spawn(index,logicalId,{-900,(static_cast<double>(index)-2)*300,200})) {
                original_enemies::WriteDiagnostics((gDirectory+L"original-enemies.json").c_str());
                Error(original_enemies::error,original_enemies::stage);return;
            }
            if(!original_camera_owners::Apply(index)) {
                original_camera_owners::WriteDiagnostics((gDirectory+L"original-camera-owners.json").c_str());
                Error(original_camera_owners::State().error,original_camera_owners::State().stage);return;
            }
            if(!original_enemy_view::Apply(index)) {
                original_enemy_view::WriteDiagnostics((gDirectory+L"original-enemy-view.json").c_str());
                Error(original_enemy_view::State().error,original_enemy_view::State().stage);return;
            }
            if(!original_inventory_slots::EnsureForPawn(original_players::players[index].character)) {
                original_inventory_slots::WriteDiagnostics((gDirectory+L"original-inventory-slots.json").c_str());
                Error(original_inventory_slots::State().error,original_inventory_slots::State().stage);return;
            }
            if(!original_effects::EnsureActor(original_players::players[index].character,"enemy_character")) {
                Error(original_effects::State().error,original_effects::State().stage);return;
            }
            if (!original_enemy_weapon::SpawnAK(index)) {
                original_enemy_weapon::WriteDiagnostics((gDirectory+L"original-enemy-weapons.json").c_str());
                Error(original_enemy_weapon::error,original_enemy_weapon::stage);return;
            }
            ++logicalId;
        }
        original_enemies::WriteDiagnostics((gDirectory+L"original-enemies.json").c_str());
        if (!original_scoreboard::Initialize(reinterpret_cast<void*>(gContext.world))) {
            Error(original_scoreboard::State().error,original_scoreboard::State().stage);return;
        }
        for (size_t index=0;index<=original_enemies::HeroCount;++index) {
            if (!original_scoreboard::RegisterPlayer(original_players::players[index].state)
                ||!original_enemies::VerifyRosterName(index)) {
                original_scoreboard::WriteDiagnostics((gDirectory+L"original-scoreboard.json").c_str());
                Error(original_scoreboard::State().error?original_scoreboard::State().error:original_enemies::error,
                      original_scoreboard::State().error?original_scoreboard::State().stage:original_enemies::stage);return;
            }
        }
        gStage="original_probe_training_authority";SaveState();
        if(!original_training::Initialize(reinterpret_cast<void*>(gContext.world))) {
            Error(original_training::State().error,original_training::State().stage);return;
        }
        if(!original_jett_world::Initialize(reinterpret_cast<void*>(gContext.world),original_players::activePawn)) {
            Error(original_jett_world::State().error,original_jett_world::State().stage);return;
        }
        if(!original_jett_events::Initialize(original_players::activePawn)) {
            Error(original_jett_events::State().error,original_jett_events::State().stage);return;
        }
        if(!original_jett_sync::Initialize(original_players::activePawn)) {
            Error(original_jett_sync::State().error,original_jett_sync::State().stage);return;
        }
        gStage="original_training_installing_hud";SaveState();
        if(!original_hud::InstallOriginalHUD(original_players::activeController,original_players::activePawn)) {
            Error(original_hud::error,original_hud::stage);return;
        }
        if(!original_camera::RestoreWatchPossessedLifecycle()) {
            Error(original_camera::State().error,original_camera::State().stage);return;
        }
    }
    if(gContext.stage==7||gContext.stage==9) {
        auto testController=original_players::activeController;
        if(!engine::Event(testController,L"/Script/Engine.Actor.DisableInput",&testController)) {
            Error(958,"original_probe_native_test_input_isolation_failed");return;
        }
    }
    gOriginalProbeReady = true; gStage = "original_player_and_weapon_probe_active"; SaveState();
}

static void ReadSettings() {
    if (!gShared) return;
    const auto seq = ReadAt<uint32_t>(gShared, 640);
    if (!seq || (seq & 1) || seq == gSettingsSequence) return;
    uint8_t copy[32]{}; std::memcpy(copy, gShared + 640, sizeof(copy)); MemoryBarrier();
    if (ReadAt<uint32_t>(gShared, 640) != seq) return;
    const double sensitivity = ReadAt<double>(copy, 8);
    const float fov = ReadAt<float>(copy, 16);
    if (!std::isfinite(sensitivity) || sensitivity < .05 || sensitivity > 2 ||
        !std::isfinite(fov) || fov < 80 || fov > 120) return;
    avatar::options.sensitivity = sensitivity;
    if (avatar::options.fieldOfView != fov) {
        avatar::options.fieldOfView = fov;
        avatar::ApplyFOV();
    }
    enemy_presentation::options.outline = ReadAt<uint32_t>(copy, 20) != 0;
    gInputPaused = ReadAt<uint32_t>(copy, 24) != 0;
    gSettingsSequence = seq;
}

static bool CreateResources() {
    engine::image = static_cast<uintptr_t>(gContext.image);
    gStage = "reading_original_player_defaults"; SaveState();
    original_players::Initialize(reinterpret_cast<void*>(gContext.world), reinterpret_cast<void*>(gContext.controller), reinterpret_cast<void*>(gContext.pawn));
    original_players::WriteDiagnostics((gDirectory + L"original-player-defaults.json").c_str());
    gStage = "initializing_jett_avatar"; SaveState();
    if (!avatar::Initialize(reinterpret_cast<void*>(gContext.world), reinterpret_cast<void*>(gContext.controller), reinterpret_cast<void*>(gContext.pawn))) {
        Error(avatar::error, avatar::stage); return false;
    }
    gContext.pawn = reinterpret_cast<uint64_t>(avatar::pawn);
    gStage = "loading_weapon_mesh"; SaveState();
    void* mesh = engine::Load(engine::Class(0x3815ac0), L"/Game/Equippables/Guns/Rifles/AK/S0/1P/Models/GN_AK_S0_Skelmesh.GN_AK_S0_Skelmesh");
    if (!mesh) { Error(101, "weapon_mesh_missing"); return false; }
    gStage = "loading_weapon_material"; SaveState();
    void* material = engine::Load(engine::Class(0x3815a10), L"/Game/Equippables/Guns/Rifles/AK/Champions/Materials/AK_Champions_MI.AK_Champions_MI");
    if (!material) { Error(102, "weapon_material_missing"); return false; }
    gStage = "loading_bot_mesh"; SaveState();
    void* botMesh = engine::Load(engine::Class(0x3815ac0), L"/Game/Characters/Wushu/S0/3P/Models/TP_Wushu_S0_Skelmesh.TP_Wushu_S0_Skelmesh");
    if (!botMesh) { Error(103, "bot_mesh_missing"); return false; }
    gStage = "creating_weapon_actor"; SaveState();
    gWeapon = engine::Spawn(reinterpret_cast<void*>(gContext.world), {-2000, 18, 285}, reinterpret_cast<void*>(gContext.pawn));
    if (!gWeapon) { Error(104, "weapon_actor_missing"); return false; }
    void* component = engine::AddSkeletal(gWeapon);
    if (!component) { Error(105, "weapon_component_missing"); return false; }
    if (!engine::SetSkeletal(component, mesh) || !engine::SetMaterial(component, material) || !engine::DisableCollision(component)) {
        Error(106, "weapon_component_setup_failed"); return false;
    }
    void* magazineMesh = engine::Load(engine::Class(0x3816800), L"/Game/Equippables/Guns/Rifles/AK/S0/1P/Models/GN_AK_Magazine_S0_Staticmesh.GN_AK_Magazine_S0_Staticmesh");
    void* magazine = magazineMesh ? engine::AddStatic(gWeapon) : nullptr;
    if (magazine) {
        engine::SetStatic(magazine, magazineMesh);
        engine::SetMaterial(magazine, material);
        engine::DisableCollision(magazine);
        engine::AttachSocket(magazine, component, L"Magazine_Main");
    }
    gStage = "creating_bot_actor"; SaveState();
    gBot = engine::Spawn(reinterpret_cast<void*>(gContext.world), {-900, 400, 200});
    if (!gBot) { Error(107, "bot_actor_missing"); return false; }
    void* botComponent = engine::AddSkeletal(gBot);
    if (!botComponent || !engine::SetSkeletal(botComponent, botMesh) || !engine::DisableCollision(botComponent)) {
        Error(108, "bot_component_setup_failed"); return false;
    }
    engine::SetTransform(gBot, {-900, 400, 200}, {0, 180, 0});
    if (gContext.stage >= 3) {
        gBarriersDisabled = engine::DisableSpawnBarriers(reinterpret_cast<void*>(gContext.world));
        if (gBarriersDisabled < 0) { Error(109, "barrier_setup_failed"); return false; }
    }
    gStage = "initializing_jett_presentation"; SaveState();
    if (!presentation::Initialize(reinterpret_cast<void*>(gContext.world), reinterpret_cast<void*>(gContext.pawn), gWeapon, component, material)) {
        Error(presentation::error, presentation::stage); return false;
    }
    gStage = "initializing_enemy_presentation"; SaveState();
    if (!enemy_presentation::Initialize(reinterpret_cast<void*>(gContext.world), gBot, botComponent)) {
        Error(enemy_presentation::error, enemy_presentation::stage); return false;
    }
    if (!enemy_hitboxes::Bind(enemy_presentation::body)) {
        Error(enemy_hitboxes::error, enemy_hitboxes::stage); return false;
    }
    gStage = "initializing_jett_abilities"; SaveState();
    if (!jett::Initialize(reinterpret_cast<void*>(gContext.world), reinterpret_cast<void*>(gContext.controller), avatar::pawn)) {
        Error(jett::State().error, jett::State().stage); return false;
    }
    audio_feedback::Initialize(reinterpret_cast<void*>(gContext.world), avatar::pawn);
    gStage = "creating_original_ui_widgets"; SaveState();
    if (original_ui::Initialize(reinterpret_cast<void*>(gContext.world), reinterpret_cast<void*>(gContext.controller))) {
        if (original_ui::ShowDuel1Map(true, {20, 20, 210, 210})) gNativeUIFlags |= 1;
        const wchar_t* icons[] {original_ui::kJettCloudburst, original_ui::kJettUpdraft, original_ui::kJettTailwind, original_ui::kJettBladeStorm};
        bool iconsReady = true;
        for (unsigned i=0;i<4;++i)
            if (original_ui::CreateOriginalImage(icons[i], {506.0 + i*76, 616, 40, 40}) == original_ui::InvalidHandle) iconsReady = false;
        if (iconsReady) gNativeUIFlags |= 2;
    }
    gStage = "native_weapon_and_bot_created"; SaveState();
    return true;
}

static void ExchangeFrame(const engine::Vec& position, const engine::Vec& rotation) {
    if (!gShared) return;
    const double now = static_cast<double>(GetTickCount64()) / 1000.0;
    DWORD foregroundPid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &foregroundPid);
    const bool focus = foregroundPid == GetCurrentProcessId();
    uint32_t flags = focus ? 8U : 0U;
    if (focus && GetAsyncKeyState(VK_LBUTTON) & 0x8000) flags |= 1;
    if (focus && GetAsyncKeyState('R') & 0x8000) flags |= 2;
    if (focus && GetAsyncKeyState(VK_RBUTTON) & 0x8000) flags |= 4;
    if (focus && GetAsyncKeyState(VK_TAB) & 0x8000) flags |= 16;
    if (focus && GetAsyncKeyState(VK_ESCAPE) & 0x8000) flags |= 32;
    if (focus && GetAsyncKeyState('C') & 0x8000) flags |= 64;
    if (focus && GetAsyncKeyState('Q') & 0x8000) flags |= 128;
    if (focus && GetAsyncKeyState('E') & 0x8000) flags |= 256;
    if (focus && GetAsyncKeyState('X') & 0x8000) flags |= 512;
    if (gContext.stage == 4) {
        const uint32_t overrideFlags = ReadAt<uint32_t>(gShared, 16);
        if (overrideFlags & 0x80000000U) flags = overrideFlags & 1023U;
        const uint32_t aimSequence = static_cast<uint32_t>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(gShared + 448), 0, 0));
        if (aimSequence && aimSequence != gAimSequence) {
            auto aim = ReadAt<engine::Vec>(gShared, 456);
            if (std::isfinite(aim.x) && std::isfinite(aim.y) && std::isfinite(aim.z) &&
                std::abs(aim.x) <= 360 && std::abs(aim.y) <= 360 && std::abs(aim.z) <= 360) {
                engine::Event(reinterpret_cast<void*>(gContext.controller), L"/Script/Engine.Controller.SetControlRotation", &aim);
            }
            gAimSequence = aimSequence;
            InterlockedExchange(reinterpret_cast<volatile LONG*>(gShared + 480), static_cast<LONG>(aimSequence));
        }
    }
    const auto key = [](int k) { return (GetAsyncKeyState(k) & 0x8000) != 0; };
    jett::SetDashInput(focus ? double(key('W'))-double(key('S')) : 0,
                       focus ? double(key('D'))-double(key('A')) : 0);
    jett::Update(now, position, rotation, gInputPaused ? 0 : flags);
    const auto& skills = jett::State();
    InterlockedIncrement(reinterpret_cast<volatile LONG*>(gShared + 64));
    WriteAt(gShared, 72, now); WriteAt(gShared, 80, position); WriteAt(gShared, 104, rotation); WriteAt(gShared, 128, flags);
    WriteAt<uint32_t>(gShared, 132, skills.ultimate_active ? 1 : 0);
    WriteAt<uint32_t>(gShared, 136, (now < skills.weapon_blocked_until ? 1U : 0U) |
                                  (skills.inside_smoke ? 2U : 0U) | (skills.request_burst ? 4U : 0U));
    WriteAt(gShared, 140, skills.active_smokes);
    WriteAt<float>(gShared, 144, static_cast<float>((std::max)(0.0, skills.smoke_ready_at-now)));
    WriteAt<float>(gShared, 148, static_cast<float>((std::max)(0.0, skills.updraft_ready_at-now)));
    WriteAt<float>(gShared, 152, static_cast<float>((std::max)(0.0, skills.dash_ready_at-now)));
    WriteAt(gShared, 156, skills.smoke_casts); WriteAt(gShared, 160, skills.updraft_casts);
    WriteAt(gShared, 164, skills.dash_casts); WriteAt(gShared, 168, skills.ultimate_casts);
    WriteAt(gShared, 172, gNativeUIFlags);
    engine::Transform targetHead;
    const bool headValid = (gRender.flags & 1) && gRender.health > 0 && enemy_presentation::ready &&
        presentation::Socket(enemy_presentation::body, L"Head", 0, targetHead);
    WriteAt<uint32_t>(gShared, 188, headValid ? gRender.botId : 0);
    WriteAt<engine::Vec>(gShared, 192, headValid ? targetHead.translation : engine::Vec{});
    MemoryBarrier();
    InterlockedIncrement(reinterpret_cast<volatile LONG*>(gShared + 64));
    const uint32_t sequence = static_cast<uint32_t>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(gShared + 256), 0, 0));
    if (sequence && sequence != gCommandSequence) {
        const auto opcode = ReadAt<uint32_t>(gShared, 260);
        int status = -3; double distance = 0; engine::Vec hit{};
        int region = 0; uint32_t hitBot = 0;
        if (opcode == 1) {
            const auto origin = ReadAt<engine::Vec>(gShared, 264);
            const auto direction = ReadAt<engine::Vec>(gShared, 288);
            const double maximum = ReadAt<double>(gShared, 312);
            if (std::isfinite(maximum) && maximum > 0 && maximum <= 1000000)
                status = engine::Trace(reinterpret_cast<void*>(gContext.pawn), origin, direction, maximum, distance, gBot);
            if (status == 1) hit = engine::Add(origin, engine::Mul(direction, distance));
        }
        if (opcode == 2) {
            const auto origin = ReadAt<engine::Vec>(gShared, 264);
            const auto direction = ReadAt<engine::Vec>(gShared, 288);
            const double maximum = ReadAt<double>(gShared, 312);
            if (std::isfinite(maximum) && maximum > 0 && maximum <= 1000000) {
                status = 0; distance = std::numeric_limits<double>::infinity();
                if ((gRender.flags & 1) && gRender.health > 0) {
                    enemy_hitboxes::Result result;
                    status = enemy_hitboxes::Trace(origin, engine::Add(origin, engine::Mul(direction, maximum)), result);
                    if (status == 1) { distance = result.distance; hit = result.location; region = result.region; hitBot = gRender.botId; }
                }
            }
        }
        WriteAt(gShared, 388, status); WriteAt(gShared, 392, distance); WriteAt(gShared, 400, hit);
        WriteAt(gShared, 424, region); WriteAt(gShared, 428, hitBot);
        MemoryBarrier();
        InterlockedExchange(reinterpret_cast<volatile LONG*>(gShared + 384), static_cast<LONG>(sequence));
        gCommandSequence = sequence;
    }
    const LONG renderSequence = InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(gShared + 512), 0, 0);
    if (renderSequence && !(renderSequence & 1)) {
        uint8_t copy[88]{}; std::memcpy(copy, gShared + 512, sizeof(copy)); MemoryBarrier();
        if (renderSequence == InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(gShared + 512), 0, 0)) {
            gRender.flags = ReadAt<uint32_t>(copy, 4); gRender.position = ReadAt<engine::Vec>(copy, 8);
            gRender.yaw = ReadAt<double>(copy, 32); gRender.pitchRecoil = ReadAt<double>(copy, 40);
            gRender.yawRecoil = ReadAt<double>(copy, 48); gRender.kick = ReadAt<double>(copy, 56);
            gRender.shots = ReadAt<uint32_t>(copy, 64); gRender.ammo = ReadAt<uint32_t>(copy, 68);
            gRender.kills = ReadAt<uint32_t>(copy, 72); gRender.hits = ReadAt<uint32_t>(copy, 76);
            gRender.botId = ReadAt<uint32_t>(copy, 80); gRender.health = ReadAt<float>(copy, 84);
        }
    }
    const bool reloading = (gRender.flags & 2) != 0;
    if (reloading && !gReloading) gReloadStarted = now;
    gReloading = reloading;
}

static void WriteSubsystemDiagnostics() {
    original_hud::WriteDiagnostics((gDirectory + L"original-hud.json").c_str());
    original_simulation::WriteDiagnostics((gDirectory + L"original-simulation.json").c_str());
    original_camera::WriteDiagnostics((gDirectory+L"original-camera.json").c_str());
    original_content_services::WriteDiagnostics((gDirectory+L"original-content-services.json").c_str());
    original_champions::WriteDiagnostics((gDirectory+L"original-champions.json").c_str());
    original_inventory_slots::WriteDiagnostics((gDirectory+L"original-inventory-slots.json").c_str());
    original_jett::WriteDiagnostics((gDirectory+L"original-jett.json").c_str());
    original_render::WriteDiagnostics((gDirectory+L"original-render.json").c_str());
    original_shots::WriteDiagnostics((gDirectory+L"original-shot-origin-probe.json").c_str());
    original_effects::WriteDiagnostics((gDirectory+L"original-effects.json").c_str());
    original_client_lifecycle::WriteDiagnostics((gDirectory+L"original-client-lifecycle.json").c_str());
    original_enemy_view::WriteDiagnostics((gDirectory+L"original-enemy-view.json").c_str());
    original_camera_owners::WriteDiagnostics((gDirectory+L"original-camera-owners.json").c_str());
    original_training::WriteDiagnostics((gDirectory+L"original-training.json").c_str());
    original_damage::WriteDiagnostics((gDirectory+L"original-damage.json").c_str());
    original_damage_tuning::WriteDiagnostics((gDirectory+L"original-damage-tuning.json").c_str());
    original_enemy_respawn::WriteDiagnostics((gDirectory+L"original-enemy-respawn.json").c_str());
    original_input_bindings::WriteDiagnostics((gDirectory+L"original-input-bindings.json").c_str());
    original_ability_tuning::WriteDiagnostics((gDirectory+L"original-ability-tuning.json").c_str());
    original_gameplay::WriteDiagnostics((gDirectory+L"original-gameplay.json").c_str());
    original_jett_world::WriteDiagnostics((gDirectory+L"original-jett-world.json").c_str());
    original_jett_events::WriteDiagnostics((gDirectory+L"original-jett-events.json").c_str());
    original_jett_sync::WriteDiagnostics((gDirectory+L"original-jett-sync.json").c_str());
    original_infinite_ammo::WriteDiagnostics((gDirectory+L"original-infinite-ammo.json").c_str());
    original_ui_input_mode::WriteDiagnostics((gDirectory+L"original-ui-input-mode.json").c_str());
    original_ads::WriteDiagnostics((gDirectory+L"original-ads.json").c_str());
    original_faults::WriteDiagnostics((gDirectory+L"original-fault-observer.json").c_str());
    original_scope::WriteDiagnostics((gDirectory+L"original-scope.json").c_str());
    original_ads_refresh::WriteDiagnostics((gDirectory+L"original-ads-refresh.json").c_str());
    if (gContext.stage >= 8) original_scoreboard::WriteDiagnostics((gDirectory+L"original-scoreboard.json").c_str());
    if (gContext.stage >= 8) original_enemy_weapon::WriteDiagnostics((gDirectory+L"original-enemy-weapons.json").c_str());
}
static void SaveState(bool includeSubsystems) {
    AcquireSRWLockExclusive(&gFileLock);
    // Publish the stop/error receipt after its subsystem evidence is complete.
    // The owning runner can then end the child without truncating diagnostics.
    if(includeSubsystems)WriteSubsystemDiagnostics();
    const std::wstring output = gDirectory + L"bridge-state.json";
    HANDLE file = CreateFileW(output.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file != INVALID_HANDLE_VALUE) {
        char buffer[8192]{};
        const int count = sprintf_s(buffer,
            "{\"protocol\":1,\"module_loaded\":true,\"pid\":%lu,\"stage\":\"%s\","
            "\"tick_count\":%llu,\"tick_thread\":%lu,\"error\":%lu,\"barriers_disabled\":%d,"
            "\"jett_presentation\":%s,\"presentation_stage\":\"%s\",\"fire_animations\":%u,\"reload_animations\":%u,"
            "\"muzzle_emitters\":%u,\"emitter_failures\":%u,\"transform_failures\":%u,"
            "\"avatar_ready\":%s,\"avatar_stage\":\"%s\",\"avatar_error\":%u,\"camera_from_manager\":%s,"
            "\"fov\":%.1f,\"actual_fov\":%.1f,\"capsule_half_height\":%.1f,\"base_eye_height\":%.1f,"
            "\"enemy_ready\":%s,\"enemy_hero\":\"%s\",\"enemy_animation\":\"%s\",\"enemy_error\":%u,\"enemy_deaths\":%u,"
            "\"enemy_pose_compositions\":%u,\"enemy_outline_mode\":%u,"
            "\"enemy_hitbox_bodies\":%u,\"enemy_hitbox_actors\":%u,\"enemy_hitbox_syncs\":%u,"
            "\"abilities_ready\":%s,\"abilities_error\":%u,\"abilities_stage\":\"%s\",\"ultimate_active\":%s,"
            "\"smoke_casts\":%u,\"updraft_casts\":%u,\"dash_casts\":%u,\"ultimate_casts\":%u,\"knife_shots\":%u,"
            "\"active_smokes\":%u,\"ability_effect_failures\":%u,\"audio_accepted_posts\":%u,\"audio_failed_posts\":%u,"
            "\"original_ui_flags\":%u,\"original_ui_error\":%u,\"original_ui_stage\":\"%s\","
            "\"original_movement_ready\":%s,\"original_max_speed\":%.3f,\"original_jump_velocity\":%.3f,"
            "\"original_crouch_held\":%s,\"original_capsule_half_height\":%.3f,\"original_moves\":%u,"
            "\"original_movement_disabled\":%s,\"original_actor_tick_enabled\":%u,\"original_movement_tick_enabled\":%u,"
            "\"original_local_role\":%u,\"original_remote_role\":%u,"
            "\"original_beginplay_flags\":%u,\"original_has_begun_play\":%u,"
            "\"original_actor_tick_calls\":%llu,\"original_actor_delta\":%.6f,\"original_actor_tick_type\":%d,"
            "\"original_actor_tick_registered\":%s,\"original_actor_tick_capability\":%s,"
            "\"original_fog_valid\":%s,\"original_fog_relevant\":%s,\"original_fog_hidden\":%s,"
            "\"original_movement_tick_calls\":%llu,\"original_movement_mode\":%u,\"original_land_mode\":%u,"
            "\"original_valid_movement_data\":%s,\"original_updated_mobility\":%u,\"original_damage_activity\":%u,"
            "\"original_skip_update\":%s,\"original_update_only_if_rendered\":%s,\"original_inventory_simulation\":%s,"
            "\"original_location\":[%.4f,%.4f,%.4f],\"original_velocity\":[%.4f,%.4f,%.4f],"
            "\"original_physics_calls\":%llu,\"original_physics_delta\":%.6f,\"original_server_prediction\":%s,"
            "\"original_client_prediction\":%s,\"original_clock_seconds\":%.6f,\"original_clock_step\":%llu,"
            "\"original_weapon_stage\":\"%s\",\"original_weapon_ammo\":%d,\"original_shot_rays\":%llu,"
            "\"original_services_stage\":\"%s\",\"original_services_error\":%u,"
            "\"original_settings_initialized\":%s,\"original_settings_count\":%d,"
            "\"original_services_exception_rva\":%llu,\"original_services_exception_operation\":\"%s\","
            "\"original_crosshair_provider\":%s,\"original_crosshair_initialized\":%s,"
            "\"original_hud_installed\":%s,\"original_hud_player_bound\":%s,\"original_hud_roots\":%s,"
            "\"operation\":\"%s\",\"exception_rva\":%llu,\"training_ready\":%s}\n",
            GetCurrentProcessId(), gStage, gTicks.load(), gTickThread, gError, gBarriersDisabled,
            presentation::ready ? "true" : "false", presentation::stage, presentation::fireAnimations,
            presentation::reloadAnimations, presentation::emittersSpawned, presentation::emitterFailures,
            presentation::transformFailures, avatar::ready ? "true" : "false", avatar::stage, avatar::error,
            avatar::positionFromCameraManager && avatar::rotationFromCameraManager ? "true" : "false",
            avatar::options.fieldOfView, gActualFov, avatar::capsuleHalfHeight, avatar::baseEyeHeight,
            enemy_presentation::ready ? "true" : "false", enemy_presentation::heroName, enemy_presentation::animationState,
            enemy_presentation::error, enemy_presentation::deathCount,
            enemy_presentation::poseCompositions, static_cast<unsigned>(enemy_presentation::observedOutlineMode),
            static_cast<unsigned>(enemy_hitboxes::bodyCount), static_cast<unsigned>(enemy_hitboxes::physicsActorCount), enemy_hitboxes::poseSyncs,
            jett::State().ready ? "true" : "false", jett::State().error, jett::State().stage,
            jett::State().ultimate_active ? "true" : "false", jett::State().smoke_casts, jett::State().updraft_casts,
            jett::State().dash_casts, jett::State().ultimate_casts, jett::State().knife_shots,
            jett::State().active_smokes, jett::State().effect_failures,
            audio_feedback::State().acceptedPosts, audio_feedback::State().failedPosts,
            gNativeUIFlags, original_ui::State().lastError, original_ui::State().stage,
            original_movement::State().ready ? "true" : "false", original_movement::State().current_max_speed,
            original_movement::State().jump_velocity, original_movement::State().crouch_held ? "true" : "false",
            original_movement::State().actual_capsule_half_height, original_movement::State().total_moves,
            original_movement::State().movement_disabled ? "true" : "false", static_cast<unsigned>(gNativeActorTick),
            static_cast<unsigned>(gNativeMovementTick), static_cast<unsigned>(gNativeLocalRole), static_cast<unsigned>(gNativeRemoteRole),
            static_cast<unsigned>(gNativeBeginPlayFlags), static_cast<unsigned>(gNativeHasBegunPlay),
            static_cast<unsigned long long>(gNativeActorTickCalls), gNativeActorDelta, gNativeActorTickType,
            original_simulation::State().actor_tick_registered ? "true" : "false", original_simulation::State().actor_tick_capability ? "true" : "false",
            original_simulation::State().fog_valid ? "true" : "false", original_simulation::State().fog_owner_relevant ? "true" : "false",
            original_simulation::State().fog_owner_hidden ? "true" : "false",
            static_cast<unsigned long long>(original_simulation::State().movement_tick_calls),
            static_cast<unsigned>(original_simulation::State().movement_mode), static_cast<unsigned>(original_simulation::State().default_land_mode),
            original_simulation::State().has_valid_movement_data ? "true" : "false", static_cast<unsigned>(original_simulation::State().updated_mobility),
            static_cast<unsigned>(original_simulation::State().damage_handler_activity),
            original_simulation::State().last_should_skip_update ? "true" : "false", original_simulation::State().update_only_if_rendered ? "true" : "false",
            original_simulation::State().inventory_simulation_enabled ? "true" : "false",
            gOriginalLocation.x, gOriginalLocation.y, gOriginalLocation.z, gOriginalVelocity.x, gOriginalVelocity.y, gOriginalVelocity.z,
            static_cast<unsigned long long>(original_simulation::State().perform_movement_calls), original_simulation::State().perform_movement_delta,
            original_simulation::State().server_prediction ? "true" : "false", original_simulation::State().client_prediction ? "true" : "false",
            original_simulation::State().clock_accumulated_seconds, static_cast<unsigned long long>(original_simulation::State().clock_current_step),
            original_weapon::State().stage, original_weapon::State().ammo, static_cast<unsigned long long>(original_shots::State().rays),
            original_hud::stage, original_hud::error, original_hud::State().settingsInitialized ? "true" : "false",
            original_hud::State().settingDataCount,
            original_hud::exceptionAddress ? static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(original_hud::exceptionAddress)-engine::image) : 0ULL,
            original_hud::exceptionOperation,
            original_hud::State().crosshairManager ? "true" : "false", original_hud::State().crosshairInitialized ? "true" : "false",
            original_hud::State().originalHUDInstalled ? "true" : "false", original_hud::State().playerModelBound ? "true" : "false",
            original_hud::State().originalRootObjectsPresent ? "true" : "false",
            gOperation, static_cast<unsigned long long>(gExceptionRva),training_session::enabled&&training_session::ready?"true":"false");
        DWORD written = 0;
        if (count > 0) WriteFile(file, buffer, static_cast<DWORD>(count), &written, nullptr);
        CloseHandle(file);
    }
    ReleaseSRWLockExclusive(&gFileLock);
}

static bool RebindTrainingLocal(void* oldPawn,void* newPawn) {
    auto pc=original_players::activeController;
    if(!pc||original_players::activePawn!=newPawn)return false;
    auto oldMovement=original_players::Value<void*>(oldPawn,0x4f8);
    if(oldMovement&&original_simulation::observedMovementTable&&original_simulation::originalMovementTable)
        InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(oldMovement),original_simulation::originalMovementTable,original_simulation::observedMovementTable);
    original_input_bindings::localInput={};
    void* restarted=newPawn;
    if(!engine::Event(pc,L"/Script/Engine.PlayerController.ClientRestart",&restarted)
        ||!original_input_bindings::InitializeLocalPC()||!original_camera::Initialize(reinterpret_cast<void*>(gContext.world),pc,newPawn)
        ||!AttachOriginalActorObserver(newPawn)||!original_movement::Initialize(pc,newPawn)
        ||!original_simulation::Initialize(newPawn)||!original_simulation::SelectOriginalLocalMovementBranch()
        ||!original_simulation::InstallMovementTickObserver()||!original_camera::RestoreWatchPossessedLifecycle(oldPawn))return false;
    original_gameplay::state.pawn=newPawn;
    if(!original_gameplay::Poll())return false;
    if(!training_session::FreeForAll()){
        original_jett::state={};original_jett::skills={};original_jett::previousInputs=0;
        original_jett_world::state={};original_jett_world::tracked={};
        original_jett_events::state={};original_jett_events::equipped={};original_jett_events::cloudburstThrown={};
        original_jett_sync::state={};original_jett_sync::machines={};
        if(!original_jett::Initialize(reinterpret_cast<void*>(gContext.world),newPawn)||!original_jett::GrantMissingLoadout()||!original_jett::Refill()
           ||!original_jett_priming::Initialize(newPawn,original_jett::skills[2].actor)
           ||!original_jett_world::Initialize(reinterpret_cast<void*>(gContext.world),newPawn)
           ||!original_jett_events::Initialize(newPawn)||!original_jett_sync::Initialize(newPawn))return false;
    }
    return original_hud::InstallOriginalHUD(pc,newPawn);
}

static void TrainingV2Tick(float deltaSeconds) {
    training_performance::Frame frameTiming(gDirectory);
    training_performance::Scope timing(training_performance::Slot::Session);
    const auto now=GetTickCount64();const double seconds=static_cast<double>(now)/1000;
    gOperation="training_v2_native_match";
    DWORD focusedProcess=0;GetWindowThreadProcessId(GetForegroundWindow(),&focusedProcess);
    if(gContext.stage==8&&focusedProcess==GetCurrentProcessId()&&(GetAsyncKeyState(VK_F10)&0x8000)){
        gStage="original_training_user_exit_requested";SaveState();return;
    }
    if(!training_session::Poll(seconds)){training_session::Publish(now+100);Error(training_session::error,training_session::stage);return;}
    timing.Step(training_performance::Slot::Effects);
    if(!original_effects::Poll()){Error(original_effects::State().error,original_effects::State().stage);return;}
    timing.Step(training_performance::Slot::UI);
    original_hud::RefreshOptionsState();
    const bool optionsOpen=original_hud::State().optionsOpen;
    if(!original_buy_menu::Update(optionsOpen||original_ui_input_mode::State().mapOpen)){
        Error(original_buy_menu::State().error,original_buy_menu::State().stage);return;
    }
    original_ui_input_mode::SetExternalMenu(original_buy_menu::BlocksGameplay(),original_buy_menu::State().screen);
    if(!original_ui_input_mode::Update()){Error(original_ui_input_mode::State().error,original_ui_input_mode::State().stage);return;}
    training_menu_probe::Sample();
    if(!training_session::LocalAlive()){
        timing.Step(training_performance::Slot::Diagnostics);
        training_session::WriteDiagnostics(gDirectory);
        if(now-gLastOriginalDiagnosticsUpdate>=500){gLastOriginalDiagnosticsUpdate=now;
            original_ui_input_mode::WriteDiagnostics((gDirectory+L"original-ui-input-mode.json").c_str());
            SaveState(gContext.stage==9);}
        return;
    }
    timing.Step(training_performance::Slot::LocalGameplay);
    if(!original_gameplay::Poll()){Error(original_gameplay::State().error,original_gameplay::State().stage);return;}
    timing.Step(training_performance::Slot::Input);
    const bool uiBlocked=original_ui_input_mode::BlocksGameplay();
    DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
    const auto down=[](int key){return (GetAsyncKeyState(key)&0x8000)!=0;};
    uint32_t input=foreground==GetCurrentProcessId()&&!uiBlocked&&deathmatch_rules::Active()?8U:0U;
    float forward=0,right=0;bool jump=false,walk=false,crouch=false;
    if(input){
        if(down(VK_F10)){gStage="original_training_user_exit_requested";SaveState();return;}
        if(down(VK_LBUTTON))input|=1;if(down('R'))input|=2;if(down(VK_RBUTTON))input|=4;
        if(!training_session::FreeForAll()){
            if(down('C'))input|=64;if(down('Q'))input|=128;if(down('E'))input|=256;if(down('X'))input|=512;
        }
        forward=float(down('W'))-float(down('S'));right=float(down('D'))-float(down('A'));
        jump=down(VK_SPACE);walk=down(VK_SHIFT);crouch=down(VK_CONTROL);
    }
    if(gContext.stage==9){
        const auto test=original_probe::TestInput(gShared,gContext.stage,seconds);
        input=test.active?test.flags:0;forward=test.forward;right=test.right;jump=test.jump;walk=test.walk;crouch=test.crouch;
        const auto aim=original_probe::TestAim(gShared,gContext.stage,seconds);
        if(aim.active&&aim.sequence!=gOriginalAimSequence){auto rotation=aim.rotation;
            if(!engine::Event(original_players::activeController,L"/Script/Engine.Controller.SetControlRotation",&rotation)){Error(2615,"training_v2_test_rotation_failed");return;}
            gOriginalAimSequence=aim.sequence;
        }
    }
    if(!deathmatch_rules::Active())input=0,forward=right=0,jump=walk=crouch=false;
    const bool toggle=gContext.stage==9?(input&32U)!=0:(input&8U)&&down(VK_F6);
    if(toggle&&!training_session::ammoToggleHeld&&original_infinite_ammo::State().initialized){
        if(!original_infinite_ammo::Toggle()){Error(original_infinite_ammo::State().error,original_infinite_ammo::State().stage);return;}
        if(original_infinite_ammo::Enabled())training_session::initial.config.flags|=training_control::InfiniteAmmo;
        else training_session::initial.config.flags&=~training_control::InfiniteAmmo;
    }
    training_session::ammoToggleHeld=toggle;
    if(training_session::FreeForAll())input&=~(64U|128U|256U|512U);
    if(!original_movement::Update(forward,right,jump,walk,crouch,(input&8U)!=0)){
        Error(original_movement::State().error,original_movement::State().stage);return;
    }
    if(gContext.stage==8&&!original_input_bindings::Update(deltaSeconds,uiBlocked)){
        Error(original_input_bindings::LocalPCState().error,original_input_bindings::LocalPCState().stage);return;
    }
    if(original_jett::State().loadoutGranted&&!original_jett::UpdateInput(training_session::FreeForAll()?0:input)){
        Error(original_jett::State().error,original_jett::State().stage);return;
    }
    const auto& binding=weapon_runtime_bindings::State();
    // The original ability may own the hands while the last firearm binding
    // remains alive. Its activation and primary/secondary inputs still use
    // the same real Pawn input buffer and native scheduler.
    const bool abilityCurrent=training_session::OriginalAbilityCurrent();
    if((all_weapons::State().ready||abilityCurrent)&&binding.ready&&!all_weapons::State().busy){
        if(!training_combat_runtime::WeaponInput(0,(input&1U)!=0,(input&2U)!=0,(input&4U)!=0)){
            Error(training_combat_runtime::State().error,training_combat_runtime::State().stage);return;
        }
        const auto current=all_weapons::CurrentWeapon();
        if(binding.firearm&&current==binding.weapon&&current==original_weapon::weapon){
            if(!original_ads_refresh::Update(reinterpret_cast<void*>(gContext.world),original_players::activePawn,all_weapons::CurrentWeapon())){
                Error(original_ads_refresh::State().error,original_ads_refresh::State().stage);return;
            }
            if(!original_infinite_ammo::Poll()){Error(original_infinite_ammo::State().error,original_infinite_ammo::State().stage);return;}
        }
    }else if(!uiBlocked&&!original_input_bindings::SubmitStandaloneViewInput()){
        Error(original_input_bindings::State().error,original_input_bindings::State().stage);return;
    }
    if(!training_session::FreeForAll()&&(!original_jett_world::Poll()||!original_jett_sync::Poll())){Error(2616,"training_v2_original_jett_update_failed");return;}
    if(!training_session::FreeForAll()&&!original_dagger_combat::Poll(seconds)){
        Error(original_dagger_combat::State().error,original_dagger_combat::State().stage);return;
    }
    if(now-gLastRenderUpdate>=16){gLastRenderUpdate=now;original_probe::Publish(gShared,original_players::activePawn,original_players::activeController,gContext.stage,input);}
    timing.Step(training_performance::Slot::Snapshot);
    if(training_local_posture_probe::State().enabled){
        static uint64_t lastPostureWrite=0;
        if(now-lastPostureWrite>=200){lastPostureWrite=now;
            training_local_posture_probe::WriteDiagnostics((gDirectory+L"training-local-posture-probe.json").c_str());
            training_landing_shake_probe::WriteDiagnostics((gDirectory+L"training-landing-shake-probe.json").c_str());
            if(training_session::FreeForAll()){
                original_timed_streak::WriteDiagnostics((gDirectory+L"original-timed-streak.json").c_str());
                original_streak_effect_authority::WriteDiagnostics((gDirectory+L"original-streak-effect-authority.json").c_str());
                original_streak_hud::WriteDiagnostics((gDirectory+L"original-streak-hud.json").c_str());}}
    }
    if(now-gLastOriginalDiagnosticsUpdate>=500){
        training_performance::Scope snapshotTiming(training_performance::Slot::SnapshotSamples);
        gLastOriginalDiagnosticsUpdate=now;
        if(original_jett::State().loadoutGranted&&!original_jett::Sample()){
            Error(original_jett::State().error,original_jett::State().stage);return;
        }
        if(gContext.stage==9){
            original_camera::Sample();original_ads::Sample();original_movement::Sample();original_simulation::Refresh();
        }else{
            static uint32_t diagnosticSample=0;
            switch(diagnosticSample++%3){
                case 0:original_camera::Sample();original_ads::Sample();break;
                case 1:original_movement::Sample();break;
                default:original_simulation::Refresh();break;
            }
        }
        if(gContext.stage==9){
            original_hud::InspectHUD();original_render::SampleRoster();
            original_players::WriteDiagnostics((gDirectory+L"original-player-probe.json").c_str());
            original_weapon::WriteDiagnostics((gDirectory+L"original-weapon-probe.json").c_str());
        }else original_ui_input_mode::WriteDiagnostics((gDirectory+L"original-ui-input-mode.json").c_str());
        snapshotTiming.Step(training_performance::Slot::SnapshotFiles);
        SaveState(gContext.stage==9);
        if(original_ui_input_mode::BlocksGameplay())training_menu_probe::WriteDiagnostics((gDirectory+L"training-menu-probe.json").c_str());
    }
    timing.Step(training_performance::Slot::Diagnostics);
    training_session::WriteDiagnostics(gDirectory);
}

static void OnGameTick(void*, float deltaSeconds) {
    const auto count = ++gTicks;
    if (count == 1) {
        gTickThread = GetCurrentThreadId();
        gStage = "game_thread_tick_observed";
        SaveState();
    }
    if (gContext.stage >= 5) {
        if (!gOriginalProbeAttempted) OriginalPlayerProbe();
        if(gOriginalWaitingWorld){
            if(!training_session::PrepareWorld(reinterpret_cast<void*>(gContext.world))){Error(training_session::error,training_session::stage);return;}
            training_maps::WriteDiagnostics((gDirectory+L"training-maps.json").c_str());
            if(training_session::WorldReady()){gOriginalWaitingWorld=false;OriginalPlayerProbe();}
        }
        if (gOriginalWaitingSettings && count % 60 == 0) {
            if (original_hud::PollSettings()) {
                gOriginalWaitingSettings = false;
                OriginalPlayerProbe();
            } else if (GetTickCount64() - gOriginalSettingsStarted > 25000) {
                Error(951, "original_settings_initialization_timeout");
            }
        }
        if (gOriginalProbeReady) {
            if(training_session::enabled){TrainingV2Tick(deltaSeconds);return;}
            if(gContext.stage>=8&&!original_gameplay::Poll()) {
                Error(original_gameplay::State().error,original_gameplay::State().stage);return;
            }
            if(!original_effects::Poll()) {Error(original_effects::State().error,original_effects::State().stage);return;}
            if (original_camera::State().error) {Error(original_camera::State().error,original_camera::State().stage);return;}
            DWORD foreground = 0; GetWindowThreadProcessId(GetForegroundWindow(), &foreground);
            uint32_t input = foreground == GetCurrentProcessId() ? 8U : 0U;
            if(original_hud::State().originalHUDInstalled)original_hud::RefreshOptionsState();
            const bool optionsOpen=original_hud::State().originalHUDInstalled&&original_hud::State().optionsOpen;
            if(gContext.stage==8&&!original_ui_input_mode::Update()) {
                Error(original_ui_input_mode::State().error,original_ui_input_mode::State().stage);return;
            }
            const bool uiBlocked=optionsOpen||(gContext.stage==8&&original_ui_input_mode::BlocksGameplay());
            if(gContext.stage==8&&input&&(GetAsyncKeyState(VK_F10)&0x8000)) {
                gStage="original_training_user_exit_requested";SaveState();return;
            }
            if(uiBlocked)input=0;
            if (input) {
                if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) input |= 1;
                if (GetAsyncKeyState('R') & 0x8000) input |= 2;
                if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) input |= 4;
                if (GetAsyncKeyState('C') & 0x8000) input |= 64;
                if (GetAsyncKeyState('Q') & 0x8000) input |= 128;
                if (GetAsyncKeyState('E') & 0x8000) input |= 256;
                if (GetAsyncKeyState('X') & 0x8000) input |= 512;
            }
            const auto down = [](int k) { return (GetAsyncKeyState(k) & 0x8000) != 0; };
            auto forward=float(down('W'))-float(down('S')),right=float(down('D'))-float(down('A'));
            bool jump=down(VK_SPACE),walk=down(VK_SHIFT),crouch=down(VK_CONTROL);
            if(uiBlocked){forward=right=0;jump=walk=crouch=false;}
            if (gContext.stage == 7 ||gContext.stage==9) {
                const auto test=original_probe::TestInput(gShared,gContext.stage,static_cast<double>(GetTickCount64())/1000.0);
                input=test.active?test.flags:0;forward=test.forward;right=test.right;
                jump=test.jump;walk=test.walk;crouch=test.crouch;
                const auto aim=original_probe::TestAim(gShared,gContext.stage,static_cast<double>(GetTickCount64())/1000.0);
                if(aim.active&&aim.sequence!=gOriginalAimSequence) {
                    auto rotation=aim.rotation;
                    if(!engine::Event(original_players::activeController,L"/Script/Engine.Controller.SetControlRotation",&rotation)) {
                        Error(957,"original_probe_control_rotation_api_missing");return;
                    }
                    gOriginalAimSequence=aim.sequence;
                }
            }
            if(gContext.stage>=8) {
                const bool toggleHeld=gContext.stage==9?(input&32U)!=0
                    :foreground==GetCurrentProcessId()&&!uiBlocked&&(GetAsyncKeyState(VK_F6)&0x8000)!=0;
                if(toggleHeld&&!gOriginalAmmoToggleHeld&&!original_infinite_ammo::Toggle()) {
                    Error(original_infinite_ammo::State().error,original_infinite_ammo::State().stage);return;
                }
                gOriginalAmmoToggleHeld=toggleHeld;
            }
            gOperation = "original_movement_input";
            if (!original_movement::Update(forward,right,jump,walk,crouch,(input & 8) != 0)) {
                Error(original_movement::State().error,original_movement::State().stage);return;
            }
            // Automated stages receive only their expiring test commands.
            // The manual trainer consumes the original physical mouse queue.
            if(gContext.stage==8&&!original_input_bindings::Update(deltaSeconds,uiBlocked)) {
                Error(original_input_bindings::LocalPCState().error,original_input_bindings::LocalPCState().stage);return;
            }
            if(gContext.stage>=8&&!uiBlocked&&!original_input_bindings::SubmitStandaloneViewInput()) {
                Error(original_input_bindings::State().error,original_input_bindings::State().stage);return;
            }
            gOperation = "original_weapon_input";
            if(original_jett::State().loadoutGranted&&!original_jett::UpdateInput(input)) {
                Error(original_jett::State().error,original_jett::State().stage);return;
            }
            original_weapon::UpdateInput(input);
            if (gContext.stage != 6 && original_weapon::State().ready
                &&!original_weapon::State().state_machine_observers_installed
                &&!original_weapon::InstallStateMachineObservers()) {
                Error(original_weapon::State().error,original_weapon::State().stage);return;
            }
            if (gContext.stage != 6 && !original_weapon::SubmitStandaloneInputBuffer()) {
                Error(original_weapon::State().error,original_weapon::State().stage);return;
            }
            original_shots::Poll();
            if(gContext.stage>=8&&!original_jett_world::Poll()) {
                Error(original_jett_world::State().error,original_jett_world::State().stage);return;
            }
            if(gContext.stage>=8&&!original_jett_sync::Poll()) {
                Error(original_jett_sync::State().error,original_jett_sync::State().stage);return;
            }
            const auto now=GetTickCount64();
            if (gContext.stage >= 8) {
                if(!original_training::SetTestStationary(gContext.stage==9&&(input&16U)!=0)
                   ||!original_training::Poll(static_cast<double>(now)/1000.0)) {
                    Error(original_training::State().error,original_training::State().stage);return;
                }
                if (original_weapon::State().ready && !original_champions::State().requested) {
                    gOperation="original_champions_apply";
                    if (!original_champions::Apply()) {Error(original_champions::State().error,original_champions::State().stage);return;}
                }
                original_champions::Poll();
                if(original_weapon::State().ready&&!original_scope::Update(reinterpret_cast<void*>(gContext.world),
                        original_players::activePawn,original_weapon::weapon)) {
                    Error(original_scope::FactoryState().error,original_scope::FactoryState().stage);return;
                }
                if(original_weapon::State().ready&&!original_ads_refresh::Update(reinterpret_cast<void*>(gContext.world),
                        original_players::activePawn,original_weapon::weapon)) {
                    Error(original_ads_refresh::State().error,original_ads_refresh::State().stage);return;
                }
                engine::Vec player{};
                engine::Event(original_players::activePawn,L"/Script/Engine.Actor.K2_GetActorLocation",&player);
                for (size_t index=1;index<=original_enemies::HeroCount;++index) {
                    if(!original_training::ShouldUpdateEnemy(index))continue;
                    if (!original_enemies::Update(index,static_cast<double>(now)/1000.0,player)) {
                        original_enemies::WriteDiagnostics((gDirectory+L"original-enemies.json").c_str());
                        Error(original_enemies::error,original_enemies::stage);return;
                    }
                }
            }
            if (gContext.stage != 6 && now-gLastRenderUpdate>=8) {
                gLastRenderUpdate=now;gOperation="original_probe_publish";
                if (!original_probe::Publish(gShared,original_players::activePawn,original_players::activeController,gContext.stage,input)) {
                    Error(954,"original_probe_publication_failed");return;
                }
            }
            // Diagnostics run on the game thread. Bound their frequency by
            // elapsed time so uncapped rendering does not multiply tree walks
            // and large JSON snapshots as the render framerate rises.
            if (now-gLastOriginalDiagnosticsUpdate >= 500) {
                gLastOriginalDiagnosticsUpdate=now;
                gOperation="original_camera_diagnostics";
                original_camera::Sample();
                original_ads::Sample();
                if(gContext.stage>=8)original_scope::Probe(reinterpret_cast<void*>(gContext.world),
                    original_players::activePawn,original_weapon::weapon);
                gOperation="original_render_diagnostics";
                original_render::SampleRoster();
                if(gContext.stage>=8) {
                    if(!original_camera_owners::Sample()) {
                        Error(original_camera_owners::State().error,original_camera_owners::State().stage);return;
                    }
                    original_enemy_colors::Sample();
                }
                if(original_jett::State().loadoutGranted)original_jett::Sample();
                if(gContext.stage>=8&&!original_infinite_ammo::Poll()) {
                    Error(original_infinite_ammo::State().error,original_infinite_ammo::State().stage);return;
                }
                original_movement::Sample();
                original_simulation::Refresh();
                engine::Event(original_players::activePawn, L"/Script/Engine.Actor.GetLocalRole", &gNativeLocalRole);
                engine::Event(original_players::activePawn, L"/Script/Engine.Actor.GetRemoteRole", &gNativeRemoteRole);
                engine::Event(original_players::activePawn, L"/Script/Engine.Actor.IsActorTickEnabled", &gNativeActorTick);
                engine::Event(original_movement::State().movement, L"/Script/Engine.ActorComponent.IsComponentTickEnabled", &gNativeMovementTick);
                gNativeBeginPlayFlags = original_players::Value<uint8_t>(original_players::activePawn, 0x11c);
                engine::Event(original_players::activePawn, L"/Script/Engine.Actor.HasActorBegunPlay", &gNativeHasBegunPlay);
                engine::Event(original_players::activePawn, L"/Script/Engine.Actor.K2_GetActorLocation", &gOriginalLocation);
                engine::Event(original_players::activePawn, L"/Script/Engine.Actor.GetVelocity", &gOriginalVelocity);
                gOperation="original_hud_diagnostics";
                if (gContext.stage == 6||gContext.stage>=8) original_hud::InspectHUD();
                original_players::WriteDiagnostics((gDirectory + L"original-player-probe.json").c_str());
                original_weapon::WriteDiagnostics((gDirectory + L"original-weapon-probe.json").c_str());
                if (gContext.stage >= 8) for(size_t index=1;index<=original_enemies::HeroCount;++index) original_enemy_weapon::Sample(index);
                if (gContext.stage >= 8) for(size_t index=1;index<=original_enemies::HeroCount;++index) original_enemy_view::Sample(index);
                if (gContext.stage >= 8) original_enemies::WriteDiagnostics((gDirectory+L"original-enemies.json").c_str());
                gOperation="original_diagnostics_publication";
                SaveState();
            }
        }
        return;
    }
    if (gContext.stage >= 2 && !gResourcesCreated) {
        gResourcesCreated = CreateResources();
    }
    if (gResourcesCreated) {
        gOperation = "avatar_input";
        ReadSettings();
        avatar::UpdateInputs(!gInputPaused);
        if (avatar::error) { Error(avatar::error, avatar::stage); return; }
        const ULONGLONG now = GetTickCount64();
        if (now - gLastRenderUpdate < 8) return;
        gLastRenderUpdate = now;
        gOperation = "camera";
        gActualFov = avatar::ActualFOV();
        const auto position = avatar::CameraPosition();
        const auto rotation = avatar::CameraRotation();
        gOperation = "channel_and_abilities";
        ExchangeFrame(position, rotation);
        if (gShared) InterlockedExchange(reinterpret_cast<volatile LONG*>(gShared + 12), 1);
        auto offset = engine::Add(engine::Mul(engine::Forward(rotation), 28), engine::Mul(engine::Right(rotation), 16));
        offset.z -= 20;
        auto gunRotation = rotation;
        gunRotation.x += gRender.pitchRecoil;
        gunRotation.y += gRender.yawRecoil;
        offset = engine::Add(offset, engine::Mul(engine::Forward(rotation), -gRender.kick));
        if (gReloading) {
            const double progress = (static_cast<double>(GetTickCount64()) / 1000.0 - gReloadStarted) / 2.5;
            const double amount = std::sin((std::min)(1.0, (std::max)(0.0, progress)) * 3.141592653589793);
            gunRotation.z += 45 * amount; offset.z -= 25 * amount;
        }
        gOperation = "weapon_presentation";
        engine::SetTransform(gWeapon, engine::Add(position, offset), gunRotation);
        if (jett::State().ultimate_active) presentation::lastShots = gRender.shots;
        presentation::Update(static_cast<double>(GetTickCount64()) / 1000.0, position, rotation,
                             gRender.shots, gRender.kills, gReloading && !jett::State().ultimate_active);
        gOperation = "ability_presentation";
        jett::OnShots(static_cast<double>(now) / 1000.0, gRender.shots, gRender.kills, position, rotation, gRender.ammo);
        gOperation = "weapon_audio";
        if (jett::State().ultimate_active) audio_feedback::diagnostics.lastObservedShots = gRender.shots;
        else audio_feedback::Update(gRender.shots, gReloading, position, rotation);
        if (gContext.stage >= 3) {
            gOperation = "enemy_presentation";
            enemy_presentation::Update(static_cast<double>(now) / 1000.0, gRender.botId, gRender.position,
                                      gRender.yaw, (gRender.flags & 1) != 0, gRender.health, gRender.kills);
        }
    }
    if (count % 120 == 0) SaveState();
}

static int ExceptionFilter(EXCEPTION_POINTERS* information) {
    gError = information->ExceptionRecord->ExceptionCode;
    const auto address = reinterpret_cast<uintptr_t>(information->ExceptionRecord->ExceptionAddress);
    gExceptionRva = address >= engine::image ? address - engine::image : address;
    return EXCEPTION_EXECUTE_HANDLER;
}

static void Tick(void* actor, float delta) {
    gOriginalTick(actor, delta);
    if (gError) return;
    __try { OnGameTick(actor, delta); }
    __except (ExceptionFilter(GetExceptionInformation())) {
        gStage = "callback_error";
        SaveState();
    }
}

static DWORD WINAPI Initialize(void*) {
    wchar_t path[32768]{};
    const DWORD length = GetModuleFileNameW(gModule, path, 32768);
    if (!length || length >= 32768) return 1;
    gDirectory.assign(path, length);
    const auto separator = gDirectory.find_last_of(L"\\/");
    if (separator == std::wstring::npos) return 2;
    gDirectory.resize(separator + 1);
    SaveState();
    const std::wstring contextPath = gDirectory + L"context-" + std::to_wstring(GetCurrentProcessId()) + L".bin";
    HANDLE file = CreateFileW(contextPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) { gError = 1; gStage = "context_missing"; SaveState(); return 3; }
    DWORD bytes = 0;
    const BOOL ok = ReadFile(file, &gContext, sizeof(gContext), &bytes, nullptr);
    CloseHandle(file);
    if (!ok || bytes != sizeof(gContext) || gContext.magic != 0x3152544c41564f4cULL ||
        gContext.version != 1 || gContext.pid != GetCurrentProcessId() ||
        gContext.image != reinterpret_cast<uint64_t>(GetModuleHandleW(nullptr))) {
        gError = 2; gStage = "context_invalid"; SaveState(); return 4;
    }
    if(!training_session::Load(gDirectory)){gError=training_session::error;gStage=training_session::stage;SaveState();return 9;}
    const std::wstring mappingName = L"Local\\ValorantTrainer-" + std::to_wstring(GetCurrentProcessId());
    gMapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 4096, mappingName.c_str());
    if (gMapping) gShared = static_cast<uint8_t*>(MapViewOfFile(gMapping, FILE_MAP_ALL_ACCESS, 0, 0, 4096));
    if (!gShared) { gError = 6; gStage = "channel_creation_failed"; SaveState(); return 8; }
    std::memset(gShared, 0, 4096);
    WriteAt<uint32_t>(gShared, 0, 0x31525456); WriteAt<uint32_t>(gShared, 4, 1);
    WriteAt<uint32_t>(gShared, 8, GetCurrentProcessId());
    original_faults::Initialize(gShared);
    auto object = reinterpret_cast<void***>(gContext.pawn);
    auto expected = reinterpret_cast<void**>(gContext.image + 0xab221a0);
    MEMORY_BASIC_INFORMATION region{};
    if (!VirtualQuery(object, &region, sizeof(region)) || region.State != MEM_COMMIT ||
        region.Protect & (PAGE_NOACCESS | PAGE_GUARD) || *object != expected ||
        expected[0x540 / 8] != reinterpret_cast<void*>(gContext.image + 0x3edab80)) {
        gError = 3; gStage = "pawn_vtable_mismatch"; SaveState(); return 5;
    }
    // Copy the verified DefaultPawn table and its preceding RTTI word.
    auto storage = static_cast<void**>(VirtualAlloc(nullptr, 288 * sizeof(void*), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!storage) { gError = 4; gStage = "table_allocation_failed"; SaveState(); return 6; }
    std::memcpy(storage, expected - 1, 288 * sizeof(void*));
    auto copied = storage + 1;
    gOriginalTick = reinterpret_cast<TickFn>(copied[0x540 / 8]);
    copied[0x540 / 8] = reinterpret_cast<void*>(Tick);
    if (InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(object), copied, expected) != expected) {
        VirtualFree(storage, 0, MEM_RELEASE);
        gError = 5; gStage = "pawn_changed_before_callback"; SaveState(); return 7;
    }
    // The owned process lifetime retains the table and module while referenced.
    return 0;
}

extern "C" __declspec(dllexport) unsigned long TrainerBridgeProtocol() { return 1; }
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        gModule = instance;
        DisableThreadLibraryCalls(instance);
        HANDLE thread = CreateThread(nullptr, 0, Initialize, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
