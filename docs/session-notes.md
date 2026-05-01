# Session Notes

This is a historical log from the original exploration workspace. Some paths and credential names mention `D:\Projects\code\hidapitester` or `hidapitester-smartthings-token`; the cleaned desk-switch project uses relative paths and `desk-switch-smartthings-token`.

Date: 2026-05-01

## Goal

Safely experiment with controlling a Logitech MX Master 3S Easy-Switch slot from scripts on Windows, with a future goal of triggering it from a Keychron Q1 Max keyboard.

## User Environment

- Repo: `D:\Projects\code\hidapitester`
- OS shell: Windows PowerShell
- Mouse: Logitech MX Master 3S
- Windows is Easy-Switch slot 1.
- Mac is Easy-Switch slot 2.
- Slot 3 is another screen/device.
- Known Bolt receiver hardware ID: `USB\VID_046D&PID_C548&MI_00\...`
- Known Bluetooth device ID: `BTHLE\DEV_DD8631CAC5B3\...`

## Safety Rules

- Do not run HID write commands until the exact command has been shown and explicitly approved.
- Do not make destructive changes.
- Do not edit registry values.
- Do not change drivers.
- Do not flash firmware.
- Do not install services or startup entries.
- Do not require admin unless absolutely necessary and explicitly approved.
- Prefer local builds from source over downloaded release binaries.
- Do not download or run random binaries.
- Ask before installs or dependency downloads.

The user later clarified:

- Local locating/listing/reading commands are pre-approved if they do not execute binaries and do not modify state.
- Commands that execute binaries still need qualification and approval.

## Commands Already Run

Read-only repo inspection:

- `Get-Location`
- `Get-ChildItem -Force`
- `git status --short`
- `rg ... README.md CMakeLists.txt Makefile hidapitester.c docs`
- `Get-Content README.md`
- `Get-Content CMakeLists.txt`
- `Get-Content Makefile`
- selected `Get-Content hidapitester.c` line ranges
- `Get-Content .github\workflows\windows.yml`

Tool location checks:

- `cmake --version` failed: not on `PATH`.
- `gcc --version` failed: not on `PATH`.
- `ninja --version` failed: not on `PATH`.
- `cl` failed: not on `PATH`.
- `make --version` failed: not on `PATH`.
- Visual Studio 2022 Community directory exists.
- Visual Studio Build Tools found via `vswhere.exe` at `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools`.
- `cmake.exe`, `ninja.exe`, and `cl.exe` were located under Visual Studio Build Tools.
- `vcvars64.bat` exists under Visual Studio Build Tools.

Device inventory attempts:

- `Get-PnpDevice -PresentOnly ...` failed with access denied.
- `Get-CimInstance Win32_PnPEntity ...` failed with access denied.
- An elevated read-only CIM query was requested; the user rejected it.
- Passive registry reads under `HKLM:\SYSTEM\CurrentControlSet\Enum` found Logitech Bolt and Bluetooth device keys. No registry values were changed.

## Build State

Build was run with existing Visual Studio Build Tools, CMake, and Ninja.

Configure command used MSVC via `vcvars64.bat` and generated `build-msvc`.

CMake fetched/configured source dependencies:

- `libusb/hidapi`
- `ludvikjerabek/getopt-win` for MSVC

Build output:

- `D:\Projects\code\hidapitester\build-msvc\hidapitester.exe`

Warnings observed:

- Static `getopt-win` build prints: `Warning static builds of getopt violate the Lesser GNU Public License`.
- `hidapitester.c(275)` has an MSVC `swprintf` format warning: `%s` expects `unsigned short *`, but receives `char *`; MSVC suggests `%hs` or `%Ts`.
- HIDAPI link warning: `hid_winapi_descriptor_reconstruct_pp_data` duplicate definition ignored.

No HID commands were run during configure/build.

## Standalone mx-switch

A new HIDAPI-free standalone experiment was added under `mx-switch/`.

Files:

- `mx-switch/mx-switch.c`
- `mx-switch/mx-switch-plan.c`
- `mx-switch/mx-switch-list.c`
- `mx-switch/CMakeLists.txt`
- `mx-switch/README.md`

The program is Windows-only C and links directly against Windows SDK libraries:

- `hid.lib`
- `setupapi.lib`

It supports:

- `list`
- `dry-run --slot <1|2|3>`
- `switch --slot <1|2|3> --execute`

A separate `mx-switch-plan.exe` dry-run planner was added after Avast reacted badly to running the HID-capable binary. `mx-switch-plan.exe` does not include Windows HID headers, does not link `hid.lib` or `setupapi.lib`, and cannot enumerate, open, or write HID devices.

A separate `mx-switch-list.exe` inventory tool was added next. It links Windows HID/setup APIs but has no write path and no Easy-Switch payload code. It opens HID paths with desired access `0` for metadata only.

Direct MSVC compile succeeded:

```powershell
cmd.exe /c '"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cl /nologo /W4 /O2 /Fe:mx-switch\mx-switch-list.exe mx-switch\mx-switch-list.c hid.lib setupapi.lib'
```

Read-only list run succeeded:

```powershell
.\mx-switch\mx-switch-list.exe
```

Observed exact default target match:

```text
046D:C548 usagePage=0xFF00 usage=0x0001
  manufacturer: Logitech
  product:      USB Receiver
  path:         \\?\hid#vid_046d&pid_c548&mi_02&col01#8&23c1964a&0&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}

Matching interfaces: 1
```

Direct MSVC compile succeeded:

```powershell
cmd.exe /c '"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cl /nologo /W4 /O2 /Fe:mx-switch\mx-switch-plan.exe mx-switch\mx-switch-plan.c'
```

Dry-runs completed successfully:

```powershell
.\mx-switch\mx-switch-plan.exe dry-run --slot 2
.\mx-switch\mx-switch-plan.exe dry-run --slot 1
```

Observed payloads:

- Slot 2: `0x10,0x02,0x0A,0x1B,0x01,0x00,0x00`
- Slot 1: `0x10,0x02,0x0A,0x1B,0x00,0x00,0x00`

Language decision: keep the tools in C for now. C++ is available through MSVC and would help if the code grows enough to need RAII/resource wrappers, but the current Windows HID API usage is naturally C-shaped and the small C programs are easier to audit for security-sensitive behavior.

The actual write path requires `--execute`; do not run that without explicit user approval of the exact command.

The original `mx-switch.exe` became locked after the Avast incident and could not be overwritten by the linker. The HID-capable source was rebuilt under the explicit name `mx-switch-write.exe` instead:

```powershell
cmd.exe /c '"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cl /nologo /W4 /O2 /Fe:mx-switch\mx-switch-write.exe mx-switch\mx-switch.c hid.lib setupapi.lib'
```

Do not run `mx-switch-write.exe switch ... --execute` without explicit user approval of the exact command.

Safe tests of `mx-switch-write.exe` completed successfully:

```powershell
.\mx-switch\mx-switch-write.exe dry-run --slot 2
.\mx-switch\mx-switch-write.exe dry-run --slot 1
.\mx-switch\mx-switch-write.exe list
```

The dry-run commands printed the expected slot 2/slot 1 payloads and sent no HID report. The `list` command found the same single default Bolt receiver interface as `mx-switch-list.exe`.

First approved HID output report was sent:

```powershell
.\mx-switch\mx-switch-write.exe switch --slot 2 --execute
```

Observed output:

```text
046D:C548 usagePage=0xFF00 usage=0x0001
  manufacturer: Logitech
  product:      USB Receiver
  path:         \\?\hid#vid_046d&pid_c548&mi_02&col01#8&23c1964a&0&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}

Opening for write: \\?\hid#vid_046d&pid_c548&mi_02&col01#8&23c1964a&0&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}
Writing output report: 0x10,0x02,0x0A,0x1B,0x01,0x00,0x00
Wrote 7 bytes.
```

Need user confirmation whether the mouse actually switched to Mac slot 2.

User reported the slot 2 write did not switch the mouse. Leading suspects:

- Receiver device index `0x02` may not be the mouse for this receiver.
- Function byte `0x1B` may be wrong for this exact MX Master 3S/Bolt setup; some Bluetooth examples use `0x1E`.
- The output-report write method reached Windows but may not have produced a meaningful HID++ response; no response reading exists yet.

Added configurable `--feature-index` and `--function-index` options to `mx-switch-plan.exe` and `mx-switch-write.exe` so hypotheses can be tested one at a time.

Second approved HID output report was sent to test receiver device index `0x01`:

```powershell
.\mx-switch\mx-switch-write.exe switch --slot 2 --device-index 0x01 --execute
```

Observed output:

```text
046D:C548 usagePage=0xFF00 usage=0x0001
  manufacturer: Logitech
  product:      USB Receiver
  path:         \\?\hid#vid_046d&pid_c548&mi_02&col01#8&23c1964a&0&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}

Opening for write: \\?\hid#vid_046d&pid_c548&mi_02&col01#8&23c1964a&0&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}
Writing output report: 0x10,0x01,0x0A,0x1B,0x01,0x00,0x00
Wrote 7 bytes.
```

Need user confirmation whether this switched the mouse.

User reported it did not switch. Added `--read-response` and `--timeout-ms` to `mx-switch-write.exe` so future writes can attempt one input report read after the output report. This should help distinguish wrong device/feature bytes from no-response behavior.

Fixed a bug where the write handle was closed before the response read, causing `ERROR_INVALID_HANDLE` (`6`).

Diagnostic short-report writes with response reads:

```powershell
.\mx-switch\mx-switch-write.exe switch --slot 2 --device-index 0x02 --read-response --timeout-ms 700 --execute
```

Response:

```text
0x10,0x02,0x8F,0x0A,0x1B,0x09,0x00
```

Interpretation: HID++ error response (`0x8F`), error code `0x09` = resource error.

```powershell
.\mx-switch\mx-switch-write.exe switch --slot 2 --device-index 0x01 --read-response --timeout-ms 700 --execute
```

Response:

```text
0x10,0x01,0x8F,0x0A,0x1B,0x04,0x00
```

Interpretation: HID++ error response (`0x8F`), error code `0x04` = connect fail.

```powershell
.\mx-switch\mx-switch-write.exe switch --slot 2 --device-index 0x03 --read-response --timeout-ms 700 --execute
```

Response:

```text
0x10,0x03,0x41,0x10,0x42,0x34,0xB0
```

Interpretation: not an HID++ error response. Device index `0x03` appears to be a live/communicating Logitech device. Need user confirmation whether this switched the mouse.

User confirmed this command switched the MX Master 3S to slot 2:

```powershell
.\mx-switch\mx-switch-write.exe switch --slot 2 --device-index 0x03 --read-response --timeout-ms 700 --execute
```

Winning payload:

```text
0x10,0x03,0x0A,0x1B,0x01,0x00,0x00
```

For returning to Windows slot 1, use the same receiver/device/function bytes with slot byte `0x00`:

```powershell
.\mx-switch\mx-switch-write.exe switch --slot 1 --device-index 0x03 --read-response --timeout-ms 700 --execute
```

Tried the return-to-Windows command from Windows while the mouse was on slot 2:

```text
Writing output report: 0x10,0x03,0x0A,0x1B,0x00,0x00,0x00
Wrote 7 bytes.
Read 7 response bytes: 0x10,0x03,0x8F,0x0A,0x1B,0x04,0x00
```

Interpretation: `0x04` = connect fail. Windows/Bolt receiver cannot switch the mouse back while the mouse is currently on slot 2. Return needs to be issued from the active host for slot 2 or done manually with the mouse Easy-Switch button.

Created PowerShell wrapper:

```text
D:\Projects\code\hidapitester\mx-switch\switch-to-mac.ps1
```

It resolves `mx-switch-write.exe` relative to its own directory and runs:

```powershell
mx-switch-write.exe switch --slot 2 --device-index 0x03 --read-response --timeout-ms 700 --execute
```

Suggested launcher command:

```text
D:\Projects\code\hidapitester\mx-switch\switch-to-mac.cmd
```

The `.cmd` shim calls the PowerShell script internally.

Screen switching status:

- Samsung Odyssey OLED G8/G80SD SmartThings switching works through `screen-switch/`.
- From Windows, `screen-switch/switch-monitor-hdmi1.cmd` switches the screen to the Mac input.
- The Samsung bottom overlay is dismissed by sending `BACK` two seconds after switching.

User reported it did not switch. Avoid further Bolt writes until checking whether the active Windows connection is Bluetooth rather than Bolt. Added report-length/capability output to `mx-switch-list.exe` and `mx-switch-write.exe list` to help identify suitable HID++ interfaces.

Read-only full HID inventory showed the Bolt receiver has:

- `046D:C548 usagePage=0xFF00 usage=0x0001`, report lengths input/output `7/7`
- `046D:C548 usagePage=0xFF00 usage=0x0002`, report lengths input/output `20/20`

Added `--report short|long`. Long reports use report id `0x11` and length 20 so the `FF00/0002` interface can be tested deliberately later.

User manually added Avast exceptions for the three exact executable paths:

- `D:\Projects\code\hidapitester\mx-switch\mx-switch-plan.exe`
- `D:\Projects\code\hidapitester\mx-switch\mx-switch-list.exe`
- `D:\Projects\code\hidapitester\mx-switch\mx-switch-write.exe`

The user did not whitelist the repo or folder broadly.

User gave general approval for HID writes in this mx-switch experiment.

Third HID output report was sent to test the receiver long-report interface:

```powershell
.\mx-switch\mx-switch-write.exe switch --slot 2 --usage 0x0002 --report long --function-index 0x1E --execute
```

Observed output:

```text
046D:C548 usagePage=0xFF00 usage=0x0002
  report lengths: input=20 output=20 feature=0
  manufacturer: Logitech
  product:      USB Receiver
  path:         \\?\hid#vid_046d&pid_c548&mi_02&col02#8&23c1964a&0&0001#{4d1e55b2-f16f-11cf-88cb-001111000030}

Opening for write: \\?\hid#vid_046d&pid_c548&mi_02&col02#8&23c1964a&0&0001#{4d1e55b2-f16f-11cf-88cb-001111000030}
Writing output report: 0x11,0x02,0x0A,0x1E,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
Wrote 20 bytes.
```

Need user confirmation whether this switched the mouse.

CMake configure for this tiny subproject unexpectedly timed out during compiler ABI detection. Direct MSVC compile succeeded:

```powershell
cmd.exe /c '"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cl /nologo /W4 /O2 /Fe:mx-switch\mx-switch.exe mx-switch\mx-switch.c hid.lib setupapi.lib'
```

Output:

- `mx-switch/mx-switch.exe`
