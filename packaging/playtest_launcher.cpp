#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <string>
#include <cwctype>
#include <cstdint>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    wchar_t executable[32768]{};
    const DWORD length = GetModuleFileNameW(nullptr, executable, 32768);
    if (!length || length >= 32768) return 1;
    const std::wstring path(executable, length);
    const auto separator = path.find_last_of(L"\\/");
    if (separator == std::wstring::npos) return 1;
    const std::wstring root = path.substr(0, separator);
    std::uint64_t identity = 14695981039346656037ull;
    for (const wchar_t character : root) {
        identity ^= static_cast<std::uint64_t>(std::towlower(character));
        identity *= 1099511628211ull;
    }
    const std::wstring mutex = L"Local\\ValorantTrainerPlaytestLauncher-" + std::to_wstring(identity);
    HANDLE instance = CreateMutexW(nullptr, TRUE, mutex.c_str());
    if (!instance) return 1;
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(nullptr, L"本目录的训练器已在启动或运行中。", L"斗牛 1 测试版", MB_OK | MB_ICONINFORMATION);
        CloseHandle(instance);
        return 0;
    }
    const std::wstring python = root + L"\\runtime\\pythonw.exe";
    const std::wstring script = root + L"\\tools\\run_duel1_training.py";
    std::wstring command = L"\"" + python + L"\" -E -s \"" + script + L"\" --prebuilt";
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    DWORD code = 1;
    if (CreateProcessW(python.c_str(), command.data(), nullptr, nullptr, FALSE,
                       CREATE_NO_WINDOW, nullptr, root.c_str(), &startup, &process)) {
        CloseHandle(process.hThread);
        WaitForSingleObject(process.hProcess, INFINITE);
        GetExitCodeProcess(process.hProcess, &code);
        CloseHandle(process.hProcess);
    }
    if (code) {
        const std::wstring message = L"训练器启动失败或运行中断。\n\n"
            L"请双击“导出测试日志.cmd”，把生成的 ZIP 发回当前对话。\n"
            L"也可以双击“诊断启动.cmd”查看具体提示。\n\n" + root;
        MessageBoxW(nullptr, message.c_str(), L"斗牛 1 测试版", MB_OK | MB_ICONERROR);
    }
    ReleaseMutex(instance);
    CloseHandle(instance);
    return static_cast<int>(code);
}
