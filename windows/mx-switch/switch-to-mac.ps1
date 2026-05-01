$ErrorActionPreference = "Stop"

# Switch Logitech MX Master 3S from Windows slot 1 to Mac slot 2 via the
# verified Logi Bolt receiver HID++ short report.
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$Switcher = Join-Path $ScriptDir "mx-switch-write.exe"

if (-not (Test-Path -LiteralPath $Switcher)) {
    throw "Cannot find mx-switch-write.exe at: $Switcher"
}

& $Switcher switch --slot 2 --device-index 0x03 --read-response --timeout-ms 700 --execute
exit $LASTEXITCODE

