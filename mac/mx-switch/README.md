# mx-switch for macOS

Mac-side Logitech MX Master 3S Easy-Switch experiment for the Bluetooth connection on slot 2.

The tools use native macOS IOKit/CoreFoundation APIs and do not use HIDAPI.

## Tools

```text
mac/mx-switch/mx-switch-plan dry-run --slot 1
mac/mx-switch/mx-switch-list
mac/mx-switch/mx-switch-write dry-run --slot 1
mac/mx-switch/mx-switch-write switch --slot 1 --execute
```

Safety model:

- `mx-switch-plan` has no IOKit imports and only prints planned payloads.
- `mx-switch-list` enumerates HID devices and prints metadata only.
- `mx-switch-write dry-run` prints the exact payload it would send and does not open the HID device.
- `mx-switch-write switch` refuses to send unless `--execute` is present.

## Defaults

The Mac defaults target the likely MX Master 3S Bluetooth HID++ interface:

- Vendor ID: `0x046D`
- Product ID: `0xB034`
- Usage page: `0x0001`
- Usage: `0x0002`
- Device index byte: `0xFF`
- Feature/function bytes: `0x0A,0x1B`
- Report ID: `0x11`
- Output payload length: 20 bytes including report ID
- IOHID send mode: `with-report-id`

Verify the actual interface locally with:

```text
mac/mx-switch/mx-switch-list --all
```

If more than one matching interface exists, pass `--registry-path` with the exact path printed by `mx-switch-list`.

## Verified Mac -> Windows Mouse Switch

After granting Input Monitoring permission to Cursor, this command wrote successfully and the MX Master 3S disappeared from the Mac HID inventory:

```text
mac/mx-switch/mx-switch-write switch --slot 1 --execute --send with-report-id
```

The default send mode is now `with-report-id`, so this is equivalent:

```text
mac/mx-switch/mx-switch-write switch --slot 1 --execute
```

## macOS Permission

If the write path fails with:

```text
IOHIDDeviceOpen failed: 0xE00002E2
```

macOS denied HID access before any report was written. Grant Input Monitoring permission to the app running the command, then rerun it. If running from Cursor, grant Cursor. If running from Terminal, grant Terminal.
