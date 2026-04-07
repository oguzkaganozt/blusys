# Play Tones on a Buzzer

## Problem Statement

A passive piezo buzzer produces sound when driven by a PWM signal — the frequency determines the pitch. This guide shows how to play single tones and multi-note sequences with the `buzzer` service module.

!!! note "Passive vs active buzzer"
    A **passive** piezo requires an oscillating signal (PWM) to produce sound. An **active** buzzer has a built-in oscillator and only needs a GPIO high/low — it will only emit a fixed-frequency beep and cannot be used with this module.

## Prerequisites

- Any supported board (ESP32, ESP32-C3, or ESP32-S3)
- A passive piezo buzzer
- One GPIO pin → buzzer signal pin; buzzer GND → board GND

## Minimal Example

```c
#include "blusys/buzzer.h"

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

## APIs Used

| Function | Purpose |
|---|---|
| `blusys_buzzer_open()` | Acquire a PWM channel and configure the pin |
| `blusys_buzzer_set_callback()` | Register a DONE event handler |
| `blusys_buzzer_play()` | Play a single tone asynchronously |
| `blusys_buzzer_play_sequence()` | Play a note array asynchronously |
| `blusys_buzzer_stop()` | Abort playback (no DONE event) |
| `blusys_buzzer_is_playing()` | Poll playback state |
| `blusys_buzzer_close()` | Release all resources |

## Expected Runtime Behavior

- **Single tone**: `blusys_buzzer_play(bz, 440, 500)` plays a 440 Hz tone for 500 ms, then fires `BLUSYS_BUZZER_EVENT_DONE`.
- **Sequence**: notes play in order; rest notes (`freq_hz = 0`) produce silence for `duration_ms`. A single `BLUSYS_BUZZER_EVENT_DONE` fires after the last note.
- **Early stop**: `blusys_buzzer_stop()` silences the buzzer immediately; no `DONE` event fires.
- **Calling `play()` while playing**: the current tone/sequence is cancelled and the new one starts immediately.

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

## Example App

The `buzzer_basic` example ships in `examples/buzzer_basic/`. It demonstrates:

1. A single 440 Hz tone for 500 ms
2. A three-note ascending jingle with rests
3. Early stop (5 s tone interrupted after 300 ms)

Build and flash:

```bash
blusys run examples/buzzer_basic /dev/ttyUSB0 esp32s3
```

Override the pin via menuconfig:

```bash
blusys config examples/buzzer_basic esp32s3
```

## API Reference

[Buzzer API Reference](../modules/buzzer.md)
