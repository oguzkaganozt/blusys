# Buzzer

PWM-driven passive piezo buzzer with single-tone and melody-sequence playback.

## Quick Example

```c
#include "blusys/drivers/actuator/buzzer.h"

static const blusys_buzzer_note_t s_jingle[] = {
    { .freq_hz = 262, .duration_ms = 200 },  /* C4 */
    { .freq_hz =   0, .duration_ms =  80 },  /* rest */
    { .freq_hz = 392, .duration_ms = 400 },  /* G4 */
};

static void on_done(blusys_buzzer_t *bz, blusys_buzzer_event_t e, void *ctx)
{
    if (e == BLUSYS_BUZZER_EVENT_DONE) {
        printf("sequence finished\n");
    }
}

void app_main(void)
{
    blusys_buzzer_t *bz;
    const blusys_buzzer_config_t cfg = { .pin = 8, .duty_permille = 500 };

    blusys_buzzer_open(&cfg, &bz);
    blusys_buzzer_set_callback(bz, on_done, NULL);
    blusys_buzzer_play_sequence(bz, s_jingle, 3);

    vTaskDelay(pdMS_TO_TICKS(3000));
    blusys_buzzer_close(bz);
}
```

## Common Mistakes

**Using an active buzzer**

Active buzzers produce a fixed tone when driven HIGH — they cannot play different frequencies. Verify the component is passive (usually labelled "PASSIVE" or has no oscillator circuit).

**Stack-allocated notes array for `play_sequence()`**

```c
/* WRONG — notes goes out of scope before the sequence finishes */
void start_melody(blusys_buzzer_t *bz)
{
    blusys_buzzer_note_t notes[] = { {440, 200}, {880, 200} };
    blusys_buzzer_play_sequence(bz, notes, 2);
}  /* notes is freed here — undefined behaviour during playback */
```

Declare the array as `static` or at a scope that outlives the sequence:

```c
static const blusys_buzzer_note_t s_notes[] = { {440, 200}, {880, 200} };
blusys_buzzer_play_sequence(bz, s_notes, 2);
```

**Calling `close()` from within the DONE callback**

This deadlocks — `close()` waits for the callback to return, but the callback is waiting for `close()`. Always call `close()` from a separate task or after the callback has returned.

**Forgetting to open before play**

`blusys_buzzer_play()` returns `BLUSYS_ERR_INVALID_ARG` if `buzzer` is NULL. Check the return value of `blusys_buzzer_open()`.

## Target Support

| Target    | Supported |
|-----------|-----------|
| ESP32     | yes       |
| ESP32-C3  | yes       |
| ESP32-S3  | yes       |

The buzzer service wraps the PWM HAL, which is available on all three targets.

## Types

### `blusys_buzzer_note_t`

A single note in a melody sequence. Set `freq_hz` to `0` for a rest (silence).

```c
typedef struct {
    uint32_t freq_hz;     /* Tone frequency in Hz; 0 = rest (silence) */
    uint32_t duration_ms; /* Duration in milliseconds */
} blusys_buzzer_note_t;
```

### `blusys_buzzer_event_t`

```c
typedef enum {
    BLUSYS_BUZZER_EVENT_DONE, /* Tone or sequence finished naturally */
} blusys_buzzer_event_t;
```

### `blusys_buzzer_callback_t`

```c
typedef void (*blusys_buzzer_callback_t)(blusys_buzzer_t *buzzer,
                                          blusys_buzzer_event_t event,
                                          void *user_ctx);
```

Fired from the `esp_timer` task. Safe to call `printf`, FreeRTOS APIs, and most blusys functions. **Do not call `blusys_buzzer_close()` from within this callback** — it will deadlock.

### `blusys_buzzer_config_t`

```c
typedef struct {
    int      pin;            /* GPIO pin connected to the buzzer signal */
    uint16_t duty_permille;  /* PWM duty cycle 0–1000; 0 defaults to 500 (50%) */
} blusys_buzzer_config_t;
```

## Functions

### `blusys_buzzer_open`

```c
blusys_err_t blusys_buzzer_open(const blusys_buzzer_config_t *config,
                                 blusys_buzzer_t **out_buzzer);
```

Opens a buzzer on the given GPIO pin. Internally opens a PWM channel. The buzzer is silent until `blusys_buzzer_play()` or `blusys_buzzer_play_sequence()` is called.

| Parameter    | Description                                   |
|--------------|-----------------------------------------------|
| `config`     | Non-null pointer to configuration struct      |
| `out_buzzer` | Receives the opaque handle on success         |

| Return value            | Condition                                |
|-------------------------|------------------------------------------|
| `BLUSYS_OK`             | Success                                  |
| `BLUSYS_ERR_INVALID_ARG`| `config` or `out_buzzer` is NULL         |
| `BLUSYS_ERR_NO_MEM`     | Allocation failed                        |
| `BLUSYS_ERR_BUSY`       | No free PWM channels (max 4 concurrent)  |
| other                   | Underlying PWM or timer error            |

### `blusys_buzzer_close`

```c
blusys_err_t blusys_buzzer_close(blusys_buzzer_t *buzzer);
```

Stops playback, releases the PWM channel and all associated resources. Blocks until any in-flight callback has finished. Must not be called from within the buzzer callback.

| Return value             | Condition             |
|--------------------------|-----------------------|
| `BLUSYS_OK`              | Always on valid input |
| `BLUSYS_ERR_INVALID_ARG` | `buzzer` is NULL      |

### `blusys_buzzer_set_callback`

```c
blusys_err_t blusys_buzzer_set_callback(blusys_buzzer_t *buzzer,
                                         blusys_buzzer_callback_t callback,
                                         void *user_ctx);
```

Registers the event callback. Pass `NULL` to clear. Waits for any in-flight callback to complete before returning.

| Return value              | Condition              |
|---------------------------|------------------------|
| `BLUSYS_OK`               | Success                |
| `BLUSYS_ERR_INVALID_ARG`  | `buzzer` is NULL       |
| `BLUSYS_ERR_INVALID_STATE`| Handle is closing      |

### `blusys_buzzer_play`

```c
blusys_err_t blusys_buzzer_play(blusys_buzzer_t *buzzer,
                                 uint32_t freq_hz,
                                 uint32_t duration_ms);
```

Plays a single tone asynchronously. Cancels any active tone or sequence and starts immediately. `BLUSYS_BUZZER_EVENT_DONE` fires when `duration_ms` elapses.

| Parameter    | Description                                    |
|--------------|------------------------------------------------|
| `freq_hz`    | Tone frequency in Hz; must be > 0              |
| `duration_ms`| Tone duration in milliseconds; must be > 0     |

| Return value              | Condition                       |
|---------------------------|---------------------------------|
| `BLUSYS_OK`               | Tone started                    |
| `BLUSYS_ERR_INVALID_ARG`  | NULL buzzer, zero freq or duration |
| `BLUSYS_ERR_INVALID_STATE`| Handle is closing               |

### `blusys_buzzer_play_sequence`

```c
blusys_err_t blusys_buzzer_play_sequence(blusys_buzzer_t *buzzer,
                                          const blusys_buzzer_note_t *notes,
                                          size_t count);
```

Plays a sequence of notes asynchronously. Cancels any active playback and starts immediately. `BLUSYS_BUZZER_EVENT_DONE` fires once after the last note finishes.

!!! warning "Caller owns the notes array"
    The `notes` pointer must remain valid until `BLUSYS_BUZZER_EVENT_DONE` fires. Do not pass a stack-allocated array that may go out of scope before the sequence finishes.

| Return value              | Condition                        |
|---------------------------|----------------------------------|
| `BLUSYS_OK`               | Sequence started                 |
| `BLUSYS_ERR_INVALID_ARG`  | NULL buzzer, notes, or count = 0 |
| `BLUSYS_ERR_INVALID_STATE`| Handle is closing                |

### `blusys_buzzer_stop`

```c
blusys_err_t blusys_buzzer_stop(blusys_buzzer_t *buzzer);
```

Stops playback immediately. Does not fire `BLUSYS_BUZZER_EVENT_DONE`. Safe to call when idle (no-op).

| Return value              | Condition              |
|---------------------------|------------------------|
| `BLUSYS_OK`               | Success                |
| `BLUSYS_ERR_INVALID_ARG`  | `buzzer` is NULL       |
| `BLUSYS_ERR_INVALID_STATE`| Handle is closing      |

### `blusys_buzzer_is_playing`

```c
blusys_err_t blusys_buzzer_is_playing(blusys_buzzer_t *buzzer, bool *out_playing);
```

Returns the current playback state.

| Return value              | Condition                         |
|---------------------------|-----------------------------------|
| `BLUSYS_OK`               | `*out_playing` set                |
| `BLUSYS_ERR_INVALID_ARG`  | `buzzer` or `out_playing` is NULL |

## Lifecycle

```
blusys_buzzer_open()
    └─ blusys_buzzer_set_callback()   (optional)
    └─ blusys_buzzer_play()           (or play_sequence)
           └─ ... BLUSYS_BUZZER_EVENT_DONE fires ...
    └─ blusys_buzzer_stop()           (optional early abort)
blusys_buzzer_close()
```

Calling `play()` or `play_sequence()` while already playing cancels the current playback and restarts immediately. `stop()` aborts silently without a DONE event.

## Thread Safety

All public functions are safe to call from any task. The handle uses a FreeRTOS mutex to serialise concurrent `play()`, `stop()`, and `set_callback()` calls, and a spinlock to protect shared state with the timer callback.

## Callback Context

The `BLUSYS_BUZZER_EVENT_DONE` callback fires from the `esp_timer` task. It is safe to call `printf`, FreeRTOS APIs, and most blusys functions from within it. **Do not call `blusys_buzzer_close()` from within the callback** — it will deadlock waiting for the callback to return.

## Limitations

- **`notes` lifetime**: the array passed to `blusys_buzzer_play_sequence()` must remain valid until `BLUSYS_BUZZER_EVENT_DONE` fires.
- **Max 4 handles**: the PWM HAL supports a maximum of 4 concurrent handles across all PWM-based modules.
- **No close from callback**: calling `blusys_buzzer_close()` from within the buzzer callback deadlocks.
- **One callback per handle**: only one callback can be registered at a time.
- **`play()` requires freq_hz > 0**: use `play_sequence()` with a rest note for silence within a melody.
