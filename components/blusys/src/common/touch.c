#include "blusys/touch.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "soc/soc_caps.h"

#if SOC_TOUCH_SENSOR_SUPPORTED

#include "soc/touch_sensor_channel.h"

#include "blusys/internal/blusys_esp_err.h"
#include "blusys/internal/blusys_lock.h"

#include "driver/touch_sens.h"
#include "freertos/FreeRTOS.h"

#define BLUSYS_TOUCH_SAMPLE_CFG_NUM 1u
#define BLUSYS_TOUCH_INIT_SCAN_COUNT 3
#define BLUSYS_TOUCH_INIT_SCAN_TIMEOUT_MS 2000

struct blusys_touch {
    int pin;
    int channel_id;
    bool enabled;
    bool scanning;
    bool closing;
    touch_sensor_handle_t sensor_handle;
    touch_channel_handle_t channel_handle;
    blusys_lock_t lock;
};

static portMUX_TYPE blusys_touch_state_lock = portMUX_INITIALIZER_UNLOCKED;
static bool blusys_touch_controller_in_use;

static bool blusys_touch_try_acquire_controller(void)
{
    bool acquired = false;

    portENTER_CRITICAL(&blusys_touch_state_lock);
    if (!blusys_touch_controller_in_use) {
        blusys_touch_controller_in_use = true;
        acquired = true;
    }
    portEXIT_CRITICAL(&blusys_touch_state_lock);

    return acquired;
}

static void blusys_touch_release_controller(void)
{
    portENTER_CRITICAL(&blusys_touch_state_lock);
    blusys_touch_controller_in_use = false;
    portEXIT_CRITICAL(&blusys_touch_state_lock);
}

static blusys_err_t blusys_touch_resolve_pin(int pin, int *out_channel_id)
{
    if (out_channel_id == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

#if SOC_TOUCH_SENSOR_VERSION == 1
    switch (pin) {
    case TOUCH_PAD_NUM0_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO4_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM1_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO0_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM2_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO2_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM3_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO15_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM4_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO13_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM5_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO12_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM6_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO14_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM7_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO27_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM8_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO33_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM9_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO32_CHANNEL;
        return BLUSYS_OK;
    default:
        return BLUSYS_ERR_INVALID_ARG;
    }
#elif SOC_TOUCH_SENSOR_VERSION == 2
    switch (pin) {
    case TOUCH_PAD_NUM1_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO1_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM2_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO2_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM3_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO3_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM4_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO4_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM5_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO5_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM6_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO6_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM7_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO7_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM8_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO8_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM9_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO9_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM10_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO10_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM11_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO11_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM12_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO12_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM13_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO13_CHANNEL;
        return BLUSYS_OK;
    case TOUCH_PAD_NUM14_GPIO_NUM:
        *out_channel_id = TOUCH_PAD_GPIO14_CHANNEL;
        return BLUSYS_OK;
    default:
        return BLUSYS_ERR_INVALID_ARG;
    }
#else
    (void) pin;
    return BLUSYS_ERR_NOT_SUPPORTED;
#endif
}

static void blusys_touch_set_enabled(blusys_touch_t *touch, bool enabled)
{
    touch->enabled = enabled;
}

static void blusys_touch_set_scanning(blusys_touch_t *touch, bool scanning)
{
    touch->scanning = scanning;
}

static blusys_err_t blusys_touch_start_scanning(blusys_touch_t *touch)
{
    blusys_err_t err;
    touch_sensor_sample_config_t sample_config[BLUSYS_TOUCH_SAMPLE_CFG_NUM] = {
#if SOC_TOUCH_SENSOR_VERSION == 1
        TOUCH_SENSOR_V1_DEFAULT_SAMPLE_CONFIG(5.0f, TOUCH_VOLT_LIM_L_0V5, TOUCH_VOLT_LIM_H_1V7),
#else
        TOUCH_SENSOR_V2_DEFAULT_SAMPLE_CONFIG(500, TOUCH_VOLT_LIM_L_0V5, TOUCH_VOLT_LIM_H_2V2),
#endif
    };
    touch_sensor_config_t sensor_config = TOUCH_SENSOR_DEFAULT_BASIC_CONFIG(BLUSYS_TOUCH_SAMPLE_CFG_NUM,
                                                                            sample_config);
    touch_channel_config_t channel_config = {
#if SOC_TOUCH_SENSOR_VERSION == 1
        .abs_active_thresh = {1000u},
        .group = TOUCH_CHAN_TRIG_GROUP_BOTH,
#else
        .active_thresh = {2000u},
#endif
        .charge_speed = TOUCH_CHARGE_SPEED_7,
        .init_charge_volt = TOUCH_INIT_CHARGE_VOLT_DEFAULT,
    };
    touch_sensor_filter_config_t filter_config = TOUCH_SENSOR_DEFAULT_FILTER_CONFIG();
    int scan_index;

    err = blusys_translate_esp_err(touch_sensor_new_controller(&sensor_config, &touch->sensor_handle));
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_translate_esp_err(touch_sensor_new_channel(touch->sensor_handle,
                                                            touch->channel_id,
                                                            &channel_config,
                                                            &touch->channel_handle));
    if (err != BLUSYS_OK) {
        goto fail_controller;
    }

    err = blusys_translate_esp_err(touch_sensor_config_filter(touch->sensor_handle, &filter_config));
    if (err != BLUSYS_OK) {
        goto fail_channel;
    }

    err = blusys_translate_esp_err(touch_sensor_enable(touch->sensor_handle));
    if (err != BLUSYS_OK) {
        goto fail_channel;
    }
    blusys_touch_set_enabled(touch, true);

    for (scan_index = 0; scan_index < BLUSYS_TOUCH_INIT_SCAN_COUNT; ++scan_index) {
        err = blusys_translate_esp_err(touch_sensor_trigger_oneshot_scanning(touch->sensor_handle,
                                                                             BLUSYS_TOUCH_INIT_SCAN_TIMEOUT_MS));
        if (err != BLUSYS_OK) {
            goto fail_enabled;
        }
    }

    err = blusys_translate_esp_err(touch_sensor_start_continuous_scanning(touch->sensor_handle));
    if (err != BLUSYS_OK) {
        goto fail_enabled;
    }
    blusys_touch_set_scanning(touch, true);

    return BLUSYS_OK;

fail_enabled:
    if (touch->enabled) {
        touch_sensor_disable(touch->sensor_handle);
        blusys_touch_set_enabled(touch, false);
    }
fail_channel:
    touch_sensor_del_channel(touch->channel_handle);
    touch->channel_handle = NULL;
fail_controller:
    touch_sensor_del_controller(touch->sensor_handle);
    touch->sensor_handle = NULL;
    return err;
}

blusys_err_t blusys_touch_open(int pin, blusys_touch_t **out_touch)
{
    blusys_touch_t *touch;
    blusys_err_t err;
    int channel_id;

    if (out_touch == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_touch_resolve_pin(pin, &channel_id);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (!blusys_touch_try_acquire_controller()) {
        return BLUSYS_ERR_BUSY;
    }

    touch = calloc(1, sizeof(*touch));
    if (touch == NULL) {
        blusys_touch_release_controller();
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&touch->lock);
    if (err != BLUSYS_OK) {
        free(touch);
        blusys_touch_release_controller();
        return err;
    }

    touch->pin = pin;

    touch->channel_id = channel_id;

    err = blusys_touch_start_scanning(touch);
    if (err != BLUSYS_OK) {
        goto fail_lock;
    }

    *out_touch = touch;
    return BLUSYS_OK;

fail_lock:
    blusys_lock_deinit(&touch->lock);
    free(touch);
    blusys_touch_release_controller();
    return err;
}

blusys_err_t blusys_touch_close(blusys_touch_t *touch)
{
    blusys_err_t err;

    if (touch == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&touch->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    touch->closing = true;

    if (touch->scanning) {
        err = blusys_translate_esp_err(touch_sensor_stop_continuous_scanning(touch->sensor_handle));
        if (err != BLUSYS_OK) {
            touch->closing = false;
            blusys_lock_give(&touch->lock);
            return err;
        }
        blusys_touch_set_scanning(touch, false);
    }

    if (touch->enabled) {
        err = blusys_translate_esp_err(touch_sensor_disable(touch->sensor_handle));
        if (err != BLUSYS_OK) {
            touch->closing = false;
            blusys_lock_give(&touch->lock);
            return err;
        }
        blusys_touch_set_enabled(touch, false);
    }

    err = blusys_translate_esp_err(touch_sensor_del_channel(touch->channel_handle));
    if (err != BLUSYS_OK) {
        touch->closing = false;
        blusys_lock_give(&touch->lock);
        return err;
    }
    touch->channel_handle = NULL;

    err = blusys_translate_esp_err(touch_sensor_del_controller(touch->sensor_handle));
    if (err != BLUSYS_OK) {
        touch->closing = false;
        blusys_lock_give(&touch->lock);
        return err;
    }
    touch->sensor_handle = NULL;

    blusys_lock_give(&touch->lock);
    blusys_lock_deinit(&touch->lock);
    blusys_touch_release_controller();
    free(touch);
    return BLUSYS_OK;
}

blusys_err_t blusys_touch_read(blusys_touch_t *touch, uint32_t *out_value)
{
    blusys_err_t err;

    if ((touch == NULL) || (out_value == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&touch->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (touch->closing || !touch->scanning) {
        blusys_lock_give(&touch->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_translate_esp_err(touch_channel_read_data(touch->channel_handle,
                                                           TOUCH_CHAN_DATA_TYPE_SMOOTH,
                                                           out_value));

    blusys_lock_give(&touch->lock);
    return err;
}

#else

blusys_err_t blusys_touch_open(int pin, blusys_touch_t **out_touch)
{
    (void) pin;
    (void) out_touch;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_touch_close(blusys_touch_t *touch)
{
    (void) touch;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_touch_read(blusys_touch_t *touch, uint32_t *out_value)
{
    (void) touch;
    (void) out_value;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif
