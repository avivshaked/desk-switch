@echo off
setlocal

set "SCRIPT_DIR=%~dp0"

call "%SCRIPT_DIR%mx-switch\switch-to-mac.cmd"
if errorlevel 1 exit /b %ERRORLEVEL%

call "%SCRIPT_DIR%screen-switch\switch-monitor-hdmi1.cmd"
if errorlevel 1 exit /b %ERRORLEVEL%

echo Mouse and monitor switched toward Mac. Switch the VP-SW200 KVM manually.
exit /b 0

