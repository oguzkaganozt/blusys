#include "blusys/services/storage/fs.h"

#include <stdlib.h>
#include <string.h>

#include "esp_spiffs.h"

#include "blusys/observe/counter.h"
#include "blusys/observe/log.h"
#include "blusys/services/internal/storage_helpers.h"
#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/lock.h"

#define BLUSYS_FS_MAX_PATH     128
#define BLUSYS_FS_DEFAULT_MAX_FILES 5

#define FS_DOMAIN err_domain_storage_fs

struct blusys_fs {
    char          base_path[32];
    char          partition_label[17]; /* stored to unregister the correct SPIFFS partition */
    blusys_lock_t lock;
};

blusys_err_t blusys_fs_mount(const blusys_fs_config_t *config, blusys_fs_t **out_fs)
{
    blusys_fs_t *h;
    blusys_err_t err;
    esp_err_t esp_err;

    if (config == NULL || config->base_path == NULL || out_fs == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    h = calloc(1, sizeof(*h));
    if (h == NULL) {
        BLUSYS_LOG(BLUSYS_LOG_ERROR, FS_DOMAIN, "op=mount alloc-failed bytes=%zu",
                   sizeof(*h));
        blusys_counter_inc(FS_DOMAIN, 1);
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        free(h);
        return err;
    }

    strncpy(h->base_path, config->base_path, sizeof(h->base_path) - 1);
    if (config->partition_label != NULL) {
        strncpy(h->partition_label, config->partition_label, sizeof(h->partition_label) - 1);
    }

    esp_vfs_spiffs_conf_t spiffs_conf = {
        .base_path              = h->base_path,
        .partition_label        = (h->partition_label[0] != '\0') ? h->partition_label : NULL,
        .max_files              = (config->max_files > 0) ? config->max_files
                                                          : BLUSYS_FS_DEFAULT_MAX_FILES,
        .format_if_mount_failed = config->format_if_mount_failed,
    };

    esp_err = esp_vfs_spiffs_register(&spiffs_conf);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err_in(FS_DOMAIN, esp_err);
        BLUSYS_LOG(BLUSYS_LOG_ERROR, FS_DOMAIN,
                   "op=mount base=%s esp_err=0x%x", h->base_path, (unsigned)esp_err);
        blusys_counter_inc(FS_DOMAIN, 1);
        blusys_lock_deinit(&h->lock);
        free(h);
        return err;
    }

    BLUSYS_LOG(BLUSYS_LOG_INFO, FS_DOMAIN, "mounted base=%s", h->base_path);
    *out_fs = h;
    return BLUSYS_OK;
}

blusys_err_t blusys_fs_unmount(blusys_fs_t *fs)
{
    blusys_err_t err;

    if (fs == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&fs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    esp_err_t esp_err = esp_vfs_spiffs_unregister(
        (fs->partition_label[0] != '\0') ? fs->partition_label : NULL);
    err = blusys_translate_esp_err_in(FS_DOMAIN, esp_err);
    if (err != BLUSYS_OK) {
        BLUSYS_LOG(BLUSYS_LOG_WARN, FS_DOMAIN, "op=unmount esp_err=0x%x",
                   (unsigned)esp_err);
        blusys_counter_inc(FS_DOMAIN, 1);
    }

    blusys_lock_give(&fs->lock);
    blusys_lock_deinit(&fs->lock);
    free(fs);
    return err;
}

blusys_err_t blusys_fs_write(blusys_fs_t *fs, const char *path, const void *data, size_t size)
{
    blusys_err_t err;
    char full[BLUSYS_FS_MAX_PATH];

    if (fs == NULL || path == NULL || (data == NULL && size > 0)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_storage_build_path(fs->base_path, path, full, sizeof(full), FS_DOMAIN);
    if (err != BLUSYS_OK) {
        return err;
    }

    return blusys_storage_write(&fs->lock, FS_DOMAIN, full, path, data, size);
}

blusys_err_t blusys_fs_read(blusys_fs_t *fs, const char *path, void *buf, size_t buf_size,
                             size_t *out_bytes_read)
{
    blusys_err_t err;
    char full[BLUSYS_FS_MAX_PATH];

    if (fs == NULL || path == NULL || buf == NULL || out_bytes_read == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_storage_build_path(fs->base_path, path, full, sizeof(full), FS_DOMAIN);
    if (err != BLUSYS_OK) {
        return err;
    }

    return blusys_storage_read(&fs->lock, FS_DOMAIN, full, path, buf, buf_size, out_bytes_read);
}

blusys_err_t blusys_fs_append(blusys_fs_t *fs, const char *path, const void *data, size_t size)
{
    blusys_err_t err;
    char full[BLUSYS_FS_MAX_PATH];

    if (fs == NULL || path == NULL || (data == NULL && size > 0)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_storage_build_path(fs->base_path, path, full, sizeof(full), FS_DOMAIN);
    if (err != BLUSYS_OK) {
        return err;
    }

    return blusys_storage_append(&fs->lock, FS_DOMAIN, full, path, data, size);
}

blusys_err_t blusys_fs_remove(blusys_fs_t *fs, const char *path)
{
    blusys_err_t err;
    char full[BLUSYS_FS_MAX_PATH];

    if (fs == NULL || path == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_storage_build_path(fs->base_path, path, full, sizeof(full), FS_DOMAIN);
    if (err != BLUSYS_OK) {
        return err;
    }

    return blusys_storage_remove(&fs->lock, FS_DOMAIN, full, path);
}

blusys_err_t blusys_fs_exists(blusys_fs_t *fs, const char *path, bool *out_exists)
{
    blusys_err_t err;
    char full[BLUSYS_FS_MAX_PATH];

    if (fs == NULL || path == NULL || out_exists == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_storage_build_path(fs->base_path, path, full, sizeof(full), FS_DOMAIN);
    if (err != BLUSYS_OK) {
        return err;
    }

    return blusys_storage_exists(&fs->lock, full, out_exists);
}

blusys_err_t blusys_fs_size(blusys_fs_t *fs, const char *path, size_t *out_size)
{
    blusys_err_t err;
    char full[BLUSYS_FS_MAX_PATH];

    if (fs == NULL || path == NULL || out_size == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_storage_build_path(fs->base_path, path, full, sizeof(full), FS_DOMAIN);
    if (err != BLUSYS_OK) {
        return err;
    }

    return blusys_storage_size(&fs->lock, FS_DOMAIN, full, path, out_size);
}
