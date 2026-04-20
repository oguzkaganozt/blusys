/**
 * @file sd_spi.h
 * @brief SD card mounted as a FAT filesystem over a SPI bus.
 *
 * Registers the FAT filesystem under a VFS base path so that standard C
 * `fopen`/`fread`/… also work. Convenience helpers are provided for common
 * read/write/append/listdir operations. See docs/hal/sd_spi.md.
 */

#ifndef BLUSYS_SD_SPI_H
#define BLUSYS_SD_SPI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle to a mounted SD-over-SPI filesystem. */
typedef struct blusys_sd_spi blusys_sd_spi_t;

/**
 * @brief Callback invoked once per directory entry by ::blusys_sd_spi_listdir.
 * @param name       Entry name (file or subdirectory), no leading path.
 * @param user_data  Caller-supplied context pointer.
 */
typedef void (*blusys_sd_spi_listdir_cb_t)(const char *name, void *user_data);

/** @brief Configuration for ::blusys_sd_spi_mount. */
typedef struct {
    const char *base_path;              /**< VFS mount point, e.g. `"/sdcard"` (required). */
    int         spi_host;               /**< SPI host index: `SPI2_HOST=1`, `SPI3_HOST=2`. */
    int         mosi_pin;               /**< MOSI GPIO pin. */
    int         miso_pin;               /**< MISO GPIO pin. */
    int         sclk_pin;               /**< Clock GPIO pin. */
    int         cs_pin;                 /**< Chip-select GPIO pin. */
    uint32_t    freq_hz;                /**< SPI clock in Hz; `0` selects 20 MHz. */
    size_t      max_files;              /**< Max simultaneously open files; `0` selects 5. */
    bool        format_if_mount_failed; /**< Auto-format the card if mount fails. */
    size_t      allocation_unit_size;   /**< FAT cluster size in bytes; `0` selects 16384. */
} blusys_sd_spi_config_t;

/**
 * @brief Mount a FAT filesystem on an SD card connected over SPI.
 *
 * Initialises the SPI bus, probes the card, and registers FAT under
 * `config->base_path` so standard C file APIs resolve there.
 *
 * @param config  Mount configuration; `base_path`, `spi_host`, and the pin
 *                fields are required.
 * @param out_sd  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_NO_MEM`, or
 *         `BLUSYS_ERR_IO` on probe/mount failure.
 */
blusys_err_t blusys_sd_spi_mount(const blusys_sd_spi_config_t *config,
                                  blusys_sd_spi_t **out_sd);

/**
 * @brief Unmount the filesystem, release the SPI bus, and free the handle.
 * @param sd  Handle from ::blusys_sd_spi_mount.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p sd is NULL.
 */
blusys_err_t blusys_sd_spi_unmount(blusys_sd_spi_t *sd);

/**
 * @brief Write (overwrite) a file with the given data.
 * @param sd    Handle.
 * @param path  Relative path, e.g. `"logs/boot.txt"`.
 * @param data  Bytes to write.
 * @param size  Number of bytes.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_IO` on failure.
 */
blusys_err_t blusys_sd_spi_write(blusys_sd_spi_t *sd, const char *path,
                                  const void *data, size_t size);

/**
 * @brief Read a file into the provided buffer.
 * @param sd              Handle.
 * @param path            Relative path.
 * @param buf             Destination buffer.
 * @param buf_size        Capacity of @p buf in bytes.
 * @param out_bytes_read  Output: actual bytes read.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_IO` if the file cannot be opened or read.
 */
blusys_err_t blusys_sd_spi_read(blusys_sd_spi_t *sd, const char *path,
                                 void *buf, size_t buf_size,
                                 size_t *out_bytes_read);

/**
 * @brief Append data to a file (creates it if absent).
 * @param sd    Handle.
 * @param path  Relative path.
 * @param data  Bytes to append.
 * @param size  Number of bytes.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_IO` on failure.
 */
blusys_err_t blusys_sd_spi_append(blusys_sd_spi_t *sd, const char *path,
                                   const void *data, size_t size);

/**
 * @brief Delete a file.
 * @param sd    Handle.
 * @param path  Relative path.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_IO` if the file does not exist.
 */
blusys_err_t blusys_sd_spi_remove(blusys_sd_spi_t *sd, const char *path);

/**
 * @brief Check whether a path (file or directory) exists.
 * @param sd          Handle.
 * @param path        Relative path.
 * @param out_exists  Output: `true` if the path exists.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if any pointer is NULL.
 */
blusys_err_t blusys_sd_spi_exists(blusys_sd_spi_t *sd, const char *path,
                                   bool *out_exists);

/**
 * @brief Get the size of a file in bytes.
 * @param sd        Handle.
 * @param path      Relative path.
 * @param out_size  Output: file size in bytes.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_IO` if the file does not exist.
 */
blusys_err_t blusys_sd_spi_size(blusys_sd_spi_t *sd, const char *path,
                                 size_t *out_size);

/**
 * @brief Create a directory (single level only).
 *
 * Treats `EEXIST` as success — the call is idempotent.
 *
 * @param sd    Handle.
 * @param path  Relative directory path, e.g. `"logs"`.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_IO` if creation fails.
 */
blusys_err_t blusys_sd_spi_mkdir(blusys_sd_spi_t *sd, const char *path);

/**
 * @brief Iterate entries in a directory, invoking @p cb for each one.
 *
 * Entries `"."` and `".."` are skipped. The filesystem lock is held for the
 * entire iteration — the callback must not call back into the same handle.
 *
 * @param sd         Handle.
 * @param path       Relative directory path; NULL or `""` lists the mount root.
 * @param cb         Callback invoked once per entry.
 * @param user_data  Passed through to @p cb unchanged.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_IO` if the directory cannot be opened.
 */
blusys_err_t blusys_sd_spi_listdir(blusys_sd_spi_t *sd, const char *path,
                                    blusys_sd_spi_listdir_cb_t cb, void *user_data);

#ifdef __cplusplus
}
#endif

#endif
