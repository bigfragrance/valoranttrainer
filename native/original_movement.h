#pragma once
#include "original_players.h"
#include <array>
#include <cstddef>

// Fixed image: work/duel1-original-movement/report.json.
// This adapter uses the original ShooterCharacter + ShooterCharacterMovement
// and their original tuning/attribute/equippable chain. It writes no speed,
// gravity, jump, capsule, eye-height, crouch-compression or CDO values.
// The normal original movement tick owns simulation and crouch interpolation.
namespace original_movement {
inline constexpr uintptr_t MovementClass = 0x6c7d380;
inline constexpr uintptr_t AttributeClass = 0x63e58b0;
enum class MovementType : uint8_t { Walking=0, Running=1, Jumping=2, Crouching=3,
                                  OnAscender=4, Flying=5, Stationary=6 };
enum class MoveInput : uint8_t { AlternateMovement=0, Crouch=1 };
struct MovementValues {
    float acceleration=0, friction=0, frictionMin=0, brakingFriction=0, maxSpeed=0;
    uint32_t alignmentPadding=0;
    double inputDirectionalMultipliers[2]{};
};
static_assert(sizeof(MovementValues)==0x28 && offsetof(MovementValues,inputDirectionalMultipliers)==0x18);
struct MovementTuning {
    MovementValues base;
    std::array<MovementValues,7> states{};
    float minimumAboveMaxSpeedDecayRate=0, jumpLandSlowMaxSpeedDecayRate=0;
};
static_assert(sizeof(MovementTuning)==0x148);
struct JumpTuning {
    float maxJumpHeight=0, jumpTotalTime=0;
    float derivedJumpVelocity=0, unreflectedDerived12=0;
    float accelerationSpeedLimit=0, lateralAngleThreshold=0, counterSteerMultiplier=0, counterSteerStartAngle=0;
};
static_assert(sizeof(JumpTuning)==0x20);
struct SourceValues {
    void* pawn=nullptr;
    void* movement=nullptr;
    bool valid=false;
    float engineJumpZVelocity=0, engineMaxWalkSpeed=0, engineMaxWalkSpeedCrouched=0, engineGravityScale=0;
    float crouchTime=0, crouchCompression=0, capsuleHalfHeight=0;
    MovementTuning tuning;
    JumpTuning jump;
};
struct Diagnostics {
    bool ready=false, dependencies_ready=false, has_executed_moves=false, locally_controlled=false;
    bool jump_held=false, walk_held=false, crouch_held=false, movement_disabled=false;
    bool walk_override_active=false, crouch_override_active=false, input_overrides_verified=false;
    uint8_t walk_override_state=0, crouch_override_state=0;
    unsigned error=0;
    const char* stage="original_movement_not_initialized";
    void *pawn=nullptr, *controller=nullptr, *movement=nullptr;
    void *attributes=nullptr, *forceModules=nullptr, *activeTuning=nullptr, *equipped=nullptr;
    uint32_t total_moves=0;
    uint8_t movement_type=0, crouch_transition=0;
    float jump_velocity=0, full_speed_modifier=0, current_max_speed=0, actual_capsule_half_height=0;
    float true_vertical_velocity=0, equipped_movement_ratio=0;
    engine::Vec player_space_velocity{};
    std::array<float,7> maximum_speed_by_type{};
    SourceValues original_defaults, instance_values;
};
inline Diagnostics state;
inline const Diagnostics& State() { return state; }
inline bool requestedJump=false, requestedWalk=false, requestedCrouch=false;

template<class T> inline bool Read(void* object,size_t offset,T& value) { return original_players::Read(object,offset,value); }
template<class T> inline T Value(void* object,size_t offset) { return original_players::Value<T>(object,offset); }
inline bool Fail(unsigned error,const char* stage) { state.error=error;state.stage=stage;return false; }
inline bool FinitePositive(float x) { return std::isfinite(x)&&x>0; }
inline bool ProbeSource(void* pawn,SourceValues& out) {
    out={};out.pawn=pawn;
    if(!original_players::IsA(pawn,original_players::ShooterCharacterClass))return false;
    out.movement=Value<void*>(pawn,0x4f8);
    if(!original_players::IsA(out.movement,MovementClass))return false;
    void* capsule=Value<void*>(pawn,0x500);
    out.valid=Read(out.movement,0x1bc,out.engineJumpZVelocity)
        &&Read(out.movement,0x2b8,out.engineMaxWalkSpeed)&&Read(out.movement,0x2bc,out.engineMaxWalkSpeedCrouched)
        &&Read(out.movement,0x1b0,out.engineGravityScale)&&Read(out.movement,0x1250,out.tuning)
        &&Read(out.movement,0x13a0,out.jump)&&Read(pawn,0x112c,out.crouchTime)
        &&Read(pawn,0x1134,out.crouchCompression)&&Read(capsule,0x6d8,out.capsuleHalfHeight);
    return out.valid;
}
inline bool Dependencies() {
    state.dependencies_ready=false;
    if(!state.pawn||!state.movement||!state.controller)return false;
    // These methods dereference the owner, active attributes, force-module
    // multipliers and active tuning without null guards in the pinned client.
    if(Value<void*>(state.pawn,0x4a0)!=state.controller||Value<void*>(state.controller,0x4b0)!=state.pawn
        ||Value<void*>(state.movement,0x190)!=state.pawn||Value<void*>(state.movement,0x1a8)!=state.pawn
        ||!Value<void*>(state.movement,0xd8)||!Value<uint8_t>(state.pawn,0xa90))return false;
    uint8_t local=0;
    state.locally_controlled=engine::Event(state.pawn,L"/Script/Engine.Pawn.IsLocallyControlled",&local)&&local!=0;
    if(!state.locally_controlled)return false; // SetCrouchHeld intentionally respects local ownership.
    auto abilities=Value<void*>(state.pawn,0xa20);
    state.attributes=Value<void*>(abilities,0x13a8);
    state.forceModules=Value<void*>(state.pawn,0x1100);
    state.activeTuning=Value<void*>(state.movement,0x1398);
    if(!original_players::IsA(state.attributes,AttributeClass)
        ||!original_players::Readable(state.forceModules,0x1ac)
        ||!original_players::Readable(state.activeTuning,0x148))return false;
    const auto world=original_players::world;
    const auto gameState=Value<void*>(world,0x178);
    if(!original_players::IsA(gameState,original_players::ShooterGameStateClass))return false;
    auto fixedStep=Value<void*>(gameState,0xcc0);
    if(!Value<uint8_t>(fixedStep,0xd9))return false;
    state.dependencies_ready=true;return true;
}
inline bool QueryFloat(const wchar_t* name,float& result) {
    result=0;
    return engine::Event(state.movement,name,&result)&&std::isfinite(result);
}
inline bool QueryBool(void* object,const wchar_t* name,bool& result) {
    uint8_t value=0;
    if(!engine::Event(object,name,&value))return false;
    result=value!=0;return true;
}
inline bool Sample() {
    if(!state.ready||!original_players::Thread())return false;
    if(!Dependencies())return Fail(751,"original_movement_dependencies_not_ready");
    if(!ProbeSource(state.pawn,state.instance_values))return Fail(752,"original_movement_values_unreadable");
    state.total_moves=Value<uint32_t>(state.movement,0x14b8);
    state.crouch_transition=Value<uint8_t>(state.pawn,0x1139);
    state.walk_override_state=Value<uint8_t>(state.pawn,0xac0);
    state.walk_override_active=Value<uint8_t>(state.pawn,0xac1)!=0;
    state.crouch_override_state=Value<uint8_t>(state.pawn,0xac2);
    state.crouch_override_active=Value<uint8_t>(state.pawn,0xac3)!=0;
    state.input_overrides_verified=state.walk_override_active&&state.crouch_override_active
        &&state.walk_override_state==static_cast<uint8_t>(requestedWalk)
        &&state.crouch_override_state==static_cast<uint8_t>(requestedCrouch);
    state.actual_capsule_half_height=state.instance_values.capsuleHalfHeight;
    if(!QueryBool(state.movement,L"/Script/ShooterGame.ShooterCharacterMovement.HasExecutedAnyMoves",state.has_executed_moves)
        ||!QueryBool(state.movement,L"/Script/ShooterGame.ShooterCharacterMovement.MovementIsDisabled",state.movement_disabled)
        ||!QueryBool(state.pawn,L"/Script/ShooterGame.ShooterCharacter.GetJumpHeld",state.jump_held)
        ||!QueryBool(state.pawn,L"/Script/ShooterGame.ShooterCharacter.GetWalkHeld",state.walk_held)
        ||!QueryBool(state.pawn,L"/Script/ShooterGame.ShooterCharacter.GetCrouchHeld",state.crouch_held)
        ||!QueryFloat(L"/Script/ShooterGame.ShooterCharacterMovement.GetJumpVelocity",state.jump_velocity)
        ||!QueryFloat(L"/Script/ShooterGame.ShooterCharacterMovement.GetFullMovementSpeedModifier",state.full_speed_modifier)
        ||!QueryFloat(L"/Script/ShooterGame.ShooterCharacterMovement.GetTrueVerticalVelocity",state.true_vertical_velocity))
        return Fail(753,"original_movement_query_failed");
    if(!engine::Event(state.movement,L"/Script/ShooterGame.ShooterCharacterMovement.GetMovementType",&state.movement_type)
        ||!engine::Event(state.movement,L"/Script/ShooterGame.ShooterCharacterMovement.GetPlayerSpaceCurrentVelocity",&state.player_space_velocity))
        return Fail(754,"original_movement_type_or_velocity_failed");
    for(uint8_t type=0;type<7;++type) {
        struct Params { uint8_t type=0;uint8_t pad[3]{};float modifier=0;float result=0; } params;
        static_assert(sizeof(Params)==12&&offsetof(Params,modifier)==4&&offsetof(Params,result)==8);
        params.type=type;params.modifier=state.full_speed_modifier;
        if(!engine::Event(state.movement,L"/Script/ShooterGame.ShooterCharacterMovement.GetMaxSpeedForMovementType",&params)
            ||!std::isfinite(params.result))return Fail(756,"original_posture_speed_query_failed");
        state.maximum_speed_by_type[type]=params.result;
    }
    // Ask the original reflected speed calculation for each original enum
    // category, then select the category reported by the real component.
    state.current_max_speed=state.movement_type<7?state.maximum_speed_by_type[state.movement_type]:0;
    const auto inventory=Value<void*>(state.pawn,0xc08);
    state.equipped=Value<void*>(inventory,0x268);
    state.equipped_movement_ratio=0;
    if(state.equipped)Read(state.equipped,0xdb4,state.equipped_movement_ratio);
    if(!std::isfinite(state.current_max_speed)||!original_players::Finite(state.player_space_velocity))
        return Fail(757,"original_movement_result_nonfinite");
    state.error=0;state.stage="original_movement_ready";return true;
}
inline bool Initialize(void* controller,void* pawn) {
    if(!original_players::Thread())return false;
    state={};state.controller=controller;state.pawn=pawn;
    if(!original_players::IsA(pawn,original_players::ShooterCharacterClass)
        ||!original_players::IsA(controller,original_players::ShooterControllerClass))return Fail(758,"original_movement_requires_original_player");
    state.movement=Value<void*>(pawn,0x4f8);
    if(!original_players::IsA(state.movement,MovementClass))return Fail(759,"original_movement_component_missing");
    auto type=Value<void*>(pawn,0x10);
    auto defaults=type?engine::DefaultObject(type):nullptr;
    if(!ProbeSource(defaults,state.original_defaults))return Fail(760,"original_movement_cdo_unreadable");
    if(!Dependencies())return Fail(751,"original_movement_dependencies_not_ready");
    requestedJump=Value<uint8_t>(pawn,0xf7b)&1;
    requestedWalk=Value<uint8_t>(pawn,0xf78)&1;
    requestedCrouch=Value<uint8_t>(pawn,0x1538)&1;
    state.ready=true;return Sample();
}
inline bool Held(const wchar_t* method,bool value) {
    uint8_t parameter=value?1:0;
    return engine::Event(state.pawn,method,&parameter);
}
inline bool ForceMovementInput(MoveInput input,bool pressed){
    // Original enum metadata:c283ff0 AlternateMovement0,Crouch1.
    // Reflection size2:MoveInput@0,EAresInputState@1 (Released0,Pressed1).
    // The original7881110 posture updater reads78826c0 each actor tick and
    // deliberately replaces transient SetWalkHeld/SetCrouchHeld values with
    // the native PlayerInput bindings. This original force interface supplies
    // the intended persistent override to that same query, not capsule/speed.
    struct Params{uint8_t input,state;} p{static_cast<uint8_t>(input),static_cast<uint8_t>(pressed)};
    static_assert(sizeof(Params)==2);
    if(!engine::Event(state.pawn,L"/Script/ShooterGame.ShooterCharacter.ForceMoveInputState",&p))return false;
    const auto offset=0xac0+static_cast<size_t>(p.input)*2;
    return Value<uint8_t>(state.pawn,offset)==p.state&&Value<uint8_t>(state.pawn,offset+1)!=0;
}
inline bool Update(float forward,float right,bool jump,bool walk,bool crouch,bool inputAllowed=true) {
    if(!state.ready||!original_players::Thread())return false;
    if(!Dependencies())return Fail(751,"original_movement_dependencies_not_ready");
    if(!std::isfinite(forward)||!std::isfinite(right)||std::abs(forward)>1||std::abs(right)>1)
        return Fail(761,"original_movement_invalid_axes");
    if(!inputAllowed){forward=right=0;jump=walk=crouch=false;}
    if((walk!=requestedWalk||!Value<uint8_t>(state.pawn,0xac1))
       &&!ForceMovementInput(MoveInput::AlternateMovement,walk))return Fail(762,"original_walk_input_failed");
    if((crouch!=requestedCrouch||!Value<uint8_t>(state.pawn,0xac3))
       &&!ForceMovementInput(MoveInput::Crouch,crouch))return Fail(763,"original_crouch_input_failed");
    if(jump!=requestedJump&&!Held(L"/Script/ShooterGame.ShooterCharacter.SetJumpHeld",jump))return Fail(764,"original_jump_input_failed");
    requestedWalk=walk;requestedCrouch=crouch;requestedJump=jump;
    // Original MoveForward/Right transform control rotation and accumulate
    // native input. Do not multiply by a local .45 slow-walk constant.
    if(!engine::Event(state.pawn,L"/Script/ShooterGame.ShooterCharacter.MoveForward",&forward)
        ||!engine::Event(state.pawn,L"/Script/ShooterGame.ShooterCharacter.MoveRight",&right))return Fail(765,"original_axis_input_failed");
    state.error=0;state.stage="original_inputs_submitted";return true;
}
inline void ReleaseInputs(bool restoreNativeBindings=false) {
    if(!state.ready||!original_players::Thread())return;
    Update(0,0,false,false,false,false);
    // Send Released even if the cache already held false. Optional teardown
    // returns ownership to the original keyboard bindings through its own API.
    ForceMovementInput(MoveInput::AlternateMovement,false);
    ForceMovementInput(MoveInput::Crouch,false);
    if(restoreNativeBindings){
        uint8_t input=static_cast<uint8_t>(MoveInput::AlternateMovement);
        engine::Event(state.pawn,L"/Script/ShooterGame.ShooterCharacter.StopForceMoveInput",&input);
        input=static_cast<uint8_t>(MoveInput::Crouch);
        engine::Event(state.pawn,L"/Script/ShooterGame.ShooterCharacter.StopForceMoveInput",&input);
    }
}
}
