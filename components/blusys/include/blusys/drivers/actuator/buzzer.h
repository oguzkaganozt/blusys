#ifndef BLUSYS_BUZZER_H
#define BLUSYS_BUZZER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_buzzer blusys_buzzer_t;

/**
 * A single note in a melody sequence.
 * Set freq_hz to 0 for a rest (silence for duration_ms).
 */
typedef struct {
    uint32_t freq_hz;     /**< Tone frequency in Hz; 0 = rest (silence) */
    uint32_t duration_ms; /**< Duration in milliseconds */
} blusys_buzzer_note_t;

typedef enum {
    BLUSYS_BUZZER_EVENT_DONE, /**< Tone or sequence finished naturally */
} blusys_buzzer_event_t;

typedef void (*blusys_buzzer_callback_t)(blusys_buzzer_t *buzzer,
                                          blusys_buzzer_event_t event,
                                          void *user_ctx);

typedef struct {
    int      pin;            /**< GPIO pin connected to the buzzer signal */
    uint16_t duty_permille;  /**< PWM duty cycle 0–1000; 0 defaults to 500 (50%) */
} blusys_buzzer_config_t;

blusys_err_t blusys_buzzer_open(const blusys_buzzer_config_t *config,
                                 blusys_buzzer_t **out_buzzer);
blusys_err_t blusys_buzzer_close(blusys_buzzer_t *buzzer);
blusys_err_t blusys_buzzer_set_callback(blusys_buzzer_t *buzzer,
                                         blusys_buzzer_callback_t callback,
                                         void *user_ctx);

/**
 * Play a single tone asynchronously.
 * Cancels any active tone or sequence and starts immediately.
 * BLUSYS_BUZZER_EVENT_DONE fires when the duration elapses.
 */
blusys_err_t blusys_buzzer_play(blusys_buzzer_t *buzzer,
                                 uint32_t freq_hz,
                                 uint32_t duration_ms);

/**
 * Play a sequence of notes asynchronously.
 * Cancels any active tone or sequence and starts immediately.
 * The caller must keep the notes array valid until BLUSYS_BUZZER_EVENT_DONE fires.
 * BLUSYS_BUZZER_EVENT_DONE fires once after the last note finishes.
 */
blusys_err_t blusys_buzzer_play_sequence(blusys_buzzer_t *buzzer,
                                          const blusys_buzzer_note_t *notes,
                                          size_t count);

/**
 * Stop playback immediately. Does not fire BLUSYS_BUZZER_EVENT_DONE.
 */
blusys_err_t blusys_buzzer_stop(blusys_buzzer_t *buzzer);

blusys_err_t blusys_buzzer_is_playing(blusys_buzzer_t *buzzer, bool *out_playing);

#ifdef __cplusplus
}
#endif

#endif
