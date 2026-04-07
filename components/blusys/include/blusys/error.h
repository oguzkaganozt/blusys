#ifndef BLUSYS_ERROR_H
#define BLUSYS_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BLUSYS_OK = 0,
    BLUSYS_ERR_INVALID_ARG,
    BLUSYS_ERR_INVALID_STATE,
    BLUSYS_ERR_NOT_SUPPORTED,
    BLUSYS_ERR_TIMEOUT,
    BLUSYS_ERR_BUSY,
    BLUSYS_ERR_NO_MEM,
    BLUSYS_ERR_IO,
    BLUSYS_ERR_NOT_FOUND,
    BLUSYS_ERR_INTERNAL,
} blusys_err_t;

#define BLUSYS_TIMEOUT_FOREVER (-1)

const char *blusys_err_string(blusys_err_t err);

#ifdef __cplusplus
}
#endif

#endif
