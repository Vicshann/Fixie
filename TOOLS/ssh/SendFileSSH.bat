set SRCFILE=gdbserver
set DSTFILE=/tmp/gdbserver
set WORKDIR=%~dp0
set SSHTIP=192.168.78.103
set SSHLOGIN=miner
set SSHPASSW=miner
set PUTTYPATH=C:\_TOOLS_\_NETWORK_\KiTTY\

.\base64.exe --output="%WORKDIR%%SRCFILE%.x64" "%WORKDIR%%SRCFILE%"
"%PUTTYPATH%klink.exe" -ssh -l %SSHLOGIN% -P 22 -pw %SSHPASSW% %SSHLOGIN%@%SSHTIP% "cat | base64 -d > %DSTFILE%;chmod 777 %DSTFILE%" < "%WORKDIR%%SRCFILE%.x64"
del "%WORKDIR%%SRCFILE%.x64"

pause