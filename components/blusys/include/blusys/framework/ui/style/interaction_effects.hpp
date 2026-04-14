#pragma once

#include <cstdint>

#include "blusys/framework/feedback/feedback.hpp"
#include "blusys/framework/feedback/presets.hpp"
#include "blusys/framework/ui/style/theme.hpp"

namespace blusys {

// Combine haptic/audio timing from feedback_preset with motion tokens so
// visual flashes stay short and consistent with the active theme.
[[nodiscard]] std::uint32_t visual_flash_ms(const blusys::feedback_preset &preset,
                                            blusys::feedback_pattern pattern,
                                            const theme_tokens &theme);

}  // namespace blusys
