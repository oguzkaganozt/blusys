# Read and Write Files on Flash

## Problem Statement

You want to persist data across reboots without using NVS key-value storage — for example, writing a log file, storing a configuration blob, or caching downloaded content on the internal flash.

## Prerequisites

- any supported board (ESP32, ESP32-C3, or ESP32-S3)
- a SPIFFS partition in the project's partition table (the default ESP-IDF partition table includes one labelled `"spiffs"`)
- no external wiring required

## Minimal Example

```c
#include "blusys/blusys.h"

blusys_fs_t *fs = NULL;
blusys_fs_config_t config = {
    .base_path              = "/fs",
    .partition_label        = NULL,   /* use default "spiffs" partition */
    .max_files              = 5,
    .format_if_mount_failed = true,
};

blusys_fs_mount(&config, &fs);

const char *msg = "hello from blusys\n";
blusys_fs_write(fs, "log.txt", msg, strlen(msg));

char buf[64] = {0};
size_t n = 0;
blusys_fs_read(fs, "log.txt", buf, sizeof(buf) - 1, &n);
printf("read back: %s", buf);

blusys_fs_unmount(fs);
```

## APIs Used

- `blusys_fs_mount()` — registers the SPIFFS partition with the VFS and returns a handle
- `blusys_fs_write()` — creates or overwrites a file with the given bytes
- `blusys_fs_read()` — reads bytes from a file into a caller-supplied buffer
- `blusys_fs_append()` — appends bytes to an existing file, or creates it
- `blusys_fs_remove()` — deletes a file
- `blusys_fs_exists()` — checks whether a file is present without opening it
- `blusys_fs_size()` — returns the byte size of a file; useful for sizing a read buffer
- `blusys_fs_unmount()` — flushes, closes, and unregisters the filesystem

## Expected Runtime Behavior

- on first boot with `format_if_mount_failed = true`, the partition is formatted automatically if it has never been initialised
- write operations overwrite the entire file; use `blusys_fs_append()` to add to an existing file
- data written before `blusys_fs_unmount()` is durable across soft resets and power cycles
- after unmounting, the VFS path (e.g. `/fs/log.txt`) is no longer accessible via `fopen()`

## Common Mistakes

- **missing SPIFFS partition** — if no `"spiffs"` partition exists in the partition table, `blusys_fs_mount()` returns `BLUSYS_ERR_IO`. Use a custom `partitions.csv` or set `format_if_mount_failed = true` to detect this early.
- **subdirectory paths** — SPIFFS has a flat namespace; paths like `"subdir/file.txt"` are treated as a single filename containing a slash, which may produce unexpected results.
- **long filenames** — SPIFFS limits filenames to 32 characters. Longer names silently truncate and may collide.
- **reading without knowing the size** — allocate a buffer sized with `blusys_fs_size()` before calling `blusys_fs_read()` to avoid truncation.
- **high-frequency writes** — SPIFFS is not wear-levelled. For frequently updated values use the `nvs` module instead.

## Example App

See `examples/fs_basic/` for a runnable example that demonstrates the full lifecycle: mount, write, exists check, size query, read, append, remove, and unmount.

## API Reference

For full type definitions and function signatures, see [Filesystem API Reference](../modules/fs.md).
