#include "blusys/hal/pwm.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/lock.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"

#define BLUSYS_PWM_MAX_HANDLES 4
#define BLUSYS_PWM_DUTY_RESOLUTION LEDC_TIMER_10_BIT
#define BLUSYS_PWM_MAX_DUTY ((1u << BLUSYS_PWM_DUTY_RESOLUTION) - 1u)

struct blusys_pwm {
    int pin;
    ledc_channel_t channel;
    ledc_timer_t timer;
    uint16_t duty_permille;
    bool started;
    blusys_lock_t lock;
};

static portMUX_TYPE blusys_pwm_alloc_lock = portMUX_INITIALIZER_UNLOCKED;
static bool blusys_pwm_slot_in_use[BLUSYS_PWM_MAX_HANDLES];

static bool blusys_pwm_is_valid_pin(int pin)
{
    return GPIO_IS_VALID_OUTPUT_GPIO(pin);
}

static bool blusys_pwm_is_valid_duty(uint16_t duty_permille)
{
    return duty_permille <= 1000u;
}

static uint32_t blusys_pwm_permille_to_duty(uint16_t duty_permille)
{
    return ((uint32_t) duty_permille * BLUSYS_PWM_MAX_DUTY) / 1000u;
}

static blusys_err_t blusys_pwm_apply_duty(blusys_pwm_t *pwm)
{
    if (!pwm->started) {
        return BLUSYS_OK;
    }

    return blusys_translate_esp_err(ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE,
                                                             pwm->channel,
                                                             blusys_pwm_permille_to_duty(pwm->duty_permille),
                                                             0));
}

static blusys_err_t blusys_pwm_alloc_slot(ledc_channel_t *out_channel, ledc_timer_t *out_timer)
{
    unsigned int slot;

    if ((out_channel == NULL) || (out_timer == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&blusys_pwm_alloc_lock);
    for (slot = 0; slot < BLUSYS_PWM_MAX_HANDLES; ++slot) {
        if (!blusys_pwm_slot_in_use[slot]) {
            blusys_pwm_slot_in_use[slot] = true;
            *out_channel = (ledc_channel_t) slot;
            *out_timer = (ledc_timer_t) slot;
            portEXIT_CRITICAL(&blusys_pwm_alloc_lock);
            return BLUSYS_OK;
        }
    }
    portEXIT_CRITICAL(&blusys_pwm_alloc_lock);

    return BLUSYS_ERR_BUSY;
}

static void blusys_pwm_free_slot(ledc_channel_t channel)
{
    unsigned int slot = (unsigned int) channel;

    if (slot >= BLUSYS_PWM_MAX_HANDLES) {
        return;
    }

    portENTER_CRITICAL(&blusys_pwm_alloc_lock);
    blusys_pwm_slot_in_use[slot] = false;
    portEXIT_CRITICAL(&blusys_pwm_alloc_lock);
}

blusys_err_t blusys_pwm_open(int pin,
                             uint32_t freq_hz,
                             uint16_t duty_permille,
                             blusys_pwm_t **out_pwm)
{
    ledc_timer_config_t timer_config;
    ledc_channel_config_t channel_config;
    blusys_pwm_t *pwm;
    blusys_err_t err;
    esp_err_t esp_err;

    if (!blusys_pwm_is_valid_pin(pin) ||
        (freq_hz == 0u) ||
        !blusys_pwm_is_valid_duty(duty_permille) ||
        (out_pwm == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    pwm = calloc(1, sizeof(*pwm));
    if (pwm == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&pwm->lock);
    if (err != BLUSYS_OK) {
        free(pwm);
        return err;
    }

    err = blusys_pwm_alloc_slot(&pwm->channel, &pwm->timer);
    if (err != BLUSYS_OK) {
        blusys_lock_deinit(&pwm->lock);
        free(pwm);
        return err;
    }

    pwm->pin = pin;
    pwm->duty_permille = duty_permille;
    pwm->started = false;

    timer_config = (ledc_timer_config_t) {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = BLUSYS_PWM_DUTY_RESOLUTION,
        .timer_num = pwm->timer,
        .freq_hz = freq_hz,
        .clk_cfg = LEDC_AUTO_CLK,
    };

    esp_err = ledc_timer_config(&timer_config);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail;
    }

    channel_config = (ledc_channel_config_t) {
        .gpio_num = pin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = pwm->channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = pwm->timer,
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
    };

    esp_err = ledc_channel_config(&channel_config);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_timer;
    }

    *out_pwm = pwm;
    return BLUSYS_OK;

fail_timer:
    ledc_timer_pause(LEDC_LOW_SPEED_MODE, pwm->timer);
    ledc_timer_config(&(ledc_timer_config_t) {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = pwm->timer,
        .deconfigure = true,
    });
fail:
    blusys_pwm_free_slot(pwm->channel);
    blusys_lock_deinit(&pwm->lock);
    free(pwm);
    return err;
}

blusys_err_t blusys_pwm_close(blusys_pwm_t *pwm)
{
    blusys_err_t err;
    esp_err_t esp_err;

    if (pwm == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&pwm->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err = ledc_stop(LEDC_LOW_SPEED_MODE, pwm->channel, 0u);
    if (esp_err == ESP_OK) {
        esp_err = gpio_reset_pin((gpio_num_t) pwm->pin);
    }
    if (esp_err == ESP_OK) {
        esp_err = ledc_timer_pause(LEDC_LOW_SPEED_MODE, pwm->timer);
    }
    if (esp_err == ESP_OK) {
        esp_err = ledc_timer_config(&(ledc_timer_config_t) {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .timer_num = pwm->timer,
            .deconfigure = true,
        });
    }

    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&pwm->lock);

    if (err == BLUSYS_OK) {
        blusys_pwm_free_slot(pwm->channel);
        blusys_lock_deinit(&pwm->lock);
        free(pwm);
    }

    return err;
}

blusys_err_t blusys_pwm_set_frequency(blusys_pwm_t *pwm, uint32_t freq_hz)
{
    blusys_err_t err;

    if ((pwm == NULL) || (freq_hz == 0u)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&pwm->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_translate_esp_err(ledc_set_freq(LEDC_LOW_SPEED_MODE, pwm->timer, freq_hz));
    if (err == BLUSYS_OK) {
        err = blusys_pwm_apply_duty(pwm);
    }

    blusys_lock_give(&pwm->lock);
    return err;
}

blusys_err_t blusys_pwm_set_duty(blusys_pwm_t *pwm, uint16_t duty_permille)
{
    blusys_err_t err;

    if ((pwm == NULL) || !blusys_pwm_is_valid_duty(duty_permille)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&pwm->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    pwm->duty_permille = duty_permille;
    err = blusys_pwm_apply_duty(pwm);

    blusys_lock_give(&pwm->lock);
    return err;
}

blusys_err_t blusys_pwm_start(blusys_pwm_t *pwm)
{
    blusys_err_t err;

    if (pwm == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&pwm->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    bool was_started = pwm->started;

    pwm->started = true;
    err = blusys_pwm_apply_duty(pwm);
    if (err != BLUSYS_OK) {
        pwm->started = was_started;
    }

    blusys_lock_give(&pwm->lock);
    return err;
}

blusys_err_t blusys_pwm_stop(blusys_pwm_t *pwm)
{
    blusys_err_t err;

    if (pwm == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&pwm->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_translate_esp_err(ledc_stop(LEDC_LOW_SPEED_MODE, pwm->channel, 0u));
    if (err == BLUSYS_OK) {
        pwm->started = false;
    }

    blusys_lock_give(&pwm->lock);
    return err;
}
