@echo off
setlocal EnableExtensions EnableDelayedExpansion

chcp 65001 && cls

:input_path
cls

echo ===================== 安装部署 =====================
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
        del /q "%dst%\dademain.exe" >nul
        if errorlevel 1 (
            echo [-] 卸载失败!
            pause
            exit /b 1
        )
        if exist "%dst%\一键部署.bat" (
            del /q "%dst%\一键部署.bat" >nul
            if errorlevel 1 (
                echo [-] 卸载失败!
                pause
                exit /b 1
            )
        )
        echo [+] 卸载完成!
    ) else (
        echo [-] 重装已取消!
        pause
        exit /b 1
    )
) else (
    if exist "%dst%\一键部署.bat" (
        set /p "choice=[-] 目标路径已安装过, 是否卸载重装? (Y/N): "
        if /i "!choice!"=="Y" (
            del /q "%dst%\一键部署.bat" >nul
            if errorlevel 1 (
                echo [-] 卸载失败!
                pause
                exit /b 1
            )
            echo [+] 卸载完成!
        )
    )
)

copy /y "%~dp0..\dademain.exe" "%dst%\dademain.exe" >nul
if errorlevel 1 (
    echo [-] 安装失败!
    pause
    exit /b 1
)

copy /y "%~dp0一键部署.bat" "%dst%\一键部署.bat" >nul
if errorlevel 1 (
    echo [-] 安装失败!
    pause
    exit /b 1
)

echo [+] 安装完成!
pause

endlocal