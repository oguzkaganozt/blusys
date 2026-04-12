#include "blusys/framework/framework.hpp"

#include "blusys/log.h"

namespace {

constexpr const char *kTag = "framework_core_basic";

class logging_route_sink final : public blusys::framework::route_sink {
public:
    void submit(const blusys::framework::route_command &command) override
    {
        BLUSYS_LOGI(kTag,
                    "route command: %s id=%lu transition=%lu",
                    blusys::framework::route_command_type_name(command.type),
                    static_cast<unsigned long>(command.id),
                    static_cast<unsigned long>(command.transition));
    }
};

class logging_feedback_sink final : public blusys::framework::feedback_sink {
public:
    bool supports(blusys::framework::feedback_channel channel) const override
    {
        return channel == blusys::framework::feedback_channel::visual
            || channel == blusys::framework::feedback_channel::audio;
    }

    void emit(const blusys::framework::feedback_event &event) override
    {
        BLUSYS_LOGI(kTag,
                    "feedback: channel=%s pattern=%s value=%lu",
                    blusys::framework::feedback_channel_name(event.channel),
                    blusys::framework::feedback_pattern_name(event.pattern),
                    static_cast<unsigned long>(event.value));
    }
};

class demo_controller final : public blusys::framework::controller {
public:
    blusys_err_t init() override
    {
        emit_feedback({
            .channel = blusys::framework::feedback_channel::visual,
            .pattern = blusys::framework::feedback_pattern::focus,
            .value = 1,
            .payload = nullptr,
        });
        return BLUSYS_OK;
    }

    void handle(const blusys::framework::app_event &event) override
    {
        if (event.kind != blusys::framework::app_event_kind::intent) {
            return;
        }

        switch (blusys::framework::app_event_intent(event)) {
        case blusys::framework::intent::press:
            submit_route(blusys::framework::route::set_root(1));
            emit_feedback({
                .channel = blusys::framework::feedback_channel::audio,
                .pattern = blusys::framework::feedback_pattern::click,
                .value = 1,
                .payload = nullptr,
            });
            break;
        case blusys::framework::intent::confirm:
            submit_route(blusys::framework::route::show_overlay(7));
            emit_feedback({
                .channel = blusys::framework::feedback_channel::visual,
                .pattern = blusys::framework::feedback_pattern::confirm,
                .value = 1,
                .payload = nullptr,
            });
            break;
        default:
            break;
        }
    }
};

}  // namespace

extern "C" void app_main(void)
{
    logging_route_sink route_sink;
    logging_feedback_sink feedback_sink;
    blusys::framework::runtime runtime;
    demo_controller controller;

    runtime.register_feedback_sink(&feedback_sink);
    runtime.init(&controller, &route_sink, 10);
    runtime.post_intent(blusys::framework::intent::press);
    runtime.post_intent(blusys::framework::intent::confirm);
    runtime.step(0);
    runtime.step(10);
    runtime.deinit();
}
