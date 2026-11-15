#pragma once
#include "original_inventory_slots.h"
#include "local_server_owned_paths.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>

// Optional, connection-scoped local credential adapter. Default: disabled.
// The project issuer supplies a new canonical PlayerKey for its own local pair;
// original cipher/tag/Stateless validation remains unchanged. No official DTO,
// Subject, account, session-ready, Actor Role, or connection-ready field is set.
// See work/local-server-playable/ares-transport-recovery/local-key-adapter.
//
// Client installation uses the original pre-handshake native delegate. The
// accepted-server callback currently records its shape and refuses installation:
// the remaining zero-cipher-I/O first-use proof is explicitly not fabricated.
namespace local_server_ares_local_key {
using Weak=original_inventory_slots::WeakIdentity;
inline constexpr uint64_t MarkerMagic=0x3159454b53455241ULL; // ARESKEY1
inline constexpr uint32_t MarkerVersion=1,MarkerBytes=128,ExplicitFlag=1;
inline constexpr uintptr_t EventRva=0xd71ca98,CipherTableRva=0xa8c8f98;
inline constexpr uintptr_t StatelessTableRva=0xaba31d0,DDoSTableRva=0xa8c9290;
inline constexpr uint8_t ImageSHA[32]{
    0x14,0x77,0x63,0x35,0x05,0x04,0xd2,0x41,0xee,0x20,0xb7,0x76,0x07,0xd0,0x4b,0xc8,
    0x5c,0xeb,0xfe,0xdb,0x1d,0x8b,0xc2,0xf3,0x24,0x73,0xca,0x93,0xbf,0x8f,0xe9,0x2f};
struct Marker {
    uint64_t magic=0;
    uint32_t version=0,bytes=0,role=0,port=0;
    uint8_t proofId[16]{};
    uint64_t nonce=0;
    char base64[44]{};
    uint32_t flags=0;
    uint8_t imageSHA[32]{};
};
static_assert(sizeof(Marker)==128&&offsetof(Marker,proofId)==24&&offsetof(Marker,nonce)==40);
static_assert(offsetof(Marker,base64)==48&&offsetof(Marker,flags)==92&&offsetof(Marker,imageSHA)==96);
struct NativeString {wchar_t* data=nullptr;int32_t count=0,capacity=0;};
struct NativeDelegate {void* instance=nullptr;int32_t units=0,padding=0;};
struct DelegateArray {NativeDelegate* data=nullptr;int32_t count=0,capacity=0;};
struct Shared {void* object=nullptr;void* control=nullptr;};
struct SharedArray {Shared* data=nullptr;int32_t count=0,capacity=0;};
static_assert(sizeof(NativeString)==16&&sizeof(NativeDelegate)==16);
static_assert(sizeof(DelegateArray)==16&&sizeof(Shared)==16&&sizeof(SharedArray)==16);
struct Shape {
    void *pending=nullptr,*driver=nullptr,*connection=nullptr,*handler=nullptr;
    void *cipher=nullptr,*stateless=nullptr,*ddos=nullptr;
    Weak pendingId{},driverId{},connectionId{};
    int32_t components=0,componentCapacity=0,handlerState=-1,connectionState=-1;
    uint8_t mode=255,connectionless=255,handshaking=255,statelessInitialized=255;
    bool actualClasses=false,originalVirtuals=false,links=false,owners=false,sharedOwners=false,exactChain=false;
    bool driverLink=false,ddosWeakLink=false,active=false,initialization=false;
    bool loopbackURL=false,portMatches=false,delegateTail=false,callbackFrame=false;
};
struct State {
    bool initialized=false,configured=false,cliMatched=false,profilesVerified=false;
    bool armed=false,applied=false,closed=false,restartRequired=false,keyLoaded=false,keyErased=false;
    bool delegateAdded=false,delegateRemoved=false,clientFirstUseShape=false;
    bool installerReturned=false,contextsPresent=false,contextsDistinct=false;
    bool outgoingContextChanged=false,incomingContextChanged=false;
    uint32_t role=0,port=0,error=0,expectedThread=0,actualThread=0,lastWin32Error=0;
    uint64_t callbacks=0,unrelatedCallbacks=0,armCalls=0,installCalls=0,reinstallRejections=0;
    uint64_t serverCallbacks=0,firstUseRejections=0,delegateHandle=0;
    uintptr_t profileFailureRva=0,exceptionRva=0;
    const char* stage="ares_local_key_disabled";
    const char* operation="none";
    Shape shape{};
};
inline State state,cached;
inline SRWLOCK cacheLock=SRWLOCK_INIT;
// This storage is intentionally separate from State and its diagnostics copy.
inline std::array<wchar_t,45> privateKey{};
inline void* delegateInstance=nullptr;
inline void *pendingClass=nullptr,*driverClass=nullptr,*connectionClass=nullptr;
inline Weak pendingClassId{},driverClassId{},connectionClassId{};
using CaptureContext=void(WINAPI*)(PCONTEXT);
using LookupFunction=PRUNTIME_FUNCTION(WINAPI*)(DWORD64,PDWORD64,PUNWIND_HISTORY_TABLE);
using Unwind=PEXCEPTION_ROUTINE(WINAPI*)(DWORD,DWORD64,DWORD64,PRUNTIME_FUNCTION,PCONTEXT,PVOID*,PDWORD64,PKNONVOLATILE_CONTEXT_POINTERS);
inline CaptureContext captureContext=nullptr;
inline LookupFunction lookupFunction=nullptr;
inline Unwind virtualUnwind=nullptr;

struct Profile {uintptr_t rva;size_t bytes;uint64_t fnv;};
// Generated from the retained pinned image; each native range is contiguous.
// Code digests identify instructions only; no credential/material digest exists.
inline constexpr Profile Profiles[]{
    {0x29f1630,536,0xb0fe1d5b232af1a2ULL},
    {0x3f4cb70,151,0x85eb4345553b9476ULL},
    {0x18d1ad0,208,0xa6ae36b3be6e3676ULL},
    {0x18cd890,299,0xb04bfb2d097eb9dbULL},
    {0x18be670,24,0x74c08f5a461d4ccdULL},
    {0x19d0320,44,0xaa94462619c48c39ULL},
    {0x1d07dd0,1669,0x3adabc6786ee6b60ULL},
    {0x1cf35b0,1571,0x40614eae57888396ULL},
    {0x1d4cfa0,1562,0xb050d7efb4cf1a25ULL},
    {0x4857e60,6036,0x482d6db2b02d5d91ULL},
    {0x5118500,578,0xe4a64a55b291f841ULL},
    {0x51132f0,492,0xcd95238df125f2d1ULL},
    {0x51134e0,733,0x795fd6068b7bca42ULL},
    {0x716db30,2324,0x37258b0b5ef21ebfULL},
    {0x29ee9c0,155,0x868aa3d69601fdbcULL},
    {0x29eea60,183,0xd5e9db30e2d1417bULL},
    {0x29f0a70,35,0x0a3e369fb83bd95bULL},
    {0x29f0a93,681,0x35478038382070feULL},
    {0x29f0d3c,7,0xb1ca6a9e28a17e05ULL},
    {0x29efff0,1332,0x512634af7cc74603ULL},
    {0x4788fd0,1827,0x02e97e8e2eaa7798ULL},
    {0x478aed0,26,0xc3887378cb775c76ULL},
};

template<class T> inline T Read(const void* object,size_t offset=0){return original_players::Value<T>(object,offset);}
inline void Cache(){AcquireSRWLockExclusive(&cacheLock);cached=state;ReleaseSRWLockExclusive(&cacheLock);}
inline State Snapshot(){AcquireSRWLockShared(&cacheLock);const auto result=cached;ReleaseSRWLockShared(&cacheLock);return result;}
inline const State& GetState(){return state;} // original game thread only
inline void EraseKey(){SecureZeroMemory(privateKey.data(),sizeof(privateKey));state.keyLoaded=false;state.keyErased=true;}
inline bool Fail(uint32_t error,const char* stage){state.error=error;state.stage=stage;EraseKey();Cache();return false;}
inline bool Live(void* object,const Weak& identity){return object&&original_inventory_slots::ResolveIdentity(identity)==object;}
inline bool Identity(void* object,Weak& identity){return original_inventory_slots::CaptureIdentity(object,identity)&&Live(object,identity);}
inline bool Thread(bool cleanup=false){
    state.actualThread=GetCurrentThreadId();
    return engine::image&&(cleanup||!state.restartRequired)&&original_players::gameThread
        &&state.actualThread==original_players::gameThread&&(!state.expectedThread||state.actualThread==state.expectedThread);
}
inline bool VerifyProfiles(){
    if(state.profilesVerified)return true;
    if(!engine::image)return Fail(6301,"ares_local_key_original_image_missing");
    for(const auto& p:Profiles){
        const auto bytes=reinterpret_cast<const uint8_t*>(engine::image+p.rva);
        if(!original_players::Readable(bytes,p.bytes)){state.profileFailureRva=p.rva;return Fail(6302,"ares_local_key_native_profile_unreadable");}
        uint64_t value=14695981039346656037ULL;
        for(size_t i=0;i<p.bytes;++i)value=(value^bytes[i])*1099511628211ULL;
        if(value!=p.fnv){state.profileFailureRva=p.rva;return Fail(6303,"ares_local_key_native_profile_changed");}
    }
    state.profilesVerified=true;return true;
}
inline bool Token(const wchar_t* command,const wchar_t* token){
    if(!command||!token)return false;size_t total=0;
    while(total<32768&&command[total])++total;if(total==32768)return false;
    const auto count=std::wcslen(token);
    for(size_t i=0;i<total;++i){
        if(i&&command[i-1]!=L' '&&command[i-1]!=L'\t'&&command[i-1]!=L'\n')continue;
        if(count>total-i||std::wcsncmp(command+i,token,count))continue;
        const auto after=command[i+count];if(!after||after==L' '||after==L'\t'||after==L'\n')return true;
    }return false;
}
inline bool ExplicitCommandLine(){
    if(state.cliMatched)return true;
    if(!Read<uint8_t>(reinterpret_cast<void*>(engine::image),0xd958b90))return Fail(6304,"ares_local_key_command_line_not_initialized");
    const auto command=engine::function<const wchar_t*(*)()>(0x19d0320)();
    if(!Token(command,L"-LocalServerNativeCipher"))return Fail(6305,"ares_local_key_marker_requires_explicit_cli");
    state.cliMatched=true;return true;
}
inline int Base64Value(unsigned char value){
    if(value>='A'&&value<='Z')return value-'A';
    if(value>='a'&&value<='z')return value-'a'+26;
    if(value>='0'&&value<='9')return value-'0'+52;
    return value=='+'?62:value=='/'?63:-1;
}
inline bool CanonicalInput(const char (&value)[44]){
    if(value[43]!='=')return false;
    for(size_t i=0;i<43;++i)if(Base64Value(static_cast<unsigned char>(value[i]))<0)return false;
    return (Base64Value(static_cast<unsigned char>(value[42]))&3)==0;
}
inline bool Init(uint32_t role,const wchar_t* directory,uint32_t port){
    if(state.initialized)return (state.role==role&&state.port==port)?!state.error:Fail(6306,"ares_local_key_initial_scope_changed");
    state.initialized=true;state.role=role;state.port=port;
    if(role<1||role>2||port<1024||port>65535)return Fail(6307,"ares_local_key_invalid_role_or_port");
    if(!directory||!directory[0]){Cache();return true;}
    const size_t length=wcsnlen_s(directory,1024);
    std::wstring folder;
    if(length>1000||!local_server_owned_paths::CanonicalAbsolute(directory,folder)
       ||!local_server_owned_paths::RoleDirectory(folder,role))
        return Fail(6308,"ares_local_key_file_must_belong_to_exact_owned_pair_role");
    const auto path=folder+L"\\native-local-key.bin";
    const auto file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){
        state.lastWin32Error=GetLastError();
        if(state.lastWin32Error==ERROR_FILE_NOT_FOUND||state.lastWin32Error==ERROR_PATH_NOT_FOUND){Cache();return true;}
        return Fail(6309,"ares_local_key_marker_read_denied");
    }
    Marker marker{};LARGE_INTEGER size{};DWORD got=0;
    const bool read=GetFileSizeEx(file,&size)&&size.QuadPart==static_cast<LONGLONG>(sizeof(marker))
        &&ReadFile(file,&marker,static_cast<DWORD>(sizeof(marker)),&got,nullptr)&&got==sizeof(marker);
    if(!read)state.lastWin32Error=GetLastError();CloseHandle(file);
    bool nonzeroProof=false;for(const auto byte:marker.proofId)nonzeroProof=nonzeroProof||byte!=0;
    const bool valid=read&&marker.magic==MarkerMagic&&marker.version==MarkerVersion&&marker.bytes==MarkerBytes
        &&marker.role==role&&marker.port==port&&marker.flags==ExplicitFlag&&marker.nonce&&nonzeroProof
        &&std::memcmp(marker.imageSHA,ImageSHA,sizeof(ImageSHA))==0&&CanonicalInput(marker.base64);
    if(valid){for(size_t i=0;i<44;++i)privateKey[i]=static_cast<wchar_t>(marker.base64[i]);privateKey[44]=0;}
    SecureZeroMemory(&marker,sizeof(marker));
    if(!valid)return Fail(6310,"ares_local_key_marker_schema_or_canonical_input_invalid");
    state.configured=true;state.keyLoaded=true;state.stage="ares_local_key_marker_loaded_cli_not_yet_checked";Cache();return true;
}
inline bool Classes(bool requirePending){
    if(driverClass&&!Live(driverClass,driverClassId))return false;
    if(connectionClass&&!Live(connectionClass,connectionClassId))return false;
    if(pendingClass&&!Live(pendingClass,pendingClassId))return false;
    if(!driverClass){driverClass=engine::Find(L"/Script/ShooterGame.AresNetDriver");if(!Identity(driverClass,driverClassId))return false;}
    if(!connectionClass){connectionClass=engine::Find(L"/Script/ShooterGame.AresNetConnection");if(!Identity(connectionClass,connectionClassId))return false;}
    if(requirePending&&!pendingClass){pendingClass=engine::Find(L"/Script/Engine.PendingNetGame");if(!Identity(pendingClass,pendingClassId))return false;}
    return !requirePending||pendingClass;
}
inline bool ValidWeakShared(const Shared& reference){
    return reference.object&&reference.control&&original_players::Readable(reference.control,16)
        &&Read<int32_t>(reference.control,8)>0&&Read<int32_t>(reference.control,12)>0;
}
inline bool LastDelegate(){
    const auto list=Read<DelegateArray>(reinterpret_cast<void*>(engine::image+EventRva));
    if(list.count<1||list.count>4096||list.capacity<list.count
       ||!original_players::Readable(list.data,static_cast<size_t>(list.count)*sizeof(NativeDelegate)))return false;
    const auto& tail=list.data[list.count-1];
    return tail.instance==delegateInstance&&tail.units==2
        &&Read<uintptr_t>(tail.instance)==engine::image+0xa84edb8
        &&Read<uint64_t>(tail.instance,0x10)==state.delegateHandle;
}
inline bool OriginalCallbackFrame(void* pending,void* connection){
    if(!captureContext||!lookupFunction||!virtualUnwind)return false;
    alignas(16) CONTEXT context{};captureContext(&context);
    const auto tib=reinterpret_cast<const NT_TIB*>(NtCurrentTeb());
    const auto low=reinterpret_cast<uintptr_t>(tib->StackLimit),high=reinterpret_cast<uintptr_t>(tib->StackBase);
    for(unsigned depth=0;depth<24&&context.Rip;++depth){
        if(context.Rsp<low||context.Rsp>high||high-context.Rsp<8)return false;
        if(context.Rip==engine::image+0x4858dc7)
            return reinterpret_cast<void*>(context.Rsi)==pending&&reinterpret_cast<void*>(context.R15)==connection;
        const auto pc=context.Rip,sp=context.Rsp;DWORD64 base=0;
        if(auto function=lookupFunction(context.Rip,&base,nullptr)){
            PVOID data=nullptr;DWORD64 frame=0;
            virtualUnwind(UNW_FLAG_NHANDLER,base,context.Rip,function,&context,&data,&frame,nullptr);
        }else{DWORD64 caller=0;if(!original_players::Read(reinterpret_cast<void*>(context.Rsp),0,caller))return false;
            context.Rip=caller;context.Rsp+=8;}
        if(context.Rsp<sp||(context.Rsp==sp&&context.Rip==pc))return false;
    }return false;
}
inline bool LoopbackPending(void* pending){
    // Original UPendingNetGame.URL is +48; the native FURL Host and Port are
    // +10/+20 within it. Read the actual requested URL, not the marker text.
    const auto host=Read<NativeString>(pending,0x58);state.shape.portMatches=Read<int32_t>(pending,0x68)==static_cast<int32_t>(state.port);
    if(!host.data||host.count<2||host.count>64||host.capacity<host.count
       ||!original_players::Readable(host.data,static_cast<size_t>(host.count)*sizeof(wchar_t))||host.data[host.count-1])return false;
    state.shape.loopbackURL=std::wcscmp(host.data,L"127.0.0.1")==0||std::wcscmp(host.data,L"::1")==0;
    return state.shape.loopbackURL&&state.shape.portMatches;
}
inline bool ObserveShape(void* connection,void* pending,uint8_t mode){
    auto& s=state.shape;s={};s.pending=pending;s.connection=connection;s.driver=Read<void*>(connection,0x60);
    if(!Classes(pending!=nullptr))return Fail(6311,"ares_local_key_actual_native_classes_missing_or_changed");
    s.actualClasses=Read<void*>(s.driver,0x10)==driverClass&&Read<void*>(connection,0x10)==connectionClass
        &&(!pending||Read<void*>(pending,0x10)==pendingClass);
    if(!s.actualClasses||!Identity(s.driver,s.driverId)||!Identity(connection,s.connectionId)
       ||(pending&&!Identity(pending,s.pendingId)))return Fail(6312,"ares_local_key_actual_connection_identity_invalid");
    const auto driverTable=Read<void*>(s.driver),connectionTable=Read<void*>(connection);
    s.originalVirtuals=Read<uintptr_t>(driverTable,0x308)==engine::image+0x5118500
        &&Read<uintptr_t>(driverTable,0x310)==engine::image+0x5118750
        &&Read<uintptr_t>(driverTable,0x320)==engine::image+0x716e770
        &&Read<uintptr_t>(connectionTable,0x320)==engine::image+0x51149b0
        &&Read<uintptr_t>(connectionTable,0x328)==engine::image+0x4712570
        &&Read<uintptr_t>(connectionTable,0x330)==engine::image+0x47125b0
        &&Read<uintptr_t>(connectionTable,0x378)==engine::image+0x5113150
        &&Read<uintptr_t>(connectionTable,0x380)==engine::image+0x51134e0
        &&Read<uintptr_t>(connectionTable,0x388)==engine::image+0x51132f0
        &&Read<uintptr_t>(connectionTable,0x398)==engine::image+0x716db30
        &&Read<uintptr_t>(connectionTable,0x3d0)==engine::image+0x716e450;
    if(!s.originalVirtuals)return Fail(6331,"ares_local_key_original_connection_virtuals_changed");
    s.links=Read<void*>(connection,0x60)==s.driver&&(mode==0?Read<void*>(s.driver,0xc0)==connection:!Read<void*>(s.driver,0xc0));
    if(pending)s.links=s.links&&Read<void*>(pending,0x38)==s.driver
        &&static_cast<uint8_t*>(pending)+0x30==Read<void*>(s.driver,0x260);
    s.handler=Read<void*>(connection,0x148);s.mode=Read<uint8_t>(s.handler,0x10);s.connectionless=Read<uint8_t>(s.handler,0x11);
    s.handlerState=Read<uint8_t>(s.handler,0x1e4);s.handshaking=Read<uint8_t>(s.handler,0x288);
    s.connectionState=Read<uint8_t>(connection,0x13c);
    const auto chain=Read<SharedArray>(s.handler,0x1c0);s.components=chain.count;s.componentCapacity=chain.capacity;
    if(!s.links||!s.handler||s.mode!=mode||s.connectionless||chain.count!=3||chain.capacity<3
       ||!original_players::Readable(chain.data,3*sizeof(Shared)))return Fail(6313,"ares_local_key_native_handler_shape_not_supported");
    s.cipher=chain.data[0].object;s.stateless=chain.data[1].object;s.ddos=chain.data[2].object;
    s.exactChain=Read<uintptr_t>(s.cipher)==engine::image+CipherTableRva
        &&Read<uintptr_t>(s.stateless)==engine::image+StatelessTableRva&&Read<uintptr_t>(s.ddos)==engine::image+DDoSTableRva;
    s.owners=Read<void*>(s.cipher,8)==s.handler&&Read<void*>(s.stateless,8)==s.handler&&Read<void*>(s.ddos,8)==s.handler;
    s.sharedOwners=ValidWeakShared(chain.data[0])&&ValidWeakShared(chain.data[1])&&ValidWeakShared(chain.data[2]);
    s.driverLink=Read<void*>(s.stateless,0x38)==s.driver;
    s.ddosWeakLink=Read<void*>(s.stateless,0x28)==s.ddos
        &&Read<void*>(s.stateless,0x30)==chain.data[2].control&&Read<int32_t>(chain.data[2].control,8)>0;
    s.active=Read<uint8_t>(s.cipher,0x1a)==1&&Read<uint8_t>(s.stateless,0x1a)==1&&Read<uint8_t>(s.ddos,0x1a)==1;
    s.statelessInitialized=Read<uint8_t>(s.stateless,0x1b);
    s.initialization=Read<uint8_t>(s.cipher,0x1b)==1&&Read<uint8_t>(s.ddos,0x1b)==1;
    if(!s.exactChain||!s.owners||!s.sharedOwners||!s.driverLink||!s.ddosWeakLink||!s.active||!s.initialization)
        return Fail(6314,"ares_local_key_original_component_ownership_or_initialization_changed");
    return true;
}
inline bool ClientInstall(void* pending){
    ++state.callbacks;
    if(!state.configured||state.closed)return true;
    if(state.error)return false;
    if(!Thread())return Fail(6315,"ares_local_key_client_callback_wrong_original_thread");
    if(state.role!=2||!state.armed)return Fail(6316,"ares_local_key_client_callback_not_armed");
    // Unrelated pending attempts are not recipients of this pair's credential.
    if(!LoopbackPending(pending)){++state.unrelatedCallbacks;Cache();return true;}
    if(state.applied){
        const auto connection=Read<void*>(Read<void*>(pending,0x38),0xc0);
        if(pending==state.shape.pending&&connection==state.shape.connection
           &&Live(pending,state.shape.pendingId)&&Live(connection,state.shape.connectionId)){Cache();return true;}
        ++state.reinstallRejections;return Fail(6317,"ares_local_key_new_connection_requires_new_local_pair");
    }
    auto connection=Read<void*>(Read<void*>(pending,0x38),0xc0);
    if(!ObserveShape(connection,pending,0))return false;
    auto& s=state.shape;
    // ObserveShape deliberately resets the snapshot: repeat the actual URL test.
    if(!LoopbackPending(pending))return Fail(6318,"ares_local_key_actual_pending_URL_changed");
    s.delegateTail=LastDelegate();s.callbackFrame=OriginalCallbackFrame(pending,connection);
    state.clientFirstUseShape=s.delegateTail&&s.callbackFrame&&s.handlerState==1&&!s.handshaking
        &&!s.statelessInitialized&&s.connectionState==2&&Read<uint8_t>(s.handler,0x260)==0;
    if(!state.clientFirstUseShape){++state.firstUseRejections;return Fail(6319,"ares_local_key_client_pre_handshake_interval_not_proven");}
    if(!state.keyLoaded||privateKey[44]||!privateKey[0])return Fail(6320,"ares_local_key_private_bootstrap_not_available");
    std::array<uint8_t,32> beforeOutgoing{},beforeIncoming{};
    if(!original_players::Read(s.cipher,0x28,beforeOutgoing)||!original_players::Read(s.cipher,0x48,beforeIncoming))
        return Fail(6321,"ares_local_key_original_context_shape_unreadable");
    NativeString value{privateKey.data(),45,45};state.operation="original_native_connection_key_initializer";
    ++state.installCalls;engine::function<void(*)(void*,const NativeString*)>(0x29f1630)(s.cipher,&value);
    state.installerReturned=true;
    std::array<uint8_t,32> outgoing{},incoming{};
    const bool observed=original_players::Read(s.cipher,0x28,outgoing)&&original_players::Read(s.cipher,0x48,incoming);
    if(observed){
        bool outPresent=false,inPresent=false;for(size_t i=0;i<32;++i){outPresent=outPresent||outgoing[i]!=0;inPresent=inPresent||incoming[i]!=0;}
        state.contextsPresent=outPresent&&inPresent;
        state.contextsDistinct=std::memcmp(outgoing.data(),incoming.data(),32)!=0;
        state.outgoingContextChanged=std::memcmp(outgoing.data(),beforeOutgoing.data(),32)!=0;
        state.incomingContextChanged=std::memcmp(incoming.data(),beforeIncoming.data(),32)!=0;
    }
    SecureZeroMemory(beforeOutgoing.data(),beforeOutgoing.size());SecureZeroMemory(beforeIncoming.data(),beforeIncoming.size());
    SecureZeroMemory(outgoing.data(),outgoing.size());SecureZeroMemory(incoming.data(),incoming.size());EraseKey();
    if(!observed||!state.contextsPresent||!state.contextsDistinct){state.restartRequired=true;
        return Fail(6322,"ares_local_key_native_context_witness_failed_requires_new_pair");}
    state.applied=true;state.operation="none";state.stage="ares_local_key_client_native_call_returned_peer_and_crypto_validation_unverified";Cache();return true;
}
inline LONG Exception(EXCEPTION_POINTERS* info){
    state.error=info&&info->ExceptionRecord?info->ExceptionRecord->ExceptionCode:0xe0006300U;
    const auto address=info&&info->ExceptionRecord?reinterpret_cast<uintptr_t>(info->ExceptionRecord->ExceptionAddress):0;
    state.exceptionRva=address>=engine::image?address-engine::image:address;state.restartRequired=true;
    state.stage="ares_local_key_adapter_exception_requires_new_owned_pair";EraseKey();Cache();return EXCEPTION_EXECUTE_HANDLER;
}
inline void ClientCallback(void* pending){__try{ClientInstall(pending);}__except(Exception(GetExceptionInformation())){}}
inline bool ArmClientBeforeTravelImpl(){
    if(!state.initialized||!state.configured||state.role!=2||state.closed)return !state.error;
    if(state.error)return false;
    if(!Thread())return Fail(6323,"ares_local_key_subscription_requires_original_game_thread");
    ++state.armCalls;if(state.armed)return true;
    if(!VerifyProfiles()||!ExplicitCommandLine())return false;state.expectedThread=GetCurrentThreadId();
    const auto ntdll=GetModuleHandleW(L"ntdll.dll");
    captureContext=reinterpret_cast<CaptureContext>(GetProcAddress(ntdll,"RtlCaptureContext"));
    lookupFunction=reinterpret_cast<LookupFunction>(GetProcAddress(ntdll,"RtlLookupFunctionEntry"));
    virtualUnwind=reinterpret_cast<Unwind>(GetProcAddress(ntdll,"RtlVirtualUnwind"));
    if(!captureContext||!lookupFunction||!virtualUnwind)return Fail(6324,"ares_local_key_native_unwind_unavailable");
    NativeDelegate storage{};state.operation="original_bind_static_pending_delegate";
    engine::function<void(*)(NativeDelegate*,void(*)(void*))>(0x3f4cb70)(&storage,ClientCallback);
    delegateInstance=storage.instance;
    if(storage.units!=2||Read<uintptr_t>(delegateInstance)!=engine::image+0xa84edb8
       ||Read<void*>(delegateInstance,0x18)!=reinterpret_cast<void*>(ClientCallback))
        return Fail(6325,"ares_local_key_original_static_delegate_shape_changed");
    state.operation="original_append_pending_delegate";
    engine::function<uint64_t*(*)(void*,uint64_t*,NativeDelegate*)>(0x18d1ad0)
        (reinterpret_cast<void*>(engine::image+EventRva),&state.delegateHandle,&storage);
    state.delegateAdded=state.delegateHandle&&storage.instance==nullptr&&storage.units==0;
    if(!state.delegateAdded||!LastDelegate())return Fail(6326,"ares_local_key_native_subscription_not_last_or_not_owned");
    state.armed=true;state.operation="none";state.stage="ares_local_key_original_pre_handshake_callback_armed";Cache();return true;
}
inline bool ArmClientBeforeTravel(){__try{return ArmClientBeforeTravelImpl();}__except(Exception(GetExceptionInformation())){return false;}}
inline bool InstallAcceptedConnectionImpl(void* connection){
    if(!state.initialized||!state.configured||state.role!=1||state.closed)return !state.error;
    if(state.error)return false;
    if(!Thread())return Fail(6327,"ares_local_key_server_callback_requires_original_game_thread");
    if(!VerifyProfiles()||!ExplicitCommandLine())return false;
    ++state.serverCallbacks;state.expectedThread=GetCurrentThreadId();
    if(!ObserveShape(connection,nullptr,1))return false;
    // NotifyAccepted is after server BeginHandshaking. The recovered ordering
    // alone does not rule out every prior ordinary cipher callback. Never infer
    // unused from the randomized outgoing counter or an incoming zero value.
    ++state.firstUseRejections;
    return Fail(6328,"ares_local_key_server_zero_cipher_io_first_use_not_proven_no_install");
}
inline bool InstallAcceptedConnection(void* connection){
    __try{return InstallAcceptedConnectionImpl(connection);}__except(Exception(GetExceptionInformation())){return false;}
}
inline bool CloseImpl(){
    if(state.closed)return !state.error;
    if(state.delegateHandle){
        if(!Thread(true))return Fail(6329,"ares_local_key_own_delegate_cleanup_requires_original_thread");
        state.operation="original_remove_own_pending_delegate";
        state.delegateRemoved=engine::function<bool(*)(void*,uint64_t)>(0x18cd890)
            (reinterpret_cast<void*>(engine::image+EventRva),state.delegateHandle);
        if(!state.delegateRemoved)return Fail(6330,"ares_local_key_own_delegate_removal_not_observed");
        state.delegateHandle=0;delegateInstance=nullptr;
    }
    EraseKey();state.closed=true;state.armed=false;state.operation="none";
    if(!state.error)state.stage=state.configured?"ares_local_key_adapter_closed":"ares_local_key_disabled_closed";
    Cache();return !state.error;
}
inline bool Close(){__try{return CloseImpl();}__except(Exception(GetExceptionInformation())){return false;}}
inline void WriteCachedState(FILE* file){
    if(!file)return;const auto s=Snapshot();const auto& o=s.shape;const auto b=[](bool value){return value?"true":"false";};
    std::fprintf(file,"{\"schema_version\":1,\"scope\":\"optional_original_native_local_credential_installation\",\"stage\":\"%s\",\"operation\":\"%s\",\"error\":%u,\"role\":%u,\"port\":%u,\"initialized\":%s,\"configured\":%s,\"explicit_cli_matched\":%s,\"profiles_verified\":%s,\"armed\":%s,\"applied\":%s,\"closed\":%s,\"restart_required\":%s,\"key_loaded\":%s,\"key_erased\":%s,\"expected_thread\":%u,\"actual_thread\":%u,\"callbacks\":%llu,\"unrelated_callbacks\":%llu,\"server_callbacks\":%llu,\"install_calls\":%llu,\"reinstall_rejections\":%llu,\"first_use_rejections\":%llu,\"delegate_added\":%s,\"delegate_removed\":%s,\"client_first_use_shape\":%s,\"native_installer_returned\":%s,\"contexts_present\":%s,\"contexts_distinct\":%s,\"outgoing_context_changed\":%s,\"incoming_context_changed\":%s,\"profile_failure_rva\":%llu,\"exception_rva\":%llu,\"last_win32_error\":%u,\"shape\":{\"components\":%d,\"handler_state\":%d,\"connection_state\":%d,\"mode\":%u,\"connectionless\":%u,\"handshaking\":%u,\"stateless_initialized\":%u,\"actual_classes\":%s,\"original_virtuals\":%s,\"links\":%s,\"owners\":%s,\"shared_owners\":%s,\"exact_chain\":%s,\"driver_link\":%s,\"ddos_weak_link\":%s,\"active\":%s,\"initialization\":%s,\"actual_loopback_URL\":%s,\"actual_port_matches\":%s,\"own_delegate_last\":%s,\"original_callback_frame\":%s},\"server_installation_supported\":false,\"zero_cipher_io_observed\":false,\"directional_peer_materials_verified\":false,\"cryptographic_installation_verified\":false,\"native_handshake_verified\":false,\"native_join_verified\":false,\"key_or_context_contents_logged\":false,\"official_identity_or_ready_writes\":0,\"native_validation_changes\":0,\"playable_verified\":false}",
        s.stage,s.operation,s.error,s.role,s.port,b(s.initialized),b(s.configured),b(s.cliMatched),b(s.profilesVerified),b(s.armed),b(s.applied),b(s.closed),b(s.restartRequired),b(s.keyLoaded),b(s.keyErased),s.expectedThread,s.actualThread,
        static_cast<unsigned long long>(s.callbacks),static_cast<unsigned long long>(s.unrelatedCallbacks),static_cast<unsigned long long>(s.serverCallbacks),static_cast<unsigned long long>(s.installCalls),static_cast<unsigned long long>(s.reinstallRejections),static_cast<unsigned long long>(s.firstUseRejections),b(s.delegateAdded),b(s.delegateRemoved),b(s.clientFirstUseShape),b(s.installerReturned),b(s.contextsPresent),b(s.contextsDistinct),b(s.outgoingContextChanged),b(s.incomingContextChanged),
        static_cast<unsigned long long>(s.profileFailureRva),static_cast<unsigned long long>(s.exceptionRva),s.lastWin32Error,o.components,o.handlerState,o.connectionState,unsigned(o.mode),unsigned(o.connectionless),unsigned(o.handshaking),unsigned(o.statelessInitialized),b(o.actualClasses),b(o.originalVirtuals),b(o.links),b(o.owners),b(o.sharedOwners),b(o.exactChain),b(o.driverLink),b(o.ddosWeakLink),b(o.active),b(o.initialization),b(o.loopbackURL),b(o.portMatches),b(o.delegateTail),b(o.callbackFrame));
}
} // namespace local_server_ares_local_key
