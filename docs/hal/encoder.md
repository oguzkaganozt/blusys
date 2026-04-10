# Encoder

Rotary encoder service with hardware quadrature decoding (PCNT on ESP32/S3) and GPIO interrupt fallback (ESP32-C3), plus optional push button.

## Quick Example

```c
#include "blusys/blusys.h"
#include "blusys/drivers/input/encoder.h"

static void on_event(blusys_encoder_t *enc, blusys_encoder_event_t event,
                      int position, void *ctx)
{
    (void)enc; (void)ctx;
    if (event == BLUSYS_ENCODER_EVENT_CW)    printf("CW  pos=%d\n", position);
    if (event == BLUSYS_ENCODER_EVENT_CCW)   printf("CCW pos=%d\n", position);
    if (event == BLUSYS_ENCODER_EVENT_PRESS) printf("pressed\n");
}

void app_main(void)
{
    blusys_encoder_t *enc;
    const blusys_encoder_config_t cfg = {
        .clk_pin          = 5,    /* change to your board's pins */
        .dt_pin           = 6,
        .sw_pin           = 7,    /* -1 to disable button */
        .glitch_filter_ns = 1000,
        .long_press_ms    = 1000,
    };

    blusys_encoder_open(&cfg, &enc);
    blusys_encoder_set_callback(enc, on_event, NULL);

    while (true) { vTaskDelay(portMAX_DELAY); }
}
```

## Common Mistakes

**Swapped CLK and DT** — if CW and CCW are reversed, swap the CLK and DT pin assignments in the config.

**Floating pins** — EC11 encoders need pull-ups on CLK, DT, and SW. The module enables internal pull-ups automatically. If using long wires, add external 10k pull-ups.

**Wrong `steps_per_detent`** — most EC11 encoders produce 4 quadrature pulses per detent (default). Some produce 2 or 1. If you get multiple events per click, increase this value. If you need to turn past a click, decrease it.

**Calling `blusys_encoder_close()` from the callback** — the close function waits for the in-flight callback to finish, which deadlocks if called from inside the callback.

## Target Support

| Target | Supported | Decoding |
|--------|-----------|----------|
| ESP32 | yes | PCNT hardware |
| ESP32-C3 | yes | GPIO software |
| ESP32-S3 | yes | PCNT hardware |

## Types

### `blusys_encoder_event_t`

```c
typedef enum {
    BLUSYS_ENCODER_EVENT_CW,
    BLUSYS_ENCODER_EVENT_CCW,
    BLUSYS_ENCODER_EVENT_PRESS,
    BLUSYS_ENCODER_EVENT_RELEASE,
    BLUSYS_ENCODER_EVENT_LONG_PRESS,
} blusys_encoder_event_t;
```

| Value | Meaning |
|-------|---------|
| `BLUSYS_ENCODER_EVENT_CW` | One detent clockwise |
| `BLUSYS_ENCODER_EVENT_CCW` | One detent counter-clockwise |
| `BLUSYS_ENCODER_EVENT_PRESS` | Push button pressed (after debounce) |
| `BLUSYS_ENCODER_EVENT_RELEASE` | Push button released (after debounce) |
| `BLUSYS_ENCODER_EVENT_LONG_PRESS` | Push button held for `long_press_ms` |

### `blusys_encoder_callback_t`

```c
typedef void (*blusys_encoder_callback_t)(blusys_encoder_t *encoder,
                                          blusys_encoder_event_t event,
                                          int position,
                                          void *user_ctx);
```

Callback signature for encoder events. `position` is the accumulated detent position at the time of the event. Fires from the `esp_timer` task — safe to call blusys APIs, `printf`, or FreeRTOS functions. Must not block indefinitely.

### `blusys_encoder_config_t`

```c
typedef struct {
    int      clk_pin;
    int      dt_pin;
    int      sw_pin;           /* -1 to disable button */
    uint32_t glitch_filter_ns; /* PCNT glitch filter (ESP32/S3 only), 0 = disabled */
    uint32_t debounce_ms;      /* Button debounce, 0 = default 50 ms */
    uint32_t long_press_ms;    /* Button long press, 0 = disabled */
    int      steps_per_detent; /* Pulses per mechanical detent, 0 = default 4 */
} blusys_encoder_config_t;
```

---

## Functions

### `blusys_encoder_open`

```c
blusys_err_t blusys_encoder_open(const blusys_encoder_config_t *config,
                                  blusys_encoder_t **out_encoder);
```

Configures GPIO pins, initializes the PCNT unit (or GPIO interrupt fallback), and optionally opens an internal button handle for the push button. Does not fire any callback until `blusys_encoder_set_callback()` is called.

**Parameters:**

- `config` — encoder configuration (required)
- `out_encoder` — output handle (required)

**Returns:** `BLUSYS_OK` on success, `BLUSYS_ERR_INVALID_ARG` if any pointer is NULL or pins are invalid, `BLUSYS_ERR_NO_MEM` if allocation fails.

---

### `blusys_encoder_close`

```c
blusys_err_t blusys_encoder_close(blusys_encoder_t *encoder);
```

Stops the PCNT unit (or GPIO interrupt), closes the button if present, waits for any in-flight callback to finish, then frees the handle.

**Parameters:**

- `encoder` — handle returned by `blusys_encoder_open()` (required)

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `encoder` is NULL.

---

### `blusys_encoder_set_callback`

```c
blusys_err_t blusys_encoder_set_callback(blusys_encoder_t *encoder,
                                          blusys_encoder_callback_t callback,
                                          void *user_ctx);
```

Registers (or replaces) the event callback. Pass `NULL` for `callback` to clear it. After this function returns, the old callback will not be called again.

**Parameters:**

- `encoder` — handle (required)
- `callback` — function to call on each event, or NULL to disable
- `user_ctx` — passed through to each callback invocation

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `encoder` is NULL, `BLUSYS_ERR_INVALID_STATE` if close is in progress.

---

### `blusys_encoder_get_position`

```c
blusys_err_t blusys_encoder_get_position(blusys_encoder_t *encoder,
                                          int *out_position);
```

Reads the accumulated detent position. CW increments, CCW decrements. Thread-safe.

**Parameters:**

- `encoder` — handle (required)
- `out_position` — output position value (required)

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if any pointer is NULL.

---

### `blusys_encoder_reset_position`

```c
blusys_err_t blusys_encoder_reset_position(blusys_encoder_t *encoder);
```

Resets the accumulated position to zero. Thread-safe.

**Parameters:**

- `encoder` — handle (required)

**Returns:** `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if `encoder` is NULL.

---

## Lifecycle

```
blusys_encoder_open()
    │
    ├── blusys_encoder_set_callback()   ← register event handler
    │
    ├── [events fire: CW / CCW / PRESS / RELEASE / LONG_PRESS]
    │
    ├── blusys_encoder_get_position()   ← poll accumulated position
    │
    ├── blusys_encoder_reset_position() ← reset to zero
    │
    └── blusys_encoder_close()
```

## Thread Safety

- Different encoder handles may be used concurrently from different tasks.
- `blusys_encoder_set_callback()` waits for any in-flight callback to complete before returning.
- `blusys_encoder_get_position()` and `blusys_encoder_reset_position()` are safe to call from any task at any time.
- Do **not** call `blusys_encoder_close()` from inside the encoder callback.

## Callback Context

Events fire from the **esp_timer task**, not from an ISR. It is safe to call `printf`, `ESP_LOGI`, FreeRTOS primitives, and most blusys APIs from inside the callback. Do not call blocking operations that wait indefinitely.

## Limitations

- One callback per handle. To fan out events to multiple consumers, forward from the single callback.
- The GPIO fallback (ESP32-C3) may miss edges at very high rotation speeds compared to the hardware PCNT path.
- `glitch_filter_ns` is only effective on ESP32 and ESP32-S3 (PCNT hardware); it is ignored on ESP32-C3.
- Position wraps at `int` limits.
- Long-press fires once per press. Repeated long-press events while held are not generated.

## Example App

See `examples/reference/encoder_basic/` for a runnable example.
