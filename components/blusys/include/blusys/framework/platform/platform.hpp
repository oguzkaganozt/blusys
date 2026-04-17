#pragma once

// Platform-layer umbrella — automatic profile selection, display constants,
// input bridges, and all first-party display profiles.
//
// Included by blusys/blusys.hpp so app authors never need to reach in.

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/platform/auto.hpp"
#include "blusys/framework/platform/auto_shell.hpp"
#include "blusys/framework/platform/build.hpp"
#include "blusys/framework/platform/dashboard_display_dims.hpp"
#include "blusys/framework/platform/display_constants.hpp"
#include "blusys/framework/platform/headless.hpp"
#include "blusys/framework/platform/host.hpp"
#include "blusys/framework/platform/input_bridge.hpp"
#include "blusys/framework/platform/layout_surface.hpp"
#include "blusys/framework/platform/profile.hpp"
#include "blusys/framework/platform/reference_build.hpp"

#include "blusys/framework/platform/profiles/ili9341.hpp"
#include "blusys/framework/platform/profiles/ili9488.hpp"
#include "blusys/framework/platform/profiles/qemu_rgb.hpp"
#include "blusys/framework/platform/profiles/ssd1306.hpp"
#include "blusys/framework/platform/profiles/st7735.hpp"
#include "blusys/framework/platform/profiles/st7789.hpp"

#endif  // BLUSYS_FRAMEWORK_HAS_UI
