# Keychron Q1 Max Bluetooth

Goal: use the Keychron Q1 Max over Bluetooth instead of relying on a direct USB/KVM connection.

Recommended slot map:

```text
BT1 / Fn+1 -> Windows
BT2 / Fn+2 -> Mac
BT3 / Fn+3 -> spare
```

This matches the mouse convention in this project:

```text
slot 1 -> Windows
slot 2 -> Mac
```

## Pair Windows On BT1

1. Put the keyboard in Bluetooth mode with the hardware connection switch.
2. Put the keyboard in Windows mode with the hardware OS switch.
3. Hold `Fn+1` until the BT1 indicator blinks quickly.
4. On Windows, open Bluetooth pairing and add the keyboard.
5. After pairing, tap `Fn+1` to reconnect to Windows.

## Pair Mac On BT2

1. Put the keyboard in Bluetooth mode with the hardware connection switch.
2. Put the keyboard in Mac mode with the hardware OS switch.
3. Hold `Fn+2` until the BT2 indicator blinks quickly.
4. On macOS, open Bluetooth settings and add the keyboard.
5. After pairing, tap `Fn+2` to reconnect to Mac.

## Daily Switching

Windows -> Mac:

```text
tap Fn+2
set the OS switch to Mac if needed
```

Mac -> Windows:

```text
tap Fn+1
set the OS switch to Windows if needed
```

The keyboard host switch is manual. This project does not currently automate the Keychron Bluetooth slot selection.

## Notes

- Keep the USB cable disconnected during normal switching if the goal is to avoid direct/KVM keyboard routing.
- Use the USB cable only for charging, firmware/configuration work, or fallback access.
- If the keyboard stays paired but keys are swapped, check the hardware OS switch first.
- The VP-SW200 KVM still remains manual for the devices that continue to depend on it.
