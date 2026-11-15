#pragma once
#include "presentation.h"

// Exact original body geometry and native signatures:
// reports/duel1-original-hitboxes.json. All calls are on the game thread.
namespace enemy_hitboxes {
inline constexpr const wchar_t* PhysicsAssetPath =
    L"/Game/Characters/_Core/3P/Models/Core_HitboxPhysicsAsset2.Core_HitboxPhysicsAsset2";
inline constexpr const wchar_t* BoneNames[]{
    L"Head", L"Neck", L"L_Elbow", L"L_Foot", L"L_Hand", L"L_Hip", L"L_Knee", L"L_Shoulder",
    L"R_Elbow", L"R_Foot", L"R_Hand", L"R_Hip", L"R_Knee", L"R_Shoulder", L"Spine1", L"Spine2",
    L"Spine3", L"Spine4", L"Splitter"
};
inline uint8_t boneIds[19][12]{};
inline void* component = nullptr;
inline void* visualComponent = nullptr;
inline void* proxyOwner = nullptr;
inline bool ready = false;
inline unsigned error = 0;
inline const char* stage = "hitboxes_not_initialized";
inline uint32_t queries = 0, hits = 0, unknownBones = 0;
inline int bodyCount = 0, physicsActorCount = 0;
inline uint32_t poseSyncs = 0, synchronizedBodies = 0, diagnosticQueries = 0, diagnosticHits = 0;
inline engine::Vec synchronizedHead{};

template<class T> inline T Read(const void* object, size_t offset) {
    T value{};
    if (object) std::memcpy(&value,reinterpret_cast<const uint8_t*>(object)+offset,sizeof(value));
    return value;
}
inline int BoneIndex(const void* name) {
    for (int i=0;i<19;++i) if (std::memcmp(boneIds[i],name,8)==0) return i;
    return -1;
}
inline void InspectBodies() {
    // Proven by native SkeletalMeshComponent::LineTraceComponent (RVA 0x4a88b60).
    bodyCount=Read<int>(component,0xd18); physicsActorCount=0;
    auto bodies=Read<void**>(component,0xd10);
    if (!bodies || bodyCount<0 || bodyCount>64) return;
    for (int i=0;i<bodyCount;++i) if (bodies[i] && Read<void*>(bodies[i],0x158)) ++physicsActorCount;
}

struct Result {
    bool hit = false;
    bool blocking = false;
    // Native EAresRegionalDamage values: body 0, head 1, leg 2.
    uint8_t region = 0;
    int boneIndex = -1;
    const wchar_t* boneName = L"";
    double distance = 0;
    engine::Vec location{}, normal{};
    alignas(8) uint8_t nativeHit[0xf0]{};
};

inline bool Bind(void* skeletalComponent) {
    ready=false; error=0; visualComponent=skeletalComponent;
    if (!visualComponent) { error=401; stage="hitbox_component_missing"; return false; }
    // A valid LeaderPose intentionally prevents SMC physics-state creation.
    // Keep the renderer's leader pose and create a non-leader native physics
    // component on the same actor, so environment traces ignoring that actor
    // also ignore this proxy. No extra actor and no fabricated hit shapes.
    auto owner=Read<void*>(visualComponent,0xc8);
    if (!owner || (proxyOwner && owner!=proxyOwner)) { error=407; stage="hitbox_proxy_owner_changed"; return false; }
    if (!component) {
        proxyOwner=owner; component=engine::AddSkeletal(owner);
        if (!component) { error=408; stage="hitbox_proxy_creation_failed"; return false; }
        auto core=engine::Load(engine::Class(0x3815ac0),
            L"/Game/Characters/_Core/3P/Models/TP_Core_NewBaseMesh_Skelmesh.TP_Core_NewBaseMesh_Skelmesh");
        if (!core || !engine::SetSkeletal(component,core) || !presentation::Visibility(component,false)) {
            error=409; stage="hitbox_proxy_mesh_failed"; return false;
        }
        *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(component)+0x92c)=3;
    }
    uint8_t queryOnly=1; // Fixed enum ECollisionEnabled::QueryOnly.
    if (!engine::Event(component,L"/Script/Engine.PrimitiveComponent.SetCollisionEnabled",&queryOnly)) {
        error=404; stage="hitbox_query_collision_failed"; return false;
    }
    // Resolve per binding; the component's reflected PhysicsAssetOverride retains it.
    auto asset=engine::Load(engine::Class(0x48eb4a0),PhysicsAssetPath);
    if (!asset) { error=402; stage="original_hitbox_asset_missing"; return false; }
    alignas(8) uint8_t p[0x10]{};
    presentation::Put(p,0,asset); p[8]=1;
    if (!engine::Event(component,L"/Script/Engine.SkinnedMeshComponent.SetPhysicsAsset",p)) {
        error=403; stage="hitbox_physics_asset_bind_failed"; return false;
    }
    for (unsigned i=0;i<19;++i) presentation::Name(boneIds[i],BoneNames[i]);
    InspectBodies();
    if (bodyCount!=19 || physicsActorCount!=19) { error=410; stage="hitbox_physics_bodies_missing"; return false; }
    ready=true; stage="hitbox_query_ready"; return true;
}

inline bool SyncPhysicsPose() {
    InspectBodies();
    if (bodyCount!=19 || physicsActorCount!=19) { error=410; stage="hitbox_physics_bodies_missing"; return false; }
    auto bodies=Read<void**>(component,0xd10);
    synchronizedBodies=0;
    for (int i=0;i<bodyCount;++i) {
        auto body=bodies[i];
        // Original InitArticulated uses WeakBodySetup @+8 -> BoneName @+0x30.
        auto setup=engine::function<void*(*)(const void*)>(0x1d4cfa0)(reinterpret_cast<const uint8_t*>(body)+8);
        if (!setup) { error=411; stage="hitbox_body_setup_missing"; return false; }
        const int bone=BoneIndex(reinterpret_cast<const uint8_t*>(setup)+0x30);
        if (bone<0) { error=412; stage="hitbox_body_bone_unknown"; return false; }
        engine::Transform transform;
        if (!presentation::Socket(visualComponent,BoneNames[bone],0,transform)) {
            error=413; stage="hitbox_rendered_bone_missing"; return false;
        }
        // Original FBodyInstance::SetBodyTransform. Native callers use enum 1
        // for teleport and enum 2 for reset; this updates the real Chaos actor.
        engine::function<void(*)(void*,const engine::Transform*,uint8_t)>(0x4864b00)(body,&transform,1);
        if (bone==0) synchronizedHead=transform.translation;
        ++synchronizedBodies;
    }
    ++poseSyncs; return true;
}

inline bool Query(const engine::Vec& start,const engine::Vec& end,uint8_t* parameters) {
    presentation::Put(parameters,0,start); presentation::Put(parameters,0x18,end);
    return engine::Event(component,L"/Script/Engine.PrimitiveComponent.K2_LineTraceComponent",parameters);
}

inline uint8_t Region(int index) {
    // Exact adjacent arrays serialized by BasePawn and used by native regional damage.
    if (index==0 || index==1) return 1; // Head, Neck.
    if (index==3 || index==5 || index==6 || index==9 || index==11 || index==12 || index==18) return 2;
    return 0;
}

// Returns -1 for setup/call failure, 0 for miss, 1 for an actual original-shape hit.
// Trace only this skeletal component. The caller must compare against world
// obstruction distance so walls remain authoritative occluders.
inline int Trace(const engine::Vec& start, const engine::Vec& end, Result& result) {
    result=Result{};
    if (!ready || !component) return -1;
    if (!SyncPhysicsPose()) return -1;
    alignas(8) uint8_t p[0x170]{};
    // bTraceComplex, both debug flags remain false: query original simple bodies.
    ++queries;
    if (!Query(start,end,p)) {
        error=405; stage="hitbox_native_trace_failed"; return -1;
    }
    if (!p[0x168]) {
        // Diagnostic only, never substituted for the shot result or damage.
        // A short line through the live Head tests whether original query
        // shapes exist at the rendered pose, independently of aim accuracy.
        if (diagnosticQueries<8) {
            alignas(8) uint8_t diagnostic[0x170]{};
            ++diagnosticQueries;
            if (Query(engine::Add(synchronizedHead,{-50,0,0}),engine::Add(synchronizedHead,{50,0,0}),diagnostic)
                && diagnostic[0x168]) ++diagnosticHits;
        }
        return 0;
    }
    float distance=0;
    std::memcpy(&distance,p+0x80,4); // OutHit @0x78, native HitResult.Distance @0x08.
    if (!std::isfinite(distance) || distance<0) { error=406; stage="hitbox_invalid_native_distance"; return -1; }
    result.hit=true; result.distance=distance;
    result.blocking=(p[0x78+0xad]&1)!=0;
    std::memcpy(&result.location,p+0x38,sizeof(result.location));
    std::memcpy(&result.normal,p+0x50,sizeof(result.normal));
    std::memcpy(result.nativeHit,p+0x78,sizeof(result.nativeHit));
    result.boneIndex=BoneIndex(p+0x68);
    if (result.boneIndex>=0) result.boneName=BoneNames[result.boneIndex];
    if (result.boneIndex<0) ++unknownBones;
    result.region=Region(result.boneIndex); ++hits; return 1;
}
}
