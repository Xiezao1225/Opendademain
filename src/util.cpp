#include "util.h"

#include "common.h"

#include <shlobj.h>

#include <cwctype>

std::wstring PathJoin(const std::wstring& dir, const std::wstring& sub)
{
    if (sub.empty()) return dir;
    if (dir.empty()) return sub;

    // 子路径已是绝对路径
    if (sub.size() >= 2 && sub[1] == L':') return sub;
    if (sub[0] == L'\\' || sub[0] == L'/') return sub;

    const wchar_t last = dir.back();
    if (last == L'\\' || last == L'/') return dir + sub;
    return dir + L"\\" + sub;
}

std::wstring TrimRight(std::wstring s)
{
    const size_t pos = s.find_last_not_of(L" \t\r\n");
    if (pos == std::wstring::npos) return {};
    s.resize(pos + 1);
    return s;
}

std::wstring Trim(std::wstring s)
{
    const size_t first = s.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return {};
    s = s.substr(first);
    while (!s.empty() && (s.back() == L' ' || s.back() == L'\t' || s.back() == L'\r' ||
                          s.back() == L'\n'))
        s.pop_back();
    return s;
}

bool FileExists(const std::wstring& path)
{
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

std::wstring GetModulePath()
{
    wchar_t buf[1024] = {};
    const DWORD n = GetModuleFileNameW(nullptr, buf, static_cast<DWORD>(std::size(buf)));
    if (n == 0) return {};
    return std::wstring(buf, n);
}

std::wstring GetModuleDir()
{
    const std::wstring path = GetModulePath();
    const size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return L".";
    return path.substr(0, slash);
}

bool ReadWholeFile(const std::wstring& path, std::string& out)
{
    out.clear();

    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(h, &size) || size.QuadPart < 0 || size.QuadPart > (1LL << 31)) {
        CloseHandle(h);
        return false;
    }

    out.assign(static_cast<size_t>(size.QuadPart), '\0');
    DWORD got = 0;
    size_t done = 0;
    while (done < out.size()) {
        const DWORD want = static_cast<DWORD>(out.size() - done);
        if (!ReadFile(h, &out[done], want, &got, nullptr) || got == 0) break;
        done += got;
    }
    out.resize(done);
    CloseHandle(h);
    return true;
}

bool WriteWholeFile(const std::wstring& path, const void* data, size_t len)
{
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;

    DWORD wrote = 0;
    const BOOL ok = WriteFile(h, data, static_cast<DWORD>(len), &wrote, nullptr);
    CloseHandle(h);
    return ok != FALSE && wrote == len;
}

bool WriteWholeFile(const std::wstring& path, const std::wstring& s)
{
    return WriteWholeFile(path, s.data(), s.size() * sizeof(wchar_t));
}

bool WriteWholeFile(const std::wstring& path, const std::string& s)
{
    return WriteWholeFile(path, s.data(), s.size());
}

std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty()) return {};

    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (n <= 0) return {};

    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}

std::string WideToUtf8(const std::wstring& s)
{
    if (s.empty()) return {};

    const int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0,
                                      nullptr, nullptr);
    if (n <= 0) return {};

    std::string out(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n, nullptr,
                        nullptr);
    return out;
}

bool EnsureParentDir(const std::wstring& filePath)
{
    const size_t slash = filePath.find_last_of(L"\\/");
    if (slash == std::wstring::npos || slash == 0) return false;

    const std::wstring dir = filePath.substr(0, slash);
    if (FileExists(dir)) return true;

    const int rc = SHCreateDirectoryExW(nullptr, dir.c_str(), nullptr);
    return rc == ERROR_SUCCESS || rc == ERROR_ALREADY_EXISTS || rc == ERROR_FILE_EXISTS;
}

std::wstring FormatWinError(DWORD code)
{
    wchar_t* msg = nullptr;
    const DWORD n = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<wchar_t*>(&msg), 0, nullptr);

    std::wstring out = (n != 0 && msg != nullptr) ? std::wstring(msg, n) : std::wstring();
    if (msg != nullptr) LocalFree(msg);

    out = Trim(out);
    if (out.empty()) out = L"未知错误";
    return out + L" (" + std::to_wstring(code) + L")";
}
