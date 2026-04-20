# ESP-NOW

Connectionless peer-to-peer wireless for ESP32 devices — no access point required.

## Quick Example

```c
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "blusys/blusys.h"

static const uint8_t BROADCAST[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

static void on_recv(const uint8_t peer[6], const uint8_t *data,
                    size_t len, void *ctx)
{
    printf("recv: %.*s\n", (int)len, (const char *)data);
}

void app_main(void)
{
    blusys_espnow_t *h = NULL;
    blusys_espnow_config_t cfg = { .recv_cb = on_recv };
    blusys_espnow_open(&cfg, &h);

    blusys_espnow_peer_t peer = {};
    memcpy(peer.addr, BROADCAST, 6);
    blusys_espnow_add_peer(h, &peer);

    blusys_espnow_send(h, BROADCAST, (const uint8_t *)"hello", 5);
    vTaskDelay(pdMS_TO_TICKS(1000));   /* wait for recv_cb */

    blusys_espnow_remove_peer(h, BROADCAST);
    blusys_espnow_close(h);
}
```

## Common Mistakes

- **sending to an unregistered peer** — `esp_now_send` returns an error. Always call `blusys_espnow_add_peer()` first.
- **payload larger than 250 bytes** — `blusys_espnow_send()` returns `BLUSYS_ERR_INVALID_ARG`. Split large transfers into multiple frames.
- **channel mismatch** — ESP-NOW peers must be on the same WiFi channel. Set `channel = 0` in `blusys_espnow_peer_t` to use the current channel automatically.
- **calling `close` from within a callback** — both callbacks run in the WiFi task; calling `blusys_espnow_close()` from inside them deadlocks. Signal a FreeRTOS task or event group instead.
- **combining with `blusys_wifi_open()`** — both modules initialize the WiFi driver; only one can be active at a time. Use ESP-NOW for peer-to-peer; use WiFi for AP-connected networking.

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

!!! note
    `blusys_espnow_open()` initializes WiFi in STA mode internally. Do not call `blusys_wifi_open()`, `blusys_wifi_mesh_open()`, or `blusys_wifi_prov_open()` in the same application.

## Thread Safety

All functions are thread-safe. The receive and send callbacks are invoked from the WiFi task — do not call `blusys_espnow_close()` from within a callback.

## Limitations

- maximum payload size is 250 bytes (`ESP_NOW_MAX_DATA_LEN`)
- maximum number of registered peers is 20
- one handle per process — only one ESP-NOW instance can run at a time
- cannot be used alongside `blusys_wifi_open()` in the same application
- no acknowledgement retry control — retries are handled transparently by the hardware

## Example App

See `examples/validation/wireless_esp_lab` (ESP-NOW scenario).
