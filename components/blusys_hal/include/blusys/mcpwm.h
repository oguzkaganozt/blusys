#ifndef BLUSYS_MCPWM_H
#define BLUSYS_MCPWM_H

#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_mcpwm blusys_mcpwm_t;

blusys_err_t blusys_mcpwm_open(int pin_a,
                               int pin_b,
                               uint32_t freq_hz,
                               uint16_t duty_permille,
                               uint32_t dead_time_ns,
                               blusys_mcpwm_t **out_mcpwm);
blusys_err_t blusys_mcpwm_close(blusys_mcpwm_t *mcpwm);
blusys_err_t blusys_mcpwm_set_duty(blusys_mcpwm_t *mcpwm, uint16_t duty_permille);

#ifdef __cplusplus
}
#endif

#endif
