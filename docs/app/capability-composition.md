# Capability Composition

Wire capabilities in `main/`, map their events to actions, and keep the reducer in control of product behavior.

## Rules

- Keep config and capability lists in the product entrypoint or a tiny helper.
- Translate integration events in `on_event()`.
- Emit an `Action` when the capability changes product behavior.
- Use `ctx.fx()` for approved navigation and overlays, not raw route sinks.

## Typical stacks

| Shape | Common set |
|-------|------------|
| Interactive | `storage`, or `storage + connectivity + diagnostics` |
| Headless connected | `connectivity + telemetry`, often with `ota`, `storage`, and `lan_control` |
| Interactive coordinator | same as headless connected, plus local UI |

## Event mapping

Use typed helpers from `blusys::integration::*` when the event set grows. A typical bridge looks like this:

```cpp
std::optional<Action> on_event(blusys::event event, State &state)
{
    (void)state;
    if (event.source != blusys::event_source::integration) {
        return std::nullopt;
    }

    if (event.id == 0x0102u) {
        return Action::wifi_connected;
    }
    return std::nullopt;
}
```

## Flows

`blusys::flows::*` and `blusys::screens::*` are LVGL-only helpers. Drive work through `app_ctx::dispatch()` and keep raw service calls out of stock UI code.

## Next steps

- [Capabilities](capabilities.md)
- [Authoring a capability](capability-authoring.md)
- [Adding a flow](adding-a-flow.md)
