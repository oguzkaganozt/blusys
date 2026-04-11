# Archetype Starters

Blusys ships four canonical archetype starting points built on the same `blusys::app` operating model.

Use them as product starting shapes, not framework branches.

Create them with `blusys create --archetype <interactive-controller|interactive-panel|edge-node|gateway-controller>`.

## Interactive Controller

Use the controller archetype when your product is:

- compact and interaction-led
- encoder-first or button-first
- centered on one primary control loop
- visually expressive and tactile
- biased toward setup, settings, status, and compact control flows

Canonical example:

- `examples/quickstart/interactive_controller/`

## Interactive Panel

Use the panel archetype when your product is:

- dashboard-led or status-led
- denser and more operational
- focused on monitoring, diagnostics, and local supervision
- still local and interactive, but less character-driven than the controller

Canonical example:

- `examples/reference/interactive_panel/`

## Edge Node

Use the edge node archetype when your product is:

- headless-first
- connectivity-led with telemetry or OTA
- built around provisioning, recovery, and status over control surface
- operationally minimal when healthy

Canonical example:

- `examples/quickstart/edge_node/`

## Gateway/Controller

Use the gateway/controller archetype when your product is:

- headless by default with an optional local operator surface
- orchestration-led or coordinator-led
- focused on diagnostics, maintenance, and connectivity management
- operationally clear at a glance

The default starter is headless-first. Use `blusys create --archetype gateway-controller --starter interactive` for the local UI variant.

Canonical example:

- `examples/reference/gateway/`

## Shared Model

All four archetypes keep the same ownership rules:

- `logic/` owns state, actions, and reducer behavior
- `ui/` owns screens and rendering
- `system/` owns capabilities, profiles, and wiring

All four also keep the same runtime model:

- `update(ctx, state, action)` reducer
- framework-owned shell and navigation
- framework-owned profile bring-up
- capability events mapped into app actions

## Next Pages

- [Interactive Quickstart](quickstart-interactive.md)
- [App](../app/index.md)
- [Capabilities](../app/capabilities.md)
- [Profiles](../app/profiles.md)
