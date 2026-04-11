# Capability Composition

Capabilities are composed in `system/` and translated into app actions.

## Composition Rules

- keep capability config in `system/`
- keep product decisions in `logic/`
- keep rendering in `ui/`
- map capability events into reducer actions in `system/`
- query capability status through `app_ctx` or the app-facing status bridge

## Typical Stacks

### Interactive Controller

- storage
- provisioning
- connectivity when needed

### Interactive Panel

- connectivity
- storage
- diagnostics when needed

### Edge Node

- connectivity
- telemetry
- provisioning
- OTA
- diagnostics
- storage

### Gateway/Controller

- connectivity
- telemetry
- provisioning
- OTA
- diagnostics
- storage

## Rule Of Thumb

If a capability changes app behavior, emit an action.
If it only wires runtime services, keep it in `system/`.
