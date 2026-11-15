#pragma once
#include "original_players.h"
#include "original_champions.h"
#include <cstdio>

// Probe is read-only. Update restores the missing local attachment factory and
// the original Reflex Blueprint's Standalone-skipped client scope branch once.
// No hand-written scope offset, FOV, animation state or pose calculation.
// ScopeComponent_C has exactly one serialized own FTransform property. Its
// runtime linker offset and class size are checked before the cached value is read.
// Evidence: reports/duel1-original-ads-animation-audit.json.
namespace original_scope {
inline constexpr const wchar_t* kScope=L"/Game/Equippables/Guns/_Core/ScopeComponent.ScopeComponent_C";
inline constexpr const wchar_t* kReflex=L"/Game/Equippables/Guns/Attachments/_Core/Reflex/Attachment_Base_Reflex.Attachment_Base_Reflex_C";
inline constexpr const wchar_t* kChampionsReflex=L"/Game/Equippables/Guns/Attachments/RedDot/Champions/Attachment_Champions_RedDot.Attachment_Champions_RedDot_C";
inline constexpr const wchar_t* kDefaultReflex=L"/Game/Equippables/Guns/Attachments/RedDot/Attachment_Base_Reflex_RedDot.Attachment_Base_Reflex_RedDot_C";
inline constexpr const wchar_t* kReflexData=L"/Game/Equippables/Guns/Rifles/AK/Attachments/Reflex/RedDot/Attachment_AK_ReflexPrimaryAsset.Attachment_AK_ReflexPrimaryAsset_C";
inline constexpr const wchar_t* kReflexGraph=L"/Game/Equippables/Guns/Attachments/_Core/Reflex/Attachment_Base_Reflex.Attachment_Base_Reflex_C.ExecuteUbergraph_Attachment_Base_Reflex";
struct Diagnostics {
    unsigned error=0;uint64_t samples=0;
    bool scope_class_loaded=false,reflex_class_loaded=false,champions_reflex_class_loaded=false;
    bool scope_present=false,reflex_present=false,champions_reflex_present=false;
    bool component_query_abi=false,scope_layout_matches=false,scope_transform_read=false;
    bool scope_transform_identity=false,gun_to_scope_read=false,gun_to_scope_identity=false;
    bool scope_registered=false,scope_begun=false,reflex_registered=false,reflex_begun=false;
    bool core_instance_present=false,core_zoomed=false,core_alt_anims=false,core_has_equippable=false;
    void *pawn=nullptr,*weapon=nullptr,*scopeClass=nullptr,*scope=nullptr,*reflex=nullptr,*championsReflex=nullptr;
    void *coreInstance=nullptr,*animationState=nullptr,*scopeProperty=nullptr;
    int scope_property_offset=-1,scope_class_bytes=0,attachment_data_count=0;
    uint32_t transformAlignmentPadding=0;
    engine::Transform scopeTransform{},gunToScope{};
    const char* stage="original_scope_not_observed";
    uint64_t trailingAlignmentPadding=0;
};
inline Diagnostics state;
struct Lifecycle {
    unsigned error=0,generation=0,created=0,clientCatchups=0,skinChanges=0;
    bool initialized=false,preloadRequested=false,assetsLoaded=false,meshReady=false;
    bool catchupAttempted=false,scopeReady=false,animationConsumed=false,metadataValid=false;
    void *world=nullptr,*pawn=nullptr,*weapon=nullptr,*skin=nullptr,*level=nullptr;
    void *data=nullptr,*baseType=nullptr,*selectedType=nullptr,*component=nullptr;
    void *mesh1P=nullptr,*parent1P=nullptr,*graph=nullptr;
    int meshOffset=-1,parentOffset=-1,skinOverrideCount=-1;
    const char* stage="original_scope_factory_not_requested";
};
inline Lifecycle lifecycle;
inline const Lifecycle& FactoryState(){return lifecycle;}
inline const Diagnostics& State(){return state;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Fail(unsigned code,const char* stage){state.error=code;state.stage=stage;return false;}
inline bool LifecycleFail(unsigned code,const char* stage){lifecycle.error=code;lifecycle.stage=stage;return false;}
inline bool Identity(const engine::Transform& t){
    return std::abs(t.translation.x)<1e-5&&std::abs(t.translation.y)<1e-5&&std::abs(t.translation.z)<1e-5
      &&std::abs(t.rotation[0])<1e-5&&std::abs(t.rotation[1])<1e-5&&std::abs(t.rotation[2])<1e-5
      &&std::abs(std::abs(t.rotation[3])-1)<1e-5&&std::abs(t.scale.x-1)<1e-5&&std::abs(t.scale.y-1)<1e-5&&std::abs(t.scale.z-1)<1e-5;
}
inline void* Query(void* actor,void* type){
    if(!actor||!type||!state.component_query_abi)return nullptr;
    struct P {void* type;void* result=nullptr;} p{type};
    return engine::Event(actor,L"/Script/Engine.Actor.GetComponentByClass",&p)?p.result:nullptr;
}
inline bool Probe(void* world,void* pawn,void* weapon){
    if(!engine::image||original_players::gameThread!=GetCurrentThreadId())return Fail(1841,"scope_observation_wrong_game_thread");
    const auto samples=state.samples;state=Diagnostics{};state.samples=samples+1;state.pawn=pawn;state.weapon=weapon;
    if(!world||!original_players::IsA(pawn,0x6c5c480)||!original_players::IsA(weapon,0x645c660))return Fail(1842,"scope_requires_actual_original_pawn_and_weapon");
    auto method=engine::Find(L"/Script/Engine.Actor.GetComponentByClass");
    state.component_query_abi=method&&Read<uint16_t>(method,0xbe)==16;
    state.scopeClass=engine::Find(kScope);auto reflexClass=engine::Find(kReflex),championClass=engine::Find(kChampionsReflex);
    state.scope_class_loaded=state.scopeClass!=nullptr;state.reflex_class_loaded=reflexClass!=nullptr;state.champions_reflex_class_loaded=championClass!=nullptr;
    state.scope=Query(weapon,state.scopeClass);state.reflex=Query(weapon,reflexClass);state.championsReflex=Query(weapon,championClass);
    state.scope_present=state.scope!=nullptr;state.reflex_present=state.reflex!=nullptr;state.champions_reflex_present=state.championsReflex!=nullptr;
    state.scope_registered=(Read<uint8_t>(state.scope,0xa0)&1)!=0;state.scope_begun=(Read<uint8_t>(state.scope,0xc2)&0x40)!=0;
    state.reflex_registered=(Read<uint8_t>(state.reflex,0xa0)&1)!=0;state.reflex_begun=(Read<uint8_t>(state.reflex,0xc2)&0x40)!=0;
    state.scope_class_bytes=Read<int>(state.scopeClass,0x60);state.scopeProperty=Read<void*>(state.scopeClass,0x58);
    state.scope_property_offset=state.scopeProperty?Read<int>(state.scopeProperty,0x44):-1;
    // ActorComponent native size d8, sole FTransform property aligned16 -> e0.
    // A mismatch reports unknown instead of guessing another property address.
    state.scope_layout_matches=state.scopeProperty&&state.scope_property_offset==0xe0&&state.scope_class_bytes==0x140;
    if(state.scope&&state.scope_layout_matches){
      state.scope_transform_read=original_players::Read(state.scope,0xe0,state.scopeTransform);
      state.scope_transform_identity=state.scope_transform_read&&Identity(state.scopeTransform);
    }
    auto core=Read<void*>(pawn,0xf30);state.coreInstance=Read<void*>(core,0xa70);state.core_instance_present=state.coreInstance!=nullptr;
    state.animationState=Read<void*>(state.coreInstance,0x390);
    if(original_players::IsA(state.animationState,0x63cef20)){
      state.gun_to_scope_read=original_players::Read(state.animationState,0xa50,state.gunToScope);
      state.gun_to_scope_identity=state.gun_to_scope_read&&Identity(state.gunToScope);
    }
    state.core_zoomed=Read<uint8_t>(state.coreInstance,0x3b0+0x163)!=0;
    state.core_alt_anims=Read<uint8_t>(state.coreInstance,0x3b0+0x173)!=0;
    state.core_has_equippable=Read<uint8_t>(state.coreInstance,0x3b0+0x17e)!=0;
    state.attachment_data_count=Read<int>(weapon,0xfc8);
    state.stage="original_scope_cached_state_observed";return true;
}
inline bool Event(void* object,const wchar_t* path,uint16_t bytes,void* params){
    auto fn=engine::Find(path);
    return object&&fn&&Read<uint16_t>(fn,0xbe)==bytes&&engine::Event(object,path,params);
}
inline void* ResolveLoadedSoft(void* value){
    if(!value)return nullptr;
    auto result=engine::function<void*(*)(void*)>(0x1d4cfa0)(value);
    return result?result:engine::function<void*(*)(void*)>(0x1ce5c40)(static_cast<uint8_t*>(value)+8);
}
inline int FieldOffset(void* type,const wchar_t* wanted){
    if(!type||!wanted)return -1;
    // Use the actual pinned FindFProperty implementation; do not walk a guessed
    // FField layout or format names read from arbitrary memory. 1c63870 compares
    // FField.Name+20 and follows Next+18 through typed iterator1b9efc0.
    // Its flags3 include inherited and deprecated properties; the iterator
    // itself checks FFieldClass.CastFlags&8001 before returning an FProperty.
    original_content_services::Name name{};
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&name,wanted,1);
    auto field=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,&name,3);
    if(!field)return -1;
    const auto fieldClass=Read<void*>(field,8);
    const auto flags=Read<uint64_t>(fieldClass,0x18);
    if((flags&0x8001)!=0x8001||Read<uint64_t>(field,0x20)!=original_scope::Read<uint64_t>(&name,0))return -1;
    const int bytes=Read<int>(type,0x60),offset=Read<int>(field,0x44);
    return offset>=0&&offset+8<=bytes?offset:-1;
}
inline bool OriginalGraphProfile(){
    lifecycle.graph=engine::Find(kReflexGraph);
    const auto type=engine::Find(kReflex);
    const auto script=Read<uint8_t*>(lifecycle.graph,0x68);
    // The exact complete loaded VM script has3631 bytes. Its normal entry
    // pushes3628 (Return), then dispatches EntryPoint. VM761 begins the original
    // client branch with GetOwner. No code or branch condition is patched.
    lifecycle.metadataValid=lifecycle.graph&&Read<void*>(lifecycle.graph,0x28)==type
        &&Read<uint16_t>(lifecycle.graph,0xbe)==4&&Read<int>(lifecycle.graph,0x70)==3631
        &&Read<uint8_t>(script,0)==0x4c&&Read<uint32_t>(script,1)==3628
        &&Read<uint8_t>(script,5)==0x4e&&Read<uint8_t>(script,761)==0x5f
        &&FieldOffset(state.scopeClass,L"ScopeTransformComponentSpace")==0xe0;
    return lifecycle.metadataValid;
}
inline bool SelectOriginalAttachment(){
    auto dataClass=original_champions::LoadClass(0x6763d30,kReflexData);
    auto base=original_champions::LoadClass(0x6574f50,kDefaultReflex);
    auto champion=original_champions::LoadClass(0x6574f50,kChampionsReflex);
    lifecycle.data=dataClass?engine::DefaultObject(dataClass):nullptr;
    if(!original_players::IsA(lifecycle.data,0x6763d30)||!base||!champion)
        return LifecycleFail(1851,"original_scope_attachment_assets_not_loaded");
    // Authored EquippableAttachmentDataAsset.Attachment is the real soft class.
    // Its ForcedAttachments membership and Champions map pair are decoded in
    // work/duel1-original-ads-attachments. No champion DataAsset is invented.
    lifecycle.baseType=ResolveLoadedSoft(static_cast<uint8_t*>(lifecycle.data)+0xb8);
    if(lifecycle.baseType!=base)return LifecycleFail(1852,"original_ak_reflex_data_class_mismatch");
    lifecycle.skin=Read<void*>(lifecycle.weapon,0xfb0);lifecycle.level=Read<void*>(lifecycle.weapon,0xfb8);
    lifecycle.skinOverrideCount=lifecycle.skin?original_content::MapCount(lifecycle.skin,0x108):0;
    if(lifecycle.skin&&lifecycle.skin==original_champions::skinData){
        if(lifecycle.skinOverrideCount!=1)return LifecycleFail(1853,"original_champions_attachment_override_map_changed");
        lifecycle.selectedType=champion;
    }else if(lifecycle.skinOverrideCount==0){lifecycle.selectedType=base;}
    else return LifecycleFail(1854,"original_scope_unresolved_skin_attachment_override");
    lifecycle.meshOffset=FieldOffset(base,L"ReflexMesh1PSpawned");
    lifecycle.parentOffset=FieldOffset(base,L"1P Parent Mesh");
    if(lifecycle.meshOffset<0||lifecycle.parentOffset<0||!OriginalGraphProfile())
        return LifecycleFail(1855,"original_scope_reflection_or_blueprint_profile_mismatch");
    return true;
}
inline bool CreateOriginalAttachment(){
    auto existing=Query(lifecycle.weapon,engine::Find(kReflex));
    if(existing){
        if(Read<void*>(existing,0x10)!=lifecycle.selectedType||Read<void*>(existing,0xf8)!=lifecycle.data)
            return LifecycleFail(1856,"original_scope_existing_attachment_identity_mismatch");
        lifecycle.component=existing;
    }else{
        struct alignas(16) Add {void* type;uint8_t manual=0,pad[7]{};engine::Transform transform;
            uint8_t deferred=1,pad2[7]{};void* result=nullptr;} add{lifecycle.selectedType};
        static_assert(sizeof(Add)==128);
        if(!Event(lifecycle.weapon,L"/Script/Engine.Actor.AddComponentByClass",128,&add)||!add.result)
            return LifecycleFail(1857,"original_scope_attachment_create_failed");
        lifecycle.component=add.result;
        if(!original_players::Write(add.result,0xf8,lifecycle.data)
            ||!original_content_services::ObserveAttachment(add.result))
            return LifecycleFail(1858,"original_scope_attachment_preload_observer_failed");
        struct alignas(16) Finish {void* component;uint8_t manual=0,pad[7]{};engine::Transform transform;} finish{add.result};
        static_assert(sizeof(Finish)==112);
        if(!Event(lifecycle.weapon,L"/Script/Engine.Actor.FinishAddComponent",112,&finish))
            return LifecycleFail(1859,"original_scope_attachment_finish_failed");
        ++lifecycle.created;
    }
    if(Read<void*>(lifecycle.component,0x28)!=lifecycle.weapon||Read<void*>(lifecycle.component,0xc8)!=lifecycle.weapon
        ||Query(lifecycle.weapon,lifecycle.selectedType)!=lifecycle.component)
        return LifecycleFail(1860,"original_scope_attachment_owner_registration_mismatch");
    for(const auto& entry:original_content_services::attachments)
        if(entry.component==lifecycle.component&&entry.requestIssued)lifecycle.preloadRequested=true;
    if(!lifecycle.preloadRequested)lifecycle.preloadRequested=original_content_services::RequestAttachment(lifecycle.component);
    if(!lifecycle.preloadRequested)return LifecycleFail(1861,"original_scope_attachment_preload_request_failed");
    ++lifecycle.generation;lifecycle.stage="original_scope_waiting_for_original_reflex_mesh";return true;
}
inline bool Initialize(void* world,void* pawn,void* weapon){
    if(lifecycle.initialized)return lifecycle.weapon==weapon;
    if(!Probe(world,pawn,weapon))return false;
    if(weapon!=original_weapon::weapon||pawn!=original_weapon::pawn||!original_weapon::State().ready)
        return LifecycleFail(1862,"original_scope_requires_original_equipped_ak");
    if(engine::function<int(*)(void*)>(0x3efc820)(weapon)!=0)
        return LifecycleFail(1863,"original_scope_local_factory_requires_standalone");
    lifecycle.world=world;lifecycle.pawn=pawn;lifecycle.weapon=weapon;
    if(!SelectOriginalAttachment()||!CreateOriginalAttachment())return false;
    lifecycle.initialized=true;lifecycle.error=0;return true;
}
inline bool OnOriginalSkinChange(){
    if(!lifecycle.initialized)return true;
    if(lifecycle.skin==Read<void*>(lifecycle.weapon,0xfb0)&&lifecycle.level==Read<void*>(lifecycle.weapon,0xfb8))return true;
    // Keep the original destruction/registration and Blueprint mesh lifecycle.
    // A new skin cannot silently retain the previous skin's reflex component.
    void* owner=lifecycle.weapon;
    if(lifecycle.component&&!Event(lifecycle.component,L"/Script/Engine.ActorComponent.K2_DestroyComponent",8,&owner))
        return LifecycleFail(1864,"original_scope_skin_change_component_destroy_failed");
    lifecycle.component=nullptr;lifecycle.preloadRequested=false;lifecycle.assetsLoaded=false;lifecycle.meshReady=false;
    lifecycle.catchupAttempted=false;lifecycle.scopeReady=false;lifecycle.animationConsumed=false;
    ++lifecycle.skinChanges;
    return SelectOriginalAttachment()&&CreateOriginalAttachment();
}
inline bool Update(void* world,void* pawn,void* weapon){
    if(!engine::image||original_players::gameThread!=GetCurrentThreadId())
        return LifecycleFail(1865,"original_scope_update_wrong_game_thread");
    if(lifecycle.error)return false;
    if(!world||!pawn||!weapon||!original_weapon::State().ready){
        lifecycle.stage="original_scope_waiting_for_original_equipped_ak";return true;
    }
    // The champion skin's actual original asynchronous completion precedes the
    // attachment, so GetFirstPersonCosmeticMesh chooses the real champion mesh.
    if(Read<void*>(weapon,0xfb0)==original_champions::skinData&&original_champions::skinData
        &&!original_champions::State().skin_assets_loaded_callback_complete){
        lifecycle.stage="original_scope_waiting_for_original_skin_assets";return true;
    }
    if(!Initialize(world,pawn,weapon)||!OnOriginalSkinChange())return false;
    lifecycle.assetsLoaded=false;
    for(const auto& entry:original_content_services::attachments)
        if(entry.component==lifecycle.component)lifecycle.assetsLoaded=entry.callbackCompleted>0
            &&entry.callbackCompleted==entry.callbackEntered&&(entry.lastAssetTypes&7)==7;
    lifecycle.mesh1P=Read<void*>(lifecycle.component,static_cast<size_t>(lifecycle.meshOffset));
    lifecycle.parent1P=Read<void*>(lifecycle.component,static_cast<size_t>(lifecycle.parentOffset));
    lifecycle.meshReady=original_players::IsA(lifecycle.mesh1P,0x385f750)
        &&original_players::IsA(lifecycle.parent1P,0x385f750)
        &&(Read<uint8_t>(lifecycle.component,0xc2)&0x40)!=0;
    if(!lifecycle.assetsLoaded||!lifecycle.meshReady){lifecycle.stage="original_scope_waiting_for_original_reflex_mesh";return true;}
    if(!lifecycle.catchupAttempted){
        if(engine::function<int(*)(void*)>(0x3efc820)(weapon)!=0||!OriginalGraphProfile())
            return LifecycleFail(1866,"original_scope_client_catchup_profile_changed");
        int32_t originalClientEntry=761;lifecycle.catchupAttempted=true;
        if(!Event(lifecycle.component,kReflexGraph,4,&originalClientEntry))
            return LifecycleFail(1867,"original_scope_client_blueprint_catchup_failed");
        ++lifecycle.clientCatchups;
    }
    if(!Probe(world,pawn,weapon))return false;
    lifecycle.scopeReady=state.scope_transform_read&&!state.scope_transform_identity;
    bool same=std::abs(state.scopeTransform.translation.x-state.gunToScope.translation.x)<1e-5
        &&std::abs(state.scopeTransform.translation.y-state.gunToScope.translation.y)<1e-5
        &&std::abs(state.scopeTransform.translation.z-state.gunToScope.translation.z)<1e-5
        &&std::abs(state.scopeTransform.scale.x-state.gunToScope.scale.x)<1e-5
        &&std::abs(state.scopeTransform.scale.y-state.gunToScope.scale.y)<1e-5
        &&std::abs(state.scopeTransform.scale.z-state.gunToScope.scale.z)<1e-5;
    for(unsigned i=0;i<4;++i)same=same&&std::abs(state.scopeTransform.rotation[i]-state.gunToScope.rotation[i])<1e-5;
    lifecycle.animationConsumed=lifecycle.scopeReady&&state.gun_to_scope_read&&!state.gun_to_scope_identity&&same
        &&Read<void*>(state.animationState,0xcf0)==weapon;
    lifecycle.stage=lifecycle.animationConsumed?"original_scope_original_animation_consumed_alignment"
        :lifecycle.scopeReady?"original_scope_ready_waiting_for_original_animation_refresh"
        :"original_scope_client_event_completed_without_alignment";
    if(!lifecycle.scopeReady)return LifecycleFail(1868,lifecycle.stage);
    return true;
}
inline bool Ensure(void* world,void* pawn,void* weapon){return Update(world,pawn,weapon);}
inline void Transform(FILE* f,const engine::Transform& t){
    std::fprintf(f,"{\"translation\":[%.12g,%.12g,%.12g],\"rotation_xyzw\":[%.12g,%.12g,%.12g,%.12g],\"scale\":[%.12g,%.12g,%.12g]}",
      t.translation.x,t.translation.y,t.translation.z,t.rotation[0],t.rotation[1],t.rotation[2],t.rotation[3],t.scale.x,t.scale.y,t.scale.z);
}
inline bool WriteDiagnostics(const wchar_t* path){
    FILE* f=nullptr;if(!path||_wfopen_s(&f,path,L"wb")||!f)return false;
    std::fprintf(f,"{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u,\"samples\":%llu,\"scope_present\":%d,\"reflex_present\":%d,\"champions_reflex_present\":%d,\"scope_class_loaded\":%d,\"reflex_class_loaded\":%d,\"champions_reflex_class_loaded\":%d,\"scope\":\"%p\",\"reflex\":\"%p\",\"champions_reflex\":\"%p\",\"scope_class_bytes\":%d,\"scope_property_offset\":%d,\"scope_layout_matches\":%d,\"scope_transform_read\":%d,\"scope_transform_identity\":%d,\"gun_to_scope_read\":%d,\"gun_to_scope_identity\":%d,\"core_zoomed\":%d,\"core_alt_anims\":%d,\"core_has_equippable\":%d,\"attachment_data_count\":%d,\"scope_transform\":",
      state.stage,state.error,static_cast<unsigned long long>(state.samples),state.scope_present,state.reflex_present,state.champions_reflex_present,
      state.scope_class_loaded,state.reflex_class_loaded,state.champions_reflex_class_loaded,state.scope,state.reflex,state.championsReflex,
      state.scope_class_bytes,state.scope_property_offset,state.scope_layout_matches,state.scope_transform_read,state.scope_transform_identity,
      state.gun_to_scope_read,state.gun_to_scope_identity,state.core_zoomed,state.core_alt_anims,state.core_has_equippable,state.attachment_data_count);
    Transform(f,state.scopeTransform);std::fputs(",\"gun_to_scope_transform\":",f);Transform(f,state.gunToScope);
    std::fprintf(f,",\"factory_stage\":\"%s\",\"factory_error\":%u,\"factory_initialized\":%d,\"factory_generation\":%u,\"attachments_created\":%u,\"client_scope_catchups\":%u,\"skin_changes\":%u,\"attachment_data_asset\":\"%p\",\"selected_attachment_class\":\"%p\",\"attachment_component\":\"%p\",\"reflex_mesh1p\":\"%p\",\"reflex_parent1p\":\"%p\",\"reflex_mesh_offset\":%d,\"reflex_parent_offset\":%d,\"skin_override_count\":%d,\"original_graph_metadata_valid\":%d,\"preload_requested\":%d,\"original_assets_callback_completed\":%d,\"original_mesh_ready\":%d,\"scope_ready\":%d,\"animation_consumed\":%d",
      lifecycle.stage,lifecycle.error,lifecycle.initialized,lifecycle.generation,lifecycle.created,lifecycle.clientCatchups,lifecycle.skinChanges,
      lifecycle.data,lifecycle.selectedType,lifecycle.component,lifecycle.mesh1P,lifecycle.parent1P,lifecycle.meshOffset,lifecycle.parentOffset,
      lifecycle.skinOverrideCount,lifecycle.metadataValid,lifecycle.preloadRequested,lifecycle.assetsLoaded,lifecycle.meshReady,lifecycle.scopeReady,lifecycle.animationConsumed);
    std::fputs(",\"probe_readonly\":true,\"diagnostic_socket_or_bone_getters_called\":false,\"manual_offset_or_fov_written\":false}\n",f);std::fclose(f);return true;
}
}
