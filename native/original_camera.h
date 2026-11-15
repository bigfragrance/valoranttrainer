#pragma once
#include "original_players.h"
#include <cstddef>

// Pinned-image evidence: reports/duel1-original-camera.json.
// The shipped Character tick skips live FollowRecoil cameras on Standalone
// Authority pawns. Fill that local scheduling gap with its complete original
// camera updater. Position, recoil, zoom and camera FOV stay in native code.
// Invoke AfterOriginalCharacterTick only immediately AFTER that pawn's original
// TickActor, with the same delta; never call it from an additional frame loop.
namespace original_camera {
inline constexpr uintptr_t CameraClass=0x6c5b630,CameraComponentClass=0x37bc400;
inline constexpr uintptr_t SceneComponentClass=0x36c7580;
inline constexpr uintptr_t ViewTargetClass=0x6dd74a0;
struct ComponentSnapshot {
    void* object=nullptr;
    void* parent=nullptr;
    engine::Vec position{},rotation{},relativePosition{};
    bool present=false,active=false,queried=false;
};
// Original /Script/ShooterGame.ViewTargetInfo: TargetActor0, Mode8,
// generated struct size16/alignment8. OnRep receives the OLD record by value.
struct ViewTargetInfo {
    void* targetActor=nullptr;
    uint8_t mode=0;
    uint8_t reserved[7]{};
};
static_assert(sizeof(ViewTargetInfo)==16&&alignof(ViewTargetInfo)==8);
static_assert(offsetof(ViewTargetInfo,mode)==8);
struct Diagnostics {
    bool bound=false,restartRequired=false,originalCamera=false,realLocalPlayer=false;
    bool locallyControlled=false,standalone=false,noDemoDriver=false,noNetDriver=false;
    bool originalWorldPredicate=false,predicateQueried=false,canBeViewTarget=false;
    bool viewTargetMatches=false,ownerMatches=false,originalSchedulerSkips=false;
    bool callbackObserved=false,originalEntryVerified=false;
    bool viewTargetComponentPresent=false,viewTargetComponentOwnerMatches=false;
    bool lifecycleEntriesVerified=false,lifecycleRequested=false,lifecycleRestored=false;
    bool lifecycleCandidateValid=false,lifecycleUsedApplyInfo=false;
    bool lifecycleOnRepAbiValid=false,lifecycleUsedOnRep=false,lifecycleInfoWritten=false;
    bool lifecycleInfoPreserved=false,lifecycleLinksCurrent=false;
    bool fovUnlockRequested=false,fovUnlockCompleted=false,fovUnlockEntryVerified=false;
    bool fovUnlockedCurrent=false,fovUnlockDefaultsPreserved=false,followOverridesFov=false;
    uint8_t localRole=0,cameraState=255,viewTargetMode=255,defaultViewTargetMode=255;
    uint8_t viewTargetReady=0,allowedViewTargetModes=0,viewTargetTickEnabled=0;
    int32_t netMode=-1;
    unsigned error=0;
    uintptr_t exceptionRva=0;
    const char* stage="original_camera_unbound";
    const char* exceptionOperation="none";
    const char* operation="none";
    void *world=nullptr,*controller=nullptr,*pawn=nullptr,*camera=nullptr,*localPlayer=nullptr;
    void *viewTargetComponent=nullptr,*viewTargetActor=nullptr,*pendingViewTarget=nullptr;
    void *viewTargetInfoActor=nullptr,*localViewTargetActor=nullptr;
    void *lifecycleSelectedPawn=nullptr,*lifecycleInfoBefore=nullptr,*lifecycleLocalBefore=nullptr;
    void *lifecycleCameraBefore=nullptr,*lifecycleCameraAfter=nullptr;
    void *lifecycleOnRepFunction=nullptr;
    ViewTargetInfo lifecyclePreviousInfo{},lifecycleDeliveredInfo{};
    uint16_t lifecycleOnRepParameterBytes=0;
    void *demoDriver=nullptr,*netDriver=nullptr;
    ComponentSnapshot root,follow,custom;
    engine::Vec pawnPosition{},eyePosition{},eyeRotation{},cameraPosition{},cameraRotation{};
    engine::Vec followBefore{},followAfter{};
    float defaultFov=0,lockedFov=0,fov=0,fov1p=0,lastDelta=0,followFov=0;
    float bootstrapLockedFovBefore=0,bootstrapLockedFovAfter=0;
    float bootstrapDefaultFovBefore=0,bootstrapFirstPersonFovBefore=0;
    uint32_t cameraFlags=0;
    uint64_t samples=0,actorCallbacks=0,scheduledCalls=0,completedCalls=0;
    uint64_t skippedAlreadyNative=0,skippedNotFollow=0,skippedNotLocal=0;
    uint64_t skippedLocalPlayer=0,skippedLocalControl=0,skippedOwner=0,skippedViewTarget=0;
    uint64_t lifecycleRequests=0,lifecycleNativeCalls=0;
    uint64_t lifecycleInfoWrites=0,lifecycleOnRepCalls=0;
    uint64_t fovUnlockCalls=0;
};
inline Diagnostics state;
inline DWORD gameThread=0;
inline const Diagnostics& State(){return state;}
template<class T> inline T Value(const void* p,size_t offset){return original_players::Value<T>(p,offset);}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;return false;}
inline bool Thread(){return gameThread&&gameThread==GetCurrentThreadId()&&!state.restartRequired;}
inline bool Event(void* object,const wchar_t* method,uint16_t size,void* params){
    auto fn=engine::Find(method);
    if(!object||!fn||Value<uint16_t>(fn,0xbe)!=size)return false;
    return engine::Event(object,method,params);
}
inline bool ReadComponent(void* object,ComponentSnapshot& out){
    out={};out.object=object;
    out.present=original_players::IsA(object,SceneComponentClass);
    if(!out.present)return !object;
    uint8_t active=0;
    out.queried=Event(object,L"/Script/Engine.SceneComponent.K2_GetComponentLocation",24,&out.position)
        &&Event(object,L"/Script/Engine.SceneComponent.K2_GetComponentRotation",24,&out.rotation)
        &&Event(object,L"/Script/Engine.SceneComponent.GetAttachParent",8,&out.parent)
        &&Event(object,L"/Script/Engine.ActorComponent.IsActive",1,&active);
    out.active=active!=0;out.relativePosition=Value<engine::Vec>(object,0x170);
    return out.queried&&original_players::Finite(out.position)&&original_players::Finite(out.rotation);
}
inline bool RefreshGate(){
    if(!Thread()||!state.bound)return false;
    state.localRole=Value<uint8_t>(state.pawn,0x224);
    state.cameraState=Value<uint8_t>(state.pawn,0x12a0);
    state.canBeViewTarget=Value<uint8_t>(state.pawn,0x1290)!=0;
    state.demoDriver=Value<void*>(state.world,0xf8);state.noDemoDriver=!state.demoDriver;
    state.netDriver=Value<void*>(state.world,0x40);state.noNetDriver=!state.netDriver;
    state.netMode=engine::function<int32_t(*)(void*)>(0x3efc820)(state.controller);
    state.standalone=state.netMode==0;
    state.localPlayer=Value<void*>(state.controller,0x510);
    state.realLocalPlayer=original_players::IsA(state.localPlayer,0x3b6e680)
        &&Value<void*>(state.localPlayer,0x38)==state.controller;
    uint8_t local=0;
    state.locallyControlled=Event(state.pawn,L"/Script/Engine.Pawn.IsLocallyControlled",1,&local)&&local!=0;
    state.ownerMatches=Value<void*>(state.pawn,0x4a0)==state.controller
        &&Value<void*>(state.controller,0x4b0)==state.pawn
        &&Value<void*>(state.camera,0x468)==state.controller;
    state.viewTargetActor=Value<void*>(state.camera,0x580);
    state.pendingViewTarget=Value<void*>(state.camera,0xe80);
    state.viewTargetMatches=state.viewTargetActor==state.pawn&&!state.pendingViewTarget;
    state.viewTargetComponent=Value<void*>(state.controller,0xc60);
    state.viewTargetComponentPresent=original_players::IsA(state.viewTargetComponent,ViewTargetClass);
    state.viewTargetComponentOwnerMatches=state.viewTargetComponentPresent
        &&Value<void*>(state.viewTargetComponent,0xc8)==state.controller;
    state.viewTargetMode=255;state.defaultViewTargetMode=255;
    state.viewTargetInfoActor=nullptr;state.localViewTargetActor=nullptr;
    state.viewTargetReady=0;state.allowedViewTargetModes=0;state.viewTargetTickEnabled=0;
    if(state.viewTargetComponentPresent){
        state.viewTargetMode=Value<uint8_t>(state.viewTargetComponent,0x178);
        state.defaultViewTargetMode=Value<uint8_t>(state.viewTargetComponent,0x16b);
        state.viewTargetReady=Value<uint8_t>(state.viewTargetComponent,0x169);
        state.allowedViewTargetModes=Value<uint8_t>(state.viewTargetComponent,0x16a);
        state.viewTargetTickEnabled=Value<uint8_t>(state.viewTargetComponent,0x168);
        state.viewTargetInfoActor=Value<void*>(state.viewTargetComponent,0x170);
        state.localViewTargetActor=Value<void*>(state.viewTargetComponent,0x180);
    }
    state.predicateQueried=false;state.originalWorldPredicate=false;
    if(state.standalone&&state.noNetDriver&&state.noDemoDriver&&state.realLocalPlayer&&state.ownerMatches){
        // Exact original read-only predicate used at787d737. It considers local
        // spectating/dead-player state; using its return avoids duplicating its
        // gameplay decision with a guessed view-mode mapping.
        state.originalWorldPredicate=engine::function<bool(*)(void*)>(0x781fe40)(state.world);
        state.predicateQueried=true;
    }
    // Reproduce only the definitive early skip: Role!=Autonomous2, no death
    // camera2, native world predicate=false, and no replay driver. Replays and
    // native spectator/death branches retain their original sole scheduling.
    state.originalSchedulerSkips=state.canBeViewTarget&&state.localRole==3
        &&state.cameraState==0&&state.predicateQueried&&!state.originalWorldPredicate
        &&state.noDemoDriver&&state.noNetDriver&&state.standalone;
    state.lifecycleLinksCurrent=state.lifecycleRestored&&state.viewTargetMatches
        &&state.viewTargetComponentOwnerMatches&&state.viewTargetMode==1
        &&state.viewTargetInfoActor==state.pawn&&state.localViewTargetActor==state.pawn;
    state.fovUnlockedCurrent=state.fovUnlockCompleted&&Value<float>(state.camera,0x48c)==0;
    return true;
}
inline bool Sample(){
    if(!Thread()||!state.bound)return false;
    if(!RefreshGate())return false;
    if(!ReadComponent(Value<void*>(state.pawn,0x290),state.root)
        ||!ReadComponent(Value<void*>(state.pawn,0x12a8),state.follow)
        ||!ReadComponent(Value<void*>(state.pawn,0x12b0),state.custom))
        return Fail(1204,"original_camera_component_sample_failed");
    struct Eyes {engine::Vec position{},rotation{};} eyes;
    static_assert(sizeof(Eyes)==48);
    if(!Event(state.pawn,L"/Script/Engine.Actor.K2_GetActorLocation",24,&state.pawnPosition)
        ||!Event(state.pawn,L"/Script/Engine.Actor.GetActorEyesViewPoint",48,&eyes)
        ||!Event(state.camera,L"/Script/Engine.PlayerCameraManager.GetCameraLocation",24,&state.cameraPosition)
        ||!Event(state.camera,L"/Script/Engine.PlayerCameraManager.GetCameraRotation",24,&state.cameraRotation)
        ||!Event(state.camera,L"/Script/Engine.PlayerCameraManager.GetFOVAngle",4,&state.fov)
        ||!Event(state.camera,L"/Script/Engine.PlayerCameraManager.GetFOVAngle1P",4,&state.fov1p))
        return Fail(1205,"original_camera_native_sample_failed");
    state.eyePosition=eyes.position;state.eyeRotation=eyes.rotation;
    state.defaultFov=Value<float>(state.camera,0x484);state.lockedFov=Value<float>(state.camera,0x48c);
    state.followFov=Value<float>(state.follow.object,0x364);
    state.followOverridesFov=Value<uint8_t>(state.follow.object,0x360)!=0;
    state.cameraFlags=Value<uint32_t>(state.camera,0x2a78);
    ++state.samples;return true;
}
inline bool Initialize(void* world,void* controller,void* pawn){
    if(!engine::image||!world||!controller||!pawn)return Fail(1200,"original_camera_context_missing");
    if(gameThread&&gameThread!=GetCurrentThreadId())return Fail(1201,"original_camera_wrong_thread");
    gameThread=GetCurrentThreadId();state={};state.world=world;state.controller=controller;state.pawn=pawn;
    state.camera=Value<void*>(controller,0x528);
    state.originalCamera=original_players::IsA(state.camera,CameraClass);
    if(!original_players::IsA(controller,original_players::ShooterControllerClass)
        ||!original_players::IsA(pawn,original_players::ShooterCharacterClass)
        ||!state.originalCamera||!original_players::IsA(Value<void*>(pawn,0x12a8),CameraComponentClass))
        return Fail(1202,"original_camera_requires_real_original_player_and_camera");
    constexpr uint8_t entry[]{0x48,0x8b,0xc4,0x48,0x89,0x58,0x10,0x48,0x89,0x70,0x18,0x55,0x57,0x41,0x54,0x41};
    state.originalEntryVerified=std::memcmp(reinterpret_cast<const void*>(engine::image+0x787e760),entry,sizeof(entry))==0;
    if(!state.originalEntryVerified)return Fail(1203,"original_camera_native_entry_mismatch");
    constexpr uint8_t authTargetEntry[]{0x48,0x89,0x5c,0x24,0x10,0x56,0x48,0x83,0xec,0x40,0x48,0x8b,0xd9,0x48,0x8b,0xf2};
    constexpr uint8_t applyInfoEntry[]{0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x40,0x48,0x8b,0x99,0xc8,0x00,0x00};
    constexpr uint8_t possessedEntry[]{0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0x99,0xc8,0x00,0x00,0x00,0x48,0x85,0xdb};
    constexpr uint8_t validTargetEntry[]{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57};
    constexpr uint8_t onRepEntry[]{0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57,0x48,0x83,0xec,0x50,0x0f};
    state.lifecycleEntriesVerified=std::memcmp(reinterpret_cast<const void*>(engine::image+0x785a790),authTargetEntry,sizeof(authTargetEntry))==0
        &&std::memcmp(reinterpret_cast<const void*>(engine::image+0x785a880),applyInfoEntry,sizeof(applyInfoEntry))==0
        &&std::memcmp(reinterpret_cast<const void*>(engine::image+0x7859fe0),possessedEntry,sizeof(possessedEntry))==0
        &&std::memcmp(reinterpret_cast<const void*>(engine::image+0x785a040),validTargetEntry,sizeof(validTargetEntry))==0
        &&std::memcmp(reinterpret_cast<const void*>(engine::image+0x6dd7720),onRepEntry,sizeof(onRepEntry))==0;
    if(!state.lifecycleEntriesVerified)return Fail(1209,"original_camera_view_target_native_entry_mismatch");
    state.bound=true;if(!Sample())return false;
    state.error=0;state.stage="original_camera_local_scheduler_bound";return true;
}
inline bool AfterOriginalCharacterTickImpl(void* actor,float delta){
    if(!state.bound||actor!=state.pawn)return true;
    if(!Thread())return false;
    ++state.actorCallbacks;state.callbackObserved=true;state.lastDelta=delta;
    if(state.error)return false;
    if(!std::isfinite(delta)||delta<0||delta>1)return Fail(1206,"original_camera_invalid_native_tick_delta");
    if(!RefreshGate())return false;
    if(!state.realLocalPlayer||!state.locallyControlled||!state.ownerMatches||!state.viewTargetMatches){
        if(!state.realLocalPlayer)++state.skippedLocalPlayer;
        if(!state.locallyControlled)++state.skippedLocalControl;
        if(!state.ownerMatches)++state.skippedOwner;
        if(!state.viewTargetMatches)++state.skippedViewTarget;
        ++state.skippedNotLocal;return true;
    }
    if(state.cameraState!=0){++state.skippedNotFollow;return true;}
    if(!state.originalSchedulerSkips){++state.skippedAlreadyNative;return true;}
    auto follow=Value<void*>(state.pawn,0x12a8);
    if(!original_players::IsA(follow,CameraComponentClass))return Fail(1207,"original_camera_follow_component_missing");
    state.followBefore=Value<engine::Vec>(follow,0x320);
    state.operation="original_character_follow_recoil_camera_update";++state.scheduledCalls;
    // Original787e760 consumes the same original TickActor delta after native
    // movement. It calculates all camera position/rotation/recoil/zoom itself.
    // Normal PCM rendering/modifiers continue through their original world loop.
    engine::function<void(*)(void*,float)>(0x787e760)(state.pawn,delta);
    ++state.completedCalls;state.followAfter=Value<engine::Vec>(follow,0x320);
    state.operation="none";state.stage="original_camera_native_local_updates_executed";return true;
}
inline bool ReleaseBootstrapFovOverrideImpl(){
    if(!state.lifecycleRestored||!state.lifecycleLinksCurrent||!Thread())
        return Fail(1220,"original_camera_fov_unlock_requires_original_local_lifecycle");
    if(state.fovUnlockRequested)return state.fovUnlockCompleted&&state.fovUnlockedCurrent;
    state.fovUnlockRequested=true;
    state.bootstrapLockedFovBefore=Value<float>(state.camera,0x48c);
    state.bootstrapDefaultFovBefore=Value<float>(state.camera,0x484);
    state.bootstrapFirstPersonFovBefore=Value<float>(state.camera,0x488);
    auto table=Value<void*>(state.camera,0);auto unlock=Value<void*>(table,0x838);
    constexpr uint8_t entry[]{0xc7,0x81,0x8c,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0xc3};
    state.fovUnlockEntryVerified=unlock==reinterpret_cast<void*>(engine::image+0x4910c60)
        &&std::memcmp(unlock,entry,sizeof(entry))==0;
    if(!state.fovUnlockEntryVerified)return Fail(1221,"original_camera_native_unlock_fov_entry_mismatch");
    if(state.bootstrapLockedFovBefore!=0){
        // ActivateLocal copied the bootstrap PC.FOV override. Native GetFOVAngle
        // returns that lock instead of dynamic zoom POV while it is nonzero.
        // Use the original public virtual UnlockFOV once; do not replace any
        // normal/ADS FOV, curve, camera pose or first-person animation values.
        state.operation="original_player_camera_manager_unlock_bootstrap_fov";
        ++state.fovUnlockCalls;reinterpret_cast<void(*)(void*)>(unlock)(state.camera);
    }
    state.bootstrapLockedFovAfter=Value<float>(state.camera,0x48c);
    state.fovUnlockDefaultsPreserved=Value<float>(state.camera,0x484)==state.bootstrapDefaultFovBefore
        &&Value<float>(state.camera,0x488)==state.bootstrapFirstPersonFovBefore;
    state.fovUnlockedCurrent=state.bootstrapLockedFovAfter==0;
    if(!state.fovUnlockedCurrent||!state.fovUnlockDefaultsPreserved)
        return Fail(1222,"original_camera_native_unlock_fov_readback_failed");
    state.fovUnlockCompleted=true;state.operation="none";return true;
}
// Call explicitly once AFTER local HUD/client initialization, before accepting
// gameplay input. The missing local authority records its native-selected Pawn
// in ViewTargetInfo.TargetActor, then delivers the ORIGINAL OnRep notification.
// Mode/padding stay native. Never call from a per-frame loop or write PCM pose.
// The shipped PC.CalcCamera reads its previous PCM cache when its target is the
// PC itself, so treating that target as a valid Pawn camera would freeze the POV.
inline bool RestoreWatchPossessedLifecycleImpl(void* previousPawn=nullptr){
    if(!Thread()||!state.bound||state.error)return false;
    ++state.lifecycleRequests;
    if(!RefreshGate())return false;
    if(state.lifecycleRequested){
        if(state.lifecycleRestored&&state.lifecycleLinksCurrent)return ReleaseBootstrapFovOverrideImpl();
        return Fail(1210,"original_camera_view_target_lifecycle_already_attempted");
    }
    state.lifecycleRequested=true;
    state.lifecycleInfoBefore=state.viewTargetInfoActor;
    state.lifecycleLocalBefore=state.localViewTargetActor;
    state.lifecycleCameraBefore=state.viewTargetActor;
    if(!state.lifecycleEntriesVerified||!state.realLocalPlayer||!state.locallyControlled
        ||!state.ownerMatches||!state.standalone||!state.noNetDriver||!state.noDemoDriver
        ||state.localRole!=3||state.cameraState!=0||!state.canBeViewTarget
        ||!state.viewTargetComponentOwnerMatches||state.viewTargetMode!=1
        ||!state.viewTargetTickEnabled||state.pendingViewTarget)
        return Fail(1211,"original_camera_view_target_lifecycle_not_local_watch_possessed");
    // Permit only the exact local startup states. Never steal an intentional
    // spectator/projectile/other-actor camera or interrupt a pending blend.
    if(previousPawn){
        uint8_t alive=1;
        if(previousPawn==state.pawn||!original_players::IsA(previousPawn,original_players::ShooterCharacterClass)
           ||Value<void*>(previousPawn,0x488)!=Value<void*>(state.pawn,0x488)
           ||!Event(previousPawn,L"/Script/ShooterGame.ShooterCharacter.IsAlive",1,&alive)||alive)
            return Fail(1220,"original_camera_respawn_previous_pawn_not_confirmed_dead");
    }
    // A respawn may retain only the just-retired, verified dead pawn in the
    // logical ViewTargetInfo while ClientRestart already selected the new one.
    const auto localTarget=[previousPawn](void* actor){return !actor||actor==state.controller||actor==state.pawn||actor==previousPawn;};
    if(!localTarget(state.viewTargetInfoActor)||!localTarget(state.localViewTargetActor)
        ||!localTarget(state.viewTargetActor))
        return Fail(1212,"original_camera_view_target_lifecycle_other_actor_present");
    state.operation="original_view_target_select_possessed_character";
    state.lifecycleSelectedPawn=engine::function<void*(*)(void*)>(0x7859fe0)(state.viewTargetComponent);
    state.lifecycleCandidateValid=state.lifecycleSelectedPawn==state.pawn
        &&engine::function<bool(*)(void*,void*,uint8_t)>(0x785a040)(state.viewTargetComponent,state.lifecycleSelectedPawn,1);
    if(!state.lifecycleCandidateValid)return Fail(1213,"original_camera_view_target_native_candidate_rejected");
    if(!original_players::Read(state.viewTargetComponent,0x170,state.lifecyclePreviousInfo)
        ||state.lifecyclePreviousInfo.mode!=1)
        return Fail(1215,"original_camera_previous_view_target_info_unreadable");
    state.lifecycleDeliveredInfo=state.lifecyclePreviousInfo;
    state.lifecycleDeliveredInfo.targetActor=state.lifecycleSelectedPawn;
    // Generated UFunction metadata: PreviousViewTargetInfo is its single
    // 16-byte parameter at0; use ProcessEvent rather than inventing a native ABI.
    state.lifecycleOnRepFunction=engine::Find(L"/Script/ShooterGame.ViewTargetComponent.OnRep_ViewTargetInfo");
    state.lifecycleOnRepParameterBytes=Value<uint16_t>(state.lifecycleOnRepFunction,0xbe);
    state.lifecycleOnRepAbiValid=state.lifecycleOnRepFunction
        &&state.lifecycleOnRepFunction==engine::function<void*(*)()>(0x6dd82c0)()
        &&state.lifecycleOnRepParameterBytes==sizeof(ViewTargetInfo)
        &&Value<void*>(state.lifecycleOnRepFunction,0x28)==engine::Class(ViewTargetClass);
    if(!state.lifecycleOnRepAbiValid)return Fail(1216,"original_camera_view_target_onrep_abi_mismatch");
    if(state.lifecyclePreviousInfo.targetActor!=state.lifecycleSelectedPawn){
        state.operation="local_authority_record_native_selected_view_target_actor";
        // Only this actual object association is supplied locally. Do not write
        // Mode178, LocalPlayerViewTarget180, PCM targets, readiness or transforms.
        if(!original_players::Write(state.viewTargetComponent,0x170,state.lifecycleSelectedPawn))
            return Fail(1217,"original_camera_local_view_target_actor_record_failed");
        ++state.lifecycleInfoWrites;state.lifecycleInfoWritten=true;
        const auto recorded=Value<ViewTargetInfo>(state.viewTargetComponent,0x170);
        if(std::memcmp(&recorded,&state.lifecycleDeliveredInfo,sizeof(recorded))!=0)
            return Fail(1218,"original_camera_view_target_record_changed_other_fields");
        state.operation="original_view_target_onrep_previous_info_notification";
        state.lifecycleUsedOnRep=true;++state.lifecycleNativeCalls;++state.lifecycleOnRepCalls;
        auto previousInfo=state.lifecyclePreviousInfo;
        if(!Event(state.viewTargetComponent,L"/Script/ShooterGame.ViewTargetComponent.OnRep_ViewTargetInfo",
            static_cast<uint16_t>(sizeof(previousInfo)),&previousInfo))
            return Fail(1219,"original_camera_view_target_onrep_notification_failed");
    }else if(!state.viewTargetMatches||state.localViewTargetActor!=state.pawn){
        // The logical native record is already correct: replay only its original
        // apply leaf, which uses the SAME native SetViewTarget/delegate pipeline.
        state.lifecycleUsedApplyInfo=true;state.operation="original_view_target_apply_existing_info";
        ++state.lifecycleNativeCalls;
        engine::function<void(*)(void*)>(0x785a880)(state.viewTargetComponent);
    }
    if(!RefreshGate())return false;
    state.lifecycleCameraAfter=state.viewTargetActor;
    const auto retained=Value<ViewTargetInfo>(state.viewTargetComponent,0x170);
    state.lifecycleInfoPreserved=std::memcmp(&retained,&state.lifecycleDeliveredInfo,sizeof(retained))==0;
    if(!state.realLocalPlayer||!state.locallyControlled||!state.ownerMatches
        ||!state.viewTargetComponentOwnerMatches||state.viewTargetMode!=1
        ||!state.viewTargetMatches||state.localViewTargetActor!=state.pawn
        ||state.viewTargetInfoActor!=state.pawn||!state.lifecycleInfoPreserved)
        return Fail(1214,"original_camera_view_target_native_lifecycle_not_observed");
    state.lifecycleRestored=true;state.lifecycleLinksCurrent=true;state.operation="none";
    if(!ReleaseBootstrapFovOverrideImpl())return false;
    state.stage="original_camera_watch_possessed_lifecycle_restored";return true;
}
inline int CaptureException(EXCEPTION_POINTERS* exception){
    const auto at=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;
    state.exceptionOperation=state.operation;state.restartRequired=true;
    Fail(1208,"original_camera_native_exception_requires_new_session");return EXCEPTION_EXECUTE_HANDLER;
}
inline bool AfterOriginalCharacterTick(void* actor,float delta){
    __try{return AfterOriginalCharacterTickImpl(actor,delta);}
    __except(CaptureException(GetExceptionInformation())){return false;}
}
inline bool RestoreWatchPossessedLifecycle(void* previousPawn=nullptr){
    __try{return RestoreWatchPossessedLifecycleImpl(previousPawn);}
    __except(CaptureException(GetExceptionInformation())){return false;}
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path||!gameThread)return false;
    FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(f,"{\"schema_version\":4,\"stage\":");original_players::Text(f,state.stage);
    std::fprintf(f,",\"error\":%u,\"restart_required\":%s,\"exception_rva\":%llu,\"exception_operation\":",state.error,b(state.restartRequired),static_cast<unsigned long long>(state.exceptionRva));original_players::Text(f,state.exceptionOperation);
    const char* bn[]{"bound","original_camera","real_local_player","locally_controlled","standalone","no_demo_driver","no_net_driver","original_world_predicate","predicate_queried","can_be_view_target","view_target_matches","owner_matches","original_scheduler_skips","callback_observed","original_entry_verified","view_target_component_present","view_target_component_owner_matches","lifecycle_entries_verified","lifecycle_requested","lifecycle_restored","lifecycle_candidate_valid","lifecycle_used_apply_info","lifecycle_onrep_abi_valid","lifecycle_used_onrep","lifecycle_info_written","lifecycle_info_preserved","lifecycle_links_current","fov_unlock_requested","fov_unlock_completed","fov_unlock_entry_verified","fov_unlocked_current","fov_unlock_defaults_preserved","follow_overrides_fov"};
    const bool bv[]{state.bound,state.originalCamera,state.realLocalPlayer,state.locallyControlled,state.standalone,state.noDemoDriver,state.noNetDriver,state.originalWorldPredicate,state.predicateQueried,state.canBeViewTarget,state.viewTargetMatches,state.ownerMatches,state.originalSchedulerSkips,state.callbackObserved,state.originalEntryVerified,state.viewTargetComponentPresent,state.viewTargetComponentOwnerMatches,state.lifecycleEntriesVerified,state.lifecycleRequested,state.lifecycleRestored,state.lifecycleCandidateValid,state.lifecycleUsedApplyInfo,state.lifecycleOnRepAbiValid,state.lifecycleUsedOnRep,state.lifecycleInfoWritten,state.lifecycleInfoPreserved,state.lifecycleLinksCurrent,state.fovUnlockRequested,state.fovUnlockCompleted,state.fovUnlockEntryVerified,state.fovUnlockedCurrent,state.fovUnlockDefaultsPreserved,state.followOverridesFov};
    for(size_t i=0;i<sizeof(bv)/sizeof(bv[0]);++i)std::fprintf(f,",\"%s\":%s",bn[i],b(bv[i]));
    std::fprintf(f,",\"local_role\":%u,\"camera_state\":%u,\"view_target_mode\":%u,\"default_view_target_mode\":%u,\"view_target_ready\":%u,\"net_mode\":%d,\"camera_flags\":%u",state.localRole,state.cameraState,state.viewTargetMode,state.defaultViewTargetMode,state.viewTargetReady,state.netMode,state.cameraFlags);
    std::fprintf(f,",\"allowed_view_target_modes\":%u,\"view_target_tick_enabled\":%u",state.allowedViewTargetModes,state.viewTargetTickEnabled);
    std::fprintf(f,",\"samples\":%llu,\"actor_callbacks\":%llu,\"scheduled_calls\":%llu,\"completed_calls\":%llu,\"skipped_already_native\":%llu,\"skipped_not_follow\":%llu,\"skipped_not_local\":%llu,\"last_delta\":%.9g",static_cast<unsigned long long>(state.samples),static_cast<unsigned long long>(state.actorCallbacks),static_cast<unsigned long long>(state.scheduledCalls),static_cast<unsigned long long>(state.completedCalls),static_cast<unsigned long long>(state.skippedAlreadyNative),static_cast<unsigned long long>(state.skippedNotFollow),static_cast<unsigned long long>(state.skippedNotLocal),state.lastDelta);
    std::fprintf(f,",\"skipped_local_player\":%llu,\"skipped_local_control\":%llu,\"skipped_owner\":%llu,\"skipped_view_target\":%llu,\"lifecycle_requests\":%llu,\"lifecycle_native_calls\":%llu",static_cast<unsigned long long>(state.skippedLocalPlayer),static_cast<unsigned long long>(state.skippedLocalControl),static_cast<unsigned long long>(state.skippedOwner),static_cast<unsigned long long>(state.skippedViewTarget),static_cast<unsigned long long>(state.lifecycleRequests),static_cast<unsigned long long>(state.lifecycleNativeCalls));
    std::fprintf(f,",\"lifecycle_info_writes\":%llu,\"lifecycle_onrep_calls\":%llu,\"lifecycle_onrep_parameter_bytes\":%u",static_cast<unsigned long long>(state.lifecycleInfoWrites),static_cast<unsigned long long>(state.lifecycleOnRepCalls),state.lifecycleOnRepParameterBytes);
    std::fprintf(f,",\"default_fov\":%.9g,\"locked_fov\":%.9g,\"fov\":%.9g,\"fov_1p\":%.9g",state.defaultFov,state.lockedFov,state.fov,state.fov1p);
    std::fprintf(f,",\"follow_fov\":%.9g,\"fov_unlock_calls\":%llu,\"bootstrap_locked_fov_before\":%.9g,\"bootstrap_locked_fov_after\":%.9g,\"bootstrap_default_fov_before\":%.9g,\"bootstrap_first_person_fov_before\":%.9g",state.followFov,static_cast<unsigned long long>(state.fovUnlockCalls),state.bootstrapLockedFovBefore,state.bootstrapLockedFovAfter,state.bootstrapDefaultFovBefore,state.bootstrapFirstPersonFovBefore);
    const char* pn[]{"world","controller","pawn","camera","local_player","view_target_component","view_target_actor","pending_view_target","demo_driver","net_driver","view_target_info_actor","local_view_target_actor","lifecycle_selected_pawn","lifecycle_info_before","lifecycle_local_before","lifecycle_camera_before","lifecycle_camera_after","lifecycle_onrep_function"};
    void* const pv[]{state.world,state.controller,state.pawn,state.camera,state.localPlayer,state.viewTargetComponent,state.viewTargetActor,state.pendingViewTarget,state.demoDriver,state.netDriver,state.viewTargetInfoActor,state.localViewTargetActor,state.lifecycleSelectedPawn,state.lifecycleInfoBefore,state.lifecycleLocalBefore,state.lifecycleCameraBefore,state.lifecycleCameraAfter,state.lifecycleOnRepFunction};
    for(size_t i=0;i<sizeof(pv)/sizeof(pv[0]);++i)std::fprintf(f,",\"%s\":\"%p\"",pn[i],pv[i]);
    const char* in[]{"lifecycle_previous_info","lifecycle_delivered_info"};
    const ViewTargetInfo* iv[]{&state.lifecyclePreviousInfo,&state.lifecycleDeliveredInfo};
    for(size_t i=0;i<2;++i){const auto& info=*iv[i];std::fprintf(f,",\"%s\":{\"target_actor\":\"%p\",\"mode\":%u,\"reserved\":[",in[i],info.targetActor,info.mode);for(size_t j=0;j<7;++j)std::fprintf(f,"%s%u",j?",":"",info.reserved[j]);std::fputs("]}",f);}
    const char* vn[]{"pawn_position","eye_position","eye_rotation","camera_position","camera_rotation","follow_before","follow_after"};
    const engine::Vec* vv[]{&state.pawnPosition,&state.eyePosition,&state.eyeRotation,&state.cameraPosition,&state.cameraRotation,&state.followBefore,&state.followAfter};
    for(size_t i=0;i<sizeof(vv)/sizeof(vv[0]);++i){std::fprintf(f,",\"%s\":",vn[i]);original_players::Vector(f,*vv[i]);}
    const char* cn[]{"root","follow","custom"};const ComponentSnapshot* cv[]{&state.root,&state.follow,&state.custom};
    for(size_t i=0;i<3;++i){const auto& c=*cv[i];std::fprintf(f,",\"%s\":{\"object\":\"%p\",\"parent\":\"%p\",\"present\":%s,\"active\":%s,\"queried\":%s,\"position\":",cn[i],c.object,c.parent,b(c.present),b(c.active),b(c.queried));original_players::Vector(f,c.position);std::fputs(",\"rotation\":",f);original_players::Vector(f,c.rotation);std::fputs(",\"relative_position\":",f);original_players::Vector(f,c.relativePosition);std::fputc('}',f);}
    std::fputs(",\"runtime_pixels_verified\":false,\"camera_pose_formula_replaced\":false,\"physics_or_role_modified\":false,\"per_frame_view_target_override\":false,\"ready_flag_written\":false,\"view_target_mode_written\":false,\"pcm_target_written\":false}\n",f);
    const bool ok=std::ferror(f)==0;std::fclose(f);return ok;
}
}
