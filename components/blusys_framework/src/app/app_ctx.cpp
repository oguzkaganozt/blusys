#include "blusys/app/app_ctx.hpp"
#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/app/capabilities/storage.hpp"

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
