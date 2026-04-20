/**
 * @file nvs.h
 * @brief Persistent key-value storage backed by ESP32 flash.
 *
 * Data survives power cycles and soft resets. `set_*` calls stage writes in
 * RAM; call ::blusys_nvs_commit to flush them to flash. See docs/hal/nvs.md.
 */

#ifndef BLUSYS_NVS_H
#define BLUSYS_NVS_H

#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle to an open NVS namespace. */
typedef struct blusys_nvs blusys_nvs_t;

/** @brief Access mode for ::blusys_nvs_open. */
typedef enum {
    BLUSYS_NVS_READONLY  = 0,   /**< Read-only; `set_*` and `erase_*` return `BLUSYS_ERR_INVALID_STATE`. */
    BLUSYS_NVS_READWRITE = 1,   /**< Read-write; allows `set_*`, `erase_*`, and `commit`. */
} blusys_nvs_mode_t;

/**
 * @brief Open a named NVS namespace.
 *
 * Initializes the NVS flash partition on the first call (idempotent with
 * `nvs_flash_init()` elsewhere). Namespace names are limited to 15 characters.
 *
 * @param namespace_name  Namespace string, max 15 characters.
 * @param mode            Read-only or read-write.
 * @param out_nvs         Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for NULL or over-long names,
 *         `BLUSYS_ERR_NO_MEM` on allocation failure.
 */
blusys_err_t blusys_nvs_open(const char *namespace_name, blusys_nvs_mode_t mode, blusys_nvs_t **out_nvs);

/**
 * @brief Close the handle and free its resources. Uncommitted writes are discarded.
 * @param nvs  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p nvs is NULL.
 */
blusys_err_t blusys_nvs_close(blusys_nvs_t *nvs);

/**
 * @brief Flush pending writes to flash.
 *
 * Required after any `set_*` or `erase_*` call for the change to persist.
 *
 * @param nvs  Read-write handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` for read-only handles.
 */
blusys_err_t blusys_nvs_commit(blusys_nvs_t *nvs);

/**
 * @brief Erase a single key from the namespace.
 * @param nvs  Read-write handle.
 * @param key  Key to erase.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` for read-only handles,
 *         `BLUSYS_ERR_IO` if the key does not exist.
 */
blusys_err_t blusys_nvs_erase_key(blusys_nvs_t *nvs, const char *key);

/**
 * @brief Erase every key in the namespace.
 * @param nvs  Read-write handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` for read-only handles.
 */
blusys_err_t blusys_nvs_erase_all(blusys_nvs_t *nvs);

/** @brief Read a `uint8_t` value. Returns `BLUSYS_ERR_IO` if @p key does not exist. */
blusys_err_t blusys_nvs_get_u8(blusys_nvs_t *nvs, const char *key, uint8_t *out_value);
/** @brief Read a `uint32_t` value. Returns `BLUSYS_ERR_IO` if @p key does not exist. */
blusys_err_t blusys_nvs_get_u32(blusys_nvs_t *nvs, const char *key, uint32_t *out_value);
/** @brief Read an `int32_t` value. Returns `BLUSYS_ERR_IO` if @p key does not exist. */
blusys_err_t blusys_nvs_get_i32(blusys_nvs_t *nvs, const char *key, int32_t *out_value);
/** @brief Read a `uint64_t` value. Returns `BLUSYS_ERR_IO` if @p key does not exist. */
blusys_err_t blusys_nvs_get_u64(blusys_nvs_t *nvs, const char *key, uint64_t *out_value);

/** @brief Stage a `uint8_t` write. Call ::blusys_nvs_commit to persist. Returns `BLUSYS_ERR_INVALID_STATE` on read-only handles. */
blusys_err_t blusys_nvs_set_u8(blusys_nvs_t *nvs, const char *key, uint8_t value);
/** @brief Stage a `uint32_t` write. Call ::blusys_nvs_commit to persist. Returns `BLUSYS_ERR_INVALID_STATE` on read-only handles. */
blusys_err_t blusys_nvs_set_u32(blusys_nvs_t *nvs, const char *key, uint32_t value);
/** @brief Stage an `int32_t` write. Call ::blusys_nvs_commit to persist. Returns `BLUSYS_ERR_INVALID_STATE` on read-only handles. */
blusys_err_t blusys_nvs_set_i32(blusys_nvs_t *nvs, const char *key, int32_t value);
/** @brief Stage a `uint64_t` write. Call ::blusys_nvs_commit to persist. Returns `BLUSYS_ERR_INVALID_STATE` on read-only handles. */
blusys_err_t blusys_nvs_set_u64(blusys_nvs_t *nvs, const char *key, uint64_t value);

/**
 * @brief Read a null-terminated string (two-call pattern).
 *
 * Call once with @p out_value = NULL to get the required buffer size
 * (including the null terminator) in `*length`. Allocate a buffer of that
 * size and call again with @p out_value set.
 *
 * @param nvs        Handle.
 * @param key        Key to read.
 * @param out_value  Output buffer, or NULL to query the required size.
 * @param length     In/out length in bytes.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_IO` if the key does not exist.
 */
blusys_err_t blusys_nvs_get_str(blusys_nvs_t *nvs, const char *key, char *out_value, size_t *length);

/**
 * @brief Stage a string write. Call ::blusys_nvs_commit to persist.
 * @param nvs    Read-write handle.
 * @param key    Key.
 * @param value  Null-terminated string.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` for read-only handles.
 */
blusys_err_t blusys_nvs_set_str(blusys_nvs_t *nvs, const char *key, const char *value);

/**
 * @brief Read an arbitrary blob (two-call pattern).
 *
 * Same two-call convention as ::blusys_nvs_get_str. Pass @p out_value = NULL
 * first to get the byte count in `*length`, then allocate and read.
 *
 * @param nvs        Handle.
 * @param key        Key to read.
 * @param out_value  Output buffer, or NULL to query the required size.
 * @param length     In/out byte count.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_IO` if the key does not exist.
 */
blusys_err_t blusys_nvs_get_blob(blusys_nvs_t *nvs, const char *key, void *out_value, size_t *length);

/**
 * @brief Stage a blob write. Call ::blusys_nvs_commit to persist.
 * @param nvs     Read-write handle.
 * @param key     Key.
 * @param value   Pointer to the blob.
 * @param length  Blob length in bytes.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` for read-only handles.
 */
blusys_err_t blusys_nvs_set_blob(blusys_nvs_t *nvs, const char *key, const void *value, size_t length);

#ifdef __cplusplus
}
#endif

#endif
