#pragma once
#include "local_server_feedback_wire.h"
#include "training_control.h"
#include "local_server_names.h"
#include "local_server_owned_paths.h"
#include <Windows.h>
#include <array>
#include <cstdio>
#include <cwchar>
#include <string>

// Session-scoped UI receipts in the unused gap of the already owned V2 mapping.
// Client access is FILE_MAP_READ. No input, native RPC, score, clock or actor is
// changed here. The parent runner's unique role directory and native name ACK
// bind this reader to the exact local host/client pair.
namespace local_server_feedback_ledger {
namespace wire=local_server_feedback_wire;
inline constexpr uint32_t Offset=static_cast<uint32_t>(training_control::CommandOffset+training_control::CommandCapacity);
static_assert(Offset%8==0&&Offset+wire::WireBytes<=training_control::StateOffset);
struct State {
    bool configured=false,enabled=false,host=false,bound=false;
    uint64_t pairDigest=0,nextCommit=0,published=0,lookups=0,matched=0,missing=0,expired=0;
    uint64_t incomplete=0,rejected=0,invalidRecords=0,duplicates=0,openAttempts=0,nextOpenAt=0;
    DWORD gameThread=0;wire::Binding binding{};
    const char* stage="feedback_ledger_not_configured";
};
inline State state;
inline HANDLE peerMapping=nullptr;
inline const uint8_t* peerMemory=nullptr;
inline std::wstring pairDirectory;
inline std::array<wire::Record,wire::Capacity> hostRecords{};
inline bool Thread(){return state.configured&&(!state.gameThread||state.gameThread==GetCurrentThreadId());}
inline bool Enabled(){return state.configured&&state.enabled;}
inline uint64_t DirectoryDigest(std::wstring path){
    for(auto& c:path)if(c>=L'A'&&c<=L'Z')c=static_cast<wchar_t>(c-L'A'+L'a');
    return wire::Hash(path.data(),path.size()*sizeof(wchar_t));
}
inline bool Configure(const std::wstring& output,bool host,bool enabled){
    std::wstring path;
    if(!local_server_owned_paths::CanonicalAbsolute(output,path)
       ||!local_server_owned_paths::RoleDirectory(path,host?1U:2U))return false;
    const auto at=path.find_last_of(L'\\');
    if(at==std::wstring::npos||_wcsicmp(path.c_str()+at+1,host?L"host":L"client")!=0)return false;
    const auto parent=path.substr(0,at);
    if(state.configured)return state.host==host&&state.enabled==enabled&&_wcsicmp(parent.c_str(),pairDirectory.c_str())==0;
    pairDirectory=parent;state.configured=true;state.host=host;state.enabled=enabled;state.pairDigest=DirectoryDigest(parent);
    state.stage=enabled?"feedback_ledger_waiting_native_pair_identity":"feedback_ledger_not_ffa";return true;
}
inline bool ControlHeader(const uint8_t* memory,uint32_t pid,uint64_t& started){
    if(!memory)return false;training_control::Header header{};std::memcpy(&header,memory,sizeof(header));
    started=header.started;
    return header.magic==training_control::Magic&&header.version==training_control::Version&&header.bytes==training_control::Bytes
        &&header.pid==pid&&header.started;
}
inline bool CurrentRoster(){
    const auto& names=local_server_names::GetState();const auto& roster=local_server_names::roster;
    return names.initialized&&!names.error&&roster.magic==local_server_names::Magic&&roster.version==1
        &&roster.publisher&&roster.generation&&roster.count>=1&&roster.count<=32;
}
inline bool BindHost(){
    if(state.bound)return true;
    if(!state.host||!Thread()||!CurrentRoster()||!local_server_names::GetState().ready)return false;
    const auto& roster=local_server_names::roster;local_server_names::Ack ack{};
    if(roster.publisher!=GetCurrentProcessId()
       ||!local_server_names::ReadFileExact(local_server_names::ackPath,&ack,sizeof(ack))
       ||ack.magic!=local_server_names::Magic||ack.digest!=local_server_names::GetState().digest
       ||ack.generation!=roster.generation||ack.count!=roster.count||ack.publisher!=roster.publisher
       ||!ack.recipient||ack.recipient==ack.publisher)return false;
    uint64_t started=0;if(!ControlHeader(training_control::memory,ack.publisher,started))return false;
    state.binding={state.pairDigest,ack.digest,started,ack.publisher,ack.recipient,ack.generation,ack.count};
    auto output=reinterpret_cast<wire::Wire*>(training_control::memory+Offset);
    std::memset(output,0,sizeof(*output));output->header.version=wire::Version;output->header.bytes=wire::WireBytes;
    const auto& b=state.binding;auto& h=output->header;
    h.pairDigest=b.pairDigest;h.rosterDigest=b.rosterDigest;h.hostStarted=b.hostStarted;
    h.hostPid=b.hostPid;h.clientPid=b.clientPid;h.rosterGeneration=b.rosterGeneration;h.rosterCount=b.rosterCount;
    MemoryBarrier();h.magic=wire::Magic;state.bound=true;state.stage="feedback_ledger_host_bound_to_original_name_ack";return true;
}
inline void ClosePeer(){
    if(peerMemory)UnmapViewOfFile(peerMemory);peerMemory=nullptr;
    if(peerMapping)CloseHandle(peerMapping);peerMapping=nullptr;state.bound=false;
}
inline bool BindClient(){
    if(state.host||!Thread()||!CurrentRoster()||!local_server_names::GetState().ready)return false;
    const auto hostPid=local_server_names::roster.publisher;
    if(state.bound&&state.binding.hostPid==hostPid&&state.binding.clientPid==GetCurrentProcessId())return true;
    ClosePeer();const auto now=GetTickCount64();if(now<state.nextOpenAt)return false;state.nextOpenAt=now+100;
    wchar_t name[100]{};swprintf_s(name,L"Local\\ValorantTrainerControlV2_%u",hostPid);++state.openAttempts;
    peerMapping=OpenFileMappingW(FILE_MAP_READ,FALSE,name);if(!peerMapping)return false;
    peerMemory=static_cast<const uint8_t*>(MapViewOfFile(peerMapping,FILE_MAP_READ,0,0,training_control::Bytes));
    if(!peerMemory){ClosePeer();return false;}uint64_t started=0;
    if(!ControlHeader(peerMemory,hostPid,started)){ClosePeer();return false;}
    const auto source=reinterpret_cast<const wire::Header*>(peerMemory+Offset);wire::Header h{};
    const auto before=source->sequence;if(before&1){ClosePeer();return false;}
    MemoryBarrier();std::memcpy(&h,source,sizeof(h));MemoryBarrier();
    if(before!=source->sequence||h.magic!=wire::Magic||h.version!=wire::Version||h.bytes!=wire::WireBytes
       ||h.pairDigest!=state.pairDigest||h.hostPid!=hostPid||h.clientPid!=GetCurrentProcessId()||h.hostStarted!=started
       ||!h.rosterGeneration||h.rosterGeneration>local_server_names::roster.generation||!h.rosterDigest
       ||h.rosterCount<1||h.rosterCount>local_server_names::roster.count){ClosePeer();return false;}
    // This is the initial real ACK receipt, pinned for this pair. Later native
    // roster generations may change at respawn without changing the pair.
    state.binding={h.pairDigest,h.rosterDigest,h.hostStarted,h.hostPid,h.clientPid,h.rosterGeneration,h.rosterCount};
    state.bound=true;state.stage="feedback_ledger_client_readonly_pair_mapping_bound";return true;
}
inline bool PollHost(){
    if(!Enabled()||!state.host||!Thread())return true;
    if(!state.gameThread)state.gameThread=GetCurrentThreadId();if(!BindHost())return true;
    auto output=reinterpret_cast<wire::Wire*>(training_control::memory+Offset);auto& h=output->header;
    if(h.commitSequence==state.nextCommit)return true;
    if(wire::ValidateHeader(h,state.binding)!=wire::Lookup::Found){++state.rejected;state.stage="feedback_ledger_owned_mapping_header_changed";return false;}
    InterlockedIncrement(reinterpret_cast<volatile LONG*>(&h.sequence));
    std::memcpy(output->records.data(),hostRecords.data(),sizeof(hostRecords));
    h.commitSequence=state.nextCommit;h.writeIndex=static_cast<uint32_t>(state.nextCommit%wire::Capacity);
    h.lastCommittedAt=state.nextCommit?hostRecords[static_cast<size_t>((state.nextCommit-1)%wire::Capacity)].committedAtMs:0;
    MemoryBarrier();InterlockedIncrement(reinterpret_cast<volatile LONG*>(&h.sequence));
    state.published=state.nextCommit;state.stage="feedback_ledger_real_committed_display_records_published";return true;
}
inline bool Commit(wire::Record record){
    if(!Enabled()||!state.host||!Thread())return false;
    if(!state.gameThread)state.gameThread=GetCurrentThreadId();
    for(const auto& old:hostRecords)if(old.sequence&&old.damageToken==record.damageToken){++state.duplicates;return true;}
    record.sequence=state.nextCommit+1;record.committedAtMs=GetTickCount64();record.checksum=wire::Checksum(record);
    if(!wire::ValidRecord(record)||!record.sequence){++state.invalidRecords;state.stage="feedback_ledger_invalid_committed_presentation_record";return false;}
    hostRecords[static_cast<size_t>(state.nextCommit%wire::Capacity)]=record;++state.nextCommit;
    // A native ACK can lag a genuine commit. Keep the bounded committed ring
    // locally until the existing pair mapping can be bound; never invent data.
    PollHost();return true;
}
inline wire::Lookup Lookup(const wire::Key& key,wire::Record& record){
    if(Enabled()&&!state.gameThread)state.gameThread=GetCurrentThreadId();
    ++state.lookups;if(!Enabled()||state.host||!Thread()||!BindClient()){++state.missing;return wire::Lookup::Missing;}
    const auto source=reinterpret_cast<const wire::Wire*>(peerMemory+Offset);wire::Wire copy{};
    for(unsigned attempt=0;attempt<3;++attempt){
        const auto before=source->header.sequence;if(before&1)continue;
        MemoryBarrier();std::memcpy(&copy,source,sizeof(copy));MemoryBarrier();
        if(before!=source->header.sequence||copy.header.sequence!=before)continue;
        uint64_t started=0;
        if(!CurrentRoster()||local_server_names::roster.publisher!=state.binding.hostPid
           ||!ControlHeader(peerMemory,state.binding.hostPid,started)||started!=state.binding.hostStarted){
            ++state.rejected;ClosePeer();return wire::Lookup::WrongSession;
        }
        const auto found=wire::Find(copy,state.binding,key,GetTickCount64(),record);
        if(found==wire::Lookup::Found||found==wire::Lookup::Missing||found==wire::Lookup::Expired)state.published=copy.header.commitSequence;
        if(found==wire::Lookup::Found)++state.matched;
        else if(found==wire::Lookup::Expired)++state.expired;
        else if(found==wire::Lookup::Missing)++state.missing;
        else ++state.rejected;
        return found;
    }
    ++state.incomplete;return wire::Lookup::Incomplete;
}
inline void Close(){ClosePeer();state.stage="feedback_ledger_reader_closed";}
inline void RejectCommittedRecord(const char* stage){++state.invalidRecords;state.stage=stage;}
} // namespace local_server_feedback_ledger
