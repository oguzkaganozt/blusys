#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/framework/ui/theme.hpp"
#endif

#include "blusys/framework/core/feedback_presets.hpp"

namespace blusys::app {

// Product identity — the single struct where a product declares its
// visual language, feedback behavior, and name.
//
// Composed in `system/` and passed to the runtime at startup via app_spec.
// The runtime applies the theme and selects the feedback preset at boot.
struct app_identity {
    const char *product_name = nullptr;

#ifdef BLUSYS_FRAMEWORK_HAS_UI
    const blusys::ui::theme_tokens *theme = nullptr;
#endif

    const blusys::framework::feedback_preset *feedback = nullptr;
};

}  // namespace blusys::app
