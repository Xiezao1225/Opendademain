#include "runner.h"

#include "console.h"
#include "elevate.h"
#include "util.h"

#include <cwchar>

namespace {

// [0x140006420] 交互收集参数，并替换 args 里的 {题面}
std::wstring CollectPrompts(const ToolNode& node)
{
    if (node.prompts.empty()) return node.args;

    std::wstring args = node.args;
    ConsoleWriteLine(L"请依次输入以下参数 [默认值]: ");
    ConsoleWriteLine();

    std::vector<std::wstring> answers;
    answers.reserve(node.prompts.size());

    for (const PromptSpec& prompt : node.prompts) {
        std::wstring label = L"  " + prompt.text;
        if (!prompt.defval.empty()) label += L" [" + prompt.defval + L"]";
        ConsoleWrite(label + L": ");

        std::wstring input;
        if (!ConsoleReadLine(input)) return {};
        input = Trim(input);

        if (input == L"back" || input == L"exit") return {};  // 放弃执行
        answers.push_back(input.empty() ? prompt.defval : input);
    }

    for (size_t i = 0; i < node.prompts.size(); ++i) {
        const std::wstring token = L"{" + node.prompts[i].key + L"}";
        for (size_t at = args.find(token); at != std::wstring::npos; at = args.find(token, at))
            args.replace(at, token.size(), answers[i]);
    }
    ConsoleWriteLine();
    return args;
}

void PrintToolHeader(const ToolNode& node)
{
    ConsoleClear();
    ConsoleWrite(app::kBanner1);
    ConsoleWrite(app::kBanner2);
    ConsoleWrite(L"工具: " + node.name + L"\n");
    ConsoleWrite(app::kRule);
    if (!node.description.empty()) ConsoleWriteLine(L"  " + node.description);
}

// 子菜单：子项列表 + "[0] 返回上级菜单"
void RunSubMenu(ToolNode& node, const std::wstring& baseDir)
{
    for (;;) {
        PrintToolHeader(node);

        for (size_t i = 0; i < node.children.size(); ++i) {
            const ToolNode& child = node.children[i];
            std::wstring    line  = L"  [" + std::to_wstring(i + 1) + L"] " + child.name;
            if (!child.description.empty()) line += L" - " + child.description;
            ConsoleWriteLine(line);
        }

        ConsoleWriteLine(L"  [0]  返回上级菜单");
        ConsoleWriteLine();
        ConsoleWrite(L"请选择 > ");

        std::wstring input;
        if (!ConsoleReadLine(input)) return;
        input = Trim(input);
        if (input.empty()) continue;
        if (input == L"0") return;

        wchar_t*   end = nullptr;
        const long idx = std::wcstol(input.c_str(), &end, 10);
        if (end != nullptr && *end == L'\0' && idx > 0 &&
            static_cast<size_t>(idx) <= node.children.size()) {
            RunTools(node.children[static_cast<size_t>(idx) - 1], baseDir);
        } else {
            ConsoleWriteLine(L"[-] 无效输入, 请重新选择");
        }
    }
}

}  // namespace

// [0x140006416] RunTools
void RunTools(ToolNode& node, const std::wstring& baseDir)
{
    if (!node.children.empty()) {
        RunSubMenu(node, baseDir);
        return;
    }

    PrintToolHeader(node);

    const std::wstring args = CollectPrompts(node);

    std::wstring exePath = PathJoin(baseDir, node.path);
    if (!FileExists(exePath)) exePath = node.path;  // 退回原样

    std::wstring workdir = node.workdir.empty() ? baseDir : PathJoin(baseDir, node.workdir);
    if (!FileExists(workdir)) workdir = baseDir;

    // wait 语义：1 等；0 不等；未设置 -> 开新控制台时才等
    const bool shouldWait = node.wait > 0 || (node.wait < 0 && node.newconsole);

    // ---- 需要管理员且当前未提权：走 UAC ----
    if (node.admin && !IsElevated()) {
        SHELLEXECUTEINFOW sei{};
        sei.cbSize       = sizeof(sei);
        sei.fMask        = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
        sei.lpVerb       = L"runas";
        sei.lpFile       = exePath.c_str();
        sei.lpParameters = args.empty() ? nullptr : args.c_str();
        sei.lpDirectory  = workdir.c_str();
        sei.nShow        = node.hidden ? SW_HIDE : SW_SHOWNORMAL;

        if (!ShellExecuteExW(&sei)) {
            ConsoleWriteLine(L"[-] 启动失败 " + exePath + L"    " + FormatWinError(GetLastError()));
            return;
        }
        if (sei.hProcess != nullptr) {
            if (shouldWait) WaitForSingleObject(sei.hProcess, INFINITE);
            CloseHandle(sei.hProcess);
        }
        return;
    }

    // ---- 普通启动 ----
    std::wstring commandLine = L"\"" + exePath + L"\"";
    if (!args.empty()) commandLine += L" " + args;

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    if (node.hidden) {
        si.dwFlags |= STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
    }

    PROCESS_INFORMATION pi{};

    // lpApplicationName 非空时不走 PATH 搜索，所以只有确实存在时才传路径；
    // 否则交给 CreateProcess 从命令行首 token 解析（支持 cmd.exe / notepad 这类裸名字）。
    LPCWSTR applicationName = FileExists(exePath) ? exePath.c_str() : nullptr;

    if (!CreateProcessW(applicationName, commandLine.data(), nullptr, nullptr, FALSE,
                        node.newconsole ? CREATE_NEW_CONSOLE : 0, nullptr, workdir.c_str(), &si,
                        &pi)) {
        ConsoleWriteLine(L"[-] 启动失败 " + exePath + L"    " + FormatWinError(GetLastError()));
        return;
    }

    if (shouldWait) WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}
