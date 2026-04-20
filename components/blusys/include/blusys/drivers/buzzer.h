/**
 * @file buzzer.h
 * @brief Passive-buzzer driver: single tones and asynchronous note sequences.
 *
 * Drives the buzzer pin from `hal/pwm`. Playback is asynchronous; one
 * ::BLUSYS_BUZZER_EVENT_DONE fires when a tone or sequence ends naturally.
 * See docs/hal/buzzer.md.
 */

#ifndef BLUSYS_BUZZER_H
#define BLUSYS_BUZZER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle to an open buzzer. */
typedef struct blusys_buzzer blusys_buzzer_t;

/**
 * @brief A single note in a melody sequence.
 *
 * Set @ref blusys_buzzer_note_t::freq_hz to `0` for a rest — the buzzer is
 * silent for @ref blusys_buzzer_note_t::duration_ms milliseconds.
 */
typedef struct {
    uint32_t freq_hz;     /**< Tone frequency in Hz; `0` = rest (silence). */
    uint32_t duration_ms; /**< Duration in milliseconds. */
} blusys_buzzer_note_t;

/** @brief Buzzer event kind. */
typedef enum {
    BLUSYS_BUZZER_EVENT_DONE,   /**< Tone or sequence finished naturally. */
} blusys_buzzer_event_t;

/**
 * @brief Callback fired from the buzzer task when an event is delivered.
 * @param buzzer    Buzzer that produced the event.
 * @param event     Event kind.
 * @param user_ctx  User pointer supplied to ::blusys_buzzer_set_callback.
 */
typedef void (*blusys_buzzer_callback_t)(blusys_buzzer_t *buzzer,
                                          blusys_buzzer_event_t event,
                                          void *user_ctx);

/** @brief Configuration for ::blusys_buzzer_open. */
typedef struct {
    int      pin;            /**< GPIO pin connected to the buzzer signal. */
    uint16_t duty_permille;  /**< PWM duty cycle in per-mille (0–1000); `0` selects 500 (50 %). */
} blusys_buzzer_config_t;

/**
 * @brief Open a buzzer and start its playback task.
 * @param config      Configuration.
 * @param out_buzzer  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_NO_MEM`.
 */
blusys_err_t blusys_buzzer_open(const blusys_buzzer_config_t *config,
                                 blusys_buzzer_t **out_buzzer);

/**
 * @brief Stop playback, release the handle, and free resources.
 * @param buzzer  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p buzzer is NULL.
 */
blusys_err_t blusys_buzzer_close(blusys_buzzer_t *buzzer);

/**
 * @brief Register or replace the event callback.
 * @param buzzer    Handle.
 * @param callback  Callback; NULL clears the existing registration.
 * @param user_ctx  Opaque pointer forwarded to @p callback.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p buzzer is NULL.
 */
blusys_err_t blusys_buzzer_set_callback(blusys_buzzer_t *buzzer,
                                         blusys_buzzer_callback_t callback,
                                         void *user_ctx);

/**
 * @brief Play a single tone asynchronously.
 *
 * Cancels any active tone or sequence and starts immediately.
 * ::BLUSYS_BUZZER_EVENT_DONE fires when @p duration_ms elapses.
 *
 * @param buzzer       Handle.
 * @param freq_hz      Tone frequency in Hz; `0` plays silence for @p duration_ms.
 * @param duration_ms  Duration in milliseconds.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`.
 */
blusys_err_t blusys_buzzer_play(blusys_buzzer_t *buzzer,
                                 uint32_t freq_hz,
                                 uint32_t duration_ms);

/**
 * @brief Play a sequence of notes asynchronously.
 *
 * Cancels any active tone or sequence and starts immediately. The caller
 * must keep @p notes valid until ::BLUSYS_BUZZER_EVENT_DONE fires.
 * ::BLUSYS_BUZZER_EVENT_DONE fires once, after the last note finishes.
 *
 * @param buzzer  Handle.
 * @param notes   Array of notes (must remain valid until DONE fires).
 * @param count   Number of notes.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`.
 */
blusys_err_t blusys_buzzer_play_sequence(blusys_buzzer_t *buzzer,
                                          const blusys_buzzer_note_t *notes,
                                          size_t count);

/**
 * @brief Stop playback immediately. Does not fire ::BLUSYS_BUZZER_EVENT_DONE.
 * @param buzzer  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p buzzer is NULL.
 */
blusys_err_t blusys_buzzer_stop(blusys_buzzer_t *buzzer);

/**
 * @brief Query whether playback is currently active.
 * @param buzzer       Handle.
 * @param out_playing  Output: `true` if a tone or sequence is still playing.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if any pointer is NULL.
 */
blusys_err_t blusys_buzzer_is_playing(blusys_buzzer_t *buzzer, bool *out_playing);

#ifdef __cplusplus
}
#endif

#endif
