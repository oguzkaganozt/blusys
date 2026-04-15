# ESP-NOW

Connectionless peer-to-peer wireless for ESP32 devices — no access point required.

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

!!! note
    `blusys_espnow_open()` initializes WiFi in STA mode internally. Do not call `blusys_wifi_open()` in the same application — only one WiFi init can be active at a time.

## Quick Example

```c
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "blusys/connectivity/espnow.h"

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

- **Sending to an unregistered peer** — `esp_now_send` returns an error. Always call `blusys_espnow_add_peer()` first.
- **Payload larger than 250 bytes** — `blusys_espnow_send()` returns `BLUSYS_ERR_INVALID_ARG`. Split large transfers into multiple frames.
- **Channel mismatch** — ESP-NOW peers must be on the same WiFi channel. Set `channel = 0` in `blusys_espnow_peer_t` to use the current channel automatically.
- **Calling `close` from within a callback** — both callbacks run in the WiFi task; calling `blusys_espnow_close()` from inside them will deadlock. Signal a FreeRTOS task or event group instead.
- **Using alongside `blusys_wifi_open()`** — both modules initialize the WiFi driver; only one can be active at a time. Use ESP-NOW for peer-to-peer; use WiFi for AP-connected networking.

## Types

### `blusys_espnow_config_t`

```c
typedef struct {
    uint8_t                  channel;        /**< WiFi channel; 0 = use current */
    blusys_espnow_recv_cb_t  recv_cb;        /**< Receive callback (required) */
    void                    *recv_user_ctx;  /**< Passed to recv_cb */
    blusys_espnow_send_cb_t  send_cb;        /**< Send-complete callback (optional) */
    void                    *send_user_ctx;  /**< Passed to send_cb */
} blusys_espnow_config_t;
```

Configuration passed to `blusys_espnow_open()`. `recv_cb` is required; all other fields are optional.

### `blusys_espnow_peer_t`

```c
typedef struct {
    uint8_t addr[6];   /**< Peer MAC address */
    uint8_t channel;   /**< 0 = same channel as sender */
    bool    encrypt;   /**< Enable CCMP encryption */
    uint8_t lmk[16];   /**< Local master key (used only when encrypt = true) */
} blusys_espnow_peer_t;
```

Peer descriptor passed to `blusys_espnow_add_peer()`. Set `channel = 0` to use the same channel as the sender. `lmk` is ignored when `encrypt = false`.

### `blusys_espnow_recv_cb_t`

```c
typedef void (*blusys_espnow_recv_cb_t)(const uint8_t peer_addr[6],
                                        const uint8_t *data, size_t len,
                                        void *user_ctx);
```

Called from the WiFi task when data arrives from any peer. `data` is valid only for the duration of the callback — copy any bytes you need before returning. `peer_addr` is the sender's MAC address.

### `blusys_espnow_send_cb_t`

```c
typedef void (*blusys_espnow_send_cb_t)(const uint8_t peer_addr[6],
                                        bool success, void *user_ctx);
```

Called from the WiFi task after a send operation completes. `success` is `true` if the frame was acknowledged, `false` if delivery failed.

## Functions

### `blusys_espnow_open`

```c
blusys_err_t blusys_espnow_open(const blusys_espnow_config_t *config,
                                blusys_espnow_t **out_handle);
```

Initializes NVS, the event loop, WiFi in STA mode, and ESP-NOW, then returns a handle.

Only one handle may be open at a time — a second call returns `BLUSYS_ERR_INVALID_STATE` until `blusys_espnow_close()` is called.

**Parameters:**

- `config` — configuration struct; `recv_cb` must be non-NULL
- `out_handle` — output handle

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` if `config` or `recv_cb` is NULL, `BLUSYS_ERR_INVALID_STATE` if already open, `BLUSYS_ERR_NO_MEM` on allocation failure.

---

### `blusys_espnow_close`

```c
blusys_err_t blusys_espnow_close(blusys_espnow_t *handle);
```

Stops ESP-NOW, shuts down WiFi, and frees the handle. The handle must not be used after this call.

---

### `blusys_espnow_add_peer`

```c
blusys_err_t blusys_espnow_add_peer(blusys_espnow_t *handle,
                                    const blusys_espnow_peer_t *peer);
```

Registers a peer so that `blusys_espnow_send()` can address it. The broadcast address (`FF:FF:FF:FF:FF:FF`) is a valid peer.

**Parameters:**

- `handle` — open ESP-NOW handle
- `peer` — peer descriptor

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` if the peer is already registered or arguments are NULL.

---

### `blusys_espnow_remove_peer`

```c
blusys_err_t blusys_espnow_remove_peer(blusys_espnow_t *handle,
                                       const uint8_t addr[6]);
```

Unregisters a peer by MAC address.

**Parameters:**

- `handle` — open ESP-NOW handle
- `addr` — peer MAC address (6 bytes)

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_NOT_FOUND` if the address is not registered.

---

### `blusys_espnow_send`

```c
blusys_err_t blusys_espnow_send(blusys_espnow_t *handle,
                                const uint8_t addr[6],
                                const void *data, size_t size);
```

Queues a frame for transmission to a registered peer. The function returns immediately; the send callback (if configured) fires when the frame is acknowledged or times out.

**Parameters:**

- `handle` — open ESP-NOW handle
- `addr` — destination MAC address; must be a registered peer (6 bytes)
- `data` — payload bytes
- `size` — number of bytes to send (max 250)

**Returns:** `BLUSYS_OK` if the frame was accepted, `BLUSYS_ERR_INVALID_ARG` if `size > 250` or any pointer is NULL.

## Lifecycle

```
blusys_espnow_open()
    │
    ├─ blusys_espnow_add_peer()    (register one or more peers)
    │
    ├─ blusys_espnow_send()        (transmit; recv_cb fires on receive)
    │
    └─ blusys_espnow_remove_peer() (optional cleanup before close)
    │
blusys_espnow_close()
```

## Thread Safety

All functions are thread-safe. The receive and send callbacks are invoked from the WiFi task — do not call `blusys_espnow_close()` from within a callback.

## Limitations

- Maximum payload size is 250 bytes (`ESP_NOW_MAX_DATA_LEN`)
- Maximum number of registered peers is 20
- One handle per process — only one ESP-NOW instance can run at a time
- Cannot be used alongside `blusys_wifi_open()` in the same application
- No acknowledgement retry control — retries are handled transparently by the hardware

## Example App

See `examples/validation/wireless_esp_lab` (ESP-NOW scenario).
