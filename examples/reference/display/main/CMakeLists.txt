idf_component_register(
    SRCS
        "display_ui_main.c"
        "disp_lcd_basic.c"
        "disp_lcd_st7735_basic.c"
        "ref_display_ui_lvgl.c"
        "ref_display_encoder_basic.c"
        "ref_display_oled_encoder.cpp"
    REQUIRES blusys
)
