# Send and Receive with ESP-NOW

## Problem Statement

You want two (or more) ESP32 devices to exchange short messages directly over WiFi without connecting to a router — instant pairing, no AP, no TCP/IP stack overhead.

## Prerequisites

- A supported board (ESP32, ESP32-C3, or ESP32-S3)
- No custom partition table or sdkconfig changes required

## Minimal Example

```c
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "blusys/espnow.h"

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

## Two-Device Setup

When running on two separate boards:

1. Flash the same firmware to both devices
2. Each device sends to the broadcast address and receives from the other
3. To restrict communication to known peers, use specific MAC addresses instead of broadcast

To read a device's own MAC address:

```c
#include "esp_wifi.h"
uint8_t mac[6];
esp_wifi_get_mac(WIFI_IF_STA, mac);
printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
```

## APIs Used

- `blusys_espnow_open()` — initializes WiFi + ESP-NOW; registers callbacks
- `blusys_espnow_add_peer()` — registers a destination MAC before sending
- `blusys_espnow_send()` — queues a frame; non-blocking
- `blusys_espnow_remove_peer()` — deregisters a peer
- `blusys_espnow_close()` — shuts down WiFi + ESP-NOW and frees the handle

## Expected Behavior

On a single device (broadcast loopback):

```
[espnow] open OK
[espnow] broadcast peer added
[espnow] send #1: queued
[espnow] send to ff:ff:ff:ff:ff:ff  OK
[espnow] recv from <own-mac>  len=21  "hello from espnow #1"
...
[espnow] close: OK
```

The device receives its own broadcast frame because ESP-NOW delivers all frames matching the channel and interface.

## Common Mistakes

- **Sending to an unregistered peer** — `esp_now_send` returns an error. Always call `blusys_espnow_add_peer()` first.
- **Payload larger than 250 bytes** — `blusys_espnow_send()` returns `BLUSYS_ERR_INVALID_ARG`. Split large transfers into multiple frames.
- **Channel mismatch** — ESP-NOW peers must be on the same WiFi channel. Set `channel = 0` in `blusys_espnow_peer_t` to use the current channel automatically.
- **Calling `close` from within a callback** — both callbacks run in the WiFi task; calling `blusys_espnow_close()` from inside them will deadlock. Signal a FreeRTOS task or event group instead.
- **Using alongside `blusys_wifi_open()`** — both modules initialize the WiFi driver; only one can be active at a time. Use ESP-NOW for peer-to-peer; use WiFi for AP-connected networking.

## Example App

See `examples/espnow_basic/` for a runnable loopback example that sends 5 messages to the broadcast address and prints each send and receive event.

## API Reference

For full type definitions and function signatures, see [ESP-NOW API Reference](../modules/espnow.md).
