# Project State

## Verified

- Logitech MX Master 3S can be switched from Windows slot 1 to Mac slot 2 through the Windows Logi Bolt receiver.
- Samsung Odyssey OLED G8/G80SD can be switched through SmartThings.
- Samsung bottom source overlay can be dismissed with SmartThings remote `BACK`.

## Verified Commands

Mouse to Mac:

```text
windows\mx-switch\switch-to-mac.cmd
```

Screen to Mac:

```text
windows\screen-switch\switch-monitor-hdmi1.cmd
```

Combined Windows -> Mac workflow:

```text
windows\switch-to-mac.cmd
```

This switches mouse and screen, then prompts for manual VP-SW200 KVM button press.

Screen input scripts:

```text
windows\screen-switch\switch-monitor-hdmi1.cmd
windows\screen-switch\switch-monitor-hdmi2.cmd
```

## Important IDs

Mouse/Bolt:

```text
Logi Bolt VID/PID: 046D:C548
HID++ short interface: usagePage=0xFF00 usage=0x0001
Short report lengths: input=7 output=7
MX Master 3S receiver device index: 0x03
Successful slot 2 payload: 0x10,0x03,0x0A,0x1B,0x01,0x00,0x00
Slot 1 payload: 0x10,0x03,0x0A,0x1B,0x00,0x00,0x00
```

Samsung SmartThings:

```text
Device ID: a8cc51eb-4cd4-c535-38d1-4e32e76edb96
Device name: 32" Odyssey OLED G8
Model code: LS32DG800SUXEN
Capability: mediaInputSource / samsungvd.mediaInputSource
Working command capability: mediaInputSource
Input map: HDMI1 -> PC, HDMI2 -> pc
```

Credential target in this handoff project:

```text
desk-switch-smartthings-token
```

The original exploration used `hidapitester-smartthings-token`; new repo users should run:

```text
windows\screen-switch\set-smartthings-token.cmd
```

## Not Working / Deferred

- DDC/CI monitor control failed with error `3223725442`.
- Windows-side mouse return from slot 2 to slot 1 fails with HID++ `CONNECT_FAIL` because the mouse is no longer connected to the Windows Bolt receiver.
- VP-SW200 KVM software switching not found.
