# Appear as a USB CDC Serial Device

## Problem Statement

You want the ESP32-S3 to appear as a virtual serial port (USB CDC ACM) on a host PC so you can send and receive data over USB without a separate UART-to-USB chip.

## Prerequisites

- **Target:** ESP32-S3 (USB OTG not available on ESP32 or ESP32-C3)
- **Hardware:** USB cable between ESP32-S3 OTG port and a host PC
- **Managed component:** `espressif/esp_tinyusb` — declare in `main/idf_component.yml`:

```yaml
dependencies:
  idf: ">=5.0.0"
  espressif/esp_tinyusb: "~0.14.0"
```

## Minimal Example

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

## APIs Used

- `blusys_usb_device_open()` — installs TinyUSB and registers the CDC ACM class driver
- `blusys_usb_device_cdc_set_rx_callback()` — invoked whenever the host sends bytes
- `blusys_usb_device_cdc_write()` — queues bytes for transmission to the host; flushes immediately
- `blusys_usb_device_close()` — uninstalls TinyUSB

## Expected Runtime Behavior

- After `open()`, the device appears on the host PC as a CDC serial port (e.g. `COMx` on Windows, `/dev/ttyACM0` on Linux)
- The RX callback fires from the TinyUSB internal task when data is received from the host
- `cdc_write()` is safe to call from any task

## Common Mistakes

- Using on ESP32 or ESP32-C3 — returns `BLUSYS_ERR_NOT_SUPPORTED`; check `blusys_target_supports(BLUSYS_FEATURE_USB_DEVICE)`
- Forgetting to add `espressif/esp_tinyusb` to `idf_component.yml` — returns `BLUSYS_ERR_NOT_SUPPORTED` at runtime
- Opening `blusys_usb_device` and `blusys_usb_host` simultaneously — the S3 OTG port is shared
- Blocking inside the RX callback — the TinyUSB task services all USB traffic; blocking stalls the bus

## Example App

See `examples/usb_device_basic/` for a runnable CDC echo example.

## API Reference

For full type definitions and function signatures, see [USB Device API Reference](../modules/usb_device.md).
