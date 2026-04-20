/* blusys/hal/error.h — packed (domain, code) error type.
 *
 * blusys_err_t is a 32-bit value: high 16 bits identify the originating
 * subsystem (see observe/error_domain.h), low 16 bits hold a
 * subsystem-defined code. Generic codes (INVALID_ARG, NO_MEM, ...) live in
 * `err_domain_generic = 0` with their historical numeric values, so existing
 * `if (err != BLUSYS_OK)` checks and switches keep working untouched.
 */

#ifndef BLUSYS_ERROR_H
#define BLUSYS_ERROR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t blusys_err_t;

/* Pack/unpack helpers. Domain in high 16 bits, code in low 16. */
#define BLUSYS_ERR(domain, code) \
    ((blusys_err_t)(((uint32_t)(domain) << 16) | ((uint32_t)(code) & 0xFFFFu)))
#define BLUSYS_ERR_DOMAIN(err) ((uint16_t)(((uint32_t)(err) >> 16) & 0xFFFFu))
#define BLUSYS_ERR_CODE(err)   ((uint16_t)((uint32_t)(err) & 0xFFFFu))

/* Generic codes — domain 0. Numeric values preserved from the original enum
 * so that comparisons throughout the codebase remain valid. */
#define BLUSYS_OK                   ((blusys_err_t)0u)
#define BLUSYS_ERR_INVALID_ARG      ((blusys_err_t)1u)
#define BLUSYS_ERR_INVALID_STATE    ((blusys_err_t)2u)
#define BLUSYS_ERR_NOT_SUPPORTED    ((blusys_err_t)3u)
#define BLUSYS_ERR_TIMEOUT          ((blusys_err_t)4u)
#define BLUSYS_ERR_BUSY             ((blusys_err_t)5u)
#define BLUSYS_ERR_NO_MEM           ((blusys_err_t)6u)
#define BLUSYS_ERR_IO               ((blusys_err_t)7u)
#define BLUSYS_ERR_NOT_FOUND        ((blusys_err_t)8u)
#define BLUSYS_ERR_INTERNAL         ((blusys_err_t)9u)

#define BLUSYS_TIMEOUT_FOREVER (-1)

/* Human-readable string for the generic code embedded in `err`. The domain is
 * rendered separately via blusys_err_domain_string() at the log front-end. */
const char *blusys_err_string(blusys_err_t err);

#ifdef __cplusplus
}
#endif

#endif
