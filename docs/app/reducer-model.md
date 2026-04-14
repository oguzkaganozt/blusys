# Reducer Model

The Blusys app model follows a reducer pattern: all state changes flow through a single `update()` function.

## Core Types

### `app_spec`

Defines the complete application:

```cpp
static const blusys::app_spec<State, Action> spec{
    .initial_state = {},      // initial value of State
    .update        = update,  // required: reducer function
    .on_init       = on_init, // optional: void(app_ctx&, app_services&, State&) — UI setup for interactive apps
    .map_intent    = map_intent, // optional: bool(app_services&, intent, Action*) — encoder/keyboard → Action
    .on_tick       = on_tick, // optional: void(app_ctx&, app_services&, State&, uint32_t) — periodic callback
    .tick_period_ms = 100,    // tick interval (default 100 ms)
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

### `app_ctx` and `app_services`

The context object provided to your reducer is `app_ctx`. Use it for **dispatch**, **capability status**, and **feedback**. Routing, overlays, shell/screen router, and ESP filesystem handles live on **`app_services`**, reached as **`ctx.services()`** (same accessor from `update`, `on_init`, and other hooks).

```cpp
ctx.dispatch(Action::increment);          // queue an action (returns false if the queue is full)
ctx.services().navigate_to(RouteId::settings);   // set root route
ctx.services().navigate_push(RouteId::detail);   // push route
ctx.services().navigate_back();                  // pop route
ctx.services().show_overlay(OverlayId::confirm); // show overlay
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

1. A widget callback, intent map, or tick hook calls `ctx.dispatch(action)`
2. The framework queues the action
3. At each step the framework calls `update(ctx, state, action)` for each queued action
4. The reducer mutates state in place — no reactive framework, no subscriptions

## Intent Map (encoder / keyboard input)

For interactive apps, `map_intent` bridges framework intents (from encoder or keyboard) to product actions:

```cpp
bool map_intent(blusys::app_services &svc, blusys::intent intent, Action *out)
{
    (void)svc; // use when navigation or UI from intents is needed
    switch (intent) {
    case blusys::intent::increment:
        *out = Action::increment;
        return true;
    case blusys::intent::decrement:
        *out = Action::decrement;
        return true;
    case blusys::intent::confirm:
        *out = Action::reset;
        return true;
    default:
        return false;
    }
}
```

## Periodic Tick

For headless apps that need periodic work, `on_tick` runs at `tick_period_ms` intervals:

```cpp
void on_tick(blusys::app_ctx &ctx, blusys::app_services &svc, State & /*state*/, std::uint32_t /*now_ms*/)
{
    (void)svc;
    ctx.dispatch(Action::temp_reading);
}
```

## Entry Points

```cpp
// Host-first interactive — SDL2 window on PC
BLUSYS_APP_MAIN_HOST(spec)

// Host with explicit window size and title
BLUSYS_APP_MAIN_HOST_PROFILE(spec, blusys::host_profile{
    .hor_res = 320, .ver_res = 240, .title = "My App"
})

// Device interactive — LCD + optional encoder
BLUSYS_APP_MAIN_DEVICE(spec, profile)

// Headless — no UI, terminal / ESP32
BLUSYS_APP_MAIN_HEADLESS(spec)
```
