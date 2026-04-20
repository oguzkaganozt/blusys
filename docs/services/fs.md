# Filesystem

SPIFFS-backed filesystem for reading and writing files on the internal SPI flash.


## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_fs_config_t`

```c
typedef struct {
    const char *base_path;        /* VFS mount point, e.g. "/fs" (required) */
    const char *partition_label;  /* Flash partition label; NULL = default "spiffs" partition */
    size_t      max_files;        /* Max simultaneously open files; 0 = use default (5) */
    bool        format_if_mount_failed; /* Auto-format partition when mount fails */
} blusys_fs_config_t;
```

### `blusys_fs_t`

Opaque handle representing a mounted filesystem. Obtained from `blusys_fs_mount()` and released by `blusys_fs_unmount()`.

## Functions

### `blusys_fs_mount`

```c
blusys_err_t blusys_fs_mount(const blusys_fs_config_t *config, blusys_fs_t **out_fs);
```

Registers the SPIFFS partition with the ESP-IDF VFS layer and makes the filesystem accessible via standard C file functions. The application's partition table must contain a SPIFFS partition (label `"spiffs"` by default).

**Parameters:**
- `config` — mount configuration; `base_path` is required
- `out_fs` — receives the handle on success

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_NO_MEM` if allocation fails, `BLUSYS_ERR_IO` if the partition cannot be mounted and `format_if_mount_failed` is false.

---

### `blusys_fs_unmount`

```c
blusys_err_t blusys_fs_unmount(blusys_fs_t *fs);
```

Unregisters the SPIFFS partition from the VFS layer and frees the handle. After this call the handle is invalid and all open files are flushed and closed.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `fs` is NULL.

---

### `blusys_fs_write`

```c
blusys_err_t blusys_fs_write(blusys_fs_t *fs, const char *path, const void *data, size_t size);
```

Creates or overwrites the file at `path` with `data`. The previous contents are discarded.

**Parameters:**
- `fs` — filesystem handle
- `path` — relative filename, e.g. `"log.txt"` (max 63 characters)
- `data` — bytes to write
- `size` — number of bytes to write

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_IO` on write failure.

---

### `blusys_fs_read`

```c
blusys_err_t blusys_fs_read(blusys_fs_t *fs, const char *path, void *buf, size_t buf_size,
                             size_t *out_bytes_read);
```

Reads up to `buf_size` bytes from the file at `path` into `buf`. Call `blusys_fs_size()` first if you need to know the exact file size before allocating a buffer.

**Parameters:**
- `fs` — filesystem handle
- `path` — relative filename
- `buf` — destination buffer
- `buf_size` — capacity of `buf` in bytes
- `out_bytes_read` — receives the number of bytes actually read

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_IO` if the file does not exist or a read error occurred.

---

### `blusys_fs_append`

```c
blusys_err_t blusys_fs_append(blusys_fs_t *fs, const char *path, const void *data, size_t size);
```

Appends `data` to the end of an existing file, or creates the file if it does not yet exist.

**Parameters:**
- `fs` — filesystem handle
- `path` — relative filename
- `data` — bytes to append
- `size` — number of bytes to append

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_IO` on write failure.

---

### `blusys_fs_remove`

```c
blusys_err_t blusys_fs_remove(blusys_fs_t *fs, const char *path);
```

Deletes the file at `path`.

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_IO` if the file does not exist.

---

### `blusys_fs_exists`

```c
blusys_err_t blusys_fs_exists(blusys_fs_t *fs, const char *path, bool *out_exists);
```

Sets `*out_exists` to `true` if the file exists, `false` otherwise. Always returns `BLUSYS_OK` unless an argument is NULL.

---

### `blusys_fs_size`

```c
blusys_err_t blusys_fs_size(blusys_fs_t *fs, const char *path, size_t *out_size);
```

Returns the size of the file in bytes.

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_IO` if the file does not exist.

---

## Lifecycle

```
blusys_fs_mount()
    └─ blusys_fs_write() / blusys_fs_read() / blusys_fs_append()
       blusys_fs_remove() / blusys_fs_exists() / blusys_fs_size()
blusys_fs_unmount()
```

A single `blusys_fs_t` handle maps to one SPIFFS partition. Mounting the same partition twice is not supported and will return an error.

## Thread Safety

All operations on a handle are serialised with an internal mutex. Concurrent calls on the same handle are safe. Concurrent calls on different handles backed by different partitions are also safe.

## Limitations

- SPIFFS does not support subdirectories. All files live in a flat namespace at the partition root.
- Maximum filename length is 32 characters (SPIFFS constraint).
- File paths passed to blusys_fs functions are limited to 63 characters.
- SPIFFS is not wear-levelled; avoid high-frequency writes to the same file. Use the `nvs` module for frequently updated key-value data.
- Only one SPIFFS partition may be mounted at a time via this module. Multiple partitions require direct use of ESP-IDF SPIFFS APIs.

## Example App

See `examples/validation/platform_lab/` (`plat_storage` scenario — menuconfig: SPIFFS).
