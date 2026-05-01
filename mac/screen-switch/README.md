# screen-switch for macOS

Mac-side SmartThings helpers for the Samsung Odyssey OLED G8/G80SD.

## Token

Store the SmartThings token in macOS Keychain:

```text
mac/screen-switch/smartthings-credential.sh set
```

The Keychain service name is `desk-switch-smartthings-token`.

Delete it with:

```text
mac/screen-switch/smartthings-credential.sh delete
```

## Read Commands

```text
mac/screen-switch/list-smartthings-devices.sh
mac/screen-switch/get-monitor-status.sh
```

## Switch Commands

For this setup:

- `HDMI1` is the Mac-facing input.
- `HDMI2` is the Windows-facing input.

```text
mac/screen-switch/switch-monitor-hdmi1.sh
mac/screen-switch/switch-monitor-hdmi2.sh
```

The fixed launchers wait two seconds after input switching, then send remote key `BACK` to dismiss the Samsung bottom overlay. Do not use `EXIT` automatically; it was observed on Windows to send the monitor to Home.
