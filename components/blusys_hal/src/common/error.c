#include "blusys/error.h"

const char *blusys_err_string(blusys_err_t err)
{
    switch (err) {
    case BLUSYS_OK:
        return "ok";
    case BLUSYS_ERR_INVALID_ARG:
        return "invalid argument";
    case BLUSYS_ERR_INVALID_STATE:
        return "invalid state";
    case BLUSYS_ERR_NOT_SUPPORTED:
        return "not supported";
    case BLUSYS_ERR_TIMEOUT:
        return "timeout";
    case BLUSYS_ERR_BUSY:
        return "busy";
    case BLUSYS_ERR_NO_MEM:
        return "out of memory";
    case BLUSYS_ERR_IO:
        return "io error";
    case BLUSYS_ERR_NOT_FOUND:
        return "not found";
    case BLUSYS_ERR_INTERNAL:
        return "internal error";
    default:
        return "unknown error";
    }
}
