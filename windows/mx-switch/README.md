# mx-switch

`mx-switch` is a narrow Windows-only Logitech Easy-Switch experiment for the MX Master 3S over a Bolt receiver.

It does not use HIDAPI. It links only to Windows SDK libraries:

- `hid.lib`
- `setupapi.lib`

## Language Choice

The tools are currently written in C.

Pros of C:

- Closest match for the Windows HID/setup APIs used here.
- Small binaries and simple compiler/linker commands.
- No C++ runtime or exception model questions while testing security-sensitive behavior.
- Easier to audit the exact write path and byte payload.

Pros of C++:

- RAII wrappers could automatically close `HANDLE` and `HDEVINFO` resources.
- `std::vector` / `std::wstring` would reduce manual buffer management.
- Cleaner parsing and formatting helpers once the tool grows.

Decision:

Stay in C for now. The current priority is a small, auditable, split set of executables with clear safety boundaries. Reconsider C++ if the code grows beyond the current planner/list/switch shape or starts needing more resource ownership helpers.

Supported commands:

```powershell
mx-switch-plan.exe dry-run --slot 2
mx-switch-list.exe
mx-switch-write.exe dry-run --slot 2
mx-switch-write.exe switch --slot 2 --execute
```

Convenience command for Windows -> Mac:

```text
windows\mx-switch\switch-to-mac.cmd
```

The `.cmd` file calls the PowerShell script internally, so launchers and keyboard macro tools can run the `.cmd` path directly.

Underlying PowerShell script:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "windows\mx-switch\switch-to-mac.ps1"
```

Safety model:

- `mx-switch-plan.exe` has no HID imports and only prints planned payloads.
- `mx-switch-list.exe` has no write path and only performs read-only HID inventory.
- `mx-switch-write.exe dry-run` prints the exact payload it would send and does not open a write handle.
- `mx-switch-write.exe switch` refuses to send unless `--execute` is present.
- Do not run `switch --execute` until the target device and payload have been reviewed.

Defaults:

- VID/PID: `046D:C548`
- Usage page: `0xFF00`
- Usage: `0x0001`
- Device index: `0x02`
- Feature/function bytes: `0x0A,0x1B`
- Report length: 7 bytes

The feature/function bytes and receiver device index are configurable for controlled tests:

```powershell
mx-switch-plan.exe dry-run --slot 2 --device-index 0x01 --feature-index 0x0A --function-index 0x1B
mx-switch-write.exe dry-run --slot 2 --device-index 0x01 --feature-index 0x0A --function-index 0x1B
mx-switch-plan.exe dry-run --slot 2 --usage 0x0002 --report long --function-index 0x1E
```

For diagnosis, `mx-switch-write.exe switch` also supports `--read-response` to attempt one input report read after writing:

```powershell
mx-switch-write.exe switch --slot 2 --device-index 0x02 --read-response --execute
```

Slot mapping:

- slot 1 -> channel byte `0x00`
- slot 2 -> channel byte `0x01`
- slot 3 -> channel byte `0x02`
