#include "blusys/storage/fatfs.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_vfs_fat.h"
#include "wear_levelling.h"

#include "blusys/internal/blusys_esp_err.h"
#include "blusys/internal/blusys_lock.h"

#define BLUSYS_FATFS_MAX_PATH          256
#define BLUSYS_FATFS_DEFAULT_MAX_FILES   5
#define BLUSYS_FATFS_DEFAULT_ALLOC_UNIT  4096
#define BLUSYS_FATFS_DEFAULT_LABEL      "ffat"
#define BLUSYS_FATFS_BASE_PATH_MAX       32
#define BLUSYS_FATFS_LABEL_MAX           17

struct blusys_fatfs {
    char          base_path[BLUSYS_FATFS_BASE_PATH_MAX];
    char          partition_label[BLUSYS_FATFS_LABEL_MAX];
    wl_handle_t   wl_handle;
    blusys_lock_t lock;
};

static blusys_err_t build_path(const blusys_fatfs_t *h, const char *rel,
                               char *out, size_t out_size)
{
    int n;
    if (rel == NULL || rel[0] == '\0') {
        n = snprintf(out, out_size, "%s", h->base_path);
    } else {
        n = snprintf(out, out_size, "%s/%s", h->base_path, rel);
    }
    if (n < 0 || (size_t)n >= out_size) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    return BLUSYS_OK;
}

blusys_err_t blusys_fatfs_mount(const blusys_fatfs_config_t *config,
                                blusys_fatfs_t **out_fatfs)
{
    if (config == NULL || config->base_path == NULL || out_fatfs == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_fatfs_t *h = calloc(1, sizeof(*h));
    if (h == NULL) {
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
        err = blusys_translate_esp_err(esp_err);
        blusys_lock_deinit(&h->lock);
        free(h);
        return err;
    }

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
    err = blusys_translate_esp_err(esp_err);

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
    blusys_err_t err = build_path(fatfs, path, full, sizeof(full));
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&fatfs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    FILE *f = fopen(full, "wb");
    if (f == NULL) {
        blusys_lock_give(&fatfs->lock);
        return BLUSYS_ERR_IO;
    }

    if (size > 0) {
        size_t written = fwrite(data, 1, size, f);
        err = (written == size) ? BLUSYS_OK : BLUSYS_ERR_IO;
    }

    fclose(f);
    blusys_lock_give(&fatfs->lock);
    return err;
}

blusys_err_t blusys_fatfs_read(blusys_fatfs_t *fatfs, const char *path,
                               void *buf, size_t buf_size,
                               size_t *out_bytes_read)
{
    if (fatfs == NULL || path == NULL || buf == NULL || out_bytes_read == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_FATFS_MAX_PATH];
    blusys_err_t err = build_path(fatfs, path, full, sizeof(full));
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&fatfs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    FILE *f = fopen(full, "rb");
    if (f == NULL) {
        blusys_lock_give(&fatfs->lock);
        return BLUSYS_ERR_IO;
    }

    *out_bytes_read = fread(buf, 1, buf_size, f);
    err = ferror(f) ? BLUSYS_ERR_IO : BLUSYS_OK;

    fclose(f);
    blusys_lock_give(&fatfs->lock);
    return err;
}

blusys_err_t blusys_fatfs_append(blusys_fatfs_t *fatfs, const char *path,
                                 const void *data, size_t size)
{
    if (fatfs == NULL || path == NULL || (data == NULL && size > 0)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_FATFS_MAX_PATH];
    blusys_err_t err = build_path(fatfs, path, full, sizeof(full));
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&fatfs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    FILE *f = fopen(full, "ab");
    if (f == NULL) {
        blusys_lock_give(&fatfs->lock);
        return BLUSYS_ERR_IO;
    }

    if (size > 0) {
        size_t written = fwrite(data, 1, size, f);
        err = (written == size) ? BLUSYS_OK : BLUSYS_ERR_IO;
    }

    fclose(f);
    blusys_lock_give(&fatfs->lock);
    return err;
}

blusys_err_t blusys_fatfs_remove(blusys_fatfs_t *fatfs, const char *path)
{
    if (fatfs == NULL || path == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_FATFS_MAX_PATH];
    blusys_err_t err = build_path(fatfs, path, full, sizeof(full));
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&fatfs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = (remove(full) == 0) ? BLUSYS_OK : BLUSYS_ERR_IO;

    blusys_lock_give(&fatfs->lock);
    return err;
}

blusys_err_t blusys_fatfs_exists(blusys_fatfs_t *fatfs, const char *path,
                                 bool *out_exists)
{
    if (fatfs == NULL || path == NULL || out_exists == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_FATFS_MAX_PATH];
    blusys_err_t err = build_path(fatfs, path, full, sizeof(full));
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&fatfs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    struct stat st;
    *out_exists = (stat(full, &st) == 0);

    blusys_lock_give(&fatfs->lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_fatfs_size(blusys_fatfs_t *fatfs, const char *path,
                               size_t *out_size)
{
    if (fatfs == NULL || path == NULL || out_size == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_FATFS_MAX_PATH];
    blusys_err_t err = build_path(fatfs, path, full, sizeof(full));
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&fatfs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    struct stat st;
    if (stat(full, &st) != 0) {
        blusys_lock_give(&fatfs->lock);
        return BLUSYS_ERR_IO;
    }

    *out_size = (size_t)st.st_size;

    blusys_lock_give(&fatfs->lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_fatfs_mkdir(blusys_fatfs_t *fatfs, const char *path)
{
    if (fatfs == NULL || path == NULL || path[0] == '\0') {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_FATFS_MAX_PATH];
    blusys_err_t err = build_path(fatfs, path, full, sizeof(full));
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&fatfs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (mkdir(full, 0755) != 0) {
        err = (errno == EEXIST) ? BLUSYS_OK : BLUSYS_ERR_IO;
    }

    blusys_lock_give(&fatfs->lock);
    return err;
}

blusys_err_t blusys_fatfs_listdir(blusys_fatfs_t *fatfs, const char *path,
                                  blusys_fatfs_listdir_cb_t cb, void *user_data)
{
    if (fatfs == NULL || cb == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_FATFS_MAX_PATH];
    blusys_err_t err = build_path(fatfs, path, full, sizeof(full));
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&fatfs->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    DIR *dir = opendir(full);
    if (dir == NULL) {
        blusys_lock_give(&fatfs->lock);
        return BLUSYS_ERR_IO;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        cb(entry->d_name, user_data);
    }

    closedir(dir);
    blusys_lock_give(&fatfs->lock);
    return BLUSYS_OK;
}
