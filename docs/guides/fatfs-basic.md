# Read and Write Files with FAT on Flash

## Problem Statement

You want a filesystem on internal flash that supports subdirectories, long filenames, or higher-frequency writes than SPIFFS allows — for example, organising log files by date, storing per-session configuration blobs, or caching larger downloaded payloads.

## Prerequisites

- any supported board (ESP32, ESP32-C3, or ESP32-S3)
- a FAT partition in the project's partition table labelled `"ffat"` with subtype `fat` (see `examples/fatfs_basic/partitions.csv`)
- no external wiring required

## Minimal Example

```c
#include "blusys/blusys_services.h"

blusys_fatfs_t *fs = NULL;
blusys_fatfs_config_t config = {
    .base_path              = "/ffat",
    .partition_label        = NULL,   /* use default "ffat" partition */
    .max_files              = 5,
    .format_if_mount_failed = true,
    .allocation_unit_size   = 0,      /* use default (4096 bytes) */
};

blusys_fatfs_mount(&config, &fs);

/* Write a file */
const char *msg = "hello from blusys fatfs\n";
blusys_fatfs_write(fs, "log.txt", msg, strlen(msg));

/* Read it back */
char buf[64] = {0};
size_t n = 0;
blusys_fatfs_read(fs, "log.txt", buf, sizeof(buf) - 1, &n);
printf("read back: %s", buf);

/* Create a subdirectory and write a nested file */
blusys_fatfs_mkdir(fs, "logs");
blusys_fatfs_write(fs, "logs/boot.txt", "boot\n", 5);

/* List the root */
blusys_fatfs_listdir(fs, NULL, my_cb, NULL);

blusys_fatfs_unmount(fs);
```

## APIs Used

- `blusys_fatfs_mount()` — registers the FAT partition with the VFS layer and returns a handle
- `blusys_fatfs_unmount()` — flushes, closes, and unregisters the filesystem
- `blusys_fatfs_write()` — creates or overwrites a file with the given bytes
- `blusys_fatfs_read()` — reads bytes from a file into a caller-supplied buffer
- `blusys_fatfs_append()` — appends bytes to an existing file, or creates it
- `blusys_fatfs_remove()` — deletes a file
- `blusys_fatfs_exists()` — checks whether a path (file or directory) exists
- `blusys_fatfs_size()` — returns the byte size of a file; useful for sizing a read buffer
- `blusys_fatfs_mkdir()` — creates a single directory level (idempotent)
- `blusys_fatfs_listdir()` — iterates directory entries via a callback

## Expected Runtime Behavior

- on first boot with `format_if_mount_failed = true`, the partition is formatted automatically if it has never been initialised
- subdirectories created with `blusys_fatfs_mkdir` appear immediately under the mount point and are visible via `blusys_fatfs_listdir`
- `blusys_fatfs_listdir` with `path = NULL` or `""` lists the root; with a relative path it lists that subdirectory
- data written before `blusys_fatfs_unmount` is durable across power cycles
- after unmounting, the VFS path (e.g. `/ffat/log.txt`) is no longer accessible via `fopen()`

## Common Mistakes

- **wrong partition subtype** — the partition table entry must have subtype `fat`, not `spiffs`. Using the wrong subtype causes mount to fail with `BLUSYS_ERR_IO`.
- **partition label mismatch** — the label in `partitions.csv` must match `partition_label` in the config (or the module default `"ffat"`). A mismatch produces `BLUSYS_ERR_IO` with an underlying `ESP_ERR_NOT_FOUND`.
- **nested mkdir** — `blusys_fatfs_mkdir("a/b")` will fail if `"a"` does not already exist. Create each level with a separate call.
- **calling back into the handle from listdir** — the mutex is held for the entire iteration; re-entering it from the callback deadlocks.
- **LFN not enabled** — if `CONFIG_FATFS_LFN_NONE` is set in sdkconfig, filenames longer than 8.3 format are silently truncated. Enable `CONFIG_FATFS_LFN_HEAP` or `CONFIG_FATFS_LFN_STACK` via menuconfig.

## Example App

See `examples/fatfs_basic/` for a runnable example that demonstrates the full lifecycle: mount, write, exists check, size query, read, append, remove, mkdir, listdir, and unmount.

## API Reference

For full type definitions and function signatures, see [FAT Filesystem API Reference](../modules/fatfs.md).
