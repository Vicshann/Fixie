rem Use . to skip args if in the middle

set SRCPATH=%~dp0
call "%SRCPATH%\..\FRAMEWORK\COMPILE\BUILD_BY_CFG.bat" DBG_WIN_USR_X86_X64 TestApp .exe . . . -DHELLO_DEF

rem pause