#pragma once

// Device platform profile for blusys::app products.
//
// A device_profile groups all hardware configuration needed to bring
// up a device target: LCD panel, UI service, and optional encoder
// input. The interactive entry path (BLUSYS_APP) uses the
// profile to perform framework-owned display and input bring-up so
// that product code never touches raw LCD init, UI service wiring,
// or encoder setup.
//
// Profiles are plain structs — override any field after construction.
// The ui.lcd pointer is left null in profile definitions; the device
// entry path fills it after blusys_lcd_open() succeeds.

#include "blusys/drivers/lcd.h"
#include "blusys/drivers/display.h"
#include "blusys/drivers/encoder.h"
#include "blusys/drivers/button.h"
#include "blusys/framework/events/event.hpp"

#include <cstddef>

namespace blusys {

static constexpr std::size_t kMaxProfileButtons = 8;

// Mapping from a hardware button to framework intents.
struct profile_button_mapping {
    blusys_button_config_t      button_config;
    blusys::intent   on_press      = blusys::intent::confirm;
    blusys::intent   on_long_press = blusys::intent::cancel;
};

struct device_profile {
    blusys_lcd_config_t       lcd;           // LCD hardware config
    blusys_display_config_t        ui;            // UI service config (ui.lcd filled at runtime)
    blusys_encoder_config_t   encoder;       // encoder config (ignored if has_encoder == false)
    bool                      has_encoder = false;
    bool                      has_touch   = false;
    int                       brightness  = 100; // initial backlight 0-100

    // Button array (ignored if button_count == 0).
    // Points to a product-owned array that must outlive the profile.
    const profile_button_mapping *buttons      = nullptr;
    std::size_t                   button_count = 0;
};

}  // namespace blusys
