#include "blusys/framework/ui/controller/navigation_controller.hpp"

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/composition/overlay_manager.hpp"
#include "blusys/framework/ui/composition/screen_router.hpp"
#include "blusys/framework/ui/composition/shell.hpp"

namespace blusys {

void navigation_controller::bind(screen_router *router, shell *s)
{
    router_ = router;
    shell_  = s;
    if (router_ != nullptr) {
        router_->set_screen_changed_callback(&transition_trampoline, this);
    }
}

bool navigation_controller::register_screen(std::uint32_t route_id,
                                             screen_create_fn create,
                                             screen_destroy_fn destroy,
                                             const screen_lifecycle &lifecycle)
{
    return router_ != nullptr && router_->register_screen(route_id, create, destroy, lifecycle);
}

bool navigation_controller::register_screens(app_ctx *ctx,
                                              const screen_registration *rows,
                                              std::size_t count)
{
    return router_ != nullptr && router_->register_screens(ctx, rows, count);
}

bool navigation_controller::register_overlay(std::uint32_t id, lv_obj_t *overlay)
{
    return router_ != nullptr && router_->overlays().register_overlay(id, overlay);
}

lv_obj_t *navigation_controller::create_overlay(lv_obj_t *parent,
                                                  std::uint32_t id,
                                                  const overlay_config &config)
{
    if (router_ == nullptr) {
        return nullptr;
    }
    return overlay_create(parent, id, config, router_->overlays());
}

void navigation_controller::set_tab_items(const shell_tab_item *items, std::size_t count)
{
    if (shell_ != nullptr && router_ != nullptr) {
        shell_set_tabs(*shell_, items, count, router_);
    }
}

lv_obj_t *navigation_controller::content_area() const
{
    return shell_ != nullptr ? shell_->content_area : nullptr;
}

lv_obj_t *navigation_controller::shell_root() const
{
    return shell_ != nullptr ? shell_->root : nullptr;
}

void navigation_controller::set_title(const char *title) const
{
    if (shell_ != nullptr) {
        shell_set_title(*shell_, title);
    }
}

lv_obj_t *navigation_controller::status_surface() const
{
    return shell_ != nullptr ? shell_->status : nullptr;
}

void navigation_controller::submit(const route_command &cmd)
{
    if (router_ != nullptr) {
        router_->submit(cmd);
    }
}

// static
void navigation_controller::transition_trampoline(lv_obj_t * /*screen*/, void *user_ctx)
{
    static_cast<navigation_controller *>(user_ctx)->on_transition();
}

void navigation_controller::on_transition()
{
    if (router_ == nullptr) {
        return;
    }
    router_->focus_scopes().refresh_current();
    if (shell_ != nullptr) {
        router_->sync_shell_chrome(*shell_);
        shell_set_back_visible(*shell_, router_->stack_depth() > 1);
    }
}

}  // namespace blusys

#endif  // BLUSYS_FRAMEWORK_HAS_UI
