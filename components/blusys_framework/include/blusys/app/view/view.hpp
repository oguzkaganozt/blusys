#pragma once

// Umbrella include for the blusys::app view layer.
//
// Provides action-bound widgets, binding helpers, page helpers,
// overlay management, custom widget contract, and LVGL scope markers.

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/view/action_widgets.hpp"
#include "blusys/app/view/bindings.hpp"
#include "blusys/app/view/composites.hpp"
#include "blusys/app/view/custom_widget.hpp"
#include "blusys/app/view/lvgl_scope.hpp"
#include "blusys/app/view/overlay_manager.hpp"
#include "blusys/app/view/page.hpp"
#include "blusys/app/view/screen_registry.hpp"
#include "blusys/app/view/screen_router.hpp"
#include "blusys/app/view/shell.hpp"

#endif  // BLUSYS_FRAMEWORK_HAS_UI
