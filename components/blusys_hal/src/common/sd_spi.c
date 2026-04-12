#include "blusys/sd_spi.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "driver/gpio.h"
#include "driver/sdspi_host.h"
#include "driver/spi_master.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include "blusys/internal/blusys_esp_err.h"
#include "blusys/internal/blusys_lock.h"

#define BLUSYS_SD_SPI_MAX_PATH         256
#define BLUSYS_SD_SPI_DEFAULT_MAX_FILES  5
#define BLUSYS_SD_SPI_DEFAULT_FREQ_HZ    20000000u
#define BLUSYS_SD_SPI_BASE_PATH_MAX      32
#define BLUSYS_SD_SPI_MAX_TRANSFER_SZ    4000
/* SPI1_HOST (0) is reserved for internal flash; user must pass >= 1 */
#define BLUSYS_SD_SPI_MIN_HOST           1

struct blusys_sd_spi {
    char          base_path[BLUSYS_SD_SPI_BASE_PATH_MAX];
    int           spi_host;
    sdmmc_card_t *card;
    bool          closing;
    blusys_lock_t lock;
};

static bool is_valid_output_pin(int pin)
{
    return GPIO_IS_VALID_OUTPUT_GPIO(pin);
}

static bool is_valid_input_pin(int pin)
{
    return GPIO_IS_VALID_GPIO(pin);
}

static blusys_err_t build_path(const blusys_sd_spi_t *h, const char *rel,
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

blusys_err_t blusys_sd_spi_mount(const blusys_sd_spi_config_t *config,
                                  blusys_sd_spi_t **out_sd)
{
    if (config == NULL || config->base_path == NULL || out_sd == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (config->spi_host < BLUSYS_SD_SPI_MIN_HOST ||
        !is_valid_output_pin(config->mosi_pin) ||
        !is_valid_input_pin(config->miso_pin) ||
        !is_valid_output_pin(config->sclk_pin) ||
        !is_valid_output_pin(config->cs_pin)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (strlen(config->base_path) >= BLUSYS_SD_SPI_BASE_PATH_MAX) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_sd_spi_t *h = calloc(1, sizeof(*h));
    if (h == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    blusys_err_t err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        free(h);
        return err;
    }

    strncpy(h->base_path, config->base_path, sizeof(h->base_path) - 1);
    h->spi_host = config->spi_host;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = config->mosi_pin,
        .miso_io_num     = config->miso_pin,
        .sclk_io_num     = config->sclk_pin,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = BLUSYS_SD_SPI_MAX_TRANSFER_SZ,
    };

    esp_err_t esp_err = spi_bus_initialize((spi_host_device_t) config->spi_host,
                                            &bus_cfg, SPI_DMA_CH_AUTO);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        blusys_lock_deinit(&h->lock);
        free(h);
        return err;
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = config->spi_host;

    uint32_t freq_hz = (config->freq_hz > 0u) ? config->freq_hz
                                               : BLUSYS_SD_SPI_DEFAULT_FREQ_HZ;
    host.max_freq_khz = (int) (freq_hz / 1000u);

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = (gpio_num_t) config->cs_pin;
    slot_config.host_id = (spi_host_device_t) config->spi_host;

    esp_vfs_fat_mount_config_t mount_cfg = {
        .format_if_mount_failed   = config->format_if_mount_failed,
        .max_files                = (config->max_files > 0u)
                                    ? (int) config->max_files
                                    : BLUSYS_SD_SPI_DEFAULT_MAX_FILES,
        .allocation_unit_size     = (config->allocation_unit_size > 0u)
                                    ? config->allocation_unit_size
                                    : 16 * 1024,
        .disk_status_check_enable = false,
    };

    esp_err = esp_vfs_fat_sdspi_mount(h->base_path, &host, &slot_config,
                                      &mount_cfg, &h->card);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        spi_bus_free((spi_host_device_t) config->spi_host);
        blusys_lock_deinit(&h->lock);
        free(h);
        return err;
    }

    *out_sd = h;
    return BLUSYS_OK;
}

blusys_err_t blusys_sd_spi_unmount(blusys_sd_spi_t *sd)
{
    if (sd == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&sd->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    sd->closing = true;

    esp_err_t esp_err = esp_vfs_fat_sdcard_unmount(sd->base_path, sd->card);
    err = blusys_translate_esp_err(esp_err);

    if (err != BLUSYS_OK) {
        sd->closing = false;
        blusys_lock_give(&sd->lock);
        return err;
    }

    spi_bus_free((spi_host_device_t) sd->spi_host);

    blusys_lock_give(&sd->lock);
    blusys_lock_deinit(&sd->lock);
    free(sd);
    return BLUSYS_OK;
}

blusys_err_t blusys_sd_spi_write(blusys_sd_spi_t *sd, const char *path,
                                  const void *data, size_t size)
{
    if (sd == NULL || path == NULL || (data == NULL && size > 0)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_SD_SPI_MAX_PATH];
    blusys_err_t err = build_path(sd, path, full, sizeof(full));
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&sd->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (sd->closing) {
        blusys_lock_give(&sd->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    FILE *f = fopen(full, "wb");
    if (f == NULL) {
        blusys_lock_give(&sd->lock);
        return BLUSYS_ERR_IO;
    }

    if (size > 0) {
        size_t written = fwrite(data, 1, size, f);
        err = (written == size) ? BLUSYS_OK : BLUSYS_ERR_IO;
    }

    fclose(f);
    blusys_lock_give(&sd->lock);
    return err;
}

blusys_err_t blusys_sd_spi_read(blusys_sd_spi_t *sd, const char *path,
                                 void *buf, size_t buf_size,
                                 size_t *out_bytes_read)
{
    if (sd == NULL || path == NULL || buf == NULL || out_bytes_read == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_SD_SPI_MAX_PATH];
    blusys_err_t err = build_path(sd, path, full, sizeof(full));
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&sd->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (sd->closing) {
        blusys_lock_give(&sd->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    FILE *f = fopen(full, "rb");
    if (f == NULL) {
        blusys_lock_give(&sd->lock);
        return BLUSYS_ERR_IO;
    }

    *out_bytes_read = fread(buf, 1, buf_size, f);
    err = ferror(f) ? BLUSYS_ERR_IO : BLUSYS_OK;

    fclose(f);
    blusys_lock_give(&sd->lock);
    return err;
}

blusys_err_t blusys_sd_spi_append(blusys_sd_spi_t *sd, const char *path,
                                   const void *data, size_t size)
{
    if (sd == NULL || path == NULL || (data == NULL && size > 0)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_SD_SPI_MAX_PATH];
    blusys_err_t err = build_path(sd, path, full, sizeof(full));
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&sd->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (sd->closing) {
        blusys_lock_give(&sd->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    FILE *f = fopen(full, "ab");
    if (f == NULL) {
        blusys_lock_give(&sd->lock);
        return BLUSYS_ERR_IO;
    }

    if (size > 0) {
        size_t written = fwrite(data, 1, size, f);
        err = (written == size) ? BLUSYS_OK : BLUSYS_ERR_IO;
    }

    fclose(f);
    blusys_lock_give(&sd->lock);
    return err;
}

blusys_err_t blusys_sd_spi_remove(blusys_sd_spi_t *sd, const char *path)
{
    if (sd == NULL || path == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_SD_SPI_MAX_PATH];
    blusys_err_t err = build_path(sd, path, full, sizeof(full));
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&sd->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (sd->closing) {
        blusys_lock_give(&sd->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    err = (remove(full) == 0) ? BLUSYS_OK : BLUSYS_ERR_IO;

    blusys_lock_give(&sd->lock);
    return err;
}

blusys_err_t blusys_sd_spi_exists(blusys_sd_spi_t *sd, const char *path,
                                   bool *out_exists)
{
    if (sd == NULL || path == NULL || out_exists == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_SD_SPI_MAX_PATH];
    blusys_err_t err = build_path(sd, path, full, sizeof(full));
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&sd->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (sd->closing) {
        blusys_lock_give(&sd->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    struct stat st;
    *out_exists = (stat(full, &st) == 0);

    blusys_lock_give(&sd->lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_sd_spi_size(blusys_sd_spi_t *sd, const char *path,
                                 size_t *out_size)
{
    if (sd == NULL || path == NULL || out_size == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_SD_SPI_MAX_PATH];
    blusys_err_t err = build_path(sd, path, full, sizeof(full));
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&sd->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (sd->closing) {
        blusys_lock_give(&sd->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    struct stat st;
    if (stat(full, &st) != 0) {
        blusys_lock_give(&sd->lock);
        return BLUSYS_ERR_IO;
    }

    *out_size = (size_t) st.st_size;

    blusys_lock_give(&sd->lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_sd_spi_mkdir(blusys_sd_spi_t *sd, const char *path)
{
    if (sd == NULL || path == NULL || path[0] == '\0') {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_SD_SPI_MAX_PATH];
    blusys_err_t err = build_path(sd, path, full, sizeof(full));
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&sd->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (sd->closing) {
        blusys_lock_give(&sd->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    if (mkdir(full, 0755) != 0) {
        err = (errno == EEXIST) ? BLUSYS_OK : BLUSYS_ERR_IO;
    }

    blusys_lock_give(&sd->lock);
    return err;
}

blusys_err_t blusys_sd_spi_listdir(blusys_sd_spi_t *sd, const char *path,
                                    blusys_sd_spi_listdir_cb_t cb, void *user_data)
{
    if (sd == NULL || cb == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    char full[BLUSYS_SD_SPI_MAX_PATH];
    blusys_err_t err = build_path(sd, path, full, sizeof(full));
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&sd->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (sd->closing) {
        blusys_lock_give(&sd->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    DIR *dir = opendir(full);
    if (dir == NULL) {
        blusys_lock_give(&sd->lock);
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
    blusys_lock_give(&sd->lock);
    return BLUSYS_OK;
}
