/* blusys/framework/test/fake_hal.h — test-side inject API for the fake HAL.
 *
 * The fake HAL sources in src/framework/test/fake_hal/ implement the real
 * blusys_{gpio,timer,uart} public headers. This header exposes only the
 * *extra* entry points tests need to drive those fakes — inject a gpio edge,
 * advance a timer, feed bytes at the uart RX. Nothing in product code ever
 * includes this.
 *
 * Linked only into test executables; never compiled into device or host
 * product builds.
 */

#ifndef BLUSYS_FRAMEWORK_TEST_FAKE_HAL_H
#define BLUSYS_FRAMEWORK_TEST_FAKE_HAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── GPIO ─────────────────────────────────────────────────────────────── */

/* Set the observable pin level as if an external agent drove it. Fires any
 * interrupt callback that would match the configured mode. */
void         blusys_test_gpio_drive(int pin, bool level);
bool         blusys_test_gpio_last_write(int pin);   /* last level written by product code */
void         blusys_test_gpio_reset_all(void);       /* reset all pin state */

/* ── Timer ────────────────────────────────────────────────────────────── */

/* Advance virtual time by `us` microseconds. Fires every elapsed period,
 * in order, for every started timer. Returns the number of callback hits. */
size_t       blusys_test_timer_advance_us(uint64_t us);
void         blusys_test_timer_reset_all(void);

/* ── UART ─────────────────────────────────────────────────────────────── */

/* Push bytes at the RX side of the first opened UART; wakes any registered
 * rx_callback synchronously. */
void         blusys_test_uart_push_rx(int port, const void *data, size_t size);

/* Snapshot bytes product code has TX'd since the last call. Returns the
 * number of bytes copied into `out` (up to `out_size`) and drains the buffer. */
size_t       blusys_test_uart_take_tx(int port, void *out, size_t out_size);
void         blusys_test_uart_reset_all(void);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_FRAMEWORK_TEST_FAKE_HAL_H */
