#include "blusys/hal/temp_sensor.h"

#include <stdlib.h>

#include "soc/soc_caps.h"

#if SOC_TEMP_SENSOR_SUPPORTED

#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/lock.h"

#include "driver/temperature_sensor.h"

struct blusys_temp_sensor {
    temperature_sensor_handle_t handle;
    blusys_lock_t lock;
};

blusys_err_t blusys_temp_sensor_open(blusys_temp_sensor_t **out_tsens)
{
    blusys_temp_sensor_t *tsens;
    blusys_err_t err;
    esp_err_t esp_err;

    if (out_tsens == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    tsens = calloc(1, sizeof(*tsens));
    if (tsens == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&tsens->lock);
    if (err != BLUSYS_OK) {
        free(tsens);
        return err;
    }

    temperature_sensor_config_t cfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);

    esp_err = temperature_sensor_install(&cfg, &tsens->handle);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        blusys_lock_deinit(&tsens->lock);
        free(tsens);
        return err;
    }

    esp_err = temperature_sensor_enable(tsens->handle);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        temperature_sensor_uninstall(tsens->handle);
        blusys_lock_deinit(&tsens->lock);
        free(tsens);
        return err;
    }

    *out_tsens = tsens;
    return BLUSYS_OK;
}

blusys_err_t blusys_temp_sensor_close(blusys_temp_sensor_t *tsens)
{
    blusys_err_t err;

    if (tsens == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&tsens->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    temperature_sensor_disable(tsens->handle);
    temperature_sensor_uninstall(tsens->handle);

    blusys_lock_give(&tsens->lock);
    blusys_lock_deinit(&tsens->lock);
    free(tsens);
    return BLUSYS_OK;
}

blusys_err_t blusys_temp_sensor_read_celsius(blusys_temp_sensor_t *tsens, float *out_celsius)
{
    blusys_err_t err;
    esp_err_t esp_err;

    if ((tsens == NULL) || (out_celsius == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&tsens->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err = temperature_sensor_get_celsius(tsens->handle, out_celsius);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&tsens->lock);
    return err;
}

#else /* !SOC_TEMP_SENSOR_SUPPORTED */

blusys_err_t blusys_temp_sensor_open(blusys_temp_sensor_t **out_tsens)
{
    (void) out_tsens;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_temp_sensor_close(blusys_temp_sensor_t *tsens)
{
    (void) tsens;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_temp_sensor_read_celsius(blusys_temp_sensor_t *tsens, float *out_celsius)
{
    (void) tsens;
    (void) out_celsius;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif /* SOC_TEMP_SENSOR_SUPPORTED */
