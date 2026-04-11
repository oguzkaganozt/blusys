#pragma once

#include "blusys/app/app_identity.hpp"
#include "blusys/framework/core/feedback.hpp"

namespace blusys::app::detail {

// Default feedback sink: logs all channels. For the visual channel, enriches
// event.value with theme-consistent flash duration (ms) when value is zero.
class feedback_logging_sink final : public blusys::framework::feedback_sink {
public:
    void set_identity(const app_identity *identity);

    bool supports(blusys::framework::feedback_channel channel) const override;
    void emit(const blusys::framework::feedback_event &event) override;

private:
    const app_identity *identity_ = nullptr;
};

}  // namespace blusys::app::detail
