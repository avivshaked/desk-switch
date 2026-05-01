param(
    [string]$DeviceId = "a8cc51eb-4cd4-c535-38d1-4e32e76edb96"
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$Token = & (Join-Path $ScriptDir "smartthings-credential.ps1") get

if ([string]::IsNullOrWhiteSpace($Token)) {
    throw "SmartThings token is not stored. Run screen-switch\set-smartthings-token.cmd first."
}

try {
    Write-Host "SmartThings token retrieved from Windows Credential Manager."
    curl.exe -s `
        -S `
        --ssl-no-revoke `
        -w "`nHTTP_STATUS:%{http_code}`n" `
        -H "Authorization: Bearer $Token" `
        "https://api.smartthings.com/v1/devices/$DeviceId/status"
}
finally {
    Remove-Variable Token -ErrorAction SilentlyContinue
}

