#pragma once

// Umbrella include for the blusys::app product-facing API.
//
// Product code should include this single header. It provides:
//   - app_spec<State, Action>  — define your app
//   - app_ctx                  — dispatch, navigate, feedback
//   - entry macros             — BLUSYS_APP_MAIN_HOST, _HEADLESS, _DEVICE
//   - theme presets            — dark, oled, light (when UI is available)
//
// For device targets, also include a profile header:
//   #include "blusys/app/profiles/st7735.hpp"

#include "blusys/app/app_spec.hpp"
#include "blusys/app/app_ctx.hpp"
#include "blusys/app/capability_list.hpp"
#include "blusys/app/entry.hpp"

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/app/theme_presets.hpp"
#include "blusys/app/view/view.hpp"
#endif
