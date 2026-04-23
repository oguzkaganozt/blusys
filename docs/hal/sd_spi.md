# SD Card over SPI

FAT filesystem on an SD card connected over the SPI bus. Files and subdirectories are also reachable via standard C file APIs under the VFS base path.

## Quick Example

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

const char *msg = "hello from blusys sd_spi\n";
blusys_sd_spi_write(sd, "log.txt", msg, strlen(msg));

char buf[64] = {0};
size_t n = 0;
blusys_sd_spi_read(sd, "log.txt", buf, sizeof(buf) - 1, &n);
printf("read back: %s", buf);

blusys_sd_spi_mkdir(sd, "logs");
blusys_sd_spi_write(sd, "logs/boot.txt", "boot\n", 5);

blusys_sd_spi_listdir(sd, NULL, my_cb, NULL);
blusys_sd_spi_unmount(sd);
```

## Common Mistakes

- **no card inserted** — mount returns `BLUSYS_ERR_IO` with an underlying timeout or CRC error
- **wrong SPI host** — `SPI1_HOST` (value 0) is reserved for internal flash and is rejected with `BLUSYS_ERR_INVALID_ARG`; use `SPI2_HOST` (1) or `SPI3_HOST` (2)
- **SPI3_HOST on ESP32-C3** — the C3 only has one user SPI bus; passing `spi_host = 2` on a C3 fails at runtime
- **card not FAT32** — exFAT / NTFS cards will not mount. Reformat as FAT32, or set `format_if_mount_failed = true` (this erases all card contents)
- **nested mkdir** — `blusys_sd_spi_mkdir("a/b")` fails if `"a"` does not already exist; create each level with a separate call
- **calling back into the handle from listdir** — the mutex is held for the entire iteration; re-entering it from the callback deadlocks
- **CRC errors at high speed** — long wires and poor connections cause bit errors; reduce `freq_hz` (e.g. 4 MHz) for debugging

## Target Support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Thread Safety

- all operations on a handle are serialised with an internal mutex
- concurrent calls on the same handle are safe
- the `listdir` callback must not call back into the same handle — the mutex is already held

## ISR Notes

No ISR-safe calls are defined for the SD-over-SPI module.

## Limitations

- `SPI1_HOST` (value 0) is reserved for internal flash and is rejected with `BLUSYS_ERR_INVALID_ARG`
- `SPI3_HOST` (value 2) is only available on ESP32 and ESP32-S3; passing it on ESP32-C3 will fail at runtime
- `blusys_sd_spi_mkdir` creates only a single directory level; nested paths require sequential calls
- `format_if_mount_failed = true` erases all card contents — use only during development
- SPI bus speed is capped at 20 MHz for reliable SD communication over longer wires; reduce `freq_hz` if you observe CRC errors

## Example App

See `examples/validation/peripheral_lab/` (`periph_sd_spi` scenario).
