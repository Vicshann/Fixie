set SRCFILE=/etc/localtime
set DSTFILE=localtime
set WORKDIR=%~dp0
set SSHTIP=127.0.0.1
set SSHLOGIN=pi
set SSHPASSW=raspberry
set PUTTYPATH=C:\_TOOLS_\_NETWORK_\KiTTY\

"%PUTTYPATH%klink.exe" -ssh -l %SSHLOGIN% -P 5022 -pw %SSHPASSW% %SSHLOGIN%@%SSHTIP% "cat %SRCFILE% | base64" > "%WORKDIR%%DSTFILE%.x64"
rem NOTE: Windows terminal is too smart and will add 0D before each 0A but only in binary files - do not save using '> file'
base64.exe -i -d --output="%WORKDIR%%DSTFILE%" "%WORKDIR%%DSTFILE%.x64" 
del "%WORKDIR%%DSTFILE%.x64"

rem pause

