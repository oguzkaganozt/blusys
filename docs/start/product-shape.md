# Product shape (schema, interface, capabilities, profile, policies)

Start from `blusys.project.yml`: new Blusys products are described by `schema`, `interface`, `capabilities`, `profile`, and `policies`, not by named starter bundles. The scaffold and checked-in `blusys.project.yml` use the same vocabulary.

## Grammar

```bash
blusys create [--interface interactive|headless] [--with cap1,cap2,...] [--policy policy1,...] [path]
blusys create --list
```

- **`--interface`** (single): how the product presents itself locally - UI on/off, host build defaults.
- **`--with`** (multi): [framework capabilities](../app/capabilities.md) - connectivity, storage, telemetry, OTA, and the rest (see `--list`).
- **`profile`** (single): built-in device/profile factory name; `null` uses the interface default.
- **`--policy`** (multi): cross-cutting behavior that is **not** a runtime capability - e.g. `low_power`.

Defaults: **`interactive`**, empty capabilities, `null` profile, empty policies. `blusys create` with no flags uses that default shape.

## Interfaces

| Interface | Role | Typical use |
|-----------|------|-------------|
| **interactive** | Local UI enabled, encoder-friendly | Small displays, operator panels, settings |
| **headless** | No local UI by default; headless entry | Telemetry, gateways, invisible-when-healthy devices |

Interactive and headless starters stay reducer-first through `update(ctx, state, action)`; interactive products add `main/ui/` only when needed.

## Reference examples in this repo

The quickstart starters were removed during the manifest-first scaffold rewrite. The starter pages on the next links describe the target shape; `examples/reference/` and `examples/validation/` remain as the deeper demo and smoke layers.

| Direction | Example path | Notes |
|-----------|--------------|-------|
| Display + LVGL | `examples/reference/display/` | LCD / UI / encoder / OLED scenarios (menuconfig) |
| Connected clients | `examples/reference/connectivity/` | Wi-Fi / HTTP / MQTT scenarios (menuconfig) |
| HAL demos | `examples/reference/hal/` | GPIO, PWM, button, timer, NVS, ADC, SPI, I2C, UART |
| Low-power telemetry | `examples/validation/headless_telemetry_low_power/` | Internal validation build |

For historical minimal connectivity demos (kept for regression), see `examples/validation/connected_device/` and `examples/validation/connected_headless/`. New projects should start from the manifest-first starter docs instead of in-tree quickstart templates.

## Capabilities and policies

- **Capabilities** are composed in the product code behind the manifest; the reducer receives work via **actions**; capability events are handled in the product event path (see [Capability composition](../app/capability-composition.md)).
- **Profiles** are named C++ factories selected by the manifest's `profile` field. Leave it `null` for the interface default.
- **Policies** adjust defaults and integration overlays (e.g. power-related `sdkconfig` fragments) without adding a new capability kind.

Dependency, target, and profile/interface compatibility rules (e.g. `telemetry` requires `connectivity`, `usb` targets ESP32-S3) are enforced by the generator and manifest validator.

## Build layout for generated projects

`blusys create` writes a top-level `CMakeLists.txt` that sets `EXTRA_COMPONENT_DIRS`
to the Blusys `components/` tree from the checkout used at generation time (embedded
path). Managed dependencies (ESP-IDF component registry, LVGL, etc.) stay in
`main/idf_component.yml`. If you move the project or build outside the original tree,
update `EXTRA_COMPONENT_DIRS` or vendor the platform components - see
[Architecture](../internals/architecture.md) (consumption models).

## Rules

- No raw LVGL in normal product screens - use `blusys::` UI helpers, stock widgets, and the custom widget contract.
- Keep the event bridge thin: profiles, capability list, `on_event`, bridges; heavy logic stays in application code.

## Advanced

### Override in code

The generated spec lives in `build/generated/blusys_app_spec.h`. If you need a one-off shape change, copy it or hand-write your own `AppSpec` in `app_main.cpp` and pass it to `blusys::run`. The manifest validator still runs, so the repo stays schema-driven.

```cpp
#include "blusys_app_spec.h"

extern "C" void app_main() {
    auto spec = blusys::generated::kAppSpec;
    blusys::run(spec);
}
```

## Next pages

- [Interactive quickstart](quickstart-interactive.md)
- [Headless quickstart](quickstart-headless.md)
- [App](../app/index.md)
- [Capabilities](../app/capabilities.md)
- [Profiles](../app/profiles.md)
