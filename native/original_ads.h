#pragma once
#include "original_camera.h"

// Pinned metadata/evidence: work/duel1-original-ads. Read-only observations of
// the shipped ZoomManager/ZoomComponent. No camera, state, curve or pose writes;
// no animation/bone/socket getters. Call Sample on the owned game thread.
namespace original_ads {
struct Transition {
    float sourceFov=0,sourceFov1P=0;
    int32_t sourceLevel=0;
    float targetFov=0,targetFov1P=0;
    int32_t targetLevel=0;
    float duration=0;
    uint8_t cooldownOption=0,reservedDescription[3]{};
    uint8_t state=0,reservedState[3]{};
};
struct Level {float multiplier=0;uint8_t useDefaultFov1P=0,reserved[3]{};float fov1P=0;};
// Read the original96-byte FTransform as POD doubles without requiring an
// over-aligned live transform object or invoking a native transform getter.
struct CachedTransform {std::array<double,4> quaternion{},translation{},scale{};};
struct FocusedParams {
    engine::Vec offset{},rotation{};
    float alpha=0;
    uint8_t zoomed=0,hasEquippable=0,altAnims=0;
    bool readable=false;
};
static_assert(sizeof(Transition)==36&&alignof(Transition)==4);
static_assert(offsetof(Transition,duration)==0x18&&offsetof(Transition,state)==0x20);
static_assert(sizeof(Level)==12&&offsetof(Level,fov1P)==8);
static_assert(sizeof(CachedTransform)==96&&alignof(CachedTransform)==8);
struct Snapshot {
    uint64_t sequence=0,wallTime=0;
    uint32_t tag=0;
    void *pawn=nullptr,*controller=nullptr,*camera=nullptr,*weapon=nullptr;
    void *manager=nullptr,*component=nullptr,*componentType=nullptr,*owningCharacter=nullptr;
    void *core1PAnimInstance=nullptr;
    uintptr_t fovGetterRva=0;
    bool localLinks=false,managerPresent=false,componentPresent=false,ownerMatches=false;
    bool active=false,animated=false,multiplier=false,fovComponent=false;
    bool authRead=false,predictedRead=false,levelsRead=false,cameraCacheVerified=false;
    float defaultFov=0,lockedFov=0,pcmFov=0,fov1P=0,followFov=0,cameraTime=0;
    float animatedWorldMin=0,animatedWorldMax=0,animated1PMin=0,animated1PMax=0;
    Transition authority{},predicted{};
    int32_t levelCount=0,levelCapacity=0;
    std::array<Level,8> levels{};
    void *animationComponent=nullptr,*animationLastEquippable=nullptr;
    bool animationComponentPresent=false,animationOwnerMatches=false,scopeTransformRead=false,adsIdleRefRead=false;
    bool animationDisabled=false,enteredClientRelevancy=false,justRespawned=false;
    float zoomBlendAlpha=0;
    std::array<uint32_t,3> adsIdleRefBoneName{};
    CachedTransform gunToScope{},adsIdleReference{};
    FocusedParams componentFocused{},instanceFocused{};
};
struct Diagnostics {Snapshot latest{};std::array<Snapshot,64> history{};uint64_t samples=0;};
inline Diagnostics state;
inline const Diagnostics& State(){return state;}
template<class T>inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline FocusedParams ReadFocusedParams(const void* object,size_t base){
    FocusedParams out;
    out.readable=original_players::Read(object,base+0xb0,out.offset)
        &&original_players::Read(object,base+0xc8,out.rotation)
        &&original_players::Read(object,base+0xe0,out.alpha);
    out.zoomed=Read<uint8_t>(object,base+0x163);out.hasEquippable=Read<uint8_t>(object,base+0x17e);
    out.altAnims=Read<uint8_t>(object,base+0x173);return out;
}
inline bool Sample(uint32_t tag=0){
    const auto& c=original_camera::State();
    if(!c.bound||GetCurrentThreadId()!=original_camera::gameThread||c.restartRequired)return false;
    Snapshot s{};s.sequence=state.samples+1;s.wallTime=GetTickCount64();s.tag=tag;
    s.pawn=c.pawn;s.controller=c.controller;s.camera=c.camera;
    const auto player=Read<void*>(s.controller,0x510);
    s.localLinks=player&&Read<void*>(player,0x38)==s.controller
        &&Read<void*>(s.controller,0x4b0)==s.pawn&&Read<void*>(s.pawn,0x4a0)==s.controller;
    if(!s.localLinks)return false;
    const auto inventory=Read<void*>(s.pawn,0xc08);s.weapon=Read<void*>(inventory,0x280);
    // Same read-only component lookup already used by original PCM7868d79.
    if(original_players::IsA(s.weapon,0x645c660))
        s.manager=engine::function<void*(*)(void*)>(0x6ec5800)(s.weapon);
    s.managerPresent=original_players::IsA(s.manager,0x6e06bb0);
    if(s.managerPresent){s.component=Read<void*>(s.manager,0xe0);s.owningCharacter=Read<void*>(s.manager,0x100);}
    s.componentPresent=original_players::IsA(s.component,0x63ccc30);
    if(s.componentPresent){
        s.componentType=Read<void*>(s.component,0x10);
        s.ownerMatches=Read<void*>(s.manager,0xc8)==s.weapon&&Read<void*>(s.component,0xc8)==s.weapon
            &&s.owningCharacter==s.pawn;
        s.active=(Read<uint8_t>(s.component,0xc1)&0x20)!=0;
        s.animated=original_players::IsA(s.component,0x63ccce0);
        s.multiplier=original_players::IsA(s.component,0x6e07790);
        s.fovComponent=original_players::IsA(s.component,0x6e04fa0);
        const auto table=Read<void*>(s.component,0);
        const auto getter=reinterpret_cast<uintptr_t>(Read<void*>(table,0x4d8));
        s.fovGetterRva=getter>=engine::image?getter-engine::image:0;
        s.authRead=original_players::Read(s.component,0x4b8,s.authority);
        s.predictedRead=original_players::Read(s.component,0x4dc,s.predicted);
        s.levelCount=Read<int32_t>(s.component,0x4b0);s.levelCapacity=Read<int32_t>(s.component,0x4b4);
        const auto data=Read<void*>(s.component,0x4a8);
        s.levelsRead=s.levelCount>=0&&s.levelCount<=8&&s.levelCapacity>=s.levelCount;
        for(int32_t i=0;s.levelsRead&&i<s.levelCount;++i)
            s.levelsRead=original_players::Read(data,static_cast<size_t>(i)*sizeof(Level),s.levels[static_cast<size_t>(i)]);
        if(s.animated){s.animatedWorldMin=Read<float>(s.component,0x518);s.animatedWorldMax=Read<float>(s.component,0x51c);s.animated1PMin=Read<float>(s.component,0x520);s.animated1PMax=Read<float>(s.component,0x524);}
    }
    const auto core1P=Read<void*>(s.pawn,0xf30);s.core1PAnimInstance=Read<void*>(core1P,0xa70);
    const auto actorTable=Read<void*>(s.pawn,0);const auto findComponent=Read<void*>(actorTable,0x740);
    if(findComponent)s.animationComponent=reinterpret_cast<void*(*)(void*,void*)>(findComponent)
        (s.pawn,engine::Class(0x63cef20));
    s.animationComponentPresent=original_players::IsA(s.animationComponent,0x63cef20);
    if(s.animationComponentPresent){
        s.animationOwnerMatches=Read<void*>(s.animationComponent,0xc8)==s.pawn;
        s.animationLastEquippable=Read<void*>(s.animationComponent,0xcf0);
        s.scopeTransformRead=original_players::Read(s.animationComponent,0xa50,s.gunToScope);
        s.adsIdleRefRead=original_players::Read(s.animationComponent,0xd30,s.adsIdleReference);
        s.adsIdleRefBoneName=Read<std::array<uint32_t,3>>(s.animationComponent,0xc1c);
        s.zoomBlendAlpha=Read<float>(s.animationComponent,0x9f8);
        s.animationDisabled=Read<uint8_t>(s.animationComponent,0xa44)!=0;
        s.enteredClientRelevancy=Read<uint8_t>(s.animationComponent,0xd90)!=0;
        s.justRespawned=Read<uint8_t>(s.animationComponent,0xd91)!=0;
        s.componentFocused=ReadFocusedParams(s.animationComponent,0xf0);
    }
    if(original_players::IsA(s.core1PAnimInstance,0x6c71870))
        s.instanceFocused=ReadFocusedParams(s.core1PAnimInstance,0x3b0);
    const auto follow=Read<void*>(s.pawn,0x12a8);s.followFov=Read<float>(follow,0x364);
    s.defaultFov=Read<float>(s.camera,0x484);s.lockedFov=Read<float>(s.camera,0x48c);s.fov1P=Read<float>(s.camera,0x488);
    s.cameraCacheVerified=Read<void*>(Read<void*>(s.camera,0),0x7a0)==reinterpret_cast<void*>(engine::image+0x4914b80);
    if(s.cameraCacheVerified){s.pcmFov=Read<float>(s.camera,0x17f0);s.cameraTime=Read<float>(s.camera,0x17b0);}
    state.latest=s;state.history[static_cast<size_t>(state.samples%state.history.size())]=s;++state.samples;return true;
}
inline void WriteTransition(FILE* f,const Transition& t){
    std::fprintf(f,"{\"source_fov\":%.9g,\"source_fov_1p\":%.9g,\"source_level\":%d,\"target_fov\":%.9g,\"target_fov_1p\":%.9g,\"target_level\":%d,\"duration\":%.9g,\"cooldown_option\":%u,\"state\":%u}",t.sourceFov,t.sourceFov1P,t.sourceLevel,t.targetFov,t.targetFov1P,t.targetLevel,t.duration,t.cooldownOption,t.state);
}
inline void WriteCachedTransform(FILE* f,const CachedTransform& t){
    std::fprintf(f,"{\"quaternion_xyzw\":[%.17g,%.17g,%.17g,%.17g],\"translation\":[%.17g,%.17g,%.17g],\"scale\":[%.17g,%.17g,%.17g]}",t.quaternion[0],t.quaternion[1],t.quaternion[2],t.quaternion[3],t.translation[0],t.translation[1],t.translation[2],t.scale[0],t.scale[1],t.scale[2]);
}
inline void WriteFocusedParams(FILE* f,const FocusedParams& p){
    std::fprintf(f,"{\"readable\":%s,\"zoomed\":%u,\"has_equippable\":%u,\"alt_anims\":%u,\"alpha\":%.9g,\"offset\":",p.readable?"true":"false",p.zoomed,p.hasEquippable,p.altAnims,p.alpha);original_players::Vector(f,p.offset);std::fputs(",\"rotation_pitch_yaw_roll\":",f);original_players::Vector(f,p.rotation);std::fputc('}',f);
}
inline void WriteSnapshot(FILE* f,const Snapshot& s){
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(f,"{\"sequence\":%llu,\"wall_time_ms\":%llu,\"tag\":%u,\"camera_time\":%.9g",static_cast<unsigned long long>(s.sequence),static_cast<unsigned long long>(s.wallTime),s.tag,s.cameraTime);
    const char* names[]{"pawn","controller","camera","weapon","zoom_manager","zoom_component","zoom_component_type","owning_character","core_1p_anim_instance"};
    void* const values[]{s.pawn,s.controller,s.camera,s.weapon,s.manager,s.component,s.componentType,s.owningCharacter,s.core1PAnimInstance};
    for(size_t i=0;i<9;++i)std::fprintf(f,",\"%s\":\"%p\"",names[i],values[i]);
    const char* bn[]{"local_links","manager_present","component_present","owner_matches","active","animated","multiplier","fov_component","auth_read","predicted_read","levels_read","camera_cache_verified"};
    const bool bv[]{s.localLinks,s.managerPresent,s.componentPresent,s.ownerMatches,s.active,s.animated,s.multiplier,s.fovComponent,s.authRead,s.predictedRead,s.levelsRead,s.cameraCacheVerified};
    for(size_t i=0;i<12;++i)std::fprintf(f,",\"%s\":%s",bn[i],b(bv[i]));
    std::fprintf(f,",\"fov_getter_rva\":%llu,\"default_fov\":%.9g,\"locked_fov\":%.9g,\"follow_fov\":%.9g,\"pcm_fov\":%.9g,\"fov_1p\":%.9g,\"animated_world_limits\":[%.9g,%.9g],\"animated_1p_limits\":[%.9g,%.9g],\"authority\":",static_cast<unsigned long long>(s.fovGetterRva),s.defaultFov,s.lockedFov,s.followFov,s.pcmFov,s.fov1P,s.animatedWorldMin,s.animatedWorldMax,s.animated1PMin,s.animated1PMax);WriteTransition(f,s.authority);std::fputs(",\"predicted\":",f);WriteTransition(f,s.predicted);
    std::fprintf(f,",\"level_count\":%d,\"level_capacity\":%d,\"levels\":[",s.levelCount,s.levelCapacity);
    for(int32_t i=0;s.levelsRead&&i<s.levelCount;++i){const auto& l=s.levels[static_cast<size_t>(i)];std::fprintf(f,"%s{\"multiplier\":%.9g,\"use_default_fov_1p\":%s,\"fov_1p\":%.9g}",i?",":"",l.multiplier,b(l.useDefaultFov1P!=0),l.fov1P);}std::fputc(']',f);
    std::fprintf(f,",\"animation_component\":\"%p\",\"animation_last_equippable\":\"%p\",\"animation_component_present\":%s,\"animation_owner_matches\":%s,\"scope_transform_read\":%s,\"ads_idle_ref_read\":%s,\"animation_disabled\":%s,\"entered_client_relevancy\":%s,\"just_respawned\":%s,\"zoom_blend_alpha\":%.9g,\"ads_idle_ref_bone_fname_u32\":[%u,%u,%u],\"gun_to_scope_cached\":",s.animationComponent,s.animationLastEquippable,b(s.animationComponentPresent),b(s.animationOwnerMatches),b(s.scopeTransformRead),b(s.adsIdleRefRead),b(s.animationDisabled),b(s.enteredClientRelevancy),b(s.justRespawned),s.zoomBlendAlpha,s.adsIdleRefBoneName[0],s.adsIdleRefBoneName[1],s.adsIdleRefBoneName[2]);WriteCachedTransform(f,s.gunToScope);std::fputs(",\"ads_idle_reference_cached\":",f);WriteCachedTransform(f,s.adsIdleReference);std::fputs(",\"component_focused_params\":",f);WriteFocusedParams(f,s.componentFocused);std::fputs(",\"instance_focused_params\":",f);WriteFocusedParams(f,s.instanceFocused);std::fputc('}',f);
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    std::fprintf(f,"{\"schema_version\":2,\"pid\":%lu,\"samples\":%llu,\"native_state_modified\":false,\"animation_or_pose_evaluated\":false,\"latest\":",GetCurrentProcessId(),static_cast<unsigned long long>(state.samples));WriteSnapshot(f,state.latest);std::fputs(",\"history\":[",f);
    const auto count=state.samples<state.history.size()?state.samples:static_cast<uint64_t>(state.history.size());
    for(uint64_t n=0;n<count;++n){if(n)std::fputc(',',f);const auto index=(state.samples-count+n)%state.history.size();WriteSnapshot(f,state.history[static_cast<size_t>(index)]);}std::fputs("]}\n",f);
    const bool ok=std::ferror(f)==0;std::fclose(f);return ok;
}
}
