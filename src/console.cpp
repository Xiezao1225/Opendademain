#include "console.h"

#include "common.h"
#include "util.h"

#include <iterator>

namespace {

// 重定向输入（管道 / 文件）时的行缓冲：std::wcin 与 ReadConsoleW 混用会导致
// EOF 判断混乱，这里直接按字节读、按行切、UTF-8 解码。
std::string g_pending;
bool        g_bomChecked = false;

bool ReadRedirectedLine(std::wstring& out)
{
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    if (h == nullptr || h == INVALID_HANDLE_VALUE) return false;

    for (;;) {
        const size_t nl = g_pending.find('\n');
        if (nl != std::string::npos) {
            std::string line = g_pending.substr(0, nl);
            g_pending.erase(0, nl + 1);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            out = Utf8ToWide(line);
            return true;
        }

        char  buf[1024];
        DWORD got = 0;
        if (!ReadFile(h, buf, static_cast<DWORD>(std::size(buf)), &got, nullptr) || got == 0) {
            if (g_pending.empty()) return false;  // EOF

            out = Utf8ToWide(g_pending);  // 最后一行没有换行
            g_pending.clear();
            if (!out.empty() && out.back() == L'\r') out.pop_back();
            return true;
        }

        g_pending.append(buf, got);

        if (!g_bomChecked) {
            g_bomChecked = true;
            if (g_pending.size() >= 3 && static_cast<unsigned char>(g_pending[0]) == 0xEF &&
                static_cast<unsigned char>(g_pending[1]) == 0xBB &&
                static_cast<unsigned char>(g_pending[2]) == 0xBF)
                g_pending.erase(0, 3);
        }
    }
}

}  // namespace

void ConsoleSetup()
{
    SetConsoleOutputCP(CP_UTF8);  // 原文 0xFDE9
    SetConsoleCP(CP_UTF8);
    SetConsoleTitleW(app::kTitle);
}

void ConsoleWrite(const std::wstring& s)
{
    if (s.empty()) return;

    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == nullptr || h == INVALID_HANDLE_VALUE) return;

    DWORD written = 0;
    if (WriteConsoleW(h, s.c_str(), static_cast<DWORD>(s.size()), &written, nullptr)) return;

    // 输出被重定向时退回 UTF-8 字节流
    const std::string utf8 = WideToUtf8(s);
    WriteFile(h, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
}

void ConsoleWriteLine(const std::wstring& s)
{
    ConsoleWrite(s);
    ConsoleWrite(L"\n");
}

void ConsoleClear()
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == nullptr || h == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(h, &info)) return;

    const DWORD total = static_cast<DWORD>(info.dwSize.X) * static_cast<DWORD>(info.dwSize.Y);
    const COORD origin{0, 0};
    DWORD       written = 0;

    FillConsoleOutputCharacterW(h, L' ', total, origin, &written);
    FillConsoleOutputAttribute(h, info.wAttributes, total, origin, &written);
    SetConsoleCursorPosition(h, origin);
}

bool ConsoleReadLine(std::wstring& out)
{
    out.clear();

    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    if (h == nullptr || h == INVALID_HANDLE_VALUE) return false;

    DWORD mode = 0;
    if (!GetConsoleMode(h, &mode)) return ReadRedirectedLine(out);  // 管道 / 文件

    wchar_t buf[4096];
    DWORD   read = 0;
    if (!ReadConsoleW(h, buf, static_cast<DWORD>(std::size(buf)), &read, nullptr)) return false;
    if (read == 0) return false;  // Ctrl+Z / EOF

    out.assign(buf, read);
    while (!out.empty() && (out.back() == L'\n' || out.back() == L'\r')) out.pop_back();
    return true;
}
