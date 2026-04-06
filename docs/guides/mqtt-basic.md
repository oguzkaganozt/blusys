# Publish and Subscribe with MQTT

Connect an ESP32 to an MQTT broker, subscribe to a topic, and publish messages.

## Prerequisites

- A WiFi-capable board: ESP32, ESP32-C3, or ESP32-S3
- An accessible MQTT broker (the default example uses `test.mosquitto.org`, a public broker)
- WiFi credentials

## Minimal Example

```c
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "blusys/blusys.h"

static void on_message(const char *topic, const uint8_t *payload,
                        size_t payload_len, void *user_ctx)
{
    printf("[%s]: %.*s\n", topic, (int)payload_len, (const char *)payload);
}

void app_main(void)
{
    /* 1. Connect to WiFi */
    blusys_wifi_t *wifi;
    blusys_wifi_sta_config_t wifi_cfg = { .ssid = "myssid", .password = "mypassword" };
    blusys_wifi_open(&wifi_cfg, &wifi);
    blusys_wifi_connect(wifi, 10000);

    /* 2. Open MQTT client */
    blusys_mqtt_t *mqtt;
    blusys_mqtt_config_t mqtt_cfg = {
        .broker_url = "mqtt://test.mosquitto.org",
        .timeout_ms = 10000,
        .message_cb = on_message,
    };
    blusys_mqtt_open(&mqtt_cfg, &mqtt);

    /* 3. Connect to broker */
    blusys_mqtt_connect(mqtt);

    /* 4. Subscribe and publish */
    blusys_mqtt_subscribe(mqtt, "blusys/test", BLUSYS_MQTT_QOS_1);
    const char *msg = "hello";
    blusys_mqtt_publish(mqtt, "blusys/test",
                         (const uint8_t *)msg, strlen(msg), BLUSYS_MQTT_QOS_1);

    /* 5. Wait for echo, then clean up */
    vTaskDelay(pdMS_TO_TICKS(5000));
    blusys_mqtt_disconnect(mqtt);
    blusys_mqtt_close(mqtt);
    blusys_wifi_disconnect(wifi);
    blusys_wifi_close(wifi);
}
```

## APIs Used

- `blusys_mqtt_open()` — allocate client with broker URL and message callback
- `blusys_mqtt_connect()` — blocking connect; waits for CONNACK
- `blusys_mqtt_subscribe()` — subscribe to a topic filter
- `blusys_mqtt_publish()` — fire-and-forget publish
- `blusys_mqtt_disconnect()` / `blusys_mqtt_close()` — graceful shutdown

## Receiving Messages

The `message_cb` is called from the MQTT client task each time a message arrives on a subscribed topic:

```c
static void on_message(const char *topic, const uint8_t *payload,
                        size_t payload_len, void *user_ctx)
{
    /* topic is null-terminated */
    /* payload is raw bytes, use payload_len — it is NOT null-terminated */
    printf("topic=%s len=%zu\n", topic, payload_len);
}
```

Keep the callback short. Do not call `blusys_mqtt_connect()`, `blusys_mqtt_disconnect()`, or `blusys_mqtt_close()` from inside the callback.

## Publishing Binary Payloads

`payload` is `const uint8_t *`, so binary data works directly:

```c
uint8_t buf[4] = { 0x01, 0x02, 0x03, 0x04 };
blusys_mqtt_publish(mqtt, "blusys/raw", buf, sizeof(buf), BLUSYS_MQTT_QOS_0);
```

## Using QoS Levels

| QoS | Guarantee |
|-----|-----------|
| `BLUSYS_MQTT_QOS_0` | Fire-and-forget; no delivery guarantee |
| `BLUSYS_MQTT_QOS_1` | At-least-once delivery |
| `BLUSYS_MQTT_QOS_2` | Exactly-once delivery |

`publish()` is non-blocking regardless of QoS. Acknowledgment exchange happens in the background within the ESP-IDF MQTT stack.

## Using TLS (MQTTS)

Pass a PEM CA certificate and use an `mqtts://` URL:

```c
extern const char broker_cert[] asm("_binary_broker_cert_pem_start");

blusys_mqtt_config_t cfg = {
    .broker_url     = "mqtts://secure.broker.example.com:8883",
    .server_cert_pem = broker_cert,
    .timeout_ms     = 15000,
    .message_cb     = on_message,
};
```

Pass `NULL` for `server_cert_pem` to skip certificate verification (not recommended for production).

## Common Mistakes

**Not waiting for WiFi before calling `connect()`**

```c
/* Wrong: connect() will fail if WiFi is not yet up */
blusys_mqtt_open(&cfg, &mqtt);
blusys_mqtt_connect(mqtt);     /* fails — no IP yet */

/* Correct: connect WiFi first */
blusys_wifi_connect(wifi, 10000);
blusys_mqtt_connect(mqtt);
```

**Treating `payload` as a C string**

The `payload` pointer in the callback is not null-terminated. Copy it if you need a string:

```c
char buf[128];
size_t len = payload_len < sizeof(buf) - 1 ? payload_len : sizeof(buf) - 1;
memcpy(buf, payload, len);
buf[len] = '\0';
```

**Calling `close()` from inside the callback**

The callback runs on the MQTT client task. Calling `close()` from inside it deadlocks. Post to a queue or set a flag, then call `close()` from `app_main`.

## Expected Runtime Behavior

`blusys_mqtt_connect()` blocks until the TCP connection is established and the broker sends CONNACK. On `test.mosquitto.org` with a good WiFi connection, this takes roughly 200–500 ms.

After subscribing to a topic and publishing to the same topic, the broker echoes the message back and `message_cb` fires within a few hundred milliseconds.

## Example App

See `examples/mqtt_basic/`.


## API Reference

For full type definitions and function signatures, see [MQTT API Reference](../modules/mqtt.md).
