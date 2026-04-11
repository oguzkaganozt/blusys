# WiFi Mesh

Self-organizing multi-hop mesh network built on the ESP-IDF `esp_mesh` stack. Each node acts as a WiFi station (connects to a parent) and a WiFi soft-AP (accepts children), so data can hop across the mesh without a direct path to the router.

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Quick Example

```c
#include <string.h>
#include "blusys/connectivity/wifi_mesh.h"

static void on_event(blusys_wifi_mesh_t *mesh, blusys_wifi_mesh_event_t event,
                     const blusys_wifi_mesh_event_info_t *info, void *ctx)
{
    if (event == BLUSYS_WIFI_MESH_EVENT_PARENT_CONNECTED) {
        printf("parent connected\n");
    }
    if (event == BLUSYS_WIFI_MESH_EVENT_GOT_IP) {
        printf("root has IP\n");
    }
}

void app_main(void)
{
    blusys_wifi_mesh_t *mesh = NULL;

    blusys_wifi_mesh_config_t cfg = {
        .mesh_id         = {0x77,0x77,0x77,0x77,0x77,0x77},
        .password        = "MAP_PASSW0RD",
        .channel         = 6,
        .router_ssid     = "MyRouter",
        .router_password = "routerpass",
        .on_event        = on_event,
    };

    blusys_wifi_mesh_open(&cfg, &mesh);

    /* Send a message to a known peer (fill in real MAC) */
    blusys_wifi_mesh_addr_t dst = { .addr = {0xAA,0xBB,0xCC,0xDD,0xEE,0xFF} };
    blusys_wifi_mesh_send(mesh, &dst, "hello", 5);

    /* Receive one reply */
    uint8_t buf[64];
    size_t len = sizeof(buf);
    blusys_wifi_mesh_addr_t src;
    blusys_wifi_mesh_recv(mesh, &src, buf, &len, 5000);

    blusys_wifi_mesh_close(mesh);
}
```

## Common Mistakes

- **`mesh_id`, `password`, or `channel` mismatch** — nodes on different IDs or passwords cannot see each other. Verify all devices use identical values.
- **No `router_ssid`** — without a reachable router, no node can become root. The `NO_PARENT_FOUND` event fires repeatedly. Set `router_ssid` to a real AP SSID.
- **Mixing with `blusys_wifi_open()`** — both modules initialize the WiFi driver. Only one can be active at a time; calling both returns `BLUSYS_ERR_INVALID_STATE` on the second open.
- **`recv` blocking during close** — if one task is blocked in `blusys_wifi_mesh_recv()` while another calls `blusys_wifi_mesh_close()`, the recv may not return until the mesh stack stops. Use a finite `timeout_ms` (e.g. 1000 ms) so the recv task can exit cleanly.
- **Short passwords** — the mesh AP password must be at least 6 characters. Shorter values cause `BLUSYS_ERR_INVALID_ARG` during configuration.

## Types

### `blusys_wifi_mesh_config_t`

```c
typedef struct {
    uint8_t                       mesh_id[6];       /* 6-byte mesh network identifier (required) */
    const char                   *password;          /* mesh AP password (6-64 chars, required) */
    uint8_t                       channel;           /* WiFi channel; 0 = auto */
    const char                   *router_ssid;       /* router SSID; root node connects here */
    const char                   *router_password;   /* router password; NULL for open networks */
    int                           max_layer;         /* max tree depth; 0 = use default (6) */
    int                           max_connections;   /* max children per node (1-10); 0 = use default (6) */
    blusys_wifi_mesh_event_cb_t   on_event;          /* optional lifecycle callback */
    void                         *user_ctx;          /* passed back in on_event */
} blusys_wifi_mesh_config_t;
```

### `blusys_wifi_mesh_event_t`

```c
typedef enum {
    BLUSYS_WIFI_MESH_EVENT_STARTED,
    BLUSYS_WIFI_MESH_EVENT_STOPPED,
    BLUSYS_WIFI_MESH_EVENT_PARENT_CONNECTED,
    BLUSYS_WIFI_MESH_EVENT_PARENT_DISCONNECTED,
    BLUSYS_WIFI_MESH_EVENT_CHILD_CONNECTED,
    BLUSYS_WIFI_MESH_EVENT_CHILD_DISCONNECTED,
    BLUSYS_WIFI_MESH_EVENT_NO_PARENT_FOUND,
    BLUSYS_WIFI_MESH_EVENT_GOT_IP,
} blusys_wifi_mesh_event_t;
```

### `blusys_wifi_mesh_addr_t`

```c
typedef struct {
    uint8_t addr[6]; /* 6-byte MAC address */
} blusys_wifi_mesh_addr_t;
```

### `blusys_wifi_mesh_event_info_t`

```c
typedef struct {
    blusys_wifi_mesh_addr_t peer; /* peer MAC for connect/disconnect events */
} blusys_wifi_mesh_event_info_t;
```

### `blusys_wifi_mesh_event_cb_t`

```c
typedef void (*blusys_wifi_mesh_event_cb_t)(blusys_wifi_mesh_t *mesh,
                                             blusys_wifi_mesh_event_t event,
                                             const blusys_wifi_mesh_event_info_t *info,
                                             void *user_ctx);
```

## Functions

### `blusys_wifi_mesh_open`

```c
blusys_err_t blusys_wifi_mesh_open(const blusys_wifi_mesh_config_t *config,
                                    blusys_wifi_mesh_t **out_handle);
```

Initializes the WiFi stack, configures the mesh network, and starts the mesh daemon. Internally calls `nvs_flash_init()`, `esp_netif_init()`, `esp_event_loop_create_default()`, and `esp_mesh_init()` — do not call these directly in the same application.

Only one handle may be open at a time. A second call returns `BLUSYS_ERR_INVALID_STATE` until `blusys_wifi_mesh_close()` is called.

**Parameters:**
- `config` — mesh configuration; `mesh_id` and `password` are required
- `out_handle` — output handle

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_STATE` if already open, `BLUSYS_ERR_NO_MEM` on allocation failure.

---

### `blusys_wifi_mesh_close`

```c
blusys_err_t blusys_wifi_mesh_close(blusys_wifi_mesh_t *handle);
```

Stops the mesh stack, shuts down WiFi, destroys network interfaces, and frees the handle. Do not call from within the `on_event` callback.

---

### `blusys_wifi_mesh_send`

```c
blusys_err_t blusys_wifi_mesh_send(blusys_wifi_mesh_t *handle,
                                    const blusys_wifi_mesh_addr_t *dst,
                                    const void *data, size_t len);
```

Sends a packet to another node by its MAC address. The packet is enqueued into the mesh TX queue and delivered asynchronously — the call returns immediately once the packet is accepted.

**Returns:** `BLUSYS_OK` if accepted, `BLUSYS_ERR_INVALID_ARG` if arguments are NULL or `len` is zero.

---

### `blusys_wifi_mesh_recv`

```c
blusys_err_t blusys_wifi_mesh_recv(blusys_wifi_mesh_t *handle,
                                    blusys_wifi_mesh_addr_t *src,
                                    void *buf, size_t *len,
                                    int timeout_ms);
```

Receives a packet addressed to this node. Blocks until a packet arrives or the timeout expires. On success, `src` (if non-NULL) is filled with the sender's MAC and `*len` is updated to the actual byte count received.

**Parameters:**
- `src` — output sender address; may be NULL to discard
- `buf` — caller-provided receive buffer
- `len` — on entry: buffer capacity; on exit: bytes received
- `timeout_ms` — maximum wait; pass `-1` to block indefinitely

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_TIMEOUT` if no packet arrived within the timeout.

---

### `blusys_wifi_mesh_is_root`

```c
bool blusys_wifi_mesh_is_root(blusys_wifi_mesh_t *handle);
```

Returns `true` if this node is currently the root of the mesh. Non-blocking.

---

### `blusys_wifi_mesh_get_layer`

```c
blusys_err_t blusys_wifi_mesh_get_layer(blusys_wifi_mesh_t *handle, int *out_layer);
```

Returns the current layer (depth) of this node. The root is layer 1; each hop adds one layer.

## Lifecycle

```text
blusys_wifi_mesh_open()
    └─ STARTED event
       └─ PARENT_CONNECTED event (once a parent is found)
          └─ GOT_IP event (root only, when it receives IP from router)

CHILD_CONNECTED event (when a child joins this node's soft-AP)
CHILD_DISCONNECTED event (when a child leaves)

PARENT_DISCONNECTED event (link loss — mesh automatically re-parents)
NO_PARENT_FOUND event (after exhaustive scan fails)

blusys_wifi_mesh_close()
    └─ STOPPED event
```

## Thread Safety

`blusys_wifi_mesh_send()` and `blusys_wifi_mesh_recv()` are thread-safe. `blusys_wifi_mesh_close()` is safe to call from any task except from inside the `on_event` callback.

## Limitations

- Only one mesh instance may be active at a time. Do not use alongside `blusys_wifi_open()`, `blusys_espnow_open()`, or any other module that initializes the WiFi driver.
- Calling `blusys_wifi_mesh_recv()` from one task while `blusys_wifi_mesh_close()` is in progress from another task may cause the recv call to block until the mesh stack stops. Use a short `timeout_ms` if concurrent close is expected.
- `router_ssid` is required for the root node to connect to the upstream network; leaf nodes that never become root may omit it.
- All nodes in a mesh must share the same `mesh_id`, `password`, and `channel`.

## Example App

See `examples/validation/wifi_mesh_basic/`.
