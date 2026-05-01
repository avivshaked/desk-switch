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

## Screen Return

The Samsung monitor can be controlled by SmartThings from either host if the token is available.

On this setup:

- `HDMI1` = Mac-facing input from Windows.
- `HDMI2` = Windows-facing input at the time status was captured.

The Mac side may need its own SmartThings token storage mechanism, such as macOS Keychain.

## KVM

The VP-SW200 KVM remains manual for now.

The likely future automation path is hardware:

- use the external desktop controller/control port,
- verify it is a simple momentary switch,
- automate it with an isolated optocoupler/relay driven by a microcontroller.
