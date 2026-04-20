# USB Device

USB device mode via TinyUSB. Configures the ESP32-S3 as a USB peripheral visible to a host PC. Supports CDC (virtual serial port), HID (keyboard / mouse / gamepad), and reserves MSC (mass storage) for a future extension.

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
/* ... application runs ... */
blusys_usb_device_close(dev);
```

String descriptor fields default to `"Blusys"`, `"USB Device"`, and `"1234567890"` when NULL.

## Common Mistakes

- using on ESP32 or ESP32-C3 — returns `BLUSYS_ERR_NOT_SUPPORTED`; check `blusys_target_supports(BLUSYS_FEATURE_USB_DEVICE)`
- forgetting to add `espressif/esp_tinyusb` to `idf_component.yml` — returns `BLUSYS_ERR_NOT_SUPPORTED` at runtime
- enabling CDC mode without `CONFIG_TINYUSB_CDC_ENABLED` — `blusys_usb_device_open()` returns `BLUSYS_ERR_NOT_SUPPORTED`
- opening `blusys_usb_device` and `blusys_usb_host` simultaneously — the S3 OTG port is shared between host and device modes
- blocking inside the RX callback — the TinyUSB task services all USB traffic; blocking stalls the bus

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

## Thread Safety

- only one instance is allowed at a time
- `cdc_write` and `hid_send_report` are internally serialized per handle
- the CDC RX callback is invoked from the TinyUSB task; do not call blocking Blusys functions from it

## ISR Notes

No ISR-safe calls are defined for the USB device module.

## Limitations

- ESP32-S3 only, requires the `espressif/esp_tinyusb` managed component
- `BLUSYS_USB_DEVICE_CLASS_MSC` returns `BLUSYS_ERR_NOT_SUPPORTED`
- mutually exclusive with `blusys_usb_host`

## Example App

See `examples/validation/usb_peripheral_lab` (USB device scenario).
