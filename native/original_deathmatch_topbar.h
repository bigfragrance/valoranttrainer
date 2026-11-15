#pragma once
#include "training_performance.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include "engine_api.h"
#include "original_hud.h"
#include "original_players.h"

// The shipped FFA_DM/FFA_PlayerCard widgets render, sort and bind their own
// cards using the requested timed-streak points. Match totals remain separate.
// This adapter supplies only the stripped local authority TeamStates /
// Points records and invokes their ORIGINAL replication notifications. Every
// TeamState references its player's existing FFATeam, never a new logical team.
// Evidence: work/deathmatch-topbar-static and work/deathmatch-topbar-native.
// All APIs run on the Initialize caller's game thread. No image/script/vtable
// writes, new drawing, UI text replacement, network or game launch is performed.
namespace original_deathmatch_topbar {
inline constexpr size_t MaximumPlayers = 32;
inline constexpr const wchar_t* WidgetClassPath =
    L"/Game/UI/InGame/HUD/GameInfoHUD/FFA/FFA_DM.FFA_DM_C";
inline constexpr const wchar_t* CardClassPath =
    L"/Game/UI/InGame/HUD/GameInfoHUD/FFA/FFA_PlayerCard.FFA_PlayerCard_C";
inline constexpr const wchar_t* MasterClassPath =
    L"/Game/UI/InGame/HUD/GameInfoHUD/GameInfoHUD_Master.GameInfoHUD_Master_C";
inline constexpr uintptr_t TeamStateClassGetter = 0x65af170;
inline constexpr uintptr_t FFATeamClassGetter = 0x6849cb0;
inline constexpr ULONGLONG OriginalCardSettleMilliseconds = 3000;
inline constexpr ULONGLONG PendingSampleMilliseconds = 100;

struct PlayerScore {
    uint32_t index = 0;
    void* playerState = nullptr;
    int32_t streak = 0; // Requested topbar metric: kills within the current time window.
    int32_t matchKills = 0; // Read-only observation; match authority still uses this total.
};
struct Name { uint32_t comparison = 0, display = 0, number = 0; };
struct ObjectArray { void** data = nullptr; int32_t count = 0, capacity = 0; };
struct TeamInfo {
    uint8_t initialRole = 0, teamRole = 0, padding[2]{};
    int32_t wins = 0, points = 0, teamLife = 0;
};
struct ObjectResult { void* value = nullptr; };
struct TeamQuery { void* team = nullptr; void* result = nullptr; };
struct PointsQuery { void* team = nullptr; int32_t result = 0, padding = 0; };
struct CreateWidget { void* world = nullptr; void* type = nullptr; void* controller = nullptr; void* result = nullptr; };
struct ChildQuery { int32_t index = 0, padding = 0; void* result = nullptr; };
struct DesiredSize { double x = 0, y = 0; };
struct TextReadback {
    // One owning FText24, followed by the independent Conv_TextToString result.
    // GetText fills the prefix; the same storage is then the const/ref input.
    alignas(8) uint8_t text[24]{};
    wchar_t* string = nullptr;
    int32_t count = 0, capacity = 0;
};
static_assert(sizeof(TeamInfo) == 16 && offsetof(TeamInfo, points) == 8);
static_assert(sizeof(ObjectArray) == 16 && sizeof(TeamQuery) == 16);
static_assert(sizeof(PointsQuery) == 16 && offsetof(PointsQuery, result) == 8);
// UFunction.ParmsSize ends at the int return, excluding C++ tail alignment.
static_assert(offsetof(PointsQuery, result) + sizeof(int32_t) == 12);
static_assert(sizeof(CreateWidget) == 32 && offsetof(CreateWidget, result) == 24);
static_assert(sizeof(TextReadback) == 40 && offsetof(TextReadback, string) == 24);

struct Entry {
    bool registered = false, created = false, nativePointsVerified = false;
    uint32_t index = 0;
    void* playerState = nullptr;
    void* team = nullptr;
    void* teamState = nullptr;
    void* model = nullptr;
    int32_t streak = 0, matchKills = 0, nativePoints = 0;
};
struct CardObservation {
    void* card = nullptr;
    void* model = nullptr;
    void* playerState = nullptr;
    void* scoreText = nullptr;
    void* hudModel = nullptr; void* gameState = nullptr;
    void* playerTeam = nullptr; void* actorTeam = nullptr;
    int32_t rosterIndex = -1, score = -1, place = -1;
    int32_t playerTeamPoints = -1, actorTeamPoints = -1, displayedNumber = -1;
    uint8_t box = 0;
    int32_t childIndex = -1;
    bool identityVerified = false, scoreVerified = false, placeVerified = false;
    bool hudModelVerified = false, teamQueriesVerified = false;
    bool textQueried = false, textParsed = false, textVerified = false;
    bool textHasSlate = false, textTruncated = false;
    wchar_t displayedText[64]{};
};
struct Diagnostics {
    const char* stage = "original_deathmatch_topbar_not_initialized";
    uint32_t error = 0;
    bool initialized = false, ready = false, restartRequired = false;
    bool originalWidget = false, originalCards = false, widgetInPlayerScreen = false;
    bool coordinatorReference = false, originalPointsVerified = false;
    bool cardBindingsVerified = false, descendingOrderVerified = false;
    bool leftRightLayoutVerified = false, sourceRecordsVerified = false;
    bool nativeLayoutVerified = false, oldMasterCollapsed = false;
    bool pendingOriginalUpdate = false, widgetVisible = false;
    bool allScoreTextsVerified = false, nativeCardOrderVerified = false;
    bool usesExistingFFATeams = true, imagePagesWritten = false;
    void* world = nullptr; void* gameState = nullptr; void* hudModel = nullptr;
    void* controller = nullptr; void* coordinator = nullptr;
    void* widget = nullptr; void* leftBox = nullptr; void* rightBox = nullptr;
    void* oldMaster = nullptr;
    uint8_t oldMasterVisibility = 0xff, masterVisibility = 0xff;
    uint32_t participants = 0, pointsLimit = 0;
    uint32_t teamStatesCreated = 0, authorityTeamRegistrations = 0;
    uint32_t teamComponentNotifications = 0, teamPlayerNotifications = 0;
    uint32_t gameStateTeamNotifications = 0, pointWrites = 0, pointNotifications = 0;
    uint32_t nativePointsQueries = 0, widgetCreations = 0, modelBindings = 0;
    uint32_t cardCount = 0, leftCount = 0, rightCount = 0, layoutSamples = 0;
    uint32_t originalCardModelCatchups = 0, originalCardScoreCatchups = 0;
    uint32_t scoreMismatches = 0, placeMismatches = 0, identityMismatches = 0;
    uint32_t textMismatches = 0, orderMismatches = 0;
    uint32_t pendingWindows = 0, pendingResolutions = 0, pendingSamples = 0;
    uint32_t textReads = 0, textDestructors = 0, stringFrees = 0;
    ULONGLONG pendingSince = 0, pendingAgeMilliseconds = 0;
    uint8_t widgetVisibility = 0xff; float widgetOpacity = 0;
    uint64_t polls = 0, unchangedPolls = 0, exceptionRva = 0;
    int32_t actualModelCount = 0;
    double desiredWidth = 0, desiredHeight = 0;
    const wchar_t* failedEvent = nullptr;
    uint16_t expectedEventSize = 0, actualEventSize = 0;
    CardObservation cards[MaximumPlayers]{};
};
struct Functions {
    void* getTeamState = nullptr; void* getPoints = nullptr;
    void* onRepTeamComponent = nullptr; void* onRepTeamPlayers = nullptr;
    void* onRepTeamStates = nullptr; void* onRepTeamInfo = nullptr;
    void* createWidget = nullptr; void* setHUDModel = nullptr;
    void* setVisibility = nullptr; void* getVisibility = nullptr;
    void* isInViewport = nullptr; void* childCount = nullptr;
    void* childAt = nullptr; void* getPlayerState = nullptr;
    void* forcePrepass = nullptr; void* getDesiredSize = nullptr;
    void* cardSetHUDModel = nullptr; void* cardUpdateScore = nullptr;
    void* playerTeam = nullptr; void* actorTeam = nullptr; void* modelGameState = nullptr;
    void* getText = nullptr; void* textToString = nullptr;
    void* getOpacity = nullptr; void* isVisible = nullptr;
};
inline Diagnostics state{};
inline Functions methods{};
inline std::array<Entry, MaximumPlayers> entries{};
inline DWORD gameThread = 0;
inline ULONGLONG lastLayoutSample = 0;
inline uint32_t inspectionMilliseconds=5000;
inline bool SetInspectionCadence(uint32_t milliseconds){
    if((milliseconds&&milliseconds<100)||milliseconds>10000)return false;
    inspectionMilliseconds=milliseconds;return true;
}
inline void* widgetClass = nullptr; inline void* cardClass = nullptr;
inline void* masterClass = nullptr;
inline int widgetModelOffset = -1, leftOffset = -1, rightOffset = -1;
inline int cardModelOffset = -1, cardScoreOffset = -1, cardPlaceOffset = -1;
inline int cardTextOffset = -1, cardHUDModelOffset = -1, pointsLimitOffset = -1;

template<class T> inline T Read(const void* object, size_t offset) { return original_hud::Read<T>(object, offset); }
template<class T> inline bool Write(void* object, size_t offset, const T& value) { return original_hud::Write(object, offset, value); }
inline bool Fail(uint32_t code, const char* text) { state.error = code; state.stage = text; state.ready = false; return false; }
inline bool Thread() { return gameThread == GetCurrentThreadId() && !state.restartRequired; }
inline const Diagnostics& State() { return state; }
inline bool Ready() { return state.ready && state.error == 0 && !state.restartRequired; }
inline bool ArrayValid(const ObjectArray& a, int maximum) {
    return a.count >= 0 && a.capacity >= a.count && a.capacity <= 4096 && a.count <= maximum
        && (!a.count || original_hud::Readable(a.data, size_t(a.count) * sizeof(void*)));
}
inline bool Contains(const ObjectArray& a, void* value) {
    if (!ArrayValid(a, 256)) return false;
    for (int i = 0; i < a.count; ++i) if (a.data[i] == value) return true;
    return false;
}
inline bool Append(void* owner, size_t offset, void* value, int maximum) {
    auto a = Read<ObjectArray>(owner, offset);
    if (!value || !ArrayValid(a, maximum) || a.count >= maximum) return false;
    if (Contains(a, value)) return true;
    const int32_t previous = a.count, next = previous + 1;
    if (!Write(owner, offset + 8, next)) return false;
    // Original FFieldClass ctor1bef8bd: increment Num, then Grow(array,oldNum)
    // only if Num>Max. 18cd1d0 preserves the old data through native realloc.
    if (next > a.capacity) engine::function<void(*)(void*, int32_t)>(0x18bd090)
        (static_cast<uint8_t*>(owner) + offset, previous);
    a = Read<ObjectArray>(owner, offset);
    if (!ArrayValid(a, maximum) || a.count != next || a.capacity < next || !a.data) return false;
    return Write(a.data, size_t(previous) * sizeof(void*), value) && a.data[previous] == value;
}
inline int Field(void* type, const wchar_t* text, int bytes, int exact = -1) {
    Name n{}; engine::function<void*(*)(void*, const wchar_t*, int)>(0x1a7b720)(&n, text, 1);
    auto f = engine::function<void*(*)(void*, const void*, uint8_t)>(0x1c63870)(type, &n, 3);
    const auto cls = Read<void*>(f, 8); const auto flags = Read<uint64_t>(cls, 0x18);
    const int offset = Read<int32_t>(f, 0x44), size = Read<int32_t>(type, 0x60);
    if (!f || (flags & 0x8001) != 0x8001 || !original_hud::Readable(static_cast<uint8_t*>(f) + 0x20, sizeof(n))
        || std::memcmp(static_cast<uint8_t*>(f) + 0x20, &n, sizeof(n)) != 0
        || Read<int32_t>(f, 0x30) != 1 || Read<int32_t>(f, 0x34) != bytes
        || offset < 0x30 || size <= 0 || size > 65536 || offset + bytes > size
        || (exact >= 0 && offset != exact)) return -1;
    return offset;
}
inline bool Bind(void*& out, const wchar_t* path, uint16_t size, uintptr_t nativeRva = 0) {
    out = engine::Find(path);
    const auto actual = Read<uint16_t>(out, 0xbe);
    if (!out || actual != size || (nativeRva && ((Read<uint32_t>(out, 0xb8) & 0x400) == 0
        || Read<void*>(out, 0xe0) != reinterpret_cast<void*>(engine::image + nativeRva)))) {
        state.failedEvent = path; state.expectedEventSize = size; state.actualEventSize = actual;
        return Fail(3760, "original_deathmatch_topbar_ufunction_layout_mismatch");
    }
    return true;
}
inline bool Event(void* object, void* fn, void* parameters) {
    auto table = Read<void**>(object, 0); auto process = Read<void*>(table, 0x298);
    if (!object || !fn || !process) return false;
    reinterpret_cast<void(*)(void*, void*, void*)>(process)(object, fn, parameters); return true;
}
inline bool BindMethods() {
    return Bind(methods.getTeamState, L"/Script/ShooterGame.ShooterGameState.GetTeamStateForTeam", 16)
        && Bind(methods.getPoints, L"/Script/ShooterGame.ShooterGameState.GetPointsForTeam", 12, 0x6ca3db0)
        && Bind(methods.onRepTeamComponent, L"/Script/ShooterGame.BaseTeamState.OnRep_TeamComponent", 0, 0x65af5d0)
        && Bind(methods.onRepTeamPlayers, L"/Script/ShooterGame.BaseTeamState.OnRep_TeamPlayerStates", 16)
        && Bind(methods.onRepTeamStates, L"/Script/ShooterGame.ShooterGameState.OnRep_TeamStates", 0, 0x6ca51f0)
        && Bind(methods.onRepTeamInfo, L"/Script/ShooterGame.BaseTeamState.OnRep_TeamInfo", 16, 0x65af760)
        && Bind(methods.createWidget, L"/Script/UMG.WidgetBlueprintLibrary.Create", 32)
        && Bind(methods.setHUDModel, L"/Game/UI/InGame/HUD/GameInfoHUD/FFA/FFA_DM.FFA_DM_C.SetHUDModel", 8)
        && Bind(methods.setVisibility, L"/Script/UMG.Widget.SetVisibility", 1)
        && Bind(methods.getVisibility, L"/Script/UMG.Widget.GetVisibility", 1)
        && Bind(methods.isInViewport, L"/Script/UMG.Widget.IsInViewport", 1)
        && Bind(methods.childCount, L"/Script/UMG.PanelWidget.GetChildrenCount", 4)
        && Bind(methods.childAt, L"/Script/UMG.PanelWidget.GetChildAt", 16)
        && Bind(methods.getPlayerState, L"/Script/ShooterGame.HUDPlayerModel.GetPlayerState", 8)
        && Bind(methods.forcePrepass, L"/Script/UMG.Widget.ForceLayoutPrepass", 0)
        && Bind(methods.getDesiredSize, L"/Script/UMG.Widget.GetDesiredSize", 16)
        && Bind(methods.cardSetHUDModel, L"/Game/UI/InGame/HUD/GameInfoHUD/FFA/FFA_PlayerCard.FFA_PlayerCard_C.SetHUDModel", 8)
        && Bind(methods.cardUpdateScore, L"/Game/UI/InGame/HUD/GameInfoHUD/FFA/FFA_PlayerCard.FFA_PlayerCard_C.UpdatePlayerScore", 0)
        && Bind(methods.playerTeam, L"/Script/ShooterGame.AresPlayerStateBase.GetTeamComponent", 8, 0x642a620)
        && Bind(methods.actorTeam, L"/Script/ShooterGame.BaseTeamComponent.GetTeamComponent", 16, 0x65a9d40)
        && Bind(methods.modelGameState, L"/Script/ShooterGame.CoordinatedHUDModel.GetGameState", 8, 0x68c31e0)
        && Bind(methods.getText, L"/Script/UMG.TextBlock.GetText", 24, 0x3bb39f0)
        && Bind(methods.textToString, L"/Script/Engine.KismetTextLibrary.Conv_TextToString", 40, 0x3e25250)
        && Bind(methods.getOpacity, L"/Script/UMG.Widget.GetRenderOpacity", 4, 0x3bc1290)
        && Bind(methods.isVisible, L"/Script/UMG.Widget.IsVisible", 1, 0x3bc1670);
}
inline bool Layouts() {
    auto ts = engine::Class(TeamStateClassGetter);
    auto gs = Read<void*>(state.gameState, 0x10);
    auto coordinator = Read<void*>(state.coordinator, 0x10);
    if (Field(ts, L"TeamPlayerStates", 16, 0x468) < 0 || Field(ts, L"TeamComponent", 8, 0x478) < 0
        || Field(ts, L"TeamInfo", 16, 0x4b0) < 0 || Field(gs, L"TeamStates", 16, 0x720) < 0
        || Field(coordinator, L"HUDElements", 16, 0x58) < 0) return Fail(3761, "original_deathmatch_topbar_native_record_layout_mismatch");
    widgetModelOffset = Field(widgetClass, L"HUDModel", 8);
    leftOffset = Field(widgetClass, L"LeftBox", 8); rightOffset = Field(widgetClass, L"RightBox", 8);
    cardModelOffset = Field(cardClass, L"PlayerModel", 8); cardScoreOffset = Field(cardClass, L"CachedScore", 4);
    cardPlaceOffset = Field(cardClass, L"CachedPlace", 4); cardTextOffset = Field(cardClass, L"PlayerScore", 8);
    cardHUDModelOffset = Field(cardClass, L"HUDModel", 8);
    pointsLimitOffset = Field(gs, L"NumPointsToWinGame", 4);
    state.nativeLayoutVerified = widgetModelOffset >= 0 && leftOffset >= 0 && rightOffset >= 0
        && cardModelOffset >= 0 && cardScoreOffset >= 0 && cardPlaceOffset >= 0 && cardTextOffset >= 0
        && cardHUDModelOffset >= 0 && pointsLimitOffset >= 0;
    return state.nativeLayoutVerified ? true : Fail(3762, "original_deathmatch_topbar_original_widget_fields_missing");
}
inline Entry* ByPlayerState(void* ps) {
    for (auto& e : entries) if (e.registered && e.playerState == ps) return &e;
    return nullptr;
}
inline bool AcceptRecords(const PlayerScore* records, size_t count, bool initial) {
    if (!records || count < 1 || count > MaximumPlayers || (!initial && count != state.participants))
        return Fail(3763, "original_deathmatch_topbar_score_roster_size_invalid");
    uint32_t mask = 0;
    for (size_t n = 0; n < count; ++n) {
        const auto& r = records[n];
        if (r.index >= MaximumPlayers || r.streak < 0 || r.matchKills < r.streak || (mask & (uint32_t(1) << r.index)))
            return Fail(3764, "original_deathmatch_topbar_score_record_invalid");
        mask |= uint32_t(1) << r.index;
        auto& e = entries[r.index];
        if (initial) {
            if (!original_hud::IsA(r.playerState, 0x6cc4040)
                || !Contains(Read<ObjectArray>(state.gameState, 0x480), r.playerState))
                return Fail(3765, "original_deathmatch_topbar_requires_registered_original_player_state");
            for (const auto& other : entries) if (other.registered && other.playerState == r.playerState)
                return Fail(3766, "original_deathmatch_topbar_duplicate_player_state");
            auto team = Read<void*>(r.playerState, 0x6a8);
            if (!original_hud::IsA(team, FFATeamClassGetter) || Read<void*>(team, 0xc8) != r.playerState)
                return Fail(3767, "original_deathmatch_topbar_requires_existing_private_ffa_team");
            e.registered = true; e.index = r.index; e.playerState = r.playerState; e.team = team;
        } else if (!e.registered || e.playerState != r.playerState)
            return Fail(3768, "original_deathmatch_topbar_original_player_identity_changed");
        e.matchKills = r.matchKills;
    }
    state.participants = static_cast<uint32_t>(count); state.sourceRecordsVerified = true; return true;
}
inline bool RefreshModels() {
    const auto models = Read<ObjectArray>(state.hudModel, 0x50);
    state.actualModelCount = models.count;
    if (!ArrayValid(models, int(MaximumPlayers)) || models.count != int(state.participants))
        return Fail(3769, "original_deathmatch_topbar_hud_model_roster_not_equal_to_match_roster");
    uint32_t seen = 0;
    for (int i = 0; i < models.count; ++i) {
        if (!original_hud::IsA(models.data[i], 0x68ba540)) return Fail(3770, "original_deathmatch_topbar_original_player_model_missing");
        ObjectResult ps{};
        if (!Event(models.data[i], methods.getPlayerState, &ps)) return Fail(3771, "original_deathmatch_topbar_model_identity_query_failed");
        auto e = ByPlayerState(ps.value);
        if (!e || (seen & (uint32_t(1) << e->index))) return Fail(3772, "original_deathmatch_topbar_model_roster_identity_mismatch");
        seen |= uint32_t(1) << e->index; e->model = models.data[i];
    }
    return true;
}
inline bool EnsureTeamStates() {
    const auto nativeClass = engine::Class(TeamStateClassGetter);
    if (!nativeClass || Read<int32_t>(nativeClass, 0x60) != 0x4d8 || (Read<uint32_t>(nativeClass, 0xdc) & 1))
        return Fail(3773, "original_deathmatch_topbar_original_team_state_class_mismatch");
    for (auto& e : entries) if (e.registered) {
        TeamQuery query{e.team, nullptr};
        if (!Event(state.gameState, methods.getTeamState, &query)) return Fail(3774, "original_deathmatch_topbar_original_team_state_query_failed");
        e.teamState = query.result;
        if (!e.teamState) {
            const engine::Transform transform{};
            e.teamState = engine::function<void*(*)(void*, void*, const engine::Transform*, uint8_t, void*, uint8_t)>(0x43fada0)
                (state.world, nativeClass, &transform, 1, state.gameState, 0);
            if (!original_hud::IsA(e.teamState, TeamStateClassGetter)) return Fail(3775, "original_deathmatch_topbar_original_team_state_spawn_failed");
            // AuthAddTeamState/AuthAddPlayerState are stripped here. Supply the
            // real existing private FFATeam and its real PS, before normal Finish.
            if (!Write(e.teamState, 0x478, e.team) || !Append(e.teamState, 0x468, e.playerState, 32))
                return Fail(3776, "original_deathmatch_topbar_local_team_record_link_failed");
            engine::function<void(*)(void*, const engine::Transform*, bool, void*, uint8_t)>(0x3ef6cb0)
                (e.teamState, &transform, false, nullptr, 0);
            if (!Append(state.gameState, 0x720, e.teamState, 128)) return Fail(3777, "original_deathmatch_topbar_local_team_state_registration_failed");
            e.created = true; ++state.teamStatesCreated; ++state.authorityTeamRegistrations;
        }
        if (!original_hud::IsA(e.teamState, TeamStateClassGetter) || Read<void*>(e.teamState, 0x478) != e.team
            || !Contains(Read<ObjectArray>(e.teamState, 0x468), e.playerState))
            return Fail(3778, "original_deathmatch_topbar_existing_team_state_identity_mismatch");
    }
    for (auto& e : entries) if (e.registered && e.created) {
        ObjectArray previous{};
        if (!Event(e.teamState, methods.onRepTeamComponent, nullptr)) return Fail(3779, "original_deathmatch_topbar_original_team_component_notification_failed");
        ++state.teamComponentNotifications;
        if (!Event(e.teamState, methods.onRepTeamPlayers, &previous)) return Fail(3780, "original_deathmatch_topbar_original_team_player_notification_failed");
        ++state.teamPlayerNotifications;
    }
    if (state.authorityTeamRegistrations) {
        if (!Event(state.gameState, methods.onRepTeamStates, nullptr)) return Fail(3781, "original_deathmatch_topbar_original_team_states_notification_failed");
        ++state.gameStateTeamNotifications;
    }
    return true;
}
inline bool SynchronizePoints(const PlayerScore* records, size_t count, bool& changed) {
    changed = false; state.originalPointsVerified = false;
    for (size_t i = 0; i < count; ++i) {
        auto& e = entries[records[i].index];
        if (Read<void*>(e.playerState, 0x6a8) != e.team || Read<void*>(e.teamState, 0x478) != e.team)
            return Fail(3782, "original_deathmatch_topbar_private_team_identity_changed");
        auto previous = Read<TeamInfo>(e.teamState, 0x4b0);
        if (previous.points != records[i].streak) {
            if (!Write(e.teamState, 0x4b8, records[i].streak)) return Fail(3783, "original_deathmatch_topbar_local_points_record_write_failed");
            ++state.pointWrites;
            if (!Event(e.teamState, methods.onRepTeamInfo, &previous)) return Fail(3784, "original_deathmatch_topbar_original_points_notification_failed");
            ++state.pointNotifications; changed = true;
        }
        e.streak = records[i].streak;
        PointsQuery points{e.team, -1, 0}; TeamQuery team{e.team, nullptr};
        if (!Event(state.gameState, methods.getPoints, &points) || !Event(state.gameState, methods.getTeamState, &team))
            return Fail(3785, "original_deathmatch_topbar_original_points_readback_failed");
        ++state.nativePointsQueries; e.nativePoints = points.result;
        e.nativePointsVerified = team.result == e.teamState && points.result == e.streak;
        if (!e.nativePointsVerified) return Fail(3786, "original_deathmatch_topbar_original_points_do_not_match_timed_streak");
    }
    state.originalPointsVerified = true; return true;
}
inline bool EnsureWidget() {
    auto elements = Read<ObjectArray>(state.coordinator, 0x58);
    if (!ArrayValid(elements, 128)) return Fail(3787, "original_deathmatch_topbar_coordinated_element_array_invalid");
    for (int i = 0; i < elements.count; ++i) {
        auto type = Read<void*>(elements.data[i], 0x10);
        if (type == widgetClass) {
            if (state.widget) return Fail(3788, "original_deathmatch_topbar_duplicate_original_ffa_roots");
            state.widget = elements.data[i];
        }
        if (type == masterClass) {
            if (state.oldMaster) return Fail(3789, "original_deathmatch_topbar_duplicate_original_master_roots");
            state.oldMaster = elements.data[i];
        }
    }
    if (!state.widget) {
        CreateWidget create{state.world, widgetClass, state.controller, nullptr};
        auto library = engine::DefaultObject(engine::Class(0x3b1d050));
        if (!Event(library, methods.createWidget, &create) || !original_hud::Derived(Read<void*>(create.result, 0x10), widgetClass))
            return Fail(3790, "original_deathmatch_topbar_original_widget_creation_failed");
        state.widget = create.result; ++state.widgetCreations;
        // Keep a real reflected coordinator strong reference, in addition to the
        // original widget's own AddToPlayerScreen lifecycle/Slate ownership.
        if (!Append(state.coordinator, 0x58, state.widget, 128)) return Fail(3791, "original_deathmatch_topbar_coordinator_reference_failed");
    }
    state.coordinatorReference = Contains(Read<ObjectArray>(state.coordinator, 0x58), state.widget);
    auto actualModel = Read<void*>(state.widget, size_t(widgetModelOffset));
    if (actualModel && actualModel != state.hudModel) return Fail(3792, "original_deathmatch_topbar_widget_already_bound_to_other_model");
    if (!actualModel) {
        ObjectResult model{state.hudModel};
        if (!Event(state.widget, methods.setHUDModel, &model)) return Fail(3793, "original_deathmatch_topbar_original_model_binding_failed");
        ++state.modelBindings;
    }
    state.originalWidget = Read<void*>(state.widget, 0x10) == widgetClass
        && Read<void*>(state.widget, size_t(widgetModelOffset)) == state.hudModel;
    state.leftBox = Read<void*>(state.widget, size_t(leftOffset)); state.rightBox = Read<void*>(state.widget, size_t(rightOffset));
    uint8_t inViewport = 0;
    state.widgetInPlayerScreen = Event(state.widget, methods.isInViewport, &inViewport) && inViewport != 0;
    return state.originalWidget && state.coordinatorReference && state.widgetInPlayerScreen
        ? true : Fail(3794, "original_deathmatch_topbar_original_player_screen_attachment_not_observed");
}
inline bool ReadScoreText(CardObservation& o) {
    if (!original_hud::IsA(o.scoreText, 0x3bb25c0)) return false;
    TextReadback value{};
    // GetText reads the actual Slate text attribute when its Slate widget exists;
    // it otherwise uses the original UObject FText. No text or score is written.
    const bool got = Event(o.scoreText, methods.getText, &value);
    const bool converted = got && Event(engine::DefaultObject(engine::Class(0x3e1ec70)), methods.textToString, &value);
    o.textQueried = got && converted; o.textHasSlate = Read<void*>(o.scoreText, 0x3a0) != nullptr;
    if (o.textQueried && value.count > 0 && value.capacity >= value.count && value.count <= 4096
        && original_hud::Readable(value.string, size_t(value.count) * sizeof(wchar_t))) {
        const int32_t length = value.count - 1;
        o.textTruncated = length >= int32_t(sizeof(o.displayedText) / sizeof(wchar_t));
        const int32_t copy = o.textTruncated ? int32_t(sizeof(o.displayedText) / sizeof(wchar_t)) - 1 : length;
        for (int32_t i = 0; i < copy; ++i) o.displayedText[i] = value.string[i];
        o.displayedText[copy] = 0;
        int32_t number = 0; bool numeric = !o.textTruncated && length > 0 && value.string[length] == 0;
        for (int32_t i = 0; i < copy && numeric; ++i) {
            const auto ch = o.displayedText[i];
            if (ch < L'0' || ch > L'9' || number > ((std::numeric_limits<int32_t>::max)() - int32_t(ch - L'0')) / 10) numeric = false;
            else number = number * 10 + int32_t(ch - L'0');
        }
        o.textParsed = numeric; if (numeric) o.displayedNumber = number;
    }
    // Original wrappers3bb39f0/3e25250 prove this ownership: FText24 owns one
    // shared reference; converted FString is an independent native allocation.
    if (value.string) { engine::function<void(*)(void*)>(0x1915720)(value.string); ++state.stringFrees; }
    if (got) { engine::function<void(*)(void*)>(0x18a31e0)(&value); ++state.textDestructors; }
    ++state.textReads; return o.textQueried;
}
inline bool ObserveCards() {
    if (!original_hud::IsA(state.leftBox, 0x3b7de20) || !original_hud::IsA(state.rightBox, 0x3b7de20))
        return Fail(3795, "original_deathmatch_topbar_original_horizontal_boxes_missing");
    int32_t counts[2]{}; void* boxes[]{state.leftBox, state.rightBox};
    for (int b = 0; b < 2; ++b) if (!Event(boxes[b], methods.childCount, &counts[b])) return Fail(3796, "original_deathmatch_topbar_child_count_query_failed");
    state.leftCount = counts[0] >= 0 ? uint32_t(counts[0]) : 0; state.rightCount = counts[1] >= 0 ? uint32_t(counts[1]) : 0;
    state.leftRightLayoutVerified = counts[0] == int32_t((state.participants + 1) / 2)
        && counts[1] == int32_t(state.participants / 2);
    if (!state.leftRightLayoutVerified) return Fail(3797, "original_deathmatch_topbar_original_split_does_not_include_every_player");
    uint32_t seen = 0, at = 0; int32_t previousScore = (std::numeric_limits<int32_t>::max)();
    int32_t previousNativeScore = (std::numeric_limits<int32_t>::max)();
    state.cardCount = 0; std::memset(state.cards, 0, sizeof(state.cards));
    state.scoreMismatches = state.placeMismatches = state.identityMismatches = 0;
    state.textMismatches = state.orderMismatches = 0;
    state.originalCards = false; state.cardBindingsVerified = false; state.descendingOrderVerified = false;
    state.allScoreTextsVerified = false; state.nativeCardOrderVerified = false;
    uint8_t visible = 0, inViewport = 0;
    if (!Event(state.widget, methods.getVisibility, &state.widgetVisibility)
        || !Event(state.widget, methods.getOpacity, &state.widgetOpacity)
        || !Event(state.widget, methods.isVisible, &visible)
        || !Event(state.widget, methods.isInViewport, &inViewport))
        return Fail(3818, "original_deathmatch_topbar_actual_visibility_query_failed");
    state.widgetVisible = visible != 0; state.widgetInPlayerScreen = inViewport != 0;
    for (int b = 0; b < 2; ++b) for (int i = 0; i < counts[b]; ++i) {
        ChildQuery child{i, 0, nullptr};
        if (!Event(boxes[b], methods.childAt, &child) || Read<void*>(child.result, 0x10) != cardClass)
            return Fail(3798, "original_deathmatch_topbar_original_card_class_mismatch");
        auto& o = state.cards[at]; o = {}; o.card = child.result; o.box = uint8_t(b); o.childIndex = i;
        o.model = Read<void*>(o.card, size_t(cardModelOffset));
        o.hudModel = Read<void*>(o.card, size_t(cardHUDModelOffset));
        bool caughtUp = false;
        if (!o.hudModel) {
            // The original designer pre-places14 cards. FFA_DM.SetChildrenHUDModels
            // forwards only its Timer; only its NEW-card branch sets card models.
            // Complete the missing original per-card initialization exactly once.
            ObjectResult model{state.hudModel};
            if (!Event(o.card, methods.cardSetHUDModel, &model)) return Fail(3819, "original_deathmatch_topbar_original_card_model_catchup_failed");
            ++state.originalCardModelCatchups; caughtUp = true;
            o.hudModel = Read<void*>(o.card, size_t(cardHUDModelOffset));
        }
        o.hudModelVerified = o.hudModel == state.hudModel;
        if (!o.hudModelVerified) return Fail(3820, "original_deathmatch_topbar_card_bound_to_different_hud_model");
        ObjectResult ps{};
        if (!Event(o.model, methods.getPlayerState, &ps)) return Fail(3799, "original_deathmatch_topbar_card_player_query_failed");
        o.playerState = ps.value; auto e = ByPlayerState(ps.value);
        if (!e) return Fail(3800, "original_deathmatch_topbar_card_player_is_not_registered");
        o.identityVerified = e->model == o.model && !(seen & (uint32_t(1) << e->index));
        if (!o.identityVerified) ++state.identityMismatches;
        seen |= uint32_t(1) << e->index; o.rosterIndex = int32_t(e->index);
        if (caughtUp) {
            // SetHUDModel binds original OnPointsUpdated but does not immediately
            // call UpdatePlayerScore. Consume current real points through that
            // original leaf once; never fill CachedScore or Text ourselves.
            if (!Event(o.card, methods.cardUpdateScore, nullptr)) return Fail(3821, "original_deathmatch_topbar_original_card_score_catchup_failed");
            ++state.originalCardScoreCatchups;
        }
        ObjectResult cardGS{}, playerTeam{}; TeamQuery actorTeam{o.playerState, nullptr};
        if (!Event(o.hudModel, methods.modelGameState, &cardGS)
            || !Event(o.playerState, methods.playerTeam, &playerTeam)
            || !Event(engine::DefaultObject(engine::Class(0x65a0b40)), methods.actorTeam, &actorTeam))
            return Fail(3822, "original_deathmatch_topbar_card_native_context_query_failed");
        o.gameState = cardGS.value; o.playerTeam = playerTeam.value; o.actorTeam = actorTeam.result;
        PointsQuery direct{o.playerTeam, -1, 0}, actor{o.actorTeam, -1, 0};
        if (!Event(o.gameState, methods.getPoints, &direct) || !Event(o.gameState, methods.getPoints, &actor))
            return Fail(3823, "original_deathmatch_topbar_card_original_points_query_failed");
        o.playerTeamPoints = direct.result; o.actorTeamPoints = actor.result;
        o.teamQueriesVerified = o.gameState == state.gameState && o.playerTeam == e->team && o.actorTeam == e->team
            && o.playerTeamPoints == e->streak && o.actorTeamPoints == e->streak;
        o.score = Read<int32_t>(o.card, size_t(cardScoreOffset)); o.place = Read<int32_t>(o.card, size_t(cardPlaceOffset));
        o.scoreText = Read<void*>(o.card, size_t(cardTextOffset));
        o.scoreVerified = o.score == e->streak && e->nativePointsVerified && o.scoreText && o.teamQueriesVerified;
        o.placeVerified = o.place == int32_t(at + 1);
        if (!o.scoreVerified) ++state.scoreMismatches;
        if (!o.placeVerified) ++state.placeMismatches;
        if (o.score > previousScore || e->streak > previousNativeScore) ++state.orderMismatches;
        if (!ReadScoreText(o)) return Fail(3824, "original_deathmatch_topbar_original_score_text_query_failed");
        o.textVerified = o.textParsed && o.textHasSlate && o.displayedNumber == e->streak && o.displayedNumber == o.score;
        if (!o.textVerified) ++state.textMismatches;
        previousScore = o.score; previousNativeScore = e->streak; ++at; state.cardCount = at;
    }
    state.cardCount = at; state.originalCards = at == state.participants;
    state.cardBindingsVerified = state.originalCards && !state.identityMismatches;
    state.descendingOrderVerified = state.originalCards && !state.orderMismatches;
    state.nativeCardOrderVerified = state.originalCards && !state.orderMismatches;
    state.allScoreTextsVerified = state.originalCards && !state.textMismatches;
    // Observe the layout from the original Slate frame. Forcing a recursive
    // prepass after every score notification duplicates the engine's work.
    DesiredSize size{};
    if (!Event(state.widget, methods.getDesiredSize, &size)) return Fail(3803, "original_deathmatch_topbar_original_desired_size_query_failed");
    state.desiredWidth = size.x; state.desiredHeight = size.y; ++state.layoutSamples; lastLayoutSample = GetTickCount64();
    const bool settled = state.originalCards && state.cardBindingsVerified && state.descendingOrderVerified
        && state.allScoreTextsVerified && !state.scoreMismatches && !state.placeMismatches
        && state.widgetInPlayerScreen && state.widgetVisible && state.widgetOpacity > 0.001f;
    if (!settled) {
        if (!state.pendingOriginalUpdate) { state.pendingOriginalUpdate = true; state.pendingSince = lastLayoutSample; ++state.pendingWindows; }
        state.pendingAgeMilliseconds = lastLayoutSample - state.pendingSince; ++state.pendingSamples;
        if (state.pendingAgeMilliseconds >= OriginalCardSettleMilliseconds)
            return Fail(3825, "original_deathmatch_topbar_original_card_update_timeout");
        state.ready = false; state.error = 0; state.stage = "original_deathmatch_topbar_waiting_original_card_update";
    } else {
        if (state.pendingOriginalUpdate) ++state.pendingResolutions;
        state.pendingOriginalUpdate = false; state.pendingSince = 0; state.pendingAgeMilliseconds = 0;
    }
    return true;
}
inline bool CollapseOldMaster() {
    if (!state.oldMaster) return true;
    if (Read<void*>(state.oldMaster, 0x10) != masterClass) return Fail(3804, "original_deathmatch_topbar_master_class_identity_changed");
    uint8_t current = 0xff;
    if (!Event(state.oldMaster, methods.getVisibility, &current)) return Fail(3805, "original_deathmatch_topbar_master_visibility_query_failed");
    if (state.oldMasterVisibility == 0xff) state.oldMasterVisibility = current;
    // ESlateVisibility::Collapsed=1. No other coordinated element is touched.
    if (current != 1) {
        uint8_t collapsed = 1;
        if (!Event(state.oldMaster, methods.setVisibility, &collapsed)
            || !Event(state.oldMaster, methods.getVisibility, &current)) return Fail(3806, "original_deathmatch_topbar_master_collapse_failed");
    }
    state.masterVisibility = current; state.oldMasterCollapsed = current == 1;
    return state.oldMasterCollapsed ? true : Fail(3807, "original_deathmatch_topbar_master_collapse_not_observed");
}
inline bool PollImpl(const PlayerScore* records, size_t count) {
    if (!state.initialized || !Thread()) return Fail(3808, "original_deathmatch_topbar_poll_requires_initialized_game_thread");
    ++state.polls;
    if (Read<void*>(state.world, 0x178) != state.gameState || Read<void*>(state.hudModel, 0x240) != state.gameState)
        return Fail(3809, "original_deathmatch_topbar_original_world_or_model_changed");
    if (!AcceptRecords(records, count, false)) return false;
    bool changed = false;
    for (size_t i = 0; i < count; ++i) if (entries[records[i].index].streak != records[i].streak) { changed = true; break; }
    const bool sampleDue = state.pendingOriginalUpdate
        ? GetTickCount64()-lastLayoutSample>=PendingSampleMilliseconds
        : inspectionMilliseconds&&GetTickCount64()-lastLayoutSample>=inspectionMilliseconds;
    if (!changed && !sampleDue && (state.ready || state.pendingOriginalUpdate)) { ++state.unchangedPolls; return true; }
    training_performance::Scope timing(training_performance::Slot::TopbarModels);
    if (!RefreshModels()) return false;
    timing.Step(training_performance::Slot::TopbarPoints);
    if (!SynchronizePoints(records, count, changed))return false;
    timing.Step(training_performance::Slot::TopbarAudit);
    if ((sampleDue||!state.ready)&&!ObserveCards())return false;
    if(!CollapseOldMaster())return false;
    state.error = 0; state.ready = !state.pendingOriginalUpdate;
    state.stage = state.pendingOriginalUpdate ? "original_deathmatch_topbar_waiting_original_card_update"
        : "original_deathmatch_topbar_original_scores_cards_and_order_verified"; return true;
}
inline bool InitializeImpl(void* realWorld, void* actualHUDModel, const PlayerScore* records, size_t count) {
    if (state.initialized) {
        if (state.world != realWorld || state.hudModel != actualHUDModel) return Fail(3810, "original_deathmatch_topbar_context_changed");
        return PollImpl(records, count);
    }
    if (!engine::image || !original_hud::IsA(actualHUDModel, 0x68ba790)
        || !original_hud::Readable(realWorld, 0x180)) return Fail(3811, "original_deathmatch_topbar_original_context_missing");
    gameThread = GetCurrentThreadId(); state.world = realWorld; state.hudModel = actualHUDModel;
    state.gameState = Read<void*>(realWorld, 0x178); state.controller = Read<void*>(actualHUDModel, 0x250);
    auto ui = Read<void*>(state.controller, 0xc30); state.coordinator = Read<void*>(ui, 0x60);
    if (!original_hud::IsA(state.gameState, 0x6c9d280) || Read<void*>(actualHUDModel, 0x240) != state.gameState
        || !original_hud::IsA(state.controller, 0x6943db0) || !original_hud::IsA(state.coordinator, 0x68ba840)
        || Read<void*>(state.coordinator, 0x68) != actualHUDModel)
        return Fail(3812, "original_deathmatch_topbar_requires_existing_original_coordinator_graph");
    if (engine::function<int(*)(void*)>(0x3efc820)(state.gameState) != 0 || Read<uint8_t>(state.gameState, 0x224) != 3)
        return Fail(3813, "original_deathmatch_topbar_requires_original_local_standalone_authority");
    widgetClass = original_hud::LoadClass(0x63c3d90, WidgetClassPath);
    cardClass = original_hud::LoadClass(0x63c3d90, CardClassPath);
    masterClass = original_hud::LoadClass(0x63c3d90, MasterClassPath);
    if (!widgetClass || !cardClass || !masterClass) return Fail(3814, "original_deathmatch_topbar_original_widget_classes_missing");
    if (!BindMethods() || !Layouts() || !AcceptRecords(records, count, true) || !RefreshModels()) return false;
    const auto limit = Read<int32_t>(state.gameState, size_t(pointsLimitOffset));
    if (limit != 40) return Fail(3815, "original_deathmatch_topbar_original_points_limit_not40");
    state.pointsLimit = uint32_t(limit);
    if (!EnsureTeamStates()) return false;
    bool changed = false;
    if (!SynchronizePoints(records, count, changed) || !EnsureWidget() || !ObserveCards() || !CollapseOldMaster()) return false;
    state.initialized = true; state.ready = !state.pendingOriginalUpdate; state.error = 0;
    state.stage = state.pendingOriginalUpdate ? "original_deathmatch_topbar_waiting_original_card_update"
        : "original_deathmatch_topbar_original_scores_cards_and_order_verified"; return true;
}
inline LONG RecordException(EXCEPTION_POINTERS* exception) {
    const auto address = exception && exception->ExceptionRecord ? reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress) : 0;
    state.exceptionRva = address >= engine::image && address - engine::image < 0x10000000 ? address - engine::image : address;
    state.restartRequired = true; Fail(3816, "original_deathmatch_topbar_native_exception_requires_new_session"); return EXCEPTION_EXECUTE_HANDLER;
}
inline bool Initialize(void* realWorld, void* actualHUDModel, const PlayerScore* records, size_t count) {
    __try { return InitializeImpl(realWorld, actualHUDModel, records, count); }
    __except (RecordException(GetExceptionInformation())) { return false; }
}
inline bool Poll(const PlayerScore* records, size_t count) {
    __try { return PollImpl(records, count); }
    __except (RecordException(GetExceptionInformation())) { return false; }
}
inline void JsonText(FILE* f, const wchar_t* text) {
    std::fputc('"', f);
    for (size_t i = 0; text && text[i] && i < 63; ++i) {
        const auto c = static_cast<uint16_t>(text[i]);
        if (c == '"' || c == '\\') { std::fputc('\\', f); std::fputc(char(c), f); }
        else if (c < 32 || c >= 127) std::fprintf(f, "\\u%04x", unsigned(c));
        else std::fputc(char(c), f);
    }
    std::fputc('"', f);
}
inline void WriteDiagnostics(const wchar_t* path) {
    FILE* f = nullptr; if (!path || _wfopen_s(&f, path, L"wb") || !f) return;
    const auto b = [](bool x) { return x ? "true" : "false"; };
    std::fprintf(f, "{\"schema_version\":2,\"score_semantics\":\"timed_kill_streak\",\"match_result_metric\":\"total_kills\",\"stage\":\"%s\",\"error\":%u,\"initialized\":%s,\"ready\":%s,\"restart_required\":%s,\"exception_rva\":%llu", state.stage, state.error, b(state.initialized), b(state.ready), b(state.restartRequired), static_cast<unsigned long long>(state.exceptionRva));
    std::fprintf(f,",\"inspection_interval_ms\":%u,\"card_observation_at_ms\":%llu",inspectionMilliseconds,static_cast<unsigned long long>(lastLayoutSample));
    std::fprintf(f, ",\"world\":\"%p\",\"game_state\":\"%p\",\"hud_model\":\"%p\",\"controller\":\"%p\",\"coordinator\":\"%p\",\"widget\":\"%p\",\"old_master\":\"%p\"", state.world, state.gameState, state.hudModel, state.controller, state.coordinator, state.widget, state.oldMaster);
    std::fprintf(f, ",\"participants\":%u,\"actual_model_count\":%d,\"points_limit\":%u,\"original_widget\":%s,\"original_cards\":%s,\"widget_in_player_screen\":%s,\"coordinator_reference\":%s,\"original_points_verified\":%s,\"card_bindings_verified\":%s,\"descending_order_verified\":%s,\"left_right_layout_verified\":%s,\"source_records_verified\":%s,\"native_layout_verified\":%s", state.participants, state.actualModelCount, state.pointsLimit, b(state.originalWidget), b(state.originalCards), b(state.widgetInPlayerScreen), b(state.coordinatorReference), b(state.originalPointsVerified), b(state.cardBindingsVerified), b(state.descendingOrderVerified), b(state.leftRightLayoutVerified), b(state.sourceRecordsVerified), b(state.nativeLayoutVerified));
    std::fprintf(f, ",\"old_master_collapsed\":%s,\"old_master_visibility\":%u,\"master_visibility\":%u,\"team_states_created\":%u,\"authority_team_registrations\":%u,\"team_component_notifications\":%u,\"team_player_notifications\":%u,\"game_state_team_notifications\":%u,\"point_writes\":%u,\"point_notifications\":%u,\"native_points_queries\":%u", b(state.oldMasterCollapsed), unsigned(state.oldMasterVisibility), unsigned(state.masterVisibility), state.teamStatesCreated, state.authorityTeamRegistrations, state.teamComponentNotifications, state.teamPlayerNotifications, state.gameStateTeamNotifications, state.pointWrites, state.pointNotifications, state.nativePointsQueries);
    std::fprintf(f, ",\"widget_creations\":%u,\"model_bindings\":%u,\"card_count\":%u,\"left_count\":%u,\"right_count\":%u,\"layout_samples\":%u,\"polls\":%llu,\"unchanged_polls\":%llu,\"desired_size\":[%.9g,%.9g],\"uses_existing_ffa_teams\":true,\"new_logical_teams\":false,\"image_pages_written\":false,\"original_scripts_modified\":false,\"rendered_pixels_verified\":false", state.widgetCreations, state.modelBindings, state.cardCount, state.leftCount, state.rightCount, state.layoutSamples, static_cast<unsigned long long>(state.polls), static_cast<unsigned long long>(state.unchangedPolls), state.desiredWidth, state.desiredHeight);
    std::fprintf(f, ",\"pending_original_update\":%s,\"pending_age_ms\":%llu,\"settle_limit_ms\":%llu,\"pending_windows\":%u,\"pending_resolutions\":%u,\"pending_samples\":%u,\"original_card_model_catchups\":%u,\"original_card_score_catchups\":%u", b(state.pendingOriginalUpdate), static_cast<unsigned long long>(state.pendingAgeMilliseconds), static_cast<unsigned long long>(OriginalCardSettleMilliseconds), state.pendingWindows, state.pendingResolutions, state.pendingSamples, state.originalCardModelCatchups, state.originalCardScoreCatchups);
    std::fprintf(f, ",\"score_mismatches\":%u,\"place_mismatches\":%u,\"identity_mismatches\":%u,\"text_mismatches\":%u,\"order_mismatches\":%u,\"all_score_texts_verified\":%s,\"native_card_order_verified\":%s,\"widget_visibility\":%u,\"widget_opacity\":%.9g,\"widget_visible\":%s,\"text_reads\":%u,\"text_destructors\":%u,\"string_frees\":%u", state.scoreMismatches, state.placeMismatches, state.identityMismatches, state.textMismatches, state.orderMismatches, b(state.allScoreTextsVerified), b(state.nativeCardOrderVerified), unsigned(state.widgetVisibility), state.widgetOpacity, b(state.widgetVisible), state.textReads, state.textDestructors, state.stringFrees);
    std::fprintf(f, ",\"expected_event_size\":%u,\"actual_event_size\":%u,\"failed_event\":\"", unsigned(state.expectedEventSize), unsigned(state.actualEventSize));
    if (state.failedEvent) for (auto p = state.failedEvent; *p; ++p) std::fputc(*p >= 32 && *p < 127 ? char(*p) : '?', f);
    std::fputs("\",\"players\":[", f); bool comma = false;
    for (const auto& e : entries) if (e.registered) {
        if (comma) std::fputc(',', f); comma = true;
        std::fprintf(f, "{\"index\":%u,\"player_state\":\"%p\",\"existing_ffa_team\":\"%p\",\"team_state\":\"%p\",\"model\":\"%p\",\"created_team_state\":%s,\"streak\":%d,\"match_kills\":%d,\"original_points\":%d,\"native_points_verified\":%s}", e.index, e.playerState, e.team, e.teamState, e.model, b(e.created), e.streak, e.matchKills, e.nativePoints, b(e.nativePointsVerified));
    }
    std::fputs("],\"cards_left_to_right\":[", f);
    for (uint32_t i = 0; i < state.cardCount && i < MaximumPlayers; ++i) {
        if (i) std::fputc(',', f); const auto& c = state.cards[i];
        std::fprintf(f, "{\"rank\":%u,\"roster_index\":%d,\"box\":\"%s\",\"child_index\":%d,\"card\":\"%p\",\"player_model\":\"%p\",\"player_state\":\"%p\",\"score_text\":\"%p\",\"cached_score\":%d,\"cached_place\":%d,\"identity_verified\":%s,\"score_verified\":%s,\"place_verified\":%s", i + 1, c.rosterIndex, c.box ? "right" : "left", c.childIndex, c.card, c.model, c.playerState, c.scoreText, c.score, c.place, b(c.identityVerified), b(c.scoreVerified), b(c.placeVerified));
        std::fprintf(f, ",\"parent_hud_model\":\"%p\",\"parent_hud_model_matches\":%s,\"card_game_state\":\"%p\",\"player_team\":\"%p\",\"actor_team\":\"%p\",\"player_team_points\":%d,\"actor_team_points\":%d,\"native_team_queries_verified\":%s,\"text_queried\":%s,\"text_has_slate\":%s,\"text_parsed\":%s,\"text_verified\":%s,\"text_truncated\":%s,\"displayed_number\":%d,\"displayed_text\":", c.hudModel, b(c.hudModelVerified), c.gameState, c.playerTeam, c.actorTeam, c.playerTeamPoints, c.actorTeamPoints, b(c.teamQueriesVerified), b(c.textQueried), b(c.textHasSlate), b(c.textParsed), b(c.textVerified), b(c.textTruncated), c.displayedNumber);
        JsonText(f, c.displayedText); std::fputc('}', f);
    }
    std::fputs("]}\n", f); std::fclose(f);
}
} // namespace original_deathmatch_topbar
