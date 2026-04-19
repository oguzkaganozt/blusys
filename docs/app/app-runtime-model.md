# App runtime model

This page summarizes how intents, integration events, and the action queue connect to your reducer. Implementation details live in `components/blusys/include/blusys/framework/app/internal/app_runtime.hpp` (framework-internal; product code uses `blusys/framework/app/app.hpp` and related public headers).

## Flow

```mermaid
flowchart LR
  subgraph inputs [Inputs]
    UI[Intents from input bridges]
    CAP[Integration events from capabilities]
  end
  E[on_event(event, state)]
  Q[Fixed-size action queue]
  U["update(ctx, state, action)"]
  V[view:: bindings / composites from update]
  UI --> E
  CAP --> E
  E --> Q
  Q --> U
  U --> V
```

1. **Intents** and **integration events** run through `on_event()` on the framework thread when the runtime drains its event queue. Each event may enqueue one app `Action`.
2. **`ctx.dispatch(action)`** (from widgets, flows, or your own code) enqueues into the same **action queue**. It returns `false` if the queue is full (the action is dropped and a warning is logged). **`ctx.action_queue_drop_count()`** returns how many drops have occurred since runtime init (monotonic counter)—use this for overload debugging or diagnostics surfaces.
3. The runtime repeatedly **pops** actions and calls **`update(ctx, state, action)`** with a bounded iteration count per step to avoid unbounded recursion.
4. Inside `update()`, mutate LVGL only through **`blusys::`** UI helpers — not raw `lv_` calls that take locks or talk to the framework runtime.

## Threading and lifecycle

- **Interactive:** intents and integration events are processed in the same context as the LVGL tick path configured by the framework; `on_event()` and `update()` therefore run with the UI conventions your build expects (typically LVGL locked when required by the entry path).
- **Headless:** the same queue + `update()` model applies; there is no LVGL.

Capabilities are **started** before `on_init` and **polled** from the runtime step; integration events they post use the reserved `event_id` ranges documented in `blusys/framework/capabilities/` and the typed helpers in `blusys/framework/app/variant_dispatch.hpp`.

## Related reading

- [Reducer model](reducer-model.md) — `app_spec`, state, and actions
- [Guidelines — product application shape](../internals/guidelines.md#product-application-cpp-shape) — conventions for routes, view sync, integration
