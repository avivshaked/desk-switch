# Agent Notes

## Project Shape

This repo is a small Windows-side handoff project for switching from a Windows workstation toward a Mac setup:

- `windows\switch-to-mac.cmd` is the verified top-level workflow.
- `windows\mx-switch\` contains Logitech MX Master 3S Easy-Switch tooling for a Logi Bolt receiver.
- `windows\screen-switch\` contains Samsung Odyssey OLED G8/G80SD monitor switching through SmartThings, plus a read-only DDC/CI inventory tool.
- `mac/` contains the Mac-side reverse handoff tools for Bluetooth HID++ and SmartThings via Keychain.
- `docs\` contains the investigation history, verified IDs, safety notes, and Mac-side follow-up work.

Keep the repo lightweight. It intentionally avoids services, driver changes, registry edits, firmware changes, broad antivirus exclusions, and system-installed HIDAPI dependencies.

## Safety Rules

- Read the relevant docs before changing behavior: start with `README.md`, `docs/project-state.md`, and the specific topic doc under `docs\`.
- Do not run HID write commands unless the exact command has been shown and explicitly approved in the current conversation.
- Treat any `mx-switch-write.exe switch ... --execute` command as a state-changing HID write, even when it targets a known device.
- Treat any `mac/mx-switch/mx-switch-write switch ... --execute` command the same way.
- Prefer read-only commands first: `mx-switch-plan.exe dry-run`, `mx-switch-list.exe`, and `screen-switch-list.exe`.
- Do not edit registry values, change drivers, install services/startup entries, flash firmware, or require admin without explicit approval.
- Do not store SmartThings tokens or other secrets in the repo. The Windows credential target is `desk-switch-smartthings-token`.
- Whitelist exact built executable paths only if antivirus handling is needed. Do not recommend whitelisting the whole repo folder.

## Build And Runtime

Build on Windows with Visual Studio Build Tools:

```text
windows\build-windows.cmd
```

This compiles the C utilities directly with MSVC and Windows SDK libraries:

- `windows\mx-switch\mx-switch-plan.exe`
- `windows\mx-switch\mx-switch-list.exe`
- `windows\mx-switch\mx-switch-write.exe`
- `windows\screen-switch\screen-switch-list.exe`

This repo is usually edited from macOS but the utilities are Windows-specific. Avoid assuming they can be built or run successfully on macOS.

Build the Mac-side helpers on macOS with:

```text
mac/build-mac.sh
```

This compiles:

- `mac/mx-switch/mx-switch-plan`
- `mac/mx-switch/mx-switch-list`
- `mac/mx-switch/mx-switch-write`

## Code Conventions

- Keep the native tools in C unless the code grows enough to justify C++ resource ownership helpers.
- Preserve the split between planner, read-only inventory, and write-capable tools. This split is part of the safety model.
- Keep `.cmd` launchers simple and path-relative using `%~dp0` so they work from keyboard macro tools and arbitrary working directories.
- Keep PowerShell scripts dependency-free where practical. Existing scripts use native Windows APIs, `curl.exe`, and Windows Credential Manager.
- Keep Mac shell scripts dependency-light. Existing scripts use `/bin/sh`, `security`, `curl`, and native compiled IOKit helpers.
- For SmartThings command bodies, keep using temporary UTF-8 JSON files with `curl.exe --data-binary @file`; this fixed malformed request handling.
- Keep command output explicit enough for hardware debugging: print targets, payloads, request bodies when useful, HTTP status codes, and Windows errors.

## Verified Local Details

Logitech/Bolt:

- Bolt VID/PID: `046D:C548`
- HID++ short interface: usage page `0xFF00`, usage `0x0001`
- Verified MX Master 3S receiver device index: `0x03`
- Verified slot 2 payload: `0x10,0x03,0x0A,0x1B,0x01,0x00,0x00`
- Slot 1 payload shape: `0x10,0x03,0x0A,0x1B,0x00,0x00,0x00`
- Windows cannot switch the mouse back to slot 1 after the mouse has moved to slot 2; that return must happen from the active Mac side or manually.

Logitech/Mac Bluetooth:

- Local MX Master 3S Bluetooth interface: VID/PID `046D:B034`, usage page `0x0001`, usage `0x0002`, report lengths `20/20`.
- Mac defaults use report ID `0x11`, device index byte `0xFF`, and IOHID send mode `with-report-id`.
- Verified Mac -> Windows mouse command: `mac/mx-switch/mx-switch-write switch --slot 1 --execute`.
- Verify locally with `mac/mx-switch/mx-switch-list` before any write.

Samsung/SmartThings:

- Device ID: `a8cc51eb-4cd4-c535-38d1-4e32e76edb96`
- Device name: `32" Odyssey OLED G8`
- Working input capability: `mediaInputSource`
- In this setup, `HDMI1` is the Mac-facing input and `HDMI2` is the Windows-facing input.
- The fixed HDMI launchers wait two seconds after switching, then send remote key `BACK` to dismiss the bottom overlay. Do not replace this with `EXIT`; `EXIT` was observed to send the monitor to Home.

KVM:

- The VP-SW200 KVM remains manual.
- Treat keyboard hotkey switching and USB HID software control as unsupported unless new model-specific evidence appears.
- The likely automation path is hardware control of the external controller/control port, not software switching through this repo.

## Documentation Hygiene

When changing behavior, update the closest README or doc:

- Verified current state belongs in `docs/project-state.md`.
- Mouse-specific findings belong in `docs/logitech-easy-switch.md` or `windows\mx-switch\README.md`.
- Screen-specific findings belong in `docs/screen-switch.md` or `windows\screen-switch\README.md`.
- Mac-side return work belongs in `docs/mac-next-steps.md`.
