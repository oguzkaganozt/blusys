# mDNS

Zero-configuration networking: advertise services and discover other devices on the local network using multicast DNS.

!!! tip "Task Guide"
    For a step-by-step walkthrough, see [mDNS Basics](../guides/mdns-basic.md).

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Types

### `blusys_mdns_proto_t`

```c
typedef enum {
    BLUSYS_MDNS_PROTO_TCP = 0,
    BLUSYS_MDNS_PROTO_UDP,
} blusys_mdns_proto_t;
```

### `blusys_mdns_txt_item_t`

```c
typedef struct {
    const char *key;
    const char *value;
} blusys_mdns_txt_item_t;
```

Key-value pair for DNS-SD TXT records.

### `blusys_mdns_config_t`

```c
typedef struct {
    const char *hostname;       /* e.g. "my-esp32" -> my-esp32.local */
    const char *instance_name;  /* Friendly name (e.g. "ESP32 Sensor Node") */
} blusys_mdns_config_t;
```

### `blusys_mdns_result_t`

```c
typedef struct {
    char instance_name[64];
    char hostname[64];
    char ip[16];       /* IPv4 address string */
    uint16_t port;
} blusys_mdns_result_t;
```

Flat result structure returned by `blusys_mdns_query()`. All strings are copied — no heap pointers to free.

## Functions

### `blusys_mdns_open`

```c
blusys_err_t blusys_mdns_open(const blusys_mdns_config_t *config,
                               blusys_mdns_t **out_mdns);
```

Initializes the mDNS responder with the given hostname. After this call, the device is reachable at `<hostname>.local`. WiFi must already be connected.

**Parameters:**

- `config` — hostname (required) and optional instance name
- `out_mdns` — output handle

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_NO_MEM` if allocation fails.

---

### `blusys_mdns_close`

```c
blusys_err_t blusys_mdns_close(blusys_mdns_t *mdns);
```

Stops the mDNS responder, removes all advertised services, and frees the handle.

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `mdns` is NULL.

---

### `blusys_mdns_add_service`

```c
blusys_err_t blusys_mdns_add_service(blusys_mdns_t *mdns,
                                      const char *instance_name,
                                      const char *service_type,
                                      blusys_mdns_proto_t proto,
                                      uint16_t port,
                                      const blusys_mdns_txt_item_t *txt,
                                      size_t txt_count);
```

Advertises a service on the local network via DNS-SD.

**Parameters:**

- `instance_name` — friendly name for this service instance (e.g. "Living Room Sensor"), or NULL
- `service_type` — service type string (e.g. `"_http"`, `"_mqtt"`)
- `proto` — `BLUSYS_MDNS_PROTO_TCP` or `BLUSYS_MDNS_PROTO_UDP`
- `port` — port number the service listens on
- `txt` — array of TXT record key-value pairs, or NULL
- `txt_count` — number of TXT items

**Returns:** `BLUSYS_OK` on success.

---

### `blusys_mdns_remove_service`

```c
blusys_err_t blusys_mdns_remove_service(blusys_mdns_t *mdns,
                                         const char *service_type,
                                         blusys_mdns_proto_t proto);
```

Removes a previously advertised service.

**Parameters:**

- `mdns` — handle
- `service_type` — service type string (e.g. `"_http"`, `"_mqtt"`)
- `proto` — `BLUSYS_MDNS_PROTO_TCP` or `BLUSYS_MDNS_PROTO_UDP`

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` if `mdns` or `service_type` is NULL.

---

### `blusys_mdns_query`

```c
blusys_err_t blusys_mdns_query(blusys_mdns_t *mdns,
                                const char *service_type,
                                blusys_mdns_proto_t proto,
                                int timeout_ms,
                                blusys_mdns_result_t *results,
                                size_t max_results,
                                size_t *out_count);
```

Discovers services of the given type on the local network. Blocks until at least one result is found or the timeout expires.

**Parameters:**

- `service_type` — service type to search for (e.g. `"_http"`)
- `proto` — `BLUSYS_MDNS_PROTO_TCP` or `BLUSYS_MDNS_PROTO_UDP`
- `timeout_ms` — maximum wait time in milliseconds; `BLUSYS_TIMEOUT_FOREVER` (`-1`) uses a 5 s default
- `results` — caller-provided array to receive results
- `max_results` — capacity of the results array
- `out_count` — number of results found

**Returns:** `BLUSYS_OK` on success (even if `*out_count` is 0).

## Lifecycle

1. `blusys_mdns_open()` — start responder
2. `blusys_mdns_add_service()` — advertise services (optional, repeatable)
3. `blusys_mdns_query()` — discover peers (optional)
4. `blusys_mdns_remove_service()` — stop advertising a service (optional)
5. `blusys_mdns_close()` — stop responder and free resources

## Thread Safety

All functions are serialized with an internal mutex. Concurrent calls on the same handle are safe.

## Limitations

- WiFi must be connected before calling `blusys_mdns_open()`.
- This module requires the `espressif/mdns` managed component. Projects using mDNS must declare it as a dependency in `main/idf_component.yml`. See `examples/mdns_basic/main/idf_component.yml` as reference.

## Example App

See `examples/mdns_basic/`.
