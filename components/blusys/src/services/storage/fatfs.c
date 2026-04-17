#include "blusys/services/storage/fatfs.h"

#include <stdlib.h>
#include <string.h>

#include "esp_vfs_fat.h"
#include "wear_levelling.h"

#include "blusys/framework/services/internal/storage_helpers.h"
#include "blusys/framework/observe/counter.h"
#include "blusys/framework/observe/log.h"
#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/lock.h"

#define BLUSYS_FATFS_MAX_PATH          256
#define BLUSYS_FATFS_DEFAULT_MAX_FILES   5
#define BLUSYS_FATFS_DEFAULT_ALLOC_UNIT  4096
#define BLUSYS_FATFS_DEFAULT_LABEL      "ffat"
#define BLUSYS_FATFS_BASE_PATH_MAX       32
#define BLUSYS_FATFS_LABEL_MAX           17

#define FATFS_DOMAIN err_domain_storage_fatfs

struct blusys_fatfs {
    char          base_path[BLUSYS_FATFS_BASE_PATH_MAX];
    char          partition_label[BLUSYS_FATFS_LABEL_MAX];
    wl_handle_t   wl_handle;
    blusys_lock_t lock;
};

blusys_err_t blusys_fatfs_mount(const blusys_fatfs_config_t *config,
                                blusys_fatfs_t **out_fatfs)
{
    if (config == NULL || config->base_path == NULL || out_fatfs == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_fatfs_t *h = calloc(1, sizeof(*h));
    if (h == NULL) {
        BLUSYS_LOG(BLUSYS_LOG_ERROR, FATFS_DOMAIN,
                   "op=mount alloc-failed bytes=%zu", sizeof(*h));
        blusys_counter_inc(FATFS_DOMAIN, 1);
        return BLUSYS_ERR_NO_MEM;
    }

    blusys_err_t err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        free(h);
        return err;
    }

    strncpy(h->base_path, config->base_path, sizeof(h->base_path) - 1);

    const char *label = (config->partition_label != NULL)
                        ? config->partition_label
                        : BLUSYS_FATFS_DEFAULT_LABEL;
    strncpy(h->partition_label, label, sizeof(h->partition_label) - 1);

    h->wl_handle = WL_INVALID_HANDLE;

    esp_vfs_fat_mount_config_t mount_cfg = {
        .format_if_mount_failed   = config->format_if_mount_failed,
        .max_files                = (config->max_files > 0)
                                    ? (int)config->max_files
                                    : BLUSYS_FATFS_DEFAULT_MAX_FILES,
        .allocation_unit_size     = (config->allocation_unit_size > 0)
                                    ? config->allocation_unit_size
                                    : BLUSYS_FATFS_DEFAULT_ALLOC_UNIT,
        .disk_status_check_enable = false,
        .use_one_fat              = false,
    };

    esp_err_t esp_err = esp_vfs_fat_spiflash_mount_rw_wl(
        h->base_path,
        h->partition_label,
        &mount_cfg,
        &h->wl_handle);

    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err_in(FATFS_DOMAIN, esp_err);
        BLUSYS_LOG(BLUSYS_LOG_ERROR, FATFS_DOMAIN,
                   "op=mount base=%s label=%s esp_err=0x%x",
                   h->base_path, h->partition_label, (unsigned)esp_err);
        blusys_counter_inc(FATFS_DOMAIN, 1);
        blusys_lock_deinit(&h->lock);
        free(h);
        return err;
    }

    BLUSYS_LOG(BLUSYS_LOG_INFO, FATFS_DOMAIN,
               "mounted base=%s label=%s", h->base_path, h->partition_label);
    *out_fatfs = h;
    return BLUSYS_OK;
}

blusys_err_t blusys_fatfs_unmount(blusys_fatfs_t *fatfs)
{
    if (fatfs == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&fatfs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err_t esp_err = esp_vfs_fat_spiflash_unmount_rw_wl(
        fatfs->base_path, fatfs->wl_handle);
    err = blusys_translate_esp_err_in(FATFS_DOMAIN, esp_err);
    if (err != BLUSYS_OK) {
        BLUSYS_LOG(BLUSYS_LOG_WARN, FATFS_DOMAIN,
                   "op=unmount esp_err=0x%x", (unsigned)esp_err);
        blusys_counter_inc(FATFS_DOMAIN, 1);
    }

    blusys_lock_give(&fatfs->lock);
    blusys_lock_deinit(&fatfs->lock);
    free(fatfs);
    return err;
}

blusys_err_t blusys_fatfs_write(blusys_fatfs_t *fatfs, const char *path,
                                const void *data, size_t size)
{
    if (fatfs == NULL || path == NULL || (data == NULL && size > 0)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_FATFS_MAX_PATH];
    blusys_err_t err = blusys_storage_build_path(fatfs->base_path, path, full, sizeof(full), FATFS_DOMAIN);
    if (err != BLUSYS_OK) {
        return err;
    }

    return blusys_storage_write(&fatfs->lock, FATFS_DOMAIN, full, path, data, size);
}

blusys_err_t blusys_fatfs_read(blusys_fatfs_t *fatfs, const char *path,
                               void *buf, size_t buf_size,
                               size_t *out_bytes_read)
{
    if (fatfs == NULL || path == NULL || buf == NULL || out_bytes_read == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_FATFS_MAX_PATH];
    blusys_err_t err = blusys_storage_build_path(fatfs->base_path, path, full, sizeof(full), FATFS_DOMAIN);
    if (err != BLUSYS_OK) {
        return err;
    }

    return blusys_storage_read(&fatfs->lock, FATFS_DOMAIN, full, path, buf, buf_size, out_bytes_read);
}

blusys_err_t blusys_fatfs_append(blusys_fatfs_t *fatfs, const char *path,
                                 const void *data, size_t size)
{
    if (fatfs == NULL || path == NULL || (data == NULL && size > 0)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_FATFS_MAX_PATH];
    blusys_err_t err = blusys_storage_build_path(fatfs->base_path, path, full, sizeof(full), FATFS_DOMAIN);
    if (err != BLUSYS_OK) {
        return err;
    }

    return blusys_storage_append(&fatfs->lock, FATFS_DOMAIN, full, path, data, size);
}

blusys_err_t blusys_fatfs_remove(blusys_fatfs_t *fatfs, const char *path)
{
    if (fatfs == NULL || path == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_FATFS_MAX_PATH];
    blusys_err_t err = blusys_storage_build_path(fatfs->base_path, path, full, sizeof(full), FATFS_DOMAIN);
    if (err != BLUSYS_OK) {
        return err;
    }

    return blusys_storage_remove(&fatfs->lock, FATFS_DOMAIN, full, path);
}

blusys_err_t blusys_fatfs_exists(blusys_fatfs_t *fatfs, const char *path,
                                 bool *out_exists)
{
    if (fatfs == NULL || path == NULL || out_exists == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_FATFS_MAX_PATH];
    blusys_err_t err = blusys_storage_build_path(fatfs->base_path, path, full, sizeof(full), FATFS_DOMAIN);
    if (err != BLUSYS_OK) {
        return err;
    }

    return blusys_storage_exists(&fatfs->lock, full, out_exists);
}

blusys_err_t blusys_fatfs_size(blusys_fatfs_t *fatfs, const char *path,
                               size_t *out_size)
{
    if (fatfs == NULL || path == NULL || out_size == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_FATFS_MAX_PATH];
    blusys_err_t err = blusys_storage_build_path(fatfs->base_path, path, full, sizeof(full), FATFS_DOMAIN);
    if (err != BLUSYS_OK) {
        return err;
    }

    return blusys_storage_size(&fatfs->lock, FATFS_DOMAIN, full, path, out_size);
}

blusys_err_t blusys_fatfs_mkdir(blusys_fatfs_t *fatfs, const char *path)
{
    if (fatfs == NULL || path == NULL || path[0] == '\0') {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_FATFS_MAX_PATH];
    blusys_err_t err = blusys_storage_build_path(fatfs->base_path, path, full, sizeof(full), FATFS_DOMAIN);
    if (err != BLUSYS_OK) {
        return err;
    }

    return blusys_storage_mkdir(&fatfs->lock, FATFS_DOMAIN, full, path);
}

blusys_err_t blusys_fatfs_listdir(blusys_fatfs_t *fatfs, const char *path,
                                  blusys_fatfs_listdir_cb_t cb, void *user_data)
{
    if (fatfs == NULL || cb == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_FATFS_MAX_PATH];
    blusys_err_t err = blusys_storage_build_path(fatfs->base_path, path, full, sizeof(full), FATFS_DOMAIN);
    if (err != BLUSYS_OK) {
        return err;
    }

    return blusys_storage_listdir(&fatfs->lock, FATFS_DOMAIN, full, path,
                                  (blusys_storage_listdir_cb_t)cb, user_data);
}
