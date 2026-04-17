# USB Device

USB device mode via TinyUSB. Configures the ESP32-S3 as a USB peripheral visible to a host PC. Supports CDC (virtual serial port), HID (keyboard/mouse/gamepad), and provides stubs for MSC (mass storage, future extension).

## Quick Example

```c
#include "blusys/blusys.h"

static void on_rx(blusys_usb_device_t *dev,
                  const uint8_t *data, size_t len,
                  void *ctx)
{
    /* Echo back to host */
    blusys_usb_device_cdc_write(dev, data, len);
}

blusys_usb_device_t *dev;
blusys_usb_device_config_t cfg = {
    .vid          = 0x303A,
    .pid          = 0x4001,
    .manufacturer = "Acme",
    .product      = "CDC Demo",
    .device_class = BLUSYS_USB_DEVICE_CLASS_CDC,
};
blusys_usb_device_open(&cfg, &dev);
blusys_usb_device_cdc_set_rx_callback(dev, on_rx, NULL);
// ... application runs ...
blusys_usb_device_close(dev);
```

## Common Mistakes

- Using on ESP32 or ESP32-C3 — returns `BLUSYS_ERR_NOT_SUPPORTED`; check `blusys_target_supports(BLUSYS_FEATURE_USB_DEVICE)`
- Forgetting to add `espressif/esp_tinyusb` to `idf_component.yml` — returns `BLUSYS_ERR_NOT_SUPPORTED` at runtime
- Enabling CDC mode without `CONFIG_TINYUSB_CDC_ENABLED` — `blusys_usb_device_open()` returns `BLUSYS_ERR_NOT_SUPPORTED`
- Opening `blusys_usb_device` and `blusys_usb_host` simultaneously — the S3 OTG port is shared
- Blocking inside the RX callback — the TinyUSB task services all USB traffic; blocking stalls the bus

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | no |
| ESP32-C3 | no |
| ESP32-S3 | yes (USB OTG / DWC2, requires `espressif/esp_tinyusb`) |

On unsupported targets, or when `espressif/esp_tinyusb` is not present in the project's `idf_component.yml`, all functions return `BLUSYS_ERR_NOT_SUPPORTED`.

!!! note "Managed component dependency"
    Add `espressif/esp_tinyusb: "^2.1.1"` to your project's `main/idf_component.yml`. See `examples/validation/usb_peripheral_lab/main/idf_component.yml` as a reference.

!!! note "Mutual exclusion"
    The ESP32-S3 USB OTG port operates in either host **or** device mode. `blusys_usb_device` and `blusys_usb_host` cannot be open simultaneously.

## Types

### `blusys_usb_device_t`

```c
typedef struct blusys_usb_device blusys_usb_device_t;
```

Opaque handle returned by `blusys_usb_device_open()`.

### `blusys_usb_device_class_t`

```c
typedef enum {
    BLUSYS_USB_DEVICE_CLASS_CDC,
    BLUSYS_USB_DEVICE_CLASS_HID,
    BLUSYS_USB_DEVICE_CLASS_MSC,  /* not yet implemented */
} blusys_usb_device_class_t;
```

### `blusys_usb_device_event_t`

```c
typedef enum {
    BLUSYS_USB_DEVICE_EVENT_CONNECTED,
    BLUSYS_USB_DEVICE_EVENT_DISCONNECTED,
    BLUSYS_USB_DEVICE_EVENT_SUSPENDED,
    BLUSYS_USB_DEVICE_EVENT_RESUMED,
} blusys_usb_device_event_t;
```

### `blusys_usb_device_config_t`

```c
typedef struct {
    uint16_t    vid;
    uint16_t    pid;
    const char *manufacturer;
    const char *product;
    const char *serial;
    blusys_usb_device_class_t device_class;
    blusys_usb_device_event_callback_t event_cb;
    void *event_user_ctx;
} blusys_usb_device_config_t;
```

String fields default to `"Blusys"`, `"USB Device"`, and `"1234567890"` when NULL.

## Functions

### `blusys_usb_device_open`

```c
blusys_err_t blusys_usb_device_open(const blusys_usb_device_config_t *config,
                                     blusys_usb_device_t **out_dev);
```

Installs the TinyUSB driver and initializes the requested device class.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_INVALID_STATE` if already open, `BLUSYS_ERR_NOT_SUPPORTED` when TinyUSB is unavailable, CDC is disabled for CDC mode, or MSC class requested.

---

### `blusys_usb_device_close`

```c
blusys_err_t blusys_usb_device_close(blusys_usb_device_t *dev);
```

Uninstalls the TinyUSB driver and frees the handle.

---

### `blusys_usb_device_cdc_write`

```c
blusys_err_t blusys_usb_device_cdc_write(blusys_usb_device_t *dev,
                                          const void *data, size_t len);
```

Writes data to the CDC ACM TX FIFO and flushes it to the host. Only valid when `device_class` is `BLUSYS_USB_DEVICE_CLASS_CDC`.

---

### `blusys_usb_device_cdc_set_rx_callback`

```c
blusys_err_t blusys_usb_device_cdc_set_rx_callback(
    blusys_usb_device_t *dev,
    blusys_usb_device_cdc_rx_callback_t callback,
    void *user_ctx);
```

Registers a callback invoked when the host sends data over CDC ACM. Called from the TinyUSB task — must not block.

---

### `blusys_usb_device_hid_send_report`

```c
blusys_err_t blusys_usb_device_hid_send_report(blusys_usb_device_t *dev,
                                                 const uint8_t *report,
                                                 size_t len);
```

Sends a HID input report to the host. Only valid when `device_class` is `BLUSYS_USB_DEVICE_CLASS_HID`.

## Lifecycle

1. `blusys_usb_device_open()` — install TinyUSB, configure class
2. For CDC: `blusys_usb_device_cdc_set_rx_callback()` to handle received data
3. Send data via `blusys_usb_device_cdc_write()` or `blusys_usb_device_hid_send_report()`
4. `blusys_usb_device_close()` — uninstall TinyUSB

## Thread Safety

- Only one instance is allowed at a time
- `cdc_write` and `hid_send_report` are internally serialized per handle
- CDC RX callback is invoked from the TinyUSB task; do not call blocking blusys functions from it

## Limitations

- ESP32-S3 only, requires `espressif/esp_tinyusb` managed component
- `BLUSYS_USB_DEVICE_CLASS_MSC` returns `BLUSYS_ERR_NOT_SUPPORTED`
- Mutually exclusive with `blusys_usb_host`

## Example App

See `examples/validation/usb_peripheral_lab` (USB device scenario).
