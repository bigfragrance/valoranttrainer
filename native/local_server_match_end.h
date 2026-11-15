#pragma once
#include "local_server_match.h"
#include "local_server_game_start.h"
#include "local_server_mode_startup.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdio>

// Host-only missing match-completion rule for the real GameModeBase session.
// Canonical GS fields plus full original notifications only. Client result UI
// remains ordinary native replication/OnMatchEnd. No mode-layout casts, score
// replacement, widget writes, account identity or direct active/phase writes.
namespace local_server_match_end {
using EndReason=deathmatch_rules::EndReason;
using Weak=original_inventory_slots::WeakIdentity;
using GameInfo=local_server_game_start::GameInfo;
inline constexpr size_t MaxPlayers=local_server_match::MaxPlayers;
struct Name {uint32_t words[3]{};};
struct MatchResult {void* winningTeam=nullptr;int32_t winningPoints=0;uint8_t completion=0,padding[3]{};};
static_assert(sizeof(Name)==12&&sizeof(MatchResult)==16&&offsetof(MatchResult,winningPoints)==8&&offsetof(MatchResult,completion)==12);
struct PlayerTotal {
    void *playerState=nullptr,*pawn=nullptr;uint32_t epoch=0,kills=0,deaths=0;
};
struct ActualTotals {
    const PlayerTotal* players=nullptr;size_t count=0;
    double elapsedSeconds=0;uint64_t sessionEpoch=1;
};
struct PlayerRecord {
    bool used=false,currentLifeDead=false;size_t index=0;PlayerTotal total{};
    void *team=nullptr,*teamActor=nullptr;Weak psIdentity{},pawnIdentity{},teamIdentity{},teamActorIdentity{};
    int32_t nativeKills=0,nativeDeaths=0,shortPointsBefore=0,shortPointsAfter=0;
};
struct State {
    bool initialized=false,profilesVerified=false,metadataVerified=false,totalsVerified=false;
    bool publicationAttempted=false,timelinePublished=false,timelineWritten=false,nativeActive=false;
    bool resultStored=false,matchStateStored=false,matchNotifyReturned=false,resultNotifyReturned=false;
    bool complete=false,restartRequired=false,clientResultUIVerified=false;
    bool originalMode=false,originalModeEndAttempted=false,originalModeEndReturned=false;
    uint32_t originalModeEndCalls=0;void *originalEndFunction=nullptr,*gameSession=nullptr;
    Weak gameSessionIdentity{};Name beforeModeMatch{},afterModeMatch{};
    unsigned error=0;uint32_t gameThread=0,profileFailureRva=0,clockIndex=0,clockStep=0,clockBaseOffset=0;
    uint64_t sessionEpoch=0,digest=0,idempotentCalls=0,polls=0,activeQueries=0;
    uint32_t gameInfoNotifications=0,matchStateNotifications=0,resultNotifications=0;
    size_t participants=0,clockPlayerIndex=MaxPlayers;int32_t winner=-1;
    uint32_t leadingKills=0,leadingTies=0;
    double elapsedSeconds=0;float nativeEndTime=-1,nativeClockTime=-1;
    EndReason reason=EndReason::None;uintptr_t exceptionRva=0;
    void *world=nullptr,*gameState=nullptr,*mode=nullptr,*winnerPS=nullptr,*winnerTeam=nullptr;
    void *clockPawn=nullptr,*movement=nullptr,*clock=nullptr,*fixedStep=nullptr;
    Weak worldIdentity{},gameStateIdentity{},modeIdentity{};
    Name beforeMatch{},beforePrevious{},postMatch{},afterMatch{},afterPrevious{};
    GameInfo beforeInfo{},afterInfo{};MatchResult beforeResult{},result{};
    void *onRepMatch=nullptr,*onRepResult=nullptr,*onRepInfo=nullptr,*activeGetter=nullptr,*winningGetter=nullptr;
    const char* stage="authority_match_end_not_requested";
    const char* operation="none";
};
inline State state;
inline std::array<PlayerRecord,MaxPlayers> records{};
inline uint64_t nextPoll=0;
inline const State& GetState(){return state;}
inline const std::array<PlayerRecord,MaxPlayers>& Players(){return records;}
template<class T> inline T Read(const void* p,size_t offset=0){return original_players::Value<T>(p,offset);}
// Actual ShooterMode.GetGameSessionClass(v7D8) returns ShooterGameSession,
// independently of the inherited GameSessionClass field478. Exact original
// native class/table/source; no arbitrary subclass or private table is admitted.
inline constexpr uintptr_t OriginalShooterSessionSlots[278]{
    0x6c9cf40,0x18a3a00,0x1cfa620,0x1af09d0,0x18a76a0,0x3ed82e0,0x1d06c40,0x18a3a00,
    0x2a652b0,0x2a652d0,0x3ed7050,0x1af0aa0,0x1af0a90,0x1af0ab0,0x3ed8280,0x18a3a00,
    0x18a3a00,0x3ed83a0,0x18a76a0,0x3ed83a0,0x18a3a00,0x36afd10,0x18a3a00,0x18a76a0,
    0x18a3ec0,0x3ed9b40,0x3ed9c00,0x3ed9030,0x36c7460,0x1c53840,0x3ef6ca0,0x3ed9770,
    0x3ed98d0,0x18a3a00,0x18a3a00,0x3f00330,0x18a3a00,0x1af0b30,0x18a3a00,0x1c53720,
    0x1c53700,0x18a3ec0,0x18a76a0,0x3ed82f0,0x18a76a0,0x18a76a0,0x18a76a0,0x1c53be0,
    0x18a3a00,0x18a3a00,0x18a3a00,0x18a3a00,0x18a3a00,0x3eda8f0,0x1af0b50,0x3ed83b0,
    0x18a76a0,0x1c562c0,0x1af0b70,0x18a3ed0,0x18a3ec0,0x1c554e0,0x18a3a00,0x3ed8300,
    0x1c56490,0x1c56550,0x1c56640,0x1c56670,0x3f06430,0x3f06af0,0x3f07590,0x3f07520,
    0x1c5dc90,0x18a3ec0,0x3f07140,0x3f050b0,0x3f050f0,0x18a3a00,0x18a3a00,0x1c54130,
    0x18a3ed0,0x18a3a00,0x18a3a00,0x3eda0d0,0x3efc9b0,0x3efd360,0x18a76a0,0x18a3ed0,
    0x18a3a00,0x3ed7960,0x3ef58f0,0x18a3a00,0x18dd100,0x18a3a00,0x18a3a00,0x3f01d20,
    0x3ee1720,0x3ede520,0x3ede430,0x18a3a00,0x3ef3450,0x3efa480,0x3efa7a0,0x3ede560,
    0x18a76a0,0x18a3a00,0x3f06b50,0x3f073b0,0x3f07480,0x3edac90,0x3edb100,0x18a3a00,
    0x3efabc0,0x3efacb0,0x3efadc0,0x3ec5810,0x3ec57c0,0x3efbce0,0x3ee3d10,0x3ed8fb0,
    0x3ed8fd0,0x3ed8ff0,0x3ed9010,0x3efa970,0x3ee1450,0x3edd790,0x3edd7f0,0x3edd850,
    0x3edd8b0,0x3edd910,0x3edda40,0x3eddb70,0x3eddbd0,0x3eddc30,0x3eddc90,0x3eddcf0,
    0x3effdb0,0x3f00160,0x18a76a0,0x3f05e10,0x2a652f0,0x3eda180,0x18a76a0,0x3f00a50,
    0x3f00b20,0x3f00d30,0x3f04d90,0x3f04f60,0x18a76a0,0x18a3a00,0x18a76a0,0x18a3a00,
    0x18a3a00,0x3eda8c0,0x3edab20,0x2a65300,0x18a3a00,0x3ee3d10,0x18a3a00,0x3efa6a0,
    0x3f051e0,0x3f052e0,0x18a3a00,0x3f056f0,0x3f05750,0x3ede1d0,0x3edc1a0,0x3edee80,
    0x3edab80,0x18a76a0,0x3f05b40,0x3f05df0,0x3edef40,0x3f00260,0x3f00240,0x3efdad0,
    0x2a65390,0x3edaaf0,0x3edaad0,0x18a76a0,0x18a76a0,0x3efde00,0x3efde70,0x3efdf10,
    0x3efdfd0,0x3efe5c0,0x18a3a00,0x3ee6350,0x3efe400,0x3efe590,0x3eff7d0,0x2a65370,
    0x3efea40,0x18a3a00,0x3ec57b0,0x18a3a00,0x3ed8aa0,0x3edb330,0x3edb2e0,0x18a3a00,
    0x3f07830,0x3f07860,0x3eda4e0,0x18a3ec0,0x3ee1640,0x3ee1eb0,0x3ee3d10,0x3edb350,
    0x3edb420,0x3edb120,0x3edb210,0x3edb2a0,0x18a3a00,0x3ee4650,0x18a3ec0,0x3ee20d0,
    0x3ee2670,0x2781760,0x3ee3d20,0x3ee3d80,0x3ee3de0,0x3ee3f50,0x3ee4190,0x1b97710,
    0x3ee1e50,0x3ede1c0,0x3ee43e0,0x3edef50,0x3ee1030,0x3ee4500,0x3edeed0,0x18a3a00,
    0x3ee57b0,0x3ee5d60,0x3f06d30,0x18a3ec0,0x3ee2af0,0x4439df0,0x443a1e0,0x443a2a0,
    0x443a340,0x443a830,0x443a720,0x18a3a00,0x443ad20,0x443ae00,0x443ae30,0x443ac90,
    0x443a8f0,0x443ab40,0x443a9f0,0x18a3a00,0x18a3a00,0x443ae50,0x443af80,0x443afa0,
    0x18a3a00,0x18a3a00,0x18a3a00,0x443a320,0x443b140,0x443b1d0,0x4439710,0x41119a0,
    0x18a76a0,0x18a3a00,0x4439af0,0x4439c70,0x18a3ec0,0x18a3a00,0x18a3a00,0x7185610,
    0x7185720,0x7185050,0x7185400,0x7185160,0x7185420,0x18a3a00,
};
struct ShooterSessionProfile {uint32_t rva,bytes;uint64_t fnv;};
inline constexpr ShooterSessionProfile OriginalShooterSessionProfiles[]{
    {0x43cf880,699,0x88fbbb4a905ea0a6ULL},
    {0x71836f0,26,0x2d355c45602d9228ULL},
    {0x6c9ce60,174,0x199065778706f62fULL},
    {0x6cac8f0,13,0xff50deb828896dceULL},
    {0x7184a30,1560,0x2d8cc4fcd74552ffULL},
};
inline bool OriginalShooterSessionTable(void* table){
    if(table!=reinterpret_cast<void*>(engine::image+0xb5023b0)
       ||!original_players::Readable(table,sizeof(OriginalShooterSessionSlots)))return false;
    for(size_t n=0;n<std::size(OriginalShooterSessionSlots);++n)
        if(Read<uintptr_t>(table,n*8)!=engine::image+OriginalShooterSessionSlots[n])return false;
    return true;
}
inline bool OriginalShooterSessionProfilesMatch(uint32_t& failure){
    failure=0;
    for(const auto& p:OriginalShooterSessionProfiles){
        auto bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes)){failure=p.rva;return false;}
        uint64_t hash=14695981039346656037ULL;
        for(uint32_t n=0;n<p.bytes;++n)hash=(hash^bytes[n])*1099511628211ULL;
        if(hash!=p.fnv){failure=p.rva;return false;}
    }return true;
}
inline bool Same(const Name& a,const Name& b){return std::memcmp(&a,&b,8)==0;}
inline bool Exact(const Name& a,const Name& b){return std::memcmp(&a,&b,sizeof(Name))==0;}
inline bool Live(void* p,const Weak& identity){return p&&original_inventory_slots::ResolveIdentity(identity)==p;}
inline bool Fail(unsigned error,const char* stage){state.error=error;state.stage=stage;state.complete=false;
    if(state.publicationAttempted)state.restartRequired=true;return false;}
inline bool Thread(){return original_players::Thread()&&(!state.initialized||state.gameThread==GetCurrentThreadId());}
inline bool Dispatch(void* object,void* function,void* params){
    if(!object||!function)return false;auto entry=Read<void*>(Read<void*>(object),0x298);
    if(!entry)return false;reinterpret_cast<void(*)(void*,void*,void*)>(entry)(object,function,params);return true;
}
inline bool Function(void*& result,const wchar_t* path,uint16_t bytes,uintptr_t exec){
    result=engine::Find(path);return result&&Read<uint16_t>(result,0xbe)==bytes
        &&Read<void*>(result,0xe0)==reinterpret_cast<void*>(engine::image+exec)
        &&(Read<uint32_t>(result,0xb8)&0x440)==0x400;
}
inline bool Property(void* type,const wchar_t* name,int32_t bytes,int32_t offset,uint64_t required=0){
    Name key{};engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&key,name,1);
    auto property=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,&key,3);
    return property&&(Read<uint64_t>(Read<void*>(property,8),0x18)&0x8001)==0x8001
        &&Read<int32_t>(property,0x30)==1&&Read<int32_t>(property,0x34)==bytes&&Read<int32_t>(property,0x44)==offset
        &&(Read<uint64_t>(property,0x38)&required)==required;
}
// One read-only evaluator is used by BOTH early preflight and real end setup.
// Finding a FName uses existing-name mode0; no missing property is synthesized.
struct EndMetadataField {
    const char* label="";void *type=nullptr,*field=nullptr;
    int32_t dimension=0,bytes=0,offset=0,expectedBytes=0,expectedOffset=0;
    uint64_t castFlags=0,flags=0,required=0;bool okay=false;
};
struct EndMetadataReadback {
    bool checked=false,originalMode=false,copyProfile=false;
    unsigned error=0;uint64_t sampledAt=0,passed=0,failed=0;
    void *gameState=nullptr,*resultType=nullptr,*infoType=nullptr;
    void *onRepMatch=nullptr,*onRepResult=nullptr,*onRepInfo=nullptr,*activeGetter=nullptr,*winningGetter=nullptr;
    void *endFunction=nullptr,*endOwner=nullptr,*expectedEndOwner=nullptr,*endExec=nullptr;
    uint16_t endBytes=0;uint32_t endFlags=0;
    void *sessionClass=nullptr,*stateClass=nullptr;
    int32_t sessionClassSize=0,stateClassSize=0;Name postMatch{},expectedPostMatch{};
    std::array<EndMetadataField,10> fields{};
};
inline constexpr const char* EndMetadataGateNames[]{
    "gs_match_state","gs_previous_state","gs_match_result","gs_game_info","gs_pending_winner",
    "result_type","result_size","result_winning_team","result_winning_points","result_completion",
    "onrep_match_function","onrep_result_function","onrep_info_function","active_getter","winning_getter",
    "game_info_type","game_info_size","game_info_auth_end","gs_postmatch_virtual","gs_ended_virtual",
    "postmatch_global_name","postmatch_existing_name","end_function_abi","end_function_owner",
    "session_reflected_name","session_native_class_size","state_native_class_size","native_name_size","native_name_copy_profile"
};
inline SRWLOCK actualEndMetadataLock=SRWLOCK_INIT;
inline EndMetadataReadback actualEndMetadata{};
inline void CacheActualEndMetadata(const EndMetadataReadback& value){
    AcquireSRWLockExclusive(&actualEndMetadataLock);actualEndMetadata=value;ReleaseSRWLockExclusive(&actualEndMetadataLock);
}
inline EndMetadataReadback CopyActualEndMetadata(){
    AcquireSRWLockShared(&actualEndMetadataLock);const auto value=actualEndMetadata;ReleaseSRWLockShared(&actualEndMetadataLock);return value;
}
inline bool ReadEndMetadataField(EndMetadataField& out,const char* label,void* type,const wchar_t* text,
                                int32_t bytes,int32_t offset,uint64_t required=0){
    out.label=label;out.type=type;out.expectedBytes=bytes;out.expectedOffset=offset;out.required=required;
    Name key{};engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&key,text,0);
    if(!key.words[0])return false;
    out.field=engine::function<void*(*)(void*,const void*,uint8_t)>(0x1c63870)(type,&key,3);
    out.castFlags=Read<uint64_t>(Read<void*>(out.field,8),0x18);out.dimension=Read<int32_t>(out.field,0x30);
    out.bytes=Read<int32_t>(out.field,0x34);out.flags=Read<uint64_t>(out.field,0x38);out.offset=Read<int32_t>(out.field,0x44);
    return out.okay=out.field&&(out.castFlags&0x8001)==0x8001&&out.dimension==1
        &&out.bytes==bytes&&out.offset==offset&&(out.flags&required)==required;
}
inline bool NativeSessionNameCopyProfile(){
    // Original GameSession.InitOptions: actual Mode's selected PS CDO+4C8/4D0
    // is copied as12 bytes to Session+478/480. PlayerState member is NOT a
    // reflected UPROPERTY in this image; class size and this exact native use
    // are its layout contract, rather than a fabricated reflection lookup.
    const auto bytes=reinterpret_cast<const uint8_t*>(engine::image+0x4439df0);
    if(!original_players::Readable(bytes,998))return false;
    uint64_t hash=14695981039346656037ULL;
    for(uint32_t n=0;n<998;++n)hash=(hash^bytes[n])*1099511628211ULL;
    return hash==0x3e09db47214f19a7ULL;
}
inline EndMetadataReadback ReadEndMetadata(void* gs,bool originalMode){
    EndMetadataReadback p{};p.checked=true;p.originalMode=originalMode;p.gameState=gs;p.sampledAt=GetTickCount64();
    const auto mark=[&](size_t n,bool okay,unsigned error){const auto mask=uint64_t(1)<<n;
        if(okay)p.passed|=mask;else{p.failed|=mask;if(!p.error)p.error=error;}};
    auto type=Read<void*>(gs,0x10);constexpr uint64_t netNotify=0x100000020ULL;
    mark(0,ReadEndMetadataField(p.fields[0],"gs_match_state",type,L"MatchState",12,0x4c0,netNotify),5103);
    mark(1,ReadEndMetadataField(p.fields[1],"gs_previous_state",type,L"PreviousMatchState",12,0x4cc),5103);
    mark(2,ReadEndMetadataField(p.fields[2],"gs_match_result",type,L"MatchResult",16,0x710,netNotify),5103);
    mark(3,ReadEndMetadataField(p.fields[3],"gs_game_info",type,L"GameInfo",64,0x7d8,netNotify),5103);
    mark(4,ReadEndMetadataField(p.fields[4],"gs_pending_winner",type,L"PendingWinningTeam",8,0x738),5103);
    p.resultType=engine::Find(L"/Script/ShooterGame.AresMatchResult");
    mark(5,p.resultType!=nullptr,5103);mark(6,Read<int32_t>(p.resultType,0x60)==16,5103);
    mark(7,ReadEndMetadataField(p.fields[5],"result_winning_team",p.resultType,L"WinningTeam",8,0),5103);
    mark(8,ReadEndMetadataField(p.fields[6],"result_winning_points",p.resultType,L"WinningTeamPoints",4,8),5103);
    mark(9,ReadEndMetadataField(p.fields[7],"result_completion",p.resultType,L"CompletionState",1,12),5103);
    mark(10,Function(p.onRepMatch,L"/Script/Engine.GameState.OnRep_MatchState",0,0x43ce630),5103);
    mark(11,Function(p.onRepResult,L"/Script/ShooterGame.ShooterGameState.OnRep_MatchResults",0,0x6ca6490),5103);
    mark(12,Function(p.onRepInfo,L"/Script/ShooterGame.ShooterGameState.OnRep_GameInfo",64,0x6ca43d0),5103);
    mark(13,Function(p.activeGetter,L"/Script/ShooterGame.ShooterGameState.IsGameplayActive",1,0x6ca0980),5103);
    mark(14,Function(p.winningGetter,L"/Script/ShooterGame.ShooterGameState.GetWinningTeam",8,0x6ca6c40),5103);
    p.infoType=engine::Find(L"/Script/ShooterGame.AresGameInfo");mark(15,p.infoType!=nullptr,5104);
    mark(16,Read<int32_t>(p.infoType,0x60)==64,5104);
    mark(17,ReadEndMetadataField(p.fields[8],"game_info_auth_end",p.infoType,L"AuthGameplayEndTimestamp",4,0x20),5104);
    auto table=Read<void*>(gs);mark(18,Read<void*>(table,0x830)==reinterpret_cast<void*>(engine::image+0x443c750),5105);
    mark(19,Read<void*>(table,0x820)==reinterpret_cast<void*>(engine::image+0x71897e0),5105);
    p.postMatch=Read<Name>(reinterpret_cast<void*>(engine::image),0xd7e6f38);mark(20,p.postMatch.words[0]!=0,5106);
    engine::function<void*(*)(void*,const wchar_t*,int)>(0x1a7b720)(&p.expectedPostMatch,L"WaitingPostMatch",0);
    mark(21,Exact(p.postMatch,p.expectedPostMatch),5106);
    if(originalMode){
        const bool abi=Function(p.endFunction,L"/Script/Engine.GameMode.EndMatch",0,0x43c9810);
        p.endOwner=Read<void*>(p.endFunction,0x28);p.expectedEndOwner=engine::Class(0x43c93e0);
        p.endBytes=Read<uint16_t>(p.endFunction,0xbe);p.endFlags=Read<uint32_t>(p.endFunction,0xb8);p.endExec=Read<void*>(p.endFunction,0xe0);
        mark(22,abi,5133);mark(23,p.endOwner==p.expectedEndOwner,5133);
        p.sessionClass=engine::Class(0x43c3760);p.stateClass=engine::Class(0x42aae70);
        mark(24,ReadEndMetadataField(p.fields[9],"session_reflected_name",p.sessionClass,L"SessionName",12,0x478),5133);
        p.sessionClassSize=Read<int32_t>(p.sessionClass,0x60);p.stateClassSize=Read<int32_t>(p.stateClass,0x60);
        mark(25,p.sessionClassSize==0x488,5133);mark(26,p.stateClassSize==0x528,5133);
        static_assert(sizeof(Name)==12);mark(27,sizeof(Name)==12,5133);
        p.copyProfile=NativeSessionNameCopyProfile();mark(28,p.copyProfile,5133);
    }else for(size_t n=22;n<std::size(EndMetadataGateNames);++n)mark(n,true,5133);
    return p;
}
inline std::string EndMetadataJson(const EndMetadataReadback& p){
    local_server_match::Json out;const auto b=[](bool value){return value?"true":"false";};
    const auto u=[](uint64_t value){return static_cast<unsigned long long>(value);};
    const auto rva=[](void* value){const auto at=reinterpret_cast<uintptr_t>(value);
        return static_cast<unsigned long long>(at>=engine::image&&at-engine::image<0xe000000?at-engine::image:0);};
    out.Add("{\"checked\":%s,\"original_mode\":%s,\"sampled_at_ms\":%llu,\"error\":%u,\"passed_mask\":%llu,\"failed_mask\":%llu,\"failed_gates\":[",
        b(p.checked),b(p.originalMode),u(p.sampledAt),p.error,u(p.passed),u(p.failed));
    bool comma=false;for(size_t n=0;n<std::size(EndMetadataGateNames);++n)if(p.failed&(uint64_t(1)<<n)){
        out.Add("%s",comma?",":"");out.String(EndMetadataGateNames[n]);comma=true;}
    out.Add("],\"session_class_size\":%d,\"state_class_size\":%d,\"native_name_bytes\":12,\"native_copy_profile\":%s,\"end_function\":\"%p\",\"end_owner28\":\"%p\",\"expected_end_owner\":\"%p\",\"end_parms_size\":%u,\"end_flags\":%u,\"end_exec_rva\":%llu,\"fields\":[",
        p.sessionClassSize,p.stateClassSize,b(p.copyProfile),p.endFunction,p.endOwner,p.expectedEndOwner,static_cast<unsigned>(p.endBytes),p.endFlags,rva(p.endExec));
    comma=false;for(const auto& field:p.fields){if(!field.label[0])continue;
        out.Add("%s{\"name\":",comma?",":"");out.String(field.label);comma=true;
        out.Add(",\"field\":\"%p\",\"type\":\"%p\",\"dimension\":%d,\"bytes\":%d,\"offset\":%d,\"expected_bytes\":%d,\"expected_offset\":%d,\"cast_flags\":%llu,\"flags\":%llu,\"required_flags\":%llu,\"okay\":%s}",
            field.field,field.type,field.dimension,field.bytes,field.offset,field.expectedBytes,field.expectedOffset,u(field.castFlags),u(field.flags),u(field.required),b(field.okay));
    }
    out.Add("],\"same_evaluator_as_real_initialization\":true,\"native_property_fabricated\":false,\"gameplay_writes\":0,\"end_calls\":0}");return out.value;
}

inline bool VerifyProfiles(){
    struct Profile {uint32_t rva,bytes;uint64_t fnv;};
    static constexpr Profile required[]{
        {0x443c750,177,0x6b082dee551913dcULL},{0x71897e0,367,0x259ac3db6aa440eeULL},
        {0x43ce630,38,0x1b618e86b04e92e5ULL},{0x6ca6490,381,0x0863b41c4c600586ULL},
        {0x6ca6c40,44,0x0de5eb847b9a40baULL},{0x6ca43d0,522,0x4b7c0fd5f0277fe9ULL},
        {0x6ca0980,62,0xdc7acb690b95ab97ULL},{0x7191300,205,0x5545ff97a8f003c9ULL},
        {0x71911e0,282,0xfd9afc64a82e265bULL},{0x787b6c0,99,0x2fa4f38ac8e06b6aULL},
        {0x7892e10,142,0x773b71bd06054ce5ULL},{0x789df70,144,0x59af83ea9f307d53ULL},
        {0x43c9810,38,0xbfaf3caf25c1b457ULL},
        {0x43cb5c0,83,0x594e890b30ab9b42ULL},
        {0x43cb620,138,0xa302ea6f7371c0d9ULL},
        {0x43cb7e0,282,0xcfdcb0de0ec90a9bULL},
        {0x43cb900,233,0x90ed3a15badc9d17ULL},
        {0x43cb770,36,0xbbeeaeefb6f56155ULL},
        {0x43c3760,174,0x88a0656141dfdb39ULL},
        {0x4439c70,382,0xb73813b0dca5c7d4ULL},
        {0x4439df0,998,0x3e09db47214f19a7ULL},
        {0x42aae70,174,0x307bd36745be5788ULL},
        {0x3ed83b0,1609,0x15d2d4ecc2f49cf7ULL},
    };
    for(const auto& p:required){auto bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes)){state.profileFailureRva=p.rva;return Fail(5101,"authority_match_end_original_profile_unreadable");}
        uint64_t hash=14695981039346656037ULL;for(uint32_t n=0;n<p.bytes;++n)hash=(hash^bytes[n])*1099511628211ULL;
        if(hash!=p.fnv){state.profileFailureRva=p.rva;return Fail(5102,"authority_match_end_original_profile_mismatch");}}
    if(!OriginalShooterSessionProfilesMatch(state.profileFailureRva))
        return Fail(5102,"authority_match_end_original_shooter_session_source_profile_mismatch");
    state.profilesVerified=true;return true;
}
inline bool InitializeMetadata(){
    const auto observed=ReadEndMetadata(state.gameState,state.originalMode);CacheActualEndMetadata(observed);
    if(observed.failed){
        const char* message=observed.error==5104?"authority_match_end_original_authority_end_time_layout_mismatch":
            observed.error==5105?"authority_match_end_original_post_match_virtuals_mismatch":
            observed.error==5106?"authority_match_end_original_waiting_post_match_name_unavailable":
            observed.error==5133?"authority_match_end_original_mode_end_metadata_or_native_layout_mismatch":
            "authority_match_end_original_field_or_event_abi_mismatch";
        return Fail(observed.error?observed.error:5103,message);
    }
    state.onRepMatch=observed.onRepMatch;state.onRepResult=observed.onRepResult;state.onRepInfo=observed.onRepInfo;
    state.activeGetter=observed.activeGetter;state.winningGetter=observed.winningGetter;
    state.originalEndFunction=observed.endFunction;state.postMatch=observed.postMatch;
    state.metadataVerified=true;return true;
}
// Diagnostic-only preflight. No admission, weak-serial allocation, lifecycle,
// state/score/timestamp write, or EndMatch call occurs in this observer.
// Only original read-only context/class/world queries and cached POD follow.
struct ModeContextPreflight {
    uint64_t sampledAt=0,samples=0,passed=0,failed=0;
    bool sampled=false,readException=false,worldQueryKnown=false,readyQuery=false;
    void *world=nullptr,*mode=nullptr,*gs=nullptr,*session=nullptr,*sessionClass=nullptr,*sessionTable=nullptr;
    void *sessionEnd=nullptr,*sessionWorldEntry=nullptr,*sessionWorld=nullptr,*sessionOuter=nullptr;
    void *expectedSessionClass=nullptr,*modeSessionClass=nullptr,*modeCdoSessionClass=nullptr;
    void *stateClass=nullptr,*stateDefault=nullptr,*currentStateClass=nullptr,*currentStateDefault=nullptr;
    void *modeCdoStateClass=nullptr,*modeCdoStateDefault=nullptr,*bootstrapState=nullptr;
    uint8_t sessionRole=255;uint32_t stateDefaultFlags=0,sourceProfileFailure=0;
    void *sessionFactory=nullptr,*endFunction=nullptr,*endOwner=nullptr,*endOwner20=nullptr,*expectedEndOwner=nullptr;
    uint16_t endBytes=0;uint32_t endFlags=0;void* endExec=nullptr;
    EndMetadataReadback endMetadata{};
    Name sessionObjectName{},sessionClassName{},sessionName{},engineDefaultName{},currentDefaultName{},modeDefaultName{},bootstrapName{};
};
inline constexpr const char* ModeContextGateNames[]{
    "startup_configured","startup_world","startup_mode","startup_gs","manual_factory_absent",
    "actual_shooter_mode","ready_for_adoption","owned_mode_table","session_present",
    "session_exact_original_shooter_class","session_complete_original_shooter_table","session_original_end_virtual",
    "session_authority_role","session_original_world_getter","session_world",
    "engine_state_cdo_present","engine_state_cdo_class","engine_state_cdo_flag","session_name_matches_engine_cdo",
    "previous_session_identity","original_shooter_session_factory","end_function_metadata","session_source_profiles","all_end_metadata"
};
inline ModeContextPreflight modeContextPreflight{};
inline SRWLOCK modeContextPreflightLock=SRWLOCK_INIT;
inline uint64_t nextModeContextPreflight=0;
inline void CacheModeContextPreflight(const ModeContextPreflight& value){
    AcquireSRWLockExclusive(&modeContextPreflightLock);modeContextPreflight=value;ReleaseSRWLockExclusive(&modeContextPreflightLock);
}
inline ModeContextPreflight CopyModeContextPreflight(){
    AcquireSRWLockShared(&modeContextPreflightLock);const auto value=modeContextPreflight;ReleaseSRWLockShared(&modeContextPreflightLock);return value;
}
inline void PollOriginalModePreflightImpl(void* world){
    const auto& startup=local_server_mode_startup::GetState();
    if(!original_players::Thread()||!startup.configured||startup.world!=world||!startup.ready||startup.failed)return;
    const auto now=GetTickCount64();if(now<nextModeContextPreflight)return;nextModeContextPreflight=now+1000;
    ModeContextPreflight p{};p.sampled=true;p.sampledAt=now;p.samples=CopyModeContextPreflight().samples+1;
    p.world=world;p.mode=Read<void*>(world,0x170);p.gs=Read<void*>(world,0x178);
    const auto mark=[&](size_t bit,bool okay){const auto mask=uint64_t(1)<<bit;if(okay)p.passed|=mask;else p.failed|=mask;};
    mark(0,startup.configured);mark(1,startup.world==p.world);mark(2,startup.mode==p.mode);mark(3,startup.gs==p.gs);
    mark(4,!original_players::createdGameState);mark(5,original_players::IsA(p.mode,0x6c8ec60));
    p.readyQuery=local_server_mode_startup::ReadyForAdoption(p.world,startup.bootstrapPC);mark(6,p.readyQuery);
    mark(7,local_server_mode_startup::OwnsModeTable(p.world,p.mode,Read<void**>(p.mode)));
    p.session=Read<void*>(p.mode,0x4d0);p.sessionClass=Read<void*>(p.session,0x10);p.sessionTable=Read<void*>(p.session);
    p.expectedSessionClass=engine::Class(0x6c9ce60);p.modeSessionClass=Read<void*>(p.mode,0x478);
    p.modeCdoSessionClass=Read<void*>(startup.modeCDO,0x478);
    p.sessionEnd=Read<void*>(p.sessionTable,0x858);p.sessionWorldEntry=Read<void*>(p.sessionTable,0x1b8);
    p.sessionOuter=Read<void*>(p.session,0x28);p.sessionRole=Read<uint8_t>(p.session,0x224);
    p.sessionObjectName=Read<Name>(p.session,0x18);p.sessionClassName=Read<Name>(p.sessionClass,0x18);
    p.sessionName=Read<Name>(p.session,0x478);
    mark(8,p.session!=nullptr);mark(9,p.sessionClass==p.expectedSessionClass);
    mark(10,OriginalShooterSessionTable(p.sessionTable));
    mark(11,p.sessionEnd==reinterpret_cast<void*>(engine::image+0x4439c70));mark(12,p.sessionRole==3);
    mark(13,p.sessionWorldEntry==reinterpret_cast<void*>(engine::image+0x3ed83b0));
    // Only the exact retained native object/table can reach this original getter.
    if(p.session&&p.sessionClass==p.expectedSessionClass&&(p.passed&(uint64_t(1)<<10))&&(p.passed&(uint64_t(1)<<13))){
        p.sessionWorld=engine::function<void*(*)(void*)>(0x3ed83b0)(p.session);p.worldQueryKnown=true;
    }
    mark(14,p.worldQueryKnown&&p.sessionWorld==p.world);
    p.stateClass=engine::Class(0x42aae70);p.stateDefault=Read<void*>(p.stateClass,0x120);
    p.stateDefaultFlags=Read<uint32_t>(p.stateDefault,8);p.engineDefaultName=Read<Name>(p.stateDefault,0x4c8);
    p.currentStateClass=Read<void*>(p.mode,0x490);p.currentStateDefault=Read<void*>(p.currentStateClass,0x120);
    p.currentDefaultName=Read<Name>(p.currentStateDefault,0x4c8);
    p.modeCdoStateClass=Read<void*>(startup.modeCDO,0x490);p.modeCdoStateDefault=Read<void*>(p.modeCdoStateClass,0x120);
    p.modeDefaultName=Read<Name>(p.modeCdoStateDefault,0x4c8);
    p.bootstrapState=startup.bootstrapPS;p.bootstrapName=Read<Name>(p.bootstrapState,0x4c8);
    mark(15,p.stateDefault!=nullptr);mark(16,Read<void*>(p.stateDefault,0x10)==p.stateClass);
    mark(17,(p.stateDefaultFlags&0x10)!=0);mark(18,Exact(p.sessionName,p.engineDefaultName));
    mark(19,!state.gameSession||(state.gameSession==p.session&&Live(p.session,state.gameSessionIdentity)));
    p.sessionFactory=Read<void*>(Read<void*>(p.mode),0x7d8);
    mark(20,p.sessionFactory==reinterpret_cast<void*>(engine::image+0x71836f0));
    // Exactly the evaluator used by InitializeMetadata, with all subconditions
    // and native-layout contracts. Preflight never caches gameplay acceptance.
    p.endMetadata=ReadEndMetadata(p.gs,true);
    p.endFunction=p.endMetadata.endFunction;p.endOwner=p.endMetadata.endOwner;
    p.endOwner20=Read<void*>(p.endFunction,0x20);p.expectedEndOwner=p.endMetadata.expectedEndOwner;
    p.endBytes=p.endMetadata.endBytes;p.endFlags=p.endMetadata.endFlags;p.endExec=p.endMetadata.endExec;
    mark(21,(p.endMetadata.passed&(uint64_t(3)<<22))==(uint64_t(3)<<22));
    mark(23,p.endMetadata.checked&&p.endMetadata.failed==0);
    mark(22,OriginalShooterSessionProfilesMatch(p.sourceProfileFailure));
    CacheModeContextPreflight(p);
}
inline void PollOriginalModePreflight(void* world){
    __try{PollOriginalModePreflightImpl(world);}__except(EXCEPTION_EXECUTE_HANDLER){
        auto p=CopyModeContextPreflight();p.sampledAt=GetTickCount64();p.readException=true;CacheModeContextPreflight(p);
    }
}
inline std::string OriginalModePreflightJson(){
    const auto p=CopyModeContextPreflight();local_server_match::Json out;
    const auto b=[](bool value){return value?"true":"false";};
    const auto u=[](uint64_t value){return static_cast<unsigned long long>(value);};
    const auto rva=[](void* value){const auto at=reinterpret_cast<uintptr_t>(value);
        return static_cast<unsigned long long>(at>=engine::image&&at-engine::image<0xe000000?at-engine::image:0);};
    out.Add("{\"sampled\":%s,\"sampled_at_ms\":%llu,\"samples\":%llu,\"read_exception\":%s,\"passed_mask\":%llu,\"failed_mask\":%llu,\"ready_query\":%s,\"world_query_known\":%s,\"failed_gates\":[",
        b(p.sampled),u(p.sampledAt),u(p.samples),b(p.readException),u(p.passed),u(p.failed),b(p.readyQuery),b(p.worldQueryKnown));
    bool comma=false;for(size_t n=0;n<std::size(ModeContextGateNames);++n)if(p.failed&(uint64_t(1)<<n)){
        out.Add("%s",comma?",":"");out.String(ModeContextGateNames[n]);comma=true;}
    out.Add("],\"world\":\"%p\",\"mode\":\"%p\",\"gs\":\"%p\",\"session\":\"%p\",\"session_class\":\"%p\",\"expected_session_class\":\"%p\",\"mode_session_class\":\"%p\",\"mode_cdo_session_class\":\"%p\",\"session_table_rva\":%llu,\"session_end_rva\":%llu,\"session_world_getter_rva\":%llu,\"session_role\":%u,\"session_world\":\"%p\",\"session_outer\":\"%p\",\"engine_state_cdo\":\"%p\",\"engine_state_cdo_flags\":%u,\"current_state_class\":\"%p\",\"current_state_cdo\":\"%p\",\"mode_cdo_state_class\":\"%p\",\"mode_cdo_state_cdo\":\"%p\",\"bootstrap_ps\":\"%p\"",
        p.world,p.mode,p.gs,p.session,p.sessionClass,p.expectedSessionClass,p.modeSessionClass,p.modeCdoSessionClass,
        rva(p.sessionTable),rva(p.sessionEnd),rva(p.sessionWorldEntry),static_cast<unsigned>(p.sessionRole),p.sessionWorld,p.sessionOuter,
        p.stateDefault,p.stateDefaultFlags,p.currentStateClass,p.currentStateDefault,p.modeCdoStateClass,p.modeCdoStateDefault,p.bootstrapState);
    out.Add(",\"session_factory_rva\":%llu,\"end_function\":\"%p\",\"end_owner28\":\"%p\",\"end_owner20\":\"%p\",\"expected_end_owner\":\"%p\",\"end_parms_size\":%u,\"end_flags\":%u,\"end_exec_rva\":%llu,\"session_source_profile_failure_rva\":%u",
        rva(p.sessionFactory),p.endFunction,p.endOwner,p.endOwner20,p.expectedEndOwner,static_cast<unsigned>(p.endBytes),p.endFlags,rva(p.endExec),p.sourceProfileFailure);
    out.Add(",\"end_metadata\":");out.value+=EndMetadataJson(p.endMetadata);
    const auto name=[&](const char* key,const Name& value){out.Add(",\"%s\":[%u,%u,%u]",key,value.words[0],value.words[1],value.words[2]);};
    name("session_object_name",p.sessionObjectName);name("session_class_name",p.sessionClassName);name("session_name",p.sessionName);
    name("engine_ps_cdo_name",p.engineDefaultName);name("current_ps_cdo_name",p.currentDefaultName);
    name("mode_ps_cdo_name",p.modeDefaultName);name("bootstrap_ps_name",p.bootstrapName);
    out.Add(",\"same_end_predicates_diagnostic_only\":true,\"weak_serial_allocations\":0,\"gameplay_writes\":0,\"end_calls\":0}");return out.value;
}

inline bool OriginalModeContext(){
    const auto& startup=local_server_mode_startup::GetState();
    if(!startup.configured||startup.world!=state.world||startup.mode!=state.mode||startup.gs!=state.gameState
       ||original_players::createdGameState||!original_players::IsA(state.mode,0x6c8ec60)
       ||!local_server_mode_startup::ReadyForAdoption(state.world,startup.bootstrapPC)
       ||!local_server_mode_startup::OwnsModeTable(state.world,state.mode,Read<void**>(state.mode))
       ||Read<void*>(Read<void*>(state.mode),0x7d8)!=reinterpret_cast<void*>(engine::image+0x71836f0))return false;
    // InitGame dispatches the original v7D8 override71836F0, whose real
    // TSubclassOf result is ShooterGameSession6C9CE60. The inherited class
    // field478 remains EngineGameSession and is not this factory's source.
    // InitGame's real GameSession is an additional original EndMatch dependency.
    // Original InitOptions copies SessionName from the bootstrap EnginePS CDO.
    // Preserve that real name and the original session-existence query/branch;
    // clearing it to force a branch would violate the original lifecycle.
    const auto session=Read<void*>(state.mode,0x4d0);
    if(!session||Read<void*>(session,0x10)!=engine::Class(0x6c9ce60)
       ||!OriginalShooterSessionTable(Read<void*>(session))
       ||Read<void*>(Read<void*>(session),0x858)!=reinterpret_cast<void*>(engine::image+0x4439c70)
       ||Read<uint8_t>(session,0x224)!=3
       ||Read<void*>(Read<void*>(session),0x1b8)!=reinterpret_cast<void*>(engine::image+0x3ed83b0)
       ||engine::function<void*(*)(void*)>(0x3ed83b0)(session)!=state.world)return false;
    const auto stateClass=engine::Class(0x42aae70),stateDefault=Read<void*>(stateClass,0x120);
    if(!stateDefault||Read<void*>(stateDefault,0x10)!=stateClass||(Read<uint32_t>(stateDefault,8)&0x10)==0
       ||!Exact(Read<Name>(session,0x478),Read<Name>(stateDefault,0x4c8)))return false;
    if(state.gameSession)return session==state.gameSession&&Live(session,state.gameSessionIdentity);
    if(!original_inventory_slots::CaptureIdentity(session,state.gameSessionIdentity))return false;
    state.gameSession=session;return true;
}
inline bool Context(){
    if(!Thread()||!Live(state.world,state.worldIdentity)||!Live(state.gameState,state.gameStateIdentity)||!Live(state.mode,state.modeIdentity)
       ||state.world!=original_players::world||Read<void*>(state.world,0x178)!=state.gameState||Read<void*>(state.world,0x170)!=state.mode
       ||Read<void*>(state.gameState,0x470)!=state.mode||Read<void*>(state.mode,0x4d8)!=state.gameState
       ||Read<uint8_t>(state.gameState,0x224)!=3||(Read<uint8_t>(state.world,0x156)&0x80)==0)
        return Fail(5107,"authority_match_end_native_world_game_state_or_epoch_changed");
    const int mode=engine::function<int(*)(void*)>(0x3efc820)(state.gameState);
    const bool admittedMode=state.originalMode?OriginalModeContext():
        original_players::IsA(state.mode,0x439aa40)&&!original_players::IsA(state.mode,0x43c93e0);
    if((mode!=1&&mode!=2)||!admittedMode
       ||!original_players::IsA(state.gameState,original_players::ShooterGameStateClass)||!original_players::IsA(state.gameState,0x43c9540))
        return Fail(5108,"authority_match_end_requires_exact_owned_native_or_generic_mode_and_server_state");
    return true;
}
inline uint64_t Mix(uint64_t hash,uint64_t value){for(unsigned n=0;n<8;++n){hash=(hash^static_cast<uint8_t>(value))*1099511628211ULL;value>>=8;}return hash;}
inline bool SnapshotTotals(void* winnerPS,EndReason reason,const ActualTotals& totals,uint64_t& digest){
    const auto& match=local_server_match::GetState();
    if(!match.initialized||!match.started||!match.ended||match.error||match.world!=state.world||match.gameState!=state.gameState
       ||!local_server_match::FreeForAll()||(reason!=EndReason::KillLimit&&reason!=EndReason::TimeLimit)||reason!=match.endReason||!totals.sessionEpoch
       ||!totals.players||totals.count<2||totals.count>MaxPlayers||totals.count!=match.participants
       ||!std::isfinite(totals.elapsedSeconds)||totals.elapsedSeconds<0)
        return Fail(5109,"authority_match_end_requires_actual_completed_ffa_match_totals");
    if(reason==EndReason::TimeLimit&&totals.elapsedSeconds<local_server_match::DurationSeconds)
        return Fail(5110,"authority_match_end_time_limit_not_reached_in_authority_totals");
    std::array<bool,MaxPlayers> seen{};uint32_t leading=0,ties=0;int32_t leader=-1;
    for(size_t n=0;n<totals.count;++n){const auto& source=totals.players[n];const local_server_match::AuthorityPlayer* actual=nullptr;
        for(const auto& p:local_server_match::AuthorityPlayers())if(p.present&&p.ps==source.playerState){actual=&p;break;}
        if(!actual||seen[actual->index]||source.pawn!=actual->pawn||source.epoch!=actual->epoch||!source.epoch
           ||source.kills!=actual->kills||source.deaths!=actual->deaths||source.kills>INT32_MAX||source.deaths>INT32_MAX
           ||!original_players::Registered(source.playerState))
            return Fail(5111,"authority_match_end_total_row_not_current_native_participant_epoch");
        seen[actual->index]=true;auto& record=records[actual->index];record={};record.used=true;record.index=actual->index;record.total=source;
        record.currentLifeDead=!actual->alive&&actual->phase==deathmatch_rules::Phase::DeadWaiting;
        const auto spawned=Read<void*>(source.playerState,0xbc8);
        if(spawned!=source.pawn&&!(record.currentLifeDead&&!spawned))
            return Fail(5111,"authority_match_end_total_row_not_current_native_participant_epoch");
        record.team=Read<void*>(source.playerState,0x6a8);record.teamActor=actual->teamActor;
        if(!original_players::IsA(record.team,deathmatch_rules::FFATeamClass)||Read<void*>(record.team,0xc8)!=source.playerState
           ||!original_players::IsA(record.teamActor,0x65af170)||Read<void*>(record.teamActor,0x478)!=record.team
           ||Read<void*>(source.playerState,0x758)!=record.teamActor
           ||!original_inventory_slots::CaptureIdentity(source.playerState,record.psIdentity)
           ||!original_inventory_slots::CaptureIdentity(source.pawn,record.pawnIdentity)
           ||!original_inventory_slots::CaptureIdentity(record.team,record.teamIdentity)
           ||!original_inventory_slots::CaptureIdentity(record.teamActor,record.teamActorIdentity))
            return Fail(5112,"authority_match_end_real_ffa_team_or_assigned_player_identity_missing");
        auto score=original_scoreboard::Find(source.playerState);
        if(!score||!original_scoreboard::ReadNativeScore(*score)||score->kills<static_cast<int32_t>(source.kills)||score->deaths<static_cast<int32_t>(source.deaths))
            return Fail(5113,"authority_match_end_native_stats_do_not_cover_confirmed_match_totals");
        record.nativeKills=score->kills;record.nativeDeaths=score->deaths;
        record.shortPointsBefore=record.shortPointsAfter=Read<int32_t>(record.teamActor,0x4b8);
        if(leader<0||source.kills>leading){leader=static_cast<int32_t>(actual->index);leading=source.kills;ties=1;}
        else if(source.kills==leading)++ties;
    }
    for(const auto& p:local_server_match::AuthorityPlayers())if(p.present&&!seen[p.index])return Fail(5114,"authority_match_end_missing_current_participant_total");
    const int32_t winner=ties==1?leader:-1;
    if(winner!=match.winner||winnerPS!=(winner>=0?records[static_cast<size_t>(winner)].total.playerState:nullptr)
       ||(reason==EndReason::KillLimit&&(winner<0||leading<local_server_match::KillLimit)))
        return Fail(5115,"authority_match_end_winner_or_kill_limit_disagrees_with_authority_totals");
    digest=Mix(14695981039346656037ULL,totals.sessionEpoch);digest=Mix(digest,static_cast<uint64_t>(reason));
    for(const auto& row:records)if(row.used){digest=Mix(digest,row.index);digest=Mix(digest,reinterpret_cast<uintptr_t>(row.total.playerState));
        digest=Mix(digest,reinterpret_cast<uintptr_t>(row.total.pawn));digest=Mix(digest,row.total.epoch);digest=Mix(digest,row.total.kills);digest=Mix(digest,row.total.deaths);}
    state.winner=winner;state.leadingKills=leading;state.leadingTies=ties;state.participants=totals.count;
    state.totalsVerified=true;return true;
}
inline bool SampleOriginalClock(){
    state.fixedStep=Read<void*>(state.gameState,0xcc0);
    if(!state.fixedStep||Read<uint8_t>(state.fixedStep,0xd9)!=128)return Fail(5116,"authority_match_end_original_world_128hz_manager_missing");
    // Read the real current participant pawn's retained native authority
    // clock. A dead current life is permitted: ending the match need not
    // manufacture a possession or respawn merely to read its original time.
    for(const auto& row:records)if(row.used&&Live(row.total.pawn,row.pawnIdentity)&&Live(row.total.playerState,row.psIdentity)){
        const auto pawn=row.total.pawn,movement=Read<void*>(pawn,0x4f8);
        const auto pawnState=Read<void*>(pawn,0x488),spawned=Read<void*>(row.total.playerState,0xbc8);
        if(Read<uint8_t>(pawn,0x224)!=3||(pawnState!=row.total.playerState&&!(row.currentLifeDead&&!pawnState))
           ||(spawned!=pawn&&!(row.currentLifeDead&&!spawned))||!movement||Read<void*>(movement,0xc8)!=pawn
           ||Read<void*>(movement,0x1a8)!=pawn||Read<void*>(movement,0x190)!=pawn
           ||engine::function<void*(*)(void*)>(0x787b6c0)(pawn)!=movement)continue;
        auto clock=engine::function<void*(*)(void*)>(0x7892e10)(movement);auto prediction=Read<void*>(movement,0x7f0);
        if(!clock||!prediction||clock!=static_cast<uint8_t*>(prediction)+0xf0
           ||Read<void*>(clock)!=reinterpret_cast<void*>(engine::image+0xb600a60))continue;
        const auto vb=Read<void*>(clock,0x10);const int32_t offset=Read<int32_t>(vb,4);
        if(!vb||offset<0||offset>0x1000)continue;
        const uint32_t index=Read<uint32_t>(clock,static_cast<size_t>(offset)+0x18),step=Read<uint32_t>(clock,static_cast<size_t>(offset)+0x1c);
        const float time=engine::function<float(*)(void*)>(0x789df70)(movement);
        auto worldEntry=Read<void*>(Read<void*>(pawn),0x1b8);
        if(step!=1||!std::isfinite(time)||time<state.beforeInfo.authStart||time!=static_cast<float>(index)*(1.f/128.f)
           ||!worldEntry||reinterpret_cast<void*(*)(void*)>(worldEntry)(pawn)!=state.world)continue;
        state.clockPlayerIndex=row.index;state.clockPawn=pawn;state.movement=movement;state.clock=clock;
        state.clockIndex=index;state.clockStep=step;state.clockBaseOffset=static_cast<uint32_t>(offset);state.nativeClockTime=time;return true;
    }return Fail(5117,"authority_match_end_current_native_pawn_epoch_clock_not_observed");
}
inline bool SampleActive(){
    uint8_t active=255;++state.activeQueries;
    if(!Dispatch(state.gameState,state.activeGetter,&active)||active>1
       ||!original_players::Read(state.gameState,0x7d8,state.afterInfo)||state.afterInfo.active!=active)
        return Fail(5118,"authority_match_end_original_active_getter_or_game_info_readback_failed");
    state.nativeActive=active!=0;return true;
}
inline bool CompletePublication(){
    if(!Context())return false;++state.polls;
    if(!SampleActive())return false;
    if(state.afterInfo.authStart!=state.beforeInfo.authStart||state.afterInfo.authEnd!=state.nativeEndTime)
        return Fail(5119,"authority_match_end_canonical_end_timeline_changed");
    if(state.nativeActive){state.stage="authority_match_end_waiting_original_timestamp_maturation";return true;}
    if(!state.resultStored){state.operation="publish_actual_authority_match_result";
        if(!original_players::Write(state.gameState,0x710,state.result))return Fail(5120,"authority_match_end_native_result_record_write_failed");state.resultStored=true;}
    if(!state.matchStateStored){
        if(state.originalMode){
            state.operation="original_game_mode_end_match_full_lifecycle";
            const auto progress=Read<Name>(reinterpret_cast<void*>(engine::image),0xd7e6d70);
            if(state.originalModeEndAttempted||!Exact(Read<Name>(state.mode,0x518),progress)
               ||!Exact(Read<Name>(state.gameState,0x4c0),progress))
                return Fail(5134,"authority_match_end_original_mode_not_in_progress_or_already_attempted");
            state.originalModeEndAttempted=true;++state.originalModeEndCalls;
            if(!Dispatch(state.mode,state.originalEndFunction,nullptr))
                return Fail(5135,"authority_match_end_original_mode_end_dispatch_failed_no_replay");
            state.originalModeEndReturned=true;state.afterModeMatch=Read<Name>(state.mode,0x518);
            if(!OriginalModeContext()||!Exact(state.afterModeMatch,state.postMatch)
               ||!Exact(Read<Name>(state.gameState,0x4c0),state.postMatch)
               ||!Exact(Read<Name>(state.gameState,0x4cc),state.postMatch))
                return Fail(5136,"authority_match_end_original_mode_and_gs_post_match_readback_failed");
            // Original SetMatchState executes the full GS notification once.
            // Its PreviousMatchState readback proves that native chain returned;
            // calling OnRep_MatchState again would repeat the native handlers.
            state.matchNotifyReturned=true;++state.matchStateNotifications;
        }else{
            state.operation="publish_authority_waiting_post_match";
            if(!original_players::Write(state.gameState,0x4c0,state.postMatch))
                return Fail(5121,"authority_match_end_native_match_state_write_failed");
        }
        state.matchStateStored=true;
    }
    if(!state.matchNotifyReturned){state.operation="original_game_state_full_match_state_notify";
        if(!Dispatch(state.gameState,state.onRepMatch,nullptr))return Fail(5122,"authority_match_end_original_match_state_notify_failed");
        state.matchNotifyReturned=true;++state.matchStateNotifications;}
    if(!state.resultNotifyReturned){state.operation="original_shooter_game_state_match_result_notify";
        if(!Dispatch(state.gameState,state.onRepResult,nullptr))return Fail(5123,"authority_match_end_original_result_notify_failed");
        state.resultNotifyReturned=true;++state.resultNotifications;}
    state.afterMatch=Read<Name>(state.gameState,0x4c0);state.afterPrevious=Read<Name>(state.gameState,0x4cc);
    const auto result=Read<MatchResult>(state.gameState,0x710);void* winning=nullptr;
    if(!Exact(state.afterMatch,state.postMatch)||!Exact(state.afterPrevious,state.postMatch)
       ||std::memcmp(&result,&state.result,sizeof(result))!=0
       ||!Dispatch(state.gameState,state.winningGetter,&winning)||winning!=state.winnerTeam)
        return Fail(5124,"authority_match_end_original_result_winner_or_post_match_readback_failed");
    for(auto& row:records)if(row.used){if(!Live(row.total.playerState,row.psIdentity)||!Live(row.team,row.teamIdentity)||!Live(row.teamActor,row.teamActorIdentity))
            return Fail(5125,"authority_match_end_participant_or_team_epoch_changed_during_publication");
        row.shortPointsAfter=Read<int32_t>(row.teamActor,0x4b8);}
    state.complete=true;state.operation="none";state.stage="authority_match_end_native_records_and_notifications_complete_client_ui_unproven";return true;
}
inline bool EndAuthorityMatchImpl(void* world,void* winnerPS,EndReason reason,const ActualTotals& totals){
    if(state.error||state.restartRequired||!Thread())return false;
    if(state.initialized&&state.world!=world)return Fail(5126,"authority_match_end_world_changed_requires_new_pair");
    if(!state.initialized){state.world=world;state.gameState=Read<void*>(world,0x178);state.mode=Read<void*>(world,0x170);state.gameThread=GetCurrentThreadId();
        const auto& startup=local_server_mode_startup::GetState();
        state.originalMode=startup.configured&&startup.world==world;
        if(!original_inventory_slots::CaptureIdentity(world,state.worldIdentity)||!original_inventory_slots::CaptureIdentity(state.gameState,state.gameStateIdentity)
           ||!original_inventory_slots::CaptureIdentity(state.mode,state.modeIdentity)||!Context()||!VerifyProfiles()||!InitializeMetadata())return false;
        state.initialized=true;
    }
    if(state.publicationAttempted){
        if(state.winnerPS!=winnerPS||state.reason!=reason||state.sessionEpoch!=totals.sessionEpoch
           ||totals.count!=state.participants||!totals.players)
            return Fail(5127,"authority_match_end_conflicting_repeated_completion_request");
        std::array<bool,MaxPlayers> seen{};
        for(size_t n=0;n<totals.count;++n){const auto& source=totals.players[n];const PlayerRecord* found=nullptr;
            for(const auto& row:records)if(row.used&&row.total.playerState==source.playerState){found=&row;break;}
            if(!found||seen[found->index]||found->total.pawn!=source.pawn||found->total.epoch!=source.epoch
               ||found->total.kills!=source.kills||found->total.deaths!=source.deaths)
                return Fail(5127,"authority_match_end_conflicting_repeated_completion_request");
            seen[found->index]=true;
        }
        // Keep the first native before/after evidence. A completed match need
        // not retain a dead pawn solely to accept the same logical receipt.
        ++state.idempotentCalls;return state.complete?Context():CompletePublication();
    }
    uint64_t digest=0;if(!SnapshotTotals(winnerPS,reason,totals,digest))return false;
    state.beforeMatch=Read<Name>(state.gameState,0x4c0);state.beforePrevious=Read<Name>(state.gameState,0x4cc);
    const auto progress=Read<Name>(reinterpret_cast<void*>(engine::image),0xd7e6d70);
    state.beforeResult=Read<MatchResult>(state.gameState,0x710);state.beforeInfo=Read<GameInfo>(state.gameState,0x7d8);
    state.beforeModeMatch=Read<Name>(state.mode,0x518);
    if(state.originalMode&&!Exact(state.beforeModeMatch,progress))
        return Fail(5134,"authority_match_end_actual_original_mode_not_in_progress");
    if(!progress.words[0]||!Exact(state.beforeMatch,progress)||!Exact(state.beforePrevious,progress)
       ||state.beforeResult.completion!=0||state.beforeResult.winningTeam||state.beforeInfo.authStart<0
       ||!std::isfinite(state.beforeInfo.authStart)||!std::isfinite(state.beforeInfo.authEnd)
       ||(state.beforeInfo.authEnd<0&&state.beforeInfo.authEnd!=-1)
       ||(state.beforeInfo.authEnd>=0&&state.beforeInfo.authEnd<state.beforeInfo.authStart))
        return Fail(5128,"authority_match_end_requires_unchanged_actual_in_progress_match_records");
    state.winnerPS=winnerPS;state.winnerTeam=state.winner>=0?records[static_cast<size_t>(state.winner)].team:nullptr;
    auto pending=Read<void*>(state.gameState,0x738);
    if(pending&&pending!=state.winnerTeam)return Fail(5129,"authority_match_end_original_pending_winning_team_disagrees");
    if(!SampleOriginalClock())return false;
    state.nativeEndTime=state.beforeInfo.authEnd>=0&&state.beforeInfo.authEnd<=state.nativeClockTime?state.beforeInfo.authEnd:state.nativeClockTime;
    state.result={};state.result.winningTeam=state.winnerTeam;state.result.winningPoints=state.winner>=0?static_cast<int32_t>(state.leadingKills):0;
    state.result.completion=state.winner>=0?1:7; // original Completed / authority ForceDraw
    state.digest=digest;state.sessionEpoch=totals.sessionEpoch;state.reason=reason;state.elapsedSeconds=totals.elapsedSeconds;state.publicationAttempted=true;
    if(state.beforeInfo.authEnd!=state.nativeEndTime){state.operation="publish_native_authority_end_timestamp";
        if(!original_players::Write(state.gameState,0x7f8,state.nativeEndTime))return Fail(5130,"authority_match_end_original_end_timestamp_write_failed");
        state.timelineWritten=true;auto previous=state.beforeInfo;
        if(!Dispatch(state.gameState,state.onRepInfo,&previous))return Fail(5131,"authority_match_end_original_game_info_notify_failed");
        ++state.gameInfoNotifications;
    }
    state.timelinePublished=true;return CompletePublication();
}
inline bool PollAuthorityImpl(){
    if(state.error||state.restartRequired||!Thread())return false;
    if(!state.publicationAttempted)return true;if(state.complete)return Context();
    const uint64_t now=GetTickCount64();if(now<nextPoll)return true;nextPoll=now+50;return CompletePublication();
}
inline int Capture(EXCEPTION_POINTERS* exception){
    auto at=exception&&exception->ExceptionRecord?reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=at>=engine::image?at-engine::image:at;state.restartRequired=true;
    Fail(exception&&exception->ExceptionRecord?exception->ExceptionRecord->ExceptionCode:5132,"authority_match_end_native_exception_requires_new_pair");return EXCEPTION_EXECUTE_HANDLER;
}
inline bool EndAuthorityMatch(void* world,void* actualWinnerPS,EndReason reason,const ActualTotals& totals){
    __try{return EndAuthorityMatchImpl(world,actualWinnerPS,reason,totals);}__except(Capture(GetExceptionInformation())){return false;}}
inline bool PollAuthority(){__try{return PollAuthorityImpl();}__except(Capture(GetExceptionInformation())){return false;}}
inline bool CaptureTotalsFromMatch(std::array<PlayerTotal,MaxPlayers>& storage,ActualTotals& totals,uint64_t sessionEpoch=1){
    const auto& match=local_server_match::GetState();if(!original_players::Thread()||!match.ended||!match.started||match.startedAt<0||!sessionEpoch)return false;
    size_t count=0;for(const auto& p:local_server_match::AuthorityPlayers())if(p.present)storage[count++]={p.ps,p.pawn,p.epoch,p.kills,p.deaths};
    totals={storage.data(),count,state.publicationAttempted?state.elapsedSeconds:(std::max)(0.,match.now-match.startedAt),sessionEpoch};return count>=2;
}
inline std::string StateJson(){
    local_server_match::Json out;const auto b=[](bool x){return x?"true":"false";};
    out.Add("{\"stage\":");out.String(state.stage);out.Add(",\"operation\":");out.String(state.operation);
    out.Add(",\"error\":%u,\"accepted\":%s,\"native_complete\":%s,\"native_active\":%s,\"participants\":%zu,\"winner_index\":%d,\"leading_total_kills\":%u,\"leading_ties\":%u,\"reason\":%u,\"session_epoch\":%llu,\"rule_elapsed_seconds\":%.6f,\"native_clock_time\":%.6f,\"native_end_time\":%.6f,\"clock_player_index\":%zu,\"clock_index\":%u,\"clock_step\":%u,\"result_completion\":%u,\"result_winning_points\":%d,\"game_info_notifications\":%u,\"match_state_notifications\":%u,\"result_notifications\":%u,\"direct_team_points_writes\":0,\"direct_phase_or_active_writes\":0,\"client_result_ui_verified\":false,\"runtime_acceptance_claimed\":false,\"players\":[",
        state.error,b(state.publicationAttempted),b(state.complete),b(state.nativeActive),state.participants,state.winner,state.leadingKills,state.leadingTies,static_cast<unsigned>(state.reason),
        static_cast<unsigned long long>(state.sessionEpoch),state.elapsedSeconds,state.nativeClockTime,state.nativeEndTime,state.clockPlayerIndex,state.clockIndex,state.clockStep,
        static_cast<unsigned>(state.result.completion),state.result.winningPoints,state.gameInfoNotifications,state.matchStateNotifications,state.resultNotifications);
    bool comma=false;for(const auto& p:records)if(p.used){out.Add("%s{\"index\":%zu,\"epoch\":%u,\"total_kills\":%u,\"total_deaths\":%u,\"native_lifetime_kills\":%d,\"native_lifetime_deaths\":%d,\"short_team_points_before\":%d,\"short_team_points_after\":%d}",
        comma?",":"",p.index,p.total.epoch,p.total.kills,p.total.deaths,p.nativeKills,p.nativeDeaths,p.shortPointsBefore,p.shortPointsAfter);comma=true;}
    out.Add("]");
    out.Add(",\"original_mode_preflight\":");out.value+=OriginalModePreflightJson();
    out.Add(",\"actual_end_metadata\":");out.value+=EndMetadataJson(CopyActualEndMetadata());
    out.Add(",\"original_mode\":%s,\"original_mode_end_attempted\":%s,\"original_mode_end_returned\":%s,\"original_mode_end_calls\":%u,\"direct_match_state_writes\":%u,\"game_session\":\"%p\"}",
        b(state.originalMode),b(state.originalModeEndAttempted),b(state.originalModeEndReturned),
        state.originalModeEndCalls,state.originalMode?0U:(state.matchStateStored?1U:0U),state.gameSession);
    return out.value;
}
inline bool WriteDiagnostics(const wchar_t* path){if(!path)return false;FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;
    const auto json=StateJson();const bool written=std::fwrite(json.data(),1,json.size(),file)==json.size()&&!std::ferror(file);std::fclose(file);return written;}
} // namespace local_server_match_end
