#include "blusys/framework/feedback/internal/logging_sink.hpp"

#include "blusys/framework/feedback/presets.hpp"
#include "blusys/hal/log.h"

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/framework/ui/style/theme.hpp"
#include "blusys/framework/ui/style/interaction_effects.hpp"
#endif

namespace blusys::detail {

void feedback_logging_sink::set_identity(const app_identity *identity)
{
    identity_ = identity;
}

bool feedback_logging_sink::supports(blusys::feedback_channel) const
{
    return true;
}

void feedback_logging_sink::emit(const blusys::feedback_event &event)
{
    std::uint32_t value = event.value;

#ifdef BLUSYS_FRAMEWORK_HAS_UI
    if (event.channel == blusys::feedback_channel::visual && value == 0) {
        const blusys::feedback_preset *preset = nullptr;
        if (identity_ != nullptr && identity_->feedback != nullptr) {
            preset = identity_->feedback;
        } else {
            const auto v = blusys::theme().feedback_voice;
            preset = (v == blusys::theme_feedback_voice::operational)
                         ? &blusys::feedback_preset_operational()
                         : &blusys::feedback_preset_expressive();
        }
        value = blusys::visual_flash_ms(*preset, event.pattern, blusys::theme());
    }
#endif

    BLUSYS_LOGI("blusys_app",
                "feedback: channel=%s pattern=%s value=%lu",
                blusys::feedback_channel_name(event.channel),
                blusys::feedback_pattern_name(event.pattern),
                static_cast<unsigned long>(value));
}

}  // namespace blusys::detail
