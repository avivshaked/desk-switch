# screen-switch

Windows DDC/CI experiments for monitor input switching.

Safety model:

- Start with `screen-switch-list.exe`, which only enumerates monitors and reads capabilities/current input source.
- Do not write VCP code `0x60` until the monitor and supported input values are known.

Build:

```powershell
cmd.exe /c '"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cl /nologo /W4 /O2 /Fe:screen-switch\screen-switch-list.exe screen-switch\screen-switch-list.c dxva2.lib user32.lib gdi32.lib'
```

Run:

```powershell
screen-switch-list.exe
```

## SmartThings Token

For Samsung smart-monitor input switching, store a SmartThings token in Windows Credential Manager:

```text
windows\screen-switch\set-smartthings-token.cmd
```

List SmartThings devices:

```text
windows\screen-switch\list-smartthings-devices.cmd
```

Get the Odyssey OLED G8 status:

```text
windows\screen-switch\get-monitor-status.cmd
```

Try setting a monitor input source:

```text
windows\screen-switch\set-monitor-input.cmd "Display Port"
windows\screen-switch\set-monitor-input.cmd HDMI1
```

Fixed launchers based on discovered G80SD SmartThings inputs:

```text
windows\screen-switch\switch-monitor-hdmi1.cmd
windows\screen-switch\switch-monitor-hdmi2.cmd
```

Discovered map:

- `HDMI1` -> `PC` in SmartThings, used here to get to the Mac from Windows
- `HDMI2` -> `pc`

Dismiss the Samsung source/menu overlay:

```text
windows\screen-switch\dismiss-monitor-menu.cmd
windows\screen-switch\dismiss-monitor-menu-back.cmd
```

This sends `samsungvd.remoteControl:send("EXIT", "PRESS_AND_RELEASED")`.
The `-back` variant sends `BACK`.

Do not auto-run `EXIT` after switching on the G80SD; it can send the monitor to Home. The fixed HDMI launchers auto-send `BACK` two seconds after switching because it was verified to dismiss the bottom overlay manually.

Verified workflow from Windows to Mac:

```text
windows\screen-switch\switch-monitor-hdmi1.cmd
```

The token is stored under the Credential Manager target:

```text
switch-handoff-smartthings-token
```

Delete it with:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "windows\screen-switch\smartthings-credential.ps1" delete
```
