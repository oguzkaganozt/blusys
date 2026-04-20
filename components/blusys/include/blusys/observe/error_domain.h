/**
 * @file error_domain.h
 * @brief Per-subsystem error domains for structured log and diagnostics.
 *
 * Each subsystem owns one domain. `blusys_err_t` (see blusys/hal/error.h) packs
 * a 16-bit domain in its high bits and a 16-bit subsystem-defined code in its
 * low bits, so structured log and diagnostics can attribute every failure to
 * its origin without stringly-typed tags.
 *
 * The enum grows *lazily*: a subsystem adds its domain the phase it migrates
 * to structured log + domain-stamped errors. Pre-declaring placeholders for
 * subsystems that haven't migrated is forbidden — the enum should always
 * reflect the actually-covered subsystems, not the hoped-for ones.
 *
 * `err_domain_generic` exists for legacy / cross-subsystem codes
 * (`INVALID_ARG`, `NO_MEM`, ...). Its numeric codes match the pre-domain enum
 * values so existing `BLUSYS_ERR_*` constants and `if (err != BLUSYS_OK)`
 * checks keep working.
 */

#ifndef BLUSYS_OBSERVE_ERROR_DOMAIN_H
#define BLUSYS_OBSERVE_ERROR_DOMAIN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Error domain identifier, packed into the high bits of `blusys_err_t`. */
typedef enum {
    err_domain_generic = 0,         /**< Legacy / cross-subsystem codes. */

    err_domain_framework_observe,   /**< Observe subsystem's own failures. */

    err_domain_driver_display,      /**< Display driver. */
    err_domain_driver_dht,          /**< DHT temperature/humidity driver. */
    err_domain_driver_encoder,      /**< Rotary encoder driver. */
    err_domain_driver_seven_seg,    /**< Seven-segment display driver. */
    err_domain_driver_lcd,          /**< LCD driver. */
    err_domain_driver_button,       /**< Button driver. */

    err_domain_storage_fs,          /**< Generic filesystem service. */
    err_domain_storage_fatfs,       /**< FAT filesystem service. */

    err_domain_hal_gpio,            /**< GPIO HAL. */
    err_domain_hal_timer,           /**< Timer HAL. */
    err_domain_hal_uart,            /**< UART HAL. */

    err_domain_persistence,         /**< Persistence capability. */

    err_domain_count,               /**< Sentinel: number of declared domains. */
} blusys_err_domain_t;

/**
 * @brief Return the stable dotted name for a domain (e.g. `"hal.gpio"`).
 *
 * Used by the structured log front-end and diagnostics.
 *
 * @param domain  Domain identifier.
 * @return Lowercase dotted name, or `"?"` for out-of-range values.
 */
const char *blusys_err_domain_string(blusys_err_domain_t domain);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_OBSERVE_ERROR_DOMAIN_H */
