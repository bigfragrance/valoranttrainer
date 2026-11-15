#pragma once
#include "original_camera.h"
#include "original_inventory_slots.h"
#include "original_jett_priming.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>

// Read-only native client observations. Never possess an NPC, change a role,
// write a camera transform, or run an additional camera update from here.
namespace local_server_spectator_view {
using Weak=original_inventory_slots::WeakIdentity;
struct AmmoView {
    Weak identity{};
    int32_t amount=0,maximum=0,authority=0;
    bool known=false;
};
struct View {
    bool enabled=false,active=false,ownViewConfirmed=false,waitingForRespawn=false;
    bool nativeSpectatingKnown=false,nativeSpectating=false,localOwnerVerified=false;
    bool shortcutsKnown=false,shortcutsAllowed=false;
    bool targetCurrentLife=false,targetAliveKnown=false,targetAlive=false;
    bool cameraTargetConfirmed=false,poseQueriesKnown=false,poseWithinTolerance=false;
    bool eyesQueryKnown=false,locationQueryKnown=false,rotationQueryKnown=false,fovQueryKnown=false;
    bool pawnFovKnown=false,zoomManagerKnown=false,zoomOwnerMatches=false;
    uint8_t mode=255;int32_t nativePlayerId=-1;
    uint64_t sampledAtMs=0,samples=0,targetGeneration=0;
    void *controller=nullptr,*ownPawn=nullptr,*targetPawn=nullptr,*targetState=nullptr;
    void *logicalTarget=nullptr,*localTarget=nullptr,*cameraTarget=nullptr,*pendingTarget=nullptr;
    Weak targetIdentity{},targetStateIdentity{};
    engine::Vec cameraPosition{},cameraRotation{},targetEyes{},targetRotation{};
    float fov=0,defaultFov=0,lockedFov=0,followFov=0,customFov=0;
    float pawnFov=0,povFov=0,firstPersonFov=0;
    double povTime=0;uint8_t cameraState=255;
    Weak weaponIdentity{},zoomManagerIdentity{},zoomIdentity{};
    Weak zoomClassIdentity{};
    uint8_t weaponRole=255,zoomState=255;
    float zoomFov=0,zoomFirstPersonFov=0;
    int32_t authZoomTarget=-1,predictedZoomTarget=-1;
    char zoomTransitionBytes[145]{};
    bool weaponOwnerMatches=false,reserveQueryKnown=false;
    AmmoView magazine{},reserve{};
    bool followOverridesFov=false,customOverridesFov=false;
    double eyeDistance=0,rotationDifference=0;
    const char* stage="native_spectator_not_sampled";
};
inline View state{},published{};
inline SRWLOCK publishedLock=SRWLOCK_INIT;
inline void PublishSnapshot(){AcquireSRWLockExclusive(&publishedLock);published=state;ReleaseSRWLockExclusive(&publishedLock);}
inline View Snapshot(){AcquireSRWLockShared(&publishedLock);const auto value=published;ReleaseSRWLockShared(&publishedLock);return value;}
inline Weak previousTarget{};
inline const View& GetState(){return state;}
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
inline bool Same(const Weak& a,const Weak& b){return a.index==b.index&&a.serial==b.serial;}
inline bool Identity(void* object,Weak& id){
    return object&&original_inventory_slots::CaptureIdentity(object,id)
        &&original_inventory_slots::ResolveIdentity(id)==object;
}
inline bool Event(void* object,const wchar_t* path,uint16_t bytes,void* params,uintptr_t exec=0){
    auto method=engine::Find(path);
    return object&&method&&Read<uint16_t>(method,0xbe)==bytes
        &&original_players::Derived(Read<void*>(object,0x10),Read<void*>(method,0x28))
        &&(!exec||Read<uintptr_t>(method,0xe0)==engine::image+exec)
        &&engine::Event(object,path,params);
}
inline bool Finite(const engine::Vec& value){return original_players::Finite(value);}
inline double AngleDifference(double a,double b){
    double value=std::fmod(a-b,360.);if(value>180.)value-=360.;if(value< -180.)value+=360.;return std::abs(value);
}
inline bool PawnFovProfile(){
    static uintptr_t checkedImage=0;static bool valid=false;
    if(checkedImage==engine::image&&checkedImage)return valid;
    checkedImage=engine::image;valid=false;
    const auto data=reinterpret_cast<const uint8_t*>(engine::image+0x7880050);
    if(!original_players::Readable(data,120))return false;
    uint64_t fnv=14695981039346656037ULL;
    for(size_t i=0;i<120;++i)fnv=(fnv^data[i])*1099511628211ULL;
    valid=fnv==0xb2fa49567e1d26ddULL;return valid;
}
// Read the exact viewed weapon. The general client weapon summary belongs
// to the local human pawn and cannot establish an observed NPC's ammo state.
inline bool SampleAmmo(void* weapon,void* component,AmmoView& out){
    if(!original_players::IsA(component,0x63c7e00)||!Identity(component,out.identity)
       ||Read<void*>(component,0xc8)!=weapon)return false;
    original_jett_priming::PropertyInfo authority{};
    if(original_jett_priming::FieldOffset(Read<void*>(component,0x10),L"AuthResourceAmount",4,&authority)!=0x110
       ||authority.arrayDim!=1||authority.elementSize!=4
       ||(Read<uint64_t>(authority.field,0x38)&0x100000020ULL)!=0x100000020ULL)return false;
    out.authority=Read<int32_t>(component,0x110);
    out.known=Event(component,L"/Script/ShooterGame.AmmoComponent.GetAmmo",4,&out.amount,0x637fea0)
        &&Event(component,L"/Script/ShooterGame.AmmoComponent.GetMaxAmmo",4,&out.maximum,0x63c84c0)
        &&out.amount>=0&&out.maximum>=0&&out.amount<=out.maximum
        &&original_inventory_slots::ResolveIdentity(out.identity)==component;
    return out.known;
}
inline void SampleWeaponAmmo(void* target,void* inventory,void* weapon,View& out){
    out.weaponOwnerMatches=original_players::IsA(inventory,0x6499330)&&Read<void*>(inventory,0xc8)==target
        &&original_players::IsA(weapon,0x645c660)&&Read<void*>(weapon,0x210)==target&&Read<void*>(weapon,0x4a8)==target;
    if(!out.weaponOwnerMatches)return;
    struct ComponentQuery {void* type=nullptr;void* result=nullptr;} ammo{engine::Class(0x63c7e00),nullptr};
    if(!Event(weapon,L"/Script/Engine.Actor.GetComponentByClass",16,&ammo)
       ||!SampleAmmo(weapon,ammo.result,out.magazine))return;
    void* reserve=nullptr;
    out.reserveQueryKnown=Event(ammo.result,L"/Script/ShooterGame.AmmoComponent.GetReserveComponent",8,&reserve,0x37e4230)
        &&reserve!=ammo.result;
    if(out.reserveQueryKnown&&reserve)SampleAmmo(weapon,reserve,out.reserve);
}
inline bool SamplePose(void* pc,void* target,View& out){
    const auto camera=Read<void*>(pc,0x528);
    struct Eyes {engine::Vec position{},rotation{};} eyes{};
    static_assert(sizeof(Eyes)==48);
    if(!original_players::IsA(camera,original_camera::CameraClass))return false;
    out.eyesQueryKnown=Event(target,L"/Script/Engine.Actor.GetActorEyesViewPoint",48,&eyes);
    out.locationQueryKnown=Event(camera,L"/Script/Engine.PlayerCameraManager.GetCameraLocation",24,&out.cameraPosition);
    out.rotationQueryKnown=Event(camera,L"/Script/Engine.PlayerCameraManager.GetCameraRotation",24,&out.cameraRotation);
    out.fovQueryKnown=Event(camera,L"/Script/Engine.PlayerCameraManager.GetFOVAngle",4,&out.fov);
    out.defaultFov=Read<float>(camera,0x484);out.lockedFov=Read<float>(camera,0x48c);
    out.firstPersonFov=Read<float>(camera,0x488);out.povTime=Read<float>(camera,0x17b0);out.povFov=Read<float>(camera,0x17f0);
    out.cameraState=Read<uint8_t>(target,0x12a0);
    if(PawnFovProfile()){out.pawnFov=engine::function<float(*)(void*)>(0x7880050)(target);out.pawnFovKnown=std::isfinite(out.pawnFov);}
    const auto inventory=Read<void*>(target,0xc08),weapon=Read<void*>(inventory,0x280);
    if(Identity(weapon,out.weaponIdentity)){
        out.weaponRole=Read<uint8_t>(weapon,0x224);
        SampleWeaponAmmo(target,inventory,weapon,out);
        struct ComponentQuery{void* type=nullptr;void* result=nullptr;} zoom{engine::Class(0x6e06bb0),nullptr};
        if(Event(weapon,L"/Script/Engine.Actor.GetComponentByClass",16,&zoom)&&Identity(zoom.result,out.zoomManagerIdentity)){
            out.zoomManagerKnown=true;out.zoomOwnerMatches=Read<void*>(zoom.result,0x100)==target;
            const auto component=Read<void*>(zoom.result,0xe0);
            if(Identity(component,out.zoomIdentity)){
                Identity(Read<void*>(component,0x10),out.zoomClassIdentity);
                out.zoomState=Read<uint8_t>(component,0x4fc);out.zoomFov=Read<float>(component,0x4e8);
                out.zoomFirstPersonFov=Read<float>(component,0x4ec);out.authZoomTarget=Read<int32_t>(component,0x4cc);
                out.predictedZoomTarget=Read<int32_t>(component,0x4f0);
                constexpr char hex[]="0123456789abcdef";
                for(size_t i=0;i<72;++i){const auto byte=Read<uint8_t>(component,0x4b8+i);out.zoomTransitionBytes[i*2]=hex[byte>>4];out.zoomTransitionBytes[i*2+1]=hex[byte&15];}
            }
        }
    }
    const auto follow=Read<void*>(target,0x12a8),custom=Read<void*>(target,0x12b0);
    out.followFov=Read<float>(follow,0x364);out.followOverridesFov=Read<uint8_t>(follow,0x360)!=0;
    out.customFov=Read<float>(custom,0x364);out.customOverridesFov=Read<uint8_t>(custom,0x360)!=0;
    out.targetEyes=eyes.position;out.targetRotation=eyes.rotation;
    if(!out.eyesQueryKnown||!out.locationQueryKnown||!out.rotationQueryKnown
       ||!Finite(eyes.position)||!Finite(eyes.rotation)||!Finite(out.cameraPosition)||!Finite(out.cameraRotation))return false;
    const auto x=out.cameraPosition.x-eyes.position.x,y=out.cameraPosition.y-eyes.position.y,z=out.cameraPosition.z-eyes.position.z;
    out.eyeDistance=std::sqrt(x*x+y*y+z*z);
    out.rotationDifference=(std::max)(AngleDifference(out.cameraRotation.x,eyes.rotation.x),AngleDifference(out.cameraRotation.y,eyes.rotation.y));
    // Audit tolerances only; recoil/shake/interpolation may produce valid angular differences.
    // Native target/current-life/origin gates, not angle equality, confirm the viewed character.
    out.poseWithinTolerance=out.eyeDistance<=75.&&out.rotationDifference<=15.;
    return out.fovQueryKnown&&std::isfinite(out.fov)&&out.fov>5&&out.fov<150;
}
inline void ObserveImpl(void* world,void* pc,void* ownPawn){
    View value{};value.samples=state.samples+1;value.targetGeneration=state.targetGeneration;
    value.sampledAtMs=GetTickCount64();value.controller=pc;value.ownPawn=ownPawn;
    const auto save=[&](const char* reason){value.stage=reason;state=value;PublishSnapshot();};
    if(!original_players::Thread()||!world||!pc){save("native_spectator_waiting_local_context");return;}
    const auto ownPS=Read<void*>(pc,0x470),localPlayer=Read<void*>(pc,0x510),camera=Read<void*>(pc,0x528),component=Read<void*>(pc,0xc60);
    Weak owner{},local{},componentIdentity{};
    const auto driver=Read<void*>(world,0x40),connection=Read<void*>(driver,0xc0);
    value.localOwnerVerified=Identity(pc,owner)&&Identity(localPlayer,local)
        &&original_players::IsA(pc,original_players::ShooterControllerClass)
        &&original_players::IsA(localPlayer,0x6cb4740)
        &&Read<void*>(localPlayer,0x38)==pc&&Read<uint8_t>(pc,0x224)==2
        &&original_players::IsA(ownPS,original_players::ShooterStateClass)&&Read<void*>(ownPS,0x210)==pc
        &&driver&&connection&&Read<void*>(driver,0x178)==world
        &&Read<void*>(connection,0x38)==pc&&Read<void*>(connection,0xa0)==pc
        &&original_players::IsA(camera,original_camera::CameraClass)&&Read<void*>(camera,0x468)==pc
        &&Identity(component,componentIdentity)&&original_players::IsA(component,original_camera::ViewTargetClass)
        &&Read<void*>(component,0xc8)==pc;
    if(!value.localOwnerVerified){save("native_spectator_waiting_verified_local_owner");return;}
    value.mode=Read<uint8_t>(component,0x178);
    uint8_t native=0;
    value.nativeSpectatingKnown=Event(pc,L"/Script/ShooterGame.AresPlayerController.InSpectatorMode",1,&native,0x64eabe0);
    value.nativeSpectating=native!=0;
    value.enabled=value.mode==4&&value.nativeSpectatingKnown&&value.nativeSpectating;
    value.logicalTarget=Read<void*>(component,0x170);value.localTarget=Read<void*>(component,0x180);
    value.cameraTarget=Read<void*>(camera,0x580);value.pendingTarget=Read<void*>(camera,0xe80);
    if(!value.enabled){
        Weak own{};
        const bool currentOwn=Identity(ownPawn,own)&&original_players::IsA(ownPawn,original_players::ShooterCharacterClass)
            &&Read<void*>(pc,0x4b0)==ownPawn&&Read<void*>(ownPawn,0x4a0)==pc
            &&Read<void*>(ownPawn,0x488)==ownPS&&Read<uint8_t>(ownPawn,0x224)==2;
        value.poseQueriesKnown=currentOwn&&SamplePose(pc,ownPawn,value);
        value.ownViewConfirmed=value.mode==1&&value.nativeSpectatingKnown&&!value.nativeSpectating
            &&value.logicalTarget==ownPawn&&value.localTarget==ownPawn&&value.cameraTarget==ownPawn&&!value.pendingTarget
            &&value.poseQueriesKnown&&value.eyeDistance<=75.;
        if(value.ownViewConfirmed)previousTarget={};
        save(value.ownViewConfirmed?"native_spectator_own_view_confirmed":"native_spectator_waiting_native_mode_or_self_view");return;
    }
    uint8_t shortcuts=0;
    const auto specific=Read<void*>(pc,0xc78);Weak specificIdentity{};
    value.shortcutsKnown=Identity(specific,specificIdentity)&&original_players::IsA(specific,0x6d103d0)
        &&Read<void*>(specific,0xc8)==pc&&Event(pc,L"/Script/ShooterGame.AresPlayerController.CanTriggerSpectateShortcuts",1,&shortcuts,0x64e9350);
    value.shortcutsAllowed=shortcuts!=0;
    void* watched=nullptr;
    if(!Event(pc,L"/Script/ShooterGame.AresPlayerController.GetSpectatedSpawnedCharacter",8,&watched,0x64eaa00)
       ||!Identity(watched,value.targetIdentity)||!original_players::IsA(watched,original_players::ShooterCharacterClass)){
        value.waitingForRespawn=true;save("native_spectator_waiting_original_spectated_character");return;
    }
    value.targetPawn=watched;value.targetState=Read<void*>(watched,0x488);
    if(!Identity(value.targetState,value.targetStateIdentity)
       ||!original_players::IsA(value.targetState,original_players::ShooterStateClass)
       ||value.targetState==ownPS){save("native_spectator_waiting_remote_player_state");return;}
    void* spawned=nullptr;
    value.targetCurrentLife=Event(value.targetState,L"/Script/ShooterGame.ShooterPlayerState.GetSpawnedCharacter",8,&spawned)
        &&spawned==watched;
    if(!Event(value.targetState,L"/Script/Engine.PlayerState.GetPlayerId",4,&value.nativePlayerId,0x49452a0)
       ||value.nativePlayerId<=0){save("native_spectator_waiting_native_player_identity");return;}
    uint8_t alive=0;value.targetAliveKnown=Event(watched,L"/Script/ShooterGame.ShooterCharacter.IsAlive",1,&alive);
    value.targetAlive=alive!=0;value.waitingForRespawn=!value.targetCurrentLife||(value.targetAliveKnown&&!value.targetAlive);
    if(!Same(previousTarget,value.targetIdentity)){previousTarget=value.targetIdentity;++value.targetGeneration;}
    value.cameraTargetConfirmed=value.logicalTarget==watched&&value.localTarget==watched&&value.cameraTarget==watched&&!value.pendingTarget;
    value.poseQueriesKnown=SamplePose(pc,watched,value);
    value.active=value.targetCurrentLife&&value.targetAliveKnown&&value.targetAlive&&value.cameraTargetConfirmed
        &&value.poseQueriesKnown&&value.eyeDistance<=75.;
    save(value.active?"native_spectator_active":value.waitingForRespawn?"native_spectator_waiting_respawn"
        :!value.cameraTargetConfirmed?"native_spectator_waiting_native_camera_target":"native_spectator_waiting_native_first_person_pose");
}
inline void Observe(void* world,void* pc,void* ownPawn){
    __try{ObserveImpl(world,pc,ownPawn);}__except(EXCEPTION_EXECUTE_HANDLER){
        View failed{};failed.samples=state.samples+1;failed.targetGeneration=state.targetGeneration;
        failed.sampledAtMs=GetTickCount64();failed.stage="native_spectator_observation_unavailable";state=failed;PublishSnapshot();
    }
}
inline void MaybeObserve(void* world,void* pc,void* ownPawn){
    const auto now=GetTickCount64();
    const auto component=Read<void*>(pc,0xc60),camera=Read<void*>(pc,0x528);
    const bool targetChanged=state.mode!=Read<uint8_t>(component,0x178)
        ||state.logicalTarget!=Read<void*>(component,0x170)||state.localTarget!=Read<void*>(component,0x180)
        ||state.cameraTarget!=Read<void*>(camera,0x580);
    if(!state.sampledAtMs||state.controller!=pc||targetChanged||now-state.sampledAtMs>=100)Observe(world,pc,ownPawn);
}
inline bool Fresh(const View& value){return value.sampledAtMs&&GetTickCount64()-value.sampledAtMs<=500;}
inline bool Fresh(){return Fresh(state);}
inline void* ExpectedCharacter(){return Fresh()&&state.enabled&&state.targetCurrentLife?state.targetPawn:nullptr;}
inline void* ExpectedPlayerState(){return ExpectedCharacter()?state.targetState:nullptr;}
inline std::string Json(){
    const auto value=Snapshot();char text[2400]{};const auto b=[](bool v){return v?"true":"false";};const auto finite=[](double v){return std::isfinite(v)?v:0.;};const bool fresh=Fresh(value);
    std::snprintf(text,sizeof(text),"{\"enabled\":%s,\"active\":%s,\"own_view_confirmed\":%s,\"waiting_for_respawn\":%s,\"stage\":\"%s\",\"native_player_id\":%d,\"native_mode\":%u,\"native_spectating_known\":%s,\"native_spectating\":%s,\"native_shortcuts_known\":%s,\"native_shortcuts_allowed\":%s,\"local_owner_verified\":%s,\"target_current_life_confirmed\":%s,\"camera_target_confirmed\":%s,\"pose_queries_known\":%s,\"pose_within_tolerance\":%s,\"rotation_scope\":\"client_target_eyes_only_not_authority_aim_proof\",\"eye_distance_cm\":%.9g,\"rotation_difference_deg\":%.9g,\"fov\":%.9g,\"camera_position\":[%.9g,%.9g,%.9g],\"camera_rotation\":[%.9g,%.9g,%.9g],\"target_eyes\":[%.9g,%.9g,%.9g],\"target_rotation\":[%.9g,%.9g,%.9g],\"target_generation\":%llu,\"target_pawn_weak\":[%d,%d],\"target_state_weak\":[%d,%d],\"sampled_at_ms\":%llu,\"sample_fresh\":%s,\"camera_calls\":0,\"possession_changes\":0}",
        b(fresh&&value.enabled),b(fresh&&value.active),b(fresh&&value.ownViewConfirmed),b(fresh&&value.waitingForRespawn),fresh?value.stage:"native_spectator_sample_stale",
        value.nativePlayerId,unsigned(value.mode),b(value.nativeSpectatingKnown),b(value.nativeSpectating),b(value.shortcutsKnown),b(value.shortcutsAllowed),b(value.localOwnerVerified),b(value.targetCurrentLife),b(value.cameraTargetConfirmed),b(value.poseQueriesKnown),b(value.poseWithinTolerance),
        finite(value.eyeDistance),finite(value.rotationDifference),finite(double(value.fov)),
        finite(value.cameraPosition.x),finite(value.cameraPosition.y),finite(value.cameraPosition.z),finite(value.cameraRotation.x),finite(value.cameraRotation.y),finite(value.cameraRotation.z),
        finite(value.targetEyes.x),finite(value.targetEyes.y),finite(value.targetEyes.z),finite(value.targetRotation.x),finite(value.targetRotation.y),finite(value.targetRotation.z),static_cast<unsigned long long>(value.targetGeneration),value.targetIdentity.index,value.targetIdentity.serial,value.targetStateIdentity.index,value.targetStateIdentity.serial,
        static_cast<unsigned long long>(value.sampledAtMs),b(fresh));
    std::string result=text;if(!result.empty()&&result.back()=='}')result.pop_back();
    char extra[600]{};std::snprintf(extra,sizeof(extra),",\"eyes_query_known\":%s,\"location_query_known\":%s,\"rotation_query_known\":%s,\"fov_query_known\":%s,\"default_fov\":%.9g,\"locked_fov\":%.9g,\"follow_fov\":%.9g,\"follow_overrides_fov\":%s,\"custom_fov\":%.9g,\"custom_overrides_fov\":%s}",
        b(value.eyesQueryKnown),b(value.locationQueryKnown),b(value.rotationQueryKnown),b(value.fovQueryKnown),finite(value.defaultFov),finite(value.lockedFov),finite(value.followFov),b(value.followOverridesFov),finite(value.customFov),b(value.customOverridesFov));
    result+=extra;if(!result.empty()&&result.back()=='}')result.pop_back();
    char zoom[650]{};std::snprintf(zoom,sizeof(zoom),",\"pawn_fov_known\":%s,\"pawn_fov\":%.9g,\"pov_time\":%.9g,\"pov_fov\":%.9g,\"first_person_fov\":%.9g,\"target_camera_state\":%u,\"weapon_weak\":[%d,%d],\"zoom_manager_known\":%s,\"zoom_manager_weak\":[%d,%d],\"zoom_weak\":[%d,%d],\"zoom_owner_matches_target\":%s}",
        b(value.pawnFovKnown),finite(value.pawnFov),finite(value.povTime),finite(value.povFov),finite(value.firstPersonFov),unsigned(value.cameraState),
        value.weaponIdentity.index,value.weaponIdentity.serial,b(value.zoomManagerKnown),value.zoomManagerIdentity.index,value.zoomManagerIdentity.serial,
        value.zoomIdentity.index,value.zoomIdentity.serial,b(value.zoomOwnerMatches));
    result+=zoom;if(!result.empty()&&result.back()=='}')result.pop_back();
    char transition[650]{};std::snprintf(transition,sizeof(transition),",\"weapon_role\":%u,\"zoom_class_weak\":[%d,%d],\"zoom_state\":%u,\"zoom_fov\":%.9g,\"zoom_first_person_fov\":%.9g,\"auth_zoom_target\":%d,\"predicted_zoom_target\":%d,\"zoom_transition_raw_4b8\":\"%s\"}",
        unsigned(value.weaponRole),value.zoomClassIdentity.index,value.zoomClassIdentity.serial,unsigned(value.zoomState),finite(value.zoomFov),finite(value.zoomFirstPersonFov),value.authZoomTarget,value.predictedZoomTarget,value.zoomTransitionBytes);
    result+=transition;if(!result.empty()&&result.back()=='}')result.pop_back();
    char ammo[700]{};std::snprintf(ammo,sizeof(ammo),",\"weapon_owner_matches_target\":%s,\"watched_magazine\":{\"known\":%s,\"weak\":[%d,%d],\"amount\":%d,\"maximum\":%d,\"authority\":%d},\"watched_reserve_query_known\":%s,\"watched_reserve\":{\"known\":%s,\"weak\":[%d,%d],\"amount\":%d,\"maximum\":%d,\"authority\":%d}}",
        b(value.weaponOwnerMatches),b(value.magazine.known),value.magazine.identity.index,value.magazine.identity.serial,
        value.magazine.amount,value.magazine.maximum,value.magazine.authority,b(value.reserveQueryKnown),b(value.reserve.known),
        value.reserve.identity.index,value.reserve.identity.serial,value.reserve.amount,value.reserve.maximum,value.reserve.authority);
    result+=ammo;return result;
}
}
