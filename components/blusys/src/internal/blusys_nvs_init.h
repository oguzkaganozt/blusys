#ifndef BLUSYS_NVS_INIT_H
#define BLUSYS_NVS_INIT_H

#include "nvs_flash.h"

#include "blusys/error.h"
#include "blusys_esp_err.h"

/**
 * Shared NVS initialization used by wifi, espnow, and ble_gatt.
 *
 * nvs_flash_init() is idempotent — calling it multiple times is safe and
 * returns ESP_OK if already initialized.  This helper centralises the
 * erase-and-retry logic so every module handles the two recoverable
 * error codes identically.
 */
static inline blusys_err_t blusys_nvs_ensure_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        return blusys_translate_esp_err(err);
    }
    return BLUSYS_OK;
}

#endif
