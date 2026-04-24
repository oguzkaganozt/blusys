# Adding a flow

Flows are UI behaviors you choose through `app_spec::flows`.

## What a flow is

A flow implements `flow_base` and registers screens during `start()`.

```cpp
class my_wizard_flow : public blusys::flows::flow_base {
public:
    void start(blusys::app_ctx &ctx) override {
        ctx.fx().nav.register_screen(RouteId::step1, my_step1_create);
        ctx.fx().nav.register_screen(RouteId::step2, my_step2_create);
    }
};
```

## Stock flows

| Flow | When to use |
|------|-------------|
| `settings_flow` | user-facing settings |
| `connectivity_flow` | connectivity capability present |
| `provisioning_flow` | Wi-Fi provisioning capability present |
| `ota_flow` | OTA capability present |
| `diagnostics_flow` | diagnostics capability present |
| `status_flow` | UI apps that need a default status screen |

`boot_flow` and `error_flow` are built in and always on.

## Use it

```cpp
static blusys::flows::settings_flow     kSettings{};
static blusys::flows::connectivity_flow  kConnect{};
static blusys::flows::flow_base *kFlows[] = {&kSettings, &kConnect};

spec.flows = kFlows;
spec.flow_count = std::size(kFlows);
```

## Next steps

- [App runtime model](app-runtime-model.md)
- [Capability composition](capability-composition.md)
