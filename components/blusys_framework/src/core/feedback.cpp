#include "blusys/framework/core/feedback.hpp"

namespace blusys::framework {

bool feedback_bus::register_sink(feedback_sink *sink)
{
    if (sink == nullptr) {
        return false;
    }

    for (std::size_t i = 0; i < sinks_.size(); ++i) {
        if (sinks_[i] == sink) {
            return true;
        }
    }

    return sinks_.push_back(sink);
}

bool feedback_bus::unregister_sink(feedback_sink *sink)
{
    if (sink == nullptr) {
        return false;
    }

    for (std::size_t i = 0; i < sinks_.size(); ++i) {
        if (sinks_[i] != sink) {
            continue;
        }

        for (std::size_t j = i + 1; j < sinks_.size(); ++j) {
            sinks_[j - 1] = sinks_[j];
        }
        sinks_.pop_back();
        return true;
    }

    return false;
}

void feedback_bus::emit(const feedback_event &event) const
{
    for (std::size_t i = 0; i < sinks_.size(); ++i) {
        if (sinks_[i]->supports(event.channel)) {
            sinks_[i]->emit(event);
        }
    }
}

std::size_t feedback_bus::sink_count() const
{
    return sinks_.size();
}

const char *feedback_channel_name(feedback_channel channel)
{
    switch (channel) {
    case feedback_channel::visual:
        return "visual";
    case feedback_channel::audio:
        return "audio";
    case feedback_channel::haptic:
        return "haptic";
    }

    return "unknown";
}

const char *feedback_pattern_name(feedback_pattern pattern)
{
    switch (pattern) {
    case feedback_pattern::click:
        return "click";
    case feedback_pattern::focus:
        return "focus";
    case feedback_pattern::confirm:
        return "confirm";
    case feedback_pattern::success:
        return "success";
    case feedback_pattern::warning:
        return "warning";
    case feedback_pattern::error:
        return "error";
    case feedback_pattern::notification:
        return "notification";
    }

    return "unknown";
}

}  // namespace blusys::framework
