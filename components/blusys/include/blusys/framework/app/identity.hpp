#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/framework/ui/style/theme.hpp"
#endif

#include "blusys/framework/feedback/presets.hpp"

namespace blusys {

// Product identity — the single struct where a product declares its
// visual language, feedback behavior, and name.
//
// Composed in `integration/` and passed to the runtime at startup via app_spec.
// The runtime applies the theme and selects the feedback preset at boot.
//
// Motion and icon selection live on `theme_tokens` (durations, paths, optional
// icon set, and `feedback_voice` for default feedback alignment). When
// `feedback` is null, the runtime uses `theme_tokens::feedback_voice` to pick
// the built-in haptic preset for visual-flash timing. Set `feedback`
// explicitly when you need a custom mapping.
struct app_identity {
    const char *product_name = nullptr;

#ifdef BLUSYS_FRAMEWORK_HAS_UI
    const blusys::theme_tokens *theme = nullptr;
#endif

    const blusys::feedback_preset *feedback = nullptr;
};

}  // namespace blusys
