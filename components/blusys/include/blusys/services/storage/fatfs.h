/**
 * @file fatfs.h
 * @brief FAT filesystem on internal flash with wear-levelling.
 *
 * Registers the FAT VFS under a base path so standard C file APIs resolve
 * there, and provides convenience helpers for common read/write/listdir
 * operations. See docs/services/fatfs.md.
 */

#ifndef BLUSYS_FATFS_H
#define BLUSYS_FATFS_H

#include <stdbool.h>
#include <stddef.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle to a mounted FAT filesystem. */
typedef struct blusys_fatfs blusys_fatfs_t;

/**
 * Callback invoked once per directory entry by blusys_fatfs_listdir().
 *
 * @param name       Entry name (file or subdirectory), no leading path.
 * @param user_data  Caller-supplied context pointer.
 */
typedef void (*blusys_fatfs_listdir_cb_t)(const char *name, void *user_data);

/** @brief Configuration for ::blusys_fatfs_mount. */
typedef struct {
    const char *base_path;              /**< VFS mount point, e.g. `"/ffat"` (required). */
    const char *partition_label;        /**< Flash partition label; NULL selects `"ffat"`. */
    size_t      max_files;              /**< Max simultaneously open files; `0` selects 5. */
    bool        format_if_mount_failed; /**< Auto-format the partition when mount fails. */
    size_t      allocation_unit_size;   /**< FAT cluster size in bytes; `0` selects 4096. */
} blusys_fatfs_config_t;

/**
 * @brief Mount a FAT filesystem on internal flash with wear-levelling.
 *
 * @param config     Mount configuration; base_path is required.
 * @param out_fatfs  Receives the handle on success.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for NULL args or missing base_path,
 *         `BLUSYS_ERR_NO_MEM` on allocation failure, `BLUSYS_ERR_IO` if VFS
 *         registration or mount fails.
 */
blusys_err_t blusys_fatfs_mount(const blusys_fatfs_config_t *config,
                                blusys_fatfs_t **out_fatfs);

/**
 * @brief Unmount the filesystem, release the wear-levelling handle, and free memory.
 *
 * @param fatfs  Handle from blusys_fatfs_mount().
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p fatfs is NULL,
 *         `BLUSYS_ERR_IO` if the unmount call fails.
 */
blusys_err_t blusys_fatfs_unmount(blusys_fatfs_t *fatfs);

/**
 * @brief Write (overwrite) a file with the given data.
 *
 * @param fatfs  Filesystem handle.
 * @param path   Relative path, e.g. `"logs/boot.txt"`.
 * @param data   Bytes to write.
 * @param size   Number of bytes.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for NULL args,
 *         `BLUSYS_ERR_IO` on open/write failure.
 */
blusys_err_t blusys_fatfs_write(blusys_fatfs_t *fatfs, const char *path,
                                const void *data, size_t size);

/**
 * @brief Read a file into the provided buffer.
 *
 * @param fatfs          Filesystem handle.
 * @param path           Relative path.
 * @param buf            Destination buffer.
 * @param buf_size       Capacity of @p buf in bytes.
 * @param out_bytes_read Receives actual bytes read.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for NULL args,
 *         `BLUSYS_ERR_IO` if the file does not exist or the read fails.
 */
blusys_err_t blusys_fatfs_read(blusys_fatfs_t *fatfs, const char *path,
                               void *buf, size_t buf_size,
                               size_t *out_bytes_read);

/**
 * @brief Append data to a file (creates it if absent).
 *
 * @param fatfs  Filesystem handle.
 * @param path   Relative path.
 * @param data   Bytes to append.
 * @param size   Number of bytes.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for NULL args,
 *         `BLUSYS_ERR_IO` on open/write failure.
 */
blusys_err_t blusys_fatfs_append(blusys_fatfs_t *fatfs, const char *path,
                                 const void *data, size_t size);

/**
 * @brief Delete a file.
 *
 * @param fatfs  Filesystem handle.
 * @param path   Relative path.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for NULL args,
 *         `BLUSYS_ERR_IO` if the file does not exist or cannot be removed.
 */
blusys_err_t blusys_fatfs_remove(blusys_fatfs_t *fatfs, const char *path);

/**
 * @brief Check whether a path (file or directory) exists.
 *
 * @param fatfs       Filesystem handle.
 * @param path        Relative path.
 * @param out_exists  Set to `true` if the path exists.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for NULL args.
 */
blusys_err_t blusys_fatfs_exists(blusys_fatfs_t *fatfs, const char *path,
                                 bool *out_exists);

/**
 * @brief Get the size of a file in bytes.
 *
 * @param fatfs     Filesystem handle.
 * @param path      Relative path.
 * @param out_size  Receives the file size.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for NULL args,
 *         `BLUSYS_ERR_IO` if the file does not exist.
 */
blusys_err_t blusys_fatfs_size(blusys_fatfs_t *fatfs, const char *path,
                               size_t *out_size);

/**
 * @brief Create a directory (single level only).
 *
 * Treats `EEXIST` as success — the call is idempotent.
 *
 * @param fatfs  Filesystem handle.
 * @param path   Relative directory path, e.g. `"logs"`.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for NULL args,
 *         `BLUSYS_ERR_IO` on underlying `mkdir()` failure.
 */
blusys_err_t blusys_fatfs_mkdir(blusys_fatfs_t *fatfs, const char *path);

/**
 * @brief Iterate entries in a directory, invoking @p cb for each one.
 *
 * Entries `"."` and `".."` are skipped. The lock is held for the entire
 * iteration — the callback must not call back into the same handle.
 *
 * @param fatfs      Filesystem handle.
 * @param path       Relative directory path; NULL or `""` lists the mount root.
 * @param cb         Callback invoked once per entry.
 * @param user_data  Passed through to @p cb unchanged.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for NULL @p fatfs or @p cb,
 *         `BLUSYS_ERR_IO` if the directory cannot be opened.
 */
blusys_err_t blusys_fatfs_listdir(blusys_fatfs_t *fatfs, const char *path,
                                  blusys_fatfs_listdir_cb_t cb, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_FATFS_H */
