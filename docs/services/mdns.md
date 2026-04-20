# mDNS

Zero-configuration networking: advertise services and discover other devices on the local network using multicast DNS.

## Quick Example

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

After `open()` the device responds to `<hostname>.local` queries:

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

## Common Mistakes

- calling `blusys_mdns_open()` before WiFi is connected — the responder needs an active network interface
- using hostnames longer than 63 characters — mDNS labels are limited to 63 bytes
- expecting `query()` to find the device's own services — mDNS queries discover *other* devices on the network

## Target Support

| Target | Supported |
|--------|-----------|
| ESP32 | yes |
| ESP32-C3 | yes |
| ESP32-S3 | yes |

## Thread Safety

All functions are serialized with an internal mutex. Concurrent calls on the same handle are safe.

## ISR Notes

No ISR-safe calls are defined for the mDNS module.

## Limitations

- WiFi must be connected before calling `blusys_mdns_open()`
- this module requires the `espressif/mdns` managed component — declare it as a dependency in `main/idf_component.yml` (see `examples/validation/network_services/main/idf_component.yml`)

## Example App

See `examples/validation/network_services/` (`net_mdns` scenario).
