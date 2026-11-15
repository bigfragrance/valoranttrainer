#pragma once
#include "human_bot_store.h"
#include <vector>

// The caller derives this filename only from the same validated pair folder
// that supplied session-initial.bin. No channel PID/process lookup is added.
namespace human_bot_initial {
inline constexpr const wchar_t* Filename=L"session-human-bots.bin";
struct LoadResult {bool okay=false,present=false;DWORD error=0;const char* stage="human_bot_initial_not_attempted";};
inline LoadResult result;
struct OwnedFile {HANDLE value=INVALID_HANDLE_VALUE;~OwnedFile(){if(value!=INVALID_HANDLE_VALUE)CloseHandle(value);}};
inline bool Load(const std::wstring& initialPath,uint32_t count,uint32_t revision,uint32_t seed){
    result={};
    const auto at=initialPath.find_last_of(L"\\/");
    if(at==std::wstring::npos||initialPath.substr(at+1)!=L"session-initial.bin"){
        result.stage="human_bot_initial_requires_same_validated_initial_path";return false;
    }
    const auto path=initialPath.substr(0,at+1)+Filename;
    const HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){
        result.error=GetLastError();
        if(result.error!=ERROR_FILE_NOT_FOUND){result.stage="human_bot_initial_sidecar_open_failed";return false;}
        result.okay=human_bot_store::store.Legacy(count,revision,seed);
        result.stage=result.okay?"human_bot_legacy_sidecar_absent_no_new_consumer_claim":"human_bot_legacy_store_initialization_rejected";
        human_bot_store::CacheSummary();return result.okay;
    }
    OwnedFile owner{file};result.present=true;LARGE_INTEGER size{};DWORD received=0;
    const bool sized=GetFileSizeEx(file,&size)!=0;
    if(!sized||size.QuadPart<static_cast<LONGLONG>(human_bot_wire::HeaderBytes)
       ||size.QuadPart>static_cast<LONGLONG>(human_bot_wire::MaximumInitialBytes)){
        result.error=sized?ERROR_BAD_LENGTH:GetLastError();result.stage="human_bot_initial_sidecar_length_invalid";return false;
    }
    std::vector<uint8_t> bytes(static_cast<size_t>(size.QuadPart));
    const bool read=ReadFile(file,bytes.data(),static_cast<DWORD>(bytes.size()),&received,nullptr)!=0&&received==bytes.size();
    if(!read)result.error=GetLastError();
    if(!read){result.stage="human_bot_initial_sidecar_read_failed";return false;}
    const auto reply=human_bot_store::store.Initial(bytes.data(),static_cast<uint32_t>(bytes.size()),count,revision,seed);
    result.okay=reply.outcome==human_bot_store::Outcome::Pending;result.stage=reply.message;
    human_bot_store::CacheSummary();return result.okay;
}
} // namespace human_bot_initial
