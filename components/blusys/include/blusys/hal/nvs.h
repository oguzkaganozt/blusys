#ifndef BLUSYS_NVS_H
#define BLUSYS_NVS_H

#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_nvs blusys_nvs_t;

typedef enum {
    BLUSYS_NVS_READONLY  = 0,
    BLUSYS_NVS_READWRITE = 1,
} blusys_nvs_mode_t;

/* Lifecycle */
blusys_err_t blusys_nvs_open(const char *namespace_name, blusys_nvs_mode_t mode, blusys_nvs_t **out_nvs);
blusys_err_t blusys_nvs_close(blusys_nvs_t *nvs);
blusys_err_t blusys_nvs_commit(blusys_nvs_t *nvs);
blusys_err_t blusys_nvs_erase_key(blusys_nvs_t *nvs, const char *key);
blusys_err_t blusys_nvs_erase_all(blusys_nvs_t *nvs);

/* Integer getters */
blusys_err_t blusys_nvs_get_u8(blusys_nvs_t *nvs, const char *key, uint8_t *out_value);
blusys_err_t blusys_nvs_get_u32(blusys_nvs_t *nvs, const char *key, uint32_t *out_value);
blusys_err_t blusys_nvs_get_i32(blusys_nvs_t *nvs, const char *key, int32_t *out_value);
blusys_err_t blusys_nvs_get_u64(blusys_nvs_t *nvs, const char *key, uint64_t *out_value);

/* Integer setters */
blusys_err_t blusys_nvs_set_u8(blusys_nvs_t *nvs, const char *key, uint8_t value);
blusys_err_t blusys_nvs_set_u32(blusys_nvs_t *nvs, const char *key, uint32_t value);
blusys_err_t blusys_nvs_set_i32(blusys_nvs_t *nvs, const char *key, int32_t value);
blusys_err_t blusys_nvs_set_u64(blusys_nvs_t *nvs, const char *key, uint64_t value);

/*
 * String get/set.
 *
 * blusys_nvs_get_str() uses a two-call pattern:
 *   1. Call with out_value = NULL to get the required buffer length in *length
 *      (includes the null terminator).
 *   2. Allocate a buffer of that size and call again with out_value set.
 *
 * Returns BLUSYS_ERR_IO if the key does not exist.
 */
blusys_err_t blusys_nvs_get_str(blusys_nvs_t *nvs, const char *key, char *out_value, size_t *length);
blusys_err_t blusys_nvs_set_str(blusys_nvs_t *nvs, const char *key, const char *value);

/*
 * Blob get/set.
 *
 * blusys_nvs_get_blob() uses the same two-call pattern as blusys_nvs_get_str().
 * Call with out_value = NULL first to get the required byte count in *length.
 *
 * Returns BLUSYS_ERR_IO if the key does not exist.
 */
blusys_err_t blusys_nvs_get_blob(blusys_nvs_t *nvs, const char *key, void *out_value, size_t *length);
blusys_err_t blusys_nvs_set_blob(blusys_nvs_t *nvs, const char *key, const void *value, size_t length);

#ifdef __cplusplus
}
#endif

#endif
