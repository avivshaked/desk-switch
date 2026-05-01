@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%smartthings-set-input.ps1" -InputSource "HDMI1" -Capability "mediaInputSource"
if errorlevel 1 exit /b %ERRORLEVEL%
timeout /t 2 /nobreak >nul
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%smartthings-remote-key.ps1" -Key BACK
exit /b %ERRORLEVEL%
