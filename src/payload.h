// payload.h -- 自解压 / 帮助文档
#pragma once

#include <string>

// 按 TOOLS_MANIFEST 释放资源到 <exe>\dade\ 下（对应原文 FUN_1400056a0）
bool ExtractPayload(const std::wstring& dataDir);

// 释放 README_HTML 为 readme.html 并打开（对应原文 FUN_140005370）
bool ShowReadme(const std::wstring& exeDir, bool verbose);
