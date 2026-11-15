// Scenario14 role2 participant observes the normal original spawn only. It
// never sends a movement, weapon, skill or model input to the SUT or human.
namespace hold_client {
struct Snapshot {
    bool configured=false,cliMatched=false,armed=false,bound=false,finished=false,failed=false,actionsCompleted=false;
    uint32_t error=0;uint64_t nonce=0;const char* stage="map_hold_client_not_observed_before_GT";
};
static_assert(std::is_trivially_copyable_v<Snapshot>);
inline Snapshot live{},cached{};inline SRWLOCK lock=SRWLOCK_INIT;
inline std::atomic<bool> stop{false};inline uint64_t lastPoll=0;
inline void Publish(){
    live.configured=state.configured;live.cliMatched=state.cliMatched;live.armed=state.armed;live.bound=state.bound;
    live.finished=state.finished;live.failed=state.failed;live.actionsCompleted=state.actionsCompleted;
    live.error=state.error;live.nonce=state.marker.nonce;live.stage=state.stage;
    AcquireSRWLockExclusive(&lock);cached=live;ReleaseSRWLockExclusive(&lock);
}
inline Snapshot Copy(){AcquireSRWLockShared(&lock);const auto result=cached;ReleaseSRWLockShared(&lock);return result;}
}
inline bool IsMapHold(){return state.marker.scenario==local_server_hold_fixture::Scenario;}
inline bool TickMapHoldBody(void* world,void* pc,void* ps,void* pawn){
    namespace fixture=local_server_hold_fixture;
    if(!fixture::context.configured||fixture::context.host||fixture::context.contract.nonce!=state.marker.nonce)
        return Fail(7890,"map_hold_client_marker_contract_nonce_mismatch");
    if(!fixture::Arm())return Fail(fixture::context.error,fixture::context.stage);
    if(!fixture::context.armed)return true;
    if(hold_client::stop.exchange(false,std::memory_order_relaxed)||state.stopRequested){
        state.finished=true;state.actionsCompleted=false;state.phase=Phase::Complete;
        state.stage="map_hold_client_explicit_stop_no_fixture_input_held_not_observed";return true;}
    if(!ValidateOwner(world,pc,ps,pawn))return false;
    if(!state.bound||state.ownershipPassedMask!=0x1fff){
        if(state.elapsedSeconds*1000>=state.marker.waitMs)return Fail(7892,"map_hold_original_human_owner_not_observed");
        return Wait("map_hold_waiting_original_human_owner");
    }
    if(state.elapsedSeconds*1000>=state.marker.totalMs){state.finished=true;state.actionsCompleted=false;state.phase=Phase::Complete;
        state.stage="map_hold_bounded_host_physics_result_not_observed";return true;}
    const auto now=GetTickCount64();if(now-hold_client::lastPoll<100)return true;hold_client::lastPoll=now;
    fixture::Completion result{};if(!fixture::ReadCompletion(result))return Wait("map_hold_original_spawn_bound_waiting_host_physics_and_release");
    if(result.verdict==2){state.finished=true;state.actionsCompleted=false;state.phase=Phase::Complete;
        state.stage="map_hold_host_original_preflight_physics_or_release_not_observed";return true;}
    state.finished=true;state.actionsCompleted=true;state.phase=Phase::Complete;
    state.stage="map_hold_host_same_life_physics_dwell_and_original_release_receipt_observed";return true;
}
inline bool TickMapHold(void* world,void* pc,void* ps,void* pawn){
    if(!original_players::gameThread||GetCurrentThreadId()!=original_players::gameThread)return true;
    bool result=false,returned=false;__try{result=TickMapHoldBody(world,pc,ps,pawn);returned=true;}
    __finally{if(!returned){state.failed=true;state.finished=true;state.actionsCompleted=false;state.error=7895;state.stage="map_hold_client_exception_unobserved";}hold_client::Publish();}
    return result;
}
inline bool StopMapHold(){
    hold_client::stop.store(true,std::memory_order_relaxed);
    if(!original_players::gameThread||GetCurrentThreadId()!=original_players::gameThread)return true;
    if(state.finished)return true;return TickMapHold(state.world,state.pc,state.ps,state.pawn);
}
inline void WriteMapHoldState(FILE* file){const auto s=hold_client::Copy();const auto b=[](bool x){return x?"true":"false";};
    std::fprintf(file,"{\"scenario\":14,\"nonce\":%llu,\"scope\":\"original_spawn_participant_only_no_SUT_input\",\"configured\":%s,\"cli_matched\":%s,\"armed\":%s,\"bound\":%s,\"finished\":%s,\"failed\":%s,\"actions_completed\":%s,\"error\":%u,\"stage\":\"%s\",\"fixture_input_calls\":0,\"per_intent_consumption_known\":false,\"passed\":false}",static_cast<unsigned long long>(s.nonce),b(s.configured),b(s.cliMatched),b(s.armed),b(s.bound),b(s.finished),b(s.failed),b(s.actionsCompleted),s.error,s.stage);
}
