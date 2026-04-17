# Adding a flow

Flows are spec-selectable UI behaviors (settings, connectivity, OTA, …). They are not hard-wired framework internals — you pick them via `app_spec::flows`.

## What a flow is

A flow is an object implementing `flow_base`:

```cpp
// blusys/framework/flows/flow_base.hpp
class flow_base {
public:
    virtual void start(blusys::app_ctx &ctx) = 0;  // called before on_init
    virtual void stop() {}                          // called on deinit
};
```

`start()` registers the flow's screens with the navigation controller. By the time `on_init` fires, the flow's screens are navigable.

## Stock flows

| Flow | Class | When to use |
|------|-------|-------------|
| `settings_flow` | `blusys/framework/flows/settings.hpp` | Any app with user-facing settings |
| `connectivity_flow` | `blusys/framework/flows/connectivity_flow.hpp` | When a connectivity capability is present |
| `provisioning_flow` | `blusys/framework/flows/provisioning_flow.hpp` | When `wifi_prov` capability is present |
| `ota_flow` | `blusys/framework/flows/ota_flow.hpp` | When `ota` capability is present |
| `diagnostics_flow` | `blusys/framework/flows/diagnostics_flow.hpp` | When `diagnostics` capability is present |
| `status_flow` | `blusys/framework/flows/status.hpp` | Default-on for apps with UI |
| `boot_flow` | `blusys/framework/flows/boot.hpp` | Framework built-in; auto-included |
| `error_flow` | `blusys/framework/flows/error.hpp` | Framework built-in; always on |
| `loading_flow` | `blusys/framework/flows/loading.hpp` | Used internally by other flows |

`boot_flow` and `error_flow` are framework built-ins that run regardless of spec. All others must be listed explicitly in `spec.flows`.

## Using stock flows

In `platform/app_main.cpp`:

```cpp
static blusys::flows::settings_flow    kSettings{};
static blusys::flows::connectivity_flow kConnect{};

static blusys::flows::flow_base *kFlows[] = {&kSettings, &kConnect};

spec.flows      = kFlows;
spec.flow_count = std::size(kFlows);
```

Activation is conditional: `connectivity_flow` is a no-op if no connectivity capability is in `spec.capabilities`. Declare both; the framework handles the rest.

## Writing a custom flow

Subclass `flow_base`, register screens in `start()`, and add it to `spec.flows`:

```cpp
class my_wizard_flow : public blusys::flows::flow_base {
public:
    void start(blusys::app_ctx &ctx) override {
        // register screens with navigation_controller via ctx
        ctx.nav().register_route(blusys::route::my_wizard_step1, my_step1_create);
        ctx.nav().register_route(blusys::route::my_wizard_step2, my_step2_create);
    }
};
```

Keep flow constructors lightweight. Heavy state init belongs in `start()`.

## See also

- [App runtime model](app-runtime-model.md) — when flows start relative to on_init
- [Capability composition](capability-composition.md) — pairing flows with capabilities
- `components/blusys/include/blusys/framework/flows/` — all flow headers
