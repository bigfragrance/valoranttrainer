#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <cstdio>
#include <algorithm>
#include "engine_api.h"
#include "original_effects.h"

// Pinned-client original weapon experiment. Evidence and explicit limitations:
// reports/duel1-original-weapon-integration.json. Game-thread calls only.
// This module supplies ONLY the missing local spawn/empty inventory-slot data.
// Input, equip restrictions, state transitions, ammo, stability and animations
// remain the installed client's implementations. No fallback firing algorithm.
namespace original_weapon {
inline constexpr const wchar_t* kAK = L"/Game/Equippables/Guns/Rifles/AK/AssaultRifle_AK.AssaultRifle_AK_C";
inline constexpr unsigned MaxObservedMachines=8;
inline constexpr unsigned MaxObservedEventResponses=16;
struct EventResponseDiagnostics{
    uintptr_t event_class=0,next_state=0;
    char event_name[128]{},next_state_name[128]{};
};
struct InventoryResponseDiagnostics{
    uint8_t equip_type=255;
    int filtered_class_count=0;
    uintptr_t next_state=0;
    char next_state_name[128]{};
};
struct MachineDiagnostics{
    uintptr_t component=0,auth_state=0,client_state=0,inactive_state=0;
    char name[128]{},auth_name[128]{},client_name[128]{};
    uint64_t process_calls=0,process_with_pending_calls=0,tick_calls=0,positive_tick_calls=0;
    int pending_before=0,pending_after=0,max_pending=0;
    uint8_t last_input=255,last_action=255,last_activation=255;
    float input_timestamp=0,input_ttl=0,input_clock=0,input_age=0,last_tick_delta=0;
    bool auth_firing_observed=false,client_firing_observed=false;
    bool inactive_is_response_state=false,inactive_machine_owner_valid=false,inactive_response_map_valid=false;
    int inactive_response_storage_count=0,inactive_response_free_count=0,inactive_response_count=0;
    unsigned captured_response_count=0;
    uintptr_t starting_state=0;
    char starting_state_name[128]{};
    EventResponseDiagnostics responses[MaxObservedEventResponses]{};
    bool inactive_inventory_responses_valid=false,inactive_native_pawn_owner_valid=false;
    bool inactive_inventory_equipped_binding=false,inactive_inventory_unequipped_binding=false,inactive_owner_set_binding=false;
    int inactive_inventory_response_count=0;
    unsigned captured_inventory_response_count=0;
    InventoryResponseDiagnostics inventory_responses[MaxObservedEventResponses]{};
};
struct EquipEventRegistration{
    bool valid=false,registered_before=false,registered_after=false,component_enabled=false;
    bool graph_valid=false,auto_equip_response_observed=false,local_equip_response_supplied=false;
    uintptr_t component=0,registry=0,type=0;
    uintptr_t inactive_state=0,equip_target=0;
    int graph_responses_before=0,graph_responses_after=0,filtered_class_count=0;
    unsigned error=0;
};
struct Diagnostics {
    bool class_loaded=false, cdo_loaded=false, original_pawn=false, initialized_pawn=false;
    bool inventory_owner_valid=false, providers_valid=false, created=false, owner_bound=false;
    bool empty_slot_published=false, equip_requested=false, desired=false, current=false;
    bool ready=false, can_fire=false;
    bool primary_slot_present=false, primary_slot_is_item=false, primary_slot_is_multi=false;
    bool primary_slot_backref_valid=false, primary_slot_outer_valid=false, primary_slot_occupied=false;
    bool primary_slot_locally_created=false;
    uint32_t populated_slot_count=0;
    uintptr_t primary_slot_pointer=0, primary_slot_class_pointer=0, primary_slot_backref_pointer=0, primary_slot_contents_pointer=0;
    unsigned error=0, input_edges=0;
    int ammo=-1, max_ammo=-1, state_machine_count=0, stability_count=0;
    int ammo_property_value=-1,ammo_predicted_value=-1,ammo_authority_value=-1,minimum_ammo_observed=-1;
    int reserve_ammo=-1,reserve_max_ammo=-1;
    bool native_ammo_query_valid=false,reserve_component_present=false,native_can_reload_query_valid=false,native_can_reload=false;
    bool character_input_context_valid=false,character_input_current_is_rifle=false;
    bool character_input_routed_while_rifle_inactive=false;
    uintptr_t character_input_current_equippable=0;
    int pending_equippable_changes=0;
    bool original_state_machine_enabled=false;
    uint8_t original_tick_state=0, original_tick_contents=0;
    uintptr_t original_current_state_pointer=0;
    uintptr_t input_component_pointer=0, input_component_owner_pointer=0, input_component_pawn_pointer=0;
    uintptr_t input_state_pointer=0;
    bool input_component_present=false, native_input_gate_valid=false, native_input_gate=false;
    bool input_component_owner_valid=false, input_component_pawn_valid=false;
    bool native_can_fire_query_valid=false, native_input_queries_valid=false;
    int input_owner_net_mode=-1, input_component_pending=0, input_state_pending=0;
    int max_input_component_pending=0, max_input_state_pending=0;
    uint8_t current_primary_input=0, current_secondary_input=0, current_reload_input=0;
    uint8_t input_state_primary=0, input_state_secondary=0, input_state_reload=0;
    bool primary_input_down=false, secondary_input_down=false, reload_input_down=false;
    bool standalone_input_submission_valid=false;
    uint32_t standalone_input_submissions=0, standalone_input_events=0;
    int submitted_input_pending_before=0, submitted_input_pending_after=0;
    bool primary_pressed_observed=false, primary_down_observed=false, can_fire_observed=false;
    bool secondary_pressed_observed=false, reload_pressed_observed=false;
    uint32_t primary_pressed_samples=0, primary_down_samples=0;
    int input_inventory_listener_kind=0;
    uintptr_t input_inventory_listener_pointer=0;
    bool input_inventory_listener_resolves_owner=false,input_inventory_callback_original=false;
    bool state_machine_observers_installed=false;
    bool equipped_event_registration_valid=false,equipped_event_registered_before=false,equipped_event_registered_after=false;
    bool equipped_event_local_registration_supplied=false,equipped_event_component_enabled=false;
    uintptr_t equipped_event_component_pointer=0,equipped_event_registry_pointer=0,equipped_event_class_pointer=0;
    int event_listener_storage_count=0,event_listener_free_count=0;
    bool original_equip_graph_valid=false,original_auto_equip_response_observed=false,local_equip_response_supplied=false;
    uintptr_t original_equip_graph_inactive=0,original_equip_graph_target=0;
    int original_equip_graph_responses_before=0,original_equip_graph_responses_after=0,original_equip_graph_class_count=0;
    unsigned observed_machine_count=0;
    MachineDiagnostics machines[MaxObservedMachines]{};
    float pitch_recoil=0, yaw_recoil=0, firing_error=0, firing_rate=0, movement_ratio=0;
    const char* stage="original_weapon_not_initialized";
};
inline Diagnostics state;
inline const Diagnostics& State() { return state; }
inline void *world=nullptr, *pawn=nullptr, *inventory=nullptr, *weaponClass=nullptr, *defaults=nullptr;
inline void *weapon=nullptr, *slot=nullptr, *firing=nullptr, *ammo=nullptr, *stability=nullptr;
inline void *mesh1p=nullptr, *mesh3p=nullptr, *stateMachine=nullptr;
inline void *nativeInput=nullptr, *nativeInputState=nullptr;
inline DWORD thread=0;
inline ULONGLONG lastInputInspection=0;
inline uint32_t previousInput=0;
inline void* observedMachineTables[MaxObservedMachines][164]{};
inline void* observedMachineOriginalTables[MaxObservedMachines]{};
template<class T> inline T Read(const void* object,size_t offset) {
    T value{}; if(object)std::memcpy(&value,static_cast<const uint8_t*>(object)+offset,sizeof(value));return value;
}
template<class T> inline void Put(void* object,size_t offset,const T& value) {
    std::memcpy(static_cast<uint8_t*>(object)+offset,&value,sizeof(value));
}
inline bool Fail(unsigned code,const char* stage) { state.error=code;state.stage=stage;state.ready=false;return false; }
inline bool IsA(void* object,void* type) {
    if(!object||!type)return false;
    auto cls=Read<void*>(object,0x10);
    for(unsigned i=0;cls&&i<64;++i){if(cls==type)return true;cls=Read<void*>(cls,0x48);}
    return false;
}
inline bool OnThread() { return thread && thread==GetCurrentThreadId(); }
inline void InspectPrimarySlot() {
    if(!inventory)return;
    slot=Read<void*>(inventory,0x1b8);
    state.primary_slot_pointer=reinterpret_cast<uintptr_t>(slot);
    state.primary_slot_present=slot!=nullptr;
    state.primary_slot_is_item=IsA(slot,engine::Class(0x68f63c0));
    state.primary_slot_is_multi=IsA(slot,engine::Class(0x69cf3d0));
    state.primary_slot_class_pointer=reinterpret_cast<uintptr_t>(Read<void*>(slot,0x10));
    state.primary_slot_backref_pointer=0;state.primary_slot_contents_pointer=0;
    state.primary_slot_backref_valid=false;state.primary_slot_outer_valid=false;state.primary_slot_occupied=false;
    if(state.primary_slot_is_item){
        state.primary_slot_backref_pointer=reinterpret_cast<uintptr_t>(Read<void*>(slot,0x40));
        state.primary_slot_contents_pointer=reinterpret_cast<uintptr_t>(Read<void*>(slot,0x38));
        state.primary_slot_backref_valid=Read<void*>(slot,0x40)==inventory;
        state.primary_slot_outer_valid=Read<void*>(slot,0x28)==inventory;
        state.primary_slot_occupied=Read<void*>(slot,0x38)!=nullptr;
    }
    state.populated_slot_count=0;
    for(unsigned i=0;i<17;++i)if(Read<void*>(inventory,0x1b8+i*8))++state.populated_slot_count;
}
inline bool EnsurePrimarySlot() {
    InspectPrimarySlot();
    if(!slot){
        state.stage="constructing_missing_original_primary_item_slot";
        struct Params {void* type;void* outer;void* result=nullptr;} params{engine::Class(0x68f63c0),inventory};
        static_assert(sizeof(Params)==24);
        auto library=engine::DefaultObject(engine::Class(0x43a9670));
        if(!engine::Event(library,L"/Script/Engine.GameplayStatics.SpawnObject",&params)||!params.result)
            return Fail(719,"original_primary_slot_constructor_failed");
        if(!IsA(params.result,params.type)||Read<void*>(params.result,0x28)!=inventory)
            return Fail(720,"original_primary_slot_constructed_identity_invalid");
        // Missing local authority assembly only: the real native ItemSlot has
        // SlotType byte+30, Contents pointer+38, Inventory pointer+40.
        Put<uint8_t>(params.result,0x30,0);Put(params.result,0x40,inventory);
        Put(inventory,0x1b8,params.result);state.primary_slot_locally_created=true;
        InspectPrimarySlot();
    }
    if(!state.primary_slot_is_item)return Fail(721,"original_primary_slot_wrong_class");
    if(state.primary_slot_is_multi)return Fail(722,"original_primary_slot_unexpected_multi_slot");
    if(!state.primary_slot_backref_valid)return Fail(723,"original_primary_slot_inventory_backref_mismatch");
    if(state.primary_slot_occupied)return Fail(724,"original_primary_slot_already_occupied");
    return true;
}
inline void* Component(void* actor,uintptr_t classGetter) {
    struct Params { void* type; void* result=nullptr; } p{engine::Class(classGetter)};
    if(!engine::Event(actor,L"/Script/Engine.Actor.GetComponentByClass",&p))return nullptr;
    return IsA(p.result,p.type)?p.result:nullptr;
}
inline void OriginalNameValue(const void* name,char (&out)[128]){
    out[0]=0;if(!name)return;
    struct String{wchar_t* data=nullptr;int count=0,capacity=0;} text;
    static_assert(sizeof(String)==16);
    // Original FName::ToString and FString destructor, as used by the native
    //state machine's own debug formatter. Observe the shipped component name.
    engine::function<void*(*)(const void*,void*)>(0x1a7bf60)
        (name,&text);
    if(text.data&&text.count>0&&text.count<=1024&&text.capacity>=text.count){
        const int count=WideCharToMultiByte(CP_UTF8,0,text.data,text.count-1,out,127,nullptr,nullptr);
        if(count>0)out[count]=0;
    }
    engine::function<void(*)(void*)>(0x18a2f10)(&text);
}
inline void OriginalObjectName(void* object,char (&out)[128]){
    OriginalNameValue(object?static_cast<uint8_t*>(object)+0x18:nullptr,out);
}
inline MachineDiagnostics* MachineRecord(void* component){
    for(unsigned n=0;n<state.observed_machine_count;++n)
        if(state.machines[n].component==reinterpret_cast<uintptr_t>(component))return &state.machines[n];
    return nullptr;
}
inline bool OriginalDelegateBindsObject(void* object,size_t offset,void* target){
    const int count=Read<int>(object,offset+8),capacity=Read<int>(object,offset+12);
    auto entries=Read<uint8_t*>(object,offset);
    if(!entries||count<0||count>128||capacity<count)return false;
    for(int n=0;n<count;++n)
      if(engine::function<void*(*)(const void*)>(0x1d4cfa0)(entries+n*20)==target)return true;
    return false;
}
inline void CaptureInactiveResponseGraph(void* component,MachineDiagnostics& item){
    auto starting=Read<void*>(component,0x1e8);
    if(item.starting_state!=reinterpret_cast<uintptr_t>(starting))OriginalObjectName(starting,item.starting_state_name);
    item.starting_state=reinterpret_cast<uintptr_t>(starting);
    auto inactive=Read<void*>(component,0x350);
    item.inactive_is_response_state=IsA(inactive,engine::Class(0x63a92a0));
    if(!item.inactive_is_response_state)return;
    item.inactive_machine_owner_valid=Read<void*>(inactive,0xe0)==component;
    item.inactive_native_pawn_owner_valid=engine::function<void*(*)(void*)>(0x7115010)(inactive)==pawn;
    item.inactive_inventory_equipped_binding=OriginalDelegateBindsObject(inventory,0x120,inactive);
    item.inactive_inventory_unequipped_binding=OriginalDelegateBindsObject(inventory,0x130,inactive);
    item.inactive_owner_set_binding=OriginalDelegateBindsObject(weapon,0x480,inactive);
    const int inventoryCount=Read<int>(inactive,0x368),inventoryCapacity=Read<int>(inactive,0x36c);
    auto inventoryEntries=Read<uint8_t*>(inactive,0x360);
    item.inactive_inventory_response_count=inventoryCount;
    item.inactive_inventory_responses_valid=inventoryCount>=0&&inventoryCount<=128&&inventoryCapacity>=inventoryCount
      &&(!inventoryCount||inventoryEntries);
    if(item.inactive_inventory_responses_valid){
      item.captured_inventory_response_count=(std::min)(static_cast<unsigned>(inventoryCount),MaxObservedEventResponses);
      for(unsigned n=0;n<item.captured_inventory_response_count;++n){
        auto& response=item.inventory_responses[n];auto entry=inventoryEntries+n*0x60;
        response.equip_type=Read<uint8_t>(entry,0);
        response.filtered_class_count=Read<int>(entry,0x10)-Read<int>(entry,0x3c);
        auto next=Read<void*>(entry,0x58);
        if(response.next_state!=reinterpret_cast<uintptr_t>(next))OriginalObjectName(next,response.next_state_name);
        response.next_state=reinterpret_cast<uintptr_t>(next);
      }
    }
    const int count=Read<int>(inactive,0x398),capacity=Read<int>(inactive,0x39c);
    const int bits=Read<int>(inactive,0x3b8),free=Read<int>(inactive,0x3c4);
    item.inactive_response_storage_count=count;item.inactive_response_free_count=free;
    auto entries=Read<uint8_t*>(inactive,0x390);
    auto bitmap=Read<uint32_t*>(inactive,0x3b0);
    if(!bitmap)bitmap=reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(inactive)+0x3a0);
    item.inactive_response_map_valid=count>=0&&count<=128&&capacity>=count&&free>=0&&free<=count
      &&bits>=count&&(!count||entries);
    if(!item.inactive_response_map_valid)return;
    item.inactive_response_count=count-free;
    unsigned captured=0;
    for(int n=0;n<count&&captured<MaxObservedEventResponses;++n){
      if(!(bitmap[n/32]&(1u<<(n%32))))continue;
      auto& response=item.responses[captured++];
      auto eventType=Read<void*>(entries,n*24),next=Read<void*>(entries,n*24+8);
      if(response.event_class!=reinterpret_cast<uintptr_t>(eventType))OriginalObjectName(eventType,response.event_name);
      if(response.next_state!=reinterpret_cast<uintptr_t>(next))OriginalObjectName(next,response.next_state_name);
      response.event_class=reinterpret_cast<uintptr_t>(eventType);response.next_state=reinterpret_cast<uintptr_t>(next);
    }
    item.captured_response_count=captured;
}
inline void CaptureMachine(void* component,MachineDiagnostics& item){
    auto auth=Read<void*>(component,0x1c8),client=Read<void*>(component,0x1b0);
    if(item.auth_state!=reinterpret_cast<uintptr_t>(auth))OriginalObjectName(auth,item.auth_name);
    if(item.client_state!=reinterpret_cast<uintptr_t>(client))OriginalObjectName(client,item.client_name);
    item.auth_state=reinterpret_cast<uintptr_t>(auth);item.client_state=reinterpret_cast<uintptr_t>(client);
    item.inactive_state=reinterpret_cast<uintptr_t>(Read<void*>(component,0x350));
    if(auth==firing)item.auth_firing_observed=true;
    if(client==firing)item.client_firing_observed=true;
    CaptureInactiveResponseGraph(component,item);
}
inline void ObserveMachineProcessInput(void* component){
    auto item=MachineRecord(component);
    if(item){
        ++item->process_calls;item->pending_before=Read<int>(component,0x388);
        item->max_pending=(std::max)(item->max_pending,item->pending_before);
        if(item->pending_before>0){
            ++item->process_with_pending_calls;
            const auto input=Read<uint8_t*>(component,0x380);
            if(input){
                item->last_input=Read<uint8_t>(input,0);item->last_action=Read<uint8_t>(input,1);
                item->last_activation=Read<uint8_t>(input,2);item->input_timestamp=Read<float>(input,4);
                if(item->last_input<15)item->input_ttl=Read<float>(component,0x390+item->last_input*4);
                item->input_clock=engine::function<float(*)(void*)>(0x789df70)(Read<void*>(pawn,0x4f8));
                item->input_age=item->input_clock-item->input_timestamp;
            }
        }
        CaptureMachine(component,*item);
    }
    engine::function<void(*)(void*)>(0x71161d0)(component);
    if(item){item->pending_after=Read<int>(component,0x388);CaptureMachine(component,*item);}
}
inline void ObserveMachineTick(void* component,float delta){
    auto item=MachineRecord(component);
    if(item){++item->tick_calls;if(delta>0)++item->positive_tick_calls;item->last_tick_delta=delta;CaptureMachine(component,*item);}
    engine::function<void(*)(void*,float)>(0x7116420)(component,delta);
    if(item)CaptureMachine(component,*item);
}
inline bool InstallStateMachineObservers(){
    if(!OnThread()||!state.ready)return false;
    if(state.state_machine_observers_installed)return true;
    const auto list=Read<void**>(weapon,0xe30);const auto count=Read<int>(weapon,0xe38);
    if(!list||count<=0||count>static_cast<int>(MaxObservedMachines))return Fail(728,"original_machine_observer_array_invalid");
    auto expected=reinterpret_cast<void**>(engine::image+0xb245af0);
    if(expected[0x4f8/8]!=reinterpret_cast<void*>(engine::image+0x71161d0)
       ||expected[0x500/8]!=reinterpret_cast<void*>(engine::image+0x7116420))
        return Fail(729,"original_machine_observer_targets_changed");
    for(int n=0;n<count;++n)
        if(!IsA(list[n],engine::Class(0x67722c0))||Read<void**>(list[n],0)!=expected)
            return Fail(730,"original_machine_observer_table_changed");
    state.observed_machine_count=static_cast<unsigned>(count);
    for(unsigned n=0;n<state.observed_machine_count;++n){
        auto& item=state.machines[n];item.component=reinterpret_cast<uintptr_t>(list[n]);
        OriginalObjectName(list[n],item.name);CaptureMachine(list[n],item);
        // Complete pinned primary table:163 code pointers followed by zero.
        // Preserve its preceding word and every existing virtual implementation.
        std::memcpy(observedMachineTables[n],expected-1,sizeof(observedMachineTables[n]));
        observedMachineOriginalTables[n]=expected;
        auto copied=observedMachineTables[n]+1;
        copied[0x4f8/8]=reinterpret_cast<void*>(&ObserveMachineProcessInput);
        copied[0x500/8]=reinterpret_cast<void*>(&ObserveMachineTick);
        if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(list[n]),copied,expected)!=expected)
            return Fail(731,"original_machine_observer_component_changed");
    }
    state.state_machine_observers_installed=true;return true;
}
inline bool Diagnose(void* inWorld,void* inPawn) {
    if(!engine::image||!inWorld||!inPawn)return Fail(701,"original_weapon_context_missing");
    if(thread&&thread!=GetCurrentThreadId())return Fail(702,"original_weapon_wrong_thread");
    thread=GetCurrentThreadId(); world=inWorld;pawn=inPawn;state.error=0;
    state.stage="loading_original_ak_class";
    weaponClass=engine::function<void*(*)(void*,void*,const wchar_t*,const wchar_t*,uint32_t,void*)>(0x1d0c6e0)
      (engine::Class(0x645c660),nullptr,kAK,nullptr,0,nullptr);
    state.class_loaded=weaponClass!=nullptr;
    if(!weaponClass)return Fail(703,"original_ak_class_missing");
    defaults=engine::DefaultObject(weaponClass);state.cdo_loaded=defaults!=nullptr;
    if(!defaults)return Fail(704,"original_ak_defaults_missing");
    state.movement_ratio=Read<float>(defaults,0xdb4);
    state.original_pawn=IsA(pawn,engine::Class(0x6c5c480));
    if(!state.original_pawn)return Fail(705,"requires_actual_shooter_character");
    state.initialized_pawn=Read<uint8_t>(pawn,0xa90)!=0;
    inventory=Read<void*>(pawn,0xc08);
    state.inventory_owner_valid=IsA(inventory,engine::Class(0x6499330)) &&
      Read<void*>(inventory,0xc8)==pawn && Read<void*>(inventory,0x2f0)==pawn;
    if(state.inventory_owner_valid)InspectPrimarySlot();
    // Original owner callback 70f0250 dereferences these without null checks.
    auto abilities=Read<void*>(pawn,0xa20);
    state.providers_valid=state.initialized_pawn && state.inventory_owner_valid &&
      Read<void*>(pawn,0xba0) && Read<void*>(pawn,0xf30) && abilities && Read<void*>(abilities,0x13a8);
    if(!state.providers_valid)return Fail(706,"original_player_initialization_or_providers_missing");
    state.stage="original_weapon_prerequisites_observed";return true;
}
inline void InspectOriginalInputStateFast(){
    if(!nativeInputState)return;
    state.input_state_pending=Read<int>(nativeInputState,0xaa0);
    state.max_input_state_pending=(std::max)(state.max_input_state_pending,state.input_state_pending);
    state.input_state_primary=Read<uint8_t>(nativeInputState,0x265);
    state.input_state_secondary=Read<uint8_t>(nativeInputState,0x266);
    state.input_state_reload=Read<uint8_t>(nativeInputState,0x267);
    if(state.input_state_primary==1){state.primary_pressed_observed=true;++state.primary_pressed_samples;}
    if(state.input_state_secondary==1)state.secondary_pressed_observed=true;
    if(state.input_state_reload==1)state.reload_pressed_observed=true;
    state.input_inventory_listener_kind=Read<int>(nativeInputState,0x160);
    state.input_inventory_listener_pointer=reinterpret_cast<uintptr_t>(Read<void*>(nativeInputState,0x158));
    if(state.input_inventory_listener_kind==3&&state.input_inventory_listener_pointer){
      auto listener=reinterpret_cast<uint8_t*>(state.input_inventory_listener_pointer);
      state.input_inventory_callback_original=Read<void*>(listener,0x20)==reinterpret_cast<void*>(engine::image+0x70fbcc0);
      state.input_inventory_listener_resolves_owner=engine::function<void*(*)(void*)>(0x1d4cfa0)(listener+0x18)==inventory;
    }
}
inline void InspectOriginalInput(){
    if(!pawn||!OnThread())return;
    lastInputInspection=GetTickCount64();
    nativeInputState=Read<void*>(pawn,0xba0);
    state.input_state_pointer=reinterpret_cast<uintptr_t>(nativeInputState);
    InspectOriginalInputStateFast();
    if(!nativeInput)nativeInput=Component(pawn,0x6492f30);
    state.input_component_pointer=reinterpret_cast<uintptr_t>(nativeInput);
    state.input_component_present=nativeInput!=nullptr;
    if(nativeInput){
      const auto owner=Read<void*>(nativeInput,0xc8), typed=Read<void*>(nativeInput,0x170);
      state.input_component_owner_pointer=reinterpret_cast<uintptr_t>(owner);
      state.input_component_pawn_pointer=reinterpret_cast<uintptr_t>(typed);
      state.input_component_owner_valid=owner==pawn;state.input_component_pawn_valid=typed==pawn;
      state.input_component_pending=Read<int>(nativeInput,0x8b8);
      state.max_input_component_pending=(std::max)(state.max_input_component_pending,state.input_component_pending);
      // Original70a5310 is a read-only ownership/controller-enable query.
      // Original input Tick70a4530 exits for GetNetMode()==Standalone0;
      // exporting both values distinguishes rejected input from an unpumped
      // legitimate native command queue. Never change Role/NetMode/tick flags.
      state.native_input_gate_valid=state.input_component_owner_valid&&state.input_component_pawn_valid;
      if(state.native_input_gate_valid){
        state.native_input_gate=engine::function<bool(*)(void*)>(0x70a5310)(nativeInput);
        state.input_owner_net_mode=engine::function<int(*)(void*)>(0x3efc820)(owner);
      }
    }
    state.native_input_queries_valid=true;
    uint8_t* current[]{&state.current_primary_input,&state.current_secondary_input,&state.current_reload_input};
    bool* down[]{&state.primary_input_down,&state.secondary_input_down,&state.reload_input_down};
    for(uint8_t input=0;input<3;++input){
      struct Params{uint8_t input,result;} p{input,0};
      if(!engine::Event(pawn,L"/Script/ShooterGame.ShooterCharacter.GetCurrentInputState",&p))
        state.native_input_queries_valid=false;
      *current[input]=p.result;p.result=0;
      if(!engine::Event(weapon,L"/Script/ShooterGame.AresEquippable.IsInputDown",&p))
        state.native_input_queries_valid=false;
      *down[input]=p.result!=0;
    }
    if(state.primary_input_down){state.primary_down_observed=true;++state.primary_down_samples;}
    struct CanFire{uint8_t cooldown=0,result=0;} p;
    state.native_can_fire_query_valid=firing&&engine::Event(firing,L"/Script/ShooterGame.FiringStateComponent.CanFire",&p);
    state.can_fire=state.native_can_fire_query_valid&&p.result!=0;
    if(state.can_fire)state.can_fire_observed=true;
}
inline void RefreshDiagnostics() {
    if(!OnThread())return;
    if(inventory)InspectPrimarySlot();
    if(!weapon)return;
    if(auto eventComponent=Read<void*>(weapon,0xe18)){
      state.event_listener_storage_count=Read<int>(eventComponent,0xe0);
      state.event_listener_free_count=Read<int>(eventComponent,0x10c);
    }
    state.owner_bound=Read<void*>(weapon,0x210)==pawn && Read<void*>(weapon,0x4a8)==pawn;
    state.desired=Read<void*>(inventory,0x268)==weapon;state.current=Read<void*>(inventory,0x280)==weapon;
    mesh1p=Read<void*>(weapon,0xde0);mesh3p=Read<void*>(weapon,0xdf0);stateMachine=Read<void*>(weapon,0xe00);
    state.state_machine_count=Read<int>(weapon,0xe38);state.stability_count=Read<int>(weapon,0xe48);
    state.pending_equippable_changes=Read<int>(inventory,0x2c0);
    state.original_state_machine_enabled=Read<uint8_t>(stateMachine,0x348)!=0;
    state.original_tick_state=Read<uint8_t>(weapon,0x4f0);state.original_tick_contents=Read<uint8_t>(weapon,0x4f1);
    void* currentState=nullptr;
    if(stateMachine)engine::Event(stateMachine,L"/Script/ShooterGame.StateMachineComponent.GetCurrentState",&currentState);
    state.original_current_state_pointer=reinterpret_cast<uintptr_t>(currentState);
    if(ammo){
      // Original AmmoComponent.GetAmmo637fea0 returns ResourceComponent's
      // PredictedResourceAmount+10c. Ammo+12c is the authored initial value,
      // so reading it cannot measure original runtime consumption/reloading.
      state.ammo_property_value=Read<int>(ammo,0x12c);
      state.ammo_predicted_value=Read<int>(ammo,0x10c);state.ammo_authority_value=Read<int>(ammo,0x110);
      state.native_ammo_query_valid=engine::Event(ammo,L"/Script/ShooterGame.AmmoComponent.GetAmmo",&state.ammo);
      state.max_ammo=Read<int>(ammo,0x130);
      if(state.native_ammo_query_valid&&(state.minimum_ammo_observed<0||state.ammo<state.minimum_ammo_observed))state.minimum_ammo_observed=state.ammo;
      auto reserve=Read<void*>(ammo,0x138);state.reserve_component_present=IsA(reserve,engine::Class(0x63c7e00));
      if(state.reserve_component_present){state.reserve_ammo=Read<int>(reserve,0x10c);state.reserve_max_ammo=Read<int>(reserve,0x130);}
      state.native_can_reload_query_valid=engine::Event(ammo,L"/Script/ShooterGame.AmmoComponent.CanReload",&state.native_can_reload);
    }
    if(firing)state.firing_rate=Read<float>(firing,0x44c);
    if(stability){
      engine::Event(stability,L"/Script/ShooterGame.StabilityComponent.GetPitchRecoil",&state.pitch_recoil);
      engine::Event(stability,L"/Script/ShooterGame.StabilityComponent.GetYawRecoil",&state.yaw_recoil);
      engine::Event(stability,L"/Script/ShooterGame.StabilityComponent.GetFiringError",&state.firing_error);
    }
    state.ready=state.owner_bound && state.current && mesh1p && mesh3p && stateMachine && firing && ammo && stability;
    if(state.ready){
      InspectOriginalInputStateFast();
      if(GetTickCount64()-lastInputInspection>=100)InspectOriginalInput();
    }
    if(state.ready)state.stage="original_weapon_equipped_observed";
}
inline bool OriginalResponseClasses(const void* response,void* (&classes)[16],int& classCount){
    classCount=0;
    const int count=Read<int>(response,0x10),capacity=Read<int>(response,0x14);
    const int bits=Read<int>(response,0x30),free=Read<int>(response,0x3c);
    auto entries=Read<uint8_t*>(response,8);auto bitmap=Read<const uint32_t*>(response,0x28);
    if(!bitmap)bitmap=reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(response)+0x18);
    if(count<0||count>128||capacity<count||bits<count||free<0||free>count||count-free>16||(!entries&&count))return false;
    for(int n=0;n<count;++n){
      if(!(bitmap[n/32]&(1u<<(n%32))))continue;
      auto type=Read<void*>(entries,n*16);if(!type)return false;
      classes[classCount++]=type;
    }
    return classCount==count-free;
}
inline bool OriginalResponseFilterMatches(const void* response,void* const* classes,int classCount){
    void* actual[16]{};int count=0;
    if(!OriginalResponseClasses(response,actual,count)||count!=classCount)return false;
    for(int n=0;n<count;++n){bool found=false;for(int k=0;k<count;++k)if(actual[n]==classes[k])found=true;if(!found)return false;}
    return true;
}
inline bool SupplyOriginalLocalEquipResponse(void* targetWeapon,EquipEventRegistration& result){
    auto machine=Read<void*>(targetWeapon,0xe00),inactive=Read<void*>(machine,0x350);
    result.inactive_state=reinterpret_cast<uintptr_t>(inactive);
    if(!IsA(machine,engine::Class(0x67722c0))||Read<void*>(machine,0xc8)!=targetWeapon
      ||!IsA(inactive,engine::Class(0x63a92a0))||Read<void*>(inactive,0xe0)!=machine
      ||Read<void*>(inactive,0xc8)!=targetWeapon){result.error=730;return false;}
    const int count=Read<int>(inactive,0x368),capacity=Read<int>(inactive,0x36c);
    auto entries=Read<uint8_t*>(inactive,0x360);
    result.graph_responses_before=count;
    if(count<0||count>128||capacity<count||(!entries&&count)){result.error=731;return false;}
    void* classes[16]{};int classCount=0;void* next=nullptr;
    for(int n=0;n<count;++n){
      auto response=entries+n*0x60;
      if(Read<uint8_t>(response,0)!=2)continue;
      if(next){result.error=732;return false;} // Do not guess among multiple authored AutoEquip targets.
      next=Read<void*>(response,0x58);
      if(!OriginalResponseClasses(response,classes,classCount)||!classCount){result.error=733;return false;}
    }
    result.auto_equip_response_observed=next!=nullptr;result.equip_target=reinterpret_cast<uintptr_t>(next);
    result.filtered_class_count=classCount;
    if(!IsA(next,engine::Class(0x676b8c0))||Read<void*>(next,0xe0)!=machine||Read<void*>(next,0xc8)!=targetWeapon){result.error=734;return false;}
    for(int n=0;n<count;++n){
      auto response=entries+n*0x60;
      if(Read<uint8_t>(response,0)==0&&Read<void*>(response,0x58)==next&&OriginalResponseFilterMatches(response,classes,classCount)){
        result.graph_responses_after=count;result.graph_valid=true;return true;
      }
    }
    // The authored Gun AutoEquip2 response is present, with its actual class
    // filter and EquipState target. This client's original processor7141820
    // implements Equip0/Unequip1 and returns false for AutoEquip2. Bridge only
    // the missing standalone equipment lifecycle by adding an Equip0 response
    // to that same original target. Original AddEquipResponse_Internal713e820
    // deep-copies the complete class set and preserves the authored AutoEquip2
    // response. Native inventory notification and native state transitions own
    // all subsequent equip/readiness/fire/reload behavior. No state is forced.
    struct ClassArray{void** data;int count,capacity;} array{classes,classCount,classCount};
    static_assert(sizeof(ClassArray)==16);
    engine::function<void(*)(void*,uint8_t,void*,const ClassArray*)>(0x713e820)(inactive,0,next,&array);
    const int after=Read<int>(inactive,0x368);entries=Read<uint8_t*>(inactive,0x360);
    result.graph_responses_after=after;
    if(after!=count+1||!entries){result.error=735;return false;}
    bool retainedAuto=false,addedEquip=false;
    for(int n=0;n<after;++n){
      auto response=entries+n*0x60;
      if(Read<void*>(response,0x58)!=next||!OriginalResponseFilterMatches(response,classes,classCount))continue;
      const auto kind=Read<uint8_t>(response,0);if(kind==2)retainedAuto=true;if(kind==0)addedEquip=true;
    }
    result.local_equip_response_supplied=retainedAuto&&addedEquip;
    result.graph_valid=result.local_equip_response_supplied;
    if(!result.graph_valid)result.error=736;
    return result.graph_valid;
}
inline bool RegisterOriginalOfflineEquipEventFor(void* targetPawn,void* targetWeapon,EquipEventRegistration* observation=nullptr,bool supplyEquipResponse=true){
    EquipEventRegistration result;
    const auto finish=[&](bool ok){if(observation)*observation=result;return ok;};
    if(!OnThread()||!targetWeapon||Read<void*>(targetWeapon,0x4a8)!=targetPawn){result.error=728;return finish(false);}
    if(engine::function<int(*)(void*)>(0x3efc820)(targetWeapon)!=0)return finish(true);
    // In this client image, ServerOnly registration is an empty branch of
    // original RegisterEvent6ff5f40. Original OnEquip70ede80 still creates and
    // emits EquippableEvent_EquippedEvent; without its registration the native
    // runtime logs "event type ... has not been registered yet" and the real
    // RespondToEvent InactiveState cannot consume the equipment lifecycle.
    // Supply this missing local registration before the first real TryEquip.
    // ServerAndClient=0 reuses the shipped registration, TSet and object pool;
    // it changes no NetMode/Role, state pointer, transition, input or fire code.
    auto component=Read<void*>(targetWeapon,0xe18);
    auto registry=Read<void*>(component,0x198);
    auto type=engine::Class(0x6767a30);
    result.component=reinterpret_cast<uintptr_t>(component);result.registry=reinterpret_cast<uintptr_t>(registry);
    result.type=reinterpret_cast<uintptr_t>(type);result.component_enabled=Read<uint8_t>(component,0x189)!=0;
    if(!IsA(component,engine::Class(0x6768f30))||Read<void*>(component,0xc8)!=targetWeapon||!registry||!type
       ||engine::function<void*(*)(void*)>(0x6ff6000)(targetWeapon)!=registry||!result.component_enabled){
        result.error=728;return finish(false);
    }
    result.registered_before=engine::function<bool(*)(void*,void*)>(0x6ff68d0)(registry,type);
    engine::function<void(*)(void*,void*,uint8_t)>(0x6ff5f40)(component,type,0);
    result.registered_after=engine::function<bool(*)(void*,void*)>(0x6ff68d0)(registry,type);
    result.valid=result.registered_after;
    if(!result.valid)result.error=729;
    if(result.valid&&supplyEquipResponse&&!SupplyOriginalLocalEquipResponse(targetWeapon,result))return finish(false);
    return finish(result.valid);
}
inline bool RegisterOriginalOfflineEquipEvent(){
    EquipEventRegistration result;
    const bool ok=RegisterOriginalOfflineEquipEventFor(pawn,weapon,&result);
    state.equipped_event_component_pointer=result.component;state.equipped_event_registry_pointer=result.registry;
    state.equipped_event_class_pointer=result.type;state.equipped_event_component_enabled=result.component_enabled;
    state.equipped_event_registered_before=result.registered_before;state.equipped_event_registered_after=result.registered_after;
    state.equipped_event_local_registration_supplied=!state.equipped_event_registered_before&&state.equipped_event_registered_after;
    state.equipped_event_registration_valid=result.valid;
    state.original_equip_graph_valid=result.graph_valid;state.original_auto_equip_response_observed=result.auto_equip_response_observed;
    state.local_equip_response_supplied=result.local_equip_response_supplied;
    state.original_equip_graph_inactive=result.inactive_state;state.original_equip_graph_target=result.equip_target;
    state.original_equip_graph_responses_before=result.graph_responses_before;state.original_equip_graph_responses_after=result.graph_responses_after;
    state.original_equip_graph_class_count=result.filtered_class_count;
    if(!ok)return Fail(result.error,result.error>=730?"original_equipment_response_graph_not_verified":result.error==729?"original_equipped_event_registration_not_observed":"original_equipped_event_registration_identity_invalid");
    return true;
}
inline bool SpawnAndBindAK(void* inWorld,void* inPawn) {
    if(weapon)return Fail(707,"original_weapon_already_spawned_no_retry");
    if(!Diagnose(inWorld,inPawn))return false;
    if(!EnsurePrimarySlot())return false;
    engine::Vec location{};
    if(!engine::Event(pawn,L"/Script/Engine.Actor.K2_GetActorLocation",&location))return Fail(709,"pawn_location_missing");
    engine::Transform transform;transform.translation=location;
    state.stage="spawning_original_ak_deferred";
    weapon=engine::function<void*(*)(void*,void*,const engine::Transform*,uint8_t,void*,uint8_t)>(0x43fada0)
      (world,weaponClass,&transform,1,nullptr,0);
    if(!weapon)return Fail(710,"original_ak_spawn_failed");
    state.created=true;state.stage="finishing_original_ak_scs_and_beginplay";
    engine::function<void(*)(void*,const engine::Transform*,bool,void*,uint8_t)>(0x3ef6cb0)
      (weapon,&transform,false,nullptr,0);
    if(!original_effects::EnsureActor(weapon,"local_weapon"))
        return Fail(original_effects::State().error,original_effects::State().stage);
    firing=Component(weapon,0x684d4c0);ammo=Component(weapon,0x63c7e00);stability=Component(weapon,0x6d1e460);
    if(!firing||!ammo||!stability)return Fail(711,"original_ak_scs_components_missing");
    state.stage="binding_original_ak_owner";
    // Actor.SetOwner updates engine ownership and notifies the original Actor event.
    // Ares AuthSetOwnerTo is an empty client virtual, so it cannot perform this grant.
    if(!engine::Event(weapon,L"/Script/Engine.Actor.SetOwner",&pawn) || Read<void*>(weapon,0x210)!=pawn)
        return Fail(712,"original_ak_actor_owner_not_observed");
    if(Read<void*>(weapon,0x4a8)!=pawn){
      // Pinned AresEquippable ownership callback: (this,oldOwner,newOwner).
      // It calls AresItem's typed pawn assignment and the original attachment,
      // state-machine, stability, effect and EquippableOnSetOwner notifications.
      auto table=*reinterpret_cast<void***>(weapon);
      if(table[0x7d8/8]!=reinterpret_cast<void*>(engine::image+0x70ec480))
          return Fail(713,"original_ak_owner_callback_changed");
      reinterpret_cast<void(*)(void*,void*,void*)>(table[0x7d8/8])(weapon,nullptr,pawn);
    }
    if(Read<void*>(weapon,0x4a8)!=pawn)return Fail(714,"original_ak_shooter_owner_not_observed");
    if(!RegisterOriginalOfflineEquipEvent())return false;
    state.stage="publishing_local_empty_primary_slot";
    // The client ItemSlot.AuthAddItem is an unconditional false stub. This is
    // narrowly the missing local grant data, not replacement weapon behavior.
    Put(slot,0x38,weapon);Put<uint8_t>(weapon,0x4b0,1);state.empty_slot_published=true;
    if(!engine::Event(slot,L"/Script/ShooterGame.ItemSlot.OnRep_Contents",nullptr))
        return Fail(715,"original_slot_notification_missing");
    state.stage="requesting_original_equip";
    struct Equip { uint8_t source=9, skipRestrictions=0, result=0; } equip;
    if(!engine::Event(weapon,L"/Script/ShooterGame.AresEquippable.TryEquip",&equip))
        return Fail(716,"original_equip_reflection_missing");
    state.equip_requested=equip.result!=0;
    RefreshDiagnostics();
    if(!state.equip_requested)return Fail(717,"original_equip_rejected_no_restriction_bypass");
    if(!state.current)state.stage="waiting_for_original_equip_state";
    return true;
}
inline bool OriginalCharacterInputContextReady(){
    state.character_input_context_valid=false;
    if(!OnThread()||!pawn||!inventory)return false;
    if(!IsA(pawn,engine::Class(0x6c5c480))||!Read<uint8_t>(pawn,0xa90)
      ||!IsA(inventory,engine::Class(0x6499330))||Read<void*>(inventory,0xc8)!=pawn
      ||Read<void*>(inventory,0x2f0)!=pawn||Read<void*>(pawn,0xc08)!=inventory)return false;
    auto abilities=Read<void*>(pawn,0xa20);auto inputState=Read<void*>(pawn,0xba0);
    if(!abilities||!Read<void*>(abilities,0x13a8)||!Read<void*>(pawn,0xf30)||!inputState)return false;
    if(nativeInputState&&nativeInputState!=inputState)return false;
    nativeInputState=inputState;
    if(!nativeInput)nativeInput=Component(pawn,0x6492f30);
    if(!IsA(nativeInput,engine::Class(0x6492f30))||!IsA(nativeInputState,engine::Class(0x6494370))
      ||Read<void*>(nativeInput,0xc8)!=pawn||Read<void*>(nativeInput,0x170)!=pawn
      ||Read<void*>(nativeInputState,0xc8)!=pawn||Read<void*>(nativeInputState,0x148)!=pawn)return false;
    auto current=Read<void*>(inventory,0x280);
    state.character_input_current_equippable=reinterpret_cast<uintptr_t>(current);
    state.character_input_current_is_rifle=current==weapon;
    // ForceInputState and the native buffer belong to the character. The
    // current original equippable may be Jett's knives or another ability;
    // releasing primary/reload and submitting ability events must remain valid.
    // Rifle readiness remains its own diagnostic and is not an input gate.
    state.character_input_context_valid=true;return true;
}
inline bool UpdateInput(uint32_t flags) {
    if(!OnThread()||!pawn)return false;
    RefreshDiagnostics();
    if(!OriginalCharacterInputContextReady())return false;
    if(!state.ready){
      InspectOriginalInputStateFast();
      if(GetTickCount64()-lastInputInspection>=100)InspectOriginalInput();
    }
    // Existing bridge flags fire=1, reload=2, ADS=4, focus=8. The original
    // Character input scheduler supplies native timestamps and state transitions.
    const uint32_t mask=(flags&8)?flags&7:0;
    const uint32_t bits[]{1,4,2};
    for(uint8_t input=0;input<3;++input){
      const uint32_t bit=bits[input];
      if((mask&bit)==(previousInput&bit))continue;
      struct ForceInput { uint8_t input,state; } p{input,static_cast<uint8_t>((mask&bit)?1:0)};
      if(!engine::Event(pawn,L"/Script/ShooterGame.ShooterCharacter.ForceInputState",&p))
          return Fail(718,"original_character_input_reflection_missing");
      if(!state.character_input_current_is_rifle)state.character_input_routed_while_rifle_inactive=true;
      ++state.input_edges;
      InspectOriginalInput();
    }
    previousInput=mask;return true;
}
inline bool SubmitStandaloneInputBuffer(){
    if(!OriginalCharacterInputContextReady())return false;
    if(engine::function<int(*)(void*)>(0x3efc820)(pawn)!=0)return true;
    // The original input Tick70a4530 intentionally returns in Standalone. The
    // ForceInputState-created events in its real FAresInputStateBuffer therefore
    // lack only a local transport. Use the original submission70a7110, which
    // merges/notifies/consumes that buffer; the normal128Hz original input-state
    // tick later owns input changes, gates, state-machine dispatch and firing.
    // Do not invoke an input Tick or HandleInput/Fire manually; do not invent
    // events, spread, recoil, ammo changes, physics, network roles or NetMode.
    auto inputTable=Read<void**>(nativeInput,0), stateTable=Read<void**>(nativeInputState,0);
    if(inputTable!=reinterpret_cast<void**>(engine::image+0xb099220)
       ||stateTable!=reinterpret_cast<void**>(engine::image+0xb093c10)
       ||Read<void*>(nativeInput,0xc8)!=pawn||Read<void*>(nativeInput,0x170)!=pawn
       ||Read<void*>(nativeInputState,0xc8)!=pawn||Read<void*>(nativeInputState,0x148)!=pawn)
        return Fail(725,"original_standalone_input_submission_identity_mismatch");
    const int pending=Read<int>(nativeInput,0x8b8),capacity=Read<int>(nativeInput,0x8bc);
    if(pending<0||pending>1024||capacity<pending|| (pending&&!Read<void*>(nativeInput,0x8b0)))
        return Fail(726,"original_standalone_input_buffer_invalid");
    state.standalone_input_submission_valid=true;
    if(!pending)return true;
    if(!state.character_input_current_is_rifle)state.character_input_routed_while_rifle_inactive=true;
    state.submitted_input_pending_before=pending;
    engine::function<void(*)(void*,void*)>(0x70a7110)
        (nativeInputState,static_cast<uint8_t*>(nativeInput)+0x178);
    state.submitted_input_pending_after=Read<int>(nativeInput,0x8b8);
    if(state.submitted_input_pending_after)return Fail(727,"original_standalone_input_buffer_not_consumed");
    ++state.standalone_input_submissions;state.standalone_input_events+=static_cast<uint32_t>(pending);
    InspectOriginalInput();return true;
}
inline bool WriteDiagnostics(const wchar_t* path) {
    if(!OnThread()||!path)return false;
    RefreshDiagnostics();
    FILE* f=nullptr;if(_wfopen_s(&f,path,L"wb")||!f)return false;
    const int count=std::fprintf(f,
      "{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u,\"class_loaded\":%d,\"cdo_loaded\":%d,"
      "\"original_pawn\":%d,\"initialized_pawn\":%d,\"inventory_owner_valid\":%d,\"providers_valid\":%d,"
      "\"created\":%d,\"owner_bound\":%d,\"local_empty_slot_published\":%d,\"equip_requested\":%d,"
      "\"desired\":%d,\"current\":%d,\"ready\":%d,\"ammo\":%d,\"max_ammo\":%d,"
      "\"firing_rate\":%.6f,\"pitch_recoil\":%.6f,\"yaw_recoil\":%.6f,\"firing_error\":%.6f,"
      "\"movement_ratio\":%.6f,\"state_machine_count\":%d,\"stability_count\":%d,\"input_edges\":%u,"
      "\"primary_slot_present\":%d,\"primary_slot_is_item\":%d,\"primary_slot_is_multi\":%d,"
      "\"primary_slot_backref_valid\":%d,\"primary_slot_outer_valid\":%d,\"primary_slot_occupied\":%d,"
      "\"primary_slot_locally_created\":%d,\"populated_slot_count\":%u,"
      "\"primary_slot_pointer\":%llu,\"primary_slot_class_pointer\":%llu,\"primary_slot_backref_pointer\":%llu,\"primary_slot_contents_pointer\":%llu,"
      "\"pending_equippable_changes\":%d,\"original_state_machine_enabled\":%d,\"original_tick_state\":%u,\"original_tick_contents\":%u,\"original_current_state_pointer\":%llu,"
      "\"original_firing_runtime_verified\":false",
      state.stage,state.error,state.class_loaded,state.cdo_loaded,state.original_pawn,state.initialized_pawn,
      state.inventory_owner_valid,state.providers_valid,state.created,state.owner_bound,state.empty_slot_published,
      state.equip_requested,state.desired,state.current,state.ready,state.ammo,state.max_ammo,state.firing_rate,
      state.pitch_recoil,state.yaw_recoil,state.firing_error,state.movement_ratio,state.state_machine_count,
      state.stability_count,state.input_edges,state.primary_slot_present,state.primary_slot_is_item,state.primary_slot_is_multi,
      state.primary_slot_backref_valid,state.primary_slot_outer_valid,state.primary_slot_occupied,state.primary_slot_locally_created,
      state.populated_slot_count,static_cast<unsigned long long>(state.primary_slot_pointer),static_cast<unsigned long long>(state.primary_slot_class_pointer),
      static_cast<unsigned long long>(state.primary_slot_backref_pointer),static_cast<unsigned long long>(state.primary_slot_contents_pointer),
      state.pending_equippable_changes,state.original_state_machine_enabled,static_cast<unsigned>(state.original_tick_state),
      static_cast<unsigned>(state.original_tick_contents),static_cast<unsigned long long>(state.original_current_state_pointer));
    const auto& s=state;
#define WEAPON_INPUT_BOOL(m) std::fprintf(f,",\"" #m "\":%s",s.m?"true":"false")
#define WEAPON_INPUT_INT(m) std::fprintf(f,",\"" #m "\":%d",static_cast<int>(s.m))
#define WEAPON_INPUT_PTR(m) std::fprintf(f,",\"" #m "\":%llu",static_cast<unsigned long long>(s.m))
    WEAPON_INPUT_BOOL(can_fire);WEAPON_INPUT_BOOL(native_can_fire_query_valid);WEAPON_INPUT_BOOL(native_input_queries_valid);
    WEAPON_INPUT_BOOL(input_component_present);WEAPON_INPUT_BOOL(native_input_gate_valid);WEAPON_INPUT_BOOL(native_input_gate);
    WEAPON_INPUT_BOOL(input_component_owner_valid);WEAPON_INPUT_BOOL(input_component_pawn_valid);
    WEAPON_INPUT_INT(input_owner_net_mode);WEAPON_INPUT_INT(input_component_pending);WEAPON_INPUT_INT(input_state_pending);
    WEAPON_INPUT_INT(max_input_component_pending);WEAPON_INPUT_INT(max_input_state_pending);
    WEAPON_INPUT_INT(current_primary_input);WEAPON_INPUT_INT(current_secondary_input);WEAPON_INPUT_INT(current_reload_input);
    WEAPON_INPUT_INT(input_state_primary);WEAPON_INPUT_INT(input_state_secondary);WEAPON_INPUT_INT(input_state_reload);
    WEAPON_INPUT_BOOL(primary_input_down);WEAPON_INPUT_BOOL(secondary_input_down);WEAPON_INPUT_BOOL(reload_input_down);
    WEAPON_INPUT_PTR(input_component_pointer);WEAPON_INPUT_PTR(input_component_owner_pointer);WEAPON_INPUT_PTR(input_component_pawn_pointer);
    WEAPON_INPUT_PTR(input_state_pointer);
    WEAPON_INPUT_BOOL(standalone_input_submission_valid);
    WEAPON_INPUT_INT(standalone_input_submissions);WEAPON_INPUT_INT(standalone_input_events);
    WEAPON_INPUT_INT(submitted_input_pending_before);WEAPON_INPUT_INT(submitted_input_pending_after);
    WEAPON_INPUT_BOOL(primary_pressed_observed);WEAPON_INPUT_BOOL(primary_down_observed);WEAPON_INPUT_BOOL(can_fire_observed);
    WEAPON_INPUT_BOOL(secondary_pressed_observed);WEAPON_INPUT_BOOL(reload_pressed_observed);
    WEAPON_INPUT_INT(primary_pressed_samples);WEAPON_INPUT_INT(primary_down_samples);WEAPON_INPUT_INT(input_inventory_listener_kind);
    WEAPON_INPUT_PTR(input_inventory_listener_pointer);
    WEAPON_INPUT_BOOL(input_inventory_callback_original);WEAPON_INPUT_BOOL(input_inventory_listener_resolves_owner);
    WEAPON_INPUT_BOOL(state_machine_observers_installed);WEAPON_INPUT_INT(observed_machine_count);
    WEAPON_INPUT_BOOL(equipped_event_registration_valid);WEAPON_INPUT_BOOL(equipped_event_registered_before);
    WEAPON_INPUT_BOOL(equipped_event_registered_after);WEAPON_INPUT_BOOL(equipped_event_local_registration_supplied);
    WEAPON_INPUT_BOOL(equipped_event_component_enabled);WEAPON_INPUT_PTR(equipped_event_component_pointer);
    WEAPON_INPUT_PTR(equipped_event_registry_pointer);WEAPON_INPUT_PTR(equipped_event_class_pointer);
    WEAPON_INPUT_INT(event_listener_storage_count);WEAPON_INPUT_INT(event_listener_free_count);
    WEAPON_INPUT_BOOL(native_ammo_query_valid);WEAPON_INPUT_INT(ammo_property_value);WEAPON_INPUT_INT(ammo_predicted_value);
    WEAPON_INPUT_INT(ammo_authority_value);WEAPON_INPUT_INT(minimum_ammo_observed);
    WEAPON_INPUT_BOOL(reserve_component_present);WEAPON_INPUT_INT(reserve_ammo);WEAPON_INPUT_INT(reserve_max_ammo);
    WEAPON_INPUT_BOOL(native_can_reload_query_valid);WEAPON_INPUT_BOOL(native_can_reload);
    WEAPON_INPUT_BOOL(character_input_context_valid);WEAPON_INPUT_BOOL(character_input_current_is_rifle);
    WEAPON_INPUT_BOOL(character_input_routed_while_rifle_inactive);WEAPON_INPUT_PTR(character_input_current_equippable);
    WEAPON_INPUT_BOOL(original_equip_graph_valid);WEAPON_INPUT_BOOL(original_auto_equip_response_observed);WEAPON_INPUT_BOOL(local_equip_response_supplied);
    WEAPON_INPUT_PTR(original_equip_graph_inactive);WEAPON_INPUT_PTR(original_equip_graph_target);
    WEAPON_INPUT_INT(original_equip_graph_responses_before);WEAPON_INPUT_INT(original_equip_graph_responses_after);WEAPON_INPUT_INT(original_equip_graph_class_count);
    const auto jsonText=[&](const char* value){
      std::fputc('"',f);
      for(const auto* at=reinterpret_cast<const unsigned char*>(value);*at;++at){
        if(*at=='"'||*at=='\\')std::fputc('\\',f);
        if(*at<32)std::fprintf(f,"\\u%04x",static_cast<unsigned>(*at));else std::fputc(*at,f);
      }
      std::fputc('"',f);
    };
    std::fprintf(f,",\"machines\":[");
    for(unsigned n=0;n<s.observed_machine_count;++n){
      if(n)std::fputc(',',f);const auto& m=s.machines[n];
      std::fprintf(f,"{\"name\":");jsonText(m.name);std::fprintf(f,",\"auth_name\":");jsonText(m.auth_name);
      std::fprintf(f,",\"client_name\":");jsonText(m.client_name);
#define WEAPON_MACHINE_INT(k) std::fprintf(f,",\"" #k "\":%llu",static_cast<unsigned long long>(m.k))
#define WEAPON_MACHINE_NUM(k) std::fprintf(f,",\"" #k "\":%.9g",static_cast<double>(m.k))
#define WEAPON_MACHINE_BOOL(k) std::fprintf(f,",\"" #k "\":%s",m.k?"true":"false")
      WEAPON_MACHINE_INT(component);WEAPON_MACHINE_INT(auth_state);WEAPON_MACHINE_INT(client_state);WEAPON_MACHINE_INT(inactive_state);
      WEAPON_MACHINE_INT(process_calls);WEAPON_MACHINE_INT(process_with_pending_calls);WEAPON_MACHINE_INT(tick_calls);WEAPON_MACHINE_INT(positive_tick_calls);
      WEAPON_MACHINE_INT(pending_before);WEAPON_MACHINE_INT(pending_after);WEAPON_MACHINE_INT(max_pending);
      WEAPON_MACHINE_INT(last_input);WEAPON_MACHINE_INT(last_action);WEAPON_MACHINE_INT(last_activation);
      WEAPON_MACHINE_NUM(input_timestamp);WEAPON_MACHINE_NUM(input_ttl);WEAPON_MACHINE_NUM(input_clock);WEAPON_MACHINE_NUM(input_age);WEAPON_MACHINE_NUM(last_tick_delta);
      WEAPON_MACHINE_BOOL(auth_firing_observed);WEAPON_MACHINE_BOOL(client_firing_observed);
      WEAPON_MACHINE_BOOL(inactive_is_response_state);WEAPON_MACHINE_BOOL(inactive_machine_owner_valid);WEAPON_MACHINE_BOOL(inactive_response_map_valid);
      WEAPON_MACHINE_INT(inactive_response_storage_count);WEAPON_MACHINE_INT(inactive_response_free_count);WEAPON_MACHINE_INT(inactive_response_count);
      WEAPON_MACHINE_INT(starting_state);std::fprintf(f,",\"starting_state_name\":");jsonText(m.starting_state_name);
      std::fprintf(f,",\"inactive_responses\":[");
      for(unsigned r=0;r<m.captured_response_count;++r){
        if(r)std::fputc(',',f);const auto& response=m.responses[r];
        std::fprintf(f,"{\"event_class\":%llu,\"next_state\":%llu,\"event_name\":",static_cast<unsigned long long>(response.event_class),static_cast<unsigned long long>(response.next_state));
        jsonText(response.event_name);std::fprintf(f,",\"next_state_name\":");jsonText(response.next_state_name);std::fputc('}',f);
      }
      std::fputc(']',f);
      WEAPON_MACHINE_BOOL(inactive_inventory_responses_valid);WEAPON_MACHINE_BOOL(inactive_native_pawn_owner_valid);
      WEAPON_MACHINE_BOOL(inactive_inventory_equipped_binding);WEAPON_MACHINE_BOOL(inactive_inventory_unequipped_binding);WEAPON_MACHINE_BOOL(inactive_owner_set_binding);
      WEAPON_MACHINE_INT(inactive_inventory_response_count);
      std::fprintf(f,",\"inactive_inventory_responses\":[");
      for(unsigned r=0;r<m.captured_inventory_response_count;++r){
        if(r)std::fputc(',',f);const auto& response=m.inventory_responses[r];
        std::fprintf(f,"{\"equip_type\":%u,\"filtered_class_count\":%d,\"next_state\":%llu,\"next_state_name\":",static_cast<unsigned>(response.equip_type),response.filtered_class_count,static_cast<unsigned long long>(response.next_state));
        jsonText(response.next_state_name);std::fputc('}',f);
      }
      std::fputc(']',f);
#undef WEAPON_MACHINE_INT
#undef WEAPON_MACHINE_NUM
#undef WEAPON_MACHINE_BOOL
      std::fputc('}',f);
    }
    std::fputc(']',f);
#undef WEAPON_INPUT_BOOL
#undef WEAPON_INPUT_INT
#undef WEAPON_INPUT_PTR
    std::fprintf(f,"}\n");
    std::fclose(f);return count>0;
}
}
