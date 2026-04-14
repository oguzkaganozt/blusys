#include "blusys/drivers/buzzer.h"

#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include "blusys/hal/pwm.h"
#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/lock.h"

#define BUZZER_DEFAULT_DUTY_PERMILLE 500u
#define BUZZER_INIT_FREQ_HZ          1000u

struct blusys_buzzer {
    int                         pin;
    uint16_t                    duty_permille;
    blusys_pwm_t               *pwm;
    blusys_lock_t               lock;         /* slow mutex: serialises public API calls */
    portMUX_TYPE                state_lock;   /* spinlock: guards shared state with timer cb */
    blusys_buzzer_callback_t    callback;
    void                       *user_ctx;
    volatile unsigned           callback_active;
    esp_timer_handle_t          timer;
    bool                        playing;
    bool                        closing;
    const blusys_buzzer_note_t *seq_notes;    /* NULL in single-tone mode */
    size_t                      seq_count;
    size_t                      seq_index;    /* index of the next note to play */
};

/* ── helpers ──────────────────────────────────────────────────────────────── */

static void invoke_callback(blusys_buzzer_t *bz, blusys_buzzer_event_t event)
{
    blusys_buzzer_callback_t cb;
    void *ctx;

    portENTER_CRITICAL(&bz->state_lock);
    cb  = bz->callback;
    ctx = bz->user_ctx;
    if (cb != NULL) {
        bz->callback_active += 1u;
    }
    portEXIT_CRITICAL(&bz->state_lock);

    if (cb != NULL) {
        cb(bz, event, ctx);
        portENTER_CRITICAL(&bz->state_lock);
        bz->callback_active -= 1u;
        portEXIT_CRITICAL(&bz->state_lock);
    }
}

/* Start a note: set frequency (if audible), start or stop PWM, arm timer. */
static void start_note(blusys_buzzer_t *bz, uint32_t freq_hz, uint32_t duration_ms)
{
    if (freq_hz > 0u) {
        blusys_pwm_set_frequency(bz->pwm, freq_hz);
        blusys_pwm_start(bz->pwm);
    } else {
        blusys_pwm_stop(bz->pwm);
    }
    esp_timer_start_once(bz->timer, (uint64_t)duration_ms * 1000u);
}

/* ── timer callback (esp_timer task context) ──────────────────────────────── */

static void buzzer_timer_cb(void *arg)
{
    blusys_buzzer_t *bz = arg;
    const blusys_buzzer_note_t *notes;
    size_t idx;
    size_t count;

    portENTER_CRITICAL(&bz->state_lock);
    if (bz->closing) {
        portEXIT_CRITICAL(&bz->state_lock);
        return;
    }
    portEXIT_CRITICAL(&bz->state_lock);

    /* The note that just ended — silence the output. */
    blusys_pwm_stop(bz->pwm);

    /* Read sequence state under the spinlock to guard against concurrent stop(). */
    portENTER_CRITICAL(&bz->state_lock);
    notes = bz->seq_notes;
    idx   = bz->seq_index;
    count = bz->seq_count;
    portEXIT_CRITICAL(&bz->state_lock);

    if ((notes != NULL) && (idx < count)) {
        /* Advance to the next note in the sequence. */
        portENTER_CRITICAL(&bz->state_lock);
        bz->seq_index = idx + 1u;
        portEXIT_CRITICAL(&bz->state_lock);

        start_note(bz, notes[idx].freq_hz, notes[idx].duration_ms);
        return;
    }

    /* Sequence exhausted or single-tone done — mark idle and fire DONE. */
    portENTER_CRITICAL(&bz->state_lock);
    bz->playing   = false;
    bz->seq_notes = NULL;
    bz->seq_count = 0u;
    bz->seq_index = 0u;
    portEXIT_CRITICAL(&bz->state_lock);

    invoke_callback(bz, BLUSYS_BUZZER_EVENT_DONE);
}

/* ── public API ───────────────────────────────────────────────────────────── */

blusys_err_t blusys_buzzer_open(const blusys_buzzer_config_t *config,
                                 blusys_buzzer_t **out_buzzer)
{
    blusys_buzzer_t *bz;
    blusys_err_t err;
    esp_err_t esp_err;

    if ((config == NULL) || (out_buzzer == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    bz = calloc(1, sizeof(*bz));
    if (bz == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    bz->pin           = config->pin;
    bz->duty_permille = (config->duty_permille > 0u) ? config->duty_permille
                                                      : BUZZER_DEFAULT_DUTY_PERMILLE;
    bz->state_lock    = (portMUX_TYPE)portMUX_INITIALIZER_UNLOCKED;

    err = blusys_lock_init(&bz->lock);
    if (err != BLUSYS_OK) {
        free(bz);
        return err;
    }

    /* Open PWM at a safe initial frequency; it will be overwritten before start(). */
    err = blusys_pwm_open(bz->pin, BUZZER_INIT_FREQ_HZ, bz->duty_permille, &bz->pwm);
    if (err != BLUSYS_OK) {
        blusys_lock_deinit(&bz->lock);
        free(bz);
        return err;
    }

    const esp_timer_create_args_t timer_args = {
        .callback        = buzzer_timer_cb,
        .arg             = bz,
        .dispatch_method = ESP_TIMER_TASK,
        .name            = "buzzer",
    };
    esp_err = esp_timer_create(&timer_args, &bz->timer);
    if (esp_err != ESP_OK) {
        blusys_pwm_close(bz->pwm);
        blusys_lock_deinit(&bz->lock);
        free(bz);
        return blusys_translate_esp_err(esp_err);
    }

    *out_buzzer = bz;
    return BLUSYS_OK;
}

blusys_err_t blusys_buzzer_close(blusys_buzzer_t *buzzer)
{
    if (buzzer == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&buzzer->state_lock);
    buzzer->closing = true;
    portEXIT_CRITICAL(&buzzer->state_lock);

    /* Stop the timer — ignore INVALID_STATE (already idle). */
    esp_timer_stop(buzzer->timer);

    /* Wait for any in-flight callback to finish. */
    while (buzzer->callback_active > 0u) {
        taskYIELD();
    }

    blusys_pwm_stop(buzzer->pwm);
    blusys_pwm_close(buzzer->pwm);
    esp_timer_delete(buzzer->timer);
    blusys_lock_deinit(&buzzer->lock);
    free(buzzer);
    return BLUSYS_OK;
}

blusys_err_t blusys_buzzer_set_callback(blusys_buzzer_t *buzzer,
                                         blusys_buzzer_callback_t callback,
                                         void *user_ctx)
{
    blusys_err_t err;

    if (buzzer == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&buzzer->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    portENTER_CRITICAL(&buzzer->state_lock);
    if (buzzer->closing) {
        portEXIT_CRITICAL(&buzzer->state_lock);
        blusys_lock_give(&buzzer->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }
    buzzer->callback  = callback;
    buzzer->user_ctx  = user_ctx;
    portEXIT_CRITICAL(&buzzer->state_lock);

    /* Wait for any in-flight callback to finish before returning. */
    while (buzzer->callback_active > 0u) {
        taskYIELD();
    }

    blusys_lock_give(&buzzer->lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_buzzer_play(blusys_buzzer_t *buzzer,
                                 uint32_t freq_hz,
                                 uint32_t duration_ms)
{
    blusys_err_t err;

    if ((buzzer == NULL) || (freq_hz == 0u) || (duration_ms == 0u)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&buzzer->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    portENTER_CRITICAL(&buzzer->state_lock);
    if (buzzer->closing) {
        portEXIT_CRITICAL(&buzzer->state_lock);
        blusys_lock_give(&buzzer->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }
    buzzer->playing   = true;
    buzzer->seq_notes = NULL;
    buzzer->seq_count = 0u;
    buzzer->seq_index = 0u;
    portEXIT_CRITICAL(&buzzer->state_lock);

    /* Cancel any active playback and start the new tone. */
    esp_timer_stop(buzzer->timer);
    start_note(buzzer, freq_hz, duration_ms);

    blusys_lock_give(&buzzer->lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_buzzer_play_sequence(blusys_buzzer_t *buzzer,
                                          const blusys_buzzer_note_t *notes,
                                          size_t count)
{
    blusys_err_t err;

    if ((buzzer == NULL) || (notes == NULL) || (count == 0u)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&buzzer->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    portENTER_CRITICAL(&buzzer->state_lock);
    if (buzzer->closing) {
        portEXIT_CRITICAL(&buzzer->state_lock);
        blusys_lock_give(&buzzer->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }
    /* seq_index = 1: the timer callback will read index 1 after note[0] finishes. */
    buzzer->playing   = true;
    buzzer->seq_notes = notes;
    buzzer->seq_count = count;
    buzzer->seq_index = 1u;
    portEXIT_CRITICAL(&buzzer->state_lock);

    /* Cancel any active playback, then start the first note. */
    esp_timer_stop(buzzer->timer);
    start_note(buzzer, notes[0].freq_hz, notes[0].duration_ms);

    blusys_lock_give(&buzzer->lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_buzzer_stop(blusys_buzzer_t *buzzer)
{
    blusys_err_t err;

    if (buzzer == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&buzzer->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    portENTER_CRITICAL(&buzzer->state_lock);
    if (buzzer->closing) {
        portEXIT_CRITICAL(&buzzer->state_lock);
        blusys_lock_give(&buzzer->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }
    buzzer->playing   = false;
    buzzer->seq_notes = NULL;
    buzzer->seq_count = 0u;
    buzzer->seq_index = 0u;
    portEXIT_CRITICAL(&buzzer->state_lock);

    /* Ignore INVALID_STATE — timer may already be idle. */
    esp_timer_stop(buzzer->timer);
    blusys_pwm_stop(buzzer->pwm);

    blusys_lock_give(&buzzer->lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_buzzer_is_playing(blusys_buzzer_t *buzzer, bool *out_playing)
{
    if ((buzzer == NULL) || (out_playing == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&buzzer->state_lock);
    *out_playing = buzzer->playing;
    portEXIT_CRITICAL(&buzzer->state_lock);

    return BLUSYS_OK;
}
