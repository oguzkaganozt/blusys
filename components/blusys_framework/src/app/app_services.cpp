#include "blusys/app/app_services.hpp"

#include "blusys/app/capabilities/storage.hpp"

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/app/view/screen_router.hpp"
#endif

namespace blusys::app {

void app_services::navigate_to(std::uint32_t route_id, const void *params)
{
    if (route_sink_ != nullptr) {
        route_sink_->submit(blusys::framework::route::set_root(route_id, params));
    }
}

void app_services::navigate_push(std::uint32_t route_id, const void *params)
{
    if (route_sink_ != nullptr) {
        route_sink_->submit(blusys::framework::route::push(route_id, params));
    }
}

void app_services::navigate_replace(std::uint32_t route_id, const void *params)
{
    if (route_sink_ != nullptr) {
        route_sink_->submit(blusys::framework::route::replace(route_id, params));
    }
}

void app_services::navigate_back()
{
    if (route_sink_ != nullptr) {
        route_sink_->submit(blusys::framework::route::pop());
    }
}

void app_services::show_overlay(std::uint32_t overlay_id, const void *params)
{
    if (route_sink_ != nullptr) {
        route_sink_->submit(blusys::framework::route::show_overlay(overlay_id, params));
    }
}

void app_services::hide_overlay(std::uint32_t overlay_id)
{
    if (route_sink_ != nullptr) {
        route_sink_->submit(blusys::framework::route::hide_overlay(overlay_id));
    }
}

#ifdef BLUSYS_FRAMEWORK_HAS_UI

std::size_t app_services::navigation_stack_depth() const
{
    if (screen_router_ != nullptr) {
        return screen_router_->stack_depth();
    }
    return 0;
}

bool app_services::can_navigate_back() const
{
    return navigation_stack_depth() > 1;
}

view::overlay_manager *app_services::overlay_manager() const
{
    if (screen_router_ != nullptr) {
        return &screen_router_->overlays();
    }
    return nullptr;
}

#endif  // BLUSYS_FRAMEWORK_HAS_UI

#ifdef ESP_PLATFORM

blusys_fs_t *app_services::spiffs() const
{
    if (storage_for_fs_ != nullptr) {
        return storage_for_fs_->spiffs();
    }
    return nullptr;
}

blusys_fatfs_t *app_services::fatfs() const
{
    if (storage_for_fs_ != nullptr) {
        return storage_for_fs_->fatfs();
    }
    return nullptr;
}

#endif  // ESP_PLATFORM

}  // namespace blusys::app
