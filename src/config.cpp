#include "config.h"

#include "console.h"
#include "util.h"

#include <cwctype>
#include <functional>
#include <unordered_map>

namespace {

bool IsTruthy(const std::wstring& v)
{
    std::wstring s = Trim(v);
    for (wchar_t& c : s) c = static_cast<wchar_t>(towlower(c));
    return s == L"true" || s == L"1" || s == L"yes" || s == L"on";
}

// 递归剪枝：既没有 path、又没有子项的节点丢弃（原文行 5025）
void PruneNodes(std::vector<ToolNode>& nodes)
{
    for (ToolNode& n : nodes) PruneNodes(n.children);

    std::vector<ToolNode> kept;
    kept.reserve(nodes.size());
    for (ToolNode& n : nodes) {
        if (n.path.empty() && n.children.empty()) {
            ConsoleWriteLine(L"[-] [" + n.name + L"] 无可用 Path 且无子项, 已忽略");
            continue;
        }
        kept.push_back(std::move(n));
    }
    nodes.swap(kept);
}

}  // namespace

void SetToolField(ToolNode& node, std::wstring key, const std::wstring& value)
{
    for (wchar_t& c : key) c = static_cast<wchar_t>(towlower(c));

    if (key == L"name") {
        node.name = value;
    } else if (key == L"path" || key == L"exe" || key == L"file") {
        node.path = value;
    } else if (key == L"args" || key == L"arguments" || key == L"params") {
        node.args = value;
    } else if (key == L"workdir" || key == L"workingdir" || key == L"dir") {
        node.workdir = value;
    } else if (key == L"description" || key == L"desc" || key == L"comment") {
        node.description = value;
    } else if (key == L"runasadmin" || key == L"admin") {
        node.admin = IsTruthy(value);
    } else if (key == L"newconsole") {
        node.newconsole = IsTruthy(value);
    } else if (key == L"hidewindow" || key == L"hidden") {
        node.hidden = IsTruthy(value);
    } else if (key == L"wait") {
        node.wait = IsTruthy(value) ? 1 : 0;
    } else if (key == L"prompts" || key == L"prompt") {
        // 真实格式（来自 dade\.dade）：
        //   key:提示文字              默认值为空
        //   key:提示文字:默认值        三段式，默认值里可以再含 ':'
        // 多条用 ';' 分隔；args 里用 {key} 引用。
        node.prompts.clear();
        for (const std::wstring& item :
             SplitString(value, [](wchar_t c) { return c == L';'; })) {  // [0x140001480]
            if (Trim(item).empty()) continue;

            const auto parts = SplitString(item, [](wchar_t c) { return c == L':'; });  // [0x140001490]

            std::wstring key0 = Trim(parts[0]);
            std::wstring text = key0;
            std::wstring defval;

            if (parts.size() >= 2) text = Trim(parts[1]);
            if (parts.size() >= 3) {
                for (size_t i = 2; i < parts.size(); ++i) {
                    if (i > 2) defval += L':';
                    defval += parts[i];
                }
                defval = Trim(defval);
            }
            node.prompts.emplace_back(PromptSpec{key0, text, defval});
        }
    }
}

bool LoadCfg(const std::wstring& path, std::vector<ToolNode>& out, std::wstring& err)
{
    out.clear();
    err.clear();

    std::string raw;
    if (!ReadWholeFile(path, raw)) {
        err = L"配置文件读取失败: " + path;
        return false;
    }

    // 去 UTF-8 BOM（原文行 8874）
    if (raw.size() >= 3 && static_cast<unsigned char>(raw[0]) == 0xEF &&
        static_cast<unsigned char>(raw[1]) == 0xBB &&
        static_cast<unsigned char>(raw[2]) == 0xBF) {
        raw.erase(0, 3);
    }

    const std::wstring text = Utf8ToWide(raw);
    if (text.empty()) {
        err = L"配置文件为空";
        return false;
    }

    // 一段 = 一个节点；段名形如 "工具:极域小工具>脱控功能>关闭极域"，
    // "工具:" 之后的部分是"路径"，用 '>' 分层（来自真实 dade\.dade 的结构）。
    struct RawNode {
        std::wstring treePath;  // "极域小工具>脱控功能>关闭极域"；非工具段为空
        ToolNode     node;
    };

    std::vector<RawNode> flat;
    RawNode*             cur = nullptr;

    size_t pos = 0;
    while (pos <= text.size()) {
        size_t nl = text.find(L'\n', pos);
        if (nl == std::wstring::npos) nl = text.size();

        std::wstring line = text.substr(pos, nl - pos);
        pos = (nl == text.size()) ? text.size() + 1 : nl + 1;

        if (!line.empty() && line.back() == L'\r') line.pop_back();
        line = Trim(line);
        if (line.empty()) continue;
        if (line[0] == L'#' || line[0] == L';') continue;  // 注释

        if (line.front() == L'[') {
            const size_t close = line.find(L']');
            if (close == std::wstring::npos) continue;

            RawNode entry;
            entry.node.section = Trim(line.substr(1, close - 1));

            const size_t prefixLen = std::wcslen(app::kGroupPrefix);
            if (entry.node.section.rfind(app::kGroupPrefix, 0) == 0)
                entry.treePath = Trim(entry.node.section.substr(prefixLen));

            // 显示名默认取路径最后一段
            const size_t bar = entry.treePath.find_last_of(L'>');
            entry.node.name = (bar == std::wstring::npos) ? entry.treePath
                                                          : Trim(entry.treePath.substr(bar + 1));

            flat.push_back(std::move(entry));
            cur = &flat.back();
            continue;
        }

        if (cur == nullptr) continue;
        const size_t eq = line.find(L'=');
        if (eq == std::wstring::npos) continue;

        SetToolField(cur->node, Trim(line.substr(0, eq)), Trim(line.substr(eq + 1)));
    }

    if (flat.empty()) {
        err = L"配置里没有任何 [段]";
        return false;
    }

    // 节点校验（[0x140004840]）：hidden 必须配 newconsole
    for (RawNode& item : flat) {
        ToolNode& n = item.node;
        if (n.hidden && !n.newconsole) {
            ConsoleWriteLine(L"[-] [" + n.name + L"] HideWindow=1 需 NewConsole=1, 已忽略 HideWindow");
            n.hidden = false;
        }
    }

    // 按路径建树：path -> 下标；父路径 = 去掉最后一个 '>' 段
    std::unordered_map<std::wstring, size_t> byPath;
    for (size_t i = 0; i < flat.size(); ++i) {
        if (!flat[i].treePath.empty()) byPath.emplace(flat[i].treePath, i);
    }

    std::vector<std::vector<size_t>> childIdx(flat.size());
    std::vector<size_t>              roots;

    for (size_t i = 0; i < flat.size(); ++i) {
        const std::wstring& treePath = flat[i].treePath;
        if (treePath.empty()) {
            roots.push_back(i);
            continue;
        }
        const size_t bar = treePath.find_last_of(L'>');
        if (bar == std::wstring::npos) {
            roots.push_back(i);
            continue;
        }
        const std::wstring parentPath = Trim(treePath.substr(0, bar));
        const auto         it         = byPath.find(parentPath);
        if (it == byPath.end()) {
            ConsoleWriteLine(L"[-] 子项 [" + flat[i].node.name + L"] 找不到父路径 [" + parentPath + L"]");
            roots.push_back(i);
            continue;
        }
        childIdx[it->second].push_back(i);
    }

    // 递归搬运成值树（避免指针在 vector 扩容后失效）
    std::function<ToolNode(size_t)> materialize = [&](size_t idx) {
        ToolNode node = std::move(flat[idx].node);
        for (size_t child : childIdx[idx]) node.children.push_back(materialize(child));
        return node;
    };

    out.clear();
    out.reserve(roots.size());
    for (size_t root : roots) out.push_back(materialize(root));

    // 既没有 path、又没有子项的节点丢弃（原文行 5025，这里递归处理子层）
    PruneNodes(out);
    return true;
}
