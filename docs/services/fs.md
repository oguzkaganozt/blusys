# Filesystem

SPIFFS-backed filesystem for simple flash storage. Use FATFS when you need directories or wear levelling.

## At a glance

- mount before file access
- SPIFFS is flat, not hierarchical
- data is not wear-levelled like FATFS

## Quick example

```c
blusys_fs_t *fs;
blusys_fs_config_t cfg = {
    .base_path = "/fs",
    .partition_label = NULL,
    .max_files = 5,
};

blusys_fs_mount(&cfg, &fs);
blusys_fs_write(fs, "log.txt", "hello\n", 6);
blusys_fs_unmount(fs);
```

## Common mistakes

- missing the SPIFFS partition
- treating SPIFFS paths as directories
- using long filenames
- writing the same file at high frequency

## Target support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Thread safety

- operations on a handle are serialised internally
- concurrent calls on different handles are safe

## Limitations

- flat namespace only
- 32-character filename limit
- only one SPIFFS partition through this module at a time

## Example app

`examples/validation/platform_lab/` (`plat_storage`)
