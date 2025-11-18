rem Call this with an example source file

@echo off
setlocal enabledelayedexpansion

set CurPath=%~dp0
set SrcFile=%1
set BuildPath="%TEMP%.\_BUILD\"

echo BuildPath is: !BuildPath!

rem Check if SrcFile contains backslash
set "temp=!SrcFile:\=!"
if "!temp!"=="!SrcFile!" (
    rem No backslash found, check forward slash
    set "temp2=!SrcFile:/=!"
    if "!temp2!"=="!SrcFile!" (
        rem No slash found, add current directory prefix
        set "SrcFile=%CurPath%%SrcFile%"
    )
)

echo SrcFile is: !SrcFile!

rem Extract just the file name without path and extension
for %%F in ("%SrcFile%") do (
  set "FileName=%%~nF"
)

echo FileName is: !FileName!

rem mkdir !BuildPath!

call "%CurPath%\..\COMPILE\COMPILE_BY_CFG.bat" DBG_WIN_USR_X86_X64 %FileName% .exe . %BuildPath% 

pause