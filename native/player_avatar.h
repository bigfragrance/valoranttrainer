#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include "engine_api.h"
#include <cstddef>
#include <algorithm>

// Pinned image ABI: reports/duel1-character-call-layouts.json and its
// avatar_extra_functions appendix. Call only from the owned game's game thread.
namespace avatar {
struct Options {
    double sensitivity = 0.35;
    float fieldOfView = 103.0f;
    float slowWalkScale = 0.45f;
    double meshYaw = -90.0;
    double meshFootOffset = 0.0;
    bool snapInitialPositionToFloor = true;
};
inline Options options;
inline void* pawn = nullptr;
inline unsigned error = 0;
inline const char* stage = "avatar_not_initialized";
inline bool ready = false;
inline void* controller = nullptr;
inline void* previousPawn = nullptr;
inline void* mesh = nullptr;
inline void* capsule = nullptr;
inline bool jumpHeld = false;
inline float capsuleHalfHeight = 88.0f;
inline float baseEyeHeight = 64.0f;
inline bool positionFromCameraManager = false;
inline bool rotationFromCameraManager = false;
inline engine::Vec lastPosition{}, lastRotation{};

template<class T> inline T Read(const void* object, size_t offset) {
    T value{};
    if (object) std::memcpy(&value, reinterpret_cast<const uint8_t*>(object) + offset, sizeof(value));
    return value;
}
template<class T> inline void Put(uint8_t* buffer, size_t offset, const T& value) {
    std::memcpy(buffer + offset, &value, sizeof(value));
}
inline bool Finite(const engine::Vec& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
inline bool Fail(unsigned code, const char* message) { error = code; stage = message; return false; }
inline bool QueryVector(void* receiver, const wchar_t* method, engine::Vec& result) {
    engine::Vec p{};
    if (!engine::Event(receiver, method, &p) || !Finite(p)) return false;
    result = p;
    return true;
}
inline void* CameraManager() { return Read<void*>(controller, 0x528); }
inline float ActualFOV() {
    void* manager = CameraManager();
    if (!manager) return 0;
    // Pinned GetCameraCacheView() is vslot 0x7a0, returns the current POV.
    auto table = *reinterpret_cast<void***>(manager);
    void* pov = reinterpret_cast<void*(*)(void*)>(table[0x7a0 / 8])(manager);
    const float value = Read<float>(pov, 0x30);
    return std::isfinite(value) ? value : 0;
}
inline bool ApplyFOV() {
    void* manager = CameraManager();
    if (!manager || !std::isfinite(options.fieldOfView)) return false;
    // Generated DefaultFOV descriptor bd0c8a0 -> +484; UpdateViewTarget
    // 490e777 copies this field into the actual POV every frame.
    std::memcpy(reinterpret_cast<uint8_t*>(manager) + 0x484, &options.fieldOfView, 4);
    return engine::Event(controller, L"/Script/Engine.PlayerController.FOV", &options.fieldOfView);
}
inline bool SetViewTarget(void* target) {
    alignas(8) uint8_t p[0x18]{};
    Put(p, 0, target); // BlendTime 0: immediate view target, no outgoing lock.
    return engine::Event(controller, L"/Script/Engine.PlayerController.SetViewTargetWithBlend", p);
}
inline bool Collision(void* actor, bool enabled) {
    uint8_t p = enabled ? 1 : 0;
    return engine::Event(actor, L"/Script/Engine.Actor.SetActorEnableCollision", &p);
}
inline bool RelativeBodyTransform(void* component, double halfHeight) {
    alignas(8) uint8_t p[0x130]{};
    const engine::Vec location{0, 0, -halfHeight + options.meshFootOffset};
    const engine::Vec rotation{0, options.meshYaw, 0};
    Put(p, 0, location); Put(p, 0x18, rotation); p[0x128] = 1;
    return engine::Event(component, L"/Script/Engine.SceneComponent.K2_SetRelativeLocationAndRotation", p);
}
inline engine::Vec CameraPosition() {
    engine::Vec result{};
    positionFromCameraManager = QueryVector(CameraManager(), L"/Script/Engine.PlayerCameraManager.GetCameraLocation", result);
    if (positionFromCameraManager) return lastPosition = result;
    if (QueryVector(pawn ? pawn : previousPawn, L"/Script/Engine.Actor.K2_GetActorLocation", result)) {
        // Character actor location is the capsule center, never the eye position.
        if (pawn) {
            const float eye = Read<float>(pawn, 0x474);
            result.z += std::isfinite(eye) && eye > 0 ? eye : baseEyeHeight;
        }
        return lastPosition = result;
    }
    return lastPosition;
}
inline engine::Vec CameraRotation() {
    engine::Vec result{};
    rotationFromCameraManager = QueryVector(CameraManager(), L"/Script/Engine.PlayerCameraManager.GetCameraRotation", result);
    if (rotationFromCameraManager) return lastRotation = result;
    if (QueryVector(controller, L"/Script/Engine.Controller.GetControlRotation", result)) return lastRotation = result;
    return lastRotation;
}
inline bool AbortInitialization(unsigned code, const char* message) {
    ready = false;
    if (pawn && Read<void*>(controller, 0x4b0) == pawn) {
        void* restore = previousPawn;
        engine::Event(controller, L"/Script/Engine.Controller.Possess", &restore);
        if (Read<void*>(controller, 0x4b0) == pawn) return Fail(code, "avatar_failed_restore_previous_pawn");
        SetViewTarget(previousPawn);
    }
    if (pawn) engine::Event(pawn, L"/Script/Engine.Actor.K2_DestroyActor", nullptr);
    pawn = nullptr; mesh = nullptr; capsule = nullptr;
    return Fail(code, message);
}
inline bool Initialize(void* world, void* inController, void* oldPawn) {
    if (ready) return true;
    if (error) return false; // Prevent a failed initialization from spawning every frame.
    controller = inController; previousPawn = oldPawn;
    if (!engine::image || !world || !controller || !oldPawn) return Fail(301, "avatar_context_missing");
    stage = "avatar_loading_character_and_jett";
    void* type = engine::Class(0x40e2560);
    void* bodyAsset = engine::Load(engine::Class(0x3815ac0),
        L"/Game/Characters/Wushu/S0/3P/Models/TP_Wushu_S0_Skelmesh.TP_Wushu_S0_Skelmesh");
    if (!type || !bodyAsset) return Fail(302, "avatar_character_or_jett_missing");
    void* defaults = engine::DefaultObject(type);
    if (!defaults) return Fail(303, "avatar_character_defaults_missing");
    baseEyeHeight = Read<float>(defaults, 0x474);
    if (!std::isfinite(baseEyeHeight) || baseEyeHeight <= 0 || baseEyeHeight > 300)
        return Fail(304, "avatar_invalid_eye_height");
    void* defaultCapsule = Read<void*>(defaults, 0x500);
    if (!engine::Event(defaultCapsule, L"/Script/Engine.CapsuleComponent.GetUnscaledCapsuleHalfHeight", &capsuleHalfHeight)
        || !std::isfinite(capsuleHalfHeight) || capsuleHalfHeight <= 0 || capsuleHalfHeight > 500)
        return Fail(305, "avatar_invalid_capsule_height");

    const auto eye = CameraPosition();
    auto rotation = CameraRotation();
    if (!Finite(eye) || !Finite(rotation)) return Fail(306, "avatar_initial_camera_invalid");
    engine::Transform transform;
    transform.translation = {eye.x, eye.y, eye.z - baseEyeHeight};
    constexpr double radians = 3.14159265358979323846 / 180.0;
    transform.rotation[2] = std::sin(rotation.y * radians * 0.5);
    transform.rotation[3] = std::cos(rotation.y * radians * 0.5);
    if (options.snapInitialPositionToFloor) {
        double distance = 0;
        // The old pawn is the trace context, so the verified ignore-self flag excludes it.
        if (engine::Trace(oldPawn, eye, {0, 0, -1}, 2000, distance) == 1)
            transform.translation.z = eye.z - distance + capsuleHalfHeight + 2.0;
    }
    stage = "avatar_spawning_character";
    pawn = engine::function<void*(*)(void*, void*, const engine::Transform*, uint8_t, void*, uint8_t)>(0x43fada0)
        (world, type, &transform, 1, nullptr, 0);
    if (!pawn) return Fail(307, "avatar_character_spawn_failed");
    engine::function<void(*)(void*, const engine::Transform*, bool, void*, uint8_t)>(0x3ef6cb0)
        (pawn, &transform, false, nullptr, 0);
    mesh = Read<void*>(pawn, 0x4f0); capsule = Read<void*>(pawn, 0x500);
    if (!mesh || !capsule || !Read<void*>(pawn, 0x4f8)) return AbortInitialization(308, "avatar_native_components_missing");
    stage = "avatar_configuring_jett_body";
    if (!engine::SetSkeletal(mesh, bodyAsset) || !engine::DisableCollision(mesh))
        return AbortInitialization(309, "avatar_jett_body_setup_failed");
    if (!engine::Event(capsule, L"/Script/Engine.CapsuleComponent.GetUnscaledCapsuleHalfHeight", &capsuleHalfHeight)
        || !std::isfinite(capsuleHalfHeight) || capsuleHalfHeight <= 0 || capsuleHalfHeight > 500
        || !RelativeBodyTransform(mesh, capsuleHalfHeight))
        return AbortInitialization(310, "avatar_jett_foot_alignment_failed");
    uint8_t ownerNoSee = 1;
    if (!engine::Event(mesh, L"/Script/Engine.PrimitiveComponent.SetOwnerNoSee", &ownerNoSee))
        return AbortInitialization(311, "avatar_owner_visibility_failed");
    stage = "avatar_possessing_character";
    if (!engine::Event(controller, L"/Script/Engine.Controller.Possess", &pawn)
        || Read<void*>(controller, 0x4b0) != pawn)
        return AbortInitialization(312, "avatar_possession_not_observed");
    if (!SetViewTarget(pawn) || !ApplyFOV())
        return AbortInitialization(313, "avatar_camera_setup_failed");
    rotation.x = (std::max)(-89.0, (std::min)(89.0, rotation.x)); rotation.z = 0;
    if (!engine::Event(controller, L"/Script/Engine.Controller.SetControlRotation", &rotation))
        return AbortInitialization(314, "avatar_control_rotation_failed");
    if (!Collision(oldPawn, false)) return AbortInitialization(315, "avatar_old_pawn_collision_failed");
    if (!engine::Hidden(oldPawn, true)) {
        Collision(oldPawn, true);
        return AbortInitialization(316, "avatar_old_pawn_hide_failed");
    }
    // Keep the old DefaultPawn alive and ticking; its instance owns the parent's callback.
    lastRotation = rotation; lastPosition = eye;
    jumpHeld = false; ready = true; stage = "avatar_ready";
    return true;
}
inline void UpdateInputs(bool inputAllowed = true) {
    if (!ready || error || !pawn || !controller) return;
    if (Read<void*>(controller, 0x4b0) != pawn) { Fail(317, "avatar_possession_lost"); return; }
    DWORD foregroundPid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &foregroundPid);
    if (!inputAllowed || foregroundPid != GetCurrentProcessId()) {
        if (jumpHeld) engine::Event(pawn, L"/Script/Engine.Character.StopJumping", nullptr);
        jumpHeld = false;
        return;
    }
    engine::Vec rotation{};
    if (!QueryVector(controller, L"/Script/Engine.Controller.GetControlRotation", rotation)) {
        Fail(318, "avatar_control_rotation_unavailable"); return;
    }
    struct MouseDelta { float x = 0, y = 0; } mouse;
    static_assert(sizeof(MouseDelta) == 8);
    if (!engine::Event(controller, L"/Script/Engine.PlayerController.GetInputMouseDelta", &mouse)) {
        Fail(319, "avatar_mouse_delta_unavailable"); return;
    }
    if (std::isfinite(mouse.x) && std::isfinite(mouse.y) && std::isfinite(options.sensitivity)) {
        rotation.y = std::remainder(rotation.y + static_cast<double>(mouse.x) * options.sensitivity, 360.0);
        rotation.x = (std::max)(-89.0, (std::min)(89.0, rotation.x + static_cast<double>(mouse.y) * options.sensitivity));
        rotation.z = 0;
        if (!engine::Event(controller, L"/Script/Engine.Controller.SetControlRotation", &rotation)) {
            Fail(320, "avatar_mouse_rotation_failed"); return;
        }
    }
    const auto key = [](int value) { return (GetAsyncKeyState(value) & 0x8000) != 0; };
    const double forward = static_cast<double>(key('W')) - static_cast<double>(key('S'));
    const double right = static_cast<double>(key('D')) - static_cast<double>(key('A'));
    if (forward || right) {
        const engine::Vec yawOnly{0, rotation.y, 0};
        auto direction = engine::Add(engine::Mul(engine::Forward(yawOnly), forward), engine::Mul(engine::Right(yawOnly), right));
        const double length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        direction = engine::Mul(direction, 1.0 / length);
        struct Movement { engine::Vec direction; float scale; uint8_t force = 0; uint8_t pad[3]{}; } move{};
        static_assert(sizeof(Movement) == 0x20 && offsetof(Movement, scale) == 0x18 && offsetof(Movement, force) == 0x1c);
        move.direction = direction; move.scale = key(VK_SHIFT) ? options.slowWalkScale : 1.0f;
        if (!engine::Event(pawn, L"/Script/Engine.Pawn.AddMovementInput", &move)) {
            Fail(321, "avatar_movement_input_failed"); return;
        }
    }
    const bool jump = key(VK_SPACE);
    if (jump != jumpHeld) {
        if (!engine::Event(pawn, jump ? L"/Script/Engine.Character.Jump" : L"/Script/Engine.Character.StopJumping", nullptr)) {
            Fail(322, "avatar_jump_input_failed"); return;
        }
        jumpHeld = jump;
    }
}
}
