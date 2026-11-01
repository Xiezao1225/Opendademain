// config.h -- dade\.dade 的 INI 解析
#pragma once

#include "common.h"

#include <string>
#include <vector>

// 按谓词切分（对应原文 FUN_14000b450）
template <class Pred>
std::vector<std::wstring> SplitString(const std::wstring& s, Pred isSep)
{
    std::vector<std::wstring> out;
    std::wstring cur;
    for (const wchar_t c : s) {
        if (isSep(c)) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    out.push_back(cur);
    return out;
}

// 键 -> 字段（对应原文 FUN_14000b610）
void SetToolField(ToolNode& node, std::wstring key, const std::wstring& value);

// 解析配置（对应原文 LoadCfg）
bool LoadCfg(const std::wstring& path, std::vector<ToolNode>& out, std::wstring& err);
