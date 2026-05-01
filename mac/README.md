# mac switchback

Mac-side counterpart for returning from the Mac setup back toward Windows.

The intended top-level command is:

```text
mac/switch-to-windows.sh
```

It switches:

- MX Master 3S from Mac Bluetooth slot 2 back to Windows slot 1.
- Samsung Odyssey OLED G8/G80SD back to the Windows-facing SmartThings input, `HDMI2`.
- VP-SW200 KVM remains a manual button press.

## Build

Build the native Mac HID helpers with:

```text
mac/build-mac.sh
```

This creates:

- `mac/mx-switch/mx-switch-plan`
- `mac/mx-switch/mx-switch-list`
- `mac/mx-switch/mx-switch-write`

## Safety

Start with read-only and dry-run commands:

```text
mac/mx-switch/mx-switch-list
mac/mx-switch/mx-switch-plan dry-run --slot 1
mac/mx-switch/mx-switch-write dry-run --slot 1
```

Do not run `mx-switch-write switch ... --execute` until the exact Bluetooth HID interface and payload have been reviewed. A successful switch moves the mouse away from the Mac.

## SmartThings Token

Store the SmartThings token in macOS Keychain:

```text
mac/screen-switch/smartthings-credential.sh set
```

The service name is:

```text
desk-switch-smartthings-token
```

The token is not stored in this repo.
