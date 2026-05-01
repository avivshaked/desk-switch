# Logitech Easy-Switch Notes

## Local Device Identifiers

Known from user context and passive local checks:

- Logitech Bolt receiver: `VID_046D&PID_C548`
- USB composite interfaces seen in registry:
  - `USB\VID_046D&PID_C548`
  - `USB\VID_046D&PID_C548&MI_00`
  - `USB\VID_046D&PID_C548&MI_01`
  - `USB\VID_046D&PID_C548&MI_02`
- Bluetooth MX Master 3S device key:
  - `BTHLE\Dev_dd8631cac5b3`

Read-only `mx-switch-list.exe` found one exact Bolt receiver HID interface:

```text
046D:C548 usagePage=0xFF00 usage=0x0001
  manufacturer: Logitech
  product:      USB Receiver
  path:         \\?\hid#vid_046d&pid_c548&mi_02&col01#8&23c1964a&0&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}
```

Community references identify common values:

- Bolt receiver VID/PID: `046D:C548`
- MX Master 3S Bluetooth PID: often `046D:B034`
- Bolt/Unifying HID usage filter: `--usagePage 0xFF00 --usage 0x0001`
- Direct Bluetooth HID usage filter: `--usagePage 0xFF43 --usage 0x0202`

These values were verified with the standalone `mx-switch` tools in this handoff project.

## Safe Read/List Commands

After building with `windows\build-windows.cmd`, start with passive list commands:

```powershell
.\windows\mx-switch\mx-switch-list.exe
```

```powershell
.\windows\mx-switch\mx-switch-list.exe --all
```

## Candidate Write Commands: Do Not Execute Without Explicit Approval

These commands write HID output reports and can switch the mouse away from the current computer.

Candidate Bolt command to switch MX Master 3S to slot 2 / Mac:

```powershell
.\windows\mx-switch\mx-switch-write.exe switch --slot 2 --device-index 0x03 --read-response --timeout-ms 700 --execute
```

Candidate Bolt command to switch MX Master 3S to slot 1 / Windows:

```powershell
.\windows\mx-switch\mx-switch-write.exe switch --slot 1 --device-index 0x03 --read-response --timeout-ms 700 --execute
```

Meaning of the command parts:

- `--vidpid 046D:C548`: target Logitech Bolt receiver.
- `--usagePage 0xFF00 --usage 0x0001`: target the vendor-defined HID++ receiver interface commonly used for Bolt/Unifying receivers.
- `--open`: open the matched HID interface.
- `--length 7`: send a 7-byte HID output report.
- `--send-output ...`: write the output report. This is the risky part.
- `0x10`: short HID++ message type.
- `0x03`: verified receiver device index for the MX Master 3S in this setup.
- `0x0a,0x1b`: candidate feature/function bytes observed in community MX Master 3S/Bolt examples; must be verified for this exact setup.
- `0x01`: target slot/channel 2; verified by successful switch to Mac.
- `0x00`: target slot/channel 1.
- trailing `0x00,0x00`: padding.

Risks:

- If the device index is wrong, the command may affect another paired Logitech device.
- If the feature/function bytes are wrong, the command may do nothing or trigger an unintended HID++ function.
- If the channel byte is wrong, the mouse may switch to the wrong Easy-Switch slot.
- Switching away from Windows can make the mouse unavailable on Windows until manually switched back.
- These commands should not affect firmware, drivers, registry, services, or startup entries; the risk is runtime device behavior through HID output reports.
