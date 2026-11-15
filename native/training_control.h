#pragma once
#include <Windows.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <array>

// Independent of the original 4 KiB observation/fault channel. Only the game's
// thread dispatches commands. The host publishes one command at a time and the
// game publishes state with a sequence lock; neither side follows remote pointers.
namespace training_control {
inline constexpr uint64_t Magic = 0x32304c5443525456ULL; // VTRCTL02
inline constexpr uint32_t Version = 2, Bytes = 131072;
inline constexpr size_t CommandOffset = 256, CommandCapacity = 16384;
inline constexpr size_t StateOffset = 32768, StateCapacity = Bytes - StateOffset;
enum class Command : uint32_t {
    Configure = 1, Routes = 2, Weapon = 3, Skin = 4, Effect = 5,
    RestartMatch = 6, ValidateRoutes = 7, ToggleBuyMenu = 8, BuyWeapon = 9,
    SaveSkinSelection = 10, ValidateSkinCatalog = 11, ResolveMapPoint = 12,
    ToggleOptions = 13, CloseActiveUI = 14, TestPlayerNearBot = 15,
    TestAudioRegistration = 16, HumanBots = 17, SpectateBot = 18
};
enum class Result : uint32_t { Applied = 0, Pending = 1, Invalid = 2, Unavailable = 3, Failed = 4 };
enum class Mode : uint32_t { HoldAngle = 0, Deathmatch = 1 };
enum class Style : uint32_t { Jump = 0, Wide = 1, Short = 2, Crouch = 3, Peek = 4, Random = 5 };
enum Flags : uint32_t { ReturnFire = 1, Invincible = 2, InfiniteAmmo = 4, Paused = 8 };
#pragma pack(push, 4)
struct Guid { uint32_t words[4]{}; };
struct Point { float x = 0, y = 0, z = 0; };
struct Configuration {
    uint32_t revision = 0, mode = 0, totalPlayers = 14, flags = InfiniteAmmo;
    float peeksPerMinute = 20, respawnSeconds = 1.5f, streakSeconds = 5;
    float reactionMs = 260, reactionSpreadMs = 100;
    float aimDegreesPerSecond = 220, aimSpread = 100;
    float accuracy = 0.65f, accuracySpread = 0.22f;
    float movement = 0.65f, movementSpread = 0.25f;
    float reserved = 0;
    uint32_t style = static_cast<uint32_t>(Style::Random), seed = 1;
};
struct Route {
    uint32_t id = 0, style = 6, enabled = 1; // 6 inherits the global style.
    float weight = 1;
    Point start{}, expose{}, retreat{};
};
struct RouteSet { uint32_t count = 0, revision = 0; Route items[128]{}; };
// Versioned extension in the unused upper 64 route slots. Legacy zero-filled
// RouteSets remain valid and the Initial and Route wire sizes stay unchanged.
struct PracticeEntry {
    uint32_t wave=0, slot=0, route=0, style=6, fire=0;
    float minimum=0, maximum=0;
};
struct PracticePlan {
    uint32_t magic=0, version=0, mode=0, count=0;
    float interval=1;uint32_t generation=0;
    PracticeEntry entries[64]{};
};
static_assert(sizeof(PracticePlan)<=64*sizeof(Route));
inline PracticePlan ReadPracticePlan(const RouteSet& routes){
    PracticePlan result{};std::memcpy(&result,&routes.items[64],sizeof(result));if(!result.magic)return {};return result;
}
struct Skin { Guid weapon{}, skin{}, level{}, chroma{}; };
struct Effect { char key[80]{}; uint32_t mode = 0; float value = 0; };
struct Initial {
    uint64_t magic = Magic;
    uint32_t version = Version, bytes = 0;
    Configuration config{};
    RouteSet routes{};
    char mapId[64]{};
    uint32_t spawnCount = 0;
    Point spawns[64]{};
    float spawnYaws[64]{};
};
#pragma pack(pop)
static_assert(sizeof(Configuration) == 72);
static_assert(sizeof(Route) == 52);
static_assert(sizeof(Skin) == 64);
static_assert(sizeof(RouteSet) <= CommandCapacity);

struct Header {
    uint64_t magic; uint32_t version, bytes, pid, capabilities;
    uint64_t started;
    volatile LONG commandSequence; uint32_t commandKind, commandBytes, reserved;
    volatile LONG acknowledgement; uint32_t result;
    volatile LONG stateSequence; uint32_t stateBytes;
    uint64_t heartbeat;
    uint32_t flags, appliedRevision;
    char message[176];
};
static_assert(sizeof(Header) == CommandOffset);
using Dispatch = Result(*)(Command, const uint8_t*, uint32_t, char*, size_t);
inline HANDLE mapping = nullptr;
inline uint8_t* memory = nullptr;
inline uint32_t lastCommand = 0;
inline std::array<uint8_t, CommandCapacity> commandCopy{};

inline bool Initialize() {
    if (memory) return true;
    wchar_t name[100]{};
    swprintf_s(name, L"Local\\ValorantTrainerControlV2_%lu", GetCurrentProcessId());
    mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, Bytes, name);
    if (!mapping || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (mapping) CloseHandle(mapping);
        mapping = nullptr; return false;
    }
    memory = static_cast<uint8_t*>(MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, Bytes));
    if (!memory) { CloseHandle(mapping); mapping = nullptr; return false; }
    std::memset(memory, 0, Bytes);
    auto h = reinterpret_cast<Header*>(memory);
    h->version = Version; h->bytes = Bytes; h->pid = GetCurrentProcessId();
    h->started = h->heartbeat = GetTickCount64();
    MemoryBarrier(); h->magic = Magic;
    return true;
}
inline void Poll(Dispatch dispatch) {
    if (!memory || !dispatch) return;
    auto h = reinterpret_cast<Header*>(memory);
    h->heartbeat = GetTickCount64();
    const auto sequence = static_cast<uint32_t>(InterlockedCompareExchange(&h->commandSequence, 0, 0));
    if (!sequence || sequence == lastCommand) return;
    const uint32_t kind = h->commandKind, length = h->commandBytes;
    Result result = Result::Invalid;
    char message[sizeof(h->message)]{};
    if (length <= CommandCapacity) {
        std::memcpy(commandCopy.data(), memory + CommandOffset, length);
        MemoryBarrier();
        if (sequence != static_cast<uint32_t>(InterlockedCompareExchange(&h->commandSequence, 0, 0))) return;
        result = dispatch(static_cast<Command>(kind), commandCopy.data(), length, message, sizeof(message));
    } else strcpy_s(message, "command_too_large");
    h->result = static_cast<uint32_t>(result);
    std::memcpy(h->message, message, sizeof(message)); h->message[sizeof(h->message)-1] = 0;
    lastCommand = sequence;
    MemoryBarrier(); InterlockedExchange(&h->acknowledgement, static_cast<LONG>(sequence));
}
inline bool Publish(const char* json, size_t length, uint32_t revision = 0, uint32_t flags = 0) {
    if (!memory || !json || length >= StateCapacity) return false;
    auto h = reinterpret_cast<Header*>(memory);
    InterlockedIncrement(&h->stateSequence);
    std::memcpy(memory + StateOffset, json, length);
    memory[StateOffset + length] = 0;
    h->stateBytes = static_cast<uint32_t>(length); h->appliedRevision = revision;
    h->heartbeat = GetTickCount64(); h->flags = flags;
    MemoryBarrier(); InterlockedIncrement(&h->stateSequence);
    return true;
}
inline void Close() {
    if (memory) UnmapViewOfFile(memory);
    if (mapping) CloseHandle(mapping);
    memory = nullptr; mapping = nullptr;
}
}
