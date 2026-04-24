# App Runtime Model

This page shows how intents and capability events reach the reducer.

## At a glance

- Input bridges and capabilities emit events.
- `on_event()` optionally turns each event into one `Action`.
- Actions go through a fixed queue.
- `update()` consumes the queue and mutates state.

## Flow

```mermaid
flowchart LR
  UI[Intents from input bridges] --> E[on_event(event, state)]
  CAP[Integration events from capabilities] --> E
  E --> Q[Fixed-size action queue]
  Q --> U[update(ctx, state, action)]
  U --> V[view:: bindings / composites]
```

## Runtime rules

1. `ctx.dispatch(action)` enqueues work from widgets, flows, or product code.
2. If the queue is full, dispatch returns `false` and the action is dropped.
3. The runtime drains the queue and calls `update()` with a bounded step budget.
4. Product code should use `blusys::` UI helpers, not raw `lv_` calls.

## Threading

- Interactive builds run `on_event()` and `update()` on the framework/UI path.
- Headless builds use the same queue and reducer model without LVGL.
- Capabilities start before `on_init` and poll from the runtime step.

## Related reading

- [Reducer model](reducer-model.md)
- [Capability composition](capability-composition.md)
- [Validation and host loop](validation-host-loop.md)
