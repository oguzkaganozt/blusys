#include "blusys/app/detail/feedback_logging_sink.hpp"

#include "blusys/framework/core/feedback_presets.hpp"
#include "blusys/log.h"

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/framework/ui/theme.hpp"
#include "blusys/framework/ui/visual_feedback.hpp"
#endif

namespace blusys::app::detail {

void feedback_logging_sink::set_identity(const app_identity *identity)
{
    identity_ = identity;
}

bool feedback_logging_sink::supports(blusys::framework::feedback_channel) const
{
    return true;
}

void feedback_logging_sink::emit(const blusys::framework::feedback_event &event)
{
    std::uint32_t value = event.value;

#ifdef BLUSYS_FRAMEWORK_HAS_UI
    if (event.channel == blusys::framework::feedback_channel::visual && value == 0) {
        const blusys::framework::feedback_preset *preset = nullptr;
        if (identity_ != nullptr && identity_->feedback != nullptr) {
            preset = identity_->feedback;
        } else {
            const auto v = blusys::ui::theme().feedback_voice;
            preset = (v == blusys::ui::theme_feedback_voice::operational)
                         ? &blusys::framework::feedback_preset_operational()
                         : &blusys::framework::feedback_preset_expressive();
        }
        value = blusys::ui::visual_flash_ms(*preset, event.pattern, blusys::ui::theme());
    }
#endif

    BLUSYS_LOGI("blusys_app",
                "feedback: channel=%s pattern=%s value=%lu",
                blusys::framework::feedback_channel_name(event.channel),
                blusys::framework::feedback_pattern_name(event.pattern),
                static_cast<unsigned long>(value));
}

}  // namespace blusys::app::detail
