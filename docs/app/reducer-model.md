# Reducer Model

## At a glance

- **Idea:** one `update(ctx, state, action)` mutates your `State`; optional `on_event` turns external events into `Action`s.
- **Define** an `app_spec<State, Action>` with `update` plus hooks you need.
- **Next:** [App runtime model](app-runtime-model.md) for queues and threading.

The Blusys app model follows a reducer pattern: all state changes flow through a single `update()` function.

## Core Types

### `app_spec`

Defines the complete application:

```cpp
static const blusys::app_spec<State, Action> spec{
    .initial_state = {},      // initial value of State
    .update        = update,  // required: reducer function
    .on_init       = on_init, // optional: void(app_ctx&, app_fx&, State&) — UI setup for interactive apps
    .on_event      = on_event, // optional: std::optional<Action>(event, State&) — unified event hook
    .on_tick       = on_tick, // optional: void(app_ctx&, app_fx&, State&, uint32_t) — periodic callback
    .tick_period_ms = 100,    // tick interval; default is 10 ms
};
```

All fields except `update` are optional. The framework owns the runtime loop — you only fill in what your product needs.

### State

Your app state is a plain struct. The reducer mutates it in place:

```cpp
struct State {
    int counter = 0;
    bool connected = false;
    float temperature = 0.0f;
};
```

### Actions

Actions are the events that drive state changes:

```cpp
enum class Action {
    increment,
    decrement,
    reset,
    wifi_connected,
    wifi_disconnected,
    temp_reading,
};
```

### `app_ctx` and `app_fx`

The context object provided to your reducer is `app_ctx`. Use it for **dispatch**, **capability status**, and **feedback**. Typed navigation, overlays, shell/screen router, and ESP filesystem handles live on **`app_fx`**, reached as **`ctx.fx()`** or the `fx` hook parameter.

```cpp
ctx.dispatch(Action::increment);          // queue an action (returns false if the queue is full)
ctx.fx().nav.to(RouteId::settings);       // set root route
ctx.fx().nav.push(RouteId::detail);       // push route
ctx.fx().nav.back();                      // pop route
ctx.fx().nav.show_overlay(OverlayId::confirm); // show overlay
ctx.emit_feedback(                        // haptic / audio feedback
    blusys::feedback_channel::haptic,
    blusys::feedback_pattern::click);
```

## The Update Function

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

## Dispatch Lifecycle

1. A widget callback, event hook, or tick hook calls `ctx.dispatch(action)`
2. The framework queues the action
3. At each step the framework calls `update(ctx, state, action)` for each queued action
4. The reducer mutates state in place — no reactive framework, no subscriptions

## Event Hook (phase 4)

`on_event` is the unified hook. It sees framework intents and capability events as one stream and returns an optional action.

```cpp
std::optional<Action> on_event(blusys::event e, State &state)
{
    (void)state;
    switch (e.source) {
    case blusys::event_source::intent:
        switch (static_cast<blusys::intent>(e.kind)) {
        case blusys::intent::increment:
            return Action::increment;
        case blusys::intent::decrement:
            return Action::decrement;
        case blusys::intent::confirm:
            return Action::reset;
        default:
            return std::nullopt;
        }
    default:
        return std::nullopt;
    }
}
```

## Periodic Tick

For headless apps that need periodic work, `on_tick` runs at `tick_period_ms` intervals:

```cpp
void on_tick(blusys::app_ctx &ctx, blusys::app_fx &fx, State & /*state*/, std::uint32_t /*now_ms*/)
{
    (void)fx;
    ctx.dispatch(Action::temp_reading);
}
```

## Entry Points

```cpp
// Interactive app — host SDL or device LCD + input
BLUSYS_APP(spec)

// Headless app — no UI, terminal / ESP32
BLUSYS_APP_HEADLESS(spec)
```
