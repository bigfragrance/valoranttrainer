#pragma once
#include "all_weapons.h"
#include "original_shot_observer.h"

// Game-thread ownership of the legacy weapon singleton during original swaps.
// V2 combat owns all shot/view/effect hooks: legacy shot handling is OFF by
// default and must never be enabled while training_combat_runtime owns them.
namespace weapon_runtime_bindings {
using Definition=all_weapons::Definition;
using AfterBind=bool(*)(void* world,void* pawn,void* weapon,const Definition&);
struct Diagnostics {
    unsigned error=0,generation=0,machineRestores=0,effectRestores=0,originRestores=0,expiredNoWrite=0;
    bool ready=false,quiesced=false,releaseIssued=false,legacyShots=false,legacyPoolTrackingComplete=false;
    bool firearm=false,melee=false,originalInputReady=false;
    void *world=nullptr,*pawn=nullptr,*weapon=nullptr,*inventory=nullptr,*slot=nullptr;
    void *firing=nullptr,*ammo=nullptr,*stability=nullptr,*mesh1p=nullptr,*mesh3p=nullptr,*machine=nullptr;
    void *legacyPawn=nullptr;void** legacyPawnParent=nullptr;
    const char* stage="weapon_runtime_bindings_not_bound";
};
inline Diagnostics state;
inline AfterBind afterBind=nullptr;
inline const Diagnostics& State(){return state;}
inline void SetAfterBind(AfterBind callback){afterBind=callback;}
inline bool Fail(unsigned code,const char* stage){state.error=code;state.stage=stage;state.ready=false;return false;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Thread(){return original_players::gameThread==GetCurrentThreadId()&&engine::image;}
struct TrackedEffect {void* object=nullptr;original_effects::WeakIdentity identity{};};
inline TrackedEffect legacyEffects[256]{};
inline original_effects::WeakIdentity legacyPawnIdentity{};
inline bool CASRestore(void* object,void** owned,void** previous,unsigned& restored){
    if(!object||!owned||!previous)return false;
    auto actual=weapon_runtime_bindings::Read<void**>(object,0);
    if(actual==previous)return true;
    if(actual!=owned)return Fail(2381,"weapon_runtime_refuses_to_overwrite_another_observer_layer");
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(object),previous,owned)!=owned)
        return Fail(2382,"weapon_runtime_observer_restore_race");
    ++restored;return true;
}
inline bool RestoreMachineObservers(){
    const auto count=original_weapon::state.observed_machine_count;
    if(count>original_weapon::MaxObservedMachines)return Fail(2383,"weapon_runtime_machine_record_count_invalid");
    for(unsigned i=0;i<count;++i){
        auto component=reinterpret_cast<void*>(original_weapon::state.machines[i].component);
        auto owned=original_weapon::observedMachineTables[i]+1;
        auto previous=reinterpret_cast<void**>(original_weapon::observedMachineOriginalTables[i]);
        if(!component||!previous)continue;
        original_effects::WeakIdentity id{};
        if(!original_effects::CaptureIdentity(component,id)||original_effects::ResolveIdentity(id)!=component){++state.expiredNoWrite;continue;}
        if(!CASRestore(component,owned,previous,state.machineRestores))return false;
    }
    // Never zero table storage: a pending-kill component may still use its
    // permanent forwarding table during native destruction callbacks.
    original_weapon::state.state_machine_observers_installed=false;
    original_weapon::state.observed_machine_count=0;
    return true;
}
inline bool TrackLegacyEffect(void* object){
    if(!object)return true;
    original_effects::WeakIdentity id{};
    if(!original_effects::CaptureIdentity(object,id))return true;
    for(auto& entry:legacyEffects){
        if(entry.object==object&&entry.identity.index==id.index&&entry.identity.serial==id.serial)return true;
        if(entry.object&&original_effects::ResolveIdentity(entry.identity)!=entry.object)entry={};
    }
    for(auto& entry:legacyEffects)if(!entry.object){entry.object=object;entry.identity=id;return true;}
    return Fail(2384,"weapon_runtime_legacy_effect_tracking_capacity_exhausted");
}
inline bool TrackLegacyEffects(){
    for(const auto& entry:original_shots::seen)if(entry.object&&!TrackLegacyEffect(entry.object))return false;
    auto objects=weapon_runtime_bindings::Read<void**>(original_shots::firing,0x500);
    const int count=weapon_runtime_bindings::Read<int>(original_shots::firing,0x508);
    if(count<0||count>64||(count&&!original_players::Readable(objects,static_cast<size_t>(count)*8)))
        return Fail(2385,"weapon_runtime_original_active_effect_array_invalid");
    for(int i=0;i<count;++i)if(!TrackLegacyEffect(objects[i]))return false;
    return true;
}
inline bool AdoptOriginParent(void* pawn,void** previous){
    if(!Thread()||!pawn||!previous||previous==original_shots::pawnTableStorage.data()+1
        ||!original_players::Readable(previous-1,sizeof(original_shots::pawnTableStorage))
        ||previous[0x850/8]!=reinterpret_cast<void*>(engine::image+0x787f820))
        return Fail(2386,"weapon_runtime_original_pawn_parent_layer_not_proven");
    state.legacyPawn=pawn;state.legacyPawnParent=previous;
    return original_effects::CaptureIdentity(pawn,legacyPawnIdentity);
}
inline bool InitializeLegacyShots(void* firing,void* ammo,original_shots::Sink sink=nullptr){
    if(!Thread()||original_shots::state.initialized)return false;
    auto pawn=weapon_runtime_bindings::Read<void*>(firing,0x4c8);
    if(!AdoptOriginParent(pawn,weapon_runtime_bindings::Read<void**>(pawn,0)))return false;
    state.legacyShots=true;state.legacyPoolTrackingComplete=true;
    return original_shots::Initialize(firing,ammo,sink);
}
inline void PollLegacyShots(){
    if(!Thread()||!state.legacyShots)return;
    if(!TrackLegacyEffects())return;
    original_shots::Poll();TrackLegacyEffects();
}
inline bool RestoreLegacyShots(){
    if(!original_shots::state.initialized)return true;
    if(!state.legacyShots)return Fail(2387,"weapon_runtime_legacy_shots_owned_by_another_coordinator");
    if(!TrackLegacyEffects())return false;
    const auto owned=original_shots::tableStorage.data()+1;
    const auto previous=reinterpret_cast<void**>(engine::image+0xb29a8a0);
    for(auto& entry:legacyEffects)if(entry.object){
        auto live=original_effects::ResolveIdentity(entry.identity);
        if(live!=entry.object){++state.expiredNoWrite;entry={};continue;}
        if(weapon_runtime_bindings::Read<void**>(live,0)==owned
            &&!CASRestore(live,owned,previous,state.effectRestores))return false;
        entry={};
    }
    if(original_shots::sourcePawn){
        if(state.legacyPawn!=original_shots::sourcePawn||!state.legacyPawnParent)
            return Fail(2388,"weapon_runtime_legacy_origin_parent_not_captured");
        if(original_effects::ResolveIdentity(legacyPawnIdentity)==state.legacyPawn){
            if(!CASRestore(state.legacyPawn,original_shots::pawnTableStorage.data()+1,state.legacyPawnParent,state.originRestores))return false;
        }else ++state.expiredNoWrite;
    }
    // Keep original dispatch pointers and all table bytes alive for older
    // pooled/pending-kill effects. Reset only observation/session data.
    original_shots::state={};original_shots::firing=nullptr;original_shots::ammoComponent=nullptr;
    original_shots::sourcePawn=nullptr;original_shots::sink=nullptr;
    original_shots::writeIndex=0;original_shots::readIndex=0;original_shots::queue={};original_shots::seen={};
    original_shots::recent={};original_shots::nextKey=0;original_shots::nextEffectOccurrence=0;
    original_shots::origins={};original_shots::originWriteIndex=0;original_shots::directionSamples={};original_shots::directionWriteIndex=0;
    state.legacyPawn=nullptr;state.legacyPawnParent=nullptr;legacyPawnIdentity={};return true;
}
inline bool ReleaseOriginalInputs(void* pawn){
    if(!original_players::IsA(pawn,0x6c5c480))return false;
    for(uint8_t input=0;input<3;++input){struct P{uint8_t input,state=0;} p{input};
        if(!all_weapons::Event(pawn,L"/Script/ShooterGame.ShooterCharacter.ForceInputState",2,&p))return false;}
    auto component=original_weapon::Component(pawn,0x6492f30);
    auto inputState=weapon_runtime_bindings::Read<void*>(pawn,0xba0);
    if(!original_players::IsA(component,0x6492f30)||!original_players::IsA(inputState,0x6494370)
        ||weapon_runtime_bindings::Read<void*>(component,0xc8)!=pawn||weapon_runtime_bindings::Read<void*>(inputState,0xc8)!=pawn
        ||weapon_runtime_bindings::Read<void*>(component,0x170)!=pawn||weapon_runtime_bindings::Read<void*>(inputState,0x148)!=pawn
        ||weapon_runtime_bindings::Read<void**>(component,0)!=reinterpret_cast<void**>(engine::image+0xb099220)
        ||weapon_runtime_bindings::Read<void**>(inputState,0)!=reinterpret_cast<void**>(engine::image+0xb093c10))
        return Fail(2389,"weapon_runtime_original_input_owner_mismatch");
    const int count=weapon_runtime_bindings::Read<int>(component,0x8b8),capacity=weapon_runtime_bindings::Read<int>(component,0x8bc);
    if(count<0||count>1024||capacity<count||(count&&!weapon_runtime_bindings::Read<void*>(component,0x8b0)))
        return Fail(2395,"weapon_runtime_original_input_buffer_invalid");
    if(engine::function<int(*)(void*)>(0x3efc820)(pawn)==0)
        engine::function<void(*)(void*,void*)>(0x70a7110)(inputState,static_cast<uint8_t*>(component)+0x178);
    original_weapon::previousInput=0;return true;
}
inline bool Quiesce(void* oldWeapon){
    if(!Thread())return false;
    (void)oldWeapon;
    if(state.quiesced)return true;
    auto pawn=all_weapons::State().pawn?all_weapons::State().pawn:original_weapon::pawn;
    if(pawn&&!state.releaseIssued){
        original_effects::WeakIdentity id{};
        if(original_effects::CaptureIdentity(pawn,id)&&original_effects::ResolveIdentity(id)==pawn)
            if(!ReleaseOriginalInputs(pawn))return false;
        state.releaseIssued=true;
    }
    if(!RestoreMachineObservers()||!RestoreLegacyShots())return false;
    state.quiesced=true;state.ready=false;state.stage="weapon_runtime_original_input_and_observers_quiesced";return true;
}
inline bool Bind(void* oldWeapon,void* newWeapon,const Definition& definition){
    if(!Thread()||!state.quiesced)return Fail(2390,"weapon_runtime_bind_requires_quiesce");
    auto& context=all_weapons::State();auto pawn=context.pawn;auto inventory=context.inventory;
    if(newWeapon!=all_weapons::CurrentWeapon()||weapon_runtime_bindings::Read<void*>(newWeapon,0x10)!=definition.runtimeClass
        ||weapon_runtime_bindings::Read<void*>(newWeapon,0x4a8)!=pawn||weapon_runtime_bindings::Read<void*>(newWeapon,0x210)!=pawn)
        return Fail(2391,"weapon_runtime_original_current_weapon_identity_mismatch");
    state.world=context.world;state.pawn=pawn;state.inventory=inventory;state.weapon=newWeapon;
    state.slot=weapon_runtime_bindings::Read<void*>(inventory,0x1b8+static_cast<size_t>(definition.equipmentSlot)*8);
    state.firing=original_weapon::Component(newWeapon,0x684d4c0);state.ammo=original_weapon::Component(newWeapon,0x63c7e00);
    state.stability=original_weapon::Component(newWeapon,0x6d1e460);
    state.mesh1p=weapon_runtime_bindings::Read<void*>(newWeapon,0xde0);state.mesh3p=weapon_runtime_bindings::Read<void*>(newWeapon,0xdf0);
    state.machine=weapon_runtime_bindings::Read<void*>(newWeapon,0xe00);state.melee=definition.categoryValue==7;state.firearm=!state.melee;
    if(!state.machine||!state.mesh1p||!state.mesh3p||(state.firearm&&(!state.firing||!state.ammo||!state.stability)))
        return Fail(2392,"weapon_runtime_original_components_missing");
    // Only these local observation pointers are reset. Never reconstruct/reset
    // actual firing, reload, stability, ammo, animation or inventory objects.
    original_weapon::state={};original_weapon::world=state.world;original_weapon::pawn=pawn;original_weapon::inventory=inventory;
    original_weapon::weapon=newWeapon;original_weapon::weaponClass=definition.runtimeClass;original_weapon::defaults=definition.defaults;
    original_weapon::slot=state.slot;original_weapon::firing=state.firing;original_weapon::ammo=state.ammo;original_weapon::stability=state.stability;
    original_weapon::mesh1p=state.mesh1p;original_weapon::mesh3p=state.mesh3p;original_weapon::stateMachine=state.machine;
    original_weapon::nativeInput=nullptr;original_weapon::nativeInputState=nullptr;original_weapon::thread=GetCurrentThreadId();
    original_weapon::previousInput=0;original_weapon::lastInputInspection=0;
    original_weapon::state.class_loaded=true;original_weapon::state.cdo_loaded=true;original_weapon::state.created=true;
    original_weapon::state.original_pawn=true;original_weapon::state.initialized_pawn=true;original_weapon::state.inventory_owner_valid=true;
    original_weapon::state.providers_valid=true;original_weapon::state.equip_requested=true;original_weapon::state.empty_slot_published=true;
    original_weapon::RefreshDiagnostics();
    original_weapon::slot=state.slot;
    state.originalInputReady=original_weapon::OriginalCharacterInputContextReady();
    if(!state.originalInputReady)return Fail(2393,"weapon_runtime_original_character_input_not_rebound");
    if(afterBind&&!afterBind(state.world,pawn,newWeapon,definition))return Fail(2394,"weapon_runtime_after_bind_failed");
    (void)oldWeapon;
    state.quiesced=false;state.releaseIssued=false;state.ready=true;state.error=0;++state.generation;
    state.stage="weapon_runtime_original_weapon_bound";return true;
}
inline bool BeforeRetire(void* weapon){
    if(!Thread())return false;
    if(weapon==original_weapon::weapon&&!Quiesce(weapon))return false;
    return original_effects::RetireActor(weapon);
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    std::fprintf(f,"{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u,\"ready\":%d,\"quiesced\":%d,\"generation\":%u,\"firearm\":%d,\"melee\":%d,\"original_input_ready\":%d,\"machine_restores\":%u,\"effect_restores\":%u,\"origin_restores\":%u,\"expired_no_write\":%u,\"legacy_shots_enabled\":%d,\"weapon\":\"%p\",\"pawn\":\"%p\",\"slot\":\"%p\",\"firing\":\"%p\",\"ammo\":\"%p\",\"stability\":\"%p\",\"legacy_parent_table\":\"%p\"}\n",
      state.stage,state.error,state.ready,state.quiesced,state.generation,state.firearm,state.melee,state.originalInputReady,
      state.machineRestores,state.effectRestores,state.originRestores,state.expiredNoWrite,state.legacyShots,
      state.weapon,state.pawn,state.slot,state.firing,state.ammo,state.stability,state.legacyPawnParent);
    const bool ok=std::ferror(f)==0;std::fclose(f);return ok;
}
} // namespace weapon_runtime_bindings
