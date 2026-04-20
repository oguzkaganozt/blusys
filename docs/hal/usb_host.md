# USB Host

USB OTG host stack lifecycle and device enumeration. Installs the ESP-IDF USB host library, manages the daemon and client FreeRTOS tasks, and delivers device connect/disconnect events with VID/PID information.

## Quick Example

```c
#include "blusys/blusys.h"

static void on_usb_event(blusys_usb_host_t *host,
                          blusys_usb_host_event_t event,
                          const blusys_usb_host_device_info_t *device,
                          void *user_ctx)
{
    if (event == BLUSYS_USB_HOST_EVENT_DEVICE_CONNECTED) {
        printf("device: VID=0x%04X PID=0x%04X addr=%u\n",
               device->vid, device->pid, device->dev_addr);
    } else {
        printf("device disconnected\n");
    }
}

blusys_usb_host_t *host;
blusys_usb_host_config_t cfg = {
    .event_cb = on_usb_event,
};
blusys_usb_host_open(&cfg, &host);
/* ... application runs ... */
blusys_usb_host_close(host);
```

The `device` pointer passed to the callback is valid only for the duration of the call. `vid` and `pid` are zero on disconnect events.

## Common Mistakes

- using this on ESP32 or ESP32-C3 — `blusys_usb_host_open()` returns `BLUSYS_ERR_NOT_SUPPORTED`; check `blusys_target_supports(BLUSYS_FEATURE_USB_HOST)` first
- calling `blusys_usb_host_close()` from within the event callback — this deadlocks the client task
- opening `blusys_usb_host` and `blusys_usb_device` simultaneously — the S3 OTG port is shared and only one mode can be active

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | no |
| ESP32-C3 | no |
| ESP32-S3 | yes (USB OTG / DWC2) |

On unsupported targets all public functions return `BLUSYS_ERR_NOT_SUPPORTED`. Use `blusys_target_supports(BLUSYS_FEATURE_USB_HOST)` to check at runtime.

!!! note "Mutual exclusion"
    The ESP32-S3 USB OTG port operates in either host **or** device mode — not both simultaneously. `blusys_usb_host` and `blusys_usb_device` cannot be open at the same time.

## Thread Safety

- `blusys_usb_host_open()` and `blusys_usb_host_close()` are internally serialized
- only one instance is allowed at a time; a second `open()` returns `BLUSYS_ERR_INVALID_STATE`
- the event callback is invoked from the USB client task — do not call `blusys_usb_host_close()` from within it

## ISR Notes

No ISR-safe calls are defined for the USB host module.

## Limitations

- ESP32-S3 only; stubs return `BLUSYS_ERR_NOT_SUPPORTED` on other targets
- mutually exclusive with `blusys_usb_device` (single OTG port)
- the `blusys_usb_host` enumeration client observes all USB devices; HID class drivers (via `blusys_usb_hid`) register additional clients on top of the same host library instance

## Example App

See `examples/validation/usb_peripheral_lab` (USB host scenario).
