#ifndef BLUSYS_FS_H
#define BLUSYS_FS_H

#include <stdbool.h>
#include <stddef.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_fs blusys_fs_t;

typedef struct {
    const char *base_path;        /**< VFS mount point, e.g. "/fs" (required) */
    const char *partition_label;  /**< Flash partition label; NULL = default "spiffs" partition */
    size_t      max_files;        /**< Max simultaneously open files; 0 = use default (5) */
    bool        format_if_mount_failed; /**< Auto-format partition when mount fails */
} blusys_fs_config_t;

/**
 * Mount a SPIFFS filesystem on the internal flash.
 *
 * @param config  Mount configuration (base_path required).
 * @param out_fs  Receives the mounted filesystem handle on success.
 * @return BLUSYS_OK on success, or an error code.
 */
blusys_err_t blusys_fs_mount(const blusys_fs_config_t *config, blusys_fs_t **out_fs);

/**
 * Unmount the filesystem and free the handle.
 *
 * @param fs  Handle returned by blusys_fs_mount().
 * @return BLUSYS_OK on success, or an error code.
 */
blusys_err_t blusys_fs_unmount(blusys_fs_t *fs);

/**
 * Write (overwrite) a file with the given data.
 *
 * @param fs    Filesystem handle.
 * @param path  Relative file path, e.g. "log.txt".
 * @param data  Data to write.
 * @param size  Number of bytes to write.
 * @return BLUSYS_OK on success, or an error code.
 */
blusys_err_t blusys_fs_write(blusys_fs_t *fs, const char *path, const void *data, size_t size);

/**
 * Read a file into the provided buffer.
 *
 * @param fs             Filesystem handle.
 * @param path           Relative file path.
 * @param buf            Buffer to read into.
 * @param buf_size       Size of buf in bytes.
 * @param out_bytes_read Receives the number of bytes actually read.
 * @return BLUSYS_OK on success, or an error code.
 */
blusys_err_t blusys_fs_read(blusys_fs_t *fs, const char *path, void *buf, size_t buf_size,
                             size_t *out_bytes_read);

/**
 * Append data to the end of a file (creates the file if it does not exist).
 *
 * @param fs    Filesystem handle.
 * @param path  Relative file path.
 * @param data  Data to append.
 * @param size  Number of bytes to append.
 * @return BLUSYS_OK on success, or an error code.
 */
blusys_err_t blusys_fs_append(blusys_fs_t *fs, const char *path, const void *data, size_t size);

/**
 * Delete a file.
 *
 * @param fs    Filesystem handle.
 * @param path  Relative file path.
 * @return BLUSYS_OK on success, BLUSYS_ERR_IO if the file does not exist, or an error code.
 */
blusys_err_t blusys_fs_remove(blusys_fs_t *fs, const char *path);

/**
 * Check whether a file exists.
 *
 * @param fs          Filesystem handle.
 * @param path        Relative file path.
 * @param out_exists  Set to true if the file exists, false otherwise.
 * @return BLUSYS_OK on success, or an error code.
 */
blusys_err_t blusys_fs_exists(blusys_fs_t *fs, const char *path, bool *out_exists);

/**
 * Get the size of a file in bytes.
 *
 * @param fs        Filesystem handle.
 * @param path      Relative file path.
 * @param out_size  Receives the file size in bytes.
 * @return BLUSYS_OK on success, BLUSYS_ERR_IO if the file does not exist, or an error code.
 */
blusys_err_t blusys_fs_size(blusys_fs_t *fs, const char *path, size_t *out_size);

#ifdef __cplusplus
}
#endif

#endif
