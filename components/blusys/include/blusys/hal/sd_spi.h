#ifndef BLUSYS_SD_SPI_H
#define BLUSYS_SD_SPI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_sd_spi blusys_sd_spi_t;

/**
 * Callback invoked once per directory entry by blusys_sd_spi_listdir().
 *
 * @param name       Entry name (file or subdirectory), no leading path.
 * @param user_data  Caller-supplied context pointer.
 */
typedef void (*blusys_sd_spi_listdir_cb_t)(const char *name, void *user_data);

typedef struct {
    const char *base_path;              /**< VFS mount point, e.g. "/sdcard" (required) */
    int         spi_host;               /**< SPI bus: SPI2_HOST=1, SPI3_HOST=2 */
    int         mosi_pin;               /**< MOSI GPIO pin */
    int         miso_pin;               /**< MISO GPIO pin */
    int         sclk_pin;               /**< Clock GPIO pin */
    int         cs_pin;                 /**< Chip-select GPIO pin */
    uint32_t    freq_hz;                /**< SPI clock in Hz; 0 = 20 MHz */
    size_t      max_files;              /**< Max simultaneously open files; 0 = 5 */
    bool        format_if_mount_failed; /**< Auto-format SD card if mount fails */
    size_t      allocation_unit_size;   /**< FAT cluster size in bytes; 0 = 16384 */
} blusys_sd_spi_config_t;

/**
 * Mount a FAT filesystem on an SD card connected over SPI.
 *
 * Initialises the SPI bus, probes the card, and registers the FAT filesystem
 * with the VFS layer so that standard C file functions work under base_path.
 *
 * @param config  Mount configuration; base_path, spi_host, and pin fields are required.
 * @param out_sd  Receives the handle on success.
 * @return BLUSYS_OK, BLUSYS_ERR_INVALID_ARG, BLUSYS_ERR_NO_MEM, or BLUSYS_ERR_IO.
 */
blusys_err_t blusys_sd_spi_mount(const blusys_sd_spi_config_t *config,
                                  blusys_sd_spi_t **out_sd);

/**
 * Unmount the filesystem, deinitialise the SPI bus, and free memory.
 *
 * @param sd  Handle from blusys_sd_spi_mount().
 * @return BLUSYS_OK or an error code.
 */
blusys_err_t blusys_sd_spi_unmount(blusys_sd_spi_t *sd);

/**
 * Write (overwrite) a file with the given data.
 *
 * @param sd    SD card handle.
 * @param path  Relative path, e.g. "logs/boot.txt".
 * @param data  Bytes to write.
 * @param size  Number of bytes.
 * @return BLUSYS_OK or BLUSYS_ERR_IO.
 */
blusys_err_t blusys_sd_spi_write(blusys_sd_spi_t *sd, const char *path,
                                  const void *data, size_t size);

/**
 * Read a file into the provided buffer.
 *
 * @param sd             SD card handle.
 * @param path           Relative path.
 * @param buf            Destination buffer.
 * @param buf_size       Capacity of buf in bytes.
 * @param out_bytes_read Receives actual bytes read.
 * @return BLUSYS_OK or BLUSYS_ERR_IO.
 */
blusys_err_t blusys_sd_spi_read(blusys_sd_spi_t *sd, const char *path,
                                 void *buf, size_t buf_size,
                                 size_t *out_bytes_read);

/**
 * Append data to a file (creates it if absent).
 *
 * @param sd    SD card handle.
 * @param path  Relative path.
 * @param data  Bytes to append.
 * @param size  Number of bytes.
 * @return BLUSYS_OK or BLUSYS_ERR_IO.
 */
blusys_err_t blusys_sd_spi_append(blusys_sd_spi_t *sd, const char *path,
                                   const void *data, size_t size);

/**
 * Delete a file.
 *
 * @param sd    SD card handle.
 * @param path  Relative path.
 * @return BLUSYS_OK or BLUSYS_ERR_IO if the file does not exist.
 */
blusys_err_t blusys_sd_spi_remove(blusys_sd_spi_t *sd, const char *path);

/**
 * Check whether a path (file or directory) exists.
 *
 * @param sd          SD card handle.
 * @param path        Relative path.
 * @param out_exists  Set to true if the path exists.
 * @return BLUSYS_OK unless an argument is NULL.
 */
blusys_err_t blusys_sd_spi_exists(blusys_sd_spi_t *sd, const char *path,
                                   bool *out_exists);

/**
 * Get the size of a file in bytes.
 *
 * @param sd        SD card handle.
 * @param path      Relative path.
 * @param out_size  Receives the file size.
 * @return BLUSYS_OK or BLUSYS_ERR_IO if the file does not exist.
 */
blusys_err_t blusys_sd_spi_size(blusys_sd_spi_t *sd, const char *path,
                                 size_t *out_size);

/**
 * Create a directory (single level only).
 *
 * Treats EEXIST as success — the call is idempotent.
 *
 * @param sd    SD card handle.
 * @param path  Relative directory path, e.g. "logs".
 * @return BLUSYS_OK, or BLUSYS_ERR_IO if creation fails.
 */
blusys_err_t blusys_sd_spi_mkdir(blusys_sd_spi_t *sd, const char *path);

/**
 * Iterate entries in a directory, invoking cb for each one.
 *
 * Entries "." and ".." are skipped. The lock is held for the entire
 * iteration — the callback must not call back into the same handle.
 *
 * @param sd         SD card handle.
 * @param path       Relative directory path; NULL or "" lists the mount root.
 * @param cb         Callback invoked once per entry.
 * @param user_data  Passed through to cb unchanged.
 * @return BLUSYS_OK, or BLUSYS_ERR_IO if the directory cannot be opened.
 */
blusys_err_t blusys_sd_spi_listdir(blusys_sd_spi_t *sd, const char *path,
                                    blusys_sd_spi_listdir_cb_t cb, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_SD_SPI_H */
