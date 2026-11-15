// Included inside local_server_native_test, after the existing original input
// and exact owner helpers. One bounded client target input state machine.
#include "local_server_peek_input_profiles.inl"
namespace peek_client {
struct Snapshot {
    bool started=false,finished=false,failed=false,crouchRequested=false,crouchHeldObserved=false,crouchReleasedObserved=false;
    bool moveOverrideCleared=false,retiredPrivateIntent=false,releaseUnavailable=false,primaryHeldObserved=false,primaryReleasedObserved=false;
    uint32_t error=0,presses=0,releases=0,crouchPresses=0,crouchReleases=0;
    uint32_t kind=0,pawnRole=0;uint64_t nonce=0,ownershipMask=0;
    bool configured=false,cliMatched=false,armed=false,actionsCompleted=false;
    uint64_t controlSequence=0;double seconds=0;const char*stage="peek_fixture_client_waiting_original_owner";
};
inline Snapshot live{},published{};
inline SRWLOCK lock=SRWLOCK_INIT;
inline bool profiles=false,methods=false,aimSet=false;
inline void *forceMove=nullptr,*stopMove=nullptr,*getCrouch=nullptr;
inline Bound forceMoveId{},stopMoveId{},getCrouchId{};
inline Bound weapon{};
inline std::atomic<bool> stopRequested{false};
inline double startedAt=-1,nextControlRead=0;
inline void Publish(){
    live.nonce=state.marker.nonce;live.kind=local_server_peek_fixture::context.contract.kind;
    live.configured=state.configured;live.cliMatched=state.cliMatched;live.armed=state.armed;live.actionsCompleted=state.actionsCompleted;
    live.ownershipMask=state.ownershipPassedMask;live.pawnRole=state.pawnRole;
    AcquireSRWLockExclusive(&lock);published=live;ReleaseSRWLockExclusive(&lock);
}
inline Snapshot Copy(){AcquireSRWLockShared(&lock);const auto copy=published;ReleaseSRWLockShared(&lock);return copy;}
}
inline bool IsPeek(){return state.marker.scenario==local_server_peek_fixture::Scenario;}
inline bool PeekLiveOwner(){
    if(!state.bound||state.ownershipPassedMask!=0x1fff||GetCurrentThreadId()!=original_players::gameThread)return false;
    for(const auto&b:state.identities)if(!b.object||Resolve(b.identity)!=b.object)return false;
    const bool links=Read<uint8_t>(state.pawn,0x224)==2&&Read<uint8_t>(state.pc,0x224)==2
        &&Read<void*>(state.pc,0x4b0)==state.pawn&&Read<void*>(state.pawn,0x4a0)==state.pc
        &&Read<void*>(state.pc,0x470)==state.ps&&Read<void*>(state.pawn,0x488)==state.ps
        &&Read<int32_t>(state.connection,0x13c)==3;
    const auto&alive=state.targetMethods[static_cast<size_t>(TargetMethod::Alive)];uint8_t value=0;
    return links&&alive.object&&Resolve(alive.identity)==alive.object&&Event(state.pawn,alive.object,&value)&&value==1;
}
inline bool PeekMethods(){
    using namespace peek_client;
    if(!profiles){for(const auto&p:PeekInputProfiles)if(!ProfileMatches(p))return Fail(7840,"peek_fixture_pinned_original_move_input_profile_changed");profiles=true;}
    if(methods)return Resolve(forceMoveId.identity)==forceMove&&Resolve(stopMoveId.identity)==stopMove&&Resolve(getCrouchId.identity)==getCrouch;
    const auto base=reinterpret_cast<void*>(engine::image);
    // Original enum table: EAresMoveInput::Crouch1, distinct from equippable
    // ForceInputState. Native function metadata and complete bytes gate the ABI.
    const auto moveName=Read<const char*>(base,0xc284000),pressedName=Read<const char*>(base,0xc285a20);
    if(!moveName||!pressedName||!original_players::Readable(moveName,21)||!original_players::Readable(pressedName,24)
       ||std::strcmp(moveName,"EAresMoveInput::Crouch")||std::strcmp(pressedName,"EAresInputState::Pressed")
       ||Read<uint64_t>(base,0xc284008)!=1||Read<uint64_t>(base,0xc285a28)!=1||Read<uint64_t>(base,0xc285a18)!=0)
        return Fail(7841,"peek_fixture_original_move_input_enum_metadata_changed");
    if(!Function(forceMove,L"/Script/ShooterGame.ShooterCharacter.ForceMoveInputState",2,0x04020401,0x6c6b190)
       ||!Function(stopMove,L"/Script/ShooterGame.ShooterCharacter.StopForceMoveInput",1,0x04020401,0x6c6b080)
       ||!Function(getCrouch,L"/Script/ShooterGame.ShooterCharacter.GetCrouchHeld",1,0x54020401,0x6c64b00))return false;
    const auto owner=engine::function<void*(*)()>(0x6c5c480)();
    for(auto object:{forceMove,stopMove,getCrouch})if(Read<void*>(object,0x28)!=owner)return Fail(7842,"peek_fixture_move_input_owner_class_changed");
    forceMoveId.object=forceMove;stopMoveId.object=stopMove;getCrouchId.object=getCrouch;
    if(!Identity(forceMove,forceMoveId.identity)||!Identity(stopMove,stopMoveId.identity)||!Identity(getCrouch,getCrouchId.identity))return Fail(7842,"peek_fixture_move_input_function_weak_identity_unavailable");
    methods=true;return true;
}
inline bool PeekForceCrouch(bool held){
    if(!PeekLiveOwner()||!PeekMethods())return false;
    struct Params {uint8_t input=1,value=0;}parameters{1,static_cast<uint8_t>(held?1:0)};
    static_assert(sizeof(Params)==2);if(!Event(state.pawn,peek_client::forceMove,&parameters))return false;
    peek_client::live.crouchRequested=held;
    if(held)++peek_client::live.crouchPresses;else ++peek_client::live.crouchReleases;return true;
}
inline bool PeekQueryCrouch(uint8_t&held){
    if(!PeekLiveOwner()||!PeekMethods())return false;held=0xfe;
    return Event(state.pawn,peek_client::getCrouch,&held)&&held<=1;
}
inline bool PeekCleanup(){
    using namespace peek_client;
    if(!IsPeek())return true;
    if(!PeekLiveOwner()){
        live.retiredPrivateIntent=state.primaryHeldRequested||live.crouchRequested;
        live.releaseUnavailable=live.retiredPrivateIntent;state.primaryHeldRequested=false;live.crouchRequested=false;Publish();return !live.releaseUnavailable;
    }
    bool okay=true;if(state.primaryHeldRequested){okay=ForcePrimary(false);if(okay)++live.releases;}
    if(live.crouchRequested&&!PeekForceCrouch(false))okay=false;
    if(live.crouchPresses&&methods){uint8_t held=0xfe;
        if(!PeekQueryCrouch(held))okay=false;
        else if(held==0){
            live.crouchReleasedObserved=true;uint8_t input=1;
            if(!Event(state.pawn,stopMove,&input))okay=false;
            else live.moveOverrideCleared=Read<uint8_t>(state.pawn,0xac3)==0;
        }
    }Publish();return okay;
}
inline bool PeekAbort(uint32_t error,const char*stage){
    const bool cleaned=PeekCleanup();peek_client::live.failed=true;peek_client::live.finished=true;
    peek_client::live.error=error;peek_client::live.stage=stage;if(!cleaned)peek_client::live.releaseUnavailable=true;
    peek_client::Publish();return Fail(error,stage);
}
inline bool TickPeekImpl(void*world,void*pc,void*ps,void*pawn,float deltaSeconds){
    using namespace peek_client;namespace fixture=local_server_peek_fixture;
    if(!original_players::gameThread||GetCurrentThreadId()!=original_players::gameThread)return true;
    if(stopRequested.exchange(false,std::memory_order_relaxed))state.stopRequested=true;
    if(!fixture::context.configured||fixture::context.host||fixture::context.contract.nonce!=state.marker.nonce)
        return PeekAbort(7843,"peek_fixture_marker_requires_matching_owned_contract");
    if(!fixture::Arm())return PeekAbort(fixture::context.error,fixture::context.stage);
    if(!fixture::context.armed)return true;
    if(!ValidateOwner(world,pc,ps,pawn)){PeekCleanup();live.failed=true;live.finished=true;live.error=state.error;live.stage=state.stage;return false;}
    if(!state.bound||state.ownershipPassedMask!=0x1fff){
        if(startedAt>=0)return PeekAbort(7844,"peek_fixture_owner_lost_after_case_start");
        if(state.elapsedSeconds*1000>=state.marker.waitMs)return PeekAbort(7845,"peek_fixture_original_target_owner_timeout");
        return true;
    }
    if(!ResolveMethods()||!ResolveTargetMethods()||!PeekMethods())return PeekAbort(state.error?state.error:7846,"peek_fixture_original_input_abi_unavailable");
    if(state.stopRequested)return PeekAbort(7847,"peek_fixture_explicit_stop_guarded_inputs_released");
    if(state.elapsedSeconds*1000>=state.marker.totalMs)return PeekAbort(7848,"peek_fixture_original_target_case_total_deadline");
    if(!QueryPrimary()||state.primaryNow>1)return PeekAbort(7849,"peek_fixture_original_primary_readback_unavailable");
    if(startedAt<0){
        const bool primaryReady=state.current&&Read<void*>(state.current,0x210)==pawn&&Read<void*>(state.current,0x4a8)==pawn
            &&Read<void*>(state.inventory,0x268)==state.current&&state.primaryNow==0&&PredictedIdle();
        if(!primaryReady){state.readySeconds=0;return true;}
        state.readySeconds+=deltaSeconds;if(state.readySeconds*1000<state.marker.warmupMs)return true;
        weapon.object=state.current;if(!Identity(state.current,weapon.identity))return true;
        engine::Vec rotation{0,fixture::context.contract.targetYaw,0};
        if(!Event(state.pc,TargetFunction(TargetMethod::SetRotation),&rotation))return PeekAbort(7850,"peek_fixture_authored_safe_view_input_failed");
        engine::Vec readback{};if(!Event(state.pc,TargetFunction(TargetMethod::GetRotation),&readback)
           ||!original_players::Finite(readback)||std::abs(std::remainder(readback.y-rotation.y,360.))>.25)
            return PeekAbort(7850,"peek_fixture_authored_safe_view_readback_failed");
        aimSet=true;startedAt=state.elapsedSeconds;live.started=true;live.stage="peek_fixture_bounded_original_target_input_active";
    }
    live.seconds=state.elapsedSeconds-startedAt;
    const auto kind=fixture::context.contract.kind;
    if(state.current!=weapon.object||Resolve(weapon.identity)!=weapon.object)return PeekAbort(7857,"peek_fixture_original_primary_equippable_source_changed");
    if(kind==6){
        const double t=live.seconds;const bool want=(t>=2&&t<2.5)||(t>=5&&t<5.5)||(t>=8&&t<8.5);
        if(want!=state.primaryHeldRequested){
            engine::Vec actual{};
            if(want&&(!PeekLiveOwner()||!Event(state.pc,TargetFunction(TargetMethod::GetRotation),&actual)||!original_players::Finite(actual)
               ||std::abs(actual.x)>.25||std::abs(std::remainder(actual.y-fixture::context.contract.targetYaw,360.))>.25))return PeekAbort(7858,"peek_fixture_authored_safe_view_changed_before_original_primary");
            if(!ForcePrimary(want))return PeekAbort(7851,"peek_fixture_original_finite_primary_edge_failed");
            if(want)++live.presses;else ++live.releases;}
        if(state.primaryHeldRequested&&state.primaryNow==1)live.primaryHeldObserved=true;
        if(live.presses&&!state.primaryHeldRequested&&state.primaryNow==0)live.primaryReleasedObserved=true;
    }
    if(kind==8&&state.elapsedSeconds>=nextControlRead){
        nextControlRead=state.elapsedSeconds+.05;fixture::Control control;
        if(fixture::ReadControl(control)&&control.sequence>live.controlSequence){
            live.controlSequence=control.sequence;
            if(control.action==2)return PeekAbort(7852,"peek_fixture_host_requested_guarded_release");
            if(control.action==1&&!live.crouchRequested&&!live.crouchPresses&&!PeekForceCrouch(true))
                return PeekAbort(7853,"peek_fixture_real_acquire_original_crouch_edge_failed");
        }
    }
    if(live.crouchRequested){uint8_t held=0xfe;if(!PeekQueryCrouch(held))return PeekAbort(7854,"peek_fixture_original_crouch_readback_unavailable");
        if(held==1)live.crouchHeldObserved=true;}
    if(live.seconds>=20){
        if(!PeekCleanup())return PeekAbort(7855,"peek_fixture_guarded_release_unavailable");
        if(!QueryPrimary()||state.primaryNow!=0)return true;
        if(live.crouchPresses&&(!live.crouchReleasedObserved||!live.moveOverrideCleared))return true;
        if(kind==6&&(!live.primaryHeldObserved||!live.primaryReleasedObserved))return PeekAbort(7856,"peek_fixture_primary_request_not_observed");
        if(kind==8&&live.crouchPresses&&!live.crouchHeldObserved)return PeekAbort(7859,"peek_fixture_crouch_request_not_observed");
        // Missing actual acquisition/occlusion is an external not-covered case,
        // not a reason to fabricate a target or silently change memory limits.
        state.finished=true;state.actionsCompleted=true;state.phase=Phase::Complete;
        live.finished=true;live.stage="peek_fixture_target_inputs_released_geometry_and_purpose_pending";state.stage=live.stage;
    }return true;
}
inline bool TickPeek(void*world,void*pc,void*ps,void*pawn,float deltaSeconds){
    if(!original_players::gameThread||GetCurrentThreadId()!=original_players::gameThread)return true;
    const bool okay=TickPeekImpl(world,pc,ps,pawn,deltaSeconds);peek_client::Publish();return okay;
}
inline void WritePeekFields(FILE*file,const peek_client::Snapshot&s){
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(file,"\"peek_fixture\":{\"scenario\":13,\"started\":%s,\"finished\":%s,\"failed\":%s,\"error\":%u,\"stage\":\"%s\",\"seconds\":%.9g,\"primary_presses\":%u,\"primary_releases\":%u,\"primary_held_observed\":%s,\"primary_released_observed\":%s,\"crouch_presses\":%u,\"crouch_releases\":%u,\"crouch_held_observed\":%s,\"crouch_released_observed\":%s,\"move_override_cleared\":%s,\"retired_private_intent\":%s,\"release_unavailable\":%s,\"host_geometry_verified\":false,\"native_consumption_known\":false},",b(s.started),b(s.finished),b(s.failed),s.error,s.stage,s.seconds,s.presses,s.releases,b(s.primaryHeldObserved),b(s.primaryReleasedObserved),s.crouchPresses,s.crouchReleases,b(s.crouchHeldObserved),b(s.crouchReleasedObserved),b(s.moveOverrideCleared),b(s.retiredPrivateIntent),b(s.releaseUnavailable));
}
inline void WritePeek(FILE*file){WritePeekFields(file,peek_client::Copy());}
inline void WritePeekState(FILE*file){
    const auto s=peek_client::Copy();const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(file,"{\"schema_version\":1,\"scope\":\"explicit_original_native_target_input_fixture\",\"scenario\":13,\"nonce\":%llu,\"kind\":%u,\"configured\":%s,\"cli_matched\":%s,\"armed\":%s,\"finished\":%s,\"failed\":%s,\"actions_completed\":%s,\"error\":%u,\"ownership_passed_mask\":%llu,\"pawn_role\":%u,",static_cast<unsigned long long>(s.nonce),s.kind,b(s.configured),b(s.cliMatched),b(s.armed),b(s.finished),b(s.failed),b(s.actionsCompleted),s.error,static_cast<unsigned long long>(s.ownershipMask),s.pawnRole);
    WritePeekFields(file,s);std::fputs("\"passed\":false,\"physical_keyboard_mouse_verified\":false,\"server_effects_verified\":false,\"native_consumption_known\":false}",file);
}
inline bool StopPeek(){
    peek_client::stopRequested.store(true,std::memory_order_relaxed);
    if(!original_players::gameThread||GetCurrentThreadId()!=original_players::gameThread)return true;
    if(state.finished)return PeekCleanup();
    state.stopRequested=true;return TickPeek(state.world,state.pc,state.ps,state.pawn,0);
}
