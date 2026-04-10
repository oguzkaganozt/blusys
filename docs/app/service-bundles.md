# Service Bundles

Service bundles provide higher-level product capabilities so apps stop assembling low-level service lifecycles manually.

## Connected-Device Bundle

The connected-device bundle handles WiFi connectivity with optional local control, mDNS, and SNTP:

```cpp
auto connectivity = blusys::app::connected_device_bundle{
    .wifi_ssid = "MyNetwork",
    .wifi_password = "password",
    .enable_mdns = true,
    .enable_sntp = true,
    .enable_local_ctrl = false,
};
```

### What it owns

- WiFi connection and reconnect lifecycle
- mDNS registration and service advertisement
- SNTP time synchronization
- Local control endpoint (optional)
- Connectivity state reporting via app actions

### App integration

The bundle dispatches connectivity actions to your reducer:

```cpp
void update(app_ctx& ctx, MyState& state, MyAction action) {
    switch (action) {
        case MyAction::wifi_connected:
            state.connected = true;
            break;
        case MyAction::wifi_disconnected:
            state.connected = false;
            break;
    }
}
```

## Storage Bundle

The storage bundle provides a unified interface over the current storage surfaces:

```cpp
auto storage = blusys::app::storage_bundle{};

// Key-value storage
storage.set("brightness", 75);
int brightness = storage.get<int>("brightness", 50);  // default: 50

// File storage
storage.write_file("/config.json", data, len);
storage.read_file("/config.json", buffer, &len);
```

## Raw Service Access

Raw service APIs (`blusys_wifi_*`, `blusys_mqtt_*`, etc.) remain available for advanced use cases but are not the recommended first path. See the [Services](../services/index.md) section for direct API documentation.
