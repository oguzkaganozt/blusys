# FAT Filesystem

FAT filesystem on internal SPI flash with hardware wear-levelling, supporting files and subdirectories.

## Quick Example

```c
#include "blusys/blusys.h"

void app_main(void)
{
    blusys_fatfs_t *fs;
    blusys_fatfs_config_t cfg = {
        .base_path              = "/ffat",
        .partition_label        = NULL,     /* default "ffat" partition */
        .max_files              = 5,
        .format_if_mount_failed = false,
        .allocation_unit_size   = 0,        /* default 4096 */
    };
    blusys_fatfs_mount(&cfg, &fs);

    blusys_fatfs_mkdir(fs, "logs");
    const char *msg = "boot\n";
    blusys_fatfs_write(fs, "logs/boot.txt", msg, strlen(msg));

    blusys_fatfs_listdir(fs, "logs", my_cb, NULL);
    blusys_fatfs_unmount(fs);
}
```

Files are also reachable via standard C file APIs under the base path, e.g. `fopen("/ffat/logs/boot.txt", "r")`.

## Common Mistakes

- **missing FAT partition** — the partition table must include a `data`/`fat` partition labelled `"ffat"` (or the label set in config); `mount()` returns `BLUSYS_ERR_IO` otherwise
- **nested `mkdir`** — `blusys_fatfs_mkdir("a/b")` fails if `"a"` does not exist; create each level with a separate call
- **calling back into the handle from `listdir`** — the mutex is held for the entire iteration; re-entering from the callback deadlocks
- **`CONFIG_FATFS_LFN_NONE` in sdkconfig** — filenames longer than 8.3 format are silently truncated; enable `CONFIG_FATFS_LFN_HEAP` or `CONFIG_FATFS_LFN_STACK` for long filenames

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
- the `listdir` callback must not call back into the same handle — the mutex is already held

## ISR Notes

No ISR-safe calls are defined for the FATFS module.

## Limitations

- the partition table must include a `data`/`fat` partition labelled `"ffat"` (or the label set in config)
- `blusys_fatfs_mkdir` creates only a single directory level; nested paths require sequential calls
- maximum simultaneously open files is bounded by `max_files` (default 5)

## Example App

See `examples/validation/platform_lab/` (`plat_storage` scenario, FAT menuconfig).
