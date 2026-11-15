#pragma once
#include "original_players.h"
#include "map_hold_fixture_core.h"
#include <array>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>

namespace local_server_hold_fixture {
namespace core=map_hold_fixture_core;
inline constexpr uint32_t Scenario=14;
inline constexpr uint64_t Magic=0x3431444c4f48544cULL;
struct Contract {
    uint64_t magic=Magic;
    uint32_t version=1,bytes=sizeof(Contract),scenario=Scenario,annotation=0,annotationRevision=0,revision=0,seed=3,flags=1;
    uint64_t nonce=0,initialHash=0,bankHash=0,caseHash=0;
    uint32_t initialBytes=7844,bankBytes=0;
    double sut[3]{},human[3]{},sutYaw=0,arrival=18,dwell=2.5,total=40;
    uint8_t imageSha256[32]{},moduleSha256[32]{},caseSha256[32]{};
};
static_assert(sizeof(Contract)==256&&offsetof(Contract,nonce)==40&&offsetof(Contract,sut)==80);
struct Completion {uint64_t magic=Magic,nonce=0;uint32_t bytes=32,verdict=0;uint64_t publication=0;};
static_assert(sizeof(Completion)==32);
struct Context {
    bool initialized=false,configured=false,host=false,armed=false,failed=false;
    uint32_t error=0;const char* stage="map_hold_fixture_disabled";
    Contract contract{};std::wstring directory,parent;
    uint64_t fingerprints[2]{};
};
inline Context context{};
inline std::atomic<bool> stopRequested{false};
inline uint64_t Hash(const void* p,size_t bytes){uint64_t h=14695981039346656037ULL;
    const auto data=static_cast<const uint8_t*>(p);for(size_t n=0;n<bytes;++n)h=(h^data[n])*1099511628211ULL;return h;}
inline bool Fail(uint32_t error,const char* stage){context.failed=true;context.error=error;context.stage=stage;return false;}
inline bool File(const std::wstring& path,std::vector<uint8_t>& out,size_t maximum){
    FILE* f=nullptr;if(_wfopen_s(&f,path.c_str(),L"rb")||!f)return false;
    const bool seek=std::fseek(f,0,SEEK_END)==0;const long bytes=seek?std::ftell(f):-1;
    if(bytes<0||static_cast<size_t>(bytes)>maximum||std::fseek(f,0,SEEK_SET)){std::fclose(f);return false;}
    out.resize(static_cast<size_t>(bytes));const auto read=std::fread(out.data(),1,out.size(),f);const int closed=std::fclose(f);
    return read==out.size()&&closed==0;
}
inline bool Init(const wchar_t* directory,uint32_t role){
    if(context.initialized)return !context.failed;context.initialized=true;context.host=role==1;
    if(!directory||role<1||role>2)return Fail(7860,"map_hold_bad_role_or_directory");
    std::wstring path(directory);if(path.empty())return true;if(path.back()==L'\\'||path.back()==L'/')path.pop_back();
    const auto at=path.find_last_of(L"\\/");if(at==std::wstring::npos)return true;
    if(path.substr(at+1)!=(role==1?L"host":L"client"))return true;
    context.directory=path+L"\\";context.parent=path.substr(0,at+1);
    std::vector<uint8_t> bytes;if(!File(context.directory+L"map-hold-fixture-contract.bin",bytes,sizeof(Contract)))return true;
    if(path.rfind(L"F:\\valoranttrainer\\.local\\local-server\\pair-",0)!=0||path.find(L"..")!=std::wstring::npos||bytes.size()!=sizeof(Contract))
        return Fail(7861,"map_hold_requires_exact_owned_F_pair_contract");
    Contract c;std::memcpy(&c,bytes.data(),sizeof(c));
    if(c.magic!=Magic||c.version!=1||c.bytes!=sizeof(c)||c.scenario!=Scenario||(c.annotation!=1301&&c.annotation!=1302&&c.annotation!=1303&&c.annotation!=1304)
       ||!c.annotationRevision||!c.revision||c.seed!=3||c.flags!=1||!c.nonce||!c.initialHash||!c.bankHash||!c.caseHash
       ||c.initialBytes!=7844||c.bankBytes<128||c.bankBytes>40000||!std::isfinite(c.sutYaw)
       ||!std::isfinite(c.arrival)||c.arrival<=0||!std::isfinite(c.dwell)||c.dwell<=0||c.dwell>10||c.total!=40)
        return Fail(7862,"map_hold_contract_schema_invalid");
    for(size_t n=0;n<3;++n)if(!std::isfinite(c.sut[n])||!std::isfinite(c.human[n]))return Fail(7862,"map_hold_nonfinite_spawn_proposal");
    std::vector<uint8_t> initial,bank,manifest;
    if(!File(context.parent+L"session-initial.bin",initial,c.initialBytes)||initial.size()!=c.initialBytes||Hash(initial.data(),initial.size())!=c.initialHash
       ||!File(context.parent+L"session-human-bots.bin",bank,c.bankBytes)||bank.size()!=c.bankBytes||Hash(bank.data(),bank.size())!=c.bankHash
       ||!File(context.parent+L"map-hold-fixture-case.json",manifest,40000)||Hash(manifest.data(),manifest.size())!=c.caseHash)
        return Fail(7863,"map_hold_exact_initial_profile_or_case_changed");
    const auto word=[](const auto& data,size_t offset){uint32_t value=0;std::memcpy(&value,data.data()+offset,4);return value;};
    if(word(initial,20)!=1||word(initial,24)!=2||(word(initial,28)&14)||word(initial,84)!=3
       ||std::memcmp(initial.data()+6752,"ascent\0",7)||word(initial,6816)!=2
       ||word(bank,24)!=c.revision||word(bank,28)!=3||word(bank,32)!=1||word(bank,36)!=2)
        return Fail(7864,"map_hold_requires_two_person_unpaused_finite_original_ascent");
    const auto record=word(bank,44);if(record<16||bank.size()!=128+2*static_cast<size_t>(record))return Fail(7863,"map_hold_profile_shape_changed");
    for(size_t n=0;n<2;++n)std::memcpy(&context.fingerprints[n],bank.data()+128+n*record+8,8);
    context.contract=c;context.configured=true;context.stage="map_hold_waiting_matching_original_CLI";return true;
}
inline bool Token(const wchar_t* command,const wchar_t* token){
    if(!command)return false;const size_t length=std::wcslen(token);size_t count=0;while(count<32768&&command[count])++count;if(count==32768)return false;
    unsigned matches=0;for(size_t n=0;n+length<=count;++n){
        if(n&&command[n-1]!=L' '&&command[n-1]!=L'\t')continue;if(std::wcsncmp(command+n,token,length))continue;
        const auto end=command[n+length];if(!end||end==L' '||end==L'\t')++matches;}
    return matches==1;
}
inline bool Arm(){
    if(!context.configured||context.failed)return !context.failed;if(context.armed)return true;
    if(!original_players::Thread()||!engine::image)return true;
    const auto getter=reinterpret_cast<const void*>(engine::image+0x19d0320);
    if(!original_players::Readable(getter,44)||Hash(getter,44)!=0xaa94462619c48c39ULL)return Fail(7865,"map_hold_original_command_getter_profile_changed");
    if(!original_players::Value<uint8_t>(reinterpret_cast<void*>(engine::image),0xd958b90))return true;
    wchar_t token[96]{};swprintf_s(token,L"-LocalServerMapHoldFixture=%016llx",static_cast<unsigned long long>(context.contract.nonce));
    if(!Token(engine::function<const wchar_t*(*)()>(0x19d0320)(),token))return Fail(7866,"map_hold_requires_matching_explicit_CLI_nonce");
    context.armed=true;context.stage="map_hold_explicit_original_spawn_fixture_armed";return true;
}
inline bool ReadCompletion(Completion& out){
    std::vector<uint8_t> bytes;if(!File(context.parent+L"map-hold-fixture-complete.bin",bytes,sizeof(out))||bytes.size()!=sizeof(out))return false;
    std::memcpy(&out,bytes.data(),sizeof(out));return out.magic==Magic&&out.nonce==context.contract.nonce&&out.bytes==sizeof(out)
        &&out.verdict>=1&&out.verdict<=2&&out.publication;
}
} // namespace local_server_hold_fixture
