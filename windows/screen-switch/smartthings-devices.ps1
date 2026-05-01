$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$Token = & (Join-Path $ScriptDir "smartthings-credential.ps1") get

if ([string]::IsNullOrWhiteSpace($Token)) {
    throw "SmartThings token is not stored. Run screen-switch\smartthings-credential.ps1 set first."
}

try {
    Write-Host "SmartThings token retrieved from Windows Credential Manager."
    curl.exe -s `
        -S `
        --ssl-no-revoke `
        -w "`nHTTP_STATUS:%{http_code}`n" `
        -H "Authorization: Bearer $Token" `
        "https://api.smartthings.com/v1/devices"
}
finally {
    Remove-Variable Token -ErrorAction SilentlyContinue
}
