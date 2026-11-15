#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define PSAPI_VERSION 2
#include <windows.h>
#include <psapi.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

// Standalone startup adapter. No dependency on trainer_bridge.dll or its ABI.
// The owner suspends its child's primary thread, then invokes Install through
// the normal DLL export. We never call a UClass getter from this loader thread.
// The only original write is CAS of an existing private-RW UClass.ClassConstructor.
namespace {
constexpr uintptr_t kClassCache=0xda6a910,kClassVtable=0xa85ec50,kCtor=0x648a040;
constexpr uintptr_t kTable=0xb0869c0,kSecondary=0xb084458;
constexpr size_t kTableBytes=0x460,kInitSlot=0x308/8,kShutdownSlot=0x310/8;
constexpr uint64_t kMagic=0x314947454e4f4c41ULL;
constexpr uint32_t kImageBytes=0xe69c000,kTimestamp=2142902647;
constexpr uintptr_t kOriginalSlots[140]={
  0x647ef00,0x18a3a00,0x1cfa620,0x1af09d0,0x18a76a0,0x1d06c20,0x1d06c40,0x18a3a00,
  0x1af0a70,0x1af0a80,0x1af0a90,0x1af0aa0,0x1af0a90,0x1af0ab0,0x460fe00,0x18a3a00,
  0x18a3a00,0x18a3a00,0x18a76a0,0x18a3a00,0x18a3a00,0x1c53bd0,0x18a3a00,0x18a76a0,
  0x18a3ec0,0x1c52d20,0x1c53b00,0x4610080,0x18a3ec0,0x43c41c0,0x1c53df0,0x1c53d80,
  0x18a3ed0,0x18a3a00,0x18a3a00,0x18a3a00,0x18a3a00,0x1af0b30,0x18a3a00,0x1c53720,
  0x1c53700,0x18a3ec0,0x18a76a0,0x18a76a0,0x18a76a0,0x18a76a0,0x18a76a0,0x1c53be0,
  0x18a3a00,0x18a3a00,0x18a3a00,0x18a3a00,0x18a3a00,0x1c52670,0x1af0b50,0x43c4240,
  0x18a76a0,0x1c562c0,0x1af0b70,0x18a3ed0,0x18a3ec0,0x1c554e0,0x18a3a00,0x1c56400,
  0x1c56490,0x1c56550,0x1c56640,0x1c56670,0x18a3a00,0x18a3a00,0x18a3a00,0x1c5d660,
  0x1c5dc90,0x1c5dce0,0x18a3a00,0x18a3a00,0x18a3a00,0x18a3a00,0x18a3a00,0x1c54130,
  0x18a3ed0,0x18a3a00,0x18a3a00,0x1cdc850,0x1914910,0x18a76a0,0x18a76a0,0x18a3ed0,
  0x18a3a00,0x1c54c40,0x18a3a00,0x18a3a00,0x18dd100,0x43c58a0,0x7047d60,0x43c58e0,
  0x43c5900,0x7047190,0x7047530,0x43c4a40,0x43c4a70,0x18a3a00,0x43c4b60,0x18a76a0,
  0x18a76a0,0x18a3a00,0x18a3a00,0x18a3a00,0x43c5eb0,0x43c64e0,0x43c6450,0x43c65f0,
  0x7047b00,0x18a3a00,0x7047d40,0x18a3a00,0x7047de0,0x43c72d0,0x7048040,0x7048250,
  0x43c7570,0x43c7640,0x18a3a00,0x43c7790,0x43c7a30,0x18a76a0,0x3e08dd0,0x43c84b0,
  0x43c7610,0x7047a20,0x18a3a00,0x18a76a0,0x18a76a0,0x43c7c70,0x43c7c90,0x43c7cc0,
  0x43c8470,0x43c92d0,0x43c9350,0x18a3a00};
static_assert(sizeof(kOriginalSlots)==kTableBytes);
struct Config {
    uint64_t magic;uint32_t version,bytes,pid,primaryThread;uint64_t image;
    uint32_t reserved,flags;wchar_t report[1024];
};
static_assert(sizeof(Config)==2088);
using Constructor=void(*)(const void*);
uintptr_t image=0;void* type=nullptr;Constructor original=nullptr;
DWORD primaryThread=0;wchar_t reportPath[1024]{};
SRWLOCK reportLock=SRWLOCK_INIT;
volatile LONG installed=0,error=0,ctorEntered=0,ctorCompleted=0,templates=0,instances=0;
uint64_t installedAt=0,firstConstructorAt=0,firstInstanceAt=0;
uintptr_t hostModeFactory=0,hostModeFactoryOwner=0;
uintptr_t lastInstance=0,lastClone=0;DWORD lastConstructorThread=0,sourceProtect=0;
uint64_t sourceWsFlags=0;bool sourceShared=false,suspensionVerified=false,installedBeforeCdo=false;
bool originalCtorPreserved=false,otherSlotsPreserved=false,secondaryPreserved=false,classPreserved=false;
const char* stage="not_installed";
template<class T> bool Read(const void* p,size_t offset,T& out){
    if(!p)return false;
    __try{std::memcpy(&out,static_cast<const uint8_t*>(p)+offset,sizeof(T));return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
template<class T> T Value(const void* p,size_t offset=0){T result{};Read(p,offset,result);return result;}
bool PrivateWritable(void* p,size_t bytes){
    MEMORY_BASIC_INFORMATION m{};
    if(!p||!VirtualQuery(p,&m,sizeof(m)))return false;
    const auto start=reinterpret_cast<uintptr_t>(p),base=reinterpret_cast<uintptr_t>(m.BaseAddress);
    return m.State==MEM_COMMIT&&m.Type==MEM_PRIVATE&&(m.Protect&0xff)==PAGE_READWRITE
        &&!(m.Protect&(PAGE_GUARD|PAGE_NOACCESS))&&start>=base&&start+bytes<=base+m.RegionSize;
}
bool SourceTable(){
    auto source=reinterpret_cast<const uintptr_t*>(image+kTable);
    for(size_t i=0;i<140;++i)if(Value<uintptr_t>(source,i*8)!=image+kOriginalSlots[i])return false;
    MEMORY_BASIC_INFORMATION m{};
    if(!VirtualQuery(source,&m,sizeof(m)))return false;
    sourceProtect=m.Protect;
    if(m.AllocationBase!=reinterpret_cast<void*>(image)||m.State!=MEM_COMMIT
        ||(m.Type!=MEM_IMAGE&&m.Type!=MEM_MAPPED)||(m.Protect&0xff)!=PAGE_READONLY)return false;
    PSAPI_WORKING_SET_EX_INFORMATION working{};working.VirtualAddress=const_cast<uintptr_t*>(source);
    using Query=BOOL(WINAPI*)(HANDLE,PVOID,DWORD);
    auto query=reinterpret_cast<Query>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"K32QueryWorkingSetEx"));
    if(!query||!query(GetCurrentProcess(),&working,sizeof(working)))return false;
    sourceWsFlags=working.VirtualAttributes.Flags;
    sourceShared=(sourceWsFlags&0x8001)==0x8001;
    return sourceShared;
}
void Report(){
    if(!reportPath[0])return;
    AcquireSRWLockExclusive(&reportLock);
    FILE* f=nullptr;
    if(!_wfopen_s(&f,reportPath,L"wb")&&f){
        std::fprintf(f,"{\"schema_version\":1,\"pid\":%lu,\"stage\":\"%s\",\"error\":%ld,\"installed\":%ld,\"primary_thread\":%lu,\"primary_suspension_verified\":%d,\"installed_before_cdo\":%d,\"class\":\"%p\",\"class_constructor_offset\":184,\"original_constructor_rva\":105422912,\"primary_vtable_bytes\":1120,\"constructor_entered\":%ld,\"constructor_completed\":%ld,\"templates_preserved\":%ld,\"instances_configured\":%ld,\"installed_tick_ms\":%llu,\"first_constructor_tick_ms\":%llu,\"first_instance_tick_ms\":%llu,\"last_constructor_thread\":%lu,\"last_instance\":\"%p\",\"last_private_vtable\":\"%p\",\"original_constructor_preserved\":%d,\"other_slots_preserved\":%d,\"secondary_vtable_preserved\":%d,\"original_class_preserved\":%d,\"source_page_protection\":%lu,\"source_working_set_flags\":%llu,\"source_valid_and_shared\":%d,\"image_pages_written\":false,\"image_page_protections_changed\":false,\"class_getter_called\":false,\"checks_disabled\":false,\"host_mode_factory_bound\":%s}\n",
          GetCurrentProcessId(),stage,error,installed,primaryThread,suspensionVerified,installedBeforeCdo,type,
          ctorEntered,ctorCompleted,templates,instances,static_cast<unsigned long long>(installedAt),
          static_cast<unsigned long long>(firstConstructorAt),static_cast<unsigned long long>(firstInstanceAt),
          lastConstructorThread,reinterpret_cast<void*>(lastInstance),reinterpret_cast<void*>(lastClone),
          originalCtorPreserved,otherSlotsPreserved,secondaryPreserved,classPreserved,sourceProtect,
          static_cast<unsigned long long>(sourceWsFlags),sourceShared,hostModeFactory?"true":"false");
        std::fclose(f);
    }
    ReleaseSRWLockExclusive(&reportLock);
}
DWORD Fail(DWORD code,const char* why){InterlockedExchange(&error,static_cast<LONG>(code));stage=why;Report();return code;}
[[noreturn]] void ConstructorFailure(DWORD code,const char* why){
    Fail(code,why);TerminateProcess(GetCurrentProcess(),code);for(;;)Sleep(INFINITE);
}
void Construct(const void* initializer){
    const auto entered=InterlockedIncrement(&ctorEntered);
    if(entered==1)firstConstructorAt=GetTickCount64();
    lastConstructorThread=GetCurrentThreadId();
    // Same ABI as648a040: rcx=&FObjectInitializer; its first qword is UObject*.
    // This is the COMPLETE Ares constructor, including its original base call.
    original(initializer);
    InterlockedIncrement(&ctorCompleted);originalCtorPreserved=true;
    auto object=Value<void*>(initializer);
    if(!object||Value<void*>(object,0x10)!=type||Value<uintptr_t>(object)!=image+kTable
        ||Value<uintptr_t>(object,0x30)!=image+kSecondary)
        ConstructorFailure(2121,"original_ares_constructor_identity_mismatch");
    if((Value<uint32_t>(object,8)&0x30)!=0){
        InterlockedIncrement(&templates);stage="original_ares_template_constructor_preserved";Report();return;
    }
    if(lastConstructorThread!=primaryThread||!PrivateWritable(object,8)||!SourceTable())
        ConstructorFailure(2122,"original_gi_instance_owner_or_source_page_mismatch");
    auto clone=static_cast<uintptr_t*>(VirtualAlloc(nullptr,kTableBytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!clone)ConstructorFailure(2123,"private_gi_vtable_allocation_failed");
    std::memcpy(clone,reinterpret_cast<const void*>(image+kTable),kTableBytes);
    clone[kInitSlot]=image+0x43c4260;clone[kShutdownSlot]=image+0x43c48c0;
    if(hostModeFactory)clone[0x3f8/8]=hostModeFactory;
    otherSlotsPreserved=true;
    for(size_t i=0;i<140;++i)if(i!=kInitSlot&&i!=kShutdownSlot&&!(hostModeFactory&&i==0x3f8/8)&&clone[i]!=image+kOriginalSlots[i])otherSlotsPreserved=false;
    DWORD previous=0;
    if(!otherSlotsPreserved||!VirtualProtect(clone,kTableBytes,PAGE_READONLY,&previous)||previous!=PAGE_READWRITE)
        ConstructorFailure(2124,"private_gi_vtable_readonly_or_copy_verification_failed");
    // Publish the fully initialized, non-executable, read-only PRIVATE table.
    auto prior=InterlockedCompareExchangePointer(static_cast<void* volatile*>(object),clone,reinterpret_cast<void*>(image+kTable));
    if(prior!=reinterpret_cast<void*>(image+kTable))ConstructorFailure(2125,"original_gi_instance_vptr_changed_before_publication");
    secondaryPreserved=Value<uintptr_t>(object,0x30)==image+kSecondary;
    classPreserved=Value<void*>(object,0x10)==type;
    if(!secondaryPreserved||!classPreserved||Value<uintptr_t>(object)!=reinterpret_cast<uintptr_t>(clone)||!SourceTable())
        ConstructorFailure(2126,"original_gi_instance_postcondition_failed");
    lastInstance=reinterpret_cast<uintptr_t>(object);lastClone=reinterpret_cast<uintptr_t>(clone);
    if(InterlockedIncrement(&instances)==1)firstInstanceAt=GetTickCount64();
    stage="original_gi_instance_engine_lifecycle_configured";Report();
}
DWORD InstallImpl(const Config& config){
    if(config.magic!=kMagic||config.version!=1||config.bytes!=sizeof(Config)
        ||config.pid!=GetCurrentProcessId()||!config.primaryThread||config.primaryThread==GetCurrentThreadId()
        ||config.flags!=1||config.reserved||!config.report[0]||config.report[1023])return 2101;
    std::memcpy(reportPath,config.report,sizeof(reportPath));primaryThread=config.primaryThread;
    image=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    if(image!=config.image)return Fail(2102,"owned_image_base_mismatch");
    auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
    if(Value<WORD>(dos)!=IMAGE_DOS_SIGNATURE)return Fail(2103,"original_image_dos_profile_mismatch");
    auto nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(image+Value<LONG>(dos,0x3c));
    if(Value<DWORD>(nt)!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64
        ||nt->FileHeader.TimeDateStamp!=kTimestamp||nt->OptionalHeader.SizeOfImage!=kImageBytes)
        return Fail(2103,"original_image_pe_profile_mismatch");
    using QueryThread=LONG(NTAPI*)(HANDLE,ULONG,PVOID,ULONG,PULONG);
    auto query=reinterpret_cast<QueryThread>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"NtQueryInformationThread"));
    auto thread=OpenThread(THREAD_QUERY_INFORMATION,FALSE,primaryThread);ULONG suspendCount=0;
    suspensionVerified=query&&thread&&GetProcessIdOfThread(thread)==GetCurrentProcessId()
        &&query(thread,35,&suspendCount,sizeof(suspendCount),nullptr)>=0&&suspendCount>0;
    if(thread)CloseHandle(thread);
    if(!suspensionVerified)return Fail(2104,"owned_primary_thread_not_suspended");
    type=Value<void*>(reinterpret_cast<void*>(image+kClassCache));
    if(!type)return Fail(2105,"original_class_not_registered_at_startup_hold");
    if(!PrivateWritable(static_cast<uint8_t*>(type)+0xb8,8)||Value<uintptr_t>(type)!=image+kClassVtable
        ||Value<int>(type,0x60)!=0x310||Value<uintptr_t>(type,0xb8)!=image+kCtor)
        return Fail(2106,"original_class_constructor_heap_profile_mismatch");
    // Strict early gate: do not guess whether an already created class CDO or
    // an existing GI is still early enough. Owner must choose an earlier hold.
    installedBeforeCdo=Value<void*>(type,0x120)==nullptr;
    if(!installedBeforeCdo)return Fail(2107,"original_class_default_object_already_created_choose_earlier_hold");
    if(!SourceTable())return Fail(2108,"original_image_vtable_not_pristine_readonly_shared");
    original=reinterpret_cast<Constructor>(image+kCtor);installedAt=GetTickCount64();
    auto slot=reinterpret_cast<void* volatile*>(static_cast<uint8_t*>(type)+0xb8);
    auto previous=InterlockedCompareExchangePointer(slot,reinterpret_cast<void*>(&Construct),reinterpret_cast<void*>(original));
    if(previous!=reinterpret_cast<void*>(original))return Fail(2109,"original_class_constructor_changed_during_startup_hold");
    if(Value<void*>(type,0x120)!=nullptr)return Fail(2110,"original_cdo_started_during_class_constructor_installation");
    InterlockedExchange(&installed,1);stage="class_constructor_hook_installed_before_original_cdo";Report();return 0;
}
}
struct HostModeFactoryBinding {
    uint64_t magic;uint32_t version,bytes,pid,primaryThread;uint64_t image,owner,factory;
};
static_assert(sizeof(HostModeFactoryBinding)==48);
extern "C" __declspec(dllexport) DWORD WINAPI OfflineGiInstanceBindHostModeFactory(void* parameter){
    HostModeFactoryBinding b{};
    if(!Read(parameter,0,b)||b.magic!=0x31524f544341464dULL||b.version!=1||b.bytes!=sizeof(b)
       ||b.pid!=GetCurrentProcessId()||b.primaryThread!=primaryThread||b.image!=image
       ||!installed||error||ctorEntered||instances||hostModeFactory||Value<void*>(type,0x120))return 2130;
    auto owner=reinterpret_cast<HMODULE>(b.owner);
    const auto entry=owner?GetProcAddress(owner,"OfflineShooterModeCreateGameModeForURL"):nullptr;
    MEMORY_BASIC_INFORMATION m{};
    if(!entry||reinterpret_cast<uint64_t>(entry)!=b.factory||!VirtualQuery(entry,&m,sizeof(m))
       ||m.State!=MEM_COMMIT||m.Type!=MEM_IMAGE||m.AllocationBase!=owner
       ||((m.Protect&0xffU)!=PAGE_EXECUTE_READ&&(m.Protect&0xffU)!=PAGE_EXECUTE_READWRITE))return 2131;
    hostModeFactoryOwner=b.owner;hostModeFactory=b.factory;
    stage="original_gi_host_mode_factory_bound_before_cdo";Report();return 0;
}
extern "C" __declspec(dllexport) DWORD WINAPI OfflineGiInstanceInstall(void* parameter){
    if(installed||error)return 2111;
    Config config{};if(!Read(parameter,0,config))return 2101;
    __try{return InstallImpl(config);}
    __except(EXCEPTION_EXECUTE_HANDLER){return Fail(2112,"offline_gi_install_native_exception");}
}
BOOL WINAPI DllMain(HINSTANCE self,DWORD reason,LPVOID){
    if(reason==DLL_PROCESS_ATTACH)DisableThreadLibraryCalls(self);
    return TRUE;
}
