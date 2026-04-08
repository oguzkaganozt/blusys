# Receive HID Input Reports

## Problem Statement

You want to read keyboard or mouse input from a HID device, either wired via USB OTG (ESP32-S3) or wirelessly via a BLE HID peripheral (all targets).

## Prerequisites

**USB transport (ESP32-S3 only):**

- Hardware: USB OTG cable + HID device (keyboard or mouse)
- Managed component: `espressif/usb_host_hid` — declare in `main/idf_component.yml`:

```yaml
dependencies:
  idf: ">=5.0.0"
  espressif/usb_host_hid: "~1.1.0"
```

**BLE transport (all targets):**

- Hardware: a BLE HID peripheral (wireless keyboard, mouse, or gamepad)
- Enable NimBLE in menuconfig: `Component config → Bluetooth → NimBLE - BLE stack`
- No managed component dependency

## Minimal Example

```c
#include "blusys/input/usb_hid.h"

static void on_hid(blusys_usb_hid_t *hid,
                   const blusys_usb_hid_event_data_t *event,
                   void *ctx)
{
    if (event->event == BLUSYS_USB_HID_EVENT_KEYBOARD_REPORT) {
        printf("modifier=0x%02X  keys=[%02X %02X %02X %02X %02X %02X]\n",
               event->data.keyboard.modifier,
               event->data.keyboard.keycodes[0],
               event->data.keyboard.keycodes[1],
               event->data.keyboard.keycodes[2],
               event->data.keyboard.keycodes[3],
               event->data.keyboard.keycodes[4],
               event->data.keyboard.keycodes[5]);
    }
}

blusys_usb_hid_t *hid;
blusys_usb_hid_config_t cfg = {
    .transport = BLUSYS_USB_HID_TRANSPORT_USB, /* or _BLE */
    .callback  = on_hid,
};
blusys_usb_hid_open(&cfg, &hid);
// ... wait for input ...
blusys_usb_hid_close(hid);
```

## APIs Used

- `blusys_usb_hid_open()` — opens the selected transport and begins listening for HID input
- `blusys_usb_hid_close()` — terminates the transport and frees the handle
- Callback fields: `event->event` selects the active union member (`keyboard`, `mouse`, or `raw`)

## Expected Runtime Behavior

**USB transport:**

- `CONNECTED` fires when a HID device is plugged in
- Keyboard/mouse reports are parsed from boot protocol and delivered via callback
- Gamepads and other devices deliver `RAW_REPORT` events
- `DISCONNECTED` fires on unplug; the driver is ready for the next device

**BLE transport:**

- NimBLE starts scanning for BLE HID peripherals after `open()` returns
- If `ble_device_name` is set, only the named device is accepted
- `CONNECTED` fires after GATT service discovery and CCCD subscription are complete
- On disconnect, scanning resumes automatically
- Reports arrive as GATT notifications parsed into keyboard, mouse, or raw events

## Common Mistakes

- Requesting `BLUSYS_USB_HID_TRANSPORT_USB` on a non-S3 target — returns `BLUSYS_ERR_NOT_SUPPORTED`
- Requesting `BLUSYS_USB_HID_TRANSPORT_BLE` without enabling NimBLE in menuconfig — returns `BLUSYS_ERR_NOT_SUPPORTED`
- Using `blusys_usb_hid` (BLE) alongside `blusys_ble_gatt` — both initialize NimBLE and conflict
- Retaining the `raw.data` pointer after the callback returns — it is stack-allocated and invalid after the call

## Example App

See `examples/usb_hid_basic/` for a runnable example. Use menuconfig to select the transport and optionally specify a BLE device name filter.

## API Reference

For full type definitions and function signatures, see [USB HID API Reference](../modules/usb_hid.md).
