#pragma once
#include <cstdint>
#include <cwctype>
#include <string>

// Bind runtime files to this loaded package and its own client image. A
// movable patch keeps the same pair/role boundary as the development checkout.
namespace local_server_package_paths {
inline std::wstring Normalize(std::wstring value){
    for(auto& character:value){if(character==L'/')character=L'\\';character=static_cast<wchar_t>(std::towlower(character));}
    return value;
}
inline bool Absolute(const std::wstring& value){
    if(value.size()<4||value[1]!=L':'||value[2]!=L'\\'
       ||!((value[0]>=L'a'&&value[0]<=L'z')||(value[0]>=L'A'&&value[0]<=L'Z')))return false;
    size_t begin=3;
    for(size_t n=3;n<=value.size();++n){
        if(n<value.size()&&value[n]!=L'\\'){
            if(value[n]<32||value[n]==L':'||value[n]==L'"'||value[n]==L'<'||value[n]==L'>'
               ||value[n]==L'|'||value[n]==L'?'||value[n]==L'*')return false;
            continue;
        }
        const auto part=value.substr(begin,n-begin);
        if(part.empty()||part==L"."||part==L".."||part.back()==L'.'||part.back()==L' ')return false;
        begin=n+1;
    }
    return true;
}
// The role directory must first pass RoleDirectory for this loaded package.
// Bind the early startup report to that same directory on any local drive.
inline bool ModeStartupReport(std::wstring directory,std::wstring report){
    directory=Normalize(directory);report=Normalize(report);
    if(!directory.empty()&&directory.back()==L'\\')directory.pop_back();
    return Absolute(directory)&&Absolute(report)&&report==directory+L"\\mode-startup-state.json";
}
inline bool RoleDirectory(std::wstring module,std::wstring image,std::wstring directory,uint32_t role){
    if(role!=1&&role!=2)return false;
    module=Normalize(module);image=Normalize(image);directory=Normalize(directory);
    if(!directory.empty()&&directory.back()==L'\\')directory.pop_back();
    if(!Absolute(module)||!Absolute(image)||!Absolute(directory))return false;
    const std::wstring suffix=L"\\.local\\local-server\\module\\local_server_bridge.dll";
    if(module.size()<=suffix.size()||module.compare(module.size()-suffix.size(),suffix.size(),suffix)!=0)return false;
    const auto root=module.substr(0,module.size()-suffix.size());
    if(image!=root+L"\\client\\shootergame\\binaries\\win64\\valorant-win64-shipping.exe")return false;
    const auto prefix=root+L"\\.local\\local-server\\pair-";
    if(directory.rfind(prefix,0)!=0)return false;
    const auto split=directory.find(L'\\',prefix.size());
    if(split==std::wstring::npos||split==prefix.size()
       ||directory.substr(split+1)!=(role==1?L"host":L"client"))return false;
    const auto pair=directory.substr(prefix.size(),split-prefix.size());
    if(pair.find(L"..")!=std::wstring::npos)return false;
    for(const auto character:pair)if(!((character>=L'a'&&character<=L'z')||(character>=L'0'&&character<=L'9')
                                      ||character==L'-'||character==L'_'||character==L'.'))return false;
    return true;
}
}
