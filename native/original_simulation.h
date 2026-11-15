#pragma once
#include "original_movement.h"

// Pinned-image evidence: work/duel1-original-simulation/report.json.
// Selects the original standalone movement branch and observes its normal tick.
// Never calls Tick/PerformMovement manually and never changes actor roles.
namespace original_simulation {
inline constexpr uintptr_t RemoteMovementClass=0x6b662e0;
inline constexpr uintptr_t FeatureTogglesClass=0x6876390;
inline constexpr uint8_t BotMovementDowntickingFeature=81;
struct Diagnostics {
    bool bound=false, enable_requested=false, active=false, tick_enabled=false;
    bool original_manager=false, manager_matches=false, manager_registered=false;
    bool native_tick_gate=false, moves_advanced=false;
    uint32_t error=0, initial_moves=0, total_moves=0;
    uint8_t local_role=0, remote_role=0, primary_tick_flags=0, primary_tick_state=0;
    uint8_t actor_tick_flags=0, actor_tick_state=0, actor_begin_play_flags=0, world_type=0;
    bool actor_tick_registered=false, actor_tick_capability=false, movement_registered=false;
    bool fog_valid=false, fog_owner_relevant=false, fog_owner_hidden=false, fog_received_update=false;
    bool movement_observer_installed=false, has_valid_movement_data=false;
    bool last_should_skip_update=false;
    bool local_render_gate_changed=false,render_gate_was_set=false;
    bool local_branch_selected=false,feature_query_valid=false;
    bool bot_downticking_before=false,bot_downticking_after=false;
    bool bot_downticking_current=false,feature_setter_called=false;
    int32_t feature_value_count=0;
    uint64_t movement_tick_calls=0, movement_tick_completed=0;
    uint64_t perform_movement_calls=0,perform_movement_completed=0;
    float perform_movement_delta=0;
    engine::Vec physics_position_before{},physics_position_after{},physics_velocity_after{};
    float movement_last_delta=0;
    int movement_last_tick_type=0;
    uint32_t movement_last_moves_before=0,movement_last_moves_after=0;
    uint8_t movement_mode=0,default_land_mode=0,updated_mobility=0,damage_handler_activity=0;
    uint8_t character_activity_override=0,character_corpse_tick_flag=0;
    uint8_t update_only_if_rendered=0;
    uint8_t inventory_simulation_enabled=0;
    float clock_accumulated_seconds=0;
    int32_t clock_current_step=0,clock_step_size=0;
    int32_t clock_virtual_base_offset=0;
    bool clock_predicate_queried=false,clock_predicate_ready=false;
    float clock_time_shift_adjustment=0;
    const char* stage="original_simulation_unbound";
    void *pawn=nullptr, *movement=nullptr, *game_state=nullptr, *remote_manager=nullptr, *registered_manager=nullptr;
    void *actor_tick_internal=nullptr, *actor_tick_manager=nullptr, *fog=nullptr, *fog_manager=nullptr;
    void *updated_component=nullptr, *damage_handler=nullptr;
    void *server_prediction=nullptr,*client_prediction=nullptr,*movement_clock=nullptr;
    void *clock_vtable=nullptr,*clock_predicate=nullptr,*time_shift_monitor=nullptr;
    void *feature_toggles=nullptr,*feature_setter=nullptr;
};
inline Diagnostics state;
using ComponentTick=void(*)(void*,float,int,void*);
using PerformMovement=void(*)(void*,float);
inline ComponentTick savedMovementTick=nullptr;
inline PerformMovement savedPerformMovement=nullptr;
inline void** originalMovementTable=nullptr;
inline void** observedMovementTable=nullptr;
inline void ObserveMovementTick(void* component,float delta,int tickType,void* tickFunction){
    ++state.movement_tick_calls;state.movement_last_delta=delta;state.movement_last_tick_type=tickType;
    state.movement_last_moves_before=original_players::Value<uint32_t>(component,0x14b8);
    auto table=original_players::Value<void**>(component,0);
    auto skip=original_players::Value<void*>(table,0x4f8);
    if(skip==reinterpret_cast<void*>(engine::image+0x41dfca0))
        state.last_should_skip_update=reinterpret_cast<bool(*)(void*,float)>(skip)(component,delta);
    savedMovementTick(component,delta,tickType,tickFunction);
    state.movement_last_moves_after=original_players::Value<uint32_t>(component,0x14b8);
    ++state.movement_tick_completed;
}
inline void ObservePerformMovement(void* component,float delta){
    ++state.perform_movement_calls;state.perform_movement_delta=delta;
    auto updated=original_players::Value<void*>(component,0xd8);
    state.physics_position_before=original_players::Value<engine::Vec>(updated,0x320);
    savedPerformMovement(component,delta);
    state.physics_position_after=original_players::Value<engine::Vec>(updated,0x320);
    state.physics_velocity_after=original_players::Value<engine::Vec>(component,0xf0);
    ++state.perform_movement_completed;
}
inline const Diagnostics& State(){return state;}
template<class T> inline T Value(void* p,size_t offset){return original_players::Value<T>(p,offset);}
inline bool Fail(uint32_t error,const char* stage){state.error=error;state.stage=stage;return false;}
inline bool BoolQuery(const wchar_t* name,bool& result){
    uint8_t p=0;
    if(!engine::Event(state.movement,name,&p))return false;
    result=p!=0;return true;
}
inline bool QueryOriginalMovementBranch(){
    // Original movement passes GetWorld() to6e08cf0. Its original getter
    //6e08e10 follows World.PersistentLevel -> AresWorldSettings -> component.
    // It is not GameState.RemoteClientMovement or a replacement feature table.
    state.feature_query_valid=false;
    state.feature_toggles=engine::function<void*(*)(void*)>(0x6e08e10)(original_players::world);
    if(!original_players::IsA(state.feature_toggles,FeatureTogglesClass))
        return Fail(797,"simulation_original_feature_component_missing");
    const auto values=Value<uint8_t*>(state.feature_toggles,0x118);
    state.feature_value_count=Value<int32_t>(state.feature_toggles,0x120);
    const auto capacity=Value<int32_t>(state.feature_toggles,0x124);
    // Original constructor7007010 allocates exactly202 enum-indexed bools.
    if(state.feature_value_count!=202||capacity<state.feature_value_count
       ||!original_players::Readable(values,static_cast<size_t>(state.feature_value_count)))
        return Fail(798,"simulation_original_feature_values_invalid");
    auto table=Value<void**>(state.feature_toggles,0);
    state.feature_setter=Value<void*>(table,0x4d8);
    if(table!=reinterpret_cast<void**>(engine::image+0xb2b4b80)
       ||state.feature_setter!=reinterpret_cast<void*>(engine::image+0x7002070))
        return Fail(799,"simulation_original_feature_setter_changed");
    state.bot_downticking_current=engine::function<bool(*)(void*,uint8_t)>(0x6e08cf0)
        (original_players::world,BotMovementDowntickingFeature);
    if(state.bot_downticking_current!=(values[BotMovementDowntickingFeature]!=0))
        return Fail(800,"simulation_original_feature_query_disagrees");
    state.feature_query_valid=true;return true;
}
inline bool Refresh(){
    if(!state.bound||!original_players::Thread())return false;
    state.local_role=Value<uint8_t>(state.pawn,0x224);
    state.remote_role=Value<uint8_t>(state.pawn,0x128);
    state.actor_tick_flags=Value<uint8_t>(state.pawn,0x3a);
    state.actor_tick_state=Value<uint8_t>(state.pawn,0x3b);
    state.actor_tick_capability=(state.actor_tick_flags&2)!=0;
    state.actor_begin_play_flags=Value<uint8_t>(state.pawn,0x11c);
    state.actor_tick_internal=Value<void*>(state.pawn,0x50);
    state.actor_tick_registered=(Value<uint8_t>(state.actor_tick_internal,0)&1)!=0;
    state.actor_tick_manager=Value<void*>(state.actor_tick_internal,0x28);
    state.movement_registered=(Value<uint8_t>(state.movement,0xa0)&1)!=0;
    state.world_type=Value<uint8_t>(original_players::world,0x152);
    state.fog=Value<void*>(state.pawn,0xa68);
    state.fog_valid=original_players::IsA(state.fog,0x676b970);
    state.fog_owner_relevant=state.fog_valid&&Value<uint8_t>(state.fog,0x1e1)!=0;
    state.fog_owner_hidden=state.fog_valid&&Value<uint8_t>(state.fog,0x1e2)!=0;
    state.fog_received_update=state.fog_valid&&Value<uint8_t>(state.fog,0x238)!=0;
    state.fog_manager=state.fog_valid?Value<void*>(state.fog,0x338):nullptr;
    state.movement_mode=Value<uint8_t>(state.movement,0x271);
    state.default_land_mode=Value<uint8_t>(state.movement,0x578);
    state.updated_component=Value<void*>(state.movement,0xd8);
    state.updated_mobility=Value<uint8_t>(state.updated_component,0x253);
    state.damage_handler=Value<void*>(state.pawn,0xc70);
    state.damage_handler_activity=Value<uint8_t>(state.damage_handler,0x1f9);
    state.character_activity_override=Value<uint8_t>(state.pawn,0x1440);
    state.character_corpse_tick_flag=Value<uint8_t>(state.pawn,0x1443);
    state.inventory_simulation_enabled=Value<uint8_t>(state.pawn,0xc18);
    state.update_only_if_rendered=Value<uint8_t>(state.movement,0x138)&1;
    // These are real prediction objects created by their original lazy getters.
    // Observe existing pointers only; do not allocate or alter either timeline.
    state.server_prediction=Value<void*>(state.movement,0x7f0);
    state.client_prediction=Value<void*>(state.movement,0x7e8);
    state.movement_clock=state.local_role==3&&state.server_prediction?
        static_cast<uint8_t*>(state.server_prediction)+0xf0:
        (state.local_role==2&&state.client_prediction?static_cast<uint8_t*>(state.client_prediction)+0x1b0:nullptr);
    state.clock_accumulated_seconds=Value<float>(state.movement_clock,0x20);
    state.clock_current_step=state.clock_step_size=0;
    auto virtualBaseTable=Value<void*>(state.movement_clock,0x10);
    int32_t displacement=Value<int32_t>(virtualBaseTable,4);
    state.clock_virtual_base_offset=displacement;
    if(state.movement_clock&&virtualBaseTable&&displacement>=0&&displacement<0x1000){
        state.clock_current_step=Value<int32_t>(state.movement_clock,static_cast<size_t>(displacement)+0x18);
        state.clock_step_size=Value<int32_t>(state.movement_clock,static_cast<size_t>(displacement)+0x1c);
    }
    state.clock_vtable=Value<void*>(state.movement_clock,0);
    state.clock_predicate=Value<void*>(state.clock_vtable,0x40);
    state.time_shift_monitor=Value<void*>(state.movement,0x1530);
    state.clock_time_shift_adjustment=Value<float>(state.time_shift_monitor,0xf0);
    state.clock_predicate_queried=false;state.clock_predicate_ready=false;
    if(state.clock_predicate==reinterpret_cast<void*>(engine::image+0x789f280)
       ||state.clock_predicate==reinterpret_cast<void*>(engine::image+0x789f440)){
        state.clock_predicate_ready=reinterpret_cast<bool(*)(void*,void*,float)>(state.clock_predicate)
            (state.movement_clock,state.time_shift_monitor,0.0f);
        state.clock_predicate_queried=true;
    }
    auto movementTable=Value<void**>(state.movement,0);
    auto validQuery=Value<void*>(movementTable,0x6c0);
    state.has_valid_movement_data=validQuery&&reinterpret_cast<bool(*)(void*)>(validQuery)(state.movement);
    state.primary_tick_flags=Value<uint8_t>(state.movement,0x52);
    state.primary_tick_state=Value<uint8_t>(state.movement,0x53);
    // Exact SetComponentTickEnabled implementation tests component+52 bit2.
    state.native_tick_gate=(state.primary_tick_flags&2)!=0;
    state.total_moves=Value<uint32_t>(state.movement,0x14b8);
    state.moves_advanced=state.total_moves>state.initial_moves;
    state.game_state=Value<void*>(original_players::world,0x178);
    if(!original_players::IsA(state.game_state,original_players::ShooterGameStateClass))
        return Fail(781,"simulation_original_game_state_missing");
    state.remote_manager=Value<void*>(state.game_state,0xcc8);
    state.original_manager=original_players::IsA(state.remote_manager,RemoteMovementClass);
    state.registered_manager=Value<void*>(state.movement,0x15d8);
    state.manager_matches=state.registered_manager==state.remote_manager&&state.remote_manager;
    state.manager_registered=Value<uint8_t>(state.movement,0x15e0)!=0;
    if(!BoolQuery(L"/Script/Engine.ActorComponent.IsActive",state.active)
       ||!BoolQuery(L"/Script/Engine.ActorComponent.IsComponentTickEnabled",state.tick_enabled))
        return Fail(782,"simulation_component_query_failed");
    return true;
}
inline bool SelectOriginalLocalMovementBranch(){
    if(!state.bound||!original_players::Thread())return false;
    if(!original_movement::Dependencies()||!original_movement::State().locally_controlled
       ||state.local_role!=3)return Fail(801,"simulation_local_branch_requires_original_local_authority");
    if(!QueryOriginalMovementBranch())return false;
    if(state.local_branch_selected)return !state.bot_downticking_current
        ||Fail(802,"simulation_original_local_branch_changed");
    state.bot_downticking_before=state.bot_downticking_current;
    if(state.bot_downticking_before){
        // Native MulticastSetGameFeatureEnabled implementation slot4d8 has ABI
        //void(this,uint8,bool). Call that exact local implementation, preserving
        //its7002430 change notification. No RPC dispatch or raw bool writes.
        state.feature_setter_called=true;
        reinterpret_cast<void(*)(void*,uint8_t,bool)>(state.feature_setter)
            (state.feature_toggles,BotMovementDowntickingFeature,false);
    }
    if(!QueryOriginalMovementBranch())return false;
    state.bot_downticking_after=state.bot_downticking_current;
    if(state.bot_downticking_after)return Fail(803,"simulation_original_local_branch_readback_failed");
    state.local_branch_selected=true;state.error=0;
    state.stage="simulation_original_local_branch_selected";return true;
}
inline bool Initialize(void* originalPawn){
    if(!original_players::Thread())return false;
    state={};state.pawn=originalPawn;
    if(!original_movement::State().ready||original_movement::State().pawn!=originalPawn
       ||!original_movement::Dependencies())return Fail(783,"simulation_original_movement_not_ready");
    state.movement=original_movement::State().movement;
    if(!original_players::IsA(state.movement,original_movement::MovementClass))
        return Fail(784,"simulation_original_component_missing");
    auto table=Value<void**>(state.movement,0);
    if(Value<void*>(table,0x3f8)!=reinterpret_cast<void*>(engine::image+0x7894ab0))
        return Fail(785,"simulation_original_tick_override_changed");
    state.initial_moves=Value<uint32_t>(state.movement,0x14b8);
    state.bound=true;
    if(!Refresh())return false;
    state.stage="simulation_diagnostics_ready";return true;
}
inline bool EnableNativeComponentTick(){
    if(!state.bound||!original_players::Thread())return false;
    if(!original_movement::Dependencies()||!Refresh())return Fail(783,"simulation_original_movement_not_ready");
    if(!state.original_manager)return Fail(786,"simulation_remote_movement_manager_missing");
    if(!state.native_tick_gate)return Fail(787,"simulation_native_tick_capability_disabled");
    // Activate(bReset=true) and SetComponentTickEnabled(bool) both have an exact
    // one-byte parameter layout in this image. Normal world tick subsequently
    // calls7894ab0 and its own78924b0 manager registration when15e0 is unset.
    uint8_t enabled=1;state.enable_requested=true;
    if(!engine::Event(state.movement,L"/Script/Engine.ActorComponent.Activate",&enabled)
       ||!engine::Event(state.movement,L"/Script/Engine.ActorComponent.SetComponentTickEnabled",&enabled))
        return Fail(788,"simulation_native_activation_call_failed");
    if(!Refresh())return false;
    if(!state.active||!state.tick_enabled)return Fail(789,"simulation_activation_readback_failed");
    state.error=0;state.stage="simulation_native_tick_enabled_waiting_for_moves";
    return true;
}
inline bool InstallMovementTickObserver(){
    if(!state.bound||!original_players::Thread())return false;
    if(state.movement_observer_installed)return true;
    if(!original_movement::Dependencies())return Fail(783,"simulation_original_movement_not_ready");
    auto expected=reinterpret_cast<void**>(engine::image+0xb4ecc50);
    if(Value<void**>(state.movement,0)!=expected||expected[0x3f8/8]!=reinterpret_cast<void*>(engine::image+0x7894ab0)
       ||expected[0xa98/8]!=reinterpret_cast<void*>(engine::image+0x78954d0))
        return Fail(790,"simulation_observer_table_mismatch");
    // Exact primary table:400 code pointers; the following entry atb4ed8d0 is
    // non-code data. Keep RTTI and every slot; observe TickComponent and its
    // original PerformMovement dispatch without changing either execution.
    auto storage=static_cast<void**>(VirtualAlloc(nullptr,401*sizeof(void*),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!storage)return Fail(791,"simulation_observer_allocation_failed");
    std::memcpy(storage,expected-1,401*sizeof(void*));
    auto copied=storage+1;
    savedMovementTick=reinterpret_cast<ComponentTick>(copied[0x3f8/8]);
    copied[0x3f8/8]=reinterpret_cast<void*>(&ObserveMovementTick);
    // Exact native frame executor ABI:78954d0(this,floatDelta) calls the
    // original418d8c0 PerformMovement and preserves its original execution.
    savedPerformMovement=reinterpret_cast<PerformMovement>(copied[0xa98/8]);
    copied[0xa98/8]=reinterpret_cast<void*>(&ObservePerformMovement);
    if(InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(state.movement),copied,expected)!=expected){
        VirtualFree(storage,0,MEM_RELEASE);return Fail(792,"simulation_observer_component_changed");
    }
    originalMovementTable=expected;observedMovementTable=copied;
    // This tiny table and loaded callback remain owned by the game session.
    // No manual tick is issued, no scheduler/tuning/roles are altered.
    state.movement_observer_installed=true;state.error=0;state.stage="simulation_observing_original_movement_tick";
    return true;
}
inline bool DisableLocalRenderVisibilityGate(){
    if(!state.bound||!original_players::Thread())return false;
    if(!original_movement::Dependencies()||!original_movement::State().locally_controlled)
        return Fail(793,"simulation_render_gate_requires_local_original_pawn");
    // Exact generated property: /Script/Engine.MovementComponent.bUpdateOnlyIfRendered,
    // descriptor aaefc10; bool setter41deb50 ORs component+138 with1.
    // No reflected setter exists. Preserve all other packed flags and CDOs.
    uint8_t flags=Value<uint8_t>(state.movement,0x138);
    state.render_gate_was_set=(flags&1)!=0;
    if(flags&1){
        flags=static_cast<uint8_t>(flags&~uint8_t(1));
        if(!original_players::Write(state.movement,0x138,flags))return Fail(794,"simulation_render_gate_write_failed");
        state.local_render_gate_changed=true;
    }
    if(!Refresh()||state.update_only_if_rendered)return Fail(795,"simulation_render_gate_readback_failed");
    auto table=Value<void**>(state.movement,0);
    auto skip=Value<void*>(table,0x4f8);
    if(skip!=reinterpret_cast<void*>(engine::image+0x41dfca0))return Fail(796,"simulation_skip_update_query_changed");
    state.last_should_skip_update=reinterpret_cast<bool(*)(void*,float)>(skip)(state.movement,state.movement_last_delta);
    state.error=0;state.stage=state.last_should_skip_update?
        "simulation_render_gate_cleared_other_skip_remains":"simulation_local_render_gate_disabled";
    return true;
}
inline bool WriteDiagnostics(const wchar_t* path){
    // Export only cached observations. No engine calls, pointer dereferences,
    // lookups, or changes to the owned game are performed from this exporter.
    if(!path||!*path||wcslen(path)>960)return false;
    wchar_t temporary[1024]{};
    if(swprintf_s(temporary,L"%ls.tmp",path)<0)return false;
    FILE* file=nullptr;
    if(_wfopen_s(&file,temporary,L"wb")!=0||!file)return false;
    const auto& s=state;
    std::fprintf(file,"{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u",s.stage,s.error);
    const auto number=[&](const char* key,double value){
        std::fprintf(file,",\"%s\":",key);
        if(std::isfinite(value))std::fprintf(file,"%.17g",value);else std::fprintf(file,"null");
    };
    const auto vector=[&](const char* key,const engine::Vec& value){
        std::fprintf(file,",\"%s\":[",key);
        const double xyz[]{value.x,value.y,value.z};
        for(int n=0;n<3;++n){if(n)std::fprintf(file,",");if(std::isfinite(xyz[n]))std::fprintf(file,"%.17g",xyz[n]);else std::fprintf(file,"null");}
        std::fprintf(file,"]");
    };
#define SIM_BOOL(m) std::fprintf(file,",\"" #m "\":%s",s.m?"true":"false")
#define SIM_UINT(m) std::fprintf(file,",\"" #m "\":%llu",static_cast<unsigned long long>(s.m))
#define SIM_INT(m) std::fprintf(file,",\"" #m "\":%d",static_cast<int>(s.m))
#define SIM_PTR(m) std::fprintf(file,",\"" #m "\":\"%p\"",s.m)
#define SIM_NUMBER(m) number(#m,s.m)
    SIM_BOOL(bound);SIM_BOOL(enable_requested);SIM_BOOL(active);SIM_BOOL(tick_enabled);
    SIM_BOOL(original_manager);SIM_BOOL(manager_matches);SIM_BOOL(manager_registered);
    SIM_BOOL(native_tick_gate);SIM_BOOL(moves_advanced);SIM_BOOL(actor_tick_registered);SIM_BOOL(actor_tick_capability);
    SIM_BOOL(movement_registered);SIM_BOOL(fog_valid);SIM_BOOL(fog_owner_relevant);SIM_BOOL(fog_owner_hidden);SIM_BOOL(fog_received_update);
    SIM_BOOL(movement_observer_installed);SIM_BOOL(has_valid_movement_data);SIM_BOOL(last_should_skip_update);
    SIM_BOOL(local_render_gate_changed);SIM_BOOL(render_gate_was_set);SIM_BOOL(clock_predicate_queried);SIM_BOOL(clock_predicate_ready);
    SIM_BOOL(local_branch_selected);SIM_BOOL(feature_query_valid);SIM_BOOL(feature_setter_called);
    SIM_BOOL(bot_downticking_before);SIM_BOOL(bot_downticking_after);SIM_BOOL(bot_downticking_current);
    SIM_INT(feature_value_count);
    SIM_UINT(initial_moves);SIM_UINT(total_moves);SIM_UINT(local_role);SIM_UINT(remote_role);SIM_UINT(primary_tick_flags);SIM_UINT(primary_tick_state);
    SIM_UINT(actor_tick_flags);SIM_UINT(actor_tick_state);SIM_UINT(actor_begin_play_flags);SIM_UINT(world_type);
    SIM_UINT(movement_tick_calls);SIM_UINT(movement_tick_completed);SIM_UINT(perform_movement_calls);SIM_UINT(perform_movement_completed);
    SIM_UINT(movement_last_moves_before);SIM_UINT(movement_last_moves_after);SIM_UINT(movement_mode);SIM_UINT(default_land_mode);
    SIM_UINT(updated_mobility);SIM_UINT(damage_handler_activity);SIM_UINT(character_activity_override);SIM_UINT(character_corpse_tick_flag);
    SIM_UINT(update_only_if_rendered);SIM_UINT(inventory_simulation_enabled);
    SIM_INT(movement_last_tick_type);SIM_INT(clock_current_step);SIM_INT(clock_step_size);SIM_INT(clock_virtual_base_offset);
    SIM_NUMBER(perform_movement_delta);SIM_NUMBER(movement_last_delta);SIM_NUMBER(clock_accumulated_seconds);SIM_NUMBER(clock_time_shift_adjustment);
    vector("physics_position_before",s.physics_position_before);vector("physics_position_after",s.physics_position_after);
    vector("physics_velocity_after",s.physics_velocity_after);
    SIM_PTR(pawn);SIM_PTR(movement);SIM_PTR(game_state);SIM_PTR(remote_manager);SIM_PTR(registered_manager);
    SIM_PTR(actor_tick_internal);SIM_PTR(actor_tick_manager);SIM_PTR(fog);SIM_PTR(fog_manager);SIM_PTR(updated_component);SIM_PTR(damage_handler);
    SIM_PTR(server_prediction);SIM_PTR(client_prediction);SIM_PTR(movement_clock);SIM_PTR(clock_vtable);SIM_PTR(clock_predicate);SIM_PTR(time_shift_monitor);
    SIM_PTR(feature_toggles);SIM_PTR(feature_setter);
#undef SIM_BOOL
#undef SIM_UINT
#undef SIM_INT
#undef SIM_PTR
#undef SIM_NUMBER
    std::fprintf(file,"}");
    const bool written=std::ferror(file)==0;
    const bool closed=std::fclose(file)==0;
    return written&&closed&&MoveFileExW(temporary,path,MOVEFILE_REPLACE_EXISTING);
}
}
