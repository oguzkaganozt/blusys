#if CONFIG_PLATFORM_LAB_SCENARIO_FRAMEWORK_CORE
#include "blusys/blusys.hpp"


namespace {

constexpr const char *kTag = "framework_core_basic";

class logging_route_sink final : public blusys::route_sink {
public:
    void submit(const blusys::route_command &command) override
    {
        BLUSYS_LOGI(kTag,
                    "route command: %s id=%lu transition=%lu",
                    blusys::route_command_type_name(command.type),
                    static_cast<unsigned long>(command.id),
                    static_cast<unsigned long>(command.transition));
    }
};

class logging_feedback_sink final : public blusys::feedback_sink {
public:
    bool supports(blusys::feedback_channel channel) const override
    {
        return channel == blusys::feedback_channel::visual
            || channel == blusys::feedback_channel::audio;
    }

    void emit(const blusys::feedback_event &event) override
    {
        BLUSYS_LOGI(kTag,
                    "feedback: channel=%s pattern=%s value=%lu",
                    blusys::feedback_channel_name(event.channel),
                    blusys::feedback_pattern_name(event.pattern),
                    static_cast<unsigned long>(event.value));
    }
};

static blusys_err_t core_demo_on_init(void * /*ctx*/, blusys::feedback_bus *fb)
{
    blusys::feedback_dispatch(fb, {
        .channel = blusys::feedback_channel::visual,
        .pattern = blusys::feedback_pattern::focus,
        .value = 1,
        .payload = nullptr,
    });
    return BLUSYS_OK;
}

static void core_demo_on_event(void * /*ctx*/,
                               const blusys::app_event &event,
                               blusys::feedback_bus *fb,
                               blusys::route_sink *routes)
{
    if (event.source != blusys::event_source::intent) {
        return;
    }

    switch (static_cast<blusys::intent>(event.kind)) {
    case blusys::intent::press:
        blusys::route_dispatch(routes, blusys::route::set_root(1));
        blusys::feedback_dispatch(fb, {
            .channel = blusys::feedback_channel::audio,
            .pattern = blusys::feedback_pattern::click,
            .value = 1,
            .payload = nullptr,
        });
        break;
    case blusys::intent::confirm:
        blusys::route_dispatch(routes, blusys::route::show_overlay(7));
        blusys::feedback_dispatch(fb, {
            .channel = blusys::feedback_channel::visual,
            .pattern = blusys::feedback_pattern::confirm,
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
    blusys::runtime runtime;

    runtime.register_feedback_sink(&feedback_sink);

    blusys::runtime_handler handler{};
    handler.context      = nullptr;
    handler.on_init      = core_demo_on_init;
    handler.handle_event = core_demo_on_event;
    handler.on_tick      = nullptr;
    handler.on_deinit    = nullptr;

    runtime.init(&route_sink, handler, 10);
    runtime.post_intent(blusys::intent::press);
    runtime.post_intent(blusys::intent::confirm);
    runtime.step(0);
    runtime.step(10);
    runtime.deinit();
}

#else
extern "C" void run_platform_framework_core(void) {}
#endif
