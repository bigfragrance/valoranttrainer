#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include "local_server_bot_roster_wire.h"
#include "local_server_hero_selection.h"
#include "local_server_package_paths.h"
#include <string>

// Host-only immutable per-pair roster. Only the sibling of the already
// validated Initial is opened; all native hero/spawn/respawn work stays in the
// original match path. The file is never reread during gameplay.
namespace local_server_bot_roster_initial {
namespace wire=local_server_bot_roster_wire;
inline constexpr const wchar_t* Filename=L"session-bot-roster.bin";
struct State {
    bool loaded=false,required=false,sidecarPresent=false,validated=false,nativeCatalogVerified=false;
    uint32_t error=0,ioError=0,bytesRead=0;wire::Context context{};wire::Plan plan{};
    const char*stage="fixed_bot_roster_not_loaded";
};
inline State state{};inline std::wstring sourcePath;
inline const State& GetState(){return state;}
inline bool Fixed(){return state.loaded&&state.validated&&state.plan.valid&&!state.error;}
inline bool Fail(uint32_t error,const char*stage){state.error=error;state.stage=stage;return false;}
inline wire::Guid CatalogGuid(const original_content::Guid&id){wire::Guid out;for(size_t n=0;n<4;++n)out.words[n]=id.words[n];return out;}
inline bool CatalogMatch(uint32_t hero,const wire::Guid&id){return hero<original_heroes::PlayableHeroCount&&wire::Same(id,CatalogGuid(original_heroes::Catalog[hero].id));}
static_assert(wire::HeroCount==original_heroes::PlayableHeroCount);
inline wire::Context Context(const void*initial,size_t bytes,uint32_t total,uint32_t revision,uint32_t seed){
    wire::Context out;out.revision=revision;out.seed=seed;out.totalPlayers=total;out.initialFnv=wire::Hash(initial,bytes);
    out.catalogProfile=local_server_hero_selection::ProfileSha256;return out;
}
inline const wire::Row* Entry(size_t slot){return Fixed()?wire::Entry(state.plan,slot):nullptr;}
inline bool Load(const std::wstring&initialPath,const void*initial,size_t bytes,uint32_t total,uint32_t revision,uint32_t seed,bool required){
    if(state.error)return false;
    if(!initial||bytes!=7844)return Fail(9030,"fixed_bot_roster_requires_exact_initial_bytes");
    const auto context=Context(initial,bytes,total,revision,seed);const auto at=initialPath.find_last_of(L"\\/");
    if(!local_server_package_paths::Absolute(local_server_package_paths::Normalize(initialPath))
       ||at==std::wstring::npos||initialPath.substr(at+1)!=L"session-initial.bin")return Fail(9031,"fixed_bot_roster_initial_sibling_path_invalid");
    const auto path=initialPath.substr(0,at+1)+Filename;
    if(state.loaded)return required==state.required&&wire::Same(context,state.context)&&sourcePath==path
        ?true:Fail(9032,"fixed_bot_roster_pair_configuration_is_immutable");
    state.required=required;state.context=context;sourcePath=path;
    const HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){
        state.ioError=GetLastError();
        if(state.ioError!=ERROR_FILE_NOT_FOUND)return Fail(9033,"fixed_bot_roster_sidecar_unreadable");
        if(required)return Fail(9034,"fixed_bot_roster_required_sidecar_missing");
        state.loaded=true;state.stage="fixed_bot_roster_legacy_sidecar_absent";return true;
    }
    state.sidecarPresent=true;std::array<uint8_t,wire::MaximumBytes>data{};LARGE_INTEGER size{};DWORD received=0;
    const bool sized=GetFileSizeEx(file,&size)&&size.QuadPart>=wire::HeaderBytes&&size.QuadPart<=static_cast<LONGLONG>(data.size());
    const bool read=sized&&ReadFile(file,data.data(),static_cast<DWORD>(size.QuadPart),&received,nullptr)&&received==size.QuadPart;
    if(!sized)state.ioError=ERROR_BAD_LENGTH;else if(!read)state.ioError=GetLastError();CloseHandle(file);state.bytesRead=received;
    if(!sized||!read)return Fail(9035,"fixed_bot_roster_sidecar_shape_or_read_failed");
    wire::Plan candidate{};const auto error=wire::Parse(data.data(),received,context,CatalogMatch,candidate);
    if(error!=wire::Error::None)return Fail(static_cast<uint32_t>(error),"fixed_bot_roster_sidecar_contract_rejected");
    state.plan=candidate;state.loaded=state.validated=true;state.ioError=0;
    state.stage="fixed_bot_roster_catalog_rows_validated_native_assignment_pending";return true;
}
inline bool Matches(const void*initial,size_t bytes,uint32_t total,uint32_t revision,uint32_t seed){
    const auto&human=local_server_hero_selection::GetState();
    return Fixed()&&human.loaded&&human.configured&&human.validated&&!human.error
        &&wire::Matches(state.plan,Context(initial,bytes,total,revision,seed),local_server_hero_selection::SelectionIndex(),CatalogGuid(local_server_hero_selection::SelectionGuid()));
}
inline void NativeCatalogVerified(){if(Fixed()){state.nativeCatalogVerified=true;state.stage="fixed_bot_roster_native_catalog_verified_assignment_fixed";}}
template<class Json>inline void Token(Json&out,uint64_t token){if(token)out.Add("\"%016llx\"",static_cast<unsigned long long>(token));else out.Add("null");}
template<class Json>inline void Guid(Json&out,const wire::Guid&id){out.Add("\"%08x%08x%08x%08x\"",id.words[0],id.words[1],id.words[2],id.words[3]);}
template<class Json>inline void PlayerFields(Json&out,size_t slot,bool human,int32_t id,bool assigned){
    out.Add(",\"entry_token\":");const auto*entry=human?nullptr:Entry(slot);Token(out,entry?entry->entryToken:0);
    out.Add(",\"native_player_id\":");if(assigned&&id>0)out.Add("%d",id);else out.Add("null");
}
template<class Json,class PlayerId>inline void WriteJson(Json&out,PlayerId playerId){
    const auto b=[](bool value){return value?"true":"false";};const auto&s=state;
    out.Add("{\"loaded\":%s,\"required\":%s,\"sidecar_present\":%s,\"fixed\":%s,\"validated\":%s,\"native_catalog_verified\":%s,\"revision\":%u,\"seed\":%u,\"count\":%u,\"human_hero_index\":",
        b(s.loaded),b(s.required),b(s.sidecarPresent),b(Fixed()),b(s.validated),b(s.nativeCatalogVerified),s.context.revision,s.context.seed,Fixed()?s.plan.header.botCount:0);
    if(Fixed())out.Add("%u",s.plan.header.humanHeroIndex);else out.Add("null");
    out.Add(",\"human_hero_id\":");if(Fixed())Guid(out,s.plan.header.humanId);else out.Add("null");
    out.Add(",\"catalog_profile_sha256\":\"%s\",\"initial_fnv\":\"%016llx\",\"rows_fnv\":",local_server_hero_selection::ProfileSha256Text,static_cast<unsigned long long>(s.context.initialFnv));
    if(Fixed())out.Add("\"%016llx\"",static_cast<unsigned long long>(s.plan.header.rowsFnv));else out.Add("null");
    out.Add(",\"error\":%u,\"io_error\":%u,\"stage\":",s.error,s.ioError);out.String(s.stage);out.Add(",\"entries\":[");
    if(Fixed())for(uint32_t slot=1;slot<s.context.totalPlayers;++slot){const auto*entry=Entry(slot);if(slot>1)out.Add(",");
        out.Add("{\"slot\":%u,\"hero_index\":%u,\"hero_id\":",slot,entry->heroIndex);Guid(out,entry->id);
        out.Add(",\"entry_token\":");Token(out,entry->entryToken);out.Add(",\"native_player_id\":");
        const auto id=playerId(slot);if(id>0)out.Add("%d",id);else out.Add("null");out.Add("}");
    }out.Add("]}");
}
} // namespace local_server_bot_roster_initial
