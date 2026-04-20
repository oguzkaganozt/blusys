# USB HID

HID input service with dual-transport support: USB OTG host (ESP32-S3, via `espressif/usb_host_hid`) and BLE HOGP central (all targets, via NimBLE). Parses boot-protocol keyboard and mouse reports and delivers them through a unified callback.

## Quick Example

```c
#include "blusys/blusys.h"

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

## Common Mistakes

- requesting `BLUSYS_USB_HID_TRANSPORT_USB` on a non-S3 target — returns `BLUSYS_ERR_NOT_SUPPORTED`
- requesting `BLUSYS_USB_HID_TRANSPORT_BLE` without enabling `CONFIG_BT_ENABLED=y`, `CONFIG_BT_NIMBLE_ENABLED=y`, `CONFIG_BT_NIMBLE_ROLE_CENTRAL=y`, and `CONFIG_BT_NIMBLE_ROLE_OBSERVER=y` — returns `BLUSYS_ERR_NOT_SUPPORTED`
- using BLE `blusys_usb_hid` alongside `blusys_bluetooth`, `blusys_ble_gatt`, `blusys_ble_hid_device`, or BLE `blusys_wifi_prov` — only one BLE-owning service may be open at a time
- retaining the `raw.data` pointer after the callback returns — it is stack-allocated and invalid after the call

## Target Support

| Target | USB transport | BLE transport |
|--------|--------------|---------------|
| ESP32 | no | yes (requires NimBLE) |
| ESP32-C3 | no | yes (requires NimBLE) |
| ESP32-S3 | yes (requires `espressif/usb_host_hid`) | yes (requires NimBLE) |

`BLUSYS_FEATURE_USB_HID` is set on all targets. The USB transport additionally requires `BLUSYS_FEATURE_USB_HOST`. Requesting an unsupported transport returns `BLUSYS_ERR_NOT_SUPPORTED`.

!!! note "Managed component — USB transport"
    Add `espressif/usb_host_hid: "^1.2.0"` to your project's `main/idf_component.yml`. See `examples/validation/usb_peripheral_lab/main/idf_component.yml`.

!!! note "BLE transport"
    Enable `CONFIG_BT_ENABLED=y`, `CONFIG_BT_NIMBLE_ENABLED=y`, `CONFIG_BT_NIMBLE_ROLE_CENTRAL=y`, and `CONFIG_BT_NIMBLE_ROLE_OBSERVER=y`. Requires NVS (initialized automatically).

## Thread Safety

- only one instance is allowed at a time; a second `open()` returns `BLUSYS_ERR_INVALID_STATE`
- the callback is invoked from the USB HID task (USB transport) or NimBLE host task (BLE transport) — do not call `blusys_usb_hid_close()` from within it

## Limitations

- raw report `data` pointer is valid only during the callback — copy if needed
- boot-protocol parsing heuristic: 8-byte reports are treated as keyboards, 3-4-byte reports as mice, shorter reports as raw
- BLE transport initiates a new scan after each disconnect; `ble_device_name` filter applies to every reconnect attempt
- BLE transport cannot be used simultaneously with `blusys_bluetooth`, `blusys_ble_gatt`, `blusys_ble_hid_device`, or BLE-transport `blusys_wifi_prov`

## Example App

See `examples/validation/usb_peripheral_lab` (USB HID host scenario).
