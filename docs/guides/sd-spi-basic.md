# Read and Write Files on an SD Card via SPI

## Problem Statement

You want a removable storage medium — an SD card — that you can swap between the device and a PC. The SDMMC peripheral is not wired up or not available on your board, so you need to talk to the card over the standard SPI bus instead.

## Prerequisites

- any supported board (ESP32, ESP32-C3, or ESP32-S3)
- a microSD card formatted as FAT32 (most cards ship this way)
- an SD card breakout board or slot wired to four GPIO pins (MOSI, MISO, SCLK, CS)
- level shifting if your board runs at 3.3 V — most SD modules include it

## Wiring

| SD card pin | Signal | Default GPIO (ESP32-S3) |
|-------------|--------|------------------------|
| DI          | MOSI   | GPIO 11                |
| DO          | MISO   | GPIO 13                |
| CLK         | SCLK   | GPIO 12                |
| CS          | CS     | GPIO 10                |
| VCC         | 3.3 V  | —                      |
| GND         | GND    | —                      |

Set the actual GPIOs in menuconfig under **Blusys SD SPI Basic Example**.

## Minimal Example

```c
#include "blusys/blusys.h"

blusys_sd_spi_t *sd = NULL;
blusys_sd_spi_config_t config = {
    .base_path              = "/sdcard",
    .spi_host               = 1,     /* SPI2_HOST */
    .mosi_pin               = 11,
    .miso_pin               = 13,
    .sclk_pin               = 12,
    .cs_pin                 = 10,
    .freq_hz                = 0,     /* use default (20 MHz) */
    .max_files              = 5,
    .format_if_mount_failed = false,
    .allocation_unit_size   = 0,     /* use default (16384 bytes) */
};

blusys_sd_spi_mount(&config, &sd);

/* Write a file */
const char *msg = "hello from blusys sd_spi\n";
blusys_sd_spi_write(sd, "log.txt", msg, strlen(msg));

/* Read it back */
char buf[64] = {0};
size_t n = 0;
blusys_sd_spi_read(sd, "log.txt", buf, sizeof(buf) - 1, &n);
printf("read back: %s", buf);

/* Create a subdirectory and a nested file */
blusys_sd_spi_mkdir(sd, "logs");
blusys_sd_spi_write(sd, "logs/boot.txt", "boot\n", 5);

/* List the root */
blusys_sd_spi_listdir(sd, NULL, my_cb, NULL);

blusys_sd_spi_unmount(sd);
```

## APIs Used

- `blusys_sd_spi_mount()` — initialises the SPI bus, probes the card, and registers the FAT filesystem with the VFS layer
- `blusys_sd_spi_unmount()` — flushes, unregisters, and frees the SPI bus
- `blusys_sd_spi_write()` — creates or overwrites a file
- `blusys_sd_spi_read()` — reads bytes into a caller-supplied buffer
- `blusys_sd_spi_append()` — appends to an existing file, or creates it
- `blusys_sd_spi_remove()` — deletes a file
- `blusys_sd_spi_exists()` — checks whether a path (file or directory) exists
- `blusys_sd_spi_size()` — returns the byte size of a file
- `blusys_sd_spi_mkdir()` — creates a single directory level (idempotent)
- `blusys_sd_spi_listdir()` — iterates directory entries via a callback

## Expected Runtime Behavior

- on a freshly formatted card the mount succeeds immediately and the root directory is empty
- files written before `blusys_sd_spi_unmount` are durable across power cycles and readable on a PC
- `blusys_sd_spi_listdir` with `path = NULL` or `""` lists the root; with a relative path it lists that subdirectory
- after unmounting, the VFS path (e.g. `/sdcard/log.txt`) is no longer accessible via `fopen()`

## Common Mistakes

- **no card inserted** — mount returns `BLUSYS_ERR_IO` with an underlying timeout or CRC error. Ensure the card is seated and the wiring is correct.
- **wrong SPI host** — SPI1_HOST (value 0) is reserved for internal flash and will be rejected with `BLUSYS_ERR_INVALID_ARG`. Use SPI2_HOST (1) or SPI3_HOST (2).
- **SPI3_HOST on ESP32-C3** — the C3 only has one user SPI bus (SPI2). Passing `spi_host = 2` on a C3 will fail at runtime.
- **card not FAT32** — cards formatted as exFAT or NTFS will not mount. Reformat the card as FAT32 on a PC, or set `format_if_mount_failed = true` (this erases all card contents).
- **nested mkdir** — `blusys_sd_spi_mkdir("a/b")` will fail if `"a"` does not already exist. Create each level with a separate call.
- **calling back into the handle from listdir** — the mutex is held for the entire iteration; re-entering it from the callback deadlocks.
- **CRC errors at high speed** — long wires and poor connections cause bit errors. Reduce `freq_hz` to 4000000 (4 MHz) for debugging.

## Example App

See `examples/sd_spi_basic/` for a runnable example that demonstrates the full lifecycle: mount, write, exists check, size query, read, append, remove, mkdir, listdir, and unmount.

## API Reference

For full type definitions and function signatures, see [SD Card over SPI API Reference](../modules/sd_spi.md).
