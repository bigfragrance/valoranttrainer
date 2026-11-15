#include "local_server_package_paths.h"
#include "local_server_owned_paths.h"
#include <cstdio>

static unsigned checks=0,failures=0;
static void Check(bool result,const char*label){++checks;if(!result){++failures;std::printf("FAIL %s\n",label);}}
int main(){
    namespace paths=local_server_package_paths;
    const std::wstring roots[]{L"F:\\valoranttrainer",L"D:\\Training",L"E:\\Games Folder\\训练器 测试",L"G:\\测试\\Portable-Patch",
                               L"C:\\Users\\Test User\\Desktop\\ValorantTrainer-Patch",L"H:\\Patch"};
    for(const auto&root:roots){
        const auto module=root+L"\\.local\\local-server\\module\\local_server_bridge.dll";
        const auto image=root+L"\\client\\ShooterGame\\Binaries\\Win64\\VALORANT-Win64-Shipping.exe";
        const auto pair=root+L"\\.local\\local-server\\pair-20261108T063603.394810Z";
        Check(paths::RoleDirectory(module,image,pair+L"\\host\\",1),"owned host can relocate");
        Check(paths::RoleDirectory(module,image,pair+L"\\client\\",2),"owned client can relocate");
        Check(!paths::RoleDirectory(module,image,pair+L"\\client\\",1),"host cannot use client directory");
        Check(!paths::RoleDirectory(module,image,pair+L"\\host\\",2),"client cannot use host directory");
        Check(!paths::RoleDirectory(module,image,pair+L"\\host\\extra",1),"no arbitrary nested outputs");
        Check(!paths::RoleDirectory(module,image,root+L"\\.local\\local-server\\pair-\\host",1),"pair identifier required");
        Check(!paths::RoleDirectory(module,image,root+L"\\.local\\local-server\\pair-..\\host",1),"parent pair rejected");
        Check(!paths::RoleDirectory(module,image,root+L"\\.local\\local-server\\pair-test\\..\\host",1),"parent traversal rejected");
        Check(!paths::RoleDirectory(module,image,pair+L"\\host\\",0),"unknown role rejected");
        Check(!paths::RoleDirectory(module,image,L"F:\\some-other-package\\.local\\local-server\\pair-test\\host",1),"output cannot leave module package");
        Check(!paths::RoleDirectory(module,L"F:\\Riot Games\\VALORANT\\live\\ShooterGame\\Binaries\\Win64\\VALORANT-Win64-Shipping.exe",pair+L"\\host",1),"installed official image is never owned client");
        Check(!paths::RoleDirectory(module,root+L"\\client-elsewhere\\ShooterGame\\Binaries\\Win64\\VALORANT-Win64-Shipping.exe",pair+L"\\host",1),"different image layout rejected");
        Check(!paths::RoleDirectory(root+L"\\elsewhere\\local_server_bridge.dll",image,pair+L"\\host",1),"different module layout rejected");
        const auto report=pair+L"\\host\\mode-startup-state.json";
        Check(paths::ModeStartupReport(pair+L"\\host\\",report),"startup report accepts own directory on every drive");
        Check(paths::Absolute(paths::Normalize(report)),"startup configure accepts absolute report on every drive");
        Check(!paths::ModeStartupReport(pair+L"\\host\\",pair+L"\\client\\mode-startup-state.json"),"startup report cannot cross role");
        Check(!paths::ModeStartupReport(pair+L"\\host",pair+L"\\host\\extra\\mode-startup-state.json"),"startup report cannot use nested output");
        Check(!paths::ModeStartupReport(pair+L"\\host",pair+L"\\host\\other.json"),"startup report filename fixed");
        Check(!paths::ModeStartupReport(pair+L"\\host",pair+L"\\host\\..\\client\\mode-startup-state.json"),"startup report traversal rejected");
        Check(!paths::ModeStartupReport(pair+L"\\host",L"C:relative\\mode-startup-state.json"),"startup report drive relative rejected");
        std::wstring canonical;
        Check(local_server_owned_paths::CanonicalAbsolute(pair+L"\\host\\",canonical)
              &&paths::Normalize(canonical)==paths::Normalize(pair+L"\\host"),"Win32 canonical role path accepts every local drive");
        Check(!local_server_owned_paths::CanonicalAbsolute(pair+L"\\host\\..\\client",canonical),"Win32 canonical role rejects unresolved traversal");
    }
    Check(paths::RoleDirectory(L"d:/portable/.LOCAL/local-server/MODULE/local_server_bridge.dll",
                              L"D:/PORTABLE/client/ShooterGame/Binaries/Win64/VALORANT-Win64-Shipping.exe",
                              L"d:/portable/.local/local-server/pair-test/client/",2),"Windows case and slash normalization");
    Check(!paths::Absolute(L"F:relative\\file"),"drive-relative path rejected");
    Check(!paths::Absolute(L"\\\\server\\share\\file"),"network share rejected");
    Check(!paths::Absolute(L"F:\\path\\folder.\\file"),"ambiguous trailing dot rejected");
    Check(!paths::Absolute(L"F:\\path\\folder \\file"),"ambiguous trailing space rejected");
    Check(paths::ModeStartupReport(L"d:/Training/.local/local-server/pair-test/HOST/",
                                  L"D:/training/.LOCAL/local-server/pair-test/host/MODE-STARTUP-STATE.JSON"),"startup report Windows normalization");
    Check(!local_server_owned_paths::RoleDirectory(L"D:\\Training\\.local\\local-server\\pair-test\\host",1),"unrelated test executable cannot claim package ownership");
    std::printf("checks=%u failures=%u\n",checks,failures);
    return failures?1:0;
}
