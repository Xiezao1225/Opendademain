// main.cpp -- dademain.exe 的主流程
//
// 对应原文 main（反编译文件行 10830-12239）。
//
// 与原实现的差异（均为有意为之，详见 README）：
//   1. 增加 --safe：跳过 UAC / UIAccess，便于本地调试；
//   2. TryGetUIAccess 通过"重启自身"成功后，父进程直接退出（原实现会留下两个实例）；
//   3. 输入改用 ReadConsoleW，避免 std::wcin 在非控制台输入下的编码问题。

#include "common.h"
#include "config.h"
#include "console.h"
#include "elevate.h"
#include "payload.h"
#include "runner.h"
#include "util.h"

#include <cstdlib>
#include <cwctype>
#include <string>
#include <vector>

namespace {

bool g_safeMode = false;

// 逐行解析 eula.txt：只认以 "eula" 开头的键。
// 原实现是"在全文里找第一个 eula 再找下一个 ="，说明文字里出现 eula 就会被误判，
// 这里按 INI 的方式解析，行为更可预期。
bool EulaAccepted(const std::wstring& text)
{
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t nl = text.find(L'\n', pos);
        if (nl == std::wstring::npos) nl = text.size();

        std::wstring line = text.substr(pos, nl - pos);
        pos = (nl == text.size()) ? text.size() + 1 : nl + 1;

        if (!line.empty() && line.back() == L'\r') line.pop_back();
        line = Trim(line);
        if (line.empty() || line[0] == L'#' || line[0] == L';') continue;
        if (line.rfind(L"eula", 0) != 0) continue;

        const size_t eq = line.find(L'=');
        if (eq == std::wstring::npos) continue;

        std::wstring value = Trim(line.substr(eq + 1));
        for (wchar_t& c : value) c = static_cast<wchar_t>(towlower(c));
        return value == L"true";
    }
    return false;
}

// 首次运行的欢迎 / 许可门禁（原文行 11833-12221）：
// 写 eula.txt -> 打开说明 -> pause -> 只有 eula=true 才放行。
bool FirstRunGate(const std::wstring& exeDir)
{
    const std::wstring eulaPath = PathJoin(exeDir, app::kEulaName);

    if (!FileExists(eulaPath)) {
        const std::wstring notice =
            L"==================== 使用前请阅读 ====================\n"
            L"\n"
            L"  本程序是 dademain.exe（极域工具箱）的可编译重实现，仅供学习与\n"
            L"  逆向分析用途。它会把内置资源释放到本目录的 dade\\ 子目录，并按\n"
            L"  其中的 .dade 配置启动外部程序。\n"
            L"\n"
            L"  请确认你有权在目标机器上运行本程序及其调起的工具。\n"
            L"\n"
            L"  把下面这一行改成 false 即可让程序不再进入主菜单：\n"
            L"-----------------------------------------------------------\n"
            L"eula=true\n";
        WriteWholeFile(eulaPath, WideToUtf8(notice));  // 与其它文本文件一致：UTF-8
    }

    ConsoleWrite(app::kBanner1);
    ConsoleWrite(app::kBanner2);
    ConsoleWrite(app::kRule);
    ShowReadme(exeDir, true);
    ConsoleWrite(L"欢迎使用 dademain.exe! 请详细阅读说明文档以继续 ...");
    ConsoleWriteLine();
    system("pause > nul");

    std::string raw;
    if (!ReadWholeFile(eulaPath, raw)) return false;
    return EulaAccepted(Utf8ToWide(raw));
}

void PrintMenu(const std::vector<ToolNode>& tools)
{
    ConsoleWrite(app::kBanner1);
    ConsoleWrite(app::kBanner2);

    if (tools.empty()) {
        ConsoleWrite(L"  r 更新配置  |  h 帮助  |  q 退出  |  !r 还原程序\n");
        ConsoleWrite(app::kRule);
        ConsoleWriteLine(L"  [-]  当前没有可用的工具, 请检查配置文件!");
        return;
    }

    ConsoleWrite(L"  输入序号选择工具  |  r 更新配置  |  h 帮助  |  q 退出\n");
    ConsoleWrite(app::kRule);

    for (size_t i = 0; i < tools.size(); ++i) {
        std::wstring line = L"  [" + std::to_wstring(i + 1) + L"] " + tools[i].name;
        if (!tools[i].description.empty()) line += L" - " + tools[i].description;
        ConsoleWriteLine(line);
    }
    ConsoleWriteLine();
    ConsoleWrite(L"请选择 > ");
}

// 配置里存在 admin 工具时才自提权（原文行 10996-11032）
bool AnyAdminTool(const std::vector<ToolNode>& tools)
{
    for (const ToolNode& t : tools) {
        if (t.admin) return true;
    }
    return false;
}

void SelfElevate(const std::wstring& exePath, const std::wstring& exeDir)
{
    SHELLEXECUTEINFOW sei{};
    sei.cbSize      = sizeof(sei);
    sei.fMask       = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb      = L"runas";
    sei.lpFile      = exePath.c_str();
    sei.lpDirectory = exeDir.c_str();
    sei.nShow       = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&sei)) {
        ConsoleWriteLine(L"[-] 提权失败, 请手动以管理员身份运行");
        return;
    }
    if (sei.hProcess != nullptr) CloseHandle(sei.hProcess);
}

}  // namespace

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--safe") g_safeMode = true;
    }

    ConsoleSetup();

    const std::wstring exePath = GetModulePath();
    const std::wstring exeDir  = GetModuleDir();
    const std::wstring dataDir = PathJoin(exeDir, app::kDataDir);
    const std::wstring cfgPath = PathJoin(dataDir, app::kConfigName);
    const std::wstring verPath = PathJoin(dataDir, app::kVersionName);

    CreateDirectoryW(dataDir.c_str(), nullptr);

    // ---- 首次运行（没有 .version）----
    if (!FileExists(verPath)) {
        if (!FirstRunGate(exeDir)) {
            ConsoleWriteLine(L"[-] eula.txt 中 eula 不为 true, 已退出。");
            return 0;
        }
        ConsoleClear();
    }

    // ---- 自解压 + 读配置 ----
    ExtractPayload(dataDir);

    std::vector<ToolNode> tools;
    std::wstring          err;
    if (!LoadCfg(cfgPath, tools, err)) {
        ConsoleWriteLine(L"[-] " + err);
        ConsoleWriteLine(L"提示: 删除 " + verPath + L" 后重新运行可重新释放默认配置。");
        return 1;
    }

    // ---- 提权 / UIAccess（--safe 时全部跳过）----
    if (!g_safeMode) {
        if (AnyAdminTool(tools) && !IsElevated()) SelfElevate(exePath, exeDir);

        ConsoleWriteLine(L"尝试获取 UIAccess 权限 ...");
        bool relaunched = false;
        if (TryGetUIAccess(&relaunched)) {
            if (relaunched) return 0;  // 新实例已接管，本进程退出
        } else {
            ConsoleWriteLine(L"[-] 获取失败, 将以管理员权限继续运行, 窗口层级可能不足");
            HWND console = GetConsoleWindow();
            if (console != nullptr)
                SetWindowPos(console, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            StartHotkeyThread();
        }
    }

    // ---- 主循环 ----
    for (;;) {
        ConsoleClear();
        PrintMenu(tools);

        std::wstring input;
        if (!ConsoleReadLine(input)) break;  // EOF / Ctrl+Z
        input = Trim(input);
        if (input.empty()) continue;

        if (input == L"q" || input == L"Q" || input == L"exit") break;

        if (input == L"h" || input == L"help" || input == L"?") {
            if (!ShowReadme(exeDir, false))
                ConsoleWriteLine(L"[-] 开启帮助文档失败, 请手动打开: " +
                                 PathJoin(exeDir, app::kReadmeName));
            continue;
        }

        if (input == L"r" || input == L"reload") {  // 更新配置
            if (LoadCfg(cfgPath, tools, err)) ConsoleWriteLine(L"[+] 配置已成功重新加载");
            else ConsoleWriteLine(L"[-] " + err);
            continue;
        }

        if (input == L"!r" || input == L"restore") {  // 还原程序
            // 原实现只清空 .version，但 .dade 载荷本身不会被覆盖（解压器会跳过已存在的
            // .dade），所以它其实还原不了配置。这里先把配置备份再删掉，让默认配置回来。
            if (FileExists(cfgPath)) {
                const std::wstring backup = cfgPath + L".bak";
                CopyFileW(cfgPath.c_str(), backup.c_str(), FALSE);
            }
            DeleteFileW(cfgPath.c_str());
            WriteWholeFile(verPath, std::string());
            ExtractPayload(dataDir);
            if (LoadCfg(cfgPath, tools, err))
                ConsoleWriteLine(L"[+] 已还原为内置默认配置（原文件已备份为 .dade.bak）");
            else
                ConsoleWriteLine(L"[-] " + err);
            continue;
        }

        wchar_t*   end = nullptr;
        const long idx = std::wcstol(input.c_str(), &end, 10);
        if (end != nullptr && *end == L'\0' && idx > 0 &&
            static_cast<size_t>(idx) <= tools.size()) {
            // 工具路径相对 dade\（载荷目录），不是 exe 目录 —— 与原程序一致
            RunTools(tools[static_cast<size_t>(idx) - 1], dataDir);
            continue;
        }

        ConsoleWriteLine(L"[-] 无效输入, 请重新选择");
    }

    ConsoleWriteLine(L"bye.");
    return 0;
}
