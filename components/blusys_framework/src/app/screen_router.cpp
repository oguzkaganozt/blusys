#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/view/screen_router.hpp"
#include "blusys/app/view/shell.hpp"
#include "blusys/app/app_ctx.hpp"
#include "blusys/log.h"

static const char *TAG = "screen_router";

namespace blusys::app::view {

void screen_router::submit(const blusys::framework::route_command &command)
{
    using T = blusys::framework::route_command_type;

    if (command.type == T::show_overlay || command.type == T::hide_overlay) {
        overlays_.submit(command);
        return;
    }

    if (ctx_ == nullptr) {
        BLUSYS_LOGW(TAG, "route command received before ctx was bound, ignored");
        return;
    }

    if (!screens_.navigate(command, *ctx_)) {
        BLUSYS_LOGW(TAG, "navigation failed for route op=%s id=%lu",
                    blusys::framework::route_command_type_name(command.type),
                    static_cast<unsigned long>(command.id));
    }
}

void screen_router::sync_shell_chrome(shell &s)
{
    shell_sync_tabs_for_nav_stack(s, screens_);
}

bool screen_router::register_screen(std::uint32_t    route_id,
                                     screen_create_fn  create_fn,
                                     screen_destroy_fn destroy_fn)
{
    return screens_.register_screen(route_id, create_fn, destroy_fn);
}

bool screen_router::register_screen(std::uint32_t         route_id,
                                     screen_create_fn       create_fn,
                                     screen_destroy_fn      destroy_fn,
                                     const screen_lifecycle &lifecycle)
{
    return screens_.register_screen(route_id, create_fn, destroy_fn, lifecycle);
}

void screen_router::set_screen_changed_callback(void (*fn)(lv_obj_t *screen, void *user_ctx),
                                                 void *user_ctx)
{
    screens_.set_screen_changed_callback(fn, user_ctx);
}

}  // namespace blusys::app::view

#endif  // BLUSYS_FRAMEWORK_HAS_UI
