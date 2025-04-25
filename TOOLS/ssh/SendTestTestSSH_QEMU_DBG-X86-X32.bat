set SRCFILE=..\BUILD\BIN\DBG_LIN_USR_X86_X32\testapp
set DSTFILE=/tmp/test
set WORKDIR=%~dp0
set SSHTIP=127.0.0.1
set SSHLOGIN=guest
set SSHPASSW=guest
set PUTTYPATH=C:\_TOOLS_\_NETWORK_\KiTTY\

.\base64.exe --output="%WORKDIR%%SRCFILE%.x64" "%WORKDIR%%SRCFILE%"
"%PUTTYPATH%klink.exe" -no-antispoof -auto-store-sshkey -batch -ssh -l %SSHLOGIN% -P 5022 -pw %SSHPASSW% %SSHLOGIN%@%SSHTIP% "cat | base64 -d > %DSTFILE%;chmod 777 %DSTFILE%" < "%WORKDIR%%SRCFILE%.x64"
del "%WORKDIR%%SRCFILE%.x64"

rem pause