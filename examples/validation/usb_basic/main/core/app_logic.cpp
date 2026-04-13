#include "core/app_logic.hpp"

#include "blusys/log.h"

namespace usb_basic {

namespace {

constexpr const char *kTag = "usb_basic";

void sync_usb_state(blusys::app::app_ctx &ctx, app_state &state)
{
    if (const auto *usb = ctx.usb(); usb != nullptr) {
        state.usb_ready = usb->capability_ready;
        state.device_ready = usb->device_ready || usb->host_ready;
        state.device_connected = usb->device_connected || usb->host_attached;
    }
}

}  // namespace

void update(blusys::app::app_ctx &ctx, app_state &state, const action &event)
{
    if (event.tag != action_tag::capability_event) {
        return;
    }

    using CET = blusys::app::capability_event_tag;
    switch (event.cap_event.tag) {
    case CET::usb_device_ready:
    case CET::usb_host_ready:
        sync_usb_state(ctx, state);
        BLUSYS_LOGI(kTag, "usb role ready");
        break;
    case CET::usb_device_connected:
    case CET::usb_host_attached:
        sync_usb_state(ctx, state);
        BLUSYS_LOGI(kTag, "usb attachment detected");
        break;
    case CET::usb_device_disconnected:
    case CET::usb_host_detached:
        sync_usb_state(ctx, state);
        BLUSYS_LOGI(kTag, "usb attachment cleared");
        break;
    case CET::usb_ready:
        sync_usb_state(ctx, state);
        BLUSYS_LOGI(kTag, "usb capability ready");
        break;
    default:
        break;
    }
}

}  // namespace usb_basic
