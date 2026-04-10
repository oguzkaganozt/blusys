# Migration Guide — V6 → V7

This guide walks product repos from the V6 three-tier model to the V7
`blusys::app` product model.

V7 is a **breaking reset** of the product-facing surface. The three-tier
architecture (HAL, services, framework) is unchanged underneath, but the
way product code interacts with the framework is completely different.
V6 products that only use HAL and services with no framework adoption
have the smallest migration. V6 products that use the controller-based
framework surface need a full rewrite of their app entry and wiring code.

If you are starting a new product, don't follow this guide — run
`blusys create --starter <headless|interactive>` instead. That scaffolds
a v7 `blusys::app` project ready to run.

## TL;DR — what changed

| V6 concept | V7 replacement |
|---|---|
| `controller` subclass with `init`/`handle`/`tick` | `update(ctx, state, action)` reducer function |
| `runtime.init(&controller, &route_sink, tick_ms)` | `BLUSYS_APP_MAIN_HOST(spec)` / `_HEADLESS` / `_DEVICE` entry macro |
| `runtime.post_intent(intent)` | `ctx.dispatch(action)` from app code; `map_intent()` for framework intents |
| `route_sink` subclass | `ctx.navigate_to()` / `navigate_push()` / `navigate_back()` / `show_overlay()` |
| `feedback_sink` subclass | `ctx.emit_feedback(channel, pattern)` |
| `blusys::framework::init()` + manual `lv_init()` | handled by entry macro |
| `blusys_ui_lock()` / `blusys_ui_unlock()` | handled by the app runtime |
| `lv_screen_load()` | handled by the app runtime |
| manual `blusys_lcd_open()` + `blusys_ui_open()` | `BLUSYS_APP_MAIN_DEVICE(spec, profile)` with a platform profile |
| manual Wi-Fi connect + SNTP + mDNS orchestration | `connectivity_capability` with config struct |
| manual SPIFFS / FAT mount | `storage_capability` with config struct |
| `#include "blusys/framework/framework.hpp"` | `#include "blusys/app/app.hpp"` |
| global mutable state + widget pointers | `State` struct owned by the runtime, mutated in `update()` |

## Scenario 1 — V6 framework app (controller-based) → V7 app model

This is the most common migration for products that adopted the V6
framework with controllers, route sinks, and the runtime spine.

### Step 1. Replace your app entry

**V6 pattern** — manual wiring in `app_main()`:

```cpp
#include "blusys/framework/framework.hpp"
#include "blusys/framework/ui/widgets.hpp"

// ... controller, route_sink, feedback_sink subclasses ...

blusys::framework::runtime g_runtime{};

extern "C" void app_main(void)
{
    blusys::framework::init();
    lv_init();
    blusys::ui::set_theme({...});

    // ... build UI, wire controller, wire sinks ...

    g_runtime.init(&controller, &route_sink, 10);

    // manual tick loop or rely on tasks
}
```

**V7 pattern** — declare a spec, use an entry macro:

```cpp
#include "blusys/app/app.hpp"

struct State { /* your product state */ };
enum class Action { /* your product actions */ };

void update(blusys::app::app_ctx &ctx, State &state, const Action &action)
{
    // handle each action, mutate state in place
}

static auto spec = blusys::app::app_spec<State, Action>{
    .initial_state = {},
    .update        = update,
};

BLUSYS_APP_MAIN_HOST(spec)   // or BLUSYS_APP_MAIN_DEVICE(spec, profile)
```

### Step 2. Move controller logic into the reducer

Your V6 `controller::handle()` method mapped intents to widget mutations
and route commands. In V7, this logic moves into `update()`:

| V6 (controller) | V7 (reducer) |
|---|---|
| `handle(event)` switches on `intent` | `update(ctx, state, action)` switches on `Action` |
| `submit_route(route::push(id))` | `ctx.navigate_push(id)` |
| `emit_feedback({channel, pattern})` | `ctx.emit_feedback(channel, pattern)` |
| direct widget mutation (`slider_set_value`) | mutate `state`, let view bindings reflect changes |

If your V6 controller used `init()` for setup, move that into `on_init`:

```cpp
static auto spec = blusys::app::app_spec<State, Action>{
    .initial_state = {},
    .update        = update,
    .on_init       = [](blusys::app::app_ctx &ctx, State &state) {
        // setup that ran in controller::init()
    },
};
```

### Step 3. Map framework intents to app actions

V6 controllers received raw `intent` values (confirm, increment, etc.).
V7 apps define their own `Action` enum and provide a `map_intent` bridge:

```cpp
bool map_intent(blusys::framework::intent intent, Action *out)
{
    switch (intent) {
    case blusys::framework::intent::confirm:   *out = Action::confirm;   return true;
    case blusys::framework::intent::increment: *out = Action::volume_up; return true;
    case blusys::framework::intent::decrement: *out = Action::volume_down; return true;
    default: return false;
    }
}

static auto spec = blusys::app::app_spec<State, Action>{
    .initial_state = {},
    .update        = update,
    .map_intent    = map_intent,
};
```

### Step 4. Remove manual route and feedback sinks

V6 required you to subclass `route_sink` and `feedback_sink`. V7 owns
both internally. Delete your sink classes and use `ctx` methods instead:

- `ctx.navigate_to(route_id)` — set root route
- `ctx.navigate_push(route_id)` — push onto stack
- `ctx.navigate_back()` — pop stack
- `ctx.show_overlay(id)` / `ctx.hide_overlay(id)` — overlay control
- `ctx.emit_feedback(channel, pattern, value)` — feedback

### Step 5. Update includes and CMakeLists

Replace:

```cpp
#include "blusys/framework/framework.hpp"
#include "blusys/framework/ui/widgets.hpp"
```

With:

```cpp
#include "blusys/app/app.hpp"
```

Your `CMakeLists.txt` `REQUIRES` line stays the same (`blusys_framework`
pulls in everything).

### Step 6. For device targets, add a platform profile

V6 required manual LCD and UI service setup. V7 uses profiles:

```cpp
#include "blusys/app/profiles/st7735.hpp"

static auto spec = blusys::app::app_spec<State, Action>{ ... };
BLUSYS_APP_MAIN_DEVICE(spec, blusys::app::st7735_profile())
```

Available profiles: `st7735.hpp`, `host.hpp`, `headless.hpp`.

## Scenario 2 — V6 headless app → V7 headless app

V6 headless products typically wired services manually (Wi-Fi connect,
SNTP sync, SPIFFS mount) inside `app_main()`. V7 replaces this with
capabilities and the same reducer model.

### Step 1. Define state and actions

```cpp
#include "blusys/app/app.hpp"
#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/app/capabilities/storage.hpp"

struct State {
    bool connected   = false;
    bool time_synced = false;
};

enum class Action {
    wifi_got_ip,
    wifi_disconnected,
    time_synced,
};
```

### Step 2. Write the reducer

```cpp
void update(blusys::app::app_ctx &ctx, State &state, const Action &action)
{
    switch (action) {
    case Action::wifi_got_ip:      state.connected = true;    break;
    case Action::wifi_disconnected: state.connected = false;   break;
    case Action::time_synced:       state.time_synced = true;  break;
    }
}
```

### Step 3. Replace manual service orchestration with capabilities

**V6** — manual wiring:

```cpp
extern "C" void app_main(void)
{
    blusys_wifi_config_t wifi_cfg = { .ssid = "...", .password = "..." };
    blusys_wifi_open(&wifi_cfg, &wifi);
    blusys_sntp_open(&sntp_cfg, &sntp);
    blusys_mdns_open(&mdns_cfg, &mdns);
    blusys_fs_open(&fs_cfg, &fs);
    // ... event handler registration, retry logic, etc.
}
```

**V7** — capability config + entry macro:

```cpp
static blusys::app::connectivity_capability conn{blusys::app::connectivity_config{
    .wifi_ssid     = "my_ssid",
    .wifi_password = "my_pass",
    .sntp_server   = "pool.ntp.org",
    .mdns_hostname = "my-device",
}};

static blusys::app::storage_capability stor{blusys::app::storage_config{
    .spiffs_base_path = "/fs",
}};

static blusys::app::capability_list capabilities{&conn, &stor};

static auto spec = blusys::app::app_spec<State, Action>{
    .initial_state = {},
    .update        = update,
    .map_event     = map_event,   // bridge capability events → app actions
    .capabilities  = &capabilities,
};

BLUSYS_APP_MAIN_HEADLESS(spec)
```

### Step 4. Bridge capability events to app actions

```cpp
bool map_event(std::uint32_t id, std::uint32_t, const void *, Action *out)
{
    using CE = blusys::app::connectivity_event;
    switch (static_cast<CE>(id)) {
    case CE::wifi_got_ip:       *out = Action::wifi_got_ip;      return true;
    case CE::wifi_disconnected: *out = Action::wifi_disconnected; return true;
    case CE::time_synced:       *out = Action::time_synced;      return true;
    default: return false;
    }
}
```

## Scenario 3 — V6 HAL/services-only app (no framework)

If your V6 product uses only HAL modules and services without the
framework tier, no migration is strictly required. Your `app_main.c` and
`REQUIRES blusys blusys_services` continue to work.

However, the recommended path for new development is `blusys::app`, even
for simple products. The benefits:

- structured state management through the reducer model
- capabilities instead of manual orchestration
- consistent product operating model across your product family

To adopt `blusys::app` incrementally:

1. Rename `app_main.c` to `app_main.cpp`.
2. Add `blusys_framework` to your `REQUIRES`.
3. Define a minimal `State`, `Action`, `update()`, and `app_spec`.
4. Wrap your entry in `BLUSYS_APP_MAIN_HEADLESS(spec)`.
5. Move service init code into capability configs or `on_init`.

## Version pinning

V7 requires all three tiers pinned to the same `v7.x.y` tag:

```yaml
dependencies:
  idf:
    version: ">=5.5"

  blusys:
    git: "https://github.com/oguzkaganozt/blusys.git"
    version: "v7.0.0"
    path: "components/blusys"
  blusys_services:
    git: "https://github.com/oguzkaganozt/blusys.git"
    version: "v7.0.0"
    path: "components/blusys_services"
  blusys_framework:
    git: "https://github.com/oguzkaganozt/blusys.git"
    version: "v7.0.0"
    path: "components/blusys_framework"
```

After editing `idf_component.yml`:

```bash
rm -rf managed_components build-*
blusys build . esp32s3
```

## Troubleshooting

**`error: 'blusys::app' has not been declared`** — your file is missing
`#include "blusys/app/app.hpp"` or `blusys_framework` is not in your
component `REQUIRES`.

**`undefined reference to blusys_app_platform::host_init`** — you used
`BLUSYS_APP_MAIN_HOST` but the host platform source is not linked. This
macro is for host builds only (`blusys host-build`), not ESP-IDF builds.

**`BLUSYS_APP_MAIN_DEVICE requires a profile`** — the device entry macro
takes two arguments: `(spec, profile)`. Include a profile header and pass
it, e.g. `BLUSYS_APP_MAIN_DEVICE(spec, blusys::app::st7735_profile())`.

**`map_intent` or `map_event` not being called** — ensure you assigned
the function pointer in your `app_spec`. If set to `nullptr` (the
default), intents and events are silently ignored.

**Widget code still using raw `lv_obj_t *` handles** — stock widgets
(`blusys::ui::button_create`, etc.) are still available in V7. The
change is in how you wire them: use `ctx.dispatch(action)` from callbacks
instead of posting intents to the runtime. Raw LVGL is allowed inside
custom widget implementations and explicit `lvgl_scope` blocks.

## Related references

- [App](../app/index.md) — the v7 product-facing API documentation
- [Reducer Model](../app/reducer-model.md) — state, actions, and `update()`
- [Views & Widgets](../app/views-and-widgets.md) — stock widgets and bindings
- [Capabilities](../app/capabilities.md) — connectivity and storage capabilities
- [Profiles](../app/profiles.md) — host, headless, and device platform profiles
- [V5 to V6 Migration](migration-v5-v6.md) — previous migration guide
