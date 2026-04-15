#include "sdkconfig.h"

void run_lcd_basic(void);
void run_lcd_st7735_basic(void);
void run_ui_basic(void);
void run_encoder_basic(void);
void run_oled_encoder_basic(void);

void app_main(void)
{
#if CONFIG_DISPLAY_UI_SCENARIO_LCD_BASIC
    run_lcd_basic();
#elif CONFIG_DISPLAY_UI_SCENARIO_LCD_ST7735_BASIC
    run_lcd_st7735_basic();
#elif CONFIG_DISPLAY_UI_SCENARIO_UI_BASIC
    run_ui_basic();
#elif CONFIG_DISPLAY_UI_SCENARIO_ENCODER_BASIC
    run_encoder_basic();
#elif CONFIG_DISPLAY_UI_SCENARIO_OLED_ENCODER
    run_oled_encoder_basic();
#endif
}
