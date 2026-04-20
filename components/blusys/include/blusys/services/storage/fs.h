/**
 * @file fs.h
 * @brief SPIFFS filesystem on internal flash.
 *
 * Registers the SPIFFS VFS under a base path so standard C file APIs resolve
 * there, and provides convenience helpers for common read/write operations.
 * For a hierarchical filesystem on flash, use ::blusys_fatfs instead. See
 * docs/services/fs.md.
 */

#ifndef BLUSYS_FS_H
#define BLUSYS_FS_H

#include <stdbool.h>
#include <stddef.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle to a mounted SPIFFS filesystem. */
typedef struct blusys_fs blusys_fs_t;

/** @brief Configuration for ::blusys_fs_mount. */
typedef struct {
    const char *base_path;              /**< VFS mount point, e.g. `"/fs"` (required). */
    const char *partition_label;        /**< Flash partition label; NULL selects the default `"spiffs"` partition. */
    size_t      max_files;              /**< Max simultaneously open files; `0` selects 5. */
    bool        format_if_mount_failed; /**< Auto-format the partition when mount fails. */
} blusys_fs_config_t;

/**
 * @brief Mount a SPIFFS filesystem on the internal flash.
 *
 * @param config  Mount configuration (base_path required).
 * @param out_fs  Receives the mounted filesystem handle on success.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for NULL args or missing base_path,
 *         `BLUSYS_ERR_NO_MEM` on allocation failure, `BLUSYS_ERR_IO` if the SPIFFS
 *         register call fails (partition missing, format failed when allowed).
 */
blusys_err_t blusys_fs_mount(const blusys_fs_config_t *config, blusys_fs_t **out_fs);

/**
 * @brief Unmount the filesystem and free the handle.
 *
 * @param fs  Handle returned by blusys_fs_mount().
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p fs is NULL,
 *         `BLUSYS_ERR_IO` if the SPIFFS unregister call fails.
 */
blusys_err_t blusys_fs_unmount(blusys_fs_t *fs);

/**
 * @brief Write (overwrite) a file with the given data.
 *
 * @param fs    Filesystem handle.
 * @param path  Relative file path, e.g. `"log.txt"`.
 * @param data  Data to write.
 * @param size  Number of bytes to write.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for NULL args,
 *         `BLUSYS_ERR_IO` on open/write failure.
 */
blusys_err_t blusys_fs_write(blusys_fs_t *fs, const char *path, const void *data, size_t size);

/**
 * @brief Read a file into the provided buffer.
 *
 * @param fs             Filesystem handle.
 * @param path           Relative file path.
 * @param buf            Buffer to read into.
 * @param buf_size       Size of @p buf in bytes.
 * @param out_bytes_read Receives the number of bytes actually read.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for NULL args,
 *         `BLUSYS_ERR_IO` if the file does not exist or the read fails.
 */
blusys_err_t blusys_fs_read(blusys_fs_t *fs, const char *path, void *buf, size_t buf_size,
                             size_t *out_bytes_read);

/**
 * @brief Append data to the end of a file (creates the file if it does not exist).
 *
 * @param fs    Filesystem handle.
 * @param path  Relative file path.
 * @param data  Data to append.
 * @param size  Number of bytes to append.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for NULL args,
 *         `BLUSYS_ERR_IO` on open/write failure.
 */
blusys_err_t blusys_fs_append(blusys_fs_t *fs, const char *path, const void *data, size_t size);

/**
 * @brief Delete a file.
 *
 * @param fs    Filesystem handle.
 * @param path  Relative file path.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for NULL args,
 *         `BLUSYS_ERR_IO` if the file does not exist or cannot be removed.
 */
blusys_err_t blusys_fs_remove(blusys_fs_t *fs, const char *path);

/**
 * @brief Check whether a file exists.
 *
 * @param fs          Filesystem handle.
 * @param path        Relative file path.
 * @param out_exists  Set to `true` if the file exists, `false` otherwise.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for NULL args.
 */
blusys_err_t blusys_fs_exists(blusys_fs_t *fs, const char *path, bool *out_exists);

/**
 * @brief Get the size of a file in bytes.
 *
 * @param fs        Filesystem handle.
 * @param path      Relative file path.
 * @param out_size  Receives the file size in bytes.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for NULL args,
 *         `BLUSYS_ERR_IO` if the file does not exist.
 */
blusys_err_t blusys_fs_size(blusys_fs_t *fs, const char *path, size_t *out_size);

#ifdef __cplusplus
}
#endif

#endif
