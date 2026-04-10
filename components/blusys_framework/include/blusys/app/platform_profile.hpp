#pragma once

// Device platform profile for blusys::app products.
//
// A device_profile bundles all hardware configuration needed to bring
// up a device target: LCD panel, UI service, and optional encoder
// input. The device entry path (BLUSYS_APP_MAIN_DEVICE) uses the
// profile to perform framework-owned display and input bring-up so
// that product code never touches raw LCD init, UI service wiring,
// or encoder setup.
//
// Profiles are plain structs — override any field after construction.
// The ui.lcd pointer is left null in profile definitions; the device
// entry path fills it after blusys_lcd_open() succeeds.

#include "blusys/drivers/display/lcd.h"
#include "blusys/display/ui.h"
#include "blusys/drivers/input/encoder.h"

namespace blusys::app {

struct device_profile {
    blusys_lcd_config_t       lcd;           // LCD hardware config
    blusys_ui_config_t        ui;            // UI service config (ui.lcd filled at runtime)
    blusys_encoder_config_t   encoder;       // encoder config (ignored if has_encoder == false)
    bool                      has_encoder = false;
    int                       brightness  = 100; // initial backlight 0-100
};

}  // namespace blusys::app
