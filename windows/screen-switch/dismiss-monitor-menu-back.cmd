@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%smartthings-remote-key.ps1" -Key BACK
exit /b %ERRORLEVEL%

