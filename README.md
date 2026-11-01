# OpenDademain

`dademain.exe`（"极域工具箱"）的开源重实现 —— 从反编译结果读通行为后，用 C++17 重写的
**可编译、可审计**版本。

原程序是闭源的 7.8 MB 单文件，业务逻辑只有 8,670 行（其余是 MSVC CRT/STL），核心是
"自解压 + 配置驱动 + 提权 + 菜单执行"。本仓库把这部分完整实现了一遍，并把"载荷"做成
**可配置的通用机制**（`res/payload/` 下放什么就打包什么），而不是硬编码。

## 特点

- **可编译**：MSVC 2022 + CMake + Ninja，一条 `build.bat`，约 14 秒，零警告
- **可审计**：约 1,700 行 C++17，没有第三方依赖，只链 `shell32/advapi32/user32`
- **行为对齐**：`.dade` 的 `工具:A>B>C` 菜单树、`prompts` 交互、`runas`/`newconsole`/
  `hidden`/`wait` 语义，都与原版实测产物逐项比对过
- **不夹带**：仓库里**不含** `udpatk.exe` / `miniweb.exe` 等第三方二进制

## 相关材料（在仓库外）

- 逆向分析报告：[`../src/FUCKUPXdademain_分析报告.md`](../src/FUCKUPXdademain_分析报告.md)
- 逐字提取的作者代码：[`../src/dademain_app.c`](../src/dademain_app.c)
- 语义还原版单文件：[`../src/dademain_main.cpp`](../src/dademain_main.cpp)

> **用途声明**：本项目用于逆向工程学习、代码审计与教学研究。请勿在未经授权的机器上运行。
> 仓库不含任何攻击载荷；`res/payload/` 里是自写的无害示例脚本。

> **许可证**：尚未指定。如需分发，建议先补一个（MIT 最简），并确认 `res/payload/`
> 里的内容你都有权分发。

---

## 1. 构建

要求：Windows 10/11 + Visual Studio 2022（C++ 生成工具）+ CMake ≥ 3.20 + Ninja。

```bat
build.bat
```

脚本会自动定位 `vcvars64.bat`，然后执行：

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

产物：`build\bin\dademain.exe`（约 124 KB，只链 `shell32/advapi32/user32`）。

也可以手动构建（任意生成器）：

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

> **构建注意**：改动 `src/common.h` 里的 `ToolNode` 结构后请做一次干净构建
> （删掉 `build\` 重跑）。增量构建若残留旧目标文件，会出现"存了某些字段就崩"的
> ABI 不一致问题——我在开发时就踩到过一次。

## 2. 目录结构

```
dademain/
├─ CMakeLists.txt
├─ build.bat
├─ cmake/gen_res.cmake    扫描 res/payload/ 生成 .rc 与 TOOLS_MANIFEST
├─ src/
│  ├─ common.h      ToolNode / PromptSpec 与常量
│  ├─ console.*     控制台读写（WriteConsoleW；重定向时按行读字节）
│  ├─ util.*        路径拼接 / 整文件读写 / UTF-8 转换 / 建目录
│  ├─ config.*      .dade 解析（"工具:A>B>C" 建树、键别名、prompts）
│  ├─ payload.*     按 TOOLS_MANIFEST 自解压；释放并打开 readme.html
│  ├─ elevate.*     IsElevated / GetSystemToken / TryGetUIAccess / 热键线程
│  ├─ runner.*      RunTools：子菜单、参数交互、拼命令行、runas、等待
│  └─ main.cpp      主流程 + 菜单循环
└─ res/
   ├─ default.dade        CFG_DADE：默认配置
   ├─ readme.html         README_HTML
   └─ payload/            ← 放这里的**所有文件**都会被嵌入
      ├─ hello.bat
      └─ tools\whoami.bat
```

## 3. 运行

首次运行会在 exe 同级创建：

```
dademain.exe
eula.txt                  首次运行生成，内容里 eula=true 才放行
dade\
├─ .dade                  工具清单（从内置资源释放，可手工修改）
├─ .version               "1.0.0.0"，用于判断是否需要重新解压
├─ readme.html
├─ hello.bat
└─ tools\whoami.bat       子目录层级会被自动创建
```

**工具路径与工作目录都相对 `dade\`**（与原程序一致），不是 exe 目录。

调试时加 `--safe` 跳过 UAC 与 UIAccess（否则可能弹 UAC、尝试窃取 SYSTEM 令牌、重启自身）：

```bat
dademain.exe --safe
```

### 菜单命令

| 输入 | 作用 |
|---|---|
| `序号` | 执行对应工具 / 进入子菜单 |
| `0` | 子菜单里返回上级 |
| `r` / `reload` | 重新读取配置 |
| `!r` / `restore` | 备份 `.dade` 后还原内置默认配置并重新解压 |
| `h` / `help` / `?` | 打开 readme.html |
| `q` / `Q` / `exit` | 退出 |

## 4. 增加自己的载荷（bat / exe / ps1）

原版的做法是"资源 + 清单"：把 15 个文件塞进 `.rsrc`，首次运行按清单释放。
本工程沿用同一机制，但改成**构建期自动生成**，加载荷不用碰一行代码：

1. 把文件丢进 `res/payload/`（可以带子目录，扩展名随意）：

   ```
   res/payload/
   ├─ hello.bat
   ├─ tools\whoami.bat
   └─ bin\mytool.exe
   ```

2. 重新构建：

   ```bat
   build.bat
   ```

   `cmake/gen_res.cmake` 会扫描该目录，为每个文件生成一条 `PAYLOAD_n RCDATA`，
   并写出配套的 `TOOLS_MANIFEST`（在 `build\` 下，可直接打开查看）：

   ```
   CFG_DADE|.dade
   PAYLOAD_1|hello.bat
   PAYLOAD_2|tools/whoami.bat
   PAYLOAD_3|bin/mytool.exe
   ```

3. 首次运行（或删掉 `dade\.version` 后运行）就会释放到 `dade\` 下的对应路径。

细节：

- 清单里的 `/` 会被换成 `\`，缺失的目录自动创建；
- **以 `.dade` 结尾的目标若已存在则跳过**（保护用户改过的配置），其它文件一律覆盖；
- 全部释放成功后才写 `.version`，中途失败下次会重来；
- 想强制重发：主菜单输入 `!r`，或删掉 `dade\.version`。

在 `.dade` 里引用它们时，`Path` 相对 `dade\`：

```ini
[工具:演示>查看本机信息]
Name=查看本机信息
Path=tools\whoami.bat
NewConsole=1
Wait=1
```

---

## 5. 配置格式（`dade\.dade`）

UTF-8（可带 BOM）的 INI 风格文本；`#` 或 `;` 开头为注释；`[段名]` 开段。

### 段名 = 菜单路径

段名是 **`工具:` + 路径**，路径用 `>` 分层，这就是菜单树的来源：

```ini
[工具:极域小工具]                 ; 顶层分组
Name=极域小工具

[工具:极域小工具>脱控功能]         ; 上一层是 极域小工具
Name=脱控功能

[工具:极域小工具>脱控功能>关闭极域]  ; 再下一层
Name=关闭极域
Path=break.bat
RunAsAdmin=1
Wait=1
```

- 子节点的父路径 = 去掉最后一个 `>` 段；找不到父节点会打印
  `[-] 子项 [X] 找不到父路径 [Y]` 并降到顶层。
- 既没有 `Path` 又没有子节点的段会被丢弃：`[-] [X] 无可用 Path 且无子项, 已忽略`。
- `Name` 不写时默认取路径最后一段。

### 键（大小写不敏感）

| 键 | 别名 | 含义 |
|---|---|---|
| `Name` | | 显示名 |
| `Path` | `Exe` `File` | 可执行文件，相对 `dade\`；不存在时按原样交给系统解析（支持 `cmd.exe` 这种裸名字） |
| `Args` | `Arguments` `Params` | 命令行参数，支持 `{prompt键}` |
| `WorkDir` | `WorkingDir` `Dir` | 工作目录，相对 `dade\` |
| `Description` | `Desc` `Comment` | 描述，菜单里显示为 `名字 - 描述` |
| `RunAsAdmin` | `Admin` | 需要管理员：未提权时走 `ShellExecuteExW("runas")` |
| `NewConsole` | | 子进程使用 `CREATE_NEW_CONSOLE` |
| `HideWindow` | `Hidden` | `STARTF_USESHOWWINDOW` + `SW_HIDE`（需同时 `NewConsole=1`） |
| `Wait` | | `1` 等待子进程结束；`0` 不等；不写则"开新控制台时才等" |
| `Prompts` | `Prompt` | 交互参数，见下 |

### Prompts

格式 `键:提示[:默认值]`，多条用 `;` 分隔：

```ini
Prompts=ip:指定ip地址 [默认全员]
Args=-c "cmd /c shutdown -s -t 0" -ip {ip}

Prompts=port:web 端口:80;dir:共享目录:.\
Args=... --port {port} --dir {dir}
```

- `{键}` 会被用户输入替换；直接回车用默认值；输入 `back` / `exit` 放弃执行。
- 默认值为空且提示里已带 `[...]` 时，只显示提示语。

## 6. 与原始二进制的差异

均为有意为之，源码注释里都标了原因：

1. **`--safe` 开关**：一键跳过 UAC / UIAccess，方便本地调试。
2. **UIAccess 重启后父进程退出**：原实现 `CreateProcessAsUserW` 成功后会留下两个实例。
3. **控制台 I/O 用 `ReadConsoleW` / `WriteConsoleW`**：避免 `std::wcin` 在重定向输入、
   中文输入下的编码问题；输入被重定向时按字节读、按行切、UTF-8 解码。
4. **`eula.txt` 按行解析**：原实现是"全文找第一个 `eula` 再找下一个 `=`"，
   说明文字里出现 `eula` 就会误判。
5. **`hidden` 校验与递归剪枝**：原实现的"无 Path 且无子项则丢弃"只处理了顶层，
   这里连子层一起剪。
6. **`!r` 真正能还原配置**：原实现只是清空 `.version`，但解压器会跳过已存在的 `.dade`，
   所以它其实还原不了配置；这里先备份成 `.dade.bak` 再删除。
7. **`Path` 不存在时才走系统解析**：`lpApplicationName` 非空时 Windows 不搜 `PATH`，
   原实现因此无法启动 `cmd.exe` 这类裸名字。
8. **文本编码**：`.version` 写成 ASCII（7 字节，与原文一致），`eula.txt` 写成 UTF-8。

## 7. 已验证的行为

Windows 11 + MSVC 19.51 实测（14 项自动验收全过）：

- 首次运行：写 `eula.txt` → 打开 `readme.html` → `pause` → `eula=true` 放行
- 自解压：释放 `.dade` + `tools\hello.bat`，写 7 字节 `.version`；二次运行跳过
- 菜单树：**用原程序释放的真实 `.dade`（28 段 / 4 层）验证**，菜单结构与之一致
  （安装与部署 / 极域小工具 / 系统小工具 / 神秘小工具，及
  极域小工具 > 重放攻击 > 快捷攻击 > 关机打击）
- 子菜单、`0` 返回、无效输入提示、空配置提示
- `Prompts` 三段式交互 → `{占位符}` 替换 → `CreateProcessW` 启动 →
  `WorkDir` 生效（`cwd: ...\dade\tools`）→ `Wait` 等待 → 子进程输出回显
- `r` 重载、`!r` 还原（备份 `.dade.bak`）、`--safe` 跳过提权
- 全程无内存错误（Release + ASan 构建跑过一遍）

## 8. 已知限制

- 只实现消费端：没有"新增/编辑工具"的 UI，改配置需手工编辑 `.dade`。
- 没有签名校验（与原程序一致），`.dade` 和载荷都是明文。
- 未实现原版的"帮助文档内嵌浏览器"之外的花样（例如原程序自带的一些显示细节）。
