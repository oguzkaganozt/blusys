#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/style/interaction_effects.hpp"

namespace blusys {

namespace {

const blusys::feedback_timing *timing_for(blusys::feedback_pattern pattern,
                                                       const blusys::feedback_preset &preset)
{
    using blusys::feedback_pattern;
    switch (pattern) {
    case feedback_pattern::click:
        return &preset.click;
    case feedback_pattern::focus:
        return &preset.focus;
    case feedback_pattern::confirm:
        return &preset.confirm;
    case feedback_pattern::success:
        return &preset.success;
    case feedback_pattern::warning:
        return &preset.warning;
    case feedback_pattern::error:
        return &preset.error;
    case feedback_pattern::notification:
        return &preset.notification;
    default:
        return &preset.click;
    }
}

}  // namespace

std::uint32_t visual_flash_ms(const blusys::feedback_preset &preset,
                              blusys::feedback_pattern pattern,
                              const theme_tokens &theme)
{
    const blusys::feedback_timing *ft = timing_for(pattern, preset);
    std::uint16_t base = ft->duration_ms;
    if (base == 0) {
        base = theme.anim_duration_fast;
    }

    // Cap by theme motion budget so operational (animations off) stays subtle.
    const std::uint16_t cap =
        theme.anim_enabled ? theme.anim_duration_slow : theme.anim_duration_normal;
    if (cap > 0 && base > cap) {
        base = cap;
    }
    return static_cast<std::uint32_t>(base);
}

}  // namespace blusys

#endif  // BLUSYS_FRAMEWORK_HAS_UI
