# Reducer Model

One `update(ctx, state, action)` function owns state changes. Everything else feeds it.

## At a glance

- `app_spec<State, Action>` wires the reducer and any optional hooks.
- `State` is a plain struct mutated in place.
- `Action` is the product vocabulary that drives behavior.
- `on_event()` translates framework/capability events into actions.
- `on_tick()` handles periodic headless work when needed.

## `app_spec`

```cpp
static const blusys::app_spec<State, Action> spec{
    .initial_state = {},
    .update        = update,
    .on_init       = on_init,
    .on_event      = on_event,
    .on_tick       = on_tick,
    .tick_period_ms = 100,
};
```

## State and actions

```cpp
struct State {
    int counter = 0;
    bool connected = false;
    float temperature = 0.0f;
};

enum class Action {
    increment,
    decrement,
    reset,
    wifi_connected,
    wifi_disconnected,
    temp_reading,
};
```

## Context and effects

`app_ctx` handles dispatch, capability status, and feedback. `app_fx` exposes typed navigation, overlays, and filesystem handles.

```cpp
ctx.dispatch(Action::increment);
ctx.fx().nav.to(RouteId::settings);
ctx.fx().nav.push(RouteId::detail);
ctx.fx().nav.show_overlay(OverlayId::confirm);
ctx.emit_feedback(blusys::feedback_channel::haptic,
                  blusys::feedback_pattern::click);
```

## Update

```cpp
void update(blusys::app_ctx &ctx, State &state, const Action &action)
{
    switch (action) {
    case Action::increment:
        ++state.counter;
        ctx.emit_feedback(blusys::feedback_channel::haptic,
                          blusys::feedback_pattern::click);
        break;
    case Action::decrement:
        --state.counter;
        break;
    case Action::reset:
        state.counter = 0;
        break;
    default:
        break;
    }
}
```

## Event hook

`on_event()` sees intents and integration events as one stream. Return `std::optional<Action>` and keep the reducer free of raw event IDs.

```cpp
std::optional<Action> on_event(blusys::event e, State &state)
{
    (void)state;
    if (e.source != blusys::event_source::intent) {
        return std::nullopt;
    }

    switch (static_cast<blusys::intent>(e.kind)) {
    case blusys::intent::increment: return Action::increment;
    case blusys::intent::decrement: return Action::decrement;
    case blusys::intent::confirm:   return Action::reset;
    default:                        return std::nullopt;
    }
}
```

## Periodic work

```cpp
void on_tick(blusys::app_ctx &ctx, blusys::app_fx &, State &, std::uint32_t)
{
    ctx.dispatch(Action::temp_reading);
}
```

## Entry points

```cpp
BLUSYS_APP(spec)
BLUSYS_APP_HEADLESS(spec)
```

## Next steps

- [App runtime model](app-runtime-model.md)
- [Views & widgets](views-and-widgets.md)
- [Capabilities](capabilities.md)
