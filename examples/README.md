# Examples

Examples are organized into three categories:

## quickstart/

Canonical starters (see `inventory.yml` for `interface` / `capabilities`): the same reducer and capability model you get from `blusys create`.

- **handheld_starter** — host-first handheld interactive (ST7735-oriented device path)
- **headless_telemetry** — headless-first connected reference

## reference/

Deeper references: capability-focused demos, surface coordinator apps, and framework validation builds.

- **connected_device** — interactive + connectivity capabilities
- **connected_headless** — headless + connectivity capabilities
- **framework_app_basic** — minimal interactive app for framework validation
- **framework_ui_basic** — stock widgets and bindings
- **framework_device_basic** — real device profile smoke build
- **surface_ops_panel**, **surface_gateway**, **headless_lan_control**, **handheld_bluetooth**, and other references (see `inventory.yml`)

## validation/

Internal stress tests, concurrency tests, and peripheral smoke tests. These are not part of the public learning surface — they exist to validate the platform on CI and real hardware.
