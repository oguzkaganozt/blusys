# eFuse

Read factory-programmed chip identity and security state from on-chip eFuse storage. This module is read-only and exposes a small common subset across all supported targets.

## Quick Example

```c
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "blusys/blusys.h"

void app_main(void)
{
    uint8_t mac[BLUSYS_EFUSE_MAC_LEN];
    uint32_t secure_version;
    bool secure_boot_enabled;

    if (blusys_efuse_base_mac(mac) == BLUSYS_OK) {
        printf("mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    if (blusys_efuse_secure_version(&secure_version) == BLUSYS_OK) {
        printf("secure_version=%lu\n", (unsigned long)secure_version);
    }

    if (blusys_efuse_secure_boot_enabled(&secure_boot_enabled) == BLUSYS_OK) {
        printf("secure_boot=%s\n", secure_boot_enabled ? "enabled" : "disabled");
    }
}
```

## Common Mistakes

- expecting board-specific names instead of numeric chip/package revision values
- assuming all boards have secure boot or flash encryption enabled
- trying to use this module to write or burn eFuse values — this API is read-only

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Constants

### `BLUSYS_EFUSE_MAC_LEN`

```c
#define BLUSYS_EFUSE_MAC_LEN 6
```

Length of the factory-programmed base MAC address.

## Functions

### `blusys_efuse_base_mac`

```c
blusys_err_t blusys_efuse_base_mac(uint8_t out_mac[BLUSYS_EFUSE_MAC_LEN]);
```

Reads the factory-programmed base MAC address.

**Parameters:**
- `out_mac` — output buffer for the 6-byte MAC address

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `out_mac` is NULL, translated ESP-IDF error if the MAC cannot be read.

---

### `blusys_efuse_chip_revision`

```c
blusys_err_t blusys_efuse_chip_revision(uint16_t *out_revision);
```

Returns the chip revision in ESP-IDF's `MXX` format.

**Parameters:**
- `out_revision` — output pointer for the revision value

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `out_revision` is NULL.

---

### `blusys_efuse_package_version`

```c
blusys_err_t blusys_efuse_package_version(uint32_t *out_version);
```

Returns the package version reported by the chip's eFuse metadata.

---

### `blusys_efuse_secure_version`

```c
blusys_err_t blusys_efuse_secure_version(uint32_t *out_version);
```

Returns the anti-rollback secure version.

---

### `blusys_efuse_flash_encryption_enabled`

```c
blusys_err_t blusys_efuse_flash_encryption_enabled(bool *out_enabled);
```

Returns whether flash encryption is enabled in hardware.

---

### `blusys_efuse_secure_boot_enabled`

```c
blusys_err_t blusys_efuse_secure_boot_enabled(bool *out_enabled);
```

Returns whether secure boot is enabled in hardware.

## Lifecycle

This module is stateless — there is no open or close step.

## Thread Safety

All functions are safe to call from multiple tasks concurrently.

## ISR Notes

No ISR-safe calls are defined for this module.

## Limitations

- read-only; Blusys does not expose eFuse programming APIs
- chip revision is reported using ESP-IDF's revision encoding rather than a board-specific marketing name
- security-state helpers report hardware state only

## Example App

See `examples/validation/hal_io_lab` (eFuse scenario).
