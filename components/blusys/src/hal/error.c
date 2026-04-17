#include "blusys/hal/error.h"

const char *blusys_err_string(blusys_err_t err)
{
    /* Render the generic code embedded in the low 16 bits. The domain prefix
     * is provided separately by the log front-end via
     * blusys_err_domain_string(), so this string focuses on the code itself. */
    switch (BLUSYS_ERR_CODE(err)) {
    case 0: /* OK */
        return "ok";
    case 1: /* INVALID_ARG */
        return "invalid argument";
    case 2: /* INVALID_STATE */
        return "invalid state";
    case 3: /* NOT_SUPPORTED */
        return "not supported";
    case 4: /* TIMEOUT */
        return "timeout";
    case 5: /* BUSY */
        return "busy";
    case 6: /* NO_MEM */
        return "out of memory";
    case 7: /* IO */
        return "io error";
    case 8: /* NOT_FOUND */
        return "not found";
    case 9: /* INTERNAL */
        return "internal error";
    default:
        return "unknown error";
    }
}
