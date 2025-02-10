rem Args are BUILD_TYPE  APP_NAME  APP_EXT  CMPLR_FLAGS
rem Use . to skip args if in the middle

set SRCPATH=%~dp0
call %SRCPATH%\FRAMEWORK\COMPILE\COMPILE_BY_CFG.bat DBG_WIN_USR_X86_X64 TestApp .exe . -DHELLO_DEF

rem pause