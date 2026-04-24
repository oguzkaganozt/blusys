# UI

LVGL-based display runtime. It opens the LCD, starts the render task, and serialises access to LVGL.

## At a glance

- low-level internal service
- product code should use profiles and views instead
- one UI instance at a time

## Quick example

```c
blusys_lcd_t *lcd = NULL;
blusys_lcd_config_t lcd_cfg = { .driver = BLUSYS_LCD_DRIVER_ST7735 };
blusys_lcd_open(&lcd_cfg, &lcd);

blusys_display_t *ui = NULL;
blusys_display_config_t ui_cfg = { .lcd = lcd };
blusys_display_open(&ui_cfg, &ui);

blusys_display_lock(ui);
lv_obj_t *label = lv_label_create(lv_screen_active());
lv_label_set_text(label, "Hello");
blusys_display_unlock(ui);
```

## Common mistakes

- touching LVGL without the lock
- closing the LCD before the UI
- opening more than one UI instance

## Target support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Thread safety

- the render task owns LVGL
- all other tasks must lock before calling `lv_*`

## Limitations

- requires `lvgl/lvgl`
- singleton instance only

## Example app

`examples/reference/display/`
