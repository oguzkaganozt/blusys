# Framework Status

The `blusys_framework` component now exists in the repo as part of the platform transition, and the first framework-core contracts are available.

Planned scope:

- framework core for product lifecycle, controllers, intents, routing, and feedback
- optional UI layer for higher-level product UI helpers and widget-kit code

Current state:

- the component is present in the build graph
- the framework umbrella header is `blusys/framework/framework.hpp`
- the current core headers are:
  - `core/router.hpp`
  - `core/intent.hpp`
  - `core/feedback.hpp`
  - `core/controller.hpp`
  - `core/runtime.hpp`
- the first UI header is `ui/theme.hpp`, which provides `theme_tokens`, `set_theme()`, and `theme()`
- the first layout-primitives umbrella is `ui/widgets.hpp`
- `examples/framework_core_basic/` shows the current minimal usage pattern: create a controller, initialize a runtime, register feedback sinks, post normalized intents, and let the runtime own route delivery and tick cadence
- `examples/framework_ui_basic/` shows the current UI-side usage pattern: set theme tokens, create a screen, compose rows/columns, and add labels/dividers

What is still missing:

- product-facing routing integration with real screens
- interactive widgets and richer UI helper APIs

For the current repo architecture, see [Architecture](../architecture.md).
