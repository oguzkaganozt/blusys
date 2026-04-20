/* blusys/observe/error_domain.h — per-subsystem error domains.
 *
 * Each subsystem owns one domain. blusys_err_t (see blusys/hal/error.h) packs
 * a 16-bit domain in its high bits and a 16-bit subsystem-defined code in its
 * low bits, so structured log and diagnostics can attribute every failure to
 * its origin without stringly-typed tags.
 *
 * The enum grows *lazily*: a subsystem adds its domain the phase it migrates
 * to structured log + domain-stamped errors. Pre-declaring placeholders for
 * subsystems that haven't migrated is forbidden — the enum should always
 * reflect the actually-covered subsystems, not the hoped-for ones.
 *
 * `err_domain_generic` exists for legacy / cross-subsystem codes (INVALID_ARG,
 * NO_MEM, ...). Its numeric codes match the pre-domain enum values so existing
 * `BLUSYS_ERR_*` constants and `if (err != BLUSYS_OK)` checks keep working.
 */

#ifndef BLUSYS_OBSERVE_ERROR_DOMAIN_H
#define BLUSYS_OBSERVE_ERROR_DOMAIN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    err_domain_generic = 0,

    /* Framework — observe itself, for its own failures. */
    err_domain_framework_observe,

    /* Drivers. */
    err_domain_driver_display,
    err_domain_driver_dht,
    err_domain_driver_encoder,
    err_domain_driver_seven_seg,
    err_domain_driver_lcd,
    err_domain_driver_button,

    /* Storage services. */
    err_domain_storage_fs,
    err_domain_storage_fatfs,

    /* HAL peripherals. */
    err_domain_hal_gpio,
    err_domain_hal_timer,
    err_domain_hal_uart,

    /* Persistence capability. */
    err_domain_persistence,

    err_domain_count,
} blusys_err_domain_t;

/* Stable, lowercase, dotted name (e.g. "hal.gpio", "storage.fs"). Used by the
 * structured log front-end and diagnostics. Returns "?" for out-of-range. */
const char *blusys_err_domain_string(blusys_err_domain_t domain);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_OBSERVE_ERROR_DOMAIN_H */
