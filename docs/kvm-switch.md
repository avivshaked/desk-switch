# KVM Switch Notes

Device discussed:

- VP-SW200 HDMI KVM switch 2x1, likely VPFET-class model.

Goal:

- Automate KVM switching from Windows so the full Windows -> Mac transition can include mouse, monitor, and USB/KVM.

Known working automation outside the KVM:

- Mouse to Mac slot 2: `mx-switch/switch-to-mac.cmd`
- Screen to Mac input: `screen-switch/switch-monitor-hdmi1.cmd`
- Keyboard can avoid USB/KVM routing by using Keychron Bluetooth slots; see `docs/keychron-bluetooth.md`.

Manual/keyboard hotkey testing:

- User extensively tested keyboard switching/hotkey combinations.
- Result: no working keyboard hotkey was found.
- Treat keyboard hotkey switching as unsupported unless a model-specific manual proves otherwise.

Online manual notes:

- VP-SW200 HDMI manual lists switching methods as:
  - front-panel Select button
  - external desktop controller connected to the Control port
- The manual does not clearly document a USB HID software-control interface.
- Some marketplace listings mention hotkeys for related VP-SW200/DisplayPort variants, but these may be inaccurate or for a different model.

Read-only Windows inventory:

- HID inventory did not show an obvious KVM vendor/control interface.
- Visible HID devices were mainly:
  - Keychron Q1 Max
  - Logitech Bolt receiver
  - Logitech webcam/audio
  - Asus AURA controller
  - Corsair PSU
- USB registry inventory showed several hubs and peripherals, but no identifiable KVM control device.

Interpretation:

- Option 2, "USB HID control interface", currently looks unlikely for this exact KVM.
- The KVM appears to expose USB hub/peripheral pass-through rather than a distinct switch-control HID interface.
- If software control exists, it is not obvious from standard HID enumeration.

Most realistic automation fallback:

- Use the external desktop controller/control port electrically.
- If the controller is a simple momentary switch, it may be possible to automate it with a small USB relay/GPIO device wired across the button contacts.
- This would be hardware work and should be handled carefully.

Keyboard direction:

- Prefer Bluetooth pairing for the Keychron Q1 Max instead of trying to switch the KVM through keyboard hotkeys.
- Recommended map is BT1 / `Fn+1` for Windows and BT2 / `Fn+2` for Mac.

