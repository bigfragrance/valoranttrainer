#pragma once
#include "original_weapon.h"
#include "original_content_services.h"
#include "original_effects.h"

// Original Champions 2021 attachment flow. Never applies substitute materials
// or manually claims assets loaded. The original attachment must preload and
// execute its original Blueprint, including FXC and animation overrides.
namespace original_champions {
inline constexpr const wchar_t* kSkin = L"/Game/Equippables/Guns/Rifles/AK/Champions/AK_Champions_PrimaryAsset.AK_Champions_PrimaryAsset_C";
inline constexpr const wchar_t* kLevel = L"/Game/Equippables/Guns/Rifles/AK/Champions/Levels/AK_Champions_Lv4_PrimaryAsset.AK_Champions_Lv4_PrimaryAsset_C";
inline constexpr const wchar_t* kChroma = L"/Game/Equippables/Guns/Rifles/AK/Champions/Chromas/Standard/AK_Champions_Standard_PrimaryAsset.AK_Champions_Standard_PrimaryAsset_C";
inline constexpr const wchar_t* kAttachment = L"/Game/Equippables/Guns/Rifles/AK/Champions/Levels/AK_Champions_Lv4_Skin.AK_Champions_Lv4_Skin_C";
inline constexpr const wchar_t* kChromaAttachment = L"/Game/Equippables/Guns/Rifles/AK/Champions/Chromas/Standard/AK_Champions_Standard_Chroma.AK_Champions_Standard_Chroma_C";
inline constexpr const wchar_t* kBaseFire = L"/Game/Equippables/Guns/Rifles/AK/FXC_AK_Fire.FXC_AK_Fire_C";
inline constexpr const wchar_t* kChampionFire = L"/Game/Equippables/Guns/Rifles/AK/Champions/FXC/FXC_AK_Champions_Fire.FXC_AK_Champions_Fire_C";
inline constexpr const wchar_t* kOriginalEquipEvent = L"/Game/Equippables/Guns/Rifles/AK/Champions/Levels/AK_Champions_Lv4_Skin.AK_Champions_Lv4_Skin_C.CustomEvent";
inline constexpr const wchar_t* kOriginalSkinBegin = L"/Game/Equippables/Guns/Rifles/AK/Champions/Levels/AK_Champions_Lv4_Skin.AK_Champions_Lv4_Skin_C.ReceiveBeginPlay";
struct Diagnostics {
    bool assets_loaded=false, preload_manager_present=false, inventory_manager_present=false, requested=false;
    bool skin_preload_requested=false, chroma_preload_requested=false;
    bool skin_component_created=false, chroma_component_created=false, original_fire_override_observed=false;
    uint64_t override_queries=0, override_query_completed=0;
    bool skin_observer_installed=false,skin_native_begun=false,skin_assets_loaded_callback_complete=false;
    bool equip_event_metadata_valid=false,equip_owner_valid=false,equip_current_native=false;
    uint32_t equip_function_flags=0;
    uint16_t equip_function_size=0;
    bool equip_function_outer_valid=false;
    bool equip_catchup_requested=false,equip_catchup_suppressed_by_original=false;
    uint32_t skin_native_begin_entered=0,skin_native_begin_completed=0;
    uint32_t skin_blueprint_begin_entered=0,skin_blueprint_begin_completed=0;
    uint32_t original_equip_entered=0,original_equip_completed=0;
    uint32_t equip_catchup_entered=0,equip_catchup_completed=0,equip_catchup_successes=0;
    const char* equip_catchup_stage="original_champions_skin_not_observed";
    unsigned error=0;
    const char* stage="champions_not_diagnosed";
};
inline Diagnostics state;
inline const Diagnostics& State(){return state;}
inline void *skinData=nullptr,*levelData=nullptr,*chromaData=nullptr,*skinClass=nullptr,*chromaClass=nullptr;
inline void *skinComponent=nullptr,*chromaComponent=nullptr,*originalFire=nullptr,*championFire=nullptr;
inline void* lastFireOverride=nullptr;
struct SkinObservation {
    void* component=nullptr;
    void* weapon=nullptr;
    void* equipFunction=nullptr;
    void* beginFunction=nullptr;
    void** originalPreloadTable=nullptr;
    void** observedTable=nullptr;
    original_content_services::ProcessEvent previousEvent=nullptr;
    original_effects::ComponentEvent previousBegin=nullptr;
    original_effects::WeakIdentity componentIdentity{},weaponIdentity{};
    bool dispatchingCatchup=false;
};
inline SkinObservation skinObservation;
inline void* LoadClass(uintptr_t base,const wchar_t* path){
    return engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t,void*)>(0x1d0c6e0)
      (engine::Class(base),nullptr,path,nullptr,0,nullptr);
}
inline bool Fail(unsigned code,const char* stage){state.error=code;state.stage=stage;return false;}
inline bool EquipMetadataValid(){
    auto fn=skinObservation.equipFunction;
    // Original cooked CustomEvent has no parameters and function flags0c000000.
    // Pinned ProcessEvent1cdc850 reads flags+b8 and ParmsSize+be.
    state.equip_function_flags=original_weapon::Read<uint32_t>(fn,0xb8);
    state.equip_function_size=original_weapon::Read<uint16_t>(fn,0xbe);
    state.equip_function_outer_valid=fn&&original_weapon::Read<void*>(fn,0x28)==skinClass;
    state.equip_event_metadata_valid=state.equip_function_outer_valid
        &&state.equip_function_flags==0x0c000000&&state.equip_function_size==0;
    return state.equip_event_metadata_valid;
}
inline void ObserveSkinBegin(void* object){
    const bool observed=object==skinObservation.component;
    if(observed)++state.skin_native_begin_entered;
    auto original=skinObservation.previousBegin?skinObservation.previousBegin:
        engine::function<original_effects::ComponentEvent>(0x710a6a0);
    original(object);
    if(observed)++state.skin_native_begin_completed;
}
inline void ObserveSkinEvent(void* object,void* function,void* parameters){
    const bool observed=object==skinObservation.component;
    const bool begin=observed&&function==skinObservation.beginFunction;
    const bool equip=observed&&function==skinObservation.equipFunction;
    const bool catchup=equip&&skinObservation.dispatchingCatchup;
    if(begin)++state.skin_blueprint_begin_entered;
    if(equip){if(catchup)++state.equip_catchup_entered;else ++state.original_equip_entered;}
    // Preserve the existing full preload observer. It forwards the original
    // ProcessEvent and counts the REAL K2_OnAssetsLoaded completion itself.
    auto original=skinObservation.previousEvent?skinObservation.previousEvent:
        engine::function<original_content_services::ProcessEvent>(0x1cdc850);
    original(object,function,parameters);
    if(begin)++state.skin_blueprint_begin_completed;
    if(equip){if(catchup)++state.equip_catchup_completed;else ++state.original_equip_completed;}
}
inline bool ObserveNewSkin(void* component){
    if(!original_weapon::OnThread()||skinObservation.component)
        return Fail(827,"original_champions_new_skin_observer_already_installed_or_wrong_thread");
    if(original_weapon::Read<void*>(component,0x10)!=skinClass
        ||original_weapon::Read<void*>(component,0x28)!=original_weapon::weapon
        ||original_weapon::Read<void*>(component,0xf8)!=levelData)
        return Fail(828,"original_champions_new_lv4_component_identity_mismatch");
    auto preload=original_content_services::ObserveAttachment(component);
    if(!preload||preload->nativeTableRva!=0xb114e20||preload->nativeTableEntries!=157)
        return Fail(829,"original_champions_requires_full_original_skin_preload_observer");
    auto source=original_weapon::Read<void**>(component,0);
    if(source!=preload->observedTable
        ||source[0x298/8]!=reinterpret_cast<void*>(&original_content_services::ObserveAttachmentEvent)
        ||source[0x3e0/8]!=reinterpret_cast<void*>(engine::image+0x710a6a0)
        ||source[156]!=reinterpret_cast<void*>(engine::image+0x6c8e950))
        return Fail(830,"original_champions_skin_preload_observer_profile_changed");
    skinObservation.component=component;skinObservation.weapon=original_weapon::weapon;
    skinObservation.equipFunction=engine::Find(kOriginalEquipEvent);
    skinObservation.beginFunction=engine::Find(kOriginalSkinBegin);
    if(!EquipMetadataValid()||!skinObservation.beginFunction
        ||original_weapon::Read<void*>(skinObservation.beginFunction,0x28)!=skinClass
        ||original_weapon::Read<uint16_t>(skinObservation.beginFunction,0xbe)!=0)
        return Fail(831,"original_champions_original_lv4_equip_event_metadata_mismatch");
    if(!original_effects::CaptureIdentity(component,skinObservation.componentIdentity)
        ||!original_effects::CaptureIdentity(skinObservation.weapon,skinObservation.weaponIdentity))
        return Fail(832,"original_champions_skin_or_weapon_weak_identity_unavailable");
    constexpr size_t bytes=(157+1)*sizeof(void*);
    auto allocation=static_cast<void**>(VirtualAlloc(nullptr,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!allocation)return Fail(833,"original_champions_skin_observer_allocation_failed");
    // All157 primary entries plus preceding compiler word; no shortened base
    // table and no change to the native preload interface+d8.
    std::memcpy(allocation,source-1,bytes);
    auto copied=allocation+1;
    skinObservation.originalPreloadTable=source;skinObservation.observedTable=copied;
    skinObservation.previousEvent=reinterpret_cast<original_content_services::ProcessEvent>(source[0x298/8]);
    skinObservation.previousBegin=reinterpret_cast<original_effects::ComponentEvent>(source[0x3e0/8]);
    copied[0x298/8]=reinterpret_cast<void*>(&ObserveSkinEvent);
    copied[0x3e0/8]=reinterpret_cast<void*>(&ObserveSkinBegin);
    DWORD previous=0;
    if(!VirtualProtect(allocation,bytes,PAGE_READONLY,&previous)){
        VirtualFree(allocation,0,MEM_RELEASE);return Fail(834,"original_champions_skin_observer_protection_failed");
    }
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(component),copied,source)!=source){
        VirtualFree(allocation,0,MEM_RELEASE);return Fail(835,"original_champions_skin_observer_install_race");
    }
    state.skin_observer_installed=true;state.equip_catchup_stage="waiting_for_original_skin_begin_and_asset_callback";
    return true;
}
inline bool CatchUpOriginalEquip(){
    if(!original_weapon::OnThread())return Fail(836,"original_champions_equip_catchup_wrong_thread");
    if(!state.skin_observer_installed)return true;
    if(state.equip_catchup_requested)return true;
    if(state.original_equip_entered){
        state.equip_catchup_suppressed_by_original=true;
        state.equip_catchup_stage="original_skin_equip_event_already_observed";return true;
    }
    if(original_effects::ResolveIdentity(skinObservation.componentIdentity)!=skinObservation.component
        ||original_effects::ResolveIdentity(skinObservation.weaponIdentity)!=skinObservation.weapon)
        return Fail(837,"original_champions_equip_catchup_original_object_expired");
    state.skin_native_begun=state.skin_native_begin_completed>0
        &&state.skin_native_begin_entered==state.skin_native_begin_completed
        &&state.skin_blueprint_begin_completed>0
        &&(original_weapon::Read<uint8_t>(skinObservation.component,0xc2)&0x40)!=0;
    state.skin_assets_loaded_callback_complete=false;
    for(const auto& attachment:original_content_services::attachments)
        if(attachment.component==skinObservation.component)
            state.skin_assets_loaded_callback_complete=attachment.callbackCompleted>0
                &&attachment.callbackEntered==attachment.callbackCompleted&&(attachment.lastAssetTypes&7)==7;
    if(!state.skin_native_begun||!state.skin_assets_loaded_callback_complete){
        state.equip_catchup_stage="waiting_for_original_skin_begin_and_asset_callback";return true;
    }
    auto weapon=skinObservation.weapon;
    state.equip_owner_valid=weapon==original_weapon::weapon
        &&original_weapon::Read<void*>(skinObservation.component,0x10)==skinClass
        &&original_weapon::Read<void*>(skinObservation.component,0x28)==weapon
        &&original_weapon::Read<void*>(skinObservation.component,0xc8)==weapon
        &&original_weapon::Read<void*>(skinObservation.component,0xf8)==levelData
        &&original_weapon::Read<void*>(weapon,0x210)==original_weapon::pawn
        &&original_weapon::Read<void*>(weapon,0x4a8)==original_weapon::pawn;
    auto query=engine::Find(L"/Script/ShooterGame.AresEquippable.IsCurrentEquippable");
    if(!query||original_weapon::Read<uint16_t>(query,0xbe)!=1)
        return Fail(838,"original_champions_current_equippable_query_abi_mismatch");
    uint8_t current=0;
    auto weaponTable=original_weapon::Read<void**>(weapon,0);
    reinterpret_cast<original_content_services::ProcessEvent>(weaponTable[0x298/8])(weapon,query,&current);
    state.equip_current_native=current!=0;
    if(!state.equip_owner_valid||!state.equip_current_native||!original_weapon::State().ready){
        state.equip_catchup_stage="waiting_for_original_current_owned_equipped_gun";return true;
    }
    if(!EquipMetadataValid())return Fail(831,"original_champions_original_lv4_equip_event_metadata_mismatch");
    if(original_weapon::Read<void**>(skinObservation.component,0)!=skinObservation.observedTable)
        return Fail(839,"original_champions_skin_observer_replaced_before_equip_catchup");
    // Lv4 BeginPlay only subscribes to OnItemEquipped. This attachment was
    // added after the real gun was already equipped. Deliver that one missed
    // listener notification through its EXACT original zero-argument function.
    // Its Blueprint alone selects/spawns the original Champion FXC and applies
    // its ranking/material/timeline rules. Never call PlayEffect or StartEffect.
    state.equip_catchup_requested=true;skinObservation.dispatchingCatchup=true;
    state.equip_catchup_stage="dispatching_original_skin_missed_equip_listener";
    auto table=original_weapon::Read<void**>(skinObservation.component,0);
    reinterpret_cast<original_content_services::ProcessEvent>(table[0x298/8])
        (skinObservation.component,skinObservation.equipFunction,nullptr);
    skinObservation.dispatchingCatchup=false;
    if(state.equip_catchup_entered!=1||state.equip_catchup_completed!=1)
        return Fail(840,"original_champions_original_equip_listener_did_not_complete_once");
    ++state.equip_catchup_successes;
    state.equip_catchup_stage="original_skin_missed_equip_listener_completed_once";
    return true;
}
inline bool Diagnose(){
    if(!original_weapon::OnThread())return Fail(821,"champions_wrong_or_missing_game_thread");
    auto skin=LoadClass(0x676d6b0,kSkin),level=LoadClass(0x676f1b0,kLevel),chroma=LoadClass(0x676eaa0,kChroma);
    skinClass=LoadClass(0x6574f50,kAttachment);chromaClass=LoadClass(0x662e900,kChromaAttachment);
    originalFire=LoadClass(0x1aed280,kBaseFire);championFire=LoadClass(0x1aed280,kChampionFire);
    skinData=skin?engine::DefaultObject(skin):nullptr;levelData=level?engine::DefaultObject(level):nullptr;
    chromaData=chroma?engine::DefaultObject(chroma):nullptr;
    state.assets_loaded=skinData&&levelData&&chromaData&&skinClass&&chromaClass&&originalFire&&championFire;
    if(!state.assets_loaded)return Fail(822,"original_champions_attachment_assets_missing");
    auto clientGI=engine::function<void*(*)(void*)>(0x703f040)(original_weapon::world);
    auto preload=clientGI?original_weapon::Read<void*>(clientGI,0x3f0):nullptr;
    auto inventory=clientGI?original_weapon::Read<void*>(clientGI,0x448):nullptr;
    state.preload_manager_present=original_players::IsA(preload,0x6aa5740);
    state.inventory_manager_present=original_players::IsA(inventory,0x68e4630);
    // ClientGI+448 is InventoryManager (the attachment's UI-model lookup),
    // not PreloadManager. The actual typed preload service is ClientGI+3f0.
    // Construction alone still does not prove an assets-loaded callback.
    state.error=0;state.stage=state.preload_manager_present&&state.inventory_manager_present?
        "original_champions_content_services_present":"original_champions_requires_content_services";
    return state.preload_manager_present&&state.inventory_manager_present;
}
inline void* AddAttachment(void* type,void* data){
    struct alignas(16) Add { void* type; uint8_t manual=0,pad[7]{};engine::Transform transform;
        uint8_t deferred=1,pad2[7]{};void* result=nullptr; } add{type};
    static_assert(sizeof(Add)==128);
    auto weapon=original_weapon::weapon;
    if(!engine::Event(weapon,L"/Script/Engine.Actor.AddComponentByClass",&add)||!add.result)return nullptr;
    // Original reflected AresWeaponAttachmentComponent.DataAsset, assigned before
    // normal FinishAddComponent invokes registration and BeginPlay/preloading.
    original_weapon::Put(add.result,0xf8,data);
    if(type==skinClass&&!ObserveNewSkin(add.result))return nullptr;
    struct alignas(16) Finish {void* component;uint8_t manual=0,pad[7]{};engine::Transform transform;} finish{add.result};
    static_assert(sizeof(Finish)==112);
    if(!engine::Event(weapon,L"/Script/Engine.Actor.FinishAddComponent",&finish))return nullptr;
    return add.result;
}
inline bool Apply(){
    if(state.requested)return Fail(823,"champions_already_requested_no_duplicate_components");
    original_weapon::RefreshDiagnostics();
    if(!original_weapon::State().ready)return Fail(824,"champions_requires_original_equipped_ak");
    if(!Diagnose())return false;
    // Local loadout identity for the real original weapon, not material changes.
    original_weapon::Put(original_weapon::weapon,0xfb0,skinData);
    original_weapon::Put(original_weapon::weapon,0xfb8,levelData);
    original_weapon::Put(original_weapon::weapon,0xfd0,chromaData);
    state.requested=true;state.stage="creating_original_champions_skin_attachment";
    skinComponent=AddAttachment(skinClass,levelData);state.skin_component_created=skinComponent!=nullptr;
    if(!skinComponent)return state.error?false:Fail(825,"original_champions_skin_component_creation_failed");
    state.skin_preload_requested=original_content_services::RequestAttachment(skinComponent);
    if(!state.skin_preload_requested)return Fail(original_content_services::State().error,original_content_services::State().stage);
    state.stage="creating_original_champions_chroma_attachment";
    chromaComponent=AddAttachment(chromaClass,chromaData);state.chroma_component_created=chromaComponent!=nullptr;
    if(!chromaComponent)return Fail(826,"original_champions_chroma_component_creation_failed");
    state.chroma_preload_requested=original_content_services::RequestAttachment(chromaComponent);
    if(!state.chroma_preload_requested)return Fail(original_content_services::State().error,original_content_services::State().stage);
    state.error=0;state.stage="waiting_for_original_champions_asset_callbacks";return true;
}
inline void Poll(){
    if(!state.requested||!original_weapon::OnThread())return;
    if(!CatchUpOriginalEquip())return;
    struct Params {void* effect;void* result=nullptr;} p{originalFire};
    ++state.override_queries;
    if(engine::Event(original_weapon::weapon,L"/Script/ShooterGame.AresEquippable.GetOverridableEffect",&p)){
      ++state.override_query_completed;lastFireOverride=p.result;
      state.original_fire_override_observed=p.result==championFire;
      if(state.original_fire_override_observed)state.stage="original_champions_fire_override_observed";
    }
    // This verifies one original override, not all effects or champion ranking.
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path||!*path)return false;
    std::wstring temporary(path);temporary+=L".tmp";
    FILE* file=nullptr;if(_wfopen_s(&file,temporary.c_str(),L"wb")||!file)return false;
    const auto boolean=[](bool value){return value?"true":"false";};
    std::fprintf(file,
        "{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u,\"assets_loaded\":%s,"
        "\"preload_manager_present\":%s,\"inventory_manager_present\":%s,\"requested\":%s,"
        "\"skin_component_created\":%s,\"chroma_component_created\":%s,"
        "\"skin_preload_requested\":%s,\"chroma_preload_requested\":%s,"
        "\"original_fire_override_observed\":%s,\"override_queries\":%llu,\"override_query_completed\":%llu,"
        "\"skin_data\":\"%p\",\"level_data\":\"%p\",\"chroma_data\":\"%p\",\"skin_component\":\"%p\","
        "\"chroma_component\":\"%p\",\"base_fire_class\":\"%p\",\"champions_fire_class\":\"%p\",\"last_fire_override\":\"%p\"",
        state.stage,state.error,boolean(state.assets_loaded),boolean(state.preload_manager_present),
        boolean(state.inventory_manager_present),boolean(state.requested),boolean(state.skin_component_created),
        boolean(state.chroma_component_created),boolean(state.skin_preload_requested),boolean(state.chroma_preload_requested),
        boolean(state.original_fire_override_observed),static_cast<unsigned long long>(state.override_queries),
        static_cast<unsigned long long>(state.override_query_completed),skinData,levelData,chromaData,skinComponent,
        chromaComponent,originalFire,championFire,lastFireOverride);
    std::fprintf(file,
        ",\"skin_observer_installed\":%s,\"skin_native_begun\":%s,\"skin_assets_loaded_callback_complete\":%s,"
        "\"equip_event_metadata_valid\":%s,\"equip_function_flags\":\"0x%x\",\"equip_function_size\":%u,\"equip_function_outer_valid\":%s,"
        "\"equip_owner_valid\":%s,\"equip_current_native\":%s,\"equip_catchup_requested\":%s,"
        "\"equip_catchup_suppressed_by_original\":%s,\"skin_native_begin_entered\":%u,\"skin_native_begin_completed\":%u,"
        "\"skin_blueprint_begin_entered\":%u,\"skin_blueprint_begin_completed\":%u,"
        "\"original_equip_entered\":%u,\"original_equip_completed\":%u,\"equip_catchup_entered\":%u,"
        "\"equip_catchup_completed\":%u,\"equip_catchup_successes\":%u,\"equip_catchup_stage\":\"%s\","
        "\"equip_event_function\":\"%p\",\"observed_skin_component\":\"%p\",\"observed_skin_weapon\":\"%p\","
        "\"skin_observer_native_entries\":157}\n",
        boolean(state.skin_observer_installed),boolean(state.skin_native_begun),boolean(state.skin_assets_loaded_callback_complete),
        boolean(state.equip_event_metadata_valid),state.equip_function_flags,static_cast<unsigned>(state.equip_function_size),
        boolean(state.equip_function_outer_valid),boolean(state.equip_owner_valid),boolean(state.equip_current_native),
        boolean(state.equip_catchup_requested),boolean(state.equip_catchup_suppressed_by_original),
        state.skin_native_begin_entered,state.skin_native_begin_completed,state.skin_blueprint_begin_entered,
        state.skin_blueprint_begin_completed,state.original_equip_entered,state.original_equip_completed,
        state.equip_catchup_entered,state.equip_catchup_completed,state.equip_catchup_successes,state.equip_catchup_stage,
        skinObservation.equipFunction,skinObservation.component,skinObservation.weapon);
    const bool closed=std::fclose(file)==0;
    return closed&&MoveFileExW(temporary.c_str(),path,MOVEFILE_REPLACE_EXISTING);
}
}
