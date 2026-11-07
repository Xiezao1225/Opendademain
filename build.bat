@echo off
setlocal EnableDelayedExpansion

set "ROOT=%~dp0"
set "BUILD=%ROOT%build"

rem ---- locate MSVC ----
rem 依次尝试：常见安装位置 -> vswhere（任意盘符/任意版本）。
rem 不要在这里写死本机路径，否则换台机器就得改脚本。
set "VCVARS="
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

if not defined VCVARS (
  for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath 2^>nul`) do (
    if exist "%%i\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%%i\VC\Auxiliary\Build\vcvars64.bat"
  )
)

if not defined VCVARS (
  echo [x] vcvars64.bat not found. Install Visual Studio C++ build tools.
  exit /b 1
)

echo [*] MSVC env: %VCVARS%
call "%VCVARS%" >nul
if errorlevel 1 (
  echo [x] failed to initialize MSVC environment.
  exit /b 1
)

rem ---- cmake ----
where cmake >nul 2>nul
if errorlevel 1 (
  echo [x] cmake not found in PATH.
  exit /b 1
)

echo [*] configuring ...
cmake -S "%ROOT%." -B "%BUILD%" -G Ninja -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1

rem ---- force resource regeneration ----
rem Copied payloads often have older timestamps, so Ninja may skip
rem regeneration and the new payloads never reach the exe.
if exist "%BUILD%\tools_manifest.txt" del /q "%BUILD%\tools_manifest.txt"
if exist "%BUILD%\dademain.rc" del /q "%BUILD%\dademain.rc"

echo [*] building ...
cmake --build "%BUILD%"
if errorlevel 1 exit /b 1

echo.
echo [+] done: %BUILD%\bin\dademain.exe
endlocal
