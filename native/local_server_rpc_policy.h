#pragma once
#include "local_server_combat.h"
#include "local_server_names.h"
#include "local_server_owned_paths.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>

// Local native receive-policy observations only. This sidecar carries no
// pointers, hit/health/score claims or gameplay commands. The original native
// NameRoster ACK supplies host/client PID + current roster binding. Parent
// owns the actual child-process receipt and unique pair-directory allocation.
// No CVar, RPC payload, role, controller replication or native actor is changed.
namespace local_server_rpc_policy {
inline constexpr uint64_t Magic=0x3150435250545656ULL;
inline constexpr uint32_t Version=1,WireBytes=128,SampleMilliseconds=500,ReadMilliseconds=100;
inline constexpr uint64_t MaximumAgeMilliseconds=2500;
enum WireFlags:uint32_t {
    ProfilesVerified=1,ValueKnown=2,FunctionKnown=4,ClientWorldObserved=8,
    DelayUnmapped=16,MulticastReliable=32,GameThreadObserved=64
};
inline constexpr uint32_t AllFlags=127,RequiredFlags=ProfilesVerified|ValueKnown|FunctionKnown|ClientWorldObserved|GameThreadObserved;
struct Wire {
    uint64_t magic=Magic;
    uint32_t version=Version,bytes=WireBytes;
    uint32_t hostPid=0,clientPid=0,port=0,generation=0,count=0,flags=0;
    uint64_t pairDigest=0,rosterDigest=0,sampledAt=0,sequence=0,profileDigest=0;
    int32_t delayUnmappedValue=0;uint32_t multicastFlags=0;
    uint64_t checksum=0,reserved[4]{};
};
static_assert(sizeof(Wire)==WireBytes&&offsetof(Wire,hostPid)==16&&offsetof(Wire,pairDigest)==40);
static_assert(offsetof(Wire,sampledAt)==56&&offsetof(Wire,sequence)==64&&offsetof(Wire,checksum)==88&&offsetof(Wire,reserved)==96);
static_assert(sizeof(local_server_names::Ack)==32&&sizeof(wchar_t)==2&&sizeof(local_server_combat::NativeProfile)==16);
struct State {
    bool initialized=false,host=false,portKnown=false,gameThreadObserved=false;
    bool namesBound=false,worldBound=false,profileVerified=false,metadataBound=false;
    bool published=false,observationApplied=false,nativePermitsNullable=false,ready=false;
    bool valueKnown=false,functionKnown=false,clientWorldObserved=false;
    unsigned error=0,lastReject=0,lastWin32Error=0;
    uint32_t processId=0,hostPid=0,clientPid=0,port=0,generation=0,count=0,multicastFlags=0;
    DWORD gameThread=0;int32_t delayUnmappedValue=0;
    uint64_t pairDigest=0,rosterDigest=0,profileDigest=0,sequence=0,sampledAt=0;
    uint64_t polls=0,reads=0,publications=0,applied=0,rejections=0,revocations=0;
    uint64_t lastPoll=0,lastAttempt=0,lastAcceptedChecksum=0;
    const char* stage="native_rpc_policy_bridge_not_initialized";
    const char* nativeSampleStage="native_rpc_policy_not_sampled";
};
inline State state;
inline std::wstring roleDirectory,pairDirectory,policyPath;
inline SRWLOCK snapshotLock=SRWLOCK_INIT;
inline const State& GetState(){return state;} // game-thread caller; WriteState takes a locked copy
inline State Snapshot(){AcquireSRWLockShared(&snapshotLock);const auto value=state;ReleaseSRWLockShared(&snapshotLock);return value;}
template<class T> inline T Read(const void* object,size_t offset){return original_players::Value<T>(object,offset);}
inline uint64_t Hash(const void* input,size_t bytes){
    const auto data=static_cast<const uint8_t*>(input);uint64_t value=14695981039346656037ULL;
    for(size_t n=0;n<bytes;++n)value=(value^data[n])*1099511628211ULL;return value;
}
inline uint64_t Checksum(const Wire& value){auto copy=value;copy.checksum=0;return Hash(&copy,sizeof(copy));}
inline uint64_t ProfileDigest(){return local_server_rpc_policy::Hash(local_server_combat::Profiles,sizeof(local_server_combat::Profiles));}
inline bool AbsoluteLocal(const std::wstring& input,std::wstring& output){
    return local_server_owned_paths::CanonicalAbsolute(input,output);
}
inline uint64_t DirectoryDigest(const std::wstring& directory){
    auto canonical=directory;
    for(auto& c:canonical)if(c>=L'A'&&c<=L'Z')c=static_cast<wchar_t>(c-L'A'+L'a');
    return Hash(canonical.data(),canonical.size()*sizeof(wchar_t));
}
inline bool SameFile(const std::wstring& left,const std::wstring& right){
    std::wstring a,b;return AbsoluteLocal(left,a)&&AbsoluteLocal(right,b)&&_wcsicmp(a.c_str(),b.c_str())==0;
}
inline void Revoke(){
    if(state.host&&state.observationApplied){
        auto& combat=local_server_combat::state;
        if(combat.replicaPolicyVerified&&combat.replicaProcessId==state.clientPid){
            // Revoke this bridge's own trust receipt, not the native policy.
            // The combat header stays frozen; no actor/CVar field is written.
            combat.replicaPolicyVerified=false;++state.revocations;
        }
    }
    state.observationApplied=false;state.metadataBound=false;state.ready=false;
}
inline bool Wait(unsigned reason,const char* stage){
    Revoke();state.lastReject=reason;state.stage=stage;return true;
}
inline bool Reject(unsigned reason,const char* stage){++state.rejections;return Wait(reason,stage);}
inline bool ConfigurePortImpl(uint32_t port){
    if(port<1024||port>65535){state.error=4901;state.stage="native_rpc_policy_bridge_port_invalid";return false;}
    if(state.portKnown&&state.port!=port){state.error=4902;state.stage="native_rpc_policy_bridge_port_changed_requires_new_pair";Revoke();return false;}
    state.port=port;state.portKnown=true;return true;
}
inline bool InitializeImpl(const std::wstring& output,bool host,uint32_t port){
    std::wstring canonical;
    if(!AbsoluteLocal(output,canonical)||!local_server_owned_paths::RoleDirectory(canonical,host?1U:2U)){
        state.error=4903;state.stage="native_rpc_policy_bridge_requires_owned_absolute_role_directory";return false;}
    const auto at=canonical.find_last_of(L'\\');
    if(at==std::wstring::npos||_wcsicmp(canonical.c_str()+at+1,host?L"host":L"client")!=0){
        state.error=4904;state.stage="native_rpc_policy_bridge_role_directory_mismatch";return false;
    }
    const auto parent=canonical.substr(0,at);
    if(parent.size()<=3){state.error=4905;state.stage="native_rpc_policy_bridge_pair_directory_invalid";return false;}
    if(state.initialized){
        if(state.host!=host||_wcsicmp(roleDirectory.c_str(),canonical.c_str())!=0){state.error=4906;state.stage="native_rpc_policy_bridge_pair_changed";Revoke();return false;}
        return port?ConfigurePortImpl(port):true;
    }
    roleDirectory=canonical;pairDirectory=parent;policyPath=parent+L"\\native-rpc-policy.bin";
    state.host=host;state.processId=GetCurrentProcessId();state.hostPid=host?state.processId:0;
    state.pairDigest=DirectoryDigest(parent);state.profileDigest=ProfileDigest();state.initialized=true;
    if(port&&!ConfigurePortImpl(port))return false;
    state.stage=state.portKnown?"native_rpc_policy_bridge_waiting_name_ack":"native_rpc_policy_bridge_waiting_owned_config_port";return true;
}
inline bool Initialize(const std::wstring& output,bool host,uint32_t port){
    AcquireSRWLockExclusive(&snapshotLock);const bool okay=InitializeImpl(output,host,port);ReleaseSRWLockExclusive(&snapshotLock);return okay;
}
inline bool Initialize(const std::wstring& output,bool host){return Initialize(output,host,0);}
inline bool ConfigurePort(uint32_t port){
    AcquireSRWLockExclusive(&snapshotLock);const bool okay=ConfigurePortImpl(port);ReleaseSRWLockExclusive(&snapshotLock);return okay;
}
inline bool AckBinding(local_server_names::Ack& ack){
    const auto& names=local_server_names::GetState();const auto& roster=local_server_names::roster;
    state.namesBound=false;
    if(!names.initialized||names.host!=state.host||names.error||!names.ready||!roster.generation||!roster.count
       ||roster.count>local_server_names::Capacity||names.appliedGeneration!=roster.generation
       ||names.digest!=local_server_names::Digest(roster))return false;
    if(!SameFile(local_server_names::path,pairDirectory+L"\\native-roster.bin")
       ||!SameFile(local_server_names::ackPath,pairDirectory+L"\\native-roster-ack.bin"))return false;
    if(!local_server_names::ReadFileExact(local_server_names::ackPath,&ack,sizeof(ack)))return false;
    if(ack.magic!=local_server_names::Magic||ack.digest!=names.digest||ack.generation!=roster.generation
       ||ack.count!=roster.count||ack.publisher!=roster.publisher||!ack.publisher||!ack.recipient||ack.publisher==ack.recipient)return false;
    if(state.host?ack.publisher!=state.processId:ack.recipient!=state.processId)return false;
    state.namesBound=true;return true;
}
inline bool WriteAtomic(const Wire& value){
    const auto temporary=policyPath+L".tmp."+std::to_wstring(state.processId);
    const auto handle=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(handle==INVALID_HANDLE_VALUE){state.lastWin32Error=GetLastError();return false;}
    DWORD written=0;const bool exact=WriteFile(handle,&value,sizeof(value),&written,nullptr)&&written==sizeof(value)&&FlushFileBuffers(handle);
    if(!exact)state.lastWin32Error=GetLastError();CloseHandle(handle);
    if(!exact)return false;
    if(!MoveFileExW(temporary.c_str(),policyPath.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){
        state.lastWin32Error=GetLastError();return false;
    }return true;
}
inline bool WorldBinding(void* world,void* connection){
    const auto driver=Read<void*>(world,0x40);
    if(!world||!driver||Read<void*>(driver,0x178)!=world)return false;
    const auto server=Read<void*>(driver,0xc0);
    if(!state.host)return server&&Read<void*>(server,0x60)==driver&&(!connection||connection==server);
    if(server||!connection||Read<void*>(connection,0x60)!=driver)return false;
    const auto peers=Read<void**>(driver,0xc8);const auto count=Read<int32_t>(driver,0xd0);
    if(count!=1||!peers||Read<void*>(peers,0)!=connection)return false;
    const auto pc=Read<void*>(connection,0xa0);
    return pc&&Read<void*>(pc,0x510)==connection&&Read<void*>(connection,0x38)==pc&&Read<uint8_t>(pc,0x224)==3;
}
inline bool Publish(void* world,const local_server_names::Ack& ack){
    local_server_combat::ReceivePolicy observed;
    const bool sampled=local_server_combat::ReadRPCPolicy(world,observed);
    state.profileVerified=observed.profilesVerified;state.valueKnown=observed.valueKnown;
    state.functionKnown=observed.functionKnown;state.clientWorldObserved=observed.clientWorldObserved;
    state.nativeSampleStage=observed.stage;state.delayUnmappedValue=observed.delayUnmappedValue;state.multicastFlags=observed.multicastFlags;
    if(!sampled||!observed.profilesVerified||!observed.valueKnown
       ||!observed.functionKnown||!observed.clientWorldObserved||observed.processId!=state.processId)
        return Wait(4910,"native_rpc_policy_bridge_waiting_full_native_client_observation");
    local_server_names::Ack current;
    if(!AckBinding(current)||std::memcmp(&ack,&current,sizeof(ack))!=0)return Wait(4911,"native_rpc_policy_bridge_ack_changed_during_sample");
    Wire value;value.hostPid=ack.publisher;value.clientPid=ack.recipient;value.port=state.port;
    value.generation=ack.generation;value.count=ack.count;value.pairDigest=state.pairDigest;value.rosterDigest=ack.digest;
    value.sampledAt=observed.sampledAt;value.sequence=state.sequence+1;value.profileDigest=state.profileDigest;
    value.flags=RequiredFlags|(observed.delayUnmapped?DelayUnmapped:0U)|(observed.multicastReliable?MulticastReliable:0U);
    value.delayUnmappedValue=observed.delayUnmappedValue;value.multicastFlags=observed.multicastFlags;value.checksum=Checksum(value);
    if(!value.sequence)return Reject(4912,"native_rpc_policy_bridge_sequence_exhausted");
    if(!WriteAtomic(value))return Wait(4913,"native_rpc_policy_bridge_atomic_publish_failed");
    state.hostPid=value.hostPid;state.clientPid=value.clientPid;state.generation=value.generation;state.count=value.count;
    state.rosterDigest=value.rosterDigest;state.sampledAt=value.sampledAt;state.sequence=value.sequence;
    state.multicastFlags=value.multicastFlags;state.delayUnmappedValue=value.delayUnmappedValue;state.lastAcceptedChecksum=value.checksum;
    state.profileVerified=true;state.metadataBound=true;state.published=true;++state.publications;state.lastReject=0;
    state.nativePermitsNullable=!observed.multicastReliable||!observed.delayUnmapped;state.ready=state.nativePermitsNullable;
    state.stage=state.ready?"native_rpc_policy_bridge_client_native_observation_published":"native_rpc_policy_bridge_client_native_delay_enabled";return true;
}
inline bool Consume(void* connection,const local_server_names::Ack& ack){
    Wire value;++state.reads;
    if(!local_server_names::ReadFileExact(policyPath,&value,sizeof(value)))return Wait(4914,"native_rpc_policy_bridge_waiting_client_record");
    if(value.magic!=Magic||value.version!=Version||value.bytes!=sizeof(Wire)||(value.flags&~AllFlags)
       ||(value.flags&RequiredFlags)!=RequiredFlags||value.checksum!=Checksum(value))return Reject(4915,"native_rpc_policy_bridge_record_schema_or_checksum_rejected");
    for(const auto reserved:value.reserved)if(reserved)return Reject(4916,"native_rpc_policy_bridge_nonzero_reserved_field");
    if(value.hostPid!=state.processId||value.hostPid!=ack.publisher||value.clientPid!=ack.recipient
       ||value.clientPid==state.processId||value.port!=state.port||value.pairDigest!=state.pairDigest
       ||value.generation!=ack.generation||value.count!=ack.count||value.rosterDigest!=ack.digest
       ||value.profileDigest!=state.profileDigest||!value.sequence)return Reject(4917,"native_rpc_policy_bridge_record_ack_pair_or_port_mismatch");
    if(((value.flags&DelayUnmapped)!=0)!=(value.delayUnmappedValue>0)
       ||((value.flags&MulticastReliable)!=0)!=((value.multicastFlags&0x80)!=0)
       ||(value.multicastFlags&0x4440)!=0x4440)return Reject(4918,"native_rpc_policy_bridge_native_fields_inconsistent");
    const auto now=GetTickCount64();
    if(value.sampledAt>now||now-value.sampledAt>MaximumAgeMilliseconds)return Reject(4919,"native_rpc_policy_bridge_client_observation_expired");
    const bool sameBinding=state.clientPid==value.clientPid&&state.generation==value.generation&&state.rosterDigest==value.rosterDigest;
    if(sameBinding&&(value.sequence<state.sequence||(value.sequence==state.sequence&&value.checksum!=state.lastAcceptedChecksum)))
        return Reject(4920,"native_rpc_policy_bridge_observation_replay_or_sequence_rewrite");
    local_server_names::Ack current;
    if(!AckBinding(current)||std::memcmp(&ack,&current,sizeof(ack))!=0)return Wait(4921,"native_rpc_policy_bridge_ack_changed_during_read");
    local_server_combat::ReceivePolicy observed;
    observed.profilesVerified=true;observed.valueKnown=true;observed.functionKnown=true;observed.clientWorldObserved=true;
    observed.delayUnmapped=(value.flags&DelayUnmapped)!=0;observed.multicastReliable=(value.flags&MulticastReliable)!=0;
    observed.processId=value.clientPid;observed.multicastFlags=value.multicastFlags;
    observed.delayUnmappedValue=value.delayUnmappedValue;observed.sampledAt=value.sampledAt;
    observed.stage="paired_native_client_rpc_policy_ack_bound";
    if(!local_server_combat::ObserveReplicaRPCPolicy(connection,value.clientPid,observed))
        return Wait(4922,"native_rpc_policy_bridge_waiting_authority_combat_receipt_context");
    state.hostPid=value.hostPid;state.clientPid=value.clientPid;state.generation=value.generation;state.count=value.count;
    state.rosterDigest=value.rosterDigest;state.sampledAt=value.sampledAt;state.sequence=value.sequence;state.lastAcceptedChecksum=value.checksum;
    state.multicastFlags=value.multicastFlags;state.delayUnmappedValue=value.delayUnmappedValue;
    state.profileVerified=true;state.metadataBound=true;state.observationApplied=true;++state.applied;state.lastReject=0;
    state.valueKnown=true;state.functionKnown=true;state.clientWorldObserved=true;state.nativeSampleStage=observed.stage;
    state.nativePermitsNullable=!observed.multicastReliable||!observed.delayUnmapped;
    state.ready=state.nativePermitsNullable&&local_server_combat::ReplicaPolicyAllowsNullableController();
    state.stage=state.ready?"native_rpc_policy_bridge_verified_native_client_policy_applied":"native_rpc_policy_bridge_native_receiver_requires_mapped_controller";return true;
}
inline bool PollImpl(void* world,void* connection){
    ++state.polls;state.lastPoll=GetTickCount64();
    if(!state.initialized||state.error)return false;
    if(!state.portKnown)return Wait(4923,"native_rpc_policy_bridge_waiting_owned_config_port");
    if(!engine::image||!original_players::gameThread)return Wait(4924,"native_rpc_policy_bridge_waiting_native_game_thread_binding");
    if(GetCurrentThreadId()!=original_players::gameThread){state.error=4925;Reject(4925,"native_rpc_policy_bridge_poll_must_run_on_game_thread");return false;}
    state.gameThread=GetCurrentThreadId();state.gameThreadObserved=true;
    state.worldBound=WorldBinding(world,connection);
    if(!state.worldBound)return Wait(4926,"native_rpc_policy_bridge_waiting_actual_native_world_connection");
    local_server_names::Ack ack;
    if(!AckBinding(ack))return Wait(4927,"native_rpc_policy_bridge_waiting_verified_name_roster_ack");
    const auto now=GetTickCount64();
    if(state.sampledAt&&(now<state.sampledAt||now-state.sampledAt>MaximumAgeMilliseconds))Revoke();
    if(state.metadataBound&&(state.hostPid!=ack.publisher||state.clientPid!=ack.recipient
       ||state.generation!=ack.generation||state.rosterDigest!=ack.digest))Revoke();
    const auto interval=state.host?ReadMilliseconds:SampleMilliseconds;
    if(state.lastAttempt&&now-state.lastAttempt<interval)return true;state.lastAttempt=now;
    return state.host?Consume(connection,ack):Publish(world,ack);
}
inline bool Poll(void* world,void* connection=nullptr){
    AcquireSRWLockExclusive(&snapshotLock);const bool okay=PollImpl(world,connection);ReleaseSRWLockExclusive(&snapshotLock);return okay;
}
inline void WriteState(FILE* file){
    if(!file)return;const auto value=Snapshot();const auto now=GetTickCount64();
    const bool fresh=value.sampledAt&&now>=value.sampledAt&&now-value.sampledAt<=MaximumAgeMilliseconds;
    const auto b=[](bool flag){return flag?"true":"false";};
    std::fprintf(file,"{\"schema_version\":1,\"stage\":\"%s\",\"error\":%u,\"last_reject\":%u,\"last_win32_error\":%u,\"initialized\":%s,\"host\":%s,\"port_known\":%s,\"game_thread_observed\":%s,\"names_bound\":%s,\"native_world_bound\":%s,\"profile_verified\":%s,\"metadata_bound\":%s,\"observation_applied\":%s,\"fresh\":%s,\"ready\":%s,\"native_permits_nullable_controller\":%s,\"process_id\":%u,\"host_pid\":%u,\"client_pid\":%u,\"port\":%u,\"generation\":%u,\"count\":%u,\"pair_digest\":%llu,\"roster_digest\":%llu,\"profile_digest\":%llu,\"sampled_at_ms\":%llu,\"sequence\":%llu,\"delay_unmapped_value\":%d,\"multicast_flags\":%u,\"polls\":%llu,\"reads\":%llu,\"publications\":%llu,\"applications\":%llu,\"rejections\":%llu,\"revocations\":%llu,\"wire_bytes\":128,\"raw_pointers_serialized\":false,\"cvar_writes\":false,\"payload_patches\":false,\"runtime_verified\":false",
        value.stage,value.error,value.lastReject,value.lastWin32Error,b(value.initialized),b(value.host),b(value.portKnown),b(value.gameThreadObserved),
        b(value.namesBound),b(value.worldBound),b(value.profileVerified),b(value.metadataBound),b(value.observationApplied),b(fresh),b(value.ready&&fresh),b(value.nativePermitsNullable&&fresh),
        value.processId,value.hostPid,value.clientPid,value.port,value.generation,value.count,
        static_cast<unsigned long long>(value.pairDigest),static_cast<unsigned long long>(value.rosterDigest),static_cast<unsigned long long>(value.profileDigest),
        static_cast<unsigned long long>(value.sampledAt),static_cast<unsigned long long>(value.sequence),value.delayUnmappedValue,value.multicastFlags,
        static_cast<unsigned long long>(value.polls),static_cast<unsigned long long>(value.reads),static_cast<unsigned long long>(value.publications),
        static_cast<unsigned long long>(value.applied),static_cast<unsigned long long>(value.rejections),static_cast<unsigned long long>(value.revocations));
    std::fprintf(file,",\"value_known\":%s,\"function_known\":%s,\"client_world_observed\":%s,\"native_sample_stage\":\"%s\"}",
        b(value.valueKnown),b(value.functionKnown),b(value.clientWorldObserved),value.nativeSampleStage);
}
inline bool WriteState(const wchar_t* path){
    if(!path)return false;FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;
    WriteState(file);std::fputc('\n',file);const bool okay=std::ferror(file)==0;std::fclose(file);return okay;
}
} // namespace local_server_rpc_policy
