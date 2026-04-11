# Examples

Examples are organized into three categories:

## quickstart/

Canonical **archetype** starters: the same reducer and capability model you get from `blusys create`.

- **interactive_controller** — host-first interactive controller (ST7735-oriented device path)
- **edge_node** — headless-first connected archetype

## reference/

Deeper references: capability-focused demos, other archetypes, and framework validation builds.

- **connected_device** — interactive + connectivity capabilities
- **connected_headless** — headless + connectivity capabilities
- **framework_app_basic** — minimal interactive app for framework validation
- **framework_ui_basic** — stock widgets and bindings
- **framework_device_basic** — real device profile smoke build
- **interactive_panel**, **gateway**, and other archetype references (see `inventory.yml`)

## validation/

Internal stress tests, concurrency tests, and peripheral smoke tests. These are not part of the public learning surface — they exist to validate the platform on CI and real hardware.
