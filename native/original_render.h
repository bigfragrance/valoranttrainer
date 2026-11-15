#pragma once
#include "original_players.h"
#include <algorithm>
#include <array>

// Read-only renderer/lifecycle snapshot. In particular this never calls a
// transform getter that can recursively update bone/socket attachments, never
// forces pose evaluation, and never changes visibility or render flags.
// Pinned metadata: work/duel1-original-render/render-properties.json.
namespace original_render {
inline constexpr size_t MaxActors=32,MaxComponents=160,MaxChain=32;
inline constexpr size_t MaxViewSamples=64,MaxGunPoseBones=64;
struct Component {
    const char* label="none";
    size_t rosterIndex=0;
    void *object=nullptr,*actor=nullptr,*parent=nullptr,*parentOwner=nullptr;
    void *skinnedAsset=nullptr,*skeletalMesh=nullptr,*animInstance=nullptr,*animClass=nullptr;
    void *leader=nullptr,*poseData=nullptr;
    void *rawPrimitive3a0=nullptr,*rawPrimitive3a8=nullptr,*rawPrimitive6b0=nullptr;
    void* rawPrimitive690=nullptr;
    bool present=false,primitive=false,skinned=false,skeletal=false;
    bool registered=false,renderStateCreated=false,active=false,visible=false,hiddenInGame=false;
    bool renderInMainPass=false,ownerNoSee=false,onlyOwnerSee=false,sceneCaptureOnly=false,firstPersonRendering=false;
    bool absoluteLocation=false,absoluteRotation=false,absoluteScale=false;
    bool recentRender=false,pauseAnims=false,onlyAutonomousPose=false,autonomousPose=false;
    bool attachCycle=false,attachTruncated=false,leaderCycle=false,leaderTruncated=false;
    uint8_t componentFlags=0,sceneFlags=0,visibilityAnimTickOption=255,animationMode=255;
    uint8_t primitiveFlags1=0,primitiveFlags2=0,skinnedFlags=0,skelFlags=0;
    int32_t attachChildren=0,attachCapacity=0,leaderObjectIndex=-1,leaderSerial=0;
    int32_t poseCount=0,poseCapacity=0,poseBufferIndex=-1,boneMapCount=0;
    uint64_t lastPoseTickFrame=0;
    float globalAnimRate=0,lastRenderTime=0,lastRenderTimeOnScreen=0;
    engine::Vec location{},scale{},relativeLocation{},relativeRotation{},relativeScale{},boundsOrigin{},boundsExtent{};
    std::array<double,4> worldQuaternion{};
    double boundsRadius=0;
    void* attachChain[MaxChain]{};
    void* leaderChain[MaxChain]{};
    uint32_t attachDepth=0,leaderDepth=0;
};
struct Actor {
    const char* label="none";
    size_t rosterIndex=0;
    void *object=nullptr,*root=nullptr,*owner=nullptr,*controller=nullptr;
    bool hidden=false,locallyHidden=false,hiddenCombined=false;
    uint8_t flags=0,role=0;
    engine::Vec location{},scale{};
};
struct MatrixSnapshot {
    std::array<double,16> values{};
    bool readable=false,finite=false;
    uint64_t changes=0;
};
struct CachedPose {
    void *object=nullptr,*parent=nullptr,*owner=nullptr;
    bool readable=false,firstPersonRendering=false;
    uint8_t paddingBeforeWorld[6]{};
    engine::Transform world{};
    engine::Vec relativeLocation{},relativeRotation{};
    uint64_t poseFrame=0;
    uint8_t paddingAfterPoseFrame[8]{};
};
static_assert(sizeof(CachedPose)==192&&offsetof(CachedPose,world)==32);
struct GunPoseCache {
    void *mesh=nullptr,*asset=nullptr,*data=nullptr;
    int32_t bufferIndex=-1,count=0,capacity=0,leaderIndex=-1,leaderSerial=0;
    uint8_t paddingBeforePoseFrame[4]{};
    uint64_t poseFrame=0;
    bool readable=false,stable=false,truncated=false;
    uint8_t paddingBeforeCaptured[1]{};
    uint32_t captured=0;
    std::array<engine::Transform,MaxGunPoseBones> transforms{};
};
static_assert(sizeof(GunPoseCache)==64+96*MaxGunPoseBones&&offsetof(GunPoseCache,transforms)==64);
struct ViewSample {
    uint64_t sequence=0,tag=0,wallTimeMs=0;
    double worldTime=0,shotTime=-1;
    void *controller=nullptr,*pawn=nullptr,*player=nullptr,*camera=nullptr,*cameraOwner=nullptr,*viewTarget=nullptr;
    void *inputState=nullptr,*weapon=nullptr;
    int32_t worldControllerCount=0,worldFirstWeakIndex=-1,worldFirstWeakSerial=0,controllerObjectIndex=-1;
    bool localLinks=false,cameraOwnerMatches=false,firstWorldIndexMatches=false,cameraCacheLayoutVerified=false;
    bool hasNativeRay=false,aiControlled=false;
    uint8_t pawnRole=0;
    uint8_t paddingBeforeRotations[1]{};
    engine::Vec controlRotation{},inputAimDirection{},pawnFacingControlRotation{};
    engine::Vec cameraPosition{},cameraRotation{},previousCameraPosition{},previousCameraRotation{};
    engine::Vec nativeRayOrigin{},nativeRayDirection{};
    float cameraTime=0,previousCameraTime=0,cachedFov=0,previousCachedFov=0,defaultFov=0,lockedFov=0,firstPersonFov=0;
    std::array<uint32_t,3> muzzlePointSocketName{};
    CachedPose pawnRoot{},follow{},core1p{},cosmetic1p{},gun1p{},gun3p{};
    int32_t gunPoseBuffer=-1,gunPoseCount=0;
    uint64_t gunPoseFrame=0;
    bool gunPoseStable=false;
    uint8_t paddingAfterGunPoseStable[15]{};
};
static_assert(sizeof(ViewSample)==1568);
struct Diagnostics {
    void* world=nullptr;
    void *scene=nullptr,*sceneVtable=nullptr,*addPrimitive=nullptr,*removePrimitive=nullptr;
    void *worldControllerArray=nullptr,*worldFirstController=nullptr,*worldFirstPlayer=nullptr;
    void *activeController=nullptr,*activePlayer=nullptr,*activeLocalPlayer=nullptr,*localPlayerController=nullptr;
    void *camera=nullptr,*cameraOwner=nullptr,*sceneProjectionUpdate=nullptr;
    int32_t worldControllerCount=0,worldFirstObjectIndex=-1,worldFirstObjectSerial=0;
    bool worldFirstIsActive=false,activePlayerIsLocalPlayer=false,localPlayerRoutesActive=false;
    bool shooterCamera=false,sceneProjectionCacheVerified=false;
    MatrixSnapshot cameraProjectionCorrection{},sceneWorldProjection{},sceneFirstPersonProjection{};
    bool bound=false;
    unsigned error=0;
    const char* stage="original_render_unbound";
    uint64_t samples=0;
    uint32_t actorCount=0,componentCount=0,attachCycles=0,leaderCycles=0;
    double worldTime=0;
    uint64_t viewSamples=0;
    size_t nextViewSample=0,viewHistoryCount=0;
    uint8_t paddingBeforeLatestView[8]{};
    ViewSample latestView{};
    std::array<ViewSample,MaxViewSamples> viewHistory{};
    GunPoseCache latestGunPose{};
    std::array<Actor,MaxActors> actors{};
    std::array<Component,MaxComponents> components{};
};
inline Diagnostics state;
inline const Diagnostics& State(){return state;}
template<class T> inline T Value(const void* p,size_t offset){return original_players::Value<T>(p,offset);}
inline bool Fail(unsigned code,const char* text){state.error=code;state.stage=text;return false;}
inline bool Contains(void* const* array,size_t count,void* p){for(size_t i=0;i<count;++i)if(array[i]==p)return true;return false;}
inline CachedPose ReadCachedPose(void* object,bool primitive,bool skeletal){
    CachedPose out;out.object=object;if(!object)return out;
    out.parent=Value<void*>(object,0xe8);out.owner=Value<void*>(object,0xc8);
    out.readable=original_players::Read(object,0x300,out.world)
      &&original_players::Read(object,0x170,out.relativeLocation)&&original_players::Read(object,0x188,out.relativeRotation);
    if(primitive)out.firstPersonRendering=(Value<uint32_t>(object,0x394)&1)!=0;
    if(skeletal)out.poseFrame=Value<uint64_t>(object,0xd78);
    return out;
}
inline void SampleGunPoseCache(void* mesh){
    auto& out=state.latestGunPose;out={};out.mesh=mesh;if(!mesh)return;
    out.asset=Value<void*>(mesh,0x710);out.poseFrame=Value<uint64_t>(mesh,0xd78);
    out.leaderIndex=Value<int32_t>(mesh,0x718);out.leaderSerial=Value<int32_t>(mesh,0x71c);
    out.bufferIndex=Value<int32_t>(mesh,0x7a8);
    if(out.bufferIndex<0||out.bufferIndex>1||!out.poseFrame)return;
    const size_t at=0x760+static_cast<size_t>(out.bufferIndex)*16;
    out.data=Value<void*>(mesh,at);out.count=Value<int32_t>(mesh,at+8);out.capacity=Value<int32_t>(mesh,at+12);
    if(!out.data||out.count<=0||out.count>4096||out.capacity<out.count||out.capacity>8192)return;
    // Exact no-leader GetBoneTransform4249af5..4249b1f uses this cached
    // ComponentSpaceTransforms array and96-byte stride. No bone/socket getter,
    // transform update, name lookup, or pose evaluation is invoked here.
    out.captured=static_cast<uint32_t>((std::min)(static_cast<size_t>(out.count),MaxGunPoseBones));
    out.truncated=static_cast<size_t>(out.count)>MaxGunPoseBones;out.readable=true;
    for(uint32_t i=0;i<out.captured;++i)if(!original_players::Read(out.data,static_cast<size_t>(i)*96,out.transforms[i])){out.readable=false;break;}
    out.stable=out.readable&&out.bufferIndex==Value<int32_t>(mesh,0x7a8)&&out.poseFrame==Value<uint64_t>(mesh,0xd78)
      &&out.data==Value<void*>(mesh,at)&&out.count==Value<int32_t>(mesh,at+8)&&out.asset==Value<void*>(mesh,0x710);
}
// Optional event-time checkpoints. Pass the untouched native shot time/origin/
// direction to correlate a FireShot with cached view/pose data. The regular
// SampleRoster also records tag0. These records never generate or change rays.
inline bool CaptureViewSample(uint64_t tag=0,double nativeShotTime=-1,const engine::Vec* nativeOrigin=nullptr,const engine::Vec* nativeDirection=nullptr){
    if(!state.bound||!original_players::Thread())return false;
    ViewSample out;out.sequence=++state.viewSamples;out.tag=tag;out.wallTimeMs=GetTickCount64();
    out.worldTime=Value<double>(state.world,0x740);out.shotTime=nativeShotTime;
    out.controller=original_players::activeController;out.pawn=original_players::activePawn;
    out.player=Value<void*>(out.controller,0x510);out.camera=Value<void*>(out.controller,0x528);
    out.cameraOwner=Value<void*>(out.camera,0x468);out.viewTarget=Value<void*>(out.camera,0x580);
    out.localLinks=out.player&&out.player==original_players::localPlayer&&Value<void*>(out.player,0x38)==out.controller;
    out.cameraOwnerMatches=out.cameraOwner&&out.cameraOwner==out.controller;
    out.worldControllerCount=Value<int32_t>(state.world,0x220);out.controllerObjectIndex=Value<int32_t>(out.controller,0xc);
    auto weak=Value<void*>(state.world,0x218);
    if(weak&&out.worldControllerCount>0){out.worldFirstWeakIndex=Value<int32_t>(weak,0);out.worldFirstWeakSerial=Value<int32_t>(weak,4);}
    out.firstWorldIndexMatches=out.worldFirstWeakSerial>0&&out.worldFirstWeakIndex==out.controllerObjectIndex;
    out.controlRotation=Value<engine::Vec>(out.controller,0x4e8);out.inputState=Value<void*>(out.pawn,0xba0);
    // Native787fa10 ->19ad970 reads238 as FVector and converts Vector.Rotation.
    // ShooterCharacter.FacingControlRotation1158 is a Pawn Rotator field.
    out.inputAimDirection=Value<engine::Vec>(out.inputState,0x238);
    out.pawnFacingControlRotation=Value<engine::Vec>(out.pawn,0x1158);
    out.pawnRole=Value<uint8_t>(out.pawn,0x224);out.aiControlled=Value<uint8_t>(out.pawn,0x152c)!=0;
    const auto cameraTable=Value<void*>(out.camera,0);
    out.cameraCacheLayoutVerified=Value<void*>(cameraTable,0x7a0)==reinterpret_cast<void*>(engine::image+0x4914b80);
    if(out.cameraCacheLayoutVerified){
      // Original4914b80/90 return current/previous POV at17c0/20b0.
      // Original4911a90 stores their timestamps17b0/20a0.
      out.cameraPosition=Value<engine::Vec>(out.camera,0x17c0);out.cameraRotation=Value<engine::Vec>(out.camera,0x17d8);
      out.previousCameraPosition=Value<engine::Vec>(out.camera,0x20b0);out.previousCameraRotation=Value<engine::Vec>(out.camera,0x20c8);
      out.cameraTime=Value<float>(out.camera,0x17b0);out.previousCameraTime=Value<float>(out.camera,0x20a0);
      out.cachedFov=Value<float>(out.camera,0x17f0);out.previousCachedFov=Value<float>(out.camera,0x20e0);
    }
    out.defaultFov=Value<float>(out.camera,0x484);out.firstPersonFov=Value<float>(out.camera,0x488);out.lockedFov=Value<float>(out.camera,0x48c);
    out.pawnRoot=ReadCachedPose(Value<void*>(out.pawn,0x290),true,false);
    out.follow=ReadCachedPose(Value<void*>(out.pawn,0x12a8),false,false);
    out.core1p=ReadCachedPose(Value<void*>(out.pawn,0xf30),true,true);
    out.cosmetic1p=ReadCachedPose(Value<void*>(out.pawn,0xf38),true,true);
    auto inventory=Value<void*>(out.pawn,0xc08);out.weapon=Value<void*>(inventory,0x280);
    out.gun1p=ReadCachedPose(Value<void*>(out.weapon,0xde0),true,true);out.gun3p=ReadCachedPose(Value<void*>(out.weapon,0xdf0),true,true);
    out.muzzlePointSocketName=Value<std::array<uint32_t,3>>(out.weapon,0xea8);
    SampleGunPoseCache(out.gun1p.object);out.gunPoseBuffer=state.latestGunPose.bufferIndex;out.gunPoseCount=state.latestGunPose.count;
    out.gunPoseFrame=state.latestGunPose.poseFrame;out.gunPoseStable=state.latestGunPose.stable;
    if(nativeOrigin&&nativeDirection){out.hasNativeRay=true;out.nativeRayOrigin=*nativeOrigin;out.nativeRayDirection=*nativeDirection;}
    state.latestView=out;state.viewHistory[state.nextViewSample]=out;state.nextViewSample=(state.nextViewSample+1)%MaxViewSamples;
    if(state.viewHistoryCount<MaxViewSamples)++state.viewHistoryCount;return true;
}
inline void SampleMatrix(MatrixSnapshot& out,void* object,size_t offset){
    std::array<double,16> values{};
    const bool readable=original_players::Read(object,offset,values);
    bool finite=readable;
    if(readable)for(double value:values)if(!std::isfinite(value)){finite=false;break;}
    if(readable&&out.readable&&std::memcmp(values.data(),out.values.data(),sizeof(values))!=0)++out.changes;
    out.values=values;out.readable=readable;out.finite=finite;
}
inline void SampleProjectionRouting(){
    // ShooterCamera786a100 explicitly selects the first weak World controller,
    // not GameplayStatics.GetPlayerController's GI.LocalPlayers-priority path.
    // Its early return for a null PC.Player+510 skips both native 1P matrices.
    state.worldControllerArray=Value<void*>(state.world,0x218);
    state.worldControllerCount=Value<int32_t>(state.world,0x220);
    state.worldFirstController=nullptr;state.worldFirstObjectIndex=-1;state.worldFirstObjectSerial=0;
    if(state.worldControllerArray&&state.worldControllerCount>0&&state.worldControllerCount<=4096){
        state.worldFirstObjectIndex=Value<int32_t>(state.worldControllerArray,0);
        state.worldFirstObjectSerial=Value<int32_t>(state.worldControllerArray,4);
        state.worldFirstController=engine::function<void*(*)(void*)>(0x1d4cfa0)(state.worldControllerArray);
    }
    state.worldFirstPlayer=Value<void*>(state.worldFirstController,0x510);
    state.activeController=original_players::activeController;
    state.activeLocalPlayer=original_players::localPlayer;
    state.activePlayer=Value<void*>(state.activeController,0x510);
    state.localPlayerController=Value<void*>(state.activeLocalPlayer,0x38);
    state.worldFirstIsActive=state.worldFirstController&&state.worldFirstController==state.activeController;
    state.activePlayerIsLocalPlayer=state.activePlayer&&state.activePlayer==state.activeLocalPlayer;
    state.localPlayerRoutesActive=state.localPlayerController&&state.localPlayerController==state.activeController;
    state.camera=Value<void*>(state.activeController,0x528);
    state.cameraOwner=Value<void*>(state.camera,0x468);
    state.shooterCamera=original_players::IsA(state.camera,0x6c5b630);
    if(state.shooterCamera)SampleMatrix(state.cameraProjectionCorrection,state.camera,0x490);
    else{state.cameraProjectionCorrection.readable=false;state.cameraProjectionCorrection.finite=false;}
    state.sceneProjectionUpdate=Value<void*>(state.sceneVtable,0x3e8);
    // Runtime Scene vtable a8da358 selects3142430. Its original body stores
    // exact128-byte double matrices at3be0/3c60, sometimes asynchronously on
    // the render thread. These are readonly snapshots, never updates.
    state.sceneProjectionCacheVerified=state.sceneProjectionUpdate==reinterpret_cast<void*>(engine::image+0x3142430);
    if(state.sceneProjectionCacheVerified){
        SampleMatrix(state.sceneWorldProjection,state.scene,0x3be0);
        SampleMatrix(state.sceneFirstPersonProjection,state.scene,0x3c60);
    }else{
        state.sceneWorldProjection.readable=state.sceneWorldProjection.finite=false;
        state.sceneFirstPersonProjection.readable=state.sceneFirstPersonProjection.finite=false;
    }
}
inline void ReadAttachmentChain(Component& out){
    void* current=out.object;
    while(current&&out.attachDepth<MaxChain){
        if(Contains(out.attachChain,out.attachDepth,current)){out.attachCycle=true;break;}
        out.attachChain[out.attachDepth++]=current;
        current=Value<void*>(current,0xe8);
    }
    out.attachTruncated=current&&!out.attachCycle;
    if(out.attachCycle)++state.attachCycles;
}
inline void* ResolveLeader(void* component){
    return engine::function<void*(*)(void*)>(0x1d4cfa0)(static_cast<uint8_t*>(component)+0x718);
}
inline void ReadLeaderChain(Component& out){
    void* current=out.object;
    while(current&&out.leaderDepth<MaxChain){
        if(Contains(out.leaderChain,out.leaderDepth,current)){out.leaderCycle=true;break;}
        out.leaderChain[out.leaderDepth++]=current;
        if(!original_players::IsA(current,0x3ea3e80))break;
        current=ResolveLeader(current);
    }
    out.leaderTruncated=current&&!out.leaderCycle&&out.leaderDepth==MaxChain;
    if(out.leaderCycle)++state.leaderCycles;
}
inline void AddComponent(size_t rosterIndex,void* actor,void* component,const char* label){
    if(!component||state.componentCount>=MaxComponents)return;
    for(uint32_t i=0;i<state.componentCount;++i)if(state.components[i].object==component)return;
    auto& out=state.components[state.componentCount++];out={};
    out.rosterIndex=rosterIndex;out.actor=actor;out.object=component;out.label=label;
    out.present=original_players::IsA(component,0x36c7580);
    if(!out.present)return;
    out.primitive=original_players::IsA(component,0x2a653a0);
    out.skeletal=original_players::IsA(component,0x385f750);
    out.skinned=original_players::IsA(component,0x3ea3e80);
    out.componentFlags=Value<uint8_t>(component,0xa0);
    out.registered=(out.componentFlags&1)!=0;
    out.renderStateCreated=(out.componentFlags&2)!=0;
    out.active=(Value<uint8_t>(component,0xc1)&0x20)!=0;
    out.sceneFlags=Value<uint8_t>(component,0x1d0);
    out.visible=(out.sceneFlags&0x20)!=0;
    out.absoluteLocation=(out.sceneFlags&4)!=0;out.absoluteRotation=(out.sceneFlags&8)!=0;out.absoluteScale=(out.sceneFlags&0x10)!=0;
    out.hiddenInGame=(Value<uint8_t>(component,0x251)&2)!=0;
    out.parent=Value<void*>(component,0xe8);out.parentOwner=Value<void*>(out.parent,0xc8);
    out.attachChildren=Value<int32_t>(component,0x108);out.attachCapacity=Value<int32_t>(component,0x10c);
    out.location=Value<engine::Vec>(component,0x320);out.scale=Value<engine::Vec>(component,0x340);
    out.worldQuaternion=Value<std::array<double,4>>(component,0x300);
    out.relativeLocation=Value<engine::Vec>(component,0x170);out.relativeRotation=Value<engine::Vec>(component,0x188);out.relativeScale=Value<engine::Vec>(component,0x1a0);
    // Native KismetSystemLibrary.GetComponentBounds45b1c30 copies these exact
    // cached fields at45b1f1c/35/4d, without updating any transform.
    out.boundsOrigin=Value<engine::Vec>(component,0x138);out.boundsExtent=Value<engine::Vec>(component,0x150);out.boundsRadius=Value<double>(component,0x168);
    ReadAttachmentChain(out);
    if(out.primitive){
        out.primitiveFlags1=Value<uint8_t>(component,0x3b1);out.primitiveFlags2=Value<uint8_t>(component,0x3b2);
        out.renderInMainPass=(out.primitiveFlags1&0x10)!=0;out.ownerNoSee=(out.primitiveFlags2&8)!=0;out.onlyOwnerSee=(out.primitiveFlags2&0x10)!=0;
        out.sceneCaptureOnly=(Value<uint8_t>(component,0x3b9)&2)!=0;
        out.firstPersonRendering=(Value<uint32_t>(component,0x394)&1)!=0;
        out.lastRenderTime=Value<float>(component,0x4ac);out.lastRenderTimeOnScreen=Value<float>(component,0x4b0);
        // Preserve raw pinned primitive pointer fields while their SceneProxy
        // role is being matched against native renderer construction. No
        // dereference or false SceneProxy-ready assertion is made here.
        out.rawPrimitive3a0=Value<void*>(component,0x3a0);out.rawPrimitive3a8=Value<void*>(component,0x3a8);out.rawPrimitive6b0=Value<void*>(component,0x6b0);
        out.rawPrimitive690=Value<void*>(component,0x690);
    }
    if(out.skinned){
        out.skeletalMesh=Value<void*>(component,0x708);out.skinnedAsset=Value<void*>(component,0x710);
        out.leaderObjectIndex=Value<int32_t>(component,0x718);out.leaderSerial=Value<int32_t>(component,0x71c);out.leader=ResolveLeader(component);
        out.visibilityAnimTickOption=Value<uint8_t>(component,0x92c);out.skinnedFlags=Value<uint8_t>(component,0x92f);out.recentRender=(out.skinnedFlags&0x20)!=0;
        out.poseBufferIndex=Value<int32_t>(component,0x7a8);
        if(out.poseBufferIndex>=0&&out.poseBufferIndex<=1){
            const size_t at=0x760+static_cast<size_t>(out.poseBufferIndex)*16;
            out.poseData=Value<void*>(component,at);out.poseCount=Value<int32_t>(component,at+8);out.poseCapacity=Value<int32_t>(component,at+12);
        }
        out.boneMapCount=Value<int32_t>(component,0x7d0);ReadLeaderChain(out);
    }
    if(out.skeletal){
        out.animClass=Value<void*>(component,0xa68);out.animInstance=Value<void*>(component,0xa70);
        out.globalAnimRate=Value<float>(component,0xbd8);out.animationMode=Value<uint8_t>(component,0xbdf);
        out.pauseAnims=(Value<uint8_t>(component,0xbe9)&0x20)!=0;out.skelFlags=Value<uint8_t>(component,0xbea);
        out.onlyAutonomousPose=(out.skelFlags&2)!=0;out.autonomousPose=(out.skelFlags&4)!=0;
        out.lastPoseTickFrame=Value<uint64_t>(component,0xd78);
    }
}
inline void AddActor(size_t rosterIndex,void* actor,const char* label,bool character){
    if(!actor||state.actorCount>=MaxActors)return;
    for(uint32_t i=0;i<state.actorCount;++i)if(state.actors[i].object==actor)return;
    auto& out=state.actors[state.actorCount++];out={};out.rosterIndex=rosterIndex;out.object=actor;out.label=label;
    out.flags=Value<uint8_t>(actor,0x65);out.hidden=(out.flags&2)!=0;out.locallyHidden=(out.flags&4)!=0;out.hiddenCombined=(out.flags&6)!=0;
    out.role=Value<uint8_t>(actor,0x224);out.owner=Value<void*>(actor,0x210);out.root=Value<void*>(actor,0x290);
    out.location=Value<engine::Vec>(out.root,0x320);out.scale=Value<engine::Vec>(out.root,0x340);
    AddComponent(rosterIndex,actor,out.root,"actor_root");
    if(character){
        out.controller=Value<void*>(actor,0x4a0);
        AddComponent(rosterIndex,actor,Value<void*>(actor,0x4f0),"character_core3p");
        AddComponent(rosterIndex,actor,Value<void*>(actor,0xf30),"character_core1p");
        AddComponent(rosterIndex,actor,Value<void*>(actor,0xf38),"character_cosmetic1p");
        AddComponent(rosterIndex,actor,Value<void*>(actor,0xf40),"character_cosmetic3p");
        AddComponent(rosterIndex,actor,Value<void*>(actor,0x12a8),"character_follow_camera");
    }else{
        AddComponent(rosterIndex,actor,Value<void*>(actor,0xde0),"equippable_mesh1p");
        AddComponent(rosterIndex,actor,Value<void*>(actor,0xdf0),"equippable_mesh3p");
    }
}
inline bool Initialize(void* world){
    if(!original_players::Thread()||!world)return Fail(1500,"original_render_context_missing");
    state={};state.world=world;state.bound=true;state.stage="original_render_readonly_bound";return true;
}
inline bool SampleRoster(){
    if(!state.bound||!original_players::Thread())return false;
    state.actorCount=state.componentCount=state.attachCycles=state.leaderCycles=0;
    state.worldTime=Value<double>(state.world,0x740);
    state.scene=Value<void*>(state.world,0x200);state.sceneVtable=Value<void*>(state.scene,0);
    // Native Primitive.CreateRenderState41ea860 calls this original scene's
    // first virtual with the primitive; DestroyRenderState41eb800 uses+8.
    state.addPrimitive=Value<void*>(state.sceneVtable,0);state.removePrimitive=Value<void*>(state.sceneVtable,8);
    SampleProjectionRouting();
    for(size_t i=0;i<original_players::MaxPlayers;++i){
        const auto& roster=original_players::players[i];if(!roster.ready||!roster.character)continue;
        AddActor(i,roster.character,i==static_cast<size_t>(original_players::localIndex)?"local_character":"enemy_character",true);
        auto inventory=Value<void*>(roster.character,0xc08);
        auto current=Value<void*>(inventory,0x280),desired=Value<void*>(inventory,0x268);
        if(current&&original_players::IsA(current,0x645c660))AddActor(i,current,"current_equippable",false);
        if(desired&&original_players::IsA(desired,0x645c660))AddActor(i,desired,"desired_equippable",false);
    }
    if(!CaptureViewSample())return false;
    ++state.samples;state.error=0;state.stage="original_render_readonly_sampled";return true;
}
inline void Pointer(FILE* f,const char* name,void* pointer){std::fprintf(f,",\"%s\":\"%p\"",name,pointer);}
inline void Vector(FILE* f,const char* name,const engine::Vec& v){std::fprintf(f,",\"%s\":",name);original_players::Vector(f,v);}
inline void Matrix(FILE* f,const char* name,const MatrixSnapshot& matrix){
    std::fprintf(f,",\"%s\":{\"readable\":%s,\"finite\":%s,\"sample_changes\":%llu,\"values\":[",name,matrix.readable?"true":"false",matrix.finite?"true":"false",static_cast<unsigned long long>(matrix.changes));
    for(size_t i=0;i<matrix.values.size();++i){if(i)std::fputc(',',f);if(matrix.readable&&std::isfinite(matrix.values[i]))std::fprintf(f,"%.17g",matrix.values[i]);else std::fputs("null",f);}
    std::fputs("]}",f);
}
inline void Numbers(FILE* f,const char* name,const double* values,size_t count){
    std::fprintf(f,",\"%s\":[",name);
    for(size_t i=0;i<count;++i){if(i)std::fputc(',',f);if(std::isfinite(values[i]))std::fprintf(f,"%.17g",values[i]);else std::fputs("null",f);}std::fputc(']',f);
}
inline void CachedVector(FILE* f,const char* name,const engine::Vec& vector){const double values[]{vector.x,vector.y,vector.z};Numbers(f,name,values,3);}
inline void Transform(FILE* f,const engine::Transform& transform){
    std::fputs("{\"quaternion_xyzw\":[",f);
    for(size_t i=0;i<4;++i){if(i)std::fputc(',',f);if(std::isfinite(transform.rotation[i]))std::fprintf(f,"%.17g",transform.rotation[i]);else std::fputs("null",f);}
    std::fputc(']',f);CachedVector(f,"location",transform.translation);CachedVector(f,"scale",transform.scale);std::fputc('}',f);
}
inline void Pose(FILE* f,const char* name,const CachedPose& pose){
    std::fprintf(f,",\"%s\":{\"readable\":%s,\"first_person_rendering\":%s,\"pose_frame\":%llu",name,pose.readable?"true":"false",pose.firstPersonRendering?"true":"false",static_cast<unsigned long long>(pose.poseFrame));
    Pointer(f,"object",pose.object);Pointer(f,"parent",pose.parent);Pointer(f,"owner",pose.owner);std::fputs(",\"world\":",f);Transform(f,pose.world);
    CachedVector(f,"relative_location",pose.relativeLocation);CachedVector(f,"relative_rotation_pitch_yaw_roll",pose.relativeRotation);std::fputc('}',f);
}
inline void View(FILE* f,const ViewSample& view){
    const auto b=[](bool x){return x?"true":"false";};
    std::fprintf(f,"{\"sequence\":%llu,\"tag\":%llu,\"wall_time_ms\":%llu,\"world_time\":%.9g,\"native_shot_time\":%.9g,\"world_controller_count\":%d,\"world_first_weak_index\":%d,\"world_first_weak_serial\":%d,\"controller_object_index\":%d,\"local_links\":%s,\"camera_owner_matches\":%s,\"first_world_index_matches\":%s,\"camera_cache_layout_verified\":%s,\"has_native_ray\":%s,\"ai_controlled\":%s,\"pawn_role\":%u",static_cast<unsigned long long>(view.sequence),static_cast<unsigned long long>(view.tag),static_cast<unsigned long long>(view.wallTimeMs),view.worldTime,view.shotTime,view.worldControllerCount,view.worldFirstWeakIndex,view.worldFirstWeakSerial,view.controllerObjectIndex,b(view.localLinks),b(view.cameraOwnerMatches),b(view.firstWorldIndexMatches),b(view.cameraCacheLayoutVerified),b(view.hasNativeRay),b(view.aiControlled),view.pawnRole);
    Pointer(f,"controller",view.controller);Pointer(f,"pawn",view.pawn);Pointer(f,"player",view.player);Pointer(f,"camera",view.camera);Pointer(f,"camera_owner",view.cameraOwner);Pointer(f,"view_target",view.viewTarget);Pointer(f,"input_state",view.inputState);Pointer(f,"weapon",view.weapon);
    CachedVector(f,"pc_control_rotation_pitch_yaw_roll",view.controlRotation);
    CachedVector(f,"input_aim_direction",view.inputAimDirection);CachedVector(f,"pawn_facing_control_rotation_pitch_yaw_roll",view.pawnFacingControlRotation);
    CachedVector(f,"camera_position",view.cameraPosition);CachedVector(f,"camera_rotation_pitch_yaw_roll",view.cameraRotation);
    CachedVector(f,"previous_camera_position",view.previousCameraPosition);CachedVector(f,"previous_camera_rotation_pitch_yaw_roll",view.previousCameraRotation);
    CachedVector(f,"native_ray_origin",view.nativeRayOrigin);CachedVector(f,"native_ray_direction",view.nativeRayDirection);
    std::fprintf(f,",\"camera_cache_time\":%.9g,\"previous_camera_cache_time\":%.9g,\"cached_fov\":%.9g,\"previous_cached_fov\":%.9g,\"default_fov\":%.9g,\"locked_fov\":%.9g,\"first_person_fov\":%.9g,\"muzzle_point_socket_fname_u32\":[%u,%u,%u],\"gun_pose_buffer\":%d,\"gun_pose_count\":%d,\"gun_pose_frame\":%llu,\"gun_pose_stable\":%s",view.cameraTime,view.previousCameraTime,view.cachedFov,view.previousCachedFov,view.defaultFov,view.lockedFov,view.firstPersonFov,view.muzzlePointSocketName[0],view.muzzlePointSocketName[1],view.muzzlePointSocketName[2],view.gunPoseBuffer,view.gunPoseCount,static_cast<unsigned long long>(view.gunPoseFrame),b(view.gunPoseStable));
    Pose(f,"pawn_root",view.pawnRoot);Pose(f,"follow_camera",view.follow);Pose(f,"core1p",view.core1p);Pose(f,"cosmetic1p",view.cosmetic1p);Pose(f,"gun1p",view.gun1p);Pose(f,"gun3p",view.gun3p);std::fputc('}',f);
}
inline void GunCache(FILE* f,const GunPoseCache& cache){
    std::fprintf(f,",\"latest_gun_component_space_cache\":{\"readable\":%s,\"stable_copy\":%s,\"truncated\":%s,\"buffer_index\":%d,\"count\":%d,\"capacity\":%d,\"leader_weak_index\":%d,\"leader_weak_serial\":%d,\"pose_frame\":%llu,\"captured\":%u,\"stride\":96,\"muzzle_socket_transform_evaluated\":false",cache.readable?"true":"false",cache.stable?"true":"false",cache.truncated?"true":"false",cache.bufferIndex,cache.count,cache.capacity,cache.leaderIndex,cache.leaderSerial,static_cast<unsigned long long>(cache.poseFrame),cache.captured);
    Pointer(f,"mesh",cache.mesh);Pointer(f,"asset",cache.asset);Pointer(f,"data",cache.data);std::fputs(",\"bones\":[",f);
    for(uint32_t i=0;i<cache.captured;++i){std::fprintf(f,"%s{\"bone_index\":%u,\"component_space\":",i?",":"",i);Transform(f,cache.transforms[i]);std::fputc('}',f);}std::fputs("]}",f);
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!state.bound||!path)return false;FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    const auto b=[](bool x){return x?"true":"false";};
    std::fprintf(f,"{\"schema_version\":2,\"stage\":\"%s\",\"error\":%u,\"samples\":%llu,\"world_time\":%.9g,\"actor_count\":%u,\"component_count\":%u,\"attach_cycles\":%u,\"leader_cycles\":%u,\"actors\":[",state.stage,state.error,static_cast<unsigned long long>(state.samples),state.worldTime,state.actorCount,state.componentCount,state.attachCycles,state.leaderCycles);
    for(uint32_t i=0;i<state.actorCount;++i){const auto& a=state.actors[i];std::fprintf(f,"%s{\"label\":\"%s\",\"roster_index\":%zu,\"hidden\":%s,\"locally_hidden\":%s,\"hidden_combined\":%s,\"flags\":%u,\"role\":%u",i?",":"",a.label,a.rosterIndex,b(a.hidden),b(a.locallyHidden),b(a.hiddenCombined),a.flags,a.role);Pointer(f,"object",a.object);Pointer(f,"root",a.root);Pointer(f,"owner",a.owner);Pointer(f,"controller",a.controller);Vector(f,"location",a.location);Vector(f,"scale",a.scale);std::fputc('}',f);}
    std::fputs("],\"components\":[",f);
    for(uint32_t i=0;i<state.componentCount;++i){const auto& c=state.components[i];std::fprintf(f,"%s{\"label\":\"%s\",\"roster_index\":%zu",i?",":"",c.label,c.rosterIndex);
        const char* pn[]{"object","actor","parent","parent_owner","skeletal_mesh","skinned_asset","anim_instance","anim_class","leader","pose_data","primitive_raw_3a0","primitive_raw_3a8","primitive_raw_6b0","primitive_raw_690"};void* const pv[]{c.object,c.actor,c.parent,c.parentOwner,c.skeletalMesh,c.skinnedAsset,c.animInstance,c.animClass,c.leader,c.poseData,c.rawPrimitive3a0,c.rawPrimitive3a8,c.rawPrimitive6b0,c.rawPrimitive690};for(size_t n=0;n<sizeof(pv)/sizeof(pv[0]);++n)Pointer(f,pn[n],pv[n]);
        const char* bn[]{"present","primitive","skinned","skeletal","registered","render_state_created","active","visible","hidden_in_game","render_in_main_pass","owner_no_see","only_owner_see","scene_capture_only","absolute_location","absolute_rotation","absolute_scale","recently_rendered","pause_anims","only_autonomous_pose","autonomous_pose","attach_cycle","attach_truncated","leader_cycle","leader_truncated"};const bool bv[]{c.present,c.primitive,c.skinned,c.skeletal,c.registered,c.renderStateCreated,c.active,c.visible,c.hiddenInGame,c.renderInMainPass,c.ownerNoSee,c.onlyOwnerSee,c.sceneCaptureOnly,c.absoluteLocation,c.absoluteRotation,c.absoluteScale,c.recentRender,c.pauseAnims,c.onlyAutonomousPose,c.autonomousPose,c.attachCycle,c.attachTruncated,c.leaderCycle,c.leaderTruncated};for(size_t n=0;n<sizeof(bv)/sizeof(bv[0]);++n)std::fprintf(f,",\"%s\":%s",bn[n],b(bv[n]));
        std::fprintf(f,",\"component_flags\":%u,\"scene_flags\":%u,\"primitive_flags1\":%u,\"primitive_flags2\":%u,\"visibility_anim_tick_option\":%u,\"animation_mode\":%u,\"skinned_flags\":%u,\"skel_flags\":%u,\"attach_children\":%d,\"attach_capacity\":%d,\"leader_object_index\":%d,\"leader_serial\":%d,\"pose_buffer_index\":%d,\"pose_count\":%d,\"pose_capacity\":%d,\"bone_map_count\":%d,\"last_pose_tick_frame\":%llu,\"global_anim_rate\":%.9g,\"last_render_time\":%.9g,\"last_render_time_on_screen\":%.9g,\"bounds_radius\":%.9g",c.componentFlags,c.sceneFlags,c.primitiveFlags1,c.primitiveFlags2,c.visibilityAnimTickOption,c.animationMode,c.skinnedFlags,c.skelFlags,c.attachChildren,c.attachCapacity,c.leaderObjectIndex,c.leaderSerial,c.poseBufferIndex,c.poseCount,c.poseCapacity,c.boneMapCount,static_cast<unsigned long long>(c.lastPoseTickFrame),c.globalAnimRate,c.lastRenderTime,c.lastRenderTimeOnScreen,c.boundsRadius);
        Vector(f,"location",c.location);Vector(f,"scale",c.scale);Vector(f,"relative_location",c.relativeLocation);Vector(f,"relative_scale",c.relativeScale);Vector(f,"bounds_origin",c.boundsOrigin);Vector(f,"bounds_extent",c.boundsExtent);
        Numbers(f,"cached_world_quaternion_xyzw",c.worldQuaternion.data(),4);CachedVector(f,"relative_rotation_pitch_yaw_roll",c.relativeRotation);std::fprintf(f,",\"first_person_rendering\":%s",b(c.firstPersonRendering));
        std::fprintf(f,",\"attach_depth\":%u,\"leader_depth\":%u,\"attach_chain\":[",c.attachDepth,c.leaderDepth);for(uint32_t n=0;n<c.attachDepth;++n)std::fprintf(f,"%s\"%p\"",n?",":"",c.attachChain[n]);std::fputs("],\"leader_chain\":[",f);for(uint32_t n=0;n<c.leaderDepth;++n)std::fprintf(f,"%s\"%p\"",n?",":"",c.leaderChain[n]);std::fputs("]}",f);
    }
    std::fputc(']',f);Pointer(f,"scene",state.scene);Pointer(f,"scene_vtable",state.sceneVtable);Pointer(f,"scene_add_primitive",state.addPrimitive);Pointer(f,"scene_remove_primitive",state.removePrimitive);
    std::fprintf(f,",\"scene_add_primitive_rva\":%llu,\"scene_remove_primitive_rva\":%llu",static_cast<unsigned long long>(state.addPrimitive?reinterpret_cast<uintptr_t>(state.addPrimitive)-engine::image:0),static_cast<unsigned long long>(state.removePrimitive?reinterpret_cast<uintptr_t>(state.removePrimitive)-engine::image:0));
    std::fprintf(f,",\"projection_routing\":{\"world_controller_count\":%d,\"world_first_object_index\":%d,\"world_first_object_serial\":%d,\"world_first_is_active\":%s,\"active_player_is_local_player\":%s,\"local_player_routes_active\":%s,\"shooter_camera\":%s,\"scene_projection_cache_offsets_verified\":%s",state.worldControllerCount,state.worldFirstObjectIndex,state.worldFirstObjectSerial,b(state.worldFirstIsActive),b(state.activePlayerIsLocalPlayer),b(state.localPlayerRoutesActive),b(state.shooterCamera),b(state.sceneProjectionCacheVerified));
    const char* routingNames[]{"world_controller_array","world_first_controller","world_first_player","active_controller","active_player","active_local_player","local_player_controller","camera","camera_owner","scene_projection_update"};void* const routingPointers[]{state.worldControllerArray,state.worldFirstController,state.worldFirstPlayer,state.activeController,state.activePlayer,state.activeLocalPlayer,state.localPlayerController,state.camera,state.cameraOwner,state.sceneProjectionUpdate};for(size_t n=0;n<sizeof(routingPointers)/sizeof(routingPointers[0]);++n)Pointer(f,routingNames[n],routingPointers[n]);
    std::fprintf(f,",\"scene_projection_update_rva\":%llu,\"camera_correction_offset\":1168,\"scene_world_projection_offset\":15328,\"scene_first_person_projection_offset\":15456,\"scene_cache_may_update_asynchronously\":true",static_cast<unsigned long long>(state.sceneProjectionUpdate?reinterpret_cast<uintptr_t>(state.sceneProjectionUpdate)-engine::image:0));
    Matrix(f,"camera_projection_correction",state.cameraProjectionCorrection);Matrix(f,"scene_world_projection",state.sceneWorldProjection);Matrix(f,"scene_first_person_projection",state.sceneFirstPersonProjection);std::fputc('}',f);
    std::fprintf(f,",\"view_sample_count\":%llu,\"view_history_count\":%zu,\"latest_view\":",static_cast<unsigned long long>(state.viewSamples),state.viewHistoryCount);View(f,state.latestView);std::fputs(",\"view_history\":[",f);
    const size_t firstView=(state.nextViewSample+MaxViewSamples-state.viewHistoryCount)%MaxViewSamples;
    for(size_t n=0;n<state.viewHistoryCount;++n){if(n)std::fputc(',',f);View(f,state.viewHistory[(firstView+n)%MaxViewSamples]);}std::fputc(']',f);GunCache(f,state.latestGunPose);
    std::fputs(",\"readonly\":true,\"cached_bounds_offsets_verified\":true,\"native_transform_getters_called\":false,\"scene_proxy_identity_verified\":false,\"runtime_pixels_verified\":false}\n",f);
    const bool ok=std::ferror(f)==0;std::fclose(f);return ok;
}
}
