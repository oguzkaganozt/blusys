#ifndef BLUSYS_DAC_H
#define BLUSYS_DAC_H

#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_dac blusys_dac_t;

blusys_err_t blusys_dac_open(int pin, blusys_dac_t **out_dac);
blusys_err_t blusys_dac_close(blusys_dac_t *dac);
blusys_err_t blusys_dac_write(blusys_dac_t *dac, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif
