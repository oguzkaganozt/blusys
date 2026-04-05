# Store and Retrieve Configuration with NVS

## Problem Statement

You want to persist configuration values across power cycles — device IDs, thresholds, calibration offsets, or small strings — using the ESP32's on-chip flash storage.

## Prerequisites

- a supported board (ESP32, ESP32-C3, or ESP32-S3)
- a partition table that includes an `nvs` partition (the default ESP-IDF partition table includes one)

## Minimal Example

Write a value, commit it, then read it back:

```c
#include "blusys/blusys.h"

blusys_nvs_t *nvs;

blusys_nvs_open("app_config", BLUSYS_NVS_READWRITE, &nvs);

blusys_nvs_set_u32(nvs, "device_id", 0xDEADBEEF);
blusys_nvs_commit(nvs);       /* required — data is not persistent until committed */

uint32_t id = 0;
blusys_nvs_get_u32(nvs, "device_id", &id);
printf("device_id: 0x%08lX\n", (unsigned long)id);

blusys_nvs_close(nvs);
```

## APIs Used

- `blusys_nvs_open()` — initializes flash and opens a named namespace
- `blusys_nvs_set_*()` — write a typed value into the namespace
- `blusys_nvs_commit()` — flush pending writes to flash
- `blusys_nvs_get_*()` — read a value back
- `blusys_nvs_close()` — release the handle

## Two-Call String Pattern

Reading a string requires two calls — one to get the required buffer length, one to read the data:

```c
size_t len = 0;
blusys_nvs_get_str(nvs, "fw_tag", NULL, &len);   /* len now holds required size */
char *tag = malloc(len);
blusys_nvs_get_str(nvs, "fw_tag", tag, &len);    /* fill the buffer */
printf("fw_tag: %s\n", tag);
free(tag);
```

Blobs work identically — pass `NULL` first to get the byte count, then allocate and call again.

## Read-Only Namespace

Open with `BLUSYS_NVS_READONLY` to prevent accidental writes:

```c
blusys_nvs_t *nvs;
blusys_nvs_open("app_config", BLUSYS_NVS_READONLY, &nvs);

uint32_t id = 0;
blusys_nvs_get_u32(nvs, "device_id", &id);

/* Any set_*, commit, or erase call returns BLUSYS_ERR_INVALID_STATE */

blusys_nvs_close(nvs);
```

## NVS Flash Init Note

`blusys_nvs_open()` calls `nvs_flash_init()` internally. Do not call `nvs_flash_init()` separately — it is safe to call multiple times (the second call is a no-op), but there is no need. If your application also uses WiFi, both modules call `nvs_flash_init()` safely.

## Common Mistakes

**Forgetting to commit after writing**

`set_*` stages the write in RAM. Without `commit()`, the data is lost on the next reboot.

```c
blusys_nvs_set_u32(nvs, "count", 5);
/* missing: blusys_nvs_commit(nvs); */
blusys_nvs_close(nvs);   /* data is NOT persisted */
```

**Reading a key that was never written**

`get_*` returns `BLUSYS_ERR_IO` if the key does not exist yet. Always check the return value on first boot or after an erase:

```c
uint32_t count = 0;
if (blusys_nvs_get_u32(nvs, "count", &count) != BLUSYS_OK) {
    count = 0;   /* key not found — use default */
}
```

**Allocating a fixed buffer for strings**

Do not guess the string length. Always use the two-call pattern to determine the exact required size before allocating.

**Namespace name too long**

NVS namespace names are limited to 15 characters. Longer names will cause `blusys_nvs_open()` to fail.

## Example App

See `examples/nvs_basic/` for a runnable example that writes integers and a string, commits, reads back, and demonstrates the two-call string pattern.
