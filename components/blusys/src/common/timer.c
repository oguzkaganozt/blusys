#include "blusys/timer.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "blusys_esp_err.h"
#include "blusys_lock.h"

#include "driver/gptimer.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BLUSYS_TIMER_RESOLUTION_HZ 1000000u

struct blusys_timer {
    uint32_t period_us;
    bool periodic;
    bool started;
    bool closing;
    unsigned int callback_active;
    blusys_timer_callback_t callback;
    void *callback_user_ctx;
    gptimer_handle_t handle;
    blusys_lock_t lock;
};

static portMUX_TYPE blusys_timer_state_lock = portMUX_INITIALIZER_UNLOCKED;

static bool blusys_timer_callback_active(const blusys_timer_t *timer)
{
    bool active;

    portENTER_CRITICAL(&blusys_timer_state_lock);
    active = timer->callback_active > 0u;
    portEXIT_CRITICAL(&blusys_timer_state_lock);

    return active;
}

static void blusys_timer_wait_for_callback_idle(const blusys_timer_t *timer)
{
    while (blusys_timer_callback_active(timer)) {
        taskYIELD();
    }
}

static bool blusys_timer_is_started(const blusys_timer_t *timer)
{
    bool started;

    portENTER_CRITICAL(&blusys_timer_state_lock);
    started = timer->started;
    portEXIT_CRITICAL(&blusys_timer_state_lock);

    return started;
}

static void blusys_timer_set_started(blusys_timer_t *timer, bool started)
{
    portENTER_CRITICAL(&blusys_timer_state_lock);
    timer->started = started;
    portEXIT_CRITICAL(&blusys_timer_state_lock);
}

static void blusys_timer_set_closing(blusys_timer_t *timer, bool closing)
{
    portENTER_CRITICAL(&blusys_timer_state_lock);
    timer->closing = closing;
    portEXIT_CRITICAL(&blusys_timer_state_lock);
}

static gptimer_alarm_config_t blusys_timer_alarm_config(const blusys_timer_t *timer)
{
    return (gptimer_alarm_config_t) {
        .alarm_count = timer->period_us,
        .reload_count = 0u,
        .flags.auto_reload_on_alarm = timer->periodic ? 1u : 0u,
    };
}

static bool IRAM_ATTR blusys_timer_on_alarm(gptimer_handle_t gptimer,
                                            const gptimer_alarm_event_data_t *edata,
                                            void *user_data)
{
    blusys_timer_t *timer = user_data;
    blusys_timer_callback_t callback;
    void *callback_user_ctx;
    bool periodic;
    bool closing;
    bool should_yield = false;

    (void) edata;

    portENTER_CRITICAL_ISR(&blusys_timer_state_lock);
    timer->callback_active += 1u;
    periodic = timer->periodic;
    closing = timer->closing;
    if (!periodic) {
        timer->started = false;
    }
    callback = closing ? NULL : timer->callback;
    callback_user_ctx = timer->callback_user_ctx;
    portEXIT_CRITICAL_ISR(&blusys_timer_state_lock);

    if (!periodic) {
        gptimer_stop(gptimer);
    }
    if (callback != NULL) {
        should_yield = callback(timer, callback_user_ctx);
    }

    portENTER_CRITICAL_ISR(&blusys_timer_state_lock);
    timer->callback_active -= 1u;
    portEXIT_CRITICAL_ISR(&blusys_timer_state_lock);

    return should_yield;
}

blusys_err_t blusys_timer_open(uint32_t period_us, bool periodic, blusys_timer_t **out_timer)
{
    blusys_timer_t *timer;
    blusys_err_t err;
    esp_err_t esp_err;

    if ((period_us == 0u) || (out_timer == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    timer = calloc(1, sizeof(*timer));
    if (timer == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&timer->lock);
    if (err != BLUSYS_OK) {
        free(timer);
        return err;
    }

    timer->period_us = period_us;
    timer->periodic = periodic;

    esp_err = gptimer_new_timer(&(gptimer_config_t) {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = BLUSYS_TIMER_RESOLUTION_HZ,
        .intr_priority = 0,
        .flags.intr_shared = 0u,
        .flags.allow_pd = 0u,
    }, &timer->handle);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_lock;
    }

    esp_err = gptimer_register_event_callbacks(timer->handle,
                                               &(gptimer_event_callbacks_t) {
                                                   .on_alarm = blusys_timer_on_alarm,
                                               },
                                               timer);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_timer;
    }

    esp_err = gptimer_enable(timer->handle);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_enabled;
    }

    *out_timer = timer;
    return BLUSYS_OK;

fail_enabled:
    if (gptimer_disable(timer->handle) != ESP_OK) {
        /* Best-effort cleanup before deleting the timer handle. */
    }
fail_timer:
    gptimer_del_timer(timer->handle);
fail_lock:
    blusys_lock_deinit(&timer->lock);
    free(timer);
    return err;
}

blusys_err_t blusys_timer_close(blusys_timer_t *timer)
{
    blusys_err_t err;
    esp_err_t esp_err;

    if (timer == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&timer->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    blusys_timer_set_closing(timer, true);

    if (blusys_timer_is_started(timer)) {
        esp_err = gptimer_stop(timer->handle);
        if ((esp_err != ESP_OK) && (esp_err != ESP_ERR_INVALID_STATE)) {
            err = blusys_translate_esp_err(esp_err);
            goto fail;
        }
        blusys_timer_set_started(timer, false);
    }

    esp_err = gptimer_disable(timer->handle);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail;
    }

    /* Release lock before the spin-wait — closing flag prevents concurrent
       callers from using the handle.  The timer is already disabled so no
       new ISR callbacks can fire; this wait is only for an in-flight one. */
    blusys_lock_give(&timer->lock);

    blusys_timer_wait_for_callback_idle(timer);

    esp_err = gptimer_del_timer(timer->handle);
    if (esp_err != ESP_OK) {
        blusys_lock_deinit(&timer->lock);
        free(timer);
        return blusys_translate_esp_err(esp_err);
    }

    blusys_lock_deinit(&timer->lock);
    free(timer);
    return BLUSYS_OK;

fail:
    blusys_timer_set_closing(timer, false);
    blusys_lock_give(&timer->lock);
    return err;
}

blusys_err_t blusys_timer_start(blusys_timer_t *timer)
{
    blusys_err_t err;
    esp_err_t esp_err;

    if (timer == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&timer->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (blusys_timer_is_started(timer)) {
        blusys_lock_give(&timer->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    blusys_timer_wait_for_callback_idle(timer);

    esp_err = gptimer_set_raw_count(timer->handle, 0u);
    if (esp_err == ESP_OK) {
        gptimer_alarm_config_t alarm_config = blusys_timer_alarm_config(timer);

        esp_err = gptimer_set_alarm_action(timer->handle, &alarm_config);
        if (esp_err == ESP_OK) {
            esp_err = gptimer_start(timer->handle);
        }
    }

    err = blusys_translate_esp_err(esp_err);
    if (err == BLUSYS_OK) {
        blusys_timer_set_started(timer, true);
    }

    blusys_lock_give(&timer->lock);
    return err;
}

blusys_err_t blusys_timer_stop(blusys_timer_t *timer)
{
    blusys_err_t err;
    esp_err_t esp_err;

    if (timer == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&timer->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (!blusys_timer_is_started(timer)) {
        blusys_lock_give(&timer->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    esp_err = gptimer_stop(timer->handle);
    err = blusys_translate_esp_err(esp_err);
    if (err == BLUSYS_OK) {
        blusys_timer_set_started(timer, false);
        blusys_timer_wait_for_callback_idle(timer);
    }

    blusys_lock_give(&timer->lock);
    return err;
}

blusys_err_t blusys_timer_set_period(blusys_timer_t *timer, uint32_t period_us)
{
    blusys_err_t err;

    if ((timer == NULL) || (period_us == 0u)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&timer->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (blusys_timer_is_started(timer)) {
        blusys_lock_give(&timer->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    blusys_timer_wait_for_callback_idle(timer);
    timer->period_us = period_us;

    blusys_lock_give(&timer->lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_timer_set_callback(blusys_timer_t *timer,
                                       blusys_timer_callback_t callback,
                                       void *user_ctx)
{
    blusys_err_t err;

    if (timer == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&timer->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (blusys_timer_is_started(timer)) {
        blusys_lock_give(&timer->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    blusys_timer_wait_for_callback_idle(timer);

    portENTER_CRITICAL(&blusys_timer_state_lock);
    timer->callback = callback;
    timer->callback_user_ctx = user_ctx;
    portEXIT_CRITICAL(&blusys_timer_state_lock);

    blusys_lock_give(&timer->lock);
    return BLUSYS_OK;
}
