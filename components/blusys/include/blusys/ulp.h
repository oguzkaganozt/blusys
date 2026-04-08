#ifndef BLUSYS_ULP_H
#define BLUSYS_ULP_H

#include <stdbool.h>
#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BLUSYS_ULP_JOB_NONE = 0,
    BLUSYS_ULP_JOB_GPIO_WATCH,
} blusys_ulp_job_t;

typedef struct {
    int      pin;
    bool     wake_on_high;
    uint32_t period_ms;
    uint8_t  stable_samples;
} blusys_ulp_gpio_watch_config_t;

typedef struct {
    bool             valid;
    blusys_ulp_job_t job;
    uint32_t         run_count;
    int32_t          last_value;
    bool             wake_triggered;
} blusys_ulp_result_t;

blusys_err_t blusys_ulp_gpio_watch_configure(const blusys_ulp_gpio_watch_config_t *config);
blusys_err_t blusys_ulp_start(void);
blusys_err_t blusys_ulp_stop(void);
blusys_err_t blusys_ulp_clear_result(void);
blusys_err_t blusys_ulp_get_result(blusys_ulp_result_t *out_result);

#ifdef __cplusplus
}
#endif

#endif
