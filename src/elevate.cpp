#include "elevate.h"

#include "common.h"
#include "util.h"

#include <tlhelp32.h>

#include <cwchar>

namespace {

// [0x140034ed0] 从 winlogon.exe / services.exe 复制 SYSTEM 令牌
HANDLE GetSystemToken()
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return nullptr;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    HANDLE result = nullptr;

    for (BOOL ok = Process32FirstW(snap, &entry); ok != FALSE; ok = Process32NextW(snap, &entry)) {
        if (_wcsicmp(entry.szExeFile, L"winlogon.exe") != 0 &&
            _wcsicmp(entry.szExeFile, L"services.exe") != 0)
            continue;

        HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, entry.th32ProcessID);
        if (proc == nullptr) continue;

        HANDLE token = nullptr;
        if (OpenProcessToken(proc, TOKEN_QUERY | TOKEN_DUPLICATE, &token)) {
            // 必须拥有 SeTcbPrivilege，SetTokenInformation(TokenUIAccess) 才可能成功
            PRIVILEGE_SET ps{};
            ps.PrivilegeCount          = 1;
            ps.Control                 = PRIVILEGE_SET_ALL_NECESSARY;
            ps.Privilege[0].Attributes = 0;
            LookupPrivilegeValueW(nullptr, L"SeTcbPrivilege", &ps.Privilege[0].Luid);

            BOOL has = FALSE;
            if (PrivilegeCheck(token, &ps, &has) && has != FALSE) {
                HANDLE dup = nullptr;
                if (DuplicateTokenEx(token, MAXIMUM_ALLOWED, nullptr, SecurityImpersonation,
                                     TokenPrimary, &dup))
                    result = dup;
            }
            CloseHandle(token);
        }
        CloseHandle(proc);

        if (result != nullptr) break;
    }

    CloseHandle(snap);
    return result;
}

DWORD WINAPI HotkeyThread(LPVOID)
{
    constexpr int kHotkeyId = 0x0DADE;  // 原文魔数
    if (!RegisterHotKey(nullptr, kHotkeyId, MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT, 'D'))
        return 1;

    HWND console = GetConsoleWindow();
    bool visible = true;

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message != WM_HOTKEY || static_cast<int>(msg.wParam) != kHotkeyId) continue;
        if (console == nullptr) continue;

        if (!visible) {
            SetWindowPos(console, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            SetForegroundWindow(console);
            visible = true;
        } else {
            ShowWindowAsync(console, SW_HIDE);
            visible = false;
        }
    }

    UnregisterHotKey(nullptr, kHotkeyId);
    return 0;
}

}  // namespace

bool IsElevated()
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return false;

    TOKEN_ELEVATION elevation{};
    DWORD           returned = 0;
    const BOOL ok =
        GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &returned);
    CloseHandle(token);
    return ok != FALSE && elevation.TokenIsElevated != 0;
}

bool TryGetUIAccess(bool* relaunched)
{
    if (relaunched != nullptr) *relaunched = false;

    // 1) 已经具备 UIAccess
    {
        HANDLE token = nullptr;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
            DWORD value = 0, returned = 0;
            const BOOL ok = GetTokenInformation(token, TokenUIAccess, &value, sizeof(value),
                                                &returned);
            CloseHandle(token);
            if (ok != FALSE && value != 0) return true;
        }
    }

    if (!IsElevated()) return false;

    HANDLE systemToken = GetSystemToken();
    if (systemToken == nullptr) return false;

    bool   done       = false;
    HANDLE selfToken  = nullptr;
    HANDLE newToken   = nullptr;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, &selfToken) &&
        DuplicateTokenEx(selfToken, TOKEN_ALL_ACCESS, nullptr, SecurityImpersonation, TokenPrimary,
                         &newToken)) {
        if (ImpersonateLoggedOnUser(systemToken)) {
            DWORD one = 1;
            const BOOL ok = SetTokenInformation(newToken, TokenUIAccess, &one, sizeof(one));
            RevertToSelf();

            if (ok != FALSE) {
                STARTUPINFOW si{};
                si.cb = sizeof(si);

                PROCESS_INFORMATION pi{};
                const std::wstring  self = GetModulePath();

                if (CreateProcessAsUserW(newToken, self.c_str(), nullptr, nullptr, nullptr, FALSE,
                                         CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi)) {
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                    if (relaunched != nullptr) *relaunched = true;
                    done = true;
                }
            }
        }
    }

    if (newToken != nullptr) CloseHandle(newToken);
    if (selfToken != nullptr) CloseHandle(selfToken);
    CloseHandle(systemToken);
    return done;
}

void StartHotkeyThread()
{
    HANDLE thread = CreateThread(nullptr, 0, HotkeyThread, nullptr, 0, nullptr);
    if (thread != nullptr) CloseHandle(thread);
}
