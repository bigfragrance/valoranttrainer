#pragma once
#include "local_server_services.h"
#include <array>
#include <string>

// Local presentation metadata only. Actual native UniqueNetId values remain
// untouched and travel on the original connection. Cache readiness precedes
// PlayerState replication so an offline UI lookup never needs an SDK request.
namespace local_server_names {
inline constexpr uint64_t Magic=0x3153454d414e5456ULL;
inline constexpr size_t Capacity=32;
struct Entry {uint32_t slot=0,epoch=0;wchar_t subject[256]{},label[64]{},tag[16]{};};
struct Roster {uint64_t magic=Magic;uint32_t version=1,generation=0,count=0,publisher=0;Entry entries[Capacity]{};};
struct Ack {uint64_t magic=Magic,digest=0;uint32_t generation=0,count=0,publisher=0,recipient=0;};
static_assert(sizeof(Entry)==680&&sizeof(Roster)==21784&&sizeof(Ack)==32);
struct State {
    bool initialized=false,host=false,ready=false,publicationPending=false,publicationIsAck=false;
    DWORD gameThread=0,lastWin32Error=ERROR_SUCCESS;
    DWORD lastProbeWin32Error=ERROR_SUCCESS;bool lastProbeAttempted=false;
    uint32_t publicationAttempts=0,pendingGeneration=0;
    uint64_t firstPublishAt=0,nextPublishAt=0,pendingDigest=0;
    uint64_t publicationRetries=0,publicationFailures=0,publicationSuccesses=0,publicationExhaustions=0;
    const char* lastIoStage="none";const char* lastProbeStage="none";
    unsigned error=0;uint32_t appliedGeneration=0,emptySubjects=0;
    uint64_t lastPoll=0,digest=0;
    const char* stage="local_roster_not_initialized";
};
inline State state;
inline Roster roster;
inline std::wstring path,ackPath;
inline const State& GetState(){return state;}
inline bool Fail(unsigned error,const char* reason){state.error=error;state.stage=reason;state.ready=false;state.publicationPending=false;return false;}
inline uint64_t Digest(const Roster& value){
    auto data=reinterpret_cast<const uint8_t*>(&value);uint64_t hash=14695981039346656037ULL;
    for(size_t i=0;i<sizeof(value);++i)hash=(hash^data[i])*1099511628211ULL;
    return hash;
}
inline bool ReadFileExact(const std::wstring& file,void* buffer,DWORD bytes){
    auto handle=CreateFileW(file.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(handle==INVALID_HANDLE_VALUE)return false;
    LARGE_INTEGER size{};DWORD read=0;
    bool ok=GetFileSizeEx(handle,&size)&&size.QuadPart==bytes&&ReadFile(handle,buffer,bytes,&read,nullptr)&&read==bytes;
    CloseHandle(handle);return ok;
}
inline constexpr uint64_t PollMilliseconds=50,MaximumPublishMilliseconds=2000;
inline constexpr uint32_t MaximumPublishAttempts=40;
enum class IoPhase:uint8_t {None,CreateTemporary,WriteTemporary,ShortWrite,CloseTemporary,ReplaceDestination,ProbeDeleteAccess,CloseDeleteProbe};
struct FileResult {
    bool okay=false;DWORD error=ERROR_SUCCESS;IoPhase phase=IoPhase::None;DWORD written=0;
    bool probeAttempted=false;DWORD probeError=ERROR_SUCCESS;IoPhase probePhase=IoPhase::None;
};
inline Ack pendingAck;
inline const char* IoStage(IoPhase phase){
    switch(phase){
    case IoPhase::CreateTemporary:return "create_temporary";
    case IoPhase::WriteTemporary:return "write_temporary";
    case IoPhase::ShortWrite:return "short_write";
    case IoPhase::CloseTemporary:return "close_temporary";
    case IoPhase::ReplaceDestination:return "replace_destination";
    case IoPhase::ProbeDeleteAccess:return "probe_destination_delete_access";
    case IoPhase::CloseDeleteProbe:return "close_destination_probe";
    default:return "none";
    }
}
inline bool Retryable(const FileResult& result){
    const bool direct=(result.phase==IoPhase::CreateTemporary||result.phase==IoPhase::WriteTemporary
        ||result.phase==IoPhase::ReplaceDestination)
        &&(result.error==ERROR_SHARING_VIOLATION||result.error==ERROR_LOCK_VIOLATION);
    const bool provenMoveSharing=result.phase==IoPhase::ReplaceDestination&&result.error==ERROR_ACCESS_DENIED
        &&result.probeAttempted&&result.probePhase==IoPhase::ProbeDeleteAccess
        &&(result.probeError==ERROR_SHARING_VIOLATION||result.probeError==ERROR_LOCK_VIOLATION);
    return !result.okay&&(direct||provenMoveSharing);
}
inline FileResult SaveFileResult(const std::wstring& file,const void* buffer,DWORD bytes){
    const auto temporary=file+L".tmp";
    auto handle=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(handle==INVALID_HANDLE_VALUE){const DWORD error=GetLastError();return {false,error,IoPhase::CreateTemporary,0};}
    DWORD written=0;
    if(!WriteFile(handle,buffer,bytes,&written,nullptr)){
        const DWORD error=GetLastError();CloseHandle(handle);return {false,error,IoPhase::WriteTemporary,written};
    }
    if(written!=bytes){CloseHandle(handle);return {false,ERROR_SUCCESS,IoPhase::ShortWrite,written};}
    if(!CloseHandle(handle)){const DWORD error=GetLastError();return {false,error,IoPhase::CloseTemporary,written};}
    if(!MoveFileExW(temporary.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){
        const DWORD error=GetLastError();FileResult result{false,error,IoPhase::ReplaceDestination,written};
        if(error==ERROR_ACCESS_DENIED){
            // Request DELETE access only to diagnose sharing. No delete flag,
            // disposition, rename, content write or delete call is issued here.
            result.probeAttempted=true;result.probePhase=IoPhase::ProbeDeleteAccess;
            auto probe=CreateFileW(file.c_str(),DELETE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(probe==INVALID_HANDLE_VALUE){const DWORD probeError=GetLastError();result.probeError=probeError;}
            else if(!CloseHandle(probe)){
                const DWORD probeError=GetLastError();result.probeError=probeError;result.probePhase=IoPhase::CloseDeleteProbe;
            }
        }
        return result;
    }
    return {true,ERROR_SUCCESS,IoPhase::None,written};
}
inline void RememberFileFailure(const FileResult& result){
    state.lastWin32Error=result.error;state.lastIoStage=IoStage(result.phase);
    state.lastProbeAttempted=result.probeAttempted;state.lastProbeWin32Error=result.probeError;state.lastProbeStage=IoStage(result.probePhase);
}
inline bool SaveFile(const std::wstring& file,const void* buffer,DWORD bytes){
    const auto result=SaveFileResult(file,buffer,bytes);
    if(!result.okay)RememberFileFailure(result);
    return result.okay;
}
inline bool PublicationTuple(){
    if(!state.initialized||!state.publicationPending||state.publicationIsAck==state.host
       ||roster.magic!=Magic||roster.version!=1||!roster.generation||!roster.publisher||roster.count>Capacity
       ||state.pendingGeneration!=roster.generation||state.pendingDigest!=state.digest||state.digest!=Digest(roster))return false;
    if(state.host)return roster.publisher==GetCurrentProcessId();
    return pendingAck.magic==Magic&&pendingAck.digest==state.digest&&pendingAck.generation==roster.generation
        &&pendingAck.count==roster.count&&pendingAck.publisher==roster.publisher&&pendingAck.recipient==GetCurrentProcessId();
}
inline bool PublicationFailure(bool exhausted){
    if(exhausted)++state.publicationExhaustions;
    return Fail(state.host?3707U:3712U,state.host?"local_roster_metadata_publish_failed":"local_roster_client_ack_failed");
}
inline bool AdvancePublication(uint64_t now){
    if(!state.publicationPending)return true;
    if(!state.gameThread||state.gameThread!=GetCurrentThreadId())return Fail(3714,"local_roster_publication_wrong_thread");
    state.ready=false;
    if(!PublicationTuple())return Fail(3713,"local_roster_pending_publication_tuple_changed");
    if(now<state.firstPublishAt||now-state.firstPublishAt>=MaximumPublishMilliseconds||state.publicationAttempts>=MaximumPublishAttempts)
        return PublicationFailure(true);
    if(now<state.nextPublishAt)return true;
    ++state.publicationAttempts;
    const auto result=state.host?SaveFileResult(path,&roster,sizeof(roster)):SaveFileResult(ackPath,&pendingAck,sizeof(pendingAck));
    if(result.okay){
        state.publicationPending=false;++state.publicationSuccesses;
        if(state.host)state.stage="local_roster_waiting_client_cache_ack";
        else{state.appliedGeneration=pendingAck.generation;state.ready=true;state.stage="local_roster_client_cache_acknowledged";}
        return true;
    }
    RememberFileFailure(result);++state.publicationFailures;
    const auto failedAt=GetTickCount64();
    if(!Retryable(result))return PublicationFailure(false);
    if(state.publicationAttempts>=MaximumPublishAttempts||failedAt<state.firstPublishAt
       ||failedAt-state.firstPublishAt>=MaximumPublishMilliseconds)return PublicationFailure(true);
    ++state.publicationRetries;state.nextPublishAt=failedAt+PollMilliseconds;
    state.stage=state.host?"local_roster_metadata_publish_retry_pending":"local_roster_client_ack_retry_pending";return true;
}
inline bool BeginPublication(bool acknowledgment,const Ack* ack=nullptr){
    if(state.error)return false;
    if(!state.gameThread||state.gameThread!=GetCurrentThreadId())return Fail(3714,"local_roster_publication_wrong_thread");
    state.ready=false;
    if(state.publicationPending){
        if(acknowledgment!=state.publicationIsAck||acknowledgment)return Fail(3713,"local_roster_pending_publication_cannot_be_replaced");
        // Host registrations cache every new subject first, then publish the
        // newest complete generation. Retain the existing contention budget.
        state.pendingGeneration=roster.generation;state.pendingDigest=state.digest;return true;
    }
    const auto now=GetTickCount64();state.publicationPending=true;state.publicationIsAck=acknowledgment;
    state.publicationAttempts=0;state.firstPublishAt=now;state.nextPublishAt=now;
    state.pendingGeneration=roster.generation;state.pendingDigest=state.digest;
    if(acknowledgment){if(!ack)return Fail(3713,"local_roster_pending_ack_missing");pendingAck=*ack;}
    return AdvancePublication(now);
}
inline bool Initialize(const std::wstring& output,bool host){
    if(state.initialized)return state.host==host;
    auto base=output;
    while(!base.empty()&&(base.back()==L'\\'||base.back()==L'/'))base.pop_back();
    const auto at=base.find_last_of(L"\\/");
    if(at==std::wstring::npos)return Fail(3701,"local_roster_session_directory_invalid");
    base.resize(at+1);path=base+L"native-roster.bin";ackPath=base+L"native-roster-ack.bin";
    state.host=host;state.initialized=true;state.gameThread=GetCurrentThreadId();roster.publisher=host?GetCurrentProcessId():0;
    state.stage=host?"local_roster_waiting_native_subject":"local_roster_waiting_authority_metadata";return true;
}
inline bool Register(size_t slot,uint32_t epoch,void* ps,const wchar_t* label){
    if(state.error)return false;
    if(!state.initialized||!state.host||slot>=Capacity||!ps||!label||!label[0])return Fail(3702,"local_roster_registration_context_invalid");
    if(state.gameThread!=GetCurrentThreadId())return Fail(3714,"local_roster_registration_wrong_thread");
    wchar_t subject[256]{};
    if(!local_server_services::ReadPlayerSubjectKey(ps,subject,256)){
        if(local_server_services::State().error)return Fail(local_server_services::State().error,local_server_services::State().stage);
        return false;
    }
    if(!subject[0]){++state.emptySubjects;state.stage="local_roster_waiting_native_subject";return false;}
    size_t selected=Capacity;
    for(size_t i=0;i<roster.count;++i){
        if(roster.entries[i].slot==slot)selected=i;
        else if(std::wcscmp(roster.entries[i].subject,subject)==0)return Fail(3703,"local_roster_duplicate_player_subject");
    }
    if(selected<Capacity&&roster.entries[selected].epoch==epoch&&std::wcscmp(roster.entries[selected].subject,subject)==0)return true;
    state.ready=false;
    if(selected==Capacity){if(roster.count>=Capacity)return Fail(3704,"local_roster_full");selected=roster.count++;}
    auto& item=roster.entries[selected];item={};item.slot=static_cast<uint32_t>(slot);item.epoch=epoch;
    if(wcscpy_s(item.subject,subject)||wcscpy_s(item.label,label)||wcscpy_s(item.tag,L"LOCAL"))return Fail(3705,"local_roster_name_too_long");
    if(!local_server_services::CacheOfflineSubject(item.subject,item.label,item.tag))return Fail(3706,"local_roster_host_cache_failed");
    ++roster.generation;state.digest=Digest(roster);state.ready=false;
    return BeginPublication(false);
}
inline bool Poll(){
    if(!state.initialized||state.error)return false;
    if(state.gameThread!=GetCurrentThreadId())return Fail(3714,"local_roster_poll_wrong_thread");
    const auto now=GetTickCount64();if(now-state.lastPoll<PollMilliseconds)return true;state.lastPoll=now;
    if(state.publicationPending){
        if(!AdvancePublication(now))return false;
        // A completed client ACK belongs to the exact already-cached roster.
        // Observe a newer authority generation on the next normal Poll.
        if(!state.host||state.publicationPending)return true;
    }
    if(state.host){
        Ack ack;
        if(roster.generation&&ReadFileExact(ackPath,&ack,sizeof(ack))&&ack.magic==Magic&&ack.digest==state.digest
           &&ack.generation==roster.generation&&ack.count==roster.count&&ack.publisher==GetCurrentProcessId()&&ack.recipient){
            state.ready=true;state.appliedGeneration=ack.generation;state.stage="local_roster_both_native_caches_ready";
        }
        return true;
    }
    Roster pending;
    if(!ReadFileExact(path,&pending,sizeof(pending)))return true;
    if(pending.magic!=Magic||pending.version!=1||!pending.publisher||!pending.generation||pending.count>Capacity)
        return Fail(3708,"local_roster_metadata_invalid");
    if(pending.generation==state.appliedGeneration)return true;
    state.ready=false;
    for(size_t i=0;i<pending.count;++i){
        const auto& item=pending.entries[i];
        if(item.slot>=Capacity||!item.epoch||!item.subject[0]||!item.label[0]||!item.tag[0]
           ||!std::wmemchr(item.subject,0,256)||!std::wmemchr(item.label,0,64)||!std::wmemchr(item.tag,0,16))
            return Fail(3709,"local_roster_entry_invalid");
        for(size_t j=0;j<i;++j)if(pending.entries[j].slot==item.slot||std::wcscmp(pending.entries[j].subject,item.subject)==0)
            return Fail(3710,"local_roster_entry_not_unique");
        if(!local_server_services::CacheOfflineSubject(item.subject,item.label,item.tag))return Fail(3711,"local_roster_client_cache_failed");
    }
    roster=pending;state.digest=Digest(roster);state.ready=false;
    Ack ack;ack.digest=state.digest;ack.generation=roster.generation;ack.count=roster.count;
    ack.publisher=roster.publisher;ack.recipient=GetCurrentProcessId();
    return BeginPublication(true,&ack);
}
}
