# Framework Status

`components/blusys_framework/` has been introduced as the third platform component, and the first public framework-core contracts are now shipped.

Intended direction:

- `core/` for product-level lifecycle, controller, routing, and feedback abstractions
- `ui/` for higher-level UI helpers above the `blusys_services` LVGL runtime

Current reality:

- the component exists and builds
- foundational headers exist for the umbrella include and fixed-capacity containers
- framework core now exposes:
  - `blusys/framework/core/router.hpp`
  - `blusys/framework/core/intent.hpp`
  - `blusys/framework/core/feedback.hpp`
  - `blusys/framework/core/controller.hpp`
  - `blusys/framework/core/runtime.hpp`
- framework UI now exposes:
  - `blusys/framework/ui/theme.hpp`
  - `blusys/framework/ui/widgets.hpp`
- the umbrella header is `blusys/framework/framework.hpp`
- current examples: `examples/framework_core_basic/`, `examples/framework_ui_basic/`

For the current layering rules, see [Architecture](../architecture.md).
