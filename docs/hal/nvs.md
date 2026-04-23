# NVS

Persistent key-value storage backed by ESP32 flash. NVS data survives power cycles and soft resets — the standard mechanism for device configuration, calibration data, and runtime state.

## Quick Example

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

Strings and blobs use a two-call pattern — query the size first, then read into a sized buffer:

```c
size_t len = 0;
blusys_nvs_get_str(nvs, "my_key", NULL, &len);
char *buf = malloc(len);
blusys_nvs_get_str(nvs, "my_key", buf, &len);
printf("%s\n", buf);
free(buf);
```

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

**Allocating a fixed buffer for strings or blobs**

Do not guess the length. Always use the two-call pattern to determine the exact required size before allocating.

**Namespace name too long**

NVS namespace names are limited to 15 characters. Longer names cause `blusys_nvs_open()` to fail.

## Target Support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## NVS Flash Init Note

`blusys_nvs_open()` calls `nvs_flash_init()` internally. Do not call `nvs_flash_init()` separately in the same application — the call is idempotent when the partition already exists. If you use both WiFi and NVS, both modules call `nvs_flash_init()` safely; the second call is a no-op.

## Thread Safety

- concurrent calls on the same handle are serialized internally
- do not call `blusys_nvs_close()` concurrently with other calls on the same handle

## ISR Notes

No ISR-safe calls are defined for the NVS module.

## Limitations

- namespace names are limited to 15 characters
- data is only persisted after an explicit `commit()`
- read-only handles return `BLUSYS_ERR_INVALID_STATE` for any write, erase, or commit attempt

## Example App

See `examples/reference/hal` (NVS scenario).
