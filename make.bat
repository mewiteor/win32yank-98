@echo off
setlocal
call :loadvcvars x86 || exit /b 1

del *.exe *.dll *.pdb

echo MZ............................................................> t

set CFLAGS=/nologo /Wall /GS- /Gy- /GR- /Oi- /O1 /c /utf-8
set LDFLAGS=/nologo /nodefaultlib /safeseh:no /incremental:no /stub:t /merge:.rdata=.text /EMITPOGOPHASEINFO /subsystem:console
set SYSLIBS=user32.lib kernel32.lib gdi32.lib shell32.lib msvcrt.lib

cl /nologo fixsysver.c || exit /b 1

cl %CFLAGS% win32yank.c || exit /b 1
lib /nologo /machine:x86 /def:msvcrt.def /out:msvcrt.lib || exit /b 1
link %LDFLAGS% win32yank.obj %SYSLIBS% || exit /b 1
:: fix win32yank system version to 4.0
fixsysver.exe || exit /b 1

del *.exp *.lib *.obj t fixsysver.exe
exit /b

:loadvcvars
for /F "delims=" %%i in ('"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property InstallationPath') do (
    call "%%i\VC\Auxiliary\Build\vcvarsall.bat" %1 && exit /b
)
exit /b 1
