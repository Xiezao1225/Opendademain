@echo off
REM 示例载荷：验证"任意文件都会被嵌入并释放到 dade\ 下"
chcp 65001 >nul
echo ==========================================
echo   hello from the embedded payload
echo ==========================================
echo   args : %*
echo   cwd  : %CD%
echo   file : %~f0
echo.
echo Press any key to close this window ...
pause >nul
