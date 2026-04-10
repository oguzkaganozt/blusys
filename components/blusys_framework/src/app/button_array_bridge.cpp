#if defined(BLUSYS_FRAMEWORK_HAS_UI) && defined(ESP_PLATFORM)

#include "blusys/app/button_array_bridge.hpp"
#include "blusys/log.h"

namespace blusys::app {

namespace {

constexpr const char *kTag = "btn_bridge";

// Per-button context stored alongside the HAL button handle.
struct button_cb_ctx {
    blusys::framework::runtime *runtime       = nullptr;
    blusys::framework::intent   on_press      = blusys::framework::intent::confirm;
    blusys::framework::intent   on_long_press = blusys::framework::intent::cancel;
};

static button_cb_ctx g_button_ctx[kMaxButtons] = {};

void button_hw_callback(blusys_button_t * /*button*/,
                        blusys_button_event_t event,
                        void *user_ctx)
{
    auto *ctx = static_cast<button_cb_ctx *>(user_ctx);
    if (ctx == nullptr || ctx->runtime == nullptr) {
        return;
    }

    switch (event) {
    case BLUSYS_BUTTON_EVENT_PRESS:
        ctx->runtime->post_intent(ctx->on_press);
        break;
    case BLUSYS_BUTTON_EVENT_LONG_PRESS:
        ctx->runtime->post_intent(ctx->on_long_press);
        break;
    case BLUSYS_BUTTON_EVENT_RELEASE:
        ctx->runtime->post_intent(blusys::framework::intent::release);
        break;
    }
}

}  // namespace

blusys_err_t button_array_open(const button_array_config &config,
                                button_array_bridge *out)
{
    if (out == nullptr || config.framework_runtime == nullptr || config.count == 0) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    *out = {};

    const std::size_t count = (config.count > kMaxButtons) ? kMaxButtons : config.count;

    for (std::size_t i = 0; i < count; ++i) {
        g_button_ctx[i] = {
            .runtime       = config.framework_runtime,
            .on_press      = config.buttons[i].on_press,
            .on_long_press = config.buttons[i].on_long_press,
        };

        blusys_err_t err = blusys_button_open(&config.buttons[i].button_config,
                                               &out->buttons[i]);
        if (err != BLUSYS_OK) {
            BLUSYS_LOGW(kTag, "button[%zu] open failed: %d",
                        i, static_cast<int>(err));
            continue;
        }

        err = blusys_button_set_callback(out->buttons[i], button_hw_callback,
                                          &g_button_ctx[i]);
        if (err != BLUSYS_OK) {
            BLUSYS_LOGW(kTag, "button[%zu] set_callback failed: %d",
                        i, static_cast<int>(err));
            blusys_button_close(out->buttons[i]);
            out->buttons[i] = nullptr;
            continue;
        }

        out->count++;
    }

    BLUSYS_LOGI(kTag, "button array bridge opened (%zu/%zu)",
                out->count, count);
    return BLUSYS_OK;
}

void button_array_close(button_array_bridge *bridge)
{
    if (bridge == nullptr) {
        return;
    }
    for (std::size_t i = 0; i < kMaxButtons; ++i) {
        if (bridge->buttons[i] != nullptr) {
            blusys_button_close(bridge->buttons[i]);
            bridge->buttons[i] = nullptr;
        }
        g_button_ctx[i] = {};
    }
    bridge->count = 0;
    BLUSYS_LOGI(kTag, "button array bridge closed");
}

}  // namespace blusys::app

#endif  // BLUSYS_FRAMEWORK_HAS_UI && ESP_PLATFORM
