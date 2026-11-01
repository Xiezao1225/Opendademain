#include "payload.h"

#include "common.h"
#include "console.h"
#include "util.h"

#include <cwctype>
#include <vector>

namespace {

struct ResBlob {
    const void* data = nullptr;
    DWORD       size = 0;
    explicit operator bool() const { return data != nullptr && size != 0; }
};

ResBlob LoadRes(const wchar_t* name)
{
    ResBlob blob;

    HRSRC info = FindResourceW(nullptr, name, RT_RCDATA);
    if (info == nullptr) return blob;

    HGLOBAL handle = LoadResource(nullptr, info);
    if (handle == nullptr) return blob;

    const DWORD size = SizeofResource(nullptr, info);
    const void* data = LockResource(handle);
    if (data == nullptr || size == 0) return blob;

    blob.data = data;
    blob.size = size;
    return blob;
}

std::wstring ToLower(std::wstring s)
{
    for (wchar_t& c : s) c = static_cast<wchar_t>(towlower(c));
    return s;
}

}  // namespace

bool ExtractPayload(const std::wstring& dataDir)
{
    const std::wstring versionPath = PathJoin(dataDir, app::kVersionName);

    std::string oldVersion;
    if (ReadWholeFile(versionPath, oldVersion) && TrimRight(Utf8ToWide(oldVersion)) == app::kVersion)
        return false;  // 版本一致，无需解压

    const ResBlob manifest = LoadRes(app::kManifestRes);
    if (!manifest) {
        ConsoleWriteLine(L"[-] 内置资源 TOOLS_MANIFEST 缺失, 跳过解压");
        return false;
    }

    const std::string text(static_cast<const char*>(manifest.data), manifest.size);
    std::size_t pos = 0;
    int         extracted = 0;

    while (pos < text.size()) {
        std::size_t nl = text.find('\n', pos);
        if (nl == std::string::npos) nl = text.size();

        std::string line = text.substr(pos, nl - pos);
        pos = nl + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        const std::size_t bar = line.find('|');
        if (bar == std::string::npos) continue;

        const std::wstring resName = Utf8ToWide(line.substr(0, bar));
        std::wstring       rel     = Utf8ToWide(line.substr(bar + 1));
        if (resName.empty() || rel.empty()) continue;

        for (wchar_t& c : rel) {
            if (c == L'/') c = L'\\';
        }

        const std::wstring target = PathJoin(dataDir, rel);

        // 已存在的 .dade 载荷保留（用户可能替换过）
        const std::wstring lower = ToLower(target);
        const bool isDade = lower.size() >= 5 && lower.compare(lower.size() - 5, 5, L".dade") == 0;
        if (isDade && FileExists(target)) continue;

        const ResBlob blob = LoadRes(resName.c_str());
        if (!blob) {
            ConsoleWriteLine(L"[-] 资源缺失: " + resName);
            continue;
        }

        EnsureParentDir(target);
        if (WriteWholeFile(target, blob.data, blob.size)) ++extracted;
    }

    // .version 是 ASCII 文本（与原文一致，7 字节），不要写成 UTF-16
    WriteWholeFile(versionPath, std::string("1.0.0.0"));
    ConsoleWriteLine(L"[+] 已释放 " + std::to_wstring(extracted) + L" 个文件到 " + dataDir);
    return true;
}

bool ShowReadme(const std::wstring& exeDir, bool verbose)
{
    const ResBlob blob = LoadRes(app::kReadmeRes);
    if (!blob) {
        if (verbose) ConsoleWriteLine(L"[-] 内置资源 README_HTML 缺失");
        return false;
    }

    const std::wstring file = PathJoin(exeDir, app::kReadmeName);
    if (!WriteWholeFile(file, blob.data, blob.size)) {
        if (verbose) ConsoleWriteLine(L"[-] 写入 readme.html 失败");
        return false;
    }

    const HINSTANCE r =
        ShellExecuteW(nullptr, L"open", file.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    const bool ok = reinterpret_cast<INT_PTR>(r) > 32;

    if (verbose) {
        if (ok) ConsoleWriteLine(L"[+] 已开启帮助文档: " + file);
        else ConsoleWriteLine(L"[-] 开启帮助文档失败, 请手动打开: " + file);
    }
    return ok;
}
