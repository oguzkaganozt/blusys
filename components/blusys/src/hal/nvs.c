#include "blusys/hal/nvs.h"

#include <stdlib.h>

#include "nvs.h"
#include "nvs_flash.h"

#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/lock.h"
#include "blusys/hal/internal/nvs_init.h"

struct blusys_nvs {
    nvs_handle_t      handle;
    blusys_lock_t     lock;
    blusys_nvs_mode_t mode;
};

blusys_err_t blusys_nvs_open(const char *namespace_name, blusys_nvs_mode_t mode, blusys_nvs_t **out_nvs)
{
    blusys_nvs_t *h;
    blusys_err_t err;
    esp_err_t esp_err;

    if (namespace_name == NULL || out_nvs == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    h = calloc(1, sizeof(*h));
    if (h == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        free(h);
        return err;
    }

    err = blusys_nvs_ensure_init();
    if (err != BLUSYS_OK) {
        blusys_lock_deinit(&h->lock);
        free(h);
        return err;
    }

    nvs_open_mode_t esp_mode = (mode == BLUSYS_NVS_READWRITE) ? NVS_READWRITE : NVS_READONLY;
    esp_err = nvs_open(namespace_name, esp_mode, &h->handle);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        blusys_lock_deinit(&h->lock);
        free(h);
        return err;
    }

    h->mode = mode;
    *out_nvs = h;
    return BLUSYS_OK;
}

blusys_err_t blusys_nvs_close(blusys_nvs_t *nvs)
{
    blusys_err_t err;

    if (nvs == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&nvs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    nvs_close(nvs->handle);

    blusys_lock_give(&nvs->lock);
    blusys_lock_deinit(&nvs->lock);
    free(nvs);
    return BLUSYS_OK;
}

blusys_err_t blusys_nvs_commit(blusys_nvs_t *nvs)
{
    blusys_err_t err;

    if (nvs == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (nvs->mode == BLUSYS_NVS_READONLY) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_lock_take(&nvs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err_t esp_err = nvs_commit(nvs->handle);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&nvs->lock);
    return err;
}

blusys_err_t blusys_nvs_erase_key(blusys_nvs_t *nvs, const char *key)
{
    blusys_err_t err;

    if (nvs == NULL || key == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (nvs->mode == BLUSYS_NVS_READONLY) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_lock_take(&nvs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err_t esp_err = nvs_erase_key(nvs->handle, key);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&nvs->lock);
    return err;
}

blusys_err_t blusys_nvs_erase_all(blusys_nvs_t *nvs)
{
    blusys_err_t err;

    if (nvs == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (nvs->mode == BLUSYS_NVS_READONLY) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_lock_take(&nvs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err_t esp_err = nvs_erase_all(nvs->handle);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&nvs->lock);
    return err;
}

/* --- Integer getters --- */

blusys_err_t blusys_nvs_get_u8(blusys_nvs_t *nvs, const char *key, uint8_t *out_value)
{
    blusys_err_t err;

    if (nvs == NULL || key == NULL || out_value == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&nvs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err_t esp_err = nvs_get_u8(nvs->handle, key, out_value);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&nvs->lock);
    return err;
}

blusys_err_t blusys_nvs_get_u32(blusys_nvs_t *nvs, const char *key, uint32_t *out_value)
{
    blusys_err_t err;

    if (nvs == NULL || key == NULL || out_value == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&nvs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err_t esp_err = nvs_get_u32(nvs->handle, key, out_value);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&nvs->lock);
    return err;
}

blusys_err_t blusys_nvs_get_i32(blusys_nvs_t *nvs, const char *key, int32_t *out_value)
{
    blusys_err_t err;

    if (nvs == NULL || key == NULL || out_value == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&nvs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err_t esp_err = nvs_get_i32(nvs->handle, key, out_value);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&nvs->lock);
    return err;
}

blusys_err_t blusys_nvs_get_u64(blusys_nvs_t *nvs, const char *key, uint64_t *out_value)
{
    blusys_err_t err;

    if (nvs == NULL || key == NULL || out_value == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&nvs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err_t esp_err = nvs_get_u64(nvs->handle, key, out_value);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&nvs->lock);
    return err;
}

/* --- Integer setters --- */

blusys_err_t blusys_nvs_set_u8(blusys_nvs_t *nvs, const char *key, uint8_t value)
{
    blusys_err_t err;

    if (nvs == NULL || key == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (nvs->mode == BLUSYS_NVS_READONLY) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_lock_take(&nvs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err_t esp_err = nvs_set_u8(nvs->handle, key, value);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&nvs->lock);
    return err;
}

blusys_err_t blusys_nvs_set_u32(blusys_nvs_t *nvs, const char *key, uint32_t value)
{
    blusys_err_t err;

    if (nvs == NULL || key == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (nvs->mode == BLUSYS_NVS_READONLY) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_lock_take(&nvs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err_t esp_err = nvs_set_u32(nvs->handle, key, value);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&nvs->lock);
    return err;
}

blusys_err_t blusys_nvs_set_i32(blusys_nvs_t *nvs, const char *key, int32_t value)
{
    blusys_err_t err;

    if (nvs == NULL || key == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (nvs->mode == BLUSYS_NVS_READONLY) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_lock_take(&nvs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err_t esp_err = nvs_set_i32(nvs->handle, key, value);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&nvs->lock);
    return err;
}

blusys_err_t blusys_nvs_set_u64(blusys_nvs_t *nvs, const char *key, uint64_t value)
{
    blusys_err_t err;

    if (nvs == NULL || key == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (nvs->mode == BLUSYS_NVS_READONLY) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_lock_take(&nvs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err_t esp_err = nvs_set_u64(nvs->handle, key, value);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&nvs->lock);
    return err;
}

/* --- String --- */

blusys_err_t blusys_nvs_get_str(blusys_nvs_t *nvs, const char *key, char *out_value, size_t *length)
{
    blusys_err_t err;

    if (nvs == NULL || key == NULL || length == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&nvs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err_t esp_err = nvs_get_str(nvs->handle, key, out_value, length);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&nvs->lock);
    return err;
}

blusys_err_t blusys_nvs_set_str(blusys_nvs_t *nvs, const char *key, const char *value)
{
    blusys_err_t err;

    if (nvs == NULL || key == NULL || value == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (nvs->mode == BLUSYS_NVS_READONLY) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_lock_take(&nvs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err_t esp_err = nvs_set_str(nvs->handle, key, value);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&nvs->lock);
    return err;
}

/* --- Blob --- */

blusys_err_t blusys_nvs_get_blob(blusys_nvs_t *nvs, const char *key, void *out_value, size_t *length)
{
    blusys_err_t err;

    if (nvs == NULL || key == NULL || length == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&nvs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err_t esp_err = nvs_get_blob(nvs->handle, key, out_value, length);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&nvs->lock);
    return err;
}

blusys_err_t blusys_nvs_set_blob(blusys_nvs_t *nvs, const char *key, const void *value, size_t length)
{
    blusys_err_t err;

    if (nvs == NULL || key == NULL || value == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (nvs->mode == BLUSYS_NVS_READONLY) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = blusys_lock_take(&nvs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err_t esp_err = nvs_set_blob(nvs->handle, key, value, length);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&nvs->lock);
    return err;
}
