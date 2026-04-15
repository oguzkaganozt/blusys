# FAT Filesystem

FAT filesystem on internal SPI flash with hardware wear-levelling, supporting files and subdirectories.


## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_fatfs_config_t`

```c
typedef struct {
    const char *base_path;              /* VFS mount point, e.g. "/ffat" (required) */
    const char *partition_label;        /* Flash partition label; NULL = "ffat" */
    size_t      max_files;              /* Max simultaneously open files; 0 = 5 */
    bool        format_if_mount_failed; /* Auto-format partition when mount fails */
    size_t      allocation_unit_size;   /* FAT cluster size in bytes; 0 = default (4096) */
} blusys_fatfs_config_t;
```

### `blusys_fatfs_t`

Opaque handle representing a mounted FAT filesystem. Obtained from `blusys_fatfs_mount()` and released by `blusys_fatfs_unmount()`.

### `blusys_fatfs_listdir_cb_t`

```c
typedef void (*blusys_fatfs_listdir_cb_t)(const char *name, void *user_data);
```

Callback type passed to `blusys_fatfs_listdir()`. Called once per directory entry with the bare entry name (no path prefix) and the caller-supplied `user_data`.

## Functions

### `blusys_fatfs_mount`

```c
blusys_err_t blusys_fatfs_mount(const blusys_fatfs_config_t *config,
                                blusys_fatfs_t **out_fatfs);
```

Registers a FAT partition with the ESP-IDF VFS layer using wear-levelling, making the filesystem accessible via standard C file functions. The partition table must contain a `data`/`fat` partition (label `"ffat"` by default).

**Parameters:**

- `config` — mount configuration; `base_path` is required
- `out_fatfs` — receives the handle on success

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_NO_MEM` if allocation fails, `BLUSYS_ERR_IO` if the partition cannot be found or mounted and `format_if_mount_failed` is false.

---

### `blusys_fatfs_unmount`

```c
blusys_err_t blusys_fatfs_unmount(blusys_fatfs_t *fatfs);
```

Flushes all open files, unregisters the partition from the VFS layer, releases the wear-levelling handle, and frees the handle. After this call the handle is invalid.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `fatfs` is NULL.

---

### `blusys_fatfs_write`

```c
blusys_err_t blusys_fatfs_write(blusys_fatfs_t *fatfs, const char *path,
                                const void *data, size_t size);
```

Creates or overwrites the file at `path` with `data`. The previous contents are discarded.

**Parameters:**

- `fatfs` — filesystem handle
- `path` — relative path, e.g. `"log.txt"` or `"logs/boot.txt"`
- `data` — bytes to write
- `size` — number of bytes to write

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_IO` on write failure or if the parent directory does not exist.

---

### `blusys_fatfs_read`

```c
blusys_err_t blusys_fatfs_read(blusys_fatfs_t *fatfs, const char *path,
                               void *buf, size_t buf_size,
                               size_t *out_bytes_read);
```

Reads up to `buf_size` bytes from the file at `path` into `buf`. Call `blusys_fatfs_size()` first if you need the exact size before allocating a buffer.

**Parameters:**

- `fatfs` — filesystem handle
- `path` — relative path
- `buf` — destination buffer
- `buf_size` — capacity of `buf` in bytes
- `out_bytes_read` — receives the number of bytes actually read

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_IO` if the file does not exist or a read error occurred.

---

### `blusys_fatfs_append`

```c
blusys_err_t blusys_fatfs_append(blusys_fatfs_t *fatfs, const char *path,
                                 const void *data, size_t size);
```

Appends `data` to the end of an existing file, or creates the file if it does not yet exist.

**Parameters:**

- `fatfs` — filesystem handle
- `path` — relative path
- `data` — bytes to append
- `size` — number of bytes to append

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_IO` on write failure.

---

### `blusys_fatfs_remove`

```c
blusys_err_t blusys_fatfs_remove(blusys_fatfs_t *fatfs, const char *path);
```

Deletes the file at `path`.

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_IO` if the file does not exist.

---

### `blusys_fatfs_exists`

```c
blusys_err_t blusys_fatfs_exists(blusys_fatfs_t *fatfs, const char *path,
                                 bool *out_exists);
```

Sets `*out_exists` to `true` if the path (file or directory) exists, `false` otherwise. Always returns `BLUSYS_OK` unless an argument is NULL.

---

### `blusys_fatfs_size`

```c
blusys_err_t blusys_fatfs_size(blusys_fatfs_t *fatfs, const char *path,
                               size_t *out_size);
```

Returns the size of the file in bytes.

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_IO` if the file does not exist.

---

### `blusys_fatfs_mkdir`

```c
blusys_err_t blusys_fatfs_mkdir(blusys_fatfs_t *fatfs, const char *path);
```

Creates a single directory level. The call is idempotent — if the directory already exists it returns `BLUSYS_OK`. To create nested paths (`"a/b/c"`) call `mkdir` once per level.

**Parameters:**

- `fatfs` — filesystem handle
- `path` — relative directory path, e.g. `"logs"`

**Returns:** `BLUSYS_OK` on success or if the directory already exists, `BLUSYS_ERR_IO` if the parent does not exist or creation fails.

---

### `blusys_fatfs_listdir`

```c
blusys_err_t blusys_fatfs_listdir(blusys_fatfs_t *fatfs, const char *path,
                                  blusys_fatfs_listdir_cb_t cb, void *user_data);
```

Iterates entries in a directory, invoking `cb` once per entry. The entries `.` and `..` are skipped automatically. The mutex is held for the entire iteration — the callback must not call back into the same `fatfs` handle.

**Parameters:**

- `fatfs` — filesystem handle
- `path` — relative directory path; `NULL` or `""` lists the mount root
- `cb` — callback invoked once per entry with the bare name and `user_data`
- `user_data` — passed through to `cb` unchanged

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_IO` if the directory cannot be opened.

---

## Lifecycle

```
blusys_fatfs_mount()
    ├─ blusys_fatfs_mkdir()
    ├─ blusys_fatfs_write() / blusys_fatfs_read() / blusys_fatfs_append()
    ├─ blusys_fatfs_remove() / blusys_fatfs_exists() / blusys_fatfs_size()
    └─ blusys_fatfs_listdir()
blusys_fatfs_unmount()
```

A single `blusys_fatfs_t` handle maps to one FAT partition. Mounting the same partition twice is not supported and will return an error.

## Thread Safety

All operations on a handle are serialised with an internal mutex. Concurrent calls on the same handle are safe. Concurrent calls on different handles backed by different partitions are also safe.

## Limitations

- Partition table must include a `data`/`fat` partition labelled `"ffat"` (or the label set in config).
- `blusys_fatfs_mkdir` creates only a single directory level. Nested paths require sequential calls.
- The `listdir` callback must not call back into the same `blusys_fatfs_t` handle — the mutex is already held.
- If `CONFIG_FATFS_LFN_NONE` is set in sdkconfig, filenames longer than 8.3 format are silently truncated. Enable `CONFIG_FATFS_LFN_HEAP` or `CONFIG_FATFS_LFN_STACK` via menuconfig to use long filenames.
- Maximum simultaneously open files is bounded by `max_files` in the config (default 5).

## Example App

See `examples/validation/storage_basic/` (menuconfig: FAT).
