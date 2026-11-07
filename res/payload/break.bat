@echo off

chcp 65001 && cls

tasklist | find "StudentMain.exe" >nul 2>&1
if errorlevel 1 (
    echo [-] 已经脱控! 双击桌面上的极域图标以恢复控制!
    pause
) else (
    taskkill /F /im StudentMain.exe 1>nul
    if errorlevel 1 (
        echo [-] 脱控失败! 请确认错误提示并尝试其他脱控方式!
    ) else (
        echo [+] 脱控成功! 双击桌面上的极域图标以恢复控制!
    )
    pause
)