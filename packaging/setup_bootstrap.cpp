#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <ShlObj.h>
#include <shellapi.h>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static std::wstring Quote(const std::wstring& value) {
    std::wstring result=L"\"";
    size_t slashes=0;
    for (const auto character:value) {
        if (character==L'\\') { ++slashes; continue; }
        if (character==L'\"') result.append(slashes*2+1,L'\\');
        else result.append(slashes,L'\\');
        result+=character;
        slashes=0;
    }
    result.append(slashes*2,L'\\');
    return result+L"\"";
}

static bool PlainDirectory(const fs::path& directory) {
    for (auto current=directory; !current.empty(); current=current.parent_path()) {
        const auto attributes=GetFileAttributesW(current.c_str());
        if (attributes!=INVALID_FILE_ATTRIBUTES && (attributes&FILE_ATTRIBUTE_REPARSE_POINT)) return false;
        if (current==current.parent_path()) break;
    }
    return true;
}

static bool ExistingGameParent(const fs::path& directory) {
    for (auto current=directory; !current.empty(); current=current.parent_path()) {
        if (fs::is_regular_file(current/L"ShooterGame/Binaries/Win64/VALORANT-Win64-Shipping.exe")) return true;
        if (current==current.parent_path()) break;
    }
    return false;
}

static fs::path ChooseTarget() {
    BROWSEINFOW info{};
    wchar_t display[MAX_PATH]{}, selected[MAX_PATH]{};
    info.pszDisplayName=display;
    info.lpszTitle=L"选择安装位置，将在所选位置创建 ValorantTrainer 文件夹。\n安装约需 36 GB 可用空间，点击“开始安装”后下载。";
    info.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE|BIF_EDITBOX;
    auto item=SHBrowseForFolderW(&info);
    if (!item) return {};
    const bool valid=SHGetPathFromIDListW(item,selected)!=FALSE;
    CoTaskMemFree(item);
    if (!valid) return {};
    fs::path target(selected);
    if (_wcsicmp(target.filename().c_str(),L"ValorantTrainer")!=0
            && !fs::exists(target/L".setup/installer-owner.json") && !fs::exists(target/L"patch-version.json"))
        target/=L"ValorantTrainer";
    return target;
}

static void ClearOwnTemporary(const fs::path& session,const fs::path& parent) noexcept {
    try {
    std::error_code error;
    if (fs::weakly_canonical(session,error)!=session || error || session.parent_path()!=parent
            || !session.filename().wstring().starts_with(L"session-")) return;
    if (!PlainDirectory(session)) return;
    for (const auto& entry:fs::recursive_directory_iterator(session,fs::directory_options::skip_permission_denied,error)) {
        if (error) return;
        const auto attributes=GetFileAttributesW(entry.path().c_str());
        if (attributes!=INVALID_FILE_ATTRIBUTES && (attributes&FILE_ATTRIBUTE_REPARSE_POINT)) return;
    }
    fs::remove_all(session,error);
    } catch (...) { return; }
}

int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int) {
    CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    int argc=0;
    auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);
    fs::path target,report,ack;
    bool preview=false;
    if (argc==3 && std::wstring(argv[1])==L"--self-test") {
        report=fs::absolute(argv[2]);
        target=report.parent_path()/L"bootstrap-self-test";
    } else if (argc==3 && std::wstring(argv[1])==L"--preview") {
        target=fs::absolute(argv[2]);
        preview=true;
    } else if (argc==5 && std::wstring(argv[1])==L"--resume" && std::wstring(argv[3])==L"--self-update-ack") {
        target=fs::absolute(argv[2]);
        ack=fs::absolute(argv[4]);
    } else target=ChooseTarget();
    if (argv) LocalFree(argv);
    if (target.empty()) { CoUninitialize(); return 0; }
    target=fs::absolute(target).lexically_normal();
    DWORD exitCode=1;
    fs::path extracted,session;
    try {
        if (!PlainDirectory(target) || ExistingGameParent(target)) throw std::runtime_error("Choose a new independent local directory");
        const auto setup=target/L".setup";
        const auto temporary=setup/L"runtime-temp";
        if (!PlainDirectory(temporary)) throw std::runtime_error("Installer cache directory redirects elsewhere");
        fs::create_directories(temporary);
        const auto suffix=std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());
        session=temporary/(L"session-"+suffix);
        fs::create_directory(session);
        if (!PlainDirectory(session)) throw std::runtime_error("Installer directory redirects elsewhere");
        extracted=setup/(L"setup-ui-"+suffix+L".exe");
        auto resource=FindResourceW(instance,MAKEINTRESOURCEW(101),RT_RCDATA);
        auto loaded=resource?LoadResource(instance,resource):nullptr;
        const auto bytes=resource?SizeofResource(instance,resource):0;
        const auto data=loaded?LockResource(loaded):nullptr;
        if (!data || !bytes) throw std::runtime_error("Installer payload missing");
        HANDLE output=CreateFileW(extracted.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if (output==INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot create installer runtime");
        DWORD written=0;
        const bool copied=WriteFile(output,data,bytes,&written,nullptr) && written==bytes;
        FlushFileBuffers(output);
        CloseHandle(output);
        if (!copied) throw std::runtime_error("Cannot extract installer runtime");
        SetEnvironmentVariableW(L"TEMP",session.c_str());
        SetEnvironmentVariableW(L"TMP",session.c_str());
        SetEnvironmentVariableW(L"PYTHONPYCACHEPREFIX",(session/L"pycache").c_str());
        wchar_t executable[32768]{};
        GetModuleFileNameW(nullptr,executable,32768);
        SetEnvironmentVariableW(L"TRAINER_SETUP_EXE",executable);
        SetEnvironmentVariableW(L"TRAINER_SETUP_PID",std::to_wstring(GetCurrentProcessId()).c_str());
        std::wstring command=Quote(extracted.wstring())+L" --target "+Quote(target.wstring());
        if (!report.empty()) command+=L" --self-test "+Quote(report.wstring());
        if (preview) command+=L" --preview";
        if (!ack.empty()) command+=L" --self-update-ack "+Quote(ack.wstring());
        STARTUPINFOW startup{};
        startup.cb=sizeof(startup);
        PROCESS_INFORMATION process{};
        if (!CreateProcessW(extracted.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,
                            nullptr,target.c_str(),&startup,&process)) throw std::runtime_error("Cannot start installer runtime");
        CloseHandle(process.hThread);
        WaitForSingleObject(process.hProcess,INFINITE);
        GetExitCodeProcess(process.hProcess,&exitCode);
        CloseHandle(process.hProcess);
        DeleteFileW(extracted.c_str());
        ClearOwnTemporary(session,temporary);
    } catch (const std::exception&) {
        if (!extracted.empty()) DeleteFileW(extracted.c_str());
        if (!report.empty()) {
            fs::create_directories(report.parent_path());
            std::ofstream stream(report);
            stream<<"{\"passed\":false,\"error\":\"bootstrap_failed\"}\n";
        } else MessageBoxW(nullptr,L"安装器无法准备运行环境。\n请选择可写的本地目录，并确认磁盘空间充足。",
                           L"ValorantTrainer 安装器",MB_OK|MB_ICONERROR);
    }
    CoUninitialize();
    return static_cast<int>(exitCode);
}
