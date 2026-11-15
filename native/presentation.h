#pragma once
#include "engine_api.h"
#include <cstddef>
#include <algorithm>

// Fixed-image layouts: reports/duel1-presentation-call-layouts.json.
// Resource identities and original curve keys: duel1-jett-champions-assets.json.
// All entry points run on the existing owned game's game thread.
namespace presentation {
struct Options {
    engine::Vec cameraLocalOffset{0, 0, 0};
    engine::Vec sourceRotationOffset{0, 0, 0};
    // Candidate native rig point, explicitly tunable until visual alignment passes.
    const wchar_t* weaponSocket = L"WeaponGameOverride";
    bool alignWeaponToRig = true;
    bool autoChampionForSoloDrill = true;
    bool muzzleEffects = true;
};
inline Options options;
inline unsigned error = 0;
inline const char* stage = "not_initialized";
inline bool ready = false;
inline void* world = nullptr;
inline void* armsActor = nullptr;
inline void* core = nullptr;
inline void* hands = nullptr;
inline void* gunActor = nullptr;
inline void* gun = nullptr;
inline void* gunMID = nullptr;
inline void* outlineBodyMID = nullptr;
inline void* outlineHandleMID = nullptr;
inline void* muzzle = nullptr;
inline void* idle = nullptr;
inline void* fire = nullptr;
inline void* reload = nullptr;
inline void* gunFire = nullptr;
inline void* gunReload = nullptr;
inline float fireLength = 0;
inline float reloadLength = 0;
inline uint32_t lastShots = 0, lastKills = 0;
inline uint32_t fireAnimations = 0, reloadAnimations = 0, emittersSpawned = 0;
inline uint32_t emitterFailures = 0, transformFailures = 0;
inline bool wasReloading = false, champion = false, championExplicit = false;
inline double animationUntil = 0, leaderSince = -1, killSince = -1000, lastMaterialUpdate = -1;
inline bool animationActive = false;

template<class T> inline void Put(uint8_t* p, size_t offset, const T& value) {
    std::memcpy(p + offset, &value, sizeof(value));
}
inline void Name(uint8_t* destination, const wchar_t* value) {
    engine::function<void*(*)(void*, const wchar_t*, int)>(0x1a7b720)(destination, value, 1);
}
inline bool Fail(unsigned code, const char* message) { error = code; stage = message; return false; }
inline bool Visibility(void* component, bool visible) {
    uint8_t p[2]{static_cast<uint8_t>(visible), 0};
    return engine::Event(component, L"/Script/Engine.SceneComponent.SetVisibility", p);
}
inline bool Play(void* component, void* asset, bool loop) {
    alignas(8) uint8_t p[0x10]{};
    Put(p, 0, asset); p[8] = loop ? 1 : 0;
    return engine::Event(component, L"/Script/Engine.SkeletalMeshComponent.PlayAnimation", p);
}
inline bool SingleNode(void* component) {
    uint8_t p[2]{1, 1};
    return engine::Event(component, L"/Script/Engine.SkeletalMeshComponent.SetAnimationMode", p);
}
inline float Length(void* animation) {
    float result = 0;
    if (!engine::Event(animation, L"/Script/Engine.AnimationAsset.GetPlayLength", &result)) return 0;
    return std::isfinite(result) && result > 0 && result < 60 ? result : 0;
}
inline bool Leader(void* follower, void* source) {
    alignas(8) uint8_t p[0x10]{};
    Put(p, 0, source); p[8] = 1; p[9] = 0;
    return engine::Event(follower, L"/Script/Engine.SkinnedMeshComponent.SetLeaderPoseComponent", p);
}
inline void* DynamicMaterial(void* component, void* material) {
    alignas(8) uint8_t p[0x28]{};
    Put(p, 8, material);
    if (!engine::Event(component, L"/Script/Engine.PrimitiveComponent.CreateDynamicMaterialInstance", p)) return nullptr;
    void* result = nullptr; std::memcpy(&result, p + 0x20, 8); return result;
}
inline bool Scalar(void* material, const wchar_t* parameter, float value) {
    alignas(4) uint8_t p[0x10]{};
    Name(p, parameter); Put(p, 0xc, value);
    return engine::Event(material, L"/Script/Engine.MaterialInstanceDynamic.SetScalarParameterValue", p);
}
inline bool Socket(void* component, const wchar_t* name, uint8_t space, engine::Transform& result) {
    alignas(16) uint8_t p[0x70]{};
    Name(p, name); p[0xc] = space;
    if (!engine::Event(component, L"/Script/Engine.SceneComponent.GetSocketTransform", p)) return false;
    std::memcpy(&result, p + 0x10, sizeof(result));
    return std::isfinite(result.translation.x) && std::isfinite(result.translation.y) && std::isfinite(result.translation.z);
}
inline void* MuzzleFlash() {
    if (!muzzle || !gun) return nullptr;
    alignas(8) uint8_t p[0x80]{};
    Put(p, 0, muzzle); Put(p, 8, gun); Name(p + 0x10, L"Muzzle");
    const engine::Vec scale{1, 1, 1}; Put(p, 0x50, scale);
    p[0x68] = 2; // SnapToTarget, from the fixed enum metadata.
    p[0x69] = 1; p[0x6a] = 1; p[0x6b] = 1; p[0x6c] = 1;
    // AutoRelease pool, original base particle color, no warmup.
    auto receiver = engine::DefaultObject(engine::Class(0x43a9670));
    if (!engine::Event(receiver, L"/Script/Engine.GameplayStatics.SpawnEmitterAttached", p)) return nullptr;
    void* result = nullptr; std::memcpy(&result, p + 0x78, 8); return result;
}

struct Quat { double x, y, z, w; };
inline Quat Multiply(const Quat& a, const Quat& b) {
    return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y, a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
            a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w, a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};
}
inline Quat Rotation(const engine::Transform& t) { return {t.rotation[0],t.rotation[1],t.rotation[2],t.rotation[3]}; }
inline engine::Vec Rotate(const Quat& q, const engine::Vec& v) {
    const Quat p{v.x,v.y,v.z,0}, inv{-q.x,-q.y,-q.z,q.w};
    const auto r = Multiply(Multiply(q,p),inv); return {r.x,r.y,r.z};
}
inline engine::Vec Rotator(const Quat& q) {
    constexpr double degrees = 180.0 / 3.14159265358979323846;
    const double sinPitch = (std::max)(-1.0, (std::min)(1.0, 2*(q.z*q.x-q.w*q.y)));
    return {std::asin(sinPitch)*degrees,
            std::atan2(2*(q.w*q.z+q.x*q.y),1-2*(q.y*q.y+q.z*q.z))*degrees,
            -std::atan2(2*(q.w*q.x+q.y*q.z),1-2*(q.x*q.x+q.y*q.y))*degrees};
}
inline engine::Vec Up(const engine::Vec& r) {
    const auto f = engine::Forward(r), right = engine::Right(r);
    return {f.y*right.z-f.z*right.y,f.z*right.x-f.x*right.z,f.x*right.y-f.y*right.x};
}
inline engine::Vec RotateByView(const engine::Vec& v, const engine::Vec& r) {
    return engine::Add(engine::Add(engine::Mul(engine::Forward(r),v.x),engine::Mul(engine::Right(r),v.y)),engine::Mul(Up(r),v.z));
}

// Exact original unweighted rich-curve keys, including nonzero tangents.
struct Key { double time, value, arrive, leave; uint8_t interpolation; };
inline constexpr Key KillLogo[]{{0,0,0,0,2},{1,8,0,0,2},{2,0,0,0,0}};
inline constexpr Key PanLocation[]{{0,0.4000000059604645,0,0,2},{0.75,-0.6000000238418579,0,0,2}};
inline constexpr Key KillPan[]{{0,0,0,0,2},{0.10000000149011612,1,2,2,2},{0.5,1,-2,-2,2},{0.6000000238418579,0,0,0,2}};
inline constexpr Key LeadLogo[]{{0,0,0,0,2},{1,8,0,0,2}};
inline constexpr Key LeadPan[]{{0,0,0,0,2},{0.10000000149011612,1,0,2,2}};
inline constexpr Key LeadOutline[]{{0,0,0,0,2},{0.4000000059604645,1,0,1.6666666269302368,2}};
template<size_t N> inline float Evaluate(const Key (&keys)[N], double time) {
    if (time <= keys[0].time) return static_cast<float>(keys[0].value);
    for (size_t i=0;i+1<N;++i) if (time < keys[i+1].time) {
        const auto& a=keys[i]; const auto& b=keys[i+1];
        const double dt=b.time-a.time, t=(time-a.time)/dt;
        if (a.interpolation==1) return static_cast<float>(a.value);
        if (a.interpolation==0) return static_cast<float>(a.value+(b.value-a.value)*t);
        const double t2=t*t,t3=t2*t;
        return static_cast<float>((2*t3-3*t2+1)*a.value+(t3-2*t2+t)*dt*a.leave+(-2*t3+3*t2)*b.value+(t3-t2)*dt*b.arrive);
    }
    return static_cast<float>(keys[N-1].value);
}
inline void SetChampion(bool isChampion) { championExplicit=true; champion=isChampion; }

inline bool Initialize(void* inWorld, void* pawn, void* weaponActor, void* weaponComponent, void* material) {
    if (ready) return true;
    error=0; world=inWorld; gunActor=weaponActor; gun=weaponComponent;
    if (!world || !pawn || !gunActor || !gun || !material) return Fail(201,"presentation_context_missing");
    stage="loading_jett_and_core";
    auto meshType=engine::Class(0x3815ac0), animationType=engine::Class(0x39e55b0);
    auto coreMesh=engine::Load(meshType,L"/Game/Characters/_Core/1P/Models/FP_Core_Retarget_Skelmesh.FP_Core_Retarget_Skelmesh");
    auto handsMesh=engine::Load(meshType,L"/Game/Characters/Wushu/S0/1P/Models/FP_Wushu_S0_Skelmesh.FP_Wushu_S0_Skelmesh");
    idle=engine::Load(animationType,L"/Game/Equippables/Guns/Rifles/AK/S0/1P/Anims/FP_Core_AK_S0_IdlePose.FP_Core_AK_S0_IdlePose");
    fire=engine::Load(animationType,L"/Game/Equippables/Guns/Rifles/AK/S0/1P/Anims/FP_Core_AK_S0_Fire.FP_Core_AK_S0_Fire");
    reload=engine::Load(animationType,L"/Game/Equippables/Guns/Rifles/AK/S0/1P/Anims/FP_Core_AK_S0_Reload.FP_Core_AK_S0_Reload");
    gunFire=engine::Load(animationType,L"/Game/Equippables/Guns/Rifles/AK/S0/1P/Anims/GN_Core_AK_S0_Fire.GN_Core_AK_S0_Fire");
    gunReload=engine::Load(animationType,L"/Game/Equippables/Guns/Rifles/AK/S0/1P/Anims/GN_Core_AK_S0_Reload.GN_Core_AK_S0_Reload");
    if (!coreMesh || !handsMesh || !idle || !fire || !reload || !gunFire || !gunReload) return Fail(202,"jett_animation_asset_missing");
    fireLength=Length(fire); reloadLength=Length(reload);
    if (!(fireLength>0) || !(reloadLength>0)) return Fail(203,"original_animation_length_unavailable");
    stage="creating_jett_pose_components";
    armsActor=engine::Spawn(world,{0,0,0},pawn);
    if (!armsActor) return Fail(204,"jett_actor_creation_failed");
    core=engine::AddSkeletal(armsActor); hands=engine::AddSkeletal(armsActor);
    if (!core || !hands || !engine::SetSkeletal(core,coreMesh) || !engine::SetSkeletal(hands,handsMesh)) return Fail(205,"jett_component_setup_failed");
    if (!engine::DisableCollision(core) || !engine::DisableCollision(hands) || !Leader(hands,core)) return Fail(206,"jett_pose_follow_failed");
    // Verified SkinnedMeshComponent property +0x92c, enum 0 is AlwaysTickPoseAndRefreshBones.
    *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(core)+0x92c)=0;
    *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(hands)+0x92c)=0;
    if (!SingleNode(core) || !Play(core,idle,true) || !SingleNode(gun) || !Visibility(core,false) || !Visibility(hands,true)) return Fail(207,"jett_animation_setup_failed");
    stage="creating_champions_materials";
    gunMID=DynamicMaterial(gun,material);
    if (!gunMID) return Fail(208,"champions_dynamic_material_failed");
    auto staticType=engine::Class(0x3816800), materialType=engine::Class(0x3815a10);
    auto bodyMesh=engine::Load(staticType,L"/Game/Equippables/Guns/Rifles/AK/Champions/Models/ak_reverse.ak_reverse");
    auto handleMesh=engine::Load(staticType,L"/Game/Equippables/Guns/Rifles/AK/Champions/Models/ak_charging_handle.ak_charging_handle");
    auto fresnel=engine::Load(materialType,L"/Game/Equippables/Guns/Rifles/AK/Champions/Models/Champions_Fresnel_Inst.Champions_Fresnel_Inst");
    if (!bodyMesh || !handleMesh || !fresnel) return Fail(209,"champions_outline_asset_missing");
    auto body=engine::AddStatic(gunActor), handle=engine::AddStatic(gunActor);
    if (!body || !handle || !engine::SetStatic(body,bodyMesh) || !engine::SetStatic(handle,handleMesh)) return Fail(210,"champions_outline_creation_failed");
    if (!engine::DisableCollision(body) || !engine::DisableCollision(handle) || !engine::AttachSocket(body,gun,L"Root") || !engine::AttachSocket(handle,gun,L"Charging_Handle")) return Fail(211,"champions_outline_attachment_failed");
    outlineBodyMID=DynamicMaterial(body,fresnel); outlineHandleMID=DynamicMaterial(handle,fresnel);
    if (!outlineBodyMID || !outlineHandleMID) return Fail(212,"champions_outline_material_failed");
    Scalar(gunMID,L"Logo Emissive Intensity",0); Scalar(gunMID,L"EmissivePanIntensity",0);
    Scalar(gunMID,L"EmissivePanLocation",-.6f); Scalar(outlineBodyMID,L"Opacity",0); Scalar(outlineHandleMID,L"Opacity",0);
    stage="loading_original_muzzle_particle";
    muzzle=engine::Load(engine::Class(0x3e58380),L"/Game/Equippables/Guns/Rifles/AK/Champions/VFX/Particles/VFX_Champions_AK_Fire.VFX_Champions_AK_Fire");
    if (!muzzle) return Fail(213,"original_muzzle_particle_missing");
    ready=true; stage="presentation_ready"; return true;
}

inline void Update(double now, const engine::Vec& cameraPosition, const engine::Vec& cameraRotation,
                   uint32_t shots, uint32_t kills, bool reloading) {
    if (!ready || error || !std::isfinite(now)) return;
    engine::Transform cameraBone;
    engine::Vec anchor{0,0,167.38023376464844}; // Verified reference-pose Camera bone; replaced by actual animated socket below.
    if (Socket(core,L"Camera",2,cameraBone)) anchor=cameraBone.translation;
    const auto rotation=engine::Add(cameraRotation,options.sourceRotationOffset);
    const auto offset=RotateByView(engine::Add(anchor,options.cameraLocalOffset),rotation);
    const auto location=engine::Add(cameraPosition,engine::Mul(offset,-1));
    if (!engine::SetTransform(armsActor,location,rotation)) ++transformFailures;
    if (options.alignWeaponToRig) {
        engine::Transform target,gunRoot;
        if (Socket(core,options.weaponSocket,0,target) && Socket(gun,L"Root",2,gunRoot)) {
            const auto root=Rotation(gunRoot); const Quat inverse{-root.x,-root.y,-root.z,root.w};
            const auto q=Multiply(Rotation(target),inverse);
            const auto weaponLocation=engine::Add(target.translation,engine::Mul(Rotate(q,gunRoot.translation),-1));
            if (!engine::SetTransform(gunActor,weaponLocation,Rotator(q))) ++transformFailures;
        }
    }
    if (reloading && !wasReloading) {
        if (Play(core,reload,false) && Play(gun,gunReload,false)) { ++reloadAnimations; animationUntil=now+reloadLength; animationActive=true; }
    } else if (shots>lastShots && !reloading) {
        if (Play(core,fire,false) && Play(gun,gunFire,false)) { ++fireAnimations; animationUntil=now+fireLength; animationActive=true; }
        if (options.muzzleEffects) { if (MuzzleFlash()) ++emittersSpawned; else ++emitterFailures; }
    } else if (animationActive && now>=animationUntil && !reloading) {
        Play(core,idle,true); animationActive=false;
    }
    if (kills>lastKills) killSince=now;
    const bool leading=championExplicit ? champion : (options.autoChampionForSoloDrill && kills>0);
    if (leading && leaderSince<0) leaderSince=now;
    if (!leading) leaderSince=-1;
    if (lastMaterialUpdate<0 || now-lastMaterialUpdate>=1.0/30.0) {
        const double leadTime=leaderSince<0 ? 0 : now-leaderSince;
        const double pulseTime=now-killSince;
        const bool pulse=pulseTime>=0 && pulseTime<=2;
        const float leadLogo=leading ? Evaluate(LeadLogo,leadTime) : 0;
        const float logo=(std::max)(leadLogo,pulse ? Evaluate(KillLogo,pulseTime) : 0.0f);
        const float pan=pulse ? Evaluate(KillPan,pulseTime) : (leading ? Evaluate(LeadPan,leadTime) : 0);
        const float sweep=pulse ? Evaluate(PanLocation,pulseTime) : (leading ? Evaluate(PanLocation,leadTime) : -.6f);
        const float outline=leading ? Evaluate(LeadOutline,leadTime) : 0;
        Scalar(gunMID,L"Logo Emissive Intensity",logo); Scalar(gunMID,L"EmissivePanLocation",sweep); Scalar(gunMID,L"EmissivePanIntensity",pan);
        Scalar(outlineBodyMID,L"Opacity",outline); Scalar(outlineHandleMID,L"Opacity",outline);
        lastMaterialUpdate=now;
    }
    lastShots=shots; lastKills=kills; wasReloading=reloading;
}
}
