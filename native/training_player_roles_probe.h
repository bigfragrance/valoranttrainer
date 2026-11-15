#pragma once
#include "original_camera_owners.h"
#include "original_jett_priming.h"
#include <array>
#include <cmath>

// Observe actual links, effective native locality and Wwise perspective.
// The effective getters may exercise the original standalone cache branch;
// the earlier baseline artifact separately records cache-only observations.
namespace training_player_roles_probe {
struct Row {
    bool present=false,local=false,playerIsLocal=false,inLocalPlayers=false;
    bool actorQuery=false,controlQuery=false,coreQuery=false,cosmeticQuery=false;
    void *pawn=nullptr,*controller=nullptr,*playerState=nullptr,*player=nullptr;
    void *cameraManager=nullptr,*cameraOwner=nullptr,*followCamera=nullptr,*followOwner=nullptr;
    engine::Vec actor{},control{},core{},cosmetic{};
    engine::Vec coreRelative{},cosmeticRelative{};
    void *root=nullptr,*coreParent=nullptr,*cosmeticParent=nullptr;
    uint8_t cachedLocal=0,role=0,remoteRole=0;double yawError=0;
    bool localityQuery=false,effectiveLocal=false,perspectiveQuery=false;
    bool alive=false;
    uint8_t rootSceneFlags=0,coreSceneFlags=0,cosmeticSceneFlags=0,coreVisibilityFlags=0,cosmeticVisibilityFlags=0;
    uint8_t coreAbsoluteFlags=0,cosmeticAbsoluteFlags=0;
    bool attenuationQuery=false;unsigned attenuationError=0;int32_t constantVisuals=0;float oneShotRadius=0,constantRadius=0;
    char perspective[64]{};
};
inline std::array<Row,original_players::MaxPlayers> rows{};
inline uint64_t samples=0;
inline int localPlayerCount=0;
inline unsigned rosterCount=0,cachedLocalCount=0,actualLocalCount=0;
inline double maximumNPCYawError=0;
inline void* listenerController=nullptr;
inline bool listenerQuery=false;
inline int attenuationStructBytes=0;
inline std::array<int,6> attenuationOffsets{};
template<class T> inline T Read(const void* o,size_t at){return original_players::Value<T>(o,at);}
inline bool Query(void* o,const wchar_t* name,void* result){
    auto fn=engine::Find(name);return o&&fn&&Read<uint16_t>(fn,0xbe)==24&&engine::Event(o,name,result);
}
inline double Angle(double x){while(x>180)x-=360;while(x<-180)x+=360;return x;}
inline bool Perspective(Row& row){
    auto fn=engine::Find(L"/Script/ShooterGame.ShooterCharacter.GetWwisePerspective");
    struct Text {wchar_t* data=nullptr;int32_t count=0,capacity=0;} p;
    static_assert(sizeof(Text)==16);
    if(!fn||Read<uint16_t>(fn,0xbe)!=16||!engine::Event(row.pawn,L"/Script/ShooterGame.ShooterCharacter.GetWwisePerspective",&p))return false;
    const bool valid=p.count>0&&p.count<=128&&p.capacity>=p.count&&original_players::Readable(p.data,static_cast<size_t>(p.count)*2)&&p.data[p.count-1]==0;
    if(valid){const auto text=original_players::Utf8(p.data);strncpy_s(row.perspective,text.c_str(),_TRUNCATE);}
    if(p.data)engine::function<void(*)(void*)>(0x1915720)(p.data);
    return valid;
}
inline bool Attenuation(Row& row){
    constexpr auto componentPath=L"/Game/UI/Screens/InGame/HUD/Minimap/Components/ShooterCharacterAudioAttenuationComponent.ShooterCharacterAudioAttenuationComponent_C";
    constexpr auto structPath=L"/Game/UI/Screens/InGame/HUD/Minimap/Components/Struct_AttenuationInformation.Struct_AttenuationInformation";
    auto type=engine::Find(componentPath),structure=engine::Find(structPath);
    row.attenuationError=1;if(!type||!structure)return false;
    row.attenuationError=2;attenuationStructBytes=Read<int32_t>(structure,0x60);
    // UUserDefinedStruct keeps21 bytes of properties; native alignment8 gives
    // the24-byte stored struct stride. Validate both instead of conflating them.
    if(attenuationStructBytes<21||((attenuationStructBytes+7)&~7)!=24)return false;
    using original_jett_priming::FieldOffset;
    original_jett_priming::PropertyInfo oneShotInfo{},constantInfo{};
    attenuationOffsets={FieldOffset(structure,L"AkComponent_14_106F310C43CDB01FD0A8DEBFDE517218",8),
       FieldOffset(structure,L"OwningActor_17_C26E53BD408A01E63122CDB78028F7CC",8),
       FieldOffset(structure,L"Radius_5_E6DBEEBE41FF7D345D977287C3DB0054",4),
       FieldOffset(structure,L"Type_12_5BA7C0944FAC16BF24CA378849F7DB22",1),
       FieldOffset(type,L"CurrentOneShotVisualization",24,&oneShotInfo),FieldOffset(type,L"ConstantVisualizations",16,&constantInfo)};
    row.attenuationError=3;if(attenuationOffsets[0]!=0||attenuationOffsets[1]!=8||attenuationOffsets[2]!=16||attenuationOffsets[3]!=20)return false;
    const int shot=attenuationOffsets[4],constant=attenuationOffsets[5];
    row.attenuationError=4;if(shot<0||constant<0||oneShotInfo.elementSize!=24||oneShotInfo.arrayDim!=1
       ||constantInfo.elementSize!=16||constantInfo.arrayDim!=1)return false;
    struct Component {void* type;void* result=nullptr;} p{type};
    auto fn=engine::Find(L"/Script/Engine.Actor.GetComponentByClass");
    row.attenuationError=5;if(!fn||Read<uint16_t>(fn,0xbe)!=16||!engine::Event(row.pawn,L"/Script/Engine.Actor.GetComponentByClass",&p)
       ||!p.result||Read<void*>(p.result,0xc8)!=row.pawn||Read<void*>(p.result,0x10)!=type)return false;
    row.oneShotRadius=Read<float>(p.result,static_cast<size_t>(shot)+16);
    auto array=static_cast<uint8_t*>(p.result)+constant;auto data=Read<void*>(array,0);
    row.constantVisuals=Read<int32_t>(array,8);const auto capacity=Read<int32_t>(array,12);
    row.attenuationError=6;if(row.constantVisuals<0||row.constantVisuals>1024||capacity<row.constantVisuals||!std::isfinite(row.oneShotRadius)
       ||(row.constantVisuals&&!original_players::Readable(data,static_cast<size_t>(row.constantVisuals)*24)))return false;
    for(int32_t i=0;i<row.constantVisuals;++i){const auto radius=Read<float>(data,static_cast<size_t>(i)*24+16);
        if(!std::isfinite(radius))return false;if(radius>row.constantRadius)row.constantRadius=radius;}
    row.attenuationError=0;return true;
}
inline bool Sample(){
    if(!original_players::Thread())return false;
    auto gi=Read<void*>(original_players::world,0x1d8);auto locals=Read<void**>(gi,0x40);
    localPlayerCount=Read<int32_t>(gi,0x48);
    if(localPlayerCount<1||localPlayerCount>64||!original_players::Readable(locals,static_cast<size_t>(localPlayerCount)*8))return false;
    rosterCount=cachedLocalCount=actualLocalCount=0;maximumNPCYawError=0;
    const auto listenerPath=L"/Script/ShooterGame.AresAudioComponent.AresGetActiveListenerPlayerController";
    auto listenerFunction=engine::Find(listenerPath);struct Listener {void* world;void* result=nullptr;} listener{original_players::world};
    listenerQuery=listenerFunction&&Read<uint16_t>(listenerFunction,0xbe)==16
        &&engine::Event(engine::DefaultObject(Read<void*>(listenerFunction,0x28)),listenerPath,&listener);
    listenerController=listenerQuery?listener.result:nullptr;
    for(size_t i=0;i<rows.size();++i){auto& r=rows[i];r={};const auto& p=original_players::players[i];
        if(!p.ready||!p.registered)continue;r.present=true;r.local=p.local;r.pawn=p.character;r.controller=p.controller;r.playerState=p.state;
        r.player=Read<void*>(r.controller,0x510);r.playerIsLocal=original_players::IsA(r.player,0x3b6e680);
        r.cameraManager=Read<void*>(r.controller,0x528);r.cameraOwner=Read<void*>(r.cameraManager,0x468);
        r.followCamera=Read<void*>(r.pawn,0x12a8);r.followOwner=Read<void*>(r.followCamera,0xc8);
        r.cachedLocal=Read<uint8_t>(r.controller,0x8b4);r.role=Read<uint8_t>(r.pawn,0x224);r.remoteRole=Read<uint8_t>(r.pawn,0x128);
        r.alive=Read<uint8_t>(Read<void*>(r.pawn,0xc70),0x1f9)!=0;
        r.root=Read<void*>(r.pawn,0x290);
        r.coreParent=Read<void*>(Read<void*>(r.pawn,0x4f0),0xe8);r.cosmeticParent=Read<void*>(Read<void*>(r.pawn,0xf40),0xe8);
        r.coreRelative=Read<engine::Vec>(Read<void*>(r.pawn,0x4f0),0x188);r.cosmeticRelative=Read<engine::Vec>(Read<void*>(r.pawn,0xf40),0x188);
        r.coreAbsoluteFlags=Read<uint8_t>(Read<void*>(r.pawn,0x4f0),0x1d0);r.cosmeticAbsoluteFlags=Read<uint8_t>(Read<void*>(r.pawn,0xf40),0x1d0);
        r.rootSceneFlags=Read<uint8_t>(Read<void*>(r.pawn,0x290),0x250);
        r.coreSceneFlags=Read<uint8_t>(Read<void*>(r.pawn,0x4f0),0x250);
        r.cosmeticSceneFlags=Read<uint8_t>(Read<void*>(r.pawn,0xf40),0x250);
        r.coreVisibilityFlags=Read<uint8_t>(Read<void*>(r.pawn,0x4f0),0x251);
        r.cosmeticVisibilityFlags=Read<uint8_t>(Read<void*>(r.pawn,0xf40),0x251);
        for(int n=0;n<localPlayerCount;++n){auto local=Read<void*>(locals,static_cast<size_t>(n)*8);
            if(r.player==local&&Read<void*>(local,0x38)==r.controller)r.inLocalPlayers=true;}
        r.actorQuery=Query(r.pawn,L"/Script/Engine.Actor.K2_GetActorRotation",&r.actor);
        r.controlQuery=Query(r.controller,L"/Script/Engine.Controller.GetControlRotation",&r.control);
        r.coreQuery=Query(Read<void*>(r.pawn,0x4f0),L"/Script/Engine.SceneComponent.K2_GetComponentRotation",&r.core);
        r.cosmeticQuery=Query(Read<void*>(r.pawn,0xf40),L"/Script/Engine.SceneComponent.K2_GetComponentRotation",&r.cosmetic);
        uint8_t effective=0;auto localFunction=engine::Find(L"/Script/Engine.Pawn.IsLocallyControlled");
        r.localityQuery=localFunction&&Read<uint16_t>(localFunction,0xbe)==1&&engine::Event(r.pawn,L"/Script/Engine.Pawn.IsLocallyControlled",&effective);
        r.effectiveLocal=r.localityQuery&&effective!=0;r.perspectiveQuery=Perspective(r);
        r.attenuationQuery=Attenuation(r);
        r.yawError=r.actorQuery&&r.controlQuery?std::abs(Angle(r.actor.y-r.control.y)):0;
        ++rosterCount;if(r.cachedLocal)++cachedLocalCount;if(r.inLocalPlayers)++actualLocalCount;
        if(!r.local&&r.yawError>maximumNPCYawError)maximumNPCYawError=r.yawError;
    }
    ++samples;return true;
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!Sample()||!path)return false;std::wstring tmp(path);tmp+=L".tmp";
    FILE* f=nullptr;if(_wfopen_s(&f,tmp.c_str(),L"wb")||!f)return false;const auto b=[](bool x){return x?"true":"false";};
    std::fprintf(f,"{\"pid\":%lu,\"samples\":%llu,\"local_player_count\":%d,\"roster_count\":%u,\"cached_local_count\":%u,\"actual_local_count\":%u,\"maximum_npc_actor_control_yaw_error\":%.9g,\"calls_local_predicates\":true,\"listener_query\":%s,\"listener_controller\":\"%p\",\"listener_is_actual_local\":%s,\"players\":[",GetCurrentProcessId(),static_cast<unsigned long long>(samples),localPlayerCount,rosterCount,cachedLocalCount,actualLocalCount,maximumNPCYawError,b(listenerQuery),listenerController,b(listenerController==original_players::activeController));
    bool comma=false;for(size_t i=0;i<rows.size();++i){const auto& r=rows[i];if(!r.present)continue;
        std::fprintf(f,"%s{\"index\":%zu,\"local\":%s,\"pawn\":\"%p\",\"controller\":\"%p\",\"player_state\":\"%p\",\"uplayer\":\"%p\",\"uplayer_is_local\":%s,\"in_local_players\":%s,\"cached_local_controller\":%u,\"role\":%u,\"remote_role\":%u,\"actor_query\":%s,\"control_query\":%s,\"core_query\":%s,\"cosmetic_query\":%s,\"actor_rotation\":",comma?",":"",i,b(r.local),r.pawn,r.controller,r.playerState,r.player,b(r.playerIsLocal),b(r.inLocalPlayers),static_cast<unsigned>(r.cachedLocal),static_cast<unsigned>(r.role),static_cast<unsigned>(r.remoteRole),b(r.actorQuery),b(r.controlQuery),b(r.coreQuery),b(r.cosmeticQuery));
        original_players::Vector(f,r.actor);std::fputs(",\"control_rotation\":",f);original_players::Vector(f,r.control);
        std::fprintf(f,",\"camera_manager\":\"%p\",\"camera_manager_owner\":\"%p\",\"follow_camera\":\"%p\",\"follow_owner\":\"%p\"",r.cameraManager,r.cameraOwner,r.followCamera,r.followOwner);
        std::fputs(",\"core_rotation\":",f);original_players::Vector(f,r.core);std::fputs(",\"cosmetic_rotation\":",f);original_players::Vector(f,r.cosmetic);
        std::fprintf(f,",\"root\":\"%p\",\"core_parent\":\"%p\",\"cosmetic_parent\":\"%p\",\"core_absolute_flags_1d0\":%u,\"cosmetic_absolute_flags_1d0\":%u,\"core_relative_rotation\":",r.root,r.coreParent,r.cosmeticParent,r.coreAbsoluteFlags,r.cosmeticAbsoluteFlags);
        original_players::Vector(f,r.coreRelative);std::fputs(",\"cosmetic_relative_rotation\":",f);original_players::Vector(f,r.cosmeticRelative);
        std::fprintf(f,",\"root_scene_flags_250\":%u,\"core_scene_flags_250\":%u,\"cosmetic_scene_flags_250\":%u,\"core_visibility_flags_251\":%u,\"cosmetic_visibility_flags_251\":%u",r.rootSceneFlags,r.coreSceneFlags,r.cosmeticSceneFlags,r.coreVisibilityFlags,r.cosmeticVisibilityFlags);
        std::fprintf(f,",\"alive\":%s,\"actor_control_yaw_error\":%.9g,\"locality_query\":%s,\"effective_locally_controlled\":%s,\"perspective_query\":%s,\"wwise_perspective\":",b(r.alive),r.yawError,b(r.localityQuery),b(r.effectiveLocal),b(r.perspectiveQuery));
        original_players::Text(f,r.perspective);
        std::fprintf(f,",\"attenuation_query\":%s,\"attenuation_error\":%u,\"attenuation_struct_bytes\":%d,\"attenuation_offsets\":[%d,%d,%d,%d,%d,%d],\"one_shot_radius\":%.9g,\"constant_visualizations\":%d,\"maximum_constant_radius\":%.9g}",b(r.attenuationQuery),r.attenuationError,attenuationStructBytes,attenuationOffsets[0],attenuationOffsets[1],attenuationOffsets[2],attenuationOffsets[3],attenuationOffsets[4],attenuationOffsets[5],r.attenuationQuery?r.oneShotRadius:0.0f,r.attenuationQuery?r.constantVisuals:0,r.attenuationQuery?r.constantRadius:0.0f);comma=true;
    }
    std::fputs("]}\n",f);const bool ok=std::ferror(f)==0;const bool closed=std::fclose(f)==0;
    return ok&&closed&&MoveFileExW(tmp.c_str(),path,MOVEFILE_REPLACE_EXISTING);
}
}
