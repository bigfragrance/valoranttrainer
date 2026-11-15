#pragma once
#include "presentation.h"

// Resource/type evidence: reports/duel1-enemy-assets.json.
// Fixed image call layouts: reports/duel1-presentation-call-layouts.json.
// Called only by the existing owned game-thread tick. One target, one reusable corpse.
namespace enemy_presentation {
struct Options {
    double yawOffset = 0;
    // Original BasePlayer/Wushu CDO: capsule half-height 98 + mesh relative Z -100.
    double meshFootOffset = -2;
    double movementThreshold = 3; // cm/s; the model remains at its original scale.
    bool outline = true;
    bool weapon = true;
    float outlineColor[4]{0.75f, 1.0f, 0.02f, 1.0f};
    const wchar_t* weaponSocket = L"R_WeaponPoint";
};
inline Options options;
inline bool ready = false, outlineReady = false, weaponReady = false;
inline unsigned error = 0;
inline const char* stage = "not_initialized";
inline const char* heroName = "unassigned";
inline const char* animationState = "none";
inline uint32_t heroIndex = 0, deathCount = 0, hitCount = 0, animationChanges = 0, transformFailures = 0;
inline void* world = nullptr;
inline void* actor = nullptr;
inline void* body = nullptr;
inline void* sourceActor = nullptr;
inline void* source = nullptr;
inline void* lowerSource = nullptr;
inline void* combinedPose = nullptr;
inline void* retentionActor = nullptr;
inline void* retainedMeshes[3]{};
inline void* retainedAnimations[5]{};
inline void* outlineComponent = nullptr;
inline uint8_t observedOutlineMode = 0;
inline uint8_t requestedOutlineMode = 0;
inline void* corpseActor = nullptr;
inline void* corpseSource = nullptr;
inline void* corpseBody = nullptr;
inline void* gunActor = nullptr;
inline void* gun = nullptr;
inline void* heroMeshes[3]{};
inline void* idle = nullptr;
inline void* walkEast = nullptr;
inline void* walkWest = nullptr;
inline void* hit = nullptr;
inline void* death = nullptr;
inline void* activeAnimation = nullptr;
inline void* activeLowerAnimation = nullptr;
inline uint32_t poseCompositions = 0;
inline float hitLength = 0, deathLength = 0;
inline double hitUntil = -1, corpseUntil = -1, lastTime = -1;
inline double lastMotionTime = -1;
inline double lastPoseTime = -1;
inline int lastMotionDirection = 0;
inline engine::Vec lastPosition{};
inline double lastYaw = 0;
inline float lastHealth = 0;
inline uint32_t lastId = 0, lastKills = 0;
inline bool haveState = false, lastVisible = false, corpseVisible = false, currentVisible = false;

inline constexpr const wchar_t* HeroPaths[] {
    L"/Game/Characters/Wushu/S0/3P/Models/TP_Wushu_S0_Skelmesh.TP_Wushu_S0_Skelmesh",
    L"/Game/Characters/Phoenix/S0/3P/Models/TP_Phoenix_S0_Skelmesh.TP_Phoenix_S0_Skelmesh",
    L"/Game/Characters/Thorne/S0/3P/Models/TP_Thorne_S0_Skelmesh.TP_Thorne_S0_Skelmesh"
};
inline constexpr const char* HeroNames[]{"Jett", "Phoenix", "Sage"};
// Actual Core mesh hierarchy order, including unkeyed hip twists. Root/Splitter
// carry locomotion into the copied upper body's existing local transforms.
inline constexpr const wchar_t* LowerBones[]{
    L"Skeleton", L"Root", L"Splitter", L"Pelvis", L"R_Foot_IKpv_IUE", L"L_Foot_IKpv_IUE",
    L"L_Hip", L"L_Knee", L"L_Knee_Ndl", L"L_Foot", L"L_Toe", L"L_Hip_Twst1", L"L_Hip_Twst3",
    L"R_Hip", L"R_Knee", L"R_Knee_Ndl", L"R_Foot", L"R_Toe", L"R_Hip_Twst1", L"R_Hip_Twst3",
    L"IK_RootTarget", L"L_IK_FootTarget", L"R_IK_FootTarget"
};
inline bool Fail(unsigned code, const char* message) { error=code; stage=message; return false; }
inline engine::Vec MeshPosition(const engine::Vec& feet) { return {feet.x,feet.y,feet.z+options.meshFootOffset}; }
inline bool MaterialAt(void* component, int slot, void* material) {
    alignas(8) uint8_t p[0x10]{};
    presentation::Put(p,0,slot); presentation::Put(p,8,material);
    return engine::Event(component,L"/Script/Engine.PrimitiveComponent.SetMaterial",p);
}
inline int MaterialCount(void* component) {
    int count=0;
    if (!engine::Event(component,L"/Script/Engine.PrimitiveComponent.GetNumMaterials",&count)) return -1;
    return count>=0 && count<=64 ? count : -1;
}
inline bool OutlineColors() {
    alignas(8) uint8_t p[0x28]{};
    presentation::Put(p,0,world);
    constexpr float allyColor[4]{0.1f,0.8f,1.0f,1.0f};
    std::memcpy(p+8,allyColor,sizeof(allyColor));
    std::memcpy(p+0x18,options.outlineColor,sizeof(options.outlineColor));
    return engine::Event(engine::DefaultObject(engine::Class(0x2a64b40)),
        L"/Script/Renderer.AresOutlineRendering.SetOutlineColorsForRender",p);
}
inline bool SetNativeOutline(bool enabled) {
    uint8_t mode=enabled ? 3 : 0; // Exact EAresOutlineMode: Enemy=3, None=0.
    if (!engine::Event(outlineComponent,L"/Script/ShooterGame.AresOutlineComponent.SetOutlineMode",&mode)) return false;
    requestedOutlineMode=mode;
    if (!engine::Event(body,L"/Script/Engine.PrimitiveComponent.GetAresOutlineMode",&observedOutlineMode)) return false;
    return observedOutlineMode==mode;
}
inline bool CreateNativeOutline() {
    alignas(8) uint8_t p[0x18]{};
    presentation::Put(p,0,body); p[8]=options.outline ? 3 : 0;
    if (!engine::Event(engine::DefaultObject(engine::Class(0x64d0c50)),
        L"/Script/ShooterGame.AresOutlineComponent.PushMeshOutline",p)) return false;
    std::memcpy(&outlineComponent,p+0x10,8);
    return outlineComponent && OutlineColors() && SetNativeOutline(options.outline);
}
inline bool OriginalScale(void* component) {
    engine::Vec scale{1,1,1};
    return engine::Event(component,L"/Script/Engine.SceneComponent.SetWorldScale3D",&scale);
}
inline void AlwaysAnimate(void* component) {
    // Fixed native metadata: VisibilityBasedAnimTickOption +0x92c; enum 0.
    *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(component)+0x92c)=0;
}
inline bool SetPoseMesh(void* component, void* mesh) {
    alignas(8) uint8_t p[0x10]{};
    presentation::Put(p,0,mesh); p[8]=1;
    return engine::Event(component,L"/Script/Engine.SkinnedMeshComponent.SetSkinnedAssetAndUpdate",p);
}
inline bool CopyUpperPose() {
    void* p=source;
    return engine::Event(combinedPose,L"/Script/Engine.PoseableMeshComponent.CopyPoseFromSkeletalComponent",&p);
}
inline bool SetPoseBone(const wchar_t* name, const engine::Transform& transform) {
    alignas(16) uint8_t p[0x80]{};
    presentation::Name(p,name); presentation::Put(p,0x10,transform);
    p[0x70]=1; // EBoneSpaces::ComponentSpace; distinct from ERelativeTransformSpace (2).
    return engine::Event(combinedPose,L"/Script/Engine.PoseableMeshComponent.SetBoneTransformByName",p);
}
inline bool ComposePose(double now) {
    if (lastPoseTime>=0 && now-lastPoseTime<1.0/60.0) return true;
    if (!CopyUpperPose()) return false;
    for (const auto name : LowerBones) {
        engine::Transform t;
        if (!presentation::Socket(lowerSource,name,2,t) || !SetPoseBone(name,t)) return false;
    }
    lastPoseTime=now; ++poseCompositions; return true;
}
inline uint32_t HeroFor(uint32_t id) {
    // Stable per logical target; no allocation or RNG state tied to rendering frames.
    uint32_t h=id+0x9e3779b9u; h=(h^(h>>16))*0x85ebca6bu;
    h=(h^(h>>13))*0xc2b2ae35u; return (h^(h>>16))%3;
}
inline bool SetHero(void* component, uint32_t index) {
    if (index>=3 || !engine::SetSkeletal(component,heroMeshes[index])) return false;
    const int count=MaterialCount(component);
    if (count<=0) return false;
    // A mesh swap must not carry another hero's material overrides with it.
    for (int slot=0;slot<count;++slot) if (!MaterialAt(component,slot,nullptr)) return false;
    return OriginalScale(component);
}
inline bool SetLiveHero(uint32_t index) {
    if (!SetHero(body,index)) return false;
    if (!presentation::Leader(body,combinedPose)) return false;
    heroIndex=index; heroName=HeroNames[index]; return true;
}
inline bool CreatePoseSource(void* coreMesh) {
    sourceActor=engine::Spawn(world,{0,0,0},actor);
    if (!sourceActor) return false;
    source=engine::AddSkeletal(sourceActor);
    if (!source || !engine::SetSkeletal(source,coreMesh) || !engine::DisableCollision(source)) return false;
    lowerSource=engine::AddSkeletal(sourceActor);
    combinedPose=engine::AddComponent(sourceActor,0x41e6140);
    if (!lowerSource || !combinedPose || !engine::SetSkeletal(lowerSource,coreMesh)
        || !SetPoseMesh(combinedPose,coreMesh) || !engine::DisableCollision(lowerSource)
        || !engine::DisableCollision(combinedPose)) return false;
    AlwaysAnimate(source);
    AlwaysAnimate(lowerSource); AlwaysAnimate(combinedPose);
    return OriginalScale(source) && presentation::SingleNode(source) && presentation::Play(source,idle,true)
        && presentation::Visibility(source,false) && OriginalScale(lowerSource) && OriginalScale(combinedPose)
        && presentation::SingleNode(lowerSource) && presentation::Play(lowerSource,idle,true)
        && presentation::Visibility(lowerSource,false) && presentation::Visibility(combinedPose,false);
}
inline bool RetainAssets(void* coreMesh) {
    // C++ statics are not UObject GC references. World owns this actor, the actor
    // owns registered components, and their reflected mesh/animation fields own
    // all candidates even when another hero/direction is currently selected.
    retentionActor=engine::Spawn(world,{0,0,0},actor);
    if (!retentionActor || !engine::Hidden(retentionActor,true)) return false;
    for (unsigned i=0;i<3;++i) {
        auto component=engine::AddSkeletal(retentionActor); retainedMeshes[i]=component;
        if (!component || !engine::SetSkeletal(component,heroMeshes[i]) || !engine::DisableCollision(component)
            || !presentation::Visibility(component,false)) return false;
        *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(component)+0x92c)=3; // OnlyTickPoseWhenRendered.
    }
    void* animations[]{idle,walkEast,walkWest,hit,death};
    for (unsigned i=0;i<5;++i) {
        auto component=engine::AddSkeletal(retentionActor); retainedAnimations[i]=component;
        if (!component || !engine::SetSkeletal(component,coreMesh) || !engine::DisableCollision(component)
            || !presentation::Visibility(component,false) || !presentation::SingleNode(component)
            || !presentation::Play(component,animations[i],false)) return false;
        *reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(component)+0x92c)=3;
    }
    return true;
}
inline bool CreateCorpse(void* coreMesh) {
    corpseActor=engine::Spawn(world,{0,0,0},actor);
    if (!corpseActor) return false;
    // Source first: root -> visible follower has an acyclic tick prerequisite.
    corpseSource=engine::AddSkeletal(corpseActor); corpseBody=engine::AddSkeletal(corpseActor);
    if (!corpseSource || !corpseBody || !engine::SetSkeletal(corpseSource,coreMesh)
        || !SetHero(corpseBody,0)) return false;
    AlwaysAnimate(corpseSource); AlwaysAnimate(corpseBody);
    return engine::DisableCollision(corpseSource) && engine::DisableCollision(corpseBody)
        && presentation::SingleNode(corpseSource) && presentation::Leader(corpseBody,corpseSource)
        && presentation::Visibility(corpseSource,false) && presentation::Visibility(corpseBody,false)
        && OriginalScale(corpseSource);
}
inline bool CreateWeapon(void* meshType, void* materialType) {
    auto mesh=engine::Load(meshType,L"/Game/Equippables/Guns/Rifles/AK/S0/1P/Models/GN_AK_S0_Skelmesh.GN_AK_S0_Skelmesh");
    auto material=engine::Load(materialType,L"/Game/Equippables/Guns/Rifles/AK/S0/1P/Materials/AK_S0_MI.AK_S0_MI");
    auto magazineMesh=engine::Load(engine::Class(0x3816800),L"/Game/Equippables/Guns/Rifles/AK/S0/1P/Models/GN_AK_Magazine_S0_Staticmesh.GN_AK_Magazine_S0_Staticmesh");
    if (!mesh || !material || !magazineMesh) return false;
    gunActor=engine::Spawn(world,{0,0,0},actor);
    if (!gunActor) return false;
    gun=engine::AddSkeletal(gunActor); auto magazine=engine::AddStatic(gunActor);
    if (!gun || !magazine || !engine::SetSkeletal(gun,mesh) || !engine::SetStatic(magazine,magazineMesh)) return false;
    return engine::SetMaterial(gun,material) && engine::DisableCollision(gun) && engine::DisableCollision(magazine)
        && engine::AttachSocket(magazine,gun,L"Magazine_Main") && OriginalScale(gun) && engine::Hidden(gunActor,true);
}
inline bool Initialize(void* inWorld, void* botActor, void* botMeshComponent) {
    if (ready) return true;
    // Failed initialization is terminal for this session: never grow duplicate actors on retries.
    if (error) return false;
    world=inWorld; actor=botActor; body=botMeshComponent;
    if (!world || !actor || !body) return Fail(301,"enemy_context_missing");
    stage="loading_original_hero_assets";
    auto meshType=engine::Class(0x3815ac0), animType=engine::Class(0x39e55b0), matType=engine::Class(0x3815a10);
    for (unsigned i=0;i<3;++i) { heroMeshes[i]=engine::Load(meshType,HeroPaths[i]); if (!heroMeshes[i]) return Fail(302,"enemy_hero_asset_missing"); }
    auto coreMesh=engine::Load(meshType,L"/Game/Characters/_Core/3P/Models/TP_Core_NewBaseMesh_Skelmesh.TP_Core_NewBaseMesh_Skelmesh");
    idle=engine::Load(animType,L"/Game/Characters/_Core/3P/Anims/Rifle/Core_Rifle_Idle.Core_Rifle_Idle");
    walkEast=engine::Load(animType,L"/Game/Characters/_Core/3P/Anims/TP_Core_WalkE_260_LB.TP_Core_WalkE_260_LB");
    walkWest=engine::Load(animType,L"/Game/Characters/_Core/3P/Anims/TP_Core_WalkW_260_LB.TP_Core_WalkW_260_LB");
    hit=engine::Load(animType,L"/Game/Characters/_Core/3P/Anims/TP_Core_HitReact_Body_Front_UB.TP_Core_HitReact_Body_Front_UB");
    death=engine::Load(animType,L"/Game/Characters/_Core/Death/3P/Anims/TP_Core_Death_Body_Front_SM.TP_Core_Death_Body_Front_SM");
    if (!coreMesh || !idle || !walkEast || !walkWest || !hit || !death) return Fail(303,"enemy_animation_asset_missing");
    hitLength=presentation::Length(hit); deathLength=presentation::Length(death);
    if (!(hitLength>0) || !(deathLength>0)) return Fail(304,"enemy_animation_length_missing");
    stage="retaining_enemy_original_assets";
    if (!RetainAssets(coreMesh)) return Fail(318,"enemy_asset_retention_failed");
    stage="creating_enemy_pose_and_outline";
    if (!CreatePoseSource(coreMesh)) return Fail(305,"enemy_pose_source_failed");
    AlwaysAnimate(body);
    if (!engine::DisableCollision(body) || !SetLiveHero(0)) return Fail(308,"enemy_hero_pose_follow_failed");
    // Native renderer outline: no duplicate mesh, emissive shader, or material replacement.
    if (!CreateNativeOutline()) return Fail(307,"enemy_native_outline_not_observed");
    outlineReady=true;
    stage="creating_reusable_death_actor";
    if (!CreateCorpse(coreMesh)) return Fail(309,"enemy_corpse_creation_failed");
    stage="creating_enemy_original_rifle";
    weaponReady=CreateWeapon(meshType,matType);
    if (!weaponReady) return Fail(310,"enemy_rifle_creation_failed");
    activeAnimation=idle; activeLowerAnimation=idle; ready=true; stage="enemy_presentation_ready"; return true;
}
inline bool ShowCorpse(double now) {
    if (!SetHero(corpseBody,heroIndex) || !presentation::Leader(corpseBody,corpseSource)) return false;
    if (!engine::SetTransform(corpseActor,MeshPosition(lastPosition),{0,lastYaw+options.yawOffset,0})) ++transformFailures;
    if (!presentation::Play(corpseSource,death,false) || !presentation::Visibility(corpseBody,true)) return false;
    corpseUntil=now+deathLength; corpseVisible=true; ++deathCount; return true;
}
inline void FollowWeapon() {
    if (!weaponReady || !options.weapon || !currentVisible) return;
    engine::Transform target,root;
    if (!presentation::Socket(combinedPose,options.weaponSocket,0,target) || !presentation::Socket(gun,L"Root",2,root)) { ++transformFailures; return; }
    const auto r=presentation::Rotation(root);
    const presentation::Quat inverse{-r.x,-r.y,-r.z,r.w};
    const auto q=presentation::Multiply(presentation::Rotation(target),inverse);
    const auto location=engine::Add(target.translation,engine::Mul(presentation::Rotate(q,root.translation),-1));
    if (!engine::SetTransform(gunActor,location,presentation::Rotator(q))) ++transformFailures;
}
inline void Update(double now, uint32_t botId, const engine::Vec& position, double yaw, bool visible, float health, uint32_t kills) {
    if (!ready || error || !std::isfinite(now) || !std::isfinite(yaw) || !std::isfinite(health)
        || !std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z)) return;
    const bool newTarget=!haveState || botId!=lastId;
    // The host may advance target id in the same packet as kills. Use the previous actor state.
    const bool died=haveState && lastVisible && lastHealth>0 && (kills>lastKills || (!newTarget && health<=0));
    if (died && !ShowCorpse(now)) { Fail(311,"enemy_death_animation_failed"); return; }
    if (corpseVisible && now>=corpseUntil) { presentation::Visibility(corpseBody,false); corpseVisible=false; }
    if (newTarget) {
        if (!SetLiveHero(HeroFor(botId))) { Fail(312,"enemy_hero_switch_failed"); return; }
        activeAnimation=nullptr; activeLowerAnimation=nullptr; hitUntil=-1;
        lastMotionTime=-1; lastMotionDirection=0; lastPoseTime=-1;
    }
    const bool alive=visible && health>0;
    if (currentVisible!=alive || newTarget) {
        engine::Hidden(actor,!alive); engine::Hidden(gunActor,!alive || !options.weapon);
        currentVisible=alive;
    }
    const uint8_t desiredOutline=options.outline ? 3 : 0;
    if (requestedOutlineMode!=desiredOutline && !SetNativeOutline(options.outline)) { Fail(317,"enemy_native_outline_change_failed"); return; }
    const engine::Vec rotation{0,yaw+options.yawOffset,0};
    const auto meshPosition=MeshPosition(position);
    if (!engine::SetTransform(actor,meshPosition,rotation)) ++transformFailures;
    if (!engine::SetTransform(sourceActor,meshPosition,rotation)) ++transformFailures;
    if (alive) {
        const bool injured=haveState && !newTarget && lastHealth>health && health>0;
        if (injured) {
            if (!presentation::Play(source,hit,false)) { Fail(313,"enemy_hit_animation_failed"); return; }
            activeAnimation=hit; animationState="hit"; hitUntil=now+hitLength; ++hitCount; ++animationChanges;
        }
        if (now>=hitUntil && activeAnimation!=idle) {
            if (!presentation::Play(source,idle,true)) { Fail(314,"enemy_upper_idle_failed"); return; }
            activeAnimation=idle; ++animationChanges;
        }
        void* desired=idle; const char* state="idle";
        const double dt=now-lastTime;
        if (!newTarget && dt>0 && dt<0.5) {
            const auto right=engine::Right(rotation);
            const double lateral=((position.x-lastPosition.x)*right.x+(position.y-lastPosition.y)*right.y)/dt;
            if (std::abs(lateral)>options.movementThreshold) { lastMotionTime=now; lastMotionDirection=lateral>0 ? 1 : -1; }
        }
        // Host snapshots can be slower than the renderer. Do not restart idle between packets.
        if (lastMotionTime>=0 && now-lastMotionTime<0.12) {
            desired=lastMotionDirection>0 ? walkEast : walkWest;
            state=lastMotionDirection>0 ? "strafe_east" : "strafe_west";
        }
        if (desired!=activeLowerAnimation) {
            if (!presentation::Play(lowerSource,desired,true)) { Fail(315,"enemy_lower_locomotion_failed"); return; }
            activeLowerAnimation=desired; ++animationChanges;
        }
        animationState=now<hitUntil ? "hit" : state;
        if (!ComposePose(now)) { Fail(316,"enemy_pose_composition_failed"); return; }
        FollowWeapon();
    } else animationState="hidden";
    lastPosition=position; lastYaw=yaw; lastTime=now; lastHealth=health;
    lastId=botId; lastKills=kills; lastVisible=alive; haveState=true;
}
}
