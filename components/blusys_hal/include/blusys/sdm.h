#ifndef BLUSYS_SDM_H
#define BLUSYS_SDM_H

#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_sdm blusys_sdm_t;

blusys_err_t blusys_sdm_open(int pin, uint32_t sample_rate_hz, blusys_sdm_t **out_sdm);
blusys_err_t blusys_sdm_close(blusys_sdm_t *sdm);
blusys_err_t blusys_sdm_set_density(blusys_sdm_t *sdm, int8_t density);

#ifdef __cplusplus
}
#endif

#endif
