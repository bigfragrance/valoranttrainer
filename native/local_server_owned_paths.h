#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include "local_server_package_paths.h"

namespace local_server_owned_paths {
inline bool CanonicalAbsolute(const std::wstring& input,std::wstring& output){
    output.clear();if(input.size()>32000)return false;
    auto candidate=input;
    while(candidate.size()>3&&(candidate.back()==L'\\'||candidate.back()==L'/'))candidate.pop_back();
    if(!local_server_package_paths::Absolute(local_server_package_paths::Normalize(candidate)))return false;
    wchar_t full[32768]{};const auto count=GetFullPathNameW(candidate.c_str(),32768,full,nullptr);
    if(!count||count>=32768)return false;
    output.assign(full,count);for(auto& character:output)if(character==L'/')character=L'\\';
    return local_server_package_paths::Absolute(local_server_package_paths::Normalize(output));
}
inline bool RoleDirectory(const std::wstring& directory,uint32_t role){
    HMODULE owner=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCWSTR>(&RoleDirectory),&owner))return false;
    wchar_t library[32768]{},image[32768]{};
    const auto libraryLength=GetModuleFileNameW(owner,library,32768),imageLength=GetModuleFileNameW(nullptr,image,32768);
    return libraryLength&&libraryLength<32768&&imageLength&&imageLength<32768
        &&local_server_package_paths::RoleDirectory(std::wstring(library,libraryLength),std::wstring(image,imageLength),directory,role);
}
}
