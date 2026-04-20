# MQTT Client

Publish/subscribe messaging client for connecting to an MQTT broker over WiFi.

## Quick Example

```c
#include "blusys/blusys.h"

static void on_message(const char *topic, const uint8_t *payload,
                       size_t len, void *ctx)
{
    printf("[%s] %.*s\n", topic, (int)len, (const char *)payload);
}

blusys_mqtt_t *mqtt = NULL;
blusys_mqtt_config_t cfg = {
    .broker_url = "mqtt://broker.example.com:1883",
    .timeout_ms = 10000,
    .message_cb = on_message,
};
blusys_mqtt_open(&cfg, &mqtt);
blusys_mqtt_connect(mqtt);

blusys_mqtt_subscribe(mqtt, "sensors/+/temp", BLUSYS_MQTT_QOS_0);
blusys_mqtt_publish(mqtt, "sensors/kitchen/temp",
                    (const uint8_t *)"21.4", 4, BLUSYS_MQTT_QOS_0);

/* ... later ... */
blusys_mqtt_disconnect(mqtt);
blusys_mqtt_close(mqtt);
```

Optional Last-Will-and-Testament fields (`will_topic`, `will_payload`, `will_qos`, `will_retain`) are published by the broker when the client disconnects uncleanly.

## Common Mistakes

- **calling `connect()` before WiFi has an IP** — returns `BLUSYS_ERR_IO` at the TCP stage
- **assuming `publish()` waits for ack** — it is fire-and-forget; QoS acks are handled internally and not surfaced
- **blocking inside the message callback** — the callback runs on the MQTT client task; keep it short
- **calling `connect()` / `disconnect()` / `close()` from inside the message callback** — this deadlocks
- **`server_cert_pem` going out of scope** — the pointer is not copied; keep it valid for the handle lifetime

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

Requires WiFi to be connected before calling `blusys_mqtt_connect()`.

## Thread Safety

- `publish()`, `subscribe()`, and `unsubscribe()` serialise concurrent callers via an internal lock
- `open()` and `close()` must not be called concurrently with any other function on the same handle
- `is_connected()` is safe to call without holding the internal lock

## ISR Notes

No ISR-safe calls are defined for the MQTT module.

## Limitations

- WiFi must be established before `connect()`
- `publish()` is fire-and-forget; no acknowledgment confirmation at the Blusys API level
- the message callback is called from the MQTT client task; it must return promptly and must not call `connect()`, `disconnect()`, or `close()` on the same handle
- MQTTS verification requires a valid PEM cert in `config.server_cert_pem`; NULL skips verification

## Example App

See `examples/reference/connectivity/` (`ref_connectivity_mqtt` scenario).
