#pragma once

#include "blusys/observe/error_domain.h"
#include "blusys/hal/internal/lock.h"
#include "blusys/hal/error.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*blusys_storage_listdir_cb_t)(const char *name, void *user_data);

typedef enum {
    BLUSYS_STORAGE_ERR_PATH_TOO_LONG = 0x100,
    BLUSYS_STORAGE_ERR_OPEN          = 0x101,
    BLUSYS_STORAGE_ERR_WRITE_SHORT   = 0x102,
    BLUSYS_STORAGE_ERR_READ          = 0x103,
    BLUSYS_STORAGE_ERR_REMOVE        = 0x104,
    BLUSYS_STORAGE_ERR_STAT          = 0x105,
    BLUSYS_STORAGE_ERR_MKDIR         = 0x106,
    BLUSYS_STORAGE_ERR_OPENDIR       = 0x107,
} blusys_storage_err_code_t;

#ifdef __cplusplus
extern "C" {
#endif

blusys_err_t blusys_storage_fail(blusys_err_domain_t domain,
                                 uint16_t code,
                                 const char *op,
                                 const char *path);

blusys_err_t blusys_storage_build_path(const char *base_path,
                                       const char *path,
                                       char *out,
                                       size_t out_size,
                                       blusys_err_domain_t domain);

blusys_err_t blusys_storage_write(blusys_lock_t *lock,
                                  blusys_err_domain_t domain,
                                  const char *full_path,
                                  const char *path,
                                  const void *data,
                                  size_t size);

blusys_err_t blusys_storage_read(blusys_lock_t *lock,
                                 blusys_err_domain_t domain,
                                 const char *full_path,
                                 const char *path,
                                 void *buf,
                                 size_t buf_size,
                                 size_t *out_bytes_read);

blusys_err_t blusys_storage_append(blusys_lock_t *lock,
                                   blusys_err_domain_t domain,
                                   const char *full_path,
                                   const char *path,
                                   const void *data,
                                   size_t size);

blusys_err_t blusys_storage_remove(blusys_lock_t *lock,
                                   blusys_err_domain_t domain,
                                   const char *full_path,
                                   const char *path);

blusys_err_t blusys_storage_exists(blusys_lock_t *lock,
                                   const char *full_path,
                                   bool *out_exists);

blusys_err_t blusys_storage_size(blusys_lock_t *lock,
                                 blusys_err_domain_t domain,
                                 const char *full_path,
                                 const char *path,
                                 size_t *out_size);

blusys_err_t blusys_storage_mkdir(blusys_lock_t *lock,
                                  blusys_err_domain_t domain,
                                  const char *full_path,
                                  const char *path);

blusys_err_t blusys_storage_listdir(blusys_lock_t *lock,
                                    blusys_err_domain_t domain,
                                    const char *full_path,
                                    const char *path,
                                    blusys_storage_listdir_cb_t cb,
                                    void *user_data);

#ifdef __cplusplus
}
#endif
