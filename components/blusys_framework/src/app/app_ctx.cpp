#include "blusys/app/app_ctx.hpp"
#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/app/capabilities/storage.hpp"
#include "blusys/app/capabilities/bluetooth.hpp"
#include "blusys/app/capabilities/ota.hpp"
#include "blusys/app/capabilities/diagnostics.hpp"
#include "blusys/app/capabilities/telemetry.hpp"
#include "blusys/app/capabilities/provisioning.hpp"

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/app/view/screen_router.hpp"
#endif

namespace blusys::app {

// ---- navigation ----

void app_ctx::navigate_to(std::uint32_t route_id, const void *params)
{
    if (route_sink_ != nullptr) {
        route_sink_->submit(blusys::framework::route::set_root(route_id, params));
    }
}

void app_ctx::navigate_push(std::uint32_t route_id, const void *params)
{
    if (route_sink_ != nullptr) {
        route_sink_->submit(blusys::framework::route::push(route_id, params));
    }
}

void app_ctx::navigate_replace(std::uint32_t route_id, const void *params)
{
    if (route_sink_ != nullptr) {
        route_sink_->submit(blusys::framework::route::replace(route_id, params));
    }
}

void app_ctx::navigate_back()
{
    if (route_sink_ != nullptr) {
        route_sink_->submit(blusys::framework::route::pop());
    }
}

void app_ctx::show_overlay(std::uint32_t overlay_id, const void *params)
{
    if (route_sink_ != nullptr) {
        route_sink_->submit(blusys::framework::route::show_overlay(overlay_id, params));
    }
}

void app_ctx::hide_overlay(std::uint32_t overlay_id)
{
    if (route_sink_ != nullptr) {
        route_sink_->submit(blusys::framework::route::hide_overlay(overlay_id));
    }
}

// ---- feedback ----

void app_ctx::emit_feedback(blusys::framework::feedback_channel channel,
                            blusys::framework::feedback_pattern pattern,
                            std::uint32_t value)
{
    if (feedback_bus_ != nullptr) {
        feedback_bus_->emit({
            .channel = channel,
            .pattern = pattern,
            .value   = value,
            .payload = nullptr,
        });
    }
}

#ifdef BLUSYS_FRAMEWORK_HAS_UI

std::size_t app_ctx::navigation_stack_depth() const
{
    if (screen_router_ != nullptr) {
        return screen_router_->stack_depth();
    }
    return 0;
}

bool app_ctx::can_navigate_back() const
{
    return navigation_stack_depth() > 1;
}

blusys::app::view::overlay_manager *blusys::app::app_ctx::overlay_manager() const
{
    if (screen_router_ != nullptr) {
        return &screen_router_->overlays();
    }
    return nullptr;
}

#endif  // BLUSYS_FRAMEWORK_HAS_UI

// ---- capability queries ----

const connectivity_status *app_ctx::connectivity() const
{
    if (connectivity_ != nullptr) {
        return &connectivity_->status();
    }
    return nullptr;
}

const storage_status *app_ctx::storage() const
{
    if (storage_ != nullptr) {
        return &storage_->status();
    }
    return nullptr;
}

const bluetooth_status *app_ctx::bluetooth() const
{
    if (bluetooth_ != nullptr) {
        return &bluetooth_->status();
    }
    return nullptr;
}

const ota_status *app_ctx::ota() const
{
    if (ota_ != nullptr) {
        return &ota_->status();
    }
    return nullptr;
}

const diagnostics_status *app_ctx::diagnostics() const
{
    if (diagnostics_ != nullptr) {
        return &diagnostics_->status();
    }
    return nullptr;
}

const telemetry_status *app_ctx::telemetry() const
{
    if (telemetry_ != nullptr) {
        return &telemetry_->status();
    }
    return nullptr;
}

const provisioning_status *app_ctx::provisioning() const
{
    if (provisioning_ != nullptr) {
        return &provisioning_->status();
    }
    return nullptr;
}

mqtt_host_capability *app_ctx::mqtt_host()
{
    return mqtt_host_;
}

blusys_err_t app_ctx::request_connectivity_reconnect()
{
    if (connectivity_ == nullptr) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    return connectivity_->request_reconnect();
}

#ifdef ESP_PLATFORM
blusys_fs_t *app_ctx::spiffs() const
{
    if (storage_ != nullptr) {
        return storage_->spiffs();
    }
    return nullptr;
}

blusys_fatfs_t *app_ctx::fatfs() const
{
    if (storage_ != nullptr) {
        return storage_->fatfs();
    }
    return nullptr;
}
#endif

}  // namespace blusys::app
