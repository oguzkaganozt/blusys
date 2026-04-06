# SD Card over SPI

FAT filesystem on an SD card connected over the SPI bus, supporting files and subdirectories.

!!! tip "Task Guide"
    For a step-by-step walkthrough, see [Read and Write Files on an SD Card via SPI](../guides/sd-spi-basic.md).

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_sd_spi_config_t`

```c
typedef struct {
    const char *base_path;              /* VFS mount point, e.g. "/sdcard" (required) */
    int         spi_host;               /* SPI bus: SPI2_HOST=1, SPI3_HOST=2 */
    int         mosi_pin;               /* MOSI GPIO pin */
    int         miso_pin;               /* MISO GPIO pin */
    int         sclk_pin;               /* Clock GPIO pin */
    int         cs_pin;                 /* Chip-select GPIO pin */
    uint32_t    freq_hz;                /* SPI clock in Hz; 0 = 20 MHz */
    size_t      max_files;              /* Max simultaneously open files; 0 = 5 */
    bool        format_if_mount_failed; /* Auto-format SD card if mount fails */
    size_t      allocation_unit_size;   /* FAT cluster size in bytes; 0 = 16384 */
} blusys_sd_spi_config_t;
```

### `blusys_sd_spi_t`

Opaque handle representing a mounted SD card filesystem. Obtained from `blusys_sd_spi_mount()` and released by `blusys_sd_spi_unmount()`.

### `blusys_sd_spi_listdir_cb_t`

```c
typedef void (*blusys_sd_spi_listdir_cb_t)(const char *name, void *user_data);
```

Callback type passed to `blusys_sd_spi_listdir()`. Called once per directory entry with the bare entry name (no path prefix) and the caller-supplied `user_data`.

## Functions

### `blusys_sd_spi_mount`

```c
blusys_err_t blusys_sd_spi_mount(const blusys_sd_spi_config_t *config,
                                  blusys_sd_spi_t **out_sd);
```

Initialises the SPI bus, probes the SD card, and registers the FAT filesystem with the ESP-IDF VFS layer, making files accessible via standard C file functions under `base_path`.

**Parameters:**

- `config` — mount configuration; `base_path`, `spi_host`, and all pin fields are required
- `out_sd` — receives the handle on success

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` if any pin or host is invalid, `BLUSYS_ERR_NO_MEM` if allocation fails, `BLUSYS_ERR_IO` if the card cannot be probed or mounted.

---

### `blusys_sd_spi_unmount`

```c
blusys_err_t blusys_sd_spi_unmount(blusys_sd_spi_t *sd);
```

Flushes all open files, unregisters the filesystem from the VFS layer, frees the SPI bus, and releases the handle. After this call the handle is invalid.

---

### `blusys_sd_spi_write`

```c
blusys_err_t blusys_sd_spi_write(blusys_sd_spi_t *sd, const char *path,
                                  const void *data, size_t size);
```

Creates or overwrites the file at `path` with `data`. The previous contents are discarded.

**Parameters:**

- `sd` — SD card handle
- `path` — relative path, e.g. `"log.txt"` or `"logs/boot.txt"`
- `data` — bytes to write
- `size` — number of bytes to write

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_IO` on write failure or if the parent directory does not exist.

---

### `blusys_sd_spi_read`

```c
blusys_err_t blusys_sd_spi_read(blusys_sd_spi_t *sd, const char *path,
                                 void *buf, size_t buf_size,
                                 size_t *out_bytes_read);
```

Reads up to `buf_size` bytes from the file at `path` into `buf`.

**Parameters:**

- `sd` — SD card handle
- `path` — relative path
- `buf` — destination buffer
- `buf_size` — capacity of `buf` in bytes
- `out_bytes_read` — receives the number of bytes actually read

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_IO` if the file does not exist or a read error occurred.

---

### `blusys_sd_spi_append`

```c
blusys_err_t blusys_sd_spi_append(blusys_sd_spi_t *sd, const char *path,
                                   const void *data, size_t size);
```

Appends `data` to the end of an existing file, or creates the file if it does not yet exist.

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_IO` on write failure.

---

### `blusys_sd_spi_remove`

```c
blusys_err_t blusys_sd_spi_remove(blusys_sd_spi_t *sd, const char *path);
```

Deletes the file at `path`.

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_IO` if the file does not exist.

---

### `blusys_sd_spi_exists`

```c
blusys_err_t blusys_sd_spi_exists(blusys_sd_spi_t *sd, const char *path,
                                   bool *out_exists);
```

Sets `*out_exists` to `true` if the path (file or directory) exists, `false` otherwise. Always returns `BLUSYS_OK` unless an argument is NULL.

---

### `blusys_sd_spi_size`

```c
blusys_err_t blusys_sd_spi_size(blusys_sd_spi_t *sd, const char *path,
                                 size_t *out_size);
```

Returns the size of the file in bytes.

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_IO` if the file does not exist.

---

### `blusys_sd_spi_mkdir`

```c
blusys_err_t blusys_sd_spi_mkdir(blusys_sd_spi_t *sd, const char *path);
```

Creates a single directory level. The call is idempotent — if the directory already exists it returns `BLUSYS_OK`. To create nested paths (`"a/b/c"`) call `mkdir` once per level.

**Returns:** `BLUSYS_OK` on success or if the directory already exists, `BLUSYS_ERR_IO` if the parent does not exist or creation fails.

---

### `blusys_sd_spi_listdir`

```c
blusys_err_t blusys_sd_spi_listdir(blusys_sd_spi_t *sd, const char *path,
                                    blusys_sd_spi_listdir_cb_t cb, void *user_data);
```

Iterates entries in a directory, invoking `cb` once per entry. The entries `.` and `..` are skipped automatically. The mutex is held for the entire iteration — the callback must not call back into the same `sd` handle.

**Parameters:**

- `sd` — SD card handle
- `path` — relative directory path; `NULL` or `""` lists the mount root
- `cb` — callback invoked once per entry with the bare name and `user_data`
- `user_data` — passed through to `cb` unchanged

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_IO` if the directory cannot be opened.

---

## Lifecycle

```
blusys_sd_spi_mount()
    ├─ blusys_sd_spi_mkdir()
    ├─ blusys_sd_spi_write() / blusys_sd_spi_read() / blusys_sd_spi_append()
    ├─ blusys_sd_spi_remove() / blusys_sd_spi_exists() / blusys_sd_spi_size()
    └─ blusys_sd_spi_listdir()
blusys_sd_spi_unmount()
```

A single `blusys_sd_spi_t` handle maps to one SD card mounted at one VFS path. Mounting the same SPI host twice without unmounting first is not supported.

## Thread Safety

All operations on a handle are serialised with an internal mutex. Concurrent calls on the same handle are safe.

## Limitations

- SPI1_HOST (value 0) is reserved for internal flash and is rejected with `BLUSYS_ERR_INVALID_ARG`.
- SPI3_HOST (value 2) is only available on ESP32 and ESP32-S3; passing it on ESP32-C3 will fail at runtime.
- `blusys_sd_spi_mkdir` creates only a single directory level. Nested paths require sequential calls.
- The `listdir` callback must not call back into the same `blusys_sd_spi_t` handle — the mutex is already held.
- `format_if_mount_failed = true` will erase all card contents; use only during development.
- SPI bus speed is capped at 20 MHz for reliable SD communication over longer wires. Reduce `freq_hz` if you observe CRC errors.
