# SD/MMC Card

Block-level read and write access to SD and MMC cards using the hardware SDMMC peripheral. For a FAT-backed VFS mount, use [`sd_spi`](sd_spi.md) instead.

## Quick Example

```c
blusys_sdmmc_t *sdmmc;
static uint8_t buf[512];

/* slot 0, 1-bit bus, 20 MHz */
blusys_sdmmc_open(0, 14, 15, 2, -1, -1, -1, 1, 20000000, &sdmmc);

blusys_sdmmc_read_blocks(sdmmc, 0, buf, 1, 1000);    /* read block 0 */
blusys_sdmmc_write_blocks(sdmmc, 0, buf, 1, 1000);   /* write it back */
blusys_sdmmc_close(sdmmc);
```

## Common Mistakes

- using this module on ESP32-C3 (SDMMC peripheral not available; returns `BLUSYS_ERR_NOT_SUPPORTED`)
- writing to block 0 on a card that has a filesystem on it
- using 4-bit mode but only wiring D0 — the card will silently fall back to 1-bit mode in some cases, or fail

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | no |
| ESP32-S3 | yes |

On ESP32-C3, all public functions return `BLUSYS_ERR_NOT_SUPPORTED`. Use `blusys_target_supports(BLUSYS_FEATURE_SDMMC)` to check at runtime.

## Thread Safety

- concurrent operations on the same handle are serialized internally
- do not call `blusys_sdmmc_close()` concurrently with other calls on the same handle

## ISR Notes

No ISR-safe calls are defined for the SDMMC module.

## Limitations

- block size is fixed at 512 bytes
- only the hardware SDMMC peripheral is used (not SPI-mode SD)
- filesystem layers (FAT, littlefs) are not part of this API; this is raw block access

## Example App

See `examples/validation/peripheral_lab/` (`periph_sdmmc` scenario).
