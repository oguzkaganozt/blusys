#pragma once

// Input bridge — wires hardware encoder to LVGL indev and framework intents.
//
// The input bridge owns:
//   1. Opening the blusys_encoder_t hardware driver
//   2. Creating an lv_indev_t (LV_INDEV_TYPE_ENCODER) with a read callback
//   3. Translating encoder events to LVGL focus/input and framework intents
//   4. Owning the lv_group_t for focus traversal
//
// Product code never touches this directly — the device entry path
// creates and manages the bridge when the device_profile has
// has_encoder == true.

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/drivers/encoder.h"
#include "blusys/hal/error.h"
#include "blusys/framework/engine/event_queue.hpp"

#include "lvgl.h"

#include <cstdint>

namespace blusys {

struct input_bridge_config {
    blusys_encoder_config_t     encoder_config;
    blusys::runtime *framework_runtime; // for posting intents
};

struct input_bridge {
    blusys_encoder_t *encoder = nullptr;
    lv_indev_t       *indev   = nullptr;
    lv_group_t       *group   = nullptr;
};

// Open the encoder hardware, create the LVGL indev, and set up the
// focus group. Returns BLUSYS_OK on success.
blusys_err_t input_bridge_open(const input_bridge_config &config,
                               input_bridge *out);

// Tear down the input bridge: close encoder, delete indev and group.
void input_bridge_close(input_bridge *bridge);

// Populate the focus group from the given screen's widget tree.
// Call after building the screen (typically from on_init or after
// page_load). Safe to call multiple times — clears the group first.
void input_bridge_attach_screen(input_bridge *bridge, lv_obj_t *screen);

}  // namespace blusys

#endif  // BLUSYS_FRAMEWORK_HAS_UI
