#ifndef BLUSYS_ADC_H
#define BLUSYS_ADC_H

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_adc blusys_adc_t;

typedef enum {
    BLUSYS_ADC_ATTEN_DB_0 = 0,
    BLUSYS_ADC_ATTEN_DB_2_5,
    BLUSYS_ADC_ATTEN_DB_6,
    BLUSYS_ADC_ATTEN_DB_12,
} blusys_adc_atten_t;

blusys_err_t blusys_adc_open(int pin, blusys_adc_atten_t atten, blusys_adc_t **out_adc);
blusys_err_t blusys_adc_close(blusys_adc_t *adc);
blusys_err_t blusys_adc_read_raw(blusys_adc_t *adc, int *out_raw);
blusys_err_t blusys_adc_read_mv(blusys_adc_t *adc, int *out_mv);

#ifdef __cplusplus
}
#endif

#endif
