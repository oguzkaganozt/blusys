# NVS

## Purpose

The `nvs` module provides persistent key-value storage backed by ESP32 flash:

- open a named namespace in read-only or read-write mode
- read and write typed values (integers, strings, blobs)
- commit writes to flash when ready
- erase individual keys or the entire namespace

NVS data survives power cycles and soft resets. It is the standard mechanism for storing device configuration, calibration data, and runtime state on ESP32.

## Supported Targets

- ESP32
- ESP32-C3
- ESP32-S3

## Quick Start Example

```c
#include "blusys/nvs.h"

void app_main(void)
{
    blusys_nvs_t *nvs;

    blusys_nvs_open("app_config", BLUSYS_NVS_READWRITE, &nvs);

    blusys_nvs_set_u32(nvs, "device_id", 0xDEADBEEF);
    blusys_nvs_set_i32(nvs, "threshold", -10);
    blusys_nvs_commit(nvs);

    uint32_t id = 0;
    blusys_nvs_get_u32(nvs, "device_id", &id);

    blusys_nvs_close(nvs);
}
```

## Lifecycle Model

1. `blusys_nvs_open()` — initializes flash (if needed) and opens a namespace handle
2. call `get_*` / `set_*` as needed
3. `blusys_nvs_commit()` — flush writes to flash (required after any `set_*` call)
4. `blusys_nvs_close()` — release the handle

## Types

```c
typedef enum {
    BLUSYS_NVS_READONLY  = 0,
    BLUSYS_NVS_READWRITE = 1,
} blusys_nvs_mode_t;
```

## Functions

### `blusys_nvs_open`

```c
blusys_err_t blusys_nvs_open(const char *namespace_name,
                               blusys_nvs_mode_t mode,
                               blusys_nvs_t **out_nvs);
```

Opens a named NVS namespace. Initializes flash storage on first call.

| Parameter | Description |
|---|---|
| `namespace_name` | Namespace string, max 15 characters |
| `mode` | `BLUSYS_NVS_READONLY` or `BLUSYS_NVS_READWRITE` |
| `out_nvs` | Receives the allocated handle |

Returns `BLUSYS_OK` on success.

### `blusys_nvs_close`

```c
blusys_err_t blusys_nvs_close(blusys_nvs_t *nvs);
```

Closes the handle and frees all associated resources. Uncommitted writes are discarded.

### `blusys_nvs_commit`

```c
blusys_err_t blusys_nvs_commit(blusys_nvs_t *nvs);
```

Flushes pending writes to flash. Must be called after one or more `set_*` calls for data to persist. Returns `BLUSYS_ERR_INVALID_STATE` if the handle was opened `BLUSYS_NVS_READONLY`.

### `blusys_nvs_erase_key`

```c
blusys_err_t blusys_nvs_erase_key(blusys_nvs_t *nvs, const char *key);
```

Erases a single key from the namespace. Returns `BLUSYS_ERR_INVALID_STATE` for read-only handles.

### `blusys_nvs_erase_all`

```c
blusys_err_t blusys_nvs_erase_all(blusys_nvs_t *nvs);
```

Erases all keys in the namespace. Returns `BLUSYS_ERR_INVALID_STATE` for read-only handles.

### Integer getters

```c
blusys_err_t blusys_nvs_get_u8(blusys_nvs_t *nvs, const char *key, uint8_t *out_value);
blusys_err_t blusys_nvs_get_u32(blusys_nvs_t *nvs, const char *key, uint32_t *out_value);
blusys_err_t blusys_nvs_get_i32(blusys_nvs_t *nvs, const char *key, int32_t *out_value);
blusys_err_t blusys_nvs_get_u64(blusys_nvs_t *nvs, const char *key, uint64_t *out_value);
```

Reads a typed integer value. Returns `BLUSYS_ERR_IO` if the key does not exist.

### Integer setters

```c
blusys_err_t blusys_nvs_set_u8(blusys_nvs_t *nvs, const char *key, uint8_t value);
blusys_err_t blusys_nvs_set_u32(blusys_nvs_t *nvs, const char *key, uint32_t value);
blusys_err_t blusys_nvs_set_i32(blusys_nvs_t *nvs, const char *key, int32_t value);
blusys_err_t blusys_nvs_set_u64(blusys_nvs_t *nvs, const char *key, uint64_t value);
```

Writes a typed integer value. Call `blusys_nvs_commit()` afterward to persist. Returns `BLUSYS_ERR_INVALID_STATE` for read-only handles.

### `blusys_nvs_get_str` / `blusys_nvs_set_str`

```c
blusys_err_t blusys_nvs_get_str(blusys_nvs_t *nvs, const char *key,
                                  char *out_value, size_t *length);
blusys_err_t blusys_nvs_set_str(blusys_nvs_t *nvs, const char *key,
                                  const char *value);
```

`blusys_nvs_get_str()` uses a two-call pattern:

1. Call with `out_value = NULL` to get the required buffer size (including null terminator) in `*length`.
2. Allocate a buffer of that size and call again with `out_value` set.

```c
size_t len = 0;
blusys_nvs_get_str(nvs, "my_key", NULL, &len);
char *buf = malloc(len);
blusys_nvs_get_str(nvs, "my_key", buf, &len);
printf("%s\n", buf);
free(buf);
```

Returns `BLUSYS_ERR_IO` if the key does not exist.

### `blusys_nvs_get_blob` / `blusys_nvs_set_blob`

```c
blusys_err_t blusys_nvs_get_blob(blusys_nvs_t *nvs, const char *key,
                                   void *out_value, size_t *length);
blusys_err_t blusys_nvs_set_blob(blusys_nvs_t *nvs, const char *key,
                                   const void *value, size_t length);
```

Same two-call pattern as `blusys_nvs_get_str()`. Pass `out_value = NULL` to get the byte count first.

## Thread Safety

Concurrent calls on the same handle are serialized internally. Do not call `blusys_nvs_close()` concurrently with other calls using the same handle.

## Error Behavior

| Error | Cause |
|---|---|
| `BLUSYS_ERR_INVALID_ARG` | null pointer arguments |
| `BLUSYS_ERR_INVALID_STATE` | write/commit/erase on a read-only handle |
| `BLUSYS_ERR_IO` | key not found, or underlying NVS error |
| `BLUSYS_ERR_NO_MEM` | allocation failure in `open()` |

## NVS Flash Init Note

`blusys_nvs_open()` calls `nvs_flash_init()` internally. Do not call `nvs_flash_init()` separately in the same application — the call is idempotent when the partition already exists. If you use both WiFi and NVS, both modules call `nvs_flash_init()` safely; the second call is a no-op.

## Example App

See `examples/nvs_basic/`.
