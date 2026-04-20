# Filesystem

SPIFFS-backed filesystem for reading and writing files on the internal SPI flash. For hierarchical paths and wear-levelling, use [`fatfs`](fatfs.md) instead.

## Quick Example

```c
#include "blusys/blusys.h"

void app_main(void)
{
    blusys_fs_t *fs;
    blusys_fs_config_t cfg = {
        .base_path              = "/fs",
        .partition_label        = NULL,     /* default "spiffs" partition */
        .max_files              = 5,
        .format_if_mount_failed = false,
    };
    blusys_fs_mount(&cfg, &fs);

    const char *msg = "hello\n";
    blusys_fs_write(fs, "log.txt", msg, strlen(msg));

    char buf[64] = {0};
    size_t n = 0;
    blusys_fs_read(fs, "log.txt", buf, sizeof(buf) - 1, &n);
    printf("%s", buf);

    blusys_fs_unmount(fs);
}
```

## Common Mistakes

- **missing SPIFFS partition** — the app's partition table must include a SPIFFS partition (label `"spiffs"` by default); `mount()` returns `BLUSYS_ERR_IO` otherwise
- **using subdirectories** — SPIFFS has a flat namespace; `"logs/boot.txt"` is just a filename, not a directory
- **filename over 32 characters** — SPIFFS rejects longer names
- **high-frequency writes to the same file** — SPIFFS is not wear-levelled; use NVS for frequently updated key-value data

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Thread Safety

- all operations on a handle are serialised with an internal mutex
- concurrent calls on the same handle are safe
- concurrent calls on different handles backed by different partitions are also safe

## ISR Notes

No ISR-safe calls are defined for the filesystem module.

## Limitations

- SPIFFS does not support subdirectories; all files live in a flat namespace at the partition root
- maximum filename length is 32 characters (SPIFFS constraint)
- file paths passed to Blusys FS functions are limited to 63 characters
- SPIFFS is not wear-levelled; avoid high-frequency writes to the same file (use NVS instead)
- only one SPIFFS partition may be mounted at a time via this module; multiple partitions require direct ESP-IDF SPIFFS APIs

## Example App

See `examples/validation/platform_lab/` (`plat_storage` scenario, SPIFFS menuconfig).
