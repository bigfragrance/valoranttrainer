#pragma once
#include "original_players.h"
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>

// Scenario 13 only; ordinary actor0 remains human/non-AI. This contract never
// supplies a contact, space proof, motion, transform, health or consumption ACK.
namespace local_server_peek_fixture {
inline constexpr uint32_t Scenario=13, Version=1;
inline constexpr uint64_t Magic=0x31584b455045544cULL;
struct Contract {
    uint64_t magic=Magic;
    uint32_t version=Version,bytes=sizeof(Contract),scenario=Scenario,kind=0,variant=0,scene=0,candidate=0,flags=1;
    uint64_t nonce=0,initialHash=0,bankHash=0,caseHash=0,stage0Hash=0,stage1Hash=0,commitHash=0;
    uint32_t initialBytes=7844,bankBytes=0,revision=0,seed=3;
    double sut[3]{},target[3]{},targetYaw=0,sutYaw=0,arrivalCm=0;
    // SHA256s are validated and archived by the ordinary owned-pair runner.
    // Native gates additionally compare exact file bytes through bounded FNV.
    uint8_t imageSha256[32]{},moduleSha256[32]{},caseSha256[32]{},initialSha256[32]{},bankSha256[32]{};
};
static_assert(sizeof(Contract)==344&&offsetof(Contract,nonce)==40&&offsetof(Contract,sut)==112);
struct Control {
    uint64_t magic=Magic,nonce=0;uint32_t bytes=32,action=0;uint64_t sequence=0;
};
static_assert(sizeof(Control)==32);
struct Context {
    bool initialized=false,configured=false,armed=false,failed=false,host=false;
    uint32_t error=0;const char*stage="peek_fixture_disabled";
    Contract contract{};std::wstring directory,parent,signalPath;
    uint64_t profileFingerprints[2]{};
};
inline Context context;
inline uint64_t Hash(const void*pointer,size_t size){
    auto bytes=static_cast<const uint8_t*>(pointer);uint64_t value=14695981039346656037ULL;
    for(size_t n=0;n<size;++n)value=(value^bytes[n])*1099511628211ULL;return value;
}
inline bool Fail(uint32_t error,const char*stage){context.failed=true;context.error=error;context.stage=stage;return false;}
inline bool ReadFileBytes(const std::wstring&path,std::vector<uint8_t>&out,size_t limit){
    FILE*file=nullptr;if(_wfopen_s(&file,path.c_str(),L"rb")||!file)return false;
    const bool seek=std::fseek(file,0,SEEK_END)==0;const long size=seek?std::ftell(file):-1;
    if(size<0||static_cast<size_t>(size)>limit||std::fseek(file,0,SEEK_SET)!=0){std::fclose(file);return false;}
    out.resize(static_cast<size_t>(size));const auto read=std::fread(out.data(),1,out.size(),file);const auto closed=std::fclose(file);
    return read==out.size()&&closed==0;
}
inline bool FileMatches(const std::wstring&path,uint64_t expected,size_t size){
    std::vector<uint8_t>bytes;return expected&&ReadFileBytes(path,bytes,size)&&bytes.size()==size&&Hash(bytes.data(),bytes.size())==expected;
}
inline bool Init(const wchar_t*directory,uint32_t role){
    if(context.initialized)return !context.failed;context.initialized=true;context.host=role==1;
    if(!directory||role<1||role>2)return Fail(7820,"peek_fixture_bad_role_or_directory");
    std::wstring path(directory);if(path.empty())return true;
    if(path.back()==L'\\'||path.back()==L'/')path.pop_back();
    const auto split=path.find_last_of(L"\\/");if(split==std::wstring::npos)return true;
    const auto leaf=path.substr(split+1);if(leaf!=(role==1?L"host":L"client"))return true;
    context.directory=path+L"\\";context.parent=path.substr(0,split+1);
    const auto prefix=std::wstring(L"F:\\valoranttrainer\\.local\\local-server\\pair-");
    std::vector<uint8_t>bytes;
    if(!ReadFileBytes(context.directory+L"peek-fixture-contract.bin",bytes,sizeof(Contract)))return true;
    if(path.rfind(prefix,0)!=0||path.find(L"..")!=std::wstring::npos||bytes.size()!=sizeof(Contract))
        return Fail(7821,"peek_fixture_requires_exact_owned_F_pair_contract");
    Contract contract;std::memcpy(&contract,bytes.data(),sizeof(contract));
    if(contract.magic!=Magic||contract.version!=Version||contract.bytes!=sizeof(Contract)||contract.scenario!=Scenario
       ||contract.kind<1||contract.kind>8||contract.variant>1||contract.scene<1||contract.scene>3||contract.candidate>7
       ||contract.flags!=1||!contract.nonce||contract.initialBytes!=7844||contract.seed!=3||!contract.revision
       ||!std::isfinite(contract.arrivalCm)||contract.arrivalCm<=0||!std::isfinite(contract.targetYaw)||!std::isfinite(contract.sutYaw))
        return Fail(7822,"peek_fixture_contract_schema_invalid");
    for(size_t n=0;n<3;++n)if(!std::isfinite(contract.sut[n])||!std::isfinite(contract.target[n]))
        return Fail(7822,"peek_fixture_contract_schema_invalid");
    if(!FileMatches(context.parent+L"session-initial.bin",contract.initialHash,contract.initialBytes)
       ||!FileMatches(context.parent+L"session-human-bots.bin",contract.bankHash,contract.bankBytes))
        return Fail(7823,"peek_fixture_exact_initial_or_profile_bank_changed");
    std::vector<uint8_t>initial,bank;
    if(!ReadFileBytes(context.parent+L"session-initial.bin",initial,7844)
       ||!ReadFileBytes(context.parent+L"session-human-bots.bin",bank,40000)||bank.size()<128)
        return Fail(7823,"peek_fixture_exact_initial_or_profile_bank_changed");
    const auto word=[](const std::vector<uint8_t>&v,size_t at){uint32_t r=0;std::memcpy(&r,v.data()+at,4);return r;};
    // The normal Initial/Store parsers remain the full format/checksum authority.
    if(word(initial,20)!=1||word(initial,24)!=2||(word(initial,28)&14)!=0||word(initial,84)!=contract.seed
       ||word(bank,24)!=contract.revision||word(bank,28)!=contract.seed||word(bank,32)!=1||word(bank,36)!=2)
        return Fail(7824,"peek_fixture_requires_two_person_unpaused_finite_original_deathmatch");
    if(std::memcmp(initial.data()+6752,"ascent\0",7)!=0||word(initial,6816)!=2)
        return Fail(7824,"peek_fixture_requires_exact_ascent_two_original_spawn_anchors");
    const auto recordBytes=word(bank,44);
    if(recordBytes<16||bank.size()!=128+2*static_cast<size_t>(recordBytes))return Fail(7823,"peek_fixture_profile_record_shape_changed");
    for(size_t n=0;n<2;++n)std::memcpy(&context.profileFingerprints[n],bank.data()+128+n*recordBytes+8,8);
    if(contract.stage0Hash||contract.stage1Hash||contract.commitHash)return Fail(7822,"peek_fixture_unexpected_reserved_payload");
    context.contract=contract;context.signalPath=context.parent+L"peek-fixture-control.bin";
    context.configured=true;context.stage="peek_fixture_waiting_matching_original_command_line";return true;
}
inline bool Token(const wchar_t*command,const wchar_t*token){
    if(!command)return false;const size_t length=std::wcslen(token);size_t total=0;
    while(total<32768&&command[total])++total;if(total==32768)return false;
    unsigned matches=0;for(size_t n=0;n+length<=total;++n){
        if(n&&command[n-1]!=L' '&&command[n-1]!=L'\t')continue;
        if(std::wcsncmp(command+n,token,length)!=0)continue;const auto end=command[n+length];
        if(!end||end==L' '||end==L'\t')++matches;
    }return matches==1;
}
inline bool Arm(){
    if(!context.configured||context.failed)return !context.failed;if(context.armed)return true;
    if(!original_players::Thread()||!engine::image)return true;
    const auto commandGetter=reinterpret_cast<const void*>(engine::image+0x19d0320);
    if(!original_players::Readable(commandGetter,44)||Hash(commandGetter,44)!=0xaa94462619c48c39ULL)
        return Fail(7827,"peek_fixture_original_command_line_getter_profile_changed");
    if(!original_players::Value<uint8_t>(reinterpret_cast<void*>(engine::image),0xd958b90))return true;
    const auto command=engine::function<const wchar_t*(*)()>(0x19d0320)();wchar_t token[96]{};
    swprintf_s(token,L"-LocalServerPeekFixture=%016llx",static_cast<unsigned long long>(context.contract.nonce));
    if(!Token(command,token))return Fail(7826,"peek_fixture_requires_matching_explicit_original_cli_nonce");
    context.armed=true;context.stage="peek_fixture_explicit_contract_armed";return true;
}
inline bool ControlValid(const Control&value){return value.magic==Magic&&value.bytes==sizeof(Control)
    &&value.nonce==context.contract.nonce&&value.action<=2&&value.sequence>0;}
inline bool ReadControl(Control&value){
    std::vector<uint8_t>bytes;if(!ReadFileBytes(context.signalPath,bytes,sizeof(Control))||bytes.size()!=sizeof(Control))return false;
    std::memcpy(&value,bytes.data(),sizeof(value));return ControlValid(value);
}
} // namespace local_server_peek_fixture
