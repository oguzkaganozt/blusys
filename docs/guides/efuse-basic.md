# Read eFuse Info

## Problem Statement

You want to inspect factory-programmed identity and security-related chip state without using raw ESP-IDF APIs directly.

## Prerequisites

- Blusys component available in the ESP-IDF project
- a serial monitor connected to the target board

## Minimal Example

```c
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "blusys/efuse.h"

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

## APIs Used

- `blusys_efuse_base_mac()` reads the factory-programmed base MAC address
- `blusys_efuse_chip_revision()` returns the chip revision value
- `blusys_efuse_package_version()` returns the package version
- `blusys_efuse_secure_version()` returns the anti-rollback secure version
- `blusys_efuse_flash_encryption_enabled()` reports flash-encryption state
- `blusys_efuse_secure_boot_enabled()` reports secure-boot state

## Expected Runtime Behavior

- values print immediately; there is no peripheral open step
- identity values remain stable for a given chip
- security-state values reflect hardware provisioning on that board

## Common Mistakes

- expecting board-specific names instead of numeric chip/package revision values
- assuming all boards have secure boot or flash encryption enabled
- trying to use this module to write or burn eFuse values — this API is read-only

## Example App

See `examples/efuse_info/` for a fuller example.

Build and run it with the helper scripts or use the pattern shown in `guides/getting-started.md`.

## API Reference

For full type definitions and function signatures, see [eFuse API Reference](../modules/efuse.md).
