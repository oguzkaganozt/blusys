#include "blusys/framework/services/internal/storage_helpers.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "blusys/framework/observe/counter.h"
#include "blusys/framework/observe/log.h"

blusys_err_t blusys_storage_fail(blusys_err_domain_t domain,
                                 uint16_t code,
                                 const char *op,
                                 const char *path)
{
    blusys_counter_inc(domain, 1);
    BLUSYS_LOG(BLUSYS_LOG_ERROR, domain, "op=%s path=%s code=0x%x",
               op, path != NULL ? path : "(none)", code);
    return BLUSYS_ERR(domain, code);
}

blusys_err_t blusys_storage_build_path(const char *base_path,
                                       const char *path,
                                       char *out,
                                       size_t out_size,
                                       blusys_err_domain_t domain)
{
    if (base_path == NULL || out == NULL || out_size == 0) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    const char *suffix = (path == NULL || path[0] == '\0') ? "" : "/";
    const char *rel    = (path == NULL || path[0] == '\0') ? "" : path;
    int n = snprintf(out, out_size, "%s%s%s", base_path, suffix, rel);
    if (n < 0 || (size_t)n >= out_size) {
        return blusys_storage_fail(domain, BLUSYS_STORAGE_ERR_PATH_TOO_LONG, "build_path", path);
    }
    return BLUSYS_OK;
}

static blusys_err_t take_lock(blusys_lock_t *lock)
{
    if (lock == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    return blusys_lock_take(lock, BLUSYS_LOCK_WAIT_FOREVER);
}

blusys_err_t blusys_storage_write(blusys_lock_t *lock,
                                  blusys_err_domain_t domain,
                                  const char *full_path,
                                  const char *path,
                                  const void *data,
                                  size_t size)
{
    if (full_path == NULL || (data == NULL && size > 0)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = take_lock(lock);
    if (err != BLUSYS_OK) {
        return err;
    }

    FILE *f = fopen(full_path, "wb");
    if (f == NULL) {
        blusys_lock_give(lock);
        return blusys_storage_fail(domain, BLUSYS_STORAGE_ERR_OPEN, "write", path);
    }

    if (size > 0) {
        size_t written = fwrite(data, 1, size, f);
        if (written != size) {
            err = blusys_storage_fail(domain, BLUSYS_STORAGE_ERR_WRITE_SHORT, "write", path);
        }
    }

    fclose(f);
    blusys_lock_give(lock);
    return err;
}

blusys_err_t blusys_storage_read(blusys_lock_t *lock,
                                 blusys_err_domain_t domain,
                                 const char *full_path,
                                 const char *path,
                                 void *buf,
                                 size_t buf_size,
                                 size_t *out_bytes_read)
{
    if (full_path == NULL || buf == NULL || out_bytes_read == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = take_lock(lock);
    if (err != BLUSYS_OK) {
        return err;
    }

    FILE *f = fopen(full_path, "rb");
    if (f == NULL) {
        blusys_lock_give(lock);
        return blusys_storage_fail(domain, BLUSYS_STORAGE_ERR_OPEN, "read", path);
    }

    *out_bytes_read = fread(buf, 1, buf_size, f);
    if (ferror(f)) {
        err = blusys_storage_fail(domain, BLUSYS_STORAGE_ERR_READ, "read", path);
    }

    fclose(f);
    blusys_lock_give(lock);
    return err;
}

blusys_err_t blusys_storage_append(blusys_lock_t *lock,
                                   blusys_err_domain_t domain,
                                   const char *full_path,
                                   const char *path,
                                   const void *data,
                                   size_t size)
{
    if (full_path == NULL || (data == NULL && size > 0)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = take_lock(lock);
    if (err != BLUSYS_OK) {
        return err;
    }

    FILE *f = fopen(full_path, "ab");
    if (f == NULL) {
        blusys_lock_give(lock);
        return blusys_storage_fail(domain, BLUSYS_STORAGE_ERR_OPEN, "append", path);
    }

    if (size > 0) {
        size_t written = fwrite(data, 1, size, f);
        if (written != size) {
            err = blusys_storage_fail(domain, BLUSYS_STORAGE_ERR_WRITE_SHORT, "append", path);
        }
    }

    fclose(f);
    blusys_lock_give(lock);
    return err;
}

blusys_err_t blusys_storage_remove(blusys_lock_t *lock,
                                   blusys_err_domain_t domain,
                                   const char *full_path,
                                   const char *path)
{
    if (full_path == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = take_lock(lock);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (remove(full_path) != 0) {
        err = blusys_storage_fail(domain, BLUSYS_STORAGE_ERR_REMOVE, "remove", path);
    }

    blusys_lock_give(lock);
    return err;
}

blusys_err_t blusys_storage_exists(blusys_lock_t *lock,
                                   const char *full_path,
                                   bool *out_exists)
{
    if (full_path == NULL || out_exists == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = take_lock(lock);
    if (err != BLUSYS_OK) {
        return err;
    }

    struct stat st;
    *out_exists = (stat(full_path, &st) == 0);

    blusys_lock_give(lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_storage_size(blusys_lock_t *lock,
                                 blusys_err_domain_t domain,
                                 const char *full_path,
                                 const char *path,
                                 size_t *out_size)
{
    if (full_path == NULL || out_size == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = take_lock(lock);
    if (err != BLUSYS_OK) {
        return err;
    }

    struct stat st;
    if (stat(full_path, &st) != 0) {
        blusys_lock_give(lock);
        return blusys_storage_fail(domain, BLUSYS_STORAGE_ERR_STAT, "size", path);
    }

    *out_size = (size_t)st.st_size;

    blusys_lock_give(lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_storage_mkdir(blusys_lock_t *lock,
                                  blusys_err_domain_t domain,
                                  const char *full_path,
                                  const char *path)
{
    if (full_path == NULL || full_path[0] == '\0') {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = take_lock(lock);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (mkdir(full_path, 0755) != 0 && errno != EEXIST) {
        err = blusys_storage_fail(domain, BLUSYS_STORAGE_ERR_MKDIR, "mkdir", path);
    }

    blusys_lock_give(lock);
    return err;
}

blusys_err_t blusys_storage_listdir(blusys_lock_t *lock,
                                    blusys_err_domain_t domain,
                                    const char *full_path,
                                    const char *path,
                                    blusys_storage_listdir_cb_t cb,
                                    void *user_data)
{
    if (full_path == NULL || cb == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = take_lock(lock);
    if (err != BLUSYS_OK) {
        return err;
    }

    DIR *dir = opendir(full_path);
    if (dir == NULL) {
        blusys_lock_give(lock);
        return blusys_storage_fail(domain, BLUSYS_STORAGE_ERR_OPENDIR, "listdir", path);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        cb(entry->d_name, user_data);
    }

    closedir(dir);
    blusys_lock_give(lock);
    return BLUSYS_OK;
}
