#pragma once
#include "original_damage.h"
#include "original_inventory_slots.h"
#include "original_effects.h"
#include <array>
#include <cmath>

// Missing local inventory authority only. The real HeavyArmor actor/SCS owns
// its real AttachedDamageSection. Original attach/detach, HUD augmentation,
// native section queries and damage-response replication remain unchanged.
// Static evidence: work/original-armor-runtime/grant-evidence.json and
// reports/original-armor-native-readonly.json. No guessed absorption fraction.
namespace original_armor {
inline constexpr const wchar_t* HeavyClassPath=L"/Game/Gear/HeavyArmorItem.HeavyArmorItem_C";
inline constexpr uintptr_t ItemClass=0x645c570,AttachedSectionClass=0x6588970;
inline constexpr uintptr_t OriginalOwnerCallback=0x7100e20;
inline constexpr size_t MaxPlayers=original_players::MaxPlayers;
using WeakIdentity=original_inventory_slots::WeakIdentity;
struct Entry {
    bool attempted=false,bound=false,granted=false,attached=false,clientNotified=false,readback=false;
    void *pawn=nullptr,*inventory=nullptr,*slot=nullptr,*item=nullptr,*section=nullptr,*damageable=nullptr;
    WeakIdentity pawnIdentity{},itemIdentity{},sectionIdentity{};
    original_damage::SectionAudit sectionAudit;
    uint8_t nativeSlot=255;
    float life=0,maximum=0,damageScale=0,nativeShield=0,nativeMaxShield=0,rawShield=0;
};
struct Diagnostics {
    bool initialized=false,restartRequired=false;
    unsigned error=0,grants=0,retired=0,expired=0,readbacks=0,authorityNotifications=0,clientNotifications=0;
    uintptr_t exceptionRva=0;
    const char* stage="original_armor_not_initialized";
};
inline Diagnostics diagnostics;
inline std::array<Entry,MaxPlayers> entries{};
inline void *world=nullptr,*heavyClass=nullptr;
inline DWORD gameThread=0;
inline const Diagnostics& State(){return diagnostics;}
inline const Entry* Armor(size_t index){return index<entries.size()?&entries[index]:nullptr;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Fail(unsigned code,const char* text){diagnostics.error=code;diagnostics.stage=text;return false;}
inline bool Thread(){return diagnostics.initialized&&!diagnostics.restartRequired&&gameThread==GetCurrentThreadId()&&original_players::Thread();}
inline bool Event(void* object,const wchar_t* name,uint16_t size,void* params){return original_damage::Event(object,name,size,params);}
inline bool Initialize(void* originalWorld){
    if(diagnostics.initialized)return world==originalWorld&&Thread();
    if(!originalWorld||!original_players::Thread())return Fail(2500,"original_armor_world_or_thread_missing");
    heavyClass=engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t,void*)>(0x1d0c6e0)
        (engine::Class(ItemClass),nullptr,HeavyClassPath,nullptr,0,nullptr);
    auto defaults=heavyClass?engine::DefaultObject(heavyClass):nullptr;
    uint8_t slot=255,equippable=1;
    if(!original_players::IsA(defaults,ItemClass)
       ||!Event(defaults,L"/Script/ShooterGame.AresItem.GetItemSlot",1,&slot)||slot>=17
       ||!Event(defaults,L"/Script/ShooterGame.AresItem.IsEquippable",1,&equippable)||equippable)
        return Fail(2501,"original_heavy_armor_cdo_item_slot_not_verified");
    world=originalWorld;gameThread=GetCurrentThreadId();diagnostics.initialized=true;
    diagnostics.error=0;diagnostics.stage="original_heavy_armor_factory_ready";return true;
}
inline bool RegisteredSection(const Entry& e){
    auto sections=Read<original_damage::Array<void*>>(e.damageable,0x1e8);
    if(sections.count<0||sections.count>32||sections.capacity<sections.count
       ||(sections.count&&!original_players::Readable(sections.data,static_cast<size_t>(sections.count)*sizeof(void*))))return false;
    unsigned found=0;for(int32_t i=0;i<sections.count;++i)if(sections.data[i]==e.section)++found;
    return found==1&&Read<void*>(e.section,0x110)==e.damageable;
}
inline bool SampleImpl(size_t index){
    if(!Thread()||index>=entries.size()||!entries[index].bound)return false;
    auto& e=entries[index];
    if(original_inventory_slots::ResolveIdentity(e.pawnIdentity)!=e.pawn
       ||original_inventory_slots::ResolveIdentity(e.itemIdentity)!=e.item
       ||original_inventory_slots::ResolveIdentity(e.sectionIdentity)!=e.section)
        return Fail(2502,"original_armor_live_weak_identity_changed");
    e.life=Read<float>(e.section,0x124);e.maximum=Read<float>(e.section,0x128);e.damageScale=Read<float>(e.section,0x144);
    e.attached=RegisteredSection(e);e.readback=false;
    if(!original_damage::DescribeSections(e.pawn,e.damageable,e.sectionAudit))
        return Fail(2528,"original_armor_native_section_audit_failed");
    unsigned effectiveOwned=0;
    for(int32_t i=0;i<e.sectionAudit.count;++i){const auto& row=e.sectionAudit.entries[i];
        if(row.type==1&&!row.emptyShield&&row.object==e.section)++effectiveOwned;
    }
    e.sectionAudit.selectedShield=e.section;
    if(e.sectionAudit.effectiveShields!=1||effectiveOwned!=1)
        return Fail(2529,"original_armor_requires_exactly_one_effective_owned_shield_section");
    if(!e.attached||Read<uint8_t>(e.section,0x118)!=1
       ||!std::isfinite(e.life)||!std::isfinite(e.maximum)||!std::isfinite(e.damageScale)
       ||e.life<0||e.maximum<=0||e.life>e.maximum+.01f||e.damageScale<=0
       ||Read<void*>(e.item,0x4a8)!=e.pawn||Read<void*>(e.slot,0x38)!=e.item
       ||!Event(e.pawn,L"/Script/ShooterGame.ShooterCharacter.GetShield",4,&e.nativeShield)
       ||!Event(e.pawn,L"/Script/ShooterGame.ShooterCharacter.GetMaxShield",4,&e.nativeMaxShield)
       ||!Event(e.damageable,L"/Script/ShooterGame.DamageableComponent.GetRawShield",4,&e.rawShield)
       ||!std::isfinite(e.nativeShield)||!std::isfinite(e.nativeMaxShield)||!std::isfinite(e.rawShield)
       ||e.nativeShield<0||e.nativeMaxShield<=0||std::abs(e.rawShield-e.life)>.01f
       ||std::abs(e.nativeMaxShield-e.maximum)>.01f)
        return Fail(2503,"original_armor_section_or_native_shield_readback_failed");
    // GetRawShield is the original aggregate without temporary-damage display
    // subtraction. Fractional life is valid; these getters return native floats.
    e.readback=true;++diagnostics.readbacks;return true;
}
inline bool GrantHeavyImpl(size_t index,void* pawn){
    if(!Thread()||index>=entries.size()||!original_players::IsA(pawn,original_players::ShooterCharacterClass))
        return Fail(2504,"original_armor_requires_original_registered_pawn");
    auto& e=entries[index];
    if(e.bound)return e.pawn==pawn?SampleImpl(index):Fail(2505,"original_armor_old_pawn_must_be_retired");
    if(e.attempted)return Fail(2506,"original_armor_partial_grant_requires_new_session");
    const auto& roster=original_players::players[index];
    if(!roster.ready||!roster.registered||roster.character!=pawn||!original_players::Registered(roster.state)
       ||engine::function<int(*)(void*)>(0x3efc820)(pawn)!=0)
        return Fail(2507,"original_armor_requires_standalone_original_roster");
    if(!original_inventory_slots::Ensure(pawn))return Fail(2508,"original_armor_inventory_slots_not_ready");
    e.attempted=true;e.pawn=pawn;e.inventory=Read<void*>(pawn,0xc08);e.damageable=Read<void*>(pawn,0xc70);
    if(!original_players::IsA(e.inventory,original_inventory_slots::InventoryClass)||Read<void*>(e.inventory,0x2f0)!=pawn
       ||!original_players::IsA(e.damageable,original_damage::DamageableClass)||Read<void*>(e.damageable,0xc8)!=pawn
       ||!original_inventory_slots::CaptureIdentity(pawn,e.pawnIdentity))
        return Fail(2509,"original_armor_native_inventory_damageable_owner_invalid");
    auto defaults=engine::DefaultObject(heavyClass);
    if(!Event(defaults,L"/Script/ShooterGame.AresItem.GetItemSlot",1,&e.nativeSlot)||e.nativeSlot>=17)
        return Fail(2510,"original_armor_native_item_slot_missing");
    e.slot=Read<void*>(e.inventory,0x1b8+e.nativeSlot*8);
    if(!original_players::IsA(e.slot,original_inventory_slots::ItemSlotClass)||Read<uint8_t>(e.slot,0x30)!=e.nativeSlot
       ||Read<void*>(e.slot,0x40)!=e.inventory||Read<void*>(e.slot,0x38))
        return Fail(2511,"original_armor_requires_empty_original_single_item_slot");
    engine::Transform transform;
    if(!Event(pawn,L"/Script/Engine.Actor.K2_GetActorLocation",24,&transform.translation))return Fail(2512,"original_armor_pawn_location_missing");
    e.item=engine::function<void*(*)(void*,void*,const engine::Transform*,uint8_t,void*,uint8_t)>(0x43fada0)
        (world,heavyClass,&transform,1,nullptr,0);
    if(!original_players::IsA(e.item,ItemClass)||!original_inventory_slots::CaptureIdentity(e.item,e.itemIdentity))
        return Fail(2513,"original_heavy_armor_deferred_spawn_failed");
    engine::function<void(*)(void*,const engine::Transform*,bool,void*,uint8_t)>(0x3ef6cb0)(e.item,&transform,false,nullptr,0);
    struct ComponentQuery {void* type;void* result=nullptr;} query{engine::Class(AttachedSectionClass)};
    if(!Event(e.item,L"/Script/Engine.Actor.GetComponentByClass",16,&query)
       ||!original_players::IsA(query.result,AttachedSectionClass))return Fail(2514,"original_heavy_armor_scs_section_missing");
    e.section=query.result;
    if(!original_inventory_slots::CaptureIdentity(e.section,e.sectionIdentity)
       ||!original_effects::EnsureActor(e.item,"original_heavy_armor")
       ||!Event(e.item,L"/Script/Engine.Actor.SetOwner",8,&pawn)||Read<void*>(e.item,0x210)!=pawn)
        return Fail(2515,"original_heavy_armor_native_owner_failed");
    if(Read<void*>(e.item,0x4a8)!=pawn){
        auto callback=Read<void*>(Read<void**>(e.item,0),0x7d8);
        if(callback!=reinterpret_cast<void*>(engine::image+OriginalOwnerCallback))return Fail(2516,"original_armor_item_owner_callback_profile_changed");
        reinterpret_cast<void(*)(void*,void*,void*)>(callback)(e.item,nullptr,pawn);
    }
    if(Read<void*>(e.item,0x4a8)!=pawn)return Fail(2517,"original_armor_typed_owner_not_observed");
    // AuthAddItem/AuthSetOwnerTo are empty in this client. Supply only their
    // missing data, then run the real native/Blueprint notifications once.
    if(!original_players::Write(e.slot,0x38,e.item)||!original_players::Write<uint8_t>(e.item,0x4b0,1)
       ||!Event(e.slot,L"/Script/ShooterGame.ItemSlot.OnRep_Contents",0,nullptr))return Fail(2518,"original_armor_inventory_grant_notification_failed");
    e.granted=true;
    if(!Event(e.item,L"/Game/Gear/BasicArmorItem.BasicArmorItem_C.ReceiveAuthSetOwningCharacter",8,&pawn))
        return Fail(2519,"original_armor_auth_attach_graph_failed");
    ++diagnostics.authorityNotifications;
    if(!Event(e.item,L"/Game/Gear/BasicArmorItem.BasicArmorItem_C.ReceiveClientSetOwningCharacter",8,&pawn))
        return Fail(2520,"original_armor_original_client_hud_graph_failed");
    e.clientNotified=true;++diagnostics.clientNotifications;e.bound=true;
    if(!SampleImpl(index)||e.life<=0||e.nativeShield<=0)return Fail(2521,"original_armor_initial_native_shield_missing");
    ++diagnostics.grants;diagnostics.error=0;diagnostics.stage="original_heavy_armor_granted_native_shield_verified";return true;
}
inline bool RetireImpl(size_t index){
    if(!Thread()||index>=entries.size())return false;
    auto& e=entries[index];if(!e.attempted)return true;
    if(original_inventory_slots::ResolveIdentity(e.itemIdentity)!=e.item){e={};++diagnostics.expired;return true;}
    if(original_inventory_slots::ResolveIdentity(e.pawnIdentity)!=e.pawn)
        return Fail(2522,"original_armor_retire_requires_live_old_pawn_identity");
    auto old=e.pawn;
    if(e.attached&&!Event(e.item,L"/Game/Gear/BasicArmorItem.BasicArmorItem_C.ReceiveAuthClearOwningCharacter",8,&old))
        return Fail(2523,"original_armor_original_detach_graph_failed");
    if(e.clientNotified&&!Event(e.item,L"/Game/Gear/BasicArmorItem.BasicArmorItem_C.ReceiveClientClearOwningCharacter",8,&old))
        return Fail(2524,"original_armor_original_client_clear_graph_failed");
    if(Read<void*>(e.slot,0x38)==e.item){void* empty=nullptr;
        if(!original_players::Write(e.slot,0x38,empty)||!original_players::Write<uint8_t>(e.item,0x4b0,0)
           ||!Event(e.slot,L"/Script/ShooterGame.ItemSlot.OnRep_Contents",0,nullptr))return Fail(2525,"original_armor_slot_retire_notification_failed");
    }
    void* empty=nullptr;
    if(!Event(e.item,L"/Script/Engine.Actor.SetOwner",8,&empty))return Fail(2526,"original_armor_actor_owner_clear_failed");
    if(Read<void*>(e.item,0x4a8)){
        auto callback=Read<void*>(Read<void**>(e.item,0),0x7d8);
        if(callback!=reinterpret_cast<void*>(engine::image+OriginalOwnerCallback))return Fail(2516,"original_armor_item_owner_callback_profile_changed");
        reinterpret_cast<void(*)(void*,void*,void*)>(callback)(e.item,old,nullptr);
    }
    if(Read<void*>(e.item,0x4a8)||!original_effects::RetireActor(e.item)
       ||!Event(e.item,L"/Script/Engine.Actor.K2_DestroyActor",0,nullptr))return Fail(2527,"original_armor_original_item_destroy_failed");
    e={};++diagnostics.retired;return true;
}
inline LONG Capture(EXCEPTION_POINTERS* p){
    diagnostics.error=p&&p->ExceptionRecord?p->ExceptionRecord->ExceptionCode:0xe0002500U;
    const auto address=p&&p->ExceptionRecord?reinterpret_cast<uintptr_t>(p->ExceptionRecord->ExceptionAddress):0;
    diagnostics.exceptionRva=address>=engine::image?address-engine::image:address;
    diagnostics.restartRequired=true;diagnostics.stage="original_armor_native_exception_requires_new_session";return EXCEPTION_EXECUTE_HANDLER;
}
inline bool GrantHeavy(size_t index,void* pawn){__try{return GrantHeavyImpl(index,pawn);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool GrantHeavy(size_t index){return index<entries.size()?GrantHeavy(index,original_players::players[index].character):false;}
inline bool Sample(size_t index){__try{return SampleImpl(index);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool Retire(size_t index){__try{return RetireImpl(index);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    auto b=[](bool v){return v?"true":"false";};
    std::fprintf(f,"{\"schema_version\":1,\"initialized\":%s,\"error\":%u,\"stage\":",b(diagnostics.initialized),diagnostics.error);original_players::Text(f,diagnostics.stage);
    std::fprintf(f,",\"grants\":%u,\"retired\":%u,\"expired\":%u,\"readbacks\":%u,\"authority_notifications\":%u,\"client_notifications\":%u,\"players\":[",diagnostics.grants,diagnostics.retired,diagnostics.expired,diagnostics.readbacks,diagnostics.authorityNotifications,diagnostics.clientNotifications);
    bool first=true;for(size_t i=0;i<entries.size();++i){const auto& e=entries[i];if(!e.attempted)continue;
        std::fprintf(f,"%s{\"index\":%zu,\"bound\":%s,\"native_slot\":%u,\"granted\":%s,\"attached\":%s,\"client_notified\":%s,\"native_readback\":%s,\"section_life\":%.9g,\"section_maximum\":%.9g,\"section_damage_scale\":%.9g,\"shield\":%.9g,\"max_shield\":%.9g,\"raw_shield\":%.9g,\"section_audit\":",first?"":",",i,b(e.bound),static_cast<unsigned>(e.nativeSlot),b(e.granted),b(e.attached),b(e.clientNotified),b(e.readback),e.life,e.maximum,e.damageScale,e.nativeShield,e.nativeMaxShield,e.rawShield);
        original_damage::WriteSectionAudit(f,e.sectionAudit);std::fputc('}',f);first=false;
    }
    std::fputs("],\"native_section_life_written_directly\":false,\"image_pages_written\":false,\"runtime_verified\":false}\n",f);
    const bool ok=std::ferror(f)==0;std::fclose(f);return ok;
}
}
