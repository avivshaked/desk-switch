# Switch Handoff

This project contains the verified Windows-side pieces for switching from the Windows workstation toward the Mac setup:

- Logitech MX Master 3S Easy-Switch via Logi Bolt.
- Samsung Odyssey OLED G8/G80SD monitor input switching via SmartThings.
- Keychron Q1 Max Bluetooth pairing notes for Windows/Mac.
- KVM investigation notes for the VP-SW200 HDMI KVM.

It intentionally avoids installing services, changing drivers, editing the registry, or depending on system-installed HIDAPI.

## Current State

Working from Windows:

```text
windows\switch-to-mac.cmd
```

Runs the verified Windows -> Mac workflow:

- switches the MX Master 3S to Easy-Switch slot 2 / Mac,
- switches the Samsung Odyssey OLED G8/G80SD to the Mac input,
- leaves the VP-SW200 KVM as a manual button press.

Individual commands remain available:

```text
windows\mx-switch\switch-to-mac.cmd
windows\screen-switch\switch-monitor-hdmi1.cmd
```

The KVM remains manual for now. Keyboard hotkeys were extensively tested and did not work; no obvious USB HID control interface was found.

The Keychron Q1 Max can be moved off direct USB/KVM routing by pairing Bluetooth slots separately:

```text
docs\keychron-bluetooth.md
```

## Build On Windows

Install/use Visual Studio Build Tools with MSVC. Then run:

```text
windows\build-windows.cmd
```

This builds:

- `windows\mx-switch\mx-switch-plan.exe`
- `windows\mx-switch\mx-switch-list.exe`
- `windows\mx-switch\mx-switch-write.exe`
- `windows\screen-switch\screen-switch-list.exe`

## Avast / Antivirus

Whitelist exact built EXE paths only, not the repo folder:

```text
...\windows\mx-switch\mx-switch-plan.exe
...\windows\mx-switch\mx-switch-list.exe
...\windows\mx-switch\mx-switch-write.exe
...\windows\screen-switch\screen-switch-list.exe
```

Do not whitelist broad folders such as the project root.

## SmartThings Token

The Samsung monitor scripts use a SmartThings Personal Access Token stored in Windows Credential Manager under:

```text
desk-switch-smartthings-token
```

Store it on Windows with:

```text
windows\screen-switch\set-smartthings-token.cmd
```

The token is not stored in this repo.

## Mac Handoff

The Mac side has source tooling for the reverse handoff. See:

```text
docs\mac-next-steps.md
mac\README.md
```

The key Mac-side job is switching the MX Master 3S back to slot 1 / Windows from the Bluetooth interface the Mac sees while the mouse is connected to slot 2. Start with read-only enumeration and dry-run commands before using the guarded write path.
