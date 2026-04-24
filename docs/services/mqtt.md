# MQTT

Client API for publish/subscribe messaging over WiFi.

## At a glance

- requires a working IP path
- publish is fire-and-forget
- callbacks run on the MQTT client task

## Quick example

```c
blusys_mqtt_t *mqtt = NULL;
blusys_mqtt_config_t cfg = {
    .broker_url = "mqtt://broker.example.com:1883",
    .timeout_ms = 10000,
};

blusys_mqtt_open(&cfg, &mqtt);
blusys_mqtt_connect(mqtt);
blusys_mqtt_subscribe(mqtt, "sensors/+/temp", BLUSYS_MQTT_QOS_0);
blusys_mqtt_publish(mqtt, "sensors/kitchen/temp", (const uint8_t *)"21.4", 4, BLUSYS_MQTT_QOS_0);
blusys_mqtt_close(mqtt);
```

## Common mistakes

- calling `connect()` before WiFi has an IP
- calling `connect()` or `close()` from inside the message callback
- expecting `publish()` to wait for broker ack

## Target support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Thread safety

- publish, subscribe, and unsubscribe serialise concurrent callers
- open and close must not overlap with other calls on the same handle

## Limitations

- WiFi must be established before `connect()`
- `server_cert_pem` is borrowed, not copied

## Example app

`examples/reference/connectivity/`
