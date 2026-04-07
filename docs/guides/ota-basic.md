# OTA Firmware Update

Download a firmware binary from an HTTP server and flash it over WiFi.

## Problem Statement

You want to update the firmware on a deployed ESP32 wirelessly, without physically connecting a flash cable, by downloading a new binary from a server.

## Prerequisites

- WiFi station connected (see [WiFi guide](wifi-connect.md))
- blusys installed (`blusys version` to verify)
- Partition table with OTA support (see below)

## Partition Table

The default ESP-IDF partition table does not include OTA partitions. Use the built-in `two_ota` table by adding this to your `sdkconfig` (or via menuconfig → Partition Table):

```
CONFIG_PARTITION_TABLE_TWO_OTA=y
```

Or use a custom `partitions.csv`.

## Minimal Example

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "blusys/blusys_all.h"

void app_main(void)
{
    /* connect WiFi first … */

    blusys_ota_config_t cfg = {
        .url = "http://192.168.1.100:8080/firmware.bin",
    };
    blusys_ota_t *ota = NULL;
    blusys_err_t err = blusys_ota_open(&cfg, &ota);
    if (err != BLUSYS_OK) {
        printf("OTA open failed: %s\n", blusys_err_string(err));
        return;
    }

    printf("Downloading firmware...\n");
    err = blusys_ota_perform(ota);
    blusys_ota_close(ota);

    if (err != BLUSYS_OK) {
        printf("OTA failed: %s\n", blusys_err_string(err));
        return;
    }

    printf("OTA complete. Restarting...\n");
    blusys_ota_restart();
}
```

## Serving the Firmware Binary

Build the new firmware, then serve it with Python from the build directory:

```sh
cd examples/ota_basic/build-esp32s3
python3 -m http.server 8080
```

The device will download `firmware.bin` from `http://<host-ip>:8080/firmware.bin`.

## APIs Used

- `blusys_ota_open()` — allocate an OTA session
- `blusys_ota_perform()` — download and flash firmware (blocking)
- `blusys_ota_close()` — free the handle
- `blusys_ota_restart()` — reboot into the new firmware
- `blusys_ota_mark_valid()` — cancel rollback after verifying the new firmware

## HTTPS

For HTTPS, provide the server's CA certificate in PEM format:

```c
extern const char server_cert[] asm("_binary_ca_cert_pem_start");

blusys_ota_config_t cfg = {
    .url      = "https://example.com/firmware.bin",
    .cert_pem = server_cert,
};
```

Embed the certificate via the ESP-IDF CMake `target_add_binary_data` helper or place it in a `EMBED_FILES` list in `idf_component_register`.

## Rollback

If your partition table is configured with rollback support, call `blusys_ota_mark_valid()` after the device boots the new firmware and you have verified it works:

```c
/* Called after boot, once application is confirmed working */
blusys_err_t err = blusys_ota_mark_valid();
if (err != BLUSYS_OK) {
    printf("mark valid failed: %s\n", blusys_err_string(err));
}
```

Without this call, the bootloader will roll back to the previous firmware on the next reset.

## Common Mistakes

**No OTA partitions in partition table**: `perform()` will return an error. Enable `CONFIG_PARTITION_TABLE_TWO_OTA` or provide a custom partition table.

**WiFi not connected before `open()`**: `open()` will succeed but `perform()` will fail with a network error since there is no interface to connect through.

**Firmware URL points to wrong target**: Flashing firmware built for a different chip results in a validation error from the ESP-IDF bootloader.

**Stack overflow during perform()**: `perform()` calls into the ESP-IDF HTTP client and OTA stack. Run it from a task with at least 8 KB of stack.

## Example App

See `examples/ota_basic/`.

## API Reference

For full type definitions and function signatures, see [OTA API Reference](../modules/ota.md).
