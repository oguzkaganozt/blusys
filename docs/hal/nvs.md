# NVS

Persistent key-value storage backed by flash.

## At a glance

- data survives power cycles
- writes are only persistent after `commit()`
- read-only handles reject writes

## Quick example

```c
blusys_nvs_t *nvs;
blusys_nvs_open("app_config", BLUSYS_NVS_READWRITE, &nvs);
blusys_nvs_set_u32(nvs, "device_id", 0xDEADBEEF);
blusys_nvs_commit(nvs);
blusys_nvs_close(nvs);
```

## Common mistakes

- forgetting to commit after writes
- reading a key that was never written
- allocating fixed buffers for strings and blobs
- using namespace names longer than 15 characters

## Target support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Thread safety

- calls on the same handle are serialised internally
- close must not overlap with other calls on the same handle

## Limitations

- data is only persistent after `commit()`
- namespace names are limited to 15 characters

## Example app

`examples/reference/hal`
