#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <limits>

namespace engine {
inline uintptr_t image = 0;
template<class T> T function(uintptr_t rva) { return reinterpret_cast<T>(image + rva); }
struct Vec { double x, y, z; };
struct alignas(16) Transform {
    double rotation[4]{0, 0, 0, 1};
    Vec translation{}; double pad1 = 0;
    Vec scale{1, 1, 1}; double pad2 = 0;
};
static_assert(sizeof(Transform) == 96 && alignof(Transform) == 16);
inline void* Class(uintptr_t getter) { return function<void*(*)()>(getter)(); }
inline void* Find(const wchar_t* name) {
    return function<void*(*)(void*, void*, const wchar_t*, bool)>(0x1d07dd0)(nullptr, reinterpret_cast<void*>(~uintptr_t(0)), name, false);
}
inline void* Load(void* type, const wchar_t* name) {
    return function<void*(*)(void*, void*, const wchar_t*, const wchar_t*, uint32_t, void*, bool, void*)>(0x1d0b900)
        (type, nullptr, name, nullptr, 0, nullptr, true, nullptr);
}
inline bool Event(void* object, const wchar_t* name, void* parameters) {
    void* fn = Find(name);
    if (!object || !fn) return false;
    auto table = *reinterpret_cast<void***>(object);
    reinterpret_cast<void(*)(void*, void*, void*)>(table[0x298 / 8])(object, fn, parameters);
    return true;
}
inline void* Spawn(void* world, const Vec& location, void* owner = nullptr) {
    Transform transform; transform.translation = location;
    void* actor = function<void*(*)(void*, void*, const Transform*, uint8_t, void*, uint8_t)>(0x43fada0)
        (world, Class(0x3683900), &transform, 1, owner, 0);
    if (actor) function<void(*)(void*, const Transform*, bool, void*, uint8_t)>(0x3ef6cb0)(actor, &transform, false, nullptr, 0);
    return actor;
}
inline void* AddComponent(void* actor, uintptr_t getter) {
    struct alignas(16) Params {
        void* type; uint8_t manual = 0; uint8_t pad[7]{};
        Transform transform;
        uint8_t deferred = 0; uint8_t pad2[7]{}; void* result = nullptr;
    } params{};
    static_assert(sizeof(Params) == 0x80);
    params.type = Class(getter);
    if (!Event(actor, L"/Script/Engine.Actor.AddComponentByClass", &params)) return nullptr;
    return params.result;
}
inline void* AddSkeletal(void* actor) { return AddComponent(actor, 0x385f750); }
inline void* AddStatic(void* actor) { return AddComponent(actor, 0x312abd0); }
inline bool SetStatic(void* component, void* mesh) {
    struct Params { void* mesh; uint8_t result = 0; uint8_t pad[7]{}; } params{mesh};
    return Event(component, L"/Script/Engine.StaticMeshComponent.SetStaticMesh", &params) && params.result;
}
inline bool SetSkeletal(void* component, void* mesh) {
    return Event(component, L"/Script/Engine.SkeletalMeshComponent.SetSkeletalMeshAsset", &mesh);
}
inline bool SetMaterial(void* component, void* material) {
    struct Params { int index = 0; int pad = 0; void* material; } params{0, 0, material};
    return Event(component, L"/Script/Engine.PrimitiveComponent.SetMaterial", &params);
}
inline bool DisableCollision(void* component) {
    uint8_t value = 0;
    return Event(component, L"/Script/Engine.PrimitiveComponent.SetCollisionEnabled", &value);
}
inline bool Hidden(void* actor, bool hidden) {
    uint8_t value = hidden ? 1 : 0;
    return Event(actor, L"/Script/Engine.Actor.SetActorHiddenInGame", &value);
}
inline bool AttachSocket(void* component, void* parent, const wchar_t* socket) {
    alignas(8) uint8_t params[0x20]{};
    std::memcpy(params, &parent, sizeof(parent));
    function<void*(*)(void*, const wchar_t*, int)>(0x1a7b720)(params + 8, socket, 1);
    params[0x14] = 2; params[0x15] = 2; params[0x16] = 0;
    return Event(component, L"/Script/Engine.SceneComponent.K2_AttachToComponent", params) && params[0x18];
}
inline void* DefaultObject(void* type) {
    auto result = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(type) + 0x120);
    if (!result) {
        auto table = *reinterpret_cast<void***>(type);
        result = reinterpret_cast<void*(*)(void*)>(table[0x420 / 8])(type);
    }
    return result;
}
inline int DisableSpawnBarriers(void* world) {
    struct Params { void* world; void* type; void** actors = nullptr; int count = 0; int capacity = 0; } params{};
    static_assert(sizeof(Params) == 0x20);
    params.world = world; params.type = Class(0x65a77e0);
    if (!Event(DefaultObject(Class(0x43a9670)), L"/Script/Engine.GameplayStatics.GetAllActorsOfClass", &params)) return -1;
    if (params.count < 0 || params.count > 128 || params.capacity < params.count || (params.count && !params.actors)) return -2;
    uint8_t disabled = 0;
    for (int index = 0; index < params.count; ++index) {
        if (!Event(params.actors[index], L"/Script/Engine.Actor.SetActorEnableCollision", &disabled) || !Hidden(params.actors[index], true)) return -3;
    }
    // One bounded engine-owned result allocation remains for the owned session.
    return params.count;
}
inline int Trace(void* worldContext, const Vec& start, const Vec& direction, double maximum, double& distance, void* ignoredActor = nullptr) {
    alignas(16) uint8_t params[0x178]{};
    const Vec end{start.x + direction.x*maximum, start.y + direction.y*maximum, start.z + direction.z*maximum};
    std::memcpy(params, &worldContext, 8);
    std::memcpy(params + 8, &start, 24);
    std::memcpy(params + 0x20, &end, 24);
    params[0x39] = 1; params[0x148] = 1;
    struct ActorArray { void** data; int count; int capacity; } ignored{&ignoredActor, 1, 1};
    if (ignoredActor) std::memcpy(params + 0x40, &ignored, sizeof(ignored));
    void* receiver = DefaultObject(Class(0x45aa110));
    if (!receiver || !Event(receiver, L"/Script/Engine.KismetSystemLibrary.LineTraceSingle", params)) return -1;
    if (!params[0x170] || !(params[0x105] & 1)) { distance = std::numeric_limits<double>::infinity(); return 0; }
    float observed = 0;
    std::memcpy(&observed, params + 0x60, 4);
    distance = observed;
    return std::isfinite(distance) && distance >= 0 ? 1 : -2;
}
inline bool SetTransform(void* actor, const Vec& location, const Vec& rotation) {
    alignas(16) uint8_t params[0x130]{};
    std::memcpy(params, &location, sizeof(location));
    std::memcpy(params + 0x18, &rotation, sizeof(rotation));
    params[0x128] = 1;
    return Event(actor, L"/Script/Engine.Actor.K2_SetActorLocationAndRotation", params) && params[0x129];
}
inline Vec Add(const Vec& a, const Vec& b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
inline Vec Mul(const Vec& a, double n) { return {a.x*n,a.y*n,a.z*n}; }
inline Vec Forward(const Vec& rotation) {
    constexpr double r = 3.14159265358979323846 / 180.0;
    const double p = rotation.x*r, y = rotation.y*r;
    return {std::cos(p)*std::cos(y), std::cos(p)*std::sin(y), std::sin(p)};
}
inline Vec Right(const Vec& rotation) {
    constexpr double r = 3.14159265358979323846 / 180.0;
    const double y = rotation.y*r;
    return {-std::sin(y), std::cos(y), 0};
}
}
