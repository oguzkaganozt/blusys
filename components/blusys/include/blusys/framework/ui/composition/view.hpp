#pragma once

// Umbrella include for the blusys::app view layer.
//
// Provides action-bound widgets, binding helpers, page helpers,
// overlay management, custom widget contract, and LVGL scope markers.

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/binding/action_widgets.hpp"
#include "blusys/framework/ui/binding/bindings.hpp"
#include "blusys/framework/ui/binding/composites.hpp"
#include "blusys/framework/ui/extension/custom_widget.hpp"
#include "blusys/framework/ui/extension/lvgl_scope.hpp"
#include "blusys/framework/ui/composition/overlay_manager.hpp"
#include "blusys/framework/ui/composition/page.hpp"
#include "blusys/framework/ui/composition/screen_registry.hpp"
#include "blusys/framework/ui/composition/screen_router.hpp"
#include "blusys/framework/ui/composition/shell.hpp"

#endif  // BLUSYS_FRAMEWORK_HAS_UI
