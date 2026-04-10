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
// ... application runs ...
blusys_usb_host_close(host);
```

## Common Mistakes

- Using this on ESP32 or ESP32-C3 — `blusys_usb_host_open()` returns `BLUSYS_ERR_NOT_SUPPORTED`; check `blusys_target_supports(BLUSYS_FEATURE_USB_HOST)` first
- Calling `blusys_usb_host_close()` from within the event callback — this deadlocks the client task
- Opening `blusys_usb_host` and `blusys_usb_device` simultaneously — the S3 OTG port is shared and only one mode can be active

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | no |
| ESP32-C3 | no |
| ESP32-S3 | yes (USB OTG / DWC2) |

On unsupported targets all public functions return `BLUSYS_ERR_NOT_SUPPORTED`. Use `blusys_target_supports(BLUSYS_FEATURE_USB_HOST)` to check at runtime.

!!! note "Mutual exclusion"
    The ESP32-S3 USB OTG port operates in either host **or** device mode — not both simultaneously. `blusys_usb_host` and `blusys_usb_device` cannot be open at the same time.

## Types

### `blusys_usb_host_t`

```c
typedef struct blusys_usb_host blusys_usb_host_t;
```

Opaque handle returned by `blusys_usb_host_open()`.

### `blusys_usb_host_device_info_t`

```c
typedef struct {
    uint16_t vid;
    uint16_t pid;
    uint8_t  dev_addr;
} blusys_usb_host_device_info_t;
```

Device descriptor fields delivered with connect events. `vid` and `pid` are zero on disconnect events.

### `blusys_usb_host_event_t`

```c
typedef enum {
    BLUSYS_USB_HOST_EVENT_DEVICE_CONNECTED,
    BLUSYS_USB_HOST_EVENT_DEVICE_DISCONNECTED,
} blusys_usb_host_event_t;
```

### `blusys_usb_host_event_callback_t`

```c
typedef void (*blusys_usb_host_event_callback_t)(
    blusys_usb_host_t *host,
    blusys_usb_host_event_t event,
    const blusys_usb_host_device_info_t *device,
    void *user_ctx);
```

Called from the USB client task. Must not block. The `device` pointer is valid only for the duration of the callback.

### `blusys_usb_host_config_t`

```c
typedef struct {
    blusys_usb_host_event_callback_t event_cb;
    void *event_user_ctx;
} blusys_usb_host_config_t;
```

## Functions

### `blusys_usb_host_open`

```c
blusys_err_t blusys_usb_host_open(const blusys_usb_host_config_t *config,
                                   blusys_usb_host_t **out_host);
```

Installs the USB host library and starts the daemon and client tasks.

**Parameters:**
- `config` — configuration (must not be NULL)
- `out_host` — output handle (must not be NULL)

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_INVALID_STATE` if already open, `BLUSYS_ERR_NO_MEM`, `BLUSYS_ERR_NOT_SUPPORTED` on non-S3 targets, translated ESP-IDF error on driver failure.

---

### `blusys_usb_host_close`

```c
blusys_err_t blusys_usb_host_close(blusys_usb_host_t *host);
```

Stops the daemon and client tasks, deregisters the enumeration client, and uninstalls the USB host library.

**Parameters:**
- `host` — handle from `blusys_usb_host_open()`

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`.

## Lifecycle

1. `blusys_usb_host_open()` — install host lib, start tasks
2. Events arrive via `event_cb` as devices are connected and disconnected
3. `blusys_usb_host_close()` — stop tasks, uninstall

## Thread Safety

- `blusys_usb_host_open()` and `blusys_usb_host_close()` are internally serialized
- Only one instance is allowed at a time; a second `open()` returns `BLUSYS_ERR_INVALID_STATE`
- The event callback is invoked from the USB client task — do not call `blusys_usb_host_close()` from within it

## Limitations

- ESP32-S3 only; stubs return `BLUSYS_ERR_NOT_SUPPORTED` on other targets
- Mutually exclusive with `blusys_usb_device` (single OTG port)
- The `blusys_usb_host` enumeration client observes all USB devices; HID class drivers (via `blusys_usb_hid`) register additional clients on top of the same host library instance

## Example App

See `examples/validation/usb_host_basic/`.
