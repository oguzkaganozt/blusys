# Capabilities

Capabilities provide higher-level product flows so apps stop assembling low-level service lifecycles manually. They own startup, polling, teardown, and event-to-action bridging for a specific system concern.

## Capability Contract

Every capability:

1. Is configured with a plain data-only config struct (no hidden side effects in construction).
2. Is composed in `main/app_main.cpp` or a small product-local wiring file and registered on `app_spec.capabilities` — not constructed ad hoc from `ui/`.
3. Owns lifecycle and runtime-service integration on the framework side; product code requests work through reducer-driven actions.
4. Exposes **status** through the app-facing query path (`app_ctx` accessors) using consistent field naming (`capability_ready` signals that the capability’s default stack is up).
5. Raises **events** with stable enum codes; the app wiring handles them in `on_event`, and `update()` reacts to the resulting actions.
6. May expose advanced escape hatches, but the recommended path stays event- and reducer-driven.

Apps bridge capability events to product actions via `on_event`:

```cpp
std::optional<Action> on_event(blusys::event event, State &state)
{
    (void)state;
    if (event.source == blusys::event_source::integration &&
        event.id == 0x0102u) { // example: connectivity ready
        return Action::wifi_connected;
    }
    return std::nullopt;
}
```

## `connectivity_capability`

Manages WiFi connection and reconnect lifecycle, SNTP, and mDNS.

### Config

```cpp
blusys::connectivity_config conn_cfg{
    .wifi_ssid      = "MyNetwork",
    .wifi_password  = "password",
    .sntp_server    = "pool.ntp.org",   // optional
    .mdns_hostname  = "my-device",      // optional
};
```

### Wiring (in `main/app_main.cpp`)

```cpp
#include "blusys/framework/capabilities/connectivity.hpp"
#include "blusys/framework/capabilities/list.hpp"

static blusys::connectivity_capability conn{conn_cfg};
static blusys::capability_list_storage capabilities{&conn};

static const blusys::app_spec<State, Action> spec{
    .initial_state = {},
    .update        = update,
    .on_event      = on_event,
    .capabilities  = &capabilities,
};
```

### Status query

```cpp
// In on_tick or update():
const auto *status = ctx.status_of<blusys::connectivity_capability>();
if (status != nullptr && status->connected) {
    // WiFi is up
}
```

### Events

| Event | Description |
|-------|-------------|
| `connectivity_event::got_ip` | WiFi connected and IP assigned |
| `connectivity_event::disconnected` | WiFi disconnected |
| `connectivity_event::time_synced` | SNTP sync complete |

## `storage_capability`

Manages SPIFFS and FAT filesystem mounting.

### Config

```cpp
blusys::storage_config stor_cfg{
    .spiffs_base_path = "/fs",    // nullptr to skip SPIFFS
    .fatfs_base_path  = "/sd",    // nullptr to skip FAT
};
```

### Wiring

```cpp
#include "blusys/framework/capabilities/storage.hpp"

static blusys::storage_capability stor{stor_cfg};
static blusys::capability_list_storage capabilities{&conn, &stor};
```

### File access (ESP-IDF target only)

```cpp
// After storage mounts, access raw filesystem handles:
blusys_fs_t    *spiffs = ctx.fx().storage.spiffs();  // POSIX path-rooted FS
blusys_fatfs_t *fatfs  = ctx.fx().storage.fatfs();   // FAT FS handle
```

### Status query

```cpp
const auto *status = ctx.status_of<blusys::storage_capability>();
if (status != nullptr && status->spiffs_mounted) {
    // SPIFFS is ready
}
```

## Raw Service Access

The underlying `blusys_wifi_*`, `blusys_sntp_*`, `blusys_fs_*` APIs remain available for advanced use cases through the capability/service internals.
