# SD/MMC Card

Block-level read and write access to SD and MMC cards using the hardware SDMMC peripheral.

!!! tip "Task Guide"
    For a step-by-step walkthrough, see [SD/MMC Basics](../guides/sdmmc-basic.md).

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | no |
| ESP32-S3 | yes |

On ESP32-C3, all public functions return `BLUSYS_ERR_NOT_SUPPORTED`. Use `blusys_target_supports(BLUSYS_FEATURE_SDMMC)` to check at runtime.

## Types

### `blusys_sdmmc_t`

```c
typedef struct blusys_sdmmc blusys_sdmmc_t;
```

Opaque handle returned by `blusys_sdmmc_open()`.

## Functions

### `blusys_sdmmc_open`

```c
blusys_err_t blusys_sdmmc_open(int slot,
                               int clk_pin,
                               int cmd_pin,
                               int d0_pin,
                               int d1_pin,
                               int d2_pin,
                               int d3_pin,
                               int bus_width,
                               uint32_t freq_hz,
                               blusys_sdmmc_t **out_sdmmc);
```

Initializes the SDMMC slot, negotiates card speed, and mounts the card.

**Parameters:**
- `slot` — SDMMC slot index (0 or 1)
- `clk_pin` — GPIO for clock
- `cmd_pin` — GPIO for command
- `d0_pin` — GPIO for data line 0
- `d1_pin` — GPIO for data line 1; set to `-1` for 1-bit mode
- `d2_pin` — GPIO for data line 2; set to `-1` for 1-bit mode
- `d3_pin` — GPIO for data line 3; set to `-1` for 1-bit mode
- `bus_width` — `1` for 1-bit mode, `4` for 4-bit mode
- `freq_hz` — bus frequency (e.g. 20000000 for 20 MHz)
- `out_sdmmc` — output handle

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid pins or zero block count, translated ESP-IDF error on card initialization failure, `BLUSYS_ERR_NOT_SUPPORTED` on ESP32-C3.

---

### `blusys_sdmmc_close`

```c
blusys_err_t blusys_sdmmc_close(blusys_sdmmc_t *sdmmc);
```

Unmounts the card and releases the handle.

---

### `blusys_sdmmc_read_blocks`

```c
blusys_err_t blusys_sdmmc_read_blocks(blusys_sdmmc_t *sdmmc,
                                      uint32_t start_block,
                                      void *buffer,
                                      uint32_t num_blocks,
                                      int timeout_ms);
```

Reads one or more 512-byte blocks from the card.

**Parameters:**
- `sdmmc` — handle
- `start_block` — logical block address of the first block to read
- `buffer` — destination buffer; must be at least `num_blocks * 512` bytes
- `num_blocks` — number of blocks to read; must be at least 1
- `timeout_ms` — milliseconds to wait for the operation

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT`, translated ESP-IDF error on card failure.

---

### `blusys_sdmmc_write_blocks`

```c
blusys_err_t blusys_sdmmc_write_blocks(blusys_sdmmc_t *sdmmc,
                                       uint32_t start_block,
                                       const void *buffer,
                                       uint32_t num_blocks,
                                       int timeout_ms);
```

Writes one or more 512-byte blocks to the card.

**Parameters:**
- `sdmmc` — handle
- `start_block` — logical block address of the first block to write
- `buffer` — source buffer; must be at least `num_blocks * 512` bytes
- `num_blocks` — number of blocks to write; must be at least 1
- `timeout_ms` — milliseconds to wait for the operation

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT`, translated ESP-IDF error on card failure.

## Lifecycle

1. `blusys_sdmmc_open()` — initialize slot and mount card
2. `blusys_sdmmc_read_blocks()` / `blusys_sdmmc_write_blocks()` — raw block access
3. `blusys_sdmmc_close()` — unmount and release

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

See `examples/sdmmc_basic/`.
