set SRCFILE=..\BUILD\app.so
set DSTFILE=/tmp/test
set WORKDIR=%~dp0
set SSHTIP=127.0.0.1
set SSHLOGIN=pi
set SSHPASSW=raspberry
set PUTTYPATH=C:\_TOOLS_\_NETWORK_\KiTTY\

.\base64.exe --output="%WORKDIR%%SRCFILE%.x64" "%WORKDIR%%SRCFILE%"
"%PUTTYPATH%klink.exe" -no-antispoof -auto-store-sshkey -batch -ssh -l %SSHLOGIN% -P 5022 -pw %SSHPASSW% %SSHLOGIN%@%SSHTIP% "cat | base64 -d > %DSTFILE%;chmod 777 %DSTFILE%" < "%WORKDIR%%SRCFILE%.x64"
del "%WORKDIR%%SRCFILE%.x64"

rem pause