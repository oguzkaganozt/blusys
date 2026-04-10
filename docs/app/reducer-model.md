# Reducer Model

The Blusys app model follows a reducer pattern: all state changes flow through a single `update()` function.

## Core Types

### `app_spec`

Defines the complete application:

```cpp
auto spec = blusys::app::app_spec{
    .name = "my_product",
    .init_state = MyState{},
    .update = my_update,
    .screens = { &home_screen, &settings_screen },
};
```

### State

Your app state is a plain struct. The reducer mutates it in place:

```cpp
struct MyState {
    int counter = 0;
    bool connected = false;
    float temperature = 0.0f;
};
```

### Actions

Actions are the events that drive state changes:

```cpp
enum class MyAction {
    increment,
    decrement,
    reset,
    wifi_connected,
    wifi_disconnected,
    temp_reading,
};
```

### `app_ctx`

The context object provided to your reducer for dispatch, navigation, effects, and framework services:

```cpp
ctx.dispatch(MyAction::increment);
ctx.navigate("/settings");
ctx.effect(my_timer_effect);
```

## The Update Function

```cpp
void my_update(blusys::app::app_ctx& ctx, MyState& state, MyAction action) {
    switch (action) {
        case MyAction::increment:
            state.counter++;
            break;
        case MyAction::decrement:
            state.counter--;
            break;
        case MyAction::reset:
            state.counter = 0;
            break;
        default:
            break;
    }
}
```

## Dispatch Lifecycle

1. A widget callback or effect calls `ctx.dispatch(action)`
2. The framework queues the action
3. The framework calls `update(ctx, state, action)` on the main loop
4. The reducer mutates state in place
5. The framework triggers view updates for bound widgets

## Effects

Effects are the mechanism for async operations (timers, service callbacks, I/O):

```cpp
ctx.effect([](blusys::app::app_ctx& ctx) {
    // Start a periodic timer that dispatches actions
    ctx.dispatch(MyAction::temp_reading);
});
```

## Entry Points

```cpp
// Host-first interactive
BLUSYS_APP_HOST_MAIN(spec);

// Device interactive
BLUSYS_APP_MAIN(spec);

// Headless (no UI)
BLUSYS_APP_HEADLESS_MAIN(spec);
```
