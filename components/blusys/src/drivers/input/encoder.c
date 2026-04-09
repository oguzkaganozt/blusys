#include "blusys/drivers/input/encoder.h"

#include <stdlib.h>

#include "soc/soc_caps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include "driver/gpio.h"

#include "blusys/drivers/input/button.h"
#include "blusys/gpio.h"
#include "blusys/internal/blusys_esp_err.h"
#include "blusys/internal/blusys_lock.h"

#if SOC_PCNT_SUPPORTED
#include "driver/pulse_cnt.h"
#endif

#define ENCODER_DEFAULT_STEPS_PER_DETENT 4

struct blusys_encoder {
    int                        clk_pin;
    int                        dt_pin;
    int                        steps_per_detent;
    blusys_lock_t              lock;
    portMUX_TYPE               state_lock;
    blusys_encoder_callback_t  callback;
    void                      *user_ctx;
    volatile unsigned          callback_active;
    volatile int32_t           position;
    bool                       closing;

    blusys_button_t           *button;

    esp_timer_handle_t         event_timer;
    blusys_encoder_event_t     pending_event;

#if SOC_PCNT_SUPPORTED
    pcnt_unit_handle_t         pcnt_unit;
    pcnt_channel_handle_t      pcnt_chan_a;
    pcnt_channel_handle_t      pcnt_chan_b;
#else
    bool                       last_clk;
    bool                       last_dt;
    int8_t                     gpio_accum;
#endif
};

/* ── helpers ──────────────────────────────────────────────────────────────── */

static void invoke_callback(blusys_encoder_t *enc, blusys_encoder_event_t event)
{
    blusys_encoder_callback_t cb;
    void *ctx;
    int pos;

    portENTER_CRITICAL(&enc->state_lock);
    cb  = enc->callback;
    ctx = enc->user_ctx;
    pos = (int)enc->position;
    if (cb != NULL) {
        enc->callback_active += 1u;
    }
    portEXIT_CRITICAL(&enc->state_lock);

    if (cb != NULL) {
        cb(enc, event, pos, ctx);
        portENTER_CRITICAL(&enc->state_lock);
        enc->callback_active -= 1u;
        portEXIT_CRITICAL(&enc->state_lock);
    }
}

/* ── deferred event timer (task context) ──────────────────────────────────── */

static void event_timer_cb(void *arg)
{
    blusys_encoder_t *enc = arg;
    blusys_encoder_event_t event;

    portENTER_CRITICAL(&enc->state_lock);
    if (enc->closing) {
        portEXIT_CRITICAL(&enc->state_lock);
        return;
    }
    event = enc->pending_event;
    portEXIT_CRITICAL(&enc->state_lock);

    invoke_callback(enc, event);
}

/* ── button bridge ────────────────────────────────────────────────────────── */

static void encoder_button_cb(blusys_button_t *button,
                               blusys_button_event_t event,
                               void *user_ctx)
{
    blusys_encoder_t *enc = user_ctx;
    blusys_encoder_event_t enc_event;

    (void)button;

    switch (event) {
    case BLUSYS_BUTTON_EVENT_PRESS:      enc_event = BLUSYS_ENCODER_EVENT_PRESS; break;
    case BLUSYS_BUTTON_EVENT_RELEASE:    enc_event = BLUSYS_ENCODER_EVENT_RELEASE; break;
    case BLUSYS_BUTTON_EVENT_LONG_PRESS: enc_event = BLUSYS_ENCODER_EVENT_LONG_PRESS; break;
    default: return;
    }

    invoke_callback(enc, enc_event);
}

/* ── PCNT path (ESP32 / ESP32-S3) ─────────────────────────────────────────── */

#if SOC_PCNT_SUPPORTED

static bool IRAM_ATTR encoder_pcnt_on_reach(pcnt_unit_handle_t unit,
                                            const pcnt_watch_event_data_t *edata,
                                            void *user_ctx)
{
    blusys_encoder_t *enc = user_ctx;

    (void)unit;

    portENTER_CRITICAL_ISR(&enc->state_lock);
    if (enc->closing) {
        portEXIT_CRITICAL_ISR(&enc->state_lock);
        return false;
    }

    if (edata->watch_point_value > 0) {
        enc->position += 1;
        enc->pending_event = BLUSYS_ENCODER_EVENT_CW;
    } else {
        enc->position -= 1;
        enc->pending_event = BLUSYS_ENCODER_EVENT_CCW;
    }
    portEXIT_CRITICAL_ISR(&enc->state_lock);

    pcnt_unit_clear_count(unit);

    esp_timer_stop(enc->event_timer);
    esp_timer_start_once(enc->event_timer, 0);

    return false;
}

static blusys_err_t encoder_pcnt_init(blusys_encoder_t *enc,
                                      uint32_t glitch_filter_ns)
{
    esp_err_t esp_err;

    esp_err = pcnt_new_unit(&(pcnt_unit_config_t) {
        .low_limit  = -32768,
        .high_limit = 32767,
        .intr_priority = 0,
        .flags.accum_count = 0u,
    }, &enc->pcnt_unit);
    if (esp_err != ESP_OK) {
        return blusys_translate_esp_err(esp_err);
    }

    if (glitch_filter_ns > 0u) {
        esp_err = pcnt_unit_set_glitch_filter(enc->pcnt_unit,
            &(pcnt_glitch_filter_config_t) {
                .max_glitch_ns = glitch_filter_ns,
            });
        if (esp_err != ESP_OK) {
            goto fail_unit;
        }
    }

    /* Channel A: edge on CLK, level on DT */
    esp_err = pcnt_new_channel(enc->pcnt_unit, &(pcnt_chan_config_t) {
        .edge_gpio_num  = enc->clk_pin,
        .level_gpio_num = enc->dt_pin,
    }, &enc->pcnt_chan_a);
    if (esp_err != ESP_OK) {
        goto fail_unit;
    }

    esp_err = pcnt_channel_set_edge_action(enc->pcnt_chan_a,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE);
    if (esp_err != ESP_OK) {
        goto fail_chan_a;
    }

    esp_err = pcnt_channel_set_level_action(enc->pcnt_chan_a,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    if (esp_err != ESP_OK) {
        goto fail_chan_a;
    }

    /* Channel B: edge on DT, level on CLK (cross-wired for 4x resolution) */
    esp_err = pcnt_new_channel(enc->pcnt_unit, &(pcnt_chan_config_t) {
        .edge_gpio_num  = enc->dt_pin,
        .level_gpio_num = enc->clk_pin,
    }, &enc->pcnt_chan_b);
    if (esp_err != ESP_OK) {
        goto fail_chan_a;
    }

    esp_err = pcnt_channel_set_edge_action(enc->pcnt_chan_b,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE);
    if (esp_err != ESP_OK) {
        goto fail_chan_b;
    }

    esp_err = pcnt_channel_set_level_action(enc->pcnt_chan_b,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    if (esp_err != ESP_OK) {
        goto fail_chan_b;
    }

    esp_err = pcnt_unit_add_watch_point(enc->pcnt_unit, enc->steps_per_detent);
    if (esp_err != ESP_OK) {
        goto fail_chan_b;
    }

    esp_err = pcnt_unit_add_watch_point(enc->pcnt_unit, -enc->steps_per_detent);
    if (esp_err != ESP_OK) {
        goto fail_chan_b;
    }

    esp_err = pcnt_unit_register_event_callbacks(enc->pcnt_unit,
        &(pcnt_event_callbacks_t) { .on_reach = encoder_pcnt_on_reach },
        enc);
    if (esp_err != ESP_OK) {
        goto fail_chan_b;
    }

    esp_err = pcnt_unit_enable(enc->pcnt_unit);
    if (esp_err != ESP_OK) {
        goto fail_chan_b;
    }

    esp_err = pcnt_unit_clear_count(enc->pcnt_unit);
    if (esp_err != ESP_OK) {
        goto fail_enabled;
    }

    esp_err = pcnt_unit_start(enc->pcnt_unit);
    if (esp_err != ESP_OK) {
        goto fail_enabled;
    }

    return BLUSYS_OK;

fail_enabled:
    pcnt_unit_disable(enc->pcnt_unit);
fail_chan_b:
    pcnt_del_channel(enc->pcnt_chan_b);
fail_chan_a:
    pcnt_del_channel(enc->pcnt_chan_a);
fail_unit:
    pcnt_del_unit(enc->pcnt_unit);
    return blusys_translate_esp_err(esp_err);
}

static void encoder_pcnt_deinit(blusys_encoder_t *enc)
{
    pcnt_unit_stop(enc->pcnt_unit);
    pcnt_unit_disable(enc->pcnt_unit);
    pcnt_del_channel(enc->pcnt_chan_b);
    pcnt_del_channel(enc->pcnt_chan_a);
    pcnt_del_unit(enc->pcnt_unit);
}

#else /* GPIO fallback (ESP32-C3) */

static const int8_t s_gray_table[16] = {
     0, +1, -1,  0,
    -1,  0,  0, +1,
    +1,  0,  0, -1,
     0, -1, +1,  0,
};

static void gpio_debounce_timer_cb(void *arg)
{
    blusys_encoder_t *enc = arg;
    bool clk, dt;

    portENTER_CRITICAL(&enc->state_lock);
    if (enc->closing) {
        portEXIT_CRITICAL(&enc->state_lock);
        return;
    }
    portEXIT_CRITICAL(&enc->state_lock);

    if (blusys_gpio_read(enc->clk_pin, &clk) != BLUSYS_OK) {
        return;
    }
    if (blusys_gpio_read(enc->dt_pin, &dt) != BLUSYS_OK) {
        return;
    }

    int8_t old_state = (enc->last_clk << 1) | enc->last_dt;
    int8_t new_state = (clk << 1) | dt;

    if (old_state == new_state) {
        return;
    }

    enc->last_clk = clk;
    enc->last_dt  = dt;

    int8_t delta = s_gray_table[(old_state << 2) | new_state];
    if (delta == 0) {
        return;
    }

    enc->gpio_accum += delta;

    if (enc->gpio_accum >= enc->steps_per_detent) {
        enc->gpio_accum = 0;
        portENTER_CRITICAL(&enc->state_lock);
        enc->position += 1;
        portEXIT_CRITICAL(&enc->state_lock);
        invoke_callback(enc, BLUSYS_ENCODER_EVENT_CW);
    } else if (enc->gpio_accum <= -enc->steps_per_detent) {
        enc->gpio_accum = 0;
        portENTER_CRITICAL(&enc->state_lock);
        enc->position -= 1;
        portEXIT_CRITICAL(&enc->state_lock);
        invoke_callback(enc, BLUSYS_ENCODER_EVENT_CCW);
    }
}

static bool IRAM_ATTR encoder_gpio_isr_cb(int pin, void *user_ctx)
{
    blusys_encoder_t *enc = user_ctx;

    (void)pin;

    esp_timer_stop(enc->event_timer);
    esp_timer_start_once(enc->event_timer, 500);

    return false;
}

static blusys_err_t encoder_gpio_init(blusys_encoder_t *enc)
{
    blusys_err_t err;

    err = blusys_gpio_set_input(enc->clk_pin);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_gpio_set_pull_mode(enc->clk_pin, BLUSYS_GPIO_PULL_UP);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_gpio_set_input(enc->dt_pin);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_gpio_set_pull_mode(enc->dt_pin, BLUSYS_GPIO_PULL_UP);
    if (err != BLUSYS_OK) {
        return err;
    }

    bool clk, dt;
    blusys_gpio_read(enc->clk_pin, &clk);
    blusys_gpio_read(enc->dt_pin, &dt);
    enc->last_clk = clk;
    enc->last_dt  = dt;

    err = blusys_gpio_set_interrupt(enc->clk_pin, BLUSYS_GPIO_INTERRUPT_ANY_EDGE);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_gpio_set_callback(enc->clk_pin, encoder_gpio_isr_cb, enc);
    if (err != BLUSYS_OK) {
        blusys_gpio_set_interrupt(enc->clk_pin, BLUSYS_GPIO_INTERRUPT_DISABLED);
        return err;
    }

    err = blusys_gpio_set_interrupt(enc->dt_pin, BLUSYS_GPIO_INTERRUPT_ANY_EDGE);
    if (err != BLUSYS_OK) {
        blusys_gpio_set_callback(enc->clk_pin, NULL, NULL);
        blusys_gpio_set_interrupt(enc->clk_pin, BLUSYS_GPIO_INTERRUPT_DISABLED);
        return err;
    }

    err = blusys_gpio_set_callback(enc->dt_pin, encoder_gpio_isr_cb, enc);
    if (err != BLUSYS_OK) {
        blusys_gpio_set_interrupt(enc->dt_pin, BLUSYS_GPIO_INTERRUPT_DISABLED);
        blusys_gpio_set_callback(enc->clk_pin, NULL, NULL);
        blusys_gpio_set_interrupt(enc->clk_pin, BLUSYS_GPIO_INTERRUPT_DISABLED);
        return err;
    }

    return BLUSYS_OK;
}

static void encoder_gpio_deinit(blusys_encoder_t *enc)
{
    blusys_gpio_set_callback(enc->dt_pin, NULL, NULL);
    blusys_gpio_set_callback(enc->clk_pin, NULL, NULL);
}

#endif /* SOC_PCNT_SUPPORTED */

/* ── public API ───────────────────────────────────────────────────────────── */

blusys_err_t blusys_encoder_open(const blusys_encoder_config_t *config,
                                  blusys_encoder_t **out_encoder)
{
    blusys_encoder_t *enc;
    blusys_err_t err;
    esp_err_t esp_err;

    if ((config == NULL) || (out_encoder == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (!GPIO_IS_VALID_GPIO(config->clk_pin) ||
        !GPIO_IS_VALID_GPIO(config->dt_pin)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    enc = calloc(1, sizeof(*enc));
    if (enc == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    enc->clk_pin          = config->clk_pin;
    enc->dt_pin           = config->dt_pin;
    enc->steps_per_detent = (config->steps_per_detent > 0)
                          ? config->steps_per_detent
                          : ENCODER_DEFAULT_STEPS_PER_DETENT;
    enc->state_lock       = (portMUX_TYPE)portMUX_INITIALIZER_UNLOCKED;

    err = blusys_lock_init(&enc->lock);
    if (err != BLUSYS_OK) {
        free(enc);
        return err;
    }

    /* Event timer: deferred callback dispatch in task context */
    const esp_timer_create_args_t timer_args = {
#if SOC_PCNT_SUPPORTED
        .callback        = event_timer_cb,
#else
        .callback        = gpio_debounce_timer_cb,
#endif
        .arg             = enc,
        .dispatch_method = ESP_TIMER_TASK,
        .name            = "enc_event",
    };
    esp_err = esp_timer_create(&timer_args, &enc->event_timer);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_lock;
    }

#if SOC_PCNT_SUPPORTED
    err = encoder_pcnt_init(enc, config->glitch_filter_ns);
    if (err != BLUSYS_OK) {
        goto fail_timer;
    }
#else
    err = encoder_gpio_init(enc);
    if (err != BLUSYS_OK) {
        goto fail_timer;
    }
#endif

    /* Optional push button */
    if (config->sw_pin >= 0) {
        const blusys_button_config_t btn_cfg = {
            .pin           = config->sw_pin,
            .pull_mode     = BLUSYS_GPIO_PULL_UP,
            .active_low    = true,
            .debounce_ms   = config->debounce_ms,
            .long_press_ms = config->long_press_ms,
        };
        err = blusys_button_open(&btn_cfg, &enc->button);
        if (err != BLUSYS_OK) {
            goto fail_hw;
        }

        err = blusys_button_set_callback(enc->button, encoder_button_cb, enc);
        if (err != BLUSYS_OK) {
            blusys_button_close(enc->button);
            goto fail_hw;
        }
    }

    *out_encoder = enc;
    return BLUSYS_OK;

fail_hw:
#if SOC_PCNT_SUPPORTED
    encoder_pcnt_deinit(enc);
#else
    encoder_gpio_deinit(enc);
#endif
fail_timer:
    esp_timer_delete(enc->event_timer);
fail_lock:
    blusys_lock_deinit(&enc->lock);
    free(enc);
    return err;
}

blusys_err_t blusys_encoder_close(blusys_encoder_t *encoder)
{
    if (encoder == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&encoder->state_lock);
    encoder->closing = true;
    portEXIT_CRITICAL(&encoder->state_lock);

    if (encoder->button != NULL) {
        blusys_button_close(encoder->button);
    }

#if SOC_PCNT_SUPPORTED
    encoder_pcnt_deinit(encoder);
#else
    encoder_gpio_deinit(encoder);
#endif

    esp_timer_stop(encoder->event_timer);

    while (encoder->callback_active > 0u) {
        taskYIELD();
    }

    esp_timer_delete(encoder->event_timer);
    blusys_lock_deinit(&encoder->lock);
    free(encoder);
    return BLUSYS_OK;
}

blusys_err_t blusys_encoder_set_callback(blusys_encoder_t *encoder,
                                          blusys_encoder_callback_t callback,
                                          void *user_ctx)
{
    blusys_err_t err;

    if (encoder == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&encoder->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    portENTER_CRITICAL(&encoder->state_lock);
    if (encoder->closing) {
        portEXIT_CRITICAL(&encoder->state_lock);
        blusys_lock_give(&encoder->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }
    encoder->callback = callback;
    encoder->user_ctx = user_ctx;
    portEXIT_CRITICAL(&encoder->state_lock);

    while (encoder->callback_active > 0u) {
        taskYIELD();
    }

    blusys_lock_give(&encoder->lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_encoder_get_position(blusys_encoder_t *encoder,
                                          int *out_position)
{
    if ((encoder == NULL) || (out_position == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&encoder->state_lock);
    *out_position = (int)encoder->position;
    portEXIT_CRITICAL(&encoder->state_lock);

    return BLUSYS_OK;
}

blusys_err_t blusys_encoder_reset_position(blusys_encoder_t *encoder)
{
    if (encoder == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&encoder->state_lock);
    encoder->position = 0;
    portEXIT_CRITICAL(&encoder->state_lock);

    return BLUSYS_OK;
}
