#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/touch_bridge.hpp"
#include "blusys/framework/core/intent.hpp"
#include "blusys/log.h"

#include <cstdlib>

namespace blusys::app {

namespace {

constexpr const char *kTag = "touch_bridge";
constexpr int kSwipeThresholdPx = 30;

struct touch_state {
    blusys::framework::runtime *runtime         = nullptr;
    bool                        enable_gestures = true;
    lv_point_t                  press_point     = {};
    bool                        pressed         = false;
};

static touch_state g_touch_state{};

// Indev-level event callback — fires for every pointer event regardless
// of which screen is active, because it is registered on the indev
// itself rather than on an individual LVGL object.
void indev_event_cb(lv_event_t *e)
{
    const lv_event_code_t code = lv_event_get_code(e);

    if (g_touch_state.runtime == nullptr) {
        return;
    }

    switch (code) {
    case LV_EVENT_PRESSED: {
        g_touch_state.pressed = true;
        lv_indev_t *indev = static_cast<lv_indev_t *>(lv_event_get_target(e));
        if (indev != nullptr) {
            lv_indev_get_point(indev, &g_touch_state.press_point);
        }
        break;
    }

    case LV_EVENT_RELEASED: {
        if (!g_touch_state.pressed) {
            break;
        }
        g_touch_state.pressed = false;

        if (g_touch_state.enable_gestures) {
            lv_point_t release_point = {};
            lv_indev_t *indev = static_cast<lv_indev_t *>(lv_event_get_target(e));
            if (indev != nullptr) {
                lv_indev_get_point(indev, &release_point);
            }

            const int dx = release_point.x - g_touch_state.press_point.x;
            const int dy = release_point.y - g_touch_state.press_point.y;

            if (std::abs(dx) > kSwipeThresholdPx && std::abs(dx) > std::abs(dy)) {
                if (dx > 0) {
                    g_touch_state.runtime->post_intent(
                        blusys::framework::intent::focus_prev);
                } else {
                    g_touch_state.runtime->post_intent(
                        blusys::framework::intent::focus_next);
                }
                break;
            }
        }
        g_touch_state.runtime->post_intent(blusys::framework::intent::confirm);
        break;
    }

    case LV_EVENT_LONG_PRESSED:
        g_touch_state.runtime->post_intent(blusys::framework::intent::long_press);
        break;

    default:
        break;
    }
}

}  // namespace

blusys_err_t touch_bridge_open(const touch_bridge_config &config,
                                lv_indev_t *pointer_indev,
                                touch_bridge *out)
{
    if (out == nullptr || pointer_indev == nullptr || config.framework_runtime == nullptr) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    g_touch_state.runtime         = config.framework_runtime;
    g_touch_state.enable_gestures = config.enable_gestures;
    g_touch_state.pressed         = false;

    // Register on the indev itself so the callback survives screen changes.
    lv_indev_add_event_cb(pointer_indev, indev_event_cb, LV_EVENT_ALL, nullptr);

    out->indev = pointer_indev;

    BLUSYS_LOGI(kTag, "touch bridge opened");
    return BLUSYS_OK;
}

void touch_bridge_close(touch_bridge *bridge)
{
    if (bridge == nullptr) {
        return;
    }
    g_touch_state.runtime = nullptr;
    // Note: lv_indev_remove_event_cb is not available in LVGL 9.x;
    // nulling the runtime pointer effectively disables the callback.
    bridge->indev = nullptr;
    BLUSYS_LOGI(kTag, "touch bridge closed");
}

}  // namespace blusys::app

#endif  // BLUSYS_FRAMEWORK_HAS_UI
