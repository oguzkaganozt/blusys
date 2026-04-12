#ifndef BLUSYS_TEMP_SENSOR_H
#define BLUSYS_TEMP_SENSOR_H

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_temp_sensor blusys_temp_sensor_t;

blusys_err_t blusys_temp_sensor_open(blusys_temp_sensor_t **out_tsens);
blusys_err_t blusys_temp_sensor_close(blusys_temp_sensor_t *tsens);
blusys_err_t blusys_temp_sensor_read_celsius(blusys_temp_sensor_t *tsens, float *out_celsius);

#ifdef __cplusplus
}
#endif

#endif
