/**
 * @file pwm.h
 * @brief Single-output PWM: configurable frequency, duty in per-mille (‰).
 *
 * Duty is expressed as 0–1000 (0 = 0%, 500 = 50%, 1000 = 100%). Each handle
 * owns one LEDC timer + channel. See docs/hal/pwm.md.
 */

#ifndef BLUSYS_PWM_H
#define BLUSYS_PWM_H

#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle for a single PWM output (one LEDC timer + channel). */
typedef struct blusys_pwm blusys_pwm_t;

/**
 * @brief Allocate a LEDC timer + channel for the given pin.
 *
 * The output is not active until ::blusys_pwm_start.
 *
 * @param pin            GPIO number.
 * @param freq_hz        PWM frequency in Hz.
 * @param duty_permille  Initial duty cycle in per-mille (0–1000).
 * @param out_pwm        Output handle.
 * @return `BLUSYS_OK`,
 *         `BLUSYS_ERR_INVALID_ARG` for invalid pin/frequency/duty,
 *         `BLUSYS_ERR_BUSY` if the timer pool is exhausted (at most four
 *         handles open at once on C3/S3).
 */
blusys_err_t blusys_pwm_open(int pin,
                             uint32_t freq_hz,
                             uint16_t duty_permille,
                             blusys_pwm_t **out_pwm);

/**
 * @brief Stop the output, release timer/channel, and free the handle.
 * @param pwm Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p pwm is NULL.
 */
blusys_err_t blusys_pwm_close(blusys_pwm_t *pwm);

/**
 * @brief Change PWM frequency while the output may be running.
 * @param pwm      Handle.
 * @param freq_hz  New frequency in Hz.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for zero or out-of-range frequency.
 */
blusys_err_t blusys_pwm_set_frequency(blusys_pwm_t *pwm, uint32_t freq_hz);

/**
 * @brief Change the duty cycle. Takes effect on the next PWM period.
 * @param pwm            Handle.
 * @param duty_permille  0 (0%) to 1000 (100%).
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if duty is out of range.
 */
blusys_err_t blusys_pwm_set_duty(blusys_pwm_t *pwm, uint16_t duty_permille);

/**
 * @brief Enable PWM output on the pin.
 * @param pwm Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p pwm is NULL.
 */
blusys_err_t blusys_pwm_start(blusys_pwm_t *pwm);

/**
 * @brief Disable PWM output without releasing the handle.
 *
 * The pin goes idle. Call ::blusys_pwm_start to resume.
 *
 * @param pwm Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p pwm is NULL.
 */
blusys_err_t blusys_pwm_stop(blusys_pwm_t *pwm);

#ifdef __cplusplus
}
#endif

#endif
