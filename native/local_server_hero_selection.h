#pragma once
#include "original_heroes_catalog.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

// Immutable, per-pair local selection. The sidecar contains only a catalog
// index/FGuid and its pinned profile identity, never a caller-selected UObject
// path. The V2 control protocol and original game resource files are unchanged.
namespace local_server_hero_selection {
using Guid=original_content::Guid;
inline constexpr uint32_t Version=1,WireBytes=72;
inline constexpr char Magic[8]{'V','T','H','E','R','O','0','1'};
inline constexpr std::array<uint8_t,32> ProfileSha256{
    0x8a,0x92,0x1b,0x08,0xe6,0x33,0x6a,0xd0,0x48,0x99,0x9f,0xe4,0xf3,0x86,0x4f,0xa3,
    0x09,0xe5,0xf3,0x15,0x08,0xef,0x5b,0xa4,0xdf,0xbd,0x0d,0xd5,0xef,0x10,0x48,0xc3};
inline constexpr const char* ProfileSha256Text="8a921b08e6336ad048999fe4f3864fa309e5f31508ef5ba4dfbd0dd5ef1048c3";
struct Wire {
    char magic[8]{};
    uint32_t version=0,bytes=0,revision=0,heroIndex=0;
    Guid id{};
    std::array<uint8_t,32> profile{};
};
static_assert(sizeof(Wire)==WireBytes&&offsetof(Wire,version)==8&&offsetof(Wire,bytes)==12
    &&offsetof(Wire,revision)==16&&offsetof(Wire,heroIndex)==20&&offsetof(Wire,id)==24&&offsetof(Wire,profile)==40);
static_assert(original_heroes::PlayableHeroCount==30);
struct State {
    bool loaded=false,configured=false,sidecarPresent=false,validated=false;
    uint32_t revision=0,heroIndex=0,bytesRead=0,error=0,ioError=0;
    const char* stage="local_hero_selection_not_loaded";
};
inline State state{};
inline std::wstring sourcePath;
inline const State& GetState(){return state;}
inline bool Ready(){return state.loaded&&!state.error;}
inline uint32_t SelectionIndex(){return state.heroIndex;}
inline const wchar_t* SelectionPath(){return original_heroes::Paths[state.heroIndex];}
inline const Guid& SelectionGuid(){return original_heroes::Catalog[state.heroIndex].id;}
inline bool SameGuid(const Guid& a,const Guid& b){return std::memcmp(&a,&b,sizeof(Guid))==0;}
inline bool Fail(uint32_t error,const char* stage){state.error=error;state.stage=stage;return false;}
inline uint32_t Validate(const Wire& candidate,uint32_t revision){
    if(std::memcmp(candidate.magic,Magic,sizeof(Magic)))return 6205;
    if(candidate.version!=Version)return 6206;
    if(candidate.bytes!=WireBytes)return 6207;
    if(candidate.revision!=revision)return 6208;
    if(candidate.heroIndex>=original_heroes::PlayableHeroCount)return 6209;
    if(!SameGuid(candidate.id,original_heroes::Catalog[candidate.heroIndex].id))return 6210;
    if(candidate.profile!=ProfileSha256)return 6211;
    return 0;
}
inline bool Load(const std::wstring& exactSidecar,bool enabled,uint32_t revision){
    if(state.error)return false;
    if(state.loaded)return state.configured==enabled&&state.revision==revision
        &&(!enabled||sourcePath==exactSidecar)
        ?true:Fail(6200,"local_hero_selection_pair_configuration_changed");
    state.configured=enabled;state.revision=revision;
    if(!enabled){
        state.heroIndex=0;state.loaded=true;state.stage="local_hero_selection_basic_probe_default_jett";return true;
    }
    if(exactSidecar.empty()||exactSidecar.size()>32000||exactSidecar.find(L'\0')!=std::wstring::npos)
        return Fail(6201,"local_hero_selection_exact_sidecar_path_invalid");
    sourcePath=exactSidecar;
    const auto file=CreateFileW(exactSidecar.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){state.ioError=GetLastError();return Fail(6202,"local_hero_selection_configured_sidecar_missing_or_unreadable");}
    state.sidecarPresent=true;Wire candidate{};LARGE_INTEGER size{};DWORD received=0;
    const bool sized=GetFileSizeEx(file,&size)&&size.QuadPart==static_cast<LONGLONG>(WireBytes);
    const bool read=sized&&ReadFile(file,&candidate,WireBytes,&received,nullptr)&&received==WireBytes;
    if(!read)state.ioError=GetLastError();state.bytesRead=received;
    CloseHandle(file);
    if(!sized)return Fail(6203,"local_hero_selection_requires_exact_72_byte_sidecar");
    if(!read)return Fail(6204,"local_hero_selection_exact_sidecar_read_failed");
    if(const auto error=Validate(candidate,revision))return Fail(error,"local_hero_selection_pinned_catalog_identity_mismatch");
    state.heroIndex=candidate.heroIndex;state.validated=state.loaded=true;
    state.stage="local_hero_selection_original_catalog_entry_selected";return true;
}
inline void Write(FILE* file){
    if(!file)return;const auto& entry=original_heroes::Catalog[state.heroIndex];
    const auto yes=[](bool value){return value?"true":"false";};
    std::fprintf(file,"{\"loaded\":%s,\"configured\":%s,\"sidecar_present\":%s,\"validated\":%s,\"revision\":%u,\"hero_index\":%u,\"hero_code\":\"%s\",\"hero_id\":\"%08x%08x%08x%08x\",\"profile_sha256\":\"%s\",\"bytes_read\":%u,\"error\":%u,\"io_error\":%u,\"stage\":\"%s\"}",
        yes(state.loaded),yes(state.configured),yes(state.sidecarPresent),yes(state.validated),state.revision,state.heroIndex,entry.code,
        entry.id.words[0],entry.id.words[1],entry.id.words[2],entry.id.words[3],ProfileSha256Text,state.bytesRead,state.error,state.ioError,state.stage);
}
} // namespace local_server_hero_selection
