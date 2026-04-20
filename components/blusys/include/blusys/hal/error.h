/**
 * @file error.h
 * @brief Packed (domain, code) error type used throughout blusys.
 *
 * ::blusys_err_t is a 32-bit value: the high 16 bits identify the originating
 * subsystem (see blusys/observe/error_domain.h), the low 16 bits hold a
 * subsystem-defined code. Generic codes (`INVALID_ARG`, `NO_MEM`, ...) live in
 * `err_domain_generic = 0` with their historical numeric values, so existing
 * `if (err != BLUSYS_OK)` checks and switches keep working untouched.
 */

#ifndef BLUSYS_ERROR_H
#define BLUSYS_ERROR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Packed error type: `(domain << 16) | code`. */
typedef uint32_t blusys_err_t;

/** @brief Pack a domain and code into a ::blusys_err_t. */
#define BLUSYS_ERR(domain, code) \
    ((blusys_err_t)(((uint32_t)(domain) << 16) | ((uint32_t)(code) & 0xFFFFu)))
/** @brief Extract the domain (high 16 bits) from a ::blusys_err_t. */
#define BLUSYS_ERR_DOMAIN(err) ((uint16_t)(((uint32_t)(err) >> 16) & 0xFFFFu))
/** @brief Extract the subsystem code (low 16 bits) from a ::blusys_err_t. */
#define BLUSYS_ERR_CODE(err)   ((uint16_t)((uint32_t)(err) & 0xFFFFu))

/** @brief Success. */
#define BLUSYS_OK                   ((blusys_err_t)0u)
/** @brief Invalid argument (e.g. NULL pointer, out-of-range value). */
#define BLUSYS_ERR_INVALID_ARG      ((blusys_err_t)1u)
/** @brief Operation not valid for the current object state. */
#define BLUSYS_ERR_INVALID_STATE    ((blusys_err_t)2u)
/** @brief Feature not supported on this target or in this build. */
#define BLUSYS_ERR_NOT_SUPPORTED    ((blusys_err_t)3u)
/** @brief Operation timed out. */
#define BLUSYS_ERR_TIMEOUT          ((blusys_err_t)4u)
/** @brief Resource is busy or already in use. */
#define BLUSYS_ERR_BUSY             ((blusys_err_t)5u)
/** @brief Out of memory. */
#define BLUSYS_ERR_NO_MEM           ((blusys_err_t)6u)
/** @brief I/O failure (bus error, protocol error, media failure). */
#define BLUSYS_ERR_IO               ((blusys_err_t)7u)
/** @brief Requested item does not exist. */
#define BLUSYS_ERR_NOT_FOUND        ((blusys_err_t)8u)
/** @brief Internal consistency failure; report as a bug. */
#define BLUSYS_ERR_INTERNAL         ((blusys_err_t)9u)

/** @brief Sentinel timeout value meaning "wait indefinitely". */
#define BLUSYS_TIMEOUT_FOREVER (-1)

/**
 * @brief Return a human-readable string for the generic code embedded in @p err.
 *
 * The domain portion is rendered separately via `blusys_err_domain_string()`
 * at the log front-end.
 *
 * @param err  Packed error.
 * @return Pointer to a static string; never NULL.
 */
const char *blusys_err_string(blusys_err_t err);

#ifdef __cplusplus
}
#endif

#endif
