#ifndef BLUSYS_PWM_H
#define BLUSYS_PWM_H

#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_pwm blusys_pwm_t;

blusys_err_t blusys_pwm_open(int pin,
                             uint32_t freq_hz,
                             uint16_t duty_permille,
                             blusys_pwm_t **out_pwm);
blusys_err_t blusys_pwm_close(blusys_pwm_t *pwm);
blusys_err_t blusys_pwm_set_frequency(blusys_pwm_t *pwm, uint32_t freq_hz);
blusys_err_t blusys_pwm_set_duty(blusys_pwm_t *pwm, uint16_t duty_permille);
blusys_err_t blusys_pwm_start(blusys_pwm_t *pwm);
blusys_err_t blusys_pwm_stop(blusys_pwm_t *pwm);

#ifdef __cplusplus
}
#endif

#endif
