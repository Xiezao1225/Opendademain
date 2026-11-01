@echo off
REM 子目录示例载荷：验证 server\tools\ 这类层级也会被原样创建
chcp 65001 >nul
echo [demo] 当前用户: %USERNAME%
echo [demo] 计算机名: %COMPUTERNAME%
echo [demo] 时间    : %DATE% %TIME%
echo [demo] 所在目录: %CD%
echo.
pause >nul
