# Capabilities

Capabilities wrap runtime services so product code configures them, reacts to events, and reads status instead of owning lifecycles.

## How they fit

1. Define config in `main/`.
2. Add capability instances to `app_spec`.
3. Translate integration events in `on_event`.
4. Query live state with `ctx.status_of<T>()`.

## Common capabilities

| Capability | Best for | Depends on |
|------------|----------|------------|
| `connectivity` | Wi-Fi, SNTP, mDNS | none |
| `telemetry` | buffered delivery | `connectivity` |
| `ota` | firmware update | `connectivity`, `bluetooth`, or `usb` |
| `lan_control` | local HTTP control | `connectivity` |
| `storage` | SPIFFS / FATFS | none |
| `diagnostics` | health snapshots | none |

## Wiring

```cpp
blusys::connectivity_config conn_cfg{
    .wifi_ssid     = "MyNetwork",
    .wifi_password = "password",
    .sntp_server   = "pool.ntp.org",
};

blusys::storage_config stor_cfg{
    .spiffs_base_path = "/fs",
    .fatfs_base_path  = "/sd",
};

static blusys::connectivity_capability conn{conn_cfg};
static blusys::storage_capability stor{stor_cfg};
static blusys::capability_list_storage capabilities{&conn, &stor};

static const blusys::app_spec<State, Action> spec{
    .initial_state = {},
    .update        = update,
    .on_event      = on_event,
    .capabilities  = &capabilities,
};
```

## Event bridge

```cpp
std::optional<Action> on_event(blusys::event event, State &state)
{
    (void)state;
    if (event.source == blusys::event_source::integration &&
        event.id == 0x0102u) {
        return Action::wifi_connected;
    }
    return std::nullopt;
}
```

## Status

```cpp
const auto *status = ctx.status_of<blusys::connectivity_capability>();
if (status != nullptr && status->connected) {
    // Wi-Fi is up
}
```

## Advanced use

If you need direct filesystem handles, `ctx.fx().storage.spiffs()` and `ctx.fx().storage.fatfs()` are available on device builds.

## Next steps

- [Capability composition](capability-composition.md)
- [Authoring a capability](capability-authoring.md)
- [Product-custom capabilities](custom-capabilities.md)
