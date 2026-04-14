#ifndef BLUSYS_TOUCH_H
#define BLUSYS_TOUCH_H

#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_touch blusys_touch_t;

blusys_err_t blusys_touch_open(int pin, blusys_touch_t **out_touch);
blusys_err_t blusys_touch_close(blusys_touch_t *touch);
blusys_err_t blusys_touch_read(blusys_touch_t *touch, uint32_t *out_value);

#ifdef __cplusplus
}
#endif

#endif
