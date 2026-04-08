#include "blusys/ulp.h"

#include <stddef.h>

#include "driver/rtc_io.h"

#include "blusys/internal/blusys_esp_err.h"
#include "blusys/internal/blusys_lock.h"

#if CONFIG_ULP_COPROC_ENABLED && CONFIG_ULP_COPROC_TYPE_FSM && \
    (CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S3)

#include "ulp.h"
#include "blusys_ulp_gpio_watch.h"

enum {
    BLUSYS_ULP_SIGNATURE = 0x4255,
    BLUSYS_ULP_LAST_VALUE_UNKNOWN = 0xffff,
};

extern const uint8_t blusys_ulp_gpio_watch_bin_start[] asm("_binary_blusys_ulp_gpio_watch_bin_start");
extern const uint8_t blusys_ulp_gpio_watch_bin_end[] asm("_binary_blusys_ulp_gpio_watch_bin_end");

static portMUX_TYPE  s_ulp_init_lock = portMUX_INITIALIZER_UNLOCKED;
static blusys_lock_t s_ulp_lock;
static bool          s_ulp_lock_inited;
static bool          s_ulp_configured;
static int           s_ulp_gpio_num = -1;

static blusys_err_t blusys_ulp_ensure_lock(void)
{
    if (s_ulp_lock_inited) {
        return BLUSYS_OK;
    }

    blusys_lock_t new_lock;
    blusys_err_t err = blusys_lock_init(&new_lock);
    if (err != BLUSYS_OK) {
        return err;
    }

    portENTER_CRITICAL(&s_ulp_init_lock);
    if (!s_ulp_lock_inited) {
        s_ulp_lock = new_lock;
        s_ulp_lock_inited = true;
        portEXIT_CRITICAL(&s_ulp_init_lock);
    } else {
        portEXIT_CRITICAL(&s_ulp_init_lock);
        blusys_lock_deinit(&new_lock);
    }

    return BLUSYS_OK;
}

static void blusys_ulp_reset_result_locked(void)
{
    blusys_ulp_gpio_watch_run_count = 0;
    blusys_ulp_gpio_watch_last_level = BLUSYS_ULP_LAST_VALUE_UNKNOWN;
    blusys_ulp_gpio_watch_match_count = 0;
    blusys_ulp_gpio_watch_wake_triggered = 0;
}

static void blusys_ulp_release_gpio_locked(void)
{
    if (s_ulp_gpio_num < 0) {
        return;
    }

    gpio_num_t gpio_num = (gpio_num_t) s_ulp_gpio_num;
    rtc_gpio_hold_dis(gpio_num);
    rtc_gpio_deinit(gpio_num);
    s_ulp_gpio_num = -1;
}

static blusys_err_t blusys_ulp_load_program_locked(void)
{
    esp_err_t esp_err = ulp_load_binary(
        0,
        blusys_ulp_gpio_watch_bin_start,
        (size_t) (blusys_ulp_gpio_watch_bin_end - blusys_ulp_gpio_watch_bin_start) / sizeof(uint32_t));

    return blusys_translate_esp_err(esp_err);
}

blusys_err_t blusys_ulp_gpio_watch_configure(const blusys_ulp_gpio_watch_config_t *config)
{
    if (config == NULL || config->pin < 0 || config->period_ms == 0 || config->stable_samples == 0 ||
        config->period_ms > (UINT32_MAX / 1000u)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    gpio_num_t gpio_num = (gpio_num_t) config->pin;
    if (!rtc_gpio_is_valid_gpio(gpio_num)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_ulp_ensure_lock();
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&s_ulp_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    ulp_timer_stop();
    blusys_ulp_release_gpio_locked();

    err = blusys_ulp_load_program_locked();
    if (err != BLUSYS_OK) {
        blusys_lock_give(&s_ulp_lock);
        return err;
    }

    int rtc_io_num = rtc_io_number_get(gpio_num);
    if (rtc_io_num < 0) {
        blusys_lock_give(&s_ulp_lock);
        return BLUSYS_ERR_INVALID_ARG;
    }

    esp_err_t esp_err = rtc_gpio_init(gpio_num);
    if (esp_err == ESP_OK) {
        esp_err = rtc_gpio_set_direction(gpio_num, RTC_GPIO_MODE_INPUT_ONLY);
    }
    if (esp_err == ESP_OK) {
        esp_err = rtc_gpio_pullup_dis(gpio_num);
    }
    if (esp_err == ESP_OK) {
        esp_err = rtc_gpio_pulldown_dis(gpio_num);
    }
    if (esp_err == ESP_OK) {
        esp_err = rtc_gpio_hold_en(gpio_num);
    }
    if (esp_err != ESP_OK) {
        rtc_gpio_deinit(gpio_num);
        blusys_lock_give(&s_ulp_lock);
        return blusys_translate_esp_err(esp_err);
    }

    s_ulp_gpio_num = config->pin;

    blusys_ulp_gpio_watch_signature = BLUSYS_ULP_SIGNATURE;
    blusys_ulp_gpio_watch_job = BLUSYS_ULP_JOB_GPIO_WATCH;
    blusys_ulp_gpio_watch_io_number = (uint32_t) rtc_io_num;
    blusys_ulp_gpio_watch_wake_level = config->wake_on_high ? 1u : 0u;
    blusys_ulp_gpio_watch_required_match_count = config->stable_samples;
    blusys_ulp_gpio_watch_period_ms = config->period_ms;
    blusys_ulp_reset_result_locked();

    s_ulp_configured = true;

    blusys_lock_give(&s_ulp_lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_ulp_start(void)
{
    blusys_err_t err = blusys_ulp_ensure_lock();
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&s_ulp_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (!s_ulp_configured) {
        blusys_lock_give(&s_ulp_lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    blusys_ulp_reset_result_locked();

    esp_err_t esp_err = ulp_set_wakeup_period(0, blusys_ulp_gpio_watch_period_ms * 1000u);
    if (esp_err == ESP_OK) {
        esp_err = ulp_run(&blusys_ulp_gpio_watch_entry - RTC_SLOW_MEM);
    }

    blusys_lock_give(&s_ulp_lock);
    return blusys_translate_esp_err(esp_err);
}

blusys_err_t blusys_ulp_stop(void)
{
    blusys_err_t err = blusys_ulp_ensure_lock();
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&s_ulp_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    ulp_timer_stop();
    blusys_ulp_release_gpio_locked();
    s_ulp_configured = false;

    blusys_lock_give(&s_ulp_lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_ulp_clear_result(void)
{
    blusys_err_t err = blusys_ulp_ensure_lock();
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&s_ulp_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (blusys_ulp_gpio_watch_signature == BLUSYS_ULP_SIGNATURE) {
        blusys_ulp_reset_result_locked();
    }

    blusys_lock_give(&s_ulp_lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_ulp_get_result(blusys_ulp_result_t *out_result)
{
    if (out_result == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_ulp_ensure_lock();
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&s_ulp_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    out_result->valid = (blusys_ulp_gpio_watch_signature == BLUSYS_ULP_SIGNATURE) &&
                        (blusys_ulp_gpio_watch_job == BLUSYS_ULP_JOB_GPIO_WATCH);
    out_result->job = out_result->valid ? (blusys_ulp_job_t) blusys_ulp_gpio_watch_job : BLUSYS_ULP_JOB_NONE;
    out_result->run_count = blusys_ulp_gpio_watch_run_count & UINT16_MAX;
    out_result->last_value = (blusys_ulp_gpio_watch_last_level & UINT16_MAX) == BLUSYS_ULP_LAST_VALUE_UNKNOWN
                                 ? -1
                                 : (int32_t) (blusys_ulp_gpio_watch_last_level & 1u);
    out_result->wake_triggered = (blusys_ulp_gpio_watch_wake_triggered & 1u) != 0;

    blusys_lock_give(&s_ulp_lock);
    return BLUSYS_OK;
}

#else

blusys_err_t blusys_ulp_gpio_watch_configure(const blusys_ulp_gpio_watch_config_t *config)
{
    (void) config;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_ulp_start(void)
{
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_ulp_stop(void)
{
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_ulp_clear_result(void)
{
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_ulp_get_result(blusys_ulp_result_t *out_result)
{
    if (out_result == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    out_result->valid = false;
    out_result->job = BLUSYS_ULP_JOB_NONE;
    out_result->run_count = 0;
    out_result->last_value = -1;
    out_result->wake_triggered = false;

    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif
