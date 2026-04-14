#ifndef BLUSYS_FATFS_H
#define BLUSYS_FATFS_H

#include <stdbool.h>
#include <stddef.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_fatfs blusys_fatfs_t;

/**
 * Callback invoked once per directory entry by blusys_fatfs_listdir().
 *
 * @param name       Entry name (file or subdirectory), no leading path.
 * @param user_data  Caller-supplied context pointer.
 */
typedef void (*blusys_fatfs_listdir_cb_t)(const char *name, void *user_data);

typedef struct {
    const char *base_path;              /**< VFS mount point, e.g. "/ffat" (required) */
    const char *partition_label;        /**< Flash partition label; NULL = "ffat" */
    size_t      max_files;              /**< Max simultaneously open files; 0 = 5 */
    bool        format_if_mount_failed; /**< Auto-format partition when mount fails */
    size_t      allocation_unit_size;   /**< FAT cluster size in bytes; 0 = default (4096) */
} blusys_fatfs_config_t;

/**
 * Mount a FAT filesystem on internal flash with wear-levelling.
 *
 * @param config     Mount configuration; base_path is required.
 * @param out_fatfs  Receives the handle on success.
 * @return BLUSYS_OK, BLUSYS_ERR_INVALID_ARG, BLUSYS_ERR_NO_MEM, or BLUSYS_ERR_IO.
 */
blusys_err_t blusys_fatfs_mount(const blusys_fatfs_config_t *config,
                                blusys_fatfs_t **out_fatfs);

/**
 * Unmount the filesystem, release the wear-levelling handle, and free memory.
 *
 * @param fatfs  Handle from blusys_fatfs_mount().
 * @return BLUSYS_OK or an error code.
 */
blusys_err_t blusys_fatfs_unmount(blusys_fatfs_t *fatfs);

/**
 * Write (overwrite) a file with the given data.
 *
 * @param fatfs  Filesystem handle.
 * @param path   Relative path, e.g. "logs/boot.txt".
 * @param data   Bytes to write.
 * @param size   Number of bytes.
 * @return BLUSYS_OK or BLUSYS_ERR_IO.
 */
blusys_err_t blusys_fatfs_write(blusys_fatfs_t *fatfs, const char *path,
                                const void *data, size_t size);

/**
 * Read a file into the provided buffer.
 *
 * @param fatfs          Filesystem handle.
 * @param path           Relative path.
 * @param buf            Destination buffer.
 * @param buf_size       Capacity of buf in bytes.
 * @param out_bytes_read Receives actual bytes read.
 * @return BLUSYS_OK or BLUSYS_ERR_IO.
 */
blusys_err_t blusys_fatfs_read(blusys_fatfs_t *fatfs, const char *path,
                               void *buf, size_t buf_size,
                               size_t *out_bytes_read);

/**
 * Append data to a file (creates it if absent).
 *
 * @param fatfs  Filesystem handle.
 * @param path   Relative path.
 * @param data   Bytes to append.
 * @param size   Number of bytes.
 * @return BLUSYS_OK or BLUSYS_ERR_IO.
 */
blusys_err_t blusys_fatfs_append(blusys_fatfs_t *fatfs, const char *path,
                                 const void *data, size_t size);

/**
 * Delete a file.
 *
 * @param fatfs  Filesystem handle.
 * @param path   Relative path.
 * @return BLUSYS_OK or BLUSYS_ERR_IO if the file does not exist.
 */
blusys_err_t blusys_fatfs_remove(blusys_fatfs_t *fatfs, const char *path);

/**
 * Check whether a path (file or directory) exists.
 *
 * @param fatfs       Filesystem handle.
 * @param path        Relative path.
 * @param out_exists  Set to true if the path exists.
 * @return BLUSYS_OK unless an argument is NULL.
 */
blusys_err_t blusys_fatfs_exists(blusys_fatfs_t *fatfs, const char *path,
                                 bool *out_exists);

/**
 * Get the size of a file in bytes.
 *
 * @param fatfs     Filesystem handle.
 * @param path      Relative path.
 * @param out_size  Receives the file size.
 * @return BLUSYS_OK or BLUSYS_ERR_IO if the file does not exist.
 */
blusys_err_t blusys_fatfs_size(blusys_fatfs_t *fatfs, const char *path,
                               size_t *out_size);

/**
 * Create a directory (single level only).
 *
 * Treats EEXIST as success — the call is idempotent.
 *
 * @param fatfs  Filesystem handle.
 * @param path   Relative directory path, e.g. "logs".
 * @return BLUSYS_OK, or BLUSYS_ERR_IO if creation fails.
 */
blusys_err_t blusys_fatfs_mkdir(blusys_fatfs_t *fatfs, const char *path);

/**
 * Iterate entries in a directory, invoking cb for each one.
 *
 * Entries "." and ".." are skipped. The lock is held for the entire
 * iteration — the callback must not call back into the same handle.
 *
 * @param fatfs      Filesystem handle.
 * @param path       Relative directory path; NULL or "" lists the mount root.
 * @param cb         Callback invoked once per entry.
 * @param user_data  Passed through to cb unchanged.
 * @return BLUSYS_OK, or BLUSYS_ERR_IO if the directory cannot be opened.
 */
blusys_err_t blusys_fatfs_listdir(blusys_fatfs_t *fatfs, const char *path,
                                  blusys_fatfs_listdir_cb_t cb, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_FATFS_H */
