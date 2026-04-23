# eFuse

Read factory-programmed chip identity and security state from on-chip eFuse storage. This module is read-only and exposes a small common subset across all supported targets.

> **API reference:** `components/blusys/include/blusys/hal/efuse.h` and the generated API reference.

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

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Thread Safety

All functions are safe to call from multiple tasks concurrently.

## ISR Notes

No ISR-safe calls are defined for the eFuse module.

## Limitations

- read-only; Blusys does not expose eFuse programming APIs
- chip revision is reported using ESP-IDF's revision encoding rather than a board-specific marketing name
- security-state helpers report hardware state only

## Example App

See `examples/validation/hal_io_lab` (eFuse scenario).
