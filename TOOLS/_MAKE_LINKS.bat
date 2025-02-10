rem This file must be copied into root folder of every project (its name is looked up by all compile scripts)
rem The project directory name must be unique

setlocal EnableDelayedExpansion 
 
set WORKDIR=%~dp0
set WORKDIR=%WORKDIR:~0,-1%

mklink /D ".\FRAMEWORK" "%FWKSRCDIR%"

for %%i in (%WORKDIR%) do set prjname="%%~ni"
echo Found the project name - %prjname%

rem Make a link for a compiler specific includes
if defined CMPLRGDIR__DISABLED (

 for /r "%CMPLRGDIR%\" %%f in (*.h) do (
    if "%%~nxf"=="intrin.h" set fwp=%%~dpf
 )

 set INCPATH=!fwp:~0,-1!
 echo Found include directory - !INCPATH!

 mkdir ".\COMPILER"
 mklink /D ".\COMPILER\bin" "%CMPLRGDIR%bin"
 mklink /D ".\COMPILER\include" "!INCPATH!"
)

rem Redirect any known IDE temporary directories out of the root
if defined PRJBUILDDIR (
 mkdir "%PRJBUILDDIR%"
 for /f "delims=" %%F in ('dir "%~dp0*.sln" /b /o-n') do set sln_name=%%F
 if defined sln_name (
  set vs_dir=%PRJBUILDDIR%\.vs
  mkdir !vs_dir!
  mklink /D ".\.vs" "!vs_dir!"
 )
 set build_dir=%PRJBUILDDIR%\%prjname%
 mkdir !build_dir!
 mklink /D ".\BUILD" "!build_dir!"
)

rem Register the project in the source code auto backup folder
if defined BACKUPSRCDIR (
 mkdir "%BACKUPSRCDIR%"
 mklink /D "%BACKUPSRCDIR%\%prjname%" "%~dp0"
)

pause