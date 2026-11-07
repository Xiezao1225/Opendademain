# test/smoke.ps1 —— 冒烟测试：在临时目录里用 --safe 跑一遍主流程。
#
# 覆盖：菜单渲染、无效输入、进入子菜单并返回、!r 还原配置 + 自解包、
#       r 重新加载配置、退出；以及 .version 内容、.dade.bak 备份、载荷释放结果。
# 不覆盖：首次运行门禁（会打开浏览器）、Prompts 交互、UAC / UIAccess、实际启动工具。
#
# 用法（在仓库根目录）：
#   powershell -NoProfile -ExecutionPolicy Bypass -File test\smoke.ps1
#   powershell -NoProfile -ExecutionPolicy Bypass -File test\smoke.ps1 -Exe build\bin\dademain.exe
#
# 退出码：0 = 全部通过；1 = 有失败项或前置条件不满足。

[CmdletBinding()]
param(
    [string]$Exe = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrEmpty($Exe)) { $Exe = Join-Path $PSScriptRoot '..\build\bin\dademain.exe' }
$exe = if ([System.IO.Path]::IsPathRooted($Exe)) { $Exe } else { [System.IO.Path]::GetFullPath((Join-Path (Get-Location) $Exe)) }

if (-not (Test-Path $exe)) {
    Write-Host "[x] 找不到 $exe —— 请先运行 build.bat" -ForegroundColor Red
    exit 1
}

$defaultCfg = Join-Path $repo 'res\default.dade'
if (-not (Test-Path $defaultCfg)) {
    Write-Host "[x] 找不到 $defaultCfg" -ForegroundColor Red
    exit 1
}

$tmp = Join-Path $repo ('build\.smoke-' + [guid]::NewGuid().ToString('N'))
$utf8 = New-Object System.Text.UTF8Encoding($false)
$script:pass = 0
$script:fail = 0

function Check([string]$name, [bool]$ok, [string]$detail) {
    if ($ok) {
        Write-Host ("[+] " + $name) -ForegroundColor Green
        $script:pass++
    } else {
        Write-Host ("[-] " + $name + "  " + $detail) -ForegroundColor Red
        $script:fail++
    }
}

try {
    [System.IO.Directory]::CreateDirectory($tmp) | Out-Null
    New-Item -ItemType Directory (Join-Path $tmp 'dade') | Out-Null
    Copy-Item $exe (Join-Path $tmp 'dademain.exe')
    Copy-Item $defaultCfg (Join-Path $tmp 'dade\.dade')

    # 预置 .version 以跳过首次运行门禁（门禁会打开 readme.html 并 pause）
    [System.IO.File]::WriteAllText((Join-Path $tmp 'dade\.version'), '1.0.0.0', [System.Text.Encoding]::ASCII)

    # 用 ProcessStartInfo 而不是 Start-Process：本机实测 Start-Process 的
    # -RedirectStandardInput 会让子进程立刻以 -1 退出，且拿不到任何输出。
    #
    # 另外：刚复制出来的 exe 第一次启动会被安全软件拦下（退出码 -1、无输出），
    # 等几秒重试即可；脚本里最多重试 3 次。
    $out = ''
    $err = ''
    $proc = $null
    for ($attempt = 1; $attempt -le 3; $attempt++) {
        $psi = New-Object System.Diagnostics.ProcessStartInfo
        $psi.FileName               = Join-Path $tmp 'dademain.exe'
        $psi.Arguments              = '--safe'
        $psi.WorkingDirectory       = $tmp
        $psi.UseShellExecute        = $false
        $psi.RedirectStandardInput  = $true
        $psi.RedirectStandardOutput = $true
        $psi.RedirectStandardError  = $true
        $psi.StandardOutputEncoding = $utf8

        $proc = [System.Diagnostics.Process]::Start($psi)
        $proc.StandardInput.Write("xyz`n1`n0`n!r`nr`nq`n")
        $proc.StandardInput.Close()
        $out = $proc.StandardOutput.ReadToEnd()
        $err = $proc.StandardError.ReadToEnd()
        $proc.WaitForExit()

        if ($proc.ExitCode -ne -1 -or $out.Length -gt 0) { break }
        Write-Host ("[*] 第 {0} 次启动被安全软件拦下，等待后重试 ..." -f $attempt) -ForegroundColor DarkYellow
        Start-Sleep -Seconds 3
    }

    if ($err.Length -gt 0) { Write-Host ("[stderr] " + $err) -ForegroundColor DarkYellow }
    $dade = Join-Path $tmp 'dade'

    Check '进程退出码为 0' ($proc.ExitCode -eq 0) ("实际为 " + $proc.ExitCode)
    Check '主菜单至少有 4 个顶层分组' (([regex]::Matches($out, '(?m)^  \[\d\] ')).Count -ge 4) '没有找到菜单项'
    Check '无效输入被拒绝' ($out.Contains('[-] 无效输入, 请重新选择')) '缺少提示'
    Check '子菜单标题正确' ($out.Contains('工具: 安装与部署')) '缺少子菜单标题'
    Check '!r 还原内置配置' ($out.Contains('已还原为内置默认配置')) '缺少还原提示'
    Check 'r 重新加载配置' ($out.Contains('配置已成功重新加载')) '缺少重载提示'
    Check '自解包报告释放文件数' ($out -match '已释放 \d+ 个文件') '缺少释放提示'
    Check '.version 为 7 字节' ((Test-Path (Join-Path $dade '.version')) -and ((Get-Item (Join-Path $dade '.version')).Length -eq 7)) '长度不对或文件缺失'
    Check '还原前配置已备份为 .dade.bak' (Test-Path (Join-Path $dade '.dade.bak')) '.dade.bak 缺失'
    Check '默认配置已重新释放' (Test-Path (Join-Path $dade '.dade')) '.dade 缺失'
    Check '顶层载荷 hello.bat 已释放' (Test-Path (Join-Path $dade 'hello.bat')) 'hello.bat 缺失'
    Check '子目录载荷 tools\whoami.bat 已释放' (Test-Path (Join-Path $dade 'tools\whoami.bat')) 'whoami.bat 缺失'
    Check '正常退出' ($out.TrimEnd().EndsWith('bye.')) '结尾不是 bye.'
}
finally {
    if (Test-Path $tmp) { Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue }
}

Write-Host ''
if ($script:fail -eq 0) {
    Write-Host ("全部通过：{0} 项" -f $script:pass) -ForegroundColor Green
    exit 0
}
Write-Host ("失败 {0} 项，通过 {1} 项" -f $script:fail, $script:pass) -ForegroundColor Red
exit 1
