# SD/MMC Card

## Purpose

The `sdmmc` module provides block-level read and write access to SD and MMC cards:

- open an SD/MMC slot with configurable pins, bus width, and frequency
- read one or more 512-byte blocks
- write one or more 512-byte blocks
- close the handle

## Supported Targets

- ESP32
- ESP32-S3

Not available on ESP32-C3 (no SDMMC hardware peripheral).

## Quick Start Example

```c
#include <stdint.h>

#include "blusys/sdmmc.h"

void app_main(void)
{
    blusys_sdmmc_t *sdmmc;
    static uint8_t buf[512];

    /* 1-bit bus, slot 0 */
    if (blusys_sdmmc_open(0, 14, 15, 2, -1, -1, -1, 1, 20000000, &sdmmc) != BLUSYS_OK) {
        return;
    }

    blusys_sdmmc_read_blocks(sdmmc, 0, buf, 1, 1000);
    blusys_sdmmc_close(sdmmc);
}
```

## Lifecycle Model

SDMMC is handle-based:

1. call `blusys_sdmmc_open()` — initializes the slot, negotiates card speed, and mounts the card
2. call `blusys_sdmmc_read_blocks()` or `blusys_sdmmc_write_blocks()` as needed
3. call `blusys_sdmmc_close()` when finished

## Bus Width

Pass `bus_width` as `1` or `4`. When using 1-bit mode, set `d1`, `d2`, and `d3` pins to `-1`.

## Blocking APIs

- `blusys_sdmmc_open()`
- `blusys_sdmmc_close()`
- `blusys_sdmmc_read_blocks()`
- `blusys_sdmmc_write_blocks()`

## Thread Safety

- concurrent operations on the same handle are serialized internally
- do not call `blusys_sdmmc_close()` concurrently with other calls using the same handle

## ISR Notes

No ISR-safe calls are defined for the SDMMC module.

## Limitations

- block size is fixed at 512 bytes; `num_blocks` must be at least 1
- only hardware SDMMC peripheral is used (not SPI-mode SD)
- available only on targets with SDMMC hardware: ESP32 and ESP32-S3
- filesystem layers (FAT, littlefs) are not part of this API; this is raw block access

## Error Behavior

- invalid pins, zero block count, or null pointer return `BLUSYS_ERR_INVALID_ARG`
- card not present or initialization failure returns the translated ESP-IDF error
- timeout waiting for a card operation returns `BLUSYS_ERR_TIMEOUT`
- `BLUSYS_ERR_NOT_SUPPORTED` is returned on ESP32-C3

## Example App

See `examples/sdmmc_basic/`.
