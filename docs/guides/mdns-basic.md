# Advertise and Discover Services with mDNS

## Problem Statement

You want your ESP32 to be reachable by hostname on the local network (e.g. `my-device.local`) and to advertise or discover services without a central DNS server.

## Prerequisites

- a supported board
- WiFi connection to a local network

## Minimal Example

```c
blusys_wifi_t *wifi;
blusys_mdns_t *mdns;

/* connect WiFi first */
blusys_wifi_sta_config_t wifi_cfg = { .ssid = "MyNetwork", .password = "secret" };
blusys_wifi_open(&wifi_cfg, &wifi);
blusys_wifi_connect(wifi, 10000);

/* start mDNS */
blusys_mdns_config_t mdns_cfg = {
    .hostname      = "my-sensor",
    .instance_name = "Kitchen Sensor",
};
blusys_mdns_open(&mdns_cfg, &mdns);

/* advertise an HTTP service */
blusys_mdns_txt_item_t txt[] = {
    { .key = "path", .value = "/" },
};
blusys_mdns_add_service(mdns, "Kitchen Sensor", "_http",
                         BLUSYS_MDNS_PROTO_TCP, 80, txt, 1);

/* discover other HTTP services */
blusys_mdns_result_t results[8];
size_t count = 0;
blusys_mdns_query(mdns, "_http", BLUSYS_MDNS_PROTO_TCP,
                   3000, results, 8, &count);

for (size_t i = 0; i < count; i++) {
    printf("Found: %s at %s:%u\n",
           results[i].instance_name, results[i].ip, results[i].port);
}

blusys_mdns_close(mdns);
```

## APIs Used

- `blusys_mdns_open()` starts the mDNS responder and registers the hostname
- `blusys_mdns_add_service()` advertises a service via DNS-SD
- `blusys_mdns_remove_service()` stops advertising a service
- `blusys_mdns_query()` discovers services on the local network
- `blusys_mdns_close()` stops the responder and frees the handle

## Hostname Resolution

After `blusys_mdns_open()`, the device responds to `<hostname>.local` queries. Any device on the same network can reach it:

```
ping my-sensor.local
curl http://my-sensor.local/
```

## Service Types

Common DNS-SD service types:

| Service | Type String | Protocol |
|---------|-------------|----------|
| HTTP | `_http` | TCP |
| MQTT | `_mqtt` | TCP |
| CoAP | `_coap` | UDP |

## TXT Records

TXT records attach metadata to a service. They are key-value pairs visible to clients during discovery:

```c
blusys_mdns_txt_item_t txt[] = {
    { .key = "board",   .value = "esp32s3" },
    { .key = "version", .value = "1.0"     },
};
```

## Common Mistakes

- calling `blusys_mdns_open()` before WiFi is connected — the responder needs an active network interface
- using hostnames longer than 63 characters — mDNS labels are limited to 63 bytes
- expecting `query()` to find the device's own services — mDNS queries discover *other* devices on the network

## Example App

See `examples/mdns_basic/` for a runnable example.


## API Reference

For full type definitions and function signatures, see [mDNS API Reference](../modules/mdns.md).
