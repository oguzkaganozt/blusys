#ifndef BLUSYS_UI_H
#define BLUSYS_UI_H

#include <stdint.h>

#include "blusys/error.h"
#include "blusys/display/lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_ui blusys_ui_t;

typedef struct {
    blusys_lcd_t *lcd;            /* Required: already-opened LCD handle */
    uint32_t      buf_lines;      /* Draw buffer height in lines (0 = default 20) */
    int           task_priority;  /* LVGL render task priority (0 = default 5) */
    int           task_stack;     /* Render task stack in bytes (0 = default 8192) */
} blusys_ui_config_t;

blusys_err_t blusys_ui_open(const blusys_ui_config_t *config,
                            blusys_ui_t **out_ui);
blusys_err_t blusys_ui_close(blusys_ui_t *ui);

/* Lock/unlock for thread-safe LVGL widget access from non-render tasks. */
blusys_err_t blusys_ui_lock(blusys_ui_t *ui);
blusys_err_t blusys_ui_unlock(blusys_ui_t *ui);

#ifdef __cplusplus
}
#endif

#endif
