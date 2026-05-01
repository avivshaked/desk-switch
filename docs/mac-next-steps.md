# Mac Next Steps

Goal: continue the automation from the Mac side after Windows switches the mouse and monitor toward the Mac.

## Known Windows-To-Mac Flow

From Windows:

```text
windows\switch-to-mac.cmd
```

This top-level script calls:

```text
windows\mx-switch\switch-to-mac.cmd
windows\screen-switch\switch-monitor-hdmi1.cmd
```

Mouse command details:

```text
VID/PID: 046D:C548
usagePage/usage: FF00/0001
receiver device index: 0x03
feature/function: 0x0A/0x1B
slot 2 byte: 0x01
payload: 0x10,0x03,0x0A,0x1B,0x01,0x00,0x00
```

The Windows-side command to switch back to slot 1 fails once the mouse is already on slot 2:

```text
response: 0x10,0x03,0x8F,0x0A,0x1B,0x04,0x00
0x04 = connect fail
```

Therefore the Mac must issue the return-to-Windows command while the mouse is connected to the Mac, or the user must press the physical Easy-Switch button.

## Mac Tasks

1. Identify how the MX Master 3S appears on the Mac while on slot 2:
   - Bluetooth HID interface, likely vendor-defined.
   - Or Logi Bolt receiver if the Mac uses its own receiver.

2. Enumerate HID devices on macOS:
   - Prefer a small local source-built tool or native macOS HID APIs.
   - Avoid downloaded random binaries.

3. Find the Logitech HID++ interface:
   - Community Bluetooth examples often mention `usagePage 0xFF43`, `usage 0x0202`.
   - Verify with actual enumeration on the Mac.

4. Prepare a return-to-Windows payload:
   - Slot 1 channel byte is `0x00`.
   - Exact report ID/length may differ on Bluetooth vs Bolt.

5. Build a Mac-side script:
   - `switch-to-windows`
   - Later bind it to a keyboard action or launcher on the Mac.

## Mac-Side Tooling Added

Mac-side source now lives under:

```text
mac/
```

Build native helpers with:

```text
mac/build-mac.sh
```

Mouse tools:

```text
mac/mx-switch/mx-switch-list
mac/mx-switch/mx-switch-plan dry-run --slot 1
mac/mx-switch/mx-switch-write dry-run --slot 1
mac/mx-switch/mx-switch-write switch --slot 1 --execute
```

Read-only local enumeration found the MX Master 3S Bluetooth interface as:

```text
VID/PID: 0x046D:0xB034
usagePage/usage: 0x0001/0x0002
transport: Bluetooth Low Energy
input/output report lengths: 20/20
```

The Mac defaults target that discovered interface:

```text
device index byte: 0xFF
feature/function: 0x0A/0x1B
slot 1 byte: 0x00
report ID: 0x11
IOHID send mode: with-report-id
```

These defaults are intentionally reviewable and configurable. Verify the actual local interface with `mac/mx-switch/mx-switch-list` before running the write path. If more than one matching interface is found, pass the exact `--registry-path` printed by the list tool.

Top-level Mac -> Windows command:

```text
mac/switch-to-windows.sh
```

This calls the guarded mouse write path, switches the Samsung monitor to `HDMI2`, sends `BACK` after two seconds to dismiss the source overlay, and then prompts for the manual VP-SW200 KVM switch.

First approved mouse write test before permission was granted:

```text
mac/mx-switch/mx-switch-write switch --slot 1 --execute
```

Result:

```text
IOHIDDeviceOpen failed: 0xE00002E2
```

The tool selected the MX Master 3S Bluetooth LE interface correctly and printed the slot 1 long-report payload, but macOS denied HID access before any report was written. Grant Input Monitoring permission to the app running the command, then rerun the same command. If running from Cursor, grant Cursor. If running from Terminal, grant Terminal.

After granting Input Monitoring permission to Cursor, this command successfully switched the mouse away from the Mac:

```text
mac/mx-switch/mx-switch-write switch --slot 1 --execute --send with-report-id
```

Observed result:

```text
Writing IOHID output report to selected device.
Wrote IOHID output report.
```

After a short delay, the Mac HID inventory returned:

```text
Matching interfaces: 0
```

The `with-report-id` send mode is now the Mac default, so the shorter verified command is:

```text
mac/mx-switch/mx-switch-write switch --slot 1 --execute
```

## Screen Return

The Samsung monitor can be controlled by SmartThings from either host if the token is available.

On this setup:

- `HDMI1` = Mac-facing input from Windows.
- `HDMI2` = Windows-facing input at the time status was captured.

The Mac side may need its own SmartThings token storage mechanism, such as macOS Keychain.

Implemented Mac SmartThings scripts store the token in macOS Keychain under:

```text
desk-switch-smartthings-token
```

Store it with:

```text
mac/screen-switch/smartthings-credential.sh set
```

Screen return to Windows:

```text
mac/screen-switch/switch-monitor-hdmi2.sh
```

## KVM

The VP-SW200 KVM remains manual for now.

The likely future automation path is hardware:

- use the external desktop controller/control port,
- verify it is a simple momentary switch,
- automate it with an isolated optocoupler/relay driven by a microcontroller.
