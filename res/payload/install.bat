@echo off
setlocal EnableExtensions EnableDelayedExpansion

chcp 65001 && cls

:input_path
cls

echo ===================== 工具安装 =====================
echo     powered        __               __      
echo         by    ____/ /  ____ _  ____/ /  ___ 
echo              / __  /  / __ `/ / __  /  / _ \
echo             / /_/ /  / /_/ / / /_/ /  /  __/
echo             \__,_/   \__,_/  \__,_/   \___/
echo.
echo ----------------------————----------------------

set "dst="
set /p "dst=请输入安装路径: "
if not defined dst (
    echo [-] 未输入路径!
    pause
    goto input_path
)

if "%dst:~-1%"=="\" set "dst=%dst:~0,-1%"

if not exist "%dst%\" (
    echo [-] 路径不存在!
    pause
    goto input_path
)

if exist "%dst%\dademain.exe" (
    set /p "choice=[-] 目标路径已安装过, 是否卸载重装? (Y/N): "
    if /i "!choice!"=="Y" (
        if exist "%dst%\dade" (
            del /s /q "%dst%\dade\*" >nul
            if errorlevel 1 (
                echo [-] 卸载失败!
                pause
                exit /b 1
            )
            rmdir /s /q "%dst%\dade" >nul
            if errorlevel 1 (
                echo [-] 卸载失败!
                pause
                exit /b 1
            )
        )
        del /q "%dst%\dademain.exe" >nul
        if errorlevel 1 (
            echo [-] 卸载失败!
            pause
            exit /b 1
        )
        echo [+] 卸载完成!
    ) else (
        echo [-] 重装已取消!
        pause
        exit /b 1
    )
)

if exist "%dst%\dade" (
    set /p "choice=[-] 目标路径已存在 dade 文件夹, 本程序需要使用该文件夹, 可能导致未知错误, 是否继续安装? (Y/N): "
    if /i "!choice!"=="N" (
        echo [-] 安装已取消!
        pause
        exit /b 1
    )
)

copy /y "%~dp0..\dademain.exe" "%dst%\dademain.exe" >nul
if errorlevel 1 (
    echo [-] 安装失败!
    pause
    exit /b 1
)

if not exist "%dst%\dade" mkdir "%dst%\dade" >nul

xcopy "%~dp0..\dade\*" "%dst%\dade\" /E /I /Y /H /K >nul
if errorlevel 1 (
    echo [-] 安装失败!
    pause
    exit /b 1
)

echo [+] 安装完成!
pause

endlocal