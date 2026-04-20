# Buzzer

PWM-driven passive piezo buzzer with single-tone and melody-sequence playback.

## Quick Example

```c
#include "blusys/blusys.h"

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

This deadlocks — `close()` waits for the callback to return, but the callback is waiting for `close()`. Call `close()` from a separate task or after the callback has returned.

## Target Support

| Target    | Supported |
|-----------|-----------|
| ESP32     | yes       |
| ESP32-C3  | yes       |
| ESP32-S3  | yes       |

The buzzer driver wraps the PWM HAL, which is available on all three targets.

## Callback Context

`BLUSYS_BUZZER_EVENT_DONE` fires from the `esp_timer` task. Safe to call `printf`, FreeRTOS APIs, and most Blusys functions. **Do not call `blusys_buzzer_close()` from within this callback** — it will deadlock.

## Thread Safety

- all public functions are safe to call from any task
- an internal mutex serialises concurrent `play()`, `stop()`, and `set_callback()` calls
- a spinlock protects shared state with the timer callback

## ISR Notes

No ISR-safe calls are defined for the buzzer module.

## Limitations

- the array passed to `blusys_buzzer_play_sequence()` must remain valid until `BLUSYS_BUZZER_EVENT_DONE` fires
- max 4 handles — the PWM HAL supports up to 4 concurrent handles across all PWM-based modules
- one callback per handle
- `play()` requires `freq_hz > 0`; use `play_sequence()` with a rest note for silence inside a melody

## Example App

See `examples/reference/hal` (buzzer scenario).
