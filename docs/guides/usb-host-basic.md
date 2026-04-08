# Enumerate USB Devices

## Problem Statement

You want to detect when USB devices (keyboards, mice, hubs, drives) are connected and disconnected from an ESP32-S3's USB OTG port, and log their VID/PID.

## Prerequisites

- **Target:** ESP32-S3 (USB OTG is not available on ESP32 or ESP32-C3)
- **Hardware:** USB OTG cable or hub connected to the ESP32-S3 `D+`/`D−` pins
- No managed component dependency — the `usb` component is built into ESP-IDF

## Minimal Example

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
// ... application runs ...
blusys_usb_host_close(host);
```

## APIs Used

- `blusys_usb_host_open()` — installs the USB host library and starts two internal FreeRTOS tasks (daemon + enumeration client)
- `blusys_usb_host_close()` — stops both tasks and uninstalls the library; blocks until they exit

## Expected Runtime Behavior

- The daemon task runs `usb_host_lib_handle_events()` to manage bus-level events
- The client task runs `usb_host_client_handle_events()` and fires the user callback
- On device connect: the callback receives VID, PID, and device address from the device descriptor
- On device disconnect: the callback is invoked with zeroed `device` fields
- Connecting a device that requires a class driver (e.g. HID keyboard) will log a connect event but the class driver itself is not active unless `blusys_usb_hid` is also open

## Common Mistakes

- Using this on ESP32 or ESP32-C3 — `blusys_usb_host_open()` returns `BLUSYS_ERR_NOT_SUPPORTED`; check `blusys_target_supports(BLUSYS_FEATURE_USB_HOST)` first
- Calling `blusys_usb_host_close()` from within the event callback — this deadlocks the client task
- Opening `blusys_usb_host` and `blusys_usb_device` simultaneously — the S3 OTG port is shared and only one mode can be active

## Example App

See `examples/usb_host_basic/` for a runnable example.

## API Reference

For full type definitions and function signatures, see [USB Host API Reference](../modules/usb_host.md).
