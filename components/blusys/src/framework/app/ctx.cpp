#include "blusys/framework/app/ctx.hpp"
#include "blusys/framework/app/internal/app_runtime.hpp"

namespace blusys {

void app_ctx::emit_feedback(blusys::feedback_channel channel,
                            blusys::feedback_pattern pattern,
                            std::uint32_t value)
{
    if (feedback_bus_ != nullptr) {
        feedback_bus_->emit({
            .channel = channel,
            .pattern = pattern,
            .value   = value,
            .payload = nullptr,
        });
    }
}

}  // namespace blusys
