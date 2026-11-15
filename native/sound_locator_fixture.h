#pragma once
#include "audio_feedback.h"
#include "original_audio_listeners.h"
#include "original_hud.h"
#include "training_control.h"
#include "training_world_geometry.h"
#include <cstdio>
#include <cmath>

// Explicit stage9-only acoustic fixture. No normal play polling or hooks.
// The original engine handles spatialization and event/media playback.
namespace sound_locator_fixture {
struct Request { uint32_t operation=0,event=0;float angle=0,distance=1000,height=0,yaw=0; };
static_assert(sizeof(Request)==24);
struct ExtendedRequest {Request request;engine::Vec position{};uint32_t sourceId=0,flags=0;};
static_assert(sizeof(ExtendedRequest)==56);
inline engine::Vec previousFeet[64]{};inline bool previousValid[64]{};
inline bool savedSetting=false,originalHrtf=false;
inline uint64_t sequence=0;
inline constexpr const wchar_t* Events[]={
 L"/Game/WwiseAudio/Events/SFX/Weapons/Events_Guns_RE/Ak47_RE/Play_Wp_AK47_Reload_1_MagOut_3P.Play_Wp_AK47_Reload_1_MagOut_3P",
 L"/Game/WwiseAudio/Events/SFX/Weapons/Events_Guns_RE/Ak47_RE/Play_Wp_AK47_Reload_4_MagIn_End_3P.Play_Wp_AK47_Reload_4_MagIn_End_3P",
 L"/Game/WwiseAudio/Events/SFX/Core/Events_FS_Final_Test/HardSoleBoot/Play_FS_HardSoleBoot_Run.Play_FS_HardSoleBoot_Run",
 L"/Game/WwiseAudio/Events/SFX/Core/Events_FS_Final_Test/QuietSneaker/Play_FS_QuietSneaker_Run.Play_FS_QuietSneaker_Run",
 L"/Game/WwiseAudio/Events/SFX/Core/Events_FS_Final_Test/HardSoleBoot/Play_FS_HardSoleBoot_Run.Play_FS_HardSoleBoot_Run"
};
inline constexpr const wchar_t* WushuStep=L"/Game/WwiseAudio/Events/SFX/Characters/Events_Char_Wushu_SFX/Movement/Play_FS_Mvt_Wushu_Run.Play_FS_Mvt_Wushu_Run";
struct String {const wchar_t* data=nullptr;int32_t count=0,capacity=0;};
struct GroundParameters {
 void* character=nullptr;void** events=nullptr;int32_t eventCount=0,eventCapacity=0;
 engine::Vec location{},rotation{};String perspective{};uint8_t surface=1,pad[3]{};
 float volume=1;void* reuse=nullptr;uint8_t result=0,tail[7]{};
};
static_assert(sizeof(GroundParameters)==112&&offsetof(GroundParameters,location)==24
 &&offsetof(GroundParameters,perspective)==72&&offsetof(GroundParameters,surface)==88
 &&offsetof(GroundParameters,volume)==92&&offsetof(GroundParameters,reuse)==96&&offsetof(GroundParameters,result)==104);
inline bool Event(void* object,const wchar_t* path,uint16_t bytes,void* params){
 auto fn=engine::Find(path);
 return object&&fn&&original_players::Value<uint16_t>(fn,0xbe)==bytes&&engine::Event(object,path,params);
}
inline bool Hrtf(bool& value){
 // Pinned enum table RVA c2c19c0: EnableHRTF=55; original FParams d52ff20 size2.
 uint8_t p[2]={55,0};auto manager=original_hud::State().settingsManager;
 if(!original_hud::State().settingsInitialized||!Event(manager,L"/Script/ShooterGame.AresSettingsManager.GetBoolSetting",2,p))return false;
 value=p[1]!=0;return true;
}
inline bool SetHrtf(bool value){
 // FParams d5311a0: enum0,value1,bSaveToRoamingServiceImmediately2. No account call requested.
 uint8_t p[3]={55,uint8_t(value),0};
 return Event(original_hud::State().settingsManager,L"/Script/ShooterGame.AresSettingsManager.SetBoolSetting",3,p);
}
inline training_control::Result DispatchImpl(const uint8_t* data,uint32_t bytes,char* msg,size_t capacity,void* world,void* pc){
 using Result=training_control::Result;
 auto fail=[&](const char* s){snprintf(msg,capacity,"%s",s);return Result::Failed;};
 ExtendedRequest extended;const bool worldCoordinates=bytes==sizeof(extended);
 if(bytes!=sizeof(Request)&&!worldCoordinates)return fail("sound_fixture_invalid_request_size");std::memcpy(&extended,data,bytes);auto r=extended.request;
 if(!original_players::Thread()||r.operation>5||r.event>=5||!world||!pc||extended.sourceId>=64||extended.flags>1
    ||!std::isfinite(r.angle)||!std::isfinite(r.distance)||!std::isfinite(r.height)||!std::isfinite(r.yaw)
    ||std::abs(r.angle)>180||r.distance<50||r.distance>5000||std::abs(r.height)>500||std::abs(r.yaw)>36000)
    return fail("sound_fixture_invalid_context_or_coordinates");
 bool hrtf=false;if(!Hrtf(hrtf))return fail("sound_fixture_hrtf_setting_unavailable");
 if(r.operation==2){if(!savedSetting){originalHrtf=hrtf;savedSetting=true;}if(!SetHrtf(true)||!Hrtf(hrtf)||!hrtf)return fail("sound_fixture_hrtf_enable_readback_failed");}
 if(r.operation==3&&savedSetting){if(!SetHrtf(originalHrtf)||!Hrtf(hrtf)||hrtf!=originalHrtf)return fail("sound_fixture_hrtf_restore_failed");savedSetting=false;}
 if(r.operation==4){engine::Vec rotation{0,r.yaw,0};if(!Event(pc,L"/Script/Engine.Controller.SetControlRotation",24,&rotation))return fail("sound_fixture_original_rotation_failed");}
 original_players::CharacterDefaults dimensions;
 if((r.operation==5||r.event==4)&&!original_players::ProbeDefaults(original_players::players[0].classPath,dimensions))return fail("sound_fixture_character_dimensions_unknown");
 if(worldCoordinates&&(!original_players::Finite(extended.position)||std::abs(extended.position.x)>1000000||std::abs(extended.position.y)>1000000||std::abs(extended.position.z)>1000000))return fail("sound_fixture_world_coordinates_invalid");
 if(r.operation==5){
  if(!worldCoordinates)return fail("sound_fixture_listener_pose_requires_world_coordinates");
  engine::Vec feet{};if(!training_world_geometry::Ground(extended.position,feet,250,500)
    ||!training_world_geometry::CapsuleClear(feet,feet,dimensions.capsuleRadius,dimensions.capsuleHalfHeight))return fail("sound_fixture_listener_pose_ground_or_clearance_failed");
  struct Teleport {engine::Vec location{},rotation{};uint8_t result=0,pad[7]{};} parameters;
  parameters.location=feet;parameters.location.z+=dimensions.capsuleHalfHeight+2;parameters.rotation={0,r.yaw,0};
  if(!Event(original_players::activePawn,L"/Script/Engine.Actor.K2_TeleportTo",49,&parameters)||!parameters.result)return fail("sound_fixture_listener_original_teleport_failed");
 }
 engine::Vec camera{},rotation{},listener{},listenerRotation{};
 auto pcm=original_players::Value<void*>(pc,0x528);
 if(!Event(pcm,L"/Script/Engine.PlayerCameraManager.GetCameraLocation",24,&camera)
    ||!Event(pcm,L"/Script/Engine.PlayerCameraManager.GetCameraRotation",24,&rotation))return fail("sound_fixture_camera_unavailable");
 if(!original_audio_listeners::Sample())return fail("sound_fixture_listener_registry_unavailable");
 const auto& registry=original_audio_listeners::State().current;
 if(!registry.valid||registry.own!=1||registry.nonLocalPC)return fail("sound_fixture_requires_single_own_listener");
 void* component=nullptr;
 for(size_t i=0;i<registry.count;++i)if(registry.rows[i].kind==original_audio_listeners::Kind::Own)component=registry.rows[i].component;
 if(!Event(component,L"/Script/Engine.SceneComponent.K2_GetComponentLocation",24,&listener)
    ||!Event(component,L"/Script/Engine.SceneComponent.K2_GetComponentRotation",24,&listenerRotation))return fail("sound_fixture_listener_transform_unavailable");
 // Unreal yaw +X forward, +Y right. Positive relative angles are clockwise in top view.
 const double radians=(listenerRotation.y+r.angle)*3.14159265358979323846/180;
 engine::Vec source{listener.x+r.distance*std::cos(radians),listener.y+r.distance*std::sin(radians),listener.z+r.height};
 if(worldCoordinates&&r.operation!=5)source=extended.position;
 bool grounded=false,segmentVerified=false;engine::Vec groundFeet{};
 if(r.event==4&&r.operation==1){
  if(!training_world_geometry::Ground(source,groundFeet,250,500)
    ||!training_world_geometry::CapsuleClear(groundFeet,groundFeet,dimensions.capsuleRadius,dimensions.capsuleHalfHeight))return fail("sound_fixture_source_ground_or_clearance_failed");
  const auto id=extended.sourceId;
  if(extended.flags&1)previousValid[id]=false;
  if(previousValid[id]&&!training_world_geometry::ValidateSegment(0,previousFeet[id],groundFeet,dimensions.capsuleRadius,dimensions.capsuleHalfHeight))return fail("sound_fixture_source_path_segment_blocked");
  segmentVerified=previousValid[id];previousFeet[id]=groundFeet;previousValid[id]=true;
  grounded=true;source=groundFeet;source.z+=dimensions.capsuleHalfHeight;
 }
 if(worldCoordinates||grounded){
  r.angle=float(std::remainder(std::atan2(source.y-listener.y,source.x-listener.x)*180/3.14159265358979323846-listenerRotation.y,360.));
  r.distance=float(std::hypot(source.x-listener.x,source.y-listener.y));r.height=float(source.z-listener.z);
 }
 uint32_t playing=0;bool handlerAccepted=false;
 if(r.operation==1){
  auto event=audio_feedback::ResolveEvent(Events[r.event]);if(!event)return fail("sound_fixture_original_event_missing");
  if(r.event==4){
   auto layer=audio_feedback::ResolveEvent(WushuStep);if(!layer)return fail("sound_fixture_original_wushu_step_missing");
   void* events[2]={event,layer};const wchar_t* perspective=L"ThirdPersonEnemy";
   GroundParameters params;params.character=original_players::activePawn;params.events=events;
   params.eventCount=params.eventCapacity=2;params.location=source;params.rotation={0,0,0};
   params.perspective={perspective,int32_t(wcslen(perspective)+1),int32_t(wcslen(perspective)+1)};
   const wchar_t* method=L"/Script/ShooterGame.AresSoundHandler.OnCharacterGroundSound";
   auto fn=engine::Find(method);const auto size=original_players::Value<uint16_t>(fn,0xbe);
   if(!fn||(size!=105&&size!=112)||!engine::Event(engine::DefaultObject(engine::Class(0x653d2e0)),method,&params)){
    snprintf(msg,capacity,"sound_fixture_ground_handler_abi_%u",unsigned(size));return Result::Failed;
   }
   handlerAccepted=params.result!=0;if(!handlerAccepted)return fail("sound_fixture_original_ground_handler_not_accepted");
  }else{
  audio_feedback::PostAtLocationParameters params;params.location=source;params.orientation={0,0,0};params.worldContext=world;
  auto post=engine::Find(audio_feedback::kPostAtLocation);
  const auto size=original_players::Value<uint16_t>(post,0xbe);
  // Generated storage is 88 bytes; runtime ParmsSize may omit the four trailing padding bytes.
  if(!post||(size!=84&&size!=88)||!engine::Event(event,audio_feedback::kPostAtLocation,&params)){
   snprintf(msg,capacity,"sound_fixture_original_post_failed_parms_%u",unsigned(size));return Result::Failed;
  }
  playing=uint32_t(params.playingId);if(!playing)return fail("sound_fixture_original_playing_id_zero");
  }
 }
 HMODULE module=nullptr;wchar_t filename[32768]{};
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
    reinterpret_cast<LPCWSTR>(&DispatchImpl),&module)||!GetModuleFileNameW(module,filename,32768))return fail("sound_fixture_module_path_unavailable");
 auto slash=wcsrchr(filename,L'\\');if(!slash)return fail("sound_fixture_module_directory_unavailable");
 wcscpy_s(slash+1,32768-size_t(slash+1-filename),L"sound-locator-fixture.json");
 FILE* f=nullptr;if(_wfopen_s(&f,filename,L"wb")||!f)return fail("sound_fixture_receipt_open_failed");
 fprintf(f,"{\"sequence\":%llu,\"pid\":%lu,\"operation\":%u,\"event_id\":%u,\"event_path\":\"%ls\",\"playing_id\":%u,\"hrtf_setting\":%s,\"hrtf_dsp_verified\":false,\"own_listener_count\":%zu,\"camera\":[%.6f,%.6f,%.6f],\"camera_rotation\":[%.6f,%.6f,%.6f],\"listener\":[%.6f,%.6f,%.6f],\"listener_rotation\":[%.6f,%.6f,%.6f],\"source\":[%.6f,%.6f,%.6f],\"angle_deg\":%.6f,\"distance_cm\":%.6f,\"height_cm\":%.6f,\"tick_ms\":%llu,\"audible_output_verified\":false,\"route\":\"%s\",\"handler_accepted\":%s,\"playing_id_available\":%s,\"surface_type\":%u,\"perspective\":\"%s\",\"layer_event_path\":\"%ls\"",
  ++sequence,GetCurrentProcessId(),r.operation,r.event,Events[r.event],playing,hrtf?"true":"false",registry.own,
  camera.x,camera.y,camera.z,rotation.x,rotation.y,rotation.z,listener.x,listener.y,listener.z,
  listenerRotation.x,listenerRotation.y,listenerRotation.z,source.x,source.y,source.z,r.angle,r.distance,r.height,GetTickCount64(),
  r.event==4?"AresSoundHandler.OnCharacterGroundSound":"AkAudioEvent.PostAtLocation",handlerAccepted?"true":"false",r.event==4?"false":"true",
  r.event==4?1U:0U,r.event==4?"ThirdPersonEnemy":"unset",r.event==4?WushuStep:L"");
 fprintf(f,",\"source_id\":%u,\"ground_verified\":%s,\"segment_verified\":%s,\"ground_feet\":[%.6f,%.6f,%.6f]}",extended.sourceId,grounded?"true":"false",segmentVerified?"true":"false",groundFeet.x,groundFeet.y,groundFeet.z);
 const bool okay=fclose(f)==0;if(!okay)return fail("sound_fixture_receipt_write_failed");
 snprintf(msg,capacity,"sound_fixture_receipt_%llu_playing_%u",sequence,playing);return Result::Applied;
}
inline training_control::Result Dispatch(const uint8_t* data,uint32_t bytes,char* msg,size_t capacity,void* world,void* pc){
 __try{return DispatchImpl(data,bytes,msg,capacity,world,pc);}__except(EXCEPTION_EXECUTE_HANDLER){snprintf(msg,capacity,"sound_fixture_exception_requires_restart");return training_control::Result::Failed;}
}
}
