#pragma once

#include "blusys/framework/app/identity.hpp"
#include "blusys/framework/feedback/feedback.hpp"

namespace blusys::detail {

// Default feedback sink: logs all channels. For the visual channel, enriches
// event.value with theme-consistent flash duration (ms) when value is zero.
class feedback_logging_sink final : public blusys::feedback_sink {
public:
    void set_identity(const app_identity *identity);

    bool supports(blusys::feedback_channel channel) const override;
    void emit(const blusys::feedback_event &event) override;

private:
    const app_identity *identity_ = nullptr;
};

}  // namespace blusys::detail
