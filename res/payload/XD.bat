@echo off
setlocal

for %%i in (1,2,3,4,5,6,7,8,9,A,B,C,D,E,F) do (
    start "XD - color %%i" "C:\Windows\System32\cmd.exe" "/c color %%i && dir /s /a C:\"
    timeout 1 >nul
)

endlocal