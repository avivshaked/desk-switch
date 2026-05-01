# Screen Switch Notes

Goal: explore switching monitor input from Windows.

Preferred path: DDC/CI over the display cable. The common VCP code for input source is `0x60`, but supported values vary by monitor.

Safety model:

- Start with read-only enumeration.
- Read current VCP `0x60`.
- Read the monitor capabilities string.
- Do not call `SetVCPFeature` until the target monitor and input values are known.

Tool added:

- `screen-switch/screen-switch-list.c`

It uses Windows Monitor Configuration APIs:

- `EnumDisplayMonitors`
- `GetPhysicalMonitorsFromHMONITOR`
- `GetVCPFeatureAndVCPFeatureReply`
- `GetCapabilitiesStringLength`
- `CapabilitiesRequestAndCapabilitiesReply`

First run:

```powershell
.\screen-switch\screen-switch-list.exe
```

Output:

```text
Display monitor 1: \\.\DISPLAY1
  bounds: left=0 top=0 right=2560 bottom=1440
  physical monitor 1: Generic PnP Monitor
    input source VCP 0x60: unavailable (error 3223725442)
    capabilities: unavailable (error 3223725442)

Display monitors enumerated: 1
```

Interpretation: Windows can enumerate the display and physical monitor handle, but DDC/CI VCP/capabilities queries are failing on this display path. Need to check monitor DDC/CI setting, cable/path, GPU/driver support, or try a vendor-specific path.

User added Avast exception for:

```text
windows\screen-switch\screen-switch-list.exe
```

Rerun after whitelisting produced the same DDC/CI failures, so Avast is not the cause.

Added Windows Credential Manager scripts for SmartThings:

- `screen-switch/smartthings-credential.ps1`
- `screen-switch/smartthings-devices.ps1`
- `screen-switch/set-smartthings-token.cmd`
- `screen-switch/list-smartthings-devices.cmd`

Credential target:

```text
switch-handoff-smartthings-token
```

The credential helper uses the native Windows Credential API via PowerShell `Add-Type` and `advapi32.dll`; it does not require external modules.

When the user ran `list-smartthings-devices.cmd`, token retrieval succeeded but Windows curl failed with:

```text
curl: (35) schannel: next InitializeSecurityContext failed: CRYPT_E_NO_REVOCATION_CHECK (0x80092012)
HTTP_STATUS:000
```

Updated `smartthings-devices.ps1` to pass `--ssl-no-revoke` to Windows curl. This still uses TLS but skips Schannel revocation checking when Windows cannot check certificate revocation.

The user reran the device list successfully. Monitor device:

```text
deviceId: a8cc51eb-4cd4-c535-38d1-4e32e76edb96
name/label: 32" Odyssey OLED G8
modelCode: LS32DG800SUXEN
presentationId: VD-SMONITOR-2024
deviceTypeName: x.com.st.d.monitor
```

Capabilities include:

- `mediaInputSource`
- `samsungvd.mediaInputSource`
- `execute`
- `refresh`
- `samsungvd.remoteControl`

Added scripts:

- `screen-switch/smartthings-status.ps1`
- `screen-switch/get-monitor-status.cmd`
- `screen-switch/smartthings-set-input.ps1`
- `screen-switch/set-monitor-input.cmd`

Monitor status showed:

```text
samsungvd.mediaInputSource.inputSource = HDMI2
samsungvd.mediaInputSource.supportedInputSourcesMap = [{"id":"HDMI1","name":"PC"},{"id":"HDMI2","name":"pc"}]
```

For this setup, from the Windows machine:

- `HDMI1` is the monitor input used to get to the Mac.
- `HDMI2` is the current/Windows-side input at the time status was captured.

Added fixed launchers:

- `screen-switch/switch-monitor-hdmi1.cmd`
- `screen-switch/switch-monitor-hdmi2.cmd`

First attempt using `samsungvd.mediaInputSource` failed:

```text
HTTP_STATUS:422
ConstraintViolationError
BodyMalformedError target=commands.arguments
```

Changed fixed launchers to call standard `mediaInputSource` first.

Second attempt using standard `mediaInputSource` also failed with the same `commands.arguments` `BodyMalformedError`.

Online references:

- Official/community SmartThings examples use the REST body shape `{"commands":[{"component":"main","capability":"mediaInputSource","command":"setInputSource","arguments":["HDMI1"]}]}`.
- `samsungvd.mediaInputSource` is a Samsung custom namespace; community posts say it has `supportedInputSourcesMap` and `setInputSource`, but public documentation is sparse.

Updated `smartthings-set-input.ps1` to write the request JSON to a temporary UTF-8 file and send it with `curl --data-binary @file`, while printing the body for inspection.

User confirmed input switching worked after the temp-file body fix.

For the source overlay/menu that appears after switching, online SmartThings community references show `samsungvd.remoteControl` has command `send` with arguments:

- key value enum including `EXIT`, `BACK`, `HOME`, `SOURCE`
- key state enum including `PRESS_AND_RELEASED`

Added:

- `screen-switch/smartthings-remote-key.ps1`
- `screen-switch/dismiss-monitor-menu.cmd`

The dismiss command sends:

```json
{"commands":[{"component":"main","capability":"samsungvd.remoteControl","command":"send","arguments":["EXIT","PRESS_AND_RELEASED"]}]}
```

User later reported auto-sending `EXIT` after input switch sent the monitor to the Home app. Reverted automatic `EXIT` from `switch-monitor-hdmi1.cmd` and `switch-monitor-hdmi2.cmd`.

Added `dismiss-monitor-menu-back.cmd` to test `BACK` manually. Do not wire dismiss commands into switch scripts until behavior is verified across inputs.

User confirmed `BACK` dismisses the overlay manually. Updated `switch-monitor-hdmi1.cmd` and `switch-monitor-hdmi2.cmd` to wait two seconds after switching and then send `BACK`.

User confirmed the integrated switch + delayed `BACK` flow worked.
