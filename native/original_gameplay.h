#pragma once
#include "original_players.h"
#include <array>

// Supply the missing standalone AUTHORITY gameplay-start timestamp. The actual
// original GameInfo OnRep, synchronized-clock conversion, gameplay-state change
// and notification are retained. In particular this never writes bGameplayActive
// (+811), input-response flags, state transitions, pawn roles or ability motion.
namespace original_gameplay {
inline constexpr size_t GameInfoOffset=0x7d8,GameInfoBytes=64;
inline constexpr size_t AuthStart=0x7f4,AuthEnd=0x7f8,LocalStart=0x7fc,LocalEnd=0x800;
struct Diagnostics {
    bool initialized=false,submitted=false,ready=false,alreadyActive=false;
    bool originalStandalone=false,originalAuthority=false,originalWorldGameState=false;
    bool localAuthorityStartSupplied=false,originalOnRepCompleted=false,originalQueryValid=false;
    bool originalActiveBefore=false,restartRequired=false;
    unsigned error=0,samples=0;
    float originalStartDefault=0,originalEndDefault=0,sourceClockTime=0;
    float beforeAuthStart=0,beforeAuthEnd=0,beforeLocalStart=0,beforeLocalEnd=0;
    float afterAuthStart=0,afterAuthEnd=0,afterLocalStart=0,afterLocalEnd=0;
    uint8_t gamePhase=255;
    uintptr_t exceptionRva=0;
    void *world=nullptr,*pawn=nullptr,*gameState=nullptr,*clock=nullptr;
    const char* stage="original_gameplay_not_initialized";
    const char* operation="none";
    const wchar_t* failedEvent=nullptr;
    uint16_t expectedEventSize=0,actualEventSize=0;
};
inline Diagnostics state;
inline const Diagnostics& State(){return state;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline bool Fail(unsigned code,const char* stage){state.error=code;state.stage=stage;return false;}
inline bool Thread(){return !state.restartRequired&&original_players::Thread();}
inline bool Event(void* object,const wchar_t* name,uint16_t size,void* parameters){
    auto fn=engine::Find(name);const auto actual=Read<uint16_t>(fn,0xbe);
    if(!object||!fn||actual!=size){state.failedEvent=name;state.expectedEventSize=size;state.actualEventSize=actual;return false;}
    auto table=Read<void**>(object,0);auto process=Read<void*>(table,0x298);
    if(!process)return false;
    reinterpret_cast<void(*)(void*,void*,void*)>(process)(object,fn,parameters);return true;
}
inline void SampleTimes(){
    state.afterAuthStart=Read<float>(state.gameState,AuthStart);state.afterAuthEnd=Read<float>(state.gameState,AuthEnd);
    state.afterLocalStart=Read<float>(state.gameState,LocalStart);state.afterLocalEnd=Read<float>(state.gameState,LocalEnd);
    state.gamePhase=Read<uint8_t>(state.gameState,0xc60);
}
inline bool PollImpl(){
    if(!state.initialized||!Thread())return false;
    if(Read<void*>(state.world,0x178)!=state.gameState||Read<void*>(state.pawn,0x488)!=original_players::players[0].state)
        return Fail(2801,"original_gameplay_original_world_or_local_player_identity_changed");
    uint8_t active=0;
    state.operation="original_gameplay_original_clock_conversion_and_query";
    // The original getter invokes original7191300. It converts the original
    // pending timestamp using the original local pawn's synchronized clock,
    // changes +811 itself, clears a consumed pending timestamp, and broadcasts
    // the actual original OnGameplayActiveChanged delegate.
    state.originalQueryValid=Event(state.gameState,L"/Script/ShooterGame.ShooterGameState.IsGameplayActive",1,&active);
    if(!state.originalQueryValid)return Fail(2802,"original_gameplay_original_active_query_signature_missing");
    state.ready=active!=0;SampleTimes();++state.samples;state.error=0;state.operation="none";
    state.stage=state.ready?"original_gameplay_original_active_observed":"original_gameplay_waiting_for_original_timestamp";
    return true;
}
inline bool StartTrainingImpl(void* world,void* pawn){
    if(!Thread()||!world||!pawn)return Fail(2803,"original_gameplay_requires_actual_owned_game_thread_context");
    if(state.initialized){
        if(state.world!=world||state.pawn!=pawn)return Fail(2801,"original_gameplay_original_context_changed");
        return PollImpl();
    }
    state.world=world;state.pawn=pawn;state.gameState=Read<void*>(world,0x178);
    state.originalWorldGameState=original_players::IsA(state.gameState,original_players::ShooterGameStateClass)
        &&state.gameState==original_players::createdGameState;
    state.originalStandalone=state.originalWorldGameState&&engine::function<int(*)(void*)>(0x3efc820)(state.gameState)==0;
    state.originalAuthority=Read<uint8_t>(state.gameState,0x224)==3&&Read<uint8_t>(pawn,0x224)==3;
    if(!state.originalWorldGameState||!state.originalStandalone||!state.originalAuthority
       ||pawn!=original_players::activePawn||!original_players::IsA(pawn,original_players::ShooterCharacterClass))
        return Fail(2804,"original_gameplay_requires_created_original_standalone_authority_graph");
    state.beforeAuthStart=Read<float>(state.gameState,AuthStart);state.beforeAuthEnd=Read<float>(state.gameState,AuthEnd);
    state.beforeLocalStart=Read<float>(state.gameState,LocalStart);state.beforeLocalEnd=Read<float>(state.gameState,LocalEnd);
    auto defaults=engine::DefaultObject(engine::Class(original_players::ShooterGameStateClass));
    if(!defaults)return Fail(2805,"original_gameplay_original_game_info_defaults_missing");
    state.originalStartDefault=Read<float>(defaults,AuthStart);state.originalEndDefault=Read<float>(defaults,AuthEnd);
    if(!std::isfinite(state.beforeAuthStart)||!std::isfinite(state.beforeAuthEnd)
       ||!std::isfinite(state.originalStartDefault)||!std::isfinite(state.originalEndDefault))
        return Fail(2806,"original_gameplay_original_timestamp_fields_nonfinite");
    state.initialized=true;
    if(!PollImpl())return false;
    state.originalActiveBefore=state.ready;
    if(state.ready){state.alreadyActive=true;state.submitted=true;return true;}
    if(state.beforeAuthStart!=state.originalStartDefault){
        // Preserve an existing original authority schedule. Subsequent native
        // clock/query polling decides when it becomes active.
        state.submitted=true;state.stage="original_gameplay_preserving_original_authority_schedule";return true;
    }
    if(state.beforeAuthEnd!=state.originalEndDefault)
        return Fail(2807,"original_gameplay_existing_authority_end_schedule_requires_existing_lifecycle");
    state.operation="original_gameplay_read_original_local_pawn_clock";
    state.clock=engine::function<void*(*)(void*)>(0x787b6c0)(pawn);
    if(!state.clock)return Fail(2808,"original_gameplay_original_local_pawn_clock_missing");
    state.sourceClockTime=engine::function<float(*)(void*)>(0x789df70)(state.clock);
    if(!std::isfinite(state.sourceClockTime)||state.sourceClockTime<0)
        return Fail(2809,"original_gameplay_original_local_pawn_clock_invalid");
    alignas(8) std::array<uint8_t,GameInfoBytes> previous{};
    std::memcpy(previous.data(),static_cast<uint8_t*>(state.gameState)+GameInfoOffset,GameInfoBytes);
    state.operation="original_gameplay_supply_missing_local_authority_start_timestamp";
    if(!original_players::Write(state.gameState,AuthStart,state.sourceClockTime))
        return Fail(2810,"original_gameplay_local_authority_start_timestamp_publish_failed");
    state.localAuthorityStartSupplied=true;
    // Exact original RepNotify consumes the old FAresGameInfo value64B. It
    // copies AuthStart/AuthEnd to LocalStart/LocalEnd and performs its original
    // per-field HUD/time/store notifications. Only AuthStart above is supplied.
    if(!Event(state.gameState,L"/Script/ShooterGame.ShooterGameState.OnRep_GameInfo",64,previous.data()))
        return Fail(2811,"original_gameplay_original_game_info_rep_notify_missing");
    state.originalOnRepCompleted=true;state.submitted=true;
    return PollImpl();
}
inline LONG Capture(EXCEPTION_POINTERS* exception){
    state.error=exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:0xe0002800U;
    const auto pc=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=pc>=engine::image?pc-engine::image:pc;state.restartRequired=true;
    state.stage="original_gameplay_exception_requires_new_session";return EXCEPTION_EXECUTE_HANDLER;
}
// A true return means the original request/query completed; State().ready is
// the separate actual gameplay gate. Poll can wait for the next native clock
// step without resubmitting the authority record or changing input responses.
inline bool StartTraining(void* world,void* pawn){
    __try{return StartTrainingImpl(world,pawn);}__except(Capture(GetExceptionInformation())){return false;}
}
inline bool Poll(){__try{return PollImpl();}__except(Capture(GetExceptionInformation())){return false;}}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;std::wstring temporary(path);temporary+=L".tmp";
    FILE* f=nullptr;if(_wfopen_s(&f,temporary.c_str(),L"wb")||!f)return false;
    const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(f,"{\"schema_version\":1,\"pid\":%lu,\"stage\":",GetCurrentProcessId());original_players::Text(f,state.stage);
    std::fprintf(f,",\"operation\":");original_players::Text(f,state.operation);
    std::fprintf(f,",\"error\":%u,\"restart_required\":%s,\"exception_rva\":%llu,\"initialized\":%s,\"submitted\":%s,\"ready\":%s,\"already_active\":%s,\"original_standalone\":%s,\"original_authority\":%s,\"original_world_game_state\":%s,\"local_authority_start_supplied\":%s,\"original_on_rep_completed\":%s,\"original_query_valid\":%s,\"original_active_before\":%s,\"samples\":%u,\"game_phase\":%u,\"original_start_default\":%.9g,\"original_end_default\":%.9g,\"source_clock_time\":%.9g,\"before_auth_start\":%.9g,\"before_auth_end\":%.9g,\"before_local_start\":%.9g,\"before_local_end\":%.9g,\"after_auth_start\":%.9g,\"after_auth_end\":%.9g,\"after_local_start\":%.9g,\"after_local_end\":%.9g,\"world\":\"%p\",\"pawn\":\"%p\",\"game_state\":\"%p\",\"clock\":\"%p\",\"failed_event\":",state.error,b(state.restartRequired),static_cast<unsigned long long>(state.exceptionRva),b(state.initialized),b(state.submitted),b(state.ready),b(state.alreadyActive),b(state.originalStandalone),b(state.originalAuthority),b(state.originalWorldGameState),b(state.localAuthorityStartSupplied),b(state.originalOnRepCompleted),b(state.originalQueryValid),b(state.originalActiveBefore),state.samples,static_cast<unsigned>(state.gamePhase),state.originalStartDefault,state.originalEndDefault,state.sourceClockTime,state.beforeAuthStart,state.beforeAuthEnd,state.beforeLocalStart,state.beforeLocalEnd,state.afterAuthStart,state.afterAuthEnd,state.afterLocalStart,state.afterLocalEnd,state.world,state.pawn,state.gameState,state.clock);
    const auto event=original_players::Utf8(state.failedEvent);original_players::Text(f,event.c_str());
    std::fprintf(f,",\"expected_event_size\":%u,\"actual_event_size\":%u,\"writes_gameplay_active_flag\":false,\"changes_input_response_flags\":false,\"runtime_skill_execution_verified\":false}\n",state.expectedEventSize,state.actualEventSize);
    const bool ok=std::ferror(f)==0,closed=std::fclose(f)==0;
    return ok&&closed&&MoveFileExW(temporary.c_str(),path,MOVEFILE_REPLACE_EXISTING);
}
}
