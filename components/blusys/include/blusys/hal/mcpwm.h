/**
 * @file mcpwm.h
 * @brief Complementary-pair PWM (MCPWM): two outputs with configurable dead-time.
 *
 * Intended for driving half-bridge / H-bridge power stages. One handle owns
 * one complementary pair. See docs/hal/mcpwm.md.
 */

#ifndef BLUSYS_MCPWM_H
#define BLUSYS_MCPWM_H

#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle for one MCPWM complementary pair. */
typedef struct blusys_mcpwm blusys_mcpwm_t;

/**
 * @brief Configure a complementary PWM pair with dead-time insertion.
 *
 * @param pin_a          GPIO for the active-high output.
 * @param pin_b          GPIO for the complementary (inverted) output.
 * @param freq_hz        PWM frequency in Hz.
 * @param duty_permille  Initial duty cycle in per-mille (0–1000).
 * @param dead_time_ns   Dead-time in nanoseconds applied between A→B and B→A transitions.
 * @param out_mcpwm      Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid arguments,
 *         `BLUSYS_ERR_BUSY` if the MCPWM pool is exhausted.
 */
blusys_err_t blusys_mcpwm_open(int pin_a,
                               int pin_b,
                               uint32_t freq_hz,
                               uint16_t duty_permille,
                               uint32_t dead_time_ns,
                               blusys_mcpwm_t **out_mcpwm);

/** @brief Stop the outputs and release the handle. */
blusys_err_t blusys_mcpwm_close(blusys_mcpwm_t *mcpwm);

/**
 * @brief Change the duty cycle. Takes effect on the next PWM period.
 * @param mcpwm          Handle.
 * @param duty_permille  0 (0%) to 1000 (100%).
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if duty is out of range.
 */
blusys_err_t blusys_mcpwm_set_duty(blusys_mcpwm_t *mcpwm, uint16_t duty_permille);

#ifdef __cplusplus
}
#endif

#endif
