# WiFi Mesh

Self-organizing multi-hop mesh network built on the ESP-IDF `esp_mesh` stack. Each node acts as a WiFi station (connects to a parent) and a WiFi soft-AP (accepts children), so data can hop across the mesh without a direct path to the router.

## Quick Example

```c
#include <string.h>
#include "blusys/blusys.h"

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
- **no `router_ssid`** — without a reachable router, no node can become root. The `NO_PARENT_FOUND` event fires repeatedly. Set `router_ssid` to a real AP SSID.
- **mixing with `blusys_wifi_open()`** — both modules initialize the WiFi driver. Only one can be active at a time; calling both returns `BLUSYS_ERR_INVALID_STATE` on the second open.
- **`recv` blocking during close** — if one task is blocked in `blusys_wifi_mesh_recv()` while another calls `blusys_wifi_mesh_close()`, the recv may not return until the mesh stack stops. Use a finite `timeout_ms` (e.g. 1000 ms) so the recv task can exit cleanly.
- **short passwords** — the mesh AP password must be at least 6 characters. Shorter values cause `BLUSYS_ERR_INVALID_ARG` during configuration.

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Thread Safety

`blusys_wifi_mesh_send()` and `blusys_wifi_mesh_recv()` are thread-safe. `blusys_wifi_mesh_close()` is safe to call from any task except from inside the `on_event` callback.

## Limitations

- only one mesh instance may be active at a time. Do not use alongside `blusys_wifi_open()`, `blusys_espnow_open()`, or any other module that initializes the WiFi driver.
- calling `blusys_wifi_mesh_recv()` from one task while `blusys_wifi_mesh_close()` is in progress from another task may cause the recv call to block until the mesh stack stops. Use a short `timeout_ms` if concurrent close is expected.
- `router_ssid` is required for the root node to connect to the upstream network; leaf nodes that never become root may omit it.
- all nodes in a mesh must share the same `mesh_id`, `password`, and `channel`.

## Example App

See `examples/validation/wireless_esp_lab` (Wi-Fi mesh scenario).
