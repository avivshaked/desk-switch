@echo off
setlocal

if "%~1"=="" (
    echo Usage: %~nx0 INPUT_SOURCE
    echo Example: %~nx0 "Display Port"
    echo Example: %~nx0 HDMI1
    exit /b 2
)

set "SCRIPT_DIR=%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%smartthings-set-input.ps1" -InputSource "%~1"
exit /b %ERRORLEVEL%

