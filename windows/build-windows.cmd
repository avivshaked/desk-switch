@echo off
setlocal

set "ROOT=%~dp0.."
set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

if not exist "%VCVARS%" (
    echo Could not find Visual Studio Build Tools vcvars64.bat:
    echo %VCVARS%
    exit /b 1
)

call "%VCVARS%"
if errorlevel 1 exit /b %ERRORLEVEL%

pushd "%ROOT%"

cl /nologo /W4 /O2 /Fe:windows\mx-switch\mx-switch-plan.exe windows\mx-switch\mx-switch-plan.c
if errorlevel 1 goto fail

cl /nologo /W4 /O2 /Fe:windows\mx-switch\mx-switch-list.exe windows\mx-switch\mx-switch-list.c hid.lib setupapi.lib
if errorlevel 1 goto fail

cl /nologo /W4 /O2 /Fe:windows\mx-switch\mx-switch-write.exe windows\mx-switch\mx-switch.c hid.lib setupapi.lib
if errorlevel 1 goto fail

cl /nologo /W4 /O2 /Fe:windows\screen-switch\screen-switch-list.exe windows\screen-switch\screen-switch-list.c dxva2.lib user32.lib gdi32.lib
if errorlevel 1 goto fail

popd
echo Build complete.
exit /b 0

:fail
set "ERR=%ERRORLEVEL%"
popd
exit /b %ERR%

