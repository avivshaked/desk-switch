param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("UP", "DOWN", "LEFT", "RIGHT", "OK", "BACK", "EXIT", "MENU", "HOME", "MUTE", "PLAY", "PAUSE", "STOP", "REWIND", "FF", "PLAY_BACK", "SOURCE")]
    [string]$Key,

    [ValidateSet("PRESSED", "RELEASED", "PRESS_AND_RELEASED")]
    [string]$KeyState = "PRESS_AND_RELEASED",

    [string]$DeviceId = "a8cc51eb-4cd4-c535-38d1-4e32e76edb96"
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$Token = & (Join-Path $ScriptDir "smartthings-credential.ps1") get

if ([string]::IsNullOrWhiteSpace($Token)) {
    throw "SmartThings token is not stored. Run screen-switch\set-smartthings-token.cmd first."
}

$Body = @{
    commands = @(
        @{
            component = "main"
            capability = "samsungvd.remoteControl"
            command = "send"
            arguments = @($Key, $KeyState)
        }
    )
} | ConvertTo-Json -Depth 8 -Compress

$BodyFile = [System.IO.Path]::GetTempFileName()

try {
    [System.IO.File]::WriteAllText($BodyFile, $Body, [System.Text.UTF8Encoding]::new($false))
    Write-Host "SmartThings token retrieved from Windows Credential Manager."
    Write-Host "Sending remote key command: key=$Key state=$KeyState"
    Write-Host "Request body: $Body"
    curl.exe -s `
        -S `
        --ssl-no-revoke `
        -w "`nHTTP_STATUS:%{http_code}`n" `
        -X POST `
        -H "Authorization: Bearer $Token" `
        -H "Content-Type: application/json" `
        --data-binary "@$BodyFile" `
        "https://api.smartthings.com/v1/devices/$DeviceId/commands"
}
finally {
    if (Test-Path -LiteralPath $BodyFile) {
        Remove-Item -LiteralPath $BodyFile -Force
    }
    Remove-Variable Token -ErrorAction SilentlyContinue
}

