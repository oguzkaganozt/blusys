# Build a Self-Organizing Mesh Network

## Problem Statement

You want multiple ESP32 devices to relay messages to each other without a direct WiFi connection to a router. Nodes that are out of router range should still reach the internet by hopping through nearer nodes.

## Prerequisites

- Two or more supported boards (ESP32, ESP32-C3, or ESP32-S3)
- An existing WiFi access point that the root node can reach
- All boards flashed with the same firmware

## Minimal Example

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

## Multi-Device Setup

All devices run the same firmware. Flash with the same `MESH_ID`, `MESH_PASSWORD`, and `MESH_CHANNEL`:

```bash
blusys run examples/wifi_mesh_basic /dev/ttyUSB0
blusys run examples/wifi_mesh_basic /dev/ttyUSB1
```

The mesh stack self-elects a root based on RSSI to the router. The root connects to `router_ssid`; all other nodes connect to the root or to intermediate nodes.

To read a device's own MAC address (useful for addressing peers):

```c
#include "esp_wifi.h"
uint8_t mac[6];
esp_wifi_get_mac(WIFI_IF_STA, mac);
printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
```

## APIs Used

- `blusys_wifi_mesh_open()` — initializes WiFi + mesh, starts daemon
- `blusys_wifi_mesh_send()` — enqueues a packet to a peer's MAC
- `blusys_wifi_mesh_recv()` — blocks until a packet arrives or timeout
- `blusys_wifi_mesh_is_root()` — check if this node is the root
- `blusys_wifi_mesh_get_layer()` — query current depth in the tree
- `blusys_wifi_mesh_close()` — stops mesh, frees resources

## Expected Behavior

On a single device (no peers present):

```
[mesh] opening mesh network (ID 77:77:77:77:77:77)...
[mesh] open OK
[mesh] event: started
[mesh] event: no_parent_found
[mesh] layer=0  root=no
...
```

On two devices within range of the router:

```
# device 1 (becomes root)
[mesh] event: started
[mesh] event: parent_connected  ← connected to router
[mesh] event: got_ip            ← root received IP
[mesh] event: child_connected   ← device 2 joined

# device 2 (leaf)
[mesh] event: started
[mesh] event: parent_connected  ← connected to device 1
[mesh] layer=2  root=no
```

## Common Mistakes

- **`mesh_id`, `password`, or `channel` mismatch** — nodes on different IDs or passwords cannot see each other. Verify all devices use identical values.
- **No `router_ssid`** — without a reachable router, no node can become root. The `NO_PARENT_FOUND` event fires repeatedly. Set `router_ssid` to a real AP SSID.
- **Mixing with `blusys_wifi_open()`** — both modules initialize the WiFi driver. Only one can be active at a time; calling both returns `BLUSYS_ERR_INVALID_STATE` on the second open.
- **`recv` blocking during close** — if one task is blocked in `blusys_wifi_mesh_recv()` while another calls `blusys_wifi_mesh_close()`, the recv may not return until the mesh stack stops. Use a finite `timeout_ms` (e.g. 1000 ms) so the recv task can exit cleanly.
- **Short passwords** — the mesh AP password must be at least 6 characters. Shorter values cause `BLUSYS_ERR_INVALID_ARG` during configuration.

## Example App

See `examples/wifi_mesh_basic/` for a runnable example that opens the mesh, polls layer and root status every 5 seconds for 30 seconds, and then closes cleanly.

## API Reference

For full type definitions and function signatures, see [WiFi Mesh API Reference](../modules/wifi_mesh.md).
