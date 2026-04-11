#pragma once

#include <cstdint>

#include "blusys/framework/core/feedback.hpp"
#include "blusys/framework/core/feedback_presets.hpp"
#include "blusys/framework/ui/theme.hpp"

namespace blusys::ui {

// Combine haptic/audio timing from feedback_preset with motion tokens so
// visual flashes stay short and consistent with the active theme.
[[nodiscard]] std::uint32_t visual_flash_ms(const blusys::framework::feedback_preset &preset,
                                            blusys::framework::feedback_pattern pattern,
                                            const theme_tokens &theme);

}  // namespace blusys::ui
