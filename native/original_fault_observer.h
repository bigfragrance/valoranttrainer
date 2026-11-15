#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>

// Passive VEH telemetry for this owned process. Every path returns SEARCH.
// The handler does not handle/recover an exception, touch its context, read a
// stack, allocate, write files, wait, or acquire a lock. Normal C++ exceptions
// are excluded. WriteDiagnostics is for the ordinary SaveState path only.
namespace original_faults {
inline constexpr uint32_t Magic=0x31544c46;
inline constexpr size_t SharedOffset=3648,SharedBytes=4096,Capacity=3,RecordBytes=128;
struct SharedHeader {
    uint32_t magic=0;
    LONG sequence=0;             // odd during publication; reader checks twice
    uint32_t published=0;        // monotonically increasing record sequence
    uint32_t droppedConcurrent=0;// independent atomic counter, not a record
};
struct alignas(8) FaultRecord {
    uint64_t uptimeMilliseconds=0;
    uint32_t threadId=0,code=0,flags=0,contextFlags=0;
    uint64_t exceptionAddress=0,rip=0,rsp=0,rcx=0,rdx=0,r8=0,r9=0;
    uint64_t rax=0,rbx=0,rbp=0,information0=0,information1=0;
    uint32_t numberParameters=0,sequence=0;
};
static_assert(sizeof(LONG)==4&&sizeof(LONG64)==8&&sizeof(SharedHeader)==16);
static_assert(sizeof(FaultRecord)==RecordBytes&&alignof(FaultRecord)==8);
static_assert(offsetof(FaultRecord,threadId)==8&&offsetof(FaultRecord,code)==12);
static_assert(offsetof(FaultRecord,flags)==16&&offsetof(FaultRecord,exceptionAddress)==24);
static_assert(offsetof(FaultRecord,rip)==32&&offsetof(FaultRecord,rsp)==40);
static_assert(offsetof(FaultRecord,rcx)==48&&offsetof(FaultRecord,r9)==72);
static_assert(offsetof(FaultRecord,information0)==104&&offsetof(FaultRecord,sequence)==124);
static_assert(SharedOffset+sizeof(SharedHeader)+Capacity*RecordBytes==4048);
static_assert(SharedOffset+sizeof(SharedHeader)+Capacity*RecordBytes<=SharedBytes);

inline PVOID registration=nullptr;
inline uint8_t* shared=nullptr;
inline DWORD initializeError=0;
inline volatile LONG globalSequence=0,publishedCount=0,droppedCount=0,activeHandlers=0;
alignas(8) inline volatile LONG64 observedCount=0;
alignas(8) inline volatile LONG64 records[Capacity][RecordBytes/8]{};

inline LONG Read32(volatile LONG* value){return InterlockedCompareExchange(value,0,0);}
inline LONG64 Read64(volatile LONG64* value){return InterlockedCompareExchange64(value,0,0);}
inline volatile LONG* HeaderWord(size_t offset){return reinterpret_cast<volatile LONG*>(shared+SharedOffset+offset);}
inline bool ObserveCode(DWORD code){
    switch(code){
        case 0x80000003U: // Breakpoint, including possibly synthetic dump contexts.
        case 0xc0000005U: // Access violation.
        case 0xc0000006U: // In-page error.
        case 0xc000001dU: // Illegal instruction.
        case 0xc0000025U: // Noncontinuable exception.
        case 0xc0000026U: // Invalid disposition.
        case 0xc000008cU: // Array bounds exceeded.
        case 0xc000008dU:case 0xc000008eU:case 0xc000008fU:
        case 0xc0000090U:case 0xc0000091U:case 0xc0000092U:case 0xc0000093U:
        case 0xc0000094U: // Integer divide by zero.
        case 0xc0000095U: // Integer overflow.
        case 0xc0000096U: // Privileged instruction.
        case 0xc00000fdU: // Stack overflow.
        case 0xc0000374U: // Heap corruption, when delivered through VEH.
        case 0xc0000409U: // Fail-fast/stack-buffer-overrun, when delivered.
        case 0xc0000420U: // Assertion failure.
        case 0xc0000602U: // Fail-fast exception, when delivered.
            return true;
        default:return false; // Includes normal C++0xe06d7363 and single-step.
    }
}
inline LONG CALLBACK Observe(EXCEPTION_POINTERS* pointers){
    if(!pointers||!pointers->ExceptionRecord)return EXCEPTION_CONTINUE_SEARCH;
    const auto exception=pointers->ExceptionRecord;
    if(!ObserveCode(exception->ExceptionCode))return EXCEPTION_CONTINUE_SEARCH;
    InterlockedIncrement64(&observedCount);
    // Only the first member of an overlapping handler cohort publishes. Others
    // count a dropped sample and immediately leave. This never waits or spins,
    // including recursive exceptions, and prevents mixed concurrent records.
    if(InterlockedIncrement(&activeHandlers)!=1){
        InterlockedIncrement(&droppedCount);
        if(shared)InterlockedIncrement(HeaderWord(12));
        InterlockedDecrement(&activeHandlers);return EXCEPTION_CONTINUE_SEARCH;
    }
    FaultRecord record;
    record.uptimeMilliseconds=GetTickCount64();record.threadId=GetCurrentThreadId();
    record.code=exception->ExceptionCode;record.flags=exception->ExceptionFlags;
    record.exceptionAddress=reinterpret_cast<uintptr_t>(exception->ExceptionAddress);
    record.numberParameters=exception->NumberParameters;
    // The two values are copied from the supplied EXCEPTION_RECORD only.
    // Neither value nor ExceptionAddress/RSP is dereferenced.
    if(exception->NumberParameters>0)record.information0=exception->ExceptionInformation[0];
    if(exception->NumberParameters>1)record.information1=exception->ExceptionInformation[1];
    const auto context=pointers->ContextRecord;
    if(context){
        record.contextFlags=context->ContextFlags;
#if defined(_M_X64) || defined(__x86_64__)
        record.rip=context->Rip;record.rsp=context->Rsp;record.rcx=context->Rcx;record.rdx=context->Rdx;
        record.r8=context->R8;record.r9=context->R9;record.rax=context->Rax;record.rbx=context->Rbx;record.rbp=context->Rbp;
#elif defined(_M_IX86) || defined(__i386__)
        record.rip=context->Eip;record.rsp=context->Esp;record.rcx=context->Ecx;record.rdx=context->Edx;
        record.rax=context->Eax;record.rbx=context->Ebx;record.rbp=context->Ebp;
#else
#error original_fault_observer_requires_Windows_x86_or_x64_CONTEXT
#endif
    }
    record.sequence=static_cast<uint32_t>(Read32(&publishedCount))+1U;
    const size_t slot=(record.sequence-1U)%Capacity;
    InterlockedIncrement(&globalSequence);
    if(shared)InterlockedIncrement(HeaderWord(4));
    for(size_t i=0;i<RecordBytes/8;++i){
        LONG64 word=0;std::memcpy(&word,reinterpret_cast<const uint8_t*>(&record)+i*8,8);
        InterlockedExchange64(&records[slot][i],word);
        if(shared)InterlockedExchange64(reinterpret_cast<volatile LONG64*>(
            shared+SharedOffset+sizeof(SharedHeader)+slot*RecordBytes+i*8),word);
    }
    InterlockedExchange(&publishedCount,static_cast<LONG>(record.sequence));
    if(shared)InterlockedExchange(HeaderWord(8),static_cast<LONG>(record.sequence));
    MemoryBarrier();
    if(shared)InterlockedIncrement(HeaderWord(4));
    InterlockedIncrement(&globalSequence);
    InterlockedDecrement(&activeHandlers);
    return EXCEPTION_CONTINUE_SEARCH;
}
inline bool WritableSharedPage(uint8_t* pointer){
    if(!pointer||(reinterpret_cast<uintptr_t>(pointer)&7U))return false;
    MEMORY_BASIC_INFORMATION info{};
    if(!VirtualQuery(pointer,&info,sizeof(info))||info.State!=MEM_COMMIT
       ||(info.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
    const DWORD protection=info.Protect&0xffU;
    if(protection!=PAGE_READWRITE&&protection!=PAGE_WRITECOPY
       &&protection!=PAGE_EXECUTE_READWRITE&&protection!=PAGE_EXECUTE_WRITECOPY)return false;
    const auto at=reinterpret_cast<uintptr_t>(pointer),base=reinterpret_cast<uintptr_t>(info.BaseAddress);
    return at>=base&&info.RegionSize>=SharedBytes&&at-base<=info.RegionSize-SharedBytes;
}
inline bool Initialize(uint8_t* shared4096){
    if(registration){if(shared==shared4096)return true;initializeError=ERROR_ALREADY_EXISTS;return false;}
    if(!WritableSharedPage(shared4096)){initializeError=ERROR_INVALID_PARAMETER;return false;}
    shared=shared4096;
    // No observer is registered yet. Only the reserved400 bytes are initialized.
    std::memset(shared+SharedOffset,0,sizeof(SharedHeader)+Capacity*RecordBytes);
    registration=AddVectoredExceptionHandler(1,Observe); // Observe first; existing handlers retain their relative order.
    if(!registration){initializeError=GetLastError();if(!initializeError)initializeError=ERROR_GEN_FAILURE;return false;}
    initializeError=0;MemoryBarrier();InterlockedExchange(HeaderWord(0),static_cast<LONG>(Magic));
    return true;
}
struct Snapshot {
    bool valid=false;uint32_t sequence=0,published=0,dropped=0;uint64_t observed=0;
    FaultRecord records[Capacity]{};
};
inline Snapshot ReadSnapshot(){
    Snapshot snapshot;
    // Normal diagnostics only; a busy writer never makes this routine wait.
    for(unsigned attempt=0;attempt<8;++attempt){
        const LONG before=Read32(&globalSequence);if(before&1)continue;
        snapshot.sequence=static_cast<uint32_t>(before);
        snapshot.published=static_cast<uint32_t>(Read32(&publishedCount));
        snapshot.dropped=static_cast<uint32_t>(Read32(&droppedCount));
        snapshot.observed=static_cast<uint64_t>(Read64(&observedCount));
        for(size_t i=0;i<Capacity;++i)for(size_t j=0;j<RecordBytes/8;++j){
            const LONG64 word=Read64(&records[i][j]);
            std::memcpy(reinterpret_cast<uint8_t*>(&snapshot.records[i])+j*8,&word,8);
        }
        MemoryBarrier();const LONG after=Read32(&globalSequence);
        if(before==after&&!(after&1)){snapshot.valid=true;return snapshot;}
    }
    return snapshot;
}
inline bool WriteDiagnostics(const wchar_t* path){
    if(!path)return false;const auto snapshot=ReadSnapshot();
    FILE* file=nullptr;if(_wfopen_s(&file,path,L"wb")||!file)return false;
    std::fprintf(file,"{\"schema_version\":1,\"installed\":%s,\"initialize_error\":%lu,\"snapshot_valid\":%s,\"magic\":%u,\"shared_offset\":%zu,\"header_bytes\":%zu,\"record_bytes\":%zu,\"capacity\":%zu,\"shared_end_exclusive\":%zu,\"seqlock_sequence\":%u,\"published\":%u,\"observed\":%llu,\"dropped_concurrent\":%u,\"records\":[",
        registration?"true":"false",initializeError,snapshot.valid?"true":"false",Magic,SharedOffset,sizeof(SharedHeader),
        RecordBytes,Capacity,SharedOffset+sizeof(SharedHeader)+Capacity*RecordBytes,snapshot.sequence,snapshot.published,
        static_cast<unsigned long long>(snapshot.observed),snapshot.dropped);
    const uint32_t count=snapshot.published<Capacity?snapshot.published:static_cast<uint32_t>(Capacity);
    if(snapshot.valid)for(uint32_t i=0;i<count;++i){
        const uint32_t expected=snapshot.published-count+i+1U;
        const auto& r=snapshot.records[(expected-1U)%Capacity];if(i)std::fputc(',',file);
        std::fprintf(file,"{\"sequence\":%u,\"sequence_matches\":%s,\"uptime_ms\":%llu,\"thread_id\":%u,\"exception_code\":%u,\"exception_code_hex\":\"%08x\",\"exception_flags\":%u,\"context_flags\":%u,\"exception_address\":%llu,\"rip\":%llu,\"rsp\":%llu,\"rcx\":%llu,\"rdx\":%llu,\"r8\":%llu,\"r9\":%llu,\"rax\":%llu,\"rbx\":%llu,\"rbp\":%llu,\"number_parameters\":%u,\"exception_information_0\":%llu,\"exception_information_1\":%llu,\"breakpoint_observation\":%s,\"root_cause_established\":false}",
            r.sequence,r.sequence==expected?"true":"false",static_cast<unsigned long long>(r.uptimeMilliseconds),r.threadId,r.code,r.code,r.flags,r.contextFlags,
            static_cast<unsigned long long>(r.exceptionAddress),static_cast<unsigned long long>(r.rip),static_cast<unsigned long long>(r.rsp),
            static_cast<unsigned long long>(r.rcx),static_cast<unsigned long long>(r.rdx),static_cast<unsigned long long>(r.r8),static_cast<unsigned long long>(r.r9),
            static_cast<unsigned long long>(r.rax),static_cast<unsigned long long>(r.rbx),static_cast<unsigned long long>(r.rbp),r.numberParameters,
            static_cast<unsigned long long>(r.information0),static_cast<unsigned long long>(r.information1),r.code==0x80000003U?"true":"false");
    }
    std::fputs("],\"always_continue_search\":true,\"cpp_exceptions_excluded\":true,\"reads_stack_memory\":false,\"handler_file_io\":false,\"handler_heap_allocation\":false,\"handler_waits_or_locks\":false,\"process_lifetime_registration\":true}\n",file);
    const bool okay=std::ferror(file)==0;std::fclose(file);return okay;
}
}
