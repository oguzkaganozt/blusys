#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/platform/input_bridge.hpp"

#include "blusys/framework/engine/intent.hpp"
#include "blusys/framework/ui/input/encoder.hpp"
#include "blusys/hal/log.h"

#include <atomic>
#include <cstdint>

namespace blusys {
namespace {

constexpr const char *kTag = "input_bridge";

// Shared state between encoder ISR callback and LVGL indev read callback.
// Only atomic types are touched from the ISR; the indev read runs in
// the LVGL timer task context.

struct encoder_shared_state {
    std::atomic<std::int32_t> diff{0};
    std::atomic<bool>         pressed{false};
    std::atomic<bool>         press_pending{false};  // edge-triggered: set on press, cleared after read
    blusys::runtime *framework_runtime = nullptr;
};

// One bridge instance at a time (matches blusys_display singleton rule).
encoder_shared_state g_encoder_state{};

// ---- encoder hardware callback (ISR-safe: only touches atomics) ----

void encoder_hw_callback(blusys_encoder_t * /*encoder*/,
                         blusys_encoder_event_t event,
                         int /*position*/,
                         void * /*user_ctx*/)
{
    switch (event) {
    case BLUSYS_ENCODER_EVENT_CW:
        g_encoder_state.diff.fetch_add(1, std::memory_order_relaxed);
        break;
    case BLUSYS_ENCODER_EVENT_CCW:
        g_encoder_state.diff.fetch_sub(1, std::memory_order_relaxed);
        break;
    case BLUSYS_ENCODER_EVENT_PRESS:
        g_encoder_state.pressed.store(true, std::memory_order_relaxed);
        g_encoder_state.press_pending.store(true, std::memory_order_relaxed);
        break;
    case BLUSYS_ENCODER_EVENT_RELEASE:
        g_encoder_state.pressed.store(false, std::memory_order_relaxed);
        break;
    case BLUSYS_ENCODER_EVENT_LONG_PRESS:
        // Could map to intent::long_press in the future.
        break;
    }
}

// ---- LVGL indev read callback (runs in LVGL timer task context) ----

void indev_read_cb(lv_indev_t * /*indev*/, lv_indev_data_t *data)
{
    const std::int32_t diff = g_encoder_state.diff.exchange(0, std::memory_order_relaxed);
    const bool pressed = g_encoder_state.pressed.load(std::memory_order_relaxed);

    data->enc_diff = static_cast<std::int32_t>(diff);
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;

    // Post framework intents so the app's `on_event` can process encoder
    // input through the standard reducer pipeline. This runs in the LVGL
    // timer task, not in ISR context, so runtime->post_intent is safe.
    if (g_encoder_state.framework_runtime != nullptr) {
        if (diff > 0) {
            for (std::int32_t i = 0; i < diff; ++i) {
                g_encoder_state.framework_runtime->post_intent(
                    blusys::intent::increment);
            }
        } else if (diff < 0) {
            for (std::int32_t i = 0; i < -diff; ++i) {
                g_encoder_state.framework_runtime->post_intent(
                    blusys::intent::decrement);
            }
        }
        // Fire confirm only on the press edge, not on every poll while held.
        if (g_encoder_state.press_pending.exchange(false, std::memory_order_relaxed)) {
            g_encoder_state.framework_runtime->post_intent(
                blusys::intent::confirm);
        }
    }
}

}  // namespace

blusys_err_t input_bridge_open(const input_bridge_config &config,
                               input_bridge *out)
{
    if (out == nullptr) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    *out = {};

    // Reset shared state.
    g_encoder_state.diff.store(0, std::memory_order_relaxed);
    g_encoder_state.pressed.store(false, std::memory_order_relaxed);
    g_encoder_state.press_pending.store(false, std::memory_order_relaxed);
    g_encoder_state.framework_runtime = config.framework_runtime;

    // Open hardware encoder.
    blusys_err_t err = blusys_encoder_open(&config.encoder_config, &out->encoder);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE(kTag, "encoder open failed: %d", static_cast<int>(err));
        return err;
    }

    err = blusys_encoder_set_callback(out->encoder, encoder_hw_callback, nullptr);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE(kTag, "encoder set_callback failed: %d", static_cast<int>(err));
        blusys_encoder_close(out->encoder);
        out->encoder = nullptr;
        return err;
    }

    // Create LVGL encoder indev.
    out->indev = lv_indev_create();
    if (out->indev == nullptr) {
        BLUSYS_LOGE(kTag, "lv_indev_create failed");
        blusys_encoder_close(out->encoder);
        out->encoder = nullptr;
        return BLUSYS_ERR_NO_MEM;
    }
    lv_indev_set_type(out->indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(out->indev, indev_read_cb);

    // Create focus group and bind to indev.
    out->group = blusys::create_encoder_group();
    if (out->group != nullptr) {
        lv_indev_set_group(out->indev, out->group);
    }

    BLUSYS_LOGI(kTag, "input bridge opened");
    return BLUSYS_OK;
}

void input_bridge_close(input_bridge *bridge)
{
    if (bridge == nullptr) {
        return;
    }

    g_encoder_state.framework_runtime = nullptr;

    if (bridge->group != nullptr) {
        lv_group_delete(bridge->group);
        bridge->group = nullptr;
    }
    if (bridge->indev != nullptr) {
        lv_indev_delete(bridge->indev);
        bridge->indev = nullptr;
    }
    if (bridge->encoder != nullptr) {
        blusys_encoder_close(bridge->encoder);
        bridge->encoder = nullptr;
    }

    BLUSYS_LOGI(kTag, "input bridge closed");
}

void input_bridge_attach_screen(input_bridge *bridge, lv_obj_t *screen)
{
    if (bridge == nullptr || bridge->group == nullptr || screen == nullptr) {
        return;
    }
    lv_group_remove_all_objs(bridge->group);
    blusys::auto_focus_screen(screen, bridge->group);
}

}  // namespace blusys

#endif  // BLUSYS_FRAMEWORK_HAS_UI
