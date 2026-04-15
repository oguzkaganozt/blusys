#if CONFIG_PLATFORM_LAB_SCENARIO_FRAMEWORK_CORE
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

static blusys_err_t core_demo_on_init(void * /*ctx*/, blusys::framework::feedback_bus *fb)
{
    blusys::framework::feedback_dispatch(fb, {
        .channel = blusys::framework::feedback_channel::visual,
        .pattern = blusys::framework::feedback_pattern::focus,
        .value = 1,
        .payload = nullptr,
    });
    return BLUSYS_OK;
}

static void core_demo_on_event(void * /*ctx*/,
                               const blusys::framework::app_event &event,
                               blusys::framework::feedback_bus *fb,
                               blusys::framework::route_sink *routes)
{
    if (event.kind != blusys::framework::app_event_kind::intent) {
        return;
    }

    switch (blusys::framework::app_event_intent(event)) {
    case blusys::framework::intent::press:
        blusys::framework::route_dispatch(routes, blusys::framework::route::set_root(1));
        blusys::framework::feedback_dispatch(fb, {
            .channel = blusys::framework::feedback_channel::audio,
            .pattern = blusys::framework::feedback_pattern::click,
            .value = 1,
            .payload = nullptr,
        });
        break;
    case blusys::framework::intent::confirm:
        blusys::framework::route_dispatch(routes, blusys::framework::route::show_overlay(7));
        blusys::framework::feedback_dispatch(fb, {
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

}  // namespace

extern "C" void run_platform_framework_core(void)
{
    logging_route_sink route_sink;
    logging_feedback_sink feedback_sink;
    blusys::framework::runtime runtime;

    runtime.register_feedback_sink(&feedback_sink);

    blusys::framework::runtime_handler handler{};
    handler.context      = nullptr;
    handler.on_init      = core_demo_on_init;
    handler.handle_event = core_demo_on_event;
    handler.on_tick      = nullptr;
    handler.on_deinit    = nullptr;

    runtime.init(&route_sink, handler, 10);
    runtime.post_intent(blusys::framework::intent::press);
    runtime.post_intent(blusys::framework::intent::confirm);
    runtime.step(0);
    runtime.step(10);
    runtime.deinit();
}

#else
extern "C" void run_platform_framework_core(void) {}
#endif
