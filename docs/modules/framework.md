# Framework Status

`components/blusys_framework/` has been introduced as the third platform component, and only minimal infrastructure is shipped so far.

Intended direction:

- `core/` for product-level lifecycle, controller, routing, and feedback abstractions
- `ui/` for higher-level UI helpers above the `blusys_services` LVGL runtime

Current reality:

- the component exists and builds
- foundational headers exist for the umbrella include and fixed-capacity containers
- framework reference pages will be added once the first public API lands

For the current layering rules, see [Architecture](../architecture.md).
