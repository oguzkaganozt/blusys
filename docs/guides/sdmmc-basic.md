# Read And Write SD Card Blocks

## Problem Statement

You want raw block-level read and write access to an SD or MMC card connected to the SDMMC peripheral.

## Prerequisites

- a supported board (ESP32 or ESP32-S3)
- an SD card slot wired to CLK, CMD, and at least D0 (1-bit mode) or D0–D3 (4-bit mode)
- the SD card inserted and powered

## Minimal Example

```c
blusys_sdmmc_t *sdmmc;
static uint8_t buf[512];

/* slot 0, 1-bit bus, 20 MHz */
blusys_sdmmc_open(0, 14, 15, 2, -1, -1, -1, 1, 20000000, &sdmmc);

blusys_sdmmc_read_blocks(sdmmc, 0, buf, 1, 1000);    /* read block 0 */
blusys_sdmmc_write_blocks(sdmmc, 0, buf, 1, 1000);   /* write it back */
blusys_sdmmc_close(sdmmc);
```

## APIs Used

- `blusys_sdmmc_open()` initializes the slot: CLK pin, CMD pin, D0–D3 pins (pass -1 for unused data lines), bus width (1 or 4), and frequency in Hz
- `blusys_sdmmc_read_blocks()` reads `num_blocks` × 512-byte blocks starting at `start_block`
- `blusys_sdmmc_write_blocks()` writes `num_blocks` × 512-byte blocks starting at `start_block`
- `blusys_sdmmc_close()` unmounts the card and releases the handle

## Expected Runtime Behavior

- `open()` negotiates the card's operating voltage and speed and prints the card capacity
- blocks are numbered from 0; block 0 is the MBR/partition table for formatted cards
- writes to block 0 on a formatted card will corrupt the filesystem; use a test region or a blank card

## Common Mistakes

- using this module on ESP32-C3 (SDMMC peripheral not available; returns `BLUSYS_ERR_NOT_SUPPORTED`)
- writing to block 0 on a card that has a filesystem on it
- using 4-bit mode but only wiring D0 (the card will silently fall back to 1-bit mode in some cases, or fail)

## Example App

See `examples/sdmmc_basic/` for a runnable example.

Build and run it with the helper scripts or use the pattern shown in `guides/getting-started.md`.


## API Reference

For full type definitions and function signatures, see [SD/MMC API Reference](../modules/sdmmc.md).
