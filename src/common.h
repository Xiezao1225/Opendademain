// common.h -- 公共类型与常量
// dademain.exe（极域工具箱）的可编译重实现。
// 语义来源见 ../FUCKUPXdademain_分析报告.md 与 dademain_app.c。
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <shellapi.h>

#include <string>
#include <utility>
#include <vector>

// 交互参数：key 用于 args 里的 {key} 占位；text 是提示语；defval 可为空。
struct PromptSpec {
    std::wstring key;     // 占位符名，例如 "ip"
    std::wstring text;    // 提示语，例如 "指定ip地址 [默认全员]"
    std::wstring defval;  // 默认值，可为空
};

// 对应反编译里 stride = 0xF8 的那个结构体（字段顺序与偏移一致）。
struct ToolNode {
    std::wstring section;      // +0x00  段名（原始标识）
    std::wstring name;         // +0x20  name=
    std::wstring path;         // +0x40  path= / exe= / file=
    std::wstring args;         // +0x60  args= / arguments= / params=
    std::wstring workdir;      // +0x80  workdir= / workingdir= / dir=
    std::wstring description;  // +0xa0  description= / desc= / comment=
    bool         admin      = false;  // +0xc0  runasadmin= / admin=
    bool         newconsole = false;  // +0xc1  newconsole=
    bool         hidden     = false;  // +0xc2  hidewindow= / hidden=
    int          wait       = -1;     // +0xc4  wait=  (-1 未设置 / 0 否 / 1 是)

    std::vector<ToolNode>  children;  // +0xc8  子菜单
    std::vector<PromptSpec> prompts;  // +0xe0  交互参数
};

namespace app {

// 原文散落在 .rdata、被 Ghidra 渲染成 DAT_1400f**** 的字符串
inline constexpr wchar_t kTitle[]       = L"dademain.exe - 极域工具箱";
inline constexpr wchar_t kGroupPrefix[] = L"工具:";       // 段名前缀 = 菜单分组
inline constexpr wchar_t kDataDir[]     = L"dade";        // 数据目录 dade
inline constexpr wchar_t kConfigName[]  = L".dade";       // dade/.dade
inline constexpr wchar_t kVersionName[] = L".version";    // dade/.version
inline constexpr wchar_t kVersion[]     = L"1.0.0.0";
inline constexpr wchar_t kEulaName[]    = L"eula.txt";
inline constexpr wchar_t kReadmeName[]  = L"readme.html";

// 资源名（res/dademain.rc）
inline constexpr wchar_t kManifestRes[] = L"TOOLS_MANIFEST";
inline constexpr wchar_t kReadmeRes[]   = L"README_HTML";
inline constexpr wchar_t kConfigRes[]   = L"CFG_DADE";

inline constexpr wchar_t kBanner1[] =
    L"==================== dademain.exe - 极域工具箱 Cracked ezzzz========\n";

	inline constexpr wchar_t kBanner2[] =
	L"    powered        __               __      \n"
	L"        by    ____/ /  ____ _  ____/ /  ___ \n"
	L"             / __  /  / __ `/ / __  /  / _ \\\n"
	L"            / /_/ /  / /_/ / / /_/ /  /  __/\n"
	L"            \\__,_/   \\__,_/  \\__,_/   \\___/\n"
	L"\n"
	L"cracked by\n"
	L"                                         \n"
	L"*@@@*   *@@*   @@                                        \n"
	L"  @@@m  m@                                              \n"
	L"   *@@m@*    *@@@    mm@*@@  @***@@@  m@*@@m    m@@*@@m \n"
	L"     @@@       @@   m@*   @@ *  @@@  @@   @@   @@*   *@@\n"
	L"   m@**@@m     !@   !@******   @@!    m@@@!@   @@     @@\n"
	L"   @   *@@m    !@   !@m    m  !@@    @!   !@   @@     !@\n"
	L"   !!   !!!    !!   !!******  !@!     !!!!:!   !@     !!\n"
	L"  !!    *!!!   !!   :!!      !!!   ! !!   :!   !!!   !!!\n"
	L": : :    : ::: : :   : : ::  : :!:!: :!: : !:   : : : :\n";

inline constexpr wchar_t kRule[] =
    L"-------------------------------------------------------------------\n";

}  // namespace app
